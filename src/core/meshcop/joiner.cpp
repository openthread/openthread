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
#include "common/string.hpp"
#include "meshcop/meshcop.hpp"
#include "radio/radio.hpp"
#include "thread/thread_netif.hpp"
#include "thread/uri_paths.hpp"
#include "utils/otns.hpp"

#if OPENTHREAD_CONFIG_JOINER_ENABLE

namespace ot {
namespace MeshCoP {

Joiner::Joiner(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mId()
    , mDiscerner()
    , mState(kStateIdle)
    , mCallback(nullptr)
    , mContext(nullptr)
    , mJoinerRouterIndex(0)
    , mFinalizeMessage(nullptr)
    , mTimer(aInstance, Joiner::HandleTimer, this)
    , mJoinerEntrust(UriPath::kJoinerEntrust, &Joiner::HandleJoinerEntrust, this)
{
    SetIdFromIeeeEui64();
    mDiscerner.Clear();
    memset(mJoinerRouters, 0, sizeof(mJoinerRouters));
    Get<Tmf::TmfAgent>().AddResource(mJoinerEntrust);
}

void Joiner::SetIdFromIeeeEui64(void)
{
    Mac::ExtAddress eui64;

    Get<Radio>().GetIeeeEui64(eui64);
    ComputeJoinerId(eui64, mId);
}

const JoinerDiscerner *Joiner::GetDiscerner(void) const
{
    return mDiscerner.IsEmpty() ? nullptr : &mDiscerner;
}

otError Joiner::SetDiscerner(const JoinerDiscerner &aDiscerner)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(aDiscerner.IsValid(), error = OT_ERROR_INVALID_ARGS);
    VerifyOrExit(mState == kStateIdle, error = OT_ERROR_INVALID_STATE);

    mDiscerner = aDiscerner;
    mDiscerner.GenerateJoinerId(mId);

exit:
    return error;
}

otError Joiner::ClearDiscerner(void)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(mState == kStateIdle, error = OT_ERROR_INVALID_STATE);
    VerifyOrExit(!mDiscerner.IsEmpty(), OT_NOOP);

    mDiscerner.Clear();
    SetIdFromIeeeEui64();

exit:
    return error;
}

void Joiner::SetState(State aState)
{
    State oldState = mState;
    OT_UNUSED_VARIABLE(oldState);

    SuccessOrExit(Get<Notifier>().Update(mState, aState, kEventJoinerStateChanged));

    otLogInfoMeshCoP("JoinerState: %s -> %s", StateToString(oldState), StateToString(aState));
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
    otError                      error;
    JoinerPskd                   joinerPskd;
    Mac::ExtAddress              randomAddress;
    SteeringData::HashBitIndexes filterIndexes;

    otLogInfoMeshCoP("Joiner starting");

    VerifyOrExit(mState == kStateIdle, error = OT_ERROR_BUSY);
    VerifyOrExit(Get<ThreadNetif>().IsUp() && Get<Mle::Mle>().GetRole() == Mle::kRoleDisabled,
                 error = OT_ERROR_INVALID_STATE);

    SuccessOrExit(error = joinerPskd.SetFrom(aPskd));

    // Use random-generated extended address.
    randomAddress.GenerateRandom();
    Get<Mac::Mac>().SetExtAddress(randomAddress);
    Get<Mle::MleRouter>().UpdateLinkLocalAddress();

    SuccessOrExit(error = Get<Coap::CoapSecure>().Start(kJoinerUdpPort));
    Get<Coap::CoapSecure>().SetPsk(joinerPskd);

    for (JoinerRouter &router : mJoinerRouters)
    {
        router.mPriority = 0; // Priority zero means entry is not in-use.
    }

    SuccessOrExit(error = PrepareJoinerFinalizeMessage(aProvisioningUrl, aVendorName, aVendorModel, aVendorSwVersion,
                                                       aVendorData));

    if (!mDiscerner.IsEmpty())
    {
        SteeringData::CalculateHashBitIndexes(mDiscerner, filterIndexes);
    }
    else
    {
        SteeringData::CalculateHashBitIndexes(mId, filterIndexes);
    }

    SuccessOrExit(error = Get<Mle::DiscoverScanner>().Discover(Mac::ChannelMask(0), Get<Mac::Mac>().GetPanId(),
                                                               /* aJoiner */ true, /* aEnableFiltering */ true,
                                                               &filterIndexes, HandleDiscoverResult, this));
    mCallback = aCallback;
    mContext  = aContext;

    SetState(kStateDiscover);

exit:
    if (error != OT_ERROR_NONE)
    {
        FreeJoinerFinalizeMessage();
    }

    LogError("start joiner", error);
    return error;
}

void Joiner::Stop(void)
{
    otLogInfoMeshCoP("Joiner stopped");

    // Callback is set to `nullptr` to skip calling it from `Finish()`
    mCallback = nullptr;
    Finish(OT_ERROR_ABORT);
}

void Joiner::Finish(otError aError)
{
    switch (mState)
    {
    case kStateIdle:
        ExitNow();

    case kStateConnect:
    case kStateConnected:
    case kStateEntrust:
    case kStateJoined:
        Get<Coap::CoapSecure>().Disconnect();
        IgnoreError(Get<Ip6::Filter>().RemoveUnsecurePort(kJoinerUdpPort));
        mTimer.Stop();

        // Fall through

    case kStateDiscover:
        Get<Coap::CoapSecure>().Stop();
        break;
    }

    SetState(kStateIdle);
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

void Joiner::HandleDiscoverResult(Mle::DiscoverScanner::ScanResult *aResult, void *aContext)
{
    static_cast<Joiner *>(aContext)->HandleDiscoverResult(aResult);
}

void Joiner::HandleDiscoverResult(Mle::DiscoverScanner::ScanResult *aResult)
{
    VerifyOrExit(mState == kStateDiscover, OT_NOOP);

    if (aResult != nullptr)
    {
        SaveDiscoveredJoinerRouter(*aResult);
    }
    else
    {
        Get<Mac::Mac>().SetExtAddress(mId);
        Get<Mle::MleRouter>().UpdateLinkLocalAddress();

        mJoinerRouterIndex = 0;
        TryNextJoinerRouter(OT_ERROR_NONE);
    }

exit:
    return;
}

void Joiner::SaveDiscoveredJoinerRouter(const Mle::DiscoverScanner::ScanResult &aResult)
{
    uint8_t       priority;
    bool          doesAllowAny;
    JoinerRouter *end = OT_ARRAY_END(mJoinerRouters);
    JoinerRouter *entry;

    doesAllowAny = static_cast<const SteeringData &>(aResult.mSteeringData).PermitsAllJoiners();

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

    VerifyOrExit(entry < end, OT_NOOP);

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
    Ip6::SockAddr sockAddr(aRouter.mJoinerUdpPort);

    otLogInfoMeshCoP("Joiner connecting to %s, pan:0x%04x, chan:%d", aRouter.mExtAddr.ToString().AsCString(),
                     aRouter.mPanId, aRouter.mChannel);

    Get<Mac::Mac>().SetPanId(aRouter.mPanId);
    SuccessOrExit(error = Get<Mac::Mac>().SetPanChannel(aRouter.mChannel));
    SuccessOrExit(error = Get<Ip6::Filter>().AddUnsecurePort(kJoinerUdpPort));

    sockAddr.GetAddress().SetToLinkLocalAddress(aRouter.mExtAddr);

    SuccessOrExit(error = Get<Coap::CoapSecure>().Connect(sockAddr, Joiner::HandleSecureCoapClientConnect, this));

    SetState(kStateConnect);

exit:
    LogError("start secure joiner connection", error);
    return error;
}

void Joiner::HandleSecureCoapClientConnect(bool aConnected, void *aContext)
{
    static_cast<Joiner *>(aContext)->HandleSecureCoapClientConnect(aConnected);
}

void Joiner::HandleSecureCoapClientConnect(bool aConnected)
{
    VerifyOrExit(mState == kStateConnect, OT_NOOP);

    if (aConnected)
    {
        SetState(kStateConnected);
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
    VendorNameTlv         vendorNameTlv;
    VendorModelTlv        vendorModelTlv;
    VendorSwVersionTlv    vendorSwVersionTlv;
    VendorStackVersionTlv vendorStackVersionTlv;
    ProvisioningUrlTlv    provisioningUrlTlv;

    VerifyOrExit((mFinalizeMessage = NewMeshCoPMessage(Get<Coap::CoapSecure>())) != nullptr, error = OT_ERROR_NO_BUFS);

    mFinalizeMessage->InitAsConfirmablePost();
    SuccessOrExit(error = mFinalizeMessage->AppendUriPathOptions(UriPath::kJoinerFinalize));
    SuccessOrExit(error = mFinalizeMessage->SetPayloadMarker());
    mFinalizeMessage->SetOffset(mFinalizeMessage->GetLength());

    SuccessOrExit(error = Tlv::AppendUint8Tlv(*mFinalizeMessage, Tlv::kState, StateTlv::kAccept));

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

    if (aVendorData != nullptr)
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
    VerifyOrExit(mState == kStateIdle && mFinalizeMessage != nullptr, OT_NOOP);

    mFinalizeMessage->Free();
    mFinalizeMessage = nullptr;

exit:
    return;
}

void Joiner::SendJoinerFinalize(void)
{
    OT_ASSERT(mFinalizeMessage != nullptr);

#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
    LogCertMessage("[THCI] direction=send | type=JOIN_FIN.req |", *mFinalizeMessage);
#endif

    SuccessOrExit(Get<Coap::CoapSecure>().SendMessage(*mFinalizeMessage, Joiner::HandleJoinerFinalizeResponse, this));
    mFinalizeMessage = nullptr;

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

    uint8_t state;

    VerifyOrExit(mState == kStateConnected && aResult == OT_ERROR_NONE && aMessage.IsAck() &&
                     aMessage.GetCode() == Coap::kCodeChanged,
                 OT_NOOP);

    SuccessOrExit(Tlv::FindUint8Tlv(aMessage, Tlv::kState, state));

    SetState(kStateEntrust);
    mTimer.Start(kReponseTimeout);

    otLogInfoMeshCoP("Joiner received finalize response %d", state);

#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
    LogCertMessage("[THCI] direction=recv | type=JOIN_FIN.rsp |", aMessage);
#endif

exit:
    Get<Coap::CoapSecure>().Disconnect();
    IgnoreError(Get<Ip6::Filter>().RemoveUnsecurePort(kJoinerUdpPort));
}

void Joiner::HandleJoinerEntrust(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
    static_cast<Joiner *>(aContext)->HandleJoinerEntrust(*static_cast<Coap::Message *>(aMessage),
                                                         *static_cast<const Ip6::MessageInfo *>(aMessageInfo));
}

void Joiner::HandleJoinerEntrust(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    otError              error;
    otOperationalDataset dataset;

    VerifyOrExit(mState == kStateEntrust && aMessage.IsConfirmablePostRequest(), error = OT_ERROR_DROP);

    otLogInfoMeshCoP("Joiner received entrust");
    otLogCertMeshCoP("[THCI] direction=recv | type=JOIN_ENT.ntf");

    memset(&dataset, 0, sizeof(dataset));

    SuccessOrExit(error = Tlv::FindTlv(aMessage, Tlv::kNetworkMasterKey, &dataset.mMasterKey, sizeof(MasterKey)));
    dataset.mComponents.mIsMasterKeyPresent = true;

    dataset.mChannel                      = Get<Mac::Mac>().GetPanChannel();
    dataset.mComponents.mIsChannelPresent = true;

    dataset.mPanId                      = Get<Mac::Mac>().GetPanId();
    dataset.mComponents.mIsPanIdPresent = true;

    IgnoreError(Get<MeshCoP::ActiveDataset>().Save(dataset));

    otLogInfoMeshCoP("Joiner successful!");

    SendJoinerEntrustResponse(aMessage, aMessageInfo);

    // Delay extended address configuration to allow DTLS wrap up.
    mTimer.Start(kConfigExtAddressDelay);

exit:
    LogError("process joiner entrust", error);
}

void Joiner::SendJoinerEntrustResponse(const Coap::Message &aRequest, const Ip6::MessageInfo &aRequestInfo)
{
    otError          error = OT_ERROR_NONE;
    Coap::Message *  message;
    Ip6::MessageInfo responseInfo(aRequestInfo);

    VerifyOrExit((message = NewMeshCoPMessage(Get<Tmf::TmfAgent>())) != nullptr, error = OT_ERROR_NO_BUFS);
    SuccessOrExit(error = message->SetDefaultResponseHeader(aRequest));
    message->SetSubType(Message::kSubTypeJoinerEntrust);

    responseInfo.GetSockAddr().Clear();
    SuccessOrExit(error = Get<Tmf::TmfAgent>().SendMessage(*message, responseInfo));

    SetState(kStateJoined);

    otLogInfoMeshCoP("Joiner sent entrust response");
    otLogCertMeshCoP("[THCI] direction=send | type=JOIN_ENT.rsp");

exit:
    FreeMessageOnError(message, error);
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
    case kStateIdle:
    case kStateDiscover:
    case kStateConnect:
        OT_ASSERT(false);
        OT_UNREACHABLE_CODE(break);

    case kStateConnected:
    case kStateEntrust:
        error = OT_ERROR_RESPONSE_TIMEOUT;
        break;

    case kStateJoined:
        Mac::ExtAddress extAddress;

        extAddress.GenerateRandom();
        Get<Mac::Mac>().SetExtAddress(extAddress);
        Get<Mle::MleRouter>().UpdateLinkLocalAddress();

        error = OT_ERROR_NONE;
        break;
    }

    Finish(error);
}

// LCOV_EXCL_START

const char *Joiner::StateToString(State aState)
{
    const char *str = "Unknown";

    switch (aState)
    {
    case kStateIdle:
        str = "Idle";
        break;
    case kStateDiscover:
        str = "Discover";
        break;
    case kStateConnect:
        str = "Connecting";
        break;
    case kStateConnected:
        str = "Connected";
        break;
    case kStateEntrust:
        str = "Entrust";
        break;
    case kStateJoined:
        str = "Joined";
        break;
    }

    return str;
}

#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
void Joiner::LogCertMessage(const char *aText, const Coap::Message &aMessage) const
{
    uint8_t buf[OPENTHREAD_CONFIG_MESSAGE_BUFFER_SIZE];

    VerifyOrExit(aMessage.GetLength() <= sizeof(buf), OT_NOOP);
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
