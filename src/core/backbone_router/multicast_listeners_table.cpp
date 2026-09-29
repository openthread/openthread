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

MulticastListenersTable::MulticastListenersTable(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mTimer(aInstance)
{
}

Error MulticastListenersTable::Add(const Ip6::Address &aAddress, Time aExpireTime)
{
    Error     error = kErrorNone;
    bool      isNew = false;
    Listener *entry;

    VerifyOrExit(aAddress.IsMulticastLargerThanRealmLocal(), error = kErrorInvalidArgs);

    entry = mListeners.FindMatching(aAddress);

    if (entry == nullptr)
    {
        entry = mListeners.PushBack();
        isNew = true;
    }

    VerifyOrExit(entry != nullptr, error = kErrorNoBufs);

    entry->mAddress    = aAddress;
    entry->mExpireTime = aExpireTime;

    mTimer.FireAtIfEarlier(aExpireTime);

    if (isNew)
    {
        InvokeCallback(kEventAdded, aAddress);
    }

exit:
    Log(kAdd, aAddress, aExpireTime, error);
    return error;
}

void MulticastListenersTable::Remove(const Ip6::Address &aAddress)
{
    Error     error = kErrorNone;
    Listener *entry;

    entry = mListeners.FindMatching(aAddress);
    VerifyOrExit(entry != nullptr, error = kErrorNotFound);

    mListeners.Remove(*entry);

    InvokeCallback(kEventRemoved, aAddress);

exit:
    Log(kRemove, aAddress, TimeMilli(0), error);
}

void MulticastListenersTable::HandleTimer(void)
{
    TimeMilli    now = TimerMilli::GetNow();
    NextFireTime nextTime(now);

    for (uint16_t i = 0; i < mListeners.GetLength();)
    {
        if (mListeners[i].mExpireTime <= now)
        {
            Ip6::Address address = mListeners[i].mAddress;

            Log(kExpire, address, mListeners[i].mExpireTime, kErrorNone);
            mListeners.Remove(mListeners[i]);
            InvokeCallback(kEventRemoved, address);

            // When the entry is removed from the array it is replaced
            // with the last element. In this case, we do not
            // increment `i`.
        }
        else
        {
            nextTime.UpdateIfEarlier(mListeners[i].mExpireTime);
            i++;
        }
    }

    mTimer.FireAtIfEarlier(nextTime);
}

bool MulticastListenersTable::Has(const Ip6::Address &aAddress) const { return mListeners.ContainsMatching(aAddress); }

void MulticastListenersTable::InvokeCallback(Event aEvent, const Ip6::Address &aAddress) const
{
    mCallback.InvokeIfSet(aEvent, &aAddress);
}

#if OT_SHOULD_LOG_AT(OT_LOG_LEVEL_DEBG)
void MulticastListenersTable::Log(Action              aAction,
                                  const Ip6::Address &aAddress,
                                  TimeMilli           aExpireTime,
                                  Error               aError) const
{
#define ActionMapList(_) \
    _(kAdd, "Add")       \
    _(kRemove, "Remove") \
    _(kExpire, "Expire")

    DefineEnumStringArray(ActionMapList);

    LogDebg("%s %s expire %lu: %s", kStrings[aAction], aAddress.ToString().AsCString(), ToUlong(aExpireTime.GetValue()),
            ErrorToString(aError));
}
#else
void MulticastListenersTable::Log(Action, const Ip6::Address &, TimeMilli, Error) const {}
#endif

void MulticastListenersTable::Clear(void)
{
    while (!mListeners.IsEmpty())
    {
        Ip6::Address address = mListeners.Back()->mAddress;

        mListeners.PopBack();
        InvokeCallback(kEventRemoved, address);
    }

    mTimer.Stop();
}

void MulticastListenersTable::SetCallback(ListenerCallback aCallback, void *aContext)
{
    mCallback.Set(aCallback, aContext);

    if (mCallback.IsSet())
    {
        for (const Listener &listener : mListeners)
        {
            InvokeCallback(kEventAdded, listener.mAddress);
        }
    }
}

Error MulticastListenersTable::GetNext(ListenerIterator &aIterator, ListenerInfo &aInfo)
{
    Error     error = kErrorNone;
    TimeMilli now;

    VerifyOrExit(aIterator < mListeners.GetLength(), error = kErrorNotFound);

    now = TimerMilli::GetNow();

    aInfo.mAddress = mListeners[aIterator].mAddress;
    aInfo.mTimeout = Time::MsecToSec(mListeners[aIterator].mExpireTime.DetermineRemainingDurationFrom(now));

    aIterator++;

exit:
    return error;
}

} // namespace BackboneRouter

} // namespace ot

#endif // OPENTHREAD_FTD && OPENTHREAD_CONFIG_BACKBONE_ROUTER_MULTICAST_ROUTING_ENABLE
