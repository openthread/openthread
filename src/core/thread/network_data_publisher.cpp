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
 *   This file implements the Network Data Publisher.
 *
 */

#include "network_data_publisher.hpp"

#if OPENTHREAD_CONFIG_NETDATA_PUBLISHER_ENABLE

#include "common/code_utils.hpp"
#include "common/instance.hpp"
#include "common/locator_getters.hpp"
#include "common/random.hpp"
#include "thread/network_data_local.hpp"
#include "thread/network_data_service.hpp"

namespace ot {
namespace NetworkData {

Publisher::Publisher(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mState(kNoEntry)
    , mTimer(aInstance, Publisher::HandleTimer)
{
}

void Publisher::PublishDnsSrpServiceAnycast(uint8_t aSequenceNumber)
{
    otLogInfoNetData("Publisher: Publishing DNS/SRP service anycast (seq-num:%d)", aSequenceNumber);
    Publish(kTypeAnycast, aSequenceNumber, nullptr);
}

void Publisher::PublishDnsSrpServiceUnicast(const Ip6::Address &aAddress, uint16_t aPort)
{
    otLogInfoNetData("Publisher: Publishing DNS/SRP service unicast (%s, port:%d)", aAddress.ToString().AsCString(),
                     aPort);
    Publish(kTypeUnicast, aPort, &aAddress);
}

void Publisher::PublishDnsSrpServiceUnicast(uint16_t aPort)
{
    otLogInfoNetData("Publisher: Publishing DNS/SRP service unicast (ml-eid, port:%d)", aPort);
    Publish(kTypeUincastMeshLocalEid, aPort, &Get<Mle::Mle>().GetMeshLocal64());
}

void Publisher::UnpublishDnsSrpService(void)
{
    otLogInfoNetData("Publisher: Unpublishing DNS/SRP service");

    Remove(/* aNextState */ kNoEntry);
}

void Publisher::Publish(Type aType, uint16_t aPortOrSeqNumber, const Ip6::Address *aAddress)
{
    if (mState != kNoEntry)
    {
        if ((mType == aType) && (aPortOrSeqNumber == mPortOrSeqNumber) &&
            ((aAddress == nullptr) || (*aAddress == mAddress)))
        {
            otLogInfoNetData("Publisher: Same service is already being published");
            ExitNow();
        }

        Remove(/* aNextState */ kNoEntry);
    }

    mType            = aType;
    mPortOrSeqNumber = aPortOrSeqNumber;

    if (aAddress != nullptr)
    {
        mAddress = *aAddress;
    }

    SetState(kToAdd);
    Process();

exit:
    return;
}

void Publisher::SetState(State aState)
{
    VerifyOrExit(mState != aState);

    otLogInfoNetData("Publisher: State: %s -> %s", StateToString(mState), StateToString(aState));
    mState = aState;

exit:
    return;
}

void Publisher::Process(void)
{
    // This method checks the entries currently present in Network Data
    // based on which it then decides whether or not take action
    // (add/remove or keep monitoring).

    uint8_t numEntries          = 0;
    uint8_t numPreferredEntries = 0;
    uint8_t desiredNumEntries   = 0;

    // Do not make any changes if device is not attached, and wait
    // for role change event.
    VerifyOrExit(Get<Mle::Mle>().IsAttached());

    VerifyOrExit(mState != kNoEntry);

    switch (mType)
    {
    case kTypeAnycast:
        CountAnycastEntries(numEntries, numPreferredEntries);
        desiredNumEntries = kDesiredNumAnycast;
        break;

    case kTypeUnicast:
    case kTypeUincastMeshLocalEid:
        CountUnicastEntries(numEntries, numPreferredEntries);
        desiredNumEntries = kDesiredNumUnicast;
        break;
    }

    UpdateState(numEntries, numPreferredEntries, desiredNumEntries);

exit:
    return;
}

void Publisher::CountAnycastEntries(uint8_t &aNumEntries, uint8_t &aNumPreferredEntries) const
{
    // Count the number of matching "DNS/SRP Anycast" service entries
    // in the Network Data (the match requires the entry to use same
    // "sequence number" value). We prefer the entries associated with
    // smaller RLCO16.

    Service::DnsSrpAnycast::ServiceData serviceData(static_cast<uint8_t>(mPortOrSeqNumber));
    const ServiceTlv *                  serviceTlv = nullptr;

    while ((serviceTlv = Get<Leader>().FindNextMatchingService(serviceTlv, Service::kThreadEnterpriseNumber,
                                                               reinterpret_cast<const uint8_t *>(&serviceData),
                                                               serviceData.GetLength())) != nullptr)
    {
        TlvIterator      subTlvIterator(*serviceTlv);
        const ServerTlv *serverSubTlv;

        while ((serverSubTlv = subTlvIterator.Iterate<ServerTlv>()) != nullptr)
        {
            aNumEntries++;

            if (IsPreferred(serverSubTlv->GetServer16()))
            {
                aNumPreferredEntries++;
            }
        }
    }
}

void Publisher::CountUnicastEntries(uint8_t &aNumEntries, uint8_t &aNumPreferredEntries) const
{
    // Count the number of "DNS/SRP Unicast" service entries in
    // the Network Data.

    const ServiceTlv *serviceTlv = nullptr;

    while ((serviceTlv = Get<Leader>().FindNextMatchingService(
                serviceTlv, Service::kThreadEnterpriseNumber, &Service::DnsSrpUnicast::kServiceData,
                sizeof(Service::DnsSrpUnicast::kServiceData))) != nullptr)
    {
        TlvIterator      subTlvIterator(*serviceTlv);
        const ServerTlv *serverSubTlv;

        while (((serverSubTlv = subTlvIterator.Iterate<ServerTlv>())) != nullptr)
        {
            if (serviceTlv->GetServiceDataLength() >= sizeof(Service::DnsSrpUnicast::ServiceData))
            {
                aNumEntries++;

                // Generally, we prefer entries where the SRP/DNS server
                // address/port info is included in the service TLV data
                // over the ones where the info is included in the
                // server TLV data (i.e., we prefer infra-provided
                // SRP/DNS entry over a BR local one using MLE-EID). If
                // our entry itself uses the service TLV data, then we
                // prefer based on the associated RLOC16.

                if (mType == kTypeUnicast)
                {
                    if (IsPreferred(serverSubTlv->GetServer16()))
                    {
                        aNumPreferredEntries++;
                    }
                }
                else
                {
                    aNumPreferredEntries++;
                }
            }

            if (serverSubTlv->GetServerDataLength() >= sizeof(Service::DnsSrpUnicast::ServerData))
            {
                aNumEntries++;

                // If our entry also uses the server TLV data (with
                // ML-EID address), then the we prefer based on the
                // associated RLOC16.

                if ((mType == kTypeUincastMeshLocalEid) && IsPreferred(serverSubTlv->GetServer16()))
                {
                    aNumPreferredEntries++;
                }
            }
        }
    }
}

bool Publisher::IsPreferred(uint16_t aRloc16) const
{
    // Indicates whether or not an entry from `aRloc16` is preferred
    // over our entry (based on our RLOC). We prefer an entry from a
    // router over an entry from an end-device (e.g., a aREED). If both
    // are the same type, then the one with smaller RLOC16 is preferred.

    bool isOtherRouter = Mle::Mle::IsActiveRouter(aRloc16);

    return (Get<Mle::Mle>().IsRouterOrLeader() == isOtherRouter) ? (aRloc16 < Get<Mle::Mle>().GetRloc16())
                                                                 : isOtherRouter;
}

void Publisher::UpdateState(uint8_t aNumEntries, uint8_t aNumPreferredEntries, uint8_t aDesiredNumEntries)
{
    // This method uses the info about number existing entries (total
    // and preferred) in Network Data along with the desired number of
    // entries we aim to have in the Network Data to decide whether or
    // not to take any action (add or remove our entry).

    otLogInfoNetData("Publisher: total:%d, preferred:%d, desired:%d", aNumEntries, aNumPreferredEntries,
                     aDesiredNumEntries);

    switch (mState)
    {
    case kNoEntry:
        break;

    case kToAdd:
        // Our entry is ready to be added. If there are too few existing
        // entries, we start adding our entry (start the timer with a
        // random wait time before adding the entry).

        if (aNumEntries < aDesiredNumEntries)
        {
            uint32_t waitInterval = Random::NonCrypto::GetUint32InRange(1, kMaxWaitTimeToAdd);
            SetState(kAdding);
            mTimer.Start(waitInterval);
            LogWaitInterval(waitInterval);
        }
        break;

    case kAdding:
        // Our entry is being added (waiting time before we add). If we
        // now see that there are enough entries, we stop adding the
        // entry.

        if (aNumEntries >= aDesiredNumEntries)
        {
            SetState(kToAdd);
        }
        break;

    case kAdded:
        // Our entry is already added in the Network Data. If there are
        // enough entries, do nothing and keep monitoring. If we see now
        // that there are too many entries, we start removing our entry
        // after a random wait time. If our entry itself is preferred
        // over other entries (indicated by `aNumPreferredEntries <
        // aDesiredNumEntries`) we add an extra wait time before removing
        // the entry. This gives higher chance for a non-preferred
        // entry from another device to be removed before our entry.

        if (aNumEntries > aDesiredNumEntries)
        {
            uint32_t waitInterval = Random::NonCrypto::GetUint32InRange(1, kMaxWaitTimeToRemove);

            if (aNumPreferredEntries < aDesiredNumEntries)
            {
                waitInterval += kExtraWaitToRemovePeferred;
            }

            SetState(kRemoving);
            mTimer.Start(waitInterval);
            LogWaitInterval(waitInterval);
        }
        break;

    case kRemoving:
        // Our entry is being removed (wait time before remove). If we
        // now see that there are enough or too few entries, we stop
        // removing our entry.

        if (aNumEntries <= aDesiredNumEntries)
        {
            SetState(kAdded);
        }
        break;
    }
}

void Publisher::HandleTimer(Timer &aTimer)
{
    aTimer.Get<Publisher>().HandleTimer();
}

void Publisher::HandleTimer(void)
{
    if (mState == kAdding)
    {
        Add();
    }
    else if (mState == kRemoving)
    {
        Remove(/* aNextState */ kToAdd);
    }
}

void Publisher::Add(void)
{
    // Adds the service entry to the network data.

    switch (mType)
    {
    case kTypeAnycast:
        SuccessOrExit(Get<Service::Manager>().Add<Service::DnsSrpAnycast>(
            Service::DnsSrpAnycast::ServiceData(static_cast<uint8_t>(mPortOrSeqNumber))));
        break;

    case kTypeUnicast:
        SuccessOrExit(Get<Service::Manager>().Add<Service::DnsSrpUnicast>(
            Service::DnsSrpUnicast::ServiceData(mAddress, mPortOrSeqNumber)));
        break;

    case kTypeUincastMeshLocalEid:
        SuccessOrExit(Get<Service::Manager>().Add<Service::DnsSrpUnicast>(
            Service::DnsSrpUnicast::ServerData(mAddress, mPortOrSeqNumber)));
        break;
    }

    SetState(kAdded);

    Get<Notifier>().HandleServerDataUpdated();

exit:
    return;
}

void Publisher::Remove(State aNextState, bool aRegisterWithLeader)
{
    // Removes the service entry from network data (if it was added).

    VerifyOrExit((mState == kAdded) || (mState == kRemoving));

    switch (mType)
    {
    case kTypeAnycast:
        SuccessOrExit(Get<Service::Manager>().Remove<Service::DnsSrpAnycast>(
            Service::DnsSrpAnycast::ServiceData(static_cast<uint8_t>(mPortOrSeqNumber))));
        break;

    case kTypeUnicast:
        SuccessOrExit(Get<Service::Manager>().Remove<Service::DnsSrpUnicast>(
            Service::DnsSrpUnicast::ServiceData(mAddress, mPortOrSeqNumber)));
        break;

    case kTypeUincastMeshLocalEid:
        SuccessOrExit(Get<Service::Manager>().Remove<Service::DnsSrpUnicast>());
        break;
    }

    if (aRegisterWithLeader)
    {
        Get<Notifier>().HandleServerDataUpdated();
    }

    otLogInfoNetData("Publisher: Removed DNS/SRP service from network data");

exit:
    SetState(aNextState);
}

void Publisher::HandleNotifierEvents(Events aEvents)
{
    if ((mType == kTypeUincastMeshLocalEid) && aEvents.Contains(kEventThreadMeshLocalAddrChanged))
    {
        mAddress = Get<Mle::Mle>().GetMeshLocal64();

        if (mState == kAdded)
        {
            // If the entry is already added, we need to update it
            // so we remove it and add it back immediately with
            // the new mesh-local address.

            Remove(/* aNextState */ kAdding);
            Add();
        }
    }

    if (aEvents.ContainsAny(kEventThreadNetdataChanged | kEventThreadRoleChanged))
    {
        Process();
    }
}

void Publisher::LogWaitInterval(uint32_t aWaitInterval) const
{
    OT_UNUSED_VARIABLE(aWaitInterval);

    otLogInfoNetData("Publisher: DNS/SRP service (state:%s) - update in %u msec", StateToString(mState), aWaitInterval);
}

const char *Publisher::StateToString(State aState)
{
    static const char *const kStateStrings[] = {
        "NoEntry",  // (0) kNoEntry
        "ToAdd",    // (1) kToAdd
        "Adding",   // (2) kAdding
        "Added",    // (3) kAdded
        "Removing", // (4) kRemoving
    };

    static_assert(0 == kNoEntry, "kNoEntry value is not correct");
    static_assert(1 == kToAdd, "kToAdd value is not correct");
    static_assert(2 == kAdding, "kAdding value is not correct");
    static_assert(3 == kAdded, "kAdded value is not correct");
    static_assert(4 == kRemoving, "kRemoving value is not correct");

    return kStateStrings[aState];
}

} // namespace NetworkData
} // namespace ot

#endif // OPENTHREAD_CONFIG_NETDATA_PUBLISHER_ENABLE
