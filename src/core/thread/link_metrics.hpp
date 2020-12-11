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

#if OPENTHREAD_CONFIG_MLE_LINK_METRICS_ENABLE

#include <openthread/ip6.h>
#include <openthread/link.h>

#include "common/code_utils.hpp"
#include "common/locator.hpp"
#include "common/non_copyable.hpp"
#include "common/pool.hpp"

#include "link_metrics_tlvs.hpp"
#include "link_quality.hpp"
#include "topology.hpp"

namespace ot {

/**
 * @addtogroup core-link-metrics
 *
 * @brief
 *   This module includes definitions for Thread Link Metrics query and management.
 *
 * @{
 */

/**
 * This class represents one Series that is being tracked by the Subject.
 *
 * When an Initiator successfully configured a Forward Tracking Series, the Subject would use an instance of this class
 * to track the information of the Series. The Subject has a `Pool` of `LinkMetricsSeriesInfo`. It would allocate one
 * when a new Series comes, and free it when a Series finishes.
 *
 * This class inherits `LinkedListEntry` and each `Neighbor` has a list of `LinkMetricsSeriesInfo` so that the Subject
 * could track per Series initiated by neighbors as long as it has available resources.
 *
 */
class LinkMetricsSeriesInfo : public LinkedListEntry<LinkMetricsSeriesInfo>
{
    friend class LinkedList<LinkMetricsSeriesInfo>;
    friend class LinkedListEntry<LinkMetricsSeriesInfo>;

public:
    ///< This represents Link Probe when filtering frames to be accounted using Series Flag. There's
    ///< already `kFcfFrameData`, `kFcfFrameAck` and `kFcfFrameMacCmd`. This item is added so that we can
    ///< filter a Link Probe for series in the same way as other frames.
    enum
    {
        kSeriesTypeLinkProbe = 0,
    };

    /**
     * This method initializes the object.
     *
     * @param[in]  aSeriesId            The Series ID.
     * @param[in]  aSeriesFlags         A reference to the Series Flags which specify what types of frames are to be
     *                                  accounted.
     * @param[in]  aLinkMetricsFlags    A reference to flags specifying what metrics to query.
     *
     */
    void Init(uint8_t aSeriesId, const SeriesFlags &aSeriesFlags, const otLinkMetrics &aLinkMetrics);

    /**
     * This method gets the Series ID.
     *
     * @returns  The Series ID.
     *
     */
    uint8_t GetSeriesId() const { return mSeriesId; }

    /**
     * This method gets the PDU count.
     *
     * @returns  The PDU count.
     *
     */
    uint32_t GetPduCount() const { return mPduCount; }

    /**
     * This method gets the average LQI.
     *
     * @returns  The average LQI.
     *
     */
    uint8_t GetAverageLqi() const { return mLqiAverager.GetAverage(); }

    /**
     * This method gets the average RSS.
     *
     * @returns  The average RSS.
     *
     */
    int8_t GetAverageRss() const { return mRssAverager.GetAverage(); }

    /**
     * This method aggregates the Link Metrics data of a frame into this series.
     *
     * @param[in]  aFrameType    The type of the frame.
     * @param[in]  aLqi          The LQI value.
     * @param[in]  aRss          Ths RSS value.
     *
     */
    void AggregateLinkMetrics(uint8_t aFrameType, uint8_t aLqi, int8_t aRss);

    /**
     * This methods gets the Link Metrics Flags.
     *
     * @param[out] aLinkMetrics  A reference to a `LinkMetrics` object to get the values.
     *
     */
    void GetLinkMetrics(otLinkMetrics &aLinkMetrics) const;

private:
    LinkMetricsSeriesInfo *mNext;

    uint8_t       mSeriesId;
    SeriesFlags   mSeriesFlags;
    otLinkMetrics mLinkMetrics;
    RssAverager   mRssAverager;
    LqiAverager   mLqiAverager;
    uint32_t      mPduCount;

    bool Matches(const uint8_t &aSeriesId) const { return mSeriesId == aSeriesId; }

    bool IsFrameTypeMatch(uint8_t aFrameType) const;
};

class LinkMetrics : public InstanceLocator, private NonCopyable
{
    friend class Neighbor;

public:
    enum LinkMetricsStatus : uint8_t
    {
        kLinkMetricsStatusSuccess                   = OT_LINK_METRICS_STATUS_SUCCESS,
        kLinkMetricsStatusCannotSupportNewSeries    = OT_LINK_METRICS_STATUS_CANNOT_SUPPORT_NEW_SERIES,
        kLinkMetricsStatusSeriesIdAlreadyRegistered = OT_LINK_METRICS_STATUS_SERIESID_ALREADY_REGISTERED,
        kLinkMetricsStatusSeriesIdNotRecognized     = OT_LINK_METRICS_STATUS_SERIESID_NOT_RECOGNIZED,
        kLinkMetricsStatusNoMatchingFramesReceived  = OT_LINK_METRICS_STATUS_NO_MATCHING_FRAMES_RECEIVED,
        kLinkMetricsStatusOtherError                = OT_LINK_METRICS_STATUS_OTHER_ERROR,
    };

    /**
     * This constructor initializes an instance of the LinkMetrics class.
     *
     * @param[in]  aInstance  A reference to the OpenThread interface.
     *
     */
    explicit LinkMetrics(Instance &aInstance);

    /**
     * This method sends an MLE Data Request containing Link Metrics Query TLV to query Link Metrics data.
     *
     * It could be either a Single Probe or a Forward Tracking Series.
     *
     * @param[in]  aDestination       A reference to the IPv6 address of the destination.
     * @param[in]  aSeriesId          The Series ID to query, 0 for single probe.
     * @param[in]  aLinkMetricsFlags  A pointer to flags specifying what metrics to query.
     *
     * @retval OT_ERROR_NONE              Successfully sent a Link Metrics query message.
     * @retval OT_ERROR_NO_BUFS           Insufficient buffers to generate the MLE Data Request message.
     * @retval OT_ERROR_INVALID_ARGS      TypeIdFlags are not valid or exceed the count limit.
     * @retval OT_ERROR_UNKNOWN_NEIGHBOR  @p aDestination is not link-local or the neighbor is not found.
     *
     */
    otError LinkMetricsQuery(const Ip6::Address & aDestination,
                             uint8_t              aSeriesId,
                             const otLinkMetrics *aLinkMetricsFlags);

    /**
     * This method sends an MLE Link Metrics Management Request to configure/clear a Forward Tracking Series.
     *
     * @param[in] aDestination       A reference to the IPv6 address of the destination.
     * @param[in] aSeriesId          The Series ID.
     * @param[in] aSeriesFlags       A reference to the Series Flags which specify what types of frames are to be
     *                               accounted.
     * @param[in] aLinkMetricsFlags  A pointer to flags specifying what metrics to query.
     *
     * @retval OT_ERROR_NONE              Successfully sent a Link Metrics Management Request message.
     * @retval OT_ERROR_NO_BUFS           Insufficient buffers to generate the MLE Link Metrics Management Request
     *                                    message.
     * @retval OT_ERROR_INVALID_ARGS      @p aSeriesId is not within the valid range.
     * @retval OT_ERROR_UNKNOWN_NEIGHBOR  @p aDestination is not link-local or the neighbor is not found.
     *
     */
    otError SendMgmtRequestForwardTrackingSeries(const Ip6::Address &            aDestination,
                                                 uint8_t                         aSeriesId,
                                                 const otLinkMetricsSeriesFlags &aSeriesFlags,
                                                 const otLinkMetrics *           aLinkMetricsFlags);

    /**
     * This method sends an MLE Link Metrics Management Request to configure/clear a Enhanced-ACK Based Probing.
     *
     * @param[in] aDestination       A reference to the IPv6 address of the destination.
     * @param[in] aEnhAckFlags       Enh-ACK Flags to indicate whether to register or clear the probing. `0` to clear
     *                               and `1` to register. Other values are reserved.
     * @param[in] aLinkMetricsFlags  A pointer to flags specifying what metrics to query. Should be `NULL` when
     *                               `aEnhAckFlags` is `0`.
     *
     * @retval OT_ERROR_NONE              Successfully sent a Link Metrics Management Request message.
     * @retval OT_ERROR_NO_BUFS           Insufficient buffers to generate the MLE Link Metrics Management Request
     *                                    message.
     * @retval OT_ERROR_INVALID_ARGS      @p aEnhAckFlags is not a valid value or @p aLinkMetricsFlags isn't correct.
     * @retval OT_ERROR_UNKNOWN_NEIGHBOR  @p aDestination is not link-local or the neighbor is not found.
     *
     */
    otError SendMgmtRequestEnhAckProbing(const Ip6::Address &     aDestination,
                                         otLinkMetricsEnhAckFlags aEnhAckFlags,
                                         const otLinkMetrics *    aLinkMetricsFlags);

    /**
     * This method sends an MLE Link Probe message.
     *
     * @param[in] aDestination    A reference to the IPv6 address of the destination.
     * @param[in] aSeriesId       The Series ID which the Probe message targets at.
     * @param[in] aLength         The length of the data payload in Link Probe TLV, [0, 64].
     *
     * @retval OT_ERROR_NONE              Successfully sent a Link Probe message.
     * @retval OT_ERROR_NO_BUFS           Insufficient buffers to generate the MLE Link Probe message.
     * @retval OT_ERROR_INVALID_ARGS      @p aSeriesId or @p aLength is not within the valid range.
     * @retval OT_ERROR_UNKNOWN_NEIGHBOR  @p aDestination is not link-local or the neighbor is not found.
     *
     */
    otError SendLinkProbe(const Ip6::Address &aDestination, uint8_t aSeriesId, uint8_t aLength);

    /**
     * This method appends a Link Metrics Report to a message according to the Link Metrics query.
     *
     * @param[out]  aMessage           A reference to the message to append report.
     * @param[in]   aRequestMessage    A reference to the message of the Data Request.
     * @param[in]   aNeighbor          A reference to the neighbor who queries the report.
     *
     * @retval OT_ERROR_NONE          Successfully appended the Thread Discovery TLV.
     * @retval OT_ERROR_PARSE         Cannot parse query sub TLV successfully.
     * @retval OT_ERROR_INVALID_ARGS  QueryId is invalid or any Type ID is invalid.
     *
     */
    otError AppendLinkMetricsReport(Message &aMessage, const Message &aRequestMessage, Neighbor &aNeighbor);

    /**
     * This method handles the received Link Metrics Management Request contained in @p aMessage and return a status.
     *
     * @param[in]   aMessage     A reference to the message that contains the Link Metrics Management Request.
     * @param[in]   aNeighbor    A reference to the neighbor who sends the request.
     * @param[out]  aStatus      A reference to the status which indicates the handling result.
     *
     * @retval OT_ERROR_NONE     Successfully handled the Link Metrics Management Request.
     * @retval OT_ERROR_PARSE    Cannot parse sub-TLVs from @p aMessage successfully.
     *
     */
    otError HandleLinkMetricsManagementRequest(const Message &    aMessage,
                                               Neighbor &         aNeighbor,
                                               LinkMetricsStatus &aStatus);

    /**
     * This method handles the received Link Metrics Management Response contained in @p aMessage.
     *
     * @param[in]  aMessage    A reference to the message that contains the Link Metrics Management Response.
     * @param[in]  aAddress    A reference to the source address of the message.
     *
     * @retval OT_ERROR_NONE     Successfully handled the Link Metrics Management Response.
     * @retval OT_ERROR_PARSE    Cannot parse sub-TLVs from @p aMessage successfully.
     *
     */
    otError HandleLinkMetricsManagementResponse(const Message &aMessage, const Ip6::Address &aAddress);

    /**
     * This method handles the received Link Metrics report contained in @p aMessage.
     *
     * @param[in]  aMessage      A reference to the message.
     * @param[in]  aOffset       The offset in bytes where the metrics report sub-TLVs start.
     * @param[in]  aLength       The length of the metrics report sub-TLVs in bytes.
     * @param[in]  aAddress      A reference to the source address of the message.
     *
     */
    void HandleLinkMetricsReport(const Message &     aMessage,
                                 uint16_t            aOffset,
                                 uint16_t            aLength,
                                 const Ip6::Address &aAddress);

    /**
     * This method handles the Link Probe contained in @p aMessage.
     *
     * @param[in]   aMessage     A reference to the message that contains the Link Probe Message.
     * @param[out]  aSeriesId    A reference to Series ID that parsed from the message.
     *
     * @retval OT_ERROR_NONE     Successfully handled the Link Metrics Management Response.
     * @retval OT_ERROR_PARSE    Cannot parse sub-TLVs from @p aMessage successfully.
     *
     */
    otError HandleLinkProbe(const Message &aMessage, uint8_t &aSeriesId);

    /**
     * This method registers a callback to handle Link Metrics report received.
     *
     * @param[in]  aCallback         A pointer to a function that is called when a Link Metrics report is received.
     * @param[in]  aCallbackContext  A pointer to application-specific context.
     *
     */
    void SetLinkMetricsReportCallback(otLinkMetricsReportCallback aCallback, void *aCallbackContext);

    /**
     * This method registers a callback to handle Link Metrics Management Response received.
     *
     * @param[in]  aCallback         A pointer to a function that is called when a Link Metrics Management Response is
     *                               received.
     * @param[in]  aCallbackContext  A pointer to application-specific context.
     *
     */
    void SetLinkMetricsMgmtResponseCallback(otLinkMetricsMgmtResponseCallback aCallback, void *aCallbackContext);

    void SetLinkMetricsEnhAckProbingCallback(otLinkMetricsEnhAckProbingIeReportCallback aCallback,
                                             void *                                     aCallbackContext);

    void ProcessEnhAckIeData(const uint8_t *aData, uint8_t aLen, const Neighbor &aNeighbor);

private:
    /**
     * TypeIdFlagPdu: 0x0_1_000_000 -> 0x40 ==> L bit set, type = 0 (count/summation), metric-enum = 0 (PDU rxed).
     * TypeIdFlagLqi: 0x0_0_001_001 -> 0x00 ==> L bit not set, type = 1 (exp ave), metric-enum = 1 (LQI).
     * TypeIdFlagLinkMargin: 0x0_0_001_010 -> 0x00 ==> L bit not set, type = 1 (exp ave), metric-enum = 2 (Link Margin).
     * TypeIdFlagRssi: 0x0_0_001_011 -> 0x00 ==> L bit not set, type = 1 (exp ave), metric-enum = 3 (RSSI).
     *
     */
    enum
    {
        kMaxTypeIdFlags = 4,

        kMaxSeriesSupported =
            OPENTHREAD_CONFIG_MLE_LINK_METRICS_MAX_SERIES_SUPPORTED, ///< Max number of LinkMetricsSeriesInfo that could
                                                                     ///< be allocated by the pool.

        kQueryIdSingleProbe = 0, ///< This query ID represents Single Probe.

        kSeriesIdAllSeries = 255, ///< This series ID represents all series.

        kLinkProbeMaxLen = 64, ///< Max length of data payload in Link Probe TLV.
    };

    otLinkMetricsReportCallback                mLinkMetricsReportCallback;
    void *                                     mLinkMetricsReportCallbackContext;
    otLinkMetricsMgmtResponseCallback          mLinkMetricsMgmtResponseCallback;
    void *                                     mLinkMetricsMgmtResponseCallbackContext;
    otLinkMetricsEnhAckProbingIeReportCallback mLinkMetricsEnhAckProbingIeReportCallback;
    void *                                     mLinkMetricsEnhAckProbingIeReportCallbackContext;

    Pool<LinkMetricsSeriesInfo, kMaxSeriesSupported> mLinkMetricsSeriesInfoPool;

    otError SendLinkMetricsQuery(const Ip6::Address &          aDestination,
                                 uint8_t                       aSeriesId,
                                 const LinkMetricsTypeIdFlags *aTypeIdFlags,
                                 uint8_t                       aTypeIdFlagsCount);

    LinkMetricsStatus ConfigureForwardTrackingSeries(uint8_t              aSeriesId,
                                                     const SeriesFlags &  aSeriesFlags,
                                                     const otLinkMetrics &aLinkMetrics,
                                                     Neighbor &           aNeighbor);

    LinkMetricsStatus ConfigureEnhAckProbing(LinkMetricsEnhAckFlags aEnhAckFlags,
                                             const otLinkMetrics &  aLinkMetrics,
                                             Neighbor &             aNeighbor);

    Neighbor *GetNeighborFromLinkLocalAddr(const Ip6::Address &aDestination);

    static otError ReadTypeIdFlagsFromMessage(const Message &aMessage,
                                              uint8_t        aStartPos,
                                              uint8_t        aEndPos,
                                              otLinkMetrics &aLinkMetrics);

    static otError AppendReportSubTlvToMessage(Message &aMessage, uint8_t &aLength, const otLinkMetricsValues &aValues);

    static otError AppendStatusSubTlvToMessage(Message &aMessage, uint8_t &aLength, LinkMetricsStatus aStatus);
};

/**
 * @}
 */

} // namespace ot

#endif // OPENTHREAD_CONFIG_MLE_LINK_METRICS_ENABLE

#endif // LINK_METRICS_HPP
