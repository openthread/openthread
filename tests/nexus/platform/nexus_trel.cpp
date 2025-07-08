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

#include <openthread/platform/trel.h>

#include "nexus_core.hpp"
#include "nexus_node.hpp"

#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE

namespace ot {
namespace Nexus {

//---------------------------------------------------------------------------------------------------------------------
// otPlatTrel APIs

extern "C" {

void otPlatTrelEnable(otInstance *aInstance, uint16_t *aUdpPort) { AsNode(aInstance).mTrel.Enable(*aUdpPort); }

void otPlatTrelDisable(otInstance *aInstance) { AsNode(aInstance).mTrel.Disable(); }

void otPlatTrelSend(otInstance       *aInstance,
                    const uint8_t    *aUdpPayload,
                    uint16_t          aUdpPayloadLen,
                    const otSockAddr *aDestSockAddr)
{
    AsNode(aInstance).mTrel.Send(aUdpPayload, aUdpPayloadLen, AsCoreType(aDestSockAddr));
}

void otPlatTrelNotifyPeerSocketAddressDifference(otInstance       *aInstance,
                                                 const otSockAddr *aPeerSockAddr,
                                                 const otSockAddr *aRxSockAddr)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aPeerSockAddr);
    OT_UNUSED_VARIABLE(aRxSockAddr);
}

const otPlatTrelCounters *otPlatTrelGetCounters(otInstance *aInstance) { return &AsNode(aInstance).mTrel.mCounters; }

void otPlatTrelResetCounters(otInstance *aInstance) { AsNode(aInstance).mTrel.ResetCounters(); }

} // extern "C"

//---------------------------------------------------------------------------------------------------------------------
// Trel

uint16_t Trel::sLastUsedUdpPort = kUdpPortStart;

Trel::Trel(void)
    : mEnabled(false)
    , mUdpPort(sLastUsedUdpPort++)
{
    ClearAllBytes(mCounters);
}

void Trel::Reset(void)
{
    mEnabled = false;
    ClearAllBytes(mCounters);
    mPendingTxList.Free();
}

void Trel::Enable(uint16_t &aUdpPort)
{
    mEnabled = true;
    aUdpPort = mUdpPort;
}

void Trel::Disable(void)
{
    mEnabled = false;
    mPendingTxList.Free();
}

void Trel::Send(const uint8_t *aUdpPayload, uint16_t aUdpPayloadLen, const Ip6::SockAddr &aDestSockAddr)
{
    PendingTx *pendingTx = PendingTx::Allocate();

    VerifyOrQuit(pendingTx != nullptr);
    SuccessOrQuit(pendingTx->mPayloadData.SetFrom(aUdpPayload, aUdpPayloadLen));
    pendingTx->mDestSockAddr = aDestSockAddr;

    mPendingTxList.PushAfterTail(*pendingTx);

    mCounters.mTxPackets++;
    mCounters.mTxBytes += aUdpPayloadLen;
}

void Trel::Receive(Instance &aInstance, Heap::Data &aPayloadData, const Ip6::SockAddr &aSenderAddr)
{
    mCounters.mRxPackets++;
    mCounters.mRxBytes += aPayloadData.GetLength();

    otPlatTrelHandleReceived(&aInstance, AsNonConst(aPayloadData.GetBytes()), aPayloadData.GetLength(), &aSenderAddr);
}

} // namespace Nexus
} // namespace ot

#endif // OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
