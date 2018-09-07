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
#include "common/instance.hpp"
#include "net/ip6.hpp"
#include "net/tcp.hpp"
#include "net/udp6.hpp"
#include "thread/mle.hpp"

namespace ot {
namespace Ip6 {

Filter::Filter(void)
{
    memset(mUnsecurePorts, 0, sizeof(mUnsecurePorts));
}

bool Filter::Accept(Message &aMessage) const
{
    bool      rval = false;
    Header    ip6;
    UdpHeader udp;
    TcpHeader tcp;
    uint16_t  dstport;

    // Allow all received IPv6 datagrams with link security enabled
    if (aMessage.IsLinkSecurityEnabled())
    {
        ExitNow(rval = true);
    }

    // Read IPv6 header
    VerifyOrExit(sizeof(ip6) == aMessage.Read(0, sizeof(ip6), &ip6));

    // Allow only link-local unicast or multicast
    VerifyOrExit(ip6.GetDestination().IsLinkLocal() || ip6.GetDestination().IsLinkLocalMulticast());

    switch (ip6.GetNextHeader())
    {
    case kProtoUdp:
        // Read the UDP header and get the dst port
        VerifyOrExit(sizeof(udp) == aMessage.Read(sizeof(ip6), sizeof(udp), &udp));

        dstport = udp.GetDestinationPort();

        // Allow MLE traffic
        if (dstport == Mle::kUdpPort)
        {
            ExitNow(rval = true);
        }

        break;

    case kProtoTcp:
        // Read the TCP header and get the dst port
        VerifyOrExit(sizeof(tcp) == aMessage.Read(sizeof(ip6), sizeof(tcp), &tcp));

        dstport = tcp.GetDestinationPort();

        break;

    default:
        // Allow UDP or TCP traffic only
        ExitNow();
    }

    // Check against allowed unsecure port list
    for (int i = 0; i < kMaxUnsecurePorts; i++)
    {
        if (mUnsecurePorts[i] != 0 && mUnsecurePorts[i] == dstport)
        {
            ExitNow(rval = true);
        }
    }

exit:
    return rval;
}

otError Filter::AddUnsecurePort(uint16_t aPort)
{
    otError error = OT_ERROR_NONE;

    for (int i = 0; i < kMaxUnsecurePorts; i++)
    {
        if (mUnsecurePorts[i] == aPort)
        {
            ExitNow();
        }
    }

    for (int i = 0; i < kMaxUnsecurePorts; i++)
    {
        if (mUnsecurePorts[i] == 0)
        {
            mUnsecurePorts[i] = aPort;
            ExitNow();
        }
    }

    ExitNow(error = OT_ERROR_NO_BUFS);

exit:
    return error;
}

otError Filter::RemoveUnsecurePort(uint16_t aPort)
{
    otError error = OT_ERROR_NONE;

    for (int i = 0; i < kMaxUnsecurePorts; i++)
    {
        if (mUnsecurePorts[i] == aPort)
        {
            // Shift all of the ports higher than this
            // port down.
            for (; i < kMaxUnsecurePorts - 1; i++)
            {
                mUnsecurePorts[i] = mUnsecurePorts[i + 1];
            }

            // Clear the last port entry.
            mUnsecurePorts[i] = 0;
            ExitNow();
        }
    }

    ExitNow(error = OT_ERROR_NOT_FOUND);

exit:
    return error;
}

void Filter::RemoveAllUnsecurePorts(void)
{
    memset(mUnsecurePorts, 0, sizeof(mUnsecurePorts));
}

const uint16_t *Filter::GetUnsecurePorts(uint8_t &aNumEntries) const
{
    // Count the number of unsecure ports.
    for (aNumEntries = 0; aNumEntries < kMaxUnsecurePorts; aNumEntries++)
    {
        if (mUnsecurePorts[aNumEntries] == 0)
        {
            break;
        }
    }

    return mUnsecurePorts;
}

} // namespace Ip6
} // namespace ot
