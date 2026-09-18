/*
 *  Copyright (c) 2026, The OpenThread Authors.
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

#include <gtest/gtest.h>

#include <net/bpf.h>
#include <string.h>
#include <vector>

#include "posix/platform/bpf_tap.hpp"

using namespace ot::Posix;

namespace {

struct Seen
{
    std::vector<std::vector<uint8_t>> mFrames;

    static void Handle(void *aContext, const uint8_t *aFrame, uint16_t aLength)
    {
        static_cast<Seen *>(aContext)->mFrames.emplace_back(aFrame, aFrame + aLength);
    }
};

// One record as the kernel lays it out: the 18 header bytes, padding up to `aHeaderLength`, the frame, and
// padding to a word boundary.
void AddRecord(std::vector<uint8_t> &aBuffer,
               uint16_t              aHeaderLength,
               uint32_t              aCapLen,
               uint32_t              aDataLen,
               uint8_t               aFill)
{
    size_t start = aBuffer.size();
    size_t total = BPF_WORDALIGN(aHeaderLength + aCapLen);

    aBuffer.resize(start + total, 0);
    memcpy(&aBuffer[start + 8], &aCapLen, sizeof(aCapLen));
    memcpy(&aBuffer[start + 12], &aDataLen, sizeof(aDataLen));
    memcpy(&aBuffer[start + 16], &aHeaderLength, sizeof(aHeaderLength));
    memset(&aBuffer[start + aHeaderLength], aFill, aCapLen);
}

TEST(BpfTapParseBuffer, AcceptsEthernetAndTunnelHeaderLengths)
{
    // macOS pads the header so the frame is word-aligned: 18 bytes before a 14-byte Ethernet header, 20 before a
    // 4-byte tunnel header. Both are shorter than or equal to `sizeof(struct bpf_hdr)` (20 here).
    std::vector<uint8_t> buffer;
    Seen                 seen;

    AddRecord(buffer, 18, 60, 60, 0xa1);
    AddRecord(buffer, 18, 61, 61, 0xa2); // odd length: the next record is realigned
    AddRecord(buffer, 20, 44, 44, 0xa3);

    EXPECT_EQ(BpfTap::ParseBuffer(buffer.data(), buffer.size(), Seen::Handle, &seen), OT_ERROR_NONE);
    ASSERT_EQ(seen.mFrames.size(), 3u);
    EXPECT_EQ(seen.mFrames[0].size(), 60u);
    EXPECT_EQ(seen.mFrames[0][0], 0xa1);
    EXPECT_EQ(seen.mFrames[1].size(), 61u);
    EXPECT_EQ(seen.mFrames[1][60], 0xa2);
    EXPECT_EQ(seen.mFrames[2].size(), 44u);
    EXPECT_EQ(seen.mFrames[2][43], 0xa3);
}

TEST(BpfTapParseBuffer, SkipsTruncatedCaptures)
{
    std::vector<uint8_t> buffer;
    Seen                 seen;

    AddRecord(buffer, 18, 40, 1400, 0xb1); // snapshot shorter than the frame
    AddRecord(buffer, 18, 60, 60, 0xb2);

    EXPECT_EQ(BpfTap::ParseBuffer(buffer.data(), buffer.size(), Seen::Handle, &seen), OT_ERROR_NONE);
    ASSERT_EQ(seen.mFrames.size(), 1u);
    EXPECT_EQ(seen.mFrames[0][0], 0xb2);
}

TEST(BpfTapParseBuffer, RejectsInconsistentHeaders)
{
    std::vector<uint8_t> buffer;
    Seen                 seen;

    // A header length of 0 would otherwise loop for ever.
    AddRecord(buffer, 18, 60, 60, 0xc1);
    buffer[16] = 0;
    buffer[17] = 0;
    EXPECT_EQ(BpfTap::ParseBuffer(buffer.data(), buffer.size(), Seen::Handle, &seen), OT_ERROR_PARSE);
    EXPECT_EQ(seen.mFrames.size(), 0u);

    // A captured length beyond the buffer.
    buffer.clear();
    AddRecord(buffer, 18, 60, 60, 0xc2);
    buffer.resize(18 + 30);
    EXPECT_EQ(BpfTap::ParseBuffer(buffer.data(), buffer.size(), Seen::Handle, &seen), OT_ERROR_PARSE);
    EXPECT_EQ(seen.mFrames.size(), 0u);

    // A trailing partial header is not a record.
    buffer.clear();
    AddRecord(buffer, 18, 60, 60, 0xc3);
    buffer.resize(buffer.size() + 10, 0);
    EXPECT_EQ(BpfTap::ParseBuffer(buffer.data(), buffer.size(), Seen::Handle, &seen), OT_ERROR_NONE);
    EXPECT_EQ(seen.mFrames.size(), 1u);
}

} // namespace
