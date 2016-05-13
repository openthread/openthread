/*
 *  Copyright (c) 2016, Nest Labs, Inc.
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
 *   This file includes the platform abstraction for the debug log service.
 */

#ifndef LOGGING_H_
#define LOGGING_H_

#include <stdint.h>

#include <openthread-core-config.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup logging Logging
 * @ingroup platform
 *
 * @brief
 *   This module includes the platform abstraction for the debug log service.
 *
 * @{
 *
 */

#define OPENTHREAD_LOG_LEVEL_NONE  0
#define OPENTHREAD_LOG_LEVEL_CRIT  1
#define OPENTHREAD_LOG_LEVEL_WARN  2
#define OPENTHREAD_LOG_LEVEL_INFO  3
#define OPENTHREAD_LOG_LEVEL_DEBG  4

/**
 * This enum represents different log levels.
 *
 */
typedef enum otLogLevel
{
    kLogLevelNone      = OPENTHREAD_LOG_LEVEL_NONE,  ///< None
    kLogLevelCrit      = OPENTHREAD_LOG_LEVEL_CRIT,  ///< Critical
    kLogLevelWarn      = OPENTHREAD_LOG_LEVEL_WARN,  ///< Warning
    kLogLevelInfo      = OPENTHREAD_LOG_LEVEL_INFO,  ///< Info
    kLogLevelDebg      = OPENTHREAD_LOG_LEVEL_DEBG,  ///< Debug
} otLogLevel;

/**
 * This enum represents log regions.
 *
 */
typedef enum otLogRegion
{
    kLogRegionApi      = 1,  ///< OpenThread API
    kLogRegionMle      = 2,  ///< MLE
    kLogRegionArp      = 3,  ///< EID-to-RLOC mapping.
    kLogRegionNetData  = 4,  ///< Network Data
    kLogRegionIcmp     = 5,  ///< ICMPv6
    kLogRegionIp6      = 6,  ///< IPv6
    kLogRegionMac      = 7,  ///< IEEE 802.15.4 MAC
    kLogRegionMem      = 8,  ///< Memory
} otLogRegion;

/**
 * This function outputs logs.
 *
 * @param[in]  aLogLevel   The log level.
 * @param[in]  aLogRegion  The log region.
 * @param[in]  aFormat     A pointer to the format string.
 * @param[in]  ...         Arguments for the format specification.
 *
 */
void otPlatLog(otLogLevel aLogLevel, otLogRegion aLogRegion, const char *aFormat, ...);

/**
 * @def otLogCrit
 *
 * Logging at log level critical.
 *
 * @param[in]  aRegion  The log region.
 * @param[in]  aFormat  A pointer to the format string.
 * @param[in]  ...      Arguments for the format specification.
 *
 */
#if OPENTHREAD_CONFIG_LOG_LEVEL >= OPENTHREAD_LOG_LEVEL_CRIT
#define otLogCrit(aRegion, aFormat, ...)  otPlatLog(kLogLevelCrit, aRegion, aFormat, ## __VA_ARGS__)
#else
#define otLogCrit(aRegion, aFormat, ...)
#endif

/**
 * @def otLogWarn
 *
 * Logging at log level warning.
 *
 * @param[in]  aRegion  The log region.
 * @param[in]  aFormat  A pointer to the format string.
 * @param[in]  ...      Arguments for the format specification.
 *
 */
#if OPENTHREAD_CONFIG_LOG_LEVEL >= OPENTHREAD_LOG_LEVEL_WARN
#define otLogWarn(aRegion, aFormat, ...)  otPlatLog(kLogLevelWarn, aRegion, aFormat, ## __VA_ARGS__)
#else
#define otLogWarn(aRegion, aFormat, ...)
#endif

/**
 * @def otLogInfo
 *
 * Logging at log level info.
 *
 * @param[in]  aRegion  The log region.
 * @param[in]  aFormat  A pointer to the format string.
 * @param[in]  ...      Arguments for the format specification.
 *
 */
#if OPENTHREAD_CONFIG_LOG_LEVEL >= OPENTHREAD_LOG_LEVEL_INFO
#define otLogInfo(aRegion, aFormat, ...)  otPlatLog(kLogLevelInfo, aRegion, aFormat, ## __VA_ARGS__)
#else
#define otLogInfo(aRegion, aFormat, ...)
#endif

/**
 * @def otLogDebg
 *
 * Logging at log level debug.
 *
 * @param[in]  aRegion  The log region.
 * @param[in]  aFormat  A pointer to the format string.
 * @param[in]  ...      Arguments for the format specification.
 *
 */
#if OPENTHREAD_CONFIG_LOG_LEVEL >= OPENTHREAD_LOG_LEVEL_DEBG
#define otLogDebg(aRegion, aFormat, ...)  otPlatLog(kLogLevelDebg, aRegion, aFormat, ## __VA_ARGS__)
#else
#define otLogDebg(aRegion, aFormat, ...)
#endif

/**
 * @def otLogCritApi
 *
 * This method generates a log with level critical for the API region.
 *
 * @param[in]  aFormat  A pointer to the format string.
 * @param[in]  ...      Arguments for the format specification.
 *
 */

/**
 * @def otLogWarnApi
 *
 * This method generates a log with level warning for the API region.
 *
 * @param[in]  aFormat  A pointer to the format string.
 * @param[in]  ...      Arguments for the format specification.
 *
 */

/**
 * @def otLogInfoApi
 *
 * This method generates a log with level info for the API region.
 *
 * @param[in]  aFormat  A pointer to the format string.
 * @param[in]  ...      Arguments for the format specification.
 *
 */

/**
 * @def otLogDebgApi
 *
 * This method generates a log with level debug for the API region.
 *
 * @param[in]  aFormat  A pointer to the format string.
 * @param[in]  ...      Arguments for the format specification.
 *
 */
#ifdef OPENTHREAD_CONFIG_LOG_API
#define otLogCritApi(aFormat, ...) otLogCrit(kLogRegionApi, aFormat, ## __VA_ARGS__)
#define otLogWarnApi(aFormat, ...) otLogWarn(kLogRegionApi, aFormat, ## __VA_ARGS__)
#define otLogInfoApi(aFormat, ...) otLogInfo(kLogRegionApi, aFormat, ## __VA_ARGS__)
#define otLogDebgApi(aFormat, ...) otLogDebg(kLogRegionApi, aFormat, ## __VA_ARGS__)
#else
#define otLogCritApi(aFormat, ...)
#define otLogWarnApi(aFormat, ...)
#define otLogInfoApi(aFormat, ...)
#define otLogDebgApi(aFormat, ...)
#endif

/**
 * @def otLogCritMle
 *
 * This method generates a log with level critical for the MLE region.
 *
 * @param[in]  aFormat  A pointer to the format string.
 * @param[in]  ...      Arguments for the format specification.
 *
 */

/**
 * @def otLogWarnMle
 *
 * This method generates a log with level warning for the MLE region.
 *
 * @param[in]  aFormat  A pointer to the format string.
 * @param[in]  ...      Arguments for the format specification.
 *
 */

/**
 * @def otLogInfoMle
 *
 * This method generates a log with level info for the MLE region.
 *
 * @param[in]  aFormat  A pointer to the format string.
 * @param[in]  ...      Arguments for the format specification.
 *
 */

/**
 * @def otLogDebgMle
 *
 * This method generates a log with level debug for the MLE region.
 *
 * @param[in]  aFormat  A pointer to the format string.
 * @param[in]  ...      Arguments for the format specification.
 *
 */
#ifdef OPENTHREAD_CONFIG_LOG_MLE
#define otLogCritMle(aFormat, ...) otLogCrit(kLogRegionMle, aFormat, ## __VA_ARGS__)
#define otLogWarnMle(aFormat, ...) otLogWarn(kLogRegionMle, aFormat, ## __VA_ARGS__)
#define otLogInfoMle(aFormat, ...) otLogInfo(kLogRegionMle, aFormat, ## __VA_ARGS__)
#define otLogDebgMle(aFormat, ...) otLogDebg(kLogRegionMle, aFormat, ## __VA_ARGS__)
#else
#define otLogCritMle(aFormat, ...)
#define otLogWarnMle(aFormat, ...)
#define otLogInfoMle(aFormat, ...)
#define otLogDebgMle(aFormat, ...)
#endif

/**
 * @def otLogCritArp
 *
 * This method generates a log with level critical for the EID-to-RLOC mapping region.
 *
 * @param[in]  aFormat  A pointer to the format string.
 * @param[in]  ...      Arguments for the format specification.
 *
 */

/**
 * @def otLogWarnArp
 *
 * This method generates a log with level warning for the EID-to-RLOC mapping region.
 *
 * @param[in]  aFormat  A pointer to the format string.
 * @param[in]  ...      Arguments for the format specification.
 *
 */

/**
 * @def otLogInfoArp
 *
 * This method generates a log with level info for the EID-to-RLOC mapping region.
 *
 * @param[in]  aFormat  A pointer to the format string.
 * @param[in]  ...      Arguments for the format specification.
 *
 */

/**
 * @def otLogDebgArp
 *
 * This method generates a log with level debug for the EID-to-RLOC mapping region.
 *
 * @param[in]  aFormat  A pointer to the format string.
 * @param[in]  ...      Arguments for the format specification.
 *
 */
#ifdef OPENTHREAD_CONFIG_LOG_ARP
#define otLogCritArp(aFormat, ...) otLogCrit(kLogRegionArp, aFormat, ## __VA_ARGS__)
#define otLogWarnArp(aFormat, ...) otLogWarn(kLogRegionArp, aFormat, ## __VA_ARGS__)
#define otLogInfoArp(aFormat, ...) otLogInfo(kLogRegionArp, aFormat, ## __VA_ARGS__)
#define otLogDebgArp(aFormat, ...) otLogDebg(kLogRegionArp, aFormat, ## __VA_ARGS__)
#else
#define otLogCritArp(aFormat, ...)
#define otLogWarnArp(aFormat, ...)
#define otLogInfoArp(aFormat, ...)
#define otLogDebgArp(aFormat, ...)
#endif

/**
 * @def otLogCritNetData
 *
 * This method generates a log with level critical for the Network Data region.
 *
 * @param[in]  aFormat  A pointer to the format string.
 * @param[in]  ...      Arguments for the format specification.
 *
 */

/**
 * @def otLogWarnNetData
 *
 * This method generates a log with level warning for the Network Data region.
 *
 * @param[in]  aFormat  A pointer to the format string.
 * @param[in]  ...      Arguments for the format specification.
 *
 */

/**
 * @def otLogInfoNetData
 *
 * This method generates a log with level info for the Network Data region.
 *
 * @param[in]  aFormat  A pointer to the format string.
 * @param[in]  ...      Arguments for the format specification.
 *
 */

/**
 * @def otLogDebgNetData
 *
 * This method generates a log with level debug for the Network Data region.
 *
 * @param[in]  aFormat  A pointer to the format string.
 * @param[in]  ...      Arguments for the format specification.
 *
 */
#ifdef OPENTHREAD_CONFIG_LOG_NETDATA
#define otLogCritNetData(aFormat, ...) otLogCrit(kLogRegionNetData, aFormat, ## __VA_ARGS__)
#define otLogWarnNetData(aFormat, ...) otLogWarn(kLogRegionNetData, aFormat, ## __VA_ARGS__)
#define otLogInfoNetData(aFormat, ...) otLogInfo(kLogRegionNetData, aFormat, ## __VA_ARGS__)
#define otLogDebgNetData(aFormat, ...) otLogDebg(kLogRegionNetData, aFormat, ## __VA_ARGS__)
#else
#define otLogCritNetData(aFormat, ...)
#define otLogWarnNetData(aFormat, ...)
#define otLogInfoNetData(aFormat, ...)
#define otLogDebgNetData(aFormat, ...)
#endif

/**
 * @def otLogCritIcmp
 *
 * This method generates a log with level critical for the ICMPv6 region.
 *
 * @param[in]  aFormat  A pointer to the format string.
 * @param[in]  ...      Arguments for the format specification.
 *
 */

/**
 * @def otLogWarnIcmp
 *
 * This method generates a log with level warning for the ICMPv6 region.
 *
 * @param[in]  aFormat  A pointer to the format string.
 * @param[in]  ...      Arguments for the format specification.
 *
 */

/**
 * @def otLogInfoIcmp
 *
 * This method generates a log with level info for the ICMPv6 region.
 *
 * @param[in]  aFormat  A pointer to the format string.
 * @param[in]  ...      Arguments for the format specification.
 *
 */

/**
 * @def otLogDebgIcmp
 *
 * This method generates a log with level debug for the ICMPv6 region.
 *
 * @param[in]  aFormat  A pointer to the format string.
 * @param[in]  ...      Arguments for the format specification.
 *
 */
#ifdef OPENTHREAD_CONFIG_LOG_ICMP
#define otLogCritIcmp(aFormat, ...) otLogCrit(kLogRegionIcmp, aFormat, ## __VA_ARGS__)
#define otLogWarnIcmp(aFormat, ...) otLogWarn(kLogRegionIcmp, aFormat, ## __VA_ARGS__)
#define otLogInfoIcmp(aFormat, ...) otLogInfo(kLogRegionIcmp, aFormat, ## __VA_ARGS__)
#define otLogDebgIcmp(aFormat, ...) otLogDebg(kLogRegionIcmp, aFormat, ## __VA_ARGS__)
#else
#define otLogCritIcmp(aFormat, ...)
#define otLogWarnIcmp(aFormat, ...)
#define otLogInfoIcmp(aFormat, ...)
#define otLogDebgIcmp(aFormat, ...)
#endif

/**
 * @def otLogCritIp6
 *
 * This method generates a log with level critical for the IPv6 region.
 *
 * @param[in]  aFormat  A pointer to the format string.
 * @param[in]  ...      Arguments for the format specification.
 *
 */

/**
 * @def otLogWarnIp6
 *
 * This method generates a log with level warning for the IPv6 region.
 *
 * @param[in]  aFormat  A pointer to the format string.
 * @param[in]  ...      Arguments for the format specification.
 *
 */

/**
 * @def otLogInfoIp6
 *
 * This method generates a log with level info for the IPv6 region.
 *
 * @param[in]  aFormat  A pointer to the format string.
 * @param[in]  ...      Arguments for the format specification.
 *
 */

/**
 * @def otLogDebgIp6
 *
 * This method generates a log with level debug for the IPv6 region.
 *
 * @param[in]  aFormat  A pointer to the format string.
 * @param[in]  ...      Arguments for the format specification.
 *
 */
#ifdef OPENTHREAD_CONFIG_LOG_IP6
#define otLogCritIp6(aFormat, ...) otLogCrit(kLogRegionIp6, aFormat, ## __VA_ARGS__)
#define otLogWarnIp6(aFormat, ...) otLogWarn(kLogRegionIp6, aFormat, ## __VA_ARGS__)
#define otLogInfoIp6(aFormat, ...) otLogInfo(kLogRegionIp6, aFormat, ## __VA_ARGS__)
#define otLogDebgIp6(aFormat, ...) otLogDebg(kLogRegionIp6, aFormat, ## __VA_ARGS__)
#else
#define otLogCritIp6(aFormat, ...)
#define otLogWarnIp6(aFormat, ...)
#define otLogInfoIp6(aFormat, ...)
#define otLogDebgIp6(aFormat, ...)
#endif

/**
 * @def otLogCritMac
 *
 * This method generates a log with level critical for the MAC region.
 *
 * @param[in]  aFormat  A pointer to the format string.
 * @param[in]  ...      Arguments for the format specification.
 *
 */

/**
 * @def otLogWarnMac
 *
 * This method generates a log with level warning for the MAC region.
 *
 * @param[in]  aFormat  A pointer to the format string.
 * @param[in]  ...      Arguments for the format specification.
 *
 */

/**
 * @def otLogInfoMac
 *
 * This method generates a log with level info for the MAC region.
 *
 * @param[in]  aFormat  A pointer to the format string.
 * @param[in]  ...      Arguments for the format specification.
 *
 */

/**
 * @def otLogDebgMac
 *
 * This method generates a log with level debug for the MAC region.
 *
 * @param[in]  aFormat  A pointer to the format string.
 * @param[in]  ...      Arguments for the format specification.
 *
 */
#ifdef OPENTHREAD_CONFIG_LOG_MAC
#define otLogCritMac(aFormat, ...) otLogCrit(kLogRegionMac, aFormat, ## __VA_ARGS__)
#define otLogWarnMac(aFormat, ...) otLogWarn(kLogRegionMac, aFormat, ## __VA_ARGS__)
#define otLogInfoMac(aFormat, ...) otLogInfo(kLogRegionMac, aFormat, ## __VA_ARGS__)
#define otLogDebgMac(aFormat, ...) otLogDebg(kLogRegionMac, aFormat, ## __VA_ARGS__)
#else
#define otLogCritMac(aFormat, ...)
#define otLogWarnMac(aFormat, ...)
#define otLogInfoMac(aFormat, ...)
#define otLogDebgMac(aFormat, ...)
#endif

/**
 * @def otLogCritMem
 *
 * This method generates a log with level critical for the memory region.
 *
 * @param[in]  aFormat  A pointer to the format string.
 * @param[in]  ...      Arguments for the format specification.
 *
 */

/**
 * @def otLogWarnMem
 *
 * This method generates a log with level warning for the memory region.
 *
 * @param[in]  aFormat  A pointer to the format string.
 * @param[in]  ...      Arguments for the format specification.
 *
 */

/**
 * @def otLogInfoMem
 *
 * This method generates a log with level info for the memory region.
 *
 * @param[in]  aFormat  A pointer to the format string.
 * @param[in]  ...      Arguments for the format specification.
 *
 */

/**
 * @def otLogDebgMem
 *
 * This method generates a log with level debug for the memory region.
 *
 * @param[in]  aFormat  A pointer to the format string.
 * @param[in]  ...      Arguments for the format specification.
 *
 */
#ifdef OPENTHREAD_CONFIG_LOG_MEM
#define otLogCritMem(aFormat, ...) otLogCrit(kLogRegionMem, aFormat, ## __VA_ARGS__)
#define otLogWarnMem(aFormat, ...) otLogWarn(kLogRegionMem, aFormat, ## __VA_ARGS__)
#define otLogInfoMem(aFormat, ...) otLogInfo(kLogRegionMem, aFormat, ## __VA_ARGS__)
#define otLogDebgMem(aFormat, ...) otLogDebg(kLogRegionMem, aFormat, ## __VA_ARGS__)
#else
#define otLogCritMem(aFormat, ...)
#define otLogWarnMem(aFormat, ...)
#define otLogInfoMem(aFormat, ...)
#define otLogDebgMem(aFormat, ...)
#endif

/**
 * @}
 *
 */

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // DEBUG_H_
