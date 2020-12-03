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
 *   This file implements the OpenThread Commissioner API.
 */

#include "openthread-core-config.h"

#include <openthread/commissioner.h>

#include "common/instance.hpp"
#include "common/locator-getters.hpp"

using namespace ot;

#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_COMMISSIONER_ENABLE
otError otCommissionerStart(otInstance *                 aInstance,
                            otCommissionerStateCallback  aStateCallback,
                            otCommissionerJoinerCallback aJoinerCallback,
                            void *                       aCallbackContext)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<MeshCoP::Commissioner>().Start(aStateCallback, aJoinerCallback, aCallbackContext);
}

otError otCommissionerStop(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<MeshCoP::Commissioner>().Stop(/* aResign */ true);
}

otError otCommissionerAddJoiner(otInstance *aInstance, const otExtAddress *aEui64, const char *aPskd, uint32_t aTimeout)
{
    otError                error;
    MeshCoP::Commissioner &commissioner = static_cast<Instance *>(aInstance)->Get<MeshCoP::Commissioner>();

    if (aEui64 == nullptr)
    {
        error = commissioner.AddJoinerAny(aPskd, aTimeout);
        ExitNow();
    }

    error = commissioner.AddJoiner(*static_cast<const Mac::ExtAddress *>(aEui64), aPskd, aTimeout);

exit:
    return error;
}

otError otCommissionerAddJoinerWithDiscerner(otInstance *             aInstance,
                                             const otJoinerDiscerner *aDiscerner,
                                             const char *             aPskd,
                                             uint32_t                 aTimeout)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<MeshCoP::Commissioner>().AddJoiner(*static_cast<const MeshCoP::JoinerDiscerner *>(aDiscerner),
                                                           aPskd, aTimeout);
}

otError otCommissionerGetNextJoinerInfo(otInstance *aInstance, uint16_t *aIterator, otJoinerInfo *aJoiner)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<MeshCoP::Commissioner>().GetNextJoinerInfo(*aIterator, *aJoiner);
}

otError otCommissionerRemoveJoiner(otInstance *aInstance, const otExtAddress *aEui64)
{
    otError                error;
    MeshCoP::Commissioner &commissioner = static_cast<Instance *>(aInstance)->Get<MeshCoP::Commissioner>();

    if (aEui64 == nullptr)
    {
        error = commissioner.RemoveJoinerAny(/* aTimeout */ 0);
        ExitNow();
    }

    error = commissioner.RemoveJoiner(*static_cast<const Mac::ExtAddress *>(aEui64), /* aTimeout */ 0);

exit:
    return error;
}

otError otCommissionerRemoveJoinerWithDiscerner(otInstance *aInstance, const otJoinerDiscerner *aDiscerner)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<MeshCoP::Commissioner>().RemoveJoiner(
        *static_cast<const MeshCoP::JoinerDiscerner *>(aDiscerner), 0);
}

otError otCommissionerSetProvisioningUrl(otInstance *aInstance, const char *aProvisioningUrl)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<MeshCoP::Commissioner>().SetProvisioningUrl(aProvisioningUrl);
}

const char *otCommissionerGetProvisioningUrl(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<MeshCoP::Commissioner>().GetProvisioningUrl();
}

otError otCommissionerAnnounceBegin(otInstance *        aInstance,
                                    uint32_t            aChannelMask,
                                    uint8_t             aCount,
                                    uint16_t            aPeriod,
                                    const otIp6Address *aAddress)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<MeshCoP::Commissioner>().GetAnnounceBeginClient().SendRequest(
        aChannelMask, aCount, aPeriod, *static_cast<const Ip6::Address *>(aAddress));
}

otError otCommissionerEnergyScan(otInstance *                       aInstance,
                                 uint32_t                           aChannelMask,
                                 uint8_t                            aCount,
                                 uint16_t                           aPeriod,
                                 uint16_t                           aScanDuration,
                                 const otIp6Address *               aAddress,
                                 otCommissionerEnergyReportCallback aCallback,
                                 void *                             aContext)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<MeshCoP::Commissioner>().GetEnergyScanClient().SendQuery(
        aChannelMask, aCount, aPeriod, aScanDuration, *static_cast<const Ip6::Address *>(aAddress), aCallback,
        aContext);
}

otError otCommissionerPanIdQuery(otInstance *                        aInstance,
                                 uint16_t                            aPanId,
                                 uint32_t                            aChannelMask,
                                 const otIp6Address *                aAddress,
                                 otCommissionerPanIdConflictCallback aCallback,
                                 void *                              aContext)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<MeshCoP::Commissioner>().GetPanIdQueryClient().SendQuery(
        aPanId, aChannelMask, *static_cast<const Ip6::Address *>(aAddress), aCallback, aContext);
}

otError otCommissionerSendMgmtGet(otInstance *aInstance, const uint8_t *aTlvs, uint8_t aLength)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<MeshCoP::Commissioner>().SendMgmtCommissionerGetRequest(aTlvs, aLength);
}

otError otCommissionerSendMgmtSet(otInstance *                  aInstance,
                                  const otCommissioningDataset *aDataset,
                                  const uint8_t *               aTlvs,
                                  uint8_t                       aLength)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<MeshCoP::Commissioner>().SendMgmtCommissionerSetRequest(*aDataset, aTlvs, aLength);
}

uint16_t otCommissionerGetSessionId(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<MeshCoP::Commissioner>().GetSessionId();
}

otCommissionerState otCommissionerGetState(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return static_cast<otCommissionerState>(instance.Get<MeshCoP::Commissioner>().GetState());
}

#endif // OPENTHREAD_FTD && OPENTHREAD_CONFIG_COMMISSIONER_ENABLE
