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
 *   This file includes definitions for MAC types.
 */

#ifndef MAC_TYPES_HPP_
#define MAC_TYPES_HPP_

#include "openthread-core-config.h"

#include <stdint.h>
#include <string.h>

#include <openthread/link.h>
#include <openthread/thread.h>

#include "common/as_core_type.hpp"
#include "common/clearable.hpp"
#include "common/data.hpp"
#include "common/equatable.hpp"
#include "common/string.hpp"
#include "crypto/storage.hpp"

namespace ot {
namespace Mac {

/**
 * @addtogroup core-mac
 *
 * @{
 */

/**
 * Represents the IEEE 802.15.4 PAN ID.
 */
typedef otPanId PanId;

constexpr PanId kPanIdBroadcast = 0xffff; ///< Broadcast PAN ID.

/**
 * Represents the IEEE 802.15.4 Short Address.
 */
typedef otShortAddress ShortAddress;

constexpr ShortAddress kShortAddrBroadcast = 0xffff; ///< Broadcast Short Address.
constexpr ShortAddress kShortAddrInvalid   = 0xfffe; ///< Invalid Short Address.

/**
 * Generates a random IEEE 802.15.4 PAN ID.
 *
 * @returns A randomly generated IEEE 802.15.4 PAN ID (excluding `kPanIdBroadcast`).
 */
PanId GenerateRandomPanId(void);

/**
 * Represents an IEEE 802.15.4 Extended Address.
 */
OT_TOOL_PACKED_BEGIN
class ExtAddress : public otExtAddress, public Equatable<ExtAddress>, public Clearable<ExtAddress>
{
public:
    static constexpr uint16_t kInfoStringSize = 17; ///< Max chars for the info string (`ToString()`).

    /**
     * Defines the fixed-length `String` object returned from `ToString()`.
     */
    typedef String<kInfoStringSize> InfoString;

    /**
     * Type specifies the copy byte order when Extended Address is being copied to/from a buffer.
     */
    enum CopyByteOrder : uint8_t
    {
        kNormalByteOrder,  ///< Copy address bytes in normal order (as provided in array buffer).
        kReverseByteOrder, ///< Copy address bytes in reverse byte order.
    };

    /**
     * Fills all bytes of address with a given byte value.
     *
     * @param[in] aByte A byte value to fill address with.
     */
    void Fill(uint8_t aByte) { memset(this, aByte, sizeof(*this)); }

    /**
     * Generates a random IEEE 802.15.4 Extended Address.
     */
    void GenerateRandom(void);

    /**
     * Sets the Extended Address from a given byte array.
     *
     * @param[in] aBuffer    Pointer to an array containing the Extended Address. `OT_EXT_ADDRESS_SIZE` bytes from
     *                       buffer are copied to form the Extended Address.
     * @param[in] aByteOrder The byte order to use when copying the address.
     */
    void Set(const uint8_t *aBuffer, CopyByteOrder aByteOrder = kNormalByteOrder)
    {
        CopyAddress(m8, aBuffer, aByteOrder);
    }

    /**
     * Indicates whether or not the Group bit is set.
     *
     * @retval TRUE   If the group bit is set.
     * @retval FALSE  If the group bit is not set.
     */
    bool IsGroup(void) const { return (m8[0] & kGroupFlag) != 0; }

    /**
     * Sets the Group bit.
     *
     * @param[in]  aGroup  TRUE if group address, FALSE otherwise.
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
     * Toggles the Group bit.
     */
    void ToggleGroup(void) { m8[0] ^= kGroupFlag; }

    /**
     * Indicates whether or not the Local bit is set.
     *
     * @retval TRUE   If the local bit is set.
     * @retval FALSE  If the local bit is not set.
     */
    bool IsLocal(void) const { return (m8[0] & kLocalFlag) != 0; }

    /**
     * Sets the Local bit.
     *
     * @param[in]  aLocal  TRUE if locally administered, FALSE otherwise.
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
     * Toggles the Local bit.
     */
    void ToggleLocal(void) { m8[0] ^= kLocalFlag; }

    /**
     * Copies the Extended Address into a given buffer.
     *
     * @param[out] aBuffer     A pointer to a buffer to copy the Extended Address into.
     * @param[in]  aByteOrder  The byte order to copy the address.
     */
    void CopyTo(uint8_t *aBuffer, CopyByteOrder aByteOrder = kNormalByteOrder) const
    {
        CopyAddress(aBuffer, m8, aByteOrder);
    }

    /**
     * Converts an address to a string.
     *
     * @returns An `InfoString` containing the string representation of the Extended Address.
     */
    InfoString ToString(void) const;

private:
    static constexpr uint8_t kGroupFlag = (1 << 0);
    static constexpr uint8_t kLocalFlag = (1 << 1);

    static void CopyAddress(uint8_t *aDst, const uint8_t *aSrc, CopyByteOrder aByteOrder);
} OT_TOOL_PACKED_END;

/**
 * Represents an IEEE 802.15.4 Short or Extended Address.
 */
class Address
{
public:
    /**
     * Defines the fixed-length `String` object returned from `ToString()`.
     */
    typedef ExtAddress::InfoString InfoString;

    /**
     * Specifies the IEEE 802.15.4 Address type.
     */
    enum Type : uint8_t
    {
        kTypeNone,     ///< No address.
        kTypeShort,    ///< IEEE 802.15.4 Short Address.
        kTypeExtended, ///< IEEE 802.15.4 Extended Address.
    };

    /**
     * Initializes an Address.
     */
    Address(void)
        : mType(kTypeNone)
    {
    }

    /**
     * Gets the address type (Short Address, Extended Address, or none).
     *
     * @returns The address type.
     */
    Type GetType(void) const { return mType; }

    /**
     * Indicates whether or not there is an address.
     *
     * @returns TRUE if there is no address (i.e. address type is `kTypeNone`), FALSE otherwise.
     */
    bool IsNone(void) const { return (mType == kTypeNone); }

    /**
     * Indicates whether or not the Address is a Short Address.
     *
     * @returns TRUE if it is a Short Address, FALSE otherwise.
     */
    bool IsShort(void) const { return (mType == kTypeShort); }

    /**
     * Indicates whether or not the Address is an Extended Address.
     *
     * @returns TRUE if it is an Extended Address, FALSE otherwise.
     */
    bool IsExtended(void) const { return (mType == kTypeExtended); }

    /**
     * Gets the address as a Short Address.
     *
     * MUST be used only if the address type is Short Address.
     *
     * @returns The Short Address.
     */
    ShortAddress GetShort(void) const { return mShared.mShortAddress; }

    /**
     * Gets the address as an Extended Address.
     *
     * MUST be used only if the address type is Extended Address.
     *
     * @returns A constant reference to the Extended Address.
     */
    const ExtAddress &GetExtended(void) const { return mShared.mExtAddress; }

    /**
     * Gets the address as an Extended Address.
     *
     * MUST be used only if the address type is Extended Address.
     *
     * @returns A reference to the Extended Address.
     */
    ExtAddress &GetExtended(void) { return mShared.mExtAddress; }

    /**
     * Sets the address to none (i.e., clears the address).
     *
     * Address type will be updated to `kTypeNone`.
     */
    void SetNone(void) { mType = kTypeNone; }

    /**
     * Sets the address with a Short Address.
     *
     * The type is also updated to indicate that address is Short.
     *
     * @param[in]  aShortAddress  A Short Address
     */
    void SetShort(ShortAddress aShortAddress)
    {
        mShared.mShortAddress = aShortAddress;
        mType                 = kTypeShort;
    }

    /**
     * Sets the address with an Extended Address.
     *
     * The type is also updated to indicate that the address is Extended.
     *
     * @param[in]  aExtAddress  An Extended Address
     */
    void SetExtended(const ExtAddress &aExtAddress)
    {
        mShared.mExtAddress = aExtAddress;
        mType               = kTypeExtended;
    }

    /**
     * Sets the address with an Extended Address given as a byte array.
     *
     * The type is also updated to indicate that the address is Extended.
     *
     * @param[in] aBuffer    Pointer to an array containing the Extended Address. `OT_EXT_ADDRESS_SIZE` bytes from
     *                       buffer are copied to form the Extended Address.
     * @param[in] aByteOrder The byte order to copy the address from @p aBuffer.
     */
    void SetExtended(const uint8_t *aBuffer, ExtAddress::CopyByteOrder aByteOrder = ExtAddress::kNormalByteOrder)
    {
        mShared.mExtAddress.Set(aBuffer, aByteOrder);
        mType = kTypeExtended;
    }

    /**
     * Indicates whether or not the address is a Short Broadcast Address.
     *
     * @returns TRUE if address is Short Broadcast Address, FALSE otherwise.
     */
    bool IsBroadcast(void) const { return ((mType == kTypeShort) && (GetShort() == kShortAddrBroadcast)); }

    /**
     * Indicates whether or not the address is a Short Invalid Address.
     *
     * @returns TRUE if address is Short Invalid Address, FALSE otherwise.
     */
    bool IsShortAddrInvalid(void) const { return ((mType == kTypeShort) && (GetShort() == kShortAddrInvalid)); }

    /**
     * Converts an address to a null-terminated string
     *
     * @returns A `String` representing the address.
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
 * Represents two MAC addresses corresponding to source and destination.
 */
struct Addresses
{
    Address mSource;      ///< Source address.
    Address mDestination; ///< Destination address.
};

/**
 * Represents two PAN IDs corresponding to source and destination.
 */
class PanIds : public Clearable<PanIds>
{
public:
    /**
     * Initializes PAN IDs as empty (no source or destination PAN ID).
     */
    PanIds(void) { Clear(); }

    /**
     * Indicates whether or not source PAN ID is present.
     *
     * @retval TRUE   The source PAN ID is present.
     * @retval FALSE  The source PAN ID is not present.
     */
    bool IsSourcePresent(void) const { return mIsSourcePresent; }

    /**
     * Gets the source PAN ID when it is present.
     *
     * @returns The source PAN ID.
     */
    PanId GetSource(void) const { return mSource; }

    /**
     * Indicates whether or not destination PAN ID is present.
     *
     * @retval TRUE   The destination PAN ID is present.
     * @retval FALSE  The destination PAN ID is not present.
     */
    bool IsDestinationPresent(void) const { return mIsDestinationPresent; }

    /**
     * Gets the destination PAN ID when it is present.
     *
     * @returns The destination PAN ID.
     */
    PanId GetDestination(void) const { return mDestination; }

    /**
     * Sets the source PAN ID.
     *
     * @param[in] aPanId  The source PAN ID.
     */
    void SetSource(PanId aPanId);

    /**
     * Sets the destination PAN ID.
     *
     * @param[in] aPanId  The source PAN ID.
     */
    void SetDestination(PanId aPanId);

    /**
     * Sets both source and destination PAN IDs to the same value.
     *
     * @param[in] aPanId  The PAN ID.
     */
    void SetBothSourceDestination(PanId aPanId);

private:
    PanId mSource;
    PanId mDestination;
    bool  mIsSourcePresent;
    bool  mIsDestinationPresent;
};

/**
 * Represents a MAC key.
 */
OT_TOOL_PACKED_BEGIN
class Key : public otMacKey, public Equatable<Key>, public Clearable<Key>
{
public:
    static constexpr uint16_t kSize = OT_MAC_KEY_SIZE; ///< Key size in bytes.

    /**
     * Gets a pointer to the bytes array containing the key
     *
     * @returns A pointer to the byte array containing the key.
     */
    const uint8_t *GetBytes(void) const { return m8; }

} OT_TOOL_PACKED_END;

/**
 * Represents a MAC Key Ref used by PSA.
 */
typedef otMacKeyRef KeyRef;

/**
 * Represents a MAC Key Material.
 */
class KeyMaterial : public otMacKeyMaterial, public Unequatable<KeyMaterial>
{
public:
    /**
     * Initializes a `KeyMaterial`.
     */
    KeyMaterial(void)
    {
        GetKey().Clear();
#if OPENTHREAD_CONFIG_PLATFORM_KEY_REFERENCES_ENABLE
        SetKeyRef(kInvalidKeyRef);
#endif
    }

#if OPENTHREAD_CONFIG_PLATFORM_KEY_REFERENCES_ENABLE
    /**
     * Overload `=` operator to assign the `KeyMaterial` from another one.
     *
     * If the `KeyMaterial` currently stores a valid and different `KeyRef`, the assignment of new value will ensure to
     * delete the previous one before using the new `KeyRef` from @p aOther.
     *
     * @param[in] aOther  aOther  The other `KeyMaterial` instance to assign from.
     *
     * @returns A reference to the current `KeyMaterial`
     */
    KeyMaterial &operator=(const KeyMaterial &aOther);

    KeyMaterial(const KeyMaterial &) = delete;
#endif

    /**
     *  This method clears the `KeyMaterial`.
     *
     * Under `OPENTHREAD_CONFIG_PLATFORM_KEY_REFERENCES_ENABLE`, if the `KeyMaterial` currently stores a valid previous
     * `KeyRef`, the `Clear()` call will ensure to delete the previous `KeyRef` and set it to `kInvalidKeyRef`.
     */
    void Clear(void);

#if !OPENTHREAD_CONFIG_PLATFORM_KEY_REFERENCES_ENABLE
    /**
     * Gets the literal `Key`.
     *
     * @returns The literal `Key`
     */
    const Key &GetKey(void) const { return static_cast<const Key &>(mKeyMaterial.mKey); }

#else
    /**
     * Gets the stored `KeyRef`
     *
     * @returns The `KeyRef`
     */
    KeyRef GetKeyRef(void) const { return mKeyMaterial.mKeyRef; }
#endif

    /**
     * Sets the `KeyMaterial` from a given Key.
     *
     * If the `KeyMaterial` currently stores a valid `KeyRef`, the `SetFrom()` call will ensure to delete the previous
     * one before creating and using a new `KeyRef` associated with the new `Key`.
     *
     * @param[in] aKey           A reference to the new key.
     * @param[in] aIsExportable  Boolean indicating if the key is exportable (this is only applicable under
     *                           `OPENTHREAD_CONFIG_PLATFORM_KEY_REFERENCES_ENABLE` config).
     */
    void SetFrom(const Key &aKey, bool aIsExportable = false);

    /**
     * Extracts the literal key from `KeyMaterial`
     *
     * @param[out] aKey  A reference to the output the key.
     */
    void ExtractKey(Key &aKey) const;

    /**
     * Converts `KeyMaterial` to a `Crypto::Key`.
     *
     * @param[out]  aCryptoKey  A reference to a `Crypto::Key` to populate.
     */
    void ConvertToCryptoKey(Crypto::Key &aCryptoKey) const;

    /**
     * Overloads operator `==` to evaluate whether or not two `KeyMaterial` instances are equal.
     *
     * @param[in]  aOther  The other `KeyMaterial` instance to compare with.
     *
     * @retval TRUE   If the two `KeyMaterial` instances are equal.
     * @retval FALSE  If the two `KeyMaterial` instances are not equal.
     */
    bool operator==(const KeyMaterial &aOther) const;

private:
#if OPENTHREAD_CONFIG_PLATFORM_KEY_REFERENCES_ENABLE
    static constexpr KeyRef kInvalidKeyRef = Crypto::Storage::kInvalidKeyRef;

    void DestroyKey(void);
    void SetKeyRef(KeyRef aKeyRef) { mKeyMaterial.mKeyRef = aKeyRef; }
#endif
    Key &GetKey(void) { return static_cast<Key &>(mKeyMaterial.mKey); }
    void SetKey(const Key &aKey) { mKeyMaterial.mKey = aKey; }
};

#if OPENTHREAD_CONFIG_MULTI_RADIO

/**
 * Defines the radio link types.
 */
enum RadioType : uint8_t
{
#if OPENTHREAD_CONFIG_RADIO_LINK_IEEE_802_15_4_ENABLE
    kRadioTypeIeee802154, ///< IEEE 802.15.4 (2.4GHz) link type.
#endif
#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
    kRadioTypeTrel, ///< Thread Radio Encapsulation link type.
#endif
};

/**
 * This constant specifies the number of supported radio link types.
 */
constexpr uint8_t kNumRadioTypes = (((OPENTHREAD_CONFIG_RADIO_LINK_IEEE_802_15_4_ENABLE) ? 1 : 0) +
                                    ((OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE) ? 1 : 0));

/**
 * Represents a set of radio links.
 */
class RadioTypes
{
public:
    static constexpr uint16_t kInfoStringSize = 32; ///< Max chars for the info string (`ToString()`).

    /**
     * Defines the fixed-length `String` object returned from `ToString()`.
     */
    typedef String<kInfoStringSize> InfoString;

    /**
     * This static class variable defines an array containing all supported radio link types.
     */
    static const RadioType kAllRadioTypes[kNumRadioTypes];

    /**
     * Initializes a `RadioTypes` object as empty set
     */
    RadioTypes(void)
        : mBitMask(0)
    {
    }

    /**
     * Initializes a `RadioTypes` object with a given bit-mask.
     *
     * @param[in] aMask   A bit-mask representing the radio types (the first bit corresponds to radio type 0, and so on)
     */
    explicit RadioTypes(uint8_t aMask)
        : mBitMask(aMask)
    {
    }

    /**
     * Clears the set.
     */
    void Clear(void) { mBitMask = 0; }

    /**
     * Indicates whether the set is empty or not
     *
     * @returns TRUE if the set is empty, FALSE otherwise.
     */
    bool IsEmpty(void) const { return (mBitMask == 0); }

    /**
     *  This method indicates whether the set contains only a single radio type.
     *
     * @returns TRUE if the set contains a single radio type, FALSE otherwise.
     */
    bool ContainsSingleRadio(void) const { return !IsEmpty() && ((mBitMask & (mBitMask - 1)) == 0); }

    /**
     * Indicates whether or not the set contains a given radio type.
     *
     * @param[in] aType  A radio link type.
     *
     * @returns TRUE if the set contains @p aType, FALSE otherwise.
     */
    bool Contains(RadioType aType) const { return ((mBitMask & BitFlag(aType)) != 0); }

    /**
     * Adds a radio type to the set.
     *
     * @param[in] aType  A radio link type.
     */
    void Add(RadioType aType) { mBitMask |= BitFlag(aType); }

    /**
     * Adds another radio types set to the current one.
     *
     * @param[in] aTypes   A radio link type set to add.
     */
    void Add(RadioTypes aTypes) { mBitMask |= aTypes.mBitMask; }

    /**
     * Adds all radio types supported by device to the set.
     */
    void AddAll(void);

    /**
     * Removes a given radio type from the set.
     *
     * @param[in] aType  A radio link type.
     */
    void Remove(RadioType aType) { mBitMask &= ~BitFlag(aType); }

    /**
     * Gets the radio type set as a bitmask.
     *
     * The first bit in the mask corresponds to first radio type (radio type with value zero), and so on.
     *
     * @returns A bitmask representing the set of radio types.
     */
    uint8_t GetAsBitMask(void) const { return mBitMask; }

    /**
     * Overloads operator `-` to return a new set which is the set difference between current set and
     * a given set.
     *
     * @param[in] aOther  Another radio type set.
     *
     * @returns A new set which is set difference between current one and @p aOther.
     */
    RadioTypes operator-(const RadioTypes &aOther) const { return RadioTypes(mBitMask & ~aOther.mBitMask); }

    /**
     * Converts the radio set to human-readable string.
     *
     * @return A string representation of the set of radio types.
     */
    InfoString ToString(void) const;

private:
    static uint8_t BitFlag(RadioType aType) { return static_cast<uint8_t>(1U << static_cast<uint8_t>(aType)); }

    uint8_t mBitMask;
};

/**
 * Converts a link type to a string
 *
 * @param[in] aRadioType  A link type value.
 *
 * @returns A string representation of the link type.
 */
const char *RadioTypeToString(RadioType aRadioType);

#endif // OPENTHREAD_CONFIG_MULTI_RADIO

/**
 * Represents Link Frame Counters for all supported radio links.
 */
class LinkFrameCounters
{
public:
    /**
     * Resets all counters (set them all to zero).
     */
    void Reset(void) { SetAll(0); }

#if OPENTHREAD_CONFIG_MULTI_RADIO

    /**
     * Gets the link Frame Counter for a given radio link.
     *
     * @param[in] aRadioType  A radio link type.
     *
     * @returns The Link Frame Counter for radio link @p aRadioType.
     */
    uint32_t Get(RadioType aRadioType) const;

    /**
     * Sets the Link Frame Counter for a given radio link.
     *
     * @param[in] aRadioType  A radio link type.
     * @param[in] aCounter    The new counter value.
     */
    void Set(RadioType aRadioType, uint32_t aCounter);

#else

    /**
     * Gets the Link Frame Counter value.
     *
     * @return The Link Frame Counter value.
     */
    uint32_t Get(void) const
#if OPENTHREAD_CONFIG_RADIO_LINK_IEEE_802_15_4_ENABLE
    {
        return m154Counter;
    }
#elif OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
    {
        return mTrelCounter;
    }
#endif

    /**
     * Sets the Link Frame Counter for a given radio link.
     *
     * @param[in] aCounter    The new counter value.
     */
    void Set(uint32_t aCounter)
#if OPENTHREAD_CONFIG_RADIO_LINK_IEEE_802_15_4_ENABLE
    {
        m154Counter = aCounter;
    }
#elif OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
    {
        mTrelCounter = aCounter;
    }
#endif

#endif // OPENTHREAD_CONFIG_MULTI_RADIO

#if OPENTHREAD_CONFIG_RADIO_LINK_IEEE_802_15_4_ENABLE
    /**
     * Gets the Link Frame Counter for 802.15.4 radio link.
     *
     * @returns The Link Frame Counter for 802.15.4 radio link.
     */
    uint32_t Get154(void) const { return m154Counter; }

    /**
     * Sets the Link Frame Counter for 802.15.4 radio link.
     *
     * @param[in] aCounter   The new counter value.
     */
    void Set154(uint32_t aCounter) { m154Counter = aCounter; }
#endif

#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
    /**
     * Gets the Link Frame Counter for TREL radio link.
     *
     * @returns The Link Frame Counter for TREL radio link.
     */
    uint32_t GetTrel(void) const { return mTrelCounter; }

    /**
     * Increments the Link Frame Counter for TREL radio link.
     */
    void IncrementTrel(void) { mTrelCounter++; }
#endif

    /**
     * Gets the maximum Link Frame Counter among all supported radio links.
     *
     * @return The maximum Link frame Counter among all supported radio links.
     */
    uint32_t GetMaximum(void) const;

    /**
     * Sets the Link Frame Counter value for all radio links.
     *
     * @param[in]  aCounter  The Link Frame Counter value.
     */
    void SetAll(uint32_t aCounter);

private:
#if OPENTHREAD_CONFIG_RADIO_LINK_IEEE_802_15_4_ENABLE
    uint32_t m154Counter;
#endif
#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
    uint32_t mTrelCounter;
#endif
};

/**
 * Represents CSL accuracy.
 */
class CslAccuracy
{
public:
    static constexpr uint8_t kWorstClockAccuracy = 255; ///< Worst possible crystal accuracy, in units of ± ppm.
    static constexpr uint8_t kWorstUncertainty   = 255; ///< Worst possible uncertainty, in units of 10 microseconds.

    /**
     * Initializes the CSL accuracy using `kWorstClockAccuracy` and `kWorstUncertainty` values.
     */
    void Init(void)
    {
        mClockAccuracy = kWorstClockAccuracy;
        mUncertainty   = kWorstUncertainty;
    }

    /**
     * Returns the CSL clock accuracy.
     *
     * @returns The CSL clock accuracy in ± ppm.
     */
    uint8_t GetClockAccuracy(void) const { return mClockAccuracy; }

    /**
     * Sets the CSL clock accuracy.
     *
     * @param[in]  aClockAccuracy  The CSL clock accuracy in ± ppm.
     */
    void SetClockAccuracy(uint8_t aClockAccuracy) { mClockAccuracy = aClockAccuracy; }

    /**
     * Returns the CSL uncertainty.
     *
     * @returns The uncertainty in units 10 microseconds.
     */
    uint8_t GetUncertainty(void) const { return mUncertainty; }

    /**
     * Gets the CLS uncertainty in microseconds.
     *
     * @returns the CLS uncertainty in microseconds.
     */
    uint16_t GetUncertaintyInMicrosec(void) const { return static_cast<uint16_t>(mUncertainty) * kUsPerUncertUnit; }

    /**
     * Sets the CSL uncertainty.
     *
     * @param[in]  aUncertainty  The CSL uncertainty in units 10 microseconds.
     */
    void SetUncertainty(uint8_t aUncertainty) { mUncertainty = aUncertainty; }

private:
    static constexpr uint8_t kUsPerUncertUnit = 10;

    uint8_t mClockAccuracy;
    uint8_t mUncertainty;
};

/**
 * @}
 */

} // namespace Mac

DefineCoreType(otExtAddress, Mac::ExtAddress);
DefineCoreType(otMacKey, Mac::Key);

} // namespace ot

#endif // MAC_TYPES_HPP_
