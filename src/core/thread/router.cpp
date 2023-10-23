/*
 *  Copyright (c) 2016-2017, The OpenThread Authors.
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
 *   This file includes definitions for Thread `Router` and `Parent`.
 */

#include "router.hpp"

#include "common/array.hpp"
#include "common/code_utils.hpp"
#include "common/debug.hpp"
#include "common/locator_getters.hpp"
#include "common/num_utils.hpp"
#include "instance/instance.hpp"

namespace ot {

void Router::Info::SetFrom(const Router &aRouter)
{
    Clear();
    mRloc16          = aRouter.GetRloc16();
    mRouterId        = Mle::RouterIdFromRloc16(mRloc16);
    mExtAddress      = aRouter.GetExtAddress();
    mAllocated       = true;
    mNextHop         = aRouter.GetNextHop();
    mLinkEstablished = aRouter.IsStateValid();
    mPathCost        = aRouter.GetCost();
    mLinkQualityIn   = aRouter.GetLinkQualityIn();
    mLinkQualityOut  = aRouter.GetLinkQualityOut();
    mAge             = static_cast<uint8_t>(Time::MsecToSec(TimerMilli::GetNow() - aRouter.GetLastHeard()));
    mVersion         = ClampToUint8(aRouter.GetVersion());
}

void Router::Info::SetFrom(const Parent &aParent)
{
    SetFrom(static_cast<const Router &>(aParent));
#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
    mCslClockAccuracy = aParent.GetCslAccuracy().GetClockAccuracy();
    mCslUncertainty   = aParent.GetCslAccuracy().GetUncertainty();
#endif
}

void Router::Clear(void)
{
    Instance &instance = GetInstance();

    memset(reinterpret_cast<void *>(this), 0, sizeof(Router));
    Init(instance);
}

LinkQuality Router::GetTwoWayLinkQuality(void) const { return Min(GetLinkQualityIn(), GetLinkQualityOut()); }

void Router::SetFrom(const Parent &aParent)
{
    // We use an intermediate pointer to copy `aParent` to silence
    // code checkers warning about object slicing (assigning a
    // sub-class to base class instance).

    const Router *parentAsRouter = &aParent;

    *this = *parentAsRouter;
}

void Parent::Clear(void)
{
    Instance &instance = GetInstance();

    memset(reinterpret_cast<void *>(this), 0, sizeof(Parent));
    Init(instance);
}

bool Router::SetNextHopAndCost(uint8_t aNextHop, uint8_t aCost)
{
    bool changed = false;

    if (mNextHop != aNextHop)
    {
        mNextHop = aNextHop;
        changed  = true;
    }

    if (mCost != aCost)
    {
        mCost   = aCost;
        changed = true;
    }

    return changed;
}

bool Router::SetNextHopToInvalid(void) { return SetNextHopAndCost(Mle::kInvalidRouterId, 0); }

} // namespace ot
