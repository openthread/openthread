/*
 *  Copyright (c) 2016-2023, The OpenThread Authors.
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
 *   This file includes compile-time configurations for Mesh Forwarder.
 */

#ifndef CONFIG_MESH_FORWARDER_H_
#define CONFIG_MESH_FORWARDER_H_

/**
 * @addtogroup config-mesh-forwarder
 *
 * @brief
 *   This module includes configuration variables for the Mesh Forwarder.
 *
 * @{
 */

#include "config/border_router.h"

/**
 * @def OPENTHREAD_CONFIG_DROP_MESSAGE_ON_FRAGMENT_TX_FAILURE
 *
 * Define as 1 for OpenThread to drop a message (and not send any remaining fragments of the message) if all transmit
 * attempts fail for a fragment of the message. For a direct transmission, a failure occurs after all MAC transmission
 * attempts for a given fragment are unsuccessful. For an indirect transmission, a failure occurs after all data poll
 * triggered transmission attempts for a given fragment fail.
 *
 * If set to zero (disabled), OpenThread will attempt to send subsequent fragments, whether or not all transmission
 * attempts fail for a given fragment.
 */
#ifndef OPENTHREAD_CONFIG_DROP_MESSAGE_ON_FRAGMENT_TX_FAILURE
#define OPENTHREAD_CONFIG_DROP_MESSAGE_ON_FRAGMENT_TX_FAILURE 1
#endif

/**
 * @def OPENTHREAD_CONFIG_6LOWPAN_REASSEMBLY_TIMEOUT
 *
 * The reassembly timeout between 6LoWPAN fragments in seconds.
 */
#ifndef OPENTHREAD_CONFIG_6LOWPAN_REASSEMBLY_TIMEOUT
#define OPENTHREAD_CONFIG_6LOWPAN_REASSEMBLY_TIMEOUT 2
#endif

/**
 * @def OPENTHREAD_CONFIG_NUM_FRAGMENT_PRIORITY_ENTRIES
 *
 * The number of fragment priority entries.
 */
#ifndef OPENTHREAD_CONFIG_NUM_FRAGMENT_PRIORITY_ENTRIES
#define OPENTHREAD_CONFIG_NUM_FRAGMENT_PRIORITY_ENTRIES 8
#endif

/**
 * @def OPENTHREAD_CONFIG_DELAY_AWARE_QUEUE_MANAGEMENT_ENABLE
 *
 * Define to 1 to enable delay-aware queue management for the send queue.
 *
 * When enabled device will monitor time-in-queue of messages in the direct tx queue and if the wait time is lager than
 * specified thresholds it may update ECN flag (if message indicates it is ECN-capable) or drop the message.
 */
#ifndef OPENTHREAD_CONFIG_DELAY_AWARE_QUEUE_MANAGEMENT_ENABLE
#define OPENTHREAD_CONFIG_DELAY_AWARE_QUEUE_MANAGEMENT_ENABLE \
    (OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_3)
#endif

/**
 * @OPENTHREAD_CONFIG_DELAY_AWARE_QUEUE_MANAGEMENT_MARK_ECN_INTERVAL
 *
 * Specifies the time-in-queue threshold interval in milliseconds to mark ECN on a message if it is ECN-capable or
 * drop the message if not ECN-capable.
 */
#ifndef OPENTHREAD_CONFIG_DELAY_AWARE_QUEUE_MANAGEMENT_MARK_ECN_INTERVAL
#define OPENTHREAD_CONFIG_DELAY_AWARE_QUEUE_MANAGEMENT_MARK_ECN_INTERVAL 500
#endif

/**
 * @OPENTHREAD_CONFIG_DELAY_AWARE_QUEUE_MANAGEMENT_DROP_MSG_INTERVAL
 *
 * Specifies the time-in-queue threshold interval in milliseconds to drop a message.
 */
#ifndef OPENTHREAD_CONFIG_DELAY_AWARE_QUEUE_MANAGEMENT_DROP_MSG_INTERVAL
#define OPENTHREAD_CONFIG_DELAY_AWARE_QUEUE_MANAGEMENT_DROP_MSG_INTERVAL 1000
#endif

/**
 * OPENTHREAD_CONFIG_DELAY_AWARE_QUEUE_MANAGEMENT_FRAG_TAG_RETAIN_TIME
 *
 * Specifies the max retain time in seconds of a mesh header fragmentation tag entry in the list.
 *
 * The entry in list is used to track whether an earlier fragment of same message was dropped by the router and if so
 * the next fragments are also dropped. The entry is removed once last fragment is processed or after the retain time
 * specified by this config parameter expires.
 */
#ifndef OPENTHREAD_CONFIG_DELAY_AWARE_QUEUE_MANAGEMENT_FRAG_TAG_RETAIN_TIME
#define OPENTHREAD_CONFIG_DELAY_AWARE_QUEUE_MANAGEMENT_FRAG_TAG_RETAIN_TIME (4 * 60) // 4 minutes
#endif

/**
 * OPENTHREAD_CONFIG_DELAY_AWARE_QUEUE_MANAGEMENT_FRAG_TAG_ENTRY_LIST_SIZE
 *
 * Specifies the number of mesh header fragmentation tag entries in the list for delay-aware queue management.
 *
 * The list is used to track whether an earlier fragment of same message was dropped by the router and if so the next
 * fragments are also dropped.
 */
#ifndef OPENTHREAD_CONFIG_DELAY_AWARE_QUEUE_MANAGEMENT_FRAG_TAG_ENTRY_LIST_SIZE
#define OPENTHREAD_CONFIG_DELAY_AWARE_QUEUE_MANAGEMENT_FRAG_TAG_ENTRY_LIST_SIZE 16
#endif

/**
 * @def OPENTHREAD_CONFIG_MAX_FRAMES_IN_DIRECT_TX_QUEUE
 *
 * Specifies the maximum number of frames in direct tx queue before new direct tx messages are dropped.
 *
 * If set to zero then the behavior is disabled, i.e., no check is performed on tx queue length.
 */
#ifndef OPENTHREAD_CONFIG_MAX_FRAMES_IN_DIRECT_TX_QUEUE
#if (OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_3)
#define OPENTHREAD_CONFIG_MAX_FRAMES_IN_DIRECT_TX_QUEUE 100
#else
#define OPENTHREAD_CONFIG_MAX_FRAMES_IN_DIRECT_TX_QUEUE 0
#endif
#endif

/**
 * @def OPENTHREAD_CONFIG_TX_QUEUE_STATISTICS_ENABLE
 *
 * Define as 1 to enable TX queue time-in-queue statistics collection feature.
 *
 * When enabled, histogram of the time-in-queue of messages in the transmit queue is collected. The time-in-queue is
 * tracked for direct transmissions only and is measured as the duration from when a message is added to the transmit
 * queue until it is passed to the MAC layer for transmission or dropped.
 *
 * The histogram data consists of number of bins, each representing a range of time-in-queue values. The bin interval
 * length is specified by the `OPENTHREAD_CONFIG_TX_QUEUE_STATISTICS_HISTOGRAM_BIN_INTERVAL` configuration, and the
 * maximum tracked interval is given by the `OPENTHREAD_CONFIG_TX_QUEUE_STATISTICS_HISTOGRAM_MAX_INTERVAL`.
 *
 * Along with histogram, the maximum observed time-in-queue is also tracked.
 */
#ifndef OPENTHREAD_CONFIG_TX_QUEUE_STATISTICS_ENABLE
#define OPENTHREAD_CONFIG_TX_QUEUE_STATISTICS_ENABLE (OPENTHREAD_CONFIG_BORDER_ROUTER_ENABLE && OPENTHREAD_FTD)
#endif

/**
 * @def OPENTHREAD_CONFIG_TX_QUEUE_STATISTICS_HISTOGRAM_MAX_INTERVAL
 *
 * Specifies the maximum time-in-queue interval in milliseconds tracked by the histogram when the TX queue time-in-queue
 * statistics collection feature is enabled.
 *
 * By default the `OPENTHREAD_CONFIG_DELAY_AWARE_QUEUE_MANAGEMENT_MARK_ECN_INTERVAL` is used which defines the
 * maximum time-in-queue interval after which a non-ECN capable message is dropped by delay-aware queue management
 * feature.
 */
#ifndef OPENTHREAD_CONFIG_TX_QUEUE_STATISTICS_HISTOGRAM_MAX_INTERVAL
#define OPENTHREAD_CONFIG_TX_QUEUE_STATISTICS_HISTOGRAM_MAX_INTERVAL \
    OPENTHREAD_CONFIG_DELAY_AWARE_QUEUE_MANAGEMENT_MARK_ECN_INTERVAL
#endif

/**
 * @def OPENTHREAD_CONFIG_TX_QUEUE_STATISTICS_HISTOGRAM_BIN_INTERVAL
 *
 * Specifies the time-in-queue histogram bin interval in milliseconds when the TX queue time-in-queue statistics
 * collection feature is enabled.
 *
 * The number of bins is calculated by dividing `OPENTHREAD_CONFIG_TX_QUEUE_STATISTICS_HISTOGRAM_MAX_INTERVAL` by
 * `OPENTHREAD_CONFIG_TX_QUEUE_STATISTICS_HISTOGRAM_BIN_INTERVAL` and rounding up to the nearest integer.
 */
#ifndef OPENTHREAD_CONFIG_TX_QUEUE_STATISTICS_HISTOGRAM_BIN_INTERVAL
#define OPENTHREAD_CONFIG_TX_QUEUE_STATISTICS_HISTOGRAM_BIN_INTERVAL 10
#endif

/**
 * @}
 */

#endif // CONFIG_MESH_FORWARDER_H_
