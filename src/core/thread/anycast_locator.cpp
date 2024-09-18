/*
 *  Copyright (c) 2021, The OpenThread Authors.
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
 *   This file implements Anycast Locator functionality.
 */

#include "anycast_locator.hpp"

#if OPENTHREAD_CONFIG_TMF_ANYCAST_LOCATOR_ENABLE

#include "instance/instance.hpp"

namespace ot {

AnycastLocator::AnycastLocator(Instance &aInstance)
    : InstanceLocator(aInstance)

{
}

Error AnycastLocator::Locate(const Ip6::Address &aAnycastAddress, LocatorCallback aCallback, void *aContext)
{
    Error            error   = kErrorNone;
    Coap::Message   *message = nullptr;
    Tmf::MessageInfo messageInfo(GetInstance());

    VerifyOrExit((aCallback != nullptr) && Get<Mle::Mle>().IsAnycastLocator(aAnycastAddress),
                 error = kErrorInvalidArgs);

    message = Get<Tmf::Agent>().NewConfirmablePostMessage(kUriAnycastLocate);
    VerifyOrExit(message != nullptr, error = kErrorNoBufs);

    if (mCallback.IsSet())
    {
        IgnoreError(Get<Tmf::Agent>().AbortTransaction(HandleResponse, this));
    }

    messageInfo.SetSockAddrToRlocPeerAddrTo(aAnycastAddress);

    SuccessOrExit(error = Get<Tmf::Agent>().SendMessage(*message, messageInfo, HandleResponse, this));

    mCallback.Set(aCallback, aContext);

exit:
    FreeMessageOnError(message, error);
    return error;
}

void AnycastLocator::HandleResponse(void                *aContext,
                                    otMessage           *aMessage,
                                    const otMessageInfo *aMessageInfo,
                                    Error                aError)
{
    static_cast<AnycastLocator *>(aContext)->HandleResponse(AsCoapMessagePtr(aMessage), AsCoreTypePtr(aMessageInfo),
                                                            aError);
}

void AnycastLocator::HandleResponse(Coap::Message *aMessage, const Ip6::MessageInfo *aMessageInfo, Error aError)
{
    OT_UNUSED_VARIABLE(aMessageInfo);

    uint16_t            rloc16  = Mle::kInvalidRloc16;
    const Ip6::Address *address = nullptr;
    Ip6::Address        meshLocalAddress;

    SuccessOrExit(aError);
    OT_ASSERT(aMessage != nullptr);

    meshLocalAddress.SetPrefix(Get<Mle::Mle>().GetMeshLocalPrefix());
    SuccessOrExit(Tlv::Find<ThreadMeshLocalEidTlv>(*aMessage, meshLocalAddress.GetIid()));
    SuccessOrExit(Tlv::Find<ThreadRloc16Tlv>(*aMessage, rloc16));

#if OPENTHREAD_FTD
    Get<AddressResolver>().UpdateSnoopedCacheEntry(meshLocalAddress, rloc16, Get<Mac::Mac>().GetShortAddress());
#endif

    address = &meshLocalAddress;

exit:
    mCallback.InvokeAndClearIfSet(aError, address, rloc16);
}

#if OPENTHREAD_CONFIG_TMF_ANYCAST_LOCATOR_SEND_RESPONSE

template <>
void AnycastLocator::HandleTmf<kUriAnycastLocate>(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    Coap::Message *message = nullptr;

    VerifyOrExit(aMessage.IsConfirmablePostRequest());

    message = Get<Tmf::Agent>().NewResponseMessage(aMessage);
    VerifyOrExit(message != nullptr);

    SuccessOrExit(Tlv::Append<ThreadMeshLocalEidTlv>(*message, Get<Mle::Mle>().GetMeshLocalEid().GetIid()));
    SuccessOrExit(Tlv::Append<ThreadRloc16Tlv>(*message, Get<Mle::Mle>().GetRloc16()));

    SuccessOrExit(Get<Tmf::Agent>().SendMessage(*message, aMessageInfo));
    message = nullptr;

exit:
    FreeMessage(message);
}

#endif // OPENTHREAD_CONFIG_TMF_ANYCAST_LOCATOR_SEND_RESPONSE

} // namespace ot

#endif // OPENTHREAD_CONFIG_TMF_ANYCAST_LOCATOR_ENABLE
