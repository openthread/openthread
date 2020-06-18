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

#include "openthread-core-config.h"

#include <openthread/dataset.h>

#include "common/instance.hpp"
#include "common/locator-getters.hpp"
#include "meshcop/dataset_manager.hpp"
#include "meshcop/meshcop.hpp"

using namespace ot;

bool otDatasetIsCommissioned(otInstance *aInstance)
{
    otOperationalDataset dataset;
    bool                 rval = false;

    SuccessOrExit(otDatasetGetActive(aInstance, &dataset));

    rval = ((dataset.mComponents.mIsMasterKeyPresent) && (dataset.mComponents.mIsNetworkNamePresent) &&
            (dataset.mComponents.mIsExtendedPanIdPresent) && (dataset.mComponents.mIsPanIdPresent) &&
            (dataset.mComponents.mIsChannelPresent));

exit:
    return rval;
}

otError otDatasetGetActive(otInstance *aInstance, otOperationalDataset *aDataset)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    OT_ASSERT(aDataset != nullptr);

    return instance.Get<MeshCoP::ActiveDataset>().Read(*aDataset);
}

otError otDatasetGetActiveTlvs(otInstance *aInstance, otOperationalDatasetTlvs *aDataset)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    OT_ASSERT(aDataset != nullptr);

    return instance.Get<MeshCoP::ActiveDataset>().Read(*aDataset);
}

otError otDatasetSetActive(otInstance *aInstance, const otOperationalDataset *aDataset)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    OT_ASSERT(aDataset != nullptr);

    return instance.Get<MeshCoP::ActiveDataset>().Save(*aDataset);
}

otError otDatasetSetActiveTlvs(otInstance *aInstance, const otOperationalDatasetTlvs *aDataset)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    OT_ASSERT(aDataset != nullptr);

    return instance.Get<MeshCoP::ActiveDataset>().Save(*aDataset);
}

otError otDatasetGetPending(otInstance *aInstance, otOperationalDataset *aDataset)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    OT_ASSERT(aDataset != nullptr);

    return instance.Get<MeshCoP::PendingDataset>().Read(*aDataset);
}

otError otDatasetGetPendingTlvs(otInstance *aInstance, otOperationalDatasetTlvs *aDataset)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    OT_ASSERT(aDataset != nullptr);

    return instance.Get<MeshCoP::PendingDataset>().Read(*aDataset);
}

otError otDatasetSetPending(otInstance *aInstance, const otOperationalDataset *aDataset)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    OT_ASSERT(aDataset != nullptr);

    return instance.Get<MeshCoP::PendingDataset>().Save(*aDataset);
}

otError otDatasetSetPendingTlvs(otInstance *aInstance, const otOperationalDatasetTlvs *aDataset)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    OT_ASSERT(aDataset != nullptr);

    return instance.Get<MeshCoP::PendingDataset>().Save(*aDataset);
}

otError otDatasetSendMgmtActiveGet(otInstance *                          aInstance,
                                   const otOperationalDatasetComponents *aDatasetComponents,
                                   const uint8_t *                       aTlvTypes,
                                   uint8_t                               aLength,
                                   const otIp6Address *                  aAddress)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<MeshCoP::ActiveDataset>().SendGetRequest(*aDatasetComponents, aTlvTypes, aLength, aAddress);
}

otError otDatasetSendMgmtActiveSet(otInstance *                aInstance,
                                   const otOperationalDataset *aDataset,
                                   const uint8_t *             aTlvs,
                                   uint8_t                     aLength)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<MeshCoP::ActiveDataset>().SendSetRequest(*aDataset, aTlvs, aLength);
}

otError otDatasetSendMgmtPendingGet(otInstance *                          aInstance,
                                    const otOperationalDatasetComponents *aDatasetComponents,
                                    const uint8_t *                       aTlvTypes,
                                    uint8_t                               aLength,
                                    const otIp6Address *                  aAddress)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<MeshCoP::PendingDataset>().SendGetRequest(*aDatasetComponents, aTlvTypes, aLength, aAddress);
}

otError otDatasetSendMgmtPendingSet(otInstance *                aInstance,
                                    const otOperationalDataset *aDataset,
                                    const uint8_t *             aTlvs,
                                    uint8_t                     aLength)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<MeshCoP::PendingDataset>().SendSetRequest(*aDataset, aTlvs, aLength);
}

#if OPENTHREAD_FTD
otError otDatasetGeneratePskc(const char *           aPassPhrase,
                              const otNetworkName *  aNetworkName,
                              const otExtendedPanId *aExtPanId,
                              otPskc *               aPskc)
{
    return MeshCoP::GeneratePskc(aPassPhrase, *static_cast<const Mac::NetworkName *>(aNetworkName),
                                 *static_cast<const Mac::ExtendedPanId *>(aExtPanId), *static_cast<Pskc *>(aPskc));
}
#endif // OPENTHREAD_FTD
