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
 *   This file implements NCP frame buffer class.
 */

#include <common/code_utils.hpp>
#include <ncp/ncp_buffer.hpp>

namespace Thread {

NcpFrameBuffer::NcpFrameBuffer(uint8_t *aBuffer, uint16_t aBufferLen) :
    mBuffer(aBuffer),
    mBufferEnd(aBuffer + aBufferLen),
    mBufferLength(aBufferLen)
{
    SetCallbacks(NULL, NULL, NULL);
    Clear();
}

NcpFrameBuffer::~NcpFrameBuffer()
{
    SetCallbacks(NULL, NULL, NULL);
    Clear();
}

void NcpFrameBuffer::Clear(void)
{
    Message *message;
    bool wasEmpty = IsEmpty();

    // Write (InFrame) related variables
    mWriteFrameStart = mBuffer;
    mWriteSegmentHead = mBuffer;
    mWriteSegmentTail = mBuffer;

    // Read (OutFrame) related variables
    mReadState = kReadStateDone;
    mReadFrameLength = kUnknownFrameLength;

    mReadFrameStart = mBuffer;
    mReadSegmentHead = mBuffer;
    mReadSegmentTail = mBuffer;
    mReadPointer = mBuffer;

    mReadMessage = NULL;
    mReadMessageOffset = 0;
    mReadMessageTail = mMessageBuffer;

    // Free all messages in the queues.

    while ((message = mWriteFrameMessageQueue.GetHead()) != NULL)
    {
        mWriteFrameMessageQueue.Dequeue(*message);
        message->Free();
    }

    while ((message = mMessageQueue.GetHead()) != NULL)
    {
        mMessageQueue.Dequeue(*message);
        message->Free();
    }

    if (!wasEmpty)
    {
        if (mEmptyBufferCallback != NULL)
        {
            mEmptyBufferCallback(mCallbackContext, this);
        }
    }
}

void NcpFrameBuffer::SetCallbacks(BufferCallback aEmptyBufferCallback, BufferCallback aNonEmptyBufferCallback,
                                  void *aContext)
{
    mEmptyBufferCallback = aEmptyBufferCallback;
    mNonEmtyBufferCallback = aNonEmptyBufferCallback;
    mCallbackContext = aContext;
}

// Returns the next buffer pointer addressing the wrap-around at the end of buffer.
uint8_t *NcpFrameBuffer::Next(uint8_t *aBufPtr) const
{
    aBufPtr++;
    return (aBufPtr == mBufferEnd) ? mBuffer : aBufPtr;
}

// Returns an advanced (moved forward) version of the given buffer pointer by the given offset.
uint8_t *NcpFrameBuffer::Advance(uint8_t *aBufPtr, uint8_t aOffset) const
{
    aBufPtr += aOffset;

    while (aBufPtr >= mBufferEnd)
    {
        aBufPtr -= mBufferLength;
    }

    return aBufPtr;
}

// Get the distance between two buffer pointers (adjusts for the wrap-around).
uint16_t NcpFrameBuffer::GetDistance(uint8_t *aStartPtr, uint8_t *aEndPtr) const
{
    size_t distance;

    if (aEndPtr >= aStartPtr)
    {
        distance = static_cast<size_t>(aEndPtr - aStartPtr);
    }
    else
    {
        distance  = static_cast<size_t>(mBufferEnd - aStartPtr);
        distance += static_cast<size_t>(aEndPtr - mBuffer);
    }

    return static_cast<uint16_t>(distance);
}

// Write a uint16 value at the given buffer pointer (big-endian style).
void NcpFrameBuffer::WriteUint16At(uint8_t *aBufPtr, uint16_t aValue)
{
    *aBufPtr = (aValue >> 8);
    *Next(aBufPtr) = (aValue & 0xff);
}

// Read a uint16 value at the given buffer pointer (big-endian style).
uint16_t NcpFrameBuffer::ReadUint16At(uint8_t *aBufPtr)
{
    uint16_t value;

    value = static_cast<uint16_t>((*aBufPtr) << 8);
    value += *Next(aBufPtr);

    return value;
}

// Writes a bytes at the write tail, discards the frame if buffer gets full.
ThreadError NcpFrameBuffer::InFrameFeedByte(uint8_t aByte)
{
    ThreadError error = kThreadError_None;
    uint8_t *newTail = Next(mWriteSegmentTail);

    VerifyOrExit(newTail != mReadFrameStart, error = kThreadError_NoBufs);

    *mWriteSegmentTail = aByte;
    mWriteSegmentTail = newTail;

exit:
    if (error != kThreadError_None)
    {
        InFrameDiscard();
    }

    return error;
}

// This method begins a new segment (if one is not already open)
ThreadError NcpFrameBuffer::InFrameBeginSegment(void)
{
    ThreadError error = kThreadError_None;
    uint16_t headerFlags = kSegmentHeaderNoFlag;

    // Verify that segment is not yet started (i.e., head and tail are the same).
    VerifyOrExit(mWriteSegmentHead == mWriteSegmentTail, ;);

    // If this is the start of a new frame (i.e., frame start is same as segment head)
    if (mWriteFrameStart == mWriteSegmentHead)
    {
        headerFlags |= kSegmentHeaderNewFrameFlag;
    }

    // Reserve space for the segment header.
    for (uint16_t i = kSegmentHeaderSize; i; i--)
    {
        SuccessOrExit(error = InFrameFeedByte(0));
    }

    // Write the flags at the segment head
    WriteUint16At(mWriteSegmentHead, headerFlags);

exit:
    return error;
}

// This function closes/ends the current segment.
void NcpFrameBuffer::InFrameEndSegment(uint16_t aHeaderFlags)
{
    uint16_t segmentLength;
    uint16_t header;

    segmentLength = GetDistance(mWriteSegmentHead, mWriteSegmentTail);

    if (segmentLength >= kSegmentHeaderSize)
    {
        // Reduce the header size.
        segmentLength -= kSegmentHeaderSize;

        // Update the length and the flags in segment header (at segment head pointer).
        header = ReadUint16At(mWriteSegmentHead);
        header |= (segmentLength & kSegmentHeaderLengthMask);
        header |= aHeaderFlags;
        WriteUint16At(mWriteSegmentHead, header);

        // Move the segment head to current tail (to be ready for a possible next segment).
        mWriteSegmentHead = mWriteSegmentTail;
    }
    else
    {
        // Remove the current segment (move the tail back to head).
        mWriteSegmentTail = mWriteSegmentHead;
    }
}

// This method discards the current frame.
void NcpFrameBuffer::InFrameDiscard(void)
{
    Message *message;

    // Move the write segment head and tail pointers back to frame start.
    mWriteSegmentHead = mWriteSegmentTail = mWriteFrameStart;

    // Free any messages associated with current frame.
    while ((message = mWriteFrameMessageQueue.GetHead()) != NULL)
    {
        mWriteFrameMessageQueue.Dequeue(*message);
        message->Free();
    }
}

ThreadError NcpFrameBuffer::InFrameBegin(void)
{
    // Discard any previous frame.
    InFrameDiscard();

    return kThreadError_None;
}

ThreadError NcpFrameBuffer::InFrameFeedData(const uint8_t *aDataBuffer, uint16_t aDataBufferLength)
{
    ThreadError error = kThreadError_None;

    // Begin a new segment (if we are not in middle of segment already).
    SuccessOrExit(error = InFrameBeginSegment());

    // Write the data buffer
    while (aDataBufferLength--)
    {
        SuccessOrExit(error = InFrameFeedByte(*aDataBuffer++));
    }

exit:
    return error;
}

ThreadError NcpFrameBuffer::InFrameFeedMessage(Message &aMessage)
{
    ThreadError error = kThreadError_None;

    // Begin a new segment (if we are not in middle of segment already).
    SuccessOrExit(error = InFrameBeginSegment());

    // Enqueue the message in the current write frame queue.
    SuccessOrExit(error = mWriteFrameMessageQueue.Enqueue(aMessage));

    // End/Close the current segment marking the flag that it contains an associated message.
    InFrameEndSegment(kSegmentHeaderMessageIndicatorFlag);

exit:
    return error;
}

ThreadError NcpFrameBuffer::InFrameEnd(void)
{
    Message *message;
    bool wasEmpty = IsEmpty();

    // End/Close the current segment (if any).
    InFrameEndSegment(kSegmentHeaderNoFlag);

    // Update the frame start pointer to current segment head to be ready for next frame.
    mWriteFrameStart = mWriteSegmentHead;

    // Move all the messages from the frame queue to the main queue.
    while ((message = mWriteFrameMessageQueue.GetHead()) != NULL)
    {
        mWriteFrameMessageQueue.Dequeue(*message);
        mMessageQueue.Enqueue(*message);
    }

    // If buffer was empty before, invoke the callback to signal that buffer is now non-empty.
    if (wasEmpty)
    {
        if (mNonEmtyBufferCallback != NULL)
        {
            mNonEmtyBufferCallback(mCallbackContext, this);
        }
    }

    return kThreadError_None;
}

bool NcpFrameBuffer::IsEmpty(void) const
{
    return (mReadFrameStart == mWriteFrameStart);
}

// Start/Prepare a new segment for reading.
ThreadError NcpFrameBuffer::OutFramePrepareSegment(void)
{
    ThreadError error = kThreadError_None;
    uint16_t header;

    while (true)
    {
        // Go to the next segment (set the segment head to current segment's end/tail).
        mReadSegmentHead = mReadSegmentTail;

        // Ensure there is something to read (i.e. segment head is not at start of frame being written).
        VerifyOrExit(mReadSegmentHead != mWriteFrameStart, error = kThreadError_NotFound);

        // Read the segment header.
        header = ReadUint16At(mReadSegmentHead);

        // Check if this segment is the start of a frame.
        if (header & kSegmentHeaderNewFrameFlag)
        {
            // Ensure that this segment is start of current frame, otherwise the current frame is finished.
            VerifyOrExit(mReadSegmentHead == mReadFrameStart, error = kThreadError_NotFound);
        }

        // Find tail/end of current segment.
        mReadSegmentTail = Advance(mReadSegmentHead,
				   kSegmentHeaderSize + static_cast<uint8_t>(header & kSegmentHeaderLengthMask));

        // Update the current read pointer to skip the segment header.
        mReadPointer = Advance(mReadSegmentHead, kSegmentHeaderSize);

        // Check if there are data bytes to be read in this segment (i.e. read pointer not at the tail).
        if (mReadPointer != mReadSegmentTail)
        {
            // Update the state to `InSegment` and return.
            mReadState = kReadStateInSegment;

            ExitNow();
        }

        // No data in this segment,  prepare any appended/associated message of this segment.
        if (OutFramePrepareMessage() == kThreadError_None)
        {
            ExitNow();
        }

        // If there is no message (`PrepareMessage()` returned an error), loop back to prepare the next segment.
    }

exit:
    if (error != kThreadError_None)
    {
        mReadState = kReadStateDone;
    }

    return error;
}

// This method prepares an associated message in current segment and fills the message buffer. It returns
// ThreadError_NotFound if there is no message or if the message has no content.
ThreadError NcpFrameBuffer::OutFramePrepareMessage(void)
{
    ThreadError error = kThreadError_None;
    uint16_t header;

    // Read the segment header
    header = ReadUint16At(mReadSegmentHead);

    // Ensure that the segment header indicates that there is an associated message or return `NotFound` error.
    VerifyOrExit((header & kSegmentHeaderMessageIndicatorFlag) != 0, error = kThreadError_NotFound);

    // Update the current message from the queue.
    mReadMessage = (mReadMessage == NULL) ? mMessageQueue.GetHead() : mReadMessage->GetNext();

    VerifyOrExit(mReadMessage != NULL, error = kThreadError_NotFound);

    // Reset the offset for reading the message.
    mReadMessageOffset = 0;

    // Fill the content from current message into the message buffer.
    SuccessOrExit(error = OutFrameFillMessageBuffer());

    // If all successful, set the state to `InMessage`.
    mReadState = kReadStateInMessage;

exit:
    return error;
}

// This method fills content from current message into the message buffer. It returns kThreadError_NotFound if no more
// content in the current message.
ThreadError NcpFrameBuffer::OutFrameFillMessageBuffer(void)
{
    ThreadError error = kThreadError_None;
    int readLength;

    VerifyOrExit(mReadMessage != NULL, error = kThreadError_NotFound);

    VerifyOrExit(mReadMessageOffset < mReadMessage->GetLength(), error = kThreadError_NotFound);

    // Read portion of current message from the offset into message buffer.
    readLength = mReadMessage->Read(mReadMessageOffset, sizeof(mMessageBuffer), mMessageBuffer);

    VerifyOrExit(readLength > 0, error = kThreadError_NotFound);

    // Update the message offset, set up the message tail, and set read pointer to start of message buffer.

    mReadMessageOffset += readLength;

    mReadMessageTail = mMessageBuffer + readLength;

    mReadPointer = mMessageBuffer;

exit:
    return error;
}

ThreadError NcpFrameBuffer::OutFrameBegin(void)
{
    ThreadError error = kThreadError_None;

    mReadMessage = NULL;

    // Move the segment head and tail to start of frame.
    mReadSegmentHead = mReadSegmentTail = mReadFrameStart;

    // Prepare the current segment for reading.
    error = OutFramePrepareSegment();

    return error;
}

bool NcpFrameBuffer::OutFrameHasEnded(void)
{
    return (mReadState == kReadStateDone);
}

uint8_t NcpFrameBuffer::OutFrameReadByte(void)
{
    ThreadError error;
    uint8_t retval = kReadByteAfterFrameHasEnded;

    switch (mReadState)
    {
    case kReadStateDone:

        retval = kReadByteAfterFrameHasEnded;

        break;

    case kReadStateInSegment:

        // Read a byte from current read pointer and move the read pointer forward.
        retval = *mReadPointer;
        mReadPointer = Next(mReadPointer);

        // Check if at end of current segment.
        if (mReadPointer == mReadSegmentTail)
        {
            // Prepare any message associated with this segment.
            error = OutFramePrepareMessage();

            // If there is no message, move to next segment (if any).
            if (error != kThreadError_None)
            {
                OutFramePrepareSegment();
            }
        }

        break;

    case kReadStateInMessage:

        // Read a byte from current read pointer and move the read pointer forward.
        retval = *mReadPointer;
        mReadPointer++;

        // Check if at the end of content in message buffer.
        if (mReadPointer == mReadMessageTail)
        {
            // Fill more bytes from current message into message buffer.
            error = OutFrameFillMessageBuffer();

            // If no more bytes in the message, move to next segment (if any).
            if (error != kThreadError_None)
            {
                OutFramePrepareSegment();
            }
        }

        break;
    }

    return retval;
}

uint16_t NcpFrameBuffer::OutFrameRead(uint16_t aReadLength, uint8_t *aDataBuffer)
{
    uint16_t bytesRead = 0;

    for (bytesRead = 0; (bytesRead < aReadLength) && !OutFrameHasEnded(); bytesRead++)
    {
        *aDataBuffer++ = OutFrameReadByte();
    }

    return bytesRead;
}

ThreadError NcpFrameBuffer::OutFrameRemove(void)
{
    ThreadError error = kThreadError_None;
    uint8_t *bufPtr;
    Message *message;
    uint16_t header;

    VerifyOrExit(!IsEmpty(), error = kThreadError_NotFound);

    // Begin at the start of current frame and move through all segments.

    bufPtr = mReadFrameStart;

    while (bufPtr != mWriteFrameStart)
    {
        // Read the segment header
        header = ReadUint16At(bufPtr);

        // If the current segment defines a new frame, and it is not the start of current frame, then we have reached
        // end of current frame.
        if (header & kSegmentHeaderNewFrameFlag)
        {
            if (bufPtr != mReadFrameStart)
            {
                break;
            }
        }

        // If current segment has an appended message, remove it from message queue and free it.
        if (header & kSegmentHeaderMessageIndicatorFlag)
        {
            if ((message = mMessageQueue.GetHead()) != NULL)
            {
                mMessageQueue.Dequeue(*message);
                message->Free();
            }
        }

        // Move the pointer to next segment.
        bufPtr = Advance(bufPtr, kSegmentHeaderSize + static_cast<uint8_t>(header & kSegmentHeaderLengthMask));
    }

    mReadFrameStart = bufPtr;

    mReadState = kReadStateDone;
    mReadFrameLength = kUnknownFrameLength;

    // If the remove causes the buffer to become empty, invoke the callback to signal this.
    if (IsEmpty())
    {
        if (mEmptyBufferCallback != NULL)
        {
            mEmptyBufferCallback(mCallbackContext, this);
        }
    }

exit:
    return error;
}


uint16_t NcpFrameBuffer::OutFrameGetLength(void)
{
    uint16_t frameLength = 0;
    uint16_t header;
    uint8_t *bufPtr;
    Message *message = NULL;

    // If the frame length was calculated before, return the previously calculated length.
    VerifyOrExit(mReadFrameLength == kUnknownFrameLength, frameLength = mReadFrameLength);

    VerifyOrExit(!IsEmpty(), frameLength = 0);

    // Calculate frame length by adding length of all segments and messages within the current frame.

    bufPtr = mReadFrameStart;

    while (bufPtr != mWriteFrameStart)
    {
        // Read the segment header
        header = ReadUint16At(bufPtr);

        // If the current segment defines a new frame, and it is not the start of current frame, then we have reached
        // end of current frame.
        if (header & kSegmentHeaderNewFrameFlag)
        {
            if (bufPtr != mReadFrameStart)
            {
                break;
            }
        }

        // If current segment has an associated message, add its length to frame length.
        if (header & kSegmentHeaderMessageIndicatorFlag)
        {
            message = (message == NULL) ? mMessageQueue.GetHead() : message->GetNext();

            if (message != NULL)
            {
                frameLength += message->GetLength();
            }
        }

        // Add the length of current segment to the frame length.
        frameLength += (header & kSegmentHeaderLengthMask);

        // Move the pointer to next segment.
        bufPtr = Advance(bufPtr, kSegmentHeaderSize + static_cast<uint8_t>(header & kSegmentHeaderLengthMask));
    }

    // Remember the calculated frame length for current frame.
    mReadFrameLength = frameLength;

exit:
    return frameLength;
}

}  // namespace Thread
