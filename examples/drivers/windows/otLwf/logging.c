/*
 *  Copyright (c) 2016, Microsoft Corporation.
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
 *  This file implements the logging function required for the OpenThread library.
 */

#include "precomp.h"
#include "logging.tmh"

void otPlatLog(otLogLevel aLogLevel, otLogRegion aLogRegion, const char *aFormat, ...)
{
    char szLogMessage[256] = { 0 };
    va_list args;
    
    va_start(args, aFormat);
    sprintf_s(szLogMessage, sizeof(szLogMessage), aFormat, args);
    va_end(args);

    switch (aLogLevel)
    {
    case kLogLevelNone:
        switch (aLogRegion)
        {
        case kLogRegionApi:     TraceEvents(TRACE_LEVEL_NONE, OT_API,  "API  %s", szLogMessage); break;
        case kLogRegionMle:     TraceEvents(TRACE_LEVEL_NONE, OT_MLE,  "MLE  %s", szLogMessage); break;
        case kLogRegionArp:     TraceEvents(TRACE_LEVEL_NONE, OT_ARP,  "ARP  %s", szLogMessage); break;
        case kLogRegionNetData: TraceEvents(TRACE_LEVEL_NONE, OT_NETD, "NETD %s", szLogMessage); break;
        case kLogRegionIcmp:    TraceEvents(TRACE_LEVEL_NONE, OT_IPV6, "IPV6 %s", szLogMessage); break;
        case kLogRegionIp6:     TraceEvents(TRACE_LEVEL_NONE, OT_ICMP, "ICMP %s", szLogMessage); break;
        case kLogRegionMac:     TraceEvents(TRACE_LEVEL_NONE, OT_MAC,  "MAC  %s", szLogMessage); break;
        case kLogRegionMem:     TraceEvents(TRACE_LEVEL_NONE, OT_MEM,  "MEM  %s", szLogMessage); break;
        }
        break;
    case kLogLevelCrit:
        switch (aLogRegion)
        {
        case kLogRegionApi:     TraceEvents(TRACE_LEVEL_CRITICAL, OT_API,  "API  %s", szLogMessage); break;
        case kLogRegionMle:     TraceEvents(TRACE_LEVEL_CRITICAL, OT_MLE,  "MLE  %s", szLogMessage); break;
        case kLogRegionArp:     TraceEvents(TRACE_LEVEL_CRITICAL, OT_ARP,  "ARP  %s", szLogMessage); break;
        case kLogRegionNetData: TraceEvents(TRACE_LEVEL_CRITICAL, OT_NETD, "NETD %s", szLogMessage); break;
        case kLogRegionIcmp:    TraceEvents(TRACE_LEVEL_CRITICAL, OT_IPV6, "IPV6 %s", szLogMessage); break;
        case kLogRegionIp6:     TraceEvents(TRACE_LEVEL_CRITICAL, OT_ICMP, "ICMP %s", szLogMessage); break;
        case kLogRegionMac:     TraceEvents(TRACE_LEVEL_CRITICAL, OT_MAC,  "MAC  %s", szLogMessage); break;
        case kLogRegionMem:     TraceEvents(TRACE_LEVEL_CRITICAL, OT_MEM,  "MEM  %s", szLogMessage); break;
        }
        break;
    case kLogLevelWarn:
        switch (aLogRegion)
        {
        case kLogRegionApi:     TraceEvents(TRACE_LEVEL_WARNING, OT_API,  "API  %s", szLogMessage); break;
        case kLogRegionMle:     TraceEvents(TRACE_LEVEL_WARNING, OT_MLE,  "MLE  %s", szLogMessage); break;
        case kLogRegionArp:     TraceEvents(TRACE_LEVEL_WARNING, OT_ARP,  "ARP  %s", szLogMessage); break;
        case kLogRegionNetData: TraceEvents(TRACE_LEVEL_WARNING, OT_NETD, "NETD %s", szLogMessage); break;
        case kLogRegionIcmp:    TraceEvents(TRACE_LEVEL_WARNING, OT_IPV6, "IPV6 %s", szLogMessage); break;
        case kLogRegionIp6:     TraceEvents(TRACE_LEVEL_WARNING, OT_ICMP, "ICMP %s", szLogMessage); break;
        case kLogRegionMac:     TraceEvents(TRACE_LEVEL_WARNING, OT_MAC,  "MAC  %s", szLogMessage); break;
        case kLogRegionMem:     TraceEvents(TRACE_LEVEL_WARNING, OT_MEM,  "MEM  %s", szLogMessage); break;
        }
        break;
    case kLogLevelInfo:
        switch (aLogRegion)
        {
        case kLogRegionApi:     TraceEvents(TRACE_LEVEL_INFORMATION, OT_API,  "API  %s", szLogMessage); break;
        case kLogRegionMle:     TraceEvents(TRACE_LEVEL_INFORMATION, OT_MLE,  "MLE  %s", szLogMessage); break;
        case kLogRegionArp:     TraceEvents(TRACE_LEVEL_INFORMATION, OT_ARP,  "ARP  %s", szLogMessage); break;
        case kLogRegionNetData: TraceEvents(TRACE_LEVEL_INFORMATION, OT_NETD, "NETD %s", szLogMessage); break;
        case kLogRegionIcmp:    TraceEvents(TRACE_LEVEL_INFORMATION, OT_IPV6, "IPV6 %s", szLogMessage); break;
        case kLogRegionIp6:     TraceEvents(TRACE_LEVEL_INFORMATION, OT_ICMP, "ICMP %s", szLogMessage); break;
        case kLogRegionMac:     TraceEvents(TRACE_LEVEL_INFORMATION, OT_MAC,  "MAC  %s", szLogMessage); break;
        case kLogRegionMem:     TraceEvents(TRACE_LEVEL_INFORMATION, OT_MEM,  "MEM  %s", szLogMessage); break;
        }
        break;
    case kLogLevelDebg:
        switch (aLogRegion)
        {
        case kLogRegionApi:     TraceEvents(TRACE_LEVEL_VERBOSE, OT_API,  "API  %s", szLogMessage); break;
        case kLogRegionMle:     TraceEvents(TRACE_LEVEL_VERBOSE, OT_MLE,  "MLE  %s", szLogMessage); break;
        case kLogRegionArp:     TraceEvents(TRACE_LEVEL_VERBOSE, OT_ARP,  "ARP  %s", szLogMessage); break;
        case kLogRegionNetData: TraceEvents(TRACE_LEVEL_VERBOSE, OT_NETD, "NETD %s", szLogMessage); break;
        case kLogRegionIcmp:    TraceEvents(TRACE_LEVEL_VERBOSE, OT_IPV6, "IPV6 %s", szLogMessage); break;
        case kLogRegionIp6:     TraceEvents(TRACE_LEVEL_VERBOSE, OT_ICMP, "ICMP %s", szLogMessage); break;
        case kLogRegionMac:     TraceEvents(TRACE_LEVEL_VERBOSE, OT_MAC,  "MAC  %s", szLogMessage); break;
        case kLogRegionMem:     TraceEvents(TRACE_LEVEL_VERBOSE, OT_MEM,  "MEM  %s", szLogMessage); break;
        }
        break;
    }
}

