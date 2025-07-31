/*
 *  Copyright (c) 2025, The OpenThread Authors.
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

#include "openthread-core-config.h"

#include "coap/coap_message.hpp"
#include "common/as_core_type.hpp"
#include "common/debug.hpp"
#include "instance/instance.hpp"
#include "openthread/ext_network_diagnostic.h"
#include "thread/ext_network_diagnostic.hpp"

using namespace ot;

#if OPENTHREAD_CONFIG_EXT_NETWORK_DIAGNOSTIC_CLIENT_ENABLE

otError otExtNetworkDiagnosticGetNextContext(const otMessage                *aMessage,
                                             otExtNetworkDiagnosticIterator *aIterator,
                                             otExtNetworkDiagnosticContext  *aContext)
{
    AssertPointerIsNotNull(aMessage);
    AssertPointerIsNotNull(aIterator);
    AssertPointerIsNotNull(aContext);

    return ExtNetworkDiagnostic::Client::GetNextContext(AsCoapMessage(aMessage), *aIterator, *aContext);
}

otError otExtNetworkDiagnosticGetNextTlv(const otMessage               *aMessage,
                                         otExtNetworkDiagnosticContext *aContext,
                                         otExtNetworkDiagnosticTlv     *aTlv)
{
    AssertPointerIsNotNull(aMessage);
    AssertPointerIsNotNull(aContext);
    AssertPointerIsNotNull(aTlv);

    return ExtNetworkDiagnostic::Client::GetNextTlv(AsCoapMessage(aMessage), *aContext, *aTlv);
}

otError otExtNetworkDiagnosticGetIp6Addresses(const otMessage *aMessage,
                                              uint16_t         aDataOffset,
                                              uint16_t         aCount,
                                              otIp6Address    *aAddresses)
{
    AssertPointerIsNotNull(aMessage);

    return ExtNetworkDiagnostic::Client::GetIp6Addresses(AsCoapMessage(aMessage), aDataOffset, aCount, aAddresses);
}

otError otExtNetworkDiagnosticGetAlocs(const otMessage *aMessage,
                                       uint16_t         aDataOffset,
                                       uint16_t         aCount,
                                       uint8_t         *aAlocs)
{
    AssertPointerIsNotNull(aMessage);

    return ExtNetworkDiagnostic::Client::GetAlocs(AsCoapMessage(aMessage), aDataOffset, aCount, aAlocs);
}

otError otExtNetworkDiagnosticGetRouteData(const otMessage                 *aMessage,
                                           uint16_t                         aDataOffset,
                                           const uint8_t                   *aRouterIdMask,
                                           uint8_t                          aCount,
                                           otExtNetworkDiagnosticRouteData *aRouteData)
{
    return ExtNetworkDiagnostic::Client::GetRouteData(AsCoapMessage(aMessage), aDataOffset, aRouterIdMask, aCount,
                                                      aRouteData);
}

void otExtNetworkDiagnosticStartClient(otInstance                                *aInstance,
                                       const otExtNetworkDiagnosticTlvSet        *aHost,
                                       const otExtNetworkDiagnosticTlvSet        *aChild,
                                       const otExtNetworkDiagnosticTlvSet        *aNeighbor,
                                       otExtNetworkDiagnosticServerUpdateCallback aCallback,
                                       void                                      *aContext)
{
    AsCoreType(aInstance).Get<ExtNetworkDiagnostic::Client>().Start(AsCoreTypePtr(aHost), AsCoreTypePtr(aChild),
                                                                    AsCoreTypePtr(aNeighbor), aCallback, aContext);
}

void otExtNetworkDiagnosticStopClient(otInstance *aInstance)
{
    AsCoreType(aInstance).Get<ExtNetworkDiagnostic::Client>().Stop();
}

bool otExtNetworkDiagnosticTlvIsSet(const otExtNetworkDiagnosticTlvSet *aTlvSet, uint8_t aTlv)
{
    bool set = false;

    VerifyOrExit(aTlvSet != nullptr);
    VerifyOrExit(ExtNetworkDiagnostic::Tlv::IsKnownTlv(aTlv));

    set = AsCoreType(aTlvSet).IsSet(static_cast<ExtNetworkDiagnostic::Tlv::Type>(aTlv));

exit:
    return set;
}

otError otExtNetworkDiagnosticSetTlv(otExtNetworkDiagnosticTlvSet *aTlvSet, uint8_t aTlv)
{
    Error error = kErrorNone;

    VerifyOrExit(aTlvSet != nullptr, error = kErrorInvalidArgs);
    VerifyOrExit(ExtNetworkDiagnostic::Tlv::IsKnownTlv(aTlv), error = kErrorInvalidArgs);

    AsCoreType(aTlvSet).Set(static_cast<ExtNetworkDiagnostic::Tlv::Type>(aTlv));

exit:
    return error;
}

void otExtNetworkDiagnosticClearTlv(otExtNetworkDiagnosticTlvSet *aTlvSet, uint8_t aTlv)
{
    VerifyOrExit(aTlvSet != nullptr);
    VerifyOrExit(ExtNetworkDiagnostic::Tlv::IsKnownTlv(aTlv));

    AsCoreType(aTlvSet).Clear(static_cast<ExtNetworkDiagnostic::Tlv::Type>(aTlv));

exit:
    return;
}

#endif // OPENTHREAD_CONFIG_EXT_NETWORK_DIAGNOSTIC_CLIENT_ENABLE
