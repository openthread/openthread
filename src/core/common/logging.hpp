/*
 *  Copyright (c) 2016, The OpenThread Authors.
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
 *   This file includes functions for debugging.
 */

#ifndef LOGGING_HPP_
#define LOGGING_HPP_

#include  " openthread_enable_defines.h"

#include <ctype.h>
#include <stdio.h>
#include "utils/wrap_string.h"

#include <openthread-core-config.h>
#include "openthread/types.h"
#include "openthread/instance.h"
#include "openthread/platform/logging.h"

#ifdef WINDOWS_LOGGING
#ifdef _KERNEL_MODE
#include <wdm.h>
#endif
#include "openthread/platform/logging-windows.h"
#ifdef WPP_NAME
#include WPP_NAME
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef WINDOWS_LOGGING
#define otLogFuncEntry()
#define otLogFuncEntryMsg(aFormat, ...)
#define otLogFuncExit()
#define otLogFuncExitMsg(aFormat, ...)
#define otLogFuncExitErr(error)
#endif

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
#define otLogCrit(aInstance, aRegion, aFormat, ...)                         \
    _otLogFormatter(aInstance, kLogLevelCrit, aRegion, aFormat, ## __VA_ARGS__)
#else
#define otLogCrit(aInstance, aRegion, aFormat, ...)
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
#define otLogWarn(aInstance, aRegion, aFormat, ...)                         \
    _otLogFormatter(aInstance, kLogLevelWarn, aRegion, aFormat, ## __VA_ARGS__)
#else
#define otLogWarn(aInstance, aRegion, aFormat, ...)
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
#define otLogInfo(aInstance, aRegion, aFormat, ...)                         \
    _otLogFormatter(aInstance, kLogLevelInfo, aRegion, aFormat, ## __VA_ARGS__)
#else
#define otLogInfo(aInstance, aRegion, aFormat, ...)
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
#define otLogDebg(aInstance, aRegion, aFormat, ...)                         \
    _otLogFormatter(aInstance, kLogLevelDebg, aRegion, aFormat, ## __VA_ARGS__)
#else
#define otLogDebg(aInstance, aRegion, aFormat, ...)
#endif

#ifndef WINDOWS_LOGGING

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
#if OPENTHREAD_CONFIG_LOG_API == 1
#define otLogCritApi(aInstance, aFormat, ...) otLogCrit(aInstance, kLogRegionApi, aFormat, ## __VA_ARGS__)
#define otLogWarnApi(aInstance, aFormat, ...) otLogWarn(aInstance, kLogRegionApi, aFormat, ## __VA_ARGS__)
#define otLogInfoApi(aInstance, aFormat, ...) otLogInfo(aInstance, kLogRegionApi, aFormat, ## __VA_ARGS__)
#define otLogDebgApi(aInstance, aFormat, ...) otLogDebg(aInstance, kLogRegionApi, aFormat, ## __VA_ARGS__)
#else
#define otLogCritApi(aInstance, aFormat, ...)
#define otLogWarnApi(aInstance, aFormat, ...)
#define otLogInfoApi(aInstance, aFormat, ...)
#define otLogDebgApi(aInstance, aFormat, ...)
#endif

/**
 * @def otLogCritMeshCoP
 *
 * This method generates a log with level critical for the MLE region.
 *
 * @param[in]  aFormat  A pointer to the format string.
 * @param[in]  ...      Arguments for the format specification.
 *
 */

/**
 * @def otLogWarnMeshCoP
 *
 * This method generates a log with level warning for the MLE region.
 *
 * @param[in]  aFormat  A pointer to the format string.
 * @param[in]  ...      Arguments for the format specification.
 *
 */

/**
 * @def otLogInfoMeshCoP
 *
 * This method generates a log with level info for the MLE region.
 *
 * @param[in]  aFormat  A pointer to the format string.
 * @param[in]  ...      Arguments for the format specification.
 *
 */

/**
 * @def otLogDebgMeshCoP
 *
 * This method generates a log with level debug for the MLE region.
 *
 * @param[in]  aFormat  A pointer to the format string.
 * @param[in]  ...      Arguments for the format specification.
 *
 */
#if OPENTHREAD_CONFIG_LOG_MLE == 1
#define otLogCritMeshCoP(aInstance, aFormat, ...) otLogCrit(aInstance, kLogRegionMeshCoP, aFormat, ## __VA_ARGS__)
#define otLogWarnMeshCoP(aInstance, aFormat, ...) otLogWarn(aInstance, kLogRegionMeshCoP, aFormat, ## __VA_ARGS__)
#define otLogInfoMeshCoP(aInstance, aFormat, ...) otLogInfo(aInstance, kLogRegionMeshCoP, aFormat, ## __VA_ARGS__)
#define otLogDebgMeshCoP(aInstance, aFormat, ...) otLogDebg(aInstance, kLogRegionMeshCoP, aFormat, ## __VA_ARGS__)
#else
#define otLogCritMeshCoP(aInstance, aFormat, ...)
#define otLogWarnMeshCoP(aInstance, aFormat, ...)
#define otLogInfoMeshCoP(aInstance, aFormat, ...)
#define otLogDebgMeshCoP(aInstance, aFormat, ...)
#endif

#define otLogCritMbedTls(aInstance, aFormat, ...) otLogCritMeshCoP(aInstance, aFormat, ## __VA_ARGS__)
#define otLogWarnMbedTls(aInstance, aFormat, ...) otLogWarnMeshCoP(aInstance, aFormat, ## __VA_ARGS__)
#define otLogInfoMbedTls(aInstance, aFormat, ...) otLogInfoMeshCoP(aInstance, aFormat, ## __VA_ARGS__)
#define otLogDebgMbedTls(aInstance, aFormat, ...) otLogDebgMeshCoP(aInstance, aFormat, ## __VA_ARGS__)

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
#if OPENTHREAD_CONFIG_LOG_MLE == 1
#define otLogCritMle(aInstance, aFormat, ...) otLogCrit(aInstance, kLogRegionMle, aFormat, ## __VA_ARGS__)
#define otLogWarnMle(aInstance, aFormat, ...) otLogWarn(aInstance, kLogRegionMle, aFormat, ## __VA_ARGS__)
#define otLogWarnMleErr(aInstance, aError, aFormat, ...)                    \
    otLogWarn(aInstance, kLogRegionMac, "Error %s: " aFormat, otThreadErrorToString(aError), ## __VA_ARGS__)
#define otLogInfoMle(aInstance, aFormat, ...) otLogInfo(aInstance, kLogRegionMle, aFormat, ## __VA_ARGS__)
#define otLogDebgMle(aInstance, aFormat, ...) otLogDebg(aInstance, kLogRegionMle, aFormat, ## __VA_ARGS__)
#else
#define otLogCritMle(aInstance, aFormat, ...)
#define otLogWarnMle(aInstance, aFormat, ...)
#define otLogWarnMleErr(aInstance, aError, aFormat, ...)
#define otLogInfoMle(aInstance, aFormat, ...)
#define otLogDebgMle(aInstance, aFormat, ...)
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
#if OPENTHREAD_CONFIG_LOG_ARP == 1
#define otLogCritArp(aInstance, aFormat, ...) otLogCrit(aInstance, kLogRegionArp, aFormat, ## __VA_ARGS__)
#define otLogWarnArp(aInstance, aFormat, ...) otLogWarn(aInstance, kLogRegionArp, aFormat, ## __VA_ARGS__)
#define otLogInfoArp(aInstance, aFormat, ...) otLogInfo(aInstance, kLogRegionArp, aFormat, ## __VA_ARGS__)
#define otLogDebgArp(aInstance, aFormat, ...) otLogDebg(aInstance, kLogRegionArp, aFormat, ## __VA_ARGS__)
#else
#define otLogCritArp(aInstance, aFormat, ...)
#define otLogWarnArp(aInstance, aFormat, ...)
#define otLogInfoArp(aInstance, aFormat, ...)
#define otLogDebgArp(aInstance, aFormat, ...)
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
#if OPENTHREAD_CONFIG_LOG_NETDATA == 1
#define otLogCritNetData(aInstance, aFormat, ...) otLogCrit(aInstance, kLogRegionNetData, aFormat, ## __VA_ARGS__)
#define otLogWarnNetData(aInstance, aFormat, ...) otLogWarn(aInstance, kLogRegionNetData, aFormat, ## __VA_ARGS__)
#define otLogInfoNetData(aInstance, aFormat, ...) otLogInfo(aInstance, kLogRegionNetData, aFormat, ## __VA_ARGS__)
#define otLogDebgNetData(aInstance, aFormat, ...) otLogDebg(aInstance, kLogRegionNetData, aFormat, ## __VA_ARGS__)
#else
#define otLogCritNetData(aInstance, aFormat, ...)
#define otLogWarnNetData(aInstance, aFormat, ...)
#define otLogInfoNetData(aInstance, aFormat, ...)
#define otLogDebgNetData(aInstance, aFormat, ...)
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
#if OPENTHREAD_CONFIG_LOG_ICMP == 1
#define otLogCritIcmp(aInstance, aFormat, ...) otLogCrit(aInstance, kLogRegionIcmp, aFormat, ## __VA_ARGS__)
#define otLogWarnIcmp(aInstance, aFormat, ...) otLogWarn(aInstance, kLogRegionIcmp, aFormat, ## __VA_ARGS__)
#define otLogInfoIcmp(aInstance, aFormat, ...) otLogInfo(aInstance, kLogRegionIcmp, aFormat, ## __VA_ARGS__)
#define otLogDebgIcmp(aInstance, aFormat, ...) otLogDebg(aInstance, kLogRegionIcmp, aFormat, ## __VA_ARGS__)
#else
#define otLogCritIcmp(aInstance, aFormat, ...)
#define otLogWarnIcmp(aInstance, aFormat, ...)
#define otLogInfoIcmp(aInstance, aFormat, ...)
#define otLogDebgIcmp(aInstance, aFormat, ...)
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
#if OPENTHREAD_CONFIG_LOG_IP6 == 1
#define otLogCritIp6(aInstance, aFormat, ...) otLogCrit(aInstance, kLogRegionIp6, aFormat, ## __VA_ARGS__)
#define otLogWarnIp6(aInstance, aFormat, ...) otLogWarn(aInstance, kLogRegionIp6, aFormat, ## __VA_ARGS__)
#define otLogInfoIp6(aInstance, aFormat, ...) otLogInfo(aInstance, kLogRegionIp6, aFormat, ## __VA_ARGS__)
#define otLogDebgIp6(aInstance, aFormat, ...) otLogDebg(aInstance, kLogRegionIp6, aFormat, ## __VA_ARGS__)
#else
#define otLogCritIp6(aInstance, aFormat, ...)
#define otLogWarnIp6(aInstance, aFormat, ...)
#define otLogInfoIp6(aInstance, aFormat, ...)
#define otLogDebgIp6(aInstance, aFormat, ...)
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
#if OPENTHREAD_CONFIG_LOG_MAC == 1
#define otLogCritMac(aInstance, aFormat, ...) otLogCrit(aInstance, kLogRegionMac, aFormat, ## __VA_ARGS__)
#define otLogWarnMac(aInstance, aFormat, ...) otLogWarn(aInstance, kLogRegionMac, aFormat, ## __VA_ARGS__)
#define otLogInfoMac(aInstance, aFormat, ...) otLogInfo(aInstance, kLogRegionMac, aFormat, ## __VA_ARGS__)
#define otLogDebgMac(aInstance, aFormat, ...) otLogDebg(aInstance, kLogRegionMac, aFormat, ## __VA_ARGS__)
#define otLogDebgMacErr(aInstance, aError, aFormat, ...)                    \
    otLogWarn(aInstance, kLogRegionMac, "Error %s: " aFormat, otThreadErrorToString(aError), ## __VA_ARGS__)
#else
#define otLogCritMac(aInstance, aFormat, ...)
#define otLogWarnMac(aInstance, aFormat, ...)
#define otLogInfoMac(aInstance, aFormat, ...)
#define otLogDebgMac(aInstance, aFormat, ...)
#define otLogDebgMacErr(aInstance, aError, aFormat, ...)
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
#if OPENTHREAD_CONFIG_LOG_MEM == 1
#define otLogCritMem(aInstance, aFormat, ...) otLogCrit(aInstance, kLogRegionMem, aFormat, ## __VA_ARGS__)
#define otLogWarnMem(aInstance, aFormat, ...) otLogWarn(aInstance, kLogRegionMem, aFormat, ## __VA_ARGS__)
#define otLogInfoMem(aInstance, aFormat, ...) otLogInfo(aInstance, kLogRegionMem, aFormat, ## __VA_ARGS__)
#define otLogDebgMem(aInstance, aFormat, ...) otLogDebg(aInstance, kLogRegionMem, aFormat, ## __VA_ARGS__)
#else
#define otLogCritMem(aInstance, aFormat, ...)
#define otLogWarnMem(aInstance, aFormat, ...)
#define otLogInfoMem(aInstance, aFormat, ...)
#define otLogDebgMem(aInstance, aFormat, ...)
#endif

/**
 * @def otLogCritNetDiag
 *
 * This method generates a log with level critical for the NETDIAG region.
 *
 * @param[in]  aFormat  A pointer to the format string.
 * @param[in]  ...      Arguments for the format specification.
 *
 */

/**
 * @def otLogWarnNetDiag
 *
 * This method generates a log with level warning for the NETDIAG region.
 *
 * @param[in]  aFormat  A pointer to the format string.
 * @param[in]  ...      Arguments for the format specification.
 *
 */

/**
 * @def otLogInfoNetDiag
 *
 * This method generates a log with level info for the NETDIAG region.
 *
 * @param[in]  aFormat  A pointer to the format string.
 * @param[in]  ...      Arguments for the format specification.
 *
 */

/**
 * @def otLogDebgNetDiag
 *
 * This method generates a log with level debug for the NETDIAG region.
 *
 * @param[in]  aFormat  A pointer to the format string.
 * @param[in]  ...      Arguments for the format specification.
 *
 */
#if OPENTHREAD_CONFIG_LOG_NETDIAG == 1
#define otLogCritNetDiag(aInstance, aFormat, ...) otLogCrit(aInstance, kLogRegionNetDiag, aFormat, ## __VA_ARGS__)
#define otLogWarnNetDiag(aInstance, aFormat, ...) otLogWarn(aInstance, kLogRegionNetDiag, aFormat, ## __VA_ARGS__)
#define otLogInfoNetDiag(aInstance, aFormat, ...) otLogInfo(aInstance, kLogRegionNetDiag, aFormat, ## __VA_ARGS__)
#define otLogDebgNetDiag(aInstance, aFormat, ...) otLogDebg(aInstance, kLogRegionNetDiag, aFormat, ## __VA_ARGS__)
#else
#define otLogCritNetDiag(aInstance, aFormat, ...)
#define otLogWarnNetDiag(aInstance, aFormat, ...)
#define otLogInfoNetDiag(aInstance, aFormat, ...)
#define otLogDebgNetDiag(aInstance, aFormat, ...)
#endif

/**
 * @def otLogCert
 *
 * This method generates a log with level none for the certification test.
 *
 * @param[in]  aFormat  A pointer to the format string.
 * @param[in]  ...      Arguments for the format specification.
 *
 */
#if OPENTHREAD_ENABLE_CERT_LOG
#define otLogCertMeshCoP(aInstance, aFormat, ...)                           \
    _otLogFormatter(aInstance, kLogLevelNone, kLogRegionMeshCoP, aFormat, ## __VA_ARGS__)
#else
#define otLogCertMeshCoP(aInstance, aFormat, ...)
#endif

/**
* @def otLogCritPlat
*
* This method generates a log with level critical for the Platform region.
*
* @param[in]  aFormat  A pointer to the format string.
* @param[in]  ...      Arguments for the format specification.
*
*/

/**
* @def otLogWarnPlat
*
* This method generates a log with level warning for the Platform region.
*
* @param[in]  aFormat  A pointer to the format string.
* @param[in]  ...      Arguments for the format specification.
*
*/

/**
* @def otLogInfoPlat
*
* This method generates a log with level info for the Platform region.
*
* @param[in]  aFormat  A pointer to the format string.
* @param[in]  ...      Arguments for the format specification.
*
*/

/**
* @def otLogDebgPlat
*
* This method generates a log with level debug for the Platform region.
*
* @param[in]  aFormat  A pointer to the format string.
* @param[in]  ...      Arguments for the format specification.
*
*/
#if OPENTHREAD_CONFIG_LOG_PLATFORM == 1
#define otLogCritPlat(aInstance, aFormat, ...) otLogCrit(aInstance, kLogRegionPlatform, aFormat, ## __VA_ARGS__)
#define otLogWarnPlat(aInstance, aFormat, ...) otLogWarn(aInstance, kLogRegionPlatform, aFormat, ## __VA_ARGS__)
#define otLogInfoPlat(aInstance, aFormat, ...) otLogInfo(aInstance, kLogRegionPlatform, aFormat, ## __VA_ARGS__)
#define otLogDebgPlat(aInstance, aFormat, ...) otLogDebg(aInstance, kLogRegionPlatform, aFormat, ## __VA_ARGS__)
#else
#define otLogCritPlat(aInstance, aFormat, ...)
#define otLogWarnPlat(aInstance, aFormat, ...)
#define otLogInfoPlat(aInstance, aFormat, ...)
#define otLogDebgPlat(aInstance, aFormat, ...)
#endif

#endif // WINDOWS_LOGGING

/**
 * @def otDumpCrit
 *
 * This method generates a memory dump with log level critical.
 *
 * @param[in]  aRegion  The log region.
 * @param[in]  aId      A pointer to a NULL-terminated string that is printed before the bytes.
 * @param[in]  aBuf     A pointer to the buffer.
 * @param[in]  aLength  Number of bytes to print.
 *
 */
#if OPENTHREAD_CONFIG_LOG_LEVEL >= OPENTHREAD_LOG_LEVEL_CRIT
#define otDumpCrit(aInstance, aRegion, aId, aBuf, aLength)                  \
    otDump(aInstance, kLogLevelCrit, aRegion, aId, aBuf, aLength)
#else
#define otDumpCrit(aInstance, aRegion, aId, aBuf, aLength)
#endif

/**
 * @def otDumpWarn
 *
 * This method generates a memory dump with log level warning.
 *
 * @param[in]  aRegion  The log region.
 * @param[in]  aId      A pointer to a NULL-terminated string that is printed before the bytes.
 * @param[in]  aBuf     A pointer to the buffer.
 * @param[in]  aLength  Number of bytes to print.
 *
 */
#if OPENTHREAD_CONFIG_LOG_LEVEL >= OPENTHREAD_LOG_LEVEL_WARN
#define otDumpWarn(aInstance, aRegion, aId, aBuf, aLength)                  \
    otDump(aInstance, kLogLevelWarn, aRegion, aId, aBuf, aLength)
#else
#define otDumpWarn(aInstance, aRegion, aId, aBuf, aLength)
#endif

/**
 * @def otDumpInfo
 *
 * This method generates a memory dump with log level info.
 *
 * @param[in]  aRegion  The log region.
 * @param[in]  aId      A pointer to a NULL-terminated string that is printed before the bytes.
 * @param[in]  aBuf     A pointer to the buffer.
 * @param[in]  aLength  Number of bytes to print.
 *
 */
#if OPENTHREAD_CONFIG_LOG_LEVEL >= OPENTHREAD_LOG_LEVEL_INFO
#define otDumpInfo(aInstance, aRegion, aId, aBuf, aLength)                  \
    otDump(aInstance, kLogLevelInfo, aRegion, aId, aBuf, aLength)
#else
#define otDumpInfo(aInstance, aRegion, aId, aBuf, aLength)
#endif

/**
 * @def otDumpDebg
 *
 * This method generates a memory dump with log level debug.
 *
 * @param[in]  aRegion  The log region.
 * @param[in]  aId      A pointer to a NULL-terminated string that is printed before the bytes.
 * @param[in]  aBuf     A pointer to the buffer.
 * @param[in]  aLength  Number of bytes to print.
 *
 */
#if OPENTHREAD_CONFIG_LOG_LEVEL >= OPENTHREAD_LOG_LEVEL_DEBG
#define otDumpDebg(aInstance, aRegion, aId, aBuf, aLength)                  \
    otDump(aInstance, kLogLevelDebg, aRegion, aId, aBuf, aLength)
#else
#define otDumpDebg(aInstance, aRegion, aId, aBuf, aLength)
#endif

/**
 * @def otDumpCritNetData
 *
 * This method generates a memory dump with log level debug and region Network Data.
 *
 * @param[in]  aId      A pointer to a NULL-terminated string that is printed before the bytes.
 * @param[in]  aBuf     A pointer to the buffer.
 * @param[in]  aLength  Number of bytes to print.
 *
 */

/**
 * @def otDumpWarnNetData
 *
 * This method generates a memory dump with log level warning and region Network Data.
 *
 * @param[in]  aId      A pointer to a NULL-terminated string that is printed before the bytes.
 * @param[in]  aBuf     A pointer to the buffer.
 * @param[in]  aLength  Number of bytes to print.
 *
 */

/**
 * @def otDumpInfoNetData
 *
 * This method generates a memory dump with log level info and region Network Data.
 *
 * @param[in]  aId      A pointer to a NULL-terminated string that is printed before the bytes.
 * @param[in]  aBuf     A pointer to the buffer.
 * @param[in]  aLength  Number of bytes to print.
 *
 */

/**
 * @def otDumpDebgNetData
 *
 * This method generates a memory dump with log level debug and region Network Data.
 *
 * @param[in]  aId      A pointer to a NULL-terminated string that is printed before the bytes.
 * @param[in]  aBuf     A pointer to the buffer.
 * @param[in]  aLength  Number of bytes to print.
 *
 */
#if OPENTHREAD_CONFIG_LOG_NETDATA == 1
#define otDumpCritNetData(aInstance, aId, aBuf, aLength) otDumpCrit(aInstance, kLogRegionNetData, aId, aBuf, aLength)
#define otDumpWarnNetData(aInstance, aId, aBuf, aLength) otDumpWarn(aInstance, kLogRegionNetData, aId, aBuf, aLength)
#define otDumpInfoNetData(aInstance, aId, aBuf, aLength) otDumpInfo(aInstance, kLogRegionNetData, aId, aBuf, aLength)
#define otDumpDebgNetData(aInstance, aId, aBuf, aLength) otDumpDebg(aInstance, kLogRegionNetData, aId, aBuf, aLength)
#else
#define otDumpCritNetData(aInstance, aId, aBuf, aLength)
#define otDumpWarnNetData(aInstance, aId, aBuf, aLength)
#define otDumpInfoNetData(aInstance, aId, aBuf, aLength)
#define otDumpDebgNetData(aInstance, aId, aBuf, aLength)
#endif

/**
 * @def otDumpCritMle
 *
 * This method generates a memory dump with log level debug and region MLE.
 *
 * @param[in]  aId      A pointer to a NULL-terminated string that is printed before the bytes.
 * @param[in]  aBuf     A pointer to the buffer.
 * @param[in]  aLength  Number of bytes to print.
 *
 */

/**
 * @def otDumpWarnMle
 *
 * This method generates a memory dump with log level warning and region MLE.
 *
 * @param[in]  aId      A pointer to a NULL-terminated string that is printed before the bytes.
 * @param[in]  aBuf     A pointer to the buffer.
 * @param[in]  aLength  Number of bytes to print.
 *
 */

/**
 * @def otDumpInfoMle
 *
 * This method generates a memory dump with log level info and region MLE.
 *
 * @param[in]  aId      A pointer to a NULL-terminated string that is printed before the bytes.
 * @param[in]  aBuf     A pointer to the buffer.
 * @param[in]  aLength  Number of bytes to print.
 *
 */

/**
 * @def otDumpDebgMle
 *
 * This method generates a memory dump with log level debug and region MLE.
 *
 * @param[in]  aId      A pointer to a NULL-terminated string that is printed before the bytes.
 * @param[in]  aBuf     A pointer to the buffer.
 * @param[in]  aLength  Number of bytes to print.
 *
 */
#if OPENTHREAD_CONFIG_LOG_MLE == 1
#define otDumpCritMle(aInstance, aId, aBuf, aLength) otDumpCrit(aInstance, kLogRegionMle, aId, aBuf, aLength)
#define otDumpWarnMle(aInstance, aId, aBuf, aLength) otDumpWarn(aInstance, kLogRegionMle, aId, aBuf, aLength)
#define otDumpInfoMle(aInstance, aId, aBuf, aLength) otDumpInfo(aInstance, kLogRegionMle, aId, aBuf, aLength)
#define otDumpDebgMle(aInstance, aId, aBuf, aLength) otDumpDebg(aInstance, kLogRegionMle, aId, aBuf, aLength)
#else
#define otDumpCritMle(aInstance, aId, aBuf, aLength)
#define otDumpWarnMle(aInstance, aId, aBuf, aLength)
#define otDumpInfoMle(aInstance, aId, aBuf, aLength)
#define otDumpDebgMle(aInstance, aId, aBuf, aLength)
#endif

/**
 * @def otDumpCritArp
 *
 * This method generates a memory dump with log level debug and region EID-to-RLOC mapping.
 *
 * @param[in]  aId      A pointer to a NULL-terminated string that is printed before the bytes.
 * @param[in]  aBuf     A pointer to the buffer.
 * @param[in]  aLength  Number of bytes to print.
 *
 */

/**
 * @def otDumpWarnArp
 *
 * This method generates a memory dump with log level warning and region EID-to-RLOC mapping.
 *
 * @param[in]  aId      A pointer to a NULL-terminated string that is printed before the bytes.
 * @param[in]  aBuf     A pointer to the buffer.
 * @param[in]  aLength  Number of bytes to print.
 *
 */

/**
 * @def otDumpInfoArp
 *
 * This method generates a memory dump with log level info and region EID-to-RLOC mapping.
 *
 * @param[in]  aId      A pointer to a NULL-terminated string that is printed before the bytes.
 * @param[in]  aBuf     A pointer to the buffer.
 * @param[in]  aLength  Number of bytes to print.
 *
 */

/**
 * @def otDumpDebgArp
 *
 * This method generates a memory dump with log level debug and region EID-to-RLOC mapping.
 *
 * @param[in]  aId      A pointer to a NULL-terminated string that is printed before the bytes.
 * @param[in]  aBuf     A pointer to the buffer.
 * @param[in]  aLength  Number of bytes to print.
 *
 */
#if OPENTHREAD_CONFIG_LOG_ARP == 1
#define otDumpCritArp(aInstance, aId, aBuf, aLength) otDumpCrit(aInstance, kLogRegionArp, aId, aBuf, aLength)
#define otDumpWarnArp(aInstance, aId, aBuf, aLength) otDumpWarn(aInstance, kLogRegionArp, aId, aBuf, aLength)
#define otDumpInfoArp(aInstance, aId, aBuf, aLength) otDumpInfo(aInstance, kLogRegionArp, aId, aBuf, aLength)
#define otDumpDebgArp(aInstance, aId, aBuf, aLength) otDumpDebg(aInstance, kLogRegionArp, aId, aBuf, aLength)
#else
#define otDumpCritArp(aInstance, aId, aBuf, aLength)
#define otDumpWarnArp(aInstance, aId, aBuf, aLength)
#define otDumpInfoArp(aInstance, aId, aBuf, aLength)
#define otDumpDebgArp(aInstance, aId, aBuf, aLength)
#endif

/**
 * @def otDumpCritIcmp
 *
 * This method generates a memory dump with log level debug and region ICMPv6.
 *
 * @param[in]  aId      A pointer to a NULL-terminated string that is printed before the bytes.
 * @param[in]  aBuf     A pointer to the buffer.
 * @param[in]  aLength  Number of bytes to print.
 *
 */

/**
 * @def otDumpWarnIcmp
 *
 * This method generates a memory dump with log level warning and region ICMPv6.
 *
 * @param[in]  aId      A pointer to a NULL-terminated string that is printed before the bytes.
 * @param[in]  aBuf     A pointer to the buffer.
 * @param[in]  aLength  Number of bytes to print.
 *
 */

/**
 * @def otDumpInfoIcmp
 *
 * This method generates a memory dump with log level info and region ICMPv6.
 *
 * @param[in]  aId      A pointer to a NULL-terminated string that is printed before the bytes.
 * @param[in]  aBuf     A pointer to the buffer.
 * @param[in]  aLength  Number of bytes to print.
 *
 */

/**
 * @def otDumpDebgIcmp
 *
 * This method generates a memory dump with log level debug and region ICMPv6.
 *
 * @param[in]  aId      A pointer to a NULL-terminated string that is printed before the bytes.
 * @param[in]  aBuf     A pointer to the buffer.
 * @param[in]  aLength  Number of bytes to print.
 *
 */
#if OPENTHREAD_CONFIG_LOG_ICMP == 1
#define otDumpCritIcmp(aInstance, aId, aBuf, aLength) otDumpCrit(aInstance, kLogRegionIcmp, aId, aBuf, aLength)
#define otDumpWarnIcmp(aInstance, aId, aBuf, aLength) otDumpWarn(aInstance, kLogRegionIcmp, aId, aBuf, aLength)
#define otDumpInfoIcmp(aInstance, aId, aBuf, aLength) otDumpInfo(aInstance, kLogRegionIcmp, aId, aBuf, aLength)
#define otDumpDebgIcmp(aInstance, aId, aBuf, aLength) otDumpDebg(aInstance, kLogRegionIcmp, aId, aBuf, aLength)
#else
#define otDumpCritIcmp(aInstance, aId, aBuf, aLength)
#define otDumpWarnIcmp(aInstance, aId, aBuf, aLength)
#define otDumpInfoIcmp(aInstance, aId, aBuf, aLength)
#define otDumpDebgIcmp(aInstance, aId, aBuf, aLength)
#endif

/**
 * @def otDumpCritIp6
 *
 * This method generates a memory dump with log level debug and region IPv6.
 *
 * @param[in]  aId      A pointer to a NULL-terminated string that is printed before the bytes.
 * @param[in]  aBuf     A pointer to the buffer.
 * @param[in]  aLength  Number of bytes to print.
 *
 */

/**
 * @def otDumpWarnIp6
 *
 * This method generates a memory dump with log level warning and region IPv6.
 *
 * @param[in]  aId      A pointer to a NULL-terminated string that is printed before the bytes.
 * @param[in]  aBuf     A pointer to the buffer.
 * @param[in]  aLength  Number of bytes to print.
 *
 */

/**
 * @def otDumpInfoIp6
 *
 * This method generates a memory dump with log level info and region IPv6.
 *
 * @param[in]  aId      A pointer to a NULL-terminated string that is printed before the bytes.
 * @param[in]  aBuf     A pointer to the buffer.
 * @param[in]  aLength  Number of bytes to print.
 *
 */

/**
 * @def otDumpDebgIp6
 *
 * This method generates a memory dump with log level debug and region IPv6.
 *
 * @param[in]  aId      A pointer to a NULL-terminated string that is printed before the bytes.
 * @param[in]  aBuf     A pointer to the buffer.
 * @param[in]  aLength  Number of bytes to print.
 *
 */
#if OPENTHREAD_CONFIG_LOG_IP6 == 1
#define otDumpCritIp6(aInstance, aId, aBuf, aLength) otDumpCrit(aInstance, kLogRegionIp6, aId, aBuf, aLength)
#define otDumpWarnIp6(aInstance, aId, aBuf, aLength) otDumpWarn(aInstance, kLogRegionIp6, aId, aBuf, aLength)
#define otDumpInfoIp6(aInstance, aId, aBuf, aLength) otDumpInfo(aInstance, kLogRegionIp6, aId, aBuf, aLength)
#define otDumpDebgIp6(aInstance, aId, aBuf, aLength) otDumpDebg(aInstance, kLogRegionIp6, aId, aBuf, aLength)
#else
#define otDumpCritIp6(aInstance, aId, aBuf, aLength)
#define otDumpWarnIp6(aInstance, aId, aBuf, aLength)
#define otDumpInfoIp6(aInstance, aId, aBuf, aLength)
#define otDumpDebgIp6(aInstance, aId, aBuf, aLength)
#endif

/**
 * @def otDumpCritMac
 *
 * This method generates a memory dump with log level debug and region MAC.
 *
 * @param[in]  aId      A pointer to a NULL-terminated string that is printed before the bytes.
 * @param[in]  aBuf     A pointer to the buffer.
 * @param[in]  aLength  Number of bytes to print.
 *
 */

/**
 * @def otDumpWarnMac
 *
 * This method generates a memory dump with log level warning and region MAC.
 *
 * @param[in]  aId      A pointer to a NULL-terminated string that is printed before the bytes.
 * @param[in]  aBuf     A pointer to the buffer.
 * @param[in]  aLength  Number of bytes to print.
 *
 */

/**
 * @def otDumpInfoMac
 *
 * This method generates a memory dump with log level info and region MAC.
 *
 * @param[in]  aId      A pointer to a NULL-terminated string that is printed before the bytes.
 * @param[in]  aBuf     A pointer to the buffer.
 * @param[in]  aLength  Number of bytes to print.
 *
 */

/**
 * @def otDumpDebgMac
 *
 * This method generates a memory dump with log level debug and region MAC.
 *
 * @param[in]  aId      A pointer to a NULL-terminated string that is printed before the bytes.
 * @param[in]  aBuf     A pointer to the buffer.
 * @param[in]  aLength  Number of bytes to print.
 *
 */
#if OPENTHREAD_CONFIG_LOG_MAC == 1
#define otDumpCritMac(aInstance, aId, aBuf, aLength) otDumpCrit(aInstance, kLogRegionMac, aId, aBuf, aLength)
#define otDumpWarnMac(aInstance, aId, aBuf, aLength) otDumpWarn(aInstance, kLogRegionMac, aId, aBuf, aLength)
#define otDumpInfoMac(aInstance, aId, aBuf, aLength) otDumpInfo(aInstance, kLogRegionMac, aId, aBuf, aLength)
#define otDumpDebgMac(aInstance, aId, aBuf, aLength) otDumpDebg(aInstance, kLogRegionMac, aId, aBuf, aLength)
#else
#define otDumpCritMac(aInstance, aId, aBuf, aLength)
#define otDumpWarnMac(aInstance, aId, aBuf, aLength)
#define otDumpInfoMac(aInstance, aId, aBuf, aLength)
#define otDumpDebgMac(aInstance, aId, aBuf, aLength)
#endif

/**
 * @def otDumpCritMem
 *
 * This method generates a memory dump with log level debug and region memory.
 *
 * @param[in]  aId      A pointer to a NULL-terminated string that is printed before the bytes.
 * @param[in]  aBuf     A pointer to the buffer.
 * @param[in]  aLength  Number of bytes to print.
 *
 */

/**
 * @def otDumpWarnMem
 *
 * This method generates a memory dump with log level warning and region memory.
 *
 * @param[in]  aId      A pointer to a NULL-terminated string that is printed before the bytes.
 * @param[in]  aBuf     A pointer to the buffer.
 * @param[in]  aLength  Number of bytes to print.
 *
 */

/**
 * @def otDumpInfoMem
 *
 * This method generates a memory dump with log level info and region memory.
 *
 * @param[in]  aId      A pointer to a NULL-terminated string that is printed before the bytes.
 * @param[in]  aBuf     A pointer to the buffer.
 * @param[in]  aLength  Number of bytes to print.
 *
 */

/**
 * @def otDumpDebgMem
 *
 * This method generates a memory dump with log level debug and region memory.
 *
 * @param[in]  aId      A pointer to a NULL-terminated string that is printed before the bytes.
 * @param[in]  aBuf     A pointer to the buffer.
 * @param[in]  aLength  Number of bytes to print.
 *
 */
#if OPENTHREAD_CONFIG_LOG_MEM == 1
#define otDumpCritMem(aInstance, aId, aBuf, aLength) otDumpCrit(aInstance, kLogRegionMem, aId, aBuf, aLength)
#define otDumpWarnMem(aInstance, aId, aBuf, aLength) otDumpWarn(aInstance, kLogRegionMem, aId, aBuf, aLength)
#define otDumpInfoMem(aInstance, aId, aBuf, aLength) otDumpInfo(aInstance, kLogRegionMem, aId, aBuf, aLength)
#define otDumpDebgMem(aInstance, aId, aBuf, aLength) otDumpDebg(aInstance, kLogRegionMem, aId, aBuf, aLength)
#else
#define otDumpCritMem(aInstance, aId, aBuf, aLength)
#define otDumpWarnMem(aInstance, aId, aBuf, aLength)
#define otDumpInfoMem(aInstance, aId, aBuf, aLength)
#define otDumpDebgMem(aInstance, aId, aBuf, aLength)
#endif

/**
 * @def otDumpCert
 *
 * This method generates a memory dump with log level none for the certification test.
 *
 * @param[in]  aId      A pointer to a NULL-terminated string that is printed before the bytes.
 * @param[in]  aBuf     A pointer to the buffer.
 * @param[in]  aLength  Number of bytes to print.
 *
 */
#if OPENTHREAD_ENABLE_CERT_LOG
#define otDumpCertMeshCoP(aInstance, aId, aBuf, aLength)                    \
    otDump(aInstance, kLogLevelNone, kLogRegionMeshCoP, aId, aBuf, aLength)
#else
#define otDumpCertMeshCoP(aInstance, aId, aBuf, aLength)
#endif

/**
 * This method dumps bytes to the log in a human-readable fashion.
 *
 * @param[in]  aLevel   The log level.
 * @param[in]  aRegion  The log region.
 * @param[in]  aId      A pointer to a NULL-terminated string that is printed before the bytes.
 * @param[in]  aBuf     A pointer to the buffer.
 * @param[in]  aLength  Number of bytes to print.
 *
 */
void otDump(otInstance *aIntsance, otLogLevel aLevel, otLogRegion aRegion, const char *aId, const void *aBuf,
            const size_t aLength);

#if OPENTHREAD_CONFIG_LOG_PREPEND_LEVEL == 1
/**
* This method converts the log level value into a string
*
* @param[in]  aLevel  The log level.
*
* @returns A const char pointer to the C string corresponding to the log level.
*
*/
const char *otLogLevelToString(otLogLevel aLevel);
#endif

#if OPENTHREAD_CONFIG_LOG_PREPEND_REGION == 1
/**
 * This method converts the log region value into a string
 *
 * @param[in]  aRegion  The log region.
 *
 * @returns A const char pointer to the C string corresponding to the log region.
 *
 */
const char *otLogRegionToString(otLogRegion aRegion);
#endif

#if OPENTHREAD_CONFIG_LOG_PREPEND_LEVEL == 1

#if OPENTHREAD_CONFIG_LOG_PREPEND_REGION == 1

/**
 * Local/private macro to format the log message
 */
#define _otLogFormatter(aInstance, aLogLevel, aRegion, aFormat, ...)        \
    _otDynamicLog(                                                          \
        aInstance,                                                          \
        aLogLevel,                                                          \
        aRegion,                                                            \
        "[%s]%s: " aFormat OPENTHREAD_CONFIG_LOG_SUFFIX,                    \
        otLogLevelToString(aLogLevel),                                      \
        otLogRegionToString(aRegion),                                       \
        ## __VA_ARGS__                                                      \
    )

#else  // OPENTHREAD_CONFIG_LOG_PREPEND_REGION

/**
* Local/private macro to format the log message
*/
#define _otLogFormatter(aInstanc, aLogLevel, aRegion, aFormat, ...)         \
    _otDynamicLog(                                                          \
        aInstance,                                                          \
        aLogLevel,                                                          \
        aRegion,                                                            \
        "[%s]: " aFormat OPENTHREAD_CONFIG_LOG_SUFFIX,                      \
        otLogLevelToString(aLogLevel),                                      \
        ## __VA_ARGS__                                                      \
    )

#endif

#else  // OPENTHREAD_CONFIG_LOG_PREPEND_LEVEL

#if OPENTHREAD_CONFIG_LOG_PREPEND_REGION == 1

/**
* Local/private macro to format the log message
*/
#define _otLogFormatter(aInstance, aLogLevel, aRegion, aFormat, ...)        \
    _otDynamicLog(                                                          \
        aInstance,                                                          \
        aLogLevel,                                                          \
        aRegion,                                                            \
        "%s: " aFormat OPENTHREAD_CONFIG_LOG_SUFFIX,                        \
        otLogRegionToString(aRegion),                                       \
        ## __VA_ARGS__                                                      \
    )

#else  // OPENTHREAD_CONFIG_LOG_PREPEND_REGION

/**
* Local/private macro to format the log message
*/
#define _otLogFormatter(aInstace, aLogLevel, aRegion, aFormat, ...)         \
    _otDynamicLog(                                                          \
        aInstance,                                                          \
        aLogLevel,                                                          \
        aRegion,                                                            \
        aFormat OPENTHREAD_CONFIG_LOG_SUFFIX,                               \
        ## __VA_ARGS__                                                      \
    )

#endif

#endif // OPENTHREAD_CONFIG_LOG_PREPEND_REGION

#if OPENTHREAD_CONFIG_ENABLE_DYNAMIC_LOG_LEVEL == 1

/**
* Local/private macro to dynamically filter log level.
*/
#define _otDynamicLog(aInstance, aLogLevel, aRegion, aFormat, ...)          \
    do {                                                                    \
        if (otGetDynamicLogLevel(aInstance) >= aLogLevel)                   \
            _otPlatLog(aLogLevel, aRegion, aFormat, ## __VA_ARGS__);        \
    } while (false)

#else // OPENTHREAD_CONFIG_ENABLE_DYNAMIC_LOG_LEVEL

#define _otDynamicLog(aInstance, aLogLevel, aRegion, aFormat, ...)          \
    _otPlatLog(aLogLevel, aRegion, aFormat, ## __VA_ARGS__)

#endif // OPENTHREAD_CONFIG_ENABLE_DYNAMIC_LOG_LEVEL

/**
 * `OPENTHREAD_CONFIG_PLAT_LOG_FUNCTION` is a configuration parameter (see `openthread-core-default-config.h`) which
 * specifies the function/macro to be used for logging in OpenThread. By default it is set to `otPlatLog()`.
 */
#define _otPlatLog(aLogLevel, aRegion, aFormat, ...)                        \
    OPENTHREAD_CONFIG_PLAT_LOG_FUNCTION(aLogLevel, aRegion, aFormat, ## __VA_ARGS__)

#ifdef __cplusplus
};
#endif

#endif  // LOGGING_HPP_
