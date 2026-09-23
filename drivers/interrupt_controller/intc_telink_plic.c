/*
 * Copyright (c) 2017 Jean-Paul Etienne <fractalclone@gmail.com>
 * Copyright (c) 2023 Meta
 * Contributors: 2018 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT telink_plic

/**
 * @brief Platform Level Interrupt Controller (PLIC) driver
 *        for RISC-V processors
 */

#include <stdlib.h>

#include "sw_isr_common.h"

#include <zephyr/debug/symtab.h>
#include <zephyr/kernel.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/device.h>
#include <zephyr/devicetree/interrupt_controller.h>
#include <zephyr/shell/shell.h>

#include <zephyr/sw_isr_table.h>
#include <zephyr/drivers/interrupt_controller/riscv_plic.h>
#include <zephyr/irq.h>

#define PLIC_BASE_ADDR(n) DT_INST_REG_ADDR(n)
/*
 * These registers' offset are defined in the RISCV PLIC specs, see:
 * https://github.com/riscv/riscv-plic-spec
 */
#define CONTEXT_BASE 0x200000
#define CONTEXT_SIZE 0x1000
#define CONTEXT_THRESHOLD 0x00
#define CONTEXT_CLAIM 0x04
#define CONTEXT_ENABLE_BASE 0x2000
#define CONTEXT_ENABLE_SIZE 0x80
#define CONTEXT_PENDING_BASE 0x1000

/* PLIC registers are 32-bit memory-mapped */
#define PLIC_REG_SIZE 32
#define PLIC_REG_MASK BIT_MASK(LOG2(PLIC_REG_SIZE))

#ifdef CONFIG_TEST_INTC_PLIC
#define INTC_PLIC_STATIC
#define INTC_PLIC_STATIC_INLINE
#else
#define INTC_PLIC_STATIC static
#define INTC_PLIC_STATIC_INLINE static inline
#endif /* CONFIG_TEST_INTC_PLIC */

typedef void (*riscv_plic_irq_config_func_t)(void);
struct plic_config {
	mem_addr_t prio;
	mem_addr_t irq_en;
	mem_addr_t reg;
	uint32_t max_prio;
	/* Number of IRQs that the PLIC physically supports */
	uint32_t riscv_ndev;
	/* Number of IRQs supported in this driver */
	uint32_t nr_irqs;
	uint32_t irq;
	riscv_plic_irq_config_func_t irq_config_func;
	struct _isr_table_entry *isr_table;
	const uint32_t *const hart_context;
};

struct plic_stats {
	uint16_t *const irq_count;
};

struct plic_data {
	struct k_spinlock lock;

#ifdef CONFIG_PLIC_TELINK_SHELL_IRQ_COUNT
	struct plic_stats stats;
#endif /* CONFIG_PLIC_TELINK_SHELL_IRQ_COUNT */
};

static uint32_t save_irq[CONFIG_MP_MAX_NUM_CPUS];
static const struct device *save_dev[CONFIG_MP_MAX_NUM_CPUS];

INTC_PLIC_STATIC_INLINE uint32_t local_irq_to_reg_index(uint32_t local_irq)
{
	return local_irq >> LOG2(PLIC_REG_SIZE);
}

INTC_PLIC_STATIC_INLINE uint32_t local_irq_to_reg_offset(uint32_t local_irq)
{
	return local_irq_to_reg_index(local_irq) * sizeof(uint32_t);
}

static inline uint32_t get_plic_enabled_size(const struct device *dev)
{
	const struct plic_config *config = dev->config;

	return local_irq_to_reg_index(config->nr_irqs + PLIC_REG_SIZE - 1);
}

static ALWAYS_INLINE uint32_t get_hart_context(const struct device *dev, uint32_t hartid)
{
	const struct plic_config *config = dev->config;

	return config->hart_context[hartid];
}

static ALWAYS_INLINE uint32_t get_irq_cpumask(const struct device *dev, uint32_t local_irq)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(local_irq);

	return 0x1;
}

static inline mem_addr_t get_context_en_addr(const struct device *dev, uint32_t cpu_num)
{
	const struct plic_config *config = dev->config;
	uint32_t hartid;
	/*
	 * We want to return the irq_en address for the context of given hart.
	 */
#if CONFIG_MP_MAX_NUM_CPUS > 1
	hartid = _kernel.cpus[cpu_num].arch.hartid;
#else
	hartid = arch_proc_id();
#endif
	return config->irq_en + get_hart_context(dev, hartid) * CONTEXT_ENABLE_SIZE;
}

static inline mem_addr_t get_claim_complete_addr(const struct device *dev)
{
	const struct plic_config *config = dev->config;

	/*
	 * We want to return the claim complete addr for the hart's context.
	 */

	return config->reg + get_hart_context(dev, arch_proc_id()) * CONTEXT_SIZE + CONTEXT_CLAIM;
}

static inline mem_addr_t get_threshold_priority_addr(const struct device *dev, uint32_t cpu_num)
{
	const struct plic_config *config = dev->config;
	uint32_t hartid;

#if CONFIG_MP_MAX_NUM_CPUS > 1
	hartid = _kernel.cpus[cpu_num].arch.hartid;
#else
	hartid = arch_proc_id();
#endif

	return config->reg + (get_hart_context(dev, hartid) * CONTEXT_SIZE);
}

static ALWAYS_INLINE uint32_t local_irq_to_irq(const struct device *dev, uint32_t local_irq)
{
	const struct plic_config *config = dev->config;

	return irq_to_level_2(local_irq) | config->irq;
}

/**
 * @brief Determine the PLIC device from the IRQ
 *
 * @param irq IRQ number
 *
 * @return PLIC device of that IRQ
 */
static inline const struct device *get_plic_dev_from_irq(uint32_t irq)
{
#ifdef CONFIG_DYNAMIC_INTERRUPTS
	return z_get_sw_isr_device_from_irq(irq);
#else
	return DEVICE_DT_INST_GET(0);
#endif
}

static void plic_irq_enable_set_state(uint32_t irq, bool enable)
{
	const struct device *dev = get_plic_dev_from_irq(irq);
	const uint32_t local_irq = irq_from_level_2(irq);

	for (uint32_t cpu_num = 0; cpu_num < arch_num_cpus(); cpu_num++) {
		mem_addr_t en_addr =
			get_context_en_addr(dev, cpu_num) + local_irq_to_reg_offset(local_irq);
		uint32_t en_value;

		en_value = sys_read32(en_addr);
		WRITE_BIT(en_value, local_irq & PLIC_REG_MASK,
			  enable ? (get_irq_cpumask(dev, local_irq) & BIT(cpu_num)) != 0 : false);
		sys_write32(en_value, en_addr);
	}
}

/**
 * @brief Enable a riscv PLIC-specific interrupt line
 *
 * This routine enables a RISCV PLIC-specific interrupt line.
 * riscv_plic_irq_enable is called by RISCV_PRIVILEGED
 * arch_irq_enable function to enable external interrupts for
 * IRQS level == 2, whenever CONFIG_RISCV_HAS_PLIC variable is set.
 *
 * @param irq IRQ number to enable
 */
void riscv_plic_irq_enable(uint32_t irq)
{
	const struct device *dev = get_plic_dev_from_irq(irq);
	struct plic_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	plic_irq_enable_set_state(irq, true);

	k_spin_unlock(&data->lock, key);
}

/**
 * @brief Disable a riscv PLIC-specific interrupt line
 *
 * This routine disables a RISCV PLIC-specific interrupt line.
 * riscv_plic_irq_disable is called by RISCV_PRIVILEGED
 * arch_irq_disable function to disable external interrupts, for
 * IRQS level == 2, whenever CONFIG_RISCV_HAS_PLIC variable is set.
 *
 * @param irq IRQ number to disable
 */
void riscv_plic_irq_disable(uint32_t irq)
{
	const struct device *dev = get_plic_dev_from_irq(irq);
	struct plic_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	plic_irq_enable_set_state(irq, false);

	k_spin_unlock(&data->lock, key);
}

/* Check if the local IRQ of a PLIC instance is enabled */
static int local_irq_is_enabled(const struct device *dev, uint32_t local_irq)
{
	uint32_t bit_position = local_irq & PLIC_REG_MASK;
	int is_enabled = 1;

	for (uint32_t cpu_num = 0; cpu_num < arch_num_cpus(); cpu_num++) {
		mem_addr_t en_addr =
			get_context_en_addr(dev, cpu_num) + local_irq_to_reg_offset(local_irq);
		uint32_t en_value = sys_read32(en_addr);

		is_enabled &= !!(en_value & BIT(bit_position));
	}

	return is_enabled;
}

/**
 * @brief Check if a riscv PLIC-specific interrupt line is enabled
 *
 * This routine checks if a RISCV PLIC-specific interrupt line is enabled.
 * @param irq IRQ number to check
 *
 * @return 1 or 0
 */
int riscv_plic_irq_is_enabled(uint32_t irq)
{
	const struct device *dev = get_plic_dev_from_irq(irq);
	struct plic_data *data = dev->data;
	const uint32_t local_irq = irq_from_level_2(irq);
	int ret = 0;

	K_SPINLOCK(&data->lock) {
		ret = local_irq_is_enabled(dev, local_irq);
	}

	return ret;
}

/**
 * @brief Set priority of a riscv PLIC-specific interrupt line
 *
 * This routine set the priority of a RISCV PLIC-specific interrupt line.
 * riscv_plic_irq_set_prio is called by riscv arch_irq_priority_set to set
 * the priority of an interrupt whenever CONFIG_RISCV_HAS_PLIC variable is set.
 *
 * @param irq IRQ number for which to set priority
 * @param priority Priority of IRQ to set to
 */
void riscv_plic_set_priority(uint32_t irq, uint32_t priority)
{
	const struct device *dev = get_plic_dev_from_irq(irq);
	const struct plic_config *config = dev->config;
	const uint32_t local_irq = irq_from_level_2(irq);
	mem_addr_t prio_addr = config->prio + (local_irq * sizeof(uint32_t));

	if (priority > config->max_prio) {
		priority = config->max_prio;
	}

	sys_write32(priority, prio_addr);
}

/**
 * @brief Get riscv PLIC-specific interrupt line causing an interrupt
 *
 * This routine returns the RISCV PLIC-specific interrupt line causing an
 * interrupt.
 *
 * @param dev Optional device pointer to get the interrupt line's controller
 *
 * @return PLIC-specific interrupt line causing an interrupt.
 */
unsigned int riscv_plic_get_irq(void)
{
	return save_irq[arch_curr_cpu()->id];
}

/**
 * @brief Get riscv PLIC causing an interrupt
 *
 * This routine returns the RISCV PLIC device causing an interrupt.
 *
 * @return PLIC device causing an interrupt.
 */
const struct device *riscv_plic_get_dev(void)
{
	return save_dev[arch_curr_cpu()->id];
}

#ifdef CONFIG_PLIC_TELINK_SHELL_IRQ_COUNT
/**
 * If there's more than one core, irq_count points to a 2D-array: irq_count[NUM_CPUs + 1][nr_irqs]
 *
 *    i.e. NUM_CPUs == 2:
 *      CPU 0    [0 ... nr_irqs - 1]
 *      CPU 1    [0 ... nr_irqs - 1]
 *      TOTAL    [0 ... nr_irqs - 1]
 */
static ALWAYS_INLINE uint16_t *get_irq_hit_count_cpu(const struct device *dev, int cpu,
						     uint32_t local_irq)
{
	const struct plic_config *config = dev->config;
	const struct plic_data *data = dev->data;
	uint32_t offset = local_irq;

	if (CONFIG_MP_MAX_NUM_CPUS > 1) {
		offset = cpu * config->nr_irqs + local_irq;
	}

	return &data->stats.irq_count[offset];
}

static ALWAYS_INLINE uint16_t *get_irq_hit_count_total(const struct device *dev, uint32_t local_irq)
{
	const struct plic_config *config = dev->config;
	const struct plic_data *data = dev->data;
	uint32_t offset = local_irq;

	if (CONFIG_MP_MAX_NUM_CPUS > 1) {
		offset = arch_num_cpus() * config->nr_irqs + local_irq;
	}

	return &data->stats.irq_count[offset];
}

void plic_irq_inc_irq_count(const struct device *dev, uint8_t cpu_id, uint32_t local_irq)
{
	uint16_t *cpu_count = get_irq_hit_count_cpu(dev, cpu_id, local_irq);
	uint16_t *total_count = get_irq_hit_count_total(dev, local_irq);

	/* Cap the count at __UINT16_MAX__ */
	if (*total_count < __UINT16_MAX__) {
		(*cpu_count)++;
		if (CONFIG_MP_MAX_NUM_CPUS > 1) {
			(*total_count)++;
		}
	}
}
#endif /* CONFIG_PLIC_TELINK_SHELL_IRQ_COUNT */

static void plic_irq_handler(const struct device *dev)
{
	const struct plic_config *config = dev->config;
	mem_addr_t claim_complete_addr = get_claim_complete_addr(dev);
	struct _isr_table_entry *ite;
	uint32_t cpu_id = arch_curr_cpu()->id;
	/* Get the IRQ number generating the interrupt */
	const uint32_t local_irq = sys_read32(claim_complete_addr);

#ifdef CONFIG_PLIC_TELINK_SHELL_IRQ_COUNT
	plic_irq_inc_irq_count(dev, cpu_id, local_irq);
#endif /* CONFIG_PLIC_TELINK_SHELL_IRQ_COUNT */

	/*
	 * Note: Because PLIC only supports multicast of interrupt, all enabled
	 * targets will receive interrupt notification. Only the fastest target
	 * will claim this interrupt, and other targets will claim ID 0 if
	 * no other pending interrupt now.
	 *
	 * (by RISC-V Privileged Architecture v1.10)
	 */
	if ((CONFIG_MP_MAX_NUM_CPUS > 1) && (local_irq == 0U)) {
		return;
	}

	/*
	 * Save IRQ in save_irq. To be used, if need be, by
	 * subsequent handlers registered in the _sw_isr_table table,
	 * as IRQ number held by the claim_complete register is
	 * cleared upon read.
	 */
	save_irq[cpu_id] = local_irq;
	save_dev[cpu_id] = dev;

	/*
	 * On Telink retention targets, the CPU can occasionally take a machine
	 * external interrupt while PLIC claim/complete returns 0. In this case
	 * there is no pending PLIC source to service or complete, so ignore it.
	 */
	if (local_irq == 0U) {
		return;
	}

	/*
	 * If the IRQ is out of range, call z_irq_spurious.
	 * A call to z_irq_spurious will not return.
	 */
	if (local_irq >= config->nr_irqs) {
		z_irq_spurious(NULL);
	}

	/* Call the corresponding IRQ handler in _sw_isr_table */
	ite = &config->isr_table[local_irq];
	ite->isr(ite->arg);

	/*
	 * Write to claim_complete register to indicate to
	 * PLIC controller that the IRQ has been handled
	 * for level triggered interrupts.
	 */
	sys_write32(local_irq, claim_complete_addr);
}

/**
 * @brief Initialize the Platform Level Interrupt Controller
 *
 * @param dev PLIC device struct
 *
 * @retval 0 on success.
 */
static int plic_init(const struct device *dev)
{
	const struct plic_config *config = dev->config;
	mem_addr_t en_addr, thres_prio_addr;
	mem_addr_t prio_addr = config->prio;

	/* Iterate through each of the contexts, HART + PRIV */
	for (uint32_t cpu_num = 0; cpu_num < arch_num_cpus(); cpu_num++) {
		en_addr = get_context_en_addr(dev, cpu_num);
		thres_prio_addr = get_threshold_priority_addr(dev, cpu_num);

		/* Ensure that all interrupts are disabled initially */
		for (uint32_t i = 0; i < get_plic_enabled_size(dev); i++) {
			sys_write32(0U, en_addr + (i * sizeof(uint32_t)));
		}

		/* Set threshold priority to 0 */
		sys_write32(0U, thres_prio_addr);
	}

	/* Set priority of each interrupt line to 0 initially */
	for (uint32_t i = 1; i < config->nr_irqs; i++) {
		sys_write32(0U, prio_addr + (i * sizeof(uint32_t)));
	}

	/* Configure IRQ for PLIC driver */
	config->irq_config_func();

	return 0;
}

#ifdef CONFIG_PLIC_TELINK_SHELL
static inline int parse_device(const struct shell *sh, size_t argc, char *argv[],
			       const struct device **plic)
{
	ARG_UNUSED(argc);

	*plic = device_get_binding(argv[1]);
	if (*plic == NULL) {
		shell_error(sh, "PLIC device (%s) not found!\n", argv[1]);
		return -ENODEV;
	}

	return 0;
}

#ifdef CONFIG_PLIC_TELINK_SHELL_IRQ_COUNT
static int cmd_stats_get(const struct shell *sh, size_t argc, char *argv[])
{
	const struct device *dev;
	int ret = parse_device(sh, argc, argv, &dev);
	uint16_t min_hit = 0;

	if (ret != 0) {
		return ret;
	}

	const struct plic_config *config = dev->config;

	if (argc > 2) {
		min_hit = (uint16_t)shell_strtoul(argv[2], 10, &ret);
		if (ret != 0) {
			shell_error(sh, "Failed to parse %s: %d", argv[2], ret);
			return ret;
		}
		shell_print(sh, "IRQ line with > %d hits:", min_hit);
	}

	shell_fprintf(sh, SHELL_NORMAL, "   IRQ");
	for (int cpu_id = 0; cpu_id < arch_num_cpus(); cpu_id++) {
		shell_fprintf(sh, SHELL_NORMAL, "  CPU%2d", cpu_id);
	}
	if (CONFIG_MP_MAX_NUM_CPUS > 1) {
		shell_fprintf(sh, SHELL_NORMAL, "  Total");
	}
	shell_fprintf(sh, SHELL_NORMAL, "\tISR(ARG)\n");

	for (int i = 0; i < config->nr_irqs; i++) {
		uint16_t *total_count = get_irq_hit_count_total(dev, i);

		if (*total_count <= min_hit) {
			/* Skips printing if total_hit is lesser than min_hit */
			continue;
		}

		shell_fprintf(sh, SHELL_NORMAL, "  %4d", i); /* IRQ number */
		/* Print the IRQ hit counts on each CPU */
		for (int cpu_id = 0; cpu_id < arch_num_cpus(); cpu_id++) {
			uint16_t *cpu_count = get_irq_hit_count_cpu(dev, cpu_id, i);

			shell_fprintf(sh, SHELL_NORMAL, "  %5d", *cpu_count);
		}
		if (CONFIG_MP_MAX_NUM_CPUS > 1) {
			/* If there's > 1 CPU, print the total hit count at the end */
			shell_fprintf(sh, SHELL_NORMAL, "  %5d", *total_count);
		}
#ifdef CONFIG_SYMTAB
		const char *name =
			symtab_find_symbol_name((uintptr_t)config->isr_table[i].isr, NULL);

		shell_fprintf(sh, SHELL_NORMAL, "\t%s(%p)\n", name, config->isr_table[i].arg);
#else
		shell_fprintf(sh, SHELL_NORMAL, "\t%p(%p)\n", (void *)config->isr_table[i].isr,
			      config->isr_table[i].arg);
#endif /* CONFIG_SYMTAB */
	}
	shell_print(sh, "");

	return 0;
}

static int cmd_stats_clear(const struct shell *sh, size_t argc, char *argv[])
{
	const struct device *dev;
	int ret = parse_device(sh, argc, argv, &dev);

	if (ret != 0) {
		return ret;
	}

	const struct plic_data *data = dev->data;
	const struct plic_config *config = dev->config;
	struct plic_stats stat = data->stats;

	memset(stat.irq_count, 0,
	       config->nr_irqs *
		       COND_CODE_1(CONFIG_MP_MAX_NUM_CPUS, (1),
				   (UTIL_INC(CONFIG_MP_MAX_NUM_CPUS))) *
		       sizeof(uint16_t));

	shell_print(sh, "Cleared stats of %s.\n", dev->name);

	return 0;
}
#endif /* CONFIG_PLIC_TELINK_SHELL_IRQ_COUNT */

/* Device name autocompletion support */
static void device_name_get(size_t idx, struct shell_static_entry *entry)
{
	const struct device *dev = shell_device_lookup(idx, "interrupt-controller");

	entry->syntax = (dev != NULL) ? dev->name : NULL;
	entry->handler = NULL;
	entry->help = NULL;
	entry->subcmd = NULL;
}

static int cmd_info(const struct shell *sh, size_t argc, char *argv[])
{
	const struct device *dev;
	int ret = parse_device(sh, argc, argv, &dev);

	if (ret != 0) {
		return ret;
	}

	shell_print(sh, "PLIC %s information:", dev->name);

	const struct plic_config *config = dev->config;
	uint32_t features = sys_read32(config->prio);

	shell_print(sh, "vectored mode %s, preemption %s", features & 0x2 ? "on" : "off",
		    features & 0x1 ? "on" : "off");

	for (uint32_t cpu_num = 0; cpu_num < arch_num_cpus(); cpu_num++) {
		mem_addr_t en_addr = get_context_en_addr(dev, cpu_num);

		if (CONFIG_MP_MAX_NUM_CPUS > 1) {
			shell_print(sh, "CPU%u:", cpu_num);
		}
		shell_print(sh, "threshold priority: %u:",
			    sys_read32(get_threshold_priority_addr(dev, cpu_num)));
		shell_fprintf(sh, SHELL_NORMAL, "enabled interrupts num(prio):");
		for (uint32_t num = 0; num < config->nr_irqs; num++) {
			if (sys_read32(en_addr + local_irq_to_reg_offset(num)) &
			    BIT(num & PLIC_REG_MASK)) {
				shell_fprintf(sh, SHELL_NORMAL, " %u(%u)", num,
					      sys_read32(config->prio + (num * sizeof(uint32_t))));
			}
		}
		shell_fprintf(sh, SHELL_NORMAL, "\n");
	}

	return 0;
}

SHELL_DYNAMIC_CMD_CREATE(dsub_device_name, device_name_get);

#ifdef CONFIG_PLIC_TELINK_SHELL_IRQ_COUNT
SHELL_STATIC_SUBCMD_SET_CREATE(plic_stats_cmds,
	SHELL_CMD_ARG(get, &dsub_device_name,
		"Read PLIC's stats.\n"
		"Usage: plic stats get <device> [minimum hits]",
		cmd_stats_get, 2, 1),
	SHELL_CMD_ARG(clear, &dsub_device_name,
		"Reset PLIC's stats.\n"
		"Usage: plic stats clear <device>",
		cmd_stats_clear, 2, 0),
	SHELL_SUBCMD_SET_END
);
#endif /* CONFIG_PLIC_TELINK_SHELL_IRQ_COUNT */

SHELL_STATIC_SUBCMD_SET_CREATE(plic_cmds,
			       SHELL_CMD(info, NULL,
					 "Show PLIC information.\n"
					 "Usage: plic info <device>",
					 cmd_info),
#ifdef CONFIG_PLIC_TELINK_SHELL_IRQ_COUNT
			       SHELL_CMD(stats, &plic_stats_cmds, "IRQ stats", NULL),
#endif /* CONFIG_PLIC_TELINK_SHELL_IRQ_COUNT */
			       SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(plic, &plic_cmds, "PLIC shell commands", NULL);
#endif /* CONFIG_PLIC_TELINK_SHELL */

#define PLIC_MIN_IRQ_NUM(n) MIN(DT_INST_PROP(n, riscv_ndev), CONFIG_MAX_IRQ_PER_AGGREGATOR)

#ifdef CONFIG_PLIC_TELINK_SHELL_IRQ_COUNT
#define PLIC_INTC_IRQ_COUNT_BUF_DEFINE(n)                                                          \
	static uint16_t local_irq_count_##n[COND_CODE_1(CONFIG_MP_MAX_NUM_CPUS, (1),               \
							(UTIL_INC(CONFIG_MP_MAX_NUM_CPUS)))]       \
					   [PLIC_MIN_IRQ_NUM(n)];
#define PLIC_INTC_IRQ_COUNT_INIT(n)                                                                \
	.stats = {                                                                                 \
		.irq_count = &local_irq_count_##n[0][0],                                           \
	},

#else
#define PLIC_INTC_IRQ_COUNT_BUF_DEFINE(n)
#define PLIC_INTC_IRQ_COUNT_INIT(n)
#endif /* CONFIG_PLIC_TELINK_SHELL_IRQ_COUNT */

#define PLIC_IRQ_CPUMASK_BUF_DECLARE(n)
#define PLIC_IRQ_CPUMASK_BUF_INIT(n)

#define PLIC_INTC_DATA_INIT(n)                                                                     \
	PLIC_INTC_IRQ_COUNT_BUF_DEFINE(n);                                                         \
	PLIC_IRQ_CPUMASK_BUF_DECLARE(n);                                                           \
	static struct plic_data plic_data_##n = {                                                  \
		PLIC_INTC_IRQ_COUNT_INIT(n)                                                        \
		PLIC_IRQ_CPUMASK_BUF_INIT(n)                                                       \
	};

#define PLIC_INTC_IRQ_FUNC_DECLARE(n) static void plic_irq_config_func_##n(void)

#define PLIC_INTC_IRQ_FUNC_DEFINE(n)                                                               \
	static void plic_irq_config_func_##n(void)                                                 \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(n), 0, plic_irq_handler, DEVICE_DT_INST_GET(n), 0);       \
		irq_enable(DT_INST_IRQN(n));                                                       \
	}

#define HART_CONTEXTS(i, n) IF_ENABLED(IS_EQ(DT_INST_IRQN_BY_IDX(n, i), DT_INST_IRQN(n)), (i,))
#define PLIC_HART_CONTEXT_DECLARE(n)                                                               \
	INTC_PLIC_STATIC const uint32_t plic_hart_contexts_##n[DT_CHILD_NUM(DT_PATH(cpus))] = {    \
		LISTIFY(DT_INST_NUM_IRQS(n), HART_CONTEXTS, (), n)}

#define PLIC_INTC_CONFIG_INIT(n)                                                                   \
	PLIC_INTC_IRQ_FUNC_DECLARE(n);                                                             \
	PLIC_HART_CONTEXT_DECLARE(n);                                                              \
	static const struct plic_config plic_config_##n = {                                        \
		.prio = PLIC_BASE_ADDR(n),                                                         \
		.irq_en = PLIC_BASE_ADDR(n) + CONTEXT_ENABLE_BASE,                                 \
		.reg = PLIC_BASE_ADDR(n) + CONTEXT_BASE,                                           \
		.max_prio = DT_INST_PROP(n, riscv_max_priority),                                   \
		.riscv_ndev = DT_INST_PROP(n, riscv_ndev),                                         \
		.nr_irqs = PLIC_MIN_IRQ_NUM(n),                                                    \
		.irq = DT_INST_IRQN(n),                                                            \
		.irq_config_func = plic_irq_config_func_##n,                                       \
		.isr_table = &_sw_isr_table[INTC_INST_ISR_TBL_OFFSET(n)],                          \
		.hart_context = plic_hart_contexts_##n,                                            \
	};                                                                                         \
	PLIC_INTC_IRQ_FUNC_DEFINE(n)

#define PLIC_INTC_DEVICE_INIT(n)                                                                   \
	IRQ_PARENT_ENTRY_DEFINE(                                                                   \
		plic##n, DEVICE_DT_INST_GET(n), DT_INST_IRQN(n),                                   \
		INTC_INST_ISR_TBL_OFFSET(n),                                                       \
		DT_INST_INTC_GET_AGGREGATOR_LEVEL(n));                                             \
	PLIC_INTC_CONFIG_INIT(n)                                                                   \
	PLIC_INTC_DATA_INIT(n)                                                                     \
	DEVICE_DT_INST_DEFINE(n, &plic_init, NULL,                                                 \
			      &plic_data_##n, &plic_config_##n,                                    \
			      PRE_KERNEL_1, CONFIG_INTC_INIT_PRIORITY,                             \
			      NULL);

DT_INST_FOREACH_STATUS_OKAY(PLIC_INTC_DEVICE_INIT)
