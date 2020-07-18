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

#include "openthread-core-config.h"

#include <stdint.h>

#include <openthread/dataset.h>

#include "common/clearable.hpp"
#include "common/equatable.hpp"
#include "common/locator.hpp"
#include "common/random.hpp"
#include "common/timer.hpp"
#include "crypto/hmac_sha256.hpp"
#include "mac/mac_types.hpp"
#include "thread/mle_types.hpp"

namespace ot {

/**
 * @addtogroup core-security
 *
 * @brief
 *   This module includes definitions for Thread security material generation.
 *
 * @{
 */

/**
 * This class represents a Thread Master Key.
 *
 */
OT_TOOL_PACKED_BEGIN
class MasterKey : public otMasterKey, public Equatable<MasterKey>
{
public:
#if !OPENTHREAD_RADIO
    /**
     * This method generates a cryptographically secure random sequence to populate the Thread Master Key.
     *
     * @retval OT_ERROR_NONE     Successfully generated a random Thread Master Key.
     * @retval OT_ERROR_FAILED   Failed to generate random sequence.
     *
     */
    otError GenerateRandom(void) { return Random::Crypto::FillBuffer(m8, sizeof(m8)); }
#endif
} OT_TOOL_PACKED_END;

/**
 * This class represents a Thread Pre-Shared Key for the Commissioner (PSKc).
 *
 */
OT_TOOL_PACKED_BEGIN
class Pskc : public otPskc, public Equatable<Pskc>, public Clearable<Pskc>
{
public:
#if !OPENTHREAD_RADIO
    /**
     * This method generates a cryptographically secure random sequence to populate the Thread PSKc.
     *
     * @retval OT_ERROR_NONE  Successfully generated a random Thread PSKc.
     *
     */
    otError GenerateRandom(void) { return Random::Crypto::FillBuffer(m8, sizeof(Pskc)); }
#endif

} OT_TOOL_PACKED_END;

/**
 *
 * This class represents a Key Encryption Key (KEK).
 *
 */
typedef Mac::Key Kek;

/**
 * This class defines Thread Key Manager.
 *
 */
class KeyManager : public InstanceLocator
{
public:
    /**
     * This constructor initializes the object.
     *
     * @param[in]  aInstance     A reference to the OpenThread instance.
     *
     */
    explicit KeyManager(Instance &aInstance);

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
     * This method returns the Thread Master Key.
     *
     * @returns The Thread Master Key.
     *
     */
    const MasterKey &GetMasterKey(void) const { return mMasterKey; }

    /**
     * This method sets the Thread Master Key.
     *
     * @param[in]  aKey        A Thread Master Key.
     *
     * @retval OT_ERROR_NONE          Successfully set the Thread Master Key.
     * @retval OT_ERROR_INVALID_ARGS  The @p aKeyLength value was invalid.
     *
     */
    otError SetMasterKey(const MasterKey &aKey);

#if OPENTHREAD_FTD || OPENTHREAD_MTD
    /**
     * This method indicates whether the PSKc is configured.
     *
     * A value of all zeros indicates that the PSKc is not configured.
     *
     * @retval TRUE  if the PSKc is configured.
     * @retval FALSE if the PSKc is not not configured.
     *
     */
    bool IsPskcSet(void) const { return mIsPskcSet; }

    /**
     * This method returns a pointer to the PSKc.
     *
     * @returns A reference to the PSKc.
     *
     */
    const Pskc &GetPskc(void) const { return mPskc; }

    /**
     * This method sets the PSKc.
     *
     * @param[in]  aPskc    A reference to the PSKc.
     *
     */
    void SetPskc(const Pskc &aPskc);
#endif

    /**
     * This method returns the current key sequence value.
     *
     * @returns The current key sequence value.
     *
     */
    uint32_t GetCurrentKeySequence(void) const { return mKeySequence; }

    /**
     * This method sets the current key sequence value.
     *
     * @param[in]  aKeySequence  The key sequence value.
     *
     */
    void SetCurrentKeySequence(uint32_t aKeySequence);

    /**
     * This method returns a pointer to the current MLE key.
     *
     * @returns A pointer to the current MLE key.
     *
     */
    const Mle::Key &GetCurrentMleKey(void) const { return mMleKey; }

    /**
     * This method returns a pointer to a temporary MLE key computed from the given key sequence.
     *
     * @param[in]  aKeySequence  The key sequence value.
     *
     * @returns A pointer to the temporary MLE key.
     *
     */
    const Mle::Key &GetTemporaryMleKey(uint32_t aKeySequence);

    /**
     * This method returns the current MAC Frame Counter value.
     *
     * @returns The current MAC Frame Counter value.
     *
     */
    uint32_t GetMacFrameCounter(void) const;

    /**
     * This method sets the current MAC Frame Counter value.
     *
     * @param[in]  aMacFrameCounter  The MAC Frame Counter value.
     *
     */
    void SetMacFrameCounter(uint32_t aMacFrameCounter);

    /**
     * This method sets the MAC Frame Counter value which is stored in non-volatile memory.
     *
     * @param[in]  aStoredMacFrameCounter  The stored MAC Frame Counter value.
     *
     */
    void SetStoredMacFrameCounter(uint32_t aStoredMacFrameCounter) { mStoredMacFrameCounter = aStoredMacFrameCounter; }

    /**
     * This method returns the current MLE Frame Counter value.
     *
     * @returns The current MLE Frame Counter value.
     *
     */
    uint32_t GetMleFrameCounter(void) const { return mMleFrameCounter; }

    /**
     * This method sets the current MLE Frame Counter value.
     *
     * @param[in]  aMleFrameCounter  The MLE Frame Counter value.
     *
     */
    void SetMleFrameCounter(uint32_t aMleFrameCounter) { mMleFrameCounter = aMleFrameCounter; }

    /**
     * This method sets the MLE Frame Counter value which is stored in non-volatile memory.
     *
     * @param[in]  aStoredMleFrameCounter  The stored MLE Frame Counter value.
     *
     */
    void SetStoredMleFrameCounter(uint32_t aStoredMleFrameCounter) { mStoredMleFrameCounter = aStoredMleFrameCounter; }

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
    const Kek &GetKek(void) const { return mKek; }

    /**
     * This method sets the KEK.
     *
     * @param[in]  aKek  A KEK.
     *
     */
    void SetKek(const Kek &aKek);

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
    uint32_t GetKekFrameCounter(void) const { return mKekFrameCounter; }

    /**
     * This method increments the current KEK Frame Counter value.
     *
     */
    void IncrementKekFrameCounter(void) { mKekFrameCounter++; }

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
     * Its value shall be larger than or equal to kMinKeyRotationTime.
     *
     * @param[in]  aKeyRotation  The KeyRotation value in hours.
     *
     * @retval  OT_ERROR_NONE          KeyRotation time updated.
     * @retval  OT_ERROR_INVALID_ARGS  @p aKeyRotation is out of range.
     *
     */
    otError SetKeyRotation(uint32_t aKeyRotation);

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
    void SetSecurityPolicyFlags(uint8_t aSecurityPolicyFlags);

    /**
     * This method indicates whether or not obtaining Master key for out-of-band is enabled.
     *
     * @retval TRUE   If obtaining Master key for out-of-band is enabled.
     * @retval FALSE  If obtaining Master key for out-of-band is not enabled.
     *
     */
    bool IsObtainMasterKeyEnabled(void) const
    {
        return (mSecurityPolicyFlags & OT_SECURITY_POLICY_OBTAIN_MASTER_KEY) != 0;
    }

    /**
     * This method indicates whether or not Native Commissioning using PSKc is allowed.
     *
     * @retval TRUE   If Native Commissioning using PSKc is allowed.
     * @retval FALSE  If Native Commissioning using PSKc is not allowed.
     *
     */
    bool IsNativeCommissioningAllowed(void) const
    {
        return (mSecurityPolicyFlags & OT_SECURITY_POLICY_NATIVE_COMMISSIONING) != 0;
    }

    /**
     * This method indicates whether or not Thread 1.1 Routers are enabled.
     *
     * @retval TRUE   If Thread 1.1 Routers are enabled.
     * @retval FALSE  If Thread 1.1 Routers are not enabled.
     *
     */
    bool IsRouterEnabled(void) const { return (mSecurityPolicyFlags & OT_SECURITY_POLICY_ROUTERS) != 0; }

    /**
     * This method indicates whether or not external Commissioner authentication is allowed using PSKc.
     *
     * @retval TRUE   If the commissioning sessions by an external Commissioner based on the PSKc are allowed
     *                to be established and that changes to the Commissioner Dataset by on-mesh nodes are allowed.
     * @retval FALSE  If the commissioning sessions by an external Commissioner based on the PSKc are not allowed
     *                to be established and that changes to the Commissioner Dataset by on-mesh nodes are not allowed.
     *
     */
    bool IsExternalCommissionerAllowed(void) const
    {
        return (mSecurityPolicyFlags & OT_SECURITY_POLICY_EXTERNAL_COMMISSIONER) != 0;
    }

    /**
     * This method indicates whether or not Thread Beacons are enabled.
     *
     * @retval TRUE   If Thread Beacons are enabled.
     * @retval FALSE  If Thread Beacons are not enabled.
     *
     */
    bool IsThreadBeaconEnabled(void) const { return (mSecurityPolicyFlags & OT_SECURITY_POLICY_BEACONS) != 0; }

    /**
     * This method updates the MAC keys and MLE key.
     *
     */
    void UpdateKeyMaterial(void);

    /**
     * This method handles MAC frame counter change.
     *
     * @param[in]  aMacFrameCounter  The MAC frame counter value.
     *
     */
    void MacFrameCounterUpdated(uint32_t aMacFrameCounter);

private:
    enum
    {
        kMinKeyRotationTime        = 1,
        kDefaultKeyRotationTime    = 672,
        kDefaultKeySwitchGuardTime = 624,
        kOneHourIntervalInMsec     = 3600u * 1000u,
    };

    OT_TOOL_PACKED_BEGIN
    struct Keys
    {
        Mle::Key mMleKey;
        Mac::Key mMacKey;
    } OT_TOOL_PACKED_END;

    union HashKeys
    {
        uint8_t mHash[Crypto::HmacSha256::kHashSize];
        Keys    mKeys;
    };

    void ComputeKeys(uint32_t aKeySequence, HashKeys &aHashKeys);

    void        StartKeyRotationTimer(void);
    static void HandleKeyRotationTimer(Timer &aTimer);
    void        HandleKeyRotationTimer(void);

    static const uint8_t     kThreadString[];
    static const otMasterKey kDefaultMasterKey;

    MasterKey mMasterKey;

    uint32_t mKeySequence;
    Mle::Key mMleKey;
    Mle::Key mTemporaryMleKey;

    uint32_t mMleFrameCounter;
    uint32_t mStoredMacFrameCounter;
    uint32_t mStoredMleFrameCounter;

    uint32_t   mHoursSinceKeyRotation;
    uint32_t   mKeyRotationTime;
    uint32_t   mKeySwitchGuardTime;
    bool       mKeySwitchGuardEnabled;
    TimerMilli mKeyRotationTimer;

#if OPENTHREAD_MTD || OPENTHREAD_FTD
    Pskc mPskc;
#endif
    Kek      mKek;
    uint32_t mKekFrameCounter;

    uint8_t mSecurityPolicyFlags;
    bool    mIsPskcSet : 1;
};

/**
 * @}
 */

} // namespace ot

#endif // KEY_MANAGER_HPP_
