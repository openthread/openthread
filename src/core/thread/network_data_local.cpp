/*
 *  Copyright (c) 2016, Nest Labs, Inc.
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
 *   This file implements the local Thread Network Data.
 */

#include <coap/coap_header.hpp>
#include <common/debug.hpp>
#include <common/logging.hpp>
#include <common/code_utils.hpp>
#include <platform/random.h>
#include <thread/network_data_local.hpp>
#include <thread/thread_netif.hpp>
#include <thread/thread_tlvs.hpp>
#include <thread/thread_uris.hpp>

#ifdef WINDOWS_LOGGING
#include <network_data_local.tmh>
#endif

namespace Thread {
namespace NetworkData {

Local::Local(ThreadNetif &aThreadNetif):
    NetworkData(),
    mMle(aThreadNetif.GetMle())
{
    mCoapMessageId = 0;
}

ThreadError Local::AddOnMeshPrefix(const uint8_t *aPrefix, uint8_t aPrefixLength, int8_t aPrf,
                                   uint8_t aFlags, bool aStable)
{
    PrefixTlv *prefixTlv;
    BorderRouterTlv *brTlv;

    RemoveOnMeshPrefix(aPrefix, aPrefixLength);

    prefixTlv = reinterpret_cast<PrefixTlv *>(mTlvs + mLength);
    Insert(reinterpret_cast<uint8_t *>(prefixTlv),
           sizeof(PrefixTlv) + BitVectorBytes(aPrefixLength) + sizeof(BorderRouterTlv) + sizeof(BorderRouterEntry));
    prefixTlv->Init(0, aPrefixLength, aPrefix);
    prefixTlv->SetSubTlvsLength(sizeof(BorderRouterTlv) + sizeof(BorderRouterEntry));

    brTlv = reinterpret_cast<BorderRouterTlv *>(prefixTlv->GetSubTlvs());
    brTlv->Init();
    brTlv->SetLength(brTlv->GetLength() + sizeof(BorderRouterEntry));
    brTlv->GetEntry(0)->Init();
    brTlv->GetEntry(0)->SetPreference(aPrf);
    brTlv->GetEntry(0)->SetFlags(aFlags);

    if (aStable)
    {
        prefixTlv->SetStable();
        brTlv->SetStable();
    }

    otDumpDebgNetData("add prefix done", mTlvs, mLength);
    return kThreadError_None;
}

ThreadError Local::RemoveOnMeshPrefix(const uint8_t *aPrefix, uint8_t aPrefixLength)
{
    ThreadError error = kThreadError_None;
    PrefixTlv *tlv;

    VerifyOrExit((tlv = FindPrefix(aPrefix, aPrefixLength)) != NULL, error = kThreadError_Error);
    VerifyOrExit(FindBorderRouter(*tlv) != NULL, error = kThreadError_Error);
    Remove(reinterpret_cast<uint8_t *>(tlv), sizeof(NetworkDataTlv) + tlv->GetLength());

exit:
    otDumpDebgNetData("remove done", mTlvs, mLength);
    return error;
}

ThreadError Local::AddHasRoutePrefix(const uint8_t *aPrefix, uint8_t aPrefixLength, int8_t aPrf, bool aStable)
{
    PrefixTlv *prefixTlv;
    HasRouteTlv *hasRouteTlv;

    RemoveHasRoutePrefix(aPrefix, aPrefixLength);

    prefixTlv = reinterpret_cast<PrefixTlv *>(mTlvs + mLength);
    Insert(reinterpret_cast<uint8_t *>(prefixTlv),
           sizeof(PrefixTlv) + BitVectorBytes(aPrefixLength) + sizeof(HasRouteTlv) + sizeof(HasRouteEntry));
    prefixTlv->Init(0, aPrefixLength, aPrefix);
    prefixTlv->SetSubTlvsLength(sizeof(HasRouteTlv) + sizeof(HasRouteEntry));

    hasRouteTlv = reinterpret_cast<HasRouteTlv *>(prefixTlv->GetSubTlvs());
    hasRouteTlv->Init();
    hasRouteTlv->SetLength(hasRouteTlv->GetLength() + sizeof(HasRouteEntry));
    hasRouteTlv->GetEntry(0)->Init();
    hasRouteTlv->GetEntry(0)->SetPreference(aPrf);

    if (aStable)
    {
        prefixTlv->SetStable();
        hasRouteTlv->SetStable();
    }

    otDumpDebgNetData("add route done", mTlvs, mLength);
    return kThreadError_None;
}

ThreadError Local::RemoveHasRoutePrefix(const uint8_t *aPrefix, uint8_t aPrefixLength)
{
    ThreadError error = kThreadError_None;
    PrefixTlv *tlv;

    VerifyOrExit((tlv = FindPrefix(aPrefix, aPrefixLength)) != NULL, error = kThreadError_Error);
    VerifyOrExit(FindHasRoute(*tlv) != NULL, error = kThreadError_Error);
    Remove(reinterpret_cast<uint8_t *>(tlv), sizeof(NetworkDataTlv) + tlv->GetLength());

exit:
    otDumpDebgNetData("remove done", mTlvs, mLength);
    return error;
}

ThreadError Local::UpdateRloc(void)
{
    for (NetworkDataTlv *cur = reinterpret_cast<NetworkDataTlv *>(mTlvs);
         cur < reinterpret_cast<NetworkDataTlv *>(mTlvs + mLength);
         cur = cur->GetNext())
    {
        switch (cur->GetType())
        {
        case NetworkDataTlv::kTypePrefix:
            UpdateRloc(*reinterpret_cast<PrefixTlv *>(cur));
            break;

        default:
            assert(false);
            break;
        }
    }

    return kThreadError_None;
}

ThreadError Local::UpdateRloc(PrefixTlv &aPrefix)
{
    for (NetworkDataTlv *cur = reinterpret_cast<NetworkDataTlv *>(aPrefix.GetSubTlvs());
         cur < reinterpret_cast<NetworkDataTlv *>(aPrefix.GetSubTlvs() + aPrefix.GetSubTlvsLength());
         cur = cur->GetNext())
    {
        switch (cur->GetType())
        {
        case NetworkDataTlv::kTypeHasRoute:
            UpdateRloc(*reinterpret_cast<HasRouteTlv *>(cur));
            break;

        case NetworkDataTlv::kTypeBorderRouter:
            UpdateRloc(*reinterpret_cast<BorderRouterTlv *>(cur));
            break;

        default:
            assert(false);
            break;
        }
    }

    return kThreadError_None;
}

ThreadError Local::UpdateRloc(HasRouteTlv &aHasRoute)
{
    HasRouteEntry *entry = aHasRoute.GetEntry(0);
    entry->SetRloc(mMle.GetRloc16());
    return kThreadError_None;
}

ThreadError Local::UpdateRloc(BorderRouterTlv &aBorderRouter)
{
    BorderRouterEntry *entry = aBorderRouter.GetEntry(0);
    entry->SetRloc(mMle.GetRloc16());
    return kThreadError_None;
}

ThreadError Local::Register(const Ip6::Address &aDestination)
{
    ThreadError error = kThreadError_None;
    Coap::Header header;
    Message *message;
    Ip6::MessageInfo messageInfo;

    UpdateRloc();
    mSocket.Open(mMle.GetOpenThreadContext(), &Local::HandleUdpReceive, this);

    for (size_t i = 0; i < sizeof(mCoapToken); i++)
    {
        mCoapToken[i] = (uint8_t)otPlatRandomGet();
    }

    header.Init();
    header.SetVersion(1);
    header.SetType(Coap::Header::kTypeConfirmable);
    header.SetCode(Coap::Header::kCodePost);
    header.SetMessageId(++mCoapMessageId);
    header.SetToken(mCoapToken, sizeof(mCoapToken));
    header.AppendUriPathOptions(OPENTHREAD_URI_SERVER_DATA);
    header.AppendContentFormatOption(Coap::Header::kApplicationOctetStream);
    header.Finalize();

    VerifyOrExit((message = Ip6::Udp::NewMessage(mMle.GetOpenThreadContext(), 0)) != NULL, error = kThreadError_NoBufs);
    SuccessOrExit(error = message->Append(header.GetBytes(), header.GetLength()));
    SuccessOrExit(error = message->Append(mTlvs, mLength));

    memset(&messageInfo, 0, sizeof(messageInfo));
    memcpy(&messageInfo.mPeerAddr, &aDestination, sizeof(messageInfo.mPeerAddr));
    messageInfo.mPeerPort = kCoapUdpPort;
    SuccessOrExit(error = mSocket.SendTo(*message, messageInfo));

    otLogInfoNetData("Sent network data registration\n");

exit:

    if (error != kThreadError_None && message != NULL)
    {
        Message::Free(*message);
    }

    return error;
}

void Local::HandleUdpReceive(void *aContext, otMessage aMessage, const otMessageInfo *aMessageInfo)
{
    Local *obj = reinterpret_cast<Local *>(aContext);
    obj->HandleUdpReceive(*static_cast<Message *>(aMessage), *static_cast<const Ip6::MessageInfo *>(aMessageInfo));
}

void Local::HandleUdpReceive(Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    Coap::Header header;

    SuccessOrExit(header.FromMessage(aMessage));
    VerifyOrExit(header.GetType() == Coap::Header::kTypeAcknowledgment &&
                 header.GetCode() == Coap::Header::kCodeChanged &&
                 header.GetMessageId() == mCoapMessageId &&
                 header.GetTokenLength() == sizeof(mCoapToken) &&
                 memcmp(mCoapToken, header.GetToken(), sizeof(mCoapToken)) == 0, ;);

    otLogInfoNetData("Network data registration acknowledged\n");

exit:
    (void)aMessageInfo;
}

}  // namespace NetworkData
}  // namespace Thread

