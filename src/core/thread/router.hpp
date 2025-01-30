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
     * Restarts the Link Accept timeout (setting it to max value).
     *
     * This method is used after sending a Link Request to the router to restart the timeout and start waiting to
     * receive a Link Accept response.
     */
    void RestartLinkAcceptTimeout(void) { mLinkAcceptTimeout = Mle::kLinkAcceptTimeout; }

    /**
     * Clears the Link Accept timeout value (setting it to zero).
     *
     * This method is used when we successfully receive and process a Link Accept.
     */
    void ClearLinkAcceptTimeout(void) { mLinkAcceptTimeout = 0; }

    /**
     * Indicates whether or not we are waiting to receive a Link Accept from this router (timeout is non-zero).
     *
     * @retval TRUE   Waiting to receive a Link Accept response.
     * @retval FALSE  Not waiting to receive a Link Accept response.
     */
    bool IsWaitingForLinkAccept(void) const { return (mLinkAcceptTimeout > 0); }

    /**
     * Decrements the Link Accept timeout value (in seconds).
     *
     * Caller MUST ensure the current value is non-zero by checking `IsWaitingForLinkAccept()`.
     *
     * @returns The decremented timeout value.
     */
    uint8_t DecrementLinkAcceptTimeout(void) { return --mLinkAcceptTimeout; }

    /**
     * Sets the counter tracking the number of Link Request attempts during link re-establishment to its maximum value
     * `Mle::kLinkRequestAttempts`.
     */
    void SetLinkRequestAttemptsToMax(void) { mLinkRequestAttempts = Mle::kLinkRequestAttempts; }

    /**
     * Indicates whether there are remaining Link Request attempts (during link re-establishment).
     *
     * @retval TRUE   There are remaining Link Request attempts.
     * @retval FALSE  There are no more Link Request attempts (the counter is zero).
     */
    bool HasRemainingLinkRequestAttempts(void) const { return mLinkRequestAttempts > 0; }

    /**
     * Decrements the counter tracking the number of remaining Link Request attempts during link re-establishment.
     *
     * Caller MUST ensure the current counter is non-zero by checking `HasRemainingLinkRequestAttempts()`.
     */
    void DecrementLinkRequestAttempts(void) { mLinkRequestAttempts--; }

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
    LinkQuality GetLinkQualityOut(void) const { return GetLinkInfo().GetLinkQualityOut(); }

    /**
     * Sets the link quality out value for this router.
     *
     * @param[in]  aLinkQuality  The link quality out value for this router.
     */
    void SetLinkQualityOut(LinkQuality aLinkQuality) { GetLinkInfo().SetLinkQualityOut(aLinkQuality); }

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

#if OPENTHREAD_CONFIG_PARENT_SEARCH_ENABLE
    /**
     * Indicates whether or not this router can be selected as parent.
     *
     * @retval TRUE  The router is selectable as parent.
     * @retval FALSE The router is not selectable as parent.
     */
    bool IsSelectableAsParent(void) const { return mIsSelectableAsParent; }

    /**
     * Sets whether or not this router is selectable as parent.
     *
     * @param[in] aIsSelectable   Boolean indicating whether or not router is selectable as parent.
     */
    void SetSelectableAsParent(bool aIsSelectable) { mIsSelectableAsParent = aIsSelectable; }

    /**
     * Restarts timeout to block reselecting this router as parent (setting it to `kParentReselectTimeout`).
     */
    void RestartParentReselectTimeout(void) { mParentReselectTimeout = Mle::kParentReselectTimeout; }

    /**
     * Gets the remaining timeout duration in seconds to block reselecting this router parent.
     *
     * @returns The remaining timeout duration in seconds.
     */
    uint16_t GetParentReselectTimeout(void) const { return mParentReselectTimeout; }

    /**
     * Decrements the reselect timeout duration (if non-zero).
     */
    void DecrementParentReselectTimeout(void) { (mParentReselectTimeout > 0) ? mParentReselectTimeout-- : 0; }
#endif

private:
    static_assert(Mle::kLinkAcceptTimeout < 4, "kLinkAcceptTimeout won't fit in mLinkAcceptTimeout (2-bit field)");
    static_assert(Mle::kLinkRequestAttempts < 4, "kLinkRequestAttempts won't fit in mLinkRequestAttempts (2-bit field");

#if OPENTHREAD_CONFIG_PARENT_SEARCH_ENABLE
    static_assert(Mle::kParentReselectTimeout <= (1U << 15) - 1,
                  "kParentReselectTimeout won't fit in mParentReselectTimeout (15-bit filed)");
#endif

    uint8_t mNextHop;                 // The next hop towards this router
    uint8_t mLinkRequestAttempts : 2; // Number of Link Request attempts
    uint8_t mLinkAcceptTimeout : 2;   // Timeout (in seconds) after sending Link Request waiting for Link Accept
#if !OPENTHREAD_CONFIG_MLE_LONG_ROUTES_ENABLE
    uint8_t mCost : 4; // The cost to this router via neighbor router
#else
    uint8_t mCost;
#endif
#if OPENTHREAD_CONFIG_PARENT_SEARCH_ENABLE
    uint16_t mIsSelectableAsParent : 1;
    uint16_t mParentReselectTimeout : 15;
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
