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

#include <stdio.h>

#include <openthread/ip6.h>
#include <openthread/platform/toolchain.h>

#include "test_util.h"
#include "thread/topology.hpp"
#include "utils/wrap_stdint.h"

extern "C" {
uint32_t       otNetifAddress_Size_c();
uint32_t       otNetifAddress_offset_mNext_c();
otNetifAddress CreateNetif_c();
}

void test_packed1()
{
    OT_TOOL_PACKED_BEGIN
    struct packed_t
    {
        uint8_t  mByte;
        uint32_t mWord;
        uint16_t mShort;
    } OT_TOOL_PACKED_END;

    CompileTimeAssert(sizeof(packed_t) == 7, "packed_t should be packed to 7 bytes");

    VerifyOrQuit(sizeof(packed_t) == 7, "Toolchain::OT_TOOL_PACKED failed 1\n");
}

void test_packed2()
{
    OT_TOOL_PACKED_BEGIN
    struct packed_t
    {
        uint8_t mBytes[3];
        uint8_t mByte;
    } OT_TOOL_PACKED_END;

    CompileTimeAssert(sizeof(packed_t) == 4, "packed_t should be packed to 4 bytes");

    VerifyOrQuit(sizeof(packed_t) == 4, "Toolchain::OT_TOOL_PACKED failed 2\n");
}

void test_packed_union()
{
    typedef struct
    {
        uint16_t mField;
    } nested_t;

    OT_TOOL_PACKED_BEGIN
    struct packed_t
    {
        uint8_t mBytes[3];
        union
        {
            nested_t mNestedStruct;
            uint8_t  mByte;
        } OT_TOOL_PACKED_FIELD;
    } OT_TOOL_PACKED_END;

    CompileTimeAssert(sizeof(packed_t) == 5, "packed_t should be packed to 5 bytes");

    VerifyOrQuit(sizeof(packed_t) == 5, "Toolchain::OT_TOOL_PACKED failed 3\n");
}

void test_packed_enum()
{
    ot::Neighbor neighbor;
    neighbor.SetState(ot::Neighbor::kStateValid);

    // Make sure that when we read the 3 bit field it is read as unsigned, so it return '4'
    VerifyOrQuit(neighbor.GetState() == ot::Neighbor::kStateValid, "Toolchain::OT_TOOL_PACKED failed 4\n");
}

void test_addr_sizes()
{
    VerifyOrQuit(offsetof(otNetifAddress, mNext) == otNetifAddress_offset_mNext_c(),
                 "mNext should offset the same in C & C++");
    VerifyOrQuit(sizeof(otNetifAddress) == otNetifAddress_Size_c(), "otNetifAddress should the same in C & C++");
}

void test_addr_bitfield()
{
    VerifyOrQuit(CreateNetif_c().mScopeOverrideValid == true, "Toolchain::test_addr_size_cpp\n");
}

void TestToolchain(void)
{
    test_packed1();
    test_packed2();
    test_packed_union();
    test_packed_enum();
    test_addr_sizes();
    test_addr_bitfield();
}

#ifdef ENABLE_TEST_MAIN
int main(void)
{
    TestToolchain();
    printf("All tests passed\n");
    return 0;
}
#endif
