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
 *   This file implements mesh forwarding of IPv6/6LoWPAN messages.
 */

#include "mesh_forwarder.hpp"

#include "instance/instance.hpp"
#include "utils/static_counter.hpp"

namespace ot {

RegisterLogModule("MeshForwarder");

void ThreadLinkInfo::SetFrom(const Mac::RxFrame &aFrame)
{
    Clear();

    if (kErrorNone != aFrame.GetSrcPanId(mPanId))
    {
        IgnoreError(aFrame.GetDstPanId(mPanId));
    }

    {
        Mac::PanId dstPanId;

        if (kErrorNone != aFrame.GetDstPanId(dstPanId))
        {
            dstPanId = mPanId;
        }

        mIsDstPanIdBroadcast = (dstPanId == Mac::kPanIdBroadcast);
    }

    if (aFrame.GetSecurityEnabled())
    {
        uint8_t keyIdMode;

        // MAC Frame Security was already validated at the MAC
        // layer. As a result, `GetKeyIdMode()` will never return
        // failure here.
        IgnoreError(aFrame.GetKeyIdMode(keyIdMode));

        mLinkSecurity = (keyIdMode == Mac::Frame::kKeyIdMode0) || (keyIdMode == Mac::Frame::kKeyIdMode1);
    }
    else
    {
        mLinkSecurity = false;
    }
    mChannel = aFrame.GetChannel();
    mRss     = aFrame.GetRssi();
    mLqi     = aFrame.GetLqi();
#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
    if (aFrame.GetTimeIe() != nullptr)
    {
        mNetworkTimeOffset = aFrame.ComputeNetworkTimeOffset();
        mTimeSyncSeq       = aFrame.ReadTimeSyncSeq();
    }
#endif
#if OPENTHREAD_CONFIG_MULTI_RADIO
    mRadioType = static_cast<uint8_t>(aFrame.GetRadioType());
#endif
}

MeshForwarder::MeshForwarder(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mMessageNextOffset(0)
    , mSendMessage(nullptr)
    , mMeshSource()
    , mMeshDest()
    , mAddMeshHeader(false)
    , mEnabled(false)
    , mTxPaused(false)
    , mSendBusy(false)
#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_MAC_COLLISION_AVOIDANCE_DELAY_ENABLE
    , mDelayNextTx(false)
    , mTxDelayTimer(aInstance)
#endif
    , mScheduleTransmissionTask(aInstance)
#if OPENTHREAD_FTD
    , mIndirectSender(aInstance)
#endif
    , mDataPollSender(aInstance)
{
    mFragTag = Random::NonCrypto::GetUint16();

    ResetCounters();

#if OPENTHREAD_CONFIG_TX_QUEUE_STATISTICS_ENABLE
    mTxQueueStats.Clear();
#endif
}

void MeshForwarder::Start(void)
{
    if (!mEnabled)
    {
        Get<Mac::Mac>().SetRxOnWhenIdle(true);
#if OPENTHREAD_FTD
        mIndirectSender.Start();
#endif

        mEnabled = true;
    }
}

void MeshForwarder::Stop(void)
{
    VerifyOrExit(mEnabled);

    mDataPollSender.StopPolling();
    Get<TimeTicker>().UnregisterReceiver(TimeTicker::kMeshForwarder);
    Get<Mle::DiscoverScanner>().Stop();

    mSendQueue.DequeueAndFreeAll();
    mReassemblyList.DequeueAndFreeAll();

#if OPENTHREAD_FTD
    mIndirectSender.Stop();
    mFwdFrameInfoArray.Clear();
#endif

#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_MAC_COLLISION_AVOIDANCE_DELAY_ENABLE
    mTxDelayTimer.Stop();
    mDelayNextTx = false;
#endif

    mEnabled     = false;
    mSendMessage = nullptr;
    Get<Mac::Mac>().SetRxOnWhenIdle(false);

exit:
    return;
}

void MeshForwarder::PrepareEmptyFrame(Mac::TxFrame &aFrame, const Mac::Address &aMacDest, bool aAckRequest)
{
    Mac::TxFrame::Info frameInfo;

    frameInfo.mAddrs.mSource.SetShort(Get<Mac::Mac>().GetShortAddress());

    if (frameInfo.mAddrs.mSource.IsShortAddrInvalid() || aMacDest.IsExtended())
    {
        frameInfo.mAddrs.mSource.SetExtended(Get<Mac::Mac>().GetExtAddress());
    }

    frameInfo.mAddrs.mDestination = aMacDest;
    frameInfo.mPanIds.SetBothSourceDestination(Get<Mac::Mac>().GetPanId());

    frameInfo.mType          = Mac::Frame::kTypeData;
    frameInfo.mSecurityLevel = Mac::Frame::kSecurityEncMic32;
    frameInfo.mKeyIdMode     = Mac::Frame::kKeyIdMode1;

    PrepareMacHeaders(aFrame, frameInfo, nullptr);

    aFrame.SetAckRequest(aAckRequest);
    aFrame.SetPayloadLength(0);
}

void MeshForwarder::ResumeMessageTransmissions(void)
{
    if (mTxPaused)
    {
        mTxPaused = false;
        mScheduleTransmissionTask.Post();
    }
}

#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_MAC_COLLISION_AVOIDANCE_DELAY_ENABLE
void MeshForwarder::HandleTxDelayTimer(void)
{
    mDelayNextTx = false;
    mScheduleTransmissionTask.Post();
    LogDebg("Tx delay timer expired");
}
#endif

#if OPENTHREAD_CONFIG_DELAY_AWARE_QUEUE_MANAGEMENT_ENABLE

Error MeshForwarder::UpdateEcnOrDrop(Message &aMessage, bool aPreparingToSend)
{
    // This method performs delay-aware active queue management for
    // direct message transmission. It parses the IPv6 header from
    // `aMessage` to determine if message is  ECN-capable. This is
    // then used along with the message's time-in-queue to decide
    // whether to keep the message as is, change the ECN field to
    // mark congestion, or drop the message. If the message is to be
    // dropped, this method clears the direct tx flag on `aMessage`
    // and removes it from the send queue (if no pending indirect tx)
    // and returns `kErrorDrop`. This method returns `kErrorNone`
    // when the message is kept as is or ECN field is updated.

    Error    error         = kErrorNone;
    uint32_t timeInQueue   = TimerMilli::GetNow() - aMessage.GetTimestamp();
    bool     shouldMarkEcn = (timeInQueue >= kTimeInQueueMarkEcn);
    bool     isEcnCapable  = false;

    VerifyOrExit(aMessage.IsDirectTransmission() && (aMessage.GetOffset() == 0));

    if (aMessage.GetType() == Message::kTypeIp6)
    {
        Ip6::Header ip6Header;

        IgnoreError(aMessage.Read(0, ip6Header));

        VerifyOrExit(!Get<ThreadNetif>().HasUnicastAddress(ip6Header.GetSource()));

        isEcnCapable = (ip6Header.GetEcn() != Ip6::kEcnNotCapable);

        if ((shouldMarkEcn && !isEcnCapable) || (timeInQueue >= kTimeInQueueDropMsg))
        {
            ExitNow(error = kErrorDrop);
        }

        if (shouldMarkEcn)
        {
            switch (ip6Header.GetEcn())
            {
            case Ip6::kEcnCapable0:
            case Ip6::kEcnCapable1:
                ip6Header.SetEcn(Ip6::kEcnMarked);
                aMessage.Write(0, ip6Header);
                LogMessage(kMessageMarkEcn, aMessage);
                break;

            case Ip6::kEcnMarked:
            case Ip6::kEcnNotCapable:
                break;
            }
        }
    }
#if OPENTHREAD_FTD
    else if (aMessage.GetType() == Message::kType6lowpan)
    {
        uint16_t               headerLength = 0;
        uint16_t               offset;
        bool                   hasFragmentHeader = false;
        Lowpan::FragmentHeader fragmentHeader;
        Lowpan::MeshHeader     meshHeader;

        IgnoreError(meshHeader.ParseFrom(aMessage, headerLength));

        offset = headerLength;

        if (fragmentHeader.ParseFrom(aMessage, offset, headerLength) == kErrorNone)
        {
            hasFragmentHeader = true;
            offset += headerLength;
        }

        if (!hasFragmentHeader || (fragmentHeader.GetDatagramOffset() == 0))
        {
            Ip6::Ecn ecn = Get<Lowpan::Lowpan>().DecompressEcn(aMessage, offset);

            isEcnCapable = (ecn != Ip6::kEcnNotCapable);

            if ((shouldMarkEcn && !isEcnCapable) || (timeInQueue >= kTimeInQueueDropMsg))
            {
                FwdFrameInfo *entry = FindFwdFrameInfoEntry(meshHeader.GetSource(), fragmentHeader.GetDatagramTag());

                if (entry != nullptr)
                {
                    entry->MarkToDrop();
                    entry->ResetLifetime();
                }

                ExitNow(error = kErrorDrop);
            }

            if (shouldMarkEcn)
            {
                switch (ecn)
                {
                case Ip6::kEcnCapable0:
                case Ip6::kEcnCapable1:
                    Get<Lowpan::Lowpan>().MarkCompressedEcn(aMessage, offset);
                    LogMessage(kMessageMarkEcn, aMessage);
                    break;

                case Ip6::kEcnMarked:
                case Ip6::kEcnNotCapable:
                    break;
                }
            }
        }
        else if (hasFragmentHeader)
        {
            FwdFrameInfo *entry = FindFwdFrameInfoEntry(meshHeader.GetSource(), fragmentHeader.GetDatagramTag());

            VerifyOrExit(entry != nullptr);

            if (entry->ShouldDrop())
            {
                error = kErrorDrop;
            }

            // We can remove the entry if it is the last fragment and
            // only if the message is being prepared to be sent out.
            if (aPreparingToSend && (fragmentHeader.GetDatagramOffset() + aMessage.GetLength() - offset >=
                                     fragmentHeader.GetDatagramSize()))
            {
                mFwdFrameInfoArray.Remove(*entry);
            }
        }
    }
#else
    OT_UNUSED_VARIABLE(aPreparingToSend);
#endif // OPENTHREAD_FTD

exit:
    if (error == kErrorDrop)
    {
#if OPENTHREAD_CONFIG_TX_QUEUE_STATISTICS_ENABLE
        mTxQueueStats.UpdateFor(aMessage);
#endif
        LogMessage(kMessageQueueMgmtDrop, aMessage);
        FinalizeMessageDirectTx(aMessage, kErrorDrop);
        RemoveMessageIfNoPendingTx(aMessage);
    }

    return error;
}

Error MeshForwarder::RemoveAgedMessages(void)
{
    // This method goes through all messages in the send queue and
    // removes all aged messages determined based on the delay-aware
    // active queue management rules. It may also mark ECN on some
    // messages. It returns `kErrorNone` if at least one message was
    // removed, or `kErrorNotFound` if none was removed.

    Error    error = kErrorNotFound;
    Message *nextMessage;

    for (Message *message = mSendQueue.GetHead(); message != nullptr; message = nextMessage)
    {
        nextMessage = message->GetNext();

        // Exclude the current message being sent `mSendMessage`.
        if ((message == mSendMessage) || !message->IsDirectTransmission() || message->GetDoNotEvict())
        {
            continue;
        }

        if (UpdateEcnOrDrop(*message, /* aPreparingToSend */ false) == kErrorDrop)
        {
            error = kErrorNone;
        }
    }

    return error;
}

#endif // OPENTHREAD_CONFIG_DELAY_AWARE_QUEUE_MANAGEMENT_ENABLE

#if (OPENTHREAD_CONFIG_MAX_FRAMES_IN_DIRECT_TX_QUEUE > 0)

bool MeshForwarder::IsDirectTxQueueOverMaxFrameThreshold(void) const
{
    uint16_t frameCount = 0;

    for (const Message &message : mSendQueue)
    {
        if (!message.IsDirectTransmission() || (&message == mSendMessage))
        {
            continue;
        }

        switch (message.GetType())
        {
        case Message::kTypeIp6:
        {
            // If it is an IPv6 message, we estimate the number of
            // fragment frames assuming typical header sizes and lowpan
            // compression. Since this estimate is only used for queue
            // management, we lean towards an under estimate in sense
            // that we may allow few more frames in the tx queue over
            // threshold in some rare cases.
            //
            // The constants below are derived as follows: Typical MAC
            // header (15 bytes) and MAC footer (6 bytes) leave 106
            // bytes for MAC payload. Next fragment header is 5 bytes
            // leaving 96 for next fragment payload. Lowpan compression
            // on average compresses 40 bytes IPv6 header into about 19
            // bytes leaving 87 bytes for the IPv6 payload, so the first
            // fragment can fit 87 + 40 = 127 bytes.

            static constexpr uint16_t kFirstFragmentMaxLength = 127;
            static constexpr uint16_t kNextFragmentSize       = 96;

            uint16_t length = message.GetLength();

            frameCount++;

            if (length > kFirstFragmentMaxLength)
            {
                frameCount += (length - kFirstFragmentMaxLength) / kNextFragmentSize;
            }

            break;
        }

        case Message::kType6lowpan:
        case Message::kTypeMacEmptyData:
            frameCount++;
            break;

        case Message::kTypeSupervision:
        default:
            break;
        }
    }

    return (frameCount > OPENTHREAD_CONFIG_MAX_FRAMES_IN_DIRECT_TX_QUEUE);
}

void MeshForwarder::ApplyDirectTxQueueLimit(Message &aMessage)
{
    VerifyOrExit(aMessage.IsDirectTransmission());
    VerifyOrExit(IsDirectTxQueueOverMaxFrameThreshold());

#if OPENTHREAD_CONFIG_DELAY_AWARE_QUEUE_MANAGEMENT_ENABLE
    {
        bool  originalEvictFlag = aMessage.GetDoNotEvict();
        Error error;

        // We mark the "do not evict" flag on the new `aMessage` so
        // that it will not be removed from `RemoveAgedMessages()`.
        // This protects against the unlikely case where the newly
        // queued `aMessage` may already be aged due to execution
        // being interrupted for a long time between the queuing of
        // the message and the `ApplyDirectTxQueueLimit()` call. We
        // do not want the message to be potentially removed and
        // freed twice.

        aMessage.SetDoNotEvict(true);
        error = RemoveAgedMessages();
        aMessage.SetDoNotEvict(originalEvictFlag);

        if (error == kErrorNone)
        {
            VerifyOrExit(IsDirectTxQueueOverMaxFrameThreshold());
        }
    }
#endif

    LogMessage(kMessageFullQueueDrop, aMessage);
    FinalizeMessageDirectTx(aMessage, kErrorDrop);
    RemoveMessageIfNoPendingTx(aMessage);

exit:
    return;
}

#endif // (OPENTHREAD_CONFIG_MAX_FRAMES_IN_DIRECT_TX_QUEUE > 0)

#if OPENTHREAD_CONFIG_TX_QUEUE_STATISTICS_ENABLE
const uint32_t *MeshForwarder::TxQueueStats::GetHistogram(uint16_t &aNumBins, uint32_t &aBinInterval) const
{
    aNumBins     = kNumHistBins;
    aBinInterval = kHistBinInterval;
    return mHistogram;
}

void MeshForwarder::TxQueueStats::UpdateFor(const Message &aMessage)
{
    uint32_t timeInQueue = TimerMilli::GetNow() - aMessage.GetTimestamp();

    mHistogram[Min<uint32_t>(timeInQueue / kHistBinInterval, kNumHistBins - 1)]++;
    mMaxInterval = Max(mMaxInterval, timeInQueue);
}
#endif

void MeshForwarder::ScheduleTransmissionTask(void)
{
    VerifyOrExit(!mSendBusy && !mTxPaused);

#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_MAC_COLLISION_AVOIDANCE_DELAY_ENABLE
    VerifyOrExit(!mDelayNextTx);
#endif

    mSendMessage = PrepareNextDirectTransmission();
    VerifyOrExit(mSendMessage != nullptr);

    if (mSendMessage->GetOffset() == 0)
    {
        mSendMessage->SetTxSuccess(true);
    }

    Get<Mac::Mac>().RequestDirectFrameTransmission();

exit:
    return;
}

Message *MeshForwarder::PrepareNextDirectTransmission(void)
{
    Message *curMessage, *nextMessage;
    Error    error = kErrorNone;

    for (curMessage = mSendQueue.GetHead(); curMessage; curMessage = nextMessage)
    {
        // We set the `nextMessage` here but it can be updated again
        // after the `switch(message.GetType())` since it may be
        // evicted during message processing (e.g., from the call to
        // `UpdateIp6Route()` due to Address Solicit).

        nextMessage = curMessage->GetNext();

        if (!curMessage->IsDirectTransmission() || curMessage->IsResolvingAddress())
        {
            continue;
        }

#if OPENTHREAD_CONFIG_DELAY_AWARE_QUEUE_MANAGEMENT_ENABLE
        if (UpdateEcnOrDrop(*curMessage, /* aPreparingToSend */ true) == kErrorDrop)
        {
            continue;
        }
#endif
        curMessage->SetDoNotEvict(true);

        switch (curMessage->GetType())
        {
        case Message::kTypeIp6:
            error = UpdateIp6Route(*curMessage);
            break;

#if OPENTHREAD_FTD

        case Message::kType6lowpan:
            error = UpdateMeshRoute(*curMessage);
            break;

#endif

#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
        case Message::kTypeMacEmptyData:
            error = kErrorNone;
            break;
#endif

        default:
            error = kErrorDrop;
            break;
        }

        curMessage->SetDoNotEvict(false);

        // the next message may have been evicted during processing (e.g. due to Address Solicit)
        nextMessage = curMessage->GetNext();

        switch (error)
        {
        case kErrorNone:
            if (!curMessage->IsDirectTransmission())
            {
                // Skip if message is no longer marked for direct transmission.
                // For example, `UpdateIp6Route()` may determine the destination
                // is an ALOC associated with an SED child of this device and
                // mark it for indirect tx to the SED child.
                continue;
            }

#if OPENTHREAD_CONFIG_TX_QUEUE_STATISTICS_ENABLE
            mTxQueueStats.UpdateFor(*curMessage);
#endif
            ExitNow();

#if OPENTHREAD_FTD
        case kErrorAddressQuery:
            curMessage->SetResolvingAddress(true);
            continue;
#endif

        default:
#if OPENTHREAD_CONFIG_TX_QUEUE_STATISTICS_ENABLE
            mTxQueueStats.UpdateFor(*curMessage);
#endif
            LogMessage(kMessageDrop, *curMessage, error);
            FinalizeMessageDirectTx(*curMessage, error);
            RemoveMessageIfNoPendingTx(*curMessage);
            continue;
        }
    }

exit:
    return curMessage;
}

Error MeshForwarder::UpdateIp6Route(Message &aMessage)
{
    Mle::MleRouter &mle   = Get<Mle::MleRouter>();
    Error           error = kErrorNone;
    Ip6::Header     ip6Header;

    mAddMeshHeader = false;

    IgnoreError(aMessage.Read(0, ip6Header));

    VerifyOrExit(!ip6Header.GetSource().IsMulticast(), error = kErrorDrop);

    GetMacSourceAddress(ip6Header.GetSource(), mMacAddrs.mSource);

    if (mle.IsDisabled() || mle.IsDetached())
    {
        if (ip6Header.GetDestination().IsLinkLocalUnicastOrMulticast())
        {
            GetMacDestinationAddress(ip6Header.GetDestination(), mMacAddrs.mDestination);
        }
        else
        {
            error = kErrorDrop;
        }

        ExitNow();
    }

    if (ip6Header.GetDestination().IsMulticast())
    {
        // With the exception of MLE multicasts and any other message
        // with link security disabled, an End Device transmits
        // multicasts, as IEEE 802.15.4 unicasts to its parent.

        if (mle.IsChild() && aMessage.IsLinkSecurityEnabled() && !aMessage.IsSubTypeMle())
        {
            mMacAddrs.mDestination.SetShort(mle.GetParentRloc16());
        }
        else
        {
            mMacAddrs.mDestination.SetShort(Mac::kShortAddrBroadcast);
        }
    }
    else if (ip6Header.GetDestination().IsLinkLocalUnicast())
    {
        GetMacDestinationAddress(ip6Header.GetDestination(), mMacAddrs.mDestination);
    }
    else if (mle.IsMinimalEndDevice())
    {
        mMacAddrs.mDestination.SetShort(mle.GetParentRloc16());
    }
    else
    {
#if OPENTHREAD_FTD
        error = UpdateIp6RouteFtd(ip6Header, aMessage);
#else
        OT_ASSERT(false);
#endif
    }

exit:
    return error;
}

bool MeshForwarder::GetRxOnWhenIdle(void) const { return Get<Mac::Mac>().GetRxOnWhenIdle(); }

void MeshForwarder::SetRxOnWhenIdle(bool aRxOnWhenIdle)
{
    Get<Mac::Mac>().SetRxOnWhenIdle(aRxOnWhenIdle);

    if (aRxOnWhenIdle)
    {
        mDataPollSender.StopPolling();
        Get<SupervisionListener>().Stop();
    }
    else
    {
        mDataPollSender.StartPolling();
        Get<SupervisionListener>().Start();
    }
}

void MeshForwarder::GetMacSourceAddress(const Ip6::Address &aIp6Addr, Mac::Address &aMacAddr)
{
    aIp6Addr.GetIid().ConvertToMacAddress(aMacAddr);

    if (aMacAddr.GetExtended() != Get<Mac::Mac>().GetExtAddress())
    {
        aMacAddr.SetShort(Get<Mac::Mac>().GetShortAddress());
    }
}

void MeshForwarder::GetMacDestinationAddress(const Ip6::Address &aIp6Addr, Mac::Address &aMacAddr)
{
    if (aIp6Addr.IsMulticast())
    {
        aMacAddr.SetShort(Mac::kShortAddrBroadcast);
    }
    else if (Get<Mle::MleRouter>().IsRoutingLocator(aIp6Addr))
    {
        aMacAddr.SetShort(aIp6Addr.GetIid().GetLocator());
    }
    else
    {
        aIp6Addr.GetIid().ConvertToMacAddress(aMacAddr);
    }
}

Mac::TxFrame *MeshForwarder::HandleFrameRequest(Mac::TxFrames &aTxFrames)
{
    Mac::TxFrame *frame         = nullptr;
    bool          addFragHeader = false;

    VerifyOrExit(mEnabled && (mSendMessage != nullptr));

#if OPENTHREAD_CONFIG_MULTI_RADIO
    frame = &Get<RadioSelector>().SelectRadio(*mSendMessage, mMacAddrs.mDestination, aTxFrames);

    // If multi-radio link is supported, when sending frame with link
    // security enabled, Fragment Header is always included (even if
    // the message is small and does not require 6LoWPAN fragmentation).
    // This allows the Fragment Header's tag to be used to detect and
    // suppress duplicate received frames over different radio links.

    if (mSendMessage->IsLinkSecurityEnabled())
    {
        addFragHeader = true;
    }
#else
    frame = &aTxFrames.GetTxFrame();
#endif

    mSendBusy = true;

    switch (mSendMessage->GetType())
    {
    case Message::kTypeIp6:
        if (mSendMessage->IsMleCommand(Mle::kCommandDiscoveryRequest))
        {
            frame = Get<Mle::DiscoverScanner>().PrepareDiscoveryRequestFrame(*frame);
            VerifyOrExit(frame != nullptr);
        }
#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
        else if (Get<Mac::Mac>().IsCslEnabled() && mSendMessage->IsSubTypeMle())
        {
            mSendMessage->SetLinkSecurityEnabled(true);
        }
#endif
        mMessageNextOffset =
            PrepareDataFrame(*frame, *mSendMessage, mMacAddrs, mAddMeshHeader, mMeshSource, mMeshDest, addFragHeader);

        if (mSendMessage->IsMleCommand(Mle::kCommandChildIdRequest) && mSendMessage->IsLinkSecurityEnabled())
        {
            LogNote("Child ID Request requires fragmentation, aborting tx");
            mMessageNextOffset = mSendMessage->GetLength();
            ExitNow(frame = nullptr);
        }
        break;

#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
    case Message::kTypeMacEmptyData:
    {
        Mac::Address macDestAddr;

        macDestAddr.SetShort(Get<Mle::MleRouter>().GetParent().GetRloc16());
        PrepareEmptyFrame(*frame, macDestAddr, /* aAckRequest */ true);
    }
    break;
#endif

#if OPENTHREAD_FTD

    case Message::kType6lowpan:
        SendMesh(*mSendMessage, *frame);
        break;

    case Message::kTypeSupervision:
        // A direct supervision message is possible in the case where
        // a sleepy child switches its mode (becomes non-sleepy) while
        // there is a pending indirect supervision message in the send
        // queue for it. The message would be then converted to a
        // direct tx.

        OT_FALL_THROUGH;
#endif

    default:
        mMessageNextOffset = mSendMessage->GetLength();
        ExitNow(frame = nullptr);
    }

    frame->SetIsARetransmission(false);

exit:
    return frame;
}

void MeshForwarder::PrepareMacHeaders(Mac::TxFrame &aTxFrame, Mac::TxFrame::Info &aTxFrameInfo, const Message *aMessage)
{
    aTxFrameInfo.mVersion = Mac::Frame::kVersion2006;

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Determine Header IE entries

#if OPENTHREAD_CONFIG_MAC_HEADER_IE_SUPPORT

#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
    if ((aMessage != nullptr) && aMessage->IsTimeSync())
    {
        aTxFrameInfo.mAppendTimeIe = true;
        aTxFrameInfo.mVersion      = Mac::Frame::kVersion2015;
    }
#endif
#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
    if (Get<Mac::Mac>().IsCslEnabled() &&
        !(aMessage != nullptr && aMessage->IsMleCommand(Mle::kCommandDiscoveryRequest)))
    {
        aTxFrameInfo.mAppendCslIe = true;
        aTxFrameInfo.mVersion     = Mac::Frame::kVersion2015;
    }
#endif

    aTxFrameInfo.mEmptyPayload = (aMessage == nullptr) || (aMessage->GetLength() == 0);

#endif // OPENTHREAD_CONFIG_MAC_HEADER_IE_SUPPORT

#if (OPENTHREAD_FTD && OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE) || \
    OPENTHREAD_CONFIG_MLE_LINK_METRICS_INITIATOR_ENABLE

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Determine frame version

    if (aTxFrameInfo.mVersion == Mac::Frame::kVersion2006)
    {
        const Neighbor *neighbor = Get<NeighborTable>().FindNeighbor(aTxFrameInfo.mAddrs.mDestination);

#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE
        if ((neighbor != nullptr) && Get<ChildTable>().Contains(*neighbor) &&
            static_cast<const Child *>(neighbor)->IsCslSynchronized())
        {
            aTxFrameInfo.mVersion = Mac::Frame::kVersion2015;
        }
#endif
#if OPENTHREAD_CONFIG_MLE_LINK_METRICS_INITIATOR_ENABLE
        if ((neighbor != nullptr) && neighbor->IsEnhAckProbingActive())
        {
            aTxFrameInfo.mVersion = Mac::Frame::kVersion2015;
        }
#endif
    }

#endif // OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE || OPENTHREAD_CONFIG_MLE_LINK_METRICS_INITIATOR_ENABLE

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Prepare MAC headers

    aTxFrameInfo.PrepareHeadersIn(aTxFrame);

    OT_UNUSED_VARIABLE(aMessage);
}

// This method constructs a MAC data from from a given IPv6 message.
//
// This method handles generation of MAC header, mesh header (if
// requested), lowpan compression of IPv6 header, lowpan fragmentation
// header (if message requires fragmentation or if it is explicitly
// requested by setting `aAddFragHeader` to `true`) It uses the
// message offset to construct next fragments. This method enables
// link security when message is MLE type and requires fragmentation.
// It returns the next offset into the message after the prepared
// frame.
//
uint16_t MeshForwarder::PrepareDataFrame(Mac::TxFrame         &aFrame,
                                         Message              &aMessage,
                                         const Mac::Addresses &aMacAddrs,
                                         bool                  aAddMeshHeader,
                                         uint16_t              aMeshSource,
                                         uint16_t              aMeshDest,
                                         bool                  aAddFragHeader)
{
    Mac::TxFrame::Info frameInfo;
    uint16_t           payloadLength;
    uint16_t           origMsgOffset;
    uint16_t           nextOffset;
    FrameBuilder       frameBuilder;

start:
    frameInfo.Clear();

    if (aMessage.IsLinkSecurityEnabled())
    {
        frameInfo.mSecurityLevel = Mac::Frame::kSecurityEncMic32;

        if (aMessage.GetSubType() == Message::kSubTypeJoinerEntrust)
        {
            frameInfo.mKeyIdMode = Mac::Frame::kKeyIdMode0;
        }
        else if (aMessage.IsMleCommand(Mle::kCommandAnnounce))
        {
            frameInfo.mKeyIdMode = Mac::Frame::kKeyIdMode2;
        }
        else
        {
            frameInfo.mKeyIdMode = Mac::Frame::kKeyIdMode1;
        }
    }

    frameInfo.mPanIds.SetBothSourceDestination(Get<Mac::Mac>().GetPanId());

    if (aMessage.IsSubTypeMle())
    {
        switch (aMessage.GetMleCommand())
        {
        case Mle::kCommandAnnounce:
            aFrame.SetChannel(aMessage.GetChannel());
            aFrame.SetRxChannelAfterTxDone(Get<Mac::Mac>().GetPanChannel());
            frameInfo.mPanIds.SetDestination(Mac::kPanIdBroadcast);
            break;

        case Mle::kCommandDiscoveryRequest:
        case Mle::kCommandDiscoveryResponse:
            frameInfo.mPanIds.SetDestination(aMessage.GetPanId());
            break;

        default:
            break;
        }
    }

    frameInfo.mType  = Mac::Frame::kTypeData;
    frameInfo.mAddrs = aMacAddrs;

    PrepareMacHeaders(aFrame, frameInfo, &aMessage);

    frameBuilder.Init(aFrame.GetPayload(), aFrame.GetMaxPayloadLength());

#if OPENTHREAD_FTD

    // Initialize Mesh header
    if (aAddMeshHeader)
    {
        Lowpan::MeshHeader meshHeader;
        uint16_t           maxPayloadLength;

        // Mesh Header frames are forwarded by routers over multiple
        // hops to reach a final destination. The forwarding path can
        // have routers supporting different radio links with varying
        // MTU sizes. Since the originator of the frame does not know the
        // path and the MTU sizes of supported radio links by the routers
        // in the path, we limit the max payload length of a Mesh Header
        // frame to a fixed minimum value (derived from 15.4 radio)
        // ensuring it can be handled by any radio link.
        //
        // Maximum payload length is calculated by subtracting the frame
        // header and footer lengths from the MTU size. The footer
        // length is derived by removing the `aFrame.GetFcsSize()` and
        // then adding the fixed `kMeshHeaderFrameFcsSize` instead
        // (updating the FCS size in the calculation of footer length).

        maxPayloadLength = kMeshHeaderFrameMtu - aFrame.GetHeaderLength() -
                           (aFrame.GetFooterLength() - aFrame.GetFcsSize() + kMeshHeaderFrameFcsSize);

        frameBuilder.Init(aFrame.GetPayload(), maxPayloadLength);

        meshHeader.Init(aMeshSource, aMeshDest, kMeshHeaderHopsLeft);

        IgnoreError(meshHeader.AppendTo(frameBuilder));
    }

#endif // OPENTHREAD_FTD

    // While performing lowpan compression, the message offset may be
    // changed to skip over the compressed IPv6 headers, we save the
    // original offset and set it back on `aMessage` at the end
    // before returning.

    origMsgOffset = aMessage.GetOffset();

    // Compress IPv6 Header
    if (aMessage.GetOffset() == 0)
    {
        uint16_t       fragHeaderOffset;
        uint16_t       maxFrameLength;
        Mac::Addresses macAddrs;

        // Before performing lowpan header compression, we reduce the
        // max length on `frameBuilder` to reserve bytes for first
        // fragment header. This ensures that lowpan compression will
        // leave room for a first fragment header. After the lowpan
        // header compression is done, we reclaim the reserved bytes
        // by setting the max length back to its original value.

        fragHeaderOffset = frameBuilder.GetLength();
        maxFrameLength   = frameBuilder.GetMaxLength();
        frameBuilder.SetMaxLength(maxFrameLength - sizeof(Lowpan::FragmentHeader::FirstFrag));

        if (aAddMeshHeader)
        {
            macAddrs.mSource.SetShort(aMeshSource);
            macAddrs.mDestination.SetShort(aMeshDest);
        }
        else
        {
            macAddrs = aMacAddrs;
        }

        SuccessOrAssert(Get<Lowpan::Lowpan>().Compress(aMessage, macAddrs, frameBuilder));

        frameBuilder.SetMaxLength(maxFrameLength);

        payloadLength = aMessage.GetLength() - aMessage.GetOffset();

        if (aAddFragHeader || (payloadLength > frameBuilder.GetRemainingLength()))
        {
            Lowpan::FragmentHeader::FirstFrag firstFragHeader;

            if ((!aMessage.IsLinkSecurityEnabled()) && aMessage.IsSubTypeMle())
            {
                // MLE messages that require fragmentation MUST use
                // link-layer security. We enable security and try
                // constructing the frame again.

                aMessage.SetOffset(0);
                aMessage.SetLinkSecurityEnabled(true);
                goto start;
            }

            // Insert Fragment header
            if (aMessage.GetDatagramTag() == 0)
            {
                // Avoid using datagram tag value 0, which indicates the tag has not been set
                if (mFragTag == 0)
                {
                    mFragTag++;
                }

                aMessage.SetDatagramTag(mFragTag++);
            }

            firstFragHeader.Init(aMessage.GetLength(), static_cast<uint16_t>(aMessage.GetDatagramTag()));
            SuccessOrAssert(frameBuilder.Insert(fragHeaderOffset, firstFragHeader));
        }
    }
    else
    {
        Lowpan::FragmentHeader::NextFrag nextFragHeader;

        nextFragHeader.Init(aMessage.GetLength(), static_cast<uint16_t>(aMessage.GetDatagramTag()),
                            aMessage.GetOffset());
        SuccessOrAssert(frameBuilder.Append(nextFragHeader));

        payloadLength = aMessage.GetLength() - aMessage.GetOffset();
    }

    if (payloadLength > frameBuilder.GetRemainingLength())
    {
        payloadLength = (frameBuilder.GetRemainingLength() & ~0x7);
    }

    // Copy IPv6 Payload
    SuccessOrAssert(frameBuilder.AppendBytesFromMessage(aMessage, aMessage.GetOffset(), payloadLength));
    aFrame.SetPayloadLength(frameBuilder.GetLength());

    nextOffset = aMessage.GetOffset() + payloadLength;

    if (nextOffset < aMessage.GetLength())
    {
        aFrame.SetFramePending(true);
#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
        aMessage.SetTimeSync(false);
#endif
    }

    aMessage.SetOffset(origMsgOffset);

    return nextOffset;
}

uint16_t MeshForwarder::PrepareDataFrameWithNoMeshHeader(Mac::TxFrame         &aFrame,
                                                         Message              &aMessage,
                                                         const Mac::Addresses &aMacAddrs)
{
    return PrepareDataFrame(aFrame, aMessage, aMacAddrs, /* aAddMeshHeader */ false, /* aMeshSource */ 0xffff,
                            /* aMeshDest */ 0xffff, /* aAddFragHeader */ false);
}

Neighbor *MeshForwarder::UpdateNeighborOnSentFrame(Mac::TxFrame       &aFrame,
                                                   Error               aError,
                                                   const Mac::Address &aMacDest,
                                                   bool                aIsDataPoll)
{
    OT_UNUSED_VARIABLE(aIsDataPoll);

    Neighbor *neighbor  = nullptr;
    uint8_t   failLimit = kFailedRouterTransmissions;

    VerifyOrExit(mEnabled);

    neighbor = Get<NeighborTable>().FindNeighbor(aMacDest);
    VerifyOrExit(neighbor != nullptr);

    VerifyOrExit(aFrame.GetAckRequest());

#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
    // TREL radio link uses deferred ack model. We ignore
    // `SendDone` event from `Mac` layer with success status and
    // wait for deferred ack callback instead.
#if OPENTHREAD_CONFIG_MULTI_RADIO
    if (aFrame.GetRadioType() == Mac::kRadioTypeTrel)
#endif
    {
        VerifyOrExit(aError != kErrorNone);
    }
#endif // OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE

#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
    if (aFrame.HasCslIe() && aIsDataPoll)
    {
        failLimit = kFailedCslDataPollTransmissions;
    }
#endif

    UpdateNeighborLinkFailures(*neighbor, aError, /* aAllowNeighborRemove */ true, failLimit);

exit:
    return neighbor;
}

void MeshForwarder::UpdateNeighborLinkFailures(Neighbor &aNeighbor,
                                               Error     aError,
                                               bool      aAllowNeighborRemove,
                                               uint8_t   aFailLimit)
{
    // Update neighbor `LinkFailures` counter on ack error.

    if (aError == kErrorNone)
    {
        aNeighbor.ResetLinkFailures();
    }
    else if (aError == kErrorNoAck)
    {
        aNeighbor.IncrementLinkFailures();

        if (aAllowNeighborRemove && (Mle::IsRouterRloc16(aNeighbor.GetRloc16())) &&
            (aNeighbor.GetLinkFailures() >= aFailLimit))
        {
#if OPENTHREAD_FTD
            Get<Mle::MleRouter>().RemoveRouterLink(static_cast<Router &>(aNeighbor));
#else
            IgnoreError(Get<Mle::Mle>().BecomeDetached());
#endif
        }
    }
}

#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
void MeshForwarder::HandleDeferredAck(Neighbor &aNeighbor, Error aError)
{
    bool allowNeighborRemove = true;

    VerifyOrExit(mEnabled);

    if (aError == kErrorNoAck)
    {
        LogInfo("Deferred ack timeout on trel for neighbor %s rloc16:0x%04x",
                aNeighbor.GetExtAddress().ToString().AsCString(), aNeighbor.GetRloc16());
    }

#if OPENTHREAD_CONFIG_MULTI_RADIO
    // In multi radio mode, `RadioSelector` will update the neighbor's
    // link failure counter and removes the neighbor if required.
    Get<RadioSelector>().UpdateOnDeferredAck(aNeighbor, aError, allowNeighborRemove);
#else
    UpdateNeighborLinkFailures(aNeighbor, aError, allowNeighborRemove, kFailedRouterTransmissions);
#endif

exit:
    return;
}
#endif // #if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE

void MeshForwarder::HandleSentFrame(Mac::TxFrame &aFrame, Error aError)
{
    Neighbor    *neighbor = nullptr;
    Mac::Address macDest;

    OT_ASSERT((aError == kErrorNone) || (aError == kErrorChannelAccessFailure) || (aError == kErrorAbort) ||
              (aError == kErrorNoAck));

    mSendBusy = false;

    VerifyOrExit(mEnabled);

#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_MAC_COLLISION_AVOIDANCE_DELAY_ENABLE
    if (mDelayNextTx && (aError == kErrorNone))
    {
        mTxDelayTimer.Start(kTxDelayInterval);
        LogDebg("Start tx delay timer for %lu msec", ToUlong(kTxDelayInterval));
    }
    else
    {
        mDelayNextTx = false;
    }
#endif

    if (!aFrame.IsEmpty())
    {
        IgnoreError(aFrame.GetDstAddr(macDest));
        neighbor = UpdateNeighborOnSentFrame(aFrame, aError, macDest, /* aIsDataPoll */ false);
    }

    UpdateSendMessage(aError, macDest, neighbor);

exit:
    return;
}

void MeshForwarder::UpdateSendMessage(Error aFrameTxError, Mac::Address &aMacDest, Neighbor *aNeighbor)
{
    Error txError = aFrameTxError;

    VerifyOrExit(mSendMessage != nullptr);

    OT_ASSERT(mSendMessage->IsDirectTransmission());

    if (aFrameTxError != kErrorNone)
    {
        // If the transmission of any fragment frame fails,
        // the overall message transmission is considered
        // as failed

        mSendMessage->SetTxSuccess(false);

#if OPENTHREAD_CONFIG_DROP_MESSAGE_ON_FRAGMENT_TX_FAILURE

        // We set the NextOffset to end of message to avoid sending
        // any remaining fragments in the message.

        mMessageNextOffset = mSendMessage->GetLength();
#endif
    }

    if (mMessageNextOffset < mSendMessage->GetLength())
    {
        mSendMessage->SetOffset(mMessageNextOffset);
        ExitNow();
    }

    txError = aFrameTxError;

    if (aNeighbor != nullptr)
    {
        aNeighbor->GetLinkInfo().AddMessageTxStatus(mSendMessage->GetTxSuccess());
    }

#if !OPENTHREAD_CONFIG_DROP_MESSAGE_ON_FRAGMENT_TX_FAILURE

    // When `CONFIG_DROP_MESSAGE_ON_FRAGMENT_TX_FAILURE` is
    // disabled, all fragment frames of a larger message are
    // sent even if the transmission of an earlier fragment fail.
    // Note that `GetTxSuccess() tracks the tx success of the
    // entire message, while `aFrameTxError` represents the error
    // status of the last fragment frame transmission.

    if (!mSendMessage->GetTxSuccess() && (txError == kErrorNone))
    {
        txError = kErrorFailed;
    }
#endif

#if OPENTHREAD_CONFIG_HISTORY_TRACKER_ENABLE
    Get<Utils::HistoryTracker>().RecordTxMessage(*mSendMessage, aMacDest);
#endif

    LogMessage(kMessageTransmit, *mSendMessage, txError, &aMacDest);
    FinalizeMessageDirectTx(*mSendMessage, txError);
    RemoveMessageIfNoPendingTx(*mSendMessage);

exit:
    mScheduleTransmissionTask.Post();
}

void MeshForwarder::FinalizeMessageDirectTx(Message &aMessage, Error aError)
{
    // Finalizes the direct transmission of `aMessage`. This can be
    // triggered by successful delivery (all fragments reaching the
    // destination), failure of any fragment, queue management
    // dropping the message, or eviction of message to accommodate
    // higher priority messages.

    VerifyOrExit(aMessage.IsDirectTransmission());

    aMessage.ClearDirectTransmission();
    aMessage.SetOffset(0);

    if (aError != kErrorNone)
    {
        aMessage.SetTxSuccess(false);
    }

    if (aMessage.GetType() == Message::kTypeIp6)
    {
        aMessage.GetTxSuccess() ? mIpCounters.mTxSuccess++ : mIpCounters.mTxFailure++;
    }

    if (aMessage.IsMleCommand(Mle::kCommandDiscoveryRequest))
    {
        // Note that `HandleDiscoveryRequestFrameTxDone()` may update
        // `aMessage` and mark it again for direct transmission.
        Get<Mle::DiscoverScanner>().HandleDiscoveryRequestFrameTxDone(aMessage, aError);
    }
    else if (aMessage.IsMleCommand(Mle::kCommandChildIdRequest))
    {
        Get<Mle::Mle>().HandleChildIdRequestTxDone(aMessage);
    }

exit:
    return;
}

void MeshForwarder::FinalizeAndRemoveMessage(Message &aMessage, Error aError, MessageAction aAction)
{
    LogMessage(aAction, aMessage, aError);

#if OPENTHREAD_FTD
    FinalizeMessageIndirectTxs(aMessage);
#endif

    FinalizeMessageDirectTx(aMessage, aError);
    RemoveMessageIfNoPendingTx(aMessage);
}

bool MeshForwarder::RemoveMessageIfNoPendingTx(Message &aMessage)
{
    bool didRemove = false;

#if OPENTHREAD_FTD
    VerifyOrExit(!aMessage.IsDirectTransmission() && aMessage.GetIndirectTxChildMask().IsEmpty());
#else
    VerifyOrExit(!aMessage.IsDirectTransmission());
#endif

    if (mSendMessage == &aMessage)
    {
        mSendMessage       = nullptr;
        mMessageNextOffset = 0;
    }

    mSendQueue.DequeueAndFree(aMessage);
    didRemove = true;

exit:
    return didRemove;
}

Error MeshForwarder::RxInfo::ParseIp6Headers(void)
{
    Error error = kErrorNone;

    VerifyOrExit(!mParsedIp6Headers);
    SuccessOrExit(error = mIp6Headers.DecompressFrom(mFrameData, mMacAddrs, GetInstance()));
    mParsedIp6Headers = true;

exit:
    return error;
}

void MeshForwarder::HandleReceivedFrame(Mac::RxFrame &aFrame)
{
    Error  error = kErrorNone;
    RxInfo rxInfo(GetInstance());

    VerifyOrExit(mEnabled, error = kErrorInvalidState);

    rxInfo.mFrameData.Init(aFrame.GetPayload(), aFrame.GetPayloadLength());

    SuccessOrExit(error = aFrame.GetSrcAddr(rxInfo.mMacAddrs.mSource));
    SuccessOrExit(error = aFrame.GetDstAddr(rxInfo.mMacAddrs.mDestination));

    rxInfo.mLinkInfo.SetFrom(aFrame);

    Get<SupervisionListener>().UpdateOnReceive(rxInfo.mMacAddrs.mSource, rxInfo.IsLinkSecurityEnabled());

    switch (aFrame.GetType())
    {
    case Mac::Frame::kTypeData:
        if (Lowpan::MeshHeader::IsMeshHeader(rxInfo.mFrameData))
        {
#if OPENTHREAD_FTD
            HandleMesh(rxInfo);
#endif
        }
        else if (Lowpan::FragmentHeader::IsFragmentHeader(rxInfo.mFrameData))
        {
            HandleFragment(rxInfo);
        }
        else if (Lowpan::Lowpan::IsLowpanHc(rxInfo.mFrameData))
        {
            HandleLowpanHc(rxInfo);
        }
        else
        {
            VerifyOrExit(rxInfo.mFrameData.GetLength() == 0, error = kErrorNotLowpanDataFrame);

            LogFrame("Received empty payload frame", aFrame, kErrorNone);
        }

        break;

    case Mac::Frame::kTypeBeacon:
        break;

    default:
        error = kErrorDrop;
        break;
    }

exit:

    if (error != kErrorNone)
    {
        LogFrame("Dropping rx frame", aFrame, error);
    }
}

void MeshForwarder::HandleFragment(RxInfo &aRxInfo)
{
    Error                  error = kErrorNone;
    Lowpan::FragmentHeader fragmentHeader;
    Message               *message = nullptr;

    SuccessOrExit(error = fragmentHeader.ParseFrom(aRxInfo.mFrameData));

#if OPENTHREAD_CONFIG_MULTI_RADIO

    if (aRxInfo.IsLinkSecurityEnabled())
    {
        Neighbor *neighbor =
            Get<NeighborTable>().FindNeighbor(aRxInfo.GetSrcAddr(), Neighbor::kInStateAnyExceptInvalid);

        if ((neighbor != nullptr) && (fragmentHeader.GetDatagramOffset() == 0))
        {
            uint16_t tag = fragmentHeader.GetDatagramTag();

            if (neighbor->IsLastRxFragmentTagSet())
            {
                VerifyOrExit(!neighbor->IsLastRxFragmentTagAfter(tag), error = kErrorDuplicated);
            }

            neighbor->SetLastRxFragmentTag(tag);
        }

        // Duplication suppression for a "next fragment" is handled
        // by the code below where the the datagram offset is
        // checked against the offset of the corresponding message
        // (same datagram tag and size) in Reassembly List. Note
        // that if there is no matching message in the Reassembly
        // List (e.g., in case the message is already fully
        // assembled) the received "next fragment" frame would be
        // dropped.
    }

#endif // OPENTHREAD_CONFIG_MULTI_RADIO

    if (fragmentHeader.GetDatagramOffset() == 0)
    {
        uint16_t datagramSize = fragmentHeader.GetDatagramSize();

#if OPENTHREAD_FTD
        UpdateRoutes(aRxInfo);
#endif

        SuccessOrExit(error = FrameToMessage(aRxInfo, datagramSize, message));

        VerifyOrExit(datagramSize >= message->GetLength(), error = kErrorParse);
        SuccessOrExit(error = message->SetLength(datagramSize));

        message->SetDatagramTag(fragmentHeader.GetDatagramTag());
        message->SetTimestampToNow();
        message->UpdateLinkInfoFrom(aRxInfo.mLinkInfo);

        VerifyOrExit(Get<Ip6::Filter>().Accept(*message), error = kErrorDrop);

#if OPENTHREAD_FTD
        SendIcmpErrorIfDstUnreach(*message, aRxInfo.mMacAddrs);
#endif

        // Allow re-assembly of only one message at a time on a SED by clearing
        // any remaining fragments in reassembly list upon receiving of a new
        // (secure) first fragment.

        if (!GetRxOnWhenIdle() && message->IsLinkSecurityEnabled())
        {
            ClearReassemblyList();
        }

        mReassemblyList.Enqueue(*message);

        Get<TimeTicker>().RegisterReceiver(TimeTicker::kMeshForwarder);
    }
    else // Received frame is a "next fragment".
    {
        for (Message &msg : mReassemblyList)
        {
            // Security Check: only consider reassembly buffers that had the same Security Enabled setting.
            if (msg.GetLength() == fragmentHeader.GetDatagramSize() &&
                msg.GetDatagramTag() == fragmentHeader.GetDatagramTag() &&
                msg.GetOffset() == fragmentHeader.GetDatagramOffset() &&
                msg.GetOffset() + aRxInfo.mFrameData.GetLength() <= fragmentHeader.GetDatagramSize() &&
                msg.IsLinkSecurityEnabled() == aRxInfo.IsLinkSecurityEnabled())
            {
                message = &msg;
                break;
            }
        }

        // For a sleepy-end-device, if we receive a new (secure) next fragment
        // with a non-matching fragmentation offset or tag, it indicates that
        // we have either missed a fragment, or the parent has moved to a new
        // message with a new tag. In either case, we can safely clear any
        // remaining fragments stored in the reassembly list.

        if (!GetRxOnWhenIdle() && (message == nullptr) && aRxInfo.IsLinkSecurityEnabled())
        {
            ClearReassemblyList();
        }

        VerifyOrExit(message != nullptr, error = kErrorDrop);

        message->WriteData(message->GetOffset(), aRxInfo.mFrameData);
        message->MoveOffset(aRxInfo.mFrameData.GetLength());
        message->AddRss(aRxInfo.mLinkInfo.GetRss());
        message->AddLqi(aRxInfo.mLinkInfo.GetLqi());
        message->SetTimestampToNow();
    }

exit:

    if (error == kErrorNone)
    {
        if (message->GetOffset() >= message->GetLength())
        {
            mReassemblyList.Dequeue(*message);
            IgnoreError(HandleDatagram(*message, aRxInfo.GetSrcAddr()));
        }
    }
    else
    {
        LogFragmentFrameDrop(error, aRxInfo, fragmentHeader);
        FreeMessage(message);
    }
}

void MeshForwarder::ClearReassemblyList(void)
{
    for (Message &message : mReassemblyList)
    {
        LogMessage(kMessageReassemblyDrop, message, kErrorNoFrameReceived);

        if (message.GetType() == Message::kTypeIp6)
        {
            mIpCounters.mRxFailure++;
        }

        mReassemblyList.DequeueAndFree(message);
    }
}

void MeshForwarder::HandleTimeTick(void)
{
    bool continueRxingTicks = false;

#if OPENTHREAD_FTD
    continueRxingTicks = UpdateFwdFrameInfoArrayOnTimeTick();
#endif

    continueRxingTicks = UpdateReassemblyList() || continueRxingTicks;

    if (!continueRxingTicks)
    {
        Get<TimeTicker>().UnregisterReceiver(TimeTicker::kMeshForwarder);
    }
}

bool MeshForwarder::UpdateReassemblyList(void)
{
    TimeMilli now = TimerMilli::GetNow();

    for (Message &message : mReassemblyList)
    {
        if (now - message.GetTimestamp() >= TimeMilli::SecToMsec(kReassemblyTimeout))
        {
            LogMessage(kMessageReassemblyDrop, message, kErrorReassemblyTimeout);

            if (message.GetType() == Message::kTypeIp6)
            {
                mIpCounters.mRxFailure++;
            }

            mReassemblyList.DequeueAndFree(message);
        }
    }

    return mReassemblyList.GetHead() != nullptr;
}

Error MeshForwarder::FrameToMessage(RxInfo &aRxInfo, uint16_t aDatagramSize, Message *&aMessage)
{
    Error             error     = kErrorNone;
    FrameData         frameData = aRxInfo.mFrameData;
    Message::Priority priority;

    SuccessOrExit(error = GetFramePriority(aRxInfo, priority));

    aMessage = Get<MessagePool>().Allocate(Message::kTypeIp6, /* aReserveHeader */ 0, Message::Settings(priority));
    VerifyOrExit(aMessage, error = kErrorNoBufs);

    SuccessOrExit(error = Get<Lowpan::Lowpan>().Decompress(*aMessage, aRxInfo.mMacAddrs, frameData, aDatagramSize));

    SuccessOrExit(error = aMessage->AppendData(frameData));
    aMessage->MoveOffset(frameData.GetLength());

exit:
    return error;
}

void MeshForwarder::HandleLowpanHc(RxInfo &aRxInfo)
{
    Error    error   = kErrorNone;
    Message *message = nullptr;

#if OPENTHREAD_FTD
    UpdateRoutes(aRxInfo);
#endif

    SuccessOrExit(error = FrameToMessage(aRxInfo, 0, message));

    message->UpdateLinkInfoFrom(aRxInfo.mLinkInfo);

    VerifyOrExit(Get<Ip6::Filter>().Accept(*message), error = kErrorDrop);

#if OPENTHREAD_FTD
    SendIcmpErrorIfDstUnreach(*message, aRxInfo.mMacAddrs);
#endif

exit:

    if (error == kErrorNone)
    {
        IgnoreError(HandleDatagram(*message, aRxInfo.GetSrcAddr()));
    }
    else
    {
        LogLowpanHcFrameDrop(error, aRxInfo);
        FreeMessage(message);
    }
}

Error MeshForwarder::HandleDatagram(Message &aMessage, const Mac::Address &aMacSource)
{
#if OPENTHREAD_CONFIG_HISTORY_TRACKER_ENABLE
    Get<Utils::HistoryTracker>().RecordRxMessage(aMessage, aMacSource);
#endif

    LogMessage(kMessageReceive, aMessage, kErrorNone, &aMacSource);

    if (aMessage.GetType() == Message::kTypeIp6)
    {
        mIpCounters.mRxSuccess++;
    }

    aMessage.SetLoopbackToHostAllowed(true);
    aMessage.SetOrigin(Message::kOriginThreadNetif);

    return Get<Ip6::Ip6>().HandleDatagram(OwnedPtr<Message>(&aMessage));
}

Error MeshForwarder::GetFramePriority(RxInfo &aRxInfo, Message::Priority &aPriority)
{
    Error error = kErrorNone;

    SuccessOrExit(error = aRxInfo.ParseIp6Headers());

    aPriority = Ip6::Ip6::DscpToPriority(aRxInfo.mIp6Headers.GetIp6Header().GetDscp());

    // Only ICMPv6 error messages are prioritized.
    if (aRxInfo.mIp6Headers.IsIcmp6() && aRxInfo.mIp6Headers.GetIcmpHeader().IsError())
    {
        aPriority = Message::kPriorityNet;
    }

    if (aRxInfo.mIp6Headers.IsUdp())
    {
        uint16_t destPort = aRxInfo.mIp6Headers.GetUdpHeader().GetDestinationPort();

        if (destPort == Mle::kUdpPort)
        {
            aPriority = Message::kPriorityNet;
        }
        else if (Get<Tmf::Agent>().IsTmfMessage(aRxInfo.mIp6Headers.GetSourceAddress(),
                                                aRxInfo.mIp6Headers.GetDestinationAddress(), destPort))
        {
            aPriority = Tmf::Agent::DscpToPriority(aRxInfo.mIp6Headers.GetIp6Header().GetDscp());
        }
    }

exit:
    return error;
}

#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
Error MeshForwarder::SendEmptyMessage(void)
{
    Error             error = kErrorNone;
    OwnedPtr<Message> messagePtr;

    VerifyOrExit(mEnabled && !Get<Mac::Mac>().GetRxOnWhenIdle() &&
                     Get<Mle::MleRouter>().GetParent().IsStateValidOrRestoring(),
                 error = kErrorInvalidState);

    messagePtr.Reset(Get<MessagePool>().Allocate(Message::kTypeMacEmptyData));
    VerifyOrExit(messagePtr != nullptr, error = kErrorNoBufs);

    SendMessage(messagePtr.PassOwnership());

exit:
    LogDebg("Send empty message, error:%s", ErrorToString(error));
    return error;
}
#endif

// LCOV_EXCL_START

#if OT_SHOULD_LOG_AT(OT_LOG_LEVEL_NOTE)

const char *MeshForwarder::MessageActionToString(MessageAction aAction, Error aError)
{
    static const char *const kMessageActionStrings[] = {
        "Received",                    // (0) kMessageReceive
        "Sent",                        // (1) kMessageTransmit
        "Prepping indir tx",           // (2) kMessagePrepareIndirect
        "Dropping",                    // (3) kMessageDrop
        "Dropping (reassembly queue)", // (4) kMessageReassemblyDrop
        "Evicting",                    // (5) kMessageEvict
#if OPENTHREAD_CONFIG_DELAY_AWARE_QUEUE_MANAGEMENT_ENABLE
        "Marked ECN",            // (6) kMessageMarkEcn
        "Dropping (queue mgmt)", // (7) kMessageQueueMgmtDrop
#endif
#if (OPENTHREAD_CONFIG_MAX_FRAMES_IN_DIRECT_TX_QUEUE > 0)
        "Dropping (dir queue full)", // (8) kMessageFullQueueDrop
#endif
    };

    const char *string = kMessageActionStrings[aAction];

    struct MessageActionChecker
    {
        InitEnumValidatorCounter();

        ValidateNextEnum(kMessageReceive);
        ValidateNextEnum(kMessageTransmit);
        ValidateNextEnum(kMessagePrepareIndirect);
        ValidateNextEnum(kMessageDrop);
        ValidateNextEnum(kMessageReassemblyDrop);
        ValidateNextEnum(kMessageEvict);
#if OPENTHREAD_CONFIG_DELAY_AWARE_QUEUE_MANAGEMENT_ENABLE
        ValidateNextEnum(kMessageMarkEcn);
        ValidateNextEnum(kMessageQueueMgmtDrop);
#endif
#if (OPENTHREAD_CONFIG_MAX_FRAMES_IN_DIRECT_TX_QUEUE > 0)
        ValidateNextEnum(kMessageFullQueueDrop);
#endif
    };

    if ((aAction == kMessageTransmit) && (aError != kErrorNone))
    {
        string = "Failed to send";
    }

    return string;
}

const char *MeshForwarder::MessagePriorityToString(const Message &aMessage)
{
    return Message::PriorityToString(aMessage.GetPriority());
}

#if OPENTHREAD_CONFIG_LOG_SRC_DST_IP_ADDRESSES
void MeshForwarder::LogIp6SourceDestAddresses(const Ip6::Headers &aHeaders, LogLevel aLogLevel)
{
    LogIp6AddressAndPort("src", aHeaders.GetSourceAddress(), aHeaders.GetSourcePort(), aLogLevel);
    LogIp6AddressAndPort("dst", aHeaders.GetDestinationAddress(), aHeaders.GetDestinationPort(), aLogLevel);
}

void MeshForwarder::LogIp6AddressAndPort(const char         *aLabel,
                                         const Ip6::Address &aAddress,
                                         uint16_t            aPort,
                                         LogLevel            aLogLevel)
{
    Ip6::SockAddr::InfoString string;

    string.Append("[%s]", aAddress.ToString().AsCString());

    if (aPort != 0)
    {
        string.Append(":%u", aPort);
    }

    LogAt(aLogLevel, "    %s:%s", aLabel, string.AsCString());
}

#else
void MeshForwarder::LogIp6SourceDestAddresses(const Ip6::Headers &, LogLevel) {}
#endif

void MeshForwarder::LogIp6Message(MessageAction       aAction,
                                  const Message      &aMessage,
                                  const Mac::Address *aMacAddress,
                                  Error               aError,
                                  LogLevel            aLogLevel)
{
    Ip6::Headers              headers;
    String<kMaxLogStringSize> string;

    SuccessOrExit(headers.ParseFrom(aMessage));

    string.Append("%s IPv6 %s msg, len:%u, chksum:%04x, ecn:%s, ", MessageActionToString(aAction, aError),
                  Ip6::Ip6::IpProtoToString(headers.GetIpProto()), aMessage.GetLength(), headers.GetChecksum(),
                  Ip6::Ip6::EcnToString(headers.GetEcn()));

    AppendMacAddrToLogString(string, aAction, aMacAddress);
    AppendSecErrorPrioRssRadioLabelsToLogString(string, aAction, aMessage, aError);

    LogAt(aLogLevel, "%s", string.AsCString());

    if (aAction != kMessagePrepareIndirect)
    {
        LogIp6SourceDestAddresses(headers, aLogLevel);
    }

exit:
    return;
}

void MeshForwarder::AppendMacAddrToLogString(StringWriter       &aString,
                                             MessageAction       aAction,
                                             const Mac::Address *aMacAddress)
{
    VerifyOrExit(aMacAddress != nullptr);

    if (aAction == kMessageReceive)
    {
        aString.Append("from:");
    }
    else
    {
        aString.Append("to:");
    }

    aString.Append("%s, ", aMacAddress->ToString().AsCString());

exit:
    return;
}

void MeshForwarder::AppendSecErrorPrioRssRadioLabelsToLogString(StringWriter  &aString,
                                                                MessageAction  aAction,
                                                                const Message &aMessage,
                                                                Error          aError)
{
    aString.Append("sec:%s, ", ToYesNo(aMessage.IsLinkSecurityEnabled()));

    if (aError != kErrorNone)
    {
        aString.Append("error:%s, ", ErrorToString(aError));
    }

    aString.Append("prio:%s", MessagePriorityToString(aMessage));

    if ((aAction == kMessageReceive) || (aAction == kMessageReassemblyDrop))
    {
        aString.Append(", rss:%s", aMessage.GetRssAverager().ToString().AsCString());
    }

#if OPENTHREAD_CONFIG_MULTI_RADIO
    aString.Append(", radio:%s", aMessage.IsRadioTypeSet() ? RadioTypeToString(aMessage.GetRadioType()) : "all");
#endif
}

void MeshForwarder::LogMessage(MessageAction aAction, const Message &aMessage)
{
    LogMessage(aAction, aMessage, kErrorNone);
}

void MeshForwarder::LogMessage(MessageAction aAction, const Message &aMessage, Error aError)
{
    LogMessage(aAction, aMessage, aError, nullptr);
}

void MeshForwarder::LogMessage(MessageAction       aAction,
                               const Message      &aMessage,
                               Error               aError,
                               const Mac::Address *aMacAddress)

{
    LogLevel logLevel = kLogLevelInfo;

    switch (aAction)
    {
    case kMessageReceive:
    case kMessageTransmit:
    case kMessagePrepareIndirect:
#if OPENTHREAD_CONFIG_DELAY_AWARE_QUEUE_MANAGEMENT_ENABLE
    case kMessageMarkEcn:
#endif
        logLevel = (aError == kErrorNone) ? kLogLevelInfo : kLogLevelNote;
        break;

    case kMessageDrop:
    case kMessageReassemblyDrop:
    case kMessageEvict:
#if OPENTHREAD_CONFIG_DELAY_AWARE_QUEUE_MANAGEMENT_ENABLE
    case kMessageQueueMgmtDrop:
#endif
#if (OPENTHREAD_CONFIG_MAX_FRAMES_IN_DIRECT_TX_QUEUE > 0)
    case kMessageFullQueueDrop:
#endif
        logLevel = kLogLevelNote;
        break;
    }

    VerifyOrExit(Instance::GetLogLevel() >= logLevel);

    switch (aMessage.GetType())
    {
    case Message::kTypeIp6:
        LogIp6Message(aAction, aMessage, aMacAddress, aError, logLevel);
        break;

#if OPENTHREAD_FTD
    case Message::kType6lowpan:
        LogMeshMessage(aAction, aMessage, aMacAddress, aError, logLevel);
        break;
#endif

    default:
        break;
    }

exit:
    return;
}

void MeshForwarder::LogFrame(const char *aActionText, const Mac::Frame &aFrame, Error aError)
{
    if (aError != kErrorNone)
    {
        LogNote("%s, aError:%s, %s", aActionText, ErrorToString(aError), aFrame.ToInfoString().AsCString());
    }
    else
    {
        LogInfo("%s, %s", aActionText, aFrame.ToInfoString().AsCString());
    }
}

void MeshForwarder::LogFragmentFrameDrop(Error                         aError,
                                         const RxInfo                 &aRxInfo,
                                         const Lowpan::FragmentHeader &aFragmentHeader)
{
    LogNote("Dropping rx frag frame, error:%s, %s, tag:%d, offset:%d, dglen:%d", ErrorToString(aError),
            aRxInfo.ToString().AsCString(), aFragmentHeader.GetDatagramTag(), aFragmentHeader.GetDatagramOffset(),
            aFragmentHeader.GetDatagramSize());
}

void MeshForwarder::LogLowpanHcFrameDrop(Error aError, const RxInfo &aRxInfo)
{
    LogNote("Dropping rx lowpan HC frame, error:%s, %s", ErrorToString(aError), aRxInfo.ToString().AsCString());
}

MeshForwarder::RxInfo::InfoString MeshForwarder::RxInfo::ToString(void) const
{
    InfoString string;

    string.Append("len:%d, src:%s, dst:%s, sec:%s", mFrameData.GetLength(), GetSrcAddr().ToString().AsCString(),
                  GetDstAddr().ToString().AsCString(), ToYesNo(IsLinkSecurityEnabled()));

    return string;
}

#else // #if OT_SHOULD_LOG_AT( OT_LOG_LEVEL_NOTE)

void MeshForwarder::LogMessage(MessageAction, const Message &) {}

void MeshForwarder::LogMessage(MessageAction, const Message &, Error) {}

void MeshForwarder::LogMessage(MessageAction, const Message &, Error, const Mac::Address *) {}

void MeshForwarder::LogFrame(const char *, const Mac::Frame &, Error) {}

void MeshForwarder::LogFragmentFrameDrop(Error, const RxInfo &, const Lowpan::FragmentHeader &) {}

void MeshForwarder::LogLowpanHcFrameDrop(Error, const RxInfo &) {}

#endif // #if OT_SHOULD_LOG_AT( OT_LOG_LEVEL_NOTE)

// LCOV_EXCL_STOP

} // namespace ot
