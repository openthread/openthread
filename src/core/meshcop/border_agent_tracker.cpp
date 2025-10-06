/*
 *  Copyright (c) 2025, The OpenThread Authors.
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
 *   This file implements the Border Agent Tracker.
 */

#include "border_agent_tracker.hpp"

#if OPENTHREAD_CONFIG_BORDER_AGENT_TRACKER_ENABLE

#include "instance/instance.hpp"

namespace ot {
namespace MeshCoP {

RegisterLogModule("BaTracker");

//---------------------------------------------------------------------------------------------------------------------
// BorderAgentTracker

const char BorderAgentTracker::kServiceType[] = "_meshcop._udp";

BorderAgentTracker::BorderAgentTracker(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mState(kStateStopped)
    , mUserEnabled(false)
    , mStackEnabled(false)
{
}

void BorderAgentTracker::SetEnabled(bool aEnable, Requester aRequester)
{
    switch (aRequester)
    {
    case kRequesterUser:
        mUserEnabled = aEnable;
        break;
    case kRequesterStack:
        mStackEnabled = aEnable;
        break;
    }

    UpdateState();
}

void BorderAgentTracker::HandleDnssdPlatformStateChange(void) { UpdateState(); }

void BorderAgentTracker::UpdateState(void)
{
    State newState;

    if (mUserEnabled || mStackEnabled)
    {
        newState = Get<Dnssd>().IsReady() ? kStateRunning : kStatePendingDnssd;
    }
    else
    {
        newState = kStateStopped;
    }

    VerifyOrExit(newState != mState);

    if (mState == kStateRunning)
    {
        Get<Dnssd>().StopBrowser(Browser());
        mAgents.Free();
    }

    LogInfo("State: %s -> %s", StateToString(mState), StateToString(newState));
    mState = newState;

    // It is important to start browser after `mState` is updated.
    // This ensures that if the `HandleBrowseResult()` callback is
    // invoked immediately from within the `StartBrowser()` method
    // call, the state is valid.

    if (newState == kStateRunning)
    {
        Get<Dnssd>().StartBrowser(Browser());
    }

exit:
    return;
}

void BorderAgentTracker::HandleBrowseResult(otInstance *aInstance, const otPlatDnssdBrowseResult *aResult)
{
    AsCoreType(aInstance).Get<BorderAgentTracker>().HandleBrowseResult(*aResult);
}

void BorderAgentTracker::HandleBrowseResult(const Dnssd::BrowseResult &aResult)
{
    Error  error = kErrorNone;
    Agent *newAgent;

    VerifyOrExit(IsRunning());

    VerifyOrExit(aResult.mServiceInstance != nullptr);

    if (aResult.mTtl == 0)
    {
        mAgents.RemoveMatching(kMatchServiceName, aResult.mServiceInstance);
        ExitNow();
    }

    VerifyOrExit(!mAgents.ContainsMatching(kMatchServiceName, aResult.mServiceInstance));

    LogInfo("Discovered agent %s", aResult.mServiceInstance);

    newAgent = Agent::Allocate(GetInstance());
    VerifyOrExit(newAgent != nullptr, error = kErrorNoBufs);

    // We add the new agent to the list first before setting service
    // name and starting the SRV and TXT resolvers. This ensures that
    // if the `HandleSrvResult()` or `HandleTxtResult()` callbacks
    // are invoked immediately from within the method call that start
    // the resolvers, the agent entry can be correctly found in the
    // list.

    mAgents.Push(*newAgent);

    if (newAgent->SetServiceNameAndStartSrvTxtResolvers(aResult.mServiceInstance) != kErrorNone)
    {
        // `Pop()` returns a temporary `OwnedPtr<Agent>` which is
        // immediately destroyed, freeing the allocated agent.
        mAgents.Pop();
        error = kErrorNoBufs;
    }

exit:
    LogOnError(error, "add new agent", aResult.mServiceInstance);
}

void BorderAgentTracker::HandleSrvResult(otInstance *aInstance, const otPlatDnssdSrvResult *aResult)
{
    AsCoreType(aInstance).Get<BorderAgentTracker>().HandleSrvResult(*aResult);
}

void BorderAgentTracker::HandleSrvResult(const Dnssd::SrvResult &aResult)
{
    Agent *agent;

    VerifyOrExit(IsRunning());

    agent = mAgents.FindMatching(kMatchServiceName, aResult.mServiceInstance);
    VerifyOrExit(agent != nullptr);

    if (aResult.mTtl == 0)
    {
        agent->SetPort(0);
        agent->ClearHost();
        ExitNow();
    }

    agent->SetPort(aResult.mPort);

    VerifyOrExit(aResult.mHostName != nullptr);
    agent->SetHost(aResult.mHostName);

exit:
    return;
}

void BorderAgentTracker::HandleTxtResult(otInstance *aInstance, const otPlatDnssdTxtResult *aResult)
{
    AsCoreType(aInstance).Get<BorderAgentTracker>().HandleTxtResult(*aResult);
}

void BorderAgentTracker::HandleTxtResult(const Dnssd::TxtResult &aResult)
{
    Agent *agent;

    VerifyOrExit(IsRunning());

    agent = mAgents.FindMatching(kMatchServiceName, aResult.mServiceInstance);
    VerifyOrExit(agent != nullptr);

    if ((aResult.mTtl == 0) || (aResult.mTxtData == nullptr))
    {
        agent->ClearTxtData();
    }
    else
    {
        agent->SetTxtData(aResult.mTxtData, aResult.mTxtDataLength);
    }

exit:
    return;
}

void BorderAgentTracker::HandleAddressResult(otInstance *aInstance, const otPlatDnssdAddressResult *aResult)
{
    AsCoreType(aInstance).Get<BorderAgentTracker>().HandleAddressResult(*aResult);
}

void BorderAgentTracker::HandleAddressResult(const Dnssd::AddressResult &aResult)
{
    Agent *agent;

    VerifyOrExit(IsRunning());

    agent = mAgents.FindMatching(kMatchHostName, aResult.mHostName);
    VerifyOrExit(agent != nullptr);

    agent->mHost->SetAddresses(aResult);

exit:
    return;
}

bool BorderAgentTracker::NameMatch(const Heap::String &aHeapString, const char *aName)
{
    return !aHeapString.IsNull() && StringMatch(aHeapString.AsCString(), aName, kStringCaseInsensitiveMatch);
}

#if OT_SHOULD_LOG_AT(OT_LOG_LEVEL_WARN)

void BorderAgentTracker::LogOnError(Error aError, const char *aText, const char *aName)
{
    if (aError != kErrorNone)
    {
        LogWarn("Error %s - Failed to %s - %s", ErrorToString(aError), aText, (aName != nullptr) ? aName : "");
    }
}

#else

void BorderAgentTracker::LogOnError(Error, const char *, const char *) {}

#endif

const char *BorderAgentTracker::StateToString(State aState)
{
    static const char *const kStateStrings[] = {
        "Stopped",
        "PendingDnssd",
        "Running",
    };

    struct EnumCheck
    {
        InitEnumValidatorCounter();
        ValidateNextEnum(kStateStopped);
        ValidateNextEnum(kStatePendingDnssd);
        ValidateNextEnum(kStateRunning);
    };

    return kStateStrings[aState];
}

//---------------------------------------------------------------------------------------------------------------------
// BorderAgentTracker::Iterator

void BorderAgentTracker::Iterator::Init(Instance &aInstance)
{
    SetAgentEntry(aInstance.Get<BorderAgentTracker>().mAgents.GetHead());
    SetInitUptime(aInstance.Get<Uptime>().GetUptime());
}

Error BorderAgentTracker::Iterator::GetNextAgentInfo(AgentInfo &aInfo)
{
    Error        error = kErrorNone;
    const Agent *agent = GetAgentEntry();

    VerifyOrExit(agent != nullptr, error = kErrorNotFound);
    agent->CopyInfoTo(aInfo, GetInitUptime());
    SetAgentEntry(agent->GetNext());

exit:
    return error;
}

//---------------------------------------------------------------------------------------------------------------------
// BorderAgentTracker::Host

BorderAgentTracker::Host::Host(Instance &aInstance)
    : InstanceLocator(aInstance)
{
}

BorderAgentTracker::Host::~Host(void)
{
    VerifyOrExit(mName != nullptr);
    Get<Dnssd>().StopIp6AddressResolver(AddressResolver(mName.AsCString()));

exit:
    return;
}

Error BorderAgentTracker::Host::SetNameAndStartAddrResolver(const char *aHostName)
{
    Error error;

    SuccessOrExit(error = mName.Set(aHostName));
    Get<Dnssd>().StartIp6AddressResolver(AddressResolver(mName.AsCString()));

exit:
    LogOnError(error, "set host name", aHostName);
    return error;
}

void BorderAgentTracker::Host::SetAddresses(const Dnssd::AddressResult &aResult)
{
    Error                       error = kErrorNone;
    uint16_t                    length;
    const Dnssd::AddressAndTtl *addrAndTtl;

    mAddresses.Free();

    length = aResult.mAddressesLength;

    SuccessOrExit(error = mAddresses.ReserveCapacity(length));

    for (addrAndTtl = aResult.mAddresses; length > 0; length--, addrAndTtl++)
    {
        const Ip6::Address &addr = AsCoreType(&addrAndTtl->mAddress);

        if (addrAndTtl->mTtl == 0)
        {
            continue;
        }

        if (!mAddresses.Contains(addr))
        {
            SuccessOrAssert(mAddresses.PushBack(addr));
        }
    }

exit:
    LogOnError(error, "set host addresses", mName.AsCString());
}

//---------------------------------------------------------------------------------------------------------------------
// BorderAgentTracker::Agent

BorderAgentTracker::Agent::Agent(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mNext(nullptr)
    , mDiscoverUptime(aInstance.Get<Uptime>().GetUptime())
    , mLastUpdateUptime(mDiscoverUptime)
    , mPort(0)
{
}

BorderAgentTracker::Agent::~Agent(void)
{
    VerifyOrExit(mServiceName != nullptr);
    Get<Dnssd>().StopSrvResolver(SrvResolver(mServiceName.AsCString()));
    Get<Dnssd>().StopTxtResolver(TxtResolver(mServiceName.AsCString()));

exit:
    return;
}

Error BorderAgentTracker::Agent::SetServiceNameAndStartSrvTxtResolvers(const char *aServiceName)
{
    Error error;

    SuccessOrExit(error = mServiceName.Set(aServiceName));

    SetUpdateTimeToNow();

    Get<Dnssd>().StartSrvResolver(SrvResolver(mServiceName.AsCString()));
    Get<Dnssd>().StartTxtResolver(TxtResolver(mServiceName.AsCString()));

exit:
    return error;
}

void BorderAgentTracker::Agent::SetHost(const char *aHostName)
{
    Agent *matchingHostAgent;

    if (mHost != nullptr)
    {
        VerifyOrExit(!NameMatch(mHost->mName, aHostName));
    }

    SetUpdateTimeToNow();

    // We handle the case where multiple meshcop services are
    // advertised from the same host. While this is unlikely in
    // actual deployments, it can be useful for testing. To minimize
    // resource usage (memory and mDNS queries), we check if another
    // `Agent` is already tracking the same host. If so, we share its
    // `Host` entry. Otherwise, we allocate a new one. Note that
    // `mHost` is defined as `RetainPtr` which does ref-counting.

    matchingHostAgent = Get<BorderAgentTracker>().mAgents.FindMatching(kMatchHostName, aHostName);

    if (matchingHostAgent != nullptr)
    {
        mHost = matchingHostAgent->mHost;
    }
    else
    {
        mHost.Reset(Host::Allocate(GetInstance()));
        VerifyOrExit(mHost != nullptr);

        if (mHost->SetNameAndStartAddrResolver(aHostName) != kErrorNone)
        {
            ClearHost();
        }
    }

exit:
    return;
}

void BorderAgentTracker::Agent::ClearHost(void)
{
    VerifyOrExit(mHost != nullptr);

    mHost.Reset();
    SetUpdateTimeToNow();

exit:
    return;
}

void BorderAgentTracker::Agent::SetPort(uint16_t aPort)
{
    VerifyOrExit(mPort != aPort);

    SetUpdateTimeToNow();
    mPort = aPort;

exit:
    return;
}

void BorderAgentTracker::Agent::SetTxtData(const uint8_t *aData, uint16_t aDataLength)
{
    Error error = kErrorNone;

    VerifyOrExit(!mTxtData.Matches(aData, aDataLength));

    SuccessOrExit(error = mTxtData.SetFrom(aData, aDataLength));
    SetUpdateTimeToNow();

exit:
    LogOnError(error, "set TXT data", mServiceName.AsCString());
}

void BorderAgentTracker::Agent::ClearTxtData(void)
{
    VerifyOrExit(!mTxtData.IsNull());

    SetUpdateTimeToNow();
    mTxtData.Free();

exit:
    return;
}

void BorderAgentTracker::Agent::SetUpdateTimeToNow(void) { mLastUpdateUptime = Get<Uptime>().GetUptime(); }

bool BorderAgentTracker::Agent::Matches(MatchType aType, const char *aName) const
{
    bool matches = false;

    switch (aType)
    {
    case kMatchServiceName:
        matches = NameMatch(mServiceName, aName);
        break;

    case kMatchHostName:
        VerifyOrExit(mHost != nullptr);
        matches = NameMatch(mHost->mName, aName);
        break;
    }

exit:
    return matches;
}

void BorderAgentTracker::Agent::CopyInfoTo(AgentInfo &aInfo, uint64_t aUptimeNow) const
{
    ClearAllBytes(aInfo);

    aInfo.mServiceName         = mServiceName.AsCString();
    aInfo.mPort                = mPort;
    aInfo.mTxtData             = mTxtData.GetBytes();
    aInfo.mTxtDataLength       = mTxtData.GetLength();
    aInfo.mMsecSinceDiscovered = aUptimeNow - mDiscoverUptime;
    aInfo.mMsecSinceLastChange = aUptimeNow - mLastUpdateUptime;

    if (mHost != nullptr)
    {
        aInfo.mHostName     = mHost->mName.AsCString();
        aInfo.mAddresses    = mHost->mAddresses.AsCArray();
        aInfo.mNumAddresses = mHost->mAddresses.GetLength();
    }
}

//----------------------------------------------------------------------------------------------------------------------
// BorderAgentTracker::Browser

BorderAgentTracker::Browser::Browser(void)
{
    Clear();
    mServiceType = kServiceType;
    mCallback    = BorderAgentTracker::HandleBrowseResult;
}

//----------------------------------------------------------------------------------------------------------------------
// BorderAgentTracker::SrvResolver

BorderAgentTracker::SrvResolver::SrvResolver(const char *aServiceName)
{
    Clear();
    mServiceInstance = aServiceName;
    mServiceType     = kServiceType;
    mCallback        = BorderAgentTracker::HandleSrvResult;
}

//----------------------------------------------------------------------------------------------------------------------
// BorderAgentTracker::TxtResolver

BorderAgentTracker::TxtResolver::TxtResolver(const char *aServiceName)
{
    Clear();
    mServiceInstance = aServiceName;
    mServiceType     = kServiceType;
    mCallback        = BorderAgentTracker::HandleTxtResult;
}

//----------------------------------------------------------------------------------------------------------------------
// BorderAgentTracker::AddressResolver

BorderAgentTracker::AddressResolver::AddressResolver(const char *aHostName)
{
    Clear();
    mHostName = aHostName;
    mCallback = BorderAgentTracker::HandleAddressResult;
}

} // namespace MeshCoP
} // namespace ot

#endif // OPENTHREAD_CONFIG_BORDER_AGENT_TRACKER_ENABLE
