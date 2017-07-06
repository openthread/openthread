/*
 *  Copyright (c) 2016, The OpenThread Authors.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. Neither the name of the copyright holder nor the
 *     names of its contributors may be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file
 * @brief
 *  This file implements the alarm functions required for the OpenThread library.
 */

#include "precomp.h"
#include "alarm.tmh"

uint32_t 
otPlatAlarmMilliGetNow()
{
    // Return number of 'ticks'
    LARGE_INTEGER PerformanceCounter = KeQueryPerformanceCounter(NULL);

    // Multiply by 1000 ms/sec and divide by 'ticks'/sec to get ms
    return (uint32_t)(PerformanceCounter.QuadPart * 1000 / FilterPerformanceFrequency.QuadPart);
}

void 
otPlatAlarmMilliStop(
    _In_ otInstance *otCtx
    )
{
    LogVerbose(DRIVER_DEFAULT, "otPlatAlarmMilliStop");
    otLwfEventProcessingIndicateNewWaitTime(otCtxToFilter(otCtx), (ULONG)(-1));
}

void 
otPlatAlarmMilliStartAt(
    _In_ otInstance *otCtx, 
    uint32_t now, 
    uint32_t waitTime
    )
{
    UNREFERENCED_PARAMETER(now);
    LogVerbose(DRIVER_DEFAULT, "otPlatAlarmMilliStartAt %u ms", waitTime);
    otLwfEventProcessingIndicateNewWaitTime(otCtxToFilter(otCtx), waitTime);
}
