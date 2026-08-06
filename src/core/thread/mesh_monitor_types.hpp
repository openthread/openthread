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

#ifndef OT_CORE_THREAD_MESH_MONITOR_TYPES_HPP_
#define OT_CORE_THREAD_MESH_MONITOR_TYPES_HPP_

#include <openthread/mesh_monitor.h>
#include <openthread/netdiag.h>
#include <openthread/platform/toolchain.h>

#include "coap/coap_message.hpp"
#include "common/as_core_type.hpp"
#include "common/bit_set.hpp"
#include "common/tlvs.hpp"

namespace ot {
namespace MeshMonitor {

OT_TOOL_PACKED_BEGIN
class Tlv : public ot::Tlv
{
public:
    /**
     * Mesh Monitor Server TLV Types.
     */
    enum Type : uint8_t
    {
        kExtAddress              = OT_MESH_MON_TLV_EXT_ADDRESS,
        kMode                    = OT_MESH_MON_TLV_MODE,
        kTimeout                 = OT_MESH_MON_TLV_TIMEOUT,
        kLastHeard               = OT_MESH_MON_TLV_LAST_HEARD,
        kConnectionTime          = OT_MESH_MON_TLV_CONNECTION_TIME,
        kCsl                     = OT_MESH_MON_TLV_CSL,
        kRoute64                 = OT_MESH_MON_TLV_ROUTE64,
        kLinkMarginIn            = OT_MESH_MON_TLV_LINK_MARGIN_IN,
        kMacLinkErrorRatesTx     = OT_MESH_MON_TLV_MAC_LINK_ERROR_RATES_TX,
        kMlEid                   = OT_MESH_MON_TLV_MLEID,
        kIp6AddressList          = OT_MESH_MON_TLV_IP6_ADDRESS_LIST,
        kAlocList                = OT_MESH_MON_TLV_ALOC_LIST,
        kThreadSpecVersion       = OT_MESH_MON_TLV_THREAD_SPEC_VERSION,
        kThreadStackVersion      = OT_MESH_MON_TLV_THREAD_STACK_VERSION,
        kVendorName              = OT_MESH_MON_TLV_VENDOR_NAME,
        kVendorModel             = OT_MESH_MON_TLV_VENDOR_MODEL,
        kVendorSwVersion         = OT_MESH_MON_TLV_VENDOR_SW_VERSION,
        kVendorAppUrl            = OT_MESH_MON_TLV_VENDOR_APP_URL,
        kIp6LinkLocalAddressList = OT_MESH_MON_TLV_IP6_LINK_LOCAL_ADDRESS_LIST,
        kEui64                   = OT_MESH_MON_TLV_EUI64,
        kMacCounters             = OT_MESH_MON_TLV_MAC_COUNTERS,
        kMacLinkErrorRatesRx     = OT_MESH_MON_TLV_MAC_LINK_ERROR_RATES_RX,
        kMleCounters             = OT_MESH_MON_TLV_MLE_COUNTERS,
        kLinkMarginOut           = OT_MESH_MON_TLV_LINK_MARGIN_OUT,
    };

    /**
     * The highest value of any known tlv that can be added to a request set.
     */
    static constexpr uint8_t kDataTlvMax = OT_MESH_MON_DATA_TLV_MAX;

    static const char *TypeToString(Type aType);
    static const char *TypeValueToString(uint8_t aType);

    static bool IsKnownTlv(uint8_t aType);

    static constexpr uint8_t kMaxThreadStackTlvLength     = OT_NETWORK_DIAGNOSTIC_MAX_THREAD_STACK_VERSION_TLV_LENGTH;
    static constexpr uint8_t kMaxVendorNameTlvLength      = OT_NETWORK_DIAGNOSTIC_MAX_VENDOR_NAME_TLV_LENGTH;
    static constexpr uint8_t kMaxVendorModelTlvLength     = OT_NETWORK_DIAGNOSTIC_MAX_VENDOR_MODEL_TLV_LENGTH;
    static constexpr uint8_t kMaxVendorSwVersionTlvLength = OT_NETWORK_DIAGNOSTIC_MAX_VENDOR_SW_VERSION_TLV_LENGTH;
    static constexpr uint8_t kMaxVendorAppUrlTlvLength    = OT_NETWORK_DIAGNOSTIC_MAX_VENDOR_APP_URL_TLV_LENGTH;

} OT_TOOL_PACKED_END;

/**
 * Stores a set of tlvs as a bitmask.
 *
 * Note: Some function can trigger undefined behaviour if bits corresponding to
 * unknown TLVs are set. All functions of this class will only set valid bits,
 * assuming that TlvSets in arguments are themselves valid.
 * If a TlvSet is provided by external code or its validity is in question
 * `FilterAllSupportedTlv` can be used to remove all invalid bits.
 */
class TlvSet : public otMeshMonTlvSet
{
public:
    static constexpr uint16_t kNumBits  = OT_MESH_MON_DATA_TLV_MAX + 1;
    static constexpr uint16_t kMaskSize = OT_MESH_MON_TLV_SET_SIZE;

    /**
     * Bitmask of all supported tlvs.
     */
    static const TlvSet kAllSupportedTlvMask;

private:
    /**
     * Implements a single Request Set header.
     */
    OT_TOOL_PACKED_BEGIN
    class RequestSet : public Clearable<RequestSet>
    {
    public:
        /**
         * Returns the Offset value.
         *
         * @returns The Offset value.
         */
        uint8_t GetOffset(void) const { return mValue >> 4; }

        /**
         * Sets the Offset value.
         *
         * @param[in]  aOffset  The Offset value.
         */
        void SetOffset(uint8_t aOffset) { mValue = static_cast<uint8_t>((mValue & 0x0F) | (aOffset << 4)); }

        /**
         * Returns the Length value.
         *
         * @returns The Length value.
         */
        uint8_t GetLength(void) const { return mValue & 0x0F; }

        /**
         * Sets the Length value.
         *
         * @param[in]  aLength  The Length value.
         */
        void SetLength(uint8_t aLength) { mValue = (mValue & 0xF0) | (aLength & 0x0F); }

    private:
        uint8_t mValue;
    } OT_TOOL_PACKED_END;

    /**
     * Bitmask of all supported TLVs in a host context.
     */
    static const TlvSet kHostSupportedTlvMask;

    /**
     * Bitmask of all supported TLVs in a child context.
     */
    static const TlvSet kChildSupportedTlvMask;

    /**
     * Bitmask of all supported TLVs in a neighbor context.
     */
    static const TlvSet kNeighborSupportedTlvMask;

    /**
     * Bitmask of all supported TLVs which are provided by a MTD child.
     */
    static const TlvSet kMtdChildProvidedTlvMask;

    /**
     * Bitmask of all supported TLVs which are provided by a FTD child.
     */
    static const TlvSet kFtdChildProvidedTlvMask;

public:
    /**
     * Iterator over TLVs in a TlvSet.
     */
    class Iterator
    {
    public:
        Iterator(void)
            : mSet(nullptr)
            , mCurrent(0xFF)
        {
        }

        explicit Iterator(const TlvSet &aSet)
            : mSet(&aSet.AsBitSet())
            , mCurrent(0)
        {
            if (!mSet->Has(0))
            {
                Advance();
            }
        }

        bool      operator==(const Iterator &aOther) const { return mCurrent == aOther.mCurrent; }
        bool      operator!=(const Iterator &aOther) const { return mCurrent != aOther.mCurrent; }
        Tlv::Type operator*(void) const { return static_cast<Tlv::Type>(mCurrent); }
        void      operator++(void) { Advance(); }

    private:
        void Advance(void)
        {
            while (++mCurrent <= Tlv::kDataTlvMax)
            {
                if (mSet->Has(mCurrent))
                {
                    return;
                }
            }
            mCurrent = 0xFF;
        }

        const BitSet<kNumBits> *mSet;
        uint8_t                 mCurrent;
    };

    TlvSet(void) { ClearAllBytes(*this); }

    bool operator==(const TlvSet &aOther) const { return AsBitSet() == aOther.AsBitSet(); }
    bool operator!=(const TlvSet &aOther) const { return AsBitSet() != aOther.AsBitSet(); }

    /**
     * Checks if a TLV is contained in the TlvSet.
     *
     * @param[in]  aType  The TLV to be checked.
     *
     * @retval true   If aType is contained in the TlvSet.
     * @retval false  If aType is not contained in the TlvSet.
     */
    bool IsSet(Tlv::Type aType) const { return AsBitSet().Has(aType); }

    /**
     * Checks if no TLV is set.
     *
     * @retval true   If no TLV is contained in the TlvSet.
     * @retval false  If any TLV is contained in the TlvSet.
     */
    bool IsEmpty(void) const { return AsBitSet().IsEmpty(); }

    /**
     * Checks if all TLVs in some TlvSet are contained in this TlvSet.
     *
     * @param[in]  aOther  The TLVs which need to be contained in this set.
     *
     * @retval true   If all TLVs in aOther are also contained in this TlvSet.
     * @retval false  If not all TLVs in aOther are also contained in this TlvSet.
     */
    bool ContainsAll(const TlvSet &aOther) const { return aOther.AsBitSet().IsSubsetOf(AsBitSet()); }

    /**
     * Adds a TLV to the TlvSet.
     *
     * Does nothing if the TLV is already contained in the set.
     *
     * @param[in]  aType  The TLV to be added.
     */
    void Set(Tlv::Type aType) { AsBitSet().Add(aType); }

    /**
     * Adds a TLV by its raw value to the TlvSet.
     *
     * Performs validity checks and does nothing if the value does not correspond
     * to a known TLV.
     *
     * @param[in]  aValue  The value of the TLV to be added.
     */
    void SetValue(uint8_t aValue);

    /**
     * Adds all TLVs from a different TlvSet to this TlvSet.
     *
     * @param[in]  aOther  The TLVs to be added to this TlvSet.
     */
    void SetAll(const TlvSet &aOther) { AsBitSet().UnionWith(aOther.AsBitSet()); }

    /**
     * Removes all TLVs from this TlvSet which are not contained in the provided
     * TlvSet.
     *
     * @param[in]  aOther  The TLVs which are not removed from this TlvSet.
     */
    void Filter(const TlvSet &aOther) { AsBitSet().IntersectWith(aOther.AsBitSet()); }

    /**
     * Removes all entries in the bitmask which do not correspond to supported TLVs.
     *
     * Having bits of invalid TLVs set can lead to undefined behaviour in some
     * functions. This function should be used whenever a TlvSet is acquired from
     * some unknown source to ensure validity.
     */
    void FilterAllSupportedTlv(void) { Filter(kAllSupportedTlvMask); }

    /**
     * Removes all TLVs from the set which are not supported in a host context.
     */
    void FilterHostSupportedTlv(void) { Filter(kHostSupportedTlvMask); }

    /**
     * Removes all TLVs from the set which are not supported in a child context.
     */
    void FilterChildSupportedTlv(void) { Filter(kChildSupportedTlvMask); }

    /**
     * Removes all TLVs from the set which are not supported in a neighbor context.
     */
    void FilterNeighborSupportedTlv(void) { Filter(kNeighborSupportedTlvMask); }

    /**
     * Removes all TLVs from this TlvSet.
     */
    void Clear(void) { ClearAllBytes(*this); }

    /**
     * Removes the specified TLV from this TlvSet.
     *
     * Does nothing if the TLV is not contained in the TlvSet.
     *
     * @param[in]  aType  The TLV to be removed.
     */
    void Clear(Tlv::Type aType) { AsBitSet().Remove(aType); }

    /**
     * Removes all TLVs from a different TlvSet from this TlvSet.
     *
     * @param[in]  aOther  The TLVs which are to be removed from this TlvSet.
     */
    void ClearAll(const TlvSet &aOther) { AsBitSet().SubtractWith(aOther.AsBitSet()); }

    /**
     * Creates a new TlvSet containing all TLVs from either this set or a provided
     * set.
     *
     * @param[in]  aOther  The TlvSet to join with.
     *
     * @returns  A TlvSet containing all TLVs in either this TlvSet or aOther.
     */
    TlvSet Join(const TlvSet &aOther) const
    {
        TlvSet set = *this;
        set.SetAll(aOther);
        return set;
    }

    /**
     * Creates a new TlvSet containing all TLVs in both this set and a provided
     * set.
     *
     * @param[in]  aOther  The TlvSet to intersect with.
     *
     * @returns  A TlvSet containing all TLVs in both this TlvSet and aOther.
     */
    TlvSet Intersect(const TlvSet &aOther) const
    {
        TlvSet set = *this;
        set.Filter(aOther);
        return set;
    }

    /**
     * Creates a new TlvSet containing all TLVs in this set but not in a provided
     * set.
     *
     * @param[in]  aOther  The TlvSet to cut with.
     *
     * @returns  A TlvSet containing all TLVs in this TlvSet but not in aOther.
     */
    TlvSet Cut(const TlvSet &aOther) const
    {
        TlvSet set = *this;
        set.ClearAll(aOther);
        return set;
    }

    /**
     * Creates a new TlvSet containing all TLVs in this set which are provided
     * by an MTD child.
     *
     * @returns  A TlvSet containing all TLVs in this TlvSet which are provided by an MTD child.
     */
    TlvSet GetMtdChildProvided(void) const { return Intersect(kMtdChildProvidedTlvMask); }

    /**
     * Creates a new TlvSet containing all TLVs in this set which are not provided
     * by an MTD child.
     *
     * @returns  A TlvSet containing all TLVs in this TlvSet which are not provided by an MTD child.
     */
    TlvSet GetNonMtdChildProvided(void) const { return Cut(kMtdChildProvidedTlvMask); }

    /**
     * Creates a new TlvSet containing all TLVs in this set which are provided
     * by an FTD child.
     *
     * @returns  A TlvSet containing all TLVs in this TlvSet which are provided by an FTD child.
     */
    TlvSet GetFtdChildProvided(void) const { return Intersect(kFtdChildProvidedTlvMask); }

    /**
     * Creates a new TlvSet containing all TLVs in this set which are not provided
     * by an FTD child.
     *
     * @returns  A TlvSet containing all TLVs in this TlvSet which are not provided by an FTD child.
     */
    TlvSet GetNonFtdChildProvided(void) const { return Cut(kFtdChildProvidedTlvMask); }

    Iterator begin(void) const { return Iterator(*this); }
    Iterator end(void) const { return Iterator(); }

    /**
     * Converts this TlvSet to a collection of Request Sets and appends them to
     * a Message.
     *
     * @param[in]   aMessage   The message to append the Request Sets to.
     * @param[out]  aSetCount  The number of Request Sets that were appended to the message. Guaranteed to be less or
     * equal to 15.
     *
     * @retval kErrorNone    If the TlvSet was successfully written.
     * @retval kErrorNoBufs  Insufficient available buffers to grow the message.
     */
    Error AppendTo(Message &aMessage, uint8_t &aSetCount) const;

    /**
     * Attempts to read a collection of Request Sets from a Message.
     *
     * All unknown TLVs will be filtered out by this function.
     *
     * @param[in]      aMessage  The message to read the Request Sets from.
     * @param[in,out]  aOffset   The offset into the message at which to start reading. Writes the offset of the first
     * byte after the last Request Set.
     * @param[in]      aSetCount The number of Request Sets that should be read.
     *
     * @retval kErrorNone   If the Request Sets were successfully read.
     * @retval kErrorParse  Failed to parse the request sets.
     */
    Error ReadFrom(const Message &aMessage, uint16_t &aOffset, uint8_t aSetCount);

    /**
     * Checks if a TLV is provided by an MTD child.
     *
     * @param[in]  aType  The TLV to be checked.
     *
     * @retval true   If aType is provided by an MTD child.
     * @retval false  If aType is not provided by an MTD child.
     */
    static bool IsMtdChildProvided(Tlv::Type aType) { return kMtdChildProvidedTlvMask.IsSet(aType); }

    /**
     * Checks if a TLV is provided by an FTD child.
     *
     * @param[in]  aType  The TLV to be checked.
     *
     * @retval true   If aType is provided by an FTD child.
     * @retval false  If aType is not provided by an FTD child.
     */
    static bool IsFtdChildProvided(Tlv::Type aType) { return kFtdChildProvidedTlvMask.IsSet(aType); }

    /**
     * Gets a pointer to the underlying mask bytes.
     *
     * @returns A pointer to the byte array representing the bit mask.
     */
    const uint8_t *GetMaskBytes(void) const { return AsBitSet().GetMaskBytes(); }

private:
    BitSet<kNumBits>       &AsBitSet(void) { return *static_cast<BitSet<kNumBits> *>(static_cast<void *>(mFields)); }
    const BitSet<kNumBits> &AsBitSet(void) const
    {
        return *static_cast<const BitSet<kNumBits> *>(static_cast<const void *>(mFields));
    }
};

static_assert(sizeof(BitSet<TlvSet::kNumBits>) <= sizeof(otMeshMonTlvSet),
              "otMeshMonTlvSet is too small to hold the TlvSet bitmask");
static_assert(sizeof(TlvSet) == sizeof(otMeshMonTlvSet), "TlvSet must not add data members beyond otMeshMonTlvSet");

/**
 * Represents a Device Type used for Device Contexts.
 */
enum DeviceType : uint8_t
{
    kTypeHost     = 0x00,
    kTypeChild    = 0x40,
    kTypeNeighbor = 0x80,
    kTypeUnknown  = 0xC0,
};

/**
 * Bitmask of bits used for the Device Type in request and update device contexts.
 */
constexpr uint8_t kDeviceTypeMask = 0xC0;

const char *DeviceTypeToString(DeviceType aType);

/**
 * Implements the header for a Mesh Monitor Server Request Message.
 */
OT_TOOL_PACKED_BEGIN
class RequestHeader : public Clearable<RequestHeader>
{
private:
    static constexpr uint8_t kQueryFlag        = 0x80;
    static constexpr uint8_t kRegistrationFlag = 0x40;

public:
    /**
     * Returns the Query value.
     *
     * @returns The Query value.
     */
    bool GetQuery(void) const { return (mHeader & kQueryFlag) != 0; }

    /**
     * Sets the Query value.
     *
     * @param[in]  aQuery  The Query value.
     */
    void SetQuery(bool aQuery);

    /**
     * Returns the Registration value.
     *
     * @returns The Registration value.
     */
    bool GetRegistration(void) const { return (mHeader & kRegistrationFlag) != 0; }

    /**
     * Sets the Registration value.
     *
     * @param[in]  aRegistration  The Registration value.
     */
    void SetRegistration(bool aRegistration);

private:
    uint8_t mHeader;
} OT_TOOL_PACKED_END;

/**
 * Implements the header for a Request Context.
 */
OT_TOOL_PACKED_BEGIN
class RequestContext : public Clearable<RequestContext>
{
private:
    static constexpr uint8_t kRequestSetCountMask = 0x0F;

public:
    /**
     * Returns the Device Type value.
     *
     * @returns The Device Type value.
     */
    DeviceType GetType(void) const { return static_cast<DeviceType>(mTypeCount & kDeviceTypeMask); }

    /**
     * Sets the Device Type value.
     *
     * @param[in]  aType  The Device Type value.
     */
    void SetType(DeviceType aType) { mTypeCount = (mTypeCount & ~kDeviceTypeMask) | aType; }

    /**
     * Returns the Request Set Count value.
     *
     * @returns The Request Set Count value.
     */
    uint8_t GetRequestSetCount(void) const { return mTypeCount & kRequestSetCountMask; }

    /**
     * Sets the Request Set Count value.
     *
     * @param[in]  aCount  The Request Set Count value.
     */
    void SetRequestSetCount(uint8_t aCount) { mTypeCount = (mTypeCount & ~kRequestSetCountMask) | aCount; }

    /**
     * Returns the Length value.
     *
     * @returns The Length value.
     */
    uint16_t GetLength(void) const { return BigEndian::HostSwap16(mLength); }

    /**
     * Sets the Length value.
     *
     * @param[in]  aLength  The Length value.
     */
    void SetLength(uint16_t aLength) { mLength = BigEndian::HostSwap16(aLength); }

private:
    uint8_t  mTypeCount;
    uint16_t mLength;
} OT_TOOL_PACKED_END;

/**
 * Represents the Update Mode of a Update Device Context.
 */
enum UpdateMode : uint8_t
{
    kModeUpdated = 0x00,
    kModeRemoved = 0x40,
    kModeAdded   = 0x80,
    kModeUnknown = 0xC0,
};

/**
 * Bitmask of bits used for the Update Mode in update device contexts.
 */
static constexpr uint8_t kUpdateModeMask = 0xC0;

const char *UpdateModeToString(UpdateMode aMode);
uint8_t     UpdateModeToApiValue(UpdateMode aMode);

/**
 * Common part of a update device context header.
 */
OT_TOOL_PACKED_BEGIN
class Context
{
public:
    static constexpr uint16_t kMaxLength = 0x3FFF;

    void Init(void) { mTypeLength = 0; }

    /**
     * Returns the Device Type value.
     *
     * @returns The Device Type value.
     */
    DeviceType GetType(void) const { return static_cast<DeviceType>(mTypeLength & kDeviceTypeMask); }

    /**
     * Sets the Device Type value.
     *
     * @param[in]  aType  The Device Type value.
     */
    void SetType(DeviceType aType) { mTypeLength = (mTypeLength & ~kDeviceTypeMask) | aType; }

    /**
     * Returns the Length value.
     *
     * @returns  The Length value.
     */
    uint16_t GetLength(void) const { return static_cast<uint16_t>(((mTypeLength & ~kDeviceTypeMask) << 8) | mLength); }

    /**
     * Sets the Length value.
     *
     * @param[in]  aLength  The Length value.
     */
    void SetLength(uint16_t aLength)
    {
        OT_ASSERT(aLength <= kMaxLength);
        mLength     = aLength & 0xFF;
        mTypeLength = (mTypeLength & kDeviceTypeMask) | (aLength >> 8);
    }

private:
    uint8_t mTypeLength;
    uint8_t mLength;
} OT_TOOL_PACKED_END;

/**
 * Implements the child update device context header.
 */
OT_TOOL_PACKED_BEGIN
class ChildContext : public Context
{
private:
    static constexpr uint8_t kLegacyFlag = 0x20;

    static constexpr uint8_t kIdMiscMask = 0x01;

public:
    void Init(void)
    {
        Context::Init();
        mMisc = 0;
        mId   = 0;
    }

    /**
     * Returns the Update Mode value.
     *
     * @returns The Update Mode value.
     */
    UpdateMode GetUpdateMode(void) const { return static_cast<UpdateMode>(mMisc & kUpdateModeMask); }

    /**
     * Sets the Update Mode value.
     *
     * @param[in]  aMode  The Update Mode value.
     */
    void SetUpdateMode(UpdateMode aMode) { mMisc = (mMisc & ~kUpdateModeMask) | aMode; }

    /**
     * Returns the Legacy value.
     *
     * @returns The Legacy value.
     */
    bool GetLegacy(void) const { return (mMisc & kLegacyFlag) != 0; }

    /**
     * Sets the Legacy value.
     *
     * @param[in]  aLegacy  The Legacy value.
     */
    void SetLegacy(bool aLegacy)
    {
        if (aLegacy)
        {
            mMisc |= kLegacyFlag;
        }
        else
        {
            mMisc &= ~kLegacyFlag;
        }
    }

    /**
     * Returns the Id value.
     *
     * @returns  The Id value.
     */
    uint16_t GetId(void) const { return static_cast<uint16_t>(((mMisc & kIdMiscMask) << 8) | mId); }

    /**
     * Sets the Id value.
     *
     * @param[in]  aId  The Id value.
     */
    void SetId(uint16_t aId)
    {
        OT_ASSERT(aId <= Mle::kMaxChildId);
        mId   = aId & 0xFF;
        mMisc = (mMisc & ~kIdMiscMask) | (aId >> 8);
    }

private:
    uint8_t mMisc;
    uint8_t mId;
} OT_TOOL_PACKED_END;

/**
 * Implements the neighbor update device context header.
 */
OT_TOOL_PACKED_BEGIN
class NeighborContext : public Context
{
private:
    static constexpr uint8_t kIdMask = 0x3F;

public:
    void Init(void)
    {
        Context::Init();
        mModeId = 0;
    }

    /**
     * Returns the Update Mode value.
     *
     * @returns The Update Mode value.
     */
    UpdateMode GetUpdateMode(void) const { return static_cast<UpdateMode>(mModeId & kUpdateModeMask); }

    /**
     * Sets the Update Mode value.
     *
     * @param[in]  aMode  The Update Mode value.
     */
    void SetUpdateMode(UpdateMode aMode) { mModeId = (mModeId & ~kUpdateModeMask) | aMode; }

    /**
     * Returns the Id value.
     *
     * @returns  The Id value.
     */
    uint8_t GetId(void) const { return (mModeId & kIdMask); }

    /**
     * Sets the Id value.
     *
     * @param[in]  aId  The Id value.
     */
    void SetId(uint8_t aId) { mModeId = (mModeId & ~kIdMask) | aId; }

private:
    uint8_t mModeId;
} OT_TOOL_PACKED_END;

/**
 * Implements a update message header.
 */
class UpdateHeader
{
private:
    static constexpr uint8_t kCompleteFlag = 0x80;
    static constexpr uint8_t kFullSeqFlag  = 0x40;
    static constexpr uint8_t kRouterIdMask = 0x3F;

public:
    void Init(void);

    /**
     * Returns the Complete value.
     *
     * @returns The Complete value.
     */
    bool GetComplete(void) const { return (mMeta & kCompleteFlag) != 0; }

    /**
     * Sets the Complete value.
     *
     * @param[in]  aComplete  The Complete value.
     */
    void SetComplete(bool aComplete);

    /**
     * Returns the Router Id value.
     *
     * @returns The Router Id value.
     */
    uint8_t GetRouterId(void) const { return mMeta & kRouterIdMask; }

    /**
     * Sets the Router Id value.
     *
     * @param[in]  aRouterId  The Router Id value.
     */
    void SetRouterId(uint8_t aRouterId) { mMeta = (mMeta & ~kRouterIdMask) | (aRouterId & kRouterIdMask); }

    /**
     * Returns whether the header contains the full sequence number or only the
     * 8 least significant bits.
     *
     * @retval true   The header contains the full sequence number.
     * @retval false  The header contains only the 8 least significant bits of the sequence number.
     */
    bool HasFullSeqNumber(void) const { return (mMeta & kFullSeqFlag) != 0; }

    /**
     * Returns the full sequence number.
     *
     * The higher order bits will be set to 0 if the header does not contain
     * the full sequence number.
     *
     * @returns The full sequence number.
     */
    uint64_t GetFullSeqNumber(void) const { return mSeqNumber; }

    /**
     * Sets the sequence number and configures the header to contain the full
     * sequence number.
     *
     * @param[in]  aSeqNumber  The sequence number.
     */
    void SetFullSeqNumber(uint64_t aSeqNumber);

    /**
     * Returns the 8 least significant bits of the sequence number.
     *
     * @returns The 8 least significant bits of the sequence number.
     */
    uint8_t GetShortSeqNumber(void) const { return static_cast<uint8_t>(mSeqNumber); }

    /**
     * Sets the sequence number and configures the header to contain only the
     * 8 least significant bits.
     *
     * @param[in]  aSeqNumber  The sequence number.
     */
    void SetShortSeqNumber(uint64_t aSeqNumber);

    /**
     * Returns the length of the header as currently configured in bytes.
     *
     * @returns The current length of the header in bytes.
     */
    uint16_t GetLength(void) const;

    /**
     * Attempts to read the header from a message a the message offset.
     *
     * @param[in]  aMessage  The message to read the header from.
     *
     * @retval kErrorNone   If the header was successfully read.
     * @retval kErrorParse  Failed to parse the header.
     */
    Error ReadFrom(const Message &aMessage) { return ReadFrom(aMessage, aMessage.GetOffset()); }

    /**
     * Attempts to read the header from a message at a specified offset.
     *
     * @param[in]  aMessage  The message to read the header from.
     * @param[in]  aOffset   The offset at which to start reading the header.
     *
     * @retval kErrorNone   If the header was successfully read.
     * @retval kErrorParse  Failed to parse the header.
     */
    Error ReadFrom(const Message &aMessage, uint16_t aOffset);

    /**
     * Writes the header to the message at a specified offset.
     *
     * @param[in]  aMessage  The message to write the header to.
     * @param[in]  aOffset   The offset at which to start writing the header at.
     */
    void WriteTo(Message &aMessage, uint16_t aOffset) const;

    /**
     * Attempts to append the header to a message.
     *
     * @param[in]  aMessage  The message to append the header to.
     *
     * @retval kErrorNone    If the header was successfully appended.
     * @retval kErrorNoBufs  Insufficient available buffers to grow the message.
     */
    Error AppendTo(Message &aMessage) const;

private:
    uint64_t mSeqNumber;
    uint8_t  mMeta;
};

/**
 * Implements the header for a child request message.
 */
OT_TOOL_PACKED_BEGIN
class ChildRequestHeader : public Clearable<ChildRequestHeader>
{
private:
    static constexpr uint8_t kCommandMask         = 0xC0;
    static constexpr uint8_t kQueryFlag           = 0x20;
    static constexpr uint8_t kRequestSetCountMask = 0x0F;

public:
    /**
     * Represents a Child Command.
     */
    enum ChildCommand : uint8_t
    {
        kNone  = 0x00,
        kStart = 0x40,
        kStop  = 0x80,
    };

    /**
     * Returns the Command value.
     *
     * @returns The Command value.
     */
    ChildCommand GetCommand(void) const { return static_cast<ChildCommand>(mHeader & kCommandMask); }

    /**
     * Sets the Command value.
     *
     * @param[in]  aCommand  The Command value.
     */
    void SetCommand(ChildCommand aCommand) { mHeader = (mHeader & ~kCommandMask) | aCommand; }

    /**
     * Returns the Query value.
     *
     * @returns The Query value.
     */
    bool GetQuery(void) const { return (mHeader & kQueryFlag) != 0; }

    /**
     * Sets the Query value.
     *
     * @param[in]  aQuery  The Query value.
     */
    void SetQuery(bool aQuery);

    /**
     * Returns the Request Set Count value.
     *
     * @returns The Request Set Count value.
     */
    uint8_t GetRequestSetCount(void) const { return mHeader & kRequestSetCountMask; }

    /**
     * Sets the Request Set Count value.
     *
     * @param[in]  aCount  The Request Set Count value.
     */
    void SetRequestSetCount(uint8_t aCount) { mHeader = (mHeader & ~kRequestSetCountMask) | aCount; }

private:
    uint8_t mHeader;
} OT_TOOL_PACKED_END;

} // namespace MeshMonitor

DefineCoreType(otMeshMonTlvSet, MeshMonitor::TlvSet);

} // namespace ot

#endif // OT_CORE_THREAD_MESH_MONITOR_TYPES_HPP_
