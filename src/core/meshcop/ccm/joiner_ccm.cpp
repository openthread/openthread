/*
 *  Copyright (c) 2024, The OpenThread Authors.
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
 *   This file implements the CCM Joiner role.
 */

#include "../joiner.hpp"

#if OPENTHREAD_CONFIG_JOINER_ENABLE && OPENTHREAD_CONFIG_CCM_ENABLE

#include "common/code_utils.hpp"
#include "common/encoding.hpp"
#include "common/log.hpp"
#include "instance/instance.hpp"
#include "meshcop/meshcop.hpp"
#include "radio/radio.hpp"
#include "thread/thread_netif.hpp"
#include "thread/uri_paths.hpp"
#include "utils/otns.hpp"

namespace ot {
namespace MeshCoP {

RegisterLogModule("JoinerCcm");

Error Joiner::StartCcm(Operation aOperation, otJoinerCallback aCallback, void *aContext)
{
    Error                        error;
    Mac::ExtAddress              randomAddress;
    SteeringData::HashBitIndexes filterIndexes;
    Ip6::SockAddr                sockAddr;

    VerifyOrExit(mState == kStateIdle, error = kErrorBusy);
    VerifyOrExit((Get<ThreadNetif>().IsUp() || aOperation == kOperationCcmBrCbrski  || aOperation == kOperationCcmAll) &&
                     Get<Mle::Mle>().GetRole() == Mle::kRoleDisabled,
                 error = kErrorInvalidState);

    switch (aOperation)
    {
    case kOperationCcmAeCbrski:
        VerifyOrExit(!Get<Credentials>().HasOperationalCert(), error = kErrorInvalidState);
        SuccessOrExit(error = Get<Credentials>().ConfigureIdevid(&Get<Tmf::SecureAgent>().GetDtls()));
        mJoinerSourcePort = kCcmCbrskiJoinerUdpSourcePort;
        break;
#if OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE
    case kOperationCcmBrCbrski:
        VerifyOrExit(!Get<Credentials>().HasOperationalCert(), error = kErrorInvalidState);
        mJoinerSourcePort = 0; // ephemeral source port, also not a mesh 'unsecure' port.
        break;
#endif
    case kOperationCcmNkp:
        VerifyOrExit(Get<Credentials>().HasOperationalCert(), error = kErrorInvalidState);
        SuccessOrExit(error = PrepareCcmNkpJoinerFinalizeMessage());
        SuccessOrExit(error = Get<Credentials>().ConfigureLdevid(&Get<Tmf::SecureAgent>().GetDtls()));
        mJoinerSourcePort = kCcmNkpJoinerUdpSourcePort;
        break;
    case kOperationCcmAll:
        break;
    default:
        ExitNow(error = kErrorInvalidArgs);
    }

    mJoinerOperation = aOperation;
    LogInfo("Start operation %s (%d)", OperationToString(mJoinerOperation), mJoinerOperation);

    if (aOperation == kOperationCcmAll)
    {
        mCallbackCcmAll.Set(aCallback, aContext);
        error = StartCcmAll();
        ExitNow();
    }

#if OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE
    if (aOperation == kOperationCcmBrCbrski)
    {
        SuccessOrExit(error = Get<Credentials>().ConfigureIdevid(&Get<Tmf::SecureAgent>().GetDtls()));
        SuccessOrExit(error = Get<BorderRouter::InfraIf>().GetDiscoveredCcmRegistrarAddress(sockAddr.GetAddress()));
        sockAddr.SetPort(OT_DEFAULT_COAP_SECURE_PORT);
        SuccessOrExit(error = Get<Tmf::SecureAgent>().Start(mJoinerSourcePort));
        SuccessOrExit(error = Get<Tmf::SecureAgent>().Connect(sockAddr, Joiner::HandleSecureCoapClientConnect, this));
        SetState(kStateConnect);
    }
    else
#endif
    {
        // Use random-generated extended address.
        randomAddress.GenerateRandom();
        Get<Mac::Mac>().SetExtAddress(randomAddress);
        Get<Mle::MleRouter>().UpdateLinkLocalAddress();

        SuccessOrExit(error = Get<Tmf::SecureAgent>().Start(mJoinerSourcePort));

        for (JoinerRouter &router : mJoinerRouters)
        {
            router.mPriority = 0; // Priority zero means entry is not in-use.
        }

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
        SetState(kStateDiscover);
    }

    mCallback.Set(aCallback, aContext);

exit:
    LogWarnOnError(error, "start JoinerCcm");
    if (error != kErrorNone)
    {
        FreeJoinerFinalizeMessage(); // applies in certain NKP cases.
    }
    return error;
}

Error Joiner::StartCcmAll(void)
{
    Error error;
    bool done = false;
    bool isNeedAe = !Get<Credentials>().HasOperationalCert();
    bool isNeedNkp = !Get<ActiveDatasetManager>().IsComplete() && !Get<ActiveDatasetManager>().IsPartiallyComplete();
    bool isNeedUp = !Get<ThreadNetif>().IsUp();
    bool isNeedThreadStart = Get<Mle::MleRouter>().IsDisabled();

    LogDebg("StartCcmAll nAe=%d nNk=%d nUp=%d nTs=%d", isNeedAe, isNeedNkp, isNeedUp, isNeedThreadStart); // FIXME can be removed
    if (isNeedUp){
        Get<ThreadNetif>().Up();
    }

    if (isNeedAe){
        error = StartCcm(Joiner::Operation::kOperationCcmAeCbrski, HandleCcmAllOperationDone, this);
    }
    else if (isNeedNkp)
    {
        error = StartCcm(Joiner::Operation::kOperationCcmNkp, HandleCcmAllOperationDone, this);
    }
    else if (isNeedThreadStart)
    {
        error = Get<Mle::MleRouter>().Start();
    }
    else
    {
        mCallbackCcmAll.InvokeIfSet(kErrorNone);
        done = true;
        error = kErrorAbort; // set abort error to signal HandleCcmAllOperationDone() to stop recursion.
    }

    if (error != kErrorNone && !done)
    {
        mCallbackCcmAll.InvokeIfSet(error);
    }
    return error;
}

Error Joiner::PrepareCcmNkpJoinerFinalizeMessage(void)
{
    Error                 error = kErrorNone;
    VendorStackVersionTlv vendorStackVersionTlv;

    // TODO consider if different URI is better (see spec)
    mFinalizeMessage = Get<Tmf::SecureAgent>().NewPriorityConfirmablePostMessage(kUriJoinerFinalize);
    VerifyOrExit(mFinalizeMessage != nullptr, error = kErrorNoBufs);

    mFinalizeMessage->SetOffset(mFinalizeMessage->GetLength());

    SuccessOrExit(error = Tlv::Append<StateTlv>(*mFinalizeMessage, StateTlv::kAccept));

    vendorStackVersionTlv.Init();
    vendorStackVersionTlv.SetOui(OPENTHREAD_CONFIG_STACK_VENDOR_OUI);
    vendorStackVersionTlv.SetMajor(OPENTHREAD_CONFIG_STACK_VERSION_MAJOR);
    vendorStackVersionTlv.SetMinor(OPENTHREAD_CONFIG_STACK_VERSION_MINOR);
    vendorStackVersionTlv.SetRevision(OPENTHREAD_CONFIG_STACK_VERSION_REV);
    SuccessOrExit(error = vendorStackVersionTlv.AppendTo(*mFinalizeMessage));

exit:
    if (error != kErrorNone)
    {
        FreeJoinerFinalizeMessage();
    }

    return error;
}

void Joiner::HandleCcmAllOperationDone(Error aErr, void *aContext)
{
    static_cast<Joiner *>(aContext)->HandleCcmAllOperationDone(aErr);
}

void Joiner::HandleCcmAllOperationDone(Error aErr) {
    Error error;

    if (aErr == kErrorNone)
    {
        error = StartCcmAll(); // proceed with next required operation
    }
    else
    {
        error = aErr;
    }

    if (error != kErrorNone)
    {
        LogDebg("CCM 'all' operation finish (err=%s)", otThreadErrorToString(error));
        mCallbackCcmAll.InvokeIfSet(error);
        mCallbackCcmAll.Clear();
    }
}

void Joiner::HandleCbrskiClientDone(Error aErr, void *aContext)
{
    static_cast<Joiner *>(aContext)->HandleCbrskiClientDone(aErr);
}

void Joiner::HandleCbrskiClientDone(Error aErr) { Finish(aErr, /* aInvokeCallback */ true); }

} // namespace MeshCoP
} // namespace ot

#endif // OPENTHREAD_CONFIG_JOINER_ENABLE && && OPENTHREAD_CONFIG_CCM_ENABLE
