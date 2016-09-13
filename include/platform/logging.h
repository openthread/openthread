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

#include <openthread-std-types.h>

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
    kLogRegionNcp      = 9,  ///< NCP
    kLogRegionMeshCoP  = 10, ///< Mesh Commissioning Protocol
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
 * @}
 *
 */

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // DEBUG_H_
