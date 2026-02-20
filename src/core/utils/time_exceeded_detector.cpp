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
 *   This file implements the TimeExceededDetector module .
 */

#include "time_exceeded_detector.hpp"

#if OPENTHREAD_CONFIG_TIME_EXCEEDED_DETECTION_ENABLE

#include "instance/instance.hpp"

namespace ot {
namespace TimeExceededDetector {

RegisterLogModule("TimeExceed");

TimeExceededDetector::TimeExceededDetector(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mRouterCount(0)
    , mTimer(aInstance)
    , mRouterIteratorIndex(0)
    , mIsQueryingChildIps(false)
    , mWaitingForNextDiscovery(true)
    , mIsTopologyComputed(false)
{
    memset(mTopologyTable, 0, sizeof(mTopologyTable));
    // Scheduling the first discovery of the Thread network topology (scheduled 5 minutes after the startup of the OTBR)
    mTimer.Start(300000);
    LogInfo("TimeExceededDetector will discover the network topology in 5 minutes");
}

void TimeExceededDetector::HandleTimer(void)
{
    if (mWaitingForNextDiscovery)
    {
        mWaitingForNextDiscovery = false;
        // Start of the first phase of the topology discovery process (Thread routers discovery)
        StartDiscoveryPhase1();
    }
    else if (!mIsQueryingChildIps)
    {
        LogInfo("First phase of the topology discovery process completed");
        mIsQueryingChildIps  = true;
        mRouterIteratorIndex = 0;
        // Start of the second phase of the topology discovery process (Thread children devices discovery)
        QueryNextRouterForChildIps();
    }
    else
    {
        // Iterate over all the routers in the mTopology table for retrieving the children devices IPv6 addresses
        mRouterIteratorIndex++;
        QueryNextRouterForChildIps();
    }
}

void TimeExceededDetector::StartDiscoveryPhase1(void)
{
    otError                  error = OT_ERROR_NONE;
    otMeshDiagDiscoverConfig config;

    memset(mTopologyTable, 0, sizeof(mTopologyTable));
    mRouterCount         = 0;
    mIsTopologyComputed  = false;
    mIsQueryingChildIps  = false;
    mRouterIteratorIndex = 0;
    // Configure the mesh diagnostic request to retrieve the list of IPv6 addresses for each router
    config.mDiscoverIp6Addresses = true;
    // Configure the mesh diagnostic request to retrieve the list of all children of the routers
    config.mDiscoverChildTable = true;

    error = otMeshDiagDiscoverTopology(static_cast<otInstance *>(&GetInstance()), &config, HandleMeshDiagDiscoverDone,
                                       this);

    if (error == OT_ERROR_NONE)
    {
        mTimer.Start(3000);
    }
    else
    {
        LogWarn("Error occured during the mesh diagnostic request : %s", otThreadErrorToString(error));
        // retrying a new network topology discovery process in 1 minute
        mWaitingForNextDiscovery = true;
        mTimer.Start(60000);
    }
}

void TimeExceededDetector::HandleMeshDiagDiscoverDone(otError aError, otMeshDiagRouterInfo *aRouterInfo, void *aContext)
{
    static_cast<TimeExceededDetector *>(aContext)->HandleMeshDiagDiscoverDone(aError, aRouterInfo);
}

void TimeExceededDetector::HandleMeshDiagDiscoverDone(otError aError, otMeshDiagRouterInfo *aRouterInfo)
{
    if (aError != OT_ERROR_NONE && aError != OT_ERROR_PENDING)
        return;

    if (aRouterInfo != nullptr)
    {
        uint8_t                  routerId        = aRouterInfo->mRouterId;
        otInstance              *instance        = static_cast<otInstance *>(&GetInstance());
        const otMeshLocalPrefix *meshLocalPrefix = otThreadGetMeshLocalPrefix(instance);

        if (routerId <= OT_NETWORK_MAX_ROUTER_ID)
        {
            if (!mTopologyTable[routerId].mValid)
            {
                mRouterCount++;
            }

            mTopologyTable[routerId].mValid  = true;
            mTopologyTable[routerId].mRloc16 = aRouterInfo->mRloc16;
            memcpy(mTopologyTable[routerId].mLinkQualities, aRouterInfo->mLinkQualities,
                   sizeof(mTopologyTable[routerId].mLinkQualities));
            mTopologyTable[routerId].mIp6AddressCount = 0;

            if (aRouterInfo->mIp6AddrIterator != nullptr)
            {
                otIp6Address ip6Address;
                // Only the OMR IPv6 addresses are store in the mTopologyTable for each router
                while (otMeshDiagGetNextIp6Address(aRouterInfo->mIp6AddrIterator, &ip6Address) == OT_ERROR_NONE)
                {
                    ot::Ip6::Address &coreAddr = AsCoreType(&ip6Address);
                    if (coreAddr.IsLinkLocalUnicast() || coreAddr.IsMulticast())
                        continue;

                    if (memcmp(ip6Address.mFields.m8, meshLocalPrefix->m8, 8) == 0)
                        continue;

                    if (mTopologyTable[routerId].mIp6AddressCount < kMaxIp6Addresses)
                    {
                        mTopologyTable[routerId].mIp6Addresses[mTopologyTable[routerId].mIp6AddressCount++] =
                            ip6Address;
                    }
                }
            }

            mTopologyTable[routerId].mChildCount = 0;
            if (aRouterInfo->mChildIterator != nullptr)
            {
                otMeshDiagChildInfo childInfo;
                while (otMeshDiagGetNextChildInfo(aRouterInfo->mChildIterator, &childInfo) == OT_ERROR_NONE)
                {
                    if (mTopologyTable[routerId].mChildCount < kMaxChildren)
                    {
                        mTopologyTable[routerId].mChildren[mTopologyTable[routerId].mChildCount].mRloc16 =
                            childInfo.mRloc16;
                        mTopologyTable[routerId].mChildren[mTopologyTable[routerId].mChildCount].mHasIp = false;
                        mTopologyTable[routerId].mChildCount++;
                    }
                }
            }
        }
    }
}

void TimeExceededDetector::QueryNextRouterForChildIps(void)
{
    while (mRouterIteratorIndex <= OT_NETWORK_MAX_ROUTER_ID)
    {
        if (mTopologyTable[mRouterIteratorIndex].mValid && mTopologyTable[mRouterIteratorIndex].mChildCount > 0)
        {
            uint16_t parentRloc16 = mTopologyTable[mRouterIteratorIndex].mRloc16;

            otError error = otMeshDiagQueryChildrenIp6Addrs(static_cast<otInstance *>(&GetInstance()), parentRloc16,
                                                            HandleChildIpQueryDone, this);
            if (error == OT_ERROR_NONE)
            {
                mTimer.Start(1000);
                return;
            }
        }
        mRouterIteratorIndex++;
    }

    LogInfo("Second phase of the topology discovery process completed");
    LogInfo("The network topology have been succesfully computed");
    mIsQueryingChildIps = false;
    mIsTopologyComputed = true;
    LogTopologyTable();

    // Scheduling the next discovery of the Thread network topology in 30 minutes
    mWaitingForNextDiscovery = true;
    mTimer.Start(1800000);
}

void TimeExceededDetector::HandleChildIpQueryDone(otError                    aError,
                                                  uint16_t                   aRloc16,
                                                  otMeshDiagIp6AddrIterator *aIterator,
                                                  void                      *aContext)
{
    static_cast<TimeExceededDetector *>(aContext)->HandleChildIpQueryDone(aError, aRloc16, aIterator);
}

void TimeExceededDetector::HandleChildIpQueryDone(otError                    aError,
                                                  uint16_t                   aRloc16,
                                                  otMeshDiagIp6AddrIterator *aIterator)
{
    if (aError != OT_ERROR_NONE && aError != OT_ERROR_PENDING)
    {
        LogInfo("Callback error occured for child with RLOC16 = 0x%04x : %s", aRloc16, otThreadErrorToString(aError));
        return;
    }

    if (aIterator == nullptr)
        return;

    uint16_t parentRloc = aRloc16 & 0xFC00;
    int      routerId   = -1;
    for (int i = 0; i <= OT_NETWORK_MAX_ROUTER_ID; i++)
    {
        if (mTopologyTable[i].mValid && mTopologyTable[i].mRloc16 == parentRloc)
        {
            routerId = i;
            break;
        }
    }

    if (routerId != -1)
    {
        otIp6Address ip6Address;
        char         ipString[Ip6::Address::kInfoStringSize];

        // Only the OMR IPv6 addresses are store in the mTopologyTable for each child device
        while (otMeshDiagGetNextIp6Address(aIterator, &ip6Address) == OT_ERROR_NONE)
        {
            ot::Ip6::Address &coreAddr = AsCoreType(&ip6Address);
            coreAddr.ToString(ipString, sizeof(ipString));

            if (coreAddr.IsLinkLocalUnicast() || coreAddr.IsMulticast())
                continue;

            for (int c = 0; c < mTopologyTable[routerId].mChildCount; c++)
            {
                if (mTopologyTable[routerId].mChildren[c].mRloc16 == aRloc16)
                {
                    mTopologyTable[routerId].mChildren[c].mIp6Address = ip6Address;
                    mTopologyTable[routerId].mChildren[c].mHasIp      = true;
                    break;
                }
            }
        }
    }
    else
    {
        LogWarn("Parent for child RLOC 16 = 0x%04x not found in topology table", aRloc16);
    }
}

void TimeExceededDetector::LogTopologyTable(void)
{
    LogInfo("< =============================================================== >");
    LogInfo("Network topology of the thread network :");
    char ipString[Ip6::Address::kInfoStringSize];
    for (uint8_t i = 0; i <= OT_NETWORK_MAX_ROUTER_ID; i++)
    {
        if (mTopologyTable[i].mValid)
        {
            LogInfo("Router ID = %d (0x%04x)", i, mTopologyTable[i].mRloc16);

            for (uint8_t k = 0; k < mTopologyTable[i].mIp6AddressCount; k++)
            {
                AsCoreType(&mTopologyTable[i].mIp6Addresses[k]).ToString(ipString, sizeof(ipString));
                LogInfo("Router IP address = %s", ipString);
            }
            if (mTopologyTable[i].mChildCount > 0)
            {
                LogInfo("Child devices of the router :");
                for (uint8_t c = 0; c < mTopologyTable[i].mChildCount; c++)
                {
                    if (mTopologyTable[i].mChildren[c].mHasIp)
                    {
                        AsCoreType(&mTopologyTable[i].mChildren[c].mIp6Address).ToString(ipString, sizeof(ipString));
                        LogInfo("Child RLOC16 = 0x%04x & IP address = %s", mTopologyTable[i].mChildren[c].mRloc16,
                                ipString);
                    }
                    else
                    {
                        LogInfo("Child RLOC16 = 0x%04x & IP = not found", mTopologyTable[i].mChildren[c].mRloc16);
                    }
                }
            }
            else
            {
                LogInfo("No child device :");
            }
        }
    }
    LogInfo("< =============================================================== >");
}

uint8_t TimeExceededDetector::GetLinkCost(uint8_t aLinkQuality)
{
    switch (aLinkQuality)
    {
    case 3:
        return 1;
    case 2:
        return 2;
    case 1:
        return 4;
    case 0:
        return 0xFF;
    default:
        return 0xFF;
    }
}

bool TimeExceededDetector::IsHopLimitInsufficient(const Ip6::Header &aHeader, Ip6::Address &aDyingAtAddress)
{
    // If the network topology is not computed then the time exceeded detector cannot detect hop limit expiration
    if (!mIsTopologyComputed)
        return false;
    // If the hop limit of the packet is greater than the number of routers in the network + 1(if child device) then no
    // need to check for possible hop limit expiration (because not likely to happen)
    if (aHeader.GetHopLimit() > mRouterCount + 1)
        return false;

    LogTopologyTable();

    const Ip6::Address &destIp          = aHeader.GetDestination();
    uint8_t             currentHopLimit = aHeader.GetHopLimit();

    DestinationInfo destInfo = FindDestination(destIp);
    if (!destInfo.mFound)
        return false;

    uint16_t myRloc16   = otThreadGetRloc16(static_cast<otInstance *>(&GetInstance()));
    uint8_t  myRouterId = static_cast<uint8_t>(myRloc16 >> 10);
    uint8_t  path[OT_NETWORK_MAX_ROUTER_ID + 1];
    // Computation of the least cost path between the OTBR and the destination router (or parent router if
    // destination is child device)
    uint8_t pathLength = ComputeLeastCostPath(myRouterId, destInfo.mRouterId, path, OT_NETWORK_MAX_ROUTER_ID + 1);
    if (pathLength == 0xFF)
        return false;

    uint8_t totalHopsNeeded = pathLength + (destInfo.mIsChild ? 1 : 0);

    // If the packet's hop limit is not enough to reach the destination then find the router address where the
    // hop limit get equal to 0
    if (currentHopLimit < totalHopsNeeded)
    {
        if (currentHopLimit < pathLength)
        {
            uint8_t dyingRouterId = path[currentHopLimit];
            if (mTopologyTable[dyingRouterId].mValid && mTopologyTable[dyingRouterId].mIp6AddressCount > 0)
            {
                aDyingAtAddress = AsCoreType(&mTopologyTable[dyingRouterId].mIp6Addresses[0]);
            }
        }
        else
        {
            uint8_t lastRouterId = path[pathLength];
            if (mTopologyTable[lastRouterId].mValid && mTopologyTable[lastRouterId].mIp6AddressCount > 0)
            {
                aDyingAtAddress = AsCoreType(&mTopologyTable[lastRouterId].mIp6Addresses[0]);
            }
        }

        char dyingIpStr[Ip6::Address::kInfoStringSize];
        aDyingAtAddress.ToString(dyingIpStr, sizeof(dyingIpStr));
        LogInfo("Time Exceeded Detected : Packet dies at Router ID = %d (IP address = %s)",
                (currentHopLimit < pathLength ? path[currentHopLimit] : path[pathLength]), dyingIpStr);

        return true;
    }
    return false;
}

TimeExceededDetector::DestinationInfo TimeExceededDetector::FindDestination(const Ip6::Address &aDestIp)
{
    DestinationInfo info = {false, 0, false};

    for (uint8_t i = 0; i <= OT_NETWORK_MAX_ROUTER_ID; i++)
    {
        if (!mTopologyTable[i].mValid)
            continue;

        for (uint8_t k = 0; k < mTopologyTable[i].mIp6AddressCount; k++)
        {
            if (AsCoreType(&mTopologyTable[i].mIp6Addresses[k]) == aDestIp)
            {
                info.mFound    = true;
                info.mRouterId = i;
                info.mIsChild  = false;
                return info;
            }
        }
        for (uint8_t c = 0; c < mTopologyTable[i].mChildCount; c++)
        {
            if (mTopologyTable[i].mChildren[c].mHasIp &&
                AsCoreType(&mTopologyTable[i].mChildren[c].mIp6Address) == aDestIp)
            {
                info.mFound    = true;
                info.mRouterId = i;
                info.mIsChild  = true;
                return info;
            }
        }
    }
    return info;
}

uint8_t TimeExceededDetector::ComputeLeastCostPath(uint8_t  aStartRouterId,
                                                   uint8_t  aEndRouterId,
                                                   uint8_t *aPathBuffer,
                                                   uint8_t  aMaxLen)
{
    if (aStartRouterId == aEndRouterId)
    {
        aPathBuffer[0] = aStartRouterId;
        return 0;
    }

    uint8_t cost[OT_NETWORK_MAX_ROUTER_ID + 1];
    uint8_t pred[OT_NETWORK_MAX_ROUTER_ID + 1];
    bool    visited[OT_NETWORK_MAX_ROUTER_ID + 1];
    // Set the initial cost (distance) to every other router = infinite
    memset(cost, 0xFF, sizeof(cost));
    memset(visited, 0, sizeof(visited));
    cost[aStartRouterId] = 0;
    pred[aStartRouterId] = aStartRouterId;

    // Finding the least cost path between start router (id = aStartRouterId) and end router (id = aEndRouterId)
    for (int count = 0; count <= OT_NETWORK_MAX_ROUTER_ID; count++)
    {
        uint8_t u       = 0xFF;
        uint8_t minCost = 0xFF;

        for (int i = 0; i <= OT_NETWORK_MAX_ROUTER_ID; i++)
        {
            if (!visited[i] && mTopologyTable[i].mValid && cost[i] < minCost)
            {
                minCost = cost[i];
                u       = i;
            }
        }

        if (u == 0xFF || minCost == 0xFF)
            break;
        if (u == aEndRouterId)
            break;

        visited[u] = true;

        for (int v = 0; v <= OT_NETWORK_MAX_ROUTER_ID; v++)
        {
            if (!visited[v] && mTopologyTable[v].mValid)
            {
                uint8_t quality  = mTopologyTable[v].mLinkQualities[u];
                uint8_t linkCost = GetLinkCost(quality);
                if (linkCost != 0xFF)
                {
                    if (cost[u] + linkCost < cost[v])
                    {
                        cost[v] = cost[u] + linkCost;
                        pred[v] = u;
                    }
                }
            }
        }
    }

    if (cost[aEndRouterId] == 0xFF)
        return 0xFF;

    // Obtaining the path and number of hop count of the least cost path computed
    uint8_t hopCount = 0;
    uint8_t curr     = aEndRouterId;
    uint8_t tempPath[OT_NETWORK_MAX_ROUTER_ID + 1];

    while (curr != aStartRouterId)
    {
        tempPath[hopCount++] = curr;
        curr                 = pred[curr];
        if (hopCount >= OT_NETWORK_MAX_ROUTER_ID)
            return 0xFF;
    }
    tempPath[hopCount] = aStartRouterId;

    if (hopCount > aMaxLen)
        return 0xFF;

    // Storing the computed least cost path in the aPathBuffer (sequence of router ID of the path)
    for (int i = 0; i <= hopCount; i++)
    {
        aPathBuffer[i] = tempPath[hopCount - i];
    }
    return hopCount;
}

} // namespace TimeExceededDetector
} // namespace ot

#endif // OPENTHREAD_CONFIG_TIME_EXCEEDED_DETECTION_ENABLE