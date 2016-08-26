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
 *   This file implements common methods for manipulating Thread Network Data.
 */

#include <coap/coap_header.hpp>
#include <common/code_utils.hpp>
#include <common/debug.hpp>
#include <common/logging.hpp>
#include <mac/mac_frame.hpp>
#include <platform/random.h>
#include <thread/network_data.hpp>
#include <thread/thread_netif.hpp>
#include <thread/thread_tlvs.hpp>
#include <thread/thread_uris.hpp>

namespace Thread {
namespace NetworkData {

NetworkData::NetworkData(ThreadNetif &aThreadNetif):
    mMle(aThreadNetif.GetMle())
{
    mLength = 0;
    mCoapMessageId = 0;
}

void NetworkData::GetNetworkData(bool aStable, uint8_t *aData, uint8_t &aDataLength)
{
    assert(aData != NULL);

    memcpy(aData, mTlvs, mLength);
    aDataLength = mLength;

    if (aStable)
    {
        RemoveTemporaryData(aData, aDataLength);
    }
}

ThreadError NetworkData::GetNextOnMeshPrefix(otNetworkDataIterator *aIterator, otBorderRouterConfig *aConfig)
{
    ThreadError error = kThreadError_NotFound;
    NetworkDataTlv *cur = reinterpret_cast<NetworkDataTlv *>(mTlvs + *aIterator);
    NetworkDataTlv *end = reinterpret_cast<NetworkDataTlv *>(mTlvs + mLength);

    for (; cur < end; cur = cur->GetNext())
    {
        PrefixTlv *prefix;
        BorderRouterTlv *borderRouter;
        BorderRouterEntry *borderRouterEntry;

        if (cur->GetType() != NetworkDataTlv::kTypePrefix)
        {
            continue;
        }

        prefix = static_cast<PrefixTlv *>(cur);

        if ((borderRouter = FindBorderRouter(*prefix)) == NULL)
        {
            continue;
        }

        borderRouterEntry = borderRouter->GetEntry(0);

        memcpy(&aConfig->mPrefix.mPrefix, prefix->GetPrefix(), sizeof(aConfig->mPrefix.mPrefix));
        aConfig->mPrefix.mLength = prefix->GetPrefixLength();
        aConfig->mPreference = borderRouterEntry->GetPreference();
        aConfig->mPreferred = borderRouterEntry->IsPreferred();
        aConfig->mSlaac = borderRouterEntry->IsSlaac();
        aConfig->mDhcp = borderRouterEntry->IsDhcp();
        aConfig->mConfigure = borderRouterEntry->IsConfigure();
        aConfig->mDefaultRoute = borderRouterEntry->IsDefaultRoute();
        aConfig->mOnMesh = borderRouterEntry->IsOnMesh();
        aConfig->mStable = cur->IsStable();

        *aIterator = static_cast<otNetworkDataIterator>(reinterpret_cast<uint8_t *>(cur->GetNext()) - mTlvs);

        ExitNow(error = kThreadError_None);
    }

exit:
    return error;
}

void NetworkData::RemoveTemporaryData(uint8_t *aData, uint8_t &aDataLength)
{
    NetworkDataTlv *cur = reinterpret_cast<NetworkDataTlv *>(aData);
    NetworkDataTlv *end;
    PrefixTlv *prefix;
    uint8_t length;
    uint8_t *dst;
    uint8_t *src;

    while (1)
    {
        end = reinterpret_cast<NetworkDataTlv *>(aData + aDataLength);

        if (cur >= end)
        {
            break;
        }

        switch (cur->GetType())
        {
        case NetworkDataTlv::kTypePrefix:
        {
            prefix = reinterpret_cast<PrefixTlv *>(cur);
            RemoveTemporaryData(aData, aDataLength, *prefix);

            if (prefix->GetSubTlvsLength() == 0)
            {
                length = sizeof(NetworkDataTlv) + cur->GetLength();
                dst = reinterpret_cast<uint8_t *>(cur);
                src = reinterpret_cast<uint8_t *>(cur->GetNext());
                memmove(dst, src, aDataLength - static_cast<size_t>(src - aData));
                aDataLength -= length;
                continue;
            }

            otDumpDebgNetData("remove prefix done", mTlvs, mLength);
            break;
        }

        default:
        {
            // remove temporary tlv
            if (!cur->IsStable())
            {
                length = sizeof(NetworkDataTlv) + cur->GetLength();
                dst = reinterpret_cast<uint8_t *>(cur);
                src = reinterpret_cast<uint8_t *>(cur->GetNext());
                memmove(dst, src, aDataLength - static_cast<size_t>(src - aData));
                aDataLength -= length;
                continue;
            }

            break;
        }
        }

        cur = cur->GetNext();
    }

    otDumpDebgNetData("remove done", aData, aDataLength);
}

void NetworkData::RemoveTemporaryData(uint8_t *aData, uint8_t &aDataLength, PrefixTlv &aPrefix)
{
    NetworkDataTlv *cur = reinterpret_cast<NetworkDataTlv *>(aPrefix.GetSubTlvs());
    NetworkDataTlv *end;
    BorderRouterTlv *borderRouter;
    HasRouteTlv *hasRoute;
    ContextTlv *context;
    BorderRouterEntry *borderRouterEntry;
    HasRouteEntry *hasRouteEntry;
    uint8_t length;
    uint8_t contextId;
    uint8_t *dst;
    uint8_t *src;

    while (1)
    {
        end = reinterpret_cast<NetworkDataTlv *>(aPrefix.GetSubTlvs() + aPrefix.GetSubTlvsLength());

        if (cur >= end)
        {
            break;
        }

        if (cur->IsStable())
        {
            switch (cur->GetType())
            {
            case NetworkDataTlv::kTypeBorderRouter:
            {
                borderRouter = FindBorderRouter(aPrefix);

                if ((context = FindContext(aPrefix)) == NULL)
                {
                    break;
                }

                contextId = context->GetContextId();

                // replace p_border_router_16
                for (uint8_t i = 0; i < borderRouter->GetNumEntries(); i++)
                {
                    borderRouterEntry = borderRouter->GetEntry(i);

                    if (borderRouterEntry->IsDhcp() || borderRouterEntry->IsConfigure())
                    {
                        borderRouterEntry->SetRloc(0xfc00 | contextId);
                    }
                    else
                    {
                        borderRouterEntry->SetRloc(0xfffe);
                    }
                }

                break;
            }

            case NetworkDataTlv::kTypeHasRoute:
            {
                hasRoute = FindHasRoute(aPrefix);

                // replace r_border_router_16
                for (uint8_t j = 0; j < hasRoute->GetNumEntries(); j++)
                {
                    hasRouteEntry = hasRoute->GetEntry(j);
                    hasRouteEntry->SetRloc(0xfffe);
                }

                break;
            }

            default:
            {
                break;
            }
            }

            // keep stable tlv
            cur = cur->GetNext();
        }
        else
        {
            // remove temporary tlv
            length = sizeof(NetworkDataTlv) + cur->GetLength();
            dst = reinterpret_cast<uint8_t *>(cur);
            src = reinterpret_cast<uint8_t *>(cur->GetNext());
            memmove(dst, src, aDataLength - static_cast<size_t>(src - aData));
            aPrefix.SetSubTlvsLength(aPrefix.GetSubTlvsLength() - length);
            aDataLength -= length;
        }
    }
}

BorderRouterTlv *NetworkData::FindBorderRouter(PrefixTlv &aPrefix)
{
    BorderRouterTlv *rval = NULL;
    NetworkDataTlv *cur = reinterpret_cast<NetworkDataTlv *>(aPrefix.GetSubTlvs());
    NetworkDataTlv *end = reinterpret_cast<NetworkDataTlv *>(aPrefix.GetSubTlvs() + aPrefix.GetSubTlvsLength());

    while (cur < end)
    {
        if (cur->GetType() == NetworkDataTlv::kTypeBorderRouter)
        {
            ExitNow(rval = reinterpret_cast<BorderRouterTlv *>(cur));
        }

        cur = cur->GetNext();
    }

exit:
    return rval;
}

BorderRouterTlv *NetworkData::FindBorderRouter(PrefixTlv &aPrefix, bool aStable)
{
    BorderRouterTlv *rval = NULL;
    NetworkDataTlv *cur = reinterpret_cast<NetworkDataTlv *>(aPrefix.GetSubTlvs());
    NetworkDataTlv *end = reinterpret_cast<NetworkDataTlv *>(aPrefix.GetSubTlvs() + aPrefix.GetSubTlvsLength());

    while (cur < end)
    {
        if (cur->GetType() == NetworkDataTlv::kTypeBorderRouter &&
            cur->IsStable() == aStable)
        {
            ExitNow(rval = reinterpret_cast<BorderRouterTlv *>(cur));
        }

        cur = cur->GetNext();
    }

exit:
    return rval;
}

HasRouteTlv *NetworkData::FindHasRoute(PrefixTlv &aPrefix)
{
    HasRouteTlv *rval = NULL;
    NetworkDataTlv *cur = reinterpret_cast<NetworkDataTlv *>(aPrefix.GetSubTlvs());
    NetworkDataTlv *end = reinterpret_cast<NetworkDataTlv *>(aPrefix.GetSubTlvs() + aPrefix.GetSubTlvsLength());

    while (cur < end)
    {
        if (cur->GetType() == NetworkDataTlv::kTypeHasRoute)
        {
            ExitNow(rval = reinterpret_cast<HasRouteTlv *>(cur));
        }

        cur = cur->GetNext();
    }

exit:
    return rval;
}

HasRouteTlv *NetworkData::FindHasRoute(PrefixTlv &aPrefix, bool aStable)
{
    HasRouteTlv *rval = NULL;
    NetworkDataTlv *cur = reinterpret_cast<NetworkDataTlv *>(aPrefix.GetSubTlvs());
    NetworkDataTlv *end = reinterpret_cast<NetworkDataTlv *>(aPrefix.GetSubTlvs() + aPrefix.GetSubTlvsLength());

    while (cur < end)
    {
        if (cur->GetType() == NetworkDataTlv::kTypeHasRoute &&
            cur->IsStable() == aStable)
        {
            ExitNow(rval = reinterpret_cast<HasRouteTlv *>(cur));
        }

        cur = cur->GetNext();
    }

exit:
    return rval;
}

ContextTlv *NetworkData::FindContext(PrefixTlv &aPrefix)
{
    ContextTlv *rval = NULL;
    NetworkDataTlv *cur = reinterpret_cast<NetworkDataTlv *>(aPrefix.GetSubTlvs());
    NetworkDataTlv *end = reinterpret_cast<NetworkDataTlv *>(aPrefix.GetSubTlvs() + aPrefix.GetSubTlvsLength());

    while (cur < end)
    {
        if (cur->GetType() == NetworkDataTlv::kTypeContext)
        {
            ExitNow(rval = reinterpret_cast<ContextTlv *>(cur));
        }

        cur = cur->GetNext();
    }

exit:
    return rval;
}

PrefixTlv *NetworkData::FindPrefix(const uint8_t *aPrefix, uint8_t aPrefixLength)
{
    return FindPrefix(aPrefix, aPrefixLength, mTlvs, mLength);
}

PrefixTlv *NetworkData::FindPrefix(const uint8_t *aPrefix, uint8_t aPrefixLength, uint8_t *aTlvs, uint8_t aTlvsLength)
{
    NetworkDataTlv *cur = reinterpret_cast<NetworkDataTlv *>(aTlvs);
    NetworkDataTlv *end = reinterpret_cast<NetworkDataTlv *>(aTlvs + aTlvsLength);
    PrefixTlv *compare;

    while (cur < end)
    {
        if (cur->GetType() == NetworkDataTlv::kTypePrefix)
        {
            compare = reinterpret_cast<PrefixTlv *>(cur);

            if (compare->GetPrefixLength() == aPrefixLength &&
                PrefixMatch(compare->GetPrefix(), aPrefix, aPrefixLength) >= aPrefixLength)
            {
                return compare;
            }
        }

        cur = cur->GetNext();
    }

    return NULL;
}

int8_t NetworkData::PrefixMatch(const uint8_t *a, const uint8_t *b, uint8_t aLength)
{
    int8_t rval = 0;
    uint8_t bytes = BitVectorBytes(aLength);
    uint8_t diff;

    for (uint8_t i = 0; i < bytes; i++)
    {
        diff = a[i] ^ b[i];

        if (diff == 0)
        {
            rval += 8;
        }
        else
        {
            while ((diff & 0x80) == 0)
            {
                rval++;
                diff <<= 1;
            }

            break;
        }
    }

    return (rval >= aLength) ? rval : -1;
}

ThreadError NetworkData::Insert(uint8_t *aStart, uint8_t aLength)
{
    assert(aLength + mLength <= sizeof(mTlvs) &&
           mTlvs <= aStart &&
           aStart <= mTlvs + mLength);
    memmove(aStart + aLength, aStart, mLength - static_cast<size_t>(aStart - mTlvs));
    mLength += aLength;
    return kThreadError_None;
}

ThreadError NetworkData::Remove(uint8_t *aStart, uint8_t aLength)
{
    assert(aLength <= mLength &&
           mTlvs <= aStart &&
           (aStart - mTlvs) + aLength <= mLength);
    memmove(aStart, aStart + aLength, mLength - (static_cast<size_t>(aStart - mTlvs) + aLength));
    mLength -= aLength;
    return kThreadError_None;
}

ThreadError NetworkData::SendServerDataNotification(bool aLocal, uint16_t aRloc16)
{
    ThreadError error = kThreadError_None;
    Coap::Header header;
    Message *message;
    Ip6::MessageInfo messageInfo;

    mSocket.Open(&HandleUdpReceive, this);

    for (size_t i = 0; i < sizeof(mCoapToken); i++)
    {
        mCoapToken[i] = static_cast<uint8_t>(otPlatRandomGet());
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

    VerifyOrExit((message = Ip6::Udp::NewMessage(0)) != NULL, error = kThreadError_NoBufs);
    SuccessOrExit(error = message->Append(header.GetBytes(), header.GetLength()));

    if (aLocal)
    {
        ThreadTlv tlv;
        tlv.SetType(ThreadTlv::kThreadNetworkData);
        tlv.SetLength(mLength);
        SuccessOrExit(error = message->Append(&tlv, sizeof(tlv)));
        SuccessOrExit(error = message->Append(mTlvs, mLength));
    }

    if (aRloc16 != Mac::kShortAddrInvalid)
    {
        ThreadRloc16Tlv rloc16Tlv;
        rloc16Tlv.Init();
        rloc16Tlv.SetRloc16(aRloc16);
        SuccessOrExit(error = message->Append(&rloc16Tlv, sizeof(rloc16Tlv)));
    }

    memset(&messageInfo, 0, sizeof(messageInfo));
    mMle.GetLeaderAddress(messageInfo.GetPeerAddr());
    messageInfo.mPeerPort = kCoapUdpPort;
    SuccessOrExit(error = mSocket.SendTo(*message, messageInfo));

    otLogInfoNetData("Sent server data notification\n");

exit:

    if (error != kThreadError_None && message != NULL)
    {
        Message::Free(*message);
    }

    return error;
}

void NetworkData::HandleUdpReceive(void *aContext, otMessage aMessage, const otMessageInfo *aMessageInfo)
{
    Local *obj = static_cast<Local *>(aContext);
    obj->HandleUdpReceive(*static_cast<Message *>(aMessage), *static_cast<const Ip6::MessageInfo *>(aMessageInfo));
}

void NetworkData::HandleUdpReceive(Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    Coap::Header header;

    SuccessOrExit(header.FromMessage(aMessage));
    VerifyOrExit(header.GetType() == Coap::Header::kTypeAcknowledgment &&
                 header.GetCode() == Coap::Header::kCodeChanged &&
                 header.GetMessageId() == mCoapMessageId &&
                 header.GetTokenLength() == sizeof(mCoapToken) &&
                 memcmp(mCoapToken, header.GetToken(), sizeof(mCoapToken)) == 0, ;);

    otLogInfoNetData("Server data notification acknowledged\n");

exit:
    (void)aMessageInfo;
}

}  // namespace NetworkData
}  // namespace Thread
