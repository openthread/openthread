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
    , mState(kStateDisabled)
    , mMappingPool(aInstance)
    , mMinHostId(0)
    , mMaxHostId(0)
    , mNextHostId(0)
    , mTimer(aInstance)
{
    Random::NonCrypto::Fill(mNextMappingId);

    mNat64Prefix.Clear();
    mIp4Cidr.Clear();
    mTimer.Start(Min(kIdleTimeout, kIcmpTimeout));

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

Error Translator::SendMessage(OwnedPtr<Message> aMessagePtr)
{
    Error error = kErrorDrop;

    SuccessOrExit(TranslateIp4ToIp6(*aMessagePtr));
    error = Get<Ip6::Ip6>().SendRaw(aMessagePtr.PassOwnership());

exit:
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

Error Translator::TranslateIp6ToIp4(Message &aMessage)
{
    Error        error      = kErrorNone;
    DropReason   dropReason = kReasonUnknown;
    Ip6::Headers ip6Headers;
    Ip4::Header  ip4Header;
    uint16_t     srcPortOrId = 0;
    Mapping     *mapping     = nullptr;

    VerifyOrExit(mState == kStateActive, error = kErrorAbort);

    // `ParseFrom()` will do basic checks for the message, including
    // the message length and IP protocol version.
    if (ip6Headers.ParseFrom(aMessage) != kErrorNone)
    {
        LogWarn("Outgoing datagram is not a valid IPv6 datagram, drop");
        dropReason = kReasonIllegalPacket;
        ExitNow(error = kErrorDrop);
    }

    if (!ip6Headers.GetDestinationAddress().MatchesPrefix(mNat64Prefix))
    {
        ExitNow(error = kErrorAbort);
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
        ExitNow(error = kErrorDrop);
    }

    mapping->Touch(ip6Headers.GetIpProto());

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
        break;
    case Ip6::kProtoTcp:
        ip4Header.SetProtocol(Ip4::kProtoTcp);
        ip6Headers.SetSourcePort(srcPortOrId);
        aMessage.Write(0, ip6Headers.GetTcpHeader());
        break;
    case Ip6::kProtoIcmp6:
        ip4Header.SetProtocol(Ip4::kProtoIcmp);
        SuccessOrExit(TranslateIcmp6(aMessage, srcPortOrId));
        break;
    default:
        dropReason = kReasonUnsupportedProto;
        ExitNow(error = kErrorDrop);
    }

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
        ExitNow(error = kErrorDrop);
    }

    aMessage.SetType(Message::kTypeIp4);

    mCounters.Count6To4Packet(ip6Headers);
    mapping->mCounters.Count6To4Packet(ip6Headers);

exit:
    if (error == kErrorDrop)
    {
        mErrorCounters.mCount6To4[dropReason]++;
    }

    return error;
}

Error Translator::TranslateIp4ToIp6(Message &aMessage)
{
    Error        error      = kErrorNone;
    DropReason   dropReason = kReasonUnknown;
    Ip6::Header  ip6Header;
    Ip4::Headers ip4Headers;
    uint16_t     dstPortOrId = 0;
    Mapping     *mapping     = nullptr;

    VerifyOrExit(mState == kStateActive, error = kErrorDrop);

    if (ip4Headers.ParseFrom(aMessage) != kErrorNone)
    {
        dropReason = kReasonIllegalPacket;
        ExitNow(error = kErrorDrop);
    }

    mapping = mActiveMappings.FindMatching(ip4Headers);

    if (mapping == nullptr)
    {
        LogWarn("No mapping found for the IPv4 address");
        dropReason = kReasonNoMapping;
        ExitNow(error = kErrorDrop);
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
        break;
    case Ip4::kProtoTcp:
        ip6Header.SetNextHeader(Ip6::kProtoTcp);
        ip4Headers.SetDestinationPort(dstPortOrId);
        aMessage.Write(0, ip4Headers.GetTcpHeader());
        break;
    case Ip4::kProtoIcmp:
        ip6Header.SetNextHeader(Ip6::kProtoIcmp6);
        SuccessOrExit(TranslateIcmp4(aMessage, dstPortOrId));
        break;
    default:
        dropReason = kReasonUnsupportedProto;
        ExitNow(error = kErrorDrop);
    }

    // TODO: Implement the logic for replying ICMP datagrams.
    ip6Header.SetPayloadLength(aMessage.GetLength() - aMessage.GetOffset());

    Checksum::UpdateMessageChecksum(aMessage, ip6Header.GetSource(), ip6Header.GetDestination(),
                                    ip6Header.GetNextHeader());

    if (aMessage.Prepend(ip6Header) != kErrorNone)
    {
        // This might happen when the platform failed to reserve
        // enough space before the original IPv4 datagram.
        LogWarn("Failed to prepend IPv6 head to translated message");
        ExitNow(error = kErrorDrop);
    }

    aMessage.SetType(Message::kTypeIp6);

    mCounters.Count4To6Packet(ip4Headers);
    mapping->mCounters.Count4To6Packet(ip4Headers);

exit:
    if (error == kErrorDrop)
    {
        mErrorCounters.mCount4To6[dropReason]++;
    }

    return error;
}

Translator::Mapping::InfoString Translator::Mapping::ToString(void) const
{
    InfoString string;

#if OPENTHREAD_CONFIG_NAT64_PORT_TRANSLATION_ENABLE
    string.Append("%s,%u -> %s,%u", mIp6Address.ToString().AsCString(), mSrcPortOrId,
                  mIp4Address.ToString().AsCString(), mTranslatedPortOrId);
#else
    string.Append("%s -> %s", mIp6Address.ToString().AsCString(), mIp4Address.ToString().AsCString());
#endif

    return string;
}

void Translator::Mapping::CopyTo(AddressMapping &aMapping, TimeMilli aNow) const
{
    ClearAllBytes(aMapping);

    aMapping.mId       = mId;
    aMapping.mIp4      = mIp4Address;
    aMapping.mIp6      = mIp6Address;
    aMapping.mCounters = mCounters;
#if OPENTHREAD_CONFIG_NAT64_PORT_TRANSLATION_ENABLE
    aMapping.mSrcPortOrId        = mSrcPortOrId;
    aMapping.mTranslatedPortOrId = mTranslatedPortOrId;
#endif

    // We are removing expired mappings lazily, and an expired mapping
    // might become active again before actually removed. Report the
    // mapping to be "just expired" to avoid confusion.

    aMapping.mRemainingTimeMs = (mExpirationTime < aNow) ? 0 : mExpirationTime - aNow;
}

void Translator::Mapping::Free(void)
{
    LogInfo("Mapping removed: %s", ToString().AsCString());

    Get<Translator>().mMappingPool.Free(*this);
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

void Translator::GetNextIp4Address(Ip4::Address &aIp4Address)
{
    aIp4Address.SynthesizeFromCidrAndHost(mIp4Cidr, mNextHostId);

    mNextHostId++;

    if (mNextHostId > mMaxHostId)
    {
        mNextHostId = mMinHostId;
    }
}

#if !OPENTHREAD_CONFIG_NAT64_PORT_TRANSLATION_ENABLE

Error Translator::AllocateIp4Address(Ip4::Address &aIp4Address)
{
    // Allocate a currently unused IPv4 address from the CIDR range.
    // If all addresses are allocated, try to free expired mappings.

    Error    error = kErrorNone;
    uint32_t numberOfHosts;

    numberOfHosts = mMaxHostId - mMinHostId + 1;

    if (mActiveMappings.CountAllEntries() >= numberOfHosts)
    {
        EvictStaleMapping();
        VerifyOrExit(mActiveMappings.CountAllEntries() < numberOfHosts, error = kErrorFailed);
    }

    do
    {
        GetNextIp4Address(aIp4Address);
    } while (mActiveMappings.ContainsMatching(aIp4Address));

exit:
    return error;
}

#endif

Translator::Mapping *Translator::AllocateMapping(const Ip6::Headers &aIp6Headers)
{
    Mapping     *mapping = nullptr;
    Ip4::Address ip4Addr;

    // When `PORT_TRANSLATION_ENABLE` is enabled, the translator uses
    // IPv4 addresses from the provided CIDR range sequentially. In
    // this case, mappings can share an IPv4 address because they are
    // distinguished by their translated port numbers.
    //
    // Otherwise, a 1-to-1 address mapping is used, where each IPv6
    // address is mapped to a unique IPv4 address from the available
    // range.

#if OPENTHREAD_CONFIG_NAT64_PORT_TRANSLATION_ENABLE
    GetNextIp4Address(ip4Addr);
#else
    SuccessOrExit(AllocateIp4Address(ip4Addr));
#endif

    mapping = mMappingPool.Allocate();

    if (mapping == nullptr)
    {
        EvictStaleMapping();
        mapping = mMappingPool.Allocate();
    }

    VerifyOrExit(mapping != nullptr);

    mActiveMappings.Push(*mapping);
    mapping->mCounters.Clear();
    mapping->mId         = ++mNextMappingId;
    mapping->mIp6Address = aIp6Headers.GetSourceAddress();
    mapping->mIp4Address = ip4Addr;
#if OPENTHREAD_CONFIG_NAT64_PORT_TRANSLATION_ENABLE
    mapping->mSrcPortOrId        = GetSourcePortOrIcmp6Id(aIp6Headers);
    mapping->mTranslatedPortOrId = AllocateSourcePort(mapping->mSrcPortOrId);
#endif

    LogInfo("Mapping created: %s", mapping->ToString().AsCString());

exit:
    return mapping;
}

Translator::Mapping::ProtoCategory Translator::Mapping::DetermineProtocolCategory(void) const
{
    ProtoCategory category;

    if (!IsCounterZero(mCounters.mTcp))
    {
        category = kTcpAndMaybeOthers;
    }
    else if (!IsCounterZero(mCounters.mUdp))
    {
        category = kUdpAndMaybeIcmp;
    }
    else
    {
        category = kIcmpOnly;
    }

    return category;
}

bool Translator::Mapping::IsCounterZero(const ProtocolCounters::Counters &aCounters)
{
    return (aCounters.m6To4Packets == 0) && (aCounters.m4To6Packets == 0);
}

bool Translator::Mapping::IsEligibleForEviction(TimeMilli aNow) const
{
    return DetermineDurationSinceUse(aNow) >= kMinEvictTimeout;
}

bool Translator::Mapping::IsBetterEvictionCandidateOver(const Mapping &aOther, TimeMilli aNow) const
{
    // Determines whether this mapping is a better candidate for
    // eviction compared to `aOther`. This method assumes both
    // mappings are already eligible for eviction.
    //
    // The eviction preference, from most to least preferred to be
    // evicted, is:
    //
    //   1) ICMP-only mappings.
    //   2) UDP mappings (maybe used with ICMP as well).
    //   3) TCP mappings (maybe used with UDP or ICMP).
    //
    // If both mappings are in the same protocol category, the one
    // that hasn't been used for the longest time is preferred.

    bool isBetter = false;
    int  compare;

    // Smaller value for `DetermineProtocolCategory` is preferred.
    // (kIcmpOnly -> kUdpAndMaybeIcmp, -> kTcpAndMaybeOthers).

    static_assert(kIcmpOnly < kUdpAndMaybeIcmp, "ProtoCategory enum constants are invalid");
    static_assert(kUdpAndMaybeIcmp < kTcpAndMaybeOthers, "ProtoCategory enum constants are invalid");

    compare = ThreeWayCompare<uint8_t>(DetermineProtocolCategory(), aOther.DetermineProtocolCategory());

    if (compare < 0)
    {
        isBetter = true;
    }
    else if (compare == 0)
    {
        isBetter = (DetermineDurationSinceUse(aNow) > aOther.DetermineDurationSinceUse(aNow));
    }

    return isBetter;
}

void Translator::EvictStaleMapping(void)
{
    // First tries to remove expired mappings, if there is no expired
    // mapping, it will then try to evict a stale mapping.

    TimeMilli      now            = TimerMilli::GetNow();
    const Mapping *evictCandidate = nullptr;
    bool           didRemoveAny;

    didRemoveAny = mActiveMappings.RemoveAndFreeAllMatching(ExpirationChecker(now));

    VerifyOrExit(!didRemoveAny);

    for (const Mapping &mapping : mActiveMappings)
    {
        if (!mapping.IsEligibleForEviction(now))
        {
            continue;
        }

        if ((evictCandidate == nullptr) || mapping.IsBetterEvictionCandidateOver(*evictCandidate, now))
        {
            evictCandidate = &mapping;
        }
    }

    if (evictCandidate != nullptr)
    {
        mActiveMappings.RemoveMatching(*evictCandidate);
    }

exit:
    return;
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

    mLastUseTime    = TimerMilli::GetNow();
    mExpirationTime = mLastUseTime + timeout;
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
    Error   error = kErrorNone;
    uint8_t len   = aCidr.GetLength();

    VerifyOrExit(IsValueInRange<uint8_t>(len, 1, BitSizeOf(Ip4::Address)), error = kErrorInvalidArgs);

    VerifyOrExit(mIp4Cidr != aCidr);

    // Determine the usable host ID range based on the CIDR prefix
    // length. For prefixes /1 through /30, the all-zeros and
    // all-ones host IDs are excluded. For /31 and /32 prefixes,
    // all host IDs are used.

    mMinHostId = 0;
    mMaxHostId = (1 << (BitSizeOf(Ip4::Address) - len)) - 1;

    if (len < BitSizeOf(Ip4::Address) - 1)
    {
        mMinHostId++;
        mMaxHostId--;
    }

    mNextHostId = mMinHostId;

    mActiveMappings.Free();

    LogInfo("IPv4 CIDR for NAT64: %s (%lu addresses)", aCidr.ToString().AsCString(),
            ToUlong(mMaxHostId - mMinHostId + 1));

    mIp4Cidr = aCidr;

    UpdateState();

    Get<Notifier>().Signal(kEventNat64TranslatorStateChanged);

exit:
    return error;
}

void Translator::ClearIp4Cidr(void)
{
    VerifyOrExit(mIp4Cidr.mLength != 0);

    LogInfo("Clearing IPv4 CIDR");

    mIp4Cidr.Clear();
    mActiveMappings.Free();

    UpdateState();

exit:
    return;
}

Error Translator::GetIp4Cidr(Ip4::Cidr &aCidr) const
{
    Error error = kErrorNone;

    VerifyOrExit(mIp4Cidr.mLength > 0, error = kErrorNotFound);
    aCidr = mIp4Cidr;

exit:
    return error;
}

void Translator::SetNat64Prefix(const Ip6::Prefix &aNat64Prefix)
{
    VerifyOrExit(mNat64Prefix != aNat64Prefix);
    LogInfo("NAT64 Prefix: %s -> %s", mNat64Prefix.ToString().AsCString(), aNat64Prefix.ToString().AsCString());

    mNat64Prefix = aNat64Prefix;
    UpdateState();

exit:
    return;
}

void Translator::ClearNat64Prefix(void)
{
    Ip6::Prefix prefix;

    prefix.Clear();
    SetNat64Prefix(prefix);
}

Error Translator::GetNat64Prefix(Ip6::Prefix &aPrefix) const
{
    Error error = kErrorNone;

    VerifyOrExit(mNat64Prefix.mLength > 0, error = kErrorNotFound);
    aPrefix = mNat64Prefix;

exit:
    return error;
}

void Translator::HandleTimer(void)
{
    mActiveMappings.RemoveAndFreeAllMatching(ExpirationChecker(TimerMilli::GetNow()));
    mTimer.Start(Min(kIcmpTimeout, kIdleTimeout));
}

void Translator::AddressMappingIterator::Init(Instance &aInstance)
{
    SetMapping(aInstance.Get<Translator>().mActiveMappings.GetHead());
    SetInitTime(TimerMilli::GetNow());
}

Error Translator::AddressMappingIterator::GetNext(AddressMapping &aMapping)
{
    Error error = kErrorNone;

    VerifyOrExit(GetMapping() != nullptr, error = kErrorNotFound);

    GetMapping()->CopyTo(aMapping, GetInitTime());
    SetMapping(GetMapping()->GetNext());

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

void Translator::SetState(State aState)
{
    VerifyOrExit(mState != aState);

    LogInfo("State: %s -> %s", StateToString(mState), StateToString(aState));
    mState = aState;

    Get<Notifier>().Signal(kEventNat64TranslatorStateChanged);

    switch (mState)
    {
    case kStateDisabled:
    case kStateNotRunning:
    case kStateIdle:
        mActiveMappings.Free();
        break;
    case kStateActive:
        break;
    }

exit:
    return;
}

bool Translator::HasValidPrefixAndCidr(void) const { return (mIp4Cidr.mLength > 0) && mNat64Prefix.IsValidNat64(); }

void Translator::UpdateState(void)
{
    if (IsEnabled())
    {
        SetState(HasValidPrefixAndCidr() ? kStateActive : kStateNotRunning);
    }
}

void Translator::SetEnabled(bool aEnable)
{
    if (aEnable)
    {
        SetState(HasValidPrefixAndCidr() ? kStateActive : kStateNotRunning);
    }
    else
    {
        SetState(kStateDisabled);
    }
}

} // namespace Nat64
} // namespace ot

#endif // OPENTHREAD_CONFIG_NAT64_TRANSLATOR_ENABLE
