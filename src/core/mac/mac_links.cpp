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

/**
 * @file
 *   This file implements the MAC radio links.
 */

#include "mac_links.hpp"

#include "instance/instance.hpp"

namespace ot {
namespace Mac {

RegisterLogModule("MacLinks");

//---------------------------------------------------------------------------------------------------------------------
// TxFrames

#if OPENTHREAD_FTD || OPENTHREAD_MTD
TxFrames::TxFrames(Instance &aInstance)
    : InstanceLocator(aInstance)
#if OPENTHREAD_CONFIG_RADIO_LINK_IEEE_802_15_4_ENABLE
    , mTxFrame802154(aInstance.Get<SubMac>().GetTransmitFrame())
#endif
#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
    , mTxFrameTrel(aInstance.Get<Trel::Link>().GetTransmitFrame())
#endif
{
}

#if OPENTHREAD_CONFIG_MULTI_RADIO

TxFrame &TxFrames::GetTxFrame(RadioType aRadioType)
{
    TxFrame *frame = nullptr;

    switch (aRadioType)
    {
#if OPENTHREAD_CONFIG_RADIO_LINK_IEEE_802_15_4_ENABLE
    case kRadioTypeIeee802154:
        frame = &mTxFrame802154;
        break;
#endif
#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
    case kRadioTypeTrel:
        frame = &mTxFrameTrel;
        break;
#endif
    }

    mSelectedRadioTypes.Add(aRadioType);

    return *frame;
}

TxFrame &TxFrames::GetTxFrame(RadioTypes aRadioTypes)
{
    // Return the TxFrame among all set of `aRadioTypes` with the smallest MTU.
    // Note that this is `TxFrame` to be sent out in parallel over multiple radio
    // radio links in `aRadioTypes, so we need to make sure that it fits in the
    // most restricted radio link (with smallest MTU).

    TxFrame *frame = nullptr;

#if OPENTHREAD_CONFIG_RADIO_LINK_IEEE_802_15_4_ENABLE
    if (aRadioTypes.Contains(kRadioTypeIeee802154))
    {
        frame = &mTxFrame802154;
    }
#endif

#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
    if (aRadioTypes.Contains(kRadioTypeTrel) && ((frame == nullptr) || (frame->GetMtu() > mTxFrameTrel.GetMtu())))
    {
        frame = &mTxFrameTrel;
    }
#endif

    mSelectedRadioTypes.Add(aRadioTypes);

    return *frame;
}

TxFrame &TxFrames::GetBroadcastTxFrame(void)
{
    RadioTypes allRadios;

    allRadios.AddAll();
    return GetTxFrame(allRadios);
}

#endif // #if OPENTHREAD_CONFIG_MULTI_RADIO
#endif // OPENTHREAD_FTD || OPENTHREAD_MTD

//---------------------------------------------------------------------------------------------------------------------
// Links

Links::Links(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mSubMac(aInstance)
#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE && (OPENTHREAD_FTD || OPENTHREAD_MTD)
    , mTrel(aInstance)
#endif
#if OPENTHREAD_FTD || OPENTHREAD_MTD
    , mTxFrames(aInstance)
#endif
{
    Init();
}

void Links::Init(void)
{
    mSubMac.Init();
#if OPENTHREAD_CONFIG_LINK_RAW_ENABLE
    mEnergyScanDoneCallback = nullptr;
    mTransmitDoneCallback   = nullptr;
    mReceiveDoneCallback    = nullptr;
#endif
    mExtAddress.Clear();
    mShortAddress          = kShortAddrInvalid;
    mAlternateShortAddress = kShortAddrInvalid;
}

void Links::SetShortAddress(ShortAddress aShortAddress)
{
    mShortAddress = aShortAddress;
    Get<Radio>().SetShortAddress(aShortAddress);
    LogDebg("RadioShortAddress: 0x%04x", mShortAddress);
}

void Links::SetAlternateShortAddress(ShortAddress aShortAddress)
{
    mAlternateShortAddress = aShortAddress;
    Get<Radio>().SetAlternateShortAddress(aShortAddress);
    LogDebg("RadioAlternateShortAddress: 0x%04x", mAlternateShortAddress);
}

void Links::SetExtAddress(const ExtAddress &aExtAddress)
{
    mExtAddress = aExtAddress;
    Get<Radio>().SetExtendedAddress(aExtAddress);
#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE && (OPENTHREAD_FTD || OPENTHREAD_MTD)
    mTrel.HandleExtAddressChange();
#endif
    LogDebg("RadioExtAddress: %s", mExtAddress.ToString().AsCString());
}

#if OPENTHREAD_RADIO || OPENTHREAD_CONFIG_LINK_RAW_ENABLE
void Links::InvokeReceiveDone(RxFrame *aFrame, Error aError)
{
    LogDebg("ReceiveDone(%d bytes), error:%s", (aFrame != nullptr) ? aFrame->mLength : 0, ErrorToString(aError));

    if (mReceiveDoneCallback)
    {
        if (aError == kErrorNone)
        {
            mReceiveDoneCallback(&GetInstance(), aFrame, aError);
        }
    }
#if OPENTHREAD_FTD || OPENTHREAD_MTD
    else
    {
        Get<Mac>().HandleReceivedFrame(aFrame, aError);
    }
#endif
}

Error Links::Transmit(otLinkRawTransmitDone aCallback)
{
    Error error = kErrorNone;

    SuccessOrExit(error = mSubMac.Send());
    mTransmitDoneCallback = aCallback;

exit:
    return error;
}

void Links::InvokeTransmitDone(TxFrame &aFrame, RxFrame *aAckFrame, Error aError)
{
    LogDebg("TransmitDone(%d bytes), error:%s", aFrame.mLength, ErrorToString(aError));

    if (mTransmitDoneCallback)
    {
        mTransmitDoneCallback(&GetInstance(), &aFrame, aAckFrame, aError);
        mTransmitDoneCallback = nullptr;
    }
#if OPENTHREAD_FTD || OPENTHREAD_MTD
    else
    {
        Get<Mac>().HandleTransmitDone(aFrame, aAckFrame, aError);
    }
#endif
}

Error Links::EnergyScan(uint8_t aScanChannel, uint16_t aScanDuration, otLinkRawEnergyScanDone aCallback)
{
    Error error;

    SuccessOrExit(error = EnergyScan(aScanChannel, aScanDuration));
    mEnergyScanDoneCallback = aCallback;

exit:
    return error;
}

void Links::InvokeEnergyScanDone(int8_t aEnergyScanMaxRssi)
{
    if (mEnergyScanDoneCallback != nullptr)
    {
        mEnergyScanDoneCallback(&GetInstance(), aEnergyScanMaxRssi);
        mEnergyScanDoneCallback = nullptr;
    }
#if OPENTHREAD_FTD || OPENTHREAD_MTD
    else
    {
        Get<Mac>().EnergyScanDone(aEnergyScanMaxRssi);
    }
#endif
}

void Links::SetMacKey(uint8_t aKeyIdMode, uint8_t aKeyId, const Key &aPrevKey, const Key &aCurrKey, const Key &aNextKey)
{
    KeyMaterial prevKey;
    KeyMaterial currKey;
    KeyMaterial nextKey;

    prevKey.SetFrom(aPrevKey);
    currKey.SetFrom(aCurrKey);
    nextKey.SetFrom(aNextKey);

    mSubMac.SetMacKey(aKeyIdMode, aKeyId, prevKey, currKey, nextKey);
}

void Links::SetMacFrameCounter(uint32_t aFrameCounter, bool aSetIfLarger)
{
    mSubMac.SetFrameCounter(aFrameCounter, aSetIfLarger);
}

#if OT_SHOULD_LOG_AT(OT_LOG_LEVEL_INFO)
void Links::RecordFrameTransmitStatus(const TxFrame &aFrame, Error aError, uint8_t aRetryCount, bool aWillRetx)
{
    OT_UNUSED_VARIABLE(aWillRetx);

    if (aError != kErrorNone)
    {
        LogInfo("Frame tx failed, error:%s, retries:%d/%d, %s", ErrorToString(aError), aRetryCount,
                aFrame.GetMaxFrameRetries(), aFrame.ToInfoString().AsCString());
    }
}
#endif
#endif // OPENTHREAD_RADIO || OPENTHREAD_CONFIG_LINK_RAW_ENABLE

#if OPENTHREAD_FTD || OPENTHREAD_MTD
#if OPENTHREAD_CONFIG_MULTI_RADIO

void Links::Send(TxFrame &aFrame, RadioTypes aRadioTypes)
{
#if OPENTHREAD_CONFIG_RADIO_LINK_IEEE_802_15_4_ENABLE
    if (aRadioTypes.Contains(kRadioTypeIeee802154) && mTxFrames.mTxFrame802154.IsEmpty())
    {
        mTxFrames.mTxFrame802154.CopyFrom(aFrame);
    }
#endif
#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
    if (aRadioTypes.Contains(kRadioTypeTrel) && mTxFrames.mTxFrameTrel.IsEmpty())
    {
        mTxFrames.mTxFrameTrel.CopyFrom(aFrame);
    }
#endif

#if OPENTHREAD_CONFIG_RADIO_LINK_IEEE_802_15_4_ENABLE
    if (aRadioTypes.Contains(kRadioTypeIeee802154))
    {
        SuccessOrAssert(mSubMac.Send());
    }
#endif
#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
    if (aRadioTypes.Contains(kRadioTypeTrel))
    {
        mTrel.Send();
    }
#endif
}

#endif // #if OPENTHREAD_CONFIG_MULTI_RADIO

const KeyMaterial *Links::GetCurrentMacKey(const Frame &aFrame) const
{
    // Gets the security MAC key (for Key Mode 1) based on radio link type of `aFrame`.

    const KeyMaterial *key = nullptr;
#if OPENTHREAD_CONFIG_MULTI_RADIO
    RadioType radioType = aFrame.GetRadioType();
#endif

#if OPENTHREAD_CONFIG_RADIO_LINK_IEEE_802_15_4_ENABLE
#if OPENTHREAD_CONFIG_MULTI_RADIO
    if (radioType == kRadioTypeIeee802154)
#endif
    {
        ExitNow(key = &Get<SubMac>().GetCurrentMacKey());
    }
#endif

#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
#if OPENTHREAD_CONFIG_MULTI_RADIO
    if (radioType == kRadioTypeTrel)
#endif
    {
        ExitNow(key = &Get<KeyManager>().GetCurrentTrelMacKey());
    }
#endif

    OT_UNUSED_VARIABLE(aFrame);

exit:
    return key;
}

const KeyMaterial *Links::GetTemporaryMacKey(const Frame &aFrame, uint32_t aKeySequence) const
{
    // Gets the security MAC key (for Key Mode 1) based on radio link
    // type of `aFrame` and given Key Sequence.

    const KeyMaterial *key = nullptr;
#if OPENTHREAD_CONFIG_MULTI_RADIO
    RadioType radioType = aFrame.GetRadioType();
#endif

#if OPENTHREAD_CONFIG_RADIO_LINK_IEEE_802_15_4_ENABLE
#if OPENTHREAD_CONFIG_MULTI_RADIO
    if (radioType == kRadioTypeIeee802154)
#endif
    {
        if (aKeySequence == Get<KeyManager>().GetCurrentKeySequence() - 1)
        {
            ExitNow(key = &Get<SubMac>().GetPreviousMacKey());
        }
        else if (aKeySequence == Get<KeyManager>().GetCurrentKeySequence() + 1)
        {
            ExitNow(key = &Get<SubMac>().GetNextMacKey());
        }
        else
        {
            OT_ASSERT(false);
        }
    }
#endif

#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
#if OPENTHREAD_CONFIG_MULTI_RADIO
    if (radioType == kRadioTypeTrel)
#endif
    {
        ExitNow(key = &Get<KeyManager>().GetTemporaryTrelMacKey(aKeySequence));
    }
#endif

    OT_UNUSED_VARIABLE(aFrame);

exit:
    return key;
}

#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
void Links::SetMacFrameCounter(TxFrame &aFrame)
{
#if OPENTHREAD_CONFIG_MULTI_RADIO
    RadioType radioType = aFrame.GetRadioType();
#endif

#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
#if OPENTHREAD_CONFIG_MULTI_RADIO
    if (radioType == kRadioTypeTrel)
#endif
    {
        aFrame.SetFrameCounter(Get<KeyManager>().GetTrelMacFrameCounter());
        Get<KeyManager>().IncrementTrelMacFrameCounter();
        ExitNow();
    }
#endif

exit:
    return;
}
#endif // #if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
#endif // #if OPENTHREAD_FTD || OPENTHREAD_MTD

} // namespace Mac
} // namespace ot
