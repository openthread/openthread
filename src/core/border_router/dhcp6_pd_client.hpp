/*
 *  Copyright (c) 2025, The OpenThread Authors.
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
 *   This file includes definitions for DHCPv6 Prefix Delegation (PD) Client.
 */

#ifndef DHCP6_PD_CLIENT_HPP_
#define DHCP6_PD_CLIENT_HPP_

#include "openthread-core-config.h"

#ifdef OT_CONFIG_DHCP6_PD_CLIENT_ENABLE
#error "OT_CONFIG_DHCP6_PD_CLIENT_ENABLE MUST NOT be defined directly. It is derived from other configs"
#endif

#define OT_CONFIG_DHCP6_PD_CLIENT_ENABLE                                                            \
    (OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE && OPENTHREAD_CONFIG_BORDER_ROUTING_DHCP6_PD_ENABLE && \
     OPENTHREAD_CONFIG_BORDER_ROUTING_DHCP6_PD_CLIENT_ENABLE)

#if OT_CONFIG_DHCP6_PD_CLIENT_ENABLE

#include "border_router/infra_if.hpp"
#include "common/array.hpp"
#include "common/clearable.hpp"
#include "common/error.hpp"
#include "common/heap_array.hpp"
#include "common/locator.hpp"
#include "common/owned_ptr.hpp"
#include "common/timer.hpp"
#include "net/dhcp6_types.hpp"
#include "net/ip6.hpp"

namespace ot {
namespace BorderRouter {

/**
 * Represents a DHCPv6 Prefix Delegation (PD) Client.
 */
class Dhcp6PdClient : public InstanceLocator
{
    friend class InfraIf;

public:
    /**
     * Represents a delegated prefix.
     */
    struct DelegatedPrefix
    {
        Ip6::Prefix mPrefix;            ///< The delegated prefix.
        Ip6::Prefix mAdjustedPrefix;    ///< The delegated prefix, adjusted to prefix length of 64.
        uint32_t    mT1;                ///< T1 duration in sec (time to renew)
        uint32_t    mT2;                ///< T1 duration in sec (time to rebind).
        uint32_t    mPreferredLifetime; ///< Preferred lifetime in sec.
        uint32_t    mValidLifetime;     ///< Valid lifetime in sec.
        TimeMilli   mUpdateTime;        ///< The last update time of this prefix.
    };

    /**
     * Initializes the `Dhcp6PdClient`.
     *
     * @param[in]  aInstance  A OpenThread instance.
     */
    explicit Dhcp6PdClient(Instance &aInstance);

    /**
     * Starts the client.
     *
     * Once started, the client locates DHCPv6 servers, selects one, and then requests the assignment of a delegated
     * prefix from the server. The client manages the lease renewal (extending the lifetimes of a delegated prefix)
     * and its removal.
     *
     * The favored delegated prefix is reported to `RoutingManager` directly using `ProcessDhcp6PdPrefix()`. Any
     * changes to the prefix (e.g., lease renewal, removal, or replacement of the prefix) are also reported using the
     * same method.
     */
    void Start(void);

    /**
     * Stops the client.
     *
     * The client will release any delegated prefixes.
     */
    void Stop(void);

    /**
     * Returns the delegated prefix.
     *
     * @returns A pointer to the delegated prefix, or `nullptr` if none.
     */
    const DelegatedPrefix *GetDelegatedPrefix(void) const { return mPdPrefixCommited ? &mPdPrefix : nullptr; }

private:
    // All intervals are in milli-seconds (from RFC 8415 section 7.6)
    static constexpr uint32_t kMaxDelayFirstSolicit  = Time::kOneSecondInMsec;        // SOL_MAX_DELAY
    static constexpr uint32_t kInitialSolicitTimeout = Time::kOneSecondInMsec;        // SOL_TIMEOUT
    static constexpr uint32_t kMaxSolicitTimeout     = 3600 * Time::kOneSecondInMsec; // SOL_MAX_RT
    static constexpr uint32_t kInitialRequestTimeout = Time::kOneSecondInMsec;        // REQ_TIMEOUT
    static constexpr uint32_t kMaxRequestTimeout     = 30 * Time::kOneSecondInMsec;   // REQ_MAX_RT
    static constexpr uint16_t kMaxRequestRetxCount   = 10;                            // REQ_MAX_RC
    static constexpr uint32_t kInitialRenewTimeout   = 10 * Time::kOneSecondInMsec;   // REN_TIMEOUT
    static constexpr uint32_t kMaxRenewTimeout       = 600 * Time::kOneSecondInMsec;  // REN_MAX_RT
    static constexpr uint32_t kInitialRebindTimeout  = 10 * Time::kOneSecondInMsec;   // REB_TIMEOUT
    static constexpr uint32_t kMaxRebindTimeout      = 600 * Time::kOneSecondInMsec;  // REB_MAX_RT
    static constexpr uint32_t kInitialReleaseTimeout = 1 * Time::kOneSecondInMsec;    // REL_TIMEOUT
    static constexpr uint16_t kMaxReleaseRetxCount   = 4;                             // REL_MAX_RC
    static constexpr uint32_t kInfiniteTimeout       = 0;
    static constexpr uint16_t kInfiniteRetxCount     = 0;

    static constexpr uint32_t kRetxDelayOnFailedTx = 330; // in msec

    static constexpr uint32_t kIaid                = 0;
    static constexpr uint8_t  kDesiredPrefixLength = 64;
    static constexpr uint8_t  kDefaultPreference   = 0;
    static constexpr uint32_t kJitterDivisor       = 10;

    // The constants below are in seconds
    static constexpr uint32_t kMinPreferredLifetime = OPENTHREAD_CONFIG_BORDER_ROUTING_DHCP6_PD_CLIENT_MIN_LIFETIME;
    static constexpr uint32_t kMaxPreferredLifetime = OPENTHREAD_CONFIG_BORDER_ROUTING_DHCP6_PD_CLIENT_MAX_LIFETIME;
    static constexpr uint32_t kMaxValidMarginAfterPreferredLifetime = 2 * Time::kOneMinuteInSec;
    static constexpr uint32_t kMinT1                                = 5 * Time::kOneMinuteInSec;
    static constexpr uint32_t kMinT1MarginBeforePreferredLifetime   = 15 * Time::kOneMinuteInSec;
    static constexpr uint32_t kMinT2MarginBeforePreferredLifetime   = 6 * Time::kOneMinuteInSec;

    static_assert(kMaxPreferredLifetime > kMinPreferredLifetime, "invalid min/max values for preferred lifetime");

    // Default T1 and T2 as 0.5 and 0.8 times the preferred lifetime
    // if they are zero (represented as 5/10 and 8/10).
    static constexpr uint32_t kDefaultT1FactorNumerator   = 5;
    static constexpr uint32_t kDefaultT1FactorDenominator = 10;
    static constexpr uint32_t kDefaultT2FactorNumerator   = 8;
    static constexpr uint32_t kDefaultT2FactorDenominator = 10;

    enum State : uint8_t
    {
        kStateStopped,
        kStateToSolicit,
        kStateSoliciting,
        kStateRequesting,
        kStateToRenew,
        kStateRenewing,
        kStateRebinding,
        kStateReleasing,
    };

    enum JitterMode : uint8_t
    {
        kPositiveJitter,
        kFullJitter,
    };

    typedef Array<uint8_t, Dhcp6::Duid::kMaxSize> ServerDuid;
    typedef Dhcp6::TransactionId                  TxnId;

    struct PdPrefix : public DelegatedPrefix, public Clearable<PdPrefix>
    {
        PdPrefix(void) { Clear(); }
        bool      IsValid(void) const { return mPrefix.GetLength() != 0; }
        TimeMilli DetermineT1Time(void) const;
        TimeMilli DetermineT2Time(void) const;
        TimeMilli DeterminePreferredTime(void) const;
        void      AdjustLifetimesT1AndT2(void);
        bool      Matches(const PdPrefix &aOther) const { return mPrefix == aOther.mPrefix; }
    };

    typedef Heap::Array<PdPrefix> PdPrefixArray;

    class RetxTracker
    {
        // Implements the retx behavior for DHCPv6 message exchanges
        // as per RFC 8415 Section 15.
    public:
        void         Start(uint32_t aInitialTimeout, uint32_t aMaxTimeout, JitterMode aJitterMode);
        void         SetMaxTimeout(uint32_t aMaxTimeout) { mMaxTimeout = aMaxTimeout; }
        void         SetMaxCount(uint16_t aMaxCount) { mMaxCount = aMaxCount; }
        void         SetRetxEndTime(TimeMilli aEndTime) { mHasEndTime = true, mEndTime = aEndTime; }
        const TxnId &GetTransactionId(void) { return mTransactionId; }
        bool         IsFirstAttempt(void) const { return (mCount <= 1); }
        void         ScheduleTimeoutTimer(TimerMilli &aTimer) const;
        void         UpdateTimeoutAndCountAfterTx(void);
        bool         ShouldRetx(void) const;
        uint16_t     DetermineElapsedTime(void);

    private:
        static constexpr uint32_t kJitterDivisor = 10;

        static uint32_t AddJitter(uint32_t aValue, JitterMode aJitterMode);

        TimeMilli mStartTime;
        TimeMilli mEndTime;
        uint32_t  mTimeout;
        uint32_t  mMaxTimeout;
        uint16_t  mCount;
        uint16_t  mMaxCount;
        TxnId     mTransactionId;
        bool      mHasEndTime;
        bool      mLongElapsedTime;
    };

    void      EnterState(State aState);
    void      HandleTimer(void);
    void      SendMessage(void);
    void      GetAllRelayAgentsAndServersMulticastAddress(Ip6::Address &aAddress) const;
    void      UpdateStateAfterRetxExhausted(void);
    Error     AppendIaPdOption(Message &aMessage);
    Error     AppendIaPrefixOption(Message &aMessage, const Ip6::Prefix &aPrefix);
    void      HandleReceived(Message &aMessage);
    Error     ParseHeaderAndValidateMessage(Message &aMessage, Dhcp6::Header &aHeader);
    void      HandleAdvertise(const Message &aMessage);
    void      HandleReply(const Message &aMessage);
    PdPrefix *SelectFavoredPrefix(PdPrefixArray &aPdPrefixes);
    void      ProcessSolMaxRtOption(const Message &aMessaget);
    Error     ProcessIaPd(const Message                   &aMessage,
                          PdPrefixArray                   &aPdPrefixes,
                          Dhcp6::StatusCodeOption::Status &aStatus) const;
    bool      ShouldSkipPdOption(const Dhcp6::IaPdOption &aIsPdOption) const;
    Error     ProcessIaPdPrefixes(const Message                   &aMessage,
                                  const Dhcp6::IaPdOption         &aIaPdOption,
                                  const OffsetRange               &aIaPdOptionsOffseRange,
                                  PdPrefixArray                   &aPdPrefixes,
                                  Dhcp6::StatusCodeOption::Status &aStatus) const;
    bool      ShouldSkipPrefixOption(const Dhcp6::IaPrefixOption &aPrefixOption) const;
    void      ProcessServerUnicastOption(const Message &aMessage, Ip6::Address &aServerAddress) const;
    void      ProcessPreferenceOption(const Message &aMessage, uint8_t &aPreference) const;
    void      SaveServerDuidAndAddress(const Message &aMessage);
    void      ClearServerDuid(void);
    void      ClearPdPrefix(void);
    void      CommitPdPrefix(const PdPrefix &aPdPrefix);
    void      ReportPdPrefixToRoutingManager(void);

    static const char *StateToString(State aState);
    static const char *MsgTypeToString(Dhcp6::MsgType aMsgType);

    using DelayTimer = TimerMilliIn<Dhcp6PdClient, &Dhcp6PdClient::HandleTimer>;

    State        mState;
    bool         mPdPrefixCommited;
    RetxTracker  mRetxTracker;
    uint32_t     mMaxSolicitTimeout;
    PdPrefix     mPdPrefix;
    ServerDuid   mServerDuid;
    Ip6::Address mServerAddress;
    DelayTimer   mTimer;
};

} // namespace BorderRouter
} // namespace ot

#endif // OT_CONFIG_DHCP6_PD_CLIENT_ENABLE

#endif // DHCP6_PD_CLIENT_HPP_
