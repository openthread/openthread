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
 * @brief
 *   This file defines the OpenThread Link Metrics Probing API.
 */

#ifndef LINK_PROBING_H_
#define LINK_PROBING_H_

#include <openthread/ip6.h>
#include <openthread/message.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * This enumeration defines link metric Id.
 *
 */
typedef enum otLinkMetricId
{
    OT_LINK_PDU_COUNT = 0, ///< Layer 2 PDUs Received
    OT_LINK_LQI       = 1, ///< Layer 2 LQI
    OT_LINK_MARGIN    = 2, ///< Link Margin - RSSI margin above noise floor
    OT_LINK_RSSI      = 3, ///< RSSI
    OT_LINK_TX_POWER  = 4, ///< Transmission Output Power
} otLinkMetricId;

/**
 * This enumeration defines link metric value type.
 *
 */
typedef enum otLinkMetricType
{
    OT_LINK_METRIC_COUNT_SUMMATION            = 0, ///< Count/summation
    OT_LINK_METRIC_EXPONENTIAL_MOVING_AVERAGE = 1, ///< Exponential moving average
} otLinkMetricType;

/**
 * This structure represents link metric type Id flags.
 */
typedef struct otLinkMetricTypeId
{
    uint8_t mLinkMetricId : 3;
    uint8_t mLinkMetricType : 3;
    uint8_t mLinkMetricFlagL : 1;
    uint8_t mLinkMetricFlagE : 1;
} otLinkMetricTypeId;

/**
 * This structure represents a link metric including its type id and value.
 */
typedef struct otLinkMetric
{
    otLinkMetricTypeId mLinkMetricTypeId;
    union
    {
        uint8_t  m8;
        uint32_t m32;
    } mLinkMetricValue;
} otLinkMetric;

/**
 * @addtogroup linkprobing   Link Probing
 *
 * @brief
 *   This module includes functions that control link metrics probing protocol.
 *
 * @{
 *
 */

/**
 * This function sends an MLE Data Request containing Link Metrics Query TLV
 * to query link metrics data. Single Probe or Forward Tracking Series.
 *
 * @param[in]  aInstance            A pointer to an OpenThread instance.
 * @param[in]  aDestination         A pointer to destination address.
 * @param[in]  aSeriesId            The id of the series to query about,
 *                                  0 for single probe.
 * @param[in]  aTypeIdFlags         A pointer to an array of Type Id Flags.
 * @param[in]  aTypeIdFlagsCount    A number of Type Id Flags entries.
 *
 * @returns OT_ERROR_NONE if Link Metrics Management Request was sent, otherwise an error code is returned.
 *
 */
otError otLinkProbingQuery(otInstance *        aInstance,
                           const otIp6Address *aDestination,
                           uint8_t             aSeriesId,
                           uint8_t *           aTypeIdFlags,
                           uint8_t             aTypeIdFlagsCount);

/**
 * This function sends an MLE Link Metrics Management Request with forward probing registration.
 *
 * @param[in]  aInstance            A pointer to an OpenThread instance.
 * @param[in]  aDestination         A pointer to destination address.
 * @param[in]  aForwardSeriesId     A Forward Series Id field value.
 * @param[in]  aForwardSeriesFlags  A Forward Series Flags field value.
 * @param[in]  aTypeIdFlags         A pointer to an array of Type Id Flags.
 * @param[in]  aTypeIdFlagsCount    A number of Type Id Flags entries.
 *
 * @returns OT_ERROR_NONE if Link Metrics Management Request was sent, otherwise an error code is returned.
 *
 */
otError otLinkProbingMgmtForward(otInstance *        aInstance,
                                 const otIp6Address *aDestination,
                                 uint8_t             aForwardSeriesId,
                                 uint8_t             aForwardSeriesFlags,
                                 uint8_t *           aTypeIdFlags,
                                 uint8_t             aTypeIdFlagsCount);

/**
 * This function sends an MLE Link Metrics Management Request to configure Enhanced ACK-based Probing.
 *
 * @param[in]  aInstance            A pointer to an OpenThread instance.
 * @param[in]  aDestination         A pointer to destination address.
 * @param[in]  aEnhAckFlags         A flag indicating to register or clear Enhanced ACK Link Metrics configurations. 0
 * to clear, 1 to register, 2-255 are reserved.
 * @param[in]  aTypeIdFlags         A pointer to an array of Type Id Flags.
 * @param[in]  aTypeIdFlagsCount    A number of Type Id Flags entries.
 *
 * @returns OT_ERROR_NONE if Link Metrics Management Request was sent, otherwise an error code is returned.
 *
 */
otError otLinkProbingMgmtEnhancedAck(otInstance *        aInstance,
                                     const otIp6Address *aDestination,
                                     uint8_t             aEnhAckFlags,
                                     uint8_t *           aTypeIdFlags,
                                     uint8_t             aTypeIdFlagsCount);

/**
 * This function sends a single MLE Link Probe message.
 *
 * @param[in]  aInstance     A pointer to an OpenThread instance.
 * @param[in]  aDestination  A pointer to destination address.
 * @param[in]  aDataLength   The length of the Link Probe TLV's data payload(1 - 65).
 *
 * @returns OT_ERROR_NONE if MLE Link Probe was sent, otherwise an error code is returned.
 *
 */
otError otLinkProbingSendLinkProbe(otInstance *aInstance, const otIp6Address *aDestination, uint8_t aDataLength);

/**
 * This function pointer is called when link metrics report is received.
 *
 * @param[in]  aSource      A pointer to the source address.
 * @param[in]  aMetrics     A pointer to the link metrics array.
 * @param[in]  aMetricsNum  The number of link metrics.
 * @param[in]  aContext     A pointer to application-specific context.
 *
 */
typedef void (*otLinkMetricsReportCallback)(const otIp6Address *aSource,
                                            otLinkMetric *      aMetrics,
                                            uint8_t             aMetricsNum,
                                            void *              aContext);

/**
 * This function registers a callback to provide received link metrics report.
 *
 * @param[in]  aInstance         A pointer to an OpenThread instance.
 * @param[in]  aCallback         A pointer to a function that is called when link metrics report is received
 * @param[in]  aCallbackContext  A pointer to application-specific context.
 *
 */
void otLinkProbingSetReportCallback(otInstance *                aInstance,
                                    otLinkMetricsReportCallback aCallback,
                                    void *                      aCallbackContext);

/**
 * @}
 *
 */

#ifdef __cplusplus
} // extern "C"
#endif

#endif // LINK_PROBING_H_
