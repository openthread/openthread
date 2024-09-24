/*
 *  Copyright (c) 2016, The OpenThread Authors.
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
 *   This file includes definitions for the Commissioner role.
 */

#ifndef COMMISSIONER_HPP_
#define COMMISSIONER_HPP_

#include "openthread-core-config.h"

#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_COMMISSIONER_ENABLE

#include <openthread/commissioner.h>

#include "coap/coap_secure.hpp"
#include "common/as_core_type.hpp"
#include "common/callback.hpp"
#include "common/clearable.hpp"
#include "common/locator.hpp"
#include "common/log.hpp"
#include "common/non_copyable.hpp"
#include "common/timer.hpp"
#include "mac/mac_types.hpp"
#include "meshcop/announce_begin_client.hpp"
#include "meshcop/energy_scan_client.hpp"
#include "meshcop/panid_query_client.hpp"
#include "meshcop/secure_transport.hpp"
#include "net/ip6_address.hpp"
#include "net/udp6.hpp"
#include "thread/key_manager.hpp"
#include "thread/mle.hpp"
#include "thread/tmf.hpp"

namespace ot {

namespace MeshCoP {

class Commissioner : public InstanceLocator, private NonCopyable
{
    friend class Tmf::Agent;
    friend class Tmf::SecureAgent;

public:
    /**
     * Type represents the Commissioner State.
     */
    enum State : uint8_t
    {
        kStateDisabled = OT_COMMISSIONER_STATE_DISABLED, ///< Disabled.
        kStatePetition = OT_COMMISSIONER_STATE_PETITION, ///< Petitioning to become a Commissioner.
        kStateActive   = OT_COMMISSIONER_STATE_ACTIVE,   ///< Active Commissioner.
    };

    /**
     * Type represents Joiner Event.
     */
    enum JoinerEvent : uint8_t
    {
        kJoinerEventStart     = OT_COMMISSIONER_JOINER_START,
        kJoinerEventConnected = OT_COMMISSIONER_JOINER_CONNECTED,
        kJoinerEventFinalize  = OT_COMMISSIONER_JOINER_FINALIZE,
        kJoinerEventEnd       = OT_COMMISSIONER_JOINER_END,
        kJoinerEventRemoved   = OT_COMMISSIONER_JOINER_REMOVED,
    };

    typedef otCommissionerStateCallback  StateCallback;  ///< State change callback function pointer type.
    typedef otCommissionerJoinerCallback JoinerCallback; ///< Joiner state change callback function pointer type.

    /**
     * Initializes the Commissioner object.
     *
     * @param[in]  aInstance     A reference to the OpenThread instance.
     */
    explicit Commissioner(Instance &aInstance);

    /**
     * Starts the Commissioner service.
     *
     * @param[in]  aStateCallback    A pointer to a function that is called when the commissioner state changes.
     * @param[in]  aJoinerCallback   A pointer to a function that is called when a joiner event occurs.
     * @param[in]  aCallbackContext  A pointer to application-specific context.
     *
     * @retval kErrorNone           Successfully started the Commissioner service.
     * @retval kErrorAlready        Commissioner is already started.
     * @retval kErrorInvalidState   Device is not currently attached to a network.
     */
    Error Start(StateCallback aStateCallback, JoinerCallback aJoinerCallback, void *aCallbackContext);

    /**
     * Stops the Commissioner service.
     *
     * @retval kErrorNone     Successfully stopped the Commissioner service.
     * @retval kErrorAlready  Commissioner is already stopped.
     */
    Error Stop(void) { return Stop(kSendKeepAliveToResign); }

    /**
     * Returns the Commissioner Id.
     *
     * @returns The Commissioner Id.
     */
    const char *GetId(void) const { return mCommissionerId; }

    /**
     * Sets the Commissioner Id.
     *
     * @param[in]  aId   A pointer to a string character array. Must be null terminated.
     *
     * @retval kErrorNone           Successfully set the Commissioner Id.
     * @retval kErrorInvalidArgs    Given name is too long.
     * @retval kErrorInvalidState   The commissioner is active and id cannot be changed.
     */
    Error SetId(const char *aId);

    /**
     * Clears all Joiner entries.
     */
    void ClearJoiners(void);

    /**
     * Adds a Joiner entry accepting any Joiner.
     *
     * @param[in]  aPskd         A pointer to the PSKd.
     * @param[in]  aTimeout      A time after which a Joiner is automatically removed, in seconds.
     *
     * @retval kErrorNone          Successfully added the Joiner.
     * @retval kErrorNoBufs        No buffers available to add the Joiner.
     * @retval kErrorInvalidState  Commissioner service is not started.
     */
    Error AddJoinerAny(const char *aPskd, uint32_t aTimeout) { return AddJoiner(nullptr, nullptr, aPskd, aTimeout); }

    /**
     * Adds a Joiner entry.
     *
     * @param[in]  aEui64        The Joiner's IEEE EUI-64.
     * @param[in]  aPskd         A pointer to the PSKd.
     * @param[in]  aTimeout      A time after which a Joiner is automatically removed, in seconds.
     *
     * @retval kErrorNone          Successfully added the Joiner.
     * @retval kErrorNoBufs        No buffers available to add the Joiner.
     * @retval kErrorInvalidState  Commissioner service is not started.
     */
    Error AddJoiner(const Mac::ExtAddress &aEui64, const char *aPskd, uint32_t aTimeout)
    {
        return AddJoiner(&aEui64, nullptr, aPskd, aTimeout);
    }

    /**
     * Adds a Joiner entry with a Joiner Discerner.
     *
     * @param[in]  aDiscerner  A Joiner Discerner.
     * @param[in]  aPskd       A pointer to the PSKd.
     * @param[in]  aTimeout    A time after which a Joiner is automatically removed, in seconds.
     *
     * @retval kErrorNone          Successfully added the Joiner.
     * @retval kErrorNoBufs        No buffers available to add the Joiner.
     * @retval kErrorInvalidState  Commissioner service is not started.
     */
    Error AddJoiner(const JoinerDiscerner &aDiscerner, const char *aPskd, uint32_t aTimeout)
    {
        return AddJoiner(nullptr, &aDiscerner, aPskd, aTimeout);
    }

    /**
     * Get joiner info at aIterator position.
     *
     * @param[in,out]   aIterator   A iterator to the index of the joiner.
     * @param[out]      aJoiner     A reference to Joiner info.
     *
     * @retval kErrorNone       Successfully get the Joiner info.
     * @retval kErrorNotFound   Not found next Joiner.
     */
    Error GetNextJoinerInfo(uint16_t &aIterator, otJoinerInfo &aJoiner) const;

    /**
     * Removes a Joiner entry accepting any Joiner.
     *
     * @param[in]  aDelay         The delay to remove Joiner (in seconds).
     *
     * @retval kErrorNone          Successfully added the Joiner.
     * @retval kErrorNotFound      The Joiner entry accepting any Joiner was not found.
     * @retval kErrorInvalidState  Commissioner service is not started.
     */
    Error RemoveJoinerAny(uint32_t aDelay) { return RemoveJoiner(nullptr, nullptr, aDelay); }

    /**
     * Removes a Joiner entry.
     *
     * @param[in]  aEui64         The Joiner's IEEE EUI-64.
     * @param[in]  aDelay         The delay to remove Joiner (in seconds).
     *
     * @retval kErrorNone          Successfully added the Joiner.
     * @retval kErrorNotFound      The Joiner specified by @p aEui64 was not found.
     * @retval kErrorInvalidState  Commissioner service is not started.
     */
    Error RemoveJoiner(const Mac::ExtAddress &aEui64, uint32_t aDelay)
    {
        return RemoveJoiner(&aEui64, nullptr, aDelay);
    }

    /**
     * Removes a Joiner entry.
     *
     * @param[in]  aDiscerner     A Joiner Discerner.
     * @param[in]  aDelay         The delay to remove Joiner (in seconds).
     *
     * @retval kErrorNone          Successfully added the Joiner.
     * @retval kErrorNotFound      The Joiner specified by @p aEui64 was not found.
     * @retval kErrorInvalidState  Commissioner service is not started.
     */
    Error RemoveJoiner(const JoinerDiscerner &aDiscerner, uint32_t aDelay)
    {
        return RemoveJoiner(nullptr, &aDiscerner, aDelay);
    }

    /**
     * Gets the Provisioning URL.
     *
     * @returns A pointer to char buffer containing the URL string.
     */
    const char *GetProvisioningUrl(void) const { return mProvisioningUrl; }

    /**
     * Sets the Provisioning URL.
     *
     * @param[in]  aProvisioningUrl  A pointer to the Provisioning URL (may be `nullptr` to set URL to empty string).
     *
     * @retval kErrorNone         Successfully set the Provisioning URL.
     * @retval kErrorInvalidArgs  @p aProvisioningUrl is invalid (too long).
     */
    Error SetProvisioningUrl(const char *aProvisioningUrl);

    /**
     * Returns the Commissioner Session ID.
     *
     * @returns The Commissioner Session ID.
     */
    uint16_t GetSessionId(void) const { return mSessionId; }

    /**
     * Indicates whether or not the Commissioner role is active.
     *
     * @returns TRUE if the Commissioner role is active, FALSE otherwise.
     */
    bool IsActive(void) const { return mState == kStateActive; }

    /**
     * Indicates whether or not the Commissioner role is disabled.
     *
     * @returns TRUE if the Commissioner role is disabled, FALSE otherwise.
     */
    bool IsDisabled(void) const { return mState == kStateDisabled; }

    /**
     * Gets the Commissioner State.
     *
     * @returns The Commissioner State.
     */
    State GetState(void) const { return mState; }

    /**
     * Sends MGMT_COMMISSIONER_GET.
     *
     * @param[in]  aTlvs        A pointer to Commissioning Data TLVs.
     * @param[in]  aLength      The length of requested TLVs in bytes.
     *
     * @retval kErrorNone          Send MGMT_COMMISSIONER_GET successfully.
     * @retval kErrorNoBufs        Insufficient buffer space to send.
     * @retval kErrorInvalidState  Commissioner service is not started.
     */
    Error SendMgmtCommissionerGetRequest(const uint8_t *aTlvs, uint8_t aLength);

    /**
     * Sends MGMT_COMMISSIONER_SET.
     *
     * @param[in]  aDataset     A reference to Commissioning Data.
     * @param[in]  aTlvs        A pointer to user specific Commissioning Data TLVs.
     * @param[in]  aLength      The length of user specific TLVs in bytes.
     *
     * @retval kErrorNone          Send MGMT_COMMISSIONER_SET successfully.
     * @retval kErrorNoBufs        Insufficient buffer space to send.
     * @retval kErrorInvalidState  Commissioner service is not started.
     */
    Error SendMgmtCommissionerSetRequest(const CommissioningDataset &aDataset, const uint8_t *aTlvs, uint8_t aLength);

    /**
     * Returns a reference to the AnnounceBeginClient instance.
     *
     * @returns A reference to the AnnounceBeginClient instance.
     */
    AnnounceBeginClient &GetAnnounceBeginClient(void) { return mAnnounceBegin; }

    /**
     * Returns a reference to the EnergyScanClient instance.
     *
     * @returns A reference to the EnergyScanClient instance.
     */
    EnergyScanClient &GetEnergyScanClient(void) { return mEnergyScan; }

    /**
     * Returns a reference to the PanIdQueryClient instance.
     *
     * @returns A reference to the PanIdQueryClient instance.
     */
    PanIdQueryClient &GetPanIdQueryClient(void) { return mPanIdQuery; }

private:
    static constexpr uint32_t kPetitionAttemptDelay = 5;  // COMM_PET_ATTEMPT_DELAY (seconds)
    static constexpr uint8_t  kPetitionRetryCount   = 2;  // COMM_PET_RETRY_COUNT
    static constexpr uint32_t kPetitionRetryDelay   = 1;  // COMM_PET_RETRY_DELAY (seconds)
    static constexpr uint32_t kKeepAliveTimeout     = 50; // TIMEOUT_COMM_PET (seconds)
    static constexpr uint32_t kRemoveJoinerDelay    = 20; // Delay to remove successfully joined joiner

    static constexpr uint32_t kJoinerSessionTimeoutMillis =
        1000 * OPENTHREAD_CONFIG_COMMISSIONER_JOINER_SESSION_TIMEOUT; // Expiration time for active Joiner session

    enum ResignMode : uint8_t
    {
        kSendKeepAliveToResign,
        kDoNotSendKeepAlive,
    };

    struct Joiner
    {
        enum Type : uint8_t
        {
            kTypeUnused = 0, // Need to be 0 to ensure `memset()` clears all `Joiners`
            kTypeAny,
            kTypeEui64,
            kTypeDiscerner,
        };

        TimeMilli mExpirationTime;

        union
        {
            Mac::ExtAddress mEui64;
            JoinerDiscerner mDiscerner;
        } mSharedId;

        JoinerPskd mPskd;
        Type       mType;

        void CopyToJoinerInfo(otJoinerInfo &aJoiner) const;
    };

    Error   Stop(ResignMode aResignMode);
    Joiner *GetUnusedJoinerEntry(void);
    Joiner *FindJoinerEntry(const Mac::ExtAddress *aEui64);
    Joiner *FindJoinerEntry(const JoinerDiscerner &aDiscerner);
    Joiner *FindBestMatchingJoinerEntry(const Mac::ExtAddress &aReceivedJoinerId);
    void    RemoveJoinerEntry(Joiner &aJoiner);

    Error AddJoiner(const Mac::ExtAddress *aEui64,
                    const JoinerDiscerner *aDiscerner,
                    const char            *aPskd,
                    uint32_t               aTimeout);
    Error RemoveJoiner(const Mac::ExtAddress *aEui64, const JoinerDiscerner *aDiscerner, uint32_t aDelay);
    void  RemoveJoiner(Joiner &aJoiner, uint32_t aDelay);

    void HandleTimer(void);
    void HandleJoinerExpirationTimer(void);

    static void HandleMgmtCommissionerSetResponse(void                *aContext,
                                                  otMessage           *aMessage,
                                                  const otMessageInfo *aMessageInfo,
                                                  Error                aResult);
    void        HandleMgmtCommissionerSetResponse(Coap::Message          *aMessage,
                                                  const Ip6::MessageInfo *aMessageInfo,
                                                  Error                   aResult);
    static void HandleMgmtCommissionerGetResponse(void                *aContext,
                                                  otMessage           *aMessage,
                                                  const otMessageInfo *aMessageInfo,
                                                  Error                aResult);
    void        HandleMgmtCommissionerGetResponse(Coap::Message          *aMessage,
                                                  const Ip6::MessageInfo *aMessageInfo,
                                                  Error                   aResult);
    static void HandleLeaderPetitionResponse(void                *aContext,
                                             otMessage           *aMessage,
                                             const otMessageInfo *aMessageInfo,
                                             Error                aResult);
    void HandleLeaderPetitionResponse(Coap::Message *aMessage, const Ip6::MessageInfo *aMessageInfo, Error aResult);
    static void HandleLeaderKeepAliveResponse(void                *aContext,
                                              otMessage           *aMessage,
                                              const otMessageInfo *aMessageInfo,
                                              Error                aResult);
    void HandleLeaderKeepAliveResponse(Coap::Message *aMessage, const Ip6::MessageInfo *aMessageInfo, Error aResult);

    static void HandleSecureAgentConnectEvent(SecureTransport::ConnectEvent aEvent, void *aContext);
    void        HandleSecureAgentConnectEvent(SecureTransport::ConnectEvent aEvent);

    template <Uri kUri> void HandleTmf(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo);

    void HandleRelayReceive(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo);

    void HandleJoinerSessionTimer(void);

    void SendJoinFinalizeResponse(const Coap::Message &aRequest, StateTlv::State aState);

    static Error SendRelayTransmit(void *aContext, Message &aMessage, const Ip6::MessageInfo &aMessageInfo);
    Error        SendRelayTransmit(Message &aMessage, const Ip6::MessageInfo &aMessageInfo);

    void  ComputeBloomFilter(SteeringData &aSteeringData) const;
    void  SendCommissionerSet(void);
    Error SendPetition(void);
    void  SendKeepAlive(void);
    void  SendKeepAlive(uint16_t aSessionId);

    void SetState(State aState);
    void SignalJoinerEvent(JoinerEvent aEvent, const Joiner *aJoiner) const;
    void LogJoinerEntry(const char *aAction, const Joiner &aJoiner) const;

    static const char *StateToString(State aState);

    using JoinerExpirationTimer = TimerMilliIn<Commissioner, &Commissioner::HandleJoinerExpirationTimer>;
    using CommissionerTimer     = TimerMilliIn<Commissioner, &Commissioner::HandleTimer>;
    using JoinerSessionTimer    = TimerMilliIn<Commissioner, &Commissioner::HandleJoinerSessionTimer>;

    Joiner mJoiners[OPENTHREAD_CONFIG_COMMISSIONER_MAX_JOINER_ENTRIES];

    Joiner                  *mActiveJoiner;
    Ip6::InterfaceIdentifier mJoinerIid;
    uint16_t                 mJoinerPort;
    uint16_t                 mJoinerRloc;
    uint16_t                 mSessionId;
    uint8_t                  mTransmitAttempts;
    JoinerExpirationTimer    mJoinerExpirationTimer;
    CommissionerTimer        mTimer;
    JoinerSessionTimer       mJoinerSessionTimer;

    AnnounceBeginClient mAnnounceBegin;
    EnergyScanClient    mEnergyScan;
    PanIdQueryClient    mPanIdQuery;

    Ip6::Netif::UnicastAddress mCommissionerAloc;

    ProvisioningUrlTlv::StringType mProvisioningUrl;
    CommissionerIdTlv::StringType  mCommissionerId;

    State mState;

    Callback<StateCallback>  mStateCallback;
    Callback<JoinerCallback> mJoinerCallback;
};

DeclareTmfHandler(Commissioner, kUriDatasetChanged);
DeclareTmfHandler(Commissioner, kUriRelayRx);
DeclareTmfHandler(Commissioner, kUriJoinerFinalize);

} // namespace MeshCoP

DefineMapEnum(otCommissionerState, MeshCoP::Commissioner::State);
DefineMapEnum(otCommissionerJoinerEvent, MeshCoP::Commissioner::JoinerEvent);

} // namespace ot

#endif // OPENTHREAD_FTD && OPENTHREAD_CONFIG_COMMISSIONER_ENABLE

#endif // COMMISSIONER_HPP_
