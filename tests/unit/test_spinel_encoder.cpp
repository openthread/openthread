/*
 *  Copyright (c) 2017, The OpenThread Authors.
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
#include "ncp/spinel_encoder.hpp"

#include "test_util.h"

namespace ot {
namespace Ncp {

enum
{
    kTestBufferSize = 800,
};

// Dump the buffer content to screen.
void DumpBuffer(const char *aTextMessage, uint8_t *aBuffer, uint16_t aBufferLength)
{
    enum
    {
        kBytesPerLine = 32, // Number of bytes per line.
    };

    char     charBuff[kBytesPerLine + 1];
    uint16_t counter;
    uint8_t  byte;

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

otError ReadFrame(NcpFrameBuffer &aNcpBuffer, uint8_t *aFrame, uint16_t &aFrameLen)
{
    otError error = OT_ERROR_NONE;

    SuccessOrExit(error = aNcpBuffer.OutFrameBegin());
    aFrameLen = aNcpBuffer.OutFrameGetLength();
    VerifyOrExit(aNcpBuffer.OutFrameRead(aFrameLen, aFrame) == aFrameLen, error = OT_ERROR_FAILED);
    SuccessOrExit(error = aNcpBuffer.OutFrameRemove());

exit:
    return error;
}

void TestSpinelEncoder(void)
{
    uint8_t        buffer[kTestBufferSize];
    NcpFrameBuffer ncpBuffer(buffer, kTestBufferSize);
    SpinelEncoder  encoder(ncpBuffer);

    uint8_t        frame[kTestBufferSize];
    uint16_t       frameLen;
    spinel_ssize_t parsedLen;

    const bool         kBool_1 = true;
    const bool         kBool_2 = false;
    const uint8_t      kUint8  = 0x42;
    const int8_t       kInt8   = -73;
    const uint16_t     kUint16 = 0xabcd;
    const int16_t      kInt16  = -567;
    const uint32_t     kUint32 = 0xdeadbeef;
    const int32_t      kInt32  = -123455678L;
    const uint64_t     kUint64 = 0xfe10dc32ba549876ULL;
    const int64_t      kInt64  = -9197712039090021561LL;
    const unsigned int kUint_1 = 9;
    const unsigned int kUint_2 = 0xa3;
    const unsigned int kUint_3 = 0x8765;
    const unsigned int kUint_4 = SPINEL_MAX_UINT_PACKED - 1;

    const spinel_ipv6addr_t kIp6Addr = {
        {0x6B, 0x41, 0x65, 0x73, 0x42, 0x68, 0x61, 0x76, 0x54, 0x61, 0x72, 0x7A, 0x49, 0x69, 0x61, 0x4E}};

    const spinel_eui48_t kEui48 = {
        {4, 8, 15, 16, 23, 42} // "Lost" EUI48!
    };

    const spinel_eui64_t kEui64 = {
        {2, 3, 5, 7, 11, 13, 17, 19}, // "Prime" EUI64!
    };

    const char kString_1[] = "OpenThread";
    const char kString_2[] = "";

    const uint16_t kData[] = {10, 20, 3, 15, 1000, 60, 16}; // ... then comes 17,18,19,20  :)

    bool               b_1, b_2;
    uint8_t            u8;
    int8_t             i8;
    uint16_t           u16;
    int16_t            i16;
    uint32_t           u32;
    int32_t            i32;
    uint64_t           u64;
    int64_t            i64;
    unsigned int       u_1, u_2, u_3, u_4;
    spinel_ipv6addr_t *ip6Addr;
    spinel_eui48_t *   eui48;
    spinel_eui64_t *   eui64;
    const char *       utf_1;
    const char *       utf_2;
    const uint8_t *    dataPtr;
    spinel_size_t      dataLen;

    memset(buffer, 0, sizeof(buffer));

    printf("\n- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    printf("\nTest 1: Encoding of simple types");

    SuccessOrQuit(encoder.BeginFrame(NcpFrameBuffer::kPriorityLow), "BeginFrame() failed.");
    SuccessOrQuit(encoder.WriteBool(kBool_1), "WriteBool() failed.");
    SuccessOrQuit(encoder.WriteBool(kBool_2), "WriteBool() failed.");
    SuccessOrQuit(encoder.WriteUint8(kUint8), "WriteUint8() failed.");
    SuccessOrQuit(encoder.WriteInt8(kInt8), "WriteUint8() failed.");
    SuccessOrQuit(encoder.WriteUint16(kUint16), "WriteUint16() failed.");
    SuccessOrQuit(encoder.WriteInt16(kInt16), "WriteInt16() failed.");
    SuccessOrQuit(encoder.WriteUint32(kUint32), "WriteUint32() failed.");
    SuccessOrQuit(encoder.WriteInt32(kInt32), "WriteUint32() failed.");
    SuccessOrQuit(encoder.WriteUint64(kUint64), "WriteUint64() failed.");
    SuccessOrQuit(encoder.WriteInt64(kInt64), "WriteUint64() failed.");
    SuccessOrQuit(encoder.WriteUintPacked(kUint_1), "WriteUintPacked() failed.");
    SuccessOrQuit(encoder.WriteUintPacked(kUint_2), "WriteUintPacked() failed.");
    SuccessOrQuit(encoder.WriteUintPacked(kUint_3), "WriteUintPacked() failed.");
    SuccessOrQuit(encoder.WriteUintPacked(kUint_4), "WriteUintPacked() failed.");
    SuccessOrQuit(encoder.WriteIp6Address(kIp6Addr), "WriteIp6Addr() failed.");
    SuccessOrQuit(encoder.WriteEui48(kEui48), "WriteEui48() failed.");
    SuccessOrQuit(encoder.WriteEui64(kEui64), "WriteEui64() failed.");
    SuccessOrQuit(encoder.WriteUtf8(kString_1), "WriteUtf8() failed.");
    SuccessOrQuit(encoder.WriteUtf8(kString_2), "WriteUtf8() failed.");
    SuccessOrQuit(encoder.WriteData((const uint8_t *)kData, sizeof(kData)), "WriteData() failed.");
    SuccessOrQuit(encoder.EndFrame(), "EndFrame() failed.");

    DumpBuffer("Buffer", buffer, 256);
    SuccessOrQuit(ReadFrame(ncpBuffer, frame, frameLen), "ReadFrame() failed.");
    DumpBuffer("Frame", frame, frameLen);

    parsedLen = spinel_datatype_unpack(
        frame, (spinel_size_t)frameLen,
        (SPINEL_DATATYPE_BOOL_S SPINEL_DATATYPE_BOOL_S SPINEL_DATATYPE_UINT8_S SPINEL_DATATYPE_INT8_S
             SPINEL_DATATYPE_UINT16_S SPINEL_DATATYPE_INT16_S SPINEL_DATATYPE_UINT32_S SPINEL_DATATYPE_INT32_S
                 SPINEL_DATATYPE_UINT64_S SPINEL_DATATYPE_INT64_S SPINEL_DATATYPE_UINT_PACKED_S
                     SPINEL_DATATYPE_UINT_PACKED_S SPINEL_DATATYPE_UINT_PACKED_S SPINEL_DATATYPE_UINT_PACKED_S
                         SPINEL_DATATYPE_IPv6ADDR_S SPINEL_DATATYPE_EUI48_S SPINEL_DATATYPE_EUI64_S
                             SPINEL_DATATYPE_UTF8_S SPINEL_DATATYPE_UTF8_S SPINEL_DATATYPE_DATA_S),
        &b_1, &b_2, &u8, &i8, &u16, &i16, &u32, &i32, &u64, &i64, &u_1, &u_2, &u_3, &u_4, &ip6Addr, &eui48, &eui64,
        &utf_1, &utf_2, &dataPtr, &dataLen);

    VerifyOrQuit(parsedLen == frameLen, "spinel parse failed");
    VerifyOrQuit(b_1 == kBool_1, "WriteBool() parse failed.");
    VerifyOrQuit(b_2 == kBool_2, "WriteBool() parse failed.");
    VerifyOrQuit(u8 == kUint8, "WriteUint8() parse failed.");
    VerifyOrQuit(i8 == kInt8, "WriteUint8() parse failed.");
    VerifyOrQuit(u16 == kUint16, "WriteUint16() parse failed.");
    VerifyOrQuit(i16 == kInt16, "WriteInt16() parse failed.");
    VerifyOrQuit(u32 == kUint32, "WriteUint32() parse failed.");
    VerifyOrQuit(i32 == kInt32, "WriteUint32() parse failed.");
    VerifyOrQuit(u64 == kUint64, "WriteUint64() parse failed.");
    VerifyOrQuit(i64 == kInt64, "WriteUint64() parse failed.");
    VerifyOrQuit(u_1 == kUint_1, "WriteUintPacked() parse failed.");
    VerifyOrQuit(u_2 == kUint_2, "WriteUintPacked() parse failed.");
    VerifyOrQuit(u_3 == kUint_3, "WriteUintPacked() parse failed.");
    VerifyOrQuit(u_4 == kUint_4, "WriteUintPacked() parse failed.");
    VerifyOrQuit(memcmp(ip6Addr, &kIp6Addr, sizeof(spinel_ipv6addr_t)) == 0, "WriteIp6Address() parse failed.");
    VerifyOrQuit(memcmp(eui48, &kEui48, sizeof(spinel_eui48_t)) == 0, "WriteEui48() parse failed.");
    VerifyOrQuit(memcmp(eui64, &kEui64, sizeof(spinel_eui64_t)) == 0, "WriteEui64() parse failed.");
    VerifyOrQuit(memcmp(utf_1, kString_1, sizeof(kString_1)) == 0, "WriteUtf8() parse failed.");
    VerifyOrQuit(memcmp(utf_2, kString_2, sizeof(kString_2)) == 0, "WriteUtf8() parse failed.");
    VerifyOrQuit(dataLen == sizeof(kData), "WriteData() parse failed.");
    VerifyOrQuit(memcmp(dataPtr, &kData, sizeof(kData)) == 0, "WriteData() parse failed.");

    printf(" -- PASS\n");

    printf("\n- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    printf("\nTest 2: Test a single simple struct.");

    SuccessOrQuit(encoder.BeginFrame(NcpFrameBuffer::kPriorityLow), "BeginFrame() failed.");
    SuccessOrQuit(encoder.WriteUint8(kUint8), "WriteUint8() failed.");
    SuccessOrQuit(encoder.OpenStruct(), "OpenStruct() failed.");
    {
        SuccessOrQuit(encoder.WriteUint32(kUint32), "WriteUint32() failed.");
        SuccessOrQuit(encoder.WriteEui48(kEui48), "WriteEui48() failed.");
        SuccessOrQuit(encoder.WriteUintPacked(kUint_3), "WriteUintPacked() failed.");
    }
    SuccessOrQuit(encoder.CloseStruct(), "CloseStruct() failed.");
    SuccessOrQuit(encoder.WriteInt16(kInt16), "WriteInt16() failed.");
    SuccessOrQuit(encoder.EndFrame(), "EndFrame() failed.");

    DumpBuffer("Buffer", buffer, 256);
    SuccessOrQuit(ReadFrame(ncpBuffer, frame, frameLen), "ReadFrame() failed.");
    DumpBuffer("Frame", frame, frameLen);

    parsedLen = spinel_datatype_unpack(
        frame, (spinel_size_t)frameLen,
        (SPINEL_DATATYPE_UINT8_S SPINEL_DATATYPE_STRUCT_S(
            SPINEL_DATATYPE_UINT32_S SPINEL_DATATYPE_EUI48_S SPINEL_DATATYPE_UINT_PACKED_S) SPINEL_DATATYPE_INT16_S

         ),
        &u8, &u32, &eui48, &u_3, &i16);

    VerifyOrQuit(parsedLen == frameLen, "spinel parse failed");
    VerifyOrQuit(u8 == kUint8, "WriteUint8() parse failed.");
    VerifyOrQuit(i16 == kInt16, "WriteInt16() parse failed.");
    VerifyOrQuit(u32 == kUint32, "WriteUint32() parse failed.");
    VerifyOrQuit(u_3 == kUint_3, "WriteUintPacked() parse failed.");
    VerifyOrQuit(memcmp(eui48, &kEui48, sizeof(spinel_eui48_t)) == 0, "WriteEui48() parse failed.");

    // Parse the struct as a "data with len".
    parsedLen = spinel_datatype_unpack(frame, (spinel_size_t)frameLen,
                                       (SPINEL_DATATYPE_UINT8_S SPINEL_DATATYPE_DATA_WLEN_S SPINEL_DATATYPE_INT16_S

                                        ),
                                       &u8, &dataPtr, &dataLen, &i16);

    VerifyOrQuit(parsedLen == frameLen, "spinel parse failed");
    VerifyOrQuit(u8 == kUint8, "WriteUint8() parse failed.");
    VerifyOrQuit(i16 == kInt16, "WriteInt16() parse failed.");

    printf(" -- PASS\n");

    printf("\n- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    printf("\nTest 3: Test multiple structs and struct within struct.");

    SuccessOrQuit(encoder.BeginFrame(NcpFrameBuffer::kPriorityLow), "BeginFrame() failed.");
    SuccessOrQuit(encoder.OpenStruct(), "OpenStruct() failed.");
    {
        SuccessOrQuit(encoder.WriteUint8(kUint8), "WriteUint8() failed.");
        SuccessOrQuit(encoder.WriteUtf8(kString_1), "WriteUtf8() failed.");
        SuccessOrQuit(encoder.OpenStruct(), "OpenStruct() failed.");
        {
            SuccessOrQuit(encoder.WriteBool(kBool_1), "WriteBool() failed.");
            SuccessOrQuit(encoder.WriteIp6Address(kIp6Addr), "WriteIp6Addr() failed.");
        }
        SuccessOrQuit(encoder.CloseStruct(), "CloseStruct() failed.");
        SuccessOrQuit(encoder.WriteUint16(kUint16), "WriteUint16() failed.");
    }
    SuccessOrQuit(encoder.CloseStruct(), "CloseStruct() failed.");
    SuccessOrQuit(encoder.WriteEui48(kEui48), "WriteEui48() failed.");
    SuccessOrQuit(encoder.OpenStruct(), "OpenStruct() failed.");
    {
        SuccessOrQuit(encoder.WriteUint32(kUint32), "WriteUint32() failed.");
    }
    SuccessOrQuit(encoder.CloseStruct(), "CloseStruct() failed.");
    SuccessOrQuit(encoder.WriteInt32(kInt32), "WriteUint32() failed.");
    SuccessOrQuit(encoder.EndFrame(), "EndFrame() failed.");

    DumpBuffer("Buffer", buffer, 256 + 100);

    SuccessOrQuit(ReadFrame(ncpBuffer, frame, frameLen), "ReadFrame() failed.");

    parsedLen = spinel_datatype_unpack(
        frame, (spinel_size_t)frameLen,
        (SPINEL_DATATYPE_STRUCT_S(SPINEL_DATATYPE_UINT8_S SPINEL_DATATYPE_UTF8_S SPINEL_DATATYPE_STRUCT_S(
            SPINEL_DATATYPE_BOOL_S SPINEL_DATATYPE_IPv6ADDR_S) SPINEL_DATATYPE_UINT16_S)
             SPINEL_DATATYPE_EUI48_S SPINEL_DATATYPE_STRUCT_S(SPINEL_DATATYPE_UINT32_S) SPINEL_DATATYPE_INT32_S),
        &u8, &utf_1, &b_1, &ip6Addr, &u16, &eui48, &u32, &i32);

    VerifyOrQuit(parsedLen == frameLen, "spinel parse failed");
    VerifyOrQuit(b_1 == kBool_1, "WriteBool() parse failed.");
    VerifyOrQuit(u8 == kUint8, "WriteUint8() parse failed.");
    VerifyOrQuit(u16 == kUint16, "WriteUint16() parse failed.");
    VerifyOrQuit(u32 == kUint32, "WriteUint32() parse failed.");
    VerifyOrQuit(i32 == kInt32, "WriteUint32() parse failed.");
    VerifyOrQuit(memcmp(ip6Addr, &kIp6Addr, sizeof(spinel_ipv6addr_t)) == 0, "WriteIp6Address() parse failed.");
    VerifyOrQuit(memcmp(eui48, &kEui48, sizeof(spinel_eui48_t)) == 0, "WriteEui48() parse failed.");
    VerifyOrQuit(memcmp(utf_1, kString_1, sizeof(kString_1)) == 0, "WriteUtf8() parse failed.");

    printf(" -- PASS\n");

    printf("\n- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    printf("\nTest 4: Test unclosed struct.");

    SuccessOrQuit(encoder.BeginFrame(NcpFrameBuffer::kPriorityLow), "BeginFrame() failed.");
    SuccessOrQuit(encoder.WriteUint8(kUint8), "WriteUint8() failed.");
    SuccessOrQuit(encoder.OpenStruct(), "OpenStruct() failed.");
    {
        SuccessOrQuit(encoder.WriteUint32(kUint32), "WriteUint32() failed.");
        SuccessOrQuit(encoder.OpenStruct(), "OpenStruct() failed.");
        {
            SuccessOrQuit(encoder.WriteEui48(kEui48), "WriteEui48() failed.");
            SuccessOrQuit(encoder.WriteUintPacked(kUint_3), "WriteUintPacked() failed.");
            // Do not close the structs expecting `EndFrame()` to close them.
        }
    }
    SuccessOrQuit(encoder.EndFrame(), "EndFrame() failed.");

    SuccessOrQuit(ReadFrame(ncpBuffer, frame, frameLen), "ReadFrame() failed.");

    parsedLen = spinel_datatype_unpack(
        frame, (spinel_size_t)frameLen,
        (SPINEL_DATATYPE_UINT8_S SPINEL_DATATYPE_STRUCT_S(
            SPINEL_DATATYPE_UINT32_S SPINEL_DATATYPE_STRUCT_S(SPINEL_DATATYPE_EUI48_S SPINEL_DATATYPE_UINT_PACKED_S))),
        &u8, &u32, &eui48, &u_3);

    VerifyOrQuit(parsedLen == frameLen, "spinel parse failed");
    VerifyOrQuit(u8 == kUint8, "WriteUint8() parse failed.");
    VerifyOrQuit(u32 == kUint32, "WriteUint32() parse failed.");
    VerifyOrQuit(u_3 == kUint_3, "WriteUintPacked() parse failed.");
    VerifyOrQuit(memcmp(eui48, &kEui48, sizeof(spinel_eui48_t)) == 0, "WriteEui48() parse failed.");

    printf(" -- PASS\n");

    printf("\n- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    printf("\nTest 5: Test saving position and reseting back to a saved position");

    SuccessOrQuit(encoder.BeginFrame(NcpFrameBuffer::kPriorityLow), "BeginFrame() failed.");
    SuccessOrQuit(encoder.WriteUint8(kUint8), "WriteUint8() failed.");
    SuccessOrQuit(encoder.OpenStruct(), "OpenStruct() failed.");
    {
        SuccessOrQuit(encoder.WriteUint32(kUint32), "WriteUint32() failed.");

        // Save position in middle a first open struct.
        SuccessOrQuit(encoder.SavePosition(), "SavePosition failed.");
        SuccessOrQuit(encoder.OpenStruct(), "OpenStruct() failed.");
        {
            SuccessOrQuit(encoder.WriteEui48(kEui48), "WriteEui48() failed.");
            SuccessOrQuit(encoder.WriteUintPacked(kUint_3), "WriteUintPacked() failed.");
        }

        // Reset to saved position in middle of the second open struct which should be discarded.

        SuccessOrQuit(encoder.ResetToSaved(), "ResetToSaved() failed.");

        SuccessOrQuit(encoder.WriteIp6Address(kIp6Addr), "WriteIp6Addr() failed.");
        SuccessOrQuit(encoder.WriteEui64(kEui64), "WriteEui64() failed.");
    }
    SuccessOrQuit(encoder.CloseStruct(), "CloseStruct() failed.");
    SuccessOrQuit(encoder.WriteUtf8(kString_1), "WriteUtf8() failed.");
    SuccessOrQuit(encoder.EndFrame(), "EndFrame() failed.");

    SuccessOrQuit(ReadFrame(ncpBuffer, frame, frameLen), "ReadFrame() failed.");

    parsedLen = spinel_datatype_unpack(
        frame, (spinel_size_t)frameLen,
        (SPINEL_DATATYPE_UINT8_S SPINEL_DATATYPE_STRUCT_S(
            SPINEL_DATATYPE_UINT32_S SPINEL_DATATYPE_IPv6ADDR_S SPINEL_DATATYPE_EUI64_S) SPINEL_DATATYPE_UTF8_S),
        &u8, &u32, &ip6Addr, &eui64, &utf_1);

    VerifyOrQuit(parsedLen == frameLen, "spinel parse failed");

    VerifyOrQuit(u8 == kUint8, "WriteUint8() parse failed.");
    VerifyOrQuit(u32 == kUint32, "WriteUint32() parse failed.");
    VerifyOrQuit(i32 == kInt32, "WriteUint32() parse failed.");
    VerifyOrQuit(memcmp(ip6Addr, &kIp6Addr, sizeof(spinel_ipv6addr_t)) == 0, "WriteIp6Address() parse failed.");
    VerifyOrQuit(memcmp(eui64, &kEui64, sizeof(spinel_eui64_t)) == 0, "WriteEui64() parse failed.");
    VerifyOrQuit(memcmp(utf_1, kString_1, sizeof(kString_1)) == 0, "WriteUtf8() parse failed.");

    printf(" -- PASS\n");
}

} // namespace Ncp
} // namespace ot

#ifdef ENABLE_TEST_MAIN
int main(void)
{
    ot::Ncp::TestSpinelEncoder();
    printf("\nAll tests passed.\n");
    return 0;
}
#endif
