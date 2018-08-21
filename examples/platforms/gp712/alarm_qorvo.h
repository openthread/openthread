/*
 *  Copyright (c) 2016-2017, The OpenThread Authors.
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
 *   This file includes the declarations of the Alarm functions from the Qorvo library.
 *
 */

#ifndef ALARM_QORVO_H_
#define ALARM_QORVO_H_

#include <stdint.h>
#include <sys/time.h>
#include <unistd.h>

typedef void (*qorvoAlarmCallback_t)( void* );

/**
 * This function initializes the alarm service used by OpenThread.
 *
 */
void qorvoAlarmInit(void);

/**
 * This function retrieves the time remaining until the alarm fires.
 *
 * @param[out]  aTimeval  A pointer to the timeval struct.
 *
 */
void qorvoAlarmUpdateTimeout(struct timeval *aTimeout);

/**
 * This function performs alarm driver processing.
 *
 */
void qorvoAlarmProcess(void);

/**
 * This function retrieves the current time.
 *
 * @param[out]  The current time in ms.
 *
 */
uint32_t qorvoAlarmGetTimeMs(void);

/**
 * This function schedules a callback after a relative amount of time.
 *
 * @param[in]  rel_time  The relative time in ms.
 * @param[in]  callback  A callback function which will be called.
 * @param[in]  arg       A context pointer which will be passed as an argument to the callback.
 *
 */
void qorvoAlarmScheduleEventArg(uint32_t rel_time, qorvoAlarmCallback_t callback, void* arg);

/**
 * This function unschedules the callback.
 *
 * @param[in]  callback  A callback function which will be removed from the list.
 * @param[in]  arg       A context pointer which will be passed as an argument to the callback.
 *
 */
bool qorvoAlarmUnScheduleEventArg(qorvoAlarmCallback_t callback, void* arg);

#endif  // ALARM_QORVO_H_
