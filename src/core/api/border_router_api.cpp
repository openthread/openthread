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
 *   This file implements the OpenThread Border Router API.
 */

#include "openthread-core-config.h"

#if OPENTHREAD_CONFIG_BORDER_ROUTER_ENABLE

#include <openthread/border_router.h>

#include "common/debug.hpp"
#include "common/instance.hpp"
#include "common/locator-getters.hpp"

using namespace ot;

otError otBorderRouterGetNetData(otInstance *aInstance, bool aStable, uint8_t *aData, uint8_t *aDataLength)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    OT_ASSERT(aData != nullptr && aDataLength != nullptr);

    return instance.Get<NetworkData::Local>().GetNetworkData(aStable, aData, *aDataLength);
}

otError otBorderRouterAddOnMeshPrefix(otInstance *aInstance, const otBorderRouterConfig *aConfig)
{
    otError   error    = OT_ERROR_NONE;
    Instance &instance = *static_cast<Instance *>(aInstance);

    OT_ASSERT(aConfig != nullptr);
    // Add Prefix validation check:
    // Thread 1.1 Specification 5.13.2 says
    // "A valid prefix MUST NOT allow both DHCPv6 and SLAAC for address configuration"
    VerifyOrExit(!aConfig->mDhcp || !aConfig->mSlaac, error = OT_ERROR_INVALID_ARGS);

    error = instance.Get<NetworkData::Local>().AddOnMeshPrefix(*aConfig);
#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE
    // Only try to configure Domain Prefix after the parameter is vaidated via above `AddOnMeshPrefix()`.
    if (error == OT_ERROR_NONE && aConfig->mDp)
    {
        // Restore local server data
        IgnoreError(instance.Get<NetworkData::Local>().RemoveOnMeshPrefix(aConfig->mPrefix.mPrefix.mFields.m8,
                                                                          aConfig->mPrefix.mLength));

        instance.Get<BackboneRouter::Local>().SetDomainPrefix(*aConfig);
    }
#endif

exit:
    return error;
}

otError otBorderRouterRemoveOnMeshPrefix(otInstance *aInstance, const otIp6Prefix *aPrefix)
{
    otError   error    = OT_ERROR_NONE;
    Instance &instance = *static_cast<Instance *>(aInstance);

    OT_ASSERT(aPrefix != nullptr);

#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE
    error = instance.Get<BackboneRouter::Local>().RemoveDomainPrefix(*aPrefix);

    if (error == OT_ERROR_NOT_FOUND)
#endif
    {
        error = instance.Get<NetworkData::Local>().RemoveOnMeshPrefix(aPrefix->mPrefix.mFields.m8, aPrefix->mLength);
    }

    return error;
}

otError otBorderRouterGetNextOnMeshPrefix(otInstance *           aInstance,
                                          otNetworkDataIterator *aIterator,
                                          otBorderRouterConfig * aConfig)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    OT_ASSERT(aIterator != nullptr && aConfig != nullptr);

    return instance.Get<NetworkData::Local>().GetNextOnMeshPrefix(*aIterator, *aConfig);
}

otError otBorderRouterAddRoute(otInstance *aInstance, const otExternalRouteConfig *aConfig)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    OT_ASSERT(aConfig != nullptr);

    return instance.Get<NetworkData::Local>().AddHasRoutePrefix(
        aConfig->mPrefix.mPrefix.mFields.m8, aConfig->mPrefix.mLength, aConfig->mPreference, aConfig->mStable);
}

otError otBorderRouterRemoveRoute(otInstance *aInstance, const otIp6Prefix *aPrefix)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    OT_ASSERT(aPrefix != nullptr);

    return instance.Get<NetworkData::Local>().RemoveHasRoutePrefix(aPrefix->mPrefix.mFields.m8, aPrefix->mLength);
}

otError otBorderRouterGetNextRoute(otInstance *           aInstance,
                                   otNetworkDataIterator *aIterator,
                                   otExternalRouteConfig *aConfig)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    OT_ASSERT(aIterator != nullptr && aConfig != nullptr);

    return instance.Get<NetworkData::Local>().GetNextExternalRoute(*aIterator, *aConfig);
}

otError otBorderRouterRegister(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    instance.Get<NetworkData::Notifier>().HandleServerDataUpdated();

    return OT_ERROR_NONE;
}

#endif // OPENTHREAD_CONFIG_BORDER_ROUTER_ENABLE
