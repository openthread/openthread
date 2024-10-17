/*
 *  Copyright (c) 2024, The OpenThread Authors.
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
 *   This file includes compile-time configurations for the Wake-up Coordinator and Wake-up End Device roles.
 */

#ifndef CONFIG_WAKEUP_H_
#define CONFIG_WAKEUP_H_

/**
 * @addtogroup config-wakeup
 *
 * @brief
 *   This module includes configuration variables for the Wake-up Coordinator and Wake-up End Device roles.
 *
 * @{
 */

/**
 * @def OPENTHREAD_CONFIG_WAKEUP_COORDINATOR_ENABLE
 *
 * Define to 1 to enable the Wake-up Coordinator role that is capable of establishing
 * a link with one or more Wake-up End Devices by sending a sequence of wake-up frames.
 */
#ifndef OPENTHREAD_CONFIG_WAKEUP_COORDINATOR_ENABLE
#define OPENTHREAD_CONFIG_WAKEUP_COORDINATOR_ENABLE 0
#endif

/**
 * @def OPENTHREAD_CONFIG_WAKEUP_END_DEVICE_ENABLE
 *
 * Define to 1 to enable the Wake-up End Device role that periodically listens for wake-up
 * frames to establish a link with a Wake-up Coordinator device.
 */
#ifndef OPENTHREAD_CONFIG_WAKEUP_END_DEVICE_ENABLE
#define OPENTHREAD_CONFIG_WAKEUP_END_DEVICE_ENABLE 0
#endif

/**
 * @def OPENTHREAD_CONFIG_WED_LISTEN_INTERVAL
 *
 * The default wake-up listen interval in microseconds.
 */
#ifndef OPENTHREAD_CONFIG_WED_LISTEN_INTERVAL
#define OPENTHREAD_CONFIG_WED_LISTEN_INTERVAL 1000000
#endif

/**
 * @def OPENTHREAD_CONFIG_WED_LISTEN_DURATION
 *
 * The default wake-up listen duration in microseconds.
 */
#ifndef OPENTHREAD_CONFIG_WED_LISTEN_DURATION
#define OPENTHREAD_CONFIG_WED_LISTEN_DURATION 8000
#endif

/**
 * @def OPENTHREAD_CONFIG_WED_RECEIVE_TIME_AFTER
 *
 * Margin to be applied after the end of a wake-up listen duration to schedule the next listen interval, in units of
 * microseconds.
 */
#ifndef OPENTHREAD_CONFIG_WED_RECEIVE_TIME_AFTER
#define OPENTHREAD_CONFIG_WED_RECEIVE_TIME_AFTER 500
#endif

/**
 * @}
 */

#endif // CONFIG_WAKEUP_H_
