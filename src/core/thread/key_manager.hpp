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
 * This class represents Security Policy Rotation and Flags.
 *
 */
class SecurityPolicy : public otSecurityPolicy, public Equatable<SecurityPolicy>
{
public:
    enum : uint8_t
    {
        kVersionThresholdOffsetVersion = 3, ///< Offset between the Thread Version and the
                                            ///< Version-thresold valid for Routing.
    };

    enum : uint16_t
    {
        kMinKeyRotationTime     = 1,   ///< The minimum Key Rotation Time in hours.
        kDefaultKeyRotationTime = 672, ///< Default Key Rotation Time (in unit of hours).
    };

    /**
     * This constructor initializes the object with default Key Rotation Time
     * and Security Policy Flags.
     *
     */
    SecurityPolicy(void) { SetToDefault(); }

    /**
     * This method sets the Security Policy to default values.
     *
     */
    void SetToDefault(void);

    /**
     * This method sets the Security Policy Flags.
     *
     * @param[in]  aFlags        The Security Policy Flags.
     * @param[in]  aFlagsLength  The length of the Security Policy Flags, 1 byte for
     *                           Thread 1.1 devices, and 2 bytes for Thread 1.2 or higher.
     *
     */
    void SetFlags(const uint8_t *aFlags, uint8_t aFlagsLength);

    /**
     * This method returns the Security Policy Flags.
     *
     * @param[out] aFlags        A pointer to the Security Policy Flags buffer.
     * @param[in]  aFlagsLength  The length of the Security Policy Flags buffer.
     *
     */
    void GetFlags(uint8_t *aFlags, uint8_t aFlagsLength) const;

private:
    enum : uint8_t
    {
        kDefaultFlags                   = 0xff,
        kObtainNetworkKeyMask           = 1 << 7,
        kNativeCommissioningMask        = 1 << 6,
        kRoutersMask                    = 1 << 5,
        kExternalCommissioningMask      = 1 << 4,
        kBeaconsMask                    = 1 << 3,
        kCommercialCommissioningMask    = 1 << 2,
        kAutonomousEnrollmentMask       = 1 << 1,
        kNetworkKeyProvisioningMask     = 1 << 0,
        kTobleLinkMask                  = 1 << 7,
        kNonCcmRoutersMask              = 1 << 6,
        kReservedMask                   = 0x38,
        kVersionThresholdForRoutingMask = 0x07,
    };

    void SetToDefaultFlags(void);
};

/**
 * This class represents a Thread Network Key.
 *
 */
OT_TOOL_PACKED_BEGIN
class NetworkKey : public otNetworkKey, public Equatable<NetworkKey>
{
public:
#if !OPENTHREAD_RADIO
    /**
     * This method generates a cryptographically secure random sequence to populate the Thread Network Key.
     *
     * @retval kErrorNone     Successfully generated a random Thread Network Key.
     * @retval kErrorFailed   Failed to generate random sequence.
     *
     */
    Error GenerateRandom(void) { return Random::Crypto::FillBuffer(m8, sizeof(m8)); }
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
     * @retval kErrorNone  Successfully generated a random Thread PSKc.
     *
     */
    Error GenerateRandom(void) { return Random::Crypto::FillBuffer(m8, sizeof(Pskc)); }
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
class KeyManager : public InstanceLocator, private NonCopyable
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
     * This method returns the Thread Network Key.
     *
     * @returns The Thread Network Key.
     *
     */
    const NetworkKey &GetNetworkKey(void) const { return mNetworkKey; }

    /**
     * This method sets the Thread Network Key.
     *
     * @param[in]  aKey        A Thread Network Key.
     *
     * @retval kErrorNone         Successfully set the Thread Network Key.
     * @retval kErrorInvalidArgs  The @p aKeyLength value was invalid.
     *
     */
    Error SetNetworkKey(const NetworkKey &aKey);

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

#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
    /**
     * This method returns the current MAC key for TREL radio link.
     *
     * @returns The current TREL MAC key.
     *
     */
    const Mac::Key &GetCurrentTrelMacKey(void) const { return mTrelKey; }

    /**
     * This method returns a temporary MAC key for TREL radio link computed from the given key sequence.
     *
     * @param[in]  aKeySequence  The key sequence value.
     *
     * @returns The temporary TREL MAC key.
     *
     */
    const Mac::Key &GetTemporaryTrelMacKey(uint32_t aKeySequence);
#endif

    /**
     * This method returns the current MLE key.
     *
     * @returns The current MLE key.
     *
     */
    const Mle::Key &GetCurrentMleKey(void) const { return mMleKey; }

    /**
     * This method returns a temporary MLE key computed from the given key sequence.
     *
     * @param[in]  aKeySequence  The key sequence value.
     *
     * @returns The temporary MLE key.
     *
     */
    const Mle::Key &GetTemporaryMleKey(uint32_t aKeySequence);

#if OPENTHREAD_CONFIG_RADIO_LINK_IEEE_802_15_4_ENABLE
    /**
     * This method returns the current MAC Frame Counter value for 15.4 radio link.
     *
     * @returns The current MAC Frame Counter value.
     *
     */
    uint32_t Get154MacFrameCounter(void) const { return mMacFrameCounters.Get154(); }
#endif

#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
    /**
     * This method returns the current MAC Frame Counter value for TREL radio link.
     *
     * @returns The current MAC Frame Counter value for TREL radio link.
     *
     */
    uint32_t GetTrelMacFrameCounter(void) const { return mMacFrameCounters.GetTrel(); }

    /**
     * This method increments the current MAC Frame Counter value for TREL radio link.
     *
     */
    void IncrementTrelMacFrameCounter(void);
#endif

    /**
     * This method gets the maximum MAC Frame Counter among all supported radio links.
     *
     * @return The maximum MAC frame Counter among all supported radio links.
     *
     */
    uint32_t GetMaximumMacFrameCounter(void) const { return mMacFrameCounters.GetMaximum(); }

    /**
     * This method sets the current MAC Frame Counter value for all radio links.
     *
     * @param[in]  aMacFrameCounter  The MAC Frame Counter value.
     *
     */
    void SetAllMacFrameCounters(uint32_t aMacFrameCounter);

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
     * This method returns the Security Policy.
     *
     * The Security Policy specifies Key Rotation Time and network administrator preferences
     * for which security-related operations are allowed or disallowed.
     *
     * @returns The SecurityPolicy.
     *
     */
    const SecurityPolicy &GetSecurityPolicy(void) const { return mSecurityPolicy; }

    /**
     * This method sets the Security Policy.
     *
     * The Security Policy specifies Key Rotation Time and network administrator preferences
     * for which security-related operations are allowed or disallowed.
     *
     * @param[in]  aSecurityPolicy  The Security Policy.
     *
     */
    void SetSecurityPolicy(const SecurityPolicy &aSecurityPolicy);

    /**
     * This method updates the MAC keys and MLE key.
     *
     */
    void UpdateKeyMaterial(void);

    /**
     * This method handles MAC frame counter change (callback from `SubMac` for 15.4 security frame change)
     *
     * @param[in]  aMacFrameCounter  The 15.4 link MAC frame counter value.
     *
     */
    void MacFrameCounterUpdated(uint32_t aMacFrameCounter);

private:
    enum
    {
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
        Crypto::HmacSha256::Hash mHash;
        Keys                     mKeys;
    };

    void ComputeKeys(uint32_t aKeySequence, HashKeys &aHashKeys);

#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
    void ComputeTrelKey(uint32_t aKeySequence, Mac::Key &aTrelKey);
#endif

    void        StartKeyRotationTimer(void);
    static void HandleKeyRotationTimer(Timer &aTimer);
    void        HandleKeyRotationTimer(void);

    static const uint8_t kThreadString[];

#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
    static const uint8_t kHkdfExtractSaltString[];
    static const uint8_t kTrelInfoString[];
#endif

    NetworkKey mNetworkKey;

    uint32_t mKeySequence;
    Mle::Key mMleKey;
    Mle::Key mTemporaryMleKey;

#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
    Mac::Key mTrelKey;
    Mac::Key mTemporaryTrelKey;
#endif

    Mac::LinkFrameCounters mMacFrameCounters;
    uint32_t               mMleFrameCounter;
    uint32_t               mStoredMacFrameCounter;
    uint32_t               mStoredMleFrameCounter;

    uint32_t   mHoursSinceKeyRotation;
    uint32_t   mKeySwitchGuardTime;
    bool       mKeySwitchGuardEnabled;
    TimerMilli mKeyRotationTimer;

#if OPENTHREAD_MTD || OPENTHREAD_FTD
    Pskc mPskc;
#endif
    Kek      mKek;
    uint32_t mKekFrameCounter;

    SecurityPolicy mSecurityPolicy;
    bool           mIsPskcSet : 1;
};

/**
 * @}
 */

} // namespace ot

#endif // KEY_MANAGER_HPP_
