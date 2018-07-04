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
 *   This file implements the OpenThread Operational Dataset API (FTD only).
 */

#include "openthread-core-config.h"

#if OPENTHREAD_FTD

#include <openthread/dataset_ftd.h>

#include "common/instance.hpp"

using namespace ot;

otError otDatasetSetActive(otInstance *aInstance, const otOperationalDataset *aDataset)
{
    otError   error;
    Instance &instance = *static_cast<Instance *>(aInstance);

    VerifyOrExit(aDataset != NULL, error = OT_ERROR_INVALID_ARGS);

    error = instance.GetThreadNetif().GetActiveDataset().Set(*aDataset);

exit:
    return error;
}

otError otDatasetSetPending(otInstance *aInstance, const otOperationalDataset *aDataset)
{
    otError   error;
    Instance &instance = *static_cast<Instance *>(aInstance);

    VerifyOrExit(aDataset != NULL, error = OT_ERROR_INVALID_ARGS);

    error = instance.GetThreadNetif().GetPendingDataset().Set(*aDataset);

exit:
    return error;
}

otError otDatasetSendMgmtActiveGet(otInstance *        aInstance,
                                   const uint8_t *     aTlvTypes,
                                   uint8_t             aLength,
                                   const otIp6Address *aAddress)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.GetThreadNetif().GetActiveDataset().SendGetRequest(aTlvTypes, aLength, aAddress);
}

otError otDatasetSendMgmtActiveSet(otInstance *                aInstance,
                                   const otOperationalDataset *aDataset,
                                   const uint8_t *             aTlvs,
                                   uint8_t                     aLength)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.GetThreadNetif().GetActiveDataset().SendSetRequest(*aDataset, aTlvs, aLength);
}

otError otDatasetSendMgmtPendingGet(otInstance *        aInstance,
                                    const uint8_t *     aTlvTypes,
                                    uint8_t             aLength,
                                    const otIp6Address *aAddress)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.GetThreadNetif().GetPendingDataset().SendGetRequest(aTlvTypes, aLength, aAddress);
}

otError otDatasetSendMgmtPendingSet(otInstance *                aInstance,
                                    const otOperationalDataset *aDataset,
                                    const uint8_t *             aTlvs,
                                    uint8_t                     aLength)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.GetThreadNetif().GetPendingDataset().SendSetRequest(*aDataset, aTlvs, aLength);
}

uint32_t otDatasetGetDelayTimerMinimal(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.GetThreadNetif().GetLeader().GetDelayTimerMinimal();
}

otError otDatasetSetDelayTimerMinimal(otInstance *aInstance, uint32_t aDelayTimerMinimal)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.GetThreadNetif().GetLeader().SetDelayTimerMinimal(aDelayTimerMinimal);
}

otError otDatasetUpdate(otInstance *                aInstance,
                        const otOperationalDataset *aDatasetFrom,
                        otOperationalDataset *      aDatasetTo)
{
    otError error = OT_ERROR_NONE;
    OT_UNUSED_VARIABLE(aInstance);

    VerifyOrExit(aDatasetFrom != NULL && aDatasetTo != NULL, error = OT_ERROR_INVALID_ARGS);

    if (aDatasetFrom->mIsActiveTimestampSet)
    {
        aDatasetTo->mActiveTimestamp      = aDatasetFrom->mActiveTimestamp;
        aDatasetTo->mIsActiveTimestampSet = true;
    }

    if (aDatasetFrom->mIsPendingTimestampSet)
    {
        aDatasetTo->mPendingTimestamp      = aDatasetFrom->mPendingTimestamp;
        aDatasetTo->mIsPendingTimestampSet = true;
    }

    if (aDatasetFrom->mIsMasterKeySet)
    {
        memcpy(aDatasetTo->mMasterKey.m8, aDatasetFrom->mMasterKey.m8, sizeof(aDatasetTo->mMasterKey));
        aDatasetTo->mIsMasterKeySet = true;
    }

    if (aDatasetFrom->mIsNetworkNameSet)
    {
        memcpy(aDatasetTo->mNetworkName.m8, aDatasetFrom->mNetworkName.m8, sizeof(aDatasetTo->mNetworkName));
        aDatasetTo->mIsNetworkNameSet = true;
    }

    if (aDatasetFrom->mIsExtendedPanIdSet)
    {
        memcpy(aDatasetTo->mExtendedPanId.m8, aDatasetFrom->mExtendedPanId.m8, sizeof(aDatasetTo->mExtendedPanId));
        aDatasetTo->mIsExtendedPanIdSet = true;
    }

    if (aDatasetFrom->mIsMeshLocalPrefixSet)
    {
        memcpy(aDatasetTo->mMeshLocalPrefix.m8, aDatasetFrom->mMeshLocalPrefix.m8,
               sizeof(aDatasetTo->mMeshLocalPrefix));
        aDatasetTo->mIsMeshLocalPrefixSet = true;
    }

    if (aDatasetFrom->mIsDelaySet)
    {
        aDatasetTo->mDelay      = aDatasetFrom->mDelay;
        aDatasetTo->mIsDelaySet = true;
    }

    if (aDatasetFrom->mIsPanIdSet)
    {
        aDatasetTo->mPanId      = aDatasetFrom->mPanId;
        aDatasetTo->mIsPanIdSet = true;
    }

    if (aDatasetFrom->mIsChannelSet)
    {
        aDatasetTo->mChannel      = aDatasetFrom->mChannel;
        aDatasetTo->mIsChannelSet = true;
    }

    if (aDatasetFrom->mIsPSKcSet)
    {
        memcpy(aDatasetTo->mPSKc.m8, aDatasetFrom->mPSKc.m8, sizeof(aDatasetTo->mPSKc));
        aDatasetTo->mIsPSKcSet = true;
    }

    if (aDatasetFrom->mIsSecurityPolicySet)
    {
        aDatasetTo->mSecurityPolicy.mRotationTime = aDatasetFrom->mSecurityPolicy.mRotationTime;
        aDatasetTo->mSecurityPolicy.mFlags        = aDatasetFrom->mSecurityPolicy.mFlags;
        aDatasetTo->mIsSecurityPolicySet          = true;
    }

    if (aDatasetFrom->mIsChannelMaskPage0Set)
    {
        aDatasetTo->mChannelMaskPage0      = aDatasetFrom->mChannelMaskPage0;
        aDatasetTo->mIsChannelMaskPage0Set = true;
    }

exit:
    return error;
}

#endif // OPENTHREAD_FTD
