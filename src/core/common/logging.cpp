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

#define otLogDump(aFormat, ...) _otDynamicLog(aLogLevel, aLogRegion, aFormat OPENTHREAD_CONFIG_LOG_SUFFIX, __VA_ARGS__)

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

    snprintf(cur, sizeof(buf) - static_cast<size_t>(cur - buf), "[%s len=%03u]", aId, static_cast<unsigned>(aLength));
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

static const char *const sThreadErrorStrings[OT_NUM_ERRORS] = {
    "OK",                         // OT_ERROR_NONE = 0
    "Failed",                     // OT_ERROR_FAILED = 1
    "Drop",                       // OT_ERROR_DROP = 2
    "NoBufs",                     // OT_ERROR_NO_BUFS = 3
    "NoRoute",                    // OT_ERROR_NO_ROUTE = 4
    "Busy",                       // OT_ERROR_BUSY = 5
    "Parse",                      // OT_ERROR_PARSE = 6
    "InvalidArgs",                // OT_ERROR_INVALID_ARGS = 7
    "Security",                   // OT_ERROR_SECURITY = 8
    "AddressQuery",               // OT_ERROR_ADDRESS_QUERY = 9
    "NoAddress",                  // OT_ERROR_NO_ADDRESS = 10
    "Abort",                      // OT_ERROR_ABORT = 11
    "NotImplemented",             // OT_ERROR_NOT_IMPLEMENTED = 12
    "InvalidState",               // OT_ERROR_INVALID_STATE = 13
    "NoAck",                      // OT_ERROR_NO_ACK = 14
    "ChannelAccessFailure",       // OT_ERROR_CHANNEL_ACCESS_FAILURE = 15
    "Detached",                   // OT_ERROR_DETACHED = 16
    "FcsErr",                     // OT_ERROR_FCS = 17
    "NoFrameReceived",            // OT_ERROR_NO_FRAME_RECEIVED = 18
    "UnknownNeighbor",            // OT_ERROR_UNKNOWN_NEIGHBOR = 19
    "InvalidSourceAddress",       // OT_ERROR_INVALID_SOURCE_ADDRESS = 20
    "AddressFiltered",            // OT_ERROR_ADDRESS_FILTERED = 21
    "DestinationAddressFiltered", // OT_ERROR_DESTINATION_ADDRESS_FILTERED = 22
    "NotFound",                   // OT_ERROR_NOT_FOUND = 23
    "Already",                    // OT_ERROR_ALREADY = 24
    "ReservedError25",            // otError 25 is reserved
    "Ipv6AddressCreationFailure", // OT_ERROR_IP6_ADDRESS_CREATION_FAILURE = 26
    "NotCapable",                 // OT_ERROR_NOT_CAPABLE = 27
    "ResponseTimeout",            // OT_ERROR_RESPONSE_TIMEOUT = 28
    "Duplicated",                 // OT_ERROR_DUPLICATED = 29
    "ReassemblyTimeout",          // OT_ERROR_REASSEMBLY_TIMEOUT = 30
    "NotTmf",                     // OT_ERROR_NOT_TMF = 31
    "NonLowpanDataFrame",         // OT_ERROR_NOT_LOWPAN_DATA_FRAME = 32
    "ReservedError33",            // otError 33 is reserved
    "LinkMarginLow",              // OT_ERROR_LINK_MARGIN_LOW = 34
    "InvalidCommand",             // OT_ERROR_INVALID_COMMAND = 35
    "Pending",                    // OT_ERROR_PENDING = 36
};

const char *otThreadErrorToString(otError aError)
{
    const char *retval;

    if (aError < OT_ARRAY_LENGTH(sThreadErrorStrings))
    {
        retval = sThreadErrorStrings[aError];
    }
    else
    {
        retval = "UnknownErrorType";
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
}
#endif
