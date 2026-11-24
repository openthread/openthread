/*
 *  Copyright (c) 2026, The OpenThread Authors.
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

#include "thread/mesh_monitor_types.hpp"

#if OPENTHREAD_CONFIG_MESH_MONITOR_SERVER_ENABLE || OPENTHREAD_CONFIG_MESH_MONITOR_CLIENT_ENABLE

#include "utils/static_counter.hpp"

namespace ot {
namespace MeshMonitor {

const char *Tlv::TypeToString(Type aType)
{
    static const char *const kTypeStrings[] = {
        "ExtAddress",
        "Mode",
        "Timeout",
        "LastHeard",
        "ConnectionTime",
        "Csl",
        "Route64",
        "LinkMarginIn",
        "MacLinkErrorRatesTx",
        "MlEid",
        "Ip6AddressList",
        "AlocList",
        "ThreadVersion",
        "ThreadStackVersion",
        "VendorName",
        "VendorModel",
        "VendorSwVersion",
        "VendorAppUrl",
        "Ip6LinkLocalAddressList",
        "Eui64",
        "MacCounters",
        "MacLinkErrorRatesRx",
        "MleCounters",
        "LinkMarginOut",
    };

    struct TypeValueChecker
    {
        InitEnumValidatorCounter();
        ValidateNextEnum(kExtAddress);
        ValidateNextEnum(kMode);
        ValidateNextEnum(kTimeout);
        ValidateNextEnum(kLastHeard);
        ValidateNextEnum(kConnectionTime);
        ValidateNextEnum(kCsl);
        ValidateNextEnum(kRoute64);
        ValidateNextEnum(kLinkMarginIn);
        ValidateNextEnum(kMacLinkErrorRatesTx);
        ValidateNextEnum(kMlEid);
        ValidateNextEnum(kIp6AddressList);
        ValidateNextEnum(kAlocList);
        ValidateNextEnum(kThreadVersion);
        ValidateNextEnum(kThreadStackVersion);
        ValidateNextEnum(kVendorName);
        ValidateNextEnum(kVendorModel);
        ValidateNextEnum(kVendorSwVersion);
        ValidateNextEnum(kVendorAppUrl);
        ValidateNextEnum(kIp6LinkLocalAddressList);
        ValidateNextEnum(kEui64);
        ValidateNextEnum(kMacCounters);
        ValidateNextEnum(kMacLinkErrorRatesRx);
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
    bool known;

    VerifyOrExit(aType <= kDataTlvMax, known = false);

    known = TlvSet::kAllSupportedTlvMask.IsSet(static_cast<Tlv::Type>(aType));

exit:
    return known;
}

constexpr TlvSet TlvSet::kAllSupportedTlvMask(Tlv::kExtAddress,
                                              Tlv::kMode,
                                              Tlv::kTimeout,
                                              Tlv::kLastHeard,
                                              Tlv::kConnectionTime,
                                              Tlv::kCsl,
                                              Tlv::kRoute64,
                                              Tlv::kLinkMarginIn,
                                              Tlv::kMacLinkErrorRatesTx,
                                              Tlv::kMlEid,
                                              Tlv::kIp6AddressList,
                                              Tlv::kAlocList,
                                              Tlv::kThreadVersion,
                                              Tlv::kThreadStackVersion,
                                              Tlv::kVendorName,
                                              Tlv::kVendorModel,
                                              Tlv::kVendorSwVersion,
                                              Tlv::kVendorAppUrl,
                                              Tlv::kIp6LinkLocalAddressList,
                                              Tlv::kEui64,
                                              Tlv::kMacCounters,
                                              Tlv::kMacLinkErrorRatesRx,
                                              Tlv::kMleCounters,
                                              Tlv::kLinkMarginOut);

constexpr TlvSet TlvSet::kHostSupportedTlvMask(Tlv::kExtAddress,
                                               Tlv::kMode,
                                               Tlv::kRoute64,
                                               Tlv::kMlEid,
                                               Tlv::kIp6AddressList,
                                               Tlv::kAlocList,
                                               Tlv::kThreadVersion,
                                               Tlv::kThreadStackVersion,
                                               Tlv::kVendorName,
                                               Tlv::kVendorModel,
                                               Tlv::kVendorSwVersion,
                                               Tlv::kVendorAppUrl,
                                               Tlv::kIp6LinkLocalAddressList,
                                               Tlv::kEui64,
                                               Tlv::kMacCounters,
                                               Tlv::kMleCounters);

constexpr TlvSet TlvSet::kChildSupportedTlvMask(Tlv::kExtAddress,
                                                Tlv::kMode,
                                                Tlv::kTimeout,
                                                Tlv::kLastHeard,
                                                Tlv::kConnectionTime,
                                                Tlv::kCsl,
                                                Tlv::kLinkMarginIn,
                                                Tlv::kMacLinkErrorRatesTx,
                                                Tlv::kMlEid,
                                                Tlv::kIp6AddressList,
                                                Tlv::kAlocList,
                                                Tlv::kThreadVersion,
                                                Tlv::kThreadStackVersion,
                                                Tlv::kVendorName,
                                                Tlv::kVendorModel,
                                                Tlv::kVendorSwVersion,
                                                Tlv::kVendorAppUrl,
                                                Tlv::kIp6LinkLocalAddressList,
                                                Tlv::kEui64,
                                                Tlv::kMacCounters,
                                                Tlv::kMacLinkErrorRatesRx,
                                                Tlv::kMleCounters,
                                                Tlv::kLinkMarginOut);

constexpr TlvSet TlvSet::kNeighborSupportedTlvMask(Tlv::kExtAddress,
                                                   Tlv::kLastHeard,
                                                   Tlv::kConnectionTime,
                                                   Tlv::kLinkMarginIn,
                                                   Tlv::kMacLinkErrorRatesTx,
                                                   Tlv::kThreadVersion);

constexpr TlvSet TlvSet::kMtdChildProvidedTlvMask(Tlv::kThreadStackVersion,
                                                  Tlv::kVendorName,
                                                  Tlv::kVendorModel,
                                                  Tlv::kVendorSwVersion,
                                                  Tlv::kVendorAppUrl,
                                                  Tlv::kIp6LinkLocalAddressList,
                                                  Tlv::kEui64,
                                                  Tlv::kMacCounters,
                                                  Tlv::kMacLinkErrorRatesRx,
                                                  Tlv::kMleCounters,
                                                  Tlv::kLinkMarginOut);

constexpr TlvSet TlvSet::kFtdChildProvidedTlvMask(Tlv::kMlEid,
                                                  Tlv::kIp6AddressList,
                                                  Tlv::kAlocList,
                                                  Tlv::kThreadStackVersion,
                                                  Tlv::kVendorName,
                                                  Tlv::kVendorModel,
                                                  Tlv::kVendorSwVersion,
                                                  Tlv::kVendorAppUrl,
                                                  Tlv::kIp6LinkLocalAddressList,
                                                  Tlv::kEui64,
                                                  Tlv::kMacCounters,
                                                  Tlv::kMacLinkErrorRatesRx,
                                                  Tlv::kMleCounters,
                                                  Tlv::kLinkMarginOut);

void TlvSet::SetValue(uint8_t aValue)
{
    if (Tlv::IsKnownTlv(aValue))
    {
        Set(static_cast<Tlv::Type>(aValue));
    }
}

Error TlvSet::AppendTo(Message &aMessage, uint8_t &aSetCount) const
{
    Error          error = kErrorNone;
    RequestSet     set;
    uint8_t        setCount  = 0;
    const uint8_t *mask      = GetMaskBytes();
    uint8_t        valOffset = 0;
    uint8_t        valLength;

    while (valOffset < kMaskSize)
    {
        if (mask[valOffset] == 0)
        {
            valOffset++;
            continue;
        }

        valLength = 1;
        while ((valOffset + valLength < kMaskSize) && (mask[valOffset + valLength] != 0))
        {
            valLength++;
        }

        set.Clear();
        set.SetOffset(valOffset);
        set.SetLength(valLength);

        SuccessOrExit(error = aMessage.Append(set));
        SuccessOrExit(error = aMessage.AppendBytes(&mask[valOffset], valLength));
        valOffset += valLength;

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
    uint16_t   offset              = aOffset;
    uint8_t    tempMask[kMaskSize] = {0};
    uint8_t    valOffset;
    uint8_t    valLength;
    uint8_t    dstOffset;
    uint8_t    data;

    for (uint8_t setIndex = 0; setIndex < aSetCount; setIndex++)
    {
        SuccessOrExit(error = aMessage.Read(offset, set));
        offset += sizeof(RequestSet);

        valOffset = set.GetOffset();
        valLength = set.GetLength();

        for (uint8_t idx = 0; idx < valLength; idx++)
        {
            SuccessOrExit(error = aMessage.Read(offset + idx, data));

            dstOffset = valOffset + idx;
            if (dstOffset < kMaskSize)
            {
                tempMask[dstOffset] = data;
            }
        }

        offset += valLength;
    }

    memcpy(mFields, tempMask, kMaskSize);
    aOffset = offset;

    FilterAllSupportedTlv();

exit:
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

void UpdateHeader::SetFullSeqNumber(uint32_t aSeqNumber)
{
    mSeqNumber = aSeqNumber;
    mMeta |= kFullSeqFlag;
}

void UpdateHeader::SetShortSeqNumber(uint32_t aSeqNumber)
{
    mSeqNumber = aSeqNumber;
    mMeta &= ~kFullSeqFlag;
}

uint16_t UpdateHeader::GetLength(void) const
{
    uint16_t length;

    if (HasFullSeqNumber())
    {
        length = sizeof(uint8_t) + sizeof(uint32_t);
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
        mSeqNumber = BigEndian::HostSwap32(mSeqNumber);
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
        uint32_t number = BigEndian::HostSwap32(mSeqNumber);

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
        uint32_t number = BigEndian::HostSwap32(mSeqNumber);

        SuccessOrExit(error = aMessage.Append(number));
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
    case kModeUpdated:
        str = "updated";
        break;

    case kModeRemoved:
        str = "removed";
        break;

    case kModeAdded:
        str = "added";
        break;

    default:
        break;
    }

    return str;
}

Error UpdateModeToApiValue(UpdateMode aMode, otMeshMonUpdateMode &aApiMode)
{
    Error error = kErrorNone;

    switch (aMode)
    {
    case kModeUpdated:
        aApiMode = OT_MESH_MON_UPDATE_MODE_UPDATED;
        break;

    case kModeRemoved:
        aApiMode = OT_MESH_MON_UPDATE_MODE_REMOVED;
        break;

    case kModeAdded:
        aApiMode = OT_MESH_MON_UPDATE_MODE_ADDED;
        break;

    default:
        error = kErrorNotFound;
        break;
    }

    return error;
}

} // namespace MeshMonitor
} // namespace ot

#endif // OPENTHREAD_CONFIG_MESH_MONITOR_SERVER_ENABLE || OPENTHREAD_CONFIG_MESH_MONITOR_CLIENT_ENABLE
