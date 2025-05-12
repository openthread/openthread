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

const char Interface::kTxtRecordExtAddressKey[] = "xa";
const char Interface::kTxtRecordExtPanIdKey[]   = "xp";

Interface::Interface(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mInitialized(false)
    , mEnabled(false)
    , mFiltered(false)
    , mRegisterServiceTask(aInstance)
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

    LogInfo("Enabled interface, local port:%u", mUdpPort);
    mRegisterServiceTask.Post();

exit:
    return;
}

void Interface::Disable(void)
{
    VerifyOrExit(mEnabled);

    mEnabled = false;
    VerifyOrExit(mInitialized);

    otPlatTrelDisable(&GetInstance());
    Get<PeerTable>().Clear();
    LogDebg("Disabled interface");

exit:
    return;
}

void Interface::NotifyPeerSocketAddressDifference(const Ip6::SockAddr &aPeerSockAddr, const Ip6::SockAddr &aRxSockAddr)
{
    otPlatTrelNotifyPeerSocketAddressDifference(&GetInstance(), &aPeerSockAddr, &aRxSockAddr);
}

void Interface::HandleExtAddressChange(void)
{
    VerifyOrExit(mInitialized && mEnabled);
    LogDebg("Extended Address changed, re-registering DNS-SD service");
    mRegisterServiceTask.Post();

exit:
    return;
}

void Interface::HandleExtPanIdChange(void)
{
    VerifyOrExit(mInitialized && mEnabled);
    LogDebg("Extended PAN ID changed, re-registering DNS-SD service");
    mRegisterServiceTask.Post();

exit:
    return;
}

void Interface::RegisterService(void)
{
    // TXT data consists of two entries: the length fields, the
    // "key" string, "=" char, and binary representation of the MAC
    // or Extended PAN ID values.
    static constexpr uint8_t kTxtDataSize =
        /* ExtAddr  */ sizeof(uint8_t) + sizeof(kTxtRecordExtAddressKey) - 1 + sizeof(char) + sizeof(Mac::ExtAddress) +
        /* ExtPanId */ sizeof(uint8_t) + sizeof(kTxtRecordExtPanIdKey) - 1 + sizeof(char) +
        sizeof(MeshCoP::ExtendedPanId);

    uint8_t             txtData[kTxtDataSize];
    Dns::TxtDataEncoder encoder(txtData, sizeof(txtData));

    VerifyOrExit(mInitialized && mEnabled);

    SuccessOrAssert(encoder.AppendEntry(kTxtRecordExtAddressKey, Get<Mac::Mac>().GetExtAddress()));
    SuccessOrAssert(encoder.AppendEntry(kTxtRecordExtPanIdKey, Get<MeshCoP::ExtendedPanIdManager>().GetExtPanId()));

    LogInfo("Registering DNS-SD service: port:%u, txt:\"%s=%s, %s=%s\"", mUdpPort, kTxtRecordExtAddressKey,
            Get<Mac::Mac>().GetExtAddress().ToString().AsCString(), kTxtRecordExtPanIdKey,
            Get<MeshCoP::ExtendedPanIdManager>().GetExtPanId().ToString().AsCString());

    otPlatTrelRegisterService(&GetInstance(), mUdpPort, txtData, static_cast<uint8_t>(encoder.GetLength()));

exit:
    return;
}

extern "C" void otPlatTrelHandleDiscoveredPeerInfo(otInstance *aInstance, const otPlatTrelPeerInfo *aInfo)
{
    Instance &instance = AsCoreType(aInstance);

    VerifyOrExit(instance.IsInitialized());
    instance.Get<Interface>().HandleDiscoveredPeerInfo(*static_cast<const Interface::PeerInfo *>(aInfo));

exit:
    return;
}

void Interface::HandleDiscoveredPeerInfo(const PeerInfo &aInfo)
{
    Peer                  *peer;
    Mac::ExtAddress        extAddress;
    MeshCoP::ExtendedPanId extPanId;
    bool                   isNew = false;

    VerifyOrExit(mInitialized && mEnabled);

    SuccessOrExit(aInfo.ParseTxtData(extAddress, extPanId));

    VerifyOrExit(extAddress != Get<Mac::Mac>().GetExtAddress());

    if (aInfo.IsRemoved())
    {
        Get<PeerTable>().RemoveAndFreeAllMatching(extAddress);
        ExitNow();
    }

    // It is a new entry or an update to an existing entry. First
    // check whether we have an existing entry that matches the same
    // socket address, and remove it if it is associated with a
    // different Extended MAC address. This ensures that we do not
    // keep stale entries in the peer table.

    peer = Get<PeerTable>().FindMatching(aInfo.GetSockAddr());

    if ((peer != nullptr) && !peer->Matches(extAddress))
    {
        Get<PeerTable>().RemoveMatching(aInfo.GetSockAddr());
        peer = nullptr;
    }

    if (peer == nullptr)
    {
        peer = Get<PeerTable>().FindMatching(extAddress);
    }

    if (peer == nullptr)
    {
        peer = Get<PeerTable>().AllocateAndAddNewPeer();
        VerifyOrExit(peer != nullptr);

        peer->SetExtAddress(extAddress);
        isNew = true;
    }

    if (!isNew)
    {
        VerifyOrExit((peer->GetExtPanId() != extPanId) || (peer->GetSockAddr() != aInfo.GetSockAddr()));
    }

    peer->SetExtPanId(extPanId);
    peer->SetSockAddr(aInfo.GetSockAddr());

    peer->Log(isNew ? Peer::kAdded : Peer::kUpdated);

exit:
    return;
}

Error Interface::PeerInfo::ParseTxtData(Mac::ExtAddress &aExtAddress, MeshCoP::ExtendedPanId &aExtPanId) const
{
    Error                   error;
    Dns::TxtEntry           entry;
    Dns::TxtEntry::Iterator iterator;
    bool                    parsedExtAddress = false;
    bool                    parsedExtPanId   = false;

    aExtPanId.Clear();

    iterator.Init(mTxtData, mTxtLength);

    while ((error = iterator.GetNextEntry(entry)) == kErrorNone)
    {
        // If the TXT data happens to have entries with key longer
        // than `kMaxIterKeyLength`, `mKey` would be `nullptr` and full
        // entry would be placed in `mValue`. We skip over such
        // entries.
        if (entry.mKey == nullptr)
        {
            continue;
        }

        if (StringMatch(entry.mKey, kTxtRecordExtAddressKey))
        {
            VerifyOrExit(!parsedExtAddress, error = kErrorParse);
            VerifyOrExit(entry.mValueLength >= sizeof(Mac::ExtAddress), error = kErrorParse);
            aExtAddress.Set(entry.mValue);
            parsedExtAddress = true;
        }
        else if (StringMatch(entry.mKey, kTxtRecordExtPanIdKey))
        {
            VerifyOrExit(!parsedExtPanId, error = kErrorParse);
            VerifyOrExit(entry.mValueLength >= sizeof(MeshCoP::ExtendedPanId), error = kErrorParse);
            memcpy(aExtPanId.m8, entry.mValue, sizeof(MeshCoP::ExtendedPanId));
            parsedExtPanId = true;
        }

        // Skip over and ignore any unknown keys.
    }

    VerifyOrExit(error == kErrorNotFound);
    error = kErrorNone;

    VerifyOrExit(parsedExtAddress && parsedExtPanId, error = kErrorParse);

exit:
    return error;
}

const Counters *Interface::GetCounters(void) const { return otPlatTrelGetCounters(&GetInstance()); }

void Interface::ResetCounters(void) { otPlatTrelResetCounters(&GetInstance()); }

Error Interface::Send(const Packet &aPacket, bool aIsDiscovery)
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
            if (!aIsDiscovery && (peer.GetExtPanId() != Get<MeshCoP::ExtendedPanIdManager>().GetExtPanId()))
            {
                continue;
            }

            otPlatTrelSend(&GetInstance(), aPacket.GetBuffer(), aPacket.GetLength(), &peer.mSockAddr);
        }
        break;

    case Header::kTypeUnicast:
    case Header::kTypeAck:
        peerEntry = Get<PeerTable>().FindMatching(aPacket.GetHeader().GetDestination());
        VerifyOrExit(peerEntry != nullptr, error = kErrorAbort);
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
