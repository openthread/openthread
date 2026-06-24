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
 *   This file includes definitions for generating and processing MLE TLVs.
 */

#ifndef OT_CORE_THREAD_MLE_TLVS_HPP_
#define OT_CORE_THREAD_MLE_TLVS_HPP_

#include "openthread-core-config.h"

#include "common/encoding.hpp"
#include "common/message.hpp"
#include "common/preference.hpp"
#include "common/tlvs.hpp"
#include "meshcop/timestamp.hpp"
#include "net/ip6_address.hpp"
#include "thread/link_metrics_tlvs.hpp"
#include "thread/mle_types.hpp"

namespace ot {

namespace Mle {

/**
 * @addtogroup core-mle-tlvs
 *
 * @brief
 *   This module includes definitions for generating and processing MLE TLVs.
 *
 * @{
 */

/**
 * Implements MLE TLV generation and parsing.
 */
OT_TOOL_PACKED_BEGIN
class Tlv : public ot::Tlv
{
public:
    /**
     * MLE TLV Types.
     */
    enum Type : uint8_t
    {
        kSourceAddress         = 0,  ///< Source Address TLV
        kMode                  = 1,  ///< Mode TLV
        kTimeout               = 2,  ///< Timeout TLV
        kChallenge             = 3,  ///< Challenge TLV
        kResponse              = 4,  ///< Response TLV
        kLinkFrameCounter      = 5,  ///< Link-Layer Frame Counter TLV
        kLinkQuality           = 6,  ///< Link Quality TLV
        kNetworkParameter      = 7,  ///< Network Parameter TLV
        kMleFrameCounter       = 8,  ///< MLE Frame Counter TLV
        kRoute                 = 9,  ///< Route64 TLV
        kAddress16             = 10, ///< Address16 TLV
        kLeaderData            = 11, ///< Leader Data TLV
        kNetworkData           = 12, ///< Network Data TLV
        kTlvRequest            = 13, ///< TLV Request TLV
        kScanMask              = 14, ///< Scan Mask TLV
        kConnectivity          = 15, ///< Connectivity TLV
        kLinkMargin            = 16, ///< Link Margin TLV
        kStatus                = 17, ///< Status TLV
        kVersion               = 18, ///< Version TLV
        kAddressRegistration   = 19, ///< Address Registration TLV
        kChannel               = 20, ///< Channel TLV
        kPanId                 = 21, ///< PAN ID TLV
        kActiveTimestamp       = 22, ///< Active Timestamp TLV
        kPendingTimestamp      = 23, ///< Pending Timestamp TLV
        kActiveDataset         = 24, ///< Active Operational Dataset TLV
        kPendingDataset        = 25, ///< Pending Operational Dataset TLV
        kDiscovery             = 26, ///< Thread Discovery TLV
        kSupervisionInterval   = 27, ///< Supervision Interval TLV
        kWakeupChannel         = 74, ///< Wakeup Channel TLV
        kCslChannel            = 80, ///< CSL Channel TLV
        kCslTimeout            = 85, ///< CSL Timeout TLV
        kCslClockAccuracy      = 86, ///< CSL Clock Accuracy TLV
        kLinkMetricsQuery      = 87, ///< Link Metrics Query TLV
        kLinkMetricsManagement = 88, ///< Link Metrics Management TLV
        kLinkMetricsReport     = 89, ///< Link Metrics Report TLV
        kLinkProbe             = 90, ///< Link Probe TLV

        /**
         * Applicable/Required only when time synchronization service
         * (`OPENTHREAD_CONFIG_TIME_SYNC_ENABLE`) is enabled.
         */
        kTimeRequest   = 252, ///< Time Request TLV
        kTimeParameter = 253, ///< Time Parameter TLV
        kXtalAccuracy  = 254, ///< XTAL Accuracy TLV

        kInvalid = 255,
    };

    /**
     * Returns the Type value.
     *
     * @returns The Type value.
     */
    Type GetType(void) const { return static_cast<Type>(ot::Tlv::GetType()); }

    /**
     * Sets the Type value.
     *
     * @param[in]  aType  The Type value.
     */
    void SetType(Type aType) { ot::Tlv::SetType(static_cast<uint8_t>(aType)); }

} OT_TOOL_PACKED_END;

/**
 * Defines Source Address TLV constants and types.
 */
typedef UintTlvInfo<Tlv::kSourceAddress, uint16_t> SourceAddressTlv;

/**
 * Defines Mode TLV constants and types.
 */
typedef UintTlvInfo<Tlv::kMode, uint8_t> ModeTlv;

/**
 * Defines Timeout TLV constants and types.
 */
typedef UintTlvInfo<Tlv::kTimeout, uint32_t> TimeoutTlv;

/**
 * Defines Challenge TLV constants and types.
 */
typedef TlvInfo<Tlv::kChallenge> ChallengeTlv;

/**
 * Defines Response TLV constants and types.
 */
typedef TlvInfo<Tlv::kResponse> ResponseTlv;

/**
 * Defines Link Frame Counter TLV constants and types.
 */
typedef UintTlvInfo<Tlv::kLinkFrameCounter, uint32_t> LinkFrameCounterTlv;

/**
 * Defines MLE Frame Counter TLV constants and types.
 */
typedef UintTlvInfo<Tlv::kMleFrameCounter, uint32_t> MleFrameCounterTlv;

/**
 * Defines Address16 TLV constants and types.
 */
typedef UintTlvInfo<Tlv::kAddress16, uint16_t> Address16Tlv;

/**
 * Defines Network Data TLV constants and types.
 */
typedef TlvInfo<Tlv::kNetworkData> NetworkDataTlv;

/**
 * Defines TLV Request TLV constants and types.
 */
typedef TlvInfo<Tlv::kTlvRequest> TlvRequestTlv;

/**
 * Defines Link Margin TLV constants and types.
 */
typedef UintTlvInfo<Tlv::kLinkMargin, uint8_t> LinkMarginTlv;

/**
 * Defines Status TLV constants and types.
 */
typedef UintTlvInfo<Tlv::kStatus, uint8_t> StatusTlv;

/**
 * Defines Version TLV constants and types.
 */
typedef UintTlvInfo<Tlv::kVersion, uint16_t> VersionTlv;

/**
 * Defines PAN ID TLV constants and types.
 */
typedef UintTlvInfo<Tlv::kPanId, uint16_t> PanIdTlv;

/**
 * Defines Active Timestamp TLV constants and types.
 */
typedef SimpleTlvInfo<Tlv::kActiveTimestamp, MeshCoP::Timestamp> ActiveTimestampTlv;

/**
 * Defines Pending Timestamp TLV constants and types.
 */
typedef SimpleTlvInfo<Tlv::kPendingTimestamp, MeshCoP::Timestamp> PendingTimestampTlv;

/**
 * Defines Timeout TLV constants and types.
 */
typedef UintTlvInfo<Tlv::kSupervisionInterval, uint16_t> SupervisionIntervalTlv;

/**
 * Defines CSL Timeout TLV constants and types.
 */
typedef UintTlvInfo<Tlv::kCslTimeout, uint32_t> CslTimeoutTlv;

/**
 * Defines XTAL Accuracy TLV constants and types.
 */
typedef UintTlvInfo<Tlv::kXtalAccuracy, uint16_t> XtalAccuracyTlv;

/**
 * Defines Route TLV constants and types.
 */
class RouteTlv : public TlvInfo<Tlv::kRoute>
{
#if !OPENTHREAD_CONFIG_MLE_LONG_ROUTES_ENABLE
    typedef uint8_t EntryType;
#else
    typedef uint16_t EntryType;
#endif

public:
    /**
     * Represents the parsed Route TLV data.
     */
    class Data
    {
    public:
        /**
         * Represents a single route entry.
         */
        class Entry
        {
            friend class Data;

        public:
            /**
             * Gets the Router ID.
             *
             * @returns The Router ID.
             */
            uint8_t GetRouterId(void) const { return mRouterId; }

            /**
             * Gets the Route Cost value.
             *
             * @returns The Route Cost value.
             */
            uint8_t GetRouteCost(void) const
            {
                return static_cast<uint8_t>(ReadBits<EntryType, kRouteCostMask>(mRouteData));
            }

            /**
             * Gets the Link Quality Out value.
             *
             * @returns The Link Quality Out value.
             */
            LinkQuality GetLinkQualityOut(void) const
            {
                return static_cast<LinkQuality>(ReadBits<EntryType, kLinkQualityOutMask>(mRouteData));
            }

            /**
             * Gets the Link Quality In value.
             *
             * @returns The Link Quality In value.
             */
            LinkQuality GetLinkQualityIn(void) const
            {
                return static_cast<LinkQuality>(ReadBits<EntryType, kLinkQualityInMask>(mRouteData));
            }

            /**
             * Indicates whether the entry matches a given Router ID.
             *
             * @param[in] aRouterId  The Router ID to match against.
             *
             * @retval TRUE   The entry matches @p aRouterId.
             * @retval FALSE  The entry does not match @p aRouterId.
             */
            bool Matches(uint8_t aRouterId) const { return (mRouterId == aRouterId); }

        private:
            uint8_t   mRouterId;
            EntryType mRouteData;
        };

        /**
         * Represents an array of route entries.
         */
        typedef Array<Entry, kMaxRouters> EntryArray;

        /**
         * Parses the Route TLV data from a given message and offset range.
         *
         * @param[in] aMessage      The message to parse from.
         * @param[in] aOffsetRange  The offset range within the message to read from.
         *
         * @retval kErrorNone   Successfully parsed the Route TLV data.
         * @retval kErrorParse  Failed to parse the Route TLV data.
         */
        Error ParseFrom(const Message &aMessage, const OffsetRange &aOffsetRange);

        /**
         * Gets the Router ID Sequence.
         *
         * @returns The Router ID Sequence.
         */
        uint8_t GetRouterIdSequence(void) const { return mIdSequence; }

        /**
         * Gets the array of route entries.
         *
         * @returns The array of route entries.
         */
        const EntryArray &GetEntries(void) const { return mEntries; }

        /**
         * Indicates whether a given Router ID is allocated in the route data.
         *
         * @param[in] aRouterId  The Router ID to check.
         *
         * @retval TRUE   The Router ID is allocated.
         * @retval FALSE  The Router ID is not allocated.
         */
        bool IsAllocated(uint8_t aRouterId) const;

        /**
         * Indicates whether the Route TLV data is a singleton (at most one router is allocated).
         *
         * @retval TRUE   It is a singleton.
         * @retval FALSE  It is not a singleton.
         */
        bool IsSingleton(void) const { return mEntries.GetLength() <= 1; }

        /**
         * Determines the Router ID Mask from the route data.
         *
         * @param[out] aRouterIdMask  A reference to a `RouterIdMask` to populate.
         */
        void DetermineRouterIdMask(RouterIdMask &aRouterIdMask) const;

    private:
        uint8_t    mIdSequence;
        EntryArray mEntries;
    };

#if !OPENTHREAD_CONFIG_MLE_LONG_ROUTES_ENABLE
    /**
     * Appends a Route Data entry (Link Quality In/Out and Route Cost) to a message.
     *
     * @param[in]  aMessage        The message to append to.
     * @param[in]  aLqIn           The Link Quality In value.
     * @param[in]  aLqOut          The Link Quality Out value.
     * @param[in]  aRouteCost      The Route Cost value.
     *
     * @retval kErrorNone      Successfully appended the data.
     * @retval kErrorNoBufs    Insufficient available buffers to grow the message.
     */
    static Error AppendRouteDataEntry(Message &aMessage, LinkQuality aLqIn, LinkQuality aLqOut, uint8_t aRouteCost);
#else
    /**
     * Appends a Route Data entry (Link Quality In/Out and Route Cost) to a message.
     *
     * Under `OPENTHREAD_CONFIG_MLE_LONG_ROUTES_ENABLE`, each route data entry uses 1.5 bytes (12 bits). Two entries
     * are packed into 3 bytes. @p aIsEven is used to indicate whether this is an even (first) or an odd (second) entry.
     *
     * @param[in]  aMessage        The message to append to.
     * @param[in]  aLqIn           The Link Quality In value.
     * @param[in]  aLqOut          The Link Quality Out value.
     * @param[in]  aRouteCost      The Route Cost value.
     * @param[in]  aIsEven         Indicates whether this is an even (first) entry.
     *
     * @retval kErrorNone      Successfully appended the data.
     * @retval kErrorNoBufs    Insufficient available buffers to grow the message.
     * @retval kErrorParse     Message length is invalid for parsing route data.
     */
    static Error AppendRouteDataEntry(Message    &aMessage,
                                      LinkQuality aLqIn,
                                      LinkQuality aLqOut,
                                      uint8_t     aRouteCost,
                                      bool        aIsEven);
#endif

private:
#if !OPENTHREAD_CONFIG_MLE_LONG_ROUTES_ENABLE
    //   7   6   5   4   3   2   1   0
    // +---+---+---+---+---+---+---+---+
    // | LQOut | LQIn  |  Route Cost   |
    // +---+---+---+---+---+---+---+---+

    static constexpr uint8_t kLinkQualityOutMask = 0x03 << 6;
    static constexpr uint8_t kLinkQualityInMask  = 0x03 << 4;
    static constexpr uint8_t kRouteCostMask      = 0x0f << 0;

#else
    // Under `LOG_ROUTES` feature, Route Data is 12 bits per route
    // (1.5 bytes). The first 4 bits are link qualities (out/in),
    // remaining 8 bits are for the route cost. The even and odd
    // entries are staggered.

    static constexpr uint16_t kEvenEntryMask = 0xfff << 4;
    static constexpr uint16_t kOddEntryMask  = 0xfff << 0;

    static constexpr uint16_t kLinkQualityOutMask = 0x03 << 10;
    static constexpr uint16_t kLinkQualityInMask  = 0x03 << 8;
    static constexpr uint16_t kRouteCostMask      = 0xff << 0;

#endif // OPENTHREAD_CONFIG_MLE_LONG_ROUTES_ENABLE
};

/**
 * Represents Leader Data TLV value.
 */
OT_TOOL_PACKED_BEGIN
class LeaderDataTlvValue
{
public:
    /**
     * Default constructor.
     */
    LeaderDataTlvValue(void) = default;

    /**
     * Initializes the `LeaderDataTlvValue` from a given `LeaderData`.
     *
     * @param[in] aLeaderData  The `LeaderData` info to use for initialization.
     */
    explicit LeaderDataTlvValue(const LeaderData &aLeaderData);

    /**
     * Gets the Leader Data info from TLV value.
     *
     * @param[out] aLeaderData   A reference to output Leader Data info.
     */
    void Get(LeaderData &aLeaderData) const;

private:
    uint32_t mPartitionId;
    uint8_t  mWeighting;
    uint8_t  mDataVersion;
    uint8_t  mStableDataVersion;
    uint8_t  mLeaderRouterId;
} OT_TOOL_PACKED_END;

/**
 * Defines Leader Data TLV constants and types.
 */
typedef SimpleTlvInfo<Tlv::kLeaderData, LeaderDataTlvValue> LeaderDataTlv;

/**
 * Implements Scan Mask TLV generation and parsing.
 */
class ScanMaskTlv : public UintTlvInfo<Tlv::kScanMask, uint8_t>
{
public:
    static constexpr uint8_t kRouterFlag    = 1 << 7; ///< Scan Mask Router Flag.
    static constexpr uint8_t kEndDeviceFlag = 1 << 6; ///< Scan Mask End Device Flag.

    /**
     * Indicates whether or not the Router flag is set.
     *
     * @param[in] aMask   A scan mask value.
     *
     * @retval TRUE   If the Router flag is set.
     * @retval FALSE  If the Router flag is not set.
     */
    static bool IsRouterFlagSet(uint8_t aMask) { return (aMask & kRouterFlag) != 0; }

    /**
     * Indicates whether or not the End Device flag is set.
     *
     * @param[in] aMask   A scan mask value.
     *
     * @retval TRUE   If the End Device flag is set.
     * @retval FALSE  If the End Device flag is not set.
     */
    static bool IsEndDeviceFlagSet(uint8_t aMask) { return (aMask & kEndDeviceFlag) != 0; }
};

/**
 * Represents a Connectivity TLV value.
 */
OT_TOOL_PACKED_BEGIN
class ConnectivityTlvValue
{
public:
    /**
     * Initializes the Connectivity TLV value from a given `Connectivity` object.
     *
     * @param[in] aConnectivity   The `Connectivity` to use for initialization.
     */
    void InitFrom(const Connectivity &aConnectivity);

    /**
     * Gets the connectivity information from the TLV value.
     *
     * @param[out] aConnectivity  A reference to a `Connectivity` object to output the information.
     */
    void GetConnectivity(Connectivity &aConnectivity) const;

    /**
     * Parses a Connectivity TLV value from a given message.
     *
     * The SED Buffer Size and Datagram Count fields are optional in the Connectivity TLV. If not present in the
     * message, this method populates them with their default minimum values defined by Thread specification.
     *
     * @param[in] aMessage        The message to parse from.
     * @param[in] aOffsetRange    The offset range within the message containing the TLV value.
     *
     * @retval kErrorNone   Successfully parsed the TLV value.
     * @retval kErrorParse  Failed to parse the TLV value from the message.
     */
    Error ParseFrom(const Message &aMessage, const OffsetRange &aOffsetRange);

private:
    static constexpr uint8_t kFlagsParentPriorityOffset = 6;
    static constexpr uint8_t kFlagsParentPriorityMask   = (3 << kFlagsParentPriorityOffset);

    static constexpr uint8_t kMinSize = 7; // Exclude the optional `mSedBufferSize` and `mSedDatagramCount`.

    // The default minimum values to use when the optional fields
    // `mSedBufferSize`, `mSedDatagramCount` are not included. These
    // numbers are from Thread Conformance Specification 1.4.1-dr3:
    // "A Thread Router MUST be able to buffer at least one 1280-octet
    // IPv6 datagram destined for an attached SED".

    static constexpr uint16_t kMinSedBufferSize    = 1280;
    static constexpr uint8_t  kMinSedDatagramCount = 1;

    uint8_t  mFlags;
    uint8_t  mLinkQuality3;
    uint8_t  mLinkQuality2;
    uint8_t  mLinkQuality1;
    uint8_t  mLeaderCost;
    uint8_t  mIdSequence;
    uint8_t  mActiveRouters;
    uint16_t mSedBufferSize;
    uint8_t  mSedDatagramCount;
} OT_TOOL_PACKED_END;

/**
 * Defines Connectivity TLV constants.
 */
typedef TlvInfo<Tlv::kConnectivity> ConnectivityTlv;

/**
 * Provides constants and methods for generation and parsing of Address Registration TLV.
 */
class AddressRegistrationTlv : public TlvInfo<Tlv::kAddressRegistration>
{
public:
    /**
     * This constant defines the control byte to use in an uncompressed entry where the full IPv6 address is included in
     * the TLV.
     */
    static constexpr uint8_t kControlByteUncompressed = 0;

    /**
     * Returns the control byte to use in a compressed entry where the 64-prefix is replaced with a
     * 6LoWPAN context identifier.
     *
     * @param[in] aContextId   The 6LoWPAN context ID.
     *
     * @returns The control byte associated with compressed entry with @p aContextId.
     */
    static uint8_t ControlByteFor(uint8_t aContextId) { return kCompressed | (aContextId & kContextIdMask); }

    /**
     * Indicates whether or not an address entry is using compressed format.
     *
     * @param[in] aControlByte  The control byte (the first byte in the entry).
     *
     * @retval TRUE   If the entry uses compressed format.
     * @retval FALSE  If the entry uses uncompressed format.
     */
    static bool IsEntryCompressed(uint8_t aControlByte) { return (aControlByte & kCompressed); }

    /**
     * Gets the context ID in a compressed entry.
     *
     * @param[in] aControlByte  The control byte (the first byte in the entry).
     *
     * @returns The 6LoWPAN context ID.
     */
    static uint8_t GetContextId(uint8_t aControlByte) { return (aControlByte & kContextIdMask); }

    AddressRegistrationTlv(void) = delete;

private:
    static constexpr uint8_t kCompressed    = 1 << 7;
    static constexpr uint8_t kContextIdMask = 0xf;
};

/**
 * Implements Channel TLV value format.
 *
 * This is used by both Channel TLV and CSL Channel TLV.
 */
OT_TOOL_PACKED_BEGIN
class ChannelTlvValue
{
public:
    /**
     * Default constructor.
     */
    ChannelTlvValue(void) = default;

    /**
     * Initializes the `ChannelTlvValue` with a given channel page and channel values.
     *
     * @param[in] aChannelPage   The channel page.
     * @param[in] aChannel       The channel.
     */
    ChannelTlvValue(uint8_t aChannelPage, uint16_t aChannel)
        : mChannelPage(aChannelPage)
        , mChannel(BigEndian::HostSwap16(aChannel))
    {
    }

    /**
     * Initializes the `ChannelTlvValue` with zero channel page and a given channel value.
     *
     * @param[in] aChannel       The channel.
     */
    ChannelTlvValue(uint16_t aChannel)
        : ChannelTlvValue(0, aChannel)
    {
    }

    /**
     * Returns the Channel Page value.
     *
     * @returns The Channel Page value.
     */
    uint8_t GetChannelPage(void) const { return mChannelPage; }

    /**
     * Sets the Channel Page value.
     *
     * @param[in]  aChannelPage  The Channel Page value.
     */
    void SetChannelPage(uint8_t aChannelPage) { mChannelPage = aChannelPage; }

    /**
     * Returns the Channel value.
     *
     * @returns The Channel value.
     */
    uint16_t GetChannel(void) const { return BigEndian::HostSwap16(mChannel); }

    /**
     * Sets the Channel value.
     *
     * @param[in]  aChannel  The Channel value.
     */
    void SetChannel(uint16_t aChannel) { mChannel = BigEndian::HostSwap16(aChannel); }

    /**
     * Sets the Channel and determines and sets the Channel Page from the given channel.
     *
     * @param[in]  aChannel  The Channel value.
     */
    void SetChannelAndPage(uint16_t aChannel);

    /**
     * Indicates whether or not the Channel and Channel Page values are valid.
     *
     * @retval TRUE   If the Channel and Channel Page values are valid.
     * @retval FALSE  If the Channel and Channel Page values are not valid.
     */
    bool IsValid(void) const;

private:
    uint8_t  mChannelPage;
    uint16_t mChannel;
} OT_TOOL_PACKED_END;

/**
 * Defines Channel TLV constants and types.
 */
typedef SimpleTlvInfo<Tlv::kChannel, ChannelTlvValue> ChannelTlv;

/**
 * Defines CSL Channel TLV constants and types.
 */
typedef SimpleTlvInfo<Tlv::kCslChannel, ChannelTlvValue> CslChannelTlv;

#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
/**
 * Defines Time Request TLV constants and types.
 */
typedef TlvInfo<Tlv::kTimeRequest> TimeRequestTlv;

/**
 * Represents a Time Parameter TLV value.
 */
OT_TOOL_PACKED_BEGIN
class TimeParameterTlvValue
{
public:
    /**
     * Returns the time sync period.
     *
     * @returns The time sync period.
     */
    uint16_t GetTimeSyncPeriod(void) const { return BigEndian::HostSwap16(mTimeSyncPeriod); }

    /**
     * Sets the time sync period.
     *
     * @param[in]  aTimeSyncPeriod  The time sync period.
     */
    void SetTimeSyncPeriod(uint16_t aTimeSyncPeriod) { mTimeSyncPeriod = BigEndian::HostSwap16(aTimeSyncPeriod); }

    /**
     * Returns the XTAL accuracy threshold.
     *
     * @returns The XTAL accuracy threshold.
     */
    uint16_t GetXtalThreshold(void) const { return BigEndian::HostSwap16(mXtalThreshold); }

    /**
     * Sets the XTAL accuracy threshold.
     *
     * @param[in]  aXTALThreshold  The XTAL accuracy threshold.
     */
    void SetXtalThreshold(uint16_t aXtalThreshold) { mXtalThreshold = BigEndian::HostSwap16(aXtalThreshold); }

private:
    uint16_t mTimeSyncPeriod;
    uint16_t mXtalThreshold;
} OT_TOOL_PACKED_END;

/**
 * Defines Time Parameter TLV constants and types.
 */
typedef SimpleTlvInfo<Tlv::kTimeParameter, TimeParameterTlvValue> TimeParameterTlv;

#endif // OPENTHREAD_CONFIG_TIME_SYNC_ENABLE

#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE || OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE

/**
 * Represents CSL Clock Accuracy TLV value.
 */
OT_TOOL_PACKED_BEGIN
class CslClockAccuracyTlvValue
{
public:
    /**
     * Default constructor.
     */
    CslClockAccuracyTlvValue(void) = default;

    /**
     * Initializes the TLV value with given clock accuracy and uncertainty.
     *
     * @param[in] aClockAccuracy  The clock accuracy in ppm.
     * @param[in] aUncertainty    The clock uncertainty in units of 10 us.
     */
    CslClockAccuracyTlvValue(uint8_t aClockAccuracy, uint8_t aUncertainty);

    /**
     * Gets the CSL clock accuracy and uncertainty values.
     *
     * @param[out] aAccuracy  A reference to a `Mac::CslAccuracy` to return the values.
     */
    void Get(Mac::CslAccuracy &aAccuracy) const;

private:
    uint8_t mClockAccuracy;
    uint8_t mUncertainty;
} OT_TOOL_PACKED_END;

/**
 * Defines CSL Clock Accuracy TLV constants and types.
 */
typedef SimpleTlvInfo<Tlv::kCslClockAccuracy, CslClockAccuracyTlvValue> CslClockAccuracyTlv;

#endif // OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE || OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE
/**
 * @}
 */

} // namespace Mle

} // namespace ot

#endif // OT_CORE_THREAD_MLE_TLVS_HPP_
