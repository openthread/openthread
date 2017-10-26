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
 *   This file implements the OpenThread Server API.
 */

#include <openthread/config.h>

#if OPENTHREAD_ENABLE_SERVICE

#include <openthread/server.h>

#include "openthread-instance.h"

using namespace ot;

otError otServerGetNetDataLocal(otInstance *aInstance, bool aStable, uint8_t *aData, uint8_t *aDataLength)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(aData != NULL && aDataLength != NULL, error = OT_ERROR_INVALID_ARGS);

    error = aInstance->mThreadNetif.GetNetworkDataLocal().GetNetworkData(aStable, aData, *aDataLength);

exit:
    return error;
}

otError otServerAddService(otInstance *aInstance, const otServiceConfig *aConfig)
{
    return aInstance->mThreadNetif.GetNetworkDataLocal().AddService(
               aConfig->mEnterpriseNumber, &aConfig->mServiceData[0], aConfig->mServiceDataLength,
               aConfig->mServerConfig.mStable, &aConfig->mServerConfig.mServerData[0], aConfig->mServerConfig.mServerDataLength
           );
}

otError otServerRemoveService(otInstance *aInstance, uint32_t aEnterpriseNumber, uint8_t *aServiceData,
                              uint8_t aServiceDataLength)
{
    return aInstance->mThreadNetif.GetNetworkDataLocal().RemoveService(aEnterpriseNumber, aServiceData, aServiceDataLength);
}

otError otServerGetNextService(otInstance *aInstance, otNetworkDataIterator *aIterator,
                               otServiceConfig *aConfig)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(aIterator && aConfig, error = OT_ERROR_INVALID_ARGS);

    error = aInstance->mThreadNetif.GetNetworkDataLocal().GetNextService(aIterator, aConfig);

exit:
    return error;
}

otError otServerRegister(otInstance *aInstance)
{
    return aInstance->mThreadNetif.GetNetworkDataLocal().SendServerDataNotification();
}

#endif  // OPENTHREAD_ENABLE_SERVICE
