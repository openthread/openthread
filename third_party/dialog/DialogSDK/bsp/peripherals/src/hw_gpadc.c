/**
 * \addtogroup BSP
 * \{
 * \addtogroup DEVICES
 * \{
 * \addtogroup GPADC
 * \{
 */

/**
 ****************************************************************************************
 *
 * @file hw_gpadc.c
 *
 * @brief Implementation of the GPADC Low Level Driver.
 *
 * Copyright (C) 2015. Dialog Semiconductor, unpublished work. This computer
 * program includes Confidential, Proprietary Information and is a Trade Secret of
 * Dialog Semiconductor. All use, disclosure, and/or reproduction is prohibited
 * unless authorized in writing. All Rights Reserved.
 *
 * <black.orca.support@diasemi.com> and contributors.
 *
 ****************************************************************************************
 */

#if dg_configUSE_HW_GPADC


#include <stdio.h>
#include <string.h>
#include <hw_gpadc.h>

#if (dg_configSYSTEMVIEW)
#  include "SEGGER_SYSVIEW_FreeRTOS.h"
#else
#  define SEGGER_SYSTEMVIEW_ISR_ENTER()
#  define SEGGER_SYSTEMVIEW_ISR_EXIT()
#endif

static hw_gpadc_interrupt_cb intr_cb = NULL;

int16_t hw_gpadc_differential_gain_error __RETAINED;
int16_t hw_gpadc_single_ended_gain_error __RETAINED;

void hw_gpadc_init(const gpadc_config *cfg)
{
        GPADC->GP_ADC_CTRL_REG = 0;
        GPADC->GP_ADC_CTRL2_REG = 0;
        GPADC->GP_ADC_CTRL3_REG = 0x40;      // default value for GP_ADDC_EN_DEL

        NVIC_DisableIRQ(ADC_IRQn);

        hw_gpadc_configure(cfg);
}

void hw_gpadc_reset(void)
{
        GPADC->GP_ADC_CTRL_REG = REG_MSK(GPADC, GP_ADC_CTRL_REG, GP_ADC_EN);
        GPADC->GP_ADC_CTRL2_REG = 0;
        GPADC->GP_ADC_CTRL3_REG = 0x40;      // default value for GP_ADDC_EN_DEL

        NVIC_DisableIRQ(ADC_IRQn);
}

void hw_gpadc_configure(const gpadc_config *cfg)
{
        if (cfg) {
                hw_gpadc_set_clock(cfg->clock);
                hw_gpadc_set_input_mode(cfg->input_mode);
                hw_gpadc_set_input(cfg->input);
                hw_gpadc_set_sample_time(cfg->sample_time);
                hw_gpadc_set_continuous(cfg->continous);
                hw_gpadc_set_interval(cfg->interval);
                hw_gpadc_set_input_attenuator_state(cfg->input_attenuator);
                hw_gpadc_set_chopping(cfg->chopping);
                hw_gpadc_set_oversampling(cfg->oversampling);
        }
}

void hw_gpadc_register_interrupt(hw_gpadc_interrupt_cb cb)
{
        intr_cb = cb;

        REG_SETF(GPADC, GP_ADC_CTRL_REG, GP_ADC_MINT, 1);

        NVIC_EnableIRQ(ADC_IRQn);
}

void hw_gpadc_unregister_interrupt(void)
{
        NVIC_DisableIRQ(ADC_IRQn);

        REG_SETF(GPADC, GP_ADC_CTRL_REG, GP_ADC_MINT, 0);

        intr_cb = NULL;
}

void hw_gpadc_adc_measure(void)
{
        hw_gpadc_start();
        while (hw_gpadc_in_progress());
        hw_gpadc_clear_interrupt();
}

void hw_gpadc_calibrate(void)
{
        uint16_t adc_off_p, adc_off_n;
        uint32_t diff;
        int i;

        for (i = 0; i < 5; i++) {               // Try up to five times to calibrate correctly.
                hw_gpadc_set_offset_positive(0x200);
                hw_gpadc_set_offset_negative(0x200);
                hw_gpadc_set_mute(true);
                if (dg_configUSE_SOC == 1) {
                        hw_gpadc_set_oversampling(4);
                }

                hw_gpadc_set_sign_change(false);
                hw_gpadc_adc_measure();
                if (dg_configUSE_SOC == 1) {
                        adc_off_p = ((hw_gpadc_get_value_without_gain() >> 6) - 0x200);
                } else {
                        adc_off_p = hw_gpadc_get_value() - 0x200;
                }

                hw_gpadc_set_sign_change(true);
                hw_gpadc_adc_measure();
                if (dg_configUSE_SOC == 1) {
                        adc_off_n = ((hw_gpadc_get_value_without_gain() >> 6) - 0x200);
                } else {
                        adc_off_n = hw_gpadc_get_value() - 0x200;
                }

                if (hw_gpadc_get_input_mode() == HW_GPADC_INPUT_MODE_SINGLE_ENDED) {
                        hw_gpadc_set_offset_positive(0x200 - 2 * adc_off_p);
                        hw_gpadc_set_offset_negative(0x200 - 2 * adc_off_n);
                } else {
                        hw_gpadc_set_offset_positive(0x200 - adc_off_p);
                        hw_gpadc_set_offset_negative(0x200 - adc_off_n);
                }

                hw_gpadc_set_sign_change(false);

                // Verify calibration result
                hw_gpadc_adc_measure();
                if (dg_configUSE_SOC == 1) {
                        adc_off_n = (hw_gpadc_get_value_without_gain() >> 6);
                } else {
                        adc_off_n = hw_gpadc_get_value();
                }

                if (adc_off_n > 0x200) {
                        diff = adc_off_n - 0x200;
                }
                else {
                        diff = 0x200 - adc_off_n;
                }

                if (diff < 0x8) {
                        break;
                }
                else if (i == 4) {
                        ASSERT_WARNING(0);      // Calibration does not converge.
                }
        }

        hw_gpadc_set_mute(false);
}

void ADC_Handler(void)
{
        SEGGER_SYSTEMVIEW_ISR_ENTER();

        if (intr_cb) {
                intr_cb();
        } else {
                hw_gpadc_clear_interrupt();
        }

        SEGGER_SYSTEMVIEW_ISR_EXIT();
}

void hw_gpadc_test_measurements(void)
{
        uint32_t val[16];
        uint32_t mid = 0, diff_n = 0, diff_p = 0;
        volatile bool loop = true;

        if (hw_gpadc_get_input_mode() != HW_GPADC_INPUT_MODE_SINGLE_ENDED) {
                hw_gpadc_set_input_mode(HW_GPADC_INPUT_MODE_SINGLE_ENDED);
                hw_gpadc_set_ldo_constant_current(true);
                hw_gpadc_set_ldo_dynamic_current(true);
                hw_gpadc_calibrate();
        }
        hw_gpadc_reset();
        hw_gpadc_set_input_mode(HW_GPADC_INPUT_MODE_SINGLE_ENDED);
        hw_gpadc_set_ldo_constant_current(true);
        hw_gpadc_set_ldo_dynamic_current(true);
        hw_gpadc_set_sample_time(15);
        hw_gpadc_set_chopping(true);
        hw_gpadc_set_input(HW_GPADC_INPUT_SE_VBAT);
//        hw_gpadc_set_oversampling(1); // 2 samples
        hw_gpadc_set_oversampling(2); // 4 samples

        while (loop) {
                for (volatile int i = 0; i < 4; i++); // wait ~1.5usec
                for (int j = 0; j < 16; j++) {
                        hw_gpadc_adc_measure();

//                        val[j] = hw_gpadc_get_raw_value() >> 4; // 2 samples
                        val[j] = hw_gpadc_get_raw_value() >> 3; // 4 samples

                        if (j == 0) {
                                mid = 0;
                                diff_p = 0;
                                diff_n = 0;
                        }

                        mid += val[j];

                        if (j == 15) {
                                mid /= 16;
                                for (int k = 0; k < 16; k++) {
                                        if ((mid > val[k]) && (diff_p < mid - val[k])) {
                                                diff_p = mid - val[k];
                                        }

                                        if ((mid < val[k]) && (diff_n < val[k] - mid)) {
                                                diff_n = val[k] - mid;
                                        }
                                }
                        }
                }
        }
}

uint16_t hw_gpadc_get_raw_value(void)
{
        int32_t adc_raw_res = GPADC->GP_ADC_RESULT_REG;
        int16_t gain_error = 0;
        uint16_t offset_trim = 0;

        if (dg_configUSE_ADC_GAIN_ERROR_CORRECTION == 1) {
                if (hw_gpadc_pre_check_for_gain_error()) {
                        const HW_GPADC_INPUT_MODE input_mode = hw_gpadc_get_input_mode();
                        if (input_mode == HW_GPADC_INPUT_MODE_DIFFERENTIAL) {
                                gain_error =  hw_gpadc_differential_gain_error;
                                offset_trim = 0x8000;
                        } else if (input_mode == HW_GPADC_INPUT_MODE_SINGLE_ENDED) {
                                gain_error = hw_gpadc_single_ended_gain_error;
                                offset_trim = 0;
                        }
                        adc_raw_res = adc_raw_res -
                                ((adc_raw_res - offset_trim ) * gain_error ) / 0x10000;
                        /* Adjust value for edge cases */
                        if (adc_raw_res & 0x80000000) { /* negative overflow */
                                return 0;
                        }
                        if (adc_raw_res & 0x7FFF0000) { /* positive overflow */
                                return UINT16_MAX;
                        }
                }
        }

        return adc_raw_res;
}

#endif /* dg_configUSE_HW_GPADC */
/**
 * \}
 * \}
 * \}
 */
