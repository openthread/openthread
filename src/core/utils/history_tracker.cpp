/*
 *  Copyright (c) 2021, The OpenThread Authors.
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
 *   This file implements the History Tracker module.
 */

#include "history_tracker.hpp"

#if OPENTHREAD_CONFIG_HISTORY_TRACKER_ENABLE

#include "instance/instance.hpp"

namespace ot {
namespace HistoryTracker {

//---------------------------------------------------------------------------------------------------------------------
// Local

Local::Local(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mTimer(aInstance)
#if OPENTHREAD_CONFIG_HISTORY_TRACKER_NET_DATA
    , mPreviousNetworkData(aInstance, mNetworkDataTlvBuffer, 0, sizeof(mNetworkDataTlvBuffer))
#endif
{
    mTimer.Start(kAgeCheckPeriod);

#if OPENTHREAD_FTD && (OPENTHREAD_CONFIG_HISTORY_TRACKER_ROUTER_LIST_SIZE > 0)
    ClearAllBytes(mRouterEntries);
#endif
}

void Local::RecordNetworkInfo(void)
{
    NetworkInfo    *entry = mNetInfoHistory.AddNewEntry();
    Mle::DeviceMode mode;

    VerifyOrExit(entry != nullptr);

    entry->mRole        = MapEnum(Get<Mle::Mle>().GetRole());
    entry->mRloc16      = Get<Mle::Mle>().GetRloc16();
    entry->mPartitionId = Get<Mle::Mle>().GetLeaderData().GetPartitionId();
    mode                = Get<Mle::Mle>().GetDeviceMode();
    mode.Get(entry->mMode);

exit:
    return;
}

void Local::RecordMessage(const Message &aMessage, const Mac::Address &aMacAddress, MessageType aType)
{
    MessageInfo *entry = nullptr;
    Ip6::Headers headers;

    VerifyOrExit(aMessage.GetType() == Message::kTypeIp6);

    SuccessOrExit(headers.ParseFrom(aMessage));

#if OPENTHREAD_CONFIG_HISTORY_TRACKER_EXCLUDE_THREAD_CONTROL_MESSAGES
    if (headers.IsUdp())
    {
        uint16_t port = 0;

        switch (aType)
        {
        case kRxMessage:
            port = headers.GetDestinationPort();
            break;

        case kTxMessage:
            port = headers.GetSourcePort();
            break;
        }

        VerifyOrExit((port != Mle::kUdpPort) && (port != Tmf::kUdpPort));
    }
#endif

    switch (aType)
    {
    case kRxMessage:
        entry = mRxHistory.AddNewEntry();
        break;

    case kTxMessage:
        entry = mTxHistory.AddNewEntry();
        break;
    }

    VerifyOrExit(entry != nullptr);

    entry->mPayloadLength        = headers.GetIp6Header().GetPayloadLength();
    entry->mNeighborRloc16       = aMacAddress.IsShort() ? aMacAddress.GetShort() : kInvalidRloc16;
    entry->mSource.mAddress      = headers.GetSourceAddress();
    entry->mSource.mPort         = headers.GetSourcePort();
    entry->mDestination.mAddress = headers.GetDestinationAddress();
    entry->mDestination.mPort    = headers.GetDestinationPort();
    entry->mChecksum             = headers.GetChecksum();
    entry->mIpProto              = headers.GetIpProto();
    entry->mIcmp6Type            = headers.IsIcmp6() ? headers.GetIcmpHeader().GetType() : 0;
    entry->mAveRxRss             = (aType == kRxMessage) ? aMessage.GetRssAverager().GetAverage() : Radio::kInvalidRssi;
    entry->mLinkSecurity         = aMessage.IsLinkSecurityEnabled();
    entry->mTxSuccess            = (aType == kTxMessage) ? aMessage.GetTxSuccess() : true;
    entry->mPriority             = aMessage.GetPriority();

    if (aMacAddress.IsExtended())
    {
        Neighbor *neighbor = Get<NeighborTable>().FindNeighbor(aMacAddress, Neighbor::kInStateAnyExceptInvalid);

        if (neighbor != nullptr)
        {
            entry->mNeighborRloc16 = neighbor->GetRloc16();
        }
    }

#if OPENTHREAD_CONFIG_MULTI_RADIO
    if (aMessage.IsRadioTypeSet())
    {
        switch (aMessage.GetRadioType())
        {
#if OPENTHREAD_CONFIG_RADIO_LINK_IEEE_802_15_4_ENABLE
        case Mac::kRadioTypeIeee802154:
            entry->mRadioIeee802154 = true;
            break;
#endif
#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
        case Mac::kRadioTypeTrel:
            entry->mRadioTrelUdp6 = true;
            break;
#endif
        }

        // Radio type may not be set on a tx message indicating that it
        // was sent over all radio types (e.g., for broadcast frame).
        // In such a case, we set all supported radios from `else`
        // block below.
    }
    else
#endif // OPENTHREAD_CONFIG_MULTI_RADIO
    {
#if OPENTHREAD_CONFIG_RADIO_LINK_IEEE_802_15_4_ENABLE
        entry->mRadioIeee802154 = true;
#endif

#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
        entry->mRadioTrelUdp6 = true;
#endif
    }

exit:
    return;
}

void Local::RecordNeighborEvent(NeighborTable::Event aEvent, const NeighborTable::EntryInfo &aInfo)
{
    NeighborInfo *entry = mNeighborHistory.AddNewEntry();

    VerifyOrExit(entry != nullptr);

    switch (aEvent)
    {
    case NeighborTable::kChildAdded:
    case NeighborTable::kChildRemoved:
    case NeighborTable::kChildModeChanged:
        entry->mExtAddress       = aInfo.mInfo.mChild.mExtAddress;
        entry->mRloc16           = aInfo.mInfo.mChild.mRloc16;
        entry->mAverageRssi      = aInfo.mInfo.mChild.mAverageRssi;
        entry->mRxOnWhenIdle     = aInfo.mInfo.mChild.mRxOnWhenIdle;
        entry->mFullThreadDevice = aInfo.mInfo.mChild.mFullThreadDevice;
        entry->mFullNetworkData  = aInfo.mInfo.mChild.mFullNetworkData;
        entry->mIsChild          = true;
        break;

    case NeighborTable::kRouterAdded:
    case NeighborTable::kRouterRemoved:
        entry->mExtAddress       = aInfo.mInfo.mRouter.mExtAddress;
        entry->mRloc16           = aInfo.mInfo.mRouter.mRloc16;
        entry->mAverageRssi      = aInfo.mInfo.mRouter.mAverageRssi;
        entry->mRxOnWhenIdle     = aInfo.mInfo.mRouter.mRxOnWhenIdle;
        entry->mFullThreadDevice = aInfo.mInfo.mRouter.mFullThreadDevice;
        entry->mFullNetworkData  = aInfo.mInfo.mRouter.mFullNetworkData;
        entry->mIsChild          = false;
        break;
    }

    switch (aEvent)
    {
    case NeighborTable::kChildAdded:
        if (aInfo.mInfo.mChild.mIsStateRestoring)
        {
            entry->mEvent = kNeighborRestoring;
            break;
        }

        OT_FALL_THROUGH;

    case NeighborTable::kRouterAdded:
        entry->mEvent = kNeighborAdded;
        break;

    case NeighborTable::kChildRemoved:
    case NeighborTable::kRouterRemoved:
        entry->mEvent = kNeighborRemoved;
        break;

    case NeighborTable::kChildModeChanged:
        entry->mEvent = kNeighborChanged;
        break;
    }

exit:
    return;
}

void Local::RecordAddressEvent(Ip6::Netif::AddressEvent aEvent, const Ip6::Netif::UnicastAddress &aUnicastAddress)
{
    UnicastAddressInfo *entry = mUnicastAddressHistory.AddNewEntry();

    VerifyOrExit(entry != nullptr);

    entry->mAddress       = aUnicastAddress.GetAddress();
    entry->mPrefixLength  = aUnicastAddress.GetPrefixLength();
    entry->mAddressOrigin = aUnicastAddress.GetOrigin();
    entry->mEvent         = (aEvent == Ip6::Netif::kAddressAdded) ? kAddressAdded : kAddressRemoved;
    entry->mScope         = (aUnicastAddress.GetScope() & 0xf);
    entry->mPreferred     = aUnicastAddress.mPreferred;
    entry->mValid         = aUnicastAddress.mValid;
    entry->mRloc          = aUnicastAddress.mRloc;

exit:
    return;
}

void Local::RecordAddressEvent(Ip6::Netif::AddressEvent            aEvent,
                               const Ip6::Netif::MulticastAddress &aMulticastAddress,
                               Ip6::Netif::AddressOrigin           aAddressOrigin)
{
    MulticastAddressInfo *entry = mMulticastAddressHistory.AddNewEntry();

    VerifyOrExit(entry != nullptr);

    entry->mAddress       = aMulticastAddress.GetAddress();
    entry->mAddressOrigin = aAddressOrigin;
    entry->mEvent         = (aEvent == Ip6::Netif::kAddressAdded) ? kAddressAdded : kAddressRemoved;

exit:
    return;
}

#if OPENTHREAD_FTD
void Local::RecordRouterTableChange(void)
{
#if OPENTHREAD_CONFIG_HISTORY_TRACKER_ROUTER_LIST_SIZE > 0

    for (uint8_t routerId = 0; routerId <= Mle::kMaxRouterId; routerId++)
    {
        RouterInfo   entry;
        RouterEntry &oldEntry = mRouterEntries[routerId];

        entry.mRouterId = routerId;

        if (Get<RouterTable>().IsAllocated(routerId))
        {
            uint16_t nextHopRloc;
            uint8_t  pathCost;

            Get<RouterTable>().GetNextHopAndPathCost(Mle::Rloc16FromRouterId(routerId), nextHopRloc, pathCost);

            entry.mNextHop  = (nextHopRloc == Mle::kInvalidRloc16) ? kNoNextHop : Mle::RouterIdFromRloc16(nextHopRloc);
            entry.mPathCost = (pathCost < Mle::kMaxRouteCost) ? pathCost : 0;

            if (!oldEntry.mIsAllocated)
            {
                entry.mEvent       = kRouterAdded;
                entry.mOldPathCost = 0;
            }
            else if (oldEntry.mNextHop != entry.mNextHop)
            {
                entry.mEvent       = kRouterNextHopChanged;
                entry.mOldPathCost = oldEntry.mPathCost;
            }
            else if ((entry.mNextHop != kNoNextHop) && (oldEntry.mPathCost != entry.mPathCost))
            {
                entry.mEvent       = kRouterCostChanged;
                entry.mOldPathCost = oldEntry.mPathCost;
            }
            else
            {
                continue;
            }

            mRouterHistory.AddNewEntry(entry);

            oldEntry.mIsAllocated = true;
            oldEntry.mNextHop     = entry.mNextHop;
            oldEntry.mPathCost    = entry.mPathCost;
        }
        else
        {
            // `routerId` is not allocated.

            if (oldEntry.mIsAllocated)
            {
                entry.mEvent       = kRouterRemoved;
                entry.mNextHop     = Mle::kInvalidRouterId;
                entry.mOldPathCost = 0;
                entry.mPathCost    = 0;

                mRouterHistory.AddNewEntry(entry);

                oldEntry.mIsAllocated = false;
            }
        }
    }

#endif // (OPENTHREAD_CONFIG_HISTORY_TRACKER_ROUTER_LIST_SIZE > 0)
}
#endif // OPENTHREAD_FTD

#if OPENTHREAD_CONFIG_HISTORY_TRACKER_NET_DATA
void Local::RecordNetworkDataChange(void)
{
    static const NetworkData::Service::DnsSrpUnicastType kDnsSrpUnicastTypes[] = {
        NetworkData::Service::kAddrInServiceData,
        NetworkData::Service::kAddrInServerData,
    };

    NetworkData::Iterator                   iterator;
    NetworkData::OnMeshPrefixConfig         prefix;
    NetworkData::ExternalRouteConfig        route;
    NetworkData::Service::DnsSrpUnicastInfo unicastInfo;
    NetworkData::Service::DnsSrpAnycastInfo anycastInfo;
    NetworkData::Service::Iterator          newDataIterator(GetInstance(), Get<NetworkData::Leader>());
    NetworkData::Service::Iterator          prvDataIterator(GetInstance(), mPreviousNetworkData);

    // On mesh prefix entries

    iterator = NetworkData::kIteratorInit;

    while (mPreviousNetworkData.GetNextOnMeshPrefix(iterator, prefix) == kErrorNone)
    {
        if (!Get<NetworkData::Leader>().ContainsOnMeshPrefix(prefix))
        {
            RecordOnMeshPrefixEvent(kNetDataEntryRemoved, prefix);
        }
    }

    iterator = NetworkData::kIteratorInit;

    while (Get<NetworkData::Leader>().GetNextOnMeshPrefix(iterator, prefix) == kErrorNone)
    {
        if (!mPreviousNetworkData.ContainsOnMeshPrefix(prefix))
        {
            RecordOnMeshPrefixEvent(kNetDataEntryAdded, prefix);
        }
    }

    // External route entries

    iterator = NetworkData::kIteratorInit;

    while (mPreviousNetworkData.GetNextExternalRoute(iterator, route) == kErrorNone)
    {
        if (!Get<NetworkData::Leader>().ContainsExternalRoute(route))
        {
            RecordExternalRouteEvent(kNetDataEntryRemoved, route);
        }
    }

    iterator = NetworkData::kIteratorInit;

    while (Get<NetworkData::Leader>().GetNextExternalRoute(iterator, route) == kErrorNone)
    {
        if (!mPreviousNetworkData.ContainsExternalRoute(route))
        {
            RecordExternalRouteEvent(kNetDataEntryAdded, route);
        }
    }

    // DNS/SRP Unicast/Anycast entries

    for (NetworkData::Service::DnsSrpUnicastType type : kDnsSrpUnicastTypes)
    {
        prvDataIterator.Reset();

        while (prvDataIterator.GetNextDnsSrpUnicastInfo(type, unicastInfo) == kErrorNone)
        {
            if (!NetDataContainsDnsSrpUnicast(Get<NetworkData::Leader>(), unicastInfo, type))
            {
                RecordDnsSrpAddrEvent(kNetDataEntryRemoved, unicastInfo, type);
            }
        }

        newDataIterator.Reset();

        while (newDataIterator.GetNextDnsSrpUnicastInfo(type, unicastInfo) == kErrorNone)
        {
            if (!NetDataContainsDnsSrpUnicast(mPreviousNetworkData, unicastInfo, type))
            {
                RecordDnsSrpAddrEvent(kNetDataEntryAdded, unicastInfo, type);
            }
        }
    }

    prvDataIterator.Reset();

    while (prvDataIterator.GetNextDnsSrpAnycastInfo(anycastInfo) == kErrorNone)
    {
        if (!NetDataContainsDnsSrpAnycast(Get<NetworkData::Leader>(), anycastInfo))
        {
            RecordDnsSrpAddrEvent(kNetDataEntryRemoved, anycastInfo);
        }
    }

    newDataIterator.Reset();

    while (newDataIterator.GetNextDnsSrpAnycastInfo(anycastInfo) == kErrorNone)
    {
        if (!NetDataContainsDnsSrpAnycast(mPreviousNetworkData, anycastInfo))
        {
            RecordDnsSrpAddrEvent(kNetDataEntryAdded, anycastInfo);
        }
    }

    SuccessOrAssert(Get<NetworkData::Leader>().CopyNetworkData(NetworkData::kFullSet, mPreviousNetworkData));
}

void Local::RecordOnMeshPrefixEvent(NetDataEvent aEvent, const NetworkData::OnMeshPrefixConfig &aPrefix)
{
    OnMeshPrefixInfo *entry = mOnMeshPrefixHistory.AddNewEntry();

    VerifyOrExit(entry != nullptr);
    entry->mPrefix = aPrefix;
    entry->mEvent  = aEvent;

exit:
    return;
}

void Local::RecordExternalRouteEvent(NetDataEvent aEvent, const NetworkData::ExternalRouteConfig &aRoute)
{
    ExternalRouteInfo *entry = mExternalRouteHistory.AddNewEntry();

    VerifyOrExit(entry != nullptr);
    entry->mRoute = aRoute;
    entry->mEvent = aEvent;

exit:
    return;
}

void Local::RecordDnsSrpAddrEvent(NetDataEvent                                   aEvent,
                                  const NetworkData::Service::DnsSrpUnicastInfo &aUnicastInfo,
                                  NetworkData::Service::DnsSrpUnicastType        aType)
{
    DnsSrpAddrInfo *entry = mDnsSrpAddrHistory.AddNewEntry();

    VerifyOrExit(entry != nullptr);

    entry->mAddress        = aUnicastInfo.mSockAddr.mAddress;
    entry->mRloc16         = aUnicastInfo.mRloc16;
    entry->mPort           = aUnicastInfo.mSockAddr.mPort;
    entry->mSequenceNumber = 0;
    entry->mVersion        = aUnicastInfo.mVersion;
    entry->mEvent          = aEvent;

    switch (aType)
    {
    case NetworkData::Service::kAddrInServerData:
        entry->mType = kDnsSrpAddrTypeUnicastLocal;
        break;
    case NetworkData::Service::kAddrInServiceData:
        entry->mType = kDnsSrpAddrTypeUnicastInfra;
        break;
    }

exit:
    return;
}

void Local::RecordDnsSrpAddrEvent(NetDataEvent aEvent, const NetworkData::Service::DnsSrpAnycastInfo &aAnycastInfo)
{
    DnsSrpAddrInfo *entry = mDnsSrpAddrHistory.AddNewEntry();

    VerifyOrExit(entry != nullptr);

    entry->mAddress        = aAnycastInfo.mAnycastAddress;
    entry->mRloc16         = aAnycastInfo.mRloc16;
    entry->mPort           = kAnycastServerPort;
    entry->mSequenceNumber = aAnycastInfo.mSequenceNumber;
    entry->mVersion        = aAnycastInfo.mVersion;
    entry->mEvent          = aEvent;
    entry->mType           = kDnsSrpAddrTypeAnycast;

exit:
    return;
}

bool Local::NetDataContainsDnsSrpUnicast(const NetworkData::NetworkData                &aNetworkData,
                                         const NetworkData::Service::DnsSrpUnicastInfo &aUnicastInfo,
                                         NetworkData::Service::DnsSrpUnicastType        aType) const
{
    bool                                    contains = false;
    NetworkData::Service::Iterator          iterator(GetInstance(), aNetworkData);
    NetworkData::Service::DnsSrpUnicastInfo unicastInfo;

    while (iterator.GetNextDnsSrpUnicastInfo(aType, unicastInfo) == kErrorNone)
    {
        if (unicastInfo == aUnicastInfo)
        {
            contains = true;
            break;
        }
    }

    return contains;
}

bool Local::NetDataContainsDnsSrpAnycast(const NetworkData::NetworkData                &aNetworkData,
                                         const NetworkData::Service::DnsSrpAnycastInfo &aAnycastInfo) const
{
    bool                                    contains = false;
    NetworkData::Service::Iterator          iterator(GetInstance(), aNetworkData);
    NetworkData::Service::DnsSrpAnycastInfo anycastInfo;

    while (iterator.GetNextDnsSrpAnycastInfo(anycastInfo) == kErrorNone)
    {
        if (anycastInfo == aAnycastInfo)
        {
            contains = true;
            break;
        }
    }

    return contains;
}

#endif // OPENTHREAD_CONFIG_HISTORY_TRACKER_NET_DATA

#if OPENTHREAD_CONFIG_BORDER_AGENT_ENABLE && OPENTHREAD_CONFIG_BORDER_AGENT_EPHEMERAL_KEY_ENABLE
void Local::RecordEpskcEvent(EpskcEvent aEvent)
{
    EpskcEvent *entry = mEpskcEventHistory.AddNewEntry();

    VerifyOrExit(entry != nullptr);
    *entry = aEvent;

exit:
    return;
}
#endif

void Local::HandleNotifierEvents(Events aEvents)
{
    if (aEvents.ContainsAny(kEventThreadRoleChanged | kEventThreadRlocAdded | kEventThreadRlocRemoved |
                            kEventThreadPartitionIdChanged))
    {
        RecordNetworkInfo();
    }

#if OPENTHREAD_CONFIG_HISTORY_TRACKER_NET_DATA
    if (aEvents.Contains(kEventThreadNetdataChanged))
    {
        RecordNetworkDataChange();
    }
#endif
}

void Local::HandleTimer(void)
{
    mNetInfoHistory.UpdateAgedEntries();
    mUnicastAddressHistory.UpdateAgedEntries();
    mMulticastAddressHistory.UpdateAgedEntries();
    mRxHistory.UpdateAgedEntries();
    mTxHistory.UpdateAgedEntries();
    mNeighborHistory.UpdateAgedEntries();
    mOnMeshPrefixHistory.UpdateAgedEntries();
    mExternalRouteHistory.UpdateAgedEntries();
    mDnsSrpAddrHistory.UpdateAgedEntries();
#if OPENTHREAD_CONFIG_BORDER_AGENT_ENABLE && OPENTHREAD_CONFIG_BORDER_AGENT_EPHEMERAL_KEY_ENABLE
    mEpskcEventHistory.UpdateAgedEntries();
#endif
    mTimer.Start(kAgeCheckPeriod);
}

void Local::EntryAgeToString(uint32_t aEntryAge, char *aBuffer, uint16_t aSize)
{
    StringWriter writer(aBuffer, aSize);

    if (aEntryAge >= kMaxAge)
    {
        writer.Append("more than %u days", static_cast<uint16_t>(kMaxAge / Time::kOneDayInMsec));
    }
    else
    {
        uint32_t days = aEntryAge / Time::kOneDayInMsec;

        if (days > 0)
        {
            writer.Append("%lu day%s ", ToUlong(days), (days == 1) ? "" : "s");
            aEntryAge -= days * Time::kOneDayInMsec;
        }

        writer.Append("%02u:%02u:%02u.%03u", static_cast<uint16_t>(aEntryAge / Time::kOneHourInMsec),
                      static_cast<uint16_t>((aEntryAge % Time::kOneHourInMsec) / Time::kOneMinuteInMsec),
                      static_cast<uint16_t>((aEntryAge % Time::kOneMinuteInMsec) / Time::kOneSecondInMsec),
                      static_cast<uint16_t>(aEntryAge % Time::kOneSecondInMsec));
    }
}

//---------------------------------------------------------------------------------------------------------------------
// Local::Timestamp

void Local::Timestamp::SetToNow(void)
{
    mTime = TimerMilli::GetNow();

    // If the current time happens to be the special value which we
    // use to indicate "distant past", decrement the time by one.

    if (mTime.GetValue() == kDistantPast)
    {
        mTime.SetValue(mTime.GetValue() - 1);
    }
}

uint32_t Local::Timestamp::GetDurationTill(TimeMilli aTime) const
{
    return IsDistantPast() ? kMaxAge : Min(aTime - mTime, kMaxAge);
}

//---------------------------------------------------------------------------------------------------------------------
// Local::List

Local::List::List(void)
    : mStartIndex(0)
    , mSize(0)
{
}

void Local::List::Clear(void)
{
    mStartIndex = 0;
    mSize       = 0;
}

uint16_t Local::List::Add(uint16_t aMaxSize, Timestamp aTimestamps[])
{
    // Add a new entry and return its list index. Overwrites the
    // oldest entry if list is full.
    //
    // Entries are saved in the order they are added such that
    // `mStartIndex` is the newest entry and the entries after up
    // to `mSize` are the previously added entries.

    mStartIndex = (mStartIndex == 0) ? aMaxSize - 1 : mStartIndex - 1;
    mSize += (mSize == aMaxSize) ? 0 : 1;

    aTimestamps[mStartIndex].SetToNow();

    return mStartIndex;
}

Error Local::List::Iterate(uint16_t        aMaxSize,
                           const Timestamp aTimestamps[],
                           Iterator       &aIterator,
                           uint16_t       &aListIndex,
                           uint32_t       &aEntryAge) const
{
    Error error = kErrorNone;

    VerifyOrExit(aIterator.GetEntryNumber() < mSize, error = kErrorNotFound);

    aListIndex = MapEntryNumberToListIndex(aIterator.GetEntryNumber(), aMaxSize);
    aEntryAge  = aTimestamps[aListIndex].GetDurationTill(aIterator.GetInitTime());

    aIterator.IncrementEntryNumber();

exit:
    return error;
}

uint16_t Local::List::MapEntryNumberToListIndex(uint16_t aEntryNumber, uint16_t aMaxSize) const
{
    // Map the `aEntryNumber` to the list index. `aEntryNumber` value
    // of zero corresponds to the newest (the most recently added)
    // entry and value one to next one and so on. List index
    // warps at the end of array to start of array. Caller MUST
    // ensure `aEntryNumber` is smaller than `mSize`.

    uint32_t index;

    OT_ASSERT(aEntryNumber < mSize);

    index = static_cast<uint32_t>(aEntryNumber) + mStartIndex;
    index -= (index >= aMaxSize) ? aMaxSize : 0;

    return static_cast<uint16_t>(index);
}

void Local::List::UpdateAgedEntries(uint16_t aMaxSize, Timestamp aTimestamps[])
{
    TimeMilli now = TimerMilli::GetNow();

    // We go through the entries in reverse (starting with the oldest
    // entry) and check if the entry's age is larger than `kMaxAge`
    // and if so mark it as "distant past". We can stop as soon as we
    // get to an entry with age smaller than max.
    //
    // The `for()` loop condition is `(entryNumber < mSize)` which
    // ensures that we go through the loop body for `entryNumber`
    // value of zero and then in the next iteration (when the
    // `entryNumber` rolls over) we stop.

    for (uint16_t entryNumber = mSize - 1; entryNumber < mSize; entryNumber--)
    {
        uint16_t index = MapEntryNumberToListIndex(entryNumber, aMaxSize);

        if (aTimestamps[index].GetDurationTill(now) < kMaxAge)
        {
            break;
        }

        aTimestamps[index].MarkAsDistantPast();
    }
}

} // namespace HistoryTracker
} // namespace ot

#endif // #if OPENTHREAD_CONFIG_HISTORY_TRACKER_ENABLE
