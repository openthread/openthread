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
 *   This file implements IPv6 datagram filtering.
 */

#include "ip6_filter.hpp"

#include <stdio.h>

#include "common/code_utils.hpp"
#include "common/locator_getters.hpp"
#include "common/log.hpp"
#include "instance/instance.hpp"
#include "meshcop/meshcop.hpp"
#include "net/ip6.hpp"
#include "net/tcp6.hpp"
#include "net/udp6.hpp"
#include "thread/mle.hpp"

namespace ot {
namespace Ip6 {

RegisterLogModule("Ip6Filter");

bool Filter::Accept(Message &aMessage) const
{
    bool     rval = false;
    Headers  headers;
    uint16_t dstPort;

    // Allow all received IPv6 datagrams with link security enabled
    if (aMessage.IsLinkSecurityEnabled())
    {
        ExitNow(rval = true);
    }

    SuccessOrExit(headers.ParseFrom(aMessage));

    // Allow only link-local unicast or multicast
    VerifyOrExit(headers.GetDestinationAddress().IsLinkLocal() ||
                 headers.GetDestinationAddress().IsLinkLocalMulticast());

    // Allow all link-local IPv6 datagrams when Thread is not enabled
    if (Get<Mle::MleRouter>().GetRole() == Mle::kRoleDisabled)
    {
        ExitNow(rval = true);
    }

    dstPort = headers.GetDestinationPort();

    switch (headers.GetIpProto())
    {
    case kProtoUdp:
        // Allow MLE traffic
        if (dstPort == Mle::kUdpPort)
        {
            ExitNow(rval = true);
        }

#if OPENTHREAD_CONFIG_BORDER_AGENT_ENABLE
        // Allow native commissioner traffic
        if (Get<KeyManager>().GetSecurityPolicy().mNativeCommissioningEnabled &&
            dstPort == Get<MeshCoP::BorderAgent>().GetUdpPort())
        {
            ExitNow(rval = true);
        }
#endif
        break;

    case kProtoTcp:
        break;

    default:
        // Allow UDP or TCP traffic only
        ExitNow();
    }

    // Check against allowed unsecure port list
    rval = mUnsecurePorts.Contains(dstPort);

exit:
    return rval;
}

Error Filter::UpdateUnsecurePorts(Action aAction, uint16_t aPort)
{
    Error     error = kErrorNone;
    uint16_t *entry;

    VerifyOrExit(aPort != 0, error = kErrorInvalidArgs);

    entry = mUnsecurePorts.Find(aPort);

    if (aAction == kAdd)
    {
        VerifyOrExit(entry == nullptr);
        SuccessOrExit(error = mUnsecurePorts.PushBack(aPort));
    }
    else
    {
        VerifyOrExit(entry != nullptr, error = kErrorNotFound);
        mUnsecurePorts.Remove(*entry);
    }

    LogInfo("%s unsecure port %d", (aAction == kAdd) ? "Added" : "Removed", aPort);

exit:
    return error;
}

} // namespace Ip6
} // namespace ot
