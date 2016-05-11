/*
 *    Copyright (c) 2016, Nest Labs, Inc.
 *    All rights reserved.
 *
 *    Redistribution and use in source and binary forms, with or without
 *    modification, are permitted provided that the following conditions are met:
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *    3. Neither the name of the copyright holder nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 *    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 *    ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 *    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
 *    DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 *    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 *    ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 *    SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file
 *   This file includes functions for debugging.
 */

#ifndef LOGGING_HPP_
#define LOGGING_HPP_

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include <openthread-core-config.h>
#include <platform/logging.h>

#ifdef __cplusplus
extern "C" {
#endif

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
#define otDumpCrit(aRegion, aId, aBuf, aLength)  otDump(kLogLevelCrit, aRegion, aId, aBuf, aLength)
#else
#define otDumpCrit(aRegion, aId, aBuf, aLength)
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
#define otDumpWarn(aRegion, aId, aBuf, aLength)  otDump(kLogLevelWarn, aRegion, aId, aBuf, aLength)
#else
#define otDumpWarn(aRegion, aId, aBuf, aLength)
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
#define otDumpInfo(aRegion, aId, aBuf, aLength)  otDump(kLogLevelInfo, aRegion, aId, aBuf, aLength)
#else
#define otDumpInfo(aRegion, aId, aBuf, aLength)
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
#define otDumpDebg(aRegion, aId, aBuf, aLength)  otDump(kLogLevelDebg, aRegion, aId, aBuf, aLength)
#else
#define otDumpDebg(aRegion, aId, aBuf, aLength)
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
#ifdef OPENTHREAD_CONFIG_LOG_NETDATA
#define otDumpCritNetData(aId, aBuf, aLength) otDumpCrit(kLogRegionNetData, aId, aBuf, aLength)
#define otDumpWarnNetData(aId, aBuf, aLength) otDumpWarn(kLogRegionNetData, aId, aBuf, aLength)
#define otDumpInfoNetData(aId, aBuf, aLength) otDumpInfo(kLogRegionNetData, aId, aBuf, aLength)
#define otDumpDebgNetData(aId, aBuf, aLength) otDumpDebg(kLogRegionNetData, aId, aBuf, aLength)
#else
#define otDumpCritNetData(aId, aBuf, aLength)
#define otDumpWarnNetData(aId, aBuf, aLength)
#define otDumpInfoNetData(aId, aBuf, aLength)
#define otDumpDebgNetData(aId, aBuf, aLength)
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
#ifdef OPENTHREAD_CONFIG_LOG_MLE
#define otDumpCritMle(aId, aBuf, aLength) otDumpCrit(kLogRegionMle, aId, aBuf, aLength)
#define otDumpWarnMle(aId, aBuf, aLength) otDumpWarn(kLogRegionMle, aId, aBuf, aLength)
#define otDumpInfoMle(aId, aBuf, aLength) otDumpInfo(kLogRegionMle, aId, aBuf, aLength)
#define otDumpDebgMle(aId, aBuf, aLength) otDumpDebg(kLogRegionMle, aId, aBuf, aLength)
#else
#define otDumpCritMle(aId, aBuf, aLength)
#define otDumpWarnMle(aId, aBuf, aLength)
#define otDumpInfoMle(aId, aBuf, aLength)
#define otDumpDebgMle(aId, aBuf, aLength)
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
#ifdef OPENTHREAD_CONFIG_LOG_ARP
#define otDumpCritArp(aId, aBuf, aLength) otDumpCrit(kLogRegionArp, aId, aBuf, aLength)
#define otDumpWarnArp(aId, aBuf, aLength) otDumpWarn(kLogRegionArp, aId, aBuf, aLength)
#define otDumpInfoArp(aId, aBuf, aLength) otDumpInfo(kLogRegionArp, aId, aBuf, aLength)
#define otDumpDebgArp(aId, aBuf, aLength) otDumpDebg(kLogRegionArp, aId, aBuf, aLength)
#else
#define otDumpCritArp(aId, aBuf, aLength)
#define otDumpWarnArp(aId, aBuf, aLength)
#define otDumpInfoArp(aId, aBuf, aLength)
#define otDumpDebgArp(aId, aBuf, aLength)
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
#ifdef OPENTHREAD_CONFIG_LOG_ICMP
#define otDumpCritIcmp(aId, aBuf, aLength) otDumpCrit(kLogRegionIcmp, aId, aBuf, aLength)
#define otDumpWarnIcmp(aId, aBuf, aLength) otDumpWarn(kLogRegionIcmp, aId, aBuf, aLength)
#define otDumpInfoIcmp(aId, aBuf, aLength) otDumpInfo(kLogRegionIcmp, aId, aBuf, aLength)
#define otDumpDebgIcmp(aId, aBuf, aLength) otDumpDebg(kLogRegionIcmp, aId, aBuf, aLength)
#else
#define otDumpCritIcmp(aId, aBuf, aLength)
#define otDumpWarnIcmp(aId, aBuf, aLength)
#define otDumpInfoIcmp(aId, aBuf, aLength)
#define otDumpDebgIcmp(aId, aBuf, aLength)
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
#ifdef OPENTHREAD_CONFIG_LOG_IP6
#define otDumpCritIp6(aId, aBuf, aLength) otDumpCrit(kLogRegionIp6, aId, aBuf, aLength)
#define otDumpWarnIp6(aId, aBuf, aLength) otDumpWarn(kLogRegionIp6, aId, aBuf, aLength)
#define otDumpInfoIp6(aId, aBuf, aLength) otDumpInfo(kLogRegionIp6, aId, aBuf, aLength)
#define otDumpDebgIp6(aId, aBuf, aLength) otDumpDebg(kLogRegionIp6, aId, aBuf, aLength)
#else
#define otDumpCritIp6(aId, aBuf, aLength)
#define otDumpWarnIp6(aId, aBuf, aLength)
#define otDumpInfoIp6(aId, aBuf, aLength)
#define otDumpDebgIp6(aId, aBuf, aLength)
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
#ifdef OPENTHREAD_CONFIG_LOG_MAC
#define otDumpCritMac(aId, aBuf, aLength) otDumpCrit(kLogRegionMac, aId, aBuf, aLength)
#define otDumpWarnMac(aId, aBuf, aLength) otDumpWarn(kLogRegionMac, aId, aBuf, aLength)
#define otDumpInfoMac(aId, aBuf, aLength) otDumpInfo(kLogRegionMac, aId, aBuf, aLength)
#define otDumpDebgMac(aId, aBuf, aLength) otDumpDebg(kLogRegionMac, aId, aBuf, aLength)
#else
#define otDumpCritMac(aId, aBuf, aLength)
#define otDumpWarnMac(aId, aBuf, aLength)
#define otDumpInfoMac(aId, aBuf, aLength)
#define otDumpDebgMac(aId, aBuf, aLength)
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
#ifdef OPENTHREAD_CONFIG_LOG_MEM
#define otDumpCritMem(aId, aBuf, aLength) otDumpCrit(kLogRegionMem, aId, aBuf, aLength)
#define otDumpWarnMem(aId, aBuf, aLength) otDumpWarn(kLogRegionMem, aId, aBuf, aLength)
#define otDumpInfoMem(aId, aBuf, aLength) otDumpInfo(kLogRegionMem, aId, aBuf, aLength)
#define otDumpDebgMem(aId, aBuf, aLength) otDumpDebg(kLogRegionMem, aId, aBuf, aLength)
#else
#define otDumpCritMem(aId, aBuf, aLength)
#define otDumpWarnMem(aId, aBuf, aLength)
#define otDumpInfoMem(aId, aBuf, aLength)
#define otDumpDebgMem(aId, aBuf, aLength)
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
void otDump(otLogLevel aLevel, otLogRegion aRegion, const char *aId, const void *aBuf, const int aLength);

#ifdef __cplusplus
};
#endif

#endif  // LOGGING_HPP_
