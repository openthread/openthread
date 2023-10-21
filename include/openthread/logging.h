/*
 *  Copyright (c) 2016-2018, The OpenThread Authors.
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
 *   This file includes OpenThread logging related definitions.
 */

#ifndef OPENTHREAD_LOGGING_H_
#define OPENTHREAD_LOGGING_H_

#include <openthread/error.h>
#include <openthread/platform/logging.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup api-logging
 *
 * @brief
 *   This module includes OpenThread logging related definitions.
 *
 * @{
 *
 */

/**
 * Returns the current log level.
 *
 * If dynamic log level feature `OPENTHREAD_CONFIG_LOG_LEVEL_DYNAMIC_ENABLE` is enabled, this function returns the
 * currently set dynamic log level. Otherwise, this function returns the build-time configured log level.
 *
 * @returns The log level.
 *
 */
otLogLevel otLoggingGetLevel(void);

/**
 * Sets the log level.
 *
 * @note This function requires `OPENTHREAD_CONFIG_LOG_LEVEL_DYNAMIC_ENABLE=1`.
 *
 * @param[in]  aLogLevel               The log level.
 *
 * @retval OT_ERROR_NONE            Successfully updated log level.
 * @retval OT_ERROR_INVALID_ARGS    Log level value is invalid.
 *
 */
otError otLoggingSetLevel(otLogLevel aLogLevel);

/**
 * Emits a log message at critical log level.
 *
 * Is intended for use by platform. If `OPENTHREAD_CONFIG_LOG_PLATFORM` is not set or the current log
 * level is below critical, this function does not emit any log message.
 *
 * @param[in]  aFormat  The format string.
 * @param[in]  ...      Arguments for the format specification.
 *
 */
void otLogCritPlat(const char *aFormat, ...) OT_TOOL_PRINTF_STYLE_FORMAT_ARG_CHECK(1, 2);

/**
 * Emits a log message at warning log level.
 *
 * Is intended for use by platform. If `OPENTHREAD_CONFIG_LOG_PLATFORM` is not set or the current log
 * level is below warning, this function does not emit any log message.
 *
 * @param[in]  aFormat  The format string.
 * @param[in]  ...      Arguments for the format specification.
 *
 */
void otLogWarnPlat(const char *aFormat, ...) OT_TOOL_PRINTF_STYLE_FORMAT_ARG_CHECK(1, 2);

/**
 * Emits a log message at note log level.
 *
 * Is intended for use by platform. If `OPENTHREAD_CONFIG_LOG_PLATFORM` is not set or the current log
 * level is below note, this function does not emit any log message.
 *
 * @param[in]  aFormat  The format string.
 * @param[in]  ...      Arguments for the format specification.
 *
 */
void otLogNotePlat(const char *aFormat, ...) OT_TOOL_PRINTF_STYLE_FORMAT_ARG_CHECK(1, 2);

/**
 * Emits a log message at info log level.
 *
 * Is intended for use by platform. If `OPENTHREAD_CONFIG_LOG_PLATFORM` is not set or the current log
 * level is below info, this function does not emit any log message.
 *
 * @param[in]  aFormat  The format string.
 * @param[in]  ...      Arguments for the format specification.
 *
 */
void otLogInfoPlat(const char *aFormat, ...) OT_TOOL_PRINTF_STYLE_FORMAT_ARG_CHECK(1, 2);

/**
 * Emits a log message at debug log level.
 *
 * Is intended for use by platform. If `OPENTHREAD_CONFIG_LOG_PLATFORM` is not set or the current log
 * level is below debug, this function does not emit any log message.
 *
 * @param[in]  aFormat  The format string.
 * @param[in]  ...      Arguments for the format specification.
 *
 */
void otLogDebgPlat(const char *aFormat, ...) OT_TOOL_PRINTF_STYLE_FORMAT_ARG_CHECK(1, 2);

/**
 * Generates a memory dump at critical log level.
 *
 * If `OPENTHREAD_CONFIG_LOG_PLATFORM` or `OPENTHREAD_CONFIG_LOG_PKT_DUMP` is not set or the current log level is below
 * critical this function does not emit any log message.
 *
 * @param[in]  aText         A string that is printed before the bytes.
 * @param[in]  aData         A pointer to the data buffer.
 * @param[in]  aDataLength   Number of bytes in @p aData.
 *
 */
void otDumpCritPlat(const char *aText, const void *aData, uint16_t aDataLength);

/**
 * Generates a memory dump at warning log level.
 *
 * If `OPENTHREAD_CONFIG_LOG_PLATFORM` or `OPENTHREAD_CONFIG_LOG_PKT_DUMP` is not set or the current log level is below
 * warning this function does not emit any log message.
 *
 * @param[in]  aText         A string that is printed before the bytes.
 * @param[in]  aData         A pointer to the data buffer.
 * @param[in]  aDataLength   Number of bytes in @p aData.
 *
 */
void otDumpWarnPlat(const char *aText, const void *aData, uint16_t aDataLength);

/**
 * Generates a memory dump at note log level.
 *
 * If `OPENTHREAD_CONFIG_LOG_PLATFORM` or `OPENTHREAD_CONFIG_LOG_PKT_DUMP` is not set or the current log level is below
 * note this function does not emit any log message.
 *
 * @param[in]  aText         A string that is printed before the bytes.
 * @param[in]  aData         A pointer to the data buffer.
 * @param[in]  aDataLength   Number of bytes in @p aData.
 *
 */
void otDumpNotePlat(const char *aText, const void *aData, uint16_t aDataLength);

/**
 * Generates a memory dump at info log level.
 *
 * If `OPENTHREAD_CONFIG_LOG_PLATFORM` or `OPENTHREAD_CONFIG_LOG_PKT_DUMP` is not set or the current log level is below
 * info this function does not emit any log message.
 *
 * @param[in]  aText         A string that is printed before the bytes.
 * @param[in]  aData         A pointer to the data buffer.
 * @param[in]  aDataLength   Number of bytes in @p aData.
 *
 */
void otDumpInfoPlat(const char *aText, const void *aData, uint16_t aDataLength);

/**
 * Generates a memory dump at debug log level.
 *
 * If `OPENTHREAD_CONFIG_LOG_PLATFORM` or `OPENTHREAD_CONFIG_LOG_PKT_DUMP` is not set or the current log level is below
 * debug this function does not emit any log message.
 *
 * @param[in]  aText         A string that is printed before the bytes.
 * @param[in]  aData         A pointer to the data buffer.
 * @param[in]  aDataLength   Number of bytes in @p aData.
 *
 */
void otDumpDebgPlat(const char *aText, const void *aData, uint16_t aDataLength);

/**
 * Emits a log message at given log level using a platform module name.
 *
 * This is is intended for use by platform. If `OPENTHREAD_CONFIG_LOG_PLATFORM` is not set or the current log
 * level is below @p aLogLevel , this function does not emit any log message.
 *
 * The @p aPlatModuleName name is used to determine the log module name in the emitted log message, following the
 * `P-{PlatModuleName}---` format. This means that the prefix string "P-" is added to indicate that this is a platform
 * sub-module, followed by the next 12 characters of the @p PlatModuleName string, with padded hyphens `-` at the end
 * to ensure that the region name is 14 characters long.

 * @param[in] aLogLevel         The log level.
 * @param[in] aPlatModuleName   The platform sub-module name.
 * @param[in] aFormat           The format string.
 * @param[in] ...               Arguments for the format specification.
 *
 */
void otLogPlat(otLogLevel aLogLevel, const char *aPlatModuleName, const char *aFormat, ...)
    OT_TOOL_PRINTF_STYLE_FORMAT_ARG_CHECK(3, 4);

/**
 * Emits a log message at given log level using a platform module name.
 *
 * This is is intended for use by platform. If `OPENTHREAD_CONFIG_LOG_PLATFORM` is not set or the current log
 * level is below @p aLogLevel , this function does not emit any log message.
 *
 * The @p aPlatModuleName name is used to determine the log module name in the emitted log message, following the
 * `P-{PlatModuleName}---` format. This means that the prefix string "P-" is added to indicate that this is a platform
 * sub-module, followed by the next 12 characters of the @p PlatModuleName string, with padded hyphens `-` at the end
 * to ensure that the region name is 14 characters long.
 *
 * @param[in] aLogLevel         The log level.
 * @param[in] aPlatModuleName   The platform sub-module name.
 * @param[in] aFormat           The format string.
 * @param[in] aArgs             Arguments for the format specification.
 *
 */
void otLogPlatArgs(otLogLevel aLogLevel, const char *aPlatModuleName, const char *aFormat, va_list aArgs);

/**
 * Emits a log message at a given log level.
 *
 * Is intended for use by CLI only. If `OPENTHREAD_CONFIG_LOG_CLI` is not set or the current log
 * level is below the given log level, this function does not emit any log message.
 *
 * @param[in]  aLogLevel The log level.
 * @param[in]  aFormat   The format string.
 * @param[in]  ...       Arguments for the format specification.
 *
 */
void otLogCli(otLogLevel aLogLevel, const char *aFormat, ...) OT_TOOL_PRINTF_STYLE_FORMAT_ARG_CHECK(2, 3);

#define OT_LOG_HEX_DUMP_LINE_SIZE 73 ///< Hex dump line string size.

/**
 * Represents information used for generating hex dump output.
 *
 */
typedef struct
{
    const uint8_t *mDataBytes;                       ///< The data byes.
    uint16_t       mDataLength;                      ///< The data length (number of bytes in @p mDataBytes)
    const char    *mTitle;                           ///< Title string to add table header (MUST NOT be `NULL`).
    char           mLine[OT_LOG_HEX_DUMP_LINE_SIZE]; ///< Buffer to output one line of generated hex dump.
    uint16_t       mIterator;                        ///< Iterator used by OT stack. MUST be initialized to zero.
} otLogHexDumpInfo;

/**
 * Generates the next hex dump line.
 *
 * Can call this method back-to-back to generate the hex dump output line by line. On the first call the `mIterator`
 * field in @p aInfo MUST be set to zero.
 *
 * Here is an example of the generated hex dump output:
 *
 *  "==========================[{mTitle} len=070]============================"
 *  "| 41 D8 87 34 12 FF FF 25 | 4C 57 DA F2 FB 2F 62 7F | A..4...%LW.../b. |"
 *  "| 3B 01 F0 4D 4C 4D 4C 54 | 4F 00 15 15 00 00 00 00 | ;..MLMLTO....... |"
 *  "| 00 00 00 01 80 DB 60 82 | 7E 33 72 3B CC B3 A1 84 | ......`.~3r;.... |"
 *  "| 3B E6 AD B2 0B 45 E7 45 | C5 B9 00 1A CB 2D 6D 1C | ;....E.E.....-m. |"
 *  "| 10 3E 3C F5 D3 70       |                         | .><..p           |"
 *  "------------------------------------------------------------------------"
 *
 * @param[in,out] aInfo        A pointer to `otLogHexDumpInfo` to use to generate hex dump.
 *
 * @retval OT_ERROR_NONE       Successfully generated the next line, `mLine` field in @p aInfo is updated.
 * @retval OT_ERROR_NOT_FOUND  Reached the end and no more line to generate.
 *
 */
otError otLogGenerateNextHexDumpLine(otLogHexDumpInfo *aInfo);

/**
 * @}
 *
 */

#ifdef __cplusplus
} // extern "C"
#endif

#endif // OPENTHREAD_LOGGING_H_
