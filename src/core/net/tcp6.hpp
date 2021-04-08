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
 *   This file includes definitions for parsing TCP header.
 */

#ifndef TCP_HPP_
#define TCP_HPP_

#include "openthread-core-config.h"

#include <openthread/tcp.h>

#include "common/locator.hpp"
#include "common/timer.hpp"
#include "net/ip6_headers.hpp"

namespace ot {
namespace Ip6 {

/**
 * @addtogroup core-tcp
 *
 * @brief
 *   This module includes definitions for parsing TCP header
 *
 * @{
 *
 */

/**
 * This class implements core TCP message handling.
 *
 */
class Tcp : public InstanceLocator, private NonCopyable
{
public:
    class Socket;
    class TcpTimer;

    /**
     * This class represents a TCP sequence number.
     */
    class Sequence
    {
    public:
        /**
         * Default constructor for `Sequence`.
         *
         */
        Sequence(void) = default;

        /**
         * This constructor initializes a `Sequence` object with a given value.
         *
         * @param[in] aValue   The numeric time value to initialize the `Sequence` object.
         *
         */
        explicit Sequence(uint32_t aValue)
            : mValue(aValue)
        {
        }

        /**
         * This method overloads operator `==` to evaluate whether or not two instances of `Sequence` are equal.
         *
         * @param[in]  aOther  The other `Sequence` instance to compare with.
         *
         * @retval TRUE   If the two `Sequence` instances are equal.
         * @retval FALSE  If the two `Sequence` instances are not equal.
         *
         */
        bool operator==(const Sequence &aOther) const { return mValue == aOther.mValue; }

        /**
         * This method returns a new `Sequence` which is ahead of this `Sequence` object by a given length.
         *
         * @param[in]   aLength  A length.
         *
         * @returns A new `Sequence` which is ahead of this sequence number by @aLength.
         *
         */
        Sequence operator+(uint16_t aLength) const { return Sequence(mValue + aLength); }

        /**
         * This method returns a new `Sequence` which is behind this `Sequence` object by a given length.
         *
         * @param[in]   aLength  A length.
         *
         * @returns A new `Sequence` which is behind this sequence number by @aLength.
         *
         */
        Sequence operator-(uint16_t aLength) const { return Sequence(mValue - aLength); }

        /**
         * This method calculates the length between two `Sequence` instances.
         *
         * @param[in]   aOther  A `Sequence` instance to subtract from.
         *
         * @returns The length from @p aOther to this `Sequence` object.
         *
         */
        uint16_t operator-(Sequence aOther) const
        {
            OT_ASSERT(*this >= aOther);
            return static_cast<uint16_t>(GetValue() - aOther.GetValue());
        }

        /**
         * This method indicates whether this `Sequence` instance is strictly before another one.
         *
         * @param[in]   aOther   A `Sequence` instance to compare with.
         *
         * @retval TRUE    This `Sequence` instance is strictly before @p aOther.
         * @retval FALSE   This `Sequence` instance is not strictly before @p aOther.
         *
         */
        bool operator<(const Sequence &aOther) const { return static_cast<int32_t>((mValue - aOther.mValue)) < 0; }

        /**
         * This method indicates whether this `Sequence` instance is after or equal to another one.
         *
         * @param[in]   aOther   A `Sequence` instance to compare with.
         *
         * @retval TRUE    This `Sequence` instance is after or equal to @p aOther.
         * @retval FALSE   This `Sequence` instance is not after or equal to @p aOther.
         *
         */
        bool operator>=(const Sequence &aOther) const { return !(*this < aOther); }

        /**
         * This method indicates whether this `Sequence` instance is before or equal to another one.
         *
         * @param[in]   aOther   A `Sequence` instance to compare with.
         *
         * @retval TRUE    This `Sequence` instance is before or equal to @p aOther.
         * @retval FALSE   This `Sequence` instance is not before or equal to @p aOther.
         *
         */
        bool operator<=(const Sequence &aOther) const { return (aOther >= *this); }

        /**
         * This method indicates whether this `Sequence` instance is strictly after another one.
         *
         * @param[in]   aOther   A `Sequence` instance to compare with.
         *
         * @retval TRUE    This `Sequence` instance is strictly after @p aOther.
         * @retval FALSE   This `Sequence` instance is not strictly after @p aOther.
         *
         */
        bool operator>(const Sequence &aOther) const { return (aOther < *this); }

        /**
         * This method moves this `Sequence` object forward by a given length.
         *
         * @param[in]   aDuration  A length.
         *
         */
        Sequence &operator+=(uint16_t aLength)
        {
            mValue += aLength;
            return *this;
        }
        /**
         * This method moves this `Sequence` object backward by a given length.
         *
         * @param[in]   aLength  A length.
         *
         */
        Sequence &operator-=(uint16_t aLength)
        {
            mValue -= aLength;
            return *this;
        }

        /**
         * This method gets the numeric sequence number associated with the `Sequence` object.
         *
         * @returns The numeric `Sequence` value.
         *
         */
        uint32_t GetValue(void) const { return mValue; }

    private:
        uint32_t mValue;
    };

    /**
     * This class implements TCP header parsing.
     *
     */
    OT_TOOL_PACKED_BEGIN
    class Header
    {
        friend class Socket;

    public:
        enum : uint8_t
        {
            kSourcePortFieldOffset = 0,  ///< The byte offset of Source Port field in TCP header.
            kDestPortFieldOffset   = 2,  ///< The byte offset of Destination Port field in TCP header.
            kChecksumFieldOffset   = 16, ///< The byte offset of Checksum field in TCP header.
        };
        /**
         * This method returns the TCP Source Port.
         *
         * @returns The TCP Source Port.
         *
         */
        uint16_t GetSourcePort(void) const { return HostSwap16(mSourcePort); }
        void     SetSourcePort(uint16_t aPort) { mSourcePort = HostSwap16(aPort); }

        /**
         * This method returns the TCP Destination Port.
         *
         * @returns The TCP Destination Port.
         *
         */
        uint16_t GetDestinationPort(void) const { return HostSwap16(mDestinationPort); }

        /**
         * This method sets the TCP Destination Port.
         *
         * @param[in]  aPort  The TCP Destination Port.
         *
         */
        void SetDestinationPort(uint16_t aPort) { mDestinationPort = HostSwap16(aPort); }

        /**
         * This method returns the TCP Sequence Number.
         *
         * @returns The TCP Sequence Number.
         *
         */
        Sequence GetSequenceNumber(void) const { return Sequence(HostSwap32(mSequenceNumber)); }

        /**
         * This method sets the TCP Sequence Number.
         *
         * @param[in]  aSequenceNumber  The TCP Sequence Number.
         *
         */
        void SetSequenceNumber(Sequence aSequenceNumber) { mSequenceNumber = HostSwap32(aSequenceNumber.GetValue()); }

        /**
         * This method returns the TCP Acknowledgment Sequence Number.
         *
         * @returns The TCP Acknowledgment Sequence Number.
         *
         */
        Sequence GetAcknowledgmentNumber(void) const { return Sequence(HostSwap32(mAckNumber)); }

        /**
         * This method sets the TCP Acknowledgment Sequence Number.
         *
         * @param[in] aAckNumber  The TCP Acknowledgment Sequence Number.
         *
         */
        void SetAcknowledgmentNumber(Sequence aAckNumber) { mAckNumber = HostSwap32(aAckNumber.GetValue()); }

        /**
         * This method returns the TCP Header length.
         *
         * @returns  The TCP Header length.
         *
         */
        uint8_t GetHeaderSize(void) const { return static_cast<uint8_t>(GetDataOffset() << 2); }

        /**
         * This method sets the TCP Header length.
         *
         * @param[in] aLength  The TCP Header size.
         */
        void SetHeaderSize(uint8_t aLength) { SetDataOffset(static_cast<uint8_t>(aLength >> 2)); }

        /**
         * This method returns the TCP Flags.
         *
         * @returns The TCP Flags.
         *
         */
        uint8_t GetFlags(void) const { return mFlags; }

        /**
         * This method sets the TCP Flags.
         *
         * @param[in] aFlags  The TCP Flags.
         *
         */
        void SetFlags(uint8_t aFlags) { mFlags = aFlags; }

        /**
         * This method checks if TCP Flags are set.
         *
         * @param[in] aFlags  The TCP Flags to check.
         *
         * @retval true   If all of the TCP Flags are set.
         * @retval false  If any of the TCP Flags is not set.
         *
         */
        bool HasFlags(uint8_t aFlags) const { return (mFlags & aFlags) == aFlags; }

        /**
         * This method returns the TCP Window.
         *
         * @returns The TCP Window.
         *
         */
        uint16_t GetWindow(void) const { return HostSwap16(mWindow); }

        /**
         * This method sets the TCP Window.
         *
         * @param[in] aWindow  The TCP Window.
         *
         */
        void SetWindow(uint16_t aWindow) { mWindow = HostSwap16(aWindow); }

        /**
         * This method returns the TCP Checksum.
         *
         * @returns The TCP Checksum.
         *
         */
        uint16_t GetChecksum(void) const { return HostSwap16(mChecksum); }

        /**
         * This method sets the TCP Checksum.
         *
         * @param[in] aChecksum  The TCP Checksum.
         *
         */
        void SetChecksum(uint16_t aChecksum) { mChecksum = HostSwap16(aChecksum); }

        /**
         * This method returns the TCP Urgent Pointer.
         *
         * @returns The TCP Urgent Pointer.
         *
         */
        uint16_t GetUrgentPointer(void) const { return HostSwap16(mUrgentPointer); }

        /**
         * This method sets the TCP Urgent Pointer.
         *
         * @param[in] aUrgentPointer  The TCP Urgent Pointer.
         *
         */
        void SetUrgentPointer(uint16_t aUrgentPointer) { mUrgentPointer = HostSwap16(aUrgentPointer); }

    private:
        uint8_t GetDataOffset(void) const { return static_cast<uint8_t>(mDataOffset >> 4); }
        void    SetDataOffset(uint8_t aOffset) { mDataOffset = static_cast<uint8_t>(aOffset << 4); }

        uint16_t mSourcePort;
        uint16_t mDestinationPort;
        uint32_t mSequenceNumber;
        uint32_t mAckNumber;
        uint8_t  mDataOffset;
        uint8_t  mFlags;
        uint16_t mWindow;
        uint16_t mChecksum;
        uint16_t mUrgentPointer;

    } OT_TOOL_PACKED_END;

    /**
     * This class implements a TCP timer.
     *
     */
    class TcpTimer : public LinkedListEntry<TcpTimer>
    {
        friend class Tcp;
        friend class LinkedListEntry<TcpTimer>;

    private:
        explicit TcpTimer(Socket &aSocket)
            : mSocket(aSocket)
            , mNext(nullptr)
        {
        }

        void      SetFireTime(TimeMilli aFireTime) { mFireTime = aFireTime; }
        TimeMilli GetFireTime(void) const { return mFireTime; }
        Socket &  GetSocket(void) { return mSocket; }

        Socket &  mSocket;
        TcpTimer *mNext;
        TimeMilli mFireTime;
    };

    /**
     * This class implements a TCP/IPv6 socket.
     *
     */
    class Socket : public InstanceLocator, public LinkedListEntry<Socket>, private NonCopyable
    {
        friend class Tcp;
        friend class LinkedList<Socket>;

    public:
        /**
         * This constructor initializes the TCP socket object.
         *
         * @param[in] aInstance      A reference to the OpenThread instance.
         * @param[in] aEventHandler  A pointer to the event handler.
         * @param[in] aContext       A pointer to the application-specific context.
         *
         */
        Socket(Instance &aInstance, otTcpEventHandler aEventHandler, void *aContext);

        /**
         * This destructor deinitializes the object.
         *
         */
        ~Socket(void);

        /**
         * This method indicates whether or not the socket is bound.
         *
         * @retval TRUE if the socket is bound (i.e. source port is non-zero).
         * @retval FALSE if the socket is not bound (source port is zero).
         *
         */
        bool IsBound(void) const { return mSockName.mPort != 0; }

        /**
         * This method returns the local socket address.
         *
         * @returns A reference to the local socket address.
         *
         */
        SockAddr &GetSockName(void) { return *static_cast<SockAddr *>(&mSockName); }

        /**
         * This method returns the local socket address.
         *
         * @returns A reference to the local socket address.
         *
         */
        const SockAddr &GetSockName(void) const { return *static_cast<const SockAddr *>(&mSockName); }

        /**
         * This method returns the peer's socket address.
         *
         * @returns A reference to the peer's socket address.
         *
         */
        SockAddr &GetPeerName(void) { return *static_cast<SockAddr *>(&mPeerName); }

        /**
         * This method returns the peer's socket address.
         *
         * @returns A reference to the peer's socket address.
         *
         */
        const SockAddr &GetPeerName(void) const { return *static_cast<const SockAddr *>(&mPeerName); }

        /**
         * This method connects the TCP socket.
         *
         * @param[in] aAddr  The peer socket address to connect.
         *
         * @retval OT_ERROR_NONE           Successfully initialized the connection.
         * @retval OT_ERROR_INVALID_ARGS   If @p aAddr is not a valid address.
         * @retval OT_ERROR_INVALID_STATE  If the socket is not in a valid state to connect.
         *
         */
        otError Connect(const SockAddr &aAddr);

        /**
         * This method binds the TCP socket.
         *
         * @param[in] aAddr  The socket address to bind.
         *
         * @retval OT_ERROR_NONE          Successfully bound the TCP socket.
         * @retval OT_ERROR_INVALID_ARGS  If @p aAddr is not a valid address.
         *
         */
        otError Bind(const SockAddr &aAddr);

        /**
         * This method listens the TCP socket.
         *
         * @retval OT_ERROR_NONE           Successfully put the socket in LISTEN state.
         * @retval OT_ERROR_INVALID_STATE  If the socket is not in a valid state to listen.
         *
         */
        otError Listen(void);

        /**
         * This method reads the TCP socket.
         *
         * @param[in] aBuf   A pointer to the buffer to receive the data.
         * @param[in] aSize  The size of the buffer.
         *
         * @returns  The number of data bytes that was read from the TCP socket.
         *
         */
        uint16_t Read(uint8_t *aBuf, uint16_t aSize);

        /**
         * This method writes the TCP socket.
         *
         * @param[in] aData    A pointer to data bytes to write.
         * @param[in] aLength  The length of data bytes.
         *
         * @returns  The number of data bytes that was written to the TCP socket.
         *
         */
        uint16_t Write(const uint8_t *aData, uint16_t aLength);

        /**
         * This method closes the TCP socket.
         *
         * @note The TCP socket will go through a number of states (e.g. FIN-WAIT-1, FIN-WAIT-2, TIME-WAIT) before it's
         * eventually closed (i.e. in CLOSED state).
         *
         */
        void Close(void);

        /**
         * This method aborts the TCP socket.
         *
         * @note The TCP socket will always be in CLOSED state after this method is called.
         *
         */
        void Abort(void);

        /**
         * This method gets the current TCP state of the TCP socket.
         *
         * @returns  The current TCP state of the TCP socket.
         *
         */
        otTcpState GetState(void) const { return mState; }

        /**
         * This method gets the application-specific context of the TCP socket.
         *
         * @returns  The application-specific context.
         *
         */
        void *GetContext(void) const { return mContext; }

        /**
         * This method configures the Round Trip Times (Rtt).
         *
         * @param[in] aMinRtt  Minimal Round Trip Time.
         * @param[in] aMaxRtt  Maximal Round Trip Time.
         *
         * @retval OT_ERROR_NONE          Successfully configured Round Trip Times.
         * @retval OT_ERROR_INVALID_ARGS  If @p aMinRtt and @p aMaxRtt are not valid Round Trip Times.
         *
         */
        otError ConfigRoundTripTime(uint32_t aMinRtt, uint32_t aMaxRtt);

#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
        /**
         * This method makes the TCP socket to always send RST for the next received segment.
         *
         * @note Only available when `OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE` is enabled.
         *
         */
        void ResetNextSegment(void);
#endif

        Socket *mNext; ///< A pointer to the next TCP socket (internal use only).

    private:
        enum SegmentAction : uint8_t
        {
            kActionNone,
            kActionAck,
            kActionReset,
            kActionAbort,
            kActionReceive,
        };

        enum : uint32_t
        {
            kSynTimeout              = 3000, // should be 75000
            kNewMessageSendTimeout   = 100,
            kAckDelay                = 50,
            kInitialRtt              = 1000,   // initial Rtt (in milliseconds)
            kDefaultMinRoundTripTime = 100,    // minimum Rtt (in milliseconds)
            kDefaultMaxRoundTripTime = 120000, // maximum Rtt (in milliseconds)
            kZeroWindowSendInterval  = 30000,
        };

        static_assert(kDefaultMinRoundTripTime <= kInitialRtt, "invalid initial Rtt");
        static_assert(kDefaultMaxRoundTripTime >= kInitialRtt, "invalid initial Rtt");
        static_assert(kAckDelay < 500, "RFC1122: Delayed ACK's Delay < 0.5 seconds");
        enum
        {
            kRttAlphaDenominator           = 10,
            kRttBetaNumerator              = 2,
            kRttBetaDenominator            = 1,
            kMinFreeMessageBufferThreshold = 5,
        };

        enum : uint8_t
        {
            kRequireAckPeerNone,
            kRequireAckPeerDelayed1,
            kRequireAckPeerDelayed2,
            kRequireAckPeerImmediately,
            kRequireAckPeerMax = kRequireAckPeerImmediately,

            kRequireAckPeerIncNormal           = 1,
            kRequireAckPeerIncFullSizedSegment = 2,
        };

        static_assert(kRequireAckPeerMax <= 3, "mRequireAckPeer has only 2 bits");
        static_assert(kRequireAckPeerNone + kRequireAckPeerIncFullSizedSegment < kRequireAckPeerImmediately,
                      "1st full-sized segment is not ACKed immediately");
        static_assert(kRequireAckPeerNone + kRequireAckPeerIncFullSizedSegment + kRequireAckPeerIncFullSizedSegment >=
                          kRequireAckPeerImmediately,
                      "2nd full-size segment is ACKed immediately");

        class SendWindow : public Clearable<SendWindow>
        {
            friend class Tcp::Socket;

        public:
            class Entry
            {
            public:
                uint16_t GetSegmentLength(void) const;
                bool     IsWritable(uint16_t aMaxSegmentSize) const;

                Message * mMessage;
                TimeMilli mLastSendTime;
                uint8_t   mSendCount;
                bool      mIsSyn : 1;
                bool      mIsFin : 1;
            };

            SendWindow(void);

            uint8_t      GetLength(void) const { return mLength; }
            uint8_t      ReclaimAcked(Tcp::Sequence aAckNumber, uint32_t &aRtt);
            Message *    GetSendNext(Sequence &aSeq,
                                     uint8_t & aFlags,
                                     uint32_t  aRtt,
                                     uint32_t  aMaxRtt,
                                     uint16_t  aMaxSegmentSize,
                                     bool &    aRetransmissionTimeout,
                                     bool &    aIsRetransmission);
            Message *    GetWritableMessage(uint16_t aMaxSegmentSize);
            void         Add(Message &aMessage);
            void         Add(Message *aMessage, bool aIsSyn, bool aIsFin);
            bool         IsFull(void);
            Sequence     GetStartSeq(void) const { return mStartSeq; }
            Sequence     GetStopSeq(void) const;
            Sequence     GetSendNextSeq(void) const;
            void         CheckInvariant(void);
            Entry &      GetEntry(uint8_t aIndex) { return mSegments[(mStartIndex + aIndex) % kMaxSendSegments]; }
            const Entry &GetEntry(uint8_t aIndex) const { return mSegments[(mStartIndex + aIndex) % kMaxSendSegments]; }
            Message *    GetMessage(uint8_t aIndex) { return GetEntry(aIndex).mMessage; }
            const Message * GetMessage(uint8_t aIndex) const { return GetEntry(aIndex).mMessage; }
            void            ReclaimHead(uint32_t &aRtt);
            TimeMilli       GetNextSendTime(TimeMilli aNow, uint32_t aRtt, uint32_t aMaxRtt, uint16_t aMaxSegmentSize);
            bool            TakeCustody(Message &aMessage);
            bool            IsEmpty(void) const;
            void            AddSYN(void);
            void            AddFIN(void);
            Entry *         GetLast(void);
            void            Flush(void);
            static void     FreeEntry(Entry &aEntry);
            void            UpdateSendWindow(const Header &aHeader);
            void            ConfigSendWindowBySYN(const Header &aHeader);
            uint16_t        GetSendWindowSize(void) const;
            uint32_t        GetSendTimeout(const Entry &aEntry,
                                           Sequence     aMsgStartSeq,
                                           uint32_t     aRtt,
                                           uint32_t     aMaxRtt,
                                           uint16_t     aMaxSegmentSize);
            static Sequence GenerateInitialSendSequence(void);
            void            ResetSendCount(void);

        private:
            enum
            {
                kMaxSendSegments = 4,
            };

            static_assert(kMaxSendSegments >= 2, "send queue is too short");

            Entry    mSegments[kMaxSendSegments];
            Sequence mStartSeq;
            Sequence mSndWl1, mSndWl2;
            uint16_t mSndWnd;
            uint8_t  mStartIndex;
            uint8_t  mLength;
            bool     mPendingFIN : 1;
        };

        class ReceiveWindow : public Clearable<ReceiveWindow>
        {
            friend class Tcp::Socket;

        private:
            enum
            {
                kMaxRecvSegments = 8,
            };

            class Entry
            {
                friend class Tcp::Socket::ReceiveWindow;

            public:
                Message *mMessage;
            };

            ReceiveWindow(void)
                : mStartSeq(0)
                , mStartIndex(0)
                , mProcessNext(0)
                , mLength(0)
            {
            }

            Entry &      GetEntry(uint8_t aIndex) { return mSegments[(mStartIndex + aIndex) % kMaxRecvSegments]; }
            const Entry &GetEntry(uint8_t aIndex) const { return mSegments[(mStartIndex + aIndex) % kMaxRecvSegments]; }
            Message &    GetMessage(uint8_t aIndex) { return *GetEntry(aIndex).mMessage; }
            const Message & GetMessage(uint8_t aIndex) const { return *GetEntry(aIndex).mMessage; }
            void            Init(Sequence aStartSeq);
            Sequence        GetStartSeq(void);
            otError         Add(Message &aMessage);
            void            CheckInvariant(void);
            bool            IsFull(void) const;
            Message *       Process(void);
            Message &       Pop(uint8_t aIndex);
            static void     GetSegmentRange(Message &aMessage, Sequence &aStartSeq, Sequence &aStopSeq);
            bool            IsEmpty(void) const { return mLength == 0; }
            bool            IsProcessEmpty(void) const { return mLength == mProcessNext; }
            void            Flush(void);
            uint32_t        GetReadable(void);
            void            ClearEmptySegments(void);
            static uint16_t GetSegmentTextLength(Message &aMessage);
            uint16_t        Read(uint8_t *aBuf, uint16_t aSize);
            uint16_t        GetReceiveWindowSize(void) const;
            static otError  MergeFlags(Message &aMessage, const Message &aMerging);

            Entry    mSegments[kMaxRecvSegments];
            Sequence mStartSeq;
            uint8_t  mStartIndex;
            uint8_t  mProcessNext;
            uint8_t  mLength;
        };

        bool                 TakeCustody(Message &aMessage);
        void                 HandleMessage(const Header &     aHeader,
                                           Message &          aMessage,
                                           const MessageInfo &aMessageInfo,
                                           SegmentAction &    aAction);
        void                 Send(void);
        void                 ProcessRecvQueue(const MessageInfo &aMessageInfo);
        Sequence             GetRcvNxt(void);
        Sequence             GetSndUna(void) const;
        Sequence             GetSndNxt(void) const;
        void                 ProcessFIN(const Header &aHeader);
        void                 SetState(otTcpState aState);
        Error                SetPeerName(const SockAddr &aAddr);
        bool                 IsAckAcceptable(Sequence aAckNumber);
        bool                 IsSeqAcceptable(const Header &aHeader, uint16_t aSegmentLength, bool &aIsDuplicate);
        uint16_t             GetReceiveWindowSize(void);
        void                 ProcessAck(const Header &aHeader);
        uint32_t             GetRtt(void) const;
        void                 ResetTimer(void);
        bool                 HasFINToBeAcked(void) const;
        void                 SendSYN(void);
        void                 SendFIN(void);
        void                 SelectSourceAddress(void);
        bool                 CanSendData(void);
        void                 ProcessFINAcked(void);
        void                 OnAborted(void);
        void                 NotifyDataReceived(void);
        void                 NotifyDataSent(void);
        void                 UpdateRtt(uint32_t aRtt);
        void                 RequireAckPeer(bool aFullSizedSegment);
        SendWindow &         GetSendQueue(void) { return mSendQueue; }
        const SendWindow &   GetSendQueue(void) const { return mSendQueue; }
        ReceiveWindow &      GetRecvQueue(void) { return mRecvQueue; }
        const ReceiveWindow &GetRecvQueue(void) const { return mRecvQueue; }
        void                 ProcessReceivedSegment(Message &aMessage, const MessageInfo &aMessageInfo);
        static void          LogTcpMessage(const char *aAction, const Message &aMessage, const Header &aHeader);
        void                 HandleTimer(void);
        void                 TriggerEvent(otTcpSocketEvent aEvent);
        static otError       AddMaxSegmentSizeOption(Header &aHeader, Message &aMessage);
        void                 ReadMaxSegmentSizeOption(Message &aSynMessage);

        SockAddr          mSockName; ///< The local IPv6 socket address.
        SockAddr          mPeerName; ///< The peer IPv6 socket address.
        otTcpEventHandler mEventHandler;
        void *            mContext; ///< A pointer to application-specific context.
        SendWindow        mSendQueue;
        ReceiveWindow     mRecvQueue;
        Tcp::TcpTimer     mTimer;
        TimeMilli         mTimeWaitStartTime;
        uint32_t          mSmoothedRtt;
        uint32_t          mMinRoundTripTime;
        uint32_t          mMaxRoundTripTime;
        uint16_t          mPeerMaxSegmentSize;
        otTcpState        mState; ///< The TCP state.
        uint8_t           mRequireAckPeer : 2;
        bool              mPendingNotifyDataSent : 1;
        bool              mPendingNotifyDataReceived : 1;

#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
        bool mResetNextSegment : 1;
#endif
    };

    static_assert(sizeof(otTcpSocket) >= sizeof(Socket), "sizeof(otTcpSocket) is too small.");

#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
    struct Counters : public otTcpCounters, public Clearable<Counters>
    {
        Counters(void) { Clear(); }
    };
#endif

    /**
     * This constructor initializes the object.
     *
     * @param[in]  aInstance  A reference to OpenThread instance.
     *
     */
    explicit Tcp(Instance &aInstance);

    /**
     * This method initialize a TCP socket.
     *
     * @param[in]  aSocket   A reference to the socket.
     * @param[in]  aHandler  A pointer to a function that is called when receiving TCP messages.
     * @param[in]  aContext  A pointer to arbitrary context information.
     *
     */
    void Initialize(Socket &aSocket, otTcpEventHandler aEventHandler, void *aContext);

    /**
     * This method returns a new ephemeral port.
     *
     * @returns A new ephemeral port.
     *
     */
    uint16_t GetEphemeralPort(void);

    /**
     * This method returns a new TCP message with sufficient header space reserved.
     *
     * @param[in]  aReserved  The number of header bytes to reserve after the TCP header.
     * @param[in]  aSettings  The message settings (default is used if not provided).
     *
     * @returns A pointer to the message or nullptr if no buffers are available.
     *
     */
    Message *NewMessage(const Message::Settings &aSettings = Message::Settings::GetDefault());

    /**
     * This method returns if the socket address is a valid TCP socket address.
     *
     * @param[in] aAddr     The TCP socket address.
     *
     * @returns  TRUE if the TCP socket address is valid, FALSE otherwise.
     *
     */
    static bool IsValidSockAddr(const SockAddr &aAddr)
    {
        return !aAddr.GetAddress().IsUnspecified() && !aAddr.GetAddress().IsMulticast() && aAddr.GetPort() != 0;
    }

    void               HandleMessage(Message &aMessage, MessageInfo &aMessageInfo);
    void               TakeCustody(Message &aMessage);
    static const char *StateToString(otTcpState aState);

    void        StopTimer(TcpTimer &aTimer);
    void        StartTimer(TcpTimer &aTimer, TimeMilli aFireTime);
    static void HandleSoleTimer(Timer &aTimer);
    void        HandleSoleTimer(void);

#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
    void SetSegmentRandomDropProb(uint8_t aProb);

    Counters mCounters;
#endif

private:
    enum Flag
    {
        kFlagFIN = 1,
        kFlagSYN = 2,
        kFlagRST = 4,
        kFlagPSH = 8,
        kFlagACK = 16,
        kFlagURG = 32,
    };

    enum OptionKind : uint8_t
    {
        kOptionKindEndOfOptionList = 0,
        kOptionKindNoOperation     = 1,
        kOptionKindMaxSegmentSize  = 2,
    };

    enum : uint8_t
    {
        kMaxSegmentSizeOptionSize = 4,
        kMaxRetransmissionCount   = 10,
    };

    enum : uint16_t
    {
        kMaxSegmentSizeNoFrag = 77,
        kMaxSegmentSize       = 250,
    };

    enum : uint32_t
    {
        kDynamicPortMin     = 49152,         ///< Service Name and Transport Protocol Port Number Registry
        kDynamicPortMax     = 65535,         ///< Service Name and Transport Protocol Port Number Registry
        kMaxSegmentLifetime = 2 * 60 * 1000, ///< Maximum Segment Lifetime (MSL) is set to 2 minutes
    };

    void        AddSocket(Socket &aSocket);
    void        RemoveSocket(Socket &aSocket);
    otError     SendMessage(Message &aMessage, MessageInfo &aMessageInfo);
    static bool ShouldHandleTcpMessage(const Socket &aSocket, const Header &aHeader, const MessageInfo &aMessageInfo);
    void        RespondReset(const Header &     aTcpHeader,
                             const Message &    aMessage,
                             const MessageInfo &aMessageInfo,
                             const Socket *     aSocket);
    void        SendReset(const Address &aSrcAddr,
                          uint16_t       aSrcPort,
                          const Address &aDstAddr,
                          uint16_t       aDstPort,
                          bool           aSetAck,
                          Sequence       aSeq,
                          Sequence       aAckNumber);
    static uint16_t GetSegmentLength(const Header &aHeader, const Message &aMessage);
    void            ResetSoleTimer(void);

    LinkedList<Socket>   mSockets;
    LinkedList<TcpTimer> mTimerList;
    TimerMilli           mSoleTimer;
    uint16_t             mEphemeralPort;
#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
    uint8_t mSegmentRandomDropProb;
#endif
};

/**
 * @}
 *
 */

} // namespace Ip6
} // namespace ot

#endif // TCP_HPP_
