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
 *   This file includes definitions for Thread link metrics query and management.
 */

#ifndef LINK_METRICS_HPP_
#define LINK_METRICS_HPP_

#include "openthread-core-config.h"

#if OPENTHREAD_CONFIG_MLE_LINK_METRICS_ENABLE

#include <openthread/ip6.h>

#include "common/code_utils.hpp"
#include "common/locator.hpp"

#include "link_metrics_tlvs.hpp"
#include "topology.hpp"

namespace ot {

/**
 * @addtogroup core-link-metrics
 *
 * @brief
 *   This module includes definitions for Thread link metrics query and management.
 *
 * @{
 */

class LinkMetrics : public InstanceLocator
{
public:
    /**
     * This constructor initializes an instance of the LinkMetrics class.
     *
     * @param[in]  aInstance  A reference to the OpenThread interface.
     *
     */
    explicit LinkMetrics(Instance &aInstance);

    /**
     * This function sends an MLE Data Request containing Link Metrics Query TLV to query link metrics data. It could
     * be either a Single Probe or a Forward Tracking Series.
     *
     * @param[in]  aDestination      A pointer to the IPv6 address of the destination.
     * @param[in]  aSeriesId         The id of the series to query, 0 for single probe.
     * @param[in]  aTypeIdFlags      A pointer to an array of Type Id Flags.
     * @param[in]  aTypeIdFlagsCount The number of Type Id Flags entries.
     *
     * @retval OT_ERROR_NONE          Successfully sent a link metrics query message.
     * @retval OT_ERROR_NO_BUFS       Insufficient buffers to generate the MLE Data Request message.
     * @retval OT_ERROR_INVALID_ARGS  TypeIdFlags are not valid or exceeds the count limit.
     *
     */
    otError LinkMetricsQuery(const otIp6Address *aDestination,
                             uint8_t             aSeriesId,
                             const uint8_t *     aTypeIdFlags,
                             uint8_t             aTypeIdFlagsCount);

    /**
     * This method appends a Link Metrics Report to a message according to the link metrics query.
     *
     * @param[out]  aMessage           A reference to the message to append report.
     * @param[in]   aLinkMetricsQuery  A pointer to the Link Metrics Query Tlv
     * @param[in]   aRequestMessage    A reference to the message of the Data Request.
     *
     * @retval OT_ERROR_NONE          Successfully appended the Thread Discovery TLV.
     * @retval OT_ERROR_PARSE         Cannot parse query sub TLV successfully.
     * @retval OT_ERROR_INVALID_ARGS  QueryId is invalid.
     *
     */
    otError AppendLinkMetricsReport(Message &                       aMessage,
                                    const Mle::LinkMetricsQueryTlv *aLinkMetricsQuery,
                                    const Message &                 aRequestMessage);

    /**
     * This method handles the received link metrics report contained in @p aMessage.
     *
     * @param[in]  aMessage      A reference to the message.
     * @param[in]  aOffset       The offset in bytes where the metrics report sub-TLVs start.
     * @param[in]  aLength       The length of the metrics report sub-TLVs in bytes.
     * @param[in]  aAddress      A reference to the  source address of the message.
     *
     */
    void HandleLinkMetricsReport(const Message &     aMessage,
                                 uint16_t            aOffset,
                                 uint16_t            aLength,
                                 const Ip6::Address &aAddress);

    /**
     * This method registers a callback to handle Link Metrics report received.
     *
     * @param[in]  aCallback         A pointer to a function that is called when link probing report is received
     * @param[in]  aCallbackContext  A pointer to application-specific context.
     *
     */
    void SetLinkMetricsReportCallback(otLinkMetricsReportCallback aCallback, void *aCallbackContext);

private:
    otLinkMetricsReportCallback mLinkMetricsReportCallback;
    void *                      mLinkMetricsReportCallbackContext;

    otError SendLinkMetricsQuery(const Ip6::Address &     aDestination,
                                 uint8_t                  aSeriesId,
                                 const LinkMetricsTypeId *aTypeIdFlags,
                                 uint8_t                  aTypeIdFlagsCount);

    otError AppendSingleProbeLinkMetricsReport(Message &                      aMessage,
                                               uint8_t &                      aLength,
                                               const LinkMetricsQueryOptions *aQueryOptions,
                                               const int8_t                   aNoiseFloor,
                                               const Message &                aRequestMessage);

    void SetLinkMetricsTypeIdFromTlv(otLinkMetricsTypeId &aOtTypeId, LinkMetricsTypeId &aTlvTypeId);
};

/**
 * @}
 */

} // namespace ot

#endif // OPENTHREAD_CONFIG_MLE_LINK_METRICS_ENABLE

#endif // LINK_METRICS_HPP
