/*
 *  Copyright (c) 2021, The OpenThread Authors.
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
 *   This file implements the ping sender module.
 */

#include "ping_sender.hpp"

#if OPENTHREAD_CONFIG_PING_SENDER_ENABLE

#include "common/encoding.hpp"
#include "common/locator-getters.hpp"
#include "common/random.hpp"

namespace ot {
namespace Utils {

using Encoding::BigEndian::HostSwap32;

void PingSender::Config::SetUnspecifiedToDefault(void)
{
    if (mSize == 0)
    {
        mSize = kDefaultSize;
    }

    if (mCount == 0)
    {
        mCount = kDefaultCount;
    }

    if (mInterval == 0)
    {
        mInterval = kDefaultInterval;
    }
}

void PingSender::Config::InvokeCallback(const Reply &aReply) const
{
    VerifyOrExit(mCallback != nullptr);
    mCallback(&aReply, mCallbackContext);

exit:
    return;
}

PingSender::PingSender(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mIdentifier(0)
    , mTimer(aInstance, PingSender::HandleTimer)
    , mIcmpHandler(PingSender::HandleIcmpReceive, this)
{
    IgnoreError(Get<Ip6::Icmp>().RegisterHandler(mIcmpHandler));
}

Error PingSender::Ping(const Config &aConfig)
{
    Error error = kErrorNone;

    VerifyOrExit(!mTimer.IsRunning(), error = kErrorBusy);

    mConfig = aConfig;
    mConfig.SetUnspecifiedToDefault();
    VerifyOrExit(mConfig.mInterval <= Timer::kMaxDelay, error = kErrorInvalidArgs);

    mIdentifier++;
    SendPing();

exit:
    return error;
}

void PingSender::Stop(void)
{
    mTimer.Stop();
    mIdentifier++;
}

void PingSender::SendPing(void)
{
    TimeMilli        now     = TimerMilli::GetNow();
    Message *        message = nullptr;
    Ip6::MessageInfo messageInfo;

    messageInfo.SetPeerAddr(mConfig.GetDestination());
    messageInfo.mHopLimit          = mConfig.mHopLimit;
    messageInfo.mAllowZeroHopLimit = mConfig.mAllowZeroHopLimit;

    message = Get<Ip6::Icmp>().NewMessage(0);
    VerifyOrExit(message != nullptr);

    SuccessOrExit(message->Append(HostSwap32(now.GetValue())));

    if (mConfig.mSize > message->GetLength())
    {
        SuccessOrExit(message->SetLength(mConfig.mSize));
    }

    SuccessOrExit(Get<Ip6::Icmp>().SendEchoRequest(*message, messageInfo, mIdentifier));

#if OPENTHREAD_CONFIG_OTNS_ENABLE
    Get<Utils::Otns>().EmitPingRequest(mConfig.GetDestination(), mConfig.mSize, now.GetValue(), mConfig.mHopLimit);
#endif

    message = nullptr;

exit:
    FreeMessage(message);
    mConfig.mCount--;

    if (mConfig.mCount != 0)
    {
        mTimer.Start(mConfig.mInterval);
    }
}

void PingSender::HandleTimer(Timer &aTimer)
{
    aTimer.Get<PingSender>().HandleTimer();
}

void PingSender::HandleTimer(void)
{
    SendPing();
}

void PingSender::HandleIcmpReceive(void *               aContext,
                                   otMessage *          aMessage,
                                   const otMessageInfo *aMessageInfo,
                                   const otIcmp6Header *aIcmpHeader)
{
    reinterpret_cast<PingSender *>(aContext)->HandleIcmpReceive(*static_cast<Message *>(aMessage),
                                                                *static_cast<const Ip6::MessageInfo *>(aMessageInfo),
                                                                *static_cast<const Ip6::Icmp::Header *>(aIcmpHeader));
}

void PingSender::HandleIcmpReceive(const Message &          aMessage,
                                   const Ip6::MessageInfo & aMessageInfo,
                                   const Ip6::Icmp::Header &aIcmpHeader)
{
    Reply    reply;
    uint32_t timestamp;

    VerifyOrExit(aIcmpHeader.GetType() == Ip6::Icmp::Header::kTypeEchoReply);
    VerifyOrExit(aIcmpHeader.GetId() == mIdentifier);

    SuccessOrExit(aMessage.Read(aMessage.GetOffset(), timestamp));
    timestamp = HostSwap32(timestamp);

    reply.mSenderAddress  = aMessageInfo.GetPeerAddr();
    reply.mRoundTripTime  = TimerMilli::GetNow() - TimeMilli(timestamp);
    reply.mSize           = aMessage.GetLength() - aMessage.GetOffset();
    reply.mSequenceNumber = aIcmpHeader.GetSequence();
    reply.mHopLimit       = aMessageInfo.GetHopLimit();

#if OPENTHREAD_CONFIG_OTNS_ENABLE
    Get<Utils::Otns>().EmitPingReply(aMessageInfo.GetPeerAddr(), reply.mSize, timestamp, reply.mHopLimit);
#endif

    mConfig.InvokeCallback(reply);

exit:
    return;
}

} // namespace Utils
} // namespace ot

#endif // #if OPENTHREAD_CONFIG_PING_SENDER_ENABLE
