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

#ifndef OT_NEXUS_MDNS_HPP_
#define OT_NEXUS_MDNS_HPP_

#include "instance/instance.hpp"

namespace ot {
namespace Nexus {

class Mdns
{
public:
    static constexpr uint16_t kUdpPort      = 5353;
    static constexpr uint32_t kInfraIfIndex = 1;

    using AddressInfo = otPlatMdnsAddressInfo;

    struct PendingTx : public Heap::Allocatable<PendingTx>, public LinkedListEntry<PendingTx>
    {
        PendingTx        *mNext;
        OwnedPtr<Message> mMessage;
        bool              mIsUnicast;
        AddressInfo       mAddress;
    };

    Mdns(void);
    Error SetListeningEnabled(Instance &aInstance, bool aEnable, uint32_t aInfraIfIndex);
    void  SendMulticast(Message &aMessage, uint32_t aInfraIfIndex);
    void  SendUnicast(Message &aMessage, const AddressInfo &aAddress);

    void SignalIfAddresses(Instance &aInstance);
    void Receive(Instance &aInstance, const PendingTx &aPendingTx, const AddressInfo &aSenderAddress);
    void GetAddress(AddressInfo &aAddress) const;

    bool                      mEnabled;
    Heap::Array<Ip6::Address> mIfAddresses;
    OwningList<PendingTx>     mPendingTxList;
};

} // namespace Nexus
} // namespace ot

#endif // OT_NEXUS_MDNS_HPP_
