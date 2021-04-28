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
 *   This file implements Thread security material generation.
 */

#include "key_manager.hpp"

#include "common/code_utils.hpp"
#include "common/encoding.hpp"
#include "common/instance.hpp"
#include "common/locator_getters.hpp"
#include "common/timer.hpp"
#include "crypto/hkdf_sha256.hpp"
#include "thread/mle_router.hpp"
#include "thread/thread_netif.hpp"

namespace ot {

const uint8_t KeyManager::kThreadString[] = {
    'T', 'h', 'r', 'e', 'a', 'd',
};

#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
const uint8_t KeyManager::kHkdfExtractSaltString[] = {'T', 'h', 'r', 'e', 'a', 'd', 'S', 'e', 'q', 'u', 'e', 'n',
                                                      'c', 'e', 'M', 'a', 's', 't', 'e', 'r', 'K', 'e', 'y'};

const uint8_t KeyManager::kTrelInfoString[] = {'T', 'h', 'r', 'e', 'a', 'd', 'O', 'v', 'e',
                                               'r', 'I', 'n', 'f', 'r', 'a', 'K', 'e', 'y'};
#endif

void SecurityPolicy::SetToDefault(void)
{
    mRotationTime = kDefaultKeyRotationTime;
    SetToDefaultFlags();
}

void SecurityPolicy::SetToDefaultFlags(void)
{
    mObtainMasterKeyEnabled         = true;
    mNativeCommissioningEnabled     = true;
    mRoutersEnabled                 = true;
    mExternalCommissioningEnabled   = true;
    mBeaconsEnabled                 = true;
    mCommercialCommissioningEnabled = false;
    mAutonomousEnrollmentEnabled    = false;
    mMasterKeyProvisioningEnabled   = false;
    mTobleLinkEnabled               = true;
    mNonCcmRoutersEnabled           = false;
    mVersionThresholdForRouting     = 0;
}

void SecurityPolicy::SetFlags(const uint8_t *aFlags, uint8_t aFlagsLength)
{
    OT_ASSERT(aFlagsLength > 0);

    SetToDefaultFlags();

    mObtainMasterKeyEnabled         = aFlags[0] & kObtainMasterKeyMask;
    mNativeCommissioningEnabled     = aFlags[0] & kNativeCommissioningMask;
    mRoutersEnabled                 = aFlags[0] & kRoutersMask;
    mExternalCommissioningEnabled   = aFlags[0] & kExternalCommissioningMask;
    mBeaconsEnabled                 = aFlags[0] & kBeaconsMask;
    mCommercialCommissioningEnabled = (aFlags[0] & kCommercialCommissioningMask) == 0;
    mAutonomousEnrollmentEnabled    = (aFlags[0] & kAutonomousEnrollmentMask) == 0;
    mMasterKeyProvisioningEnabled   = (aFlags[0] & kMasterKeyProvisioningMask) == 0;

    VerifyOrExit(aFlagsLength > sizeof(aFlags[0]));
    mTobleLinkEnabled           = aFlags[1] & kTobleLinkMask;
    mNonCcmRoutersEnabled       = (aFlags[1] & kNonCcmRoutersMask) == 0;
    mVersionThresholdForRouting = aFlags[1] & kVersionThresholdForRoutingMask;

exit:
    return;
}

void SecurityPolicy::GetFlags(uint8_t *aFlags, uint8_t aFlagsLength) const
{
    OT_ASSERT(aFlagsLength > 0);

    memset(aFlags, 0, aFlagsLength);

    if (mObtainMasterKeyEnabled)
    {
        aFlags[0] |= kObtainMasterKeyMask;
    }

    if (mNativeCommissioningEnabled)
    {
        aFlags[0] |= kNativeCommissioningMask;
    }

    if (mRoutersEnabled)
    {
        aFlags[0] |= kRoutersMask;
    }

    if (mExternalCommissioningEnabled)
    {
        aFlags[0] |= kExternalCommissioningMask;
    }

    if (mBeaconsEnabled)
    {
        aFlags[0] |= kBeaconsMask;
    }

    if (!mCommercialCommissioningEnabled)
    {
        aFlags[0] |= kCommercialCommissioningMask;
    }

    if (!mAutonomousEnrollmentEnabled)
    {
        aFlags[0] |= kAutonomousEnrollmentMask;
    }

    if (!mMasterKeyProvisioningEnabled)
    {
        aFlags[0] |= kMasterKeyProvisioningMask;
    }

    VerifyOrExit(aFlagsLength > sizeof(aFlags[0]));

    if (mTobleLinkEnabled)
    {
        aFlags[1] |= kTobleLinkMask;
    }

    if (!mNonCcmRoutersEnabled)
    {
        aFlags[1] |= kNonCcmRoutersMask;
    }

    aFlags[1] |= kReservedMask;
    aFlags[1] |= mVersionThresholdForRouting;

exit:
    return;
}

KeyManager::KeyManager(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mKeySequence(0)
    , mMleFrameCounter(0)
    , mStoredMacFrameCounter(0)
    , mStoredMleFrameCounter(0)
    , mHoursSinceKeyRotation(0)
    , mKeySwitchGuardTime(kDefaultKeySwitchGuardTime)
    , mKeySwitchGuardEnabled(false)
    , mKeyRotationTimer(aInstance, KeyManager::HandleKeyRotationTimer)
    , mKekFrameCounter(0)
    , mIsPskcSet(false)
{
    Error error = mMasterKey.GenerateRandom();

    OT_ASSERT(error == kErrorNone);
    OT_UNUSED_VARIABLE(error);

    mMacFrameCounters.Reset();
    mPskc.Clear();

#if OPENTHREAD_CONFIG_PSA_CRYPTO_ENABLE
    otPlatPsaInit();
    mMasterKeyRef = 0;
    StoreMasterKey(false);
#endif
}

void KeyManager::Start(void)
{
    mKeySwitchGuardEnabled = false;
    StartKeyRotationTimer();
}

void KeyManager::Stop(void)
{
    mKeyRotationTimer.Stop();
}

#if OPENTHREAD_CONFIG_PSA_CRYPTO_ENABLE

#if OPENTHREAD_MTD || OPENTHREAD_FTD
void KeyManager::SetPskc(const Pskc &aPskc)
{
    Error error = kErrorNone;

    mPskcRef = kPkscPsaItsOffset;

    CheckAndDestroyStoredKey(mPskcRef);
    IgnoreError(Get<Notifier>().Update(mPskc, aPskc, kEventPskcChanged));

    error = otPlatPsaImportKey(&mPskcRef, PSA_KEY_TYPE_RAW_DATA, PSA_ALG_VENDOR_FLAG, PSA_KEY_USAGE_EXPORT,
                               PSA_KEY_LIFETIME_PERSISTENT, mPskc.m8, OT_PSKC_MAX_SIZE);

    OT_ASSERT(error == kErrorNone);

    mPskc.Clear();
    mIsPskcSet = true;
}
#endif // OPENTHREAD_MTD || OPENTHREAD_FTD

Pskc &KeyManager::GetPskc(void)
{
    size_t aKeySize = 0;

    Error error = otPlatPsaExportKey(mPskcRef, mPskc.m8, OT_PSKC_MAX_SIZE, &aKeySize);
    OT_ASSERT(error == kErrorNone);

    return mPskc;
}

#else

#if OPENTHREAD_MTD || OPENTHREAD_FTD
void KeyManager::SetPskc(const Pskc &aPskc)
{
    IgnoreError(Get<Notifier>().Update(mPskc, aPskc, kEventPskcChanged));
    mIsPskcSet = true;
}
#endif // OPENTHREAD_MTD || OPENTHREAD_FTD
#endif

#if OPENTHREAD_CONFIG_PSA_CRYPTO_ENABLE
Error KeyManager::StoreMasterKey(bool aOverWriteExisting)
{
    Error           error     = kErrorNone;
    psa_key_usage_t mKeyUsage = PSA_KEY_USAGE_SIGN_HASH;

    mMasterKeyRef = kMasterKeyPsaItsOffset;

    if (!aOverWriteExisting)
    {
        psa_key_attributes_t mKeyAttributes;

        error = otPlatPsaGetKeyAttributes(mMasterKeyRef, &mKeyAttributes);
        // We will be able to retrieve the key_attributes only if there is
        // already a master key stored in ITS. If stored, and we are not
        // overwriting the existing key, return without doing anything.
        SuccessOrExit(error != OT_ERROR_NONE);
    }

    CheckAndDestroyStoredKey(mMasterKeyRef);

#if OPENTHREAD_FTD
    mKeyUsage |= PSA_KEY_USAGE_EXPORT;
#endif

    error = otPlatPsaImportKey(&mMasterKeyRef, PSA_KEY_TYPE_HMAC, PSA_ALG_HMAC(PSA_ALG_SHA_256), mKeyUsage,
                               PSA_KEY_LIFETIME_PERSISTENT, mMasterKey.m8, OT_MASTER_KEY_SIZE);

    OT_ASSERT(error == kErrorNone);

    mMasterKey.Clear();

exit:
    return error;
}
#endif

Error KeyManager::SetMasterKey(const MasterKey &aKey)
{
    Error   error = kErrorNone;
    Router *parent;

    SuccessOrExit(Get<Notifier>().Update(mMasterKey, aKey, kEventMasterKeyChanged));
    Get<Notifier>().Signal(kEventThreadKeySeqCounterChanged);
#if OPENTHREAD_CONFIG_PSA_CRYPTO_ENABLE
    StoreMasterKey(true);
#endif
    mKeySequence = 0;
    UpdateKeyMaterial();

    // reset parent frame counters
    parent = &Get<Mle::MleRouter>().GetParent();
    parent->SetKeySequence(0);
    parent->GetLinkFrameCounters().Reset();
    parent->SetLinkAckFrameCounter(0);
    parent->SetMleFrameCounter(0);

#if OPENTHREAD_FTD
    // reset router frame counters
    for (Router &router : Get<RouterTable>().Iterate())
    {
        router.SetKeySequence(0);
        router.GetLinkFrameCounters().Reset();
        router.SetLinkAckFrameCounter(0);
        router.SetMleFrameCounter(0);
    }

    // reset child frame counters
    for (Child &child : Get<ChildTable>().Iterate(Child::kInStateAnyExceptInvalid))
    {
        child.SetKeySequence(0);
        child.GetLinkFrameCounters().Reset();
        child.SetLinkAckFrameCounter(0);
        child.SetMleFrameCounter(0);
    }
#endif

exit:
    return error;
}

#if OPENTHREAD_CONFIG_PSA_CRYPTO_ENABLE
MasterKey &KeyManager::GetMasterKey(void)
{
    size_t aKeySize = 0;

    Error error = otPlatPsaExportKey(mMasterKeyRef, mMasterKey.m8, mKek.kSize, &aKeySize);
    OT_ASSERT(error == kErrorNone);

    return mMasterKey;
}

void KeyManager::ComputeKeys(uint32_t aKeySequence, HashKeys &aHashKeys)
{
    Crypto::HmacSha256 hmac;
    uint8_t            keySequenceBytes[sizeof(uint32_t)];

    hmac.Start(mMasterKeyRef);

    Encoding::BigEndian::WriteUint32(aKeySequence, keySequenceBytes);
    hmac.Update(keySequenceBytes);
    hmac.Update(kThreadString);

    hmac.Finish(aHashKeys.mHash);
}
#else
void KeyManager::ComputeKeys(uint32_t aKeySequence, HashKeys &aHashKeys)
{
    Crypto::HmacSha256 hmac;
    uint8_t            keySequenceBytes[sizeof(uint32_t)];

    hmac.Start(mMasterKey.m8, sizeof(mMasterKey.m8));

    Encoding::BigEndian::WriteUint32(aKeySequence, keySequenceBytes);
    hmac.Update(keySequenceBytes);
    hmac.Update(kThreadString);

    hmac.Finish(aHashKeys.mHash);
}
#endif

#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
void KeyManager::ComputeTrelKey(uint32_t aKeySequence, Mac::Key &aTrelKey)
{
    Crypto::HkdfSha256 hkdf;
    uint8_t            salt[sizeof(uint32_t) + sizeof(kHkdfExtractSaltString)];

    Encoding::BigEndian::WriteUint32(aKeySequence, salt);
    memcpy(salt + sizeof(uint32_t), kHkdfExtractSaltString, sizeof(kHkdfExtractSaltString));

    hkdf.Extract(salt, sizeof(salt), mMasterKey.m8, sizeof(MasterKey));
    hkdf.Expand(kTrelInfoString, sizeof(kTrelInfoString), aTrelKey.m8, sizeof(Mac::Key));
}
#endif

#if OPENTHREAD_CONFIG_PSA_CRYPTO_ENABLE
void KeyManager::UpdateKeyMaterial(void)
{
    HashKeys    cur;
    otMacKeyRef curMacKeyRef;
    Error       error;
#if OPENTHREAD_CONFIG_RADIO_LINK_IEEE_802_15_4_ENABLE
    HashKeys    prev;
    HashKeys    next;
    otMacKeyRef prevMacKeyRef;
    otMacKeyRef nextMacKeyRef;
#endif

    ComputeKeys(mKeySequence, cur);

    error = otPlatPsaImportKey(&curMacKeyRef, PSA_KEY_TYPE_AES, PSA_ALG_ECB_NO_PADDING,
                               (PSA_KEY_USAGE_ENCRYPT | PSA_KEY_USAGE_DECRYPT), PSA_KEY_LIFETIME_VOLATILE,
                               cur.mKeys.mMacKey.GetKey(), cur.mKeys.mMacKey.kSize);
    OT_ASSERT(error == kErrorNone);

    CheckAndDestroyStoredKey(mMleKeyRef);

    error = otPlatPsaImportKey(&mMleKeyRef, PSA_KEY_TYPE_AES, PSA_ALG_ECB_NO_PADDING,
                               (PSA_KEY_USAGE_ENCRYPT | PSA_KEY_USAGE_DECRYPT), PSA_KEY_LIFETIME_VOLATILE,
                               cur.mKeys.mMleKey.m8, cur.mKeys.mMleKey.kSize);
    OT_ASSERT(error == kErrorNone);

#if OPENTHREAD_CONFIG_RADIO_LINK_IEEE_802_15_4_ENABLE
    ComputeKeys(mKeySequence - 1, prev);
    ComputeKeys(mKeySequence + 1, next);

    error = otPlatPsaImportKey(&prevMacKeyRef, PSA_KEY_TYPE_AES, PSA_ALG_ECB_NO_PADDING,
                               (PSA_KEY_USAGE_ENCRYPT | PSA_KEY_USAGE_DECRYPT), PSA_KEY_LIFETIME_VOLATILE,
                               prev.mKeys.mMacKey.m8, prev.mKeys.mMleKey.kSize);
    OT_ASSERT(error == kErrorNone);

    error = otPlatPsaImportKey(&nextMacKeyRef, PSA_KEY_TYPE_AES, PSA_ALG_ECB_NO_PADDING,
                               (PSA_KEY_USAGE_ENCRYPT | PSA_KEY_USAGE_DECRYPT), PSA_KEY_LIFETIME_VOLATILE,
                               next.mKeys.mMleKey.m8, next.mKeys.mMleKey.kSize);
    OT_ASSERT(error == kErrorNone);

    cur.mKeys.mMacKey.Clear();
    cur.mKeys.mMleKey.Clear();
    prev.mKeys.mMacKey.Clear();
    prev.mKeys.mMacKey.Clear();
    next.mKeys.mMacKey.Clear();
    next.mKeys.mMacKey.Clear();

    Get<Mac::SubMac>().SetMacKey(Mac::Frame::kKeyIdMode1, (mKeySequence & 0x7f) + 1, prevMacKeyRef, curMacKeyRef,
                                 nextMacKeyRef);

#endif

#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
    ComputeTrelKey(mKeySequence, mTrelKey);
#endif
}
#else
void KeyManager::UpdateKeyMaterial(void)
{
    HashKeys cur;
#if OPENTHREAD_CONFIG_RADIO_LINK_IEEE_802_15_4_ENABLE
    HashKeys prev;
    HashKeys next;
#endif

    ComputeKeys(mKeySequence, cur);
    mMleKey = cur.mKeys.mMleKey;

#if OPENTHREAD_CONFIG_RADIO_LINK_IEEE_802_15_4_ENABLE
    ComputeKeys(mKeySequence - 1, prev);
    ComputeKeys(mKeySequence + 1, next);

    Get<Mac::SubMac>().SetMacKey(Mac::Frame::kKeyIdMode1, (mKeySequence & 0x7f) + 1, prev.mKeys.mMacKey,
                                 cur.mKeys.mMacKey, next.mKeys.mMacKey);
#endif

#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
    ComputeTrelKey(mKeySequence, mTrelKey);
#endif
}
#endif

void KeyManager::SetCurrentKeySequence(uint32_t aKeySequence)
{
    VerifyOrExit(aKeySequence != mKeySequence, Get<Notifier>().SignalIfFirst(kEventThreadKeySeqCounterChanged));

    if ((aKeySequence == (mKeySequence + 1)) && mKeyRotationTimer.IsRunning())
    {
        if (mKeySwitchGuardEnabled)
        {
            // Check if the guard timer has expired if key rotation is requested.
            VerifyOrExit(mHoursSinceKeyRotation >= mKeySwitchGuardTime);
            StartKeyRotationTimer();
        }

        mKeySwitchGuardEnabled = true;
    }

    mKeySequence = aKeySequence;
    UpdateKeyMaterial();

    mMacFrameCounters.Reset();
    mMleFrameCounter = 0;

    Get<Notifier>().Signal(kEventThreadKeySeqCounterChanged);

exit:
    return;
}

#if OPENTHREAD_CONFIG_PSA_CRYPTO_ENABLE
KeyRef KeyManager::GetTemporaryMleKeyRef(uint32_t aKeySequence)
{
    HashKeys hashKeys;

    ComputeKeys(aKeySequence, hashKeys);

    CheckAndDestroyStoredKey(mTemporaryMleKeyRef);
    mTemporaryMleKeyRef = 0;

    Error error = otPlatPsaImportKey(&mTemporaryMleKeyRef, PSA_KEY_TYPE_AES, PSA_ALG_ECB_NO_PADDING,
                                     (PSA_KEY_USAGE_ENCRYPT | PSA_KEY_USAGE_DECRYPT), PSA_KEY_LIFETIME_VOLATILE,
                                     hashKeys.mKeys.mMleKey.m8, hashKeys.mKeys.mMleKey.kSize);
    OT_ASSERT(error == kErrorNone);

    return mTemporaryMleKeyRef;
}
#else
const Mle::Key &KeyManager::GetTemporaryMleKey(uint32_t aKeySequence)
{
    HashKeys hashKeys;

    ComputeKeys(aKeySequence, hashKeys);
    mTemporaryMleKey = hashKeys.mKeys.mMleKey;

    return mTemporaryMleKey;
}
#endif

#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
const Mac::Key &KeyManager::GetTemporaryTrelMacKey(uint32_t aKeySequence)
{
    ComputeTrelKey(aKeySequence, mTemporaryTrelKey);

    return mTemporaryTrelKey;
}
#endif

void KeyManager::SetAllMacFrameCounters(uint32_t aMacFrameCounter)
{
    mMacFrameCounters.SetAll(aMacFrameCounter);

#if OPENTHREAD_CONFIG_RADIO_LINK_IEEE_802_15_4_ENABLE
    Get<Mac::SubMac>().SetFrameCounter(aMacFrameCounter);
#endif
}

#if OPENTHREAD_CONFIG_RADIO_LINK_IEEE_802_15_4_ENABLE
void KeyManager::MacFrameCounterUpdated(uint32_t aMacFrameCounter)
{
    mMacFrameCounters.Set154(aMacFrameCounter);

    if (mMacFrameCounters.Get154() >= mStoredMacFrameCounter)
    {
        IgnoreError(Get<Mle::MleRouter>().Store());
    }
}
#else
void KeyManager::MacFrameCounterUpdated(uint32_t)
{
}
#endif

#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
void KeyManager::IncrementTrelMacFrameCounter(void)
{
    mMacFrameCounters.IncrementTrel();

    if (mMacFrameCounters.GetTrel() >= mStoredMacFrameCounter)
    {
        IgnoreError(Get<Mle::MleRouter>().Store());
    }
}
#endif

void KeyManager::IncrementMleFrameCounter(void)
{
    mMleFrameCounter++;

    if (mMleFrameCounter >= mStoredMleFrameCounter)
    {
        IgnoreError(Get<Mle::MleRouter>().Store());
    }
}

#if OPENTHREAD_CONFIG_PSA_CRYPTO_ENABLE

void KeyManager::CheckAndDestroyStoredKey(psa_key_id_t aKeyRef)
{
    if (aKeyRef != 0)
    {
        otPlatPsaDestroyKey(aKeyRef);
    }
}

Error KeyManager::ImportKek(const uint8_t *aKey, uint8_t aKeyLen)
{
    Error           error     = kErrorNone;
    psa_key_usage_t mKeyUsage = (PSA_KEY_USAGE_ENCRYPT | PSA_KEY_USAGE_DECRYPT | PSA_KEY_USAGE_EXPORT);

    CheckAndDestroyStoredKey(mKekRef);

    error = otPlatPsaImportKey(&mKekRef, PSA_KEY_TYPE_AES, PSA_ALG_ECB_NO_PADDING, mKeyUsage, PSA_KEY_LIFETIME_VOLATILE,
                               aKey, aKeyLen);

    return error;
}

void KeyManager::SetKek(const Kek &aKek)
{
    Error error = kErrorNone;

    error            = ImportKek(aKek.m8, aKek.kSize);
    mKekFrameCounter = 0;
    OT_ASSERT(error == kErrorNone);
}

void KeyManager::SetKek(const uint8_t *aKek)
{
    Error error;

    error            = ImportKek(aKek, 16);
    mKekFrameCounter = 0;
    OT_ASSERT(error == kErrorNone);
}

const Kek &KeyManager::GetKek(void)
{
    size_t aKeySize = 0;
    Error  error    = otPlatPsaExportKey(mKekRef, mKek.m8, mKek.kSize, &aKeySize);
    OT_ASSERT(error == kErrorNone);

    return mKek;
}

#else
void KeyManager::SetKek(const Kek &aKek)
{
    mKek             = aKek;
    mKekFrameCounter = 0;
}

void KeyManager::SetKek(const uint8_t *aKek)
{
    memcpy(mKek.m8, aKek, sizeof(mKek));
    mKekFrameCounter = 0;
}
#endif

void KeyManager::SetSecurityPolicy(const SecurityPolicy &aSecurityPolicy)
{
    OT_ASSERT(aSecurityPolicy.mRotationTime >= SecurityPolicy::kMinKeyRotationTime);

    IgnoreError(Get<Notifier>().Update(mSecurityPolicy, aSecurityPolicy, kEventSecurityPolicyChanged));
}

void KeyManager::StartKeyRotationTimer(void)
{
    mHoursSinceKeyRotation = 0;
    mKeyRotationTimer.Start(kOneHourIntervalInMsec);
}

void KeyManager::HandleKeyRotationTimer(Timer &aTimer)
{
    aTimer.Get<KeyManager>().HandleKeyRotationTimer();
}

void KeyManager::HandleKeyRotationTimer(void)
{
    mHoursSinceKeyRotation++;

    // Order of operations below is important. We should restart the timer (from
    // last fire time for one hour interval) before potentially calling
    // `SetCurrentKeySequence()`. `SetCurrentKeySequence()` uses the fact that
    // timer is running to decide to check for the guard time and to reset the
    // rotation timer (and the `mHoursSinceKeyRotation`) if it updates the key
    // sequence.

    mKeyRotationTimer.StartAt(mKeyRotationTimer.GetFireTime(), kOneHourIntervalInMsec);

    if (mHoursSinceKeyRotation >= mSecurityPolicy.mRotationTime)
    {
        SetCurrentKeySequence(mKeySequence + 1);
    }
}

} // namespace ot
