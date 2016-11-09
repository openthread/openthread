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

/**
 * @file
 *   This file includes definitions for Thread security material generation.
 */

#ifndef KEY_MANAGER_HPP_
#define KEY_MANAGER_HPP_

#include <stdint.h>

#include <openthread-types.h>
#include <common/timer.hpp>
#include <crypto/hmac_sha256.hpp>

namespace Thread {

class ThreadNetif;

/**
 * @addtogroup core-security
 *
 * @brief
 *   This module includes definitions for Thread security material generation.
 *
 * @{
 */

class KeyManager
{
public:
    /**
     * This constructor initializes the object.
     *
     * @param[in]  aThreadNetif  A reference to the Thread network interface.
     *
     */
    explicit KeyManager(ThreadNetif &aThreadNetif);

    /**
     * This method starts KeyManager rotation timer and sets guard timer to initial value.
     *
     */
    void Start(void);

    /**
     * This method stops KeyManager timers.
     *
     */
    void Stop(void);

    /**
     * This method returns a pointer to the Thread Master Key
     *
     * @param[out]  aKeyLength  A pointer where the key length value will be placed.
     *
     * @returns A pointer to the Thread Master Key.
     *
     */
    const uint8_t *GetMasterKey(uint8_t *aKeyLength) const;

    /**
     * This method sets the Thread Master Key.
     *
     * @param[in]  aKey        A pointer to the Thread Master Key.
     * @param[in]  aKeyLength  The length of @p aKey.
     *
     * @retval kThreadError_None         Successfully set the Thread Master Key.
     * @retval kThreadError_InvalidArgs  The @p aKeyLength value was invalid.
     *
     */
    ThreadError SetMasterKey(const void *aKey, uint8_t aKeyLength);

    /**
     * This method returns the current key sequence value.
     *
     * @returns The current key sequence value.
     *
     */
    uint32_t GetCurrentKeySequence(void) const;

    /**
     * This method sets the current key sequence value.
     *
     * @param[in]  aKeySequence  The key sequence value.
     *
     */
    void SetCurrentKeySequence(uint32_t aKeySequence);

    /**
     * This method returns a pointer to the current MAC key.
     *
     * @returns A pointer to the current MAC key.
     *
     */
    const uint8_t *GetCurrentMacKey(void) const;

    /**
     * This method returns a pointer to the current MLE key.
     *
     * @returns A pointer to the current MLE key.
     *
     */
    const uint8_t *GetCurrentMleKey(void) const;

    /**
     * This method returns a pointer to a temporary MAC key computed from the given key sequence.
     *
     * @param[in]  aKeySequence  The key sequence value.
     *
     * @returns A pointer to the temporary MAC key.
     *
     */
    const uint8_t *GetTemporaryMacKey(uint32_t aKeySequence);

    /**
     * This method returns a pointer to a temporary MLE key computed from the given key sequence.
     *
     * @param[in]  aKeySequence  The key sequence value.
     *
     * @returns A pointer to the temporary MLE key.
     *
     */
    const uint8_t *GetTemporaryMleKey(uint32_t aKeySequence);

    /**
     * This method returns the current MAC Frame Counter value.
     *
     * @returns The current MAC Frame Counter value.
     *
     */
    uint32_t GetMacFrameCounter(void) const;

    /**
     * This method increments the current MAC Frame Counter value.
     *
     */
    void IncrementMacFrameCounter(void);

    /**
     * This method returns the current MLE Frame Counter value.
     *
     * @returns The current MLE Frame Counter value.
     *
     */
    uint32_t GetMleFrameCounter(void) const;

    /**
     * This method increments the current MLE Frame Counter value.
     *
     */
    void IncrementMleFrameCounter(void);

    /**
     * This method returns the KEK.
     *
     * @returns A pointer to the KEK.
     *
     */
    const uint8_t *GetKek(void) const;

    /**
     * This method sets the KEK.
     *
     * @param[in]  aKek  A pointer to the KEK.
     *
     */
    void SetKek(const uint8_t *aKek);

    /**
     * This method returns the current KEK Frame Counter value.
     *
     * @returns The current KEK Frame Counter value.
     *
     */
    uint32_t GetKekFrameCounter(void) const;

    /**
     * This method increments the current KEK Frame Counter value.
     *
     */
    void IncrementKekFrameCounter(void);

    /**
     * This method returns the KeyRotation time.
     *
     * The KeyRotation time is the time interval after witch security key will be automatically rotated.
     *
     * @returns The KeyRotation value in hours.
     */
    uint32_t GetKeyRotation(void) const { return mKeyRotationTime; }

    /**
     * This method sets the KeyRotation time.
     *
     * The KeyRotation time is the time interval after witch security key will be automatically rotated.
     * It's value shall be in range [kMinKeyRotationTime, kMaxKeyRotationTime].
     *
     * @param[in]  aKeyRotation  The KeyRotation value in hours.
     *
     * @retval  kThreadError_None         KeyRotation time updated.
     * @retval  kThreadError_InvalidArgs  @p aKeyRotation is out of range.
     *
     */
    ThreadError SetKeyRotation(uint32_t aKeyRotation);

    /**
     * This method returns the KeySwitchGuardTime.
     *
     * The KeySwitchGuardTime is the time interval during which key rotation procedure is prevented.
     *
     * @returns The KeySwitchGuardTime value in hours.
     *
     */
    uint32_t GetKeySwitchGuardTime(void) const { return mKeySwitchGuardTime; }

    /**
     * This method sets the KeySwitchGuardTime.
     *
     * The KeySwitchGuardTime is the time interval during which key rotation procedure is prevented.
     *
     * @param[in]  aKeySwitchGuardTime  The KeySwitchGuardTime value in hours.
     *
     */
    void SetKeySwitchGuardTime(uint32_t aKeySwitchGuardTime) { mKeySwitchGuardTime = aKeySwitchGuardTime; }

    /**
     * This method returns the Security Policy Flags.
     *
     * The Security Policy Flags specifies network administrator preferences for which
     * security-related operations are allowed or disallowed.
     *
     * @returns The SecurityPolicy Flags.
     *
     */
    uint8_t GetSecurityPolicyFlags(void) const { return mSecurityPolicyFlags; }

    /**
     * This method sets the Security Policy Flags.
     *
     * The Security Policy Flags specifies network administrator preferences for which
     * security-related operations are allowed or disallowed.
     *
     * @param[in]  aSecurityPolicyFlags  The Security Policy Flags.
     *
     */
    void SetSecurityPolicyFlags(uint8_t aSecurityPolicyFlags) { mSecurityPolicyFlags = aSecurityPolicyFlags; }

private:
    enum
    {
        kMaxKeyLength = 16,
        kMinKeyRotationTime = 1,
        kMaxKeyRotationTime = 0xffffffff / 3600u / 1000u,
        kDefaultKeyRotationTime = 672,
        kDefaultKeySwitchGuardTime = 624,
    };

    ThreadError ComputeKey(uint32_t aKeySequence, uint8_t *aKey);

    static void HandleKeyRotationTimer(void *aContext);
    void HandleKeyRotationTimer(void);

    ThreadNetif &mNetif;

    uint8_t mMasterKey[kMaxKeyLength];
    uint8_t mMasterKeyLength;

    uint32_t mKeySequence;
    uint8_t mKey[Crypto::HmacSha256::kHashSize];

    uint8_t mTemporaryKey[Crypto::HmacSha256::kHashSize];

    uint32_t mMacFrameCounter;
    uint32_t mMleFrameCounter;

    uint32_t mKeyRotationTime;
    uint32_t mKeySwitchGuardTime;
    bool     mKeySwitchGuardEnabled;
    Timer    mKeyRotationTimer;

    uint8_t mKek[kMaxKeyLength];
    uint32_t mKekFrameCounter;

    uint8_t mSecurityPolicyFlags;
};

/**
 * @}
 */

}  // namespace Thread

#endif  // KEY_MANAGER_HPP_
