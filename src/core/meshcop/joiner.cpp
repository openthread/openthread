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
 *   This file implements the Joiner role.
 */

#define WPP_NAME "joiner.tmh"

#ifdef OPENTHREAD_CONFIG_FILE
#include OPENTHREAD_CONFIG_FILE
#else
#include <openthread-config.h>
#endif

#include <stdio.h>

#include <common/code_utils.hpp>
#include <common/crc16.hpp>
#include <common/encoding.hpp>
#include <common/logging.hpp>
#include <mac/mac_frame.hpp>
#include <meshcop/joiner.hpp>
#include <platform/radio.h>
#include <platform/random.h>
#include <thread/thread_netif.hpp>
#include <thread/thread_uris.hpp>

using Thread::Encoding::BigEndian::HostSwap16;
using Thread::Encoding::BigEndian::HostSwap64;

namespace Thread {
namespace MeshCoP {

Joiner::Joiner(ThreadNetif &aNetif):
    mTransmitMessage(NULL),
    mSocket(aNetif.GetIp6().mUdp),
    mTransmitTask(aNetif.GetIp6().mTaskletScheduler, &Joiner::HandleUdpTransmit, this),
    mJoinerEntrust(OPENTHREAD_URI_JOINER_ENTRUST, &Joiner::HandleJoinerEntrust, this),
    mNetif(aNetif)
{
    mNetif.GetCoapServer().AddResource(mJoinerEntrust);
}

ThreadError Joiner::Start(const char *aPSKd, const char *aProvisioningUrl)
{
    otLogFuncEntry();
    ThreadError error;
    Mac::ExtAddress extAddress;

    // use extended address based on factory-assigned IEEE EUI-64
    mNetif.GetMac().GetHashMacAddress(&extAddress);
    mNetif.GetMac().SetExtAddress(extAddress);
    mNetif.GetMle().UpdateLinkLocalAddress();

    SuccessOrExit(error = mNetif.GetDtls().SetPsk(reinterpret_cast<const uint8_t *>(aPSKd),
                                                  static_cast<uint8_t>(strlen(aPSKd))));
    SuccessOrExit(error = mNetif.GetDtls().mProvisioningUrl.SetProvisioningUrl(aProvisioningUrl));

    mJoinerRouterPanId = Mac::kPanIdBroadcast;
    SuccessOrExit(error = mNetif.GetMle().Discover(0, 0, mNetif.GetMac().GetPanId(), HandleDiscoverResult, this));

exit:
    otLogFuncExitErr(error);
    return error;
}

ThreadError Joiner::Stop(void)
{
    otLogFuncEntry();
    mNetif.GetIp6Filter().RemoveUnsecurePort(mSocket.GetSockName().mPort);
    mSocket.Close();
    mNetif.GetDtls().Stop();
    otLogFuncExit();
    return kThreadError_None;
}

void Joiner::HandleDiscoverResult(otActiveScanResult *aResult, void *aContext)
{
    static_cast<Joiner *>(aContext)->HandleDiscoverResult(aResult);
}

void Joiner::HandleDiscoverResult(otActiveScanResult *aResult)
{
    if (aResult != NULL)
    {
        otLogFuncEntryMsg("aResult = %llX", HostSwap64(*(uint64_t *)&aResult->mExtAddress));

        SteeringDataTlv steeringData;
        Mac::ExtAddress extAddress;
        Crc16 ccitt(Crc16::kCcitt);
        Crc16 ansi(Crc16::kAnsi);

        mNetif.GetMac().GetHashMacAddress(&extAddress);

        for (size_t i = 0; i < sizeof(extAddress); i++)
        {
            ccitt.Update(extAddress.m8[i]);
            ansi.Update(extAddress.m8[i]);
        }

        steeringData.SetLength(aResult->mSteeringData.mLength);
        memcpy(steeringData.GetValue(), aResult->mSteeringData.m8, steeringData.GetLength());

        if (steeringData.GetBit(ccitt.Get() % steeringData.GetNumBits()) &&
            steeringData.GetBit(ansi.Get() % steeringData.GetNumBits()))
        {
            mJoinerUdpPort = aResult->mJoinerUdpPort;
            mJoinerRouterPanId = aResult->mPanId;
            mJoinerRouterChannel = aResult->mChannel;
            memcpy(&mJoinerRouter, &aResult->mExtAddress, sizeof(mJoinerRouter));
        }
        else
        {
            otLogDebgMeshCoP("Steering data not set\n");
        }
    }
    else if (mJoinerRouterPanId != Mac::kPanIdBroadcast)
    {
        otLogFuncEntryMsg("aResult = NULL");

        // open UDP port
        Ip6::SockAddr sockaddr;
        sockaddr.mPort = mJoinerUdpPort;
        mSocket.Open(&Joiner::HandleUdpReceive, this);
        mSocket.Bind(sockaddr);

        mNetif.GetMac().SetPanId(mJoinerRouterPanId);
        mNetif.GetMac().SetChannel(mJoinerRouterChannel);
        mNetif.GetIp6Filter().AddUnsecurePort(sockaddr.mPort);

        mNetif.GetDtls().Start(true, HandleDtlsReceive, HandleDtlsSend, this);
    }

    otLogFuncExit();
}

ThreadError Joiner::HandleDtlsSend(void *aContext, const uint8_t *aBuf, uint16_t aLength)
{
    otLogInfoMeshCoP("Joiner::HandleDtlsTransmit\n");
    return static_cast<Joiner *>(aContext)->HandleDtlsSend(aBuf, aLength);
}

ThreadError Joiner::HandleDtlsSend(const unsigned char *aBuf, uint16_t aLength)
{
    otLogFuncEntry();
    ThreadError error = kThreadError_None;

    if (mTransmitMessage == NULL)
    {
        VerifyOrExit((mTransmitMessage = mSocket.NewMessage(0)) != NULL, error = kThreadError_NoBufs);
        mTransmitMessage->SetLinkSecurityEnabled(false);
    }

    VerifyOrExit(mTransmitMessage->Append(aBuf, aLength) == kThreadError_None, error = kThreadError_NoBufs);

    mTransmitTask.Post();

exit:

    if (error != kThreadError_None && mTransmitMessage != NULL)
    {
        mTransmitMessage->Free();
    }

    otLogFuncExitErr(error);
    return error;
}

void Joiner::HandleDtlsReceive(void *aContext, uint8_t *aBuf, uint16_t aLength)
{
    otLogInfoMeshCoP("Joiner::HandleDtlsReceive\n");
    static_cast<Joiner *>(aContext)->HandleDtlsReceive(aBuf, aLength);
}

void Joiner::HandleDtlsReceive(uint8_t *aBuf, uint16_t aLength)
{
    ReceiveJoinerFinalizeResponse(aBuf, aLength);
}

void Joiner::HandleUdpReceive(void *aContext, otMessage aMessage, const otMessageInfo *aMessageInfo)
{
    otLogInfoMeshCoP("Joiner::HandleUdpReceive\n");
    static_cast<Joiner *>(aContext)->HandleUdpReceive(*static_cast<Message *>(aMessage),
                                                      *static_cast<const Ip6::MessageInfo *>(aMessageInfo));
}

void Joiner::HandleUdpReceive(Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    otLogFuncEntry();
    (void)aMessageInfo;

    mNetif.GetDtls().Receive(aMessage, aMessage.GetOffset(), aMessage.GetLength() - aMessage.GetOffset());

    if (mNetif.GetDtls().IsConnected())
    {
        SendJoinerFinalize();
    }

    otLogFuncExit();
}

void Joiner::HandleUdpTransmit(void *aContext)
{
    otLogInfoMeshCoP("Joiner::HandleUdpTransmit\n");
    static_cast<Joiner *>(aContext)->HandleUdpTransmit();
}

void Joiner::HandleUdpTransmit(void)
{
    ThreadError error = kThreadError_None;
    Ip6::MessageInfo messageInfo;

    otLogFuncEntry();

    VerifyOrExit(mTransmitMessage != NULL, error = kThreadError_NoBufs);

    otLogInfoMeshCoP("transmit %d (to %llX)\n", mTransmitMessage->GetLength(), HostSwap64(*(uint64_t *)&mJoinerRouter));

    memset(&messageInfo, 0, sizeof(messageInfo));
    messageInfo.GetPeerAddr().mFields.m16[0] = HostSwap16(0xfe80);
    messageInfo.GetPeerAddr().SetIid(mJoinerRouter);
    messageInfo.mPeerPort = mJoinerUdpPort;
    messageInfo.mInterfaceId = 1;

    SuccessOrExit(error = mSocket.SendTo(*mTransmitMessage, messageInfo));

exit:

    if (error != kThreadError_None && mTransmitMessage != NULL)
    {
        mTransmitMessage->Free();
    }

    mTransmitMessage = NULL;
    otLogFuncExit();
}

void Joiner::SendJoinerFinalize(void)
{
    Coap::Header header;
    StateTlv *stateTlv;
    uint8_t length;
    uint8_t buf[128];
    uint8_t *cur = buf;

    otLogFuncEntry();

    header.Init();
    header.SetType(Coap::Header::kTypeConfirmable);
    header.SetCode(Coap::Header::kCodePost);
    header.SetMessageId(0);
    header.SetToken(NULL, 0);
    header.AppendUriPathOptions(OPENTHREAD_URI_JOINER_FINALIZE);
    header.Finalize();
    memcpy(cur, header.GetBytes(), header.GetLength());
    cur += header.GetLength();

    stateTlv = reinterpret_cast<MeshCoP::StateTlv *>(cur);
    stateTlv->Init();
    stateTlv->SetState(MeshCoP::StateTlv::kAccept);
    cur += sizeof(*stateTlv);

    length = mNetif.GetDtls().mProvisioningUrl.GetLength();

    if (length > 0)
    {
        length += sizeof(Tlv);
        memcpy(cur, &mNetif.GetDtls().mProvisioningUrl, length);
        cur += length;
    }

    mNetif.GetDtls().Send(buf, static_cast<uint16_t>(cur - buf));

    otLogInfoMeshCoP("Sent joiner finalize\n");
    otDumpCertMeshCoP("[THCI] direction=send | type=JOIN_FIN.req |", buf + header.GetLength(),
                      cur - buf - header.GetLength());
    otLogFuncExit();
}

void Joiner::ReceiveJoinerFinalizeResponse(uint8_t *buf, uint16_t length)
{
    Message *message = NULL;
    Coap::Header header;
    StateTlv state;

    otLogFuncEntry();

    VerifyOrExit((message = mNetif.GetIp6().mMessagePool.New(Message::kTypeIp6, 0)) != NULL, ;);
    SuccessOrExit(message->Append(buf, length));
    SuccessOrExit(header.FromMessage(*message));
    SuccessOrExit(message->SetOffset(header.GetLength()));

    VerifyOrExit(header.GetType() == Coap::Header::kTypeAcknowledgment &&
                 header.GetCode() == Coap::Header::kCodeChanged &&
                 header.GetMessageId() == 0 &&
                 header.GetTokenLength() == 0, ;);

    SuccessOrExit(Tlv::GetTlv(*message, Tlv::kState, sizeof(state), state));
    VerifyOrExit(state.IsValid(), ;);

    otLogInfoMeshCoP("received joiner finalize response %d\n", static_cast<uint8_t>(state.GetState()));
    otLogCertMeshCoP("[THCI] direction=recv | type=JOIN_FIN.rsp\n");

    Close();

exit:

    if (message != NULL)
    {
        message->Free();
    }

    otLogFuncExit();
}

void Joiner::HandleJoinerEntrust(void *aContext, Coap::Header &aHeader,
                                 Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    static_cast<Joiner *>(aContext)->HandleJoinerEntrust(aHeader, aMessage, aMessageInfo);
}

void Joiner::HandleJoinerEntrust(Coap::Header &aHeader, Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    ThreadError error;

    NetworkMasterKeyTlv masterKey;
    MeshLocalPrefixTlv meshLocalPrefix;
    ExtendedPanIdTlv extendedPanId;
    NetworkNameTlv networkName;
    ActiveTimestampTlv activeTimestamp;
    Mac::ExtAddress extAddress;

    otLogFuncEntry();

    VerifyOrExit(aHeader.GetType() == Coap::Header::kTypeConfirmable &&
                 aHeader.GetCode() == Coap::Header::kCodePost, error = kThreadError_Drop);

    otLogInfoMeshCoP("Received joiner entrust\n");
    otLogCertMeshCoP("[THCI] direction=recv | type=JOIN_ENT.ntf\n");

    SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kNetworkMasterKey, sizeof(masterKey), masterKey));
    VerifyOrExit(masterKey.IsValid(), error = kThreadError_Parse);

    SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kMeshLocalPrefix, sizeof(meshLocalPrefix), meshLocalPrefix));
    VerifyOrExit(meshLocalPrefix.IsValid(), error = kThreadError_Parse);

    SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kExtendedPanId, sizeof(extendedPanId), extendedPanId));
    VerifyOrExit(extendedPanId.IsValid(), error = kThreadError_Parse);

    SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kNetworkName, sizeof(networkName), networkName));
    VerifyOrExit(networkName.IsValid(), error = kThreadError_Parse);

    SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kActiveTimestamp, sizeof(activeTimestamp), activeTimestamp));
    VerifyOrExit(activeTimestamp.IsValid(), error = kThreadError_Parse);

    mNetif.GetKeyManager().SetMasterKey(masterKey.GetNetworkMasterKey(), masterKey.GetLength());
    mNetif.GetMle().SetMeshLocalPrefix(meshLocalPrefix.GetMeshLocalPrefix());
    mNetif.GetMac().SetExtendedPanId(extendedPanId.GetExtendedPanId());
    mNetif.GetMac().SetNetworkName(networkName.GetNetworkName());

    otLogInfoMeshCoP("join success!\n");

    // configure a random Extended Address
    for (size_t i = 0; i < sizeof(extAddress); i++)
    {
        extAddress.m8[i] = static_cast<uint8_t>(otPlatRandomGet());
    }

    mNetif.GetMac().SetExtAddress(extAddress);
    mNetif.GetMle().UpdateLinkLocalAddress();

exit:
    (void)aMessageInfo;
    otLogFuncExit();
}

void Joiner::Close(void)
{
    otLogFuncEntry();
    mNetif.GetDtls().Stop();
    otLogFuncExit();
}

}  // namespace Dtls
}  // namespace Thread
