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
#include "common/crc16.hpp"
#include "common/locator.hpp"
#include "common/message.hpp"
#include "meshcop/dtls.hpp"
#include "meshcop/meshcop_tlvs.hpp"
#include "net/udp6.hpp"

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
     * @param[in]  aVendorName       A pointer to the Vendor Name (must be static).
     * @param[in]  aVendorModel      A pointer to the Vendor Model (must be static).
     * @param[in]  aVendorSwVersion  A pointer to the Vendor SW Version (must be static).
     * @param[in]  aVendorData       A pointer to the Vendor Data (must be static).
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
     * @retval OT_ERROR_NONE  Successfully stopped the Joiner service.
     *
     */
    otError Stop(void);

    /**
     * This function returns the Joiner State.
     *
     * @retval OT_JOINER_STATE_IDLE
     * @retval OT_JOINER_STATE_DISCOVER
     * @retval OT_JOINER_STATE_CONNECT
     * @retval OT_JOINER_STATE_CONNECTED
     * @retval OT_JOINER_STATE_ENTRUST
     * @retval OT_JOINER_STATE_JOINED
     *
     */
    otJoinerState GetState(void) const;

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
        kConfigExtAddressDelay = 100,  ///< milliseconds
        kTimeout               = 4000, ///< milliseconds
        kSpecificPriorityBonus = (1 << 9),
    };

    struct JoinerRouter
    {
        Mac::ExtAddress mExtAddr;
        uint16_t        mPriority;
        uint16_t        mPanId;
        uint16_t        mJoinerUdpPort;
        uint8_t         mChannel;
    };

    static void HandleDiscoverResult(otActiveScanResult *aResult, void *aContext);
    void        HandleDiscoverResult(otActiveScanResult *aResult);

    static void HandleTimer(Timer &aTimer);
    void        HandleTimer(void);

    void Close(void);
    void Complete(otError aError);

    void    AddJoinerRouter(JoinerRouter &aJoinerRouter);
    otError TryNextJoin();

    static void HandleSecureCoapClientConnect(bool aConnected, void *aContext);
    void        HandleSecureCoapClientConnect(bool aConnected);

    void        SendJoinerFinalize(void);
    static void HandleJoinerFinalizeResponse(void *               aContext,
                                             otMessage *          aMessage,
                                             const otMessageInfo *aMessageInfo,
                                             otError              aResult);
    void HandleJoinerFinalizeResponse(Coap::Message &aMessage, const Ip6::MessageInfo *aMessageInfo, otError aResult);

    static void HandleJoinerEntrust(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo);
    void        HandleJoinerEntrust(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo);
    void        SendJoinerEntrustResponse(const Coap::Message &aRequest, const Ip6::MessageInfo &aRequestInfo);

    otJoinerState mState;

    otJoinerCallback mCallback;
    void *           mContext;

    uint16_t mCcitt;
    uint16_t mAnsi;

    JoinerRouter mJoinerRouters[OPENTHREAD_CONFIG_MAX_JOINER_ROUTER_ENTRIES];

    const char *mVendorName;
    const char *mVendorModel;
    const char *mVendorSwVersion;
    const char *mVendorData;

    TimerMilli     mTimer;
    Coap::Resource mJoinerEntrust;
};

} // namespace MeshCoP
} // namespace ot

#endif // JOINER_HPP_
