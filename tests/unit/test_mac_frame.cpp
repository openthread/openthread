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
#include "utils/wrap_string.h"

#include "test_util.h"

namespace ot {

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
        uint8_t    psdu[Mac::Frame::kMTU];
        Mac::Frame frame;

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

    for (channel = OT_RADIO_CHANNEL_MIN; channel <= OT_RADIO_CHANNEL_MAX; channel++)
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
    Mac::ChannelMask mask2(OT_RADIO_SUPPORTED_CHANNELS);

    printf("Testing Mac::ChannelMask\n");

    VerifyOrQuit(mask1.IsEmpty(), "ChannelMask.IsEmpty failed\n");
    printf("empty = %s\n", mask1.ToString().AsCString());

    VerifyOrQuit(!mask2.IsEmpty(), "ChannelMask.IsEmpty failed\n");
    VerifyOrQuit(mask2.GetMask() == OT_RADIO_SUPPORTED_CHANNELS, "ChannelMask.GetMask() failed\n");
    printf("all_channels = %s\n", mask2.ToString().AsCString());

    mask1.SetMask(OT_RADIO_SUPPORTED_CHANNELS);
    VerifyOrQuit(!mask1.IsEmpty(), "ChannelMask.IsEmpty failed\n");
    VerifyOrQuit(mask1.GetMask() == OT_RADIO_SUPPORTED_CHANNELS, "ChannelMask.GetMask() failed\n");

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

    mask1.SetMask(OT_RADIO_SUPPORTED_CHANNELS);
    mask2.SetMask(OT_RADIO_SUPPORTED_CHANNELS);
    VerifyOrQuit(mask1 == mask2, "ChannelMask.operator== failed\n");

    mask1.Clear();
    VerifyOrQuit(mask1 != mask2, "ChannelMask.operator== failed\n");
}

} // namespace ot

#ifdef ENABLE_TEST_MAIN
int main(void)
{
    ot::TestMacHeader();
    ot::TestMacChannelMask();
    printf("All tests passed\n");
    return 0;
}
#endif
