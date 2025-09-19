/*
 *  Copyright (c) 2022, The OpenThread Authors.
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
 *   This file includes definitions for the NAT64 translator.
 */

#ifndef NAT64_TRANSLATOR_HPP_
#define NAT64_TRANSLATOR_HPP_

#include "openthread-core-config.h"

#include "common/locator.hpp"
#include "common/owning_list.hpp"
#include "common/pool.hpp"
#include "common/timer.hpp"
#include "net/ip4_types.hpp"
#include "net/ip6.hpp"

namespace ot {
namespace Nat64 {

enum State : uint8_t
{
    kStateDisabled   = OT_NAT64_STATE_DISABLED,    ///< The component is disabled.
    kStateNotRunning = OT_NAT64_STATE_NOT_RUNNING, ///< The component is enabled, but is not running.
    kStateIdle       = OT_NAT64_STATE_IDLE,        ///< NAT64 is enabled, but this BR is not an active NAT64 BR.
    kStateActive     = OT_NAT64_STATE_ACTIVE,      ///< The component is running.
};

/**
 * Converts a `State` into a string.
 *
 * @param[in]  aState     A state.
 *
 * @returns  A string representation of @p aState.
 */
const char *StateToString(State aState);

#if OPENTHREAD_CONFIG_NAT64_TRANSLATOR_ENABLE

/**
 * Implements the NAT64 translator.
 */
class Translator : public InstanceLocator, private NonCopyable
{
    struct Mapping;

public:
    typedef otNat64AddressMapping AddressMapping; ///< Address mapping.
    typedef otNat64DropReason     DropReason;     ///< Drop reason.
    typedef otNat64ErrorCounters  ErrorCounters;  ///< Error counters.

    /**
     * An iterator to iterate over `AddressMapping` entries.
     */
    class AddressMappingIterator : public otNat64AddressMappingIterator
    {
    public:
        /**
         * Initializes an `AddressMappingIterator`.
         *
         * An iterator MUST be initialized before it is used. An iterator can be initialized again to start from the
         * beginning of the mapping info list.
         *
         * @param[in] aInstance  The OpenThread instance.
         */
        void Init(Instance &aInstance);

        /**
         * Gets the next `AddressMapping` info using the iterator.
         *
         * @param[out] aMapping    An `AddressMapping` to output to next NAT64 address mapping.
         *
         * @retval kErrorNone      Successfully found the next NAT64 address mapping info.
         * @retval kErrorNotFound  No subsequent NAT64 address mapping info was found.
         */
        Error GetNext(AddressMapping &aMapping);

    private:
        void           SetMapping(const Mapping *aMapping) { mPtr = aMapping; }
        const Mapping *GetMapping(void) const { return static_cast<const Mapping *>(mPtr); }
        void           SetInitTime(TimeMilli aNow) { mData32 = aNow.GetValue(); }
        TimeMilli      GetInitTime(void) const { return TimeMilli(mData32); }
    };

    /**
     * Represents the counters for the protocols supported by NAT64.
     */
    class ProtocolCounters : public otNat64ProtocolCounters,
                             public Clearable<ProtocolCounters>,
                             public Equatable<ProtocolCounters>
    {
        friend class Translator;

    private:
        typedef otNat64Counters Counters;

        void Count6To4Packet(const Ip6::Headers &aIp6Headers);
        void Count4To6Packet(const Ip4::Headers &aIp4Headers);

        static void Update6To4(Counters &aCounters, uint16_t aSize);
        static void Update4To6(Counters &aCounters, uint16_t aSize);
    };

    /**
     * Initializes the NAT64 translator.
     *
     * @param[in] aInstance  The OpenThread instance.
     */
    explicit Translator(Instance &aInstance);

    /**
     * Set the state of NAT64 translator.
     *
     * Note: Disabling the translator will invalidate all address mappings.
     *
     * @param[in]  aEnabled   A boolean to enable/disable NAT64 translator.
     */
    void SetEnabled(bool aEnabled);

    /**
     * Gets the state of NAT64 translator.
     *
     * @retval  kNat64StateDisabled  The translator is disabled.
     * @retval  kNat64StateIdle      The translator is not configured with a valid NAT64 prefix and a CIDR.
     * @retval  kNat64StateActive    The translator is translating packets.
     */
    State GetState(void) const { return mState; }

    /**
     * Translates an IPv4 datagram to an IPv6 datagram and sends it via Thread interface.
     *
     * @param[in] aMessagePtr   An owned pointer to a message (ownership is transferred to the method).
     *
     * @retval kErrorNone     Successfully processed the message.
     * @retval kErrorDrop     Message was well-formed but not fully processed due to datagram processing rules.
     * @retval kErrorNoBufs   Could not allocate necessary message buffers when processing the datagram.
     * @retval kErrorNoRoute  No route to host.
     * @retval kErrorParse    Encountered a malformed header when processing the message.
     */
    Error SendMessage(OwnedPtr<Message> aMessagePtr);

    /**
     * Allocate a new message buffer for sending an IPv4 message (which will be translated into an IPv6 datagram by
     * NAT64 later). Message buffers allocated by this function will have 20 bytes (The differences between the size of
     * IPv6 headers and the size of IPv4 headers) reserved.
     *
     * @param[in]  aSettings  The message settings.
     *
     * @returns A pointer to the message buffer or NULL if no message buffers are available or parameters are invalid.
     */
    Message *NewIp4Message(const Message::Settings &aSettings);

    /**
     * Translates an IPv4 message to an IPv6 message.
     *
     * @param[in,out] aMessage    The message to be processed.
     *
     * @retval kErrorNone    The message was successfully translated from IPv4 to IPv6.
     * @retval kErrorDrop    The message should be dropped, e.g., it is malformed or translation failed.
     */
    Error TranslateIp4ToIp6(Message &aMessage);

    /**
     * Translates an IPv6 message to an IPv4 message.
     *
     * @param[in,out] aMessage    The message to be processed.
     *
     * @retval kErrorNone    The message was successfully translated from IPv6 to IPv4.
     * @retval kErrorDrop    The message should be dropped, e.g., it is malformed or translation failed.
     * @retval kErrorAbort   No translation was performed or required (e.g., NAT64 is disabled, the message is not
     *                       destined for a NAT64-mapped address). In this case, the message is not modified.
     */
    Error TranslateIp6ToIp4(Message &aMessage);

    /**
     * Sets the CIDR used when setting the source address of the outgoing translated IPv4 datagrams. A valid CIDR must
     * have a non-zero prefix length.
     *
     * @note The actual addresses pool is limited by the size of the mapping pool and the number of addresses available
     * in the CIDR block. If the provided is a valid IPv4 CIDR for NAT64, and it is different from the one already
     * configured, the NAT64 translator will be reset and all existing sessions will be expired.
     *
     * @param[in] aCidr the CIDR for the sources of the translated datagrams.
     *
     * @retval  kErrorInvalidArgs    The the given CIDR a valid CIDR for NAT64.
     * @retval  kErrorNone           Successfully enabled/disabled the NAT64 translator.
     */
    Error SetIp4Cidr(const Ip4::Cidr &aCidr);

    /**
     * Clears the CIDR used when setting the source address of the outgoing translated IPv4 datagrams.
     *
     * @note The NAT64 translator will be reset and all existing sessions will be expired when clearing the configured
     * CIDR.
     */
    void ClearIp4Cidr(void);

    /**
     * Gets the configured CIDR in the NAT64 translator.
     *
     * @param[out] aCidr        The `Ip4::Cidr` Where the configured CIDR will be placed.
     *
     * @retval kErrorNone       @p aCidr is set to the configured CIDR.
     * @retval kErrorNotFound   The translator is not configured with an IPv4 CIDR.
     */
    Error GetIp4Cidr(Ip4::Cidr &aCidr) const;

    /**
     * Sets the prefix of NAT64-mapped addresses in the thread network. The address mapping table will not be cleared.
     * Equals to `ClearNat64Prefix` when an empty prefix is provided.
     *
     * @param[in] aNat64Prefix The prefix of the NAT64-mapped addresses.
     */
    void SetNat64Prefix(const Ip6::Prefix &aNat64Prefix);

    /**
     * Clear the prefix of NAT64-mapped addresses in the thread network. The address mapping table will not be cleared.
     * The translator will return kNotTranslated for all IPv6 datagrams and kDrop for all IPv4 datagrams.
     */
    void ClearNat64Prefix(void);

    /**
     * Gets the configured IPv6 prefix in the NAT64 translator.
     *
     * @param[out] aPrefix      The `Ip6::Prefix` where the configured NAT64 prefix will be placed.
     *
     * @retval kErrorNone       @p aPrefix is set to the configured prefix.
     * @retval kErrorNotFound   The translator is not configured with an IPv6 prefix.
     */
    Error GetNat64Prefix(Ip6::Prefix &aPrefix) const;

    /**
     * Gets the NAT64 translator counters.
     *
     * The counters are initialized to zero when the OpenThread instance is initialized.
     *
     * @returns The protocol counters.
     */
    const ProtocolCounters &GetCounters(void) const { return mCounters; }

    /**
     * Gets the NAT64 translator error counters.
     *
     * The counters are initialized to zero when the OpenThread instance is initialized.
     *
     * @returns The error counters.
     */
    const ErrorCounters &GetErrorCounters(void) const { return mErrorCounters; }

private:
    // Timeouts are in milliseconds
    static constexpr uint32_t kIdleTimeout = OPENTHREAD_CONFIG_NAT64_IDLE_TIMEOUT_SECONDS * Time::kOneSecondInMsec;
    static constexpr uint32_t kIcmpTimeout = OPENTHREAD_CONFIG_NAT64_ICMP_IDLE_TIMEOUT_SECONDS * Time::kOneSecondInMsec;

    static constexpr uint32_t kMinUdpTimeout   = 2 * Time::kOneMinuteInMsec; //  UDP_MIN in RFC 6146 (2 min)
    static constexpr uint32_t kMinEvictTimeout = kMinUdpTimeout;

    static_assert(kIdleTimeout >= kMinUdpTimeout, "OPENTHREAD_CONFIG_NAT64_IDLE_TIMEOUT_SECONDS is too short");

    static constexpr uint32_t kPoolSize = OPENTHREAD_CONFIG_NAT64_MAX_MAPPINGS;

#if OPENTHREAD_CONFIG_NAT64_PORT_TRANSLATION_ENABLE
    static constexpr uint16_t kMinTranslationPort = 49152;
    static constexpr uint16_t kMaxTranslationPort = 65535;
#endif

    static constexpr DropReason kReasonUnknown          = OT_NAT64_DROP_REASON_UNKNOWN;
    static constexpr DropReason kReasonIllegalPacket    = OT_NAT64_DROP_REASON_ILLEGAL_PACKET;
    static constexpr DropReason kReasonUnsupportedProto = OT_NAT64_DROP_REASON_UNSUPPORTED_PROTO;
    static constexpr DropReason kReasonNoMapping        = OT_NAT64_DROP_REASON_NO_MAPPING;

    struct Mapping : public InstanceLocatorInit, public LinkedListEntry<Mapping>
    {
        static constexpr uint16_t kInfoStringSize = 80;

        typedef String<kInfoStringSize> InfoString;

        enum ProtoCategory : uint8_t
        {
            kIcmpOnly,
            kUdpAndMaybeIcmp,
            kTcpAndMaybeOthers,
        };

        void          Init(Instance &aInstance) { InstanceLocatorInit::Init(aInstance); }
        void          Free(void);
        void          Touch(uint8_t aProtocol);
        InfoString    ToString(void) const;
        void          CopyTo(AddressMapping &aMapping, TimeMilli aNow) const;
        uint32_t      DetermineDurationSinceUse(TimeMilli aNow) const { return aNow - mLastUseTime; }
        ProtoCategory DetermineProtocolCategory(void) const;
        bool          IsEligibleForEviction(TimeMilli aNow) const;
        bool          IsBetterEvictionCandidateOver(const Mapping &aOther, TimeMilli aNow) const;
        bool          Matches(const Ip6::Headers &aIp6Headers) const;
        bool          Matches(const Ip4::Headers &aIp4Headers) const;
        bool          Matches(const ExpirationChecker &aChecker) const { return aChecker.IsExpired(mExpirationTime); }
        bool          Matches(const Mapping &aMapping) const { return this == &aMapping; }
#if OPENTHREAD_CONFIG_NAT64_PORT_TRANSLATION_ENABLE
        bool Matches(const uint16_t aPort) const { return mTranslatedPortOrId == aPort; }
#else
        bool Matches(const Ip4::Address &aIp4Address) const { return mIp4Address == aIp4Address; }
#endif

        static bool IsCounterZero(const ProtocolCounters::Counters &aCounters);

        Mapping         *mNext;
        uint64_t         mId;
        TimeMilli        mLastUseTime;
        TimeMilli        mExpirationTime;
        Ip4::Address     mIp4Address;
        Ip6::Address     mIp6Address;
        ProtocolCounters mCounters;
#if OPENTHREAD_CONFIG_NAT64_PORT_TRANSLATION_ENABLE
        uint16_t mSrcPortOrId;
        uint16_t mTranslatedPortOrId;
#endif
    };

    bool     IsEnabled(void) const { return mState != kStateDisabled; }
    bool     HasValidPrefixAndCidr(void) const;
    void     SetState(State aState);
    void     UpdateState(void);
    Error    TranslateIcmp4(Message &aMessage, uint16_t aOriginalId);
    Error    TranslateIcmp6(Message &aMessage, uint16_t aTranslatedId);
    void     GetNextIp4Address(Ip4::Address &aIp4Address);
    Error    AllocateIp4Address(Ip4::Address &aIp4Address);
    Mapping *AllocateMapping(const Ip6::Headers &aIp6Headers);
    void     EvictStaleMapping(void);
    void     HandleTimer(void);
#if OPENTHREAD_CONFIG_NAT64_PORT_TRANSLATION_ENABLE
    uint16_t AllocateSourcePort(uint16_t aSrcPort);
#endif

    static uint16_t GetSourcePortOrIcmp6Id(const Ip6::Headers &aIp6Headers);
    static uint16_t GetDestinationPortOrIcmp4Id(const Ip4::Headers &aIp4Headers);

    using TranslatorTimer = TimerMilliIn<Translator, &Translator::HandleTimer>;

    State                    mState;
    uint64_t                 mNextMappingId;
    Pool<Mapping, kPoolSize> mMappingPool;
    OwningList<Mapping>      mActiveMappings;
    Ip6::Prefix              mNat64Prefix;
    Ip4::Cidr                mIp4Cidr;
    uint32_t                 mMinHostId;
    uint32_t                 mMaxHostId;
    uint32_t                 mNextHostId;
    TranslatorTimer          mTimer;
    ProtocolCounters         mCounters;
    ErrorCounters            mErrorCounters;
};
#endif // OPENTHREAD_CONFIG_NAT64_TRANSLATOR_ENABLE

} // namespace Nat64

DefineMapEnum(otNat64State, Nat64::State);
#if OPENTHREAD_CONFIG_NAT64_TRANSLATOR_ENABLE
DefineCoreType(otNat64AddressMappingIterator, Nat64::Translator::AddressMappingIterator);
DefineCoreType(otNat64ProtocolCounters, Nat64::Translator::ProtocolCounters);
#endif

} // namespace ot

#endif // NAT64_TRANSLATOR_HPP_
