/*
 *  Copyright (c) 2016-2017, The OpenThread Authors.
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
 *   This file implements common methods for manipulating MeshCoP Datasets.
 *
 */

#include "dataset_local.hpp"

#include <stdio.h>

#include "common/code_utils.hpp"
#include "common/instance.hpp"
#include "common/locator_getters.hpp"
#include "common/log.hpp"
#include "common/settings.hpp"
#include "crypto/storage.hpp"
#include "meshcop/dataset.hpp"
#include "meshcop/meshcop_tlvs.hpp"
#include "thread/mle_tlvs.hpp"

namespace ot {
namespace MeshCoP {

RegisterLogModule("DatasetLocal");

DatasetLocal::DatasetLocal(Instance &aInstance, Dataset::Type aType)
    : InstanceLocator(aInstance)
    , mUpdateTime(0)
    , mType(aType)
    , mTimestampPresent(false)
    , mSaved(false)
{
    mTimestamp.Clear();
}

void DatasetLocal::Clear(void)
{
#if OPENTHREAD_CONFIG_PLATFORM_KEY_REFERENCES_ENABLE
    DestroySecurelyStoredKeys();
#endif
    IgnoreError(Get<Settings>().DeleteOperationalDataset(mType));
    mTimestamp.Clear();
    mTimestampPresent = false;
    mSaved            = false;
}

Error DatasetLocal::Restore(Dataset &aDataset)
{
    Error error;

    mTimestampPresent = false;

    error = Read(aDataset);
    SuccessOrExit(error);

    mSaved            = true;
    mTimestampPresent = (aDataset.GetTimestamp(mType, mTimestamp) == kErrorNone);

exit:
    return error;
}

Error DatasetLocal::Read(Dataset &aDataset) const
{
    DelayTimerTlv *delayTimer;
    uint32_t       elapsed;
    Error          error;

    error = Get<Settings>().ReadOperationalDataset(mType, aDataset);
    VerifyOrExit(error == kErrorNone, aDataset.mLength = 0);

#if OPENTHREAD_CONFIG_PLATFORM_KEY_REFERENCES_ENABLE
    EmplaceSecurelyStoredKeys(aDataset);
#endif

    if (mType == Dataset::kActive)
    {
        aDataset.RemoveTlv(Tlv::kPendingTimestamp);
        aDataset.RemoveTlv(Tlv::kDelayTimer);
    }
    else
    {
        delayTimer = aDataset.GetTlv<DelayTimerTlv>();
        VerifyOrExit(delayTimer);

        elapsed = TimerMilli::GetNow() - mUpdateTime;

        if (delayTimer->GetDelayTimer() > elapsed)
        {
            delayTimer->SetDelayTimer(delayTimer->GetDelayTimer() - elapsed);
        }
        else
        {
            delayTimer->SetDelayTimer(0);
        }
    }

    aDataset.mUpdateTime = TimerMilli::GetNow();

exit:
    return error;
}

Error DatasetLocal::Read(Dataset::Info &aDatasetInfo) const
{
    Dataset dataset;
    Error   error;

    aDatasetInfo.Clear();

    SuccessOrExit(error = Read(dataset));
    dataset.ConvertTo(aDatasetInfo);

exit:
    return error;
}

Error DatasetLocal::Read(otOperationalDatasetTlvs &aDataset) const
{
    Dataset dataset;
    Error   error;

    memset(&aDataset, 0, sizeof(aDataset));

    SuccessOrExit(error = Read(dataset));
    dataset.ConvertTo(aDataset);

exit:
    return error;
}

Error DatasetLocal::Save(const Dataset::Info &aDatasetInfo)
{
    Error   error;
    Dataset dataset;

    SuccessOrExit(error = dataset.SetFrom(aDatasetInfo));
    SuccessOrExit(error = Save(dataset));

exit:
    return error;
}

Error DatasetLocal::Save(const otOperationalDatasetTlvs &aDataset)
{
    Dataset dataset;

    dataset.SetFrom(aDataset);

    return Save(dataset);
}

Error DatasetLocal::Save(const Dataset &aDataset)
{
    Error error = kErrorNone;

#if OPENTHREAD_CONFIG_PLATFORM_KEY_REFERENCES_ENABLE
    DestroySecurelyStoredKeys();
#endif

    if (aDataset.GetSize() == 0)
    {
        // do not propagate error back
        IgnoreError(Get<Settings>().DeleteOperationalDataset(mType));
        mSaved = false;
        LogInfo("%s dataset deleted", Dataset::TypeToString(mType));
    }
    else
    {
#if OPENTHREAD_CONFIG_PLATFORM_KEY_REFERENCES_ENABLE
        // Store the network key and PSKC in the secure storage instead of settings.
        Dataset dataset;

        dataset.Set(GetType(), aDataset);
        MoveKeysToSecureStorage(dataset);
        SuccessOrExit(error = Get<Settings>().SaveOperationalDataset(mType, dataset));
#else
        SuccessOrExit(error = Get<Settings>().SaveOperationalDataset(mType, aDataset));
#endif

        mSaved = true;
        LogInfo("%s dataset set", Dataset::TypeToString(mType));
    }

    mTimestampPresent = (aDataset.GetTimestamp(mType, mTimestamp) == kErrorNone);
    mUpdateTime       = TimerMilli::GetNow();

exit:
    return error;
}

#if OPENTHREAD_CONFIG_PLATFORM_KEY_REFERENCES_ENABLE
void DatasetLocal::DestroySecurelyStoredKeys(void) const
{
    using namespace Crypto::Storage;

    KeyRef networkKeyRef = IsActive() ? kActiveDatasetNetworkKeyRef : kPendingDatasetNetworkKeyRef;
    KeyRef pskcRef       = IsActive() ? kActiveDatasetPskcRef : kPendingDatasetPskcRef;

    // Destroy securely stored keys associated with the given operational dataset type.
    DestroyKey(networkKeyRef);
    DestroyKey(pskcRef);
}

void DatasetLocal::MoveKeysToSecureStorage(Dataset &aDataset) const
{
    using namespace Crypto::Storage;

    KeyRef         networkKeyRef = IsActive() ? kActiveDatasetNetworkKeyRef : kPendingDatasetNetworkKeyRef;
    KeyRef         pskcRef       = IsActive() ? kActiveDatasetPskcRef : kPendingDatasetPskcRef;
    NetworkKeyTlv *networkKeyTlv = aDataset.GetTlv<NetworkKeyTlv>();
    PskcTlv *      pskcTlv       = aDataset.GetTlv<PskcTlv>();

    if (networkKeyTlv != nullptr)
    {
        // If the dataset contains a network key, put it in the secure storage
        // and zero the corresponding TLV element.
        NetworkKey networkKey;
        SuccessOrAssert(ImportKey(networkKeyRef, kKeyTypeRaw, kKeyAlgorithmVendor, kUsageExport, kTypePersistent,
                                  networkKeyTlv->GetNetworkKey().m8, NetworkKey::kSize));
        networkKey.Clear();
        networkKeyTlv->SetNetworkKey(networkKey);
    }

    if (pskcTlv != nullptr)
    {
        // If the dataset contains a PSKC, put it in the secure storage and zero
        // the corresponding TLV element.
        Pskc pskc;
        SuccessOrAssert(ImportKey(pskcRef, kKeyTypeRaw, kKeyAlgorithmVendor, kUsageExport, kTypePersistent,
                                  pskcTlv->GetPskc().m8, Pskc::kSize));
        pskc.Clear();
        pskcTlv->SetPskc(pskc);
    }
}

void DatasetLocal::EmplaceSecurelyStoredKeys(Dataset &aDataset) const
{
    using namespace Crypto::Storage;

    KeyRef         networkKeyRef = IsActive() ? kActiveDatasetNetworkKeyRef : kPendingDatasetNetworkKeyRef;
    KeyRef         pskcRef       = IsActive() ? kActiveDatasetPskcRef : kPendingDatasetPskcRef;
    NetworkKeyTlv *networkKeyTlv = aDataset.GetTlv<NetworkKeyTlv>();
    PskcTlv *      pskcTlv       = aDataset.GetTlv<PskcTlv>();
    bool           moveKeys      = false;
    size_t         keyLen;
    Error          error;

    if (networkKeyTlv != nullptr)
    {
        // If the dataset contains a network key, its real value must have been moved to
        // the secure storage upon saving the dataset, so restore it back now.
        NetworkKey networkKey;
        error = ExportKey(networkKeyRef, networkKey.m8, NetworkKey::kSize, keyLen);

        if (error != kErrorNone)
        {
            // If ExportKey fails, key is not in secure storage and is stored in settings
            moveKeys = true;
        }
        else
        {
            OT_ASSERT(keyLen == NetworkKey::kSize);
            networkKeyTlv->SetNetworkKey(networkKey);
        }
    }

    if (pskcTlv != nullptr)
    {
        // If the dataset contains a PSKC, its real value must have been moved to
        // the secure storage upon saving the dataset, so restore it back now.
        Pskc pskc;
        error = ExportKey(pskcRef, pskc.m8, Pskc::kSize, keyLen);

        if (error != kErrorNone)
        {
            // If ExportKey fails, key is not in secure storage and is stored in settings
            moveKeys = true;
        }
        else
        {
            OT_ASSERT(keyLen == Pskc::kSize);
            pskcTlv->SetPskc(pskc);
        }
    }

    if (moveKeys)
    {
        // Clear the networkkey and Pskc stored in the settings and move them to secure storage.
        // Store the network key and PSKC in the secure storage instead of settings.
        Dataset dataset;

        dataset.Set(GetType(), aDataset);
        MoveKeysToSecureStorage(dataset);
        SuccessOrAssert(error = Get<Settings>().SaveOperationalDataset(mType, dataset));
    }
}
#endif

} // namespace MeshCoP
} // namespace ot
