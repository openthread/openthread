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

/**
 * @file
 *   This file includes implementation of Thread Direct Key Manager.
 */

#include "td_key_manager.hpp"

#if OT_CONFIG_THREAD_DIRECT_ENABLE

#include "instance/instance.hpp"

namespace ot {
namespace ThreadDirect {

RegisterLogModule("TdKeyManager");

//---------------------------------------------------------------------------------------------------------------------
// KeyIterator

void KeyIterator::Init(Instance &aInstance) { mPtr = aInstance.Get<KeyManager>().GetKeyList().GetHead(); }

const Key *KeyIterator::GetNext(void)
{
    const Key *key = static_cast<const Key *>(mPtr);

    VerifyOrExit(key != nullptr);
    mPtr = key->GetNext();

exit:
    return key;
}

//---------------------------------------------------------------------------------------------------------------------
// KeyManager

KeyManager::KeyManager(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mStoreTask(aInstance)
{
}

#if OPENTHREAD_CONFIG_TD_KEY_LIST_USE_HEAP_ENABLE
KeyManager::~KeyManager(void)
{
    Key *key;

    while ((key = mKeys.Pop()) != nullptr)
    {
        key->Free();
    }
}
#endif

Key *KeyManager::AllocateKey(void)
{
    Key *key;

#if OPENTHREAD_CONFIG_TD_KEY_LIST_USE_HEAP_ENABLE
    if ((kMaxKeys != 0) && (mKeys.CountAllEntries() >= kMaxKeys))
    {
        key = nullptr;
    }
    else
    {
        key = Key::Allocate();
    }
#else
    key = mPool.Allocate();
#endif

    return key;
}

void KeyManager::FreeKey(Key &aKey)
{
#if OPENTHREAD_CONFIG_PLATFORM_KEY_REFERENCES_ENABLE
    // A `KeyRef` can be shared across multiple registered `Key` entries
    // with different Key IDs. If another entry in `mKeys` is still
    // using the same `KeyRef`, reset `aKey`'s `KeyRef` to invalid
    // before calling `Clear()` so that `Clear()` does not destroy the
    // shared `KeyRef` in platform crypto storage.

    if (mKeys.ContainsMatching(aKey.GetKeyMaterial().GetKeyRef()))
    {
        aKey.SetKeyRefToInvalid();
    }
#endif

    aKey.mKeyMaterial.Clear();

#if OPENTHREAD_CONFIG_TD_KEY_LIST_USE_HEAP_ENABLE
    aKey.Free();
#else
    mPool.Free(aKey);
#endif
}

Key *KeyManager::RegisterKey(const KeyMaterial &aKeyMaterial, const KeyId &aKeyId)
{
    Key *key = FindKey(aKeyMaterial, aKeyId);

    VerifyOrExit(key == nullptr);

    key = AllocateKey();
    VerifyOrExit(key != nullptr);

    key->Init(GetInstance(), aKeyId, /* aFrameCounter */ 0);
    key->SetKeyMaterial(aKeyMaterial);

    mKeys.Push(*key);

    Get<Settings>().SaveThreadDirectKey(*key);

    Log(kActionRegister, *key);

exit:
    return key;
}

Key *KeyManager::FindKey(const KeyMaterial &aKeyMaterial, const KeyId &aKeyId)
{
    return mKeys.FindMatching(aKeyMaterial, aKeyId);
}

Key *KeyManager::FindKey(const Settings::ThreadDirectKeyInfo &aKeyInfo)
{
    Key *matchedKey = nullptr;

    for (Key &key : mKeys)
    {
        if (aKeyInfo.Matches(key))
        {
            matchedKey = &key;
            break;
        }
    }

    return matchedKey;
}

void KeyManager::UnregisterKey(Key &aKey)
{
    SuccessOrAssert(mKeys.Remove(aKey));
    Get<Settings>().DeleteThreadDirectKey(aKey);

    Log(kActionUnregister, aKey);

    FreeKey(aKey);
}

void KeyManager::UnregisterAllKeys(void)
{
    Key *key;

    LogInfo("Unregistering all keys");

    while ((key = mKeys.Pop()) != nullptr)
    {
        FreeKey(*key);
    }

    Get<Settings>().DeleteAllThreadDirectKeys();
}

void KeyManager::RestoreKeys(void)
{
    Key                          *key;
    Settings::ThreadDirectKeyInfo keyInfo;

    for (int index = 0;; index++)
    {
        SuccessOrExit(Get<Settings>().ReadThreadDirectKey(index, keyInfo));

        key = FindKey(keyInfo);

        if (key != nullptr)
        {
            // Multiple entries for the same key may exist in settings if a
            // reset occurred while `SaveThreadDirectKey()` was adding a new
            // entry and before the older entry was removed. Keep the highest
            // frame counter.

            if (keyInfo.GetFrameCounter() > key->GetFrameCounter())
            {
                key->SetFrameCounters(keyInfo.GetFrameCounter());
                Log(kActionRestoreNewCounter, *key);
            }

            continue;
        }

        key = AllocateKey();

        if (key == nullptr)
        {
            LogWarn("Could not restore - too many saved keys (kMaxKeys:%u)", kMaxKeys);
            ExitNow();
        }

        key->Init(GetInstance(), keyInfo.GetKeyId(), keyInfo.GetFrameCounter());
#if !OPENTHREAD_CONFIG_PLATFORM_KEY_REFERENCES_ENABLE
        key->SetKeyMaterial(keyInfo.GetKey());
#else
        key->SetKeyMaterial(keyInfo.GetKeyRef());
#endif

        mKeys.Push(*key);

        Log(kActionRestore, *key);
    }

exit:
    return;
}

void KeyManager::StoreKeys(void)
{
    for (Key &key : mKeys)
    {
        if (key.ShouldUpdateSavedFrameCounter())
        {
            Get<Settings>().SaveThreadDirectKey(key);
        }
    }
}

#if OT_SHOULD_LOG_AT(OT_LOG_LEVEL_INFO)

const char *KeyManager::ActionToString(Action aAction)
{
#define ActionMapList(_)                 \
    _(kActionRegister, "Registered")     \
    _(kActionUnregister, "Unregistered") \
    _(kActionRestore, "Restored")        \
    _(kActionRestoreNewCounter, "Restored (new counter)")

    DefineEnumStringArray(ActionMapList);

    return kStrings[aAction];
}

void KeyManager::Log(Action aAction, const Key &aKey)
{
    LogInfo("%s key {id:%s, ctr:0x%lx, savedctr:0x%lx}", ActionToString(aAction), aKey.GetId().ToString().AsCString(),
            ToUlong(aKey.GetFrameCounter()), ToUlong(aKey.GetSavedFrameCounter()));
}

#endif // #if OT_SHOULD_LOG_AT(OT_LOG_LEVEL_INFO)

} // namespace ThreadDirect
} // namespace ot

#endif // OT_CONFIG_THREAD_DIRECT_ENABLE
