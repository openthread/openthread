/*
 *  Copyright (c) 2025, The OpenThread Authors.
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
 * @brief This file implements the OpenThread MeshCoP helper API.
 */

#include "openthread-core-config.h"

#include <openthread/steering_data.h>

#include "meshcop/meshcop.hpp"

using namespace ot;

#if OPENTHREAD_CONFIG_MESHCOP_STEERING_DATA_API_ENABLE

otError otSteeringDataInit(otSteeringData *aSteeringData, uint8_t aLength)
{
    return AsCoreType(aSteeringData).Init(aLength);
}

bool otSteeringDataIsValid(const otSteeringData *aSteeringData) { return AsCoreType(aSteeringData).IsValid(); }

void otSteeringDataSetToPermitAllJoiners(otSteeringData *aSteeringData)
{
    AsCoreType(aSteeringData).SetToPermitAllJoiners();
}

otError otSteeringDataUpdateWithJoinerId(otSteeringData *aSteeringData, const otExtAddress *aJoinerId)
{
    return AsCoreType(aSteeringData).UpdateBloomFilter(AsCoreType(aJoinerId));
}

otError otSteeringDataUpdateWithDiscerner(otSteeringData *aSteeringData, const otJoinerDiscerner *aDiscerner)
{
    return AsCoreType(aSteeringData).UpdateBloomFilter(AsCoreType(aDiscerner));
}

otError otSteeringDataMerge(otSteeringData *aSteeringData, const otSteeringData *aOtherSteeringData)
{
    return AsCoreType(aSteeringData).MergeBloomFilterWith(AsCoreType(aOtherSteeringData));
}

bool otSteeringDataPermitsAllJoiners(const otSteeringData *aSteeringData)
{
    return AsCoreType(aSteeringData).PermitsAllJoiners();
}

bool otSteeringDataIsEmpty(const otSteeringData *aSteeringData) { return AsCoreType(aSteeringData).IsEmpty(); }

bool otSteeringDataContainsJoinerId(const otSteeringData *aSteeringData, const otExtAddress *aJoinerId)
{
    return AsCoreType(aSteeringData).Contains(AsCoreType(aJoinerId));
}

bool otSteeringDataContainsDiscerner(const otSteeringData *aSteeringData, const otJoinerDiscerner *aDiscerner)
{
    return AsCoreType(aSteeringData).Contains(AsCoreType(aDiscerner));
}

#endif // OPENTHREAD_CONFIG_MESHCOP_STEERING_DATA_API_ENABLE
