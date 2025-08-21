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

#ifndef DIAGNOSTIC_SERVER_TYPES_HPP_
#define DIAGNOSTIC_SERVER_TYPES_HPP_

#include <openthread/diag_server.h>
#include <openthread/platform/toolchain.h>

#include "coap/coap_message.hpp"
#include "common/tlvs.hpp"

namespace ot {
namespace DiagnosticServer {

OT_TOOL_PACKED_BEGIN
class Tlv : public ot::Tlv
{
public:
    /**
     * Diagnostic Server TLV Types.
     */
    enum Type : uint8_t
    {
        kMacAddress     = OT_DIAG_SERVER_TLV_MAC_ADDRESS,
        kMode           = OT_DIAG_SERVER_TLV_MODE,
        kTimeout        = OT_DIAG_SERVER_TLV_TIMEOUT,
        kLastHeard      = OT_DIAG_SERVER_TLV_LAST_HEARD,
        kConnectionTime = OT_DIAG_SERVER_TLV_CONNECTION_TIME,
        kCsl            = OT_DIAG_SERVER_TLV_CSL,
        kRoute64        = OT_DIAG_SERVER_TLV_ROUTE64,
        kLinkMarginIn   = OT_DIAG_SERVER_TLV_LINK_MARGIN_IN,

        kMacLinkErrorRatesOut = OT_DIAG_SERVER_TLV_MAC_LINK_ERROR_RATES_OUT,
        kMlEid                = OT_DIAG_SERVER_TLV_MLEID,
        kIp6AddressList       = OT_DIAG_SERVER_TLV_IP6_ADDRESS_LIST,
        kAlocList             = OT_DIAG_SERVER_TLV_ALOC_LIST,

        kThreadSpecVersion       = OT_DIAG_SERVER_TLV_THREAD_SPEC_VERSION,
        kThreadStackVersion      = OT_DIAG_SERVER_TLV_THREAD_STACK_VERSION,
        kVendorName              = OT_DIAG_SERVER_TLV_VENDOR_NAME,
        kVendorModel             = OT_DIAG_SERVER_TLV_VENDOR_MODEL,
        kVendorSwVersion         = OT_DIAG_SERVER_TLV_VENDOR_SW_VERSION,
        kVendorAppUrl            = OT_DIAG_SERVER_TLV_VENDOR_APP_URL,
        kIp6LinkLocalAddressList = OT_DIAG_SERVER_TLV_IP6_LINK_LOCAL_ADDRESS_LIST,
        kEui64                   = OT_DIAG_SERVER_TLV_EUI64,

        kMacCounters         = OT_DIAG_SERVER_TLV_MAC_COUNTERS,
        kMacLinkErrorRatesIn = OT_DIAG_SERVER_TLV_MAC_LINK_ERROR_RATES_IN,
        kMleCounters         = OT_DIAG_SERVER_TLV_MLE_COUNTERS,
        kLinkMarginOut       = OT_DIAG_SERVER_TLV_LINK_MARGIN_OUT,
    };

    /**
     * The highest value of any known tlv that can be added to a request set.
     */
    static constexpr uint8_t kDataTlvMax = OT_DIAG_SERVER_DATA_TLV_MAX;

    static const char *TypeToString(Type aType);
    static const char *TypeValueToString(uint8_t aType);

    static bool IsKnownTlv(uint8_t aType);

    static constexpr uint8_t kMaxThreadStackTlvLength     = OT_DIAG_SERVER_MAX_THREAD_STACK_VERSION_TLV_LENGTH;
    static constexpr uint8_t kMaxVendorNameTlvLength      = OT_DIAG_SERVER_MAX_VENDOR_NAME_TLV_LENGTH;
    static constexpr uint8_t kMaxVendorModelTlvLength     = OT_DIAG_SERVER_MAX_VENDOR_MODEL_TLV_LENGTH;
    static constexpr uint8_t kMaxVendorSwVersionTlvLength = OT_DIAG_SERVER_MAX_VENDOR_SW_VERSION_TLV_LENGTH;
    static constexpr uint8_t kMaxVendorAppUrlTlvLength    = OT_DIAG_SERVER_MAX_VENDOR_APP_URL_TLV_LENGTH;

    /**
     * Bitmask of all known tlvs.
     */
    static constexpr otDiagServerTlvSet kKnownTlvMask{{
        {(1U << Tlv::kMacAddress) | (1U << Tlv::kMode) | (1U << Tlv::kTimeout) | (1U << Tlv::kLastHeard) |
             (1U << Tlv::kConnectionTime) | (1U << Tlv::kCsl) | (1U << Tlv::kRoute64) | (1U << Tlv::kLinkMarginIn),
         (1U << (Tlv::kMacLinkErrorRatesOut - 8U)) | (1U << (Tlv::kMlEid - 8U)) | (1U << (Tlv::kIp6AddressList - 8U)) |
             (1U << (Tlv::kAlocList - 8U)),
         (1U << (Tlv::kThreadSpecVersion - 16U)) | (1U << (Tlv::kThreadStackVersion - 16U)) |
             (1U << (Tlv::kVendorName - 16U)) | (1U << (Tlv::kVendorModel - 16U)) |
             (1U << (Tlv::kVendorSwVersion - 16U)) | (1U << (Tlv::kVendorAppUrl - 16U)) |
             (1U << (Tlv::kIp6LinkLocalAddressList - 16U)) | (1U << (Tlv::kEui64 - 16U)),
         (1U << (Tlv::kMacCounters - 24U)) | (1U << (Tlv::kMacLinkErrorRatesIn - 24U)) |
             (1U << (Tlv::kMleCounters - 24U)) | (1U << (Tlv::kLinkMarginOut - 24U))},
    }};
} OT_TOOL_PACKED_END;

/**
 * Stores a set of tlvs as a bitmask.
 *
 * Note: Some function can trigger undefined behaviour if bits coresponding to
 * unknwon TLVs are set. All functions of this class will only set valid bits,
 * assuming that TlvSets in arguments are themselves valid.
 * If a TlvSet is provided by external code or its validity is in question
 * `FilterValid` can be used to remove all invalid bits.
 */
class TlvSet : public otDiagServerTlvSet
{
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
        void SetOffset(uint8_t aOffset) { mValue = (mValue & 0x0F) | (aOffset << 4); }

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
     * Bitmask of all know TLVs which are valid in a host context.
     */
    static constexpr otDiagServerTlvSet kHostValidMask{{
        {(1U << Tlv::kMacAddress) | (1U << Tlv::kMode) | (1U << Tlv::kRoute64),
         (1U << (Tlv::kMlEid - 8U)) | (1U << (Tlv::kIp6AddressList - 8U)) | (1U << (Tlv::kAlocList - 8U)),
         (1U << (Tlv::kThreadSpecVersion - 16U)) | (1U << (Tlv::kThreadStackVersion - 16U)) |
             (1U << (Tlv::kVendorName - 16U)) | (1U << (Tlv::kVendorModel - 16U)) |
             (1U << (Tlv::kVendorSwVersion - 16U)) | (1U << (Tlv::kVendorAppUrl - 16U)) |
             (1U << (Tlv::kIp6LinkLocalAddressList - 16U)) | (1U << (Tlv::kEui64 - 16U)),
         (1U << (Tlv::kMacCounters - 24U)) | (1U << (Tlv::kMleCounters - 24U))},
    }};

    /**
     * Bitmask of all known TLVs whihc are valid in a child context.
     */
    static constexpr otDiagServerTlvSet kChildValidMask{{
        {(1U << Tlv::kMacAddress) | (1U << Tlv::kMode) | (1U << Tlv::kTimeout) | (1U << Tlv::kLastHeard) |
             (1U << Tlv::kConnectionTime) | (1U << Tlv::kCsl) | (1U << Tlv::kLinkMarginIn),
         (1U << (Tlv::kMacLinkErrorRatesOut - 8U)) | (1U << (Tlv::kMlEid - 8U)) | (1U << (Tlv::kIp6AddressList - 8U)) |
             (1U << (Tlv::kAlocList - 8U)),
         (1U << (Tlv::kThreadSpecVersion - 16U)) | (1U << (Tlv::kThreadStackVersion - 16U)) |
             (1U << (Tlv::kVendorName - 16U)) | (1U << (Tlv::kVendorModel - 16U)) |
             (1U << (Tlv::kVendorSwVersion - 16U)) | (1U << (Tlv::kVendorAppUrl - 16U)) |
             (1U << (Tlv::kIp6LinkLocalAddressList - 16U)) | (1U << (Tlv::kEui64 - 16U)),
         (1U << (Tlv::kMacCounters - 24U)) | (1U << (Tlv::kMacLinkErrorRatesIn - 24U)) |
             (1U << (Tlv::kMleCounters - 24U)) | (1U << (Tlv::kLinkMarginOut - 24U))},
    }};

    /**
     * Bitmask of all known TLVs which are provided by a MTD child.
     */
    static constexpr otDiagServerTlvSet kChildProvidedMtdMask{{
        {0, 0,
         (1U << (Tlv::kThreadStackVersion - 16U)) | (1U << (Tlv::kVendorName - 16U)) |
             (1U << (Tlv::kVendorModel - 16U)) | (1U << (Tlv::kVendorSwVersion - 16U)) |
             (1U << (Tlv::kVendorAppUrl - 16U)) | (1U << (Tlv::kIp6LinkLocalAddressList - 16U)) |
             (1U << (Tlv::kEui64 - 16U)),
         (1U << (Tlv::kMacCounters - 24U)) | (1U << (Tlv::kMacLinkErrorRatesIn - 24U)) |
             (1U << (Tlv::kMleCounters - 24U)) | (1U << (Tlv::kLinkMarginOut - 24U))},
    }};

    /**
     * Bitmask of all known TLVs which are provided by a FTD child.
     */
    static constexpr otDiagServerTlvSet kChildProvidedFtdMask{{
        {kChildProvidedMtdMask.m8[0],
         kChildProvidedMtdMask.m8[1] | (1U << (Tlv::kMlEid - 8U)) | (1U << (Tlv::kIp6AddressList - 8U)) |
             (1U << (Tlv::kAlocList - 8U)),
         kChildProvidedMtdMask.m8[2], kChildProvidedMtdMask.m8[3]},
    }};

    /**
     * Bitmask of all known TLVs whihc are valid in a neighbor context.
     */
    static constexpr otDiagServerTlvSet kNeighborValidMask{{
        {(1U << Tlv::kMacAddress) | (1U << Tlv::kLastHeard) | (1U << Tlv::kConnectionTime) | (1U << Tlv::kLinkMarginIn),
         (1U << (Tlv::kMacLinkErrorRatesOut - 8U)), (1U << (Tlv::kThreadSpecVersion - 16U)), 0U},
    }};

public:
    /**
     * Iterator over TLVs in a TlvSet.
     */
    class Iterator
    {
    public:
        Iterator(void)
            : mCurrent{0xFF}
            , mState{}
        {
        }

        Iterator(otDiagServerTlvSet aState);

        bool operator==(const Iterator &aOther) const { return mCurrent == aOther.mCurrent; }
        bool operator!=(const Iterator &aOther) const { return mCurrent != aOther.mCurrent; }

        Tlv::Type operator*(void) const { return static_cast<Tlv::Type>(mCurrent); }

        void Advance(void);
        void operator++(void) { Advance(); }

    private:
        uint8_t            mCurrent;
        otDiagServerTlvSet mState;
    };

    bool operator==(const TlvSet &aOther) const
    {
        bool eq = true;
        for (size_t i = 0; i < (OT_DIAG_SERVER_TLV_SET_SIZE / sizeof(uint32_t)); i++)
        {
            eq &= m32[i] == aOther.m32[i];
        }
        return eq;
    }

    bool operator!=(const TlvSet &aOther) const
    {
        bool neq = true;
        for (size_t i = 0; i < (OT_DIAG_SERVER_TLV_SET_SIZE / sizeof(uint32_t)); i++)
        {
            neq &= m32[i] != aOther.m32[i];
        }
        return neq;
    }

    /**
     * Checks if a TLV is contained in the TlvSet.
     *
     * @param[in]  aType  The TLV to be checked.
     *
     * @retval true   If aType is contained in the TlvSet.
     * @retval false  If aType is not contained in the TlvSet.
     */
    bool IsSet(Tlv::Type aType) const { return (m8[aType / 8U] & (1U << (aType % 8U))) != 0; }

    /**
     * Checks if no TLV is set.
     *
     * @retval true   If no TLV is contained in the TlvSet.
     * @retval false  If any TLV is contained in the TlvSet.
     */
    bool IsEmpty(void) const
    {
        bool empty = true;
        for (size_t i = 0; i < (OT_DIAG_SERVER_TLV_SET_SIZE / sizeof(uint32_t)); i++)
        {
            empty &= m32[i] == 0;
        }
        return empty;
    }

    /**
     * Checks if all TLVs in some TlvSet are contained in this TlvSet.
     *
     * @param[in]  aOther  The TLVs which need to be contained in this set.
     *
     * @retval true   If all TLVs in aOther are also contained in this TlvSet.
     * @retval false  If not all TLVs in aOther are also contained in this TlvSet.
     */
    bool ContainsAll(const TlvSet &aOther)
    {
        bool contains = true;
        for (size_t i = 0; i < (OT_DIAG_SERVER_TLV_SET_SIZE / sizeof(uint32_t)); i++)
        {
            contains &= (m32[i] & aOther.m32[i]) == aOther.m32[i];
        }
        return contains;
    }

    /**
     * Adds a TLV to the TlvSet.
     *
     * Does nothing if the TLV is already contained in the set.
     *
     * @param[in]  aType  The TLV to be added.
     */
    void Set(Tlv::Type aType) { m8[aType / 8U] |= (1U << (aType % 8U)); }

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
    void SetAll(const TlvSet &aOther)
    {
        for (size_t i = 0; i < (OT_DIAG_SERVER_TLV_SET_SIZE / sizeof(uint32_t)); i++)
        {
            m32[i] |= aOther.m32[i];
        }
    }

    /**
     * Removes all TLVs from this TlvSet which are not contained in the provided
     * TlvSet.
     *
     * @param[in]  aOther  The TLVs which are not removed from this TlvSet.
     */
    void Filter(const TlvSet &aOther)
    {
        for (size_t i = 0; i < (OT_DIAG_SERVER_TLV_SET_SIZE / sizeof(uint32_t)); i++)
        {
            m32[i] &= aOther.m32[i];
        }
    }

    /**
     * Removes all entries in the bitmask which do not correspond to known TLVs.
     *
     * Having bits of invalid TLVs set can lead to undefinied behaviour in some
     * functions. This function should be used whenever a TlvSet is acquired from
     * some unknown source to ensure validity.
     */
    void FilterValid(void) { Filter(static_cast<const TlvSet &>(Tlv::kKnownTlvMask)); }

    /**
     * Removes all TLVs from the set which are not valid in a host context.
     */
    void FilterHostValid(void) { Filter(static_cast<const TlvSet &>(kHostValidMask)); }

    /**
     * Removes all TLVs from the set which are not valid in a child context.
     */
    void FilterChildValid(void) { Filter(static_cast<const TlvSet &>(kChildValidMask)); }

    /**
     * Removes all TLVs from the set which are not valid in a neighbor context.
     */
    void FilterNeighborValid(void) { Filter(static_cast<const TlvSet &>(kNeighborValidMask)); }

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
    void Clear(Tlv::Type aType) { m8[aType / 8U] &= ~(1U << (aType % 8U)); }

    /**
     * Removes all TLVs from a different TlvSet from this TlvSet.
     *
     * @param[in]  aOther  The TLVs which are to be removed from this TlvSet.
     */
    void ClearAll(const TlvSet &aOther)
    {
        for (size_t i = 0; i < (OT_DIAG_SERVER_TLV_SET_SIZE / sizeof(uint32_t)); i++)
        {
            m32[i] &= ~aOther.m32[i];
        }
    }

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
        TlvSet set;
        for (size_t i = 0; i < (OT_DIAG_SERVER_TLV_SET_SIZE / sizeof(uint32_t)); i++)
        {
            set.m32[i] = m32[i] | aOther.m32[i];
        }
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
        TlvSet set;
        for (size_t i = 0; i < (OT_DIAG_SERVER_TLV_SET_SIZE / sizeof(uint32_t)); i++)
        {
            set.m32[i] = m32[i] & aOther.m32[i];
        }
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
        TlvSet set;
        for (size_t i = 0; i < (OT_DIAG_SERVER_TLV_SET_SIZE / sizeof(uint32_t)); i++)
        {
            set.m32[i] = m32[i] & ~aOther.m32[i];
        }
        return set;
    }

    /**
     * Creates a new TlvSet containing all TLVs in this set which are provided
     * by an MTD child.
     *
     * @returns  A TlvSet containing all TLVs in this TlvSet which are provided by an MTD child.
     */
    TlvSet GetChildProvidedMtd(void) const { return Intersect(static_cast<const TlvSet &>(kChildProvidedMtdMask)); }

    /**
     * Creates a new TlvSet containing all TLVs in this set which are not provided
     * by an MTD child.
     *
     * @returns  A TlvSet containing all TLVs in this TlvSet which are not provided by an MTD child.
     */
    TlvSet GetNotChildProvidedMtd(void) const { return Cut(static_cast<const TlvSet &>(kChildProvidedMtdMask)); }

    /**
     * Creates a new TlvSet containing all TLVs in this set which are provided
     * by an FTD child.
     *
     * @returns  A TlvSet containing all TLVs in this TlvSet which are provided by an FTD child.
     */
    TlvSet GetChildProvidedFtd(void) const { return Intersect(static_cast<const TlvSet &>(kChildProvidedFtdMask)); }

    /**
     * Creates a new TlvSet containing all TLVs in this set which are not provided
     * by an FTD child.
     *
     * @returns  A TlvSet containing all TLVs in this TlvSet which are not provided by an FTD child.
     */
    TlvSet GetNotChildProvidedFtd(void) const { return Cut(static_cast<const TlvSet &>(kChildProvidedFtdMask)); }

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
    static bool IsChildProvidedMtd(Tlv::Type aType)
    {
        return static_cast<const TlvSet &>(kChildProvidedMtdMask).IsSet(aType);
    }

    /**
     * Checks if a TLV is provided by an FTD child.
     *
     * @param[in]  aType  The TLV to be checked.
     *
     * @retval true   If aType is provided by an FTD child.
     * @retval false  If aType is not provided by an FTD child.
     */
    static bool IsChildProvidedFtd(Tlv::Type aType)
    {
        return static_cast<const TlvSet &>(kChildProvidedFtdMask).IsSet(aType);
    }
};

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
 * Implements the header for a Diagnostic Server Request Message.
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
    uint16_t GetLength(void) const { return mLength; }

    /**
     * Sets the Length value.
     *
     * @param[in]  aLength  The Length value.
     */
    void SetLength(uint16_t aLength) { mLength = aLength; }

private:
    uint8_t  mTypeCount;
    uint16_t mLength;
} OT_TOOL_PACKED_END;

/**
 * Represents the Update Mode of a Update Device Context.
 */
enum UpdateMode : uint8_t
{
    kModeUpdate  = 0x00,
    kModeRemove  = 0x40,
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
    uint16_t GetLength(void) const { return (static_cast<uint16_t>(mTypeLength & ~kDeviceTypeMask) << 8) | mLength; }

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
    uint16_t GetId(void) const { return (static_cast<uint16_t>(mMisc & kIdMiscMask) << 8) | mId; }

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
    void SetRouterId(uint8_t aRouterId) { mMeta |= aRouterId; }

    /**
     * Returns wether the header contains the full sequence number or only the
     * 8 least signifficant bits.
     *
     * @retval true   The header contains the full sequence number.
     * @retval false  The header contains only the 8 least signifficant bits of the sequence number.
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
    uint64_t GetSeqNumberFull(void) const { return mSeqNumber; }

    /**
     * Sets the sequence number and configures the header to contain the full
     * sequence number.
     *
     * @param[in]  aSeqNumber  The sequence number.
     */
    void SetSeqNumberFull(uint64_t aSeqNumber);

    /**
     * Returns the 8 least signifficant bits of the sequence number.
     *
     * @returns The 8 least signifficant bits of the sequence number.
     */
    uint8_t GetSeqNumberShort(void) const { return static_cast<uint8_t>(mSeqNumber); }

    /**
     * Sets the sequence number and configures the header to contain only the
     * 8 least signifficant bits.
     *
     * @param[in]  aSeqNumber  The sequence number.
     */
    void SetSeqNumberShort(uint64_t aSeqNumber);

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
     * @retval kErrorNone    If the hedaer was successfully appended.
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

} // namespace DiagnosticServer

DefineCoreType(otDiagServerTlvSet, DiagnosticServer::TlvSet);

} // namespace ot

#endif // DIAGNOSTIC_SERVER_TYPES_HPP_
