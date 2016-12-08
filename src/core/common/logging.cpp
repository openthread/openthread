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

#include <common/logging.hpp>

#ifndef WINDOWS_LOGGING
#define otLogDump(aFormat, ...) otPlatLog(aLogLevel, aLogRegion, aFormat, ## __VA_ARGS__)
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * This static method outputs a line of the memory dump.
 *
 * @param[in]  aLogLevel   The log level.
 * @param[in]  aLogRegion  The log region.
 * @param[in]  aBuf        A pointer to the buffer.
 * @param[in]  aLength     Number of bytes in the buffer.
 *
 */
static void DumpLine(otLogLevel aLogLevel, otLogRegion aLogRegion, const void *aBuf, const size_t aLength)
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
}

void otDump(otLogLevel aLogLevel, otLogRegion aLogRegion, const char *aId, const void *aBuf, const size_t aLength)
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
        DumpLine(aLogLevel, aLogRegion, (uint8_t *)(aBuf) + i, (aLength - i) < 16 ? (aLength - i) : 16);
    }

    cur = buf;

    for (size_t i = 0; i < width; i++)
    {
        snprintf(cur, sizeof(buf) - static_cast<size_t>(cur - buf), "-");
        cur += strlen(cur);
    }

    otLogDump("%s", buf);
}

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

#ifdef __cplusplus
};
#endif
