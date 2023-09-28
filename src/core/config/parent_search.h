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
 *   This file includes compile-time configurations for Parent Search.
 *
 */

#ifndef CONFIG_PARENT_SEARCH_H_
#define CONFIG_PARENT_SEARCH_H_

/**
 * @def OPENTHREAD_CONFIG_PARENT_SEARCH_ENABLE
 *
 * Define as 1 to enable periodic parent search feature.
 *
 * When this feature is enabled, an end device/child (while staying attached) periodically searches for a possible
 * better parent and switches parents if a better one is found.
 *
 * Every `OPENTHREAD_CONFIG_PARENT_SEARCH_CHECK_INTERVAL` seconds, the child checks its average RSS to its current
 * parent. Only if the average RSS is below a configured threshold (`OPENTHREAD_CONFIG_PARENT_SEARCH_RSS_THRESHOLD`)
 * does the device start a parent search process.
 *
 * The parent search mechanism depends on whether the device is an FTD child or an MTD child.
 *
 * An FTD device receives and processes MLE Advertisements from neighboring routers. It uses this information to track
 * which neighboring routers can be selected as potential new parents and to track the link quality to each.
 *
 * To select the best potential parent, the FTD device checks the list of neighboring routers and the tracked link
 * quality information. A new parent is selected only if its average RSS has a margin over the current parent RSS
 * (`OPENTHREAD_CONFIG_PARENT_SEARCH_RSS_MARGIN` config). If the attach attempt to the selected router fails (e.g., the
 * router already has the maximum number of children it can support), the FTD device ensures that the same router
 * cannot be selected again until a "reselect timeout" expires. This timeout is specified by the configuration
 * `OPENTHREAD_CONFIG_PARENT_SEARCH_RESELECT_TIMEOUT`.
 *
 * An MTD child sends an MLE Parent Request to discover possible new parents. Since this process can be power consuming
 * (the child needs to stay in RX mode to collect parent responses) and to limit its impact on battery-powered devices,
 * after a parent search is triggered on an MTD, the MTD child does not trigger another one before the specified
 * backoff interval `OPENTHREAD_CONFIG_PARENT_SEARCH_BACKOFF_INTERVAL` expires.
 *
 */
#ifndef OPENTHREAD_CONFIG_PARENT_SEARCH_ENABLE
#define OPENTHREAD_CONFIG_PARENT_SEARCH_ENABLE 0
#endif

/**
 * @def OPENTHREAD_CONFIG_PARENT_SEARCH_CHECK_INTERVAL
 *
 * Specifies the interval in seconds for a child to check the trigger condition to perform a parent search.
 *
 * Applicable only if periodic parent search feature is enabled (see `OPENTHREAD_CONFIG_PARENT_SEARCH_ENABLE`).
 *
 */
#ifndef OPENTHREAD_CONFIG_PARENT_SEARCH_CHECK_INTERVAL
#define OPENTHREAD_CONFIG_PARENT_SEARCH_CHECK_INTERVAL (9 * 60)
#endif

/**
 * @def OPENTHREAD_CONFIG_PARENT_SEARCH_BACKOFF_INTERVAL
 *
 * Specifies the backoff interval in seconds for a child to not perform a parent search after triggering one. This is
 * used when device is an MTD child.
 *
 * Applicable only if periodic parent search feature is enabled (see `OPENTHREAD_CONFIG_PARENT_SEARCH_ENABLE`).
 *
 *
 */
#ifndef OPENTHREAD_CONFIG_PARENT_SEARCH_BACKOFF_INTERVAL
#define OPENTHREAD_CONFIG_PARENT_SEARCH_BACKOFF_INTERVAL (10 * 60 * 60)
#endif

/**
 * @def OPENTHREAD_CONFIG_PARENT_SEARCH_RSS_THRESHOLD
 *
 * Specifies the RSS threshold used to trigger a parent search.
 *
 * Applicable only if periodic parent search feature is enabled (see `OPENTHREAD_CONFIG_PARENT_SEARCH_ENABLE`).
 *
 */
#ifndef OPENTHREAD_CONFIG_PARENT_SEARCH_RSS_THRESHOLD
#define OPENTHREAD_CONFIG_PARENT_SEARCH_RSS_THRESHOLD -65
#endif

/**
 * @def OPENTHREAD_CONFIG_PARENT_SEARCH_RESELECT_TIMEOUT
 *
 * Specifies parent reselect timeout duration in seconds used on FTD child devices. When an attach attempt to a
 * neighboring router selected as a potential new parent fails, the same router cannot be selected again until this
 * timeout expires.
 *
 * Applicable only if periodic parent search feature is enabled (see `OPENTHREAD_CONFIG_PARENT_SEARCH_ENABLE`).
 *
 */
#ifndef OPENTHREAD_CONFIG_PARENT_SEARCH_RESELECT_TIMEOUT
#define OPENTHREAD_CONFIG_PARENT_SEARCH_RESELECT_TIMEOUT (90 * 60)
#endif

/**
 * @def OPENTHREAD_CONFIG_PARENT_SEARCH_RSS_MARGIN
 *
 * Specifies the RSS margin over the current parent RSS for allowing selection of a neighboring router as a potential
 * new parent to attach to. Used on FTD child devices.
 *
 */
#ifndef OPENTHREAD_CONFIG_PARENT_SEARCH_RSS_MARGIN
#define OPENTHREAD_CONFIG_PARENT_SEARCH_RSS_MARGIN 7
#endif

#endif // CONFIG_PARENT_SEARCH_H_
