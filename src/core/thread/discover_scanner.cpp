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

#include "instance/instance.hpp"

namespace ot {
namespace Mle {

DiscoverScanner::DiscoverScanner(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mScanDoneTask(aInstance)
    , mTimer(aInstance)
    , mFilterIndexes()
    , mState(kStateIdle)
    , mScanChannel(0)
    , mEnableFiltering(false)
    , mShouldRestorePanId(false)
#if OPENTHREAD_CONFIG_JOINER_ADV_EXPERIMENTAL_ENABLE
    , mAdvDataLength(0)
#endif
{
}

Error DiscoverScanner::Discover(const Mac::ChannelMask &aScanChannels,
                                uint16_t                aPanId,
                                bool                    aJoiner,
                                bool                    aEnableFiltering,
                                const FilterIndexes    *aFilterIndexes,
                                ScanResult::Handler     aHandler,
                                void                   *aContext)
{
    Error                             error   = kErrorNone;
    Mle::TxMessage                   *message = nullptr;
    Tlv::Bookmark                     tlvBookmark;
    MeshCoP::DiscoveryRequestTlvValue discoveryRequestTlvValue;

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

    mCallback.Set(aHandler, aContext);
    mShouldRestorePanId = false;
    mScanChannels       = Get<Mac::Mac>().GetSupportedChannelMask();

    if (!aScanChannels.IsEmpty())
    {
        mScanChannels.Intersect(aScanChannels);
    }

    VerifyOrExit((message = Get<Mle>().NewMleMessage(kCommandDiscoveryRequest)) != nullptr, error = kErrorNoBufs);
    message->SetPanId(aPanId);

    // Append Discovery TLV with one or two sub-TLVs.

    SuccessOrExit(error = Tlv::StartTlv(*message, Tlv::kDiscovery, tlvBookmark));

    discoveryRequestTlvValue.Clear();
    discoveryRequestTlvValue.SetVersion(kThreadVersion);

    if (aJoiner)
    {
        discoveryRequestTlvValue.SetJoinerFlag();
    }

    SuccessOrExit(error = Tlv::Append<MeshCoP::DiscoveryRequestTlv>(*message, discoveryRequestTlvValue));

#if OPENTHREAD_CONFIG_JOINER_ADV_EXPERIMENTAL_ENABLE
    if (mAdvDataLength != 0)
    {
        MeshCoP::JoinerAdvertisementTlv joinerAdvTlv;

        joinerAdvTlv.Init();
        joinerAdvTlv.SetOui(mOui);
        joinerAdvTlv.SetAdvData(mAdvData, mAdvDataLength);
        SuccessOrExit(error = joinerAdvTlv.AppendTo(*message));
    }
#endif

    SuccessOrExit(error = Tlv::EndTlv(*message, tlvBookmark));

    message->RegisterTxCallback(HandleDiscoveryRequestFrameTxDone, this);

    SuccessOrExit(error = message->SendTo(Ip6::Address::GetLinkLocalAllRoutersMulticast()));

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

    Mle::Log(Mle::kMessageSend, Mle::kTypeDiscoveryRequest, Ip6::Address::GetLinkLocalAllRoutersMulticast());

exit:
    FreeMessageOnError(message, error);
    return error;
}

#if OPENTHREAD_CONFIG_JOINER_ADV_EXPERIMENTAL_ENABLE
Error DiscoverScanner::SetJoinerAdvertisement(uint32_t aOui, const uint8_t *aAdvData, uint8_t aAdvDataLength)
{
    Error error = kErrorNone;

    VerifyOrExit(aAdvData != nullptr, error = kErrorInvalidArgs);
    VerifyOrExit(IsValueInRange(aAdvDataLength, kMinAdvDataLength, kMaxAdvDataLength), error = kErrorInvalidArgs);
    VerifyOrExit(aOui <= kMaxOui, error = kErrorInvalidArgs);

    mOui           = aOui;
    mAdvDataLength = aAdvDataLength;
    memcpy(mAdvData, aAdvData, aAdvDataLength);

exit:
    return error;
}
#endif

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

void DiscoverScanner::HandleDiscoveryRequestFrameTxDone(const otMessage *aMessage, otError aError, void *aContext)
{
    // Since we prepared the discovery message originally, we can
    // safely cast away the `const` from it.

    static_cast<DiscoverScanner *>(aContext)->HandleDiscoveryRequestFrameTxDone(AsNonConst(AsCoreType(aMessage)),
                                                                                aError);
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
            aMessage.RegisterTxCallback(HandleDiscoveryRequestFrameTxDone, this);
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
    Error                              error = kErrorNone;
    ScanResult                         result;
    OffsetRange                        offsetRange;
    MeshCoP::DiscoveryResponseTlvValue respTlvValue;
    MeshCoP::SteeringDataTlv           steeringDataTlv;

    Mle::Log(Mle::kMessageReceive, Mle::kTypeDiscoveryResponse, aRxInfo.mMessageInfo.GetPeerAddr());

    VerifyOrExit(mState == kStateScanning, error = kErrorDrop);

    // Find MLE Discovery TLV and restrict the message to this TLV value,
    // so we can parse all the included MeshCoP sub-TLVs within this TLV.

    SuccessOrExit(error = Tlv::FindTlvValueOffsetRange(aRxInfo.mMessage, Tlv::kDiscovery, offsetRange));

    aRxInfo.mMessage.SetOffset(offsetRange.GetOffset());
    IgnoreError(aRxInfo.mMessage.SetLength(offsetRange.GetEndOffset()));

    result.Clear();
    result.mDiscover = true;
    result.mPanId    = aRxInfo.mMessage.GetPanId();
    result.mChannel  = aRxInfo.mMessage.GetChannel();
    result.mRssi     = aRxInfo.mMessage.GetAverageRss();
    result.mLqi      = aRxInfo.mMessage.GetAverageLqi();

    AsCoreType(&result.mExtAddress).SetFromIid(aRxInfo.mMessageInfo.GetPeerAddr().GetIid());

    // Required TLVs

    SuccessOrExit(error = Tlv::Find<MeshCoP::DiscoveryResponseTlv>(aRxInfo.mMessage, respTlvValue));
    result.mVersion  = respTlvValue.GetVersion();
    result.mIsNative = respTlvValue.GetNativeCommissionerFlag();

    SuccessOrExit(error = Tlv::Find<MeshCoP::ExtendedPanIdTlv>(aRxInfo.mMessage, AsCoreType(&result.mExtendedPanId)));
    SuccessOrExit(error = Tlv::Find<MeshCoP::NetworkNameTlv>(aRxInfo.mMessage, result.mNetworkName.m8));

    // Optional TLVs

    switch (Tlv::Find<MeshCoP::JoinerUdpPortTlv>(aRxInfo.mMessage, result.mJoinerUdpPort))
    {
    case kErrorNone:
        break;
    case kErrorNotFound:
        result.mJoinerUdpPort = 0;
        break;
    default:
        ExitNow(error = kErrorParse);
    }

    switch (Tlv::FindTlv(aRxInfo.mMessage, steeringDataTlv))
    {
    case kErrorNone:
        if (steeringDataTlv.IsValid())
        {
            MeshCoP::SteeringData &steeringData = AsCoreType(&result.mSteeringData);

            IgnoreError(steeringDataTlv.CopyTo(steeringData));

            if (mEnableFiltering)
            {
                VerifyOrExit(steeringData.Contains(mFilterIndexes));
            }
        }

        break;

    case kErrorNotFound:
        VerifyOrExit(!mEnableFiltering);
        break;

    default:
        ExitNow(error = kErrorParse);
    }

    mCallback.InvokeIfSet(&result);

exit:
    Mle::LogProcessError(Mle::kTypeDiscoveryResponse, error);
}

} // namespace Mle
} // namespace ot
