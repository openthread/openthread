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

#include <openthread/config.h>

#include "common/debug.hpp"
#include "crypto/hmac_sha256.hpp"
#include "utils/wrap_string.h"

#include "test_platform.h"
#include "test_util.h"

void TestHmacSha256(void)
{
    static const struct
    {
        const char *key;
        const char *data;
        uint8_t     hash[ot::Crypto::HmacSha256::kHashSize];
    } tests[] = {
        {
            "\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b",
            "Hi There",
            {
                0xb0, 0x34, 0x4c, 0x61, 0xd8, 0xdb, 0x38, 0x53, 0x5c, 0xa8, 0xaf, 0xce, 0xaf, 0x0b, 0xf1, 0x2b,
                0x88, 0x1d, 0xc2, 0x00, 0xc9, 0x83, 0x3d, 0xa7, 0x26, 0xe9, 0x37, 0x6c, 0x2e, 0x32, 0xcf, 0xf7,
            },
        },
        {
            NULL,
            NULL,
            {},
        },
    };

    otInstance *instance = testInitInstance();

    // Make sure hmac is destructed before freeing instance.
    {
        ot::Crypto::HmacSha256 hmac;
        uint8_t                hash[ot::Crypto::HmacSha256::kHashSize];

        VerifyOrQuit(instance != NULL, "Null OpenThread instance");

        for (int i = 0; tests[i].key != NULL; i++)
        {
            hmac.Start(reinterpret_cast<const uint8_t *>(tests[i].key), static_cast<uint16_t>(strlen(tests[i].key)));
            hmac.Update(reinterpret_cast<const uint8_t *>(tests[i].data), static_cast<uint16_t>(strlen(tests[i].data)));
            hmac.Finish(hash);

            VerifyOrQuit(memcmp(hash, tests[i].hash, sizeof(tests[i].hash)) == 0, "HMAC-SHA-256 failed\n");
        }
    }

    testFreeInstance(instance);
}

#ifdef ENABLE_TEST_MAIN
int main(void)
{
    TestHmacSha256();
    printf("All tests passed\n");
    return 0;
}
#endif
