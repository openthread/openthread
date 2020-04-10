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
class HasRouteEntry
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
     * This method returns a pointer to the i'th HasRoute entry.
     *
     * @param[in]  i  An index.
     *
     * @returns A pointer to the i'th HasRoute entry.
     *
     */
    HasRouteEntry *GetEntry(uint8_t i)
    {
        return reinterpret_cast<HasRouteEntry *>(GetValue() + (i * sizeof(HasRouteEntry)));
    }

    /**
     * This method returns a pointer to the i'th HasRoute entry.
     *
     * @param[in]  i  An index.
     *
     * @returns A pointer to the i'th HasRoute entry.
     *
     */
    const HasRouteEntry *GetEntry(uint8_t i) const
    {
        return reinterpret_cast<const HasRouteEntry *>(GetValue() + (i * sizeof(HasRouteEntry)));
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
        memcpy(GetPrefix(), aPrefix, BitVectorBytes(aPrefixLength));
        SetSubTlvsLength(0);
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
                (GetLength() >= BitVectorBytes(mPrefixLength) + sizeof(*this) - sizeof(NetworkDataTlv)) &&
                (BitVectorBytes(mPrefixLength) <= sizeof(Ip6::Address)));
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
     * This method returns a pointer to the Sub-TLVs.
     *
     * @returns A pointer to the Sub-TLVs.
     *
     */
    NetworkDataTlv *GetSubTlvs(void)
    {
        return reinterpret_cast<NetworkDataTlv *>(GetPrefix() + BitVectorBytes(mPrefixLength));
    }

    /**
     * This method returns a pointer to the Sub-TLVs.
     *
     * @returns A pointer to the Sub-TLVs.
     *
     */
    const NetworkDataTlv *GetSubTlvs(void) const
    {
        return reinterpret_cast<const NetworkDataTlv *>(GetPrefix() + BitVectorBytes(mPrefixLength));
    }

    /**
     * This method returns the Sub-TLVs length in bytes.
     *
     * @returns The Sub-TLVs length in bytes.
     *
     */
    uint8_t GetSubTlvsLength(void) const
    {
        return GetLength() - (sizeof(*this) - sizeof(NetworkDataTlv) + BitVectorBytes(mPrefixLength));
    }

    /**
     * This method sets the Sub-TLVs length in bytes.
     *
     * @param[in]  aLength  The Sub-TLVs length in bytes.
     *
     */
    void SetSubTlvsLength(uint8_t aLength)
    {
        SetLength(sizeof(*this) - sizeof(NetworkDataTlv) + BitVectorBytes(mPrefixLength) + aLength);
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
class BorderRouterEntry
{
public:
    enum
    {
        kPreferenceOffset = 6,
        kPreferenceMask   = 3 << kPreferenceOffset,
        kPreferredFlag    = 1 << 5,
        kSlaacFlag        = 1 << 4,
        kDhcpFlag         = 1 << 3,
        kConfigureFlag    = 1 << 2,
        kDefaultRouteFlag = 1 << 1,
        kOnMeshFlag       = 1 << 0,
    };

    /**
     * This method initializes the TLV.
     *
     */
    void Init(void)
    {
        SetRloc(Mac::kShortAddrInvalid);
        mFlags    = 0;
        mReserved = 0;
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
     * This method returns the Flags byte value.
     *
     * @returns The Flags byte value.
     *
     */
    uint8_t GetFlags(void) const { return mFlags & ~kPreferenceMask; }

    /**
     * This method sets the Flags byte value.
     *
     * @param[in]  aFlags  The Flags byte value.
     *
     */
    void SetFlags(uint8_t aFlags) { mFlags = (mFlags & kPreferenceMask) | (aFlags & ~kPreferenceMask); }

    /**
     * This method returns the Preference value.
     *
     * @returns the Preference value.
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
        mFlags = (mFlags & ~kPreferenceMask) | ((static_cast<uint8_t>(aPrf) << kPreferenceOffset) & kPreferenceMask);
    }

    /**
     * This method indicates whether or not the Preferred flag is set.
     *
     * @retval TRUE   If the Preferred flag is set.
     * @retval FALSE  If the Preferred flag is not set.
     *
     */
    bool IsPreferred(void) const { return (mFlags & kPreferredFlag) != 0; }

    /**
     * This method clears the Preferred flag.
     *
     */
    void ClearPreferred(void) { mFlags &= ~kPreferredFlag; }

    /**
     * This method sets the Preferred flag.
     *
     */
    void SetPreferred(void) { mFlags |= kPreferredFlag; }

    /**
     * This method indicates whether or not the SLAAC flag is set.
     *
     * @retval TRUE   If the SLAAC flag is set.
     * @retval FALSE  If the SLAAC flag is not set.
     *
     */
    bool IsSlaac(void) const { return (mFlags & kSlaacFlag) != 0; }

    /**
     * This method clears the SLAAC flag.
     *
     */
    void ClearSlaac(void) { mFlags &= ~kSlaacFlag; }

    /**
     * This method sets the SLAAC flag.
     *
     */
    void SetSlaac(void) { mFlags |= kSlaacFlag; }

    /**
     * This method indicates whether or not the DHCP flag is set.
     *
     * @retval TRUE   If the DHCP flag is set.
     * @retval FALSE  If the DHCP flag is not set.
     *
     */
    bool IsDhcp(void) const { return (mFlags & kDhcpFlag) != 0; }

    /**
     * This method clears the DHCP flag.
     *
     */
    void ClearDhcp(void) { mFlags &= ~kDhcpFlag; }

    /**
     * This method sets the DHCP flag.
     *
     */
    void SetDhcp(void) { mFlags |= kDhcpFlag; }

    /**
     * This method indicates whether or not the Configure flag is set.
     *
     * @retval TRUE   If the Configure flag is set.
     * @retval FALSE  If the Configure flag is not set.
     *
     */
    bool IsConfigure(void) const { return (mFlags & kConfigureFlag) != 0; }

    /**
     * This method clears the Configure flag.
     *
     */
    void ClearConfigure(void) { mFlags &= ~kConfigureFlag; }

    /**
     * This method sets the Configure flag.
     *
     */
    void SetConfigure(void) { mFlags |= kConfigureFlag; }

    /**
     * This method indicates whether or not the Default Route flag is set.
     *
     * @retval TRUE   If the Default Route flag is set.
     * @retval FALSE  If the Default Route flag is not set.
     *
     */
    bool IsDefaultRoute(void) const { return (mFlags & kDefaultRouteFlag) != 0; }

    /**
     * This method clears the Default Route flag.
     *
     */
    void ClearDefaultRoute(void) { mFlags &= ~kDefaultRouteFlag; }

    /**
     * This method sets the Default Route flag.
     *
     */
    void SetDefaultRoute(void) { mFlags |= kDefaultRouteFlag; }

    /**
     * This method indicates whether or not the On-Mesh flag is set.
     *
     * @retval TRUE   If the On-Mesh flag is set.
     * @retval FALSE  If the On-Mesh flag is not set.
     *
     */
    bool IsOnMesh(void) const { return (mFlags & kOnMeshFlag) != 0; }

    /**
     * This method clears the On-Mesh flag.
     *
     */
    void ClearOnMesh(void) { mFlags &= ~kOnMeshFlag; }

    /**
     * This method sets the On-Mesh flag.
     *
     */
    void SetOnMesh(void) { mFlags |= kOnMeshFlag; }

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
    uint8_t  mFlags;
    uint8_t  mReserved;
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
     * This method returns a pointer to the i'th Border Router entry.
     *
     * @param[in]  i  The index.
     *
     * @returns A pointer to the i'th Border Router entry.
     *
     */
    BorderRouterEntry *GetEntry(uint8_t i)
    {
        return reinterpret_cast<BorderRouterEntry *>(GetValue() + (i * sizeof(BorderRouterEntry)));
    }

    /**
     * This method returns a pointer to the i'th Border Router entry.
     *
     * @param[in]  i  The index.
     *
     * @returns A pointer to the i'th Border Router entry.
     *
     */
    const BorderRouterEntry *GetEntry(uint8_t i) const
    {
        return reinterpret_cast<const BorderRouterEntry *>(GetValue() + (i * sizeof(BorderRouterEntry)));
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
     * This method initializes the TLV.
     *
     */
    void Init(void)
    {
        NetworkDataTlv::Init();
        SetType(kTypeContext);
        SetLength(2);
        mFlags         = 0;
        mContextLength = 0;
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
     * This method sets the Context ID value.
     *
     * @param[in]  aContextId  The Context ID value.
     *
     */
    void SetContextId(uint8_t aContextId)
    {
        mFlags = (mFlags & ~kContextIdMask) | ((aContextId << kContextIdOffset) & kContextIdMask);
    }

    /**
     * This method returns the Context Length value.
     *
     * @returns The Context Length value.
     *
     */
    uint8_t GetContextLength(void) const { return mContextLength; }

    /**
     * This method sets the Context Length value.
     *
     * @param[in]  aLength  The Context Length value.
     *
     */
    void SetContextLength(uint8_t aLength) { mContextLength = aLength; }

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
               (length >= kMinLength + (IsThreadEnterprise() ? 0 : sizeof(uint32_t))) && (length >= GetFieldsLength());
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
    static uint16_t GetSize(uint32_t aEnterpriseNumber, uint8_t aServiceDataLength)
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
     * This method initializes the TLV.
     *
     */
    void Init(void)
    {
        NetworkDataTlv::Init();
        SetType(kTypeServer);
        SetLength(sizeof(*this) - sizeof(NetworkDataTlv));
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
     * This method returns the S_server_16 value.
     *
     * @returns The S_server_16 value.
     *
     */
    uint16_t GetServer16(void) const { return HostSwap16(mServer16); }

    /**
     * This method sets the S_server_16 value.
     *
     * @param[in]  aServer16  The S_server_16 value.
     *
     */
    void SetServer16(uint16_t aServer16) { mServer16 = HostSwap16(aServer16); }

    /**
     * This method returns a pointer to the Server Data.
     *
     * @returns A pointer to the Server Data.
     *
     */
    const uint8_t *GetServerData(void) const { return reinterpret_cast<const uint8_t *>(this) + sizeof(*this); }

    /**
     * This method sets Server Data to the given values.
     *
     * Caller must ensure that there is enough memory allocated.
     *
     * @param aServerData       pointer to the server data to use
     * @param aServerDataLength length of the provided server data in bytes
     */
    void SetServerData(const uint8_t *aServerData, uint8_t aServerDataLength)
    {
        SetLength(sizeof(*this) - sizeof(NetworkDataTlv) + aServerDataLength);
        memcpy(reinterpret_cast<uint8_t *>(this) + sizeof(*this), aServerData, aServerDataLength);
    }

    /**
     * This method returns the Server Data length in bytes.
     *
     * @returns The Server Data length in bytes.
     *
     */
    uint8_t GetServerDataLength(void) const { return GetLength() - (sizeof(*this) - sizeof(NetworkDataTlv)); }

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
