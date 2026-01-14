/*
 *  Copyright (c) 2025, The OpenThread Authors.
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
 *   This file implements the History Tracker Client.
 */

#include "history_tracker_client.hpp"

#if OPENTHREAD_CONFIG_HISTORY_TRACKER_ENABLE && OPENTHREAD_CONFIG_HISTORY_TRACKER_CLIENT_ENABLE

#include "instance/instance.hpp"

namespace ot {
namespace HistoryTracker {

RegisterLogModule("HistoryClient");

Client::Client(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mActive(false)
    , mQueryRloc16(0)
    , mQueryId(0)
    , mAnswerIndex(0)
    , mTimer(aInstance)
{
}

void Client::CancelQuery(void)
{
    mActive = false;
    mTimer.Stop();
}

Error Client::QueryNetInfo(uint16_t        aRloc16,
                           uint16_t        aMaxEntries,
                           uint32_t        aMaxEntryAge,
                           NetInfoCallback aCallback,
                           void           *aContext)
{
    Error error = kErrorNone;

    VerifyOrExit(!mActive, error = kErrorBusy);

    mCallbacks.mNetInfo.Set(aCallback, aContext);
    error = SendQuery(Tlv::kNetworkInfo, aMaxEntries, aMaxEntryAge, aRloc16);

exit:
    return error;
}

Error Client::SendQuery(Tlv::Type aTlvType, uint16_t aMaxEntries, uint32_t aMaxEntryAge, uint16_t aRloc16)
{
    Error                   error = kErrorNone;
    OwnedPtr<Coap::Message> message;
    Tmf::MessageInfo        messageInfo(GetInstance());
    RequestTlv              requestTlv;

    VerifyOrExit(Get<Mle::Mle>().IsAttached(), error = kErrorInvalidState);

    message.Reset(Get<Tmf::Agent>().NewNonConfirmablePostMessage(kUriHistoryQuery));
    VerifyOrExit(message != nullptr, error = kErrorNoBufs);
    IgnoreError(message->SetPriority(Message::kPriorityLow));

    mQueryId++;
    SuccessOrExit(error = Tlv::Append<QueryIdTlv>(*message, mQueryId));

    requestTlv.Init(aTlvType, aMaxEntries, aMaxEntryAge);
    SuccessOrExit(error = message->Append(requestTlv));

    messageInfo.SetSockAddrToRloc();
    messageInfo.GetPeerAddr().SetToRoutingLocator(Get<Mle::Mle>().GetMeshLocalPrefix(), aRloc16);

    SuccessOrExit(error = Get<Tmf::Agent>().SendMessage(*message, messageInfo));
    message.Release();

    LogInfo("Sent %s for TLV %u to 0x%04x", UriToString<kUriHistoryQuery>(), aTlvType, aRloc16);

    mActive      = true;
    mTlvType     = aTlvType;
    mQueryRloc16 = aRloc16;
    mAnswerIndex = 0;
    mTimer.Start(kResponseTimeout);

exit:
    return error;
}

template <> void Client::HandleTmf<kUriHistoryAnswer>(Coap::Msg &aMsg)
{
    VerifyOrExit(aMsg.mMessage.IsConfirmablePostRequest());
    IgnoreError(Get<Tmf::Agent>().SendEmptyAck(aMsg));

    LogInfo("Received %s from %s", ot::UriToString<kUriHistoryAnswer>(),
            aMsg.mMessageInfo.GetPeerAddr().ToString().AsCString());

    SuccessOrExit(ProcessAnswer(aMsg));

    switch (mTlvType)
    {
    case Tlv::kNetworkInfo:
        ProcessNetInfoAnswer(aMsg.mMessage);
        break;
    default:
        ExitNow();
    }

exit:
    return;
}

Error Client::ProcessAnswer(const Coap::Msg &aMsg)
{
    Error     error = kErrorFailed;
    AnswerTlv answerTlv;
    uint16_t  queryId;

    VerifyOrExit(mActive);
    VerifyOrExit(Get<Mle::Mle>().IsRoutingLocator(aMsg.mMessageInfo.GetPeerAddr()));
    VerifyOrExit(aMsg.mMessageInfo.GetPeerAddr().GetIid().GetLocator() == mQueryRloc16);

    SuccessOrExit(Tlv::Find<QueryIdTlv>(aMsg.mMessage, queryId));
    VerifyOrExit(queryId == mQueryId);

    SuccessOrExit(Tlv::FindTlv(aMsg.mMessage, answerTlv));

    if (answerTlv.GetIndex() != mAnswerIndex)
    {
        Finalize(kErrorResponseTimeout);
        ExitNow();
    }

    mAnswerIndex++;
    error = kErrorNone;

exit:
    return error;
}

void Client::ProcessNetInfoAnswer(const Coap::Message &aMessage)
{
    Error          error = kErrorNone;
    OffsetRange    offsetRange;
    Tlv            tlv;
    NetworkInfoTlv netInfoTlv;
    NetworkInfo    netInfo;

    offsetRange.InitFromMessageOffsetToEnd(aMessage);

    for (; !offsetRange.IsEmpty(); offsetRange.AdvanceOffset(tlv.GetSize()))
    {
        SuccessOrExit(error = aMessage.Read(offsetRange, tlv));
        VerifyOrExit(offsetRange.Contains(tlv.GetSize()), error = kErrorParse);

        if (tlv.GetType() != Tlv::kNetworkInfo)
        {
            continue;
        }

        if (tlv.GetLength() == 0)
        {
            Finalize(kErrorNone);
            ExitNow();
        }

        SuccessOrExit(error = aMessage.Read(offsetRange, netInfoTlv));
        VerifyOrExit(netInfoTlv.IsValid(), error = kErrorParse);

        netInfoTlv.CopyTo(netInfo);

        mCallbacks.mNetInfo.InvokeIfSet(kErrorPending, &netInfo, netInfoTlv.GetEntryAge());
        VerifyOrExit(mActive);
    }

exit:
    if (error != kErrorNone)
    {
        Finalize(error);
    }
}

void Client::Finalize(Error aError)
{
    VerifyOrExit(mActive);

    CancelQuery();

    switch (mTlvType)
    {
    case Tlv::kNetworkInfo:
        mCallbacks.mNetInfo.InvokeIfSet(aError, nullptr, 0);
        break;
    default:
        break;
    }

exit:
    return;
}

void Client::HandleTimer(void) { Finalize(kErrorResponseTimeout); }

} // namespace HistoryTracker
} // namespace ot

#endif // #if  OPENTHREAD_CONFIG_HISTORY_TRACKER_ENABLE && OPENTHREAD_CONFIG_HISTORY_TRACKER_CLIENT_ENABLE
