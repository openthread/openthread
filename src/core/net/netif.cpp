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
#ifdef OPENTHREAD_CONFIG_FILE
#include OPENTHREAD_CONFIG_FILE
#else
#include <openthread-config.h>
#endif

#include "netif.hpp"

#include "common/code_utils.hpp"
#include "common/debug.hpp"
#include "common/message.hpp"
#include "net/ip6.hpp"

namespace ot {
namespace Ip6 {

Netif::Netif(Ip6 &aIp6, int8_t aInterfaceId):
    mIp6(aIp6),
    mCallbacks(NULL),
    mUnicastAddresses(NULL),
    mMulticastAddresses(NULL),
    mInterfaceId(aInterfaceId),
    mAllRoutersSubscribed(false),
    mMulticastPromiscuous(false),
    mStateChangedTask(aIp6.mTaskletScheduler, &Netif::HandleStateChangedTask, this),
    mNext(NULL),
    mStateChangedFlags(0)
{
    for (size_t i = 0; i < sizeof(mExtUnicastAddresses) / sizeof(mExtUnicastAddresses[0]); i++)
    {
        // To mark the address as unused/available, set the `mNext` to point back to itself.
        mExtUnicastAddresses[i].mNext = &mExtUnicastAddresses[i];
    }

    for (size_t i = 0; i < sizeof(mExtMulticastAddresses) / sizeof(mExtMulticastAddresses[0]); i++)
    {
        // To mark the address as unused/available, set the `mNext` to point back to itself.
        mExtMulticastAddresses[i].mNext = &mExtMulticastAddresses[i];
    }
}

otError Netif::RegisterCallback(NetifCallback &aCallback)
{
    otError error = OT_ERROR_NONE;

    for (NetifCallback *cur = mCallbacks; cur; cur = cur->mNext)
    {
        if (cur == &aCallback)
        {
            ExitNow(error = OT_ERROR_ALREADY);
        }
    }

    aCallback.mNext = mCallbacks;
    mCallbacks = &aCallback;

exit:
    return error;
}

otError Netif::RemoveCallback(NetifCallback &aCallback)
{
    otError error = OT_ERROR_ALREADY;
    NetifCallback *prev = NULL;

    for (NetifCallback *cur = mCallbacks; cur; cur = cur->mNext)
    {
        if (cur == &aCallback)
        {
            if (prev)
            {
                prev->mNext = cur->mNext;
            }
            else
            {
                mCallbacks = mCallbacks->mNext;
            }

            cur->mNext = NULL;
            error = OT_ERROR_NONE;
            break;
        }

        prev = cur;
    }

    return error;
}

bool Netif::IsMulticastSubscribed(const Address &aAddress) const
{
    bool rval = false;

    if (aAddress.IsLinkLocalAllNodesMulticast() || aAddress.IsRealmLocalAllNodesMulticast() ||
        aAddress.IsRealmLocalAllMplForwarders())
    {
        ExitNow(rval = true);
    }
    else if (aAddress.IsLinkLocalAllRoutersMulticast() || aAddress.IsRealmLocalAllRoutersMulticast())
    {
        ExitNow(rval = mAllRoutersSubscribed);
    }

    for (NetifMulticastAddress *cur = mMulticastAddresses; cur; cur = cur->GetNext())
    {
        if (memcmp(&cur->mAddress, &aAddress, sizeof(cur->mAddress)) == 0)
        {
            ExitNow(rval = true);
        }
    }

exit:
    return rval;
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

    aAddress.mNext = mMulticastAddresses;
    mMulticastAddresses = &aAddress;

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
    return error;
}

otError Netif::SubscribeExternalMulticast(const Address &aAddress)
{
    otError error = OT_ERROR_NONE;
    NetifMulticastAddress *entry;
    size_t num = sizeof(mExtMulticastAddresses) / sizeof(mExtMulticastAddresses[0]);

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
    entry->mAddress = aAddress;
    entry->mNext = mMulticastAddresses;
    mMulticastAddresses = entry;

exit:
    return error;
}

otError Netif::UnsubscribeExternalMulticast(const Address &aAddress)
{
    otError error = OT_ERROR_NONE;
    NetifMulticastAddress *entry;
    NetifMulticastAddress *last = NULL;
    size_t num = sizeof(mExtMulticastAddresses) / sizeof(mExtMulticastAddresses[0]);

    for (entry = mMulticastAddresses; entry; entry = entry->GetNext())
    {
        if (memcmp(&entry->mAddress, &aAddress, sizeof(otIp6Address)) == 0)
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

exit:
    return error;
}

void Netif::UnsubscribeAllExternalMulticastAddresses(void)
{
    size_t num = sizeof(mExtMulticastAddresses) / sizeof(mExtMulticastAddresses[0]);

    for (NetifMulticastAddress *entry = &mExtMulticastAddresses[0]; num > 0; num--, entry++)
    {
        // In unused entries, the `mNext` points back to the entry itself.
        if (entry->mNext != entry)
        {
            UnsubscribeExternalMulticast(*static_cast<Address *>(&entry->mAddress));
        }
    }
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

    aAddress.mNext = mUnicastAddresses;
    mUnicastAddresses = &aAddress;

    SetStateChangedFlags(aAddress.mRloc ? OT_IP6_RLOC_ADDED : OT_IP6_ADDRESS_ADDED);

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
        SetStateChangedFlags(aAddress.mRloc ? OT_IP6_RLOC_REMOVED : OT_IP6_ADDRESS_REMOVED);
    }

    return error;
}

otError Netif::AddExternalUnicastAddress(const NetifUnicastAddress &aAddress)
{
    otError error = OT_ERROR_NONE;
    NetifUnicastAddress *entry;
    size_t num = sizeof(mExtUnicastAddresses) / sizeof(mExtUnicastAddresses[0]);

    for (entry = mUnicastAddresses; entry; entry = entry->GetNext())
    {
        if (memcmp(&entry->mAddress, &aAddress.mAddress, sizeof(otIp6Address)) == 0)
        {
            VerifyOrExit((entry >= &mExtUnicastAddresses[0]) && (entry < &mExtUnicastAddresses[num]),
                         error = OT_ERROR_INVALID_ARGS);

            entry->mPrefixLength = aAddress.mPrefixLength;
            entry->mPreferred = aAddress.mPreferred;
            entry->mValid = aAddress.mValid;
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
    *entry = aAddress;
    entry->mNext = mUnicastAddresses;
    mUnicastAddresses = entry;

    SetStateChangedFlags(OT_IP6_ADDRESS_ADDED);

exit:
    return error;
}

otError Netif::RemoveExternalUnicastAddress(const Address &aAddress)
{
    otError error = OT_ERROR_NONE;
    NetifUnicastAddress *entry;
    NetifUnicastAddress *last = NULL;
    size_t num = sizeof(mExtUnicastAddresses) / sizeof(mExtUnicastAddresses[0]);

    for (entry = mUnicastAddresses; entry; entry = entry->GetNext())
    {
        if (memcmp(&entry->mAddress, &aAddress, sizeof(otIp6Address)) == 0)
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

    SetStateChangedFlags(OT_IP6_ADDRESS_REMOVED);

exit:
    return error;
}

void Netif::RemoveAllExternalUnicastAddresses(void)
{
    size_t num = sizeof(mExtUnicastAddresses) / sizeof(mExtUnicastAddresses[0]);

    for (NetifUnicastAddress *entry = &mExtUnicastAddresses[0]; num > 0; num--, entry++)
    {
        // In unused entries, the `mNext` points back to the entry itself.
        if (entry->mNext != entry)
        {
            RemoveExternalUnicastAddress(*static_cast<Address *>(&entry->mAddress));
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

void Netif::SetStateChangedFlags(uint32_t aFlags)
{
    mStateChangedFlags |= aFlags;
    mStateChangedTask.Post();
}

void Netif::HandleStateChangedTask(void *aContext)
{
    static_cast<Netif *>(aContext)->HandleStateChangedTask();
}

void Netif::HandleStateChangedTask(void)
{
    uint32_t flags = mStateChangedFlags;

    mStateChangedFlags = 0;

    for (NetifCallback *callback = mCallbacks; callback; callback = callback->mNext)
    {
        callback->Callback(flags);
    }
}

}  // namespace Ip6
}  // namespace ot
