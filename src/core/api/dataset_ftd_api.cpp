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

#include  "openthread_enable_defines.h"

#if OPENTHREAD_FTD

#include "openthread/dataset_ftd.h"

#include "openthread-instance.h"

using namespace ot;

ThreadError otDatasetSetActive(otInstance *aInstance, const otOperationalDataset *aDataset)
{
    ThreadError error;

    VerifyOrExit(aDataset != NULL, error = kThreadError_InvalidArgs);

    error = aInstance->mThreadNetif.GetActiveDataset().Set(*aDataset);

exit:
    return error;
}

ThreadError otDatasetSetPending(otInstance *aInstance, const otOperationalDataset *aDataset)
{
    ThreadError error;

    VerifyOrExit(aDataset != NULL, error = kThreadError_InvalidArgs);

    error = aInstance->mThreadNetif.GetPendingDataset().Set(*aDataset);

exit:
    return error;
}

ThreadError otDatasetSendMgmtActiveGet(otInstance *aInstance, const uint8_t *aTlvTypes, uint8_t aLength,
                                       const otIp6Address *aAddress)
{
    return aInstance->mThreadNetif.GetActiveDataset().SendGetRequest(aTlvTypes, aLength, aAddress);
}

ThreadError otDatasetSendMgmtActiveSet(otInstance *aInstance, const otOperationalDataset *aDataset,
                                       const uint8_t *aTlvs, uint8_t aLength)
{
    return aInstance->mThreadNetif.GetActiveDataset().SendSetRequest(*aDataset, aTlvs, aLength);
}

ThreadError otDatasetSendMgmtPendingGet(otInstance *aInstance, const uint8_t *aTlvTypes, uint8_t aLength,
                                        const otIp6Address *aAddress)
{
    return aInstance->mThreadNetif.GetPendingDataset().SendGetRequest(aTlvTypes, aLength, aAddress);
}

ThreadError otDatasetSendMgmtPendingSet(otInstance *aInstance, const otOperationalDataset *aDataset,
                                        const uint8_t *aTlvs, uint8_t aLength)
{
    return aInstance->mThreadNetif.GetPendingDataset().SendSetRequest(*aDataset, aTlvs, aLength);
}

uint32_t otDatasetGetDelayTimerMinimal(otInstance *aInstance)
{
    return aInstance->mThreadNetif.GetLeader().GetDelayTimerMinimal();
}

ThreadError otDatasetSetDelayTimerMinimal(otInstance *aInstance, uint32_t aDelayTimerMinimal)
{
    return aInstance->mThreadNetif.GetLeader().SetDelayTimerMinimal(aDelayTimerMinimal);
}

#endif  // OPENTHREAD_FTD
