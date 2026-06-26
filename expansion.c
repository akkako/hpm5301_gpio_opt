/*
 * Copyright (c) 2024 HalfSweet
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include "hpm_common.h"
#include <hpm_gpiom_drv.h>
#include <hpm_l1c_drv.h>
#include <hpm_romapi.h>
#include <hpm_ewdg_drv.h>
#include <hpm_spi.h>
#include "expansion.h"

#include "board.h"
#include "hpm_gptmr_drv.h"
#include "hpm_gpio_drv.h"
#include "hpm_adc16_drv.h"
#include "setting.h"
#include "reset_way.h"

volatile bool VREF_ENABLE = false;

GPTMR_Type *const USER_PWM = HPM_GPTMR0;
const clock_name_t USER_PWM_CLK = clock_gptmr0;
const uint8_t PWM_CHANNEL = 2;

ADC16_Type *const USER_ADC = HPM_ADC0;

const uint32_t DEFAULT_PWM_FREQ = 100000;
const uint8_t DEFAULT_PWM_DUTY = 85;

const uint8_t DEFAULT_ADC_RUN_MODE = adc16_conv_mode_oneshot;
const uint8_t DEFAULT_ADC_CYCLE = 20;

static uint32_t pwm_current_reload;

static uint32_t GPIO_Power_EN = IOC_PAD_PA31;
static uint32_t GPIO_Port_EN = IOC_PAD_PA04;
static uint32_t GPIO_BTN = IOC_PAD_PA03;

static void IONum_Init()
{
    GPIO_Power_EN = IOC_PAD_PA31;
    GPIO_Port_EN = IOC_PAD_PA04;
    GPIO_BTN = IOC_PAD_PA03;
}

static void set_pwm_waveform_edge_aligned_frequency(uint32_t freq)
{
    gptmr_channel_config_t config;
    uint32_t gptmr_freq;

    gptmr_channel_get_default_config(USER_PWM, &config);
    gptmr_freq = clock_get_frequency(USER_PWM_CLK);
    pwm_current_reload = gptmr_freq / freq;
    config.reload = pwm_current_reload;
    config.cmp_initial_polarity_high = true;
    gptmr_stop_counter(USER_PWM, PWM_CHANNEL);
    gptmr_channel_config(USER_PWM, PWM_CHANNEL, &config, false);
    gptmr_channel_reset_count(USER_PWM, PWM_CHANNEL);
    gptmr_start_counter(USER_PWM, PWM_CHANNEL);
}

static void set_pwm_waveform_edge_aligned_duty(uint8_t duty)
{
    uint32_t cmp;
    if (duty > 100)
    {
        duty = 100;
    }
    cmp = ((pwm_current_reload * duty) / 100) + 1;
    gptmr_update_cmp(USER_PWM, PWM_CHANNEL, 0, cmp);
}

static void Power_PWM_Init(void)
{
    // PA10 100k PWM，占空比50%
    HPM_IOC->PAD[IOC_PAD_PA10].FUNC_CTL = IOC_PA10_FUNC_CTL_GPTMR0_COMP_2;
    set_pwm_waveform_edge_aligned_frequency(DEFAULT_PWM_FREQ);
    set_pwm_waveform_edge_aligned_duty(DEFAULT_PWM_DUTY); // 以目前的控制形式来看，电压越高，输出电压就越小，先给个较高的占空比
}

static void Power_Set_TVCC_Voltage(void)
{
    // 3.3V
    set_pwm_waveform_edge_aligned_duty(51);
}

static void ADC_Init()
{
    /* Configure the ADC clock from AHB (@200MHz by default)*/
    clock_set_adc_source(clock_adc0, clk_adc_src_ahb0);

    adc16_config_t cfg;

    /* initialize an ADC instance */
    adc16_get_default_config(&cfg);

    cfg.res = adc16_res_16_bits;
    cfg.conv_mode = DEFAULT_ADC_RUN_MODE;
    cfg.adc_clk_div = adc16_clock_divider_4;
    cfg.sel_sync_ahb = (clk_adc_src_ahb0 == clock_get_source(BOARD_APP_ADC16_CLK_NAME)) ? true : false;

    if (cfg.conv_mode == adc16_conv_mode_sequence ||
        cfg.conv_mode == adc16_conv_mode_preemption)
    {
        cfg.adc_ahb_en = true;
    }

    /* adc16 initialization */
    if (adc16_init(USER_ADC, &cfg) == status_success)
    {
        /* enable irq */
        //        intc_m_enable_irq_with_priority(BOARD_APP_ADC16_IRQn, 1);
    }
}

ATTR_ALWAYS_INLINE
static inline void VREF_Init(void)
{
    // VREF 的输入是 PB08
    HPM_IOC->PAD[IOC_PAD_PB08].FUNC_CTL = IOC_PAD_FUNC_CTL_ANALOG_MASK;
}

ATTR_ALWAYS_INLINE
static inline void TVCC_Init(void)
{
    // TVCC 的输入是 PB09
    HPM_IOC->PAD[IOC_PAD_PB09].FUNC_CTL = IOC_PAD_FUNC_CTL_ANALOG_MASK;
}

static void Power_Enable_Init(void)
{
    HPM_IOC->PAD[GPIO_Power_EN].FUNC_CTL = IOC_PAD_FUNC_CTL_ALT_SELECT_SET(0);

    const uint8_t port_index = GPIO_GET_PORT_INDEX(GPIO_Power_EN);
    const uint8_t pin_index = GPIO_GET_PIN_INDEX(GPIO_Power_EN);

    gpiom_set_pin_controller(HPM_GPIOM, port_index, pin_index, gpiom_soc_gpio0);
    gpio_set_pin_output(HPM_GPIO0, port_index, pin_index);
    gpio_write_pin(HPM_GPIO0, port_index, pin_index, 0);
}

ATTR_ALWAYS_INLINE
static inline void Port_Enable_Init(void)
{
    // PA04
    HPM_IOC->PAD[GPIO_Port_EN].FUNC_CTL = IOC_PAD_FUNC_CTL_ALT_SELECT_SET(0);

    const uint32_t port_index = GPIO_GET_PORT_INDEX(GPIO_Port_EN);
    const uint8_t pin_index = GPIO_GET_PIN_INDEX(GPIO_Port_EN);

    gpiom_set_pin_controller(HPM_GPIOM, port_index, pin_index, gpiom_soc_gpio0);
    gpio_set_pin_output(HPM_GPIO0, port_index, pin_index);
    gpio_write_pin(HPM_GPIO0, port_index, pin_index, 0);
}

void Power_Turn(bool on)
{
    const uint32_t port_index = GPIO_GET_PORT_INDEX(GPIO_Power_EN);
    const uint8_t pin_index = GPIO_GET_PIN_INDEX(GPIO_Power_EN);
    gpio_write_pin(HPM_GPIO0, port_index, pin_index, on);
}

void Port_Turn(bool on)
{
    const uint32_t port_index = GPIO_GET_PORT_INDEX(GPIO_Port_EN);
    const uint8_t pin_index = GPIO_GET_PIN_INDEX(GPIO_Port_EN);
    gpio_write_pin(HPM_GPIO0, port_index, pin_index, on);
}

void HSP_Init(void)
{
    IONum_Init();
    // 初始化电源部分
    Power_Enable_Init();
    Port_Enable_Init();
    Power_PWM_Init();
    // 初始化ADC部分
    ADC_Init();
    VREF_Init();
    TVCC_Init();

    Power_Turn(HSLink_Setting.power.power_on);
    Port_Turn(HSLink_Setting.power.port_on);
}

void HSP_Loop(void)
{
    static uint32_t last_pwr_chk_time = 0;
    if (millis() - last_pwr_chk_time > 5)
    {
        last_pwr_chk_time = millis();

        {
            Power_Turn(HSLink_Setting.power.power_on);
            Power_Set_TVCC_Voltage(); // TVCC恢复默认设置
            Port_Turn(HSLink_Setting.power.port_on);
            VREF_ENABLE = false;
        }
    }
}
