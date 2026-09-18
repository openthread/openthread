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
 *   This file implements Backbone Router multicast routing for platforms
 *   without kernel IPv6 multicast routing (MRT6): the packets are forwarded
 *   in userspace, between a BPF tap on the Thread interface and one on the
 *   infrastructure interface.
 */

#include "posix/platform/multicast_routing.hpp"

#if OPENTHREAD_POSIX_CONFIG_BACKBONE_ROUTER_MULTICAST_ROUTING_ENABLE && !defined(__linux__)

#include <errno.h>
#include <ifaddrs.h>
#include <net/bpf.h>
#include <net/if.h>
#include <net/if_dl.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include <openthread/backbone_router_ftd.h>
#include <openthread/platform/time.h>

#include "utils.hpp"
#include "common/code_utils.hpp"

namespace ot {
namespace Posix {

using namespace MulticastForwarding;

// Thread group traffic is a few packets per second at most; these limits leave headroom while keeping a storm on
// either side well below what a mesh can absorb. Into the mesh is tighter: every packet sent there is flooded (MPL)
// to every router.
const RateLimiter::Config MulticastRoutingManager::kThreadToBackboneLimits = {
    /* mPerGroupRatePerSecond */ 20,
    /* mPerGroupBurst */ 40,
    /* mTotalRatePerSecond */ 100,
    /* mTotalBurst */ 200,
};

const RateLimiter::Config MulticastRoutingManager::kBackboneToThreadLimits = {
    /* mPerGroupRatePerSecond */ 10,
    /* mPerGroupBurst */ 20,
    /* mTotalRatePerSecond */ 30,
    /* mTotalBurst */ 60,
};

namespace {

// Accept IPv6 packets to a multicast destination of admin-local scope or larger, the only ones a Backbone Router
// carries: one program for tunnel interfaces (DLT_NULL, 4-byte address family header), one for Ethernet.
const struct bpf_insn kFilterNull[] = {
    BPF_STMT(BPF_LD | BPF_B | BPF_ABS, 4),           // IPv6 version nibble
    BPF_STMT(BPF_ALU | BPF_AND | BPF_K, 0xf0),       //
    BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, 0x60, 0, 6), // not IPv6 -> drop
    BPF_STMT(BPF_LD | BPF_B | BPF_ABS, 4 + 24),      // destination[0]
    BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, 0xff, 0, 4), // not multicast -> drop
    BPF_STMT(BPF_LD | BPF_B | BPF_ABS, 4 + 25),      // destination[1]: flags | scope
    BPF_STMT(BPF_ALU | BPF_AND | BPF_K, 0x0f),       //
    BPF_JUMP(BPF_JMP | BPF_JGE | BPF_K, 0x04, 0, 1), // scope < admin-local -> drop
    BPF_STMT(BPF_RET | BPF_K, 0xffffffff),           // accept
    BPF_STMT(BPF_RET | BPF_K, 0),                    // drop
};

const struct bpf_insn kFilterEthernet[] = {
    BPF_STMT(BPF_LD | BPF_H | BPF_ABS, 12),            // EtherType
    BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, 0x86dd, 0, 6), // not IPv6 -> drop
    BPF_STMT(BPF_LD | BPF_B | BPF_ABS, 14 + 24),       // destination[0]
    BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, 0xff, 0, 4),   // not multicast -> drop
    BPF_STMT(BPF_LD | BPF_B | BPF_ABS, 14 + 25),       // destination[1]: flags | scope
    BPF_STMT(BPF_ALU | BPF_AND | BPF_K, 0x0f),         //
    BPF_JUMP(BPF_JMP | BPF_JGE | BPF_K, 0x04, 0, 1),   // scope < admin-local -> drop
    BPF_STMT(BPF_RET | BPF_K, 0xffffffff),             // accept
    BPF_STMT(BPF_RET | BPF_K, 0),                      // drop
};

} // namespace

MulticastRoutingManager::MulticastRoutingManager()
    : mThreadToBackboneLimiter(kThreadToBackboneLimits)
    , mBackboneToThreadLimiter(kBackboneToThreadLimits)
    , mNextReportTime(0)
    , mCountersChanged(false)
    , mMulticastRouterSock(-1)
    , mState(kStateDisabled)
    , mRetryIntervalMs(kMinRetryIntervalMs)
    , mNextRetryTime(0)
{
    memset(mBackboneMac, 0, sizeof(mBackboneMac));
    memset(&mThreadToBackbone, 0, sizeof(mThreadToBackbone));
    memset(&mBackboneToThread, 0, sizeof(mBackboneToThread));
}

void MulticastRoutingManager::Add(const Ip6::Address &aAddress)
{
    VerifyOrExit(IsEnabled());

    // Joining the group on the infrastructure link makes the link (the interface's own filter, MLD snooping switches)
    // deliver it; whether a packet then enters the mesh is decided per packet from the listener table.
    UpdateMldReport(aAddress, true);

exit:
    return;
}

void MulticastRoutingManager::Remove(const Ip6::Address &aAddress)
{
    VerifyOrExit(IsEnabled());

    UpdateMldReport(aAddress, false);

exit:
    return;
}

void MulticastRoutingManager::Update(Mainloop::Context &aContext)
{
    uint64_t now = otPlatTimeGet();

    if (mState == kStateEnabling)
    {
        Mainloop::SetTimeoutIfEarlier((mNextRetryTime > now) ? (mNextRetryTime - now) : 0, aContext);
        ExitNow();
    }

    VerifyOrExit(IsEnabled());

    Mainloop::AddToReadFdSet(mThreadTap.GetFd(), aContext);
    Mainloop::AddToReadFdSet(mBackboneTap.GetFd(), aContext);
    Mainloop::SetTimeoutIfEarlier((mNextReportTime > now) ? (mNextReportTime - now) : 0, aContext);

exit:
    return;
}

void MulticastRoutingManager::Process(const Mainloop::Context &aContext)
{
    otError error = OT_ERROR_NONE;

    if (mState == kStateEnabling)
    {
        VerifyOrExit(otPlatTimeGet() >= mNextRetryTime);

        if (InitMulticastRouterSock() == OT_ERROR_NONE)
        {
            mState           = kStateEnabled;
            mRetryIntervalMs = kMinRetryIntervalMs;
            mNextRetryTime   = 0;
            LogInfo("Retried InitMulticastRouterSock successfully");
        }
        else
        {
            mRetryIntervalMs = OT_MIN(mRetryIntervalMs * 2, kMaxRetryIntervalMs);
            mNextRetryTime   = otPlatTimeGet() + static_cast<uint64_t>(mRetryIntervalMs) * OT_US_PER_MS;
            LogWarn("Failed to retry InitMulticastRouterSock: %s, will retry again in %u ms", strerror(errno),
                    mRetryIntervalMs);
        }
        ExitNow();
    }

    VerifyOrExit(IsEnabled());

    if (Mainloop::IsFdReadable(mThreadTap.GetFd(), aContext))
    {
        error = mThreadTap.Read(HandleThreadFrame, this);
    }

    if (error == OT_ERROR_NONE && Mainloop::IsFdReadable(mBackboneTap.GetFd(), aContext))
    {
        error = mBackboneTap.Read(HandleBackboneFrame, this);
    }

    if (error != OT_ERROR_NONE)
    {
        // An interface went away under its tap: start over once both are back.
        LogWarn("Failed to read from a packet tap: %s, will retry in mainloop", strerror(errno));
        FinalizeMulticastRouterSock();
        mState         = kStateEnabling;
        mNextRetryTime = otPlatTimeGet() + static_cast<uint64_t>(mRetryIntervalMs) * OT_US_PER_MS;
        ExitNow();
    }

    if (otPlatTimeGet() >= mNextReportTime)
    {
        ReportCounters();
    }

exit:
    return;
}

otError MulticastRoutingManager::InitMulticastRouterSock(void)
{
    otError error = OT_ERROR_NONE;

    mDedupCache.Clear();
    mThreadToBackboneLimiter.Clear();
    mBackboneToThreadLimiter.Clear();

    // Only carries the group memberships on the infrastructure interface.
    mMulticastRouterSock = SocketWithCloseExec(AF_INET6, SOCK_DGRAM, IPPROTO_UDP, kSocketNonBlock);
    VerifyOrExit(mMulticastRouterSock != -1, error = OT_ERROR_FAILED);

    SuccessOrExit(error = ReadBackboneMac());
    SuccessOrExit(error = OpenBackboneTap());
    SuccessOrExit(error = OpenThreadTap());

    {
        otBackboneRouterMulticastListenerIterator iter = OT_BACKBONE_ROUTER_MULTICAST_LISTENER_ITERATOR_INIT;
        otBackboneRouterMulticastListenerInfo     listenerInfo;

        while (otBackboneRouterMulticastListenerGetNext(gInstance, &iter, &listenerInfo) == OT_ERROR_NONE)
        {
            UpdateMldReport(static_cast<const Ip6::Address &>(listenerInfo.mAddress), true);
        }
    }

    mNextReportTime = otPlatTimeGet() + static_cast<uint64_t>(kReportIntervalSec) * OT_US_PER_S;

exit:
    if (error != OT_ERROR_NONE)
    {
        int savedErrno = errno;

        FinalizeMulticastRouterSock();
        errno = savedErrno;
    }

    return error;
}

void MulticastRoutingManager::FinalizeMulticastRouterSock(void)
{
    if (mThreadTap.IsOpen() || mBackboneTap.IsOpen())
    {
        ReportCounters();
    }

    mThreadTap.Close();
    mBackboneTap.Close();

    // Closing the socket leaves its groups.
    if (mMulticastRouterSock != -1)
    {
        close(mMulticastRouterSock);
        mMulticastRouterSock = -1;
    }
}

otError MulticastRoutingManager::ReadBackboneMac(void)
{
    otError         error     = OT_ERROR_NOT_FOUND;
    struct ifaddrs *addresses = nullptr;
    const char     *ifName    = otSysGetInfraNetifName();

    VerifyOrExit(ifName != nullptr && ifName[0] != '\0', errno = ENODEV);
    VerifyOrExit(getifaddrs(&addresses) == 0, error = OT_ERROR_FAILED);

    for (struct ifaddrs *ifa = addresses; ifa != nullptr; ifa = ifa->ifa_next)
    {
        const struct sockaddr_dl *link;

        if (ifa->ifa_addr == nullptr || ifa->ifa_addr->sa_family != AF_LINK || strcmp(ifa->ifa_name, ifName) != 0)
        {
            continue;
        }

        link = reinterpret_cast<const struct sockaddr_dl *>(ifa->ifa_addr);

        if (link->sdl_alen == kMacSize)
        {
            memcpy(mBackboneMac, LLADDR(link), kMacSize);
            error = OT_ERROR_NONE;
            break;
        }
    }

    freeifaddrs(addresses);

    if (error != OT_ERROR_NONE)
    {
        errno = ENODEV;
    }

exit:
    return error;
}

otError MulticastRoutingManager::OpenBackboneTap(void)
{
    otError error;

    // Frames this host sends are included, so that applications on the Backbone Router itself reach the mesh. That
    // also shows the forwarder its own emissions; they are in the duplicate cache from when they were forwarded.
    SuccessOrExit(error = mBackboneTap.Open(otSysGetInfraNetifName(), /* aSeeSent */ true));
    VerifyOrExit(mBackboneTap.GetDataLinkType() == DLT_EN10MB, (errno = ENOTSUP, error = OT_ERROR_NOT_CAPABLE));
    error = mBackboneTap.SetFilter(kFilterEthernet, OT_ARRAY_LENGTH(kFilterEthernet));

exit:
    return error;
}

otError MulticastRoutingManager::OpenThreadTap(void)
{
    otError error;

    // Received packets only: what the Thread stack hands to the host, which is everything it received from the mesh,
    // never what the host sent it. Writing to the same tap sends a packet into the mesh.
    SuccessOrExit(error = mThreadTap.Open(gNetifName, /* aSeeSent */ false));
    VerifyOrExit(mThreadTap.GetDataLinkType() == DLT_NULL, (errno = ENOTSUP, error = OT_ERROR_NOT_CAPABLE));
    error = mThreadTap.SetFilter(kFilterNull, OT_ARRAY_LENGTH(kFilterNull));

exit:
    return error;
}

void MulticastRoutingManager::HandleThreadFrame(void *aContext, const uint8_t *aFrame, uint16_t aLength)
{
    if (aLength > kTunnelHeaderSize)
    {
        static_cast<MulticastRoutingManager *>(aContext)->HandleThreadPacket(aFrame + kTunnelHeaderSize,
                                                                             aLength - kTunnelHeaderSize);
    }
}

void MulticastRoutingManager::HandleBackboneFrame(void *aContext, const uint8_t *aFrame, uint16_t aLength)
{
    if (aLength > kEthernetHeaderSize)
    {
        static_cast<MulticastRoutingManager *>(aContext)->HandleBackbonePacket(aFrame + kEthernetHeaderSize,
                                                                               aLength - kEthernetHeaderSize);
    }
}

void MulticastRoutingManager::HandleThreadPacket(const uint8_t *aPacket, uint16_t aLength)
{
    uint64_t     now = otPlatTimeGet();
    Verdict      verdict;
    Ip6::Address group;
    uint8_t      frame[kMaxFrameSize];
    uint16_t     frameLength;

    mThreadToBackbone.mReceived++;
    mCountersChanged = true;

    verdict = Check(aPacket, aLength);
    VerifyOrExit(verdict == kForward, mThreadToBackbone.mRejected++);

    VerifyOrExit(!mDedupCache.Check(DedupCache::Fingerprint(aPacket, aLength), now), mThreadToBackbone.mDuplicates++);

    group = GetDestination(aPacket);
    VerifyOrExit(mThreadToBackboneLimiter.Allow(group, now), mThreadToBackbone.mRateLimited++);

    frameLength = BuildEthernetFrame(aPacket, aLength, mBackboneMac, frame, sizeof(frame));
    VerifyOrExit(frameLength > 0, mThreadToBackbone.mRejected++);
    DecrementHopLimit(frame + kEthernetHeaderSize);

    VerifyOrExit(mBackboneTap.Write(frame, frameLength) == OT_ERROR_NONE, mThreadToBackbone.mErrors++);

    mThreadToBackbone.mForwarded++;
    LogDebg("Thread -> Backbone: %s -> %s, %u bytes", GetSource(aPacket).ToString().AsCString(),
            group.ToString().AsCString(), aLength);

exit:
    return;
}

void MulticastRoutingManager::HandleBackbonePacket(const uint8_t *aPacket, uint16_t aLength)
{
    uint64_t     now = otPlatTimeGet();
    Verdict      verdict;
    Ip6::Address group;
    uint8_t      frame[kMaxFrameSize];
    uint32_t     family = AF_INET6;

    mBackboneToThread.mReceived++;
    mCountersChanged = true;

    verdict = Check(aPacket, aLength);
    VerifyOrExit(verdict == kForward, mBackboneToThread.mRejected++);

    // Into the mesh only for groups a Thread device registered for, as the inbound forwarding cache does where the
    // kernel forwards: a packet sent into the mesh is flooded to every router.
    group = GetDestination(aPacket);
    VerifyOrExit(HasMulticastListener(group), mBackboneToThread.mNoListener++);

    VerifyOrExit(!mDedupCache.Check(DedupCache::Fingerprint(aPacket, aLength), now), mBackboneToThread.mDuplicates++);
    VerifyOrExit(mBackboneToThreadLimiter.Allow(group, now), mBackboneToThread.mRateLimited++);

    VerifyOrExit(static_cast<uint32_t>(kTunnelHeaderSize) + aLength <= sizeof(frame), mBackboneToThread.mRejected++);

    // A BPF write to a tunnel interface skips the framer and lands in the driver's output path, which converts the
    // address family header to the byte order the tunnel's client expects: it goes in host byte order here.
    memcpy(frame, &family, kTunnelHeaderSize);
    memcpy(frame + kTunnelHeaderSize, aPacket, aLength);
    DecrementHopLimit(frame + kTunnelHeaderSize);

    VerifyOrExit(mThreadTap.Write(frame, kTunnelHeaderSize + aLength) == OT_ERROR_NONE, mBackboneToThread.mErrors++);

    mBackboneToThread.mForwarded++;
    LogDebg("Backbone -> Thread: %s -> %s, %u bytes", GetSource(aPacket).ToString().AsCString(),
            group.ToString().AsCString(), aLength);

exit:
    return;
}

void MulticastRoutingManager::ReportCounters(void)
{
    // Only when a packet was seen since the last report, so that an idle forwarder stays quiet.
    if (mCountersChanged)
    {
        ReportCounters("Thread -> Backbone", mThreadToBackbone);
        ReportCounters("Backbone -> Thread", mBackboneToThread);
        mCountersChanged = false;
    }

    mNextReportTime = otPlatTimeGet() + static_cast<uint64_t>(kReportIntervalSec) * OT_US_PER_S;
}

void MulticastRoutingManager::ReportCounters(const char *aDirection, const Counters &aCounters)
{
    LogNote("%s: received %lu, forwarded %lu, rejected %lu, no listener %lu, duplicates %lu, rate limited %lu, "
            "errors %lu",
            aDirection, ToUlong(aCounters.mReceived), ToUlong(aCounters.mForwarded), ToUlong(aCounters.mRejected),
            ToUlong(aCounters.mNoListener), ToUlong(aCounters.mDuplicates), ToUlong(aCounters.mRateLimited),
            ToUlong(aCounters.mErrors));
}

} // namespace Posix
} // namespace ot

#endif // OPENTHREAD_POSIX_CONFIG_BACKBONE_ROUTER_MULTICAST_ROUTING_ENABLE && !defined(__linux__)
