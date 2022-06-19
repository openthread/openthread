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

#include "coap/coap.hpp"
#include "coap/coap_secure.hpp"
#include "common/as_core_type.hpp"
#include "common/clearable.hpp"
#include "common/locator.hpp"
#include "common/log.hpp"
#include "common/non_copyable.hpp"
#include "common/timer.hpp"
#include "mac/mac_types.hpp"
#include "meshcop/announce_begin_client.hpp"
#include "meshcop/dtls.hpp"
#include "meshcop/energy_scan_client.hpp"
#include "meshcop/panid_query_client.hpp"
#include "net/ip6_address.hpp"
#include "net/udp6.hpp"
#include "thread/key_manager.hpp"
#include "thread/mle.hpp"

namespace ot {

namespace MeshCoP {

class Commissioner : public InstanceLocator, private NonCopyable
{
public:
    /**
     * This enumeration type represents the Commissioner State.
     *
     */
    enum State : uint8_t
    {
        kStateDisabled = OT_COMMISSIONER_STATE_DISABLED, ///< Disabled.
        kStatePetition = OT_COMMISSIONER_STATE_PETITION, ///< Petitioning to become a Commissioner.
        kStateActive   = OT_COMMISSIONER_STATE_ACTIVE,   ///< Active Commissioner.
    };

    /**
     * This enumeration type represents Joiner Event.
     *
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
     * This type represents a Commissioning Dataset.
     *
     */
    class Dataset : public otCommissioningDataset, public Clearable<Dataset>
    {
    public:
        /**
         * This method indicates whether or not the Border Router RLOC16 Locator is set in the Dataset.
         *
         * @returns TRUE if Border Router RLOC16 Locator is set, FALSE otherwise.
         *
         */
        bool IsLocatorSet(void) const { return mIsLocatorSet; }

        /**
         * This method gets the Border Router RLOC16 Locator in the Dataset.
         *
         * This method MUST be used when Locator is set in the Dataset, otherwise its behavior is undefined.
         *
         * @returns The Border Router RLOC16 Locator in the Dataset.
         *
         */
        uint16_t GetLocator(void) const { return mLocator; }

        /**
         * This method sets the Border Router RLOCG16 Locator in the Dataset.
         *
         * @param[in] aLocator  A Locator.
         *
         */
        void SetLocator(uint16_t aLocator)
        {
            mIsLocatorSet = true;
            mLocator      = aLocator;
        }

        /**
         * This method indicates whether or not the Session ID is set in the Dataset.
         *
         * @returns TRUE if Session ID is set, FALSE otherwise.
         *
         */
        bool IsSessionIdSet(void) const { return mIsSessionIdSet; }

        /**
         * This method gets the Session ID in the Dataset.
         *
         * This method MUST be used when Session ID is set in the Dataset, otherwise its behavior is undefined.
         *
         * @returns The Session ID in the Dataset.
         *
         */
        uint16_t GetSessionId(void) const { return mSessionId; }

        /**
         * This method sets the Session ID in the Dataset.
         *
         * @param[in] aSessionId  The Session ID.
         *
         */
        void SetSessionId(uint16_t aSessionId)
        {
            mIsSessionIdSet = true;
            mSessionId      = aSessionId;
        }

        /**
         * This method indicates whether or not the Steering Data is set in the Dataset.
         *
         * @returns TRUE if Steering Data is set, FALSE otherwise.
         *
         */
        bool IsSteeringDataSet(void) const { return mIsSteeringDataSet; }

        /**
         * This method gets the Steering Data in the Dataset.
         *
         * This method MUST be used when Steering Data is set in the Dataset, otherwise its behavior is undefined.
         *
         * @returns The Steering Data in the Dataset.
         *
         */
        const SteeringData &GetSteeringData(void) const { return AsCoreType(&mSteeringData); }

        /**
         * This method returns a reference to the Steering Data in the Dataset to be updated by caller.
         *
         * @returns A reference to the Steering Data in the Dataset.
         *
         */
        SteeringData &UpdateSteeringData(void)
        {
            mIsSteeringDataSet = true;
            return AsCoreType(&mSteeringData);
        }

        /**
         * This method indicates whether or not the Joiner UDP port is set in the Dataset.
         *
         * @returns TRUE if Joiner UDP port is set, FALSE otherwise.
         *
         */
        bool IsJoinerUdpPortSet(void) const { return mIsJoinerUdpPortSet; }

        /**
         * This method gets the Joiner UDP port in the Dataset.
         *
         * This method MUST be used when Joiner UDP port is set in the Dataset, otherwise its behavior is undefined.
         *
         * @returns The Joiner UDP port in the Dataset.
         *
         */
        uint16_t GetJoinerUdpPort(void) const { return mJoinerUdpPort; }

        /**
         * This method sets the Joiner UDP Port in the Dataset.
         *
         * @param[in] aJoinerUdpPort  The Joiner UDP Port.
         *
         */
        void SetJoinerUdpPort(uint16_t aJoinerUdpPort)
        {
            mIsJoinerUdpPortSet = true;
            mJoinerUdpPort      = aJoinerUdpPort;
        }
    };

    /**
     * This constructor initializes the Commissioner object.
     *
     * @param[in]  aInstance     A reference to the OpenThread instance.
     *
     */
    explicit Commissioner(Instance &aInstance);

    /**
     * This method starts the Commissioner service.
     *
     * @param[in]  aStateCallback    A pointer to a function that is called when the commissioner state changes.
     * @param[in]  aJoinerCallback   A pointer to a function that is called when a joiner event occurs.
     * @param[in]  aCallbackContext  A pointer to application-specific context.
     *
     * @retval kErrorNone           Successfully started the Commissioner service.
     * @retval kErrorAlready        Commissioner is already started.
     * @retval kErrorInvalidState   Device is not currently attached to a network.
     *
     */
    Error Start(StateCallback aStateCallback, JoinerCallback aJoinerCallback, void *aCallbackContext);

    /**
     * This method stops the Commissioner service.
     *
     * @retval kErrorNone     Successfully stopped the Commissioner service.
     * @retval kErrorAlready  Commissioner is already stopped.
     *
     */
    Error Stop(void) { return Stop(kSendKeepAliveToResign); }

    /**
     * This method returns the Commissioner Id.
     *
     * @returns The Commissioner Id.
     *
     */
    const char *GetId(void) const { return mCommissionerId; }

    /**
     * This method sets the Commissioner Id.
     *
     * @param[in]  aId   A pointer to a string character array. Must be null terminated.
     *
     * @retval kErrorNone           Successfully set the Commissioner Id.
     * @retval kErrorInvalidArgs    Given name is too long.
     * @retval kErrorInvalidState   The commissioner is active and id cannot be changed.
     *
     */
    Error SetId(const char *aId);

    /**
     * This method clears all Joiner entries.
     *
     */
    void ClearJoiners(void);

    /**
     * This method adds a Joiner entry accepting any Joiner.
     *
     * @param[in]  aPskd         A pointer to the PSKd.
     * @param[in]  aTimeout      A time after which a Joiner is automatically removed, in seconds.
     *
     * @retval kErrorNone          Successfully added the Joiner.
     * @retval kErrorNoBufs        No buffers available to add the Joiner.
     * @retval kErrorInvalidState  Commissioner service is not started.
     *
     */
    Error AddJoinerAny(const char *aPskd, uint32_t aTimeout) { return AddJoiner(nullptr, nullptr, aPskd, aTimeout); }

    /**
     * This method adds a Joiner entry.
     *
     * @param[in]  aEui64        The Joiner's IEEE EUI-64.
     * @param[in]  aPskd         A pointer to the PSKd.
     * @param[in]  aTimeout      A time after which a Joiner is automatically removed, in seconds.
     *
     * @retval kErrorNone          Successfully added the Joiner.
     * @retval kErrorNoBufs        No buffers available to add the Joiner.
     * @retval kErrorInvalidState  Commissioner service is not started.
     *
     */
    Error AddJoiner(const Mac::ExtAddress &aEui64, const char *aPskd, uint32_t aTimeout)
    {
        return AddJoiner(&aEui64, nullptr, aPskd, aTimeout);
    }

    /**
     * This method adds a Joiner entry with a Joiner Discerner.
     *
     * @param[in]  aDiscerner  A Joiner Discerner.
     * @param[in]  aPskd       A pointer to the PSKd.
     * @param[in]  aTimeout    A time after which a Joiner is automatically removed, in seconds.
     *
     * @retval kErrorNone          Successfully added the Joiner.
     * @retval kErrorNoBufs        No buffers available to add the Joiner.
     * @retval kErrorInvalidState  Commissioner service is not started.
     *
     */
    Error AddJoiner(const JoinerDiscerner &aDiscerner, const char *aPskd, uint32_t aTimeout)
    {
        return AddJoiner(nullptr, &aDiscerner, aPskd, aTimeout);
    }

    /**
     * This method get joiner info at aIterator position.
     *
     * @param[in,out]   aIterator   A iterator to the index of the joiner.
     * @param[out]      aJoiner     A reference to Joiner info.
     *
     * @retval kErrorNone       Successfully get the Joiner info.
     * @retval kErrorNotFound   Not found next Joiner.
     *
     */
    Error GetNextJoinerInfo(uint16_t &aIterator, otJoinerInfo &aJoiner) const;

    /**
     * This method removes a Joiner entry accepting any Joiner.
     *
     * @param[in]  aDelay         The delay to remove Joiner (in seconds).
     *
     * @retval kErrorNone          Successfully added the Joiner.
     * @retval kErrorNotFound      The Joiner entry accepting any Joiner was not found.
     * @retval kErrorInvalidState  Commissioner service is not started.
     *
     */
    Error RemoveJoinerAny(uint32_t aDelay) { return RemoveJoiner(nullptr, nullptr, aDelay); }

    /**
     * This method removes a Joiner entry.
     *
     * @param[in]  aEui64         The Joiner's IEEE EUI-64.
     * @param[in]  aDelay         The delay to remove Joiner (in seconds).
     *
     * @retval kErrorNone          Successfully added the Joiner.
     * @retval kErrorNotFound      The Joiner specified by @p aEui64 was not found.
     * @retval kErrorInvalidState  Commissioner service is not started.
     *
     */
    Error RemoveJoiner(const Mac::ExtAddress &aEui64, uint32_t aDelay)
    {
        return RemoveJoiner(&aEui64, nullptr, aDelay);
    }

    /**
     * This method removes a Joiner entry.
     *
     * @param[in]  aDiscerner     A Joiner Discerner.
     * @param[in]  aDelay         The delay to remove Joiner (in seconds).
     *
     * @retval kErrorNone          Successfully added the Joiner.
     * @retval kErrorNotFound      The Joiner specified by @p aEui64 was not found.
     * @retval kErrorInvalidState  Commissioner service is not started.
     *
     */
    Error RemoveJoiner(const JoinerDiscerner &aDiscerner, uint32_t aDelay)
    {
        return RemoveJoiner(nullptr, &aDiscerner, aDelay);
    }

    /**
     * This method gets the Provisioning URL.
     *
     * @returns A pointer to char buffer containing the URL string.
     *
     */
    const char *GetProvisioningUrl(void) const { return mProvisioningUrl; }

    /**
     * This method sets the Provisioning URL.
     *
     * @param[in]  aProvisioningUrl  A pointer to the Provisioning URL (may be `nullptr` to set URL to empty string).
     *
     * @retval kErrorNone         Successfully set the Provisioning URL.
     * @retval kErrorInvalidArgs  @p aProvisioningUrl is invalid (too long).
     *
     */
    Error SetProvisioningUrl(const char *aProvisioningUrl);

    /**
     * This method returns the Commissioner Session ID.
     *
     * @returns The Commissioner Session ID.
     *
     */
    uint16_t GetSessionId(void) const { return mSessionId; }

    /**
     * This method indicates whether or not the Commissioner role is active.
     *
     * @returns TRUE if the Commissioner role is active, FALSE otherwise.
     *
     */
    bool IsActive(void) const { return mState == kStateActive; }

    /**
     * This method indicates whether or not the Commissioner role is disabled.
     *
     * @returns TRUE if the Commissioner role is disabled, FALSE otherwise.
     *
     */
    bool IsDisabled(void) const { return mState == kStateDisabled; }

    /**
     * This method gets the Commissioner State.
     *
     * @returns The Commissioner State.
     *
     */
    State GetState(void) const { return mState; }

    /**
     * This method sends MGMT_COMMISSIONER_GET.
     *
     * @param[in]  aTlvs        A pointer to Commissioning Data TLVs.
     * @param[in]  aLength      The length of requested TLVs in bytes.
     *
     * @retval kErrorNone          Send MGMT_COMMISSIONER_GET successfully.
     * @retval kErrorNoBufs        Insufficient buffer space to send.
     * @retval kErrorInvalidState  Commissioner service is not started.
     *
     */
    Error SendMgmtCommissionerGetRequest(const uint8_t *aTlvs, uint8_t aLength);

    /**
     * This method sends MGMT_COMMISSIONER_SET.
     *
     * @param[in]  aDataset     A reference to Commissioning Data.
     * @param[in]  aTlvs        A pointer to user specific Commissioning Data TLVs.
     * @param[in]  aLength      The length of user specific TLVs in bytes.
     *
     * @retval kErrorNone          Send MGMT_COMMISSIONER_SET successfully.
     * @retval kErrorNoBufs        Insufficient buffer space to send.
     * @retval kErrorInvalidState  Commissioner service is not started.
     *
     */
    Error SendMgmtCommissionerSetRequest(const Dataset &aDataset, const uint8_t *aTlvs, uint8_t aLength);

    /**
     * This method returns a reference to the AnnounceBeginClient instance.
     *
     * @returns A reference to the AnnounceBeginClient instance.
     *
     */
    AnnounceBeginClient &GetAnnounceBeginClient(void) { return mAnnounceBegin; }

    /**
     * This method returns a reference to the EnergyScanClient instance.
     *
     * @returns A reference to the EnergyScanClient instance.
     *
     */
    EnergyScanClient &GetEnergyScanClient(void) { return mEnergyScan; }

    /**
     * This method returns a reference to the PanIdQueryClient instance.
     *
     * @returns A reference to the PanIdQueryClient instance.
     *
     */
    PanIdQueryClient &GetPanIdQueryClient(void) { return mPanIdQuery; }

    /**
     * This method applies the Mesh Local Prefix.
     *
     */
    void ApplyMeshLocalPrefix(void);

private:
    static constexpr uint32_t kPetitionAttemptDelay = 5;  // COMM_PET_ATTEMPT_DELAY (seconds)
    static constexpr uint8_t  kPetitionRetryCount   = 2;  // COMM_PET_RETRY_COUNT
    static constexpr uint32_t kPetitionRetryDelay   = 1;  // COMM_PET_RETRY_DELAY (seconds)
    static constexpr uint32_t kKeepAliveTimeout     = 50; // TIMEOUT_COMM_PET (seconds)
    static constexpr uint32_t kRemoveJoinerDelay    = 20; // Delay to remove successfully joined joiner

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
                    const char *           aPskd,
                    uint32_t               aTimeout);
    Error RemoveJoiner(const Mac::ExtAddress *aEui64, const JoinerDiscerner *aDiscerner, uint32_t aDelay);
    void  RemoveJoiner(Joiner &aJoiner, uint32_t aDelay);

    void AddCoapResources(void);
    void RemoveCoapResources(void);

    static void HandleTimer(Timer &aTimer);
    void        HandleTimer(void);

    static void HandleJoinerExpirationTimer(Timer &aTimer);
    void        HandleJoinerExpirationTimer(void);

    void UpdateJoinerExpirationTimer(void);

    static void HandleMgmtCommissionerSetResponse(void *               aContext,
                                                  otMessage *          aMessage,
                                                  const otMessageInfo *aMessageInfo,
                                                  Error                aResult);
    void        HandleMgmtCommissionerSetResponse(Coap::Message *         aMessage,
                                                  const Ip6::MessageInfo *aMessageInfo,
                                                  Error                   aResult);
    static void HandleMgmtCommissionerGetResponse(void *               aContext,
                                                  otMessage *          aMessage,
                                                  const otMessageInfo *aMessageInfo,
                                                  Error                aResult);
    void        HandleMgmtCommissionerGetResponse(Coap::Message *         aMessage,
                                                  const Ip6::MessageInfo *aMessageInfo,
                                                  Error                   aResult);
    static void HandleLeaderPetitionResponse(void *               aContext,
                                             otMessage *          aMessage,
                                             const otMessageInfo *aMessageInfo,
                                             Error                aResult);
    void HandleLeaderPetitionResponse(Coap::Message *aMessage, const Ip6::MessageInfo *aMessageInfo, Error aResult);
    static void HandleLeaderKeepAliveResponse(void *               aContext,
                                              otMessage *          aMessage,
                                              const otMessageInfo *aMessageInfo,
                                              Error                aResult);
    void HandleLeaderKeepAliveResponse(Coap::Message *aMessage, const Ip6::MessageInfo *aMessageInfo, Error aResult);

    static void HandleCoapsConnected(bool aConnected, void *aContext);
    void        HandleCoapsConnected(bool aConnected);

    static void HandleRelayReceive(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo);
    void        HandleRelayReceive(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo);

    static void HandleDatasetChanged(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo);
    void        HandleDatasetChanged(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo);

    static void HandleJoinerFinalize(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo);
    void        HandleJoinerFinalize(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo);

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

    Joiner mJoiners[OPENTHREAD_CONFIG_COMMISSIONER_MAX_JOINER_ENTRIES];

    Joiner *                 mActiveJoiner;
    Ip6::InterfaceIdentifier mJoinerIid;
    uint16_t                 mJoinerPort;
    uint16_t                 mJoinerRloc;
    uint16_t                 mSessionId;
    uint8_t                  mTransmitAttempts;
    TimerMilli               mJoinerExpirationTimer;
    TimerMilli               mTimer;

    Coap::Resource mRelayReceive;
    Coap::Resource mDatasetChanged;
    Coap::Resource mJoinerFinalize;

    AnnounceBeginClient mAnnounceBegin;
    EnergyScanClient    mEnergyScan;
    PanIdQueryClient    mPanIdQuery;

    Ip6::Netif::UnicastAddress mCommissionerAloc;

    char mProvisioningUrl[OT_PROVISIONING_URL_MAX_SIZE + 1]; // + 1 is for null char at end of string.
    char mCommissionerId[CommissionerIdTlv::kMaxLength + 1];

    State mState;

    StateCallback  mStateCallback;
    JoinerCallback mJoinerCallback;
    void *         mCallbackContext;
};

} // namespace MeshCoP

DefineMapEnum(otCommissionerState, MeshCoP::Commissioner::State);
DefineMapEnum(otCommissionerJoinerEvent, MeshCoP::Commissioner::JoinerEvent);
DefineCoreType(otCommissioningDataset, MeshCoP::Commissioner::Dataset);

} // namespace ot

#endif // OPENTHREAD_FTD && OPENTHREAD_CONFIG_COMMISSIONER_ENABLE

#endif // COMMISSIONER_HPP_
