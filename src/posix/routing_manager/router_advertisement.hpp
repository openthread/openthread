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
 *   This file includes definitions for IPv6 Router Advertisement.
 *
 * See RFC 4861: Neighbor Discovery for IP version 6 (https://tools.ietf.org/html/rfc4861).
 *
 */

#ifndef POSIX_ROUTER_ADVERTISEMENT_HPP_
#define POSIX_ROUTER_ADVERTISEMENT_HPP_

#include "openthread-posix-config.h"

#if OPENTHREAD_CONFIG_DUCKHORN_BORDER_ROUTER_ENABLE

#include <memory.h>
#include <stdint.h>

#include <netinet/icmp6.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <openthread/icmp6.h>
#include <openthread/openthread-system.h>
#include <openthread/platform/toolchain.h>

#include "infra_netif.hpp"

#include "common/encoding.hpp"

#ifndef ND_OPT_ROUTE_INFORMATION
#define ND_OPT_ROUTE_INFORMATION 24
#endif

using ot::Encoding::BigEndian::HostSwap16;
using ot::Encoding::BigEndian::HostSwap32;

namespace ot {

namespace Posix {

namespace RouterAdvertisement {

static constexpr uint32_t kInfiniteLifetime = 0xffffffff;

/**
 * This class represents the variable length options in Neighbor
 * Discovery messages.
 *
 * See PrefixInfoOption
 * See RouteInfoOption
 *
 */
OT_TOOL_PACKED_BEGIN
class Option
{
public:
    /**
     * This constructor initializes the option with given type and length.
     *
     * @param[in]  aType    The type of this option.
     * @param[in]  aLength  The length of this option in unit of 8 octets.
     *
     */
    explicit Option(uint8_t aType, uint8_t aLength = 0)
        : mType(aType)
        , mLength(aLength)
    {
    }

    /**
     * This method returns the type of this option.
     *
     * @return  The option type.
     *
     */
    uint8_t GetType() const { return mType; }

    /**
     * This method sets the length of the option (in bytes).
     *
     * Since the option must end on their natural 64-bits boundaries,
     * the actual length set to the option is padded to (aLength + 7) / 8 * 8.
     *
     * @param[in]  aLength  The length of the option in unit of 1 byte.
     *
     */
    void SetLength(uint16_t aLength) { mLength = (aLength + 7) / 8; }

    /**
     * This method returns the length of the option (in bytes).
     *
     * @return  The length of the option.
     *
     */
    uint16_t GetLength() const { return mLength * 8; }

    /**
     * This methods returns the starting address of the next option if exists.
     *
     * @return  the address of the next option.
     *
     * @note  This method has no guarantee of whether the returned address is accessible.
     *
     */
    const Option *GetNextOption() const
    {
        return reinterpret_cast<const Option *>(reinterpret_cast<const uint8_t *>(this) + GetLength());
    }

private:
    uint8_t mType;   ///< Type of the option.
    uint8_t mLength; ///< Length of the option in unit of 8 octets,
                     ///< including the `type` and `length` fields.
} OT_TOOL_PACKED_END;

/**
 * This class represents the Prefix Information Option.
 *
 * See section 4.6.2 of RFC 4861 for definition of this option.
 * https://tools.ietf.org/html/rfc4861#section-4.6.2
 *
 */
OT_TOOL_PACKED_BEGIN
class PrefixInfoOption : public Option
{
public:
    /**
     * This constructor initializes this option with zero prefix length
     * and lifetime.
     *
     */
    PrefixInfoOption()
        : Option(ND_OPT_PREFIX_INFORMATION, 4)
        , mPrefixLength(0)
        , mReserved1(0)
        , mValidLifetime(0)
        , mPreferredLifetime(0)
        , mReserved2(0)
    {
        memset(mPrefix, 0, sizeof(mPrefix));
    }

    /**
     * This method sets the on on-link (L) flag.
     *
     * @param[in]  aOnLink  A boolean indicates whether the prefix is on-link or off-link.
     *
     */
    void SetOnLink(bool aOnLink)
    {
        if (aOnLink)
        {
            mReserved1 |= ND_OPT_PI_FLAG_ONLINK;
        }
        else
        {
            mReserved1 &= ~ND_OPT_PI_FLAG_ONLINK;
        }
    }

    /**
     * This method sets the autonomous address-configuration (A) flag.
     *
     * @param[in]  aAutoAddrConfig  A boolean indicates whether this prefix can be used
     *                          for SLAAC.
     *
     */
    void SetAutoAddrConfig(bool aAutoAddrConfig)
    {
        if (aAutoAddrConfig)
        {
            mReserved1 |= ND_OPT_PI_FLAG_AUTO;
        }
        else
        {
            mReserved1 &= ~ND_OPT_PI_FLAG_AUTO;
        }
    }

    /**
     * This method set the valid lifetime of the prefix in seconds.
     *
     * @param[in]  aValidLifetime  The valid lifetime in seconds.
     *
     */
    void SetValidLifetime(uint32_t aValidLifetime) { mValidLifetime = HostSwap32(aValidLifetime); }

    /**
     * THis method returns the valid lifetime of the prefix in seconds.
     *
     * @return  The valid lifetime in seconds.
     *
     */
    uint32_t GetValidLifetime() const { return HostSwap32(mValidLifetime); }

    /**
     * This method sets the preferred lifetime of the prefix in seconds.
     *
     * @param[in]  aPreferredLifetime  The preferred lifetime in seconds.
     *
     */
    void SetPreferredLifetime(uint32_t aPreferredLifetime) { mPreferredLifetime = HostSwap32(aPreferredLifetime); }

    /**
     * This method sets the prefix.
     *
     * @param[in]  aPrefix  The prefix contained in this option.
     *
     */
    void SetPrefix(const otIp6Prefix &aPrefix)
    {
        mPrefixLength = aPrefix.mLength;
        memcpy(mPrefix, &aPrefix.mPrefix, sizeof(aPrefix.mPrefix));
    }

    /**
     * This method returns the prefix length in unit bits.
     *
     * @return  The prefix length in bits.
     *
     */
    uint8_t GetPrefixLength() const { return mPrefixLength; }

    /**
     * This method returns a pointer to the prefix.
     *
     * @return  The pointer to the prefix.
     *
     */
    const uint8_t *GetPrefix() const { return mPrefix; }

private:
    uint8_t  mPrefixLength;      ///< The prefix length in bits.
    uint8_t  mReserved1;         ///< The reserved field.
    uint32_t mValidLifetime;     ///< The valid lifetime of the prefix.
    uint32_t mPreferredLifetime; ///< The preferred lifetime of the prefix.s
    uint32_t mReserved2;         ///< The reserved field.
    uint8_t  mPrefix[16];        ///< The prefix.
} OT_TOOL_PACKED_END;

/**
 * This class represents the Route Information Option.
 *
 * See section 2.3 of RFC 4191 for definition of this option.
 * https://tools.ietf.org/html/rfc4191#section-2.3
 *
 */
OT_TOOL_PACKED_BEGIN
class RouteInfoOption : public Option
{
public:
    /**
     * This constructor initializes this option with zero prefix length.
     *
     */
    RouteInfoOption()
        : Option(ND_OPT_ROUTE_INFORMATION, 0)
        , mPrefixLength(0)
        , mReserved(0)
        , mRouteLifetime(0)
    {
        memset(mPrefix, 0, sizeof(mPrefix));
    }

    /**
     * This method sets the lifetime of the route in seconds.
     *
     * @param[in]  aLifetime  The lifetime of the route in seconds.
     *
     */
    void SetRouteLifetime(uint32_t aLifetime) { mRouteLifetime = HostSwap32(aLifetime); }

    /**
     * This method sets the prefix.
     *
     * @param[in]  aPrefix  The prefix contained in this option.
     *
     */
    void SetPrefix(const otIp6Prefix &aPrefix)
    {
        SetLength(((aPrefix.mLength + 63) / 64) * 8 + 8);

        mPrefixLength = aPrefix.mLength;
        memcpy(mPrefix, &aPrefix.mPrefix, sizeof(aPrefix.mPrefix));
    }

private:
    uint8_t  mPrefixLength;  ///< The prefix length in bits.
    uint8_t  mReserved;      ///< The reserved field.
    uint32_t mRouteLifetime; ///< The lifetime in seconds.
    uint8_t  mPrefix[16];    ///< The prefix.
} OT_TOOL_PACKED_END;

/**
 * This class implements the base class for Router Advertisement messages.
 *
 * Two additional bytes are added before the head of the message to indicate
 * the length of a Router Advertisement message. We need this because a Router
 * Advertisement message may include one or more options, we need a way know
 * where the message ends.
 *
 */
OT_TOOL_PACKED_BEGIN
class MessageBase
{
public:
    /**
     * This constructor initialize the base message with given length.
     *
     * @param  aLength  The length of the message.
     *
     */
    explicit MessageBase(uint16_t aLength = 0)
        : mLength(aLength)
    {
    }

    /**
     * This method sets the length of the message in bytes.
     *
     * @param  aLength  The message length in bytes.
     *
     */
    void SetLength(uint16_t aLength) { mLength = aLength; }

    /**
     * This method returns the length of the message in bytes.
     *
     * @return  The length of the message in bytes.
     *
     */
    uint16_t GetLength() const { return mLength; }

    /**
     * This method returns the length of the message plus the additional
     * two bytes `length` field before the head of the message.
     *
     * @return  GetLength() + 2.
     *
     */
    uint16_t GetTotalLength() const { return sizeof(*this) + GetLength(); }

    /**
     * This method returns a const pointer to the beginning of the message.
     *
     * @return  A const pointer to the beginning of the message.
     *
     */
    uint8_t *GetBegin() { return mBegin; }

    /**
     * This method returns a pointer to the beginning of the message.
     *
     * @return  A pointer to the beginning of the message.
     *
     */
    const uint8_t *GetBegin() const { return mBegin; }

private:
    uint16_t mLength;   ///< The length of the message in bytes.
    uint8_t  mBegin[0]; ///< The beginning of the message.
} OT_TOOL_PACKED_END;

/**
 * This class implements a fixed-length message buffer for
 * Router Advertisement messages.
 *
 * @see  MessageBuffer::kMaxLength
 *
 */
OT_TOOL_PACKED_BEGIN
class MessageBuffer : public MessageBase
{
public:
    static constexpr uint16_t kMaxLength = 1500; ///< The maximum length of the buffer.

    /**
     * This method appends raw bytes to the end of the message buffer.
     *
     * @param[in]  aData    The raw data to be appended.
     * @param[in]  aLength  The length of the data.
     *
     */
    void Append(const void *aData, uint16_t aLength)
    {
        memcpy(GetBegin() + GetLength(), aData, aLength);
        SetLength(GetLength() + aLength);
    }

    /**
     * This method appends a message to the end of the message buffer.
     *
     * @param[in]  aMessage  The message to be appended.
     *
     */
    void Append(const MessageBase &aMessage) { Append(aMessage.GetBegin(), aMessage.GetLength()); }

    /**
     * This method appends an option to the end of the message buffer.
     *
     * @param[in]  aOption  The option to be appended.
     *
     */
    void Append(const Option &aOption) { Append(&aOption, aOption.GetLength()); }

private:
    uint8_t mBuffer[kMaxLength]; ///< the buffer to hold the message.
} OT_TOOL_PACKED_END;

/**
 * This class implements the Router Advertisement message.
 *
 * Only the fixed-length ICMP fields are included in this class. A Router
 * Advertisement message may include zero or multiple options and it is not
 * practical to preserve spaces for them. The caller can get all the possible
 * options with GetNextPrefixInfo.
 *
 * This class is typically used to reinterpret the content in a message buffer
 * when the Type field indicates it is a Router Advertisement message. An example
 * could be:
 *
 * len = recv(messageBuffer.GetBegin(), messageBuffer::kMaxLength);
 * if (len > 0 && messageBuffer.GetBegin()[0] == ND_ROUTER_ADVERT)
 * {
 *   routerAdv = reinterpret_cast<const RouterAdvMessage*>(&messageBuffer);
 *   ...
 * }
 *
 */
OT_TOOL_PACKED_BEGIN
class RouterAdvMessage : public MessageBase
{
public:
    /**
     * This constructor initializes the Router Advertisement message with
     * zero reachable time and retransmission timer.
     *
     */
    RouterAdvMessage()
        : MessageBase(sizeof(*this) - sizeof(MessageBase))
        , mReachableTime(0)
        , mRetransTimer(0)
    {
        memset(&mHeader, 0, sizeof(mHeader));
        mHeader.mType = ND_ROUTER_ADVERT;
        mHeader.mCode = 0;
    }

    /**
     * This method returns the next prefix information option in this message.
     *
     * An example of how to use this method to iterate over all the prefix
     * information options:
     *
     *  PrefixInfoOption *pio = nullptr;
     *  while ((pio = routerAdv->GetNextOption(pio)) != nullptr)
     *  {
     *    ...
     *  }
     *
     * @param[in]  aCurPrefixInfo  The current prefix information option.
     *
     * @return  The pointer to the next prefix information option if exists. Otherwise NULL.
     *
     */
    const PrefixInfoOption *GetNextPrefixInfo(const PrefixInfoOption *aCurPrefixInfo) const;

private:
    /**
     * This method returns the next option in this message.
     *
     * @param[in]  aCurOption  The current option.
     *
     * @return  The pointer to the next option if exists. Otherwise, NULL.
     *
     */
    const Option *GetNextOption(const Option *aCurOption) const;

    otIcmp6Header mHeader;        ///< The common ICMPv6 header.
    uint32_t      mReachableTime; ///< The reachable time.
    uint32_t      mRetransTimer;  ///< The retransmission timer.
    uint8_t       mOptions[0];    ///< The beginning of the options.
} OT_TOOL_PACKED_END;

/**
 * This class sends and receives Router Solicitation, Advertisement messages.
 *
 */
class RouterAdvertiser
{
public:
    /**
     * This function handles a Router Solicitation message.
     *
     * @param[in]  aIfIndex  The index of the interface on which the message is received.
     * @param[in]  aContext  The arbitrary context data associated with the handler.
     *
     */
    typedef void (*RouterSolicitHandler)(unsigned int aIfIndex, void *aContext);

    /**
     * This function handles a Router Advertisement message.
     *
     * @param[in]  aRouterAdv  The Router Advertisement message.
     * @param[in]  aIfIndex    The index of the interface on which the message is received.
     * @param[in]  aContext    The arbitrary context data associated with the handler.
     *
     */
    typedef void (*RouterAdvertisementHandler)(const RouterAdvMessage &aRouterAdv,
                                               unsigned int            aIfIndex,
                                               void *                  aContext);

    /**
     * This constructor initializes the object with given Router Solicitation
     * and Advertisement handlers.
     *
     * @param[in]  aRouterSolicitHandler               A Router Solicitation message handler.
     * @param[in]  aRouterSolicitHandlerContext        An arbitrary context data associated
     *                                                 with @p aRouterSolicitHandler.
     * @param[in]  aRouterAdvertisementHandler         A Router Advertisement message handler.
     * @param[in]  aRouterAdvertisementHandlerContext  An arbitrary context data associated
     *                                                 with @p aRouterAdvertisementHandler.
     *
     */
    RouterAdvertiser(RouterSolicitHandler       aRouterSolicitHandler,
                     void *                     aRouterSolicitHandlerContext,
                     RouterAdvertisementHandler aRouterAdvertisementHandler,
                     void *                     aRouterAdvertisementHandlerContext);

    /**
     * This method initializes the object.
     *
     */
    void Init();

    /**
     * This method deinitializes the object.
     *
     */
    void Deinit();

    /**
     * This method updates a mainloop context.
     *
     * @param[in]  aMainloop  A mainloop context to be updated.
     *
     */
    void Update(otSysMainloopContext *aMainloop);

    /**
     * This method process events in a mainloop context.
     *
     * @param[in]  aMainloop  A mainloop context to be processed.
     *
     */
    void Process(const otSysMainloopContext *aMainloop);

    /**
     * This method sends a Router Advertisement message with given OMR
     * and on-link prefix.
     *
     * @param[in]  aOmrPrefix     An OMR prefix.
     * @param[in]  aOnLinkPrefix  An on-link prefix.
     * @param[in]  aInfraNetif    An infrastructure network interface to send the message.
     * @param[in]  aDest          A destination address to send the message.
     *
     */
    void SendRouterAdvertisement(const otIp6Prefix *    aOmrPrefix,
                                 const otIp6Prefix *    aOnLinkPrefix,
                                 const InfraNetif &     aInfraNetif,
                                 const struct in6_addr &aDest);

    /**
     * This method sends a Router Solicitation message.
     *
     * @param[in]  aInfraNetif  An infrastructure network interface to send the message.
     * @param[in]  aDest        A destination address to send the message.
     *
     */
    void SendRouterSolicit(const InfraNetif &aInfraNetif, const struct in6_addr &aDest);

private:
    void HandleRouterSolicit(unsigned int           aIfIndex,
                             const MessageBuffer &  aBuffer,
                             const struct in6_addr &aSrcAddr,
                             const struct in6_addr &aDstAddr);
    void HandleRouterAdvertisement(unsigned int           aIfIndex,
                                   const MessageBuffer &  aBuffer,
                                   const struct in6_addr &aSrcAddr,
                                   const struct in6_addr &aDstAddr);

    /**
     * This method sends a message on given network interface.
     *
     * @param[in]  aMessageBuffer
     * @param[in]  aInfraNetif
     * @param[in]  aDest
     *
     * @retval  OT_ERROR_NONE    Successfully sent the message.
     * @retval  OT_ERROR_FAILED  Failed to send the message.
     */
    otError Send(MessageBuffer &aMessageBuffer, const InfraNetif &aInfraNetif, const struct in6_addr &aDest);

    /**
     * This method receives ICMP messages.
     *
     */
    void Recv();

    int                        mSocketFd;                          ///< The ICMP socket fd.
    RouterSolicitHandler       mRouterSolicitHandler;              ///< The Router Solicitation handler.
    void *                     mRouterSolicitHandlerContext;       ///< The context of the Router Solicitation handler.
    RouterAdvertisementHandler mRouterAdvertisementHandler;        ///< The Router Advertisement handler.
    void *                     mRouterAdvertisementHandlerContext; ///< The context of the Router Advertisement handler.
};

} // namespace RouterAdvertisement

} // namespace Posix

} // namespace ot

#endif // OPENTHREAD_CONFIG_DUCKHORN_BORDER_ROUTER_ENABLE

#endif // POSIX_ROUTER_ADVERTISEMENT_HPP_
