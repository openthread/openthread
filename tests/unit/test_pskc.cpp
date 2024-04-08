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
#include <openthread/config.h>

#include "meshcop/commissioner.hpp"
#include "meshcop/meshcop.hpp"

#include "test_platform.h"
#include "test_util.h"

namespace ot {
namespace MeshCoP {

#if OPENTHREAD_FTD

void TestMinimumPassphrase(void)
{
    static const otExtendedPanId kExtPanId     = {{0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08}};
    static const otNetworkName   kNetworkName  = {{'O', 'p', 'e', 'n', 'T', 'h', 'r', 'e', 'a', 'd', '\0'}};
    static const char            kPassphrase[] = "123456";

    static const otPskc kExpectedPskc = {
        {0x44, 0x98, 0x8e, 0x22, 0xcf, 0x65, 0x2e, 0xee, 0xcc, 0xd1, 0xe4, 0xc0, 0x1d, 0x01, 0x54, 0xf8}};

    Instance *instance = testInitInstance();
    Pskc      pskc;

    SuccessOrQuit(GeneratePskc(kPassphrase, AsCoreType(&kNetworkName), AsCoreType(&kExtPanId), pskc));
    VerifyOrQuit(pskc == AsCoreType(&kExpectedPskc));

    testFreeInstance(instance);
}

void TestMaximumPassphrase(void)
{
    static const otExtendedPanId kExtPanId    = {{0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08}};
    static const otNetworkName   kNetworkName = {{'O', 'p', 'e', 'n', 'T', 'h', 'r', 'e', 'a', 'd', '\0'}};

    static const char kPassphrase[] = "1234567812345678"
                                      "1234567812345678"
                                      "1234567812345678"
                                      "1234567812345678"
                                      "1234567812345678"
                                      "1234567812345678"
                                      "1234567812345678"
                                      "1234567812345678"
                                      "1234567812345678"
                                      "1234567812345678"
                                      "1234567812345678"
                                      "1234567812345678"
                                      "1234567812345678"
                                      "1234567812345678"
                                      "1234567812345678"
                                      "123456781234567";

    static const otPskc kExpectedPskc = {
        {0x9e, 0x81, 0xbd, 0x35, 0xa2, 0x53, 0x76, 0x2f, 0x80, 0xee, 0x04, 0xff, 0x2f, 0xa2, 0x85, 0xe9}};

    Instance *instance = testInitInstance();
    Pskc      pskc;

    SuccessOrQuit(GeneratePskc(kPassphrase, AsCoreType(&kNetworkName), AsCoreType(&kExtPanId), pskc));
    VerifyOrQuit(pskc == AsCoreType(&kExpectedPskc));

    testFreeInstance(instance);
}

void TestExampleInSpec(void)
{
    static const otExtendedPanId kExtPanId     = {{0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07}};
    static const otNetworkName   kNetworkName  = {{'T', 'e', 's', 't', ' ', 'N', 'e', 't', 'w', 'o', 'r', 'k', '\0'}};
    static const char            kPassphrase[] = "12SECRETPASSWORD34";

    static const otPskc kExpectedPskc = {
        {0xc3, 0xf5, 0x93, 0x68, 0x44, 0x5a, 0x1b, 0x61, 0x06, 0xbe, 0x42, 0x0a, 0x70, 0x6d, 0x4c, 0xc9}};

    Instance *instance = testInitInstance();
    Pskc      pskc;

    SuccessOrQuit(GeneratePskc(kPassphrase, AsCoreType(&kNetworkName), AsCoreType(&kExtPanId), pskc));
    VerifyOrQuit(pskc == AsCoreType(&kExpectedPskc));

    testFreeInstance(instance);
}

} // namespace MeshCoP
} // namespace ot

#endif // OPENTHREAD_FTD

int main(void)
{
#if OPENTHREAD_FTD
    ot::MeshCoP::TestMinimumPassphrase();
    ot::MeshCoP::TestMaximumPassphrase();
    ot::MeshCoP::TestExampleInSpec();
    printf("All tests passed\n");
#else
    printf("PSKc generation is not supported on non-ftd build\n");
#endif
    return 0;
}
