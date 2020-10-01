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

/*
 * This structure represents what metrics are specified to query.
 *
 */
typedef struct otLinkMetrics
{
    bool mPduCount : 1;
    bool mLqi : 1;
    bool mLinkMargin : 1;
    bool mRssi : 1;
} otLinkMetrics;

/*
 * This structure represents the result (value) for a Link Metrics query.
 *
 */
typedef struct otLinkMetricsValues
{
    otLinkMetrics mMetrics; ///< Specifies which metrics values are present/included.

    uint32_t mPduCountValue;   ///< The value of Pdu Count.
    uint8_t  mLqiValue;        ///< The value LQI.
    uint8_t  mLinkMarginValue; ///< The value of Link Margin.
    int8_t   mRssiValue;       ///< The value of Rssi.
} otLinkMetricsValues;

/**
 * This function pointer is called when a Link Metrics report is received.
 *
 * @param[in]  aSource         A pointer to the source address.
 * @param[in]  aMetricsValues  A pointer to the Link Metrics values (the query result).
 * @param[in]  aContext        A pointer to application-specific context.
 *
 */
typedef void (*otLinkMetricsReportCallback)(const otIp6Address *       aSource,
                                            const otLinkMetricsValues *aMetricsValues,
                                            void *                     aContext);

/**
 * This function sends an MLE Data Request to query Link Metrics.
 *
 * It could be either Single Probe or Forward Tracking Series.
 *
 * @param[in]  aInstance            A pointer to an OpenThread instance.
 * @param[in]  aDestination         A pointer to the destination address.
 * @param[in]  aSeriesId            The Series ID to query about, 0 for Single Probe.
 * @param[in]  aLinkMetricsFlags    A pointer to flags specifying what metrics to query.
 * @param[in]  aCallback            A pointer to a function that is called when Link Metrics report is received.
 * @param[in]  aCallbackContext     A pointer to application-specific context.
 *
 * @retval OT_ERROR_NONE          Successfully sent a Link Metrics query message.
 * @retval OT_ERROR_NO_BUFS       Insufficient buffers to generate the MLE Data Request message.
 *
 */
otError otLinkMetricsQuery(otInstance *                aInstance,
                           const otIp6Address *        aDestination,
                           uint8_t                     aSeriesId,
                           const otLinkMetrics *       aLinkMetricsFlags,
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
