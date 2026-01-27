/*
 *  Copyright (c) 2025, The OpenThread Authors.
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
 *   This file includes definitions for the IPFIX module (flow observation and metering process).
 */

#ifndef IPFIX_HPP_
#define IPFIX_HPP_

#include "openthread-core-config.h"

#if OPENTHREAD_CONFIG_IPFIX_ENABLE

#include <chrono>
#include <inttypes.h>

#include <net/if.h>
#include <openthread/ip6.h>
#include <openthread/ipfix.h>
#include <openthread/platform/time.h>
#include "common/clearable.hpp"
#include "common/encoding.hpp"
#include "common/linked_list.hpp"
#include "common/locator.hpp"
#include "common/log.hpp"
#include "common/message.hpp"
#include "common/non_copyable.hpp"
#include "common/time.hpp"
#include "crypto/sha256.hpp"

namespace ot {
namespace Ipfix {

typedef otIpfixFlowInfo IpfixFlowInfo; ///< Data structure used to store the IPFIX flow informations.

/**
 * This constant specifies the maximum numbers of flows to register into a period of time (before exporting).
 */

static constexpr uint16_t kMaxFlows = OT_IPFIX_MAX_FLOWS;

/**
 * This constant specifies the number of buckets in the hash table.
 */
static constexpr uint16_t kNbrBuckets = OT_IPFIX_NBR_BUCKETS;

/**
 * Implements the IPFIX functionalities for flow observation and metering (at layer 2 and layer 3).
 */
class IpfixFlowCapture : public InstanceLocator, private NonCopyable
{
public:
    /**
     * Initializes the object.
     *
     * @param[in]  aInstance     A reference to the OpenThread instance.
     */

    explicit IpfixFlowCapture(Instance &aInstance);

    /**
     * Metering process of the IPFIX exporter that meter the layer3 flows (IPv6 traffic flow)
     * and creates or update the corresponding IPFIX flow record in the hashtable.
     *
     * @param[in]  aMessage             A reference to the message (IPv6 message).
     * @param[in]  aObservationPoint    The observation point where the flow is observed.
     *
     */

    void MeterLayer3FlowTraffic(Message &aMessage, otIpfixFlowObservationPoint aObservationPoint);

    /**
     * Metering process of the IPFIX exporter that meter the layer2 flows (link layer traffic flow)
     * and creates or update the corresponding IPFIX flow record in the hashtable.
     *
     * @param[in]  aMacAddrs            Mac Information of the layer2 flow (link layer traffic flow).
     * @param[in]  aMessage             A reference to the message (IPv6 message associated to the layer 2 frame).
     * @param[in]  aObservationPoint    The observation point where the flow is observed.
     *
     */

    void MeterLayer2FlowTraffic(const Mac::Addresses       &aMacAddrs,
                                Message                    &aMessage,
                                otIpfixFlowObservationPoint aObservationPoint);

    // Functions to be used by the IPFIX Exporter

    /**
     * Returns the number of IPFIX flow records currently stored in the hashtable.
     *
     * @returns Number of IPFIX flow records currently stored in the hashtable.
     */

    uint16_t GetFlowCount(void);

    /**
     * Returns the entire content of the hashtable (all IPFIX flow records) into the aFlowBuffer.
     *
     * @param[out] aFlowBuffer  A pointer to the buffer where the IPFIX flow records will be copied.
     */

    void GetFlowTable(IpfixFlowInfo *aFlowBuffer);

    /**
     * Resets the IPFIX hashtable (all the IPFIX flow records are reset).
     */

    void ResetFlowTable(void);

private:
    /**
     *   Number of IPFIX flow records currently stored in the hashtable.
     */

    uint16_t mFlowCount;

    /**
     * Represents an IPFIX flow record entry in the hashtable.
     */

    struct IpfixFlowEntry : public LinkedListEntry<IpfixFlowEntry>
    {
        IpfixFlowInfo   mFlow;    ///< The IPFIX flow record.
        uint32_t        mKeyHash; ///< The 32-bit hash key of the flow record (computed using the flow keys).
        IpfixFlowEntry *mNext;    ///< Pointer to the next IpfixFlowEntry.
    };

    /**
     * Hashtable buckets that are used to store the IPFIX flow records entries.
     */
    LinkedList<IpfixFlowEntry> mBuckets[kNbrBuckets];

    /**
     * Pool used for allocating the IPFIX flow record entries.
     */
    Pool<IpfixFlowEntry, kMaxFlows> mPool;

    /**
     * Computes a hash value based on the flow keys of a given flow (using SHA 256).
     *
     * @param[in] aFlow     The IPFIX flow record corresponding to the flow.
     *
     * @returns  The resulting computed hash key for the flow.
     */

    static uint32_t HashFunction(const IpfixFlowInfo &aFlow);

    /**
     * Determines the source and destination networks for a given flow.
     *
     * @param[in] aFlow              The IPFIX flow record corresponding to the flow.
     * @param[in] aObservationPoint  The observation point where the flow is observed.
     *
     * @retval kErrorNone     Successfully determined the source and destination networks.
     */

    Error GetSourceDestinationNetworks(IpfixFlowInfo &aFlow, otIpfixFlowObservationPoint aObservationPoint);

    /**
     * Determines if a given IPFIX flow record is already in the hashtable or not.
     *
     * @param[in] aFlow         The IPFIX flow record corresponding to the flow.
     * @param[in] aBucketId     The bucket ID where the flow should be stored.
     * @param[in] aHashValue    The hash value computed for the flow (using the flow keys).
     *
     * @returns A pointer to the matching IPFIX flow record entry in the hashtable or the nullptr if it is not found.
     */

    IpfixFlowEntry *FindFlowEntryInHashtable(const IpfixFlowInfo &aFlow, uint16_t aBucketId, uint32_t aHashValue);

    /**
     * Verify if two IPFIX flow records correspond to the same flow.
     *
     * @param[in] aFirstFlow   The first IPFIX flow record.
     * @param[in] aSecondFlow  The second IPFIX flow record.
     *
     * @returns the response of the verification ('true' if they correspond to the same flow , 'false' otherwise).
     */

    static bool VerifyFlowEquality(const IpfixFlowInfo &aFirstFlow, const IpfixFlowInfo &aSecondFlow);

    /**
     * Updates the Layer 3 counters and timestamps of an existing IPFIX flow record.
     *
     * @param[in] aExistingFlowEntry  A referance to the IPFIX flow record entry in the hashtable to be updated.
     * @param[in] aObservedFlow       IPFIX flow information containing the layer 3 updates.
     */

    static void UpdateLayer3FlowEntry(IpfixFlowEntry &aExistingFlowEntry, const IpfixFlowInfo &aObservedFlow);

    /**
     * Updates the Layer 2 counters and timestamps of an existing IPFIX flow record.
     *
     * @param[in] aExistingFlowEntry  A referance to the IPFIX flow record entry in the hashtable to be updated.
     * @param[in] aObservedFlow       IPFIX flow information containing the layer 2 updates.
     */

    static void UpdateLayer2FlowEntry(IpfixFlowEntry &aExistingFlowEntry, const IpfixFlowInfo &aObservedFlow);

    /**
     * Creates a new IPFIX flow record entry in the hashtable for the observed flow.
     *
     * @param[in] aObservedFlow  The IPFIX flow record to be created.
     * @param[in] aBucketId      The bucket ID where the flow record entry should be added.
     * @param[in] aHashValue     The hash value computed for the flow (using the flow keys).
     */

    void CreateFlowEntry(const IpfixFlowInfo &aObservedFlow, uint16_t aBucketId, uint32_t aHashValue);

    /**
     * Debug function used to log the layer 3 informations of a given IPFIX flow record stored in the hashtable.
     *
     * @param[in] aEntry           A referance to the IPFIX flow record entry.
     * @param[in] aAdditionalInfo  Additional information that can be add in the log.
     * @param[in] aBucketId        The bucket ID where the IPFIX flow record entry is stored.
     */
    void LogFlowEntry(const IpfixFlowEntry &aEntry, const char *aAdditionalInfo, uint16_t aBucketId);

    /**
     * Return the name of the protocol ID as a string.
     *
     * @param[in] aProtoId  The IP protocol ID.
     *
     * @returns The name of the protocol ID as a string.
     */

    static const char *GetProtoIdName(uint8_t aProtoId);

    /**
     * Returns the name of the network interface as a string.
     *
     * @param[in] aNetworkInterface  The network interface (ID).
     *
     * @returns The name of the network interface as a string.
     */

    static const char *GetNetworkName(otIpfixFlowInterface aNetworkInterface);
};

/**
 * @}
 */

} // namespace Ipfix
} // namespace ot

#endif // OPENTHREAD_CONFIG_IPFIX_ENABLE
#endif // IPFIX_HPP_