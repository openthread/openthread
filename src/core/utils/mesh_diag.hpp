/*
 *  Copyright (c) 2023, The OpenThread Authors.
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
 *   This file includes definitions for Mesh Diagnostic module.
 */

#ifndef MESH_DIAG_HPP_
#define MESH_DIAG_HPP_

#include "openthread-core-config.h"

#if OPENTHREAD_CONFIG_MESH_DIAG_ENABLE && OPENTHREAD_FTD

#include <openthread/mesh_diag.h>

#include "coap/coap.hpp"
#include "common/callback.hpp"
#include "common/locator.hpp"
#include "common/message.hpp"
#include "common/timer.hpp"
#include "net/ip6_address.hpp"
#include "thread/network_diagnostic_tlvs.hpp"

struct otMeshDiagIp6AddrIterator
{
};

struct otMeshDiagChildIterator
{
};

namespace ot {
namespace Utils {

/**
 * This class implements the Mesh Diagnostics.
 *
 */
class MeshDiag : public InstanceLocator
{
public:
    static constexpr uint16_t kVersionUnknown = OT_MESH_DIAG_VERSION_UNKNOWN; ///< Unknown version.

    typedef otMeshDiagDiscoverConfig   DiscoverConfig;   ///< The discovery configuration.
    typedef otMeshDiagDiscoverCallback DiscoverCallback; ///< The discovery callback function pointer type.

    /**
     * This type represents an iterator to go over list of IPv6 addresses of a router.
     *
     */
    class Ip6AddrIterator : public otMeshDiagIp6AddrIterator
    {
        friend class MeshDiag;

    public:
        /**
         * Iterates through the discovered IPv6 address of a router.
         *
         * @param[out]     aIp6Address  A reference to return the next IPv6 address (if any).
         *
         * @retval kErrorNone      Successfully retrieved the next address. @p aIp6Address is updated.
         * @retval kErrorNotFound  No more address. Reached the end of the list.
         *
         */
        Error GetNextAddress(Ip6::Address &aAddress);

    private:
        Error InitFrom(const Message &aMessage);

        const Message *mMessage;
        uint16_t       mCurOffset;
        uint16_t       mEndOffset;
    };

    /**
     * This type represents information about a router in Thread mesh.
     *
     */
    class RouterInfo : public otMeshDiagRouterInfo, public Clearable<RouterInfo>
    {
        friend class MeshDiag;

    private:
        Error ParseFrom(const Message &aMessage);
    };

    /**
     * This type represents information about a child in Thread mesh.
     *
     */
    class ChildInfo : public otMeshDiagChildInfo, public Clearable<ChildInfo>
    {
    };

    /**
     * This type represents an iterator to go over list of IPv6 addresses of a router.
     *
     */
    class ChildIterator : public otMeshDiagChildIterator
    {
        friend class MeshDiag;

    public:
        /**
         * Iterates through the discovered children of a router.
         *
         * @param[out]     aChildInfo  A reference to return the info for the next child (if any).
         *
         * @retval kErrorNone      Successfully retrieved the next child info. @p aChildInfo is updated.
         * @retval kErrorNotFound  No more child entry. Reached the end of the list.
         *
         */
        Error GetNextChildInfo(ChildInfo &aChildInfo);

    private:
        Error InitFrom(const Message &aMessage, uint16_t aParentRloc16);

        const Message *mMessage;
        uint16_t       mCurOffset;
        uint16_t       mEndOffset;
        uint16_t       mParentRloc16;
    };

    /**
     * This constructor initializes the `MeshDiag` instance.
     *
     * @param[in] aInstance   The OpenThread instance.
     *
     */
    explicit MeshDiag(Instance &aInstance);

    /**
     * Starts network topology discovery.
     *
     * @param[in] aConfig          The configuration to use for discovery (e.g., which items to discover).
     * @param[in] aCallback        The callback to report the discovered routers.
     * @param[in] aContext         A context to pass in @p aCallback.
     *
     * @retval kErrorNone          The network topology discovery started successfully.
     * @retval kErrorBusy          A previous discovery request is still ongoing.
     * @retval kErrorInvalidState  Device is not attached.
     * @retval kErrorNoBufs        Could not allocate buffer to send discovery messages.
     *
     */
    Error DiscoverTopology(const DiscoverConfig &aConfig, DiscoverCallback aCallback, void *aContext);

    /**
     * Cancels an ongoing topology discovery if there one, otherwise no action.
     *
     * When ongoing discovery is cancelled, the callback from `DiscoverTopology()` will not be called anymore.
     *
     */
    void Cancel(void);

private:
    typedef ot::NetworkDiagnostic::Tlv Tlv;

    static constexpr uint32_t kResponseTimeout = OPENTHREAD_CONFIG_MESH_DIAG_RESPONSE_TIMEOUT;

    Error SendDiagGetTo(uint16_t aRloc16, const DiscoverConfig &aConfig);
    void  HandleTimer(void);
    void  HandleDiagGetResponse(Coap::Message *aMessage, const Ip6::MessageInfo *aMessageInfo, Error aResult);

    static void HandleDiagGetResponse(void                *aContext,
                                      otMessage           *aMessage,
                                      const otMessageInfo *aMessageInfo,
                                      Error                aResult);

    using TimeoutTimer = TimerMilliIn<MeshDiag, &MeshDiag::HandleTimer>;

    Callback<DiscoverCallback> mDiscoverCallback;
    Mle::RouterIdSet           mExpectedRouterIdSet;
    TimeoutTimer               mTimer;
};

} // namespace Utils

DefineCoreType(otMeshDiagIp6AddrIterator, Utils::MeshDiag::Ip6AddrIterator);
DefineCoreType(otMeshDiagRouterInfo, Utils::MeshDiag::RouterInfo);
DefineCoreType(otMeshDiagChildInfo, Utils::MeshDiag::ChildInfo);
DefineCoreType(otMeshDiagChildIterator, Utils::MeshDiag::ChildIterator);

} // namespace ot

#endif // OPENTHREAD_CONFIG_MESH_DIAG_ENABLE && OPENTHREAD_FTD

#endif // MESH_DIAG_HPP_
