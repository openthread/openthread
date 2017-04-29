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
 *   This file implements the OpenThread Operational Dataset API (for both FTD and MTD).
 */

#ifdef OPENTHREAD_CONFIG_FILE
#include OPENTHREAD_CONFIG_FILE
#else
#include <openthread-config.h>
#endif

#include "openthread/dataset.h"

#include "openthread-instance.h"

using namespace ot;

bool otDatasetIsCommissioned(otInstance *aInstance)
{
    otOperationalDataset dataset;

    otDatasetGetActive(aInstance, &dataset);

    if ((dataset.mIsMasterKeySet) && (dataset.mIsNetworkNameSet) &&
        (dataset.mIsExtendedPanIdSet) && (dataset.mIsPanIdSet) && (dataset.mIsChannelSet))
    {
        return true;
    }

    return false;
}

ThreadError otDatasetGetActive(otInstance *aInstance, otOperationalDataset *aDataset)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(aDataset != NULL, error = kThreadError_InvalidArgs);

    aInstance->mThreadNetif.GetActiveDataset().GetLocal().Get(*aDataset);

exit:
    return error;
}

ThreadError otDatasetGetPending(otInstance *aInstance, otOperationalDataset *aDataset)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(aDataset != NULL, error = kThreadError_InvalidArgs);

    aInstance->mThreadNetif.GetPendingDataset().GetLocal().Get(*aDataset);

exit:
    return error;
}
