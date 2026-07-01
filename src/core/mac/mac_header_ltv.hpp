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

/**
 * @file
 *   Thread Direct LTV (Length-Type-Value) types and encoding helpers for the Thread Header IE.
 *
 *   The Thread Header IE (IEEE 802.15.4 Header IE, Element ID 0x2d) carries a compressed
 *   LTV payload.  This file defines the per-LTV type structs (`ScaParams`, `ChallengeLtv`),
 *   the `PackedLtvStream`-backed encode/decode wrappers, and the high-level
 *   `AppendScaLtv` / `ParseThreadHeaderIe` helpers used by the Thread Direct MAC layer.
 *
 *   For the generic LTV encoding primitives (`Ltv`, `PackedLtvStream`, `SimpleLtvInfo`)
 *   see `common/ltvs.hpp`.
 */

#ifndef OT_CORE_MAC_MAC_HEADER_LTV_HPP_
#define OT_CORE_MAC_MAC_HEADER_LTV_HPP_

#include "openthread-core-config.h"

#include "common/frame_builder.hpp"
#include "common/ltvs.hpp"
#include "mac/mac_header_ie.hpp"

namespace ot {
namespace Mac {

#if OPENTHREAD_CONFIG_THREAD_DIRECT_WAKE_INITIATOR_ENABLE || OPENTHREAD_CONFIG_THREAD_DIRECT_WAKE_LISTENER_ENABLE

/**
 * In-memory representation of SCA LTV parameters.
 *
 * Wire format (value bytes after Type):
 *   2B fixed header (LE): bits[1:0]=SlotDuration, bits[12:2]=RamOffset, bit[13]=RamAvailable, bits[15:14]=RSV
 *   If mRamAvailable: 1B RamDuration + ceil(mRamDuration/8) RAM Bits bytes
 *   If mHasSlw: 2B SLW Period (LE) + 2B SLW Phase (LE)
 *   Teardown: zero-length value (mIsTeardown flag; AppendScaLtvTeardown emits this form).
 */
struct ScaParams
{
    static constexpr uint8_t kRamBitsMaxBytes = 32; ///< Maximum RAM bitmap size (256 bits).
    static constexpr int16_t kRamOffsetUsMin  = -1024;
    static constexpr int16_t kRamOffsetUsMax  = 1023;

    uint16_t mSlwPeriodSlots; ///< SLW period in slot-duration units (0 = rx-on-when-idle); valid when mHasSlw.
    uint16_t mSlwPhaseSlots;  ///< SLW phase in slot-duration units; valid when mHasSlw.
    int16_t  mRamOffsetUs;    ///< RAM Offset in us, signed [-1024, 1023].
    uint8_t  mRamDuration;    ///< Number of bits in mRamBits (0 = no change); valid when mRamAvailable.
    uint8_t  mSlotDuration;   ///< Slot duration code: 0=625us, 1=1.25ms, 2=625ms, 3=1.25s.
    bool     mRamAvailable;   ///< True if RAM Duration and RAM Bits are present in the SCA LTV.
    bool     mHasSlw;         ///< True if SLW Period and Phase are present in the SCA LTV.
    uint8_t  mRamBits[kRamBitsMaxBytes]; ///< RAM bitmap; ceil(mRamDuration/8) bytes valid when mRamAvailable.
};

/**
 * Holds the 16-byte challenge value carried in a Thread Direct Challenge LTV.
 */
struct ChallengeLtv
{
    static constexpr uint8_t kLength = 16; ///< Challenge value length in bytes (truncated HMAC-SHA256).
    uint8_t                  mChallenge[kLength];
};

/// Type alias for the Thread Direct Challenge LTV; used with `Ltv::Append<ChallengeLtvInfo>()`.
using ChallengeLtvInfo = SimpleLtvInfo<ThreadHeaderIe::kTypeChallenge, ChallengeLtv>;

/**
 * Appends a Thread Header IE (Element ID 0x2d) containing the given LTV payload to a FrameBuilder.
 *
 * @param[in,out] aFrameBuilder  The FrameBuilder to append to.
 * @param[in]     aLtvPayload    Pointer to the serialised LTV bytes to place inside the IE.
 * @param[in]     aLtvLen        Length of aLtvPayload in bytes (must fit in 7-bit IE length field).
 *
 * @retval kErrorNone   Successfully appended.
 * @retval kErrorNoBufs Insufficient space in the FrameBuilder.
 */
Error AppendThreadHeaderIe(FrameBuilder &aFrameBuilder, const uint8_t *aLtvPayload, uint8_t aLtvLen);

/**
 * Appends an SCA LTV to a FrameBuilder.
 *
 * @param[in,out] aFrameBuilder  The FrameBuilder to append to.
 * @param[in]     aParams        SCA parameters to encode.
 *
 * @retval kErrorNone   Successfully appended.
 * @retval kErrorNoBufs Insufficient space in the FrameBuilder.
 */
Error AppendScaLtv(FrameBuilder &aFrameBuilder, const ScaParams &aParams);

/**
 * Appends a zero-length (teardown) SCA LTV to a FrameBuilder.
 *
 * A zero-length SCA LTV signals TD link teardown to the receiver.
 *
 * @param[in,out] aFrameBuilder  The FrameBuilder to append to.
 *
 * @retval kErrorNone   Successfully appended.
 * @retval kErrorNoBufs Insufficient space in the FrameBuilder.
 */
Error AppendScaLtvTeardown(FrameBuilder &aFrameBuilder);

/**
 * Appends a Challenge LTV to a FrameBuilder.
 *
 * @param[in,out] aFrameBuilder  The FrameBuilder to append to.
 * @param[in]     aChallenge     The 16-byte challenge value to encode.
 *
 * @retval kErrorNone   Successfully appended.
 * @retval kErrorNoBufs Insufficient space in the FrameBuilder.
 */
Error AppendChallengeLtv(FrameBuilder &aFrameBuilder, const ChallengeLtv &aChallenge);

/**
 * Appends a Target ID LTV to a FrameBuilder.
 *
 * @param[in,out] aFrameBuilder  The FrameBuilder to append to.
 * @param[in]     aTargetId      Pointer to the Target ID bytes.
 * @param[in]     aLen           Length of aTargetId in bytes.
 *
 * @retval kErrorNone   Successfully appended.
 * @retval kErrorNoBufs Insufficient space in the FrameBuilder.
 */
Error AppendTargetIdLtv(FrameBuilder &aFrameBuilder, const uint8_t *aTargetId, uint8_t aLen);

/// Convenience wrapper around `PackedLtvStream::Encode`. Prefer calling `PackedLtvStream::Encode` directly.
uint8_t PackThreadHeaderIeLtvs(const uint8_t *aPlain, uint8_t aPlainLen, uint8_t *aPacked, uint8_t aPackedMax);

/// Convenience wrapper around `PackedLtvStream::Decode`. Prefer calling `PackedLtvStream::Decode` directly.
Error UnpackThreadHeaderIeLtvs(const uint8_t *aPacked,
                               uint8_t        aPackedLen,
                               uint8_t       *aPlain,
                               uint8_t        aPlainMax,
                               uint8_t       &aPlainLen);

/**
 * Parses the LTV payload of a Thread Header IE.
 *
 * Scans the packed LTV sequence; for each LTV whose type matches a known Thread Direct type the
 * corresponding output parameter is populated when non-null.  Unknown types are skipped silently.
 * A zero-length SCA LTV (teardown signal) sets @p aTeardown when non-null.
 *
 * @param[in]  aBuffer           Pointer to the IE payload (bytes after the 2-byte HeaderIe header).
 * @param[in]  aLength           Length of @p aBuffer in bytes.
 * @param[out] aScaParams        If non-null, populated when a non-empty SCA LTV is found.
 * @param[out] aChallenge        If non-null, populated when a Challenge LTV is found.
 * @param[out] aTeardown         If non-null, set to true when a zero-length (teardown) SCA LTV is found.
 * @param[out] aChallengePresent If non-null, set to true when a Challenge LTV is found.
 *
 * @retval kErrorNone   Parsing succeeded.
 * @retval kErrorParse  Buffer is truncated or malformed.
 */
Error ParseThreadHeaderIe(const uint8_t *aBuffer,
                          uint8_t        aLength,
                          ScaParams     *aScaParams,
                          ChallengeLtv  *aChallenge,
                          bool          *aTeardown         = nullptr,
                          bool          *aChallengePresent = nullptr);

#endif // OPENTHREAD_CONFIG_THREAD_DIRECT_WAKE_INITIATOR_ENABLE || OPENTHREAD_CONFIG_THREAD_DIRECT_WAKE_LISTENER_ENABLE

} // namespace Mac
} // namespace ot

#endif // OT_CORE_MAC_MAC_HEADER_LTV_HPP_
