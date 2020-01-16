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
 *   This file includes definitions for MAC types such as Address, Extended PAN Identifier, Network Name, etc.
 */

#ifndef MAC_TYPES_HPP_
#define MAC_TYPES_HPP_

#include "openthread-core-config.h"

#include <stdint.h>
#include <string.h>

#include <openthread/link.h>

#include "common/string.hpp"

namespace ot {
namespace Mac {

/**
 * @addtogroup core-mac
 *
 * @{
 *
 */

enum
{
    kShortAddrBroadcast = 0xffff,
    kShortAddrInvalid   = 0xfffe,
    kPanIdBroadcast     = 0xffff,
};

/**
 * This type represents the IEEE 802.15.4 PAN ID.
 *
 */
typedef otPanId PanId;

/**
 * This type represents the IEEE 802.15.4 Short Address.
 *
 */
typedef otShortAddress ShortAddress;

/**
 * This function generates a random IEEE 802.15.4 PAN ID.
 *
 * @returns A randomly generated IEEE 802.15.4 PAN ID (excluding `kPanIdBroadcast`).
 *
 */
PanId GenerateRandomPanId(void);

/**
 * This structure represents an IEEE 802.15.4 Extended Address.
 *
 */
OT_TOOL_PACKED_BEGIN
class ExtAddress : public otExtAddress
{
public:
    enum
    {
        kInfoStringSize = 17, // Max chars for the info string (`ToString()`).
    };

    /**
     * This type defines the fixed-length `String` object returned from `ToString()`.
     *
     */
    typedef String<kInfoStringSize> InfoString;

    /**
     * This enumeration type specifies the copy byte order when Extended Address is being copied to/from a buffer.
     *
     */
    enum CopyByteOrder
    {
        kNormalByteOrder,  // Copy address bytes in normal order (as provided in array buffer).
        kReverseByteOrder, // Copy address bytes in reverse byte order.
    };

    /**
     * This method clears the Extended Address (sets all bytes to zero).
     *
     */
    void Clear(void) { Fill(0); }

    /**
     * This method fills all bytes of address with a given byte value.
     *
     * @param[in] aByte A byte value to fill address with.
     *
     */
    void Fill(uint8_t aByte) { memset(this, aByte, sizeof(*this)); }

    /**
     * This method generates a random IEEE 802.15.4 Extended Address.
     *
     */
    void GenerateRandom(void);

    /**
     * This method sets the Extended Address from a given byte array.
     *
     * @param[in] aBuffer    Pointer to an array containing the Extended Address. `OT_EXT_ADDRESS_SIZE` bytes from
     *                       buffer are copied to form the Extended Address.
     * @param[in] aByteOrder The byte order to use when copying the address.
     *
     */
    void Set(const uint8_t *aBuffer, CopyByteOrder aByteOrder = kNormalByteOrder)
    {
        CopyAddress(m8, aBuffer, aByteOrder);
    }

    /**
     * This method indicates whether or not the Group bit is set.
     *
     * @retval TRUE   If the group bit is set.
     * @retval FALSE  If the group bit is not set.
     *
     */
    bool IsGroup(void) const { return (m8[0] & kGroupFlag) != 0; }

    /**
     * This method sets the Group bit.
     *
     * @param[in]  aGroup  TRUE if group address, FALSE otherwise.
     *
     */
    void SetGroup(bool aGroup)
    {
        if (aGroup)
        {
            m8[0] |= kGroupFlag;
        }
        else
        {
            m8[0] &= ~kGroupFlag;
        }
    }

    /**
     * This method toggles the Group bit.
     *
     */
    void ToggleGroup(void) { m8[0] ^= kGroupFlag; }

    /**
     * This method indicates whether or not the Local bit is set.
     *
     * @retval TRUE   If the local bit is set.
     * @retval FALSE  If the local bit is not set.
     *
     */
    bool IsLocal(void) const { return (m8[0] & kLocalFlag) != 0; }

    /**
     * This method sets the Local bit.
     *
     * @param[in]  aLocal  TRUE if locally administered, FALSE otherwise.
     *
     */
    void SetLocal(bool aLocal)
    {
        if (aLocal)
        {
            m8[0] |= kLocalFlag;
        }
        else
        {
            m8[0] &= ~kLocalFlag;
        }
    }

    /**
     * This method toggles the Local bit.
     *
     */
    void ToggleLocal(void) { m8[0] ^= kLocalFlag; }

    /**
     * This method copies the Extended Address into a given buffer.
     *
     * @param[out] aBuffer     A pointer to a buffer to copy the Extended Address into.
     * @param[in]  aByteOrder  The byte order to copy the address.
     *
     */
    void CopyTo(uint8_t *aBuffer, CopyByteOrder aByteOrder = kNormalByteOrder) const
    {
        CopyAddress(aBuffer, m8, aByteOrder);
    }

    /**
     * This method evaluates whether or not the Extended Addresses match.
     *
     * @param[in]  aOther  The Extended Address to compare.
     *
     * @retval TRUE   If the Extended Addresses match.
     * @retval FALSE  If the Extended Addresses do not match.
     *
     */
    bool operator==(const ExtAddress &aOther) const;

    /**
     * This method evaluates whether or not the Extended Addresses match.
     *
     * @param[in]  aOther  The Extended Address to compare.
     *
     * @retval TRUE   If the Extended Addresses do not match.
     * @retval FALSE  If the Extended Addresses match.
     *
     */
    bool operator!=(const ExtAddress &aOther) const { return !(*this == aOther); }

    /**
     * This method converts an address to a string.
     *
     * @returns An `InfoString` containing the string representation of the Extended Address.
     *
     */
    InfoString ToString(void) const;

private:
    static void CopyAddress(uint8_t *aDst, const uint8_t *aSrc, CopyByteOrder aByteOrder);

    enum
    {
        kGroupFlag = 1 << 0,
        kLocalFlag = 1 << 1,
    };
} OT_TOOL_PACKED_END;

/**
 * This class represents an IEEE 802.15.4 Short or Extended Address.
 *
 */
class Address
{
public:
    /**
     * This type defines the fixed-length `String` object returned from `ToString()`.
     *
     */
    typedef ExtAddress::InfoString InfoString;

    /**
     * This enumeration specifies the IEEE 802.15.4 Address type.
     *
     */
    enum Type
    {
        kTypeNone,     ///< No address.
        kTypeShort,    ///< IEEE 802.15.4 Short Address.
        kTypeExtended, ///< IEEE 802.15.4 Extended Address.
    };

    /**
     * This constructor initializes an Address.
     *
     */
    Address(void)
        : mType(kTypeNone)
    {
    }

    /**
     * This method gets the address type (Short Address, Extended Address, or none).
     *
     * @returns The address type.
     *
     */
    Type GetType(void) const { return mType; }

    /**
     * This method indicates whether or not there is an address.
     *
     * @returns TRUE if there is no address (i.e. address type is `kTypeNone`), FALSE otherwise.
     *
     */
    bool IsNone(void) const { return (mType == kTypeNone); }

    /**
     * This method indicates whether or not the Address is a Short Address.
     *
     * @returns TRUE if it is a Short Address, FALSE otherwise.
     *
     */
    bool IsShort(void) const { return (mType == kTypeShort); }

    /**
     * This method indicates whether or not the Address is an Extended Address.
     *
     * @returns TRUE if it is an Extended Address, FALSE otherwise.
     *
     */
    bool IsExtended(void) const { return (mType == kTypeExtended); }

    /**
     * This method gets the address as a Short Address.
     *
     * This method MUST be used only if the address type is Short Address.
     *
     * @returns The Short Address.
     *
     */
    ShortAddress GetShort(void) const { return mShared.mShortAddress; }

    /**
     * This method gets the address as an Extended Address.
     *
     * This method MUST be used only if the address type is Extended Address.
     *
     * @returns A constant reference to the Extended Address.
     *
     */
    const ExtAddress &GetExtended(void) const { return mShared.mExtAddress; }

    /**
     * This method gets the address as an Extended Address.
     *
     * This method MUST be used only if the address type is Extended Address.
     *
     * @returns A reference to the Extended Address.
     *
     */
    ExtAddress &GetExtended(void) { return mShared.mExtAddress; }

    /**
     * This method sets the address to none (i.e., clears the address).
     *
     * Address type will be updated to `kTypeNone`.
     *
     */
    void SetNone(void) { mType = kTypeNone; }

    /**
     * This method sets the address with a Short Address.
     *
     * The type is also updated to indicate that address is Short.
     *
     * @param[in]  aShortAddress  A Short Address
     *
     */
    void SetShort(ShortAddress aShortAddress)
    {
        mShared.mShortAddress = aShortAddress;
        mType                 = kTypeShort;
    }

    /**
     * This method sets the address with an Extended Address.
     *
     * The type is also updated to indicate that the address is Extended.
     *
     * @param[in]  aExtAddress  An Extended Address
     *
     */
    void SetExtended(const ExtAddress &aExtAddress)
    {
        mShared.mExtAddress = aExtAddress;
        mType               = kTypeExtended;
    }

    /**
     * This method sets the address with an Extended Address given as a byte array.
     *
     * The type is also updated to indicate that the address is Extended.
     *
     * @param[in] aBuffer    Pointer to an array containing the Extended Address. `OT_EXT_ADDRESS_SIZE` bytes from
     *                       buffer are copied to form the Extended Address.
     * @param[in] aByteOrder The byte order to copy the address from @p aBuffer.
     *
     */
    void SetExtended(const uint8_t *aBuffer, ExtAddress::CopyByteOrder aByteOrder = ExtAddress::kNormalByteOrder)
    {
        mShared.mExtAddress.Set(aBuffer, aByteOrder);
        mType = kTypeExtended;
    }

    /**
     * This method indicates whether or not the address is a Short Broadcast Address.
     *
     * @returns TRUE if address is Short Broadcast Address, FALSE otherwise.
     *
     */
    bool IsBroadcast(void) const { return ((mType == kTypeShort) && (GetShort() == kShortAddrBroadcast)); }

    /**
     * This method indicates whether or not the address is a Short Invalid Address.
     *
     * @returns TRUE if address is Short Invalid Address, FALSE otherwise.
     *
     */
    bool IsShortAddrInvalid(void) const { return ((mType == kTypeShort) && (GetShort() == kShortAddrInvalid)); }

    /**
     * This method converts an address to a null-terminated string
     *
     * @returns A `String` representing the address.
     *
     */
    InfoString ToString(void) const;

private:
    union
    {
        ShortAddress mShortAddress; ///< The IEEE 802.15.4 Short Address.
        ExtAddress   mExtAddress;   ///< The IEEE 802.15.4 Extended Address.
    } mShared;

    Type mType; ///< The address type (Short, Extended, or none).
};

/**
 * This structure represents an IEEE 802.15.4 Extended PAN Identifier.
 *
 */
OT_TOOL_PACKED_BEGIN
class ExtendedPanId : public otExtendedPanId
{
public:
    enum
    {
        kInfoStringSize = 17, // Max chars for the info string (`ToString()`).
    };

    /**
     * This type defines the fixed-length `String` object returned from `ToString()`.
     *
     */
    typedef String<kInfoStringSize> InfoString;

    /**
     * This method clears the Extended PAN Identifier (sets all bytes to zero).
     *
     */
    void Clear(void) { memset(this, 0, sizeof(*this)); }

    /**
     * This method evaluates whether or not the Extended PAN Identifiers match.
     *
     * @param[in]  aOther  The Extended PAN Id to compare.
     *
     * @retval TRUE   If the Extended PAN Identifiers match.
     * @retval FALSE  If the Extended PAN Identifiers do not match.
     *
     */
    bool operator==(const ExtendedPanId &aOther) const;

    /**
     * This method evaluates whether or not the Extended PAN Identifiers match.
     *
     * @param[in]  aOther  The Extended PAN Id to compare.
     *
     * @retval TRUE   If the Extended Addresses do not match.
     * @retval FALSE  If the Extended Addresses match.
     *
     */
    bool operator!=(const ExtendedPanId &aOther) const { return !(*this == aOther); }

    /**
     * This method converts an address to a string.
     *
     * @returns An `InfoString` containing the string representation of the Extended PAN Identifier.
     *
     */
    InfoString ToString(void) const;

} OT_TOOL_PACKED_END;

/**
 * This structure represents an IEEE802.15.4 Network Name.
 *
 */
class NetworkName : public otNetworkName
{
public:
    enum
    {
        kMaxSize = OT_NETWORK_NAME_MAX_SIZE, // Maximum number of chars in Network Name (excludes null char).
    };

    /**
     * This class represents an IEEE802.15.4 Network Name as Data (pointer to a char buffer along with a length).
     *
     * @note The char array does NOT need to be null terminated.
     *
     */
    class Data
    {
    public:
        /**
         * This constructor initializes the Data object.
         *
         * @param[in] aBuffer   A pointer to a `char` buffer (does not need to be null terminated).
         * @param[in] aLength   The length (number of chars) in the buffer.
         *
         */
        Data(const char *aBuffer, uint8_t aLength)
            : mBuffer(aBuffer)
            , mLength(aLength)
        {
        }

        /**
         * This method returns the pointer to char buffer (not necessarily null terminated).
         *
         * @returns The pointer to the char buffer.
         *
         */
        const char *GetBuffer(void) const { return mBuffer; }

        /**
         * This method returns the length (number of chars in buffer).
         *
         * @returns The name length.
         *
         */
        uint8_t GetLength(void) const { return mLength; }

        /**
         * This method copies the name data into a given char buffer with a given size.
         *
         * The given buffer is cleared (`memset` to zero) before copying the Network Name into it. The copied string
         * in @p aBuffer is NOT necessarily null terminated.
         *
         * @param[out] aBuffer   A pointer to a buffer where to copy the Network Name into.
         * @param[in]  aMaxSize  Size of @p aBuffer (maximum number of chars to write into @p aBuffer).
         *
         * @returns The actual number of chars copied into @p aBuffer.
         *
         */
        uint8_t CopyTo(char *aBuffer, uint8_t aMaxSize) const;

    private:
        const char *mBuffer;
        uint8_t     mLength;
    };

    /**
     * This constructor initializes the IEEE802.15.4 Network Name as an empty string.
     *
     */
    NetworkName(void) { m8[0] = '\0'; }

    /**
     * This method gets the IEEE802.15.4 Network Name as a null terminated C string.
     *
     * @returns The Network Name as a null terminated C string array.
     *
     */
    const char *GetAsCString(void) const { return m8; }

    /**
     * This method gets the IEEE802.15.4 Network Name as Data.
     *
     * @returns The Network Name as Data.
     *
     */
    Data GetAsData(void) const;

    /**
     * This method sets the IEEE 802.15.4 Network Name.
     *
     * @param[in]  aNameData           A reference to name data.
     *
     * @retval OT_ERROR_NONE           Successfully set the IEEE 802.15.4 Network Name.
     * @retval OT_ERROR_ALREADY        The name is already set to the same string.
     * @retval OT_ERROR_INVALID_ARGS   Given name is too long.
     *
     */
    otError Set(const Data &aNameData);
};

/**
 * @}
 *
 */

} // namespace Mac
} // namespace ot

#endif // MAC_TYPES_HPP_
