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

#include <windows.h>
#include <openthread.h>
#include <stdio.h>
#include <time.h>

#include <platform/logging.h>

EXTERN_C void otPlatLog(otLogLevel aLogLevel, otLogRegion aLogRegion, const char *aFormat, ...)
{
    char timeString[40];
    va_list args;

    time_t now = time(NULL);
    tm tmLocalTime;
    (void)localtime_s(&tmLocalTime, &now); 

    strftime(timeString, sizeof(timeString), "%Y-%m-%d %H:%M:%S ", &tmLocalTime);
    fprintf(stderr, "%s ", timeString);

    switch (aLogLevel)
    {
    case kLogLevelNone:
        fprintf(stderr, "NONE ");
        break;

    case kLogLevelCrit:
        fprintf(stderr, "CRIT ");
        break;

    case kLogLevelWarn:
        fprintf(stderr, "WARN ");
        break;

    case kLogLevelInfo:
        fprintf(stderr, "INFO ");
        break;

    case kLogLevelDebg:
        fprintf(stderr, "DEBG ");
        break;
    }

    switch (aLogRegion)
    {
    case kLogRegionApi:
        fprintf(stderr, "API  ");
        break;

    case kLogRegionMle:
        fprintf(stderr, "MLE  ");
        break;

    case kLogRegionArp:
        fprintf(stderr, "ARP  ");
        break;

    case kLogRegionNetData:
        fprintf(stderr, "NETD ");
        break;

    case kLogRegionIp6:
        fprintf(stderr, "IPV6 ");
        break;

    case kLogRegionIcmp:
        fprintf(stderr, "ICMP ");
        break;

    case kLogRegionMac:
        fprintf(stderr, "MAC  ");
        break;

    case kLogRegionMem:
        fprintf(stderr, "MEM  ");
        break;

    case kLogRegionNcp:
        fprintf(stderr, "NCP  ");
        break;

    case kLogRegionMeshCoP:
        fprintf(stderr, "MCOP ");
        break;
    }

    va_start(args, aFormat);
    vfprintf(stderr, aFormat, args);
    fprintf(stderr, "\r");
    va_end(args);
}

