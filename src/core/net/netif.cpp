/*
 *  Copyright (c) 2016, The OpenThread Authors.
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
 *   This file implements IPv6 network interfaces.
 */

#include "netif.hpp"

#include "common/debug.hpp"
#include "common/instance.hpp"
#include "common/locator-getters.hpp"
#include "common/message.hpp"
#include "net/ip6.hpp"

namespace ot {
namespace Ip6 {

/*
 * Certain fixed multicast addresses are defined as a set of chained (linked-list) constant `otNetifMulticastAddress`
 * entries:
 *
 * LinkLocalAllRouters -> RealmLocalAllRouters -> LinkLocalAll -> RealmLocalAll -> RealmLocalAllMplForwarders -> nullptr
 *
 * All or a portion of the chain is appended to the end of `mMulticastAddresses` linked-list. If the interface is
 * subscribed to all-routers multicast addresses (using `SubscribeAllRoutersMulticast()`) then all the five entries
 * are appended. Otherwise only the last three are appended.
 *
 */

// "ff03::fc"
const otNetifMulticastAddress Netif::kRealmLocalAllMplForwardersMulticastAddress = {
    {{{0xff, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfc}}},
    nullptr};

// "ff03::01"
const otNetifMulticastAddress Netif::kRealmLocalAllNodesMulticastAddress = {
    {{{0xff, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01}}},
    &Netif::kRealmLocalAllMplForwardersMulticastAddress};

// "ff02::01"
const otNetifMulticastAddress Netif::kLinkLocalAllNodesMulticastAddress = {
    {{{0xff, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01}}},
    &Netif::kRealmLocalAllNodesMulticastAddress};

// "ff03::02"
const otNetifMulticastAddress Netif::kRealmLocalAllRoutersMulticastAddress = {
    {{{0xff, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02}}},
    &Netif::kLinkLocalAllNodesMulticastAddress};

// "ff02::02"
const otNetifMulticastAddress Netif::kLinkLocalAllRoutersMulticastAddress = {
    {{{0xff, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02}}},
    &Netif::kRealmLocalAllRoutersMulticastAddress};

Netif::Netif(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mUnicastAddresses()
    , mMulticastAddresses()
    , mMulticastPromiscuous(false)
    , mAddressCallback(nullptr)
    , mAddressCallbackContext(nullptr)
    , mExtUnicastAddressPool()
    , mExtMulticastAddressPool()
{
}

bool Netif::IsMulticastSubscribed(const Address &aAddress) const
{
    const NetifMulticastAddress *prev;

    return mMulticastAddresses.FindMatching(aAddress, prev) != nullptr;
}

void Netif::SubscribeAllNodesMulticast(void)
{
    NetifMulticastAddress *tail;
    NetifMulticastAddress &linkLocalAllNodesAddress =
        static_cast<NetifMulticastAddress &>(const_cast<otNetifMulticastAddress &>(kLinkLocalAllNodesMulticastAddress));

    VerifyOrExit(!mMulticastAddresses.Contains(linkLocalAllNodesAddress), OT_NOOP);

    // Append the fixed chain of three multicast addresses to the
    // tail of the list:
    //
    //    LinkLocalAll -> RealmLocalAll -> RealmLocalAllMpl.

    tail = mMulticastAddresses.GetTail();

    if (tail == nullptr)
    {
        mMulticastAddresses.SetHead(&linkLocalAllNodesAddress);
    }
    else
    {
        tail->SetNext(&linkLocalAllNodesAddress);
    }

    Get<Notifier>().Signal(kEventIp6MulticastSubscribed);

    VerifyOrExit(mAddressCallback != nullptr, OT_NOOP);

    for (const NetifMulticastAddress *entry = &linkLocalAllNodesAddress; entry; entry = entry->GetNext())
    {
        mAddressCallback(&entry->GetAddress(), kMulticastPrefixLength, /* IsAdded */ true, mAddressCallbackContext);
    }

exit:
    return;
}

void Netif::UnsubscribeAllNodesMulticast(void)
{
    NetifMulticastAddress *      prev;
    const NetifMulticastAddress &linkLocalAllNodesAddress =
        static_cast<NetifMulticastAddress &>(const_cast<otNetifMulticastAddress &>(kLinkLocalAllNodesMulticastAddress));

    // The tail of multicast address linked list contains the
    // fixed addresses. Search if LinkLocalAll is present
    // in the list and find entry before it.
    //
    //    LinkLocalAll -> RealmLocalAll -> RealmLocalAllMpl.

    SuccessOrExit(mMulticastAddresses.Find(linkLocalAllNodesAddress, prev));

    // This method MUST be called after `UnsubscribeAllRoutersMulticast().
    // Verify this by checking the chain at the end of the list only
    // contains three entries and not the five fixed addresses (check that
    // `prev` entry before `LinkLocalAll` is not `RealmLocalRouters`):
    //
    //    LinkLocalAllRouters -> RealmLocalAllRouters -> LinkLocalAll
    //         -> RealmLocalAll -> RealmLocalAllMpl.

    OT_ASSERT(prev != static_cast<NetifMulticastAddress *>(
                          const_cast<otNetifMulticastAddress *>(&kRealmLocalAllRoutersMulticastAddress)));

    if (prev == nullptr)
    {
        mMulticastAddresses.Clear();
    }
    else
    {
        prev->SetNext(nullptr);
    }

    Get<Notifier>().Signal(kEventIp6MulticastUnsubscribed);

    VerifyOrExit(mAddressCallback != nullptr, OT_NOOP);

    for (const NetifMulticastAddress *entry = &linkLocalAllNodesAddress; entry; entry = entry->GetNext())
    {
        mAddressCallback(&entry->GetAddress(), kMulticastPrefixLength, /* IsAdded */ false, mAddressCallbackContext);
    }

exit:
    return;
}

void Netif::SubscribeAllRoutersMulticast(void)
{
    otError                error                      = OT_ERROR_NONE;
    NetifMulticastAddress *prev                       = nullptr;
    NetifMulticastAddress &linkLocalAllRoutersAddress = static_cast<NetifMulticastAddress &>(
        const_cast<otNetifMulticastAddress &>(kLinkLocalAllRoutersMulticastAddress));
    NetifMulticastAddress &linkLocalAllNodesAddress =
        static_cast<NetifMulticastAddress &>(const_cast<otNetifMulticastAddress &>(kLinkLocalAllNodesMulticastAddress));
    NetifMulticastAddress &realmLocalAllRoutersAddress = static_cast<NetifMulticastAddress &>(
        const_cast<otNetifMulticastAddress &>(kRealmLocalAllRoutersMulticastAddress));

    error = mMulticastAddresses.Find(linkLocalAllNodesAddress, prev);

    // This method MUST be called after `SubscribeAllNodesMulticast()`
    // Ensure that the `LinkLocalAll` was found on the list.

    OT_ASSERT(error == OT_ERROR_NONE);
    OT_UNUSED_VARIABLE(error);

    // The tail of multicast address linked list contains the
    // fixed addresses. We either have a chain of five addresses
    //
    //    LinkLocalAllRouters -> RealmLocalAllRouters ->
    //        LinkLocalAll -> RealmLocalAll -> RealmLocalAllMpl.
    //
    // or just the last three addresses
    //
    //    LinkLocalAll -> RealmLocalAll -> RealmLocalAllMpl.
    //
    // If the previous entry behind `LinkLocalAll` is
    // `RealmLocalAllRouters` then all five addresses are on
    // the list already.

    VerifyOrExit(prev != &realmLocalAllRoutersAddress, OT_NOOP);

    if (prev == nullptr)
    {
        mMulticastAddresses.SetHead(&linkLocalAllRoutersAddress);
    }
    else
    {
        prev->SetNext(&linkLocalAllRoutersAddress);
    }

    Get<Notifier>().Signal(kEventIp6MulticastSubscribed);

    VerifyOrExit(mAddressCallback != nullptr, OT_NOOP);

    for (const NetifMulticastAddress *entry = &linkLocalAllRoutersAddress; entry != &linkLocalAllNodesAddress;
         entry                              = entry->GetNext())
    {
        mAddressCallback(&entry->GetAddress(), kMulticastPrefixLength, /* IsAdded */ true, mAddressCallbackContext);
    }

exit:
    return;
}

void Netif::UnsubscribeAllRoutersMulticast(void)
{
    NetifMulticastAddress *prev;
    NetifMulticastAddress &linkLocalAllRoutersAddress = static_cast<NetifMulticastAddress &>(
        const_cast<otNetifMulticastAddress &>(kLinkLocalAllRoutersMulticastAddress));
    NetifMulticastAddress &linkLocalAllNodesAddress =
        static_cast<NetifMulticastAddress &>(const_cast<otNetifMulticastAddress &>(kLinkLocalAllNodesMulticastAddress));

    // The tail of multicast address linked list contains the
    // fixed addresses. We check for the chain of five addresses:
    //
    //    LinkLocalAllRouters -> RealmLocalAllRouters ->
    //        LinkLocalAll -> RealmLocalAll -> RealmLocalAllMpl.
    //
    // If found, we then replace the entry behind `LinkLocalAllRouters`
    // to point to `LinkLocalAll` instead (so that tail contains the
    // three fixed addresses at end of the chain).

    SuccessOrExit(mMulticastAddresses.Find(linkLocalAllRoutersAddress, prev));

    if (prev == nullptr)
    {
        mMulticastAddresses.SetHead(&linkLocalAllNodesAddress);
    }
    else
    {
        prev->SetNext(&linkLocalAllNodesAddress);
    }

    Get<Notifier>().Signal(kEventIp6MulticastUnsubscribed);

    VerifyOrExit(mAddressCallback != nullptr, OT_NOOP);

    for (const NetifMulticastAddress *entry = &linkLocalAllRoutersAddress; entry != &linkLocalAllNodesAddress;
         entry                              = entry->GetNext())
    {
        mAddressCallback(&entry->GetAddress(), kMulticastPrefixLength, /* IsAdded */ false, mAddressCallbackContext);
    }

exit:
    return;
}

bool Netif::IsMulticastAddressExternal(const NetifMulticastAddress &aAddress) const
{
    return mExtMulticastAddressPool.IsPoolEntry(static_cast<const ExternalNetifMulticastAddress &>(aAddress));
}

void Netif::SubscribeMulticast(NetifMulticastAddress &aAddress)
{
    SuccessOrExit(mMulticastAddresses.Add(aAddress));

    Get<Notifier>().Signal(kEventIp6MulticastSubscribed);

    VerifyOrExit(mAddressCallback != nullptr, OT_NOOP);
    mAddressCallback(&aAddress.mAddress, kMulticastPrefixLength, /* IsAdded */ true, mAddressCallbackContext);

exit:
    return;
}

void Netif::UnsubscribeMulticast(const NetifMulticastAddress &aAddress)
{
    SuccessOrExit(mMulticastAddresses.Remove(aAddress));

    Get<Notifier>().Signal(kEventIp6MulticastUnsubscribed);

    VerifyOrExit(mAddressCallback != nullptr, OT_NOOP);
    mAddressCallback(&aAddress.mAddress, kMulticastPrefixLength, /* IsAdded */ false, mAddressCallbackContext);

exit:
    return;
}

otError Netif::SubscribeExternalMulticast(const Address &aAddress)
{
    otError                error                      = OT_ERROR_NONE;
    NetifMulticastAddress &linkLocalAllRoutersAddress = static_cast<NetifMulticastAddress &>(
        const_cast<otNetifMulticastAddress &>(kLinkLocalAllRoutersMulticastAddress));
    ExternalNetifMulticastAddress *entry;

    VerifyOrExit(aAddress.IsMulticast(), error = OT_ERROR_INVALID_ARGS);
    VerifyOrExit(!IsMulticastSubscribed(aAddress), error = OT_ERROR_ALREADY);

    // Check that the address is not one of the fixed addresses:
    // LinkLocalAllRouters -> RealmLocalAllRouters -> LinkLocalAllNodes
    // -> RealmLocalAllNodes -> RealmLocalAllMpl.

    for (const NetifMulticastAddress *cur = &linkLocalAllRoutersAddress; cur; cur = cur->GetNext())
    {
        VerifyOrExit(cur->GetAddress() != aAddress, error = OT_ERROR_INVALID_ARGS);
    }

    entry = mExtMulticastAddressPool.Allocate();
    VerifyOrExit(entry != nullptr, error = OT_ERROR_NO_BUFS);

    entry->mAddress = aAddress;
#if OPENTHREAD_CONFIG_MLR_ENABLE
    entry->mMlrState = kMlrStateToRegister;
#endif
    mMulticastAddresses.Push(*entry);
    Get<Notifier>().Signal(kEventIp6MulticastSubscribed);

exit:
    return error;
}

otError Netif::UnsubscribeExternalMulticast(const Address &aAddress)
{
    otError                error = OT_ERROR_NONE;
    NetifMulticastAddress *entry;
    NetifMulticastAddress *prev;

    entry = mMulticastAddresses.FindMatching(aAddress, prev);
    VerifyOrExit(entry != nullptr, error = OT_ERROR_NOT_FOUND);

    VerifyOrExit(IsMulticastAddressExternal(*entry), error = OT_ERROR_INVALID_ARGS);

    mMulticastAddresses.PopAfter(prev);

    mExtMulticastAddressPool.Free(static_cast<ExternalNetifMulticastAddress &>(*entry));

    Get<Notifier>().Signal(kEventIp6MulticastUnsubscribed);

exit:
    return error;
}

void Netif::UnsubscribeAllExternalMulticastAddresses(void)
{
    NetifMulticastAddress *next;

    for (NetifMulticastAddress *entry = mMulticastAddresses.GetHead(); entry != nullptr; entry = next)
    {
        next = entry->GetNext();

        if (IsMulticastAddressExternal(*entry))
        {
            IgnoreError(UnsubscribeExternalMulticast(entry->GetAddress()));
        }
    }
}

void Netif::SetAddressCallback(otIp6AddressCallback aCallback, void *aCallbackContext)
{
    mAddressCallback        = aCallback;
    mAddressCallbackContext = aCallbackContext;
}

void Netif::AddUnicastAddress(NetifUnicastAddress &aAddress)
{
    SuccessOrExit(mUnicastAddresses.Add(aAddress));

    Get<Notifier>().Signal(aAddress.mRloc ? kEventThreadRlocAdded : kEventIp6AddressAdded);

    VerifyOrExit(mAddressCallback != nullptr, OT_NOOP);
    mAddressCallback(&aAddress.mAddress, aAddress.mPrefixLength, /* IsAdded */ true, mAddressCallbackContext);

exit:
    return;
}

void Netif::RemoveUnicastAddress(const NetifUnicastAddress &aAddress)
{
    SuccessOrExit(mUnicastAddresses.Remove(aAddress));

    Get<Notifier>().Signal(aAddress.mRloc ? kEventThreadRlocRemoved : kEventIp6AddressRemoved);

    VerifyOrExit(mAddressCallback != nullptr, OT_NOOP);
    mAddressCallback(&aAddress.mAddress, aAddress.mPrefixLength, /* IsAdded */ false, mAddressCallbackContext);

exit:
    return;
}

otError Netif::AddExternalUnicastAddress(const NetifUnicastAddress &aAddress)
{
    otError              error = OT_ERROR_NONE;
    NetifUnicastAddress *entry;
    NetifUnicastAddress *prev;

    VerifyOrExit(!aAddress.GetAddress().IsMulticast(), error = OT_ERROR_INVALID_ARGS);

    entry = mUnicastAddresses.FindMatching(aAddress.GetAddress(), prev);

    if (entry != nullptr)
    {
        VerifyOrExit(IsUnicastAddressExternal(*entry), error = OT_ERROR_ALREADY);

        entry->mPrefixLength  = aAddress.mPrefixLength;
        entry->mAddressOrigin = aAddress.mAddressOrigin;
        entry->mPreferred     = aAddress.mPreferred;
        entry->mValid         = aAddress.mValid;
        ExitNow();
    }

    VerifyOrExit(!aAddress.GetAddress().IsLinkLocal(), error = OT_ERROR_INVALID_ARGS);

    entry = mExtUnicastAddressPool.Allocate();
    VerifyOrExit(entry != nullptr, error = OT_ERROR_NO_BUFS);

    *entry = aAddress;
    mUnicastAddresses.Push(*entry);
    Get<Notifier>().Signal(kEventIp6AddressAdded);

exit:
    return error;
}

otError Netif::RemoveExternalUnicastAddress(const Address &aAddress)
{
    otError              error = OT_ERROR_NONE;
    NetifUnicastAddress *entry;
    NetifUnicastAddress *prev;

    entry = mUnicastAddresses.FindMatching(aAddress, prev);
    VerifyOrExit(entry != nullptr, error = OT_ERROR_NOT_FOUND);

    VerifyOrExit(IsUnicastAddressExternal(*entry), error = OT_ERROR_INVALID_ARGS);

    mUnicastAddresses.PopAfter(prev);
    mExtUnicastAddressPool.Free(*entry);
    Get<Notifier>().Signal(kEventIp6AddressRemoved);

exit:
    return error;
}

void Netif::RemoveAllExternalUnicastAddresses(void)
{
    NetifUnicastAddress *next;

    for (NetifUnicastAddress *entry = mUnicastAddresses.GetHead(); entry != nullptr; entry = next)
    {
        next = entry->GetNext();

        if (IsUnicastAddressExternal(*entry))
        {
            IgnoreError(RemoveExternalUnicastAddress(entry->GetAddress()));
        }
    }
}

bool Netif::HasUnicastAddress(const Address &aAddress) const
{
    const NetifUnicastAddress *prev;

    return mUnicastAddresses.FindMatching(aAddress, prev) != nullptr;
}

bool Netif::IsUnicastAddressExternal(const NetifUnicastAddress &aAddress) const
{
    return mExtUnicastAddressPool.IsPoolEntry(aAddress);
}

void NetifUnicastAddress::InitAsThreadOrigin(void)
{
    Clear();
    mPrefixLength  = NetworkPrefix::kLength;
    mAddressOrigin = OT_ADDRESS_ORIGIN_THREAD;
    mPreferred     = true;
    mValid         = true;
}

void NetifUnicastAddress::InitAsThreadOriginRealmLocalScope(void)
{
    InitAsThreadOrigin();
    SetScopeOverride(Address::kRealmLocalScope);
}

void NetifUnicastAddress::InitAsThreadOriginGlobalScope(void)
{
    Clear();
    mAddressOrigin = OT_ADDRESS_ORIGIN_THREAD;
    mValid         = true;
    SetScopeOverride(Address::kGlobalScope);
}

void NetifUnicastAddress::InitAsSlaacOrigin(uint8_t aPrefixLength, bool aPreferred)
{
    Clear();
    mPrefixLength  = aPrefixLength;
    mAddressOrigin = OT_ADDRESS_ORIGIN_SLAAC;
    mPreferred     = aPreferred;
    mValid         = true;
}

} // namespace Ip6
} // namespace ot
