/*
 *  Copyright (c) 2019, The OpenThread Authors.
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
 *   This file includes the platform abstraction for the tick alarm service.
 */

#ifndef OPENTHREAD_PLATFORM_BLE_ALARM_H_
#define OPENTHREAD_PLATFORM_BLE_ALARM_H_

#include <stdint.h>

#include <openthread/instance.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup plat-cordio-alarm
 *
 * @brief
 *   This module includes the platform abstraction for the Cordio BLE stack alarm service.
 *
 * @{
 *
 */

/**
 * Set the alarm for Cordio BLE stack to fire at @p aDt ticks after @p aT0.
 *
 * @note The clock should increment at the rate OPENTHREAD_CONFIG_BLE_MS_PER_TICKS (wrapping as appropriate).
 *
 * @param[in] aInstance  The OpenThread instance structure.
 * @param[in] aT0        The reference time.
 * @param[in] aDt        The time delay in ticks from @p aT0.
 */
void otCordioPlatAlarmTickStartAt(otInstance *aInstance, uint32_t aT0, uint32_t aDt);

/**
 * Stop the alarm for the BLE stack.
 *
 * @param[in] aInstance  The OpenThread instance structure.
 */
void otCordioPlatAlarmTickStop(otInstance *aInstance);

/**
 * Signal that the alarm for the BLE stack has fired.
 *
 * @param[in] aInstance  The OpenThread instance structure.
 */
uint32_t otCordioPlatAlarmTickGetNow(void);

/**
 * Signal BLE stack that the alarm has fired.
 *
 * @param[in] aInstance  The OpenThread instance structure.
 */
extern void otCordioPlatAlarmTickFired(otInstance *aInstance);

/*
 * Enable BLE alarm timer interrupt.
 *
 */
void otCordioPlatAlarmEnableInterrupt(void);

/*
 * Disable BLE alarm timer interrupt.
 *
 */
void otCordioPlatAlarmDisableInterrupt(void);

/**
 * @}
 *
 */

#ifdef __cplusplus
} // extern "C"
#endif

#endif // OPENTHREAD_PLATFORM_BLE_ALARM_H_
