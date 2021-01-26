/*
 *  Copyright (c) 2018, The OpenThread Authors.
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
 *   This file implements MTD-specific mesh forwarding of IPv6/6LoWPAN messages.
 */

#if OPENTHREAD_MTD

#include "mesh_forwarder.hpp"

#include "common/locator-getters.hpp"

namespace ot {

otError MeshForwarder::SendMessage(Message &aMessage)
{
#if OPENTHREAD_MTD_S2S
    Mle::MleRouter &mle = Get<Mle::MleRouter>();
    Child *         rx_off_child;
#endif

    aMessage.SetDirectTransmission();
    aMessage.SetOffset(0);
    aMessage.SetDatagramTag(0);

    mSendQueue.Enqueue(aMessage);

#if OPENTHREAD_MTD_S2S
    switch (aMessage.GetType())
    {
    case Message::kTypeIp6:
    {
        Ip6::Header ip6Header;

        IgnoreError(aMessage.Read(0, ip6Header));

        if (ip6Header.GetDestination().IsMulticast())
        {
            // For traffic destined to multicast address larger than realm local, generally it uses IP-in-IP
            // encapsulation (RFC2473), with outer destination as ALL_MPL_FORWARDERS. So here if the destination
            // is multicast address larger than realm local, it should be for indirection transmission for the
            // device's sleepy child, thus there should be no direct transmission.
            if (!ip6Header.GetDestination().IsMulticastLargerThanRealmLocal())
            {
                // schedule direct transmission
                aMessage.SetDirectTransmission();
            }

            if (aMessage.GetSubType() != Message::kSubTypeMplRetransmission)
            {
                if (ip6Header.GetDestination() == mle.GetLinkLocalAllThreadNodesAddress() ||
                    ip6Header.GetDestination() == mle.GetRealmLocalAllThreadNodesAddress())
                {
                    // destined for all rx-off neighbors
                    for (Child &neighbor : Get<ChildTable>().Iterate(Child::kInStateValidOrRestoring))
                    {
                        if (!neighbor.IsRxOnWhenIdle())
                        {
                            mIndirectSender.AddMessageForSleepyChild(aMessage, neighbor);
                        }
                    }
                }
                else
                {
                    // destined for some sleepy children which subscribed the multicast address.
                    for (Child &child : Get<ChildTable>().Iterate(Child::kInStateValidOrRestoring))
                    {
                        if (!child.IsRxOnWhenIdle() && child.HasIp6Address(ip6Header.GetDestination()))
                        {
                            mIndirectSender.AddMessageForSleepyChild(aMessage, child);
                        }
                    }
                }
            }
        }
        else if ((rx_off_child = Get<ChildTable>().FindChild(ip6Header.GetDestination())) != nullptr &&
                 !rx_off_child->IsRxOnWhenIdle() && !aMessage.GetDirectTransmission())
        {
            // destined for an rx-off child
            mIndirectSender.AddMessageForSleepyChild(aMessage, *rx_off_child);
        }
        else
        {
            // schedule direct transmission
            aMessage.SetDirectTransmission();
        }

        break;
    }

    default:
        aMessage.SetDirectTransmission();
        break;
    }
#endif // OPENTHREAD_MTD_S2S

    mScheduleTransmissionTask.Post();

    return OT_ERROR_NONE;
}

otError MeshForwarder::EvictMessage(Message::Priority aPriority)
{
    otError  error = OT_ERROR_NOT_FOUND;
    Message *message;

    VerifyOrExit((message = mSendQueue.GetTail()) != nullptr);

    if (message->GetPriority() < static_cast<uint8_t>(aPriority))
    {
        RemoveMessage(*message);
        ExitNow(error = OT_ERROR_NONE);
    }

exit:
    return error;
}

} // namespace ot

#endif // OPENTHREAD_MTD
