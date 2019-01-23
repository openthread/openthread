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

#include "openthread-core-config.h"
#include "platform-posix.h"

#include <assert.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <syslog.h>

#include <openthread/platform/logging.h>

#include "code_utils.h"

#define LOGGING_MAX_LOG_STRING_SIZE 512

void platformLoggingInit(const char *aName)
{
#if (OPENTHREAD_CONFIG_LOG_OUTPUT == OPENTHREAD_CONFIG_LOG_OUTPUT_PLATFORM_DEFINED) || \
    (OPENTHREAD_CONFIG_LOG_OUTPUT == OPENTHREAD_CONFIG_LOG_OUTPUT_NCP_SPINEL)

    openlog(aName, LOG_PID, LOG_USER);
    setlogmask(setlogmask(0) & LOG_UPTO(LOG_DEBUG));

#else
    OT_UNUSED_VARIABLE(aName);
#endif
}

#if (OPENTHREAD_CONFIG_LOG_OUTPUT == OPENTHREAD_CONFIG_LOG_OUTPUT_PLATFORM_DEFINED) || \
    (OPENTHREAD_CONFIG_LOG_OUTPUT == OPENTHREAD_CONFIG_LOG_OUTPUT_NCP_SPINEL)
OT_TOOL_WEAK void otPlatLog(otLogLevel aLogLevel, otLogRegion aLogRegion, const char *aFormat, ...)
{
    OT_UNUSED_VARIABLE(aLogRegion);

    char         logString[LOGGING_MAX_LOG_STRING_SIZE];
    int          charsWritten;
    va_list      args;
    int          logLevel;
    unsigned int offset = 0;

    charsWritten = snprintf(&logString[offset], sizeof(logString), "[%" PRIx64 "] ", gNodeId);
    otEXPECT_ACTION(charsWritten >= 0, logString[offset] = 0);
    offset += (unsigned int)charsWritten;
    otEXPECT_ACTION(offset < sizeof(logString), logString[sizeof(logString) - 1] = 0);

    va_start(args, aFormat);
    charsWritten = vsnprintf(&logString[offset], sizeof(logString) - offset, aFormat, args);
    va_end(args);

    otEXPECT_ACTION(charsWritten >= 0, logString[offset] = 0);

exit:
    switch (aLogLevel)
    {
    case OT_LOG_LEVEL_NONE:
        logLevel = LOG_ALERT;
        break;
    case OT_LOG_LEVEL_CRIT:
        logLevel = LOG_CRIT;
        break;
    case OT_LOG_LEVEL_WARN:
        logLevel = LOG_WARNING;
        break;
    case OT_LOG_LEVEL_NOTE:
        logLevel = LOG_NOTICE;
        break;
    case OT_LOG_LEVEL_INFO:
        logLevel = LOG_INFO;
        break;
    case OT_LOG_LEVEL_DEBG:
        logLevel = LOG_DEBUG;
        break;
    default:
        assert(false);
        logLevel = LOG_DEBUG;
        break;
    }
    syslog(logLevel, "%s", logString);
}

#endif // #if (OPENTHREAD_CONFIG_LOG_OUTPUT == OPENTHREAD_CONFIG_LOG_OUTPUT_PLATFORM_DEFINED)
