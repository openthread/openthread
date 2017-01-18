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
#include "test_util.h"
#include "openthread/openthread.h"
#include <common/code_utils.hpp>
#include <common/message.hpp>
#include <ncp/ncp_buffer.hpp>

namespace Thread {

// This module implements unit-test for NcpFrameBuffer class.

enum
{
    kTestBufferSize = 101,         // Size of backed buffer for NcpFrameBuffer.
    kTestIterationAttemps = 120,
};

//  Messages used for building frames...
static const uint8_t sOpenThreadText[] = "OpenThread Rocks";
static const uint8_t sHelloText[]      = "Hello there!";
static const uint8_t sMottoText[]      = "Think good thoughts, say good words, do good deeds!";
static const uint8_t sMysteryText[]    = "4871(\\):|(3$}{4|/4/2%14(\\)";

static MessagePool sMessagePool;

struct CallbackContext
{
    uint16_t mEmptyCount;           // Number of times BufferEmptyCallback is invoked.
    uint16_t mNonEmptyCount;        // Number of times BufferNonEmptyCallback is invoked.
};

void BufferDidGetEmptyCallback(void *aContext, NcpFrameBuffer *aNcpBuffer)
{
    CallbackContext *callbackContext = reinterpret_cast<CallbackContext *>(aContext);

    VerifyOrQuit(aNcpBuffer != NULL, "Null NcpFrameBuffer in the callback");
    VerifyOrQuit(callbackContext != NULL, "Null context in the callback");

    callbackContext->mEmptyCount++;
}

void BufferDidGetNonEmptyCallback(void *aContext, NcpFrameBuffer *aNcpBuffer)
{
    CallbackContext *callbackContext = reinterpret_cast<CallbackContext *>(aContext);

    VerifyOrQuit(aNcpBuffer != NULL, "Null NcpFrameBuffer in the callback");
    VerifyOrQuit(callbackContext != NULL, "Null context in the callback");

    callbackContext->mNonEmptyCount++;
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

    message = sMessagePool.New(Message::kTypeIp6, 0);
    VerifyOrQuit(message != NULL, "Null Message");
    SuccessOrQuit(message->SetLength(sizeof(sMottoText)), "Could not set the length of message.");
    message->Write(0, sizeof(sMottoText), sMottoText);

    SuccessOrQuit(aNcpBuffer.InFrameBegin(), "InFrameBegin() failed.");
    SuccessOrQuit(aNcpBuffer.InFrameFeedData(sMottoText, sizeof(sMottoText)), "InFrameFeedData() failed.");
    SuccessOrQuit(aNcpBuffer.InFrameFeedData(sMysteryText, sizeof(sMysteryText)), "InFrameFeedData() failed.");
    SuccessOrQuit(aNcpBuffer.InFrameFeedMessage(message), "InFrameFeedMessage() failed.");
    SuccessOrQuit(aNcpBuffer.InFrameFeedData(sHelloText, sizeof(sHelloText)), "InFrameFeedData() failed.");
    SuccessOrQuit(aNcpBuffer.InFrameEnd(), "InFrameEnd() failed.");
}

void VerifyAndRemoveFrame1(NcpFrameBuffer &aNcpBuffer)
{
    SuccessOrQuit(aNcpBuffer.OutFrameBegin(), "OutFrameBegin() failed unexpectedly.");

    VerifyOrQuit(aNcpBuffer.OutFrameGetLength() == sizeof(sMottoText) + sizeof(sMysteryText) + sizeof(sMottoText)
                 + sizeof(sHelloText), "GetLength() is incorrect.");

    ReadAndVerifyContent(aNcpBuffer, sMottoText, sizeof(sMottoText));
    ReadAndVerifyContent(aNcpBuffer, sMysteryText, sizeof(sMysteryText));
    ReadAndVerifyContent(aNcpBuffer, sMottoText, sizeof(sMottoText));
    ReadAndVerifyContent(aNcpBuffer, sHelloText, sizeof(sHelloText));

    VerifyOrQuit(aNcpBuffer.OutFrameHasEnded() == true, "Frame longer than expected.");
    VerifyOrQuit(aNcpBuffer.OutFrameReadByte() == 0, "ReadByte() returned non-zero after end of frame.");
    SuccessOrQuit(aNcpBuffer.OutFrameRemove(), "Remove() failed.");
}

void WriteTestFrame2(NcpFrameBuffer &aNcpBuffer)
{
    Message *message1;
    Message *message2;

    message1 = sMessagePool.New(Message::kTypeIp6, 0);
    VerifyOrQuit(message1 != NULL, "Null Message");
    SuccessOrQuit(message1->SetLength(sizeof(sMysteryText)), "Could not set the length of message.");
    message1->Write(0, sizeof(sMysteryText), sMysteryText);

    message2 = sMessagePool.New(Message::kTypeIp6, 0);
    VerifyOrQuit(message2 != NULL, "Null Message");
    SuccessOrQuit(message2->SetLength(sizeof(sHelloText)), "Could not set the length of message.");
    message2->Write(0, sizeof(sHelloText), sHelloText);

    SuccessOrQuit(aNcpBuffer.InFrameBegin(), "InFrameFeedBegin() failed.");
    SuccessOrQuit(aNcpBuffer.InFrameFeedMessage(message1), "InFrameFeedMessage() failed.");
    SuccessOrQuit(aNcpBuffer.InFrameFeedData(sOpenThreadText, sizeof(sOpenThreadText)), "InFrameFeedData() failed.");
    SuccessOrQuit(aNcpBuffer.InFrameFeedMessage(message2), "InFrameFeedMessage() failed.");
    SuccessOrQuit(aNcpBuffer.InFrameEnd(), "InFrameEnd() failed.");
}

void VerifyAndRemoveFrame2(NcpFrameBuffer &aNcpBuffer)
{
    SuccessOrQuit(aNcpBuffer.OutFrameBegin(), "OutFrameBegin() failed unexpectedly.");

    VerifyOrQuit(aNcpBuffer.OutFrameGetLength() == sizeof(sMysteryText) + sizeof(sHelloText) + sizeof(sOpenThreadText),
                 "GetLength() is incorrect.");

    ReadAndVerifyContent(aNcpBuffer, sMysteryText, sizeof(sMysteryText));
    ReadAndVerifyContent(aNcpBuffer, sOpenThreadText, sizeof(sOpenThreadText));
    ReadAndVerifyContent(aNcpBuffer, sHelloText, sizeof(sHelloText));

    VerifyOrQuit(aNcpBuffer.OutFrameHasEnded() == true, "Frame longer than expected.");
    VerifyOrQuit(aNcpBuffer.OutFrameReadByte() == 0, "ReadByte() returned non-zero after end of frame.");
    SuccessOrQuit(aNcpBuffer.OutFrameRemove(), "Remove() failed.");
}

void WriteTestFrame3(NcpFrameBuffer &aNcpBuffer)
{
    Message *message1;

    message1 = sMessagePool.New(Message::kTypeIp6, 0);
    VerifyOrQuit(message1 != NULL, "Null Message");

    // An empty message with no content.
    SuccessOrQuit(message1->SetLength(0), "Could not set the length of message.");

    SuccessOrQuit(aNcpBuffer.InFrameBegin(), "InFrameFeedBegin() failed.");
    SuccessOrQuit(aNcpBuffer.InFrameFeedMessage(message1), "InFrameFeedMessage() failed.");
    SuccessOrQuit(aNcpBuffer.InFrameFeedData(sMysteryText, sizeof(sMysteryText)), "InFrameFeedData() failed.");
    SuccessOrQuit(aNcpBuffer.InFrameEnd(), "InFrameEnd() failed.");
}

void VerifyAndRemoveFrame3(NcpFrameBuffer &aNcpBuffer)
{
    SuccessOrQuit(aNcpBuffer.OutFrameBegin(), "OutFrameBegin() failed unexpectedly.");

    VerifyOrQuit(aNcpBuffer.OutFrameGetLength() == sizeof(sMysteryText), "GetLength() is incorrect.");

    ReadAndVerifyContent(aNcpBuffer, sMysteryText, sizeof(sMysteryText));

    VerifyOrQuit(aNcpBuffer.OutFrameHasEnded() == true, "Frame longer than expected.");
    VerifyOrQuit(aNcpBuffer.OutFrameReadByte() == 0, "ReadByte() returned non-zero after end of frame.");
    SuccessOrQuit(aNcpBuffer.OutFrameRemove(), "Remove() failed.");
}

// This function implements the NcpFrameBuffer tests
void TestNcpFrameBuffer(void)
{
    unsigned i, j;
    uint8_t buffer[kTestBufferSize];
    NcpFrameBuffer ncpBuffer(buffer, kTestBufferSize);

    Message *message;
    CallbackContext context;
    CallbackContext oldContext;
    uint8_t readBuffer[16];
    uint16_t readLen, readOffset;

    for (i = 0; i < sizeof(buffer); i++)
    {
        buffer[i] = 0;
    }

    context.mEmptyCount = 0;
    context.mNonEmptyCount = 0;

    // Set the callbacks.
    ncpBuffer.SetCallbacks(BufferDidGetEmptyCallback, BufferDidGetNonEmptyCallback, &context);

    printf("\n- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    printf("\nTest 1: Write a frame 1 ");

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
    printf("\nTest 2: Multiple frames write and read ");

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
    printf("\nTest 3: Frame discard when buffer full and partial read restart");

    for (j = 0; j < kTestIterationAttemps; j++)
    {
        WriteTestFrame2(ncpBuffer);
        WriteTestFrame3(ncpBuffer);

        ncpBuffer.InFrameBegin();
        ncpBuffer.InFrameFeedData(sHelloText, sizeof(sHelloText));

        message = sMessagePool.New(Message::kTypeIp6, 0);
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
    printf("\nTest 4: Callbacks ");

    printf("\nIterations: ");

    // Repeat this multiple times.
    for (j = 0; j < kTestIterationAttemps; j++)
    {
        printf("*");
        oldContext = context;
        WriteTestFrame2(ncpBuffer);
        VerifyOrQuit(ncpBuffer.IsEmpty() == false, "IsEmpty() is incorrect when buffer is non-empty");
        VerifyOrQuit(oldContext.mEmptyCount == context.mEmptyCount, "Empty callback called incorrectly");
        VerifyOrQuit(oldContext.mNonEmptyCount + 1 == context.mNonEmptyCount, "NonEmpty callback was not invoked.");

        oldContext = context;
        WriteTestFrame3(ncpBuffer);
        VerifyOrQuit(oldContext.mEmptyCount == context.mEmptyCount, "Empty callback called incorrectly");
        VerifyOrQuit(oldContext.mNonEmptyCount == context.mNonEmptyCount, "NonEmpty callback called incorrectly.");

        oldContext = context;
        ncpBuffer.OutFrameRemove();
        VerifyOrQuit(ncpBuffer.IsEmpty() == false, "IsEmpty() is incorrect when buffer is non empty.");
        VerifyOrQuit(oldContext.mEmptyCount == context.mEmptyCount, "Empty callback called incorrectly");
        VerifyOrQuit(oldContext.mNonEmptyCount == context.mNonEmptyCount, "NonEmpty callback called incorrectly.");

        oldContext = context;
        ncpBuffer.OutFrameRemove();
        VerifyOrQuit(ncpBuffer.IsEmpty() == true, "IsEmpty() is incorrect when buffer is empty.");
        VerifyOrQuit(oldContext.mEmptyCount + 1 == context.mEmptyCount, "Empty callback was not invoked.");
        VerifyOrQuit(oldContext.mNonEmptyCount == context.mNonEmptyCount, "NonEmpty callback called incorrectly.");
    }

    printf(" -- PASS\n");

    printf("\n- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    printf("\nTest 5: Clear() and empty buffer method tests");

    WriteTestFrame1(ncpBuffer);

    oldContext = context;
    ncpBuffer.Clear();

    VerifyOrQuit(ncpBuffer.IsEmpty() == true, "IsEmpty() is incorrect when buffer is empty.");
    VerifyOrQuit(ncpBuffer.OutFrameHasEnded() == true, "OutFrameHasEnded() is incorrect when no data in buffer.");
    VerifyOrQuit(ncpBuffer.OutFrameRemove() == kThreadError_NotFound,
                 "Remove() returned incorrect error status when buffer is empty.");
    VerifyOrQuit(ncpBuffer.OutFrameGetLength() == 0, "OutFrameGetLength() returned non-zero length when buffer is empty.");
    VerifyOrQuit(oldContext.mEmptyCount + 1 == context.mEmptyCount, "Empty callback was not invoked.");
    VerifyOrQuit(oldContext.mNonEmptyCount == context.mNonEmptyCount, "NonEmpty callback called incorrectly.");

    WriteTestFrame1(ncpBuffer);

    oldContext = context;
    VerifyAndRemoveFrame1(ncpBuffer);

    VerifyOrQuit(ncpBuffer.IsEmpty() == true, "IsEmpty() is incorrect when buffer is empty.");
    VerifyOrQuit(ncpBuffer.OutFrameHasEnded() == true, "OutFrameHasEnded() is incorrect when no data in buffer.");
    VerifyOrQuit(ncpBuffer.OutFrameRemove() == kThreadError_NotFound,
                 "Remove() returned incorrect error status when buffer is empty.");
    VerifyOrQuit(ncpBuffer.OutFrameGetLength() == 0, "OutFrameGetLength() returned non-zero length when buffer is empty.");
    VerifyOrQuit(oldContext.mEmptyCount + 1 == context.mEmptyCount, "Empty callback was not invoked.");
    VerifyOrQuit(oldContext.mNonEmptyCount == context.mNonEmptyCount, "NonEmpty callback called incorrectly.");

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
}

}  // namespace Thread

#ifdef ENABLE_TEST_MAIN
int main(void)
{
    Thread::TestNcpFrameBuffer();
    printf("\nAll tests passed.\n");
    return 0;
}
#endif
