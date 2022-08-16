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

#if OPENTHREAD_CONFIG_NAT64_TRANSLATOR_ENABLE

#include "common/array.hpp"
#include "common/linked_list.hpp"
#include "common/locator.hpp"
#include "common/pool.hpp"
#include "common/timer.hpp"
#include "net/ip4_types.hpp"
#include "net/ip6.hpp"

namespace ot {
namespace Nat64 {

/**
 * This class implements the NAT64 translator for thread.
 *
 */
class Translator : public InstanceLocator, private NonCopyable
{
public:
    static constexpr uint32_t kAddressMappingIdleTimeoutMsec =
        OPENTHREAD_CONFIG_NAT64_IDLE_TIMEOUT_SECONDS * Time::kOneSecondInMsec;
    static constexpr uint32_t kAddressMappingPoolSize = OPENTHREAD_CONFIG_NAT64_MAX_MAPPINGS;

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
     * This constructor initializes the NAT64 translator.
     *
     */
    explicit Translator(Instance &aInstance);

    /**
     * This method translates an IPv4 datagram to an IPv6 datagram and send it via thread interface.
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
     * is already an IPv6 datagram, Result::kNotTranslated will be returned and @p aMessage won't be modified.
     *
     * @param[in,out] aMessage the message to be processed.
     *
     * @retval kNotTranslated The message is already an IPv6 datagram. @p aMessage is not updated.
     * @retval kForward       The caller should contiue forwarding the datagram.
     * @retval kDrop          The caller should drop the datagram silently.
     *
     */
    Result TranslateToIp6(Message &message);

    /**
     * Translates an IPv6 datagram to IPv4 datagram. Note the datagram and datagramLength might be adjusted.
     * If the message is not targeted to NAT64-mapped address, Result::kNotTranslated will be returned and @p aMessage
     * won't be modified.
     *
     * @param[in,out] aMessage the message to be processed.
     *
     * @retval kNotTranslated The datagram is not sending to the configured NAT64 prefix.
     * @retval kForward       The caller should contiue forwarding the datagram.
     * @retval kDrop          The caller should drop the datagram silently.
     *
     */
    Result TranslateFromIp6(Message &aMessage);

    /**
     * This function sets the CIDR used when setting the source address of the outgoing translated IPv4 datagrams.
     * A valid CIDR must have a non-zero prefix length.
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
     * This function sets the prefix of NAT64-mapped addresses in the thread network. The address mapping table
     * will not be cleared. If an empty NAT64 prefix is set, the translator will return kNotTranslated for all IPv6
     * datagrams and kDrop for all IPv4 datagrams.
     *
     * @param[in] aNat64Prefix the prefix of the NAT64-mapped addresses.
     *
     */
    void SetNat64Prefix(const Ip6::Prefix &aNat64Prefix);

private:
    class AddressMapping : public LinkedListEntry<AddressMapping>
    {
    public:
        friend class LinkedListEntry<AddressMapping>;
        friend class LinkedList<AddressMapping>;

        typedef String<Ip6::Address::kInfoStringSize + Ip4::Address::kAddressStringSize + 4> InfoString;

        void       Touch(TimeMilli aNow) { mExpiry = aNow + kAddressMappingIdleTimeoutMsec; }
        InfoString ToString(void);

        Ip4::Address mIp4;
        Ip6::Address mIp6;
        TimeMilli    mExpiry; // The timestamp when this mapping expires, in milliseconds.

    private:
        bool Matches(const Ip4::Address &aIp4) const { return mIp4 == aIp4; }
        bool Matches(const Ip6::Address &aIp6) const { return mIp6 == aIp6; }
        bool Matches(const TimeMilli aNow) const { return mExpiry < aNow; }

        AddressMapping *mNext;
    };

    Error TranslateIcmp4(Message &aMessage);
    Error TranslateIcmp6(Message &aMessage);

    void            ReleaseMapping(AddressMapping &aMapping);
    uint16_t        ReleaseExpiredMappings(void);
    AddressMapping *AllocateMapping(const Ip6::Address &aIp6Addr);
    AddressMapping *FindOrAllocateMapping(const Ip6::Address &aIp6Addr);
    AddressMapping *FindMapping(const Ip4::Address &aIp4Addr);

    Array<Ip4::Address, kAddressMappingPoolSize>  mIp4AddressPool;
    Pool<AddressMapping, kAddressMappingPoolSize> mAddressMappingPool;
    LinkedList<AddressMapping>                    mActiveAddressMappings;

    Ip6::Prefix mNat64Prefix;
    Ip4::Cidr   mIp4Cidr;
};

} // namespace Nat64
} // namespace ot

#endif // OPENTHREAD_CONFIG_NAT64_TRANSLATOR_ENABLE

#endif // NAT64_TRANSLATOR_HPP_
