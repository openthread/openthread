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

#include "common/code_utils.hpp"
#include "common/instance.hpp"
#include "common/locator-getters.hpp"
#include "common/logging.hpp"
#include "thread/mesh_forwarder.hpp"
#include "thread/mle.hpp"
#include "thread/mle_router.hpp"

namespace ot {
namespace Mle {

DiscoverScanner::DiscoverScanner(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mHandler(nullptr)
    , mHandlerContext(nullptr)
    , mTimer(aInstance, DiscoverScanner::HandleTimer, this)
    , mFilterIndexes()
    , mScanChannels()
    , mState(kStateIdle)
    , mScanChannel(0)
    , mAdvDataLength(0)
    , mEnableFiltering(false)
    , mShouldRestorePanId(false)
{
}

otError DiscoverScanner::Discover(const Mac::ChannelMask &aScanChannels,
                                  uint16_t                aPanId,
                                  bool                    aJoiner,
                                  bool                    aEnableFiltering,
                                  const FilterIndexes *   aFilterIndexes,
                                  Handler                 aCallback,
                                  void *                  aContext)
{
    otError                         error   = OT_ERROR_NONE;
    Message *                       message = nullptr;
    Tlv                             tlv;
    Ip6::Address                    destination;
    MeshCoP::DiscoveryRequestTlv    discoveryRequest;
    MeshCoP::JoinerAdvertisementTlv joinerAdvertisement;

    VerifyOrExit(mState == kStateIdle, error = OT_ERROR_BUSY);

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

    mHandler            = aCallback;
    mHandlerContext     = aContext;
    mShouldRestorePanId = false;
    mScanChannels       = Get<Mac::Mac>().GetSupportedChannelMask();

    if (!aScanChannels.IsEmpty())
    {
        mScanChannels.Intersect(aScanChannels);
    }

    VerifyOrExit((message = Get<Mle>().NewMleMessage()) != nullptr, error = OT_ERROR_NO_BUFS);
    message->SetSubType(Message::kSubTypeMleDiscoverRequest);
    message->SetPanId(aPanId);
    SuccessOrExit(error = Get<Mle>().AppendHeader(*message, Mle::kCommandDiscoveryRequest));

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

    SuccessOrExit(error = message->Append(&tlv, sizeof(tlv)));
    SuccessOrExit(error = discoveryRequest.AppendTo(*message));

    if (mAdvDataLength != 0)
    {
        SuccessOrExit(error = joinerAdvertisement.AppendTo(*message));
    }

    destination.SetToLinkLocalAllRoutersMulticast();

    SuccessOrExit(error = Get<Mle>().SendMessage(*message, destination));

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
    mState       = (mScanChannels.GetNextChannel(mScanChannel) == OT_ERROR_NONE) ? kStateScanning : kStateScanDone;

    Mle::Log(Mle::kMessageSend, Mle::kTypeDiscoveryRequest, destination);

exit:
    FreeMessageOnError(message, error);
    return error;
}

otError DiscoverScanner::SetJoinerAdvertisement(uint32_t aOui, const uint8_t *aAdvData, uint8_t aAdvDataLength)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit((aAdvData != nullptr) && (aAdvDataLength != 0) &&
                     (aAdvDataLength <= MeshCoP::JoinerAdvertisementTlv::kAdvDataMaxLength) && (aOui <= kMaxOui),
                 error = OT_ERROR_INVALID_ARGS);

    mOui           = aOui;
    mAdvDataLength = aAdvDataLength;

    memcpy(mAdvData, aAdvData, aAdvDataLength);

exit:
    return error;
}

otError DiscoverScanner::PrepareDiscoveryRequestFrame(Mac::TxFrame &aFrame)
{
    otError error = OT_ERROR_NONE;

    switch (mState)
    {
    case kStateIdle:
    case kStateScanDone:
        // If scan is finished (no more channels to scan), abort the
        // Discovery Request frame tx. The handler callback is invoked &
        // state is cleared from `HandleDiscoveryRequestFrameTxDone()`.
        error = OT_ERROR_ABORT;
        break;

    case kStateScanning:
        aFrame.SetChannel(mScanChannel);
        IgnoreError(Get<Mac::Mac>().SetTemporaryChannel(mScanChannel));
        break;
    }

    return error;
}

void DiscoverScanner::HandleDiscoveryRequestFrameTxDone(Message &aMessage)
{
    switch (mState)
    {
    case kStateIdle:
        break;

    case kStateScanning:
        // Mark the Discovery Request message for direct tx to ensure it
        // is not dequeued and freed by `MeshForwarder` and is ready for
        // the next scan channel. Also pause message tx on `MeshForwarder`
        // while listening to receive Discovery Responses.
        aMessage.SetDirectTransmission();
        Get<MeshForwarder>().PauseMessageTransmissions();
        mTimer.Start(kDefaultScanDuration);
        break;

    case kStateScanDone:
        HandleDiscoverComplete();
        break;
    }
}

void DiscoverScanner::HandleDiscoverComplete(void)
{
    switch (mState)
    {
    case kStateIdle:
        break;

    case kStateScanning:
        mTimer.Stop();
        Get<MeshForwarder>().ResumeMessageTransmissions();

        // Fall through

    case kStateScanDone:
        Get<Mac::Mac>().ClearTemporaryChannel();

        if (mShouldRestorePanId)
        {
            Get<Mac::Mac>().SetPanId(Mac::kPanIdBroadcast);
            mShouldRestorePanId = false;
        }

        mState = kStateIdle;

        if (mHandler)
        {
            mHandler(nullptr, mHandlerContext);
        }

        break;
    }
}

void DiscoverScanner::HandleTimer(Timer &aTimer)
{
    aTimer.GetOwner<DiscoverScanner>().HandleTimer();
}

void DiscoverScanner::HandleTimer(void)
{
    VerifyOrExit(mState == kStateScanning, OT_NOOP);

    // Move to next scan channel and resume message transmissions on
    // `MeshForwarder` so that the queued MLE Discovery Request message
    // is prepared again for the next scan channel. If no more channel
    // to scan, change the state to `kStateScanDone` which ensures the
    // frame tx is aborted  from `PrepareDiscoveryRequestFrame()` and
    // then wraps up the scan (invoking handler callback).

    if (mScanChannels.GetNextChannel(mScanChannel) != OT_ERROR_NONE)
    {
        mState = kStateScanDone;
    }

    Get<MeshForwarder>().ResumeMessageTransmissions();

exit:
    return;
}

void DiscoverScanner::HandleDiscoveryResponse(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo) const
{
    otError                       error    = OT_ERROR_NONE;
    const ThreadLinkInfo *        linkInfo = aMessageInfo.GetThreadLinkInfo();
    Tlv                           tlv;
    MeshCoP::Tlv                  meshcopTlv;
    MeshCoP::DiscoveryResponseTlv discoveryResponse;
    MeshCoP::NetworkNameTlv       networkName;
    ScanResult                    result;
    uint16_t                      offset;
    uint16_t                      end;
    bool                          didCheckSteeringData = false;

    Mle::Log(Mle::kMessageReceive, Mle::kTypeDiscoveryResponse, aMessageInfo.GetPeerAddr());

    VerifyOrExit(mState == kStateScanning, error = OT_ERROR_DROP);

    // Find MLE Discovery TLV
    VerifyOrExit(Tlv::FindTlvOffset(aMessage, Tlv::kDiscovery, offset) == OT_ERROR_NONE, error = OT_ERROR_PARSE);
    aMessage.Read(offset, sizeof(tlv), &tlv);

    offset += sizeof(tlv);
    end = offset + tlv.GetLength();

    memset(&result, 0, sizeof(result));
    result.mPanId   = linkInfo->mPanId;
    result.mChannel = linkInfo->mChannel;
    result.mRssi    = linkInfo->mRss;
    result.mLqi     = linkInfo->mLqi;

    aMessageInfo.GetPeerAddr().GetIid().ConvertToExtAddress(static_cast<Mac::ExtAddress &>(result.mExtAddress));

    // Process MeshCoP TLVs
    while (offset < end)
    {
        aMessage.Read(offset, sizeof(meshcopTlv), &meshcopTlv);

        switch (meshcopTlv.GetType())
        {
        case MeshCoP::Tlv::kDiscoveryResponse:
            aMessage.Read(offset, sizeof(discoveryResponse), &discoveryResponse);
            VerifyOrExit(discoveryResponse.IsValid(), error = OT_ERROR_PARSE);
            result.mVersion  = discoveryResponse.GetVersion();
            result.mIsNative = discoveryResponse.IsNativeCommissioner();
            break;

        case MeshCoP::Tlv::kExtendedPanId:
            SuccessOrExit(error = Tlv::ReadTlv(aMessage, offset, &result.mExtendedPanId, sizeof(Mac::ExtendedPanId)));
            break;

        case MeshCoP::Tlv::kNetworkName:
            aMessage.Read(offset, sizeof(networkName), &networkName);
            IgnoreError(static_cast<Mac::NetworkName &>(result.mNetworkName).Set(networkName.GetNetworkName()));
            break;

        case MeshCoP::Tlv::kSteeringData:
            if (meshcopTlv.GetLength() > 0)
            {
                MeshCoP::SteeringData &steeringData = static_cast<MeshCoP::SteeringData &>(result.mSteeringData);
                uint8_t                dataLength   = MeshCoP::SteeringData::kMaxLength;

                if (meshcopTlv.GetLength() < dataLength)
                {
                    dataLength = meshcopTlv.GetLength();
                }

                steeringData.Init(dataLength);

                SuccessOrExit(error = Tlv::ReadTlv(aMessage, offset, steeringData.GetData(), dataLength));

                if (mEnableFiltering)
                {
                    VerifyOrExit(steeringData.Contains(mFilterIndexes), OT_NOOP);
                }

                didCheckSteeringData = true;
            }
            break;

        case MeshCoP::Tlv::kJoinerUdpPort:
            SuccessOrExit(error = Tlv::ReadUint16Tlv(aMessage, offset, result.mJoinerUdpPort));
            break;

        default:
            break;
        }

        offset += sizeof(meshcopTlv) + meshcopTlv.GetLength();
    }

    VerifyOrExit(!mEnableFiltering || didCheckSteeringData, OT_NOOP);

    if (mHandler)
    {
        mHandler(&result, mHandlerContext);
    }

exit:
    Mle::LogProcessError(Mle::kTypeDiscoveryResponse, error);
}

} // namespace Mle
} // namespace ot
