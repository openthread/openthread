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

    if (aFrame->mDidTX)
    {
        flags |= SPINEL_MD_FLAG_TX;
    }

    // Append frame header and frame length
    SuccessOrExit(mEncoder.BeginFrame(header, SPINEL_CMD_PROP_VALUE_IS, SPINEL_PROP_STREAM_RAW));
    SuccessOrExit(mEncoder.WriteUint16((aError == OT_ERROR_NONE) ? aFrame->mLength : 0));

    if (aError == OT_ERROR_NONE)
    {
        // Append the frame contents
        SuccessOrExit(mEncoder.WriteData(aFrame->mPsdu, aFrame->mLength));
    }

    // Append metadata (rssi, etc)
    SuccessOrExit(mEncoder.WriteInt8(aFrame->mPower));      // TX Power
    SuccessOrExit(mEncoder.WriteInt8(-128));                // Noise Floor (Currently unused)
    SuccessOrExit(mEncoder.WriteUint16(flags));             // Flags

    SuccessOrExit(mEncoder.OpenStruct());                   // PHY-data
    SuccessOrExit(mEncoder.WriteUint8(aFrame->mChannel));   // 802.15.4 channel (Receive channel)
    SuccessOrExit(mEncoder.WriteUint8(aFrame->mLqi));       // 802.15.4 LQI
    SuccessOrExit(mEncoder.WriteUint32(aFrame->mMsec));     // The timestamp milliseconds
    SuccessOrExit(mEncoder.WriteUint16(aFrame->mUsec));     // The timestamp microseconds, offset to mMsec
    SuccessOrExit(mEncoder.CloseStruct());

    SuccessOrExit(mEncoder.OpenStruct());                   // Vendor-data
    SuccessOrExit(mEncoder.WriteUintPacked(aError));        // Receive error
    SuccessOrExit(mEncoder.CloseStruct());

    SuccessOrExit(mEncoder.EndFrame());

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

        SuccessOrExit(mEncoder.BeginFrame(header, SPINEL_CMD_PROP_VALUE_IS, SPINEL_PROP_LAST_STATUS));
        SuccessOrExit(mEncoder.WriteUintPacked(ThreadErrorToSpinelStatus(aError)));
        SuccessOrExit(mEncoder.WriteBool(framePending));

        if (aAckFrame && aError == OT_ERROR_NONE)
        {
            SuccessOrExit(mEncoder.WriteUint16(aAckFrame->mLength));
            SuccessOrExit(mEncoder.WriteData(aAckFrame->mPsdu, aAckFrame->mLength));

            SuccessOrExit(mEncoder.WriteInt8(aAckFrame->mPower));    // RSSI
            SuccessOrExit(mEncoder.WriteInt8(-128));                 // Noise Floor (Currently unused)
            SuccessOrExit(mEncoder.WriteUint16(0));                  // Flags

            SuccessOrExit(mEncoder.OpenStruct());                    // PHY-data

            SuccessOrExit(mEncoder.WriteUint8(aAckFrame->mChannel)); // Receive channel
            SuccessOrExit(mEncoder.WriteUint8(aAckFrame->mLqi));     // Link Quality Indicator

            SuccessOrExit(mEncoder.CloseStruct());
        }

        SuccessOrExit(mEncoder.EndFrame());
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
    int8_t scanChannel = mCurScanChannel;

    // Clear current scan channel
    mCurScanChannel = kInvalidScanChannel;

    // Make sure we are back listening on the original receive channel,
    // since the energy scan could have been on a different channel.
    otLinkRawReceive(mInstance, mCurReceiveChannel, &NcpBase::LinkRawReceiveDone);

    SuccessOrExit(
        mEncoder.BeginFrame(
            SPINEL_HEADER_FLAG | SPINEL_HEADER_IID_0,
            SPINEL_CMD_PROP_VALUE_IS,
            SPINEL_PROP_MAC_ENERGY_SCAN_RESULT
        ));

    SuccessOrExit(mEncoder.WriteUint8(static_cast<uint8_t>(scanChannel)));
    SuccessOrExit(mEncoder.WriteInt8(aEnergyScanMaxRssi));
    SuccessOrExit(mEncoder.EndFrame());

    // We are finished with the scan, so send out
    // a property update indicating such.
    SuccessOrExit(
        mEncoder.BeginFrame(
            SPINEL_HEADER_FLAG | SPINEL_HEADER_IID_0,
            SPINEL_CMD_PROP_VALUE_IS,
            SPINEL_PROP_MAC_SCAN_STATE
        ));

    SuccessOrExit(mEncoder.WriteUint8(SPINEL_SCAN_STATE_IDLE));
    SuccessOrExit(mEncoder.EndFrame());

exit:
    return;
}

otError NcpBase::SetPropertyHandler_MAC_SRC_MATCH_ENABLED(void)
{
    bool enabled;
    otError error = OT_ERROR_NONE;

    SuccessOrExit(error = mDecoder.ReadBool(enabled));

    error = otLinkRawSrcMatchEnable(mInstance, enabled);

exit:
    return error;
}

otError NcpBase::SetPropertyHandler_MAC_SRC_MATCH_SHORT_ADDRESSES(void)
{
    otError error = OT_ERROR_NONE;

    // Clear the list first
    SuccessOrExit(error = otLinkRawSrcMatchClearShortEntries(mInstance));

    // Loop through the addresses and add them
    while (mDecoder.GetRemainingLengthInStruct() >= sizeof(uint16_t))
    {
        uint16_t shortAddress;

        SuccessOrExit(error = mDecoder.ReadUint16(shortAddress));

        SuccessOrExit(error = otLinkRawSrcMatchAddShortEntry(mInstance, shortAddress));
    }

exit:
    return error;
}

otError NcpBase::SetPropertyHandler_MAC_SRC_MATCH_EXTENDED_ADDRESSES(void)
{
    otError error = OT_ERROR_NONE;

    // Clear the list first
    SuccessOrExit(error = otLinkRawSrcMatchClearExtEntries(mInstance));

    // Loop through the addresses and add them
    while (mDecoder.GetRemainingLengthInStruct() >= sizeof(otExtAddress))
    {
        const otExtAddress *extAddress;

        SuccessOrExit(error = mDecoder.ReadEui64(extAddress));

        SuccessOrExit(error = otLinkRawSrcMatchAddExtEntry(mInstance, extAddress));
    }

exit:
    return error;
}

otError NcpBase::RemovePropertyHandler_MAC_SRC_MATCH_SHORT_ADDRESSES(void)
{
    otError error = OT_ERROR_NONE;
    uint16_t shortAddress;

    SuccessOrExit(error = mDecoder.ReadUint16(shortAddress));

    error = otLinkRawSrcMatchClearShortEntry(mInstance, shortAddress);

exit:
    return error;
}

otError NcpBase::RemovePropertyHandler_MAC_SRC_MATCH_EXTENDED_ADDRESSES(void)
{
    otError error = OT_ERROR_NONE;
    const otExtAddress *extAddress;

    SuccessOrExit(error = mDecoder.ReadEui64(extAddress));;

    error = otLinkRawSrcMatchClearExtEntry(mInstance, extAddress);

exit:
    return error;
}

otError NcpBase::InsertPropertyHandler_MAC_SRC_MATCH_SHORT_ADDRESSES(void)
{
    otError error = OT_ERROR_NONE;
    uint16_t shortAddress;

    SuccessOrExit(error = mDecoder.ReadUint16(shortAddress));

    error = otLinkRawSrcMatchAddShortEntry(mInstance, shortAddress);

exit:
    return error;
}

otError NcpBase::InsertPropertyHandler_MAC_SRC_MATCH_EXTENDED_ADDRESSES(void)
{
    otError error = OT_ERROR_NONE;
    const otExtAddress *extAddress = NULL;

    SuccessOrExit(error = mDecoder.ReadEui64(extAddress));

    error = otLinkRawSrcMatchAddExtEntry(mInstance, extAddress);

exit:
    return error;
}

otError NcpBase::SetPropertyHandler_PHY_ENABLED(void)
{
    bool value = false;
    otError error = OT_ERROR_NONE;

    SuccessOrExit(error = mDecoder.ReadBool(value));

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
    return error;
}

otError NcpBase::SetPropertyHandler_MAC_15_4_SADDR(void)
{
    uint16_t shortAddress;
    otError error = OT_ERROR_NONE;

    SuccessOrExit(error = mDecoder.ReadUint16(shortAddress));

    error = otLinkRawSetShortAddress(mInstance, shortAddress);

exit:
    return error;
}

otError NcpBase::SetPropertyHandler_STREAM_RAW(uint8_t aHeader)
{
    const uint8_t *frameBuffer = NULL;
    otRadioFrame *frame;
    uint16_t frameLen = 0;
    otError error = OT_ERROR_NONE;

    VerifyOrExit(otLinkRawIsEnabled(mInstance), error = OT_ERROR_INVALID_STATE);

    frame = otLinkRawGetTransmitBuffer(mInstance);

    SuccessOrExit(error = mDecoder.ReadDataWithLen(frameBuffer, frameLen));
    SuccessOrExit(error = mDecoder.ReadUint8(frame->mChannel));
    SuccessOrExit(error = mDecoder.ReadInt8(frame->mPower));

    VerifyOrExit(frameLen <= OT_RADIO_FRAME_MAX_SIZE, error = OT_ERROR_PARSE);

    // Cache the transaction ID for async response
    mCurTransmitTID = SPINEL_HEADER_GET_TID(aHeader);

    // Update frame buffer and length
    frame->mLength = static_cast<uint8_t>(frameLen);
    memcpy(frame->mPsdu, frameBuffer, frame->mLength);

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

    return error;
}

}  // namespace Ncp
}  // namespace ot

#endif // OPENTHREAD_ENABLE_RAW_LINK_API
