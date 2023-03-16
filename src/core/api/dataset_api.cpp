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

#include "common/as_core_type.hpp"
#include "common/locator_getters.hpp"
#include "meshcop/dataset_manager.hpp"
#include "meshcop/meshcop.hpp"

using namespace ot;

bool otDatasetIsCommissioned(otInstance *aInstance)
{
    return AsCoreType(aInstance).Get<MeshCoP::ActiveDatasetManager>().IsCommissioned();
}

otError otDatasetGetActive(otInstance *aInstance, otOperationalDataset *aDataset)
{
    return AsCoreType(aInstance).Get<MeshCoP::ActiveDatasetManager>().Read(AsCoreType(aDataset));
}

otError otDatasetGetActiveTlvs(otInstance *aInstance, otOperationalDatasetTlvs *aDataset)
{
    AssertPointerIsNotNull(aDataset);

    return AsCoreType(aInstance).Get<MeshCoP::ActiveDatasetManager>().Read(*aDataset);
}

otError otDatasetSetActive(otInstance *aInstance, const otOperationalDataset *aDataset)
{
    return AsCoreType(aInstance).Get<MeshCoP::ActiveDatasetManager>().Save(AsCoreType(aDataset));
}

otError otDatasetSetActiveTlvs(otInstance *aInstance, const otOperationalDatasetTlvs *aDataset)
{
    AssertPointerIsNotNull(aDataset);

    return AsCoreType(aInstance).Get<MeshCoP::ActiveDatasetManager>().Save(*aDataset);
}

otError otDatasetGetPending(otInstance *aInstance, otOperationalDataset *aDataset)
{
    return AsCoreType(aInstance).Get<MeshCoP::PendingDatasetManager>().Read(AsCoreType(aDataset));
}

otError otDatasetGetPendingTlvs(otInstance *aInstance, otOperationalDatasetTlvs *aDataset)
{
    AssertPointerIsNotNull(aDataset);

    return AsCoreType(aInstance).Get<MeshCoP::PendingDatasetManager>().Read(*aDataset);
}

otError otDatasetSetPending(otInstance *aInstance, const otOperationalDataset *aDataset)
{
    return AsCoreType(aInstance).Get<MeshCoP::PendingDatasetManager>().Save(AsCoreType(aDataset));
}

otError otDatasetSetPendingTlvs(otInstance *aInstance, const otOperationalDatasetTlvs *aDataset)
{
    AssertPointerIsNotNull(aDataset);

    return AsCoreType(aInstance).Get<MeshCoP::PendingDatasetManager>().Save(*aDataset);
}

otError otDatasetSendMgmtActiveGet(otInstance                           *aInstance,
                                   const otOperationalDatasetComponents *aDatasetComponents,
                                   const uint8_t                        *aTlvTypes,
                                   uint8_t                               aLength,
                                   const otIp6Address                   *aAddress)
{
    return AsCoreType(aInstance).Get<MeshCoP::ActiveDatasetManager>().SendGetRequest(AsCoreType(aDatasetComponents),
                                                                                     aTlvTypes, aLength, aAddress);
}

otError otDatasetSendMgmtActiveSet(otInstance                 *aInstance,
                                   const otOperationalDataset *aDataset,
                                   const uint8_t              *aTlvs,
                                   uint8_t                     aLength,
                                   otDatasetMgmtSetCallback    aCallback,
                                   void                       *aContext)
{
    return AsCoreType(aInstance).Get<MeshCoP::ActiveDatasetManager>().SendSetRequest(AsCoreType(aDataset), aTlvs,
                                                                                     aLength, aCallback, aContext);
}

otError otDatasetSendMgmtPendingGet(otInstance                           *aInstance,
                                    const otOperationalDatasetComponents *aDatasetComponents,
                                    const uint8_t                        *aTlvTypes,
                                    uint8_t                               aLength,
                                    const otIp6Address                   *aAddress)
{
    return AsCoreType(aInstance).Get<MeshCoP::PendingDatasetManager>().SendGetRequest(AsCoreType(aDatasetComponents),
                                                                                      aTlvTypes, aLength, aAddress);
}

otError otDatasetSendMgmtPendingSet(otInstance                 *aInstance,
                                    const otOperationalDataset *aDataset,
                                    const uint8_t              *aTlvs,
                                    uint8_t                     aLength,
                                    otDatasetMgmtSetCallback    aCallback,
                                    void                       *aContext)
{
    return AsCoreType(aInstance).Get<MeshCoP::PendingDatasetManager>().SendSetRequest(AsCoreType(aDataset), aTlvs,
                                                                                      aLength, aCallback, aContext);
}

#if OPENTHREAD_FTD
otError otDatasetGeneratePskc(const char            *aPassPhrase,
                              const otNetworkName   *aNetworkName,
                              const otExtendedPanId *aExtPanId,
                              otPskc                *aPskc)
{
    return MeshCoP::GeneratePskc(aPassPhrase, AsCoreType(aNetworkName), AsCoreType(aExtPanId), AsCoreType(aPskc));
}
#endif

otError otNetworkNameFromString(otNetworkName *aNetworkName, const char *aNameString)
{
    otError error = AsCoreType(aNetworkName).Set(aNameString);

    return (error == OT_ERROR_ALREADY) ? OT_ERROR_NONE : error;
}

otError otDatasetParseTlvs(const otOperationalDatasetTlvs *aDatasetTlvs, otOperationalDataset *aDataset)
{
    Error            error = kErrorNone;
    MeshCoP::Dataset dataset;

    AssertPointerIsNotNull(aDatasetTlvs);

    dataset.SetFrom(*aDatasetTlvs);
    VerifyOrExit(dataset.IsValid(), error = kErrorInvalidArgs);
    dataset.ConvertTo(AsCoreType(aDataset));

exit:
    return error;
}

otError otDatasetConvertToTlvs(const otOperationalDataset *aDataset, otOperationalDatasetTlvs *aDatasetTlvs)
{
    Error            error = kErrorNone;
    MeshCoP::Dataset dataset;

    AssertPointerIsNotNull(aDatasetTlvs);

    SuccessOrExit(error = dataset.SetFrom(AsCoreType(aDataset)));
    dataset.ConvertTo(*aDatasetTlvs);

exit:
    return error;
}

otError otDatasetUpdateTlvs(const otOperationalDataset *aDataset, otOperationalDatasetTlvs *aDatasetTlvs)
{
    Error            error = kErrorNone;
    MeshCoP::Dataset dataset;

    AssertPointerIsNotNull(aDatasetTlvs);

    dataset.SetFrom(*aDatasetTlvs);
    SuccessOrExit(error = dataset.SetFrom(AsCoreType(aDataset)));
    dataset.ConvertTo(*aDatasetTlvs);

exit:
    return error;
}
