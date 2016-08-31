/*
 *  Copyright (c) 2016, Nest Labs, Inc.
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
#include <net/netif.hpp>

namespace Thread {
namespace Ip6 {

Netif *Netif::sNetifListHead = NULL;
int8_t Netif::sNextInterfaceId = 1;

Netif::Netif() :
    mStateChangedTask(&HandleStateChangedTask, this)
{
    mCallbacks = NULL;
    mUnicastAddresses = NULL;
    mMulticastAddresses = NULL;
    mInterfaceId = -1;
    mAllRoutersSubscribed = false;
    mNext = NULL;
    mCountExtUnicastAddresses = 0;
    mMaskExtUnicastAddresses = 0;

    mStateChangedFlags = 0;
}

ThreadError Netif::RegisterCallback(NetifCallback &aCallback)
{
    ThreadError error = kThreadError_None;

    for (NetifCallback *cur = mCallbacks; cur; cur = cur->mNext)
    {
        if (cur == &aCallback)
        {
            ExitNow(error = kThreadError_Busy);
        }
    }

    aCallback.mNext = mCallbacks;
    mCallbacks = &aCallback;

exit:
    return error;
}

ThreadError Netif::AddNetif()
{
    ThreadError error = kThreadError_None;
    Netif *netif;

    if (sNetifListHead == NULL)
    {
        sNetifListHead = this;
    }
    else
    {
        netif = sNetifListHead;

        do
        {
            if (netif == this)
            {
                ExitNow(error = kThreadError_Busy);
            }
        }
        while (netif->mNext);

        netif->mNext = this;
    }

    mNext = NULL;

    if (mInterfaceId < 0)
    {
        mInterfaceId = sNextInterfaceId++;
    }

exit:
    return error;
}

ThreadError Netif::RemoveNetif()
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(sNetifListHead != NULL, error = kThreadError_Busy);

    if (sNetifListHead == this)
    {
        sNetifListHead = mNext;
    }
    else
    {
        for (Netif *netif = sNetifListHead; netif->mNext; netif = netif->mNext)
        {
            if (netif->mNext != this)
            {
                continue;
            }

            netif->mNext = mNext;
            break;
        }
    }

    mNext = NULL;

exit:
    return error;
}

Netif *Netif::GetNext() const
{
    return mNext;
}

Netif *Netif::GetNetifById(int8_t aInterfaceId)
{
    Netif *netif;

    for (netif = sNetifListHead; netif; netif = netif->mNext)
    {
        if (netif->mInterfaceId == aInterfaceId)
        {
            ExitNow();
        }
    }

exit:
    return netif;
}

Netif *Netif::GetNetifByName(char *aName)
{
    Netif *netif;

    for (netif = sNetifListHead; netif; netif = netif->mNext)
    {
        if (strcmp(netif->GetName(), aName) == 0)
        {
            ExitNow();
        }
    }

exit:
    return netif;
}

int8_t Netif::GetInterfaceId() const
{
    return mInterfaceId;
}

bool Netif::IsMulticastSubscribed(const Address &aAddress) const
{
    bool rval = false;

    if (aAddress.IsLinkLocalAllNodesMulticast() || aAddress.IsRealmLocalAllNodesMulticast())
    {
        ExitNow(rval = true);
    }
    else if (aAddress.IsLinkLocalAllRoutersMulticast() || aAddress.IsRealmLocalAllRoutersMulticast())
    {
        ExitNow(rval = mAllRoutersSubscribed);
    }

    for (NetifMulticastAddress *cur = mMulticastAddresses; cur; cur = cur->mNext)
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

ThreadError Netif::SubscribeMulticast(NetifMulticastAddress &aAddress)
{
    ThreadError error = kThreadError_None;

    for (NetifMulticastAddress *cur = mMulticastAddresses; cur; cur = cur->mNext)
    {
        if (cur == &aAddress)
        {
            ExitNow(error = kThreadError_Busy);
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
        mMulticastAddresses = mMulticastAddresses->mNext;
        ExitNow();
    }
    else if (mMulticastAddresses != NULL)
    {
        for (NetifMulticastAddress *cur = mMulticastAddresses; cur->mNext; cur = cur->mNext)
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
            ExitNow(error = kThreadError_Busy);
        }
    }

    aAddress.mNext = mUnicastAddresses;
    mUnicastAddresses = &aAddress;

    if (!aAddress.GetAddress().IsRoutingLocator())
    {
        SetStateChangedFlags(OT_IP6_ADDRESS_ADDED);
    }

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

    ExitNow(error = kThreadError_Busy);

exit:

    if (!aAddress.GetAddress().IsRoutingLocator())
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
            ExitNow(error = kThreadError_Busy);
        }
    }

    // Make sure we haven't reached our limit already
    VerifyOrExit(mCountExtUnicastAddresses != OPENTHREAD_CONFIG_MAX_EXT_IP_ADDRS, error = kThreadError_NoBufs);

    // Get next available entry index
    while ((mMaskExtUnicastAddresses & (1 << index)) != 0)
    {
        index++;
    }

    assert(index < OPENTHREAD_CONFIG_MAX_EXT_IP_ADDRS);

    // Increase the count and mask the index
    mCountExtUnicastAddresses++;
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
        mCountExtUnicastAddresses--;
    
        SetStateChangedFlags(OT_IP6_ADDRESS_REMOVED);
    }
    else
    {
        error = kThreadError_Busy;
    }

exit:

    return error;
}

Netif *Netif::GetNetifList()
{
    return sNetifListHead;
}

bool Netif::IsUnicastAddress(const Address &aAddress)
{
    bool rval = false;

    for (Netif *netif = sNetifListHead; netif; netif = netif->mNext)
    {
        for (NetifUnicastAddress *cur = netif->mUnicastAddresses; cur; cur = cur->GetNext())
        {
            if (cur->GetAddress() == aAddress)
            {
                ExitNow(rval = true);
            }
        }
    }

exit:
    return rval;
}

const NetifUnicastAddress *Netif::SelectSourceAddress(MessageInfo &aMessageInfo)
{
    Address *destination = &aMessageInfo.GetPeerAddr();
    int interfaceId = aMessageInfo.mInterfaceId;
    const NetifUnicastAddress *rvalAddr = NULL;
    const Address *candidateAddr;
    int8_t candidateId;
    int8_t rvalIface = 0;

    for (Netif *netif = GetNetifList(); netif; netif = netif->mNext)
    {
        candidateId = netif->GetInterfaceId();

        for (const NetifUnicastAddress *addr = netif->GetUnicastAddresses(); addr; addr = addr->GetNext())
        {
            candidateAddr = &addr->GetAddress();

            if (destination->IsLinkLocal() || destination->IsMulticast())
            {
                if (interfaceId != candidateId)
                {
                    continue;
                }
            }

            if (rvalAddr == NULL)
            {
                // Rule 0: Prefer any address
                rvalAddr = addr;
                rvalIface = candidateId;
            }
            else if (*candidateAddr == *destination)
            {
                // Rule 1: Prefer same address
                rvalAddr = addr;
                rvalIface = candidateId;
                goto exit;
            }
            else if (candidateAddr->GetScope() < rvalAddr->GetAddress().GetScope())
            {
                // Rule 2: Prefer appropriate scope
                if (candidateAddr->GetScope() >= destination->GetScope())
                {
                    rvalAddr = addr;
                    rvalIface = candidateId;
                }
            }
            else if (candidateAddr->GetScope() > rvalAddr->GetAddress().GetScope())
            {
                if (rvalAddr->GetAddress().GetScope() < destination->GetScope())
                {
                    rvalAddr = addr;
                    rvalIface = candidateId;
                }
            }
            else if (addr->mPreferredLifetime != 0 && rvalAddr->mPreferredLifetime == 0)
            {
                // Rule 3: Avoid deprecated addresses
                rvalAddr = addr;
                rvalIface = candidateId;
            }
            else if (aMessageInfo.mInterfaceId != 0 && aMessageInfo.mInterfaceId == candidateId &&
                     rvalIface != candidateId)
            {
                // Rule 4: Prefer home address
                // Rule 5: Prefer outgoing interface
                rvalAddr = addr;
                rvalIface = candidateId;
            }
            else if (destination->PrefixMatch(*candidateAddr) > destination->PrefixMatch(rvalAddr->GetAddress()))
            {
                // Rule 6: Prefer matching label
                // Rule 7: Prefer public address
                // Rule 8: Use longest prefix matching
                rvalAddr = addr;
                rvalIface = candidateId;
            }
        }
    }

exit:
    aMessageInfo.mInterfaceId = rvalIface;
    return rvalAddr;
}

int8_t Netif::GetOnLinkNetif(const Address &aAddress)
{
    int8_t rval = -1;

    for (Netif *netif = sNetifListHead; netif; netif = netif->mNext)
    {
        for (NetifUnicastAddress *cur = netif->mUnicastAddresses; cur; cur = cur->GetNext())
        {
            if (cur->GetAddress().PrefixMatch(aAddress) >= cur->mPrefixLength)
            {
                ExitNow(rval = netif->mInterfaceId);
            }
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
    Netif *obj = reinterpret_cast<Netif *>(aContext);
    obj->HandleStateChangedTask();
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
