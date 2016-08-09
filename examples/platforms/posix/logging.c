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

#include <ctype.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

#include <common/code_utils.hpp>
#include <platform/logging.h>

// Macro to append content to end of the log string.

#define LOG_PRINTF(...)                                                                     \
    charsWritten = snprintf(&logString[index], sizeof(logString) - index , __VA_ARGS__);    \
    VerifyOrExit(charsWritten >= 0, logString[index] = 0);                                  \
    index += (unsigned int)charsWritten;                                    \
    VerifyOrExit(index < sizeof(logString), logString[sizeof(logString) -1 ] = 0)

void otPlatLog(otLogLevel aLogLevel, otLogRegion aLogRegion, const char *aFormat, ...)
{
    struct timeval tv;
    char timeString[40];
    char logString[512];
    unsigned int index;
    int charsWritten;
    va_list args;

    index = 0;

    gettimeofday(&tv, NULL);
    strftime(timeString, sizeof(timeString), "%Y-%m-%d %H:%M:%S", localtime(&tv.tv_sec));

    LOG_PRINTF("%s.%06d ", timeString, (uint32_t)tv.tv_usec);

    switch (aLogLevel)
    {
    case kLogLevelNone:
        LOG_PRINTF("NONE ");
        break;

    case kLogLevelCrit:
        LOG_PRINTF("CRIT ");
        break;

    case kLogLevelWarn:
        LOG_PRINTF("WARN ");
        break;

    case kLogLevelInfo:
        LOG_PRINTF("INFO ");
        break;

    case kLogLevelDebg:
        LOG_PRINTF("DEBG ");
        break;
    }

    switch (aLogRegion)
    {
    case kLogRegionApi:
        LOG_PRINTF("API  ");
        break;

    case kLogRegionMle:
        LOG_PRINTF("MLE  ");
        break;

    case kLogRegionArp:
        LOG_PRINTF("ARP  ");
        break;

    case kLogRegionNetData:
        LOG_PRINTF("NETD ");
        break;

    case kLogRegionIp6:
        LOG_PRINTF("IPV6 ");
        break;

    case kLogRegionIcmp:
        LOG_PRINTF("ICMP ");
        break;

    case kLogRegionMac:
        LOG_PRINTF("MAC  ");
        break;

    case kLogRegionMem:
        LOG_PRINTF("MEM  ");
        break;

    case kLogRegionNcp:
        LOG_PRINTF("NCP  ");
        break;

    case kLogRegionMeshCoP:
        LOG_PRINTF("MCOP ");
        break;
    }

    va_start(args, aFormat);
    charsWritten = vsnprintf(&logString[index], sizeof(logString) - index, aFormat, args);
    va_end(args);

    VerifyOrExit(charsWritten >= 0, logString[index] = 0);

exit:
    fprintf(stderr, "%s\r", logString);
}

