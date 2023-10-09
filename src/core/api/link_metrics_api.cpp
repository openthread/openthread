/*
 *  Copyright (c) 2020, The OpenThread Authors.
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
 *   This file implements the OpenThread Link Metrics API.
 */

#include "openthread-core-config.h"

#if OPENTHREAD_CONFIG_MLE_LINK_METRICS_INITIATOR_ENABLE
#include <openthread/link_metrics.h>

#include "common/as_core_type.hpp"
#include "common/locator_getters.hpp"

using namespace ot;

otError otLinkMetricsQuery(otInstance                 *aInstance,
                           const otIp6Address         *aDestination,
                           uint8_t                     aSeriesId,
                           const otLinkMetrics        *aLinkMetricsFlags,
                           otLinkMetricsReportCallback aCallback,
                           void                       *aCallbackContext)
{
    AsCoreType(aInstance).Get<LinkMetrics::Initiator>().SetReportCallback(aCallback, aCallbackContext);

    return AsCoreType(aInstance).Get<LinkMetrics::Initiator>().Query(AsCoreType(aDestination), aSeriesId,
                                                                     AsCoreTypePtr(aLinkMetricsFlags));
}

otError otLinkMetricsConfigForwardTrackingSeries(otInstance                       *aInstance,
                                                 const otIp6Address               *aDestination,
                                                 uint8_t                           aSeriesId,
                                                 const otLinkMetricsSeriesFlags    aSeriesFlags,
                                                 const otLinkMetrics              *aLinkMetricsFlags,
                                                 otLinkMetricsMgmtResponseCallback aCallback,
                                                 void                             *aCallbackContext)
{
    LinkMetrics::Initiator &initiator = AsCoreType(aInstance).Get<LinkMetrics::Initiator>();

    initiator.SetMgmtResponseCallback(aCallback, aCallbackContext);

    return initiator.SendMgmtRequestForwardTrackingSeries(AsCoreType(aDestination), aSeriesId,
                                                          AsCoreType(&aSeriesFlags), AsCoreTypePtr(aLinkMetricsFlags));
}

otError otLinkMetricsConfigEnhAckProbing(otInstance                                *aInstance,
                                         const otIp6Address                        *aDestination,
                                         otLinkMetricsEnhAckFlags                   aEnhAckFlags,
                                         const otLinkMetrics                       *aLinkMetricsFlags,
                                         otLinkMetricsMgmtResponseCallback          aCallback,
                                         void                                      *aCallbackContext,
                                         otLinkMetricsEnhAckProbingIeReportCallback aEnhAckCallback,
                                         void                                      *aEnhAckCallbackContext)
{
    LinkMetrics::Initiator &initiator = AsCoreType(aInstance).Get<LinkMetrics::Initiator>();

    initiator.SetMgmtResponseCallback(aCallback, aCallbackContext);
    initiator.SetEnhAckProbingCallback(aEnhAckCallback, aEnhAckCallbackContext);

    return initiator.SendMgmtRequestEnhAckProbing(AsCoreType(aDestination), MapEnum(aEnhAckFlags),
                                                  AsCoreTypePtr(aLinkMetricsFlags));
}

otError otLinkMetricsSendLinkProbe(otInstance         *aInstance,
                                   const otIp6Address *aDestination,
                                   uint8_t             aSeriesId,
                                   uint8_t             aLength)
{
    LinkMetrics::Initiator &initiator = AsCoreType(aInstance).Get<LinkMetrics::Initiator>();

    return initiator.SendLinkProbe(AsCoreType(aDestination), aSeriesId, aLength);
}

#if OPENTHREAD_CONFIG_LINK_METRICS_MANAGER_ENABLE
void otLinkMetricsManagerSetEnabled(otInstance *aInstance, bool aEnable)
{
    AsCoreType(aInstance).Get<Utils::LinkMetricsManager>().SetEnabled(aEnable);
}

otError otLinkMetricsManagerGetMetricsValueByExtAddr(otInstance          *aInstance,
                                                     const otExtAddress  *aExtAddress,
                                                     otLinkMetricsValues *aLinkMetricsValues)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(aExtAddress != nullptr && aLinkMetricsValues != nullptr, error = OT_ERROR_INVALID_ARGS);

    error = AsCoreType(aInstance).Get<Utils::LinkMetricsManager>().GetLinkMetricsValueByExtAddr(
        AsCoreType(aExtAddress), AsCoreType(aLinkMetricsValues));
exit:
    return error;
}
#endif

#endif // OPENTHREAD_CONFIG_MLE_LINK_METRICS_INITIATOR_ENABLE
