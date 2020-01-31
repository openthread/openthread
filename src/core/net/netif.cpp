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

#include "common/code_utils.hpp"
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
 * LinkLocalAllRouters -> RealmLocalAllRouters -> LinkLocalAll -> RealmLocalAll -> RealmLocalAllMplForwarders -> NULL
 *
 * All or a portion of the chain is appended to the end of `mMulticastAddresses` linked-list. If the interface is
 * subscribed to all-routers multicast addresses (using `SubscribeAllRoutersMulticast()`) then all the five entries
 * are appended. Otherwise only the last three are appended.
 *
 */

// "ff03::fc"
const otNetifMulticastAddress Netif::kRealmLocalAllMplForwardersMulticastAddress = {
    {{{0xff, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfc}}},
    NULL};

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
    , mAddressCallback(NULL)
    , mAddressCallbackContext(NULL)
{
    for (NetifUnicastAddress *entry = &mExtUnicastAddresses[0]; entry < OT_ARRAY_END(mExtUnicastAddresses); entry++)
    {
        entry->MarkAsNotInUse();
    }

    for (NetifMulticastAddress *entry = &mExtMulticastAddresses[0]; entry < OT_ARRAY_END(mExtMulticastAddresses);
         entry++)
    {
        entry->MarkAsNotInUse();
    }
}

bool Netif::IsMulticastSubscribed(const Address &aAddress) const
{
    bool rval = false;

    for (const NetifMulticastAddress *cur = mMulticastAddresses.GetHead(); cur; cur = cur->GetNext())
    {
        if (cur->GetAddress() == aAddress)
        {
            ExitNow(rval = true);
        }
    }

exit:
    return rval;
}

otError Netif::SubscribeAllNodesMulticast(void)
{
    otError                error = OT_ERROR_NONE;
    NetifMulticastAddress *tail;
    NetifMulticastAddress &linkLocalAllNodesAddress =
        static_cast<NetifMulticastAddress &>(const_cast<otNetifMulticastAddress &>(kLinkLocalAllNodesMulticastAddress));

    VerifyOrExit(!mMulticastAddresses.Contains(linkLocalAllNodesAddress), error = OT_ERROR_ALREADY);

    // Append the fixed chain of three multicast addresses to the
    // tail of the list:
    //
    //    LinkLocalAll -> RealmLocalAll -> RealmLocalAllMpl.

    tail = mMulticastAddresses.GetTail();

    if (tail == NULL)
    {
        mMulticastAddresses.SetHead(&linkLocalAllNodesAddress);
    }
    else
    {
        tail->SetNext(&linkLocalAllNodesAddress);
    }

    Get<Notifier>().Signal(OT_CHANGED_IP6_MULTICAST_SUBSCRIBED);

    VerifyOrExit(mAddressCallback != NULL);

    for (const NetifMulticastAddress *entry = &linkLocalAllNodesAddress; entry; entry = entry->GetNext())
    {
        mAddressCallback(&entry->GetAddress(), kMulticastPrefixLength, /* IsAdded */ true, mAddressCallbackContext);
    }

exit:
    return error;
}

otError Netif::UnsubscribeAllNodesMulticast(void)
{
    otError                      error = OT_ERROR_NONE;
    NetifMulticastAddress *      prev;
    const NetifMulticastAddress &linkLocalAllNodesAddress =
        static_cast<NetifMulticastAddress &>(const_cast<otNetifMulticastAddress &>(kLinkLocalAllNodesMulticastAddress));

    // The tail of multicast address linked list contains the
    // fixed addresses. Search if LinkLocalAll is present
    // in the list and find entry before it.
    //
    //    LinkLocalAll -> RealmLocalAll -> RealmLocalAllMpl.

    SuccessOrExit(error = mMulticastAddresses.Find(linkLocalAllNodesAddress, prev));

    // This method MUST be called after `UnsubscribeAllRoutersMulticast().
    // Verify this by checking the chain at the end of the list only
    // contains three entries and not the five fixed addresses (check that
    // `prev` entry before `LinkLocalAll` is not `RealmLocalRouters`):
    //
    //    LinkLocalAllRouters -> RealmLocalAllRouters -> LinkLocalAll
    //         -> RealmLocalAll -> RealmLocalAllMpl.

    assert(prev != static_cast<NetifMulticastAddress *>(
                       const_cast<otNetifMulticastAddress *>(&kRealmLocalAllRoutersMulticastAddress)));

    if (prev == NULL)
    {
        mMulticastAddresses.Clear();
    }
    else
    {
        prev->SetNext(NULL);
    }

    Get<Notifier>().Signal(OT_CHANGED_IP6_MULTICAST_UNSUBSCRIBED);

    VerifyOrExit(mAddressCallback != NULL);

    for (const NetifMulticastAddress *entry = &linkLocalAllNodesAddress; entry; entry = entry->GetNext())
    {
        mAddressCallback(&entry->GetAddress(), kMulticastPrefixLength, /* IsAdded */ false, mAddressCallbackContext);
    }

exit:
    return error;
}

otError Netif::SubscribeAllRoutersMulticast(void)
{
    otError                error = OT_ERROR_NONE;
    NetifMulticastAddress *prev;
    NetifMulticastAddress &linkLocalAllRoutersAddress = static_cast<NetifMulticastAddress &>(
        const_cast<otNetifMulticastAddress &>(kLinkLocalAllRoutersMulticastAddress));
    NetifMulticastAddress &linkLocalAllNodesAddress =
        static_cast<NetifMulticastAddress &>(const_cast<otNetifMulticastAddress &>(kLinkLocalAllNodesMulticastAddress));
    NetifMulticastAddress &realmLocalAllRoutersAddress = static_cast<NetifMulticastAddress &>(
        const_cast<otNetifMulticastAddress &>(kRealmLocalAllRoutersMulticastAddress));

    error = mMulticastAddresses.Find(linkLocalAllNodesAddress, prev);

    // This method MUST be called after `SubscribeAllNodesMulticast()`
    // Ensure that the `LinkLocalAll` was found on the list.

    assert(error == OT_ERROR_NONE);

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

    VerifyOrExit(prev != &realmLocalAllRoutersAddress, error = OT_ERROR_ALREADY);

    if (prev == NULL)
    {
        mMulticastAddresses.SetHead(&linkLocalAllRoutersAddress);
    }
    else
    {
        prev->SetNext(&linkLocalAllRoutersAddress);
    }

    Get<Notifier>().Signal(OT_CHANGED_IP6_MULTICAST_SUBSCRIBED);

    VerifyOrExit(mAddressCallback != NULL);

    for (const NetifMulticastAddress *entry = &linkLocalAllRoutersAddress; entry != &linkLocalAllNodesAddress;
         entry                              = entry->GetNext())
    {
        mAddressCallback(&entry->GetAddress(), kMulticastPrefixLength, /* IsAdded */ true, mAddressCallbackContext);
    }

exit:
    return error;
}

otError Netif::UnsubscribeAllRoutersMulticast(void)
{
    otError                error;
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

    SuccessOrExit(error = mMulticastAddresses.Find(linkLocalAllRoutersAddress, prev));

    if (prev == NULL)
    {
        mMulticastAddresses.SetHead(&linkLocalAllNodesAddress);
    }
    else
    {
        prev->SetNext(&linkLocalAllNodesAddress);
    }

    Get<Notifier>().Signal(OT_CHANGED_IP6_MULTICAST_UNSUBSCRIBED);

    VerifyOrExit(mAddressCallback != NULL);

    for (const NetifMulticastAddress *entry = &linkLocalAllRoutersAddress; entry != &linkLocalAllNodesAddress;
         entry                              = entry->GetNext())
    {
        mAddressCallback(&entry->GetAddress(), kMulticastPrefixLength, /* IsAdded */ false, mAddressCallbackContext);
    }

exit:
    return error;
}

otError Netif::SubscribeMulticast(NetifMulticastAddress &aAddress)
{
    otError error;

    SuccessOrExit(error = mMulticastAddresses.Add(aAddress));

    Get<Notifier>().Signal(OT_CHANGED_IP6_MULTICAST_SUBSCRIBED);

    VerifyOrExit(mAddressCallback != NULL);
    mAddressCallback(&aAddress.mAddress, kMulticastPrefixLength, /* IsAdded */ true, mAddressCallbackContext);

exit:
    return error;
}

otError Netif::UnsubscribeMulticast(const NetifMulticastAddress &aAddress)
{
    otError error;

    SuccessOrExit(error = mMulticastAddresses.Remove(aAddress));

    Get<Notifier>().Signal(OT_CHANGED_IP6_MULTICAST_UNSUBSCRIBED);

    VerifyOrExit(mAddressCallback != NULL);
    mAddressCallback(&aAddress.mAddress, kMulticastPrefixLength, /* IsAdded */ false, mAddressCallbackContext);

exit:
    return error;
}

otError Netif::GetNextExternalMulticast(uint8_t &aIterator, Address &aAddress) const
{
    otError error = OT_ERROR_NOT_FOUND;
    size_t  num   = OT_ARRAY_LENGTH(mExtMulticastAddresses);

    VerifyOrExit(aIterator < num);

    for (uint8_t i = aIterator; i < num; i++)
    {
        const NetifMulticastAddress &entry = mExtMulticastAddresses[i];

        if (entry.IsInUse())
        {
            aAddress  = entry.GetAddress();
            aIterator = i + 1;
            ExitNow(error = OT_ERROR_NONE);
        }
    }

exit:
    return error;
}

otError Netif::SubscribeExternalMulticast(const Address &aAddress)
{
    otError                error = OT_ERROR_NONE;
    NetifMulticastAddress *entry;
    NetifMulticastAddress &linkLocalAllRoutersAddress = static_cast<NetifMulticastAddress &>(
        const_cast<otNetifMulticastAddress &>(kLinkLocalAllRoutersMulticastAddress));

    // Check that the address is not one of the fixed addresses:
    // LinkLocalAllRouters -> RealmLocalAllRouters -> LinkLocalAllNodes
    // -> RealmLocalAllNodes -> RealmLocalAllMpl.

    for (const NetifMulticastAddress *cur = &linkLocalAllRoutersAddress; cur; cur = cur->GetNext())
    {
        VerifyOrExit(cur->GetAddress() != aAddress, error = OT_ERROR_INVALID_ARGS);
    }

    VerifyOrExit(!IsMulticastSubscribed(aAddress), error = OT_ERROR_ALREADY);

    for (entry = &mExtMulticastAddresses[0]; entry < OT_ARRAY_END(mExtMulticastAddresses); entry++)
    {
        if (!entry->IsInUse())
        {
            entry->mAddress = aAddress;
            mMulticastAddresses.Push(*entry);
            Get<Notifier>().Signal(OT_CHANGED_IP6_MULTICAST_SUBSCRIBED);
            ExitNow();
        }
    }

    error = OT_ERROR_NO_BUFS;

exit:
    return error;
}

otError Netif::UnsubscribeExternalMulticast(const Address &aAddress)
{
    otError                error = OT_ERROR_NONE;
    NetifMulticastAddress *entry;
    NetifMulticastAddress *last = NULL;

    for (entry = mMulticastAddresses.GetHead(); entry; entry = entry->GetNext())
    {
        if (entry->GetAddress() == aAddress)
        {
            VerifyOrExit((entry >= &mExtMulticastAddresses[0]) && (entry < OT_ARRAY_END(mExtMulticastAddresses)),
                         error = OT_ERROR_INVALID_ARGS);

            if (last)
            {
                mMulticastAddresses.PopAfter(*last);
            }
            else
            {
                mMulticastAddresses.Pop();
            }

            break;
        }

        last = entry;
    }

    VerifyOrExit(entry != NULL, error = OT_ERROR_NOT_FOUND);

    entry->MarkAsNotInUse();

    Get<Notifier>().Signal(OT_CHANGED_IP6_MULTICAST_UNSUBSCRIBED);

exit:
    return error;
}

void Netif::UnsubscribeAllExternalMulticastAddresses(void)
{
    for (NetifMulticastAddress *entry = &mExtMulticastAddresses[0]; entry < OT_ARRAY_END(mExtMulticastAddresses);
         entry++)
    {
        if (entry->IsInUse())
        {
            UnsubscribeExternalMulticast(entry->GetAddress());
        }
    }
}

void Netif::SetAddressCallback(otIp6AddressCallback aCallback, void *aCallbackContext)
{
    mAddressCallback        = aCallback;
    mAddressCallbackContext = aCallbackContext;
}

otError Netif::AddUnicastAddress(NetifUnicastAddress &aAddress)
{
    otError error;

    SuccessOrExit(error = mUnicastAddresses.Add(aAddress));

    Get<Notifier>().Signal(aAddress.mRloc ? OT_CHANGED_THREAD_RLOC_ADDED : OT_CHANGED_IP6_ADDRESS_ADDED);

    VerifyOrExit(mAddressCallback != NULL);
    mAddressCallback(&aAddress.mAddress, aAddress.mPrefixLength, /* IsAdded */ true, mAddressCallbackContext);

exit:
    return error;
}

otError Netif::RemoveUnicastAddress(const NetifUnicastAddress &aAddress)
{
    otError error;

    SuccessOrExit(error = mUnicastAddresses.Remove(aAddress));

    Get<Notifier>().Signal(aAddress.mRloc ? OT_CHANGED_THREAD_RLOC_REMOVED : OT_CHANGED_IP6_ADDRESS_REMOVED);

    VerifyOrExit(mAddressCallback != NULL);
    mAddressCallback(&aAddress.mAddress, aAddress.mPrefixLength, /* IsAdded */ false, mAddressCallbackContext);

exit:
    return error;
}

otError Netif::AddExternalUnicastAddress(const NetifUnicastAddress &aAddress)
{
    otError              error = OT_ERROR_NONE;
    NetifUnicastAddress *entry;

    VerifyOrExit(!aAddress.GetAddress().IsLinkLocal(), error = OT_ERROR_INVALID_ARGS);

    for (entry = mUnicastAddresses.GetHead(); entry; entry = entry->GetNext())
    {
        if (entry->GetAddress() == aAddress.GetAddress())
        {
            VerifyOrExit((entry >= &mExtUnicastAddresses[0]) && (entry < OT_ARRAY_END(mExtUnicastAddresses)),
                         error = OT_ERROR_INVALID_ARGS);

            entry->mPrefixLength = aAddress.mPrefixLength;
            entry->mPreferred    = aAddress.mPreferred;
            entry->mValid        = aAddress.mValid;
            ExitNow();
        }
    }

    for (entry = &mExtUnicastAddresses[0]; entry < OT_ARRAY_END(mExtUnicastAddresses); entry++)
    {
        if (!entry->IsInUse())
        {
            *entry = aAddress;
            mUnicastAddresses.Push(*entry);
            Get<Notifier>().Signal(OT_CHANGED_IP6_ADDRESS_ADDED);
            ExitNow();
        }
    }

    error = OT_ERROR_NO_BUFS;

exit:
    return error;
}

otError Netif::RemoveExternalUnicastAddress(const Address &aAddress)
{
    otError              error = OT_ERROR_NONE;
    NetifUnicastAddress *entry;
    NetifUnicastAddress *last = NULL;

    for (entry = mUnicastAddresses.GetHead(); entry; entry = entry->GetNext())
    {
        if (entry->GetAddress() == aAddress)
        {
            VerifyOrExit((entry >= &mExtUnicastAddresses[0]) && (entry < OT_ARRAY_END(mExtUnicastAddresses)),
                         error = OT_ERROR_INVALID_ARGS);

            if (last)
            {
                mUnicastAddresses.PopAfter(*last);
            }
            else
            {
                mUnicastAddresses.Pop();
            }

            break;
        }

        last = entry;
    }

    VerifyOrExit(entry != NULL, error = OT_ERROR_NOT_FOUND);

    entry->MarkAsNotInUse();

    Get<Notifier>().Signal(OT_CHANGED_IP6_ADDRESS_REMOVED);

exit:
    return error;
}

void Netif::RemoveAllExternalUnicastAddresses(void)
{
    for (NetifUnicastAddress *entry = &mExtUnicastAddresses[0]; entry < OT_ARRAY_END(mExtUnicastAddresses); entry++)
    {
        if (entry->IsInUse())
        {
            RemoveExternalUnicastAddress(entry->GetAddress());
        }
    }
}

bool Netif::IsUnicastAddress(const Address &aAddress) const
{
    bool rval = false;

    for (const NetifUnicastAddress *cur = mUnicastAddresses.GetHead(); cur; cur = cur->GetNext())
    {
        if (cur->GetAddress() == aAddress)
        {
            ExitNow(rval = true);
        }
    }

exit:
    return rval;
}

} // namespace Ip6
} // namespace ot
