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
 *   This file includes definitions for Thread Direct Key Manager
 */

#ifndef OT_CORE_THREAD_DIRECT_TD_KEY_MANAGER_HPP_
#define OT_CORE_THREAD_DIRECT_TD_KEY_MANAGER_HPP_

#include "openthread-core-config.h"

#include "common/as_core_type.hpp"
#include "common/linked_list.hpp"
#include "common/locator.hpp"
#include "common/log.hpp"
#include "common/non_copyable.hpp"
#include "common/pool.hpp"
#include "common/settings.hpp"
#include "common/tasklet.hpp"
#include "thread_direct/td_types.hpp"

#if OT_CONFIG_THREAD_DIRECT_ENABLE

namespace ot {
namespace ThreadDirect {

/**
 * Represents an iterator for iterating through registered Thread Direct keys.
 */
class KeyIterator : public otThreadDirectKeyIterator
{
public:
    /**
     * Initializes the `KeyIterator` to iterate over registered Thread Direct keys.
     *
     * @param[in] aInstance  A reference to the OpenThread instance.
     */
    void Init(Instance &aInstance);

    /**
     * Gets the next registered Thread Direct key in the list and advances the iterator.
     *
     * @returns A pointer to the next `Key` entry, or `nullptr` if the end of the list is reached.
     */
    const Key *GetNext(void);
};

/**
 * Manages registered Thread Direct keys and their persistent storage in settings.
 */
class KeyManager : public InstanceLocator, private NonCopyable
{
    friend class ot::Instance;
    friend class Key;

public:
    /**
     * Represents the linked list of registered Thread Direct keys.
     */
    typedef LinkedList<Key> KeyList;

    /**
     * Initializes the `KeyManager` object.
     *
     * @param[in] aInstance  A reference to the OpenThread instance.
     */
    explicit KeyManager(Instance &aInstance);

#if OPENTHREAD_CONFIG_TD_KEY_LIST_USE_HEAP_ENABLE
    /**
     * Destroys the `KeyManager` object and frees any heap-allocated `Key` entries.
     */
    ~KeyManager(void);
#endif

    /**
     * Registers a Thread Direct key and its associated Key ID.
     *
     * If a matching key with the same @p aKeyMaterial and @p aKeyId is already registered, a pointer to the existing
     * `Key` entry is returned. Otherwise, a new `Key` entry is allocated, initialized, and saved in persistent
     * settings.
     *
     * @param[in] aKeyMaterial  The key material to register.
     * @param[in] aKeyId        The Key ID to register.
     *
     * @returns A pointer to the registered `Key` entry, or `nullptr` if a new entry could not be allocated.
     */
    Key *RegisterKey(const KeyMaterial &aKeyMaterial, const KeyId &aKeyId);

    /**
     * Finds a registered Thread Direct key matching a given key material and Key ID.
     *
     * @param[in] aKeyMaterial  The key material to match.
     * @param[in] aKeyId        The Key ID to match.
     *
     * @returns A pointer to the matching `Key` entry, or `nullptr` if no match is found.
     */
    Key *FindKey(const KeyMaterial &aKeyMaterial, const KeyId &aKeyId);

    /**
     * Unregisters a given Thread Direct key, removes it from persistent settings, and frees its entry.
     *
     * @param[in] aKey  The registered `Key` to unregister.
     */
    void UnregisterKey(Key &aKey);

    /**
     * Unregisters all Thread Direct keys, removes them from persistent settings, and frees their entries.
     */
    void UnregisterAllKeys(void);

    /**
     * Gets the list of registered Thread Direct keys.
     *
     * @returns A constant reference to the `KeyList`.
     */
    const KeyList &GetKeyList(void) const { return mKeys; }

    /**
     * Gets the list of registered Thread Direct keys.
     *
     * @returns A reference to the `KeyList`.
     */
    KeyList &GetKeyList(void) { return mKeys; }

private:
    static constexpr uint16_t kMaxKeys = OPENTHREAD_CONFIG_TD_MAX_REGISTERED_KEYS;

    enum Action : uint8_t // Used in `Log()`.
    {
        kActionRegister,
        kActionUnregister,
        kActionRestore,
        kActionRestoreNewCounter,
    };

    Key *AllocateKey(void);
    void FreeKey(Key &aKey);
    Key *FindKey(const Settings::ThreadDirectKeyInfo &aKeyInfo);
    void RestoreKeys(void);
    void StoreKeys(void);
    void PostStoreTask(void) { mStoreTask.Post(); }

#if OT_SHOULD_LOG_AT(OT_LOG_LEVEL_INFO)
    static const char *ActionToString(Action aAction);
    void               Log(Action aAction, const Key &aKey);
#else
    void Log(Action, const Key &) {}
#endif

    using StoreTask = TaskletIn<KeyManager, &KeyManager::StoreKeys>;

    LinkedList<Key> mKeys;
    StoreTask       mStoreTask;
#if !OPENTHREAD_CONFIG_TD_KEY_LIST_USE_HEAP_ENABLE
    Pool<Key, kMaxKeys> mPool;
#endif
};

} // namespace ThreadDirect

DefineCoreType(otThreadDirectKeyIterator, ThreadDirect::KeyIterator);

} // namespace ot

#endif // OT_CONFIG_THREAD_DIRECT_ENABLE

#endif // OT_CORE_THREAD_DIRECT_TD_KEY_MANAGER_HPP_
