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

#include "thread/ext_network_diagnostic_types.hpp"
#include "common/log.hpp"
#include "openthread/ext_network_diagnostic.h"
#include "utils/static_counter.hpp"

namespace ot {

RegisterLogModule("ExtNetDiag");

namespace ExtNetworkDiagnostic {

#if OPENTHREAD_CONFIG_EXT_NETWORK_DIAGNOSTIC_SERVER_ENABLE || OPENTHREAD_CONFIG_EXT_NETWORK_DIAGNOSTIC_CLIENT_ENABLE

const char *Tlv::TypeToString(Type aType)
{
    static const char *const kTypeStrings[] = {
        "MacAddress",
        "Mode",
        "Timeout",
        "LastHeard",
        "ConnectionTime",
        "Csl",
        "Route64",
        "LinkMarginIn",

        "MacLinkErrorRatesOut",
        "",
        "",
        "",
        "",
        "MlEid",
        "Ip6AddressList",
        "AlocList",

        "ThreadSpecVersion",
        "ThreadStackVersion",
        "VendorName",
        "VendorModel",
        "VendorSwVersion",
        "VendorAppUrl",
        "Ip6LinkLocalAddressList",
        "Eui64",

        "MacCounters",
        "MacLinkErrorRatesIn",
        "MleCounters",
        "LinkMarginOut",
    };

    struct TypeValueChecker
    {
        InitEnumValidatorCounter();

        ValidateNextEnum(kMacAddress);
        ValidateNextEnum(kMode);
        ValidateNextEnum(kTimeout);
        ValidateNextEnum(kLastHeard);
        ValidateNextEnum(kConnectionTime);
        ValidateNextEnum(kCsl);
        ValidateNextEnum(kRoute64);
        ValidateNextEnum(kLinkMarginIn);

        ValidateNextEnum(kMacLinkErrorRatesOut);
        SkipNextEnum();
        SkipNextEnum();
        SkipNextEnum();
        SkipNextEnum();
        ValidateNextEnum(kMlEid);
        ValidateNextEnum(kIp6AddressList);
        ValidateNextEnum(kAlocList);

        ValidateNextEnum(kThreadSpecVersion);
        ValidateNextEnum(kThreadStackVersion);
        ValidateNextEnum(kVendorName);
        ValidateNextEnum(kVendorModel);
        ValidateNextEnum(kVendorSwVersion);
        ValidateNextEnum(kVendorAppUrl);
        ValidateNextEnum(kIp6LinkLocalAddressList);
        ValidateNextEnum(kEui64);

        ValidateNextEnum(kMacCounters);
        ValidateNextEnum(kMacLinkErrorRatesIn);
        ValidateNextEnum(kMleCounters);
        ValidateNextEnum(kLinkMarginOut);
    };

    return kTypeStrings[aType];
}

const char *Tlv::TypeValueToString(uint8_t aType)
{
    const char *string = "Unknown";

    if (IsKnownTlv(aType))
    {
        string = TypeToString(static_cast<Type>(aType));
    }

    return string;
}

bool Tlv::IsKnownTlv(uint8_t aType)
{
    bool known = false;
    VerifyOrExit(aType <= kDataTlvMax);

    known = (kAllSupportedTlvMask.m8[aType / 8U] & (1U << (aType % 8U))) != 0;

exit:
    return known;
}

constexpr otExtNetworkDiagnosticTlvSet Tlv::kAllSupportedTlvMask;

constexpr otExtNetworkDiagnosticTlvSet TlvSet::kHostSupportedTlvMask;
constexpr otExtNetworkDiagnosticTlvSet TlvSet::kChildSupportedTlvMask;
constexpr otExtNetworkDiagnosticTlvSet TlvSet::kMtdChildProvidedTlvMask;
constexpr otExtNetworkDiagnosticTlvSet TlvSet::kFtdChildProvidedTlvMask;
constexpr otExtNetworkDiagnosticTlvSet TlvSet::kNeighborSupportedTlvMask;

TlvSet::Iterator::Iterator(otExtNetworkDiagnosticTlvSet aState)
    : mCurrent{0}
    , mState{aState}
{
    if ((aState.m8[0] & 1) == 0)
    {
        Advance();
    }
}

void TlvSet::Iterator::Advance(void)
{
    uint8_t idx = mCurrent / 8U;

    OT_ASSERT(mCurrent <= Tlv::kDataTlvMax);

    // When iterating over a byte we shift the state byte. This way the
    // current bit is always the first bit of the byte.
    mState.m8[idx] &= ~1U;

    // Skip all empty bytes
    while (mState.m8[idx] == 0)
    {
        // Cant overflow here since kDataTlvMax is guaranteed to be less than 248
        mCurrent = (mCurrent + 8U) & ~7U;

        if (mCurrent > Tlv::kDataTlvMax)
        {
            ExitNow(mCurrent = 0xFF);
        }

        idx = mCurrent / 8U;
    }

    while ((mState.m8[idx] & 1U) == 0)
    {
        mCurrent++;
        mState.m8[idx] = mState.m8[idx] >> 1;

        // No need to check for end of set here. If we were to go past the end
        // of the struct the empty bytes check wouldve caught it.
    }

exit:
    return;
}

void TlvSet::SetValue(uint8_t aValue)
{
    if (Tlv::IsKnownTlv(aValue))
    {
        Set(static_cast<Tlv::Type>(aValue));
    }
}

Error TlvSet::AppendTo(Message &aMessage, uint8_t &aSetCount) const
{
    Error      error = kErrorNone;
    RequestSet set;
    uint8_t    setCount = 0;

    uint8_t valOffset = 0;
    uint8_t valLength;

    while (valOffset < OT_EXT_NETWORK_DIAGNOSTIC_TLV_SET_SIZE)
    {
        if (m8[valOffset] == 0)
        {
            valOffset++;
            continue;
        }

        valLength = 1;
        while (m8[valOffset + valLength] != 0)
        {
            if (valOffset + valLength >= OT_EXT_NETWORK_DIAGNOSTIC_TLV_SET_SIZE)
            {
                break;
            }

            valLength++;
        }

        set.Clear();
        set.SetOffset(valOffset);
        set.SetLength(valLength);

        SuccessOrExit(error = aMessage.Append(set));
        valOffset += valLength;

        for (uint8_t idx = 1; idx <= valLength; idx++)
        {
            SuccessOrExit(error = aMessage.Append(m8[valOffset - idx]));
        }

        setCount++;
    }

    aSetCount = setCount;

exit:
    return error;
}

Error TlvSet::ReadFrom(const Message &aMessage, uint16_t &aOffset, uint8_t aSetCount)
{
    Error      error = kErrorNone;
    RequestSet set;
    uint16_t   offset = aOffset;

    uint8_t valOffset;
    uint8_t valLength;
    uint8_t dstOffset;
    uint8_t data;

    Clear();

    for (uint8_t setIndex = 0; setIndex < aSetCount; setIndex++)
    {
        SuccessOrExit(error = aMessage.Read(offset, set));
        offset += sizeof(RequestSet);

        valOffset = set.GetOffset();
        valLength = set.GetLength();

        for (uint8_t idx = 0; idx < valLength; idx++)
        {
            SuccessOrExit(error = aMessage.Read(offset + idx, data));

            dstOffset = (valOffset + valLength) - idx - 1;
            if (dstOffset < OT_EXT_NETWORK_DIAGNOSTIC_TLV_SET_SIZE)
            {
                m8[dstOffset] = data;
            }
        }

        offset += valLength;
    }

    aOffset = offset;

exit:
    FilterAllSupportedTlv();
    return error;
}

void RequestHeader::SetQuery(bool aQuery)
{
    if (aQuery)
    {
        mHeader |= kQueryFlag;
    }
    else
    {
        mHeader &= ~kQueryFlag;
    }
}

void RequestHeader::SetRegistration(bool aRegistration)
{
    if (aRegistration)
    {
        mHeader |= kRegistrationFlag;
    }
    else
    {
        mHeader &= ~kRegistrationFlag;
    }
}

void UpdateHeader::Init(void)
{
    mSeqNumber = 0;
    mMeta      = 0;
}

void UpdateHeader::SetComplete(bool aComplete)
{
    if (aComplete)
    {
        mMeta |= kCompleteFlag;
    }
    else
    {
        mMeta &= ~kCompleteFlag;
    }
}

void UpdateHeader::SetFullSeqNumber(uint64_t aSeqNumber)
{
    mSeqNumber = aSeqNumber;
    mMeta |= kFullSeqFlag;
}

void UpdateHeader::SetShortSeqNumber(uint64_t aSeqNumber)
{
    mSeqNumber = aSeqNumber;
    mMeta &= ~kFullSeqFlag;
}

uint16_t UpdateHeader::GetLength(void) const
{
    uint16_t length;

    if (HasFullSeqNumber())
    {
        length = sizeof(uint8_t) + sizeof(uint64_t);
    }
    else
    {
        length = sizeof(uint8_t) + sizeof(uint8_t);
    }

    return length;
}

Error UpdateHeader::ReadFrom(const Message &aMessage, uint16_t aOffset)
{
    Error error = kErrorNone;

    SuccessOrExit(error = aMessage.Read(aOffset, mMeta));

    if (HasFullSeqNumber())
    {
        SuccessOrExit(error = aMessage.Read(aOffset + 1, mSeqNumber));
        mSeqNumber = BigEndian::HostSwap64(mSeqNumber);
    }
    else
    {
        uint8_t number = 0;
        SuccessOrExit(error = aMessage.Read(aOffset + 1, number));
        mSeqNumber = number;
    }

exit:
    return error;
}

void UpdateHeader::WriteTo(Message &aMessage, uint16_t aOffset) const
{
    aMessage.Write(aOffset, mMeta);

    if (HasFullSeqNumber())
    {
        uint64_t number = BigEndian::HostSwap64(mSeqNumber);
        aMessage.Write(aOffset + 1, number);
    }
    else
    {
        uint8_t number = static_cast<uint8_t>(mSeqNumber);
        aMessage.Write(aOffset + 1, number);
    }
}

Error UpdateHeader::AppendTo(Message &aMessage) const
{
    Error error = kErrorNone;

    SuccessOrExit(error = aMessage.Append(mMeta));

    if (HasFullSeqNumber())
    {
        uint64_t number = BigEndian::HostSwap64(mSeqNumber);
        SuccessOrExit(aMessage.Append(number));
    }
    else
    {
        uint8_t number = static_cast<uint8_t>(mSeqNumber);
        SuccessOrExit(error = aMessage.Append(number));
    }

exit:
    return error;
}

void ChildRequestHeader::SetQuery(bool aQuery)
{
    if (aQuery)
    {
        mHeader |= kQueryFlag;
    }
    else
    {
        mHeader &= ~kQueryFlag;
    }
}

const char *DeviceTypeToString(DeviceType aType)
{
    const char *str = "unknown";

    switch (aType)
    {
    case kTypeHost:
        str = "host";
        break;

    case kTypeChild:
        str = "child";
        break;

    case kTypeNeighbor:
        str = "neighbor";
        break;

    default:
        break;
    }

    return str;
}

const char *UpdateModeToString(UpdateMode aMode)
{
    const char *str = "unknown";

    switch (aMode)
    {
    case kModeUpdate:
        str = "update";
        break;

    case kModeRemove:
        str = "remove";
        break;

    case kModeAdded:
        str = "added";
        break;

    default:
        break;
    }

    return str;
}

uint8_t UpdateModeToApiValue(UpdateMode aMode)
{
    uint8_t value;

    switch (aMode)
    {
    case kModeUpdate:
        value = OT_EXT_NETWORK_DIAGNOSTIC_UPDATE_MODE_UPDATE;
        break;

    case kModeRemove:
        value = OT_EXT_NETWORK_DIAGNOSTIC_UPDATE_MODE_REMOVED;
        break;

    case kModeAdded:
        value = OT_EXT_NETWORK_DIAGNOSTIC_UPDATE_MODE_ADDED;
        break;

    default:
        OT_ASSERT(false);
    }

    return value;
}

#endif // OPENTHREAD_CONFIG_EXT_NETWORK_DIAGNOSTIC_SERVER_ENABLE ||
       // OPENTHREAD_CONFIG_EXT_NETWORK_DIAGNOSTIC_CLIENT_ENABLE

} // namespace ExtNetworkDiagnostic
} // namespace ot
