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
 *   This file implements Thread Multicast Listener Registration Table management.
 */

#include "multicast_listener_registration_table.hpp"

#if (OPENTHREAD_FTD || OPENTHREAD_MTD) && OPENTHREAD_CONFIG_MLR_ENABLE

#include "common/instance.hpp"

namespace ot {

MulticastListenerRegistrationTable::Iterator::Iterator(Instance &                                 aInstance,
                                                       MulticastListenerRegistrationTable::Filter aFilter)
    : InstanceLocator(aInstance)
    , mFilter(aFilter)
{
    MulticastListenerRegistrationTable &table = GetInstance().Get<MulticastListenerRegistrationTable>();

    mCurrent = &table.mRegistrations[0];
    if (!MatchesFilter(*mCurrent, mFilter))
    {
        Advance();
    }
}

void MulticastListenerRegistrationTable::Iterator::Advance(void)
{
    MulticastListenerRegistrationTable &table = GetInstance().Get<MulticastListenerRegistrationTable>();

    do
    {
        mCurrent++;

        if (mCurrent >= OT_ARRAY_END(table.mRegistrations))
        {
            mCurrent = nullptr;
            break;
        }

    } while (!MatchesFilter(*mCurrent, mFilter));
}

MulticastListenerRegistrationTable::MulticastListenerRegistrationTable(Instance &aInstance)
    : InstanceLocator(aInstance)
{
}

bool MulticastListenerRegistrationTable::MatchesFilter(const MulticastListenerRegistration &      aRegistration,
                                                       MulticastListenerRegistrationTable::Filter aFilter)
{
    bool rval = false;

    switch (aFilter)
    {
    case kFilterInvalid:
        rval = !aRegistration.IsListened();
        break;
    case kFilterListened:
        rval = aRegistration.IsListened();
        break;
    case kFilterSubscribed:
        rval = aRegistration.IsSubscribed();
        break;
    case kFilterNotRegistered:
        rval = aRegistration.IsListened() &&
               aRegistration.GetRegistrationState() != MulticastListenerRegistration::kRegistrationStateRegistered;
        break;
    case kFilterRegistering:
        rval = aRegistration.GetRegistrationState() == MulticastListenerRegistration::kRegistrationStateRegistering;
        break;
    }

    return rval;
}

MulticastListenerRegistration *MulticastListenerRegistrationTable::Find(const Ip6::Address &aAddress) const
{
    MulticastListenerRegistration *ret = nullptr;

    for (Iterator iter(GetInstance(), kFilterListened); !iter.IsDone(); iter.Advance())
    {
        if (aAddress == iter.GetMulticastListenerRegistration().GetAddress())
        {
            ExitNow(ret = &iter.GetMulticastListenerRegistration());
        }
    }

exit:
    return ret;
}

MulticastListenerRegistration *MulticastListenerRegistrationTable::New(void)
{
    Iterator iter(GetInstance(), kFilterInvalid);
    return iter.IsDone() ? nullptr : &iter.GetMulticastListenerRegistration();
}

void MulticastListenerRegistrationTable::SetSubscribed(const Ip6::Address &aAddress, bool aSubscribed)
{
    otError                        error        = OT_ERROR_NONE;
    MulticastListenerRegistration *registration = Find(aAddress);

    if (registration == nullptr)
    {
        if (!aSubscribed)
        {
            ExitNow();
        }

        registration = New();
        VerifyOrExit(registration != nullptr, error = OT_ERROR_NO_BUFS);

        registration->SetAddress(aAddress);
    }

    SetSubscribed(*registration, aSubscribed);

exit:
    if (error != OT_ERROR_NONE)
    {
        LogRegistration(aSubscribed ? "subscribe" : "unsubscribe", aAddress, error);
    }
}

void MulticastListenerRegistrationTable::SetSubscribed(MulticastListenerRegistration &aRegistration, bool aSubscribed)
{
    bool oldListened;

    VerifyOrExit(aRegistration.IsSubscribed() != aSubscribed, OT_NOOP);

    oldListened = aRegistration.IsListened();
    aRegistration.SetLocallySubscribed(aSubscribed);
    OnRegistrationChanged(aRegistration, oldListened);

    LogRegistration(aSubscribed ? "subscribe" : "unsubscribe", aRegistration.GetAddress(), OT_ERROR_NONE);
exit:
    return;
}

void MulticastListenerRegistrationTable::SetRegistering(uint8_t aNum)
{
    Iterator iter(GetInstance(), kFilterNotRegistered);
    for (uint8_t i = 0; i < aNum && !iter.IsDone(); i++, iter.Advance())
    {
        iter.GetMulticastListenerRegistration().SetRegistrationState(
            MulticastListenerRegistration::kRegistrationStateRegistering);
    }
}

void MulticastListenerRegistrationTable::FinishRegistering(bool aRegisteredOk)
{
    for (Iterator iter(GetInstance(), kFilterRegistering); !iter.IsDone(); iter.Advance())
    {
        iter.GetMulticastListenerRegistration().SetRegistrationState(
            aRegisteredOk ? MulticastListenerRegistration::kRegistrationStateRegistered
                          : MulticastListenerRegistration::kRegistrationStateToRegister);
    }
}

size_t MulticastListenerRegistrationTable::Count(MulticastListenerRegistrationTable::Filter aFilter)
{
    size_t count = 0;

    for (Iterator iter(GetInstance(), aFilter); !iter.IsDone(); iter.Advance())
    {
        count++;
    }

    return count;
}

void MulticastListenerRegistrationTable::SetAllToRegister(void)
{
    for (Iterator iter(GetInstance(), kFilterListened); !iter.IsDone(); iter.Advance())
    {
        iter.GetMulticastListenerRegistration().SetRegistrationState(
            MulticastListenerRegistration::kRegistrationStateToRegister);
    }
}

void MulticastListenerRegistrationTable::Print(void)
{
#if OPENTHREAD_CONFIG_LOG_BBR && OPENTHREAD_CONFIG_LOG_LEVEL >= OT_LOG_LEVEL_DEBG
    for (Iterator iter(GetInstance(), kFilterListened); !iter.IsDone(); iter.Advance())
    {
        MulticastListenerRegistration &registration = iter.GetMulticastListenerRegistration();

        otLogDebgBbr(
            "MLR: %s: %s%s", registration.GetAddress().ToString().AsCString(),
            registration.GetRegistrationState() == MulticastListenerRegistration::kRegistrationStateRegistered
                ? "R"
                : (registration.GetRegistrationState() == MulticastListenerRegistration::kRegistrationStateRegistering
                       ? "r"
                       : ""),
            registration.IsSubscribed() ? "S" : "");
    }
#endif
}

void MulticastListenerRegistrationTable::LogRegistration(const char *        aAction,
                                                         const Ip6::Address &aAddress,
                                                         otError             aError)
{
    OT_UNUSED_VARIABLE(aAddress);
    OT_UNUSED_VARIABLE(aAction);

#if OPENTHREAD_CONFIG_LOG_BBR && OPENTHREAD_CONFIG_LOG_LEVEL >= OT_LOG_LEVEL_WARN
    if (aError == OT_ERROR_NONE)
    {
        otLogInfoBbr("%s %s: %s", aAction, aAddress.ToString().AsCString(), otThreadErrorToString(aError));
    }
    else
    {
        otLogWarnBbr("%s %s: %s", aAction, aAddress.ToString().AsCString(), otThreadErrorToString(aError));
    }
#endif
}

void MulticastListenerRegistrationTable::OnRegistrationChanged(MulticastListenerRegistration &aRegistration,
                                                               bool                           aOldListened)
{
    if (!aRegistration.IsListened())
    {
        aRegistration.Clear();
    }
    else if (!aOldListened)
    {
        aRegistration.SetRegistrationState(MulticastListenerRegistration::kRegistrationStateToRegister);
    }
}

} // namespace ot

#endif // (OPENTHREAD_FTD || OPENTHREAD_MTD) && OPENTHREAD_CONFIG_MLR_ENABLE
