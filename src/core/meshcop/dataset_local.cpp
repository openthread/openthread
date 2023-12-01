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
#include "common/locator_getters.hpp"
#include "common/log.hpp"
#include "common/settings.hpp"
#include "crypto/storage.hpp"
#include "instance/instance.hpp"
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
    Error error;

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
        uint32_t elapsed;
        uint32_t delayTimer;
        Tlv     *tlv = aDataset.FindTlv(Tlv::kDelayTimer);

        VerifyOrExit(tlv != nullptr);

        elapsed    = TimerMilli::GetNow() - mUpdateTime;
        delayTimer = tlv->ReadValueAs<DelayTimerTlv>();

        if (delayTimer > elapsed)
        {
            delayTimer -= elapsed;
        }
        else
        {
            delayTimer = 0;
        }

        tlv->WriteValueAs<DelayTimerTlv>(delayTimer);
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

const DatasetLocal::SecurelyStoredTlv DatasetLocal::kSecurelyStoredTlvs[] = {
    {
        Tlv::kNetworkKey,
        Crypto::Storage::kActiveDatasetNetworkKeyRef,
        Crypto::Storage::kPendingDatasetNetworkKeyRef,
    },
    {
        Tlv::kPskc,
        Crypto::Storage::kActiveDatasetPskcRef,
        Crypto::Storage::kPendingDatasetPskcRef,
    },
};

void DatasetLocal::DestroySecurelyStoredKeys(void) const
{
    for (const SecurelyStoredTlv &entry : kSecurelyStoredTlvs)
    {
        Crypto::Storage::DestroyKey(entry.GetKeyRef(mType));
    }
}

void DatasetLocal::MoveKeysToSecureStorage(Dataset &aDataset) const
{
    for (const SecurelyStoredTlv &entry : kSecurelyStoredTlvs)
    {
        aDataset.SaveTlvInSecureStorageAndClearValue(entry.mTlvType, entry.GetKeyRef(mType));
    }
}

void DatasetLocal::EmplaceSecurelyStoredKeys(Dataset &aDataset) const
{
    bool moveKeys = false;

    // If reading any of the TLVs fails, it indicates they are not yet
    // stored in secure storage and are still contained in the `Dataset`
    // read from `Settings`. In this case, we move the keys to secure
    // storage and then clear them from 'Settings'.

    for (const SecurelyStoredTlv &entry : kSecurelyStoredTlvs)
    {
        if (aDataset.ReadTlvFromSecureStorage(entry.mTlvType, entry.GetKeyRef(mType)) != kErrorNone)
        {
            moveKeys = true;
        }
    }

    if (moveKeys)
    {
        Dataset dataset;

        dataset.Set(GetType(), aDataset);
        MoveKeysToSecureStorage(dataset);
        SuccessOrAssert(Get<Settings>().SaveOperationalDataset(mType, dataset));
    }
}

#endif // OPENTHREAD_CONFIG_PLATFORM_KEY_REFERENCES_ENABLE

} // namespace MeshCoP
} // namespace ot
