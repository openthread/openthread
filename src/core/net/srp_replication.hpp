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

#ifndef SRP_REPLICATION_HPP_
#define SRP_REPLICATION_HPP_

#include "openthread-core-config.h"

#if OPENTHREAD_CONFIG_SRP_REPLICATION_ENABLE

#if !OPENTHREAD_CONFIG_SRP_SERVER_ENABLE
#error "SRP Replication requires SRP Server support (OPENTHREAD_CONFIG_SRP_SERVER_ENABLE)."
#endif

#if !OPENTHREAD_CONFIG_DNS_DSO_ENABLE
#error "SRP Replication requires DSO support (OPENTHREAD_CONFIG_DNS_DSO_ENABLE)."
#endif

#include <openthread/srp_replication.h>
#include <openthread/platform/srp_replication.h>

#include "common/as_core_type.hpp"
#include "common/clearable.hpp"
#include "common/equatable.hpp"
#include "common/heap_allocatable.hpp"
#include "common/heap_string.hpp"
#include "common/linked_list.hpp"
#include "common/locator.hpp"
#include "common/non_copyable.hpp"
#include "common/owned_ptr.hpp"
#include "common/owning_list.hpp"
#include "common/retain_ptr.hpp"
#include "common/time.hpp"
#include "common/timer.hpp"
#include "net/dns_dso.hpp"
#include "net/dns_types.hpp"
#include "net/srp_server.hpp"

/**
 * @file
 *   This file includes definitions for the SRP Replication Protocol (SRPL).
 */

namespace ot {
namespace Srp {

extern "C" void otPlatSrplHandleDnssdBrowseResult(otInstance *aInstance, const otPlatSrplPartnerInfo *aPartnerInfo);

/**
 * This class implements SRP Replication Protocol (SRPL).
 *
 */
class Srpl : public InstanceLocator, private NonCopyable
{
    friend void otPlatSrplHandleDnssdBrowseResult(otInstance *aInstance, const otPlatSrplPartnerInfo *aPartnerInfo);
    friend class Server;

    class Session;

public:
    enum State : uint8_t
    {
        kStateDisabled  = OT_SRP_REPLICATION_STATE_DISABLED,  ///< Disabled.
        kStateDiscovery = OT_SRP_REPLICATION_STATE_DISCOVERY, ///< Discovering domain, partners.
        kStateRunning   = OT_SRP_REPLICATION_STATE_RUNNING,   ///< Active and running (SRP server is enabled).
    };

    /**
     * This class represents SRPL partner info.
     *
     */
    class Partner : public otSrpReplicationPartner
    {
        friend class Srpl;

    public:
        /**
         * This enumeration represents state of an SRPL session with a partner.
         *
         */
        enum SessionState
        {
            kSessionDisconnected     = OT_SRP_REPLICATION_SESSION_DISCONNECTED,      ///< Disconnected.
            kSessionConnecting       = OT_SRP_REPLICATION_SESSION_CONNECTING,        ///< Establishing connection.
            kSessionEstablishing     = OT_SRP_REPLICATION_SESSION_ESTABLISHING,      ///< Establishing SRPL session.
            kSessionInitialSync      = OT_SRP_REPLICATION_SESSION_INITIAL_SYNC,      ///< Initial SRPL sync.
            kSessionRoutineOperation = OT_SRP_REPLICATION_SESSION_ROUTINE_OPERATION, ///< Routine operation.
            kSessionErrored          = OT_SRP_REPLICATION_SESSION_ERRORED,           ///< Session errored earlier.
        };

        /**
         * This structure represents an iterator to iterate over SRPL partner list.
         *
         */
        struct Iterator : public otSrpReplicationPartnerIterator
        {
            /**
             * This method initializes the iterator.
             *
             */
            void Init(void) { mData = nullptr; }
        };

    private:
        void SetFrom(const Session &aSession);
    };

    /**
     * This constructor initializes the `Srpl`.
     *
     * @param[in] aInstance   The OpenThread instance.
     *
     */
    explicit Srpl(Instance &aInstance);

    /**
     * This method enables/disables the SRP Replication (SRPL).
     *
     * @param[in]  aEnable  A boolean to enable/disable SRPL.
     *
     * @retval kErrorNone           Successfully enabled/disabled SRP Replication (SRPL).
     * @retval kErrorInvalidState   Failed to enable SRP Replication since SRP server is already enabled.
     *                              Note that SRP Replication when enabled manages when to enable SRP server.
     *
     */
    Error SetEnabled(bool aEnable);

    /**
     * This method indicates whether or not SRP Replication (SRPL) is enabled.
     *
     * @retval TRUE   If SRPL is enabled.
     * @retval FALSE  If SRPL is disabled.
     *
     */
    bool IsEnabled(void) const { return mState != kStateDisabled; }

    /**
     * This method gets the current SRP Replication (SRPL) state.
     *
     * @returns The current SRPL state.
     *
     */
    State GetState(void) const { return mState; }

    /**
     * This method sets the domain name and the join behavior (accept any domain, or require exact match)
     *
     * This method can be called only when SRPL is disabled, otherwise `kErrorInvalidState` is returned.
     *
     * If @p aName is not `nullptr`, then SRPL will only accept and join peers with the same domain name and includes
     * * @p aName as domain when advertising "_srpl-tls._tcp" service using DNS-SD.
     *
     * If @p aName is `nullptr` then SRPL will accept any joinable domain, i.e., after start it will adopt the
     * domain name of the first joinable SRPL peer it discovers while performing DNS-SD browse for "_srpl-tls._tcp"
     * service. If SRPL does not discover any peer to adopt its domain name (e.g., it is first/only SRPL device) it
     * starts advertising using the default domain name from `GetDefaultDomain()` method.
     *
     * @param[in] aName            The domain name. Can be `nullptr` to adopt domain from any discovered joinable SRPL
     *                             peer.
     *
     * @retval kErrorNone          Successfully set the domain name.
     * @retval kErrorInvalidState  SRPL is enabled and therefore the domain cannot be changed.
     * @retval kErrorNoBufs        Failed to allocate buffer to save the domain name.
     *
     */
    Error SetDomain(const char *aName);

    /**
     * This method gets the current domain name.
     *
     * @returns The domain name string, or `nullptr` if no domain.
     *
     */
    const char *GetDomain(void) const { return mDomainName.AsCString(); }

    /**
     * This method sets the default domain name.
     *
     * This method can be called only when SRPL is disabled, otherwise `kErrorInvalidState` is returned.
     *
     * The default domain name is only used when `GetDomain() == nullptr` and if SRPL does not discover any suitable
     * peer to adopt their domain name (during domain discovery phase after SRPL start).
     *
     * @param[in] aName            The domain name. MUST NOT be `nullptr`.
     *
     * @retval kErrorNone          Successfully set the default domain name.
     * @retval kErrorInvalidState  SRPL is enabled and therefore the default domain cannot be changed.
     * @retval kErrorNoBufs        Failed to allocate buffer to save the domain name.
     *
     */
    Error SetDefaultDomain(const char *aName);

    /**
     * This method gets the default domain name.
     *
     * @returns The domain name string.
     *
     */
    const char *GetDefaultDomain(void) const { return mDefaultDomainName.AsCString(); }

    /**
     * This method gets the peer ID assigned to the SRPL itself (if any).
     *
     * @returns The peer ID of the SRPL.
     *
     */
    uint64_t GetId(void) const { return GetPeerId().GetId(); }

    /**
     * This method gets the current dataset ID of SRPL (if any).
     *
     * @param[out] aDatasetId  A reference to a `uint64_t` to return the current dataset ID.
     *
     * @retval kErrorNone      Successfully retrieved the dataset ID and updated @p aDatasetId.
     * @retval kErrorNotFound  SRPL does not yet have any dataset ID.
     *
     */
    Error GetDatasetId(uint64_t &aDatasetId) const;

    /**
     * This method iterates over the SRPL partners using an iterator and retrieves the info for the next partner in the
     * list.
     *
     * @param[in]  aIterator   An iterator. It MUST be already initialized.
     * @param[out] aPartnter   A reference to  a `Partner` to return the partner info.
     *
     * @retval kErrorNone       Retrieved the next partner info from the list. @p aPartner is updated.
     * @retval kErrorNotFound   No more partner in the list.
     *
     */
    Error GetNextPartner(Partner::Iterator &aIterator, Partner &aPartner) const;

#if OPENTHREAD_CONFIG_SRP_REPLICATION_TEST_API_ENABLE
    /**
     * This structure defines a set of configuration flags and parameters for behavior changes used for testing.
     *
     */
    class TestConfig : public otSrpReplicationTestConfig, public Clearable<TestConfig>
    {
        friend class Srpl;

    private:
        TestConfig(void) { Clear(); }
    };

    /**
     * This method gets the current `TestConfig` setting.
     *
     * @returns The current `TestConfig` settings.
     *
     */
    const TestConfig &GetTestConfig(void) const { return mTestConfig; }

    /**
     * This method sets the `TestConfig` settings.
     *
     * @param[in] aTestConfig        The new test config to use.
     *
     */
    void SetTestConfig(const TestConfig &aTestConfig);

#endif // OPENTHREAD_CONFIG_SRP_REPLICATION_TEST_API_ENABLE

private:
    // All intervals are in msec
    static constexpr uint32_t kDiscoveryMinInterval = OPENTHREAD_CONFIG_SRP_REPLICATION_MIN_DISCOVERY_INTERVAL;
    static constexpr uint32_t kDiscoveryMaxInterval = OPENTHREAD_CONFIG_SRP_REPLICATION_MAX_DISCOVERY_INTERVAL;

    static constexpr uint32_t kSelfSelectedPrecIdWindow = 200;
    static constexpr uint32_t kAssignPrecIdWindow       = 5;

    static constexpr uint16_t kPrecIdStringSize = 16;

    // String size to represent `uint64_t` value in hex format
    static constexpr uint16_t kIdStringSize = 17;

    static const char    kTxtDataKeyDomain[];
    static const char    kTxtDataKeyAllowsJoin[];
    static const char    kTxtDataKeyPeerId[];
    static const char    kTxtDataKeyDatasetId[];
    static const char    kDefaultDomainName[];
    static const uint8_t kYesString[];
    static const uint8_t kNoString[];

    typedef String<kIdStringSize> IdString;

    using UpdateMessage = Server::UpdateMessage;

    struct UpdateMessageQueueEntry : public Heap::Allocatable<UpdateMessageQueueEntry>,
                                     public LinkedListEntry<UpdateMessageQueueEntry>
    {
        explicit UpdateMessageQueueEntry(const RetainPtr<UpdateMessage> &aMessagePtr)
            : mMessagePtr(aMessagePtr)
            , mNext(nullptr)
        {
        }

        bool Matches(const RetainPtr<UpdateMessage> &aMessagePtr) const { return (mMessagePtr == aMessagePtr); }

        RetainPtr<UpdateMessage> mMessagePtr;
        UpdateMessageQueueEntry *mNext;
    };

    using UpdateMessageQueue = OwningList<UpdateMessageQueueEntry>;

    // A `uint64_t` ID - base of `PeerId` and `DatasetId`.
    class Id : public Unequatable<Id>
    {
    public:
        static constexpr uint16_t kSize          = sizeof(uint64_t);
        static constexpr uint16_t kHexStringSize = 17;

        typedef String<kHexStringSize> HexString;

        uint64_t GetId(void) const { return mId; }
        void     SetId(uint64_t aId) { mId = aId; }

        void GenerateRandom(void);

        bool operator==(const Id &aOther) const { return mId == aOther.mId; }
        bool operator<(const Id &aOther) const { return SerialNumber::IsLess(mId, aOther.mId); }
        bool operator>(const Id &aOther) const { return SerialNumber::IsGreater(mId, aOther.mId); }

        Error     ParseFromString(const char *aString);
        void      ToString(HexString &aString) const;
        HexString ToString(void) const;

    protected:
        uint64_t mId;
    };

    class PeerId : public Id
    {
    };

    class DatasetId : public Id
    {
    public:
        uint8_t GetSeqNumber(void) const { return static_cast<uint8_t>(mId >> 56); }
        void    SetSeqNumber(uint8_t aSeqNumber);
    };

    struct PartnerInfo : public otPlatSrplPartnerInfo
    {
        Error ParseTxtData(Heap::String &aDomainName, PeerId &aPeerId, DatasetId &aDatasetId, bool &aAllowsJoin) const;

        Ip6::SockAddr &      GetSockAddr(void) { return AsCoreType(&mSockAddr); }
        const Ip6::SockAddr &GetSockAddr(void) const { return AsCoreType(&mSockAddr); }
    };

    OT_TOOL_PACKED_BEGIN
    class Tlv : public Dns::Dso::Tlv
    {
    public:
        static constexpr uint16_t kInfoStringSize = 30;

        using InfoString = String<kInfoStringSize>;

        // Currently using values from the experimental number space
        // MUST be updated when values are assigned.
        static constexpr Type kSessionType        = 0xf90c;
        static constexpr Type kSendCandidatesType = 0xf90d;
        static constexpr Type kCandidateType      = 0xf90e;
        static constexpr Type kHostType           = 0xf90f;
        static constexpr Type kCandidateYesType   = 0xf911;
        static constexpr Type kCandidateNoType    = 0xf912;
        static constexpr Type kConflictType       = 0xf913;
        static constexpr Type kHostnameType       = 0xf914;
        static constexpr Type kHostMessageType    = 0xf915;
        static constexpr Type kTimeOffsetType     = 0xf916;
        static constexpr Type kKeyIdType          = 0xf917;
        static constexpr Type kNewPeerType        = 0xf918;

        // Host Message TLV contains:
        //   - Rx Time offset as `uint32_t`
        //   - Granted lease as `uint32_t`.
        //   - Granted key lease as `uint32_t`
        //   - SRP update message (starting from DNS header).
        //
        static constexpr uint16_t kMinHostMessageTlvLength = sizeof(uint32_t) * 3 + sizeof(Dns::Header);

        Tlv(void) = default;
        explicit Tlv(Type aType, uint16_t aLength = 0) { Init(aType, aLength); }

        bool       IsUnrecognizedOrPadding(void) { return IsUnrecognizedOrPaddingTlv(GetType()); }
        Error      ReadFrom(const Message &aMessage, uint16_t aOffset);
        InfoString ToString(void) const { return TypeToString(GetType()); }

        static bool       IsUnrecognizedOrPaddingTlv(Type aType);
        static Error      WriteUint32Value(Message &aMessage, uint32_t aValue);
        static Error      ReadUint32Value(const Message &aMessage, uint16_t aOffset, uint32_t &aValue);
        static Error      AppendUint32Tlv(Message &aMessage, Type aType, uint32_t aValue);
        static Error      WriteUint64Value(Message &aMessage, uint64_t aValue);
        static Error      ReadUint64Value(const Message &aMessage, uint16_t aOffset, uint64_t &aValue);
        static Error      AppendUint64Tlv(Message &aMessage, Type aType, uint64_t aValue);
        static InfoString TypeToString(Type aType);
    } OT_TOOL_PACKED_END;

    using Connection = Dns::Dso::Connection;

    class Session : public Connection, public Heap::Allocatable<Session>
    {
    public:
        static constexpr uint16_t kInfoStringSize = 70;

        typedef String<kInfoStringSize> InfoString;

        enum Phase : uint8_t
        {
            kToSync,
            kEstablishingSession,
            kSyncCandidatesFromPartner,
            kSendCandidatesToPartner,
            kRoutineOperation,
        };

        enum PartnerState : uint8_t
        {
            kNewPartner,        // New partner (not yet discovered from DNS-SD browse).
            kDiscoveredPartner, // DNS-SD browse discovered the partner (received "add event").
            kRemovedPartner,    // DNS-SD browse removed the partner (received "remove event").
        };

        struct RemoveTime
        {
            explicit RemoveTime(TimeMilli aNow) { mNow = aNow; }
            TimeMilli mNow;
        };

        struct MatchContext
        {
            PeerId   mPartnerId;
            Session *mExcludedSession;
        };

        Session(Instance &aInstance, const Ip6::SockAddr &aSockAddr, PartnerState aPartnerState);
        Session(Instance &           aInstance,
                const Ip6::SockAddr &aSockAddr,
                PartnerState         aPartnerState,
                const PeerId &       aPartnerId);
        ~Session(void);

        void           HandleError(void);
        void           HandleTimer(TimeMilli aNow, TimeMilli &aNextFireTime);
        Session *      GetNext(void) { return mNextSession; }
        const Session *GetNext(void) const { return mNextSession; }
        void           SetNext(Session *aSession) { mNextSession = aSession; }
        Phase          GetPhase(void) const { return mPhase; }
        PartnerState   GetPartnerState(void) const { return mPartnerState; }
        void           SetPartnerState(PartnerState aPartnerState);
        void           StartRemoveTimer(void);
        bool           HasPartnerId(void) const { return mHasPartnerId; }
        const PeerId & GetPartnerId(void) const { return mPartnerId; }
        void           SetPartnerId(const PeerId &aPartnerId) { mHasPartnerId = true, mPartnerId = aPartnerId; }
        void           ClearPartnerId(void) { mHasPartnerId = false; }
        void           MarkToSyncBeforeServerStart(void) { mSyncBeforeServerStart = true; }
        bool           IsMarkedForSyncBeforeServerStart(void) const { return mSyncBeforeServerStart; }
        bool           IsMarkedAsErrored(void) const { return mErrored; }
        bool           Matches(const PeerId &aPartnerId) const { return mHasPartnerId && (mPartnerId == aPartnerId); }
        bool           Matches(const Ip6::SockAddr &aSockAddr) const { return GetPeerSockAddr() == aSockAddr; }
        bool           Matches(const MatchContext &aMatchContext) const;
        bool           Matches(const RemoveTime &aRemoveTime) const;
        InfoString     ToString(void) const;

        // Methods called from `Srp::Server`
        void SendUpdateMessage(const RetainPtr<UpdateMessage> &aMessagePtr);
        void HandleServerRemovingHost(const Server::Host &aHost);

        // Callbacks from `Connection`.
        void  HandleConnected(void);
        void  HandleSessionEstablished(void);
        void  HandleDisconnected(void);
        Error ProcessRequestMessage(MessageId aMessageId, const Message &aMessage, Tlv::Type aPrimaryTlvType);
        Error ProcessUnidirectionalMessage(const Message &aMessage, Tlv::Type aPrimaryTlvType);
        Error ProcessResponseMessage(const Dns::Header &aHeader,
                                     const Message &    aMessage,
                                     Tlv::Type          aResponseTlvType,
                                     Tlv::Type          aRequestTlvType);

    private:
        // This timeout is used when DNS-SD browse signals that an SRPL
        // partner is removed. We mark the partner to be removed and
        // wait for `kRemoveTimeout` interval before removing it from
        // the list of partners and dropping any connection/session to
        // it. This ensures that if the partner is re-added within the
        // timeout, we continue with any existing connection/session and
        // potentially avoid going through session establishment and
        // initial sync with the partner again.

        static constexpr uint32_t kRemoveTimeout = OPENTHREAD_CONFIG_SRP_REPLICATION_PARTNER_REMOVE_TIMEOUT;

        // If there is a disconnect or failure (misbehavior) on an SRPL
        // session with a partner, the reconnect interval is used
        // before trying to connect again or accepting connection
        // request from the partner. The reconnect interval starts with
        // the min interval. On back-to-back failures the reconnect
        // interval is increased using growth factor up to its maximum
        // value. The reconnect interval is reset back to its minimum
        // value after establishing an SRP session with the partner
        // and successfully finishing the initial synchronization.

        static constexpr uint32_t kMinReconnectInterval    = OPENTHREAD_CONFIG_SRP_REPLICATION_MIN_RECONNECT_INTERVAL;
        static constexpr uint32_t kMaxReconnectIntervalMax = OPENTHREAD_CONFIG_SRP_REPLICATION_MAX_RECONNECT_INTERVAL;
        static constexpr uint32_t kReconnectGrowthFactorNumerator =
            OPENTHREAD_CONFIG_SRP_REPLICATION_RECONNECT_GROWTH_FACTOR_NUMERATOR;
        static constexpr uint32_t kReconnectGrowthFactorDenominator =
            OPENTHREAD_CONFIG_SRP_REPLICATION_RECONNECT_GROWTH_FACTOR_DENOMINATOR;

        static constexpr uint32_t kUpdateSkewWindow = 1100; // in msec

        static constexpr uint32_t kOneSecondInMsec = TimeMilli::SecToMsec(1);

        void            SetPhase(Phase aPhase);
        bool            CanRemove(void) const;
        void            StartRoutineOperation(void);
        Error           SendRequestMessage(Message &aMessage);
        Error           SendResponseMessage(Message &aMessage, MessageId aResponseId);
        Message *       PrepareMessage(Tlv::Type aTlvType, Tlv::Type aSecondTlvType = Tlv::kReservedType);
        static Error    ParseMessage(const Message &aMessage, Tlv::Type aTlvType);
        static Error    ParseAnyUnrecognizedTlvs(const Message &aMessage, uint16_t aOffset);
        static void     UpdateTlvLengthInMessage(Message &aMessage, uint16_t aOffset);
        static uint32_t CalculateKeyId(const Server::Host &aCandidateHost);
        static uint32_t CalculateSecondsSince(TimeMilli aTime, TimeMilli aNow = TimerMilli::GetNow());

        void     SendSessionRequest(void);
        Error    ProcessSessionRequest(MessageId aMessageId, const Message &aMessage);
        void     SendSessionResponse(MessageId aMessageId);
        Error    ProcessSessionResponse(const Message &aMessage);
        Message *PrepareSessionMessage(bool aAppendNewPeerTlv = false);
        Error    ParseSessionMessage(const Message &aMessage, PeerId &aPartnerId, bool *aIsNew = nullptr);
        void     SendSendCandidatesRequest(void);
        Error    ProcessSendCandidatesRequest(MessageId aMessageId, const Message &aMessage);
        void     SendSendCandidatesResponse(void);
        Error    ProcessSendCandidatesResponse(const Message &aMessage);
        void     SendCandidateRequest(void);
        Error    ProcessCandidateRequest(MessageId aMessageId, const Message &aMessage);
        void     SendCandidateResponse(MessageId aMessageId, Tlv::Type aResponseTlvType);
        Error    ProcessCandidateResponse(const Message &aMessage);
        void     SendHostRequest(void);
        Error    InsertInQueue(const RetainPtr<UpdateMessage> &aMessagePtr, UpdateMessageQueue &aQueue);
        Error    AppendHostMessageTlv(Message &aMessage, const RetainPtr<UpdateMessage> &aMessagePtr, TimeMilli aNow);
        Error    ProcessHostRequest(MessageId aMessageId, const Message &aMessage);
        void     ProcessHostMessageTlv(const Message &aMessage, uint16_t aOffset, const Tlv &aTlv, TimeMilli aNow);
        void     SendHostResponse(MessageId aMessageId);
        Error    ProcessHostResponse(const Message &aMessage);

        static const char *PhaseToString(Phase aPhase);
        static const char *PartnerStateToString(PartnerState aPartnerState);

        Session *           mNextSession;
        Phase               mPhase;
        PartnerState        mPartnerState;
        PeerId              mPartnerId;
        UpdateMessageQueue  mQueue;                     // Queue of SRP Update messages to be sent to partner.
        const Server::Host *mCandidateHost;             // Cur candidate (used only in `kSendCandidatesToPartner`).
        MessageId           mSendCandidatesMessageId;   // Used only in `kSendCandidatesToPartner` phase.
        uint32_t            mReconnectInterval;         // Current reconnect wait interval.
        TimeMilli           mReconnectTime;             // Reconnect time - only used when `mErrored == true`.
        TimeMilli           mRemoveTime;                // Remove time.
        bool                mHasPartnerId : 1;          // Partner ID is known and set.
        bool                mSyncBeforeServerStart : 1; // Mark partner to sync before entering `kStateRunning`
        bool                mErrored : 1;               // Session disconnected (error or misbehavior).
        bool                mExpectHostRequest : 1;     // Expect rx of Host req while in `kSyncCandidatesFromPartner`.
    };

    void  SetState(State aState);
    void  Start(void);
    void  Stop(void);
    void  AddPartner(const PartnerInfo &aInfo);
    void  RemovePartner(const PartnerInfo &aInfo);
    void  StartSession(Session &aSession);
    Error CheckForIdConflict(const PeerId &aPartnerId, Session *aSession = nullptr);
    void  UpdateState(void);
    void  StartDnssdAdvertisement(void);

    // Methods called from `Srp::Server`
    void SendUpdateMessageToPartners(const RetainPtr<UpdateMessage> &aMessagePtr);
    void HandleServerRemovingHost(const Server::Host &aHost);

    const PeerId &             GetPeerId(void) const { return mPeerId; }
    void                       SetPeerid(const PeerId &aPeerId) { mPeerId = aPeerId; }
    TimerMilli &               GetTimer(void) { return mTimer; }
    const OwningList<Session> &GetSessions(void) const { return mSessions; }
    OwningList<Session> &      GetSessions(void) { return mSessions; }

    static void HandleTimer(Timer &aTimer);
    void        HandleTimer(void);

    static const char *StateToString(State aState);

    // `otPlatSrpl` methods and callbacks
    void DnssdBrowse(bool aEnable);
    void HandleDnssdBrowseResult(const PartnerInfo &aInfo);
    void RegisterDnssdService(const uint8_t *aTxtData, uint16_t aTxtLength);
    void UnregisterDnssdService(void);

    // Callback from `Dso`
    static Connection *AcceptConnection(Instance &aInstance, const Ip6::SockAddr &aPeerSockAddr);
    Connection *       AcceptConnection(const Ip6::SockAddr &aPeerSockAddr);

    // Callbacks from `Dso::Connection`
    static void  HandleConnected(Connection &aConnection);
    static void  HandleSessionEstablished(Connection &aConnection);
    static void  HandleDisconnected(Connection &aConnection);
    static Error ProcessRequestMessage(Connection &          aConnection,
                                       Connection::MessageId aMessageId,
                                       const Message &       aMessage,
                                       Tlv::Type             aPrimaryTlvType);
    static Error ProcessUnidirectionalMessage(Connection &   aConnection,
                                              const Message &aMessage,
                                              Tlv::Type      aPrimaryTlvType);
    static Error ProcessResponseMessage(Connection &       aConnection,
                                        const Dns::Header &aHeader,
                                        const Message &    aMessage,
                                        Tlv::Type          aResponseTlvType,
                                        Tlv::Type          aRequestTlvType);

    State                 mState;
    Heap::String          mDomainName;
    Heap::String          mDefaultDomainName;
    PeerId                mPeerId;
    DatasetId             mDatasetId;
    bool                  mHasDatasetId;
    TimeMilli             mDiscoveryEndTime;
    TimerMilli            mTimer;
    OwningList<Session>   mSessions;
    Connection::Callbacks mConnectionCallbacks;
#if OPENTHREAD_CONFIG_SRP_REPLICATION_TEST_API_ENABLE
    TestConfig mTestConfig;
#endif
};

} // namespace Srp

DefineCoreType(otSrpReplicationPartner, Srp::Srpl::Partner);
DefineCoreType(otSrpReplicationPartnerIterator, Srp::Srpl::Partner::Iterator);
#if OPENTHREAD_CONFIG_SRP_REPLICATION_TEST_API_ENABLE
DefineCoreType(otSrpReplicationTestConfig, Srp::Srpl::TestConfig);
#endif
DefineMapEnum(otSrpReplicationState, Srp::Srpl::State);
DefineMapEnum(otSrpReplicationSessionState, Srp::Srpl::Partner::SessionState);

} // namespace ot

#endif // OPENTHREAD_CONFIG_SRP_REPLICATION_ENABLE

#endif // SRP_REPLICATION_HPP_
