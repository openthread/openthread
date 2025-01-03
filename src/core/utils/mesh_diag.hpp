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

#if !OPENTHREAD_CONFIG_TMF_NETDIAG_CLIENT_ENABLE
#error "OPENTHREAD_CONFIG_MESH_DIAG_ENABLE requires OPENTHREAD_CONFIG_TMF_NETDIAG_CLIENT_ENABLE"
#endif

#include <openthread/mesh_diag.h>

#include "coap/coap.hpp"
#include "common/callback.hpp"
#include "common/locator.hpp"
#include "common/message.hpp"
#include "common/timer.hpp"
#include "net/ip6_address.hpp"
#include "thread/network_diagnostic.hpp"
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
 * Implements the Mesh Diagnostics.
 */
class MeshDiag : public InstanceLocator
{
    friend class ot::NetworkDiagnostic::Client;

public:
    static constexpr uint16_t kVersionUnknown = OT_MESH_DIAG_VERSION_UNKNOWN; ///< Unknown version.

    typedef otMeshDiagDiscoverConfig                   DiscoverConfig;              ///< Discovery configuration.
    typedef otMeshDiagDiscoverCallback                 DiscoverCallback;            ///< Discovery callback.
    typedef otMeshDiagQueryChildTableCallback          QueryChildTableCallback;     ///< Query Child Table callback.
    typedef otMeshDiagChildIp6AddrsCallback            ChildIp6AddrsCallback;       ///< Child IPv6 addresses callback.
    typedef otMeshDiagQueryRouterNeighborTableCallback RouterNeighborTableCallback; ///< Neighbor table callback.

    /**
     * Represents an iterator to go over list of IPv6 addresses of a router or an MTD child.
     */
    class Ip6AddrIterator : public otMeshDiagIp6AddrIterator
    {
        friend class MeshDiag;

    public:
        /**
         * Iterates through the discovered IPv6 address of a router.
         *
         * @param[out]     aAddress  A reference to return the next IPv6 address (if any).
         *
         * @retval kErrorNone      Successfully retrieved the next address. @p aAddress is updated.
         * @retval kErrorNotFound  No more address. Reached the end of the list.
         */
        Error GetNextAddress(Ip6::Address &aAddress);

    private:
        Error InitFrom(const Message &aMessage);

        const Message *mMessage;
        OffsetRange    mOffsetRange;
    };

    /**
     * Represents information about a router in Thread mesh.
     */
    class RouterInfo : public otMeshDiagRouterInfo, public Clearable<RouterInfo>
    {
        friend class MeshDiag;

    private:
        Error ParseFrom(const Message &aMessage);
    };

    /**
     * Represents information about a child in Thread mesh.
     */
    class ChildInfo : public otMeshDiagChildInfo, public Clearable<ChildInfo>
    {
    };

    /**
     * Represents an iterator to go over list of IPv6 addresses of a router.
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
         */
        Error GetNextChildInfo(ChildInfo &aChildInfo);

    private:
        Error InitFrom(const Message &aMessage, uint16_t aParentRloc16);

        const Message *mMessage;
        OffsetRange    mOffsetRange;
        uint16_t       mParentRloc16;
    };

    /**
     * Initializes the `MeshDiag` instance.
     *
     * @param[in] aInstance   The OpenThread instance.
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
     * @retval kErrorBusy          A previous discovery or query request is still ongoing.
     * @retval kErrorInvalidState  Device is not attached.
     * @retval kErrorNoBufs        Could not allocate buffer to send discovery messages.
     */
    Error DiscoverTopology(const DiscoverConfig &aConfig, DiscoverCallback aCallback, void *aContext);

    /**
     * Starts query for child table for a given router.
     *
     * @param[in] aRloc16          The RLOC16 of router to query.
     * @param[in] aCallback        The callback to report the queried child table.
     * @param[in] aContext         A context to pass in @p aCallback.
     *
     * @retval kErrorNone          The query started successfully.
     * @retval kErrorBusy          A previous discovery or query request is still ongoing.
     * @retval kErrorInvalidArgs   The @p aRloc16 is not a valid router RLOC16.
     * @retval kErrorInvalidState  Device is not attached.
     * @retval kErrorNoBufs        Could not allocate buffer to send query messages.
     */
    Error QueryChildTable(uint16_t aRloc16, QueryChildTableCallback aCallback, void *aContext);

    /**
     * Sends a query to a parent to retrieve the IPv6 addresses of all its MTD children.
     *
     * @param[in] aRloc16          The RLOC16 of parent to query.
     * @param[in] aCallback        The callback to report the queried child IPv6 address list.
     * @param[in] aContext         A context to pass in @p aCallback.
     *
     * @retval kErrorNone          The query started successfully.
     * @retval kErrorBusy          A previous discovery or query request is still ongoing.
     * @retval kErrorInvalidArgs   The @p aRloc16 is not a valid  RLOC16.
     * @retval kErrorInvalidState  Device is not attached.
     * @retval kErrorNoBufs        Could not allocate buffer to send query messages.
     */
    Error QueryChildrenIp6Addrs(uint16_t aRloc16, ChildIp6AddrsCallback aCallback, void *aContext);

    /**
     * Starts query for router neighbor table for a given router.
     *
     * @param[in] aRloc16          The RLOC16 of router to query.
     * @param[in] aCallback        The callback to report the queried table.
     * @param[in] aContext         A context to pass in @p aCallback.
     *
     * @retval kErrorNone          The query started successfully.
     * @retval kErrorBusy          A previous discovery or query request is still ongoing.
     * @retval kErrorInvalidArgs   The @p aRloc16 is not a valid router RLOC16.
     * @retval kErrorInvalidState  Device is not attached.
     * @retval kErrorNoBufs        Could not allocate buffer to send query messages.
     */
    Error QueryRouterNeighborTable(uint16_t aRloc16, RouterNeighborTableCallback aCallback, void *aContext);

    /**
     * Cancels an ongoing discovery or query operation if there one, otherwise no action.
     *
     * When ongoing discovery is cancelled, the callback from `DiscoverTopology()` or  `QueryChildTable()` will not be
     * called anymore.
     */
    void Cancel(void);

private:
    typedef ot::NetworkDiagnostic::Tlv Tlv;

    static constexpr uint32_t kResponseTimeout = OPENTHREAD_CONFIG_MESH_DIAG_RESPONSE_TIMEOUT;

    enum State : uint8_t
    {
        kStateIdle,
        kStateDiscoverTopology,
        kStateQueryChildTable,
        kStateQueryChildrenIp6Addrs,
        kStateQueryRouterNeighborTable,
    };

    struct DiscoverInfo
    {
        Callback<DiscoverCallback> mCallback;
        Mle::RouterIdSet           mExpectedRouterIdSet;
    };

    struct QueryChildTableInfo
    {
        Callback<QueryChildTableCallback> mCallback;
        uint16_t                          mRouterRloc16;
    };

    struct QueryChildrenIp6AddrsInfo
    {
        Callback<ChildIp6AddrsCallback> mCallback;
        uint16_t                        mParentRloc16;
    };

    struct QueryRouterNeighborTableInfo
    {
        Callback<RouterNeighborTableCallback> mCallback;
        uint16_t                              mRouterRloc16;
    };

    class ChildEntry : public otMeshDiagChildEntry
    {
        friend class MeshDiag;

    private:
        void SetFrom(const NetworkDiagnostic::ChildTlv &aChildTlv);
    };

    class RouterNeighborEntry : public otMeshDiagRouterNeighborEntry
    {
        friend class MeshDiag;

    private:
        void SetFrom(const NetworkDiagnostic::RouterNeighborTlv &aTlv);
    };

    Error SendQuery(uint16_t aRloc16, const uint8_t *aTlvs, uint8_t aTlvsLength);
    void  Finalize(Error aError);
    void  HandleTimer(void);
    bool  HandleDiagnosticGetAnswer(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo);
    Error ProcessMessage(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo, uint16_t aSenderRloc16);
    bool  ProcessChildTableAnswer(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo);
    bool  ProcessChildrenIp6AddrsAnswer(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo);
    bool  ProcessRouterNeighborTableAnswer(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo);

    void HandleDiagGetResponse(Coap::Message *aMessage, const Ip6::MessageInfo *aMessageInfo, Error aResult);

    static void HandleDiagGetResponse(void                *aContext,
                                      otMessage           *aMessage,
                                      const otMessageInfo *aMessageInfo,
                                      Error                aResult);

    using TimeoutTimer = TimerMilliIn<MeshDiag, &MeshDiag::HandleTimer>;

    State        mState;
    uint16_t     mExpectedQueryId;
    uint16_t     mExpectedAnswerIndex;
    TimeoutTimer mTimer;

    union
    {
        DiscoverInfo                 mDiscover;
        QueryChildTableInfo          mQueryChildTable;
        QueryChildrenIp6AddrsInfo    mQueryChildrenIp6Addrs;
        QueryRouterNeighborTableInfo mQueryRouterNeighborTable;
    };
};

} // namespace Utils

DefineCoreType(otMeshDiagIp6AddrIterator, Utils::MeshDiag::Ip6AddrIterator);
DefineCoreType(otMeshDiagRouterInfo, Utils::MeshDiag::RouterInfo);
DefineCoreType(otMeshDiagChildInfo, Utils::MeshDiag::ChildInfo);
DefineCoreType(otMeshDiagChildIterator, Utils::MeshDiag::ChildIterator);

} // namespace ot

#endif // OPENTHREAD_CONFIG_MESH_DIAG_ENABLE && OPENTHREAD_FTD

#endif // MESH_DIAG_HPP_
