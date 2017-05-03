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

#include "openthread/commissioner.h"

#include "openthread-instance.h"

using namespace ot;

#if OPENTHREAD_FTD && OPENTHREAD_ENABLE_COMMISSIONER

ThreadError otCommissionerStart(otInstance *aInstance)
{
    return aInstance->mThreadNetif.GetCommissioner().Start();
}

ThreadError otCommissionerStop(otInstance *aInstance)
{
    return aInstance->mThreadNetif.GetCommissioner().Stop();
}

ThreadError otCommissionerAddJoiner(otInstance *aInstance, const otExtAddress *aExtAddress, const char *aPSKd,
                                    uint32_t aTimeout)
{
    return aInstance->mThreadNetif.GetCommissioner().AddJoiner(static_cast<const Mac::ExtAddress *>(aExtAddress), aPSKd,
                                                               aTimeout);
}

ThreadError otCommissionerRemoveJoiner(otInstance *aInstance, const otExtAddress *aExtAddress)
{
    return aInstance->mThreadNetif.GetCommissioner().RemoveJoiner(static_cast<const Mac::ExtAddress *>(aExtAddress), 0);
}

ThreadError otCommissionerSetProvisioningUrl(otInstance *aInstance, const char *aProvisioningUrl)
{
    return aInstance->mThreadNetif.GetCommissioner().SetProvisioningUrl(aProvisioningUrl);
}

ThreadError otCommissionerAnnounceBegin(otInstance *aInstance, uint32_t aChannelMask, uint8_t aCount, uint16_t aPeriod,
                                        const otIp6Address *aAddress)
{
    return aInstance->mThreadNetif.GetCommissioner().mAnnounceBegin.SendRequest(aChannelMask, aCount, aPeriod,
                                                                                *static_cast<const Ip6::Address *>(aAddress));
}

ThreadError otCommissionerEnergyScan(otInstance *aInstance, uint32_t aChannelMask, uint8_t aCount, uint16_t aPeriod,
                                     uint16_t aScanDuration, const otIp6Address *aAddress,
                                     otCommissionerEnergyReportCallback aCallback, void *aContext)
{
    return aInstance->mThreadNetif.GetCommissioner().mEnergyScan.SendQuery(aChannelMask, aCount, aPeriod, aScanDuration,
                                                                           *static_cast<const Ip6::Address *>(aAddress),
                                                                           aCallback, aContext);
}

ThreadError otCommissionerPanIdQuery(otInstance *aInstance, uint16_t aPanId, uint32_t aChannelMask,
                                     const otIp6Address *aAddress,
                                     otCommissionerPanIdConflictCallback aCallback, void *aContext)
{
    return aInstance->mThreadNetif.GetCommissioner().mPanIdQuery.SendQuery(
               aPanId, aChannelMask, *static_cast<const Ip6::Address *>(aAddress), aCallback, aContext);
}

ThreadError otCommissionerSendMgmtGet(otInstance *aInstance, const uint8_t *aTlvs, uint8_t aLength)
{
    return aInstance->mThreadNetif.GetCommissioner().SendMgmtCommissionerGetRequest(aTlvs, aLength);
}

ThreadError otCommissionerSendMgmtSet(otInstance *aInstance, const otCommissioningDataset *aDataset,
                                      const uint8_t *aTlvs, uint8_t aLength)
{
    return aInstance->mThreadNetif.GetCommissioner().SendMgmtCommissionerSetRequest(*aDataset, aTlvs, aLength);
}

uint16_t otCommissionerGetSessionId(otInstance *aInstance)
{
    return aInstance->mThreadNetif.GetCommissioner().GetSessionId();
}

otCommissionerState otCommissionerGetState(otInstance *aInstance)
{
    return aInstance->mThreadNetif.GetCommissioner().GetState();
}

ThreadError otCommissionerGeneratePSKc(otInstance *aInstance, const char *aPassPhrase, const char *aNetworkName,
                                       const uint8_t *aExtPanId, uint8_t *aPSKc)
{
    return aInstance->mThreadNetif.GetCommissioner().GeneratePSKc(aPassPhrase, aNetworkName, aExtPanId, aPSKc);
}

#endif  // OPENTHREAD_ENABLE_COMMISSIONER && OPENTHREAD_FTD
