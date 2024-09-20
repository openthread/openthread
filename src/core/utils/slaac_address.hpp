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
 *   This file includes definitions for Thread global IPv6 address configuration with SLAAC.
 */

#ifndef SLAAC_ADDRESS_HPP_
#define SLAAC_ADDRESS_HPP_

#include "openthread-core-config.h"

#if OPENTHREAD_CONFIG_IP6_SLAAC_ENABLE

#include "common/locator.hpp"
#include "common/non_copyable.hpp"
#include "common/notifier.hpp"
#include "net/netif.hpp"
#include "thread/network_data.hpp"

namespace ot {
namespace Utils {

/**
 * @addtogroup core-slaac-address
 *
 * @brief
 *   This module includes definitions for Thread global IPv6 address configuration with SLAAC.
 *
 * @{
 */

/**
 * Implements the SLAAC utility for Thread protocol.
 */
class Slaac : public InstanceLocator, private NonCopyable
{
    friend class ot::Notifier;

public:
    typedef otIp6SlaacPrefixFilter PrefixFilter; ///< Prefix filter function pointer.

    /**
     * Represents the secret key used for generating semantically opaque IID (per RFC 7217).
     */
    struct IidSecretKey
    {
        static constexpr uint16_t kSize = 32; ///< Secret key size for generating semantically opaque IID.

        uint8_t m8[kSize];
    };

    /**
     * Initializes the SLAAC manager object.
     *
     * Note that SLAAC module starts enabled.
     *
     * @param[in]  aInstance  A reference to the OpenThread instance.
     */
    explicit Slaac(Instance &aInstance);

    /**
     * Enables the SLAAC module.
     *
     * When enabled, new SLAAC addresses are generated and added from on-mesh prefixes in network data.
     */
    void Enable(void);

    /**
     * Disables the SLAAC module.
     *
     * When disabled, any previously added SLAAC address by this module is removed.
     */
    void Disable(void);

    /**
     * Indicates whether SLAAC module is enabled or not.
     *
     * @retval TRUE    SLAAC module is enabled.
     * @retval FALSE   SLAAC module is disabled.
     */
    bool IsEnabled(void) const { return mEnabled; }

    /**
     * Sets a SLAAC prefix filter handler.
     *
     * The handler is invoked by SLAAC module when it is about to add a SLAAC address based on a prefix. The return
     * boolean value from handler determines whether the address is filtered or added (TRUE to filter the address,
     * FALSE to add address).
     *
     * The filter can be set to `nullptr` to disable filtering (i.e., allow SLAAC addresses for all prefixes).
     *
     * @param[in] aFilter   The filter to use.
     */
    void SetFilter(PrefixFilter aFilter);

    /**
     * Generates the IID of an IPv6 address.
     *
     * @param[in,out]  aAddress            A reference to the address that will be filled with the IID generated.
     *                                     Note the prefix of the address must already be filled and will be used
     *                                     to generate the IID.
     * @param[in,out]  aDadCounter         The DAD_Counter that is employed to resolve Duplicate Address Detection
     *                                     conflicts.
     *
     * @retval kErrorNone    If successfully generated the IID.
     * @retval kErrorFailed  If no valid IID was generated.
     */
    Error GenerateIid(Ip6::Netif::UnicastAddress &aAddress, uint8_t &aDadCounter) const;

    /**
     * Searches in the list of deprecating SLAAC prefixes for a match to a given address and if found, returns the
     * Domain ID from the Prefix TLV in the Network Data for this SLAAC prefix.
     *
     * The `Slaac` module keeps track of the associated Domain IDs for deprecating SLAAC prefixes, even if the related
     * Prefix TLV has already been removed from the Network Data. This information is used during external route lookup
     * if a deprecating SLAAC address is used as the source address in an outbound message.
     *
     * @param[in]  aAddress   The address to search for.
     * @param[out] aDomainId  A reference to return the Domain ID.
     *
     * @retval kErrorNone       Found a match for @p aAddress and updated @p aDomainId.
     * @retval kErrorNotFound   Could not find a match for @p aAddress in deprecating SLAAC prefixes.
     */
    Error FindDomainIdFor(const Ip6::Address &aAddress, uint8_t &aDomainId) const;

private:
    static constexpr uint16_t kNumSlaacAddresses = OPENTHREAD_CONFIG_IP6_SLAAC_NUM_ADDRESSES;

    static constexpr uint16_t kMaxIidCreationAttempts = 256; // Maximum number of attempts when generating IID.

    static constexpr uint32_t kDeprecationInterval =
        TimeMilli::SecToMsec(OPENTHREAD_CONFIG_IP6_SLAAC_DEPRECATION_INTERVAL);

    enum Action : uint8_t
    {
        kAdding,
        kRemoving,
        kDeprecating,
    };

    class SlaacAddress : public Ip6::Netif::UnicastAddress
    {
    public:
        static constexpr uint8_t kInvalidContextId = 0;

        bool      IsInUse(void) const { return mValid; }
        void      MarkAsNotInUse(void) { mValid = false; }
        uint8_t   GetContextId(void) const { return mContextId; }
        void      SetContextId(uint8_t aContextId) { mContextId = aContextId; }
        uint8_t   GetDomainId(void) const { return mDomainId; }
        void      SetDomainId(uint8_t aDomainId) { mDomainId = aDomainId; }
        bool      IsDeprecating(void) const { return (mExpirationTime.GetValue() != kNotDeprecated); };
        void      MarkAsNotDeprecating(void) { mExpirationTime.SetValue(kNotDeprecated); }
        TimeMilli GetExpirationTime(void) const { return mExpirationTime; }
        void      SetExpirationTime(TimeMilli aTime)
        {
            mExpirationTime = aTime;

            if (mExpirationTime.GetValue() == kNotDeprecated)
            {
                mExpirationTime.SetValue(kNotDeprecated + 1);
            }
        }

    private:
        static constexpr uint32_t kNotDeprecated = 0; // Special `mExpirationTime` value to indicate not deprecated.

        uint8_t   mContextId;
        uint8_t   mDomainId;
        TimeMilli mExpirationTime;
    };

    bool        IsSlaac(const NetworkData::OnMeshPrefixConfig &aConfig) const;
    bool        IsFiltered(const NetworkData::OnMeshPrefixConfig &aConfig) const;
    void        RemoveOrDeprecateAddresses(void);
    void        RemoveAllAddresses(void);
    void        AddAddresses(void);
    void        DeprecateAddress(SlaacAddress &aAddress);
    void        RemoveAddress(SlaacAddress &aAddress);
    void        AddAddressFor(const NetworkData::OnMeshPrefixConfig &aConfig);
    bool        UpdateContextIdFor(SlaacAddress &aSlaacAddress);
    void        HandleTimer(void);
    void        GetIidSecretKey(IidSecretKey &aKey) const;
    void        HandleNotifierEvents(Events aEvents);
    void        LogAddress(Action aAction, const SlaacAddress &aAddress);
    static bool DoesConfigMatchNetifAddr(const NetworkData::OnMeshPrefixConfig &aConfig,
                                         const Ip6::Netif::UnicastAddress      &aAddr);

    using ExpireTimer = TimerMilliIn<Slaac, &Slaac::HandleTimer>;

    bool         mEnabled;
    PrefixFilter mFilter;
    ExpireTimer  mTimer;
    SlaacAddress mSlaacAddresses[kNumSlaacAddresses];
};

/**
 * @}
 */

} // namespace Utils
} // namespace ot

#endif // OPENTHREAD_CONFIG_IP6_SLAAC_ENABLE

#endif // SLAAC_ADDRESS_HPP_
