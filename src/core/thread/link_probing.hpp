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
 *   This file includes definitions for link metrics probing protocol.
 */

#ifndef LINK_PROBING_HPP_
#define LINK_PROBING_HPP_

#include "openthread-core-config.h"

#include <openthread/link_probing.h>

#include "common/locator.hpp"
#include "common/timer.hpp"
#include "net/udp6.hpp"
#include "thread/link_metrics_tlvs.hpp"
#include "thread/link_quality.hpp"

#if OPENTHREAD_CONFIG_LINK_PROBE_ENABLE

namespace ot {

class ThreadNetif;

namespace Mle {
class LinkMetricsQueryTlv;
}

namespace LinkProbing {

enum
{
    kMaxTypeIdFlagsCount      = 4,
    kMaxLinkProbingDataLength = 64,

    kFrameTypeLinkProbe = 1 << 4,  // This type is deliberately added to count
                                   // Link Probe, as Link Probe is not on the
                                   // same level with Data, Ack and MacCmd
};

class LinkMetricsInfoEntry
{
public:
    struct SeriesFlag
    {
        uint8_t mMleLinkProbes : 1;
        uint8_t mMacData : 1;
        uint8_t mMacDataRequest : 1;
        uint8_t mAck : 1;
        uint8_t mReserved : 4;
    };

    inline uint8_t GetSeriesId() { return mSeriesId; }

    inline void SetSeriesId(uint8_t aSeriesId) { mSeriesId = aSeriesId; }

    inline void SetSeriesFlags(uint8_t aSeriesFlags) { memset(&mSeriesFlags, aSeriesFlags, sizeof(uint8_t)); }

    inline void SetTypeIdFlags(LinkMetricTypeId *aTypeIdFlags, uint8_t aTypeIdFlagsCount);

    inline bool IsFrameTypeMatch(uint8_t aFrameType);

    void AggregateLinkMetrics(uint8_t aLqi, int8_t aRss);

    inline void Reset();

    inline void GetLinkMetricsReport(int8_t aNoiseFloor, LinkMetricsReportSubTlv *aDstMetrics, uint8_t &aMetricsCount);

    inline void GetLinkMetricsValue(int8_t aNoiseFloor, uint8_t *aContent, uint8_t &aCount);

    inline uint8_t GetTmp() { return mTypeIdFlagsCount; }

private:
    uint8_t          mSeriesId;
    SeriesFlag       mSeriesFlags;
    LinkMetricTypeId mTypeIdFlags[kMaxTypeIdFlagsCount];
    uint8_t          mTypeIdFlagsCount;

    RssAverager mRssAverager;
    LqiAverager mLqiAverager;
    uint8_t     mPsduCount;
};

/**
 * This class contains all the link metrics data about one connection.
 *
 */
class LinkMetricsInfo
{
public:
    enum
    {
        kLinkMetricsStatusSuccess                   = 0,
        kLinkMetricsStatusCannotSupportNewSeries    = 1,
        kLinkMetricsStatusSeriesIdAlreadyRegistered = 2,
        kLinkMetricsStatusSeriesIdNotRecognized     = 3,
        kLinkMetricsStatusNoMatchingFramesReceived  = 4,
        kLinkMetricsStatusOtherError                = 254,
    };

    enum
    {
        kTypeAverageEnumCount             = 0,
        kTypeAverageEnumExponential       = 1,
        kEnhancedAckProbingTypeIdCountMax = 2,
    };

    enum
    {
        SERIES_ID_MAX_SIZE = 10,
    };

    // Configure
    uint8_t RegisterForwardProbing(uint8_t           aSeriesId,
                                   uint8_t           aSeriesFlags,
                                   LinkMetricTypeId *aTypeIdFlags,
                                   uint8_t           aTypeIdFlagsCount);

    uint8_t ConfigureEnhancedAckProbing(Instance &          aInstance,
                                        uint8_t             aEnhancedAckFlags,
                                        LinkMetricTypeId *  aTypeIdFlags,
                                        uint8_t             aTypeIdFlagsCount,
                                        uint16_t            aRloc16,
                                        const otExtAddress &aExtAddress);

    // Aggregate
    void AggregateLinkMetrics(uint8_t aFrameType, uint8_t aLqi, int8_t aRss);

    // Query
    uint8_t GetForwardMetricsReport(uint8_t                  aQueryId,
                                    int8_t                   aNoiseFloor,
                                    LinkMetricsReportSubTlv *aDstMetrics,
                                    uint8_t &                aMetricsCount);

    void GetEnhancedAckMetricsValue(int8_t aNoiseFloor, uint8_t *aContent, uint8_t &aCount);

    bool IsEnhancedAckProbingEnabled() { return mEnhancedAckProbingEnabled; }

    void PrintEntries();

    void Clear();

private:
    LinkMetricsInfoEntry mLinkMetricsInfoEntries[SERIES_ID_MAX_SIZE];
    LinkMetricsInfoEntry mLinkMetricsInfoEnhancedAckEntry;

    bool mEnhancedAckProbingEnabled;

    uint8_t mLinkMetricsInfoEntriesCount;

    void AddLinkMetricsInfoEntry(uint8_t           aSeriesId,
                                 uint8_t           aSeriesFlags,
                                 LinkMetricTypeId *aTypeIdFlags,
                                 uint8_t           aTypeIdFlagsCount);

    uint8_t GetLinkMetricsInfoEntryIndex(uint8_t aSeriesId);

    LinkMetricsInfoEntry *FindLinkMetricsInfoEntry(uint8_t aSeriesId);

    uint8_t RemoveLinkMetricsInfoEntry(uint8_t aSeriesId);

    void RemoveAll();
};

/**
 * @addtogroup core-link-probing
 *
 * @brief
 *   This module includes definitions for Thread link metrics probing protocol.
 *
 * @{
 */

class LinkProbing : public InstanceLocator
{
public:
    /**
     *  Link Metrics parameters.
     *
     */
    enum
    {
        kLinkProbingCount   = 6,    ///< The number of link proble meesages for each tracking serie
        kLinkProbingTimeout = 1000, ///< The timeout value for link probing
    };

    /**
     *  Link Metrics status.
     *
     */
    enum
    {
        kLinkMetricsStatusValid   = 0, ///< Valid status
        kLinkMetricsStatusInvalid = 1, ///< Invalid status
    };

    /**
     * This constructor initializes an instance of the LinkProbing class.
     *
     * @param[in]  aThreadNetif  A reference to the Thread network interface.
     *
     */
    explicit LinkProbing(Instance &aInstance);

    /**
     * This method registers a callback to provide received link probing report.
     *
     * @param[in]  aCallback         A pointer to a function that is called when link probing report is received
     * @param[in]  aCallbackContext  A pointer to application-specific context.
     *
     */
    void SetLinkProbingReportCallback(otLinkMetricsReportCallback aCallback, void *aCallbackContext);

    /**
     * This function sends an MLE Data Request containing Link Metrics Query TLV
     * to query link metrics data. Single Probe or Forward Tracking Series.
     *
     * @param[in]  aDestination      A reference to the IPv6 address of the destination.
     * @param[in]  aSeriesId         The id of the series to query about, 0 for single probe.
     * @param[in]  aTypeIdFlags      A pointer to an array of Type Id Flags.
     * @param[in]  aTypeIdFlagsCount A number of Type Id Flags entries.
     *
     * @retval OT_ERROR_NONE       Successfully sent a link metrics single probe message.
     * @retval OT_ERROR_NO_BUFS    Insufficient buffers to generate single probe message.
     * @retval OT_ERROR_NOT_FOUND  The destination is not a neighbor.
     *
     */
    otError LinkProbeQuery(const Ip6::Address &aDestination,
                           uint8_t             aSeriesId,
                           LinkMetricTypeId *  aTypeIdFlags,
                           uint8_t             aTypeIdFlagsCount);

    /**
     * This function sends a MLE Link Metrics Management Request with forward probing registration.
     *
     * @param[in]  aDestination         A reference to destination address.
     * @param[in]  aForwardSeriesId     A Forward Series Id field value.
     * @param[in]  aForwardSeriesFlags  A Forward Series Flags field value.
     * @param[in]  aTimeout             A Timeout value (in seconds).
     * @param[in]  aTypeIdFlags         A pointer to an array of Type Id Flags.
     * @param[in]  aTypeIdFlagsCount    A number of Type Id Flags entries.
     *
     * @returns OT_ERROR_NONE if Link Metrics Management Request was sent, otherwise an error code is returned.
     *
     */
    otError ForwardMgmtRequest(const Ip6::Address &aDestination,
                               uint8_t             aForwardSeriesId,
                               uint8_t             aForwardSeriesFlags,
                               LinkMetricTypeId *  aTypeIdFlags,
                               uint8_t             aTypeIdFlagsCount);

    /**
     * This function sends a single MLE Link Probe message.
     *
     * @param[in]  aDestination  A pointer to destination address.
     * @param[in]  aDataLength   The length of the Link Probe TLV's data payload(1 - 65).
     *
     * @returns OT_ERROR_NONE if MLE Link Probe was sent, otherwise an error code is returned.
     *
     */
    otError SendLinkProbeTo(const Ip6::Address &aDestination, uint8_t aDataLength);

    /**
     * This method sends an Enhanced ACK based link metrics probing request message.
     *
     * @param[in]  aDestination  A reference to the IPv6 address of the destination.
     *
     * @retval OT_ERROR_NONE           Successfully sent an Enhanced ACK based link metrics probing request message.
     * @retval OT_ERROR_NO_BUFS        Insufficient buffers to generate single probe message.
     * @retval OT_ERROR_NOT_FOUND      The destination is not a neighbor.
     * @retval OT_ERROR_INVALID_STATE  Another link metrics tracking series is under progress.
     *
     */
    otError EnhancedAckConfigureRequest(const Ip6::Address &aDestination,
                                        uint8_t             aEnhAckFlags,
                                        LinkMetricTypeId *  aTypeIdFlags,
                                        uint8_t             aTypeIdFlagsCount);

    /**
     * This method handles the received link probe message.
     *
     * @param[in]  aMessage      A reference to the message.
     * @param[in]  aMessageInfo  A reference to the message info associated with @p aMessage.
     */
    void HandleLinkProbe(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo);

    /**
     * This method handles the received link metrics management request message.
     *
     * @param[in]  aMessage      A reference to the message.
     * @param[in]  aMessageInfo  A reference to the message info associated with @p aMessage.
     */
    void HandleLinkMetricsManagementRequest(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo);

    /**
     * This method handles the received link metrics management response message.
     *
     * @param[in]  aMessage      A reference to the message.
     * @param[in]  aMessageInfo  A reference to the message info associated with @p aMessage.
     */
    void HandleLinkMetricsManagementResponse(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo);

    /**
     * This method handles the received link metrics report.
     *
     * @param[in]  aMessageInfo  A reference to the message info associated with @p aMessage
     * @param[in]  aMessage      A reference to the message.
     * @param[in]  aOffset       The offset in bytes where the metrics report sub-TLVs start.
     * @param[in]  aLength       The length of the metrics report sub-TLVs in bytes.
     *
     */
    void HandleLinkMetricsReport(const Ip6::MessageInfo &aMessageInfo,
                                 const Message &         aMessage,
                                 uint16_t                aOffset,
                                 uint16_t                aLength);

    /**
     * This method appends a Link Metrics Report TLV to a message.
     *
     * @param[in]  aMessage           A reference to the message.
     * @param[in]  aDestination       A reference to the IPv6 address of the link metrics source
     * @param[in]  aLinkMetricsQuery  A pointer to the Link Metrics Query Tlv
     * @param[in]  aLinkInfo          A pointer to the message infomation
     *
     * @retval OT_ERROR_NONE     Successfully appended the Thread Discovery TLV.
     * @retval OT_ERROR_NO_BUFS  Insufficient buffers available to append the Address Registration TLV.
     *
     */
    otError AppendLinkMetricsReport(Message &                       aMessage,
                                    const Ip6::Address &            aSource,
                                    const Mle::LinkMetricsQueryTlv *aLinkMetricsQuery,
                                    const otThreadLinkInfo *        aLinkInfo);

    /**
     * This method handles the link metrics received from Enhanced ACK.
     *
     * @param[in]  aAddr           A pointer to the address of the source.
     * @param[in]  aMetrics        A pointer to the link metrics array.
     * @param[in]  aMetricsCount   The link metrics count in the array.
     *
     */
    void HandleLinkMetrics(const Mac::Address &aAddr, otLinkMetric *aMetrics, uint8_t aMetricsCount);

private:
    static void HandleTimer(Timer &aTimer);
    void        HandleTimer(void);

    otError AppendLinkProbe(Message &aMessage, uint8_t aSeriesId, uint8_t aDataLength);
    otError SendLinkMetricsQuery(uint16_t          aRloc16,
                                 uint8_t           aSeriesId,
                                 LinkMetricTypeId *aTypeIdFlags,
                                 uint8_t           aTypeIdFlagsCount);
    otError SendLinkProbe(uint16_t aRloc16, uint8_t aSeriesId, uint8_t aDataLength);
    otError SendLinkMetricsManagementResponse(const Ip6::Address &aDestination, uint8_t aStatus);

    uint8_t SetDefaultLinkMetricTypeIds(LinkMetricTypeId *aLinkMetricTypeIds);

    otError AppendSingleProbeLinkMetricsReport(const LinkMetricsQueryOptions *aQueryOptions,
                                               const otThreadLinkInfo *       aLinkInfo,
                                               const int8_t                   aNoiseFloor,
                                               Message &                      aMessage,
                                               uint8_t &                      aLength);

    otError AppendForwardTrackingSeriesLinkMetricsReport(const uint8_t    aSeriesId,
                                                         const int8_t     aNoiseFloor,
                                                         LinkMetricsInfo &aLinkMetricsInfo,
                                                         Message &        aMessage,
                                                         uint8_t &        aLength);

    otLinkMetricsReportCallback mLinkMetricsReportCallback;
    void *                      mContext;
};

/**
 * @}
 */

} // namespace LinkProbing
} // namespace ot

#endif // OPENTHREAD_CONFIG_LINK_PROBE_ENABLE

#endif // LINK_PROBING_HPP_
