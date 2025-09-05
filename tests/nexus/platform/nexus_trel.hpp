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

#ifndef OT_NEXUS_TREL_HPP_
#define OT_NEXUS_TREL_HPP_

#include "instance/instance.hpp"

#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE

namespace ot {
namespace Nexus {

struct Trel
{
    static constexpr uint16_t kUdpPortStart = 49152;

    typedef otPlatTrelCounters Counters;

    Trel(void);
    void Reset(void);
    void Enable(uint16_t &aUdpPort);
    void Disable(void);
    void Send(const uint8_t *aUdpPayload, uint16_t aUdpPayloadLen, const Ip6::SockAddr &aDestSockAddr);
    void ResetCounters(void) { ClearAllBytes(mCounters); }
    void Receive(Instance &aInstance, Heap::Data &aPayloadData, const Ip6::SockAddr &aSenderAddr);

    struct PendingTx : public Heap::Allocatable<PendingTx>, public LinkedListEntry<PendingTx>
    {
        PendingTx    *mNext;
        Heap::Data    mPayloadData;
        Ip6::SockAddr mDestSockAddr;
    };

    static uint16_t sLastUsedUdpPort;

    bool                  mEnabled;
    uint16_t              mUdpPort;
    OwningList<PendingTx> mPendingTxList;
    Counters              mCounters;
};

} // namespace Nexus
} // namespace ot

#endif // OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE

#endif // OT_NEXUS_TREL_HPP_
