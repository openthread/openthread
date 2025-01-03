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
 *   This file includes definitions for Thread Link Metrics query and management.
 */

#ifndef LINK_METRICS_HPP_
#define LINK_METRICS_HPP_

#include "openthread-core-config.h"

#if OPENTHREAD_CONFIG_MLE_LINK_METRICS_INITIATOR_ENABLE || OPENTHREAD_CONFIG_MLE_LINK_METRICS_SUBJECT_ENABLE

#if (OPENTHREAD_CONFIG_THREAD_VERSION < OT_THREAD_VERSION_1_2)
#error "Thread 1.2 or higher version is required for OPENTHREAD_CONFIG_MLE_LINK_METRICS_INITIATOR_ENABLE" \
       "and OPENTHREAD_CONFIG_MLE_LINK_METRICS_SUBJECT_ENABLE."
#endif

#include <openthread/link.h>
#include <openthread/link_metrics.h>

#include "common/as_core_type.hpp"
#include "common/callback.hpp"
#include "common/clearable.hpp"
#include "common/locator.hpp"
#include "common/message.hpp"
#include "common/non_copyable.hpp"
#include "common/pool.hpp"
#include "net/ip6_address.hpp"
#include "thread/link_metrics_tlvs.hpp"
#include "thread/link_quality.hpp"

namespace ot {
class Neighbor;
class UnitTester;

namespace LinkMetrics {

/**
 * @addtogroup core-link-metrics
 *
 * @brief
 *   This module includes definitions for Thread Link Metrics query and management.
 *
 * @{
 */

#if OPENTHREAD_CONFIG_MLE_LINK_METRICS_INITIATOR_ENABLE

/**
 * Implements the Thread Link Metrics Initiator.
 *
 * The Initiator makes queries, configures Link Metrics probing at the Subject and generates reports of the results.
 */
class Initiator : public InstanceLocator, private NonCopyable
{
public:
    // Initiator callbacks
    typedef otLinkMetricsReportCallback                ReportCallback;
    typedef otLinkMetricsMgmtResponseCallback          MgmtResponseCallback;
    typedef otLinkMetricsEnhAckProbingIeReportCallback EnhAckProbingIeReportCallback;

    /**
     * Provides the info used for appending MLE Link Metric Query TLV.
     */
    struct QueryInfo : public Clearable<QueryInfo>
    {
        uint8_t mSeriesId;             ///< Series ID.
        uint8_t mTypeIds[kMaxTypeIds]; ///< Type IDs.
        uint8_t mTypeIdCount;          ///< Number of entries in `mTypeIds[]`.
    };

    /**
     * Initializes an instance of the Initiator class.
     *
     * @param[in]  aInstance  A reference to the OpenThread interface.
     */
    explicit Initiator(Instance &aInstance);

    /**
     * Sends an MLE Data Request containing Link Metrics Query TLV to query Link Metrics data.
     *
     * It could be either a Single Probe or a Forward Tracking Series.
     *
     * @param[in]  aDestination       A reference to the IPv6 address of the destination.
     * @param[in]  aSeriesId          The Series ID to query, 0 for single probe.
     * @param[in]  aMetrics           A pointer to metrics to query.
     *
     * @retval kErrorNone             Successfully sent a Link Metrics query message.
     * @retval kErrorNoBufs           Insufficient buffers to generate the MLE Data Request message.
     * @retval kErrorInvalidArgs      Type IDs are not valid or exceed the count limit.
     * @retval kErrorUnknownNeighbor  @p aDestination is not link-local or the neighbor is not found.
     */
    Error Query(const Ip6::Address &aDestination, uint8_t aSeriesId, const Metrics *aMetrics);

    /**
     * Appends MLE Link Metrics Query TLV to a given message.
     *
     * @param[in] aMessage     The message to append to.
     * @param[in] aInfo        The link metrics query info to use to prepare the message.
     *
     * @retval kErrorNone     Successfully appended the TLV to the message.
     * @retval kErrorNoBufs   Insufficient buffers available to append the TLV.
     */
    Error AppendLinkMetricsQueryTlv(Message &aMessage, const QueryInfo &aInfo);

    /**
     * Registers a callback to handle Link Metrics report received.
     *
     * @param[in]  aCallback  A pointer to a function that is called when a Link Metrics report is received.
     * @param[in]  aContext   A pointer to application-specific context.
     */
    void SetReportCallback(ReportCallback aCallback, void *aContext) { mReportCallback.Set(aCallback, aContext); }

    /**
     * Handles the received Link Metrics report contained in @p aMessage.
     *
     * @param[in]  aMessage      A reference to the message.
     * @param[in]  aOffsetRange  The offset range in @p aMessage where the metrics report sub-TLVs are present.
     * @param[in]  aAddress      A reference to the source address of the message.
     */
    void HandleReport(const Message &aMessage, OffsetRange &aOffsetRange, const Ip6::Address &aAddress);

    /**
     * Sends an MLE Link Metrics Management Request to configure/clear a Forward Tracking Series.
     *
     * @param[in] aDestination       A reference to the IPv6 address of the destination.
     * @param[in] aSeriesId          The Series ID.
     * @param[in] aSeriesFlags       The Series Flags info which specify what types of frames are to be accounted.
     * @param[in] aMetrics           A pointer to flags specifying what metrics to query.
     *
     * @retval kErrorNone             Successfully sent a Link Metrics Management Request message.
     * @retval kErrorNoBufs           Insufficient buffers to generate the MLE Link Metrics Management Request message.
     * @retval kErrorInvalidArgs      @p aSeriesId is not within the valid range.
     * @retval kErrorUnknownNeighbor  @p aDestination is not link-local or the neighbor is not found.
     */
    Error SendMgmtRequestForwardTrackingSeries(const Ip6::Address &aDestination,
                                               uint8_t             aSeriesId,
                                               const SeriesFlags  &aSeriesFlags,
                                               const Metrics      *aMetrics);

    /**
     * Registers a callback to handle Link Metrics Management Response received.
     *
     * @param[in]  aCallback A pointer to a function that is called when a Link Metrics Management Response is received.
     * @param[in]  aContext  A pointer to application-specific context.
     */
    void SetMgmtResponseCallback(MgmtResponseCallback aCallback, void *aContext)
    {
        mMgmtResponseCallback.Set(aCallback, aContext);
    }

    /**
     * Sends an MLE Link Metrics Management Request to configure/clear a Enhanced-ACK Based Probing.
     *
     * @param[in] aDestination       A reference to the IPv6 address of the destination.
     * @param[in] aEnhAckFlags       Enh-ACK Flags to indicate whether to register or clear the probing. `0` to clear
     *                               and `1` to register. Other values are reserved.
     * @param[in] aMetrics           A pointer to flags specifying what metrics to query. Should be `nullptr` when
     *                               `aEnhAckFlags` is `0`.
     *
     * @retval kErrorNone             Successfully sent a Link Metrics Management Request message.
     * @retval kErrorNoBufs           Insufficient buffers to generate the MLE Link Metrics Management Request message.
     * @retval kErrorInvalidArgs      @p aEnhAckFlags is not a valid value or @p aMetrics isn't correct.
     * @retval kErrorUnknownNeighbor  @p aDestination is not link-local or the neighbor is not found.
     */
    Error SendMgmtRequestEnhAckProbing(const Ip6::Address &aDestination,
                                       EnhAckFlags         aEnhAckFlags,
                                       const Metrics      *aMetrics);

    /**
     * Registers a callback to handle Link Metrics when Enh-ACK Probing IE is received.
     *
     * @param[in]  aCallback A pointer to a function that is called when Enh-ACK Probing IE is received is received.
     * @param[in]  aContext  A pointer to application-specific context.
     */
    void SetEnhAckProbingCallback(EnhAckProbingIeReportCallback aCallback, void *aContext)
    {
        mEnhAckProbingIeReportCallback.Set(aCallback, aContext);
    }

    /**
     * Handles the received Link Metrics Management Response contained in @p aMessage.
     *
     * @param[in]  aMessage    A reference to the message that contains the Link Metrics Management Response.
     * @param[in]  aAddress    A reference to the source address of the message.
     *
     * @retval kErrorNone     Successfully handled the Link Metrics Management Response.
     * @retval kErrorParse    Cannot parse sub-TLVs from @p aMessage successfully.
     */
    Error HandleManagementResponse(const Message &aMessage, const Ip6::Address &aAddress);

    /**
     * Sends an MLE Link Probe message.
     *
     * @param[in] aDestination    A reference to the IPv6 address of the destination.
     * @param[in] aSeriesId       The Series ID which the Probe message targets at.
     * @param[in] aLength         The length of the data payload in Link Probe TLV, [0, 64].
     *
     * @retval kErrorNone             Successfully sent a Link Probe message.
     * @retval kErrorNoBufs           Insufficient buffers to generate the MLE Link Probe message.
     * @retval kErrorInvalidArgs      @p aSeriesId or @p aLength is not within the valid range.
     * @retval kErrorUnknownNeighbor  @p aDestination is not link-local or the neighbor is not found.
     */
    Error SendLinkProbe(const Ip6::Address &aDestination, uint8_t aSeriesId, uint8_t aLength);

    /**
     * Processes received Enh-ACK Probing IE data.
     *
     * @param[in] aData      A pointer to buffer containing the Enh-ACK Probing IE data.
     * @param[in] aLength    The length of @p aData.
     * @param[in] aNeighbor  The neighbor from which the Enh-ACK Probing IE was received.
     */
    void ProcessEnhAckIeData(const uint8_t *aData, uint8_t aLength, const Neighbor &aNeighbor);

private:
    static constexpr uint8_t kLinkProbeMaxLen = 64; // Max length of data payload in Link Probe TLV.

    Error FindNeighbor(const Ip6::Address &aDestination, Neighbor *&aNeighbor);

    Callback<ReportCallback>                mReportCallback;
    Callback<MgmtResponseCallback>          mMgmtResponseCallback;
    Callback<EnhAckProbingIeReportCallback> mEnhAckProbingIeReportCallback;
};

#endif // OPENTHREAD_CONFIG_MLE_LINK_METRICS_INITIATOR_ENABLE

#if OPENTHREAD_CONFIG_MLE_LINK_METRICS_SUBJECT_ENABLE

/**
 * Implements the Thread Link Metrics Subject.
 *
 * The Subject responds queries with reports, handles Link Metrics Management Requests and Link Probe Messages.
 */
class Subject : public InstanceLocator, private NonCopyable
{
public:
    typedef otLinkMetricsEnhAckProbingIeReportCallback EnhAckProbingIeReportCallback;

    /**
     * Initializes an instance of the Subject class.
     *
     * @param[in]  aInstance  A reference to the OpenThread interface.
     */
    explicit Subject(Instance &aInstance);

    /**
     * Appends a Link Metrics Report to a message according to the Link Metrics query.
     *
     * @param[out]  aMessage           A reference to the message to append report.
     * @param[in]   aRequestMessage    A reference to the message of the Data Request.
     * @param[in]   aNeighbor          A reference to the neighbor who queries the report.
     *
     * @retval kErrorNone         Successfully appended the Thread Discovery TLV.
     * @retval kErrorParse        Cannot parse query sub TLV successfully.
     * @retval kErrorInvalidArgs  QueryId is invalid or any Type ID is invalid.
     */
    Error AppendReport(Message &aMessage, const Message &aRequestMessage, Neighbor &aNeighbor);

    /**
     * Handles the received Link Metrics Management Request contained in @p aMessage and return a status.
     *
     * @param[in]   aMessage     A reference to the message that contains the Link Metrics Management Request.
     * @param[in]   aNeighbor    A reference to the neighbor who sends the request.
     * @param[out]  aStatus      A reference to the status which indicates the handling result.
     *
     * @retval kErrorNone     Successfully handled the Link Metrics Management Request.
     * @retval kErrorParse    Cannot parse sub-TLVs from @p aMessage successfully.
     */
    Error HandleManagementRequest(const Message &aMessage, Neighbor &aNeighbor, Status &aStatus);

    /**
     * Handles the Link Probe contained in @p aMessage.
     *
     * @param[in]   aMessage     A reference to the message that contains the Link Probe Message.
     * @param[out]  aSeriesId    A reference to Series ID that parsed from the message.
     *
     * @retval kErrorNone     Successfully handled the Link Metrics Management Response.
     * @retval kErrorParse    Cannot parse sub-TLVs from @p aMessage successfully.
     */
    Error HandleLinkProbe(const Message &aMessage, uint8_t &aSeriesId);

    /**
     * Frees a SeriesInfo entry that was allocated from the Subject object.
     *
     * @param[in]  aSeries    A reference to the SeriesInfo to free.
     */
    void Free(SeriesInfo &aSeriesInfo);

private:
    // Max number of SeriesInfo that could be allocated by the pool.
#if OPENTHREAD_FTD
    static constexpr uint16_t kMaxSeriesSupported = OPENTHREAD_CONFIG_MLE_LINK_METRICS_MAX_SERIES_SUPPORTED;
#elif OPENTHREAD_MTD
    static constexpr uint16_t kMaxSeriesSupported = OPENTHREAD_CONFIG_MLE_LINK_METRICS_SERIES_MTD;
#endif

    static Error ReadTypeIdsFromMessage(const Message &aMessage, const OffsetRange &aOffsetRange, Metrics &aMetrics);
    static Error AppendReportSubTlvToMessage(Message &aMessage, const MetricsValues &aValues);

    Status ConfigureForwardTrackingSeries(uint8_t        aSeriesId,
                                          uint8_t        aSeriesFlags,
                                          const Metrics &aMetrics,
                                          Neighbor      &aNeighbor);
    Status ConfigureEnhAckProbing(uint8_t aEnhAckFlags, const Metrics &aMetrics, Neighbor &aNeighbor);

    Pool<SeriesInfo, kMaxSeriesSupported> mSeriesInfoPool;
};

#endif // OPENTHREAD_CONFIG_MLE_LINK_METRICS_SUBJECT_ENABLE

uint8_t ScaleLinkMarginToRawValue(uint8_t aLinkMargin);
uint8_t ScaleRawValueToLinkMargin(uint8_t aRawValue);
uint8_t ScaleRssiToRawValue(int8_t aRssi);
int8_t  ScaleRawValueToRssi(uint8_t aRawValue);

/**
 * @}
 */

} // namespace LinkMetrics
} // namespace ot

#endif // OPENTHREAD_CONFIG_MLE_LINK_METRICS_INITIATOR_ENABLE || OPENTHREAD_CONFIG_MLE_LINK_METRICS_SUBJECT_ENABLE

#endif // LINK_METRICS_HPP
