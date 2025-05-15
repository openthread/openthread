/*
 *    Copyright (c) 2019-2025, The OpenThread Authors.
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
 *   This file implements Thread Radio Encapsulation Link (TREL) peer discovery.
 */

#include "trel_peer_discoverer.hpp"

#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE

#include "instance/instance.hpp"

namespace ot {
namespace Trel {

RegisterLogModule("TrelDiscoverer");

PeerDiscoverer::PeerDiscoverer(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mIsRunning(false)
    , mRegisterServiceTask(aInstance)
{
}

void PeerDiscoverer::Start(void)
{
    VerifyOrExit(!mIsRunning);

    mIsRunning = true;
    PostRegisterServiceTask();

exit:
    return;
}

void PeerDiscoverer::Stop(void)
{
    VerifyOrExit(mIsRunning);

    mIsRunning = false;
    Get<PeerTable>().Clear();

exit:
    return;
}

void PeerDiscoverer::NotifyPeerSocketAddressDifference(const Ip6::SockAddr &aPeerSockAddr,
                                                       const Ip6::SockAddr &aRxSockAddr)
{
    otPlatTrelNotifyPeerSocketAddressDifference(&GetInstance(), &aPeerSockAddr, &aRxSockAddr);
}

void PeerDiscoverer::PostRegisterServiceTask(void)
{
    if (mIsRunning)
    {
        mRegisterServiceTask.Post();
    }
}

void PeerDiscoverer::RegisterService(void)
{
    TxtDataEncoder txtData(GetInstance());
    uint16_t       port;

    VerifyOrExit(mIsRunning);

    port = Get<Interface>().GetUdpPort();

    txtData.Encode();

    LogInfo("Registering DNS-SD service: port:%u", port);
    otPlatTrelRegisterService(&GetInstance(), port, txtData.GetBytes(), static_cast<uint8_t>(txtData.GetLength()));

exit:
    return;
}

extern "C" void otPlatTrelHandleDiscoveredPeerInfo(otInstance *aInstance, const otPlatTrelPeerInfo *aInfo)
{
    Instance &instance = AsCoreType(aInstance);

    VerifyOrExit(instance.IsInitialized());
    instance.Get<PeerDiscoverer>().HandleDiscoveredPeerInfo(*static_cast<const PeerDiscoverer::PeerInfo *>(aInfo));

exit:
    return;
}

void PeerDiscoverer::HandleDiscoveredPeerInfo(const PeerInfo &aInfo)
{
    Peer         *peer;
    TxtData       txtData;
    TxtData::Info txtInfo;
    bool          isNew = false;

    VerifyOrExit(mIsRunning);

    txtData.Init(aInfo.mTxtData, aInfo.mTxtLength);
    SuccessOrExit(txtData.Decode(txtInfo));

    VerifyOrExit(txtInfo.mExtAddress != Get<Mac::Mac>().GetExtAddress());

    if (aInfo.IsRemoved())
    {
        Get<PeerTable>().RemoveAndFreeAllMatching(txtInfo.mExtAddress);
        ExitNow();
    }

    // It is a new entry or an update to an existing entry. First
    // check whether we have an existing entry that matches the same
    // socket address, and remove it if it is associated with a
    // different Extended MAC address. This ensures that we do not
    // keep stale entries in the peer table.

    peer = Get<PeerTable>().FindMatching(aInfo.GetSockAddr());

    if ((peer != nullptr) && !peer->Matches(txtInfo.mExtAddress))
    {
        Get<PeerTable>().RemoveMatching(aInfo.GetSockAddr());
        peer = nullptr;
    }

    if (peer == nullptr)
    {
        peer = Get<PeerTable>().FindMatching(txtInfo.mExtAddress);
    }

    if (peer == nullptr)
    {
        peer = Get<PeerTable>().AllocateAndAddNewPeer();
        VerifyOrExit(peer != nullptr);

        peer->SetExtAddress(txtInfo.mExtAddress);
        isNew = true;
    }

    if (!isNew)
    {
        VerifyOrExit((peer->GetExtPanId() != txtInfo.mExtPanId) || (peer->GetSockAddr() != aInfo.GetSockAddr()));
    }

    peer->SetExtPanId(txtInfo.mExtPanId);
    peer->SetSockAddr(aInfo.GetSockAddr());

    peer->Log(isNew ? Peer::kAdded : Peer::kUpdated);

exit:
    return;
}

//----------------------------------------------------------------------------------------------------------------------
// PeerDiscoverer::TxtData

const char PeerDiscoverer::TxtData::kExtAddressKey[] = "xa";
const char PeerDiscoverer::TxtData::kExtPanIdKey[]   = "xp";

void PeerDiscoverer::TxtData::Init(const uint8_t *aData, uint16_t aLength)
{
    mData   = aData;
    mLength = aLength;
}

Error PeerDiscoverer::TxtData::Decode(Info &aInfo)
{
    Error                   error;
    Dns::TxtEntry           entry;
    Dns::TxtEntry::Iterator iterator;
    bool                    parsedExtAddress = false;
    bool                    parsedExtPanId   = false;

    aInfo.Clear();

    iterator.Init(mData, mLength);

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

        if (StringMatch(entry.mKey, kExtAddressKey))
        {
            VerifyOrExit(!parsedExtAddress, error = kErrorParse);
            VerifyOrExit(entry.mValueLength >= sizeof(Mac::ExtAddress), error = kErrorParse);
            aInfo.mExtAddress.Set(entry.mValue);
            parsedExtAddress = true;
        }
        else if (StringMatch(entry.mKey, kExtPanIdKey))
        {
            VerifyOrExit(!parsedExtPanId, error = kErrorParse);
            VerifyOrExit(entry.mValueLength >= sizeof(MeshCoP::ExtendedPanId), error = kErrorParse);
            memcpy(aInfo.mExtPanId.m8, entry.mValue, sizeof(MeshCoP::ExtendedPanId));
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

//----------------------------------------------------------------------------------------------------------------------
// PeerDiscoverer::TxtDataEncoder

PeerDiscoverer::TxtDataEncoder::TxtDataEncoder(Instance &aInstance)
    : InstanceLocator(aInstance)
{
}

void PeerDiscoverer::TxtDataEncoder::Encode(void)
{
    Dns::TxtDataEncoder encoder(mBuffer, sizeof(mBuffer));

    SuccessOrAssert(encoder.AppendEntry(kExtAddressKey, Get<Mac::Mac>().GetExtAddress()));
    SuccessOrAssert(encoder.AppendEntry(kExtPanIdKey, Get<MeshCoP::ExtendedPanIdManager>().GetExtPanId()));

    mData   = mBuffer;
    mLength = encoder.GetLength();
}

} // namespace Trel
} // namespace ot

#endif // #if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
