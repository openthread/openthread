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
#include "regmap.h"

static FTDF_PSec FTDF_lowPowerClockCycle    __attribute__((section(".retention")));
static FTDF_PSec FTDF_wakeUpLatency         __attribute__((section(".retention")));
/* Pre-calculated value for optimization. */
static FTDF_USec FTDF_lowPowerClockCycleUSec __attribute__((section(".retention")));
/* Pre-calculated value for optimization. */
static FTDF_USec FTDF_wakeUpLatencyUSec     __attribute__((section(".retention")));
/* Pre-calculated value for optimization. */
static FTDF_NrLowPowerClockCycles FTDF_csmacaWakeupThr __attribute__((section(".retention")));

#if !defined(FTDF_NO_CSL) || !defined(FTDF_NO_TSCH)
static uint32_t  FTDF_eventCurrVal          __attribute__((section(".retention")));
static uint32_t  FTDF_timeStampCurrVal      __attribute__((section(".retention")));
static uint32_t  FTDF_timeStampCurrPhaseVal __attribute__((section(".retention")));
#endif
#ifndef FTDF_NO_CSL
FTDF_Boolean     FTDF_wakeUpEnableLe        __attribute__((section(".retention")));
#endif /* FTDF_NO_CSL */
#ifndef FTDF_NO_TSCH
FTDF_Boolean     FTDF_wakeUpEnableTsch;
#endif /* FTDF_NO_TSCH */

void FTDF_setSleepAttributes(FTDF_PSec                  lowPowerClockCycle,
                             FTDF_NrLowPowerClockCycles wakeUpLatency)
{
    FTDF_lowPowerClockCycle = lowPowerClockCycle;
    FTDF_wakeUpLatency      = (uint64_t)wakeUpLatency * lowPowerClockCycle;
    FTDF_lowPowerClockCycleUSec = FTDF_lowPowerClockCycle / 1000000;
    FTDF_wakeUpLatencyUSec = FTDF_wakeUpLatency / 1000000;
    FTDF_csmacaWakeupThr = (FTDF_NrLowPowerClockCycles)
                           (((FTDF_PSec) 0xffffffff * 1000000 - FTDF_wakeUpLatency) / FTDF_lowPowerClockCycle);
}

FTDF_USec FTDF_canSleep(void)
{
#ifdef FTDF_PHY_API

    if (FTDF_txInProgress || FTDF_pib.keepPhyEnabled)
    {
        return 0;
    }

#else

    if (FTDF_reqCurrent || FTDF_pib.keepPhyEnabled)
    {
        return 0;
    }

#endif

    if (FTDF_GET_FIELD(ON_OFF_REGMAP_LMACREADY4SLEEP) == 0 ||
        FTDF_GET_FIELD(ON_OFF_REGMAP_SECBUSY) == 1)
    {
        return 0;
    }

#ifndef FTDF_NO_CSL

    if (FTDF_pib.leEnabled)
    {
        if (FTDF_txInProgress == FTDF_FALSE)
        {
            FTDF_Time curTime = FTDF_GET_FIELD(ON_OFF_REGMAP_SYMBOLTIMESNAPSHOTVAL);
            FTDF_Time delta   = curTime - FTDF_startCslSampleTime;

            // A delta larger than 0x80000000 is assumed to be negative
            // Do not return a sleep value when CSL sample time is in the past
            if (delta < 0x80000000)
            {
                return 0;
            }

            return (FTDF_startCslSampleTime - curTime) * 16;
        }
        else
        {
            return 0;
        }
    }

#endif /* FTDF_NO_CSL */

#ifndef FTDF_NO_TSCH

    if (FTDF_pib.tschEnabled)
    {
        FTDF_Time64 curTime64 = FTDF_getCurTime64();
        FTDF_Time64 delta     = curTime64 - FTDF_tschSlotTime;

        // A delta larger than 0x8000000000000000 is assumed to be negative
        // Do not return a sleep value when TSCH slot time is in the past
        if (delta < 0x8000000000000000ULL)
        {
            return 0;
        }

        FTDF_USec    sleepTime = (FTDF_USec)(FTDF_tschSlotTime - curTime64);

        FTDF_Time    pendListTime;
        FTDF_Time    curTime = FTDF_GET_FIELD(ON_OFF_REGMAP_SYMBOLTIMESNAPSHOTVAL);
        FTDF_Boolean pending = FTDF_getTxPendingTimerHead(&pendListTime);

        if (pending)
        {
            // A delta larger than 0x80000000 is assumed to be negative
            // Do not return a sleep value when pending timer time is in the past
            if (curTime - pendListTime < 0x80000000)
            {
                return 0;
            }

            FTDF_USec tmpSleepTime = (FTDF_USec)(pendListTime - curTime);

            if (tmpSleepTime < sleepTime)
            {
                sleepTime = tmpSleepTime;
            }
        }

        if (sleepTime < FTDF_TSCH_MAX_PROCESS_REQUEST_TIME + FTDF_TSCH_MAX_SCHEDULE_TIME)
        {
            return 0;
        }

        return (sleepTime - (FTDF_TSCH_MAX_PROCESS_REQUEST_TIME + FTDF_TSCH_MAX_SCHEDULE_TIME)) * 16;
    }

#endif /* FTDF_NO_TSCH */

#ifndef FTDF_LITE
    // Normal mode
    int n;

    for (n = 0; n < FTDF_NR_OF_REQ_BUFFERS; n++)
    {
        if (FTDF_txPendingList[ n ].addrMode != FTDF_NO_ADDRESS)
        {
            return 0;
        }
    }

#endif /* !FTDF_LITE */
    return 0xffffffff;
}

FTDF_Boolean FTDF_prepareForSleep(FTDF_USec sleepTime)
{
#if !defined(FTDF_NO_CSL) || !defined(FTDF_NO_TSCH)

    if (FTDF_pib.leEnabled || FTDF_pib.tschEnabled)
    {
        if (sleepTime < 2 * FTDF_lowPowerClockCycleUSec)
        {
            return FTDF_FALSE;
        }

        // Correct sleeptime with the inaccuracy of this function
        sleepTime -= 2 * FTDF_lowPowerClockCycleUSec;

        if (sleepTime < FTDF_wakeUpLatencyUSec + 500)
        {
            return FTDF_FALSE;
        }
    }

#endif

    FTDF_criticalVar();
    FTDF_enterCritical();

#if !defined(FTDF_NO_CSL) || !defined(FTDF_NO_TSCH)
    // Capture the current value of both the event generator and the timestamp generator
    // and phase on the rising edge of LP_CLK
    FTDF_SET_FIELD(ON_OFF_REGMAP_GETGENERATORVAL, 1);

#endif
    // Save current LMAC PM counters
    FTDF_lmacCounters.fcsErrorCnt += FTDF_GET_FIELD(ON_OFF_REGMAP_MACFCSERRORCOUNT);
    FTDF_lmacCounters.txStdAckCnt += FTDF_GET_FIELD(ON_OFF_REGMAP_MACTXSTDACKFRMCNT);
    FTDF_lmacCounters.rxStdAckCnt += FTDF_GET_FIELD(ON_OFF_REGMAP_MACRXSTDACKFRMOKCNT);

#if !defined(FTDF_NO_CSL) || !defined(FTDF_NO_TSCH)
    volatile uint32_t *getGeneratorValE = FTDF_GET_FIELD_ADDR(ON_OFF_REGMAP_GETGENERATORVAL_E);

    // Wait until data is ready
    while ((*getGeneratorValE & MSK_F_FTDF_ON_OFF_REGMAP_GETGENERATORVAL_E) == 0)
    { }

    FTDF_eventCurrVal          = FTDF_GET_FIELD(ON_OFF_REGMAP_EVENTCURRVAL);
    FTDF_timeStampCurrVal      = FTDF_GET_FIELD(ON_OFF_REGMAP_TIMESTAMPCURRVAL);
    FTDF_timeStampCurrPhaseVal = FTDF_GET_FIELD(ON_OFF_REGMAP_TIMESTAMPCURRPHASEVAL);

#ifdef SIMULATOR
    *getGeneratorValE &= ~MSK_F_FTDF_ON_OFF_REGMAP_GETGENERATORVAL_E;
#else
    *getGeneratorValE  = MSK_F_FTDF_ON_OFF_REGMAP_GETGENERATORVAL_E;
#endif
    uint64_t nextWakeUpThr;

    if (FTDF_pib.leEnabled || FTDF_pib.tschEnabled)
    {
        uint64_t picoSleepTime = (uint64_t)sleepTime * 1000000;
        nextWakeUpThr = (picoSleepTime - FTDF_wakeUpLatency) / FTDF_lowPowerClockCycle;
    }
    else
    {
        nextWakeUpThr = FTDF_csmacaWakeupThr;
    }

    // Set wake up threshold
    uint32_t wakeUpIntThr = FTDF_eventCurrVal + nextWakeUpThr;

    FTDF_SET_FIELD(ALWAYS_ON_REGMAP_WAKEUPINTTHR, wakeUpIntThr);
    FTDF_SET_FIELD(ALWAYS_ON_REGMAP_WAKEUPENABLE, 1);
#endif
    FTDF_exitCritical();

    return FTDF_TRUE;
}

void FTDF_wakeUp(void)
{
#ifndef FTDF_PHY_API
    FTDF_criticalVar();
    FTDF_enterCritical();

    FTDF_SET_FIELD(ALWAYS_ON_REGMAP_WAKEUPENABLE, 0);

#if !defined(FTDF_NO_CSL) || !defined(FTDF_NO_TSCH)
    // Capture the current value of both the event generator and the timestamp generator
    // and phase on the rising edge of LP_CLK
    FTDF_SET_FIELD(ON_OFF_REGMAP_GETGENERATORVAL, 1);

    volatile uint32_t *getGeneratorValE = FTDF_GET_FIELD_ADDR(ON_OFF_REGMAP_GETGENERATORVAL_E);

    // Wait until data is ready
    while ((*getGeneratorValE & MSK_F_FTDF_ON_OFF_REGMAP_GETGENERATORVAL_E) == 0)
    { }

    uint32_t eventNewCurrVal = FTDF_GET_FIELD(ON_OFF_REGMAP_EVENTCURRVAL);

#ifdef SIMULATOR
    *getGeneratorValE &= ~MSK_F_FTDF_ON_OFF_REGMAP_GETGENERATORVAL_E;
#else
    *getGeneratorValE  = MSK_F_FTDF_ON_OFF_REGMAP_GETGENERATORVAL_E;
#endif

    // Backward calculate the time slept
    FTDF_PSec sleepTime = ((uint64_t)(eventNewCurrVal - FTDF_eventCurrVal) * FTDF_lowPowerClockCycle) + FTDF_wakeUpLatency;

    // Calculate sync values
    uint64_t newSyncVals = ((uint64_t)FTDF_timeStampCurrVal << 8) | (FTDF_timeStampCurrPhaseVal & 0xff);
    newSyncVals += (sleepTime / 62500) + 1;

    uint32_t syncTimestampThr      = FTDF_eventCurrVal + (sleepTime / FTDF_lowPowerClockCycle);
    uint32_t syncTimestampVal      = (newSyncVals & 0xffffffff00) >> 8;
    uint32_t syncTimestampPhaseVal = (newSyncVals & 0xff);

    // Set values
    FTDF_SET_FIELD(ON_OFF_REGMAP_SYNCTIMESTAMPTHR, syncTimestampThr);
    FTDF_SET_FIELD(ON_OFF_REGMAP_SYNCTIMESTAMPVAL, syncTimestampVal);
    FTDF_SET_FIELD(ON_OFF_REGMAP_SYNCTIMESTAMPPHASEVAL, syncTimestampPhaseVal);
    FTDF_SET_FIELD(ON_OFF_REGMAP_SYNCTIMESTAMPENA, 1);
#endif

    FTDF_exitCritical();

#ifndef FTDF_NO_CSL
    FTDF_wakeUpEnableLe   = FTDF_pib.leEnabled;
    FTDF_pib.leEnabled    = FTDF_FALSE;
#endif /* FTDF_NO_CSL */

#ifndef FTDF_NO_TSCH
    FTDF_wakeUpEnableTsch = FTDF_pib.tschEnabled;
    FTDF_pib.tschEnabled  = FTDF_FALSE;
#endif /* FTDF_NO_TSCH */
#endif /* ! FTDF_PHY_API */
    // Init LMAC
    FTDF_initLmac();
}
