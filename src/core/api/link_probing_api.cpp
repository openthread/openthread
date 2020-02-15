/*
 *  Copyright (c) 2017, The OpenThread Authors.
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
 *   This file implements the OpenThread Link Metrics Probing API.
 */

#include "openthread-core-config.h"

#include <openthread/link_probing.h>

#include "common/instance.hpp"
#if OPENTHREAD_CONFIG_LINK_PROBE_ENABLE
#include "thread/link_probing.hpp"
#endif

using namespace ot;

#if OPENTHREAD_CONFIG_LINK_PROBE_ENABLE
otError otLinkProbingQuery(otInstance *        aInstance,
                           const otIp6Address *aDestination,
                           uint8_t             aSeriesId,
                           uint8_t *           aTypeIdFlags,
                           uint8_t             aTypeIdFlagsCount)
{
    LinkProbing::LinkMetricTypeId *typeIdFlags = reinterpret_cast<LinkProbing::LinkMetricTypeId *>(aTypeIdFlags);

    return static_cast<Instance *>(aInstance)->Get<LinkProbing::LinkProbing>().LinkProbeQuery(
        *static_cast<const Ip6::Address *>(aDestination), aSeriesId, typeIdFlags, aTypeIdFlagsCount);
}

otError otLinkProbingMgmtForward(otInstance *        aInstance,
                                 const otIp6Address *aDestination,
                                 uint8_t             aForwardSeriesId,
                                 uint8_t             aForwardSeriesFlags,
                                 uint8_t *           aTypeIdFlags,
                                 uint8_t             aTypeIdFlagsCount)
{
    LinkProbing::LinkMetricTypeId *typeIdFlags = reinterpret_cast<LinkProbing::LinkMetricTypeId *>(aTypeIdFlags);

    return static_cast<Instance *>(aInstance)->Get<LinkProbing::LinkProbing>().ForwardMgmtRequest(
        *static_cast<const Ip6::Address *>(aDestination), aForwardSeriesId, aForwardSeriesFlags, typeIdFlags,
        aTypeIdFlagsCount);
}

otError otLinkProbingMgmtEnhancedAck(otInstance *        aInstance,
                                     const otIp6Address *aDestination,
                                     uint8_t             aEnhAckFlags,
                                     uint8_t *           aTypeIdFlags,
                                     uint8_t             aTypeIdFlagsCount)
{
    LinkProbing::LinkMetricTypeId *typeIdFlags = reinterpret_cast<LinkProbing::LinkMetricTypeId *>(aTypeIdFlags);

    return static_cast<Instance *>(aInstance)->Get<LinkProbing::LinkProbing>().EnhancedAckConfigureRequest(
        *static_cast<const Ip6::Address *>(aDestination), aEnhAckFlags, typeIdFlags, aTypeIdFlagsCount);
}

otError otLinkProbingSendLinkProbe(otInstance *aInstance, const otIp6Address *aDestination, uint8_t aDataLength)
{
    return static_cast<Instance *>(aInstance)->Get<LinkProbing::LinkProbing>().SendLinkProbeTo(
        *static_cast<const Ip6::Address *>(aDestination), aDataLength);
}

void otLinkProbingSetReportCallback(otInstance *                aInstance,
                                    otLinkMetricsReportCallback aCallback,
                                    void *                      aCallbackContext)
{
    static_cast<Instance *>(aInstance)->Get<LinkProbing::LinkProbing>().SetLinkProbingReportCallback(aCallback,
                                                                                                     aCallbackContext);
}
#endif // OPENTHREAD_CONFIG_LINK_PROBE_ENABLE
