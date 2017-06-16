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

#include <ctype.h>

#include <openthread/openthread.h>

#include "openthread-instance.h"
#include "common/code_utils.hpp"
#include "common/message.hpp"
#include "ncp/ncp_buffer.hpp"

#include "test_platform.h"
#include "test_util.h"

namespace ot {

// This module implements unit-test for NcpFrameBuffer class.

enum
{
    kTestBufferSize = 2500,
    kTestIterationAttemps = 10000,
    kTagArraySize = 1000,
};

//  Messages used for building frames...
static const uint8_t sOpenThreadText[] = "OpenThread Rocks";
static const uint8_t sHelloText[]      = "Hello there!";
static const uint8_t sMottoText[]      = "Think good thoughts, say good words, do good deeds!";
static const uint8_t sMysteryText[]    = "4871(\\):|(3$}{4|/4/2%14(\\)";

static otInstance *sInstance;
static MessagePool *sMessagePool;

struct CallbackContext
{
    uint32_t mFrameAddedCount;           // Number of times FrameAddedCallback is invoked.
    uint32_t mFrameRemovedCount;         // Number of times FrameRemovedCallback is invoked.
};

CallbackContext sContext;

NcpFrameBuffer::FrameTag sTagHistoryArray[kTagArraySize];
uint32_t sTagHistoryHead = 0;
uint32_t sTagHistoryTail = 0;
NcpFrameBuffer::FrameTag sExpectedRemovedTag = NcpFrameBuffer::kInvalidTag;

void ClearTagHistory(void)
{
    sTagHistoryHead = sTagHistoryTail;
}

void AddTagToHistory(NcpFrameBuffer::FrameTag aTag)
{
    sTagHistoryArray[sTagHistoryTail] = aTag;

    if (++sTagHistoryTail == kTagArraySize)
    {
        sTagHistoryTail = 0;
    }

    VerifyOrQuit(sTagHistoryTail != sTagHistoryHead, "Ran out of space in `TagHistoryArray`, increase its size.");
}

void VerifyAndRemoveTagFromHistory(NcpFrameBuffer::FrameTag aTag)
{
    VerifyOrQuit(sTagHistoryHead != sTagHistoryTail, "Tag history is empty,");
    VerifyOrQuit(aTag == sTagHistoryArray[sTagHistoryHead], "Removed tag does not match the added one");

    if (++sTagHistoryHead == kTagArraySize)
    {
        sTagHistoryHead = 0;
    }

    if (sExpectedRemovedTag != NcpFrameBuffer::kInvalidTag)
    {
        VerifyOrQuit(sExpectedRemovedTag == aTag, "Removed tag does match the previous OutFrameGetTag()");
        sExpectedRemovedTag = NcpFrameBuffer::kInvalidTag;
    }
}

void FrameAddedCallback(void *aContext, NcpFrameBuffer::FrameTag aTag, NcpFrameBuffer *aNcpBuffer)
{
    CallbackContext *callbackContext = reinterpret_cast<CallbackContext *>(aContext);

    VerifyOrQuit(aNcpBuffer != NULL, "Null NcpFrameBuffer in the callback");
    VerifyOrQuit(callbackContext != NULL, "Null context in the callback");
    VerifyOrQuit(aTag != NcpFrameBuffer::kInvalidTag, "Invalid tag in the callback");
    VerifyOrQuit(aTag == aNcpBuffer->InFrameGetLastTag(), "InFrameGetLastTag() does not match the tag from callback");
    AddTagToHistory(aTag);

    callbackContext->mFrameAddedCount++;
}

void FrameRemovedCallback(void *aContext, NcpFrameBuffer::FrameTag aTag, NcpFrameBuffer *aNcpBuffer)
{
    CallbackContext *callbackContext = reinterpret_cast<CallbackContext *>(aContext);

    VerifyOrQuit(aNcpBuffer != NULL, "Null NcpFrameBuffer in the callback");
    VerifyOrQuit(callbackContext != NULL, "Null context in the callback");
    VerifyOrQuit(aTag != NcpFrameBuffer::kInvalidTag, "Invalid tag in the callback");
    VerifyAndRemoveTagFromHistory(aTag);

    callbackContext->mFrameRemovedCount++;
}

// Dump the buffer content to screen.
void DumpBuffer(const char *aTextMessage, uint8_t *aBuffer, uint16_t aBufferLength)
{
    enum
    {
        kBytesPerLine = 32,    // Number of bytes per line.
    };

    char charBuff[kBytesPerLine + 1];
    uint16_t counter;
    uint8_t byte;

    printf("\n%s - len = %u\n    ", aTextMessage, aBufferLength);

    counter = 0;

    while (aBufferLength--)
    {
        byte = *aBuffer++;
        printf("%02X ", byte);
        charBuff[counter] = isprint(byte) ? static_cast<char>(byte) : '.';
        counter++;

        if (counter == kBytesPerLine)
        {
            charBuff[counter] = 0;
            printf("    %s\n    ", charBuff);
            counter = 0;
        }
    }

    charBuff[counter] = 0;

    while (counter++ < kBytesPerLine)
    {
        printf("   ");
    }

    printf("    %s\n", charBuff);
}

// Reads bytes from the ncp buffer, and verifies that it matches with the given content buffer.
void ReadAndVerifyContent(NcpFrameBuffer &aNcpBuffer, const uint8_t *aContentBuffer, uint16_t aBufferLength)
{
    while (aBufferLength--)
    {
        VerifyOrQuit(aNcpBuffer.OutFrameHasEnded() == false, "Out frame ended before end of expected content.");

        VerifyOrQuit(aNcpBuffer.OutFrameReadByte() == *aContentBuffer++,
                     "Out frame read byte does not match expected content");
    }
}

void WriteTestFrame1(NcpFrameBuffer &aNcpBuffer)
{
    Message *message;
    CallbackContext oldContext;

    message = sMessagePool->New(Message::kTypeIp6, 0);
    VerifyOrQuit(message != NULL, "Null Message");
    SuccessOrQuit(message->SetLength(sizeof(sMottoText)), "Could not set the length of message.");
    message->Write(0, sizeof(sMottoText), sMottoText);

    oldContext = sContext;
    SuccessOrQuit(aNcpBuffer.InFrameBegin(), "InFrameBegin() failed.");
    SuccessOrQuit(aNcpBuffer.InFrameFeedData(sMottoText, sizeof(sMottoText)), "InFrameFeedData() failed.");
    SuccessOrQuit(aNcpBuffer.InFrameFeedData(sMysteryText, sizeof(sMysteryText)), "InFrameFeedData() failed.");
    SuccessOrQuit(aNcpBuffer.InFrameFeedMessage(message), "InFrameFeedMessage() failed.");
    SuccessOrQuit(aNcpBuffer.InFrameFeedData(sHelloText, sizeof(sHelloText)), "InFrameFeedData() failed.");
    SuccessOrQuit(aNcpBuffer.InFrameEnd(), "InFrameEnd() failed.");

    VerifyOrQuit(oldContext.mFrameAddedCount + 1 == sContext.mFrameAddedCount, "FrameAddedCallback failed.");
    VerifyOrQuit(oldContext.mFrameRemovedCount == sContext.mFrameRemovedCount, "FrameRemovedCallback failed.");
}

void VerifyAndRemoveFrame1(NcpFrameBuffer &aNcpBuffer)
{
    CallbackContext oldContext = sContext;

    sExpectedRemovedTag = aNcpBuffer.OutFrameGetTag();
    SuccessOrQuit(aNcpBuffer.OutFrameBegin(), "OutFrameBegin() failed unexpectedly.");
    VerifyOrQuit(sExpectedRemovedTag == aNcpBuffer.OutFrameGetTag(), "OutFrameGetTag() value changed unexpectedly.");

    VerifyOrQuit(aNcpBuffer.OutFrameGetLength() == sizeof(sMottoText) + sizeof(sMysteryText) + sizeof(sMottoText)
                 + sizeof(sHelloText), "GetLength() is incorrect.");

    ReadAndVerifyContent(aNcpBuffer, sMottoText, sizeof(sMottoText));
    ReadAndVerifyContent(aNcpBuffer, sMysteryText, sizeof(sMysteryText));
    ReadAndVerifyContent(aNcpBuffer, sMottoText, sizeof(sMottoText));
    ReadAndVerifyContent(aNcpBuffer, sHelloText, sizeof(sHelloText));

    VerifyOrQuit(aNcpBuffer.OutFrameHasEnded() == true, "Frame longer than expected.");
    VerifyOrQuit(aNcpBuffer.OutFrameReadByte() == 0, "ReadByte() returned non-zero after end of frame.");
    VerifyOrQuit(sExpectedRemovedTag == aNcpBuffer.OutFrameGetTag(), "OutFrameGetTag() value changed unexpectedly.");
    SuccessOrQuit(aNcpBuffer.OutFrameRemove(), "Remove() failed.");

    VerifyOrQuit(oldContext.mFrameAddedCount == sContext.mFrameAddedCount, "FrameAddedCallback failed.");
    VerifyOrQuit(oldContext.mFrameRemovedCount + 1 == sContext.mFrameRemovedCount, "FrameRemovedCallback failed.");
}

void WriteTestFrame2(NcpFrameBuffer &aNcpBuffer)
{
    Message *message1;
    Message *message2;
    CallbackContext oldContext = sContext;

    message1 = sMessagePool->New(Message::kTypeIp6, 0);
    VerifyOrQuit(message1 != NULL, "Null Message");
    SuccessOrQuit(message1->SetLength(sizeof(sMysteryText)), "Could not set the length of message.");
    message1->Write(0, sizeof(sMysteryText), sMysteryText);

    message2 = sMessagePool->New(Message::kTypeIp6, 0);
    VerifyOrQuit(message2 != NULL, "Null Message");
    SuccessOrQuit(message2->SetLength(sizeof(sHelloText)), "Could not set the length of message.");
    message2->Write(0, sizeof(sHelloText), sHelloText);

    SuccessOrQuit(aNcpBuffer.InFrameBegin(), "InFrameFeedBegin() failed.");
    SuccessOrQuit(aNcpBuffer.InFrameFeedMessage(message1), "InFrameFeedMessage() failed.");
    SuccessOrQuit(aNcpBuffer.InFrameFeedData(sOpenThreadText, sizeof(sOpenThreadText)), "InFrameFeedData() failed.");
    SuccessOrQuit(aNcpBuffer.InFrameFeedMessage(message2), "InFrameFeedMessage() failed.");
    SuccessOrQuit(aNcpBuffer.InFrameEnd(), "InFrameEnd() failed.");

    VerifyOrQuit(oldContext.mFrameAddedCount + 1 == sContext.mFrameAddedCount, "FrameAddedCallback failed.");
    VerifyOrQuit(oldContext.mFrameRemovedCount == sContext.mFrameRemovedCount, "FrameRemovedCallback failed.");
}

void VerifyAndRemoveFrame2(NcpFrameBuffer &aNcpBuffer)
{
    CallbackContext oldContext = sContext;

    SuccessOrQuit(aNcpBuffer.OutFrameBegin(), "OutFrameBegin() failed unexpectedly.");

    VerifyOrQuit(aNcpBuffer.OutFrameGetLength() == sizeof(sMysteryText) + sizeof(sHelloText) + sizeof(sOpenThreadText),
                 "GetLength() is incorrect.");

    ReadAndVerifyContent(aNcpBuffer, sMysteryText, sizeof(sMysteryText));
    ReadAndVerifyContent(aNcpBuffer, sOpenThreadText, sizeof(sOpenThreadText));
    ReadAndVerifyContent(aNcpBuffer, sHelloText, sizeof(sHelloText));

    VerifyOrQuit(aNcpBuffer.OutFrameHasEnded() == true, "Frame longer than expected.");
    VerifyOrQuit(aNcpBuffer.OutFrameReadByte() == 0, "ReadByte() returned non-zero after end of frame.");
    sExpectedRemovedTag = aNcpBuffer.OutFrameGetTag();
    SuccessOrQuit(aNcpBuffer.OutFrameRemove(), "Remove() failed.");

    VerifyOrQuit(oldContext.mFrameAddedCount == sContext.mFrameAddedCount, "FrameAddedCallback failed.");
    VerifyOrQuit(oldContext.mFrameRemovedCount + 1 == sContext.mFrameRemovedCount, "FrameRemovedCallback failed.");
}

void WriteTestFrame3(NcpFrameBuffer &aNcpBuffer)
{
    Message *message1;
    CallbackContext oldContext = sContext;

    message1 = sMessagePool->New(Message::kTypeIp6, 0);
    VerifyOrQuit(message1 != NULL, "Null Message");

    // An empty message with no content.
    SuccessOrQuit(message1->SetLength(0), "Could not set the length of message.");

    SuccessOrQuit(aNcpBuffer.InFrameBegin(), "InFrameFeedBegin() failed.");
    SuccessOrQuit(aNcpBuffer.InFrameFeedMessage(message1), "InFrameFeedMessage() failed.");
    SuccessOrQuit(aNcpBuffer.InFrameFeedData(sMysteryText, sizeof(sMysteryText)), "InFrameFeedData() failed.");
    SuccessOrQuit(aNcpBuffer.InFrameEnd(), "InFrameEnd() failed.");

    VerifyOrQuit(oldContext.mFrameAddedCount + 1 == sContext.mFrameAddedCount, "FrameAddedCallback failed.");
    VerifyOrQuit(oldContext.mFrameRemovedCount == sContext.mFrameRemovedCount, "FrameRemovedCallback failed.");
}

void VerifyAndRemoveFrame3(NcpFrameBuffer &aNcpBuffer)
{
    CallbackContext oldContext = sContext;

    SuccessOrQuit(aNcpBuffer.OutFrameBegin(), "OutFrameBegin() failed unexpectedly.");

    VerifyOrQuit(aNcpBuffer.OutFrameGetLength() == sizeof(sMysteryText), "GetLength() is incorrect.");

    ReadAndVerifyContent(aNcpBuffer, sMysteryText, sizeof(sMysteryText));

    VerifyOrQuit(aNcpBuffer.OutFrameHasEnded() == true, "Frame longer than expected.");
    VerifyOrQuit(aNcpBuffer.OutFrameReadByte() == 0, "ReadByte() returned non-zero after end of frame.");
    sExpectedRemovedTag = aNcpBuffer.OutFrameGetTag();
    SuccessOrQuit(aNcpBuffer.OutFrameRemove(), "Remove() failed.");

    VerifyOrQuit(oldContext.mFrameAddedCount == sContext.mFrameAddedCount, "FrameAddedCallback failed.");
    VerifyOrQuit(oldContext.mFrameRemovedCount + 1 == sContext.mFrameRemovedCount, "FrameRemovedCallback failed.");
}

// This function implements the NcpFrameBuffer tests
void TestNcpFrameBuffer(void)
{
    unsigned i, j;
    uint8_t buffer[kTestBufferSize];
    NcpFrameBuffer ncpBuffer(buffer, kTestBufferSize);

    Message *message;
    uint8_t readBuffer[16];
    uint16_t readLen, readOffset;

    sInstance = testInitInstance();
    sMessagePool = &sInstance->mIp6.mMessagePool;

    for (i = 0; i < sizeof(buffer); i++)
    {
        buffer[i] = 0;
    }

    sContext.mFrameAddedCount = 0;
    sContext.mFrameRemovedCount = 0;
    ClearTagHistory();

    // Set the callbacks.
    ncpBuffer.SetFrameAddedCallback(FrameAddedCallback, &sContext);
    ncpBuffer.SetFrameRemovedCallback(FrameRemovedCallback, &sContext);

    printf("\n- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    printf("\nTest 1: Check initial buffer state");

    VerifyOrQuit(ncpBuffer.IsEmpty() == true, "Not empty after init.");
    VerifyOrQuit(ncpBuffer.InFrameGetLastTag() == NcpFrameBuffer::kInvalidTag, "Incorrect tag after init.");
    VerifyOrQuit(ncpBuffer.OutFrameGetTag() == NcpFrameBuffer::kInvalidTag, "Incorrect OutFrameTag after init.");

    printf("\n- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    printf("\nTest 2: Write a frame 1 ");

    WriteTestFrame1(ncpBuffer);
    DumpBuffer("\nBuffer after frame1", buffer, kTestBufferSize);
    printf("\nFrameLen is %u", ncpBuffer.OutFrameGetLength());

    VerifyAndRemoveFrame1(ncpBuffer);

    printf("\nIterations: ");

    // Repeat this multiple times.
    for (j = 0; j < kTestIterationAttemps; j++)
    {
        printf("*");
        WriteTestFrame1(ncpBuffer);
        VerifyOrQuit(ncpBuffer.IsEmpty() == false, "IsEmpty() is incorrect when buffer is non-empty");

        VerifyAndRemoveFrame1(ncpBuffer);
        VerifyOrQuit(ncpBuffer.IsEmpty() == true, "IsEmpty() is incorrect when buffer is empty.");
    }

    printf(" -- PASS\n");

    printf("\n- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    printf("\nTest 3: Multiple frames write and read ");

    WriteTestFrame2(ncpBuffer);
    WriteTestFrame3(ncpBuffer);
    WriteTestFrame2(ncpBuffer);
    WriteTestFrame2(ncpBuffer);

    DumpBuffer("\nBuffer after multiple frames", buffer, kTestBufferSize);

    VerifyAndRemoveFrame2(ncpBuffer);
    VerifyAndRemoveFrame3(ncpBuffer);
    VerifyAndRemoveFrame2(ncpBuffer);
    VerifyAndRemoveFrame2(ncpBuffer);

    printf("\nIterations: ");

    // Repeat this multiple times.
    for (j = 0; j < kTestIterationAttemps; j++)
    {
        printf("*");

        WriteTestFrame2(ncpBuffer);
        WriteTestFrame3(ncpBuffer);
        WriteTestFrame2(ncpBuffer);

        VerifyAndRemoveFrame2(ncpBuffer);
        VerifyAndRemoveFrame3(ncpBuffer);

        WriteTestFrame2(ncpBuffer);
        WriteTestFrame3(ncpBuffer);

        VerifyAndRemoveFrame2(ncpBuffer);
        VerifyAndRemoveFrame2(ncpBuffer);
        VerifyAndRemoveFrame3(ncpBuffer);

        VerifyOrQuit(ncpBuffer.IsEmpty() == true, "IsEmpty() is incorrect when buffer is empty.");
    }

    printf(" -- PASS\n");

    printf("\n- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    printf("\nTest 4: Frame discard when buffer full and partial read restart");

    for (j = 0; j < kTestIterationAttemps; j++)
    {
        WriteTestFrame2(ncpBuffer);
        WriteTestFrame3(ncpBuffer);

        ncpBuffer.InFrameBegin();
        ncpBuffer.InFrameFeedData(sHelloText, sizeof(sHelloText));

        message = sMessagePool->New(Message::kTypeIp6, 0);
        VerifyOrQuit(message != NULL, "Null Message");
        SuccessOrQuit(message->SetLength(sizeof(sMysteryText)), "Could not set the length of message.");
        message->Write(0, sizeof(sMysteryText), sMysteryText);

        ncpBuffer.InFrameFeedMessage(message);

        // Now cause a restart the current frame and test if it's discarded ok.
        WriteTestFrame2(ncpBuffer);

        if (j == 0)
        {
            DumpBuffer("\nAfter frame gets discarded", buffer, kTestBufferSize);
            printf("\nIterations: ");
        }
        else
        {
            printf("*");
        }

        VerifyAndRemoveFrame2(ncpBuffer);

        // Start reading few bytes from the frame
        ncpBuffer.OutFrameBegin();
        ncpBuffer.OutFrameReadByte();
        ncpBuffer.OutFrameReadByte();
        ncpBuffer.OutFrameReadByte();

        // Now reset the read pointer and read/verify the frame from start.
        VerifyAndRemoveFrame3(ncpBuffer);
        VerifyAndRemoveFrame2(ncpBuffer);

        VerifyOrQuit(ncpBuffer.IsEmpty() == true, "IsEmpty() is incorrect when buffer is empty.");
    }

    printf(" -- PASS\n");

    printf("\n- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    printf("\nTest 5: Clear() and empty buffer method tests");

    WriteTestFrame1(ncpBuffer);

    ncpBuffer.Clear();
    ClearTagHistory();

    VerifyOrQuit(ncpBuffer.InFrameGetLastTag() == NcpFrameBuffer::kInvalidTag, "Incorrect last tag after Clear().");
    VerifyOrQuit(ncpBuffer.OutFrameGetTag() == NcpFrameBuffer::kInvalidTag, "Incorrect OutFrameTag after Clear().");
    VerifyOrQuit(ncpBuffer.IsEmpty() == true, "IsEmpty() is incorrect when buffer is empty.");
    VerifyOrQuit(ncpBuffer.OutFrameHasEnded() == true, "OutFrameHasEnded() is incorrect when no data in buffer.");
    VerifyOrQuit(ncpBuffer.OutFrameRemove() == OT_ERROR_NOT_FOUND,
                 "Remove() returned incorrect error status when buffer is empty.");
    VerifyOrQuit(ncpBuffer.OutFrameGetLength() == 0, "OutFrameGetLength() returned non-zero length when buffer is empty.");

    WriteTestFrame1(ncpBuffer);
    VerifyAndRemoveFrame1(ncpBuffer);

    VerifyOrQuit(ncpBuffer.IsEmpty() == true, "IsEmpty() is incorrect when buffer is empty.");
    VerifyOrQuit(ncpBuffer.OutFrameHasEnded() == true, "OutFrameHasEnded() is incorrect when no data in buffer.");
    VerifyOrQuit(ncpBuffer.OutFrameRemove() == OT_ERROR_NOT_FOUND,
                 "Remove() returned incorrect error status when buffer is empty.");
    VerifyOrQuit(ncpBuffer.OutFrameGetLength() == 0, "OutFrameGetLength() returned non-zero length when buffer is empty.");

    printf(" -- PASS\n");

    printf("\n- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    printf("\nTest 6: OutFrameRead() in parts\n");

    ncpBuffer.InFrameBegin();
    ncpBuffer.InFrameFeedData(sMottoText, sizeof(sMottoText));
    ncpBuffer.InFrameEnd();

    ncpBuffer.OutFrameBegin();
    readOffset = 0;

    while ((readLen = ncpBuffer.OutFrameRead(sizeof(readBuffer), readBuffer)) != 0)
    {
        DumpBuffer("Read() returned", readBuffer, readLen);

        VerifyOrQuit(memcmp(readBuffer, sMottoText + readOffset, readLen) == 0,
                     "Read() does not match expected content.");

        readOffset += readLen;
    }

    VerifyOrQuit(readOffset == sizeof(sMottoText), "Read len does not match expected length.");
    printf("\n -- PASS\n");

    testFreeInstance(sInstance);
}

/**
 * NCP Buffer Fuzz testing
 *
 * Randomly decide if to read or write a frame to the NCP buffer (use `kReadProbability` in percent to control the
 * behavior).
 *
 * When writing a frame, use a random length (1 up to `kMaxFrameLen`) and generate random byte sequences.
 * When reading a frame ensure the length and the content matches what was written earlier.
 * Handle the cases where buffer gets full or empty.
 *
 */

enum
{
    kFuzTestBufferSize = 2000,             // Size of the buffer used during fuzz testing
    kFuzTestIterationAttempts = 500000,    // Number of iterations  to run
    kLensArraySize = 500,                  // Size of "Lengths" array.
    kMaxFrameLen = 400,                    // Maximum frame length
    kReadProbability = 50,                 // Probability (in percent) to randomly choose to read vs write frame
    kUseTrueRandomNumberGenerator = 1,     // To use true random number generator or not.
};

uint8_t sFrameBuffer[kFuzTestBufferSize];
uint32_t sFrameBufferTailIndex = 0;

uint32_t GetRandom(uint32_t max)
{
    uint32_t value;

    if (kUseTrueRandomNumberGenerator)
    {
        otPlatRandomGetTrue(reinterpret_cast<uint8_t *>(&value), sizeof(value));
    }
    else
    {
        value = otPlatRandomGet();
    }

    return value % max;
}

otError WriteRandomFrame(uint32_t aLength, NcpFrameBuffer &aNcpBuffer)
{
    otError error;
    uint8_t byte;
    CallbackContext oldContext = sContext;
    uint32_t tail = sFrameBufferTailIndex;

    SuccessOrExit(error = aNcpBuffer.InFrameBegin());

    while (aLength--)
    {
        byte = static_cast<uint8_t>(GetRandom(256));
        SuccessOrExit(error = aNcpBuffer.InFrameFeedData(&byte, sizeof(byte)));
        sFrameBuffer[tail++] = byte;
    }

    SuccessOrExit(error = aNcpBuffer.InFrameEnd());

    sFrameBufferTailIndex = tail;

    // check the callbacks
    VerifyOrQuit(oldContext.mFrameAddedCount + 1 == sContext.mFrameAddedCount, "FrameAddedCallback failed.");
    VerifyOrQuit(oldContext.mFrameRemovedCount == sContext.mFrameRemovedCount, "FrameRemovedCallback failed.");

exit:
    return error;
}

otError ReadRandomFrame(uint32_t aLength, NcpFrameBuffer &aNcpBuffer)
{
    CallbackContext oldContext = sContext;

    SuccessOrQuit(aNcpBuffer.OutFrameBegin(), "OutFrameBegin failed");
    VerifyOrQuit(aNcpBuffer.OutFrameGetLength() == aLength, "OutFrameGetLength() does not match");

    // Read and verify that the content is same as sFrameBuffer values...
    ReadAndVerifyContent(aNcpBuffer, sFrameBuffer, static_cast<uint16_t>(aLength));
    sExpectedRemovedTag = aNcpBuffer.OutFrameGetTag();

    SuccessOrQuit(aNcpBuffer.OutFrameRemove(), "OutFrameRemove failed");

    sFrameBufferTailIndex -= aLength;
    memmove(sFrameBuffer, sFrameBuffer + aLength, sFrameBufferTailIndex);

    // If successful check the callbacks
    VerifyOrQuit(oldContext.mFrameAddedCount == sContext.mFrameAddedCount, "FrameAddedCallback failed.");
    VerifyOrQuit(oldContext.mFrameRemovedCount + 1 == sContext.mFrameRemovedCount, "FrameRemovedCallback failed.");

    return OT_ERROR_NONE;
}


// This runs a fuzz test of NCP buffer
void TestFuzzNcpFrameBuffer(void)
{
    uint8_t buffer[kFuzTestBufferSize];
    NcpFrameBuffer ncpBuffer(buffer, kFuzTestBufferSize);

    uint32_t lensArray[kLensArraySize];          // Keeps track of length of written frames so far
    uint32_t lensArrayStart;
    uint32_t lensArrayCount;

    sInstance = testInitInstance();
    sMessagePool = &sInstance->mIp6.mMessagePool;

    memset(buffer, 0, sizeof(buffer));

    memset(lensArray, 0, sizeof(lensArray));
    lensArrayStart = 0;
    lensArrayCount = 0;

    sContext.mFrameAddedCount = 0;
    sContext.mFrameRemovedCount = 0;
    ClearTagHistory();

    ncpBuffer.SetFrameAddedCallback(FrameAddedCallback, &sContext);
    ncpBuffer.SetFrameRemovedCallback(FrameRemovedCallback, &sContext);

    for (uint32_t iter = 0; iter < kFuzTestIterationAttempts; iter++)
    {
        bool shouldRead;

        if (lensArrayCount == 0)
        {
            shouldRead = false;
        }
        else if (lensArrayCount == kLensArraySize - 1)
        {
            shouldRead = true;
        }
        else
        {
            // Randomly decide to read or write.
            shouldRead = (GetRandom(100) < kReadProbability);
        }

        if (shouldRead)
        {
            uint32_t len = lensArray[lensArrayStart];

            lensArrayStart = (lensArrayStart + 1) % kLensArraySize;
            lensArrayCount--;

            printf("R%d ", len);

            SuccessOrQuit(ReadRandomFrame(len, ncpBuffer), "Failed to read random frame.");
        }
        else
        {
            uint32_t len = GetRandom(kMaxFrameLen) + 1;

            if (WriteRandomFrame(len, ncpBuffer) == OT_ERROR_NONE)
            {
                lensArray[(lensArrayStart + lensArrayCount) % kLensArraySize] = len;
                lensArrayCount++;

                printf("W%d ", len);
            }
            else
            {
                printf("FULL ");
            }
        }

        if (lensArrayCount == 0)
        {
            VerifyOrQuit(ncpBuffer.IsEmpty() == true, "IsEmpty failed.");
            printf("EMPTY ");
        }

    }

    printf("\n -- PASS\n");

    testFreeInstance(sInstance);
}

}  // namespace ot

#ifdef ENABLE_TEST_MAIN
int main(void)
{
    ot::TestNcpFrameBuffer();
    ot::TestFuzzNcpFrameBuffer();
    printf("\nAll tests passed.\n");
    return 0;
}
#endif
