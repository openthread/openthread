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

#include "common/instance.hpp"
#include "common/locator-getters.hpp"
#include "common/logging.hpp"
#include "common/new.hpp"
#include "net/dns_headers.hpp"
#include "thread/network_data_service.hpp"
#include "thread/thread_netif.hpp"

namespace ot {
namespace Srp {

static const char kDefaultDomain[] = "default.service.arpa.";

static Dns::UpdateHeader::Response ErrorToDnsResponseCode(otError aError)
{
    Dns::UpdateHeader::Response responseCode;

    switch (aError)
    {
    case OT_ERROR_NONE:
        responseCode = Dns::UpdateHeader::kResponseSuccess;
        break;
    case OT_ERROR_NO_BUFS:
        responseCode = Dns::UpdateHeader::kResponseServerFailure;
        break;
    case OT_ERROR_PARSE:
        responseCode = Dns::UpdateHeader::kResponseFormatError;
        break;
    case OT_ERROR_DUPLICATED:
        responseCode = Dns::UpdateHeader::kResponseNameExists;
        break;
    default:
        responseCode = Dns::UpdateHeader::kResponseRefused;
        break;
    }

    return responseCode;
}

Server::Server(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mSocket(aInstance)
    , mAdvertisingHandler(nullptr)
    , mAdvertisingHandlerContext(nullptr)
    , mDomain(nullptr)
    , mMinLease(kDefaultMinLease)
    , mMaxLease(kDefaultMaxLease)
    , mMinKeyLease(kDefaultMinKeyLease)
    , mMaxKeyLease(kDefaultMaxKeyLease)
    , mLeaseTimer(aInstance, HandleLeaseTimer)
    , mOutstandingUpdatesTimer(aInstance, HandleOutstandingUpdatesTimer)
    , mEnabled(false)
{
    IgnoreError(SetDomain(kDefaultDomain));
}

Server::~Server(void)
{
    Instance::HeapFree(mDomain);
}

void Server::SetServiceHandler(otSrpServerServiceUpdateHandler aServiceHandler, void *aServiceHandlerContext)
{
    mAdvertisingHandler        = aServiceHandler;
    mAdvertisingHandlerContext = aServiceHandlerContext;
}

bool Server::IsRunning(void) const
{
    return mSocket.IsBound();
}

void Server::SetEnabled(bool aEnabled)
{
    VerifyOrExit(mEnabled != aEnabled);

    mEnabled = aEnabled;

    if (!mEnabled)
    {
        Stop();
    }
    else if (Get<Mle::MleRouter>().IsAttached())
    {
        Start();
    }

exit:
    return;
}

otError Server::SetLeaseRange(uint32_t aMinLease, uint32_t aMaxLease, uint32_t aMinKeyLease, uint32_t aMaxKeyLease)
{
    otError error = OT_ERROR_NONE;

    // TODO: Support longer LEASE.
    // We use milliseconds timer for LEASE & KEY-LEASE, this is to avoid overflow.
    VerifyOrExit(aMaxKeyLease <= TimerMilli::kMaxDelay / 1000, error = OT_ERROR_INVALID_ARGS);
    VerifyOrExit(aMinLease <= aMaxLease, error = OT_ERROR_INVALID_ARGS);
    VerifyOrExit(aMinKeyLease <= aMaxKeyLease, error = OT_ERROR_INVALID_ARGS);
    VerifyOrExit(aMinLease <= aMinKeyLease, error = OT_ERROR_INVALID_ARGS);
    VerifyOrExit(aMaxLease <= aMaxKeyLease, error = OT_ERROR_INVALID_ARGS);

    mMinLease    = aMinLease;
    mMaxLease    = aMaxLease;
    mMinKeyLease = aMinKeyLease;
    mMaxKeyLease = aMaxKeyLease;

exit:
    return error;
}

uint32_t Server::GrantLease(uint32_t aLease) const
{
    OT_ASSERT(mMinLease <= mMaxLease);

    return (aLease == 0) ? 0 : OT_MAX(mMinLease, OT_MIN(mMaxLease, aLease));
}

uint32_t Server::GrantKeyLease(uint32_t aKeyLease) const
{
    OT_ASSERT(mMinKeyLease <= mMaxKeyLease);

    return (aKeyLease == 0) ? 0 : OT_MAX(mMinKeyLease, OT_MIN(mMaxKeyLease, aKeyLease));
}

const char *Server::GetDomain(void) const
{
    return mDomain;
}

otError Server::SetDomain(const char *aDomain)
{
    otError error             = OT_ERROR_NONE;
    char *  buf               = nullptr;
    size_t  appendTrailingDot = 0;
    size_t  length            = strlen(aDomain);

    VerifyOrExit(!mEnabled, error = OT_ERROR_INVALID_STATE);

    VerifyOrExit(length > 0 && length < Dns::Name::kMaxNameSize, error = OT_ERROR_INVALID_ARGS);
    if (aDomain[length - 1] != '.')
    {
        appendTrailingDot = 1;
    }

    buf = static_cast<char *>(Instance::HeapCAlloc(1, length + appendTrailingDot + 1));
    VerifyOrExit(buf != nullptr, error = OT_ERROR_NO_BUFS);

    strcpy(buf, aDomain);
    if (appendTrailingDot)
    {
        buf[length]     = '.';
        buf[length + 1] = '\0';
    }
    Instance::HeapFree(mDomain);
    mDomain = buf;

exit:
    if (error != OT_ERROR_NONE)
    {
        Instance::HeapFree(buf);
    }
    return error;
}

const Server::Host *Server::GetNextHost(const Server::Host *aHost)
{
    return (aHost == nullptr) ? mHosts.GetHead() : aHost->GetNext();
}

// This method adds a SRP service host and takes ownership of it.
// The caller MUST make sure that there is no existing host with the same hostname.
void Server::AddHost(Host *aHost)
{
    OT_ASSERT(mHosts.FindMatching(aHost->GetFullName()) == nullptr);
    IgnoreError(mHosts.Add(*aHost));
}

void Server::RemoveAndFreeHost(Host *aHost)
{
    otLogInfoSrp("[server] fully remove host %s", aHost->GetFullName());
    IgnoreError(mHosts.Remove(*aHost));
    aHost->Free();
}

const Server::Service *Server::FindService(const char *aFullName) const
{
    const Service *service = nullptr;

    for (const Host *host = mHosts.GetHead(); host != nullptr; host = host->GetNext())
    {
        service = host->FindService(aFullName);
        if (service != nullptr)
        {
            break;
        }
    }

    return service;
}

bool Server::HasNameConflictsWith(Host &aHost) const
{
    bool           hasConflicts = false;
    const Service *service      = nullptr;
    const Host *   existingHost = mHosts.FindMatching(aHost.GetFullName());

    if (existingHost != nullptr && *aHost.GetKey() != *existingHost->GetKey())
    {
        ExitNow(hasConflicts = true);
    }

    // Check not only services of this host but all hosts.
    while ((service = aHost.GetNextService(service)) != nullptr)
    {
        const Service *existingService = FindService(service->mFullName);
        if (existingService != nullptr && *service->GetHost().GetKey() != *existingService->GetHost().GetKey())
        {
            ExitNow(hasConflicts = true);
        }
    }

exit:
    return hasConflicts;
}

void Server::HandleAdvertisingResult(const Host *aHost, otError aError)
{
    UpdateMetadata *update = mOutstandingUpdates.FindMatching(aHost);

    if (update != nullptr)
    {
        HandleAdvertisingResult(update, aError);
    }
    else
    {
        otLogInfoSrp("[server] delayed SRP host update result, the SRP update has been committed");
    }
}

void Server::HandleAdvertisingResult(UpdateMetadata *aUpdate, otError aError)
{
    HandleSrpUpdateResult(aError, aUpdate->GetDnsHeader(), aUpdate->GetHost(), aUpdate->GetMessageInfo());

    IgnoreError(mOutstandingUpdates.Remove(*aUpdate));
    aUpdate->Free();

    if (mOutstandingUpdates.IsEmpty())
    {
        mOutstandingUpdatesTimer.Stop();
    }
    else
    {
        mOutstandingUpdatesTimer.StartAt(mOutstandingUpdates.GetTail()->GetExpireTime(), 0);
    }
}

void Server::HandleSrpUpdateResult(otError                  aError,
                                   const Dns::UpdateHeader &aDnsHeader,
                                   Host &                   aHost,
                                   const Ip6::MessageInfo & aMessageInfo)
{
    Host *   existingHost;
    uint32_t hostLease;
    uint32_t hostKeyLease;
    uint32_t grantedLease;
    uint32_t grantedKeyLease;

    SuccessOrExit(aError);

    hostLease       = aHost.GetLease();
    hostKeyLease    = aHost.GetKeyLease();
    grantedLease    = GrantLease(hostLease);
    grantedKeyLease = GrantKeyLease(hostKeyLease);

    aHost.SetLease(grantedLease);
    aHost.SetKeyLease(grantedKeyLease);

    existingHost = mHosts.FindMatching(aHost.GetFullName());

    if (aHost.GetLease() == 0)
    {
        if (aHost.GetKeyLease() == 0)
        {
            otLogInfoSrp("[server] remove key of host %s", aHost.GetFullName());

            if (existingHost != nullptr)
            {
                RemoveAndFreeHost(existingHost);
            }
        }
        else if (existingHost != nullptr)
        {
            Service *service = nullptr;

            existingHost->SetLease(aHost.GetLease());
            existingHost->SetKeyLease(aHost.GetKeyLease());

            // Clear all resources associated to this host and its services.
            existingHost->ClearResources();
            otLogInfoSrp("[server] remove host '%s' (but retain its name)", existingHost->GetFullName());
            while ((service = existingHost->GetNextService(service)) != nullptr)
            {
                service->DeleteResourcesButRetainName();
            }
        }

        aHost.Free();
    }
    else if (existingHost != nullptr)
    {
        const Service *service = nullptr;

        // Merge current updates into existing host.

        otLogInfoSrp("[server] update host %s", existingHost->GetFullName());

        existingHost->CopyResourcesFrom(aHost);
        while ((service = aHost.GetNextService(service)) != nullptr)
        {
            Service *existingService = existingHost->FindService(service->mFullName);

            if (service->mIsDeleted)
            {
                if (existingService != nullptr)
                {
                    existingService->DeleteResourcesButRetainName();
                }
            }
            else
            {
                Service *newService = existingHost->AddService(service->mFullName);

                VerifyOrExit(newService != nullptr, aError = OT_ERROR_NO_BUFS);
                SuccessOrExit(aError = newService->CopyResourcesFrom(*service));
                otLogInfoSrp("[server] %s service %s", (existingService != nullptr) ? "update existing" : "add new",
                             service->mFullName);
            }
        }

        aHost.Free();
    }
    else
    {
        otLogInfoSrp("[server] add new host %s", aHost.GetFullName());
        AddHost(&aHost);
    }

    // Re-schedule the lease timer.
    HandleLeaseTimer();

exit:
    if (aError == OT_ERROR_NONE && !(grantedLease == hostLease && grantedKeyLease == hostKeyLease))
    {
        SendResponse(aDnsHeader, grantedLease, grantedKeyLease, aMessageInfo);
    }
    else
    {
        SendResponse(aDnsHeader, ErrorToDnsResponseCode(aError), aMessageInfo);
    }
}

void Server::Start(void)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(!IsRunning());

    SuccessOrExit(error = mSocket.Open(HandleUdpReceive, this));
    SuccessOrExit(error = mSocket.Bind(0));

    SuccessOrExit(error = PublishServerData());

    otLogInfoSrp("[server] start listening on port %hu", mSocket.GetSockName().mPort);

exit:
    if (error != OT_ERROR_NONE)
    {
        otLogCritSrp("[server] failed to start: %s", otThreadErrorToString(error));
        // Cleanup any resources we may have allocated.
        Stop();
    }
}

void Server::Stop(void)
{
    VerifyOrExit(IsRunning());

    UnpublishServerData();

    while (!mHosts.IsEmpty())
    {
        mHosts.Pop()->Free();
    }

    while (!mOutstandingUpdates.IsEmpty())
    {
        mOutstandingUpdates.Pop()->Free();
    }

    mLeaseTimer.Stop();
    mOutstandingUpdatesTimer.Stop();

    otLogInfoSrp("[server] stop listening on %hu", mSocket.GetSockName().mPort);
    IgnoreError(mSocket.Close());

exit:
    return;
}

void Server::HandleNotifierEvents(Events aEvents)
{
    VerifyOrExit(mEnabled);
    VerifyOrExit(aEvents.Contains(kEventThreadRoleChanged));

    if (Get<Mle::MleRouter>().IsAttached())
    {
        Start();
    }
    else
    {
        Stop();
    }

exit:
    return;
}

otError Server::PublishServerData(void)
{
    NetworkData::Service::SrpServer::ServerData serverData;

    OT_ASSERT(mSocket.IsBound());

    serverData.SetPort(mSocket.GetSockName().GetPort());

    return Get<NetworkData::Service::Manager>().Add<NetworkData::Service::SrpServer>(serverData);
}

void Server::UnpublishServerData(void)
{
    otError error = Get<NetworkData::Service::Manager>().Remove<NetworkData::Service::SrpServer>();

    if (error != OT_ERROR_NONE)
    {
        otLogWarnSrp("[server] failed to unpublish SRP service: %s", otThreadErrorToString(error));
    }
}

const Server::UpdateMetadata *Server::FindOutstandingUpdate(const Ip6::MessageInfo &aMessageInfo,
                                                            uint16_t                aDnsMessageId)
{
    const UpdateMetadata *ret = nullptr;

    for (const UpdateMetadata *update = mOutstandingUpdates.GetHead(); update != nullptr; update = update->GetNext())
    {
        if (aDnsMessageId == update->GetDnsHeader().GetMessageId() &&
            aMessageInfo.GetPeerAddr() == update->GetMessageInfo().GetPeerAddr() &&
            aMessageInfo.GetPeerPort() == update->GetMessageInfo().GetPeerPort())
        {
            ExitNow(ret = update);
        }
    }

exit:
    return ret;
}

void Server::HandleDnsUpdate(Message &                aMessage,
                             const Ip6::MessageInfo & aMessageInfo,
                             const Dns::UpdateHeader &aDnsHeader,
                             uint16_t                 aOffset)
{
    otError   error = OT_ERROR_NONE;
    Dns::Zone zone;
    Host *    host = nullptr;

    otLogInfoSrp("[server] receive DNS update from %s", aMessageInfo.GetPeerAddr().ToString().AsCString());

    SuccessOrExit(error = ProcessZoneSection(aMessage, aDnsHeader, aOffset, zone));

    if (FindOutstandingUpdate(aMessageInfo, aDnsHeader.GetMessageId()) != nullptr)
    {
        otLogInfoSrp("[server] drop duplicated SRP update request: messageId=%hu", aDnsHeader.GetMessageId());

        // Silently drop duplicate requests.
        // This could rarely happen, because the outstanding SRP update timer should
        // be shorter than the SRP update retransmission timer.
        ExitNow(error = OT_ERROR_NONE);
    }

    // Per 2.3.2 of SRP draft 6, no prerequisites should be included in a SRP update.
    VerifyOrExit(aDnsHeader.GetPrerequisiteRecordCount() == 0, error = OT_ERROR_FAILED);

    host = Host::New();
    VerifyOrExit(host != nullptr, error = OT_ERROR_NO_BUFS);
    SuccessOrExit(error = ProcessUpdateSection(*host, aMessage, aDnsHeader, zone, aOffset));

    // Parse lease time and validate signature.
    SuccessOrExit(error = ProcessAdditionalSection(host, aMessage, aDnsHeader, aOffset));

    HandleUpdate(aDnsHeader, host, aMessageInfo);

exit:
    if (error != OT_ERROR_NONE)
    {
        if (host != nullptr)
        {
            host->Free();
        }
        SendResponse(aDnsHeader, ErrorToDnsResponseCode(error), aMessageInfo);
    }
}

otError Server::ProcessZoneSection(const Message &          aMessage,
                                   const Dns::UpdateHeader &aDnsHeader,
                                   uint16_t &               aOffset,
                                   Dns::Zone &              aZone) const
{
    otError   error = OT_ERROR_NONE;
    char      name[Dns::Name::kMaxNameSize];
    Dns::Zone zone;

    VerifyOrExit(aDnsHeader.GetZoneRecordCount() == 1, error = OT_ERROR_PARSE);

    SuccessOrExit(error = Dns::Name::ReadName(aMessage, aOffset, name, sizeof(name)));
    // TODO: return `Dns::kResponseNotAuth` for not authorized zone names.
    VerifyOrExit(strcmp(name, GetDomain()) == 0, error = OT_ERROR_SECURITY);
    SuccessOrExit(error = aMessage.Read(aOffset, zone));
    aOffset += sizeof(zone);

    VerifyOrExit(zone.GetType() == Dns::ResourceRecord::kTypeSoa, error = OT_ERROR_PARSE);
    aZone = zone;

exit:
    return error;
}

otError Server::ProcessUpdateSection(Host &                   aHost,
                                     const Message &          aMessage,
                                     const Dns::UpdateHeader &aDnsHeader,
                                     const Dns::Zone &        aZone,
                                     uint16_t &               aOffset) const
{
    otError error = OT_ERROR_NONE;

    // Process Service Discovery, Host and Service Description Instructions with
    // 3 times iterations over all DNS update RRs. The order of those processes matters.

    // 0. Enumerate over all Service Discovery Instructions before processing any other records.
    // So that we will know whether a name is a hostname or service instance name when processing
    // a "Delete All RRsets from a name" record.
    error = ProcessServiceDiscoveryInstructions(aHost, aMessage, aDnsHeader, aZone, aOffset);
    SuccessOrExit(error);

    // 1. Enumerate over all RRs to build the Host Description Instruction.
    error = ProcessHostDescriptionInstruction(aHost, aMessage, aDnsHeader, aZone, aOffset);
    SuccessOrExit(error);

    // 2. Enumerate over all RRs to build the Service Description Insutructions.
    error = ProcessServiceDescriptionInstructions(aHost, aMessage, aDnsHeader, aZone, aOffset);
    SuccessOrExit(error);

    // 3. Verify that there are no name conflicts.
    VerifyOrExit(!HasNameConflictsWith(aHost), error = OT_ERROR_DUPLICATED);

exit:
    return error;
}

otError Server::ProcessHostDescriptionInstruction(Host &                   aHost,
                                                  const Message &          aMessage,
                                                  const Dns::UpdateHeader &aDnsHeader,
                                                  const Dns::Zone &        aZone,
                                                  uint16_t                 aOffset) const
{
    otError error;

    OT_ASSERT(aHost.GetFullName() == nullptr);

    for (uint16_t i = 0; i < aDnsHeader.GetUpdateRecordCount(); ++i)
    {
        char                name[Dns::Name::kMaxNameSize];
        Dns::ResourceRecord record;

        SuccessOrExit(error = Dns::Name::ReadName(aMessage, aOffset, name, sizeof(name)));
        // TODO: return `Dns::kResponseNotZone` for names not in the zone.
        VerifyOrExit(Dns::Name::IsSubDomainOf(name, GetDomain()), error = OT_ERROR_SECURITY);
        SuccessOrExit(error = aMessage.Read(aOffset, record));

        if (record.GetClass() == Dns::ResourceRecord::kClassAny)
        {
            Service *service;

            // Delete All RRsets from a name.
            VerifyOrExit(IsValidDeleteAllRecord(record), error = OT_ERROR_FAILED);

            service = aHost.FindService(name);
            if (service == nullptr)
            {
                // A "Delete All RRsets from a name" RR can only apply to a Service or Host Description.

                if (aHost.GetFullName())
                {
                    VerifyOrExit(aHost.Matches(name), error = OT_ERROR_FAILED);
                }
                else
                {
                    SuccessOrExit(error = aHost.SetFullName(name));
                }
                aHost.ClearResources();
            }

            aOffset += record.GetSize();
            continue;
        }

        if (record.GetType() == Dns::ResourceRecord::kTypeAaaa)
        {
            Dns::AaaaRecord aaaaRecord;

            VerifyOrExit(record.GetClass() == aZone.GetClass(), error = OT_ERROR_FAILED);
            if (aHost.GetFullName() == nullptr)
            {
                SuccessOrExit(error = aHost.SetFullName(name));
            }
            else
            {
                VerifyOrExit(aHost.Matches(name), error = OT_ERROR_FAILED);
            }

            SuccessOrExit(error = aMessage.Read(aOffset, aaaaRecord));
            VerifyOrExit(aaaaRecord.IsValid(), error = OT_ERROR_PARSE);

            // Tolerate OT_ERROR_DROP for AAAA Resources.
            VerifyOrExit(aHost.AddIp6Address(aaaaRecord.GetAddress()) != OT_ERROR_NO_BUFS, error = OT_ERROR_NO_BUFS);

            aOffset += aaaaRecord.GetSize();
        }
        else if (record.GetType() == Dns::ResourceRecord::kTypeKey)
        {
            // We currently support only ECDSA P-256.
            Dns::Ecdsa256KeyRecord key;

            VerifyOrExit(record.GetClass() == aZone.GetClass(), error = OT_ERROR_FAILED);
            VerifyOrExit(aHost.GetKey() == nullptr, error = OT_ERROR_FAILED);

            SuccessOrExit(error = aMessage.Read(aOffset, key));
            VerifyOrExit(key.IsValid(), error = OT_ERROR_PARSE);

            aHost.SetKey(key);

            aOffset += record.GetSize();
        }
        else
        {
            aOffset += record.GetSize();
        }
    }

    // Verify that we have a complete Host Description Instruction.

    VerifyOrExit(aHost.GetFullName() != nullptr, error = OT_ERROR_FAILED);
    VerifyOrExit(aHost.GetKey() != nullptr, error = OT_ERROR_FAILED);
    {
        uint8_t hostAddressesNum;

        aHost.GetAddresses(hostAddressesNum);

        // There MUST be at least one valid address if we have nonzero lease.
        VerifyOrExit(aHost.GetLease() > 0 || hostAddressesNum > 0, error = OT_ERROR_FAILED);
    }

exit:
    return error;
}

otError Server::ProcessServiceDiscoveryInstructions(Host &                   aHost,
                                                    const Message &          aMessage,
                                                    const Dns::UpdateHeader &aDnsHeader,
                                                    const Dns::Zone &        aZone,
                                                    uint16_t                 aOffset) const
{
    otError error = OT_ERROR_NONE;

    for (uint16_t i = 0; i < aDnsHeader.GetUpdateRecordCount(); ++i)
    {
        char                name[Dns::Name::kMaxNameSize];
        Dns::ResourceRecord record;
        char                serviceName[Dns::Name::kMaxNameSize];
        Service *           service;

        SuccessOrExit(error = Dns::Name::ReadName(aMessage, aOffset, name, sizeof(name)));
        VerifyOrExit(Dns::Name::IsSubDomainOf(name, GetDomain()), error = OT_ERROR_SECURITY);
        SuccessOrExit(error = aMessage.Read(aOffset, record));

        aOffset += sizeof(record);

        if (record.GetType() == Dns::ResourceRecord::kTypePtr)
        {
            SuccessOrExit(error = Dns::Name::ReadName(aMessage, aOffset, serviceName, sizeof(serviceName)));
            VerifyOrExit(Dns::Name::IsSubDomainOf(name, GetDomain()), error = OT_ERROR_SECURITY);
        }
        else
        {
            aOffset += record.GetLength();
            continue;
        }

        VerifyOrExit(record.GetClass() == Dns::ResourceRecord::kClassNone || record.GetClass() == aZone.GetClass(),
                     error = OT_ERROR_FAILED);

        // TODO: check if the RR name and the full service name matches.

        service = aHost.FindService(serviceName);
        VerifyOrExit(service == nullptr, error = OT_ERROR_FAILED);
        service = aHost.AddService(serviceName);
        VerifyOrExit(service != nullptr, error = OT_ERROR_NO_BUFS);

        // This RR is a "Delete an RR from an RRset" update when the CLASS is NONE.
        service->mIsDeleted = (record.GetClass() == Dns::ResourceRecord::kClassNone);
    }

exit:
    return error;
}

otError Server::ProcessServiceDescriptionInstructions(Host &                   aHost,
                                                      const Message &          aMessage,
                                                      const Dns::UpdateHeader &aDnsHeader,
                                                      const Dns::Zone &        aZone,
                                                      uint16_t &               aOffset) const
{
    Service *service;
    otError  error = OT_ERROR_NONE;

    for (uint16_t i = 0; i < aDnsHeader.GetUpdateRecordCount(); ++i)
    {
        char                name[Dns::Name::kMaxNameSize];
        Dns::ResourceRecord record;

        SuccessOrExit(error = Dns::Name::ReadName(aMessage, aOffset, name, sizeof(name)));
        VerifyOrExit(Dns::Name::IsSubDomainOf(name, GetDomain()), error = OT_ERROR_SECURITY);
        SuccessOrExit(error = aMessage.Read(aOffset, record));

        if (record.GetClass() == Dns::ResourceRecord::kClassAny)
        {
            // Delete All RRsets from a name.
            VerifyOrExit(IsValidDeleteAllRecord(record), error = OT_ERROR_FAILED);
            service = aHost.FindService(name);
            if (service != nullptr)
            {
                service->ClearResources();
            }

            aOffset += record.GetSize();
            continue;
        }

        if (record.GetType() == Dns::ResourceRecord::kTypeSrv)
        {
            Dns::SrvRecord srvRecord;
            char           hostName[Dns::Name::kMaxNameSize];
            uint16_t       hostNameLength = sizeof(hostName);

            VerifyOrExit(record.GetClass() == aZone.GetClass(), error = OT_ERROR_FAILED);
            SuccessOrExit(error = aMessage.Read(aOffset, srvRecord));
            aOffset += sizeof(srvRecord);

            SuccessOrExit(error = Dns::Name::ReadName(aMessage, aOffset, hostName, hostNameLength));
            VerifyOrExit(Dns::Name::IsSubDomainOf(name, GetDomain()), error = OT_ERROR_SECURITY);
            VerifyOrExit(aHost.Matches(hostName), error = OT_ERROR_FAILED);

            service = aHost.FindService(name);
            VerifyOrExit(service != nullptr && !service->mIsDeleted, error = OT_ERROR_FAILED);

            // Make sure that this is the first SRV RR for this service.
            VerifyOrExit(service->mPort == 0, error = OT_ERROR_FAILED);
            service->mPriority = srvRecord.GetPriority();
            service->mWeight   = srvRecord.GetWeight();
            service->mPort     = srvRecord.GetPort();
        }
        else if (record.GetType() == Dns::ResourceRecord::kTypeTxt)
        {
            VerifyOrExit(record.GetClass() == aZone.GetClass(), error = OT_ERROR_FAILED);

            service = aHost.FindService(name);
            VerifyOrExit(service != nullptr && !service->mIsDeleted, error = OT_ERROR_FAILED);

            aOffset += sizeof(record);
            SuccessOrExit(error = service->SetTxtDataFromMessage(aMessage, aOffset, record.GetLength()));
            aOffset += record.GetLength();
        }
        else
        {
            aOffset += record.GetSize();
        }
    }

    service = nullptr;
    while ((service = aHost.GetNextService(service)) != nullptr)
    {
        VerifyOrExit(service->mIsDeleted || (service->mTxtData != nullptr && service->mPort != 0),
                     error = OT_ERROR_FAILED);
    }

exit:
    return error;
}

bool Server::IsValidDeleteAllRecord(const Dns::ResourceRecord &aRecord)
{
    return aRecord.GetClass() == Dns::ResourceRecord::kClassAny && aRecord.GetType() == Dns::ResourceRecord::kTypeAny &&
           aRecord.GetTtl() == 0 && aRecord.GetLength() == 0;
}

otError Server::ProcessAdditionalSection(Host *                   aHost,
                                         const Message &          aMessage,
                                         const Dns::UpdateHeader &aDnsHeader,
                                         uint16_t &               aOffset) const
{
    otError          error = OT_ERROR_NONE;
    Dns::OptRecord   optRecord;
    Dns::LeaseOption leaseOption;
    Dns::SigRecord   sigRecord;
    char             name[2]; // The root domain name (".") is expected.
    uint16_t         sigOffset;
    uint16_t         sigRdataOffset;
    char             signerName[Dns::Name::kMaxNameSize];
    uint16_t         signatureLength;

    VerifyOrExit(aDnsHeader.GetAdditionalRecordCount() == 2, error = OT_ERROR_FAILED);

    // EDNS(0) Update Lease Option.

    SuccessOrExit(error = Dns::Name::ReadName(aMessage, aOffset, name, sizeof(name)));
    SuccessOrExit(error = aMessage.Read(aOffset, optRecord));
    SuccessOrExit(error = aMessage.Read(aOffset + sizeof(optRecord), leaseOption));
    VerifyOrExit(leaseOption.IsValid(), error = OT_ERROR_FAILED);
    VerifyOrExit(optRecord.GetSize() == sizeof(optRecord) + sizeof(leaseOption), error = OT_ERROR_PARSE);

    aOffset += optRecord.GetSize();

    aHost->SetLease(leaseOption.GetLeaseInterval());
    aHost->SetKeyLease(leaseOption.GetKeyLeaseInterval());

    // SIG(0).

    sigOffset = aOffset;
    SuccessOrExit(error = Dns::Name::ReadName(aMessage, aOffset, name, sizeof(name)));
    SuccessOrExit(error = aMessage.Read(aOffset, sigRecord));
    VerifyOrExit(sigRecord.IsValid(), error = OT_ERROR_PARSE);

    sigRdataOffset = aOffset + sizeof(Dns::ResourceRecord);
    aOffset += sizeof(sigRecord);

    // TODO: Verify that the signature doesn't expire. This is not
    // implemented because the end device may not be able to get
    // the synchronized date/time.

    SuccessOrExit(error = Dns::Name::ReadName(aMessage, aOffset, signerName, sizeof(signerName)));

    signatureLength = sigRecord.GetLength() - (aOffset - sigRdataOffset);
    aOffset += signatureLength;

    // Verify the signature. Currently supports only ECDSA.

    VerifyOrExit(sigRecord.GetAlgorithm() == Dns::KeyRecord::kAlgorithmEcdsaP256Sha256, error = OT_ERROR_FAILED);
    VerifyOrExit(sigRecord.GetTypeCovered() == 0, error = OT_ERROR_FAILED);
    VerifyOrExit(signatureLength == Crypto::Ecdsa::P256::Signature::kSize, error = OT_ERROR_PARSE);

    SuccessOrExit(error = VerifySignature(*aHost->GetKey(), aMessage, aDnsHeader, sigOffset, sigRdataOffset,
                                          sigRecord.GetLength(), signerName));

exit:
    return error;
}

otError Server::VerifySignature(const Dns::Ecdsa256KeyRecord &aKey,
                                const Message &               aMessage,
                                Dns::UpdateHeader             aDnsHeader,
                                uint16_t                      aSigOffset,
                                uint16_t                      aSigRdataOffset,
                                uint16_t                      aSigRdataLength,
                                const char *                  aSignerName) const
{
    otError                        error;
    uint16_t                       offset = aMessage.GetOffset();
    uint16_t                       signatureOffset;
    Crypto::Sha256                 sha256;
    Crypto::Sha256::Hash           hash;
    Crypto::Ecdsa::P256::Signature signature;
    Message *                      signerNameMessage = nullptr;

    VerifyOrExit(aSigRdataLength >= Crypto::Ecdsa::P256::Signature::kSize, error = OT_ERROR_INVALID_ARGS);

    sha256.Start();

    // SIG RDATA less signature.
    sha256.Update(aMessage, aSigRdataOffset, sizeof(Dns::SigRecord) - sizeof(Dns::ResourceRecord));

    // The uncompressed (canonical) form of the signer name should be used for signature
    // verification. See https://tools.ietf.org/html/rfc2931#section-3.1 for details.
    signerNameMessage = Get<Ip6::Udp>().NewMessage(0);
    VerifyOrExit(signerNameMessage != nullptr, error = OT_ERROR_NO_BUFS);
    SuccessOrExit(error = Dns::Name::AppendName(aSignerName, *signerNameMessage));
    sha256.Update(*signerNameMessage, signerNameMessage->GetOffset(), signerNameMessage->GetLength());

    // We need the DNS header before appending the SIG RR.
    aDnsHeader.SetAdditionalRecordCount(aDnsHeader.GetAdditionalRecordCount() - 1);
    sha256.Update(aDnsHeader);
    sha256.Update(aMessage, offset + sizeof(aDnsHeader), aSigOffset - offset - sizeof(aDnsHeader));

    sha256.Finish(hash);

    signatureOffset = aSigRdataOffset + aSigRdataLength - Crypto::Ecdsa::P256::Signature::kSize;
    SuccessOrExit(error = aMessage.Read(signatureOffset, signature));

    error = aKey.GetKey().Verify(hash, signature);

exit:
    FreeMessage(signerNameMessage);
    return error;
}

void Server::HandleUpdate(const Dns::UpdateHeader &aDnsHeader, Host *aHost, const Ip6::MessageInfo &aMessageInfo)
{
    otError error = OT_ERROR_NONE;

    if (aHost->GetLease() == 0)
    {
        Host *existingHost = mHosts.FindMatching(aHost->GetFullName());

        aHost->ClearResources();

        // The client may not include all services it has registered and we should append
        // those services for current SRP update.
        if (existingHost != nullptr)
        {
            Service *existingService = nullptr;

            while ((existingService = existingHost->GetNextService(existingService)) != nullptr)
            {
                if (!existingService->mIsDeleted)
                {
                    Service *service = aHost->AddService(existingService->mFullName);
                    VerifyOrExit(service != nullptr, error = OT_ERROR_NO_BUFS);
                    service->mIsDeleted = true;
                }
            }
        }
    }

exit:
    if (error != OT_ERROR_NONE)
    {
        HandleSrpUpdateResult(error, aDnsHeader, *aHost, aMessageInfo);
    }
    else if (mAdvertisingHandler != nullptr)
    {
        UpdateMetadata *update = UpdateMetadata::New(GetInstance(), aDnsHeader, aHost, aMessageInfo);

        IgnoreError(mOutstandingUpdates.Add(*update));
        mOutstandingUpdatesTimer.StartAt(mOutstandingUpdates.GetTail()->GetExpireTime(), 0);

        mAdvertisingHandler(aHost, kDefaultEventsHandlerTimeout, mAdvertisingHandlerContext);
    }
    else
    {
        HandleSrpUpdateResult(OT_ERROR_NONE, aDnsHeader, *aHost, aMessageInfo);
    }
}

void Server::SendResponse(const Dns::UpdateHeader &   aHeader,
                          Dns::UpdateHeader::Response aResponseCode,
                          const Ip6::MessageInfo &    aMessageInfo)
{
    otError           error;
    Message *         response = nullptr;
    Dns::UpdateHeader header;

    response = mSocket.NewMessage(0);
    VerifyOrExit(response != nullptr, error = OT_ERROR_NO_BUFS);

    header.SetMessageId(aHeader.GetMessageId());
    header.SetType(Dns::UpdateHeader::kTypeResponse);
    header.SetQueryType(aHeader.GetQueryType());
    header.SetResponseCode(aResponseCode);
    SuccessOrExit(error = response->Append(header));

    SuccessOrExit(error = mSocket.SendTo(*response, aMessageInfo));

    if (aResponseCode != Dns::UpdateHeader::kResponseSuccess)
    {
        otLogInfoSrp("[server] send fail response: %d", aResponseCode);
    }
    else
    {
        otLogInfoSrp("[server] send success response");
    }

exit:
    if (error != OT_ERROR_NONE)
    {
        otLogWarnSrp("[server] failed to send response: %s", otThreadErrorToString(error));
        FreeMessage(response);
    }
}

void Server::SendResponse(const Dns::UpdateHeader &aHeader,
                          uint32_t                 aLease,
                          uint32_t                 aKeyLease,
                          const Ip6::MessageInfo & aMessageInfo)
{
    otError           error;
    Message *         response = nullptr;
    Dns::UpdateHeader header;
    Dns::OptRecord    optRecord;
    Dns::LeaseOption  leaseOption;

    response = mSocket.NewMessage(0);
    VerifyOrExit(response != nullptr, error = OT_ERROR_NO_BUFS);

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
    optRecord.SetLength(sizeof(Dns::LeaseOption));
    SuccessOrExit(error = response->Append(optRecord));

    leaseOption.Init();
    leaseOption.SetLeaseInterval(aLease);
    leaseOption.SetKeyLeaseInterval(aKeyLease);
    SuccessOrExit(error = response->Append(leaseOption));

    SuccessOrExit(error = mSocket.SendTo(*response, aMessageInfo));

    otLogInfoSrp("[server] send response with granted lease: %u and key lease: %u", aLease, aKeyLease);

exit:
    if (error != OT_ERROR_NONE)
    {
        otLogWarnSrp("[server] failed to send response: %s", otThreadErrorToString(error));
        FreeMessage(response);
    }
}

void Server::HandleUdpReceive(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
    static_cast<Server *>(aContext)->HandleUdpReceive(*static_cast<Message *>(aMessage),
                                                      *static_cast<const Ip6::MessageInfo *>(aMessageInfo));
}

void Server::HandleUdpReceive(Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    otError           error;
    Dns::UpdateHeader dnsHeader;
    uint16_t          offset = aMessage.GetOffset();

    SuccessOrExit(error = aMessage.Read(offset, dnsHeader));
    offset += sizeof(dnsHeader);

    // Handles only queries.
    VerifyOrExit(dnsHeader.GetType() == Dns::UpdateHeader::Type::kTypeQuery, error = OT_ERROR_DROP);

    switch (dnsHeader.GetQueryType())
    {
    case Dns::UpdateHeader::kQueryTypeUpdate:
        HandleDnsUpdate(aMessage, aMessageInfo, dnsHeader, offset);
        break;
    default:
        error = OT_ERROR_DROP;
        break;
    }

exit:
    if (error != OT_ERROR_NONE)
    {
        otLogInfoSrp("[server] failed to handle DNS message: %s", otThreadErrorToString(error));
    }
}

void Server::HandleLeaseTimer(Timer &aTimer)
{
    aTimer.Get<Server>().HandleLeaseTimer();
}

void Server::HandleLeaseTimer(void)
{
    TimeMilli now                = TimerMilli::GetNow();
    TimeMilli earliestExpireTime = now.GetDistantFuture();
    Host *    host               = mHosts.GetHead();

    while (host != nullptr)
    {
        Host *nextHost = host->GetNext();

        if (host->GetKeyExpireTime() <= now)
        {
            otLogInfoSrp("[server] KEY LEASE of host %s expired", host->GetFullName());

            // Removes the whole host and all services if the KEY RR expired.
            RemoveAndFreeHost(host);
        }
        else if (host->IsDeleted())
        {
            // The host has been deleted, but the hostname & service instance names retain.

            Service *service;

            earliestExpireTime = OT_MIN(earliestExpireTime, host->GetKeyExpireTime());

            // Check if any service instance name expired.
            service = host->GetNextService(nullptr);
            while (service != nullptr)
            {
                OT_ASSERT(service->mIsDeleted);

                Service *nextService = service->GetNext();

                if (service->GetKeyExpireTime() <= now)
                {
                    otLogInfoSrp("[server] KEY LEASE of service %s expired", service->mFullName);
                    host->RemoveAndFreeService(service);
                }
                else
                {
                    earliestExpireTime = OT_MIN(earliestExpireTime, service->GetKeyExpireTime());
                }

                service = nextService;
            }
        }
        else if (host->GetExpireTime() <= now)
        {
            Service *service = nullptr;

            otLogInfoSrp("[server] LEASE of host %s expired", host->GetFullName());

            // If the host expired, delete all resources of this host and its services.
            host->DeleteResourcesButRetainName();
            while ((service = host->GetNextService(service)) != nullptr)
            {
                service->DeleteResourcesButRetainName();
            }

            earliestExpireTime = OT_MIN(earliestExpireTime, host->GetKeyExpireTime());
        }
        else
        {
            // The host doesn't expire, check if any service expired or is explicitly removed.

            OT_ASSERT(!host->IsDeleted());

            Service *service = host->GetNextService(nullptr);

            earliestExpireTime = OT_MIN(earliestExpireTime, host->GetExpireTime());

            while (service != nullptr)
            {
                Service *nextService = service->GetNext();

                if (service->mIsDeleted)
                {
                    // The service has been deleted but the name retains.
                    earliestExpireTime = OT_MIN(earliestExpireTime, service->GetKeyExpireTime());
                }
                else if (service->GetExpireTime() <= now)
                {
                    otLogInfoSrp("[server] LEASE of service %s expired", service->mFullName);

                    // The service gets expired, delete it.
                    service->DeleteResourcesButRetainName();
                    earliestExpireTime = OT_MIN(earliestExpireTime, service->GetKeyExpireTime());
                }
                else
                {
                    earliestExpireTime = OT_MIN(earliestExpireTime, service->GetExpireTime());
                }

                service = nextService;
            }
        }

        host = nextHost;
    }

    if (earliestExpireTime != now.GetDistantFuture())
    {
        if (!mLeaseTimer.IsRunning() || earliestExpireTime <= mLeaseTimer.GetFireTime())
        {
            otLogInfoSrp("[server] lease timer is scheduled for %u seconds", (earliestExpireTime - now) / 1000);
            mLeaseTimer.StartAt(earliestExpireTime, 0);
        }
    }
    else
    {
        otLogInfoSrp("[server] lease timer is stopped");
        mLeaseTimer.Stop();
    }
}

void Server::HandleOutstandingUpdatesTimer(Timer &aTimer)
{
    aTimer.Get<Server>().HandleOutstandingUpdatesTimer();
}

void Server::HandleOutstandingUpdatesTimer(void)
{
    otLogInfoSrp("[server] outstanding service update timeout");
    while (!mOutstandingUpdates.IsEmpty() && mOutstandingUpdates.GetTail()->GetExpireTime() <= TimerMilli::GetNow())
    {
        HandleAdvertisingResult(mOutstandingUpdates.GetTail(), OT_ERROR_RESPONSE_TIMEOUT);
    }
}

Server::Service *Server::Service::New(const char *aFullName)
{
    void *   buf;
    Service *service = nullptr;

    buf = Instance::HeapCAlloc(1, sizeof(Service));
    VerifyOrExit(buf != nullptr);

    service = new (buf) Service();
    if (service->SetFullName(aFullName) != OT_ERROR_NONE)
    {
        service->Free();
        service = nullptr;
    }

exit:
    return service;
}

void Server::Service::Free(void)
{
    Instance::HeapFree(mFullName);
    Instance::HeapFree(mTxtData);
    Instance::HeapFree(this);
}

Server::Service::Service(void)
    : mFullName(nullptr)
    , mPriority(0)
    , mWeight(0)
    , mPort(0)
    , mTxtLength(0)
    , mTxtData(nullptr)
    , mHost(nullptr)
    , mNext(nullptr)
    , mTimeLastUpdate(TimerMilli::GetNow())
{
}

otError Server::Service::SetFullName(const char *aFullName)
{
    OT_ASSERT(aFullName != nullptr);

    otError error    = OT_ERROR_NONE;
    char *  nameCopy = static_cast<char *>(Instance::HeapCAlloc(1, strlen(aFullName) + 1));

    VerifyOrExit(nameCopy != nullptr, error = OT_ERROR_NO_BUFS);
    strcpy(nameCopy, aFullName);

    Instance::HeapFree(mFullName);
    mFullName = nameCopy;

exit:
    return error;
}

otError Server::Service::GetNextTxtEntry(Dns::TxtRecord::TxtIterator &aIterator, Dns::TxtEntry &aTxtEntry) const
{
    return Dns::TxtRecord::GetNextTxtEntry(mTxtData, mTxtLength, aIterator, aTxtEntry);
}

TimeMilli Server::Service::GetExpireTime(void) const
{
    OT_ASSERT(!mIsDeleted);
    OT_ASSERT(!GetHost().IsDeleted());

    return mTimeLastUpdate + Time::SecToMsec(GetHost().GetLease());
}

TimeMilli Server::Service::GetKeyExpireTime(void) const
{
    return mTimeLastUpdate + Time::SecToMsec(GetHost().GetKeyLease());
}

otError Server::Service::SetTxtData(const uint8_t *aTxtData, uint16_t aTxtDataLength)
{
    otError  error = OT_ERROR_NONE;
    uint8_t *txtData;

    txtData = static_cast<uint8_t *>(Instance::HeapCAlloc(1, aTxtDataLength));
    VerifyOrExit(txtData != nullptr, error = OT_ERROR_NO_BUFS);

    memcpy(txtData, aTxtData, aTxtDataLength);

    Instance::HeapFree(mTxtData);
    mTxtData   = txtData;
    mTxtLength = aTxtDataLength;

    // If a TXT RR is associated to this service, the service will retain.
    mIsDeleted = false;

exit:
    return error;
}

otError Server::Service::SetTxtDataFromMessage(const Message &aMessage, uint16_t aOffset, uint16_t aLength)
{
    otError  error = OT_ERROR_NONE;
    uint8_t *txtData;

    txtData = static_cast<uint8_t *>(Instance::HeapCAlloc(1, aLength));
    VerifyOrExit(txtData != nullptr, error = OT_ERROR_NO_BUFS);
    VerifyOrExit(aMessage.ReadBytes(aOffset, txtData, aLength) == aLength, error = OT_ERROR_PARSE);
    VerifyOrExit(Dns::TxtRecord::VerifyTxtData(txtData, aLength), error = OT_ERROR_PARSE);

    Instance::HeapFree(mTxtData);
    mTxtData   = txtData;
    mTxtLength = aLength;

    mIsDeleted = false;

exit:
    if (error != OT_ERROR_NONE)
    {
        Instance::HeapFree(txtData);
    }

    return error;
}

void Server::Service::ClearResources(void)
{
    mPort = 0;
    Instance::HeapFree(mTxtData);
    mTxtData   = nullptr;
    mTxtLength = 0;
}

void Server::Service::DeleteResourcesButRetainName(void)
{
    mIsDeleted = true;
    ClearResources();
    otLogInfoSrp("[server] remove service '%s' (but retain its name)", mFullName);
}

otError Server::Service::CopyResourcesFrom(const Service &aService)
{
    otError error;

    SuccessOrExit(error = SetTxtData(aService.mTxtData, aService.mTxtLength));
    mPriority = aService.mPriority;
    mWeight   = aService.mWeight;
    mPort     = aService.mPort;

    mIsDeleted      = false;
    mTimeLastUpdate = TimerMilli::GetNow();

exit:
    return error;
}

bool Server::Service::Matches(const char *aFullName) const
{
    return (mFullName != nullptr) && (strcmp(mFullName, aFullName) == 0);
}

bool Server::Service::MatchesServiceName(const char *aServiceName) const
{
    uint8_t i = static_cast<uint8_t>(strlen(mFullName));
    uint8_t j = static_cast<uint8_t>(strlen(aServiceName));

    while (i > 0 && j > 0 && mFullName[i - 1] == aServiceName[j - 1])
    {
        i--;
        j--;
    }

    return j == 0 && i > 0 && mFullName[i - 1] == '.';
}

Server::Host *Server::Host::New(void)
{
    void *buf;
    Host *host = nullptr;

    buf = Instance::HeapCAlloc(1, sizeof(Host));
    VerifyOrExit(buf != nullptr);

    host = new (buf) Host();

exit:
    return host;
}

void Server::Host::Free(void)
{
    RemoveAndFreeAllServices();
    Instance::HeapFree(mFullName);
    Instance::HeapFree(this);
}

Server::Host::Host(void)
    : mFullName(nullptr)
    , mAddressesNum(0)
    , mNext(nullptr)
    , mLease(0)
    , mKeyLease(0)
    , mTimeLastUpdate(TimerMilli::GetNow())
{
    mKey.Clear();
}

otError Server::Host::SetFullName(const char *aFullName)
{
    OT_ASSERT(aFullName != nullptr);

    otError error    = OT_ERROR_NONE;
    char *  nameCopy = static_cast<char *>(Instance::HeapCAlloc(1, strlen(aFullName) + 1));

    VerifyOrExit(nameCopy != nullptr, error = OT_ERROR_NO_BUFS);
    strcpy(nameCopy, aFullName);

    if (mFullName != nullptr)
    {
        Instance::HeapFree(mFullName);
    }
    mFullName = nameCopy;

exit:
    return error;
}

void Server::Host::SetKey(Dns::Ecdsa256KeyRecord &aKey)
{
    OT_ASSERT(aKey.IsValid());

    mKey = aKey;
}

void Server::Host::SetLease(uint32_t aLease)
{
    mLease = aLease;
}

void Server::Host::SetKeyLease(uint32_t aKeyLease)
{
    mKeyLease = aKeyLease;
}

TimeMilli Server::Host::GetExpireTime(void) const
{
    OT_ASSERT(!IsDeleted());

    return mTimeLastUpdate + Time::SecToMsec(mLease);
}

TimeMilli Server::Host::GetKeyExpireTime(void) const
{
    return mTimeLastUpdate + Time::SecToMsec(mKeyLease);
}

// Add a new service entry to the host, do nothing if there is already
// such services with the same name.
Server::Service *Server::Host::AddService(const char *aFullName)
{
    Service *service = FindService(aFullName);

    VerifyOrExit(service == nullptr);

    service = Service::New(aFullName);
    if (service != nullptr)
    {
        IgnoreError(mServices.Add(*service));
        service->mHost = this;
    }

exit:
    return service;
}

void Server::Host::RemoveAndFreeService(Service *aService)
{
    if (aService != nullptr)
    {
        IgnoreError(mServices.Remove(*aService));
        aService->Free();
    }
}

void Server::Host::RemoveAndFreeAllServices(void)
{
    while (!mServices.IsEmpty())
    {
        RemoveAndFreeService(mServices.GetHead());
    }
}

void Server::Host::ClearResources(void)
{
    mAddressesNum = 0;
}

void Server::Host::DeleteResourcesButRetainName(void)
{
    // Mark the host as deleted.
    mLease = 0;
    ClearResources();
    otLogInfoSrp("[server] remove host '%s' (but retain its name)", mFullName);
}

void Server::Host::CopyResourcesFrom(const Host &aHost)
{
    memcpy(mAddresses, aHost.mAddresses, aHost.mAddressesNum * sizeof(mAddresses[0]));
    mAddressesNum = aHost.mAddressesNum;
    mKey          = aHost.mKey;
    mLease        = aHost.mLease;
    mKeyLease     = aHost.mKeyLease;

    mTimeLastUpdate = TimerMilli::GetNow();
}

Server::Service *Server::Host::FindService(const char *aFullName)
{
    return mServices.FindMatching(aFullName);
}

const Server::Service *Server::Host::FindService(const char *aFullName) const
{
    return const_cast<Host *>(this)->FindService(aFullName);
}

otError Server::Host::AddIp6Address(const Ip6::Address &aIp6Address)
{
    otError error = OT_ERROR_NONE;

    if (aIp6Address.IsMulticast() || aIp6Address.IsUnspecified() || aIp6Address.IsLoopback())
    {
        // We don't like those address because they cannot be used
        // for communication with exterior devices.
        ExitNow(error = OT_ERROR_DROP);
    }

    for (const Ip6::Address &addr : mAddresses)
    {
        if (aIp6Address == addr)
        {
            // Drop duplicate addresses.
            ExitNow(error = OT_ERROR_DROP);
        }
    }

    if (mAddressesNum >= kMaxAddressesNum)
    {
        otLogWarnSrp("[server] too many addresses for host %s", GetFullName());
        ExitNow(error = OT_ERROR_NO_BUFS);
    }

    mAddresses[mAddressesNum++] = aIp6Address;

exit:
    return error;
}

bool Server::Host::Matches(const char *aName) const
{
    return mFullName != nullptr && strcmp(mFullName, aName) == 0;
}

Server::UpdateMetadata *Server::UpdateMetadata::New(Instance &               aInstance,
                                                    const Dns::UpdateHeader &aHeader,
                                                    Host *                   aHost,
                                                    const Ip6::MessageInfo & aMessageInfo)
{
    void *          buf;
    UpdateMetadata *update = nullptr;

    buf = aInstance.HeapCAlloc(1, sizeof(UpdateMetadata));
    VerifyOrExit(buf != nullptr);

    update = new (buf) UpdateMetadata(aInstance, aHeader, aHost, aMessageInfo);

exit:
    return update;
}

void Server::UpdateMetadata::Free(void)
{
    Instance::HeapFree(this);
}

Server::UpdateMetadata::UpdateMetadata(Instance &               aInstance,
                                       const Dns::UpdateHeader &aHeader,
                                       Host *                   aHost,
                                       const Ip6::MessageInfo & aMessageInfo)
    : InstanceLocator(aInstance)
    , mExpireTime(TimerMilli::GetNow() + kDefaultEventsHandlerTimeout)
    , mDnsHeader(aHeader)
    , mHost(aHost)
    , mMessageInfo(aMessageInfo)
    , mNext(nullptr)
{
}

} // namespace Srp
} // namespace ot

#endif // OPENTHREAD_CONFIG_SRP_SERVER_ENABLE
