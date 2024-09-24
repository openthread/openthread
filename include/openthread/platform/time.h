/*
 *  Copyright (c) 2018, The OpenThread Authors.
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
 *   This file includes the platform abstraction for the time service.
 */

#ifndef OPENTHREAD_PLATFORM_TIME_H_
#define OPENTHREAD_PLATFORM_TIME_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define OT_MS_PER_S 1000    ///< Number of milliseconds per second
#define OT_US_PER_MS 1000   ///< Number of microseconds per millisecond
#define OT_US_PER_S 1000000 ///< Number of microseconds per second
#define OT_NS_PER_US 1000   ///< Number of nanoseconds per microsecond

/**
 * @addtogroup plat-time
 *
 * @brief
 *   This module includes the platform abstraction for the time service.
 *
 * @{
 */

/**
 * Get the current platform time in microseconds referenced to a continuous
 * monotonic local clock (64 bits width).
 *
 * The clock SHALL NOT wrap during the device's uptime. Implementations SHALL
 * therefore identify and compensate for internal counter overflows. The clock
 * does not have a defined epoch and it SHALL NOT introduce any continuous or
 * discontinuous adjustments (e.g. leap seconds). Implementations SHALL
 * compensate for any sleep times of the device.
 *
 * Implementations MAY choose to discipline the platform clock and compensate
 * for sleep times by any means (e.g. by combining a high precision/low power
 * RTC with a high resolution counter) as long as the exposed combined clock
 * provides continuous monotonic microsecond resolution ticks within the
 * accuracy limits announced by @ref otPlatTimeGetXtalAccuracy.
 *
 * @returns The current time in microseconds.
 */
uint64_t otPlatTimeGet(void);

/**
 * Get the current estimated worst case accuracy (maximum ± deviation from the
 * nominal frequency) of the local platform clock in units of PPM.
 *
 * @note Implementations MAY estimate this value based on current operating
 * conditions (e.g. temperature).
 *
 * In case the implementation does not estimate the current value but returns a
 * fixed value, this value MUST be the worst-case accuracy over all possible
 * foreseen operating conditions (temperature, pressure, etc) of the
 * implementation.
 *
 * @returns The current platform clock accuracy, in PPM.
 */
uint16_t otPlatTimeGetXtalAccuracy(void);

/**
 * @}
 */

#ifdef __cplusplus
} // extern "C"
#endif

#endif // OPENTHREAD_PLATFORM_TIME_H_
