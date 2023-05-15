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

#include "platform-simulation.h"
#include <openthread-core-config.h>
#include <openthread/config.h>

#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <syslog.h>

#include <openthread/platform/logging.h>
#include <openthread/platform/toolchain.h>

#include "utils/code_utils.h"

#if (OPENTHREAD_CONFIG_LOG_OUTPUT == OPENTHREAD_CONFIG_LOG_OUTPUT_PLATFORM_DEFINED)

static FILE *sLogFile = NULL;

void platformLoggingSetFileName(const char *aName)
{
    if (sLogFile != NULL)
    {
        fclose(sLogFile);
    }

    sLogFile = fopen(aName, "wt");

    if (sLogFile == NULL)
    {
        fprintf(stderr, "Failed to open log file '%s': %s\r\n", aName, strerror(errno));
        exit(EXIT_FAILURE);
    }
}

void platformLoggingInit(const char *aName)
{
    if (sLogFile == NULL)
    {
        openlog(aName, LOG_PID, LOG_USER);
        setlogmask(setlogmask(0) & LOG_UPTO(LOG_NOTICE));
    }
    else
    {
        fprintf(sLogFile, "OpenThread logs\r\n");
        fprintf(sLogFile, "- Program:  %s\r\n", aName);
        fprintf(sLogFile, "- Platform: simulation\r\n");
        fprintf(sLogFile, "- Node ID:  %lu\r\n", (unsigned long)gNodeId);
        fprintf(sLogFile, "\r\n");
    }
}

void platformLoggingDeinit(void)
{
    if (sLogFile != NULL)
    {
        fclose(sLogFile);
        sLogFile = NULL;
    }
}

void otPlatLog(otLogLevel aLogLevel, otLogRegion aLogRegion, const char *aFormat, ...)
{
    OT_UNUSED_VARIABLE(aLogLevel);
    OT_UNUSED_VARIABLE(aLogRegion);

    va_list args;

    va_start(args, aFormat);

    if (sLogFile == NULL)
    {
        char logString[512];
        int  offset;

        offset = snprintf(logString, sizeof(logString), "[%lu]", (unsigned long)gNodeId);

        vsnprintf(&logString[offset], sizeof(logString) - (uint16_t)offset, aFormat, args);
        syslog(LOG_CRIT, "%s", logString);
    }
    else
    {
        vfprintf(sLogFile, aFormat, args);
        fprintf(sLogFile, "\r\n");
    }

    va_end(args);
}

#else

void platformLoggingInit(const char *aName) { OT_UNUSED_VARIABLE(aName); }
void platformLoggingDeinit(void) {}

#endif // (OPENTHREAD_CONFIG_LOG_OUTPUT == OPENTHREAD_CONFIG_LOG_OUTPUT_PLATFORM_DEFINED)
