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

#include "logging.hpp"

#include "common/instance.hpp"

/*
 * Verify debug uart dependency.
 *
 * It is reasonable to only enable the debug uart and not enable logs to the DEBUG uart.
 */
#if (OPENTHREAD_CONFIG_LOG_OUTPUT == OPENTHREAD_CONFIG_LOG_OUTPUT_DEBUG_UART) && (!OPENTHREAD_CONFIG_ENABLE_DEBUG_UART)
#error OPENTHREAD_CONFIG_ENABLE_DEBUG_UART_LOG requires OPENTHREAD_CONFIG_ENABLE_DEBUG_UART
#endif

#ifndef WINDOWS_LOGGING
#define otLogDump(aFormat, ...) \
    _otDynamicLog(aLogLevel, aLogRegion, aFormat OPENTHREAD_CONFIG_LOG_SUFFIX, ##__VA_ARGS__)
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
static void DumpLine(otLogLevel aLogLevel, otLogRegion aLogRegion, const void *aBuf, const size_t aLength)
{
    char  buf[80];
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
    size_t       idlen = strlen(aId);
    const size_t width = 72;
    char         buf[80];
    char *       cur = buf;

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
#else  // OPENTHREAD_CONFIG_LOG_PKT_DUMP
void otDump(otLogLevel, otLogRegion, const char *, const void *, const size_t)
{
}
#endif // OPENTHREAD_CONFIG_LOG_PKT_DUMP

const char *otThreadErrorToString(otError aError)
{
    const char *retval;

    switch (aError)
    {
    case OT_ERROR_NONE:
        retval = "OK";
        break;

    case OT_ERROR_FAILED:
        retval = "Failed";
        break;

    case OT_ERROR_DROP:
        retval = "Drop";
        break;

    case OT_ERROR_NO_BUFS:
        retval = "NoBufs";
        break;

    case OT_ERROR_NO_ROUTE:
        retval = "NoRoute";
        break;

    case OT_ERROR_BUSY:
        retval = "Busy";
        break;

    case OT_ERROR_PARSE:
        retval = "Parse";
        break;

    case OT_ERROR_INVALID_ARGS:
        retval = "InvalidArgs";
        break;

    case OT_ERROR_SECURITY:
        retval = "Security";
        break;

    case OT_ERROR_ADDRESS_QUERY:
        retval = "AddressQuery";
        break;

    case OT_ERROR_NO_ADDRESS:
        retval = "NoAddress";
        break;

    case OT_ERROR_ABORT:
        retval = "Abort";
        break;

    case OT_ERROR_NOT_IMPLEMENTED:
        retval = "NotImplemented";
        break;

    case OT_ERROR_INVALID_STATE:
        retval = "InvalidState";
        break;

    case OT_ERROR_NO_ACK:
        retval = "NoAck";
        break;

    case OT_ERROR_CHANNEL_ACCESS_FAILURE:
        retval = "ChannelAccessFailure";
        break;

    case OT_ERROR_DETACHED:
        retval = "Detached";
        break;

    case OT_ERROR_FCS:
        retval = "FcsErr";
        break;

    case OT_ERROR_NO_FRAME_RECEIVED:
        retval = "NoFrameReceived";
        break;

    case OT_ERROR_UNKNOWN_NEIGHBOR:
        retval = "UnknownNeighbor";
        break;

    case OT_ERROR_INVALID_SOURCE_ADDRESS:
        retval = "InvalidSourceAddress";
        break;

    case OT_ERROR_ADDRESS_FILTERED:
        retval = "AddressFiltered";
        break;

    case OT_ERROR_DESTINATION_ADDRESS_FILTERED:
        retval = "DestinationAddressFiltered";
        break;

    case OT_ERROR_NOT_FOUND:
        retval = "NotFound";
        break;

    case OT_ERROR_ALREADY:
        retval = "Already";
        break;

    case OT_ERROR_IP6_ADDRESS_CREATION_FAILURE:
        retval = "Ipv6AddressCreationFailure";
        break;

    case OT_ERROR_NOT_CAPABLE:
        retval = "NotCapable";
        break;

    case OT_ERROR_RESPONSE_TIMEOUT:
        retval = "ResponseTimeout";
        break;

    case OT_ERROR_DUPLICATED:
        retval = "Duplicated";
        break;

    case OT_ERROR_REASSEMBLY_TIMEOUT:
        retval = "ReassemblyTimeout";
        break;

    case OT_ERROR_NOT_TMF:
        retval = "NotTmf";
        break;

    case OT_ERROR_NOT_LOWPAN_DATA_FRAME:
        retval = "NonLowpanDataFrame";
        break;

    case OT_ERROR_DISABLED_FEATURE:
        retval = "DisabledFeature";
        break;

    case OT_ERROR_GENERIC:
        retval = "GenericError";
        break;

    case OT_ERROR_LINK_MARGIN_LOW:
        retval = "LinkMarginLow";
        break;

    default:
        retval = "UnknownErrorType";
        break;
    }

    return retval;
}

const char *otLogLevelToPrefixString(otLogLevel aLogLevel)
{
    const char *retval = "";

    switch (aLogLevel)
    {
    case OT_LOG_LEVEL_NONE:
        retval = _OT_LEVEL_NONE_PREFIX;
        break;

    case OT_LOG_LEVEL_CRIT:
        retval = _OT_LEVEL_CRIT_PREFIX;
        break;

    case OT_LOG_LEVEL_WARN:
        retval = _OT_LEVEL_WARN_PREFIX;
        break;

    case OT_LOG_LEVEL_NOTE:
        retval = _OT_LEVEL_NOTE_PREFIX;
        break;

    case OT_LOG_LEVEL_INFO:
        retval = _OT_LEVEL_INFO_PREFIX;
        break;

    case OT_LOG_LEVEL_DEBG:
        retval = _OT_LEVEL_DEBG_PREFIX;
        break;
    }

    return retval;
}

#if OPENTHREAD_CONFIG_LOG_OUTPUT == OPENTHREAD_CONFIG_LOG_OUTPUT_NONE
/* this provides a stub, incase something uses the function */
void otPlatLog(otLogLevel aLogLevel, otLogRegion aLogRegion, const char *aFormat, ...)
{
    OT_UNUSED_VARIABLE(aLogLevel);
    OT_UNUSED_VARIABLE(aLogRegion);
    OT_UNUSED_VARIABLE(aFormat);
}
#endif

#ifdef __cplusplus
};
#endif
