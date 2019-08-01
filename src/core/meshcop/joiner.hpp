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

#include <openthread/joiner.h>

#include "coap/coap.hpp"
#include "coap/coap_message.hpp"
#include "coap/coap_secure.hpp"
#include "common/locator.hpp"
#include "common/logging.hpp"
#include "common/message.hpp"
#include "mac/mac_frame.hpp"
#include "meshcop/dtls.hpp"
#include "meshcop/meshcop_tlvs.hpp"

namespace ot {

namespace MeshCoP {

class Joiner : public InstanceLocator
{
public:
    /**
     * This constructor initializes the Joiner object.
     *
     * @param[in]  aInstance     A reference to the OpenThread instance.
     *
     */
    explicit Joiner(Instance &aInstance);

    /**
     * This method starts the Joiner service.
     *
     * @param[in]  aPSKd             A pointer to the PSKd.
     * @param[in]  aProvisioningUrl  A pointer to the Provisioning URL (may be NULL).
     * @param[in]  aVendorName       A pointer to the Vendor Name (may be NULL).
     * @param[in]  aVendorModel      A pointer to the Vendor Model (may be NULL).
     * @param[in]  aVendorSwVersion  A pointer to the Vendor SW Version (may be NULL).
     * @param[in]  aVendorData       A pointer to the Vendor Data (may be NULL).
     * @param[in]  aCallback         A pointer to a function that is called when the join operation completes.
     * @param[in]  aContext          A pointer to application-specific context.
     *
     * @retval OT_ERROR_NONE  Successfully started the Joiner service.
     *
     */
    otError Start(const char *     aPSKd,
                  const char *     aProvisioningUrl,
                  const char *     aVendorName,
                  const char *     aVendorModel,
                  const char *     aVendorSwVersion,
                  const char *     aVendorData,
                  otJoinerCallback aCallback,
                  void *           aContext);

    /**
     * This method stops the Joiner service.
     *
     */
    void Stop(void);

    /**
     * This function returns the Joiner State.
     *
     * @returns The Joiner state (see `otJoinerState`).
     *
     */
    otJoinerState GetState(void) const { return mState; }

    /**
     * This method retrieves the Joiner ID.
     *
     * @param[out]  aJoinerId  The Joiner ID.
     *
     */
    void GetJoinerId(Mac::ExtAddress &aJoinerId) const;

private:
    enum
    {
        kJoinerUdpPort         = OPENTHREAD_CONFIG_JOINER_UDP_PORT,
        kConfigExtAddressDelay = 100,  ///< [milliseconds]
        kReponseTimeout        = 4000, ///< Maximum wait time to receive response [milliseconds].
    };

    struct JoinerRouter
    {
        Mac::ExtAddress mExtAddr;
        Mac::PanId      mPanId;
        uint16_t        mJoinerUdpPort;
        uint8_t         mChannel;
        uint8_t         mPriority;
    };

    static void HandleDiscoverResult(otActiveScanResult *aResult, void *aContext);
    void        HandleDiscoverResult(otActiveScanResult *aResult);

    static void HandleSecureCoapClientConnect(bool aConnected, void *aContext);
    void        HandleSecureCoapClientConnect(bool aConnected);

    static void HandleJoinerFinalizeResponse(void *               aContext,
                                             otMessage *          aMessage,
                                             const otMessageInfo *aMessageInfo,
                                             otError              aResult);
    void HandleJoinerFinalizeResponse(Coap::Message &aMessage, const Ip6::MessageInfo *aMessageInfo, otError aResult);

    static void HandleJoinerEntrust(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo);
    void        HandleJoinerEntrust(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo);

    static void HandleTimer(Timer &aTimer);
    void        HandleTimer(void);

    static const char *JoinerStateToString(otJoinerState aState);

    void    SetState(otJoinerState aState);
    void    SaveDiscoveredJoinerRouter(const otActiveScanResult &aResult);
    void    TryNextJoinerRouter(otError aPrevError);
    otError Connect(JoinerRouter &aRouter);
    void    Finish(otError aError);
    uint8_t CalculatePriority(int8_t aRssi, bool aSteeringDataAllowsAny);

    otError PrepareJoinerFinalizeMessage(const char *aProvisioningUrl,
                                         const char *aVendorName,
                                         const char *aVendorModel,
                                         const char *aVendorSwVersion,
                                         const char *aVendorData);
    void    FreeJoinerFinalizeMessage(void);
    void    SendJoinerFinalize(void);
    void    SendJoinerEntrustResponse(const Coap::Message &aRequest, const Ip6::MessageInfo &aRequestInfo);

#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
    void LogCertMessage(const char *aText, const Coap::Message &aMessage) const;
#endif

    otJoinerState mState;

    otJoinerCallback mCallback;
    void *           mContext;

    JoinerRouter mJoinerRouters[OPENTHREAD_CONFIG_JOINER_MAX_CANDIDATES];
    uint16_t     mJoinerRouterIndex;

    Coap::Message *mFinalizeMessage;

    TimerMilli     mTimer;
    Coap::Resource mJoinerEntrust;
};

} // namespace MeshCoP
} // namespace ot

#endif // JOINER_HPP_
