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

#ifndef OT_NEXUS_PLATFORM_NEXUS_INFRA_IF_HPP_
#define OT_NEXUS_PLATFORM_NEXUS_INFRA_IF_HPP_

#include <openthread/platform/infra_if.h>
#include "instance/instance.hpp"

namespace ot {
namespace Nexus {

class Node;

class InfraIf
{
public:
    InfraIf(void);

    void Init(Node &aNode);

    bool IsInitialized(void) const { return mIfIndex != 0; }

    bool HasAddress(const Ip6::Address &aAddress) const;
    void AddAddress(const Ip6::Address &aAddress);
    void RemoveAddress(const Ip6::Address &aAddress);

    const Ip6::Address *FindAddress(const char *aPrefix) const;
    const Ip6::Address &FindMatchingAddress(const char *aPrefix) const;

    const Ip6::Address &GetLinkLocalAddress(void) const { return mAddresses[0]; }

    void SendIcmp6Nd(const Ip6::Address &aDestAddress, const uint8_t *aBuffer, uint16_t aBufferLength);
    void SendIp6(const Ip6::Address &aSrcAddress,
                 const Ip6::Address &aDestAddress,
                 const uint8_t      *aBuffer,
                 uint16_t            aBufferLength);
    void SendEchoRequest(const Ip6::Address &aSrcAddress,
                         const Ip6::Address &aDestAddress,
                         uint16_t            aIdentifier,
                         uint16_t            aPayloadSize);
    void Receive(Node &aSrcNode, const Ip6::Header &aHeader, Message &aMessage);
    void GetLinkLayerAddress(otPlatInfraIfLinkLayerAddress &aLinkLayerAddress) const;

    Node       &GetNode(void);
    const Node &GetNode(void) const;

    MessageQueue mPendingTxQueue;

private:
    void ProcessIcmp6Nd(const Ip6::Address &aSrcAddress, const uint8_t *aBuffer, uint16_t aBufferLength);
    void HandlePrefixInfoOption(const Ip6::Nd::PrefixInfoOption &aPio);
    void HandleEchoRequest(const Ip6::Header &aHeader, Message &aMessage);

    Node                     *mNode;
    uint32_t                  mNodeId;
    uint32_t                  mIfIndex;
    Heap::Array<Ip6::Address> mAddresses;
};

} // namespace Nexus
} // namespace ot

#endif // OT_NEXUS_PLATFORM_NEXUS_INFRA_IF_HPP_
