/*
 *  Copyright (c) 2021, The OpenThread Authors.
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

#include "srp_replication.hpp"

#if OPENTHREAD_CONFIG_SRP_REPLICATION_ENABLE

#include "common/as_core_type.hpp"
#include "common/code_utils.hpp"
#include "common/debug.hpp"
#include "common/encoding.hpp"
#include "common/heap.hpp"
#include "common/instance.hpp"
#include "common/locator_getters.hpp"
#include "common/logging.hpp"
#include "common/numeric_limits.hpp"
#include "common/random.hpp"
#include "common/serial_number.hpp"
#include "common/string.hpp"
#include "utils/parse_cmdline.hpp"

/**
 * @file
 *   This file implements the SRP Replication Protocol (SRPL).
 */

namespace ot {

using Encoding::BigEndian::HostSwap32;
using Utils::CmdLineParser::ParseAsUint32;

namespace Srp {

extern "C" void otPlatSrplHandleDnssdBrowseResult(otInstance *aInstance, const otPlatSrplPartnerInfo *aPartnerInfo)
{
    AsCoreType(aInstance).Get<Srpl>().HandleDnssdBrowseResult(*static_cast<const Srpl::PartnerInfo *>(aPartnerInfo));
}

//---------------------------------------------------------------------------------------------------------------------
// `Srpl`

const char    Srpl::kTxtDataKeyDomain[]     = "domain";
const char    Srpl::kTxtDataKeyAllowsJoin[] = "join";
const char    Srpl::kTxtDataKeyPeerId[]     = "prec";
const char    Srpl::kDefaultDomainName[]    = "openthread.local.";
const uint8_t Srpl::kYesString[]            = {'y', 'e', 's'};
const uint8_t Srpl::kNoString[]             = {'n', 'o'};

Srpl::Srpl(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mState(kStateDisabled)
    , mTimer(aInstance, HandleTimer)
    , mPrimarySession(nullptr)
    , mConnectionCallbacks(HandleConnected,
                           HandleSessionEstablished,
                           HandleDisconnected,
                           ProcessRequestMessage,
                           ProcessUnidirectionalMessage,
                           ProcessResponseMessage)

{
    SuccessOrAssert(mDefaultDomainName.Set(kDefaultDomainName));
}

Error Srpl::SetEnabled(bool aEnable)
{
    Error error = kErrorNone;

    VerifyOrExit(aEnable != IsEnabled());

    otLogInfoSrp("[srpl] %s SRPL", aEnable ? "Enabling" : "Disabling");

    if (aEnable)
    {
        VerifyOrExit(Get<Server>().GetState() == Server::kStateDisabled, error = kErrorInvalidState);
        IgnoreError(Get<Server>().SetAddressMode(Server::kAddressModeAnycast));
        Start();
    }
    else
    {
        Stop();
    }

exit:
    return error;
}

Error Srpl::SetDomain(const char *aName)
{
    Error error = kErrorNone;

    VerifyOrExit(mState == kStateDisabled, error = kErrorInvalidState);
    SuccessOrExit(error = mDomainName.Set(aName));

    if (aName == nullptr)
    {
        otLogInfoSrp("[srpl] Domain name cleared");
    }
    else
    {
        otLogInfoSrp("[srpl] Domain name set to \"%s\"", aName);
    }

exit:
    return error;
}

Error Srpl::SetDefaultDomain(const char *aName)
{
    Error error = kErrorNone;

    OT_ASSERT(aName != nullptr);

    VerifyOrExit(mState == kStateDisabled, error = kErrorInvalidState);
    SuccessOrExit(error = mDefaultDomainName.Set(aName));
    otLogInfoSrp("[srpl] Default domain name set to \"%s\"", aName);

exit:
    return error;
}

Error Srpl::GetId(uint32_t &aId) const
{
    Error error = kErrorNone;

    VerifyOrExit(mPeerId.HasId(), error = kErrorNotFound);
    aId = mPeerId.GetId();

exit:
    return error;
}

Error Srpl::GetNextPartner(Partner::Iterator &aIterator, Partner &aPartner) const
{
    Error          error   = kErrorNone;
    const Session *session = static_cast<const Session *>(aIterator.mData);

    session = (session == nullptr) ? mSessions.GetHead() : session->GetNext();
    VerifyOrExit(session != nullptr, error = kErrorNotFound);

    aPartner.SetFrom(*session);
    aIterator.mData = session;

exit:
    return error;
}

void Srpl::SetState(State aState)
{
    VerifyOrExit(mState != aState);
    otLogInfoSrp("[srpl] State: %s -> %s", StateToString(mState), StateToString(aState));
    mState = aState;

exit:
    return;
}

void Srpl::Start(void)
{
    uint32_t interval;

    DnssdBrowse(/* aEnable */ true);
    interval = Random::NonCrypto::GetUint32InRange(kDiscoveryMinInterval, kDiscoveryMaxInterval);
    mTimer.Start(interval);
    otLogInfoSrp("[srpl] Starting DNS-SD browse - partner discovery for %u msec", interval);

    Get<Dns::Dso>().StartListening(AcceptConnection);

    SetState(kStateDiscovery);
}

void Srpl::Stop(void)
{
    Get<Dns::Dso>().StopListening();
    DnssdBrowse(/* aEnable */ false);
    UnregisterDnssdService();
    mTimer.Stop();
    mSessions.Free();
    mPeerId.Clear();
    SetState(kStateDisabled);
    Get<Server>().Disable();
}

void Srpl::AddPartner(const PartnerInfo &aInfo)
{
    Error        error = kErrorNone;
    Heap::String domainName;
    PeerId       partnerId;
    bool         allowsJoin;
    Session *    session;

    OT_ASSERT(!aInfo.mRemoved);

    error = aInfo.ParseTxtData(domainName, partnerId, allowsJoin);

    if (error != kErrorNone)
    {
        otLogInfoSrp("[srpl] Error %s parsing TXT data from %s", ErrorToString(error),
                     aInfo.GetSockAddr().ToString().AsCString());
        ExitNow();
    }

    otLogInfoSrp("[srpl] Discovered partner %s", aInfo.GetSockAddr().ToString().AsCString());
    otLogInfoSrp("[srpl]   domain:\"%s\", id:%u, allow-join:%s", domainName.AsCString(), partnerId.GetId(),
                 allowsJoin ? "yes" : "no");

    session = mSessions.FindMatching(aInfo.GetSockAddr());

    if (session != nullptr)
    {
        if (session->GetPartnerId().HasId() && (session->GetPartnerId() != partnerId))
        {
            otLogInfoSrp("[srpl] Mismatched id %u for %s", partnerId.GetId(), session->ToString().AsCString());

            session->SetPartnerId(partnerId);
            session->Disconnect(Connection::kForciblyAbort, Connection::kReasonPeerMisbehavior);
            session->HandleError();
            ExitNow();
        }

        session->SetPartnerId(partnerId);

        if (session->IsMarkedToRemove())
        {
            session->MarkToRemove(false);
            otLogInfoSrp("[srpl] Re-added partner %s", session->ToString().AsCString());

            if ((mState == kStateRunning) && (GetPeerId() > session->GetPartnerId()))
            {
                session->Connect();
            }
        }

        ExitNow();
    }

    VerifyOrExit(allowsJoin);

    switch (mState)
    {
    case kStateDiscovery:
        if (mDomainName.IsNull())
        {
            // We accept any domain so adopt the domain name from partner.
            SuccessOrExit(mDomainName.Set(static_cast<Heap::String &&>(domainName)));
            break;
        }

        // Fall through to match the domain name from partner with `mDomainName`.
        OT_FALL_THROUGH;

    case kStateAcquireId:
    case kStatePrimaryPartnerSync:
    case kStateRunning:
        VerifyOrExit(StringMatch(mDomainName.AsCString(), domainName.AsCString(), kStringCaseInsensitiveMatch));
        break;

    case kStateDisabled:
        ExitNow();
    }

    if (mPeerId.HasId() && (mPeerId == partnerId))
    {
        otLogInfoSrp("[srpl] Id conflict - %u assigned to us and %s - restarting SRPL", mPeerId.GetId(),
                     aInfo.GetSockAddr().ToString().AsCString());
        Stop();
        Start();
        ExitNow();
    }

    session = Session::Allocate(GetInstance(), aInfo.GetSockAddr(), partnerId);
    VerifyOrExit(session != nullptr);

    mSessions.Push(*session);
    otLogInfoSrp("[srpl] Added new partner %s", session->ToString().AsCString());

    // We try to connect and establish session (as client) with
    // the new partner if our ID is larger than partner's ID.

    if ((mState == kStateRunning) && (GetPeerId() > session->GetPartnerId()))
    {
        session->Connect();
    }

exit:
    return;
}

void Srpl::RemovePartner(const PartnerInfo &aInfo)
{
    Session *session;

    OT_ASSERT(aInfo.mRemoved);

    switch (mState)
    {
    case kStateDisabled:
        break;

    case kStateDiscovery:
        // While in discovery phase, we can immediately remove the
        // partner. Since `mSessions` is an `OwnlingList`, removing
        // the entry from it without assigning it to an `OwnedPtr`
        // will automatically free the entry.

        if (mSessions.RemoveMatching(aInfo.GetSockAddr()) != nullptr)
        {
            otLogInfoSrp("[srpl] Removed partner %s", aInfo.GetSockAddr().ToString().AsCString());
        }
        break;

    case kStateAcquireId:
    case kStatePrimaryPartnerSync:
    case kStateRunning:
        session = mSessions.FindMatching(aInfo.GetSockAddr());
        VerifyOrExit(session != nullptr);
        session->MarkToRemove(true);
        break;
    }

exit:
    return;
}

void Srpl::AcquireId(void)
{
    if (mDomainName.IsNull())
    {
        SuccessOrAssert(mDomainName.Set(mDefaultDomainName));
        otLogInfoSrp("[srpl] no domain discovered, using default domain: %s", mDomainName.AsCString());
    }

    SetState(kStateAcquireId);
    SelectPrimaryPartner();
}

void Srpl::SelectPrimaryPartner(void)
{
    // We go through all discovered partners and select the one with
    // smallest ID as `mPrimarySession`. If we do not yet have an ID,
    // we ask the primary partner to assign an ID to us. After the
    // primary partner assigns an ID to us, we start establishing
    // session with other discovered partners. However, we always
    // wait till we successfully finish the initial sync with the
    // primary partner before starting SRP server and advertising
    // SRPL service using DNS-SD.
    //
    // If the session with the primary partner is disconnected
    // (error/misbehavior), we try to select another primary
    // partner.

    OT_ASSERT((mState == kStateAcquireId) || (mState == kStatePrimaryPartnerSync));

    mPrimarySession = nullptr;

    for (Session &session : mSessions)
    {
        if (!session.GetPartnerId().HasId() || session.IsMarkedToRemove() || session.IsMarkedAsErrored())
        {
            // Skip over any session that is marked to be removed,
            // or has misbehaved earlier.
            continue;
        }

        if (mPeerId.HasId() && (mPeerId < session.GetPartnerId()))
        {
            // If we have an ID, we skip over any partner with
            // larger ID than us.
            continue;
        }

        if ((mPrimarySession == nullptr) || (session.GetPartnerId() < mPrimarySession->GetPartnerId()))
        {
            mPrimarySession = &session;
        }
    }

    if (mPrimarySession != nullptr)
    {
        otLogInfoSrp("[srpl] Selected partner %d as primary", mPrimarySession->GetPartnerId().GetId());

        switch (mPrimarySession->GetPhase())
        {
        case Session::kToSync:
            // If already connecting, calling `Connect` again does nothing.
            mPrimarySession->Connect();
            break;

        case Session::kEstablishingSession:
        case Session::kSyncCandidatesFromPartner:
        case Session::kSendCandidatesToPartner:
            // Wait for the end of initial sync.
            break;

        case Session::kRoutineOperation:
            // We already finished initial sync with the  primary
            // partner, so we can start SRP server and advertise
            // SRPL DNS-SD service.
            mPrimarySession = nullptr;
            StartSrpServer();
            break;
        }
    }
    else
    {
        // If there is no primary partner, we can start SRP server
        // immediately. If we have not yet acquired an ID, we pick
        // one randomly selected from a range.

        if (mState == kStateAcquireId)
        {
            mPeerId.SetId(Random::NonCrypto::GetUint32InRange(0, kSelfSelectedPeerIdWindow));
            otLogInfoSrp("[srpl] No partner discovered, selected id %u", mPeerId.GetId());
        }

        StartSrpServer();
    }
}

void Srpl::StartSrpServer(void)
{
    static constexpr uint16_t kTxtDataSize   = 100 + Dns::Name::kMaxNameSize;
    static constexpr uint8_t  kNumTxtEntries = 3;

    uint8_t                        txtDataBuffer[kTxtDataSize];
    MutableData<kWithUint16Length> txtData;
    Dns::TxtEntry                  entries[kNumTxtEntries];
    String<kPeerIdStringSize>      idString;

    // Prepare the TXT record data to register SRPL DNS-SD service.

    txtData.Init(txtDataBuffer, sizeof(txtDataBuffer));

    entries[0].mKey         = kTxtDataKeyDomain;
    entries[0].mValue       = reinterpret_cast<const uint8_t *>(mDomainName.AsCString());
    entries[0].mValueLength = StringLength(mDomainName.AsCString(), Dns::Name::kMaxNameSize);

    entries[1].mKey         = kTxtDataKeyAllowsJoin;
    entries[1].mValue       = kYesString;
    entries[1].mValueLength = sizeof(kYesString);

    idString.Append("%u", mPeerId.GetId());
    entries[2].mKey         = kTxtDataKeyPeerId;
    entries[2].mValue       = reinterpret_cast<const uint8_t *>(idString.AsCString());
    entries[2].mValueLength = idString.GetLength();

    SuccessOrAssert(Dns::TxtEntry::AppendEntries(entries, kNumTxtEntries, txtData));
    RegisterDnssdService(txtData.GetBytes(), txtData.GetLength());

    otLogInfoSrp("[srpl] Started advertising DNS-SD SRPL service, domain:\"%s\", id:%u, allow-join:yes",
                 mDomainName.AsCString(), mPeerId.GetId());

    SetState(kStateRunning);

    Get<Server>().Enable();
}

void Srpl::HandleTimer(Timer &aTimer)
{
    aTimer.Get<Srpl>().HandleTimer();
}

void Srpl::HandleTimer(void)
{
    TimeMilli           now = TimerMilli::GetNow();
    TimeMilli           nextFireTime;
    OwningList<Session> removedSessions;

    switch (mState)
    {
    case kStateDisabled:
        break;

    case kStateDiscovery:
        AcquireId();
        break;

    case kStateAcquireId:
    case kStatePrimaryPartnerSync:
    case kStateRunning:
        // Remove all sessions that are marked to be removed and expired
        mSessions.RemoveAllMatching(Session::RemoveTime(now), removedSessions);

        for (const Session &session : removedSessions)
        {
            OT_UNUSED_VARIABLE(session);
            otLogInfoSrp("[srpl] Removed partner %s after timeout", session.ToString().AsCString());
        }

        nextFireTime = now.GetDistantFuture();

        for (Session &session : mSessions)
        {
            session.HandleTimer(now, nextFireTime);
        }

        if (nextFireTime != now.GetDistantFuture())
        {
            mTimer.FireAtIfEarlier(nextFireTime);
        }

        break;
    }
}

const char *Srpl::StateToString(State aState)
{
    static const char *const kStateStrings[] = {
        "Disabled",           // (0) kStateDisabled
        "Discovery",          // (1) kStateDiscovery
        "AcquireId",          // (2) kStateAcquireId
        "PrimaryPartnerSync", // (3) kStatePrimaryPartnerSync
        "Running",            // (4) kStateRunning
    };

    static_assert(0 == kStateDisabled, "kStateDisabled value is incorrect");
    static_assert(1 == kStateDiscovery, "kStateDiscovery value is incorrect");
    static_assert(2 == kStateAcquireId, "kStateAcquireId value is incorrect");
    static_assert(3 == kStatePrimaryPartnerSync, "kStatePrimaryPartnerSync value is incorrect");
    static_assert(4 == kStateRunning, "kStateRunning value is incorrect");

    return kStateStrings[aState];
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// `otPlatSrpl` methods and callbacks

void Srpl::DnssdBrowse(bool aEnable)
{
    otPlatSrplDnssdBrowse(&GetInstance(), aEnable);
}

void Srpl::HandleDnssdBrowseResult(const PartnerInfo &aInfo)
{
    if (!aInfo.mRemoved)
    {
        AddPartner(aInfo);
    }
    else
    {
        RemovePartner(aInfo);
    }
}

void Srpl::RegisterDnssdService(const uint8_t *aTxtData, uint16_t aTxtLength)
{
    otPlatSrplRegisterDnssdService(&GetInstance(), aTxtData, aTxtLength);
}

void Srpl::UnregisterDnssdService(void)
{
    otPlatSrplUnregisterDnssdService(&GetInstance());
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Methods called from `Srp::Server`

void Srpl::SendUpdateMessageToPartners(const RetainPtr<UpdateMessage> &aMessagePtr)
{
    // This is called by `Srp::Server` when an SRP Update message is
    // directly received from a client.

    for (Session &session : mSessions)
    {
        session.SendUpdateMessage(aMessagePtr);
    }
}

void Srpl::HandleServerRemovingHost(const Server::Host &aHost)
{
    // This is called by `Srp::Server` when it is about to
    // fully remove a host entry.

    for (Session &session : mSessions)
    {
        session.HandleServerRemovingHost(aHost);
    }
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Callbacks from `Dso`

Srpl::Connection *Srpl::AcceptConnection(Instance &aInstance, const Ip6::SockAddr &aPeerSockAddr)
{
    return aInstance.Get<Srpl>().AcceptConnection(aPeerSockAddr);
}

Srpl::Connection *Srpl::AcceptConnection(const Ip6::SockAddr &aPeerSockAddr)
{
    Session *session = nullptr;

    VerifyOrExit(mState == kStateRunning);

    session = mSessions.FindMatching(aPeerSockAddr);

    if (session != nullptr)
    {
        if (session->GetState() != Connection::kStateDisconnected)
        {
            otLogInfoSrp("[srpl] AcceptConnection - Already connected to %s", aPeerSockAddr.ToString().AsCString());
            session = nullptr;
            ExitNow();
        }

        if (session->IsMarkedAsErrored())
        {
            // The session is marked as "errored" while we are in
            // reconnect interval and we then reject new connection
            // requests from partner.

            otLogInfoSrp("[srpl] AcceptConnection - Earlier session with %s errored - still in reconnect interval",
                         aPeerSockAddr.ToString().AsCString());
            session = nullptr;
        }

        ExitNow();
    }

    session = Session::Allocate(GetInstance(), aPeerSockAddr);
    VerifyOrExit(session != nullptr);
    mSessions.Push(*session);

    otLogInfoSrp("[srpl] Accepted connection request from %s", aPeerSockAddr.ToString().AsCString());

exit:
    return session;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Callbacks from `Dso::Connection`

void Srpl::HandleConnected(Connection &aConnection)
{
    static_cast<Session &>(aConnection).HandleConnected();
}

void Srpl::HandleSessionEstablished(Connection &aConnection)
{
    static_cast<Session &>(aConnection).HandleSessionEstablished();
}

void Srpl::HandleDisconnected(Connection &aConnection)
{
    static_cast<Session &>(aConnection).HandleDisconnected();
}

Error Srpl::ProcessRequestMessage(Connection &          aConnection,
                                  Connection::MessageId aMessageId,
                                  const Message &       aMessage,
                                  Tlv::Type             aPrimaryTlvType)
{
    return static_cast<Session &>(aConnection).ProcessRequestMessage(aMessageId, aMessage, aPrimaryTlvType);
}

Error Srpl::ProcessUnidirectionalMessage(Connection &aConnection, const Message &aMessage, Tlv::Type aPrimaryTlvType)
{
    return static_cast<Session &>(aConnection).ProcessUnidirectionalMessage(aMessage, aPrimaryTlvType);
}

Error Srpl::ProcessResponseMessage(Connection &       aConnection,
                                   const Dns::Header &aHeader,
                                   const Message &    aMessage,
                                   Tlv::Type          aResponseTlvType,
                                   Tlv::Type          aRequestTlvType)
{
    return static_cast<Session &>(aConnection)
        .ProcessResponseMessage(aHeader, aMessage, aResponseTlvType, aRequestTlvType);
}

//---------------------------------------------------------------------------------------------------------------------
// `Srpl::Partner`

void Srpl::Partner::SetFrom(const Session &aSession)
{
    SessionState state = kSessionDisconnected;

    mSockAddr = aSession.GetPeerSockAddr();
    mHasId    = aSession.GetPartnerId().HasId();

    if (mHasId)
    {
        mId = aSession.GetPartnerId().GetId();
    }

    // Map the DSO connection state and SRPL phase to
    // `Partner::SessionState`.

    if (aSession.IsMarkedAsErrored())
    {
        state = kSessionErrored;
    }
    else
    {
        switch (aSession.GetState())
        {
        case Connection::kStateDisconnected:
            break;

        case Connection::kStateConnecting:
            state = kSessionConnecting;
            break;

        case Connection::kStateConnectedButSessionless:
        case Connection::kStateEstablishingSession:
            state = kSessionEstablishing;
            break;

        case Connection::kStateSessionEstablished:
            switch (aSession.GetPhase())
            {
            case Session::kToSync:
            case Session::kEstablishingSession:
                state = kSessionEstablishing;
                break;
            case Session::kSyncCandidatesFromPartner:
            case Session::kSendCandidatesToPartner:
                state = kSessionInitialSync;
                break;
            case Session::kRoutineOperation:
                state = kSessionRoutineOperation;
                break;
            }
            break;
        }
    }

    mSessionState = MapEnum(state);
}

//---------------------------------------------------------------------------------------------------------------------
// `Srpl::PeerId`

void Srpl::PeerId::SetId(uint32_t aId)
{
    mHasId = true;
    mId    = aId;
}

uint32_t &Srpl::PeerId::UpdateId(void)
{
    mHasId = true;
    return mId;
}

uint32_t Srpl::PeerId::GetId(void) const
{
    OT_ASSERT(mHasId);
    return mId;
}

bool Srpl::PeerId::operator==(uint32_t aId) const
{
    OT_ASSERT(mHasId);
    return (mId == aId);
}

bool Srpl::PeerId::operator==(const PeerId &aOther) const
{
    OT_ASSERT(mHasId);
    OT_ASSERT(aOther.mHasId);

    return (mId == aOther.mId);
}

bool Srpl::PeerId::operator<(const PeerId &aOther) const
{
    OT_ASSERT(mHasId);
    OT_ASSERT(aOther.mHasId);

    return SerialNumber::IsLess(mId, aOther.mId);
}

//---------------------------------------------------------------------------------------------------------------------
// `Srpl::PartnerInfo`

Error Srpl::PartnerInfo::ParseTxtData(Heap::String &aDomainName, PeerId &aPeerId, bool &aAllowsJoin) const
{
    Error                   error;
    Dns::TxtEntry           entry;
    Dns::TxtEntry::Iterator iterator;
    bool                    parsedDomain     = false;
    bool                    parsedAllowsJoin = false;

    iterator.Init(mTxtData, mTxtLength);

    aPeerId.Clear();

    while ((error = iterator.GetNextEntry(entry)) == kErrorNone)
    {
        if (strcmp(entry.mKey, kTxtDataKeyDomain) == 0)
        {
            char name[Dns::Name::kMaxNameSize];

            VerifyOrExit(!parsedDomain, error = kErrorParse);

            VerifyOrExit(entry.mValueLength < sizeof(name), error = kErrorParse);
            memcpy(name, entry.mValue, entry.mValueLength);
            name[entry.mValueLength] = kNullChar;

            SuccessOrExit(error = aDomainName.Set(name));
            parsedDomain = true;
        }
        else if (strcmp(entry.mKey, kTxtDataKeyPeerId) == 0)
        {
            char string[kPeerIdStringSize];

            VerifyOrExit(!aPeerId.HasId(), error = kErrorParse);

            VerifyOrExit(entry.mValueLength < sizeof(string), error = kErrorParse);
            memcpy(string, entry.mValue, entry.mValueLength);
            string[entry.mValueLength] = kNullChar;

            SuccessOrExit(error = ParseAsUint32(string, aPeerId.UpdateId()));
        }
        else if (strcmp(entry.mKey, kTxtDataKeyAllowsJoin) == 0)
        {
            VerifyOrExit(!parsedAllowsJoin, error = kErrorParse);

            if (entry.mValueLength == sizeof(char))
            {
                switch (entry.mValue[0])
                {
                case 'y':
                case '1':
                    aAllowsJoin = true;
                    break;

                case 'n':
                case '0':
                    aAllowsJoin = false;
                    break;

                default:
                    ExitNow(error = kErrorParse);
                }
            }
            else if ((entry.mValueLength == sizeof(kNoString)) &&
                     (memcmp(entry.mValue, kNoString, sizeof(kNoString)) == 0))
            {
                aAllowsJoin = false;
            }
            else if ((entry.mValueLength == sizeof(kYesString)) &&
                     (memcmp(entry.mValue, kYesString, sizeof(kYesString)) == 0))
            {
                aAllowsJoin = true;
            }
            else
            {
                ExitNow(error = kErrorParse);
            }

            parsedAllowsJoin = true;
        }

        // Skip over and ignore any unknown keys.
    }

    VerifyOrExit(error == kErrorNotFound);
    error = kErrorNone;

    VerifyOrExit(parsedDomain && aPeerId.HasId() && parsedAllowsJoin, error = kErrorParse);

exit:
    return error;
}

//---------------------------------------------------------------------------------------------------------------------
// Srpl::Tlv

Error Srpl::Tlv::ReadFrom(const Message &aMessage, uint16_t aOffset)
{
    // Reads the `Tlv` from `aMessage` at the given `aOffset` and
    // validates that entire TLV (including its value) is present
    // in `aMessage`.

    Error error;

    SuccessOrExit(error = aMessage.Read(aOffset, *this));
    VerifyOrExit(GetSize() + static_cast<uint32_t>(aOffset) <= NumericLimits<uint16_t>::kMax, error = kErrorParse);
    VerifyOrExit(aOffset + GetSize() <= aMessage.GetLength(), error = kErrorParse);

exit:
    return error;
}

bool Srpl::Tlv::IsUnrecognizedOrPaddingTlv(Type aType)
{
    bool isUnrecognizedOrPadding = true;

    switch (aType)
    {
    // Common DSO TLVs
    case kReservedType:
    case kKeepAliveType:
    case kRetryDelayType:
        OT_FALL_THROUGH;

    // SRPL TLVs
    case kSessionType:
    case kSendCandidatesType:
    case kCandidateType:
    case kHostType:
    case kCandidateYesType:
    case kCandidateNoType:
    case kConflictType:
    case kHostnameType:
    case kHostMessageType:
    case kTimeOffsetType:
    case kKeyIdType:
        isUnrecognizedOrPadding = false;
        break;

    case kEncryptionPaddingType:
    default:
        break;
    }

    return isUnrecognizedOrPadding;
}

Error Srpl::Tlv::WriteUint32Value(Message &aMessage, uint32_t aValue)
{
    uint32_t value = HostSwap32(aValue);

    return aMessage.Append(value);
}

Error Srpl::Tlv::ReadUint32Value(const Message &aMessage, uint16_t aOffset, uint32_t &aValue)
{
    Error error;

    SuccessOrExit(error = aMessage.Read(aOffset, aValue));
    aValue = HostSwap32(aValue);

exit:
    return error;
}

Error Srpl::Tlv::AppendUint32Tlv(Message &aMessage, Type aType, uint32_t aValue)
{
    Error error;

    SuccessOrExit(error = aMessage.Append(Tlv(aType, sizeof(uint32_t))));
    error = WriteUint32Value(aMessage, aValue);

exit:
    return error;
}

Srpl::Tlv::InfoString Srpl::Tlv::TypeToString(Type aType)
{
    InfoString string;

    switch (aType)
    {
    // Common DSO TLVs:
    case kReservedType:
        string.Append("Reserved");
        break;
    case kKeepAliveType:
        string.Append("KeepAlive");
        break;
    case kRetryDelayType:
        string.Append("RetryDelay");
        break;
    case kEncryptionPaddingType:
        string.Append("EncryptionPadding");
        break;

    // SRPL TLVs
    case kSessionType:
        string.Append("Session");
        break;
    case kSendCandidatesType:
        string.Append("SendCandidates");
        break;
    case kCandidateType:
        string.Append("Candidate");
        break;
    case kHostType:
        string.Append("Host");
        break;
    case kCandidateYesType:
        string.Append("CandidateYes");
        break;
    case kCandidateNoType:
        string.Append("CandidateNo");
        break;
    case kConflictType:
        string.Append("Conflict");
        break;
    case kHostnameType:
        string.Append("Hostname");
        break;
    case kHostMessageType:
        string.Append("HostMessage");
        break;
    case kTimeOffsetType:
        string.Append("TimeOffset");
        break;
    case kKeyIdType:
        string.Append("KeyId");
        break;
    default:
        break;
    }

    string.Append("(0x%x)", aType);

    return string;
}

//---------------------------------------------------------------------------------------------------------------------
// `Srpl::Session`

Srpl::Session::Session(Instance &aInstance, const Ip6::SockAddr &aSockAddr)
    : Connection(aInstance, aSockAddr, aInstance.Get<Srp::Srpl>().mConnectionCallbacks)
    , mNextSession(nullptr)
    , mPhase(kToSync)
    , mCandidateHost(nullptr)
    , mSendCandidatesMessageId(0)
    , mReconnectInterval(kMinReconnectInterval)
    , mToRemove(false)
    , mErrored(false)
    , mExpectHostRequest(false)
{
}

Srpl::Session::Session(Instance &aInstance, const Ip6::SockAddr &aSockAddr, const PeerId &aPartnerId)
    : Session(aInstance, aSockAddr)
{
    SetPartnerId(aPartnerId);
}

Srpl::Session::~Session(void)
{
    Disconnect(kGracefullyClose, kReasonUnknown);
}

void Srpl::Session::Connect(void)
{
    VerifyOrExit(GetState() == Connection::kStateDisconnected);
    VerifyOrExit(!IsMarkedToRemove() && !IsMarkedAsErrored());

    Connection::Connect();

exit:
    return;
}

void Srpl::Session::MarkToRemove(bool aToRemove)
{
    VerifyOrExit(mToRemove != aToRemove);

    mToRemove = aToRemove;

    // We wait for`kRemoveTimeout` and if the partner is not
    // re-added after the timeout, we remove it from the list and
    // close the session with it.

    mRemoveTime = TimerMilli::GetNow() + kRemoveTimeout;

    Get<Srpl>().GetTimer().FireAtIfEarlier(mRemoveTime);

    otLogInfoSrp("[srpl] Marking %s to be removed in %u msec", ToString().AsCString(), kRemoveTimeout);

exit:
    return;
}

void Srpl::Session::SetPhase(Phase aPhase)
{
    VerifyOrExit(mPhase != aPhase);

    otLogInfoSrp("[srpl] Phase: %s -> %s on %s", PhaseToString(mPhase), PhaseToString(aPhase), ToString().AsCString());
    mPhase = aPhase;

exit:
    return;
}

void Srpl::Session::StartRoutineOperation(void)
{
    SetPhase(kRoutineOperation);

    // Once initial sync is performed and we start routine operation,
    // we reset the reconnect interval (which will be used if the session
    // errors and disconnects) back to its minimum value.

    mReconnectInterval = kMinReconnectInterval;

    // We mark the DSO session as long-lived so to clear the DSO
    // inactivity timeout and ensure that the DSO session stays connected
    // even if no messages are exchanged.

    SetLongLivedOperation(true);

    if (!mQueue.IsEmpty())
    {
        SendHostRequest();
    }

    if (Get<Srpl>().mPrimarySession == this)
    {
        // Once we finish initial sync with the primary partner
        // we can start SRP server and advertise SRPL DNS-SD
        // service.

        Get<Srpl>().mPrimarySession = nullptr;
        Get<Srpl>().StartSrpServer();
    }
}

void Srpl::Session::HandleError(void)
{
    // This method handles any fatal error (e.g., disconnects or peer
    // misbehavior). We mark the session as "errored", reset the
    // phase, clear any pending SRP updates in queue for this
    // session, and set up timer for reconnect time.

    constexpr uint16_t kLogInMsecLimit = 5000; // Max interval (in msec) to log the value in msec unit

    OT_UNUSED_VARIABLE(kLogInMsecLimit);

    SetPhase(kToSync);
    mQueue.Free();

    otLogInfoSrp("[srpl] Session %s errored - reconnect in %u %s", ToString().AsCString(),
                 (mReconnectInterval < kLogInMsecLimit) ? mReconnectInterval : Time::MsecToSec(mReconnectInterval),
                 (mReconnectInterval < kLogInMsecLimit) ? "ms" : "sec");

    mErrored           = true;
    mReconnectTime     = TimerMilli::GetNow() + mReconnectInterval;
    mReconnectInterval = mReconnectInterval / kReconnectGrowthFactorDenominator * kReconnectGrowthFactorNumerator;
    Get<Srpl>().GetTimer().FireAtIfEarlier(mReconnectTime);

    // If the disconnected/errored session is with the primary
    // partner, we try to select another primary partner.

    if (Get<Srpl>().mPrimarySession == this)
    {
        Get<Srpl>().SelectPrimaryPartner();
    }
}

void Srpl::Session::HandleTimer(TimeMilli aNow, TimeMilli &aNextFireTime)
{
    // In case the session errored and disconnected earlier, we check
    // if the reconnect interval has expired and if so, we clear the
    // `mErrored` flags (to allow reconnects again). Then we check if
    // our ID is larger than partner's ID, we start establishing
    // connection (acting as client) with the partner. Otherwise, it
    // is up to the partner to start the connection.

    if (mErrored && (mReconnectTime < aNow))
    {
        const PeerId peerId = Get<Srpl>().GetPeerId();

        mErrored = false;

        if (peerId.HasId() && mPartnerId.HasId() && (peerId > mPartnerId))
        {
            Connect();
        }
    }

    if (mToRemove)
    {
        aNextFireTime = OT_MIN(aNextFireTime, mRemoveTime);
    }

    if (mErrored)
    {
        aNextFireTime = OT_MIN(aNextFireTime, mReconnectTime);
    }
}

bool Srpl::Session::Matches(const RemoveTime &aRemoveTime) const
{
    // This indicates whether the `Session` entry can be removed now,
    // i.e., `kRemoveTimeout` has passed since it was marked to be
    // removed.  `aRemoveTime.mNow` provides the current time. This
    // method is used by `LinkedList::RemoveAllMatching()` to remove
    // all matching entries (that is why it is named `Matches()`).

    return (mToRemove && (mRemoveTime <= aRemoveTime.mNow));
}

Srpl::Session::InfoString Srpl::Session::ToString(void) const
{
    InfoString string;

    if (GetPartnerId().HasId())
    {
        string.Append("{id:%u", GetPartnerId().GetId());
    }
    else
    {
        string.Append("{id:(none)");
    }

    string.Append(", %s}", GetPeerSockAddr().ToString().AsCString());

    return string;
}

const char *Srpl::Session::PhaseToString(Phase aPhase)
{
    static const char *const kPhaseStrings[] = {
        "ToSync",                    // (0) kToSync,
        "EstablishingSession",       // (1) kEstablishingSession,
        "SyncCandidatesFromPartner", // (2) kSyncCandidatesFromPartner,
        "SendCandidatesToPartner",   // (3) kSendCandidatesToPartner,
        "RoutineOperation",          // (4) kRoutineOperation,
    };

    static_assert(0 == kToSync, "kToSync value is incorrect");
    static_assert(1 == kEstablishingSession, "kEstablishingSession value is incorrect");
    static_assert(2 == kSyncCandidatesFromPartner, "kSyncCandidatesFromPartner value is incorrect");
    static_assert(3 == kSendCandidatesToPartner, "kSendCandidatesToPartner value is incorrect");
    static_assert(4 == kRoutineOperation, "kRoutineOperation value is incorrect");

    return kPhaseStrings[aPhase];
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Methods called from `Srp::Server`

void Srpl::Session::SendUpdateMessage(const RetainPtr<UpdateMessage> &aMessagePtr)
{
    bool                     isFirst = false;
    UpdateMessageQueueEntry *entry;

    switch (GetPhase())
    {
    case kRoutineOperation:
    case kSyncCandidatesFromPartner:
    case kSendCandidatesToPartner:
        break;

    case kToSync:
    case kEstablishingSession:
        ExitNow();
    }

    entry = UpdateMessageQueueEntry::Allocate(aMessagePtr);
    VerifyOrExit(entry != nullptr);

    if (mQueue.IsEmpty())
    {
        isFirst = true;
        mQueue.Push(*entry);
    }
    else
    {
        mQueue.PushAfter(*entry, *mQueue.GetTail());
    }

    // During initial sync, we queue all the received Update messages
    // and send them once we enter routine operation phase.
    //
    // If in routine operation phase, and this is the first message in
    // `mQueue` we send "Host" request message. Otherwise (if `mQueue`
    // is not empty) we are in the middle of sending "Host" request for
    // a previously queued message. In this case, the new `aMessagePtr`
    // will be sent once we receive "Host" response.

    if ((GetPhase() == kRoutineOperation) && isFirst)
    {
        SendHostRequest();
    }

exit:
    return;
}

void Srpl::Session::HandleServerRemovingHost(const Server::Host &aHost)
{
    // This is called when `Srp::Server` is about to fully remove a
    // host entry. If we are in "SendCandidate" phase and the removed
    // host is the current one being synced, then we set it to
    // `nullptr` to indicate that its removed.

    VerifyOrExit(GetPhase() == kSendCandidatesToPartner);

    if (mCandidateHost == &aHost)
    {
        otLogInfoSrp("[srpl] Cur candidate %s removed on server, session:%s", aHost.GetFullName(),
                     ToString().AsCString());
        mCandidateHost = nullptr;
    }

exit:
    return;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Callbacks from DSO

void Srpl::Session::HandleConnected(void)
{
    // If acting as client, we send a "Session" request message, if
    // acting as server, we wait for client's "Session" request
    // message.

    SetPhase(kEstablishingSession);

    if (IsClient())
    {
        SendSessionRequest();
    }
}

void Srpl::Session::HandleSessionEstablished(void)
{
    // Nothing to be done!
}

void Srpl::Session::HandleDisconnected(void)
{
    // A disconnect indicates an error (misbehavior).
    HandleError();
}

Error Srpl::Session::ProcessRequestMessage(MessageId aMessageId, const Message &aMessage, Tlv::Type aPrimaryTlvType)
{
    Error error = kErrorNotFound;

    otLogInfoSrp("[srpl] Received request, msg-id:%u, tlv:%s, from:%s", aMessageId,
                 Tlv::TypeToString(aPrimaryTlvType).AsCString(), ToString().AsCString());

    switch (aPrimaryTlvType)
    {
    case Tlv::kSessionType:
        error = ProcessSessionRequest(aMessageId, aMessage);
        break;
    case Tlv::kSendCandidatesType:
        error = ProcessSendCandidatesRequest(aMessageId, aMessage);
        break;
    case Tlv::kCandidateType:
        error = ProcessCandidateRequest(aMessageId, aMessage);
        break;

    case Tlv::kHostType:
        error = ProcessHostRequest(aMessageId, aMessage);
        break;

    case Tlv::kCandidateYesType:
    case Tlv::kCandidateNoType:
    case Tlv::kConflictType:
    case Tlv::kHostnameType:
    case Tlv::kHostMessageType:
    case Tlv::kTimeOffsetType:
    case Tlv::kKeyIdType:
        error = kErrorAbort;
        break;

    default:
        break;
    }

    return error;
}

Error Srpl::Session::ProcessUnidirectionalMessage(const Message &aMessage, Tlv::Type aPrimaryTlvType)
{
    OT_UNUSED_VARIABLE(aMessage);
    OT_UNUSED_VARIABLE(aPrimaryTlvType);

    // SRPL does not use any unidirectional messages.
    //
    // If a DSO unidirectional message is received containing an
    // unrecognized Primary TLV then this is a fatal error and the
    // recipient MUST forcibly abort the connection immediately
    // [RFC 8490 - 5.4.5].

    otLogInfoSrp("[srpl] Received unidirectional msg, tlv %s, from:%s", Tlv::TypeToString(aPrimaryTlvType).AsCString(),
                 ToString().AsCString());

    return kErrorAbort;
}

Error Srpl::Session::ProcessResponseMessage(const Dns::Header &aHeader,
                                            const Message &    aMessage,
                                            Tlv::Type          aResponseTlvType,
                                            Tlv::Type          aRequestTlvType)
{
    Error error = kErrorAbort;

    // Before calling this, the `Dso::Connection` already validated
    // that the message is a response for an earlier request message
    // (based on the message ID).

    otLogInfoSrp("[srpl] Received response, msg-id:%u, tlv:%s, from:%s", aHeader.GetMessageId(),
                 Tlv::TypeToString(aRequestTlvType).AsCString(), ToString().AsCString());

    if (aHeader.GetResponseCode() != Dns::Header::kResponseSuccess)
    {
        otLogInfoSrp("[srpl]  DNS-error-code:%u", aHeader.GetResponseCode());
        ExitNow();
    }

    if (aRequestTlvType != aResponseTlvType)
    {
        otLogInfoSrp("[srpl]  Mismatched primary TLVs, request:%s, response:%s",
                     Tlv::TypeToString(aRequestTlvType).AsCString(), Tlv::TypeToString(aResponseTlvType).AsCString());
        ExitNow();
    }

    switch (aRequestTlvType)
    {
    case Tlv::kSessionType:
        error = ProcessSessionResponse(aMessage);
        break;
    case Tlv::kSendCandidatesType:
        error = ProcessSendCandidatesResponse(aMessage);
        break;
    case Tlv::kCandidateType:
        error = ProcessCandidateResponse(aMessage);
        break;
    case Tlv::kHostType:
        error = ProcessHostResponse(aMessage);
        break;
    default:
        break;
    }

exit:
    return error;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Helper methods for preparing, sending, parsing messages.

Error Srpl::Session::SendRequestMessage(Message &aMessage)
{
    Error     error;
    MessageId messageId;
    Tlv       tlv;

    SuccessOrAssert(tlv.ReadFrom(aMessage, 0));
    SuccessOrExit(error = Connection::SendRequestMessage(aMessage, messageId));

    otLogInfoSrp("[srpl] Sent request, msg-id:%u, tlv:%s, to:%s", messageId, tlv.ToString().AsCString(),
                 ToString().AsCString());

exit:
    return error;
}

Error Srpl::Session::SendResponseMessage(Message &aMessage, MessageId aResponseId)
{
    Error error;
    Tlv   tlv;

    SuccessOrAssert(tlv.ReadFrom(aMessage, 0));
    SuccessOrExit(error = Connection::SendResponseMessage(aMessage, aResponseId));

    otLogInfoSrp("[srpl] Sent response, msg-id:%u, tlv:%s, to:%s", aResponseId, tlv.ToString().AsCString(),
                 ToString().AsCString());

exit:
    return error;
}

Message *Srpl::Session::PrepareMessage(Tlv::Type aTlvType, Tlv::Type aSecondTlvType)
{
    // Allocates and prepares a `Message` which includes a primary
    // TLV of type `aTlvType` with empty value, along with an optional
    // second TLV of type `aSecondTlvType` again with empty value
    // (if `aSeconTlvType != Tlv::kReservedType`).

    Error    error   = kErrorNone;
    Message *message = NewMessage();

    VerifyOrExit(message != nullptr);

    SuccessOrExit(error = message->Append(Tlv(aTlvType)));

    VerifyOrExit(aSecondTlvType != Tlv::kReservedType);
    error = message->Append(Tlv(aSecondTlvType));

exit:
    FreeAndNullMessageOnError(message, error);
    return message;
}

Error Srpl::Session::ParseMessage(const Message &aMessage, Tlv::Type aTlvType)
{
    // Parses the `aMessage` expecting to see just a primary TLV
    // with type `aTlvType` (with empty/any value).

    Error    error  = kErrorAbort;
    uint16_t offset = aMessage.GetOffset();
    Tlv      tlv;

    SuccessOrExit(tlv.ReadFrom(aMessage, offset));
    VerifyOrExit(tlv.GetType() == aTlvType);
    offset += tlv.GetSize();

    SuccessOrExit(ParseAnyUnrecognizedTlvs(aMessage, offset));
    error = kErrorNone;

exit:
    return error;
}

Error Srpl::Session::ParseAnyUnrecognizedTlvs(const Message &aMessage, uint16_t aOffset)
{
    Error error = kErrorAbort;
    Tlv   tlv;

    while (aOffset < aMessage.GetLength())
    {
        SuccessOrExit(tlv.ReadFrom(aMessage, aOffset));
        aOffset += tlv.GetSize();

        VerifyOrExit(tlv.IsUnrecognizedOrPadding());
    }

    VerifyOrExit(aOffset == aMessage.GetLength());
    error = kErrorNone;

exit:
    return error;
}

void Srpl::Session::UpdateTlvLengthInMessage(Message &aMessage, uint16_t aOffset)
{
    // This method is used to calculate the TLV length and update
    // (rewrite) it in a message. This should be called immediately
    // after the full TLV value is written in the message. `aOffset`
    // gives the offset in the message to the start of the TLV.

    Tlv tlv;

    SuccessOrAssert(aMessage.Read(aOffset, tlv));
    tlv.Init(tlv.GetType(), aMessage.GetLength() - aOffset - sizeof(Tlv));
    aMessage.Write(aOffset, tlv);
}

uint32_t Srpl::Session::CalculateKeyId(const Server::Host &aCandidateHost)
{
    static constexpr uint8_t kKeyLength = Crypto::Ecdsa::P256::PublicKey::kSize;

    uint32_t       keyId = 0;
    const uint8_t *keyData;

    static_assert((kKeyLength % sizeof(uint32_t)) == 0, "Host Key Length MUST be a factor of 4");
    OT_ASSERT(aCandidateHost.GetKeyRecord() != nullptr);

    keyData = aCandidateHost.GetKeyRecord()->GetKey().GetBytes();

    for (uint8_t index = 0; index < kKeyLength; index += sizeof(uint32_t))
    {
        keyId += Encoding::BigEndian::ReadUint32(&keyData[index]);
    }

    return keyId;
}

uint32_t Srpl::Session::CalculateSecondsSince(TimeMilli aTime, TimeMilli aNow)
{
    // Calculates seconds since `aTime` till `aNow`, rounding the
    // value to the nearest integer. This is used for Time Offset
    // TLV.

    return Time::MsecToSec((aNow - aTime) + kOneSecondInMsec / 2);
}

/*
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *
 *  SRPL Sequence Diagram
 *
 *
 *                    Client                                                Server
 *                      |                                                     |
 *                      |              Session Request                        |
 *                      |---------------------------------------------------->|
 *   Establishing       |              Session Response                       |    Establishing
 *     Session          |<----------------------------------------------------|      Session
 *                      |                                                     |
 *                      |              SendCandidates Request                 |
 *                     /|~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~>|\
 *                    / |                                                     | \
 *                   |  |              Candidate Request                      |  |
 *                   |  |<----------------------------------------------------|  |
 *                   |  |              Candidate Response (Yes)               |  |
 *                   |  |---------------------------------------------------->|  |
 *                   |  |              Host Request                           |  |
 *                   |  |<----------------------------------------------------|  |
 *  SyncCandidates  /   |              Host Response                          |   \   SendCandidates
 *   FromPartner    \   |---------------------------------------------------->|   /     ToPartner
 *                   |  |                                                     |  |
 *                   |  |              Candidate Request                      |  |
 *                   |  |<----------------------------------------------------|  |
 *                   |  |              Candidate Response (No)                |  |
 *                   |  |---------------------------------------------------->|  |
 *                   |  |                                                     |  |
 *                   |  |                     ...                             |  |
 *                   |  |                                                     |  |
 *                    \ |              SendCandidates Response                | /
 *                     \|<~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~|/
 *                      |                                                     |
 *                      |              SendCandidates Request                 |
 *                     /|<~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~|\
 *                    / |                                                     | \
 *                   |  |              Candidate Request                      |  |
 *                   |  |---------------------------------------------------->|  |
 *                   |  |              Candidate Response (No)                |  |
 *  SendCandidates  /   |<----------------------------------------------------|   \  SyncCandidates
 *    ToPartner     \   |                                                     |   /    FromPartner
 *                   |  |                     ...                             |  |
 *                   |  |                                                     |  |
 *                    \ |              SendCandidates Response                | /
 *                     \|~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~>|/
 *                      |                                                     |
 *   RoutineOperation   |                                                     |     RoutineOperation
 *
 */

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "Session" Message

void Srpl::Session::SendSessionRequest(void)
{
    Error    error   = kErrorNone;
    Message *message = nullptr;

    // Client starts SRPL session by sending "Session" request.

    OT_ASSERT(IsClient());

    message = PrepareSessionMessage();
    VerifyOrExit(message != nullptr, error = kErrorNoBufs);

    error = SendRequestMessage(*message);

exit:
    FreeMessageOnError(message, error);
}

Error Srpl::Session::ProcessSessionRequest(MessageId aMessageId, const Message &aMessage)
{
    Error  error = kErrorAbort;
    PeerId partnerId;
    PeerId assignedId;

    // A "Session" request can only be received on a server from
    // a client during session establishment phase.

    VerifyOrExit(IsServer() && (GetPhase() == kEstablishingSession));

    SuccessOrExit(ParseSessionMessage(aMessage, partnerId, assignedId));

    VerifyOrExit(!assignedId.HasId());

    SendSessionResponse(aMessageId, !partnerId.HasId());

    MarkSessionEstablished();
    SetPhase(kSendCandidatesToPartner);
    error = kErrorNone;

exit:
    return error;
}

void Srpl::Session::SendSessionResponse(MessageId aMessageId, bool aAssignIdToPartner)
{
    Error    error   = kErrorNone;
    Message *message = nullptr;
    PeerId   partnerId;

    // Only on server to respond to a "Session" request from client.

    OT_ASSERT(IsServer());

    if (aAssignIdToPartner)
    {
        // We find the largest in-use ID among all partners including
        // our ID, then add a small random value to it to determine
        // the ID to assign to the partner in "Session" response.

        partnerId = Get<Srpl>().GetPeerId();

        for (const Session &session : Get<Srpl>().mSessions)
        {
            if (!session.GetPartnerId().HasId())
            {
                continue;
            }

            if (!partnerId.HasId() || (partnerId < session.GetPartnerId()))
            {
                partnerId = session.GetPartnerId();
            }
        }

        OT_ASSERT(partnerId.HasId());

        partnerId.SetId(partnerId.GetId() + Random::NonCrypto::GetUint32InRange(1, kAssignPeerIdWindow));
        otLogInfoSrp("[srpl] Assigning id %u to partner", partnerId.GetId());
        mPartnerId = partnerId;
    }

    message = PrepareSessionMessage(partnerId);
    VerifyOrExit(message != nullptr, error = kErrorNoBufs);

    SuccessOrExit(error = SendResponseMessage(*message, aMessageId));

exit:
    FreeMessageOnError(message, error);
}

Error Srpl::Session::ProcessSessionResponse(const Message &aMessage)
{
    Error  error = kErrorAbort;
    PeerId partnerId;
    PeerId assignedId;

    // Session response MUST be from server to client for a request
    // during SRPL session establishment.

    VerifyOrExit((GetPhase() == kEstablishingSession) && IsClient());

    // The response's Session TLV MUST include the partner ID. If we
    // do not have an ID and therefore asked for an ID to be assigned
    // (by not including an ID in the request message), then the TLV
    // MUST also include an assigned ID.

    SuccessOrExit(ParseSessionMessage(aMessage, partnerId, assignedId));

    VerifyOrExit(partnerId.HasId());

    if (Get<Srpl>().GetPeerId().HasId())
    {
        VerifyOrExit(!assignedId.HasId());
    }
    else
    {
        VerifyOrExit(assignedId.HasId());

        for (const Session &session : Get<Srpl>().mSessions)
        {
            if (session.GetPartnerId().HasId() && (session.GetPartnerId() == assignedId))
            {
                otLogInfoSrp("[srpl] Conflicting id %u assigned by server (also assigned to %s)", assignedId.GetId(),
                             session.ToString().AsCString());
                ExitNow();
            }
        }

        Get<Srpl>().SetPeerId(assignedId);
        otLogInfoSrp("[srpl] Got assigned peer ID %u", assignedId.GetId());

        Get<Srpl>().SetState(kStatePrimaryPartnerSync);

        // Now that we got an ID, we can start establishing
        // SRPL session with other partners.

        for (Session &session : Get<Srpl>().mSessions)
        {
            if (session.GetPartnerId().HasId() && (assignedId > session.GetPartnerId()))
            {
                // Calling `Connect()` if session is already connected
                // changes nothing.
                session.Connect();
            }
        }
    }

    error = kErrorNone;
    MarkSessionEstablished();

    SendSendCandidatesRequest();

exit:
    return error;
}

Message *Srpl::Session::PrepareSessionMessage(const PeerId &aPartnerId)
{
    Error         error   = kErrorNone;
    Message *     message = NewMessage();
    const PeerId &peerId  = Get<Srpl>().GetPeerId();
    uint16_t      tlvLength;

    VerifyOrExit(message != nullptr);

    // Assert that we are not in the situation where we don't have ID
    // (`peerId` is empty) but we are assigning an ID to the partner.

    OT_ASSERT(peerId.HasId() || !aPartnerId.HasId());

    // If we have an ID, include it in the Session message
    // Otherwise we keep it empty for the server partner to
    // assign to us.

    tlvLength = (peerId.HasId() ? PeerId::kSize : 0) + (aPartnerId.HasId() ? PeerId::kSize : 0);
    SuccessOrExit(error = message->Append(Tlv(Tlv::kSessionType, tlvLength)));

    VerifyOrExit(peerId.HasId());
    SuccessOrExit(error = Tlv::WriteUint32Value(*message, peerId.GetId()));

    VerifyOrExit(aPartnerId.HasId());
    SuccessOrExit(error = Tlv::WriteUint32Value(*message, aPartnerId.GetId()));

exit:
    FreeAndNullMessageOnError(message, error);
    return message;
}

Error Srpl::Session::ParseSessionMessage(const Message &aMessage, PeerId &aPartnerId, PeerId &aAssignedId)
{
    Error    error  = kErrorAbort;
    uint16_t offset = aMessage.GetOffset();
    Tlv      tlv;

    aPartnerId.Clear();
    aAssignedId.Clear();

    SuccessOrExit(tlv.ReadFrom(aMessage, offset));
    offset += sizeof(Tlv);

    VerifyOrExit(tlv.GetType() == Tlv::kSessionType);

    if (tlv.GetLength() == 0)
    {
        ExitNow(error = kErrorNone);
    }

    VerifyOrExit(tlv.GetLength() >= PeerId::kSize);

    SuccessOrExit(Tlv::ReadUint32Value(aMessage, offset, aPartnerId.UpdateId()));
    offset += sizeof(uint32_t);

    if (mPartnerId.HasId() && (mPartnerId != aPartnerId))
    {
        otLogInfoSrp("[srpl] Received ID %u does not match existing ID %u", aPartnerId.GetId(), mPartnerId.GetId());
        ExitNow();
    }

    mPartnerId = aPartnerId;

    if (tlv.GetLength() == PeerId::kSize)
    {
        ExitNow(error = kErrorNone);
    }

    VerifyOrExit(tlv.GetLength() == 2 * PeerId::kSize);

    SuccessOrExit(Tlv::ReadUint32Value(aMessage, offset, aAssignedId.UpdateId()));
    offset += sizeof(uint32_t);

    error = ParseAnyUnrecognizedTlvs(aMessage, offset);

exit:
    return error;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "SendCandidates" Message

void Srpl::Session::SendSendCandidatesRequest(void)
{
    Error    error = kErrorNone;
    Message *message;

    message = PrepareMessage(Tlv::kSendCandidatesType);
    VerifyOrExit(message != nullptr, error = kErrorNoBufs);

    SuccessOrExit(error = SendRequestMessage(*message));

    SetPhase(kSyncCandidatesFromPartner);
    mExpectHostRequest = false;

exit:
    FreeMessageOnError(message, error);
}

Error Srpl::Session::ProcessSendCandidatesRequest(MessageId aMessageId, const Message &aMessage)
{
    Error error = kErrorAbort;

    VerifyOrExit(GetPhase() == kSendCandidatesToPartner);

    SuccessOrExit(ParseMessage(aMessage, Tlv::kSendCandidatesType));
    error = kErrorNone;

    // Start sending all candidates
    mSendCandidatesMessageId = aMessageId;
    mCandidateHost           = Get<Server>().mHosts.GetHead();
    SendCandidateRequest();

exit:
    return error;
}

void Srpl::Session::SendSendCandidatesResponse(void)
{
    Error    error   = kErrorNone;
    Message *message = PrepareMessage(Tlv::kSendCandidatesType);

    VerifyOrExit(message != nullptr);
    SuccessOrExit(error = SendResponseMessage(*message, mSendCandidatesMessageId));

    if (IsServer())
    {
        // Acting as server we are now done syncing candidates with
        // the client. Now we ask client to sync with us.

        SendSendCandidatesRequest();
    }
    else
    {
        // Acting as client, we are now done syncing candidates with
        // the server. The initial synchronization is over so we start
        // routine operation.

        StartRoutineOperation();
    }

exit:
    FreeMessageOnError(message, error);
}

Error Srpl::Session::ProcessSendCandidatesResponse(const Message &aMessage)
{
    Error error = kErrorAbort;

    VerifyOrExit(GetPhase() == kSyncCandidatesFromPartner);

    SuccessOrExit(ParseMessage(aMessage, Tlv::kSendCandidatesType));
    error = kErrorNone;

    if (IsServer())
    {
        // Acting as server, we are now done receiving and syncing
        // candidates from client. The initial synchronization is over
        // so we start routine operation.

        StartRoutineOperation();
    }
    else
    {
        // Acting as client, we are now done receiving and syncing
        // candidates from server. Next we expect server to send
        // a "SendCandidates" request.

        SetPhase(kSendCandidatesToPartner);
    }

exit:
    return error;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "Candidate" Message

void Srpl::Session::SendCandidateRequest(void)
{
    Error    error   = kErrorNone;
    Message *message = nullptr;
    uint16_t offset;
    uint32_t timeOffset;

    if (mCandidateHost == nullptr)
    {
        // We are done with all candidates, so we send
        // "SendCandidate" response to end current phase.

        SendSendCandidatesResponse();
        ExitNow();
    }

    // Prepare and send "Candidate" request message. In addition to
    // SRPL Candidate TLV as the primary TLV, message MUST include
    // the following secondary TLVs: SRPL Hostname, SRPL Time Offset,
    // and SRPL Key ID.

    message = PrepareMessage(Tlv::kCandidateType);
    VerifyOrExit(message != nullptr, error = kErrorNoBufs);

    offset = message->GetLength();
    SuccessOrExit(error = message->Append(Tlv(Tlv::kHostnameType)));
    SuccessOrExit(error = Dns::Name::AppendName(mCandidateHost->GetFullName(), *message));
    UpdateTlvLengthInMessage(*message, offset);

    timeOffset = CalculateSecondsSince(mCandidateHost->mUpdateTime);
    SuccessOrExit(error = Tlv::AppendUint32Tlv(*message, Tlv::kTimeOffsetType, timeOffset));

    SuccessOrExit(error = Tlv::AppendUint32Tlv(*message, Tlv::kKeyIdType, CalculateKeyId(*mCandidateHost)));

    SuccessOrExit(error = SendRequestMessage(*message));
    otLogInfoSrp("[srpl] Candidate request - hostname:%s, time-offset:%u", mCandidateHost->GetFullName(), timeOffset);

exit:
    FreeMessageOnError(message, error);
}

Error Srpl::Session::ProcessCandidateRequest(MessageId aMessageId, const Message &aMessage)
{
    Error               error  = kErrorAbort;
    uint16_t            offset = aMessage.GetOffset();
    uint16_t            nameOffset;
    bool                parsedhostName    = false;
    bool                parsedTimedOffset = false;
    bool                parsedKeyId       = false;
    char                hostName[Dns::Name::kMaxNameSize];
    uint32_t            timeOffset;
    uint32_t            keyId;
    Tlv                 tlv;
    const Server::Host *candidateHost;
    Tlv::Type           responseTlv;

    VerifyOrExit(GetPhase() == kSyncCandidatesFromPartner);

    SuccessOrExit(tlv.ReadFrom(aMessage, offset));
    VerifyOrExit(tlv.GetType() == Tlv::kCandidateType);
    offset += tlv.GetSize();

    while (offset < aMessage.GetLength())
    {
        SuccessOrExit(tlv.ReadFrom(aMessage, offset));
        offset += sizeof(Tlv);

        switch (tlv.GetType())
        {
        case Tlv::kHostnameType:
            VerifyOrExit(!parsedhostName);
            nameOffset = offset;
            SuccessOrExit(Dns::Name::ReadName(aMessage, nameOffset, hostName, sizeof(hostName)));
            VerifyOrExit(tlv.GetLength() >= nameOffset - offset);
            parsedhostName = true;
            break;

        case Tlv::kTimeOffsetType:
            VerifyOrExit(!parsedTimedOffset);
            VerifyOrExit(tlv.GetLength() >= sizeof(uint32_t));
            SuccessOrExit(Tlv::ReadUint32Value(aMessage, offset, timeOffset));
            parsedTimedOffset = true;
            break;

        case Tlv::kKeyIdType:
            VerifyOrExit(!parsedKeyId);
            VerifyOrExit(tlv.GetLength() >= sizeof(uint32_t));
            SuccessOrExit(Tlv::ReadUint32Value(aMessage, offset, keyId));
            parsedKeyId = true;
            break;

        default:
            VerifyOrExit(tlv.IsUnrecognizedOrPadding());
            break;
        }

        offset += tlv.GetLength();
    }

    VerifyOrExit(offset == aMessage.GetLength());
    VerifyOrExit(parsedhostName && parsedTimedOffset && parsedKeyId);

    error = kErrorNone;
    otLogInfoSrp("[srpl] Candidate request - hostname:%s, time-offset:%u", hostName, timeOffset);

    candidateHost = Get<Server>().mHosts.FindMatching(hostName);

    responseTlv = Tlv::kCandidateYesType;

    if (candidateHost != nullptr)
    {
        uint32_t candidateKeyId = CalculateKeyId(*candidateHost);

        if (candidateKeyId != keyId)
        {
            responseTlv = Tlv::kConflictType;
            otLogInfoSrp("[srpl] Key ID conflict - recved:0x%08x, expected:0x%08x", keyId, candidateKeyId);
        }
        else
        {
            TimeMilli updateTime = TimerMilli::GetNow() - Time::SecToMsec(timeOffset);
            uint32_t  diff;

            diff = (updateTime > candidateHost->mUpdateTime) ? updateTime - candidateHost->mUpdateTime
                                                             : candidateHost->mUpdateTime - updateTime;

            responseTlv = (diff <= kUpdateSkewWindow) ? Tlv::kCandidateNoType : Tlv::kCandidateYesType;
        }
    }

    SendCandidateResponse(aMessageId, responseTlv);

exit:
    return error;
}

void Srpl::Session::SendCandidateResponse(MessageId aMessageId, Tlv::Type aResponseTlvType)
{
    Error    error   = kErrorNone;
    Message *message = nullptr;

    message = PrepareMessage(Tlv::kCandidateType, aResponseTlvType);
    VerifyOrExit(message != nullptr, error = kErrorNoBufs);
    SuccessOrExit(error = SendResponseMessage(*message, aMessageId));
    otLogInfoSrp("[srpl] Candidate response - %s", Tlv::TypeToString(aResponseTlvType).AsCString());
    mExpectHostRequest = (aResponseTlvType == Tlv::kCandidateYesType);

exit:
    FreeMessageOnError(message, error);
}

Error Srpl::Session::ProcessCandidateResponse(const Message &aMessage)
{
    Error     error           = kErrorAbort;
    uint16_t  offset          = aMessage.GetOffset();
    Tlv::Type responseTlvType = Tlv::kReservedType;
    Tlv       tlv;

    SuccessOrExit(tlv.ReadFrom(aMessage, offset));
    VerifyOrExit(tlv.GetType() == Tlv::kCandidateType);
    offset += tlv.GetSize();

    for (; offset < aMessage.GetLength(); offset += tlv.GetSize())
    {
        SuccessOrExit(tlv.ReadFrom(aMessage, offset));

        switch (tlv.GetType())
        {
        case Tlv::kCandidateYesType:
        case Tlv::kCandidateNoType:
        case Tlv::kConflictType:
            VerifyOrExit(responseTlvType == Tlv::kReservedType);
            responseTlvType = tlv.GetType();
            break;

        default:
            VerifyOrExit(tlv.IsUnrecognizedOrPadding());
            break;
        }
    }

    VerifyOrExit(offset == aMessage.GetLength());
    VerifyOrExit(responseTlvType != Tlv::kReservedType);

    error = kErrorNone;
    otLogInfoSrp("[srpl] Candidate response - %s", Tlv::TypeToString(responseTlvType).AsCString());

    // Before proceeding we need to make sure the `mCandidateHost` is
    // still valid and not removed on `Server`. After sending
    // "Candidate" request message and before receiving its response
    // the host entry may be removed (e.g., lease expired, or explicit
    // remove from client). If it is removed, then `mCandidateHost` is
    // set to `nullptr` from `HandleServerRemovingHost()`. In such a
    // case, we start over the sync from the first entry on `mHosts`
    // list.

    if (mCandidateHost == nullptr)
    {
        mCandidateHost = Get<Server>().mHosts.GetHead();
        SendCandidateRequest();
    }
    else
    {
        switch (responseTlvType)
        {
        case Tlv::kCandidateYesType:
            SendHostRequest();
            break;

        case Tlv::kCandidateNoType:
        case Tlv::kConflictType:
            mCandidateHost = mCandidateHost->GetNext();
            SendCandidateRequest();
            break;

        default:
            OT_ASSERT(true);
        }
    }

exit:
    return error;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "Host" message

void Srpl::Session::SendHostRequest(void)
{
    Error     error   = kErrorNone;
    Message * message = nullptr;
    TimeMilli now     = TimerMilli::GetNow();

    message = PrepareMessage(Tlv::kHostType);
    VerifyOrExit(message != nullptr, error = kErrorNoBufs);

    if (GetPhase() == kSendCandidatesToPartner)
    {
        UpdateMessageQueue queue;

        // We append all retained SRP Update messages from
        // `mCandidateHost` in the order of their rx time.

        for (const Server::Service &service : mCandidateHost->mServices)
        {
            SuccessOrExit(error = InsertInQueue(service.mAddMessagePtr, queue));
            SuccessOrExit(error = InsertInQueue(service.mDeleteMessagePtr, queue));
        }

        SuccessOrExit(error = InsertInQueue(mCandidateHost->mMessagePtr, queue));

        for (const UpdateMessageQueueEntry &entry : queue)
        {
            SuccessOrExit(error = AppendHostMessageTlv(*message, entry.mMessagePtr, now));
        }
    }
    else
    {
        OT_ASSERT(GetPhase() == kRoutineOperation);
        OT_ASSERT(!mQueue.IsEmpty());

        SuccessOrExit(error = AppendHostMessageTlv(*message, mQueue.GetHead()->mMessagePtr, now));
    }

    error = SendRequestMessage(*message);

exit:
    FreeMessageOnError(message, error);
}

Error Srpl::Session::InsertInQueue(const RetainPtr<UpdateMessage> &aMessagePtr, UpdateMessageQueue &aQueue)
{
    // Inserts a `aMessagePtr` into `aQueue` keeping the entries
    // sorted based on `mRxTime`. If `aMessagePtr` is `nullptr` or
    // if it is already in the queue, `aQueue` remains unchanged.

    Error                    error     = kErrorNone;
    UpdateMessageQueueEntry *prevEntry = nullptr;
    UpdateMessageQueueEntry *newEntry;

    VerifyOrExit(aMessagePtr != nullptr);

    for (UpdateMessageQueueEntry &entry : aQueue)
    {
        if (entry.mMessagePtr == aMessagePtr)
        {
            ExitNow();
        }

        if (entry.mMessagePtr->mRxTime > aMessagePtr->mRxTime)
        {
            break;
        }

        prevEntry = &entry;
    }

    newEntry = UpdateMessageQueueEntry::Allocate(aMessagePtr);
    VerifyOrExit(newEntry != nullptr, error = kErrorNoBufs);

    if (prevEntry == nullptr)
    {
        aQueue.Push(*newEntry);
    }
    else
    {
        aQueue.PushAfter(*newEntry, *prevEntry);
    }

exit:
    return error;
}

Error Srpl::Session::AppendHostMessageTlv(Message &                       aMessage,
                                          const RetainPtr<UpdateMessage> &aMessagePtr,
                                          TimeMilli                       aNow)
{
    Error    error;
    uint16_t offset;
    uint32_t rxTimeOffset;

    OT_ASSERT(aMessagePtr != nullptr);

    offset = aMessage.GetLength();
    SuccessOrExit(error = aMessage.Append(Tlv(Tlv::kHostMessageType)));
    rxTimeOffset = CalculateSecondsSince(aMessagePtr->mRxTime, aNow);
    SuccessOrExit(error = Tlv::WriteUint32Value(aMessage, rxTimeOffset));
    SuccessOrExit(error = Tlv::WriteUint32Value(aMessage, aMessagePtr->mGrantedLease));
    SuccessOrExit(error = Tlv::WriteUint32Value(aMessage, aMessagePtr->mGrantedKeyLease));
    SuccessOrExit(error = aMessage.AppendBytes(aMessagePtr->mData.GetBytes(), aMessagePtr->mData.GetLength()));
    UpdateTlvLengthInMessage(aMessage, offset);

    otLogInfoSrp("[srpl] Host request - msg, rx-time:%u, lease:%u, key-lease:%u, len:%u", rxTimeOffset,
                 aMessagePtr->mGrantedLease, aMessagePtr->mGrantedKeyLease, aMessagePtr->mData.GetLength());

exit:
    return error;
}

Error Srpl::Session::ProcessHostRequest(MessageId aMessageId, const Message &aMessage)
{
    Error     error  = kErrorAbort;
    TimeMilli now    = TimerMilli::GetNow();
    uint16_t  offset = aMessage.GetOffset();
    Tlv       tlv;

    switch (GetPhase())
    {
    case kSyncCandidatesFromPartner:
        VerifyOrExit(mExpectHostRequest);
        mExpectHostRequest = false;
        break;
    case kRoutineOperation:
        break;
    default:
        break;
    }

    // We first parse and validate all TLVs in `aMessage`.

    SuccessOrExit(tlv.ReadFrom(aMessage, offset));
    VerifyOrExit(tlv.GetType() == Tlv::kHostType);
    offset += tlv.GetSize();

    for (; offset < aMessage.GetLength(); offset += tlv.GetSize())
    {
        SuccessOrExit(tlv.ReadFrom(aMessage, offset));

        switch (tlv.GetType())
        {
        case Tlv::kHostnameType:
        case Tlv::kTimeOffsetType:
        case Tlv::kKeyIdType:
            break;

        case Tlv::kHostMessageType:
            VerifyOrExit(tlv.GetLength() > Tlv::kMinHostMessageTlvLength);
            break;

        default:
            VerifyOrExit(tlv.IsUnrecognizedOrPadding());
            break;
        }
    }

    VerifyOrExit(offset == aMessage.GetLength());
    error = kErrorNone;

    // Now process the Host Message TLVs

    for (offset = aMessage.GetOffset(); offset < aMessage.GetLength(); offset += tlv.GetSize())
    {
        SuccessOrAssert(tlv.ReadFrom(aMessage, offset));
        ProcessHostMessageTlv(aMessage, offset, tlv, now);
    }

    SendHostResponse(aMessageId);

exit:
    return error;
}

void Srpl::Session::ProcessHostMessageTlv(const Message &aMessage, uint16_t aOffset, const Tlv &aTlv, TimeMilli aNow)
{
    // Process Host Message TLV. `aOffset` points to start of
    // the TLV. The caller MUST have already validated the TLV
    // type and format.

    Error    error   = kErrorNone;
    Message *message = nullptr;
    uint32_t rxTimeOffset;
    uint32_t grantedLease;
    uint32_t grantedKeyLease;
    uint16_t msgLength;

    VerifyOrExit(aTlv.GetType() == Tlv::kHostMessageType);
    aOffset += sizeof(Tlv);

    SuccessOrAssert(Tlv::ReadUint32Value(aMessage, aOffset, rxTimeOffset));
    aOffset += sizeof(uint32_t);

    SuccessOrAssert(Tlv::ReadUint32Value(aMessage, aOffset, grantedLease));
    aOffset += sizeof(uint32_t);

    SuccessOrAssert(Tlv::ReadUint32Value(aMessage, aOffset, grantedKeyLease));
    aOffset += sizeof(uint32_t);

    message = Get<MessagePool>().Allocate(Message::kTypeOther);
    VerifyOrExit(message != nullptr);

    msgLength = aTlv.GetLength() - sizeof(uint32_t) * 3;
    SuccessOrExit(message->AppendBytesFromMessage(aMessage, aOffset, msgLength));

    otLogInfoSrp("[srpl] Host request - msg, rx-time:%u, lease:%u, key-lease:%u, len:%u", rxTimeOffset, grantedLease,
                 grantedKeyLease, msgLength);

    message->SetOffset(0);

    error = Get<Server>().ProcessMessage(*message, aNow - Time::SecToMsec(rxTimeOffset), grantedLease, grantedKeyLease);

    if (error != kErrorNone)
    {
        otLogInfoSrp("[srpl] Server failed to process msg, error: %s", ErrorToString(error));
    }

exit:
    FreeMessage(message);
}

void Srpl::Session::SendHostResponse(MessageId aMessageId)
{
    Error    error   = kErrorNone;
    Message *message = PrepareMessage(Tlv::kHostType);

    VerifyOrExit(message != nullptr);
    error = SendResponseMessage(*message, aMessageId);

exit:
    FreeMessageOnError(message, error);
}

Error Srpl::Session::ProcessHostResponse(const Message &aMessage)
{
    Error error = kErrorAbort;

    SuccessOrExit(ParseMessage(aMessage, Tlv::kHostType));
    error = kErrorNone;

    if (GetPhase() == kSendCandidatesToPartner)
    {
        // While waiting for "Host" response, the candidate host may be
        // removed on `Server`. If it is removed, then `mCandidateHost`
        // is set to `nullptr` from `HandleServerRemovingHost()`. In such
        // a case, we start over the sync from the first entry on
        // `mHosts` list. Otherwise we go to the next candidate host.

        mCandidateHost = (mCandidateHost == nullptr) ? Get<Server>().mHosts.GetHead() : mCandidateHost->GetNext();
        SendCandidateRequest();
    }
    else
    {
        OT_ASSERT(!mQueue.IsEmpty());

        // Since `mQueue` is an `OwningList`, popping the entry and not
        // assigning it to an `OwnedPtr` will automatically free it.

        mQueue.Pop();

        if (!mQueue.IsEmpty())
        {
            SendHostRequest();
        }
    }

exit:
    return error;
}

} // namespace Srp
} // namespace ot

#endif // OPENTHREAD_CONFIG_SRP_REPLICATION_ENABLE
