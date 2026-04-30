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
 *   This file implements Network Identity tracker.
 */

#include "network_identity.hpp"

#include "instance/instance.hpp"

namespace ot {
namespace MeshCoP {

const otExtendedPanId NetworkIdentity::sExtendedPanidInit = {
    {0xde, 0xad, 0x00, 0xbe, 0xef, 0x00, 0xca, 0xfe},
};

const char NetworkIdentity::kDefaultNetworkName[] = "OpenThread";
const char NetworkIdentity::kDefaultDomainName[]  = "DefaultDomain"; // Section 5.22 Thread spec, MUST NOT change.

NetworkIdentity::NetworkIdentity(Instance &aInstance)
    : InstanceLocator(aInstance)
{
    mExtendedPanId.Clear();
    SetExtPanId(AsCoreType(&sExtendedPanidInit));

    IgnoreError(SetNetworkName(kDefaultNetworkName));

#if (OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2)
    IgnoreError(SetDomainName(kDefaultDomainName));
#endif
}

void NetworkIdentity::SetExtPanId(const ExtendedPanId &aExtendedPanId)
{
    IgnoreError(Get<Notifier>().Update(mExtendedPanId, aExtendedPanId, kEventThreadExtPanIdChanged));
}

Error NetworkIdentity::SetNetworkName(const char *aNameString)
{
    return SignalNetworkNameChange(mNetworkName.Set(aNameString));
}

Error NetworkIdentity::SetNetworkName(const NameData &aNameData)
{
    return SignalNetworkNameChange(mNetworkName.Set(aNameData));
}

Error NetworkIdentity::SignalNetworkNameChange(Error aError)
{
    switch (aError)
    {
    case kErrorNone:
        Get<Notifier>().Signal(kEventThreadNetworkNameChanged);
        break;

    case kErrorAlready:
        Get<Notifier>().SignalIfFirst(kEventThreadNetworkNameChanged);
        aError = kErrorNone;
        break;

    default:
        break;
    }

    return aError;
}

#if (OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2)
Error NetworkIdentity::SetDomainName(const char *aNameString)
{
    Error error = mDomainName.Set(aNameString);

    return (error == kErrorAlready) ? kErrorNone : error;
}

Error NetworkIdentity::SetDomainName(const NameData &aNameData)
{
    Error error = mDomainName.Set(aNameData);

    return (error == kErrorAlready) ? kErrorNone : error;
}

bool NetworkIdentity::IsDefaultDomainNameSet(void) const
{
    return StringMatch(mDomainName.GetAsCString(), kDefaultDomainName);
}
#endif // (OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2)

} // namespace MeshCoP
} // namespace ot
