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
#include <openthread/platform/crypto.h>

#include "common/as_core_type.hpp"
#include "common/clearable.hpp"
#include "common/encoding.hpp"
#include "common/equatable.hpp"
#include "common/locator.hpp"
#include "common/non_copyable.hpp"
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
 * Represents Security Policy Rotation and Flags.
 */
class SecurityPolicy : public otSecurityPolicy, public Equatable<SecurityPolicy>, public Clearable<SecurityPolicy>
{
public:
    /**
     * Offset between the Thread Version and the Version-threshold valid for Routing.
     */
    static constexpr uint8_t kVersionThresholdOffsetVersion = 3;

    /**
     * Default Key Rotation Time (in unit of hours).
     */
    static constexpr uint16_t kDefaultKeyRotationTime = 672;

    /**
     * Minimum Key Rotation Time (in unit of hours).
     */
    static constexpr uint16_t kMinKeyRotationTime = 2;

    /**
     * Initializes the object with default Key Rotation Time
     * and Security Policy Flags.
     */
    SecurityPolicy(void) { SetToDefault(); }

    /**
     * Sets the Security Policy to default values.
     */
    void SetToDefault(void);

    /**
     * Sets the Security Policy Flags.
     *
     * @param[in]  aFlags        The Security Policy Flags.
     * @param[in]  aFlagsLength  The length of the Security Policy Flags, 1 byte for
     *                           Thread 1.1 devices, and 2 bytes for Thread 1.2 or higher.
     */
    void SetFlags(const uint8_t *aFlags, uint8_t aFlagsLength);

    /**
     * Returns the Security Policy Flags.
     *
     * @param[out] aFlags        A pointer to the Security Policy Flags buffer.
     * @param[in]  aFlagsLength  The length of the Security Policy Flags buffer.
     */
    void GetFlags(uint8_t *aFlags, uint8_t aFlagsLength) const;

private:
    static constexpr uint8_t kDefaultFlags                   = 0xff;
    static constexpr uint8_t kObtainNetworkKeyMask           = 1 << 7;
    static constexpr uint8_t kNativeCommissioningMask        = 1 << 6;
    static constexpr uint8_t kRoutersMask                    = 1 << 5;
    static constexpr uint8_t kExternalCommissioningMask      = 1 << 4;
    static constexpr uint8_t kBeaconsMask                    = 1 << 3;
    static constexpr uint8_t kCommercialCommissioningMask    = 1 << 2;
    static constexpr uint8_t kAutonomousEnrollmentMask       = 1 << 1;
    static constexpr uint8_t kNetworkKeyProvisioningMask     = 1 << 0;
    static constexpr uint8_t kTobleLinkMask                  = 1 << 7;
    static constexpr uint8_t kNonCcmRoutersMask              = 1 << 6;
    static constexpr uint8_t kReservedMask                   = 0x38;
    static constexpr uint8_t kVersionThresholdForRoutingMask = 0x07;

    void SetToDefaultFlags(void);
};

/**
 * Represents a Thread Network Key.
 */
OT_TOOL_PACKED_BEGIN
class NetworkKey : public otNetworkKey, public Equatable<NetworkKey>, public Clearable<NetworkKey>
{
public:
    static constexpr uint8_t kSize = OT_NETWORK_KEY_SIZE; ///< Size of the Thread Network Key (in bytes).

#if !OPENTHREAD_RADIO
    /**
     * Generates a cryptographically secure random sequence to populate the Thread Network Key.
     *
     * @retval kErrorNone     Successfully generated a random Thread Network Key.
     * @retval kErrorFailed   Failed to generate random sequence.
     */
    Error GenerateRandom(void) { return Random::Crypto::Fill(*this); }
#endif

} OT_TOOL_PACKED_END;

#if OPENTHREAD_CONFIG_PLATFORM_KEY_REFERENCES_ENABLE
/**
 * Provides a representation for Network Key reference.
 */
typedef otNetworkKeyRef NetworkKeyRef;
#endif

/**
 * Represents a Thread Pre-Shared Key for the Commissioner (PSKc).
 */
OT_TOOL_PACKED_BEGIN
class Pskc : public otPskc, public Equatable<Pskc>, public Clearable<Pskc>
{
public:
    static constexpr uint8_t kSize = OT_PSKC_MAX_SIZE; ///< Size (number of bytes) of the PSKc.

#if !OPENTHREAD_RADIO
    /**
     * Generates a cryptographically secure random sequence to populate the Thread PSKc.
     *
     * @retval kErrorNone  Successfully generated a random Thread PSKc.
     */
    Error GenerateRandom(void) { return Random::Crypto::Fill(*this); }
#endif
} OT_TOOL_PACKED_END;

#if OPENTHREAD_CONFIG_PLATFORM_KEY_REFERENCES_ENABLE
/**
 * Provides a representation for Network Key reference.
 */
typedef otPskcRef PskcRef;
#endif

/**
 *
 * Represents a Key Encryption Key (KEK).
 */
typedef Mac::Key Kek;

/**
 *
 * Represents a Key Material for Key Encryption Key (KEK).
 */
typedef Mac::KeyMaterial KekKeyMaterial;

/**
 * Defines Thread Key Manager.
 */
class KeyManager : public InstanceLocator, private NonCopyable
{
public:
    /**
     * Defines bit-flag constants specifying how to handle key sequence update used in `KeySeqUpdateFlags`.
     */
    enum KeySeqUpdateFlag : uint8_t
    {
        kApplySwitchGuard    = (1 << 0), ///< Apply key switch guard check.
        kForceUpdate         = (0 << 0), ///< Ignore key switch guard check and forcibly update.
        kResetGuardTimer     = (1 << 1), ///< On key seq change, reset the guard timer.
        kGuardTimerUnchanged = (0 << 1), ///< On key seq change, leave guard timer unchanged.
    };

    /**
     * Represents a combination of `KeySeqUpdateFlag` bits.
     *
     * Used as input by `SetCurrentKeySequence()`.
     */
    typedef uint8_t KeySeqUpdateFlags;

    /**
     * Initializes the object.
     *
     * @param[in]  aInstance     A reference to the OpenThread instance.
     */
    explicit KeyManager(Instance &aInstance);

    /**
     * Starts KeyManager rotation timer and sets guard timer to initial value.
     */
    void Start(void);

    /**
     * Stops KeyManager timers.
     */
    void Stop(void);

    /**
     * Gets the Thread Network Key.
     *
     * @param[out] aNetworkKey   A reference to a `NetworkKey` to output the Thread Network Key.
     */
    void GetNetworkKey(NetworkKey &aNetworkKey) const;

    /**
     * Sets the Thread Network Key.
     *
     * @param[in]  aNetworkKey        A Thread Network Key.
     */
    void SetNetworkKey(const NetworkKey &aNetworkKey);

#if OPENTHREAD_CONFIG_PLATFORM_KEY_REFERENCES_ENABLE
    /**
     * Returns a Key Ref to Thread Network Key.
     *
     * @returns A key reference to the Thread Network Key.
     */
    NetworkKeyRef GetNetworkKeyRef(void) const { return mNetworkKeyRef; }

    /**
     * Sets the Thread Network Key using Key Reference.
     *
     * @param[in]  aKeyRef        Reference to Thread Network Key.
     */
    void SetNetworkKeyRef(NetworkKeyRef aKeyRef);
#endif

    /**
     * Indicates whether the PSKc is configured.
     *
     * A value of all zeros indicates that the PSKc is not configured.
     *
     * @retval TRUE  if the PSKc is configured.
     * @retval FALSE if the PSKc is not not configured.
     */
    bool IsPskcSet(void) const { return mIsPskcSet; }

    /**
     * Gets the PKSc.
     *
     * @param[out] aPskc  A reference to a `Pskc` to return the PSKc.
     */
    void GetPskc(Pskc &aPskc) const;

    /**
     * Sets the PSKc.
     *
     * @param[in]  aPskc    A reference to the PSKc.
     */
    void SetPskc(const Pskc &aPskc);

#if OPENTHREAD_CONFIG_PLATFORM_KEY_REFERENCES_ENABLE
    /**
     * Returns a Key Ref to PSKc.
     *
     * @returns A key reference to the PSKc.
     */
    const PskcRef &GetPskcRef(void) const { return mPskcRef; }

    /**
     * Sets the PSKc as a Key reference.
     *
     * @param[in]  aPskc    A reference to the PSKc.
     */
    void SetPskcRef(PskcRef aKeyRef);
#endif

    /**
     * Returns the current key sequence value.
     *
     * @returns The current key sequence value.
     */
    uint32_t GetCurrentKeySequence(void) const { return mKeySequence; }

    /**
     * Sets the current key sequence value.
     *
     * @param[in]  aKeySequence    The key sequence value.
     * @param[in]  aFlags          Specify behavior when updating the key sequence, i.e., whether or not to apply the
     *                             key switch guard or reset guard timer upon change.
     */
    void SetCurrentKeySequence(uint32_t aKeySequence, KeySeqUpdateFlags aFlags);

#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
    /**
     * Returns the current MAC key for TREL radio link.
     *
     * @returns The current TREL MAC key.
     */
    const Mac::KeyMaterial &GetCurrentTrelMacKey(void) const { return mTrelKey; }

    /**
     * Returns a temporary MAC key for TREL radio link computed from the given key sequence.
     *
     * @param[in]  aKeySequence  The key sequence value.
     *
     * @returns The temporary TREL MAC key.
     */
    const Mac::KeyMaterial &GetTemporaryTrelMacKey(uint32_t aKeySequence);
#endif

    /**
     * Returns the current MLE key Material.
     *
     * @returns The current MLE key.
     */
    const Mle::KeyMaterial &GetCurrentMleKey(void) const { return mMleKey; }

    /**
     * Returns a temporary MLE key Material computed from the given key sequence.
     *
     * @param[in]  aKeySequence  The key sequence value.
     *
     * @returns The temporary MLE key.
     */
    const Mle::KeyMaterial &GetTemporaryMleKey(uint32_t aKeySequence);

    /**
     * Returns a temporary MAC key Material computed from the given key sequence.
     *
     * @param[in]  aKeySequence  The key sequence value.
     *
     * @returns The temporary MAC key.
     */
    const Mle::KeyMaterial &GetTemporaryMacKey(uint32_t aKeySequence);

#if OPENTHREAD_CONFIG_RADIO_LINK_IEEE_802_15_4_ENABLE
    /**
     * Returns the current MAC Frame Counter value for 15.4 radio link.
     *
     * @returns The current MAC Frame Counter value.
     */
    uint32_t Get154MacFrameCounter(void) const { return mMacFrameCounters.Get154(); }
#endif

#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
    /**
     * Returns the current MAC Frame Counter value for TREL radio link.
     *
     * @returns The current MAC Frame Counter value for TREL radio link.
     */
    uint32_t GetTrelMacFrameCounter(void) const { return mMacFrameCounters.GetTrel(); }

    /**
     * Increments the current MAC Frame Counter value for TREL radio link.
     */
    void IncrementTrelMacFrameCounter(void);
#endif

    /**
     * Gets the maximum MAC Frame Counter among all supported radio links.
     *
     * @return The maximum MAC frame Counter among all supported radio links.
     */
    uint32_t GetMaximumMacFrameCounter(void) const { return mMacFrameCounters.GetMaximum(); }

    /**
     * Sets the current MAC Frame Counter value for all radio links.
     *
     * @param[in] aFrameCounter  The MAC Frame Counter value.
     * @param[in] aSetIfLarger   If `true`, set only if the new value @p aFrameCounter is larger than current value.
     *                           If `false`, set the new value independent of the current value.

     */
    void SetAllMacFrameCounters(uint32_t aFrameCounter, bool aSetIfLarger);

    /**
     * Sets the MAC Frame Counter value which is stored in non-volatile memory.
     *
     * @param[in]  aStoredMacFrameCounter  The stored MAC Frame Counter value.
     */
    void SetStoredMacFrameCounter(uint32_t aStoredMacFrameCounter) { mStoredMacFrameCounter = aStoredMacFrameCounter; }

    /**
     * Returns the current MLE Frame Counter value.
     *
     * @returns The current MLE Frame Counter value.
     */
    uint32_t GetMleFrameCounter(void) const { return mMleFrameCounter; }

    /**
     * Sets the current MLE Frame Counter value.
     *
     * @param[in]  aMleFrameCounter  The MLE Frame Counter value.
     */
    void SetMleFrameCounter(uint32_t aMleFrameCounter) { mMleFrameCounter = aMleFrameCounter; }

    /**
     * Sets the MLE Frame Counter value which is stored in non-volatile memory.
     *
     * @param[in]  aStoredMleFrameCounter  The stored MLE Frame Counter value.
     */
    void SetStoredMleFrameCounter(uint32_t aStoredMleFrameCounter) { mStoredMleFrameCounter = aStoredMleFrameCounter; }

    /**
     * Increments the current MLE Frame Counter value.
     */
    void IncrementMleFrameCounter(void);

    /**
     * Returns the KEK as `KekKeyMaterial`
     *
     * @returns The KEK as `KekKeyMaterial`.
     */
    const KekKeyMaterial &GetKek(void) const { return mKek; }

    /**
     * Retrieves the KEK as literal `Kek` key.
     *
     * @param[out] aKek  A reference to a `Kek` to output the retrieved KEK.
     */
    void ExtractKek(Kek &aKek) { mKek.ExtractKey(aKek); }

    /**
     * Sets the KEK.
     *
     * @param[in]  aKek  A KEK.
     */
    void SetKek(const Kek &aKek);

    /**
     * Sets the KEK.
     *
     * @param[in]  aKekBytes  A pointer to the KEK bytes.
     */
    void SetKek(const uint8_t *aKekBytes) { SetKek(*reinterpret_cast<const Kek *>(aKekBytes)); }

    /**
     * Returns the current KEK Frame Counter value.
     *
     * @returns The current KEK Frame Counter value.
     */
    uint32_t GetKekFrameCounter(void) const { return mKekFrameCounter; }

    /**
     * Increments the current KEK Frame Counter value.
     */
    void IncrementKekFrameCounter(void) { mKekFrameCounter++; }

    /**
     * Returns the KeySwitchGuardTime.
     *
     * The KeySwitchGuardTime is the time interval during which key rotation procedure is prevented.
     *
     * @returns The KeySwitchGuardTime value in hours.
     */
    uint16_t GetKeySwitchGuardTime(void) const { return mKeySwitchGuardTime; }

    /**
     * Sets the KeySwitchGuardTime.
     *
     * The KeySwitchGuardTime is the time interval during which key rotation procedure is prevented.
     *
     * Intended for testing only. Changing the guard time will render device non-compliant with the Thread spec.
     *
     * @param[in]  aGuardTime  The KeySwitchGuardTime value in hours.
     */
    void SetKeySwitchGuardTime(uint16_t aGuardTime) { mKeySwitchGuardTime = aGuardTime; }

    /**
     * Returns the Security Policy.
     *
     * The Security Policy specifies Key Rotation Time and network administrator preferences
     * for which security-related operations are allowed or disallowed.
     *
     * @returns The SecurityPolicy.
     */
    const SecurityPolicy &GetSecurityPolicy(void) const { return mSecurityPolicy; }

    /**
     * Sets the Security Policy.
     *
     * The Security Policy specifies Key Rotation Time and network administrator preferences
     * for which security-related operations are allowed or disallowed.
     *
     * @param[in]  aSecurityPolicy  The Security Policy.
     */
    void SetSecurityPolicy(const SecurityPolicy &aSecurityPolicy);

    /**
     * Updates the MAC keys and MLE key.
     */
    void UpdateKeyMaterial(void);

    /**
     * Handles MAC frame counter changes (callback from `SubMac` for 15.4 security frame change).
     *
     * This is called to indicate the @p aMacFrameCounter value is now used.
     *
     * @param[in]  aMacFrameCounter     The 15.4 link MAC frame counter value.
     */
    void MacFrameCounterUsed(uint32_t aMacFrameCounter);

#if OPENTHREAD_CONFIG_PLATFORM_KEY_REFERENCES_ENABLE
    /**
     * Destroys all the volatile mac keys stored in PSA ITS.
     */
    void DestroyTemporaryKeys(void);

    /**
     * Destroys all the persistent keys stored in PSA ITS.
     */
    void DestroyPersistentKeys(void);
#endif

private:
    static constexpr uint16_t kDefaultKeySwitchGuardTime    = 624; // ~ 93% of 672 (default key rotation time)
    static constexpr uint32_t kKeySwitchGuardTimePercentage = 93;  // Percentage of key rotation time.
    static constexpr bool     kExportableMacKeys            = OPENTHREAD_CONFIG_PLATFORM_MAC_KEYS_EXPORTABLE_ENABLE;

    static_assert(kDefaultKeySwitchGuardTime ==
                      SecurityPolicy::kDefaultKeyRotationTime * kKeySwitchGuardTimePercentage / 100,
                  "Default key switch guard time value is not correct");

    OT_TOOL_PACKED_BEGIN
    struct Keys
    {
        Mle::Key mMleKey;
        Mac::Key mMacKey;
    } OT_TOOL_PACKED_END;

    union HashKeys
    {
        Crypto::HmacSha256::Hash mHash;
        Keys                     mKeys;

        const Mle::Key &GetMleKey(void) const { return mKeys.mMleKey; }
        const Mac::Key &GetMacKey(void) const { return mKeys.mMacKey; }
    };

    void ComputeKeys(uint32_t aKeySequence, HashKeys &aHashKeys) const;

#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
    void ComputeTrelKey(uint32_t aKeySequence, Mac::Key &aKey) const;
#endif

    void ResetKeyRotationTimer(void);
    void HandleKeyRotationTimer(void);
    void CheckForKeyRotation(void);

#if OPENTHREAD_CONFIG_PLATFORM_KEY_REFERENCES_ENABLE
    void StoreNetworkKey(const NetworkKey &aNetworkKey, bool aOverWriteExisting);
    void StorePskc(const Pskc &aPskc);
#endif

    void ResetFrameCounters(void);

    using RotationTimer = TimerMilliIn<KeyManager, &KeyManager::HandleKeyRotationTimer>;

    static const uint8_t kThreadString[];

#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
    static const uint8_t kHkdfExtractSaltString[];
    static const uint8_t kTrelInfoString[];
#endif

#if OPENTHREAD_CONFIG_PLATFORM_KEY_REFERENCES_ENABLE
    NetworkKeyRef mNetworkKeyRef;
#else
    NetworkKey mNetworkKey;
#endif

    uint32_t         mKeySequence;
    Mle::KeyMaterial mMleKey;
    Mle::KeyMaterial mTemporaryMleKey;

#if OPENTHREAD_CONFIG_WAKEUP_END_DEVICE_ENABLE
    Mle::KeyMaterial mTemporaryMacKey;
#endif

#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
    Mac::KeyMaterial mTrelKey;
    Mac::KeyMaterial mTemporaryTrelKey;
#endif

    Mac::LinkFrameCounters mMacFrameCounters;
    uint32_t               mMleFrameCounter;
    uint32_t               mStoredMacFrameCounter;
    uint32_t               mStoredMleFrameCounter;

    uint16_t      mHoursSinceKeyRotation;
    uint16_t      mKeySwitchGuardTime;
    uint16_t      mKeySwitchGuardTimer;
    RotationTimer mKeyRotationTimer;

#if OPENTHREAD_CONFIG_PLATFORM_KEY_REFERENCES_ENABLE
    PskcRef mPskcRef;
#else
    Pskc       mPskc;
#endif

    KekKeyMaterial mKek;
    uint32_t       mKekFrameCounter;

    SecurityPolicy mSecurityPolicy;
    bool           mIsPskcSet : 1;
};

/**
 * @}
 */

DefineCoreType(otSecurityPolicy, SecurityPolicy);
DefineCoreType(otNetworkKey, NetworkKey);
DefineCoreType(otPskc, Pskc);

} // namespace ot

#endif // KEY_MANAGER_HPP_
