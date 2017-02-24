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
#include <common/debug.hpp>
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
    mState(kStateIdle),
    mCallback(NULL),
    mContext(NULL),
    mCcitt(Crc16::kCcitt),
    mAnsi(Crc16::kAnsi),
    mJoinerRouterChannel(0),
    mJoinerRouterPanId(0),
    mJoinerUdpPort(0),
    mVendorName(NULL),
    mVendorModel(NULL),
    mVendorSwVersion(NULL),
    mVendorData(NULL),
    mTimer(aNetif.GetIp6().mTimerScheduler, &Joiner::HandleTimer, this),
    mJoinerEntrust(OPENTHREAD_URI_JOINER_ENTRUST, &Joiner::HandleJoinerEntrust, this),
    mNetif(aNetif)
{
    mNetif.GetCoapServer().AddResource(mJoinerEntrust);
}

ThreadError Joiner::Start(const char *aPSKd, const char *aProvisioningUrl,
                          const char *aVendorName, const char *aVendorModel, const char *aVendorSwVersion,
                          const char *aVendorData, otJoinerCallback aCallback, void *aContext)
{
    ThreadError error;
    Mac::ExtAddress extAddress;

    otLogFuncEntry();

    VerifyOrExit(mState == kStateIdle, error = kThreadError_Busy);

    // use extended address based on factory-assigned IEEE EUI-64
    mNetif.GetMac().GetHashMacAddress(&extAddress);
    mNetif.GetMac().SetExtAddress(extAddress);
    mNetif.GetMle().UpdateLinkLocalAddress();

    for (size_t i = 0; i < sizeof(extAddress); i++)
    {
        mCcitt.Update(extAddress.m8[i]);
        mAnsi.Update(extAddress.m8[i]);
    }

    error = mNetif.GetSecureCoapClient().GetDtls().SetPsk(reinterpret_cast<const uint8_t *>(aPSKd),
                                                          static_cast<uint8_t>(strlen(aPSKd)));
    SuccessOrExit(error);
    error = mNetif.GetSecureCoapClient().GetDtls().mProvisioningUrl.SetProvisioningUrl(aProvisioningUrl);
    SuccessOrExit(error);

    mJoinerRouterPanId = Mac::kPanIdBroadcast;
    SuccessOrExit(error = mNetif.GetMle().Discover(0, 0, mNetif.GetMac().GetPanId(), true, HandleDiscoverResult, this));

    mVendorName = aVendorName;
    mVendorModel = aVendorModel;
    mVendorSwVersion = aVendorSwVersion;
    mVendorData = aVendorData;
    mCallback = aCallback;
    mContext = aContext;
    mState = kStateDiscover;

exit:
    otLogFuncExitErr(error);
    return error;
}

ThreadError Joiner::Stop(void)
{
    otLogFuncEntry();

    Close();

    otLogFuncExit();
    return kThreadError_None;
}

void Joiner::Close(void)
{
    otLogFuncEntry();

    mNetif.GetSecureCoapClient().Disconnect();
    mNetif.GetIp6Filter().RemoveUnsecurePort(mNetif.GetSecureCoapClient().GetPort());

    otLogFuncExit();
}

void Joiner::Complete(ThreadError aError)
{
    mState = kStateIdle;

    if (mCallback)
    {
        otJoinerCallback callback = mCallback;
        mCallback = NULL;
        callback(aError, mContext);
    }
}

void Joiner::HandleDiscoverResult(otActiveScanResult *aResult, void *aContext)
{
    static_cast<Joiner *>(aContext)->HandleDiscoverResult(aResult);
}

void Joiner::HandleDiscoverResult(otActiveScanResult *aResult)
{
    Ip6::MessageInfo messageInfo;

    otLogFuncEntry();

    if (aResult != NULL)
    {
        otLogFuncEntryMsg("aResult = %llX", HostSwap64(*reinterpret_cast<uint64_t *>(&aResult->mExtAddress)));

        // Joining is disabled if the Steering Data is not included
        if (aResult->mSteeringData.mLength == 0)
        {
            otLogDebgMeshCoP("No steering data, joining disabled");
            ExitNow();
        }

        SteeringDataTlv steeringData;
        steeringData.SetLength(aResult->mSteeringData.mLength);
        memcpy(steeringData.GetValue(), aResult->mSteeringData.m8, steeringData.GetLength());

        if (steeringData.DoesAllowAny() ||
            (steeringData.GetBit(mCcitt.Get() % steeringData.GetNumBits()) &&
             steeringData.GetBit(mAnsi.Get() % steeringData.GetNumBits())))
        {
            mJoinerUdpPort = aResult->mJoinerUdpPort;
            mJoinerRouterPanId = aResult->mPanId;
            mJoinerRouterChannel = aResult->mChannel;
            memcpy(&mJoinerRouter, &aResult->mExtAddress, sizeof(mJoinerRouter));
        }
        else
        {
            otLogDebgMeshCoP("Steering data does not include this device");
        }
    }
    else if (mJoinerRouterPanId != Mac::kPanIdBroadcast)
    {
        otLogFuncEntryMsg("aResult = NULL");

        mNetif.GetMac().SetPanId(mJoinerRouterPanId);
        mNetif.GetMac().SetChannel(mJoinerRouterChannel);
        mNetif.GetIp6Filter().AddUnsecurePort(mNetif.GetSecureCoapClient().GetPort());

        messageInfo.GetPeerAddr().mFields.m16[0] = HostSwap16(0xfe80);
        messageInfo.GetPeerAddr().SetIid(mJoinerRouter);
        messageInfo.mPeerPort = mJoinerUdpPort;
        messageInfo.mInterfaceId = OT_NETIF_INTERFACE_ID_THREAD;

        mNetif.GetSecureCoapClient().Connect(messageInfo, Joiner::HandleSecureCoapClientConnect, this);
        mState = kStateConnect;
    }
    else
    {
        otLogDebgMeshCoP("No joinable network found");
        Complete(kThreadError_NotFound);
    }

exit:
    otLogFuncExit();
}

void Joiner::HandleSecureCoapClientConnect(bool aConnected, void *aContext)
{
    static_cast<Joiner *>(aContext)->HandleSecureCoapClientConnect(aConnected);
}

void Joiner::HandleSecureCoapClientConnect(bool aConnected)
{
    switch (mState)
    {
    case kStateConnect:
        if (aConnected)
        {
            mState = kStateConnected;
            SendJoinerFinalize();
            mTimer.Start(kTimeout);
        }
        else
        {
            Complete(kThreadError_Security);
        }

        break;

    default:
        break;
    }
}

void Joiner::SendJoinerFinalize(void)
{
    Coap::Header header;
    ThreadError error = kThreadError_None;
    Message *message = NULL;
    StateTlv stateTlv;
    VendorNameTlv vendorNameTlv;
    VendorModelTlv vendorModelTlv;
    VendorSwVersionTlv vendorSwVersionTlv;
    VendorStackVersionTlv vendorStackVersionTlv;

    otLogFuncEntry();

    header.Init(kCoapTypeConfirmable, kCoapRequestPost);
    header.AppendUriPathOptions(OPENTHREAD_URI_JOINER_FINALIZE);
    header.SetPayloadMarker();

    VerifyOrExit((message = mNetif.GetSecureCoapClient().NewMeshCoPMessage(header)) != NULL,
                 error = kThreadError_NoBufs);

    stateTlv.Init();
    stateTlv.SetState(MeshCoP::StateTlv::kAccept);
    SuccessOrExit(error = message->Append(&stateTlv, sizeof(stateTlv)));

    vendorNameTlv.Init();
    vendorNameTlv.SetVendorName(mVendorName);
    SuccessOrExit(error = message->Append(&vendorNameTlv, vendorNameTlv.GetSize()));

    vendorModelTlv.Init();
    vendorModelTlv.SetVendorModel(mVendorModel);
    SuccessOrExit(error = message->Append(&vendorModelTlv, vendorModelTlv.GetSize()));

    vendorSwVersionTlv.Init();
    vendorSwVersionTlv.SetVendorSwVersion(mVendorSwVersion);
    SuccessOrExit(error = message->Append(&vendorSwVersionTlv, vendorSwVersionTlv.GetSize()));

    vendorStackVersionTlv.Init();
    vendorStackVersionTlv.SetOui(OPENTHREAD_CONFIG_STACK_VENDOR_OUI);
    vendorStackVersionTlv.SetMajor(OPENTHREAD_CONFIG_STACK_VERSION_MAJOR);
    vendorStackVersionTlv.SetMinor(OPENTHREAD_CONFIG_STACK_VERSION_MINOR);
    vendorStackVersionTlv.SetRevision(OPENTHREAD_CONFIG_STACK_VERSION_REV);
    SuccessOrExit(error = message->Append(&vendorStackVersionTlv, vendorStackVersionTlv.GetSize()));

    if (mVendorData != NULL)
    {
        VendorDataTlv vendorDataTlv;
        vendorDataTlv.Init();
        vendorDataTlv.SetVendorData(mVendorData);
        SuccessOrExit(error = message->Append(&vendorDataTlv, vendorDataTlv.GetSize()));
    }

    if (mNetif.GetSecureCoapClient().GetDtls().mProvisioningUrl.GetLength() > 0)
    {
        SuccessOrExit(error = message->Append(&mNetif.GetSecureCoapClient().GetDtls().mProvisioningUrl,
                                              mNetif.GetSecureCoapClient().GetDtls().mProvisioningUrl.GetSize()));
    }

#if OPENTHREAD_ENABLE_CERT_LOG
    uint8_t buf[OPENTHREAD_CONFIG_MESSAGE_BUFFER_SIZE];
    VerifyOrExit(message->GetLength() <= sizeof(buf), ;);
    message->Read(header.GetLength(), message->GetLength() - header.GetLength(), buf);
    otDumpCertMeshCoP("[THCI] direction=send | type=JOIN_FIN.req |", buf, message->GetLength() - header.GetLength());
#endif

    mNetif.GetSecureCoapClient().SendMessage(*message, Joiner::HandleJoinerFinalizeResponse, this);

    otLogInfoMeshCoP("Sent joiner finalize");

exit:

    if (error != kThreadError_None && message != NULL)
    {
        message->Free();
    }

    otLogFuncExit();
}

void Joiner::HandleJoinerFinalizeResponse(void *aContext, otCoapHeader *aHeader, otMessage aMessage,
                                          const otMessageInfo *aMessageInfo, ThreadError aResult)
{
    static_cast<Joiner *>(aContext)->HandleJoinerFinalizeResponse(
        static_cast<Coap::Header *>(aHeader), static_cast<Message *>(aMessage),
        static_cast<const Ip6::MessageInfo *>(aMessageInfo), aResult);
}

void Joiner::HandleJoinerFinalizeResponse(Coap::Header *aHeader, Message *aMessage,
                                          const Ip6::MessageInfo *aMessageInfo, ThreadError aResult)
{
    (void) aMessageInfo;
    StateTlv state;

    otLogFuncEntry();

    VerifyOrExit(mState == kStateConnected &&
                 aResult == kThreadError_None &&
                 aHeader->GetType() == kCoapTypeAcknowledgment &&
                 aHeader->GetCode() == kCoapResponseChanged, ;);

    SuccessOrExit(Tlv::GetTlv(*aMessage, Tlv::kState, sizeof(state), state));
    VerifyOrExit(state.IsValid(), ;);

    mState = kStateEntrust;
    mTimer.Start(kTimeout);

    otLogInfoMeshCoP("received joiner finalize response %d", static_cast<uint8_t>(state.GetState()));
    otLogCertMeshCoP("[THCI] direction=recv | type=JOIN_FIN.rsp");

exit:
    Close();
    otLogFuncExit();
}

void Joiner::HandleJoinerEntrust(void *aContext, otCoapHeader *aHeader, otMessage aMessage,
                                 const otMessageInfo *aMessageInfo)
{
    static_cast<Joiner *>(aContext)->HandleJoinerEntrust(
        *static_cast<Coap::Header *>(aHeader), *static_cast<Message *>(aMessage),
        *static_cast<const Ip6::MessageInfo *>(aMessageInfo));
}

void Joiner::HandleJoinerEntrust(Coap::Header &aHeader, Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    ThreadError error;

    NetworkMasterKeyTlv masterKey;
    MeshLocalPrefixTlv meshLocalPrefix;
    ExtendedPanIdTlv extendedPanId;
    NetworkNameTlv networkName;
    ActiveTimestampTlv activeTimestamp;
    NetworkKeySequenceTlv networkKeySeq;

    otLogFuncEntry();

    VerifyOrExit(mState == kStateEntrust &&
                 aHeader.GetType() == kCoapTypeConfirmable &&
                 aHeader.GetCode() == kCoapRequestPost, error = kThreadError_Drop);

    otLogInfoMeshCoP("Received joiner entrust");
    otLogCertMeshCoP("[THCI] direction=recv | type=JOIN_ENT.ntf");

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

    SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kNetworkKeySequence, sizeof(networkKeySeq), networkKeySeq));
    VerifyOrExit(networkKeySeq.IsValid(), error = kThreadError_Parse);

    mNetif.GetKeyManager().SetMasterKey(masterKey.GetNetworkMasterKey(), masterKey.GetLength());
    mNetif.GetKeyManager().SetCurrentKeySequence(networkKeySeq.GetNetworkKeySequence());
    mNetif.GetMle().SetMeshLocalPrefix(meshLocalPrefix.GetMeshLocalPrefix());
    mNetif.GetMac().SetExtendedPanId(extendedPanId.GetExtendedPanId());
    mNetif.GetMac().SetNetworkName(networkName.GetNetworkName());

    otLogInfoMeshCoP("join success!");

    // Send dummy response.
    SendJoinerEntrustResponse(aHeader, aMessageInfo);

    // Delay extended address configuration to allow DTLS wrap up.
    mTimer.Start(kConfigExtAddressDelay);

exit:
    otLogFuncExit();
}

void Joiner::SendJoinerEntrustResponse(const Coap::Header &aRequestHeader,
                                       const Ip6::MessageInfo &aRequestInfo)
{
    ThreadError error = kThreadError_None;
    Message *message;
    Coap::Header responseHeader;
    Ip6::MessageInfo responseInfo(aRequestInfo);

    otLogFuncEntry();

    VerifyOrExit((message = mNetif.GetCoapServer().NewMeshCoPMessage(0)) != NULL, error = kThreadError_NoBufs);
    message->SetSubType(Message::kSubTypeJoinerEntrust);

    responseHeader.SetDefaultResponseHeader(aRequestHeader);

    SuccessOrExit(error = message->Append(responseHeader.GetBytes(), responseHeader.GetLength()));

    memset(&responseInfo.mSockAddr, 0, sizeof(responseInfo.mSockAddr));
    SuccessOrExit(error = mNetif.GetCoapServer().SendMessage(*message, responseInfo));

    mState = kStateJoined;

    otLogInfoArp("Sent Joiner Entrust response");

    otLogInfoMeshCoP("Sent joiner entrust response length = %d", message->GetLength());
    otLogCertMeshCoP("[THCI] direction=send | type=JOIN_ENT.rsp");

exit:

    if (error != kThreadError_None && message != NULL)
    {
        message->Free();
    }

    otLogFuncExit();
}

void Joiner::HandleTimer(void *aContext)
{
    static_cast<Joiner *>(aContext)->HandleTimer();
}

void Joiner::HandleTimer(void)
{
    ThreadError error = kThreadError_Error;

    switch (mState)
    {
    case kStateIdle:
    case kStateDiscover:
    case kStateConnect:
        assert(false);
        break;

    case kStateConnected:
    case kStateEntrust:
        error = kThreadError_ResponseTimeout;
        break;

    case kStateJoined:
        Mac::ExtAddress extAddress;

        for (size_t i = 0; i < sizeof(extAddress); i++)
        {
            extAddress.m8[i] = static_cast<uint8_t>(otPlatRandomGet());
        }

        mNetif.GetMac().SetExtAddress(extAddress);
        mNetif.GetMle().UpdateLinkLocalAddress();

        error = kThreadError_None;
        break;

    }

    Complete(error);
}

}  // namespace MeshCoP
}  // namespace Thread
