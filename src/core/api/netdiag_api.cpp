/*
 *  Copyright (c) 2020, The OpenThread Authors.
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
 *   This file implements the OpenThread Network Diagnostic API.
 */

#include "openthread-core-config.h"

#include <openthread/netdiag.h>

#include "common/as_core_type.hpp"
#include "common/locator_getters.hpp"

using namespace ot;

#if OPENTHREAD_CONFIG_TMF_NETDIAG_CLIENT_ENABLE

otError otThreadGetNextDiagnosticTlv(const otMessage       *aMessage,
                                     otNetworkDiagIterator *aIterator,
                                     otNetworkDiagTlv      *aNetworkDiagTlv)
{
    AssertPointerIsNotNull(aIterator);
    AssertPointerIsNotNull(aNetworkDiagTlv);

    return NetworkDiagnostic::Client::GetNextDiagTlv(AsCoapMessage(aMessage), *aIterator, *aNetworkDiagTlv);
}

otError otThreadSendDiagnosticGet(otInstance                    *aInstance,
                                  const otIp6Address            *aDestination,
                                  const uint8_t                  aTlvTypes[],
                                  uint8_t                        aCount,
                                  otReceiveDiagnosticGetCallback aCallback,
                                  void                          *aCallbackContext)
{
    return AsCoreType(aInstance).Get<NetworkDiagnostic::Client>().SendDiagnosticGet(
        AsCoreType(aDestination), aTlvTypes, aCount, aCallback, aCallbackContext);
}

otError otThreadSendDiagnosticReset(otInstance         *aInstance,
                                    const otIp6Address *aDestination,
                                    const uint8_t       aTlvTypes[],
                                    uint8_t             aCount)
{
    return AsCoreType(aInstance).Get<NetworkDiagnostic::Client>().SendDiagnosticReset(AsCoreType(aDestination),
                                                                                      aTlvTypes, aCount);
}

#endif // OPENTHREAD_CONFIG_TMF_NETDIAG_CLIENT_ENABLE

const char *otThreadGetVendorName(otInstance *aInstance)
{
    return AsCoreType(aInstance).Get<NetworkDiagnostic::Server>().GetVendorName();
}

const char *otThreadGetVendorModel(otInstance *aInstance)
{
    return AsCoreType(aInstance).Get<NetworkDiagnostic::Server>().GetVendorModel();
}

const char *otThreadGetVendorSwVersion(otInstance *aInstance)
{
    return AsCoreType(aInstance).Get<NetworkDiagnostic::Server>().GetVendorSwVersion();
}

#if OPENTHREAD_CONFIG_NET_DIAG_VENDOR_INFO_SET_API_ENABLE
otError otThreadSetVendorName(otInstance *aInstance, const char *aVendorName)
{
    return AsCoreType(aInstance).Get<NetworkDiagnostic::Server>().SetVendorName(aVendorName);
}

otError otThreadSetVendorModel(otInstance *aInstance, const char *aVendorModel)
{
    return AsCoreType(aInstance).Get<NetworkDiagnostic::Server>().SetVendorModel(aVendorModel);
}

otError otThreadSetVendorSwVersion(otInstance *aInstance, const char *aVendorSwVersion)
{
    return AsCoreType(aInstance).Get<NetworkDiagnostic::Server>().SetVendorSwVersion(aVendorSwVersion);
}
#endif
