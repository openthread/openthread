/*
 *  Copyright (c) 2024, The OpenThread Authors.
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

#include "logger.hpp"

#include "openthread-spinel-config.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include <openthread/error.h>
#include <openthread/logging.h>
#include <openthread/platform/radio.h>

#include "lib/spinel/spinel.h"
#include "lib/utils/math.hpp"
#include "lib/utils/utils.hpp"

namespace ot {
namespace Spinel {

using Lib::Utils::Min;
using Lib::Utils::ToUlong;

Logger::Logger(const char *aModuleName)
    : mModuleName(aModuleName)
{
}

void Logger::LogIfFail(const char *aText, otError aError)
{
    OT_UNUSED_VARIABLE(aText);

    if (aError != OT_ERROR_NONE && aError != OT_ERROR_NO_ACK)
    {
        LogWarn("%s: %s", aText, otThreadErrorToString(aError));
    }
}

void Logger::LogCrit(const char *aFormat, ...)
{
    va_list args;

    va_start(args, aFormat);
    otLogPlatArgs(OT_LOG_LEVEL_CRIT, mModuleName, aFormat, args);
    va_end(args);
}

void Logger::LogWarn(const char *aFormat, ...)
{
    va_list args;

    va_start(args, aFormat);
    otLogPlatArgs(OT_LOG_LEVEL_WARN, mModuleName, aFormat, args);
    va_end(args);
}

void Logger::LogNote(const char *aFormat, ...)
{
    va_list args;

    va_start(args, aFormat);
    otLogPlatArgs(OT_LOG_LEVEL_NOTE, mModuleName, aFormat, args);
    va_end(args);
}

void Logger::LogInfo(const char *aFormat, ...)
{
    va_list args;

    va_start(args, aFormat);
    otLogPlatArgs(OT_LOG_LEVEL_INFO, mModuleName, aFormat, args);
    va_end(args);
}

void Logger::LogDebg(const char *aFormat, ...)
{
    va_list args;

    va_start(args, aFormat);
    otLogPlatArgs(OT_LOG_LEVEL_DEBG, mModuleName, aFormat, args);
    va_end(args);
}

uint32_t Logger::Snprintf(char *aDest, uint32_t aSize, const char *aFormat, ...)
{
    int     len;
    va_list args;

    va_start(args, aFormat);
    len = vsnprintf(aDest, static_cast<size_t>(aSize), aFormat, args);
    va_end(args);

    return (len < 0) ? 0 : Min(static_cast<uint32_t>(len), aSize - 1);
}

void Logger::LogSpinelFrame(const uint8_t *aFrame, uint16_t aLength, bool aTx)
{
    otError           error                                   = OT_ERROR_NONE;
    char              buf[OPENTHREAD_LIB_SPINEL_LOG_MAX_SIZE] = {0};
    spinel_ssize_t    unpacked;
    uint8_t           header;
    uint32_t          cmd;
    spinel_prop_key_t key;
    uint8_t          *data;
    spinel_size_t     len;
    const char       *prefix = nullptr;
    char             *start  = buf;
    char             *end    = buf + sizeof(buf);

    EXPECT(otLoggingGetLevel() >= OT_LOG_LEVEL_DEBG, NO_ACTION);

    prefix   = aTx ? "Sent spinel frame" : "Received spinel frame";
    unpacked = spinel_datatype_unpack(aFrame, aLength, "CiiD", &header, &cmd, &key, &data, &len);
    EXPECT(unpacked > 0, error = OT_ERROR_PARSE);

    start += Snprintf(start, static_cast<uint32_t>(end - start), "%s, flg:0x%x, iid:%d, tid:%u, cmd:%s", prefix,
                      SPINEL_HEADER_GET_FLAG(header), SPINEL_HEADER_GET_IID(header), SPINEL_HEADER_GET_TID(header),
                      spinel_command_to_cstr(cmd));
    EXPECT(cmd != SPINEL_CMD_RESET, NO_ACTION);

    start += Snprintf(start, static_cast<uint32_t>(end - start), ", key:%s", spinel_prop_key_to_cstr(key));
    EXPECT(cmd != SPINEL_CMD_PROP_VALUE_GET, NO_ACTION);

    switch (key)
    {
    case SPINEL_PROP_LAST_STATUS:
    {
        spinel_status_t status;

        unpacked = spinel_datatype_unpack(data, len, SPINEL_DATATYPE_UINT_PACKED_S, &status);
        EXPECT(unpacked > 0, error = OT_ERROR_PARSE);
        start += Snprintf(start, static_cast<uint32_t>(end - start), ", status:%s", spinel_status_to_cstr(status));
    }
    break;

    case SPINEL_PROP_MAC_RAW_STREAM_ENABLED:
    case SPINEL_PROP_MAC_SRC_MATCH_ENABLED:
    case SPINEL_PROP_PHY_ENABLED:
    case SPINEL_PROP_RADIO_COEX_ENABLE:
    {
        bool enabled;

        unpacked = spinel_datatype_unpack(data, len, SPINEL_DATATYPE_BOOL_S, &enabled);
        EXPECT(unpacked > 0, error = OT_ERROR_PARSE);
        start += Snprintf(start, static_cast<uint32_t>(end - start), ", enabled:%u", enabled);
    }
    break;

    case SPINEL_PROP_PHY_CCA_THRESHOLD:
    case SPINEL_PROP_PHY_FEM_LNA_GAIN:
    case SPINEL_PROP_PHY_RX_SENSITIVITY:
    case SPINEL_PROP_PHY_RSSI:
    case SPINEL_PROP_PHY_TX_POWER:
    {
        const char *name = nullptr;
        int8_t      value;

        unpacked = spinel_datatype_unpack(data, len, SPINEL_DATATYPE_INT8_S, &value);
        EXPECT(unpacked > 0, error = OT_ERROR_PARSE);

        switch (key)
        {
        case SPINEL_PROP_PHY_TX_POWER:
            name = "power";
            break;
        case SPINEL_PROP_PHY_CCA_THRESHOLD:
            name = "threshold";
            break;
        case SPINEL_PROP_PHY_FEM_LNA_GAIN:
            name = "gain";
            break;
        case SPINEL_PROP_PHY_RX_SENSITIVITY:
            name = "sensitivity";
            break;
        case SPINEL_PROP_PHY_RSSI:
            name = "rssi";
            break;
        }

        start += Snprintf(start, static_cast<uint32_t>(end - start), ", %s:%d", name, value);
    }
    break;

    case SPINEL_PROP_MAC_PROMISCUOUS_MODE:
    case SPINEL_PROP_MAC_SCAN_STATE:
    case SPINEL_PROP_PHY_CHAN:
    case SPINEL_PROP_RCP_CSL_ACCURACY:
    case SPINEL_PROP_RCP_CSL_UNCERTAINTY:
    {
        const char *name = nullptr;
        uint8_t     value;

        unpacked = spinel_datatype_unpack(data, len, SPINEL_DATATYPE_UINT8_S, &value);
        EXPECT(unpacked > 0, error = OT_ERROR_PARSE);

        switch (key)
        {
        case SPINEL_PROP_MAC_SCAN_STATE:
            name = "state";
            break;
        case SPINEL_PROP_RCP_CSL_ACCURACY:
            name = "accuracy";
            break;
        case SPINEL_PROP_RCP_CSL_UNCERTAINTY:
            name = "uncertainty";
            break;
        case SPINEL_PROP_MAC_PROMISCUOUS_MODE:
            name = "mode";
            break;
        case SPINEL_PROP_PHY_CHAN:
            name = "channel";
            break;
        }

        start += Snprintf(start, static_cast<uint32_t>(end - start), ", %s:%u", name, value);
    }
    break;

    case SPINEL_PROP_MAC_15_4_PANID:
    case SPINEL_PROP_MAC_15_4_SADDR:
    case SPINEL_PROP_MAC_SCAN_PERIOD:
    case SPINEL_PROP_PHY_REGION_CODE:
    {
        const char *name = nullptr;
        uint16_t    value;

        unpacked = spinel_datatype_unpack(data, len, SPINEL_DATATYPE_UINT16_S, &value);
        EXPECT(unpacked > 0, error = OT_ERROR_PARSE);

        switch (key)
        {
        case SPINEL_PROP_MAC_SCAN_PERIOD:
            name = "period";
            break;
        case SPINEL_PROP_PHY_REGION_CODE:
            name = "region";
            break;
        case SPINEL_PROP_MAC_15_4_SADDR:
            name = "saddr";
            break;
        case SPINEL_PROP_MAC_SRC_MATCH_SHORT_ADDRESSES:
            name = "saddr";
            break;
        case SPINEL_PROP_MAC_15_4_PANID:
            name = "panid";
            break;
        }

        start += Snprintf(start, static_cast<uint32_t>(end - start), ", %s:0x%04x", name, value);
    }
    break;

    case SPINEL_PROP_MAC_SRC_MATCH_SHORT_ADDRESSES:
    {
        uint16_t saddr;

        start += Snprintf(start, static_cast<uint32_t>(end - start), ", saddr:");

        if (len < sizeof(saddr))
        {
            start += Snprintf(start, static_cast<uint32_t>(end - start), "none");
        }
        else
        {
            while (len >= sizeof(saddr))
            {
                unpacked = spinel_datatype_unpack(data, len, SPINEL_DATATYPE_UINT16_S, &saddr);
                EXPECT(unpacked > 0, error = OT_ERROR_PARSE);
                data += unpacked;
                len -= static_cast<spinel_size_t>(unpacked);
                start += Snprintf(start, static_cast<uint32_t>(end - start), "0x%04x ", saddr);
            }
        }
    }
    break;

    case SPINEL_PROP_RCP_MAC_FRAME_COUNTER:
    case SPINEL_PROP_RCP_TIMESTAMP:
    {
        const char *name;
        uint32_t    value;

        unpacked = spinel_datatype_unpack(data, len, SPINEL_DATATYPE_UINT32_S, &value);
        EXPECT(unpacked > 0, error = OT_ERROR_PARSE);

        name = (key == SPINEL_PROP_RCP_TIMESTAMP) ? "timestamp" : "counter";
        start += Snprintf(start, static_cast<uint32_t>(end - start), ", %s:%u", name, value);
    }
    break;

    case SPINEL_PROP_RADIO_CAPS:
    case SPINEL_PROP_RCP_API_VERSION:
    case SPINEL_PROP_RCP_MIN_HOST_API_VERSION:
    {
        const char  *name;
        unsigned int value;

        unpacked = spinel_datatype_unpack(data, len, SPINEL_DATATYPE_UINT_PACKED_S, &value);
        EXPECT(unpacked > 0, error = OT_ERROR_PARSE);

        switch (key)
        {
        case SPINEL_PROP_RADIO_CAPS:
            name = "caps";
            break;
        case SPINEL_PROP_RCP_API_VERSION:
            name = "version";
            break;
        case SPINEL_PROP_RCP_MIN_HOST_API_VERSION:
            name = "min-host-version";
            break;
        default:
            name = "";
            break;
        }

        start += Snprintf(start, static_cast<uint32_t>(end - start), ", %s:%u", name, value);
    }
    break;

    case SPINEL_PROP_RCP_LOG_CRASH_DUMP:
    {
        const char *name;
        name = "log-crash-dump";

        start += Snprintf(start, static_cast<uint32_t>(end - start), ", %s", name);
    }
    break;

    case SPINEL_PROP_MAC_ENERGY_SCAN_RESULT:
    case SPINEL_PROP_PHY_CHAN_MAX_POWER:
    {
        const char *name;
        uint8_t     channel;
        int8_t      value;

        unpacked = spinel_datatype_unpack(data, len, SPINEL_DATATYPE_UINT8_S SPINEL_DATATYPE_INT8_S, &channel, &value);
        EXPECT(unpacked > 0, error = OT_ERROR_PARSE);

        name = (key == SPINEL_PROP_MAC_ENERGY_SCAN_RESULT) ? "rssi" : "power";
        start += Snprintf(start, static_cast<uint32_t>(end - start), ", channel:%u, %s:%d", channel, name, value);
    }
    break;

    case SPINEL_PROP_CAPS:
    {
        unsigned int capability;

        start += Snprintf(start, static_cast<uint32_t>(end - start), ", caps:");

        while (len > 0)
        {
            unpacked = spinel_datatype_unpack(data, len, SPINEL_DATATYPE_UINT_PACKED_S, &capability);
            EXPECT(unpacked > 0, error = OT_ERROR_PARSE);
            data += unpacked;
            len -= static_cast<spinel_size_t>(unpacked);
            start += Snprintf(start, static_cast<uint32_t>(end - start), "%s ", spinel_capability_to_cstr(capability));
        }
    }
    break;

    case SPINEL_PROP_PROTOCOL_VERSION:
    {
        unsigned int major;
        unsigned int minor;

        unpacked = spinel_datatype_unpack(data, len, SPINEL_DATATYPE_UINT_PACKED_S SPINEL_DATATYPE_UINT_PACKED_S,
                                          &major, &minor);
        EXPECT(unpacked > 0, error = OT_ERROR_PARSE);
        start += Snprintf(start, static_cast<uint32_t>(end - start), ", major:%u, minor:%u", major, minor);
    }
    break;

    case SPINEL_PROP_PHY_CHAN_PREFERRED:
    case SPINEL_PROP_PHY_CHAN_SUPPORTED:
    {
        uint8_t        maskBuffer[kChannelMaskBufferSize];
        uint32_t       channelMask = 0;
        const uint8_t *maskData    = maskBuffer;
        spinel_size_t  maskLength  = sizeof(maskBuffer);

        unpacked = spinel_datatype_unpack_in_place(data, len, SPINEL_DATATYPE_DATA_S, maskBuffer, &maskLength);
        EXPECT(unpacked > 0, error = OT_ERROR_PARSE);

        while (maskLength > 0)
        {
            uint8_t channel;

            unpacked = spinel_datatype_unpack(maskData, maskLength, SPINEL_DATATYPE_UINT8_S, &channel);
            EXPECT(unpacked > 0, error = OT_ERROR_PARSE);
            EXPECT(channel < kChannelMaskBufferSize, error = OT_ERROR_PARSE);
            channelMask |= (1UL << channel);

            maskData += unpacked;
            maskLength -= static_cast<spinel_size_t>(unpacked);
        }

        start += Snprintf(start, static_cast<uint32_t>(end - start), ", channelMask:0x%08x", channelMask);
    }
    break;

    case SPINEL_PROP_NCP_VERSION:
    {
        const char *version;

        unpacked = spinel_datatype_unpack(data, len, SPINEL_DATATYPE_UTF8_S, &version);
        EXPECT(unpacked >= 0, error = OT_ERROR_PARSE);
        start += Snprintf(start, static_cast<uint32_t>(end - start), ", version:%s", version);
    }
    break;

    case SPINEL_PROP_STREAM_RAW:
    {
        otRadioFrame frame;

        if (cmd == SPINEL_CMD_PROP_VALUE_IS)
        {
            uint16_t     flags;
            int8_t       noiseFloor;
            unsigned int receiveError;

            unpacked = spinel_datatype_unpack(data, len,
                                              SPINEL_DATATYPE_DATA_WLEN_S                          // Frame
                                                  SPINEL_DATATYPE_INT8_S                           // RSSI
                                                      SPINEL_DATATYPE_INT8_S                       // Noise Floor
                                                          SPINEL_DATATYPE_UINT16_S                 // Flags
                                                              SPINEL_DATATYPE_STRUCT_S(            // PHY-data
                                                                  SPINEL_DATATYPE_UINT8_S          // 802.15.4 channel
                                                                      SPINEL_DATATYPE_UINT8_S      // 802.15.4 LQI
                                                                          SPINEL_DATATYPE_UINT64_S // Timestamp (us).
                                                                  ) SPINEL_DATATYPE_STRUCT_S(      // Vendor-data
                                                                  SPINEL_DATATYPE_UINT_PACKED_S    // Receive error
                                                                  ),
                                              &frame.mPsdu, &frame.mLength, &frame.mInfo.mRxInfo.mRssi, &noiseFloor,
                                              &flags, &frame.mChannel, &frame.mInfo.mRxInfo.mLqi,
                                              &frame.mInfo.mRxInfo.mTimestamp, &receiveError);
            EXPECT(unpacked > 0, error = OT_ERROR_PARSE);
            start += Snprintf(start, static_cast<uint32_t>(end - start), ", len:%u, rssi:%d ...", frame.mLength,
                              frame.mInfo.mRxInfo.mRssi);
            OT_UNUSED_VARIABLE(start); // Avoid static analysis error
            LogDebg("%s", buf);

            start = buf;
            start += Snprintf(start, static_cast<uint32_t>(end - start),
                              "... noise:%d, flags:0x%04x, channel:%u, lqi:%u, timestamp:%lu, rxerr:%u", noiseFloor,
                              flags, frame.mChannel, frame.mInfo.mRxInfo.mLqi,
                              static_cast<unsigned long>(frame.mInfo.mRxInfo.mTimestamp), receiveError);
        }
        else if (cmd == SPINEL_CMD_PROP_VALUE_SET)
        {
            bool csmaCaEnabled;
            bool isHeaderUpdated;
            bool isARetx;
            bool skipAes;

            unpacked = spinel_datatype_unpack(
                data, len,
                SPINEL_DATATYPE_DATA_WLEN_S                                   // Frame data
                    SPINEL_DATATYPE_UINT8_S                                   // Channel
                        SPINEL_DATATYPE_UINT8_S                               // MaxCsmaBackoffs
                            SPINEL_DATATYPE_UINT8_S                           // MaxFrameRetries
                                SPINEL_DATATYPE_BOOL_S                        // CsmaCaEnabled
                                    SPINEL_DATATYPE_BOOL_S                    // IsHeaderUpdated
                                        SPINEL_DATATYPE_BOOL_S                // IsARetx
                                            SPINEL_DATATYPE_BOOL_S            // SkipAes
                                                SPINEL_DATATYPE_UINT32_S      // TxDelay
                                                    SPINEL_DATATYPE_UINT32_S, // TxDelayBaseTime
                &frame.mPsdu, &frame.mLength, &frame.mChannel, &frame.mInfo.mTxInfo.mMaxCsmaBackoffs,
                &frame.mInfo.mTxInfo.mMaxFrameRetries, &csmaCaEnabled, &isHeaderUpdated, &isARetx, &skipAes,
                &frame.mInfo.mTxInfo.mTxDelay, &frame.mInfo.mTxInfo.mTxDelayBaseTime);

            EXPECT(unpacked > 0, error = OT_ERROR_PARSE);
            start += Snprintf(start, static_cast<uint32_t>(end - start),
                              ", len:%u, channel:%u, maxbackoffs:%u, maxretries:%u ...", frame.mLength, frame.mChannel,
                              frame.mInfo.mTxInfo.mMaxCsmaBackoffs, frame.mInfo.mTxInfo.mMaxFrameRetries);
            OT_UNUSED_VARIABLE(start); // Avoid static analysis error
            LogDebg("%s", buf);

            start = buf;
            start += Snprintf(start, static_cast<uint32_t>(end - start),
                              "... csmaCaEnabled:%u, isHeaderUpdated:%u, isARetx:%u, skipAes:%u"
                              ", txDelay:%u, txDelayBase:%u",
                              csmaCaEnabled, isHeaderUpdated, isARetx, skipAes, frame.mInfo.mTxInfo.mTxDelay,
                              frame.mInfo.mTxInfo.mTxDelayBaseTime);
        }
    }
    break;

    case SPINEL_PROP_STREAM_DEBUG:
    {
        char          debugString[OPENTHREAD_LIB_SPINEL_NCP_LOG_MAX_SIZE + 1];
        spinel_size_t stringLength = sizeof(debugString);

        unpacked = spinel_datatype_unpack_in_place(data, len, SPINEL_DATATYPE_DATA_S, debugString, &stringLength);
        assert(stringLength < sizeof(debugString));
        EXPECT(unpacked > 0, error = OT_ERROR_PARSE);
        debugString[stringLength] = '\0';
        start += Snprintf(start, static_cast<uint32_t>(end - start), ", debug:%s", debugString);
    }
    break;

    case SPINEL_PROP_STREAM_LOG:
    {
        const char *logString;
        uint8_t     logLevel;

        unpacked = spinel_datatype_unpack(data, len, SPINEL_DATATYPE_UTF8_S, &logString);
        EXPECT(unpacked >= 0, error = OT_ERROR_PARSE);
        data += unpacked;
        len -= static_cast<spinel_size_t>(unpacked);

        unpacked = spinel_datatype_unpack(data, len, SPINEL_DATATYPE_UINT8_S, &logLevel);
        EXPECT(unpacked > 0, error = OT_ERROR_PARSE);
        start += Snprintf(start, static_cast<uint32_t>(end - start), ", level:%u, log:%s", logLevel, logString);
    }
    break;

    case SPINEL_PROP_NEST_STREAM_MFG:
    {
        const char *output;
        size_t      outputLen;

        unpacked = spinel_datatype_unpack(data, len, SPINEL_DATATYPE_UTF8_S, &output, &outputLen);
        EXPECT(unpacked > 0, error = OT_ERROR_PARSE);
        start += Snprintf(start, static_cast<uint32_t>(end - start), ", diag:%s", output);
    }
    break;

    case SPINEL_PROP_RCP_MAC_KEY:
    {
        uint8_t      keyIdMode;
        uint8_t      keyId;
        otMacKey     prevKey;
        unsigned int prevKeyLen = sizeof(otMacKey);
        otMacKey     currKey;
        unsigned int currKeyLen = sizeof(otMacKey);
        otMacKey     nextKey;
        unsigned int nextKeyLen = sizeof(otMacKey);

        unpacked = spinel_datatype_unpack(data, len,
                                          SPINEL_DATATYPE_UINT8_S SPINEL_DATATYPE_UINT8_S SPINEL_DATATYPE_DATA_WLEN_S
                                              SPINEL_DATATYPE_DATA_WLEN_S SPINEL_DATATYPE_DATA_WLEN_S,
                                          &keyIdMode, &keyId, prevKey.m8, &prevKeyLen, currKey.m8, &currKeyLen,
                                          nextKey.m8, &nextKeyLen);
        EXPECT(unpacked > 0, error = OT_ERROR_PARSE);
        start += Snprintf(start, static_cast<uint32_t>(end - start),
                          ", keyIdMode:%u, keyId:%u, prevKey:***, currKey:***, nextKey:***", keyIdMode, keyId);
    }
    break;

    case SPINEL_PROP_HWADDR:
    case SPINEL_PROP_MAC_15_4_LADDR:
    {
        const char *name                    = nullptr;
        uint8_t     m8[OT_EXT_ADDRESS_SIZE] = {0};

        unpacked = spinel_datatype_unpack_in_place(data, len, SPINEL_DATATYPE_EUI64_S, &m8[0]);
        EXPECT(unpacked > 0, error = OT_ERROR_PARSE);

        name = (key == SPINEL_PROP_HWADDR) ? "eui64" : "laddr";
        start += Snprintf(start, static_cast<uint32_t>(end - start), ", %s:%02x%02x%02x%02x%02x%02x%02x%02x", name,
                          m8[0], m8[1], m8[2], m8[3], m8[4], m8[5], m8[6], m8[7]);
    }
    break;

    case SPINEL_PROP_MAC_SRC_MATCH_EXTENDED_ADDRESSES:
    {
        uint8_t m8[OT_EXT_ADDRESS_SIZE];

        start += Snprintf(start, static_cast<uint32_t>(end - start), ", extaddr:");

        if (len < sizeof(m8))
        {
            start += Snprintf(start, static_cast<uint32_t>(end - start), "none");
        }
        else
        {
            while (len >= sizeof(m8))
            {
                unpacked = spinel_datatype_unpack_in_place(data, len, SPINEL_DATATYPE_EUI64_S, m8);
                EXPECT(unpacked > 0, error = OT_ERROR_PARSE);
                data += unpacked;
                len -= static_cast<spinel_size_t>(unpacked);
                start += Snprintf(start, static_cast<uint32_t>(end - start), "%02x%02x%02x%02x%02x%02x%02x%02x ", m8[0],
                                  m8[1], m8[2], m8[3], m8[4], m8[5], m8[6], m8[7]);
            }
        }
    }
    break;

    case SPINEL_PROP_RADIO_COEX_METRICS:
    {
        otRadioCoexMetrics metrics;
        unpacked = spinel_datatype_unpack(
            data, len,
            SPINEL_DATATYPE_STRUCT_S(                                    // Tx Coex Metrics Structure
                SPINEL_DATATYPE_UINT32_S                                 // NumTxRequest
                    SPINEL_DATATYPE_UINT32_S                             // NumTxGrantImmediate
                        SPINEL_DATATYPE_UINT32_S                         // NumTxGrantWait
                            SPINEL_DATATYPE_UINT32_S                     // NumTxGrantWaitActivated
                                SPINEL_DATATYPE_UINT32_S                 // NumTxGrantWaitTimeout
                                    SPINEL_DATATYPE_UINT32_S             // NumTxGrantDeactivatedDuringRequest
                                        SPINEL_DATATYPE_UINT32_S         // NumTxDelayedGrant
                                            SPINEL_DATATYPE_UINT32_S     // AvgTxRequestToGrantTime
                ) SPINEL_DATATYPE_STRUCT_S(                              // Rx Coex Metrics Structure
                SPINEL_DATATYPE_UINT32_S                                 // NumRxRequest
                    SPINEL_DATATYPE_UINT32_S                             // NumRxGrantImmediate
                        SPINEL_DATATYPE_UINT32_S                         // NumRxGrantWait
                            SPINEL_DATATYPE_UINT32_S                     // NumRxGrantWaitActivated
                                SPINEL_DATATYPE_UINT32_S                 // NumRxGrantWaitTimeout
                                    SPINEL_DATATYPE_UINT32_S             // NumRxGrantDeactivatedDuringRequest
                                        SPINEL_DATATYPE_UINT32_S         // NumRxDelayedGrant
                                            SPINEL_DATATYPE_UINT32_S     // AvgRxRequestToGrantTime
                                                SPINEL_DATATYPE_UINT32_S // NumRxGrantNone
                ) SPINEL_DATATYPE_BOOL_S                                 // Stopped
                SPINEL_DATATYPE_UINT32_S,                                // NumGrantGlitch
            &metrics.mNumTxRequest, &metrics.mNumTxGrantImmediate, &metrics.mNumTxGrantWait,
            &metrics.mNumTxGrantWaitActivated, &metrics.mNumTxGrantWaitTimeout,
            &metrics.mNumTxGrantDeactivatedDuringRequest, &metrics.mNumTxDelayedGrant,
            &metrics.mAvgTxRequestToGrantTime, &metrics.mNumRxRequest, &metrics.mNumRxGrantImmediate,
            &metrics.mNumRxGrantWait, &metrics.mNumRxGrantWaitActivated, &metrics.mNumRxGrantWaitTimeout,
            &metrics.mNumRxGrantDeactivatedDuringRequest, &metrics.mNumRxDelayedGrant,
            &metrics.mAvgRxRequestToGrantTime, &metrics.mNumRxGrantNone, &metrics.mStopped, &metrics.mNumGrantGlitch);

        EXPECT(unpacked > 0, error = OT_ERROR_PARSE);

        LogDebg("%s ...", buf);
        LogDebg(" txRequest:%lu", ToUlong(metrics.mNumTxRequest));
        LogDebg(" txGrantImmediate:%lu", ToUlong(metrics.mNumTxGrantImmediate));
        LogDebg(" txGrantWait:%lu", ToUlong(metrics.mNumTxGrantWait));
        LogDebg(" txGrantWaitActivated:%lu", ToUlong(metrics.mNumTxGrantWaitActivated));
        LogDebg(" txGrantWaitTimeout:%lu", ToUlong(metrics.mNumTxGrantWaitTimeout));
        LogDebg(" txGrantDeactivatedDuringRequest:%lu", ToUlong(metrics.mNumTxGrantDeactivatedDuringRequest));
        LogDebg(" txDelayedGrant:%lu", ToUlong(metrics.mNumTxDelayedGrant));
        LogDebg(" avgTxRequestToGrantTime:%lu", ToUlong(metrics.mAvgTxRequestToGrantTime));
        LogDebg(" rxRequest:%lu", ToUlong(metrics.mNumRxRequest));
        LogDebg(" rxGrantImmediate:%lu", ToUlong(metrics.mNumRxGrantImmediate));
        LogDebg(" rxGrantWait:%lu", ToUlong(metrics.mNumRxGrantWait));
        LogDebg(" rxGrantWaitActivated:%lu", ToUlong(metrics.mNumRxGrantWaitActivated));
        LogDebg(" rxGrantWaitTimeout:%lu", ToUlong(metrics.mNumRxGrantWaitTimeout));
        LogDebg(" rxGrantDeactivatedDuringRequest:%lu", ToUlong(metrics.mNumRxGrantDeactivatedDuringRequest));
        LogDebg(" rxDelayedGrant:%lu", ToUlong(metrics.mNumRxDelayedGrant));
        LogDebg(" avgRxRequestToGrantTime:%lu", ToUlong(metrics.mAvgRxRequestToGrantTime));
        LogDebg(" rxGrantNone:%lu", ToUlong(metrics.mNumRxGrantNone));
        LogDebg(" stopped:%u", metrics.mStopped);

        start = buf;
        start += Snprintf(start, static_cast<uint32_t>(end - start), " grantGlitch:%u", metrics.mNumGrantGlitch);
    }
    break;

    case SPINEL_PROP_MAC_SCAN_MASK:
    {
        constexpr uint8_t kNumChannels = 16;
        uint8_t           channels[kNumChannels];
        spinel_size_t     size;

        unpacked = spinel_datatype_unpack(data, len, SPINEL_DATATYPE_DATA_S, channels, &size);
        EXPECT(unpacked > 0, error = OT_ERROR_PARSE);
        start += Snprintf(start, static_cast<uint32_t>(end - start), ", channels:");

        for (spinel_size_t i = 0; i < size; i++)
        {
            start += Snprintf(start, static_cast<uint32_t>(end - start), "%u ", channels[i]);
        }
    }
    break;

    case SPINEL_PROP_RCP_ENH_ACK_PROBING:
    {
        uint16_t saddr;
        uint8_t  m8[OT_EXT_ADDRESS_SIZE];
        uint8_t  flags;

        unpacked = spinel_datatype_unpack(
            data, len, SPINEL_DATATYPE_UINT16_S SPINEL_DATATYPE_EUI64_S SPINEL_DATATYPE_UINT8_S, &saddr, m8, &flags);

        EXPECT(unpacked > 0, error = OT_ERROR_PARSE);
        start += Snprintf(start, static_cast<uint32_t>(end - start),
                          ", saddr:%04x, extaddr:%02x%02x%02x%02x%02x%02x%02x%02x, flags:0x%02x", saddr, m8[0], m8[1],
                          m8[2], m8[3], m8[4], m8[5], m8[6], m8[7], flags);
    }
    break;

    case SPINEL_PROP_PHY_CALIBRATED_POWER:
    {
        if (cmd == SPINEL_CMD_PROP_VALUE_INSERT)
        {
            uint8_t      channel;
            int16_t      actualPower;
            uint8_t     *rawPowerSetting;
            unsigned int rawPowerSettingLength;

            unpacked = spinel_datatype_unpack(
                data, len, SPINEL_DATATYPE_UINT8_S SPINEL_DATATYPE_INT16_S SPINEL_DATATYPE_DATA_WLEN_S, &channel,
                &actualPower, &rawPowerSetting, &rawPowerSettingLength);
            EXPECT(unpacked > 0, error = OT_ERROR_PARSE);

            start += Snprintf(start, static_cast<uint32_t>(end - start),
                              ", ch:%u, actualPower:%d, rawPowerSetting:", channel, actualPower);
            for (unsigned int i = 0; i < rawPowerSettingLength; i++)
            {
                start += Snprintf(start, static_cast<uint32_t>(end - start), "%02x", rawPowerSetting[i]);
            }
        }
    }
    break;

    case SPINEL_PROP_PHY_CHAN_TARGET_POWER:
    {
        uint8_t channel;
        int16_t targetPower;

        unpacked =
            spinel_datatype_unpack(data, len, SPINEL_DATATYPE_UINT8_S SPINEL_DATATYPE_INT16_S, &channel, &targetPower);
        EXPECT(unpacked > 0, error = OT_ERROR_PARSE);
        start += Snprintf(start, static_cast<uint32_t>(end - start), ", ch:%u, targetPower:%d", channel, targetPower);
    }
    break;
    }

exit:
    OT_UNUSED_VARIABLE(start); // Avoid static analysis error
    if (error == OT_ERROR_NONE)
    {
        LogDebg("%s", buf);
    }
    else if (prefix != nullptr)
    {
        LogDebg("%s, failed to parse spinel frame !", prefix);
    }
}

} // namespace Spinel
} // namespace ot
