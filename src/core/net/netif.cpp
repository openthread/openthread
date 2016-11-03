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
    mMulticastPromiscuousMode(false),
    mStateChangedTask(aIp6.mTaskletScheduler, &Netif::HandleStateChangedTask, this),
    mNext(NULL),
    mStateChangedFlags(0),
    mMaskExtUnicastAddresses(0),
    mMaskExtMulticastAddresses(0)
{
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
    int8_t index = 0;

    for (NetifMulticastAddress *cur = mMulticastAddresses; cur; cur = cur->GetNext())
    {
        if (memcmp(&cur->mAddress, &aAddress, sizeof(otIp6Address)) == 0)
        {
            VerifyOrExit(GetExtMulticastAddressIndex(cur) != -1, error = kThreadError_InvalidArgs);
            ExitNow(error = kThreadError_Already);
        }
    }

    // Make sure we haven't set all the bits in the mask already
    VerifyOrExit(mMaskExtMulticastAddresses != ((1 << OPENTHREAD_CONFIG_MAX_EXT_MULTICAST_IP_ADDRS) - 1),
                 error = kThreadError_NoBufs);

    // Get next available entry index
    while ((mMaskExtMulticastAddresses & (1 << index)) != 0)
    {
        index++;
    }

    assert(index < OPENTHREAD_CONFIG_MAX_EXT_MULTICAST_IP_ADDRS);

    // Increase the count and mask the index
    mMaskExtMulticastAddresses |= 1 << index;

    // Copy the address to the next available dynamic address
    mExtMulticastAddresses[index].mAddress = aAddress;
    mExtMulticastAddresses[index].mNext = mMulticastAddresses;

    mMulticastAddresses = &mExtMulticastAddresses[index];

exit:
    return error;
}

ThreadError Netif::UnsubscribeExternalMulticast(const Address &aAddress)
{
    ThreadError error = kThreadError_None;
    NetifMulticastAddress *last = NULL;
    int8_t aAddressIndexToRemove = -1;

    for (NetifMulticastAddress *cur = mMulticastAddresses; cur; cur = cur->GetNext())
    {
        if (memcmp(&cur->mAddress, &aAddress, sizeof(otIp6Address)) == 0)
        {
            aAddressIndexToRemove = GetExtMulticastAddressIndex(cur);
            VerifyOrExit(aAddressIndexToRemove != -1, error = kThreadError_InvalidArgs);

            if (last)
            {
                last->mNext = cur->GetNext();
            }
            else
            {
                mMulticastAddresses = cur->GetNext();
            }

            break;
        }

        last = cur;
    }

    if (aAddressIndexToRemove != -1)
    {
        mMaskExtMulticastAddresses &= ~(1 << aAddressIndexToRemove);
    }
    else
    {
        error = kThreadError_NotFound;
    }

exit:

    return error;
}

bool Netif::IsMulticastPromiscuousModeEnabled(void)
{
    return mMulticastPromiscuousMode;
}

void Netif::EnableMulticastPromiscuousMode(void)
{
    mMulticastPromiscuousMode = true;
}

void Netif::DisableMulticastPromiscuousMode(void)
{
    mMulticastPromiscuousMode = false;
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

    SetStateChangedFlags(OT_IP6_ADDRESS_ADDED);

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
        SetStateChangedFlags(OT_IP6_ADDRESS_REMOVED);
    }

    return error;
}

ThreadError Netif::AddExternalUnicastAddress(const NetifUnicastAddress &aAddress)
{
    ThreadError error = kThreadError_None;
    int8_t index = 0;

    for (NetifUnicastAddress *cur = mUnicastAddresses; cur; cur = cur->GetNext())
    {
        if (memcmp(&cur->mAddress, &aAddress.mAddress, sizeof(otIp6Address)) == 0)
        {
            VerifyOrExit(GetExtUnicastAddressIndex(cur) != -1, error = kThreadError_InvalidArgs);

            cur->mPreferredLifetime = aAddress.mPreferredLifetime;
            cur->mValidLifetime = aAddress.mValidLifetime;
            cur->mPrefixLength = aAddress.mPrefixLength;
            ExitNow();
        }
    }

    // Make sure we haven't set all the bits in the mask already
    VerifyOrExit(mMaskExtUnicastAddresses != ((1 << OPENTHREAD_CONFIG_MAX_EXT_IP_ADDRS) - 1),
                 error = kThreadError_NoBufs);

    // Get next available entry index
    while ((mMaskExtUnicastAddresses & (1 << index)) != 0)
    {
        index++;
    }

    assert(index < OPENTHREAD_CONFIG_MAX_EXT_IP_ADDRS);

    // Increase the count and mask the index
    mMaskExtUnicastAddresses |= 1 << index;

    // Copy the address to the next available dynamic address
    mExtUnicastAddresses[index] = aAddress;
    mExtUnicastAddresses[index].mNext = mUnicastAddresses;

    mUnicastAddresses = &mExtUnicastAddresses[index];

    SetStateChangedFlags(OT_IP6_ADDRESS_ADDED);

exit:
    return error;
}

ThreadError Netif::RemoveExternalUnicastAddress(const Address &aAddress)
{
    ThreadError error = kThreadError_None;
    NetifUnicastAddress *last = NULL;
    int8_t aAddressIndexToRemove = -1;

    for (NetifUnicastAddress *cur = mUnicastAddresses; cur; cur = cur->GetNext())
    {
        if (memcmp(&cur->mAddress, &aAddress, sizeof(otIp6Address)) == 0)
        {
            aAddressIndexToRemove = GetExtUnicastAddressIndex(cur);
            VerifyOrExit(aAddressIndexToRemove != -1, error = kThreadError_InvalidArgs);

            if (last)
            {
                last->mNext = cur->mNext;
            }
            else
            {
                mUnicastAddresses = cur->GetNext();
            }

            break;
        }

        last = cur;
    }

    if (aAddressIndexToRemove != -1)
    {
        mMaskExtUnicastAddresses &= ~(1 << aAddressIndexToRemove);

        SetStateChangedFlags(OT_IP6_ADDRESS_REMOVED);
    }
    else
    {
        error = kThreadError_NotFound;
    }

exit:

    return error;
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
