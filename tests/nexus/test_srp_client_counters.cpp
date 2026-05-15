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

#include <stdio.h>

#include "platform/nexus_core.hpp"
#include "platform/nexus_node.hpp"

#if OPENTHREAD_CONFIG_SRP_CLIENT_COUNTERS_ENABLE

namespace ot {
namespace Nexus {

static constexpr uint32_t kFormNetworkTime  = 13 * 1000;
static constexpr uint32_t kJoinNetworkTime  = 10 * 1000;
static constexpr uint32_t kRegistrationTime = 5 * 1000;

static constexpr uint32_t kLeaseTime    = 60;
static constexpr uint32_t kKeyLeaseTime = 240;

static const char         kSrpServiceType[]   = "_ipps._tcp";
static const char         kSrpInstanceName[]  = "my-service";
static const char         kSrpInstanceName2[] = "my-service-2";
static const char         kSrpHostName[]      = "my-host";
static const char         kSrpHostAddress[]   = "2001::1";
static constexpr uint16_t kSrpServicePort     = 12345;
static constexpr uint16_t kSrpServicePort2    = 12346;

void TestSrpClientCounters(void)
{
    Core                 nexus;
    Ip6::Address         srpHostAddress;
    Srp::Client::Service srpService;
    Srp::Client::Service srpService2;

    Node &server = nexus.CreateNode();
    Node &client = nexus.CreateNode();

    server.SetName("SRP_SERVER");
    client.SetName("SRP_CLIENT");

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelNote));

    Log("Step 0: Form network and bring up server.");

    server.Form();
    nexus.AdvanceTime(kFormNetworkTime);

    {
        Srp::Server::LeaseConfig leaseConfig;
        leaseConfig.mMinLease    = kLeaseTime;
        leaseConfig.mMaxLease    = kLeaseTime;
        leaseConfig.mMinKeyLease = kKeyLeaseTime;
        leaseConfig.mMaxKeyLease = kKeyLeaseTime;
        SuccessOrQuit(server.Get<Srp::Server>().SetLeaseConfig(leaseConfig));
    }
    server.Get<Srp::Server>().SetEnabled(true);
    nexus.AdvanceTime(5 * 1000);

    client.Join(server, Node::kAsFed);
    nexus.AdvanceTime(kJoinNetworkTime);

    Log("Step 1: Verify counters start at zero.");
    {
        const otSrpClientCounters &counters = client.Get<Srp::Client>().GetCounters();

        VerifyOrQuit(counters.mTxUpdates == 0);
        VerifyOrQuit(counters.mUpdateAttempts == 0);
        VerifyOrQuit(counters.mSuccess == 0);
        VerifyOrQuit(counters.mRejectedDuplicate == 0);
        VerifyOrQuit(counters.mRejectedSecurity == 0);
        VerifyOrQuit(counters.mRejectedOther == 0);
        VerifyOrQuit(counters.mTimeouts == 0);
        VerifyOrQuit(counters.mHostAddressChanges == 0);
        VerifyOrQuit(counters.mServerChanges == 0);
        VerifyOrQuit(counters.mServiceAdds == 0);
        VerifyOrQuit(counters.mServiceRemoves == 0);
        VerifyOrQuit(counters.mServiceClears == 0);
        VerifyOrQuit(counters.mHostAndServicesRemoves == 0);
        VerifyOrQuit(counters.mHostAndServicesClears == 0);
        VerifyOrQuit(counters.mTxTotalBytes == 0);
        VerifyOrQuit(counters.mRegisteredTime == 0);
        VerifyOrQuit(counters.mAnycastAvailableTime == 0);
        VerifyOrQuit(counters.mUnicastAvailableTime == 0);
    }

    Log("Step 2: Register a service and verify Tx/Attempts/Success/ServerChanges/UnicastAvailable counters advance.");

    client.Get<Srp::Client>().EnableAutoStartMode(nullptr, nullptr);
    SuccessOrQuit(client.Get<Srp::Client>().SetHostName(kSrpHostName));
    SuccessOrQuit(srpHostAddress.FromString(kSrpHostAddress));
    SuccessOrQuit(client.Get<Srp::Client>().SetHostAddresses(&srpHostAddress, 1));

    ClearAllBytes(srpService);
    srpService.mName         = kSrpServiceType;
    srpService.mInstanceName = kSrpInstanceName;
    srpService.mPort         = kSrpServicePort;
    SuccessOrQuit(client.Get<Srp::Client>().AddService(srpService));

    nexus.AdvanceTime(kRegistrationTime);

    VerifyOrQuit(client.Get<Srp::Client>().GetHostInfo().GetState() == Srp::Client::kRegistered);

    {
        const otSrpClientCounters &counters = client.Get<Srp::Client>().GetCounters();

        VerifyOrQuit(counters.mTxUpdates >= 1);
        VerifyOrQuit(counters.mUpdateAttempts >= 1);
        VerifyOrQuit(counters.mSuccess >= 1);
        VerifyOrQuit(counters.mServerChanges >= 1);
        VerifyOrQuit(counters.mServiceAdds == 1);
        VerifyOrQuit(counters.mHostAddressChanges >= 1);
        VerifyOrQuit(counters.mTxTotalBytes > 0);
        VerifyOrQuit(counters.mTrackedTime > 0);
        VerifyOrQuit(counters.mRegisteredTime <= counters.mTrackedTime);
        VerifyOrQuit(counters.mUnicastAvailableTime > 0);
        VerifyOrQuit(counters.mUnicastAvailableTime <= counters.mRegisteredTime);
        VerifyOrQuit(counters.mAnycastAvailableTime == 0);

        // Wire-level transmissions are at least as numerous as logical transactions.
        VerifyOrQuit(counters.mTxUpdates >= counters.mUpdateAttempts);

        // Every completed transaction was attempted.
        VerifyOrQuit(counters.mUpdateAttempts >= counters.mSuccess + counters.mRejectedDuplicate +
                                                     counters.mRejectedSecurity + counters.mRejectedOther);
    }

    Log("Step 3: Wait for a refresh cycle and verify counters keep advancing.");
    {
        const otSrpClientCounters &before                 = client.Get<Srp::Client>().GetCounters();
        uint32_t                   txBefore               = before.mTxUpdates;
        uint32_t                   attemptsBefore         = before.mUpdateAttempts;
        uint32_t                   successBefore          = before.mSuccess;
        uint64_t                   unicastAvailableBefore = before.mUnicastAvailableTime;

        nexus.AdvanceTime(kLeaseTime * 1000);

        const otSrpClientCounters &after = client.Get<Srp::Client>().GetCounters();

        VerifyOrQuit(after.mTxUpdates > txBefore);
        VerifyOrQuit(after.mUpdateAttempts > attemptsBefore);
        VerifyOrQuit(after.mSuccess > successBefore);
        VerifyOrQuit(after.mRegisteredTime > 0);
        VerifyOrQuit(after.mUnicastAvailableTime > unicastAvailableBefore);
    }

    Log("Step 4: Add a second service, remove the first, clear the second; verify Service{Adds,Removes,Clears}.");
    {
        ClearAllBytes(srpService2);
        srpService2.mName         = kSrpServiceType;
        srpService2.mInstanceName = kSrpInstanceName2;
        srpService2.mPort         = kSrpServicePort2;
        SuccessOrQuit(client.Get<Srp::Client>().AddService(srpService2));

        nexus.AdvanceTime(kRegistrationTime);

        VerifyOrQuit(client.Get<Srp::Client>().GetCounters().mServiceAdds == 2);

        SuccessOrQuit(client.Get<Srp::Client>().RemoveService(srpService));

        nexus.AdvanceTime(kRegistrationTime);

        VerifyOrQuit(client.Get<Srp::Client>().GetCounters().mServiceRemoves == 1);

        SuccessOrQuit(client.Get<Srp::Client>().ClearService(srpService2));

        VerifyOrQuit(client.Get<Srp::Client>().GetCounters().mServiceClears == 1);
    }

    Log("Step 5: Remove host & services, then clear; verify HostAndServices{Removes,Clears}.");
    {
        SuccessOrQuit(client.Get<Srp::Client>().RemoveHostAndServices(/* aShouldRemoveKeyLease */ false,
                                                                      /* aSendUnregToServer */ false));

        nexus.AdvanceTime(kRegistrationTime);

        VerifyOrQuit(client.Get<Srp::Client>().GetCounters().mHostAndServicesRemoves == 1);

        client.Get<Srp::Client>().ClearHostAndServices();

        VerifyOrQuit(client.Get<Srp::Client>().GetCounters().mHostAndServicesClears == 1);
    }

    Log("Step 6: Reset counters and verify all event counters return to zero.");
    {
        client.Get<Srp::Client>().ResetCounters();

        const otSrpClientCounters &counters = client.Get<Srp::Client>().GetCounters();

        VerifyOrQuit(counters.mTxUpdates == 0);
        VerifyOrQuit(counters.mUpdateAttempts == 0);
        VerifyOrQuit(counters.mSuccess == 0);
        VerifyOrQuit(counters.mRejectedDuplicate == 0);
        VerifyOrQuit(counters.mRejectedSecurity == 0);
        VerifyOrQuit(counters.mRejectedOther == 0);
        VerifyOrQuit(counters.mTimeouts == 0);
        VerifyOrQuit(counters.mHostAddressChanges == 0);
        VerifyOrQuit(counters.mServerChanges == 0);
        VerifyOrQuit(counters.mServiceAdds == 0);
        VerifyOrQuit(counters.mServiceRemoves == 0);
        VerifyOrQuit(counters.mServiceClears == 0);
        VerifyOrQuit(counters.mHostAndServicesRemoves == 0);
        VerifyOrQuit(counters.mHostAndServicesClears == 0);
        VerifyOrQuit(counters.mTxTotalBytes == 0);
        VerifyOrQuit(counters.mRegisteredTime == 0);
        VerifyOrQuit(counters.mAnycastAvailableTime == 0);
        VerifyOrQuit(counters.mUnicastAvailableTime == 0);
        VerifyOrQuit(counters.mTrackedTime == 0);
    }

    Log("Step 7: Verify time tracking restarts cleanly after reset.");
    {
        nexus.AdvanceTime(kRegistrationTime);

        const otSrpClientCounters &counters = client.Get<Srp::Client>().GetCounters();

        VerifyOrQuit(counters.mTrackedTime > 0);
        VerifyOrQuit(counters.mRegisteredTime <= counters.mTrackedTime);
    }

    // Note: mTimeouts, mRejected{Duplicate,Security,Other}, and mAnycastAvailableTime are
    // not exercised by this nexus test — those paths require packet manipulation or an
    // anycast-published SRP server. Their increment sites are simple single-line bumps
    // verified by code review.
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::TestSrpClientCounters();
    printf("All tests passed\n");
    return 0;
}

#else // OPENTHREAD_CONFIG_SRP_CLIENT_COUNTERS_ENABLE

int main(void)
{
    printf("OPENTHREAD_CONFIG_SRP_CLIENT_COUNTERS_ENABLE is disabled, skipping test\n");
    return 0;
}

#endif // OPENTHREAD_CONFIG_SRP_CLIENT_COUNTERS_ENABLE
