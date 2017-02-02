/**
 * \addtogroup BSP
 * \{
 * \addtogroup DEVICES
 * \{
 * \addtogroup GPADC
 * \{
 * \brief General Purpose ADC
 */

/**
 *****************************************************************************************
 *
 * @file hw_gpadc.h
 *
 * @brief Definition of API for the GPADC Low Level Driver.
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
 *****************************************************************************************
 */

#ifndef HW_GPADC_H
#define HW_GPADC_H

#if dg_configUSE_HW_GPADC

#include <stdbool.h>
#include <stdint.h>
#include <sdk_defs.h>

extern int16_t hw_gpadc_differential_gain_error;
extern int16_t hw_gpadc_single_ended_gain_error;

/**
 * \brief ADC input mode
 *
 */
typedef enum {
        HW_GPADC_INPUT_MODE_DIFFERENTIAL = 0,    /**< differential mode (default) */
        HW_GPADC_INPUT_MODE_SINGLE_ENDED = 1     /**< single ended mode */
} HW_GPADC_INPUT_MODE;

/**
 * \brief ADC clock source
 *
 */
typedef enum {
        HW_GPADC_CLOCK_INTERNAL = 0,       /**< internal high-speed clock (default) */
        HW_GPADC_CLOCK_DIGITAL = 1         /**< digital clock (16/96MHz) */
} HW_GPADC_CLOCK;

/**
 * \brief ADC input
 *
 * \p GPADC_INPUT_SE_* values should be used only in single ended mode
 * \p GPADC_INPUT_DIFF_* values should be used only in differential mode
 *
 */
typedef enum {
        HW_GPADC_INPUT_SE_P12 = 0,         /**< GPIO 1.2 */
        HW_GPADC_INPUT_SE_P14 = 1,         /**< GPIO 1.4 */
        HW_GPADC_INPUT_SE_P13 = 2,         /**< GPIO 1.3 */
        HW_GPADC_INPUT_SE_P07 = 3,         /**< GPIO 0.7 */
        HW_GPADC_INPUT_SE_AVS = 4,         /**< analog ground level */
        HW_GPADC_INPUT_SE_VDD = 5,
        HW_GPADC_INPUT_SE_VDCDC = 6,
        HW_GPADC_INPUT_SE_V33 = 7,
        HW_GPADC_INPUT_SE_VBAT = 9,        /**< battery */
        HW_GPADC_INPUT_SE_TEMPSENS = 14,   /**< temperature sensor */
        HW_GPADC_INPUT_SE_P06 = 16,        /**< GPIO 0.6 */
        HW_GPADC_INPUT_SE_P10 = 17,        /**< GPIO 1.0 */
        HW_GPADC_INPUT_SE_P15 = 18,        /**< GPIO 1.5 */
        HW_GPADC_INPUT_SE_P24 = 19,        /**< GPIO 2.4 */
        HW_GPADC_INPUT_DIFF_P12_P14 = 0,   /**< GPIO 1.2 vs 1.4 */
        HW_GPADC_INPUT_DIFF_P13_P07 = 1,   /**< GPIO 1.3 vs 0.7 */
} HW_GPADC_INPUT;

/**
 * \brief ADC interrupt handler
 *
 */
typedef void (*hw_gpadc_interrupt_cb)(void);

/**
 * \brief ADC configuration
 *
 */
typedef struct {
        HW_GPADC_CLOCK        clock;              /**< clock source */
        HW_GPADC_INPUT_MODE   input_mode;         /**< input mode */
        HW_GPADC_INPUT        input;              /**< ADC input */

        uint8_t               sample_time;        /**< sample time */
        bool                  continous;          /**< continous mode state */
        uint8_t               interval;           /**< interval between conversions in continous mode */

        bool                  input_attenuator;   /**< input attenuator state */
        bool                  chopping;           /**< chopping state */
        uint8_t               oversampling;       /**< oversampling value */
} gpadc_config;

/**
 * \brief Initialize ADC
 *
 * \p cfg can be NULL - no configuration is performed in such case.
 *
 * \param [in] cfg configuration
 *
 */
void hw_gpadc_init(const gpadc_config *cfg);

/**
 * \brief Configure ADC
 *
 * Shortcut to call appropriate configuration function. If \p cfg is NULL, this function does
 * nothing.
 *
 * \param [in] cfg configuration
 *
 */
void hw_gpadc_configure(const gpadc_config *cfg);

/**
 * \brief Reset ADC to its default values without disabling the LDO.
 *
 */
void hw_gpadc_reset(void);

/**
 * \brief Register interrupt handler
 *
 * Interrupt is enabled after calling this function. Application is responsible for clearing
 * interrupt using hw_gpadc_clear_interrupt(). If no callback is specified interrupt is cleared by
 * driver.
 *
 * \param [in] cb callback fired on interrupt
 *
 * \sa hw_gpadc_clear_interrupt
 *
 */
void hw_gpadc_register_interrupt(hw_gpadc_interrupt_cb cb);

/**
 * \brief Unregister interrupt handler
 *
 * Interrupt is disabled after calling this function.
 *
 */
void hw_gpadc_unregister_interrupt(void);

/**
 * \brief Clear interrupt
 *
 * Application should call this in interrupt handler to clear interrupt.
 *
 * \sa hw_gpadc_register_interrupt
 *
 */
static inline void hw_gpadc_clear_interrupt(void)
{
        GPADC->GP_ADC_CLEAR_INT_REG = 1;
}

/**
 * \brief Enable ADC
 *
 * Sampling is started after calling this function, to start conversion application should call
 * hw_gpadc_start().
 *
 * \sa hw_gpadc_start
 *
 */
static inline void hw_gpadc_enable(void)
{
        REG_SETF(GPADC, GP_ADC_CTRL_REG, GP_ADC_EN, 1);
}

/**
 * \brief Disable ADC
 *
 * Application should wait for conversion to be completed before disabling ADC. In case of
 * continuous mode, application should disable continuous mode and then wait for conversion to be
 * completed in order to have ADC in defined state.
 *
 * \sa hw_gpadc_in_progress
 * \sa hw_gpadc_set_continuous
 *
 */
static inline void hw_gpadc_disable(void)
{
        REG_SETF(GPADC, GP_ADC_CTRL_REG, GP_ADC_EN, 0);
}

/**
 * \brief Set the delay required to enable the ADC_LDO.
 *
 * param [in] LDO enable delay
 *
 */
static inline void hw_gpadc_set_ldo_delay(uint32_t delay)
{
        REG_SETF(GPADC, GP_ADC_CTRL3_REG, GP_ADC_EN_DEL, delay);
}

/** \brief Perform ADC calibration
 *
 * Calibration is performed. The input mode must be HW_GPADC_INPUT_SE_VDD (1.2V).
 * LDO constant and dynamic currents may be on. The input mode must be Single Ended. The calibration
 * is performed only once.
 *
 * \sa hw_gpadc_set_input_mode
 *
 */
void hw_gpadc_calibrate(void);

/**
 * \brief Start conversion
 *
 * Application should not call this function while conversion is still in progress.
 *
 * \sa hw_gpadc_in_progress
 *
 */
static inline void hw_gpadc_start(void)
{
        REG_SETF(GPADC, GP_ADC_CTRL_REG, GP_ADC_START, 1);
}

/**
 * \brief Check if conversion is in progress
 *
 * \return conversion state
 *
 */
static inline bool hw_gpadc_in_progress(void)
{
        return REG_GETF(GPADC, GP_ADC_CTRL_REG, GP_ADC_START);
}

/**
 * \brief Set continuous mode
 *
 * With continuous mode enabled ADC will automatically restart conversion once completed. It's still
 * required to start 1st conversion using hw_gpadc_start(). Interval between subsequent conversions
 * can be adjusted using hw_gpadc_set_interval().
 *
 * \param [in] enabled continuous mode state
 *
 * \sa hw_gpadc_start
 * \sa hw_gpadc_set_interval
 *
 */
static inline void hw_gpadc_set_continuous(bool enabled)
{
        REG_SETF(GPADC, GP_ADC_CTRL_REG, GP_ADC_CONT, !!enabled);
}

/**
 * \brief Get continuous mode state
 *
 * \return continuous mode state
 *
 */
static inline bool hw_gpadc_get_continuous(void)
{
        return REG_GETF(GPADC, GP_ADC_CTRL_REG, GP_ADC_CONT);
}

/**
 * \brief Set input channel
 *
 * Application is responsible for using proper input symbols depending on whether single ended or
 * differential mode is used.
 *
 * \param [in] input input channel
 *
 * \sa hw_gpadc_set_input_mode
 * \sa GPADC_INPUT_MODE
 *
 */
static inline void hw_gpadc_set_input(HW_GPADC_INPUT input)
{
        REG_SETF(GPADC, GP_ADC_CTRL_REG, GP_ADC_SEL, input);
}

/**
 * \brief Get current input channel
 *
 * \return input channel
 *
 */
static inline HW_GPADC_INPUT hw_gpadc_get_input(void)
{
        return (HW_GPADC_INPUT) REG_GETF(GPADC, GP_ADC_CTRL_REG, GP_ADC_SEL);
}

/**
 * \brief Set input mode
 *
 * \param [in] mode input mode
 *
 */
static inline void hw_gpadc_set_input_mode(HW_GPADC_INPUT_MODE mode)
{
        REG_SETF(GPADC, GP_ADC_CTRL_REG, GP_ADC_SE, mode);
}

/**
 * \brief Get current input mode
 *
 * return input mode
 *
 */
static inline HW_GPADC_INPUT_MODE hw_gpadc_get_input_mode(void)
{
        return (HW_GPADC_INPUT_MODE) REG_GETF(GPADC, GP_ADC_CTRL_REG, GP_ADC_SE);
}

/**
 * \brief Set clock source
 *
 * \param [in] clock clock source
 *
 */
static inline void hw_gpadc_set_clock(HW_GPADC_CLOCK clock)
{
        REG_SETF(GPADC, GP_ADC_CTRL_REG, GP_ADC_CLK_SEL, clock);
}

/**
 * \brief Get current clock source
 *
 * \return clock source
 *
 */
static inline HW_GPADC_CLOCK hw_gpadc_get_clock(void)
{
        return (HW_GPADC_CLOCK) REG_GETF(GPADC, GP_ADC_CTRL_REG, GP_ADC_CLK_SEL);
}

/**
 * \brief Set oversampling
 *
 * With oversampling enabled multiple successive conversions will be executed and results are added
 * together to increase effective number of bits in result.
 *
 * Number of samples taken is 2<sup>\p n_samples</sup>. Valid values for \p n_samples are 0-7 thus
 * at most 128 samples can be taken. In this case 17bits of result are generated with the least
 * significant bit being discarded.
 *
 * \param [in] n_samples number of samples to be taken
 *
 * \sa hw_gpadc_get_raw_value
 *
 */
static inline void hw_gpadc_set_oversampling(uint8_t n_samples)
{
        REG_SETF(GPADC, GP_ADC_CTRL2_REG, GP_ADC_CONV_NRS, n_samples);
}

/**
 * \brief Get current oversampling
 *
 * \return number of samples to be taken
 *
 * \sa hw_gpadc_set_oversampling
 *
 */
static inline uint8_t hw_gpadc_get_oversampling(void)
{
        return REG_GETF(GPADC, GP_ADC_CTRL2_REG, GP_ADC_CONV_NRS);
}

/**
 * \brief Set sample time
 *
 * Sample time is \p mult x 32 clock cycles or 1 clock cycle when \p mult is 0. Valid values are
 * 0-15.
 *
 * \param [in] mult multiplier
 *
 */
static inline void hw_gpadc_set_sample_time(uint8_t mult)
{
        REG_SETF(GPADC, GP_ADC_CTRL2_REG, GP_ADC_SMPL_TIME, mult);
}

/**
 * \brief Get current sample time
 *
 * \return multiplier
 *
 * \sa hw_gpadc_set_sample_time
 *
 */
static inline uint8_t hw_gpadc_get_sample_time(void)
{
        return REG_GETF(GPADC, GP_ADC_CTRL2_REG, GP_ADC_SMPL_TIME);
}

/**
 * \brief Set state of input attenuator
 *
 * Enabling internal attenuator scales input voltage by factor of 3 thus increasing effective input
 * scale from 0-1.2V to 0-3.6V in single ended mode or from -1.2-1.2V to -3.6-3.6V in differential
 * mode.
 *
 * \param [in] enabled attenuator state
 *
 */
static inline void hw_gpadc_set_input_attenuator_state(bool enabled)
{
        REG_SETF(GPADC, GP_ADC_CTRL2_REG, GP_ADC_ATTN3X, !!enabled);
}

/**
 * \brief Get current state of input attenuator
 *
 * \return attenuator state
 *
 */
static inline bool hw_gpadc_get_input_attenuator_state(void)
{
        return REG_GETF(GPADC, GP_ADC_CTRL2_REG, GP_ADC_ATTN3X);
}

/**
 * \brief Set input mute state
 *
 * Once enabled, samples are taken at mid-scale to determine internal offset and/or notice of the
 * ADC with regards to VDD_REF.
 *
 * \param [in] enabled mute state
 *
 * \sa hw_gpadc_calibrate
 *
 */
static inline void hw_gpadc_set_mute(bool enabled)
{
        REG_SETF(GPADC, GP_ADC_CTRL_REG, GP_ADC_MUTE, !!enabled);
}

/**
 * \brief Get current input mute state
 *
 * \return mute state
 *
 * \sa hw_gpadc_set_mute
 *
 */
static inline bool hw_gpadc_get_mute(void)
{
        return REG_GETF(GPADC, GP_ADC_CTRL_REG, GP_ADC_MUTE);
}

/**
 * \brief Set input and output sign change
 *
 * Once enabled, sign of ADC input and output is changed.
 *
 * \param [in] enabled sign change state
 *
 */
static inline void hw_gpadc_set_sign_change(bool enabled)
{
        REG_SETF(GPADC, GP_ADC_CTRL_REG, GP_ADC_SIGN, !!enabled);
}

/**
 * \brief Set input and output sign change
 *
 * \return sign change state
 *
 */
static inline bool hw_gpadc_get_sign_change(void)
{
        return REG_GETF(GPADC, GP_ADC_CTRL_REG, GP_ADC_SIGN);
}

/**
 * \brief Set chopping state
 *
 * Once enabled, two samples with opposite polarity are taken to cancel offset.
 *
 * \param [in] enabled chopping state
 *
 */
static inline void hw_gpadc_set_chopping(bool enabled)
{
        REG_SETF(GPADC, GP_ADC_CTRL_REG, GP_ADC_CHOP, !!enabled);
}

/**
 * \brief Get current chopping state
 *
 * \return chopping state
 *
 */
static inline bool hw_gpadc_get_chopping(void)
{
        return REG_GETF(GPADC, GP_ADC_CTRL_REG, GP_ADC_CHOP);
}

/**
 * \brief Set state of constant 20uA load current on ADC LDO output
 *
 * Constant 20uA load current on LDO output can be enabled so that the current will not drop to 0.
 *
 * \param [in] enabled load current state
 *
 */
static inline void hw_gpadc_set_ldo_constant_current(bool enabled)
{
        REG_SETF(GPADC, GP_ADC_CTRL2_REG, GP_ADC_I20U, !!enabled);
}

/**
 * \brief Get current state of constant 20uA load current on ADC LDO output
 *
 * \return load current current state
 *
 * \sa hw_gpadc_set_ldo_constant_current
 *
 */
static inline bool hw_gpadc_get_ldo_constant_current(void)
{
        return REG_GETF(GPADC, GP_ADC_CTRL2_REG, GP_ADC_I20U);
}

/**
 * \brief Set state of dynamic 10uA load current on ADC LDO output
 *
 * 10uA load current on LDO output can be enabled during sample phase so that the load current
 * during sampling and conversion phase becomes approximately the same.
 *
 * \param [in] enabled load current state
 *
 */
static inline void hw_gpadc_set_ldo_dynamic_current(bool enabled)
{
        REG_SETF(GPADC, GP_ADC_CTRL2_REG, GP_ADC_IDYN, !!enabled);
}

/**
 * \brief Get current state of dynamic 10uA load current on ADC LDO output
 *
 * \return load current current state
 *
 * \sa hw_gpadc_set_ldo_dynamic_current
 *
 */
static inline bool hw_gpadc_get_ldo_dynamic_current(void)
{
        return REG_GETF(GPADC, GP_ADC_CTRL2_REG, GP_ADC_IDYN);
}

/**
 * \brief Set interval between conversions in continuous mode
 *
 * Interval time is \p mult x 1.024ms. Valid values are 0-255.
 *
 * \param [in] mult multiplier
 *
 * \sa hw_gpadc_set_continuous
 *
 */
static inline void hw_gpadc_set_interval(uint8_t mult)
{
        REG_SETF(GPADC, GP_ADC_CTRL3_REG, GP_ADC_INTERVAL, mult);
}

/**
 * \brief Get current interval between conversions in continuous mode
 *
 * \return multiplier
 *
 * \sa hw_gpadc_set_interval
 *
 */
static inline uint8_t hw_gpadc_get_interval(void)
{
        return REG_GETF(GPADC, GP_ADC_CTRL3_REG, GP_ADC_INTERVAL);
}

/**
 * \brief Set offset adjustment for positive ADC array
 *
 * \param [in] offset offset value
 *
 * \sa hw_gpadc_calibrate
 * \sa hw_gpadc_set_negative_offset
 *
 */
static inline void hw_gpadc_set_offset_positive(uint16_t offset)
{
        GPADC->GP_ADC_OFFP_REG = offset & REG_MSK(GPADC, GP_ADC_OFFP_REG, GP_ADC_OFFP);
}

/**
 * \brief Get current offset adjustment for positive ADC array
 *
 * \return offset value
 *
 */
static inline uint16_t hw_gpadc_get_offset_positive(void)
{
        return GPADC->GP_ADC_OFFP_REG & REG_MSK(GPADC, GP_ADC_OFFP_REG, GP_ADC_OFFP);
}

/**
 * \brief Set offset adjustment for negative ADC array
 *
 * \param [in] offset offset value
 *
 * \sa hw_gpadc_calibrate
 * \sa hw_gpadc_set_positive_offset
 *
 */
static inline void hw_gpadc_set_offset_negative(uint16_t offset)
{
        GPADC->GP_ADC_OFFN_REG = offset & REG_MSK(GPADC, GP_ADC_OFFN_REG, GP_ADC_OFFN);
}


/**
 * \brief Get current offset adjustment for negative ADC array
 *
 * \return offset value
 *
 */
static inline uint16_t hw_gpadc_get_offset_negative(void)
{
        return GPADC->GP_ADC_OFFN_REG & REG_MSK(GPADC, GP_ADC_OFFN_REG, GP_ADC_OFFN);
}

/**
 * \brief Store Single Ended ADC Gain Error
 *
 * \param [in] single ADC Single Ended Gain Error
 *
 */
static inline void hw_gpadc_store_se_gain_error(int16_t single)
{
        hw_gpadc_single_ended_gain_error = single;
}

/**
 * \brief Store Differential ADC Gain Error
 *
 * \param [in] diff ADC Differential Gain Error
 *
 */
static inline void hw_gpadc_store_diff_gain_error(int16_t diff)
{
        hw_gpadc_differential_gain_error = diff;
}

/**
 * \brief Check the availability of ADC Gain Error
 *
 * \return ADC Gain Error availability
 *
 */
static inline bool hw_gpadc_pre_check_for_gain_error(void)
{
        if (dg_configUSE_ADC_GAIN_ERROR_CORRECTION == 1) {
                return (hw_gpadc_single_ended_gain_error && hw_gpadc_differential_gain_error);
        }

        return false;
}

/**
 * \brief Get raw conversion result value
 *
 * Upper 10 bits are always valid, lower 6 bits are only valid in case oversampling is enabled.
 *
 * \return conversion result value
 *
 * \sa hw_gpadc_start
 * \sa hw_gpadc_set_oversampling
 * \sa hw_gpadc_get_value
 *
 */
uint16_t hw_gpadc_get_raw_value(void);

/**
 * \brief Get conversion result value
 *
 * Invalid bits are discarded from result, i.e. oversampling is taken into account when calculating
 * value.
 *
 * \return conversion result value
 *
 * \sa hw_gpadc_set_oversampling
 * \sa hw_gpadc_get_raw_value
 *
 */
static inline uint16_t hw_gpadc_get_value(void)
{
        return hw_gpadc_get_raw_value() >> (6 - MIN(6, hw_gpadc_get_oversampling()));
}

/**
 * \brief Get conversion result value without gain compensation and over sampling.
 *
 * \return conversion result value
 *
 */
static inline uint16_t hw_gpadc_get_value_without_gain(void)
{
        uint16_t adc_raw_res = GPADC->GP_ADC_RESULT_REG;
        return adc_raw_res;
}

/**
 * \brief Start a measurement and wait for the result.
 *
 * \sa hw_gpadc_start
 * \sa hw_gpadc_in_progress
 * \sa hw_gpadc_clear_interrupt
 *
 */
void hw_gpadc_adc_measure(void);

/**
 * \brief Take measurements using the ADC and evaluate the results.
 *
 * \sa hw_gpadc_adc_measure
 * \sa hw_gpadc_get_raw_value
 *
 */
void hw_gpadc_test_measurements(void);

#endif /* dg_configUSE_HW_GPADC */

#endif /* HW_GPADC_H_ */

/**
 * \}
 * \}
 * \}
 */
