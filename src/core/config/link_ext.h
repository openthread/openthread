/*
 *  Copyright (c) 2026, The OpenThread Authors.
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
 *   This file includes compile-time configurations for the Link Extension service.
 */

#ifndef OT_CORE_CONFIG_LINK_EXT_H_
#define OT_CORE_CONFIG_LINK_EXT_H_

/**
 * @addtogroup config-link-ext
 *
 * @brief
 *   This module includes configuration variables for the Link Extension service.
 *
 * @{
 */

/**
 * @def OPENTHREAD_CONFIG_LINK_EXT_ENABLE
 *
 * Define to 1 to enable the Link Extension service (e.g., burst transmission and periodic receive schedules).
 *
 * Only supported in RCP builds (`OPENTHREAD_RADIO`). Requires `OPENTHREAD_CONFIG_PLATFORM_USEC_TIMER_ENABLE` and
 * target time TX support (e.g. CSL transmitter or software TX timing).
 */
#ifndef OPENTHREAD_CONFIG_LINK_EXT_ENABLE
#define OPENTHREAD_CONFIG_LINK_EXT_ENABLE 0
#endif

/**
 * @def OPENTHREAD_CONFIG_LINK_EXT_BURST_TX_AHEAD_US
 *
 * Time (in microseconds) before the target TX time of a timed burst tick at which the tick is handed to the radio.
 */
#ifndef OPENTHREAD_CONFIG_LINK_EXT_BURST_TX_AHEAD_US
#define OPENTHREAD_CONFIG_LINK_EXT_BURST_TX_AHEAD_US 1000
#endif

/**
 * @def OPENTHREAD_CONFIG_LINK_EXT_PERIODIC_RX_MAX_SCHEDULES
 *
 * The maximum number of periodic receive schedules (see `otLinkExtSetPeriodicRxSchedules()`).
 *
 * Zero disables periodic receive schedules. Applicable only when `OPENTHREAD_CONFIG_LINK_EXT_ENABLE` is enabled.
 */
#ifndef OPENTHREAD_CONFIG_LINK_EXT_PERIODIC_RX_MAX_SCHEDULES
#define OPENTHREAD_CONFIG_LINK_EXT_PERIODIC_RX_MAX_SCHEDULES 3
#endif

/**
 * @def OPENTHREAD_CONFIG_LINK_EXT_PERIODIC_RX_CHANNEL_SWITCH_GUARD_US
 *
 * The channel switch guard time (in microseconds) used when arbitrating periodic receive schedules.
 *
 * When a higher-priority receive window preempts an ongoing lower-priority window on a different channel, the
 * higher-priority window is started this much earlier so that the radio is listening on the new channel no later
 * than the window start. The guard time is taken from the lower-priority window. A truncated window shorter than this
 * guard time plus the SHR duration (160 microseconds, the time needed to detect a frame) is dropped.
 *
 * Applicable only when `OPENTHREAD_CONFIG_LINK_EXT_PERIODIC_RX_MAX_SCHEDULES` is non-zero.
 */
#ifndef OPENTHREAD_CONFIG_LINK_EXT_PERIODIC_RX_CHANNEL_SWITCH_GUARD_US
#define OPENTHREAD_CONFIG_LINK_EXT_PERIODIC_RX_CHANNEL_SWITCH_GUARD_US 40
#endif

/**
 * @def OPENTHREAD_CONFIG_LINK_EXT_MAC_KEY_TABLE_MAX_ENTRIES
 *
 * The maximum number of entries in the MAC key table of the MAC frame filter (see `otLinkExtSetMacKeyTable()`).
 *
 * Zero disables the key table. Applicable only when `OPENTHREAD_CONFIG_LINK_EXT_ENABLE` is enabled.
 */
#ifndef OPENTHREAD_CONFIG_LINK_EXT_MAC_KEY_TABLE_MAX_ENTRIES
#define OPENTHREAD_CONFIG_LINK_EXT_MAC_KEY_TABLE_MAX_ENTRIES 8
#endif

/**
 * @def OPENTHREAD_CONFIG_LINK_EXT_MAC_NEIGHBOR_TABLE_MAX_ENTRIES
 *
 * The maximum number of entries in the MAC neighbor table of the MAC frame filter (see
 * `otLinkExtSetMacNeighborTable()`).
 *
 * Applicable only when `OPENTHREAD_CONFIG_LINK_EXT_MAC_KEY_TABLE_MAX_ENTRIES` is non-zero, in which case it MUST be
 * non-zero.
 */
#ifndef OPENTHREAD_CONFIG_LINK_EXT_MAC_NEIGHBOR_TABLE_MAX_ENTRIES
#define OPENTHREAD_CONFIG_LINK_EXT_MAC_NEIGHBOR_TABLE_MAX_ENTRIES 8
#endif

/**
 * @}
 */

#endif // OT_CORE_CONFIG_LINK_EXT_H_
