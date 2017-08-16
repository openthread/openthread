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

#include <openthread/config.h>

#include "joiner.hpp"

#include <stdio.h>

#include <openthread/platform/radio.h>
#include <openthread/platform/random.h>

#include "common/code_utils.hpp"
#include "common/crc16.hpp"
#include "common/debug.hpp"
#include "common/encoding.hpp"
#include "common/logging.hpp"
#include "mac/mac_frame.hpp"
#include "meshcop/meshcop.hpp"
#include "thread/thread_netif.hpp"
#include "thread/thread_uri_paths.hpp"

#if OPENTHREAD_ENABLE_JOINER

using ot::Encoding::BigEndian::HostSwap16;
using ot::Encoding::BigEndian::HostSwap64;

namespace ot {
namespace MeshCoP {

Joiner::Joiner(ThreadNetif &aNetif):
    ThreadNetifLocator(aNetif),
    mState(kStateIdle),
    mCallback(NULL),
    mContext(NULL),
    mCcitt(0),
    mAnsi(0),
    mJoinerRouterChannel(0),
    mJoinerRouterPanId(0),
    mJoinerUdpPort(0),
    mVendorName(NULL),
    mVendorModel(NULL),
    mVendorSwVersion(NULL),
    mVendorData(NULL),
    mTimer(aNetif.GetInstance(), &Joiner::HandleTimer, this),
    mJoinerEntrust(OT_URI_PATH_JOINER_ENTRUST, &Joiner::HandleJoinerEntrust, this)
{
    aNetif.GetCoap().AddResource(mJoinerEntrust);
}

otError Joiner::Start(const char *aPSKd, const char *aProvisioningUrl,
                      const char *aVendorName, const char *aVendorModel, const char *aVendorSwVersion,
                      const char *aVendorData, otJoinerCallback aCallback, void *aContext)
{
    ThreadNetif &netif = GetNetif();
    otError error;
    Mac::ExtAddress extAddress;
    Crc16 ccitt(Crc16::kCcitt);
    Crc16 ansi(Crc16::kAnsi);

    otLogFuncEntry();

    VerifyOrExit(mState == kStateIdle, error = OT_ERROR_BUSY);

    // use extended address based on factory-assigned IEEE EUI-64
    netif.GetMac().GetHashMacAddress(&extAddress);
    netif.GetMac().SetExtAddress(extAddress);
    netif.GetMle().UpdateLinkLocalAddress();

    for (size_t i = 0; i < sizeof(extAddress); i++)
    {
        ccitt.Update(extAddress.m8[i]);
        ansi.Update(extAddress.m8[i]);
    }

    mCcitt = ccitt.Get();
    mAnsi = ansi.Get();

    error = netif.GetCoapSecure().Start(OPENTHREAD_CONFIG_JOINER_UDP_PORT);
    SuccessOrExit(error);

    error = netif.GetCoapSecure().GetDtls().SetPsk(reinterpret_cast<const uint8_t *>(aPSKd),
                                                   static_cast<uint8_t>(strlen(aPSKd)));
    SuccessOrExit(error);

    error = netif.GetCoapSecure().GetDtls().mProvisioningUrl.SetProvisioningUrl(aProvisioningUrl);
    SuccessOrExit(error);

    mJoinerRouterPanId = Mac::kPanIdBroadcast;
    SuccessOrExit(error = netif.GetMle().Discover(0, netif.GetMac().GetPanId(), true, false, HandleDiscoverResult,
                                                  this));

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

otError Joiner::Stop(void)
{
    otLogFuncEntry();

    Close();

    otLogFuncExit();
    return OT_ERROR_NONE;
}

void Joiner::Close(void)
{
    ThreadNetif &netif = GetNetif();
    otLogFuncEntry();

    netif.GetCoapSecure().Disconnect();
    netif.GetIp6Filter().RemoveUnsecurePort(netif.GetCoapSecure().GetPort());

    otLogFuncExit();
}

void Joiner::Complete(otError aError)
{
    mState = kStateIdle;

    GetNetif().GetCoapSecure().Stop();

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
    ThreadNetif &netif = GetNetif();
    Ip6::MessageInfo messageInfo;

    otLogFuncEntry();

    if (aResult != NULL)
    {
        otLogFuncEntryMsg("aResult = %llX", HostSwap64(*reinterpret_cast<uint64_t *>(&aResult->mExtAddress)));

        // Joining is disabled if the Steering Data is not included
        if (aResult->mSteeringData.mLength == 0)
        {
            otLogDebgMeshCoP(GetInstance(), "No steering data, joining disabled");
            ExitNow();
        }

        SteeringDataTlv steeringData;
        steeringData.SetLength(aResult->mSteeringData.mLength);
        memcpy(steeringData.GetValue(), aResult->mSteeringData.m8, steeringData.GetLength());

        if (steeringData.DoesAllowAny() ||
            (steeringData.GetBit(mCcitt % steeringData.GetNumBits()) &&
             steeringData.GetBit(mAnsi % steeringData.GetNumBits())))
        {
            mJoinerUdpPort = aResult->mJoinerUdpPort;
            mJoinerRouterPanId = aResult->mPanId;
            mJoinerRouterChannel = aResult->mChannel;
            memcpy(&mJoinerRouter, &aResult->mExtAddress, sizeof(mJoinerRouter));
        }
        else
        {
            otLogDebgMeshCoP(GetInstance(), "Steering data does not include this device");
        }
    }
    else if (mJoinerRouterPanId != Mac::kPanIdBroadcast)
    {
        otLogFuncEntryMsg("aResult = NULL");

        netif.GetMac().SetPanId(mJoinerRouterPanId);
        netif.GetMac().SetChannel(mJoinerRouterChannel);
        netif.GetIp6Filter().AddUnsecurePort(netif.GetCoapSecure().GetPort());

        messageInfo.GetPeerAddr().mFields.m16[0] = HostSwap16(0xfe80);
        messageInfo.GetPeerAddr().SetIid(mJoinerRouter);
        messageInfo.mPeerPort = mJoinerUdpPort;
        messageInfo.mInterfaceId = OT_NETIF_INTERFACE_ID_THREAD;

        netif.GetCoapSecure().Connect(messageInfo, Joiner::HandleSecureCoapClientConnect, this);
        mState = kStateConnect;
    }
    else
    {
        otLogDebgMeshCoP(GetInstance(), "No joinable network found");
        Complete(OT_ERROR_NOT_FOUND);
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
            Complete(OT_ERROR_SECURITY);
        }

        break;

    default:
        break;
    }
}

void Joiner::SendJoinerFinalize(void)
{
    ThreadNetif &netif = GetNetif();
    Coap::Header header;
    otError error = OT_ERROR_NONE;
    Message *message = NULL;
    StateTlv stateTlv;
    VendorNameTlv vendorNameTlv;
    VendorModelTlv vendorModelTlv;
    VendorSwVersionTlv vendorSwVersionTlv;
    VendorStackVersionTlv vendorStackVersionTlv;

    otLogFuncEntry();

    header.Init(OT_COAP_TYPE_CONFIRMABLE, OT_COAP_CODE_POST);
    header.AppendUriPathOptions(OT_URI_PATH_JOINER_FINALIZE);
    header.SetPayloadMarker();

    VerifyOrExit((message = NewMeshCoPMessage(netif.GetCoapSecure(), header)) != NULL, error = OT_ERROR_NO_BUFS);

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

    if (netif.GetCoapSecure().GetDtls().mProvisioningUrl.GetLength() > 0)
    {
        SuccessOrExit(error = message->Append(&netif.GetCoapSecure().GetDtls().mProvisioningUrl,
                                              netif.GetCoapSecure().GetDtls().mProvisioningUrl.GetSize()));
    }

#if OPENTHREAD_ENABLE_CERT_LOG
    uint8_t buf[OPENTHREAD_CONFIG_MESSAGE_BUFFER_SIZE];
    VerifyOrExit(message->GetLength() <= sizeof(buf));
    message->Read(header.GetLength(), message->GetLength() - header.GetLength(), buf);
    otDumpCertMeshCoP(GetInstance(), "[THCI] direction=send | type=JOIN_FIN.req |", buf,
                      message->GetLength() - header.GetLength());
#endif

    SuccessOrExit(error = netif.GetCoapSecure().SendMessage(*message, Joiner::HandleJoinerFinalizeResponse, this));

    otLogInfoMeshCoP(GetInstance(), "Sent joiner finalize");

exit:

    if (error != OT_ERROR_NONE && message != NULL)
    {
        message->Free();
    }

    otLogFuncExit();
}

void Joiner::HandleJoinerFinalizeResponse(void *aContext, otCoapHeader *aHeader, otMessage *aMessage,
                                          const otMessageInfo *aMessageInfo, otError aResult)
{
    static_cast<Joiner *>(aContext)->HandleJoinerFinalizeResponse(
        static_cast<Coap::Header *>(aHeader), static_cast<Message *>(aMessage),
        static_cast<const Ip6::MessageInfo *>(aMessageInfo), aResult);
}

void Joiner::HandleJoinerFinalizeResponse(Coap::Header *aHeader, Message *aMessage,
                                          const Ip6::MessageInfo *aMessageInfo, otError aResult)
{
    (void) aMessageInfo;
    StateTlv state;

    otLogFuncEntry();

    VerifyOrExit(mState == kStateConnected &&
                 aResult == OT_ERROR_NONE &&
                 aHeader->GetType() == OT_COAP_TYPE_ACKNOWLEDGMENT &&
                 aHeader->GetCode() == OT_COAP_CODE_CHANGED);

    SuccessOrExit(Tlv::GetTlv(*aMessage, Tlv::kState, sizeof(state), state));
    VerifyOrExit(state.IsValid());

    mState = kStateEntrust;
    mTimer.Start(kTimeout);

    otLogInfoMeshCoP(GetInstance(), "received joiner finalize response %d", static_cast<uint8_t>(state.GetState()));
#if OPENTHREAD_ENABLE_CERT_LOG
    uint8_t buf[OPENTHREAD_CONFIG_MESSAGE_BUFFER_SIZE];
    VerifyOrExit(aMessage->GetLength() <= sizeof(buf));
    aMessage->Read(aHeader->GetLength(), aMessage->GetLength() - aHeader->GetLength(), buf);
    otDumpCertMeshCoP(GetInstance(), "[THCI] direction=recv | type=JOIN_FIN.rsp |", buf,
                      aMessage->GetLength() - aHeader->GetLength());
#endif

exit:
    Close();
    otLogFuncExit();
}

void Joiner::HandleJoinerEntrust(void *aContext, otCoapHeader *aHeader, otMessage *aMessage,
                                 const otMessageInfo *aMessageInfo)
{
    static_cast<Joiner *>(aContext)->HandleJoinerEntrust(
        *static_cast<Coap::Header *>(aHeader), *static_cast<Message *>(aMessage),
        *static_cast<const Ip6::MessageInfo *>(aMessageInfo));
}

void Joiner::HandleJoinerEntrust(Coap::Header &aHeader, Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    ThreadNetif &netif = GetNetif();
    otError error;
    NetworkMasterKeyTlv masterKey;
    MeshLocalPrefixTlv meshLocalPrefix;
    ExtendedPanIdTlv extendedPanId;
    NetworkNameTlv networkName;
    ActiveTimestampTlv activeTimestamp;
    NetworkKeySequenceTlv networkKeySeq;

    otLogFuncEntry();

    VerifyOrExit(mState == kStateEntrust &&
                 aHeader.GetType() == OT_COAP_TYPE_CONFIRMABLE &&
                 aHeader.GetCode() == OT_COAP_CODE_POST, error = OT_ERROR_DROP);

    otLogInfoMeshCoP(GetInstance(), "Received joiner entrust");
    otLogCertMeshCoP(GetInstance(), "[THCI] direction=recv | type=JOIN_ENT.ntf");

    SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kNetworkMasterKey, sizeof(masterKey), masterKey));
    VerifyOrExit(masterKey.IsValid(), error = OT_ERROR_PARSE);

    SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kMeshLocalPrefix, sizeof(meshLocalPrefix), meshLocalPrefix));
    VerifyOrExit(meshLocalPrefix.IsValid(), error = OT_ERROR_PARSE);

    SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kExtendedPanId, sizeof(extendedPanId), extendedPanId));
    VerifyOrExit(extendedPanId.IsValid(), error = OT_ERROR_PARSE);

    SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kNetworkName, sizeof(networkName), networkName));
    VerifyOrExit(networkName.IsValid(), error = OT_ERROR_PARSE);

    SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kActiveTimestamp, sizeof(activeTimestamp), activeTimestamp));
    VerifyOrExit(activeTimestamp.IsValid(), error = OT_ERROR_PARSE);

    SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kNetworkKeySequence, sizeof(networkKeySeq), networkKeySeq));
    VerifyOrExit(networkKeySeq.IsValid(), error = OT_ERROR_PARSE);

    netif.GetKeyManager().SetMasterKey(masterKey.GetNetworkMasterKey());
    netif.GetKeyManager().SetCurrentKeySequence(networkKeySeq.GetNetworkKeySequence());
    netif.GetMle().SetMeshLocalPrefix(meshLocalPrefix.GetMeshLocalPrefix());
    netif.GetMac().SetExtendedPanId(extendedPanId.GetExtendedPanId());

    {
        otNetworkName name;
        memcpy(name.m8, networkName.GetNetworkName(), networkName.GetLength());
        name.m8[networkName.GetLength()] = '\0';
        netif.GetMac().SetNetworkName(name.m8);
    }

    otLogInfoMeshCoP(GetInstance(), "join success!");

    // Send dummy response.
    SendJoinerEntrustResponse(aHeader, aMessageInfo);

    // Delay extended address configuration to allow DTLS wrap up.
    mTimer.Start(kConfigExtAddressDelay);

exit:

    if (error != OT_ERROR_NONE)
    {
        otLogWarnMeshCoP(GetInstance(), "Error while processing joiner entrust: %s",
                         otThreadErrorToString(error));
    }

    otLogFuncExit();
}

void Joiner::SendJoinerEntrustResponse(const Coap::Header &aRequestHeader,
                                       const Ip6::MessageInfo &aRequestInfo)
{
    ThreadNetif &netif = GetNetif();
    otError error = OT_ERROR_NONE;
    Message *message;
    Coap::Header responseHeader;
    Ip6::MessageInfo responseInfo(aRequestInfo);

    otLogFuncEntry();

    responseHeader.SetDefaultResponseHeader(aRequestHeader);

    VerifyOrExit((message = NewMeshCoPMessage(netif.GetCoap(), responseHeader)) != NULL, error = OT_ERROR_NO_BUFS);
    message->SetSubType(Message::kSubTypeJoinerEntrust);

    memset(&responseInfo.mSockAddr, 0, sizeof(responseInfo.mSockAddr));
    SuccessOrExit(error = netif.GetCoap().SendMessage(*message, responseInfo));

    mState = kStateJoined;

    otLogInfoArp(GetInstance(), "Sent Joiner Entrust response");

    otLogInfoMeshCoP(GetInstance(), "Sent joiner entrust response length = %d", message->GetLength());
    otLogCertMeshCoP(GetInstance(), "[THCI] direction=send | type=JOIN_ENT.rsp");

exit:

    if (error != OT_ERROR_NONE && message != NULL)
    {
        message->Free();
    }

    otLogFuncExit();
}

void Joiner::HandleTimer(Timer &aTimer)
{
    GetOwner(aTimer).HandleTimer();
}

void Joiner::HandleTimer(void)
{
    ThreadNetif &netif = GetNetif();
    otError error = OT_ERROR_NONE;

    switch (mState)
    {
    case kStateIdle:
    case kStateDiscover:
    case kStateConnect:
        assert(false);
        break;

    case kStateConnected:
    case kStateEntrust:
        error = OT_ERROR_RESPONSE_TIMEOUT;
        break;

    case kStateJoined:
        Mac::ExtAddress extAddress;

        netif.GetMac().GenerateExtAddress(&extAddress);
        netif.GetMac().SetExtAddress(extAddress);
        netif.GetMle().UpdateLinkLocalAddress();

        error = OT_ERROR_NONE;
        break;

    }

    Complete(error);
}

Joiner &Joiner::GetOwner(const Context &aContext)
{
#if OPENTHREAD_ENABLE_MULTIPLE_INSTANCES
    Joiner &joiner = *static_cast<Joiner *>(aContext.GetContext());
#else
    Joiner &joiner = otGetThreadNetif().GetJoiner();
    OT_UNUSED_VARIABLE(aContext);
#endif
    return joiner;
}

}  // namespace MeshCoP
}  // namespace ot

#endif // OPENTHREAD_ENABLE_JOINER
