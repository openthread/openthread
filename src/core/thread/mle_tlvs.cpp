/*
 *  Copyright (c) 2022, The OpenThread Authors.
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
 *   This file implements function for generating and processing MLE TLVs.
 */

#include "mle_tlvs.hpp"

#include "common/code_utils.hpp"

namespace ot {
namespace Mle {

#if !OPENTHREAD_CONFIG_MLE_LONG_ROUTES_ENABLE

void RouteTlv::Init(void)
{
    SetType(kRoute);
    SetLength(sizeof(*this) - sizeof(Tlv));
    mRouterIdMask.Clear();
    memset(mRouteData, 0, sizeof(mRouteData));
}

bool RouteTlv::IsValid(void) const
{
    bool    isValid = false;
    uint8_t numAllocatedIds;

    VerifyOrExit(GetLength() >= sizeof(mRouterIdSequence) + sizeof(mRouterIdMask));

    numAllocatedIds = mRouterIdMask.GetNumberOfAllocatedIds();
    VerifyOrExit(numAllocatedIds <= Mle::kMaxRouters);

    isValid = (GetRouteDataLength() >= numAllocatedIds);

exit:
    return isValid;
}

#endif // #if !OPENTHREAD_CONFIG_MLE_LONG_ROUTES_ENABLE

void ConnectivityTlv::IncrementLinkQuality(LinkQuality aLinkQuality)
{
    switch (aLinkQuality)
    {
    case kLinkQuality0:
        break;
    case kLinkQuality1:
        mLinkQuality1++;
        break;
    case kLinkQuality2:
        mLinkQuality2++;
        break;
    case kLinkQuality3:
        mLinkQuality3++;
        break;
    }
}

} // namespace Mle
} // namespace ot
