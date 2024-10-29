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

#if OPENTHREAD_CONFIG_JOINER_ENABLE

#include "instance/instance.hpp"

namespace ot {
namespace MeshCoP {

RegisterLogModule("Joiner");

Joiner::Joiner(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mId()
    , mDiscerner()
    , mState(kStateIdle)
    , mJoinerRouterIndex(0)
    , mFinalizeMessage(nullptr)
    , mTimer(aInstance)
{
    SetIdFromIeeeEui64();
    mDiscerner.Clear();
    ClearAllBytes(mJoinerRouters);
}

void Joiner::SetIdFromIeeeEui64(void)
{
    Mac::ExtAddress eui64;

    Get<Radio>().GetIeeeEui64(eui64);
    ComputeJoinerId(eui64, mId);
}

const JoinerDiscerner *Joiner::GetDiscerner(void) const { return mDiscerner.IsEmpty() ? nullptr : &mDiscerner; }

Error Joiner::SetDiscerner(const JoinerDiscerner &aDiscerner)
{
    Error error = kErrorNone;

    VerifyOrExit(aDiscerner.IsValid(), error = kErrorInvalidArgs);
    VerifyOrExit(mState == kStateIdle, error = kErrorInvalidState);

    mDiscerner = aDiscerner;
    mDiscerner.GenerateJoinerId(mId);

exit:
    return error;
}

Error Joiner::ClearDiscerner(void)
{
    Error error = kErrorNone;

    VerifyOrExit(mState == kStateIdle, error = kErrorInvalidState);
    VerifyOrExit(!mDiscerner.IsEmpty());

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

    LogInfo("JoinerState: %s -> %s", StateToString(oldState), StateToString(aState));
exit:
    return;
}

Error Joiner::Start(const char      *aPskd,
                    const char      *aProvisioningUrl,
                    const char      *aVendorName,
                    const char      *aVendorModel,
                    const char      *aVendorSwVersion,
                    const char      *aVendorData,
                    otJoinerCallback aCallback,
                    void            *aContext)
{
    Error                        error;
    JoinerPskd                   joinerPskd;
    Mac::ExtAddress              randomAddress;
    SteeringData::HashBitIndexes filterIndexes;

    LogInfo("Joiner starting");

    VerifyOrExit(aProvisioningUrl == nullptr || IsValidUtf8String(aProvisioningUrl), error = kErrorInvalidArgs);
    VerifyOrExit(aVendorName == nullptr || IsValidUtf8String(aVendorName), error = kErrorInvalidArgs);
    VerifyOrExit(aVendorSwVersion == nullptr || IsValidUtf8String(aVendorSwVersion), error = kErrorInvalidArgs);

    VerifyOrExit(mState == kStateIdle, error = kErrorBusy);
    VerifyOrExit(Get<ThreadNetif>().IsUp() && Get<Mle::Mle>().GetRole() == Mle::kRoleDisabled,
                 error = kErrorInvalidState);

    SuccessOrExit(error = joinerPskd.SetFrom(aPskd));

    // Use random-generated extended address.
    randomAddress.GenerateRandom();
    Get<Mac::Mac>().SetExtAddress(randomAddress);
    Get<Mle::MleRouter>().UpdateLinkLocalAddress();

    SuccessOrExit(error = Get<Tmf::SecureAgent>().Start(kJoinerUdpPort));
    Get<Tmf::SecureAgent>().SetPsk(joinerPskd);

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

    mCallback.Set(aCallback, aContext);
    SetState(kStateDiscover);

exit:
    if (error != kErrorNone)
    {
        FreeJoinerFinalizeMessage();
    }

    LogWarnOnError(error, "start joiner");
    return error;
}

void Joiner::Stop(void)
{
    LogInfo("Joiner stopped");

    // Callback is set to `nullptr` to skip calling it from `Finish()`
    mCallback.Clear();
    Finish(kErrorAbort);
}

void Joiner::Finish(Error aError)
{
    switch (mState)
    {
    case kStateIdle:
        ExitNow();

    case kStateConnect:
    case kStateConnected:
    case kStateEntrust:
    case kStateJoined:
        Get<Tmf::SecureAgent>().Disconnect();
        IgnoreError(Get<Ip6::Filter>().RemoveUnsecurePort(kJoinerUdpPort));
        mTimer.Stop();

        OT_FALL_THROUGH;

    case kStateDiscover:
        Get<Tmf::SecureAgent>().Stop();
        break;
    }

    SetState(kStateIdle);
    FreeJoinerFinalizeMessage();

    mCallback.InvokeIfSet(aError);

exit:
    return;
}

uint8_t Joiner::CalculatePriority(int8_t aRssi, bool aSteeringDataAllowsAny)
{
    int16_t priority;

    if (aRssi == Radio::kInvalidRssi)
    {
        aRssi = -127;
    }

    // Clamp the RSSI to range [-127, -1]
    priority = Clamp<int8_t>(aRssi, -127, -1);

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
    VerifyOrExit(mState == kStateDiscover);

    if (aResult != nullptr && aResult->mJoinerUdpPort > 0)
    {
        SaveDiscoveredJoinerRouter(*aResult);
    }
    else
    {
        Get<Mac::Mac>().SetExtAddress(mId);
        Get<Mle::MleRouter>().UpdateLinkLocalAddress();

        mJoinerRouterIndex = 0;
        TryNextJoinerRouter(kErrorNone);
    }

exit:
    return;
}

void Joiner::SaveDiscoveredJoinerRouter(const Mle::DiscoverScanner::ScanResult &aResult)
{
    uint8_t       priority;
    bool          doesAllowAny;
    JoinerRouter *end = GetArrayEnd(mJoinerRouters);
    JoinerRouter *entry;

    doesAllowAny = AsCoreType(&aResult.mSteeringData).PermitsAllJoiners();

    LogInfo("Joiner discover network: %s, pan:0x%04x, port:%d, chan:%d, rssi:%d, allow-any:%s",
            AsCoreType(&aResult.mExtAddress).ToString().AsCString(), aResult.mPanId, aResult.mJoinerUdpPort,
            aResult.mChannel, aResult.mRssi, ToYesNo(doesAllowAny));

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

    entry->mExtAddr       = AsCoreType(&aResult.mExtAddress);
    entry->mPanId         = aResult.mPanId;
    entry->mJoinerUdpPort = aResult.mJoinerUdpPort;
    entry->mChannel       = aResult.mChannel;
    entry->mPriority      = priority;

exit:
    return;
}

void Joiner::TryNextJoinerRouter(Error aPrevError)
{
    for (; mJoinerRouterIndex < GetArrayLength(mJoinerRouters); mJoinerRouterIndex++)
    {
        JoinerRouter &router = mJoinerRouters[mJoinerRouterIndex];
        Error         error;

        if (router.mPriority == 0)
        {
            break;
        }

        error = Connect(router);
        VerifyOrExit(error != kErrorNone, mJoinerRouterIndex++);

        // Save the error from `Connect` only if there is no previous
        // error from earlier attempts. This ensures that if there has
        // been a previous Joiner Router connect attempt where
        // `Connect()` call itself was successful, the error status
        // emitted from `Finish()` call corresponds to the error from
        // that attempt.

        if (aPrevError == kErrorNone)
        {
            aPrevError = error;
        }
    }

    if (aPrevError == kErrorNone)
    {
        aPrevError = kErrorNotFound;
    }

    Finish(aPrevError);

exit:
    return;
}

Error Joiner::Connect(JoinerRouter &aRouter)
{
    Error         error = kErrorNotFound;
    Ip6::SockAddr sockAddr(aRouter.mJoinerUdpPort);

    LogInfo("Joiner connecting to %s, pan:0x%04x, chan:%d", aRouter.mExtAddr.ToString().AsCString(), aRouter.mPanId,
            aRouter.mChannel);

    Get<Mac::Mac>().SetPanId(aRouter.mPanId);
    SuccessOrExit(error = Get<Mac::Mac>().SetPanChannel(aRouter.mChannel));
    SuccessOrExit(error = Get<Ip6::Filter>().AddUnsecurePort(kJoinerUdpPort));

    sockAddr.GetAddress().SetToLinkLocalAddress(aRouter.mExtAddr);

    SuccessOrExit(error = Get<Tmf::SecureAgent>().Connect(sockAddr, Joiner::HandleSecureCoapClientConnect, this));

    SetState(kStateConnect);

exit:
    LogWarnOnError(error, "start secure joiner connection");
    return error;
}

void Joiner::HandleSecureCoapClientConnect(SecureTransport::ConnectEvent aEvent, void *aContext)
{
    static_cast<Joiner *>(aContext)->HandleSecureCoapClientConnect(aEvent);
}

void Joiner::HandleSecureCoapClientConnect(SecureTransport::ConnectEvent aEvent)
{
    VerifyOrExit(mState == kStateConnect);

    if (aEvent == SecureTransport::kConnected)
    {
        SetState(kStateConnected);
        SendJoinerFinalize();
        mTimer.Start(kResponseTimeout);
    }
    else
    {
        TryNextJoinerRouter(kErrorSecurity);
    }

exit:
    return;
}

Error Joiner::PrepareJoinerFinalizeMessage(const char *aProvisioningUrl,
                                           const char *aVendorName,
                                           const char *aVendorModel,
                                           const char *aVendorSwVersion,
                                           const char *aVendorData)
{
    Error                 error = kErrorNone;
    VendorStackVersionTlv vendorStackVersionTlv;

    mFinalizeMessage = Get<Tmf::SecureAgent>().NewPriorityConfirmablePostMessage(kUriJoinerFinalize);
    VerifyOrExit(mFinalizeMessage != nullptr, error = kErrorNoBufs);

    mFinalizeMessage->SetOffset(mFinalizeMessage->GetLength());

    SuccessOrExit(error = Tlv::Append<StateTlv>(*mFinalizeMessage, StateTlv::kAccept));

    SuccessOrExit(error = Tlv::Append<VendorNameTlv>(*mFinalizeMessage, aVendorName));
    SuccessOrExit(error = Tlv::Append<VendorModelTlv>(*mFinalizeMessage, aVendorModel));
    SuccessOrExit(error = Tlv::Append<VendorSwVersionTlv>(*mFinalizeMessage, aVendorSwVersion));

    vendorStackVersionTlv.Init();
    vendorStackVersionTlv.SetOui(OPENTHREAD_CONFIG_STACK_VENDOR_OUI);
    vendorStackVersionTlv.SetMajor(OPENTHREAD_CONFIG_STACK_VERSION_MAJOR);
    vendorStackVersionTlv.SetMinor(OPENTHREAD_CONFIG_STACK_VERSION_MINOR);
    vendorStackVersionTlv.SetRevision(OPENTHREAD_CONFIG_STACK_VERSION_REV);
    SuccessOrExit(error = vendorStackVersionTlv.AppendTo(*mFinalizeMessage));

    if (aVendorData != nullptr)
    {
        SuccessOrExit(error = Tlv::Append<VendorDataTlv>(*mFinalizeMessage, aVendorData));
    }

    if (aProvisioningUrl != nullptr)
    {
        SuccessOrExit(error = Tlv::Append<ProvisioningUrlTlv>(*mFinalizeMessage, aProvisioningUrl));
    }

exit:
    if (error != kErrorNone)
    {
        FreeJoinerFinalizeMessage();
    }

    return error;
}

void Joiner::FreeJoinerFinalizeMessage(void)
{
    VerifyOrExit(mState == kStateIdle && mFinalizeMessage != nullptr);

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

    SuccessOrExit(Get<Tmf::SecureAgent>().SendMessage(*mFinalizeMessage, Joiner::HandleJoinerFinalizeResponse, this));
    mFinalizeMessage = nullptr;

    LogInfo("Sent %s", UriToString<kUriJoinerFinalize>());

exit:
    return;
}

void Joiner::HandleJoinerFinalizeResponse(void                *aContext,
                                          otMessage           *aMessage,
                                          const otMessageInfo *aMessageInfo,
                                          Error                aResult)
{
    static_cast<Joiner *>(aContext)->HandleJoinerFinalizeResponse(AsCoapMessagePtr(aMessage), &AsCoreType(aMessageInfo),
                                                                  aResult);
}

void Joiner::HandleJoinerFinalizeResponse(Coap::Message *aMessage, const Ip6::MessageInfo *aMessageInfo, Error aResult)
{
    OT_UNUSED_VARIABLE(aMessageInfo);

    uint8_t state;

    VerifyOrExit(mState == kStateConnected && aResult == kErrorNone);
    OT_ASSERT(aMessage != nullptr);

    VerifyOrExit(aMessage->IsAck() && aMessage->GetCode() == Coap::kCodeChanged);

    SuccessOrExit(Tlv::Find<StateTlv>(*aMessage, state));

    SetState(kStateEntrust);
    mTimer.Start(kResponseTimeout);

    LogInfo("Received %s %d", UriToString<kUriJoinerFinalize>(), state);

#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
    LogCertMessage("[THCI] direction=recv | type=JOIN_FIN.rsp |", *aMessage);
#endif

exit:
    Get<Tmf::SecureAgent>().Disconnect();
    IgnoreError(Get<Ip6::Filter>().RemoveUnsecurePort(kJoinerUdpPort));
}

template <> void Joiner::HandleTmf<kUriJoinerEntrust>(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    Error         error;
    Dataset::Info datasetInfo;

    VerifyOrExit(mState == kStateEntrust && aMessage.IsConfirmablePostRequest(), error = kErrorDrop);

    LogInfo("Received %s", UriToString<kUriJoinerEntrust>());
    LogCert("[THCI] direction=recv | type=JOIN_ENT.ntf");

    datasetInfo.Clear();

    SuccessOrExit(error = Tlv::Find<NetworkKeyTlv>(aMessage, datasetInfo.Update<Dataset::kNetworkKey>()));

    datasetInfo.Set<Dataset::kChannel>(Get<Mac::Mac>().GetPanChannel());
    datasetInfo.Set<Dataset::kPanId>(Get<Mac::Mac>().GetPanId());

    Get<ActiveDatasetManager>().SaveLocal(datasetInfo);

    LogInfo("Joiner successful!");

    SendJoinerEntrustResponse(aMessage, aMessageInfo);

    // Delay extended address configuration to allow DTLS wrap up.
    mTimer.Start(kConfigExtAddressDelay);

exit:
    LogWarnOnError(error, "process joiner entrust");
}

void Joiner::SendJoinerEntrustResponse(const Coap::Message &aRequest, const Ip6::MessageInfo &aRequestInfo)
{
    Error            error = kErrorNone;
    Coap::Message   *message;
    Ip6::MessageInfo responseInfo(aRequestInfo);

    message = Get<Tmf::Agent>().NewPriorityResponseMessage(aRequest);
    VerifyOrExit(message != nullptr, error = kErrorNoBufs);

    message->SetSubType(Message::kSubTypeJoinerEntrust);

    responseInfo.GetSockAddr().Clear();
    SuccessOrExit(error = Get<Tmf::Agent>().SendMessage(*message, responseInfo));

    SetState(kStateJoined);

    LogInfo("Sent %s response", UriToString<kUriJoinerEntrust>());
    LogCert("[THCI] direction=send | type=JOIN_ENT.rsp");

exit:
    FreeMessageOnError(message, error);
}

void Joiner::HandleTimer(void)
{
    Error error = kErrorNone;

    switch (mState)
    {
    case kStateConnected:
    case kStateEntrust:
        error = kErrorResponseTimeout;
        break;

    case kStateJoined:
        Mac::ExtAddress extAddress;

        extAddress.GenerateRandom();
        Get<Mac::Mac>().SetExtAddress(extAddress);
        Get<Mle::MleRouter>().UpdateLinkLocalAddress();

        error = kErrorNone;
        break;

    case kStateIdle:
    case kStateDiscover:
    case kStateConnect:
        OT_ASSERT(false);
    }

    Finish(error);
}

// LCOV_EXCL_START

const char *Joiner::StateToString(State aState)
{
    static const char *const kStateStrings[] = {
        "Idle",       // (0) kStateIdle
        "Discover",   // (1) kStateDiscover
        "Connecting", // (2) kStateConnect
        "Connected",  // (3) kStateConnected
        "Entrust",    // (4) kStateEntrust
        "Joined",     // (5) kStateJoined
    };

    struct EnumCheck
    {
        InitEnumValidatorCounter();
        ValidateNextEnum(kStateIdle);
        ValidateNextEnum(kStateDiscover);
        ValidateNextEnum(kStateConnect);
        ValidateNextEnum(kStateConnected);
        ValidateNextEnum(kStateEntrust);
        ValidateNextEnum(kStateJoined);
    };

    return kStateStrings[aState];
}

#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
void Joiner::LogCertMessage(const char *aText, const Coap::Message &aMessage) const
{
    OT_UNUSED_VARIABLE(aText);

    uint8_t buf[OPENTHREAD_CONFIG_MESSAGE_BUFFER_SIZE];

    VerifyOrExit(aMessage.GetLength() <= sizeof(buf));
    aMessage.ReadBytes(aMessage.GetOffset(), buf, aMessage.GetLength() - aMessage.GetOffset());

    DumpCert(aText, buf, aMessage.GetLength() - aMessage.GetOffset());

exit:
    return;
}
#endif

// LCOV_EXCL_STOP

} // namespace MeshCoP
} // namespace ot

#endif // OPENTHREAD_CONFIG_JOINER_ENABLE
