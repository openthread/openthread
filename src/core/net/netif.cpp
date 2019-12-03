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

// "ff03:02"
const otNetifMulticastAddress Netif::kRealmLocalAllRoutersMulticastAddress = {
    {{{0xff, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02}}},
    &Netif::kLinkLocalAllNodesMulticastAddress};

// "ff02:02"
const otNetifMulticastAddress Netif::kLinkLocalAllRoutersMulticastAddress = {
    {{{0xff, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02}}},
    &Netif::kRealmLocalAllRoutersMulticastAddress};

Netif::Netif(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mUnicastAddresses()
    , mMulticastAddresses()
    , mMulticastPromiscuous(false)
    , mNext(NULL)
    , mAddressCallback(NULL)
    , mAddressCallbackContext(NULL)
{
    for (NetifUnicastAddress *entry = &mExtUnicastAddresses[0]; entry < OT_ARRAY_END(mExtUnicastAddresses); entry++)
    {
        // To mark the address as unused/available, set the `mNext` to point back to itself.
        entry->mNext = entry;
    }

    for (NetifMulticastAddress *entry = &mExtMulticastAddresses[0]; entry < OT_ARRAY_END(mExtMulticastAddresses);
         entry++)
    {
        // To mark the address as unused/available, set the `mNext` to point back to itself.
        entry->mNext = entry;
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

void Netif::SubscribeAllNodesMulticast(void)
{
    assert(mMulticastAddresses.IsEmpty());

    mMulticastAddresses.SetHead(static_cast<NetifMulticastAddress *>(
        const_cast<otNetifMulticastAddress *>(&kLinkLocalAllNodesMulticastAddress)));

    if (mAddressCallback != NULL)
    {
        for (const otNetifMulticastAddress *entry = &kLinkLocalAllNodesMulticastAddress; entry != NULL;
             entry                                = entry->mNext)
        {
            mAddressCallback(&entry->mAddress, kMulticastPrefixLength, true, mAddressCallbackContext);
        }
    }

    Get<Notifier>().Signal(OT_CHANGED_IP6_MULTICAST_SUBSCRIBED);
}

void Netif::UnsubscribeAllNodesMulticast(void)
{
    assert(mMulticastAddresses.IsEmpty() || mMulticastAddresses.GetHead() == &kLinkLocalAllNodesMulticastAddress);

    mMulticastAddresses.SetHead(NULL);

    if (mAddressCallback != NULL)
    {
        for (const otNetifMulticastAddress *entry = &kLinkLocalAllNodesMulticastAddress; entry != NULL;
             entry                                = entry->mNext)
        {
            mAddressCallback(&entry->mAddress, kMulticastPrefixLength, false, mAddressCallbackContext);
        }
    }

    Get<Notifier>().Signal(OT_CHANGED_IP6_MULTICAST_UNSUBSCRIBED);
}

otError Netif::SubscribeAllRoutersMulticast(void)
{
    otError error = OT_ERROR_NONE;

    if (mMulticastAddresses.GetHead() == &kLinkLocalAllNodesMulticastAddress)
    {
        mMulticastAddresses.SetHead(static_cast<NetifMulticastAddress *>(
            const_cast<otNetifMulticastAddress *>(&kLinkLocalAllRoutersMulticastAddress)));
    }
    else
    {
        for (NetifMulticastAddress *cur = mMulticastAddresses.GetHead(); cur; cur = cur->GetNext())
        {
            if (cur == &kLinkLocalAllRoutersMulticastAddress)
            {
                ExitNow(error = OT_ERROR_ALREADY);
            }

            if (cur->mNext == &kLinkLocalAllNodesMulticastAddress)
            {
                cur->mNext = &kLinkLocalAllRoutersMulticastAddress;
                break;
            }
        }
    }

    if (mAddressCallback != NULL)
    {
        for (const otNetifMulticastAddress *entry                = &kLinkLocalAllRoutersMulticastAddress;
             entry != &kLinkLocalAllNodesMulticastAddress; entry = entry->mNext)
        {
            mAddressCallback(&entry->mAddress, kMulticastPrefixLength, true, mAddressCallbackContext);
        }
    }

    Get<Notifier>().Signal(OT_CHANGED_IP6_MULTICAST_SUBSCRIBED);

exit:
    return error;
}

otError Netif::UnsubscribeAllRoutersMulticast(void)
{
    otError error = OT_ERROR_NONE;

    if (mMulticastAddresses.GetHead() == &kLinkLocalAllRoutersMulticastAddress)
    {
        mMulticastAddresses.SetHead(static_cast<NetifMulticastAddress *>(
            const_cast<otNetifMulticastAddress *>(&kLinkLocalAllNodesMulticastAddress)));
        ExitNow();
    }

    for (NetifMulticastAddress *cur = mMulticastAddresses.GetHead(); cur; cur = cur->GetNext())
    {
        if (cur->mNext == &kLinkLocalAllRoutersMulticastAddress)
        {
            cur->mNext = &kLinkLocalAllNodesMulticastAddress;
            ExitNow();
        }
    }

    error = OT_ERROR_NOT_FOUND;

exit:

    if (error != OT_ERROR_NOT_FOUND)
    {
        if (mAddressCallback != NULL)
        {
            for (const otNetifMulticastAddress *entry                = &kLinkLocalAllRoutersMulticastAddress;
                 entry != &kLinkLocalAllNodesMulticastAddress; entry = entry->mNext)
            {
                mAddressCallback(&entry->mAddress, kMulticastPrefixLength, false, mAddressCallbackContext);
            }
        }

        Get<Notifier>().Signal(OT_CHANGED_IP6_MULTICAST_UNSUBSCRIBED);
    }

    return error;
}

otError Netif::SubscribeMulticast(NetifMulticastAddress &aAddress)
{
    otError error;

    SuccessOrExit(error = mMulticastAddresses.Add(aAddress));

    if (mAddressCallback != NULL)
    {
        mAddressCallback(&aAddress.mAddress, kMulticastPrefixLength, true, mAddressCallbackContext);
    }

    Get<Notifier>().Signal(OT_CHANGED_IP6_MULTICAST_SUBSCRIBED);

exit:
    return error;
}

otError Netif::UnsubscribeMulticast(const NetifMulticastAddress &aAddress)
{
    otError error;

    SuccessOrExit(error = mMulticastAddresses.Remove(aAddress));

    if (mAddressCallback != NULL)
    {
        mAddressCallback(&aAddress.mAddress, kMulticastPrefixLength, false, mAddressCallbackContext);
    }

    Get<Notifier>().Signal(OT_CHANGED_IP6_MULTICAST_UNSUBSCRIBED);

exit:
    return error;
}

otError Netif::GetNextExternalMulticast(uint8_t &aIterator, Address &aAddress) const
{
    otError error = OT_ERROR_NOT_FOUND;
    size_t  num   = OT_ARRAY_LENGTH(mExtMulticastAddresses);

    VerifyOrExit(aIterator < num);

    // Find an available entry in the `mExtMulticastAddresses` array.
    for (uint8_t i = aIterator; i < num; i++)
    {
        const NetifMulticastAddress &entry = mExtMulticastAddresses[i];

        // In an unused/available entry, `mNext` points back to the entry itself.
        if (entry.mNext != &entry)
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

    VerifyOrExit(!mMulticastAddresses.IsEmpty(), error = OT_ERROR_INVALID_STATE);

    if (IsMulticastSubscribed(aAddress))
    {
        ExitNow(error = OT_ERROR_ALREADY);
    }

    // Find an available entry in the `mExtMulticastAddresses` array.
    for (entry = &mExtMulticastAddresses[0]; entry < OT_ARRAY_END(mExtMulticastAddresses); entry++)
    {
        // In an unused/available entry, `mNext` points back to the entry itself.
        if (entry->mNext == entry)
        {
            break;
        }
    }

    VerifyOrExit(entry < OT_ARRAY_END(mExtMulticastAddresses), error = OT_ERROR_NO_BUFS);

    // Copy the address into the available entry and add it to linked-list.
    entry->mAddress = aAddress;
    mMulticastAddresses.Push(*entry);
    Get<Notifier>().Signal(OT_CHANGED_IP6_MULTICAST_SUBSCRIBED);

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

    // To mark the address entry as unused/available, set the `mNext` pointer back to the entry itself.
    entry->mNext = entry;

    Get<Notifier>().Signal(OT_CHANGED_IP6_MULTICAST_UNSUBSCRIBED);

exit:
    return error;
}

void Netif::UnsubscribeAllExternalMulticastAddresses(void)
{
    for (NetifMulticastAddress *entry = &mExtMulticastAddresses[0]; entry < OT_ARRAY_END(mExtMulticastAddresses);
         entry++)
    {
        // In unused entries, the `mNext` points back to the entry itself.
        if (entry->mNext != entry)
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

    if (mAddressCallback != NULL)
    {
        mAddressCallback(&aAddress.mAddress, aAddress.mPrefixLength, true, mAddressCallbackContext);
    }

    Get<Notifier>().Signal(aAddress.mRloc ? OT_CHANGED_THREAD_RLOC_ADDED : OT_CHANGED_IP6_ADDRESS_ADDED);

exit:
    return error;
}

otError Netif::RemoveUnicastAddress(const NetifUnicastAddress &aAddress)
{
    otError error;

    SuccessOrExit(error = mUnicastAddresses.Remove(aAddress));

    if (mAddressCallback != NULL)
    {
        mAddressCallback(&aAddress.mAddress, aAddress.mPrefixLength, false, mAddressCallbackContext);
    }

    Get<Notifier>().Signal(aAddress.mRloc ? OT_CHANGED_THREAD_RLOC_REMOVED : OT_CHANGED_IP6_ADDRESS_REMOVED);

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

    // Find an available entry in the `mExtUnicastAddresses` array.
    for (entry = &mExtUnicastAddresses[0]; entry < OT_ARRAY_END(mExtUnicastAddresses); entry++)
    {
        // In an unused/available entry, `mNext` points back to the entry itself.
        if (entry->mNext == entry)
        {
            break;
        }
    }

    VerifyOrExit(entry < OT_ARRAY_END(mExtUnicastAddresses), error = OT_ERROR_NO_BUFS);

    // Copy the new address into the available entry and insert it in linked-list.
    *entry = aAddress;
    mUnicastAddresses.Push(*entry);

    Get<Notifier>().Signal(OT_CHANGED_IP6_ADDRESS_ADDED);

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

    // To mark the address entry as unused/available, set the `mNext` pointer back to the entry itself.
    entry->mNext = entry;

    Get<Notifier>().Signal(OT_CHANGED_IP6_ADDRESS_REMOVED);

exit:
    return error;
}

void Netif::RemoveAllExternalUnicastAddresses(void)
{
    for (NetifUnicastAddress *entry = &mExtUnicastAddresses[0]; entry < OT_ARRAY_END(mExtUnicastAddresses); entry++)
    {
        // In unused entries, the `mNext` points back to the entry itself.
        if (entry->mNext != entry)
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
