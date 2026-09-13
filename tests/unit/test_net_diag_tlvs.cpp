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

#include "test_platform.h"

#include <string.h>

#include "test_util.h"
#include "common/clearable.hpp"
#include "thread/net_diag_tlvs.hpp"

namespace ot {
namespace NetDiag {

void TestSrpClientCountersTlvValue(void)
{
    // The full expected wire image for the counter values set below. Pinning every
    // byte (rather than spot-checking a few) is what catches a transposition of two
    // same-width fields applied consistently to both `InitFrom()` and `Read()`, which
    // a round-trip comparison cannot detect.
    static const uint8_t kExpectedWire[] = {
        0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, // mRegisteredTime
        0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, // mAnycastAvailableTime
        0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, // mUnicastAvailableTime
        0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, // mTrackedTime
        0x41, 0x42, 0x43, 0x44,                         // mTxUpdates
        0x45, 0x46, 0x47, 0x48,                         // mUpdateAttempts
        0x49, 0x4a, 0x4b, 0x4c,                         // mSuccess
        0x4d, 0x4e, 0x4f, 0x50,                         // mRejectedDuplicate
        0x51, 0x52, 0x53, 0x54,                         // mRejectedSecurity
        0x55, 0x56, 0x57, 0x58,                         // mRejectedOther
        0x59, 0x5a, 0x5b, 0x5c,                         // mTimeouts
        0x5d, 0x5e, 0x5f, 0x60,                         // mHostAddressChanges
        0x61, 0x62, 0x63, 0x64,                         // mServerChanges
        0x65, 0x66, 0x67, 0x68,                         // mServiceAdds
        0x69, 0x6a, 0x6b, 0x6c,                         // mServiceRemoves
        0x6d, 0x6e, 0x6f, 0x70,                         // mServiceClears
        0x71, 0x72, 0x73, 0x74,                         // mHostAndServicesRemoves
        0x75, 0x76, 0x77, 0x78,                         // mHostAndServicesClears
        0x79, 0x7a, 0x7b, 0x7c,                         // mTxTotalBytes
    };

    static_assert(sizeof(kExpectedWire) == 92, "kExpectedWire does not match the 92-byte TLV value");

    otSrpClientCounters       counters;
    SrpClientCountersTlvValue tlvValue;
    SrpClientCounters         readBack;
    const uint8_t            *bytes = reinterpret_cast<const uint8_t *>(&tlvValue);

    printf("TestSrpClientCountersTlvValue\n");

    ClearAllBytes(counters);

    // Distinct, non-palindromic values so a swapped field pair or a wrong
    // HostSwap width is detectable byte-for-byte.
    counters.mRegisteredTime         = 0x0102030405060708ULL;
    counters.mAnycastAvailableTime   = 0x1112131415161718ULL;
    counters.mUnicastAvailableTime   = 0x2122232425262728ULL;
    counters.mTrackedTime            = 0x3132333435363738ULL;
    counters.mTxUpdates              = 0x41424344;
    counters.mUpdateAttempts         = 0x45464748;
    counters.mSuccess                = 0x494a4b4c;
    counters.mRejectedDuplicate      = 0x4d4e4f50;
    counters.mRejectedSecurity       = 0x51525354;
    counters.mRejectedOther          = 0x55565758;
    counters.mTimeouts               = 0x595a5b5c;
    counters.mHostAddressChanges     = 0x5d5e5f60;
    counters.mServerChanges          = 0x61626364;
    counters.mServiceAdds            = 0x65666768;
    counters.mServiceRemoves         = 0x696a6b6c;
    counters.mServiceClears          = 0x6d6e6f70;
    counters.mHostAndServicesRemoves = 0x71727374;
    counters.mHostAndServicesClears  = 0x75767778;
    counters.mTxTotalBytes           = 0x797a7b7c;

    tlvValue.InitFrom(counters);

    VerifyOrQuit(memcmp(bytes, kExpectedWire, sizeof(kExpectedWire)) == 0);

    ClearAllBytes(readBack);
    tlvValue.Read(readBack);

    VerifyOrQuit(readBack.mRegisteredTime == counters.mRegisteredTime);
    VerifyOrQuit(readBack.mAnycastAvailableTime == counters.mAnycastAvailableTime);
    VerifyOrQuit(readBack.mUnicastAvailableTime == counters.mUnicastAvailableTime);
    VerifyOrQuit(readBack.mTrackedTime == counters.mTrackedTime);
    VerifyOrQuit(readBack.mTxUpdates == counters.mTxUpdates);
    VerifyOrQuit(readBack.mUpdateAttempts == counters.mUpdateAttempts);
    VerifyOrQuit(readBack.mSuccess == counters.mSuccess);
    VerifyOrQuit(readBack.mRejectedDuplicate == counters.mRejectedDuplicate);
    VerifyOrQuit(readBack.mRejectedSecurity == counters.mRejectedSecurity);
    VerifyOrQuit(readBack.mRejectedOther == counters.mRejectedOther);
    VerifyOrQuit(readBack.mTimeouts == counters.mTimeouts);
    VerifyOrQuit(readBack.mHostAddressChanges == counters.mHostAddressChanges);
    VerifyOrQuit(readBack.mServerChanges == counters.mServerChanges);
    VerifyOrQuit(readBack.mServiceAdds == counters.mServiceAdds);
    VerifyOrQuit(readBack.mServiceRemoves == counters.mServiceRemoves);
    VerifyOrQuit(readBack.mServiceClears == counters.mServiceClears);
    VerifyOrQuit(readBack.mHostAndServicesRemoves == counters.mHostAndServicesRemoves);
    VerifyOrQuit(readBack.mHostAndServicesClears == counters.mHostAndServicesClears);
    VerifyOrQuit(readBack.mTxTotalBytes == counters.mTxTotalBytes);

    printf(" -- PASS\n");
}

} // namespace NetDiag
} // namespace ot

int main(void)
{
    ot::NetDiag::TestSrpClientCountersTlvValue();
    printf("\nAll tests passed.\n");
    return 0;
}
