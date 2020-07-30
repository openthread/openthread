/*
 *  Copyright (c) 2016, The OpenThread Authors.
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
 *   This file includes definitions for generating and processing Thread Network Data TLVs.
 */

#ifndef NETWORK_DATA_TLVS_HPP_
#define NETWORK_DATA_TLVS_HPP_

#include "openthread-core-config.h"

#include <openthread/netdata.h>

#include "common/debug.hpp"
#include "common/encoding.hpp"
#include "common/equatable.hpp"
#include "net/ip6_address.hpp"

namespace ot {
namespace NetworkData {

using ot::Encoding::BigEndian::HostSwap16;
using ot::Encoding::BigEndian::HostSwap32;

/**
 * @addtogroup core-netdata-tlvs
 *
 * @brief
 *   This module includes definitions for generating and processing Thread Network Data TLVs.
 *
 * @{
 *
 */

/**
 * This class implements Thread Network Data TLV generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class NetworkDataTlv
{
public:
    /**
     * This method initializes TLV.
     *
     */
    void Init(void)
    {
        mType   = 0;
        mLength = 0;
    }

    /**
     * Thread Network Data Type values.
     *
     */
    enum Type
    {
        kTypeHasRoute          = 0, ///< Has Route TLV
        kTypePrefix            = 1, ///< Prefix TLV
        kTypeBorderRouter      = 2, ///< Border Router TLV
        kTypeContext           = 3, ///< Context TLV
        kTypeCommissioningData = 4, ///< Commissioning Dataset TLV
        kTypeService           = 5, ///< Service TLV
        kTypeServer            = 6, ///< Server TLV
    };

    /**
     * This method returns the Type value.
     *
     * @returns The Type value.
     *
     */
    Type GetType(void) const { return static_cast<Type>(mType >> kTypeOffset); }

    /**
     * This method sets the Type value.
     *
     * @param[in]  aType  The Type value.
     *
     */
    void SetType(Type aType) { mType = (mType & ~kTypeMask) | ((aType << kTypeOffset) & kTypeMask); }

    /**
     * This method returns the Length value.
     *
     * @returns The Length value.
     *
     */
    uint8_t GetLength(void) const { return mLength; }

    /**
     * This method sets the Length value.
     *
     * @param[in]  aLength  The Length value.
     *
     */
    void SetLength(uint8_t aLength) { mLength = aLength; }

    /**
     * This methods increases the Length value by a given amount.
     *
     * @param[in]  aIncrement  The increment amount to increase the length.
     *
     */
    void IncreaseLength(uint8_t aIncrement) { mLength += aIncrement; }

    /**
     * This methods decreases the Length value by a given amount.
     *
     * @param[in]  aDecrement  The decrement amount to decrease the length.
     *
     */
    void DecreaseLength(uint8_t aDecrement) { mLength -= aDecrement; }

    /**
     * This method returns the TLV's total size (number of bytes) including Type, Length, and Value fields.
     *
     * @returns The total size include Type, Length, and Value fields.
     *
     */
    uint8_t GetSize(void) const { return sizeof(NetworkDataTlv) + mLength; }

    /**
     * This method returns a pointer to the Value.
     *
     * @returns A pointer to the value.
     *
     */
    uint8_t *GetValue(void) { return reinterpret_cast<uint8_t *>(this) + sizeof(NetworkDataTlv); }

    /**
     * This method returns a pointer to the Value.
     *
     * @returns A pointer to the value.
     *
     */
    const uint8_t *GetValue(void) const { return reinterpret_cast<const uint8_t *>(this) + sizeof(NetworkDataTlv); }

    /**
     * This method returns a pointer to the next Network Data TLV.
     *
     * @returns A pointer to the next Network Data TLV.
     *
     */
    NetworkDataTlv *GetNext(void)
    {
        return reinterpret_cast<NetworkDataTlv *>(reinterpret_cast<uint8_t *>(this) + sizeof(*this) + mLength);
    }

    /**
     * This method returns a pointer to the next Network Data TLV.
     *
     * @returns A pointer to the next Network Data TLV.
     *
     */
    const NetworkDataTlv *GetNext(void) const
    {
        return reinterpret_cast<const NetworkDataTlv *>(reinterpret_cast<const uint8_t *>(this) + sizeof(*this) +
                                                        mLength);
    }

    /**
     * This method clears the Stable bit.
     *
     */
    void ClearStable(void) { mType &= ~kStableMask; }

    /**
     * This method indicates whether or not the Stable bit is set.
     *
     * @retval TRUE   If the Stable bit is set.
     * @retval FALSE  If the Stable bit is not set.
     *
     */
    bool IsStable(void) const { return (mType & kStableMask); }

    /**
     * This method sets the Stable bit.
     *
     */
    void SetStable(void) { mType |= kStableMask; }

private:
    enum
    {
        kTypeOffset = 1,
        kTypeMask   = 0x7f << kTypeOffset,
        kStableMask = 1 << 0,
    };
    uint8_t mType;
    uint8_t mLength;
} OT_TOOL_PACKED_END;

/**
 * This class implements Has Route TLV entry generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class HasRouteEntry : public Equatable<HasRouteEntry>
{
public:
    /**
     * This method initializes the header.
     *
     */
    void Init(void)
    {
        SetRloc(Mac::kShortAddrInvalid);
        mFlags = 0;
    }

    /**
     * This method returns the RLOC16 value.
     *
     * @returns The RLOC16 value.
     */
    uint16_t GetRloc(void) const { return HostSwap16(mRloc); }

    /**
     * This method sets the RLOC16 value.
     *
     * @param[in]  aRloc16  The RLOC16 value.
     *
     */
    void SetRloc(uint16_t aRloc16) { mRloc = HostSwap16(aRloc16); }

    /**
     * This method returns the Preference value.
     *
     * @returns The preference value.
     *
     */
    int8_t GetPreference(void) const { return static_cast<int8_t>(mFlags) >> kPreferenceOffset; }

    /**
     * This method sets the Preference value.
     *
     * @param[in]  aPrf  The Preference value.
     *
     */
    void SetPreference(int8_t aPrf)
    {
        OT_ASSERT((aPrf == OT_ROUTE_PREFERENCE_LOW) || (aPrf == OT_ROUTE_PREFERENCE_MED) ||
                  (aPrf == OT_ROUTE_PREFERENCE_HIGH));
        mFlags = (mFlags & ~kPreferenceMask) | ((static_cast<uint8_t>(aPrf) << kPreferenceOffset) & kPreferenceMask);
    }

    /**
     * This method returns a pointer to the next HasRouteEntry.
     *
     * @returns A pointer to the next HasRouteEntry.
     *
     */
    HasRouteEntry *GetNext(void) { return (this + 1); }

    /**
     * This method returns a pointer to the next HasRouteEntry.
     *
     * @returns A pointer to the next HasRouteEntry.
     *
     */
    const HasRouteEntry *GetNext(void) const { return (this + 1); }

private:
    enum
    {
        kPreferenceOffset = 6,
        kPreferenceMask   = 3 << kPreferenceOffset,
    };

    uint16_t mRloc;
    uint8_t  mFlags;
} OT_TOOL_PACKED_END;

/**
 * This class implements Has Route TLV generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class HasRouteTlv : public NetworkDataTlv
{
public:
    enum
    {
        kType = kTypeHasRoute, ///< The TLV Type.
    };

    /**
     * This method initializes the TLV.
     *
     */
    void Init(void)
    {
        NetworkDataTlv::Init();
        SetType(kTypeHasRoute);
        SetLength(0);
    }

    /**
     * This method returns the number of HasRoute entries.
     *
     * @returns The number of HasRoute entries.
     *
     */
    uint8_t GetNumEntries(void) const { return GetLength() / sizeof(HasRouteEntry); }

    /**
     * This method returns a pointer to the HasRoute entry at a given index.
     *
     * @param[in]  aIndex  An index.
     *
     * @returns A pointer to the HasRoute entry at @p aIndex.
     *
     */
    HasRouteEntry *GetEntry(uint8_t aIndex)
    {
        return reinterpret_cast<HasRouteEntry *>(GetValue() + (aIndex * sizeof(HasRouteEntry)));
    }

    /**
     * This method returns a pointer to the HasRoute entry at a given index.
     *
     * @param[in]  aIndex  An index.
     *
     * @returns A pointer to the HasRoute entry at @p aIndex.
     *
     */
    const HasRouteEntry *GetEntry(uint8_t aIndex) const
    {
        return reinterpret_cast<const HasRouteEntry *>(GetValue() + (aIndex * sizeof(HasRouteEntry)));
    }

    /**
     * This method returns a pointer to the first HasRouteEntry (at index 0'th).
     *
     * @returns A pointer to the first HasRouteEntry.
     *
     */
    HasRouteEntry *GetFirstEntry(void) { return reinterpret_cast<HasRouteEntry *>(GetValue()); }

    /**
     * This method returns a pointer to the first HasRouteEntry (at index 0'th).
     *
     * @returns A pointer to the first HasRouteEntry.
     *
     */
    const HasRouteEntry *GetFirstEntry(void) const { return reinterpret_cast<const HasRouteEntry *>(GetValue()); }

    /**
     * This method returns a pointer to the last HasRouteEntry.
     *
     * If there are no entries the pointer will be invalid but guaranteed to be before the `GetFirstEntry()` pointer.
     *
     * @returns A pointer to the last HasRouteEntry.
     *
     */
    HasRouteEntry *GetLastEntry(void)
    {
        return reinterpret_cast<HasRouteEntry *>(GetValue() + GetLength() - sizeof(HasRouteEntry));
    }

    /**
     * This method returns a pointer to the last HasRouteEntry.
     *
     * If there are no entries the pointer will be invalid but guaranteed to be before the `GetFirstEntry()` pointer.
     *
     * @returns A pointer to the last HasRouteEntry.
     *
     */
    const HasRouteEntry *GetLastEntry(void) const
    {
        return reinterpret_cast<const HasRouteEntry *>(GetValue() + GetLength() - sizeof(HasRouteEntry));
    }

} OT_TOOL_PACKED_END;

/**
 * This class implements Prefix TLV generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class PrefixTlv : public NetworkDataTlv
{
public:
    enum
    {
        kType = kTypePrefix, ///< The TLV Type.
    };

    /**
     * This method initializes the TLV.
     *
     * @param[in]  aDomainId      The Domain ID.
     * @param[in]  aPrefixLength  The Prefix Length in bits.
     * @param[in]  aPrefix        A pointer to the prefix.
     *
     */
    void Init(uint8_t aDomainId, uint8_t aPrefixLength, const uint8_t *aPrefix)
    {
        NetworkDataTlv::Init();
        SetType(kTypePrefix);
        mDomainId     = aDomainId;
        mPrefixLength = aPrefixLength;
        memcpy(GetPrefix(), aPrefix, Ip6::Prefix::SizeForLength(aPrefixLength));
        SetSubTlvsLength(0);
    }

    /**
     * This method initializes the TLV.
     *
     * @param[in]  aDomainId      The Domain ID.
     * @param[in]  aPrefix        The Prefix.
     *
     */
    void Init(uint8_t aDomainId, const Ip6::Prefix aPrefix)
    {
        Init(aDomainId, aPrefix.GetLength(), aPrefix.GetBytes());
    }

    /**
     * This method indicates whether or not the TLV appears to be well-formed.
     *
     * @retval TRUE   If the TLV appears to be well-formed.
     * @retval FALSE  If the TLV does not appear to be well-formed.
     *
     */
    bool IsValid(void) const
    {
        return ((GetLength() >= sizeof(*this) - sizeof(NetworkDataTlv)) &&
                (GetLength() >= Ip6::Prefix::SizeForLength(mPrefixLength) + sizeof(*this) - sizeof(NetworkDataTlv)) &&
                (Ip6::Prefix::SizeForLength(mPrefixLength) <= sizeof(Ip6::Address)));
    }

    /**
     * This method returns the Domain ID value.
     *
     * @returns The Domain ID value.
     *
     */
    uint8_t GetDomainId(void) const { return mDomainId; }

    /**
     * This method returns the Prefix Length value.
     *
     * @returns The Prefix Length value (in bits).
     *
     */
    uint8_t GetPrefixLength(void) const { return mPrefixLength; }

    /**
     * This method returns a pointer to the Prefix.
     *
     * @returns A pointer to the Prefix.
     *
     */
    uint8_t *GetPrefix(void) { return reinterpret_cast<uint8_t *>(this) + sizeof(*this); }

    /**
     * This method returns a pointer to the Prefix.
     *
     * @returns A pointer to the Prefix.
     *
     */
    const uint8_t *GetPrefix(void) const { return reinterpret_cast<const uint8_t *>(this) + sizeof(*this); }

    /**
     * This method copies the Prefix from TLV into a given `Ip6::Prefix`.
     *
     * @param[out] aPrefix  An `Ip6::Prefix` to copy the Prefix from TLV into.
     *
     */
    void CopyPrefixTo(Ip6::Prefix &aPrefix) const { aPrefix.Set(GetPrefix(), GetPrefixLength()); }

    /**
     * This method indicates whether the Prefix from TLV is equal to a given `Ip6::Prefix`.
     *
     * @param[in] aPrefix  A Prefix to compare with.
     *
     * @retval TRUE   The TLV's Prefix is equal to @p aPrefix.
     * @retval FALSE  The TLV's Prefix is not equal to @p aPrefix.
     *
     */
    bool IsEqual(Ip6::Prefix &aPrefix) const { return aPrefix.IsEqual(GetPrefix(), GetPrefixLength()); }

    /**
     * This method indicates whether the Prefix from TLV is equal to a given Prefix.
     *
     * @param[in]  aPrefix        A pointer to an IPv6 prefix to compare with.
     * @param[in]  aPrefixLength  The prefix length pointed to by @p aPrefix (in bits).
     *
     * @retval TRUE   The TLV's Prefix is equal to @p aPrefix.
     * @retval FALSE  The TLV's Prefix is not euqal @p aPrefix.
     *
     */
    bool IsEqual(const uint8_t *aPrefix, uint8_t aPrefixLength) const
    {
        return (aPrefixLength == mPrefixLength) &&
               (Ip6::Prefix::MatchLength(GetPrefix(), aPrefix, Ip6::Prefix::SizeForLength(aPrefixLength)) >=
                mPrefixLength);
    }

    /**
     * This method returns a pointer to the Sub-TLVs.
     *
     * @returns A pointer to the Sub-TLVs.
     *
     */
    NetworkDataTlv *GetSubTlvs(void)
    {
        return reinterpret_cast<NetworkDataTlv *>(GetPrefix() + Ip6::Prefix::SizeForLength(mPrefixLength));
    }

    /**
     * This method returns a pointer to the Sub-TLVs.
     *
     * @returns A pointer to the Sub-TLVs.
     *
     */
    const NetworkDataTlv *GetSubTlvs(void) const
    {
        return reinterpret_cast<const NetworkDataTlv *>(GetPrefix() + Ip6::Prefix::SizeForLength(mPrefixLength));
    }

    /**
     * This method returns the Sub-TLVs length in bytes.
     *
     * @returns The Sub-TLVs length in bytes.
     *
     */
    uint8_t GetSubTlvsLength(void) const
    {
        return GetLength() - (sizeof(*this) - sizeof(NetworkDataTlv) + Ip6::Prefix::SizeForLength(mPrefixLength));
    }

    /**
     * This method sets the Sub-TLVs length in bytes.
     *
     * @param[in]  aLength  The Sub-TLVs length in bytes.
     *
     */
    void SetSubTlvsLength(uint8_t aLength)
    {
        SetLength(sizeof(*this) - sizeof(NetworkDataTlv) + Ip6::Prefix::SizeForLength(mPrefixLength) + aLength);
    }

    /**
     * This static method calculates the total size (number of bytes) of a Prefix TLV with a given Prefix Length value.
     *
     * Note that the returned size does include the Type and Length fields in the TLV, but does not account for any
     * sub TLVs of the Prefix TLV.
     *
     * @param[in]  aPrefixLength     A Prefix Length in bits.

     * @returns    The size (number of bytes) of the Prefix TLV.
     *
     */
    static uint16_t CalculateSize(uint8_t aPrefixLength)
    {
        return sizeof(PrefixTlv) + Ip6::Prefix::SizeForLength(aPrefixLength);
    }

private:
    uint8_t mDomainId;
    uint8_t mPrefixLength;
} OT_TOOL_PACKED_END;

/**
 * This class implements Border Router Entry generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class BorderRouterEntry : public Equatable<BorderRouterEntry>
{
public:
    enum
    {
        kPreferenceOffset = 14,
        kPreferenceMask   = 3 << kPreferenceOffset,
        kPreferredFlag    = 1 << 13,
        kSlaacFlag        = 1 << 12,
        kDhcpFlag         = 1 << 11,
        kConfigureFlag    = 1 << 10,
        kDefaultRouteFlag = 1 << 9,
        kOnMeshFlag       = 1 << 8,
        kNdDnsFlag        = 1 << 7,
        kDpFlag           = 1 << 6,
    };

    /**
     * This method initializes the TLV.
     *
     */
    void Init(void)
    {
        SetRloc(Mac::kShortAddrInvalid);
        mFlags = 0;
    }

    /**
     * This method returns the RLOC16 value.
     *
     * @returns The RLOC16 value.
     */
    uint16_t GetRloc(void) const { return HostSwap16(mRloc); }

    /**
     * This method sets the RLOC16 value.
     *
     * @param[in]  aRloc16  The RLOC16 value.
     *
     */
    void SetRloc(uint16_t aRloc16) { mRloc = HostSwap16(aRloc16); }

    /**
     * This method returns the Flags value.
     *
     * @returns The Flags value.
     *
     */
    uint16_t GetFlags(void) const { return HostSwap16(mFlags) & ~kPreferenceMask; }

    /**
     * This method sets the Flags value.
     *
     * @param[in]  aFlags  The Flags value.
     *
     */
    void SetFlags(uint16_t aFlags)
    {
        mFlags = HostSwap16((HostSwap16(mFlags) & kPreferenceMask) | (aFlags & ~kPreferenceMask));
    }

    /**
     * This method returns the Preference value.
     *
     * @returns the Preference value.
     *
     */
    int8_t GetPreference(void) const
    {
        return static_cast<int8_t>(static_cast<int16_t>(HostSwap16(mFlags)) >> kPreferenceOffset);
    }

    /**
     * This method sets the Preference value.
     *
     * @param[in]  aPrf  The Preference value.
     *
     */
    void SetPreference(int8_t aPrf)
    {
        mFlags = HostSwap16(GetFlags() | ((static_cast<uint16_t>(aPrf) << kPreferenceOffset) & kPreferenceMask));
    }

    /**
     * This method indicates whether or not the Preferred flag is set.
     *
     * @retval TRUE   If the Preferred flag is set.
     * @retval FALSE  If the Preferred flag is not set.
     *
     */
    bool IsPreferred(void) const { return (HostSwap16(mFlags) & kPreferredFlag) != 0; }

    /**
     * This method indicates whether or not the SLAAC flag is set.
     *
     * @retval TRUE   If the SLAAC flag is set.
     * @retval FALSE  If the SLAAC flag is not set.
     *
     */
    bool IsSlaac(void) const { return (HostSwap16(mFlags) & kSlaacFlag) != 0; }

    /**
     * This method indicates whether or not the DHCP flag is set.
     *
     * @retval TRUE   If the DHCP flag is set.
     * @retval FALSE  If the DHCP flag is not set.
     *
     */
    bool IsDhcp(void) const { return (HostSwap16(mFlags) & kDhcpFlag) != 0; }

    /**
     * This method indicates whether or not the Configure flag is set.
     *
     * @retval TRUE   If the Configure flag is set.
     * @retval FALSE  If the Configure flag is not set.
     *
     */
    bool IsConfigure(void) const { return (HostSwap16(mFlags) & kConfigureFlag) != 0; }

    /**
     * This method indicates whether or not the Default Route flag is set.
     *
     * @retval TRUE   If the Default Route flag is set.
     * @retval FALSE  If the Default Route flag is not set.
     *
     */
    bool IsDefaultRoute(void) const { return (HostSwap16(mFlags) & kDefaultRouteFlag) != 0; }

    /**
     * This method indicates whether or not the On-Mesh flag is set.
     *
     * @retval TRUE   If the On-Mesh flag is set.
     * @retval FALSE  If the On-Mesh flag is not set.
     *
     */
    bool IsOnMesh(void) const { return (HostSwap16(mFlags) & kOnMeshFlag) != 0; }

    /**
     * This method indicates whether or not the Nd-Dns flag is set.
     *
     * @retval TRUE   If the Nd-Dns flag is set.
     * @retval FALSE  If the Nd-Dns flag is not set.
     *
     */
    bool IsNdDns(void) const { return (HostSwap16(mFlags) & kNdDnsFlag) != 0; }

    /**
     * This method indicates whether or not the Domain Prefix flag is set.
     *
     * @retval TRUE   If the Domain Prefix flag is set.
     * @retval FALSE  If the Domain Prefix flag is not set.
     *
     */
    bool IsDp(void) const { return (HostSwap16(mFlags) & kDpFlag) != 0; }

    /**
     * This method returns a pointer to the next BorderRouterEntry
     *
     * @returns A pointer to the next BorderRouterEntry.
     *
     */
    BorderRouterEntry *GetNext(void) { return (this + 1); }

    /**
     * This method returns a pointer to the next BorderRouterEntry
     *
     * @returns A pointer to the next BorderRouterEntry.
     *
     */
    const BorderRouterEntry *GetNext(void) const { return (this + 1); }

private:
    uint16_t mRloc;
    uint16_t mFlags;
} OT_TOOL_PACKED_END;

/**
 * This class implements Border Router TLV generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class BorderRouterTlv : public NetworkDataTlv
{
public:
    enum
    {
        kType = kTypeBorderRouter, ///< The TLV Type.
    };

    /**
     * This method initializes the TLV.
     *
     */
    void Init(void)
    {
        NetworkDataTlv::Init();
        SetType(kTypeBorderRouter);
        SetLength(0);
    }

    /**
     * This method returns the number of Border Router entries.
     *
     * @returns The number of Border Router entries.
     *
     */
    uint8_t GetNumEntries(void) const { return GetLength() / sizeof(BorderRouterEntry); }

    /**
     * This method returns a pointer to the Border Router entry at a given index
     *
     * @param[in]  aIndex  The index.
     *
     * @returns A pointer to the Border Router entry at @p aIndex.
     *
     */
    BorderRouterEntry *GetEntry(uint8_t aIndex)
    {
        return reinterpret_cast<BorderRouterEntry *>(GetValue() + (aIndex * sizeof(BorderRouterEntry)));
    }

    /**
     * This method returns a pointer to the Border Router entry at a given index.
     *
     * @param[in]  aIndex  The index.
     *
     * @returns A pointer to the Border Router entry at @p aIndex
     *
     */
    const BorderRouterEntry *GetEntry(uint8_t aIndex) const
    {
        return reinterpret_cast<const BorderRouterEntry *>(GetValue() + (aIndex * sizeof(BorderRouterEntry)));
    }

    /**
     * This method returns a pointer to the first BorderRouterEntry (at index 0'th).
     *
     * @returns A pointer to the first BorderRouterEntry.
     *
     */
    BorderRouterEntry *GetFirstEntry(void) { return reinterpret_cast<BorderRouterEntry *>(GetValue()); }

    /**
     * This method returns a pointer to the first BorderRouterEntry (at index 0'th).
     *
     * @returns A pointer to the first BorderRouterEntry.
     *
     */
    const BorderRouterEntry *GetFirstEntry(void) const
    {
        return reinterpret_cast<const BorderRouterEntry *>(GetValue());
    }

    /**
     * This method returns a pointer to the last BorderRouterEntry.
     *
     * If there are no entries the pointer will be invalid but guaranteed to be before the `GetFirstEntry()` pointer.
     *
     * @returns A pointer to the last BorderRouterEntry.
     *
     */
    BorderRouterEntry *GetLastEntry(void)
    {
        return reinterpret_cast<BorderRouterEntry *>(GetValue() + GetLength() - sizeof(BorderRouterEntry));
    }

    /**
     * This method returns a pointer to the last BorderRouterEntry.
     *
     * If there are no entries the pointer will be invalid but guaranteed to be before the `GetFirstEntry()` pointer.
     *
     * @returns A pointer to the last BorderRouterEntry.
     *
     */
    const BorderRouterEntry *GetLastEntry(void) const
    {
        return reinterpret_cast<const BorderRouterEntry *>(GetValue() + GetLength() - sizeof(BorderRouterEntry));
    }

} OT_TOOL_PACKED_END;

/**
 * This class implements Context TLV generation and processing.
 *
 */
OT_TOOL_PACKED_BEGIN
class ContextTlv : public NetworkDataTlv
{
public:
    enum
    {
        kType = kTypeContext, ///< The TLV Type.
    };

    /**
     * This method initializes the Context TLV.
     *
     * @param[in]  aConextId   The Context ID value.
     * @param[in]  aLength     The Context Length value.
     *
     */
    void Init(uint8_t aContextId, uint8_t aConextLength)
    {
        NetworkDataTlv::Init();
        SetType(kTypeContext);
        SetLength(sizeof(ContextTlv) - sizeof(NetworkDataTlv));
        mFlags         = ((aContextId << kContextIdOffset) & kContextIdMask);
        mContextLength = aConextLength;
    }

    /**
     * This method indicates whether or not the Compress flag is set.
     *
     * @retval TRUE   The Compress flag is set.
     * @retval FALSE  The Compress flags is not set.
     *
     */
    bool IsCompress(void) const { return (mFlags & kCompressFlag) != 0; }

    /**
     * This method clears the Compress flag.
     *
     */
    void ClearCompress(void) { mFlags &= ~kCompressFlag; }

    /**
     * This method sets the Compress flag.
     *
     */
    void SetCompress(void) { mFlags |= kCompressFlag; }

    /**
     * This method returns the Context ID value.
     *
     * @returns The Context ID value.
     *
     */
    uint8_t GetContextId(void) const { return mFlags & kContextIdMask; }

    /**
     * This method returns the Context Length value.
     *
     * @returns The Context Length value.
     *
     */
    uint8_t GetContextLength(void) const { return mContextLength; }

private:
    enum
    {
        kCompressFlag    = 1 << 4,
        kContextIdOffset = 0,
        kContextIdMask   = 0xf << kContextIdOffset,
    };
    uint8_t mFlags;
    uint8_t mContextLength;
} OT_TOOL_PACKED_END;

/**
 * This class implements Commissioning Data TLV generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class CommissioningDataTlv : public NetworkDataTlv
{
public:
    enum
    {
        kType = kTypeCommissioningData, ///< The TLV Type.
    };

    /**
     * This method initializes the TLV.
     *
     */
    void Init(void)
    {
        NetworkDataTlv::Init();
        SetType(kTypeCommissioningData);
        SetLength(0);
    }
} OT_TOOL_PACKED_END;

/**
 * This class implements Service Data TLV generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class ServiceTlv : public NetworkDataTlv
{
public:
    enum
    {
        kType                      = kTypeService, ///< The TLV Type.
        kThreadEnterpriseNumber    = 44970,        ///< Thread enterprise number.
        kServiceDataBackboneRouter = 0x01,         ///< const THREAD_SERVICE_DATA_BBR
    };

    /**
     * This method initializes the TLV.
     *
     * @param[in]  aServiceId          The Service Id value.
     * @param[in]  aEnterpriseNumber   The Enterprise Number.
     * @param[in]  aServiceData        The Service Data.
     * @param[in]  aServiceDataLength  The Service Data length (number of bytes).
     *
     */
    void Init(uint8_t aServiceId, uint32_t aEnterpriseNumber, const uint8_t *aServiceData, uint8_t aServiceDataLength)
    {
        NetworkDataTlv::Init();
        SetType(kTypeService);

        mFlagsServiceId = (aEnterpriseNumber == kThreadEnterpriseNumber) ? kThreadEnterpriseFlag : 0;
        mFlagsServiceId |= (aServiceId & kServiceIdMask);

        if (aEnterpriseNumber != kThreadEnterpriseNumber)
        {
            mShared.mEnterpriseNumber = HostSwap32(aEnterpriseNumber);
            mServiceDataLength        = aServiceDataLength;
            memcpy(&mServiceDataLength + sizeof(uint8_t), aServiceData, aServiceDataLength);
        }
        else
        {
            mShared.mServiceDataLengthThreadEnterprise = aServiceDataLength;
            memcpy(&mShared.mServiceDataLengthThreadEnterprise + sizeof(uint8_t), aServiceData, aServiceDataLength);
        }

        SetLength(GetFieldsLength());
    }

    /**
     * This method indicates whether or not the TLV appears to be well-formed.
     *
     * @retval TRUE   If the TLV appears to be well-formed.
     * @retval FALSE  If the TLV does not appear to be well-formed.
     *
     */
    bool IsValid(void) const
    {
        uint8_t length = GetLength();

        return (length >= sizeof(mFlagsServiceId)) &&
               (length >= kMinLength + (IsThreadEnterprise() ? 0 : sizeof(uint32_t))) &&
               (static_cast<uint16_t>(length) + sizeof(NetworkDataTlv) >=
                CalculateSize(GetEnterpriseNumber(), GetServiceDataLength()));
    }

    /**
     * This method returns the Service ID. It is in range 0x00-0x0f.
     *
     * @returns the Service ID.
     *
     */
    uint8_t GetServiceId(void) const { return (mFlagsServiceId & kServiceIdMask); }

    /**
     * This method returns Enterprise Number field.
     *
     * @returns The Enterprise Number.
     *
     */
    uint32_t GetEnterpriseNumber(void) const
    {
        return IsThreadEnterprise() ? static_cast<uint32_t>(kThreadEnterpriseNumber)
                                    : HostSwap32(mShared.mEnterpriseNumber);
    }

    /**
     * This method gets Service Data length.
     *
     * @returns length of the Service Data field in bytes.
     *
     */
    uint8_t GetServiceDataLength(void) const
    {
        return IsThreadEnterprise() ? mShared.mServiceDataLengthThreadEnterprise : mServiceDataLength;
    }

    /**
     * This method returns a pointer to the Service Data.
     *
     * @returns A pointer to the Service Data.
     *
     */
    uint8_t *GetServiceData(void)
    {
        return (IsThreadEnterprise() ? &mShared.mServiceDataLengthThreadEnterprise : &mServiceDataLength) +
               sizeof(uint8_t);
    }

    /**
     * This method returns a pointer to the Service Data.
     *
     * @returns A pointer to the Service Data.
     *
     */
    const uint8_t *GetServiceData(void) const
    {
        return (IsThreadEnterprise() ? &mShared.mServiceDataLengthThreadEnterprise : &mServiceDataLength) +
               sizeof(uint8_t);
    }

    /**
     * This method returns the Sub-TLVs length in bytes.
     *
     * @returns The Sub-TLVs length in bytes.
     *
     */
    uint8_t GetSubTlvsLength(void) { return GetLength() - GetFieldsLength(); }

    /**
     * This method sets the Sub-TLVs length in bytes.
     *
     * @param[in]  aLength  The Sub-TLVs length in bytes.
     *
     */
    void SetSubTlvsLength(uint8_t aLength) { SetLength(GetFieldsLength() + aLength); }

    /**
     * This method returns a pointer to the Sub-TLVs.
     *
     * @returns A pointer to the Sub-TLVs.
     *
     */
    NetworkDataTlv *GetSubTlvs(void) { return reinterpret_cast<NetworkDataTlv *>(GetValue() + GetFieldsLength()); }

    /**
     * This method returns a pointer to the Sub-TLVs.
     *
     * @returns A pointer to the Sub-TLVs.
     *
     */
    const NetworkDataTlv *GetSubTlvs(void) const
    {
        return reinterpret_cast<const NetworkDataTlv *>(GetValue() + GetFieldsLength());
    }

    /**
     * This static method calculates the total size (number of bytes) of a Service TLV with a given Enterprise Number
     * and Service Data length.
     *
     * Note that the returned size does include the Type and Length fields in the TLV, but does not account for any
     * sub-TLVs of the Service TLV.
     *
     * @param[in]  aEnterpriseNumber   A Enterprise Number.
     * @param[in]  aServiceDataLength  A Service Data length.
     *
     * @returns    The size (number of bytes) of the Service TLV.
     *
     */
    static uint16_t CalculateSize(uint32_t aEnterpriseNumber, uint8_t aServiceDataLength)
    {
        return sizeof(NetworkDataTlv) + kMinLength + aServiceDataLength +
               ((aEnterpriseNumber == kThreadEnterpriseNumber) ? 0 : sizeof(uint32_t) /* mEnterpriseNumber  */);
    }

private:
    bool IsThreadEnterprise(void) const { return (mFlagsServiceId & kThreadEnterpriseFlag) != 0; }

    uint8_t GetFieldsLength(void) const
    {
        // Returns the length of TLV value's common fields (flags, enterprise
        // number and service data) excluding any sub-TLVs.

        return kMinLength + (IsThreadEnterprise() ? 0 : sizeof(uint32_t)) + GetServiceDataLength();
    }

    enum
    {
        kThreadEnterpriseFlag = (1 << 7),
        kServiceIdMask        = 0xf,
        kMinLength            = sizeof(uint8_t) + sizeof(uint8_t), // Flags and Service Data length.
    };

    // When `kThreadEnterpriseFlag is set in the `mFlagsServiceId`, the
    // `mEnterpriseNumber` field is elided and `mFlagsServiceId` is
    // immediately followed by the Service Data length field (which is
    // represented by `mServiceDataLengthThreadEnterprise`)

    uint8_t mFlagsServiceId;
    union OT_TOOL_PACKED_FIELD
    {
        uint32_t mEnterpriseNumber;
        uint8_t  mServiceDataLengthThreadEnterprise;
    } mShared;
    uint8_t mServiceDataLength;

} OT_TOOL_PACKED_END;

/**
 * This class implements Server Data TLV generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class ServerTlv : public NetworkDataTlv
{
public:
    enum
    {
        kType = kTypeServer, ///< The TLV Type.
    };

    /**
     * This method initializes the Server TLV.
     *
     * @param[in] aServer16          The Server16 value.
     * @param[in] aServerData        The Server Data.
     * @param[in] aServerDataLength  Server Data length in bytes.
     *
     */
    void Init(uint16_t aServer16, const uint8_t *aServerData, uint8_t aServerDataLength)
    {
        NetworkDataTlv::Init();
        SetType(kTypeServer);
        SetServer16(aServer16);
        memcpy(reinterpret_cast<uint8_t *>(this) + sizeof(*this), aServerData, aServerDataLength);
        SetLength(sizeof(*this) - sizeof(NetworkDataTlv) + aServerDataLength);
    }

    /**
     * This method indicates whether or not the TLV appears to be well-formed.
     *
     * @retval TRUE   If the TLV appears to be well-formed.
     * @retval FALSE  If the TLV does not appear to be well-formed.
     *
     */
    bool IsValid(void) const { return GetLength() >= (sizeof(*this) - sizeof(NetworkDataTlv)); }

    /**
     * This method returns the Server16 value.
     *
     * @returns The Server16 value.
     *
     */
    uint16_t GetServer16(void) const { return HostSwap16(mServer16); }

    /*
     * This method sets the Server16 value.
     *
     * @param[in]  aServer16  The Server16 value.
     *
     */
    void SetServer16(uint16_t aServer16) { mServer16 = HostSwap16(aServer16); }

    /**
     * This method returns the Server Data.
     *
     * @returns A pointer to the Server Data.
     *
     */
    const uint8_t *GetServerData(void) const { return reinterpret_cast<const uint8_t *>(this) + sizeof(*this); }

    /**
     * This method returns the Server Data length in bytes.
     *
     * @returns The Server Data length in bytes.
     *
     */
    uint8_t GetServerDataLength(void) const { return GetLength() - (sizeof(*this) - sizeof(NetworkDataTlv)); }

    /**
     * This method indicates whether two Server TLVs fully match.
     *
     * @param[in]  aOther  Another Server TLV to compare with it.
     *
     * @retval TRUE  The two TLVs are equal.
     * @retval FALSE The two TLVs are not equal.
     *
     */
    bool operator==(const ServerTlv &aOther) const
    {
        return (GetLength() == aOther.GetLength()) && (memcmp(GetValue(), aOther.GetValue(), GetLength()) == 0);
    }

    /**
     * This static method calculates the total size (number of bytes) of a Service TLV with a given Server Data length.
     *
     * Note that the returned size does include the Type and Length fields in the TLV.
     *
     * @param[in]  aServerDataLength    Server Data length in bytes.
     *
     * @returns    The size (number of bytes) of the Server TLV.
     *
     */
    static uint16_t CalculateSize(uint8_t aServerDataLength) { return sizeof(ServerTlv) + aServerDataLength; }

private:
    uint16_t mServer16;
} OT_TOOL_PACKED_END;

OT_TOOL_PACKED_BEGIN
class BackboneRouterServerData
{
public:
    /**
     * This method returns the sequence number of Backbone Router.
     *
     * @returns  The sequence number of the Backbone Router.
     *
     */
    uint8_t GetSequenceNumber(void) const { return mSequenceNumber; }

    /**
     * This method sets the sequence number of Backbone Router.
     *
     * @param[in]  aSequenceNumber  The sequence number of Backbone Router.
     *
     */
    void SetSequenceNumber(uint8_t aSequenceNumber) { mSequenceNumber = aSequenceNumber; }

    /**
     * This method returns the Registration Delay (in seconds) of Backbone Router.
     *
     * @returns The BBR Registration Delay (in seconds) of Backbone Router.
     *
     */
    uint16_t GetReregistrationDelay(void) const { return HostSwap16(mReregistrationDelay); }

    /**
     * This method sets the Registration Delay (in seconds) of Backbone Router.
     *
     * @param[in]  aReregistrationDelay  The Registration Delay (in seconds) of Backbone Router.
     *
     */
    void SetReregistrationDelay(uint16_t aReregistrationDelay)
    {
        mReregistrationDelay = HostSwap16(aReregistrationDelay);
    }

    /**
     * This method returns the multicast listener report timeout (in seconds) of Backbone Router.
     *
     * @returns The multicast listener report timeout (in seconds) of Backbone Router.
     *
     */
    uint32_t GetMlrTimeout(void) const { return HostSwap32(mMlrTimeout); }

    /**
     * This method sets multicast listener report timeout (in seconds) of Backbone Router.
     *
     * @param[in]  aMlrTimeout  The multicast listener report timeout (in seconds) of Backbone Router.
     *
     */
    void SetMlrTimeout(uint32_t aMlrTimeout) { mMlrTimeout = HostSwap32(aMlrTimeout); }

private:
    uint8_t  mSequenceNumber;
    uint16_t mReregistrationDelay;
    uint32_t mMlrTimeout;
} OT_TOOL_PACKED_END;

/**
 * @}
 *
 */

} // namespace NetworkData
} // namespace ot

#endif // NETWORK_DATA_TLVS_HPP_
