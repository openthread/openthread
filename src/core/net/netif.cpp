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

#include <common/code_utils.hpp>
#include <common/debug.hpp>
#include <common/message.hpp>
#include <net/ip6.hpp>
#include <net/netif.hpp>

namespace Thread {
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

ThreadError Netif::RegisterCallback(NetifCallback &aCallback)
{
    ThreadError error = kThreadError_None;

    for (NetifCallback *cur = mCallbacks; cur; cur = cur->mNext)
    {
        if (cur == &aCallback)
        {
            ExitNow(error = kThreadError_Already);
        }
    }

    aCallback.mNext = mCallbacks;
    mCallbacks = &aCallback;

exit:
    return error;
}

ThreadError Netif::RemoveCallback(NetifCallback &aCallback)
{
    ThreadError error = kThreadError_Already;
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
            error = kThreadError_None;
            break;
        }

        prev = cur;
    }

    return error;
}

Netif *Netif::GetNext() const
{
    return mNext;
}

Ip6 &Netif::GetIp6(void)
{
    return mIp6;
}

int8_t Netif::GetInterfaceId() const
{
    return mInterfaceId;
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

void Netif::SubscribeAllRoutersMulticast()
{
    mAllRoutersSubscribed = true;
}

void Netif::UnsubscribeAllRoutersMulticast()
{
    mAllRoutersSubscribed = false;
}

const NetifMulticastAddress *Netif::GetMulticastAddresses() const
{
    return mMulticastAddresses;
}

ThreadError Netif::SubscribeMulticast(NetifMulticastAddress &aAddress)
{
    ThreadError error = kThreadError_None;

    for (NetifMulticastAddress *cur = mMulticastAddresses; cur; cur = cur->GetNext())
    {
        if (cur == &aAddress)
        {
            ExitNow(error = kThreadError_Already);
        }
    }

    aAddress.mNext = mMulticastAddresses;
    mMulticastAddresses = &aAddress;

exit:
    return error;
}

ThreadError Netif::UnsubscribeMulticast(const NetifMulticastAddress &aAddress)
{
    ThreadError error = kThreadError_None;

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

    ExitNow(error = kThreadError_Error);

exit:
    return error;
}

ThreadError Netif::SubscribeExternalMulticast(const Address &aAddress)
{
    ThreadError error = kThreadError_None;
    NetifMulticastAddress *entry;
    size_t num = sizeof(mExtMulticastAddresses) / sizeof(mExtMulticastAddresses[0]);

    for (entry = mMulticastAddresses; entry; entry = entry->GetNext())
    {
        if (memcmp(&entry->mAddress, &aAddress, sizeof(otIp6Address)) == 0)
        {
            VerifyOrExit((entry >= &mExtMulticastAddresses[0]) && (entry < &mExtMulticastAddresses[num]),
                         error = kThreadError_InvalidArgs);

            ExitNow(error = kThreadError_Already);
        }
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

    VerifyOrExit(num > 0, error = kThreadError_NoBufs);

    // Copy the address into the available entry and add it to linked-list.
    entry->mAddress = aAddress;
    entry->mNext = mMulticastAddresses;
    mMulticastAddresses = entry;

exit:
    return error;
}

ThreadError Netif::UnsubscribeExternalMulticast(const Address &aAddress)
{
    ThreadError error = kThreadError_None;
    NetifMulticastAddress *entry;
    NetifMulticastAddress *last = NULL;
    size_t num = sizeof(mExtMulticastAddresses) / sizeof(mExtMulticastAddresses[0]);

    for (entry = mMulticastAddresses; entry; entry = entry->GetNext())
    {
        if (memcmp(&entry->mAddress, &aAddress, sizeof(otIp6Address)) == 0)
        {
            VerifyOrExit((entry >= &mExtMulticastAddresses[0]) && (entry < &mExtMulticastAddresses[num]),
                         error = kThreadError_InvalidArgs);

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

    VerifyOrExit(entry != NULL, error = kThreadError_NotFound);

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

bool Netif::IsMulticastPromiscuousEnabled(void)
{
    return mMulticastPromiscuous;
}

void Netif::SetMulticastPromiscuous(bool aEnabled)
{
    mMulticastPromiscuous = aEnabled;
}

const NetifUnicastAddress *Netif::GetUnicastAddresses() const
{
    return mUnicastAddresses;
}

ThreadError Netif::AddUnicastAddress(NetifUnicastAddress &aAddress)
{
    ThreadError error = kThreadError_None;

    for (NetifUnicastAddress *cur = mUnicastAddresses; cur; cur = cur->GetNext())
    {
        if (cur == &aAddress)
        {
            ExitNow(error = kThreadError_Already);
        }
    }

    aAddress.mNext = mUnicastAddresses;
    mUnicastAddresses = &aAddress;

    SetStateChangedFlags(aAddress.mRloc ? OT_IP6_RLOC_ADDED : OT_IP6_ADDRESS_ADDED);

exit:
    return error;
}

ThreadError Netif::RemoveUnicastAddress(const NetifUnicastAddress &aAddress)
{
    ThreadError error = kThreadError_None;

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

    ExitNow(error = kThreadError_NotFound);

exit:

    if (error != kThreadError_NotFound)
    {
        SetStateChangedFlags(aAddress.mRloc ? OT_IP6_RLOC_REMOVED : OT_IP6_ADDRESS_REMOVED);
    }

    return error;
}

ThreadError Netif::AddExternalUnicastAddress(const NetifUnicastAddress &aAddress)
{
    ThreadError error = kThreadError_None;
    NetifUnicastAddress *entry;
    size_t num = sizeof(mExtUnicastAddresses) / sizeof(mExtUnicastAddresses[0]);

    for (entry = mUnicastAddresses; entry; entry = entry->GetNext())
    {
        if (memcmp(&entry->mAddress, &aAddress.mAddress, sizeof(otIp6Address)) == 0)
        {
            VerifyOrExit((entry >= &mExtUnicastAddresses[0]) && (entry < &mExtUnicastAddresses[num]),
                         error = kThreadError_InvalidArgs);

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

    VerifyOrExit(num > 0, error = kThreadError_NoBufs);

    // Copy the new address into the available entry and insert it in linked-list.
    *entry = aAddress;
    entry->mNext = mUnicastAddresses;
    mUnicastAddresses = entry;

    SetStateChangedFlags(OT_IP6_ADDRESS_ADDED);

exit:
    return error;
}

ThreadError Netif::RemoveExternalUnicastAddress(const Address &aAddress)
{
    ThreadError error = kThreadError_None;
    NetifUnicastAddress *entry;
    NetifUnicastAddress *last = NULL;
    size_t num = sizeof(mExtUnicastAddresses) / sizeof(mExtUnicastAddresses[0]);

    for (entry = mUnicastAddresses; entry; entry = entry->GetNext())
    {
        if (memcmp(&entry->mAddress, &aAddress, sizeof(otIp6Address)) == 0)
        {
            VerifyOrExit((entry >= &mExtUnicastAddresses[0]) && (entry < &mExtUnicastAddresses[num]),
                         error = kThreadError_InvalidArgs);

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

    VerifyOrExit(entry != NULL, error = kThreadError_NotFound);

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

bool Netif::IsStateChangedCallbackPending(void)
{
    return mStateChangedFlags != 0;
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
}  // namespace Thread
