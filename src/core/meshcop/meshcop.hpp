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
 *   This file includes definitions for MeshCoP.
 *
 */

#ifndef MESHCOP_HPP_
#define MESHCOP_HPP_

#include "openthread-core-config.h"

#include <openthread/commissioner.h>
#include <openthread/instance.h>
#include <openthread/joiner.h>

#include "coap/coap.hpp"
#include "common/as_core_type.hpp"
#include "common/clearable.hpp"
#include "common/equatable.hpp"
#include "common/log.hpp"
#include "common/message.hpp"
#include "common/numeric_limits.hpp"
#include "common/string.hpp"
#include "mac/mac_types.hpp"
#include "meshcop/meshcop_tlvs.hpp"

namespace ot {

class ThreadNetif;

namespace MeshCoP {

/**
 * Represents a Joiner PSKd.
 *
 */
class JoinerPskd : public otJoinerPskd, public Clearable<JoinerPskd>, public Unequatable<JoinerPskd>
{
public:
    static constexpr uint8_t kMinLength = 6;                         ///< Min PSKd string length (excludes null char)
    static constexpr uint8_t kMaxLength = OT_JOINER_MAX_PSKD_LENGTH; ///< Max PSKd string length (excludes null char)

    /**
     * Indicates whether the PSKd if well-formed and valid.
     *
     * Per Thread specification, a Joining Device Credential is encoded as uppercase alphanumeric characters
     * (base32-thread: 0-9, A-Z excluding I, O, Q, and Z for readability) with a minimum length of 6 such characters
     * and a maximum length of 32 such characters.
     *
     * @returns TRUE if the PSKd is valid, FALSE otherwise.
     *
     */
    bool IsValid(void) const { return IsPskdValid(m8); }

    /**
     * Sets the joiner PSKd from a given C string.
     *
     * @param[in] aPskdString   A pointer to the PSKd C string array.
     *
     * @retval kErrorNone          The PSKd was updated successfully.
     * @retval kErrorInvalidArgs   The given PSKd C string is not valid.
     *
     */
    Error SetFrom(const char *aPskdString);

    /**
     * Gets the PSKd as a null terminated C string.
     *
     * Must be used after the PSKd is validated, otherwise its behavior is undefined.
     *
     * @returns The PSKd as a C string.
     *
     */
    const char *GetAsCString(void) const { return m8; }

    /**
     * Gets the PSKd string length.
     *
     * Must be used after the PSKd is validated, otherwise its behavior is undefined.
     *
     * @returns The PSKd string length.
     *
     */
    uint8_t GetLength(void) const { return static_cast<uint8_t>(StringLength(m8, kMaxLength + 1)); }

    /**
     * Overloads operator `==` to evaluate whether or not two PSKds are equal.
     *
     * @param[in]  aOther  The other PSKd to compare with.
     *
     * @retval TRUE   If the two are equal.
     * @retval FALSE  If the two are not equal.
     *
     */
    bool operator==(const JoinerPskd &aOther) const;

    /**
     * Indicates whether a given PSKd string if well-formed and valid.
     *
     * @param[in] aPskdString  A pointer to a PSKd string array.
     *
     * @sa IsValid()
     *
     * @returns TRUE if @p aPskdString is valid, FALSE otherwise.
     *
     */
    static bool IsPskdValid(const char *aPskdString);
};

/**
 * Represents a Joiner Discerner.
 *
 */
class JoinerDiscerner : public otJoinerDiscerner, public Unequatable<JoinerDiscerner>
{
    friend class SteeringData;

public:
    static constexpr uint8_t kMaxLength = OT_JOINER_MAX_DISCERNER_LENGTH; ///< Max length of a Discerner in bits.

    static constexpr uint16_t kInfoStringSize = 45; ///< Size of `InfoString` to use with `ToString()

    /**
     * Defines the fixed-length `String` object returned from `ToString()`.
     *
     */
    typedef String<kInfoStringSize> InfoString;

    /**
     * Clears the Joiner Discerner.
     *
     */
    void Clear(void) { mLength = 0; }

    /**
     * Indicates whether the Joiner Discerner is empty (no value set).
     *
     * @returns TRUE if empty, FALSE otherwise.
     *
     */
    bool IsEmpty(void) const { return mLength == 0; }

    /**
     * Gets the Joiner Discerner's value.
     *
     * @returns The Joiner Discerner value.
     *
     */
    uint64_t GetValue(void) const { return mValue; }

    /**
     * Gets the Joiner Discerner's length (in bits).
     *
     * @return The Joiner Discerner length.
     *
     */
    uint8_t GetLength(void) const { return mLength; }

    /**
     * Indicates whether the Joiner Discerner is valid (i.e. it not empty and its length is within
     * valid range).
     *
     * @returns TRUE if Joiner Discerner is valid, FALSE otherwise.
     *
     */
    bool IsValid(void) const { return (0 < mLength) && (mLength <= kMaxLength); }

    /**
     * Generates a Joiner ID from the Discerner.
     *
     * @param[out] aJoinerId   A reference to `Mac::ExtAddress` to output the generated Joiner ID.
     *
     */
    void GenerateJoinerId(Mac::ExtAddress &aJoinerId) const;

    /**
     * Indicates whether a given Joiner ID matches the Discerner.
     *
     * @param[in] aJoinerId  A Joiner ID to match with the Discerner.
     *
     * @returns TRUE if the Joiner ID matches the Discerner, FALSE otherwise.
     *
     */
    bool Matches(const Mac::ExtAddress &aJoinerId) const;

    /**
     * Overloads operator `==` to evaluate whether or not two Joiner Discerner instances are equal.
     *
     * @param[in]  aOther  The other Joiner Discerner to compare with.
     *
     * @retval TRUE   If the two are equal.
     * @retval FALSE  If the two are not equal.
     *
     */
    bool operator==(const JoinerDiscerner &aOther) const;

    /**
     * Converts the Joiner Discerner to a string.
     *
     * @returns An `InfoString` representation of Joiner Discerner.
     *
     */
    InfoString ToString(void) const;

private:
    uint64_t GetMask(void) const { return (static_cast<uint64_t>(1ULL) << mLength) - 1; }
    void     CopyTo(Mac::ExtAddress &aExtAddress) const;
};

/**
 * Represents Steering Data (bloom filter).
 *
 */
class SteeringData : public otSteeringData
{
public:
    static constexpr uint8_t kMaxLength = OT_STEERING_DATA_MAX_LENGTH; ///< Maximum Steering Data length (in bytes).

    /**
     * Represents the hash bit index values for the bloom filter calculated from a Joiner ID.
     *
     * The first hash bit index is derived using CRC16-CCITT and second one using CRC16-ANSI.
     *
     */
    struct HashBitIndexes
    {
        static constexpr uint8_t kNumIndexes = 2; ///< Number of hash bit indexes.

        uint16_t mIndex[kNumIndexes]; ///< The hash bit index array.
    };

    /**
     * Initializes the Steering Data and clears the bloom filter.
     *
     * @param[in]  aLength   The Steering Data length (in bytes) - MUST be smaller than or equal to `kMaxLength`.
     *
     */
    void Init(uint8_t aLength = kMaxLength);

    /**
     * Clears the bloom filter (all bits are cleared and no Joiner Id is accepted)..
     *
     * The Steering Data length (bloom filter length) is set to one byte with all bits cleared.
     *
     */
    void Clear(void) { Init(1); }

    /**
     * Sets the bloom filter to permit all Joiner IDs.
     *
     * To permit all Joiner IDs, The Steering Data length (bloom filter length) is set to one byte with all bits set.
     *
     */
    void SetToPermitAllJoiners(void);

    /**
     * Returns the Steering Data length (in bytes).
     *
     * @returns The Steering Data length (in bytes).
     *
     */
    uint8_t GetLength(void) const { return mLength; }

    /**
     * Gets the Steering Data buffer (bloom filter).
     *
     * @returns A pointer to the Steering Data buffer.
     *
     */
    const uint8_t *GetData(void) const { return m8; }

    /**
     * Gets the Steering Data buffer (bloom filter).
     *
     * @returns A pointer to the Steering Data buffer.
     *
     */
    uint8_t *GetData(void) { return m8; }

    /**
     * Updates the bloom filter adding the given Joiner ID.
     *
     * @param[in]  aJoinerId  The Joiner ID to add to bloom filter.
     *
     */
    void UpdateBloomFilter(const Mac::ExtAddress &aJoinerId);

    /**
     * Updates the bloom filter adding a given Joiner Discerner.
     *
     * @param[in]  aDiscerner  The Joiner Discerner to add to bloom filter.
     *
     */
    void UpdateBloomFilter(const JoinerDiscerner &aDiscerner);

    /**
     * Indicates whether the bloom filter is empty (all the bits are cleared).
     *
     * @returns TRUE if the bloom filter is empty, FALSE otherwise.
     *
     */
    bool IsEmpty(void) const { return DoesAllMatch(0); }

    /**
     * Indicates whether the bloom filter permits all Joiner IDs (all the bits are set).
     *
     * @returns TRUE if the bloom filter permits all Joiners IDs, FALSE otherwise.
     *
     */
    bool PermitsAllJoiners(void) const { return (mLength > 0) && DoesAllMatch(kPermitAll); }

    /**
     * Indicates whether the bloom filter contains a given Joiner ID.
     *
     * @param[in] aJoinerId  A Joiner ID.
     *
     * @returns TRUE if the bloom filter contains @p aJoinerId, FALSE otherwise.
     *
     */
    bool Contains(const Mac::ExtAddress &aJoinerId) const;

    /**
     * Indicates whether the bloom filter contains a given Joiner Discerner.
     *
     * @param[in] aDiscerner   A Joiner Discerner.
     *
     * @returns TRUE if the bloom filter contains @p aDiscerner, FALSE otherwise.
     *
     */
    bool Contains(const JoinerDiscerner &aDiscerner) const;

    /**
     * Indicates whether the bloom filter contains the hash bit indexes (derived from a Joiner ID).
     *
     * @param[in]  aIndexes   A hash bit index structure (derived from a Joiner ID).
     *
     * @returns TRUE if the bloom filter contains the Joiner ID mapping to @p aIndexes, FALSE otherwise.
     *
     */
    bool Contains(const HashBitIndexes &aIndexes) const;

    /**
     * Calculates the bloom filter hash bit indexes from a given Joiner ID.
     *
     * The first hash bit index is derived using CRC16-CCITT and second one using CRC16-ANSI.
     *
     * @param[in]  aJoinerId  The Joiner ID to calculate the hash bit indexes.
     * @param[out] aIndexes   A reference to a `HashBitIndexes` structure to output the calculated index values.
     *
     */
    static void CalculateHashBitIndexes(const Mac::ExtAddress &aJoinerId, HashBitIndexes &aIndexes);

    /**
     * Calculates the bloom filter hash bit indexes from a given Joiner Discerner.
     *
     * The first hash bit index is derived using CRC16-CCITT and second one using CRC16-ANSI.
     *
     * @param[in]  aDiscerner     The Joiner Discerner to calculate the hash bit indexes.
     * @param[out] aIndexes       A reference to a `HashBitIndexes` structure to output the calculated index values.
     *
     */
    static void CalculateHashBitIndexes(const JoinerDiscerner &aDiscerner, HashBitIndexes &aIndexes);

private:
    static constexpr uint8_t kPermitAll = 0xff;

    uint8_t GetNumBits(void) const { return (mLength * kBitsPerByte); }

    uint8_t BitIndex(uint8_t aBit) const { return (mLength - 1 - (aBit / kBitsPerByte)); }
    uint8_t BitFlag(uint8_t aBit) const { return static_cast<uint8_t>(1U << (aBit % kBitsPerByte)); }

    bool GetBit(uint8_t aBit) const { return (m8[BitIndex(aBit)] & BitFlag(aBit)) != 0; }
    void SetBit(uint8_t aBit) { m8[BitIndex(aBit)] |= BitFlag(aBit); }
    void ClearBit(uint8_t aBit) { m8[BitIndex(aBit)] &= ~BitFlag(aBit); }

    bool DoesAllMatch(uint8_t aMatch) const;
    void UpdateBloomFilter(const HashBitIndexes &aIndexes);
};

/**
 * Represents a Commissioning Dataset.
 *
 */
class CommissioningDataset : public otCommissioningDataset, public Clearable<CommissioningDataset>
{
public:
    /**
     * Indicates whether or not the Border Router RLOC16 Locator is set in the Dataset.
     *
     * @returns TRUE if Border Router RLOC16 Locator is set, FALSE otherwise.
     *
     */
    bool IsLocatorSet(void) const { return mIsLocatorSet; }

    /**
     * Gets the Border Router RLOC16 Locator in the Dataset.
     *
     * MUST be used when Locator is set in the Dataset, otherwise its behavior is undefined.
     *
     * @returns The Border Router RLOC16 Locator in the Dataset.
     *
     */
    uint16_t GetLocator(void) const { return mLocator; }

    /**
     * Sets the Border Router RLOCG16 Locator in the Dataset.
     *
     * @param[in] aLocator  A Locator.
     *
     */
    void SetLocator(uint16_t aLocator)
    {
        mIsLocatorSet = true;
        mLocator      = aLocator;
    }

    /**
     * Indicates whether or not the Session ID is set in the Dataset.
     *
     * @returns TRUE if Session ID is set, FALSE otherwise.
     *
     */
    bool IsSessionIdSet(void) const { return mIsSessionIdSet; }

    /**
     * Gets the Session ID in the Dataset.
     *
     * MUST be used when Session ID is set in the Dataset, otherwise its behavior is undefined.
     *
     * @returns The Session ID in the Dataset.
     *
     */
    uint16_t GetSessionId(void) const { return mSessionId; }

    /**
     * Sets the Session ID in the Dataset.
     *
     * @param[in] aSessionId  The Session ID.
     *
     */
    void SetSessionId(uint16_t aSessionId)
    {
        mIsSessionIdSet = true;
        mSessionId      = aSessionId;
    }

    /**
     * Indicates whether or not the Steering Data is set in the Dataset.
     *
     * @returns TRUE if Steering Data is set, FALSE otherwise.
     *
     */
    bool IsSteeringDataSet(void) const { return mIsSteeringDataSet; }

    /**
     * Gets the Steering Data in the Dataset.
     *
     * MUST be used when Steering Data is set in the Dataset, otherwise its behavior is undefined.
     *
     * @returns The Steering Data in the Dataset.
     *
     */
    const SteeringData &GetSteeringData(void) const { return static_cast<const SteeringData &>(mSteeringData); }

    /**
     * Returns a reference to the Steering Data in the Dataset to be updated by caller.
     *
     * @returns A reference to the Steering Data in the Dataset.
     *
     */
    SteeringData &UpdateSteeringData(void)
    {
        mIsSteeringDataSet = true;
        return static_cast<SteeringData &>(mSteeringData);
    }

    /**
     * Indicates whether or not the Joiner UDP port is set in the Dataset.
     *
     * @returns TRUE if Joiner UDP port is set, FALSE otherwise.
     *
     */
    bool IsJoinerUdpPortSet(void) const { return mIsJoinerUdpPortSet; }

    /**
     * Gets the Joiner UDP port in the Dataset.
     *
     * MUST be used when Joiner UDP port is set in the Dataset, otherwise its behavior is undefined.
     *
     * @returns The Joiner UDP port in the Dataset.
     *
     */
    uint16_t GetJoinerUdpPort(void) const { return mJoinerUdpPort; }

    /**
     * Sets the Joiner UDP Port in the Dataset.
     *
     * @param[in] aJoinerUdpPort  The Joiner UDP Port.
     *
     */
    void SetJoinerUdpPort(uint16_t aJoinerUdpPort)
    {
        mIsJoinerUdpPortSet = true;
        mJoinerUdpPort      = aJoinerUdpPort;
    }
};

/**
 * Generates PSKc.
 *
 * PSKc is used to establish the Commissioner Session.
 *
 * @param[in]  aPassPhrase   The commissioning passphrase.
 * @param[in]  aNetworkName  The network name for PSKc computation.
 * @param[in]  aExtPanId     The extended PAN ID for PSKc computation.
 * @param[out] aPskc         A reference to a PSKc where the generated PSKc will be placed.
 *
 * @retval kErrorNone          Successfully generate PSKc.
 * @retval kErrorInvalidArgs   If the length of passphrase is out of range.
 *
 */
Error GeneratePskc(const char          *aPassPhrase,
                   const NetworkName   &aNetworkName,
                   const ExtendedPanId &aExtPanId,
                   Pskc                &aPskc);

/**
 * Computes the Joiner ID from a factory-assigned IEEE EUI-64.
 *
 * @param[in]   aEui64     The factory-assigned IEEE EUI-64.
 * @param[out]  aJoinerId  The Joiner ID.
 *
 */
void ComputeJoinerId(const Mac::ExtAddress &aEui64, Mac::ExtAddress &aJoinerId);

#if OT_SHOULD_LOG_AT(OT_LOG_LEVEL_WARN)
/**
 * Emits a log message indicating an error during a MeshCoP action.
 *
 * Note that log message is emitted only if there is an error, i.e. @p aError is not `kErrorNone`. The log
 * message will have the format "Failed to {aActionText} : {ErrorString}".
 *
 * @param[in] aActionText   A string representing the failed action.
 * @param[in] aError        The error in sending the message.
 *
 */
void LogError(const char *aActionText, Error aError);
#else
inline void LogError(const char *, Error) {}
#endif

} // namespace MeshCoP

DefineCoreType(otJoinerPskd, MeshCoP::JoinerPskd);
DefineCoreType(otJoinerDiscerner, MeshCoP::JoinerDiscerner);
DefineCoreType(otSteeringData, MeshCoP::SteeringData);
DefineCoreType(otCommissioningDataset, MeshCoP::CommissioningDataset);

} // namespace ot

#endif // MESHCOP_HPP_
