/*
 *  Copyright (c) 2021, The OpenThread Authors.
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
 *   This file includes compile-time configurations for the SRP Replication Protocol (SRPL).
 *
 */

#ifndef CONFIG_SRP_REPLICATION_H_
#define CONFIG_SRP_REPLICATION_H_

/**
 * @def OPENTHREAD_CONFIG_SRP_REPLICATION_ENABLE
 *
 * Define to 1 to enable SRP Replication (SRPL) support.
 *
 */
#ifndef OPENTHREAD_CONFIG_SRP_REPLICATION_ENABLE
#define OPENTHREAD_CONFIG_SRP_REPLICATION_ENABLE 0
#endif

/**
 * @def OPENTHREAD_CONFIG_SRP_REPLICATION_MIN_DISCOVERY_INTERVAL
 *
 * Specifies the minimum domain discovery interval in milliseconds.
 *
 * When SRPL starts, it starts a DNS-SD browse for SRPL service. It picks a random interval from the range
 * `MIN_DISCOVERY_INTERVAL` to `MAX_DISCOVERY_INTERVAL` to discover SRPL peers.
 *
 */
#ifndef OPENTHREAD_CONFIG_SRP_REPLICATION_MIN_DISCOVERY_INTERVAL
#define OPENTHREAD_CONFIG_SRP_REPLICATION_MIN_DISCOVERY_INTERVAL 4000
#endif

/**
 * @def OPENTHREAD_CONFIG_SRP_REPLICATION_MAX_DISCOVERY_INTERVAL
 *
 * Specifies the maximum domain discovery interval in milliseconds.
 *
 */
#ifndef OPENTHREAD_CONFIG_SRP_REPLICATION_MAX_DISCOVERY_INTERVAL
#define OPENTHREAD_CONFIG_SRP_REPLICATION_MAX_DISCOVERY_INTERVAL 7500
#endif

/**
 * @def OPENTHREAD_CONFIG_SRP_REPLICATION_PARTNER_REMOVE_TIMEOUT
 *
 * Specifies the wait timeout in milliseconds for removing a partner.
 *
 * This timeout is used when DNS-SD browse signals that an SRPL partner is removed. We mark the partner to be
 * removed and wait for `REMOVE_TIMEOUT` interval before removing it from the list of partners and dropping any
 * connection/session to it. This ensures that if the partner is re-added within the `REMOVE_TIMEOUT`. we continue
 * with any existing connection/session and potentially avoid going through session establishment and initial sync
 * with the partner again.
 *
 */
#ifndef OPENTHREAD_CONFIG_SRP_REPLICATION_PARTNER_REMOVE_TIMEOUT
#define OPENTHREAD_CONFIG_SRP_REPLICATION_PARTNER_REMOVE_TIMEOUT 15000
#endif

/**
 * @def OPENTHREAD_CONFIG_SRP_REPLICATION_MIN_RECONNECT_INTERVAL
 *
 * Specifies the minimum reconnect interval in milliseconds.
 *
 * If there is a disconnect or failure (misbehavior) on an SRPL session with a partner, the reconnect interval
 * is used before trying to connect again or accepting connection request from the partner.
 *
 * The reconnect interval starts as `MIN_RECONNECT_INTERVAL`. On back-to-back failures the reconnect interval
 * is increased using growth factor (given by `GROWTH_FACTOR_NUMERATOR` and `GROWTH_FACTOR_DENOMINATOR`) up to
 * maximum value `MAX_RECONNECT_INTERVAL`. The reconnect interval is reset back to its minimum value after
 * successfully establishing an SRP session with the partner and successfully finishing the initial synchronization.
 *
 */
#ifndef OPENTHREAD_CONFIG_SRP_REPLICATION_MIN_RECONNECT_INTERVAL
#define OPENTHREAD_CONFIG_SRP_REPLICATION_MIN_RECONNECT_INTERVAL 3000
#endif

/**
 * @def OPENTHREAD_CONFIG_SRP_REPLICATION_MAX_RECONNECT_INTERVAL
 *
 * Specifies the maximum reconnect interval in milliseconds.
 *
 */
#ifndef OPENTHREAD_CONFIG_SRP_REPLICATION_MAX_RECONNECT_INTERVAL
#define OPENTHREAD_CONFIG_SRP_REPLICATION_MAX_RECONNECT_INTERVAL (5 * 60 * 1000) // 5 minutes
#endif

/**
 * @def OPENTHREAD_CONFIG_SRP_REPLICATION_RECONNECT_GROWTH_FACTOR_NUMERATOR
 *
 * Specifies the reconnect interval growth factor numerator.
 *
 * The growth factor is represented as a fraction (e.g., for 1.5, we can use 15 as the numerator and 10 as the
 * denominator).
 *
 */
#ifndef OPENTHREAD_CONFIG_SRP_REPLICATION_RECONNECT_GROWTH_FACTOR_NUMERATOR
#define OPENTHREAD_CONFIG_SRP_REPLICATION_RECONNECT_GROWTH_FACTOR_NUMERATOR 15
#endif

/**
 * @def OPENTHREAD_CONFIG_SRP_REPLICATION_RECONNECT_GROWTH_FACTOR_DENOMINATOR
 *
 * Specifies the reconnect interval growth factor denominator.
 *
 */
#ifndef OPENTHREAD_CONFIG_SRP_REPLICATION_RECONNECT_GROWTH_FACTOR_DENOMINATOR
#define OPENTHREAD_CONFIG_SRP_REPLICATION_RECONNECT_GROWTH_FACTOR_DENOMINATOR 10
#endif

/**
 * @def OPENTHREAD_CONFIG_SRP_REPLICATION_TEST_API_ENABLE
 *
 * Define to 1 to enable SRP Replication (SRPL) test APIs (which are used for testing SRPL behavior).
 *
 */
#ifndef OPENTHREAD_CONFIG_SRP_REPLICATION_TEST_API_ENABLE
#define OPENTHREAD_CONFIG_SRP_REPLICATION_TEST_API_ENABLE 1
#endif

#endif // CONFIG_SRP_REPLICATION_H_
