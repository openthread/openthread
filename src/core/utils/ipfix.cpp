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
 *   This file implements the IPFIX module (flow observation and metering process).
 */

#include "ipfix.hpp"

#if OPENTHREAD_CONFIG_IPFIX_ENABLE

#include "instance/instance.hpp"

namespace ot {
namespace Ipfix {

RegisterLogModule("IPFIX");

IpfixFlowCapture::IpfixFlowCapture(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mFlowCount(0)
{
}

void IpfixFlowCapture::MeterLayer3FlowTraffic(Message &aMessage, otIpfixFlowObservationPoint aObservationPoint)
{
    Ip6::Headers headers;

    if (aMessage.GetType() != Message::kTypeIp6)
    {
        LogInfo("Message type problem : The Message is not a IPv6 packet !!");
        return;
    }
    if (headers.ParseFrom(aMessage) != kErrorNone)
    {
        LogInfo("Parsing of IPv6 packet header has failed !!");
        return;
    }

    // Temporary Data strcuture to hold the Layer 3 flow informations

    IpfixFlowInfo observed_flow{};
    observed_flow.mSourceAddress      = headers.GetSourceAddress();
    observed_flow.mDestinationAddress = headers.GetDestinationAddress();
    observed_flow.mIpProto            = headers.GetIpProto();
    observed_flow.mPacketsCount       = 1;
    observed_flow.mBytesCount         = aMessage.GetLength();
    observed_flow.mSourcePort         = 0;
    observed_flow.mDestinationPort    = 0;
    observed_flow.mIcmp6Type          = 0;
    observed_flow.mIcmp6Code          = 0;

    if (headers.IsUdp() || headers.IsTcp() || headers.GetIpProto() == 0)
    {
        observed_flow.mSourcePort      = headers.GetSourcePort();
        observed_flow.mDestinationPort = headers.GetDestinationPort();
    }
    else if (headers.IsIcmp6())
    {
        observed_flow.mIcmp6Type = headers.GetIcmpHeader().GetType();
        observed_flow.mIcmp6Code = headers.GetIcmpHeader().GetCode();
    }

    // Get the time the Layer 3 flow was recorded

    const uint64_t now = TimerMilli::GetNow().GetValue();

    observed_flow.mFlowStartTime = now;
    observed_flow.mFlowEndTime   = now;

    // Get the destination and source network of the flow

    if (GetSourceDestinationNetworks(observed_flow, aObservationPoint) != kErrorNone)
    {
        return;
    }

    // Get the hash value and the bucket ID of the flow

    const uint32_t hash_value = HashFunction(observed_flow);
    const uint16_t bucket_id  = static_cast<uint16_t>(hash_value % kNbrBuckets);

    // Find if the corresponding flow entry is already in the hash table or not

    IpfixFlowEntry *myentry = FindFlowEntryInHashtable(observed_flow, bucket_id, hash_value);

    // If the corresponding entry for the flow exists in the hashtable then update these metrics
    // Else create a new entry into the hashtable

    if (myentry != nullptr)
    {
        UpdateLayer3FlowEntry(*myentry, observed_flow);
        LogFlowEntry(*myentry, "existing entry updated", bucket_id);
        return;
    }
    else
    {
        CreateFlowEntry(observed_flow, bucket_id, hash_value);
        return;
    }

    return;
}

void IpfixFlowCapture::MeterLayer2FlowTraffic(const Mac::Addresses       &aMacAddrs,
                                              Message                    &aMessage,
                                              otIpfixFlowObservationPoint aObservationPoint)
{
    Ip6::Headers headers;

    if (aMessage.GetType() != Message::kTypeIp6)
    {
        LogInfo("Message type problem: Message is not a IPv6 packet");
        return;
    }
    if (headers.ParseFrom(aMessage) != kErrorNone)
    {
        return;
    }

    // Temporary Data structure to hold the Layer 2 flow informations

    IpfixFlowInfo observed_flow{};

    // Necessary IP information context to find the entry in the hashtable

    observed_flow.mSourceAddress      = headers.GetSourceAddress();
    observed_flow.mDestinationAddress = headers.GetDestinationAddress();
    observed_flow.mIpProto            = headers.GetIpProto();

    if (headers.IsUdp() || headers.IsTcp())
    {
        observed_flow.mSourcePort      = headers.GetSourcePort();
        observed_flow.mDestinationPort = headers.GetDestinationPort();
    }
    else if (headers.IsIcmp6())
    {
        observed_flow.mIcmp6Type = headers.GetIcmpHeader().GetType();
        observed_flow.mIcmp6Code = headers.GetIcmpHeader().GetCode();
    }

    // Observing the source Mac Address of the IEEE 802.15.4 frames (MAC extended address 64 bits or RLOC16)
    if (aMacAddrs.mSource.GetType() == Mac::Address::kTypeShort)
    {
        observed_flow.mThreadSrcRloc16Address = aMacAddrs.mSource.GetShort();
    }

    else if (aMacAddrs.mSource.GetType() == Mac::Address::kTypeExtended)
    {
        observed_flow.mThreadSrcMacAddress = aMacAddrs.mSource.GetExtended();
    }

    // Observing the destination Mac Address of the IEEE 802.15.4 frames (MAC extended address 64 bits or RLOC16)

    if (aMacAddrs.mDestination.GetType() == Mac::Address::kTypeShort)
    {
        observed_flow.mThreadDestRloc16Address = aMacAddrs.mDestination.GetShort();
    }
    else if (aMacAddrs.mDestination.GetType() == Mac::Address::kTypeExtended)
    {
        observed_flow.mThreadDestMacAddress = aMacAddrs.mDestination.GetExtended();
    }

    const uint64_t now           = TimerMilli::GetNow().GetValue();
    observed_flow.mFlowStartTime = now;
    observed_flow.mFlowEndTime   = now;

    observed_flow.mThreadFramesCount = 1;

    // Get the destination and source network of the flow

    if (GetSourceDestinationNetworks(observed_flow, aObservationPoint) != kErrorNone)
    {
        return;
    }

    // Get the hash value and the bucket ID of the flow

    const uint32_t hash_value = HashFunction(observed_flow);
    const uint16_t bucket_id  = static_cast<uint16_t>(hash_value % kNbrBuckets);

    // Find if the corresponding flow entry is already in the hash table or not

    IpfixFlowEntry *entry = FindFlowEntryInHashtable(observed_flow, bucket_id, hash_value);

    // If the corresponding entry for the flow exists in the hashtable then update the metrics
    // Else create a new entry into the hashtable

    if (entry != nullptr)
    {
        UpdateLayer2FlowEntry(*entry, observed_flow);
    }
    else
    {
        CreateFlowEntry(observed_flow, bucket_id, hash_value);
    }
}

uint16_t IpfixFlowCapture::GetFlowCount(void) { return mFlowCount; }

void IpfixFlowCapture::GetFlowTable(IpfixFlowInfo *aFlowBuffer)
{
    uint16_t flow_count = 0;
    uint16_t bucket     = 0;

    VerifyOrExit(aFlowBuffer != nullptr);

    while (bucket < kNbrBuckets)
    {
        IpfixFlowEntry *entry = mBuckets[bucket].GetHead();

        while (entry != nullptr)
        {
            aFlowBuffer[flow_count] = entry->mFlow;
            flow_count++;

            entry = entry->GetNext();
        }
        bucket++;
    }

exit:
    return;
}

void IpfixFlowCapture::ResetFlowTable(void)
{
    uint16_t mybucket = 0;

    while (mybucket < kNbrBuckets)
    {
        IpfixFlowEntry *myentry = mBuckets[mybucket].GetHead();

        while (myentry != nullptr)
        {
            IpfixFlowEntry *next = myentry->GetNext();
            mBuckets[mybucket].Remove(*myentry);
            mPool.Free(*myentry);

            if (mFlowCount > 0)
            {
                mFlowCount--;
            }
            myentry = next;
        }

        mybucket++;
    }

    OT_ASSERT(mFlowCount == 0);
    LogInfo("IPFIX Hashtable is reset");
}

uint32_t IpfixFlowCapture::HashFunction(const IpfixFlowInfo &aFlow)
{
    Crypto::Sha256 sha256;
    sha256.Start();

    sha256.Update(aFlow.mSourceAddress.mFields.m8, 16);
    sha256.Update(aFlow.mDestinationAddress.mFields.m8, 16);

    uint8_t source_port[2];
    BigEndian::WriteUint16(aFlow.mSourcePort, source_port);
    sha256.Update(source_port, 2);

    uint8_t destination_port[2];
    BigEndian::WriteUint16(aFlow.mDestinationPort, destination_port);
    sha256.Update(destination_port, 2);

    uint8_t ip_proto = aFlow.mIpProto;
    sha256.Update(&ip_proto, 1);

    Crypto::Sha256::Hash hash;
    sha256.Finish(hash);
    const uint8_t *h = hash.GetBytes();

    return (static_cast<uint32_t>(h[0]) << 24) | (static_cast<uint32_t>(h[1]) << 16) |
           (static_cast<uint32_t>(h[2]) << 8) | static_cast<uint32_t>(h[3]);
}

Error IpfixFlowCapture::GetSourceDestinationNetworks(IpfixFlowInfo              &aFlow,
                                                     otIpfixFlowObservationPoint aObservationPoint)
{
    if (aObservationPoint == OT_IPFIX_OBSERVATION_POINT_WPAN_TO_RCP)
    {
        const otIp6Address &src = aFlow.mSourceAddress;

        // Verifying if the source address is from the OTBR or else it is from the AIL

        bool isOtbrAddress = Get<ThreadNetif>().HasUnicastAddress(AsCoreType(&src));
        OT_UNUSED_VARIABLE(isOtbrAddress);

        if (isOtbrAddress)
        {
            aFlow.mSourceNetwork      = OT_IPFIX_INTERFACE_OTBR;
            aFlow.mDestinationNetwork = OT_IPFIX_INTERFACE_THREAD_NETWORK;
            return kErrorNone;
        }
        else
        {
            aFlow.mSourceNetwork      = OT_IPFIX_INTERFACE_AIL_NETWORK;
            aFlow.mDestinationNetwork = OT_IPFIX_INTERFACE_THREAD_NETWORK;
            return kErrorNone;
        }
    }

    else if (aObservationPoint == OT_IPFIX_OBSERVATION_POINT_RCP_TO_WPAN)
    {
        const otIp6Address &src = aFlow.mSourceAddress;

        bool isSrcOtbrAddress = Get<ThreadNetif>().HasUnicastAddress(AsCoreType(&src));

        if (isSrcOtbrAddress)
        {
            return kErrorAlready;
        }

        const otIp6Address &dst = aFlow.mDestinationAddress;

        // Verifying if the destination address is for the OTBR or else destination is for AIL

        bool isDstOtbrAddress = Get<ThreadNetif>().HasUnicastAddress(AsCoreType(&dst));
        bool isOtbrSubscribed = Get<ThreadNetif>().IsMulticastSubscribed(AsCoreType(&dst));

        if (isDstOtbrAddress || isOtbrSubscribed)
        {
            aFlow.mSourceNetwork      = OT_IPFIX_INTERFACE_THREAD_NETWORK;
            aFlow.mDestinationNetwork = OT_IPFIX_INTERFACE_OTBR;
            return kErrorNone;
        }
        else
        {
            aFlow.mSourceNetwork      = OT_IPFIX_INTERFACE_THREAD_NETWORK;
            aFlow.mDestinationNetwork = OT_IPFIX_INTERFACE_AIL_NETWORK;
            return kErrorNone;
        }
    }
    else if (aObservationPoint == OT_IPFIX_OBSERVATION_POINT_AIL_TO_OTBR)
    {
        aFlow.mSourceNetwork      = OT_IPFIX_INTERFACE_AIL_NETWORK;
        aFlow.mDestinationNetwork = OT_IPFIX_INTERFACE_OTBR;
        return kErrorNone;
    }
    else if (aObservationPoint == OT_IPFIX_OBSERVATION_POINT_OTBR_TO_AIL)
    {
        aFlow.mSourceNetwork      = OT_IPFIX_INTERFACE_OTBR;
        aFlow.mDestinationNetwork = OT_IPFIX_INTERFACE_AIL_NETWORK;
        return kErrorNone;
    }

    return kErrorNone;
}

bool IpfixFlowCapture::VerifyFlowEquality(const IpfixFlowInfo &aFirstFlow, const IpfixFlowInfo &aSecondFlow)
{
    if (aFirstFlow.mIpProto != aSecondFlow.mIpProto)
        return false;

    if (memcmp(&aFirstFlow.mSourceAddress, &aSecondFlow.mSourceAddress, sizeof(otIp6Address)) != 0)
        return false;

    if (memcmp(&aFirstFlow.mDestinationAddress, &aSecondFlow.mDestinationAddress, sizeof(otIp6Address)) != 0)
        return false;

    if (aFirstFlow.mIpProto == OT_IP6_PROTO_TCP)
    {
        return (aFirstFlow.mSourcePort == aSecondFlow.mSourcePort) &&
               (aFirstFlow.mDestinationPort == aSecondFlow.mDestinationPort);
    }

    if (aFirstFlow.mIpProto == OT_IP6_PROTO_ICMP6)
    {
        return ((aFirstFlow.mIcmp6Type == aSecondFlow.mIcmp6Type) && (aFirstFlow.mIcmp6Code == aSecondFlow.mIcmp6Code));
    }

    return true;
}

IpfixFlowCapture::IpfixFlowEntry *IpfixFlowCapture::FindFlowEntryInHashtable(const IpfixFlowInfo &aFlow,
                                                                             uint16_t             aBucketId,
                                                                             uint32_t             aHashValue)
{
    OT_ASSERT(aBucketId < kNbrBuckets);
    if (mBuckets[aBucketId].IsEmpty())
    {
        return nullptr;
    }

    IpfixFlowEntry *myentry = mBuckets[aBucketId].GetHead();

    while (myentry != nullptr)
    {
        if (myentry->mKeyHash == aHashValue && VerifyFlowEquality(myentry->mFlow, aFlow))
        {
            return myentry;
        }
        myentry = myentry->GetNext();
    }

    return nullptr;
}

void IpfixFlowCapture::UpdateLayer3FlowEntry(IpfixFlowEntry &aExistingFlowEntry, const IpfixFlowInfo &aObservedFlow)
{
    if (aExistingFlowEntry.mFlow.mSourceNetwork == OT_IPFIX_INTERFACE_THREAD_NETWORK &&
        aObservedFlow.mDestinationNetwork == OT_IPFIX_INTERFACE_THREAD_NETWORK)
    {
        aExistingFlowEntry.mFlow.mDestinationNetwork = OT_IPFIX_INTERFACE_THREAD_NETWORK;
    }
    aExistingFlowEntry.mFlow.mPacketsCount += aObservedFlow.mPacketsCount;
    aExistingFlowEntry.mFlow.mBytesCount += aObservedFlow.mBytesCount;

    if (aObservedFlow.mFlowEndTime > aExistingFlowEntry.mFlow.mFlowEndTime)
    {
        aExistingFlowEntry.mFlow.mFlowEndTime = aObservedFlow.mFlowEndTime;
    }
}

void IpfixFlowCapture::UpdateLayer2FlowEntry(IpfixFlowEntry &aExistingFlowEntry, const IpfixFlowInfo &aObservedFlow)
{
    if (aExistingFlowEntry.mFlow.mSourceNetwork == OT_IPFIX_INTERFACE_THREAD_NETWORK &&
        aObservedFlow.mDestinationNetwork == OT_IPFIX_INTERFACE_THREAD_NETWORK)
    {
        aExistingFlowEntry.mFlow.mDestinationNetwork = OT_IPFIX_INTERFACE_THREAD_NETWORK;

        otExtAddress null_addr = {};

        if (memcmp(&aObservedFlow.mThreadDestMacAddress, &null_addr, sizeof(otExtAddress)) != 0)
        {
            aExistingFlowEntry.mFlow.mThreadDestMacAddress = aObservedFlow.mThreadDestMacAddress;
        }

        if (aObservedFlow.mThreadDestRloc16Address != 0)
        {
            aExistingFlowEntry.mFlow.mThreadDestRloc16Address = aObservedFlow.mThreadDestRloc16Address;
        }
    }
    else
    {
        aExistingFlowEntry.mFlow.mThreadFramesCount += aObservedFlow.mThreadFramesCount;

        otExtAddress null_addr = {};
        if (memcmp(&aObservedFlow.mThreadSrcMacAddress, &null_addr, sizeof(otExtAddress)) != 0)
        {
            aExistingFlowEntry.mFlow.mThreadSrcMacAddress = aObservedFlow.mThreadSrcMacAddress;
        }

        if (aObservedFlow.mThreadSrcRloc16Address != 0)
        {
            aExistingFlowEntry.mFlow.mThreadSrcRloc16Address = aObservedFlow.mThreadSrcRloc16Address;
        }

        if (memcmp(&aObservedFlow.mThreadDestMacAddress, &null_addr, sizeof(otExtAddress)) != 0)
        {
            aExistingFlowEntry.mFlow.mThreadDestMacAddress = aObservedFlow.mThreadDestMacAddress;
        }

        if (aObservedFlow.mThreadDestRloc16Address != 0)
        {
            aExistingFlowEntry.mFlow.mThreadDestRloc16Address = aObservedFlow.mThreadDestRloc16Address;
        }
    }

    return;
}

void IpfixFlowCapture::CreateFlowEntry(const IpfixFlowInfo &aObservedFlow, uint16_t aBucketId, uint32_t aHashValue)
{
    OT_ASSERT(aBucketId < kNbrBuckets);
    IpfixFlowEntry *myentry = mPool.Allocate();
    if (myentry == nullptr)
    {
        LogInfo("IPFIX pool full (max number of flows = %u => actual number of flows count = %u)",
                static_cast<unsigned>(kMaxFlows), static_cast<unsigned>(mFlowCount));
        return;
    }

    myentry->mFlow    = aObservedFlow;
    myentry->mKeyHash = aHashValue;
    myentry->SetNext(nullptr);
    mBuckets[aBucketId].PushAfterTail(*myentry);

    mFlowCount++;
    LogFlowEntry(*myentry, "new entry created", aBucketId);

    return;
}

void IpfixFlowCapture::LogFlowEntry(const IpfixFlowEntry &aEntry, const char *aAdditionalInfo, uint16_t aBucketId)
{
    const IpfixFlowEntry &myentree = aEntry;
    const IpfixFlowInfo  &myflow   = myentree.mFlow;

    char src_address[OT_IP6_ADDRESS_STRING_SIZE];
    char dst_address[OT_IP6_ADDRESS_STRING_SIZE];
    otIp6AddressToString(&myflow.mSourceAddress, src_address, sizeof(src_address));
    otIp6AddressToString(&myflow.mDestinationAddress, dst_address, sizeof(dst_address));

    LogInfo("< =============================================================== >");

    LogInfo("Hashtable %s : bucket = %u/%u hash value = 0x%08x", (aAdditionalInfo ? aAdditionalInfo : ""),
            static_cast<unsigned>(aBucketId), static_cast<unsigned>(kNbrBuckets),
            static_cast<unsigned>(myentree.mKeyHash));

    LogInfo("Src Addr %s -> Dst Addr %s  protoID = %u(%s)", src_address, dst_address,
            static_cast<unsigned>(myflow.mIpProto), GetProtoIdName(myflow.mIpProto));

    if (myflow.mIpProto == OT_IP6_PROTO_TCP || myflow.mIpProto == OT_IP6_PROTO_UDP)
    {
        LogInfo("Src Port %u -> Dst Port %u", static_cast<unsigned>(myflow.mSourcePort),
                static_cast<unsigned>(myflow.mDestinationPort));
    }
    else if (myflow.mIpProto == OT_IP6_PROTO_ICMP6)
    {
        LogInfo("ICMPv6 Type = %u", static_cast<unsigned>(myflow.mIcmp6Type));
    }

    LogInfo("Packets = %" PRIu32 " BytesCount = %" PRIu32, static_cast<unsigned>(myflow.mPacketsCount),
            static_cast<unsigned>(myflow.mBytesCount));

    LogInfo("FlowStartTime = %" PRIu64 " ms & FlowEndTime = %" PRIu64 " ms", myflow.mFlowStartTime,
            myflow.mFlowEndTime);

    LogInfo("Src Network = %s  & Dst Network = %s", GetNetworkName(myflow.mSourceNetwork),
            GetNetworkName(myflow.mDestinationNetwork));

    LogInfo("< =============================================================== >");
}

const char *IpfixFlowCapture::GetProtoIdName(uint8_t aProtoId)
{
    switch (aProtoId)
    {
    case OT_IP6_PROTO_HOP_OPTS:
        return "IPv6 Hop-by-Hop Option";

    case OT_IP6_PROTO_TCP:
        return "TCP (Transmission Control Protocol)";

    case OT_IP6_PROTO_UDP:
        return "UDP (User Datagram Protocol)";

    case OT_IP6_PROTO_IP6:
        return "IPv6 Encapsulation";

    case OT_IP6_PROTO_ROUTING:
        return "Routing Header for IPv6";

    case OT_IP6_PROTO_FRAGMENT:
        return "Fragment Header for IPv6";

    case OT_IP6_PROTO_ICMP6:
        return "ICMPv6 (ICMP for IPv6)";

    case OT_IP6_PROTO_NONE:
        return "No Next Header";

    case OT_IP6_PROTO_DST_OPTS:
        return "Destination Options for IPv6";

    default:
        return "Unknown Protocol";
    }
}

const char *IpfixFlowCapture::GetNetworkName(otIpfixFlowInterface aNetworkInterface)
{
    switch (aNetworkInterface)
    {
    case OT_IPFIX_INTERFACE_THREAD_NETWORK:
        return "Thread Network";

    case OT_IPFIX_INTERFACE_AIL_NETWORK:
        return "AIL (Adjacent Infrastructure (AIL))";

    case OT_IPFIX_INTERFACE_OTBR:
        return "OTBR";

    default:
        return "UNKNOWN";
    }
}

extern "C" void otIpfixMeterLayer3InfraFlowTraffic(otInstance                 *aInstance,
                                                   const otIp6Address         *aSrcAddress,
                                                   const otIp6Address         *aDstAddress,
                                                   const uint8_t              *aBuffer,
                                                   uint16_t                    aBufferLength,
                                                   otIpfixFlowObservationPoint aLocation)
{
    using namespace ot;
    Instance    &instance = AsCoreType(aInstance);
    Message     *msg      = nullptr;
    Ip6::Header  ip6Header;
    Ip6::Address src = AsCoreType(aSrcAddress);
    Ip6::Address dst = AsCoreType(aDstAddress);

    msg = instance.Get<MessagePool>().Allocate(Message::kTypeIp6, 0);
    VerifyOrExit(msg != nullptr);

    ip6Header.InitVersionTrafficClassFlow();
    ip6Header.SetSource(src);
    ip6Header.SetDestination(dst);
    ip6Header.SetNextHeader(Ip6::kProtoIcmp6);
    ip6Header.SetPayloadLength(aBufferLength);
    ip6Header.SetHopLimit(255);

    SuccessOrExit(msg->Append(ip6Header));
    SuccessOrExit(msg->AppendBytes(aBuffer, aBufferLength));

    instance.Get<Ipfix::IpfixFlowCapture>().MeterLayer3FlowTraffic(*msg, aLocation);

exit:
    if (msg != nullptr)
    {
        msg->Free();
    }
}

} // namespace Ipfix
} // namespace ot

#endif // OPENTHREAD_CONFIG_IPFIX_ENABLE
