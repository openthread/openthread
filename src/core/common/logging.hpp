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
 *   This file includes logging related macro/function definitions.
 */

#ifndef LOGGING_HPP_
#define LOGGING_HPP_

#include "openthread-core-config.h"

#include <ctype.h>
#include <stdio.h>
#include "utils/wrap_string.h"

#include <openthread/logging.h>
#include <openthread/platform/logging.h>

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

/**
 * Log level prefix string definitions.
 *
 */
#if OPENTHREAD_CONFIG_LOG_PREPEND_LEVEL
#define _OT_LEVEL_NONE_PREFIX "[NONE]"
#define _OT_LEVEL_CRIT_PREFIX "[CRIT]"
#define _OT_LEVEL_WARN_PREFIX "[WARN]"
#define _OT_LEVEL_NOTE_PREFIX "[NOTE]"
#define _OT_LEVEL_INFO_PREFIX "[INFO]"
#define _OT_LEVEL_DEBG_PREFIX "[DEBG]"
#define _OT_REGION_SUFFIX ": "
#else
#define _OT_LEVEL_NONE_PREFIX ""
#define _OT_LEVEL_CRIT_PREFIX ""
#define _OT_LEVEL_WARN_PREFIX ""
#define _OT_LEVEL_NOTE_PREFIX ""
#define _OT_LEVEL_INFO_PREFIX ""
#define _OT_LEVEL_DEBG_PREFIX ""
#define _OT_REGION_SUFFIX
#endif

/**
 * Log region prefix string definitions.
 *
 */
#if OPENTHREAD_CONFIG_LOG_PREPEND_REGION
#define _OT_REGION_API_PREFIX "-API-----: "
#define _OT_REGION_MLE_PREFIX "-MLE-----: "
#define _OT_REGION_ARP_PREFIX "-ARP-----: "
#define _OT_REGION_NET_DATA_PREFIX "-N-DATA--: "
#define _OT_REGION_ICMP_PREFIX "-ICMP----: "
#define _OT_REGION_IP6_PREFIX "-IP6-----: "
#define _OT_REGION_MAC_PREFIX "-MAC-----: "
#define _OT_REGION_MEM_PREFIX "-MEM-----: "
#define _OT_REGION_NCP_PREFIX "-NCP-----: "
#define _OT_REGION_MESH_COP_PREFIX "-MESH-CP-: "
#define _OT_REGION_NET_DIAG_PREFIX "-DIAG----: "
#define _OT_REGION_PLATFORM_PREFIX "-PLAT----: "
#define _OT_REGION_COAP_PREFIX "-COAP----: "
#define _OT_REGION_CLI_PREFIX "-CLI-----: "
#define _OT_REGION_CORE_PREFIX "-CORE----: "
#define _OT_REGION_UTIL_PREFIX "-UTIL----: "
#else
#define _OT_REGION_API_PREFIX _OT_REGION_SUFFIX
#define _OT_REGION_MLE_PREFIX _OT_REGION_SUFFIX
#define _OT_REGION_ARP_PREFIX _OT_REGION_SUFFIX
#define _OT_REGION_NET_DATA_PREFIX _OT_REGION_SUFFIX
#define _OT_REGION_ICMP_PREFIX _OT_REGION_SUFFIX
#define _OT_REGION_IP6_PREFIX _OT_REGION_SUFFIX
#define _OT_REGION_MAC_PREFIX _OT_REGION_SUFFIX
#define _OT_REGION_MEM_PREFIX _OT_REGION_SUFFIX
#define _OT_REGION_NCP_PREFIX _OT_REGION_SUFFIX
#define _OT_REGION_MESH_COP_PREFIX _OT_REGION_SUFFIX
#define _OT_REGION_NET_DIAG_PREFIX _OT_REGION_SUFFIX
#define _OT_REGION_PLATFORM_PREFIX _OT_REGION_SUFFIX
#define _OT_REGION_COAP_PREFIX _OT_REGION_SUFFIX
#define _OT_REGION_CLI_PREFIX _OT_REGION_SUFFIX
#define _OT_REGION_CORE_PREFIX _OT_REGION_SUFFIX
#define _OT_REGION_UTIL_PREFIX _OT_REGION_SUFFIX
#endif

/**
 * @def otLogCrit
 *
 * Logging at log level critical.
 *
 * @param[in]  aRegion   The log region.
 * @param[in]  aFormat   A pointer to the format string.
 * @param[in]  ...       Arguments for the format specification.
 *
 */
#if OPENTHREAD_CONFIG_LOG_LEVEL >= OT_LOG_LEVEL_CRIT
#define otLogCrit(aRegion, aFormat, ...) \
    _otLogFormatter(OT_LOG_LEVEL_CRIT, aRegion, _OT_LEVEL_CRIT_PREFIX aFormat, ##__VA_ARGS__)
#else
#define otLogCrit(aRegion, aFormat, ...)
#endif

/**
 * @def otLogWarn
 *
 * Logging at log level warning.
 *
 * @param[in]  aRegion   The log region.
 * @param[in]  aFormat   A pointer to the format string.
 * @param[in]  ...       Arguments for the format specification.
 *
 */
#if OPENTHREAD_CONFIG_LOG_LEVEL >= OT_LOG_LEVEL_WARN
#define otLogWarn(aRegion, aFormat, ...) \
    _otLogFormatter(OT_LOG_LEVEL_WARN, aRegion, _OT_LEVEL_WARN_PREFIX aFormat, ##__VA_ARGS__)
#else
#define otLogWarn(aRegion, aFormat, ...)
#endif

/**
 * @def otLogNote
 *
 * Logging at log level note
 *
 * @param[in]  aRegion   The log region.
 * @param[in]  aFormat   A pointer to the format string.
 * @param[in]  ...       Arguments for the format specification.
 *
 */
#if OPENTHREAD_CONFIG_LOG_LEVEL >= OT_LOG_LEVEL_NOTE
#define otLogNote(aRegion, aFormat, ...) \
    _otLogFormatter(OT_LOG_LEVEL_NOTE, aRegion, _OT_LEVEL_NOTE_PREFIX aFormat, ##__VA_ARGS__)
#else
#define otLogNote(aRegion, aFormat, ...)
#endif

/**
 * @def otLogInfo
 *
 * Logging at log level info.
 *
 * @param[in]  aRegion   The log region.
 * @param[in]  aFormat   A pointer to the format string.
 * @param[in]  ...       Arguments for the format specification.
 *
 */
#if OPENTHREAD_CONFIG_LOG_LEVEL >= OT_LOG_LEVEL_INFO
#define otLogInfo(aRegion, aFormat, ...) \
    _otLogFormatter(OT_LOG_LEVEL_INFO, aRegion, _OT_LEVEL_INFO_PREFIX aFormat, ##__VA_ARGS__)
#else
#define otLogInfo(aRegion, aFormat, ...)
#endif

/**
 * @def otLogDebg
 *
 * Logging at log level debug.
 *
 * @param[in]  aRegion   The log region.
 * @param[in]  aFormat   A pointer to the format string.
 * @param[in]  ...       Arguments for the format specification.
 *
 */
#if OPENTHREAD_CONFIG_LOG_LEVEL >= OT_LOG_LEVEL_DEBG
#define otLogDebg(aRegion, aFormat, ...) \
    _otLogFormatter(OT_LOG_LEVEL_DEBG, aRegion, _OT_LEVEL_DEBG_PREFIX aFormat, ##__VA_ARGS__)
#else
#define otLogDebg(aRegion, aFormat, ...)
#endif

#ifndef WINDOWS_LOGGING

/**
 * @def otLogCritApi
 *
 * This method generates a log with level critical for the API region.
 *
 * @param[in]  aFormat      A pointer to the format string.
 * @param[in]  ...          Arguments for the format specification.
 *
 */

/**
 * @def otLogWarnApi
 *
 * This method generates a log with level warning for the API region.
 *
 * @param[in]  aFormat      A pointer to the format string.
 * @param[in]  ...          Arguments for the format specification.
 *
 */

/**
 * @def otLogNoteApi
 *
 * This method generates a log with level note for the API region.
 *
 * @param[in]  aFormat      A pointer to the format string.
 * @param[in]  ...          Arguments for the format specification.
 *
 */

/**
 * @def otLogInfoApi
 *
 * This method generates a log with level info for the API region.
 *
 * @param[in]  aFormat      A pointer to the format string.
 * @param[in]  ...          Arguments for the format specification.
 *
 */

/**
 * @def otLogDebgApi
 *
 * This method generates a log with level debug for the API region.
 *
 * @param[in]  aFormat      A pointer to the format string.
 * @param[in]  ...          Arguments for the format specification.
 *
 */
#if OPENTHREAD_CONFIG_LOG_API == 1
#define otLogCritApi(aFormat, ...) otLogCrit(OT_LOG_REGION_API, _OT_REGION_API_PREFIX aFormat, ##__VA_ARGS__)
#define otLogWarnApi(aFormat, ...) otLogWarn(OT_LOG_REGION_API, _OT_REGION_API_PREFIX aFormat, ##__VA_ARGS__)
#define otLogNoteApi(aFormat, ...) otLogNote(OT_LOG_REGION_API, _OT_REGION_API_PREFIX aFormat, ##__VA_ARGS__)
#define otLogInfoApi(aFormat, ...) otLogInfo(OT_LOG_REGION_API, _OT_REGION_API_PREFIX aFormat, ##__VA_ARGS__)
#define otLogDebgApi(aFormat, ...) otLogDebg(OT_LOG_REGION_API, _OT_REGION_API_PREFIX aFormat, ##__VA_ARGS__)
#else
#define otLogCritApi(aFormat, ...)
#define otLogWarnApi(aFormat, ...)
#define otLogNoteApi(aFormat, ...)
#define otLogInfoApi(aFormat, ...)
#define otLogDebgApi(aFormat, ...)
#endif

/**
 * @def otLogCritMeshCoP
 *
 * This method generates a log with level critical for the MLE region.
 *
 * @param[in]  aFormat      A pointer to the format string.
 * @param[in]  ...          Arguments for the format specification.
 *
 *
 */

/**
 * @def otLogWarnMeshCoP
 *
 * This method generates a log with level warning for the MLE region.
 *
 * @param[in]  aFormat      A pointer to the format string.
 * @param[in]  ...          Arguments for the format specification.
 *
 */

/**
 * @def otLogNoteMeshCoP
 *
 * This method generates a log with level note for the MLE region.
 *
 * @param[in]  aFormat      A pointer to the format string.
 * @param[in]  ...          Arguments for the format specification.
 *
 */

/**
 * @def otLogInfoMeshCoP
 *
 * This method generates a log with level info for the MLE region.
 *
 * @param[in]  aFormat      A pointer to the format string.
 * @param[in]  ...          Arguments for the format specification.
 *
 */

/**
 * @def otLogDebgMeshCoP
 *
 * This method generates a log with level debug for the MLE region.
 *
 * @param[in]  aFormat      A pointer to the format string.
 * @param[in]  ...          Arguments for the format specification.
 *
 */
#if OPENTHREAD_CONFIG_LOG_MLE == 1
#define otLogCritMeshCoP(aFormat, ...) \
    otLogCrit(OT_LOG_REGION_MESH_COP, _OT_REGION_MESH_COP_PREFIX aFormat, ##__VA_ARGS__)
#define otLogWarnMeshCoP(aFormat, ...) \
    otLogWarn(OT_LOG_REGION_MESH_COP, _OT_REGION_MESH_COP_PREFIX aFormat, ##__VA_ARGS__)
#define otLogNoteMeshCoP(aFormat, ...) \
    otLogNote(OT_LOG_REGION_MESH_COP, _OT_REGION_MESH_COP_PREFIX aFormat, ##__VA_ARGS__)
#define otLogInfoMeshCoP(aFormat, ...) \
    otLogInfo(OT_LOG_REGION_MESH_COP, _OT_REGION_MESH_COP_PREFIX aFormat, ##__VA_ARGS__)
#define otLogDebgMeshCoP(aFormat, ...) \
    otLogDebg(OT_LOG_REGION_MESH_COP, _OT_REGION_MESH_COP_PREFIX aFormat, ##__VA_ARGS__)
#else
#define otLogCritMeshCoP(aFormat, ...)
#define otLogWarnMeshCoP(aFormat, ...)
#define otLogNoteMeshCoP(aFormat, ...)
#define otLogInfoMeshCoP(aFormat, ...)
#define otLogDebgMeshCoP(aFormat, ...)
#endif

#define otLogCritMbedTls(aFormat, ...) otLogCritMeshCoP(aFormat, ##__VA_ARGS__)
#define otLogWarnMbedTls(aFormat, ...) otLogWarnMeshCoP(aFormat, ##__VA_ARGS__)
#define otLogNoteMbedTls(aFormat, ...) otLogNoteMeshCoP(aFormat, ##__VA_ARGS__)
#define otLogInfoMbedTls(aFormat, ...) otLogInfoMeshCoP(aFormat, ##__VA_ARGS__)
#define otLogDebgMbedTls(aFormat, ...) otLogDebgMeshCoP(aFormat, ##__VA_ARGS__)

/**
 * @def otLogCritMle
 *
 * This method generates a log with level critical for the MLE region.
 *
 * @param[in]  aFormat      A pointer to the format string.
 * @param[in]  ...          Arguments for the format specification.
 *
 */

/**
 * @def otLogWarnMle
 *
 * This method generates a log with level warning for the MLE region.
 *
 * @param[in]  aFormat      A pointer to the format string.
 * @param[in]  ...          Arguments for the format specification.
 *
 */

/**
 * @def otLogNoteMle
 *
 * This method generates a log with level note for the MLE region.
 *
 * @param[in]  aFormat      A pointer to the format string.
 * @param[in]  ...          Arguments for the format specification.
 *
 */

/**
 * @def otLogInfoMle
 *
 * This method generates a log with level info for the MLE region.
 *
 * @param[in]  aFormat      A pointer to the format string.
 * @param[in]  ...          Arguments for the format specification.
 *
 */

/**
 * @def otLogDebgMle
 *
 * This method generates a log with level debug for the MLE region.
 *
 * @param[in]  aFormat      A pointer to the format string.
 * @param[in]  ...          Arguments for the format specification.
 *
 *
 */
#if OPENTHREAD_CONFIG_LOG_MLE == 1
#define otLogCritMle(aFormat, ...) otLogCrit(OT_LOG_REGION_MLE, _OT_REGION_MLE_PREFIX aFormat, ##__VA_ARGS__)
#define otLogWarnMle(aFormat, ...) otLogWarn(OT_LOG_REGION_MLE, _OT_REGION_MLE_PREFIX aFormat, ##__VA_ARGS__)
#define otLogWarnMleErr(aError, aFormat, ...)                                                               \
    otLogWarn(OT_LOG_REGION_MLE, _OT_REGION_MLE_PREFIX "Error %s: " aFormat, otThreadErrorToString(aError), \
              ##__VA_ARGS__)
#define otLogNoteMle(aFormat, ...) otLogNote(OT_LOG_REGION_MLE, _OT_REGION_MLE_PREFIX aFormat, ##__VA_ARGS__)
#define otLogInfoMle(aFormat, ...) otLogInfo(OT_LOG_REGION_MLE, _OT_REGION_MLE_PREFIX aFormat, ##__VA_ARGS__)
#define otLogDebgMle(aFormat, ...) otLogDebg(OT_LOG_REGION_MLE, _OT_REGION_MLE_PREFIX aFormat, ##__VA_ARGS__)
#else
#define otLogCritMle(aFormat, ...)
#define otLogWarnMle(aFormat, ...)
#define otLogWarnMleErr(aError, aFormat, ...)
#define otLogNoteMle(aFormat, ...)
#define otLogInfoMle(aFormat, ...)
#define otLogDebgMle(aFormat, ...)
#endif

/**
 * @def otLogCritArp
 *
 * This method generates a log with level critical for the EID-to-RLOC mapping region.
 *
 * @param[in]  aFormat      A pointer to the format string.
 * @param[in]  ...          Arguments for the format specification.
 *
 */

/**
 * @def otLogWarnArp
 *
 * This method generates a log with level warning for the EID-to-RLOC mapping region.
 *
 * @param[in]  aFormat      A pointer to the format string.
 * @param[in]  ...          Arguments for the format specification.
 *
 */

/**
 * @def otLogInfoArp
 *
 * This method generates a log with level note for the EID-to-RLOC mapping region.
 *
 * @param[in]  aFormat      A pointer to the format string.
 * @param[in]  ...          Arguments for the format specification.
 *
 */

/**
 * @def otLogInfoArp
 *
 * This method generates a log with level info for the EID-to-RLOC mapping region.
 *
 * @param[in]  aFormat      A pointer to the format string.
 * @param[in]  ...          Arguments for the format specification.
 *
 */

/**
 * @def otLogDebgArp
 *
 * This method generates a log with level debug for the EID-to-RLOC mapping region.
 *
 * @param[in]  aFormat      A pointer to the format string.
 * @param[in]  ...          Arguments for the format specification.
 *
 */
#if OPENTHREAD_CONFIG_LOG_ARP == 1
#define otLogCritArp(aFormat, ...) otLogCrit(OT_LOG_REGION_ARP, _OT_REGION_ARP_PREFIX aFormat, ##__VA_ARGS__)
#define otLogWarnArp(aFormat, ...) otLogWarn(OT_LOG_REGION_ARP, _OT_REGION_ARP_PREFIX aFormat, ##__VA_ARGS__)
#define otLogNoteArp(aFormat, ...) otLogNote(OT_LOG_REGION_ARP, _OT_REGION_ARP_PREFIX aFormat, ##__VA_ARGS__)
#define otLogInfoArp(aFormat, ...) otLogInfo(OT_LOG_REGION_ARP, _OT_REGION_ARP_PREFIX aFormat, ##__VA_ARGS__)
#define otLogDebgArp(aFormat, ...) otLogDebg(OT_LOG_REGION_ARP, _OT_REGION_ARP_PREFIX aFormat, ##__VA_ARGS__)
#else
#define otLogCritArp(aFormat, ...)
#define otLogWarnArp(aFormat, ...)
#define otLogNoteArp(aFormat, ...)
#define otLogInfoArp(aFormat, ...)
#define otLogDebgArp(aFormat, ...)
#endif

/**
 * @def otLogCritNetData
 *
 * This method generates a log with level critical for the Network Data region.
 *
 * @param[in]  aFormat      A pointer to the format string.
 * @param[in]  ...          Arguments for the format specification.
 *
 */

/**
 * @def otLogWarnNetData
 *
 * This method generates a log with level warning for the Network Data region.
 *
 * @param[in]  aFormat      A pointer to the format string.
 * @param[in]  ...          Arguments for the format specification.
 *
 */

/**
 * @def otLogInfoNetData
 *
 * This method generates a log with level note for the Network Data region.
 *
 * @param[in]  aFormat      A pointer to the format string.
 * @param[in]  ...          Arguments for the format specification.
 *
 */

/**
 * @def otLogInfoNetData
 *
 * This method generates a log with level info for the Network Data region.
 *
 * @param[in]  aFormat      A pointer to the format string.
 * @param[in]  ...          Arguments for the format specification.
 *
 */

/**
 * @def otLogDebgNetData
 *
 * This method generates a log with level debug for the Network Data region.
 *
 * @param[in]  aFormat      A pointer to the format string.
 * @param[in]  ...          Arguments for the format specification.
 *
 */
#if OPENTHREAD_CONFIG_LOG_NETDATA == 1
#define otLogCritNetData(aFormat, ...) \
    otLogCrit(OT_LOG_REGION_NET_DATA, _OT_REGION_NET_DATA_PREFIX aFormat, ##__VA_ARGS__)
#define otLogWarnNetData(aFormat, ...) \
    otLogWarn(OT_LOG_REGION_NET_DATA, _OT_REGION_NET_DATA_PREFIX aFormat, ##__VA_ARGS__)
#define otLogNoteNetData(aFormat, ...) \
    otLogNote(OT_LOG_REGION_NET_DATA, _OT_REGION_NET_DATA_PREFIX aFormat, ##__VA_ARGS__)
#define otLogInfoNetData(aFormat, ...) \
    otLogInfo(OT_LOG_REGION_NET_DATA, _OT_REGION_NET_DATA_PREFIX aFormat, ##__VA_ARGS__)
#define otLogDebgNetData(aFormat, ...) \
    otLogDebg(OT_LOG_REGION_NET_DATA, _OT_REGION_NET_DATA_PREFIX aFormat, ##__VA_ARGS__)
#else
#define otLogCritNetData(aFormat, ...)
#define otLogWarnNetData(aFormat, ...)
#define otLogNoteNetData(aFormat, ...)
#define otLogInfoNetData(aFormat, ...)
#define otLogDebgNetData(aFormat, ...)
#endif

/**
 * @def otLogCritIcmp
 *
 * This method generates a log with level critical for the ICMPv6 region.
 *
 * @param[in]  aFormat      A pointer to the format string.
 * @param[in]  ...          Arguments for the format specification.
 *
 */

/**
 * @def otLogWarnIcmp
 *
 * This method generates a log with level warning for the ICMPv6 region.
 *
 * @param[in]  aFormat      A pointer to the format string.
 * @param[in]  ...          Arguments for the format specification.
 *
 */

/**
 * @def otLogNoteIcmp
 *
 * This method generates a log with level note for the ICMPv6 region.
 *
 * @param[in]  aFormat      A pointer to the format string.
 * @param[in]  ...          Arguments for the format specification.
 *
 */

/**
 * @def otLogInfoIcmp
 *
 * This method generates a log with level info for the ICMPv6 region.
 *
 * @param[in]  aFormat      A pointer to the format string.
 * @param[in]  ...          Arguments for the format specification.
 *
 */

/**
 * @def otLogDebgIcmp
 *
 * This method generates a log with level debug for the ICMPv6 region.
 *
 * @param[in]  aFormat      A pointer to the format string.
 * @param[in]  ...          Arguments for the format specification.
 *
 */
#if OPENTHREAD_CONFIG_LOG_ICMP == 1
#define otLogCritIcmp(aFormat, ...) otLogCrit(OT_LOG_REGION_ICMP, _OT_REGION_ICMP_PREFIX aFormat, ##__VA_ARGS__)
#define otLogWarnIcmp(aFormat, ...) otLogWarn(OT_LOG_REGION_ICMP, _OT_REGION_ICMP_PREFIX aFormat, ##__VA_ARGS__)
#define otLogNoteIcmp(aFormat, ...) otLogNote(OT_LOG_REGION_ICMP, _OT_REGION_ICMP_PREFIX aFormat, ##__VA_ARGS__)
#define otLogInfoIcmp(aFormat, ...) otLogInfo(OT_LOG_REGION_ICMP, _OT_REGION_ICMP_PREFIX aFormat, ##__VA_ARGS__)
#define otLogDebgIcmp(aFormat, ...) otLogDebg(OT_LOG_REGION_ICMP, _OT_REGION_ICMP_PREFIX aFormat, ##__VA_ARGS__)
#else
#define otLogCritIcmp(aFormat, ...)
#define otLogWarnIcmp(aFormat, ...)
#define otLogNoteIcmp(aFormat, ...)
#define otLogInfoIcmp(aFormat, ...)
#define otLogDebgIcmp(aFormat, ...)
#endif

/**
 * @def otLogCritIp6
 *
 * This method generates a log with level critical for the IPv6 region.
 *
 * @param[in]  aFormat      A pointer to the format string.
 * @param[in]  ...          Arguments for the format specification.
 *
 */

/**
 * @def otLogWarnIp6
 *
 * This method generates a log with level warning for the IPv6 region.
 *
 * @param[in]  aFormat      A pointer to the format string.
 * @param[in]  ...          Arguments for the format specification.
 *
 */

/**
 * @def otLogNoteIp6
 *
 * This method generates a log with level note for the IPv6 region.
 *
 * @param[in]  aFormat      A pointer to the format string.
 * @param[in]  ...          Arguments for the format specification.
 *
 */

/**
 * @def otLogInfoIp6
 *
 * This method generates a log with level info for the IPv6 region.
 *
 * @param[in]  aFormat      A pointer to the format string.
 * @param[in]  ...          Arguments for the format specification.
 *
 */

/**
 * @def otLogDebgIp6
 *
 * This method generates a log with level debug for the IPv6 region.
 *
 * @param[in]  aFormat      A pointer to the format string.
 * @param[in]  ...          Arguments for the format specification.
 *
 */
#if OPENTHREAD_CONFIG_LOG_IP6 == 1
#define otLogCritIp6(aFormat, ...) otLogCrit(OT_LOG_REGION_IP6, _OT_REGION_IP6_PREFIX aFormat, ##__VA_ARGS__)
#define otLogWarnIp6(aFormat, ...) otLogWarn(OT_LOG_REGION_IP6, _OT_REGION_IP6_PREFIX aFormat, ##__VA_ARGS__)
#define otLogNoteIp6(aFormat, ...) otLogNote(OT_LOG_REGION_IP6, _OT_REGION_IP6_PREFIX aFormat, ##__VA_ARGS__)
#define otLogInfoIp6(aFormat, ...) otLogInfo(OT_LOG_REGION_IP6, _OT_REGION_IP6_PREFIX aFormat, ##__VA_ARGS__)
#define otLogDebgIp6(aFormat, ...) otLogDebg(OT_LOG_REGION_IP6, _OT_REGION_IP6_PREFIX aFormat, ##__VA_ARGS__)
#else
#define otLogCritIp6(aFormat, ...)
#define otLogWarnIp6(aFormat, ...)
#define otLogNoteIp6(aFormat, ...)
#define otLogInfoIp6(aFormat, ...)
#define otLogDebgIp6(aFormat, ...)
#endif

/**
 * @def otLogCritMac
 *
 * This method generates a log with level critical for the MAC region.
 *
 * @param[in]  aFormat      A pointer to the format string.
 * @param[in]  ...          Arguments for the format specification.
 *
 */

/**
 * @def otLogWarnMac
 *
 * This method generates a log with level warning for the MAC region.
 *
 * @param[in]  aFormat      A pointer to the format string.
 * @param[in]  ...          Arguments for the format specification.
 *
 */

/**
 * @def otLogNoteMac
 *
 * This method generates a log with level note for the MAC region.
 *
 * @param[in]  aFormat      A pointer to the format string.
 * @param[in]  ...          Arguments for the format specification.
 *
 */

/**
 * @def otLogInfoMac
 *
 * This method generates a log with level info for the MAC region.
 *
 * @param[in]  aFormat      A pointer to the format string.
 * @param[in]  ...          Arguments for the format specification.
 *
 */

/**
 * @def otLogDebgMac
 *
 * This method generates a log with level debug for the MAC region.
 *
 * @param[in]  aFormat      A pointer to the format string.
 * @param[in]  ...          Arguments for the format specification.
 *
 */

/**
 * @def otLogMac
 *
 * This method generates a log with a given log level for the MAC region.
 *
 * @param[in]  aLogLevel    A log level.
 * @param[in]  aFormat      A pointer to the format string.
 * @param[in]  ...          Arguments for the format specification.
 *
 */
#if OPENTHREAD_CONFIG_LOG_MAC == 1
#define otLogCritMac(aFormat, ...) otLogCrit(OT_LOG_REGION_MAC, _OT_REGION_MAC_PREFIX aFormat, ##__VA_ARGS__)
#define otLogWarnMac(aFormat, ...) otLogWarn(OT_LOG_REGION_MAC, _OT_REGION_MAC_PREFIX aFormat, ##__VA_ARGS__)
#define otLogNoteMac(aFormat, ...) otLogNote(OT_LOG_REGION_MAC, _OT_REGION_MAC_PREFIX aFormat, ##__VA_ARGS__)
#define otLogInfoMac(aFormat, ...) otLogInfo(OT_LOG_REGION_MAC, _OT_REGION_MAC_PREFIX aFormat, ##__VA_ARGS__)
#define otLogDebgMac(aFormat, ...) otLogDebg(OT_LOG_REGION_MAC, _OT_REGION_MAC_PREFIX aFormat, ##__VA_ARGS__)
#define otLogDebgMacErr(aError, aFormat, ...)                                                               \
    otLogWarn(OT_LOG_REGION_MAC, _OT_REGION_MAC_PREFIX "Error %s: " aFormat, otThreadErrorToString(aError), \
              ##__VA_ARGS__)
#define otLogMac(aLogLevel, aFormat, ...)                                                     \
    do                                                                                        \
    {                                                                                         \
        if (otLoggingGetLevel() >= aLogLevel)                                                 \
        {                                                                                     \
            _otLogFormatter(aLogLevel, OT_LOG_REGION_MAC, "%s" _OT_REGION_MAC_PREFIX aFormat, \
                            otLogLevelToPrefixString(aLogLevel), ##__VA_ARGS__);              \
        }                                                                                     \
    } while (false)

#else
#define otLogCritMac(aFormat, ...)
#define otLogWarnMac(aFormat, ...)
#define otLogNoteMac(aFormat, ...)
#define otLogInfoMac(aFormat, ...)
#define otLogDebgMac(aFormat, ...)
#define otLogDebgMacErr(aError, aFormat, ...)
#define otLogMac(aLogLevel, aFormat, ...)
#endif

/**
 * @def otLogCritCore
 *
 * This method generates a log with level critical for the Core region.
 *
 * @param[in]  aFormat      A pointer to the format string.
 * @param[in]  ...          Arguments for the format specification.
 *
 */

/**
 * @def otLogWarnCore
 *
 * This method generates a log with level warning for the Core region.
 *
 * @param[in]  aFormat      A pointer to the format string.
 * @param[in]  ...          Arguments for the format specification.
 *
 */

/**
 * @def otLogNoteCore
 *
 * This method generates a log with level note for the Core region.
 *
 * @param[in]  aFormat      A pointer to the format string.
 * @param[in]  ...          Arguments for the format specification.
 *
 */

/**
 * @def otLogInfoCore
 *
 * This method generates a log with level info for the Core region.
 *
 * @param[in]  aFormat      A pointer to the format string.
 * @param[in]  ...          Arguments for the format specification.
 *
 */

/**
 * @def otLogDebgCore
 *
 * This method generates a log with level debug for the Core region.
 *
 * @param[in]  aFormat      A pointer to the format string.
 * @param[in]  ...          Arguments for the format specification.
 *
 */
#if OPENTHREAD_CONFIG_LOG_CORE == 1
#define otLogCritCore(aFormat, ...) otLogCrit(OT_LOG_REGION_CORE, _OT_REGION_CORE_PREFIX aFormat, ##__VA_ARGS__)
#define otLogWarnCore(aFormat, ...) otLogWarn(OT_LOG_REGION_CORE, _OT_REGION_CORE_PREFIX aFormat, ##__VA_ARGS__)
#define otLogNoteCore(aFormat, ...) otLogNote(OT_LOG_REGION_CORE, _OT_REGION_CORE_PREFIX aFormat, ##__VA_ARGS__)
#define otLogInfoCore(aFormat, ...) otLogInfo(OT_LOG_REGION_CORE, _OT_REGION_CORE_PREFIX aFormat, ##__VA_ARGS__)
#define otLogDebgCore(aFormat, ...) otLogDebg(OT_LOG_REGION_CORE, _OT_REGION_CORE_PREFIX aFormat, ##__VA_ARGS__)
#define otLogDebgCoreErr(aError, aFormat, ...)                                                                \
    otLogWarn(OT_LOG_REGION_CORE, _OT_REGION_CORE_PREFIX "Error %s: " aFormat, otThreadErrorToString(aError), \
              ##__VA_ARGS__)
#else
#define otLogCritCore(aFormat, ...)
#define otLogWarnCore(aFormat, ...)
#define otLogInfoCore(aFormat, ...)
#define otLogDebgCore(aFormat, ...)
#define otLogDebgCoreErr(aError, aFormat, ...)
#endif

/**
 * @def otLogCritMem
 *
 * This method generates a log with level critical for the memory region.
 *
 * @param[in]  aFormat      A pointer to the format string.
 * @param[in]  ...          Arguments for the format specification.
 *
 */

/**
 * @def otLogWarnMem
 *
 * This method generates a log with level warning for the memory region.
 *
 * @param[in]  aFormat      A pointer to the format string.
 * @param[in]  ...          Arguments for the format specification.
 *
 */

/**
 * @def otLogNoteMem
 *
 * This method generates a log with level note for the memory region.
 *
 * @param[in]  aFormat      A pointer to the format string.
 * @param[in]  ...          Arguments for the format specification.
 *
 */

/**
 * @def otLogInfoMem
 *
 * This method generates a log with level info for the memory region.
 *
 * @param[in]  aFormat      A pointer to the format string.
 * @param[in]  ...          Arguments for the format specification.
 *
 */

/**
 * @def otLogDebgMem
 *
 * This method generates a log with level debug for the memory region.
 *
 * @param[in]  aFormat      A pointer to the format string.
 * @param[in]  ...          Arguments for the format specification.
 *
 */
#if OPENTHREAD_CONFIG_LOG_MEM == 1
#define otLogCritMem(aFormat, ...) otLogCrit(OT_LOG_REGION_MEM, _OT_REGION_MEM_PREFIX aFormat, ##__VA_ARGS__)
#define otLogWarnMem(aFormat, ...) otLogWarn(OT_LOG_REGION_MEM, _OT_REGION_MEM_PREFIX aFormat, ##__VA_ARGS__)
#define otLogNoteMem(aFormat, ...) otLogNote(OT_LOG_REGION_MEM, _OT_REGION_MEM_PREFIX aFormat, ##__VA_ARGS__)
#define otLogInfoMem(aFormat, ...) otLogInfo(OT_LOG_REGION_MEM, _OT_REGION_MEM_PREFIX aFormat, ##__VA_ARGS__)
#define otLogDebgMem(aFormat, ...) otLogDebg(OT_LOG_REGION_MEM, _OT_REGION_MEM_PREFIX aFormat, ##__VA_ARGS__)
#else
#define otLogCritMem(aFormat, ...)
#define otLogWarnMem(aFormat, ...)
#define otLogNoteMem(aFormat, ...)
#define otLogInfoMem(aFormat, ...)
#define otLogDebgMem(aFormat, ...)
#endif

/**
 * @def otLogCritUtil
 *
 * This method generates a log with level critical for the Util region.
 *
 * @param[in]  aFormat      A pointer to the format string.
 * @param[in]  ...          Arguments for the format specification.
 *
 */

/**
 * @def otLogWarnUtil
 *
 * This method generates a log with level warning for the Util region.
 *
 * @param[in]  aFormat      A pointer to the format string.
 * @param[in]  ...          Arguments for the format specification.
 *
 */

/**
 * @def otLogNoteUtil
 *
 * This method generates a log with level note for the Util region.
 *
 * @param[in]  aFormat      A pointer to the format string.
 * @param[in]  ...          Arguments for the format specification.
 *
 */

/**
 * @def otLogInfoUtil
 *
 * This method generates a log with level info for the Util region.
 *
 * @param[in]  aFormat      A pointer to the format string.
 * @param[in]  ...          Arguments for the format specification.
 *
 */

/**
 * @def otLogDebgUtil
 *
 * This method generates a log with level debug for the Util region.
 *
 * @param[in]  aFormat      A pointer to the format string.
 * @param[in]  ...          Arguments for the format specification.
 *
 */
#if OPENTHREAD_CONFIG_LOG_UTIL == 1
#define otLogCritUtil(aFormat, ...) otLogCrit(OT_LOG_REGION_UTIL, _OT_REGION_UTIL_PREFIX aFormat, ##__VA_ARGS__)
#define otLogWarnUtil(aFormat, ...) otLogWarn(OT_LOG_REGION_UTIL, _OT_REGION_UTIL_PREFIX aFormat, ##__VA_ARGS__)
#define otLogNoteUtil(aFormat, ...) otLogNote(OT_LOG_REGION_UTIL, _OT_REGION_UTIL_PREFIX aFormat, ##__VA_ARGS__)
#define otLogInfoUtil(aFormat, ...) otLogInfo(OT_LOG_REGION_UTIL, _OT_REGION_UTIL_PREFIX aFormat, ##__VA_ARGS__)
#define otLogInfoUtilErr(aError, aFormat, ...)                                                                \
    otLogInfo(OT_LOG_REGION_UTIL, _OT_REGION_CORE_PREFIX "Error %s: " aFormat, otThreadErrorToString(aError), \
              ##__VA_ARGS__)
#define otLogDebgUtil(aFormat, ...) otLogDebg(OT_LOG_REGION_UTIL, _OT_REGION_UTIL_PREFIX aFormat, ##__VA_ARGS__)
#else
#define otLogCritUtil(aFormat, ...)
#define otLogWarnUtil(aFormat, ...)
#define otLogNoteUtil(aFormat, ...)
#define otLogInfoUtil(aFormat, ...)
#define otLogInfoUtilErr(aError, aFormat, ...)
#define otLogDebgUtil(aFormat, ...)
#endif

/**
 * @def otLogCritNetDiag
 *
 * This method generates a log with level critical for the NETDIAG region.
 *
 * @param[in]  aFormat      A pointer to the format string.
 * @param[in]  ...          Arguments for the format specification.
 *
 */

/**
 * @def otLogWarnNetDiag
 *
 * This method generates a log with level warning for the NETDIAG region.
 *
 * @param[in]  aFormat      A pointer to the format string.
 * @param[in]  ...          Arguments for the format specification.
 *
 */

/**
 * @def otLogNoteNetDiag
 *
 * This method generates a log with level note for the NETDIAG region.
 *
 * @param[in]  aFormat      A pointer to the format string.
 * @param[in]  ...          Arguments for the format specification.
 *
 */

/**
 * @def otLogInfoNetDiag
 *
 * This method generates a log with level info for the NETDIAG region.
 *
 * @param[in]  aFormat      A pointer to the format string.
 * @param[in]  ...          Arguments for the format specification.
 *
 */

/**
 * @def otLogDebgNetDiag
 *
 * This method generates a log with level debug for the NETDIAG region.
 *
 * @param[in]  aFormat      A pointer to the format string.
 * @param[in]  ...          Arguments for the format specification.
 *
 */
#if OPENTHREAD_CONFIG_LOG_NETDIAG == 1
#define otLogCritNetDiag(aFormat, ...) \
    otLogCrit(OT_LOG_REGION_NET_DIAG, _OT_REGION_NET_DIAG_PREFIX aFormat, ##__VA_ARGS__)
#define otLogWarnNetDiag(aFormat, ...) \
    otLogWarn(OT_LOG_REGION_NET_DIAG, _OT_REGION_NET_DIAG_PREFIX aFormat, ##__VA_ARGS__)
#define otLogNoteNetDiag(aFormat, ...) \
    otLogNote(OT_LOG_REGION_NET_DIAG, _OT_REGION_NET_DIAG_PREFIX aFormat, ##__VA_ARGS__)
#define otLogInfoNetDiag(aFormat, ...) \
    otLogInfo(OT_LOG_REGION_NET_DIAG, _OT_REGION_NET_DIAG_PREFIX aFormat, ##__VA_ARGS__)
#define otLogDebgNetDiag(aFormat, ...) \
    otLogDebg(OT_LOG_REGION_NET_DIAG, _OT_REGION_NET_DIAG_PREFIX aFormat, ##__VA_ARGS__)
#else
#define otLogCritNetDiag(aFormat, ...)
#define otLogWarnNetDiag(aFormat, ...)
#define otLogNoteNetDiag(aFormat, ...)
#define otLogInfoNetDiag(aFormat, ...)
#define otLogDebgNetDiag(aFormat, ...)
#endif

/**
 * @def otLogCert
 *
 * This method generates a log with level none for the certification test.
 *
 * @param[in]  aFormat      A pointer to the format string.
 * @param[in]  ...          Arguments for the format specification.
 *
 *
 */
#if OPENTHREAD_ENABLE_CERT_LOG
#define otLogCertMeshCoP(aFormat, ...) \
    _otLogFormatter(OT_LOG_LEVEL_NONE, OT_LOG_REGION_MESH_COP, aFormat, ##__VA_ARGS__)
#else
#define otLogCertMeshCoP(aFormat, ...)
#endif

/**
 * @def otLogCritCli
 *
 * This method generates a log with level critical for the CLI region.
 *
 * @param[in]  aFormat      A pointer to the format string.
 * @param[in]  ...          Arguments for the format specification.
 *
 */

/**
 * @def otLogWarnCli
 *
 * This method generates a log with level warning for the CLI region.
 *
 * @param[in]  aFormat      A pointer to the format string.
 * @param[in]  ...          Arguments for the format specification.
 *
 */

/**
 * @def otLogInfoCli
 *
 * This method generates a log with level note for the CLI region.
 *
 * @param[in]  aFormat      A pointer to the format string.
 * @param[in]  ...          Arguments for the format specification.
 *
 */

/**
 * @def otLogInfoCli
 *
 * This method generates a log with level info for the CLI region.
 *
 * @param[in]  aFormat      A pointer to the format string.
 * @param[in]  ...          Arguments for the format specification.
 *
 */

/**
 * @def otLogDebgCli
 *
 * This method generates a log with level debug for the CLI region.
 *
 * @param[in]  aFormat      A pointer to the format string.
 * @param[in]  ...          Arguments for the format specification.
 *
 */
#if OPENTHREAD_CONFIG_LOG_CLI == 1

#define otLogCritCli(aFormat, ...) otLogCrit(OT_LOG_REGION_CLI, _OT_REGION_CLI_PREFIX aFormat, ##__VA_ARGS__)
#define otLogWarnCli(aFormat, ...) otLogWarn(OT_LOG_REGION_CLI, _OT_REGION_CLI_PREFIX aFormat, ##__VA_ARGS__)
#define otLogNoteCli(aFormat, ...) otLogNote(OT_LOG_REGION_CLI, _OT_REGION_CLI_PREFIX aFormat, ##__VA_ARGS__)
#define otLogInfoCli(aFormat, ...) otLogInfo(OT_LOG_REGION_CLI, _OT_REGION_CLI_PREFIX aFormat, ##__VA_ARGS__)
#define otLogInfoCliErr(aError, aFormat, ...) \
    otLogInfo(OT_LOG_REGION_CLI, "Error %s: " aFormat, otThreadErrorToString(aError), ##__VA_ARGS__)
#define otLogDebgCli(aFormat, ...) otLogDebg(OT_LOG_REGION_CLI, _OT_REGION_CLI_PREFIX aFormat, ##__VA_ARGS__)
#else
#define otLogCritCli(aFormat, ...)
#define otLogWarnCli(aFormat, ...)
#define otLogNoteCli(aFormat, ...)
#define otLogInfoCli(aFormat, ...)
#define otLogInfoCliErr(aError, aFormat, ...)
#define otLogDebgCli(aFormat, ...)
#endif

/**
 * @def otLogCritCoap
 *
 * This method generates a log with level critical for the CoAP region.
 *
 * @param[in]  aFormat      A pointer to the format string.
 * @param[in]  ...          Arguments for the format specification.
 *
 */

/**
 * @def otLogWarnCoap
 *
 * This method generates a log with level warning for the CoAP region.
 *
 * @param[in]  aFormat      A pointer to the format string.
 * @param[in]  ...          Arguments for the format specification.
 *
 */

/**
 * @def otLogNoteCoap
 *
 * This method generates a log with level note for the CoAP region.
 *
 * @param[in]  aFormat      A pointer to the format string.
 * @param[in]  ...          Arguments for the format specification.
 *
 */

/**
 * @def otLogInfoCoap
 *
 * This method generates a log with level info for the CoAP region.
 *
 * @param[in]  aFormat      A pointer to the format string.
 * @param[in]  ...          Arguments for the format specification.
 *
 */

/**
 * @def otLogDebgCoap
 *
 * This method generates a log with level debug for the CoAP region.
 *
 * @param[in]  aFormat      A pointer to the format string.
 * @param[in]  ...          Arguments for the format specification.
 *
 */
#if OPENTHREAD_CONFIG_LOG_COAP == 1
#define otLogCritCoap(aFormat, ...) otLogCrit(OT_LOG_REGION_COAP, _OT_REGION_COAP_PREFIX aFormat, ##__VA_ARGS__)
#define otLogWarnCoap(aFormat, ...) otLogWarn(OT_LOG_REGION_COAP, _OT_REGION_COAP_PREFIX aFormat, ##__VA_ARGS__)
#define otLogNoteCoap(aFormat, ...) otLogNote(OT_LOG_REGION_COAP, _OT_REGION_COAP_PREFIX aFormat, ##__VA_ARGS__)
#define otLogInfoCoap(aFormat, ...) otLogInfo(OT_LOG_REGION_COAP, _OT_REGION_COAP_PREFIX aFormat, ##__VA_ARGS__)
#define otLogInfoCoapErr(aError, aFormat, ...)                                                                \
    otLogInfo(OT_LOG_REGION_COAP, _OT_REGION_COAP_PREFIX "Error %s: " aFormat, otThreadErrorToString(aError), \
              ##__VA_ARGS__)
#define otLogDebgCoap(aFormat, ...) otLogDebg(OT_LOG_REGION_COAP, _OT_REGION_COAP_PREFIX aFormat, ##__VA_ARGS__)
#else
#define otLogCritCoap(aFormat, ...)
#define otLogWarnCoap(aFormat, ...)
#define otLogNoteCoap(aFormat, ...)
#define otLogInfoCoap(aFormat, ...)
#define otLogInfoCoapErr(aError, aFormat, ...)
#define otLogDebgCoap(aFormat, ...)
#endif

/**
 * @def otLogCritPlat
 *
 * This method generates a log with level critical for the Platform region.
 *
 * @param[in]  aFormat   A pointer to the format string.
 * @param[in]  ...       Arguments for the format specification.
 *
 */

/**
 * @def otLogWarnPlat
 *
 * This method generates a log with level warning for the Platform region.
 *
 * @param[in]  aFormat   A pointer to the format string.
 * @param[in]  ...       Arguments for the format specification.
 *
 */

/**
 * @def otLogNotePlat
 *
 * This method generates a log with level note for the Platform region.
 *
 * @param[in]  aFormat   A pointer to the format string.
 * @param[in]  ...       Arguments for the format specification.
 *
 */

/**
 * @def otLogInfoPlat
 *
 * This method generates a log with level info for the Platform region.
 *
 * @param[in]  aFormat   A pointer to the format string.
 * @param[in]  ...       Arguments for the format specification.
 *
 */

/**
 * @def otLogDebgPlat
 *
 * This method generates a log with level debug for the Platform region.
 *
 * @param[in]  aFormat   A pointer to the format string.
 * @param[in]  ...       Arguments for the format specification.
 *
 */
#if OPENTHREAD_CONFIG_LOG_PLATFORM == 1
#define otLogCritPlat(aFormat, ...) otLogCrit(OT_LOG_REGION_PLATFORM, _OT_REGION_PLATFORM_PREFIX aFormat, ##__VA_ARGS__)
#define otLogWarnPlat(aFormat, ...) otLogWarn(OT_LOG_REGION_PLATFORM, _OT_REGION_PLATFORM_PREFIX aFormat, ##__VA_ARGS__)
#define otLogNotePlat(aFormat, ...) otLogNote(OT_LOG_REGION_PLATFORM, _OT_REGION_PLATFORM_PREFIX aFormat, ##__VA_ARGS__)
#define otLogInfoPlat(aFormat, ...) otLogInfo(OT_LOG_REGION_PLATFORM, _OT_REGION_PLATFORM_PREFIX aFormat, ##__VA_ARGS__)
#define otLogDebgPlat(aFormat, ...) otLogDebg(OT_LOG_REGION_PLATFORM, _OT_REGION_PLATFORM_PREFIX aFormat, ##__VA_ARGS__)
#else
#define otLogCritPlat(aFormat, ...)
#define otLogWarnPlat(aFormat, ...)
#define otLogNotePlat(aFormat, ...)
#define otLogInfoPlat(aFormat, ...)
#define otLogDebgPlat(aFormat, ...)
#endif

#endif // WINDOWS_LOGGING

/**
 * @def otDumpCrit
 *
 * This method generates a memory dump with log level critical.
 *
 * @param[in]  aRegion      The log region.
 * @param[in]  aId          A pointer to a NULL-terminated string that is printed before the bytes.
 * @param[in]  aBuf         A pointer to the buffer.
 * @param[in]  aLength      Number of bytes to print.
 *
 */
#if OPENTHREAD_CONFIG_LOG_LEVEL >= OT_LOG_LEVEL_CRIT
#define otDumpCrit(aRegion, aId, aBuf, aLength) otDump(OT_LOG_LEVEL_CRIT, aRegion, aId, aBuf, aLength)
#else
#define otDumpCrit(aRegion, aId, aBuf, aLength)
#endif

/**
 * @def otDumpWarn
 *
 * This method generates a memory dump with log level warning.
 *
 * @param[in]  aRegion      The log region.
 * @param[in]  aId          A pointer to a NULL-terminated string that is printed before the bytes.
 * @param[in]  aBuf         A pointer to the buffer.
 * @param[in]  aLength      Number of bytes to print.
 *
 */
#if OPENTHREAD_CONFIG_LOG_LEVEL >= OT_LOG_LEVEL_WARN
#define otDumpWarn(aRegion, aId, aBuf, aLength) otDump(OT_LOG_LEVEL_WARN, aRegion, aId, aBuf, aLength)
#else
#define otDumpWarn(aRegion, aId, aBuf, aLength)
#endif

/**
 * @def otDumpNote
 *
 * This method generates a memory dump with log level note.
 *
 * @param[in]  aRegion      The log region.
 * @param[in]  aId          A pointer to a NULL-terminated string that is printed before the bytes.
 * @param[in]  aBuf         A pointer to the buffer.
 * @param[in]  aLength      Number of bytes to print.
 *
 */
#if OPENTHREAD_CONFIG_LOG_LEVEL >= OT_LOG_LEVEL_NOTE
#define otDumpNote(aRegion, aId, aBuf, aLength) otDump(OT_LOG_LEVEL_NOTE, aRegion, aId, aBuf, aLength)
#else
#define otDumpInfo(aRegion, aId, aBuf, aLength)
#endif

/**
 * @def otDumpInfo
 *
 * This method generates a memory dump with log level info.
 *
 * @param[in]  aRegion      The log region.
 * @param[in]  aId          A pointer to a NULL-terminated string that is printed before the bytes.
 * @param[in]  aBuf         A pointer to the buffer.
 * @param[in]  aLength      Number of bytes to print.
 *
 */
#if OPENTHREAD_CONFIG_LOG_LEVEL >= OT_LOG_LEVEL_INFO
#define otDumpInfo(aRegion, aId, aBuf, aLength) otDump(OT_LOG_LEVEL_INFO, aRegion, aId, aBuf, aLength)
#else
#define otDumpInfo(aRegion, aId, aBuf, aLength)
#endif

/**
 * @def otDumpDebg
 *
 * This method generates a memory dump with log level debug.
 *
 * @param[in]  aRegion      The log region.
 * @param[in]  aId          A pointer to a NULL-terminated string that is printed before the bytes.
 * @param[in]  aBuf         A pointer to the buffer.
 * @param[in]  aLength      Number of bytes to print.
 *
 */
#if OPENTHREAD_CONFIG_LOG_LEVEL >= OT_LOG_LEVEL_DEBG
#define otDumpDebg(aRegion, aId, aBuf, aLength) otDump(OT_LOG_LEVEL_DEBG, aRegion, aId, aBuf, aLength)
#else
#define otDumpDebg(aRegion, aId, aBuf, aLength)
#endif

/**
 * @def otDumpCritNetData
 *
 * This method generates a memory dump with log level debug and region Network Data.
 *
 * @param[in]  aId          A pointer to a NULL-terminated string that is printed before the bytes.
 * @param[in]  aBuf         A pointer to the buffer.
 * @param[in]  aLength      Number of bytes to print.
 *
 */

/**
 * @def otDumpWarnNetData
 *
 * This method generates a memory dump with log level warning and region Network Data.
 *
 * @param[in]  aBuf         A pointer to the buffer.
 * @param[in]  aLength      Number of bytes to print.
 *
 */

/**
 * @def otDumpNoteNetData
 *
 * @param[in]  aId          A pointer to a NULL-terminated string that is printed before the bytes.
 * @param[in]  aBuf         A pointer to the buffer.
 * @param[in]  aLength      Number of bytes to print.
 *
 */

/**
 * @def otDumpInfoNetData
 *
 * @param[in]  aId          A pointer to a NULL-terminated string that is printed before the bytes.
 * @param[in]  aBuf         A pointer to the buffer.
 * @param[in]  aLength      Number of bytes to print.
 *
 */

/**
 * @def otDumpDebgNetData
 *
 * This method generates a memory dump with log level debug and region Network Data.
 *
 * @param[in]  aId          A pointer to a NULL-terminated string that is printed before the bytes.
 * @param[in]  aBuf         A pointer to the buffer.
 * @param[in]  aLength      Number of bytes to print.
 *
 */
#if OPENTHREAD_CONFIG_LOG_NETDATA == 1
#define otDumpCritNetData(aId, aBuf, aLength) otDumpCrit(OT_LOG_REGION_NET_DATA, aId, aBuf, aLength)
#define otDumpWarnNetData(aId, aBuf, aLength) otDumpWarn(OT_LOG_REGION_NET_DATA, aId, aBuf, aLength)
#define otDumpNoteNetData(aId, aBuf, aLength) otDumpNote(OT_LOG_REGION_NET_DATA, aId, aBuf, aLength)
#define otDumpInfoNetData(aId, aBuf, aLength) otDumpInfo(OT_LOG_REGION_NET_DATA, aId, aBuf, aLength)
#define otDumpDebgNetData(aId, aBuf, aLength) otDumpDebg(OT_LOG_REGION_NET_DATA, aId, aBuf, aLength)
#else
#define otDumpCritNetData(aId, aBuf, aLength)
#define otDumpWarnNetData(aId, aBuf, aLength)
#define otDumpNoteNetData(aId, aBuf, aLength)
#define otDumpInfoNetData(aId, aBuf, aLength)
#define otDumpDebgNetData(aId, aBuf, aLength)
#endif

/**
 * @def otDumpCritMle
 *
 * This method generates a memory dump with log level debug and region MLE.
 *
 * @param[in]  aId          A pointer to a NULL-terminated string that is printed before the bytes.
 * @param[in]  aBuf         A pointer to the buffer.
 * @param[in]  aLength      Number of bytes to print.
 *
 */

/**
 * @def otDumpWarnMle
 *
 * This method generates a memory dump with log level warning and region MLE.
 *
 * @param[in]  aId          A pointer to a NULL-terminated string that is printed before the bytes.
 * @param[in]  aBuf         A pointer to the buffer.
 * @param[in]  aLength      Number of bytes to print.
 *
 */

/**
 * @def otDumpNoteMle
 *
 * This method generates a memory dump with log level note and region MLE.
 *
 * @param[in]  aId          A pointer to a NULL-terminated string that is printed before the bytes.
 * @param[in]  aBuf         A pointer to the buffer.
 * @param[in]  aLength      Number of bytes to print.
 *
 */

/**
 * @def otDumpInfoMle
 *
 * This method generates a memory dump with log level info and region MLE.
 *
 * @param[in]  aId          A pointer to a NULL-terminated string that is printed before the bytes.
 * @param[in]  aBuf         A pointer to the buffer.
 * @param[in]  aLength      Number of bytes to print.
 *
 */

/**
 * @def otDumpDebgMle
 *
 * This method generates a memory dump with log level debug and region MLE.
 *
 * @param[in]  aId          A pointer to a NULL-terminated string that is printed before the bytes.
 * @param[in]  aBuf         A pointer to the buffer.
 * @param[in]  aLength      Number of bytes to print.
 *
 */
#if OPENTHREAD_CONFIG_LOG_MLE == 1
#define otDumpCritMle(aId, aBuf, aLength) otDumpCrit(OT_LOG_REGION_MLE, aId, aBuf, aLength)
#define otDumpWarnMle(aId, aBuf, aLength) otDumpWarn(OT_LOG_REGION_MLE, aId, aBuf, aLength)
#define otDumpNoteMle(aId, aBuf, aLength) otDumpNote(OT_LOG_REGION_MLE, aId, aBuf, aLength)
#define otDumpInfoMle(aId, aBuf, aLength) otDumpInfo(OT_LOG_REGION_MLE, aId, aBuf, aLength)
#define otDumpDebgMle(aId, aBuf, aLength) otDumpDebg(OT_LOG_REGION_MLE, aId, aBuf, aLength)
#else
#define otDumpCritMle(aId, aBuf, aLength)
#define otDumpWarnMle(aId, aBuf, aLength)
#define otDumpNoteMle(aId, aBuf, aLength)
#define otDumpInfoMle(aId, aBuf, aLength)
#define otDumpDebgMle(aId, aBuf, aLength)
#endif

/**
 * @def otDumpCritArp
 *
 * This method generates a memory dump with log level debug and region EID-to-RLOC mapping.
 *
 * @param[in]  aId          A pointer to a NULL-terminated string that is printed before the bytes.
 * @param[in]  aBuf         A pointer to the buffer.
 * @param[in]  aLength      Number of bytes to print.
 *
 */

/**
 * @def otDumpWarnArp
 *
 * This method generates a memory dump with log level warning and region EID-to-RLOC mapping.
 *
 * @param[in]  aId          A pointer to a NULL-terminated string that is printed before the bytes.
 * @param[in]  aBuf         A pointer to the buffer.
 * @param[in]  aLength      Number of bytes to print.
 *
 */

/**
 * @def otDumpNoteArp
 *
 * This method generates a memory dump with log level note and region EID-to-RLOC mapping.
 *
 * @param[in]  aId          A pointer to a NULL-terminated string that is printed before the bytes.
 * @param[in]  aBuf         A pointer to the buffer.
 * @param[in]  aLength      Number of bytes to print.
 *
 */

/**
 * @def otDumpInfoArp
 *
 * This method generates a memory dump with log level info and region EID-to-RLOC mapping.
 *
 * @param[in]  aId          A pointer to a NULL-terminated string that is printed before the bytes.
 * @param[in]  aBuf         A pointer to the buffer.
 * @param[in]  aLength      Number of bytes to print.
 *
 */

/**
 * @def otDumpDebgArp
 *
 * This method generates a memory dump with log level debug and region EID-to-RLOC mapping.
 *
 * @param[in]  aId          A pointer to a NULL-terminated string that is printed before the bytes.
 * @param[in]  aBuf         A pointer to the buffer.
 * @param[in]  aLength      Number of bytes to print.
 *
 */
#if OPENTHREAD_CONFIG_LOG_ARP == 1
#define otDumpCritArp(aId, aBuf, aLength) otDumpCrit(OT_LOG_REGION_ARP, aId, aBuf, aLength)
#define otDumpWarnArp(aId, aBuf, aLength) otDumpWarn(OT_LOG_REGION_ARP, aId, aBuf, aLength)
#define otDumpNoteArp(aId, aBuf, aLength) otDumpNote(OT_LOG_REGION_ARP, aId, aBuf, aLength)
#define otDumpInfoArp(aId, aBuf, aLength) otDumpInfo(OT_LOG_REGION_ARP, aId, aBuf, aLength)
#define otDumpDebgArp(aId, aBuf, aLength) otDumpDebg(OT_LOG_REGION_ARP, aId, aBuf, aLength)
#else
#define otDumpCritArp(aId, aBuf, aLength)
#define otDumpWarnArp(aId, aBuf, aLength)
#define otDumpNoteArp(aId, aBuf, aLength)
#define otDumpInfoArp(aId, aBuf, aLength)
#define otDumpDebgArp(aId, aBuf, aLength)
#endif

/**
 * @def otDumpCritIcmp
 *
 * This method generates a memory dump with log level debug and region ICMPv6.
 *
 * @param[in]  aId          A pointer to a NULL-terminated string that is printed before the bytes.
 * @param[in]  aBuf         A pointer to the buffer.
 * @param[in]  aLength      Number of bytes to print.
 *
 */

/**
 * @def otDumpWarnIcmp
 *
 * This method generates a memory dump with log level warning and region ICMPv6.
 *
 * @param[in]  aId          A pointer to a NULL-terminated string that is printed before the bytes.
 * @param[in]  aBuf         A pointer to the buffer.
 * @param[in]  aLength      Number of bytes to print.
 *
 */

/**
 * @def otDumpNoteIcmp
 *
 * This method generates a memory dump with log level note and region ICMPv6.
 *
 * @param[in]  aId          A pointer to a NULL-terminated string that is printed before the bytes.
 * @param[in]  aBuf         A pointer to the buffer.
 * @param[in]  aLength      Number of bytes to print.
 *
 */

/**
 * @def otDumpInfoIcmp
 *
 * This method generates a memory dump with log level info and region ICMPv6.
 *
 * @param[in]  aId          A pointer to a NULL-terminated string that is printed before the bytes.
 * @param[in]  aBuf         A pointer to the buffer.
 * @param[in]  aLength      Number of bytes to print.
 *
 */

/**
 * @def otDumpDebgIcmp
 *
 * This method generates a memory dump with log level debug and region ICMPv6.
 *
 * @param[in]  aId          A pointer to a NULL-terminated string that is printed before the bytes.
 * @param[in]  aBuf         A pointer to the buffer.
 * @param[in]  aLength      Number of bytes to print.
 *
 */
#if OPENTHREAD_CONFIG_LOG_ICMP == 1
#define otDumpCritIcmp(aId, aBuf, aLength) otDumpCrit(OT_LOG_REGION_ICMP, aId, aBuf, aLength)
#define otDumpWarnIcmp(aId, aBuf, aLength) otDumpWarn(OT_LOG_REGION_ICMP, aId, aBuf, aLength)
#define otDumpNoteIcmp(aId, aBuf, aLength) otDumpNote(OT_LOG_REGION_ICMP, aId, aBuf, aLength)
#define otDumpInfoIcmp(aId, aBuf, aLength) otDumpInfo(OT_LOG_REGION_ICMP, aId, aBuf, aLength)
#define otDumpDebgIcmp(aId, aBuf, aLength) otDumpDebg(OT_LOG_REGION_ICMP, aId, aBuf, aLength)
#else
#define otDumpCritIcmp(aId, aBuf, aLength)
#define otDumpWarnIcmp(aId, aBuf, aLength)
#define otDumpNoteIcmp(aId, aBuf, aLength)
#define otDumpInfoIcmp(aId, aBuf, aLength)
#define otDumpDebgIcmp(aId, aBuf, aLength)
#endif

/**
 * @def otDumpCritIp6
 *
 * This method generates a memory dump with log level debug and region IPv6.
 *
 * @param[in]  aId          A pointer to a NULL-terminated string that is printed before the bytes.
 * @param[in]  aBuf         A pointer to the buffer.
 * @param[in]  aLength      Number of bytes to print.
 *
 */

/**
 * @def otDumpWarnIp6
 *
 * This method generates a memory dump with log level warning and region IPv6.
 *
 * @param[in]  aId          A pointer to a NULL-terminated string that is printed before the bytes.
 * @param[in]  aBuf         A pointer to the buffer.
 * @param[in]  aLength      Number of bytes to print.
 *
 */

/**
 * @def otDumpNoteIp6
 *
 * This method generates a memory dump with log level note and region IPv6.
 *
 * @param[in]  aId          A pointer to a NULL-terminated string that is printed before the bytes.
 * @param[in]  aBuf         A pointer to the buffer.
 * @param[in]  aLength      Number of bytes to print.
 *
 */

/**
 * @def otDumpInfoIp6
 *
 * This method generates a memory dump with log level info and region IPv6.
 *
 * @param[in]  aId          A pointer to a NULL-terminated string that is printed before the bytes.
 * @param[in]  aBuf         A pointer to the buffer.
 * @param[in]  aLength      Number of bytes to print.
 *
 */

/**
 * @def otDumpDebgIp6
 *
 * This method generates a memory dump with log level debug and region IPv6.
 *
 * @param[in]  aId          A pointer to a NULL-terminated string that is printed before the bytes.
 * @param[in]  aBuf         A pointer to the buffer.
 * @param[in]  aLength      Number of bytes to print.
 *
 */
#if OPENTHREAD_CONFIG_LOG_IP6 == 1
#define otDumpCritIp6(aId, aBuf, aLength) otDumpCrit(OT_LOG_REGION_IP6, aId, aBuf, aLength)
#define otDumpWarnIp6(aId, aBuf, aLength) otDumpWarn(OT_LOG_REGION_IP6, aId, aBuf, aLength)
#define otDumpNoteIp6(aId, aBuf, aLength) otDumpNote(OT_LOG_REGION_IP6, aId, aBuf, aLength)
#define otDumpInfoIp6(aId, aBuf, aLength) otDumpInfo(OT_LOG_REGION_IP6, aId, aBuf, aLength)
#define otDumpDebgIp6(aId, aBuf, aLength) otDumpDebg(OT_LOG_REGION_IP6, aId, aBuf, aLength)
#else
#define otDumpCritIp6(aId, aBuf, aLength)
#define otDumpWarnIp6(aId, aBuf, aLength)
#define otDumpNoteIp6(aId, aBuf, aLength)
#define otDumpInfoIp6(aId, aBuf, aLength)
#define otDumpDebgIp6(aId, aBuf, aLength)
#endif

/**
 * @def otDumpCritMac
 *
 * This method generates a memory dump with log level debug and region MAC.
 *
 * @param[in]  aId          A pointer to a NULL-terminated string that is printed before the bytes.
 * @param[in]  aBuf         A pointer to the buffer.
 * @param[in]  aLength      Number of bytes to print.
 *
 */

/**
 * @def otDumpWarnMac
 *
 * This method generates a memory dump with log level warning and region MAC.
 *
 * @param[in]  aId          A pointer to a NULL-terminated string that is printed before the bytes.
 * @param[in]  aBuf         A pointer to the buffer.
 * @param[in]  aLength      Number of bytes to print.
 *
 */

/**
 * @def otDumpNoteMac
 *
 * This method generates a memory dump with log level note and region MAC.
 *
 * @param[in]  aId          A pointer to a NULL-terminated string that is printed before the bytes.
 * @param[in]  aBuf         A pointer to the buffer.
 * @param[in]  aLength      Number of bytes to print.
 *
 */

/**
 * @def otDumpInfoMac
 *
 * This method generates a memory dump with log level info and region MAC.
 *
 * @param[in]  aId          A pointer to a NULL-terminated string that is printed before the bytes.
 * @param[in]  aBuf         A pointer to the buffer.
 * @param[in]  aLength      Number of bytes to print.
 *
 */

/**
 * @def otDumpDebgMac
 *
 * This method generates a memory dump with log level debug and region MAC.
 *
 * @param[in]  aId          A pointer to a NULL-terminated string that is printed before the bytes.
 * @param[in]  aBuf         A pointer to the buffer.
 * @param[in]  aLength      Number of bytes to print.
 *
 */
#if OPENTHREAD_CONFIG_LOG_MAC == 1
#define otDumpCritMac(aId, aBuf, aLength) otDumpCrit(OT_LOG_REGION_MAC, aId, aBuf, aLength)
#define otDumpWarnMac(aId, aBuf, aLength) otDumpWarn(OT_LOG_REGION_MAC, aId, aBuf, aLength)
#define otDumpNoteMac(aId, aBuf, aLength) otDumpNote(OT_LOG_REGION_MAC, aId, aBuf, aLength)
#define otDumpInfoMac(aId, aBuf, aLength) otDumpInfo(OT_LOG_REGION_MAC, aId, aBuf, aLength)
#define otDumpDebgMac(aId, aBuf, aLength) otDumpDebg(OT_LOG_REGION_MAC, aId, aBuf, aLength)
#else
#define otDumpCritMac(aId, aBuf, aLength)
#define otDumpWarnMac(aId, aBuf, aLength)
#define otDumpNoteMac(aId, aBuf, aLength)
#define otDumpInfoMac(aId, aBuf, aLength)
#define otDumpDebgMac(aId, aBuf, aLength)
#endif

/**
 * @def otDumpCritCore
 *
 * This method generates a memory dump with log level debug and region Core.
 *
 * @param[in]  aId          A pointer to a NULL-terminated string that is printed before the bytes.
 * @param[in]  aBuf         A pointer to the buffer.
 * @param[in]  aLength      Number of bytes to print.
 *
 */

/**
 * @def otDumpWarnCore
 *
 * This method generates a memory dump with log level warning and region Core.
 *
 * @param[in]  aId          A pointer to a NULL-terminated string that is printed before the bytes.
 * @param[in]  aBuf         A pointer to the buffer.
 * @param[in]  aLength      Number of bytes to print.
 *
 */

/**
 * @def otDumpNoteCore
 *
 * This method generates a memory dump with log level note and region Core.
 *
 * @param[in]  aId          A pointer to a NULL-terminated string that is printed before the bytes.
 * @param[in]  aBuf         A pointer to the buffer.
 * @param[in]  aLength      Number of bytes to print.
 *
 */

/**
 * @def otDumpInfoCore
 *
 * This method generates a memory dump with log level info and region Core.
 *
 * @param[in]  aId          A pointer to a NULL-terminated string that is printed before the bytes.
 * @param[in]  aBuf         A pointer to the buffer.
 * @param[in]  aLength      Number of bytes to print.
 *
 */

/**
 * @def otDumpDebgCore
 *
 * This method generates a memory dump with log level debug and region Core.
 *
 * @param[in]  aId          A pointer to a NULL-terminated string that is printed before the bytes.
 * @param[in]  aBuf         A pointer to the buffer.
 * @param[in]  aLength      Number of bytes to print.
 *
 */
#if OPENTHREAD_CONFIG_LOG_CORE == 1
#define otDumpCritCore(aId, aBuf, aLength) otDumpCrit(OT_LOG_REGION_CORE, aId, aBuf, aLength)
#define otDumpWarnCore(aId, aBuf, aLength) otDumpWarn(OT_LOG_REGION_CORE, aId, aBuf, aLength)
#define otDumpNoteCore(aId, aBuf, aLength) otDumpNote(OT_LOG_REGION_CORE, aId, aBuf, aLength)
#define otDumpInfoCore(aId, aBuf, aLength) otDumpInfo(OT_LOG_REGION_CORE, aId, aBuf, aLength)
#define otDumpDebgCore(aId, aBuf, aLength) otDumpDebg(OT_LOG_REGION_CORE, aId, aBuf, aLength)
#else
#define otDumpCritCore(aId, aBuf, aLength)
#define otDumpWarnCore(aId, aBuf, aLength)
#define otDumpNoteCore(aId, aBuf, aLength)
#define otDumpInfoCore(aId, aBuf, aLength)
#define otDumpDebgCore(aId, aBuf, aLength)
#endif

/**
 * @def otDumpCritMem
 *
 * This method generates a memory dump with log level debug and region memory.
 *
 * @param[in]  aId          A pointer to a NULL-terminated string that is printed before the bytes.
 * @param[in]  aBuf         A pointer to the buffer.
 * @param[in]  aLength      Number of bytes to print.
 *
 */

/**
 * @def otDumpWarnMem
 *
 * This method generates a memory dump with log level warning and region memory.
 *
 * @param[in]  aId          A pointer to a NULL-terminated string that is printed before the bytes.
 * @param[in]  aBuf         A pointer to the buffer.
 * @param[in]  aLength      Number of bytes to print.
 *
 */

/**
 * @def otDumpNoteMem
 *
 * This method generates a memory dump with log level note and region memory.
 *
 * @param[in]  aId          A pointer to a NULL-terminated string that is printed before the bytes.
 * @param[in]  aBuf         A pointer to the buffer.
 * @param[in]  aLength      Number of bytes to print.
 *
 */

/**
 * @def otDumpInfoMem
 *
 * This method generates a memory dump with log level info and region memory.
 *
 * @param[in]  aId          A pointer to a NULL-terminated string that is printed before the bytes.
 * @param[in]  aBuf         A pointer to the buffer.
 * @param[in]  aLength      Number of bytes to print.
 *
 */

/**
 * @def otDumpDebgMem
 *
 * This method generates a memory dump with log level debug and region memory.
 *
 * @param[in]  aId          A pointer to a NULL-terminated string that is printed before the bytes.
 * @param[in]  aBuf         A pointer to the buffer.
 * @param[in]  aLength      Number of bytes to print.
 *
 */
#if OPENTHREAD_CONFIG_LOG_MEM == 1
#define otDumpCritMem(aId, aBuf, aLength) otDumpCrit(OT_LOG_REGION_MEM, aId, aBuf, aLength)
#define otDumpWarnMem(aId, aBuf, aLength) otDumpWarn(OT_LOG_REGION_MEM, aId, aBuf, aLength)
#define otDumpNoteMem(aId, aBuf, aLength) otDumpNote(OT_LOG_REGION_MEM, aId, aBuf, aLength)
#define otDumpInfoMem(aId, aBuf, aLength) otDumpInfo(OT_LOG_REGION_MEM, aId, aBuf, aLength)
#define otDumpDebgMem(aId, aBuf, aLength) otDumpDebg(OT_LOG_REGION_MEM, aId, aBuf, aLength)
#else
#define otDumpCritMem(aId, aBuf, aLength)
#define otDumpWarnMem(aId, aBuf, aLength)
#define otDumpNoteMem(aId, aBuf, aLength)
#define otDumpInfoMem(aId, aBuf, aLength)
#define otDumpDebgMem(aId, aBuf, aLength)
#endif

/**
 * @def otDumpCert
 *
 * This method generates a memory dump with log level none for the certification test.
 *
 * @param[in]  aId          A pointer to a NULL-terminated string that is printed before the bytes.
 * @param[in]  aBuf         A pointer to the buffer.
 * @param[in]  aLength      Number of bytes to print.
 *
 */
#if OPENTHREAD_ENABLE_CERT_LOG
#define otDumpCertMeshCoP(aId, aBuf, aLength) otDump(OT_LOG_LEVEL_NONE, OT_LOG_REGION_MESH_COP, aId, aBuf, aLength)
#else
#define otDumpCertMeshCoP(aId, aBuf, aLength)
#endif

/**
 * This method dumps bytes to the log in a human-readable fashion.
 *
 * @param[in]  aLevel    The log level.
 * @param[in]  aRegion   The log region.
 * @param[in]  aId       A pointer to a NULL-terminated string that is printed before the bytes.
 * @param[in]  aBuf      A pointer to the buffer.
 * @param[in]  aLength   Number of bytes to print.
 *
 */
void otDump(otLogLevel aLevel, otLogRegion aRegion, const char *aId, const void *aBuf, const size_t aLength);

/**
 * This function converts a log level to a prefix string for appending to log message.
 *
 * @param[in]  aLogLevel    A log level.
 *
 * @returns A C string representing the log level.
 *
 */
const char *otLogLevelToPrefixString(otLogLevel aLogLevel);

/**
 * Local/private macro to format the log message
 */
#define _otLogFormatter(aLogLevel, aRegion, aFormat, ...) \
    _otDynamicLog(aLogLevel, aRegion, aFormat OPENTHREAD_CONFIG_LOG_SUFFIX, ##__VA_ARGS__)

#if OPENTHREAD_CONFIG_ENABLE_DYNAMIC_LOG_LEVEL == 1

/**
 * Local/private macro to dynamically filter log level.
 */
#define _otDynamicLog(aLogLevel, aRegion, aFormat, ...)             \
    do                                                              \
    {                                                               \
        if (otLoggingGetLevel() >= aLogLevel)                       \
            _otPlatLog(aLogLevel, aRegion, aFormat, ##__VA_ARGS__); \
    } while (false)

#else // OPENTHREAD_CONFIG_ENABLE_DYNAMIC_LOG_LEVEL

#define _otDynamicLog(aLogLevel, aRegion, aFormat, ...) _otPlatLog(aLogLevel, aRegion, aFormat, ##__VA_ARGS__)

#endif // OPENTHREAD_CONFIG_ENABLE_DYNAMIC_LOG_LEVEL

/**
 * `OPENTHREAD_CONFIG_PLAT_LOG_FUNCTION` is a configuration parameter (see `openthread-core-default-config.h`) which
 * specifies the function/macro to be used for logging in OpenThread. By default it is set to `otPlatLog()`.
 */
#define _otPlatLog(aLogLevel, aRegion, aFormat, ...) \
    OPENTHREAD_CONFIG_PLAT_LOG_FUNCTION(aLogLevel, aRegion, aFormat, ##__VA_ARGS__)

#ifdef __cplusplus
};
#endif

#endif // LOGGING_HPP_
