/*
 *  Copyright (c) 2020, The OpenThread Authors.
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
 *   This file implements the Multicast Listeners Table.
 */

#include "multicast_listeners_table.hpp"

#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_BACKBONE_ROUTER_MULTICAST_ROUTING_ENABLE

#include "instance/instance.hpp"

namespace ot {

namespace BackboneRouter {

RegisterLogModule("BbrMlt");

Error MulticastListenersTable::Add(const Ip6::Address &aAddress, Time aExpireTime)
{
    Error error = kErrorNone;

    VerifyOrExit(aAddress.IsMulticastLargerThanRealmLocal(), error = kErrorInvalidArgs);

    for (uint16_t i = 0; i < mNumValidListeners; i++)
    {
        Listener &listener = mListeners[i];

        if (listener.GetAddress() == aAddress)
        {
            listener.SetExpireTime(aExpireTime);
            FixHeap(i);
            ExitNow();
        }
    }

    VerifyOrExit(mNumValidListeners < GetArrayLength(mListeners), error = kErrorNoBufs);

    mListeners[mNumValidListeners].SetAddress(aAddress);
    mListeners[mNumValidListeners].SetExpireTime(aExpireTime);
    mNumValidListeners++;

    FixHeap(mNumValidListeners - 1);

    mCallback.InvokeIfSet(MapEnum(Listener::kEventAdded), &aAddress);

exit:
    Log(kAdd, aAddress, aExpireTime, error);
    CheckInvariants();
    return error;
}

void MulticastListenersTable::Remove(const Ip6::Address &aAddress)
{
    Error error = kErrorNotFound;

    for (uint16_t i = 0; i < mNumValidListeners; i++)
    {
        Listener &listener = mListeners[i];

        if (listener.GetAddress() == aAddress)
        {
            mNumValidListeners--;

            if (i != mNumValidListeners)
            {
                listener = mListeners[mNumValidListeners];
                FixHeap(i);
            }

            mCallback.InvokeIfSet(MapEnum(Listener::kEventRemoved), &aAddress);

            ExitNow(error = kErrorNone);
        }
    }

exit:
    Log(kRemove, aAddress, TimeMilli(0), error);
    CheckInvariants();
}

void MulticastListenersTable::Expire(void)
{
    TimeMilli    now = TimerMilli::GetNow();
    Ip6::Address address;

    while (mNumValidListeners > 0 && now >= mListeners[0].GetExpireTime())
    {
        Log(kExpire, mListeners[0].GetAddress(), mListeners[0].GetExpireTime(), kErrorNone);
        address = mListeners[0].GetAddress();

        mNumValidListeners--;

        if (mNumValidListeners > 0)
        {
            mListeners[0] = mListeners[mNumValidListeners];
            FixHeap(0);
        }

        mCallback.InvokeIfSet(MapEnum(Listener::kEventRemoved), &address);
    }

    CheckInvariants();
}

#if OT_SHOULD_LOG_AT(OT_LOG_LEVEL_DEBG)
void MulticastListenersTable::Log(Action              aAction,
                                  const Ip6::Address &aAddress,
                                  TimeMilli           aExpireTime,
                                  Error               aError) const
{
    static const char *const kActionStrings[] = {
        "Add",    // (0) kAdd
        "Remove", // (1) kRemove
        "Expire", // (2) kExpire
    };

    struct EnumCheck
    {
        InitEnumValidatorCounter();
        ValidateNextEnum(kAdd);
        ValidateNextEnum(kRemove);
        ValidateNextEnum(kExpire);
    };

    LogDebg("%s %s expire %lu: %s", kActionStrings[aAction], aAddress.ToString().AsCString(),
            ToUlong(aExpireTime.GetValue()), ErrorToString(aError));
}
#else
void MulticastListenersTable::Log(Action, const Ip6::Address &, TimeMilli, Error) const {}
#endif

void MulticastListenersTable::FixHeap(uint16_t aIndex)
{
    if (!SiftHeapElemDown(aIndex))
    {
        SiftHeapElemUp(aIndex);
    }
}

void MulticastListenersTable::CheckInvariants(void) const
{
#if OPENTHREAD_EXAMPLES_SIMULATION && OPENTHREAD_CONFIG_ASSERT_ENABLE
    for (uint16_t child = 1; child < mNumValidListeners; ++child)
    {
        uint16_t parent = (child - 1) / 2;

        OT_ASSERT(!(mListeners[child] < mListeners[parent]));
    }
#endif
}

bool MulticastListenersTable::SiftHeapElemDown(uint16_t aIndex)
{
    uint16_t index = aIndex;
    Listener saveElem;

    OT_ASSERT(aIndex < mNumValidListeners);

    saveElem = mListeners[aIndex];

    for (;;)
    {
        uint16_t child = 2 * index + 1;

        if (child >= mNumValidListeners || child <= index) // child <= index after int overflow
        {
            break;
        }

        if (child + 1 < mNumValidListeners && mListeners[child + 1] < mListeners[child])
        {
            child++;
        }

        if (!(mListeners[child] < saveElem))
        {
            break;
        }

        mListeners[index] = mListeners[child];

        index = child;
    }

    if (index > aIndex)
    {
        mListeners[index] = saveElem;
    }

    return index > aIndex;
}

void MulticastListenersTable::SiftHeapElemUp(uint16_t aIndex)
{
    uint16_t index = aIndex;
    Listener saveElem;

    OT_ASSERT(aIndex < mNumValidListeners);

    saveElem = mListeners[aIndex];

    for (;;)
    {
        uint16_t parent = (index - 1) / 2;

        if (index == 0 || !(saveElem < mListeners[parent]))
        {
            break;
        }

        mListeners[index] = mListeners[parent];

        index = parent;
    }

    if (index < aIndex)
    {
        mListeners[index] = saveElem;
    }
}

MulticastListenersTable::Listener *MulticastListenersTable::IteratorBuilder::begin(void)
{
    return &Get<MulticastListenersTable>().mListeners[0];
}

MulticastListenersTable::Listener *MulticastListenersTable::IteratorBuilder::end(void)
{
    return &Get<MulticastListenersTable>().mListeners[Get<MulticastListenersTable>().mNumValidListeners];
}

void MulticastListenersTable::Clear(void)
{
    if (mCallback.IsSet())
    {
        for (uint16_t i = 0; i < mNumValidListeners; i++)
        {
            mCallback.Invoke(MapEnum(Listener::kEventRemoved), &mListeners[i].GetAddress());
        }
    }

    mNumValidListeners = 0;

    CheckInvariants();
}

void MulticastListenersTable::SetCallback(Listener::Callback aCallback, void *aContext)
{
    mCallback.Set(aCallback, aContext);

    if (mCallback.IsSet())
    {
        for (uint16_t i = 0; i < mNumValidListeners; i++)
        {
            mCallback.Invoke(MapEnum(Listener::kEventAdded), &mListeners[i].GetAddress());
        }
    }
}

Error MulticastListenersTable::GetNext(Listener::Iterator &aIterator, Listener::Info &aInfo)
{
    Error     error = kErrorNone;
    TimeMilli now;

    VerifyOrExit(aIterator < mNumValidListeners, error = kErrorNotFound);

    now = TimerMilli::GetNow();

    aInfo.mAddress = mListeners[aIterator].mAddress;
    aInfo.mTimeout =
        Time::MsecToSec(mListeners[aIterator].mExpireTime > now ? mListeners[aIterator].mExpireTime - now : 0);

    aIterator++;

exit:
    return error;
}

} // namespace BackboneRouter

} // namespace ot

#endif // OPENTHREAD_FTD && OPENTHREAD_CONFIG_BACKBONE_ROUTER_MULTICAST_ROUTING_ENABLE
