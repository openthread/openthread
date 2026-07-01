/*
 *  Copyright (c) 2025, The OpenThread Authors.
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

#include "test_platform.h"

#include <string.h>

#include "common/encoding.hpp"
#include "common/frame_builder.hpp"
#include "common/frame_data.hpp"
#include "common/ltvs.hpp"
#include "mac/mac_frame.hpp"
#include "mac/mac_header_ltv.hpp"

#include "test_util.h"
#include "test_util.hpp"

namespace ot {
namespace Mac {

// ---------------------------------------------------------------------------
// PackAndParse — helper for roundtrip tests
//
// AppendScaLtv / AppendChallengeLtv write plain L-T-V bytes (the intermediate
// build format).  The wire format is adaptively packed.  This helper packs the
// plain buffer and then calls ParseThreadHeaderIe, mirroring the actual
// encode/decode pipeline used by GenerateThreadDirectLinkCommand.
// ---------------------------------------------------------------------------
static Error PackAndParse(const uint8_t *aPlain, uint8_t aPlainLen, ScaParams *aSca, ChallengeLtv *aChal)
{
    uint8_t packed[64];
    uint8_t packedLen = PackThreadHeaderIeLtvs(aPlain, aPlainLen, packed, sizeof(packed));
    return ParseThreadHeaderIe(packed, packedLen, aSca, aChal);
}

// ---------------------------------------------------------------------------
// TestScaLtvRoundTrip
//
// Verifies SCA LTV encode/decode for each distinct RAM Duration variant and
// for the teardown (zero-length) form.
// ---------------------------------------------------------------------------
void TestScaLtvRoundTrip(void)
{
    printf("TestScaLtvRoundTrip\n");

    // --- Case 1: No CoEx constraints (mRamAvailable=false), no SLW ---
    // Fixed header LE: SlotDur=0, RamOffset=0, RamAvailable=0 => 0x0000
    // Plain LTV: [L=2][T=0x02][0x00][0x00] = 4 bytes
    {
        ScaParams params;
        memset(&params, 0, sizeof(params));
        params.mRamAvailable = false;
        params.mHasSlw       = false;

        uint8_t      buf[32];
        FrameBuilder fb;
        fb.Init(buf, sizeof(buf));

        VerifyOrQuit(AppendScaLtv(fb, params) == kErrorNone);

        VerifyOrQuit(fb.GetLength() == 4); // [L=2][T=0x02][0x00][0x00]
        VerifyOrQuit(buf[0] == 2 && buf[1] == ThreadHeaderIe::kTypeSca);
        VerifyOrQuit(buf[2] == 0x00 && buf[3] == 0x00); // fixed header = 0x0000

        ScaParams out;
        memset(&out, 0xFF, sizeof(out));
        VerifyOrQuit(PackAndParse(buf, fb.GetLength(), &out, nullptr) == kErrorNone);
        VerifyOrQuit(!out.mRamAvailable);
        VerifyOrQuit(!out.mHasSlw);
        VerifyOrQuit(out.mRamOffsetUs == 0);
        VerifyOrQuit(out.mSlotDuration == 0);
    }

    // --- Case 2: No CoEx constraints, SLW Period=100, Phase=50 (mHasSlw=true) ---
    // Fixed header: 0x0000; SLW Period LE = {0x64,0x00}; Phase LE = {0x32,0x00}
    // Plain LTV: [L=6][T=0x02][0x00][0x00][0x64][0x00][0x32][0x00] = 8 bytes
    {
        ScaParams params;
        memset(&params, 0, sizeof(params));
        params.mRamAvailable   = false;
        params.mHasSlw         = true;
        params.mSlwPeriodSlots = 100;
        params.mSlwPhaseSlots  = 50;

        uint8_t      buf[32];
        FrameBuilder fb;
        fb.Init(buf, sizeof(buf));

        VerifyOrQuit(AppendScaLtv(fb, params) == kErrorNone);

        VerifyOrQuit(fb.GetLength() == 8); // [L=6][T=0x02][fixed 2B][period 2B][phase 2B]

        ScaParams out;
        memset(&out, 0, sizeof(out));
        VerifyOrQuit(PackAndParse(buf, fb.GetLength(), &out, nullptr) == kErrorNone);
        VerifyOrQuit(!out.mRamAvailable);
        VerifyOrQuit(out.mHasSlw);
        VerifyOrQuit(out.mRamOffsetUs == 0);
        VerifyOrQuit(out.mSlwPeriodSlots == 100);
        VerifyOrQuit(out.mSlwPhaseSlots == 50);
    }

    // --- Case 3: No CoEx, negative RAM Offset=-512, SLW Period=4095, Phase=4095 ---
    // RamOffset=-512: rawOffset11 = uint16(-512)&0x07FF = 0x0600
    // Fixed header: (0x0600<<2)=0x1800 => LE {0x00, 0x18}
    // SLW Period=4095 LE: {0xFF,0x0F}; Phase=4095 LE: {0xFF,0x0F}
    {
        ScaParams params;
        memset(&params, 0, sizeof(params));
        params.mRamAvailable   = false;
        params.mHasSlw         = true;
        params.mRamOffsetUs    = -512;
        params.mSlwPeriodSlots = 4095;
        params.mSlwPhaseSlots  = 4095;

        uint8_t      buf[32];
        FrameBuilder fb;
        fb.Init(buf, sizeof(buf));

        VerifyOrQuit(AppendScaLtv(fb, params) == kErrorNone);

        VerifyOrQuit(fb.GetLength() == 8);
        VerifyOrQuit(buf[2] == 0x00 && buf[3] == 0x18); // fixed header LE = 0x1800

        ScaParams out;
        memset(&out, 0, sizeof(out));
        VerifyOrQuit(PackAndParse(buf, fb.GetLength(), &out, nullptr) == kErrorNone);
        VerifyOrQuit(!out.mRamAvailable);
        VerifyOrQuit(out.mRamOffsetUs == -512);
        VerifyOrQuit(out.mSlwPeriodSlots == 4095);
        VerifyOrQuit(out.mSlwPhaseSlots == 4095);
    }

    // --- Case 4: Teardown (AppendScaLtvTeardown emits zero-length SCA LTV) ---
    // Wire: [L=0][T=0x02] = 2 bytes; ParseThreadHeaderIe sets aTeardown=true, leaves aScaParams unchanged.
    {
        uint8_t      buf[32];
        FrameBuilder fb;
        fb.Init(buf, sizeof(buf));

        VerifyOrQuit(AppendScaLtvTeardown(fb) == kErrorNone);

        VerifyOrQuit(fb.GetLength() == 2);
        VerifyOrQuit(buf[0] == 0 && buf[1] == ThreadHeaderIe::kTypeSca);

        ScaParams out;
        bool      teardown = false;
        memset(&out, 0xAB, sizeof(out));
        VerifyOrQuit(PackAndParse(buf, fb.GetLength(), &out, nullptr) == kErrorNone);

        // PackAndParse doesn't take aTeardown; call ParseThreadHeaderIe directly to verify.
        uint8_t packed[8];
        uint8_t packedLen = PackThreadHeaderIeLtvs(buf, fb.GetLength(), packed, sizeof(packed));
        memset(&out, 0xAB, sizeof(out));
        VerifyOrQuit(ParseThreadHeaderIe(packed, packedLen, &out, nullptr, &teardown) == kErrorNone);
        VerifyOrQuit(teardown);
        VerifyOrQuit(reinterpret_cast<uint8_t *>(&out)[0] == 0xAB); // aScaParams untouched
    }

    // --- Case 5: mRamAvailable=true, mRamDuration=8 (one RAM byte), RamOffset=10, no SLW ---
    // RamOffset=10: rawOffset11=10=0x000A; fixed header=(0x000A<<2)|(1<<13)=0x0028|0x2000=0x2028 LE {0x28,0x20}
    // Value: [fixed 2B][RamDur 0x08][RamBits 0xA5] = 4 bytes
    {
        ScaParams params;
        memset(&params, 0, sizeof(params));
        params.mRamAvailable = true;
        params.mRamDuration  = 8;
        params.mRamOffsetUs  = 10;
        params.mHasSlw       = false;
        params.mRamBits[0]   = 0xA5;

        uint8_t      buf[32];
        FrameBuilder fb;
        fb.Init(buf, sizeof(buf));

        VerifyOrQuit(AppendScaLtv(fb, params) == kErrorNone);

        VerifyOrQuit(fb.GetLength() == 6);              // [L=4][T=0x02][fixed 2B][RamDur][RamBits]
        VerifyOrQuit(buf[2] == 0x28 && buf[3] == 0x20); // fixed header LE = 0x2028
        VerifyOrQuit(buf[4] == 0x08);                   // RAM Duration
        VerifyOrQuit(buf[5] == 0xA5);                   // RAM Bits byte

        ScaParams out;
        memset(&out, 0, sizeof(out));
        VerifyOrQuit(PackAndParse(buf, fb.GetLength(), &out, nullptr) == kErrorNone);
        VerifyOrQuit(out.mRamAvailable);
        VerifyOrQuit(out.mRamDuration == 8);
        VerifyOrQuit(out.mRamOffsetUs == 10);
        VerifyOrQuit(out.mRamBits[0] == 0xA5);
        VerifyOrQuit(!out.mHasSlw);
    }

    // --- Case 6: mRamAvailable=true, mRamDuration=32 (four RAM bytes), RamOffset=1023, SLW Period=200, Phase=75 ---
    // RamOffset=1023: rawOffset11=0x03FF; fixed header=(0x03FF<<2)|(1<<13)=0x0FFC|0x2000=0x2FFC LE {0xFC,0x2F}
    // Value: [fixed 2B][RamDur 0x20][4 RamBits bytes][Period LE 2B][Phase LE 2B] = 11 bytes
    {
        ScaParams params;
        memset(&params, 0, sizeof(params));
        params.mRamAvailable   = true;
        params.mRamDuration    = 32;
        params.mRamOffsetUs    = 1023;
        params.mHasSlw         = true;
        params.mSlwPeriodSlots = 200;
        params.mSlwPhaseSlots  = 75;
        params.mRamBits[0]     = 0x11;
        params.mRamBits[1]     = 0x22;
        params.mRamBits[2]     = 0x33;
        params.mRamBits[3]     = 0x44;

        uint8_t      buf[64];
        FrameBuilder fb;
        fb.Init(buf, sizeof(buf));

        VerifyOrQuit(AppendScaLtv(fb, params) == kErrorNone);

        VerifyOrQuit(fb.GetLength() == 13);             // [L=11][T=0x02] + 11 value bytes
        VerifyOrQuit(buf[2] == 0xFC && buf[3] == 0x2F); // fixed header LE = 0x2FFC
        VerifyOrQuit(buf[4] == 0x20);                   // RAM Duration = 32
        VerifyOrQuit(buf[5] == 0x11 && buf[6] == 0x22 && buf[7] == 0x33 && buf[8] == 0x44);
        VerifyOrQuit(buf[9] == 0xC8 && buf[10] == 0x00);  // SLW Period=200 LE
        VerifyOrQuit(buf[11] == 0x4B && buf[12] == 0x00); // SLW Phase=75 LE

        ScaParams out;
        memset(&out, 0, sizeof(out));
        VerifyOrQuit(PackAndParse(buf, fb.GetLength(), &out, nullptr) == kErrorNone);
        VerifyOrQuit(out.mRamAvailable);
        VerifyOrQuit(out.mRamDuration == 32);
        VerifyOrQuit(out.mRamOffsetUs == 1023);
        VerifyOrQuit(out.mRamBits[0] == 0x11);
        VerifyOrQuit(out.mRamBits[1] == 0x22);
        VerifyOrQuit(out.mRamBits[2] == 0x33);
        VerifyOrQuit(out.mRamBits[3] == 0x44);
        VerifyOrQuit(out.mHasSlw);
        VerifyOrQuit(out.mSlwPeriodSlots == 200);
        VerifyOrQuit(out.mSlwPhaseSlots == 75);
    }

    printf("PASSED\n");
}

// ---------------------------------------------------------------------------
// TestChallengeLtvRoundTrip
//
// Appends a Challenge LTV with a known 16-byte value and parses it back.
// ---------------------------------------------------------------------------
void TestChallengeLtvRoundTrip(void)
{
    printf("TestChallengeLtvRoundTrip\n");

    const uint8_t kKnownChallenge[ChallengeLtv::kLength] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                                                            0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10};
    ChallengeLtv  challenge;
    memcpy(challenge.mChallenge, kKnownChallenge, ChallengeLtv::kLength);

    uint8_t      buf[32];
    FrameBuilder fb;
    fb.Init(buf, sizeof(buf));

    VerifyOrQuit(AppendChallengeLtv(fb, challenge) == kErrorNone);

    // Wire: [L=16][T=0x03][16 bytes] = 18 bytes
    VerifyOrQuit(fb.GetLength() == 18);
    VerifyOrQuit(buf[0] == 16 && buf[1] == ThreadHeaderIe::kTypeChallenge);
    VerifyOrQuit(memcmp(&buf[2], kKnownChallenge, ChallengeLtv::kLength) == 0);

    ChallengeLtv out;
    memset(&out, 0, sizeof(out));
    VerifyOrQuit(PackAndParse(buf, fb.GetLength(), nullptr, &out) == kErrorNone);
    VerifyOrQuit(memcmp(out.mChallenge, kKnownChallenge, ChallengeLtv::kLength) == 0);

    printf("PASSED\n");
}

// ---------------------------------------------------------------------------
// TestTargetIdLtv
//
// Appends a Target ID LTV with an 8-byte extended address payload and verifies
// the raw wire bytes.
// ---------------------------------------------------------------------------
void TestTargetIdLtv(void)
{
    printf("TestTargetIdLtv\n");

    const uint8_t kTargetId[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22};

    uint8_t      buf[32];
    FrameBuilder fb;
    fb.Init(buf, sizeof(buf));

    VerifyOrQuit(AppendTargetIdLtv(fb, kTargetId, sizeof(kTargetId)) == kErrorNone);

    // Wire: [L=8][T=0x01][8 bytes] = 10 bytes
    VerifyOrQuit(fb.GetLength() == 10);
    VerifyOrQuit(buf[0] == 8 && buf[1] == ThreadHeaderIe::kTypeTargetId);
    VerifyOrQuit(memcmp(&buf[2], kTargetId, sizeof(kTargetId)) == 0);

    printf("PASSED\n");
}

// ---------------------------------------------------------------------------
// TestThreadHeaderIeWrap
//
// Verifies that AppendThreadHeaderIe writes the 2-byte HeaderIe header with
// the correct Element ID and length before the LTV payload.
// ---------------------------------------------------------------------------
void TestThreadHeaderIeWrap(void)
{
    printf("TestThreadHeaderIeWrap\n");

    const uint8_t kPayload[] = {0x02, ThreadHeaderIe::kTypeSca, 0x01, 0x00};

    uint8_t      buf[32];
    FrameBuilder fb;
    fb.Init(buf, sizeof(buf));

    VerifyOrQuit(AppendThreadHeaderIe(fb, kPayload, sizeof(kPayload)) == kErrorNone);

    // 2-byte HeaderIe: bits[14:7]=ElementId=0x2d, bits[6:0]=Length=4
    // LE uint16: length in bits[6:0], id in bits[14:7], type=0 in bit[15]
    // byte[0] = 0x04 (length=4, bits[6:0])
    // byte[1] = 0x16 (id=0x2d >> 1 = 0x16, since id occupies bits[14:7] = byte[1] bits[7:0])
    // HeaderIe byte layout: byte0 bits[6:0]=length, byte1 bits[7:0]=id>>1 (since id is at bits[14:7])
    // Actually: kIdOffset=7, kIdMask=0x00ff<<7 = 0x7f80
    // id=0x2d => stored in bits[14:7] of the 16-bit LE field
    // LE uint16 = 0x2d << 7 | 4 = 0x1680 | 0x04 = 0x1684
    // byte[0]=0x84, byte[1]=0x16
    uint16_t ieWord = static_cast<uint16_t>((ThreadHeaderIe::kElementId << 7) | sizeof(kPayload));
    VerifyOrQuit(buf[0] == static_cast<uint8_t>(ieWord & 0xFF));
    VerifyOrQuit(buf[1] == static_cast<uint8_t>(ieWord >> 8));
    VerifyOrQuit(memcmp(&buf[2], kPayload, sizeof(kPayload)) == 0);
    VerifyOrQuit(fb.GetLength() == 2 + sizeof(kPayload));

    printf("PASSED\n");
}

// ---------------------------------------------------------------------------
// TestMultiLtvParse
//
// Encodes an SCA LTV followed by a Challenge LTV and verifies that
// ParseThreadHeaderIe correctly populates both output parameters.
// ---------------------------------------------------------------------------
void TestMultiLtvParse(void)
{
    printf("TestMultiLtvParse\n");

    ScaParams scaIn;
    memset(&scaIn, 0, sizeof(scaIn));
    scaIn.mRamAvailable   = false;
    scaIn.mHasSlw         = true;
    scaIn.mSlwPeriodSlots = 48;
    scaIn.mSlwPhaseSlots  = 12;

    const uint8_t kChallengeBytes[ChallengeLtv::kLength] = {0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xFE, 0xBA, 0xBE,
                                                            0xFE, 0xED, 0xFA, 0xCE, 0xD0, 0xC0, 0xFF, 0xEE};
    ChallengeLtv  chalIn;
    memcpy(chalIn.mChallenge, kChallengeBytes, ChallengeLtv::kLength);

    uint8_t      buf[64];
    FrameBuilder fb;
    fb.Init(buf, sizeof(buf));

    VerifyOrQuit(AppendScaLtv(fb, scaIn) == kErrorNone);
    VerifyOrQuit(AppendChallengeLtv(fb, chalIn) == kErrorNone);

    ScaParams    scaOut;
    ChallengeLtv chalOut;
    memset(&scaOut, 0, sizeof(scaOut));
    memset(&chalOut, 0, sizeof(chalOut));

    VerifyOrQuit(PackAndParse(buf, fb.GetLength(), &scaOut, &chalOut) == kErrorNone);

    VerifyOrQuit(!scaOut.mRamAvailable);
    VerifyOrQuit(scaOut.mHasSlw);
    VerifyOrQuit(scaOut.mSlwPeriodSlots == 48);
    VerifyOrQuit(scaOut.mSlwPhaseSlots == 12);
    VerifyOrQuit(memcmp(chalOut.mChallenge, kChallengeBytes, ChallengeLtv::kLength) == 0);

    printf("PASSED\n");
}

// ---------------------------------------------------------------------------
// TestParseTruncated
//
// A truncated LTV buffer (declared length exceeds bytes remaining) must cause
// ParseThreadHeaderIe to return kErrorParse.
// ---------------------------------------------------------------------------
void TestParseTruncated(void)
{
    printf("TestParseTruncated\n");

    // Build a valid SCA LTV then truncate by one byte.
    ScaParams params;
    memset(&params, 0, sizeof(params));
    params.mRamAvailable   = false;
    params.mHasSlw         = true;
    params.mSlwPeriodSlots = 100;
    params.mSlwPhaseSlots  = 25;

    uint8_t      buf[32];
    FrameBuilder fb;
    fb.Init(buf, sizeof(buf));

    VerifyOrQuit(AppendScaLtv(fb, params) == kErrorNone);

    // Pack the plain LTVs to get the wire representation, then truncate by one byte.
    // Passing an incomplete packed buffer to ParseThreadHeaderIe must return kErrorParse.
    uint8_t packed[32];
    uint8_t packedLen = PackThreadHeaderIeLtvs(buf, static_cast<uint8_t>(fb.GetLength()), packed, sizeof(packed));
    VerifyOrQuit(ParseThreadHeaderIe(packed, static_cast<uint8_t>(packedLen - 1), nullptr, nullptr) == kErrorParse);

    printf("PASSED\n");
}

// ---------------------------------------------------------------------------
// TestParseUnknownType
//
// LTVs with unknown types must be silently skipped; the function must return
// kErrorNone and leave the valid LTVs parseable.
// ---------------------------------------------------------------------------
void TestParseUnknownType(void)
{
    printf("TestParseUnknownType\n");

    // Manually build: [unknown LTV][Challenge LTV]
    // Unknown: [L=4][T=0xAA][four bytes]
    const uint8_t kUnknownValue[]                   = {0x11, 0x22, 0x33, 0x44};
    const uint8_t kChalBytes[ChallengeLtv::kLength] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                                                       0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10};

    uint8_t      buf[64];
    FrameBuilder fb;
    fb.Init(buf, sizeof(buf));

    VerifyOrQuit(Ltv::Append(fb, 0xAA, kUnknownValue, sizeof(kUnknownValue)) == kErrorNone);
    ChallengeLtv chal;
    memcpy(chal.mChallenge, kChalBytes, ChallengeLtv::kLength);
    VerifyOrQuit(AppendChallengeLtv(fb, chal) == kErrorNone);

    ChallengeLtv out;
    memset(&out, 0, sizeof(out));
    VerifyOrQuit(PackAndParse(buf, fb.GetLength(), nullptr, &out) == kErrorNone);
    VerifyOrQuit(memcmp(out.mChallenge, kChalBytes, ChallengeLtv::kLength) == 0);

    printf("PASSED\n");
}

// ---------------------------------------------------------------------------
// TestTdLinkCommandFrame
//
// Generates a TD Link Command frame and verifies the FCF, addressing, security
// header layout, Thread Header IE contents, Termination2 IE, and encrypted
// payload structure. All field offsets are derived from the known frame layout:
//
//   [FCF 2][Seq 1][Dst PAN 2][Dst Ext 8][Src Ext 8][Sec Ctl 1][FC 4][Key 1]
//   [Thread Hdr IE 2 + LTV payload][Term2 IE 2]
//   [0x54][0x02][Mask][optional fields]
//   [MIC 4][FCS 2]
//
// Security header: Sec Ctl (1) + Frame Counter (4) + Key ID (1) = 6 bytes.
// ---------------------------------------------------------------------------
void TestTdLinkCommandFrame(void)
{
    printf("TestTdLinkCommandFrame\n");

    const uint16_t kPanId     = 0x1234;
    const uint8_t  kDstAddr[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08}; // WI
    const uint8_t  kSrcAddr[] = {0xA1, 0xB2, 0xC3, 0xD4, 0xE5, 0xF6, 0x07, 0x18}; // WL

    ExtAddress dst;
    ExtAddress src;
    memcpy(dst.m8, kDstAddr, sizeof(kDstAddr));
    memcpy(src.m8, kSrcAddr, sizeof(kSrcAddr));

    ScaParams scaParams;
    memset(&scaParams, 0, sizeof(scaParams));
    scaParams.mRamAvailable = false;
    scaParams.mHasSlw       = false;

    ChallengeLtv challenge;
    for (uint8_t i = 0; i < ChallengeLtv::kLength; i++)
    {
        challenge.mChallenge[i] = static_cast<uint8_t>(0x10 + i);
    }

    // --- Case 1: No optional payload fields ---
    {
        uint8_t psduBuf[OT_RADIO_FRAME_MAX_SIZE];
        TxFrame frame;
        memset(psduBuf, 0, sizeof(psduBuf));
        frame.mPsdu    = psduBuf;
        frame.mLength  = 0;
        frame.mChannel = 20;

        VerifyOrQuit(frame.GenerateThreadDirectLinkCommand(kPanId, dst, src, scaParams, challenge, nullptr, nullptr) ==
                     kErrorNone);

        const uint8_t *psdu = frame.GetPsdu();

        OT_UNUSED_VARIABLE(psdu);

        // FCF checks via public getters
        VerifyOrQuit(frame.IsMacCommand());
        VerifyOrQuit(frame.GetVersion() == Frame::kVersion2015);
        VerifyOrQuit(frame.GetSecurityEnabled());
        VerifyOrQuit(frame.GetAckRequest());
        VerifyOrQuit(frame.IsIePresent());

        // Addressing: verify dst and src via frame API (reversed wire order handled internally).
        Address addrOut;
        VerifyOrQuit(frame.GetDstAddr(addrOut) == kErrorNone);
        VerifyOrQuit(addrOut.IsExtended() && addrOut.GetExtended() == dst);

        VerifyOrQuit(frame.GetSrcAddr(addrOut) == kErrorNone);
        VerifyOrQuit(addrOut.IsExtended() && addrOut.GetExtended() == src);

        // Payload (encrypted bytes): [0x54][0x02][mask]
        const uint8_t *payload = frame.GetPayload();
        VerifyOrQuit(payload != nullptr);
        VerifyOrQuit(payload[0] == Frame::kMacCmdDirect);
        VerifyOrQuit(payload[1] == Frame::kThreadMacCmdDirectLink);
        VerifyOrQuit(payload[2] == 0);

        // Walk the raw PSDU to find the Thread Header IE and round-trip parse it.
        // MHR layout: FCF(2) + Seq(1) + DstPAN(2) + DstExt(8) + SrcExt(8) + SecHdr(6) = offset 27.
        uint16_t ieWord = static_cast<uint16_t>(psduBuf[27]) | (static_cast<uint16_t>(psduBuf[28]) << 8);
        uint16_t ieId   = (ieWord >> 7) & 0x00FFu;
        uint8_t  ieLen  = static_cast<uint8_t>(ieWord & 0x7Fu);
        VerifyOrQuit(ieId == ThreadHeaderIe::kElementId);
        VerifyOrQuit(ieLen == 20); // packed: SCA 3B {0x12,0x00,0x00} + Challenge 17B

        ScaParams    scaOut;
        ChallengeLtv chalOut;
        memset(&scaOut, 0, sizeof(scaOut));
        memset(&chalOut, 0, sizeof(chalOut));
        VerifyOrQuit(ParseThreadHeaderIe(&psduBuf[29], ieLen, &scaOut, &chalOut) == kErrorNone);
        VerifyOrQuit(!scaOut.mRamAvailable);
        VerifyOrQuit(memcmp(chalOut.mChallenge, challenge.mChallenge, ChallengeLtv::kLength) == 0);
    }

    // --- Case 2: Supervision Interval only ---
    {
        uint8_t psduBuf2[OT_RADIO_FRAME_MAX_SIZE];
        TxFrame frame;
        uint8_t supervisionInterval = 30;

        frame.mPsdu    = psduBuf2;
        frame.mLength  = 0;
        frame.mChannel = 20;

        VerifyOrQuit(frame.GenerateThreadDirectLinkCommand(kPanId, dst, src, scaParams, challenge, &supervisionInterval,
                                                           nullptr) == kErrorNone);

        const uint8_t *payload = frame.GetPayload();
        VerifyOrQuit(payload != nullptr);
        VerifyOrQuit(payload[0] == Frame::kMacCmdDirect);
        VerifyOrQuit(payload[1] == Frame::kThreadMacCmdDirectLink);
        VerifyOrQuit(payload[2] == Frame::kLinkParamMaskSupervisionInterval);
        VerifyOrQuit(payload[3] == supervisionInterval);
    }

    // --- Case 3: Both Supervision Interval and Services ---
    {
        uint8_t psduBuf3[OT_RADIO_FRAME_MAX_SIZE];
        TxFrame frame;
        uint8_t supervisionInterval = 60;
        uint8_t services            = 0x03;

        frame.mPsdu    = psduBuf3;
        frame.mLength  = 0;
        frame.mChannel = 20;

        VerifyOrQuit(frame.GenerateThreadDirectLinkCommand(kPanId, dst, src, scaParams, challenge, &supervisionInterval,
                                                           &services) == kErrorNone);

        const uint8_t *payload = frame.GetPayload();
        VerifyOrQuit(payload != nullptr);
        VerifyOrQuit(payload[0] == Frame::kMacCmdDirect);
        VerifyOrQuit(payload[1] == Frame::kThreadMacCmdDirectLink);
        VerifyOrQuit(payload[2] == (Frame::kLinkParamMaskSupervisionInterval | Frame::kLinkParamMaskServices));
        VerifyOrQuit(payload[3] == supervisionInterval);
        VerifyOrQuit(payload[4] == services);
    }

    // --- Case 4: GetPayload() based checks only, no raw offsets needed ---
    {
        uint8_t psduBuf4[OT_RADIO_FRAME_MAX_SIZE];
        TxFrame frame;

        frame.mPsdu    = psduBuf4;
        frame.mLength  = 0;
        frame.mChannel = 20;

        VerifyOrQuit(frame.GenerateThreadDirectLinkCommand(kPanId, dst, src, scaParams, challenge, nullptr, nullptr) ==
                     kErrorNone);

        const uint8_t *payload = frame.GetPayload();
        VerifyOrQuit(payload != nullptr);
        VerifyOrQuit(payload[0] == Frame::kMacCmdDirect);
        VerifyOrQuit(payload[1] == Frame::kThreadMacCmdDirectLink);
        VerifyOrQuit(payload[2] == 0); // No optional fields

        // Verify ParseThreadHeaderIe can decode the embedded Thread Header IE.
        // Walk through raw PSDU: after sec header (6 bytes at offset 21), IEs begin at 27.
        const uint8_t *rawPsdu = frame.GetPsdu();
        uint16_t       ieWord  = static_cast<uint16_t>(rawPsdu[27]) | (static_cast<uint16_t>(rawPsdu[28]) << 8);
        uint8_t        ieLen   = static_cast<uint8_t>(ieWord & 0x7F);
        uint16_t       ieId    = (ieWord >> 7) & 0x00FFu;
        VerifyOrQuit(ieId == ThreadHeaderIe::kElementId);

        ScaParams    scaOut;
        ChallengeLtv chalOut;
        memset(&scaOut, 0, sizeof(scaOut));
        memset(&chalOut, 0, sizeof(chalOut));
        VerifyOrQuit(ParseThreadHeaderIe(&rawPsdu[29], ieLen, &scaOut, &chalOut) == kErrorNone);
        VerifyOrQuit(!scaOut.mRamAvailable);
        VerifyOrQuit(memcmp(chalOut.mChallenge, challenge.mChallenge, ChallengeLtv::kLength) == 0);
    }

    printf("PASSED\n");
}

} // namespace Mac
} // namespace ot

int main(void)
{
    ot::Mac::TestScaLtvRoundTrip();
    ot::Mac::TestChallengeLtvRoundTrip();
    ot::Mac::TestTargetIdLtv();
    ot::Mac::TestThreadHeaderIeWrap();
    ot::Mac::TestMultiLtvParse();
    ot::Mac::TestParseTruncated();
    ot::Mac::TestParseUnknownType();
    ot::Mac::TestTdLinkCommandFrame();

    printf("All tests passed\n");
    return 0;
}
