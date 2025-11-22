/*
 *  Copyright (c) 2018, The OpenThread Authors.
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
 *   This file includes definitions for the BorderAgent role.
 */

#ifndef BORDER_AGENT_HPP_
#define BORDER_AGENT_HPP_

#include "openthread-core-config.h"

#if OPENTHREAD_CONFIG_BORDER_AGENT_ENABLE

#include <openthread/border_agent.h>
#include <openthread/history_tracker.h>

#include "border_router/routing_manager.hpp"
#include "common/appender.hpp"
#include "common/as_core_type.hpp"
#include "common/heap_allocatable.hpp"
#include "common/linked_list.hpp"
#include "common/locator.hpp"
#include "common/log.hpp"
#include "common/non_copyable.hpp"
#include "common/notifier.hpp"
#include "common/owned_ptr.hpp"
#include "common/tasklet.hpp"
#include "common/uptime.hpp"
#include "meshcop/border_agent_txt_data.hpp"
#include "meshcop/dataset.hpp"
#include "meshcop/secure_transport.hpp"
#include "net/dns_types.hpp"
#include "net/dnssd.hpp"
#include "net/socket.hpp"
#include "net/udp6.hpp"
#include "thread/tmf.hpp"
#include "thread/uri_paths.hpp"

namespace ot {
namespace MeshCoP {
namespace BorderAgent {

#if !OPENTHREAD_CONFIG_SECURE_TRANSPORT_ENABLE
#error "Border Agent feature requires `OPENTHREAD_CONFIG_SECURE_TRANSPORT_ENABLE`"
#endif

#if OPENTHREAD_CONFIG_BORDER_AGENT_MESHCOP_SERVICE_ENABLE

#if !(OPENTHREAD_CONFIG_PLATFORM_DNSSD_ENABLE || OPENTHREAD_CONFIG_MULTICAST_DNS_ENABLE)
#error "OPENTHREAD_CONFIG_BORDER_AGENT_MESHCOP_SERVICE_ENABLE requires either the native mDNS or platform DNS-SD APIs"
#endif

#if !OPENTHREAD_CONFIG_BORDER_AGENT_ID_ENABLE
#error "OPENTHREAD_CONFIG_BORDER_AGENT_MESHCOP_SERVICE_ENABLE requires OPENTHREAD_CONFIG_BORDER_AGENT_ID_ENABLE"
#endif

#endif

#if OPENTHREAD_CONFIG_BORDER_AGENT_EPHEMERAL_KEY_ENABLE
class EphemeralKeyManager;
#endif

#if OPENTHREAD_CONFIG_BORDER_AGENT_ID_ENABLE
/**
 *  Represents a Border Agent Identifier.
 */
struct Id : public otBorderAgentId, public Clearable<Id>, public Equatable<Id>
{
    static constexpr uint16_t kLength = OT_BORDER_AGENT_ID_LENGTH; ///< The ID length (number of bytes).

    /**
     * Generates a random ID.
     */
    void GenerateRandom(void) { Random::NonCrypto::Fill(mId); }
};
#endif

class Manager : public InstanceLocator, private NonCopyable
{
#if OPENTHREAD_CONFIG_BORDER_AGENT_MESHCOP_SERVICE_ENABLE
    friend ot::Dnssd;
#endif
#if OPENTHREAD_CONFIG_BORDER_AGENT_EPHEMERAL_KEY_ENABLE
    friend class EphemeralKeyManager;
#endif
    friend class ot::Notifier;
    friend class Tmf::Agent;
    friend class TxtData;

    class CoapDtlsSession;

public:
    typedef otBorderAgentCounters    Counters;    ///< Border Agent Counters.
    typedef otBorderAgentSessionInfo SessionInfo; ///< A session info.

    /**
     * Represents an iterator for secure sessions.
     */
    class SessionIterator : public otBorderAgentSessionIterator
    {
    public:
        /**
         * Initializes the `SessionIterator`.
         *
         * @param[in] aInstance  The OpenThread instance.
         */
        void Init(Instance &aInstance);

        /**
         * Retrieves the next session information.
         *
         * @param[out] aSessionInfo     A `SessionInfo` to populate.
         *
         * @retval kErrorNone        Successfully retrieved the next session. @p aSessionInfo is updated.
         * @retval kErrorNotFound    No more sessions are available. The end of the list has been reached.
         */
        Error GetNextSessionInfo(SessionInfo &aSessionInfo);

    private:
        CoapDtlsSession *GetSession(void) const { return static_cast<CoapDtlsSession *>(mPtr); }
        void             SetSession(CoapDtlsSession *aSession) { mPtr = aSession; }
        uint64_t         GetInitTime(void) const { return mData; }
        void             SetInitTime(uint64_t aInitTime) { mData = aInitTime; }
    };

    /**
     * Initializes the `Manager` object.
     *
     * @param[in]  aInstance     A reference to the OpenThread instance.
     */
    explicit Manager(Instance &aInstance);

    /**
     * Enables or disables the Border Agent service.
     *
     * By default, the Border Agent service is enabled. This method allows us to explicitly control its state. This can
     * be useful in scenarios such as:
     * - The code wishes to delay the start of the Border Agent service (and its mDNS advertisement of the
     *   `_meshcop._udp` service on the infrastructure link). This allows time to prepare or determine vendor-specific
     *   TXT data entries for inclusion.
     * - Unit tests or test scripts might disable the Border Agent service to prevent it from interfering with specific
     *   test steps. For example, tests validating mDNS or DNS-SD functionality may disable the Border Agent to prevent
     *   its  registration of the MeshCoP service.
     *
     * @param[in] aEnabled  Whether to enable or disable.
     */
    void SetEnabled(bool aEnabled);

    /**
     * Indicated whether or not the Border Agent is enabled.
     *
     * @retval TRUE   The Border Agent is enabled.
     * @retval FALSE  The Border Agent is disabled.
     */
    bool IsEnabled(void) const { return mEnabled; }

    /**
     * Indicates whether the Border Agent service is enabled and running.
     *
     * @retval TRUE  Border Agent service is running.
     * @retval FALSE Border Agent service is not running.
     */
    bool IsRunning(void) const { return mIsRunning; }

#if OPENTHREAD_CONFIG_BORDER_AGENT_ID_ENABLE
    static_assert(sizeof(Id) == Id::kLength, "sizeof(Id) is not valid");

    /**
     * Gets the randomly generated Border Agent ID.
     *
     * The ID is saved in persistent storage and survives reboots. The typical use case of the ID is to
     * be published in the MeshCoP mDNS service as the `id` TXT value for the client to identify this
     * Border Router/Agent device.
     *
     * @param[out] aId  Reference to return the Border Agent ID.
     */
    void GetId(Id &aId);

    /**
     * Sets the Border Agent ID.
     *
     * The Border Agent ID will be saved in persistent storage and survive reboots. It's required
     * to set the ID only once after factory reset. If the ID has never been set by calling this
     * method, a random ID will be generated and returned when `GetId()` is called.
     *
     * @param[in] aId   The Border Agent ID.
     */
    void SetId(const Id &aId);
#endif

    /**
     * Gets the UDP port of this service.
     *
     * @returns  UDP port number.
     */
    uint16_t GetUdpPort(void) const;

#if OPENTHREAD_CONFIG_BORDER_AGENT_MESHCOP_SERVICE_ENABLE
    /**
     * Sets the base name to construct the service instance name used when advertising the mDNS `_meshcop._udp` service
     * by the Border Agent.
     *
     * @param[in] aBaseName  The base name to use (MUST not be NULL).
     *
     * @retval kErrorNone          The name was set successfully.
     * @retval kErrorInvalidArgs   The name is too long or invalid.
     */
    Error SetServiceBaseName(const char *aBaseName);
#endif

    /**
     * Gets the set of border agent counters.
     *
     * @returns The border agent counters.
     */
    const Counters &GetCounters(void) { return mCounters; }

private:
    static constexpr uint16_t kUdpPort          = OPENTHREAD_CONFIG_BORDER_AGENT_UDP_PORT;
    static constexpr uint32_t kKeepAliveTimeout = 50 * 1000; // Timeout to reject a commissioner (in msec)
    static constexpr uint16_t kTxtDataMaxSize   = OT_BORDER_AGENT_MESHCOP_SERVICE_TXT_DATA_MAX_LENGTH;

#if OPENTHREAD_CONFIG_BORDER_AGENT_MESHCOP_SERVICE_ENABLE
    static constexpr uint16_t kDummyUdpPort          = 49152;
    static constexpr uint8_t  kBaseServiceNameMaxLen = OT_BORDER_AGENT_MESHCOP_SERVICE_BASE_NAME_MAX_LENGTH;
#endif

    class CoapDtlsSession : public Coap::SecureSession, public Heap::Allocatable<CoapDtlsSession>
    {
        friend Heap::Allocatable<CoapDtlsSession>;

    public:
        Error    SendMessage(OwnedPtr<Coap::Message> aMessage);
        void     ForwardUdpProxyToCommissioner(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo);
        void     ForwardUdpRelayToCommissioner(const Message &aMessage);
        void     Cleanup(void);
        bool     IsActiveCommissioner(void) const;
        uint64_t GetAllocationTime(void) const { return mAllocationTime; }
        uint16_t GetIndex(void) const { return mIndex; }

    private:
        enum Action : uint8_t
        {
            kReceive,
            kSend,
            kForward,
        };

        struct ForwardContext : public ot::LinkedListEntry<ForwardContext>,
                                public Heap::Allocatable<ForwardContext>,
                                private ot::NonCopyable
        {
            ForwardContext(CoapDtlsSession &aSession, const Coap::Message &aMessage, Uri aUri);

            CoapDtlsSession &mSession;
            ForwardContext  *mNext;
            Uri              mUri;
            uint8_t          mTokenLength;
            uint8_t          mToken[Coap::Message::kMaxTokenLength];
        };

        CoapDtlsSession(Instance &aInstance, Dtls::Transport &aDtlsTransport);

        Error ForwardToCommissioner(OwnedPtr<Coap::Message> aForwardMessage, const Message &aMessage);
        void  HandleTmfCommissionerKeepAlive(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo);
        void  HandleTmfRelayTx(Coap::Message &aMessage);
        void  HandleTmfProxyTx(Coap::Message &aMessage);
        void  HandleTmfDatasetGet(Coap::Message &aMessage, Uri aUri);
        Error ForwardToLeader(const Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo, Uri aUri);
        void  SendErrorMessage(Error aError, const uint8_t *aToken, uint8_t aTokenLength);

        static void HandleConnected(ConnectEvent aEvent, void *aContext);
        void        HandleConnected(ConnectEvent aEvent);
        static void HandleLeaderResponseToFwdTmf(void                *aContext,
                                                 otMessage           *aMessage,
                                                 const otMessageInfo *aMessageInfo,
                                                 otError              aResult);
        void        HandleLeaderResponseToFwdTmf(const ForwardContext &aForwardContext,
                                                 const Coap::Message  *aResponse,
                                                 Error                 aResult);
        static bool HandleResource(CoapBase               &aCoapBase,
                                   const char             *aUriPath,
                                   Coap::Message          &aMessage,
                                   const Ip6::MessageInfo &aMessageInfo);
        bool        HandleResource(const char *aUriPath, Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo);
        static void HandleTimer(Timer &aTimer);
        void        HandleTimer(void);

#if OT_SHOULD_LOG_AT(OT_LOG_LEVEL_INFO)
        void LogUri(Action aAction, const char *aUriString, const char *aTxt);

        template <Uri kUri> void Log(Action aAction) { Log<kUri>(aAction, ""); }
        template <Uri kUri> void Log(Action aAction, const char *aTxt) { LogUri(aAction, UriToString<kUri>(), aTxt); }
#else
        template <Uri kUri> void Log(Action) {}
        template <Uri kUri> void Log(Action, const char *) {}
#endif

        LinkedList<ForwardContext> mForwardContexts;
        TimerMilliContext          mTimer;
        UptimeMsec                 mAllocationTime;
        uint16_t                   mIndex;
    };

    void UpdateState(void);
    void Start(void);
    void Stop(void);

    // Callback from Notifier
    void HandleNotifierEvents(Events aEvents);

    template <Uri kUri> void HandleTmf(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo);

    // Callbacks used with `Dtls::Transport`.
    static SecureSession *HandleAcceptSession(void *aContext, const Ip6::MessageInfo &aMessageInfo);
    CoapDtlsSession      *HandleAcceptSession(void);
    static void           HandleRemoveSession(void *aContext, SecureSession &aSession);
    void                  HandleRemoveSession(SecureSession &aSession);

    uint16_t            GetNextSessionIndex(void) { return ++mSessionIndex; }
    const Ip6::Address &GetCommissionerAloc(void) const { return mCommissionerAloc.GetAddress(); }
    CoapDtlsSession    *GetCommissionerSession(void) { return mCommissionerSession; }

    bool IsCommissionerSession(const CoapDtlsSession &aSession) const { return mCommissionerSession == &aSession; }
    void HandleSessionConnected(CoapDtlsSession &aSession);
    void HandleSessionDisconnected(CoapDtlsSession &aSession, CoapDtlsSession::ConnectEvent aEvent);
    void HandleCommissionerPetitionAccepted(CoapDtlsSession &aSession, uint16_t aSessionId);
    void RevokeRoleIfActiveCommissioner(CoapDtlsSession &aSession);

    static bool HandleUdpReceive(void *aContext, const otMessage *aMessage, const otMessageInfo *aMessageInfo);
    bool        HandleUdpReceive(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo);

#if OPENTHREAD_CONFIG_BORDER_AGENT_MESHCOP_SERVICE_ENABLE
    // Callback from `BorderAgent::TxtData`.
    void HandleServiceTxtDataChanged(void) { RegisterService(); }

    // Callback from `Dnssd`
    void HandleDnssdPlatformStateChange(void) { RegisterService(); }

    const char *GetServiceName(void);
    bool        IsServiceNameEmpty(void) const { return mServiceName[0] == kNullChar; }
    void        ConstrcutServiceName(const char *aBaseName, Dns::Name::LabelBuffer &aNameBuffer);
    void        RegisterService(void);
    void        UnregisterService(void);
#endif

#if OPENTHREAD_CONFIG_BORDER_AGENT_MESHCOP_SERVICE_ENABLE
    static const char kServiceType[];
    static const char kDefaultBaseServiceName[];
#endif

    bool                       mEnabled;
    bool                       mIsRunning;
    uint16_t                   mSessionIndex;
    Dtls::Transport            mDtlsTransport;
    CoapDtlsSession           *mCommissionerSession;
    Ip6::Udp::Receiver         mCommissionerUdpReceiver;
    Ip6::Netif::UnicastAddress mCommissionerAloc;

#if OPENTHREAD_CONFIG_BORDER_AGENT_ID_ENABLE
    Id   mId;
    bool mIdInitialized;
#endif
#if OPENTHREAD_CONFIG_BORDER_AGENT_MESHCOP_SERVICE_ENABLE
    Dns::Name::LabelBuffer mServiceName;
#endif
    Counters mCounters;
};

DeclareTmfHandler(Manager, kUriRelayRx);

} // namespace BorderAgent
} // namespace MeshCoP

#if OPENTHREAD_CONFIG_BORDER_AGENT_ID_ENABLE
DefineCoreType(otBorderAgentId, MeshCoP::BorderAgent::Id);
#endif

DefineCoreType(otBorderAgentSessionIterator, MeshCoP::BorderAgent::Manager::SessionIterator);

} // namespace ot

#endif // OPENTHREAD_CONFIG_BORDER_AGENT_ENABLE

#endif // BORDER_AGENT_HPP_
