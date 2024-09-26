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
 *   This file implements MPL.
 */

#include "ip6_mpl.hpp"

#include "instance/instance.hpp"

namespace ot {
namespace Ip6 {

Mpl::Mpl(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mSequence(0)
#if OPENTHREAD_FTD
    , mRetransmissionTimer(aInstance)
#endif
{
    ClearAllBytes(mSeedSet);
}

void MplOption::Init(SeedIdLength aSeedIdLength)
{
    SetType(kType);

    switch (aSeedIdLength)
    {
    case kSeedIdLength0:
        SetLength(sizeof(*this) - sizeof(Option) - sizeof(mSeedId));
        break;
    case kSeedIdLength2:
        SetLength(sizeof(*this) - sizeof(Option));
        break;
    default:
        OT_ASSERT(false);
    }

    mControl = aSeedIdLength;
}

void Mpl::InitOption(MplOption &aOption, const Address &aAddress)
{
    if (aAddress == Get<Mle::Mle>().GetMeshLocalRloc())
    {
        // Seed ID can be elided when `aAddress` is RLOC.
        aOption.Init(MplOption::kSeedIdLength0);
    }
    else
    {
        aOption.Init(MplOption::kSeedIdLength2);
        aOption.SetSeedId(Get<Mle::Mle>().GetRloc16());
    }

    aOption.SetSequence(mSequence++);
}

Error Mpl::ProcessOption(Message &aMessage, const OffsetRange &aOffsetRange, const Address &aAddress, bool &aReceive)
{
    Error     error;
    MplOption option;

    // Read the min size bytes first, then check the expected
    // `SeedIdLength` and read the full `MplOption` if needed.
    SuccessOrExit(error = aMessage.Read(aOffsetRange, &option, MplOption::kMinSize));

    switch (option.GetSeedIdLength())
    {
    case MplOption::kSeedIdLength0:
        // Retrieve Seed ID from the IPv6 Source Address RLOC.
        VerifyOrExit(aAddress.GetIid().IsLocator(), error = kErrorDrop);
        option.SetSeedId(aAddress.GetIid().GetLocator());
        break;

    case MplOption::kSeedIdLength2:
        SuccessOrExit(error = aMessage.Read(aOffsetRange, option));
        break;

    case MplOption::kSeedIdLength8:
    case MplOption::kSeedIdLength16:
        ExitNow(error = kErrorParse);
    }

    // Check if the MPL Data Message is new.
    error = UpdateSeedSet(option.GetSeedId(), option.GetSequence());

    if (error == kErrorNone)
    {
#if OPENTHREAD_FTD
        AddBufferedMessage(aMessage, option.GetSeedId(), option.GetSequence());
#endif
    }
    else if (!aMessage.IsOriginThreadNetif())
    {
        aReceive = false;
        // In case MPL Data Message is generated locally, ignore potential error of the MPL Seed Set
        // to allow subsequent retransmissions with the same sequence number.
        ExitNow(error = kErrorNone);
    }

exit:
    return error;
}

/*
 * mSeedSet stores recently received (Seed ID, Sequence) values.
 * - (Seed ID, Sequence) values are grouped by Seed ID.
 * - (Seed ID, Sequence) groups are not sorted by Seed ID relative to other groups.
 * - (Seed ID, Sequence) values within a group are sorted by Sequence.
 * - All unused entries (marked by 0 lifetime) are grouped at the end.
 *
 * Update process:
 *
 * - Eviction selection:
 *   - If there are unused entries, mark the first unused entry for "eviction"
 *   - Otherwise, pick the first entry of the group that has the most entries.
 *
 * - Insert selection:
 *   - If there exists a group matching the Seed ID, select insert entry based on Sequence ordering.
 *   - Otherwise, set insert entry equal to evict entry.
 *
 * - If evicting a valid entry (lifetime non-zero):
 *   - Require group size to have >=2 entries.
 *   - If inserting into existing group, require Sequence to be larger than oldest stored Sequence in group.
 */
Error Mpl::UpdateSeedSet(uint16_t aSeedId, uint8_t aSequence)
{
    Error      error    = kErrorNone;
    SeedEntry *insert   = nullptr;
    SeedEntry *group    = mSeedSet;
    SeedEntry *evict    = mSeedSet;
    uint8_t    curCount = 0;
    uint8_t    maxCount = 0;

    for (uint32_t i = 0; i < kNumSeedEntries; i++, curCount++)
    {
        if (mSeedSet[i].mLifetime == 0)
        {
            // unused entries exist

            if (insert == nullptr)
            {
                // no existing group, set insert and evict entry to be the same
                insert = &mSeedSet[i];
            }

            // mark first unused entry for eviction
            evict = &mSeedSet[i];
            break;
        }

        if (mSeedSet[i].mSeedId != group->mSeedId)
        {
            // processing new group

            if (aSeedId == group->mSeedId && insert == nullptr)
            {
                // insert at end of existing group
                insert = &mSeedSet[i];
                curCount++;
            }

            if (maxCount < curCount)
            {
                // look to evict an entry from the seed with the most entries
                evict    = group;
                maxCount = curCount;
            }

            group    = &mSeedSet[i];
            curCount = 0;
        }

        if (aSeedId == mSeedSet[i].mSeedId)
        {
            // have existing entries for aSeedId

            if (aSequence == mSeedSet[i].mSequence)
            {
                // already received, drop message

                mSeedSet[i].mLifetime = kSeedEntryLifetime;
                ExitNow(error = kErrorDrop);
            }
            else if (insert == nullptr && SerialNumber::IsLess(aSequence, mSeedSet[i].mSequence))
            {
                // insert in order of sequence
                insert = &mSeedSet[i];
                curCount++;
            }
        }
    }

    if (evict->mLifetime != 0)
    {
        // no free entries available, look to evict an existing entry
        OT_ASSERT(curCount != 0);

        if (aSeedId == group->mSeedId && insert == nullptr)
        {
            // insert at end of existing group
            insert = &mSeedSet[kNumSeedEntries];
            curCount++;
        }

        if (maxCount < curCount)
        {
            // look to evict an entry from the seed with the most entries
            evict    = group;
            maxCount = curCount;
        }

        // require evict group size to have >= 2 entries
        VerifyOrExit(maxCount > 1, error = kErrorDrop);

        if (insert == nullptr)
        {
            // no existing entries for aSeedId
            insert = evict;
        }
        else
        {
            // require Sequence to be larger than oldest stored Sequence in group
            VerifyOrExit(insert > mSeedSet && aSeedId == (insert - 1)->mSeedId, error = kErrorDrop);
        }
    }

    if (evict > insert)
    {
        OT_ASSERT(insert >= mSeedSet);
        memmove(insert + 1, insert, static_cast<size_t>(evict - insert) * sizeof(SeedEntry));
    }
    else if (evict < insert)
    {
        OT_ASSERT(evict >= mSeedSet);
        memmove(evict, evict + 1, static_cast<size_t>(insert - 1 - evict) * sizeof(SeedEntry));
        insert--;
    }

    insert->mSeedId   = aSeedId;
    insert->mSequence = aSequence;
    insert->mLifetime = kSeedEntryLifetime;

    Get<TimeTicker>().RegisterReceiver(TimeTicker::kIp6Mpl);

exit:
    return error;
}

void Mpl::HandleTimeTick(void)
{
    bool continueRxingTicks = false;
    int  j                  = 0;

    for (int i = 0; i < kNumSeedEntries && mSeedSet[i].mLifetime; i++)
    {
        mSeedSet[i].mLifetime--;

        if (mSeedSet[i].mLifetime > 0)
        {
            mSeedSet[j++]      = mSeedSet[i];
            continueRxingTicks = true;
        }
    }

    for (; j < kNumSeedEntries && mSeedSet[j].mLifetime; j++)
    {
        mSeedSet[j].mLifetime = 0;
    }

    if (!continueRxingTicks)
    {
        Get<TimeTicker>().UnregisterReceiver(TimeTicker::kIp6Mpl);
    }
}

#if OPENTHREAD_FTD

uint8_t Mpl::DetermineMaxRetransmissions(void) const
{
    uint8_t maxRetx = 0;

    switch (Get<Mle::Mle>().GetRole())
    {
    case Mle::kRoleDisabled:
    case Mle::kRoleDetached:
        break;

    case Mle::kRoleChild:
        maxRetx = kChildRetransmissions;
        break;

    case Mle::kRoleRouter:
    case Mle::kRoleLeader:
        maxRetx = kRouterRetransmissions;
        break;
    }

    return maxRetx;
}

void Mpl::AddBufferedMessage(Message &aMessage, uint16_t aSeedId, uint8_t aSequence)
{
    Error    error       = kErrorNone;
    Message *messageCopy = nullptr;
    Metadata metadata;
    uint8_t  hopLimit = 0;
    uint8_t  interval;

#if OPENTHREAD_CONFIG_MPL_DYNAMIC_INTERVAL_ENABLE
    // adjust the first MPL forward interval dynamically according to the network scale
    interval = (kDataMessageInterval / Mle::kMaxRouters) * Get<RouterTable>().GetNeighborCount(kLinkQuality1);
#else
    interval = kDataMessageInterval;
#endif

    VerifyOrExit(DetermineMaxRetransmissions() > 0);
    VerifyOrExit((messageCopy = aMessage.Clone()) != nullptr, error = kErrorNoBufs);

    if (aMessage.IsOriginThreadNetif())
    {
        IgnoreError(aMessage.Read(Header::kHopLimitFieldOffset, hopLimit));
        VerifyOrExit(hopLimit-- > 1, error = kErrorDrop);
        messageCopy->Write(Header::kHopLimitFieldOffset, hopLimit);
    }

    // If the message originates from Thread Netif (i.e., it was
    // received over Thread radio), set the `mTransmissionCount` to
    // zero. Otherwise, the message originates from the host and will
    // be forwarded by `Ip6` to the Thread mesh, so the message itself
    // will be the first transmission and we set `mTransmissionCount`
    // to one.

    metadata.mSeedId            = aSeedId;
    metadata.mSequence          = aSequence;
    metadata.mTransmissionCount = aMessage.IsOriginThreadNetif() ? 0 : 1;
    metadata.mIntervalOffset    = 0;
    metadata.GenerateNextTransmissionTime(TimerMilli::GetNow(), interval);

    SuccessOrExit(error = metadata.AppendTo(*messageCopy));
    mBufferedMessageSet.Enqueue(*messageCopy);

    mRetransmissionTimer.FireAtIfEarlier(metadata.mTransmissionTime);

exit:
    FreeMessageOnError(messageCopy, error);
}

void Mpl::HandleRetransmissionTimer(void)
{
    NextFireTime nextTime;

    for (Message &message : mBufferedMessageSet)
    {
        Metadata metadata;
        Message *messageCopy;
        uint8_t  maxRetx;

        metadata.ReadFrom(message);

        if (nextTime.GetNow() < metadata.mTransmissionTime)
        {
            nextTime.UpdateIfEarlier(metadata.mTransmissionTime);
            continue;
        }

        metadata.mTransmissionCount++;

        maxRetx = DetermineMaxRetransmissions();

        if (metadata.mTransmissionCount > maxRetx)
        {
            // If the number of tx already exceeds the limit, remove
            // the message. This situation can potentially happen on
            // a device role change, which then updates the max MPL
            // retx.

            mBufferedMessageSet.DequeueAndFree(message);
            continue;
        }

        if (metadata.mTransmissionCount < maxRetx)
        {
            metadata.GenerateNextTransmissionTime(nextTime.GetNow(), kDataMessageInterval);
            metadata.UpdateIn(message);

            nextTime.UpdateIfEarlier(metadata.mTransmissionTime);

            messageCopy = message.Clone();
        }
        else
        {
            // This is the last retx of message, we can use the
            // `message` directly.

            mBufferedMessageSet.Dequeue(message);
            messageCopy = &message;
        }

        if (messageCopy != nullptr)
        {
            if (metadata.mTransmissionCount > 1)
            {
                // Mark all transmissions after the first one as "MPL
                // retx". This is used to decide whether to send this
                // message to the device's sleepy children.

                messageCopy->SetSubType(Message::kSubTypeMplRetransmission);
            }

            metadata.RemoveFrom(*messageCopy);
            messageCopy->SetLoopbackToHostAllowed(true);
            messageCopy->SetOrigin(Message::kOriginHostTrusted);
            Get<Ip6>().EnqueueDatagram(*messageCopy);
        }
    }

    mRetransmissionTimer.FireAt(nextTime);
}

void Mpl::Metadata::GenerateNextTransmissionTime(TimeMilli aCurrentTime, uint8_t aInterval)
{
    // Emulate Trickle timer behavior and set up the next retransmission within [0,I) range.
    uint8_t t = (aInterval == 0) ? aInterval : Random::NonCrypto::GetUint8InRange(0, aInterval);

    // Set transmission time at the beginning of the next interval.
    mTransmissionTime = aCurrentTime + static_cast<uint32_t>(mIntervalOffset + t);
    mIntervalOffset   = aInterval - t;
}

#endif // OPENTHREAD_FTD

} // namespace Ip6
} // namespace ot
