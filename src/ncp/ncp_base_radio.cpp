/*
 *    Copyright (c) 2016-2017, The OpenThread Authors.
 *    All rights reserved.
 *
 *    Redistribution and use in source and binary forms, with or without
 *    modification, are permitted provided that the following conditions are met:
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *    3. Neither the name of the copyright holder nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 *    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 *    ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 *    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
 *    DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 *    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 *    ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 *    SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file
 *   This file implements raw link required Spinel interface to the OpenThread stack.
 */

#include <openthread/config.h>

#include "ncp_base.hpp"

#include <stdlib.h>

#include <openthread/diag.h>
#include <openthread/icmp6.h>

#include <openthread/ncp.h>
#include <openthread/openthread.h>

#include <openthread/platform/misc.h>
#include <openthread/platform/radio.h>

#include "openthread-instance.h"
#include "common/code_utils.hpp"
#include "common/debug.hpp"
#include "net/ip6.hpp"

#if OPENTHREAD_ENABLE_RAW_LINK_API
namespace ot {
namespace Ncp {

// ----------------------------------------------------------------------------
// MARK: Raw Link-Layer Datapath Glue
// ----------------------------------------------------------------------------

void NcpBase::LinkRawReceiveDone(otInstance *, otRadioFrame *aFrame, otError aError)
{
    sNcpInstance->LinkRawReceiveDone(aFrame, aError);
}

void NcpBase::LinkRawReceiveDone(otRadioFrame *aFrame, otError aError)
{
    uint16_t flags = 0;
    uint8_t header = SPINEL_HEADER_FLAG | SPINEL_HEADER_IID_0;

    SuccessOrExit(OutboundFrameBegin(header));

    if (aFrame->mDidTX)
    {
        flags |= SPINEL_MD_FLAG_TX;
    }

    // Append frame header and frame length
    SuccessOrExit(
        OutboundFrameFeedPacked(
            SPINEL_DATATYPE_COMMAND_PROP_S SPINEL_DATATYPE_UINT16_S,
            header,
            SPINEL_CMD_PROP_VALUE_IS,
            SPINEL_PROP_STREAM_RAW,
            (aError == OT_ERROR_NONE) ? aFrame->mLength : 0
        ));

    if (aError == OT_ERROR_NONE)
    {
        // Append the frame contents
        SuccessOrExit(OutboundFrameFeedData(aFrame->mPsdu, aFrame->mLength));
    }

    // Append metadata (rssi, etc)
    SuccessOrExit(
        OutboundFrameFeedPacked(
            SPINEL_DATATYPE_INT8_S
            SPINEL_DATATYPE_INT8_S
            SPINEL_DATATYPE_UINT16_S
            SPINEL_DATATYPE_STRUCT_S( // PHY-data
                SPINEL_DATATYPE_UINT8_S // 802.15.4 channel
                SPINEL_DATATYPE_UINT8_S // 802.15.4 LQI
                SPINEL_DATATYPE_UINT32_S // The timestamp milliseconds
                SPINEL_DATATYPE_UINT16_S // The timestamp microseconds
            )
            SPINEL_DATATYPE_STRUCT_S( // Vendor-data
                SPINEL_DATATYPE_UINT_PACKED_S
            ),
            aFrame->mPower,    // TX Power
            -128,              // Noise Floor (Currently unused)
            flags,             // Flags
            aFrame->mChannel,  // Receive channel
            aFrame->mLqi,      // Link quality indicator
            aFrame->mMsec,     // The timestamp milliseconds
            aFrame->mUsec,     // The timestamp microseconds, offset to mMsec
            aError             // Receive error
        ));

    SuccessOrExit(OutboundFrameSend());

exit:
    return;
}

void NcpBase::LinkRawTransmitDone(otInstance *, otRadioFrame *aFrame,
                                  otRadioFrame *aAckFrame, otError aError)
{
    sNcpInstance->LinkRawTransmitDone(aFrame, aAckFrame, aError);
}

void NcpBase::LinkRawTransmitDone(otRadioFrame *aFrame, otRadioFrame *aAckFrame, otError aError)
{
    if (mCurTransmitTID)
    {
        uint8_t header = SPINEL_HEADER_FLAG | SPINEL_HEADER_IID_0 | mCurTransmitTID;
        bool framePending = (aAckFrame != NULL && static_cast<Mac::Frame *>(aAckFrame)->GetFramePending());

        // Clear cached transmit TID
        mCurTransmitTID = 0;

        SuccessOrExit(OutboundFrameBegin(header));

        SuccessOrExit(OutboundFrameFeedPacked(
            SPINEL_DATATYPE_COMMAND_PROP_S SPINEL_DATATYPE_UINT_PACKED_S SPINEL_DATATYPE_BOOL_S,
            header,
            SPINEL_CMD_PROP_VALUE_IS,
            SPINEL_PROP_LAST_STATUS,
            ThreadErrorToSpinelStatus(aError),
            framePending
        ));

        if (aAckFrame && aError == OT_ERROR_NONE)
        {
            SuccessOrExit(OutboundFrameFeedPacked(SPINEL_DATATYPE_UINT16_S, aAckFrame->mLength));
            SuccessOrExit(OutboundFrameFeedData(aAckFrame->mPsdu, aAckFrame->mLength));
            SuccessOrExit(OutboundFrameFeedPacked(
                SPINEL_DATATYPE_INT8_S
                SPINEL_DATATYPE_INT8_S
                SPINEL_DATATYPE_UINT16_S
                SPINEL_DATATYPE_STRUCT_S(   // PHY-data
                    SPINEL_DATATYPE_UINT8_S // 802.15.4 channel
                    SPINEL_DATATYPE_UINT8_S // 802.15.4 LQI
                ),
                aAckFrame->mPower,          // RSSI
                -128,                       // Noise Floor (Currently unused)
                0,                          // Flags
                aAckFrame->mChannel,        // Receive channel
                aAckFrame->mLqi,            // Link quality indicator
                aFrame->mMsec,              // The timestamp milliseconds
                aFrame->mUsec               // The timestamp microseconds, offset to mMsec
            ));
        }

        SuccessOrExit(OutboundFrameSend());
    }

exit:
    OT_UNUSED_VARIABLE(aFrame);
    return;
}

void NcpBase::LinkRawEnergyScanDone(otInstance *, int8_t aEnergyScanMaxRssi)
{
    sNcpInstance->LinkRawEnergyScanDone(aEnergyScanMaxRssi);
}

void NcpBase::LinkRawEnergyScanDone(int8_t aEnergyScanMaxRssi)
{
    SendPropertyUpdate(
        SPINEL_HEADER_FLAG | SPINEL_HEADER_IID_0,
        SPINEL_CMD_PROP_VALUE_IS,
        SPINEL_PROP_MAC_ENERGY_SCAN_RESULT,
        SPINEL_DATATYPE_UINT8_S
        SPINEL_DATATYPE_INT8_S,
        mCurScanChannel,
        aEnergyScanMaxRssi
    );

    // Clear current scan channel
    mCurScanChannel = kInvalidScanChannel;

    // Make sure we are back listening on the original receive channel,
    // since the energy scan could have been on a different channel.
    otLinkRawReceive(mInstance, mCurReceiveChannel, &NcpBase::LinkRawReceiveDone);

    // We are finished with the scan, so send out
    // a property update indicating such.
    SendPropertyUpdate(
        SPINEL_HEADER_FLAG | SPINEL_HEADER_IID_0,
        SPINEL_CMD_PROP_VALUE_IS,
        SPINEL_PROP_MAC_SCAN_STATE,
        SPINEL_DATATYPE_UINT8_S,
        SPINEL_SCAN_STATE_IDLE
    );
}


otError NcpBase::SetPropertyHandler_MAC_SRC_MATCH_ENABLED(uint8_t aHeader, spinel_prop_key_t aKey,
                                                          const uint8_t *aValuePtr, uint16_t aValueLen)
{
    bool enabled;
    spinel_ssize_t parsedLength;
    otError error = OT_ERROR_NONE;

    parsedLength = spinel_datatype_unpack(
                       aValuePtr,
                       aValueLen,
                       SPINEL_DATATYPE_BOOL_S,
                       &enabled
                   );

    VerifyOrExit(parsedLength > 0, error = OT_ERROR_PARSE);

    error = otLinkRawSrcMatchEnable(mInstance, enabled);

exit:
    return SendSetPropertyResponse(aHeader, aKey, error);
}

otError NcpBase::SetPropertyHandler_MAC_SRC_MATCH_SHORT_ADDRESSES(uint8_t aHeader, spinel_prop_key_t aKey,
                                                                  const uint8_t *aValuePtr, uint16_t aValueLen)
{
    otError error = OT_ERROR_NONE;
    spinel_status_t spinelError = SPINEL_STATUS_OK;
    const uint8_t *data = aValuePtr;
    uint16_t dataLen = aValueLen;

    // Clear the list first
    error = otLinkRawSrcMatchClearShortEntries(mInstance);

    VerifyOrExit(error == OT_ERROR_NONE, spinelError = ThreadErrorToSpinelStatus(error));

    // Loop through the addresses and add them
    while (dataLen >= sizeof(uint16_t))
    {
        spinel_ssize_t parsedLength;
        uint16_t short_address;

        parsedLength = spinel_datatype_unpack(
                           data,
                           dataLen,
                           SPINEL_DATATYPE_UINT16_S,
                           &short_address
                       );

        VerifyOrExit(parsedLength > 0, spinelError = SPINEL_STATUS_PARSE_ERROR);

        data += parsedLength;
        dataLen -= (uint16_t)parsedLength;

        error = otLinkRawSrcMatchAddShortEntry(mInstance, short_address);

        VerifyOrExit(error == OT_ERROR_NONE, spinelError = ThreadErrorToSpinelStatus(error));
    }

    error =
        SendPropertyUpdate(
            aHeader,
            SPINEL_CMD_PROP_VALUE_IS,
            aKey,
            aValuePtr,
            aValueLen
        );

exit:

    if (spinelError != SPINEL_STATUS_OK)
    {
        error = SendLastStatus(aHeader, spinelError);
    }

    return error;
}

otError NcpBase::SetPropertyHandler_MAC_SRC_MATCH_EXTENDED_ADDRESSES(uint8_t aHeader, spinel_prop_key_t aKey,
                                                                     const uint8_t *aValuePtr, uint16_t aValueLen)
{
    otError error = OT_ERROR_NONE;
    spinel_status_t spinelError = SPINEL_STATUS_OK;
    const uint8_t *data = aValuePtr;
    uint16_t dataLen = aValueLen;

    // Clear the list first
    error = otLinkRawSrcMatchClearExtEntries(mInstance);

    VerifyOrExit(error == OT_ERROR_NONE, spinelError = ThreadErrorToSpinelStatus(error));

    // Loop through the addresses and add them
    while (dataLen >= sizeof(otExtAddress))
    {
        spinel_ssize_t parsedLength;
        otExtAddress *extAddress;

        parsedLength = spinel_datatype_unpack(
                           data,
                           dataLen,
                           SPINEL_DATATYPE_EUI64_S,
                           &extAddress
                       );

        VerifyOrExit(parsedLength > 0, spinelError = SPINEL_STATUS_PARSE_ERROR);

        data += parsedLength;
        dataLen -= (uint16_t)parsedLength;

        error = otLinkRawSrcMatchAddExtEntry(mInstance, extAddress);

        VerifyOrExit(error == OT_ERROR_NONE, spinelError = ThreadErrorToSpinelStatus(error));
    }

    error =
        SendPropertyUpdate(
            aHeader,
            SPINEL_CMD_PROP_VALUE_IS,
            aKey,
            aValuePtr,
            aValueLen
        );

exit:

    if (spinelError != SPINEL_STATUS_OK)
    {
        error = SendLastStatus(aHeader, spinelError);
    }

    return error;
}

otError NcpBase::RemovePropertyHandler_MAC_SRC_MATCH_SHORT_ADDRESSES(uint8_t aHeader, spinel_prop_key_t aKey,
                                                                     const uint8_t *aValuePtr, uint16_t aValueLen)
{
    spinel_ssize_t parsedLength;
    otError error = OT_ERROR_NONE;
    spinel_status_t spinelError = SPINEL_STATUS_OK;
    uint16_t shortAddress;

    parsedLength = spinel_datatype_unpack(
                       aValuePtr,
                       aValueLen,
                       SPINEL_DATATYPE_UINT16_S,
                       &shortAddress
                   );

    VerifyOrExit(parsedLength > 0, spinelError = SPINEL_STATUS_PARSE_ERROR);

    error = otLinkRawSrcMatchClearShortEntry(mInstance, shortAddress);
    VerifyOrExit(error == OT_ERROR_NONE, spinelError = ThreadErrorToSpinelStatus(error));

    error = SendPropertyUpdate(
                aHeader,
                SPINEL_CMD_PROP_VALUE_REMOVED,
                aKey,
                aValuePtr,
                aValueLen
            );

exit:

    if (spinelError != SPINEL_STATUS_OK)
    {
        error = SendLastStatus(aHeader, spinelError);
    }

    return error;
}

otError NcpBase::RemovePropertyHandler_MAC_SRC_MATCH_EXTENDED_ADDRESSES(uint8_t aHeader, spinel_prop_key_t aKey,
                                                                        const uint8_t *aValuePtr, uint16_t aValueLen)
{
    spinel_ssize_t parsedLength;
    otError error = OT_ERROR_NONE;
    spinel_status_t spinelError = SPINEL_STATUS_OK;
    otExtAddress *extAddress;

    parsedLength = spinel_datatype_unpack(
                       aValuePtr,
                       aValueLen,
                       SPINEL_DATATYPE_EUI64_S,
                       &extAddress
                   );

    VerifyOrExit(parsedLength > 0, spinelError = SPINEL_STATUS_PARSE_ERROR);

    error = otLinkRawSrcMatchClearExtEntry(mInstance, extAddress);
    VerifyOrExit(error == OT_ERROR_NONE, spinelError = ThreadErrorToSpinelStatus(error));

    error = SendPropertyUpdate(
                aHeader,
                SPINEL_CMD_PROP_VALUE_REMOVED,
                aKey,
                aValuePtr,
                aValueLen
            );

exit:

    if (spinelError != SPINEL_STATUS_OK)
    {
        error = SendLastStatus(aHeader, spinelError);
    }

    return error;
}

otError NcpBase::InsertPropertyHandler_MAC_SRC_MATCH_SHORT_ADDRESSES(uint8_t aHeader, spinel_prop_key_t aKey,
                                                                     const uint8_t *aValuePtr, uint16_t aValueLen)
{
    spinel_ssize_t parsedLength;
    otError error = OT_ERROR_NONE;
    spinel_status_t spinelError = SPINEL_STATUS_OK;
    uint16_t short_address;

    parsedLength = spinel_datatype_unpack(
                       aValuePtr,
                       aValueLen,
                       SPINEL_DATATYPE_UINT16_S,
                       &short_address
                   );

    VerifyOrExit(parsedLength > 0, spinelError = SPINEL_STATUS_PARSE_ERROR);

    error = otLinkRawSrcMatchAddShortEntry(mInstance, short_address);

    VerifyOrExit(error == OT_ERROR_NONE, spinelError = ThreadErrorToSpinelStatus(error));

    error = SendPropertyUpdate(
                    aHeader,
                    SPINEL_CMD_PROP_VALUE_INSERTED,
                    aKey,
                    aValuePtr,
                    aValueLen
                );

exit:

    if (spinelError != SPINEL_STATUS_OK)
    {
        error = SendLastStatus(aHeader, spinelError);
    }

    return error;
}

otError NcpBase::InsertPropertyHandler_MAC_SRC_MATCH_EXTENDED_ADDRESSES(uint8_t aHeader, spinel_prop_key_t aKey,
                                                                        const uint8_t *aValuePtr, uint16_t aValueLen)
{
    spinel_ssize_t parsedLength;
    otError error = OT_ERROR_NONE;
    spinel_status_t spinelError = SPINEL_STATUS_OK;
    otExtAddress *extAddress = NULL;

    parsedLength = spinel_datatype_unpack(
                       aValuePtr,
                       aValueLen,
                       SPINEL_DATATYPE_EUI64_S,
                       &extAddress
                   );

    VerifyOrExit(parsedLength > 0, spinelError = SPINEL_STATUS_PARSE_ERROR);

    error = otLinkRawSrcMatchAddExtEntry(mInstance, extAddress);

    VerifyOrExit(error == OT_ERROR_NONE, spinelError = ThreadErrorToSpinelStatus(error));

    error = SendPropertyUpdate(
                    aHeader,
                    SPINEL_CMD_PROP_VALUE_INSERTED,
                    aKey,
                    aValuePtr,
                    aValueLen
                );

exit:

    if (spinelError != SPINEL_STATUS_OK)
    {
        error = SendLastStatus(aHeader, spinelError);
    }

    return error;
}

otError NcpBase::SetPropertyHandler_PHY_ENABLED(uint8_t aHeader, spinel_prop_key_t aKey, const uint8_t *aValuePtr,
                                                uint16_t aValueLen)
{
    bool value = false;
    spinel_ssize_t parsedLength;
    otError error = OT_ERROR_NONE;

    parsedLength = spinel_datatype_unpack(
                       aValuePtr,
                       aValueLen,
                       SPINEL_DATATYPE_BOOL_S,
                       &value
                   );

    VerifyOrExit(parsedLength > 0, error = OT_ERROR_PARSE);

    if (value == false)
    {
        // If we have raw stream enabled stop receiving
        if (mIsRawStreamEnabled)
        {
            otLinkRawSleep(mInstance);
        }

        error = otLinkRawSetEnable(mInstance, false);
    }
    else
    {
        error = otLinkRawSetEnable(mInstance, true);

        // If we have raw stream enabled already, start receiving
        if (error == OT_ERROR_NONE && mIsRawStreamEnabled)
        {
            error = otLinkRawReceive(mInstance, mCurReceiveChannel, &NcpBase::LinkRawReceiveDone);
        }
    }

exit:
    return SendSetPropertyResponse(aHeader, aKey, error);
}

otError NcpBase::SetPropertyHandler_MAC_15_4_SADDR(uint8_t aHeader, spinel_prop_key_t aKey, const uint8_t *aValuePtr,
                                                   uint16_t aValueLen)
{
    uint16_t shortAddress;
    spinel_ssize_t parsedLength;
    otError error = OT_ERROR_NONE;

    parsedLength = spinel_datatype_unpack(
                       aValuePtr,
                       aValueLen,
                       SPINEL_DATATYPE_UINT16_S,
                       &shortAddress
                   );

    VerifyOrExit(parsedLength > 0, error = OT_ERROR_PARSE);

    error = otLinkRawSetShortAddress(mInstance, shortAddress);

exit:
    return SendSetPropertyResponse(aHeader, aKey, error);
}

otError NcpBase::SetPropertyHandler_STREAM_RAW(uint8_t aHeader, spinel_prop_key_t aKey, const uint8_t *aValuePtr,
                                               uint16_t aValueLen)
{
    uint8_t *frame_buffer = NULL;
    otRadioFrame *frame;
    unsigned int frameLen = 0;
    spinel_ssize_t parsedLength;
    otError error = OT_ERROR_NONE;

    VerifyOrExit(otLinkRawIsEnabled(mInstance), error = OT_ERROR_INVALID_STATE);

    frame = otLinkRawGetTransmitBuffer(mInstance);

    parsedLength = spinel_datatype_unpack(
                       aValuePtr,
                       aValueLen,
                       SPINEL_DATATYPE_DATA_WLEN_S
                       SPINEL_DATATYPE_UINT8_S
                       SPINEL_DATATYPE_INT8_S,
                       &frame_buffer,
                       &frameLen,
                       &frame->mChannel,
                       &frame->mPower
                   );

    VerifyOrExit(parsedLength > 0 && frameLen <= OT_RADIO_FRAME_MAX_SIZE, error = OT_ERROR_PARSE);

    // Cache the transaction ID for async response
    mCurTransmitTID = SPINEL_HEADER_GET_TID(aHeader);

    // Update frame buffer and length
    frame->mLength = static_cast<uint8_t>(frameLen);
    memcpy(frame->mPsdu, frame_buffer, frame->mLength);

    // TODO: This should be later added in the STREAM_RAW argument to allow user to directly specify it.
    frame->mMaxTxAttempts = OPENTHREAD_CONFIG_MAX_TX_ATTEMPTS_DIRECT;

    // Pass frame to the radio layer. Note, this fails if we
    // haven't enabled raw stream or are already transmitting.
    error = otLinkRawTransmit(mInstance, frame, &NcpBase::LinkRawTransmitDone);

exit:

    if (error == OT_ERROR_NONE)
    {
        // Don't do anything here yet. We will complete the transaction when we get a transmit done callback
    }
    else
    {
        error = SendLastStatus(aHeader, ThreadErrorToSpinelStatus(error));
    }

    OT_UNUSED_VARIABLE(aKey);

    return error;
}

}  // namespace Ncp
}  // namespace ot

#endif // OPENTHREAD_ENABLE_RAW_LINK_API
