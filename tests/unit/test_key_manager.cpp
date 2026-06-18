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

#include "common/code_utils.hpp"
#include "common/debug.hpp"
#include "thread/key_manager.hpp"

#include "test_platform.h"
#include "test_util.hpp"

namespace ot {

#if OPENTHREAD_CONFIG_THREAD_DIRECT_WAKE_INITIATOR_ENABLE || OPENTHREAD_CONFIG_THREAD_DIRECT_WAKE_LISTENER_ENABLE

void TestWakeKeyDerivation(void)
{
    // `KeyManager::GetDefaultWakeKey()` matches the first 16 bytes of
    // HMAC-SHA256 over the literal "Thread-Wake" using the Thread Network Key
    // as the HMAC key (see `KeyManager::GetDefaultWakeKey()`).
    //
    // Test vector (network key is non-zero on purpose):
    //   Network Key : 00 01 02 03 04 05 06 07 08 09 0a 0b 0c 0d 0e 0f
    //   Wake Key    : f5 ca 7d 94 57 48 c4 55 e8 ad 69 6a 76 fb 18 d0
    //
    // Recompute with:
    //   python3 -c "import
    //   hmac,hashlib;k=bytes(range(16));print(hmac.new(k,b'Thread-Wake',hashlib.sha256).digest()[:16].hex())"

    constexpr uint8_t kNetworkKeyBytes[OT_NETWORK_KEY_SIZE] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
    };

    constexpr uint8_t kExpectedWakeKey[Mac::Key::kSize] = {
        0xf5, 0xca, 0x7d, 0x94, 0x57, 0x48, 0xc4, 0x55, 0xe8, 0xad, 0x69, 0x6a, 0x76, 0xfb, 0x18, 0xd0,
    };

    Instance  *instance;
    NetworkKey networkKey;
    Mac::Key   derivedKey;

    instance = testInitInstance();
    VerifyOrQuit(instance != nullptr);

    memcpy(networkKey.m8, kNetworkKeyBytes, sizeof(networkKey.m8));
    instance->Get<KeyManager>().SetNetworkKey(networkKey);

    const Mac::KeyMaterial &wakeMaterial = instance->Get<KeyManager>().GetDefaultWakeKey();

    wakeMaterial.ExtractKey(derivedKey);
    VerifyOrQuit(memcmp(derivedKey.GetBytes(), kExpectedWakeKey, Mac::Key::kSize) == 0, "Wake key derivation mismatch");

    testFreeInstance(instance);
}

#endif // OPENTHREAD_CONFIG_THREAD_DIRECT_WAKE_INITIATOR_ENABLE || OPENTHREAD_CONFIG_THREAD_DIRECT_WAKE_LISTENER_ENABLE

} // namespace ot

int main(void)
{
#if OPENTHREAD_CONFIG_THREAD_DIRECT_WAKE_INITIATOR_ENABLE || OPENTHREAD_CONFIG_THREAD_DIRECT_WAKE_LISTENER_ENABLE
    ot::TestWakeKeyDerivation();
#endif
    printf("All tests passed\n");
    return 0;
}
