/*
 *  Copyright (c) 2021, The OpenThread Authors.
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
#include <string.h>

#include "common/code_utils.hpp"
#include "lib/spinel/spinel_prop_display.hpp"

#include "test_util.hpp"

namespace ot {
namespace Spinel {

constexpr uint32_t kMaxBufferSize = 512;
char               sBuffer[kMaxBufferSize];

static int32_t SpinelDisplayPropSimpleDataType(uint32_t aDataType, ...)
{
    spinel_datatype_t dataType = static_cast<spinel_datatype_t>(aDataType);
    int32_t           ret;
    va_list           args;

    va_start(args, aDataType);
    ret = SpinelPropDisplaySimpleDataType(aDataType, /* aPtrArgs */ false, args, sBuffer, kMaxBufferSize);
    va_end(args);

    return ret;
}

static int32_t SpinelDisplayProp(spinel_prop_key_t aKey, bool aPtrArgs, uint16_t aBufSize, const char *aPackFormat, ...)
{
    int32_t ret;
    va_list args;

    va_start(args, aPackFormat);
    ret = SpinelPropDisplay(aKey, aPackFormat, aPtrArgs, args, sBuffer, aBufSize);
    va_end(args);

    return ret;
}

static void TestPropDisplaySimpleDataType(void)
{
    int32_t ret;

    const bool              kBool    = true;
    const uint8_t           kUint8   = 255;
    const int8_t            kInt8    = -127;
    const uint16_t          kUint16  = 65535;
    const int16_t           kInt16   = -32767;
    const uint32_t          kUint32  = 4294967295;
    const int32_t           kInt32   = -2147483647;
    const uint64_t          kUint64  = 18446744073709551615U;
    const int64_t           kInt64   = -9223372036854775807;
    const spinel_ipv6addr_t kIp6Addr = {
        {0xff, 0xee, 0xdd, 0xcc, 0xbb, 0xaa, 0x99, 0x88, 0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11, 0x00}};
    const uint8_t kData[] = {11, 13, 17, 23};

    const spinel_eui48_t kEui48 = {
        {4, 8, 15, 16, 23, 42} // "Lost" EUI48!
    };

    const spinel_eui64_t kEui64 = {
        {2, 3, 5, 7, 11, 13, 17, 19}, // "Prime" EUI64!
    };

    const uint8_t kUtf8[] = {0xE8, 0xB0, 0xB7, 0xE6, 0xAD, 0x8C}; // Google's Chinese Name

    memset(sBuffer, 0, sizeof(sBuffer));
    ret = SpinelDisplayPropSimpleDataType(SPINEL_DATATYPE_BOOL_C, kBool);
    VerifyOrQuit(ret > 0, "spinel_prop_display_simple_data_type failed");
    printf("%s\n", sBuffer);
    VerifyOrQuit(strcmp(sBuffer, "(BOOL: 1)") == 0, "spinel_prop_display_simple_data_type failed to display BOOL");

    memset(sBuffer, 0, sizeof(sBuffer));
    ret = SpinelDisplayPropSimpleDataType(SPINEL_DATATYPE_UINT8_C, kUint8);
    VerifyOrQuit(ret > 0, "spinel_prop_display_simple_data_type failed");
    printf("%s\n", sBuffer);
    VerifyOrQuit(strcmp(sBuffer, "(UINT8: 255)") == 0, "spinel_prop_display_simple_data_type failed to display UINT8");

    memset(sBuffer, 0, sizeof(sBuffer));
    ret = SpinelDisplayPropSimpleDataType(SPINEL_DATATYPE_INT8_C, kInt8);
    VerifyOrQuit(ret > 0, "spinel_prop_display_simple_data_type failed");
    printf("%s\n", sBuffer);
    VerifyOrQuit(strcmp(sBuffer, "(INT8: -127)") == 0, "spinel_prop_display_simple_data_type failed to display INT8");

    memset(sBuffer, 0, sizeof(sBuffer));
    ret = SpinelDisplayPropSimpleDataType(SPINEL_DATATYPE_UINT16_C, kUint16);
    VerifyOrQuit(ret > 0, "spinel_prop_display_simple_data_type failed");
    printf("%s\n", sBuffer);
    VerifyOrQuit(strcmp(sBuffer, "(UINT16: 65535)") == 0,
                 "spinel_prop_display_simple_data_type failed to display UINT16");

    memset(sBuffer, 0, sizeof(sBuffer));
    ret = SpinelDisplayPropSimpleDataType(SPINEL_DATATYPE_INT16_C, kInt16);
    VerifyOrQuit(ret > 0, "spinel_prop_display_simple_data_type failed");
    printf("%s\n", sBuffer);
    VerifyOrQuit(strcmp(sBuffer, "(INT16: -32767)") == 0,
                 "spinel_prop_display_simple_data_type failed to display INT16");

    memset(sBuffer, 0, sizeof(sBuffer));
    ret = SpinelDisplayPropSimpleDataType(SPINEL_DATATYPE_UINT32_C, kUint32);
    VerifyOrQuit(ret > 0, "spinel_prop_display_simple_data_type failed");
    printf("%s\n", sBuffer);
    VerifyOrQuit(strcmp(sBuffer, "(UINT32: 4294967295)") == 0,
                 "spinel_prop_display_simple_data_type failed to display UINT32");

    memset(sBuffer, 0, sizeof(sBuffer));
    ret = SpinelDisplayPropSimpleDataType(SPINEL_DATATYPE_INT32_C, kInt32);
    VerifyOrQuit(ret > 0, "spinel_prop_display_simple_data_type failed");
    printf("%s\n", sBuffer);
    VerifyOrQuit(strcmp(sBuffer, "(INT32: -2147483647)") == 0,
                 "spinel_prop_display_simple_data_type failed to display INT32");

    memset(sBuffer, 0, sizeof(sBuffer));
    ret = SpinelDisplayPropSimpleDataType(SPINEL_DATATYPE_UINT64_C, kUint64);
    VerifyOrQuit(ret > 0, "spinel_prop_display_simple_data_type failed");
    printf("%s\n", sBuffer);
    VerifyOrQuit(strcmp(sBuffer, "(UINT64: 18446744073709551615)") == 0,
                 "spinel_prop_display_simple_data_type failed to display UINT64");

    memset(sBuffer, 0, sizeof(sBuffer));
    ret = SpinelDisplayPropSimpleDataType(SPINEL_DATATYPE_INT64_C, kInt64);
    VerifyOrQuit(ret > 0, "spinel_prop_display_simple_data_type failed");
    printf("%s\n", sBuffer);
    VerifyOrQuit(strcmp(sBuffer, "(INT64: -9223372036854775807)") == 0,
                 "spinel_prop_display_simple_data_type failed to display INT64");

    memset(sBuffer, 0, sizeof(sBuffer));
    ret = SpinelDisplayPropSimpleDataType(SPINEL_DATATYPE_IPv6ADDR_C, &kIp6Addr);
    VerifyOrQuit(ret > 0, "spinel_prop_display_simple_data_type failed");
    printf("%s\n", sBuffer);
    VerifyOrQuit(strcmp(sBuffer, "(IPv6: ffee:ddcc:bbaa:9988:7766:5544:3322:1100)") == 0,
                 "spinel_prop_display_simple_data_type failed to display IPv6ADDR");

    memset(sBuffer, 0, sizeof(sBuffer));
    ret = SpinelDisplayPropSimpleDataType(SPINEL_DATATYPE_EUI48_C, &kEui48);
    VerifyOrQuit(ret > 0, "spinel_prop_display_simple_data_type failed");
    printf("%s\n", sBuffer);
    VerifyOrQuit(strcmp(sBuffer, "(EUI48: 4:8:f:10:17:2a)") == 0,
                 "spinel_prop_display_simple_data_type failed to display EUI48");

    memset(sBuffer, 0, sizeof(sBuffer));
    ret = SpinelDisplayPropSimpleDataType(SPINEL_DATATYPE_EUI64_C, &kEui64);
    VerifyOrQuit(ret > 0, "spinel_prop_display_simple_data_type failed");
    printf("%s\n", sBuffer);
    VerifyOrQuit(strcmp(sBuffer, "(EUI64: 2:3:5:7:b:d:11:13)") == 0,
                 "spinel_prop_display_simple_data_type failed to display EUI64");

    memset(sBuffer, 0, sizeof(sBuffer));
    ret = SpinelDisplayPropSimpleDataType(SPINEL_DATATYPE_UTF8_C, &kUtf8, sizeof(kUtf8));
    VerifyOrQuit(ret > 0, "spinel_prop_display_simple_data_type failed");
    printf("%s\n", sBuffer);
    VerifyOrQuit(strcmp(sBuffer, "(UTF8: e8b0b7e6ad8c)") == 0,
                 "spinel_prop_display_simple_data_type failed to display UTF8");

    memset(sBuffer, 0, sizeof(sBuffer));
    ret = SpinelDisplayPropSimpleDataType(SPINEL_DATATYPE_DATA_C, &kData, sizeof(kData));
    VerifyOrQuit(ret > 0, "spinel_prop_display_simple_data_type failed");
    printf("%s\n", sBuffer);
    VerifyOrQuit(strcmp(sBuffer, "(DATA: 0b0d1117)") == 0,
                 "spinel_prop_display_simple_data_type failed to display DATA");
}

static void TestGetPropDisplay(void)
{
    int32_t ret;

    const bool     kBool     = false;
    const int8_t   kInt8     = 1;
    const uint16_t kUint16   = 2;
    const uint32_t kUint32_1 = 111;
    const uint32_t kUint32_2 = 222;
    const uint32_t kUint32_3 = 333;
    const uint32_t kUint32_4 = 444;
    const uint32_t kUint32_5 = 555;
    const uint32_t kUint32_6 = 666;
    const uint32_t kUint32_7 = 777;
    const uint32_t kUint32_8 = 888;
    const uint32_t kUint32_9 = 999;

    memset(sBuffer, 0, sizeof(sBuffer));
    ret = SpinelDisplayProp(SPINEL_PROP_PHY_TX_POWER, /* aPtrArgs */ true, sizeof(sBuffer), SPINEL_DATATYPE_INT8_S,
                            &kInt8);
    VerifyOrQuit(ret > 0, "Get Prop display failed, SPINEL_PROP_PHY_TX_POWER");
    printf("%s\n", sBuffer);

    memset(sBuffer, 0, sizeof(sBuffer));
    ret = SpinelDisplayProp(SPINEL_PROP_RADIO_COEX_ENABLE, /* aPtrArgs */ true, sizeof(sBuffer), SPINEL_DATATYPE_BOOL_S,
                            &kBool);
    VerifyOrQuit(ret > 0, "Get Prop display failed, SPINEL_PROP_RADIO_COEX_ENABLE");
    printf("%s\n", sBuffer);

    memset(sBuffer, 0, sizeof(sBuffer));
    ret = SpinelDisplayProp(
        SPINEL_PROP_RADIO_COEX_METRICS, /* aPtrArgs */ true, sizeof(sBuffer),
        SPINEL_DATATYPE_STRUCT_S(
            SPINEL_DATATYPE_UINT32_S SPINEL_DATATYPE_UINT32_S SPINEL_DATATYPE_UINT32_S SPINEL_DATATYPE_UINT32_S
                SPINEL_DATATYPE_UINT32_S SPINEL_DATATYPE_UINT32_S SPINEL_DATATYPE_UINT32_S SPINEL_DATATYPE_UINT32_S)
            SPINEL_DATATYPE_STRUCT_S(SPINEL_DATATYPE_UINT32_S SPINEL_DATATYPE_UINT32_S SPINEL_DATATYPE_UINT32_S
                                         SPINEL_DATATYPE_UINT32_S SPINEL_DATATYPE_UINT32_S SPINEL_DATATYPE_UINT32_S
                                             SPINEL_DATATYPE_UINT32_S SPINEL_DATATYPE_UINT32_S SPINEL_DATATYPE_UINT32_S)
                SPINEL_DATATYPE_BOOL_S SPINEL_DATATYPE_UINT32_S,
        &kUint32_1, &kUint32_2, &kUint32_3, &kUint32_4, &kUint32_5, &kUint32_6, &kUint32_7, &kUint32_8, &kUint32_9,
        &kUint32_1, &kUint32_2, &kUint32_3, &kUint32_4, &kUint32_5, &kUint32_6, &kUint32_7, &kUint32_8, &kBool,
        &kUint32_9);
    VerifyOrQuit(ret > 0, "Get Prop display failed, SPINEL_PROP_RADIO_COEX_METRICS");
    printf("%s\n", sBuffer);

    memset(sBuffer, 0, sizeof(sBuffer));
    ret = SpinelDisplayProp(SPINEL_PROP_PHY_REGION_CODE, /* aPtrArgs */ true, sizeof(sBuffer), SPINEL_DATATYPE_UINT16_S,
                            &kUint16);
    VerifyOrQuit(ret > 0, "Get Prop display failed, SPINEL_PROP_PHY_REGION_CODE");
    printf("%s\n", sBuffer);
}

static void TestSetPropDisplay(void)
{
}

static void TestInvalidPropDisplay(void)
{
    int32_t            ret;
    const uint16_t     kUint16          = 1;
    constexpr uint32_t kShortBufferSize = 8;
    const char *       kInvalidDataType = "Z";

    // Test buffer overflow
    memset(sBuffer, 0, sizeof(sBuffer));
    ret = SpinelDisplayProp(SPINEL_PROP_PHY_TX_POWER, /* aPtrArgs */ true, kShortBufferSize, SPINEL_DATATYPE_INT8_S,
                            &kUint16);
    VerifyOrQuit(ret == -1, "SpinelDisplayProp should get overflow error");

    // Test invalid spinel data type
    memset(sBuffer, 0, sizeof(sBuffer));
    ret = SpinelDisplayProp(SPINEL_PROP_PHY_TX_POWER, /* aPtrArgs */ true, sizeof(sBuffer), kInvalidDataType, &kUint16);
    VerifyOrQuit(ret == -1, "SpinelDisplayProp should get invalid data type error");

    // Test invalid pack format error
    memset(sBuffer, 0, sizeof(sBuffer));
    ret = SpinelDisplayProp(SPINEL_PROP_PHY_TX_POWER, /* aPtrArgs */ true, sizeof(sBuffer), "t(s", &kUint16);
    VerifyOrQuit(ret == -1, "SpinelDisplayProp should get invalid pack format error");

    memset(sBuffer, 0, sizeof(sBuffer));
    ret = SpinelDisplayProp(SPINEL_PROP_PHY_TX_POWER, /* aPtrArgs */ true, sizeof(sBuffer), "t((s)", &kUint16);
    VerifyOrQuit(ret == -1, "SpinelDisplayProp should get invalid pack format error");

    memset(sBuffer, 0, sizeof(sBuffer));
    ret = SpinelDisplayProp(SPINEL_PROP_PHY_TX_POWER, /* aPtrArgs */ true, sizeof(sBuffer), "ts", &kUint16);
    VerifyOrQuit(ret == -1, "SpinelDisplayProp should get invalid pack format error");

    memset(sBuffer, 0, sizeof(sBuffer));
    ret = SpinelDisplayProp(SPINEL_PROP_PHY_TX_POWER, /* aPtrArgs */ true, sizeof(sBuffer), "ts)", &kUint16);
    VerifyOrQuit(ret == -1, "SpinelDisplayProp should get invalid pack format error");
}

static void TestOtherPropDisplay(void)
{
    bool     kBool   = true;
    uint8_t  kUint8  = 1;
    int8_t   kInt8   = 2;
    uint16_t kUint16 = 3;
    int16_t  kInt16  = 4;
    uint32_t kUint32 = 5;
    int32_t  kInt32  = 6;
    uint64_t kUint64 = 7;
    int64_t  kInt64  = 8;

    memset(sBuffer, 0, sizeof(sBuffer));
    int32_t ret =
        SpinelDisplayProp(SPINEL_PROP_MAC_15_4_SADDR, /* aPtrArgs */ true, sizeof(sBuffer),
                          SPINEL_DATATYPE_STRUCT_S(SPINEL_DATATYPE_BOOL_S SPINEL_DATATYPE_UINT16_S), &kBool, &kUint16);

    VerifyOrQuit(ret > 0, "spinel_prop_display failed!");
    printf("ret:%d\n%s\n", ret, sBuffer);
}

} // namespace Spinel
} // namespace ot

int main(void)
{
    ot::Spinel::TestPropDisplaySimpleDataType();
    ot::Spinel::TestGetPropDisplay();
    ot::Spinel::TestSetPropDisplay();
    ot::Spinel::TestInvalidPropDisplay();
    ot::Spinel::TestOtherPropDisplay();
    printf("\nAll tests passed.\n");
    return 0;
}
