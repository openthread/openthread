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
 *   This file includes definitions for the message buffer pool and message buffers.
 */

#ifndef MESSAGE_HPP_
#define MESSAGE_HPP_

#include "openthread-core-config.h"

#include "utils/wrap_stdint.h"
#include "utils/wrap_string.h"

#include <openthread/message.h>
#include <openthread/platform/messagepool.h>

#include "common/code_utils.hpp"
#include "common/locator.hpp"
#include "mac/mac_frame.hpp"
#include "thread/link_quality.hpp"

namespace ot {

/**
 * @addtogroup core-message
 *
 * @brief
 *   This module includes definitions for the message buffer pool and message buffers.
 *
 * @{
 *
 */

enum
{
    kNumBuffers = OPENTHREAD_CONFIG_NUM_MESSAGE_BUFFERS,
    kBufferSize = OPENTHREAD_CONFIG_MESSAGE_BUFFER_SIZE,
};

class Message;
class MessagePool;
class MessageQueue;
class PriorityQueue;

/**
 * This structure contains metdata about a Message.
 *
 */
struct MessageInfo
{
    enum
    {
        kListAll       = 0, ///< Identifies the all messages list (maintained by the MessagePool).
        kListInterface = 1, ///< Identifies the list for per-interface message queue.
        kNumLists      = 2, ///< Number of lists.
    };

    Message *    mNext[kNumLists]; ///< A pointer to the next Message in a doubly linked list.
    Message *    mPrev[kNumLists]; ///< A pointer to the previous Message in a doubly linked list.
    MessagePool *mMessagePool;     ///< Identifies the message pool for this message.
    union
    {
        MessageQueue * mMessage;  ///< Identifies the message queue (if any) where this message is queued.
        PriorityQueue *mPriority; ///< Identifies the priority queue (if any) where this message is queued.
    } mQueue;                     ///< Identifies the queue (if any) where this message is queued.

    uint16_t    mReserved;    ///< Number of header bytes reserved for the message.
    uint16_t    mLength;      ///< Number of bytes within the message.
    uint16_t    mOffset;      ///< A byte offset within the message.
    uint16_t    mDatagramTag; ///< The datagram tag used for 6LoWPAN fragmentation.
    RssAverager mRssAverager; ///< The averager maintaining the received signal strength (RSS) average.

    uint8_t mChildMask[8]; ///< A bit-vector to indicate which sleepy children need to receive this.
    uint8_t mTimeout;      ///< Seconds remaining before dropping the message.
    int8_t  mInterfaceId;  ///< The interface ID.
    union
    {
        uint16_t mPanId;   ///< Used for MLE Discover Request and Response messages.
        uint8_t  mChannel; ///< Used for MLE Announce.
    } mPanIdChannel;       ///< Used for MLE Discover Request, Response, and Announce messages.

    uint8_t mType : 2;         ///< Identifies the type of message.
    uint8_t mSubType : 4;      ///< Identifies the message sub type.
    bool    mDirectTx : 1;     ///< Used to indicate whether a direct transmission is required.
    bool    mLinkSecurity : 1; ///< Indicates whether or not link security is enabled.
    uint8_t mPriority : 2;     ///< Identifies the message priority level (lower value is higher priority).
    bool    mInPriorityQ : 1;  ///< Indicates whether the message is queued in normal or priority queue.
    bool    mTxSuccess : 1;    ///< Indicates whether the direct tx of the message was successful.
#if OPENTHREAD_CONFIG_ENABLE_TIME_SYNC
    bool    mTimeSync : 1;      ///< Indicates whether the message is also used for time sync purpose.
    uint8_t mTimeSyncSeq;       ///< The time sync sequence.
    int64_t mNetworkTimeOffset; ///< The time offset to the Thread network time, in microseconds.
#endif
};

/**
 * This class represents a Message buffer.
 *
 */
class Buffer : public ::otMessage
{
    friend class Message;

public:
    /**
     * This method returns a pointer to the next message buffer.
     *
     * @returns A pointer to the next message buffer.
     *
     */
    class Buffer *GetNextBuffer(void) const { return static_cast<Buffer *>(mNext); }

    /**
     * This method sets the pointer to the next message buffer.
     *
     */
    void SetNextBuffer(class Buffer *buf) { mNext = static_cast<otMessage *>(buf); }

private:
    /**
     * This method returns a pointer to the first byte of data in the first message buffer.
     *
     * @returns A pointer to the first data byte.
     *
     */
    uint8_t *GetFirstData(void) { return mBuffer.mHead.mData; }

    /**
     * This method returns a pointer to the first byte of data in the first message buffer.
     *
     * @returns A pointer to the first data byte.
     *
     */
    const uint8_t *GetFirstData(void) const { return mBuffer.mHead.mData; }

    /**
     * This method returns a pointer to the first data byte of a subsequent message buffer.
     *
     * @returns A pointer to the first data byte.
     *
     */
    uint8_t *GetData(void) { return mBuffer.mData; }

    /**
     * This method returns a pointer to the first data byte of a subsequent message buffer.
     *
     * @returns A pointer to the first data byte.
     *
     */
    const uint8_t *GetData(void) const { return mBuffer.mData; }

    enum
    {
        kBufferDataSize     = kBufferSize - sizeof(struct otMessage),
        kHeadBufferDataSize = kBufferDataSize - sizeof(struct MessageInfo),
    };

protected:
    union
    {
        struct
        {
            MessageInfo mInfo;
            uint8_t     mData[kHeadBufferDataSize];
        } mHead;
        uint8_t mData[kBufferDataSize];
    } mBuffer;
};

/**
 * This class represents a message.
 *
 */
class Message : public Buffer
{
    friend class MessagePool;
    friend class MessageQueue;
    friend class PriorityQueue;

public:
    enum
    {
        kTypeIp6         = 0, ///< A full uncompressed IPv6 packet
        kType6lowpan     = 1, ///< A 6lowpan frame
        kTypeMacDataPoll = 2, ///< A MAC data poll message
        kTypeSupervision = 3, ///< A child supervision frame.
    };

    enum
    {
        kSubTypeNone                   = 0, ///< None
        kSubTypeMleAnnounce            = 1, ///< MLE Announce
        kSubTypeMleDiscoverRequest     = 2, ///< MLE Discover Request
        kSubTypeMleDiscoverResponse    = 3, ///< MLE Discover Response
        kSubTypeJoinerEntrust          = 4, ///< Joiner Entrust
        kSubTypeMplRetransmission      = 5, ///< MPL next retransmission message
        kSubTypeMleGeneral             = 6, ///< General MLE
        kSubTypeJoinerFinalizeResponse = 7, ///< Joiner Finalize Response
        kSubTypeMleChildUpdateRequest  = 8, ///< MLE Child Update Request
        kSubTypeMleDataResponse        = 9, ///< MLE Data Response
    };

    enum
    {
        kPriorityLow    = OT_MESSAGE_PRIORITY_LOW,      ///< Low priority level.
        kPriorityNormal = OT_MESSAGE_PRIORITY_NORMAL,   ///< Normal priority level.
        kPriorityHigh   = OT_MESSAGE_PRIORITY_HIGH,     ///< High priority level.
        kPriorityNet    = OT_MESSAGE_PRIORITY_HIGH + 1, ///< Network Control priority level.

        kNumPriorities = 4, ///< Number of priority levels.
    };

    /**
     * This method frees this message buffer.
     *
     */
    void Free(void);

    /**
     * This method returns a pointer to the next message in the same interface list.
     *
     * @returns A pointer to the next message in the same interface list or NULL if at the end of the list.
     *
     */
    Message *GetNext(void) const;

    /**
     * This method returns the number of bytes in the message.
     *
     * @returns The number of bytes in the message.
     */
    uint16_t GetLength(void) const { return mBuffer.mHead.mInfo.mLength; }

    /**
     * This method sets the number of bytes in the message.
     *
     * @param[in]  aLength  Requested number of bytes in the message.
     *
     * @retval OT_ERROR_NONE     Successfully set the length of the message.
     * @retval OT_ERROR_NO_BUFS  Failed to grow the size of the message because insufficient buffers were available.
     *
     */
    otError SetLength(uint16_t aLength);

    /**
     * This method returns the number of buffers in the message.
     *
     */
    uint8_t GetBufferCount(void) const;

    /**
     * This method returns the byte offset within the message.
     *
     * @returns A byte offset within the message.
     *
     */
    uint16_t GetOffset(void) const { return mBuffer.mHead.mInfo.mOffset; }

    /**
     * This method moves the byte offset within the message.
     *
     * @param[in]  aDelta  The number of bytes to move the current offset, which may be positive or negative.
     *
     * @retval OT_ERROR_NONE          Successfully moved the byte offset.
     * @retval OT_ERROR_INVALID_ARGS  The resulting byte offset is not within the existing message.
     *
     */
    otError MoveOffset(int aDelta);

    /**
     * This method sets the byte offset within the message.
     *
     * @param[in]  aOffset  The number of bytes to move the current offset, which may be positive or negative.
     *
     * @retval OT_ERROR_NONE          Successfully moved the byte offset.
     * @retval OT_ERROR_INVALID_ARGS  The requested byte offset is not within the existing message.
     *
     */
    otError SetOffset(uint16_t aOffset);

    /**
     * This method returns the type of the message.
     *
     * @returns The type of the message.
     *
     */
    uint8_t GetType(void) const { return mBuffer.mHead.mInfo.mType; }

    /**
     * This method sets the message type.
     *
     * @param[in]  aType  The message type.
     *
     */
    void SetType(uint8_t aType) { mBuffer.mHead.mInfo.mType = aType; }

    /**
     * This method returns the sub type of the message.
     *
     * @returns The sub type of the message.
     *
     */
    uint8_t GetSubType(void) const { return mBuffer.mHead.mInfo.mSubType; }

    /**
     * This method sets the message sub type.
     *
     * @param[in]  aSubType  The message sub type.
     *
     */
    void SetSubType(uint8_t aSubType) { mBuffer.mHead.mInfo.mSubType = aSubType; }

    /**
     * This method returns whether or not the message is of MLE subtype.
     *
     * @retval TRUE   If message is of MLE subtype.
     * @retval FALSE  If message is not of MLE subtype.
     *
     */
    bool IsSubTypeMle(void) const;

    /**
     * This method returns the message priority level.
     *
     * @returns The priority level associated with this message.
     *
     */
    uint8_t GetPriority(void) const { return mBuffer.mHead.mInfo.mPriority; }

    /**
     * This method sets the messages priority.
     * If the message is already queued in a priority queue, changing the priority ensures to
     * update the message in the associated queue.
     *
     * @param[in]  aPriority  The message priority level.
     *
     * @retval OT_ERROR_NONE           Successfully set the priority for the message.
     * @retval OT_ERROR_INVALID_ARGS   Priority level is not invalid.
     *
     */
    otError SetPriority(uint8_t aPriority);

    /**
     * This method prepends bytes to the front of the message.
     *
     * On success, this method grows the message by @p aLength bytes.
     *
     * @param[in]  aBuf     A pointer to a data buffer.
     * @param[in]  aLength  The number of bytes to prepend.
     *
     * @retval OT_ERROR_NONE     Successfully prepended the bytes.
     * @retval OT_ERROR_NO_BUFS  Not enough reserved bytes in the message.
     *
     */
    otError Prepend(const void *aBuf, uint16_t aLength);

    /**
     * This method removes header bytes from the message.
     *
     * @param[in]  aLength  Number of header bytes to remove.
     *
     * @retval OT_ERROR_NONE  Successfully removed header bytes from the message.
     *
     */
    otError RemoveHeader(uint16_t aLength);

    /**
     * This method appends bytes to the end of the message.
     *
     * On success, this method grows the message by @p aLength bytes.
     *
     * @param[in]  aBuf     A pointer to a data buffer.
     * @param[in]  aLength  The number of bytes to append.
     *
     * @retval OT_ERROR_NONE     Successfully appended the bytes.
     * @retval OT_ERROR_NO_BUFS  Insufficient available buffers to grow the message.
     *
     */
    otError Append(const void *aBuf, uint16_t aLength);

    /**
     * This method reads bytes from the message.
     *
     * @param[in]  aOffset  Byte offset within the message to begin reading.
     * @param[in]  aLength  Number of bytes to read.
     * @param[in]  aBuf     A pointer to a data buffer.
     *
     * @returns The number of bytes read.
     *
     */
    uint16_t Read(uint16_t aOffset, uint16_t aLength, void *aBuf) const;

    /**
     * This method writes bytes to the message.
     *
     * @param[in]  aOffset  Byte offset within the message to begin writing.
     * @param[in]  aLength  Number of bytes to write.
     * @param[in]  aBuf     A pointer to a data buffer.
     *
     * @returns The number of bytes written.
     *
     */
    int Write(uint16_t aOffset, uint16_t aLength, const void *aBuf);

    /**
     * This method copies bytes from one message to another.
     *
     * @param[in] aSourceOffset       Byte offset within the source message to begin reading.
     * @param[in] aDestinationOffset  Byte offset within the destination message to begin writing.
     * @param[in] aLength             Number of bytes to copy.
     * @param[in] aMessage            Message to copy to.
     *
     * @returns The number of bytes copied.
     *
     */
    int CopyTo(uint16_t aSourceOffset, uint16_t aDestinationOffset, uint16_t aLength, Message &aMessage) const;

    /**
     * This method creates a copy of the message.
     *
     * It allocates the new message from the same message pool as the original one and copies @p aLength octets
     * of the payload. The `Type`, `SubType`, `LinkSecurity`, `Offset`, `InterfaceId`, and `Priority` fields on the
     * cloned message are also copied from the original one.
     *
     * @param[in] aLength  Number of payload bytes to copy.
     *
     * @returns A pointer to the message or NULL if insufficient message buffers are available.
     *
     */
    Message *Clone(uint16_t aLength) const;

    /**
     * This method creates a copy of the message.
     *
     * It allocates the new message from the same message pool as the original one and copies the entire payload. The
     * `Type`, `SubType`, `LinkSecurity`, `Offset`, `InterfaceId`, and `Priority` fields on the cloned message are also
     * copied from the original one.
     *
     * @returns A pointer to the message or NULL if insufficient message buffers are available.
     *
     */
    Message *Clone(void) const { return Clone(GetLength()); };

    /**
     * This method returns the datagram tag used for 6LoWPAN fragmentation.
     *
     * @returns The 6LoWPAN datagram tag.
     *
     */
    uint16_t GetDatagramTag(void) const { return mBuffer.mHead.mInfo.mDatagramTag; }

    /**
     * This method sets the datagram tag used for 6LoWPAN fragmentation.
     *
     * @param[in]  aTag  The 6LoWPAN datagram tag.
     *
     */
    void SetDatagramTag(uint16_t aTag) { mBuffer.mHead.mInfo.mDatagramTag = aTag; }

    /**
     * This method returns whether or not the message forwarding is scheduled for the child.
     *
     * @param[in]  aChildIndex  The index into the child table.
     *
     * @retval TRUE   If the message is scheduled to be forwarded to the child.
     * @retval FALSE  If the message is not scheduled to be forwarded to the child.
     *
     */
    bool GetChildMask(uint8_t aChildIndex) const;

    /**
     * This method unschedules forwarding of the message to the child.
     *
     * @param[in]  aChildIndex  The index into the child table.
     *
     */
    void ClearChildMask(uint8_t aChildIndex);

    /**
     * This method schedules forwarding of the message to the child.
     *
     * @param[in]  aChildIndex  The index into the child table.
     *
     */
    void SetChildMask(uint8_t aChildIndex);

    /**
     * This method returns whether or not the message forwarding is scheduled for at least one child.
     *
     * @retval TRUE   If message forwarding is scheduled for at least one child.
     * @retval FALSE  If message forwarding is not scheduled for any child.
     *
     */
    bool IsChildPending(void) const;

    /**
     * This method returns the IEEE 802.15.4 Destination PAN ID.
     *
     * @note Only use this when sending MLE Discover Request or Response messages.
     *
     * @returns The IEEE 802.15.4 Destination PAN ID.
     *
     */
    uint16_t GetPanId(void) const { return mBuffer.mHead.mInfo.mPanIdChannel.mPanId; }

    /**
     * This method sets the IEEE 802.15.4 Destination PAN ID.
     *
     * @note Only use this when sending MLE Discover Request or Response messages.
     *
     * @param[in]  aPanId  The IEEE 802.15.4 Destination PAN ID.
     *
     */
    void SetPanId(uint16_t aPanId) { mBuffer.mHead.mInfo.mPanIdChannel.mPanId = aPanId; }

    /**
     * This method returns the IEEE 802.15.4 Channel to use for transmission.
     *
     * @note Only use this when sending MLE Announce messages.
     *
     * @returns The IEEE 802.15.4 Channel to use for transmission.
     *
     */
    uint8_t GetChannel(void) const { return mBuffer.mHead.mInfo.mPanIdChannel.mChannel; }

    /**
     * This method sets the IEEE 802.15.4 Channel to use for transmission.
     *
     * @note Only use this when sending MLE Announce messages.
     *
     * @param[in]  aChannel  The IEEE 802.15.4 Channel to use for transmission.
     *
     */
    void SetChannel(uint8_t aChannel) { mBuffer.mHead.mInfo.mPanIdChannel.mChannel = aChannel; }

    /**
     * This method returns the timeout used for 6LoWPAN reassembly.
     *
     * @returns The time remaining in seconds.
     *
     */
    uint8_t GetTimeout(void) const { return mBuffer.mHead.mInfo.mTimeout; }

    /**
     * This method sets the timeout used for 6LoWPAN reassembly.
     *
     * @param[in]  aTimeout  The timeout value.
     *
     */
    void SetTimeout(uint8_t aTimeout) { mBuffer.mHead.mInfo.mTimeout = aTimeout; }

    /**
     * This method decrements the timeout.
     *
     */
    void DecrementTimeout(void) { mBuffer.mHead.mInfo.mTimeout--; }

    /**
     * This method returns the interface ID.
     *
     * @returns The interface ID.
     *
     */
    int8_t GetInterfaceId(void) const { return mBuffer.mHead.mInfo.mInterfaceId; }

    /**
     * This method sets the interface ID.
     *
     * @param[in]  aInterfaceId  The interface ID value.
     *
     */
    void SetInterfaceId(int8_t aInterfaceId) { mBuffer.mHead.mInfo.mInterfaceId = aInterfaceId; }

    /**
     * This method returns whether or not message forwarding is scheduled for direct transmission.
     *
     * @retval TRUE   If message forwarding is scheduled for direct transmission.
     * @retval FALSE  If message forwarding is not scheduled for direct transmission.
     *
     */
    bool GetDirectTransmission(void) const { return mBuffer.mHead.mInfo.mDirectTx; }

    /**
     * This method unschedules forwarding using direct transmission.
     *
     */
    void ClearDirectTransmission(void) { mBuffer.mHead.mInfo.mDirectTx = false; }

    /**
     * This method schedules forwarding using direct transmission.
     *
     */
    void SetDirectTransmission(void) { mBuffer.mHead.mInfo.mDirectTx = true; }

    /**
     * This methods indicates whether the direct transmission of message was successful.
     *
     * @retval TRUE   If direct transmission of message was successful (all fragments were delivered and acked).
     * @retval FALSE  If direct transmission of message failed (at least one fragment failed).
     *
     */
    bool GetTxSuccess(void) const { return mBuffer.mHead.mInfo.mTxSuccess; }

    /**
     * This methods sets whether the direct transmission of message was successful.
     *
     * @param[in] aTxSuccess   TRUE if the direct transmission is successful, FALSE otherwise (i.e., at least one
     *                         fragment transmission failed).
     *
     */
    void SetTxSuccess(bool aTxSuccess) { mBuffer.mHead.mInfo.mTxSuccess = aTxSuccess; }

    /**
     * This method indicates whether or not link security is enabled for the message.
     *
     * @retval TRUE   If link security is enabled.
     * @retval FALSE  If link security is not enabled.
     *
     */
    bool IsLinkSecurityEnabled(void) const { return mBuffer.mHead.mInfo.mLinkSecurity; }

    /**
     * This method sets whether or not link security is enabled for the message.
     *
     * @param[in]  aEnabled  TRUE if link security is enabled, FALSE otherwise.
     *
     */
    void SetLinkSecurityEnabled(bool aEnabled) { mBuffer.mHead.mInfo.mLinkSecurity = aEnabled; }

    /**
     * This method updates the average RSS (Received Signal Strength) associated with the message by adding the given
     * RSS value to the average. Note that a message can be composed of multiple 802.15.4 data frame fragments each
     * received with a different signal strength.
     *
     * @param[in] aRss A new RSS value (in dBm) to be added to average.
     *
     */
    void AddRss(int8_t aRss) { mBuffer.mHead.mInfo.mRssAverager.Add(aRss); }

    /**
     * This method returns the average RSS (Received Signal Strength) associated with the message.
     *
     * @returns The current average RSS value (in dBm) or OT_RADIO_RSSI_INVALID if no average is available.
     *
     */
    int8_t GetAverageRss(void) const { return mBuffer.mHead.mInfo.mRssAverager.GetAverage(); }

    /**
     * This method returns a const reference to RssAverager of the message.
     *
     * @returns A const reference to the RssAverager of the message.
     *
     */
    const RssAverager &GetRssAverager(void) const { return mBuffer.mHead.mInfo.mRssAverager; }

    /**
     * This static method updates a checksum.
     *
     * @param[in]  aChecksum  The checksum value to update.
     * @param[in]  aValue     The 16-bit value to update @p aChecksum with.
     *
     * @returns The updated checksum.
     *
     */
    static uint16_t UpdateChecksum(uint16_t aChecksum, uint16_t aValue);

    /**
     * This static method updates a checksum.
     *
     * @param[in]  aChecksum  The checksum value to update.
     * @param[in]  aBuf       A pointer to a buffer.
     * @param[in]  aLength    The number of bytes in @p aBuf.
     *
     * @returns The updated checksum.
     *
     */
    static uint16_t UpdateChecksum(uint16_t aChecksum, const void *aBuf, uint16_t aLength);

    /**
     * This method is used to update a checksum value.
     *
     * @param[in]  aChecksum  Initial checksum value.
     * @param[in]  aOffset    Byte offset within the message to begin checksum computation.
     * @param[in]  aLength    Number of bytes to compute the checksum over.
     *
     * @retval The updated checksum value.
     *
     */
    uint16_t UpdateChecksum(uint16_t aChecksum, uint16_t aOffset, uint16_t aLength) const;

    /**
     * This method returns a pointer to the message queue (if any) where this message is queued.
     *
     * @returns A pointer to the message queue or NULL if not in any message queue.
     *
     */
    MessageQueue *GetMessageQueue(void) const
    {
        return (!mBuffer.mHead.mInfo.mInPriorityQ) ? mBuffer.mHead.mInfo.mQueue.mMessage : NULL;
    }

#if OPENTHREAD_CONFIG_ENABLE_TIME_SYNC
    /**
     * This method indicates whether or not the message is also used for time sync purpose.
     *
     * @retval TRUE   If the message is also used for time sync purpose.
     * @retval FALSE  If the message is not used for time sync purpose.
     *
     */
    bool IsTimeSync(void) const { return mBuffer.mHead.mInfo.mTimeSync; }

    /**
     * This method sets whether or not the message is also used for time sync purpose.
     *
     * @param[in]  aEnabled  TRUE if the message is also used for time sync purpose, FALSE otherwise.
     *
     */
    void SetTimeSync(bool aEnabled) { mBuffer.mHead.mInfo.mTimeSync = aEnabled; }

    /**
     * This method sets the offset to network time.
     *
     * @param[in]  aNetworkTimeOffset  The offset to network time.
     *
     */
    void SetNetworkTimeOffset(int64_t aNetworkTimeOffset)
    {
        mBuffer.mHead.mInfo.mNetworkTimeOffset = aNetworkTimeOffset;
    }

    /**
     * This method gets the offset to network time.
     *
     * @returns  The offset to network time.
     *
     */
    int64_t GetNetworkTimeOffset(void) const { return mBuffer.mHead.mInfo.mNetworkTimeOffset; }

    /**
     * This method sets the time sync sequence.
     *
     * @param[in]  aTimeSyncSeq  The time sync sequence.
     *
     */
    void SetTimeSyncSeq(uint8_t aTimeSyncSeq) { mBuffer.mHead.mInfo.mTimeSyncSeq = aTimeSyncSeq; }

    /**
     * This method gets the time sync sequence.
     *
     * @returns  The time sync sequence.
     *
     */
    uint8_t GetTimeSyncSeq(void) const { return mBuffer.mHead.mInfo.mTimeSyncSeq; }
#endif // OPENTHREAD_CONFIG_ENABLE_TIME_SYNC

private:
    /**
     * This method returns a pointer to the message pool to which this message belongs
     *
     * @returns A pointer to the message pool.
     *
     */
    MessagePool *GetMessagePool(void) const { return mBuffer.mHead.mInfo.mMessagePool; }

    /**
     * This method sets the message pool this message to which this message belongs.
     *
     * @param[in] aMessagePool  A pointer to the message pool
     *
     */
    void SetMessagePool(MessagePool *aMessagePool) { mBuffer.mHead.mInfo.mMessagePool = aMessagePool; }

    /**
     * This method returns `true` if the message is enqueued in any queue (`MessageQueue` or `PriorityQueue`).
     *
     * @returns `true` if the message is in any queue, `false` otherwise.
     *
     */
    bool IsInAQueue(void) const { return (mBuffer.mHead.mInfo.mQueue.mMessage != NULL); }

    /**
     * This method sets the message queue information for the message.
     *
     * @param[in]  aMessageQueue  A pointer to the message queue where this message is queued.
     *
     */
    void SetMessageQueue(MessageQueue *aMessageQueue);

    /**
     * This method returns a pointer to the priority message queue (if any) where this message is queued.
     *
     * @returns A pointer to the priority queue or NULL if not in any priority queue.
     *
     */
    PriorityQueue *GetPriorityQueue(void) const
    {
        return (mBuffer.mHead.mInfo.mInPriorityQ) ? mBuffer.mHead.mInfo.mQueue.mPriority : NULL;
    }

    /**
     * This method sets the message queue information for the message.
     *
     * @param[in]  aPriorityQueue  A pointer to the priority queue where this message is queued.
     *
     */
    void SetPriorityQueue(PriorityQueue *aPriorityQueue);

    /**
     * This method returns a reference to the `mNext` pointer for a given list.
     *
     * @param[in]  aList  The index to the message list.
     *
     * @returns A reference to the mNext pointer for the specified list.
     *
     */
    Message *&Next(uint8_t aList) { return mBuffer.mHead.mInfo.mNext[aList]; }

    /**
     * This method returns a const reference to the `mNext` pointer for a given list.
     *
     * @param[in]  aList  The index to the message list.
     *
     * @returns A const reference to the mNext pointer for the specified list.
     *
     */
    Message *const &Next(uint8_t aList) const { return mBuffer.mHead.mInfo.mNext[aList]; }

    /**
     * This method returns a reference to the `mPrev` pointer for a given list.
     *
     * @param[in]  aList  The index to the message list.
     *
     * @returns A reference to the mPrev pointer for the specified list.
     *
     */
    Message *&Prev(uint8_t aList) { return mBuffer.mHead.mInfo.mPrev[aList]; }

    /**
     * This method returns the number of reserved header bytes.
     *
     * @returns The number of reserved header bytes.
     *
     */
    uint16_t GetReserved(void) const { return mBuffer.mHead.mInfo.mReserved; }

    /**
     * This method sets the number of reserved header bytes.
     *
     * @param[in] aReservedHeader  The number of header bytes to reserve.
     *
     */
    void SetReserved(uint16_t aReservedHeader) { mBuffer.mHead.mInfo.mReserved = aReservedHeader; }

    /**
     * This method adds or frees message buffers to meet the requested length.
     *
     * @param[in]  aLength  The number of bytes that the message buffer needs to handle.
     *
     * @retval OT_ERROR_NONE     Successfully resized the message.
     * @retval OT_ERROR_NO_BUFS  Could not grow the message due to insufficient available message buffers.
     *
     */
    otError ResizeMessage(uint16_t aLength);
};

/**
 * This class implements a message queue.
 *
 */
class MessageQueue : public otMessageQueue
{
    friend class Message;
    friend class PriorityQueue;

public:
    /**
     * This enumeration represents a position (head or tail) in the queue. This is used to specify where a new message
     * should be added in the queue.
     *
     */
    enum QueuePosition
    {
        kQueuePositionHead, ///< Indicates the head (front) of the list.
        kQueuePositionTail, ///< Indicates the tail (end) of the list.
    };

    /**
     * This constructor initializes the message queue.
     *
     */
    MessageQueue(void);

    /**
     * This method returns a pointer to the first message.
     *
     * @returns A pointer to the first message.
     *
     */
    Message *GetHead(void) const;

    /**
     * This method adds a message to the end of the list.
     *
     * @param[in]  aMessage  The message to add.
     *
     * @retval OT_ERROR_NONE     Successfully added the message to the list.
     * @retval OT_ERROR_ALREADY  The message is already enqueued in a list.
     *
     */
    otError Enqueue(Message &aMessage) { return Enqueue(aMessage, kQueuePositionTail); }

    /**
     * This method adds a message at a given position (head/tail) of the list.
     *
     * @param[in]  aMessage  The message to add.
     * @param[in]  aPosition The position (head or tail) where to add the message.
     *
     * @retval OT_ERROR_NONE     Successfully added the message to the list.
     * @retval OT_ERROR_ALREADY  The message is already enqueued in a list.
     *
     */
    otError Enqueue(Message &aMessage, QueuePosition aPosition);

    /**
     * This method removes a message from the list.
     *
     * @param[in]  aMessage  The message to remove.
     *
     * @retval OT_ERROR_NONE       Successfully removed the message from the list.
     * @retval OT_ERROR_NOT_FOUND  The message is not enqueued in a list.
     *
     */
    otError Dequeue(Message &aMessage);

    /**
     * This method returns the number of messages and buffers enqueued.
     *
     * @param[out]  aMessageCount  Returns the number of messages enqueued.
     * @param[out]  aBufferCount   Returns the number of buffers enqueued.
     *
     */
    void GetInfo(uint16_t &aMessageCount, uint16_t &aBufferCount) const;

private:
    /**
     * This method returns the tail of the list (last message in the list)
     *
     * @returns A pointer to the tail of the list.
     *
     */
    Message *GetTail(void) const { return static_cast<Message *>(mData); }

    /**
     * This method set the tail of the list.
     *
     * @param[in]  aMessage  A pointer to the message to set as new tail.
     *
     */
    void SetTail(Message *aMessage) { mData = aMessage; }

    /**
     * This method adds a message to the end of the list.
     *
     * @param[in]  aListId   The list to add @p aMessage to.
     * @param[in]  aMessage  The message to add to @p aListId.
     *
     */
    void AddToList(uint8_t aListId, Message &aMessage) { AddToList(aListId, aMessage, kQueuePositionTail); }

    /**
     * This method adds a message at a give position (head or tail) of the list.
     *
     * @param[in]  aListId   The list to add @p aMessage to.
     * @param[in]  aMessage  The message to add to @p aListId.
     * @param[in]  aPosition The position where to add the message.
     *
     */
    void AddToList(uint8_t aListId, Message &aMessage, QueuePosition aPosition);

    /**
     * This method removes a message from a list.
     *
     * @param[in]  aListId   The list to add @p aMessage to.
     * @param[in]  aMessage  The message to add to @p aListId.
     *
     */
    void RemoveFromList(uint8_t aListId, Message &aMessage);
};

/**
 * This class implements a priority queue.
 *
 */
class PriorityQueue
{
    friend class Message;
    friend class MessageQueue;
    friend class MessagePool;

public:
    /**
     * This constructor initializes the priority queue.
     *
     */
    PriorityQueue(void);

    /**
     * This method returns a pointer to the first message.
     *
     * @returns A pointer to the first message.
     *
     */
    Message *GetHead(void) const;

    /**
     * This method returns a pointer to the first message for a given priority level.
     *
     * @param[in] aPriority   Priority level.
     *
     * @returns A pointer to the first message with given priority level or NULL if there is no messages with
     *          this priority level.
     *
     */
    Message *GetHeadForPriority(uint8_t aPriority) const;

    /**
     * This method adds a message to the queue.
     *
     * @param[in]  aMessage  The message to add.
     *
     * @retval OT_ERROR_NONE     Successfully added the message to the list.
     * @retval OT_ERROR_ALREADY  The message is already enqueued in a list.
     *
     */
    otError Enqueue(Message &aMessage);

    /**
     * This method removes a message from the list.
     *
     * @param[in]  aMessage  The message to remove.
     *
     * @retval OT_ERROR_NONE       Successfully removed the message from the list.
     * @retval OT_ERROR_NOT_FOUND  The message is not enqueued in a list.
     *
     */
    otError Dequeue(Message &aMessage);

    /**
     * This method returns the number of messages and buffers enqueued.
     *
     * @param[out]  aMessageCount  Returns the number of messages enqueued.
     * @param[out]  aBufferCount   Returns the number of buffers enqueued.
     *
     */
    void GetInfo(uint16_t &aMessageCount, uint16_t &aBufferCount) const;

    /**
     * This method returns the tail of the list (last message in the list)
     *
     * @returns A pointer to the tail of the list.
     *
     */
    Message *GetTail(void) const;

private:
    /**
     * This method adds a message to a list.
     *
     * @param[in]  aListId   The list to add @p aMessage to.
     * @param[in]  aMessage  The message to add to @p aListId.
     *
     */
    void AddToList(uint8_t aListId, Message &aMessage);

    /**
     * This method removes a message from a list.
     *
     * @param[in]  aListId   The list to add @p aMessage to.
     * @param[in]  aMessage  The message to add to @p aListId.
     *
     */
    void RemoveFromList(uint8_t aListId, Message &aMessage);

    /**
     * This method increases (moves forward) the given priority while ensuring to wrap from
     * priority value `kNumPriorities` -1 back to 0.
     *
     * @param[in] aPriority  A given priority level
     *
     * @returns Increased/Moved forward priority level
     */
    uint8_t PrevPriority(uint8_t aPriority) const
    {
        return (aPriority == Message::kNumPriorities - 1) ? 0 : (aPriority + 1);
    }

    /**
     * This private method finds the first non-NULL tail starting from the given priority level and moving forward.
     * It wraps from priority value `kNumPriorities` -1 back to 0.
     *
     * aStartPriorityLevel  Starting priority level.
     *
     * @returns The first non-NULL tail pointer, or NULL if all the
     *
     */
    Message *FindFirstNonNullTail(uint8_t aStartPriorityLevel) const;

private:
    Message *mTails[Message::kNumPriorities]; ///< Tail pointers associated with different priority levels.
};

/**
 * This class represents a message pool
 *
 */
class MessagePool : public InstanceLocator
{
    friend class Message;
    friend class MessageQueue;
    friend class PriorityQueue;

public:
    /**
     * This class represents an iterator for iterating through all queued message from this pool.
     *
     */
    class Iterator
    {
        friend class MessagePool;

    public:
        /**
         * This constructor initializes an empty iterator.
         */
        Iterator(void)
            : mMessage(NULL)
        {
        }

        /**
         * This method returns the associated message with the iterator.
         *
         * @returns A pointer to associated message with this iterator.
         *
         */
        Message *GetMessage(void) const { return mMessage; }

        /**
         * This method returns `true` if the iterator is empty (i.e., associated with a NULL message)
         *
         * @returns `true` if the iterator is empty, `false` otherwise.
         */
        bool IsEmpty(void) const { return (mMessage == NULL); }

        /**
         * This method returns `true` if the iterator has ended (beyond the last message on list).
         *
         * @returns `true` if the iterator has ended , `false` otherwise.
         */
        bool HasEnded(void) const { return IsEmpty(); }

        /**
         * This method returns a new iterator corresponding to next message on the list.
         *
         * @returns An iterator corresponding to next message on the list.
         *
         */
        Iterator GetNext(void) const { return Iterator(Next()); }

        /**
         * This method returns a new iterator corresponding to previous message on the list.
         *
         * @returns An iterator corresponding to previous message on the list.
         *
         */
        Iterator GetPrev(void) const { return Iterator(Prev()); }

        /**
         * This method moves the current iterator to the next message on the list.
         *
         * @returns A reference to current iterator.
         *
         */
        Iterator &GoToNext(void)
        {
            mMessage = Next();
            return *this;
        }

        /**
         * This method moves the current iterator to the previous message on the list.
         *
         * @returns A reference to current iterator.
         *
         */
        Iterator &GoToPrev(void)
        {
            mMessage = Prev();
            return *this;
        }

    private:
        Iterator(Message *aMessage)
            : mMessage(aMessage)
        {
        }
        Message *Next(void) const;
        Message *Prev(void) const;

        Message *mMessage;
    };

    /**
     * This constructor initializes the object.
     *
     */
    explicit MessagePool(Instance &aInstance);

    /**
     * This method is used to obtain a new message. The default priority `kDefaultMessagePriority`
     * is assigned to the message.
     *
     * @param[in]  aType           The message type.
     * @param[in]  aReserveHeader  The number of header bytes to reserve.
     * @param[in]  aPriority       The priority level of the message.
     *
     * @returns A pointer to the message or NULL if no message buffers are available.
     *
     */
    Message *New(uint8_t aType, uint16_t aReserveHeader, uint8_t aPriority = kDefaultMessagePriority);

    /**
     * This method is used to obtain a new message with specified settings.
     *
     * @note If @p aSettings is 'NULL', the link layer security is enabled and the message priority is set to
     * OT_MESSAGE_PRIORITY_NORMAL by default.
     *
     * @param[in]  aType           The message type.
     * @param[in]  aReserveHeader  The number of header bytes to reserve.
     * @param[in]  aSettings       A pointer to the message settings or NULL to set default settings.
     *
     * @returns A pointer to the message or NULL if no message buffers are available.
     *
     */
    Message *New(uint8_t aType, uint16_t aReserveHeader, const otMessageSettings *aSettings);

    /**
     * This method is used to free a message and return all message buffers to the buffer pool.
     *
     * @param[in]  aMessage  The message to free.
     *
     */
    void Free(Message *aMessage);

    /**
     * This method returns a pointer to the first message (head) in the all-messages list.
     * Messages are sorted based on their priority (head with highest priority) and order by which they are enqueued.
     *
     * @returns A pointer to the first message.
     *
     */
    Iterator GetAllMessagesHead(void) const;

    /**
     * This method returns a pointer to the last message (head) in the all-messages list.
     * Messages are sorted based on their priority (head with highest priority) and order by which they are enqueued.
     *
     * @returns A pointer to the last message.
     *
     */
    Iterator GetAllMessagesTail(void) const { return Iterator(mAllQueue.GetTail()); }

    /**
     * This method returns the number of free buffers.
     *
     * @returns The number of free buffers.
     *
     */
    uint16_t GetFreeBufferCount(void) const;

private:
    enum
    {
        kDefaultMessagePriority = Message::kPriorityNormal,
    };

    Buffer *       NewBuffer(uint8_t aPriority);
    void           FreeBuffers(Buffer *aBuffer);
    otError        ReclaimBuffers(int aNumBuffers, uint8_t aPriority);
    PriorityQueue *GetAllMessagesQueue(void) { return &mAllQueue; }

#if OPENTHREAD_CONFIG_PLATFORM_MESSAGE_MANAGEMENT == 0
    uint16_t mNumFreeBuffers;
    Buffer   mBuffers[kNumBuffers];
    Buffer * mFreeBuffers;
#endif

    PriorityQueue mAllQueue;
};

/**
 * @}
 *
 */

} // namespace ot

#endif // MESSAGE_HPP_
