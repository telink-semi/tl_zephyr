/*
 * Copyright (c) 2026 Telink Semiconductor
 * SPDX-License-Identifier: Apache-2.0
 *
 * tlkdbg UART demo: emits a 1 ms VCD pattern plus LOG records in the
 * usbtraceplus wire format. The PC must be configured with mhz = 24.
 */
#include <zephyr/kernel.h>

#include <tlkdbg.h>

/* Signal IDs. Mirror these in the PC-side .def file:
 *   SLEV_timestamp 0   (timestamp; SID 0 is reserved)
 *   SL01_task_test 1
 *   SL16_counter   1
 *   SL24_FCNT      1
 */
#define SLEV_timestamp 0
#define SL01_task_test 1
#define SL16_counter   1
#define SL24_FCNT      1

int main(void)
{
	if (tlkdbg_init() != 0) {
		return 0;
	}

	LOG_STR_DYNAMIC("boot", "tlkdbg uart demo", 16);

	unsigned int seq = 0;

	while (1) {
		vcd_tick(SLEV_timestamp);
		vcd_task(SL01_task_test, 1);
		vcd_b24(SL24_FCNT, seq & 0xffffffu);
		vcd_b16(SL16_counter, seq & 0xffffu);
		vcd_task(SL01_task_test, 0);

		if ((seq % 100u) == 0u) {
			unsigned int v = seq;

			/* Build-time hash label (type 4); the text is scanned by
			 * tlkdbg_label_gen.py and only the 2-byte hash is sent. */
			LOG_STR_DATA(fcnt, "FCNT", &v, sizeof(v));
		}

		seq++;
		k_sleep(K_MSEC(1));
	}

	return 0;
}
