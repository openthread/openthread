/*
 *  Copyright (c) 2022, The OpenThread Authors.
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
 *   This file includes implementation for the NAT64 translator.
 */

#include "nat64_translator.hpp"

#if OPENTHREAD_CONFIG_NAT64_TRANSLATOR_ENABLE

#include "instance/instance.hpp"

namespace ot {
namespace Nat64 {

RegisterLogModule("Nat64");

const char *StateToString(State aState)
{
    static const char *const kStateString[] = {
        "Disabled",
        "NotRunning",
        "Idle",
        "Active",
    };

    struct EnumCheck
    {
        InitEnumValidatorCounter();
        ValidateNextEnum(kStateDisabled);
        ValidateNextEnum(kStateNotRunning);
        ValidateNextEnum(kStateIdle);
        ValidateNextEnum(kStateActive);
    };

    return kStateString[aState];
}

Translator::Translator(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mState(State::kStateDisabled)
    , mTimer(aInstance)
{
    Random::NonCrypto::Fill(mNextMappingId);

    mNat64Prefix.Clear();
    mIp4Cidr.Clear();
    mTimer.Start(kIdleTimeout);

    mCounters.Clear();
    ClearAllBytes(mErrorCounters);
}

Message *Translator::NewIp4Message(const Message::Settings &aSettings)
{
    Message *message = Get<Ip6::Ip6>().NewMessage(sizeof(Ip6::Header) - sizeof(Ip4::Header), aSettings);

    if (message != nullptr)
    {
        message->SetType(Message::kTypeIp4);
    }

    return message;
}

Error Translator::SendMessage(Message &aMessage)
{
    bool   freed  = false;
    Error  error  = kErrorDrop;
    Result result = TranslateToIp6(aMessage);

    VerifyOrExit(result == kForward);

    error = Get<Ip6::Ip6>().SendRaw(OwnedPtr<Message>(&aMessage).PassOwnership());
    freed = true;

exit:
    if (!freed)
    {
        aMessage.Free();
    }

    return error;
}

uint16_t Translator::GetSourcePortOrIcmp6Id(const Ip6::Headers &aIp6Headers)
{
    return aIp6Headers.IsIcmp6() ? aIp6Headers.GetIcmpHeader().GetId() : aIp6Headers.GetSourcePort();
}

uint16_t Translator::GetDestinationPortOrIcmp4Id(const Ip4::Headers &aIp4Headers)
{
    return aIp4Headers.IsIcmp4() ? aIp4Headers.GetIcmpHeader().GetId() : aIp4Headers.GetDestinationPort();
}

Translator::Result Translator::TranslateFromIp6(Message &aMessage)
{
    Result       result     = kDrop;
    DropReason   dropReason = kReasonUnknown;
    Ip6::Headers ip6Headers;
    Ip4::Header  ip4Header;
    uint16_t     srcPortOrId = 0;
    Mapping     *mapping     = nullptr;

    if (mIp4Cidr.mLength == 0 || !mNat64Prefix.IsValidNat64())
    {
        ExitNow(result = kNotTranslated);
    }

    // `ParseFrom()` will do basic checks for the message, including
    // the message length and IP protocol version.
    if (ip6Headers.ParseFrom(aMessage) != kErrorNone)
    {
        LogWarn("Outgoing datagram is not a valid IPv6 datagram, drop");
        dropReason = kReasonIllegalPacket;
        ExitNow(result = kDrop);
    }

    if (!ip6Headers.GetDestinationAddress().MatchesPrefix(mNat64Prefix))
    {
        ExitNow(result = kNotTranslated);
    }

    mapping = mActiveMappings.FindMatching(ip6Headers);

    if (mapping == nullptr)
    {
        mapping = AllocateMapping(ip6Headers);
    }

    if (mapping == nullptr)
    {
        LogWarn("Failed to get a mapping for %s (mapping pool full?)",
                ip6Headers.GetSourceAddress().ToString().AsCString());
        dropReason = kReasonNoMapping;
        ExitNow(result = kDrop);
    }

#if OPENTHREAD_CONFIG_NAT64_PORT_TRANSLATION_ENABLE
    srcPortOrId = mapping->mTranslatedPortOrId;
#else
    srcPortOrId = GetSourcePortOrIcmp6Id(ip6Headers);
#endif

    aMessage.RemoveHeader(sizeof(Ip6::Header));

    ip4Header.Clear();
    ip4Header.InitVersionIhl();
    ip4Header.SetSource(mapping->mIp4Address);
    ip4Header.GetDestination().ExtractFromIp6Address(mNat64Prefix.mLength, ip6Headers.GetDestinationAddress());
    ip4Header.SetTtl(ip6Headers.GetIpHopLimit());
    ip4Header.SetIdentification(0);

    switch (ip6Headers.GetIpProto())
    {
    // The IP header is consumed, so the next header is at offset 0.
    case Ip6::kProtoUdp:
        ip4Header.SetProtocol(Ip4::kProtoUdp);
        ip6Headers.SetSourcePort(srcPortOrId);
        aMessage.Write(0, ip6Headers.GetUdpHeader());
        result = kForward;
        break;
    case Ip6::kProtoTcp:
        ip4Header.SetProtocol(Ip4::kProtoTcp);
        ip6Headers.SetSourcePort(srcPortOrId);
        aMessage.Write(0, ip6Headers.GetTcpHeader());
        result = kForward;
        break;
    case Ip6::kProtoIcmp6:
        ip4Header.SetProtocol(Ip4::kProtoIcmp);
        SuccessOrExit(TranslateIcmp6(aMessage, srcPortOrId));
        result = kForward;
        break;
    default:
        dropReason = kReasonUnsupportedProto;
        ExitNow(result = kDrop);
    }

    // `result` here must be kForward based on the switch above.
    // TODO: Implement the logic for replying ICMP messages.
    ip4Header.SetTotalLength(sizeof(Ip4::Header) + aMessage.GetLength() - aMessage.GetOffset());

    Checksum::UpdateMessageChecksum(aMessage, ip4Header.GetSource(), ip4Header.GetDestination(),
                                    ip4Header.GetProtocol());
    Checksum::UpdateIp4HeaderChecksum(ip4Header);

    if (aMessage.Prepend(ip4Header) != kErrorNone)
    {
        // This should never happen since the IPv4 header is shorter
        // than the IPv6 header.
        LogCrit("failed to prepend IPv4 head to translated message");
        ExitNow(result = kDrop);
    }

    aMessage.SetType(Message::kTypeIp4);

    mCounters.Count6To4Packet(ip6Headers);
    mapping->mCounters.Count6To4Packet(ip6Headers);

exit:
    if (result == kDrop)
    {
        mErrorCounters.mCount6To4[dropReason]++;
    }

    return result;
}

Translator::Result Translator::TranslateToIp6(Message &aMessage)
{
    Result       result     = kDrop;
    DropReason   dropReason = kReasonUnknown;
    Ip6::Header  ip6Header;
    Ip4::Headers ip4Headers;
    uint16_t     dstPortOrId = 0;
    Mapping     *mapping     = nullptr;

    // `ParseFrom()` may return an error value when the incoming
    // message is an IPv4 datagram. If the message is already an IPv6
    // datagram, forward it directly.
    VerifyOrExit(ip6Header.ParseFrom(aMessage) != kErrorNone, result = kNotTranslated);

    if (mIp4Cidr.mLength == 0)
    {
        LogWarn("Incoming message is an IPv4 datagram but no IPv4 CIDR for NAT64 configured, drop");
        ExitNow(result = kForward);
    }

    if (!mNat64Prefix.IsValidNat64())
    {
        LogWarn("Incoming message is an IPv4 datagram but no NAT64 prefix configured, drop");
        ExitNow(result = kDrop);
    }

    if (ip4Headers.ParseFrom(aMessage) != kErrorNone)
    {
        LogWarn("Incoming message is neither IPv4 nor an IPv6 datagram, drop");
        dropReason = kReasonIllegalPacket;
        ExitNow(result = kDrop);
    }

    mapping = mActiveMappings.FindMatching(ip4Headers);

    if (mapping == nullptr)
    {
        LogWarn("No mapping found for the IPv4 address");
        dropReason = kReasonNoMapping;
        ExitNow(result = kDrop);
    }

    mapping->Touch(ip4Headers.GetIpProto());

#if OPENTHREAD_CONFIG_NAT64_PORT_TRANSLATION_ENABLE
    dstPortOrId = mapping->mSrcPortOrId;
#else
    dstPortOrId = GetDestinationPortOrIcmp4Id(ip4Headers);
#endif

    aMessage.RemoveHeader(sizeof(Ip4::Header));

    ip6Header.Clear();
    ip6Header.InitVersionTrafficClassFlow();
    ip6Header.GetSource().SynthesizeFromIp4Address(mNat64Prefix, ip4Headers.GetSourceAddress());
    ip6Header.SetDestination(mapping->mIp6Address);
    ip6Header.SetFlow(0);
    ip6Header.SetHopLimit(ip4Headers.GetIpTtl());

    // Note: TCP and UDP are the same for both IPv4 and IPv6 except
    // for the checksum calculation, we will update the checksum in
    // the payload later. However, we need to translate ICMPv6
    // messages to ICMP messages in IPv4.
    switch (ip4Headers.GetIpProto())
    {
    // The IP header is consumed , so the next header is at offset 0.
    case Ip4::kProtoUdp:
        ip6Header.SetNextHeader(Ip6::kProtoUdp);
        ip4Headers.SetDestinationPort(dstPortOrId);
        aMessage.Write(0, ip4Headers.GetUdpHeader());
        result = kForward;
        break;
    case Ip4::kProtoTcp:
        ip6Header.SetNextHeader(Ip6::kProtoTcp);
        ip4Headers.SetDestinationPort(dstPortOrId);
        aMessage.Write(0, ip4Headers.GetTcpHeader());
        result = kForward;
        break;
    case Ip4::kProtoIcmp:
        ip6Header.SetNextHeader(Ip6::kProtoIcmp6);
        SuccessOrExit(TranslateIcmp4(aMessage, dstPortOrId));
        result = kForward;
        break;
    default:
        dropReason = kReasonUnsupportedProto;
        ExitNow(result = kDrop);
    }

    // result here must be kForward based on the switch above.
    // TODO: Implement the logic for replying ICMP datagrams.
    ip6Header.SetPayloadLength(aMessage.GetLength() - aMessage.GetOffset());

    Checksum::UpdateMessageChecksum(aMessage, ip6Header.GetSource(), ip6Header.GetDestination(),
                                    ip6Header.GetNextHeader());

    if (aMessage.Prepend(ip6Header) != kErrorNone)
    {
        // This might happen when the platform failed to reserve
        // enough space before the original IPv4 datagram.
        LogWarn("Failed to prepend IPv6 head to translated message");
        ExitNow(result = kDrop);
    }

    aMessage.SetType(Message::kTypeIp6);

    mCounters.Count4To6Packet(ip4Headers);
    mapping->mCounters.Count4To6Packet(ip4Headers);

exit:
    if (result == kDrop)
    {
        mErrorCounters.mCount4To6[dropReason]++;
    }

    return result;
}

Translator::Mapping::InfoString Translator::Mapping::ToString(void) const
{
    InfoString string;

    string.Append("%s -> %s", mIp6Address.ToString().AsCString(), mIp4Address.ToString().AsCString());

    return string;
}

void Translator::Mapping::CopyTo(AddressMapping &aMapping, TimeMilli aNow) const
{
    aMapping.mId                 = mId;
    aMapping.mIp4                = mIp4Address;
    aMapping.mIp6                = mIp6Address;
    aMapping.mSrcPortOrId        = mSrcPortOrId;
    aMapping.mTranslatedPortOrId = mTranslatedPortOrId;
    aMapping.mCounters           = mCounters;

    // We are removing expired mappings lazily, and an expired mapping
    // might become active again before actually removed. Report the
    // mapping to be "just expired" to avoid confusion.

    aMapping.mRemainingTimeMs = (mExpiry < aNow) ? 0 : mExpiry - aNow;
}

void Translator::ReleaseMapping(Mapping &aMapping)
{
    if (mIp4Cidr.mLength <= kMaxCidrLenForValidAddrPool)
    {
        // IPv4 addresses are allocated from the pool only when the
        // pool size is above a minimum value. Otherwise use just the
        // first address from the list and we are not removing it
        // from the array.
        IgnoreError(mIp4AddressPool.PushBack(aMapping.mIp4Address));
    }

    mMappingPool.Free(aMapping);

    LogInfo("Mapping removed: %s", aMapping.ToString().AsCString());
}

uint16_t Translator::ReleaseMappings(LinkedList<Mapping> &aMappings)
{
    uint16_t numRemoved = 0;

    for (Mapping *mapping = aMappings.Pop(); mapping != nullptr; mapping = aMappings.Pop())
    {
        numRemoved++;
        ReleaseMapping(*mapping);
    }

    return numRemoved;
}

uint16_t Translator::ReleaseExpiredMappings(void)
{
    LinkedList<Mapping> idleMappings;

    mActiveMappings.RemoveAllMatching(idleMappings, TimerMilli::GetNow());

    return ReleaseMappings(idleMappings);
}
#if OPENTHREAD_CONFIG_NAT64_PORT_TRANSLATION_ENABLE
uint16_t Translator::AllocateSourcePort(uint16_t aSrcPort)
{
    // The translated port is randomly allocated from the range of
    // dynamic or private ports (RFC 7605 section 4). In this way, we
    // will not pick a random port that could be a well-known port
    // preventing an unknown situation on the receiver side.

    uint16_t port;

    do
    {
        port = Random::NonCrypto::GetUint16InRange(kMinTranslationPort, kMaxTranslationPort);

        // The NAT64 SHOULD preserve the port parity (odd/even), as
        // per Section 4.2.2 of [RFC4787]). Determine if original and
        // allocated port have different parity

        if (((aSrcPort ^ port) & 1) == 1)
        {
            port++;
        }

    } while (mActiveMappings.ContainsMatching(port));

    return port;
}
#endif

Translator::Mapping *Translator::AllocateMapping(const Ip6::Headers &aIp6Headers)
{
    Mapping     *mapping = nullptr;
    Ip4::Address ip4Addr;

    // The NAT64 translator can work in 2 ways, either with a single
    // IPv4 address or a larger pool of addresses. There is also the
    // corner case where the address pool is generated from a big
    // CIDR length and the number of available IPv4 addresses is not
    // big enough to apply a 1 to 1 translation from IPv6 to IPv4
    // address. When operating in the first case, there is no need to
    // manage the address pool and all active mappings will use 1
    // single address (or the limited number alternatively). If a
    // larger pool is available each active mapping will use a
    // separate IPv4 address.

    if (mIp4Cidr.mLength > kMaxCidrLenForValidAddrPool)
    {
        // TODO: add logic to cycle between available IPv4 addresses
        ip4Addr = *mIp4AddressPool.At(0);
    }
    else
    {
        if (mIp4AddressPool.IsEmpty())
        {
            // `ReleaseExpiredMappings()` returns the number of
            // mappings removed.

            VerifyOrExit(ReleaseExpiredMappings() > 0);
        }
        ip4Addr = *mIp4AddressPool.PopBack();
    }

    mapping = mMappingPool.Allocate();

    // We should get a valid item, there is enough space in the
    // mapping pool. Otherwise return null and fail the translation.
    VerifyOrExit(mapping != nullptr);

    mActiveMappings.Push(*mapping);
    mapping->mCounters.Clear();
    mapping->mId         = ++mNextMappingId;
    mapping->mIp6Address = aIp6Headers.GetSourceAddress();
    mapping->mIp4Address = ip4Addr;
#if OPENTHREAD_CONFIG_NAT64_PORT_TRANSLATION_ENABLE
    mapping->mSrcPortOrId        = GetSourcePortOrIcmp6Id(aIp6Headers);
    mapping->mTranslatedPortOrId = AllocateSourcePort(mapping->mSrcPortOrId);
#else
    mapping->mSrcPortOrId        = 0;
    mapping->mTranslatedPortOrId = 0;
#endif
    mapping->Touch(aIp6Headers.GetIpProto());

    LogInfo("Mapping created: %s", mapping->ToString().AsCString());

exit:
    return mapping;
}

void Translator::Mapping::Touch(uint8_t aProtocol)
{
    uint32_t timeout;

    if ((aProtocol == Ip6::kProtoIcmp6) || (aProtocol == Ip4::kProtoIcmp))
    {
        timeout = kIcmpTimeout;
    }
    else
    {
        timeout = kIdleTimeout;
    }

    mExpiry = TimerMilli::GetNow() + timeout;
}

bool Translator::Mapping::Matches(const Ip6::Headers &aIp6Headers) const
{
    bool matches = false;

#if OPENTHREAD_CONFIG_NAT64_PORT_TRANSLATION_ENABLE
    VerifyOrExit(mSrcPortOrId == GetSourcePortOrIcmp6Id(aIp6Headers));
#endif
    VerifyOrExit(mIp6Address == aIp6Headers.GetSourceAddress());

    matches = true;

exit:
    return matches;
}

bool Translator::Mapping::Matches(const Ip4::Headers &aIp4Headers) const
{
    bool matches = false;

#if OPENTHREAD_CONFIG_NAT64_PORT_TRANSLATION_ENABLE
    VerifyOrExit(mTranslatedPortOrId == GetDestinationPortOrIcmp4Id(aIp4Headers));
#endif
    VerifyOrExit(mIp4Address == aIp4Headers.GetDestinationAddress());

    matches = true;

exit:
    return matches;
}

Error Translator::TranslateIcmp4(Message &aMessage, uint16_t aOriginalId)
{
    Error             error = kErrorNone;
    Ip4::Icmp::Header icmp4Header;
    Ip6::Icmp::Header icmp6Header;

    // TODO: Implement the translation of other ICMP messages.

    // Note: The caller consumed the IP header, so the ICMP header is
    // at offset 0.
    SuccessOrExit(error = aMessage.Read(0, icmp4Header));

    switch (icmp4Header.GetType())
    {
    case Ip4::Icmp::Header::Type::kTypeEchoReply:
        // The only difference between ICMPv6 echo and ICMP4 echo is
        // the message type field, so we can reinterpret it as ICMP6
        // header and set the message type.
        SuccessOrExit(error = aMessage.Read(0, icmp6Header));
        icmp6Header.SetType(Ip6::Icmp::Header::kTypeEchoReply);
        icmp6Header.SetId(aOriginalId);
        aMessage.Write(0, icmp6Header);
        break;

    default:
        error = kErrorInvalidArgs;
        break;
    }

exit:
    return error;
}

Error Translator::TranslateIcmp6(Message &aMessage, uint16_t aTranslatedId)
{
    Error             error = kErrorNone;
    Ip4::Icmp::Header icmp4Header;
    Ip6::Icmp::Header icmp6Header;

    // TODO: Implement the translation of other ICMP messages.

    // Note: The caller have consumed the IP header, so the ICMP
    // header is at offset 0.
    SuccessOrExit(error = aMessage.Read(0, icmp6Header));

    switch (icmp6Header.GetType())
    {
    case Ip6::Icmp::Header::kTypeEchoRequest:
        // The only difference between ICMPv6 echo and ICMP4 echo is
        // the message type field, so we can reinterpret it as ICMP6
        // header and set the message type.
        SuccessOrExit(error = aMessage.Read(0, icmp4Header));
        icmp4Header.SetType(Ip4::Icmp::Header::Type::kTypeEchoRequest);
        icmp4Header.SetId(aTranslatedId);
        aMessage.Write(0, icmp4Header);
        break;

    default:
        error = kErrorInvalidArgs;
        break;
    }

exit:
    return error;
}

Error Translator::SetIp4Cidr(const Ip4::Cidr &aCidr)
{
    Error error = kErrorNone;

    uint32_t numberOfHosts;
    uint32_t hostIdBegin;

    VerifyOrExit(aCidr.mLength > 0 && aCidr.mLength <= 32, error = kErrorInvalidArgs);

    VerifyOrExit(mIp4Cidr != aCidr);

    // Avoid using the 0s and 1s in the host id of an address, but
    // what if the user provides us with /32 or /31 addresses?

    if (aCidr.mLength == 32)
    {
        hostIdBegin   = 0;
        numberOfHosts = 1;
    }
    else if (aCidr.mLength == 31)
    {
        hostIdBegin   = 0;
        numberOfHosts = 2;
    }
    else
    {
        hostIdBegin   = 1;
        numberOfHosts = static_cast<uint32_t>((1 << (Ip4::Address::kSize * 8 - aCidr.mLength)) - 2);
    }

    numberOfHosts = OT_MIN(numberOfHosts, kPoolSize);

    mMappingPool.FreeAll();
    mActiveMappings.Clear();
    mIp4AddressPool.Clear();

    for (uint32_t i = 0; i < numberOfHosts; i++)
    {
        Ip4::Address addr;

        addr.SynthesizeFromCidrAndHost(aCidr, i + hostIdBegin);
        IgnoreError(mIp4AddressPool.PushBack(addr));
    }

    LogInfo("IPv4 CIDR for NAT64: %s (actual address pool: %s - %s, %lu addresses)", aCidr.ToString().AsCString(),
            mIp4AddressPool.Front()->ToString().AsCString(), mIp4AddressPool.Back()->ToString().AsCString(),
            ToUlong(numberOfHosts));

    mIp4Cidr = aCidr;

    UpdateState();

    Get<Notifier>().Signal(kEventNat64TranslatorStateChanged);

exit:
    return error;
}

void Translator::ClearIp4Cidr(void)
{
    mIp4Cidr.Clear();
    mMappingPool.FreeAll();
    mActiveMappings.Clear();
    mIp4AddressPool.Clear();

    UpdateState();
}

void Translator::SetNat64Prefix(const Ip6::Prefix &aNat64Prefix)
{
    if (aNat64Prefix.GetLength() == 0)
    {
        ClearNat64Prefix();
    }
    else if (mNat64Prefix != aNat64Prefix)
    {
        LogInfo("IPv6 Prefix for NAT64 updated to %s", aNat64Prefix.ToString().AsCString());
        mNat64Prefix = aNat64Prefix;
        UpdateState();
    }
}

void Translator::ClearNat64Prefix(void)
{
    VerifyOrExit(mNat64Prefix.GetLength() != 0);
    mNat64Prefix.Clear();
    LogInfo("IPv6 Prefix for NAT64 cleared");
    UpdateState();

exit:
    return;
}

void Translator::HandleTimer(void)
{
    uint16_t numReleased = ReleaseExpiredMappings();

    LogInfo("Released %u expired mappings", numReleased);

    mTimer.Start(Min(kIcmpTimeout, kIdleTimeout));

    OT_UNUSED_VARIABLE(numReleased);
}

void Translator::InitAddressMappingIterator(AddressMappingIterator &aIterator)
{
    aIterator.mPtr = mActiveMappings.GetHead();
}

Error Translator::GetNextAddressMapping(AddressMappingIterator &aIterator, AddressMapping &aMapping)
{
    Error    error   = kErrorNotFound;
    Mapping *mapping = static_cast<Mapping *>(aIterator.mPtr);

    VerifyOrExit(mapping != nullptr);

    mapping->CopyTo(aMapping, TimerMilli::GetNow());
    aIterator.mPtr = mapping->GetNext();
    error          = kErrorNone;

exit:
    return error;
}

Error Translator::GetIp4Cidr(Ip4::Cidr &aCidr) const
{
    Error error = kErrorNone;

    VerifyOrExit(mIp4Cidr.mLength > 0, error = kErrorNotFound);
    aCidr = mIp4Cidr;

exit:
    return error;
}

Error Translator::GetIp6Prefix(Ip6::Prefix &aPrefix) const
{
    Error error = kErrorNone;

    VerifyOrExit(mNat64Prefix.mLength > 0, error = kErrorNotFound);
    aPrefix = mNat64Prefix;

exit:
    return error;
}

void Translator::ProtocolCounters::Count6To4Packet(const Ip6::Headers &aIp6Headers)
{
    uint16_t size = aIp6Headers.GetIpLength();

    switch (aIp6Headers.GetIpProto())
    {
    case Ip6::kProtoUdp:
        Update6To4(mUdp, size);
        break;
    case Ip6::kProtoTcp:
        Update6To4(mTcp, size);
        break;
    case Ip6::kProtoIcmp6:
        Update6To4(mIcmp, size);
        break;
    }

    Update6To4(mTotal, size);
}

void Translator::ProtocolCounters::Update6To4(Counters &aCounters, uint16_t aSize)
{
    aCounters.m6To4Packets++;
    aCounters.m6To4Bytes += aSize;
}

void Translator::ProtocolCounters::Count4To6Packet(const Ip4::Headers &aIp4Headers)
{
    uint16_t size = aIp4Headers.GetIpLength() - sizeof(Ip4::Header);

    switch (aIp4Headers.GetIpProto())
    {
    case Ip4::kProtoUdp:
        Update4To6(mUdp, size);
        break;
    case Ip4::kProtoTcp:
        Update4To6(mTcp, size);
        break;
    case Ip4::kProtoIcmp:
        Update4To6(mIcmp, size);
        break;
    }

    Update4To6(mTotal, size);
}

void Translator::ProtocolCounters::Update4To6(Counters &aCounters, uint16_t aSize)
{
    aCounters.m4To6Packets++;
    aCounters.m4To6Bytes += aSize;
}

void Translator::UpdateState(void)
{
    State newState;

    if (mEnabled)
    {
        if (mIp4Cidr.mLength > 0 && mNat64Prefix.IsValidNat64())
        {
            newState = kStateActive;
        }
        else
        {
            newState = kStateNotRunning;
        }
    }
    else
    {
        newState = kStateDisabled;
    }

    SuccessOrExit(Get<Notifier>().Update(mState, newState, kEventNat64TranslatorStateChanged));
    LogInfo("NAT64 translator is now %s", StateToString(mState));

exit:
    return;
}

void Translator::SetEnabled(bool aEnabled)
{
    VerifyOrExit(mEnabled != aEnabled);
    mEnabled = aEnabled;

    if (!aEnabled)
    {
        ReleaseMappings(mActiveMappings);
    }

    UpdateState();

exit:
    return;
}

} // namespace Nat64
} // namespace ot

#endif // OPENTHREAD_CONFIG_NAT64_TRANSLATOR_ENABLE
