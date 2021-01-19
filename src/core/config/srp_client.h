/*
 *  Copyright (c) 2020, The OpenThread Authors.
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
 *   This file includes compile-time configurations for the SRP (Service Registration Protocol) Client.
 *
 */

#ifndef CONFIG_SRP_CLIENT_H_
#define CONFIG_SRP_CLIENT_H_

/**
 * @def OPENTHREAD_CONFIG_SRP_CLIENT_ENABLE
 *
 * Define to 1 to enable SRP Client support.
 *
 */
#ifndef OPENTHREAD_CONFIG_SRP_CLIENT_ENABLE
#define OPENTHREAD_CONFIG_SRP_CLIENT_ENABLE 0
#endif

/**
 * @def OPENTHREAD_CONFIG_SRP_CLIENT_DOMAIN_NAME_API_ENABLE
 *
 * Define to 1 for the SRP client implementation to provide APIs that get/set the domain name.
 *
 */
#ifndef OPENTHREAD_CONFIG_SRP_CLIENT_DOMAIN_NAME_API_ENABLE
#define OPENTHREAD_CONFIG_SRP_CLIENT_DOMAIN_NAME_API_ENABLE 0
#endif

/**
 * @def OPENTHREAD_CONFIG_SRP_CLIENT_DEFAULT_LEASE
 *
 * Specifies the default requested lease interval (in seconds). Set to two hours.
 *
 */
#ifndef OPENTHREAD_CONFIG_SRP_CLIENT_DEFAULT_LEASE
#define OPENTHREAD_CONFIG_SRP_CLIENT_DEFAULT_LEASE (2 * 60 * 60)
#endif

/**
 * @def OPENTHREAD_CONFIG_SRP_CLIENT_DEFAULT_KEY_LEASE
 *
 * Specifies the default requested key lease interval (in seconds). Set to 14 days.
 *
 */
#ifndef OPENTHREAD_CONFIG_SRP_CLIENT_DEFAULT_KEY_LEASE
#define OPENTHREAD_CONFIG_SRP_CLIENT_DEFAULT_KEY_LEASE (14 * 24 * 60 * 60)
#endif

/**
 * @def OPENTHREAD_CONFIG_SRP_CLIENT_LEASE_RENEW_GUARD_INTERVAL
 *
 * Specifies the guard interval (in seconds) for lease renew time. The guard interval determines how much earlier
 * (relative to the lease expiration time) the SRP client will send an SRP update for lease renewal.
 *
 */
#ifndef OPENTHREAD_CONFIG_SRP_CLIENT_LEASE_RENEW_GUARD_INTERVAL
#define OPENTHREAD_CONFIG_SRP_CLIENT_LEASE_RENEW_GUARD_INTERVAL 120 // two minutes in seconds
#endif

/**
 * @def OPENTHREAD_CONFIG_SRP_CLIENT_EARLY_LEASE_RENEW_FACTOR_NUMERATOR
 *
 * Specifies the numerator of early lease renewal factor.
 *
 * This value is used for opportunistic early refresh behave. When sending an SRP update, the services that are not yet
 * expired but are close, are allowed to refresh early and are included in the SRP update.
 *
 * The "early lease renewal interval" is used to determine if a service can renew early. The interval is calculated by
 * multiplying the accepted lease interval by the "early lease renewal factor" which is given as a fraction (numerator
 * and denominator).
 *
 * If the factor is set to zero (numerator=0, denominator=1), the opportunistic early refresh behavior is disabled.
 * If  denominator is set to zero (the factor is set to infinity), then all services (including previously registered
 * ones) are always included in SRP update message.
 *
 * Default value is 1/2 (i.e., services that are within half of the lease interval are allowed to refresh early).
 *
 */
#ifndef OPENTHREAD_CONFIG_SRP_CLIENT_EARLY_LEASE_RENEW_FACTOR_NUMERATOR
#define OPENTHREAD_CONFIG_SRP_CLIENT_EARLY_LEASE_RENEW_FACTOR_NUMERATOR 1
#endif

/**
 * @def OPENTHREAD_CONFIG_SRP_CLIENT_EARLY_LEASE_RENEW_FACTOR_DENOMINATOR
 *
 * Specifies the denominator of early lease renewal factor.
 *
 * Please see OPENTHREAD_CONFIG_SRP_CLIENT_EARLY_LEASE_RENEW_FACTOR_NUMERATOR for more details.
 *
 */
#ifndef OPENTHREAD_CONFIG_SRP_CLIENT_EARLY_LEASE_RENEW_FACTOR_DENOMINATOR
#define OPENTHREAD_CONFIG_SRP_CLIENT_EARLY_LEASE_RENEW_FACTOR_DENOMINATOR 2
#endif

/**
 * @def OPENTHREAD_CONFIG_SRP_CLIENT_UPDATE_TX_DELAY
 *
 * Specifies the (short) delay (in msec) after an update is required before SRP client sends the update message.
 *
 * When there is a change (e.g., a new service is added/removed) that requires an update, the SRP client will wait for
 * a short delay before preparing and sending an SRP update message to server. This allows user to provide more change
 * that are then all sent in same update message. The delay is only applied on the first change that triggers an
 * update message transmission. Subsequent changes (API calls) while waiting for the tx to start will not reset the
 * delay timer.
 *
 */
#ifndef OPENTHREAD_CONFIG_SRP_CLIENT_UPDATE_TX_DELAY
#define OPENTHREAD_CONFIG_SRP_CLIENT_UPDATE_TX_DELAY 10
#endif

/**
 * @def OPENTHREAD_CONFIG_SRP_CLIENT_MIN_RETRY_WAIT_INTERVAL
 *
 * Specifies the minimum wait interval (in msec) between SRP update message retries.
 *
 * The update message is retransmitted if there is no response from server or if server rejects the update. The wait
 * interval starts from the minimum value and is increased by the growth factor every failure up to the max value.
 *
 */
#ifndef OPENTHREAD_CONFIG_SRP_CLIENT_MIN_RETRY_WAIT_INTERVAL
#define OPENTHREAD_CONFIG_SRP_CLIENT_MIN_RETRY_WAIT_INTERVAL 1800
#endif

/**
 * @def OPENTHREAD_CONFIG_SRP_CLIENT_MAX_RETRY_WAIT_INTERVAL
 *
 * Specifies the maximum wait interval (in msec) between SRP update message retries.
 *
 */
#ifndef OPENTHREAD_CONFIG_SRP_CLIENT_MAX_RETRY_WAIT_INTERVAL
#define OPENTHREAD_CONFIG_SRP_CLIENT_MAX_RETRY_WAIT_INTERVAL (1 * 60 * 60 * 1000) // 1 hour in ms.
#endif

/**
 * @def OPENTHREAD_CONFIG_SRP_CLIENT_RETRY_WAIT_INTERVAL_JITTER
 *
 * Specifies jitter (in msec) for retry wait interval. If the current retry wait interval is smaller than the jitter
 * then the the wait interval itself is used as jitter (e.g., with jitter 500 msec and if retry interval is 300ms
 * the retry interval is then randomly selected from [0, 2*300] ms).
 *
 */
#ifndef OPENTHREAD_CONFIG_SRP_CLIENT_RETRY_WAIT_INTERVAL_JITTER
#define OPENTHREAD_CONFIG_SRP_CLIENT_RETRY_WAIT_INTERVAL_JITTER 500
#endif

/**
 * @def OPENTHREAD_CONFIG_SRP_CLIENT_RETRY_INTERVAL_GROWTH_FACTOR_NUMERATOR
 *
 * Specifies the numerator of the retry wait interval growth factor fraction. The growth factor is represented as
 * a fraction (e.g., for 1.5, we can use 15 as the numerator and 10 as the denominator).
 *
 */
#ifndef OPENTHREAD_CONFIG_SRP_CLIENT_RETRY_INTERVAL_GROWTH_FACTOR_NUMERATOR
#define OPENTHREAD_CONFIG_SRP_CLIENT_RETRY_INTERVAL_GROWTH_FACTOR_NUMERATOR 17
#endif

/**
 * @def OPENTHREAD_CONFIG_SRP_CLIENT_RETRY_INTERVAL_GROWTH_FACTOR_DENOMINATOR
 *
 * Specifies the denominator of the retry wait interval growth factor fraction.
 *
 */
#ifndef OPENTHREAD_CONFIG_SRP_CLIENT_RETRY_INTERVAL_GROWTH_FACTOR_DENOMINATOR
#define OPENTHREAD_CONFIG_SRP_CLIENT_RETRY_INTERVAL_GROWTH_FACTOR_DENOMINATOR 10
#endif

#endif // CONFIG_SRP_CLIENT_H_
