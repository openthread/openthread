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
 *
 */

#ifndef NAT64_TRANSLATOR_HPP_
#define NAT64_TRANSLATOR_HPP_

#include "openthread-core-config.h"

#include "common/array.hpp"
#include "common/linked_list.hpp"
#include "common/locator.hpp"
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
 *
 */
const char *StateToString(State aState);

#if OPENTHREAD_CONFIG_NAT64_TRANSLATOR_ENABLE

/**
 * Implements the NAT64 translator.
 *
 */
class Translator : public InstanceLocator, private NonCopyable
{
public:
    static constexpr uint32_t kAddressMappingIdleTimeoutMsec =
        OPENTHREAD_CONFIG_NAT64_IDLE_TIMEOUT_SECONDS * Time::kOneSecondInMsec;
    static constexpr uint32_t kAddressMappingPoolSize = OPENTHREAD_CONFIG_NAT64_MAX_MAPPINGS;

    typedef otNat64AddressMappingIterator AddressMappingIterator; ///< Address mapping Iterator.

    /**
     * The possible results of NAT64 translation.
     *
     */
    enum Result : uint8_t
    {
        kNotTranslated, ///< The message is not translated, it might be sending to an non-nat64 prefix (for outgoing
                        ///< datagrams), or it is already an IPv6 message (for incoming datagrams).
        kForward,       ///< Message is successfully translated, the caller should continue forwarding the translated
                        ///< datagram.
        kDrop,          ///< The caller should drop the datagram silently.
    };

    /**
     * Represents the counters for the protocols supported by NAT64.
     *
     */
    class ProtocolCounters : public otNat64ProtocolCounters, public Clearable<ProtocolCounters>
    {
    public:
        /**
         * Adds the packet to the counter for the given IPv6 protocol.
         *
         * @param[in] aProtocol    The protocol of the packet.
         * @param[in] aPacketSize  The size of the packet.
         *
         */
        void Count6To4Packet(uint8_t aProtocol, uint64_t aPacketSize);

        /**
         * Adds the packet to the counter for the given IPv4 protocol.
         *
         * @param[in] aProtocol    The protocol of the packet.
         * @param[in] aPacketSize  The size of the packet.
         *
         */
        void Count4To6Packet(uint8_t aProtocol, uint64_t aPacketSize);
    };

    /**
     * Represents the counters of dropped packets due to errors when handling NAT64 packets.
     *
     */
    class ErrorCounters : public otNat64ErrorCounters, public Clearable<otNat64ErrorCounters>
    {
    public:
        enum Reason : uint8_t
        {
            kUnknown          = OT_NAT64_DROP_REASON_UNKNOWN,
            kIllegalPacket    = OT_NAT64_DROP_REASON_ILLEGAL_PACKET,
            kUnsupportedProto = OT_NAT64_DROP_REASON_UNSUPPORTED_PROTO,
            kNoMapping        = OT_NAT64_DROP_REASON_NO_MAPPING,
        };

        /**
         * Adds the counter for the given reason when translating an IPv4 datagram.
         *
         * @param[in] aReason    The reason of packet drop.
         *
         */
        void Count4To6(Reason aReason) { mCount4To6[aReason]++; }

        /**
         * Adds the counter for the given reason when translating an IPv6 datagram.
         *
         * @param[in] aReason    The reason of packet drop.
         *
         */
        void Count6To4(Reason aReason) { mCount6To4[aReason]++; }
    };

    /**
     * Initializes the NAT64 translator.
     *
     */
    explicit Translator(Instance &aInstance);

    /**
     * Set the state of NAT64 translator.
     *
     * Note: Disabling the translator will invalidate all address mappings.
     *
     * @param[in]  aEnabled   A boolean to enable/disable NAT64 translator.
     *
     */
    void SetEnabled(bool aEnabled);

    /**
     * Gets the state of NAT64 translator.
     *
     * @retval  kNat64StateDisabled  The translator is disabled.
     * @retval  kNat64StateIdle      The translator is not configured with a valid NAT64 prefix and a CIDR.
     * @retval  kNat64StateActive    The translator is translating packets.
     *
     */
    State GetState(void) const { return mState; }

    /**
     * Translates an IPv4 datagram to an IPv6 datagram and sends it via Thread interface.
     *
     * The caller transfers ownership of @p aMessage when making this call. OpenThread will free @p aMessage when
     * processing is complete, including when a value other than `kErrorNone` is returned.
     *
     * @param[in]  aMessage          A reference to the message.
     *
     * @retval kErrorNone     Successfully processed the message.
     * @retval kErrorDrop     Message was well-formed but not fully processed due to datagram processing rules.
     * @retval kErrorNoBufs   Could not allocate necessary message buffers when processing the datagram.
     * @retval kErrorNoRoute  No route to host.
     * @retval kErrorParse    Encountered a malformed header when processing the message.
     *
     */
    Error SendMessage(Message &aMessage);

    /**
     * Allocate a new message buffer for sending an IPv4 message (which will be translated into an IPv6 datagram by
     * NAT64 later). Message buffers allocated by this function will have 20 bytes (The differences between the size of
     * IPv6 headers and the size of IPv4 headers) reserved.
     *
     * @param[in]  aSettings  The message settings.
     *
     * @returns A pointer to the message buffer or NULL if no message buffers are available or parameters are invalid.
     *
     */
    Message *NewIp4Message(const Message::Settings &aSettings);

    /**
     * Translates an IPv4 datagram to IPv6 datagram. Note the datagram and datagramLength might be adjusted.
     * Note the message can have 20 bytes reserved before the message to avoid potential copy operations. If the message
     * is already an IPv6 datagram, `Result::kNotTranslated` will be returned and @p aMessage won't be modified.
     *
     * @param[in,out] aMessage the message to be processed.
     *
     * @retval kNotTranslated The message is already an IPv6 datagram. @p aMessage is not updated.
     * @retval kForward       The caller should continue forwarding the datagram.
     * @retval kDrop          The caller should drop the datagram silently.
     *
     */
    Result TranslateToIp6(Message &message);

    /**
     * Translates an IPv6 datagram to IPv4 datagram. Note the datagram and datagramLength might be adjusted.
     * If the message is not targeted to NAT64-mapped address, `Result::kNotTranslated` will be returned and @p aMessage
     * won't be modified.
     *
     * @param[in,out] aMessage the message to be processed.
     *
     * @retval kNotTranslated The datagram is not sending to the configured NAT64 prefix.
     * @retval kForward       The caller should continue forwarding the datagram.
     * @retval kDrop          The caller should drop the datagram silently.
     *
     */
    Result TranslateFromIp6(Message &aMessage);

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
     *
     */
    Error SetIp4Cidr(const Ip4::Cidr &aCidr);

    /**
     * Sets the prefix of NAT64-mapped addresses in the thread network. The address mapping table will not be cleared.
     * Equals to `ClearNat64Prefix` when an empty prefix is provided.
     *
     * @param[in] aNat64Prefix The prefix of the NAT64-mapped addresses.
     *
     */
    void SetNat64Prefix(const Ip6::Prefix &aNat64Prefix);

    /**
     * Clear the prefix of NAT64-mapped addresses in the thread network. The address mapping table will not be cleared.
     * The translator will return kNotTranslated for all IPv6 datagrams and kDrop for all IPv4 datagrams.
     *
     */
    void ClearNat64Prefix(void);

    /**
     * Initializes an `otNat64AddressMappingIterator`.
     *
     * An iterator MUST be initialized before it is used.
     *
     * An iterator can be initialized again to restart from the beginning of the mapping info.
     *
     * @param[out] aIterator  An iterator to initialize.
     *
     */
    void InitAddressMappingIterator(AddressMappingIterator &aIterator);

    /**
     * Gets the next AddressMapping info (using an iterator).
     *
     * @param[in,out]  aIterator      The iterator. On success the iterator will be updated to point to next NAT64
     *                                address mapping record. To get the first entry the iterator should be set to
     *                                OT_NAT64_ADDRESS_MAPPING_ITERATOR_INIT.
     * @param[out]     aMapping       An `otNat64AddressMapping` where information of next NAT64 address mapping record
     *                                is placed (on success).
     *
     * @retval kErrorNone      Successfully found the next NAT64 address mapping info (@p aMapping was successfully
     *                         updated).
     * @retval kErrorNotFound  No subsequent NAT64 address mapping info was found.
     *
     */
    Error GetNextAddressMapping(AddressMappingIterator &aIterator, otNat64AddressMapping &aMapping);

    /**
     * Gets the NAT64 translator counters.
     *
     * The counters are initialized to zero when the OpenThread instance is initialized.
     *
     * @param[out] aCounters A `ProtocolCounters` where the counters of NAT64 translator will be placed.
     *
     */
    void GetCounters(ProtocolCounters &aCounters) const { aCounters = mCounters; }

    /**
     * Gets the NAT64 translator error counters.
     *
     * The counters are initialized to zero when the OpenThread instance is initialized.
     *
     * @param[out] aCounters  An `ErrorCounters` where the counters of NAT64 translator will be placed.
     *
     */
    void GetErrorCounters(ErrorCounters &aCounters) const { aCounters = mErrorCounters; }

    /**
     * Gets the configured CIDR in the NAT64 translator.
     *
     * @param[out] aCidr        The `Ip4::Cidr` Where the configured CIDR will be placed.
     *
     * @retval kErrorNone       @p aCidr is set to the configured CIDR.
     * @retval kErrorNotFound   The translator is not configured with an IPv4 CIDR.
     *
     */
    Error GetIp4Cidr(Ip4::Cidr &aCidr);

    /**
     * Gets the configured IPv6 prefix in the NAT64 translator.
     *
     * @param[out] aPrefix      The `Ip6::Prefix` where the configured NAT64 prefix will be placed.
     *
     * @retval kErrorNone       @p aPrefix is set to the configured prefix.
     * @retval kErrorNotFound   The translator is not configured with an IPv6 prefix.
     *
     */
    Error GetIp6Prefix(Ip6::Prefix &aPrefix);

private:
    class AddressMapping : public LinkedListEntry<AddressMapping>
    {
    public:
        friend class LinkedListEntry<AddressMapping>;
        friend class LinkedList<AddressMapping>;

        typedef String<Ip6::Address::kInfoStringSize + Ip4::Address::kAddressStringSize + 4> InfoString;

        void       Touch(TimeMilli aNow) { mExpiry = aNow + kAddressMappingIdleTimeoutMsec; }
        InfoString ToString(void) const;
        void       CopyTo(otNat64AddressMapping &aMapping, TimeMilli aNow) const;

        uint64_t mId; // The unique id for a mapping session.

        Ip4::Address mIp4;
        Ip6::Address mIp6;
        TimeMilli    mExpiry; // The timestamp when this mapping expires, in milliseconds.

        ProtocolCounters mCounters;

    private:
        bool Matches(const Ip4::Address &aIp4) const { return mIp4 == aIp4; }
        bool Matches(const Ip6::Address &aIp6) const { return mIp6 == aIp6; }
        bool Matches(const TimeMilli aNow) const { return mExpiry < aNow; }

        AddressMapping *mNext;
    };

    Error TranslateIcmp4(Message &aMessage);
    Error TranslateIcmp6(Message &aMessage);

    uint16_t        ReleaseMappings(LinkedList<AddressMapping> &aMappings);
    void            ReleaseMapping(AddressMapping &aMapping);
    uint16_t        ReleaseExpiredMappings(void);
    AddressMapping *AllocateMapping(const Ip6::Address &aIp6Addr);
    AddressMapping *FindOrAllocateMapping(const Ip6::Address &aIp6Addr);
    AddressMapping *FindMapping(const Ip4::Address &aIp4Addr);

    void HandleMappingExpirerTimer(void);

    using MappingTimer = TimerMilliIn<Translator, &Translator::HandleMappingExpirerTimer>;

    void UpdateState(bool aAlwaysNotify = false);

    bool  mEnabled;
    State mState;

    uint64_t mNextMappingId;

    Array<Ip4::Address, kAddressMappingPoolSize>  mIp4AddressPool;
    Pool<AddressMapping, kAddressMappingPoolSize> mAddressMappingPool;
    LinkedList<AddressMapping>                    mActiveAddressMappings;

    Ip6::Prefix mNat64Prefix;
    Ip4::Cidr   mIp4Cidr;

    MappingTimer mMappingExpirerTimer;

    ProtocolCounters mCounters;
    ErrorCounters    mErrorCounters;
};
#endif // OPENTHREAD_CONFIG_NAT64_TRANSLATOR_ENABLE

} // namespace Nat64

DefineMapEnum(otNat64State, Nat64::State);
#if OPENTHREAD_CONFIG_NAT64_TRANSLATOR_ENABLE
DefineCoreType(otNat64ProtocolCounters, Nat64::Translator::ProtocolCounters);
DefineCoreType(otNat64ErrorCounters, Nat64::Translator::ErrorCounters);
#endif

} // namespace ot

#endif // NAT64_TRANSLATOR_HPP_
