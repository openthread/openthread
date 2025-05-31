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

#if OPENTHREAD_CONFIG_TREL_MANAGE_DNSSD_ENABLE
const char PeerDiscoverer::kTrelServiceType[] = "_trel._udp";
#endif

PeerDiscoverer::PeerDiscoverer(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mState(kStateStopped)
    , mServiceTask(aInstance)
#if OPENTHREAD_CONFIG_TREL_MANAGE_DNSSD_ENABLE
    , mServiceName(aInstance)
    , mBrowsing(false)
#endif
{
}

PeerDiscoverer::~PeerDiscoverer(void)
{
    mState = kStateStopped;
    Get<PeerTable>().Clear();
}

void PeerDiscoverer::Start(void)
{
    VerifyOrExit(mState == kStateStopped);

#if OPENTHREAD_CONFIG_TREL_MANAGE_DNSSD_ENABLE
    if (!Get<Dnssd>().IsReady())
    {
        mState = kStatePendingDnssd;
    }
    else
#endif
    {
        mState = kStateRunning;
        PostServiceTask();
    }

exit:
    return;
}

void PeerDiscoverer::Stop(void)
{
    VerifyOrExit(mState != kStateStopped);

    mState = kStateStopped;
    Get<PeerTable>().Clear();

#if OPENTHREAD_CONFIG_TREL_MANAGE_DNSSD_ENABLE
    UnregisterService();

    if (mBrowsing)
    {
        mBrowsing = false;
        Get<Dnssd>().StopBrowser(Browser());
    }
#endif

exit:
    return;
}

void PeerDiscoverer::NotifyPeerSocketAddressDifference(const Ip6::SockAddr &aPeerSockAddr,
                                                       const Ip6::SockAddr &aRxSockAddr)
{
    otPlatTrelNotifyPeerSocketAddressDifference(&GetInstance(), &aPeerSockAddr, &aRxSockAddr);
}

void PeerDiscoverer::PostServiceTask(void)
{
    if (IsRunning())
    {
        mServiceTask.Post();
    }
}

void PeerDiscoverer::HandleServiceTask(void)
{
    VerifyOrExit(IsRunning());

    RegisterService();

#if OPENTHREAD_CONFIG_TREL_MANAGE_DNSSD_ENABLE
    if (!mBrowsing)
    {
        mBrowsing = true;
        Get<Dnssd>().StartBrowser(Browser());
    }
#endif

exit:
    return;
}

void PeerDiscoverer::RegisterService(void)
{
    TxtDataEncoder txtData(GetInstance());
    uint16_t       port;

    port = Get<Interface>().GetUdpPort();

    txtData.Encode();

#if OPENTHREAD_CONFIG_TREL_MANAGE_DNSSD_ENABLE
    RegisterService(port, txtData);
#else
    LogInfo("Registering DNS-SD service: port:%u", port);
    otPlatTrelRegisterService(&GetInstance(), port, txtData.GetBytes(), static_cast<uint8_t>(txtData.GetLength()));
#endif
}

#if !OPENTHREAD_CONFIG_TREL_MANAGE_DNSSD_ENABLE

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
    Peer::Action  action    = Peer::kUpdated;
    bool          shouldLog = true;

    VerifyOrExit(IsRunning());

    txtData.Init(aInfo.mTxtData, aInfo.mTxtLength);
    SuccessOrExit(txtData.Decode(txtInfo));

    VerifyOrExit(txtInfo.mExtAddress != Get<Mac::Mac>().GetExtAddress());

    if (aInfo.IsRemoved())
    {
        peer = Get<PeerTable>().FindMatching(txtInfo.mExtAddress);
        VerifyOrExit(peer != nullptr);
        peer->ScheduleToRemoveAfter(kRemoveDelay);
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
        action = Peer::kAdded;
    }
    else if (!peer->IsStateValid())
    {
        action = Peer::kReAdded;
    }
    else
    {
        shouldLog = (peer->GetExtPanId() != txtInfo.mExtPanId) || (peer->GetSockAddr() != aInfo.GetSockAddr());
        action    = Peer::kUpdated;
    }

    peer->SetState(Peer::kStateValid);
    peer->SetExtPanId(txtInfo.mExtPanId);
    peer->SetSockAddr(aInfo.GetSockAddr());

    if (shouldLog)
    {
        peer->Log(action);
    }

exit:
    return;
}

#endif // !OPENTHREAD_CONFIG_TREL_MANAGE_DNSSD_ENABLE

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

#if OPENTHREAD_CONFIG_TREL_MANAGE_DNSSD_ENABLE

void PeerDiscoverer::HandleDnssdPlatformStateChange(void)
{
    if (Get<Dnssd>().IsReady())
    {
        VerifyOrExit(mState == kStatePendingDnssd);
        mState = kStateRunning;
        PostServiceTask();
    }
    else
    {
        VerifyOrExit(mState != kStateStopped);
        mState    = kStatePendingDnssd;
        mBrowsing = false;
        Get<PeerTable>().Clear();
    }

exit:
    return;
}

void PeerDiscoverer::RegisterService(uint16_t aPort, const TxtData &aTxtData)
{
    Dnssd::Service service;

    service.Clear();
    service.mServiceType     = kTrelServiceType;
    service.mServiceInstance = mServiceName.GetName();
    service.mTxtData         = aTxtData.GetBytes();
    service.mTxtDataLength   = aTxtData.GetLength();
    service.mPort            = aPort;

    LogInfo("Registering service %s.%s", service.mServiceInstance, kTrelServiceType);
    LogInfo("    port:%u, ext-addr:%s, ext-panid:%s", aPort, Get<Mac::Mac>().GetExtAddress().ToString().AsCString(),
            Get<MeshCoP::ExtendedPanIdManager>().GetExtPanId().ToString().AsCString());

    Get<Dnssd>().RegisterService(service, /* aRequestId */ 0, HandleRegisterDone);
}

void PeerDiscoverer::UnregisterService(void)
{
    Dnssd::Service service;

    service.Clear();
    service.mServiceType     = kTrelServiceType;
    service.mServiceInstance = mServiceName.GetName();

    Get<Dnssd>().UnregisterService(service, /* aRequestId */ 0, /* aCallback */ nullptr);
}

void PeerDiscoverer::HandleRegisterDone(otInstance *aInstance, Dnssd::RequestId aRequestId, otError aError)
{
    OT_UNUSED_VARIABLE(aRequestId);
    AsCoreType(aInstance).Get<PeerDiscoverer>().HandleRegisterDone(aError);
}

void PeerDiscoverer::HandleRegisterDone(Error aError)
{
    VerifyOrExit(IsRunning());

    if (aError == kErrorNone)
    {
        LogInfo("DNS-SD service registered successfully");
    }
    else
    {
        LogInfo("Failed to register DNS-SD service with name:%s, Error:%s", mServiceName.GetName(),
                ErrorToString(aError));
        UnregisterService();

        // Generate a new name (appending a suffix index to the name)
        // and try again.
        mServiceName.GenerateName();
        PostServiceTask();
    }

exit:
    return;
}

void PeerDiscoverer::HandleBrowseResult(otInstance *aInstance, const otPlatDnssdBrowseResult *aResult)
{
    AsCoreType(aInstance).Get<PeerDiscoverer>().HandleBrowseResult(*aResult);
}

void PeerDiscoverer::HandleBrowseResult(const Dnssd::BrowseResult &aResult)
{
    Peer *peer;

    VerifyOrExit(IsRunning());

    peer = Get<PeerTable>().FindMatching(Peer::ServiceNameMatcher(aResult.mServiceInstance));

    if (aResult.mTtl == 0)
    {
        // Previously discovered service is now removed.

        VerifyOrExit(peer != nullptr);
        peer->ScheduleToRemoveAfter(kRemoveDelay);
    }
    else
    {
        // A new service is discovered.

        Peer::Action action = Peer::kAdded;

        if (peer == nullptr)
        {
            peer = Get<PeerTable>().AllocateAndAddNewPeer();
            VerifyOrExit(peer != nullptr);
            SuccessOrAssert(peer->mServiceName.Set(aResult.mServiceInstance));
        }
        else
        {
            action = Peer::kReAdded;
        }

        peer->SetState(Peer::kStateResolving);
        peer->Log(action);

        StartServiceResolvers(*peer);
    }

exit:
    return;
}

void PeerDiscoverer::StartServiceResolvers(Peer &aPeer)
{
    VerifyOrExit(!aPeer.mResolvingService);

    aPeer.mResolvingService = true;
    Get<Dnssd>().StartSrvResolver(SrvResolver(aPeer));
    Get<Dnssd>().StartTxtResolver(TxtResolver(aPeer));

exit:
    return;
}

void PeerDiscoverer::StopServiceResolvers(Peer &aPeer)
{
    VerifyOrExit(aPeer.mResolvingService);

    aPeer.mResolvingService = false;
    aPeer.mTxtDataValidated = false;
    aPeer.mPort             = 0;
    aPeer.mHostName.Free();

    Get<Dnssd>().StopSrvResolver(SrvResolver(aPeer));
    Get<Dnssd>().StopTxtResolver(TxtResolver(aPeer));

exit:
    return;
}

void PeerDiscoverer::HandleSrvResult(otInstance *aInstance, const otPlatDnssdSrvResult *aResult)
{
    AsCoreType(aInstance).Get<PeerDiscoverer>().HandleSrvResult(*aResult);
}

void PeerDiscoverer::HandleSrvResult(const Dnssd::SrvResult &aResult)
{
    Peer *peer;

    VerifyOrExit(IsRunning());

    peer = Get<PeerTable>().FindMatching(Peer::ServiceNameMatcher(aResult.mServiceInstance));
    VerifyOrExit(peer != nullptr);

    if (aResult.mTtl == 0)
    {
        peer->SetPort(0);
        StopHostAddressResolver(*peer);
    }
    else
    {
        peer->SetPort(aResult.mPort);

        if (!Peer::NameMatch(peer->mHostName, aResult.mHostName))
        {
            StopHostAddressResolver(*peer);
            SuccessOrAssert(peer->mHostName.Set(aResult.mHostName));
            StartHostAddressResolver(*peer);
        }
    }

    UpdatePeerState(*peer);

exit:
    return;
}

void PeerDiscoverer::HandleTxtResult(otInstance *aInstance, const otPlatDnssdTxtResult *aResult)
{
    AsCoreType(aInstance).Get<PeerDiscoverer>().HandleTxtResult(*aResult);
}

void PeerDiscoverer::HandleTxtResult(const Dnssd::TxtResult &aResult)
{
    Peer *peer;

    VerifyOrExit(IsRunning());

    peer = Get<PeerTable>().FindMatching(Peer::ServiceNameMatcher(aResult.mServiceInstance));
    VerifyOrExit(peer != nullptr);

    ProcessPeerTxtData(aResult, *peer);

    UpdatePeerState(*peer);

exit:
    return;
}

void PeerDiscoverer::ProcessPeerTxtData(const Dnssd::TxtResult &aResult, Peer &aPeer)
{
    TxtData       txtData;
    TxtData::Info txtInfo;

    aPeer.mTxtDataValidated = false;

    VerifyOrExit(aResult.mTtl != 0);

    txtData.Init(aResult.mTxtData, aResult.mTxtDataLength);

    SuccessOrExit(txtData.Decode(txtInfo));

    if (txtInfo.mExtAddress == Get<Mac::Mac>().GetExtAddress())
    {
        LogInfo("Peer %s is this device itself", aPeer.mServiceName.AsCString());
        aPeer.ScheduleToRemoveAfter(0);
        ExitNow();
    }

    aPeer.SetExtPanId(txtInfo.mExtPanId);

    if (aPeer.GetExtAddress() != txtInfo.mExtAddress)
    {
        // Remove any peer that is associated with the same ExtAddress.
        // These are likely stale entries. This ensure we have at most
        // one entry associated with an `ExtAddress`.

        Get<PeerTable>().RemoveAndFreeAllMatching(txtInfo.mExtAddress);

        aPeer.SetExtAddress(txtInfo.mExtAddress);
    }

    aPeer.mTxtDataValidated = true;

exit:
    return;
}

void PeerDiscoverer::StartHostAddressResolver(Peer &aPeer)
{
    Peer *sameHostPeer;

    VerifyOrExit(!aPeer.mResolvingHost);

    sameHostPeer = Get<PeerTable>().FindMatching(Peer::HostNameMatcher(aPeer.mHostName.AsCString()));

    aPeer.mResolvingHost = true;

    if (sameHostPeer != nullptr)
    {
        UpdatePeerAddresses(aPeer, sameHostPeer->mHostAddresses);
    }
    else
    {
        Get<Dnssd>().StartIp6AddressResolver(AddressResolver(aPeer));
    }

exit:
    return;
}

void PeerDiscoverer::StopHostAddressResolver(Peer &aPeer)
{
    // We check if any `Peer` in the table is associated with the
    // same host name before we decide whether we stop the
    // `AddressResolver` for the given host name.

    VerifyOrExit(aPeer.mResolvingHost);

    aPeer.mResolvingHost = false;
    aPeer.mHostAddresses.Free();

    VerifyOrExit(!Get<PeerTable>().ContainsMatching(Peer::HostNameMatcher(aPeer.mHostName.AsCString())));

    Get<Dnssd>().StopIp6AddressResolver(AddressResolver(aPeer));

exit:
    return;
}

void PeerDiscoverer::HandleAddressResult(otInstance *aInstance, const otPlatDnssdAddressResult *aResult)
{
    AsCoreType(aInstance).Get<PeerDiscoverer>().HandleAddressResult(*aResult);
}

void PeerDiscoverer::HandleAddressResult(const Dnssd::AddressResult &aResult)
{
    Peer::AddressArray sortedAddresses;

    VerifyOrExit(IsRunning());

    while (true)
    {
        // Iterate through addresses in `aResult`, adding them one by
        // one to `sortedAddresses` such that more favored addresses
        // are placed at the beginning of the array.

        AddrAndTtl favored;

        favored.Clear();

        for (uint16_t index = 0; index < aResult.mAddressesLength; index++)
        {
            const Dnssd::AddressAndTtl &newAddrAndTtl = aResult.mAddresses[index];
            const Ip6::Address         &newAddr       = AsCoreType(&aResult.mAddresses[index].mAddress);
            uint32_t                    newTtl        = aResult.mAddresses[index].mTtl;

            // Skip the address if it is already in the `sortedAddresses`
            // list or if the address is invalid(e.g., zero TTL or
            // multicast). Then check if it is favored over the current
            // `favored` selection and update `favored` accordingly.

            if (sortedAddresses.Contains(newAddr))
            {
                continue;
            }

            if ((newTtl == 0) || newAddr.IsUnspecified() || newAddr.IsLoopback() || newAddr.IsMulticast())
            {
                continue;
            }

            if (favored.IsFavoredOver(newAddrAndTtl))
            {
                continue;
            }

            favored.SetFrom(newAddrAndTtl);
        }

        if (favored.IsEmpty())
        {
            break;
        }

        SuccessOrAssert(sortedAddresses.PushBack(favored.mAddress));
    }

    // Update the addresses of all `Peer`s that are associated with
    // the resolved `aResult.mHostName`.
    //
    // This handles the case where multiple TREL services may be
    // present on the same host machine. While this is unlikely in
    // actual deployments, it can be useful for testing and
    // simulation where a single machine may be acting as multiple
    // Thread nodes thus advertising multiple TREL services from the
    // same host name.

    for (Peer &peer : Get<PeerTable>())
    {
        if (peer.IsStateRemoving())
        {
            continue;
        }

        if (peer.Matches(Peer::HostNameMatcher(aResult.mHostName)))
        {
            UpdatePeerAddresses(peer, sortedAddresses);
            UpdatePeerState(peer);
        }
    }

exit:
    return;
}

void PeerDiscoverer::UpdatePeerAddresses(Peer &aPeer, const Peer::AddressArray &aSortedAddresses)
{
    // Updates `aPeer.mHostAddresses` and decides whether to update
    // `aPeer.mSockAddr`(the primary address used for communication
    // with the peer).

    bool shouldChangeSockAddr = false;

    // If the new `aSortedAddresses` is empty, clear
    // `aPeer.mHostAddresses` but leave `aPeer.mSockAddr`
    // unchanged (retaining the last known good address).

    if (aSortedAddresses.IsEmpty())
    {
        aPeer.mHostAddresses.Clear();
        ExitNow();
    }

    // Determine if `aPeer.mSockAddr` should be updated: The goal is
    // to use the most stable address, preferring one learned from a
    // received packet if mDNS still considers it valid or if mDNS
    // information is unstable.
    //
    // If `mSockAddr` was not set by a received packet, then the
    // `mSockAddr` was last set by a previous mDNS resolution. Always
    // update it with the new address `aSortedAddresses[0]`.
    //
    // If `mSockAddr` was previously set by a received packet
    // (`aPeer.mSockAddrUpdatedBasedOnRx` is true):
    //
    // - If the current `mSockAddr` is in the `aSortedAddresses` list
    //   then we keep the current `mSockAddr`. It's packet-verified
    //   and now mDNS-confirmed.
    //
    // - If the current `mSockAddr` (from rx packet) is not present in
    //   the `aSortedAddresses`, then we only update and use
    //   `aSortedAddresses[0]` if this  differs from the previous one
    //   discovered through mDNS resolution. This approach avoids
    //   changing a working, rx-verified address due to transient
    //   mDNS issues. If we see a change to the list of addresses
    //   reported through mDNS, we can be sure that an mDNS answer
    //   was indeed received and processed (which updated the list,
    //   so we know the list is most likely more recent and correct)

    if (!aPeer.mSockAddrUpdatedBasedOnRx)
    {
        shouldChangeSockAddr = true;
    }
    else if (!aSortedAddresses.Contains(aPeer.GetSockAddr().GetAddress()))
    {
        const Ip6::Address *prevFront = aPeer.mHostAddresses.Front();

        if ((prevFront == nullptr) || (*prevFront != aSortedAddresses[0]))
        {
            shouldChangeSockAddr = true;
        }
    }

    if (shouldChangeSockAddr && (aPeer.GetSockAddr().GetAddress() != aSortedAddresses[0]))
    {
        AsCoreType(&aPeer.mSockAddr).SetAddress(aSortedAddresses[0]);
        aPeer.mSockAddrUpdatedBasedOnRx = false;
    }

    aPeer.mHostAddresses.CloneFrom(aSortedAddresses);

exit:
    return;
}

void PeerDiscoverer::UpdatePeerState(Peer &aPeer)
{
    VerifyOrExit(aPeer.IsStateResolving());
    VerifyOrExit(aPeer.mResolvingService && aPeer.mResolvingHost);
    VerifyOrExit(aPeer.mTxtDataValidated);
    VerifyOrExit(aPeer.mPort != 0);
    VerifyOrExit(aPeer.mHostAddresses.GetLength() > 0);

    aPeer.SetState(Peer::kStateValid);
    aPeer.Log(Peer::kUpdated);

exit:
    return;
}

void PeerDiscoverer::HandlePeerRemoval(Peer &aPeer)
{
    // Callback from `Peer` signaling that peer is being removed
    // or scheduled to be removed. We stop any active resolvers
    // associated with this peer.
    //
    // The order of calls is important here since the
    // `StopServiceResolvers()` clears the `aPeer.mHostName` which
    // would be needed in `StopHostAddressResolver()`.

    StopHostAddressResolver(aPeer);
    StopServiceResolvers(aPeer);
}

#endif // OPENTHREAD_CONFIG_TREL_MANAGE_DNSSD_ENABLE

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

#if OPENTHREAD_CONFIG_TREL_MANAGE_DNSSD_ENABLE

//----------------------------------------------------------------------------------------------------------------------
// PeerDiscoverer::ServiceName

const char PeerDiscoverer::ServiceName::kNamePrefix[] = "otTREL";

PeerDiscoverer::ServiceName::ServiceName(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mSuffixIndex(0)
{
    ClearAllBytes(mName);
}

const char *PeerDiscoverer::ServiceName::GetName(void)
{
    if (mName[0] == kNullChar)
    {
        GenerateName();
    }

    return mName;
}

void PeerDiscoverer::ServiceName::GenerateName(void)
{
    StringWriter writer(mName, sizeof(mName));

    writer.Append("%s%s", kNamePrefix, Get<Mac::Mac>().GetExtAddress().ToString().AsCString());

    if (mSuffixIndex != 0)
    {
        writer.Append("(%u)", mSuffixIndex);
    }

    mSuffixIndex++;
}

//----------------------------------------------------------------------------------------------------------------------
// PeerDiscoverer::Browser

PeerDiscoverer::Browser::Browser(void)
{
    Clear();
    mServiceType = kTrelServiceType;
    mCallback    = PeerDiscoverer::HandleBrowseResult;
}

//----------------------------------------------------------------------------------------------------------------------
// PeerDiscoverer::SrvResolver

PeerDiscoverer::SrvResolver::SrvResolver(const Peer &aPeer)
{
    Clear();
    mServiceInstance = aPeer.mServiceName.AsCString();
    mServiceType     = kTrelServiceType;
    mCallback        = PeerDiscoverer::HandleSrvResult;
}

//----------------------------------------------------------------------------------------------------------------------
// PeerDiscoverer::TxtResolver

PeerDiscoverer::TxtResolver::TxtResolver(const Peer &aPeer)
{
    Clear();
    mServiceInstance = aPeer.mServiceName.AsCString();
    mServiceType     = kTrelServiceType;
    mCallback        = PeerDiscoverer::HandleTxtResult;
}

//----------------------------------------------------------------------------------------------------------------------
// PeerDiscoverer::AddressResolver

PeerDiscoverer::AddressResolver::AddressResolver(const Peer &aPeer)
{
    Clear();
    mHostName = aPeer.mHostName.AsCString();
    mCallback = PeerDiscoverer::HandleAddressResult;
}

//----------------------------------------------------------------------------------------------------------------------
// PeerDiscoverer::AddrAndTtl

void PeerDiscoverer::AddrAndTtl::SetFrom(const Dnssd::AddressAndTtl &aAddrAndTtl)
{
    mAddress = AsCoreType(&aAddrAndTtl.mAddress);
    mTtl     = aAddrAndTtl.mTtl;
}

bool PeerDiscoverer::AddrAndTtl::IsFavoredOver(const Dnssd::AddressAndTtl &aAddrAndTtl) const
{
    int                 compare;
    const Ip6::Address &newAddress = AsCoreType(&aAddrAndTtl.mAddress);
    uint32_t            newTtl     = aAddrAndTtl.mTtl;
    Ip6::Prefix         prefix;
    Ip6::Prefix         newPrefix;

    if (IsEmpty())
    {
        // Prefer any address when empty.
        ExitNow(compare = -1);
    }

    // Prefer a link-local address over non-link-local.

    compare = ThreeWayCompare(mAddress.IsLinkLocalUnicast(), newAddress.IsLinkLocalUnicast());
    VerifyOrExit(compare == 0);

    // Prefer non-ULA address over ULA address

    mAddress.GetPrefix(Ip6::NetworkPrefix::kLength, prefix);
    newAddress.GetPrefix(Ip6::NetworkPrefix::kLength, newPrefix);
    compare = ThreeWayCompare(!prefix.IsUniqueLocal(), !newPrefix.IsUniqueLocal());
    VerifyOrExit(compare == 0);

    // Prefer address with longer TTL.

    compare = ThreeWayCompare(mTtl, newTtl);
    VerifyOrExit(compare == 0);

    compare = (mAddress < newAddress) ? 1 : 0;

exit:
    return (compare > 0);
}

#endif // OPENTHREAD_CONFIG_TREL_MANAGE_DNSSD_ENABLE

} // namespace Trel
} // namespace ot

#endif // #if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
