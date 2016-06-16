/*
 *  Copyright (c) 2016, Nest Labs, Inc.
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

#include <string.h>

#include <common/encoding.hpp>
#include <net/ip6_address.hpp>

using Thread::Encoding::BigEndian::HostSwap16;

namespace Thread {
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
    void Init() { mType = 0; mLength = 0; }

    /**
     * Thread Network Data Type values.
     *
     */
    enum Type
    {
        kTypeHasRoute          = 0,  ///< Has Route TLV
        kTypePrefix            = 1,  ///< Prefix TLV
        kTypeBorderRouter      = 2,  ///< Border Router TLV
        kTypeContext           = 3,  ///< Context TLV
        kTypeCommissioningData = 4,  ///< Commissioning Dataset TLV
    };

    /**
     * This method returns the Type value.
     *
     * @returns The Type value.
     *
     */
    Type GetType() const { return static_cast<Type>(mType >> kTypeOffset); }

    /**
     * This method sets the Type value.
     *
     * @param[in]  aType  The Type value.
     *
     */
    void SetType(Type aType) { mType = (mType & ~kTypeMask) | (static_cast<uint8_t>(aType) << kTypeOffset); }

    /**
     * This method returns the Length value.
     *
     * @returns The Length value.
     *
     */
    uint8_t GetLength() const { return mLength; }

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
    uint8_t *GetValue() { return reinterpret_cast<uint8_t *>(this) + sizeof(NetworkDataTlv); }

    /**
     * This method returns a pointer to the next Network Data TLV.
     *
     * @returns A pointer to the next Network Data TLV.
     *
     */
    NetworkDataTlv *GetNext() {
        return reinterpret_cast<NetworkDataTlv *>(reinterpret_cast<uint8_t *>(this) + sizeof(*this) + mLength);
    }

    /**
     * This method clears the Stable bit.
     *
     */
    void ClearStable() { mType &= ~kStableMask; }

    /**
     * This method indicates whether or not the Stable bit is set.
     *
     * @retval TRUE   If the Stable bit is set.
     * @retval FALSE  If the Stable bit is not set.
     *
     */
    bool IsStable() const { return (mType & kStableMask); }

    /**
     * This method sets the Stable bit.
     *
     */
    void SetStable() { mType |= kStableMask; }

private:
    enum
    {
        kTypeOffset = 1,
        kTypeMask = 0x7f << kTypeOffset,
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
    void Init() { SetRloc(Mac::kShortAddrInvalid); mFlags = 0; }

    /**
     * This method returns the RLOC16 value.
     *
     * @returns The RLOC16 value.
     */
    uint16_t GetRloc() const { return HostSwap16(mRloc); }

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
    int8_t GetPreference() const { return static_cast<int8_t>(mFlags) >> kPreferenceOffset; }

    /**
     * This method sets the Preference value.
     *
     * @param[in]  aPrf  The Preference value.
     *
     */
    void SetPreference(int8_t aPrf) { mFlags = (mFlags & ~kPreferenceMask) | (aPrf << kPreferenceOffset); }

private:
    enum
    {
        kPreferenceOffset = 6,
        kPreferenceMask = 3 << kPreferenceOffset,
    };

    uint16_t mRloc;
    uint8_t mFlags;
} OT_TOOL_PACKED_END;

/**
 * This class implements Has Route TLV generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class HasRouteTlv: public NetworkDataTlv
{
public:
    /**
     * This method initializes the TLV.
     *
     */
    void Init() { NetworkDataTlv::Init(); SetType(kTypeHasRoute); SetLength(0); }

    /**
     * This method returns the number of HasRoute entries.
     *
     * @returns The number of HasRoute entries.
     *
     */
    uint8_t GetNumEntries() const { return GetLength() / sizeof(HasRouteEntry); }

    /**
     * This method returns a pointer to the i'th HasRoute entry.
     *
     * @param[in]  i  An index.
     *
     * @returns A pointer to the i'th HasRoute entry.
     *
     */
    HasRouteEntry *GetEntry(int i) {
        return reinterpret_cast<HasRouteEntry *>(GetValue() + (i * sizeof(HasRouteEntry)));
    }
} OT_TOOL_PACKED_END;

/**
 * This class implements Prefix TLV generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class PrefixTlv: public NetworkDataTlv
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
    void Init(uint8_t aDomainId, uint8_t aPrefixLength, const uint8_t *aPrefix) {
        NetworkDataTlv::Init();
        SetType(kTypePrefix);
        mDomainId = aDomainId;
        mPrefixLength = aPrefixLength;
        memcpy(GetPrefix(), aPrefix, BitVectorBytes(aPrefixLength));
        SetSubTlvsLength(0);
    }

    /**
     * This method returns the Domain ID value.
     *
     * @returns The Domain ID value.
     *
     */
    uint8_t GetDomainId() const { return mDomainId; }

    /**
     * This method returns the Prefix Length value.
     *
     * @returns The Prefix Length value.
     *
     */
    uint8_t GetPrefixLength() const { return mPrefixLength; }

    /**
     * This method returns a pointer to the Prefix.
     *
     * @returns A pointer to the Prefix.
     *
     */
    uint8_t *GetPrefix() { return reinterpret_cast<uint8_t *>(this) + sizeof(*this); }

    /**
     * This method returns a pointer to the Sub-TLVs.
     *
     * @returns A pointer to the Sub-TLVs.
     *
     */
    uint8_t *GetSubTlvs() { return GetPrefix() + BitVectorBytes(mPrefixLength); }

    /**
     * This method returns the Sub-TLVs length in bytes.
     *
     * @returns The Sub-TLVs length in bytes.
     *
     */
    uint8_t GetSubTlvsLength() const {
        return GetLength() - (sizeof(*this) - sizeof(NetworkDataTlv) + BitVectorBytes(mPrefixLength));
    }

    /**
     * This method sets the Sub-TLVs length in bytes.
     *
     * @param[in]  aLength  The Sub-TLVs length in bytes.
     *
     */
    void SetSubTlvsLength(int aLength) {
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
        kPreferenceMask = 3 << kPreferenceOffset,
        kPreferredFlag = 1 << 5,
        kValidFlag = 1 << 4,
        kDhcpFlag = 1 << 3,
        kConfigureFlag = 1 << 2,
        kDefaultRouteFlag = 1 << 1,
    };

    /**
     * This method initializes the TLV.
     *
     */
    void Init() { SetRloc(Mac::kShortAddrInvalid); mFlags = 0; mReserved = 0; }

    /**
     * This method returns the RLOC16 value.
     *
     * @returns The RLOC16 value.
     */
    uint16_t GetRloc() const { return HostSwap16(mRloc); }

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
    uint8_t GetFlags() const { return mFlags & ~kPreferenceMask; }

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
    int8_t GetPreference() const { return static_cast<int8_t>(mFlags) >> kPreferenceOffset; }

    /**
     * This method sets the Preference value.
     *
     * @param[in]  aPrf  The Preference value.
     *
     */
    void SetPreference(int8_t aPrf) { mFlags = (mFlags & ~kPreferenceMask) | (aPrf << kPreferenceOffset); }

    /**
     * This method indicates whether or not the Preferred flag is set.
     *
     * @retval TRUE   If the Preferred flag is set.
     * @retval FALSE  If the Preferred flag is not set.
     *
     */
    bool IsPreferred() const { return (mFlags & kPreferredFlag) != 0; }

    /**
     * This method clears the Preferred flag.
     *
     */
    void ClearPreferred() { mFlags &= ~kPreferredFlag; }

    /**
     * This method sets the Preferred flag.
     *
     */
    void SetPreferred() { mFlags |= kPreferredFlag; }

    /**
     * This method indicates whether or not the Valid flag is set.
     *
     * @retval TRUE   If the Valid flag is set.
     * @retval FALSE  If the Valid flag is not set.
     *
     */
    bool IsValid() const { return (mFlags & kValidFlag) != 0; }

    /**
     * This method clears the Valid flag.
     *
     */
    void ClearValid() { mFlags &= ~kValidFlag; }

    /**
     * This method sets the Valid flag.
     *
     */
    void SetValid() { mFlags |= kValidFlag; }

    /**
     * This method indicates whether or not the DHCP flag is set.
     *
     * @retval TRUE   If the DHCP flag is set.
     * @retval FALSE  If the DHCP flag is not set.
     *
     */
    bool IsDhcp() const { return (mFlags & kDhcpFlag) != 0; }

    /**
     * This method clears the DHCP flag.
     *
     */
    void ClearDhcp() { mFlags &= ~kDhcpFlag; }

    /**
     * This method sets the DHCP flag.
     *
     */
    void SetDhcp() { mFlags |= kDhcpFlag; }

    /**
     * This method indicates whether or not the Configure flag is set.
     *
     * @retval TRUE   If the Configure flag is set.
     * @retval FALSE  If the Configure flag is not set.
     *
     */
    bool IsConfigure() const { return (mFlags & kConfigureFlag) != 0; }

    /**
     * This method clears the Configure flag.
     *
     */
    void ClearConfigure() { mFlags &= ~kConfigureFlag; }

    /**
     * This method sets the Configure flag.
     *
     */
    void SetConfigure() { mFlags |= kConfigureFlag; }

    /**
     * This method indicates whether or not the Default Route flag is set.
     *
     * @retval TRUE   If the Default Route flag is set.
     * @retval FALSE  If the Default Route flag is not set.
     *
     */
    bool IsDefaultRoute() const { return (mFlags & kDefaultRouteFlag) != 0; }

    /**
     * This method clears the Default Route flag.
     *
     */
    void ClearDefaultRoute() { mFlags &= ~kDefaultRouteFlag; }

    /**
     * This method sets the Default Route flag.
     *
     */
    void SetDefaultRoute() { mFlags |= kDefaultRouteFlag; }

private:
    uint16_t mRloc;
    uint8_t mFlags;
    uint8_t mReserved;
} OT_TOOL_PACKED_END;

/**
 * This class implements Border Router TLV generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class BorderRouterTlv: public NetworkDataTlv
{
public:
    /**
     * This method initializes the TLV.
     *
     */
    void Init() { NetworkDataTlv::Init(); SetType(kTypeBorderRouter); SetLength(0); }

    /**
     * This method returns the number of Border Router entries.
     *
     * @returns The number of Border Router entries.
     *
     */
    uint8_t GetNumEntries() const { return GetLength() / sizeof(BorderRouterEntry); }

    /**
     * This method returns a pointer to the i'th Border Router entry.
     *
     * @param[in]  i  The index.
     *
     * @returns A pointer to the i'th Border Router entry.
     *
     */
    BorderRouterEntry *GetEntry(int i) {
        return reinterpret_cast<BorderRouterEntry *>(GetValue() + (i * sizeof(BorderRouterEntry)));
    }
} OT_TOOL_PACKED_END;

/**
 * This class implements Context TLV generation and processing.
 *
 */
OT_TOOL_PACKED_BEGIN
class ContextTlv: public NetworkDataTlv
{
public:
    /**
     * This method initializes the TLV.
     *
     */
    void Init() { NetworkDataTlv::Init(); SetType(kTypeContext); SetLength(2); mFlags = 0; mContextLength = 0; }

    /**
     * This method indicates whether or not the Compress flag is set.
     *
     * @retval TRUE   The Compress flag is set.
     * @retval FALSE  The Compress flags is not set.
     *
     */
    bool IsCompress() const { return (mFlags & kCompressFlag) != 0; }

    /**
     * This method clears the Compress flag.
     *
     */
    void ClearCompress() { mFlags &= ~kCompressFlag; }

    /**
     * This method sets the Compress flag.
     *
     */
    void SetCompress() { mFlags |= kCompressFlag; }

    /**
     * This method returns the Context ID value.
     *
     * @returns The Context ID value.
     *
     */
    uint8_t GetContextId() const { return mFlags & kContextIdMask; }

    /**
     * This method sets the Context ID value.
     *
     * @param[in]  aContextId  The Context ID value.
     *
     */
    void SetContextId(uint8_t aContextId) { mFlags = (mFlags & ~kContextIdMask) | (aContextId << kContextIdOffset); }

    /**
     * This method returns the Context Length value.
     *
     * @returns The Context Length value.
     *
     */
    uint8_t GetContextLength() const { return mContextLength; }

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
        kCompressFlag = 1 << 4,
        kContextIdOffset = 0,
        kContextIdMask = 0xf << kContextIdOffset,
    };
    uint8_t mFlags;
    uint8_t mContextLength;
} OT_TOOL_PACKED_END;

/**
 * @}
 *
 */

}  // namespace NetworkData
}  // namespace Thread

#endif  // NETWORK_DATA_TLVS_HPP_
