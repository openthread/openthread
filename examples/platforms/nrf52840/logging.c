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
 *   This file implements the OpenThread platform abstraction for logging.
 *
 */

#include <openthread-core-config.h>
#include <openthread/config.h>

#include <stdarg.h>
#include <stdio.h>

#include <utils/code_utils.h>
#include <openthread/platform/alarm-milli.h>
#include <openthread/platform/logging.h>

#include "platform-nrf5.h"

#include <openthread-core-config.h>
#include <openthread/config.h>
#include <openthread/types.h>

#if (OPENTHREAD_CONFIG_LOG_OUTPUT == OPENTHREAD_CONFIG_LOG_OUTPUT_PLATFORM_DEFINED) || \
    (OPENTHREAD_CONFIG_LOG_OUTPUT == OPENTHREAD_CONFIG_LOG_OUTPUT_NCP_SPINEL)
#include <segger_rtt/SEGGER_RTT.h>

#if (LOG_RTT_COLOR_ENABLE == 1)
#define RTT_COLOR_CODE_DEFAULT "\x1B[0m"
#define RTT_COLOR_CODE_RED "\x1B[1;31m"
#define RTT_COLOR_CODE_GREEN "\x1B[1;32m"
#define RTT_COLOR_CODE_YELLOW "\x1B[1;33m"
#define RTT_COLOR_CODE_CYAN "\x1B[1;36m"
#else // LOG_RTT_COLOR_ENABLE == 1
#define RTT_COLOR_CODE_DEFAULT ""
#define RTT_COLOR_CODE_RED ""
#define RTT_COLOR_CODE_GREEN ""
#define RTT_COLOR_CODE_YELLOW ""
#define RTT_COLOR_CODE_CYAN ""
#endif // LOG_RTT_COLOR_ENABLE == 1

static bool    sLogInitialized = false;
static uint8_t sLogBuffer[LOG_RTT_BUFFER_SIZE];

/**
 * Function for getting color of a given level log.
 *
 * @param[in]  aLogLevel The log level.
 *
 * @returns  String with a log level color value.
 */
static inline const char *levelToString(otLogLevel aLogLevel)
{
    switch (aLogLevel)
    {
    case OT_LOG_LEVEL_CRIT:
        return RTT_COLOR_CODE_RED;

    case OT_LOG_LEVEL_WARN:
        return RTT_COLOR_CODE_YELLOW;

    case OT_LOG_LEVEL_INFO:
        return RTT_COLOR_CODE_GREEN;

    case OT_LOG_LEVEL_DEBG:
    default:
        return RTT_COLOR_CODE_DEFAULT;
    }
}

#if (LOG_TIMESTAMP_ENABLE == 1)
/**
 * Function for printing actual timestamp.
 *
 * @param[inout]  aLogString Pointer to the log buffer.
 * @param[in]     aMaxSize   Maximum size of the log buffer.
 *
 * @returns  Number of bytes successfully written to the log buffer.
 */
static inline uint16_t logTimestamp(char *aLogString, uint16_t aMaxSize)
{
    return snprintf(aLogString, aMaxSize, "%s[%010ld]", RTT_COLOR_CODE_CYAN, otPlatAlarmMilliGetNow());
}
#endif

/**
 * Function for printing log level.
 *
 * @param[inout]  aLogString  Pointer to log buffer.
 * @param[in]     aMaxSize    Maximum size of log buffer.
 * @param[in]     aLogLevel   Log level.
 *
 * @returns  Number of bytes successfully written to the log buffer.
 */
static inline uint16_t logLevel(char *aLogString, uint16_t aMaxSize, otLogLevel aLogLevel)
{
    return snprintf(aLogString, aMaxSize, "%s ", levelToString(aLogLevel));
}

void nrf5LogInit()
{
    int res = SEGGER_RTT_ConfigUpBuffer(LOG_RTT_BUFFER_INDEX, LOG_RTT_BUFFER_NAME, sLogBuffer, LOG_RTT_BUFFER_SIZE,
                                        SEGGER_RTT_MODE_NO_BLOCK_TRIM);
    otEXPECT(res >= 0);

    sLogInitialized = true;

exit:
    return;
}

void nrf5LogDeinit()
{
    sLogInitialized = false;
}

OT_TOOL_WEAK void otPlatLog(otLogLevel aLogLevel, otLogRegion aLogRegion, const char *aFormat, ...)
{
    (void)aLogRegion;

    uint16_t length = 0;
    int      charsWritten;
    char     logString[LOG_PARSE_BUFFER_SIZE + 1];

    otEXPECT(sLogInitialized == true);

#if (LOG_TIMESTAMP_ENABLE == 1)
    length += logTimestamp(logString, LOG_PARSE_BUFFER_SIZE);
#endif

    // Add level information.
    length += logLevel(&logString[length], (LOG_PARSE_BUFFER_SIZE - length), aLogLevel);

    // Parse user string.
    va_list paramList;
    va_start(paramList, aFormat);
    charsWritten = vsnprintf(&logString[length], (LOG_PARSE_BUFFER_SIZE - length), aFormat, paramList);
    otEXPECT(charsWritten >= 0);
    length += charsWritten;

    if (length > LOG_PARSE_BUFFER_SIZE)
    {
        length = LOG_PARSE_BUFFER_SIZE;
    }

    logString[length++] = '\n';
    va_end(paramList);

    // Write user log to the RTT memory block.
    SEGGER_RTT_WriteNoLock(0, logString, length);

exit:
    return;
}

#endif // (OPENTHREAD_CONFIG_LOG_OUTPUT == OPENTHREAD_CONFIG_LOG_OUTPUT_PLATFORM_DEFINED)
