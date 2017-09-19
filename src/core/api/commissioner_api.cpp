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

#include <openthread/config.h>

#include <openthread/commissioner.h>

#include "openthread-instance.h"

using namespace ot;

otError otCommissionerStart(otInstance *aInstance)
{
    otError error = OT_ERROR_DISABLED_FEATURE;

#if OPENTHREAD_FTD && OPENTHREAD_ENABLE_COMMISSIONER
    error = aInstance->mThreadNetif.GetCommissioner().Start();
#else  // OPENTHREAD_ENABLE_COMMISSIONER && OPENTHREAD_FTD
    OT_UNUSED_VARIABLE(aInstance);
#endif // OPENTHREAD_ENABLE_COMMISSIONER && OPENTHREAD_FTD

    return error;
}

otError otCommissionerStop(otInstance *aInstance)
{
    otError error = OT_ERROR_DISABLED_FEATURE;

#if OPENTHREAD_FTD && OPENTHREAD_ENABLE_COMMISSIONER
    error = aInstance->mThreadNetif.GetCommissioner().Stop();
#else  // OPENTHREAD_ENABLE_COMMISSIONER && OPENTHREAD_FTD
    OT_UNUSED_VARIABLE(aInstance);
#endif // OPENTHREAD_ENABLE_COMMISSIONER && OPENTHREAD_FTD

    return error;
}

otError otCommissionerAddJoiner(otInstance *aInstance, const otExtAddress *aExtAddress, const char *aPSKd,
                                uint32_t aTimeout)
{
    otError error = OT_ERROR_DISABLED_FEATURE;

#if OPENTHREAD_FTD && OPENTHREAD_ENABLE_COMMISSIONER
    error = aInstance->mThreadNetif.GetCommissioner().AddJoiner(static_cast<const Mac::ExtAddress *>(aExtAddress), aPSKd,
                                                                aTimeout);
#else  // OPENTHREAD_ENABLE_COMMISSIONER && OPENTHREAD_FTD
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aExtAddress);
    OT_UNUSED_VARIABLE(aPSKd);
    OT_UNUSED_VARIABLE(aTimeout);
#endif // OPENTHREAD_ENABLE_COMMISSIONER && OPENTHREAD_FTD

    return error;
}

otError otCommissionerRemoveJoiner(otInstance *aInstance, const otExtAddress *aExtAddress)
{
    otError error = OT_ERROR_DISABLED_FEATURE;

#if OPENTHREAD_FTD && OPENTHREAD_ENABLE_COMMISSIONER
    error = aInstance->mThreadNetif.GetCommissioner().RemoveJoiner(static_cast<const Mac::ExtAddress *>(aExtAddress), 0);
#else  // OPENTHREAD_ENABLE_COMMISSIONER && OPENTHREAD_FTD
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aExtAddress);
#endif // OPENTHREAD_ENABLE_COMMISSIONER && OPENTHREAD_FTD

    return error;
}

otError otCommissionerSetProvisioningUrl(otInstance *aInstance, const char *aProvisioningUrl)
{
    otError error = OT_ERROR_DISABLED_FEATURE;

#if OPENTHREAD_FTD && OPENTHREAD_ENABLE_COMMISSIONER
    error = aInstance->mThreadNetif.GetCommissioner().SetProvisioningUrl(aProvisioningUrl);
#else  // OPENTHREAD_ENABLE_COMMISSIONER && OPENTHREAD_FTD
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aProvisioningUrl);
#endif // OPENTHREAD_ENABLE_COMMISSIONER && OPENTHREAD_FTD

    return error;
}

otError otCommissionerAnnounceBegin(otInstance *aInstance, uint32_t aChannelMask, uint8_t aCount, uint16_t aPeriod,
                                    const otIp6Address *aAddress)
{
    otError error = OT_ERROR_DISABLED_FEATURE;

#if OPENTHREAD_FTD && OPENTHREAD_ENABLE_COMMISSIONER
    error = aInstance->mThreadNetif.GetCommissioner().mAnnounceBegin.SendRequest(aChannelMask, aCount, aPeriod,
                                                                                 *static_cast<const Ip6::Address *>(aAddress));
#else  // OPENTHREAD_ENABLE_COMMISSIONER && OPENTHREAD_FTD
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aChannelMask);
    OT_UNUSED_VARIABLE(aCount);
    OT_UNUSED_VARIABLE(aPeriod);
    OT_UNUSED_VARIABLE(aAddress);
#endif // OPENTHREAD_ENABLE_COMMISSIONER && OPENTHREAD_FTD

    return error;
}

otError otCommissionerEnergyScan(otInstance *aInstance, uint32_t aChannelMask, uint8_t aCount, uint16_t aPeriod,
                                 uint16_t aScanDuration, const otIp6Address *aAddress,
                                 otCommissionerEnergyReportCallback aCallback, void *aContext)
{
    otError error = OT_ERROR_DISABLED_FEATURE;

#if OPENTHREAD_FTD && OPENTHREAD_ENABLE_COMMISSIONER
    error = aInstance->mThreadNetif.GetCommissioner().mEnergyScan.SendQuery(aChannelMask, aCount, aPeriod, aScanDuration,
                                                                            *static_cast<const Ip6::Address *>(aAddress),
                                                                            aCallback, aContext);
#else  // OPENTHREAD_ENABLE_COMMISSIONER && OPENTHREAD_FTD
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aChannelMask);
    OT_UNUSED_VARIABLE(aCount);
    OT_UNUSED_VARIABLE(aPeriod);
    OT_UNUSED_VARIABLE(aScanDuration);
    OT_UNUSED_VARIABLE(aAddress);
    OT_UNUSED_VARIABLE(aCallback);
    OT_UNUSED_VARIABLE(aContext);
#endif // OPENTHREAD_ENABLE_COMMISSIONER && OPENTHREAD_FTD

    return error;
}

otError otCommissionerPanIdQuery(otInstance *aInstance, uint16_t aPanId, uint32_t aChannelMask,
                                 const otIp6Address *aAddress,
                                 otCommissionerPanIdConflictCallback aCallback, void *aContext)
{
    otError error = OT_ERROR_DISABLED_FEATURE;

#if OPENTHREAD_FTD && OPENTHREAD_ENABLE_COMMISSIONER
    error = aInstance->mThreadNetif.GetCommissioner().mPanIdQuery.SendQuery(
                aPanId, aChannelMask, *static_cast<const Ip6::Address *>(aAddress), aCallback, aContext);
#else  // OPENTHREAD_ENABLE_COMMISSIONER && OPENTHREAD_FTD
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aPanId);
    OT_UNUSED_VARIABLE(aChannelMask);
    OT_UNUSED_VARIABLE(aAddress);
    OT_UNUSED_VARIABLE(aCallback);
    OT_UNUSED_VARIABLE(aContext);
#endif // OPENTHREAD_ENABLE_COMMISSIONER && OPENTHREAD_FTD

    return error;
}

otError otCommissionerSendMgmtGet(otInstance *aInstance, const uint8_t *aTlvs, uint8_t aLength)
{
    otError error = OT_ERROR_DISABLED_FEATURE;

#if OPENTHREAD_FTD && OPENTHREAD_ENABLE_COMMISSIONER
    error = aInstance->mThreadNetif.GetCommissioner().SendMgmtCommissionerGetRequest(aTlvs, aLength);
#else  // OPENTHREAD_ENABLE_COMMISSIONER && OPENTHREAD_FTD
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aTlvs);
    OT_UNUSED_VARIABLE(aLength);
#endif // OPENTHREAD_ENABLE_COMMISSIONER && OPENTHREAD_FTD

    return error;
}

otError otCommissionerSendMgmtSet(otInstance *aInstance, const otCommissioningDataset *aDataset,
                                  const uint8_t *aTlvs, uint8_t aLength)
{
    otError error = OT_ERROR_DISABLED_FEATURE;

#if OPENTHREAD_FTD && OPENTHREAD_ENABLE_COMMISSIONER
    error = aInstance->mThreadNetif.GetCommissioner().SendMgmtCommissionerSetRequest(*aDataset, aTlvs, aLength);
#else  // OPENTHREAD_ENABLE_COMMISSIONER && OPENTHREAD_FTD
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aDataset);
    OT_UNUSED_VARIABLE(aTlvs);
    OT_UNUSED_VARIABLE(aLength);
#endif // OPENTHREAD_ENABLE_COMMISSIONER && OPENTHREAD_FTD

    return error;
}

uint16_t otCommissionerGetSessionId(otInstance *aInstance)
{
    uint16_t sessionId = 0;

#if OPENTHREAD_FTD && OPENTHREAD_ENABLE_COMMISSIONER
    sessionId = aInstance->mThreadNetif.GetCommissioner().GetSessionId();
#else  // OPENTHREAD_ENABLE_COMMISSIONER && OPENTHREAD_FTD
    OT_UNUSED_VARIABLE(aInstance);
#endif // OPENTHREAD_ENABLE_COMMISSIONER && OPENTHREAD_FTD

    return sessionId;
}

otCommissionerState otCommissionerGetState(otInstance *aInstance)
{
    otCommissionerState state = OT_COMMISSIONER_STATE_DISABLED;

#if OPENTHREAD_FTD && OPENTHREAD_ENABLE_COMMISSIONER
    state = aInstance->mThreadNetif.GetCommissioner().GetState();
#else  // OPENTHREAD_ENABLE_COMMISSIONER && OPENTHREAD_FTD
    OT_UNUSED_VARIABLE(aInstance);
#endif // OPENTHREAD_ENABLE_COMMISSIONER && OPENTHREAD_FTD

    return state;
}

otError otCommissionerGeneratePSKc(otInstance *aInstance, const char *aPassPhrase, const char *aNetworkName,
                                   const uint8_t *aExtPanId, uint8_t *aPSKc)
{
    otError error = OT_ERROR_DISABLED_FEATURE;

#if OPENTHREAD_FTD && OPENTHREAD_ENABLE_COMMISSIONER
    error = aInstance->mThreadNetif.GetCommissioner().GeneratePSKc(aPassPhrase, aNetworkName, aExtPanId, aPSKc);
#else  // OPENTHREAD_ENABLE_COMMISSIONER && OPENTHREAD_FTD
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aPassPhrase);
    OT_UNUSED_VARIABLE(aNetworkName);
    OT_UNUSED_VARIABLE(aExtPanId);
    OT_UNUSED_VARIABLE(aPSKc);
#endif // OPENTHREAD_ENABLE_COMMISSIONER && OPENTHREAD_FTD

    return error;
}
