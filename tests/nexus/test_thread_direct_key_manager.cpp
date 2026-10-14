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

#include <stdio.h>
#include <string.h>

#include <openthread/thread_direct.h>
#include <openthread/platform/settings.h>

#include "platform/nexus_core.hpp"
#include "platform/nexus_node.hpp"

namespace ot {
namespace Nexus {

using Key                 = ThreadDirect::Key;
using KeyId               = ThreadDirect::KeyId;
using KeyIterator         = ThreadDirect::KeyIterator;
using KeyManager          = ThreadDirect::KeyManager;
using KeyMaterial         = ThreadDirect::KeyMaterial;
using ThreadDirectKeyInfo = ot::Settings::ThreadDirectKeyInfo;

static void MakeKeyMaterial(otThreadDirectKeyMaterial &aKeyMaterial, uint8_t aSeed)
{
    ClearAllBytes(aKeyMaterial);

    for (uint16_t i = 0; i < OT_THREAD_DIRECT_KEY_SIZE; i++)
    {
        aKeyMaterial.mKeyMaterial.mKey.m8[i] = static_cast<uint8_t>(aSeed + i);
    }
}

static void MakeKeyId(otThreadDirectKeyId &aKeyId, uint8_t aSeed)
{
    for (uint16_t i = 0; i < OT_THREAD_DIRECT_KEY_ID_SIZE; i++)
    {
        aKeyId.m8[i] = static_cast<uint8_t>((aSeed << 4) + i + 1);
    }
}

static uint16_t CountKeysViaIterator(otInstance *aInstance)
{
    otThreadDirectKeyIterator iter;
    uint16_t                  count = 0;

    otThreadDirectInitKeyIterator(aInstance, &iter);

    while (otThreadDirectGetNextKey(&iter) != nullptr)
    {
        count++;
    }

    return count;
}

static void VerifyKeyProperties(const otThreadDirectKey         *aKey,
                                const otThreadDirectKeyMaterial &aExpectedMaterial,
                                const otThreadDirectKeyId       &aExpectedId,
                                uint32_t                         aExpectedFrameCounter)
{
    const otThreadDirectKeyMaterial *material;
    const otThreadDirectKeyId       *id;

    VerifyOrQuit(aKey != nullptr);

    material = otThreadDirectKeyGetMaterial(aKey);
    VerifyOrQuit(material != nullptr);
    VerifyOrQuit(AsCoreType(material) == AsCoreType(&aExpectedMaterial));

    id = otThreadDirectKeyGetId(aKey);
    VerifyOrQuit(id != nullptr);
    VerifyOrQuit(AsCoreType(id) == AsCoreType(&aExpectedId));
    VerifyOrQuit(AsCoreType(aKey).HasId(AsCoreType(&aExpectedId)));

    VerifyOrQuit(otThreadDirectKeyGetFrameCounter(aKey) == aExpectedFrameCounter);
}

void TestThreadDirectKeyManager(void)
{
    static constexpr uint16_t kMaxKeys            = OPENTHREAD_CONFIG_TD_MAX_REGISTERED_KEYS;
    static constexpr uint32_t kStoredCounterAhead = OPENTHREAD_CONFIG_TD_FRAME_COUNTER_AHEAD;

    Core                      nexus;
    Node                     &node0 = nexus.CreateNode();
    otInstance               *instance;
    otThreadDirectKeyMaterial materials[kMaxKeys + 1];
    otThreadDirectKeyId       ids[kMaxKeys + 1];
    const otThreadDirectKey  *keys[kMaxKeys + 1];
    const otThreadDirectKey  *key;
    otThreadDirectKeyIterator iter;

    Log("------------------------------------------------------------------------------------------------------");
    Log("TestThreadDirectKeyManager");

    nexus.AdvanceTime(0);
    instance = &node0.GetInstance();

    SuccessOrQuit(node0.SetLogLevel(kLogLevelInfo));

    for (uint16_t i = 0; i <= kMaxKeys; i++)
    {
        MakeKeyMaterial(materials[i], static_cast<uint8_t>(i + 1));
        MakeKeyId(ids[i], static_cast<uint8_t>(i + 1));
        keys[i] = nullptr;
    }

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Check initial state (empty key list)");

    VerifyOrQuit(node0.Get<KeyManager>().GetKeyList().IsEmpty());
    VerifyOrQuit(CountKeysViaIterator(instance) == 0);

    otThreadDirectInitKeyIterator(instance, &iter);
    VerifyOrQuit(otThreadDirectGetNextKey(&iter) == nullptr);
    VerifyOrQuit(otThreadDirectGetNextKey(&iter) == nullptr);

    VerifyOrQuit(otThreadDirectFindKey(instance, &materials[0], &ids[0]) == nullptr);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Register keys, including keys sharing the same material or same Key ID");

    // Key 0: (materials[0], ids[0])
    keys[0] = otThreadDirectRegisterKey(instance, &materials[0], &ids[0]);
    VerifyKeyProperties(keys[0], materials[0], ids[0], 0);
    VerifyOrQuit(CountKeysViaIterator(instance) == 1);

    // Key 1: (materials[1], ids[1])
    keys[1] = otThreadDirectRegisterKey(instance, &materials[1], &ids[1]);
    VerifyKeyProperties(keys[1], materials[1], ids[1], 0);
    VerifyOrQuit(keys[1] != keys[0]);
    VerifyOrQuit(CountKeysViaIterator(instance) == 2);

    // Key 2: same key material as Key 0, but different Key ID (materials[0], ids[2])
    keys[2] = otThreadDirectRegisterKey(instance, &materials[0], &ids[2]);
    VerifyKeyProperties(keys[2], materials[0], ids[2], 0);
    VerifyOrQuit(keys[2] != keys[0]);
    VerifyOrQuit(!AsCoreType(keys[2]).HasId(AsCoreType(&ids[0])));
    VerifyOrQuit(CountKeysViaIterator(instance) == 3);

    // Key 3: different key material, but same Key ID as Key 0 (materials[3], ids[0])
    keys[3] = otThreadDirectRegisterKey(instance, &materials[3], &ids[0]);
    VerifyKeyProperties(keys[3], materials[3], ids[0], 0);
    VerifyOrQuit(keys[3] != keys[0]);
    VerifyOrQuit(CountKeysViaIterator(instance) == 4);

    // Verify `otThreadDirectFindKey()` matches both key material and Key ID
    VerifyOrQuit(otThreadDirectFindKey(instance, &materials[0], &ids[0]) == keys[0]);
    VerifyOrQuit(otThreadDirectFindKey(instance, &materials[1], &ids[1]) == keys[1]);
    VerifyOrQuit(otThreadDirectFindKey(instance, &materials[0], &ids[2]) == keys[2]);
    VerifyOrQuit(otThreadDirectFindKey(instance, &materials[3], &ids[0]) == keys[3]);

    // Mismatched combinations must return `nullptr`
    VerifyOrQuit(otThreadDirectFindKey(instance, &materials[0], &ids[1]) == nullptr);
    VerifyOrQuit(otThreadDirectFindKey(instance, &materials[1], &ids[0]) == nullptr);
    VerifyOrQuit(otThreadDirectFindKey(instance, &materials[3], &ids[2]) == nullptr);
    VerifyOrQuit(otThreadDirectFindKey(instance, &materials[2], &ids[2]) == nullptr);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Check re-registering an already registered key returns the existing entry");

    AsNonConst(AsCoreType(keys[0])).IncrementFrameCounter();
    VerifyOrQuit(otThreadDirectKeyGetFrameCounter(keys[0]) == 1);

    key = otThreadDirectRegisterKey(instance, &materials[0], &ids[0]);
    VerifyOrQuit(key == keys[0]);
    VerifyOrQuit(otThreadDirectKeyGetFrameCounter(key) == 1);
    VerifyOrQuit(CountKeysViaIterator(instance) == 4);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Check iterator traversal and re-initialization");

    {
        bool found[4] = {false, false, false, false};

        otThreadDirectInitKeyIterator(instance, &iter);

        while ((key = otThreadDirectGetNextKey(&iter)) != nullptr)
        {
            bool matched = false;

            for (uint16_t i = 0; i < 4; i++)
            {
                if (key == keys[i])
                {
                    VerifyOrQuit(!found[i]);
                    found[i] = true;
                    matched  = true;
                    break;
                }
            }

            VerifyOrQuit(matched);
        }

        for (bool isFound : found)
        {
            VerifyOrQuit(isFound);
        }

        // Re-initialize iterator and verify it starts from the head again
        otThreadDirectInitKeyIterator(instance, &iter);
        VerifyOrQuit(otThreadDirectGetNextKey(&iter) != nullptr);
    }

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Register up to kMaxKeys (%u) and verify limit enforcement", kMaxKeys);

    for (uint16_t i = 4; i < kMaxKeys; i++)
    {
        keys[i] = otThreadDirectRegisterKey(instance, &materials[i], &ids[i]);
        VerifyKeyProperties(keys[i], materials[i], ids[i], 0);
    }

    VerifyOrQuit(CountKeysViaIterator(instance) == kMaxKeys);

    // Attempting to register one more key beyond `kMaxKeys` must fail and return `nullptr`
    VerifyOrQuit(otThreadDirectRegisterKey(instance, &materials[kMaxKeys], &ids[kMaxKeys]) == nullptr);
    VerifyOrQuit(otThreadDirectFindKey(instance, &materials[kMaxKeys], &ids[kMaxKeys]) == nullptr);
    VerifyOrQuit(CountKeysViaIterator(instance) == kMaxKeys);

    // Re-registering an existing key when at `kMaxKeys` must still succeed and return the existing entry
    VerifyOrQuit(otThreadDirectRegisterKey(instance, &materials[2], &ids[2]) == nullptr);
    VerifyOrQuit(otThreadDirectRegisterKey(instance, &materials[0], &ids[2]) == keys[2]);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Unregister a key to free a slot and register a new key");

    otThreadDirectUnregisterKey(instance, keys[2]);
    keys[2] = nullptr;

    VerifyOrQuit(otThreadDirectFindKey(instance, &materials[0], &ids[2]) == nullptr);
    VerifyOrQuit(CountKeysViaIterator(instance) == kMaxKeys - 1);

    // Registering a new key now succeeds
    keys[2] = otThreadDirectRegisterKey(instance, &materials[2], &ids[2]);
    VerifyKeyProperties(keys[2], materials[2], ids[2], 0);
    VerifyOrQuit(CountKeysViaIterator(instance) == kMaxKeys);

    // Unregister keys[3..kMaxKeys-1] to leave 4 distinct keys (keys[0], keys[1], keys[2], keys[4]) for reset tests
    otThreadDirectUnregisterKey(instance, keys[3]);
    keys[3] = nullptr;

    for (uint16_t i = 5; i < kMaxKeys; i++)
    {
        otThreadDirectUnregisterKey(instance, keys[i]);
        keys[i] = nullptr;
    }

    VerifyOrQuit(CountKeysViaIterator(instance) == 4);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Advance frame counters on keys and verify persistence across reset");

    // Reset keys[0] by unregistering and re-registering so it starts cleanly at frame counter 0
    otThreadDirectUnregisterKey(instance, keys[0]);
    keys[0] = otThreadDirectRegisterKey(instance, &materials[0], &ids[0]);
    VerifyKeyProperties(keys[0], materials[0], ids[0], 0);

    // Key 0: frame counter stays at 0 (saved counter in settings is kStoredCounterAhead = 1000)

    // Key 1: increment frame counter 500 times (< kStoredCounterAhead, so saved counter remains 1000)
    for (uint32_t i = 0; i < kStoredCounterAhead / 2; i++)
    {
        AsNonConst(AsCoreType(keys[1])).IncrementFrameCounter();
    }
    VerifyOrQuit(otThreadDirectKeyGetFrameCounter(keys[1]) == kStoredCounterAhead / 2);

    // Key 2: increment frame counter to kStoredCounterAhead (1000), let store task run, then add 25 more (1025).
    // Saved counter in settings should be updated to 2 * kStoredCounterAhead (2000).
    for (uint32_t i = 0; i < kStoredCounterAhead; i++)
    {
        AsNonConst(AsCoreType(keys[2])).IncrementFrameCounter();
    }
    nexus.AdvanceTime(0);

    for (uint32_t i = 0; i < 25; i++)
    {
        AsNonConst(AsCoreType(keys[2])).IncrementFrameCounter();
    }
    VerifyOrQuit(otThreadDirectKeyGetFrameCounter(keys[2]) == kStoredCounterAhead + 25);

    // Key 4: increment frame counter 2500 times (processing tasklets along the way).
    // Saved counter in settings should advance to 2000 at 1000, and to 3000 at 2000.
    for (uint32_t i = 0; i < 2 * kStoredCounterAhead + 500; i++)
    {
        AsNonConst(AsCoreType(keys[4])).IncrementFrameCounter();
        nexus.AdvanceTime(0);
    }
    VerifyOrQuit(otThreadDirectKeyGetFrameCounter(keys[4]) == 2 * kStoredCounterAhead + 500);

    // Reset node0 and verify all 4 keys and their stored frame counters are restored from settings
    node0.Reset();
    nexus.AdvanceTime(0);
    instance = &node0.GetInstance();

    VerifyOrQuit(CountKeysViaIterator(instance) == 4);

    keys[0] = otThreadDirectFindKey(instance, &materials[0], &ids[0]);
    keys[1] = otThreadDirectFindKey(instance, &materials[1], &ids[1]);
    keys[2] = otThreadDirectFindKey(instance, &materials[2], &ids[2]);
    keys[4] = otThreadDirectFindKey(instance, &materials[4], &ids[4]);

    VerifyKeyProperties(keys[0], materials[0], ids[0], kStoredCounterAhead);
    VerifyKeyProperties(keys[1], materials[1], ids[1], kStoredCounterAhead);
    VerifyKeyProperties(keys[2], materials[2], ids[2], 2 * kStoredCounterAhead);
    VerifyKeyProperties(keys[4], materials[4], ids[4], 3 * kStoredCounterAhead);

    // Verify previously unregistered keys were not restored
    VerifyOrQuit(otThreadDirectFindKey(instance, &materials[3], &ids[0]) == nullptr);
    VerifyOrQuit(otThreadDirectFindKey(instance, &materials[5], &ids[5]) == nullptr);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Check first frame counter increment after reset immediately triggers a settings update");

    // After `RestoreKeys()`, `mSavedFrameCounter == mFrameCounter` (1000 for keys[0]).
    // Incrementing `keys[0]` once makes `mFrameCounter == 1001 >= mSavedFrameCounter`, posting `mStoreTask`.
    // When `mStoreTask` runs, `SaveThreadDirectKey()` saves `1001 + kStoredCounterAhead` (2001).
    AsNonConst(AsCoreType(keys[0])).IncrementFrameCounter();
    VerifyOrQuit(otThreadDirectKeyGetFrameCounter(keys[0]) == kStoredCounterAhead + 1);
    nexus.AdvanceTime(0);

    node0.Reset();
    nexus.AdvanceTime(0);
    instance = &node0.GetInstance();

    keys[0] = otThreadDirectFindKey(instance, &materials[0], &ids[0]);
    VerifyKeyProperties(keys[0], materials[0], ids[0], 2 * kStoredCounterAhead + 1);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Check RestoreKeys() deduplicates multiple settings entries for the same key and keeps highest counter");

    {
        ThreadDirectKeyInfo dupKeyInfo;

        dupKeyInfo.Init();
        dupKeyInfo.SetKey(AsCoreType(keys[0]).GetKeyMaterial().GetKey());
        dupKeyInfo.SetKeyId(AsCoreType(&ids[0]));

        // Inject a duplicate entry with a higher frame counter (50000) and one with a lower frame counter (500)
        dupKeyInfo.SetFrameCounter(50000);
        SuccessOrQuit(otPlatSettingsAdd(instance, ThreadDirectKeyInfo::kKey,
                                        reinterpret_cast<const uint8_t *>(&dupKeyInfo), sizeof(ThreadDirectKeyInfo)));

        dupKeyInfo.SetFrameCounter(500);
        SuccessOrQuit(otPlatSettingsAdd(instance, ThreadDirectKeyInfo::kKey,
                                        reinterpret_cast<const uint8_t *>(&dupKeyInfo), sizeof(ThreadDirectKeyInfo)));
    }

    node0.Reset();
    nexus.AdvanceTime(0);
    instance = &node0.GetInstance();

    // Must still have only 4 unique keys, and keys[0] must have the highest frame counter (50000)
    VerifyOrQuit(CountKeysViaIterator(instance) == 4);
    keys[0] = otThreadDirectFindKey(instance, &materials[0], &ids[0]);
    VerifyKeyProperties(keys[0], materials[0], ids[0], 50000);

    // Incrementing keys[0] triggers `SaveThreadDirectKey()`, which removes all stale duplicate entries in settings
    AsNonConst(AsCoreType(keys[0])).IncrementFrameCounter();
    nexus.AdvanceTime(0);

    node0.Reset();
    nexus.AdvanceTime(0);
    instance = &node0.GetInstance();

    VerifyOrQuit(CountKeysViaIterator(instance) == 4);
    keys[0] = otThreadDirectFindKey(instance, &materials[0], &ids[0]);
    VerifyKeyProperties(keys[0], materials[0], ids[0], 50001 + kStoredCounterAhead);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Check otThreadDirectUnregisterAllKeys() removes all keys from memory and persistent settings");

    otThreadDirectUnregisterAllKeys(instance);
    VerifyOrQuit(CountKeysViaIterator(instance) == 0);
    VerifyOrQuit(node0.Get<KeyManager>().GetKeyList().IsEmpty());

    node0.Reset();
    nexus.AdvanceTime(0);
    instance = &node0.GetInstance();

    VerifyOrQuit(CountKeysViaIterator(instance) == 0);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Check otInstanceErasePersistentInfo() unregisters all keys and wipes settings");

    keys[0] = otThreadDirectRegisterKey(instance, &materials[0], &ids[0]);
    keys[1] = otThreadDirectRegisterKey(instance, &materials[1], &ids[1]);
    VerifyOrQuit(CountKeysViaIterator(instance) == 2);

    SuccessOrQuit(otInstanceErasePersistentInfo(instance));
    VerifyOrQuit(CountKeysViaIterator(instance) == 0);

    node0.Reset();
    nexus.AdvanceTime(0);
    instance = &node0.GetInstance();

    VerifyOrQuit(CountKeysViaIterator(instance) == 0);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Check otInstanceFactoryReset() unregisters all keys and wipes settings");

    keys[0] = otThreadDirectRegisterKey(instance, &materials[0], &ids[0]);
    keys[1] = otThreadDirectRegisterKey(instance, &materials[1], &ids[1]);
    VerifyOrQuit(CountKeysViaIterator(instance) == 2);

    otInstanceFactoryReset(instance);
    VerifyOrQuit(CountKeysViaIterator(instance) == 0);

    node0.Reset();
    nexus.AdvanceTime(0);
    instance = &node0.GetInstance();

    VerifyOrQuit(CountKeysViaIterator(instance) == 0);

    Log("TestThreadDirectKeyManager passed successfully!");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::TestThreadDirectKeyManager();
    printf("All tests passed\n");
    return 0;
}
