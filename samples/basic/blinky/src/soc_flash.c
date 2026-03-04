
#include <flash.h>
#include <lib/include/flash_base.h>
#include <lib/include/mspi.h>

struct storage_device {

};

#define _tlk_always_inline                      inline __attribute__((always_inline))
#define _tlk_attribute_ram_code_sec_            __attribute__((section(".ram_code")))
#define _tlk_attribute_ram_code_sec_noinline_   __attribute__((section(".ram_code"))) __attribute__((noinline))
#define TLK_BIT(n)                              (1 << (n))
#define mmisc_ctl                               0x7d0

#define tlk_mspi_stop_xip                       mspi_stop_xip
#define tlk_mspi_set_xip_en                     mspi_set_xip_en
#define tlk_mspi_func_e                         mspi_func_e
#define TLK_MSPI_READ                           MSPI_READ

#define tlk_core_interrupt_disable              core_interrupt_disable
#define tlk_core_restore_interrupt              core_restore_interrupt

#define TLK_MSPI_WRITE                          MSPI_WRITE
#define TLK_MSPI_READ                           MSPI_READ
#define TLK_MSPI_ERASE                          MSPI_ERASE

#define reg_mspi_cipher_ctrl            REG_ADDR8(MSPI_BASE_ADDR + 0x85)
enum{
    FLD_MSPI_CIPHER_RD_EN               = BIT(0),
    FLD_MSPI_CIPHER_WR_EN               = BIT(1),
};

#define tlk_mspi_tx_cnt                         mspi_tx_cnt
#define tlk_mspi_set_address                    mspi_set_address
#define tlk_mspi_set_ctrl                       mspi_set_ctrl
#define tlk_mspi_set_reg_ctrl0                  mspi_set_reg_ctrl0
#define tlk_mspi_set_cmd                        mspi_set_cmd
#define tlk_mspi_write                          mspi_write
#define tlk_mspi_rx_cnt                         mspi_rx_cnt
#define tlk_mspi_read                           mspi_read

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define MMISC_CTL                   0x7d0
#define MMISC_CTL_BRPE_MASK         TLK_BIT(3)

enum {
    SOC_FLASH_DREAD_CMD                    = 0x3b4097a9,
    SOC_FLASH_X4READ_CMD                   = 0xeb4493ba,
    SOC_FLASH_READ_SECURITY_REGISTERS_CMD  = 0x480097a8,
    SOC_FLASH_READ_UID_CMD_GD_PUYA_ZB_TH   = 0x4b0097a8,
    SOC_FLASH_GET_JEDEC_ID                 = 0x9f002080,
    SOC_FLASH_READ_STATUS_CMD_LOWBYTE      = 0x05002080,
    SOC_FLASH_READ_STATUS_CMD_HIGHBYTE     = 0x35002080,
    SOC_FLASH_READ_CONFIGURE_CMD           = 0x15002080,
    SOC_FLASH_WRITE_CMD                    = 0x020010a8,
    SOC_FLASH_QUAD_PAGE_PROGRAM_CMD        = 0x320010aa,
    SOC_FLASH_SECT_ERASE_CMD               = 0x200070a8,
    SOC_FLASH_WRITE_SECURITY_REGISTERS_CMD = 0x420010a8,
    SOC_FLASH_ERASE_SECURITY_REGISTERS_CMD = 0x440070a8,
    SOC_FLASH_WRITE_STATUS_CMD_LOWBYTE     = 0x01001080,
    SOC_FLASH_WRITE_STATUS_CMD_HIGHBYTE    = 0x31001080,
    SOC_FLASH_WRITE_CONFIGURE_CMD_1        = 0x31001080,
    SOC_FLASH_WRITE_CONFIGURE_CMD_2        = 0x11001080,
    SOC_FLASH_WRITE_DISABLE_CMD            = 0x04007080,
    SOC_FLASH_WRITE_ENABLE_CMD             = 0x06007080,
    SOC_FLASH_WRITE_DEEP_CMD               = 0xb9007080,
    SOC_FLASH_WRITE_RELEASE_CMD            = 0xab007080
};

_tlk_always_inline
static inline bool mmisc_ctl_brpe_clear_and_get(void)
{
	uint32_t old;

	__asm__ volatile ("csrrc %0, %1, %2"
					   : "=r"(old)
					   : "i"(MMISC_CTL), "r"(MMISC_CTL_BRPE_MASK)
					   : "memory");
    return (old & MMISC_CTL_BRPE_MASK);
}

_tlk_always_inline
static void mmisc_ctl_brpe_restore(bool saved)
{
    if (saved) {
        __asm__ volatile ("csrrs x0, %0, %1"
                          :
                          : "i"(MMISC_CTL), "r"(MMISC_CTL_BRPE_MASK)
                          : "memory");
    } else {
        __asm__ volatile ("csrrc x0, %0, %1"
                          :
                          : "i"(MMISC_CTL), "r"(MMISC_CTL_BRPE_MASK)
                          : "memory");
    }
}

_tlk_always_inline
static void soc_flash_mspi_cipher_read_en(void)
{
    reg_mspi_cipher_ctrl |= FLD_MSPI_CIPHER_RD_EN;
}

_tlk_always_inline
static void soc_flash_mspi_cipher_read_dis(void)
{
    reg_mspi_cipher_ctrl &= ~FLD_MSPI_CIPHER_RD_EN;
}

_tlk_always_inline
static void soc_flash_mspi_cipher_write_en(void)
{
    reg_mspi_cipher_ctrl |= FLD_MSPI_CIPHER_WR_EN;
}

_tlk_always_inline
static void soc_flash_mspi_cipher_write_dis(void)
{
    reg_mspi_cipher_ctrl &= ~FLD_MSPI_CIPHER_WR_EN;
}

_tlk_always_inline
static void soc_flash_mspi_read(uint32_t cmd, uintptr_t addr, void *data, size_t data_len)
{
    tlk_mspi_rx_cnt(data_len);
    tlk_mspi_set_address(addr);
    tlk_mspi_set_ctrl(cmd);
    tlk_mspi_set_reg_ctrl0(cmd >> 16);
    tlk_mspi_set_cmd(cmd >> 24);
    tlk_mspi_read(data, data_len);
}

_tlk_always_inline
static void soc_flash_mspi_write(uint32_t cmd, uintptr_t addr, const void *data, size_t data_len)
{
    tlk_mspi_tx_cnt(data_len);
    tlk_mspi_set_address(addr);
    tlk_mspi_set_ctrl(cmd);
    tlk_mspi_set_reg_ctrl0(cmd >> 16);
    tlk_mspi_set_cmd(cmd >> 24);
    tlk_mspi_write(data, data_len);
}

_tlk_attribute_ram_code_sec_noinline_
static void soc_flash_mspi_wr_ram(uint32_t cmd, uintptr_t addr, void *data, size_t data_len,
                                  bool is_encrypt, tlk_mspi_func_e mspi_wr)
{
    uint32_t r = tlk_core_interrupt_disable();

    tlk_mspi_stop_xip();

    uint8_t cipher_sta = reg_mspi_cipher_ctrl;

    if (is_encrypt) {
        if (mspi_wr == TLK_MSPI_READ) {
            soc_flash_mspi_cipher_read_en();
            soc_flash_mspi_read(cmd, addr, data, data_len);
        } else if (mspi_wr == TLK_MSPI_WRITE) {
            soc_flash_mspi_cipher_write_en();
            soc_flash_mspi_write(cmd, addr, data, data_len);
        }
    } else {
        if (mspi_wr == TLK_MSPI_READ) {
            soc_flash_mspi_cipher_read_dis();
            soc_flash_mspi_read(cmd, addr, data, data_len);
        } else if (mspi_wr == TLK_MSPI_WRITE) {
            soc_flash_mspi_cipher_write_dis();
            soc_flash_mspi_write(cmd, addr, data, data_len);
        }
    }
    reg_mspi_cipher_ctrl = cipher_sta;
    __asm__ __volatile__("nop");
    __asm__ __volatile__("nop");
    __asm__ __volatile__("nop");
    __asm__ __volatile__("nop");
    __asm__ __volatile__("nop");
    tlk_mspi_set_xip_en();
    tlk_core_restore_interrupt(r);
}

static int soc_flash_read(struct storage_device *dev, uintptr_t addr, void *buf, size_t len)
{
    (void)dev;
    bool brpe = mmisc_ctl_brpe_clear_and_get();

    soc_flash_mspi_wr_ram(SOC_FLASH_X4READ_CMD, addr, buf, len, false, TLK_MSPI_READ);
    mmisc_ctl_brpe_restore(brpe);
    return 0;
}
