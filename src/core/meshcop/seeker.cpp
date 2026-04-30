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

#if OPENTHREAD_CONFIG_SEEKER_ENABLE || OPENTHREAD_CONFIG_JOINER_ENABLE

#include "instance/instance.hpp"

namespace ot {
namespace MeshCoP {

RegisterLogModule("Seeker");

Seeker::Seeker(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mState(kStateStopped)
    , mUdpPort(kDefaultUdpPort)
    , mCandidates(aInstance)
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
    case kStateConnectingNetworks:
    case kStateConnectingAny:
        IgnoreError(Get<Ip6::Filter>().RemoveUnsecurePort(mUdpPort));
        break;
    }

    mCandidates.Clear();

    SetState(kStateStopped);
}

Error Seeker::SetUdpPort(uint16_t aUdpPort)
{
    Error error = kErrorNone;

    VerifyOrExit(!IsRunning(), error = kErrorInvalidState);

    VerifyOrExit(aUdpPort != mUdpPort);
    LogInfo("UDP port changed: %u -> %u", mUdpPort, aUdpPort);

    mUdpPort = aUdpPort;

exit:
    return error;
}

void Seeker::HandleDiscoverResult(const ScanResult *aResult)
{
    bool preferred = false;

    VerifyOrExit(GetState() == kStateDiscovering);

    if (aResult == nullptr)
    {
        SetState(kStateDiscoverDone);
        IgnoreReturnValue(mScanEvaluator.Invoke(aResult));
        ExitNow();
    }

    VerifyOrExit(aResult->GetJoinerUdpPort() > 0);
    VerifyOrExit(aResult->GetSteeringData().IsValid());

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
    Error          error           = kErrorNone;
    bool           shouldPushAsNew = false;
    CandidateEntry entry;

    entry.MarkAsEmpty();

    if (mCandidates.FindMatching(entry, aResult.GetExtendedPanId(), aResult.GetExtAddress()) == kErrorNone)
    {
        entry.Log(Candidate::kReplace);
    }
    else
    {
        uint16_t count = CountAndSelectLeastFavoredCandidateFor(aResult.GetExtendedPanId(), entry);

        if ((count == kMaxCandidatesPerNetwork) || ((count > 0) && mCandidates.IsFull()))
        {
            // We have reached the max allowed candidates for this
            // network (`extPanId`), or the array is full but we have
            // some existing entries for this network. In both cases
            // we replace the least favored existing entry with the
            // new scan result only if the new result is favored over
            // the existing one.

            if (entry.IsFavoredOver(aResult, aPreferred))
            {
                error = kErrorDrop;
            }
            else
            {
                entry.Log(Candidate::kReplace);
            }
        }
        else if (!mCandidates.IsFull())
        {
            shouldPushAsNew = true;
        }
        else
        {
            // When the array is full, we only evict an entry to make
            // room for a new network (one not yet in the list). We
            // do not evict to add redundancy (additional candidates)
            // for a network we have already discovered. We know
            // `count == 0` when we get here.

            error = EvictCandidate(entry);
        }
    }

    entry.SetFrom(aResult, aPreferred);

    // We check the `error` after `entry.SetFrom()` so that the
    // `entry.Log()` call at `exit` uses the proper candidate
    // entry from the scan `aResult`.

    SuccessOrExit(error);

    SuccessOrExit(error = shouldPushAsNew ? mCandidates.Push(entry) : mCandidates.Write(entry));

    entry.Log(Candidate::kSave);

exit:
    if (error != kErrorNone)
    {
        entry.Log(Candidate::kDrop);
    }
}

Error Seeker::EvictCandidate(CandidateEntry &aEntry)
{
    Error          error = kErrorNoBufs;
    CandidateEntry entry;

    aEntry.MarkAsEmpty();

    for (entry.InitForIteration(); mCandidates.ReadNext(entry) == kErrorNone;)
    {
        uint16_t count = CountAndSelectLeastFavoredCandidateFor(entry.mExtPanId, aEntry);

        if (count > 1)
        {
            aEntry.Log(Candidate::kEvict);
            error = kErrorNone;
            break;
        }
    }

    return error;
}

Error Seeker::SetUpNextConnection(Ip6::SockAddr &aSockAddr)
{
    Error          error = kErrorNone;
    CandidateEntry entry;

    switch (GetState())
    {
    case kStateDiscoverDone:
        SetState(kStateConnectingNetworks);
        break;

    case kStateConnectingNetworks:
    case kStateConnectingAny:
        break;

    case kStateStopped:
    case kStateDiscovering:
        ExitNow(error = kErrorInvalidState);
    }

    error = SelectNextCandidate(entry);

    if (error != kErrorNone)
    {
        Stop();
        ExitNow();
    }

    entry.Log(Candidate::kConnect);

    entry.mConnAttempted = true;
    IgnoreError(mCandidates.Write(entry));

    Get<Mac::Mac>().SetPanId(entry.mPanId);
    SuccessOrExit(error = Get<Mac::Mac>().SetPanChannel(entry.mChannel));

    if (!Get<Ip6::Filter>().IsUnsecurePort(mUdpPort))
    {
        SuccessOrExit(error = Get<Ip6::Filter>().AddUnsecurePort(mUdpPort));
    }

    aSockAddr.Clear();
    aSockAddr.SetPort(entry.mJoinerUdpPort);
    aSockAddr.GetAddress().SetToLinkLocalAddress(entry.mExtAddr);

exit:
    return error;
}

Error Seeker::SelectNextCandidate(CandidateEntry &aEntry)
{
    CandidateEntry entry;

    aEntry.MarkAsEmpty();

    if (mState == kStateConnectingNetworks)
    {
        // While in `kStateConnectingNetworks` we first try to cover all
        // discovered networks (Extended PAN IDs). We determine the most
        // favored candidate among all discovered networks which has not
        // yet been attempted.

        for (entry.InitForIteration(); mCandidates.ReadNext(entry) == kErrorNone;)
        {
            CandidateEntry matchingPanEntry;

            if (SelectMostFavoredCandidateFor(entry.mExtPanId, matchingPanEntry) == kErrorNone)
            {
                aEntry.ReplaceWithIfFavored(matchingPanEntry);
            }
        }

        if (!aEntry.IsEmpty())
        {
            ExitNow();
        }

        // If we have already covered the most favored candidate per Network
        // (Extended PAN ID), we switch to `kStateConnectingAny` where we
        // try any remaining discovered candidates (e.g. backup candidates
        // associated with the same networks).

        mState = kStateConnectingAny;
    }

    for (entry.InitForIteration(); mCandidates.ReadNext(entry) == kErrorNone;)
    {
        if (entry.mConnAttempted)
        {
            continue;
        }

        aEntry.ReplaceWithIfFavored(entry);
    }

exit:
    return aEntry.IsEmpty() ? kErrorNotFound : kErrorNone;
}

uint16_t Seeker::CountAndSelectLeastFavoredCandidateFor(const MeshCoP::ExtendedPanId &aExtPanId,
                                                        CandidateEntry               &aEntry) const
{
    // Iterates through all candidates matching a given Extended PAN ID.
    // Returns the total count of such entries. Also finds the least
    // favored matching entry and returns it in `aEntry`. This is then
    // used to decide whether to replace an existing candidate entry with
    // a new one.

    uint16_t       count = 0;
    CandidateEntry entry;

    aEntry.MarkAsEmpty();

    for (entry.InitForIteration(); mCandidates.ReadNext(entry) == kErrorNone;)
    {
        if (!entry.Matches(aExtPanId))
        {
            continue;
        }

        count++;

        if ((count == 1) || aEntry.IsFavoredOver(entry))
        {
            aEntry = entry;
        }
    }

    return count;
}

Error Seeker::SelectMostFavoredCandidateFor(const MeshCoP::ExtendedPanId &aExtPanId,
                                            CandidateEntry               &aFavoredEntry) const
{
    // Iterates through all candidates associated with a given network
    // (matching `aExtPanId`). If a connection has already been attempted
    // with any candidate from this network, `kErrorAlready` is returned.
    // Otherwise, the most favored candidate for this network is determined
    // and returned in `aFavoredEntry`.

    Error          error = kErrorNone;
    CandidateEntry entry;

    aFavoredEntry.MarkAsEmpty();

    for (entry.InitForIteration(); mCandidates.ReadNext(entry) == kErrorNone;)
    {
        if (!entry.Matches(aExtPanId))
        {
            continue;
        }

        if (entry.mConnAttempted)
        {
            error = kErrorAlready;
            ExitNow();
        }

        aFavoredEntry.ReplaceWithIfFavored(entry);
    }

exit:
    return error;
}

//---------------------------------------------------------------------------------------------------------------------
// Seeker::Candidate

void Seeker::Candidate::SetFrom(const ScanResult &aResult, bool aPreferred)
{
    mExtPanId      = aResult.GetExtendedPanId();
    mExtAddr       = aResult.GetExtAddress();
    mPanId         = aResult.GetPanId();
    mJoinerUdpPort = aResult.GetJoinerUdpPort();
    mChannel       = aResult.GetChannel();
    mRssi          = aResult.GetRssi();
    mPreferred     = aPreferred;
    mConnAttempted = false;
}

bool Seeker::Candidate::IsFavoredOver(const Candidate &aOther) const
{
    return IsFavoredOver(aOther.mRssi, aOther.mPreferred);
}

bool Seeker::Candidate::IsFavoredOver(const ScanResult &aResult, bool aPreferred) const
{
    return IsFavoredOver(aResult.GetRssi(), aPreferred);
}

bool Seeker::Candidate::IsFavoredOver(int8_t aRssi, bool aPreferred) const
{
    int compare;

    compare = ThreeWayCompare(mPreferred, aPreferred);
    VerifyOrExit(compare == 0);
    compare = ThreeWayCompare(mRssi, aRssi);

exit:
    return (compare > 0);
}

bool Seeker::Candidate::Matches(const MeshCoP::ExtendedPanId &aExtPanId, const Mac::ExtAddress &aExtAddr) const
{
    return (mExtPanId == aExtPanId) && (mExtAddr == aExtAddr);
}

#if OT_SHOULD_LOG_AT(OT_LOG_LEVEL_INFO)

const char *Seeker::Candidate::ActionToString(Action aAction)
{
#define ActionMapList(_)     \
    _(kSave, "Saving")       \
    _(kReplace, "Replacing") \
    _(kEvict, "Evicting")    \
    _(kDrop, "Dropping")     \
    _(kConnect, "Connecting to")

    DefineEnumStringArray(ActionMapList);

    return kStrings[aAction];
}

void Seeker::Candidate::Log(Action aAction) const
{
    LogInfo("%s candidate:", ActionToString(aAction));
    LogInfo("   ext-panid: %s", mExtPanId.ToString().AsCString());
    LogInfo("   ext-addr: %s", mExtAddr.ToString().AsCString());
    LogInfo("   panid: 0x%04x", mPanId);
    LogInfo("   channel: %u", mChannel);
    LogInfo("   rssi: %d", mRssi);
    LogInfo("   preferred: %s", ToYesNo(mPreferred));
    LogInfo("   joiner-port: %u", mJoinerUdpPort);
}

#else
void Seeker::Candidate::Log(Action) const {}
#endif

//----------------------------------------------------------------------------------------------------------------------
// Seeker::CandidateEntry

void Seeker::CandidateEntry::ReplaceWithIfFavored(const CandidateEntry &aEntry)
{
    // Replaces this entry with `aEntry` if this entry is currently
    // empty (not yet set) or if `aEntry` is favored over the
    // current one.

    if (IsEmpty() || aEntry.IsFavoredOver(*this))
    {
        *this = aEntry;
    }
}

} // namespace MeshCoP
} // namespace ot

#endif // OPENTHREAD_CONFIG_SEEKER_ENABLE || OPENTHREAD_CONFIG_JOINER_ENABLE
