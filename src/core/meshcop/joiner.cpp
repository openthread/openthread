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

#include "joiner.hpp"

#include <stdio.h>

#include <openthread/platform/radio.h>
#include <openthread/platform/random.h>

#include "common/code_utils.hpp"
#include "common/crc16.hpp"
#include "common/debug.hpp"
#include "common/encoding.hpp"
#include "common/instance.hpp"
#include "common/logging.hpp"
#include "common/owner-locator.hpp"
#include "mac/mac_frame.hpp"
#include "meshcop/meshcop.hpp"
#include "thread/thread_netif.hpp"
#include "thread/thread_uri_paths.hpp"

#if OPENTHREAD_ENABLE_JOINER

using ot::Encoding::BigEndian::HostSwap16;

namespace ot {
namespace MeshCoP {

Joiner::Joiner(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mState(OT_JOINER_STATE_IDLE)
    , mCallback(NULL)
    , mContext(NULL)
    , mCcitt(0)
    , mAnsi(0)
    , mVendorName(NULL)
    , mVendorModel(NULL)
    , mVendorSwVersion(NULL)
    , mVendorData(NULL)
    , mTimer(aInstance, &Joiner::HandleTimer, this)
    , mJoinerEntrust(OT_URI_PATH_JOINER_ENTRUST, &Joiner::HandleJoinerEntrust, this)
{
    memset(mJoinerRouters, 0, sizeof(mJoinerRouters));
    GetNetif().GetCoap().AddResource(mJoinerEntrust);
}

void Joiner::GetJoinerId(Mac::ExtAddress &aJoinerId) const
{
    otPlatRadioGetIeeeEui64(&GetInstance(), aJoinerId.m8);
    ComputeJoinerId(aJoinerId, aJoinerId);
}

otError Joiner::Start(const char *     aPSKd,
                      const char *     aProvisioningUrl,
                      const char *     aVendorName,
                      const char *     aVendorModel,
                      const char *     aVendorSwVersion,
                      const char *     aVendorData,
                      otJoinerCallback aCallback,
                      void *           aContext)
{
    ThreadNetif &   netif = GetNetif();
    otError         error;
    Mac::ExtAddress joinerId;
    Crc16           ccitt(Crc16::kCcitt);
    Crc16           ansi(Crc16::kAnsi);

    VerifyOrExit(mState == OT_JOINER_STATE_IDLE, error = OT_ERROR_BUSY);

    GetNotifier().Signal(OT_CHANGED_JOINER_STATE);

    // use extended address based on factory-assigned IEEE EUI-64
    GetJoinerId(joinerId);
    netif.GetMac().SetExtAddress(joinerId);
    netif.GetMle().UpdateLinkLocalAddress();

    for (size_t i = 0; i < sizeof(joinerId); i++)
    {
        ccitt.Update(joinerId.m8[i]);
        ansi.Update(joinerId.m8[i]);
    }

    mCcitt = ccitt.Get();
    mAnsi  = ansi.Get();

    error = netif.GetCoapSecure().Start(OPENTHREAD_CONFIG_JOINER_UDP_PORT);
    SuccessOrExit(error);

    error = netif.GetCoapSecure().SetPsk(reinterpret_cast<const uint8_t *>(aPSKd), static_cast<uint8_t>(strlen(aPSKd)));
    SuccessOrExit(error);

    error = netif.GetCoapSecure().GetDtls().mProvisioningUrl.SetProvisioningUrl(aProvisioningUrl);
    SuccessOrExit(error);

    memset(mJoinerRouters, 0, sizeof(mJoinerRouters));

    SuccessOrExit(error =
                      netif.GetMle().Discover(0, netif.GetMac().GetPanId(), true, false, HandleDiscoverResult, this));

    mVendorName      = aVendorName;
    mVendorModel     = aVendorModel;
    mVendorSwVersion = aVendorSwVersion;
    mVendorData      = aVendorData;
    mCallback        = aCallback;
    mContext         = aContext;
    mState           = OT_JOINER_STATE_DISCOVER;

exit:
    return error;
}

otError Joiner::Stop(void)
{
    Close();

    return OT_ERROR_NONE;
}

otJoinerState Joiner::GetState(void) const
{
    return mState;
}

void Joiner::Close(void)
{
    ThreadNetif &netif = GetNetif();

    netif.GetCoapSecure().Disconnect();
    netif.GetIp6Filter().RemoveUnsecurePort(OPENTHREAD_CONFIG_JOINER_UDP_PORT);
}

void Joiner::Complete(otError aError)
{
    ThreadNetif &netif = GetNetif();
    mState             = OT_JOINER_STATE_IDLE;
    otError error      = OT_ERROR_NOT_FOUND;
    GetNotifier().Signal(OT_CHANGED_JOINER_STATE);

    netif.GetCoapSecure().Disconnect();

    if (aError != OT_ERROR_NONE && aError != OT_ERROR_NOT_FOUND)
    {
        error = TryNextJoin();
    }

    if (error == OT_ERROR_NOT_FOUND && mCallback)
    {
        netif.GetCoapSecure().Stop();
        otJoinerCallback callback = mCallback;
        mCallback                 = NULL;
        callback(aError, mContext);
    }
}

void Joiner::HandleDiscoverResult(otActiveScanResult *aResult, void *aContext)
{
    static_cast<Joiner *>(aContext)->HandleDiscoverResult(aResult);
}

void Joiner::HandleDiscoverResult(otActiveScanResult *aResult)
{
    if (aResult != NULL)
    {
        JoinerRouter joinerRouter;

        otLogDebgMeshCoP("Received Discovery Response (%s)",
                         static_cast<Mac::ExtAddress &>(aResult->mExtAddress).ToString().AsCString());

        // Joining is disabled if the Steering Data is not included
        if (aResult->mSteeringData.mLength == 0)
        {
            otLogDebgMeshCoP("No steering data, joining disabled");
            ExitNow();
        }

        SteeringDataTlv steeringData;
        steeringData.SetLength(aResult->mSteeringData.mLength);
        memcpy(steeringData.GetValue(), aResult->mSteeringData.m8, steeringData.GetLength());

        if (!steeringData.GetBit(mCcitt % steeringData.GetNumBits()) ||
            !steeringData.GetBit(mAnsi % steeringData.GetNumBits()))
        {
            otLogDebgMeshCoP("Steering data does not include this device");
            ExitNow();
        }

        joinerRouter.mPriority = static_cast<uint16_t>(aResult->mRssi) + 0x80;

        if (!steeringData.DoesAllowAny())
        {
            joinerRouter.mPriority += kSpecificPriorityBonus;
        }

        joinerRouter.mJoinerUdpPort = aResult->mJoinerUdpPort;
        joinerRouter.mPanId         = aResult->mPanId;
        joinerRouter.mChannel       = aResult->mChannel;
        joinerRouter.mExtAddr       = static_cast<Mac::ExtAddress &>(aResult->mExtAddress);
        AddJoinerRouter(joinerRouter);
    }
    else
    {
        otError error = TryNextJoin();

        if (error != OT_ERROR_NONE)
        {
            Complete(error);
        }
    }

exit:
    return;
}

void Joiner::AddJoinerRouter(JoinerRouter &aJoinerRouter)
{
    JoinerRouter *joinerRouter = &mJoinerRouters[0];

    // Find the lowest priority
    for (size_t i = 1; i < OPENTHREAD_CONFIG_MAX_JOINER_ROUTER_ENTRIES; i++)
    {
        if (mJoinerRouters[i].mPriority < joinerRouter->mPriority)
        {
            joinerRouter = &mJoinerRouters[i];
        }

        if (joinerRouter->mPriority == 0)
        {
            break;
        }
    }

    if (aJoinerRouter.mPriority > joinerRouter->mPriority)
    {
        memcpy(joinerRouter, &aJoinerRouter, sizeof(aJoinerRouter));
    }
}

otError Joiner::TryNextJoin()
{
    ThreadNetif & netif        = GetNetif();
    otError       error        = OT_ERROR_NOT_FOUND;
    JoinerRouter *joinerRouter = &mJoinerRouters[0];

    for (size_t i = 1; i < OPENTHREAD_CONFIG_MAX_JOINER_ROUTER_ENTRIES; i++)
    {
        if (mJoinerRouters[i].mPriority > joinerRouter->mPriority)
        {
            joinerRouter = &mJoinerRouters[i];
        }
    }

    if (joinerRouter->mPriority > 0)
    {
        Ip6::SockAddr sockaddr;

        joinerRouter->mPriority = 0;

        netif.GetMac().SetPanId(joinerRouter->mPanId);
        netif.GetMac().SetPanChannel(joinerRouter->mChannel);
        netif.GetIp6Filter().AddUnsecurePort(OPENTHREAD_CONFIG_JOINER_UDP_PORT);

        sockaddr.GetAddress().mFields.m16[0] = HostSwap16(0xfe80);
        sockaddr.GetAddress().SetIid(joinerRouter->mExtAddr);
        sockaddr.mPort    = joinerRouter->mJoinerUdpPort;
        sockaddr.mScopeId = OT_NETIF_INTERFACE_ID_THREAD;

        netif.GetCoapSecure().Connect(sockaddr, Joiner::HandleSecureCoapClientConnect, this);
        mState = OT_JOINER_STATE_CONNECT;
        error  = OT_ERROR_NONE;
    }
    else
    {
        otLogDebgMeshCoP("No joinable networks remaining to try");
    }

    return error;
}

void Joiner::HandleSecureCoapClientConnect(bool aConnected, void *aContext)
{
    static_cast<Joiner *>(aContext)->HandleSecureCoapClientConnect(aConnected);
}

void Joiner::HandleSecureCoapClientConnect(bool aConnected)
{
    switch (mState)
    {
    case OT_JOINER_STATE_CONNECT:
        if (aConnected)
        {
            mState = OT_JOINER_STATE_CONNECTED;
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
    ThreadNetif &         netif   = GetNetif();
    otError               error   = OT_ERROR_NONE;
    Coap::Message *       message = NULL;
    StateTlv              stateTlv;
    VendorNameTlv         vendorNameTlv;
    VendorModelTlv        vendorModelTlv;
    VendorSwVersionTlv    vendorSwVersionTlv;
    VendorStackVersionTlv vendorStackVersionTlv;

    VerifyOrExit((message = NewMeshCoPMessage(netif.GetCoapSecure())) != NULL, error = OT_ERROR_NO_BUFS);

    message->Init(OT_COAP_TYPE_CONFIRMABLE, OT_COAP_CODE_POST);
    message->AppendUriPathOptions(OT_URI_PATH_JOINER_FINALIZE);
    message->SetPayloadMarker();
    message->SetOffset(message->GetLength());

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
    message->Read(message->GetOffset(), message->GetLength() - message->GetOffset(), buf);
    otDumpCertMeshCoP("[THCI] direction=send | type=JOIN_FIN.req |", buf, message->GetLength() - message->GetOffset());
#endif

    SuccessOrExit(error = netif.GetCoapSecure().SendMessage(*message, Joiner::HandleJoinerFinalizeResponse, this));

    otLogInfoMeshCoP("Sent joiner finalize");

exit:

    if (error != OT_ERROR_NONE && message != NULL)
    {
        message->Free();
    }
}

void Joiner::HandleJoinerFinalizeResponse(void *               aContext,
                                          otMessage *          aMessage,
                                          const otMessageInfo *aMessageInfo,
                                          otError              aResult)
{
    static_cast<Joiner *>(aContext)->HandleJoinerFinalizeResponse(
        *static_cast<Coap::Message *>(aMessage), static_cast<const Ip6::MessageInfo *>(aMessageInfo), aResult);
}

void Joiner::HandleJoinerFinalizeResponse(Coap::Message &         aMessage,
                                          const Ip6::MessageInfo *aMessageInfo,
                                          otError                 aResult)
{
    OT_UNUSED_VARIABLE(aMessageInfo);

    StateTlv state;

    VerifyOrExit(mState == OT_JOINER_STATE_CONNECTED && aResult == OT_ERROR_NONE &&
                 aMessage.GetType() == OT_COAP_TYPE_ACKNOWLEDGMENT && aMessage.GetCode() == OT_COAP_CODE_CHANGED);

    SuccessOrExit(Tlv::GetTlv(aMessage, Tlv::kState, sizeof(state), state));
    VerifyOrExit(state.IsValid());

    mState = OT_JOINER_STATE_ENTRUST;
    mTimer.Start(kTimeout);

    otLogInfoMeshCoP("received joiner finalize response %d", static_cast<uint8_t>(state.GetState()));
#if OPENTHREAD_ENABLE_CERT_LOG
    uint8_t buf[OPENTHREAD_CONFIG_MESSAGE_BUFFER_SIZE];

    VerifyOrExit(aMessage.GetLength() <= sizeof(buf));
    aMessage.Read(aMessage.GetOffset(), aMessage.GetLength() - aMessage.GetOffset(), buf);
    otDumpCertMeshCoP("[THCI] direction=recv | type=JOIN_FIN.rsp |", buf, aMessage.GetLength() - aMessage.GetOffset());
#endif

exit:
    Close();
}

void Joiner::HandleJoinerEntrust(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
    static_cast<Joiner *>(aContext)->HandleJoinerEntrust(*static_cast<Coap::Message *>(aMessage),
                                                         *static_cast<const Ip6::MessageInfo *>(aMessageInfo));
}

void Joiner::HandleJoinerEntrust(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    ThreadNetif &         netif = GetNetif();
    otError               error;
    NetworkMasterKeyTlv   masterKey;
    MeshLocalPrefixTlv    meshLocalPrefix;
    ExtendedPanIdTlv      extendedPanId;
    NetworkNameTlv        networkName;
    ActiveTimestampTlv    activeTimestamp;
    NetworkKeySequenceTlv networkKeySeq;

    VerifyOrExit(mState == OT_JOINER_STATE_ENTRUST && aMessage.GetType() == OT_COAP_TYPE_CONFIRMABLE &&
                     aMessage.GetCode() == OT_COAP_CODE_POST,
                 error = OT_ERROR_DROP);

    otLogInfoMeshCoP("Received joiner entrust");
    otLogCertMeshCoP("[THCI] direction=recv | type=JOIN_ENT.ntf");

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

    otLogInfoMeshCoP("join success!");

    // Send dummy response.
    SendJoinerEntrustResponse(aMessage, aMessageInfo);

    // Delay extended address configuration to allow DTLS wrap up.
    mTimer.Start(kConfigExtAddressDelay);

exit:

    if (error != OT_ERROR_NONE)
    {
        otLogWarnMeshCoP("Error while processing joiner entrust: %s", otThreadErrorToString(error));
    }
}

void Joiner::SendJoinerEntrustResponse(const Coap::Message &aRequest, const Ip6::MessageInfo &aRequestInfo)
{
    ThreadNetif &    netif = GetNetif();
    otError          error = OT_ERROR_NONE;
    Coap::Message *  message;
    Ip6::MessageInfo responseInfo(aRequestInfo);

    VerifyOrExit((message = NewMeshCoPMessage(netif.GetCoap())) != NULL, error = OT_ERROR_NO_BUFS);
    message->SetDefaultResponseHeader(aRequest);
    message->SetSubType(Message::kSubTypeJoinerEntrust);

    memset(&responseInfo.mSockAddr, 0, sizeof(responseInfo.mSockAddr));
    SuccessOrExit(error = netif.GetCoap().SendMessage(*message, responseInfo));

    mState = OT_JOINER_STATE_JOINED;

    otLogInfoArp("Sent Joiner Entrust response");

    otLogInfoMeshCoP("Sent joiner entrust response length = %d", message->GetLength());
    otLogCertMeshCoP("[THCI] direction=send | type=JOIN_ENT.rsp");

exit:

    if (error != OT_ERROR_NONE && message != NULL)
    {
        message->Free();
    }
}

void Joiner::HandleTimer(Timer &aTimer)
{
    aTimer.GetOwner<Joiner>().HandleTimer();
}

void Joiner::HandleTimer(void)
{
    ThreadNetif &netif = GetNetif();
    otError      error = OT_ERROR_NONE;

    switch (mState)
    {
    case OT_JOINER_STATE_IDLE:
    case OT_JOINER_STATE_DISCOVER:
    case OT_JOINER_STATE_CONNECT:
        assert(false);
        break;

    case OT_JOINER_STATE_CONNECTED:
    case OT_JOINER_STATE_ENTRUST:
        error = OT_ERROR_RESPONSE_TIMEOUT;
        break;

    case OT_JOINER_STATE_JOINED:
        Mac::ExtAddress extAddress;

        extAddress.GenerateRandom();
        netif.GetMac().SetExtAddress(extAddress);
        netif.GetMle().UpdateLinkLocalAddress();

        error = OT_ERROR_NONE;
        break;
    }

    Complete(error);
}

} // namespace MeshCoP
} // namespace ot

#endif // OPENTHREAD_ENABLE_JOINER
