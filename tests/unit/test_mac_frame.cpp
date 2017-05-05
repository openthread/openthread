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

#include "utils/wrap_string.h"

#include <openthread/openthread.h>

#include "common/debug.hpp"
#include "mac/mac_frame.hpp"

#include "test_util.h"

namespace ot {

void TestMacHeader(void)
{
    static const struct
    {
        uint16_t fcf;
        uint8_t secCtl;
        uint8_t headerLength;
    } tests[] =
    {
        { Mac::Frame::kFcfDstAddrNone | Mac::Frame::kFcfSrcAddrNone, 0, 3 },
        { Mac::Frame::kFcfDstAddrNone | Mac::Frame::kFcfSrcAddrShort, 0, 7 },
        { Mac::Frame::kFcfDstAddrNone | Mac::Frame::kFcfSrcAddrExt, 0, 13 },
        { Mac::Frame::kFcfDstAddrShort | Mac::Frame::kFcfSrcAddrNone, 0, 7 },
        { Mac::Frame::kFcfDstAddrExt | Mac::Frame::kFcfSrcAddrNone, 0, 13 },
        { Mac::Frame::kFcfDstAddrShort | Mac::Frame::kFcfSrcAddrShort, 0, 11 },
        { Mac::Frame::kFcfDstAddrShort | Mac::Frame::kFcfSrcAddrExt, 0, 17 },
        { Mac::Frame::kFcfDstAddrExt | Mac::Frame::kFcfSrcAddrShort, 0, 17 },
        { Mac::Frame::kFcfDstAddrExt | Mac::Frame::kFcfSrcAddrExt, 0, 23 },

        { Mac::Frame::kFcfDstAddrShort | Mac::Frame::kFcfSrcAddrShort | Mac::Frame::kFcfPanidCompression, 0, 9 },
        { Mac::Frame::kFcfDstAddrShort | Mac::Frame::kFcfSrcAddrExt | Mac::Frame::kFcfPanidCompression, 0, 15 },
        { Mac::Frame::kFcfDstAddrExt | Mac::Frame::kFcfSrcAddrShort | Mac::Frame::kFcfPanidCompression, 0, 15 },
        { Mac::Frame::kFcfDstAddrExt | Mac::Frame::kFcfSrcAddrExt | Mac::Frame::kFcfPanidCompression, 0, 21 },

        {
            Mac::Frame::kFcfDstAddrShort | Mac::Frame::kFcfSrcAddrShort | Mac::Frame::kFcfPanidCompression |
            Mac::Frame::kFcfSecurityEnabled, Mac::Frame::kSecMic32 | Mac::Frame::kKeyIdMode1, 15
        },
        {
            Mac::Frame::kFcfDstAddrShort | Mac::Frame::kFcfSrcAddrShort | Mac::Frame::kFcfPanidCompression |
            Mac::Frame::kFcfSecurityEnabled, Mac::Frame::kSecMic32 | Mac::Frame::kKeyIdMode2, 19
        },
    };

    for (unsigned i = 0; i < sizeof(tests) / sizeof(tests[0]); i++)
    {
        uint8_t psdu[Mac::Frame::kMTU];
        Mac::Frame frame;

        frame.mPsdu = psdu;

        frame.InitMacHeader(tests[i].fcf, tests[i].secCtl);
        printf("%d\n", frame.GetHeaderLength());
        VerifyOrQuit(frame.GetHeaderLength() == tests[i].headerLength,
                     "MacHeader test failed\n");
    }
}

}  // namespace ot

#ifdef ENABLE_TEST_MAIN
int main(void)
{
    ot::TestMacHeader();
    printf("All tests passed\n");
    return 0;
}
#endif
