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

#ifndef OT_CORE_NET_SNTP_CLIENT_HPP_
#define OT_CORE_NET_SNTP_CLIENT_HPP_

#include "openthread-core-config.h"

#if OPENTHREAD_CONFIG_SNTP_CLIENT_ENABLE

#include <openthread/sntp.h>

#include "common/as_core_type.hpp"
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
     * Represents an SNTP Query parameters.
     */
    class QueryInfo : public otSntpQuery
    {
        friend class Client;

    private:
        bool                    IsValid(void) const { return mMessageInfo != nullptr; }
        const Ip6::MessageInfo &GetMessageInfo(void) const { return AsCoreType(mMessageInfo); }
    };

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
    Error Query(const QueryInfo &aQuery, ResponseHandler aHandler, void *aContext);

private:
    static constexpr uint32_t kTimeAt1970 = 2208988800UL; // num seconds between 1st Jan 1900 and 1st Jan 1970.

    static constexpr uint32_t kResponseTimeout = OPENTHREAD_CONFIG_SNTP_CLIENT_RESPONSE_TIMEOUT;
    static constexpr uint8_t  kMaxRetransmit   = OPENTHREAD_CONFIG_SNTP_CLIENT_MAX_RETRANSMIT;

    typedef Callback<ResponseHandler> ResponseCallback;

    OT_TOOL_PACKED_BEGIN
    class Timestamp : public Clearable<Timestamp>
    {
    public:
        bool     IsZero(void) const { return (mSeconds == 0) && (mFraction == 0); }
        uint32_t GetSeconds(void) const { return BigEndian::HostSwap32(mSeconds); }
        void     SetSeconds(uint32_t aSeconds) { mSeconds = BigEndian::HostSwap32(aSeconds); }
        uint32_t GetFraction(void) const { return BigEndian::HostSwap32(mFraction); }
        void     SetFraction(uint32_t aFraction) { mFraction = BigEndian::HostSwap32(aFraction); }

    private:
        uint32_t mSeconds;
        uint32_t mFraction;
    } OT_TOOL_PACKED_END;

    OT_TOOL_PACKED_BEGIN
    class Header : public Clearable<Header>
    {
    public:
        static constexpr uint8_t kModeClient     = 3;
        static constexpr uint8_t kModeServer     = 4;
        static constexpr uint8_t kKissCodeLength = 4;

        void             Init(void) { Clear(), mFlags = (kNtpVersion << kVersionOffset | kModeClient << kModeOffset); }
        uint8_t          GetFlags(void) const { return mFlags; }
        void             SetFlags(uint8_t aFlags) { mFlags = aFlags; }
        uint8_t          GetMode(void) const { return ReadBits<uint8_t, kModeMask>(mFlags); }
        uint8_t          GetStratum(void) const { return mStratum; }
        void             SetStratum(uint8_t aStratum) { mStratum = aStratum; }
        uint8_t          GetPoll(void) const { return mPoll; }
        void             SetPoll(uint8_t aPoll) { mPoll = aPoll; }
        uint8_t          GetPrecision(void) const { return mPrecision; }
        void             SetPrecision(uint8_t aPrecision) { mPrecision = aPrecision; }
        uint32_t         GetRootDelay(void) const { return BigEndian::HostSwap32(mRootDelay); }
        void             SetRootDelay(uint32_t aDelay) { mRootDelay = BigEndian::HostSwap32(aDelay); }
        uint32_t         GetRootDispersion(void) const { return BigEndian::HostSwap32(mRootDispersion); }
        void             SetRootDispersion(uint32_t aDisp) { mRootDispersion = BigEndian::HostSwap32(aDisp); }
        uint32_t         GetReferenceId(void) const { return BigEndian::HostSwap32(mReferenceId); }
        void             SetReferenceId(uint32_t aId) { mReferenceId = BigEndian::HostSwap32(aId); }
        char            *GetKissCode(void) { return reinterpret_cast<char *>(&mReferenceId); }
        Timestamp       &GetReferenceTimestamp(void) { return mReferenceTimestamp; }
        Timestamp       &GetOriginateTimestamp(void) { return mOriginateTimestamp; }
        Timestamp       &GetRxTimestamp(void) { return mRxTimestamp; }
        Timestamp       &GetTxTimestamp(void) { return mTxTimestamp; }
        const Timestamp &GetReferenceTimestamp(void) const { return mReferenceTimestamp; }
        const Timestamp &GetOriginateTimestamp(void) const { return mOriginateTimestamp; }
        const Timestamp &GetRxTimestamp(void) const { return mRxTimestamp; }
        const Timestamp &GetTxTimestamp(void) const { return mTxTimestamp; }

    private:
        static constexpr uint8_t kNtpVersion    = 4;                      // Current NTP version.
        static constexpr uint8_t kLeapOffset    = 6;                      // Leap Indicator field offset.
        static constexpr uint8_t kLeapMask      = 0x03 << kLeapOffset;    // Leap Indicator field mask.
        static constexpr uint8_t kVersionOffset = 3;                      // Version field offset.
        static constexpr uint8_t kVersionMask   = 0x07 << kVersionOffset; // Version field mask.
        static constexpr uint8_t kModeOffset    = 0;                      // Mode field offset.
        static constexpr uint8_t kModeMask      = 0x07 << kModeOffset;    // Mode filed mask.

        uint8_t   mFlags;              // SNTP flags: LI Leap Indicator, VN Version Number and Mode.
        uint8_t   mStratum;            // Packet Stratum.
        uint8_t   mPoll;               // Maximum interval between successive messages, in log2 seconds.
        uint8_t   mPrecision;          // The precision of the system clock, in log2 seconds.
        uint32_t  mRootDelay;          // Total round-trip delay to the reference clock, in NTP short format.
        uint32_t  mRootDispersion;     // Total dispersion to the reference clock.
        uint32_t  mReferenceId;        // ID identifying the particular server or reference clock.
        Timestamp mReferenceTimestamp; // Time the system clock was last set or corrected.
        Timestamp mOriginateTimestamp; // Time at client when request departed for the server.
        Timestamp mRxTimestamp;        // Time at server when request arrived from the client.
        Timestamp mTxTimestamp;        // Time at server when the response left for the client.
    } OT_TOOL_PACKED_END;

    struct QueryMetadata : public Message::FooterData<QueryMetadata>
    {
        uint32_t         mTxTimestamp;      // Time at client when request departed for server
        ResponseCallback mResponseCallback; // Response handler callback
        TimeMilli        mRetxTime;         // Time to retx the query (`kResponseTimeout` after last tx)
        Ip6::Address     mSourceAddr;       // Source IPv6 address
        Ip6::Address     mDestAddr;         // Destination IPv6 address
        uint16_t         mDestPort;         // Destination UDP port
        uint8_t          mRetxCount;        // Number of retransmissions
    };

    Message *CopyAndEnqueueMessage(const Message &aMessage, const QueryMetadata &aMetadata);
    void     SendCopy(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo);
    Message *FindRelatedQuery(const Header &aResponseHeader, QueryMetadata &aMetadata);
    void     Finalize(Message &aQuery, const QueryMetadata &aMetadata, uint64_t aTime, Error aResult);
    void     HandleTimer(void);
    void     HandleUdpReceive(Message &aMessage, const Ip6::MessageInfo &aMessageInfo);
    void     ProcessResponse(Message &aQuery, const QueryMetadata &aMetadata, Header &aResponseHeader);

    using RetxTimer    = TimerMilliIn<Client, &Client::HandleTimer>;
    using ClientSocket = Ip6::Udp::SocketIn<Client, &Client::HandleUdpReceive>;

    ClientSocket mSocket;
    MessageQueue mPendingQueries;
    RetxTimer    mTimer;
    uint32_t     mUnixEra;
};

} // namespace Sntp

DefineCoreType(otSntpQuery, Sntp::Client::QueryInfo);

} // namespace ot

#endif // OPENTHREAD_CONFIG_SNTP_CLIENT_ENABLE

#endif // OT_CORE_NET_SNTP_CLIENT_HPP_
