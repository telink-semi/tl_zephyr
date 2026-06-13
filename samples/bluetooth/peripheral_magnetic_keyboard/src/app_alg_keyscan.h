/********************************************************************************************************
 * @file    app_alg_keyscan.h
 *
 * @brief   This is the header file for Telink RISC-V MCU
 *
 * @author  2.4G Group
 * @date    2025
 *
 * @par     Copyright (c) 2025, Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
 *
 *          Licensed under the Apache License, Version 2.0 (the "License");
 *          you may not use this file except in compliance with the License.
 *          You may obtain a copy of the License at
 *
 *              http://www.apache.org/licenses/LICENSE-2.0
 *
 *          Unless required by applicable law or agreed to in writing, software
 *          distributed under the License is distributed on an "AS IS" BASIS,
 *          WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *          See the License for the specific language governing permissions and
 *          limitations under the License.
 *
 *******************************************************************************************************/
#ifndef __APP_ALG_KEYSCAN_H__
#define __APP_ALG_KEYSCAN_H__

/* Enable C linkage for C++ Compilers: */
#if defined(__cplusplus)
extern "C" {
#endif

#include "keyscan_ana.h"

#define KEYSCAN_TEST_1XADC_8K_ONCE          0//For internal testing only. Not available.
#define KEYSCAN_TEST_2XADC_8K_ONCE          1
#define KEYSCAN_TEST_2XADC_8K_TWICE_M1      2//For internal testing only. Not available.
#define KEYSCAN_TEST_2XADC_8K_TWICE_M2      3//For internal testing only. Not available.


#define KS_ANA_ADC0_DMA_CHN   DMA0
#define KS_ANA_ADC1_DMA_CHN   DMA1


#define KS_TEST_MODE                        KEYSCAN_TEST_2XADC_8K_ONCE

#define KS_DMA_LLP_ENABLE                   1

#define KS_TEST_THRESHOLD                   0

/**
 * This configure is for PWM
 */
#define PWM_PCLK_96M 96000000
#define PWM_PCLK_32M 32000000

enum
{
    CLOCK_PWM_96M_CLOCK_1S  = PWM_PCLK_96M,
    CLOCK_PWM_96M_CLOCK_1MS = (CLOCK_PWM_96M_CLOCK_1S / 1000),
    CLOCK_PWM_96M_CLOCK_1US = (CLOCK_PWM_96M_CLOCK_1S / 1000000),
};

enum
{
    CLOCK_PWM_32M_CLOCK_1S  = PWM_PCLK_32M,
    CLOCK_PWM_32M_CLOCK_1MS = (CLOCK_PWM_32M_CLOCK_1S / 1000),
    CLOCK_PWM_32M_CLOCK_1US = (CLOCK_PWM_32M_CLOCK_1S / 1000000),
};

extern short adc_buffer[];
extern ks_ana_threshold_t ks_ana_threshold;

void alg_keyscan_init(ks_ana_clock_e ks_ana_clock);

void ks_pwm_mode_disable(void);

void ks_pwm_mode_enable(void);


/* Disable C linkage for C++ Compilers: */
#if defined(__cplusplus)
}
#endif

#endif // __APP_ALG_KEYSCAN_H__


