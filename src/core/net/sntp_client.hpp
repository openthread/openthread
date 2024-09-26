/*
 *  Copyright (c) 2018, The OpenThread Authors.
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

#ifndef SNTP_CLIENT_HPP_
#define SNTP_CLIENT_HPP_

#include "openthread-core-config.h"

#if OPENTHREAD_CONFIG_SNTP_CLIENT_ENABLE

#include <openthread/sntp.h>

#include "common/clearable.hpp"
#include "common/message.hpp"
#include "common/non_copyable.hpp"
#include "common/timer.hpp"
#include "net/ip6.hpp"
#include "net/netif.hpp"

/**
 * @file
 *   This file includes definitions for the SNTP client.
 */

namespace ot {
namespace Sntp {

/**
 * Implements SNTP client.
 */
class Client : private NonCopyable
{
public:
    typedef otSntpResponseHandler ResponseHandler; ///< Response handler callback.

    /**
     * Initializes the object.
     *
     * @param[in]  aInstance     A reference to the OpenThread instance.
     */
    explicit Client(Instance &aInstance);

    /**
     * Starts the SNTP client.
     *
     * @retval kErrorNone     Successfully started the SNTP client.
     * @retval kErrorAlready  The socket is already open.
     */
    Error Start(void);

    /**
     * Stops the SNTP client.
     *
     * @retval kErrorNone  Successfully stopped the SNTP client.
     */
    Error Stop(void);

    /**
     * Returns the unix era number.
     *
     * @returns The unix era number.
     */
    uint32_t GetUnixEra(void) const { return mUnixEra; }

    /**
     * Sets the unix era number.
     *
     * @param[in]  aUnixEra  The unix era number.
     */
    void SetUnixEra(uint32_t aUnixEra) { mUnixEra = aUnixEra; }

    /**
     * Sends an SNTP query.
     *
     * @param[in]  aQuery    A pointer to specify SNTP query parameters.
     * @param[in]  aHandler  A function pointer that shall be called on response reception or time-out.
     * @param[in]  aContext  A pointer to arbitrary context information.
     *
     * @retval kErrorNone         Successfully sent SNTP query.
     * @retval kErrorNoBufs       Failed to allocate retransmission data.
     * @retval kErrorInvalidArgs  Invalid arguments supplied.
     */
    Error Query(const otSntpQuery *aQuery, ResponseHandler aHandler, void *aContext);

private:
    static constexpr uint32_t kTimeAt1970 = 2208988800UL; // num seconds between 1st Jan 1900 and 1st Jan 1970.

    static constexpr uint32_t kResponseTimeout = OPENTHREAD_CONFIG_SNTP_CLIENT_RESPONSE_TIMEOUT;
    static constexpr uint8_t  kMaxRetransmit   = OPENTHREAD_CONFIG_SNTP_CLIENT_MAX_RETRANSMIT;

    OT_TOOL_PACKED_BEGIN
    class Header : public Clearable<Header>
    {
    public:
        enum Mode : uint8_t
        {
            kModeClient = 3,
            kModeServer = 4,
        };

        static constexpr uint8_t kKissCodeLength = 4;

        void Init(void)
        {
            Clear();
            mFlags = (kNtpVersion << kVersionOffset | kModeClient << kModeOffset);
        }

        uint8_t GetFlags(void) const { return mFlags; }
        void    SetFlags(uint8_t aFlags) { mFlags = aFlags; }

        Mode GetMode(void) const { return static_cast<Mode>((mFlags & kModeMask) >> kModeOffset); }

        uint8_t GetStratum(void) const { return mStratum; }
        void    SetStratum(uint8_t aStratum) { mStratum = aStratum; }

        uint8_t GetPoll(void) const { return mPoll; }
        void    SetPoll(uint8_t aPoll) { mPoll = aPoll; }

        uint8_t GetPrecision(void) const { return mPrecision; }
        void    SetPrecision(uint8_t aPrecision) { mPrecision = aPrecision; }

        uint32_t GetRootDelay(void) const { return BigEndian::HostSwap32(mRootDelay); }
        void     SetRootDelay(uint32_t aRootDelay) { mRootDelay = BigEndian::HostSwap32(aRootDelay); }

        uint32_t GetRootDispersion(void) const { return BigEndian::HostSwap32(mRootDispersion); }
        void SetRootDispersion(uint32_t aRootDispersion) { mRootDispersion = BigEndian::HostSwap32(aRootDispersion); }

        uint32_t GetReferenceId(void) const { return BigEndian::HostSwap32(mReferenceId); }
        void     SetReferenceId(uint32_t aReferenceId) { mReferenceId = BigEndian::HostSwap32(aReferenceId); }

        char *GetKissCode(void) { return reinterpret_cast<char *>(&mReferenceId); }

        uint32_t GetReferenceTimestampSeconds(void) const { return BigEndian::HostSwap32(mReferenceTimestampSeconds); }
        void     SetReferenceTimestampSeconds(uint32_t aTimestamp)
        {
            mReferenceTimestampSeconds = BigEndian::HostSwap32(aTimestamp);
        }

        uint32_t GetReferenceTimestampFraction(void) const
        {
            return BigEndian::HostSwap32(mReferenceTimestampFraction);
        }
        void SetReferenceTimestampFraction(uint32_t aFraction)
        {
            mReferenceTimestampFraction = BigEndian::HostSwap32(aFraction);
        }

        uint32_t GetOriginateTimestampSeconds(void) const { return BigEndian::HostSwap32(mOriginateTimestampSeconds); }
        void     SetOriginateTimestampSeconds(uint32_t aTimestamp)
        {
            mOriginateTimestampSeconds = BigEndian::HostSwap32(aTimestamp);
        }

        uint32_t GetOriginateTimestampFraction(void) const
        {
            return BigEndian::HostSwap32(mOriginateTimestampFraction);
        }
        void SetOriginateTimestampFraction(uint32_t aFraction)
        {
            mOriginateTimestampFraction = BigEndian::HostSwap32(aFraction);
        }

        uint32_t GetReceiveTimestampSeconds(void) const { return BigEndian::HostSwap32(mReceiveTimestampSeconds); }
        void     SetReceiveTimestampSeconds(uint32_t aTimestamp)
        {
            mReceiveTimestampSeconds = BigEndian::HostSwap32(aTimestamp);
        }

        uint32_t GetReceiveTimestampFraction(void) const { return BigEndian::HostSwap32(mReceiveTimestampFraction); }
        void     SetReceiveTimestampFraction(uint32_t aFraction)
        {
            mReceiveTimestampFraction = BigEndian::HostSwap32(aFraction);
        }

        uint32_t GetTransmitTimestampSeconds(void) const { return BigEndian::HostSwap32(mTransmitTimestampSeconds); }
        void     SetTransmitTimestampSeconds(uint32_t aTimestamp)
        {
            mTransmitTimestampSeconds = BigEndian::HostSwap32(aTimestamp);
        }

        uint32_t GetTransmitTimestampFraction(void) const { return BigEndian::HostSwap32(mTransmitTimestampFraction); }
        void     SetTransmitTimestampFraction(uint32_t aFraction)
        {
            mTransmitTimestampFraction = BigEndian::HostSwap32(aFraction);
        }

    private:
        static constexpr uint8_t kNtpVersion    = 4;                      // Current NTP version.
        static constexpr uint8_t kLeapOffset    = 6;                      // Leap Indicator field offset.
        static constexpr uint8_t kLeapMask      = 0x03 << kLeapOffset;    // Leap Indicator field mask.
        static constexpr uint8_t kVersionOffset = 3;                      // Version field offset.
        static constexpr uint8_t kVersionMask   = 0x07 << kVersionOffset; // Version field mask.
        static constexpr uint8_t kModeOffset    = 0;                      // Mode field offset.
        static constexpr uint8_t kModeMask      = 0x07 << kModeOffset;    // Mode filed mask.

        uint8_t  mFlags;                      // SNTP flags: LI Leap Indicator, VN Version Number and Mode.
        uint8_t  mStratum;                    // Packet Stratum.
        uint8_t  mPoll;                       // Maximum interval between successive messages, in log2 seconds.
        uint8_t  mPrecision;                  // The precision of the system clock, in log2 seconds.
        uint32_t mRootDelay;                  // Total round-trip delay to the reference clock, in NTP short format.
        uint32_t mRootDispersion;             // Total dispersion to the reference clock.
        uint32_t mReferenceId;                // ID identifying the particular server or reference clock.
        uint32_t mReferenceTimestampSeconds;  // Time the system clock was last set or corrected (NTP format).
        uint32_t mReferenceTimestampFraction; // Fraction part of above value.
        uint32_t mOriginateTimestampSeconds;  // Time at client when request departed for the server (NTP format).
        uint32_t mOriginateTimestampFraction; // Fraction part of above value.
        uint32_t mReceiveTimestampSeconds;    // Time at server when request arrived from the client (NTP format).
        uint32_t mReceiveTimestampFraction;   // Fraction part of above value.
        uint32_t mTransmitTimestampSeconds;   // Time at server when the response left for the client (NTP format).
        uint32_t mTransmitTimestampFraction;  // Fraction part of above value.
    } OT_TOOL_PACKED_END;

    struct QueryMetadata : public Message::FooterData<QueryMetadata>
    {
        uint32_t                  mTransmitTimestamp;   // Time at client when request departed for server
        Callback<ResponseHandler> mResponseHandler;     // Response handler callback
        TimeMilli                 mTransmissionTime;    // Time when the timer should shoot for this message
        Ip6::Address              mSourceAddress;       // Source IPv6 address
        Ip6::Address              mDestinationAddress;  // Destination IPv6 address
        uint16_t                  mDestinationPort;     // Destination UDP port
        uint8_t                   mRetransmissionCount; // Number of retransmissions
    };

    Message *NewMessage(const Header &aHeader);
    Message *CopyAndEnqueueMessage(const Message &aMessage, const QueryMetadata &aQueryMetadata);
    void     DequeueMessage(Message &aMessage);
    Error    SendMessage(Message &aMessage, const Ip6::MessageInfo &aMessageInfo);
    void     SendCopy(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo);

    Message *FindRelatedQuery(const Header &aResponseHeader, QueryMetadata &aQueryMetadata);
    void FinalizeSntpTransaction(Message &aQuery, const QueryMetadata &aQueryMetadata, uint64_t aTime, Error aResult);

    void HandleRetransmissionTimer(void);
    void HandleUdpReceive(Message &aMessage, const Ip6::MessageInfo &aMessageInfo);

    using RetxTimer    = TimerMilliIn<Client, &Client::HandleRetransmissionTimer>;
    using ClientSocket = Ip6::Udp::SocketIn<Client, &Client::HandleUdpReceive>;

    ClientSocket mSocket;
    MessageQueue mPendingQueries;
    RetxTimer    mRetransmissionTimer;

    uint32_t mUnixEra;
};

} // namespace Sntp
} // namespace ot

#endif // OPENTHREAD_CONFIG_SNTP_CLIENT_ENABLE

#endif // SNTP_CLIENT_HPP_
