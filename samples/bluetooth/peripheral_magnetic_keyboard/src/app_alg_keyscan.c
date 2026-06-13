/** @file app_alg_keyscan.c
 *  @brief
 */

/*
 * Copyright (c) 2025 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/kernel.h>

#include <zephyr/settings/settings.h>

#include "app_public.h"
#include "pwm.h"
#include "app_alg_keyscan.h"

#define LOG_LEVEL LOG_LEVEL_DBG
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(app_alg_keyscan);


#if ALG_KEYSCAN_APP_FUN_ENABLE

#define BUFFER_SIZE (256) //the minimum buffer size (unit:byte), adc data format 16bits

#define  KEYSCAN_CYCLE_US       44

#define KEYSCAN_ADC_BUFFER_SIZE (BUFFER_SIZE / 2)
short adc_buffer[KEYSCAN_ADC_BUFFER_SIZE] = {0};

/* update threshold according to the real hall value */
ks_ana_threshold_t ks_ana_threshold = {
    // .release_threshold = 1200,       //1F2
    // .press_threshold = 1000
    .release_threshold = 850,       //1F2
    .press_threshold = 780
};

/* update gpio pin according to the hardware design */
ks_ana_gpio_pin_t ks_ana_gpio_pin = {
    .pin_group_id = KS_PIN_GROUP_ID_0,
    .ana_switch_enable_pin = GPIO_PF3,
    // .ana_switch_channel_pin = {GPIO_PE3, GPIO_PE4, GPIO_PE5, GPIO_PE6}
    // .ana_switch_channel_pin = {GPIO_PC7, GPIO_PD0, GPIO_PD1, GPIO_PD2},
};

void alg_keyscan_init(ks_ana_clock_e ks_ana_clock)
{
    gpio_function_en(GPIO_PE3); //short press KEY3 to generate an edge signal.
    gpio_output_en(GPIO_PE3);
    gpio_input_dis(GPIO_PE3);
    gpio_function_en(GPIO_PE4); //short press KEY3 to generate an edge signal.
    gpio_output_en(GPIO_PE4);
    gpio_input_dis(GPIO_PE4);
    gpio_function_en(GPIO_PE5); //short press KEY3 to generate an edge signal.
    gpio_output_en(GPIO_PE5);
    gpio_input_dis(GPIO_PE5);
    gpio_function_en(GPIO_PE6); //short press KEY3 to generate an edge signal.
    gpio_output_en(GPIO_PE6);
    gpio_input_dis(GPIO_PE6);
    gpio_set_level((gpio_pin_e)GPIO_FC_PE3,1);
    gpio_set_level((gpio_pin_e)GPIO_FC_PE4,1);
    gpio_set_level((gpio_pin_e)GPIO_FC_PE5,1);
    gpio_set_level((gpio_pin_e)GPIO_FC_PE6,1);

    if (ks_ana_clock == KEYSCAN_PWM_CLOCK_96M)
    {
        pwm_set_clk((unsigned char)(sys_clk.pclk * 1000 * 1000 / PWM_PCLK_96M - 1));

        #if (KEYSCAN_CYCLE_US == 44)
        pwm_set_tcmp(PWM0_ID, 11*(CLOCK_PWM_96M_CLOCK_1US / 4));//2.75
        pwm_set_tmax(PWM0_ID, 11*(CLOCK_PWM_96M_CLOCK_1US / 4)*2 + 1);

        pwm_set_tcmp(PWM1_ID, 11*(CLOCK_PWM_96M_CLOCK_1US / 4)*2 + 1);
        pwm_set_tmax(PWM1_ID, 11*(CLOCK_PWM_96M_CLOCK_1US / 4)*4 + 2);

        pwm_set_tcmp(PWM2_ID, 11*(CLOCK_PWM_96M_CLOCK_1US / 4)*4 + 2);
        pwm_set_tmax(PWM2_ID, 11*(CLOCK_PWM_96M_CLOCK_1US / 4)*8 + 4);

        pwm_set_tcmp(PWM3_ID, 11*(CLOCK_PWM_96M_CLOCK_1US / 4)*8 + 4);
        pwm_set_tmax(PWM3_ID, 11*(CLOCK_PWM_96M_CLOCK_1US / 4)*16 + 8);
        #elif (KEYSCAN_CYCLE_US == 88)
        pwm_set_tcmp(PWM0_ID, 22*(CLOCK_PWM_96M_CLOCK_1US / 4));//5.5
        pwm_set_tmax(PWM0_ID, 22*(CLOCK_PWM_96M_CLOCK_1US / 4)*2 + 1);

        pwm_set_tcmp(PWM1_ID, 22*(CLOCK_PWM_96M_CLOCK_1US / 4)*2 + 1);
        pwm_set_tmax(PWM1_ID, 22*(CLOCK_PWM_96M_CLOCK_1US / 4)*4 + 2);

        pwm_set_tcmp(PWM2_ID, 22*(CLOCK_PWM_96M_CLOCK_1US / 4)*4 + 2);
        pwm_set_tmax(PWM2_ID, 22*(CLOCK_PWM_96M_CLOCK_1US / 4)*8 + 4);

        pwm_set_tcmp(PWM3_ID, 22*(CLOCK_PWM_96M_CLOCK_1US / 4)*8 + 4);
        pwm_set_tmax(PWM3_ID, 22*(CLOCK_PWM_96M_CLOCK_1US / 4)*16 + 8);
        #endif
    }
    else if (ks_ana_clock == KEYSCAN_PWM_CLOCK_32M)
    {
        pwm_set_clk((unsigned char)(sys_clk.pclk * 1000 * 1000 / PWM_PCLK_32M - 1));

        #if (KEYSCAN_CYCLE_US == 44)
        pwm_set_tcmp(PWM0_ID, 11*(CLOCK_PWM_32M_CLOCK_1US / 4));//2.75
        pwm_set_tmax(PWM0_ID, 11*(CLOCK_PWM_32M_CLOCK_1US / 4)*2 + 1);

        pwm_set_tcmp(PWM1_ID, 11*(CLOCK_PWM_32M_CLOCK_1US / 4)*2 + 1);
        pwm_set_tmax(PWM1_ID, 11*(CLOCK_PWM_32M_CLOCK_1US / 4)*4 + 2);

        pwm_set_tcmp(PWM2_ID, 11*(CLOCK_PWM_32M_CLOCK_1US / 4)*4 + 2);
        pwm_set_tmax(PWM2_ID, 11*(CLOCK_PWM_32M_CLOCK_1US / 4)*8 + 4);

        pwm_set_tcmp(PWM3_ID, 11*(CLOCK_PWM_32M_CLOCK_1US / 4)*8 + 4);
        pwm_set_tmax(PWM3_ID, 11*(CLOCK_PWM_32M_CLOCK_1US / 4)*16 + 8);
        #elif (KEYSCAN_CYCLE_US == 88)
        pwm_set_tcmp(PWM0_ID, 22*(CLOCK_PWM_32M_CLOCK_1US / 4));//5.5
        pwm_set_tmax(PWM0_ID, 22*(CLOCK_PWM_32M_CLOCK_1US / 4)*2 + 1);

        pwm_set_tcmp(PWM1_ID, 22*(CLOCK_PWM_32M_CLOCK_1US / 4)*2 + 1);
        pwm_set_tmax(PWM1_ID, 22*(CLOCK_PWM_32M_CLOCK_1US / 4)*4 + 2);

        pwm_set_tcmp(PWM2_ID, 22*(CLOCK_PWM_32M_CLOCK_1US / 4)*4 + 2);
        pwm_set_tmax(PWM2_ID, 22*(CLOCK_PWM_32M_CLOCK_1US / 4)*8 + 4);

        pwm_set_tcmp(PWM3_ID, 22*(CLOCK_PWM_32M_CLOCK_1US / 4)*8 + 4);
        pwm_set_tmax(PWM3_ID, 22*(CLOCK_PWM_32M_CLOCK_1US / 4)*16 + 8);
        #endif
    }

    pwm_invert_en(PWM0_ID);
    pwm_invert_en(PWM1_ID);
    pwm_invert_en(PWM2_ID);
    pwm_invert_en(PWM3_ID);

    pwm_set_polarity_en(PWM0_ID);
    pwm_set_polarity_en(PWM1_ID);
    pwm_set_polarity_en(PWM2_ID);
    pwm_set_polarity_en(PWM3_ID);

    pwm_set_pin(GPIO_FC_PE3, PWM0);
    pwm_set_pin(GPIO_FC_PE4, PWM1);
    pwm_set_pin(GPIO_FC_PE5, PWM2);
    pwm_set_pin(GPIO_FC_PE6, PWM3);

    ks_ana_rx_dma_chain_init(ADC0, KS_ANA_ADC0_DMA_CHN, (unsigned short*)adc_buffer, KEYSCAN_ADC_BUFFER_SIZE);
    ks_ana_rx_dma_chain_init(ADC1, KS_ANA_ADC1_DMA_CHN, (unsigned short*)(adc_buffer + KEYSCAN_ADC_BUFFER_SIZE / 2), KEYSCAN_ADC_BUFFER_SIZE);
    ks_ana_init(ks_ana_gpio_pin, KS_TEST_MODE, ks_ana_clock, ks_ana_threshold);
}

void ks_pwm_mode_disable(void)
{
    pwm_stop(FLD_PWM0_EN|FLD_PWM1_EN|FLD_PWM2_EN|FLD_PWM3_EN);
    dma_chn_dis(KS_ANA_ADC0_DMA_CHN);
    dma_chn_dis(KS_ANA_ADC1_DMA_CHN);
    ks_ana_disable();
}

void ks_pwm_mode_enable(void)
{
    ks_ana_enable();
    ks_ana_rx_dma_chain_init(ADC0, KS_ANA_ADC0_DMA_CHN, (unsigned short*)adc_buffer, KEYSCAN_ADC_BUFFER_SIZE);
    ks_ana_rx_dma_chain_init(ADC1, KS_ANA_ADC1_DMA_CHN, (unsigned short*)(adc_buffer + KEYSCAN_ADC_BUFFER_SIZE / 2), KEYSCAN_ADC_BUFFER_SIZE);
    reg_ks_a_en1 |= FLD_KS_A_EN;//keyscan enable
    reg_pwm_enable |= 0x100000e;//(FLD_PWM0_EN|FLD_PWM1_EN|FLD_PWM2_EN|FLD_PWM3_EN)
}

#endif







