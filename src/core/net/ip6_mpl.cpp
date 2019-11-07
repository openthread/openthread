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

#include "common/code_utils.hpp"
#include "common/instance.hpp"
#include "common/locator-getters.hpp"
#include "common/message.hpp"
#include "common/random.hpp"
#include "net/ip6.hpp"

namespace ot {
namespace Ip6 {

void MplBufferedMessageMetadata::GenerateNextTransmissionTime(TimeMilli aCurrentTime, uint8_t aInterval)
{
    // Emulate Trickle timer behavior and set up the next retransmission within [0,I) range.
    uint8_t t = (aInterval == 0) ? aInterval : Random::NonCrypto::GetUint8InRange(0, aInterval);

    // Set transmission time at the beginning of the next interval.
    SetTransmissionTime(aCurrentTime + static_cast<uint32_t>(GetIntervalOffset() + t));
    SetIntervalOffset(aInterval - t);
}

Mpl::Mpl(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mTimerExpirations(0)
    , mSequence(0)
    , mSeedId(0)
    , mSeedSetTimer(aInstance, &Mpl::HandleSeedSetTimer, this)
    , mRetransmissionTimer(aInstance, &Mpl::HandleRetransmissionTimer, this)
    , mMatchingAddress(NULL)
{
    memset(mSeedSet, 0, sizeof(mSeedSet));
}

void Mpl::InitOption(OptionMpl &aOption, const Address &aAddress)
{
    aOption.Init();
    aOption.SetSequence(mSequence++);

    // Check if Seed Id can be elided.
    if (mMatchingAddress && aAddress == *mMatchingAddress)
    {
        aOption.SetSeedIdLength(OptionMpl::kSeedIdLength0);

        // Decrease default option length.
        aOption.SetLength(aOption.GetLength() - sizeof(mSeedId));
    }
    else
    {
        aOption.SetSeedIdLength(OptionMpl::kSeedIdLength2);
        aOption.SetSeedId(mSeedId);
    }
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
otError Mpl::UpdateSeedSet(uint16_t aSeedId, uint8_t aSequence)
{
    otError       error    = OT_ERROR_NONE;
    MplSeedEntry *insert   = NULL;
    MplSeedEntry *group    = mSeedSet;
    MplSeedEntry *evict    = mSeedSet;
    uint8_t       curCount = 0;
    uint8_t       maxCount = 0;

    for (uint32_t i = 0; i < kNumSeedEntries; i++, curCount++)
    {
        if (mSeedSet[i].GetLifetime() == 0)
        {
            // unused entries exist

            if (insert == NULL)
            {
                // no existing group, set insert and evict entry to be the same
                insert = &mSeedSet[i];
            }

            // mark first unused entry for eviction
            evict = &mSeedSet[i];
            break;
        }

        if (mSeedSet[i].GetSeedId() != group->GetSeedId())
        {
            // processing new group

            if (aSeedId == group->GetSeedId() && insert == NULL)
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

        if (aSeedId == mSeedSet[i].GetSeedId())
        {
            // have existing entries for aSeedId

            int8_t diff = static_cast<int8_t>(aSequence - mSeedSet[i].GetSequence());

            if (diff == 0)
            {
                // already received, drop message
                ExitNow(error = OT_ERROR_DROP);
            }
            else if (insert == NULL && diff < 0)
            {
                // insert in order of sequence
                insert = &mSeedSet[i];
                curCount++;
            }
        }
    }

    if (evict->GetLifetime() != 0)
    {
        // no free entries available, look to evict an existing entry
        assert(curCount != 0);

        if (aSeedId == group->GetSeedId() && insert == NULL)
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
        VerifyOrExit(maxCount > 1, error = OT_ERROR_DROP);

        if (insert == NULL)
        {
            // no existing entries for aSeedId
            insert = evict;
        }
        else
        {
            // require Sequence to be larger than oldest stored Sequence in group
            VerifyOrExit(insert > mSeedSet && aSeedId == (insert - 1)->GetSeedId(), error = OT_ERROR_DROP);
        }
    }

    if (evict > insert)
    {
        assert(insert >= mSeedSet);
        memmove(insert + 1, insert, static_cast<size_t>(evict - insert) * sizeof(MplSeedEntry));
    }
    else if (evict < insert)
    {
        assert(evict >= mSeedSet);
        memmove(evict, evict + 1, static_cast<size_t>(insert - 1 - evict) * sizeof(MplSeedEntry));
        insert--;
    }

    insert->SetSeedId(aSeedId);
    insert->SetSequence(aSequence);
    insert->SetLifetime(kSeedEntryLifetime);

    if (!mSeedSetTimer.IsRunning())
    {
        mSeedSetTimer.Start(kSeedEntryLifetimeDt);
    }

exit:
    return error;
}

void Mpl::AddBufferedMessage(Message &aMessage, uint16_t aSeedId, uint8_t aSequence, bool aIsOutbound)
{
    otError                    error       = OT_ERROR_NONE;
    Message *                  messageCopy = NULL;
    MplBufferedMessageMetadata messageMetadata;
    uint8_t                    hopLimit = 0;

#if OPENTHREAD_CONFIG_MPL_DYNAMIC_INTERVAL_ENABLE
    // adjust the first MPL forward interval dynamically according to the network scale
    uint8_t interval = (kDataMessageInterval / Mle::kMaxRouters) * Get<RouterTable>().GetNeighborCount();
#else
    uint8_t interval = kDataMessageInterval;
#endif

    VerifyOrExit(GetTimerExpirations() > 0);
    VerifyOrExit((messageCopy = aMessage.Clone()) != NULL, error = OT_ERROR_NO_BUFS);

    if (!aIsOutbound)
    {
        aMessage.Read(Header::GetHopLimitOffset(), Header::GetHopLimitSize(), &hopLimit);
        VerifyOrExit(hopLimit-- > 1, error = OT_ERROR_DROP);
        messageCopy->Write(Header::GetHopLimitOffset(), Header::GetHopLimitSize(), &hopLimit);
    }

    messageMetadata.SetSeedId(aSeedId);
    messageMetadata.SetSequence(aSequence);
    messageMetadata.SetTransmissionCount(aIsOutbound ? 1 : 0);
    messageMetadata.GenerateNextTransmissionTime(TimerMilli::GetNow(), interval);

    SuccessOrExit(error = messageMetadata.AppendTo(*messageCopy));
    mBufferedMessageSet.Enqueue(*messageCopy);

    mRetransmissionTimer.FireAtIfEarlier(messageMetadata.GetTransmissionTime());

exit:

    if (error != OT_ERROR_NONE && messageCopy != NULL)
    {
        messageCopy->Free();
    }
}

otError Mpl::ProcessOption(Message &aMessage, const Address &aAddress, bool aIsOutbound)
{
    otError   error;
    OptionMpl option;

    VerifyOrExit(aMessage.Read(aMessage.GetOffset(), sizeof(option), &option) >= OptionMpl::kMinLength &&
                     (option.GetSeedIdLength() == OptionMpl::kSeedIdLength0 ||
                      option.GetSeedIdLength() == OptionMpl::kSeedIdLength2),
                 error = OT_ERROR_PARSE);

    if (option.GetSeedIdLength() == OptionMpl::kSeedIdLength0)
    {
        // Retrieve MPL Seed Id from the IPv6 Source Address.
        option.SetSeedId(HostSwap16(aAddress.mFields.m16[7]));
    }

    // Check if the MPL Data Message is new.
    error = UpdateSeedSet(option.GetSeedId(), option.GetSequence());

    if (error == OT_ERROR_NONE)
    {
        AddBufferedMessage(aMessage, option.GetSeedId(), option.GetSequence(), aIsOutbound);
    }
    else if (aIsOutbound)
    {
        // In case MPL Data Message is generated locally, ignore potential error of the MPL Seed Set
        // to allow subsequent retransmissions with the same sequence number.
        ExitNow(error = OT_ERROR_NONE);
    }

exit:
    return error;
}

void Mpl::HandleRetransmissionTimer(Timer &aTimer)
{
    aTimer.GetOwner<Mpl>().HandleRetransmissionTimer();
}

void Mpl::HandleRetransmissionTimer(void)
{
    TimeMilli                  now      = TimerMilli::GetNow();
    TimeMilli                  nextTime = now.GetDistantFuture();
    MplBufferedMessageMetadata messageMetadata;
    Message *                  message;
    Message *                  nextMessage;

    for (message = mBufferedMessageSet.GetHead(); message != NULL; message = nextMessage)
    {
        nextMessage = message->GetNext();

        messageMetadata.ReadFrom(*message);

        if (now < messageMetadata.GetTransmissionTime())
        {
            if (nextTime > messageMetadata.GetTransmissionTime())
            {
                nextTime = messageMetadata.GetTransmissionTime();
            }
        }
        else
        {
            // Update the number of transmission timer expirations.
            messageMetadata.SetTransmissionCount(messageMetadata.GetTransmissionCount() + 1);

            if (messageMetadata.GetTransmissionCount() < GetTimerExpirations())
            {
                Message *messageCopy = message->Clone(message->GetLength() - sizeof(MplBufferedMessageMetadata));

                if (messageCopy != NULL)
                {
                    if (messageMetadata.GetTransmissionCount() > 1)
                    {
                        messageCopy->SetSubType(Message::kSubTypeMplRetransmission);
                    }

                    Get<Ip6>().EnqueueDatagram(*messageCopy);
                }

                messageMetadata.GenerateNextTransmissionTime(now, kDataMessageInterval);
                messageMetadata.UpdateIn(*message);

                if (nextTime > messageMetadata.GetTransmissionTime())
                {
                    nextTime = messageMetadata.GetTransmissionTime();
                }
            }
            else
            {
                mBufferedMessageSet.Dequeue(*message);

                if (messageMetadata.GetTransmissionCount() == GetTimerExpirations())
                {
                    if (messageMetadata.GetTransmissionCount() > 1)
                    {
                        message->SetSubType(Message::kSubTypeMplRetransmission);
                    }

                    // Remove the extra metadata from the MPL Data Message.
                    MplBufferedMessageMetadata::RemoveFrom(*message);
                    Get<Ip6>().EnqueueDatagram(*message);
                }
                else
                {
                    // Stop retransmitting if the number of timer expirations is already exceeded.
                    message->Free();
                }
            }
        }
    }

    if (nextTime < now.GetDistantFuture())
    {
        mRetransmissionTimer.FireAt(nextTime);
    }
}

void Mpl::HandleSeedSetTimer(Timer &aTimer)
{
    aTimer.GetOwner<Mpl>().HandleSeedSetTimer();
}

void Mpl::HandleSeedSetTimer(void)
{
    bool startTimer = false;
    int  j          = 0;

    for (int i = 0; i < kNumSeedEntries && mSeedSet[i].GetLifetime(); i++)
    {
        mSeedSet[i].SetLifetime(mSeedSet[i].GetLifetime() - 1);

        if (mSeedSet[i].GetLifetime() > 0)
        {
            mSeedSet[j++] = mSeedSet[i];
            startTimer    = true;
        }
    }

    for (; j < kNumSeedEntries && mSeedSet[j].GetLifetime(); j++)
    {
        mSeedSet[j].SetLifetime(0);
    }

    if (startTimer)
    {
        mSeedSetTimer.Start(kSeedEntryLifetimeDt);
    }
}

} // namespace Ip6
} // namespace ot
