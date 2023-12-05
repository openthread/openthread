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

#include "common/code_utils.hpp"
#include "common/debug.hpp"
#include "common/encoding.hpp"
#include "common/locator_getters.hpp"
#include "common/message.hpp"
#include "common/random.hpp"
#include "common/time_ticker.hpp"
#include "instance/instance.hpp"
#include "net/ip6.hpp"
#include "net/ip6_filter.hpp"
#include "net/netif.hpp"
#include "net/tcp6.hpp"
#include "net/udp6.hpp"
#include "radio/radio.hpp"
#include "thread/mle.hpp"
#include "thread/mle_router.hpp"
#include "thread/thread_netif.hpp"

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

#if OPENTHREAD_FTD
    mFragmentPriorityList.Clear();
#endif

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
    mFragmentPriorityList.Clear();
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
    Mac::Addresses addresses;
    Mac::PanIds    panIds;

    addresses.mSource.SetShort(Get<Mac::Mac>().GetShortAddress());

    if (addresses.mSource.IsShortAddrInvalid() || aMacDest.IsExtended())
    {
        addresses.mSource.SetExtended(Get<Mac::Mac>().GetExtAddress());
    }

    addresses.mDestination = aMacDest;
    panIds.SetBothSourceDestination(Get<Mac::Mac>().GetPanId());

    PrepareMacHeaders(aFrame, Mac::Frame::kTypeData, addresses, panIds, Mac::Frame::kSecurityEncMic32,
                      Mac::Frame::kKeyIdMode1, nullptr);

    aFrame.SetAckRequest(aAckRequest);
    aFrame.SetPayloadLength(0);
}

void MeshForwarder::EvictMessage(Message &aMessage)
{
    PriorityQueue *queue = aMessage.GetPriorityQueue();

    OT_ASSERT(queue != nullptr);

    if (queue == &mSendQueue)
    {
#if OPENTHREAD_FTD
        for (Child &child : Get<ChildTable>().Iterate(Child::kInStateAnyExceptInvalid))
        {
            IgnoreError(mIndirectSender.RemoveMessageFromSleepyChild(aMessage, child));
        }
#endif

        FinalizeMessageDirectTx(aMessage, kErrorNoBufs);

        if (mSendMessage == &aMessage)
        {
            mSendMessage = nullptr;
        }
    }

    LogMessage(kMessageEvict, aMessage, kErrorNoBufs);
    queue->DequeueAndFree(aMessage);
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
                FragmentPriorityList::Entry *entry;

                entry = mFragmentPriorityList.FindEntry(meshHeader.GetSource(), fragmentHeader.GetDatagramTag());

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
            FragmentPriorityList::Entry *entry;

            entry = mFragmentPriorityList.FindEntry(meshHeader.GetSource(), fragmentHeader.GetDatagramTag());
            VerifyOrExit(entry != nullptr);

            if (entry->ShouldDrop())
            {
                error = kErrorDrop;
            }

            // We can clear the entry if it is the last fragment and
            // only if the message is being prepared to be sent out.
            if (aPreparingToSend && (fragmentHeader.GetDatagramOffset() + aMessage.GetLength() - offset >=
                                     fragmentHeader.GetDatagramSize()))
            {
                entry->Clear();
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
            mSendQueue.DequeueAndFree(*curMessage);
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
        if (ip6Header.GetDestination().IsLinkLocal() || ip6Header.GetDestination().IsLinkLocalMulticast())
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
            mMacAddrs.mDestination.SetShort(mle.GetNextHop(Mac::kShortAddrBroadcast));
        }
        else
        {
            mMacAddrs.mDestination.SetShort(Mac::kShortAddrBroadcast);
        }
    }
    else if (ip6Header.GetDestination().IsLinkLocal())
    {
        GetMacDestinationAddress(ip6Header.GetDestination(), mMacAddrs.mDestination);
    }
    else if (mle.IsMinimalEndDevice())
    {
        mMacAddrs.mDestination.SetShort(mle.GetNextHop(Mac::kShortAddrBroadcast));
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
        if (mSendMessage->GetSubType() == Message::kSubTypeMleDiscoverRequest)
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

        if ((mSendMessage->GetSubType() == Message::kSubTypeMleChildIdRequest) && mSendMessage->IsLinkSecurityEnabled())
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

void MeshForwarder::PrepareMacHeaders(Mac::TxFrame             &aFrame,
                                      Mac::Frame::Type          aFrameType,
                                      const Mac::Addresses     &aMacAddrs,
                                      const Mac::PanIds        &aPanIds,
                                      Mac::Frame::SecurityLevel aSecurityLevel,
                                      Mac::Frame::KeyIdMode     aKeyIdMode,
                                      const Message            *aMessage)
{
    bool                iePresent;
    Mac::Frame::Version version;

    iePresent = CalcIePresent(aMessage);
    version   = CalcFrameVersion(Get<NeighborTable>().FindNeighbor(aMacAddrs.mDestination), iePresent);

    aFrame.InitMacHeader(aFrameType, version, aMacAddrs, aPanIds, aSecurityLevel, aKeyIdMode);

#if OPENTHREAD_CONFIG_MAC_HEADER_IE_SUPPORT
    if (iePresent)
    {
        AppendHeaderIe(aMessage, aFrame);
    }
#endif
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
    Mac::Frame::SecurityLevel securityLevel;
    Mac::Frame::KeyIdMode     keyIdMode;
    Mac::PanIds               panIds;
    uint16_t                  payloadLength;
    uint16_t                  origMsgOffset;
    uint16_t                  nextOffset;
    FrameBuilder              frameBuilder;

start:

    securityLevel = Mac::Frame::kSecurityNone;
    keyIdMode     = Mac::Frame::kKeyIdMode1;

    if (aMessage.IsLinkSecurityEnabled())
    {
        securityLevel = Mac::Frame::kSecurityEncMic32;

        switch (aMessage.GetSubType())
        {
        case Message::kSubTypeJoinerEntrust:
            keyIdMode = Mac::Frame::kKeyIdMode0;
            break;

        case Message::kSubTypeMleAnnounce:
            keyIdMode = Mac::Frame::kKeyIdMode2;
            break;

        default:
            // Use the `kKeyIdMode1`
            break;
        }
    }

    panIds.SetBothSourceDestination(Get<Mac::Mac>().GetPanId());

    switch (aMessage.GetSubType())
    {
    case Message::kSubTypeMleAnnounce:
        aFrame.SetChannel(aMessage.GetChannel());
        aFrame.SetRxChannelAfterTxDone(Get<Mac::Mac>().GetPanChannel());
        panIds.SetDestination(Mac::kPanIdBroadcast);
        break;

    case Message::kSubTypeMleDiscoverRequest:
    case Message::kSubTypeMleDiscoverResponse:
        panIds.SetDestination(aMessage.GetPanId());
        break;

    default:
        break;
    }

    PrepareMacHeaders(aFrame, Mac::Frame::kTypeData, aMacAddrs, panIds, securityLevel, keyIdMode, &aMessage);

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
    if ((aFrame.GetHeaderIe(Mac::CslIe::kHeaderIeId) != nullptr) && aIsDataPoll)
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

        if (aAllowNeighborRemove && (Mle::IsActiveRouter(aNeighbor.GetRloc16())) &&
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

    switch (aMessage.GetSubType())
    {
    case Message::kSubTypeMleDiscoverRequest:
        // Note that `HandleDiscoveryRequestFrameTxDone()` may update
        // `aMessage` and mark it again for direct transmission.
        Get<Mle::DiscoverScanner>().HandleDiscoveryRequestFrameTxDone(aMessage, aError);
        break;

    case Message::kSubTypeMleChildIdRequest:
        Get<Mle::Mle>().HandleChildIdRequestTxDone(aMessage);
        break;

    default:
        break;
    }

exit:
    return;
}

bool MeshForwarder::RemoveMessageIfNoPendingTx(Message &aMessage)
{
    bool didRemove = false;

#if OPENTHREAD_FTD
    VerifyOrExit(!aMessage.IsDirectTransmission() && !aMessage.IsChildPending());
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

void MeshForwarder::HandleReceivedFrame(Mac::RxFrame &aFrame)
{
    ThreadLinkInfo linkInfo;
    Mac::Addresses macAddrs;
    FrameData      frameData;
    Error          error = kErrorNone;

    VerifyOrExit(mEnabled, error = kErrorInvalidState);

    SuccessOrExit(error = aFrame.GetSrcAddr(macAddrs.mSource));
    SuccessOrExit(error = aFrame.GetDstAddr(macAddrs.mDestination));

    linkInfo.SetFrom(aFrame);

    frameData.Init(aFrame.GetPayload(), aFrame.GetPayloadLength());

    Get<SupervisionListener>().UpdateOnReceive(macAddrs.mSource, linkInfo.IsLinkSecurityEnabled());

    switch (aFrame.GetType())
    {
    case Mac::Frame::kTypeData:
        if (Lowpan::MeshHeader::IsMeshHeader(frameData))
        {
#if OPENTHREAD_FTD
            HandleMesh(frameData, macAddrs.mSource, linkInfo);
#endif
        }
        else if (Lowpan::FragmentHeader::IsFragmentHeader(frameData))
        {
            HandleFragment(frameData, macAddrs, linkInfo);
        }
        else if (Lowpan::Lowpan::IsLowpanHc(frameData))
        {
            HandleLowpanHC(frameData, macAddrs, linkInfo);
        }
        else
        {
            VerifyOrExit(frameData.GetLength() == 0, error = kErrorNotLowpanDataFrame);

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

void MeshForwarder::HandleFragment(FrameData            &aFrameData,
                                   const Mac::Addresses &aMacAddrs,
                                   const ThreadLinkInfo &aLinkInfo)
{
    Error                  error = kErrorNone;
    Lowpan::FragmentHeader fragmentHeader;
    Message               *message = nullptr;

    SuccessOrExit(error = fragmentHeader.ParseFrom(aFrameData));

#if OPENTHREAD_CONFIG_MULTI_RADIO

    if (aLinkInfo.mLinkSecurity)
    {
        Neighbor *neighbor = Get<NeighborTable>().FindNeighbor(aMacAddrs.mSource, Neighbor::kInStateAnyExceptInvalid);

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
        UpdateRoutes(aFrameData, aMacAddrs);
#endif

        SuccessOrExit(error = FrameToMessage(aFrameData, datagramSize, aMacAddrs, message));

        VerifyOrExit(datagramSize >= message->GetLength(), error = kErrorParse);
        SuccessOrExit(error = message->SetLength(datagramSize));

        message->SetDatagramTag(fragmentHeader.GetDatagramTag());
        message->SetTimestampToNow();
        message->SetLinkInfo(aLinkInfo);

        VerifyOrExit(Get<Ip6::Filter>().Accept(*message), error = kErrorDrop);

#if OPENTHREAD_FTD
        SendIcmpErrorIfDstUnreach(*message, aMacAddrs);
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
                msg.GetOffset() + aFrameData.GetLength() <= fragmentHeader.GetDatagramSize() &&
                msg.IsLinkSecurityEnabled() == aLinkInfo.IsLinkSecurityEnabled())
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

        if (!GetRxOnWhenIdle() && (message == nullptr) && aLinkInfo.IsLinkSecurityEnabled())
        {
            ClearReassemblyList();
        }

        VerifyOrExit(message != nullptr, error = kErrorDrop);

        message->WriteData(message->GetOffset(), aFrameData);
        message->MoveOffset(aFrameData.GetLength());
        message->AddRss(aLinkInfo.GetRss());
#if OPENTHREAD_CONFIG_MLE_LINK_METRICS_SUBJECT_ENABLE
        message->AddLqi(aLinkInfo.GetLqi());
#endif
        message->SetTimestampToNow();
    }

exit:

    if (error == kErrorNone)
    {
        if (message->GetOffset() >= message->GetLength())
        {
            mReassemblyList.Dequeue(*message);
            IgnoreError(HandleDatagram(*message, aLinkInfo, aMacAddrs.mSource));
        }
    }
    else
    {
        LogFragmentFrameDrop(error, aFrameData.GetLength(), aMacAddrs, fragmentHeader,
                             aLinkInfo.IsLinkSecurityEnabled());
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
    continueRxingTicks = mFragmentPriorityList.UpdateOnTimeTick();
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

Error MeshForwarder::FrameToMessage(const FrameData      &aFrameData,
                                    uint16_t              aDatagramSize,
                                    const Mac::Addresses &aMacAddrs,
                                    Message             *&aMessage)
{
    Error             error     = kErrorNone;
    FrameData         frameData = aFrameData;
    Message::Priority priority;

    SuccessOrExit(error = GetFramePriority(frameData, aMacAddrs, priority));

    aMessage = Get<MessagePool>().Allocate(Message::kTypeIp6, /* aReserveHeader */ 0, Message::Settings(priority));
    VerifyOrExit(aMessage, error = kErrorNoBufs);

    SuccessOrExit(error = Get<Lowpan::Lowpan>().Decompress(*aMessage, aMacAddrs, frameData, aDatagramSize));

    SuccessOrExit(error = aMessage->AppendData(frameData));
    aMessage->MoveOffset(frameData.GetLength());

exit:
    return error;
}

void MeshForwarder::HandleLowpanHC(const FrameData      &aFrameData,
                                   const Mac::Addresses &aMacAddrs,
                                   const ThreadLinkInfo &aLinkInfo)
{
    Error    error   = kErrorNone;
    Message *message = nullptr;

#if OPENTHREAD_FTD
    UpdateRoutes(aFrameData, aMacAddrs);
#endif

    SuccessOrExit(error = FrameToMessage(aFrameData, 0, aMacAddrs, message));

    message->SetLinkInfo(aLinkInfo);

    VerifyOrExit(Get<Ip6::Filter>().Accept(*message), error = kErrorDrop);

#if OPENTHREAD_FTD
    SendIcmpErrorIfDstUnreach(*message, aMacAddrs);
#endif

exit:

    if (error == kErrorNone)
    {
        IgnoreError(HandleDatagram(*message, aLinkInfo, aMacAddrs.mSource));
    }
    else
    {
        LogLowpanHcFrameDrop(error, aFrameData.GetLength(), aMacAddrs, aLinkInfo.IsLinkSecurityEnabled());
        FreeMessage(message);
    }
}

Error MeshForwarder::HandleDatagram(Message &aMessage, const ThreadLinkInfo &aLinkInfo, const Mac::Address &aMacSource)
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

    return Get<Ip6::Ip6>().HandleDatagram(OwnedPtr<Message>(&aMessage), &aLinkInfo);
}

Error MeshForwarder::GetFramePriority(const FrameData      &aFrameData,
                                      const Mac::Addresses &aMacAddrs,
                                      Message::Priority    &aPriority)
{
    Error        error = kErrorNone;
    Ip6::Headers headers;

    SuccessOrExit(error = headers.DecompressFrom(aFrameData, aMacAddrs, GetInstance()));

    aPriority = Ip6::Ip6::DscpToPriority(headers.GetIp6Header().GetDscp());

    // Only ICMPv6 error messages are prioritized.
    if (headers.IsIcmp6() && headers.GetIcmpHeader().IsError())
    {
        aPriority = Message::kPriorityNet;
    }

    if (headers.IsUdp())
    {
        uint16_t destPort = headers.GetUdpHeader().GetDestinationPort();

        if (destPort == Mle::kUdpPort)
        {
            aPriority = Message::kPriorityNet;
        }
        else if (Get<Tmf::Agent>().IsTmfMessage(headers.GetSourceAddress(), headers.GetDestinationAddress(), destPort))
        {
            aPriority = Tmf::Agent::DscpToPriority(headers.GetIp6Header().GetDscp());
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

bool MeshForwarder::CalcIePresent(const Message *aMessage)
{
    bool iePresent = false;

    OT_UNUSED_VARIABLE(aMessage);

#if OPENTHREAD_CONFIG_MAC_HEADER_IE_SUPPORT
#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
    iePresent |= (aMessage != nullptr && aMessage->IsTimeSync());
#endif
#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
    if (!(aMessage != nullptr && aMessage->GetSubType() == Message::kSubTypeMleDiscoverRequest))
    {
        iePresent |= Get<Mac::Mac>().IsCslEnabled();
    }
#endif
#endif

    return iePresent;
}

#if OPENTHREAD_CONFIG_MAC_HEADER_IE_SUPPORT
void MeshForwarder::AppendHeaderIe(const Message *aMessage, Mac::TxFrame &aFrame)
{
    uint8_t index     = 0;
    bool    iePresent = false;
    bool    payloadPresent =
        (aFrame.GetType() == Mac::Frame::kTypeMacCmd) || (aMessage != nullptr && aMessage->GetLength() != 0);

#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
    if (aMessage != nullptr && aMessage->IsTimeSync())
    {
        IgnoreError(aFrame.AppendHeaderIeAt<Mac::TimeIe>(index));
        iePresent = true;
    }
#endif
#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
    if (Get<Mac::Mac>().IsCslEnabled())
    {
        IgnoreError(aFrame.AppendHeaderIeAt<Mac::CslIe>(index));
        aFrame.SetCslIePresent(true);
        iePresent = true;
    }
#endif

    if (iePresent && payloadPresent)
    {
        // Assume no Payload IE in current implementation
        IgnoreError(aFrame.AppendHeaderIeAt<Mac::Termination2Ie>(index));
    }
}
#endif

Mac::Frame::Version MeshForwarder::CalcFrameVersion(const Neighbor *aNeighbor, bool aIePresent) const
{
    Mac::Frame::Version version = Mac::Frame::kVersion2006;
    OT_UNUSED_VARIABLE(aNeighbor);

    if (aIePresent)
    {
        version = Mac::Frame::kVersion2015;
    }
#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE
    else if ((aNeighbor != nullptr) && Get<ChildTable>().Contains(*aNeighbor) &&
             static_cast<const Child *>(aNeighbor)->IsCslSynchronized())
    {
        version = Mac::Frame::kVersion2015;
    }
#endif
#if OPENTHREAD_CONFIG_MLE_LINK_METRICS_INITIATOR_ENABLE
    else if (aNeighbor != nullptr && aNeighbor->IsEnhAckProbingActive())
    {
        version = Mac::Frame::kVersion2015; ///< Set version to 2015 to fetch Link Metrics data in Enh-ACK.
    }
#endif

    return version;
}

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

    static_assert(kMessageReceive == 0, "kMessageReceive value is incorrect");
    static_assert(kMessageTransmit == 1, "kMessageTransmit value is incorrect");
    static_assert(kMessagePrepareIndirect == 2, "kMessagePrepareIndirect value is incorrect");
    static_assert(kMessageDrop == 3, "kMessageDrop value is incorrect");
    static_assert(kMessageReassemblyDrop == 4, "kMessageReassemblyDrop value is incorrect");
    static_assert(kMessageEvict == 5, "kMessageEvict value is incorrect");
#if OPENTHREAD_CONFIG_DELAY_AWARE_QUEUE_MANAGEMENT_ENABLE
    static_assert(kMessageMarkEcn == 6, "kMessageMarkEcn is incorrect");
    static_assert(kMessageQueueMgmtDrop == 7, "kMessageQueueMgmtDrop is incorrect");
#if (OPENTHREAD_CONFIG_MAX_FRAMES_IN_DIRECT_TX_QUEUE > 0)
    static_assert(kMessageFullQueueDrop == 8, "kMessageFullQueueDrop is incorrect");
#endif
#else
#if (OPENTHREAD_CONFIG_MAX_FRAMES_IN_DIRECT_TX_QUEUE > 0)
    static_assert(kMessageFullQueueDrop == 6, "kMessageFullQueueDrop is incorrect");
#endif
#endif

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
    uint16_t srcPort = aHeaders.GetSourcePort();
    uint16_t dstPort = aHeaders.GetDestinationPort();

    if (srcPort != 0)
    {
        LogAt(aLogLevel, "    src:[%s]:%d", aHeaders.GetSourceAddress().ToString().AsCString(), srcPort);
    }
    else
    {
        LogAt(aLogLevel, "    src:[%s]", aHeaders.GetSourceAddress().ToString().AsCString());
    }

    if (dstPort != 0)
    {
        LogAt(aLogLevel, "    dst:[%s]:%d", aHeaders.GetDestinationAddress().ToString().AsCString(), dstPort);
    }
    else
    {
        LogAt(aLogLevel, "    dst:[%s]", aHeaders.GetDestinationAddress().ToString().AsCString());
    }
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
    Ip6::Headers headers;
    bool         shouldLogRss;
    bool         shouldLogRadio = false;
    const char  *radioString    = "";

    SuccessOrExit(headers.ParseFrom(aMessage));

    shouldLogRss = (aAction == kMessageReceive) || (aAction == kMessageReassemblyDrop);

#if OPENTHREAD_CONFIG_MULTI_RADIO
    shouldLogRadio = true;
    radioString    = aMessage.IsRadioTypeSet() ? RadioTypeToString(aMessage.GetRadioType()) : "all";
#endif

    LogAt(aLogLevel, "%s IPv6 %s msg, len:%d, chksum:%04x, ecn:%s%s%s, sec:%s%s%s, prio:%s%s%s%s%s",
          MessageActionToString(aAction, aError), Ip6::Ip6::IpProtoToString(headers.GetIpProto()), aMessage.GetLength(),
          headers.GetChecksum(), Ip6::Ip6::EcnToString(headers.GetEcn()),
          (aMacAddress == nullptr) ? "" : ((aAction == kMessageReceive) ? ", from:" : ", to:"),
          (aMacAddress == nullptr) ? "" : aMacAddress->ToString().AsCString(),
          ToYesNo(aMessage.IsLinkSecurityEnabled()),
          (aError == kErrorNone) ? "" : ", error:", (aError == kErrorNone) ? "" : ErrorToString(aError),
          MessagePriorityToString(aMessage), shouldLogRss ? ", rss:" : "",
          shouldLogRss ? aMessage.GetRssAverager().ToString().AsCString() : "", shouldLogRadio ? ", radio:" : "",
          radioString);

    if (aAction != kMessagePrepareIndirect)
    {
        LogIp6SourceDestAddresses(headers, aLogLevel);
    }

exit:
    return;
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
                                         uint16_t                      aFrameLength,
                                         const Mac::Addresses         &aMacAddrs,
                                         const Lowpan::FragmentHeader &aFragmentHeader,
                                         bool                          aIsSecure)
{
    LogNote("Dropping rx frag frame, error:%s, len:%d, src:%s, dst:%s, tag:%d, offset:%d, dglen:%d, sec:%s",
            ErrorToString(aError), aFrameLength, aMacAddrs.mSource.ToString().AsCString(),
            aMacAddrs.mDestination.ToString().AsCString(), aFragmentHeader.GetDatagramTag(),
            aFragmentHeader.GetDatagramOffset(), aFragmentHeader.GetDatagramSize(), ToYesNo(aIsSecure));
}

void MeshForwarder::LogLowpanHcFrameDrop(Error                 aError,
                                         uint16_t              aFrameLength,
                                         const Mac::Addresses &aMacAddrs,
                                         bool                  aIsSecure)
{
    LogNote("Dropping rx lowpan HC frame, error:%s, len:%d, src:%s, dst:%s, sec:%s", ErrorToString(aError),
            aFrameLength, aMacAddrs.mSource.ToString().AsCString(), aMacAddrs.mDestination.ToString().AsCString(),
            ToYesNo(aIsSecure));
}

#else // #if OT_SHOULD_LOG_AT( OT_LOG_LEVEL_NOTE)

void MeshForwarder::LogMessage(MessageAction, const Message &) {}

void MeshForwarder::LogMessage(MessageAction, const Message &, Error) {}

void MeshForwarder::LogMessage(MessageAction, const Message &, Error, const Mac::Address *) {}

void MeshForwarder::LogFrame(const char *, const Mac::Frame &, Error) {}

void MeshForwarder::LogFragmentFrameDrop(Error, uint16_t, const Mac::Addresses &, const Lowpan::FragmentHeader &, bool)
{
}

void MeshForwarder::LogLowpanHcFrameDrop(Error, uint16_t, const Mac::Addresses &, bool) {}

#endif // #if OT_SHOULD_LOG_AT( OT_LOG_LEVEL_NOTE)

// LCOV_EXCL_STOP

} // namespace ot
