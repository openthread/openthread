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

#include "utils/wrap_string.h"

#include "common/encoding.hpp"
#include "net/ip6_address.hpp"

using ot::Encoding::BigEndian::HostSwap16;

#define THREAD_ENTERPRISE_NUMBER 44970

namespace ot {
namespace NetworkData {

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
     * This method returns a pointer to the Value.
     *
     * @returns A pointer to the value.
     *
     */
    uint8_t *GetValue(void) { return reinterpret_cast<uint8_t *>(this) + sizeof(NetworkDataTlv); }

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
        assert((aPrf == OT_ROUTE_PREFERENCE_LOW) || (aPrf == OT_ROUTE_PREFERENCE_MED) ||
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
     * This method returns a pointer to the first HasRouteEntry (at index 0'th).
     *
     * @returns A pointer to the first HasRouteEntry.
     *
     */
    HasRouteEntry *GetFirstEntry(void) { return reinterpret_cast<HasRouteEntry *>(GetValue()); }

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
} OT_TOOL_PACKED_END;

/**
 * This class implements Prefix TLV generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class PrefixTlv : public NetworkDataTlv
{
public:
    /**
     * This method initializes the TLV.
     *
     * @param[in]  aDomainId      The Domain ID.
     * @param[in]  aPrefixLength  The Prefix Length
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
                (GetLength() >= BitVectorBytes(mPrefixLength) + sizeof(*this) - sizeof(NetworkDataTlv)));
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
     * @returns The Prefix Length value.
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
     * This method returns a pointer to the first BorderRouterEntry (at index 0'th).
     *
     * @returns A pointer to the first BorderRouterEntry.
     *
     */
    BorderRouterEntry *GetFirstEntry(void) { return reinterpret_cast<BorderRouterEntry *>(GetValue()); }

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
} OT_TOOL_PACKED_END;

/**
 * This class implements Context TLV generation and processing.
 *
 */
OT_TOOL_PACKED_BEGIN
class ContextTlv : public NetworkDataTlv
{
public:
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
    /**
     * This method initializes the TLV.
     * Initial length is set to 2, to hold S_service_data_length field.
     */
    void Init(void)
    {
        NetworkDataTlv::Init();
        SetType(kTypeService);
        SetLength(2);
        mTResSId = kTMask;
        SetServiceDataLength(0);
    }

    /**
     * This method indicates whether or not the TLV appears to be well-formed.
     *
     * @retval TRUE   If the TLV appears to be well-formed.
     * @retval FALSE  If the TLV does not appear to be well-formed.
     *
     */
    bool IsValid(void)
    {
        uint8_t length = GetLength();
        return ((length >= (sizeof(*this) - sizeof(NetworkDataTlv))) &&
                (length >= ((sizeof(*this) - sizeof(NetworkDataTlv)) + (IsThreadEnterprise() ? 0 : sizeof(uint32_t)) +
                            sizeof(uint8_t))) &&
                (length >= ((sizeof(*this) - sizeof(NetworkDataTlv)) + (IsThreadEnterprise() ? 0 : sizeof(uint32_t)) +
                            sizeof(uint8_t) + GetServiceDataLength())));
    }

    /**
     * This method gets Service Data length.
     *
     * @returns length of the Service Data field in bytes.
     */
    uint8_t GetServiceDataLength(void) { return *GetServiceDataLengthLocation(); }

    /**
     * This method sets Service Data length.
     *
     * @param aServiceDataLength desired length of the Service Data field in bytes.
     */
    void SetServiceDataLength(uint8_t aServiceDataLength) { *GetServiceDataLengthLocation() = aServiceDataLength; }

    /**
     * This method returns a pointer to the Service Data.
     *
     * @returns A pointer to the Service Data.
     */
    uint8_t *GetServiceData(void) { return GetServiceDataLengthLocation() + sizeof(uint8_t); }

    /**
     * This method sets Service Data to the given values.
     *
     * Caller must ensure that there is enough memory allocated.
     *
     * @param aServiceData       pointer to the service data to use
     * @param aServiceDataLength length of the provided service data in bytes
     */
    void SetServiceData(const uint8_t *aServiceData, uint8_t aServiceDataLength)
    {
        SetServiceDataLength(aServiceDataLength);

        memcpy(GetServiceData(), aServiceData, aServiceDataLength);
    }

    /**
     * This method returns Enterprise Number field.
     *
     * @returns Enterprise Number
     */
    uint32_t GetEnterpriseNumber(void)
    {
        if (IsThreadEnterprise())
        {
            return THREAD_ENTERPRISE_NUMBER;
        }
        else
        {
            // This memory access most likely will not be aligned to 4 bytes
            return HostSwap32(*reinterpret_cast<uint32_t *>(GetEnterpriseNumberLocation()));
        }
    }

    /**
     * This method returns the T flag. It is set when Enterprise Number is equal to THREAD_ENTERPRISE_NUMBER.
     *
     * @returns Flag whether Enterprise Number is equal to THREAD_ENTERPRISE_NUMBER
     */
    bool IsThreadEnterprise(void) const { return (mTResSId & kTMask) != 0; }

    /**
     * This method sets Enterprise Number and updates the T flag.
     *
     * Note: this method does not preserve service data / sub-TLV fields. Changing the T flag and inserting
     * few bytes in the middle of this TLV effectively destroys rest of the content of this TLV (and might lead to
     * memory corruption) so modification of Enterprise Number must be done before adding any content to the TLV.
     *
     * @param [in] aEnterpriseNumber Enterprise Number
     */
    void SetEnterpriseNumber(uint32_t aEnterpriseNumber)
    {
        if (aEnterpriseNumber == THREAD_ENTERPRISE_NUMBER)
        {
            mTResSId |= kTMask;
        }
        else
        {
            mTResSId &= ~kTMask;

            // This memory access most likely will not be aligned to 4 bytes
            *reinterpret_cast<uint32_t *>(GetEnterpriseNumberLocation()) = HostSwap32(aEnterpriseNumber);
        }
    }

    /**
     * This method returns length of the S_enterprise_number TLV field in bytes, for given Enterprise Number.
     *
     * @returns length of the S_enterprise_number field in bytes
     */
    static uint8_t GetEnterpriseNumberFieldLength(uint32_t aEnterpriseNumber)
    {
        if (aEnterpriseNumber == THREAD_ENTERPRISE_NUMBER)
        {
            return 0;
        }
        else
        {
            return (sizeof(aEnterpriseNumber));
        }
    }

    /**
     * This method returns Service ID. It is in range 0x00-0x0f.
     *
     * @returns Service ID
     */
    uint8_t GetServiceID(void) const { return (mTResSId & kSIdMask) >> kSIdOffset; }

    /**
     * This method sets Service ID.
     *
     * @param [in] aServiceID Service ID to be set. Expected range: 0x00-0x0f.
     */
    void SetServiceID(uint8_t aServiceID)
    {
        mTResSId = static_cast<uint8_t>((mTResSId & ~kSIdMask) | (aServiceID << kSIdOffset));
    }

    /**
     * This method returns the Sub-TLVs length in bytes.
     *
     * @returns The Sub-TLVs length in bytes.
     *
     */
    uint8_t GetSubTlvsLength(void)
    {
        return GetLength() - (sizeof(*this) - sizeof(NetworkDataTlv)) - (IsThreadEnterprise() ? 0 : sizeof(uint32_t)) -
               sizeof(uint8_t) /* mServiceDataLength */ - GetServiceDataLength();
    }

    /**
     * This method sets the Sub-TLVs length in bytes.
     *
     * @param[in]  aLength  The Sub-TLVs length in bytes.
     *
     */
    void SetSubTlvsLength(uint8_t aLength)
    {
        SetLength(sizeof(*this) - sizeof(NetworkDataTlv) + (IsThreadEnterprise() ? 0 : sizeof(uint32_t)) +
                  sizeof(uint8_t) /* mServiceDataLength */ + GetServiceDataLength() + aLength);
    }

    /**
     * This method returns a pointer to the Sub-TLVs.
     *
     * @returns A pointer to the Sub-TLVs.
     *
     */
    NetworkDataTlv *GetSubTlvs(void)
    {
        return reinterpret_cast<NetworkDataTlv *>(GetServiceDataLengthLocation() + sizeof(uint8_t) +
                                                  GetServiceDataLength());
    }

private:
    /**
     * This method returns pointer to where mServiceDataLength would be.
     *
     * @returns pointer to service data length location
     */
    uint8_t *GetServiceDataLengthLocation(void)
    {
        return GetEnterpriseNumberLocation() + (IsThreadEnterprise() ? 0 : sizeof(uint32_t));
    }

    /**
     * This method returns pointer to where mEnterpriseNumber would be.
     *
     * Note: this method returns uint8_t*, not uint32_t*.
     *
     * @returns pointer to enterprise number location
     */
    uint8_t *GetEnterpriseNumberLocation(void) { return &mTResSId + sizeof(mTResSId); }

    enum
    {
        kTOffset   = 7,
        kTMask     = 0x1 << kTOffset,
        kSIdOffset = 0,
        kSIdMask   = 0xf << kSIdOffset,
    };

    uint8_t mTResSId;
} OT_TOOL_PACKED_END;

/**
 * This class implements Server Data TLV generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class ServerTlv : public NetworkDataTlv
{
public:
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
     */
    const uint8_t *GetServerData(void) { return reinterpret_cast<uint8_t *>(this) + sizeof(*this); }

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

/**
 * @}
 *
 */

} // namespace NetworkData
} // namespace ot

#endif // NETWORK_DATA_TLVS_HPP_
