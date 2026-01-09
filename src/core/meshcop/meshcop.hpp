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
 */

#ifndef OT_CORE_MESHCOP_MESHCOP_HPP_
#define OT_CORE_MESHCOP_MESHCOP_HPP_

#include "openthread-core-config.h"

#include <openthread/commissioner.h>
#include <openthread/instance.h>
#include <openthread/joiner.h>

#include "coap/coap.hpp"
#include "common/as_core_type.hpp"
#include "common/clearable.hpp"
#include "common/code_utils.hpp"
#include "common/equatable.hpp"
#include "common/log.hpp"
#include "common/message.hpp"
#include "common/num_utils.hpp"
#include "common/numeric_limits.hpp"
#include "common/string.hpp"
#include "mac/mac_types.hpp"
#include "meshcop/meshcop_tlvs.hpp"
#include "meshcop/steering_data.hpp"

namespace ot {

class ThreadNetif;

namespace MeshCoP {

/**
 * Represents a Joiner PSKd.
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
     */
    bool IsValid(void) const { return IsPskdValid(m8); }

    /**
     * Sets the joiner PSKd from a given C string.
     *
     * @param[in] aPskdString   A pointer to the PSKd C string array.
     *
     * @retval kErrorNone          The PSKd was updated successfully.
     * @retval kErrorInvalidArgs   The given PSKd C string is not valid.
     */
    Error SetFrom(const char *aPskdString);

    /**
     * Gets the PSKd as a null terminated C string.
     *
     * Must be used after the PSKd is validated, otherwise its behavior is undefined.
     *
     * @returns The PSKd as a C string.
     */
    const char *GetAsCString(void) const { return m8; }

    /**
     * Gets the PSKd string length.
     *
     * Must be used after the PSKd is validated, otherwise its behavior is undefined.
     *
     * @returns The PSKd string length.
     */
    uint8_t GetLength(void) const { return static_cast<uint8_t>(StringLength(m8, kMaxLength + 1)); }

    /**
     * Gets the PSKd as a byte array.
     *
     * @returns The PSKd as a byte array.
     */
    const uint8_t *GetBytes(void) const { return reinterpret_cast<const uint8_t *>(m8); }

    /**
     * Overloads operator `==` to evaluate whether or not two PSKds are equal.
     *
     * @param[in]  aOther  The other PSKd to compare with.
     *
     * @retval TRUE   If the two are equal.
     * @retval FALSE  If the two are not equal.
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
     */
    static bool IsPskdValid(const char *aPskdString);
};

/**
 * Represents a Joiner Discerner.
 */
class JoinerDiscerner : public otJoinerDiscerner, public Unequatable<JoinerDiscerner>
{
    friend class SteeringData;

public:
    static constexpr uint8_t kMaxLength = OT_JOINER_MAX_DISCERNER_LENGTH; ///< Max length of a Discerner in bits.

    static constexpr uint16_t kInfoStringSize = 45; ///< Size of `InfoString` to use with `ToString()

    /**
     * Defines the fixed-length `String` object returned from `ToString()`.
     */
    typedef String<kInfoStringSize> InfoString;

    /**
     * Clears the Joiner Discerner.
     */
    void Clear(void) { mLength = 0; }

    /**
     * Indicates whether the Joiner Discerner is empty (no value set).
     *
     * @returns TRUE if empty, FALSE otherwise.
     */
    bool IsEmpty(void) const { return mLength == 0; }

    /**
     * Gets the Joiner Discerner's value.
     *
     * @returns The Joiner Discerner value.
     */
    uint64_t GetValue(void) const { return mValue; }

    /**
     * Gets the Joiner Discerner's length (in bits).
     *
     * @return The Joiner Discerner length.
     */
    uint8_t GetLength(void) const { return mLength; }

    /**
     * Indicates whether the Joiner Discerner is valid (i.e. it not empty and its length is within
     * valid range).
     *
     * @returns TRUE if Joiner Discerner is valid, FALSE otherwise.
     */
    bool IsValid(void) const { return (0 < mLength) && (mLength <= kMaxLength); }

    /**
     * Generates a Joiner ID from the Discerner.
     *
     * @param[out] aJoinerId   A reference to `Mac::ExtAddress` to output the generated Joiner ID.
     */
    void GenerateJoinerId(Mac::ExtAddress &aJoinerId) const;

    /**
     * Indicates whether a given Joiner ID matches the Discerner.
     *
     * @param[in] aJoinerId  A Joiner ID to match with the Discerner.
     *
     * @returns TRUE if the Joiner ID matches the Discerner, FALSE otherwise.
     */
    bool Matches(const Mac::ExtAddress &aJoinerId) const;

    /**
     * Overloads operator `==` to evaluate whether or not two Joiner Discerner instances are equal.
     *
     * @param[in]  aOther  The other Joiner Discerner to compare with.
     *
     * @retval TRUE   If the two are equal.
     * @retval FALSE  If the two are not equal.
     */
    bool operator==(const JoinerDiscerner &aOther) const;

    /**
     * Converts the Joiner Discerner to a string.
     *
     * @returns An `InfoString` representation of Joiner Discerner.
     */
    InfoString ToString(void) const;

private:
    uint64_t GetMask(void) const { return (static_cast<uint64_t>(1ULL) << mLength) - 1; }
    void     CopyTo(Mac::ExtAddress &aExtAddress) const;
};

/**
 * Represents a Commissioning Dataset.
 */
class CommissioningDataset : public otCommissioningDataset, public Clearable<CommissioningDataset>
{
public:
    /**
     * Indicates whether or not the Border Router RLOC16 Locator is set in the Dataset.
     *
     * @returns TRUE if Border Router RLOC16 Locator is set, FALSE otherwise.
     */
    bool IsLocatorSet(void) const { return mIsLocatorSet; }

    /**
     * Gets the Border Router RLOC16 Locator in the Dataset.
     *
     * MUST be used when Locator is set in the Dataset, otherwise its behavior is undefined.
     *
     * @returns The Border Router RLOC16 Locator in the Dataset.
     */
    uint16_t GetLocator(void) const { return mLocator; }

    /**
     * Sets the Border Router RLOCG16 Locator in the Dataset.
     *
     * @param[in] aLocator  A Locator.
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
     */
    bool IsSessionIdSet(void) const { return mIsSessionIdSet; }

    /**
     * Gets the Session ID in the Dataset.
     *
     * MUST be used when Session ID is set in the Dataset, otherwise its behavior is undefined.
     *
     * @returns The Session ID in the Dataset.
     */
    uint16_t GetSessionId(void) const { return mSessionId; }

    /**
     * Sets the Session ID in the Dataset.
     *
     * @param[in] aSessionId  The Session ID.
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
     */
    bool IsSteeringDataSet(void) const { return mIsSteeringDataSet; }

    /**
     * Gets the Steering Data in the Dataset.
     *
     * MUST be used when Steering Data is set in the Dataset, otherwise its behavior is undefined.
     *
     * @returns The Steering Data in the Dataset.
     */
    const SteeringData &GetSteeringData(void) const { return static_cast<const SteeringData &>(mSteeringData); }

    /**
     * Returns a reference to the Steering Data in the Dataset to be updated by caller.
     *
     * @returns A reference to the Steering Data in the Dataset.
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
     */
    bool IsJoinerUdpPortSet(void) const { return mIsJoinerUdpPortSet; }

    /**
     * Gets the Joiner UDP port in the Dataset.
     *
     * MUST be used when Joiner UDP port is set in the Dataset, otherwise its behavior is undefined.
     *
     * @returns The Joiner UDP port in the Dataset.
     */
    uint16_t GetJoinerUdpPort(void) const { return mJoinerUdpPort; }

    /**
     * Sets the Joiner UDP Port in the Dataset.
     *
     * @param[in] aJoinerUdpPort  The Joiner UDP Port.
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
 */
void ComputeJoinerId(const Mac::ExtAddress &aEui64, Mac::ExtAddress &aJoinerId);

#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE

/**
 * Generates a message dump log for certification test.
 *
 * @param[in] aText     The title text to include in the log.
 * @param[in] aMessage  The message to dump the content of.
 */
void LogCertMessage(const char *aText, const Coap::Message &aMessage);

#endif

} // namespace MeshCoP

DefineCoreType(otJoinerPskd, MeshCoP::JoinerPskd);
DefineCoreType(otJoinerDiscerner, MeshCoP::JoinerDiscerner);
DefineCoreType(otCommissioningDataset, MeshCoP::CommissioningDataset);

} // namespace ot

#endif // OT_CORE_MESHCOP_MESHCOP_HPP_
