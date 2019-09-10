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

#include "common/code_utils.hpp"
#include "common/debug.hpp"
#include "mac/mac.hpp"
#include "mac/mac_frame.hpp"
#include "radio/radio.hpp"
#include "utils/wrap_string.h"

#include "test_platform.h"
#include "test_util.h"

namespace ot {

bool CompareReversed(const uint8_t *aFirst, const uint8_t *aSecond, uint16_t aLength)
{
    bool matches = true;

    for (uint16_t i = 0; i < aLength; i++)
    {
        if (aFirst[i] != aSecond[aLength - 1 - i])
        {
            matches = false;
            break;
        }
    }

    return matches;
}

void TestMacAddress(void)
{
    const uint8_t           kExtAddr[OT_EXT_ADDRESS_SIZE] = {0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0};
    const Mac::ShortAddress kShortAddr                    = 0x1234;

    ot::Instance *  instance;
    Mac::Address    addr;
    Mac::ExtAddress extAddr;
    uint8_t         buffer[OT_EXT_ADDRESS_SIZE];

    instance = testInitInstance();
    VerifyOrQuit(instance != NULL, "NULL instance\n");

    // Mac::ExtAddress

    extAddr.GenerateRandom();
    VerifyOrQuit(extAddr.IsLocal(), "Random Extended Address should have its Local bit set\n");
    VerifyOrQuit(!extAddr.IsGroup(), "Random Extended Address should not have its Group bit set\n");

    extAddr.CopyTo(buffer);
    VerifyOrQuit(memcmp(extAddr.m8, buffer, OT_EXT_ADDRESS_SIZE) == 0, "ExtAddress::CopyTo() failed\n");

    extAddr.CopyTo(buffer, Mac::ExtAddress::kReverseByteOrder);
    VerifyOrQuit(CompareReversed(extAddr.m8, buffer, OT_EXT_ADDRESS_SIZE), "ExtAddress::CopyTo() failed\n");

    extAddr.Set(kExtAddr);
    VerifyOrQuit(memcmp(extAddr.m8, kExtAddr, OT_EXT_ADDRESS_SIZE) == 0, "ExtAddress::Set() failed\n");

    extAddr.Set(kExtAddr, Mac::ExtAddress::kReverseByteOrder);
    VerifyOrQuit(CompareReversed(extAddr.m8, kExtAddr, OT_EXT_ADDRESS_SIZE), "ExtAddress::Set() failed\n");

    extAddr.SetLocal(true);
    VerifyOrQuit(extAddr.IsLocal(), "ExtAddress::SetLocal() failed\n");
    extAddr.SetLocal(false);
    VerifyOrQuit(!extAddr.IsLocal(), "ExtAddress::SetLocal() failed\n");
    extAddr.ToggleLocal();
    VerifyOrQuit(extAddr.IsLocal(), "ExtAddress::SetLocal() failed\n");
    extAddr.ToggleLocal();
    VerifyOrQuit(!extAddr.IsLocal(), "ExtAddress::SetLocal() failed\n");

    extAddr.SetGroup(true);
    VerifyOrQuit(extAddr.IsGroup(), "ExtAddress::SetGroup() failed\n");
    extAddr.SetGroup(false);
    VerifyOrQuit(!extAddr.IsGroup(), "ExtAddress::SetGroup() failed\n");
    extAddr.ToggleGroup();
    VerifyOrQuit(extAddr.IsGroup(), "ExtAddress::SetGroup() failed\n");
    extAddr.ToggleGroup();
    VerifyOrQuit(!extAddr.IsGroup(), "ExtAddress::SetGroup() failed\n");

    // Mac::Address

    VerifyOrQuit(addr.IsNone(), "Address constructor failed\n");
    VerifyOrQuit(addr.GetType() == Mac::Address::kTypeNone, "Address::GetType() failed\n");

    addr.SetShort(kShortAddr);
    VerifyOrQuit(addr.GetType() == Mac::Address::kTypeShort, "Address::GetType() failed\n");
    VerifyOrQuit(addr.IsShort(), "Address::SetShort() failed\n");
    VerifyOrQuit(!addr.IsExtended(), "Address::SetShort() failed\n");
    VerifyOrQuit(addr.GetShort() == kShortAddr, "Address::GetShort() failed\n");

    addr.SetExtended(extAddr);
    VerifyOrQuit(addr.GetType() == Mac::Address::kTypeExtended, "Address::GetType() failed\n");
    VerifyOrQuit(!addr.IsShort(), "Address::SetExtended() failed\n");
    VerifyOrQuit(addr.IsExtended(), "Address::SetExtended() failed\n");
    VerifyOrQuit(addr.GetExtended() == extAddr, "Address::GetExtended() failed\n");

    addr.SetExtended(extAddr.m8, Mac::ExtAddress::kReverseByteOrder);
    VerifyOrQuit(addr.GetType() == Mac::Address::kTypeExtended, "Address::GetType() failed\n");
    VerifyOrQuit(!addr.IsShort(), "Address::SetExtended() failed\n");
    VerifyOrQuit(addr.IsExtended(), "Address::SetExtended() failed\n");
    VerifyOrQuit(CompareReversed(addr.GetExtended().m8, extAddr.m8, OT_EXT_ADDRESS_SIZE),
                 "Address::SetExtended() reverse byte order failed");

    addr.SetNone();
    VerifyOrQuit(addr.GetType() == Mac::Address::kTypeNone, "Address::GetType() failed\n");
    VerifyOrQuit(addr.IsNone(), "Address:SetNone() failed\n");
    VerifyOrQuit(!addr.IsShort(), "Address::SetNone() failed\n");
    VerifyOrQuit(!addr.IsExtended(), "Address::SetNone() failed\n");

    VerifyOrQuit(!addr.IsBroadcast(), "Address:IsBroadcast() failed\n");
    VerifyOrQuit(!addr.IsShortAddrInvalid(), "Address:IsShortAddrInvalid() failed\n");

    addr.SetExtended(extAddr);
    VerifyOrQuit(!addr.IsBroadcast(), "Address:IsBroadcast() failed\n");
    VerifyOrQuit(!addr.IsShortAddrInvalid(), "Address:IsShortAddrInvalid() failed\n");

    addr.SetShort(kShortAddr);
    VerifyOrQuit(!addr.IsBroadcast(), "Address:IsBroadcast() failed\n");
    VerifyOrQuit(!addr.IsShortAddrInvalid(), "Address:IsShortAddrInvalid() failed\n");

    addr.SetShort(Mac::kShortAddrBroadcast);
    VerifyOrQuit(addr.IsBroadcast(), "Address:IsBroadcast() failed\n");
    VerifyOrQuit(!addr.IsShortAddrInvalid(), "Address:IsShortAddrInvalid() failed\n");

    addr.SetShort(Mac::kShortAddrInvalid);
    VerifyOrQuit(!addr.IsBroadcast(), "Address:IsBroadcast() failed\n");
    VerifyOrQuit(addr.IsShortAddrInvalid(), "Address:IsShortAddrInvalid() failed\n");

    testFreeInstance(instance);
}

void CompareNetworkName(const Mac::NetworkName &aNetworkName, const char *aNameString)
{
    uint8_t len = static_cast<uint8_t>(strlen(aNameString));

    VerifyOrQuit(strcmp(aNetworkName.GetAsCString(), aNameString) == 0, "NetworkName does not match expected value\n");

    VerifyOrQuit(aNetworkName.GetAsData().GetLength() == len, "NetworkName:GetAsData().GetLength() is incorrect\n");
    VerifyOrQuit(memcmp(aNetworkName.GetAsData().GetBuffer(), aNameString, len) == 0,
                 "NetworkName:GetAsData().GetBuffer() is incorrect\n");
}

void TestMacNetworkName(void)
{
    const char kEmptyName[]   = "";
    const char kName1[]       = "network";
    const char kName2[]       = "network-name";
    const char kLongName[]    = "0123456789abcdef";
    const char kTooLongName[] = "0123456789abcdef0";

    char             buffer[sizeof(kTooLongName) + 2];
    uint8_t          len;
    Mac::NetworkName networkName;

    CompareNetworkName(networkName, kEmptyName);

    SuccessOrQuit(networkName.Set(Mac::NetworkName::Data(kName1, sizeof(kName1))), "NetworkName::Set() failed\n");
    CompareNetworkName(networkName, kName1);

    VerifyOrQuit(networkName.Set(Mac::NetworkName::Data(kName1, sizeof(kName1))) == OT_ERROR_ALREADY,
                 "NetworkName::Set() accepted same name without returning OT_ERROR_ALREADY");
    CompareNetworkName(networkName, kName1);

    VerifyOrQuit(networkName.Set(Mac::NetworkName::Data(kName1, sizeof(kName1) - 1)) == OT_ERROR_ALREADY,
                 "NetworkName::Set() accepted same name without returning OT_ERROR_ALREADY");

    SuccessOrQuit(networkName.Set(Mac::NetworkName::Data(kName2, sizeof(kName2))), "NetworkName::Set() failed\n");
    CompareNetworkName(networkName, kName2);

    SuccessOrQuit(networkName.Set(Mac::NetworkName::Data(kEmptyName, 0)), "NetworkName::Set() failed\n");
    CompareNetworkName(networkName, kEmptyName);

    SuccessOrQuit(networkName.Set(Mac::NetworkName::Data(kLongName, sizeof(kLongName))), "NetworkName::Set() failed\n");
    CompareNetworkName(networkName, kLongName);

    VerifyOrQuit(networkName.Set(Mac::NetworkName::Data(kLongName, sizeof(kLongName) - 1)) == OT_ERROR_ALREADY,
                 "NetworkName::Set() accepted same name without returning OT_ERROR_ALREADY");

    SuccessOrQuit(networkName.Set(Mac::NetworkName::Data(NULL, 0)), "NetworkName::Set() failed\n");
    CompareNetworkName(networkName, kEmptyName);

    SuccessOrQuit(networkName.Set(Mac::NetworkName::Data(kName1, sizeof(kName1))), "NetworkName::Set() failed\n");

    VerifyOrQuit(networkName.Set(Mac::NetworkName::Data(kTooLongName, sizeof(kTooLongName))) == OT_ERROR_INVALID_ARGS,
                 "NetworkName::Set() accepted an invalid (too long) name\n");

    CompareNetworkName(networkName, kName1);

    memset(buffer, 'a', sizeof(buffer));
    len = networkName.GetAsData().CopyTo(buffer, 1);
    VerifyOrQuit(len == 1, "NetworkName::Data::CopyTo() failed\n");
    VerifyOrQuit(buffer[0] == kName1[0], "NetworkName::Data::CopyTo() failed\n");
    VerifyOrQuit(buffer[1] == 'a', "NetworkName::Data::CopyTo() failed\n");

    memset(buffer, 'a', sizeof(buffer));
    len = networkName.GetAsData().CopyTo(buffer, sizeof(kName1) - 1);
    VerifyOrQuit(len == sizeof(kName1) - 1, "NetworkName::Data::CopyTo() failed\n");
    VerifyOrQuit(memcmp(buffer, kName1, sizeof(kName1) - 1) == 0, "NetworkName::Data::CopyTo() failed\n");
    VerifyOrQuit(buffer[sizeof(kName1)] == 'a', "NetworkName::Data::CopyTo() failed\n");

    memset(buffer, 'a', sizeof(buffer));
    len = networkName.GetAsData().CopyTo(buffer, sizeof(buffer));
    VerifyOrQuit(len == sizeof(kName1) - 1, "NetworkName::Data::CopyTo() failed\n");
    VerifyOrQuit(memcmp(buffer, kName1, sizeof(kName1) - 1) == 0, "NetworkName::Data::CopyTo() failed\n");
    VerifyOrQuit(buffer[sizeof(kName1)] == 0, "NetworkName::Data::CopyTo() failed\n");
}

void TestMacHeader(void)
{
    static const struct
    {
        uint16_t fcf;
        uint8_t  secCtl;
        uint8_t  headerLength;
    } tests[] = {
        {Mac::Frame::kFcfFrameVersion2006 | Mac::Frame::kFcfDstAddrNone | Mac::Frame::kFcfSrcAddrNone, 0, 3},
        {Mac::Frame::kFcfFrameVersion2006 | Mac::Frame::kFcfDstAddrNone | Mac::Frame::kFcfSrcAddrShort, 0, 7},
        {Mac::Frame::kFcfFrameVersion2006 | Mac::Frame::kFcfDstAddrNone | Mac::Frame::kFcfSrcAddrExt, 0, 13},
        {Mac::Frame::kFcfFrameVersion2006 | Mac::Frame::kFcfDstAddrShort | Mac::Frame::kFcfSrcAddrNone, 0, 7},
        {Mac::Frame::kFcfFrameVersion2006 | Mac::Frame::kFcfDstAddrExt | Mac::Frame::kFcfSrcAddrNone, 0, 13},
        {Mac::Frame::kFcfFrameVersion2006 | Mac::Frame::kFcfDstAddrShort | Mac::Frame::kFcfSrcAddrShort, 0, 11},
        {Mac::Frame::kFcfFrameVersion2006 | Mac::Frame::kFcfDstAddrShort | Mac::Frame::kFcfSrcAddrExt, 0, 17},
        {Mac::Frame::kFcfFrameVersion2006 | Mac::Frame::kFcfDstAddrExt | Mac::Frame::kFcfSrcAddrShort, 0, 17},
        {Mac::Frame::kFcfFrameVersion2006 | Mac::Frame::kFcfDstAddrExt | Mac::Frame::kFcfSrcAddrExt, 0, 23},

        {Mac::Frame::kFcfFrameVersion2006 | Mac::Frame::kFcfDstAddrShort | Mac::Frame::kFcfSrcAddrShort |
             Mac::Frame::kFcfPanidCompression,
         0, 9},
        {Mac::Frame::kFcfFrameVersion2006 | Mac::Frame::kFcfDstAddrShort | Mac::Frame::kFcfSrcAddrExt |
             Mac::Frame::kFcfPanidCompression,
         0, 15},
        {Mac::Frame::kFcfFrameVersion2006 | Mac::Frame::kFcfDstAddrExt | Mac::Frame::kFcfSrcAddrShort |
             Mac::Frame::kFcfPanidCompression,
         0, 15},
        {Mac::Frame::kFcfFrameVersion2006 | Mac::Frame::kFcfDstAddrExt | Mac::Frame::kFcfSrcAddrExt |
             Mac::Frame::kFcfPanidCompression,
         0, 21},

        {Mac::Frame::kFcfFrameVersion2006 | Mac::Frame::kFcfDstAddrShort | Mac::Frame::kFcfSrcAddrShort |
             Mac::Frame::kFcfPanidCompression | Mac::Frame::kFcfSecurityEnabled,
         Mac::Frame::kSecMic32 | Mac::Frame::kKeyIdMode1, 15},
        {Mac::Frame::kFcfFrameVersion2006 | Mac::Frame::kFcfDstAddrShort | Mac::Frame::kFcfSrcAddrShort |
             Mac::Frame::kFcfPanidCompression | Mac::Frame::kFcfSecurityEnabled,
         Mac::Frame::kSecMic32 | Mac::Frame::kKeyIdMode2, 19},
    };

    for (unsigned i = 0; i < OT_ARRAY_LENGTH(tests); i++)
    {
        uint8_t      psdu[Mac::Frame::kMTU];
        Mac::TxFrame frame;

        frame.mPsdu = psdu;

        frame.InitMacHeader(tests[i].fcf, tests[i].secCtl);
        printf("%d\n", frame.GetHeaderLength());
        VerifyOrQuit(frame.GetHeaderLength() == tests[i].headerLength, "MacHeader test failed\n");
    }
}

void VerifyChannelMaskContent(const Mac::ChannelMask &aMask, uint8_t *aChannels, uint8_t aLength)
{
    uint8_t index = 0;
    uint8_t channel;

    for (channel = Radio::kChannelMin; channel <= Radio::kChannelMax; channel++)
    {
        if (index < aLength)
        {
            if (channel == aChannels[index])
            {
                index++;
                VerifyOrQuit(aMask.ContainsChannel(channel), "ChannelMask.ContainsChannel() failed\n");
            }
            else
            {
                VerifyOrQuit(!aMask.ContainsChannel(channel), "ChannelMask.ContainsChannel() failed\n");
            }
        }
    }

    index   = 0;
    channel = Mac::ChannelMask::kChannelIteratorFirst;

    while (aMask.GetNextChannel(channel) == OT_ERROR_NONE)
    {
        VerifyOrQuit(channel == aChannels[index++], "ChannelMask.GetNextChannel() failed\n");
    }

    VerifyOrQuit(index == aLength, "ChannelMask.GetNextChannel() failed\n");

    if (aLength == 1)
    {
        VerifyOrQuit(aMask.IsSingleChannel(), "ChannelMask.IsSingleChannel() failed\n");
    }
    else
    {
        VerifyOrQuit(!aMask.IsSingleChannel(), "ChannelMask.IsSingleChannel() failed\n");
    }

    VerifyOrQuit(aLength == aMask.GetNumberOfChannels(), "ChannelMask.GetNumberOfChannels() failed\n");
}

void TestMacChannelMask(void)
{
    uint8_t all_channels[] = {11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26};
    uint8_t channels1[]    = {11, 14, 15, 16, 17, 20, 21, 22, 24, 25};
    uint8_t channels2[]    = {14, 21, 26};
    uint8_t channels3[]    = {14, 21};
    uint8_t channles4[]    = {20};

    Mac::ChannelMask mask1;
    Mac::ChannelMask mask2(Radio::kSupportedChannels);

    printf("Testing Mac::ChannelMask\n");

    VerifyOrQuit(mask1.IsEmpty(), "ChannelMask.IsEmpty failed\n");
    printf("empty = %s\n", mask1.ToString().AsCString());

    VerifyOrQuit(!mask2.IsEmpty(), "ChannelMask.IsEmpty failed\n");
    VerifyOrQuit(mask2.GetMask() == Radio::kSupportedChannels, "ChannelMask.GetMask() failed\n");
    printf("all_channels = %s\n", mask2.ToString().AsCString());

    mask1.SetMask(Radio::kSupportedChannels);
    VerifyOrQuit(!mask1.IsEmpty(), "ChannelMask.IsEmpty failed\n");
    VerifyOrQuit(mask1.GetMask() == Radio::kSupportedChannels, "ChannelMask.GetMask() failed\n");

    VerifyChannelMaskContent(mask1, all_channels, sizeof(all_channels));

    // Test ChannelMask::RemoveChannel()
    for (uint8_t index = 0; index < sizeof(all_channels) - 1; index++)
    {
        mask1.RemoveChannel(all_channels[index]);
        VerifyChannelMaskContent(mask1, &all_channels[index + 1], sizeof(all_channels) - 1 - index);
    }

    mask1.Clear();
    VerifyOrQuit(mask1.IsEmpty(), "ChannelMask.IsEmpty failed\n");
    VerifyChannelMaskContent(mask1, NULL, 0);

    for (uint16_t index = 0; index < sizeof(channels1); index++)
    {
        mask1.AddChannel(channels1[index]);
    }

    printf("channels1 = %s\n", mask1.ToString().AsCString());

    VerifyOrQuit(!mask1.IsEmpty(), "ChannelMask.IsEmpty failed\n");
    VerifyChannelMaskContent(mask1, channels1, sizeof(channels1));

    mask2.Clear();

    for (uint16_t index = 0; index < sizeof(channels2); index++)
    {
        mask2.AddChannel(channels2[index]);
    }

    printf("channels2 = %s\n", mask2.ToString().AsCString());

    VerifyOrQuit(!mask2.IsEmpty(), "ChannelMask.IsEmpty failed\n");
    VerifyChannelMaskContent(mask2, channels2, sizeof(channels2));

    mask1.Intersect(mask2);
    VerifyChannelMaskContent(mask1, channels3, sizeof(channels3));

    mask2.Clear();
    mask2.AddChannel(channles4[0]);
    VerifyChannelMaskContent(mask2, channles4, sizeof(channles4));

    printf("channels4 = %s\n", mask2.ToString().AsCString());

    mask1.Clear();
    mask2.Clear();
    VerifyOrQuit(mask1 == mask2, "ChannelMask.operator== failed\n");

    mask1.SetMask(Radio::kSupportedChannels);
    mask2.SetMask(Radio::kSupportedChannels);
    VerifyOrQuit(mask1 == mask2, "ChannelMask.operator== failed\n");

    mask1.Clear();
    VerifyOrQuit(mask1 != mask2, "ChannelMask.operator== failed\n");
}

} // namespace ot

#ifdef ENABLE_TEST_MAIN
int main(void)
{
    ot::TestMacAddress();
    ot::TestMacNetworkName();
    ot::TestMacHeader();
    ot::TestMacChannelMask();
    printf("All tests passed\n");
    return 0;
}
#endif
