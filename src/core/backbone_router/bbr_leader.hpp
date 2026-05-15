/*
 *  Copyright (c) 2019, The OpenThread Authors.
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
 *   This file includes definitions for Primary Backbone Router service management in the Thread Network.
 */

#ifndef OT_CORE_BACKBONE_ROUTER_BBR_LEADER_HPP_
#define OT_CORE_BACKBONE_ROUTER_BBR_LEADER_HPP_

#include "openthread-core-config.h"

#if (OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2)

#include <openthread/backbone_router.h>
#include <openthread/backbone_router_ftd.h>
#include <openthread/ip6.h>

#include "coap/coap.hpp"
#include "coap/coap_message.hpp"
#include "common/locator.hpp"
#include "common/log.hpp"
#include "common/non_copyable.hpp"
#include "common/notifier.hpp"
#include "common/string.hpp"
#include "net/ip6_address.hpp"

namespace ot {

namespace BackboneRouter {

constexpr uint16_t kDefaultRegistrationDelay  = 5;                 ///< Default registration delay (in sec).
constexpr uint32_t kDefaultMlrTimeout         = 3600;              ///< Default MLR Timeout (in sec).
constexpr uint32_t kMinMlrTimeout             = 300;               ///< Minimum MLR Timeout (in sec).
constexpr uint32_t kMaxMlrTimeout             = 0x7fffffff / 1000; ///< Max MLR Timeout (in sec ~ about 24 days.
constexpr uint8_t  kDefaultRegistrationJitter = 5;                 ///< Default registration jitter (in sec).
constexpr uint8_t  kParentAggregateDelay      = 5;                 ///< Parent Aggregate Delay (in sec).

static_assert(kDefaultMlrTimeout >= kMinMlrTimeout && kDefaultMlrTimeout <= kMaxMlrTimeout,
              "kDefaultMlrTimeout is not in valid range");
static_assert(kMaxMlrTimeout * 1000 > kMaxMlrTimeout, "SecToMsec(kMaxMlrTimeout) will overflow");
static_assert(kParentAggregateDelay > 1, "kParentAggregateDelay should be larger than 1 second");

/**
 * Represents Domain Prefix changes.
 */
enum DomainPrefixEvent : uint8_t
{
    kDomainPrefixAdded     = OT_BACKBONE_ROUTER_DOMAIN_PREFIX_ADDED,   ///< Domain Prefix Added.
    kDomainPrefixRemoved   = OT_BACKBONE_ROUTER_DOMAIN_PREFIX_REMOVED, ///< Domain Prefix Removed.
    kDomainPrefixRefreshed = OT_BACKBONE_ROUTER_DOMAIN_PREFIX_CHANGED, ///< Domain Prefix Changed.
};

class Leader;

/**
 * Represents a Backbone Router configuration.
 */
class Config : public otBackboneRouterConfig
{
    friend class Leader;

public:
    /**
     * Marks the configuration as absent.
     *
     * This is done by setting the Primary Backbone Router short address (`GetServer16()`) to `Mle::kInvalidRloc16`.
     */
    void MarkAsAbsent(void) { mServer16 = Mle::kInvalidRloc16; }

    /**
     * Indicates whether the configuration is present (i.e., it is derived from a Primary Backbone Router).
     *
     * The presence state is tracked using the Primary Backbone Router short address (`GetServer16()`).
     *
     * @retval TRUE   The configuration is present.
     * @retval FALSE  The configuration is not present.
     */
    bool IsPresent(void) const { return mServer16 != Mle::kInvalidRloc16; }

    /**
     * Gets the Primary Backbone Router short address.
     *
     * @returns The Primary Backbone Router short address, or `Mle::kInvalidRloc16` if not present.
     */
    uint16_t GetServer16(void) const { return mServer16; }

    /**
     * Gets the Reregistration Delay value.
     *
     * @returns The Reregistration Delay value (in seconds).
     */
    uint16_t GetReregistrationDelay(void) const { return mReregistrationDelay; }

    /**
     * Gets the Multicast Listener Registration (MLR) Timeout value.
     *
     * @returns The MLR Timeout value (in seconds).
     */
    uint32_t GetMlrTimeout(void) const { return mMlrTimeout; }

    /**
     * Gets the Sequence Number.
     *
     * @returns The Sequence Number.
     */
    uint8_t GetSequenceNumber(void) const { return mSequenceNumber; }

    /**
     * Selects a random reregistration delay.
     *
     * @returns A random reregistration delay in seconds.
     */
    uint16_t SelectRandomReregistrationDelay(void) const;

private:
    void AdjustMlrTimeout(void);

#if OT_SHOULD_LOG_AT(OT_LOG_LEVEL_INFO)
    void Log(const char *aTitle) const;
#endif
};

/**
 * Implements the basic Primary Backbone Router service operations.
 */
class Leader : public InstanceLocator, private NonCopyable
{
    friend class ot::Notifier;

public:
    // Primary Backbone Router Service state or state change.
    enum State : uint8_t
    {
        kStateNone = 0,       ///< Not exist (trigger Backbone Router register its service).
        kStateAdded,          ///< Newly added.
        kStateRemoved,        ///< Newly removed (trigger Backbone Router register its service).
        kStateToTriggerRereg, ///< Short address or sequence number changes (trigger re-registration).
                              ///< May also have ReregistrationDelay or MlrTimeout update.
        kStateRefreshed,      ///< Only ReregistrationDelay or MlrTimeout changes.
        kStateUnchanged,      ///< No change on Primary Backbone Router information (only for logging).
    };

    /**
     * Initializes the `Leader`.
     *
     * @param[in] aInstance  A reference to the OpenThread instance.
     */
    explicit Leader(Instance &aInstance);

    /**
     * Resets the cached Primary Backbone Router.
     */
    void Reset(void);

    /**
     * Gets the Primary Backbone Router configuration.
     *
     * @returns The Primary Backbone Router configuration.
     */
    const Config &GetConfig(void) const { return mConfig; }

    /**
     * Reads the Primary Backbone Router configuration in the Thread Network.
     *
     * @param[out]  aConfig        A reference to a `Config` to populate.
     *
     * @retval kErrorNone          Successfully read the Primary BBR config. @p aConfig is updated.
     * @retval kErrorNotFound      No Backbone Router in the Thread Network.
     */
    Error ReadConfig(Config &aConfig) const;

    /**
     * Gets the Backbone Router Service ID.
     *
     * @param[out]  aServiceId     The reference whether to put the Backbone Router Service ID.
     *
     * @retval kErrorNone          Successfully got the Backbone Router Service ID.
     * @retval kErrorNotFound      Backbone Router service doesn't exist.
     */
    Error GetServiceId(uint8_t &aServiceId) const;

    /**
     * Gets the short address of the Primary Backbone Router.
     *
     * @returns short address of Primary Backbone Router, or Mle::kInvalidRloc16 if no Primary Backbone Router.
     */
    uint16_t GetServer16(void) const { return mConfig.mServer16; }

    /**
     * Indicates whether or not there is Primary Backbone Router.
     *
     * @retval TRUE   If there is Primary Backbone Router.
     * @retval FALSE  If there is no Primary Backbone Router.
     */
    bool HasPrimary(void) const { return mConfig.IsPresent(); }

    /**
     * Gets the Domain Prefix in the Thread Network.
     *
     * @retval A pointer to the Domain Prefix or nullptr if there is no Domain Prefix.
     */
    const Ip6::Prefix *GetDomainPrefix(void) const { return HasDomainPrefix() ? &mDomainPrefix : nullptr; }

    /**
     * Indicates whether or not the Domain Prefix is available in the Thread Network.
     *
     * @retval TRUE   If there is Domain Prefix.
     * @retval FALSE  If there is no Domain Prefix.
     */
    bool HasDomainPrefix(void) const { return (mDomainPrefix.GetLength() > 0); }

    /**
     * Indicates whether or not the address is a Domain Unicast Address.
     *
     * @param[in]  aAddress A reference to the address.
     *
     * @retval true  @p aAddress is a Domain Unicast Address.
     * @retval false @p aAddress is not a Domain Unicast Address.
     */
    bool IsDomainUnicast(const Ip6::Address &aAddress) const;

private:
    void HandleNotifierEvents(Events aEvents);
    void UpdateBackboneRouterPrimary(void);
    void UpdateDomainPrefixConfig(void);
#if OT_SHOULD_LOG_AT(OT_LOG_LEVEL_INFO)
    static const char *StateToString(State aState);
    static const char *DomainPrefixEventToString(DomainPrefixEvent aEvent);
#endif

    Config      mConfig;
    Ip6::Prefix mDomainPrefix;
};

} // namespace BackboneRouter

DefineMapEnum(otBackboneRouterDomainPrefixEvent, BackboneRouter::DomainPrefixEvent);
DefineCoreType(otBackboneRouterConfig, BackboneRouter::Config);

} // namespace ot

#endif // (OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2)

#endif // OT_CORE_BACKBONE_ROUTER_BBR_LEADER_HPP_
