/*
 * Copyright (c) 2026 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Core voltage boost required for TL322X RRAM writes.
 *
 * RRAM (MTP) cell writes require the digital core LDO to be at 1.1V.
 * System profiles running at 48/64/72MHz use 1.0V and must boost the
 * voltage around each write; profiles at 96/192MHz already run at 1.1V
 * and need no change.
 */

#ifndef TL322X_RRAM_VOLT_H_
#define TL322X_RRAM_VOLT_H_

#include <stdbool.h>

#include "clock.h"
#include "pm.h"

/* Minimum CCLK (MHz) for which the system runs at 1.1V */
#define TL322X_RRAM_VOLT_1V1_MIN_CCLK_MHZ 96

/**
 * @brief Boost the core voltage to 1.1V before an RRAM write.
 *
 * Call once before the write sequence and pass the return value to
 * tl322x_rram_volt_restore(). May briefly block while pending DMA
 * transfers complete.
 *
 * @retval true  Voltage was boosted and must be restored afterwards.
 * @retval false System already runs at 1.1V, no restore needed.
 */
static inline bool tl322x_rram_volt_boost(void)
{
	if (sys_clk.cclk >= TL322X_RRAM_VOLT_1V1_MIN_CCLK_MHZ) {
		return false;
	}

	pm_set_dig_ldo(DIG_VOL_1V1_MODE, 1000);
	return true;
}

/**
 * @brief Restore the core voltage to 1.0V after an RRAM write.
 *
 * @param boosted Return value of tl322x_rram_volt_boost().
 */
static inline void tl322x_rram_volt_restore(bool boosted)
{
	if (boosted) {
		pm_set_dig_ldo(DIG_VOL_1V_MODE, 1000);
	}
}

#endif /* TL322X_RRAM_VOLT_H_ */
