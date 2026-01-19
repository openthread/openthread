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
 *  This file includes definitions for the Joiner role.
 */

#ifndef OT_CORE_MESHCOP_JOINER_HPP_
#define OT_CORE_MESHCOP_JOINER_HPP_

#include "openthread-core-config.h"

#if OPENTHREAD_CONFIG_JOINER_ENABLE

#include <openthread/joiner.h>

#include "coap/coap_message.hpp"
#include "coap/coap_secure.hpp"
#include "common/as_core_type.hpp"
#include "common/callback.hpp"
#include "common/locator.hpp"
#include "common/log.hpp"
#include "common/message.hpp"
#include "common/non_copyable.hpp"
#include "mac/mac_types.hpp"
#include "meshcop/meshcop.hpp"
#include "meshcop/meshcop_tlvs.hpp"
#include "meshcop/secure_transport.hpp"
#include "thread/discover_scanner.hpp"
#include "thread/tmf.hpp"

namespace ot {

namespace MeshCoP {

#if !OPENTHREAD_CONFIG_SECURE_TRANSPORT_ENABLE
#error "Joiner feature requires `OPENTHREAD_CONFIG_SECURE_TRANSPORT_ENABLE`"
#endif

class Joiner : public InstanceLocator, private NonCopyable
{
    friend class Tmf::Agent;

public:
    typedef otJoinerRetryCallback RetryCallback;      ///< Callback to notify the state of retrying join operation.
    typedef otJoinerCallback      CompletionCallback; ///< Callback to notify the completion of join operation.

    /**
     * Represents the Joiner State.
     */
    enum State : uint8_t
    {
        kStateIdle      = OT_JOINER_STATE_IDLE,      ///< Idle state.
        kStateDiscover  = OT_JOINER_STATE_DISCOVER,  ///< Discovering Joiner Routers (performing scan).
        kStateConnect   = OT_JOINER_STATE_CONNECT,   ///< Establishing a connection to commissioner.
        kStateConnected = OT_JOINER_STATE_CONNECTED, ///< Successfully connected to commissioner.
        kStateEntrust   = OT_JOINER_STATE_ENTRUST,   ///< Waiting to receive Joiner Entrust message.
        kStateJoined    = OT_JOINER_STATE_JOINED,    ///< Join completed successfully.
    };

    /**
     * Initializes the Joiner object.
     *
     * @param[in]  aInstance     A reference to the OpenThread instance.
     */
    explicit Joiner(Instance &aInstance);

    /**
     * Starts the Joiner service with retries.
     *
     * The progressively longer delays between retries follows an exponential back-off strategy.
     * The callback may be called multiple times and notify about the status after each join attempt.
     * When the join timeout is reached, `kErrorFailed` is passed to the callback to indicate the overall failure.
     *
     * @param[in]  aPskd             A pointer to the PSKd.
     * @param[in]  aProvisioningUrl  A pointer to the Provisioning URL (may be `nullptr`).
     * @param[in]  aRetryBaseDelay   The minimal delay (in milliseconds) used between join attempts.
     * @param[in]  aJoinTimeout      The overall timeout (in milliseconds) after the retrying joiner stops attempts.
     * @param[in]  aVendorName       A pointer to the Vendor Name (may be `nullptr`).
     * @param[in]  aVendorModel      A pointer to the Vendor Model (may be `nullptr`).
     * @param[in]  aVendorSwVersion  A pointer to the Vendor SW Version (may be `nullptr`).
     * @param[in]  aVendorData       A pointer to the Vendor Data (may be `nullptr`).
     * @param[in]  aCallback         A pointer to a function that is called when the join operation completes.
     * @param[in]  aContext          A pointer to application-specific context.
     *
     * @retval kErrorNone          Successfully started the Joiner service.
     * @retval kErrorBusy          The previous attempt is still on-going.
     * @retval kErrorInvalidState  The IPv6 stack is not enabled or Thread stack is fully enabled.
     */
    Error StartWithRetries(const char   *aPskd,
                           const char   *aProvisioningUrl,
                           uint16_t      aRetryBaseDelay,
                           uint32_t      aJoinTimeout,
                           const char   *aVendorName,
                           const char   *aVendorModel,
                           const char   *aVendorSwVersion,
                           const char   *aVendorData,
                           RetryCallback aCallback,
                           void         *aContext);

    /**
     * Starts the Joiner service.
     *
     * @param[in]  aPskd             A pointer to the PSKd.
     * @param[in]  aProvisioningUrl  A pointer to the Provisioning URL (may be `nullptr`).
     * @param[in]  aVendorName       A pointer to the Vendor Name (may be `nullptr`).
     * @param[in]  aVendorModel      A pointer to the Vendor Model (may be `nullptr`).
     * @param[in]  aVendorSwVersion  A pointer to the Vendor SW Version (may be `nullptr`).
     * @param[in]  aVendorData       A pointer to the Vendor Data (may be `nullptr`).
     * @param[in]  aCallback         A pointer to a function that is called when the join operation completes.
     * @param[in]  aContext          A pointer to application-specific context.
     *
     * @retval kErrorNone          Successfully started the Joiner service.
     * @retval kErrorBusy          The previous attempt is still on-going.
     * @retval kErrorInvalidState  The IPv6 stack is not enabled or Thread stack is fully enabled.
     */
    Error Start(const char        *aPskd,
                const char        *aProvisioningUrl,
                const char        *aVendorName,
                const char        *aVendorModel,
                const char        *aVendorSwVersion,
                const char        *aVendorData,
                CompletionCallback aCallback,
                void              *aContext);

    /**
     * Stops the Joiner service.
     */
    void Stop(void);

    /**
     * Gets the Joiner State.
     *
     * @returns The Joiner state (see `State`).
     */
    State GetState(void) const { return mState; }

    /**
     * Retrieves the Joiner ID.
     *
     * @returns The Joiner ID.
     */
    const Mac::ExtAddress &GetId(void) const { return mId; }

    /**
     * Gets the Jointer Discerner.
     *
     * @returns A pointer to the current Joiner Discerner or `nullptr` if none is set.
     */
    const JoinerDiscerner *GetDiscerner(void) const;

    /**
     * Sets the Joiner Discerner.
     *
     * The Joiner Discerner is used to calculate the Joiner ID used during commissioning/joining process.
     *
     * By default (when a discerner is not provided or cleared), Joiner ID is derived as first 64 bits of the
     * result of computing SHA-256 over factory-assigned IEEE EUI-64. Note that this is the main behavior expected by
     * Thread specification.
     *
     * @param[in]   aDiscerner  A Joiner Discerner
     *
     * @retval kErrorNone          The Joiner Discerner updated successfully.
     * @retval kErrorInvalidArgs   @p aDiscerner is not valid (specified length is not within valid range).
     * @retval kErrorInvalidState  There is an ongoing Joining process so Joiner Discerner could not be changed.
     */
    Error SetDiscerner(const JoinerDiscerner &aDiscerner);

    /**
     * Clears any previously set Joiner Discerner.
     *
     * When cleared, Joiner ID is derived as first 64 bits of SHA-256 of factory-assigned IEEE EUI-64.
     *
     * @retval kErrorNone          The Joiner Discerner cleared and Joiner ID updated.
     * @retval kErrorInvalidState  There is an ongoing Joining process so Joiner Discerner could not be changed.
     */
    Error ClearDiscerner(void);

    /**
     * Converts a given Joiner state to its human-readable string representation.
     *
     * @param[in] aState  The Joiner state to convert.
     *
     * @returns A human-readable string representation of @p aState.
     */
    static const char *StateToString(State aState);

private:
    static constexpr uint16_t kMaxJoinerRouterCandidates = OPENTHREAD_CONFIG_JOINER_MAX_CANDIDATES;
    static constexpr uint16_t kJoinerUdpPort             = OPENTHREAD_CONFIG_JOINER_UDP_PORT;
    static constexpr uint32_t kConfigExtAddressDelay     = 100;  // in msec.
    static constexpr uint32_t kResponseTimeout           = 4000; // in msec.
    static constexpr uint16_t kMinJoinerBaseTimeout      = 500;  // in msec.
    static constexpr uint16_t kMaxJoinerBaseTimeout      = 3500; // in msec.

    struct JoinerRouter
    {
        Mac::ExtAddress mExtAddr;
        Mac::PanId      mPanId;
        uint16_t        mJoinerUdpPort;
        uint8_t         mChannel;
        uint8_t         mPriority;
    };

    void        SetState(State aState);
    void        SetIdFromIeeeEui64(void);
    void        SaveDiscoveredJoinerRouter(const Mle::DiscoverScanner::ScanResult &aResult);
    void        TryNextJoinerRouter(Error aPrevError);
    Error       Connect(JoinerRouter &aRouter);
    void        Finish(Error aError);
    void        HandleTimer(void);
    uint8_t     CalculatePriority(int8_t aRssi, bool aSteeringDataAllowsAny);
    Error       PrepareJoinerFinalizeMessage(const char *aProvisioningUrl,
                                             const char *aVendorName,
                                             const char *aVendorModel,
                                             const char *aVendorSwVersion,
                                             const char *aVendorData);
    void        FreeJoinerFinalizeMessage(void);
    void        SendJoinerFinalize(void);
    void        SendJoinerEntrustResponse(const Coap::Msg &aMsg);
    static void HandleDiscoverResult(Mle::DiscoverScanner::ScanResult *aResult, void *aContext);
    void        HandleDiscoverResult(Mle::DiscoverScanner::ScanResult *aResult);
    static void HandleSecureCoapClientConnect(Dtls::Session::ConnectEvent aEvent, void *aContext);
    void        HandleSecureCoapClientConnect(Dtls::Session::ConnectEvent aEvent);
    void        StartNewJoinAttempt(void);
    static void CallbackWhenRetrying(otError aError, void *aContext);

    DeclareTmfResponseHandlerIn(Joiner, HandleJoinerFinalizeResponse);

    template <Uri kUri> void HandleTmf(Coap::Msg &aMsg);

    using JoinerTimer       = TimerMilliIn<Joiner, &Joiner::HandleTimer>;
    using RetryTimeoutTimer = TimerMilliIn<Joiner, &Joiner::StartNewJoinAttempt>;

    Mac::ExtAddress              mId;
    JoinerDiscerner              mDiscerner;
    State                        mState;
    Callback<CompletionCallback> mCompletionCallback;
    JoinerRouter                 mJoinerRouters[kMaxJoinerRouterCandidates];
    uint16_t                     mJoinerRouterIndex;
    Coap::Message               *mFinalizeMessage;
    JoinerTimer                  mTimer;
    uint16_t                     mRetryBaseDelay;
    uint32_t                     mJoinTimeout;
    uint8_t                      mRetryDelayFactor;
    uint8_t                      mRetryRandomOffset;
    RetryTimeoutTimer            mRetryTimeoutTimer;
    Callback<RetryCallback>      mRetryingCallback;
    uint32_t                     mRetryStartTimestamp;
    const char                  *mPskd;
    const char                  *mProvisioningUrl;
    const char                  *mVendorName;
    const char                  *mVendorModel;
    const char                  *mVendorSwVersion;
    const char                  *mVendorData;
};

DeclareTmfHandler(Joiner, kUriJoinerEntrust);

} // namespace MeshCoP

DefineMapEnum(otJoinerState, MeshCoP::Joiner::State);

} // namespace ot

#endif // OPENTHREAD_CONFIG_JOINER_ENABLE

#endif // OT_CORE_MESHCOP_JOINER_HPP_
