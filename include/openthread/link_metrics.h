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
 * @brief
 *   This file defines the OpenThread Link Metrics API.
 */

#ifndef LINK_METRICS_H_
#define LINK_METRICS_H_

#include "openthread-core-config.h"

#include <openthread/ip6.h>
#include <openthread/message.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup linkmetrics   Link Metrics
 *
 * @brief
 *   This module includes functions that control Link Metrics protocol.
 *
 * @{
 *
 */

enum
{
    OT_LINK_METRICS_TYPE_ID_MAX_COUNT = 4, ///< Max TypeIdFlags count in a link metrics query
};

/**
 * This enumeration defines link metric ID.
 *
 */
typedef enum otLinkMetricsId
{
    OT_LINK_METRICS_PDU_COUNT = 0, ///< Layer 2 PDUs Received
    OT_LINK_METRICS_LQI       = 1, ///< Layer 2 LQI
    OT_LINK_METRICS_MARGIN    = 2, ///< Link Margin - RSSI margin above noise floor
    OT_LINK_METRICS_RSSI      = 3, ///< RSSI
} otLinkMetricsId;

/**
 * This enumeration defines link metric type.
 *
 */
typedef enum otLinkMetricsType
{
    OT_LINK_METRICS_METRIC_COUNT_SUMMATION            = 0, ///< Count/summation
    OT_LINK_METRICS_METRIC_EXPONENTIAL_MOVING_AVERAGE = 1, ///< Exponential moving average
} otLinkMetricsType;

/**
 * This structure represents link metric type Id flags.
 */
typedef struct otLinkMetricsTypeId
{
    uint8_t mLinkMetricsId : 3;
    uint8_t mLinkMetricsType : 3;
    uint8_t mLinkMetricsFlagL : 1;
    uint8_t mLinkMetricsFlagE : 1;
} otLinkMetricsTypeId;

/**
 * This structure represents one link metrics item including its type id and value.
 */
typedef struct otLinkMetric
{
    otLinkMetricsTypeId mLinkMetricsTypeId;
    union
    {
        uint8_t  m8;
        uint32_t m32;
    } mLinkMetricValue;
} otLinkMetric;

/**
 * This function sends an MLE Data Request containing Link Metrics Query TLV
 * to query link metrics data. Single Probe or Forward Tracking Series.
 *
 * @param[in]  aInstance            A pointer to an OpenThread instance.
 * @param[in]  aDestination         A pointer to the destination address.
 * @param[in]  aSeriesId            The id of the series to query about, 0 for single probe.
 * @param[in]  aTypeIdFlags         A pointer to an array of Type Id Flags.
 * @param[in]  aTypeIdFlagsCount    The size of the array @p aTypeIdFlags.
 *
 * @retval OT_ERROR_NONE          Successfully sent a link metrics query message.
 * @retval OT_ERROR_NO_BUFS       Insufficient buffers to generate the MLE Data Request message.
 * @retval OT_ERROR_INVALID_ARGS  TypeIdFlags are not valid or exceeds the count limit.
 *
 */
otError otLinkMetricsQuery(otInstance *        aInstance,
                           const otIp6Address *aDestination,
                           uint8_t             aSeriesId,
                           const uint8_t *     aTypeIdFlags,
                           uint8_t             aTypeIdFlagsCount);

/**
 * This function pointer is called when a Link Metrics report is received.
 *
 * @param[in]  aSource      A pointer to the source address.
 * @param[in]  aMetrics     A pointer to the link metrics array.
 * @param[in]  aMetricsNum  The number of link metrics.
 * @param[in]  aContext     A pointer to application-specific context.
 *
 */
typedef void (*otLinkMetricsReportCallback)(const otIp6Address *aSource,
                                            const otLinkMetric *aMetrics,
                                            uint8_t             aMetricsNum,
                                            void *              aContext);

/**
 * This function registers a callback to handle Link Metrics report received.
 *
 * @param[in]  aInstance         A pointer to an OpenThread instance.
 * @param[in]  aCallback         A pointer to a function that is called when link metrics report is received.
 * @param[in]  aCallbackContext  A pointer to application-specific context.
 *
 */
void otLinkMetricsSetReportCallback(otInstance *                aInstance,
                                    otLinkMetricsReportCallback aCallback,
                                    void *                      aCallbackContext);

/**
 * @}
 *
 */

#ifdef __cplusplus
} // extern "C"
#endif

#endif // LINK_METRICS_H_
