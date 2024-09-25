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
 *   This file implements Network Name management.
 */

#include "network_name.hpp"

#include "instance/instance.hpp"

namespace ot {
namespace MeshCoP {

uint8_t NameData::CopyTo(char *aBuffer, uint8_t aMaxSize) const
{
    MutableData<kWithUint8Length> destData;

    destData.Init(aBuffer, aMaxSize);
    destData.ClearBytes();
    IgnoreError(destData.CopyBytesFrom(*this));

    return destData.GetLength();
}

NameData NetworkName::GetAsData(void) const
{
    return NameData(m8, static_cast<uint8_t>(StringLength(m8, kMaxSize + 1)));
}

Error NetworkName::Set(const char *aNameString)
{
    // When setting `NetworkName` from a string, we treat it as `NameData`
    // with `kMaxSize + 1` chars. `NetworkName::Set(data)` will look
    // for null char in the data (within its given size) to calculate
    // the name's length and ensure that the name fits in `kMaxSize`
    // chars. The `+ 1` ensures that a `aNameString` with length
    // longer than `kMaxSize` is correctly rejected (returning error
    // `kErrorInvalidArgs`).
    // Additionally, no minimum length is verified in order to ensure
    // backwards compatibility with previous versions that allowed
    // a zero-length name.

    Error    error;
    NameData data(aNameString, kMaxSize + 1);

    VerifyOrExit(IsValidUtf8String(aNameString), error = kErrorInvalidArgs);

    error = Set(data);

exit:
    return error;
}

Error NetworkName::Set(const NameData &aNameData)
{
    Error    error  = kErrorNone;
    NameData data   = aNameData;
    uint8_t  newLen = static_cast<uint8_t>(StringLength(data.GetBuffer(), data.GetLength()));

    VerifyOrExit(newLen <= kMaxSize, error = kErrorInvalidArgs);

    data.SetLength(newLen);

    // Ensure the new name does not match the current one.
    if (data.MatchesBytesIn(m8) && m8[newLen] == '\0')
    {
        ExitNow(error = kErrorAlready);
    }

    data.CopyBytesTo(m8);
    m8[newLen] = '\0';

exit:
    return error;
}

bool NetworkName::operator==(const NetworkName &aOther) const { return GetAsData() == aOther.GetAsData(); }

NetworkNameManager::NetworkNameManager(Instance &aInstance)
    : InstanceLocator(aInstance)
{
    IgnoreError(SetNetworkName(NetworkName::kNetworkNameInit));

#if (OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2)
    IgnoreError(SetDomainName(NetworkName::kDomainNameInit));
#endif
}

Error NetworkNameManager::SetNetworkName(const char *aNameString)
{
    return SignalNetworkNameChange(mNetworkName.Set(aNameString));
}

Error NetworkNameManager::SetNetworkName(const NameData &aNameData)
{
    return SignalNetworkNameChange(mNetworkName.Set(aNameData));
}

Error NetworkNameManager::SignalNetworkNameChange(Error aError)
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
Error NetworkNameManager::SetDomainName(const char *aNameString)
{
    Error error = mDomainName.Set(aNameString);

    return (error == kErrorAlready) ? kErrorNone : error;
}

Error NetworkNameManager::SetDomainName(const NameData &aNameData)
{
    Error error = mDomainName.Set(aNameData);

    return (error == kErrorAlready) ? kErrorNone : error;
}

bool NetworkNameManager::IsDefaultDomainNameSet(void) const
{
    return StringMatch(mDomainName.GetAsCString(), NetworkName::kDomainNameInit);
}
#endif // (OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2)

} // namespace MeshCoP
} // namespace ot
