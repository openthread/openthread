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
#include "thread/net_diag.hpp"

#if OPENTHREAD_CONFIG_SRP_CLIENT_COUNTERS_ENABLE

namespace ot {
namespace Nexus {

static constexpr uint32_t kFormNetworkTime  = 13 * 1000;
static constexpr uint32_t kJoinNetworkTime  = 10 * 1000;
static constexpr uint32_t kRegistrationTime = 5 * 1000;
static constexpr uint32_t kDiagResponseTime = 5 * 1000;

static constexpr uint32_t kLeaseTime    = 60;
static constexpr uint32_t kKeyLeaseTime = 240;

static constexpr uint8_t kSrpCountersTlvSize = 92;

static const char         kSrpServiceType[]  = "_ipps._tcp";
static const char         kSrpInstanceName[] = "my-service";
static const char         kSrpHostName[]     = "my-host";
static const char         kSrpHostAddress[]  = "2001::1";
static constexpr uint16_t kSrpServicePort    = 12345;

struct DiagResult
{
    bool                mReceived;
    bool                mHasSrpCounters;
    uint8_t             mTlvCount;
    uint16_t            mSrpTlvLength;
    otSrpClientCounters mSrpCounters;
};

static DiagResult sDiagResult;

void HandleDiagGetResponse(otError aError, otMessage *aMessage, const otMessageInfo *aMessageInfo, void *aContext)
{
    NetDiag::Client::Iterator iterator = NetDiag::Client::kIteratorInit;
    NetDiag::Client::DiagTlv  diagTlv;

    OT_UNUSED_VARIABLE(aMessageInfo);
    OT_UNUSED_VARIABLE(aContext);

    SuccessOrQuit(aError);
    VerifyOrQuit(aMessage != nullptr);

    sDiagResult.mReceived = true;

    {
        OffsetRange offsetRange;

        if (Tlv::FindTlvValueOffsetRange(AsCoapMessage(aMessage), NetDiag::Tlv::kSrpClientCounters, offsetRange) ==
            kErrorNone)
        {
            sDiagResult.mSrpTlvLength = offsetRange.GetLength();
        }
    }

    while (NetDiag::Client::GetNextDiagTlv(AsCoapMessage(aMessage), iterator, diagTlv) == kErrorNone)
    {
        sDiagResult.mTlvCount++;

        if (diagTlv.mType == OT_NETWORK_DIAGNOSTIC_TLV_SRP_CLIENT_COUNTERS)
        {
            sDiagResult.mHasSrpCounters = true;
            sDiagResult.mSrpCounters    = diagTlv.mData.mSrpClientCounters;
        }
    }
}

void QueryTlvs(Core &aNexus, Node &aQuerier, const Ip6::Address &aDest, const uint8_t *aTlvTypes, uint8_t aCount)
{
    ClearAllBytes(sDiagResult);

    SuccessOrQuit(
        aQuerier.Get<NetDiag::Client>().SendDiagnosticGet(aDest, aTlvTypes, aCount, HandleDiagGetResponse, nullptr));

    aNexus.AdvanceTime(kDiagResponseTime);

    VerifyOrQuit(sDiagResult.mReceived);
}

void TestNetDiagSrpClientCounters(void)
{
    Core                 nexus;
    Ip6::Address         srpHostAddress;
    Srp::Client::Service srpService;

    Node &server = nexus.CreateNode();
    Node &client = nexus.CreateNode();

    server.SetName("SRP_SERVER");
    client.SetName("SRP_CLIENT");

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelNote));

    Log("Step 0: Form network, bring up SRP server, attach the SRP client node.");

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

    const Ip6::Address &clientRloc = client.Get<Mle::Mle>().GetMeshLocalRloc();

    uint8_t srpCountersTlv[] = {NetDiag::Tlv::kSrpClientCounters};

    Log("Step 1: Client compiled in but never started - TLV MUST still be present, with zeroed counters.");
    {
        QueryTlvs(nexus, server, clientRloc, srpCountersTlv, sizeof(srpCountersTlv));

        VerifyOrQuit(sDiagResult.mHasSrpCounters);
        VerifyOrQuit(sDiagResult.mSrpCounters.mTrackedTime > 0);
        VerifyOrQuit(sDiagResult.mSrpCounters.mRegisteredTime == 0);
        VerifyOrQuit(sDiagResult.mSrpCounters.mAnycastAvailableTime == 0);
        VerifyOrQuit(sDiagResult.mSrpCounters.mUnicastAvailableTime == 0);
        VerifyOrQuit(sDiagResult.mSrpCounters.mTxUpdates == 0);
        VerifyOrQuit(sDiagResult.mSrpCounters.mUpdateAttempts == 0);
        VerifyOrQuit(sDiagResult.mSrpCounters.mSuccess == 0);
        VerifyOrQuit(sDiagResult.mSrpCounters.mServiceAdds == 0);
        VerifyOrQuit(sDiagResult.mSrpCounters.mTxTotalBytes == 0);
    }

    Log("Step 2: Client started but nothing registered yet - counters MUST still read zero.");
    {
        client.Get<Srp::Client>().EnableAutoStartMode(nullptr, nullptr);
        SuccessOrQuit(client.Get<Srp::Client>().SetHostName(kSrpHostName));
        SuccessOrQuit(srpHostAddress.FromString(kSrpHostAddress));
        SuccessOrQuit(client.Get<Srp::Client>().SetHostAddresses(&srpHostAddress, 1));

        VerifyOrQuit(client.Get<Srp::Client>().IsRunning());
        VerifyOrQuit(client.Get<Srp::Client>().GetHostInfo().GetState() != Srp::Client::kRegistered);

        QueryTlvs(nexus, server, clientRloc, srpCountersTlv, sizeof(srpCountersTlv));

        // Starting the client and setting host info must not move any transaction counter.
        // `mRegisteredTime` is deliberately not asserted zero here: `Start()` enters
        // `kStateUpdated`, which is the state `UpdateTimeCounters()` accrues that bucket on,
        // so it begins advancing before anything is actually registered.
        VerifyOrQuit(sDiagResult.mHasSrpCounters);
        VerifyOrQuit(sDiagResult.mSrpCounters.mTxUpdates == 0);
        VerifyOrQuit(sDiagResult.mSrpCounters.mUpdateAttempts == 0);
        VerifyOrQuit(sDiagResult.mSrpCounters.mSuccess == 0);
        VerifyOrQuit(sDiagResult.mSrpCounters.mServiceAdds == 0);
        VerifyOrQuit(sDiagResult.mSrpCounters.mTxTotalBytes == 0);
    }

    Log("Step 3: After registration - every field MUST agree with the local read.");
    {
        otSrpClientCounters before;

        ClearAllBytes(srpService);
        srpService.mName         = kSrpServiceType;
        srpService.mInstanceName = kSrpInstanceName;
        srpService.mPort         = kSrpServicePort;
        SuccessOrQuit(client.Get<Srp::Client>().AddService(srpService));

        nexus.AdvanceTime(kRegistrationTime);

        VerifyOrQuit(client.Get<Srp::Client>().GetHostInfo().GetState() == Srp::Client::kRegistered);

        // The time-based counters keep advancing while the query is in flight,
        // so they are sampled on the client both before and after the exchange
        // and the reported values must fall inside that window. Every other
        // counter is event-driven and must match exactly.

        before = client.Get<Srp::Client>().GetCounters();

        QueryTlvs(nexus, server, clientRloc, srpCountersTlv, sizeof(srpCountersTlv));

        const otSrpClientCounters &local  = client.Get<Srp::Client>().GetCounters();
        const otSrpClientCounters &remote = sDiagResult.mSrpCounters;

        VerifyOrQuit(sDiagResult.mHasSrpCounters);

        VerifyOrQuit((remote.mRegisteredTime >= before.mRegisteredTime) &&
                     (remote.mRegisteredTime <= local.mRegisteredTime));
        VerifyOrQuit((remote.mAnycastAvailableTime >= before.mAnycastAvailableTime) &&
                     (remote.mAnycastAvailableTime <= local.mAnycastAvailableTime));
        VerifyOrQuit((remote.mUnicastAvailableTime >= before.mUnicastAvailableTime) &&
                     (remote.mUnicastAvailableTime <= local.mUnicastAvailableTime));
        VerifyOrQuit((remote.mTrackedTime >= before.mTrackedTime) && (remote.mTrackedTime <= local.mTrackedTime));

        VerifyOrQuit(remote.mTxUpdates == local.mTxUpdates);
        VerifyOrQuit(remote.mUpdateAttempts == local.mUpdateAttempts);
        VerifyOrQuit(remote.mSuccess == local.mSuccess);
        VerifyOrQuit(remote.mRejectedDuplicate == local.mRejectedDuplicate);
        VerifyOrQuit(remote.mRejectedSecurity == local.mRejectedSecurity);
        VerifyOrQuit(remote.mRejectedOther == local.mRejectedOther);
        VerifyOrQuit(remote.mTimeouts == local.mTimeouts);
        VerifyOrQuit(remote.mHostAddressChanges == local.mHostAddressChanges);
        VerifyOrQuit(remote.mServerChanges == local.mServerChanges);
        VerifyOrQuit(remote.mServiceAdds == local.mServiceAdds);
        VerifyOrQuit(remote.mServiceRemoves == local.mServiceRemoves);
        VerifyOrQuit(remote.mServiceClears == local.mServiceClears);
        VerifyOrQuit(remote.mHostAndServicesRemoves == local.mHostAndServicesRemoves);
        VerifyOrQuit(remote.mHostAndServicesClears == local.mHostAndServicesClears);
        VerifyOrQuit(remote.mTxTotalBytes == local.mTxTotalBytes);

        // Sanity: registration actually happened, so the comparisons above are not all-zeros.
        VerifyOrQuit(remote.mSuccess >= 1);
        VerifyOrQuit(remote.mServiceAdds == 1);
        VerifyOrQuit(remote.mTxTotalBytes > 0);
        VerifyOrQuit(remote.mUnicastAvailableTime > 0);
    }

    Log("Step 4: Time counters MUST advance between two queries (server reads live, not cached).");
    {
        uint64_t trackedBefore    = sDiagResult.mSrpCounters.mTrackedTime;
        uint64_t registeredBefore = sDiagResult.mSrpCounters.mRegisteredTime;

        nexus.AdvanceTime(kRegistrationTime);

        QueryTlvs(nexus, server, clientRloc, srpCountersTlv, sizeof(srpCountersTlv));

        VerifyOrQuit(sDiagResult.mHasSrpCounters);
        VerifyOrQuit(sDiagResult.mSrpCounters.mTrackedTime > trackedBefore);
        VerifyOrQuit(sDiagResult.mSrpCounters.mRegisteredTime > registeredBefore);
        VerifyOrQuit(sDiagResult.mSrpCounters.mRegisteredTime <= sDiagResult.mSrpCounters.mTrackedTime);
    }

    Log("Step 5: A mixed request MUST return all requested TLVs, with 45 still decoding correctly.");
    {
        uint8_t mixedTlvs[] = {NetDiag::Tlv::kExtMacAddress, NetDiag::Tlv::kSrpClientCounters,
                               NetDiag::Tlv::kMleCounters};

        QueryTlvs(nexus, server, clientRloc, mixedTlvs, sizeof(mixedTlvs));

        VerifyOrQuit(sDiagResult.mTlvCount == 3);
        VerifyOrQuit(sDiagResult.mHasSrpCounters);
        VerifyOrQuit(sDiagResult.mSrpCounters.mSuccess >= 1);
    }

    Log("Step 6: A multicast DIAG_GET.qry MUST also carry TLV 45 in the answer.");
    {
        Ip6::Address realmLocalAllNodes;

        SuccessOrQuit(realmLocalAllNodes.FromString("ff03::1"));

        QueryTlvs(nexus, server, realmLocalAllNodes, srpCountersTlv, sizeof(srpCountersTlv));

        VerifyOrQuit(sDiagResult.mHasSrpCounters);
    }

    Log("Step 7: The encoded TLV value MUST be exactly 92 bytes.");
    {
        QueryTlvs(nexus, server, clientRloc, srpCountersTlv, sizeof(srpCountersTlv));

        VerifyOrQuit(sDiagResult.mHasSrpCounters);
        VerifyOrQuit(sDiagResult.mSrpTlvLength == kSrpCountersTlvSize);
    }

    Log("Step 8: After stopping the client, the TLV MUST still be returned and MUST retain counter history.");
    {
        // The SRP client renews its lease autonomously (independent of this
        // test's steps), so the last diag response captured in Step 7 is not
        // a reliable "before" baseline: a renewal may have completed between
        // that response being generated and now. Sample the live local
        // counters right before `Stop()` instead.

        otSrpClientCounters before          = client.Get<Srp::Client>().GetCounters();
        uint32_t            successBefore   = before.mSuccess;
        uint32_t            txUpdatesBefore = before.mTxUpdates;

        VerifyOrQuit(successBefore >= 1);
        VerifyOrQuit(txUpdatesBefore >= 1);

        client.Get<Srp::Client>().DisableAutoStartMode();
        client.Get<Srp::Client>().Stop();

        nexus.AdvanceTime(kRegistrationTime);

        VerifyOrQuit(!client.Get<Srp::Client>().IsRunning());

        QueryTlvs(nexus, server, clientRloc, srpCountersTlv, sizeof(srpCountersTlv));

        VerifyOrQuit(sDiagResult.mHasSrpCounters);
        VerifyOrQuit(sDiagResult.mSrpCounters.mSuccess == successBefore);
        VerifyOrQuit(sDiagResult.mSrpCounters.mTxUpdates == txUpdatesBefore);
        VerifyOrQuit(sDiagResult.mSrpCounters.mTrackedTime > 0);
    }

    Log("Step 9: Re-register, then DIAG_RST.ntf for TLV 45 MUST zero SRP counters only.");
    {
        client.Get<Srp::Client>().EnableAutoStartMode(nullptr, nullptr);
        nexus.AdvanceTime(kRegistrationTime);
        VerifyOrQuit(client.Get<Srp::Client>().GetHostInfo().GetState() == Srp::Client::kRegistered);

        uint32_t mleAttachAttemptsBefore = client.Get<Mle::Mle>().GetCounters().mAttachAttempts;
        uint32_t macTxTotalBefore        = client.Get<Mac::Mac>().GetCounters().mTxTotal;

        VerifyOrQuit(mleAttachAttemptsBefore >= 1);

        VerifyOrQuit(client.Get<Srp::Client>().GetCounters().mSuccess >= 1);

        uint8_t resetTlvs[] = {NetDiag::Tlv::kSrpClientCounters};

        SuccessOrQuit(server.Get<NetDiag::Client>().SendDiagnosticReset(clientRloc, resetTlvs, sizeof(resetTlvs)));
        nexus.AdvanceTime(kDiagResponseTime);

        QueryTlvs(nexus, server, clientRloc, srpCountersTlv, sizeof(srpCountersTlv));

        VerifyOrQuit(sDiagResult.mHasSrpCounters);
        VerifyOrQuit(sDiagResult.mSrpCounters.mTxUpdates == 0);
        VerifyOrQuit(sDiagResult.mSrpCounters.mUpdateAttempts == 0);
        VerifyOrQuit(sDiagResult.mSrpCounters.mSuccess == 0);
        VerifyOrQuit(sDiagResult.mSrpCounters.mServiceAdds == 0);
        VerifyOrQuit(sDiagResult.mSrpCounters.mTxTotalBytes == 0);

        // MAC and MLE counters MUST be untouched (guards a misplaced case fallthrough).
        VerifyOrQuit(client.Get<Mle::Mle>().GetCounters().mAttachAttempts == mleAttachAttemptsBefore);
        VerifyOrQuit(client.Get<Mac::Mac>().GetCounters().mTxTotal >= macTxTotalBefore);
    }

    Log("Step 10: DIAG_RST.ntf for TLV 9 MUST NOT zero the SRP counters.");
    {
        nexus.AdvanceTime(kLeaseTime * 1000);

        VerifyOrQuit(client.Get<Srp::Client>().GetCounters().mTxUpdates >= 1);

        uint32_t srpTxUpdatesBefore = client.Get<Srp::Client>().GetCounters().mTxUpdates;

        uint8_t macResetTlvs[] = {NetDiag::Tlv::kMacCounters};

        SuccessOrQuit(
            server.Get<NetDiag::Client>().SendDiagnosticReset(clientRloc, macResetTlvs, sizeof(macResetTlvs)));
        nexus.AdvanceTime(kDiagResponseTime);

        QueryTlvs(nexus, server, clientRloc, srpCountersTlv, sizeof(srpCountersTlv));

        VerifyOrQuit(sDiagResult.mHasSrpCounters);
        VerifyOrQuit(sDiagResult.mSrpCounters.mTxUpdates >= srpTxUpdatesBefore);

        // The case immediately preceding kSrpClientCounters in the reset switch is
        // kMleCounters, so a missing `break` there would wrongly zero the SRP counters.
        uint32_t srpTxUpdatesBeforeMleReset = client.Get<Srp::Client>().GetCounters().mTxUpdates;

        VerifyOrQuit(srpTxUpdatesBeforeMleReset >= 1);

        uint8_t mleResetTlvs[] = {NetDiag::Tlv::kMleCounters};

        SuccessOrQuit(
            server.Get<NetDiag::Client>().SendDiagnosticReset(clientRloc, mleResetTlvs, sizeof(mleResetTlvs)));
        nexus.AdvanceTime(kDiagResponseTime);

        // Prove the MLE reset actually landed, so the SRP assertion below is not vacuous.
        VerifyOrQuit(client.Get<Mle::Mle>().GetCounters().mAttachAttempts == 0);

        QueryTlvs(nexus, server, clientRloc, srpCountersTlv, sizeof(srpCountersTlv));

        VerifyOrQuit(sDiagResult.mHasSrpCounters);
        VerifyOrQuit(sDiagResult.mSrpCounters.mTxUpdates >= srpTxUpdatesBeforeMleReset);
    }
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::TestNetDiagSrpClientCounters();
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
