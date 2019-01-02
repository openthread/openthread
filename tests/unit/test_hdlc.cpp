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

#include <ctype.h>

#include "common/code_utils.hpp"
#include "common/instance.hpp"
#include "ncp/hdlc.hpp"

#include "test_util.h"

namespace ot {
namespace Ncp {

enum
{
    kBufferSize                   = 1500,  // Frame buffer size
    kMaxFrameLength               = 500,   // Maximum allowed frame length (used when randomly generating frames)
    kFuzzTestIteration            = 50000, // Number of iteration during fuzz test (randomly generating frames)
    kUseTrueRandomNumberGenerator = 1,     // To use true random number generator or not.

    kFlagXOn        = 0x11,
    kFlagXOff       = 0x13,
    kFlagSequence   = 0x7e, ///< HDLC Flag value
    kEscapeSequence = 0x7d, ///< HDLC Escape value
    kFlagSpecial    = 0xf8,

};

static const uint8_t sOpenThreadText[] = "OpenThread Rocks";
static const uint8_t sHelloText[]      = "Hello there!";
static const uint8_t sMottoText[]      = "Think good thoughts, say good words, do good deeds!";
static const uint8_t sHexText[]        = "0123456789abcdef";
static const uint8_t sHdlcSpeicals[]   = {kFlagSequence, kFlagXOn,        kFlagXOff,
                                        kFlagSequence, kEscapeSequence, kFlagSpecial};

otError WriteToBuffer(const uint8_t *aText, Hdlc::FrameWritePointer &aWritePointer)
{
    otError error = OT_ERROR_NONE;

    while (*aText != 0)
    {
        SuccessOrExit(aWritePointer.WriteByte(*aText++));
    }

exit:
    return error;
}

void TestHdlcFrameBuffer(void)
{
    Hdlc::FrameBuffer<kBufferSize> frameBuffer;

    printf("Testing Hdlc::FrameBuffer");

    VerifyOrQuit(frameBuffer.IsEmpty(), "IsEmpty() failed after constructor");
    VerifyOrQuit(frameBuffer.GetLength() == 0, "GetLength() failed after constructor");

    SuccessOrQuit(WriteToBuffer(sOpenThreadText, frameBuffer), "WriteByte() failed");

    VerifyOrQuit(frameBuffer.GetLength() == sizeof(sOpenThreadText) - 1, "GetLength() failed");
    VerifyOrQuit(memcmp(frameBuffer.GetFrame(), sOpenThreadText, frameBuffer.GetLength()) == 0,
                 "GetFrame() content is incorrect");

    VerifyOrQuit(frameBuffer.CanWrite(1), "CanWrite() failed");
    VerifyOrQuit(!frameBuffer.IsEmpty(), "IsEmpty() failed");

    SuccessOrQuit(WriteToBuffer(sHelloText, frameBuffer), "WriteByte() failed");
    VerifyOrQuit(frameBuffer.GetLength() == sizeof(sOpenThreadText) + sizeof(sHelloText) - 2, "GetLength() failed");

    frameBuffer.UndoLastWrites(sizeof(sHelloText) - 1);
    VerifyOrQuit(frameBuffer.GetLength() == sizeof(sOpenThreadText) - 1, "GetLength() failed");
    VerifyOrQuit(memcmp(frameBuffer.GetFrame(), sOpenThreadText, frameBuffer.GetLength()) == 0,
                 "GetFrame() content is incorrect");

    VerifyOrQuit(!frameBuffer.IsEmpty(), "IsEmpty() failed");
    frameBuffer.Clear();
    VerifyOrQuit(frameBuffer.IsEmpty(), "IsEmpty() failed after Clear()");
    VerifyOrQuit(frameBuffer.GetLength() == 0, "GetLength() failed after Clear()");

    SuccessOrQuit(WriteToBuffer(sMottoText, frameBuffer), "WriteByte() failed");

    VerifyOrQuit(frameBuffer.GetLength() == sizeof(sMottoText) - 1, "GetLength() failed");
    VerifyOrQuit(memcmp(frameBuffer.GetFrame(), sMottoText, frameBuffer.GetLength()) == 0,
                 "GetFrame() content is incorrect");

    frameBuffer.Clear();
    VerifyOrQuit(frameBuffer.CanWrite(kBufferSize), "CanWrite(kBufferSize) failed unexpectedly");
    VerifyOrQuit(frameBuffer.CanWrite(kBufferSize + 1) == false, "CanWrite(kBufferSize + 1) did not fail as expected");

    for (uint16_t i = 0; i < kBufferSize; i++)
    {
        VerifyOrQuit(frameBuffer.CanWrite(1), "CanWrite() failed unexpectedly");
        SuccessOrQuit(frameBuffer.WriteByte(i & 0xff), "WriteByte() failed unexpectedly");
    }

    VerifyOrQuit(frameBuffer.CanWrite(1) == false, "CanWrite() did not fail with full buffer");
    VerifyOrQuit(frameBuffer.WriteByte(0) == OT_ERROR_NO_BUFS, "WriteByte() did not fail with full buffer");

    printf(" -- PASS\n");
}

void TestHdlcMultiFrameBuffer(void)
{
    Hdlc::MultiFrameBuffer<kBufferSize> frameBuffer;
    uint8_t *                           frame;
    uint16_t                            length;

    printf("Testing Hdlc::MultiFrameBuffer");

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Check state after constructor

    VerifyOrQuit(!frameBuffer.HasFrame(), "HasFrame() failed after constructor");
    VerifyOrQuit(!frameBuffer.HasSavedFrame(), "HasSavedFrame() failed after constructor");
    VerifyOrQuit(frameBuffer.GetLength() == 0, "GetLength() failed after constructor");
    VerifyOrQuit(frameBuffer.ReadSavedFrame(frame, length) == OT_ERROR_NOT_FOUND,
                 "ReadSavedFrame() incorrect behavior after constructor");

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Write multiple frames, save them and read later

    SuccessOrQuit(WriteToBuffer(sMottoText, frameBuffer), "WriteByte() failed");

    VerifyOrQuit(frameBuffer.GetLength() == sizeof(sMottoText) - 1, "GetLength() failed");
    VerifyOrQuit(memcmp(frameBuffer.GetFrame(), sMottoText, frameBuffer.GetLength()) == 0,
                 "GetFrame() content is incorrect");

    frameBuffer.SaveFrame();

    VerifyOrQuit(!frameBuffer.HasFrame(), "HasFrame() failed after SaveFrame()");
    VerifyOrQuit(frameBuffer.HasSavedFrame(), "HasSavedFrame() failed after SaveFrame()");
    VerifyOrQuit(frameBuffer.GetLength() == 0, "GetLength() failed after SaveFrame()");

    SuccessOrQuit(WriteToBuffer(sHelloText, frameBuffer), "WriteByte() failed");
    VerifyOrQuit(frameBuffer.GetLength() == sizeof(sHelloText) - 1, "GetLength() failed");
    VerifyOrQuit(memcmp(frameBuffer.GetFrame(), sHelloText, frameBuffer.GetLength()) == 0,
                 "GetFrame() content is incorrect");

    frameBuffer.SaveFrame();

    VerifyOrQuit(!frameBuffer.HasFrame(), "HasFrame() failed after SaveFrame()");
    VerifyOrQuit(frameBuffer.HasSavedFrame(), "HasSavedFrame() failed after SaveFrame()");
    VerifyOrQuit(frameBuffer.GetLength() == 0, "GetLength() failed after SaveFrame()");

    SuccessOrQuit(WriteToBuffer(sOpenThreadText, frameBuffer), "WriteByte() failed");
    VerifyOrQuit(frameBuffer.GetLength() == sizeof(sOpenThreadText) - 1, "GetLength() failed");
    VerifyOrQuit(memcmp(frameBuffer.GetFrame(), sOpenThreadText, frameBuffer.GetLength()) == 0,
                 "GetFrame() content is incorrect");

    frameBuffer.DiscardFrame();

    VerifyOrQuit(!frameBuffer.HasFrame(), "HasFrame() failed after DiscardFrame()");
    VerifyOrQuit(frameBuffer.HasSavedFrame(), "HasSavedFrame() failed after SaveFrame()");
    VerifyOrQuit(frameBuffer.GetLength() == 0, "GetLength() failed after DiscardFrame()");

    SuccessOrQuit(WriteToBuffer(sMottoText, frameBuffer), "WriteByte() failed");
    VerifyOrQuit(frameBuffer.GetLength() == sizeof(sMottoText) - 1, "GetLength() failed");
    VerifyOrQuit(memcmp(frameBuffer.GetFrame(), sMottoText, frameBuffer.GetLength()) == 0,
                 "GetFrame() content is incorrect");

    frameBuffer.DiscardFrame();

    VerifyOrQuit(!frameBuffer.HasFrame(), "HasFrame() failed after DiscardFrame()");
    VerifyOrQuit(frameBuffer.GetLength() == 0, "GetLength() failed after DiscardFrame()");

    SuccessOrQuit(WriteToBuffer(sHexText, frameBuffer), "WriteByte() failed");
    VerifyOrQuit(frameBuffer.GetLength() == sizeof(sHexText) - 1, "GetLength() failed");
    VerifyOrQuit(memcmp(frameBuffer.GetFrame(), sHexText, frameBuffer.GetLength()) == 0,
                 "GetFrame() content is incorrect");

    frameBuffer.SaveFrame();

    VerifyOrQuit(!frameBuffer.HasFrame(), "HasFrame() failed after SaveFrame()");
    VerifyOrQuit(frameBuffer.HasSavedFrame(), "HasSavedFrame() failed after SaveFrame()");
    VerifyOrQuit(frameBuffer.GetLength() == 0, "GetLength() failed after SaveFrame()");

    SuccessOrQuit(WriteToBuffer(sOpenThreadText, frameBuffer), "WriteByte() failed");
    VerifyOrQuit(frameBuffer.GetLength() == sizeof(sOpenThreadText) - 1, "GetLength() failed");
    VerifyOrQuit(memcmp(frameBuffer.GetFrame(), sOpenThreadText, frameBuffer.GetLength()) == 0,
                 "GetFrame() content is incorrect");

    // Read the first saved frame and check the content
    SuccessOrQuit(frameBuffer.ReadSavedFrame(frame, length), "ReadSavedFrame() failed unexpectedly");
    VerifyOrQuit(length == sizeof(sMottoText) - 1, "ReadSavedFrame() returned length is incorrect");
    VerifyOrQuit(memcmp(frame, sMottoText, length) == 0, "ReadSavedFrame() returned frame content is incorrect");

    // Read the second saved frame and check the content
    SuccessOrQuit(frameBuffer.ReadSavedFrame(frame, length), "ReadSavedFrame() failed unexpectedly");
    VerifyOrQuit(length == sizeof(sHelloText) - 1, "ReadSavedFrame() returned length is incorrect");
    VerifyOrQuit(memcmp(frame, sHelloText, length) == 0, "ReadSavedFrame() returned frame content is incorrect");

    // Read the third saved frame and check the content
    SuccessOrQuit(frameBuffer.ReadSavedFrame(frame, length), "ReadSavedFrame() failed unexpectedly");
    VerifyOrQuit(length == sizeof(sHexText) - 1, "ReadSavedFrame() returned length is incorrect");
    VerifyOrQuit(memcmp(frame, sHexText, length) == 0, "ReadSavedFrame() returned frame content is incorrect");

    VerifyOrQuit(frameBuffer.ReadSavedFrame(frame, length) == OT_ERROR_NOT_FOUND,
                 "ReadSavedFrame() incorrect behavior after all frames were read");

    VerifyOrQuit(frameBuffer.GetLength() == sizeof(sOpenThreadText) - 1, "GetLength() failed");
    VerifyOrQuit(memcmp(frameBuffer.GetFrame(), sOpenThreadText, frameBuffer.GetLength()) == 0,
                 "GetFrame() content is incorrect");

    frameBuffer.SaveFrame();

    // Read the fourth saved frame and check the content
    SuccessOrQuit(frameBuffer.ReadSavedFrame(frame, length), "ReadSavedFrame() failed unexpectedly");
    VerifyOrQuit(length == sizeof(sOpenThreadText) - 1, "ReadSavedFrame() returned length is incorrect");
    VerifyOrQuit(memcmp(frame, sOpenThreadText, length) == 0, "ReadSavedFrame() returned frame content is incorrect");

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Verify behavior of `Clear()`

    frameBuffer.Clear();

    VerifyOrQuit(!frameBuffer.HasFrame(), "HasFrame() failed after Clear()");
    VerifyOrQuit(!frameBuffer.HasSavedFrame(), "HasSavedFrame() failed after Clear()");
    VerifyOrQuit(frameBuffer.GetLength() == 0, "GetLength() failed after Clear()");

    SuccessOrQuit(WriteToBuffer(sOpenThreadText, frameBuffer), "WriteByte() failed");
    frameBuffer.SaveFrame();

    SuccessOrQuit(WriteToBuffer(sHelloText, frameBuffer), "WriteByte() failed");
    frameBuffer.SaveFrame();
    VerifyOrQuit(frameBuffer.HasSavedFrame(), "HasFrame() incorrect behavior after SaveFrame()");

    SuccessOrQuit(frameBuffer.ReadSavedFrame(frame, length), "ReadSavedFrame() failed unexpectedly");
    VerifyOrQuit(frameBuffer.HasSavedFrame(), "HasFrame() incorrect behavior after SaveFrame()");

    frameBuffer.Clear();

    VerifyOrQuit(frameBuffer.ReadSavedFrame(frame, length) == OT_ERROR_NOT_FOUND,
                 "ReadSavedFrame() incorrect behavior after Clear()");

    VerifyOrQuit(!frameBuffer.HasFrame(), "HasFrame() incorrect behavior after Clear()");
    VerifyOrQuit(!frameBuffer.HasSavedFrame(), "HasFrame() incorrect behavior after Clear()");
    VerifyOrQuit(frameBuffer.CanWrite(kBufferSize - 1) == false, "CanWrite() incorrect behavior after Clear()");
    VerifyOrQuit(frameBuffer.CanWrite(kBufferSize - 2) == true, "CanWrite() incorrect behavior after Clear()");

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Verify behavior of `ClearReadFrames()`

    SuccessOrQuit(WriteToBuffer(sHelloText, frameBuffer), "WriteByte() failed");
    frameBuffer.SaveFrame();
    SuccessOrQuit(WriteToBuffer(sOpenThreadText, frameBuffer), "WriteByte() failed");
    frameBuffer.SaveFrame();
    SuccessOrQuit(WriteToBuffer(sMottoText, frameBuffer), "WriteByte() failed");
    frameBuffer.SaveFrame();
    SuccessOrQuit(WriteToBuffer(sHexText, frameBuffer), "WriteByte() failed");

    SuccessOrQuit(frameBuffer.ReadSavedFrame(frame, length), "ReadSavedFrame() failed unexpectedly");
    VerifyOrQuit(length == sizeof(sHelloText) - 1, "ReadSavedFrame() returned length is incorrect");
    VerifyOrQuit(memcmp(frame, sHelloText, length) == 0, "ReadSavedFrame() returned frame content is incorrect");

    frameBuffer.ClearReadFrames();

    VerifyOrQuit(frameBuffer.HasFrame(), "HasFrame() failed after ClearReadFrames()");
    VerifyOrQuit(frameBuffer.HasSavedFrame(), "HasSavedFrame() failed after ClearReadFrames()");

    VerifyOrQuit(frameBuffer.GetLength() == sizeof(sHexText) - 1, "Frame length is incorrect after ClearReadFrames()");
    VerifyOrQuit(memcmp(frameBuffer.GetFrame(), sHexText, frameBuffer.GetLength()) == 0,
                 "GetFrame() content is incorrect after ClearReadFrames()");

    frameBuffer.SaveFrame();

    SuccessOrQuit(WriteToBuffer(sHelloText, frameBuffer), "WriteByte() failed");

    SuccessOrQuit(frameBuffer.ReadSavedFrame(frame, length), "ReadSavedFrame() failed unexpectedly");
    VerifyOrQuit(length == sizeof(sOpenThreadText) - 1, "ReadSavedFrame() returned length is incorrect");
    VerifyOrQuit(memcmp(frame, sOpenThreadText, length) == 0, "ReadSavedFrame() returned frame content is incorrect");

    SuccessOrQuit(frameBuffer.ReadSavedFrame(frame, length), "ReadSavedFrame() failed unexpectedly");
    VerifyOrQuit(length == sizeof(sMottoText) - 1, "ReadSavedFrame() returned length is incorrect");
    VerifyOrQuit(memcmp(frame, sMottoText, length) == 0, "ReadSavedFrame() returned frame content is incorrect");

    SuccessOrQuit(frameBuffer.ReadSavedFrame(frame, length), "ReadSavedFrame() failed unexpectedly");
    VerifyOrQuit(length == sizeof(sHexText) - 1, "ReadSavedFrame() returned length is incorrect");
    VerifyOrQuit(memcmp(frame, sHexText, length) == 0, "ReadSavedFrame() returned frame content is incorrect");

    VerifyOrQuit(frameBuffer.GetLength() == sizeof(sHelloText) - 1,
                 "Frame length is incorrect after ClearReadFrames()");
    VerifyOrQuit(memcmp(frameBuffer.GetFrame(), sHelloText, frameBuffer.GetLength()) == 0,
                 "GetFrame() content is incorrect after ClearReadFrames()");

    frameBuffer.DiscardFrame();

    // Since all previously saved frames are read and last frame is discarded
    // the `DiscardFrame()` call should basically `Clear()` the buffer.

    VerifyOrQuit(!frameBuffer.HasFrame(), "HasFrame() incorrect behavior after all frames are read and discarded");
    VerifyOrQuit(!frameBuffer.HasSavedFrame(), "HasFrame() incorrect behavior after all read or discarded");
    VerifyOrQuit(frameBuffer.CanWrite(kBufferSize - 1) == false,
                 "CanWrite() incorrect behavior after all read or discarded");
    VerifyOrQuit(frameBuffer.CanWrite(kBufferSize - 2) == true,
                 "CanWrite() incorrect behavior after all read of discarded");

    frameBuffer.Clear();
    SuccessOrQuit(WriteToBuffer(sHelloText, frameBuffer), "WriteByte() failed");

    // Since there are no saved frames, the `ClearReadFrames()` call
    // here, should practically do nothing

    frameBuffer.ClearReadFrames();

    VerifyOrQuit(frameBuffer.GetLength() == sizeof(sHelloText) - 1, "GetLength() failed");
    VerifyOrQuit(memcmp(frameBuffer.GetFrame(), sHelloText, frameBuffer.GetLength()) == 0,
                 "GetFrame() content is incorrect");

    printf(" -- PASS\n");
}

struct DecoderContext
{
    bool    mWasCalled;
    otError mError;
};

void ProcessDecodedFrame(void *aContext, otError aError)
{
    DecoderContext &decoderContext = *static_cast<DecoderContext *>(aContext);

    decoderContext.mError     = aError;
    decoderContext.mWasCalled = true;
}

void TestEncoderDecoder(void)
{
    otError                             error;
    uint8_t                             byte;
    Hdlc::MultiFrameBuffer<kBufferSize> encoderBuffer;
    Hdlc::MultiFrameBuffer<kBufferSize> decoderBuffer;
    DecoderContext                      decoderContext;
    Hdlc::Encoder                       encoder(encoderBuffer);
    Hdlc::Decoder                       decoder(decoderBuffer, ProcessDecodedFrame, &decoderContext);
    uint8_t *                           frame;
    uint16_t                            length;
    uint8_t                             badShortFrame[3] = {kFlagSequence, 0xaa, kFlagSequence};

    printf("Testing Hdlc::Encoder and Hdlc::Decoder");

    SuccessOrQuit(encoder.BeginFrame(), "Encoder::BeginFrame() failed");
    SuccessOrQuit(encoder.Encode(sOpenThreadText, sizeof(sOpenThreadText) - 1), "Encoder::Encode() failed");
    SuccessOrQuit(encoder.EndFrame(), "Encoder::EndFrame() failed");
    encoderBuffer.SaveFrame();

    SuccessOrQuit(encoder.BeginFrame(), "Encoder::BeginFrame() failed");
    SuccessOrQuit(encoder.Encode(sMottoText, sizeof(sMottoText) - 1), "Encoder::Encode() failed");
    SuccessOrQuit(encoder.EndFrame(), "Encoder::EndFrame() failed");
    encoderBuffer.SaveFrame();

    SuccessOrQuit(encoder.BeginFrame(), "Encoder::BeginFrame() failed");
    SuccessOrQuit(encoder.Encode(sHdlcSpeicals, sizeof(sHdlcSpeicals)), "Encoder::Encode() failed");
    SuccessOrQuit(encoder.EndFrame(), "Encoder::EndFrame() failed");
    encoderBuffer.SaveFrame();

    SuccessOrQuit(encoder.BeginFrame(), "Encoder::BeginFrame() failed");
    SuccessOrQuit(encoder.Encode(sHelloText, sizeof(sHelloText) - 1), "Encoder::Encode() failed");
    SuccessOrQuit(encoder.EndFrame(), "Encoder::EndFrame() failed");
    encoderBuffer.SaveFrame();

    SuccessOrQuit(encoder.BeginFrame(), "Encoder::BeginFrame() failed");
    // Empty frame
    SuccessOrQuit(encoder.EndFrame(), "Encoder::EndFrame() failed");
    encoderBuffer.SaveFrame();

    byte = kFlagSequence;
    SuccessOrQuit(encoder.BeginFrame(), "Encoder::BeginFrame() failed");
    SuccessOrQuit(encoder.Encode(&byte, sizeof(uint8_t)), "Encoder::Encode() failed");
    SuccessOrQuit(encoder.EndFrame(), "Encoder::EndFrame() failed");
    encoderBuffer.SaveFrame();

    // Feed the encoded frames to decoder and save the content
    while (encoderBuffer.ReadSavedFrame(frame, length) == OT_ERROR_NONE)
    {
        decoderContext.mWasCalled = false;

        decoder.Decode(frame, length);

        VerifyOrQuit(decoderContext.mWasCalled, "Decoder::Decode() failed");
        VerifyOrQuit(decoderContext.mError == OT_ERROR_NONE, "Decoder::Decode() returned incorrect error code");

        decoderBuffer.SaveFrame();
    }

    // Verify the decoded frames match the original frames

    SuccessOrQuit(decoderBuffer.ReadSavedFrame(frame, length), "Incorrect decoded frame");
    VerifyOrQuit(length == sizeof(sOpenThreadText) - 1, "Decoded frame length does not match original frame");
    VerifyOrQuit(memcmp(frame, sOpenThreadText, length) == 0, "Decoded frame content does not match original frame");

    SuccessOrQuit(decoderBuffer.ReadSavedFrame(frame, length), "Incorrect decoded frame");
    VerifyOrQuit(length == sizeof(sMottoText) - 1, "Decoded frame length does not match original frame");
    VerifyOrQuit(memcmp(frame, sMottoText, length) == 0, "Decoded frame content does not match original frame");

    SuccessOrQuit(decoderBuffer.ReadSavedFrame(frame, length), "Incorrect decoded frame");
    VerifyOrQuit(length == sizeof(sHdlcSpeicals), "Decoded frame length does not match original frame");
    VerifyOrQuit(memcmp(frame, sHdlcSpeicals, length) == 0, "Decoded frame content does not match original frame");

    SuccessOrQuit(decoderBuffer.ReadSavedFrame(frame, length), "Incorrect decoded frame");
    VerifyOrQuit(length == sizeof(sHelloText) - 1, "Decoded frame length does not match original frame");
    VerifyOrQuit(memcmp(frame, sHelloText, length) == 0, "Decoded frame content does not match original frame");

    SuccessOrQuit(decoderBuffer.ReadSavedFrame(frame, length), "Incorrect decoded frame");
    VerifyOrQuit(length == 0, "Decoded frame length does not match original frame");

    SuccessOrQuit(decoderBuffer.ReadSavedFrame(frame, length), "Incorrect decoded frame");
    VerifyOrQuit(length == sizeof(uint8_t), "Decoded frame length does not match original frame");
    VerifyOrQuit(*frame == kFlagSequence, "Decoded frame content does not match original frame");

    VerifyOrQuit(decoderBuffer.ReadSavedFrame(frame, length) == OT_ERROR_NOT_FOUND, "Extra decoded frame");

    encoderBuffer.Clear();
    decoderBuffer.Clear();

    // Test `Encoder` behavior when running out of buffer space
    SuccessOrQuit(encoder.BeginFrame(), "Encoder::BeginFrame() failed");

    error = OT_ERROR_NONE;

    for (uint16_t i = 0; error == OT_ERROR_NONE; i++)
    {
        byte  = i & 0xff;
        error = encoder.Encode(&byte, sizeof(uint8_t));
    }

    VerifyOrQuit(encoder.Encode(&byte, sizeof(uint8_t)) == OT_ERROR_NO_BUFS,
                 "Encoder::Encode() did not fail with a full buffer");
    VerifyOrQuit(encoder.EndFrame(), "Encoder::EndFrame() did not fail with a full buffer");

    encoderBuffer.Clear();

    // Test `Decoder` behavior with incorrect FCS

    SuccessOrQuit(encoder.BeginFrame(), "Encoder::BeginFrame() failed");
    SuccessOrQuit(encoder.Encode(sMottoText, sizeof(sMottoText) - 1), "Encoder::Encode() failed");
    SuccessOrQuit(encoder.EndFrame(), "Encoder::EndFrame() failed");

    encoderBuffer.GetFrame()[0] ^= 0x0a; // Change the first byte in the frame to cause FCS failure

    decoderContext.mWasCalled = false;
    decoder.Decode(encoderBuffer.GetFrame(), encoderBuffer.GetLength());
    VerifyOrQuit(decoderContext.mWasCalled, "Decoder::Decode() failed");
    VerifyOrQuit(decoderContext.mError == OT_ERROR_PARSE, "Decoder::Decode() did not fail with bad FCS");

    decoderBuffer.Clear();

    // Test `Decoder` behavior with short frame (smaller than FCS)

    decoderContext.mWasCalled = false;
    decoder.Decode(badShortFrame, sizeof(badShortFrame));
    VerifyOrQuit(decoderContext.mWasCalled, "Decoder::Decode() failed");
    VerifyOrQuit(decoderContext.mError == OT_ERROR_PARSE, "Decoder::Decode() did not fail for short frame");

    decoderBuffer.Clear();

    // Test `Decoder` with back to back `kFlagSequence` and ensure callback is not invoked.

    byte                      = kFlagSequence;
    decoderContext.mWasCalled = false;
    decoder.Decode(&byte, sizeof(uint8_t));
    VerifyOrQuit(!decoderContext.mWasCalled, "Decoder::Decode() failed");
    decoder.Decode(&byte, sizeof(uint8_t));
    VerifyOrQuit(!decoderContext.mWasCalled, "Decoder::Decode() failed");
    decoder.Decode(&byte, sizeof(uint8_t));
    VerifyOrQuit(!decoderContext.mWasCalled, "Decoder::Decode() failed");
    decoder.Decode(&byte, sizeof(uint8_t));
    VerifyOrQuit(!decoderContext.mWasCalled, "Decoder::Decode() failed");

    printf(" -- PASS\n");
}

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

void TestFuzzEncoderDecoder(void)
{
    uint16_t                       length;
    uint8_t                        frame[kMaxFrameLength];
    Hdlc::FrameBuffer<kBufferSize> encoderBuffer;
    Hdlc::FrameBuffer<kBufferSize> decoderBuffer;
    DecoderContext                 decoderContext;
    Hdlc::Encoder                  encoder(encoderBuffer);
    Hdlc::Decoder                  decoder(decoderBuffer, ProcessDecodedFrame, &decoderContext);

    printf("Testing Hdlc::Encoder and Hdlc::Decoder with randomly generated frames");

    for (uint32_t iter = 0; iter < kFuzzTestIteration; iter++)
    {
        encoderBuffer.Clear();
        decoderBuffer.Clear();

        do
        {
            length = static_cast<uint16_t>(GetRandom(kMaxFrameLength));
        } while (length == 0);

        for (uint16_t i = 0; i < length; i++)
        {
            frame[i] = static_cast<uint8_t>(GetRandom(256));
        }

        SuccessOrQuit(encoder.BeginFrame(), "Encoder::BeginFrame() failed");
        SuccessOrQuit(encoder.Encode(frame, length), "Encoder::Encode() failed");
        SuccessOrQuit(encoder.EndFrame(), "Encoder::EndFrame() failed");

        VerifyOrQuit(!encoderBuffer.IsEmpty(), "Encoded frame is empty");
        VerifyOrQuit(encoderBuffer.GetLength() > length, "Encoded frame is too short");

        decoderContext.mWasCalled = false;
        decoder.Decode(encoderBuffer.GetFrame(), encoderBuffer.GetLength());
        VerifyOrQuit(decoderContext.mWasCalled, "Decoder::Decode() failed");
        VerifyOrQuit(decoderContext.mError == OT_ERROR_NONE, "Decoder::Decode() returned incorrect error code");

        VerifyOrQuit(!decoderBuffer.IsEmpty(), "Decoded frame is empty");
        VerifyOrQuit(decoderBuffer.GetLength() == length, "Decoded frame length does not match original frame");
        VerifyOrQuit(memcmp(decoderBuffer.GetFrame(), frame, length) == 0,
                     "Decoded frame content does not match original frame");
    }

    printf(" -- PASS\n");
}

} // namespace Ncp
} // namespace ot

#ifdef ENABLE_TEST_MAIN
int main(void)
{
    ot::Ncp::TestHdlcFrameBuffer();
    ot::Ncp::TestHdlcMultiFrameBuffer();
    ot::Ncp::TestEncoderDecoder();
    ot::Ncp::TestFuzzEncoderDecoder();
    printf("\nAll tests passed.\n");
    return 0;
}
#endif
