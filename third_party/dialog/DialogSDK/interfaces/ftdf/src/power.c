/**
 ****************************************************************************************
 *
 * @file power.c
 *
 * @brief FTDF power on/off functions
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
 ****************************************************************************************
 */

#include <stdlib.h>
#include <ftdf.h>
#include "internal.h"
#include "sdk_defs.h"

#ifdef CONFIG_USE_FTDF
static ftdf_psec_t ftdf_low_power_clock_cycle                  __attribute__ ((section(".retention")));
static ftdf_psec_t ftdf_wake_up_latency                        __attribute__ ((section(".retention")));
/* Pre-calculated value for optimization. */
static ftdf_usec_t ftdf_low_power_clock_cycle_u_sec            __attribute__ ((section(".retention")));
/* Pre-calculated value for optimization. */
static ftdf_usec_t ftdf_wake_up_latency_u_sec                  __attribute__ ((section(".retention")));
/* Pre-calculated value for optimization. */
static ftdf_nr_low_power_clock_cycles_t ftdf_csmaca_wakeup_thr __attribute__ ((section(".retention")));

#if !defined(FTDF_NO_CSL) || !defined(FTDF_NO_TSCH)
static uint32_t ftdf_event_curr_val                            __attribute__ ((section(".retention")));
static uint32_t ftdf_time_stamp_curr_val                       __attribute__ ((section(".retention")));
static uint32_t ftdf_time_stamp_curr_phase_val                 __attribute__ ((section(".retention")));
#endif
#ifndef FTDF_NO_CSL
ftdf_boolean_t ftdf_wake_up_enable_le                          __attribute__ ((section(".retention")));
#endif /* FTDF_NO_CSL */
#ifndef FTDF_NO_TSCH
ftdf_boolean_t ftdf_wake_up_enable_tsch;
#endif /* FTDF_NO_TSCH */

void ftdf_set_sleep_attributes(ftdf_psec_t low_power_clock_cycle,
                               ftdf_nr_low_power_clock_cycles_t wake_up_latency)
{
        ftdf_low_power_clock_cycle = low_power_clock_cycle;
        ftdf_wake_up_latency = (ftdf_psec_t)wake_up_latency * low_power_clock_cycle;
        ftdf_low_power_clock_cycle_u_sec = ftdf_low_power_clock_cycle / 1000000;
        ftdf_wake_up_latency_u_sec = ftdf_wake_up_latency / 1000000;
        ftdf_csmaca_wakeup_thr = (ftdf_nr_low_power_clock_cycles_t)
            (((ftdf_psec_t) 0xffffffff * 1000000 - ftdf_wake_up_latency) / ftdf_low_power_clock_cycle);
}

ftdf_usec_t ftdf_can_sleep(void)
{
#ifdef FTDF_PHY_API
        if (ftdf_tx_in_progress || ftdf_pib.keep_phy_enabled) {
                return 0;
        }
#else
#if FTDF_USE_SLEEP_DURING_BACKOFF
        if (ftdf_pib.keep_phy_enabled)
#else
        if (ftdf_req_current || ftdf_pib.keep_phy_enabled)
#endif /* FTDF_USE_SLEEP_DURING_BACKOFF */
        {
                return 0;
        }
#endif
#if FTDF_USE_SLEEP_DURING_BACKOFF
        if (REG_GETF(FTDF, FTDF_SECURITY_STATUS_REG, SECBUSY) == 1)
#else /* FTDF_USE_SLEEP_DURING_BACKOFF */
        if ((REG_GETF(FTDF, FTDF_LMAC_CONTROL_STATUS_REG, LMACREADY4SLEEP) == 0) ||
                (REG_GETF(FTDF, FTDF_SECURITY_STATUS_REG, SECBUSY) == 1))
#endif /* FTDF_USE_SLEEP_DURING_BACKOFF */
        {

                return 0;
        }

#ifndef FTDF_NO_CSL
        if (ftdf_pib.le_enabled) {
#if FTDF_USE_SLEEP_DURING_BACKOFF
                /* Abort sleep when LMAC is busy. */
                if (REG_GETF(FTDF, FTDF_LMAC_CONTROL_STATUS_REG, LMACREADY4SLEEP) == 0) {
                        return 0;
                }
#endif /* FTDF_USE_SLEEP_DURING_BACKOFF */
                if (ftdf_tx_in_progress == FTDF_FALSE) {
                        ftdf_time_t cur_time = REG_GETF(FTDF, FTDF_SYMBOLTIMESNAPSHOTVAL_REG,
                                                        SYMBOLTIMESNAPSHOTVAL);
                        ftdf_time_t delta = cur_time - ftdf_start_csl_sample_time;

                        /* A delta larger than 0x80000000 is assumed to be negative */
                        /* Do not return a sleep value when CSL sample time is in the past */
                        if (delta < 0x80000000) {
                                return 0;
                        }

                        return (ftdf_start_csl_sample_time - cur_time) * 16;
                } else {
                        return 0;
                }
        }
#endif /* FTDF_NO_CSL */

#ifndef FTDF_NO_TSCH
        if (ftdf_pib.tsch_enabled) {
#if FTDF_USE_SLEEP_DURING_BACKOFF
                /* Abort sleep when LMAC is busy. */
                if (REG_GETF(FTDF, FTDF_LMAC_CONTROL_STATUS_REG, LMACREADY4SLEEP) == 0) {
                        return 0;
                }
#endif
                ftdf_time64_t curTime64 = ftdf_get_cur_time64();
                ftdf_time64_t delta = curTime64 - ftdf_tsch_slot_time;

                /* A delta larger than 0x8000000000000000 is assumed to be negative */
                /* Do not return a sleep value when TSCH slot time is in the past */
                if (delta < 0x8000000000000000ULL) {
                        return 0;
                }

                ftdf_usec_t sleep_time = (ftdf_usec_t)(ftdf_tsch_slot_time - curTime64);

                ftdf_time_t pend_list_time;
                ftdf_time_t curTime = REG_GETF(FTDF, FTDF_SYMBOLTIMESNAPSHOTVAL_REG, SYMBOLTIMESNAPSHOTVAL);
                ftdf_boolean_t pending = ftdf_get_tx_pending_timer_head(&pend_list_time);

                if (pending) {
                        /* A delta larger than 0x80000000 is assumed to be negative */
                        /* Do not return a sleep value when pending timer time is in the past */
                        if (curTime - pend_list_time < 0x80000000) {
                                return 0;
                        }

                        ftdf_usec_t tmp_sleep_time = (ftdf_usec_t)(pend_list_time - curTime);

                        if (tmp_sleep_time < sleep_time) {
                                sleep_time = tmp_sleep_time;
                        }
                }

                if (sleep_time < FTDF_TSCH_MAX_PROCESS_REQUEST_TIME + FTDF_TSCH_MAX_SCHEDULE_TIME) {
                        return 0;
                }

                return (sleep_time - (FTDF_TSCH_MAX_PROCESS_REQUEST_TIME + FTDF_TSCH_MAX_SCHEDULE_TIME)) * 16;
        }
#endif /* FTDF_NO_TSCH */

#ifndef FTDF_LITE
        /* Normal mode */
        int n;

        for (n = 0; n < FTDF_NR_OF_REQ_BUFFERS; n++) {
                if (ftdf_tx_pending_list[n].addr_mode != FTDF_NO_ADDRESS) {
                        return 0;
                }
        }
#endif /* !FTDF_LITE */
#if FTDF_USE_SLEEP_DURING_BACKOFF
        return ftdf_sdb_get_sleep_time();
#else /* FTDF_USE_SLEEP_DURING_BACKOFF */
        return 0xffffffff;
#endif /* FTDF_USE_SLEEP_DURING_BACKOFF */
}

ftdf_boolean_t ftdf_prepare_for_sleep(ftdf_usec_t sleep_time)
{
#if !defined(FTDF_NO_CSL) || !defined(FTDF_NO_TSCH)
        if (ftdf_pib.le_enabled || ftdf_pib.tsch_enabled) {
                if (sleep_time < (2 * ftdf_low_power_clock_cycle_u_sec)) {
                        return FTDF_FALSE;
                }

                // Correct sleeptime with the inaccuracy of this function
                sleep_time -= (2 * ftdf_low_power_clock_cycle_u_sec);

                if (sleep_time < (ftdf_wake_up_latency_u_sec + 500)) {
                        return FTDF_FALSE;
                }
        }

#endif

        ftdf_critical_var();
        ftdf_enter_critical();

#if !defined(FTDF_NO_CSL) || !defined(FTDF_NO_TSCH)
        /* Capture the current value of both the event generator and the timestamp generator
           and phase on the rising edge of LP_CLK */
        REG_SETF(FTDF, FTDF_LMAC_CONTROL_OS_REG, GETGENERATORVAL, 1);

#endif
        /* Save current LMAC PM counters */
        ftdf_lmac_counters.fcs_error_cnt += REG_GETF(FTDF, FTDF_MACFCSERRORCOUNT_REG, MACFCSERRORCOUNT);
        ftdf_lmac_counters.tx_std_ack_cnt += REG_GETF(FTDF, FTDF_MACTXSTDACKFRMCNT_REG, MACTXSTDACKFRMCNT);
        ftdf_lmac_counters.rx_std_ack_cnt += REG_GETF(FTDF, FTDF_MACRXSTDACKFRMOKCNT_REG, MACRXSTDACKFRMOKCNT);

#if !defined(FTDF_NO_CSL) || !defined(FTDF_NO_TSCH)

        /* Wait until data is ready */
        while (REG_GETF(FTDF, FTDF_LMAC_CONTROL_DELTA_REG, GETGENERATORVAL_E) == 0) {}

        ftdf_event_curr_val = REG_GETF(FTDF, FTDF_EVENTCURRVAL_REG, EVENTCURRVAL);
        ftdf_time_stamp_curr_val = REG_GETF(FTDF, FTDF_TIMESTAMPCURRVAL_REG, TIMESTAMPCURRVAL);
        ftdf_time_stamp_curr_phase_val = REG_GETF(FTDF, FTDF_TIMESTAMPCURRPHASEVAL_REG, TIMESTAMPCURRPHASEVAL);

#ifdef SIMULATOR
        REG_CLR_FIELD(FTDF, FTDF_LMAC_CONTROL_DELTA_REG, GETGENERATORVAL_E, FTDF->FTDF_LMAC_CONTROL_DELTA_REG);
#else
        FTDF->FTDF_LMAC_CONTROL_DELTA_REG = REG_MSK(FTDF, FTDF_LMAC_CONTROL_DELTA_REG, GETGENERATORVAL_E);
#endif
        uint64_t next_wake_up_thr;
#if FTDF_USE_SLEEP_DURING_BACKOFF
        uint64_t pico_sleep_time = (uint64_t)sleep_time * 1000000;
        next_wake_up_thr = ( pico_sleep_time - ftdf_wake_up_latency ) / ftdf_low_power_clock_cycle;
#else /* FTDF_USE_SLEEP_DURING_BACKOFF */
        if (ftdf_pib.le_enabled || ftdf_pib.tsch_enabled) {
                uint64_t picoSleepTime = (uint64_t)sleep_time * 1000000;
                next_wake_up_thr = (picoSleepTime - ftdf_wake_up_latency) / ftdf_low_power_clock_cycle;
        } else {
                next_wake_up_thr = ftdf_csmaca_wakeup_thr;
        }
#endif /* FTDF_USE_SLEEP_DURING_BACKOFF */

        /* Set wake up threshold */
        uint32_t wake_up_int_thr = ftdf_event_curr_val + next_wake_up_thr;
        /* Note that for IC revs other than A the size of WAKEUPINTTHR is 25 bits. */
        REG_SETF(FTDF, FTDF_WAKEUP_CONTROL_REG, WAKEUPINTTHR, wake_up_int_thr);
        REG_SETF(FTDF, FTDF_WAKEUP_CONTROL_REG, WAKEUPENABLE, 1);
#endif
        ftdf_exit_critical();

        return FTDF_TRUE;
}

void ftdf_wakeup(void)
{
#ifndef FTDF_PHY_API
        ftdf_critical_var();
        ftdf_enter_critical();

        REG_SETF(FTDF, FTDF_WAKEUP_CONTROL_REG, WAKEUPENABLE, 0);

#if !defined(FTDF_NO_CSL) || !defined(FTDF_NO_TSCH)
        /* Capture the current value of both the event generator and the timestamp generator
           and phase on the rising edge of LP_CLK */
        REG_SETF(FTDF, FTDF_LMAC_CONTROL_OS_REG, GETGENERATORVAL, 1);

        /* Wait until data is ready */
        while (REG_GETF(FTDF, FTDF_LMAC_CONTROL_DELTA_REG, GETGENERATORVAL_E) == 0) {}

        uint32_t event_new_curr_val = REG_GETF(FTDF, FTDF_EVENTCURRVAL_REG, EVENTCURRVAL);

#ifdef SIMULATOR
        REG_CLR_FIELD(FTDF, FTDF_LMAC_CONTROL_DELTA_REG, GETGENERATORVAL_E);
#else
        FTDF->FTDF_LMAC_CONTROL_DELTA_REG = REG_MSK(FTDF, FTDF_LMAC_CONTROL_DELTA_REG, GETGENERATORVAL_E);
#endif

        /* Backward calculate the time slept */
        /* ftdf_psec_t sleep_time = ((uint64_t)(event_new_curr_val - ftdf_event_curr_val) *
                ftdf_low_power_clock_cycle) + ftdf_wake_up_latency; */

        ftdf_psec_t sleep_time;
        if (event_new_curr_val >= ftdf_event_curr_val) { /* Check for wraps. */
                sleep_time = (event_new_curr_val - ftdf_event_curr_val) * ftdf_low_power_clock_cycle  +
                        ftdf_wake_up_latency;
        } else {
                sleep_time = (event_new_curr_val +
                        (REG_MSK(FTDF, FTDF_EVENTCURRVAL_REG, EVENTCURRVAL) - ftdf_event_curr_val)) *
                                ftdf_low_power_clock_cycle  + ftdf_wake_up_latency;
        }
        /* Calculate sync values */
        uint64_t new_sync_vals = ((uint64_t)ftdf_time_stamp_curr_val << 8) |
            (ftdf_time_stamp_curr_phase_val & 0xff);

        new_sync_vals += (sleep_time / 62500) + 1;

        uint32_t sync_timestamp_thr = ftdf_event_curr_val + (sleep_time / ftdf_low_power_clock_cycle);
        uint32_t sync_timestamp_val = (new_sync_vals & 0xffffffff00) >> 8;
        uint32_t sync_timestamp_phase_val = (new_sync_vals & 0xff);

        /* Set values */
        REG_SETF(FTDF, FTDF_SYNCTIMESTAMPTHR_REG, SYNCTIMESTAMPTHR, sync_timestamp_thr);
        REG_SETF(FTDF, FTDF_SYNCTIMESTAMPVAL_REG, SYNCTIMESTAMPVAL, sync_timestamp_val);
        REG_SETF(FTDF, FTDF_SYNCTIMESTAMPPHASEVAL_REG, SYNCTIMESTAMPPHASEVAL, sync_timestamp_phase_val);
        REG_SETF(FTDF, FTDF_TIMER_CONTROL_1_REG, SYNCTIMESTAMPENA, 1);
#endif

        ftdf_exit_critical();

#ifndef FTDF_NO_CSL
        ftdf_wake_up_enable_le = ftdf_pib.le_enabled;
        ftdf_pib.le_enabled = FTDF_FALSE;
#endif /* FTDF_NO_CSL */

#ifndef FTDF_NO_TSCH
        ftdf_wake_up_enable_tsch = ftdf_pib.tsch_enabled;
        ftdf_pib.tsch_enabled = FTDF_FALSE;
#endif /* FTDF_NO_TSCH */
#endif /* ! FTDF_PHY_API */
        /* Init LMAC */
        ftdf_init_lmac();
}
#endif /* CONFIG_USE_FTDF */
