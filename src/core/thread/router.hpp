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
 *   This file includes definitions Thread `Router` and `Parent`.
 */

#ifndef ROUTER_HPP_
#define ROUTER_HPP_

#include "openthread-core-config.h"

#include "thread/neighbor.hpp"

namespace ot {

class Parent;

/**
 * Represents a Thread Router
 */
class Router : public Neighbor
{
public:
    /**
     * Represents diagnostic information for a Thread Router.
     */
    class Info : public otRouterInfo, public Clearable<Info>
    {
    public:
        /**
         * Sets the `Info` instance from a given `Router`.
         *
         * @param[in] aRouter   A router.
         */
        void SetFrom(const Router &aRouter);

        /**
         * Sets the `Info` instance from a given `Parent`.
         *
         * @param[in] aParent   A parent.
         */
        void SetFrom(const Parent &aParent);
    };

    /**
     * Initializes the `Router` object.
     *
     * @param[in] aInstance  A reference to OpenThread instance.
     */
    void Init(Instance &aInstance) { Neighbor::Init(aInstance); }

    /**
     * Clears the router entry.
     */
    void Clear(void);

    /**
     * Sets the `Router` entry from a `Parent`
     */
    void SetFrom(const Parent &aParent);

    /**
     * Gets the router ID of the next hop to this router.
     *
     * @returns The router ID of the next hop to this router.
     */
    uint8_t GetNextHop(void) const { return mNextHop; }

    /**
     * Gets the link quality out value for this router.
     *
     * @returns The link quality out value for this router.
     */
    LinkQuality GetLinkQualityOut(void) const { return static_cast<LinkQuality>(mLinkQualityOut); }

    /**
     * Sets the link quality out value for this router.
     *
     * @param[in]  aLinkQuality  The link quality out value for this router.
     */
    void SetLinkQualityOut(LinkQuality aLinkQuality) { mLinkQualityOut = aLinkQuality; }

    /**
     * Gets the two-way link quality value (minimum of link quality in and out).
     *
     * @returns The two-way link quality value.
     */
    LinkQuality GetTwoWayLinkQuality(void) const;

    /**
     * Get the route cost to this router.
     *
     * @returns The route cost to this router.
     */
    uint8_t GetCost(void) const { return mCost; }

    /**
     * Sets the next hop and cost to this router.
     *
     * @param[in]  aNextHop  The Router ID of the next hop to this router.
     * @param[in]  aCost     The cost to this router.
     *
     * @retval TRUE   If there was a change, i.e., @p aNextHop or @p aCost were different from their previous values.
     * @retval FALSE  If no change to next hop and cost values (new values are the same as before).
     */
    bool SetNextHopAndCost(uint8_t aNextHop, uint8_t aCost);

    /**
     * Sets the next hop to this router as invalid and clears the cost.
     *
     * @retval TRUE   If there was a change (next hop was valid before).
     * @retval FALSE  No change to next hop (next hop was invalid before).
     */
    bool SetNextHopToInvalid(void);

private:
    uint8_t mNextHop;            ///< The next hop towards this router
    uint8_t mLinkQualityOut : 2; ///< The link quality out for this router

#if OPENTHREAD_CONFIG_MLE_LONG_ROUTES_ENABLE
    uint8_t mCost; ///< The cost to this router via neighbor router
#else
    uint8_t mCost : 4; ///< The cost to this router via neighbor router
#endif
};

/**
 * Represent parent of a child node.
 */
class Parent : public Router
{
public:
    /**
     * Initializes the `Parent`.
     *
     * @param[in] aInstance  A reference to OpenThread instance.
     */
    void Init(Instance &aInstance)
    {
        Neighbor::Init(aInstance);
#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
        mCslAccuracy.Init();
#endif
    }

    /**
     * Clears the parent entry.
     */
    void Clear(void);

    /**
     * Gets route cost from parent to leader.
     *
     * @returns The route cost from parent to leader
     */
    uint8_t GetLeaderCost(void) const { return mLeaderCost; }

    /**
     * Sets route cost from parent to leader.
     *
     * @param[in] aLeaderCost  The route cost.
     */
    void SetLeaderCost(uint8_t aLeaderCost) { mLeaderCost = aLeaderCost; }

#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
    /**
     * Gets the CSL accuracy (clock accuracy and uncertainty).
     *
     * @returns The CSL accuracy.
     */
    const Mac::CslAccuracy &GetCslAccuracy(void) const { return mCslAccuracy; }

    /**
     * Sets CSL accuracy.
     *
     * @param[in] aCslAccuracy  The CSL accuracy.
     */
    void SetCslAccuracy(const Mac::CslAccuracy &aCslAccuracy) { mCslAccuracy = aCslAccuracy; }
#endif

private:
    uint8_t mLeaderCost;
#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
    Mac::CslAccuracy mCslAccuracy; // CSL accuracy (clock accuracy in ppm and uncertainty).
#endif
};

DefineCoreType(otRouterInfo, Router::Info);

} // namespace ot

#endif // ROUTER_HPP_
