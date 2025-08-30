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

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "platform/nexus_core.hpp"
#include "platform/nexus_node.hpp"

namespace ot {
namespace Nexus {

static bool           sNotifierCallbackInvoked = false;
static otChangedFlags sNotifierEvents          = 0;

void HandleNotifierEvent(otChangedFlags aFlags, void *aContext)
{
    VerifyOrQuit(aContext == nullptr);

    sNotifierCallbackInvoked = true;
    sNotifierEvents          = aFlags;
}

void TestNat64StateChanges(void)
{
    Core                                      nexus;
    Node                                     &node = nexus.CreateNode();
    Ip6::Prefix                               prefix, testPrefix;
    Ip4::Cidr                                 cidr, testCidr;
    Nat64::Translator::AddressMappingIterator iterator;
    Nat64::Translator::AddressMapping         mapping;

    Log("------------------------------------------------------------------------------------------------------");
    Log("TestNat64StateChanges");

    nexus.AdvanceTime(0);

    node.Form();
    nexus.AdvanceTime(50 * Time::kOneSecondInMsec);
    VerifyOrQuit(node.Get<Mle::Mle>().IsLeader());

    node.Get<Instance>().SetLogLevel(kLogLevelInfo);

    SuccessOrQuit(node.Get<Notifier>().RegisterCallback(HandleNotifierEvent, nullptr));

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Check NAT64 Translator's initial state");

    VerifyOrQuit(node.Get<Nat64::Translator>().GetState() == Nat64::kStateDisabled);

    VerifyOrQuit(node.Get<Nat64::Translator>().GetIp4Cidr(cidr) == kErrorNotFound);
    VerifyOrQuit(node.Get<Nat64::Translator>().GetIp6Prefix(prefix) == kErrorNotFound);

    iterator.Init(node.GetInstance());
    VerifyOrQuit(iterator.GetNext(mapping) == kErrorNotFound);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Enable NAT64");

    node.Get<Nat64::Translator>().SetEnabled(true);

    VerifyOrQuit(node.Get<Nat64::Translator>().GetState() == Nat64::kStateNotRunning);

    VerifyOrQuit(node.Get<Nat64::Translator>().GetIp4Cidr(cidr) == kErrorNotFound);
    VerifyOrQuit(node.Get<Nat64::Translator>().GetIp6Prefix(prefix) == kErrorNotFound);

    iterator.Init(node.GetInstance());
    VerifyOrQuit(iterator.GetNext(mapping) == kErrorNotFound);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Set an invalid CIDR");

    testCidr.Clear();
    VerifyOrQuit(node.Get<Nat64::Translator>().SetIp4Cidr(testCidr) == kErrorInvalidArgs);

    VerifyOrQuit(node.Get<Nat64::Translator>().GetState() == Nat64::kStateNotRunning);

    VerifyOrQuit(node.Get<Nat64::Translator>().GetIp4Cidr(cidr) == kErrorNotFound);
    VerifyOrQuit(node.Get<Nat64::Translator>().GetIp6Prefix(prefix) == kErrorNotFound);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Set a valid CIDR");

    SuccessOrQuit(testCidr.FromString("192.168.100.0/8"));
    SuccessOrQuit(node.Get<Nat64::Translator>().SetIp4Cidr(testCidr));

    VerifyOrQuit(node.Get<Nat64::Translator>().GetState() == Nat64::kStateNotRunning);

    VerifyOrQuit(node.Get<Nat64::Translator>().GetIp6Prefix(prefix) == kErrorNotFound);

    SuccessOrQuit(node.Get<Nat64::Translator>().GetIp4Cidr(cidr));
    VerifyOrQuit(cidr == testCidr);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Set a IPv6 NAT64 prefix");

    SuccessOrQuit(testPrefix.FromString("fd01::/96"));
    node.Get<Nat64::Translator>().SetNat64Prefix(testPrefix);

    SuccessOrQuit(node.Get<Nat64::Translator>().GetIp6Prefix(prefix));
    VerifyOrQuit(prefix == testPrefix);

    SuccessOrQuit(node.Get<Nat64::Translator>().GetIp4Cidr(cidr));
    VerifyOrQuit(cidr == testCidr);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Check that NAT64 is now active");

    nexus.AdvanceTime(1);

    VerifyOrQuit(node.Get<Nat64::Translator>().GetState() == Nat64::kStateActive);

    VerifyOrQuit(sNotifierCallbackInvoked);
    VerifyOrQuit(sNotifierEvents & kEventNat64TranslatorStateChanged);

    SuccessOrQuit(node.Get<Nat64::Translator>().GetIp6Prefix(prefix));
    VerifyOrQuit(prefix == testPrefix);

    SuccessOrQuit(node.Get<Nat64::Translator>().GetIp4Cidr(cidr));
    VerifyOrQuit(cidr == testCidr);

    iterator.Init(node.GetInstance());
    VerifyOrQuit(iterator.GetNext(mapping) == kErrorNotFound);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Disable and re-enable NAT64");

    nexus.AdvanceTime(1000);

    VerifyOrQuit(node.Get<Nat64::Translator>().GetState() == Nat64::kStateActive);
    sNotifierCallbackInvoked = false;

    node.Get<Nat64::Translator>().SetEnabled(false);

    VerifyOrQuit(node.Get<Nat64::Translator>().GetState() == Nat64::kStateDisabled);

    SuccessOrQuit(node.Get<Nat64::Translator>().GetIp6Prefix(prefix));
    VerifyOrQuit(prefix == testPrefix);

    SuccessOrQuit(node.Get<Nat64::Translator>().GetIp4Cidr(cidr));
    VerifyOrQuit(cidr == testCidr);

    nexus.AdvanceTime(1);

    VerifyOrQuit(sNotifierCallbackInvoked);
    VerifyOrQuit(sNotifierEvents & kEventNat64TranslatorStateChanged);

    // Re-enable

    sNotifierCallbackInvoked = false;

    node.Get<Nat64::Translator>().SetEnabled(true);

    nexus.AdvanceTime(1);

    VerifyOrQuit(node.Get<Nat64::Translator>().GetState() == Nat64::kStateActive);
    VerifyOrQuit(sNotifierCallbackInvoked);
    VerifyOrQuit(sNotifierEvents & kEventNat64TranslatorStateChanged);

    SuccessOrQuit(node.Get<Nat64::Translator>().GetIp6Prefix(prefix));
    VerifyOrQuit(prefix == testPrefix);

    SuccessOrQuit(node.Get<Nat64::Translator>().GetIp4Cidr(cidr));
    VerifyOrQuit(cidr == testCidr);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Set the NAT64 prefix to the same value");

    nexus.AdvanceTime(1000);

    node.Get<Nat64::Translator>().SetNat64Prefix(testPrefix);

    VerifyOrQuit(node.Get<Nat64::Translator>().GetState() == Nat64::kStateActive);

    SuccessOrQuit(node.Get<Nat64::Translator>().GetIp6Prefix(prefix));
    VerifyOrQuit(prefix == testPrefix);

    SuccessOrQuit(node.Get<Nat64::Translator>().GetIp4Cidr(cidr));
    VerifyOrQuit(cidr == testCidr);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Clear NAT64 prefix and ensure NAT64 is stopped");

    nexus.AdvanceTime(1000);

    sNotifierCallbackInvoked = false;
    node.Get<Nat64::Translator>().ClearNat64Prefix();

    nexus.AdvanceTime(1);
    VerifyOrQuit(node.Get<Nat64::Translator>().GetState() == Nat64::kStateNotRunning);

    VerifyOrQuit(node.Get<Nat64::Translator>().GetIp6Prefix(prefix) == kErrorNotFound);

    SuccessOrQuit(node.Get<Nat64::Translator>().GetIp4Cidr(cidr));
    VerifyOrQuit(cidr == testCidr);

    VerifyOrQuit(sNotifierCallbackInvoked);
    VerifyOrQuit(sNotifierEvents & kEventNat64TranslatorStateChanged);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Change NAT64 prefix and ensure NAT64 is again active");

    sNotifierCallbackInvoked = false;

    SuccessOrQuit(testPrefix.FromString("fd02::/96"));
    node.Get<Nat64::Translator>().SetNat64Prefix(testPrefix);

    nexus.AdvanceTime(1);
    VerifyOrQuit(node.Get<Nat64::Translator>().GetState() == Nat64::kStateActive);

    SuccessOrQuit(node.Get<Nat64::Translator>().GetIp6Prefix(prefix));
    VerifyOrQuit(prefix == testPrefix);

    SuccessOrQuit(node.Get<Nat64::Translator>().GetIp4Cidr(cidr));
    VerifyOrQuit(cidr == testCidr);

    VerifyOrQuit(sNotifierCallbackInvoked);
    VerifyOrQuit(sNotifierEvents & kEventNat64TranslatorStateChanged);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Clear the configured CIDR and ensure NAT64 is stopped");

    nexus.AdvanceTime(1000);

    sNotifierCallbackInvoked = false;
    node.Get<Nat64::Translator>().ClearIp4Cidr();

    nexus.AdvanceTime(1);
    VerifyOrQuit(node.Get<Nat64::Translator>().GetState() == Nat64::kStateNotRunning);

    SuccessOrQuit(node.Get<Nat64::Translator>().GetIp6Prefix(prefix));
    VerifyOrQuit(prefix == testPrefix);

    VerifyOrQuit(node.Get<Nat64::Translator>().GetIp4Cidr(cidr) == kErrorNotFound);

    VerifyOrQuit(sNotifierCallbackInvoked);
    VerifyOrQuit(sNotifierEvents & kEventNat64TranslatorStateChanged);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Set the CIDR again and ensure NAT64 becomes active");

    sNotifierCallbackInvoked = false;

    SuccessOrQuit(testCidr.FromString("192.168.200.1/32"));
    SuccessOrQuit(node.Get<Nat64::Translator>().SetIp4Cidr(testCidr));

    nexus.AdvanceTime(1);
    VerifyOrQuit(node.Get<Nat64::Translator>().GetState() == Nat64::kStateActive);

    SuccessOrQuit(node.Get<Nat64::Translator>().GetIp6Prefix(prefix));
    VerifyOrQuit(prefix == testPrefix);

    SuccessOrQuit(node.Get<Nat64::Translator>().GetIp4Cidr(cidr));
    VerifyOrQuit(cidr == testCidr);

    VerifyOrQuit(sNotifierCallbackInvoked);
    VerifyOrQuit(sNotifierEvents & kEventNat64TranslatorStateChanged);

    Log("End of TestNat64StateChanges");
}

Message *PrepareMessage(Node               &aNode,
                        const Ip6::Address &aSrcIp6Address,
                        const Ip4::Address &aDstIp4Address,
                        uint16_t            aSrcPort    = 1234,
                        uint16_t            aDstPort    = 1235,
                        uint16_t            aPayloadLen = 10)
{
    Message         *message = nullptr;
    Ip6::Prefix      nat64Prefix;
    Ip6::Header      ip6Header;
    Ip6::Udp::Header udpHeader;

    message = aNode.Get<MessagePool>().Allocate(Message::kTypeIp6);
    VerifyOrQuit(message != nullptr);

    ip6Header.Clear();
    ip6Header.InitVersionTrafficClassFlow();

    ip6Header.SetSource(aSrcIp6Address);

    SuccessOrQuit(aNode.Get<Nat64::Translator>().GetIp6Prefix(nat64Prefix));
    ip6Header.GetDestination().SynthesizeFromIp4Address(nat64Prefix, aDstIp4Address);

    ip6Header.SetNextHeader(Ip6::kProtoUdp);
    ip6Header.SetPayloadLength(sizeof(Ip6::Udp::Header) + aPayloadLen);

    SuccessOrQuit(message->Append(ip6Header));

    udpHeader.Clear();
    udpHeader.SetSourcePort(aSrcPort);
    udpHeader.SetDestinationPort(aDstPort);
    udpHeader.SetLength(aPayloadLen);

    SuccessOrQuit(message->Append(udpHeader));

    for (uint16_t i = 0; i < aPayloadLen; i++)
    {
        SuccessOrQuit(message->Append<uint8_t>(static_cast<uint8_t>(i & 0xff)));
    }

    return message;
}

void LogAddressMapping(const Nat64::Translator::AddressMapping &aMapping)
{
    Log("Mapping: %s -> %s, remaining-time:%lu", AsCoreType(&aMapping.mIp6).ToString().AsCString(),
        AsCoreType(&aMapping.mIp4).ToString().AsCString(), ToUlong(aMapping.mRemainingTimeMs));
}

void TestNat64Mapping(void)
{
    static constexpr uint32_t kExpireTimeout = 250 * Time::kOneSecondInMsec;

    static constexpr uint16_t kSrcPort       = 55387;
    static constexpr uint16_t kDstPort       = 55388;
    static constexpr uint16_t kPayloadLength = 32;

    Core                                      nexus;
    Node                                     &node = nexus.CreateNode();
    Ip6::Prefix                               prefix;
    Ip4::Cidr                                 cidr;
    Nat64::Translator::AddressMappingIterator iterator;
    Nat64::Translator::AddressMapping         mapping;
    OwnedPtr<Message>                         message;
    Nat64::Translator::Result                 result;
    Ip6::Address                              ip6Addr;
    Ip6::Address                              ip6Addr2;
    Ip4::Address                              ip4Addr;
    Ip4::Headers                              ip4Headers;
    uint16_t                                  count;

    Log("------------------------------------------------------------------------------------------------------");
    Log("TestNat64Mapping");

    nexus.AdvanceTime(0);

    node.Form();
    nexus.AdvanceTime(50 * Time::kOneSecondInMsec);
    VerifyOrQuit(node.Get<Mle::Mle>().IsLeader());

    node.Get<Instance>().SetLogLevel(kLogLevelInfo);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Enable NAT64 translator");

    SuccessOrQuit(prefix.FromString("fd01::/96"));
    SuccessOrQuit(cidr.FromString("192.168.100.0/24"));

    node.Get<Nat64::Translator>().SetNat64Prefix(prefix);
    SuccessOrQuit(node.Get<Nat64::Translator>().SetIp4Cidr(cidr));

    node.Get<Nat64::Translator>().SetEnabled(true);
    VerifyOrQuit(node.Get<Nat64::Translator>().GetState() == Nat64::kStateActive);

    iterator.Init(node.GetInstance());
    VerifyOrQuit(iterator.GetNext(mapping) == kErrorNotFound);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Translate an IPv6 message");

    SuccessOrQuit(ip6Addr.FromString("fd02::1"));
    SuccessOrQuit(ip4Addr.FromString("200.100.1.1"));

    message.Reset(PrepareMessage(node, ip6Addr, ip4Addr, kSrcPort, kDstPort, kPayloadLength));

    result = node.Get<Nat64::Translator>().TranslateFromIp6(*message);
    VerifyOrQuit(result == Nat64::Translator::kForward);

    SuccessOrQuit(ip4Headers.ParseFrom(*message));
    VerifyOrQuit(ip4Headers.GetDestinationAddress() == ip4Addr);
    VerifyOrQuit(ip4Headers.IsUdp());
    VerifyOrQuit(ip4Headers.GetSourcePort() == kSrcPort);
    VerifyOrQuit(ip4Headers.GetDestinationPort() == kDstPort);
    VerifyOrQuit(ip4Headers.GetUdpHeader().GetLength() == kPayloadLength);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Check the created Address Mapping");

    iterator.Init(node.GetInstance());
    SuccessOrQuit(iterator.GetNext(mapping));
    LogAddressMapping(mapping);

    VerifyOrQuit(AsCoreType(&mapping.mIp6) == ip6Addr);
    VerifyOrQuit(AsCoreType(&mapping.mIp4) == ip4Headers.GetSourceAddress());
    VerifyOrQuit(mapping.mRemainingTimeMs > 0);

    VerifyOrQuit(iterator.GetNext(mapping) == kErrorNotFound);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Translate another IPv6 message from the same IPv6 sender to a new IPv4 dest");

    SuccessOrQuit(ip4Addr.FromString("200.100.1.2"));

    message.Reset(PrepareMessage(node, ip6Addr, ip4Addr, kSrcPort, kDstPort, kPayloadLength * 2));

    result = node.Get<Nat64::Translator>().TranslateFromIp6(*message);
    VerifyOrQuit(result == Nat64::Translator::kForward);

    SuccessOrQuit(ip4Headers.ParseFrom(*message));
    VerifyOrQuit(ip4Headers.GetDestinationAddress() == ip4Addr);
    VerifyOrQuit(ip4Headers.IsUdp());
    VerifyOrQuit(ip4Headers.GetSourcePort() == kSrcPort);
    VerifyOrQuit(ip4Headers.GetDestinationPort() == kDstPort);
    VerifyOrQuit(ip4Headers.GetUdpHeader().GetLength() == kPayloadLength * 2);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Ensure the previous Address Mapping is reused");

    iterator.Init(node.GetInstance());
    SuccessOrQuit(iterator.GetNext(mapping));
    LogAddressMapping(mapping);

    VerifyOrQuit(AsCoreType(&mapping.mIp6) == ip6Addr);
    VerifyOrQuit(AsCoreType(&mapping.mIp4) == ip4Headers.GetSourceAddress());
    VerifyOrQuit(mapping.mRemainingTimeMs > 0);

    VerifyOrQuit(iterator.GetNext(mapping) == kErrorNotFound);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Translate a new IPv6 message from a new IPv6 address");

    nexus.AdvanceTime(30 * 1000);

    SuccessOrQuit(ip6Addr2.FromString("fd02::2"));

    message.Reset(PrepareMessage(node, ip6Addr2, ip4Addr, kSrcPort, kDstPort, kPayloadLength));

    result = node.Get<Nat64::Translator>().TranslateFromIp6(*message);
    VerifyOrQuit(result == Nat64::Translator::kForward);

    SuccessOrQuit(ip4Headers.ParseFrom(*message));
    VerifyOrQuit(ip4Headers.GetDestinationAddress() == ip4Addr);
    VerifyOrQuit(ip4Headers.IsUdp());
    VerifyOrQuit(ip4Headers.GetSourcePort() == kSrcPort);
    VerifyOrQuit(ip4Headers.GetDestinationPort() == kDstPort);
    VerifyOrQuit(ip4Headers.GetUdpHeader().GetLength() == kPayloadLength);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Ensure a new Address Mapping is created");

    iterator.Init(node.GetInstance());

    for (count = 0; iterator.GetNext(mapping) == kErrorNone; count++)
    {
        LogAddressMapping(mapping);

        if (AsCoreType(&mapping.mIp6) == ip6Addr)
        {
            continue;
        }

        VerifyOrQuit(AsCoreType(&mapping.mIp6) == ip6Addr2);
        VerifyOrQuit(AsCoreType(&mapping.mIp4) == ip4Headers.GetSourceAddress());
        VerifyOrQuit(mapping.mRemainingTimeMs > 0);
    }

    VerifyOrQuit(count == 2);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Translate another IPv6 message from with previous used addresses");

    SuccessOrQuit(ip4Addr.FromString("200.100.1.5"));

    message.Reset(PrepareMessage(node, ip6Addr, ip4Addr, kSrcPort, kDstPort, kPayloadLength));

    result = node.Get<Nat64::Translator>().TranslateFromIp6(*message);
    VerifyOrQuit(result == Nat64::Translator::kForward);

    SuccessOrQuit(ip4Headers.ParseFrom(*message));
    VerifyOrQuit(ip4Headers.GetDestinationAddress() == ip4Addr);
    VerifyOrQuit(ip4Headers.IsUdp());
    VerifyOrQuit(ip4Headers.GetSourcePort() == kSrcPort);
    VerifyOrQuit(ip4Headers.GetDestinationPort() == kDstPort);
    VerifyOrQuit(ip4Headers.GetUdpHeader().GetLength() == kPayloadLength);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Ensure the previous Address Mapping is reused");

    iterator.Init(node.GetInstance());

    for (count = 0; iterator.GetNext(mapping) == kErrorNone; count++)
    {
        LogAddressMapping(mapping);

        if (AsCoreType(&mapping.mIp6) == ip6Addr2)
        {
            continue;
        }

        VerifyOrQuit(AsCoreType(&mapping.mIp6) == ip6Addr);
        VerifyOrQuit(AsCoreType(&mapping.mIp4) == ip4Headers.GetSourceAddress());
        VerifyOrQuit(mapping.mRemainingTimeMs > 0);
    }

    VerifyOrQuit(count == 2);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    message.Reset(PrepareMessage(node, ip6Addr2, ip4Addr, kSrcPort, kDstPort, kPayloadLength));

    result = node.Get<Nat64::Translator>().TranslateFromIp6(*message);
    VerifyOrQuit(result == Nat64::Translator::kForward);

    SuccessOrQuit(ip4Headers.ParseFrom(*message));
    VerifyOrQuit(ip4Headers.GetDestinationAddress() == ip4Addr);
    VerifyOrQuit(ip4Headers.IsUdp());
    VerifyOrQuit(ip4Headers.GetSourcePort() == kSrcPort);
    VerifyOrQuit(ip4Headers.GetDestinationPort() == kDstPort);
    VerifyOrQuit(ip4Headers.GetUdpHeader().GetLength() == kPayloadLength);

    iterator.Init(node.GetInstance());

    for (count = 0; iterator.GetNext(mapping) == kErrorNone; count++)
    {
        LogAddressMapping(mapping);

        if (AsCoreType(&mapping.mIp6) == ip6Addr)
        {
            continue;
        }

        VerifyOrQuit(AsCoreType(&mapping.mIp6) == ip6Addr2);
        VerifyOrQuit(AsCoreType(&mapping.mIp4) == ip4Headers.GetSourceAddress());
        VerifyOrQuit(mapping.mRemainingTimeMs > 0);
    }

    VerifyOrQuit(count == 2);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Check Address Mapping expiration and removal");

    nexus.AdvanceTime(kExpireTimeout);

    iterator.Init(node.GetInstance());
    VerifyOrQuit(iterator.GetNext(mapping) == kErrorNotFound);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Translate a new IPv6 message and check that a new Address Mapping is created");

    message.Reset(PrepareMessage(node, ip6Addr, ip4Addr, kSrcPort, kDstPort, kPayloadLength));

    result = node.Get<Nat64::Translator>().TranslateFromIp6(*message);
    VerifyOrQuit(result == Nat64::Translator::kForward);

    SuccessOrQuit(ip4Headers.ParseFrom(*message));
    VerifyOrQuit(ip4Headers.GetDestinationAddress() == ip4Addr);
    VerifyOrQuit(ip4Headers.IsUdp());
    VerifyOrQuit(ip4Headers.GetSourcePort() == kSrcPort);
    VerifyOrQuit(ip4Headers.GetDestinationPort() == kDstPort);
    VerifyOrQuit(ip4Headers.GetUdpHeader().GetLength() == kPayloadLength);

    iterator.Init(node.GetInstance());
    SuccessOrQuit(iterator.GetNext(mapping));
    LogAddressMapping(mapping);

    VerifyOrQuit(AsCoreType(&mapping.mIp6) == ip6Addr);
    VerifyOrQuit(AsCoreType(&mapping.mIp4) == ip4Headers.GetSourceAddress());
    VerifyOrQuit(mapping.mRemainingTimeMs > 0);

    VerifyOrQuit(iterator.GetNext(mapping) == kErrorNotFound);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Change the CIDR and check that mapping list is cleared");

    SuccessOrQuit(cidr.FromString("192.168.200.0/24"));
    SuccessOrQuit(node.Get<Nat64::Translator>().SetIp4Cidr(cidr));

    VerifyOrQuit(node.Get<Nat64::Translator>().GetState() == Nat64::kStateActive);

    iterator.Init(node.GetInstance());
    VerifyOrQuit(iterator.GetNext(mapping) == kErrorNotFound);
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::TestNat64StateChanges();
    ot::Nexus::TestNat64Mapping();

    printf("All tests passed\n");
    return 0;
}
