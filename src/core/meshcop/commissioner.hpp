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

#include <openthread/commissioner.h>

#include "openthread-core-config.h"
#include "coap/coap.hpp"
#include "coap/coap_secure.hpp"
#include "common/timer.hpp"
#include "mac/mac_frame.hpp"
#include "meshcop/announce_begin_client.hpp"
#include "meshcop/dtls.hpp"
#include "meshcop/energy_scan_client.hpp"
#include "meshcop/panid_query_client.hpp"
#include "net/udp6.hpp"
#include "thread/mle.hpp"

namespace ot {

class ThreadNetif;

namespace MeshCoP {

class Commissioner
{
public:
    /**
     * This constructor initializes the Commissioner object.
     *
     * @param[in]  aThreadNetif  A reference to the Thread network interface.
     *
     */
    Commissioner(ThreadNetif &aThreadNetif);

    /**
     * This method returns the pointer to the parent otInstance structure.
     *
     * @returns The pointer to the parent otInstance structure.
     *
     */
    otInstance *GetInstance(void);

    /**
     * This method starts the Commissioner service.
     *
     * @retval OT_ERROR_NONE  Successfully started the Commissioner service.
     *
     */
    otError Start(void);

    /**
     * This method stops the Commissioner service.
     *
     * @retval OT_ERROR_NONE  Successfully stopped the Commissioner service.
     *
     */
    otError Stop(void);

    /**
     * This method clears all Joiner entries.
     *
     */
    void ClearJoiners(void);

    /**
     * This method adds a Joiner entry.
     *
     * @param[in]  aExtAddress      A pointer to the Joiner's extended address or NULL for any Joiner.
     * @param[in]  aPSKd            A pointer to the PSKd.
     * @param[in]  aTimeout         A time after which a Joiner is automatically removed, in seconds.
     *
     * @retval OT_ERROR_NONE     Successfully added the Joiner.
     * @retval OT_ERROR_NO_BUFS  No buffers available to add the Joiner.
     *
     */
    otError AddJoiner(const Mac::ExtAddress *aExtAddress, const char *aPSKd, uint32_t aTimeout);

    /**
     * This method removes a Joiner entry.
     *
     * @param[in]  aExtAddress        A pointer to the Joiner's extended address or NULL for any Joiner.
     * @param[in]  aDelay             The delay to remove Joiner (in seconds).
     *
     * @retval OT_ERROR_NONE       Successfully added the Joiner.
     * @retval OT_ERROR_NOT_FOUND  The Joiner specified by @p aExtAddress was not found.
     *
     */
    otError RemoveJoiner(const Mac::ExtAddress *aExtAddress, uint32_t aDelay);

    /**
     * This method sets the Provisioning URL.
     *
     * @param[in]  aProvisioningUrl  A pointer to the Provisioning URL (may be NULL).
     *
     * @retval OT_ERROR_NONE          Successfully added the Joiner.
     * @retval OT_ERROR_INVALID_ARGS  @p aProvisioningUrl is invalid.
     *
     */
    otError SetProvisioningUrl(const char *aProvisioningUrl);

    /**
     * This method returns the Commissioner Session ID.
     *
     * @returns The Commissioner Session ID.
     *
     */
    uint16_t GetSessionId(void) const;

    /**
     * This function returns the Commissioner State.
     *
     * @param[in]  aInstance  A pointer to an OpenThread instance.
     *
     * @retval kCommissionerStateDisabled    Commissioner disabled.
     * @retval kCommissionerStatePetition    Becoming the commissioner.
     * @retval kCommissionerStateActive      Commissioner enabled.
     *
     */
    otCommissionerState GetState(void) const;

    /**
     * This method sends MGMT_COMMISSIONER_GET.
     *
     * @param[in]  aTlvs        A pointer to Commissioning Data TLVs.
     * @param[in]  aLength      The length of requested TLVs in bytes.
     *
     * @retval OT_ERROR_NONE     Send MGMT_COMMISSIONER_GET successfully.
     * @retval OT_ERROR_FAILED   Send MGMT_COMMISSIONER_GET fail.
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
     * @retval OT_ERROR_NONE     Send MGMT_COMMISSIONER_SET successfully.
     * @retval OT_ERROR_FAILED   Send MGMT_COMMISSIONER_SET fail.
     *
     */
    otError SendMgmtCommissionerSetRequest(const otCommissioningDataset &aDataset,
                                           const uint8_t *aTlvs, uint8_t aLength);

    /**
     * This static method generates PSKc.
     *
     * PSKc is used to establish the Commissioner Session.
     *
     * @param[in]  aPassPhrase   The commissioning passphrase.
     * @param[in]  aNetworkName  The network name for PSKc computation.
     * @param[in]  aExtPanId     The extended pan id for PSKc computation.
     * @param[out] aPSKc         A pointer to where the generated PSKc will be placed.
     *
     * @retval OT_ERROR_NONE          Successfully generate PSKc.
     * @retval OT_ERROR_INVALID_ARGS  If the length of passphrase is out of range.
     *
     */
    static otError GeneratePSKc(const char *aPassPhrase, const char *aNetworkName, const uint8_t *aExtPanId,
                                uint8_t *aPSKc);

    AnnounceBeginClient mAnnounceBegin;
    EnergyScanClient mEnergyScan;
    PanIdQueryClient mPanIdQuery;

private:
    enum
    {
        kPetitionAttemptDelay = 5,      ///< COMM_PET_ATTEMPT_DELAY (seconds)
        kPetitionRetryCount   = 2,      ///< COMM_PET_RETRY_COUNT
        kPetitionRetryDelay   = 1,      ///< COMM_PET_RETRY_DELAY (seconds)
        kKeepAliveTimeout     = 50,     ///< TIMEOUT_COMM_PET (seconds)
        kRemoveJoinerDelay    = 20,     ///< Delay to remove successfully joined joiner
    };

    void AddCoapResources(void);
    void RemoveCoapResources(void);

    static void HandleTimer(void *aContext);
    void HandleTimer(void);

    static void HandleJoinerExpirationTimer(void *aContext);
    void HandleJoinerExpirationTimer(void);

    void UpdateJoinerExpirationTimer(void);

    static void HandleMgmtCommissionerSetResponse(void *aContext, otCoapHeader *aHeader, otMessage *aMessage,
                                                  const otMessageInfo *aMessageInfo, otError aResult);
    void HandleMgmtCommissisonerSetResponse(Coap::Header *aHeader, Message *aMessage,
                                            const Ip6::MessageInfo *aMessageInfo, otError aResult);
    static void HandleMgmtCommissionerGetResponse(void *aContext, otCoapHeader *aHeader, otMessage *aMessage,
                                                  const otMessageInfo *aMessageInfo, otError aResult);
    void HandleMgmtCommissisonerGetResponse(Coap::Header *aHeader, Message *aMessage,
                                            const Ip6::MessageInfo *aMessageInfo, otError aResult);
    static void HandleLeaderPetitionResponse(void *aContext, otCoapHeader *aHeader, otMessage *aMessage,
                                             const otMessageInfo *aMessageInfo, otError aResult);
    void HandleLeaderPetitionResponse(Coap::Header *aHeader, Message *aMessage,
                                      const Ip6::MessageInfo *aMessageInfo, otError aResult);
    static void HandleLeaderKeepAliveResponse(void *aContext, otCoapHeader *aHeader, otMessage *aMessage,
                                              const otMessageInfo *aMessageInfo, otError aResult);
    void HandleLeaderKeepAliveResponse(Coap::Header *aHeader, Message *aMessage,
                                       const Ip6::MessageInfo *aMessageInfo, otError aResult);

    static void HandleRelayReceive(void *aContext, otCoapHeader *aHeader, otMessage *aMessage,
                                   const otMessageInfo *aMessageInfo);
    void HandleRelayReceive(Coap::Header &aHeader, Message &aMessage, const Ip6::MessageInfo &aMessageInfo);

    static void HandleDatasetChanged(void *aContext, otCoapHeader *aHeader, otMessage *aMessage,
                                     const otMessageInfo *aMessageInfo);
    void HandleDatasetChanged(Coap::Header &aHeader, Message &aMessage, const Ip6::MessageInfo &aMessageInfo);

    static void HandleJoinerFinalize(void *aContext, otCoapHeader *aHeader, otMessage *aMessage,
                                     const otMessageInfo *aMessageInfo);
    void HandleJoinerFinalize(Coap::Header &aHeader, Message &aMessage, const Ip6::MessageInfo &aMessageInfo);

    void SendJoinFinalizeResponse(const Coap::Header &aRequestHeader, StateTlv::State aState);

    static otError SendRelayTransmit(void *aContext, Message &aMessage, const Ip6::MessageInfo &aMessageInfo);
    otError SendRelayTransmit(Message &aMessage, const Ip6::MessageInfo &aMessageInfo);

    otError SendCommissionerSet(void);
    otError SendPetition(void);
    otError SendKeepAlive(void);

    otCommissionerState mState;

    struct Joiner
    {
        Mac::ExtAddress mExtAddress;
        uint32_t mExpirationTime;
        char mPsk[Dtls::kPskMaxLength + 1];
        bool mValid : 1;
        bool mAny : 1;
    };
    Joiner mJoiners[OPENTHREAD_CONFIG_MAX_JOINER_ENTRIES];

    union
    {
        uint8_t mJoinerIid[8];
        uint64_t mJoinerIid64;
    };
    uint16_t mJoinerPort;
    uint16_t mJoinerRloc;
    Timer mJoinerExpirationTimer;

    Timer mTimer;
    uint16_t mSessionId;
    uint8_t mTransmitAttempts;

    Coap::Resource mRelayReceive;
    Coap::Resource mDatasetChanged;
    Coap::Resource mJoinerFinalize;

    ThreadNetif &mNetif;
};

}  // namespace MeshCoP
}  // namespace ot

#endif  // COMMISSIONER_HPP_
