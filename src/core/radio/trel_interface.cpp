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

#include <string.h>

#include "common/as_core_type.hpp"
#include "common/code_utils.hpp"
#include "common/debug.hpp"
#include "common/instance.hpp"
#include "common/locator_getters.hpp"
#include "common/logging.hpp"
#include "common/string.hpp"
#include "net/dns_types.hpp"
#include "utils/parse_cmdline.hpp"

namespace ot {
namespace Trel {

const char Interface::kTxtRecordExtAddressKey[] = "xa";
const char Interface::kTxtRecordExtPanIdKey[]   = "xp";

Interface::Interface(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mInitialized(false)
    , mEnabled(false)
    , mFiltered(false)
    , mRegisterServiceTask(aInstance, HandleRegisterServiceTask)
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

void Interface::Enable(void)
{
    VerifyOrExit(!mEnabled);

    mEnabled = true;
    VerifyOrExit(mInitialized);

    otPlatTrelEnable(&GetInstance(), &mUdpPort);

    otLogInfoMac("Trel: Enabled interface, local port:%u", mUdpPort);
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
    otLogDebgMac("Trel: Disabled interface");

exit:
    return;
}

void Interface::HandleExtAddressChange(void)
{
    VerifyOrExit(mInitialized && mEnabled);
    otLogDebgMac("Trel: Extended Address changed, re-registering DNS-SD service");
    mRegisterServiceTask.Post();

exit:
    return;
}

void Interface::HandleExtPanIdChange(void)
{
    VerifyOrExit(mInitialized && mEnabled);
    otLogDebgMac("Trel: Extended PAN ID changed, re-registering DNS-SD service");
    mRegisterServiceTask.Post();

exit:
    return;
}

void Interface::HandleRegisterServiceTask(Tasklet &aTasklet)
{
    aTasklet.Get<Interface>().RegisterService();
}

void Interface::RegisterService(void)
{
    // TXT data consists of two entries: the length fields , the
    // "key" string, "=" char, and hex representation of the MAC or
    // Extended PAN ID values.
    static constexpr uint8_t kTxtDataSize =
        sizeof(uint8_t) + sizeof(kTxtRecordExtAddressKey) - 1 + sizeof(char) + sizeof(Mac::ExtAddress) * 2 +
        sizeof(uint8_t) + sizeof(kTxtRecordExtPanIdKey) - 1 + sizeof(char) + sizeof(Mac::ExtendedPanId) * 2;

    uint8_t              txtData[kTxtDataSize];
    uint8_t *            txtPtr = &txtData[0];
    String<kTxtDataSize> string;

    VerifyOrExit(mInitialized && mEnabled);

    string.Append("%s=", kTxtRecordExtAddressKey);
    string.AppendHexBytes(Get<Mac::Mac>().GetExtAddress().m8, sizeof(Mac::ExtAddress));

    *txtPtr++ = static_cast<uint8_t>(string.GetLength());
    memcpy(txtPtr, string.AsCString(), string.GetLength());
    txtPtr += string.GetLength();

    string.Clear();
    string.Append("%s=", kTxtRecordExtPanIdKey);
    string.AppendHexBytes(Get<Mac::Mac>().GetExtendedPanId().m8, sizeof(Mac::ExtendedPanId));

    *txtPtr++ = static_cast<uint8_t>(string.GetLength());
    memcpy(txtPtr, string.AsCString(), string.GetLength());
    txtPtr += string.GetLength();

    OT_ASSERT(txtPtr == OT_ARRAY_END(txtData));

    otLogInfoMac("Trel: Registering DNS-SD service: port:%u, txt:\"%s=%s, %s=%s\"", mUdpPort, kTxtRecordExtAddressKey,
                 Get<Mac::Mac>().GetExtAddress().ToString().AsCString(), kTxtRecordExtPanIdKey,
                 Get<Mac::Mac>().GetExtendedPanId().ToString().AsCString());

    otPlatTrelRegisterService(&GetInstance(), mUdpPort, txtData, sizeof(txtData));

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
    Peer *             entry;
    Mac::ExtAddress    extAddress;
    Mac::ExtendedPanId extPanId;
    bool               isNew = false;

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

template <uint16_t kBufferSize>
static Error ParseValueAsHexString(const Dns::TxtEntry &aTxtEntry, uint8_t (&aBuffer)[kBufferSize])
{
    // Parse the value from `Dns::TxtEntry` as a hex string and
    // populates the parsed bytes into `aBuffer`. It requires parsing of
    // the hex string to result in exactly `kBufferSize` decoded bytes.

    Error error = kErrorParse;
    char  hexString[kBufferSize * 2 + 1];

    VerifyOrExit(aTxtEntry.mValueLength < sizeof(hexString));

    memcpy(hexString, aTxtEntry.mValue, aTxtEntry.mValueLength);
    hexString[aTxtEntry.mValueLength] = '\0';

    error = Utils::CmdLineParser::ParseAsHexString(hexString, aBuffer);

exit:
    return error;
}

Error Interface::ParsePeerInfoTxtData(const Peer::Info &  aInfo,
                                      Mac::ExtAddress &   aExtAddress,
                                      Mac::ExtendedPanId &aExtPanId) const
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
        if (strcmp(entry.mKey, kTxtRecordExtAddressKey) == 0)
        {
            VerifyOrExit(!parsedExtAddress, error = kErrorParse);
            SuccessOrExit(error = ParseValueAsHexString(entry, aExtAddress.m8));
            parsedExtAddress = true;
        }
        else if (strcmp(entry.mKey, kTxtRecordExtPanIdKey) == 0)
        {
            VerifyOrExit(!parsedExtPanId, error = kErrorParse);
            SuccessOrExit(error = ParseValueAsHexString(entry, aExtPanId.m8));
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
        if (entry.GetExtPanId() != Get<Mac::Mac>().GetExtendedPanId())
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
        {
            Mac::Address macAddress;

            macAddress.SetExtended(entry.GetExtAddress());

            if (Get<NeighborTable>().FindRxOnlyNeighborRouter(macAddress) != nullptr)
            {
                continue;
            }
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
            if (!aIsDiscovery && (entry.GetExtPanId() != Get<Mac::Mac>().GetExtendedPanId()))
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
    otLogDebgMac("Trel: HandleReceived(aLength:%u)", aLength);

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

    otLogInfoMac("Trel: %s peer mac:%s, xpan:%s, %s", aAction, GetExtAddress().ToString().AsCString(),
                 GetExtPanId().ToString().AsCString(), GetSockAddr().ToString().AsCString());
}

} // namespace Trel
} // namespace ot

#endif // #if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
