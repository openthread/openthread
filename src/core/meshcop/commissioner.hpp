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

#include <openthread/commissioner.h>

#include "coap/coap.hpp"
#include "coap/coap_secure.hpp"
#include "common/locator.hpp"
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

class Commissioner : public InstanceLocator
{
public:
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
     * @retval OT_ERROR_NONE           Successfully started the Commissioner service.
     * @retval OT_ERROR_ALREADY        Commissioner is already started.
     * @retval OT_ERROR_INVALID_STATE  Device is not currently attached to a network.
     *
     */
    otError Start(otCommissionerStateCallback  aStateCallback,
                  otCommissionerJoinerCallback aJoinerCallback,
                  void *                       aCallbackContext);

    /**
     * This method stops the Commissioner service.
     *
     * @param[in]  aResign      Whether send LEAD_KA.req to resign as Commissioner
     *
     * @retval OT_ERROR_NONE     Successfully stopped the Commissioner service.
     * @retval OT_ERROR_ALREADY  Commissioner is already stopped.
     *
     */
    otError Stop(bool aResign);

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
     * @retval OT_ERROR_NONE           Successfully added the Joiner.
     * @retval OT_ERROR_NO_BUFS        No buffers available to add the Joiner.
     * @retval OT_ERROR_INVALID_STATE  Commissioner service is not started.
     *
     */
    otError AddJoinerAny(const char *aPskd, uint32_t aTimeout) { return AddJoiner(nullptr, nullptr, aPskd, aTimeout); }

    /**
     * This method adds a Joiner entry.
     *
     * @param[in]  aEui64        The Joiner's IEEE EUI-64.
     * @param[in]  aPskd         A pointer to the PSKd.
     * @param[in]  aTimeout      A time after which a Joiner is automatically removed, in seconds.
     *
     * @retval OT_ERROR_NONE           Successfully added the Joiner.
     * @retval OT_ERROR_NO_BUFS        No buffers available to add the Joiner.
     * @retval OT_ERROR_INVALID_STATE  Commissioner service is not started.
     *
     */
    otError AddJoiner(const Mac::ExtAddress &aEui64, const char *aPskd, uint32_t aTimeout)
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
     * @retval OT_ERROR_NONE           Successfully added the Joiner.
     * @retval OT_ERROR_NO_BUFS        No buffers available to add the Joiner.
     * @retval OT_ERROR_INVALID_STATE  Commissioner service is not started.
     *
     */
    otError AddJoiner(const JoinerDiscerner &aDiscerner, const char *aPskd, uint32_t aTimeout)
    {
        return AddJoiner(nullptr, &aDiscerner, aPskd, aTimeout);
    }

    /**
     * This method get joiner info at aIterator position.
     *
     * @param[inout]    aIterator   A iterator to the index of the joiner.
     * @param[out]      aJoiner     A reference to Joiner info.
     *
     * @retval OT_ERROR_NONE        Successfully get the Joiner info.
     * @retval OT_ERROR_NOT_FOUND   Not found next Joiner.
     *
     */
    otError GetNextJoinerInfo(uint16_t &aIterator, otJoinerInfo &aJoiner) const;

    /**
     * This method removes a Joiner entry accepting any Joiner.
     *
     * @param[in]  aDelay         The delay to remove Joiner (in seconds).
     *
     * @retval OT_ERROR_NONE           Successfully added the Joiner.
     * @retval OT_ERROR_NOT_FOUND      The Joiner entry accepting any Joiner was not found.
     * @retval OT_ERROR_INVALID_STATE  Commissioner service is not started.
     *
     */
    otError RemoveJoinerAny(uint32_t aDelay) { return RemoveJoiner(nullptr, nullptr, aDelay); }

    /**
     * This method removes a Joiner entry.
     *
     * @param[in]  aEui64         The Joiner's IEEE EUI-64.
     * @param[in]  aDelay         The delay to remove Joiner (in seconds).
     *
     * @retval OT_ERROR_NONE           Successfully added the Joiner.
     * @retval OT_ERROR_NOT_FOUND      The Joiner specified by @p aEui64 was not found.
     * @retval OT_ERROR_INVALID_STATE  Commissioner service is not started.
     *
     */
    otError RemoveJoiner(const Mac::ExtAddress &aEui64, uint32_t aDelay)
    {
        return RemoveJoiner(&aEui64, nullptr, aDelay);
    }

    /**
     * This method removes a Joiner entry.
     *
     * @param[in]  aDiscerner     A Joiner Discerner.
     * @param[in]  aDelay         The delay to remove Joiner (in seconds).
     *
     * @retval OT_ERROR_NONE           Successfully added the Joiner.
     * @retval OT_ERROR_NOT_FOUND      The Joiner specified by @p aEui64 was not found.
     * @retval OT_ERROR_INVALID_STATE  Commissioner service is not started.
     *
     */
    otError RemoveJoiner(const JoinerDiscerner &aDiscerner, uint32_t aDelay)
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
     * @param[in]  aProvisioningUrl  A pointer to the Provisioning URL (may be nullptr to set URL to empty string).
     *
     * @retval OT_ERROR_NONE          Successfully set the Provisioning URL.
     * @retval OT_ERROR_INVALID_ARGS  @p aProvisioningUrl is invalid (too long).
     *
     */
    otError SetProvisioningUrl(const char *aProvisioningUrl);

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
    bool IsActive(void) const { return mState == OT_COMMISSIONER_STATE_ACTIVE; }

    /**
     * This method indicates whether or not the Commissioner role is disabled.
     *
     * @returns TRUE if the Commissioner role is disabled, FALSE otherwise.
     *
     */
    bool IsDisabled(void) const { return mState == OT_COMMISSIONER_STATE_DISABLED; }

    /**
     * This function returns the Commissioner State.
     *
     * @param[in]  aInstance  A pointer to an OpenThread instance.
     *
     * @retval OT_COMMISSIONER_STATE_DISABLED  Commissioner disabled.
     * @retval OT_COMMISSIONER_STATE_PETITION  Becoming the commissioner.
     * @retval OT_COMMISSIONER_STATE_ACTIVE    Commissioner enabled.
     *
     */
    otCommissionerState GetState(void) const { return mState; }

    /**
     * This method sends MGMT_COMMISSIONER_GET.
     *
     * @param[in]  aTlvs        A pointer to Commissioning Data TLVs.
     * @param[in]  aLength      The length of requested TLVs in bytes.
     *
     * @retval OT_ERROR_NONE           Send MGMT_COMMISSIONER_GET successfully.
     * @retval OT_ERROR_NO_BUFS        Insufficient buffer space to send.
     * @retval OT_ERROR_INVALID_STATE  Commissioner service is not started.
     *
     */
    otError SendMgmtCommissionerGetRequest(const uint8_t *aTlvs, uint8_t aLength);

    /**
     * This method sends MGMT_COMMISSIONER_SET.
     *
     * @param[in]  aDataset     A reference to Commissioning Data.
     * @param[in]  aTlvs        A pointer to user specific Commissioning Data TLVs.
     * @param[in]  aLength      The length of user specific TLVs in bytes.
     *
     * @retval OT_ERROR_NONE           Send MGMT_COMMISSIONER_SET successfully.
     * @retval OT_ERROR_NO_BUFS        Insufficient buffer space to send.
     * @retval OT_ERROR_INVALID_STATE  Commissioner service is not started.
     *
     */
    otError SendMgmtCommissionerSetRequest(const otCommissioningDataset &aDataset,
                                           const uint8_t *               aTlvs,
                                           uint8_t                       aLength);

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
    enum
    {
        kPetitionAttemptDelay = 5,  ///< COMM_PET_ATTEMPT_DELAY (seconds)
        kPetitionRetryCount   = 2,  ///< COMM_PET_RETRY_COUNT
        kPetitionRetryDelay   = 1,  ///< COMM_PET_RETRY_DELAY (seconds)
        kKeepAliveTimeout     = 50, ///< TIMEOUT_COMM_PET (seconds)
        kRemoveJoinerDelay    = 20, ///< Delay to remove successfully joined joiner
    };

    struct Joiner
    {
        enum Type
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

        void CopyToJoinerInfo(otJoinerInfo &aInfo) const;
    };

    Joiner *GetUnusedJoinerEntry(void);
    Joiner *FindJoinerEntry(const Mac::ExtAddress *aEui64);
    Joiner *FindJoinerEntry(const JoinerDiscerner &aDiscerner);
    Joiner *FindBestMatchingJoinerEntry(const Mac::ExtAddress &aRxJoinerId);
    void    RemoveJoinerEntry(Joiner &aJoiner);

    otError AddJoiner(const Mac::ExtAddress *aEui64,
                      const JoinerDiscerner *aDiscerner,
                      const char *           aPskd,
                      uint32_t               aTimeout);
    otError RemoveJoiner(const Mac::ExtAddress *aEui64, const JoinerDiscerner *aDiscerner, uint32_t aDelay);
    void    RemoveJoiner(Joiner &aJoiner, uint32_t aDelay);

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
                                                  otError              aResult);
    void        HandleMgmtCommissionerSetResponse(Coap::Message *         aMessage,
                                                  const Ip6::MessageInfo *aMessageInfo,
                                                  otError                 aResult);
    static void HandleMgmtCommissionerGetResponse(void *               aContext,
                                                  otMessage *          aMessage,
                                                  const otMessageInfo *aMessageInfo,
                                                  otError              aResult);
    void        HandleMgmtCommissionerGetResponse(Coap::Message *         aMessage,
                                                  const Ip6::MessageInfo *aMessageInfo,
                                                  otError                 aResult);
    static void HandleLeaderPetitionResponse(void *               aContext,
                                             otMessage *          aMessage,
                                             const otMessageInfo *aMessageInfo,
                                             otError              aResult);
    void HandleLeaderPetitionResponse(Coap::Message *aMessage, const Ip6::MessageInfo *aMessageInfo, otError aResult);
    static void HandleLeaderKeepAliveResponse(void *               aContext,
                                              otMessage *          aMessage,
                                              const otMessageInfo *aMessageInfo,
                                              otError              aResult);
    void HandleLeaderKeepAliveResponse(Coap::Message *aMessage, const Ip6::MessageInfo *aMessageInfo, otError aResult);

    static void HandleCoapsConnected(bool aConnected, void *aContext);
    void        HandleCoapsConnected(bool aConnected);

    static void HandleRelayReceive(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo);
    void        HandleRelayReceive(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo);

    static void HandleDatasetChanged(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo);
    void        HandleDatasetChanged(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo);

    static void HandleJoinerFinalize(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo);
    void        HandleJoinerFinalize(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo);

    void SendJoinFinalizeResponse(const Coap::Message &aRequest, StateTlv::State aState);

    static otError SendRelayTransmit(void *aContext, Message &aMessage, const Ip6::MessageInfo &aMessageInfo);
    otError        SendRelayTransmit(Message &aMessage, const Ip6::MessageInfo &aMessageInfo);

    void    ComputeBloomFilter(SteeringData &aSteeringData) const;
    void    SendCommissionerSet(void);
    otError SendPetition(void);
    void    SendKeepAlive(void);
    void    SendKeepAlive(uint16_t aSessionId);

    void SetState(otCommissionerState aState);
    void SignalJoinerEvent(otCommissionerJoinerEvent aEvent, const Joiner *aJoiner) const;
    void LogJoinerEntry(const char *aAction, const Joiner &aJoiner) const;

    static const char *StateToString(otCommissionerState aState);

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

    Ip6::NetifUnicastAddress mCommissionerAloc;

    char mProvisioningUrl[OT_PROVISIONING_URL_MAX_SIZE + 1]; // + 1 is for null char at end of string.

    otCommissionerState mState;

    otCommissionerStateCallback  mStateCallback;
    otCommissionerJoinerCallback mJoinerCallback;
    void *                       mCallbackContext;
};

} // namespace MeshCoP
} // namespace ot

#endif // COMMISSIONER_HPP_
