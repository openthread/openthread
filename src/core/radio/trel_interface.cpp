/*
 *    Copyright (c) 2019, The OpenThread Authors.
 *    All rights reserved.
 *
 *    Redistribution and use in source and binary forms, with or without
 *    modification, are permitted provided that the following conditions are met:
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *    3. Neither the name of the copyright holder nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 *    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 *    ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 *    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
 *    DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 *    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 *    ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 *    SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file
 *   This file implements Thread Radio Encapsulation Link (TREL) interface.
 */

#include "trel_interface.hpp"

#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE

#include "instance/instance.hpp"

namespace ot {
namespace Trel {

RegisterLogModule("TrelInterface");

Interface::Interface(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mInitialized(false)
    , mEnabled(false)
    , mFiltered(false)
{
}

void Interface::Init(void)
{
    OT_ASSERT(!mInitialized);

    mInitialized = true;

    if (mEnabled)
    {
        mEnabled = false;
        Enable();
    }
}

void Interface::SetEnabled(bool aEnable)
{
    if (aEnable)
    {
        Enable();
    }
    else
    {
        Disable();
    }
}

void Interface::Enable(void)
{
    VerifyOrExit(!mEnabled);

    mEnabled = true;
    VerifyOrExit(mInitialized);

    otPlatTrelEnable(&GetInstance(), &mUdpPort);
    Get<PeerDiscoverer>().Start();

    LogInfo("Enabled interface, local port:%u", mUdpPort);

exit:
    return;
}

void Interface::Disable(void)
{
    VerifyOrExit(mEnabled);

    mEnabled = false;
    VerifyOrExit(mInitialized);

    otPlatTrelDisable(&GetInstance());
    Get<PeerDiscoverer>().Stop();

    LogDebg("Disabled interface");

exit:
    return;
}

const Counters *Interface::GetCounters(void) const { return otPlatTrelGetCounters(&GetInstance()); }

void Interface::ResetCounters(void) { otPlatTrelResetCounters(&GetInstance()); }

Error Interface::Send(Packet &aPacket, bool aIsDiscovery)
{
    Error error = kErrorNone;
    Peer *peerEntry;

    VerifyOrExit(mInitialized && mEnabled, error = kErrorAbort);
    VerifyOrExit(!mFiltered);

    switch (aPacket.GetHeader().GetType())
    {
    case Header::kTypeBroadcast:
        for (const Peer &peer : Get<PeerTable>())
        {
            uint16_t        originalPacketNumber = aPacket.GetHeader().GetPacketNumber();
            Header::AckMode originalAckMode      = aPacket.GetHeader().GetAckMode();
            Neighbor       *neighbor;

            if (!peer.IsStateValid())
            {
                continue;
            }

            if (!aIsDiscovery && (peer.GetExtPanId() != Get<MeshCoP::ExtendedPanIdManager>().GetExtPanId()))
            {
                continue;
            }

            neighbor = Get<NeighborTable>().FindNeighbor(peer.GetExtAddress(), Neighbor::kInStateAnyExceptInvalid);

            if (neighbor != nullptr)
            {
                aPacket.GetHeader().SetAckMode(Header::kAckRequested);
                aPacket.GetHeader().SetPacketNumber(neighbor->mTrelTxPacketNumber++);
                neighbor->mTrelCurrentPendingAcks++;
            }

            otPlatTrelSend(&GetInstance(), aPacket.GetBuffer(), aPacket.GetLength(), &peer.mSockAddr);

            aPacket.GetHeader().SetPacketNumber(originalPacketNumber);
            aPacket.GetHeader().SetAckMode(originalAckMode);
        }
        break;

    case Header::kTypeUnicast:
    case Header::kTypeAck:
        peerEntry = Get<PeerTable>().FindMatching(aPacket.GetHeader().GetDestination());
        VerifyOrExit((peerEntry != nullptr) && peerEntry->IsStateValid(), error = kErrorAbort);
        otPlatTrelSend(&GetInstance(), aPacket.GetBuffer(), aPacket.GetLength(), &peerEntry->mSockAddr);
        break;
    }

exit:
    return error;
}

extern "C" void otPlatTrelHandleReceived(otInstance       *aInstance,
                                         uint8_t          *aBuffer,
                                         uint16_t          aLength,
                                         const otSockAddr *aSenderAddress)
{
    Instance &instance = AsCoreType(aInstance);

    VerifyOrExit(instance.IsInitialized());
    instance.Get<Interface>().HandleReceived(aBuffer, aLength, AsCoreType(aSenderAddress));

exit:
    return;
}

void Interface::HandleReceived(uint8_t *aBuffer, uint16_t aLength, const Ip6::SockAddr &aSenderAddr)
{
    LogDebg("HandleReceived(aLength:%u)", aLength);

    VerifyOrExit(mInitialized && mEnabled && !mFiltered);

    mRxPacket.Init(aBuffer, aLength);
    Get<Link>().ProcessReceivedPacket(mRxPacket, aSenderAddr);

exit:
    return;
}

} // namespace Trel
} // namespace ot

#endif // #if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
