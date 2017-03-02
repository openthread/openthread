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
 *   This file includes the platform abstraction for the microsecond alarm service.
 */

#ifndef USEC_ALARM_H_
#define USEC_ALARM_H_

#include <stdint.h>

#include "openthread/types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup usec-alarm Microsecond alarm
 * @ingroup alarm
 *
 * @brief
 *   This module includes the platform abstraction for the microsecond alarm service.
 *
 * @{
 *
 */

typedef struct
{
    uint32_t mMs;  ///< Time in milliseconds.
    uint16_t mUs;  ///< Time fraction in microseconds.
} otPlatUsecAlarmTime;

typedef void (*otPlatUsecAlarmHandler)(void *aContext);

/**
 * Set the alarm to fire at @p aDt milliseconds and microseconds after @p aT0.
 *
 * @param[in]  aInstance  The OpenThread instance structure.
 * @param[in]  aT0        The reference time.
 * @param[in]  aDt        The time delay in milliseconds and microseconds from @p aT0.
 * @param[in]  aHandler   A pointer to a function that is called when the timer expires.
 * @param[in]  aContext   A pointer to arbitrary context information.
 */
void otPlatUsecAlarmStartAt(otInstance *aInstance,
                            const otPlatUsecAlarmTime *aT0,
                            const otPlatUsecAlarmTime *aDt,
                            otPlatUsecAlarmHandler aHandler,
                            void *aContext);

/**
 * Stop the alarm.
 *
 * @param[in] aInstance  The OpenThread instance structure.
 */
void otPlatUsecAlarmStop(otInstance *aInstance);

/**
 * Get the current time.
 *
 * @param[out]  aNow  The current time in milliseconds and microseconds.
 */
void otPlatUsecAlarmGetNow(otPlatUsecAlarmTime *aNow);

/**
 * @}
 *
 */

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // USEC_ALARM_H_
