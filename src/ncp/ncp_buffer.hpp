/*
 *    Copyright (c) 2016, The OpenThread Authors.
 *    All rights reserved.
 *
 *    Redistribution and use in source and binary forms, with or without
 *    modification, are permitted provided that the following conditions are met:
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *    3. Neither the name of the copyright holder nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 *    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 *    ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 *    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
 *    DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 *    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 *    ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 *    SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file
 *   This file contains definitions for the NCP frame buffer class.
 */

#ifndef NCP_FRAME_BUFFER_HPP_
#define NCP_FRAME_BUFFER_HPP_

#include <openthread/openthread_config.h>
#include <openthread/message.h>
#include <openthread/types.h>

namespace ot {

class NcpFrameBuffer
{
public:
    /**
     * Defines the frame tag type. Frame tags can be compared with one another using operator `==`.
     *
     */
    typedef const void *FrameTag;

    /**
     * Defines the tag to indicate an invalid tag (e.g., when there is no frame).
     *
     */
    static const FrameTag kInvalidTag;

    /**
     * Defines a function pointer callback which is invoked to inform a change in `NcpFrameBuffer` either when a new
     * frame is added/written to `NcpFrameBuffer` or when a frame is removed from `NcpFrameBuffer`.
     *
     * @param[in]  aContext         A pointer to arbitrary context information.
     * @param[in]  aTag             The tag associated with the frame which is added or removed.
     * @param[in]  aNcpFrameBuffer  A pointer to the `NcpFrameBuffer`.
     *
     */
    typedef void (*BufferCallback)(void *aContext, FrameTag aTag, NcpFrameBuffer *aNcpFrameBuffer);

    /**
     * This constructor creates an NCP frame buffer.
     *
     * @param[in]  aBuffer        A pointer to a buffer which will be used by NCP frame buffer.
     * @param[in]  aBufferLength  The buffer size (in bytes).
     *
     */
    NcpFrameBuffer(uint8_t *aBuffer, uint16_t aBufferLength);

    /**
     * This method clears the NCP frame buffer. All the frames are cleared/removed.
     *
     */
    void Clear(void);

    /**
     * This method sets the `FrameAdded` callback and its context.
     *
     * Subsequent calls to this method will overwrite the previous callback and its context.
     *
     * @param[in]  aFrameAddedCallback      Callback invoked when a new frame is successfully added to buffer.
     * @param[in]  aFrameAddedContext       A pointer to arbitrary context used with frame added callback.
     *
     */
    void SetFrameAddedCallback(BufferCallback aFrameAddedCallback, void *aFrameAddedContext);

    /**
     * This method sets the `FrameRemoved` callback and its context.
     *
     * Subsequent calls to this method will overwrite the previous callback and its context.
     *
     * @param[in]  aFrameRemovedCallback    Callback invoked when a frame is removed from buffer.
     * @param[in]  aFrameRemovedContext     A pointer to arbitrary context used with frame removed callback.
     *
     */
    void SetFrameRemovedCallback(BufferCallback aFrameRemovedCallback, void *aFrameRemovedContext);

    /**
     * This method begins a new input frame to be added/written to the frame buffer.

     * If there is a previous frame being written (`InFrameEnd()` has not yet been called on the frame), this method
     * will discard and clear the previous unfinished frame.
     *
     * @retval OT_ERROR_NONE      Successfully started a new frame.
     * @retval OT_ERROR_NO_BUFS   Insufficient buffer space available to start a new frame.
     *
     */
    otError InFrameBegin(void);

    /**
     * This method adds data to the current input frame being written to the buffer.
     *
     * If no buffer space is available, this method will discard and clear the frame before returning an error status.
     *
     * @param[in]  aDataBuffer        A pointer to data buffer.
     * @param[in]  aDataBufferLength  The length of the data buffer.
     *
     * @retval OT_ERROR_NONE      Successfully added new data to the frame.
     * @retval OT_ERROR_NO_BUFS   Insufficient buffer space available to add data.
     *
     */
    otError InFrameFeedData(const uint8_t *aDataBuffer, uint16_t aDataBufferLength);

    /**
     * This method adds a message to the current input frame being written to the buffer.
     *
     * If no buffer space is available, this method will discard and clear the frame before returning an error status.
     * In case of success, the passed-in message @p aMessage will be owned by the frame buffer instance and will be
     * freed when either the the frame is removed or discarded.
     *
     * @param[in]  aMessage         A  message to be added to current frame.
     *
     * @retval OT_ERROR_NONE     Successfully added the message to the frame.
     * @retval OT_ERROR_NO_BUFS  Insufficient buffer space available to add message.
     *
     */
    otError InFrameFeedMessage(otMessage *aMessage);

    /**
     * This method finalizes/ends the current input frame being written to the buffer.
     *
     * If no buffer space is available, this method will discard and clear the frame before returning an error status.
     *
     * @retval OT_ERROR_NONE     Successfully added the message to the frame.
     * @retval OT_ERROR_NO_BUFS  Insufficient buffer space available to add message.
     *
     */
    otError InFrameEnd(void);

    /**
     * This method returns the tag assigned to last successfully written/added frame to NcpBuffer (i.e., last input
     * frame for which `InFrameEnd()` was called and returned success status). The tag is a unique value (within
     * currently queued frames) associated with each frame in the `NcpFrameBuffer`. The tag can be used to identify the
     * same frame when it is read and removed from the NcpBuffer. Tags can be compared using operator `==`.
     *
     * @returns The tag of last successfully written frame, or `kInvalidTag` if no frame is written so far.
     */
    FrameTag InFrameGetLastTag(void) const;

    /**
     * This method checks if the buffer is empty. An non-empty buffer contains at least one full frame for reading.
     *
     * @retval true                 Buffer is not empty and contains at least one full frame for reading.
     * @retval false                Buffer is empty and contains no frame for reading.
     *
     */
    bool IsEmpty(void) const;

    /**
     * This method begins/prepares a new output frame to be read from the frame buffer.
     *
     * The NCP buffer maintains a read offset for the current frame being read. Before reading any bytes from the frame
     * this method should be called to prepare the frame and set the read offset.
     *
     * If part of current frame has already been read, a sub-sequent call to this method will reset the read offset
     * back to beginning of current output frame.
     *
     * @retval OT_ERROR_NONE       Successfully started/prepared a new output frame for reading.
     * @retval OT_ERROR_NOT_FOUND  No frame available in buffer for reading.
     *
     */
    otError OutFrameBegin(void);

    /**
     * This method checks if the current output frame (being read) has ended.
     *
     * The NCP buffer maintains a read offset for the current output frame being read. This method returns true if the
     * read offset is at the end of the frame and there are no more bytes available to read from current frame.
     *
     * @retval true                   Frame has ended (no more bytes available to read from current output frame).
     * @retval false                  Frame has data (can read more bytes from current output frame).
     *
     */
    bool OutFrameHasEnded(void);

    /**
     * This method reads and returns the next byte from the current output frame.
     *
     * The NCP buffer maintains a read offset for the current output frame being read. This method reads and returns
     * the next byte from the current frame and moves the read offset forward. If read offset is already at the end
     * current output frame, this method returns zero.
     *
     * @returns     The next byte from the current output frame or zero if frame has ended.
     *
     */
    uint8_t OutFrameReadByte(void);

    /**
     * This method reads bytes from the current output frame.
     *
     * The NCP buffer maintains a read offset for the current output frame being read. This method attempts to read
     * the given number of bytes (@p aDataBufferLength) from the current frame and copies the bytes into the given
     * data buffer (@p aDataBuffer). It also moves the read offset forward accordingly. If there are less bytes
     * remaining in current frame, the available bytes are read/copied. This methods returns the actual number of bytes
     * read.
     *
     * @param[in]  aDataBuffer        A pointer to a data buffer.
     * @param[in]  aReadLength        Number of bytes to read.
     *
     * @returns    The number of bytes read and copied into data buffer.
     *
     */
    uint16_t OutFrameRead(uint16_t aReadLength, uint8_t *aDataBuffer);

    /**
     * This method removes the current/front output frame from the buffer.
     *
     * The NCP buffer stores the frames in FIFO order. This method removes the front frame (which may be the current
     * output frame being read) from the buffer. There is no need to prepare/begin reading the current frame before
     * removing it, so the front frame can be removed without a previous call to `OutFrameBegin()`.
     *
     * When a frame is removed all its associated messages will be freed.
     *
     * If the remove operation is successful, this method will invoke the `FrameRemovedCallback` (if provided).
     *
     * @retval OT_ERROR_NONE       Successfully removed the front frame.
     * @retval OT_ERROR_NOT_FOUND  No frame available in NCP frame buffer to remove.
     *
     */
    otError OutFrameRemove(void);

    /**
     * This method returns the number of bytes (length) of current/front frame in the NCP frame buffer.
     *
     * The NCP buffer stores the frames in FIFO order. This method returns the length of the front frame (which may
     * be the current output frame being read) from the buffer. There is no need to prepare/begin reading the current
     * frame before calling this method so this method can be used without a previous call to `OutFrameBegin()`.
     *
     * If there is no frame in buffer, this method returns zero.
     *
     * @returns    The number of bytes (length) of current/front frame, or zero if no frame in buffer.
     *
     */
    uint16_t OutFrameGetLength(void);

    /**
     * This method returns the tag value associated to current/front frame in the NCP frame buffer.
     *
     * The NCP buffer stores the frames in FIFO order. This method returns the tag of the front frame (which may
     * be the current output frame being read) from the buffer. There is no need to prepare/begin reading the current
     * frame before calling this method so this method can be used without a previous call to `OutFrameBegin()`.
     *
     * If there is no frame in buffer, this method returns `kInvalidTag`.
     *
     * @returns    The tag assigned to the current/from output frame, or `kInvalidTag` if no frame in buffer.
     *
     */
    FrameTag OutFrameGetTag(void) const;

private:

    /*
     * NcpFrameBuffer Implementation
     * -----------------------------
     *
     * NcpFrameBuffer internally stores a frame as a sequence of data segments. The data segments are stored in the
     * the main buffer `mBuffer`. mBuffer is utilized as a circular buffer.

     * Messages (which are added using `InFrameFeedMessaged()`) are not copied in the `mBuffer` but instead are
     * enqueued in a message queue `mMessageQueue`.
     *
     * The data segments include a header before the data portion. The header is 2 bytes long is formated as follows
     *
     *    Bit 0-13: Give the length of the data segment (max segment len is 2^14 = 16,384 bytes).
     *    Bit 14:   Flag bit set to indicate that this segment has an associated `Message` (appended to its end).
     *    Bit 15:   Flag bit set to indicate that this segment defines the start of a new frame.
     *
     *        Bit  15         Bit 14            Bits: 0 - 13
     *    +--------------+--------------+--------------------------------------------------------+
     *    |   New Frame  |  Has Message |  Length of segment (excluding the header)              |
     *    +--------------+--------------+--------------------------------------------------------+
     *
     * The header is encoded in big-endian (msb first) style.

     * Consider the following calls to create a frame:
     *
     *    ncpBuffer.InFrameBegin();
     *    ncpBuffer.InFrameFeedData("Hello", 5);
     *    ncpBuffer.InFrameFeedData("There", 5);
     *    ncpBuffer.InFrameFeedMessage(*someMessage);
     *    ncpBuffer.InFrameFeedData("Bye", 3);
     *    ncpBuffer.InFrameEnd();
     *
     * This frame is stored as two segments:
     *
     *    - Segment #1 contains "HelloThere" with a header of `0xC00A` which shows that this segment contains 10 data
     *      bytes, and it starts a new frame, and also must include a message from the message queue.
     *
     *    - Segment #2 contains "Bye" with a header value of `0x0003` showing length of 3 and no appended message.
     *
     *    +-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
     *    | C0  | 0A  | `H` | 'e' | 'l' | 'l' | 'o' | 'T' | 'h' | 'e' | 'r' | 'e' | 00  | 03  | 'B' | 'y' | 'e' |
     *    +-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
     *     \         /                                                             \         /
     *   Segment #1 Header                                                      Segment #2 Header
     *
     *
     * Buffer pointers:
     *
     *          mReadFrameStart
     *          |
     *          |         mReadSegmentHead                             mWriteFrameStart
     *          |         |                                            |
     *          |         |   mReadPointer                             |         mWriteSegmentHead
     *          |         |   |                                        |         |
     *  mBuffer |         |   |     mReadSegmentTail                   |         |         mWriteSegmentTail mBufferEnd
     *  |       |         |   |     |                                  |         |         |                      |
     *  V       V         V   V     V                                  V         V         V                      V
     * +-------+---------+---------+-------+--------------------------+---------+---------+----------------------+-
     * | ...   | Seg 1   | Seg 2   | Seg 3 |   . . .                  | Seg 1   | Seg 2   :   . . .              |
     * +-------+---------+---------+-------+---------------------------+---------+--------+----------------------+-
     *         \          \        /       /                           \                  /
     *         |         Cur segment      |                            |                 |
     *         |                          |                            |                 |
     *        Current OutFrame (being read)                      Current InFrame (being written)
     *
     */

    enum
    {
        kReadByteAfterFrameHasEnded        = 0,          // Value returned by ReadByte() when frame has ended.
        kMessageReadBufferSize             = 16,         // Size of message buffer array.
        kUnknownFrameLength                = 0xffff,     // Value used when frame length is unknown.
        kSegmentHeaderSize                 = 2,          // Length of the segment header.
        kSegmentHeaderLengthMask           = 0x3fff,     // Bit mask to get the length from the segment header

        kSegmentHeaderNoFlag               = 0,          // No flags are set.
        kSegmentHeaderNewFrameFlag         = (1 << 15),  // Indicates that this segment starts a new frame.
        kSegmentHeaderMessageIndicatorFlag = (1 << 14),  // Indicates this segment ends with a Message.
    };

    enum ReadState
    {
        kReadStateInSegment,        // In middle of a data segment while reading current (out) frame.
        kReadStateInMessage,        // In middle of a message while reading current (out) frame.
        kReadStateDone,             // Current (out) frame is read fully.
    };

    // Private methods

    uint8_t *       Next(uint8_t *aBufferPtr) const;
    uint8_t *       Advance(uint8_t *aBufPtr, uint16_t aOffset) const;
    uint16_t        GetDistance(uint8_t *aStartPtr, uint8_t *aEndPtr) const;

    uint16_t        ReadUint16At(uint8_t *aBufPtr);
    void            WriteUint16At(uint8_t *aBufPtr, uint16_t aValue);

    otError         InFrameFeedByte(uint8_t aByte);
    otError         InFrameBeginSegment(void);
    void            InFrameEndSegment(uint16_t aSegmentHeaderFlags);
    void            InFrameDiscard(void);

    otError         OutFramePrepareSegment(void);
    void            OutFrameMoveToNextSegment(void);
    otError         OutFramePrepareMessage(void);
    otError         OutFrameFillMessageBuffer(void);

    uint8_t * const  mBuffer;                    // Pointer to the buffer used to store the data.
    uint8_t * const  mBufferEnd;                 // Points to after the end of buffer.
    const uint16_t   mBufferLength;              // Length of the the buffer.

    BufferCallback   mFrameAddedCallback;        // Callback to signal when a new frame is added
    void *           mFrameAddedContext;         // Context passed to `mFrameAddedCallback`.
    BufferCallback   mFrameRemovedCallback;      // Callback to signal when a frame is removed.
    void *           mFrameRemovedContext;       // Context passed to `mFrameRemovedCallback`.

    otMessageQueue   mMessageQueue;              // Main message queue.

    otMessageQueue   mWriteFrameMessageQueue;    // Message queue for the current frame being written.
    uint8_t *        mWriteFrameStart;           // Pointer to start of current frame being written.
    uint8_t *        mWriteSegmentHead;          // Pointer to start of current segment in the frame being written.
    uint8_t *        mWriteSegmentTail;          // Pointer to end of current segment in the frame being written.
    FrameTag         mWriteFrameTag;             // Tag associated with last successfully written frame.

    ReadState        mReadState;                 // Read state.
    uint16_t         mReadFrameLength;           // Length of current frame being read.

    uint8_t *        mReadFrameStart;            // Pointer to start of current frame being read.
    uint8_t *        mReadSegmentHead;           // Pointer to start of current segment in the frame being read.
    uint8_t *        mReadSegmentTail;           // Pointer to end of current segment in the frame being read.
    uint8_t *        mReadPointer;               // Pointer to next byte to read (either in segment or in msg buffer).

    otMessage *      mReadMessage;               // Current Message in the frame being read.
    uint16_t         mReadMessageOffset;         // Offset within current message being read.

    uint8_t          mMessageBuffer[kMessageReadBufferSize];   // Buffer to hold part of current message being read.
    uint8_t *        mReadMessageTail;           // Pointer to end of current part in mMessageBuffer.
};

}  // namespace ot

#endif  // NCP_FRAME_BUFFER_HPP_
