/*
 *  Copyright (c) 2019, The OpenThread Authors.
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

#include "mac_frame.h"

#include <assert.h>
#include "mac/mac_frame.hpp"

using namespace ot;

bool otMacFrameDoesAddrMatch(const otRadioFrame *aFrame,
                             otPanId             aPanId,
                             otShortAddress      aShortAddress,
                             const otExtAddress *aExtAddress)
{
    const Mac::Frame &frame = *static_cast<const Mac::Frame *>(aFrame);
    bool              rval  = true;
    Mac::Address      dst;
    Mac::PanId        panid;

    SuccessOrExit(frame.GetDstAddr(dst));

    switch (dst.GetType())
    {
    case Mac::Address::kTypeShort:
        VerifyOrExit(dst.GetShort() == Mac::kShortAddrBroadcast || dst.GetShort() == aShortAddress, rval = false);
        break;

    case Mac::Address::kTypeExtended:
        VerifyOrExit(dst.GetExtended() == *static_cast<const Mac::ExtAddress *>(aExtAddress), rval = false);
        break;

    case Mac::Address::kTypeNone:
        break;
    }

    SuccessOrExit(frame.GetDstPanId(panid));
    VerifyOrExit(panid == Mac::kPanIdBroadcast || panid == aPanId, rval = false);

exit:
    return rval;
}

bool otMacFrameIsAck(const otRadioFrame *aFrame)
{
    return static_cast<const Mac::Frame *>(aFrame)->GetType() == Mac::Frame::kFcfFrameAck;
}

bool otMacFrameIsData(const otRadioFrame *aFrame)
{
    return static_cast<const Mac::Frame *>(aFrame)->GetType() == Mac::Frame::kFcfFrameData;
}

bool otMacFrameIsDataRequest(const otRadioFrame *aFrame)
{
    return static_cast<const Mac::Frame *>(aFrame)->IsDataRequestCommand();
}

bool otMacFrameIsAckRequested(const otRadioFrame *aFrame)
{
    return static_cast<const Mac::Frame *>(aFrame)->GetAckRequest();
}

otError otMacFrameGetSrcAddr(const otRadioFrame *aFrame, otMacAddress *aMacAddress)
{
    otError      error;
    Mac::Address address;

    error = static_cast<const Mac::Frame *>(aFrame)->GetSrcAddr(address);
    SuccessOrExit(error);

    switch (address.GetType())
    {
    case Mac::Address::kTypeNone:
        aMacAddress->mType = OT_MAC_ADDRESS_TYPE_NONE;
        break;

    case Mac::Address::kTypeShort:
        aMacAddress->mType                  = OT_MAC_ADDRESS_TYPE_SHORT;
        aMacAddress->mAddress.mShortAddress = address.GetShort();
        break;

    case Mac::Address::kTypeExtended:
        aMacAddress->mType                = OT_MAC_ADDRESS_TYPE_EXTENDED;
        aMacAddress->mAddress.mExtAddress = address.GetExtended();
        break;
    }

exit:
    return error;
}

uint8_t otMacFrameGetSequence(const otRadioFrame *aFrame)
{
    return static_cast<const Mac::Frame *>(aFrame)->GetSequence();
}

void otMacFrameProcessTransmitAesCcm(otRadioFrame *aFrame, const otExtAddress *aExtAddress)
{
    static_cast<Mac::TxFrame *>(aFrame)->ProcessTransmitAesCcm(*static_cast<const Mac::ExtAddress *>(aExtAddress));
}

bool otMacFrameIsVersion2015(const otRadioFrame *aFrame)
{
    return static_cast<const Mac::Frame *>(aFrame)->IsVersion2015();
}

void otMacFrameGenerateImmAck(const otRadioFrame *aFrame, bool aIsFramePending, otRadioFrame *aAckFrame)
{
    assert(aFrame != nullptr && aAckFrame != nullptr);

    static_cast<Mac::TxFrame *>(aAckFrame)->GenerateImmAck(*static_cast<const Mac::RxFrame *>(aFrame), aIsFramePending);
}

#if OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2
otError otMacFrameGenerateEnhAck(const otRadioFrame *aFrame,
                                 bool                aIsFramePending,
                                 const uint8_t *     aIeData,
                                 uint8_t             aIeLength,
                                 otRadioFrame *      aAckFrame)
{
    assert(aFrame != nullptr && aAckFrame != nullptr);

    return static_cast<Mac::TxFrame *>(aAckFrame)->GenerateEnhAck(*static_cast<const Mac::RxFrame *>(aFrame),
                                                                  aIsFramePending, aIeData, aIeLength);
}
#endif

#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
void otMacFrameSetCslIe(otRadioFrame *aFrame, uint16_t aCslPeriod, uint16_t aCslPhase)
{
    static_cast<Mac::Frame *>(aFrame)->SetCslIe(aCslPeriod, aCslPhase);
}
#endif // OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE

bool otMacFrameIsSecurityEnabled(otRadioFrame *aFrame)
{
    return static_cast<const Mac::Frame *>(aFrame)->GetSecurityEnabled();
}

bool otMacFrameIsKeyIdMode1(otRadioFrame *aFrame)
{
    uint8_t keyIdMode;
    otError error;

    error = static_cast<const Mac::Frame *>(aFrame)->GetKeyIdMode(keyIdMode);

    return (error == OT_ERROR_NONE) ? (keyIdMode == Mac::Frame::kKeyIdMode1) : false;
}

uint8_t otMacFrameGetKeyId(otRadioFrame *aFrame)
{
    uint8_t keyId = 0;

    IgnoreError(static_cast<const Mac::Frame *>(aFrame)->GetKeyId(keyId));

    return keyId;
}

void otMacFrameSetKeyId(otRadioFrame *aFrame, uint8_t aKeyId)
{
    static_cast<Mac::Frame *>(aFrame)->SetKeyId(aKeyId);
}

uint32_t otMacFrameGetFrameCounter(otRadioFrame *aFrame)
{
    uint32_t frameCounter = UINT32_MAX;

    IgnoreError(static_cast<Mac::Frame *>(aFrame)->GetFrameCounter(frameCounter));

    return frameCounter;
}

void otMacFrameSetFrameCounter(otRadioFrame *aFrame, uint32_t aFrameCounter)
{
    static_cast<Mac::Frame *>(aFrame)->SetFrameCounter(aFrameCounter);
}
