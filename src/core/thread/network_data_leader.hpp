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
 *   This file includes definitions for manipulating Thread Network Data managed by the Thread Leader.
 */

#ifndef NETWORK_DATA_LEADER_HPP_
#define NETWORK_DATA_LEADER_HPP_

#include "openthread-core-config.h"

#include <stdint.h>

#include "coap/coap.hpp"
#include "common/const_cast.hpp"
#include "common/timer.hpp"
#include "net/ip6_address.hpp"
#include "thread/mle_router.hpp"
#include "thread/network_data.hpp"

namespace ot {

namespace NetworkData {

/**
 * @addtogroup core-netdata-leader
 *
 * @brief
 *   This module includes definitions for manipulating Thread Network Data managed by the Thread Leader.
 *
 * @{
 *
 */

/**
 * Implements the Thread Network Data maintained by the Leader.
 *
 */
class LeaderBase : public MutableNetworkData
{
public:
    /**
     * Initializes the object.
     *
     * @param[in]  aInstance     A reference to the OpenThread instance.
     *
     */
    explicit LeaderBase(Instance &aInstance)
        : MutableNetworkData(aInstance, mTlvBuffer, 0, sizeof(mTlvBuffer))
        , mMaxLength(0)
    {
        Reset();
    }

    /**
     * Reset the Thread Network Data.
     *
     */
    void Reset(void);

    /**
     * Returns the maximum observed Network Data length since OT stack initialization or since the last
     * call to `ResetMaxLength()`.
     *
     * @returns The maximum observed Network Data length (high water mark for Network Data length).
     *
     */
    uint8_t GetMaxLength(void) const { return mMaxLength; }

    /**
     * Resets the tracked maximum Network Data Length.
     *
     * @sa GetMaxLength
     *
     */
    void ResetMaxLength(void) { mMaxLength = GetLength(); }

    /**
     * Returns the Data Version value for a type (full set or stable subset).
     *
     * @param[in] aType   The Network Data type (full set or stable subset).
     *
     * @returns The Data Version value for @p aType.
     *
     */
    uint8_t GetVersion(Type aType) const { return (aType == kFullSet) ? mVersion : mStableVersion; }

    /**
     * Retrieves the 6LoWPAN Context information based on a given IPv6 address.
     *
     * @param[in]   aAddress  A reference to an IPv6 address.
     * @param[out]  aContext  A reference to 6LoWPAN Context information.
     *
     * @retval kErrorNone       Successfully retrieved 6LoWPAN Context information.
     * @retval kErrorNotFound   Could not find the 6LoWPAN Context information.
     *
     */
    Error GetContext(const Ip6::Address &aAddress, Lowpan::Context &aContext) const;

    /**
     * Retrieves the 6LoWPAN Context information based on a given Context ID.
     *
     * @param[in]   aContextId  The Context ID value.
     * @param[out]  aContext    A reference to the 6LoWPAN Context information.
     *
     * @retval kErrorNone       Successfully retrieved 6LoWPAN Context information.
     * @retval kErrorNotFound   Could not find the 6LoWPAN Context information.
     *
     */
    Error GetContext(uint8_t aContextId, Lowpan::Context &aContext) const;

    /**
     * Indicates whether or not the given IPv6 address is on-mesh.
     *
     * @param[in]  aAddress  A reference to an IPv6 address.
     *
     * @retval TRUE   If @p aAddress is on-link.
     * @retval FALSE  If @p aAddress if not on-link.
     *
     */
    bool IsOnMesh(const Ip6::Address &aAddress) const;

    /**
     * Performs a route lookup using the Network Data.
     *
     * @param[in]   aSource             A reference to the IPv6 source address.
     * @param[in]   aDestination        A reference to the IPv6 destination address.
     * @param[out]  aRloc16             A reference to return the RLOC16 for the selected route.
     *
     * @retval kErrorNone      Successfully found a route. @p aRloc16 is updated.
     * @retval kErrorNoRoute   No valid route was found.
     *
     */
    Error RouteLookup(const Ip6::Address &aSource, const Ip6::Address &aDestination, uint16_t &aRloc16) const;

    /**
     * Is used by non-Leader devices to set Network Data by reading it from a message from Leader.
     *
     * @param[in]  aVersion        The Version value.
     * @param[in]  aStableVersion  The Stable Version value.
     * @param[in]  aType           The Network Data type to set, the full set or stable subset.
     * @param[in]  aMessage        A reference to the message.
     * @param[in]  aOffset         The offset in @p aMessage pointing to start of Network Data.
     * @param[in]  aLength         The length of Network Data.
     *
     * @retval kErrorNone   Successfully set the network data.
     * @retval kErrorParse  Network Data in @p aMessage is not valid.
     *
     */
    Error SetNetworkData(uint8_t        aVersion,
                         uint8_t        aStableVersion,
                         Type           aType,
                         const Message &aMessage,
                         uint16_t       aOffset,
                         uint16_t       aLength);

    /**
     * Gets the Commissioning Dataset from Network Data.
     *
     * @param[out] aDataset    A reference to a `MeshCoP::CommissioningDataset` to populate.
     *
     */
    void GetCommissioningDataset(MeshCoP::CommissioningDataset &aDataset) const;

    /**
     * Searches for given sub-TLV in Commissioning Data TLV.
     *
     * @tparam SubTlvType    The sub-TLV type to search for.
     *
     * @returns A pointer to the Commissioning Data Sub-TLV or `nullptr` if no such sub-TLV exists.
     *
     */
    template <typename SubTlvType> const SubTlvType *FindInCommissioningData(void) const
    {
        return As<SubTlvType>(FindCommissioningDataSubTlv(SubTlvType::kType));
    }

    /**
     * Searches for given sub-TLV in Commissioning Data TLV.
     *
     * @tparam SubTlvType    The sub-TLV type to search for.
     *
     * @returns A pointer to the Commissioning Data Sub-TLV or `nullptr` if no such sub-TLV exists.
     *
     */
    template <typename SubTlvType> SubTlvType *FindInCommissioningData(void)
    {
        return As<SubTlvType>(FindCommissioningDataSubTlv(SubTlvType::kType));
    }

    /**
     * Finds and reads the Commissioning Session ID in Commissioning Data TLV.
     *
     * @param[out] aSessionId  A reference to return the read session ID.
     *
     * @retval kErrorNone       Successfully read the session ID, @p aSessionId is updated.
     * @retval kErrorNotFound   Did not find Session ID sub-TLV.
     * @retval kErrorParse      Failed to parse Commissioning Data TLV (invalid format).
     *
     */
    Error FindCommissioningSessionId(uint16_t &aSessionId) const;

    /**
     * Finds and reads the Border Agent RLOC16 in Commissioning Data TLV.
     *
     * @param[out] aRloc16  A reference to return the read RLOC16.
     *
     * @retval kErrorNone       Successfully read the Border Agent RLOC16, @p aRloc16 is updated.
     * @retval kErrorNotFound   Did not find Border Agent RLOC16 sub-TLV.
     * @retval kErrorParse      Failed to parse Commissioning Data TLV (invalid format).
     *
     */
    Error FindBorderAgentRloc(uint16_t &aRloc16) const;

    /**
     * Finds and reads the Joiner UDP Port in Commissioning Data TLV.
     *
     * @param[out] aPort  A reference to return the read port number.
     *
     * @retval kErrorNone       Successfully read the Joiner UDP port, @p aPort is updated.
     * @retval kErrorNotFound   Did not find Joiner UDP Port sub-TLV.
     * @retval kErrorParse      Failed to parse Commissioning Data TLV (invalid format).
     *
     */
    Error FindJoinerUdpPort(uint16_t &aPort) const;

    /**
     * Finds and read the Steering Data in Commissioning Data TLV.
     *
     * @param[out] aSteeringData  A reference to return the read Steering Data.
     *
     * @retval kErrorNone       Successfully read the Steering Data, @p aSteeringData is updated.
     * @retval kErrorNotFound   Did not find Steering Data sub-TLV.
     *
     */
    Error FindSteeringData(MeshCoP::SteeringData &aSteeringData) const;

    /**
     * Indicates whether or not the Commissioning Data TLV indicates Joining is allowed.
     *
     * Joining is allowed if a Border Agent Locator TLV exist and the Steering Data TLV is non-zero.
     *
     * @retval TRUE    If joining is allowed.
     * @retval FALSE   If joining is not allowed.
     *
     */
    bool IsJoiningAllowed(void) const;

    /**
     * Checks if the steering data includes a Joiner.
     *
     * @param[in]  aEui64             A reference to the Joiner's IEEE EUI-64.
     *
     * @retval kErrorNone          @p aEui64 is in the bloom filter.
     * @retval kErrorInvalidState  No steering data present.
     * @retval kErrorNotFound      @p aEui64 is not in the bloom filter.
     *
     */
    Error SteeringDataCheckJoiner(const Mac::ExtAddress &aEui64) const;

    /**
     * Checks if the steering data includes a Joiner with a given discerner value.
     *
     * @param[in]  aDiscerner         A reference to the Joiner Discerner.
     *
     * @retval kErrorNone          @p aDiscerner is in the bloom filter.
     * @retval kErrorInvalidState  No steering data present.
     * @retval kErrorNotFound      @p aDiscerner is not in the bloom filter.
     *
     */
    Error SteeringDataCheckJoiner(const MeshCoP::JoinerDiscerner &aDiscerner) const;

    /**
     * Gets the Service ID for the specified service.
     *
     * @param[in]  aEnterpriseNumber  Enterprise Number (IANA-assigned) for Service TLV
     * @param[in]  aServiceData       The Service Data.
     * @param[in]  aServerStable      The Stable flag value for Server TLV.
     * @param[out] aServiceId         A reference where to put the Service ID.
     *
     * @retval kErrorNone       Successfully got the Service ID.
     * @retval kErrorNotFound   The specified service was not found.
     *
     */
    Error GetServiceId(uint32_t           aEnterpriseNumber,
                       const ServiceData &aServiceData,
                       bool               aServerStable,
                       uint8_t           &aServiceId) const;

    /**
     * Gets the preferred NAT64 prefix from network data.
     *
     * The returned prefix is the highest preference external route entry in Network Data with NAT64 flag set. If there
     * are multiple such entries the first one is returned.
     *
     * @param[out] aConfig      A reference to an `ExternalRouteConfig` to return the prefix.
     *
     * @retval kErrorNone       Found the NAT64 prefix and updated @p aConfig.
     * @retval kErrorNotFound   Could not find any NAT64 entry.
     *
     */
    Error GetPreferredNat64Prefix(ExternalRouteConfig &aConfig) const;

protected:
    void                        SignalNetDataChanged(void);
    const CommissioningDataTlv *FindCommissioningData(void) const;
    CommissioningDataTlv *FindCommissioningData(void) { return AsNonConst(AsConst(this)->FindCommissioningData()); }
    const MeshCoP::Tlv   *FindCommissioningDataSubTlv(uint8_t aType) const;
    MeshCoP::Tlv         *FindCommissioningDataSubTlv(uint8_t aType)
    {
        return AsNonConst(AsConst(this)->FindCommissioningDataSubTlv(aType));
    }

    uint8_t mStableVersion;
    uint8_t mVersion;

private:
    using FilterIndexes = MeshCoP::SteeringData::HashBitIndexes;

    const PrefixTlv *FindNextMatchingPrefixTlv(const Ip6::Address &aAddress, const PrefixTlv *aPrevTlv) const;

    template <typename EntryType> int CompareRouteEntries(const EntryType &aFirst, const EntryType &aSecond) const;
    int                               CompareRouteEntries(int8_t   aFirstPreference,
                                                          uint16_t aFirstRloc,
                                                          int8_t   aSecondPreference,
                                                          uint16_t aSecondRloc) const;

    Error ExternalRouteLookup(uint8_t aDomainId, const Ip6::Address &aDestination, uint16_t &aRloc16) const;
    Error DefaultRouteLookup(const PrefixTlv &aPrefix, uint16_t &aRloc16) const;
    Error SteeringDataCheck(const FilterIndexes &aFilterIndexes) const;
    void  GetContextForMeshLocalPrefix(Lowpan::Context &aContext) const;
    Error ReadCommissioningDataUint16SubTlv(MeshCoP::Tlv::Type aType, uint16_t &aValue) const;

    uint8_t mTlvBuffer[kMaxSize];
    uint8_t mMaxLength;
};

/**
 * @}
 */

} // namespace NetworkData
} // namespace ot

#if OPENTHREAD_MTD
namespace ot {
namespace NetworkData {
class Leader : public LeaderBase
{
public:
    using LeaderBase::LeaderBase;
};
} // namespace NetworkData
} // namespace ot
#elif OPENTHREAD_FTD
#include "network_data_leader_ftd.hpp"
#else
#error "Please define OPENTHREAD_MTD=1 or OPENTHREAD_FTD=1"
#endif

#endif // NETWORK_DATA_LEADER_HPP_
