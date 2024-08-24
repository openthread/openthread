/*
 *  Copyright (c) 2024, The OpenThread Authors.
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
 *   This file implements the CCM Joiner role.
 */

#include "../joiner.hpp"

#if OPENTHREAD_CONFIG_JOINER_ENABLE

#include <stdio.h>

#include "common/array.hpp"
#include "common/as_core_type.hpp"
#include "common/code_utils.hpp"
#include "common/debug.hpp"
#include "common/encoding.hpp"
#include "common/locator_getters.hpp"
#include "common/log.hpp"
#include "common/string.hpp"
#include "instance/instance.hpp"
#include "meshcop/meshcop.hpp"
#include "radio/radio.hpp"
#include "thread/thread_netif.hpp"
#include "thread/uri_paths.hpp"
#include "utils/otns.hpp"

namespace ot {
namespace MeshCoP {

RegisterLogModule("JoinerCcm");

void Joiner::SetCcmIdentity(const uint8_t *aX509Cert,
                            uint32_t       aX509Length,
                            const uint8_t *aPrivateKey,
                            uint32_t       aPrivateKeyLength,
                            const uint8_t *aX509CaCertificateChain,
                            uint32_t aX509CaCertChainLength)
{
    Get<Tmf::SecureAgent>().SetCertificate(aX509Cert, aX509Length, aPrivateKey, aPrivateKeyLength);
    Get<Tmf::SecureAgent>().SetCaCertificateChain(aX509CaCertificateChain, aX509CaCertChainLength);
    Get<Tmf::SecureAgent>().SetSslAuthMode(false); // MUST provisionally trust any Registrar
}

Error Joiner::StartCcmAe(otJoinerCallback aCallback,
                    void            *aContext)
{
    Error                        error;
    Mac::ExtAddress              randomAddress;
    SteeringData::HashBitIndexes filterIndexes;

    LogInfo("JoinerCcm starting AE");

    VerifyOrExit(mState == kStateIdle, error = kErrorBusy);
    VerifyOrExit(Get<ThreadNetif>().IsUp() && Get<Mle::Mle>().GetRole() == Mle::kRoleDisabled,
                 error = kErrorInvalidState);

    mJoinerType = kTypeCcmAe;
    mJoinerSourcePort = kCcmAeJoinerUdpSourcePort;

    // Use random-generated extended address.
    randomAddress.GenerateRandom();
    Get<Mac::Mac>().SetExtAddress(randomAddress);
    Get<Mle::MleRouter>().UpdateLinkLocalAddress();

    SuccessOrExit(error = Get<Tmf::SecureAgent>().Start(mJoinerSourcePort));

    for (JoinerRouter &router : mJoinerRouters)
    {
        router.mPriority = 0; // Priority zero means entry is not in-use.
    }

    if (!mDiscerner.IsEmpty())
    {
        SteeringData::CalculateHashBitIndexes(mDiscerner, filterIndexes);
    }
    else
    {
        SteeringData::CalculateHashBitIndexes(mId, filterIndexes);
    }

    SuccessOrExit(error = Get<Mle::DiscoverScanner>().Discover(Mac::ChannelMask(0), Get<Mac::Mac>().GetPanId(),
                                                               /* aJoiner */ true, /* aEnableFiltering */ true,
                                                               &filterIndexes, HandleDiscoverResult, this));

    mCallback.Set(aCallback, aContext);
    SetState(kStateDiscover);

exit:
    LogWarnOnError(error, "start JoinerCcm AE");
    return error;
}

Error Joiner::StartCcmNkp(otJoinerCallback aCallback,
                         void            *aContext)
{
    Error                        error;
    Mac::ExtAddress              randomAddress;
    SteeringData::HashBitIndexes filterIndexes;

    LogInfo("JoinerCcm starting NKP");

    VerifyOrExit(mState == kStateIdle, error = kErrorBusy);
    VerifyOrExit(Get<ThreadNetif>().IsUp() && Get<Mle::Mle>().GetRole() == Mle::kRoleDisabled,
                 error = kErrorInvalidState);

    mJoinerType = kTypeCcmNkp;
    mJoinerSourcePort = kCcmAeJoinerUdpSourcePort;

    // Use random-generated extended address.
    randomAddress.GenerateRandom();
    Get<Mac::Mac>().SetExtAddress(randomAddress);
    Get<Mle::MleRouter>().UpdateLinkLocalAddress();

    SuccessOrExit(error = Get<Tmf::SecureAgent>().Start(mJoinerSourcePort));

    for (JoinerRouter &router : mJoinerRouters)
    {
        router.mPriority = 0; // Priority zero means entry is not in-use.
    }

    if (!mDiscerner.IsEmpty())
    {
        SteeringData::CalculateHashBitIndexes(mDiscerner, filterIndexes);
    }
    else
    {
        SteeringData::CalculateHashBitIndexes(mId, filterIndexes);
    }

    SuccessOrExit(error = Get<Mle::DiscoverScanner>().Discover(Mac::ChannelMask(0), Get<Mac::Mac>().GetPanId(),
                                                               /* aJoiner */ true, /* aEnableFiltering */ true,
                                                               &filterIndexes, HandleDiscoverResult, this));

    mCallback.Set(aCallback, aContext);
    SetState(kStateDiscover);

exit:
    LogWarnOnError(error, "start JoinerCcm NKP");
    return error;
}

} // namespace MeshCoP
} // namespace ot

#endif // OPENTHREAD_CONFIG_JOINER_ENABLE
