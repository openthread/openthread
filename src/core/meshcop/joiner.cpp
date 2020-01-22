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

#include "joiner.hpp"

#include <stdio.h>

#include "common/code_utils.hpp"
#include "common/debug.hpp"
#include "common/encoding.hpp"
#include "common/instance.hpp"
#include "common/locator-getters.hpp"
#include "common/logging.hpp"
#include "meshcop/meshcop.hpp"
#include "radio/radio.hpp"
#include "thread/thread_netif.hpp"
#include "thread/thread_uri_paths.hpp"

#if OPENTHREAD_CONFIG_JOINER_ENABLE

using ot::Encoding::BigEndian::HostSwap16;

namespace ot {
namespace MeshCoP {

Joiner::Joiner(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mState(OT_JOINER_STATE_IDLE)
    , mCallback(NULL)
    , mContext(NULL)
    , mJoinerRouterIndex(0)
    , mFinalizeMessage(NULL)
    , mTimer(aInstance, &Joiner::HandleTimer, this)
    , mJoinerEntrust(OT_URI_PATH_JOINER_ENTRUST, &Joiner::HandleJoinerEntrust, this)
{
    memset(mJoinerRouters, 0, sizeof(mJoinerRouters));
    Get<Coap::Coap>().AddResource(mJoinerEntrust);
}

void Joiner::GetJoinerId(Mac::ExtAddress &aJoinerId) const
{
    Get<Radio>().GetIeeeEui64(aJoinerId);
    ComputeJoinerId(aJoinerId, aJoinerId);
}

void Joiner::SetState(otJoinerState aState)
{
    VerifyOrExit(aState != mState);

    otLogInfoMeshCoP("JoinerState: %s -> %s", JoinerStateToString(mState), JoinerStateToString(aState));
    mState = aState;

exit:
    return;
}

otError Joiner::Start(const char *     aPskd,
                      const char *     aProvisioningUrl,
                      const char *     aVendorName,
                      const char *     aVendorModel,
                      const char *     aVendorSwVersion,
                      const char *     aVendorData,
                      otJoinerCallback aCallback,
                      void *           aContext)
{
    otError         error;
    Mac::ExtAddress joinerId;

    otLogInfoMeshCoP("Joiner starting");

    VerifyOrExit(mState == OT_JOINER_STATE_IDLE, error = OT_ERROR_BUSY);

    // Use extended address based on factory-assigned IEEE EUI-64
    GetJoinerId(joinerId);
    Get<Mac::Mac>().SetExtAddress(joinerId);
    Get<Mle::MleRouter>().UpdateLinkLocalAddress();

    SuccessOrExit(error = Get<Coap::CoapSecure>().Start(kJoinerUdpPort));
    SuccessOrExit(error = Get<Coap::CoapSecure>().SetPsk(reinterpret_cast<const uint8_t *>(aPskd),
                                                         static_cast<uint8_t>(strlen(aPskd))));

    for (JoinerRouter *router = &mJoinerRouters[0]; router < OT_ARRAY_END(mJoinerRouters); router++)
    {
        router->mPriority = 0; // Priority zero means entry is not in-use.
    }

    SuccessOrExit(error = PrepareJoinerFinalizeMessage(aProvisioningUrl, aVendorName, aVendorModel, aVendorSwVersion,
                                                       aVendorData));

    SuccessOrExit(error = Get<Mle::MleRouter>().Discover(Mac::ChannelMask(0), Get<Mac::Mac>().GetPanId(),
                                                         /* aJoiner */ true, /* aEnableFiltering */ true,
                                                         HandleDiscoverResult, this));
    mCallback = aCallback;
    mContext  = aContext;

    SetState(OT_JOINER_STATE_DISCOVER);

exit:
    if (error != OT_ERROR_NONE)
    {
        otLogInfoMeshCoP("Error %s while starting joiner", otThreadErrorToString(error));
        FreeJoinerFinalizeMessage();
    }

    return error;
}

void Joiner::Stop(void)
{
    otLogInfoMeshCoP("Joiner stopped");

    // Callback is set to `NULL` to skip calling it from `Finish()`
    mCallback = NULL;
    Finish(OT_ERROR_ABORT);
}

void Joiner::Finish(otError aError)
{
    switch (mState)
    {
    case OT_JOINER_STATE_IDLE:
        ExitNow();

    case OT_JOINER_STATE_CONNECT:
    case OT_JOINER_STATE_CONNECTED:
    case OT_JOINER_STATE_ENTRUST:
    case OT_JOINER_STATE_JOINED:
        Get<Coap::CoapSecure>().Disconnect();
        Get<Ip6::Filter>().RemoveUnsecurePort(kJoinerUdpPort);
        mTimer.Stop();

        // Fall through

    case OT_JOINER_STATE_DISCOVER:
        Get<Coap::CoapSecure>().Stop();
        break;
    }

    SetState(OT_JOINER_STATE_IDLE);
    FreeJoinerFinalizeMessage();

    if (mCallback)
    {
        mCallback(aError, mContext);
    }

exit:
    return;
}

uint8_t Joiner::CalculatePriority(int8_t aRssi, bool aSteeringDataAllowsAny)
{
    int16_t priority;

    if (aRssi == OT_RADIO_RSSI_INVALID)
    {
        aRssi = -127;
    }

    // Limit the RSSI to range (-128, 0), i.e. -128 < aRssi < 0.

    if (aRssi <= -128)
    {
        priority = -127;
    }
    else if (aRssi >= 0)
    {
        priority = -1;
    }
    else
    {
        priority = aRssi;
    }

    // Assign higher priority to networks with an exact match of Joiner
    // ID in the Steering Data (128 < priority < 256) compared to ones
    // that allow all Joiners (0 < priority < 128). Sub-prioritize
    // based on signal strength. Priority 0 is reserved for unused
    // entry.

    priority += aSteeringDataAllowsAny ? 128 : 256;

    return static_cast<uint8_t>(priority);
}

void Joiner::HandleDiscoverResult(otActiveScanResult *aResult, void *aContext)
{
    static_cast<Joiner *>(aContext)->HandleDiscoverResult(aResult);
}

void Joiner::HandleDiscoverResult(otActiveScanResult *aResult)
{
    VerifyOrExit(mState == OT_JOINER_STATE_DISCOVER);

    if (aResult != NULL)
    {
        SaveDiscoveredJoinerRouter(*aResult);
    }
    else
    {
        mJoinerRouterIndex = 0;
        TryNextJoinerRouter(OT_ERROR_NONE);
    }

exit:
    return;
}

void Joiner::SaveDiscoveredJoinerRouter(const otActiveScanResult &aResult)
{
    uint8_t       priority;
    bool          doesAllowAny = true;
    JoinerRouter *end          = OT_ARRAY_END(mJoinerRouters);
    JoinerRouter *entry;

    // Check whether Steering Data allows any joiner (it is all 0xff).
    for (uint8_t i = 0; i < aResult.mSteeringData.mLength; i++)
    {
        if (aResult.mSteeringData.m8[i] != 0xff)
        {
            doesAllowAny = false;
            break;
        }
    }

    otLogInfoMeshCoP("Joiner discover network: %s, pan:0x%04x, port:%d, chan:%d, rssi:%d, allow-any:%s",
                     static_cast<const Mac::ExtAddress &>(aResult.mExtAddress).ToString().AsCString(), aResult.mPanId,
                     aResult.mJoinerUdpPort, aResult.mChannel, aResult.mRssi, doesAllowAny ? "yes" : "no");

    priority = CalculatePriority(aResult.mRssi, doesAllowAny);

    // We keep the list sorted based on priority. Find the place to
    // add the new result.

    for (entry = &mJoinerRouters[0]; entry < end; entry++)
    {
        if (priority > entry->mPriority)
        {
            break;
        }
    }

    VerifyOrExit(entry < end);

    // Shift elements in array to make room for the new one.
    memmove(entry + 1, entry,
            static_cast<size_t>(reinterpret_cast<uint8_t *>(end - 1) - reinterpret_cast<uint8_t *>(entry)));

    entry->mExtAddr       = static_cast<const Mac::ExtAddress &>(aResult.mExtAddress);
    entry->mPanId         = aResult.mPanId;
    entry->mJoinerUdpPort = aResult.mJoinerUdpPort;
    entry->mChannel       = aResult.mChannel;
    entry->mPriority      = priority;

exit:
    return;
}

void Joiner::TryNextJoinerRouter(otError aPrevError)
{
    for (; mJoinerRouterIndex < OT_ARRAY_LENGTH(mJoinerRouters); mJoinerRouterIndex++)
    {
        JoinerRouter &router = mJoinerRouters[mJoinerRouterIndex];
        otError       error;

        if (router.mPriority == 0)
        {
            break;
        }

        error = Connect(router);
        VerifyOrExit(error != OT_ERROR_NONE, mJoinerRouterIndex++);

        // Save the error from `Connect` only if there is no previous
        // error from earlier attempts. This ensures that if there has
        // been a previous Joiner Router connect attempt where
        // `Connect()` call itself was successful, the error status
        // emitted from `Finish()` call corresponds to the error from
        // that attempt.

        if (aPrevError == OT_ERROR_NONE)
        {
            aPrevError = error;
        }
    }

    if (aPrevError == OT_ERROR_NONE)
    {
        aPrevError = OT_ERROR_NOT_FOUND;
    }

    Finish(aPrevError);

exit:
    return;
}

otError Joiner::Connect(JoinerRouter &aRouter)
{
    otError       error = OT_ERROR_NOT_FOUND;
    Ip6::SockAddr sockaddr;

    otLogInfoMeshCoP("Joiner connecting to %s, pan:0x%04x, chan:%d", aRouter.mExtAddr.ToString().AsCString(),
                     aRouter.mPanId, aRouter.mChannel);

    Get<Mac::Mac>().SetPanId(aRouter.mPanId);
    SuccessOrExit(error = Get<Mac::Mac>().SetPanChannel(aRouter.mChannel));
    SuccessOrExit(error = Get<Ip6::Filter>().AddUnsecurePort(kJoinerUdpPort));

    sockaddr.GetAddress().mFields.m16[0] = HostSwap16(0xfe80);
    sockaddr.GetAddress().SetIid(aRouter.mExtAddr);
    sockaddr.mPort = aRouter.mJoinerUdpPort;

    SuccessOrExit(error = Get<Coap::CoapSecure>().Connect(sockaddr, Joiner::HandleSecureCoapClientConnect, this));

    SetState(OT_JOINER_STATE_CONNECT);

exit:

    if (error != OT_ERROR_NONE)
    {
        otLogInfoMeshCoP("Error %s while joiner trying to connect", otThreadErrorToString(error));
    }

    return error;
}

void Joiner::HandleSecureCoapClientConnect(bool aConnected, void *aContext)
{
    static_cast<Joiner *>(aContext)->HandleSecureCoapClientConnect(aConnected);
}

void Joiner::HandleSecureCoapClientConnect(bool aConnected)
{
    VerifyOrExit(mState == OT_JOINER_STATE_CONNECT);

    if (aConnected)
    {
        SetState(OT_JOINER_STATE_CONNECTED);
        SendJoinerFinalize();
        mTimer.Start(kReponseTimeout);
    }
    else
    {
        TryNextJoinerRouter(OT_ERROR_SECURITY);
    }

exit:
    return;
}

otError Joiner::PrepareJoinerFinalizeMessage(const char *aProvisioningUrl,
                                             const char *aVendorName,
                                             const char *aVendorModel,
                                             const char *aVendorSwVersion,
                                             const char *aVendorData)
{
    otError               error = OT_ERROR_NONE;
    StateTlv              stateTlv;
    VendorNameTlv         vendorNameTlv;
    VendorModelTlv        vendorModelTlv;
    VendorSwVersionTlv    vendorSwVersionTlv;
    VendorStackVersionTlv vendorStackVersionTlv;
    ProvisioningUrlTlv    provisioningUrlTlv;

    VerifyOrExit((mFinalizeMessage = NewMeshCoPMessage(Get<Coap::CoapSecure>())) != NULL, error = OT_ERROR_NO_BUFS);

    mFinalizeMessage->Init(OT_COAP_TYPE_CONFIRMABLE, OT_COAP_CODE_POST);
    SuccessOrExit(error = mFinalizeMessage->AppendUriPathOptions(OT_URI_PATH_JOINER_FINALIZE));
    SuccessOrExit(error = mFinalizeMessage->SetPayloadMarker());
    mFinalizeMessage->SetOffset(mFinalizeMessage->GetLength());

    stateTlv.Init();
    stateTlv.SetState(MeshCoP::StateTlv::kAccept);
    SuccessOrExit(error = stateTlv.AppendTo(*mFinalizeMessage));

    vendorNameTlv.Init();
    vendorNameTlv.SetVendorName(aVendorName);
    SuccessOrExit(error = vendorNameTlv.AppendTo(*mFinalizeMessage));

    vendorModelTlv.Init();
    vendorModelTlv.SetVendorModel(aVendorModel);
    SuccessOrExit(error = vendorModelTlv.AppendTo(*mFinalizeMessage));

    vendorSwVersionTlv.Init();
    vendorSwVersionTlv.SetVendorSwVersion(aVendorSwVersion);
    SuccessOrExit(error = vendorSwVersionTlv.AppendTo(*mFinalizeMessage));

    vendorStackVersionTlv.Init();
    vendorStackVersionTlv.SetOui(OPENTHREAD_CONFIG_STACK_VENDOR_OUI);
    vendorStackVersionTlv.SetMajor(OPENTHREAD_CONFIG_STACK_VERSION_MAJOR);
    vendorStackVersionTlv.SetMinor(OPENTHREAD_CONFIG_STACK_VERSION_MINOR);
    vendorStackVersionTlv.SetRevision(OPENTHREAD_CONFIG_STACK_VERSION_REV);
    SuccessOrExit(error = vendorStackVersionTlv.AppendTo(*mFinalizeMessage));

    if (aVendorData != NULL)
    {
        VendorDataTlv vendorDataTlv;
        vendorDataTlv.Init();
        vendorDataTlv.SetVendorData(aVendorData);
        SuccessOrExit(error = vendorDataTlv.AppendTo(*mFinalizeMessage));
    }

    provisioningUrlTlv.Init();
    provisioningUrlTlv.SetProvisioningUrl(aProvisioningUrl);

    if (provisioningUrlTlv.GetLength() > 0)
    {
        SuccessOrExit(error = provisioningUrlTlv.AppendTo(*mFinalizeMessage));
    }

exit:
    if (error != OT_ERROR_NONE)
    {
        FreeJoinerFinalizeMessage();
    }

    return error;
}

void Joiner::FreeJoinerFinalizeMessage(void)
{
    VerifyOrExit(mState == OT_JOINER_STATE_IDLE && mFinalizeMessage != NULL);

    mFinalizeMessage->Free();
    mFinalizeMessage = NULL;

exit:
    return;
}

void Joiner::SendJoinerFinalize(void)
{
    assert(mFinalizeMessage != NULL);

#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
    LogCertMessage("[THCI] direction=send | type=JOIN_FIN.req |", *mFinalizeMessage);
#endif

    SuccessOrExit(Get<Coap::CoapSecure>().SendMessage(*mFinalizeMessage, Joiner::HandleJoinerFinalizeResponse, this));
    mFinalizeMessage = NULL;

    otLogInfoMeshCoP("Joiner sent finalize");

exit:
    return;
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

    SetState(OT_JOINER_STATE_ENTRUST);
    mTimer.Start(kReponseTimeout);

    otLogInfoMeshCoP("Joiner received finalize response %d", static_cast<uint8_t>(state.GetState()));

#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
    LogCertMessage("[THCI] direction=recv | type=JOIN_FIN.rsp |", aMessage);
#endif

exit:
    Get<Coap::CoapSecure>().Disconnect();
    Get<Ip6::Filter>().RemoveUnsecurePort(kJoinerUdpPort);
}

void Joiner::HandleJoinerEntrust(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
    static_cast<Joiner *>(aContext)->HandleJoinerEntrust(*static_cast<Coap::Message *>(aMessage),
                                                         *static_cast<const Ip6::MessageInfo *>(aMessageInfo));
}

void Joiner::HandleJoinerEntrust(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    otError              error;
    NetworkMasterKeyTlv  masterKey;
    otOperationalDataset dataset;

    VerifyOrExit(mState == OT_JOINER_STATE_ENTRUST && aMessage.GetType() == OT_COAP_TYPE_CONFIRMABLE &&
                     aMessage.GetCode() == OT_COAP_CODE_POST,
                 error = OT_ERROR_DROP);

    otLogInfoMeshCoP("Joiner received entrust");
    otLogCertMeshCoP("[THCI] direction=recv | type=JOIN_ENT.ntf");

    SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kNetworkMasterKey, sizeof(masterKey), masterKey));
    VerifyOrExit(masterKey.IsValid(), error = OT_ERROR_PARSE);

    memset(&dataset, 0, sizeof(dataset));

    dataset.mMasterKey                      = masterKey.GetNetworkMasterKey();
    dataset.mComponents.mIsMasterKeyPresent = true;

    dataset.mChannel                      = Get<Mac::Mac>().GetPanChannel();
    dataset.mComponents.mIsChannelPresent = true;

    dataset.mPanId                      = Get<Mac::Mac>().GetPanId();
    dataset.mComponents.mIsPanIdPresent = true;

    Get<MeshCoP::ActiveDataset>().Save(dataset);

    otLogInfoMeshCoP("Joiner successful!");

    SendJoinerEntrustResponse(aMessage, aMessageInfo);

    // Delay extended address configuration to allow DTLS wrap up.
    mTimer.Start(kConfigExtAddressDelay);

exit:

    if (error != OT_ERROR_NONE)
    {
        otLogWarnMeshCoP("Error %s while processing joiner entrust", otThreadErrorToString(error));
    }
}

void Joiner::SendJoinerEntrustResponse(const Coap::Message &aRequest, const Ip6::MessageInfo &aRequestInfo)
{
    otError          error = OT_ERROR_NONE;
    Coap::Message *  message;
    Ip6::MessageInfo responseInfo(aRequestInfo);

    VerifyOrExit((message = NewMeshCoPMessage(Get<Coap::Coap>())) != NULL, error = OT_ERROR_NO_BUFS);
    SuccessOrExit(error = message->SetDefaultResponseHeader(aRequest));
    message->SetSubType(Message::kSubTypeJoinerEntrust);

    responseInfo.GetSockAddr().Clear();
    SuccessOrExit(error = Get<Coap::Coap>().SendMessage(*message, responseInfo));

    SetState(OT_JOINER_STATE_JOINED);

    otLogInfoMeshCoP("Joiner sent entrust response");
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
    otError error = OT_ERROR_NONE;

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
        Get<Mac::Mac>().SetExtAddress(extAddress);
        Get<Mle::MleRouter>().UpdateLinkLocalAddress();

        error = OT_ERROR_NONE;
        break;
    }

    Finish(error);
}

// LOCV_EXCL_START

const char *Joiner::JoinerStateToString(otJoinerState aState)
{
    const char *str = "Unknown";

    switch (aState)
    {
    case OT_JOINER_STATE_IDLE:
        str = "Idle";
        break;
    case OT_JOINER_STATE_DISCOVER:
        str = "Discover";
        break;
    case OT_JOINER_STATE_CONNECT:
        str = "Connecting";
        break;
    case OT_JOINER_STATE_CONNECTED:
        str = "Connected";
        break;
    case OT_JOINER_STATE_ENTRUST:
        str = "Entrust";
        break;
    case OT_JOINER_STATE_JOINED:
        str = "Joined";
        break;
    }

    return str;
}

#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
void Joiner::LogCertMessage(const char *aText, const Coap::Message &aMessage) const
{
    uint8_t buf[OPENTHREAD_CONFIG_MESSAGE_BUFFER_SIZE];

    VerifyOrExit(aMessage.GetLength() <= sizeof(buf));
    aMessage.Read(aMessage.GetOffset(), aMessage.GetLength() - aMessage.GetOffset(), buf);

    otDumpCertMeshCoP(aText, buf, aMessage.GetLength() - aMessage.GetOffset());

exit:
    return;
}
#endif

// LCOV_EXCL_STOP

} // namespace MeshCoP
} // namespace ot

#endif // OPENTHREAD_CONFIG_JOINER_ENABLE
