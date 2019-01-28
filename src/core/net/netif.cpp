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
#include "common/message.hpp"
#include "common/owner-locator.hpp"
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

Netif::Netif(Instance &aInstance, int8_t aInterfaceId)
    : InstanceLocator(aInstance)
    , mUnicastAddresses(NULL)
    , mMulticastAddresses(NULL)
    , mInterfaceId(aInterfaceId)
    , mMulticastPromiscuous(false)
    , mNext(NULL)
    , mAddressCallback(NULL)
    , mAddressCallbackContext(NULL)
{
    for (size_t i = 0; i < OT_ARRAY_LENGTH(mExtUnicastAddresses); i++)
    {
        // To mark the address as unused/available, set the `mNext` to point back to itself.
        mExtUnicastAddresses[i].mNext = &mExtUnicastAddresses[i];
    }

    for (size_t i = 0; i < OT_ARRAY_LENGTH(mExtMulticastAddresses); i++)
    {
        // To mark the address as unused/available, set the `mNext` to point back to itself.
        mExtMulticastAddresses[i].mNext = &mExtMulticastAddresses[i];
    }
}

bool Netif::IsMulticastSubscribed(const Address &aAddress) const
{
    bool rval = false;

    for (NetifMulticastAddress *cur = mMulticastAddresses; cur; cur = cur->GetNext())
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
    assert(mMulticastAddresses == NULL);

    mMulticastAddresses = static_cast<NetifMulticastAddress *>(
        const_cast<otNetifMulticastAddress *>(&kLinkLocalAllNodesMulticastAddress));

    if (mAddressCallback != NULL)
    {
        for (const otNetifMulticastAddress *entry = &kLinkLocalAllNodesMulticastAddress; entry != NULL;
             entry                                = entry->mNext)
        {
            mAddressCallback(&entry->mAddress, kMulticastPrefixLength, true, mAddressCallbackContext);
        }
    }

    GetNotifier().Signal(OT_CHANGED_IP6_MULTICAST_SUBSRCRIBED);
}

void Netif::UnsubscribeAllNodesMulticast(void)
{
    assert(mMulticastAddresses == NULL || mMulticastAddresses == &kLinkLocalAllNodesMulticastAddress);

    mMulticastAddresses = NULL;

    if (mAddressCallback != NULL)
    {
        for (const otNetifMulticastAddress *entry = &kLinkLocalAllNodesMulticastAddress; entry != NULL;
             entry                                = entry->mNext)
        {
            mAddressCallback(&entry->mAddress, kMulticastPrefixLength, false, mAddressCallbackContext);
        }
    }

    GetNotifier().Signal(OT_CHANGED_IP6_MULTICAST_UNSUBSRCRIBED);
}

otError Netif::SubscribeAllRoutersMulticast(void)
{
    otError error = OT_ERROR_NONE;

    if (mMulticastAddresses == &kLinkLocalAllNodesMulticastAddress)
    {
        mMulticastAddresses = static_cast<NetifMulticastAddress *>(
            const_cast<otNetifMulticastAddress *>(&kLinkLocalAllRoutersMulticastAddress));
    }
    else
    {
        for (NetifMulticastAddress *cur = mMulticastAddresses; cur; cur = cur->GetNext())
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

    GetNotifier().Signal(OT_CHANGED_IP6_MULTICAST_SUBSRCRIBED);

exit:
    return error;
}

otError Netif::UnsubscribeAllRoutersMulticast(void)
{
    otError error = OT_ERROR_NONE;

    if (mMulticastAddresses == &kLinkLocalAllRoutersMulticastAddress)
    {
        mMulticastAddresses = static_cast<NetifMulticastAddress *>(
            const_cast<otNetifMulticastAddress *>(&kLinkLocalAllNodesMulticastAddress));
        ExitNow();
    }

    for (NetifMulticastAddress *cur = mMulticastAddresses; cur; cur = cur->GetNext())
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

        GetNotifier().Signal(OT_CHANGED_IP6_MULTICAST_UNSUBSRCRIBED);
    }

    return error;
}

otError Netif::SubscribeMulticast(NetifMulticastAddress &aAddress)
{
    otError error = OT_ERROR_NONE;

    for (NetifMulticastAddress *cur = mMulticastAddresses; cur; cur = cur->GetNext())
    {
        if (cur == &aAddress)
        {
            ExitNow(error = OT_ERROR_ALREADY);
        }
    }

    aAddress.mNext      = mMulticastAddresses;
    mMulticastAddresses = &aAddress;

    if (mAddressCallback != NULL)
    {
        mAddressCallback(&aAddress.mAddress, kMulticastPrefixLength, true, mAddressCallbackContext);
    }

    GetNotifier().Signal(OT_CHANGED_IP6_MULTICAST_SUBSRCRIBED);

exit:
    return error;
}

otError Netif::UnsubscribeMulticast(const NetifMulticastAddress &aAddress)
{
    otError error = OT_ERROR_NONE;

    if (mMulticastAddresses == &aAddress)
    {
        mMulticastAddresses = mMulticastAddresses->GetNext();
        ExitNow();
    }
    else if (mMulticastAddresses != NULL)
    {
        for (NetifMulticastAddress *cur = mMulticastAddresses; cur->GetNext(); cur = cur->GetNext())
        {
            if (cur->mNext == &aAddress)
            {
                cur->mNext = aAddress.mNext;
                ExitNow();
            }
        }
    }

    ExitNow(error = OT_ERROR_NOT_FOUND);

exit:

    if (error != OT_ERROR_NOT_FOUND)
    {
        if (mAddressCallback != NULL)
        {
            mAddressCallback(&aAddress.mAddress, kMulticastPrefixLength, false, mAddressCallbackContext);
        }

        GetNotifier().Signal(OT_CHANGED_IP6_MULTICAST_UNSUBSRCRIBED);
    }

    return error;
}

otError Netif::GetNextExternalMulticast(uint8_t &aIterator, Address &aAddress)
{
    otError                error = OT_ERROR_NOT_FOUND;
    size_t                 num   = OT_ARRAY_LENGTH(mExtMulticastAddresses);
    NetifMulticastAddress *entry;

    VerifyOrExit(aIterator < num);

    // Find an available entry in the `mExtMulticastAddresses` array.
    for (uint8_t i = aIterator; i < num; i++)
    {
        entry = &mExtMulticastAddresses[i];

        // In an unused/available entry, `mNext` points back to the entry itself.
        if (entry->mNext != entry)
        {
            aAddress  = entry->GetAddress();
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
    size_t                 num = OT_ARRAY_LENGTH(mExtMulticastAddresses);

    VerifyOrExit(mMulticastAddresses != NULL, error = OT_ERROR_INVALID_STATE);

    if (IsMulticastSubscribed(aAddress))
    {
        ExitNow(error = OT_ERROR_ALREADY);
    }

    // Find an available entry in the `mExtMulticastAddresses` array.
    for (entry = &mExtMulticastAddresses[0]; num > 0; num--, entry++)
    {
        // In an unused/available entry, `mNext` points back to the entry itself.
        if (entry->mNext == entry)
        {
            break;
        }
    }

    VerifyOrExit(num > 0, error = OT_ERROR_NO_BUFS);

    // Copy the address into the available entry and add it to linked-list.
    entry->mAddress     = aAddress;
    entry->mNext        = mMulticastAddresses;
    mMulticastAddresses = entry;
    GetNotifier().Signal(OT_CHANGED_IP6_MULTICAST_SUBSRCRIBED);

exit:
    return error;
}

otError Netif::UnsubscribeExternalMulticast(const Address &aAddress)
{
    otError                error = OT_ERROR_NONE;
    NetifMulticastAddress *entry;
    NetifMulticastAddress *last = NULL;
    size_t                 num  = OT_ARRAY_LENGTH(mExtMulticastAddresses);

    for (entry = mMulticastAddresses; entry; entry = entry->GetNext())
    {
        if (entry->GetAddress() == aAddress)
        {
            VerifyOrExit((entry >= &mExtMulticastAddresses[0]) && (entry < &mExtMulticastAddresses[num]),
                         error = OT_ERROR_INVALID_ARGS);

            if (last)
            {
                last->mNext = entry->GetNext();
            }
            else
            {
                mMulticastAddresses = entry->GetNext();
            }

            break;
        }

        last = entry;
    }

    VerifyOrExit(entry != NULL, error = OT_ERROR_NOT_FOUND);

    // To mark the address entry as unused/available, set the `mNext` pointer back to the entry itself.
    entry->mNext = entry;

    GetNotifier().Signal(OT_CHANGED_IP6_MULTICAST_UNSUBSRCRIBED);

exit:
    return error;
}

void Netif::UnsubscribeAllExternalMulticastAddresses(void)
{
    size_t num = OT_ARRAY_LENGTH(mExtMulticastAddresses);

    for (NetifMulticastAddress *entry = &mExtMulticastAddresses[0]; num > 0; num--, entry++)
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
    otError error = OT_ERROR_NONE;

    for (NetifUnicastAddress *cur = mUnicastAddresses; cur; cur = cur->GetNext())
    {
        if (cur == &aAddress)
        {
            ExitNow(error = OT_ERROR_ALREADY);
        }
    }

    aAddress.mNext    = mUnicastAddresses;
    mUnicastAddresses = &aAddress;

    if (mAddressCallback != NULL)
    {
        mAddressCallback(&aAddress.mAddress, aAddress.mPrefixLength, true, mAddressCallbackContext);
    }

    GetNotifier().Signal(aAddress.mRloc ? OT_CHANGED_THREAD_RLOC_ADDED : OT_CHANGED_IP6_ADDRESS_ADDED);

exit:
    return error;
}

otError Netif::RemoveUnicastAddress(const NetifUnicastAddress &aAddress)
{
    otError error = OT_ERROR_NONE;

    if (mUnicastAddresses == &aAddress)
    {
        mUnicastAddresses = mUnicastAddresses->GetNext();
        ExitNow();
    }
    else if (mUnicastAddresses != NULL)
    {
        for (NetifUnicastAddress *cur = mUnicastAddresses; cur->GetNext(); cur = cur->GetNext())
        {
            if (cur->mNext == &aAddress)
            {
                cur->mNext = aAddress.mNext;
                ExitNow();
            }
        }
    }

    ExitNow(error = OT_ERROR_NOT_FOUND);

exit:

    if (error != OT_ERROR_NOT_FOUND)
    {
        if (mAddressCallback != NULL)
        {
            mAddressCallback(&aAddress.mAddress, aAddress.mPrefixLength, false, mAddressCallbackContext);
        }

        GetNotifier().Signal(aAddress.mRloc ? OT_CHANGED_THREAD_RLOC_REMOVED : OT_CHANGED_IP6_ADDRESS_REMOVED);
    }

    return error;
}

otError Netif::AddExternalUnicastAddress(const NetifUnicastAddress &aAddress)
{
    otError              error = OT_ERROR_NONE;
    NetifUnicastAddress *entry;
    size_t               num = OT_ARRAY_LENGTH(mExtUnicastAddresses);

    VerifyOrExit(!aAddress.GetAddress().IsLinkLocal(), error = OT_ERROR_INVALID_ARGS);

    for (entry = mUnicastAddresses; entry; entry = entry->GetNext())
    {
        if (entry->GetAddress() == aAddress.GetAddress())
        {
            VerifyOrExit((entry >= &mExtUnicastAddresses[0]) && (entry < &mExtUnicastAddresses[num]),
                         error = OT_ERROR_INVALID_ARGS);

            entry->mPrefixLength = aAddress.mPrefixLength;
            entry->mPreferred    = aAddress.mPreferred;
            entry->mValid        = aAddress.mValid;
            ExitNow();
        }
    }

    // Find an available entry in the `mExtUnicastAddresses` array.
    for (entry = &mExtUnicastAddresses[0]; num > 0; num--, entry++)
    {
        // In an unused/available entry, `mNext` points back to the entry itself.
        if (entry->mNext == entry)
        {
            break;
        }
    }

    VerifyOrExit(num > 0, error = OT_ERROR_NO_BUFS);

    // Copy the new address into the available entry and insert it in linked-list.
    *entry            = aAddress;
    entry->mNext      = mUnicastAddresses;
    mUnicastAddresses = entry;

    GetNotifier().Signal(OT_CHANGED_IP6_ADDRESS_ADDED);

exit:
    return error;
}

otError Netif::RemoveExternalUnicastAddress(const Address &aAddress)
{
    otError              error = OT_ERROR_NONE;
    NetifUnicastAddress *entry;
    NetifUnicastAddress *last = NULL;
    size_t               num  = OT_ARRAY_LENGTH(mExtUnicastAddresses);

    for (entry = mUnicastAddresses; entry; entry = entry->GetNext())
    {
        if (entry->GetAddress() == aAddress)
        {
            VerifyOrExit((entry >= &mExtUnicastAddresses[0]) && (entry < &mExtUnicastAddresses[num]),
                         error = OT_ERROR_INVALID_ARGS);

            if (last)
            {
                last->mNext = entry->mNext;
            }
            else
            {
                mUnicastAddresses = entry->GetNext();
            }

            break;
        }

        last = entry;
    }

    VerifyOrExit(entry != NULL, error = OT_ERROR_NOT_FOUND);

    // To mark the address entry as unused/available, set the `mNext` pointer back to the entry itself.
    entry->mNext = entry;

    GetNotifier().Signal(OT_CHANGED_IP6_ADDRESS_REMOVED);

exit:
    return error;
}

void Netif::RemoveAllExternalUnicastAddresses(void)
{
    size_t num = OT_ARRAY_LENGTH(mExtUnicastAddresses);

    for (NetifUnicastAddress *entry = &mExtUnicastAddresses[0]; num > 0; num--, entry++)
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

    for (const NetifUnicastAddress *cur = mUnicastAddresses; cur; cur = cur->GetNext())
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
