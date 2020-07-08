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
 *   This file includes definitions for Multicast Listener Registration Table on a Thread device.
 */

#ifndef MULTICAST_LISTENER_REGISTRATION_TABLE_HPP_
#define MULTICAST_LISTENER_REGISTRATION_TABLE_HPP_

#include "openthread-core-config.h"
#include <common/locator.hpp>

#if (OPENTHREAD_FTD || OPENTHREAD_MTD) && OPENTHREAD_CONFIG_MLR_ENABLE

#include "common/code_utils.hpp"
#include "common/debug.hpp"
#include "net/ip6_address.hpp"

namespace ot {

/**
 * @addtogroup core-mlr
 *
 * @{
 *
 */

/**
 * This class represents a Multicast Listener Registration entry.
 *
 * A multicast address is listened if it satisfies any of these cases:
 *   1. Subscribed: it's added to the Thread Netif.
 *   2. Proxied: it's added to the Thread Netif of at least one MTD Child.
 *
 */
class MulticastListenerRegistration : public Clearable<MulticastListenerRegistration>
{
    friend class MulticastListenerRegistrationTable;

public:
    /**
     * This method returns the multicast address.
     *
     * @returns The multicast address.
     *
     */
    const Ip6::Address &GetAddress(void) const { return mAddress; }

private:
    typedef enum RegistrationState
    {
        kRegistrationStateToRegister,  ///< Multicast address is subscribed and to be registered.
        kRegistrationStateRegistering, ///< Multicast address is subscribed and is being registered.
        kRegistrationStateRegistered,  ///< Multicast address is subscribed and registered.
    } RegistrationState;

    MulticastListenerRegistration(void) { Clear(); }

    void SetAddress(const Ip6::Address &aAddress)
    {
        OT_ASSERT(mAddress.IsUnspecified());
        OT_ASSERT(aAddress.IsMulticastLargerThanRealmLocal());

        mAddress = aAddress;
    }

    void              SetRegistrationState(RegistrationState aState) { mRegistrationState = aState; }
    RegistrationState GetRegistrationState(void) const { return mRegistrationState; }

    void SetLocallySubscribed(bool aLocallySubscribed) { mSubscribed = aLocallySubscribed; }
    bool IsSubscribed(void) const { return mSubscribed; }
    bool IsListened(void) const { return IsSubscribed(); }

    Ip6::Address      mAddress;
    RegistrationState mRegistrationState : 2; ///< The registration state.
    bool              mSubscribed : 1;        ///< Whether the multicast address is subscribed.
};

class MulticastListenerRegistrationTable : public InstanceLocator
{
public:
    /**
     * This enumeration defines Multicast Listener Registration filters used for finding a Multicast Listener
     * Registration or iterating through the Multicast Listener Registration Table. Each filter definition accepts a
     * subset of Multicast Listener Registration states.
     *
     */
    enum Filter : uint8_t
    {
        kFilterInvalid,       ///< Filter invalid (i.e. not subscribed) entries.
        kFilterListened,      ///< Filter subscribed entries.
        kFilterSubscribed,    ///< Filter subscribed entries.
        kFilterNotRegistered, ///< Filter entries subscribed to but not registered.
        kFilterRegistering,   ///< Filter entries subscribed and is being registered (MLR.rsp is pending).
    };

    /**
     * This class represents an iterator for iterating through the Multicast Listener Registration Table.
     *
     */
    class Iterator : public InstanceLocator
    {
    public:
        /**
         * This constructor initializes an `Iterator` instance to start from beginning of the listener table.
         *
         * @param[in] aInstance  A reference to the OpenThread instance.
         * @param[in] aFilter    A Listener filter.
         *
         */
        Iterator(Instance &aInstance, Filter aFilter);

        /**
         * This method advances the iterator.
         *
         * The iterator is moved to point to the next `MulticastListenerRegistration` entry matching the given mask
         * filter in the constructor. If there are no more `MulticastListenerRegistration` entries matching the given
         * filter, the iterator becomes empty (i.e. `IsDone()` returns `true`).
         *
         */
        void Advance(void);

        /**
         * This method advances the iterator.
         *
         * The iterator is moved to point to the next `MulticastListenerRegistration` entry matching the given mask
         * filter in the constructor. If there are no more `MulticastListenerRegistration` entries matching the given
         * filter, the iterator becomes empty (i.e. `IsDone()` returns `true`).
         *
         */
        bool IsDone(void) const { return mCurrent == nullptr; }

        /**
         * This method gets the `MulticastListenerRegistration` entry to which the iterator is currently pointing.
         *
         * @returns A reference to the `MulticastListenerRegistration` entry.
         *
         */
        MulticastListenerRegistration &GetMulticastListenerRegistration(void) { return *mCurrent; }

    private:
        Filter                         mFilter;
        MulticastListenerRegistration *mCurrent;
    };

    /**
     * This constructor initializes the Multicast Listener Registration Table object.
     *
     * @param[in]  aInstance     A reference to the OpenThread instance.
     *
     */
    explicit MulticastListenerRegistrationTable(Instance &aInstance);

    /**
     * This method searches the Multicast Listener Registration Table for a `MulticastListenerRegistration` with a given
     * address also matching a given filter.
     *
     * @param[in] aAddress  The IP6 address.
     *
     * @returns A pointer to the `MulticastListenerRegistration` entry if found, or `nullptr` otherwise.
     *
     */
    MulticastListenerRegistration *Find(const Ip6::Address &aAddress) const;

    /**
     * This method gets a new/unused `MulticastListenerRegistration` entry from the Multicast Listener Registration
     * Table.
     *
     * @note The returned listener entry will be cleared (`memset` to zero).
     *
     * @returns A pointer to a new `MulticastListenerRegistration` entry, or `nullptr` if all
     * `MulticastListenerRegistration` entries are in use.
     *
     */
    MulticastListenerRegistration *New(void);

    /**
     * This method counts the `MulticastListenerRegistration` entries that matched by filter @p aFilter.
     *
     * @param[in] aFilter  The filter to match.
     *
     * @returns The count of filtered `MulticastListenerRegistration` entries.
     *
     */
    size_t Count(Filter aFilter);

    /**
     * This method sets if multicast address @p aAddress is subscribed.
     *
     * @param[in] aAddress            The multicast address.
     * @param[in] aSubscribed  Whether the multicast address is subscribed.
     *
     */
    void SetSubscribed(const Ip6::Address &aAddress, bool aSubscribed);

    /**
     * This method sets if the Multicast Listener Registration @p aRegistration is subscribed.
     *
     * @param[in] aRegistration  The Multicast Listener Registration.
     * @param[in] aSubscribed    Whether the Multicast Listener Registration is subscribed.
     *
     */
    static void SetSubscribed(MulticastListenerRegistration &aRegistration, bool aSubscribed);

    /**
     * This method sets the registration state of a number of Multicast Listener Registrations from
     * `kRegistrationStateToRegister` to `kRegistrationStateRegistering`.
     *
     * @param[in] aNum  The number of Multicast Listener Registrations that is registering.
     *
     */
    void SetRegistering(uint8_t aNum);

    /**
     * This method sets the registration state of all Multicast Listener Registrations' to
     * `kRegistrationStateToRegister`.
     *
     * This is used by MLR reregistration or renewing.
     *
     */
    void SetAllToRegister(void);

    /**
     * This method sets the registration state of Multicast Listener Registrations from `kRegistrationStateRegistering`
     * to `kRegistrationStateRegistered` or `kRegistrationStateToRegister` according to whether the registration
     * succeed.
     *
     * @param[in] aRegisteredOk  Whether the registration was successful.
     *
     */
    void FinishRegistering(bool aRegisteredOk);

    /**
     * This method prints this Multicast Listener Registration Table.
     *
     */
    void Print(void);

private:
    static_assert(OPENTHREAD_CONFIG_IP6_MAX_MULTICAST_LISTENER_REGISTRATION_NUM >=
                      OPENTHREAD_CONFIG_IP6_MAX_EXT_MCAST_ADDRS,
                  "Too few Multicast Listener Registrations");
    enum
    {
        kMaxMulticastListenerRegistrationNum = OPENTHREAD_CONFIG_IP6_MAX_MULTICAST_LISTENER_REGISTRATION_NUM,
    };

    static bool MatchesFilter(const MulticastListenerRegistration &aRegistration, Filter aFilter);
    static void LogRegistration(const char *aAction, const Ip6::Address &aAddress, otError aError);
    static void OnRegistrationChanged(MulticastListenerRegistration &aRegistration, bool aOldListened);

    MulticastListenerRegistration mRegistrations[kMaxMulticastListenerRegistrationNum];
};

} // namespace ot

#endif // (OPENTHREAD_FTD || OPENTHREAD_MTD) && OPENTHREAD_CONFIG_MLR_ENABLE
#endif // MULTICAST_LISTENER_REGISTRATION_TABLE_HPP_
