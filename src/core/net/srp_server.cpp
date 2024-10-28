/*
 *  Copyright (c) 2020, The OpenThread Authors.
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

/**
 * @file
 *   This file includes implementation for SRP server.
 */

#include "srp_server.hpp"

#if OPENTHREAD_CONFIG_SRP_SERVER_ENABLE

#include "instance/instance.hpp"

namespace ot {
namespace Srp {

RegisterLogModule("SrpServer");

static const char kDefaultDomain[]       = "default.service.arpa.";
static const char kServiceSubTypeLabel[] = "._sub.";

static Dns::UpdateHeader::Response ErrorToDnsResponseCode(Error aError)
{
    Dns::UpdateHeader::Response responseCode;

    switch (aError)
    {
    case kErrorNone:
        responseCode = Dns::UpdateHeader::kResponseSuccess;
        break;
    case kErrorNoBufs:
        responseCode = Dns::UpdateHeader::kResponseServerFailure;
        break;
    case kErrorParse:
        responseCode = Dns::UpdateHeader::kResponseFormatError;
        break;
    case kErrorDuplicated:
        responseCode = Dns::UpdateHeader::kResponseNameExists;
        break;
    default:
        responseCode = Dns::UpdateHeader::kResponseRefused;
        break;
    }

    return responseCode;
}

//---------------------------------------------------------------------------------------------------------------------
// Server

Server::Server(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mSocket(aInstance, *this)
    , mLeaseTimer(aInstance)
    , mOutstandingUpdatesTimer(aInstance)
    , mCompletedUpdateTask(aInstance)
    , mServiceUpdateId(Random::NonCrypto::GetUint32())
    , mPort(kUninitializedPort)
    , mState(kStateDisabled)
    , mAddressMode(kDefaultAddressMode)
    , mAnycastSequenceNumber(0)
    , mHasRegisteredAnyService(false)
#if OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE
    , mAutoEnable(false)
#endif
{
    IgnoreError(SetDomain(kDefaultDomain));
}

Error Server::SetAddressMode(AddressMode aMode)
{
    Error error = kErrorNone;

    VerifyOrExit(mState == kStateDisabled, error = kErrorInvalidState);
    VerifyOrExit(mAddressMode != aMode);
    LogInfo("Address Mode: %s -> %s", AddressModeToString(mAddressMode), AddressModeToString(aMode));
    mAddressMode = aMode;

exit:
    return error;
}

Error Server::SetAnycastModeSequenceNumber(uint8_t aSequenceNumber)
{
    Error error = kErrorNone;

    VerifyOrExit(mState == kStateDisabled, error = kErrorInvalidState);
    mAnycastSequenceNumber = aSequenceNumber;

    LogInfo("Set Anycast Address Mode Seq Number to %d", aSequenceNumber);

exit:
    return error;
}

void Server::SetEnabled(bool aEnabled)
{
#if OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE
    mAutoEnable = false;
#endif

    if (aEnabled)
    {
        Enable();
    }
    else
    {
        Disable();
    }
}

void Server::Enable(void)
{
    VerifyOrExit(mState == kStateDisabled);
    mState = kStateStopped;

    // Request publishing of "DNS/SRP Address Service" entry in the
    // Thread Network Data based of `mAddressMode`. Then wait for
    // callback `HandleNetDataPublisherEntryChange()` from the
    // `Publisher` to start the SRP server.

    switch (mAddressMode)
    {
    case kAddressModeUnicast:
        SelectPort();
        Get<NetworkData::Publisher>().PublishDnsSrpServiceUnicast(mPort);
        break;

    case kAddressModeAnycast:
        mPort = kAnycastAddressModePort;
        Get<NetworkData::Publisher>().PublishDnsSrpServiceAnycast(mAnycastSequenceNumber);
        break;
    }

exit:
    return;
}

void Server::Disable(void)
{
    VerifyOrExit(mState != kStateDisabled);
    Get<NetworkData::Publisher>().UnpublishDnsSrpService();
    Stop();
    mState = kStateDisabled;

exit:
    return;
}

#if OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE
void Server::SetAutoEnableMode(bool aEnabled)
{
    VerifyOrExit(mAutoEnable != aEnabled);
    mAutoEnable = aEnabled;

    Get<BorderRouter::RoutingManager>().HandleSrpServerAutoEnableMode();

exit:
    return;
}
#endif

Server::TtlConfig::TtlConfig(void)
{
    mMinTtl = kDefaultMinTtl;
    mMaxTtl = kDefaultMaxTtl;
}

Error Server::SetTtlConfig(const TtlConfig &aTtlConfig)
{
    Error error = kErrorNone;

    VerifyOrExit(aTtlConfig.IsValid(), error = kErrorInvalidArgs);
    mTtlConfig = aTtlConfig;

exit:
    return error;
}

uint32_t Server::TtlConfig::GrantTtl(uint32_t aLease, uint32_t aTtl) const
{
    OT_ASSERT(mMinTtl <= mMaxTtl);

    return Clamp(Min(aTtl, aLease), mMinTtl, mMaxTtl);
}

Server::LeaseConfig::LeaseConfig(void)
{
    mMinLease    = kDefaultMinLease;
    mMaxLease    = kDefaultMaxLease;
    mMinKeyLease = kDefaultMinKeyLease;
    mMaxKeyLease = kDefaultMaxKeyLease;
}

bool Server::LeaseConfig::IsValid(void) const
{
    bool valid = false;

    // TODO: Support longer LEASE.
    // We use milliseconds timer for LEASE & KEY-LEASE, this is to avoid overflow.
    VerifyOrExit(mMaxKeyLease <= Time::MsecToSec(TimerMilli::kMaxDelay));
    VerifyOrExit(mMinLease <= mMaxLease);
    VerifyOrExit(mMinKeyLease <= mMaxKeyLease);
    VerifyOrExit(mMinLease <= mMinKeyLease);
    VerifyOrExit(mMaxLease <= mMaxKeyLease);

    valid = true;

exit:
    return valid;
}

uint32_t Server::LeaseConfig::GrantLease(uint32_t aLease) const
{
    OT_ASSERT(mMinLease <= mMaxLease);

    return (aLease == 0) ? 0 : Clamp(aLease, mMinLease, mMaxLease);
}

uint32_t Server::LeaseConfig::GrantKeyLease(uint32_t aKeyLease) const
{
    OT_ASSERT(mMinKeyLease <= mMaxKeyLease);

    return (aKeyLease == 0) ? 0 : Clamp(aKeyLease, mMinKeyLease, mMaxKeyLease);
}

Error Server::SetLeaseConfig(const LeaseConfig &aLeaseConfig)
{
    Error error = kErrorNone;

    VerifyOrExit(aLeaseConfig.IsValid(), error = kErrorInvalidArgs);
    mLeaseConfig = aLeaseConfig;

exit:
    return error;
}

Error Server::SetDomain(const char *aDomain)
{
    Error    error = kErrorNone;
    uint16_t length;

    VerifyOrExit(mState == kStateDisabled, error = kErrorInvalidState);

    length = StringLength(aDomain, Dns::Name::kMaxNameSize);
    VerifyOrExit((length > 0) && (length < Dns::Name::kMaxNameSize), error = kErrorInvalidArgs);

    if (aDomain[length - 1] == '.')
    {
        error = mDomain.Set(aDomain);
    }
    else
    {
        // Need to append dot at the end

        char buf[Dns::Name::kMaxNameSize];

        VerifyOrExit(length < Dns::Name::kMaxNameSize - 1, error = kErrorInvalidArgs);

        memcpy(buf, aDomain, length);
        buf[length]     = '.';
        buf[length + 1] = '\0';

        error = mDomain.Set(buf);
    }

exit:
    return error;
}

const Server::Host *Server::GetNextHost(const Server::Host *aHost)
{
    return (aHost == nullptr) ? mHosts.GetHead() : aHost->GetNext();
}

void Server::RemoveHost(Host *aHost, RetainName aRetainName)
{
    VerifyOrExit(aHost != nullptr);

    aHost->mLease = 0;
    aHost->ClearResources();

    if (aRetainName)
    {
        LogInfo("Remove host %s (but retain its name)", aHost->GetFullName());
    }
    else
    {
        aHost->mKeyLease = 0;
        IgnoreError(mHosts.Remove(*aHost));
        LogInfo("Fully remove host %s", aHost->GetFullName());
    }

    if (mServiceUpdateHandler.IsSet())
    {
        uint32_t updateId = AllocateServiceUpdateId();

        LogInfo("SRP update handler is notified (updatedId = %lu)", ToUlong(updateId));
        mServiceUpdateHandler.Invoke(updateId, aHost, static_cast<uint32_t>(kDefaultEventsHandlerTimeout));
        // We don't wait for the reply from the service update handler,
        // but always remove the host (and its services) regardless of
        // host/service update result. Because removing a host should fail
        // only when there is system failure of the platform mDNS implementation
        // and in which case the host is not expected to be still registered.
    }
#if OPENTHREAD_CONFIG_SRP_SERVER_ADVERTISING_PROXY_ENABLE
    else
    {
        Get<AdvertisingProxy>().AdvertiseRemovalOf(*aHost);
    }
#endif

    if (!aRetainName)
    {
        aHost->Free();
    }

exit:
    return;
}

bool Server::HasNameConflictsWith(Host &aHost) const
{
    bool        hasConflicts = false;
    const Host *existingHost = mHosts.FindMatching(aHost.GetFullName());

    if ((existingHost != nullptr) && (aHost.mKey != existingHost->mKey))
    {
        LogWarn("Name conflict: host name %s has already been allocated", aHost.GetFullName());
        ExitNow(hasConflicts = true);
    }

    for (const Service &service : aHost.mServices)
    {
        // Check on all hosts for a matching service with the same
        // instance name and if found, verify that it has the same
        // key.

        for (const Host &host : mHosts)
        {
            if (host.HasService(service.GetInstanceName()) && (aHost.mKey != host.mKey))
            {
                LogWarn("Name conflict: service name %s has already been allocated", service.GetInstanceName());
                ExitNow(hasConflicts = true);
            }
        }
    }

exit:
    return hasConflicts;
}

void Server::HandleServiceUpdateResult(ServiceUpdateId aId, Error aError)
{
    UpdateMetadata *update = mOutstandingUpdates.RemoveMatching(aId);

    if (update == nullptr)
    {
        LogInfo("Delayed SRP host update result, the SRP update has been committed (updateId = %lu)", ToUlong(aId));
        ExitNow();
    }

    update->SetError(aError);

    LogInfo("Handler result of SRP update (id = %lu) is received: %s", ToUlong(update->GetId()), ErrorToString(aError));

    // We add new `update` at the tail of the `mCompletedUpdates` list
    // so that updates are processed in the order we receive the
    // `HandleServiceUpdateResult()` callbacks for them. The
    // completed updates are processed from `mCompletedUpdateTask`
    // and `ProcessCompletedUpdates()`.

    mCompletedUpdates.PushAfterTail(*update);
    mCompletedUpdateTask.Post();

    if (mOutstandingUpdates.IsEmpty())
    {
        mOutstandingUpdatesTimer.Stop();
    }
    else
    {
        mOutstandingUpdatesTimer.FireAt(mOutstandingUpdates.GetTail()->GetExpireTime());
    }

exit:
    return;
}

void Server::ProcessCompletedUpdates(void)
{
    UpdateMetadata *update;

    while ((update = mCompletedUpdates.Pop()) != nullptr)
    {
        CommitSrpUpdate(*update);
    }
}

void Server::CommitSrpUpdate(Error aError, Host &aHost, const MessageMetadata &aMessageMetadata)
{
    CommitSrpUpdate(aError, aHost, aMessageMetadata.mDnsHeader, aMessageMetadata.mMessageInfo,
                    aMessageMetadata.mTtlConfig, aMessageMetadata.mLeaseConfig);
}

void Server::CommitSrpUpdate(UpdateMetadata &aUpdateMetadata)
{
    CommitSrpUpdate(aUpdateMetadata.GetError(), aUpdateMetadata.GetHost(), aUpdateMetadata.GetDnsHeader(),
                    aUpdateMetadata.IsDirectRxFromClient() ? &aUpdateMetadata.GetMessageInfo() : nullptr,
                    aUpdateMetadata.GetTtlConfig(), aUpdateMetadata.GetLeaseConfig());

    aUpdateMetadata.Free();
}

void Server::CommitSrpUpdate(Error                    aError,
                             Host                    &aHost,
                             const Dns::UpdateHeader &aDnsHeader,
                             const Ip6::MessageInfo  *aMessageInfo,
                             const TtlConfig         &aTtlConfig,
                             const LeaseConfig       &aLeaseConfig)
{
    Host    *existingHost    = nullptr;
    uint32_t grantedTtl      = 0;
    uint32_t hostLease       = 0;
    uint32_t hostKeyLease    = 0;
    uint32_t grantedLease    = 0;
    uint32_t grantedKeyLease = 0;
    bool     useShortLease   = aHost.ShouldUseShortLeaseOption();

    if (aError != kErrorNone || (mState != kStateRunning))
    {
        aHost.Free();
        ExitNow();
    }

    hostLease       = aHost.GetLease();
    hostKeyLease    = aHost.GetKeyLease();
    grantedLease    = aLeaseConfig.GrantLease(hostLease);
    grantedKeyLease = useShortLease ? grantedLease : aLeaseConfig.GrantKeyLease(hostKeyLease);
    grantedTtl      = aTtlConfig.GrantTtl(grantedLease, aHost.GetTtl());

    existingHost = mHosts.RemoveMatching(aHost.GetFullName());

    LogInfo("Committing update for %s host %s", (existingHost != nullptr) ? "existing" : "new", aHost.GetFullName());
    LogInfo("    Granted lease:%lu, key-lease:%lu, ttl:%lu", ToUlong(grantedLease), ToUlong(grantedKeyLease),
            ToUlong(grantedTtl));

    aHost.SetLease(grantedLease);
    aHost.SetKeyLease(grantedKeyLease);
    aHost.SetTtl(grantedTtl);

    if (grantedKeyLease == 0)
    {
        VerifyOrExit(existingHost != nullptr);
        LogInfo("Fully remove host %s", aHost.GetFullName());
        aHost.Free();
        ExitNow();
    }

    mHosts.Push(aHost);

    for (Service &service : aHost.mServices)
    {
        service.mLease       = grantedLease;
        service.mKeyLease    = grantedKeyLease;
        service.mTtl         = grantedTtl;
        service.mIsCommitted = true;

#if OT_SHOULD_LOG_AT(OT_LOG_LEVEL_INFO)
        {
            Service::Action action = Service::kAddNew;

            if (service.mIsDeleted)
            {
                action = Service::kRemoveButRetainName;
            }
            else if ((existingHost != nullptr) && existingHost->HasService(service.GetInstanceName()))
            {
                action = Service::kUpdateExisting;
            }

            service.Log(action);
        }
#endif
    }

    if (existingHost != nullptr)
    {
        // Move any existing service that is not included in the new
        // update into `aHost`.

        Service *existingService;

        while ((existingService = existingHost->mServices.Pop()) != nullptr)
        {
            if (!aHost.HasService(existingService->GetInstanceName()))
            {
                aHost.AddService(*existingService);

                // If host is deleted we make sure to add any existing
                // service that is not already included as deleted.
                // When processing an SRP update message that removes
                // a host, we construct `aHost` and add any existing
                // services that we have for the same host name as
                // deleted. However, due to asynchronous nature
                // of "update handler" (advertising proxy), by the
                // time we get the callback to commit `aHost`, there
                // may be new services added that were not included
                // when constructing `aHost`.

                if (aHost.IsDeleted() && !existingService->IsDeleted())
                {
                    existingService->mIsDeleted = true;
                    existingService->Log(Service::kRemoveButRetainName);
                }
                else
                {
                    existingService->Log(Service::kKeepUnchanged);
                }
            }
            else
            {
                existingService->Free();
            }
        }
    }

#if OPENTHREAD_CONFIG_SRP_SERVER_PORT_SWITCH_ENABLE
    if (!mHasRegisteredAnyService && (mAddressMode == kAddressModeUnicast))
    {
        Settings::SrpServerInfo info;

        mHasRegisteredAnyService = true;
        info.SetPort(GetSocket().mSockName.mPort);
        IgnoreError(Get<Settings>().Save(info));
    }
#endif

    if (!aHost.IsDeleted())
    {
        mLeaseTimer.FireAtIfEarlier(Min(aHost.GetExpireTime(), aHost.GetKeyExpireTime()));
    }

exit:
    if (aMessageInfo != nullptr)
    {
        if (aError == kErrorNone && !(grantedLease == hostLease && grantedKeyLease == hostKeyLease))
        {
            SendResponse(aDnsHeader, grantedLease, grantedKeyLease, useShortLease, *aMessageInfo);
        }
        else
        {
            SendResponse(aDnsHeader, ErrorToDnsResponseCode(aError), *aMessageInfo);
        }
    }

    if (existingHost != nullptr)
    {
        existingHost->Free();
    }
}

void Server::InitPort(void)
{
    mPort = kUdpPortMin;

#if OPENTHREAD_CONFIG_SRP_SERVER_PORT_SWITCH_ENABLE
    {
        Settings::SrpServerInfo info;

        if (Get<Settings>().Read(info) == kErrorNone)
        {
            mPort = info.GetPort();
        }
    }
#endif
}

void Server::SelectPort(void)
{
    if (mPort == kUninitializedPort)
    {
        InitPort();
    }
    ++mPort;
    if (mPort < kUdpPortMin || mPort > kUdpPortMax)
    {
        mPort = kUdpPortMin;
    }

    LogInfo("Selected port %u", mPort);
}

void Server::Start(void)
{
    Error error = kErrorNone;

    VerifyOrExit(mState == kStateStopped);

    mState = kStateRunning;
    SuccessOrExit(error = PrepareSocket());
    LogInfo("Start listening on port %u", mPort);

#if OPENTHREAD_CONFIG_SRP_SERVER_ADVERTISING_PROXY_ENABLE
    Get<AdvertisingProxy>().HandleServerStateChange();
#endif

exit:
    // Re-enable server to select a new port.
    if (error != kErrorNone)
    {
        Disable();
        Enable();
    }
}

Error Server::PrepareSocket(void)
{
    Error error = kErrorNone;

#if OPENTHREAD_CONFIG_DNSSD_SERVER_ENABLE
    Ip6::Udp::Socket &dnsSocket = Get<Dns::ServiceDiscovery::Server>().mSocket;

    if (dnsSocket.GetSockName().GetPort() == mPort)
    {
        // If the DNS-SD socket matches our port number, we use the
        // same socket so we close our own socket (in case it was
        // open). `GetSocket()` will now return the DNS-SD socket.

        IgnoreError(mSocket.Close());
        ExitNow();
    }
#endif

    VerifyOrExit(!mSocket.IsOpen());
    SuccessOrExit(error = mSocket.Open());
    error = mSocket.Bind(mPort, Ip6::kNetifThread);

exit:
    if (error != kErrorNone)
    {
        LogWarnOnError(error, "prepare socket");
        IgnoreError(mSocket.Close());
        Stop();
    }

    return error;
}

Ip6::Udp::Socket &Server::GetSocket(void)
{
    Ip6::Udp::Socket *socket = &mSocket;

#if OPENTHREAD_CONFIG_DNSSD_SERVER_ENABLE
    Ip6::Udp::Socket &dnsSocket = Get<Dns::ServiceDiscovery::Server>().mSocket;

    if (dnsSocket.GetSockName().GetPort() == mPort)
    {
        socket = &dnsSocket;
    }
#endif

    return *socket;
}

#if OPENTHREAD_CONFIG_DNSSD_SERVER_ENABLE

void Server::HandleDnssdServerStateChange(void)
{
    // This is called from` Dns::ServiceDiscovery::Server` to notify
    // that it has started or stopped. We check whether we need to
    // share the socket.

    if (mState == kStateRunning)
    {
        IgnoreError(PrepareSocket());
    }
}

Error Server::HandleDnssdServerUdpReceive(Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    // This is called from` Dns::ServiceDiscovery::Server` when a UDP
    // message is received on its socket. We check whether we are
    // sharing socket and if so we process the received message. We
    // return `kErrorNone` to indicate that message was successfully
    // processed by `Srp::Server`, otherwise `kErrorDrop` is returned.

    Error error = kErrorDrop;

    VerifyOrExit((mState == kStateRunning) && !mSocket.IsOpen());

    error = ProcessMessage(aMessage, aMessageInfo);

exit:
    return error;
}

#endif // OPENTHREAD_CONFIG_DNSSD_SERVER_ENABLE

void Server::Stop(void)
{
    VerifyOrExit(mState == kStateRunning);

    mState = kStateStopped;

    while (!mHosts.IsEmpty())
    {
        RemoveHost(mHosts.GetHead(), kDeleteName);
    }

    // TODO: We should cancel any outstanding service updates, but current
    // OTBR mDNS publisher cannot properly handle it.
    while (!mOutstandingUpdates.IsEmpty())
    {
        mOutstandingUpdates.Pop()->Free();
    }

    mLeaseTimer.Stop();
    mOutstandingUpdatesTimer.Stop();

    LogInfo("Stop listening on %u", mPort);
    IgnoreError(mSocket.Close());
    mHasRegisteredAnyService = false;

#if OPENTHREAD_CONFIG_SRP_SERVER_ADVERTISING_PROXY_ENABLE
    Get<AdvertisingProxy>().HandleServerStateChange();
#endif

exit:
    return;
}

void Server::HandleNetDataPublisherEvent(NetworkData::Publisher::Event aEvent)
{
    switch (aEvent)
    {
    case NetworkData::Publisher::kEventEntryAdded:
        Start();
        break;

    case NetworkData::Publisher::kEventEntryRemoved:
        Stop();
        break;
    }
}

const Server::UpdateMetadata *Server::FindOutstandingUpdate(const MessageMetadata &aMessageMetadata) const
{
    const UpdateMetadata *ret = nullptr;

    VerifyOrExit(aMessageMetadata.IsDirectRxFromClient());

    for (const UpdateMetadata &update : mOutstandingUpdates)
    {
        if (aMessageMetadata.mDnsHeader.GetMessageId() == update.GetDnsHeader().GetMessageId() &&
            aMessageMetadata.mMessageInfo->GetPeerAddr() == update.GetMessageInfo().GetPeerAddr() &&
            aMessageMetadata.mMessageInfo->GetPeerPort() == update.GetMessageInfo().GetPeerPort())
        {
            ExitNow(ret = &update);
        }
    }

exit:
    return ret;
}

void Server::ProcessDnsUpdate(Message &aMessage, MessageMetadata &aMetadata)
{
    Error error = kErrorNone;
    Host *host  = nullptr;

    LogInfo("Received DNS update from %s", aMetadata.IsDirectRxFromClient()
                                               ? aMetadata.mMessageInfo->GetPeerAddr().ToString().AsCString()
                                               : "an SRPL Partner");

    SuccessOrExit(error = ProcessZoneSection(aMessage, aMetadata));

    if (FindOutstandingUpdate(aMetadata) != nullptr)
    {
        LogInfo("Drop duplicated SRP update request: MessageId=%u", aMetadata.mDnsHeader.GetMessageId());

        // Silently drop duplicate requests.
        // This could rarely happen, because the outstanding SRP update timer should
        // be shorter than the SRP update retransmission timer.
        ExitNow(error = kErrorNone);
    }

    // Per 2.3.2 of SRP draft 6, no prerequisites should be included in a SRP update.
    VerifyOrExit(aMetadata.mDnsHeader.GetPrerequisiteRecordCount() == 0, error = kErrorFailed);

    host = Host::Allocate(GetInstance(), aMetadata.mRxTime);
    VerifyOrExit(host != nullptr, error = kErrorNoBufs);
    SuccessOrExit(error = ProcessUpdateSection(*host, aMessage, aMetadata));

    // Parse lease time and validate signature.
    SuccessOrExit(error = ProcessAdditionalSection(host, aMessage, aMetadata));

#if OPENTHREAD_FTD
    if (aMetadata.IsDirectRxFromClient())
    {
        UpdateAddrResolverCacheTable(*aMetadata.mMessageInfo, *host);
    }
#endif

    HandleUpdate(*host, aMetadata);

exit:
    if (error != kErrorNone)
    {
        if (host != nullptr)
        {
            host->Free();
        }

        if (aMetadata.IsDirectRxFromClient())
        {
            SendResponse(aMetadata.mDnsHeader, ErrorToDnsResponseCode(error), *aMetadata.mMessageInfo);
        }
    }
}

Error Server::ProcessZoneSection(const Message &aMessage, MessageMetadata &aMetadata) const
{
    Error             error = kErrorNone;
    Dns::Name::Buffer name;
    uint16_t          offset = aMetadata.mOffset;

    VerifyOrExit(aMetadata.mDnsHeader.GetZoneRecordCount() == 1, error = kErrorParse);

    SuccessOrExit(error = Dns::Name::ReadName(aMessage, offset, name));
    // TODO: return `Dns::kResponseNotAuth` for not authorized zone names.
    VerifyOrExit(StringMatch(name, GetDomain(), kStringCaseInsensitiveMatch), error = kErrorSecurity);
    SuccessOrExit(error = aMessage.Read(offset, aMetadata.mDnsZone));
    offset += sizeof(Dns::Zone);

    VerifyOrExit(aMetadata.mDnsZone.GetType() == Dns::ResourceRecord::kTypeSoa, error = kErrorParse);
    aMetadata.mOffset = offset;

exit:
    LogWarnOnError(error, "process DNS Zone section");
    return error;
}

Error Server::ProcessUpdateSection(Host &aHost, const Message &aMessage, MessageMetadata &aMetadata) const
{
    Error error = kErrorNone;

    // Process Service Discovery, Host and Service Description Instructions with
    // 3 times iterations over all DNS update RRs. The order of those processes matters.

    // 0. Enumerate over all Service Discovery Instructions before processing any other records.
    // So that we will know whether a name is a hostname or service instance name when processing
    // a "Delete All RRsets from a name" record.
    SuccessOrExit(error = ProcessServiceDiscoveryInstructions(aHost, aMessage, aMetadata));

    // 1. Enumerate over all RRs to build the Host Description Instruction.
    SuccessOrExit(error = ProcessHostDescriptionInstruction(aHost, aMessage, aMetadata));

    // 2. Enumerate over all RRs to build the Service Description Instructions.
    SuccessOrExit(error = ProcessServiceDescriptionInstructions(aHost, aMessage, aMetadata));

    // 3. Verify that there are no name conflicts.
    VerifyOrExit(!HasNameConflictsWith(aHost), error = kErrorDuplicated);

exit:
    LogWarnOnError(error, "Process DNS Update section");
    return error;
}

Error Server::ProcessHostDescriptionInstruction(Host                  &aHost,
                                                const Message         &aMessage,
                                                const MessageMetadata &aMetadata) const
{
    Error    error  = kErrorNone;
    uint16_t offset = aMetadata.mOffset;

    OT_ASSERT(aHost.GetFullName() == nullptr);

    for (uint16_t numRecords = aMetadata.mDnsHeader.GetUpdateRecordCount(); numRecords > 0; numRecords--)
    {
        Dns::Name::Buffer   name;
        Dns::ResourceRecord record;

        SuccessOrExit(error = Dns::Name::ReadName(aMessage, offset, name));

        SuccessOrExit(error = aMessage.Read(offset, record));

        if (record.GetClass() == Dns::ResourceRecord::kClassAny)
        {
            // Delete All RRsets from a name.
            VerifyOrExit(IsValidDeleteAllRecord(record), error = kErrorFailed);

            // A "Delete All RRsets from a name" RR can only apply to a Service or Host Description.

            if (!aHost.HasService(name))
            {
                // If host name is already set to a different name, `SetFullName()`
                // will return `kErrorFailed`.
                SuccessOrExit(error = aHost.SetFullName(name));
                aHost.ClearResources();
            }
        }
        else if (record.GetType() == Dns::ResourceRecord::kTypeAaaa)
        {
            Dns::AaaaRecord aaaaRecord;

            VerifyOrExit(record.GetClass() == aMetadata.mDnsZone.GetClass(), error = kErrorFailed);

            SuccessOrExit(error = aHost.ProcessTtl(record.GetTtl()));

            SuccessOrExit(error = aHost.SetFullName(name));

            SuccessOrExit(error = aMessage.Read(offset, aaaaRecord));
            VerifyOrExit(aaaaRecord.IsValid(), error = kErrorParse);

            // Tolerate kErrorDrop for AAAA Resources.
            VerifyOrExit(aHost.AddIp6Address(aaaaRecord.GetAddress()) != kErrorNoBufs, error = kErrorNoBufs);
        }
        else if (record.GetType() == Dns::ResourceRecord::kTypeKey)
        {
            // We currently support only ECDSA P-256.
            Dns::Ecdsa256KeyRecord keyRecord;

            VerifyOrExit(record.GetClass() == aMetadata.mDnsZone.GetClass(), error = kErrorFailed);

            SuccessOrExit(error = aHost.ProcessTtl(record.GetTtl()));

            SuccessOrExit(error = aMessage.Read(offset, keyRecord));
            VerifyOrExit(keyRecord.IsValid(), error = kErrorParse);

            if (aHost.mParsedKey)
            {
                VerifyOrExit(aHost.mKey == keyRecord.GetKey(), error = kErrorSecurity);
            }
            else
            {
                aHost.mParsedKey = true;
                aHost.mKey       = keyRecord.GetKey();
            }
        }

        offset += record.GetSize();
    }

    // Verify that we have a complete Host Description Instruction.

    VerifyOrExit(aHost.GetFullName() != nullptr, error = kErrorFailed);
    VerifyOrExit(aHost.mParsedKey, error = kErrorFailed);

    // We check the number of host addresses after processing of the
    // Lease Option in the Addition Section and determining whether
    // the host is being removed or registered.

exit:
    LogWarnOnError(error, "process Host Description instructions");
    return error;
}

Error Server::ProcessServiceDiscoveryInstructions(Host                  &aHost,
                                                  const Message         &aMessage,
                                                  const MessageMetadata &aMetadata) const
{
    Error    error  = kErrorNone;
    uint16_t offset = aMetadata.mOffset;

    for (uint16_t numRecords = aMetadata.mDnsHeader.GetUpdateRecordCount(); numRecords > 0; numRecords--)
    {
        Dns::Name::Buffer               serviceName;
        Dns::Name::LabelBuffer          instanceLabel;
        Dns::Name::Buffer               instanceServiceName;
        String<Dns::Name::kMaxNameSize> instanceName;
        Dns::PtrRecord                  ptrRecord;
        const char                     *subServiceName;
        Service                        *service;
        bool                            isSubType;
        bool                            isDelete;

        SuccessOrExit(error = Dns::Name::ReadName(aMessage, offset, serviceName));
        VerifyOrExit(Dns::Name::IsSubDomainOf(serviceName, GetDomain()), error = kErrorSecurity);

        error = Dns::ResourceRecord::ReadRecord(aMessage, offset, ptrRecord);

        if (error == kErrorNotFound)
        {
            // `ReadRecord()` updates `aOffset` to skip over a
            // non-matching record.
            error = kErrorNone;
            continue;
        }

        SuccessOrExit(error);

        SuccessOrExit(error = ptrRecord.ReadPtrName(aMessage, offset, instanceLabel, instanceServiceName));
        instanceName.Append("%s.%s", instanceLabel, instanceServiceName);

        // Class None indicates "Delete an RR from an RRset".
        isDelete = (ptrRecord.GetClass() == Dns::ResourceRecord::kClassNone);

        VerifyOrExit(isDelete || ptrRecord.GetClass() == aMetadata.mDnsZone.GetClass(), error = kErrorParse);

        // Check if the `serviceName` is a sub-type with name
        // format: "<sub-label>._sub.<service-labels>.<domain>."

        subServiceName = StringFind(serviceName, kServiceSubTypeLabel, kStringCaseInsensitiveMatch);
        isSubType      = (subServiceName != nullptr);

        if (isSubType)
        {
            // Skip over the "._sub." label to get to the base
            // service name.
            subServiceName += sizeof(kServiceSubTypeLabel) - 1;
        }

        // Verify that instance name and service name are related.
        VerifyOrExit(Dns::Name::IsSubDomainOf(instanceName.AsCString(), isSubType ? subServiceName : serviceName),
                     error = kErrorFailed);

        // Find a matching existing service or allocate a new one.

        service = aHost.FindService(instanceName.AsCString());

        if (service == nullptr)
        {
            service = aHost.AddNewService(instanceName.AsCString(), instanceLabel, aMetadata.mRxTime);
            VerifyOrExit(service != nullptr, error = kErrorNoBufs);
        }

        if (isSubType)
        {
            VerifyOrExit(!service->HasSubTypeServiceName(serviceName), error = kErrorFailed);

            // Ignore a sub-type service delete.

            if (!isDelete)
            {
                Heap::String *newSubTypeLabel = service->mSubTypes.PushBack();

                VerifyOrExit(newSubTypeLabel != nullptr, error = kErrorNoBufs);
                SuccessOrExit(error = newSubTypeLabel->Set(serviceName));
            }
        }
        else
        {
            // Processed PTR record is the base service (not a
            // sub-type). `mServiceName` is only set when base
            // service is processed.

            VerifyOrExit(service->mServiceName.IsNull(), error = kErrorFailed);
            SuccessOrExit(error = service->mServiceName.Set(serviceName));
            service->mIsDeleted = isDelete;
        }

        if (!isDelete)
        {
            SuccessOrExit(error = aHost.ProcessTtl(ptrRecord.GetTtl()));
        }
    }

    // Verify that for all services, a PTR record was processed for
    // the base service (`mServiceName` is set), and for a deleted
    // service, no PTR record was seen adding a sub-type.

    for (const Service &service : aHost.mServices)
    {
        VerifyOrExit(!service.mServiceName.IsNull(), error = kErrorParse);

        if (service.mIsDeleted)
        {
            VerifyOrExit(service.mSubTypes.GetLength() == 0, error = kErrorParse);
        }
    }

exit:
    LogWarnOnError(error, "process Service Discovery instructions");
    return error;
}

Error Server::ProcessServiceDescriptionInstructions(Host            &aHost,
                                                    const Message   &aMessage,
                                                    MessageMetadata &aMetadata) const
{
    Error    error  = kErrorNone;
    uint16_t offset = aMetadata.mOffset;

    for (uint16_t numRecords = aMetadata.mDnsHeader.GetUpdateRecordCount(); numRecords > 0; numRecords--)
    {
        Dns::Name::Buffer   name;
        Dns::ResourceRecord record;
        Service            *service;

        SuccessOrExit(error = Dns::Name::ReadName(aMessage, offset, name));
        SuccessOrExit(error = aMessage.Read(offset, record));

        if (record.GetClass() == Dns::ResourceRecord::kClassAny)
        {
            // Delete All RRsets from a name.
            VerifyOrExit(IsValidDeleteAllRecord(record), error = kErrorFailed);

            service = aHost.FindService(name);

            if (service != nullptr)
            {
                VerifyOrExit(!service->mParsedDeleteAllRrset);
                service->mParsedDeleteAllRrset = true;
            }

            offset += record.GetSize();
            continue;
        }

        if (record.GetType() == Dns::ResourceRecord::kTypeSrv)
        {
            Dns::SrvRecord    srvRecord;
            Dns::Name::Buffer hostName;

            VerifyOrExit(record.GetClass() == aMetadata.mDnsZone.GetClass(), error = kErrorFailed);

            SuccessOrExit(error = aHost.ProcessTtl(record.GetTtl()));

            SuccessOrExit(error = aMessage.Read(offset, srvRecord));
            offset += sizeof(srvRecord);

            SuccessOrExit(error = Dns::Name::ReadName(aMessage, offset, hostName));
            VerifyOrExit(Dns::Name::IsSubDomainOf(name, GetDomain()), error = kErrorSecurity);
            VerifyOrExit(aHost.Matches(hostName), error = kErrorFailed);

            service = aHost.FindService(name);
            VerifyOrExit(service != nullptr, error = kErrorFailed);

            VerifyOrExit(!service->mParsedSrv, error = kErrorParse);
            service->mParsedSrv = true;

            service->mTtl      = srvRecord.GetTtl();
            service->mPriority = srvRecord.GetPriority();
            service->mWeight   = srvRecord.GetWeight();
            service->mPort     = srvRecord.GetPort();
        }
        else if (record.GetType() == Dns::ResourceRecord::kTypeTxt)
        {
            VerifyOrExit(record.GetClass() == aMetadata.mDnsZone.GetClass(), error = kErrorFailed);

            SuccessOrExit(error = aHost.ProcessTtl(record.GetTtl()));

            service = aHost.FindService(name);
            VerifyOrExit(service != nullptr, error = kErrorFailed);

            service->mParsedTxt = true;

            offset += sizeof(record);
            SuccessOrExit(error = service->SetTxtDataFromMessage(aMessage, offset, record.GetLength()));
            offset += record.GetLength();
        }
        else
        {
            offset += record.GetSize();
        }
    }

    for (const Service &service : aHost.mServices)
    {
        VerifyOrExit(service.mParsedDeleteAllRrset, error = kErrorFailed);
        VerifyOrExit(service.mParsedSrv == service.mParsedTxt, error = kErrorFailed);

        if (!service.mIsDeleted)
        {
            VerifyOrExit(service.mParsedSrv, error = kErrorFailed);
        }
    }

    aMetadata.mOffset = offset;

exit:
    LogWarnOnError(error, "process Service Description instructions");
    return error;
}

bool Server::IsValidDeleteAllRecord(const Dns::ResourceRecord &aRecord)
{
    return aRecord.GetClass() == Dns::ResourceRecord::kClassAny && aRecord.GetType() == Dns::ResourceRecord::kTypeAny &&
           aRecord.GetTtl() == 0 && aRecord.GetLength() == 0;
}

Error Server::ProcessAdditionalSection(Host *aHost, const Message &aMessage, MessageMetadata &aMetadata) const
{
    Error             error = kErrorNone;
    Dns::OptRecord    optRecord;
    Dns::LeaseOption  leaseOption;
    Dns::SigRecord    sigRecord;
    char              name[2]; // The root domain name (".") is expected.
    uint16_t          offset = aMetadata.mOffset;
    uint16_t          sigOffset;
    uint16_t          sigRdataOffset;
    Dns::Name::Buffer signerName;
    uint16_t          signatureLength;

    VerifyOrExit(aMetadata.mDnsHeader.GetAdditionalRecordCount() == 2, error = kErrorFailed);

    // EDNS(0) Update Lease Option.

    SuccessOrExit(error = Dns::Name::ReadName(aMessage, offset, name));
    SuccessOrExit(error = aMessage.Read(offset, optRecord));

    SuccessOrExit(error = leaseOption.ReadFrom(aMessage, offset + sizeof(optRecord), optRecord.GetLength()));

    offset += optRecord.GetSize();

    aHost->SetLease(leaseOption.GetLeaseInterval());
    aHost->SetKeyLease(leaseOption.GetKeyLeaseInterval());

    for (Service &service : aHost->mServices)
    {
        if (aHost->GetLease() == 0)
        {
            service.mIsDeleted = true;
        }

        service.mLease    = service.mIsDeleted ? 0 : leaseOption.GetLeaseInterval();
        service.mKeyLease = leaseOption.GetKeyLeaseInterval();
    }

    // If the client included the short variant of Lease Option,
    // server must also use the short variant in its response.
    aHost->SetUseShortLeaseOption(leaseOption.IsShortVariant());

    if (aHost->GetLease() > 0)
    {
        // There MUST be at least one valid address if we have nonzero lease.
        VerifyOrExit(aHost->mAddresses.GetLength() > 0, error = kErrorFailed);
    }

    // SIG(0).

    sigOffset = offset;
    SuccessOrExit(error = Dns::Name::ReadName(aMessage, offset, name));
    SuccessOrExit(error = aMessage.Read(offset, sigRecord));
    VerifyOrExit(sigRecord.IsValid(), error = kErrorParse);

    sigRdataOffset = offset + sizeof(Dns::ResourceRecord);
    offset += sizeof(sigRecord);

    // TODO: Verify that the signature doesn't expire. This is not
    // implemented because the end device may not be able to get
    // the synchronized date/time.

    SuccessOrExit(error = Dns::Name::ReadName(aMessage, offset, signerName));

    signatureLength = sigRecord.GetLength() - (offset - sigRdataOffset);
    offset += signatureLength;

    // Verify the signature. Currently supports only ECDSA.

    VerifyOrExit(sigRecord.GetAlgorithm() == Dns::KeyRecord::kAlgorithmEcdsaP256Sha256, error = kErrorFailed);
    VerifyOrExit(sigRecord.GetTypeCovered() == 0, error = kErrorFailed);
    VerifyOrExit(signatureLength == Crypto::Ecdsa::P256::Signature::kSize, error = kErrorParse);

    SuccessOrExit(error = VerifySignature(aHost->mKey, aMessage, aMetadata.mDnsHeader, sigOffset, sigRdataOffset,
                                          sigRecord.GetLength(), signerName));

    aMetadata.mOffset = offset;

exit:
    LogWarnOnError(error, "process DNS Additional section");
    return error;
}

Error Server::VerifySignature(const Host::Key  &aKey,
                              const Message    &aMessage,
                              Dns::UpdateHeader aDnsHeader,
                              uint16_t          aSigOffset,
                              uint16_t          aSigRdataOffset,
                              uint16_t          aSigRdataLength,
                              const char       *aSignerName) const
{
    Error                          error;
    uint16_t                       offset = aMessage.GetOffset();
    uint16_t                       signatureOffset;
    Crypto::Sha256                 sha256;
    Crypto::Sha256::Hash           hash;
    Crypto::Ecdsa::P256::Signature signature;
    Message                       *signerNameMessage = nullptr;

    VerifyOrExit(aSigRdataLength >= Crypto::Ecdsa::P256::Signature::kSize, error = kErrorInvalidArgs);

    sha256.Start();

    // SIG RDATA less signature.
    sha256.Update(aMessage, aSigRdataOffset, sizeof(Dns::SigRecord) - sizeof(Dns::ResourceRecord));

    // The uncompressed (canonical) form of the signer name should be used for signature
    // verification. See https://tools.ietf.org/html/rfc2931#section-3.1 for details.
    signerNameMessage = Get<Ip6::Udp>().NewMessage();
    VerifyOrExit(signerNameMessage != nullptr, error = kErrorNoBufs);
    SuccessOrExit(error = Dns::Name::AppendName(aSignerName, *signerNameMessage));
    sha256.Update(*signerNameMessage, signerNameMessage->GetOffset(), signerNameMessage->GetLength());

    // We need the DNS header before appending the SIG RR.
    aDnsHeader.SetAdditionalRecordCount(aDnsHeader.GetAdditionalRecordCount() - 1);
    sha256.Update(aDnsHeader);
    sha256.Update(aMessage, offset + sizeof(aDnsHeader), aSigOffset - offset - sizeof(aDnsHeader));

    sha256.Finish(hash);

    signatureOffset = aSigRdataOffset + aSigRdataLength - Crypto::Ecdsa::P256::Signature::kSize;
    SuccessOrExit(error = aMessage.Read(signatureOffset, signature));

    error = aKey.Verify(hash, signature);

exit:
    LogWarnOnError(error, "verify message signature");
    FreeMessage(signerNameMessage);
    return error;
}

void Server::HandleUpdate(Host &aHost, const MessageMetadata &aMetadata)
{
    Error error = kErrorNone;
    Host *existingHost;

    // Check whether the SRP update wants to remove `aHost`.

    VerifyOrExit(aHost.GetLease() == 0);

    aHost.ClearResources();

    existingHost = mHosts.FindMatching(aHost.GetFullName());
    VerifyOrExit(existingHost != nullptr);

    // The client may not include all services it has registered before
    // when removing a host. We copy and append any missing services to
    // `aHost` from the `existingHost` and mark them as deleted.

    for (const Service &existingService : existingHost->mServices)
    {
        Service *service;

        if (existingService.mIsDeleted || aHost.HasService(existingService.GetInstanceName()))
        {
            continue;
        }

        service = aHost.AddNewService(existingService.GetInstanceName(), existingService.GetInstanceLabel(),
                                      aMetadata.mRxTime);
        VerifyOrExit(service != nullptr, error = kErrorNoBufs);

        SuccessOrExit(error = service->mServiceName.Set(existingService.GetServiceName()));
        service->mIsDeleted = true;
        service->mKeyLease  = existingService.mKeyLease;
    }

exit:
    InformUpdateHandlerOrCommit(error, aHost, aMetadata);
}

void Server::InformUpdateHandlerOrCommit(Error aError, Host &aHost, const MessageMetadata &aMetadata)
{
#if OT_SHOULD_LOG_AT(OT_LOG_LEVEL_INFO)
    if (aError == kErrorNone)
    {
        uint8_t             numAddrs;
        const Ip6::Address *addrs;

        LogInfo("Processed SRP update info");
        LogInfo("    Host:%s", aHost.GetFullName());
        LogInfo("    Lease:%lu, key-lease:%lu, ttl:%lu", ToUlong(aHost.GetLease()), ToUlong(aHost.GetKeyLease()),
                ToUlong(aHost.GetTtl()));

        addrs = aHost.GetAddresses(numAddrs);

        if (numAddrs == 0)
        {
            LogInfo("    No host address");
        }
        else
        {
            LogInfo("    %d host address(es):", numAddrs);

            for (; numAddrs > 0; addrs++, numAddrs--)
            {
                LogInfo("      %s", addrs->ToString().AsCString());
            }
        }

        for (const Service &service : aHost.mServices)
        {
            LogInfo("    %s service '%s'", service.IsDeleted() ? "Deleting" : "Adding", service.GetInstanceName());

            for (const Heap::String &subType : service.mSubTypes)
            {
                Dns::Name::LabelBuffer label;

                IgnoreError(Service::ParseSubTypeServiceName(subType.AsCString(), label));
                LogInfo("      sub-type: %s", label);
            }
        }
    }
    else
    {
        LogInfo("Error %s processing received SRP update", ErrorToString(aError));
    }
#endif // OT_SHOULD_LOG_AT(OT_LOG_LEVEL_INFO)

    if ((aError == kErrorNone) && mServiceUpdateHandler.IsSet())
    {
        UpdateMetadata *update = UpdateMetadata::Allocate(GetInstance(), aHost, aMetadata);

        if (update != nullptr)
        {
            mOutstandingUpdates.Push(*update);
            mOutstandingUpdatesTimer.FireAtIfEarlier(update->GetExpireTime());

            LogInfo("SRP update handler is notified (updatedId = %lu)", ToUlong(update->GetId()));
            mServiceUpdateHandler.Invoke(update->GetId(), &aHost, static_cast<uint32_t>(kDefaultEventsHandlerTimeout));
            ExitNow();
        }

        aError = kErrorNoBufs;
    }
#if OPENTHREAD_CONFIG_SRP_SERVER_ADVERTISING_PROXY_ENABLE
    else if (aError == kErrorNone)
    {
        // Ask `AdvertisingProxy` to advertise the new update.
        // Proxy will report the outcome by calling
        // `Server::CommitSrpUpdate()` directly.

        Get<AdvertisingProxy>().Advertise(aHost, aMetadata);
        ExitNow();
    }
#endif

    CommitSrpUpdate(aError, aHost, aMetadata);

exit:
    return;
}

void Server::SendResponse(const Dns::UpdateHeader    &aHeader,
                          Dns::UpdateHeader::Response aResponseCode,
                          const Ip6::MessageInfo     &aMessageInfo)
{
    Error             error    = kErrorNone;
    Message          *response = nullptr;
    Dns::UpdateHeader header;

    VerifyOrExit(mState == kStateRunning, error = kErrorInvalidState);

    response = GetSocket().NewMessage();
    VerifyOrExit(response != nullptr, error = kErrorNoBufs);

    header.SetMessageId(aHeader.GetMessageId());
    header.SetType(Dns::UpdateHeader::kTypeResponse);
    header.SetQueryType(aHeader.GetQueryType());
    header.SetResponseCode(aResponseCode);
    SuccessOrExit(error = response->Append(header));

    SuccessOrExit(error = GetSocket().SendTo(*response, aMessageInfo));

    if (aResponseCode != Dns::UpdateHeader::kResponseSuccess)
    {
        LogWarn("Send fail response: %d", aResponseCode);
    }
    else
    {
        LogInfo("Send success response");
    }

    UpdateResponseCounters(aResponseCode);

exit:
    LogWarnOnError(error, "send response");
    FreeMessageOnError(response, error);
}

void Server::SendResponse(const Dns::UpdateHeader &aHeader,
                          uint32_t                 aLease,
                          uint32_t                 aKeyLease,
                          bool                     mUseShortLeaseOption,
                          const Ip6::MessageInfo  &aMessageInfo)
{
    Error             error;
    Message          *response = nullptr;
    Dns::UpdateHeader header;
    Dns::OptRecord    optRecord;
    Dns::LeaseOption  leaseOption;
    uint16_t          optionSize;

    response = GetSocket().NewMessage();
    VerifyOrExit(response != nullptr, error = kErrorNoBufs);

    header.SetMessageId(aHeader.GetMessageId());
    header.SetType(Dns::UpdateHeader::kTypeResponse);
    header.SetQueryType(aHeader.GetQueryType());
    header.SetResponseCode(Dns::UpdateHeader::kResponseSuccess);
    header.SetAdditionalRecordCount(1);
    SuccessOrExit(error = response->Append(header));

    // Append the root domain (".").
    SuccessOrExit(error = Dns::Name::AppendTerminator(*response));

    optRecord.Init();
    optRecord.SetUdpPayloadSize(kUdpPayloadSize);
    optRecord.SetDnsSecurityFlag();

    if (mUseShortLeaseOption)
    {
        leaseOption.InitAsShortVariant(aLease);
    }
    else
    {
        leaseOption.InitAsLongVariant(aLease, aKeyLease);
    }

    optionSize = static_cast<uint16_t>(leaseOption.GetSize());
    optRecord.SetLength(optionSize);

    SuccessOrExit(error = response->Append(optRecord));
    SuccessOrExit(error = response->AppendBytes(&leaseOption, optionSize));

    SuccessOrExit(error = GetSocket().SendTo(*response, aMessageInfo));

    LogInfo("Send success response with granted lease: %lu and key lease: %lu", ToUlong(aLease), ToUlong(aKeyLease));

    UpdateResponseCounters(Dns::UpdateHeader::kResponseSuccess);

exit:
    LogWarnOnError(error, "send response");
    FreeMessageOnError(response, error);
}

void Server::HandleUdpReceive(Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    Error error = ProcessMessage(aMessage, aMessageInfo);

    LogWarnOnError(error, "handle DNS message");
    OT_UNUSED_VARIABLE(error);
}

Error Server::ProcessMessage(Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    return ProcessMessage(aMessage, TimerMilli::GetNow(), mTtlConfig, mLeaseConfig, &aMessageInfo);
}

Error Server::ProcessMessage(Message                &aMessage,
                             TimeMilli               aRxTime,
                             const TtlConfig        &aTtlConfig,
                             const LeaseConfig      &aLeaseConfig,
                             const Ip6::MessageInfo *aMessageInfo)
{
    Error           error;
    MessageMetadata metadata;

    metadata.mOffset      = aMessage.GetOffset();
    metadata.mRxTime      = aRxTime;
    metadata.mTtlConfig   = aTtlConfig;
    metadata.mLeaseConfig = aLeaseConfig;
    metadata.mMessageInfo = aMessageInfo;

    SuccessOrExit(error = aMessage.Read(metadata.mOffset, metadata.mDnsHeader));
    metadata.mOffset += sizeof(Dns::UpdateHeader);

    VerifyOrExit(metadata.mDnsHeader.GetType() == Dns::UpdateHeader::Type::kTypeQuery, error = kErrorDrop);
    VerifyOrExit(metadata.mDnsHeader.GetQueryType() == Dns::UpdateHeader::kQueryTypeUpdate, error = kErrorDrop);

    ProcessDnsUpdate(aMessage, metadata);

exit:
    return error;
}

void Server::HandleLeaseTimer(void)
{
    NextFireTime nextExpireTime;
    Host        *nextHost;

    for (Host *host = mHosts.GetHead(); host != nullptr; host = nextHost)
    {
        nextHost = host->GetNext();

        if (host->GetKeyExpireTime() <= nextExpireTime.GetNow())
        {
            LogInfo("KEY LEASE of host %s expired", host->GetFullName());

            // Removes the whole host and all services if the KEY RR expired.
            RemoveHost(host, kDeleteName);
        }
        else if (host->IsDeleted())
        {
            // The host has been deleted, but the hostname & service instance names retain.

            Service *next;

            nextExpireTime.UpdateIfEarlier(host->GetKeyExpireTime());

            // Check if any service instance name expired.
            for (Service *service = host->mServices.GetHead(); service != nullptr; service = next)
            {
                next = service->GetNext();

                OT_ASSERT(service->mIsDeleted);

                if (service->GetKeyExpireTime() <= nextExpireTime.GetNow())
                {
                    service->Log(Service::kKeyLeaseExpired);
                    host->RemoveService(service, kDeleteName, kNotifyServiceHandler);
                }
                else
                {
                    nextExpireTime.UpdateIfEarlier(service->GetKeyExpireTime());
                }
            }
        }
        else if (host->GetExpireTime() <= nextExpireTime.GetNow())
        {
            LogInfo("LEASE of host %s expired", host->GetFullName());

            // If the host expired, delete all resources of this host and its services.
            for (Service &service : host->mServices)
            {
                // Don't need to notify the service handler as `RemoveHost` at below will do.
                host->RemoveService(&service, kRetainName, kDoNotNotifyServiceHandler);
            }

            RemoveHost(host, kRetainName);

            nextExpireTime.UpdateIfEarlier(host->GetKeyExpireTime());
        }
        else
        {
            // The host doesn't expire, check if any service expired or is explicitly removed.

            Service *next;

            OT_ASSERT(!host->IsDeleted());

            nextExpireTime.UpdateIfEarlier(host->GetExpireTime());

            for (Service *service = host->mServices.GetHead(); service != nullptr; service = next)
            {
                next = service->GetNext();

                if (service->GetKeyExpireTime() <= nextExpireTime.GetNow())
                {
                    service->Log(Service::kKeyLeaseExpired);
                    host->RemoveService(service, kDeleteName, kNotifyServiceHandler);
                }
                else if (service->mIsDeleted)
                {
                    // The service has been deleted but the name retains.
                    nextExpireTime.UpdateIfEarlier(service->GetKeyExpireTime());
                }
                else if (service->GetExpireTime() <= nextExpireTime.GetNow())
                {
                    service->Log(Service::kLeaseExpired);

                    // The service is expired, delete it.
                    host->RemoveService(service, kRetainName, kNotifyServiceHandler);
                    nextExpireTime.UpdateIfEarlier(service->GetKeyExpireTime());
                }
                else
                {
                    nextExpireTime.UpdateIfEarlier(service->GetExpireTime());
                }
            }
        }
    }

    mLeaseTimer.FireAtIfEarlier(nextExpireTime);
}

void Server::HandleOutstandingUpdatesTimer(void)
{
    TimeMilli       now = TimerMilli::GetNow();
    UpdateMetadata *update;

    while ((update = mOutstandingUpdates.GetTail()) != nullptr)
    {
        if (update->GetExpireTime() > now)
        {
            mOutstandingUpdatesTimer.FireAtIfEarlier(update->GetExpireTime());
            break;
        }

        LogInfo("Outstanding service update timeout (updateId = %lu)", ToUlong(update->GetId()));

        IgnoreError(mOutstandingUpdates.Remove(*update));
        update->SetError(kErrorResponseTimeout);
        CommitSrpUpdate(*update);
    }
}

const char *Server::AddressModeToString(AddressMode aMode)
{
    static const char *const kAddressModeStrings[] = {
        "unicast", // (0) kAddressModeUnicast
        "anycast", // (1) kAddressModeAnycast
    };

    struct EnumCheck
    {
        InitEnumValidatorCounter();
        ValidateNextEnum(kAddressModeUnicast);
        ValidateNextEnum(kAddressModeAnycast);
    };

    return kAddressModeStrings[aMode];
}

void Server::UpdateResponseCounters(Dns::UpdateHeader::Response aResponseCode)
{
    switch (aResponseCode)
    {
    case Dns::UpdateHeader::kResponseSuccess:
        ++mResponseCounters.mSuccess;
        break;
    case Dns::UpdateHeader::kResponseServerFailure:
        ++mResponseCounters.mServerFailure;
        break;
    case Dns::UpdateHeader::kResponseFormatError:
        ++mResponseCounters.mFormatError;
        break;
    case Dns::UpdateHeader::kResponseNameExists:
        ++mResponseCounters.mNameExists;
        break;
    case Dns::UpdateHeader::kResponseRefused:
        ++mResponseCounters.mRefused;
        break;
    default:
        ++mResponseCounters.mOther;
        break;
    }
}

#if OPENTHREAD_FTD
void Server::UpdateAddrResolverCacheTable(const Ip6::MessageInfo &aMessageInfo, const Host &aHost)
{
    // If message is from a client on mesh, we add all registered
    // addresses as snooped entries in the address resolver cache
    // table. We associate the registered addresses with the same
    // RLOC16 (if any) as the received message's peer IPv6 address.

    uint16_t rloc16;

    VerifyOrExit(aHost.GetLease() != 0);
    VerifyOrExit(aHost.GetTtl() > 0);

    // If the `LookUp()` call succeeds, the cache entry will be marked
    // as "cached and in-use". We can mark it as "in-use" early since
    // the entry will be needed and used soon when sending the SRP
    // response. This also prevents a snooped cache entry (added for
    // `GetPeerAddr()` due to rx of the SRP update message) from
    // being overwritten by `UpdateSnoopedCacheEntry()` calls when
    // there are limited snoop entries available.

    rloc16 = Get<AddressResolver>().LookUp(aMessageInfo.GetPeerAddr());

    VerifyOrExit(rloc16 != Mle::kInvalidRloc16);

    for (const Ip6::Address &address : aHost.mAddresses)
    {
        Get<AddressResolver>().UpdateSnoopedCacheEntry(address, rloc16, Get<Mle::Mle>().GetRloc16());
    }

exit:
    return;
}
#endif

//---------------------------------------------------------------------------------------------------------------------
// Server::Service

Error Server::Service::Init(const char *aInstanceName, const char *aInstanceLabel, Host &aHost, TimeMilli aUpdateTime)
{
    Error error;

    mNext        = nullptr;
    mHost        = &aHost;
    mPriority    = 0;
    mWeight      = 0;
    mTtl         = 0;
    mPort        = 0;
    mLease       = 0;
    mKeyLease    = 0;
    mUpdateTime  = aUpdateTime;
    mIsDeleted   = false;
    mIsCommitted = false;
#if OPENTHREAD_CONFIG_SRP_SERVER_ADVERTISING_PROXY_ENABLE
    mIsRegistered      = false;
    mIsKeyRegistered   = false;
    mIsReplaced        = false;
    mShouldAdvertise   = false;
    mShouldRegisterKey = false;
    mAdvId             = kInvalidRequestId;
    mKeyAdvId          = kInvalidRequestId;
#endif

    mParsedDeleteAllRrset = false;
    mParsedSrv            = false;
    mParsedTxt            = false;

    SuccessOrExit(error = mInstanceLabel.Set(aInstanceLabel));
    error = mInstanceName.Set(aInstanceName);

exit:
    return error;
}

const char *Server::Service::GetSubTypeServiceNameAt(uint16_t aIndex) const
{
    const Heap::String *subType = mSubTypes.At(aIndex);

    return (subType == nullptr) ? nullptr : subType->AsCString();
}

Error Server::Service::ParseSubTypeServiceName(const char *aSubTypeServiceName, char *aLabel, uint8_t aLabelSize)
{
    Error       error = kErrorNone;
    const char *subPos;
    uint8_t     labelLength;

    aLabel[0] = kNullChar;

    subPos = StringFind(aSubTypeServiceName, kServiceSubTypeLabel, kStringCaseInsensitiveMatch);
    VerifyOrExit(subPos != nullptr, error = kErrorInvalidArgs);

    if (subPos - aSubTypeServiceName < aLabelSize)
    {
        labelLength = static_cast<uint8_t>(subPos - aSubTypeServiceName);
    }
    else
    {
        labelLength = aLabelSize - 1;
        error       = kErrorNoBufs;
    }

    memcpy(aLabel, aSubTypeServiceName, labelLength);
    aLabel[labelLength] = kNullChar;

exit:
    return error;
}

TimeMilli Server::Service::GetExpireTime(void) const
{
    OT_ASSERT(!mIsDeleted);
    OT_ASSERT(!GetHost().IsDeleted());

    return mUpdateTime + Time::SecToMsec(mLease);
}

TimeMilli Server::Service::GetKeyExpireTime(void) const { return mUpdateTime + Time::SecToMsec(mKeyLease); }

void Server::Service::GetLeaseInfo(LeaseInfo &aLeaseInfo) const
{
    TimeMilli now           = TimerMilli::GetNow();
    TimeMilli keyExpireTime = GetKeyExpireTime();

    aLeaseInfo.mLease             = Time::SecToMsec(GetLease());
    aLeaseInfo.mKeyLease          = Time::SecToMsec(GetKeyLease());
    aLeaseInfo.mRemainingKeyLease = (now <= keyExpireTime) ? (keyExpireTime - now) : 0;

    if (!mIsDeleted)
    {
        TimeMilli expireTime = GetExpireTime();

        aLeaseInfo.mRemainingLease = (now <= expireTime) ? (expireTime - now) : 0;
    }
    else
    {
        aLeaseInfo.mRemainingLease = 0;
    }
}

bool Server::Service::MatchesInstanceName(const char *aInstanceName) const { return Matches(aInstanceName); }

bool Server::Service::MatchesServiceName(const char *aServiceName) const
{
    return StringMatch(mServiceName.AsCString(), aServiceName, kStringCaseInsensitiveMatch);
}

bool Server::Service::Matches(const char *aInstanceName) const
{
    return StringMatch(mInstanceName.AsCString(), aInstanceName, kStringCaseInsensitiveMatch);
}

bool Server::Service::HasSubTypeServiceName(const char *aSubTypeServiceName) const
{
    bool has = false;

    for (const Heap::String &subType : mSubTypes)
    {
        if (StringMatch(subType.AsCString(), aSubTypeServiceName, kStringCaseInsensitiveMatch))
        {
            has = true;
            break;
        }
    }

    return has;
}

#if OT_SHOULD_LOG_AT(OT_LOG_LEVEL_INFO)
void Server::Service::Log(Action aAction) const
{
    static const char *const kActionStrings[] = {
        "Add new",                   // (0) kAddNew
        "Update existing",           // (1) kUpdateExisting
        "Keep unchanged",            // (2) kKeepUnchanged
        "Remove but retain name of", // (3) kRemoveButRetainName
        "Fully remove",              // (4) kFullyRemove
        "LEASE expired for",         // (5) kLeaseExpired
        "KEY LEASE expired for",     // (6) kKeyLeaseExpired
    };

    struct EnumCheck
    {
        InitEnumValidatorCounter();
        ValidateNextEnum(kAddNew);
        ValidateNextEnum(kUpdateExisting);
        ValidateNextEnum(kKeepUnchanged);
        ValidateNextEnum(kRemoveButRetainName);
        ValidateNextEnum(kFullyRemove);
        ValidateNextEnum(kLeaseExpired);
        ValidateNextEnum(kKeyLeaseExpired);
    };

    // We only log if the `Service` is marked as committed. This
    // ensures that temporary `Service` entries associated with a
    // newly received SRP update message are not logged (e.g., when
    // associated `Host` is being freed).

    if (mIsCommitted)
    {
        LogInfo("%s service '%s'", kActionStrings[aAction], GetInstanceName());

        for (const Heap::String &subType : mSubTypes)
        {
            Dns::Name::LabelBuffer label;

            IgnoreError(ParseSubTypeServiceName(subType.AsCString(), label));
            LogInfo("  sub-type: %s", subType.AsCString());
        }
    }
}
#else
void Server::Service::Log(Action) const {}
#endif // #if OT_SHOULD_LOG_AT(OT_LOG_LEVEL_INFO)

Error Server::Service::SetTxtDataFromMessage(const Message &aMessage, uint16_t aOffset, uint16_t aLength)
{
    Error error;

    SuccessOrExit(error = mTxtData.SetFrom(aMessage, aOffset, aLength));
    VerifyOrExit(Dns::TxtRecord::VerifyTxtData(mTxtData.GetBytes(), mTxtData.GetLength(), /* aAllowEmpty */ false),
                 error = kErrorParse);

exit:
    if (error != kErrorNone)
    {
        mTxtData.Free();
    }

    return error;
}

//---------------------------------------------------------------------------------------------------------------------
// Server::Host

Server::Host::Host(Instance &aInstance, TimeMilli aUpdateTime)
    : InstanceLocator(aInstance)
    , mNext(nullptr)
    , mTtl(0)
    , mLease(0)
    , mKeyLease(0)
    , mUpdateTime(aUpdateTime)
    , mParsedKey(false)
    , mUseShortLeaseOption(false)
#if OPENTHREAD_CONFIG_SRP_SERVER_ADVERTISING_PROXY_ENABLE
    , mIsRegistered(false)
    , mIsKeyRegistered(false)
    , mIsReplaced(false)
    , mShouldAdvertise(false)
    , mShouldRegisterKey(false)
    , mAdvId(kInvalidRequestId)
    , mKeyAdvId(kInvalidRequestId)
#endif
{
}

Server::Host::~Host(void) { FreeAllServices(); }

Error Server::Host::SetFullName(const char *aFullName)
{
    // `mFullName` becomes immutable after it is set, so if it is
    // already set, we only accept a `aFullName` that matches the
    // current name.

    Error error;

    if (mFullName.IsNull())
    {
        error = mFullName.Set(aFullName);
    }
    else
    {
        error = Matches(aFullName) ? kErrorNone : kErrorFailed;
    }

    return error;
}

bool Server::Host::Matches(const char *aFullName) const
{
    return StringMatch(mFullName.AsCString(), aFullName, kStringCaseInsensitiveMatch);
}

TimeMilli Server::Host::GetExpireTime(void) const
{
    OT_ASSERT(!IsDeleted());

    return mUpdateTime + Time::SecToMsec(mLease);
}

TimeMilli Server::Host::GetKeyExpireTime(void) const { return mUpdateTime + Time::SecToMsec(mKeyLease); }

void Server::Host::GetLeaseInfo(LeaseInfo &aLeaseInfo) const
{
    TimeMilli now           = TimerMilli::GetNow();
    TimeMilli keyExpireTime = GetKeyExpireTime();

    aLeaseInfo.mLease             = Time::SecToMsec(GetLease());
    aLeaseInfo.mKeyLease          = Time::SecToMsec(GetKeyLease());
    aLeaseInfo.mRemainingKeyLease = (now <= keyExpireTime) ? (keyExpireTime - now) : 0;

    if (!IsDeleted())
    {
        TimeMilli expireTime = GetExpireTime();

        aLeaseInfo.mRemainingLease = (now <= expireTime) ? (expireTime - now) : 0;
    }
    else
    {
        aLeaseInfo.mRemainingLease = 0;
    }
}

Error Server::Host::ProcessTtl(uint32_t aTtl)
{
    // This method processes the TTL value received in a resource record.
    //
    // If no TTL value is stored, this method will set the stored value to @p aTtl and return `kErrorNone`.
    // If a TTL value is stored and @p aTtl equals the stored value, this method returns `kErrorNone`.
    // Otherwise, this method returns `kErrorRejected`.

    Error error = kErrorRejected;

    VerifyOrExit(aTtl && (mTtl == 0 || mTtl == aTtl));

    mTtl = aTtl;

    error = kErrorNone;

exit:
    return error;
}

const Server::Service *Server::Host::GetNextService(const Service *aPrevService) const
{
    return (aPrevService == nullptr) ? mServices.GetHead() : aPrevService->GetNext();
}

Server::Service *Server::Host::AddNewService(const char *aInstanceName,
                                             const char *aInstanceLabel,
                                             TimeMilli   aUpdateTime)
{
    Service *service = Service::AllocateAndInit(aInstanceName, aInstanceLabel, *this, aUpdateTime);

    VerifyOrExit(service != nullptr);
    AddService(*service);

exit:
    return service;
}

void Server::Host::AddService(Service &aService)
{
    aService.mHost = this;
    mServices.Push(aService);
}

void Server::Host::RemoveService(Service *aService, RetainName aRetainName, NotifyMode aNotifyServiceHandler)
{
    Server &server = Get<Server>();

    VerifyOrExit(aService != nullptr);

    aService->mIsDeleted = true;
    aService->mLease     = 0;

    if (!aRetainName)
    {
        aService->mKeyLease = 0;
    }

    aService->Log(aRetainName ? Service::kRemoveButRetainName : Service::kFullyRemove);

    if (aNotifyServiceHandler && server.mServiceUpdateHandler.IsSet())
    {
        uint32_t updateId = server.AllocateServiceUpdateId();

        LogInfo("SRP update handler is notified (updatedId = %lu)", ToUlong(updateId));
        server.mServiceUpdateHandler.Invoke(updateId, this, static_cast<uint32_t>(kDefaultEventsHandlerTimeout));
        // We don't wait for the reply from the service update handler,
        // but always remove the service regardless of service update result.
        // Because removing a service should fail only when there is system
        // failure of the platform mDNS implementation and in which case the
        // service is not expected to be still registered.
    }
#if OPENTHREAD_CONFIG_SRP_SERVER_ADVERTISING_PROXY_ENABLE
    else if (aNotifyServiceHandler)
    {
        Get<AdvertisingProxy>().AdvertiseRemovalOf(*aService);
    }
#endif

    if (!aRetainName)
    {
        IgnoreError(mServices.Remove(*aService));
        aService->Free();
    }

exit:
    return;
}

void Server::Host::FreeAllServices(void)
{
    while (!mServices.IsEmpty())
    {
        RemoveService(mServices.GetHead(), kDeleteName, kDoNotNotifyServiceHandler);
    }
}

void Server::Host::ClearResources(void) { mAddresses.Free(); }

Server::Service *Server::Host::FindService(const char *aInstanceName) { return mServices.FindMatching(aInstanceName); }

const Server::Service *Server::Host::FindService(const char *aInstanceName) const
{
    return mServices.FindMatching(aInstanceName);
}

bool Server::Host::HasService(const char *aInstanceName) const { return mServices.ContainsMatching(aInstanceName); }

Error Server::Host::AddIp6Address(const Ip6::Address &aIp6Address)
{
    Error error = kErrorNone;

    if (aIp6Address.IsMulticast() || aIp6Address.IsUnspecified() || aIp6Address.IsLoopback())
    {
        // We don't like those address because they cannot be used
        // for communication with exterior devices.
        ExitNow(error = kErrorDrop);
    }

    // Drop duplicate addresses.
    VerifyOrExit(!mAddresses.Contains(aIp6Address), error = kErrorDrop);

    error = mAddresses.PushBack(aIp6Address);

    if (error == kErrorNoBufs)
    {
        LogWarn("Too many addresses for host %s", GetFullName());
    }

exit:
    return error;
}

//---------------------------------------------------------------------------------------------------------------------
// Server::UpdateMetadata

Server::UpdateMetadata::UpdateMetadata(Instance &aInstance, Host &aHost, const MessageMetadata &aMessageMetadata)
    : InstanceLocator(aInstance)
    , mNext(nullptr)
    , mExpireTime(TimerMilli::GetNow() + kDefaultEventsHandlerTimeout)
    , mDnsHeader(aMessageMetadata.mDnsHeader)
    , mId(Get<Server>().AllocateServiceUpdateId())
    , mTtlConfig(aMessageMetadata.mTtlConfig)
    , mLeaseConfig(aMessageMetadata.mLeaseConfig)
    , mHost(aHost)
    , mError(kErrorNone)
    , mIsDirectRxFromClient(aMessageMetadata.IsDirectRxFromClient())
{
    if (aMessageMetadata.mMessageInfo != nullptr)
    {
        mMessageInfo = *aMessageMetadata.mMessageInfo;
    }
}

} // namespace Srp
} // namespace ot

#endif // OPENTHREAD_CONFIG_SRP_SERVER_ENABLE
