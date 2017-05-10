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
 *   This file implements the tasklet scheduler.
 */

#define WPP_NAME "logging.tmh"

#ifdef OPENTHREAD_CONFIG_FILE
#include OPENTHREAD_CONFIG_FILE
#else
#include <openthread-config.h>
#endif

#include "logging.hpp"

#include <openthread/openthread.h>

#ifndef WINDOWS_LOGGING
#define otLogDump(aFormat, ...)                                             \
    _otDynamicLog(aInstance, aLogLevel, aLogRegion, aFormat OPENTHREAD_CONFIG_LOG_SUFFIX, ## __VA_ARGS__)
#endif

#ifdef __cplusplus
extern "C" {
#endif

#if OPENTHREAD_CONFIG_LOG_PKT_DUMP == 1
/**
 * This static method outputs a line of the memory dump.
 *
 * @param[in]  aLogLevel   The log level.
 * @param[in]  aLogRegion  The log region.
 * @param[in]  aBuf        A pointer to the buffer.
 * @param[in]  aLength     Number of bytes in the buffer.
 *
 */
static void DumpLine(otInstance *aInstance, otLogLevel aLogLevel, otLogRegion aLogRegion, const void *aBuf,
                     const size_t aLength)
{
    char buf[80];
    char *cur = buf;

    snprintf(cur, sizeof(buf) - static_cast<size_t>(cur - buf), "|");
    cur += strlen(cur);

    for (size_t i = 0; i < 16; i++)
    {
        if (i < aLength)
        {
            snprintf(cur, sizeof(buf) - static_cast<size_t>(cur - buf), " %02X", ((uint8_t *)(aBuf))[i]);
            cur += strlen(cur);
        }
        else
        {
            snprintf(cur, sizeof(buf) - static_cast<size_t>(cur - buf), " ..");
            cur += strlen(cur);
        }

        if (!((i + 1) % 8))
        {
            snprintf(cur, sizeof(buf) - static_cast<size_t>(cur - buf), " |");
            cur += strlen(cur);
        }
    }

    snprintf(cur, sizeof(buf) - static_cast<size_t>(cur - buf), " ");
    cur += strlen(cur);

    for (size_t i = 0; i < 16; i++)
    {
        char c = 0x7f & ((char *)(aBuf))[i];

        if (i < aLength && isprint(c))
        {
            snprintf(cur, sizeof(buf) - static_cast<size_t>(cur - buf), "%c", c);
            cur += strlen(cur);
        }
        else
        {
            snprintf(cur, sizeof(buf) - static_cast<size_t>(cur - buf), ".");
            cur += strlen(cur);
        }
    }

    otLogDump("%s", buf);

    (void)aInstance;
}

void otDump(otInstance *aInstance, otLogLevel aLogLevel, otLogRegion aLogRegion, const char *aId, const void *aBuf,
            const size_t aLength)
{
    size_t idlen = strlen(aId);
    const size_t width = 72;
    char buf[80];
    char *cur = buf;

    for (size_t i = 0; i < (width - idlen) / 2 - 5; i++)
    {
        snprintf(cur, sizeof(buf) - static_cast<size_t>(cur - buf), "=");
        cur += strlen(cur);
    }

    snprintf(cur, sizeof(buf) - static_cast<size_t>(cur - buf), "[%s len=%03u]", aId, static_cast<uint16_t>(aLength));
    cur += strlen(cur);

    for (size_t i = 0; i < (width - idlen) / 2 - 4; i++)
    {
        snprintf(cur, sizeof(buf) - static_cast<size_t>(cur - buf), "=");
        cur += strlen(cur);
    }

    otLogDump("%s", buf);

    for (size_t i = 0; i < aLength; i += 16)
    {
        DumpLine(aInstance, aLogLevel, aLogRegion, (uint8_t *)(aBuf) + i, (aLength - i) < 16 ? (aLength - i) : 16);
    }

    cur = buf;

    for (size_t i = 0; i < width; i++)
    {
        snprintf(cur, sizeof(buf) - static_cast<size_t>(cur - buf), "-");
        cur += strlen(cur);
    }

    otLogDump("%s", buf);
}
#else // OPENTHREAD_CONFIG_LOG_PKT_DUMP
void otDump(otInstance *, otLogLevel, otLogRegion, const char *, const void *, const size_t) {}
#endif // OPENTHREAD_CONFIG_LOG_PKT_DUMP

#ifdef OPENTHREAD_CONFIG_LOG_PREPEND_LEVEL
const char *otLogLevelToString(otLogLevel aLevel)
{
    const char *retval;

    switch (aLevel)
    {
    case kLogLevelNone:
        retval = "NONE";
        break;

    case kLogLevelCrit:
        retval = "CRIT";
        break;

    case kLogLevelWarn:
        retval = "WARN";
        break;

    case kLogLevelInfo:
        retval = "INFO";
        break;

    case kLogLevelDebg:
        retval = "DEBG";
        break;

    default:
        retval = "----";
        break;
    }

    return retval;
}
#endif // OPENTHREAD_CONFIG_LOG_PREPEND_REGION

#ifdef OPENTHREAD_CONFIG_LOG_PREPEND_REGION
const char *otLogRegionToString(otLogRegion aRegion)
{
    const char *retval;

    switch (aRegion)
    {
    case kLogRegionApi:
        retval = "-API-----";
        break;

    case kLogRegionMle:
        retval = "-MLE-----";
        break;

    case kLogRegionArp:
        retval = "-ARP-----";
        break;

    case kLogRegionNetData:
        retval = "-N-DATA--";
        break;

    case kLogRegionIcmp:
        retval = "-ICMP----";
        break;

    case kLogRegionIp6:
        retval = "-IP6-----";
        break;

    case kLogRegionMac:
        retval = "-MAC-----";
        break;

    case kLogRegionMem:
        retval = "-MEM-----";
        break;

    case kLogRegionNcp:
        retval = "-NCP-----";
        break;

    case kLogRegionMeshCoP:
        retval = "-MESH-CP-";
        break;

    case kLogRegionNetDiag:
        retval = "-DIAG----";
        break;

    case kLogRegionPlatform:
        retval = "-PLAT----";
        break;

    default:
        retval = "---------";
        break;
    }

    return retval;
}
#endif // OPENTHREAD_CONFIG_LOG_PREPEND_REGION

const char *otThreadErrorToString(ThreadError aError)
{
    const char *retval;

    switch (aError)
    {
    case kThreadError_None:
        retval = "None";
        break;

    case kThreadError_Failed:
        retval = "Failed";
        break;

    case kThreadError_Drop:
        retval = "Drop";
        break;

    case kThreadError_NoBufs:
        retval = "NoBufs";
        break;

    case kThreadError_NoRoute:
        retval = "NoRoute";
        break;

    case kThreadError_Busy:
        retval = "Busy";
        break;

    case kThreadError_Parse:
        retval = "Parse";
        break;

    case kThreadError_InvalidArgs:
        retval = "InvalidArgs";
        break;

    case kThreadError_Security:
        retval = "Security";
        break;

    case kThreadError_AddressQuery:
        retval = "AddressQuery";
        break;

    case kThreadError_NoAddress:
        retval = "NoAddress";
        break;

    case kThreadError_NotReceiving:
        retval = "NotReceiving";
        break;

    case kThreadError_Abort:
        retval = "Abort";
        break;

    case kThreadError_NotImplemented:
        retval = "NotImplemented";
        break;

    case kThreadError_InvalidState:
        retval = "InvalidState";
        break;

    case kThreadError_NoTasklets:
        retval = "NoTasklets";
        break;

    case kThreadError_NoAck:
        retval = "NoAck";
        break;

    case kThreadError_ChannelAccessFailure:
        retval = "ChannelAccessFailure";
        break;

    case kThreadError_Detached:
        retval = "Detached";
        break;

    case kThreadError_FcsErr:
        retval = "FcsErr";
        break;

    case kThreadError_NoFrameReceived:
        retval = "NoFrameReceived";
        break;

    case kThreadError_UnknownNeighbor:
        retval = "UnknownNeighbor";
        break;

    case kThreadError_InvalidSourceAddress:
        retval = "InvalidSourceAddress";
        break;

    case kThreadError_WhitelistFiltered:
        retval = "WhitelistFiltered";
        break;

    case kThreadError_DestinationAddressFiltered:
        retval = "DestinationAddressFiltered";
        break;

    case kThreadError_NotFound:
        retval = "NotFound";
        break;

    case kThreadError_Already:
        retval = "Already";
        break;

    case kThreadError_BlacklistFiltered:
        retval = "BlacklistFiltered";
        break;

    case kThreadError_Ipv6AddressCreationFailure:
        retval = "Ipv6AddressCreationFailure";
        break;

    case kThreadError_NotCapable:
        retval = "NotCapable";
        break;

    case kThreadError_ResponseTimeout:
        retval = "ResponseTimeout";
        break;

    case kThreadError_Duplicated:
        retval = "Duplicated";
        break;

    case kThreadError_ReassemblyTimeout:
        retval = "ReassemblyTimeout";
        break;

    case kThreadError_NotTmf:
        retval = "NotTmf";
        break;

    case kThreadError_NonLowpanDataFrame:
        retval = "NonLowpanDataFrame";
        break;

    case kThreadError_DisabledFeature:
        retval = "DisabledFeature";
        break;

    case kThreadError_Error:
        retval = "GenericError";
        break;

    default:
        retval = "UnknownErrorType";
        break;
    }

    return retval;
}

#ifdef __cplusplus
};
#endif
