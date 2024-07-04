/*
 *  Copyright (c) 2016-2020, The OpenThread Authors.
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
 *   This file implements MLE Discover Scan process.
 */

#include "discover_scanner.hpp"

#include "common/as_core_type.hpp"
#include "common/code_utils.hpp"
#include "common/locator_getters.hpp"
#include "instance/instance.hpp"
#include "thread/mesh_forwarder.hpp"
#include "thread/mle.hpp"
#include "thread/mle_router.hpp"
#include "thread/version.hpp"

namespace ot {
namespace Mle {

DiscoverScanner::DiscoverScanner(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mScanDoneTask(aInstance)
    , mTimer(aInstance)
    , mFilterIndexes()
    , mState(kStateIdle)
    , mScanChannel(0)
    , mAdvDataLength(0)
    , mEnableFiltering(false)
    , mShouldRestorePanId(false)
{
}

Error DiscoverScanner::Discover(const Mac::ChannelMask &aScanChannels,
                                uint16_t                aPanId,
                                bool                    aJoiner,
                                bool                    aEnableFiltering,
                                const FilterIndexes    *aFilterIndexes,
                                Handler                 aCallback,
                                void                   *aContext)
{
    Error                           error   = kErrorNone;
    Mle::TxMessage                 *message = nullptr;
    Tlv                             tlv;
    Ip6::Address                    destination;
    MeshCoP::DiscoveryRequestTlv    discoveryRequest;
    MeshCoP::JoinerAdvertisementTlv joinerAdvertisement;

    VerifyOrExit(Get<ThreadNetif>().IsUp(), error = kErrorInvalidState);

    VerifyOrExit(mState == kStateIdle, error = kErrorBusy);

    mEnableFiltering = aEnableFiltering;

    if (mEnableFiltering)
    {
        if (aFilterIndexes == nullptr)
        {
            Mac::ExtAddress extAddress;

            Get<Radio>().GetIeeeEui64(extAddress);
            MeshCoP::ComputeJoinerId(extAddress, extAddress);
            MeshCoP::SteeringData::CalculateHashBitIndexes(extAddress, mFilterIndexes);
        }
        else
        {
            mFilterIndexes = *aFilterIndexes;
        }
    }

    mCallback.Set(aCallback, aContext);
    mShouldRestorePanId = false;
    mScanChannels       = Get<Mac::Mac>().GetSupportedChannelMask();

    if (!aScanChannels.IsEmpty())
    {
        mScanChannels.Intersect(aScanChannels);
    }

    VerifyOrExit((message = Get<Mle>().NewMleMessage(Mle::kCommandDiscoveryRequest)) != nullptr, error = kErrorNoBufs);
    message->SetPanId(aPanId);

    // Prepare sub-TLV MeshCoP Discovery Request.
    discoveryRequest.Init();
    discoveryRequest.SetVersion(kThreadVersion);
    discoveryRequest.SetJoiner(aJoiner);

    if (mAdvDataLength != 0)
    {
        // Prepare sub-TLV MeshCoP Joiner Advertisement.
        joinerAdvertisement.Init();
        joinerAdvertisement.SetOui(mOui);
        joinerAdvertisement.SetAdvData(mAdvData, mAdvDataLength);
    }

    // Append Discovery TLV with one or two sub-TLVs.
    tlv.SetType(Tlv::kDiscovery);
    tlv.SetLength(
        static_cast<uint8_t>(discoveryRequest.GetSize() + ((mAdvDataLength != 0) ? joinerAdvertisement.GetSize() : 0)));

    SuccessOrExit(error = message->Append(tlv));
    SuccessOrExit(error = discoveryRequest.AppendTo(*message));

    if (mAdvDataLength != 0)
    {
        SuccessOrExit(error = joinerAdvertisement.AppendTo(*message));
    }

    destination.SetToLinkLocalAllRoutersMulticast();

    SuccessOrExit(error = message->SendTo(destination));

    if ((aPanId == Mac::kPanIdBroadcast) && (Get<Mac::Mac>().GetPanId() == Mac::kPanIdBroadcast))
    {
        // In case a specific PAN ID of a Thread Network to be
        // discovered is not known, Discovery Request messages MUST
        // have the Destination PAN ID in the IEEE 802.15.4 MAC
        // header set to be the Broadcast PAN ID (0xffff) and the
        // Source PAN ID set to a randomly generated value.

        Get<Mac::Mac>().SetPanId(Mac::GenerateRandomPanId());
        mShouldRestorePanId = true;
    }

    mScanChannel = Mac::ChannelMask::kChannelIteratorFirst;
    mState       = (mScanChannels.GetNextChannel(mScanChannel) == kErrorNone) ? kStateScanning : kStateScanDone;

    // For rx-off-when-idle device, temporarily enable receiver during discovery procedure.
    if (!Get<Mle>().IsDisabled() && !Get<Mle>().IsRxOnWhenIdle())
    {
        Get<MeshForwarder>().SetRxOnWhenIdle(true);
    }

    Mle::Log(Mle::kMessageSend, Mle::kTypeDiscoveryRequest, destination);

exit:
    FreeMessageOnError(message, error);
    return error;
}

Error DiscoverScanner::SetJoinerAdvertisement(uint32_t aOui, const uint8_t *aAdvData, uint8_t aAdvDataLength)
{
    Error error = kErrorNone;

    VerifyOrExit((aAdvData != nullptr) && (aAdvDataLength != 0) &&
                     (aAdvDataLength <= MeshCoP::JoinerAdvertisementTlv::kAdvDataMaxLength) && (aOui <= kMaxOui),
                 error = kErrorInvalidArgs);

    mOui           = aOui;
    mAdvDataLength = aAdvDataLength;

    memcpy(mAdvData, aAdvData, aAdvDataLength);

exit:
    return error;
}

Mac::TxFrame *DiscoverScanner::PrepareDiscoveryRequestFrame(Mac::TxFrame &aFrame)
{
    Mac::TxFrame *frame = &aFrame;

    switch (mState)
    {
    case kStateIdle:
    case kStateScanDone:
        // If scan is finished (no more channels to scan), abort the
        // Discovery Request frame tx. The handler callback is invoked &
        // state is cleared from `HandleDiscoveryRequestFrameTxDone()`.
        frame = nullptr;
        break;

    case kStateScanning:
        frame->SetChannel(mScanChannel);
        IgnoreError(Get<Mac::Mac>().SetTemporaryChannel(mScanChannel));
        break;
    }

    return frame;
}

void DiscoverScanner::HandleDiscoveryRequestFrameTxDone(Message &aMessage, Error aError)
{
    switch (mState)
    {
    case kStateIdle:
        break;

    case kStateScanning:
        if ((aError == kErrorNone) || (aError == kErrorChannelAccessFailure))
        {
            // Mark the Discovery Request message for direct tx to ensure it
            // is not dequeued and freed by `MeshForwarder` and is ready for
            // the next scan channel. Also pause message tx on `MeshForwarder`
            // while listening to receive Discovery Responses.
            aMessage.SetDirectTransmission();
            aMessage.SetTimestampToNow();
            Get<MeshForwarder>().PauseMessageTransmissions();
            mTimer.Start(kDefaultScanDuration);
            break;
        }

        // If we encounter other error failures (e.g., `kErrorDrop` due
        // to queue management dropping the message or if message being
        // evicted), `aMessage` may be immediately freed. This prevents
        // us from reusing it to request a scan on the next scan channel.
        // As a result, we stop the scan operation in such cases.

        mState = kStateScanDone;

        OT_FALL_THROUGH;

    case kStateScanDone:
        HandleDiscoverComplete();
        break;
    }
}

void DiscoverScanner::HandleDiscoverComplete(void)
{
    // Restore Data Polling or CSL for rx-off-when-idle device.
    if (!Get<Mle>().IsDisabled() && !Get<Mle>().IsRxOnWhenIdle())
    {
        Get<MeshForwarder>().SetRxOnWhenIdle(false);
    }

    switch (mState)
    {
    case kStateIdle:
        break;

    case kStateScanning:
        mTimer.Stop();
        Get<MeshForwarder>().ResumeMessageTransmissions();

        OT_FALL_THROUGH;

    case kStateScanDone:
        Get<Mac::Mac>().ClearTemporaryChannel();

        if (mShouldRestorePanId)
        {
            Get<Mac::Mac>().SetPanId(Mac::kPanIdBroadcast);
            mShouldRestorePanId = false;
        }

        // Post the tasklet to change `mState` and invoke handler
        // callback. This allows users to safely call OT APIs from
        // the callback.
        mScanDoneTask.Post();
        break;
    }
}

void DiscoverScanner::HandleScanDoneTask(void)
{
    mState = kStateIdle;
    mCallback.InvokeIfSet(nullptr);
}

void DiscoverScanner::HandleTimer(void)
{
    VerifyOrExit(mState == kStateScanning);

    // Move to next scan channel and resume message transmissions on
    // `MeshForwarder` so that the queued MLE Discovery Request message
    // is prepared again for the next scan channel. If no more channel
    // to scan, change the state to `kStateScanDone` which ensures the
    // frame tx is aborted  from `PrepareDiscoveryRequestFrame()` and
    // then wraps up the scan (invoking handler callback).

    if (mScanChannels.GetNextChannel(mScanChannel) != kErrorNone)
    {
        mState = kStateScanDone;
    }

    Get<MeshForwarder>().ResumeMessageTransmissions();

exit:
    return;
}

void DiscoverScanner::HandleDiscoveryResponse(Mle::RxInfo &aRxInfo) const
{
    Error                         error = kErrorNone;
    MeshCoP::DiscoveryResponseTlv discoveryResponse;
    ScanResult                    result;
    OffsetRange                   offsetRange;
    Tlv::ParsedInfo               tlvInfo;
    bool                          didCheckSteeringData = false;

    Mle::Log(Mle::kMessageReceive, Mle::kTypeDiscoveryResponse, aRxInfo.mMessageInfo.GetPeerAddr());

    VerifyOrExit(mState == kStateScanning, error = kErrorDrop);

    // Find MLE Discovery TLV
    SuccessOrExit(error = Tlv::FindTlvValueOffsetRange(aRxInfo.mMessage, Tlv::kDiscovery, offsetRange));

    ClearAllBytes(result);
    result.mDiscover = true;
    result.mPanId    = aRxInfo.mMessage.GetPanId();
    result.mChannel  = aRxInfo.mMessage.GetChannel();
    result.mRssi     = aRxInfo.mMessage.GetAverageRss();
    result.mLqi      = aRxInfo.mMessage.GetAverageLqi();

    aRxInfo.mMessageInfo.GetPeerAddr().GetIid().ConvertToExtAddress(AsCoreType(&result.mExtAddress));

    for (; !offsetRange.IsEmpty(); offsetRange.AdvanceOffset(tlvInfo.GetSize()))
    {
        SuccessOrExit(error = tlvInfo.ParseFrom(aRxInfo.mMessage, offsetRange));

        if (tlvInfo.mIsExtended)
        {
            continue;
        }

        switch (tlvInfo.mType)
        {
        case MeshCoP::Tlv::kDiscoveryResponse:
            SuccessOrExit(error = aRxInfo.mMessage.Read(offsetRange, discoveryResponse));
            VerifyOrExit(discoveryResponse.IsValid(), error = kErrorParse);
            result.mVersion  = discoveryResponse.GetVersion();
            result.mIsNative = discoveryResponse.IsNativeCommissioner();
            break;

        case MeshCoP::Tlv::kExtendedPanId:
            SuccessOrExit(error = Tlv::Read<MeshCoP::ExtendedPanIdTlv>(aRxInfo.mMessage, offsetRange.GetOffset(),
                                                                       AsCoreType(&result.mExtendedPanId)));
            break;

        case MeshCoP::Tlv::kNetworkName:
            SuccessOrExit(error = Tlv::Read<MeshCoP::NetworkNameTlv>(aRxInfo.mMessage, offsetRange.GetOffset(),
                                                                     result.mNetworkName.m8));
            break;

        case MeshCoP::Tlv::kSteeringData:
            if (!tlvInfo.mValueOffsetRange.IsEmpty())
            {
                MeshCoP::SteeringData &steeringData     = AsCoreType(&result.mSteeringData);
                OffsetRange            valueOffsetRange = tlvInfo.mValueOffsetRange;

                valueOffsetRange.ShrinkLength(MeshCoP::SteeringData::kMaxLength);
                steeringData.Init(static_cast<uint8_t>(valueOffsetRange.GetLength()));
                aRxInfo.mMessage.ReadBytes(valueOffsetRange, steeringData.GetData());

                if (mEnableFiltering)
                {
                    VerifyOrExit(steeringData.Contains(mFilterIndexes));
                }

                didCheckSteeringData = true;
            }
            break;

        case MeshCoP::Tlv::kJoinerUdpPort:
            SuccessOrExit(error = Tlv::Read<MeshCoP::JoinerUdpPortTlv>(aRxInfo.mMessage, offsetRange.GetOffset(),
                                                                       result.mJoinerUdpPort));
            break;

        default:
            break;
        }
    }

    VerifyOrExit(!mEnableFiltering || didCheckSteeringData);

    mCallback.InvokeIfSet(&result);

exit:
    Mle::LogProcessError(Mle::kTypeDiscoveryResponse, error);
}

} // namespace Mle
} // namespace ot
