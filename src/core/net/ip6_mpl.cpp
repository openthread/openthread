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

#include "openthread/platform/random.h"

#include <common/code_utils.hpp>
#include <common/message.hpp>
#include <net/ip6.hpp>
#include <net/ip6_mpl.hpp>

namespace Thread {
namespace Ip6 {

void MplBufferedMessageMetadata::GenerateNextTransmissionTime(uint32_t aCurrentTime, uint8_t aInterval)
{
    // Emulate Trickle timer behavior and set up the next retransmission within [0,I) range.
    uint8_t t = otPlatRandomGet() % aInterval;

    // Set transmission time at the beginning of the next interval.
    SetTransmissionTime(aCurrentTime + GetIntervalOffset() + t);
    SetIntervalOffset(aInterval - t);
}

Mpl::Mpl(Ip6 &aIp6):
    mIp6(aIp6),
    mSeedSetTimer(aIp6.mTimerScheduler, &Mpl::HandleSeedSetTimer, this),
    mRetransmissionTimer(aIp6.mTimerScheduler, &Mpl::HandleRetransmissionTimer, this),
    mTimerExpirations(0),
    mSequence(0),
    mSeedId(0),
    mMatchingAddress(NULL)
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

ThreadError Mpl::UpdateSeedSet(uint16_t aSeedId, uint8_t aSequence)
{
    ThreadError error = kThreadError_None;
    MplSeedEntry *entry = NULL;
    int8_t diff;

    for (uint32_t i = 0; i < kNumSeedEntries; i++)
    {
        if (mSeedSet[i].GetLifetime() == 0)
        {
            // Start allocating from the first possible entry to speed up process of searching.
            if (entry == NULL)
            {
                entry = &mSeedSet[i];
            }
        }
        else if (mSeedSet[i].GetSeedId() == aSeedId)
        {
            entry = &mSeedSet[i];
            diff = static_cast<int8_t>(aSequence - entry->GetSequence());

            VerifyOrExit(diff > 0, error = kThreadError_Drop);

            break;
        }
    }

    VerifyOrExit(entry != NULL, error = kThreadError_Drop);

    entry->SetSeedId(aSeedId);
    entry->SetSequence(aSequence);
    entry->SetLifetime(kSeedEntryLifetime);
    mSeedSetTimer.Start(kSeedEntryLifetimeDt);

exit:
    return error;
}

void Mpl::UpdateBufferedSet(uint16_t aSeedId, uint8_t aSequence)
{
    int8_t diff;
    MplBufferedMessageMetadata messageMetadata;

    Message *message = mBufferedMessageSet.GetHead();
    Message *nextMessage = NULL;

    // Check if multicast forwarding is enabled.
    VerifyOrExit(GetTimerExpirations() > 0, ;);

    while (message != NULL)
    {
        nextMessage = message->GetNext();
        messageMetadata.ReadFrom(*message);

        if (messageMetadata.GetSeedId() == aSeedId)
        {
            diff = static_cast<int8_t>(aSequence - messageMetadata.GetSequence());

            if (diff > 0)
            {
                // Stop retransmitting MPL Data Message that is consider to be old.
                mBufferedMessageSet.Dequeue(*message);
                message->Free();
            }

            break;
        }

        message = nextMessage;
    }

exit:
    return;
}

void Mpl::AddBufferedMessage(Message &aMessage, uint16_t aSeedId, uint8_t aSequence, bool aIsOutbound)
{
    uint32_t now = Timer::GetNow();
    ThreadError error = kThreadError_None;
    Message *messageCopy = NULL;
    MplBufferedMessageMetadata messageMetadata;
    uint32_t nextTransmissionTime;
    uint8_t hopLimit = 0;

    VerifyOrExit(GetTimerExpirations() > 0,);
    VerifyOrExit((messageCopy = aMessage.Clone()) != NULL, error = kThreadError_NoBufs);

    if (!aIsOutbound)
    {
        aMessage.Read(Header::GetHopLimitOffset(), Header::GetHopLimitSize(), &hopLimit);
        VerifyOrExit(hopLimit-- > 1, error = kThreadError_Drop);
        messageCopy->Write(Header::GetHopLimitOffset(), Header::GetHopLimitSize(), &hopLimit);
    }

    messageMetadata.SetSeedId(aSeedId);
    messageMetadata.SetSequence(aSequence);
    messageMetadata.SetTransmissionCount(aIsOutbound ? 1 : 0);
    messageMetadata.GenerateNextTransmissionTime(now, kDataMessageInterval);

    // Append the message with MplBufferedMessageMetadata and add it to the queue.
    SuccessOrExit(error = messageMetadata.AppendTo(*messageCopy));
    mBufferedMessageSet.Enqueue(*messageCopy);

    if (mRetransmissionTimer.IsRunning())
    {
        // If timer is already running, check if it should be restarted with earlier fire time.
        nextTransmissionTime = mRetransmissionTimer.Gett0() + mRetransmissionTimer.Getdt();

        if (messageMetadata.IsEarlier(nextTransmissionTime))
        {
            mRetransmissionTimer.Start(messageMetadata.GetTransmissionTime() - now);
        }
    }
    else
    {
        // Otherwise just set the timer.
        mRetransmissionTimer.Start(messageMetadata.GetTransmissionTime() - now);
    }

exit:

    if (error != kThreadError_None && messageCopy != NULL)
    {
        messageCopy->Free();
    }

    return;
}

ThreadError Mpl::ProcessOption(Message &aMessage, const Address &aAddress, bool aIsOutbound)
{
    ThreadError error;
    OptionMpl option;

    VerifyOrExit(aMessage.Read(aMessage.GetOffset(), sizeof(option), &option) >= OptionMpl::kMinLength &&
                 (option.GetSeedIdLength() == OptionMpl::kSeedIdLength0 ||
                  option.GetSeedIdLength() == OptionMpl::kSeedIdLength2),
                 error = kThreadError_Drop);

    if (option.GetSeedIdLength() == OptionMpl::kSeedIdLength0)
    {
        // Retrieve MPL Seed Id from the IPv6 Source Address.
        option.SetSeedId(HostSwap16(aAddress.mFields.m16[7]));
    }

    // Check MPL Data Messages in the MPL Buffered Set against sequence number.
    UpdateBufferedSet(option.GetSeedId(), option.GetSequence());

    // Check if the MPL Data Message is new.
    error = UpdateSeedSet(option.GetSeedId(), option.GetSequence());

    if (error == kThreadError_None)
    {
        AddBufferedMessage(aMessage, option.GetSeedId(), option.GetSequence(), aIsOutbound);
    }
    else if (aIsOutbound)
    {
        // In case MPL Data Message is generated locally, ignore potential error of the MPL Seed Set
        // to allow subsequent retransmissions with the same sequence number.
        ExitNow(error = kThreadError_None);
    }

exit:
    return error;
}

void Mpl::HandleRetransmissionTimer(void *aContext)
{
    static_cast<Mpl *>(aContext)->HandleRetransmissionTimer();
}

void Mpl::HandleRetransmissionTimer()
{
    uint32_t now = Timer::GetNow();
    uint32_t nextDelta = 0xffffffff;
    MplBufferedMessageMetadata messageMetadata;

    Message *message = mBufferedMessageSet.GetHead();
    Message *nextMessage = NULL;

    while (message != NULL)
    {
        nextMessage = message->GetNext();
        messageMetadata.ReadFrom(*message);

        if (messageMetadata.IsLater(now))
        {
            // Calculate the next retransmission time and choose the lowest.
            if (messageMetadata.GetTransmissionTime() - now < nextDelta)
            {
                nextDelta = messageMetadata.GetTransmissionTime() - now;
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

                    mIp6.EnqueueDatagram(*messageCopy);
                }

                messageMetadata.GenerateNextTransmissionTime(now, kDataMessageInterval);
                messageMetadata.UpdateIn(*message);

                // Check if retransmission time is lower than the current lowest one.
                if (messageMetadata.GetTransmissionTime() - now < nextDelta)
                {
                    nextDelta = messageMetadata.GetTransmissionTime() - now;
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
                    messageMetadata.RemoveFrom(*message);
                    mIp6.EnqueueDatagram(*message);
                }
                else
                {
                    // Stop retransmitting if the number of timer expirations is already exceeded.
                    message->Free();
                }
            }
        }

        message = nextMessage;
    }

    if (nextDelta != 0xffffffff)
    {
        mRetransmissionTimer.Start(nextDelta);
    }
}

void Mpl::HandleSeedSetTimer(void *aContext)
{
    static_cast<Mpl *>(aContext)->HandleSeedSetTimer();
}

void Mpl::HandleSeedSetTimer()
{
    bool startTimer = false;

    for (int i = 0; i < kNumSeedEntries; i++)
    {
        if (mSeedSet[i].GetLifetime() > 0)
        {
            mSeedSet[i].SetLifetime(mSeedSet[i].GetLifetime() - 1);
            startTimer = true;
        }
    }

    if (startTimer)
    {
        mSeedSetTimer.Start(kSeedEntryLifetimeDt);
    }
}

}  // namespace Ip6
}  // namespace Thread
