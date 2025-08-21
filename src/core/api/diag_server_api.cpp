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
#include "openthread/diag_server.h"
#include "thread/diagnostic_server.hpp"

using namespace ot;

#if OPENTHREAD_CONFIG_DIAG_CLIENT_ENABLE

otError otDiagServerGetNextContext(const otMessage      *aMessage,
                                   otDiagServerIterator *aIterator,
                                   otDiagServerContext  *aContext)
{
    AssertPointerIsNotNull(aMessage);
    AssertPointerIsNotNull(aIterator);
    AssertPointerIsNotNull(aContext);

    return DiagnosticServer::Client::GetNextContext(AsCoapMessage(aMessage), *aIterator, *aContext);
}

otError otDiagServerGetNextTlv(const otMessage *aMessage, otDiagServerContext *aContext, otDiagServerTlv *aTlv)
{
    AssertPointerIsNotNull(aMessage);
    AssertPointerIsNotNull(aContext);
    AssertPointerIsNotNull(aTlv);

    return DiagnosticServer::Client::GetNextTlv(AsCoapMessage(aMessage), *aContext, *aTlv);
}

otError otDiagServerGetIp6Addresses(const otMessage *aMessage,
                                    uint16_t         aDataOffset,
                                    uint16_t         aCount,
                                    otIp6Address    *aAddresses)
{
    AssertPointerIsNotNull(aMessage);

    return DiagnosticServer::Client::GetIp6Addresses(AsCoapMessage(aMessage), aDataOffset, aCount, aAddresses);
}

otError otDiagServerGetAlocs(const otMessage *aMessage, uint16_t aDataOffset, uint16_t aCount, uint8_t *aAlocs)
{
    AssertPointerIsNotNull(aMessage);

    return DiagnosticServer::Client::GetAlocs(AsCoapMessage(aMessage), aDataOffset, aCount, aAlocs);
}

void otDiagServerStartClient(otInstance                *aInstance,
                             const otDiagServerTlvSet  *aHost,
                             const otDiagServerTlvSet  *aChild,
                             const otDiagServerTlvSet  *aNeighbor,
                             otDiagServerUpdateCallback aCallback,
                             void                      *aContext)
{
    AsCoreType(aInstance).Get<DiagnosticServer::Client>().Start(AsCoreTypePtr(aHost), AsCoreTypePtr(aChild),
                                                                AsCoreTypePtr(aNeighbor), aCallback, aContext);
}

void otDiagServerStopClient(otInstance *aInstance) { AsCoreType(aInstance).Get<DiagnosticServer::Client>().Stop(); }

bool otDiagServerGetTlv(const otDiagServerTlvSet *aSet, uint8_t aTlv)
{
    bool set = false;

    VerifyOrExit(aSet != nullptr);
    VerifyOrExit(DiagnosticServer::Tlv::IsKnownTlv(aTlv));

    set = AsCoreType(aSet).IsSet(static_cast<DiagnosticServer::Tlv::Type>(aTlv));

exit:
    return set;
}

otError otDiagServerSetTlv(otDiagServerTlvSet *aSet, uint8_t aTlv)
{
    Error error = kErrorNone;

    VerifyOrExit(aSet != nullptr, error = kErrorInvalidArgs);
    VerifyOrExit(DiagnosticServer::Tlv::IsKnownTlv(aTlv), error = kErrorInvalidArgs);

    AsCoreType(aSet).Set(static_cast<DiagnosticServer::Tlv::Type>(aTlv));

exit:
    return error;
}

void otDiagServerClearTlv(otDiagServerTlvSet *aSet, uint8_t aTlv)
{
    VerifyOrExit(aSet != nullptr);
    VerifyOrExit(DiagnosticServer::Tlv::IsKnownTlv(aTlv));

    AsCoreType(aSet).Clear(static_cast<DiagnosticServer::Tlv::Type>(aTlv));

exit:
    return;
}

#endif // OPENTHREAD_CONFIG_DIAG_CLIENT_ENABLE
