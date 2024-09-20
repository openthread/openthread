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
    mPeerTable.Clear();
    LogDebg("Disabled interface");

exit:
    return;
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

    uint8_t                        txtDataBuffer[kTxtDataSize];
    MutableData<kWithUint16Length> txtData;
    Dns::TxtEntry                  txtEntries[2];

    VerifyOrExit(mInitialized && mEnabled);

    txtEntries[0].Init(kTxtRecordExtAddressKey, Get<Mac::Mac>().GetExtAddress().m8, sizeof(Mac::ExtAddress));
    txtEntries[1].Init(kTxtRecordExtPanIdKey, Get<MeshCoP::ExtendedPanIdManager>().GetExtPanId().m8,
                       sizeof(MeshCoP::ExtendedPanId));

    txtData.Init(txtDataBuffer, sizeof(txtDataBuffer));
    SuccessOrAssert(Dns::TxtEntry::AppendEntries(txtEntries, GetArrayLength(txtEntries), txtData));

    LogInfo("Registering DNS-SD service: port:%u, txt:\"%s=%s, %s=%s\"", mUdpPort, kTxtRecordExtAddressKey,
            Get<Mac::Mac>().GetExtAddress().ToString().AsCString(), kTxtRecordExtPanIdKey,
            Get<MeshCoP::ExtendedPanIdManager>().GetExtPanId().ToString().AsCString());

    otPlatTrelRegisterService(&GetInstance(), mUdpPort, txtData.GetBytes(), static_cast<uint8_t>(txtData.GetLength()));

exit:
    return;
}

extern "C" void otPlatTrelHandleDiscoveredPeerInfo(otInstance *aInstance, const otPlatTrelPeerInfo *aInfo)
{
    Instance &instance = AsCoreType(aInstance);

    VerifyOrExit(instance.IsInitialized());
    instance.Get<Interface>().HandleDiscoveredPeerInfo(*static_cast<const Interface::Peer::Info *>(aInfo));

exit:
    return;
}

void Interface::HandleDiscoveredPeerInfo(const Peer::Info &aInfo)
{
    Peer                  *entry;
    Mac::ExtAddress        extAddress;
    MeshCoP::ExtendedPanId extPanId;
    bool                   isNew = false;

    VerifyOrExit(mInitialized && mEnabled);

    SuccessOrExit(ParsePeerInfoTxtData(aInfo, extAddress, extPanId));

    VerifyOrExit(extAddress != Get<Mac::Mac>().GetExtAddress());

    if (aInfo.IsRemoved())
    {
        entry = mPeerTable.FindMatching(extAddress);
        VerifyOrExit(entry != nullptr);
        RemovePeerEntry(*entry);
        ExitNow();
    }

    // It is a new entry or an update to an existing entry. First
    // check whether we have an existing entry that matches the same
    // socket address, and remove it if it is associated with a
    // different Extended MAC address. This ensures that we do not
    // keep stale entries in the peer table.

    entry = mPeerTable.FindMatching(aInfo.GetSockAddr());

    if ((entry != nullptr) && !entry->Matches(extAddress))
    {
        RemovePeerEntry(*entry);
        entry = nullptr;
    }

    if (entry == nullptr)
    {
        entry = mPeerTable.FindMatching(extAddress);
    }

    if (entry == nullptr)
    {
        entry = GetNewPeerEntry();
        VerifyOrExit(entry != nullptr);

        entry->SetExtAddress(extAddress);
        isNew = true;
    }

    if (!isNew)
    {
        VerifyOrExit((entry->GetExtPanId() != extPanId) || (entry->GetSockAddr() != aInfo.GetSockAddr()));
    }

    entry->SetExtPanId(extPanId);
    entry->SetSockAddr(aInfo.GetSockAddr());

    entry->Log(isNew ? "Added" : "Updated");

exit:
    return;
}

Error Interface::ParsePeerInfoTxtData(const Peer::Info       &aInfo,
                                      Mac::ExtAddress        &aExtAddress,
                                      MeshCoP::ExtendedPanId &aExtPanId) const
{
    Error                   error;
    Dns::TxtEntry           entry;
    Dns::TxtEntry::Iterator iterator;
    bool                    parsedExtAddress = false;
    bool                    parsedExtPanId   = false;

    aExtPanId.Clear();

    iterator.Init(aInfo.GetTxtData(), aInfo.GetTxtLength());

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
            VerifyOrExit(entry.mValueLength == sizeof(Mac::ExtAddress), error = kErrorParse);
            aExtAddress.Set(entry.mValue);
            parsedExtAddress = true;
        }
        else if (StringMatch(entry.mKey, kTxtRecordExtPanIdKey))
        {
            VerifyOrExit(!parsedExtPanId, error = kErrorParse);
            VerifyOrExit(entry.mValueLength == sizeof(MeshCoP::ExtendedPanId), error = kErrorParse);
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

Interface::Peer *Interface::GetNewPeerEntry(void)
{
    Peer *peerEntry;

    peerEntry = mPeerTable.PushBack();
    VerifyOrExit(peerEntry == nullptr);

    for (Peer &entry : mPeerTable)
    {
        if (entry.GetExtPanId() != Get<MeshCoP::ExtendedPanIdManager>().GetExtPanId())
        {
            ExitNow(peerEntry = &entry);
        }
    }

    for (Peer &entry : mPeerTable)
    {
        // We skip over any existing entry in neighbor table (even if the
        // entry is in invalid state).

        if (Get<NeighborTable>().FindNeighbor(entry.GetExtAddress(), Neighbor::kInStateAny) != nullptr)
        {
            continue;
        }

#if OPENTHREAD_FTD
        if (Get<NeighborTable>().FindRxOnlyNeighborRouter(entry.GetExtAddress()) != nullptr)
        {
            continue;
        }
#endif

        ExitNow(peerEntry = &entry);
    }

exit:
    return peerEntry;
}

void Interface::RemovePeerEntry(Peer &aEntry)
{
    aEntry.Log("Removing");

    // Replace the entry being removed with the last entry (if not the
    // last one already) and then pop the last entry from array.

    if (&aEntry != mPeerTable.Back())
    {
        aEntry = *mPeerTable.Back();
    }

    mPeerTable.PopBack();
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
        for (Peer &entry : mPeerTable)
        {
            if (!aIsDiscovery && (entry.GetExtPanId() != Get<MeshCoP::ExtendedPanIdManager>().GetExtPanId()))
            {
                continue;
            }

            otPlatTrelSend(&GetInstance(), aPacket.GetBuffer(), aPacket.GetLength(), &entry.mSockAddr);
        }
        break;

    case Header::kTypeUnicast:
    case Header::kTypeAck:
        peerEntry = mPeerTable.FindMatching(aPacket.GetHeader().GetDestination());
        VerifyOrExit(peerEntry != nullptr, error = kErrorAbort);
        otPlatTrelSend(&GetInstance(), aPacket.GetBuffer(), aPacket.GetLength(), &peerEntry->mSockAddr);
        break;
    }

exit:
    return error;
}

extern "C" void otPlatTrelHandleReceived(otInstance *aInstance, uint8_t *aBuffer, uint16_t aLength)
{
    Instance &instance = AsCoreType(aInstance);

    VerifyOrExit(instance.IsInitialized());
    instance.Get<Interface>().HandleReceived(aBuffer, aLength);

exit:
    return;
}

void Interface::HandleReceived(uint8_t *aBuffer, uint16_t aLength)
{
    LogDebg("HandleReceived(aLength:%u)", aLength);

    VerifyOrExit(mInitialized && mEnabled && !mFiltered);

    mRxPacket.Init(aBuffer, aLength);
    Get<Link>().ProcessReceivedPacket(mRxPacket);

exit:
    return;
}

const Interface::Peer *Interface::GetNextPeer(PeerIterator &aIterator) const
{
    const Peer *entry = mPeerTable.At(aIterator);

    if (entry != nullptr)
    {
        aIterator++;
    }

    return entry;
}

void Interface::Peer::Log(const char *aAction) const
{
    OT_UNUSED_VARIABLE(aAction);

    LogInfo("%s peer mac:%s, xpan:%s, %s", aAction, GetExtAddress().ToString().AsCString(),
            GetExtPanId().ToString().AsCString(), GetSockAddr().ToString().AsCString());
}

} // namespace Trel
} // namespace ot

#endif // #if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
