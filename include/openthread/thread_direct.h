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
 * @brief
 *  This file defines the OpenThread Thread Direct APIs.
 */

#ifndef OPENTHREAD_THREAD_DIRECT_H_
#define OPENTHREAD_THREAD_DIRECT_H_

#include <stdint.h>

#include <openthread/instance.h>
#include <openthread/platform/radio.h>
#include <openthread/platform/toolchain.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup api-thread-direct
 *
 * @brief
 *   This module includes functions and definitions for Thread Direct (TD).
 *
 *   The APIs in this module require Thread Direct functionality to be enabled (e.g., via
 *   `OPENTHREAD_CONFIG_TD_WAKE_INITIATOR_ENABLE` or `OPENTHREAD_CONFIG_TD_WAKE_LISTENER_ENABLE`).
 *
 * @{
 */

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Thread Direct Key

#define OT_THREAD_DIRECT_KEY_SIZE OT_MAC_KEY_SIZE ///< Size of a Thread Direct Key in bytes (16 bytes).

#define OT_THREAD_DIRECT_KEY_ID_SIZE 5 ///< Size of a Thread Direct Key ID in bytes.

/**
 * An opaque type representing a registered Thread Direct key entry in OpenThread.
 */
typedef struct otThreadDirectKey otThreadDirectKey;

/**
 * Represents Thread Direct key material (literal key bytes or cryptographic key reference).
 *
 * Depending on the build-time configuration `OPENTHREAD_CONFIG_PLATFORM_KEY_REFERENCES_ENABLE`, either
 * `mKeyMaterial.mKey` or `mKeyMaterial.mKeyRef` is used:
 *
 * - When `OPENTHREAD_CONFIG_PLATFORM_KEY_REFERENCES_ENABLE` is disabled, `mKeyMaterial.mKey.m8` is used to specify or
 *   retrieve the literal 16-byte key material.
 * - When `OPENTHREAD_CONFIG_PLATFORM_KEY_REFERENCES_ENABLE` is enabled, `mKeyMaterial.mKeyRef` is used to pass or
 *   retrieve the `otMacKeyRef` (`otCryptoKeyRef`) associated with the key in platform crypto storage.
 *
 * When key references (`mKeyMaterial.mKeyRef`) are used, the next higher layer is responsible for importing/creating
 * the key in platform crypto storage prior to registering it with OpenThread. The key reference MUST be stored
 * persistently in platform crypto storage so that the registered `mKeyRef` remains valid across device reboots/resets.
 * The caller MAY reuse the same `mKeyRef` associated with an already registered Thread Direct key when registering
 * additional keys with different Key IDs (`otThreadDirectKeyId`). Upon successful registration via
 * `otThreadDirectRegisterKey()`, OpenThread manages the lifetime of the `mKeyRef` and will destroy/remove the key from
 * platform crypto storage (via `otPlatCryptoDestroyKey()`) once all registered Thread Direct keys using that `mKeyRef`
 * have been unregistered.
 */
typedef otMacKeyMaterial otThreadDirectKeyMaterial;

/**
 * @struct otThreadDirectKeyId
 *
 * Represents a Thread Direct Key ID.
 */
OT_TOOL_PACKED_BEGIN
struct otThreadDirectKeyId
{
    uint8_t m8[OT_THREAD_DIRECT_KEY_ID_SIZE]; ///< Key ID bytes.
} OT_TOOL_PACKED_END;

typedef struct otThreadDirectKeyId otThreadDirectKeyId;

/**
 * Represents an iterator for iterating through registered Thread Direct keys.
 *
 * Use `otThreadDirectInitKeyIterator()` to initialize the iterator before using it with `otThreadDirectGetNextKey()`.
 *
 * The caller MUST NOT access or update the fields in this struct. It is intended for OpenThread internal use only.
 */
typedef struct otThreadDirectKeyIterator
{
    const void *mPtr;
} otThreadDirectKeyIterator;

/**
 * Registers a Thread Direct key and its associated Key ID with OpenThread.
 *
 * If a matching key with the same @p aKeyMaterial and @p aKeyId is already registered, this function returns a pointer
 * to the existing `otThreadDirectKey` entry. Otherwise, it allocates and registers a new entry. If a new entry cannot
 * be allocated (e.g., the maximum number of allowed/supported registered keys is reached), `NULL` is returned.
 *
 * As described in `otThreadDirectKeyMaterial`, either `aKeyMaterial->mKeyMaterial.mKey` or
 * `aKeyMaterial->mKeyMaterial.mKeyRef` is used depending on whether `OPENTHREAD_CONFIG_PLATFORM_KEY_REFERENCES_ENABLE`
 * is enabled. When using `mKeyMaterial.mKeyRef`, the caller MUST ensure that the key reference is stored persistently
 * in platform crypto storage across reboots, and MAY reuse the same `mKeyRef` across multiple registered keys with
 * different Key IDs. Upon successful registration, OpenThread manages `aKeyMaterial->mKeyMaterial.mKeyRef` and will
 * destroy/remove the key from platform crypto storage once all Thread Direct keys using that `mKeyRef` are
 * unregistered.
 *
 * @param[in] aInstance     The OpenThread instance.
 * @param[in] aKeyMaterial  The Thread Direct key material to register.
 * @param[in] aKeyId        The Thread Direct Key ID to register.
 *
 * @returns A pointer to the registered `otThreadDirectKey` entry, or `NULL` if it could not be registered.
 */
const otThreadDirectKey *otThreadDirectRegisterKey(otInstance                      *aInstance,
                                                   const otThreadDirectKeyMaterial *aKeyMaterial,
                                                   const otThreadDirectKeyId       *aKeyId);

/**
 * Finds a registered Thread Direct key matching the given key material and Key ID.
 *
 * @param[in] aInstance     The OpenThread instance.
 * @param[in] aKeyMaterial  The key material to match.
 * @param[in] aKeyId        The Key ID to match.
 *
 * @returns A pointer to the matching registered `otThreadDirectKey`, or `NULL` if no match was found.
 */
const otThreadDirectKey *otThreadDirectFindKey(otInstance                      *aInstance,
                                               const otThreadDirectKeyMaterial *aKeyMaterial,
                                               const otThreadDirectKeyId       *aKeyId);

/**
 * Unregisters a previously registered Thread Direct key from OpenThread.
 *
 * Once unregistered, the @p aKey pointer is no longer valid and MUST NOT be used.
 *
 * When `OPENTHREAD_CONFIG_PLATFORM_KEY_REFERENCES_ENABLE` is enabled, this call also destroys/removes the associated
 * key reference (`mKeyRef`) from platform crypto storage (via `otPlatCryptoDestroyKey()`) if no other registered
 * Thread Direct key is still using the same `mKeyRef`.
 *
 * @param[in] aInstance  The OpenThread instance.
 * @param[in] aKey       The registered `otThreadDirectKey` to unregister.
 */
void otThreadDirectUnregisterKey(otInstance *aInstance, const otThreadDirectKey *aKey);

/**
 * Unregisters all previously registered Thread Direct keys from OpenThread.
 *
 * When `OPENTHREAD_CONFIG_PLATFORM_KEY_REFERENCES_ENABLE` is enabled, this call also destroys/removes the associated
 * key references (`mKeyRef`) for all registered keys from platform crypto storage (via `otPlatCryptoDestroyKey()`).
 *
 * @param[in] aInstance  The OpenThread instance.
 */
void otThreadDirectUnregisterAllKeys(otInstance *aInstance);

/**
 * Initializes an `otThreadDirectKeyIterator`.
 *
 * An iterator MUST be initialized before it is used with `otThreadDirectGetNextKey()`. An iterator can be initialized
 * again to restart iteration from the beginning of the registered keys list.
 *
 * @param[in]  aInstance  The OpenThread instance.
 * @param[out] aIterator  The iterator to initialize.
 */
void otThreadDirectInitKeyIterator(otInstance *aInstance, otThreadDirectKeyIterator *aIterator);

/**
 * Iterates through the registered Thread Direct keys and returns the next key entry.
 *
 * @param[in,out] aIterator  The iterator. MUST be initialized via `otThreadDirectInitKeyIterator()`.
 *
 * @returns A pointer to the next registered `otThreadDirectKey`, or `NULL` if the end of the list is reached.
 */
const otThreadDirectKey *otThreadDirectGetNextKey(otThreadDirectKeyIterator *aIterator);

/**
 * Gets the key material associated with a registered Thread Direct key.
 *
 * @param[in] aKey  A registered `otThreadDirectKey`.
 *
 * @returns The `otThreadDirectKeyMaterial` associated with @p aKey.
 */
const otThreadDirectKeyMaterial *otThreadDirectKeyGetMaterial(const otThreadDirectKey *aKey);

/**
 * Gets the Key ID associated with a registered Thread Direct key.
 *
 * @param[in] aKey  A registered `otThreadDirectKey`.
 *
 * @returns The `otThreadDirectKeyId` associated with @p aKey.
 */
const otThreadDirectKeyId *otThreadDirectKeyGetId(const otThreadDirectKey *aKey);

/**
 * Gets the current MAC security frame counter associated with a registered Thread Direct key.
 *
 * @param[in] aKey  A registered `otThreadDirectKey`.
 *
 * @returns The MAC security frame counter associated with @p aKey.
 */
uint32_t otThreadDirectKeyGetFrameCounter(const otThreadDirectKey *aKey);

/**
 * @}
 */

#ifdef __cplusplus
} // extern "C"
#endif

#endif // OPENTHREAD_THREAD_DIRECT_H_
