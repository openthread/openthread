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
 *   This file includes definitions for Anycast Locator functionality.
 */

#ifndef ANYCAST_LOCATOR_HPP_
#define ANYCAST_LOCATOR_HPP_

#include "openthread-core-config.h"

#if OPENTHREAD_CONFIG_TMF_ANYCAST_LOCATOR_ENABLE

#include "common/callback.hpp"
#include "common/locator.hpp"
#include "common/non_copyable.hpp"
#include "net/ip6_address.hpp"
#include "thread/tmf.hpp"

namespace ot {

/**
 * Implements Anycast Locator functionality which allows caller to determine the mesh local EID and RLOC16
 * of the closest destination of an anycast address (if any).
 *
 * The closest destination is determined based on the current routing table and path costs within the Thread mesh.
 */
class AnycastLocator : public InstanceLocator, private NonCopyable
{
    friend class Tmf::Agent;

public:
    /**
     * Pointer type defines the callback to notify the outcome of a request.
     */
    typedef otThreadAnycastLocatorCallback LocatorCallback;

    /**
     * Initializes the `AnycastLocator` object.
     *
     * @param[in]  aInstance  A reference to the OpenThread instance.
     */
    explicit AnycastLocator(Instance &aInstance);

    /**
     * Requests the closest destination of a given anycast address to be located.
     *
     * If a previous `Locate()` request is ongoing, a subsequent call to this method will cancel and replace the
     * earlier request.
     *
     * @param[in] aAnycastAddress   The anycast address to locate the closest destination of.
     * @param[in] aCallback         The callback handler to report the result.
     * @param[in] aContext          An arbitrary context used with @p aCallback.
     *
     * @retval kErrorNone         The request started successfully. @p aCallback will be invoked to report the result.
     * @retval kErrorNoBufs       Out of buffers to prepare and send the request message.
     * @retval kErrorInvalidArgs  The @p aAnycastAddress is not a valid anycast address or @p aCallback is `nullptr`.
     */
    Error Locate(const Ip6::Address &aAnycastAddress, LocatorCallback aCallback, void *aContext);

    /**
     * Indicates whether an earlier request is in progress.
     *
     * @returns TRUE if an earlier request is in progress, FALSE otherwise.
     */
    bool IsInProgress(void) const { return mCallback.IsSet(); }

private:
    static void HandleResponse(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo, Error aError);

    void HandleResponse(Coap::Message *aMessage, const Ip6::MessageInfo *aMessageInfo, Error aError);

    template <Uri kUri> void HandleTmf(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo);

    Callback<LocatorCallback> mCallback;
};

#if OPENTHREAD_CONFIG_TMF_ANYCAST_LOCATOR_SEND_RESPONSE
DeclareTmfHandler(AnycastLocator, kUriAnycastLocate);
#endif

} // namespace ot

#endif // OPENTHREAD_CONFIG_TMF_ANYCAST_LOCATOR_ENABLE

#endif //  ANYCAST_LOCATOR_HPP_
