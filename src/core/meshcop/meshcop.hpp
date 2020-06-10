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

#include <limits.h>

#include <openthread/instance.h>

#include "coap/coap.hpp"
#include "common/message.hpp"
#include "mac/mac_types.hpp"
#include "meshcop/meshcop_tlvs.hpp"

namespace ot {

class ThreadNetif;

namespace MeshCoP {

enum
{
    kBorderAgentUdpPort = 49191, ///< UDP port of border agent service.
};

/**
 * This type represents Steering Data (bloom filter).
 *
 */
class SteeringData : public otSteeringData
{
public:
    enum
    {
        kMaxLength = OT_STEERING_DATA_MAX_LENGTH, ///< Maximum Steering Data length (in bytes).
    };

    /**
     * This structure represents the hash bit index values for the bloom filter calculated from a Joiner ID.
     *
     * The first hash bit index is derived using CRC16-CCITT and second one using CRC16-ANSI.
     *
     */
    struct HashBitIndexes
    {
        enum
        {
            kNumIndexes = 2, ///< Number of hash bit indexes.
        };

        uint16_t mIndex[kNumIndexes]; ///< The hash bit index array.
    };

    /**
     * This method initializes the Steering Data and clears the bloom filter.
     *
     * @param[in]  aLength   The Steering Data length (in bytes) - MUST be smaller than or equal to `kMaxLength`.
     *
     */
    void Init(uint8_t aLength = kMaxLength);

    /**
     * This method clears the bloom filter (all bits are cleared and no Joiner Id is accepted)..
     *
     * The Steering Data length (bloom filter length) is set to one byte with all bits cleared.
     *
     */
    void Clear(void) { Init(1); }

    /**
     * This method sets the bloom filter to permit all Joiner IDs.
     *
     * To permit all Joiner IDs, The Steering Data length (bloom filter length) is set to one byte with all bits set.
     *
     */
    void SetToPermitAllJoiners(void);

    /**
     * This method returns the Steering Data length (in bytes).
     *
     * @returns The Steering Data length (in bytes).
     *
     */
    uint8_t GetLength(void) const { return mLength; }

    /**
     * This method gets the Steering Data buffer (bloom filter).
     *
     * @returns A pointer to the Steering Data buffer.
     *
     */
    const uint8_t *GetData(void) const { return m8; }

    /**
     * This method gets the Steering Data buffer (bloom filter).
     *
     * @returns A pointer to the Steering Data buffer.
     *
     */
    uint8_t *GetData(void) { return m8; }

    /**
     * This method updates the bloom filter adding the given Joiner ID.
     *
     * @param[in]  aJoinerId  The Joiner ID to add to bloom filter.
     *
     */
    void UpdateBloomFilter(const Mac::ExtAddress &aJoinerId);

    /**
     * This method indicates whether the bloom filter is empty (all the bits are cleared).
     *
     * @returns TRUE if the bloom filter is empty, FALSE otherwise.
     *
     */
    bool IsEmpty(void) const { return DoesAllMatch(0); }

    /**
     * This method indicates whether the bloom filter permits all Joiner IDs (all the bits are set).
     *
     * @returns TRUE if the bloom filter permits all Joiners IDs, FALSE otherwise.
     *
     */
    bool PermitsAllJoiners(void) const { return (mLength > 0) && DoesAllMatch(kPermitAll); }

    /**
     * This method indicates whether the bloom filter contains a given Joiner ID.
     *
     * @param[in] aJoinderId  A Joiner ID.
     *
     * @returns TRUE if the bloom filter contains @p aJoinerId, FALSE otherwise.
     *
     */
    bool Contains(const Mac::ExtAddress &aJoinerId) const;

    /**
     * This method indicates whether the bloom filter contains the hash bit indexes (derived from a Joiner ID).
     *
     * @param[in]  aIndexes   A hash bit index structure (derived from a Joiner ID).
     *
     * @returns TRUE if the bloom filter contains the Joiner ID mapping to @p aIndexes, FALSE otherwise.
     *
     */
    bool Contains(const HashBitIndexes &aIndexes) const;

    /**
     * This static method calculates the bloom filter hash bit indexes from a given Joiner ID.
     *
     * The first hash bit index is derived using CRC16-CCITT and second one using CRC16-ANSI.
     *
     * @param[in]  aJoinerId  The Joiner ID to calculate the hash bit indexes.
     * @param[out] aIndexes   A reference to a `HashBitIndexes` structure to output the calculated index values.
     *
     */
    static void CalculateHashBitIndexes(const Mac::ExtAddress &aJoinerId, HashBitIndexes &aIndexes);

private:
    enum
    {
        kPermitAll = 0xff,
    };

    uint8_t GetNumBits(void) const { return (mLength * CHAR_BIT); }

    uint8_t BitIndex(uint8_t aBit) const { return (mLength - 1 - (aBit / CHAR_BIT)); }
    uint8_t BitFlag(uint8_t aBit) const { return static_cast<uint8_t>(1U << (aBit % CHAR_BIT)); };

    bool GetBit(uint8_t aBit) const { return (m8[BitIndex(aBit)] & BitFlag(aBit)) != 0; }
    void SetBit(uint8_t aBit) { m8[BitIndex(aBit)] |= BitFlag(aBit); }
    void ClearBit(uint8_t aBit) { m8[BitIndex(aBit)] &= ~BitFlag(aBit); }

    bool DoesAllMatch(uint8_t aMatch) const;
};

/**
 * This function creates Message for MeshCoP.
 *
 */
inline Coap::Message *NewMeshCoPMessage(Coap::CoapBase &aCoap)
{
    return aCoap.NewMessage(Message::Settings(Message::kWithLinkSecurity, Message::kPriorityNet));
}

/**
 * This function generates PSKc.
 *
 * PSKc is used to establish the Commissioner Session.
 *
 * @param[in]  aPassPhrase   The commissioning passphrase.
 * @param[in]  aNetworkName  The network name for PSKc computation.
 * @param[in]  aExtPanId     The extended PAN ID for PSKc computation.
 * @param[out] aPskc         A reference to a PSKc where the generated PSKc will be placed.
 *
 * @retval OT_ERROR_NONE          Successfully generate PSKc.
 * @retval OT_ERROR_INVALID_ARGS  If the length of passphrase is out of range.
 *
 */
otError GeneratePskc(const char *              aPassPhrase,
                     const Mac::NetworkName &  aNetworkName,
                     const Mac::ExtendedPanId &aExtPanId,
                     Pskc &                    aPskc);

/**
 * This function computes the Joiner ID from a factory-assigned IEEE EUI-64.
 *
 * @param[in]   aEui64     The factory-assigned IEEE EUI-64.
 * @param[out]  aJoinerId  The Joiner ID.
 *
 */
void ComputeJoinerId(const Mac::ExtAddress &aEui64, Mac::ExtAddress &aJoinerId);

/**
 * This function gets the border agent RLOC.
 *
 * @param[in]   aNetif  A reference to the thread interface.
 * @param[out]  aRloc   Border agent RLOC.
 *
 * @retval OT_ERROR_NONE        Successfully got the Border Agent Rloc.
 * @retval OT_ERROR_NOT_FOUND   Border agent is not available.
 *
 */
otError GetBorderAgentRloc(ThreadNetif &aNetIf, uint16_t &aRloc);

#if OPENTHREAD_CONFIG_JOINER_ENABLE || OPENTHREAD_CONFIG_COMMISSIONER_ENABLE
/**
 * This method validates the PSKd.
 *
 * Per Thread specification, a Joining Device Credential is encoded as
 * uppercase alphanumeric characters (base32-thread: 0-9, A-Z excluding
 * I, O, Q, and Z for readability) with a minimum length of 6 such
 * characters and a maximum length of 32 such characters.
 *
 * param[in]  aPskd  The PSKd to validate.
 *
 * @retval A boolean indicates whether the given @p aPskd is valid.
 *
 */
bool IsPskdValid(const char *aPskd);
#endif // OPENTHREAD_CONFIG_JOINER_ENABLE || OPENTHREAD_CONFIG_COMMISSIONER_ENABLE

} // namespace MeshCoP

} // namespace ot

#endif // MESHCOP_HPP_
