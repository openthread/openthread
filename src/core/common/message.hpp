/*
 *  Copyright (c) 2016, Nest Labs, Inc.
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

#include <stdint.h>
#include <string.h>

#include <common/checksum.hpp>
#include <openthread-types.h>
#include <openthread-core-config.h>
#include <common/code_utils.hpp>
#include <mac/mac_frame.hpp>

namespace Thread {

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

/**
 * This structure contains pointers to the head and tail of a Message list.
 *
 */
struct MessageList
{
    Message *mHead;  ///< A pointer to the first Message in the list.
    Message *mTail;  ///< A pointer to the last Message in the list.
};

/**
 * This structure contains pointers to the MessageList structure, the next Message, and previous Message.
 *
 */
struct MessageListEntry
{
    struct MessageList *mList;  ///< A pointer to the MessageList structure for the list.
    Message            *mNext;  ///< A pointer to the next Message in the list.
    Message            *mPrev;  ///< A pointer to the pprevious Message in the list.
};

/**
 * This structure contains a pointer to the next Message buffer.
 *
 */
struct BufferHeader
{
    class Buffer *mNext;  ///< A pointer to the next Message buffer.
};

/**
 * This structure contains metdata about a Message.
 *
 */
struct MessageInfo
{
    enum
    {
        kListAll = 0,                    ///< Identifies the all messages list.
        kListInterface = 1,              ///< Identifies the per-inteface message list.
    };
    MessageListEntry mList[2];           ///< Message lists.
    uint16_t         mReserved;          ///< Number of header bytes reserved for the message.
    uint16_t         mLength;            ///< Number of bytes within the message.
    uint16_t         mOffset;            ///< A byte offset within the message.
    uint16_t         mDatagramTag;       ///< The datagram tag used for 6LoWPAN fragmentation.
    uint8_t          mTimeout;           ///< Seconds remaining before dropping the message.

    uint8_t          mChildMask[8];      ///< A bit-vector to indicate which sleepy children need to receive this message.

    uint8_t          mType : 2;          ///< Identifies the type of message.
    bool             mDirectTx : 1;      ///< Used to indicate whether a direct transmission is required.
    bool             mSecurityValid : 1; ///< Indicates whether received frames were secure and passed validation.
};

/**
 * This class represents a Message buffer.
 *
 */
class Buffer
{
    friend class Message;

public:
    /**
     * This method returns a pointer to the next message buffer.
     *
     * @returns A pointer to the next message buffer.
     *
     */
    class Buffer *GetNextBuffer(void) const { return mHeader.mNext; }

    /**
     * This method sets the pointer to the next message buffer.
     *
     */
    void SetNextBuffer(class Buffer *buf) { mHeader.mNext = buf; }

private:
    /**
     * This method returns a pointer to the first byte of data in the first message buffer.
     *
     * @returns A pointer to the first data byte.
     *
     */
    uint8_t *GetFirstData(void) { return mHeadData; }

    /**
     * This method returns a pointer to the first byte of data in the first message buffer.
     *
     * @returns A pointer to the first data byte.
     *
     */
    const uint8_t *GetFirstData(void) const { return mHeadData; }

    /**
     * This method returns a pointer to the first data byte of a subsequent message buffer.
     *
     * @returns A pointer to the first data byte.
     *
     */
    uint8_t *GetData(void) { return mData; }

    /**
     * This method returns a pointer to the first data byte of a subsequent message buffer.
     *
     * @returns A pointer to the first data byte.
     *
     */
    const uint8_t *GetData(void) const { return mData; }

    enum
    {
        kBufferDataSize = kBufferSize - sizeof(struct BufferHeader),
        kHeadBufferDataSize = kBufferDataSize - sizeof(struct MessageInfo),
    };

    struct BufferHeader mHeader;
    union
    {
        struct
        {
            MessageInfo mInfo;
            uint8_t mHeadData[kHeadBufferDataSize];
        };
        uint8_t mData[kBufferDataSize];
    };
};

/**
 * This class represents a message.
 *
 */
class Message: private Buffer
{
    friend class MessageQueue;

public:
    enum
    {
        kTypeIp6         = 0,   ///< A full uncompress IPv6 packet
        kType6lowpan     = 1,   ///< A 6lowpan frame
        kTypeMacDataPoll = 2,   ///< A MAC data poll message
    };

    /**
     * This method returns a pointer to the next message in the same interface list.
     *
     * @returns A pointer to the next message in the same interface list.
     *
     */
    Message *GetNext(void) const;

    /**
     * This method returns the number of bytes in the message.
     *
     * @returns The number of bytes in the message.
     */
    uint16_t GetLength(void) const;

    /**
     * This method sets the number of bytes in the message.
     *
     * @param[in]  aLength  Requested number of bytes in the message.
     *
     * @retval kThreadError_None    Successfully set the length of the message.
     * @retval kThreadError_NoBufs  Failed to grow the size of the message because insufficient buffers were
     *                              available.
     *
     */
    ThreadError SetLength(uint16_t aLength);

    /**
     * This method returns the byte offset within the message.
     *
     * @returns A byte offset within the message.
     *
     */
    uint16_t GetOffset(void) const;

    /**
     * This method moves the byte offset within the message.
     *
     * @param[in]  aDelta  The number of bytes to move the current offset, which may be positive or negative.
     *
     * @retval kThreadError_None         Successfully moved the byte offset.
     * @retval kThreadError_InvalidArgs  The resulting byte offset is not within the existing message.
     *
     */
    ThreadError MoveOffset(int aDelta);

    /**
     * This method sets the byte offset within the message.
     *
     * @param[in]  aOffset  The number of bytes to move the current offset, which may be positive or negative.
     *
     * @retval kThreadError_None         Successfully moved the byte offset.
     * @retval kThreadError_InvalidArgs  The requestd byte offset is not within the existing message.
     *
     */
    ThreadError SetOffset(uint16_t aOffset);

    /**
     * This method returns the type of the message.
     *
     * @returns The type of the message.
     *
     */
    uint8_t GetType(void) const;

    /**
     * This method prepends bytes to the front of the message.
     *
     * On success, this method grows the message by @p aLength bytes.
     *
     * @param[in]  aBuf     A pointer to a data buffer.
     * @param[in]  aLength  The number of bytes to prepend.
     *
     * @retval kThreadError_None    Successfully prepended the bytes.
     * @retval kThreadError_NoBufs  Not enough reserved bytes in the message.
     *
     */
    ThreadError Prepend(const void *aBuf, uint16_t aLength);

    /**
     * This method appends bytes to the end of the message.
     *
     * On success, this method grows the message by @p aLength bytes.
     *
     * @param[in]  aBuf     A pointer to a data buffer.
     * @param[in]  aLength  The number of bytes to append.
     *
     * @retval kThreadError_None    Successfully appended the bytes.
     * @retval kThreadError_NoBufs  Insufficient available buffers to grow the message.
     *
     */
    ThreadError Append(const void *aBuf, uint16_t aLength);

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
    int Read(uint16_t aOffset, uint16_t aLength, void *aBuf) const;

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
    int CopyTo(uint16_t aSourceOffset, uint16_t aDestinationOffset, uint16_t aLength, Message &aMessage);

    /**
     * This method returns the datagram tag used for 6LoWPAN fragmentation.
     *
     * @returns The 6LoWPAN datagram tag.
     *
     */
    uint16_t GetDatagramTag(void) const;

    /**
     * This method sets the datagram tag used for 6LoWPAN fragmentation.
     *
     * @param[in]  aTag  The 6LoWPAN datagram tag.
     *
     */
    void SetDatagramTag(uint16_t aTag);

    /**
     * This method returns the timeout used for 6LoWPAN reassembly.
     *
     * @returns The time remaining in seconds.
     *
     */
    uint8_t GetTimeout(void) const;

    /**
     * This method sets the timeout used for 6LoWPAN reassembly.
     *
     */
    void SetTimeout(uint8_t aTimeout);

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
     * This method returns whether or not message forwarding is scheduled for direct transmission.
     *
     * @retval TRUE   If message forwarding is scheduled for direct transmission.
     * @retval FALSE  If message forwarding is not scheduled for direct transmission.
     *
     */
    bool GetDirectTransmission(void) const;

    /**
     * This method unschedules forwarding using direct transmission.
     *
     */
    void ClearDirectTransmission(void);

    /**
     * This method schedules forwarding using direct transmission.
     *
     */
    void SetDirectTransmission(void);

    /**
     * This method indicates whether or not the message was secure and passed validation at the link layer.
     *
     * @retval TRUE   If the message was secure and passed validation at the link layer.
     * @retval FALSE  If the message was not secure or did not pass validation at the link layer.
     *
     */
    bool GetSecurityValid(void) const;

    /**
     * This method sets whether or not the message was secure and passed validation at the link layer.
     *
     * @param[in]  aSecurityValid  TRUE if the message was secure and passed link layer validation, FALSE otherwise.
     *
     */
    void SetSecurityValid(bool aSecurityValid);

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
     * This static method is used to initialize the message buffer pool.
     *
     */
    static ThreadError Init(void);

    /**
     * This static method is used to obtain a new message.
     *
     * @param[in]  aType           The message type.
     * @param[in]  aReserveHeader  The number of header bytes to reserve.
     *
     * @returns A pointer to the message or NULL if no message buffers are available.
     *
     */
    static Message *New(uint8_t aType, uint16_t aReserveHeader);

    /**
     * This static method is used to free a message and return all message buffers to the buffer pool.
     *
     * @param[in]  aMessage  The message to free.
     *
     * @retval kThreadError_None         Successfully freed the message.
     * @retval kThreadError_InvalidArgs  The message is already freed.
     *
     */
    static ThreadError Free(Message &aMessage);

private:
    /**
     * This method returns a reference to a message list.
     *
     * @param[in]  aList  The message list.
     *
     * @returns A reference to a message lis.
     *
     */
    MessageListEntry &GetMessageList(uint8_t aList) { return mInfo.mList[aList]; }

    /**
     * This method returns a reference to a message list.
     *
     * @param[in]  aList  The message list.
     *
     * @returns A reference to a message lis.
     *
     */
    const MessageListEntry &GetMessageList(uint8_t aList) const { return mInfo.mList[aList]; }

    /**
     * This method returns the number of reserved header bytes.
     *
     * @returns The number of reserved header bytes.
     *
     */
    uint16_t GetReserved(void) const;

    /**
     * This method sets the number of reserved header bytes.
     *
     * @pram[in]  aReservedHeader  The number of header bytes to reserve.
     *
     */
    void SetReserved(uint16_t aReservedHeader);

    /**
     * This method sets the message type.
     *
     * @param[in]  aType  The message type.
     *
     */
    void SetType(uint8_t aType);

    /**
     * This method adds or frees message buffers to meet the requested length.
     *
     * @param[in]  aLength  The number of bytes that the message buffer needs to handle.
     *
     * @retval kThreadError_None          Successfully resized the message.
     * @retval kThreadError_InvalidArags  Could not grow the message due to insufficient available message buffers.
     *
     */
    ThreadError ResizeMessage(uint16_t aLength);
};

/**
 * This class implements a message queue.
 *
 */
class MessageQueue
{
public:
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
     * @retval kThreadError_None  Successfully added the message to the list.
     * @retval kThreadError_Busy  The message is already enqueued in a list.
     *
     */
    ThreadError Enqueue(Message &aMessage);

    /**
     * This method removes a message from the list.
     *
     * @param[in]  aMessage  The message to remove.
     *
     * @retval kThreadError_None  Successfully removed the message from the list.
     * @retval kThreadError_Busy  The message is not enqueued in a list.
     *
     */
    ThreadError Dequeue(Message &aMessage);

private:
    /**
     * This static method adds a message to a list.
     *
     * @param[in]  aListId   The list to add @p aMessage to.
     * @param[in]  aMessage  The message to add to @p aListId.
     *
     * @retval kThreadError_None  Successfully added the message to the list.
     * @retval kThreadError_Busy  The message is already enqueued in a list.
     *
     */
    static ThreadError AddToList(int aListId, Message &aMessage);

    /**
     * This static method removes a message from a list.
     *
     * @param[in]  aListId   The list to add @p aMessage to.
     * @param[in]  aMessage  The message to add to @p aListId.
     *
     * @retval kThreadError_None  Successfully added the message to the list.
     * @retval kThreadError_Busy  The message is not enqueued in the list.
     *
     */
    static ThreadError RemoveFromList(int aListId, Message &aMessage);

    MessageList mInterface;   ///< The instance-specific message list.
};

/**
 * @}
 *
 */

}  // namespace Thread

#endif  // MESSAGE_HPP_
