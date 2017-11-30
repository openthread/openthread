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
 * Copyright (c) 2016, Dialog Semiconductor
 * All rights reserved.
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its contributors
 *    may be used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL,  SPECIAL,  EXEMPLARY,  OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 *
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
        uint16_t adc_raw_res = GPADC->GP_ADC_RESULT_REG;
        int32_t res = adc_raw_res;

        if (dg_configUSE_ADC_GAIN_ERROR_CORRECTION == 1) {
                if (hw_gpadc_pre_check_for_gain_error()) {
                        int16_t gain_trim;
                        const HW_GPADC_INPUT_MODE input_mode = hw_gpadc_get_input_mode();

                        if (input_mode == HW_GPADC_INPUT_MODE_SINGLE_ENDED) {
                                gain_trim = hw_gpadc_single_ended_gain_error;
                                /* Gain correction */
                                res = 0xFFFFU * adc_raw_res / (0xFFFFU + gain_trim);
                                /* Boundary check */
                                if ((uint32_t)res > UINT16_MAX) {
                                        return UINT16_MAX;
                                }
                        } else if (input_mode == HW_GPADC_INPUT_MODE_DIFFERENTIAL) {
                                gain_trim =  hw_gpadc_differential_gain_error;
                                res = (int16_t)(adc_raw_res ^ 0x8000);
                                /* Gain correction */
                                res = 0xFFFF * res / (0xFFFF + gain_trim);
                                /* Boundary check for lower limit */
                                if (res < INT16_MIN) {
                                        return 0;               /* INT16_MIN ^ 0x8000 */
                                }
                                /* Boundary check for upper limit */
                                if (res > INT16_MAX) {
                                        return UINT16_MAX;      /* INT16_MAX ^ 0x8000 */
                                }
                                res ^= 0x8000;
                        }
                }
        }

        return res;
}

#endif /* dg_configUSE_HW_GPADC */
/**
 * \}
 * \}
 * \}
 */
