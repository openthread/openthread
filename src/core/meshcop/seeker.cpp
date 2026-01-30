/*
 *  Copyright (c) 2026, The OpenThread Authors.
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
 *   This file implements the Seeker functionality.
 */

#include "seeker.hpp"

#if OPENTHREAD_CONFIG_JOINER_ENABLE

#include "instance/instance.hpp"

namespace ot {
namespace MeshCoP {

RegisterLogModule("Seeker");

Seeker::Seeker(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mState(kStateStopped)
    , mCandidateIndex(0)
{
}

Error Seeker::Start(ScanEvaluator aScanEvaluator, void *aContext)
{
    Error           error = kErrorNone;
    Mac::ExtAddress randomAddress;

    VerifyOrExit(aScanEvaluator != nullptr, error = kErrorInvalidArgs);

    VerifyOrExit(GetState() == kStateStopped, error = kErrorBusy);
    VerifyOrExit(Get<ThreadNetif>().IsUp() && Get<Mle::Mle>().IsDisabled(), error = kErrorInvalidState);

    randomAddress.GenerateRandom();
    Get<Mac::Mac>().SetExtAddress(randomAddress);
    Get<Mle::Mle>().UpdateLinkLocalAddress();

    mScanEvaluator.Set(aScanEvaluator, aContext);

    ClearAllBytes(mCandidates);
    mCandidateIndex = 0;

    SuccessOrExit(error =
                      Get<Mle::DiscoverScanner>().Discover(Mac::ChannelMask(0), Get<Mac::Mac>().GetPanId(),
                                                           /* aJoiner */ true, /* aEnableFiltering */ false,
                                                           /* aFilterIndexes */ nullptr, HandleDiscoverResult, this));
    SetState(kStateDiscovering);

exit:
    return error;
}

void Seeker::Stop(void)
{
    switch (GetState())
    {
    case kStateStopped:
    case kStateDiscovering:
    case kStateDiscoverDone:
        break;
    case kStateConnecting:
        IgnoreError(Get<Ip6::Filter>().RemoveUnsecurePort(kUdpPort));
        break;
    }

    SetState(kStateStopped);
}

void Seeker::HandleDiscoverResult(ScanResult *aResult, void *aContext)
{
    static_cast<Seeker *>(aContext)->HandleDiscoverResult(aResult);
}

void Seeker::HandleDiscoverResult(ScanResult *aResult)
{
    bool preferred = false;

    VerifyOrExit(GetState() == kStateDiscovering);

    if (aResult == nullptr)
    {
        SetState(kStateDiscoverDone);
        IgnoreReturnValue(mScanEvaluator.Invoke(aResult));
        ExitNow();
    }

    VerifyOrExit(aResult->mJoinerUdpPort > 0);
    VerifyOrExit(AsCoreType(&aResult->mSteeringData).IsValid());

    switch (mScanEvaluator.Invoke(aResult))
    {
    case kAccept:
        break;
    case kAcceptPreferred:
        preferred = true;
        break;

    case kIgnore:
    default:
        ExitNow();
    }

    SaveCandidate(*aResult, preferred);

exit:
    return;
}

void Seeker::SaveCandidate(const ScanResult &aResult, bool aPreferred)
{
    uint8_t    priority;
    Candidate *end;
    Candidate *entry;

    LogInfo("Discovered: %s, pan:0x%04x, port:%u, chan:%u, rssi:%d, preferred:%s",
            AsCoreType(&aResult.mExtAddress).ToString().AsCString(), aResult.mPanId, aResult.mJoinerUdpPort,
            aResult.mChannel, aResult.mRssi, ToYesNo(aPreferred));

    priority = CalculatePriority(aResult.mRssi, aPreferred);

    // We keep the list sorted based on priority. Find the place to
    // add the new result.

    end = GetArrayEnd(mCandidates);

    for (entry = &mCandidates[0]; entry < end; entry++)
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

uint8_t Seeker::CalculatePriority(int8_t aRssi, bool aPreferred)
{
    int16_t priority;

    if (aRssi == Radio::kInvalidRssi)
    {
        aRssi = -127;
    }

    priority = Clamp<int8_t>(aRssi, -127, -1);

    // We assign a higher priority value to networks marked as
    // preferred (128 < priority < 256) compared to normal
    // (0 < priority < 128). Sub-prioritize based on signal
    // strength. Priority 0 is reserved for unused entry.

    priority += aPreferred ? 256 : 128;

    return static_cast<uint8_t>(priority);
}

Error Seeker::SetUpNextConnection(Ip6::SockAddr &aSockAddr)
{
    Error            error = kErrorNone;
    const Candidate *candidate;

    switch (GetState())
    {
    case kStateDiscoverDone:
    case kStateConnecting:
        break;

    case kStateStopped:
    case kStateDiscovering:
        ExitNow(error = kErrorInvalidState);
    }

    candidate = &mCandidates[mCandidateIndex];

    if (!candidate->IsValid())
    {
        Stop();
        ExitNow(error = kErrorNotFound);
    }

    mCandidateIndex++;

    LogInfo("Setting up conn to %s, pan:0x%04x, chan:%u", candidate->mExtAddr.ToString().AsCString(), candidate->mPanId,
            candidate->mChannel);

    Get<Mac::Mac>().SetPanId(candidate->mPanId);
    SuccessOrExit(error = Get<Mac::Mac>().SetPanChannel(candidate->mChannel));

    if (!Get<Ip6::Filter>().IsUnsecurePort(kUdpPort))
    {
        SuccessOrExit(error = Get<Ip6::Filter>().AddUnsecurePort(kUdpPort));
    }

    SetState(kStateConnecting);

    aSockAddr.Clear();
    aSockAddr.SetPort(candidate->mJoinerUdpPort);
    aSockAddr.GetAddress().SetToLinkLocalAddress(candidate->mExtAddr);

exit:
    return error;
}

} // namespace MeshCoP
} // namespace ot

#endif // OPENTHREAD_CONFIG_JOINER_ENABLE
