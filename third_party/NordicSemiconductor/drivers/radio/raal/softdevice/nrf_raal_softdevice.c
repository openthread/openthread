/* Copyright (c) 2017, Nordic Semiconductor ASA
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   1. Redistributions of source code must retain the above copyright notice, this
 *      list of conditions and the following disclaimer.
 *
 *   2. Redistributions in binary form must reproduce the above copyright notice,
 *      this list of conditions and the following disclaimer in the documentation
 *      and/or other materials provided with the distribution.
 *
 *   3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *      contributors may be used to endorse or promote products derived from
 *      this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

/**
 * @file
 *   This file implements the nrf 802.15.4 radio arbiter for softdevice.
 *
 * This arbiter should be used when 802.15.4 works concurrently with SoftDevice's radio stack.
 *
 */

#include "nrf_raal_softdevice.h"

#include <assert.h>
#include <stdbool.h>
#include <string.h>

#include <hal/nrf_timer.h>
#include <nrf_raal_api.h>
#include <nrf_drv_radio802154.h>
#include <nrf_drv_radio802154_debug.h>
#include <platform/clock/nrf_drv_radio802154_clock.h>

#if defined(__GNUC__)
    _Pragma("GCC diagnostic push")
    _Pragma("GCC diagnostic ignored \"-Wreturn-type\"")
    _Pragma("GCC diagnostic ignored \"-Wunused-parameter\"")
    _Pragma("GCC diagnostic ignored \"-Wpedantic\"")
#endif

#include <nrf_soc.h>

#if defined(__GNUC__)
    _Pragma("GCC diagnostic pop")
#endif

/**@brief Enable Request and End on timeslot safety interrupt. */
#define ENABLE_REQUEST_AND_END_ON_TIMESLOT_END 0

/**@brief RAAL Timer instance. */
#define RAAL_TIMER                NRF_TIMER0

/**@brief RAAL Timer interrupt number. */
#define RAAL_TIMER_IRQn           TIMER0_IRQn

/**@brief Maximum jitter relative to the start time of START and TIMER0 (safety margin) events . */
#define TIMER_TO_SIGNAL_JITTER_US NRF_RADIO_START_JITTER_US + 6

/**@brief Timer compare channel definitions. */
#define TIMER_CC_EXTEND           NRF_TIMER_CC_CHANNEL0
#define TIMER_CC_EXTEND_EVENT     NRF_TIMER_EVENT_COMPARE0
#define TIMER_CC_EXTEND_INT       NRF_TIMER_INT_COMPARE0_MASK

#define TIMER_CC_MARGIN           NRF_TIMER_CC_CHANNEL1
#define TIMER_CC_MARGIN_EVENT     NRF_TIMER_EVENT_COMPARE1
#define TIMER_CC_MARGIN_INT       NRF_TIMER_INT_COMPARE1_MASK

#define TIMER_CC_CAPTURE          NRF_TIMER_CC_CHANNEL2

/**@brief Defines number of microseconds in one second. */
#define US_PER_S                  1000000

/**@brief Ceil division helper */
#define DIVIDE_AND_CEIL(A, B) (((A) + (B) - 1) / (B))

/**@brief Defines pending events. */
typedef enum
{
    PENDING_EVENT_NONE = 0,
    PENDING_EVENT_STARTED,
    PENDING_EVENT_ENDED
} pending_events_t;

/**@brief Request parameters. */
static nrf_radio_request_t m_request;

/**@brief Return parameter for SD radio signal handler. */
static nrf_radio_signal_callback_return_param_t m_ret_param;

/**@brief Current configuration of the RAAL. */
static nrf_raal_softdevice_cfg_t m_config;

/**@brief Current timeslot length. */
static uint16_t m_timeslot_length;

/**@brief Defines if RAAL is in continous mode. */
static volatile bool m_continuous = false;

/**@brief Defines if RAAL is currently in a timeslot. */
static volatile bool m_in_timeslot = false;

/**@brief Current iteration process number. */
static volatile uint16_t m_alloc_iters;

/**@brief Defines if module has been initialized. */
static bool m_initialize = false;

/**@breif Defines if Radio Driver entered critical section. */
static volatile bool m_in_critical_section = false;

/**@brief Defines current pending event. */
static volatile pending_events_t m_pending_event = PENDING_EVENT_NONE;

/**@brief Defines RTC0 counter value on timeslot begin. */
static uint32_t m_start_rtc_ticks = 0;

/**@brief Initialize timeslot internal variables. */
static inline void timeslot_data_init(void)
{
    m_alloc_iters     = 0;
    m_timeslot_length = m_config.timeslot_length;
}

/**@brief Get actual time. */
static inline uint32_t timer_time_get(void)
{
    RAAL_TIMER->TASKS_CAPTURE[TIMER_CC_CAPTURE] = 1;
    return RAAL_TIMER->CC[TIMER_CC_CAPTURE];
}

/**@brief Check if margin is already reached. */
static inline bool timer_is_margin_reached(void)
{
    return RAAL_TIMER->EVENTS_COMPARE[TIMER_CC_MARGIN];
}

/**@brief Enter timeslot critical section. */
static inline void timeslot_critical_section_enter(void)
{
    NVIC_DisableIRQ(RAAL_TIMER_IRQn);
    __DSB();
    __ISB();
}

/**@brief Exit timeslot critical section. */
static inline void timeslot_critical_section_exit(void)
{
    NVIC_EnableIRQ(RAAL_TIMER_IRQn);
}

static inline void timeslot_started_notify(void)
{
    if (m_in_timeslot && m_continuous)
    {
        nrf_raal_timeslot_started();
    }
}

static inline void timeslot_ended_notify(void)
{
    if (!m_in_timeslot && m_continuous)
    {
        nrf_raal_timeslot_ended();
    }
}

/**@brief Calculate maximal crystal drift. */
static inline uint32_t rtc_drift_calculate(uint32_t timeslot_length)
{
    return DIVIDE_AND_CEIL(((uint64_t)timeslot_length * m_config.lf_clk_accuracy_ppm), US_PER_S);
}

/**@brief Prepare earliest timeslot request. */
static void timeslot_request_prepare(void)
{
    memset(&m_request, 0, sizeof(m_request));
    m_request.request_type               = NRF_RADIO_REQ_TYPE_EARLIEST;
    m_request.params.earliest.hfclk      = NRF_RADIO_HFCLK_CFG_NO_GUARANTEE;
    m_request.params.earliest.priority   = NRF_RADIO_PRIORITY_NORMAL;
    m_request.params.earliest.length_us  = m_timeslot_length;
    m_request.params.earliest.timeout_us = m_config.timeslot_timeout;
}

/**@brief Request earliest timeslot. */
static void timeslot_request(void)
{
    timeslot_request_prepare();
    sd_radio_request(&m_request);
}

/**@brief Set timer on timeslot started. */
static void timer_start(void)
{
    nrf_timer_task_trigger(RAAL_TIMER, NRF_TIMER_TASK_STOP);
    nrf_timer_task_trigger(RAAL_TIMER, NRF_TIMER_TASK_CLEAR);
    nrf_timer_bit_width_set(RAAL_TIMER, NRF_TIMER_BIT_WIDTH_32);
    nrf_timer_int_enable(RAAL_TIMER, TIMER_CC_MARGIN_INT);
    nrf_timer_cc_write(RAAL_TIMER, TIMER_CC_EXTEND, 0);
    nrf_timer_cc_write(RAAL_TIMER, TIMER_CC_MARGIN, m_timeslot_length - m_config.timeslot_safe_margin);
    nrf_timer_task_trigger(RAAL_TIMER, NRF_TIMER_TASK_START);

    NVIC_EnableIRQ(RAAL_TIMER_IRQn);
}

/**@brief Reset timer. */
static void timer_reset(void)
{
    nrf_timer_task_trigger(RAAL_TIMER, NRF_TIMER_TASK_STOP);
    nrf_timer_event_clear(RAAL_TIMER, TIMER_CC_EXTEND_EVENT);
    nrf_timer_event_clear(RAAL_TIMER, TIMER_CC_MARGIN_EVENT);
    NVIC_ClearPendingIRQ(RAAL_TIMER_IRQn);
}

/**@brief Set timer on extend event. */
static void timer_extend(void)
{
    NVIC_ClearPendingIRQ(RAAL_TIMER_IRQn);

    nrf_timer_int_enable(RAAL_TIMER, TIMER_CC_MARGIN_INT);
    nrf_timer_cc_write(RAAL_TIMER,
                       TIMER_CC_MARGIN,
                       nrf_timer_cc_read(RAAL_TIMER, TIMER_CC_MARGIN) + m_timeslot_length);

    if (!m_alloc_iters)
    {
        nrf_timer_int_enable(RAAL_TIMER, TIMER_CC_EXTEND_INT);
        nrf_timer_cc_write(RAAL_TIMER,
                           TIMER_CC_EXTEND,
                           nrf_timer_cc_read(RAAL_TIMER, TIMER_CC_EXTEND) + m_timeslot_length);
    }
}

/**@brief Eliminate timers jitters. */
static void timer_jitter_adjust(void)
{
    // Adjust TIMER0 and RTC0 clocks drifts.
    uint32_t timer_ticks = timer_time_get();
    uint64_t rtc_ticks   = NRF_RTC0->COUNTER;

    if (rtc_ticks > m_start_rtc_ticks)
    {
        rtc_ticks -= m_start_rtc_ticks;
    }
    else
    {
        // Overflow detected.
        rtc_ticks = RTC_COUNTER_COUNTER_Msk - m_start_rtc_ticks + rtc_ticks;
    }

    // Adjust RTC0 ticks to TIMER0 resolution. RTC0 works with 32768kHz so first
    // multiply with 10^6 (microseconds) and divide by 32768Hz (2^15) to get microseconds.
    rtc_ticks = DIVIDE_AND_CEIL((rtc_ticks * US_PER_S), 32768);

    // Check if we are still in time.
    uint32_t cc_margin = nrf_timer_cc_read(RAAL_TIMER, TIMER_CC_MARGIN);
    assert(cc_margin > rtc_ticks + rtc_drift_calculate(cc_margin));
    
    if (rtc_ticks > timer_ticks)
    {
        nrf_timer_cc_write(RAAL_TIMER,
                           TIMER_CC_MARGIN,
                           cc_margin - (rtc_ticks - timer_ticks));
    }
    else
    {
        nrf_timer_cc_write(RAAL_TIMER,
                           TIMER_CC_MARGIN,
                           cc_margin + (rtc_ticks - timer_ticks));
    }

    // Add safety drift time.
    uint32_t safety_drift_time = rtc_drift_calculate(2 * m_config.timeslot_length) +
                                 TIMER_TO_SIGNAL_JITTER_US;
    nrf_timer_cc_write(RAAL_TIMER,
                       TIMER_CC_MARGIN,
                       nrf_timer_cc_read(RAAL_TIMER, TIMER_CC_MARGIN) - safety_drift_time);
}

/**@brief Decrease timeslot length. */
static void timeslot_decrease_length(void)
{
    m_alloc_iters++;
    m_timeslot_length = m_timeslot_length >> 1;
}

/**@brief Extend timeslot. */
static void timeslot_extend(void)
{
    if (m_alloc_iters < m_config.timeslot_alloc_iters)
    {
        timeslot_decrease_length();

        // Try to extend right after start.
        m_ret_param.callback_action         = NRF_RADIO_SIGNAL_CALLBACK_ACTION_EXTEND;
        m_ret_param.params.extend.length_us = m_timeslot_length;

        nrf_drv_radio802154_pin_set(PIN_DBG_TIMESLOT_EXTEND_REQ);
    }
    else
    {
        timer_jitter_adjust();
    }
}

static void timer_irq_handle(void)
{
    // Safe margin exceeded.
    if (RAAL_TIMER->EVENTS_COMPARE[TIMER_CC_MARGIN])
    {
        nrf_drv_radio802154_pin_clr(PIN_DBG_TIMESLOT_ACTIVE);
        nrf_drv_radio802154_log(EVENT_TRACE_ENTER, FUNCTION_RAAL_SIG_EVENT_MARGIN);

        m_in_timeslot = false;

        if (m_in_critical_section)
        {
            assert(m_pending_event != PENDING_EVENT_ENDED);

            if (m_pending_event == PENDING_EVENT_STARTED)
            {
                m_pending_event = PENDING_EVENT_NONE;
            }
            else
            {
                m_pending_event = PENDING_EVENT_ENDED;
            }
        }
        else
        {
            timeslot_ended_notify();
        }

        // Ignore any other events.
        timer_reset();

#if (ENABLE_REQUEST_AND_END_ON_TIMESLOT_END == 1)
        timeslot_data_init();
        timeslot_request_prepare();
        m_ret_param.callback_action       = NRF_RADIO_SIGNAL_CALLBACK_ACTION_REQUEST_AND_END;
        m_ret_param.params.request.p_next = &m_request;
#else
        // Return and wait for NRF_EVT_RADIO_SESSION_IDLE event.
        m_ret_param.callback_action = NRF_RADIO_SIGNAL_CALLBACK_ACTION_NONE;
#endif

        nrf_drv_radio802154_log(EVENT_TRACE_EXIT, FUNCTION_RAAL_SIG_EVENT_MARGIN);
    }

    // Extension margin exceeded.
    else if (RAAL_TIMER->EVENTS_COMPARE[TIMER_CC_EXTEND])
    {
        nrf_drv_radio802154_log(EVENT_TRACE_ENTER, FUNCTION_RAAL_SIG_EVENT_EXTEND);

        nrf_timer_int_disable(RAAL_TIMER, TIMER_CC_EXTEND_INT);
        nrf_timer_event_clear(RAAL_TIMER, TIMER_CC_EXTEND_EVENT);

        if (m_continuous &&
            (nrf_timer_cc_read(RAAL_TIMER, TIMER_CC_EXTEND) +
            m_config.timeslot_length < m_config.timeslot_max_length))
        {
            nrf_drv_radio802154_pin_set(PIN_DBG_TIMESLOT_EXTEND_REQ);

            m_ret_param.callback_action         = NRF_RADIO_SIGNAL_CALLBACK_ACTION_EXTEND;
            m_ret_param.params.extend.length_us = m_config.timeslot_length;
        }
        else
        {
            timer_jitter_adjust();

            m_ret_param.callback_action = NRF_RADIO_SIGNAL_CALLBACK_ACTION_NONE;
        }

        nrf_drv_radio802154_log(EVENT_TRACE_EXIT, FUNCTION_RAAL_SIG_EVENT_EXTEND);
    }
    else
    {
        // Should not happen.
        assert(false);
    }
}

/**@brief Signal handler. */
static nrf_radio_signal_callback_return_param_t *signal_handler(uint8_t signal_type)
{
    nrf_drv_radio802154_log(EVENT_TRACE_ENTER, FUNCTION_RAAL_SIG_HANDLER);

    // Default response.
    m_ret_param.callback_action = NRF_RADIO_SIGNAL_CALLBACK_ACTION_NONE;

    if (!m_continuous)
    {
        nrf_drv_radio802154_pin_clr(PIN_DBG_TIMESLOT_ACTIVE);
        nrf_drv_radio802154_log(EVENT_TRACE_ENTER, FUNCTION_RAAL_SIG_EVENT_ENDED);

        m_pending_event = PENDING_EVENT_NONE;
        m_in_timeslot   = false;

        // TODO: Change to NRF_RADIO_SIGNAL_CALLBACK_ACTION_END (KRKNWK-937)
        m_ret_param.callback_action = NRF_RADIO_SIGNAL_CALLBACK_ACTION_NONE;
        timer_reset();

        nrf_drv_radio802154_log(EVENT_TRACE_EXIT, FUNCTION_RAAL_SIG_EVENT_ENDED);
        nrf_drv_radio802154_log(EVENT_TRACE_EXIT, FUNCTION_RAAL_SIG_HANDLER);
        return &m_ret_param;
    }

    switch (signal_type)
    {
    case NRF_RADIO_CALLBACK_SIGNAL_TYPE_START: /**< This signal indicates the start of the radio timeslot. */
    {
        nrf_drv_radio802154_pin_set(PIN_DBG_TIMESLOT_ACTIVE);
        nrf_drv_radio802154_log(EVENT_TRACE_ENTER, FUNCTION_RAAL_SIG_EVENT_START);

        // Ensure HFCLK is running before start is issued.
        assert(NRF_CLOCK->HFCLKSTAT == (CLOCK_HFCLKSTAT_SRC_Msk | CLOCK_HFCLKSTAT_STATE_Msk));

        timer_start();

        m_start_rtc_ticks = NRF_RTC0->COUNTER;
        m_in_timeslot     = true;

        timeslot_data_init();

        if (m_in_critical_section)
        {
            assert(m_pending_event != PENDING_EVENT_STARTED);

            if (m_pending_event == PENDING_EVENT_ENDED)
            {
                m_pending_event = PENDING_EVENT_NONE;
            }
            else
            {
                m_pending_event = PENDING_EVENT_STARTED;
            }
        }
        else
        {
            timeslot_started_notify();
        }

        // Try to extend right after start.
        m_ret_param.callback_action         = NRF_RADIO_SIGNAL_CALLBACK_ACTION_EXTEND;
        m_ret_param.params.extend.length_us = m_timeslot_length;

        nrf_drv_radio802154_log(EVENT_TRACE_EXIT, FUNCTION_RAAL_SIG_EVENT_START);
        nrf_drv_radio802154_pin_set(PIN_DBG_TIMESLOT_EXTEND_REQ);
        break;
    }

    case NRF_RADIO_CALLBACK_SIGNAL_TYPE_TIMER0: /**< This signal indicates the TIMER0 interrupt. */
        timer_irq_handle();
        break;

    case NRF_RADIO_CALLBACK_SIGNAL_TYPE_RADIO: /**< This signal indicates the NRF_RADIO interrupt. */
        nrf_drv_radio802154_pin_set(PIN_DBG_TIMESLOT_RADIO_IRQ);
        nrf_drv_radio802154_log(EVENT_TRACE_ENTER, FUNCTION_RAAL_SIG_EVENT_RADIO);

        if (m_in_timeslot)
        {
            if (!timer_is_margin_reached())
            {
                nrf_drv_radio802154_irq_handler();
            }
            else
            {
                //  Handle margin exceeded event.
                timer_irq_handle();
            }
        }

        nrf_drv_radio802154_log(EVENT_TRACE_EXIT, FUNCTION_RAAL_SIG_EVENT_RADIO);
        nrf_drv_radio802154_pin_clr(PIN_DBG_TIMESLOT_RADIO_IRQ);
        break;

    case NRF_RADIO_CALLBACK_SIGNAL_TYPE_EXTEND_FAILED: /**< This signal indicates extend action failed. */
        nrf_drv_radio802154_pin_clr(PIN_DBG_TIMESLOT_EXTEND_REQ);
        nrf_drv_radio802154_pin_tgl(PIN_DBG_TIMESLOT_FAILED);
        nrf_drv_radio802154_log(EVENT_TRACE_ENTER, FUNCTION_RAAL_SIG_EVENT_EXCEED_FAIL);

        timeslot_extend();

        nrf_drv_radio802154_log(EVENT_TRACE_EXIT, FUNCTION_RAAL_SIG_EVENT_EXCEED_FAIL);
        break;

    case NRF_RADIO_CALLBACK_SIGNAL_TYPE_EXTEND_SUCCEEDED: /**< This signal indicates extend action succeeded. */
        nrf_drv_radio802154_pin_clr(PIN_DBG_TIMESLOT_EXTEND_REQ);
        nrf_drv_radio802154_log(EVENT_TRACE_ENTER, FUNCTION_RAAL_SIG_EVENT_EXCEED_SUCCESS);

        timer_extend();

        if (m_alloc_iters != 0)
        {
            timeslot_extend();
        }

        nrf_drv_radio802154_log(EVENT_TRACE_EXIT, FUNCTION_RAAL_SIG_EVENT_EXCEED_SUCCESS);
        break;

    default:
        break;
    }

    nrf_drv_radio802154_log(EVENT_TRACE_EXIT, FUNCTION_RAAL_SIG_HANDLER);

    return &m_ret_param;
}

void nrf_raal_softdevice_soc_evt_handler(uint32_t evt_id)
{
    switch (evt_id)
    {
    case NRF_EVT_RADIO_BLOCKED:
    case NRF_EVT_RADIO_CANCELED:
    {
        nrf_drv_radio802154_pin_tgl(PIN_DBG_TIMESLOT_BLOCKED);

        assert(!m_in_timeslot);

        if (m_continuous)
        {
            if (m_alloc_iters < m_config.timeslot_alloc_iters)
            {
                timeslot_decrease_length();
            }

            timeslot_request();
        }

        break;
    }

    case NRF_EVT_RADIO_SIGNAL_CALLBACK_INVALID_RETURN:
        assert(false);
        break;

    case NRF_EVT_RADIO_SESSION_IDLE:
        if (m_continuous)
        {
            nrf_drv_radio802154_pin_tgl(PIN_DBG_TIMESLOT_SESSION_IDLE);

            timeslot_data_init();
            timeslot_request();
        }

        break;

    case NRF_EVT_RADIO_SESSION_CLOSED:
        break;

    default:
        break;
    }
}

void nrf_raal_softdevice_config(const nrf_raal_softdevice_cfg_t * p_cfg)
{
    assert(m_initialize);
    assert(!m_continuous);
    assert(p_cfg);

    m_config = *p_cfg;
}

void nrf_raal_init(void)
{
    assert(!m_initialize);

    m_continuous = false;
    m_in_timeslot = false;

    m_config.lf_clk_accuracy_ppm  = NRF_RAAL_DEFAULT_LF_CLK_ACCURACY_PPM;
    m_config.timeslot_length      = NRF_RAAL_TIMESLOT_DEFAULT_LENGTH;
    m_config.timeslot_alloc_iters = NRF_RAAL_TIMESLOT_DEFAULT_ALLOC_ITERS;
    m_config.timeslot_safe_margin = NRF_RAAL_TIMESLOT_DEFAULT_SAFE_MARGIN;
    m_config.timeslot_max_length  = NRF_RAAL_TIMESLOT_DEFAULT_MAX_LENGTH;
    m_config.timeslot_timeout     = NRF_RAAL_TIMESLOT_DEFAULT_TIMEOUT;

    uint32_t err_code = sd_radio_session_open(signal_handler);
    assert(err_code == NRF_SUCCESS);

    m_initialize = true;
}

void nrf_raal_uninit(void)
{
    assert(m_initialize);

    uint32_t err_code = sd_radio_session_close();
    assert(err_code == NRF_SUCCESS);

    m_continuous  = false;
    m_in_timeslot = false;

    nrf_drv_radio802154_pin_clr(PIN_DBG_TIMESLOT_ACTIVE);
}

void nrf_raal_continuous_mode_enter(void)
{
    nrf_drv_radio802154_log(EVENT_TRACE_ENTER, FUNCTION_RAAL_CONTINUOUS_ENTER);

    assert(m_initialize);
    assert(!m_continuous);

    m_alloc_iters     = 0;
    m_timeslot_length = m_config.timeslot_length;
    m_continuous      = true;

    nrf_drv_radio802154_clock_hfclk_start();

    nrf_drv_radio802154_log(EVENT_TRACE_EXIT, FUNCTION_RAAL_CONTINUOUS_ENTER);
}

void nrf_raal_continuous_mode_exit(void)
{
    nrf_drv_radio802154_log(EVENT_TRACE_ENTER, FUNCTION_RAAL_CONTINUOUS_EXIT);

    assert(m_initialize);
    assert(m_continuous);

    m_continuous = false;

    // Emulate signal interrupt to inform SD about end of m_continuous mode.
    if (m_in_timeslot)
    {
        NVIC_SetPendingIRQ(RAAL_TIMER_IRQn);
    }

    nrf_drv_radio802154_clock_hfclk_stop();

    nrf_drv_radio802154_log(EVENT_TRACE_EXIT, FUNCTION_RAAL_CONTINUOUS_EXIT);
}

bool nrf_raal_timeslot_request(uint32_t length_us)
{
    if (!m_continuous || !m_in_timeslot)
    {
        return false;
    }

    return timer_time_get() + length_us < nrf_timer_cc_read(RAAL_TIMER, TIMER_CC_MARGIN);
}

uint32_t nrf_raal_timeslot_us_left_get(void)
{
    if (!m_continuous || !m_in_timeslot)
    {
        return 0;
    }

    return nrf_timer_cc_read(RAAL_TIMER, TIMER_CC_MARGIN) - timer_time_get();
}

void nrf_raal_critical_section_enter(void)
{
    nrf_drv_radio802154_log(EVENT_TRACE_ENTER, FUNCTION_RAAL_CRIT_SECT_ENTER);

    assert(!m_in_critical_section);
    m_in_critical_section = true;

    nrf_drv_radio802154_log(EVENT_TRACE_EXIT, FUNCTION_RAAL_CRIT_SECT_ENTER);
}

void nrf_raal_critical_section_exit(void)
{
    nrf_drv_radio802154_log(EVENT_TRACE_ENTER, FUNCTION_RAAL_CRIT_SECT_EXIT);

    timeslot_critical_section_enter();

    assert(m_in_critical_section);
    m_in_critical_section = false;

    switch (m_pending_event)
    {
    case PENDING_EVENT_STARTED:
        timeslot_started_notify();
        break;

    case PENDING_EVENT_ENDED:
        timeslot_ended_notify();
        break;

    default:
        break;
    }

    m_pending_event = PENDING_EVENT_NONE;

    timeslot_critical_section_exit();

    nrf_drv_radio802154_log(EVENT_TRACE_EXIT, FUNCTION_RAAL_CRIT_SECT_EXIT);
}

void nrf_drv_radio802154_clock_hfclk_ready(void)
{
    if (m_continuous && !m_in_timeslot)
    {
        timeslot_request();
    }
}
