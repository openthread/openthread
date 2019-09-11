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
 *   This file contains definitions for the network performance tool.
 */

#ifndef CLI_PERF_HPP_
#define CLI_PERF_HPP_

#include "openthread-core-config.h"

#include "cli_config.h"

#include <stdio.h>
#include <openthread/error.h>
#include <openthread/udp.h>
#include <openthread/platform/toolchain.h>

#include "common/encoding.hpp"
#include "common/instance.hpp"
#include "common/timer.hpp"
#include "net/ip6_headers.hpp"
#include "net/udp6.hpp"

#if OPENTHREAD_CONFIG_CLI_PERF_ENABLE

using ot::Encoding::BigEndian::HostSwap32;
namespace ot {
namespace Cli {

class Interpreter;
class Perf;
class Session;
class Setting;

#define USEC_PER_SEC 1000000UL
#define MS_PER_SEC 1000
#define USEC_PER_MS 1000
#define COLOR_CODE_RED "\033[31m"
#define COLOR_CODE_END "\033[0m"

OT_TOOL_PACKED_BEGIN
class PerfHeader
{
public:
    /**
     * This method sets the value of time.
     *
     * @param[in] aUsec  The value of time (in microseconds).
     *
     */
    void SetTime(uint64_t aUsec)
    {
        mSec  = HostSwap32(static_cast<uint32_t>(aUsec / USEC_PER_SEC));
        mUsec = HostSwap32(static_cast<uint32_t>(aUsec % USEC_PER_SEC));
    }

    /**
     * This method gets the value of time.
     *
     * @returns The value time (in microseconds).
     *
     */
    uint64_t GetTime(void) const { return static_cast<uint64_t>(HostSwap32(mSec)) * USEC_PER_SEC + HostSwap32(mUsec); }

    /**
     * This method gets the sequence number.
     *
     * @returns The sequence number.
     *
     */
    uint32_t GetSeqNumber(void) const { return HostSwap32(mSeqNumber); }

    /**
     * This method sets the sequence number.
     *
     * @param[in]  aSeqNumber  A sequence number.
     *
     */
    void SetSeqNumber(uint32_t aSeqNumber) { mSeqNumber = HostSwap32(aSeqNumber); }

    /**
     * This method gets the sending interval.
     *
     * @returns The sending interval (in microseconds).
     *
     */
    uint32_t GetSendingInterval(void) const { return HostSwap32(mSendingIntervalUs); }

    /**
     * This method sets the sending interval.
     *
     * @param[in]  aInterval  A sending interval (in microseconds).
     *
     */
    void SetSendingInterval(uint32_t aInterval) { mSendingIntervalUs = HostSwap32(aInterval); }

    /**
     * This method gets the fin delay.
     *
     * @returns The fin delay (in milliseconds).
     *
     */
    uint8_t GetFinDelay(void) const { return mFinDelay; }

    /**
     * This method sets the fin delay.
     *
     * @param[in]  aFinDelay  The fin delay (in milliseconds).
     *
     */
    void SetFinDelay(uint8_t aFinDelay) { mFinDelay = aFinDelay; }

    /**
     * This method gets the session identifier value.
     *
     * @returns The session identifier.
     *
     */
    uint8_t GetSessionId(void) const { return mSessionId; }

    /**
     * This method sets the session identifier value.
     *
     * @param[in]  aSessionId  A session identifier.
     *
     */
    void SetSessionId(uint8_t aSessionId) { mSessionId = aSessionId; }

    /**
     * This method gets the fin flag.
     *
     * @returns The fin flag.
     *
     */
    bool GetFinFlag(void) const { return mFinFlag; }

    /**
     * This method sets the fin flag.
     *
     * @param[in]  aFin  The fin flag.
     *
     */
    void SetFinFlag(bool aFinFlag) { mFinFlag = aFinFlag; }

    /**
     * This method gets the echo flag.
     *
     * @returns The echo flag.
     *
     */
    bool GetEchoFlag(void) const { return mEchoFlag; }

    /**
     * This method sets the echo flag.
     *
     * @param[in]  aEchoFlag  The echo flag.
     *
     */
    void SetEchoFlag(bool aEchoFlag) { mEchoFlag = aEchoFlag; }

    /**
     * This method sets the synchronization flag.
     *
     * @param[in]  aSyncFlag  The synchronization flag.
     *
     */
    void SetSyncFlag(bool aSyncFlag) { mSyncFlag = aSyncFlag; }

    /**
     * This method gets the synchronization flag.
     *
     * @returns The synchronization flag.
     *
     */
    bool GetSyncFlag(void) const { return mSyncFlag; }

    /**
     * This method sets the reply flag.
     *
     * @param[in]  aReplyFlag  The reply flag.
     *
     */
    void SetReplyFlag(bool aReplyFlag) { mReplyFlag = aReplyFlag; }

    /**
     * This method gets the reply flag.
     *
     * @returns The reply flag.
     *
     */
    bool GetReplyFlag(void) const { return mReplyFlag; }

private:
    uint32_t mSec;
    uint32_t mUsec;
    uint32_t mSeqNumber;
    uint32_t mSendingIntervalUs;
    uint8_t  mSessionId;
    uint8_t  mFinDelay;
    bool     mFinFlag : 1;
    bool     mEchoFlag : 1;
    bool     mReplyFlag : 1;
    bool     mSyncFlag : 1;
} OT_TOOL_PACKED_END;

/**
 * This class implements the ListNode object.
 *
 */
template <typename Type> class ListNode
{
public:
    /**
     * This constructor initializes the ListNode object.
     *
     */
    ListNode()
        : mNext(NULL)
    {
    }

    /**
     * This method sets the poiner of the next node.
     *
     * @param[in]  aNode  A pointer to the next node.
     *
     */
    void SetNext(Type *aNode) { mNext = aNode; }

    /**
     * This method gets the poiner of the nest node.
     *
     * @retval A pointer to the next node.
     *
     */
    Type *GetNext(void) const { return mNext; }

private:
    Type *mNext;
};

/**
 * This class implements the List object.
 *
 */
template <typename Type> class List
{
public:
    /**
     * This constructor initializes the List object.
     *
     */
    List()
        : mHead(NULL)
    {
    }

    /**
     * This method adds the node to the List.
     *
     * @param[in]  aNode  A reference to the node.
     *
     */
    void Add(Type &aNode)
    {
        aNode.SetNext(mHead);
        mHead = &aNode;
    }

    /**
     * This method removes the node from the List.
     *
     * @param[in]  aNode  A reference to the node.
     *
     */
    void Remove(Type &aNode);

    /**
     * This method returns the header of the List.
     *
     * @retval  A pointer to the header of the list.
     *
     */
    Type *GetHead(void) const { return mHead; }

private:
    Type *mHead;
};

/**
 * This class implements the Setting object.
 *
 */
class Setting : public ListNode<Setting>
{
public:
    /**
     * This enumeration defines the setting flags.
     *
     */
    enum
    {
        kFlagValid       = 1,       ///< Indicates whether the Setting is valid.
        kFlagClient      = 1 << 1,  ///< Indicates whether the type of the Setting is a client.
        kFlagEcho        = 1 << 2,  ///< Indicates whether the echo flag is set.
        kFlagBandwidth   = 1 << 3,  ///< Indicates whether the bandwidth is set.
        kFlagLength      = 1 << 4,  ///< Indicates whether the length is set.
        kFlagInterval    = 1 << 5,  ///< Indicates whether the display interval is set.
        kFlagPriority    = 1 << 6,  ///< Indicates whether the priority is set.
        kFlagTime        = 1 << 7,  ///< Indicates whether the testing time is set.
        kFlagNumber      = 1 << 8,  ///< Indicates whether the number of packets is set.
        kFlagSessionId   = 1 << 9,  ///< Indicates whether the session ID is set.
        kFlagFormatCvs   = 1 << 10, ///< Indicates whether the display format is CVS.
        kFlagFormatQuiet = 1 << 11, ///< Indicates whether the display is disabled.
        kFlagFinDelay    = 1 << 12, ///< Indicates whether the fin delay is set.
        kFlagDestAddr    = 1 << 13, ///< Indicates whether the destination address is set.
    };

    enum
    {
        kMaxFinDelayMs = 200, ///< Maximum Fin delay time (in milliseconds).
    };

    /**
     * This constructor initializes the Setting object.
     *
     */
    Setting(void)
        : mFlags(0)
        , mLength(kDefaultLength)
        , mBandwidth(kDefaultBandwidth)
        , mInterval(kDefaultInterval)
        , mTime(kDefaultTime)
        , mPriority(OT_MESSAGE_PRIORITY_LOW)
        , mFinDelay(0)
    {
        memset(&mDestAddr, 0, sizeof(otIp6Address));
    }

    /**
     * This method sets the flag.
     *
     * @param[in]  aFlag  The flag.
     *
     */
    void SetFlag(uint32_t aFlag) { mFlags |= aFlag; }

    /**
     * This method clears the flag.
     *
     * @param[in]  aFlag  The flag.
     *
     */
    void ClearFlag(uint32_t aFlag) { mFlags &= (~aFlag); }

    /**
     * This method indicates whether or not the flag is set.
     *
     * @param[in]  aFlag  The flag.
     *
     * @retval TRUE   If the flag is set.
     * @retval FALSE  If the flag is not set.
     *
     */
    bool IsFlagSet(uint32_t aFlag) const { return (mFlags & aFlag) ? true : false; }

    /**
     * This method returns a pointer to the IPv6 destination address.
     *
     * @returns A pointer to the IPv6 destination address.
     *
     */
    const otIp6Address *GetDestAddr() const { return &mDestAddr; }

    /**
     * This method sets the IPv6 destination address.
     *
     * @param[in]  aAddr  A reference to the IPv6 destination address.
     *
     */
    void SetDestAddr(otIp6Address &aAddr) { memcpy(mDestAddr.mFields.m8, aAddr.mFields.m8, sizeof(otIp6Address)); }

    /**
     * This method indicates whether the echo flag is set.
     *
     * @retval TRUE   If the echo flag is set.
     * @retval FALSE  If the echo flag is not set.
     *
     */
    bool GetEchoFlag(void) const { return IsFlagSet(kFlagEcho); }

    /**
     * This method sets the echo flag.
     *
     * @param[in]  aFlag  The echo flag.
     *
     */
    void SetEchoFlag(bool aFlag) { aFlag ? SetFlag(kFlagEcho) : ClearFlag(kFlagEcho); }

    /**
     * This method returns the bandwidth value.
     *
     * @retval The bandwidth value (in bits/sec).
     *
     */
    uint32_t GetBandwidth(void) const { return mBandwidth; }

    /**
     * This method sets the bandwidth value.
     *
     * @param[in]  aBandwidth  The bandwidth value (in bits/sec).
     *
     */
    void SetBandwidth(uint32_t aBandwidth) { mBandwidth = aBandwidth; }

    /**
     * This method returns the length value.
     *
     * @retval The length value.
     *
     */
    uint16_t GetLength(void) const { return mLength; }

    /**
     * This method sets the length value.
     *
     * @param[in]  aLength  The length value.
     *
     */
    void SetLength(uint16_t aLength) { mLength = aLength; }

    /**
     * This method returns the report interval value.
     *
     * @retval The interval value (in milliseconds).
     *
     */
    uint32_t GetInterval(void) const { return mInterval; }

    /**
     * This method sets the interval value.
     *
     * @param[in]  aLength  The interval value (in milliseconds).
     *
     */
    void SetInterval(uint32_t aInterval) { mInterval = aInterval; }

    /**
     * This method returns the time value.
     *
     * @retval The time value (in milliseconds).
     *
     */
    uint32_t GetTime(void) const { return mTime; }

    /**
     * This method sets the time value.
     *
     * @param[in]  aTime  The time value (in milliseconds).
     *
     */
    void SetTime(uint32_t aTime) { mTime = aTime; }

    /**
     * This method returns the number value.
     *
     * @retval The number value.
     *
     */
    uint32_t GetCount(void) const { return mCount; }

    /**
     * This method sets the count value.
     *
     * @param[in]  aCount  The count value.
     *
     */
    void SetCount(uint32_t aCount) { mCount = aCount; }

    /**
     * This method returns the priority value.
     *
     * @retval The priority value.
     *
     */
    otMessagePriority GetPriority(void) const { return mPriority; }

    /**
     * This method sets the priority value.
     *
     * @param[in]  aPriority  The priority value.
     *
     */
    void SetPriority(otMessagePriority aPriority) { mPriority = aPriority; }

    /**
     * This method returns the session ID.
     *
     * @retval The session ID.
     *
     */
    uint8_t GetSessionId(void) const { return mSessionId; }

    /**
     * This method sets the session ID.
     *
     * @param[in]  aSessionId  The session ID.
     *
     */
    void SetSessionId(uint8_t aSessionId) { mSessionId = aSessionId; }

    /**
     * This method returns the fin delay value.
     *
     * @retval The fin delay value (in seconds).
     *
     */
    uint8_t GetFinDelay(void) const { return mFinDelay; }

    /**
     * This method sets the fin delay value. The fin delay is the delay between the last sent data packet and the
     * first sent FIN packet.
     *
     * @param[in]  aFinDelay  The fin delay value (in seconds).
     *
     */
    void SetFinDelay(uint8_t aFinDelay) { mFinDelay = aFinDelay; }

private:
    enum
    {
        kDefaultBandwidth = 2000,  ///< Default bandwidth (in bits/sec).
        kDefaultLength    = 64,    ///< Default length (in bytes).
        kDefaultInterval  = 1000,  ///< Default interval of bandwidth reports (in milliseconds).
        kDefaultTime      = 10000, ///< Default time to transmit for (in milliseconds).
    };

    uint16_t          mFlags;
    uint16_t          mLength;
    otIp6Address      mDestAddr;
    uint32_t          mBandwidth;
    uint32_t          mInterval;
    uint32_t          mTime;
    uint32_t          mCount;
    otMessagePriority mPriority;
    uint8_t           mSessionId;
    uint8_t           mFinDelay;
};

/**
 * This structure represents the context passed to UDP socket.
 */
struct otPerfContext
{
    Perf *   mPerf;    ///< A pointer to a Perf object.
    Session *mSession; ///< A pointer to a Session object.
};

/**
 * This class implements the session.
 *
 */
class Session : public ListNode<Session>
{
public:
    enum
    {
        kRoleClient,   ///< Client.
        kRoleListener, ///< Listener.
        kRoleServer,   ///< Server.
    };

    enum
    {
        kStateInvalid,       ///< Invalid.
        kStateIdle,          ///< Idle.
        kStateListening,     ///< Server is listening.
        kStateSendingData,   ///< Client is sending data packets.
        kStateReceivingData, ///< Server is receiving data packets.
        kStateSendingFin,    ///< Client is sending Fin packets.
        kStateReceivingFin,  ///< Server is receiving Fin packets.
    };

    /**
     * This constructor initializes the Session object.
     *
     */
    Session(void) {}

    /**
     * This constructor initializes the Session object.
     *
     * @param[in]  aPerf     A reference to a Perf object.
     * @param[in]  aSetting  A reference to a Setting object.
     * @param[in]  aRole     Session role.
     *
     */
    Session(Perf &aPerf, const Setting &aSetting, uint8_t aRole);

    /**
     * This method gets the session role.
     *
     * @returns The session role.
     *
     */
    uint8_t GetRole(void) const { return mRole; }

    /**
     * This method sets the session role.
     *
     * @param[in]  aRole   The session role.
     *
     */
    void SetRole(uint8_t aRole) { mRole = aRole; }

    /**
     * This method indicates if the session is valid.
     *
     * @retval true   The session is valid.
     * @retval false  The session is invalid.
     *
     */
    bool IsStateValid(void) { return mState != kStateInvalid; }

    /**
     * This method closes the opened sockets.
     *
     */
    void CloseSocket(void);

    /**
     * This method returns a reference to the setting object.
     *
     * @returns A reference to the setting object.
     *
     */
    const Setting &GetSetting(void) { return *mSetting; }

    /**
     * This method returns a pointer to the socket context.
     *
     * @returns A pointer to the socket context.
     *
     */
    otPerfContext *GetContext(void) { return &mContext; }

    /**
     * This method sets the socket context.
     *
     * @param[in]  aPerf     A reference to the Perf object.
     * @param[in]  aSession  A reference to the Session object.
     *
     */
    void SetContext(Perf &aPerf, Session &aSession)
    {
        mContext.mPerf    = &aPerf;
        mContext.mSession = &aSession;
    }

    /**
     * This method compares two timestamps.
     *
     * @param[in]  aTimeA  A reference to the first timestamp.
     * @param[in]  aTimeB  A reference to the second timestamp.
     *
     * @retval -1  if @p aTimeA is less than @p aTimeB.
     * @retval  0  if @p aTimeA is equal to @p aTimeB.
     * @retval  1  if @p aTimeA is greater than @p aTimeB.
     *
     */
    int TimeCompare(const uint32_t &aTimeA, const uint32_t &aTimeB)
    {
        uint32_t diff = aTimeA - aTimeB;

        return (diff == 0) ? 0 : (((diff & (1UL << 31)) ? -1 : 1));
    }

    /**
     * This method opens a socket and listens on the default port.
     *
     * @retval OT_ERROR_NONE    Successfully opened the socket and listened on the  default port.
     * @retval OT_ERROR_FAILED  The role is not a listener or the state isn't idle.
     * @retval OT_ERROR_ALREADY The socket has been opened.
     */
    otError PrepareReceive(void);

    /**
     * This method opens a socket and prepares to transmit packets.
     *
     * @param[in]  aSessionId  A default session ID. If user doesn't specify a session ID, the session will use it as
     *                         the session ID.
     *
     * @retval OT_ERROR_NONE    Successfully opened the socket and prepared to transmit packets.
     * @retval OT_ERROR_FAILED  Either the role is not a client, or the state isn't idle.
     * @retval OT_ERROR_ALREADY The socket was already opened.
     */
    otError PrepareTransmit(uint8_t aSessionId);

    /**
     * This method processes the first received message.
     *
     * @param[in]  aMessage      A reference to the message.
     * @param[in]  aMessageInfo  A reference to the message information.
     *
     * @retval OT_ERROR_NONE    Successfully processed the received message.
     * @retval OT_ERROR_FAILED  Either the role is not a server, or the state isn't idle.
     * @retval OT_ERROR_PARSE   The format of the received message is not correct.
     */
    otError HandleFistMessage(otMessage &aMessage, const otMessageInfo &aMessageInfo);

    /**
     * This method processes the subsequenct received messages.
     *
     * @param[in]  aMessage      A reference to the message.
     * @param[in]  aMessageInfo  A reference to the message information.
     */
    void HandleSubsequentMessages(otMessage &aMessage, const otMessageInfo &aMessageInfo);

    /**
     * This method returns the timer delay interval.
     *
     * @param[out]  aInterval  A reference to the delay interval.
     *
     * @retval OT_ERROR_NONE           Successfully get delay interval.
     * @retval OT_ERROR_INVALID_STATE  The session isn't in sending data, sending Fin or receiving Fin state.
     */
    otError GetDelayInterval(uint32_t &aInterval);

    /**
     * This method indicates if the message is sent to this session.
     *
     * @param[in]  aMessageInfo  A reference to the message information.
     *
     * @retval true  The message is sent to this session.
     * @retval false The message isn't sent to this session.
     */
    bool MatchMsgInfo(const otMessageInfo &aMessageInfo);

    /**
     * This method handles the timer event.
     *
     */
    void TimerFired(void);

    /**
     * This method prints the client report header.
     *
     */
    void PrintClientReportHeader(void);

private:
    enum
    {
        kReportTypeInvalid   = 0, ///< Invalid report.
        kReportTypeClient    = 1, ///< Client periodic report.
        kReportTypeClientEnd = 2, ///< Client session end report.
        kReportTypeServer    = 3, ///< Server periodic report.
        kReportTypeServerEnd = 4, ///< Server session end report.
    };

    enum
    {
        kUdpPort              = 5001,       ///< Default listening port.
        kMaxNumFin            = 20,         ///< Maximum number of FIN packets.
        kFinIntervalUs        = 100000,     ///< Interval of sending FIN packets.
        kMinSendingIntervalUs = 2000,       ///< Mimimum interval of sending data packets.
        kInvalidTime          = UINT32_MAX, ///< Invalid time.
    };

    struct Stats
    {
        int32_t mJitter;          ///< The jitter value (in microseconds).
        int64_t mRelativeLatency; ///< The relative latency (in microseconds) of the received packet.

        uint64_t mCurBytes;         ///< Number of reveived bytes per reporting period.
        uint32_t mCurCntDatagram;   ///< Number of received packets per reporting period.
        uint32_t mCurCntOutOfOrder; ///< Number of out of order packets per reporting period.
        uint32_t mCurCntError;      ///< Number of lost packets per reporting period.

        uint32_t mCurMinLatency; ///< The minimum latency (in microseconds) per reporting period.
        uint32_t mCurMaxLatency; ///< The maximum latency (in microseconds) per reporting period.
        uint32_t mCurLatency;    ///< The sum of latency (in microseconds) per reporting period.

        uint64_t mTotalBytes;         ///< Total number of received bytes.
        uint32_t mTotalCntDatagram;   ///< Total number of received packets.
        uint32_t mTotalCntOutOfOrder; ///< Total number of out of order packets.
        uint32_t mTotalCntError;      ///< Total number of lost packets.

        uint32_t mTotalMinLatency; ///< The minimum latency (in microseconds) for all received packets.
        uint32_t mTotalMaxLatency; ///< The maximum latency (in microseconds) for all received packets.
        uint32_t mTotalLatency;    ///< The sum of latency (in microseconds) for all received packets.

        bool mLatencyValid; ///< Indicates whether the latency is valid.
    };

    struct Report
    {
        uint32_t mSessionId;        ///< Session ID.
        uint32_t mStartTime;        ///< Start time of report.
        uint32_t mEndTime;          ///< End time of report.
        int32_t  mJitter;           ///< Jitter.
        uint64_t mNumBytes;         ///< Total received bytes.
        uint32_t mCntError;         ///< Number of lost packets.
        uint32_t mCntDatagram;      ///< Total sent packets.
        uint32_t mCntOutOfOrder;    ///< Number of out of order packets.
        uint32_t mLatency;          ///< The sum of latency.
        uint32_t mMinLatency;       ///< Minimum latency.
        uint32_t mMaxLatency;       ///< Maximum latency.
        uint8_t  mReportType : 3;   ///< Report type.
        bool     mIsFormatCvs : 1;  ///< Indicates if the output format is CVS format.
        bool     mLatencyValid : 1; ///< Indicates if the latency is valid.
    };

    struct PacketInfo
    {
        uint16_t mLength;          ///< Packet paylod length.
        uint32_t mSeqNumber;       ///< Sequence number.
        uint32_t mAbsoluteLatency; ///< Absolute latency.
        int64_t  mRelativeLatency; ///< Relative latency.
    };

    void InitStats();
    void InitCurrentStats();

    otError  GetSynchronizedTime(uint64_t &aSyncTime);
    void     GetPacketInfo(PerfHeader &aPerfHeader, uint64_t aMicroNow, PacketInfo &aPacketInfo);
    void     UpdatePacketStats(PacketInfo &aPacketInfo);
    uint32_t UsToTimerTime(uint32_t aTime);

    otError OpenSocket(void);

    otError SendData(void);
    otError SendReply(PerfHeader &aPerfHeader, otMessagePriority aPriority, uint16_t aLength);
    otError SendFin(void);

    void PrintReport(Report &aReport, bool aServer);
    void PrintConnection(void);
    void PrintClientReport(void);
    void PrintClientReportEnd(void);
    void PrintServerReportHeader(void);
    void PrintServerReport(void);
    void PrintServerReportEnd(void);

    otUdpSocket  mSocket;
    otIp6Address mLocalAddr;
    otIp6Address mPeerAddr;
    uint16_t     mLocalPort;
    uint16_t     mPeerPort;

    uint32_t mFireTime;
    uint32_t mSessionStartTime;
    uint32_t mSessionEndTime;
    uint32_t mPrintStartTime;
    uint32_t mPrintEndTime;

    uint32_t mSeqNumber;
    uint32_t mSendingIntervalUs;
    Stats    mStats;

    otUdpSocket   mReplySocket;
    otPerfContext mContext;

    Perf *         mPerf;
    const Setting *mSetting;

    uint8_t mSessionId;
    uint8_t mFinCounter;
    uint8_t mRole : 2;
    uint8_t mState : 3;
    bool    mSocketValid : 1;
    bool    mReplySocketValid : 1;
};

/**
 * This class implements a CLI-based performance test tool.
 *
 */
class Perf
{
    friend class Session;

public:
    /**
     * Constructor
     *
     * @param[in]  aInterpreter  The CLI interpreter.
     *
     */
    explicit Perf(Interpreter &aInterpreter);

    /**
     * This method interprets a list of CLI arguments.
     *
     * @param[in]  argc  The number of elements in argv.
     * @param[in]  argv  A pointer to an array of command line arguments.
     *
     */
    otError Process(int argc, char *argv[]);

private:
    struct Command
    {
        const char *mName;
        otError (Perf::*mCommand)(int argc, char *argv[]);
    };

    enum
    {
        kMaxPayloadLength = Ip6::Ip6::kMaxDatagramLength - sizeof(Ip6::Header) - sizeof(Ip6::UdpHeader),
    };

    otError SetSetting(int argc, char *argv[], bool aClient, Setting &aSetting);
    void    PrintSetting(Setting &aSetting);
    otError ProcessHelp(int argc, char *argv[]);
    otError ProcessClient(int argc, char *argv[]);
    otError ProcessServer(int argc, char *argv[]);
    otError ProcessStart(int argc, char *argv[]);
    otError ProcessStop(int argc, char *argv[]);
    otError ProcessSync(int argc, char *argv[]);
    otError ProcessList(int argc, char *argv[]);
    otError ProcessClear(int argc, char *argv[]);

    void    SessionStop(uint8_t aRole);
    otError ServerStart(void);
    otError ClientStart(void);
    void    ServerStop(void);
    void    ClientStop(void);

    static void s_HandleUdpReceive(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo);
    void        HandleUdpReceive(otMessage *aMessage, const otMessageInfo *aMessageInfo, Session &aSession);

    Session *NewSession(const Setting &aSetting, uint8_t aRole);
    void     FreeSession(Session &aSession);
    Session *FindSession(const otMessageInfo &aMessageInfo);
    Session *FindValidSession(uint8_t aRole);
    void     UpdateClientState(void);

    Setting *NewSetting(void);
    void     FreeSetting(Setting &aSetting);

    static Perf &GetOwner(OwnerLocator &aOwnerLocator);

    static void s_HandleTimer(Timer &aTimer);
    void        HandleTimer(void);
    otError     FindMinDelayInterval(uint32_t &aInterval);
    void        StartTimer(void);

    static const Command sCommands[];

    bool mServerRunning : 1;
    bool mClientRunning : 1;
    bool mPrintServerHeaderFlag : 1;
    bool mPrintClientHeaderFlag : 1;

#if OPENTHREAD_CONFIG_PLATFORM_USEC_TIMER_ENABLE
    TimerMicro mTimer;
#else
    TimerMilli mTimer;
#endif

    Setting *mServerSetting;

    List<Setting> mSettingList;
    List<Session> mSessionList;

    Interpreter &mInterpreter;
    Instance *   mInstance;
};

} // namespace Cli
} // namespace ot

#endif // OPENTHREAD_CONFIG_CLI_PERF_ENABLE
#endif // CLI_PERF_HPP_
