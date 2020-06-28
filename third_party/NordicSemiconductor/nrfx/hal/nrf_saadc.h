/*
 * Copyright (c) 2015 - 2019, Nordic Semiconductor ASA
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef NRF_SAADC_H_
#define NRF_SAADC_H_

#include <nrfx.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup nrf_saadc_hal SAADC HAL
 * @{
 * @ingroup nrf_saadc
 * @brief   Hardware access layer for managing the SAADC peripheral.
 */

/** @brief Number of available SAADC channels. */
#define NRF_SAADC_CHANNEL_COUNT 8


/** @brief Resolution of the analog-to-digital converter. */
typedef enum
{
    NRF_SAADC_RESOLUTION_8BIT  = SAADC_RESOLUTION_VAL_8bit,  ///< 8 bit resolution.
    NRF_SAADC_RESOLUTION_10BIT = SAADC_RESOLUTION_VAL_10bit, ///< 10 bit resolution.
    NRF_SAADC_RESOLUTION_12BIT = SAADC_RESOLUTION_VAL_12bit, ///< 12 bit resolution.
    NRF_SAADC_RESOLUTION_14BIT = SAADC_RESOLUTION_VAL_14bit  ///< 14 bit resolution.
} nrf_saadc_resolution_t;

/** @brief Input selection for the analog-to-digital converter. */
typedef enum
{
    NRF_SAADC_INPUT_DISABLED = SAADC_CH_PSELP_PSELP_NC,           ///< Not connected.
    NRF_SAADC_INPUT_AIN0     = SAADC_CH_PSELP_PSELP_AnalogInput0, ///< Analog input 0 (AIN0).
    NRF_SAADC_INPUT_AIN1     = SAADC_CH_PSELP_PSELP_AnalogInput1, ///< Analog input 1 (AIN1).
    NRF_SAADC_INPUT_AIN2     = SAADC_CH_PSELP_PSELP_AnalogInput2, ///< Analog input 2 (AIN2).
    NRF_SAADC_INPUT_AIN3     = SAADC_CH_PSELP_PSELP_AnalogInput3, ///< Analog input 3 (AIN3).
    NRF_SAADC_INPUT_AIN4     = SAADC_CH_PSELP_PSELP_AnalogInput4, ///< Analog input 4 (AIN4).
    NRF_SAADC_INPUT_AIN5     = SAADC_CH_PSELP_PSELP_AnalogInput5, ///< Analog input 5 (AIN5).
    NRF_SAADC_INPUT_AIN6     = SAADC_CH_PSELP_PSELP_AnalogInput6, ///< Analog input 6 (AIN6).
    NRF_SAADC_INPUT_AIN7     = SAADC_CH_PSELP_PSELP_AnalogInput7, ///< Analog input 7 (AIN7).
    NRF_SAADC_INPUT_VDD      = SAADC_CH_PSELP_PSELP_VDD           ///< VDD as input.
} nrf_saadc_input_t;

/** @brief Analog-to-digital converter oversampling mode. */
typedef enum
{
    NRF_SAADC_OVERSAMPLE_DISABLED = SAADC_OVERSAMPLE_OVERSAMPLE_Bypass,   ///< No oversampling.
    NRF_SAADC_OVERSAMPLE_2X       = SAADC_OVERSAMPLE_OVERSAMPLE_Over2x,   ///< Oversample 2x.
    NRF_SAADC_OVERSAMPLE_4X       = SAADC_OVERSAMPLE_OVERSAMPLE_Over4x,   ///< Oversample 4x.
    NRF_SAADC_OVERSAMPLE_8X       = SAADC_OVERSAMPLE_OVERSAMPLE_Over8x,   ///< Oversample 8x.
    NRF_SAADC_OVERSAMPLE_16X      = SAADC_OVERSAMPLE_OVERSAMPLE_Over16x,  ///< Oversample 16x.
    NRF_SAADC_OVERSAMPLE_32X      = SAADC_OVERSAMPLE_OVERSAMPLE_Over32x,  ///< Oversample 32x.
    NRF_SAADC_OVERSAMPLE_64X      = SAADC_OVERSAMPLE_OVERSAMPLE_Over64x,  ///< Oversample 64x.
    NRF_SAADC_OVERSAMPLE_128X     = SAADC_OVERSAMPLE_OVERSAMPLE_Over128x, ///< Oversample 128x.
    NRF_SAADC_OVERSAMPLE_256X     = SAADC_OVERSAMPLE_OVERSAMPLE_Over256x  ///< Oversample 256x.
} nrf_saadc_oversample_t;

/** @brief Analog-to-digital converter channel resistor control. */
typedef enum
{
    NRF_SAADC_RESISTOR_DISABLED = SAADC_CH_CONFIG_RESP_Bypass,   ///< Bypass resistor ladder.
    NRF_SAADC_RESISTOR_PULLDOWN = SAADC_CH_CONFIG_RESP_Pulldown, ///< Pull-down to GND.
    NRF_SAADC_RESISTOR_PULLUP   = SAADC_CH_CONFIG_RESP_Pullup,   ///< Pull-up to VDD.
    NRF_SAADC_RESISTOR_VDD1_2   = SAADC_CH_CONFIG_RESP_VDD1_2    ///< Set input at VDD/2.
} nrf_saadc_resistor_t;

/** @brief Gain factor of the analog-to-digital converter input. */
typedef enum
{
    NRF_SAADC_GAIN1_6 = SAADC_CH_CONFIG_GAIN_Gain1_6, ///< Gain factor 1/6.
    NRF_SAADC_GAIN1_5 = SAADC_CH_CONFIG_GAIN_Gain1_5, ///< Gain factor 1/5.
    NRF_SAADC_GAIN1_4 = SAADC_CH_CONFIG_GAIN_Gain1_4, ///< Gain factor 1/4.
    NRF_SAADC_GAIN1_3 = SAADC_CH_CONFIG_GAIN_Gain1_3, ///< Gain factor 1/3.
    NRF_SAADC_GAIN1_2 = SAADC_CH_CONFIG_GAIN_Gain1_2, ///< Gain factor 1/2.
    NRF_SAADC_GAIN1   = SAADC_CH_CONFIG_GAIN_Gain1,   ///< Gain factor 1.
    NRF_SAADC_GAIN2   = SAADC_CH_CONFIG_GAIN_Gain2,   ///< Gain factor 2.
    NRF_SAADC_GAIN4   = SAADC_CH_CONFIG_GAIN_Gain4,   ///< Gain factor 4.
} nrf_saadc_gain_t;

/** @brief Reference selection for the analog-to-digital converter. */
typedef enum
{
    NRF_SAADC_REFERENCE_INTERNAL = SAADC_CH_CONFIG_REFSEL_Internal, ///< Internal reference (0.6 V).
    NRF_SAADC_REFERENCE_VDD4     = SAADC_CH_CONFIG_REFSEL_VDD1_4    ///< VDD/4 as reference.
} nrf_saadc_reference_t;

/** @brief Analog-to-digital converter acquisition time. */
typedef enum
{
    NRF_SAADC_ACQTIME_3US  = SAADC_CH_CONFIG_TACQ_3us,  ///< 3 us.
    NRF_SAADC_ACQTIME_5US  = SAADC_CH_CONFIG_TACQ_5us,  ///< 5 us.
    NRF_SAADC_ACQTIME_10US = SAADC_CH_CONFIG_TACQ_10us, ///< 10 us.
    NRF_SAADC_ACQTIME_15US = SAADC_CH_CONFIG_TACQ_15us, ///< 15 us.
    NRF_SAADC_ACQTIME_20US = SAADC_CH_CONFIG_TACQ_20us, ///< 20 us.
    NRF_SAADC_ACQTIME_40US = SAADC_CH_CONFIG_TACQ_40us  ///< 40 us.
} nrf_saadc_acqtime_t;

/** @brief Analog-to-digital converter channel mode. */
typedef enum
{
    NRF_SAADC_MODE_SINGLE_ENDED = SAADC_CH_CONFIG_MODE_SE,  ///< Single-ended mode. PSELN will be ignored, negative input to ADC shorted to GND.
    NRF_SAADC_MODE_DIFFERENTIAL = SAADC_CH_CONFIG_MODE_Diff ///< Differential mode.
} nrf_saadc_mode_t;

/** @brief Analog-to-digital converter channel burst mode. */
typedef enum
{
    NRF_SAADC_BURST_DISABLED = SAADC_CH_CONFIG_BURST_Disabled, ///< Burst mode is disabled (normal operation).
    NRF_SAADC_BURST_ENABLED  = SAADC_CH_CONFIG_BURST_Enabled   ///< Burst mode is enabled. SAADC takes 2^OVERSAMPLE number of samples as fast as it can, and sends the average to Data RAM.
} nrf_saadc_burst_t;

/** @brief Analog-to-digital converter tasks. */
typedef enum
{
    NRF_SAADC_TASK_START           = offsetof(NRF_SAADC_Type, TASKS_START),           ///< Start the ADC and prepare the result buffer in RAM.
    NRF_SAADC_TASK_SAMPLE          = offsetof(NRF_SAADC_Type, TASKS_SAMPLE),          ///< Take one ADC sample. If scan is enabled, all channels are sampled.
    NRF_SAADC_TASK_STOP            = offsetof(NRF_SAADC_Type, TASKS_STOP),            ///< Stop the ADC and terminate any ongoing conversion.
    NRF_SAADC_TASK_CALIBRATEOFFSET = offsetof(NRF_SAADC_Type, TASKS_CALIBRATEOFFSET), ///< Starts offset auto-calibration.
} nrf_saadc_task_t;

/** @brief Analog-to-digital converter events. */
typedef enum
{
    NRF_SAADC_EVENT_STARTED       = offsetof(NRF_SAADC_Type, EVENTS_STARTED),       ///< The ADC has started.
    NRF_SAADC_EVENT_END           = offsetof(NRF_SAADC_Type, EVENTS_END),           ///< The ADC has filled up the result buffer.
    NRF_SAADC_EVENT_DONE          = offsetof(NRF_SAADC_Type, EVENTS_DONE),          ///< A conversion task has been completed.
    NRF_SAADC_EVENT_RESULTDONE    = offsetof(NRF_SAADC_Type, EVENTS_RESULTDONE),    ///< A result is ready to get transferred to RAM.
    NRF_SAADC_EVENT_CALIBRATEDONE = offsetof(NRF_SAADC_Type, EVENTS_CALIBRATEDONE), ///< Calibration is complete.
    NRF_SAADC_EVENT_STOPPED       = offsetof(NRF_SAADC_Type, EVENTS_STOPPED),       ///< The ADC has stopped.
    NRF_SAADC_EVENT_CH0_LIMITH    = offsetof(NRF_SAADC_Type, EVENTS_CH[0].LIMITH),  ///< Last result is equal or above CH[0].LIMIT.HIGH.
    NRF_SAADC_EVENT_CH0_LIMITL    = offsetof(NRF_SAADC_Type, EVENTS_CH[0].LIMITL),  ///< Last result is equal or below CH[0].LIMIT.LOW.
    NRF_SAADC_EVENT_CH1_LIMITH    = offsetof(NRF_SAADC_Type, EVENTS_CH[1].LIMITH),  ///< Last result is equal or above CH[1].LIMIT.HIGH.
    NRF_SAADC_EVENT_CH1_LIMITL    = offsetof(NRF_SAADC_Type, EVENTS_CH[1].LIMITL),  ///< Last result is equal or below CH[1].LIMIT.LOW.
    NRF_SAADC_EVENT_CH2_LIMITH    = offsetof(NRF_SAADC_Type, EVENTS_CH[2].LIMITH),  ///< Last result is equal or above CH[2].LIMIT.HIGH.
    NRF_SAADC_EVENT_CH2_LIMITL    = offsetof(NRF_SAADC_Type, EVENTS_CH[2].LIMITL),  ///< Last result is equal or below CH[2].LIMIT.LOW.
    NRF_SAADC_EVENT_CH3_LIMITH    = offsetof(NRF_SAADC_Type, EVENTS_CH[3].LIMITH),  ///< Last result is equal or above CH[3].LIMIT.HIGH.
    NRF_SAADC_EVENT_CH3_LIMITL    = offsetof(NRF_SAADC_Type, EVENTS_CH[3].LIMITL),  ///< Last result is equal or below CH[3].LIMIT.LOW.
    NRF_SAADC_EVENT_CH4_LIMITH    = offsetof(NRF_SAADC_Type, EVENTS_CH[4].LIMITH),  ///< Last result is equal or above CH[4].LIMIT.HIGH.
    NRF_SAADC_EVENT_CH4_LIMITL    = offsetof(NRF_SAADC_Type, EVENTS_CH[4].LIMITL),  ///< Last result is equal or below CH[4].LIMIT.LOW.
    NRF_SAADC_EVENT_CH5_LIMITH    = offsetof(NRF_SAADC_Type, EVENTS_CH[5].LIMITH),  ///< Last result is equal or above CH[5].LIMIT.HIGH.
    NRF_SAADC_EVENT_CH5_LIMITL    = offsetof(NRF_SAADC_Type, EVENTS_CH[5].LIMITL),  ///< Last result is equal or below CH[5].LIMIT.LOW.
    NRF_SAADC_EVENT_CH6_LIMITH    = offsetof(NRF_SAADC_Type, EVENTS_CH[6].LIMITH),  ///< Last result is equal or above CH[6].LIMIT.HIGH.
    NRF_SAADC_EVENT_CH6_LIMITL    = offsetof(NRF_SAADC_Type, EVENTS_CH[6].LIMITL),  ///< Last result is equal or below CH[6].LIMIT.LOW.
    NRF_SAADC_EVENT_CH7_LIMITH    = offsetof(NRF_SAADC_Type, EVENTS_CH[7].LIMITH),  ///< Last result is equal or above CH[7].LIMIT.HIGH.
    NRF_SAADC_EVENT_CH7_LIMITL    = offsetof(NRF_SAADC_Type, EVENTS_CH[7].LIMITL)   ///< Last result is equal or below CH[7].LIMIT.LOW.
} nrf_saadc_event_t;

/** @brief Analog-to-digital converter interrupt masks. */
typedef enum
{
    NRF_SAADC_INT_STARTED       = SAADC_INTENSET_STARTED_Msk,       ///< Interrupt on EVENTS_STARTED event.
    NRF_SAADC_INT_END           = SAADC_INTENSET_END_Msk,           ///< Interrupt on EVENTS_END event.
    NRF_SAADC_INT_DONE          = SAADC_INTENSET_DONE_Msk,          ///< Interrupt on EVENTS_DONE event.
    NRF_SAADC_INT_RESULTDONE    = SAADC_INTENSET_RESULTDONE_Msk,    ///< Interrupt on EVENTS_RESULTDONE event.
    NRF_SAADC_INT_CALIBRATEDONE = SAADC_INTENSET_CALIBRATEDONE_Msk, ///< Interrupt on EVENTS_CALIBRATEDONE event.
    NRF_SAADC_INT_STOPPED       = SAADC_INTENSET_STOPPED_Msk,       ///< Interrupt on EVENTS_STOPPED event.
    NRF_SAADC_INT_CH0LIMITH     = SAADC_INTENSET_CH0LIMITH_Msk,     ///< Interrupt on EVENTS_CH[0].LIMITH event.
    NRF_SAADC_INT_CH0LIMITL     = SAADC_INTENSET_CH0LIMITL_Msk,     ///< Interrupt on EVENTS_CH[0].LIMITL event.
    NRF_SAADC_INT_CH1LIMITH     = SAADC_INTENSET_CH1LIMITH_Msk,     ///< Interrupt on EVENTS_CH[1].LIMITH event.
    NRF_SAADC_INT_CH1LIMITL     = SAADC_INTENSET_CH1LIMITL_Msk,     ///< Interrupt on EVENTS_CH[1].LIMITL event.
    NRF_SAADC_INT_CH2LIMITH     = SAADC_INTENSET_CH2LIMITH_Msk,     ///< Interrupt on EVENTS_CH[2].LIMITH event.
    NRF_SAADC_INT_CH2LIMITL     = SAADC_INTENSET_CH2LIMITL_Msk,     ///< Interrupt on EVENTS_CH[2].LIMITL event.
    NRF_SAADC_INT_CH3LIMITH     = SAADC_INTENSET_CH3LIMITH_Msk,     ///< Interrupt on EVENTS_CH[3].LIMITH event.
    NRF_SAADC_INT_CH3LIMITL     = SAADC_INTENSET_CH3LIMITL_Msk,     ///< Interrupt on EVENTS_CH[3].LIMITL event.
    NRF_SAADC_INT_CH4LIMITH     = SAADC_INTENSET_CH4LIMITH_Msk,     ///< Interrupt on EVENTS_CH[4].LIMITH event.
    NRF_SAADC_INT_CH4LIMITL     = SAADC_INTENSET_CH4LIMITL_Msk,     ///< Interrupt on EVENTS_CH[4].LIMITL event.
    NRF_SAADC_INT_CH5LIMITH     = SAADC_INTENSET_CH5LIMITH_Msk,     ///< Interrupt on EVENTS_CH[5].LIMITH event.
    NRF_SAADC_INT_CH5LIMITL     = SAADC_INTENSET_CH5LIMITL_Msk,     ///< Interrupt on EVENTS_CH[5].LIMITL event.
    NRF_SAADC_INT_CH6LIMITH     = SAADC_INTENSET_CH6LIMITH_Msk,     ///< Interrupt on EVENTS_CH[6].LIMITH event.
    NRF_SAADC_INT_CH6LIMITL     = SAADC_INTENSET_CH6LIMITL_Msk,     ///< Interrupt on EVENTS_CH[6].LIMITL event.
    NRF_SAADC_INT_CH7LIMITH     = SAADC_INTENSET_CH7LIMITH_Msk,     ///< Interrupt on EVENTS_CH[7].LIMITH event.
    NRF_SAADC_INT_CH7LIMITL     = SAADC_INTENSET_CH7LIMITL_Msk,     ///< Interrupt on EVENTS_CH[7].LIMITL event.
    NRF_SAADC_INT_ALL           = 0x7FFFFFFFUL                      ///< Mask of all interrupts.
} nrf_saadc_int_mask_t;

/** @brief Analog-to-digital converter value limit type. */
typedef enum
{
    NRF_SAADC_LIMIT_LOW  = 0, ///< Low limit type.
    NRF_SAADC_LIMIT_HIGH = 1  ///< High limit type.
} nrf_saadc_limit_t;

/** @brief Type of a single ADC conversion result. */
typedef int16_t nrf_saadc_value_t;

/** @brief Analog-to-digital converter configuration structure. */
typedef struct
{
    nrf_saadc_resolution_t resolution;  ///< Resolution of samples.
    nrf_saadc_oversample_t oversample;  ///< Oversampling configuration.
    nrf_saadc_value_t *    buffer;      ///< Pointer to sample buffer.
    uint32_t               buffer_size; ///< Size of the sample buffer.
} nrf_saadc_config_t;

/** @brief Analog-to-digital converter channel configuration structure. */
typedef struct
{
    nrf_saadc_resistor_t  resistor_p; ///< Resistor value on positive input.
    nrf_saadc_resistor_t  resistor_n; ///< Resistor value on negative input.
    nrf_saadc_gain_t      gain;       ///< Gain control value.
    nrf_saadc_reference_t reference;  ///< Reference control value.
    nrf_saadc_acqtime_t   acq_time;   ///< Acquisition time.
    nrf_saadc_mode_t      mode;       ///< SAADC mode. Single-ended or differential.
    nrf_saadc_burst_t     burst;      ///< Burst mode configuration.
    nrf_saadc_input_t     pin_p;      ///< Input positive pin selection.
    nrf_saadc_input_t     pin_n;      ///< Input negative pin selection.
} nrf_saadc_channel_config_t;


/**
 * @brief Function for triggering the specified SAADC task.
 *
 * @param[in] task SAADC task.
 */
__STATIC_INLINE void nrf_saadc_task_trigger(nrf_saadc_task_t task);

/**
 * @brief Function for getting the address of the specified SAADC task register.
 *
 * @param[in] task SAADC task.
 *
 * @return Address of the specified SAADC task.
 */
__STATIC_INLINE uint32_t nrf_saadc_task_address_get(nrf_saadc_task_t task);

/**
 * @brief Function for retrieving the state of the UARTE event.
 *
 * @param[in] event Event to be checked.
 *
 * @retval true  The event has been generated.
 * @retval false The event has not been generated.
 */
__STATIC_INLINE bool nrf_saadc_event_check(nrf_saadc_event_t event);

/**
 * @brief Function for clearing the specific SAADC event.
 *
 * @param[in] event SAADC event.
 */
__STATIC_INLINE void nrf_saadc_event_clear(nrf_saadc_event_t event);

/**
 * @brief Function for getting the address of the specified SAADC event register.
 *
 * @param[in] event SAADC event.
 *
 * @return Address of the specified SAADC event.
 */
__STATIC_INLINE uint32_t nrf_saadc_event_address_get(nrf_saadc_event_t event);

#if defined(DPPI_PRESENT) || defined(__NRFX_DOXYGEN__)
/**
 * @brief Function for setting the subscribe configuration for a given
 *        SAADC task.
 *
 * @param[in] task    Task for which to set the configuration.
 * @param[in] channel Channel through which to subscribe events.
 */
__STATIC_INLINE void nrf_saadc_subscribe_set(nrf_saadc_task_t task,
                                             uint8_t          channel);

/**
 * @brief Function for clearing the subscribe configuration for a given
 *        SAADC task.
 *
 * @param[in] task Task for which to clear the configuration.
 */
__STATIC_INLINE void nrf_saadc_subscribe_clear(nrf_saadc_task_t task);

/**
 * @brief Function for setting the publish configuration for a given
 *        SAADC event.
 *
 * @param[in] event   Event for which to set the configuration.
 * @param[in] channel Channel through which to publish the event.
 */
__STATIC_INLINE void nrf_saadc_publish_set(nrf_saadc_event_t event,
                                           uint8_t           channel);

/**
 * @brief Function for clearing the publish configuration for a given
 *        SAADC event.
 *
 * @param[in] event Event for which to clear the configuration.
 */
__STATIC_INLINE void nrf_saadc_publish_clear(nrf_saadc_event_t event);
#endif // defined(DPPI_PRESENT) || defined(__NRFX_DOXYGEN__)

/**
 * @brief Function for getting the address of the SAADC limit event register,
 *        as specified by the channel and the limit type.
 *
 * @param[in] channel    Channel number.
 * @param[in] limit_type Low limit or high limit.
 *
 * @return Address of the specified SAADC limit event.
 */
__STATIC_INLINE volatile uint32_t * nrf_saadc_event_limit_address_get(uint8_t           channel,
                                                                      nrf_saadc_limit_t limit_type);

/**
 * @brief Function for getting the SAADC channel monitoring limit events.
 *
 * @param[in] channel    Channel number.
 * @param[in] limit_type Low limit or high limit.
 *
 * @return The SAADC channel monitoring limit event.
 */
__STATIC_INLINE nrf_saadc_event_t nrf_saadc_event_limit_get(uint8_t           channel,
                                                            nrf_saadc_limit_t limit_type);

/**
 * @brief Function for configuring the input pins for the specified SAADC channel.
 *
 * @param[in] channel Channel number.
 * @param[in] pselp   Positive input.
 * @param[in] pseln   Negative input. Set to NRF_SAADC_INPUT_DISABLED in single ended mode.
 */
__STATIC_INLINE void nrf_saadc_channel_input_set(uint8_t           channel,
                                                 nrf_saadc_input_t pselp,
                                                 nrf_saadc_input_t pseln);

/**
 * @brief Function for configuring the positive input pin for the specified SAADC channel.
 *
 * @param[in] channel Channel number.
 * @param[in] pselp   Positive input.
 */
__STATIC_INLINE void nrf_saadc_channel_pos_input_set(uint8_t           channel,
                                                     nrf_saadc_input_t pselp);

/**
 * @brief Function for setting the SAADC channel monitoring limits.
 *
 * @param[in] channel Channel number.
 * @param[in] low     Low limit.
 * @param[in] high    High limit.
 */
__STATIC_INLINE void nrf_saadc_channel_limits_set(uint8_t channel, int16_t low, int16_t high);

/**
 * @brief Function for enabling specified SAADC interrupts.
 *
 * @param[in] saadc_int_mask Interrupt(s) to be enabled.
 */
__STATIC_INLINE void nrf_saadc_int_enable(uint32_t saadc_int_mask);

/**
 * @brief Function for retrieving the state of specified SAADC interrupts.
 *
 * @param[in] saadc_int_mask Interrupt(s) to be checked.
 *
 * @retval true  All specified interrupts are enabled.
 * @retval false At least one of the given interrupts is not enabled.
 */
__STATIC_INLINE bool nrf_saadc_int_enable_check(uint32_t saadc_int_mask);

/**
 * @brief Function for disabling specified interrupts.
 *
 * @param saadc_int_mask Interrupt(s) to be disabled.
 */
__STATIC_INLINE void nrf_saadc_int_disable(uint32_t saadc_int_mask);

/**
 * @brief Function for generating masks for SAADC channel limit interrupts.
 *
 * @param[in] channel    SAADC channel number.
 * @param[in] limit_type Limit type.
 *
 * @return Interrupt mask.
 */
__STATIC_INLINE uint32_t nrf_saadc_limit_int_get(uint8_t channel, nrf_saadc_limit_t limit_type);

/**
 * @brief Function for checking whether the SAADC is busy.
 *
 * This function checks whether the analog-to-digital converter is busy with a conversion.
 *
 * @retval true  The SAADC is busy.
 * @retval false The SAADC is not busy.
 */
__STATIC_INLINE bool nrf_saadc_busy_check(void);

/**
 * @brief Function for enabling the SAADC.
 *
 * The analog-to-digital converter must be enabled before use.
 */
__STATIC_INLINE void nrf_saadc_enable(void);

/**
 * @brief Function for disabling the SAADC.
 */
__STATIC_INLINE void nrf_saadc_disable(void);

/**
 * @brief Function for checking if the SAADC is enabled.
 *
 * @retval true  The SAADC is enabled.
 * @retval false The SAADC is not enabled.
 */
__STATIC_INLINE bool nrf_saadc_enable_check(void);

/**
 * @brief Function for initializing the SAADC result buffer.
 *
 * @param[in] p_buffer Pointer to the result buffer.
 * @param[in] size     Size of the buffer (in 16-bit samples).
 */
__STATIC_INLINE void nrf_saadc_buffer_init(nrf_saadc_value_t * p_buffer,
                                           uint32_t            size);

/**
 * @brief Function for setting the SAADC result buffer pointer.
 *
 * @param[in] p_buffer Pointer to the result buffer.
 */
__STATIC_INLINE void nrf_saadc_buffer_pointer_set(nrf_saadc_value_t * p_buffer);

/**
 * @brief Function for getting the SAADC result buffer pointer.
 *
 * @return Pointer to the result buffer.
 */
__STATIC_INLINE nrf_saadc_value_t * nrf_saadc_buffer_pointer_get(void);

/**
 * @brief Function for getting the number of samples written to the result
 *        buffer since the previous START task.
 *
 * @returns Number of 16-bit samples written to the buffer.
 */
__STATIC_INLINE uint16_t nrf_saadc_amount_get(void);

/**
 * @brief Function for setting the SAADC sample resolution.
 *
 * @param[in] resolution Bit resolution.
 */
__STATIC_INLINE void nrf_saadc_resolution_set(nrf_saadc_resolution_t resolution);

/**
 * @brief Function for configuring the oversampling feature.
 *
 * @param[in] oversample Oversampling mode.
 */
__STATIC_INLINE void nrf_saadc_oversample_set(nrf_saadc_oversample_t oversample);

/**
 * @brief Function for getting the oversampling feature configuration.
 *
 * @return Oversampling configuration.
 */
__STATIC_INLINE nrf_saadc_oversample_t nrf_saadc_oversample_get(void);

/**
 * @brief Function for enabling the continuous sampling.
 *
 * This function configures the SAADC internal timer to automatically take new samples at a fixed
 * sample rate. Trigger the START task to begin continuous sampling. To stop the sampling, trigger
 * the STOP task.
 *
 * @note The internal timer can only be used when a single input channel is enabled.
 *
 * @param[in] cc Capture and compare value. Sample rate is 16 MHz/cc. Valid CC range is
 *               from 80 to 2047.
 */
__STATIC_INLINE void nrf_saadc_continuous_mode_enable(uint16_t cc);

/**
 * @brief Function for disabling the continuous sampling.
 *
 * New samples can still be acquired by manually triggering the SAMPLE task or by PPI.
 */
__STATIC_INLINE void nrf_saadc_continuous_mode_disable(void);

/**
 * @brief Function for initializing the SAADC channel.
 *
 * @param[in] channel Channel number.
 * @param[in] config  Pointer to the channel configuration structure.
 */
__STATIC_INLINE void nrf_saadc_channel_init(uint8_t                                  channel,
                                            nrf_saadc_channel_config_t const * const config);

/**
 * @brief Function for configuring the burst mode for the specified channel.
 *
 * @param[in] channel Channel number.
 * @param[in] burst   Burst mode setting.
 */
__STATIC_INLINE void nrf_saadc_burst_set(uint8_t           channel,
                                         nrf_saadc_burst_t burst);

#ifndef SUPPRESS_INLINE_IMPLEMENTATION

__STATIC_INLINE void nrf_saadc_task_trigger(nrf_saadc_task_t task)
{
    *((volatile uint32_t *)((uint8_t *)NRF_SAADC + (uint32_t)task)) = 0x1UL;
}

__STATIC_INLINE uint32_t nrf_saadc_task_address_get(nrf_saadc_task_t task)
{
    return (uint32_t)((uint8_t *)NRF_SAADC + (uint32_t)task);
}

__STATIC_INLINE bool nrf_saadc_event_check(nrf_saadc_event_t event)
{
    return (bool)*(volatile uint32_t *)((uint8_t *)NRF_SAADC + (uint32_t)event);
}

__STATIC_INLINE void nrf_saadc_event_clear(nrf_saadc_event_t event)
{
    *((volatile uint32_t *)((uint8_t *)NRF_SAADC + (uint32_t)event)) = 0x0UL;
#if __CORTEX_M == 0x04
    volatile uint32_t dummy = *((volatile uint32_t *)((uint8_t *)NRF_SAADC + (uint32_t)event));
    (void)dummy;
#endif
}

__STATIC_INLINE uint32_t  nrf_saadc_event_address_get(nrf_saadc_event_t event)
{
    return (uint32_t )((uint8_t *)NRF_SAADC + (uint32_t)event);
}

#if defined(DPPI_PRESENT)
__STATIC_INLINE void nrf_saadc_subscribe_set(nrf_saadc_task_t task,
                                             uint8_t          channel)
{
    *((volatile uint32_t *) ((uint8_t *) NRF_SAADC + (uint32_t) task + 0x80uL)) =
            ((uint32_t)channel | SAADC_SUBSCRIBE_START_EN_Msk);
}

__STATIC_INLINE void nrf_saadc_subscribe_clear(nrf_saadc_task_t task)
{
    *((volatile uint32_t *) ((uint8_t *) NRF_SAADC + (uint32_t) task + 0x80uL)) = 0;
}

__STATIC_INLINE void nrf_saadc_publish_set(nrf_saadc_event_t event,
                                           uint8_t           channel)
{
    *((volatile uint32_t *) ((uint8_t *) NRF_SAADC + (uint32_t) event + 0x80uL)) =
            ((uint32_t)channel | SAADC_PUBLISH_STARTED_EN_Msk);
}

__STATIC_INLINE void nrf_saadc_publish_clear(nrf_saadc_event_t event)
{
    *((volatile uint32_t *) ((uint8_t *) NRF_SAADC + (uint32_t) event + 0x80uL)) = 0;
}
#endif // defined(DPPI_PRESENT)

__STATIC_INLINE volatile uint32_t * nrf_saadc_event_limit_address_get(uint8_t           channel,
                                                                      nrf_saadc_limit_t limit_type)
{
    NRFX_ASSERT(channel < NRF_SAADC_CHANNEL_COUNT);
    if (limit_type == NRF_SAADC_LIMIT_HIGH)
    {
        return &NRF_SAADC->EVENTS_CH[channel].LIMITH;
    }
    else
    {
        return &NRF_SAADC->EVENTS_CH[channel].LIMITL;
    }
}

__STATIC_INLINE nrf_saadc_event_t nrf_saadc_event_limit_get(uint8_t           channel,
                                                            nrf_saadc_limit_t limit_type)
{
    if (limit_type == NRF_SAADC_LIMIT_HIGH)
    {
        return (nrf_saadc_event_t)( (uint32_t) NRF_SAADC_EVENT_CH0_LIMITH +
                        (uint32_t) (NRF_SAADC_EVENT_CH1_LIMITH - NRF_SAADC_EVENT_CH0_LIMITH)
                        * (uint32_t) channel );
    }
    else
    {
        return (nrf_saadc_event_t)( (uint32_t) NRF_SAADC_EVENT_CH0_LIMITL +
                        (uint32_t) (NRF_SAADC_EVENT_CH1_LIMITL - NRF_SAADC_EVENT_CH0_LIMITL)
                        * (uint32_t) channel );
    }
}

__STATIC_INLINE void nrf_saadc_channel_input_set(uint8_t           channel,
                                                 nrf_saadc_input_t pselp,
                                                 nrf_saadc_input_t pseln)
{
    NRF_SAADC->CH[channel].PSELN = pseln;
    NRF_SAADC->CH[channel].PSELP = pselp;
}

__STATIC_INLINE void nrf_saadc_channel_pos_input_set(uint8_t           channel,
                                                     nrf_saadc_input_t pselp)
{
    NRF_SAADC->CH[channel].PSELP = pselp;
}

__STATIC_INLINE void nrf_saadc_channel_limits_set(uint8_t channel, int16_t low, int16_t high)
{
    NRF_SAADC->CH[channel].LIMIT = (
            (((uint32_t) low << SAADC_CH_LIMIT_LOW_Pos) & SAADC_CH_LIMIT_LOW_Msk)
          | (((uint32_t) high << SAADC_CH_LIMIT_HIGH_Pos) & SAADC_CH_LIMIT_HIGH_Msk));
}

__STATIC_INLINE void nrf_saadc_int_enable(uint32_t saadc_int_mask)
{
    NRF_SAADC->INTENSET = saadc_int_mask;
}

__STATIC_INLINE bool nrf_saadc_int_enable_check(uint32_t saadc_int_mask)
{
    return (bool)(NRF_SAADC->INTENSET & saadc_int_mask);
}

__STATIC_INLINE void nrf_saadc_int_disable(uint32_t saadc_int_mask)
{
    NRF_SAADC->INTENCLR = saadc_int_mask;
}

__STATIC_INLINE uint32_t nrf_saadc_limit_int_get(uint8_t channel, nrf_saadc_limit_t limit_type)
{
    NRFX_ASSERT(channel < NRF_SAADC_CHANNEL_COUNT);
    uint32_t mask = (limit_type == NRF_SAADC_LIMIT_LOW) ?
                     NRF_SAADC_INT_CH0LIMITL : NRF_SAADC_INT_CH0LIMITH;
    return mask << (channel * 2);
}

__STATIC_INLINE bool nrf_saadc_busy_check(void)
{
    //return ((NRF_SAADC->STATUS & SAADC_STATUS_STATUS_Msk) == SAADC_STATUS_STATUS_Msk);
    //simplified for performance
    return NRF_SAADC->STATUS;
}

__STATIC_INLINE void nrf_saadc_enable(void)
{
    NRF_SAADC->ENABLE = (SAADC_ENABLE_ENABLE_Enabled << SAADC_ENABLE_ENABLE_Pos);
}

__STATIC_INLINE void nrf_saadc_disable(void)
{
    NRF_SAADC->ENABLE = (SAADC_ENABLE_ENABLE_Disabled << SAADC_ENABLE_ENABLE_Pos);
}

__STATIC_INLINE bool nrf_saadc_enable_check(void)
{
    //simplified for performance
    return NRF_SAADC->ENABLE;
}

__STATIC_INLINE void nrf_saadc_buffer_init(nrf_saadc_value_t * p_buffer,
                                           uint32_t            size)
{
    NRF_SAADC->RESULT.PTR = (uint32_t)p_buffer;
    NRF_SAADC->RESULT.MAXCNT = size;
}

__STATIC_INLINE void nrf_saadc_buffer_pointer_set(nrf_saadc_value_t * p_buffer)
{
    NRF_SAADC->RESULT.PTR = (uint32_t)p_buffer;
}

__STATIC_INLINE nrf_saadc_value_t * nrf_saadc_buffer_pointer_get(void)
{
    return (nrf_saadc_value_t *)NRF_SAADC->RESULT.PTR;
}

__STATIC_INLINE uint16_t nrf_saadc_amount_get(void)
{
    return NRF_SAADC->RESULT.AMOUNT;
}

__STATIC_INLINE void nrf_saadc_resolution_set(nrf_saadc_resolution_t resolution)
{
    NRF_SAADC->RESOLUTION = resolution;
}

__STATIC_INLINE void nrf_saadc_oversample_set(nrf_saadc_oversample_t oversample)
{
    NRF_SAADC->OVERSAMPLE = oversample;
}

__STATIC_INLINE nrf_saadc_oversample_t nrf_saadc_oversample_get(void)
{
    return (nrf_saadc_oversample_t)NRF_SAADC->OVERSAMPLE;
}

__STATIC_INLINE void nrf_saadc_continuous_mode_enable(uint16_t cc)
{
    NRFX_ASSERT((cc >= 80) && (cc <= 2047));
    NRF_SAADC->SAMPLERATE = (SAADC_SAMPLERATE_MODE_Timers << SAADC_SAMPLERATE_MODE_Pos)
                            | ((uint32_t)cc << SAADC_SAMPLERATE_CC_Pos);
}

__STATIC_INLINE void nrf_saadc_continuous_mode_disable(void)
{
    NRF_SAADC->SAMPLERATE = SAADC_SAMPLERATE_MODE_Task << SAADC_SAMPLERATE_MODE_Pos;
}

__STATIC_INLINE void nrf_saadc_channel_init(uint8_t                                  channel,
                                            nrf_saadc_channel_config_t const * const config)
{
    NRF_SAADC->CH[channel].CONFIG =
            ((config->resistor_p   << SAADC_CH_CONFIG_RESP_Pos)   & SAADC_CH_CONFIG_RESP_Msk)
            | ((config->resistor_n << SAADC_CH_CONFIG_RESN_Pos)   & SAADC_CH_CONFIG_RESN_Msk)
            | ((config->gain       << SAADC_CH_CONFIG_GAIN_Pos)   & SAADC_CH_CONFIG_GAIN_Msk)
            | ((config->reference  << SAADC_CH_CONFIG_REFSEL_Pos) & SAADC_CH_CONFIG_REFSEL_Msk)
            | ((config->acq_time   << SAADC_CH_CONFIG_TACQ_Pos)   & SAADC_CH_CONFIG_TACQ_Msk)
            | ((config->mode       << SAADC_CH_CONFIG_MODE_Pos)   & SAADC_CH_CONFIG_MODE_Msk)
            | ((config->burst      << SAADC_CH_CONFIG_BURST_Pos)  & SAADC_CH_CONFIG_BURST_Msk);
    nrf_saadc_channel_input_set(channel, config->pin_p, config->pin_n);
}

__STATIC_INLINE void nrf_saadc_burst_set(uint8_t           channel,
                                         nrf_saadc_burst_t burst)
{
    NRF_SAADC->CH[channel].CONFIG =
        (NRF_SAADC->CH[channel].CONFIG & ~SAADC_CH_CONFIG_BURST_Msk) |
        (burst << SAADC_CH_CONFIG_BURST_Pos);
}

#endif // SUPPRESS_INLINE_IMPLEMENTATION

/** @} */

#ifdef __cplusplus
}
#endif

#endif // NRF_SAADC_H_
