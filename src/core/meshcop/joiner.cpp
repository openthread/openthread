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
    , mState(kStateIdle)
    , mFinalizeMessage(nullptr)
    , mTimer(aInstance)
{
    SetIdFromIeeeEui64();
    mDiscerner.Clear();
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
    Error      error;
    bool       shouldCleanup = false;
    JoinerPskd joinerPskd;

    LogInfo("Joiner starting");

    SuccessOrExit(error = Tlv::ValidateStringValue<ProvisioningUrlTlv>(aProvisioningUrl));
    SuccessOrExit(error = Tlv::ValidateStringValue<VendorNameTlv>(aVendorName));
    SuccessOrExit(error = Tlv::ValidateStringValue<VendorModelTlv>(aVendorModel));
    SuccessOrExit(error = Tlv::ValidateStringValue<VendorSwVersionTlv>(aVendorSwVersion));
    SuccessOrExit(error = joinerPskd.SetFrom(aPskd));

    VerifyOrExit(mState == kStateIdle, error = kErrorBusy);
    VerifyOrExit(!Get<Seeker>().IsRunning(), error = kErrorBusy);

    SuccessOrExit(error = Get<Tmf::SecureAgent>().Open(Ip6::NetifIdentifier::kNetifThreadInternal));

    // After this, if any of the steps fails, we need to cleanup
    // (free allocated message, stop seeker, close agent, etc).
    shouldCleanup = true;

    SuccessOrExit(error = Get<Tmf::SecureAgent>().Bind(Seeker::kUdpPort));
    Get<Tmf::SecureAgent>().SetConnectCallback(HandleSecureCoapClientConnect, this);
    Get<Tmf::SecureAgent>().SetPsk(joinerPskd);

    SuccessOrExit(error = PrepareJoinerFinalizeMessage(aProvisioningUrl, aVendorName, aVendorModel, aVendorSwVersion,
                                                       aVendorData));

    mCompletionCallback.Set(aCallback, aContext);
    SetState(kStateDiscover);

    error = Get<Seeker>().Start(EvaluateScanResult, this);

exit:
    if ((error != kErrorNone) && shouldCleanup)
    {
        FreeJoinerFinalizeMessage();
        Get<Tmf::SecureAgent>().Close();
        Get<Seeker>().Stop();
        SetState(kStateIdle);
    }

    LogWarnOnError(error, "start joiner");
    return error;
}

void Joiner::Stop(void)
{
    LogInfo("Joiner stopped");

    // Callback is set to `nullptr` to skip calling it from `Finish()`
    mCompletionCallback.Clear();
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
        mTimer.Stop();

        OT_FALL_THROUGH;

    case kStateDiscover:
        Get<Tmf::SecureAgent>().Close();
        Get<Seeker>().Stop();
        break;
    }

    SetState(kStateIdle);
    FreeJoinerFinalizeMessage();

    mCompletionCallback.InvokeIfSet(aError);

exit:
    return;
}

Seeker::Verdict Joiner::EvaluateScanResult(void *aContext, const Seeker::ScanResult *aResult)
{
    return static_cast<Joiner *>(aContext)->EvaluateScanResult(aResult);
}

Seeker::Verdict Joiner::EvaluateScanResult(const Seeker::ScanResult *aResult)
{
    Seeker::Verdict     verdict = Seeker::kIgnore;
    const SteeringData *steeringData;

    if (aResult == nullptr)
    {
        HandleScanCompleted();
        ExitNow();
    }

    steeringData = AsCoreTypePtr(&aResult->mSteeringData);

    // We prefer networks with an exact match of Joiner ID or
    // Discerner in the Steering Data compared to ones that allow all
    // Joiners.

    if (steeringData->PermitsAllJoiners())
    {
        verdict = Seeker::kAccept;
        ExitNow();
    }

    if (!mDiscerner.IsEmpty())
    {
        VerifyOrExit(steeringData->Contains(mDiscerner));
    }
    else
    {
        VerifyOrExit(steeringData->Contains(mId));
    }

    verdict = Seeker::kAcceptPreferred;

exit:
    return verdict;
}

void Joiner::HandleScanCompleted(void)
{
    VerifyOrExit(mState == kStateDiscover);

    Get<Mac::Mac>().SetExtAddress(mId);
    Get<Mle::Mle>().UpdateLinkLocalAddress();

    TryNextCandidate(kErrorNone);

exit:
    return;
}

void Joiner::TryNextCandidate(Error aPrevError)
{
    Error error;

    do
    {
        error = ConnectToNextCandidate();

        if (error == kErrorNone)
        {
            ExitNow();
        }

        // Save the error from `Connect` only if there is no previous
        // error from earlier attempts. This ensures that if there has
        // been a previous Joiner Router connect attempt where
        // `Connect()` call itself was successful, the error status
        // emitted from `Finish()` call corresponds to the error from
        // that attempt.

        aPrevError = (aPrevError == kErrorNone) ? error : aPrevError;

    } while (error != kErrorNotFound);

    Finish(aPrevError);

exit:
    return;
}

Error Joiner::ConnectToNextCandidate(void)
{
    Error         error;
    Ip6::SockAddr sockAddr;

    SuccessOrExit(error = Get<Seeker>().SetUpNextConnection(sockAddr));
    SuccessOrExit(error = Get<Tmf::SecureAgent>().Connect(sockAddr));
    SetState(kStateConnect);

exit:
    return error;
}

void Joiner::HandleSecureCoapClientConnect(Dtls::Session::ConnectEvent aEvent, void *aContext)
{
    static_cast<Joiner *>(aContext)->HandleSecureCoapClientConnect(aEvent);
}

void Joiner::HandleSecureCoapClientConnect(Dtls::Session::ConnectEvent aEvent)
{
    VerifyOrExit(mState == kStateConnect);

    if (aEvent == Dtls::Session::kConnected)
    {
        SetState(kStateConnected);
        SendJoinerFinalize();
        mTimer.Start(kResponseTimeout);
    }
    else
    {
        TryNextCandidate(kErrorSecurity);
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

    SuccessOrExit(Get<Tmf::SecureAgent>().SendMessage(*mFinalizeMessage, HandleJoinerFinalizeResponse, this));
    mFinalizeMessage = nullptr;

    LogInfo("Sent %s", UriToString<kUriJoinerFinalize>());

exit:
    return;
}

void Joiner::HandleJoinerFinalizeResponse(Coap::Msg *aMsg, Error aResult)
{
    uint8_t state;

    VerifyOrExit(mState == kStateConnected && aResult == kErrorNone);
    OT_ASSERT(aMsg != nullptr);

    VerifyOrExit(aMsg->IsAck() && aMsg->GetCode() == Coap::kCodeChanged);

    SuccessOrExit(Tlv::Find<StateTlv>(aMsg->mMessage, state));

    SetState(kStateEntrust);
    mTimer.Start(kResponseTimeout);

    LogInfo("Received %s %d", UriToString<kUriJoinerFinalize>(), state);

#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
    LogCertMessage("[THCI] direction=recv | type=JOIN_FIN.rsp |", aMsg->mMessage);
#endif

exit:
    Get<Tmf::SecureAgent>().Disconnect();
    Get<Seeker>().Stop();
}

template <> void Joiner::HandleTmf<kUriJoinerEntrust>(Coap::Msg &aMsg)
{
    Error         error;
    Dataset::Info datasetInfo;

    VerifyOrExit(mState == kStateEntrust && aMsg.IsConfirmablePostRequest(), error = kErrorDrop);

    LogInfo("Received %s", UriToString<kUriJoinerEntrust>());
    LogCert("[THCI] direction=recv | type=JOIN_ENT.ntf");

    datasetInfo.Clear();

    SuccessOrExit(error = Tlv::Find<NetworkKeyTlv>(aMsg.mMessage, datasetInfo.Update<Dataset::kNetworkKey>()));

    datasetInfo.Set<Dataset::kChannel>(Get<Mac::Mac>().GetPanChannel());
    datasetInfo.Set<Dataset::kPanId>(Get<Mac::Mac>().GetPanId());

    Get<ActiveDatasetManager>().SaveLocal(datasetInfo);

    LogInfo("Joiner successful!");

    SendJoinerEntrustResponse(aMsg);

    // Delay extended address configuration to allow DTLS wrap up.
    mTimer.Start(kConfigExtAddressDelay);

exit:
    LogWarnOnError(error, "process joiner entrust");
}

void Joiner::SendJoinerEntrustResponse(const Coap::Msg &aMsg)
{
    Error            error = kErrorNone;
    Coap::Message   *message;
    Ip6::MessageInfo responseInfo(aMsg.mMessageInfo);

    message = Get<Tmf::Agent>().NewPriorityResponseMessage(aMsg.mMessage);
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
        Get<Mle::Mle>().UpdateLinkLocalAddress();

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
#define StateMapList(_)             \
    _(kStateIdle, "Idle")           \
    _(kStateDiscover, "Discover")   \
    _(kStateConnect, "Connecting")  \
    _(kStateConnected, "Connected") \
    _(kStateEntrust, "Entrust")     \
    _(kStateJoined, "Joined")

    DefineEnumStringArray(StateMapList);

    return kStrings[aState];
}

// LCOV_EXCL_STOP

} // namespace MeshCoP
} // namespace ot

#endif // OPENTHREAD_CONFIG_JOINER_ENABLE
