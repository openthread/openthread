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

#ifndef JOINER_HPP_
#define JOINER_HPP_

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
#include "meshcop/ccm/ae_client.hpp"
#include "meshcop/meshcop.hpp"
#include "meshcop/meshcop_tlvs.hpp"
#include "meshcop/secure_transport.hpp"
#include "thread/discover_scanner.hpp"
#include "thread/tmf.hpp"

// the configuration now denotes a base/start for a range of port numbers. The joiner protocol identifier is
// multiplexed into the 4 lower bits of the Joiner (source) port number.
static_assert(OPENTHREAD_CONFIG_JOINER_UDP_PORT % 16 == 0, "OPENTHREAD_CONFIG_JOINER_UDP_PORT modulo 16 must be 0 due to CCM commissioning extension");

namespace ot {

namespace MeshCoP {

class Joiner : public InstanceLocator, private NonCopyable
{
    friend class Tmf::Agent;

public:
    /**
     * Type defines the Joiner State.
     */
    enum State : uint8_t
    {
        kStateIdle      = OT_JOINER_STATE_IDLE,
        kStateDiscover  = OT_JOINER_STATE_DISCOVER,
        kStateConnect   = OT_JOINER_STATE_CONNECT,
        kStateConnected = OT_JOINER_STATE_CONNECTED,
        kStateEntrust   = OT_JOINER_STATE_ENTRUST,
        kStateJoined    = OT_JOINER_STATE_JOINED,
    };

    /**
     * Defines Joiner types. Each type has a bit pattern that is used in the UDP source port
     * to identify the Joiner type.
     */
     enum Type : uint8_t
    {
        kTypeCcmAe   = 1,
        kTypeCcmNkp  = 2,
        kTypeMeshcop = 8,
    };

    /**
     * Initializes the Joiner object.
     *
     * @param[in]  aInstance     A reference to the OpenThread instance.
     */
    explicit Joiner(Instance &aInstance);

    /**
     * Starts the Joiner service for MeshCoP commissioning.
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
    Error Start(const char      *aPskd,
                const char      *aProvisioningUrl,
                const char      *aVendorName,
                const char      *aVendorModel,
                const char      *aVendorSwVersion,
                const char      *aVendorData,
                otJoinerCallback aCallback,
                void            *aContext);

    /**
     * Starts the Joiner service in CCM mode to perform Autonomous Enrollment (AE), using the IETF cBRSKI
     * protocol.
     *
     * @param[in]  aCallback       A pointer to a function that is called when the join operation completes.
     * @param[in]  aContext        A pointer to application-specific context.
     *
     * @retval kErrorNone          Successfully started the CCM Joiner service.
     * @retval kErrorBusy          The previous attempt is still on-going.
     * @retval kErrorInvalidState  The IPv6 stack is not enabled or Thread stack is not fully enabled.
     *
     */
    Error StartCcmAe(otJoinerCallback aCallback, void *aContext);

    /**
     * Starts the Joiner service in CCM mode to perform Network Key Provisioning (NKP) to join a new
     * CCM network and obtain the key for it. This requires a successfully completed CCM AE procedure.
     *
     * @param[in]  aCallback       A pointer to a function that is called when the join operation completes.
     * @param[in]  aContext        A pointer to application-specific context.
     *
     * @retval kErrorNone          Successfully started the CCM Joiner service.
     * @retval kErrorBusy          The previous attempt is still on-going.
     * @retval kErrorInvalidState  The IPv6 stack is not enabled, or Thread stack is not fully enabled, or
     *                             CCM AE was not completed prior to this call.
     *
     */
    Error StartCcmNkp(otJoinerCallback aCallback, void *aContext);

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
     * FIXME comments insert here from coap_secure.hpp
     */
    void SetCcmIdentity(const uint8_t *aX509Cert,
                        uint32_t       aX509Length,
                        const uint8_t *aPrivateKey,
                        uint32_t       aPrivateKeyLength,
                        const uint8_t *aX509CaCertificateChain,
                        uint32_t aX509CaCertChainLength);

    /**
     * Converts a given Joiner state to its human-readable string representation.
     *
     * @param[in] aState  The Joiner state to convert.
     *
     * @returns A human-readable string representation of @p aState.
     */
    static const char *StateToString(State aState);

private:
    static constexpr uint16_t kJoinerUdpPort = OPENTHREAD_CONFIG_JOINER_UDP_PORT;
    static constexpr uint16_t kMeshcopJoinerUdpSourcePort = OPENTHREAD_CONFIG_JOINER_UDP_PORT + kTypeMeshcop;
    static constexpr uint16_t kCcmAeJoinerUdpSourcePort = OPENTHREAD_CONFIG_JOINER_UDP_PORT + kTypeCcmAe;
    static constexpr uint16_t kCcmNkpJoinerUdpSourcePort = OPENTHREAD_CONFIG_JOINER_UDP_PORT + kTypeCcmNkp;

    static constexpr uint32_t kConfigExtAddressDelay = 100;  // in msec.
    static constexpr uint32_t kResponseTimeout       = 4000; ///< Max wait time to receive response (in msec).
    static constexpr uint32_t kCcmAeResponseTimeout  = 12000; ///< Max wait time for CCM AE to receive response (in msec), TODO.

    struct JoinerRouter
    {
        Mac::ExtAddress mExtAddr;
        Mac::PanId      mPanId;
        uint16_t        mJoinerUdpPort;
        uint8_t         mChannel;
        uint8_t         mPriority;
    };

    static void HandleDiscoverResult(Mle::DiscoverScanner::ScanResult *aResult, void *aContext);
    void        HandleDiscoverResult(Mle::DiscoverScanner::ScanResult *aResult);

    static void HandleSecureCoapClientConnect(SecureTransport::ConnectEvent aEvent, void *aContext);
    void        HandleSecureCoapClientConnect(SecureTransport::ConnectEvent aEvent);

    static void HandleJoinerFinalizeResponse(void                *aContext,
                                             otMessage           *aMessage,
                                             const otMessageInfo *aMessageInfo,
                                             Error                aResult);
    void HandleJoinerFinalizeResponse(Coap::Message *aMessage, const Ip6::MessageInfo *aMessageInfo, Error aResult);

    template <Uri kUri> void HandleTmf(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo);

    void HandleTimer(void);

    void    SetState(State aState);
    void    SetIdFromIeeeEui64(void);
    void    SaveDiscoveredJoinerRouter(const Mle::DiscoverScanner::ScanResult &aResult);
    void    TryNextJoinerRouter(Error aPrevError);
    Error   Connect(JoinerRouter &aRouter);
    void    Finish(Error aError);
    uint8_t CalculatePriority(int8_t aRssi, bool aSteeringDataAllowsAny);

    Error PrepareJoinerFinalizeMessage(const char *aProvisioningUrl,
                                       const char *aVendorName,
                                       const char *aVendorModel,
                                       const char *aVendorSwVersion,
                                       const char *aVendorData);
    void  FreeJoinerFinalizeMessage(void);
    void  SendJoinerFinalize(void);
    void  SendJoinerEntrustResponse(const Coap::Message &aRequest, const Ip6::MessageInfo &aRequestInfo);

#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
    void LogCertMessage(const char *aText, const Coap::Message &aMessage) const;
#endif

    using JoinerTimer = TimerMilliIn<Joiner, &Joiner::HandleTimer>;

    Type            mJoinerType;
    AeClient       *mAeClient;
    Mac::ExtAddress mId;
    JoinerDiscerner mDiscerner;

    State mState;
    uint16_t mJoinerSourcePort;

    Callback<otJoinerCallback> mCallback;

    JoinerRouter mJoinerRouters[OPENTHREAD_CONFIG_JOINER_MAX_CANDIDATES];
    uint16_t     mJoinerRouterIndex;

    Coap::Message *mFinalizeMessage;

    JoinerTimer mTimer;
};

DeclareTmfHandler(Joiner, kUriJoinerEntrust);

} // namespace MeshCoP

DefineMapEnum(otJoinerState, MeshCoP::Joiner::State);

} // namespace ot

#endif // OPENTHREAD_CONFIG_JOINER_ENABLE

#endif // JOINER_HPP_
