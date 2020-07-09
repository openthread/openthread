/*
 *  Copyright (c) 2016-2019, The OpenThread Authors.
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
 *   This file implements MAC types such as Address, Extended PAN Identifier, Network Name, etc.
 */

#include "mac_types.hpp"

#include <stdio.h>

#include "common/code_utils.hpp"
#include "common/random.hpp"
#include "common/string.hpp"

namespace ot {
namespace Mac {

PanId GenerateRandomPanId(void)
{
    PanId panId;

    do
    {
        panId = Random::NonCrypto::GetUint16();
    } while (panId == kPanIdBroadcast);

    return panId;
}

#if !OPENTHREAD_RADIO
void ExtAddress::GenerateRandom(void)
{
    IgnoreError(Random::Crypto::FillBuffer(m8, sizeof(ExtAddress)));
    SetGroup(false);
    SetLocal(true);
}
#endif

ExtAddress::InfoString ExtAddress::ToString(void) const
{
    InfoString string;

    IgnoreError(string.AppendHexBytes(m8, sizeof(ExtAddress)));

    return string;
}

void ExtAddress::CopyAddress(uint8_t *aDst, const uint8_t *aSrc, CopyByteOrder aByteOrder)
{
    switch (aByteOrder)
    {
    case kNormalByteOrder:
        memcpy(aDst, aSrc, sizeof(ExtAddress));
        break;

    case kReverseByteOrder:
        aSrc += sizeof(ExtAddress) - 1;
        for (uint8_t len = sizeof(ExtAddress); len > 0; len--)
        {
            *aDst++ = *aSrc--;
        }
        break;
    }
}

Address::InfoString Address::ToString(void) const
{
    return (mType == kTypeExtended) ? GetExtended().ToString()
                                    : (mType == kTypeNone ? InfoString("None") : InfoString("0x%04x", GetShort()));
}

ExtendedPanId::InfoString ExtendedPanId::ToString(void) const
{
    InfoString string;

    IgnoreError(string.AppendHexBytes(m8, sizeof(ExtendedPanId)));

    return string;
}

uint8_t NameData::CopyTo(char *aBuffer, uint8_t aMaxSize) const
{
    uint8_t len = GetLength();

    memset(aBuffer, 0, aMaxSize);

    if (len > aMaxSize)
    {
        len = aMaxSize;
    }

    memcpy(aBuffer, GetBuffer(), len);

    return len;
}

NameData NetworkName::GetAsData(void) const
{
    uint8_t len = static_cast<uint8_t>(StringLength(m8, kMaxSize + 1));

    return NameData(m8, len);
}

otError NetworkName::Set(const NameData &aNameData)
{
    otError error  = OT_ERROR_NONE;
    uint8_t newLen = static_cast<uint8_t>(StringLength(aNameData.GetBuffer(), aNameData.GetLength()));

    VerifyOrExit(newLen <= kMaxSize, error = OT_ERROR_INVALID_ARGS);

    // Ensure the new name does not match the current one.
    VerifyOrExit(memcmp(m8, aNameData.GetBuffer(), newLen) || (m8[newLen] != '\0'), error = OT_ERROR_ALREADY);

    memcpy(m8, aNameData.GetBuffer(), newLen);
    m8[newLen] = '\0';

exit:
    return error;
}

#if (OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2)
NameData DomainName::GetAsData(void) const
{
    uint8_t len = static_cast<uint8_t>(StringLength(m8, kMaxSize + 1));

    return NameData(m8, len);
}

otError DomainName::Set(const NameData &aNameData)
{
    otError error  = OT_ERROR_NONE;
    uint8_t newLen = static_cast<uint8_t>(StringLength(aNameData.GetBuffer(), aNameData.GetLength()));

    VerifyOrExit(newLen <= kMaxSize, error = OT_ERROR_INVALID_ARGS);

    // Ensure the new name does not match the current one.
    VerifyOrExit(memcmp(m8, aNameData.GetBuffer(), newLen) || (m8[newLen] != '\0'), error = OT_ERROR_ALREADY);

    memcpy(m8, aNameData.GetBuffer(), newLen);
    m8[newLen] = '\0';

exit:
    return error;
}
#endif // (OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2)

} // namespace Mac
} // namespace ot
