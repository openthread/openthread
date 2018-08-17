#include "nrf_802154_rsch.h"

#include <assert.h>
#include <stddef.h>
#include <nrf.h>

#include "nrf_802154_debug.h"
#include "platform/clock/nrf_802154_clock.h"
#include "raal/nrf_raal_api.h"
#include "timer_scheduler/nrf_802154_timer_sched.h"

#define PREC_RAMP_UP_TIME 300  ///< Ramp-up time of preconditions [us]. 300 is worst case for HFclock

typedef enum
{
    RSCH_PREC_STATE_IDLE,
    RSCH_PREC_STATE_REQUESTED,
    RSCH_PREC_STATE_APPROVED,
} rsch_prec_state_t;

static volatile uint8_t           m_mutex;                      ///< Mutex for notyfying core.
static volatile uint8_t           m_mutex_monitor;              ///< Mutex monitor, incremented every failed mutex lock.
static volatile bool              m_last_notified_approved;     ///< Last reported state was approved.
static volatile rsch_prec_state_t m_prec_states[RSCH_PREC_CNT]; ///< State of all preconditions.
static bool                       m_in_cont_mode;               ///< If RSCH operates in continuous mode.

static bool               m_delayed_timeslot_is_scheduled;  ///< If delayed timeslot is scheduled at the moment.
static uint32_t           m_delayed_timeslot_t0;            ///< Time base of the delayed timeslot trigger time.
static uint32_t           m_delayed_timeslot_dt;            ///< Time delta of the delayed timeslot trigger time.
static nrf_802154_timer_t m_timer;                          ///< Timer used to trigger delayed timeslot.

/** @brief Non-blocking mutex for notifying core.
 *
 *  @retval  true   Mutex was acquired.
 *  @retval  false  Mutex could not be acquired.
 */
static inline bool mutex_trylock(void)
{
    do
    {
        uint8_t mutex_value = __LDREXB(&m_mutex);

        if (mutex_value)
        {
            __CLREX();

            m_mutex_monitor++;
            return false;
        }
    } while (__STREXB(1, &m_mutex));

    __DMB();

    return true;
}

/** @brief Release mutex. */
static inline void mutex_unlock(void)
{
    __DMB();
    m_mutex = 0;
}

/** @brief Check if any precondition should be requested at the moment for delayed timeslot.
 *
 * To meet delayed timeslot timing requirements there is a time window in which radio
 * preconditions should be requested. This function is used to prevent releasing preconditions
 * in this time window.
 *
 * @retval true   A precondition should be requested at the moment for delayed timeslot feature.
 * @retval false  None of preconditions should be requested at the moment for delayed timeslot.
 */
static bool any_prec_should_be_requested_for_delayed_timeslot(void)
{
    uint32_t now = nrf_802154_timer_sched_time_get();
    uint32_t t0  = m_delayed_timeslot_t0;
    uint32_t dt  = m_delayed_timeslot_dt - PREC_RAMP_UP_TIME -
            nrf_802154_timer_sched_granularity_get();

    return (m_delayed_timeslot_is_scheduled &&
            !nrf_802154_timer_sched_time_is_in_future(now, t0, dt));
}

/** @brief Set RSCH_PREC_STATE_APPROVED state on given precondition @p prec only if
 *         its current state is other than RSCH_PREC_STATE_IDLE.
 * 
 * @param[in]  prec    Precondition which state will be changed.
 */
static inline void prec_approve(rsch_prec_t prec)
{
    do
    {
        rsch_prec_state_t old_state = (rsch_prec_state_t) __LDREXB((uint8_t*)&m_prec_states[prec]);

        assert(old_state != RSCH_PREC_STATE_APPROVED);

        if (old_state == RSCH_PREC_STATE_IDLE)
        {
            __CLREX();
            return;
        }
    } while (__STREXB((uint8_t)RSCH_PREC_STATE_APPROVED, (uint8_t*)&m_prec_states[prec]));
}

/** @brief Set RSCH_PREC_STATE_REQUESTED state on given precondition @p prec only if
 *         its current state is RSCH_PREC_STATE_APPROVED.
 *
 * @param[in]  prec    Precondition which state will be changed.
 */
static inline void prec_deny(rsch_prec_t prec)
{
    do
    {
        rsch_prec_state_t old_state = (rsch_prec_state_t) __LDREXB((uint8_t*)&m_prec_states[prec]);

        assert(old_state != RSCH_PREC_STATE_REQUESTED);

        if (old_state != RSCH_PREC_STATE_APPROVED)
        {
            __CLREX();
            return;
        }
    } while (__STREXB((uint8_t)RSCH_PREC_STATE_REQUESTED, (uint8_t*)&m_prec_states[prec]));
}

/** @brief Set RSCH_PREC_STATE_REQUESTED state on given precondition @p prec only if
 *         its current state is RSCH_PREC_STATE_IDLE.
 *
 * @param[in]  prec    Precondition which state will be changed.
 *
 * @retval true   Precondition changed state to requested.
 * @retval false  Precondition cannot change state to requested due to invalid state.
 */
static inline bool prec_request(rsch_prec_t prec)
{
    do
    {
        rsch_prec_state_t old_state = (rsch_prec_state_t) __LDREXB((uint8_t*)&m_prec_states[prec]);

        if (old_state != RSCH_PREC_STATE_IDLE)
        {
            __CLREX();
            return false;
        }
    } while (__STREXB((uint8_t)RSCH_PREC_STATE_REQUESTED, (uint8_t*)&m_prec_states[prec]));

    return true;
}

/** @brief Set RSCH_PREC_STATE_IDLE state on given precondition @p prec.
 * 
 * @param[in]  prec    Precondition which state will be changed.
 */
static inline void prec_release(rsch_prec_t prec)
{
    assert(m_prec_states[prec] != RSCH_PREC_STATE_IDLE);
    m_prec_states[prec] = RSCH_PREC_STATE_IDLE;
    __CLREX();
}

/** @brief Request all preconditions.
 */
static inline void all_prec_request(void)
{
    if (prec_request(RSCH_PREC_HFCLK))
    {
        nrf_802154_clock_hfclk_start();
    }

    if (prec_request(RSCH_PREC_RAAL))
    {
        nrf_raal_continuous_mode_enter();
    }
}

/** @brief Release all preconditions if not needed.
 *
 * If RSCH is not in continuous mode and delayed timeslot is not expected all preconditions are
 * released.
 */
static inline void all_prec_release(void)
{
    if (!m_in_cont_mode && !any_prec_should_be_requested_for_delayed_timeslot())
    {
        prec_release(RSCH_PREC_HFCLK);
        nrf_802154_clock_hfclk_stop();

        prec_release(RSCH_PREC_RAAL);
        nrf_raal_continuous_mode_exit();
    }
}

/** @brief Check if all preconditions are met.
 *
 * @retval true   All preconditions are met.
 * @retval false  At least one precondition is not met.
 */
static inline bool all_prec_are_approved(void)
{
    for (uint32_t i = 0; i < RSCH_PREC_CNT; i++)
    {
        if (m_prec_states[i] != RSCH_PREC_STATE_APPROVED)
        {
            return false;
        }
    }

    return true;
}

/** @brief Check if all preconditions are requested or met.
 *
 * @retval true   All preconditions are requested or met.
 * @retval false  At least one precondition is idle.
 */
static inline bool all_prec_are_requested(void)
{
    for (uint32_t i = 0; i < RSCH_PREC_CNT; i++)
    {
        if (m_prec_states[i] == RSCH_PREC_STATE_IDLE)
        {
            return false;
        }
    }

    return true;
}


/** @brief Notify core if preconditions are approved or denied if current state differs from last reported.
 */
static inline void notify_core(void)
{
    bool    notify_approved;
    uint8_t temp_mon;

    do
    {
        if (!mutex_trylock())
        {
            return;
        }

        /* It is possible that preemption is not detected (m_mutex_monitor is read after acquiring mutex).
         * It is not a problem because we will call proper handler function requested by preempting context.
         * Avoiding this race would generate one additional iteration without any effect.
         */
        temp_mon        = m_mutex_monitor;
        notify_approved = all_prec_are_approved();

        if (m_in_cont_mode && (m_last_notified_approved != notify_approved))
        {
            m_last_notified_approved = notify_approved;

            if (notify_approved)
            {
                nrf_802154_rsch_prec_approved();
            }
            else
            {
                nrf_802154_rsch_prec_denied();
            }
        }

        mutex_unlock();
    } while(temp_mon != m_mutex_monitor);
}

/** Timer callback used to trigger delayed timeslot.
 *
 * @param[in]  p_context  Unused parameter.
 */
static void delayed_timeslot_start(void * p_context)
{
    (void)p_context;

    nrf_802154_log(EVENT_TRACE_ENTER, FUNCTION_RSCH_TIMER_DELAYED_START);

    m_delayed_timeslot_is_scheduled = false;

    if (all_prec_are_approved())
    {
        nrf_802154_rsch_delayed_timeslot_started();
    }
    else
    {
        nrf_802154_rsch_delayed_timeslot_failed();
    }

    nrf_802154_log(EVENT_TRACE_EXIT, FUNCTION_RSCH_TIMER_DELAYED_START);
}

/** Timer callback used to request preconditions for delayed timeslot.
 *
 * @param[in]  p_context  Unused parameter.
 */
static void delayed_timeslot_prec_request(void * p_context)
{
    (void)p_context;

    nrf_802154_log(EVENT_TRACE_ENTER, FUNCTION_RSCH_TIMER_DELAYED_PREC);

    all_prec_request();

    m_timer.t0        = m_delayed_timeslot_t0;
    m_timer.dt        = m_delayed_timeslot_dt;
    m_timer.callback  = delayed_timeslot_start;
    m_timer.p_context = NULL;

    nrf_802154_timer_sched_add(&m_timer, true);

    nrf_802154_log(EVENT_TRACE_EXIT, FUNCTION_RSCH_TIMER_DELAYED_PREC);
}

/***************************************************************************************************
 * Public API
 **************************************************************************************************/

void nrf_802154_rsch_init(void)
{
    nrf_raal_init();

    m_mutex                         = 0;
    m_last_notified_approved        = false;
    m_in_cont_mode                  = false;
    m_delayed_timeslot_is_scheduled = false;

    for (uint32_t i = 0; i < RSCH_PREC_CNT; i++)
    {
        m_prec_states[i] = RSCH_PREC_STATE_IDLE;
    }
}

void nrf_802154_rsch_uninit(void)
{
    nrf_802154_timer_sched_remove(&m_timer);

    nrf_raal_uninit();
}

void nrf_802154_rsch_continuous_mode_enter(void)
{
    nrf_802154_log(EVENT_TRACE_ENTER, FUNCTION_RSCH_CONTINUOUS_ENTER);

    m_in_cont_mode = true;
    __DMB();

    all_prec_request();
    notify_core();

    nrf_802154_log(EVENT_TRACE_EXIT, FUNCTION_RSCH_CONTINUOUS_ENTER);
}

void nrf_802154_rsch_continuous_mode_exit(void)
{
    nrf_802154_log(EVENT_TRACE_ENTER, FUNCTION_RSCH_CONTINUOUS_EXIT);

    __DMB();
    m_in_cont_mode = false;

    all_prec_release();
    notify_core();
    m_last_notified_approved = false;

    nrf_802154_log(EVENT_TRACE_EXIT, FUNCTION_RSCH_CONTINUOUS_EXIT);
}

bool nrf_802154_rsch_prec_is_approved(rsch_prec_t prec)
{
    assert(prec < RSCH_PREC_CNT);
    return m_prec_states[prec] == RSCH_PREC_STATE_APPROVED;
}

bool nrf_802154_rsch_timeslot_request(uint32_t length_us)
{
    return nrf_raal_timeslot_request(length_us);
}

bool nrf_802154_rsch_delayed_timeslot_request(uint32_t t0, uint32_t dt, uint32_t length)
{
    (void)length;

    nrf_802154_log(EVENT_TRACE_ENTER, FUNCTION_RSCH_DELAYED_TIMESLOT_REQ);

    uint32_t now    = nrf_802154_timer_sched_time_get();
    uint32_t req_dt = dt - PREC_RAMP_UP_TIME;
    bool     result;

    assert(!nrf_802154_timer_sched_is_running(&m_timer));
    assert(!m_delayed_timeslot_is_scheduled);

    if (nrf_802154_timer_sched_time_is_in_future(now, t0, req_dt))
    {
        m_delayed_timeslot_is_scheduled = true;
        m_delayed_timeslot_t0           = t0;
        m_delayed_timeslot_dt           = dt;

        m_timer.t0        = t0;
        m_timer.dt        = req_dt;
        m_timer.callback  = delayed_timeslot_prec_request;
        m_timer.p_context = NULL;

        nrf_802154_timer_sched_add(&m_timer, false);

        result = true;
    }
    else if (all_prec_are_requested() && nrf_802154_timer_sched_time_is_in_future(now, t0, dt))
    {
        m_delayed_timeslot_is_scheduled = true;
        m_delayed_timeslot_t0           = t0;
        m_delayed_timeslot_dt           = dt;

        m_timer.t0        = t0;
        m_timer.dt        = dt;
        m_timer.callback  = delayed_timeslot_start;
        m_timer.p_context = NULL;

        nrf_802154_timer_sched_add(&m_timer, true);

        result = true;
    }
    else
    {
        result = false;
    }

    nrf_802154_log(EVENT_TRACE_EXIT, FUNCTION_RSCH_DELAYED_TIMESLOT_REQ);

    return result;
}

uint32_t nrf_802154_rsch_timeslot_us_left_get(void)
{
    return nrf_raal_timeslot_us_left_get();
}

// External handlers

void nrf_raal_timeslot_started(void)
{
    nrf_802154_log(EVENT_TRACE_ENTER, FUNCTION_RSCH_TIMESLOT_STARTED);

    prec_approve(RSCH_PREC_RAAL);

    notify_core();

    nrf_802154_log(EVENT_TRACE_EXIT, FUNCTION_RSCH_TIMESLOT_STARTED);
}

void nrf_raal_timeslot_ended(void)
{
    nrf_802154_log(EVENT_TRACE_ENTER, FUNCTION_RSCH_TIMESLOT_ENDED);

    prec_deny(RSCH_PREC_RAAL);

    notify_core();

    nrf_802154_log(EVENT_TRACE_EXIT, FUNCTION_RSCH_TIMESLOT_ENDED);
}

void nrf_802154_clock_hfclk_ready(void)
{
    prec_approve(RSCH_PREC_HFCLK);

    notify_core();
}
