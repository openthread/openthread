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

#include "openthread-core-config.h"

#include <ctype.h>
#include <stdio.h>
#include "utils/wrap_string.h"

#include <openthread/instance.h>
#include <openthread/types.h>
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
 * @param[in]  aInstance A pointer to the OpenThread instance.
 * @param[in]  aRegion   The log region.
 * @param[in]  aFormat   A pointer to the format string.
 * @param[in]  ...       Arguments for the format specification.
 *
 */
#if OPENTHREAD_CONFIG_LOG_LEVEL >= OT_LOG_LEVEL_CRIT
#define otLogCrit(aInstance, aRegion, aFormat, ...) \
    _otLogFormatter(aInstance, OT_LOG_LEVEL_CRIT, aRegion, _OT_LEVEL_CRIT_PREFIX aFormat, ##__VA_ARGS__)
#else
#define otLogCrit(aInstance, aRegion, aFormat, ...)
#endif

/**
 * @def otLogWarn
 *
 * Logging at log level warning.
 *
 * @param[in]  aInstance A pointer to the OpenThread instance.
 * @param[in]  aRegion   The log region.
 * @param[in]  aFormat   A pointer to the format string.
 * @param[in]  ...       Arguments for the format specification.
 *
 */
#if OPENTHREAD_CONFIG_LOG_LEVEL >= OT_LOG_LEVEL_WARN
#define otLogWarn(aInstance, aRegion, aFormat, ...) \
    _otLogFormatter(aInstance, OT_LOG_LEVEL_WARN, aRegion, _OT_LEVEL_WARN_PREFIX aFormat, ##__VA_ARGS__)
#else
#define otLogWarn(aInstance, aRegion, aFormat, ...)
#endif

/**
 * @def otLogNote
 *
 * Logging at log level note
 *
 * @param[in]  aInstance A pointer to the OpenThread instance.
 * @param[in]  aRegion   The log region.
 * @param[in]  aFormat   A pointer to the format string.
 * @param[in]  ...       Arguments for the format specification.
 *
 */
#if OPENTHREAD_CONFIG_LOG_LEVEL >= OT_LOG_LEVEL_NOTE
#define otLogNote(aInstance, aRegion, aFormat, ...) \
    _otLogFormatter(aInstance, OT_LOG_LEVEL_NOTE, aRegion, _OT_LEVEL_NOTE_PREFIX aFormat, ##__VA_ARGS__)
#else
#define otLogNote(aInstance, aRegion, aFormat, ...)
#endif

/**
 * @def otLogInfo
 *
 * Logging at log level info.
 *
 * @param[in]  aInstance A pointer to the OpenThread instance.
 * @param[in]  aRegion   The log region.
 * @param[in]  aFormat   A pointer to the format string.
 * @param[in]  ...       Arguments for the format specification.
 *
 */
#if OPENTHREAD_CONFIG_LOG_LEVEL >= OT_LOG_LEVEL_INFO
#define otLogInfo(aInstance, aRegion, aFormat, ...) \
    _otLogFormatter(aInstance, OT_LOG_LEVEL_INFO, aRegion, _OT_LEVEL_INFO_PREFIX aFormat, ##__VA_ARGS__)
#else
#define otLogInfo(aInstance, aRegion, aFormat, ...)
#endif

/**
 * @def otLogDebg
 *
 * Logging at log level debug.
 *
 * @param[in]  aInstance A pointer to the OpenThread instance.
 * @param[in]  aRegion   The log region.
 * @param[in]  aFormat   A pointer to the format string.
 * @param[in]  ...       Arguments for the format specification.
 *
 */
#if OPENTHREAD_CONFIG_LOG_LEVEL >= OT_LOG_LEVEL_DEBG
#define otLogDebg(aInstance, aRegion, aFormat, ...) \
    _otLogFormatter(aInstance, OT_LOG_LEVEL_DEBG, aRegion, _OT_LEVEL_DEBG_PREFIX aFormat, ##__VA_ARGS__)
#else
#define otLogDebg(aInstance, aRegion, aFormat, ...)
#endif

#ifndef WINDOWS_LOGGING

/**
 * @def otLogCritApi
 *
 * This method generates a log with level critical for the API region.
 *
 * @param[in]  aInstance    A reference to the OpenThread instance.
 * @param[in]  aFormat      A pointer to the format string.
 * @param[in]  ...          Arguments for the format specification.
 *
 */

/**
 * @def otLogWarnApi
 *
 * This method generates a log with level warning for the API region.
 *
 * @param[in]  aInstance    A reference to the OpenThread instance.
 * @param[in]  aFormat      A pointer to the format string.
 * @param[in]  ...          Arguments for the format specification.
 *
 */

/**
 * @def otLogNoteApi
 *
 * This method generates a log with level note for the API region.
 *
 * @param[in]  aInstance    A reference to the OpenThread instance.
 * @param[in]  aFormat      A pointer to the format string.
 * @param[in]  ...          Arguments for the format specification.
 *
 */

/**
 * @def otLogInfoApi
 *
 * This method generates a log with level info for the API region.
 *
 * @param[in]  aInstance    A reference to the OpenThread instance.
 * @param[in]  aFormat      A pointer to the format string.
 * @param[in]  ...          Arguments for the format specification.
 *
 */

/**
 * @def otLogDebgApi
 *
 * This method generates a log with level debug for the API region.
 *
 * @param[in]  aInstance    A reference to the OpenThread instance.
 * @param[in]  aFormat      A pointer to the format string.
 * @param[in]  ...          Arguments for the format specification.
 *
 */
#if OPENTHREAD_CONFIG_LOG_API == 1
#define otLogCritApi(aInstance, aFormat, ...) \
    otLogCrit(&aInstance, OT_LOG_REGION_API, _OT_REGION_API_PREFIX aFormat, ##__VA_ARGS__)
#define otLogWarnApi(aInstance, aFormat, ...) \
    otLogWarn(&aInstance, OT_LOG_REGION_API, _OT_REGION_API_PREFIX aFormat, ##__VA_ARGS__)
#define otLogNoteApi(aInstance, aFormat, ...) \
    otLogNote(&aInstance, OT_LOG_REGION_API, _OT_REGION_API_PREFIX aFormat, ##__VA_ARGS__)
#define otLogInfoApi(aInstance, aFormat, ...) \
    otLogInfo(&aInstance, OT_LOG_REGION_API, _OT_REGION_API_PREFIX aFormat, ##__VA_ARGS__)
#define otLogDebgApi(aInstance, aFormat, ...) \
    otLogDebg(&aInstance, OT_LOG_REGION_API, _OT_REGION_API_PREFIX aFormat, ##__VA_ARGS__)
#else
#define otLogCritApi(aInstance, aFormat, ...)
#define otLogWarnApi(aInstance, aFormat, ...)
#define otLogNoteApi(aInstance, aFormat, ...)
#define otLogInfoApi(aInstance, aFormat, ...)
#define otLogDebgApi(aInstance, aFormat, ...)
#endif

/**
 * @def otLogCritMeshCoP
 *
 * This method generates a log with level critical for the MLE region.
 *
 * @param[in]  aInstance    A reference to the OpenThread instance.
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
 * @param[in]  aInstance    A reference to the OpenThread instance.
 * @param[in]  aFormat      A pointer to the format string.
 * @param[in]  ...          Arguments for the format specification.
 *
 */

/**
 * @def otLogNoteMeshCoP
 *
 * This method generates a log with level note for the MLE region.
 *
 * @param[in]  aInstance    A reference to the OpenThread instance.
 * @param[in]  aFormat      A pointer to the format string.
 * @param[in]  ...          Arguments for the format specification.
 *
 */

/**
 * @def otLogInfoMeshCoP
 *
 * This method generates a log with level info for the MLE region.
 *
 * @param[in]  aInstance    A reference to the OpenThread instance.
 * @param[in]  aFormat      A pointer to the format string.
 * @param[in]  ...          Arguments for the format specification.
 *
 */

/**
 * @def otLogDebgMeshCoP
 *
 * This method generates a log with level debug for the MLE region.
 *
 * @param[in]  aInstance    A reference to the OpenThread instance.
 * @param[in]  aFormat      A pointer to the format string.
 * @param[in]  ...          Arguments for the format specification.
 *
 */
#if OPENTHREAD_CONFIG_LOG_MLE == 1
#define otLogCritMeshCoP(aInstance, aFormat, ...) \
    otLogCrit(&aInstance, OT_LOG_REGION_MESH_COP, _OT_REGION_MESH_COP_PREFIX aFormat, ##__VA_ARGS__)
#define otLogWarnMeshCoP(aInstance, aFormat, ...) \
    otLogWarn(&aInstance, OT_LOG_REGION_MESH_COP, _OT_REGION_MESH_COP_PREFIX aFormat, ##__VA_ARGS__)
#define otLogNoteMeshCoP(aInstance, aFormat, ...) \
    otLogNote(&aInstance, OT_LOG_REGION_MESH_COP, _OT_REGION_MESH_COP_PREFIX aFormat, ##__VA_ARGS__)
#define otLogInfoMeshCoP(aInstance, aFormat, ...) \
    otLogInfo(&aInstance, OT_LOG_REGION_MESH_COP, _OT_REGION_MESH_COP_PREFIX aFormat, ##__VA_ARGS__)
#define otLogDebgMeshCoP(aInstance, aFormat, ...) \
    otLogDebg(&aInstance, OT_LOG_REGION_MESH_COP, _OT_REGION_MESH_COP_PREFIX aFormat, ##__VA_ARGS__)
#else
#define otLogCritMeshCoP(aInstance, aFormat, ...)
#define otLogWarnMeshCoP(aInstance, aFormat, ...)
#define otLogNoteMeshCoP(aInstance, aFormat, ...)
#define otLogInfoMeshCoP(aInstance, aFormat, ...)
#define otLogDebgMeshCoP(aInstance, aFormat, ...)
#endif

#define otLogCritMbedTls(aInstance, aFormat, ...) otLogCritMeshCoP(aInstance, aFormat, ##__VA_ARGS__)
#define otLogWarnMbedTls(aInstance, aFormat, ...) otLogWarnMeshCoP(aInstance, aFormat, ##__VA_ARGS__)
#define otLogNoteMbedTls(aInstance, aFormat, ...) otLogNoteMeshCoP(aInstance, aFormat, ##__VA_ARGS__)
#define otLogInfoMbedTls(aInstance, aFormat, ...) otLogInfoMeshCoP(aInstance, aFormat, ##__VA_ARGS__)
#define otLogDebgMbedTls(aInstance, aFormat, ...) otLogDebgMeshCoP(aInstance, aFormat, ##__VA_ARGS__)

/**
 * @def otLogCritMle
 *
 * This method generates a log with level critical for the MLE region.
 *
 * @param[in]  aInstance    A reference to the OpenThread instance.
 * @param[in]  aFormat      A pointer to the format string.
 * @param[in]  ...          Arguments for the format specification.
 *
 */

/**
 * @def otLogWarnMle
 *
 * This method generates a log with level warning for the MLE region.
 *
 * @param[in]  aInstance    A reference to the OpenThread instance.
 * @param[in]  aFormat      A pointer to the format string.
 * @param[in]  ...          Arguments for the format specification.
 *
 */

/**
 * @def otLogNoteMle
 *
 * This method generates a log with level note for the MLE region.
 *
 * @param[in]  aInstance    A reference to the OpenThread instance.
 * @param[in]  aFormat      A pointer to the format string.
 * @param[in]  ...          Arguments for the format specification.
 *
 */

/**
 * @def otLogInfoMle
 *
 * This method generates a log with level info for the MLE region.
 *
 * @param[in]  aInstance    A reference to the OpenThread instance.
 * @param[in]  aFormat      A pointer to the format string.
 * @param[in]  ...          Arguments for the format specification.
 *
 */

/**
 * @def otLogDebgMle
 *
 * This method generates a log with level debug for the MLE region.
 *
 * @param[in]  aInstance    A reference to the OpenThread instance.
 * @param[in]  aFormat      A pointer to the format string.
 * @param[in]  ...          Arguments for the format specification.
 *
 *
 */
#if OPENTHREAD_CONFIG_LOG_MLE == 1
#define otLogCritMle(aInstance, aFormat, ...) \
    otLogCrit(&aInstance, OT_LOG_REGION_MLE, _OT_REGION_MLE_PREFIX aFormat, ##__VA_ARGS__)
#define otLogWarnMle(aInstance, aFormat, ...) \
    otLogWarn(&aInstance, OT_LOG_REGION_MLE, _OT_REGION_MLE_PREFIX aFormat, ##__VA_ARGS__)
#define otLogWarnMleErr(aInstance, aError, aFormat, ...)                                 \
    otLogWarn(&aInstance, OT_LOG_REGION_MLE, _OT_REGION_MLE_PREFIX "Error %s: " aFormat, \
              otThreadErrorToString(aError), ##__VA_ARGS__)
#define otLogNoteMle(aInstance, aFormat, ...) \
    otLogNote(&aInstance, OT_LOG_REGION_MLE, _OT_REGION_MLE_PREFIX aFormat, ##__VA_ARGS__)
#define otLogInfoMle(aInstance, aFormat, ...) \
    otLogInfo(&aInstance, OT_LOG_REGION_MLE, _OT_REGION_MLE_PREFIX aFormat, ##__VA_ARGS__)
#define otLogDebgMle(aInstance, aFormat, ...) \
    otLogDebg(&aInstance, OT_LOG_REGION_MLE, _OT_REGION_MLE_PREFIX aFormat, ##__VA_ARGS__)
#else
#define otLogCritMle(aInstance, aFormat, ...)
#define otLogWarnMle(aInstance, aFormat, ...)
#define otLogWarnMleErr(aInstance, aError, aFormat, ...)
#define otLogNoteMle(aInstance, aFormat, ...)
#define otLogInfoMle(aInstance, aFormat, ...)
#define otLogDebgMle(aInstance, aFormat, ...)
#endif

/**
 * @def otLogCritArp
 *
 * This method generates a log with level critical for the EID-to-RLOC mapping region.
 *
 * @param[in]  aInstance    A reference to the OpenThread instance.
 * @param[in]  aFormat      A pointer to the format string.
 * @param[in]  ...          Arguments for the format specification.
 *
 */

/**
 * @def otLogWarnArp
 *
 * This method generates a log with level warning for the EID-to-RLOC mapping region.
 *
 * @param[in]  aInstance    A reference to the OpenThread instance.
 * @param[in]  aFormat      A pointer to the format string.
 * @param[in]  ...          Arguments for the format specification.
 *
 */

/**
 * @def otLogInfoArp
 *
 * This method generates a log with level note for the EID-to-RLOC mapping region.
 *
 * @param[in]  aInstance    A reference to the OpenThread instance.
 * @param[in]  aFormat      A pointer to the format string.
 * @param[in]  ...          Arguments for the format specification.
 *
 */

/**
 * @def otLogInfoArp
 *
 * This method generates a log with level info for the EID-to-RLOC mapping region.
 *
 * @param[in]  aInstance    A reference to the OpenThread instance.
 * @param[in]  aFormat      A pointer to the format string.
 * @param[in]  ...          Arguments for the format specification.
 *
 */

/**
 * @def otLogDebgArp
 *
 * This method generates a log with level debug for the EID-to-RLOC mapping region.
 *
 * @param[in]  aInstance    A reference to the OpenThread instance.
 * @param[in]  aFormat      A pointer to the format string.
 * @param[in]  ...          Arguments for the format specification.
 *
 */
#if OPENTHREAD_CONFIG_LOG_ARP == 1
#define otLogCritArp(aInstance, aFormat, ...) \
    otLogCrit(&aInstance, OT_LOG_REGION_ARP, _OT_REGION_ARP_PREFIX aFormat, ##__VA_ARGS__)
#define otLogWarnArp(aInstance, aFormat, ...) \
    otLogWarn(&aInstance, OT_LOG_REGION_ARP, _OT_REGION_ARP_PREFIX aFormat, ##__VA_ARGS__)
#define otLogNoteArp(aInstance, aFormat, ...) \
    otLogNote(&aInstance, OT_LOG_REGION_ARP, _OT_REGION_ARP_PREFIX aFormat, ##__VA_ARGS__)
#define otLogInfoArp(aInstance, aFormat, ...) \
    otLogInfo(&aInstance, OT_LOG_REGION_ARP, _OT_REGION_ARP_PREFIX aFormat, ##__VA_ARGS__)
#define otLogDebgArp(aInstance, aFormat, ...) \
    otLogDebg(&aInstance, OT_LOG_REGION_ARP, _OT_REGION_ARP_PREFIX aFormat, ##__VA_ARGS__)
#else
#define otLogCritArp(aInstance, aFormat, ...)
#define otLogWarnArp(aInstance, aFormat, ...)
#define otLogNoteArp(aInstance, aFormat, ...)
#define otLogInfoArp(aInstance, aFormat, ...)
#define otLogDebgArp(aInstance, aFormat, ...)
#endif

/**
 * @def otLogCritNetData
 *
 * This method generates a log with level critical for the Network Data region.
 *
 * @param[in]  aInstance    A reference to the OpenThread instance.
 * @param[in]  aFormat      A pointer to the format string.
 * @param[in]  ...          Arguments for the format specification.
 *
 */

/**
 * @def otLogWarnNetData
 *
 * This method generates a log with level warning for the Network Data region.
 *
 * @param[in]  aInstance    A reference to the OpenThread instance.
 * @param[in]  aFormat      A pointer to the format string.
 * @param[in]  ...          Arguments for the format specification.
 *
 */

/**
 * @def otLogInfoNetData
 *
 * This method generates a log with level note for the Network Data region.
 *
 * @param[in]  aInstance    A reference to the OpenThread instance.
 * @param[in]  aFormat      A pointer to the format string.
 * @param[in]  ...          Arguments for the format specification.
 *
 */

/**
 * @def otLogInfoNetData
 *
 * This method generates a log with level info for the Network Data region.
 *
 * @param[in]  aInstance    A reference to the OpenThread instance.
 * @param[in]  aFormat      A pointer to the format string.
 * @param[in]  ...          Arguments for the format specification.
 *
 */

/**
 * @def otLogDebgNetData
 *
 * This method generates a log with level debug for the Network Data region.
 *
 * @param[in]  aInstance    A reference to the OpenThread instance.
 * @param[in]  aFormat      A pointer to the format string.
 * @param[in]  ...          Arguments for the format specification.
 *
 */
#if OPENTHREAD_CONFIG_LOG_NETDATA == 1
#define otLogCritNetData(aInstance, aFormat, ...) \
    otLogCrit(&aInstance, OT_LOG_REGION_NET_DATA, _OT_REGION_NET_DATA_PREFIX aFormat, ##__VA_ARGS__)
#define otLogWarnNetData(aInstance, aFormat, ...) \
    otLogWarn(&aInstance, OT_LOG_REGION_NET_DATA, _OT_REGION_NET_DATA_PREFIX aFormat, ##__VA_ARGS__)
#define otLogNoteNetData(aInstance, aFormat, ...) \
    otLogNote(&aInstance, OT_LOG_REGION_NET_DATA, _OT_REGION_NET_DATA_PREFIX aFormat, ##__VA_ARGS__)
#define otLogInfoNetData(aInstance, aFormat, ...) \
    otLogInfo(&aInstance, OT_LOG_REGION_NET_DATA, _OT_REGION_NET_DATA_PREFIX aFormat, ##__VA_ARGS__)
#define otLogDebgNetData(aInstance, aFormat, ...) \
    otLogDebg(&aInstance, OT_LOG_REGION_NET_DATA, _OT_REGION_NET_DATA_PREFIX aFormat, ##__VA_ARGS__)
#else
#define otLogCritNetData(aInstance, aFormat, ...)
#define otLogWarnNetData(aInstance, aFormat, ...)
#define otLogNoteNetData(aInstance, aFormat, ...)
#define otLogInfoNetData(aInstance, aFormat, ...)
#define otLogDebgNetData(aInstance, aFormat, ...)
#endif

/**
 * @def otLogCritIcmp
 *
 * This method generates a log with level critical for the ICMPv6 region.
 *
 * @param[in]  aInstance    A reference to the OpenThread instance.
 * @param[in]  aFormat      A pointer to the format string.
 * @param[in]  ...          Arguments for the format specification.
 *
 */

/**
 * @def otLogWarnIcmp
 *
 * This method generates a log with level warning for the ICMPv6 region.
 *
 * @param[in]  aInstance    A reference to the OpenThread instance.
 * @param[in]  aFormat      A pointer to the format string.
 * @param[in]  ...          Arguments for the format specification.
 *
 */

/**
 * @def otLogNoteIcmp
 *
 * This method generates a log with level note for the ICMPv6 region.
 *
 * @param[in]  aInstance    A reference to the OpenThread instance.
 * @param[in]  aFormat      A pointer to the format string.
 * @param[in]  ...          Arguments for the format specification.
 *
 */

/**
 * @def otLogInfoIcmp
 *
 * This method generates a log with level info for the ICMPv6 region.
 *
 * @param[in]  aInstance    A reference to the OpenThread instance.
 * @param[in]  aFormat      A pointer to the format string.
 * @param[in]  ...          Arguments for the format specification.
 *
 */

/**
 * @def otLogDebgIcmp
 *
 * This method generates a log with level debug for the ICMPv6 region.
 *
 * @param[in]  aInstance    A reference to the OpenThread instance.
 * @param[in]  aFormat      A pointer to the format string.
 * @param[in]  ...          Arguments for the format specification.
 *
 */
#if OPENTHREAD_CONFIG_LOG_ICMP == 1
#define otLogCritIcmp(aInstance, aFormat, ...) \
    otLogCrit(&aInstance, OT_LOG_REGION_ICMP, _OT_REGION_ICMP_PREFIX aFormat, ##__VA_ARGS__)
#define otLogWarnIcmp(aInstance, aFormat, ...) \
    otLogWarn(&aInstance, OT_LOG_REGION_ICMP, _OT_REGION_ICMP_PREFIX aFormat, ##__VA_ARGS__)
#define otLogNoteIcmp(aInstance, aFormat, ...) \
    otLogNote(&aInstance, OT_LOG_REGION_ICMP, _OT_REGION_ICMP_PREFIX aFormat, ##__VA_ARGS__)
#define otLogInfoIcmp(aInstance, aFormat, ...) \
    otLogInfo(&aInstance, OT_LOG_REGION_ICMP, _OT_REGION_ICMP_PREFIX aFormat, ##__VA_ARGS__)
#define otLogDebgIcmp(aInstance, aFormat, ...) \
    otLogDebg(&aInstance, OT_LOG_REGION_ICMP, _OT_REGION_ICMP_PREFIX aFormat, ##__VA_ARGS__)
#else
#define otLogCritIcmp(aInstance, aFormat, ...)
#define otLogWarnIcmp(aInstance, aFormat, ...)
#define otLogNoteIcmp(aInstance, aFormat, ...)
#define otLogInfoIcmp(aInstance, aFormat, ...)
#define otLogDebgIcmp(aInstance, aFormat, ...)
#endif

/**
 * @def otLogCritIp6
 *
 * This method generates a log with level critical for the IPv6 region.
 *
 * @param[in]  aInstance    A reference to the OpenThread instance.
 * @param[in]  aFormat      A pointer to the format string.
 * @param[in]  ...          Arguments for the format specification.
 *
 */

/**
 * @def otLogWarnIp6
 *
 * This method generates a log with level warning for the IPv6 region.
 *
 * @param[in]  aInstance    A reference to the OpenThread instance.
 * @param[in]  aFormat      A pointer to the format string.
 * @param[in]  ...          Arguments for the format specification.
 *
 */

/**
 * @def otLogNoteIp6
 *
 * This method generates a log with level note for the IPv6 region.
 *
 * @param[in]  aInstance    A reference to the OpenThread instance.
 * @param[in]  aFormat      A pointer to the format string.
 * @param[in]  ...          Arguments for the format specification.
 *
 */

/**
 * @def otLogInfoIp6
 *
 * This method generates a log with level info for the IPv6 region.
 *
 * @param[in]  aInstance    A reference to the OpenThread instance.
 * @param[in]  aFormat      A pointer to the format string.
 * @param[in]  ...          Arguments for the format specification.
 *
 */

/**
 * @def otLogDebgIp6
 *
 * This method generates a log with level debug for the IPv6 region.
 *
 * @param[in]  aInstance    A reference to the OpenThread instance.
 * @param[in]  aFormat      A pointer to the format string.
 * @param[in]  ...          Arguments for the format specification.
 *
 */
#if OPENTHREAD_CONFIG_LOG_IP6 == 1
#define otLogCritIp6(aInstance, aFormat, ...) \
    otLogCrit(&aInstance, OT_LOG_REGION_IP6, _OT_REGION_IP6_PREFIX aFormat, ##__VA_ARGS__)
#define otLogWarnIp6(aInstance, aFormat, ...) \
    otLogWarn(&aInstance, OT_LOG_REGION_IP6, _OT_REGION_IP6_PREFIX aFormat, ##__VA_ARGS__)
#define otLogNoteIp6(aInstance, aFormat, ...) \
    otLogNote(&aInstance, OT_LOG_REGION_IP6, _OT_REGION_IP6_PREFIX aFormat, ##__VA_ARGS__)
#define otLogInfoIp6(aInstance, aFormat, ...) \
    otLogInfo(&aInstance, OT_LOG_REGION_IP6, _OT_REGION_IP6_PREFIX aFormat, ##__VA_ARGS__)
#define otLogDebgIp6(aInstance, aFormat, ...) \
    otLogDebg(&aInstance, OT_LOG_REGION_IP6, _OT_REGION_IP6_PREFIX aFormat, ##__VA_ARGS__)
#else
#define otLogCritIp6(aInstance, aFormat, ...)
#define otLogWarnIp6(aInstance, aFormat, ...)
#define otLogNoteIp6(aInstance, aFormat, ...)
#define otLogInfoIp6(aInstance, aFormat, ...)
#define otLogDebgIp6(aInstance, aFormat, ...)
#endif

/**
 * @def otLogCritMac
 *
 * This method generates a log with level critical for the MAC region.
 *
 * @param[in]  aInstance    A reference to the OpenThread instance.
 * @param[in]  aFormat      A pointer to the format string.
 * @param[in]  ...          Arguments for the format specification.
 *
 */

/**
 * @def otLogWarnMac
 *
 * This method generates a log with level warning for the MAC region.
 *
 * @param[in]  aInstance    A reference to the OpenThread instance.
 * @param[in]  aFormat      A pointer to the format string.
 * @param[in]  ...          Arguments for the format specification.
 *
 */

/**
 * @def otLogNoteMac
 *
 * This method generates a log with level note for the MAC region.
 *
 * @param[in]  aInstance    A reference to the OpenThread instance.
 * @param[in]  aFormat      A pointer to the format string.
 * @param[in]  ...          Arguments for the format specification.
 *
 */

/**
 * @def otLogInfoMac
 *
 * This method generates a log with level info for the MAC region.
 *
 * @param[in]  aInstance    A reference to the OpenThread instance.
 * @param[in]  aFormat      A pointer to the format string.
 * @param[in]  ...          Arguments for the format specification.
 *
 */

/**
 * @def otLogDebgMac
 *
 * This method generates a log with level debug for the MAC region.
 *
 * @param[in]  aInstance    A reference to the OpenThread instance.
 * @param[in]  aFormat      A pointer to the format string.
 * @param[in]  ...          Arguments for the format specification.
 *
 */

/**
 * @def otLogMac
 *
 * This method generates a log with a given log level for the MAC region.
 *
 * @param[in]  aInstance    A reference to the OpenThread instance.
 * @param[in]  aLogLevel    A log level.
 * @param[in]  aFormat      A pointer to the format string.
 * @param[in]  ...          Arguments for the format specification.
 *
 */
#if OPENTHREAD_CONFIG_LOG_MAC == 1
#define otLogCritMac(aInstance, aFormat, ...) \
    otLogCrit(&aInstance, OT_LOG_REGION_MAC, _OT_REGION_MAC_PREFIX aFormat, ##__VA_ARGS__)
#define otLogWarnMac(aInstance, aFormat, ...) \
    otLogWarn(&aInstance, OT_LOG_REGION_MAC, _OT_REGION_MAC_PREFIX aFormat, ##__VA_ARGS__)
#define otLogNoteMac(aInstance, aFormat, ...) \
    otLogNote(&aInstance, OT_LOG_REGION_MAC, _OT_REGION_MAC_PREFIX aFormat, ##__VA_ARGS__)
#define otLogInfoMac(aInstance, aFormat, ...) \
    otLogInfo(&aInstance, OT_LOG_REGION_MAC, _OT_REGION_MAC_PREFIX aFormat, ##__VA_ARGS__)
#define otLogDebgMac(aInstance, aFormat, ...) \
    otLogDebg(&aInstance, OT_LOG_REGION_MAC, _OT_REGION_MAC_PREFIX aFormat, ##__VA_ARGS__)
#define otLogDebgMacErr(aInstance, aError, aFormat, ...)                                                               \
    otLogWarn(aInstance, OT_LOG_REGION_MAC, _OT_REGION_MAC_PREFIX "Error %s: " aFormat, otThreadErrorToString(aError), \
              ##__VA_ARGS__)
#define otLogMac(aInstance, aLogLevel, aFormat, ...)                                                      \
    do                                                                                                    \
    {                                                                                                     \
        if (aInstance.GetLogLevel() >= aLogLevel)                                                         \
        {                                                                                                 \
            _otLogFormatter(&aInstance, aLogLevel, OT_LOG_REGION_MAC, "%s" _OT_REGION_MAC_PREFIX aFormat, \
                            otLogLevelToPrefixString(aLogLevel), ##__VA_ARGS__);                          \
        }                                                                                                 \
    } while (false)

#else
#define otLogCritMac(aInstance, aFormat, ...)
#define otLogWarnMac(aInstance, aFormat, ...)
#define otLogNoteMac(aInstance, aFormat, ...)
#define otLogInfoMac(aInstance, aFormat, ...)
#define otLogDebgMac(aInstance, aFormat, ...)
#define otLogDebgMacErr(aInstance, aError, aFormat, ...)
#define otLogMac(aInstance, aLogLevel, aFormat, ...)
#endif

/**
 * @def otLogCritCore
 *
 * This method generates a log with level critical for the Core region.
 *
 * @param[in]  aInstance    A reference to the OpenThread instance.
 * @param[in]  aFormat      A pointer to the format string.
 * @param[in]  ...          Arguments for the format specification.
 *
 */

/**
 * @def otLogWarnCore
 *
 * This method generates a log with level warning for the Core region.
 *
 * @param[in]  aInstance    A reference to the OpenThread instance.
 * @param[in]  aFormat      A pointer to the format string.
 * @param[in]  ...          Arguments for the format specification.
 *
 */

/**
 * @def otLogNoteCore
 *
 * This method generates a log with level note for the Core region.
 *
 * @param[in]  aInstance    A reference to the OpenThread instance.
 * @param[in]  aFormat      A pointer to the format string.
 * @param[in]  ...          Arguments for the format specification.
 *
 */

/**
 * @def otLogInfoCore
 *
 * This method generates a log with level info for the Core region.
 *
 * @param[in]  aInstance    A reference to the OpenThread instance.
 * @param[in]  aFormat      A pointer to the format string.
 * @param[in]  ...          Arguments for the format specification.
 *
 */

/**
 * @def otLogDebgCore
 *
 * This method generates a log with level debug for the Core region.
 *
 * @param[in]  aInstance    A reference to the OpenThread instance.
 * @param[in]  aFormat      A pointer to the format string.
 * @param[in]  ...          Arguments for the format specification.
 *
 */
#if OPENTHREAD_CONFIG_LOG_CORE == 1
#define otLogCritCore(aInstance, aFormat, ...) \
    otLogCrit(&aInstance, OT_LOG_REGION_CORE, _OT_REGION_CORE_PREFIX aFormat, ##__VA_ARGS__)
#define otLogWarnCore(aInstance, aFormat, ...) \
    otLogWarn(&aInstance, OT_LOG_REGION_CORE, _OT_REGION_CORE_PREFIX aFormat, ##__VA_ARGS__)
#define otLogNoteCore(aInstance, aFormat, ...) \
    otLogNote(&aInstance, OT_LOG_REGION_CORE, _OT_REGION_CORE_PREFIX aFormat, ##__VA_ARGS__)
#define otLogInfoCore(aInstance, aFormat, ...) \
    otLogInfo(&aInstance, OT_LOG_REGION_CORE, _OT_REGION_CORE_PREFIX aFormat, ##__VA_ARGS__)
#define otLogDebgCore(aInstance, aFormat, ...) \
    otLogDebg(&aInstance, OT_LOG_REGION_CORE, _OT_REGION_CORE_PREFIX aFormat, ##__VA_ARGS__)
#define otLogDebgCoreErr(aInstance, aError, aFormat, ...)                                 \
    otLogWarn(aInstance, OT_LOG_REGION_CORE, _OT_REGION_CORE_PREFIX "Error %s: " aFormat, \
              otThreadErrorToString(aError), ##__VA_ARGS__)
#else
#define otLogCritCore(aInstance, aFormat, ...)
#define otLogWarnCore(aInstance, aFormat, ...)
#define otLogInfoCore(aInstance, aFormat, ...)
#define otLogDebgCore(aInstance, aFormat, ...)
#define otLogDebgCoreErr(aInstance, aError, aFormat, ...)
#endif

/**
 * @def otLogCritMem
 *
 * This method generates a log with level critical for the memory region.
 *
 * @param[in]  aInstance    A reference to the OpenThread instance.
 * @param[in]  aFormat      A pointer to the format string.
 * @param[in]  ...          Arguments for the format specification.
 *
 */

/**
 * @def otLogWarnMem
 *
 * This method generates a log with level warning for the memory region.
 *
 * @param[in]  aInstance    A reference to the OpenThread instance.
 * @param[in]  aFormat      A pointer to the format string.
 * @param[in]  ...          Arguments for the format specification.
 *
 */

/**
 * @def otLogNoteMem
 *
 * This method generates a log with level note for the memory region.
 *
 * @param[in]  aInstance    A reference to the OpenThread instance.
 * @param[in]  aFormat      A pointer to the format string.
 * @param[in]  ...          Arguments for the format specification.
 *
 */

/**
 * @def otLogInfoMem
 *
 * This method generates a log with level info for the memory region.
 *
 * @param[in]  aInstance    A reference to the OpenThread instance.
 * @param[in]  aFormat      A pointer to the format string.
 * @param[in]  ...          Arguments for the format specification.
 *
 */

/**
 * @def otLogDebgMem
 *
 * This method generates a log with level debug for the memory region.
 *
 * @param[in]  aInstance    A reference to the OpenThread instance.
 * @param[in]  aFormat      A pointer to the format string.
 * @param[in]  ...          Arguments for the format specification.
 *
 */
#if OPENTHREAD_CONFIG_LOG_MEM == 1
#define otLogCritMem(aInstance, aFormat, ...) \
    otLogCrit(&aInstance, OT_LOG_REGION_MEM, _OT_REGION_MEM_PREFIX aFormat, ##__VA_ARGS__)
#define otLogWarnMem(aInstance, aFormat, ...) \
    otLogWarn(&aInstance, OT_LOG_REGION_MEM, _OT_REGION_MEM_PREFIX aFormat, ##__VA_ARGS__)
#define otLogNoteMem(aInstance, aFormat, ...) \
    otLogNote(&aInstance, OT_LOG_REGION_MEM, _OT_REGION_MEM_PREFIX aFormat, ##__VA_ARGS__)
#define otLogInfoMem(aInstance, aFormat, ...) \
    otLogInfo(&aInstance, OT_LOG_REGION_MEM, _OT_REGION_MEM_PREFIX aFormat, ##__VA_ARGS__)
#define otLogDebgMem(aInstance, aFormat, ...) \
    otLogDebg(&aInstance, OT_LOG_REGION_MEM, _OT_REGION_MEM_PREFIX aFormat, ##__VA_ARGS__)
#else
#define otLogCritMem(aInstance, aFormat, ...)
#define otLogWarnMem(aInstance, aFormat, ...)
#define otLogNoteMem(aInstance, aFormat, ...)
#define otLogInfoMem(aInstance, aFormat, ...)
#define otLogDebgMem(aInstance, aFormat, ...)
#endif

/**
 * @def otLogCritUtil
 *
 * This method generates a log with level critical for the Util region.
 *
 * @param[in]  aInstance    A reference to the OpenThread instance.
 * @param[in]  aFormat      A pointer to the format string.
 * @param[in]  ...          Arguments for the format specification.
 *
 */

/**
 * @def otLogWarnUtil
 *
 * This method generates a log with level warning for the Util region.
 *
 * @param[in]  aInstance    A reference to the OpenThread instance.
 * @param[in]  aFormat      A pointer to the format string.
 * @param[in]  ...          Arguments for the format specification.
 *
 */

/**
 * @def otLogNoteUtil
 *
 * This method generates a log with level note for the Util region.
 *
 * @param[in]  aInstance    A reference to the OpenThread instance.
 * @param[in]  aFormat      A pointer to the format string.
 * @param[in]  ...          Arguments for the format specification.
 *
 */

/**
 * @def otLogInfoUtil
 *
 * This method generates a log with level info for the Util region.
 *
 * @param[in]  aInstance    A reference to the OpenThread instance.
 * @param[in]  aFormat      A pointer to the format string.
 * @param[in]  ...          Arguments for the format specification.
 *
 */

/**
 * @def otLogDebgUtil
 *
 * This method generates a log with level debug for the Util region.
 *
 * @param[in]  aInstance    A reference to the OpenThread instance.
 * @param[in]  aFormat      A pointer to the format string.
 * @param[in]  ...          Arguments for the format specification.
 *
 */
#if OPENTHREAD_CONFIG_LOG_UTIL == 1
#define otLogCritUtil(aInstance, aFormat, ...) \
    otLogCrit(&aInstance, OT_LOG_REGION_UTIL, _OT_REGION_UTIL_PREFIX aFormat, ##__VA_ARGS__)
#define otLogWarnUtil(aInstance, aFormat, ...) \
    otLogWarn(&aInstance, OT_LOG_REGION_UTIL, _OT_REGION_UTIL_PREFIX aFormat, ##__VA_ARGS__)
#define otLogNoteUtil(aInstance, aFormat, ...) \
    otLogNote(&aInstance, OT_LOG_REGION_UTIL, _OT_REGION_UTIL_PREFIX aFormat, ##__VA_ARGS__)
#define otLogInfoUtil(aInstance, aFormat, ...) \
    otLogInfo(&aInstance, OT_LOG_REGION_UTIL, _OT_REGION_UTIL_PREFIX aFormat, ##__VA_ARGS__)
#define otLogInfoUtilErr(aInstance, aError, aFormat, ...)                                  \
    otLogInfo(&aInstance, OT_LOG_REGION_UTIL, _OT_REGION_CORE_PREFIX "Error %s: " aFormat, \
              otThreadErrorToString(aError), ##__VA_ARGS__)
#define otLogDebgUtil(aInstance, aFormat, ...) \
    otLogDebg(&aInstance, OT_LOG_REGION_UTIL, _OT_REGION_UTIL_PREFIX aFormat, ##__VA_ARGS__)
#else
#define otLogCritUtil(aInstance, aFormat, ...)
#define otLogWarnUtil(aInstance, aFormat, ...)
#define otLogNoteUtil(aInstance, aFormat, ...)
#define otLogInfoUtil(aInstance, aFormat, ...)
#define otLogInfoUtilErr(aInstance, aError, aFormat, ...)
#define otLogDebgUtil(aInstance, aFormat, ...)
#endif

/**
 * @def otLogCritNetDiag
 *
 * This method generates a log with level critical for the NETDIAG region.
 *
 * @param[in]  aInstance    A reference to the OpenThread instance.
 * @param[in]  aFormat      A pointer to the format string.
 * @param[in]  ...          Arguments for the format specification.
 *
 */

/**
 * @def otLogWarnNetDiag
 *
 * This method generates a log with level warning for the NETDIAG region.
 *
 * @param[in]  aInstance    A reference to the OpenThread instance.
 * @param[in]  aFormat      A pointer to the format string.
 * @param[in]  ...          Arguments for the format specification.
 *
 */

/**
 * @def otLogNoteNetDiag
 *
 * This method generates a log with level note for the NETDIAG region.
 *
 * @param[in]  aInstance    A reference to the OpenThread instance.
 * @param[in]  aFormat      A pointer to the format string.
 * @param[in]  ...          Arguments for the format specification.
 *
 */

/**
 * @def otLogInfoNetDiag
 *
 * This method generates a log with level info for the NETDIAG region.
 *
 * @param[in]  aInstance    A reference to the OpenThread instance.
 * @param[in]  aFormat      A pointer to the format string.
 * @param[in]  ...          Arguments for the format specification.
 *
 */

/**
 * @def otLogDebgNetDiag
 *
 * This method generates a log with level debug for the NETDIAG region.
 *
 * @param[in]  aInstance    A reference to the OpenThread instance.
 * @param[in]  aFormat      A pointer to the format string.
 * @param[in]  ...          Arguments for the format specification.
 *
 */
#if OPENTHREAD_CONFIG_LOG_NETDIAG == 1
#define otLogCritNetDiag(aInstance, aFormat, ...) \
    otLogCrit(&aInstance, OT_LOG_REGION_NET_DIAG, _OT_REGION_NET_DIAG_PREFIX aFormat, ##__VA_ARGS__)
#define otLogWarnNetDiag(aInstance, aFormat, ...) \
    otLogWarn(&aInstance, OT_LOG_REGION_NET_DIAG, _OT_REGION_NET_DIAG_PREFIX aFormat, ##__VA_ARGS__)
#define otLogNoteNetDiag(aInstance, aFormat, ...) \
    otLogNote(&aInstance, OT_LOG_REGION_NET_DIAG, _OT_REGION_NET_DIAG_PREFIX aFormat, ##__VA_ARGS__)
#define otLogInfoNetDiag(aInstance, aFormat, ...) \
    otLogInfo(&aInstance, OT_LOG_REGION_NET_DIAG, _OT_REGION_NET_DIAG_PREFIX aFormat, ##__VA_ARGS__)
#define otLogDebgNetDiag(aInstance, aFormat, ...) \
    otLogDebg(&aInstance, OT_LOG_REGION_NET_DIAG, _OT_REGION_NET_DIAG_PREFIX aFormat, ##__VA_ARGS__)
#else
#define otLogCritNetDiag(aInstance, aFormat, ...)
#define otLogWarnNetDiag(aInstance, aFormat, ...)
#define otLogNoteNetDiag(aInstance, aFormat, ...)
#define otLogInfoNetDiag(aInstance, aFormat, ...)
#define otLogDebgNetDiag(aInstance, aFormat, ...)
#endif

/**
 * @def otLogCert
 *
 * This method generates a log with level none for the certification test.
 *
 * @param[in]  aInstance    A reference to the OpenThread instance.
 * @param[in]  aFormat      A pointer to the format string.
 * @param[in]  ...          Arguments for the format specification.
 *
 *
 */
#if OPENTHREAD_ENABLE_CERT_LOG
#define otLogCertMeshCoP(aInstance, aFormat, ...) \
    _otLogFormatter(&aInstance, OT_LOG_LEVEL_NONE, OT_LOG_REGION_MESH_COP, aFormat, ##__VA_ARGS__)
#else
#define otLogCertMeshCoP(aInstance, aFormat, ...)
#endif

/**
 * @def otLogCritCli
 *
 * This method generates a log with level critical for the CLI region.
 *
 * @param[in]  aInstance    A reference to the OpenThread instance.
 * @param[in]  aFormat      A pointer to the format string.
 * @param[in]  ...          Arguments for the format specification.
 *
 */

/**
 * @def otLogWarnCli
 *
 * This method generates a log with level warning for the CLI region.
 *
 * @param[in]  aInstance    A reference to the OpenThread instance.
 * @param[in]  aFormat      A pointer to the format string.
 * @param[in]  ...          Arguments for the format specification.
 *
 */

/**
 * @def otLogInfoCli
 *
 * This method generates a log with level note for the CLI region.
 *
 * @param[in]  aInstance    A reference to the OpenThread instance.
 * @param[in]  aFormat      A pointer to the format string.
 * @param[in]  ...          Arguments for the format specification.
 *
 */

/**
 * @def otLogInfoCli
 *
 * This method generates a log with level info for the CLI region.
 *
 * @param[in]  aInstance    A reference to the OpenThread instance.
 * @param[in]  aFormat      A pointer to the format string.
 * @param[in]  ...          Arguments for the format specification.
 *
 */

/**
 * @def otLogDebgCli
 *
 * This method generates a log with level debug for the CLI region.
 *
 * @param[in]  aInstance    A reference to the OpenThread instance.
 * @param[in]  aFormat      A pointer to the format string.
 * @param[in]  ...          Arguments for the format specification.
 *
 */
#if OPENTHREAD_CONFIG_LOG_CLI == 1

#define otLogCritCli(aInstance, aFormat, ...) \
    otLogCrit(&aInstance, OT_LOG_REGION_CLI, _OT_REGION_CLI_PREFIX aFormat, ##__VA_ARGS__)
#define otLogWarnCli(aInstance, aFormat, ...) \
    otLogWarn(&aInstance, OT_LOG_REGION_CLI, _OT_REGION_CLI_PREFIX aFormat, ##__VA_ARGS__)
#define otLogNoteCli(aInstance, aFormat, ...) \
    otLogNote(&aInstance, OT_LOG_REGION_CLI, _OT_REGION_CLI_PREFIX aFormat, ##__VA_ARGS__)
#define otLogInfoCli(aInstance, aFormat, ...) \
    otLogInfo(&aInstance, OT_LOG_REGION_CLI, _OT_REGION_CLI_PREFIX aFormat, ##__VA_ARGS__)
#define otLogInfoCliErr(aInstance, aError, aFormat, ...) \
    otLogInfo(&aInstance, OT_LOG_REGION_CLI, "Error %s: " aFormat, otThreadErrorToString(aError), ##__VA_ARGS__)
#define otLogDebgCli(aInstance, aFormat, ...) \
    otLogDebg(&aInstance, OT_LOG_REGION_CLI, _OT_REGION_CLI_PREFIX aFormat, ##__VA_ARGS__)
#else
#define otLogCritCli(aInstance, aFormat, ...)
#define otLogWarnCli(aInstance, aFormat, ...)
#define otLogNoteCli(aInstance, aFormat, ...)
#define otLogInfoCli(aInstance, aFormat, ...)
#define otLogInfoCliErr(aInstance, aError, aFormat, ...)
#define otLogDebgCli(aInstance, aFormat, ...)
#endif

/**
 * @def otLogCritCoap
 *
 * This method generates a log with level critical for the CoAP region.
 *
 * @param[in]  aInstance    A reference to the OpenThread instance.
 * @param[in]  aFormat      A pointer to the format string.
 * @param[in]  ...          Arguments for the format specification.
 *
 */

/**
 * @def otLogWarnCoap
 *
 * This method generates a log with level warning for the CoAP region.
 *
 * @param[in]  aInstance    A reference to the OpenThread instance.
 * @param[in]  aFormat      A pointer to the format string.
 * @param[in]  ...          Arguments for the format specification.
 *
 */

/**
 * @def otLogNoteCoap
 *
 * This method generates a log with level note for the CoAP region.
 *
 * @param[in]  aInstance    A reference to the OpenThread instance.
 * @param[in]  aFormat      A pointer to the format string.
 * @param[in]  ...          Arguments for the format specification.
 *
 */

/**
 * @def otLogInfoCoap
 *
 * This method generates a log with level info for the CoAP region.
 *
 * @param[in]  aInstance    A reference to the OpenThread instance.
 * @param[in]  aFormat      A pointer to the format string.
 * @param[in]  ...          Arguments for the format specification.
 *
 */

/**
 * @def otLogDebgCoap
 *
 * This method generates a log with level debug for the CoAP region.
 *
 * @param[in]  aInstance    A reference to the OpenThread instance.
 * @param[in]  aFormat      A pointer to the format string.
 * @param[in]  ...          Arguments for the format specification.
 *
 */
#if OPENTHREAD_CONFIG_LOG_COAP == 1
#define otLogCritCoap(aInstance, aFormat, ...) \
    otLogCrit(&aInstance, OT_LOG_REGION_COAP, _OT_REGION_COAP_PREFIX aFormat, ##__VA_ARGS__)
#define otLogWarnCoap(aInstance, aFormat, ...) \
    otLogWarn(&aInstance, OT_LOG_REGION_COAP, _OT_REGION_COAP_PREFIX aFormat, ##__VA_ARGS__)
#define otLogNoteCoap(aInstance, aFormat, ...) \
    otLogNote(&aInstance, OT_LOG_REGION_COAP, _OT_REGION_COAP_PREFIX aFormat, ##__VA_ARGS__)
#define otLogInfoCoap(aInstance, aFormat, ...) \
    otLogInfo(&aInstance, OT_LOG_REGION_COAP, _OT_REGION_COAP_PREFIX aFormat, ##__VA_ARGS__)
#define otLogInfoCoapErr(aInstance, aError, aFormat, ...)                                  \
    otLogInfo(&aInstance, OT_LOG_REGION_COAP, _OT_REGION_COAP_PREFIX "Error %s: " aFormat, \
              otThreadErrorToString(aError), ##__VA_ARGS__)
#define otLogDebgCoap(aInstance, aFormat, ...) \
    otLogDebg(&aInstance, OT_LOG_REGION_COAP, _OT_REGION_COAP_PREFIX aFormat, ##__VA_ARGS__)
#else
#define otLogCritCoap(aInstance, aFormat, ...)
#define otLogWarnCoap(aInstance, aFormat, ...)
#define otLogNoteCoap(aInstance, aFormat, ...)
#define otLogInfoCoap(aInstance, aFormat, ...)
#define otLogInfoCoapErr(aInstance, aError, aFormat, ...)
#define otLogDebgCoap(aInstance, aFormat, ...)
#endif

/**
 * @def otLogCritPlat
 *
 * This method generates a log with level critical for the Platform region.
 *
 * @param[in]  aInstance A pointer to the OpenThread instance.
 * @param[in]  aFormat   A pointer to the format string.
 * @param[in]  ...       Arguments for the format specification.
 *
 */

/**
 * @def otLogWarnPlat
 *
 * This method generates a log with level warning for the Platform region.
 *
 * @param[in]  aInstance A pointer to the OpenThread instance.
 * @param[in]  aFormat   A pointer to the format string.
 * @param[in]  ...       Arguments for the format specification.
 *
 */

/**
 * @def otLogNotePlat
 *
 * This method generates a log with level note for the Platform region.
 *
 * @param[in]  aInstance A pointer to the OpenThread instance.
 * @param[in]  aFormat   A pointer to the format string.
 * @param[in]  ...       Arguments for the format specification.
 *
 */

/**
 * @def otLogInfoPlat
 *
 * This method generates a log with level info for the Platform region.
 *
 * @param[in]  aInstance A pointer to the OpenThread instance.
 * @param[in]  aFormat   A pointer to the format string.
 * @param[in]  ...       Arguments for the format specification.
 *
 */

/**
 * @def otLogDebgPlat
 *
 * This method generates a log with level debug for the Platform region.
 *
 * @param[in]  aInstance A pointer to the OpenThread instance.
 * @param[in]  aFormat   A pointer to the format string.
 * @param[in]  ...       Arguments for the format specification.
 *
 */
#if OPENTHREAD_CONFIG_LOG_PLATFORM == 1
#define otLogCritPlat(aInstance, aFormat, ...) \
    otLogCrit(aInstance, OT_LOG_REGION_PLATFORM, _OT_REGION_PLATFORM_PREFIX aFormat, ##__VA_ARGS__)
#define otLogWarnPlat(aInstance, aFormat, ...) \
    otLogWarn(aInstance, OT_LOG_REGION_PLATFORM, _OT_REGION_PLATFORM_PREFIX aFormat, ##__VA_ARGS__)
#define otLogNotePlat(aInstance, aFormat, ...) \
    otLogNote(aInstance, OT_LOG_REGION_PLATFORM, _OT_REGION_PLATFORM_PREFIX aFormat, ##__VA_ARGS__)
#define otLogInfoPlat(aInstance, aFormat, ...) \
    otLogInfo(aInstance, OT_LOG_REGION_PLATFORM, _OT_REGION_PLATFORM_PREFIX aFormat, ##__VA_ARGS__)
#define otLogDebgPlat(aInstance, aFormat, ...) \
    otLogDebg(aInstance, OT_LOG_REGION_PLATFORM, _OT_REGION_PLATFORM_PREFIX aFormat, ##__VA_ARGS__)
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
 * @param[in]  aInstance    A reference to the OpenThread instance.
 * @param[in]  aRegion      The log region.
 * @param[in]  aId          A pointer to a NULL-terminated string that is printed before the bytes.
 * @param[in]  aBuf         A pointer to the buffer.
 * @param[in]  aLength      Number of bytes to print.
 *
 */
#if OPENTHREAD_CONFIG_LOG_LEVEL >= OT_LOG_LEVEL_CRIT
#define otDumpCrit(aInstance, aRegion, aId, aBuf, aLength) \
    otDump(&aInstance, OT_LOG_LEVEL_CRIT, aRegion, aId, aBuf, aLength)
#else
#define otDumpCrit(aInstance, aRegion, aId, aBuf, aLength)
#endif

/**
 * @def otDumpWarn
 *
 * This method generates a memory dump with log level warning.
 *
 * @param[in]  aInstance    A reference to the OpenThread instance.
 * @param[in]  aRegion      The log region.
 * @param[in]  aId          A pointer to a NULL-terminated string that is printed before the bytes.
 * @param[in]  aBuf         A pointer to the buffer.
 * @param[in]  aLength      Number of bytes to print.
 *
 */
#if OPENTHREAD_CONFIG_LOG_LEVEL >= OT_LOG_LEVEL_WARN
#define otDumpWarn(aInstance, aRegion, aId, aBuf, aLength) \
    otDump(&aInstance, OT_LOG_LEVEL_WARN, aRegion, aId, aBuf, aLength)
#else
#define otDumpWarn(aInstance, aRegion, aId, aBuf, aLength)
#endif

/**
 * @def otDumpNote
 *
 * This method generates a memory dump with log level note.
 *
 * @param[in]  aInstance    A reference to the OpenThread instance.
 * @param[in]  aRegion      The log region.
 * @param[in]  aId          A pointer to a NULL-terminated string that is printed before the bytes.
 * @param[in]  aBuf         A pointer to the buffer.
 * @param[in]  aLength      Number of bytes to print.
 *
 */
#if OPENTHREAD_CONFIG_LOG_LEVEL >= OT_LOG_LEVEL_NOTE
#define otDumpNote(aInstance, aRegion, aId, aBuf, aLength) \
    otDump(&aInstance, OT_LOG_LEVEL_NOTE, aRegion, aId, aBuf, aLength)
#else
#define otDumpInfo(aInstance, aRegion, aId, aBuf, aLength)
#endif

/**
 * @def otDumpInfo
 *
 * This method generates a memory dump with log level info.
 *
 * @param[in]  aInstance    A reference to the OpenThread instance.
 * @param[in]  aRegion      The log region.
 * @param[in]  aId          A pointer to a NULL-terminated string that is printed before the bytes.
 * @param[in]  aBuf         A pointer to the buffer.
 * @param[in]  aLength      Number of bytes to print.
 *
 */
#if OPENTHREAD_CONFIG_LOG_LEVEL >= OT_LOG_LEVEL_INFO
#define otDumpInfo(aInstance, aRegion, aId, aBuf, aLength) \
    otDump(&aInstance, OT_LOG_LEVEL_INFO, aRegion, aId, aBuf, aLength)
#else
#define otDumpInfo(aInstance, aRegion, aId, aBuf, aLength)
#endif

/**
 * @def otDumpDebg
 *
 * This method generates a memory dump with log level debug.
 *
 * @param[in]  aInstance    A reference to the OpenThread instance.
 * @param[in]  aRegion      The log region.
 * @param[in]  aId          A pointer to a NULL-terminated string that is printed before the bytes.
 * @param[in]  aBuf         A pointer to the buffer.
 * @param[in]  aLength      Number of bytes to print.
 *
 */
#if OPENTHREAD_CONFIG_LOG_LEVEL >= OT_LOG_LEVEL_DEBG
#define otDumpDebg(aInstance, aRegion, aId, aBuf, aLength) \
    otDump(&aInstance, OT_LOG_LEVEL_DEBG, aRegion, aId, aBuf, aLength)
#else
#define otDumpDebg(aInstance, aRegion, aId, aBuf, aLength)
#endif

/**
 * @def otDumpCritNetData
 *
 * This method generates a memory dump with log level debug and region Network Data.
 *
 * @param[in]  aInstance    A reference to the OpenThread instance.
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
 * @param[in]  aInstance    A reference to the OpenThread instance.
 * @param[in]  aBuf         A pointer to the buffer.
 * @param[in]  aLength      Number of bytes to print.
 *
 */

/**
 * @def otDumpNoteNetData
 *
 * @param[in]  aInstance    A reference to the OpenThread instance.
 * @param[in]  aId          A pointer to a NULL-terminated string that is printed before the bytes.
 * @param[in]  aBuf         A pointer to the buffer.
 * @param[in]  aLength      Number of bytes to print.
 *
 */

/**
 * @def otDumpInfoNetData
 *
 * @param[in]  aInstance    A reference to the OpenThread instance.
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
 * @param[in]  aInstance    A reference to the OpenThread instance.
 * @param[in]  aId          A pointer to a NULL-terminated string that is printed before the bytes.
 * @param[in]  aBuf         A pointer to the buffer.
 * @param[in]  aLength      Number of bytes to print.
 *
 */
#if OPENTHREAD_CONFIG_LOG_NETDATA == 1
#define otDumpCritNetData(aInstance, aId, aBuf, aLength) \
    otDumpCrit(aInstance, OT_LOG_REGION_NET_DATA, aId, aBuf, aLength)
#define otDumpWarnNetData(aInstance, aId, aBuf, aLength) \
    otDumpWarn(aInstance, OT_LOG_REGION_NET_DATA, aId, aBuf, aLength)
#define otDumpNoteNetData(aInstance, aId, aBuf, aLength) \
    otDumpNote(aInstance, OT_LOG_REGION_NET_DATA, aId, aBuf, aLength)
#define otDumpInfoNetData(aInstance, aId, aBuf, aLength) \
    otDumpInfo(aInstance, OT_LOG_REGION_NET_DATA, aId, aBuf, aLength)
#define otDumpDebgNetData(aInstance, aId, aBuf, aLength) \
    otDumpDebg(aInstance, OT_LOG_REGION_NET_DATA, aId, aBuf, aLength)
#else
#define otDumpCritNetData(aInstance, aId, aBuf, aLength)
#define otDumpWarnNetData(aInstance, aId, aBuf, aLength)
#define otDumpNoteNetData(aInstance, aId, aBuf, aLength)
#define otDumpInfoNetData(aInstance, aId, aBuf, aLength)
#define otDumpDebgNetData(aInstance, aId, aBuf, aLength)
#endif

/**
 * @def otDumpCritMle
 *
 * This method generates a memory dump with log level debug and region MLE.
 *
 * @param[in]  aInstance    A reference to the OpenThread instance.
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
 * @param[in]  aInstance    A reference to the OpenThread instance.
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
 * @param[in]  aInstance    A reference to the OpenThread instance.
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
 * @param[in]  aInstance    A reference to the OpenThread instance.
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
 * @param[in]  aInstance    A reference to the OpenThread instance.
 * @param[in]  aId          A pointer to a NULL-terminated string that is printed before the bytes.
 * @param[in]  aBuf         A pointer to the buffer.
 * @param[in]  aLength      Number of bytes to print.
 *
 */
#if OPENTHREAD_CONFIG_LOG_MLE == 1
#define otDumpCritMle(aInstance, aId, aBuf, aLength) otDumpCrit(aInstance, OT_LOG_REGION_MLE, aId, aBuf, aLength)
#define otDumpWarnMle(aInstance, aId, aBuf, aLength) otDumpWarn(aInstance, OT_LOG_REGION_MLE, aId, aBuf, aLength)
#define otDumpNoteMle(aInstance, aId, aBuf, aLength) otDumpNote(aInstance, OT_LOG_REGION_MLE, aId, aBuf, aLength)
#define otDumpInfoMle(aInstance, aId, aBuf, aLength) otDumpInfo(aInstance, OT_LOG_REGION_MLE, aId, aBuf, aLength)
#define otDumpDebgMle(aInstance, aId, aBuf, aLength) otDumpDebg(aInstance, OT_LOG_REGION_MLE, aId, aBuf, aLength)
#else
#define otDumpCritMle(aInstance, aId, aBuf, aLength)
#define otDumpWarnMle(aInstance, aId, aBuf, aLength)
#define otDumpNoteMle(aInstance, aId, aBuf, aLength)
#define otDumpInfoMle(aInstance, aId, aBuf, aLength)
#define otDumpDebgMle(aInstance, aId, aBuf, aLength)
#endif

/**
 * @def otDumpCritArp
 *
 * This method generates a memory dump with log level debug and region EID-to-RLOC mapping.
 *
 * @param[in]  aInstance    A reference to the OpenThread instance.
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
 * @param[in]  aInstance    A reference to the OpenThread instance.
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
 * @param[in]  aInstance    A reference to the OpenThread instance.
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
 * @param[in]  aInstance    A reference to the OpenThread instance.
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
 * @param[in]  aInstance    A reference to the OpenThread instance.
 * @param[in]  aId          A pointer to a NULL-terminated string that is printed before the bytes.
 * @param[in]  aBuf         A pointer to the buffer.
 * @param[in]  aLength      Number of bytes to print.
 *
 */
#if OPENTHREAD_CONFIG_LOG_ARP == 1
#define otDumpCritArp(aInstance, aId, aBuf, aLength) otDumpCrit(aInstance, OT_LOG_REGION_ARP, aId, aBuf, aLength)
#define otDumpWarnArp(aInstance, aId, aBuf, aLength) otDumpWarn(aInstance, OT_LOG_REGION_ARP, aId, aBuf, aLength)
#define otDumpNoteArp(aInstance, aId, aBuf, aLength) otDumpNote(aInstance, OT_LOG_REGION_ARP, aId, aBuf, aLength)
#define otDumpInfoArp(aInstance, aId, aBuf, aLength) otDumpInfo(aInstance, OT_LOG_REGION_ARP, aId, aBuf, aLength)
#define otDumpDebgArp(aInstance, aId, aBuf, aLength) otDumpDebg(aInstance, OT_LOG_REGION_ARP, aId, aBuf, aLength)
#else
#define otDumpCritArp(aInstance, aId, aBuf, aLength)
#define otDumpWarnArp(aInstance, aId, aBuf, aLength)
#define otDumpNoteArp(aInstance, aId, aBuf, aLength)
#define otDumpInfoArp(aInstance, aId, aBuf, aLength)
#define otDumpDebgArp(aInstance, aId, aBuf, aLength)
#endif

/**
 * @def otDumpCritIcmp
 *
 * This method generates a memory dump with log level debug and region ICMPv6.
 *
 * @param[in]  aInstance    A reference to the OpenThread instance.
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
 * @param[in]  aInstance    A reference to the OpenThread instance.
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
 * @param[in]  aInstance    A reference to the OpenThread instance.
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
 * @param[in]  aInstance    A reference to the OpenThread instance.
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
 * @param[in]  aInstance    A reference to the OpenThread instance.
 * @param[in]  aId          A pointer to a NULL-terminated string that is printed before the bytes.
 * @param[in]  aBuf         A pointer to the buffer.
 * @param[in]  aLength      Number of bytes to print.
 *
 */
#if OPENTHREAD_CONFIG_LOG_ICMP == 1
#define otDumpCritIcmp(aInstance, aId, aBuf, aLength) otDumpCrit(aInstance, OT_LOG_REGION_ICMP, aId, aBuf, aLength)
#define otDumpWarnIcmp(aInstance, aId, aBuf, aLength) otDumpWarn(aInstance, OT_LOG_REGION_ICMP, aId, aBuf, aLength)
#define otDumpNoteIcmp(aInstance, aId, aBuf, aLength) otDumpNote(aInstance, OT_LOG_REGION_ICMP, aId, aBuf, aLength)
#define otDumpInfoIcmp(aInstance, aId, aBuf, aLength) otDumpInfo(aInstance, OT_LOG_REGION_ICMP, aId, aBuf, aLength)
#define otDumpDebgIcmp(aInstance, aId, aBuf, aLength) otDumpDebg(aInstance, OT_LOG_REGION_ICMP, aId, aBuf, aLength)
#else
#define otDumpCritIcmp(aInstance, aId, aBuf, aLength)
#define otDumpWarnIcmp(aInstance, aId, aBuf, aLength)
#define otDumpNoteIcmp(aInstance, aId, aBuf, aLength)
#define otDumpInfoIcmp(aInstance, aId, aBuf, aLength)
#define otDumpDebgIcmp(aInstance, aId, aBuf, aLength)
#endif

/**
 * @def otDumpCritIp6
 *
 * This method generates a memory dump with log level debug and region IPv6.
 *
 * @param[in]  aInstance    A reference to the OpenThread instance.
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
 * @param[in]  aInstance    A reference to the OpenThread instance.
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
 * @param[in]  aInstance    A reference to the OpenThread instance.
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
 * @param[in]  aInstance    A reference to the OpenThread instance.
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
 * @param[in]  aInstance    A reference to the OpenThread instance.
 * @param[in]  aId          A pointer to a NULL-terminated string that is printed before the bytes.
 * @param[in]  aBuf         A pointer to the buffer.
 * @param[in]  aLength      Number of bytes to print.
 *
 */
#if OPENTHREAD_CONFIG_LOG_IP6 == 1
#define otDumpCritIp6(aInstance, aId, aBuf, aLength) otDumpCrit(aInstance, OT_LOG_REGION_IP6, aId, aBuf, aLength)
#define otDumpWarnIp6(aInstance, aId, aBuf, aLength) otDumpWarn(aInstance, OT_LOG_REGION_IP6, aId, aBuf, aLength)
#define otDumpNoteIp6(aInstance, aId, aBuf, aLength) otDumpNote(aInstance, OT_LOG_REGION_IP6, aId, aBuf, aLength)
#define otDumpInfoIp6(aInstance, aId, aBuf, aLength) otDumpInfo(aInstance, OT_LOG_REGION_IP6, aId, aBuf, aLength)
#define otDumpDebgIp6(aInstance, aId, aBuf, aLength) otDumpDebg(aInstance, OT_LOG_REGION_IP6, aId, aBuf, aLength)
#else
#define otDumpCritIp6(aInstance, aId, aBuf, aLength)
#define otDumpWarnIp6(aInstance, aId, aBuf, aLength)
#define otDumpNoteIp6(aInstance, aId, aBuf, aLength)
#define otDumpInfoIp6(aInstance, aId, aBuf, aLength)
#define otDumpDebgIp6(aInstance, aId, aBuf, aLength)
#endif

/**
 * @def otDumpCritMac
 *
 * This method generates a memory dump with log level debug and region MAC.
 *
 * @param[in]  aInstance    A reference to the OpenThread instance.
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
 * @param[in]  aInstance    A reference to the OpenThread instance.
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
 * @param[in]  aInstance    A reference to the OpenThread instance.
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
 * @param[in]  aInstance    A reference to the OpenThread instance.
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
 * @param[in]  aInstance    A reference to the OpenThread instance.
 * @param[in]  aId          A pointer to a NULL-terminated string that is printed before the bytes.
 * @param[in]  aBuf         A pointer to the buffer.
 * @param[in]  aLength      Number of bytes to print.
 *
 */
#if OPENTHREAD_CONFIG_LOG_MAC == 1
#define otDumpCritMac(aInstance, aId, aBuf, aLength) otDumpCrit(aInstance, OT_LOG_REGION_MAC, aId, aBuf, aLength)
#define otDumpWarnMac(aInstance, aId, aBuf, aLength) otDumpWarn(aInstance, OT_LOG_REGION_MAC, aId, aBuf, aLength)
#define otDumpNoteMac(aInstance, aId, aBuf, aLength) otDumpNote(aInstance, OT_LOG_REGION_MAC, aId, aBuf, aLength)
#define otDumpInfoMac(aInstance, aId, aBuf, aLength) otDumpInfo(aInstance, OT_LOG_REGION_MAC, aId, aBuf, aLength)
#define otDumpDebgMac(aInstance, aId, aBuf, aLength) otDumpDebg(aInstance, OT_LOG_REGION_MAC, aId, aBuf, aLength)
#else
#define otDumpCritMac(aInstance, aId, aBuf, aLength)
#define otDumpWarnMac(aInstance, aId, aBuf, aLength)
#define otDumpNoteMac(aInstance, aId, aBuf, aLength)
#define otDumpInfoMac(aInstance, aId, aBuf, aLength)
#define otDumpDebgMac(aInstance, aId, aBuf, aLength)
#endif

/**
 * @def otDumpCritCore
 *
 * This method generates a memory dump with log level debug and region Core.
 *
 * @param[in]  aInstance    A reference to the OpenThread instance.
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
 * @param[in]  aInstance    A reference to the OpenThread instance.
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
 * @param[in]  aInstance    A reference to the OpenThread instance.
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
 * @param[in]  aInstance    A reference to the OpenThread instance.
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
 * @param[in]  aInstance    A reference to the OpenThread instance.
 * @param[in]  aId          A pointer to a NULL-terminated string that is printed before the bytes.
 * @param[in]  aBuf         A pointer to the buffer.
 * @param[in]  aLength      Number of bytes to print.
 *
 */
#if OPENTHREAD_CONFIG_LOG_CORE == 1
#define otDumpCritCore(aInstance, aId, aBuf, aLength) otDumpCrit(aInstance, OT_LOG_REGION_CORE, aId, aBuf, aLength)
#define otDumpWarnCore(aInstance, aId, aBuf, aLength) otDumpWarn(aInstance, OT_LOG_REGION_CORE, aId, aBuf, aLength)
#define otDumpNoteCore(aInstance, aId, aBuf, aLength) otDumpNote(aInstance, OT_LOG_REGION_CORE, aId, aBuf, aLength)
#define otDumpInfoCore(aInstance, aId, aBuf, aLength) otDumpInfo(aInstance, OT_LOG_REGION_CORE, aId, aBuf, aLength)
#define otDumpDebgCore(aInstance, aId, aBuf, aLength) otDumpDebg(aInstance, OT_LOG_REGION_CORE, aId, aBuf, aLength)
#else
#define otDumpCritCore(aInstance, aId, aBuf, aLength)
#define otDumpWarnCore(aInstance, aId, aBuf, aLength)
#define otDumpNoteCore(aInstance, aId, aBuf, aLength)
#define otDumpInfoCore(aInstance, aId, aBuf, aLength)
#define otDumpDebgCore(aInstance, aId, aBuf, aLength)
#endif

/**
 * @def otDumpCritMem
 *
 * This method generates a memory dump with log level debug and region memory.
 *
 * @param[in]  aInstance    A reference to the OpenThread instance.
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
 * @param[in]  aInstance    A reference to the OpenThread instance.
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
 * @param[in]  aInstance    A reference to the OpenThread instance.
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
 * @param[in]  aInstance    A reference to the OpenThread instance.
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
 * @param[in]  aInstance    A reference to the OpenThread instance.
 * @param[in]  aId          A pointer to a NULL-terminated string that is printed before the bytes.
 * @param[in]  aBuf         A pointer to the buffer.
 * @param[in]  aLength      Number of bytes to print.
 *
 */
#if OPENTHREAD_CONFIG_LOG_MEM == 1
#define otDumpCritMem(aInstance, aId, aBuf, aLength) otDumpCrit(aInstance, OT_LOG_REGION_MEM, aId, aBuf, aLength)
#define otDumpWarnMem(aInstance, aId, aBuf, aLength) otDumpWarn(aInstance, OT_LOG_REGION_MEM, aId, aBuf, aLength)
#define otDumpNoteMem(aInstance, aId, aBuf, aLength) otDumpNote(aInstance, OT_LOG_REGION_MEM, aId, aBuf, aLength)
#define otDumpInfoMem(aInstance, aId, aBuf, aLength) otDumpInfo(aInstance, OT_LOG_REGION_MEM, aId, aBuf, aLength)
#define otDumpDebgMem(aInstance, aId, aBuf, aLength) otDumpDebg(aInstance, OT_LOG_REGION_MEM, aId, aBuf, aLength)
#else
#define otDumpCritMem(aInstance, aId, aBuf, aLength)
#define otDumpWarnMem(aInstance, aId, aBuf, aLength)
#define otDumpNoteMem(aInstance, aId, aBuf, aLength)
#define otDumpInfoMem(aInstance, aId, aBuf, aLength)
#define otDumpDebgMem(aInstance, aId, aBuf, aLength)
#endif

/**
 * @def otDumpCert
 *
 * This method generates a memory dump with log level none for the certification test.
 *
 * @param[in]  aInstance    A reference to the OpenThread instance.
 * @param[in]  aId          A pointer to a NULL-terminated string that is printed before the bytes.
 * @param[in]  aBuf         A pointer to the buffer.
 * @param[in]  aLength      Number of bytes to print.
 *
 */
#if OPENTHREAD_ENABLE_CERT_LOG
#define otDumpCertMeshCoP(aInstance, aId, aBuf, aLength) \
    otDump(&aInstance, OT_LOG_LEVEL_NONE, OT_LOG_REGION_MESH_COP, aId, aBuf, aLength)
#else
#define otDumpCertMeshCoP(aInstance, aId, aBuf, aLength)
#endif

/**
 * This method dumps bytes to the log in a human-readable fashion.
 *
 * @param[in]  aInstance A pointer to the instance.
 * @param[in]  aLevel    The log level.
 * @param[in]  aRegion   The log region.
 * @param[in]  aId       A pointer to a NULL-terminated string that is printed before the bytes.
 * @param[in]  aBuf      A pointer to the buffer.
 * @param[in]  aLength   Number of bytes to print.
 *
 */
void otDump(otInstance * aIntsance,
            otLogLevel   aLevel,
            otLogRegion  aRegion,
            const char * aId,
            const void * aBuf,
            const size_t aLength);

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
#define _otLogFormatter(aInstance, aLogLevel, aRegion, aFormat, ...) \
    _otDynamicLog(aInstance, aLogLevel, aRegion, aFormat OPENTHREAD_CONFIG_LOG_SUFFIX, ##__VA_ARGS__)

#if OPENTHREAD_CONFIG_ENABLE_DYNAMIC_LOG_LEVEL == 1

/**
 * Local/private macro to dynamically filter log level.
 */
#define _otDynamicLog(aInstance, aLogLevel, aRegion, aFormat, ...)  \
    do                                                              \
    {                                                               \
        if (otGetDynamicLogLevel(aInstance) >= aLogLevel)           \
            _otPlatLog(aLogLevel, aRegion, aFormat, ##__VA_ARGS__); \
    } while (false)

#else // OPENTHREAD_CONFIG_ENABLE_DYNAMIC_LOG_LEVEL

#define _otDynamicLog(aInstance, aLogLevel, aRegion, aFormat, ...) \
    _otPlatLog(aLogLevel, aRegion, aFormat, ##__VA_ARGS__)

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
