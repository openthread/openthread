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
 *   Implements Thread Direct LTV encoding and parsing for the Thread Header IE.
 */

#include "mac_header_ltv.hpp"

#include <string.h>

#include "common/code_utils.hpp"
#include "common/encoding.hpp"

namespace ot {
namespace Mac {

#if (OPENTHREAD_CONFIG_THREAD_DIRECT_WAKE_INITIATOR_ENABLE || OPENTHREAD_CONFIG_THREAD_DIRECT_WAKE_LISTENER_ENABLE) && \
    (OPENTHREAD_FTD || OPENTHREAD_MTD)

// SCA LTV fixed-header word (2-byte LE) bit positions and masks.
enum : uint16_t
{
    kScaSlotDurationShift = 0,
    kScaSlotDurationMask  = 0x0003u,
    kScaRamOffsetShift    = 2,
    kScaRamOffsetMask     = 0x07FFu,
    kScaRamOffsetMsb      = 0x0400u, ///< Sign bit of the 11-bit offset field.
    kScaRamOffsetSignExt  = 0xF800u, ///< Upper bits set when sign-extending to int16_t.
    kScaRamAvailableShift = 13,
};

Error AppendThreadHeaderIe(FrameBuilder &aFrameBuilder, const uint8_t *aLtvPayload, uint8_t aLtvLen)
{
    Error    error;
    HeaderIe ie;

    ie.Init(ThreadHeaderIe::kElementId, aLtvLen);
    SuccessOrExit(error = aFrameBuilder.AppendBytes(&ie, sizeof(ie)));

    if (aLtvLen > 0)
    {
        SuccessOrExit(error = aFrameBuilder.AppendBytes(aLtvPayload, aLtvLen));
    }

exit:
    return error;
}

Error AppendScaLtv(FrameBuilder &aFrameBuilder, const ScaParams &aParams)
{
    uint16_t fixedHdr =
        static_cast<uint16_t>((aParams.mSlotDuration & kScaSlotDurationMask) << kScaSlotDurationShift) |
        static_cast<uint16_t>((static_cast<uint16_t>(aParams.mRamOffsetUs) & kScaRamOffsetMask) << kScaRamOffsetShift) |
        (aParams.mRamAvailable ? static_cast<uint16_t>(1u << kScaRamAvailableShift) : 0u);

    uint8_t ramBitsLen = (aParams.mRamAvailable && aParams.mRamDuration > 0)
                             ? static_cast<uint8_t>((aParams.mRamDuration + 7u) / 8u)
                             : 0u;

    uint8_t valueLen =
        static_cast<uint8_t>(2u + (aParams.mRamAvailable ? (1u + ramBitsLen) : 0u) + (aParams.mHasSlw ? 4u : 0u));

    uint8_t value[39]; // 2 (fixed hdr) + 1 (RAM Duration) + 32 (RAM Bits) + 4 (SLW Period + Phase)
    uint8_t pos = 0;

    LittleEndian::WriteUint16(fixedHdr, value + pos);
    pos += 2;

    if (aParams.mRamAvailable)
    {
        value[pos++] = aParams.mRamDuration;

        for (uint8_t i = 0; i < ramBitsLen; i++)
        {
            value[pos++] = aParams.mRamBits[i];
        }
    }

    if (aParams.mHasSlw)
    {
        LittleEndian::WriteUint16(aParams.mSlwPeriodSlots, value + pos);
        pos += 2;
        LittleEndian::WriteUint16(aParams.mSlwPhaseSlots, value + pos);
        pos += 2;
    }

    return Ltv::Append(aFrameBuilder, ThreadHeaderIe::kTypeSca, value, valueLen);
}

Error AppendScaLtvTeardown(FrameBuilder &aFrameBuilder)
{
    return Ltv::Append(aFrameBuilder, ThreadHeaderIe::kTypeSca, nullptr, 0);
}

Error AppendChallengeLtv(FrameBuilder &aFrameBuilder, const ChallengeLtv &aChallenge)
{
    return Ltv::Append<ChallengeLtvInfo>(aFrameBuilder, aChallenge);
}

Error AppendTargetIdLtv(FrameBuilder &aFrameBuilder, const uint8_t *aTargetId, uint8_t aLen)
{
    return Ltv::Append(aFrameBuilder, ThreadHeaderIe::kTypeTargetId, aTargetId, aLen);
}

Error ParseThreadHeaderIe(const uint8_t *aBuffer,
                          uint8_t        aLength,
                          ScaParams     *aScaParams,
                          ChallengeLtv  *aChallenge,
                          bool          *aTeardown,
                          bool          *aChallengePresent)
{
    Error                     error = kErrorNone;
    PackedLtvStream::Iterator iter;

    iter.Init(aBuffer, aLength);

    while (!iter.IsDone())
    {
        uint8_t        ltvType  = iter.GetType();
        uint8_t        ltvLen   = iter.GetLength();
        const uint8_t *ltvValue = iter.GetValue();

        VerifyOrExit(iter.Advance() == kErrorNone, error = kErrorParse);

        if (ltvType == ThreadHeaderIe::kTypeSca)
        {
            if (ltvLen == 0)
            {
                if (aTeardown != nullptr)
                {
                    *aTeardown = true;
                }
            }
            else if (ltvLen >= 2 && aScaParams != nullptr)
            {
                uint16_t fixedHdr = LittleEndian::ReadUint16(ltvValue);
                uint16_t rawOff   = (fixedHdr >> kScaRamOffsetShift) & kScaRamOffsetMask;
                uint8_t  consumed = 2;

                memset(aScaParams, 0, sizeof(ScaParams));
                aScaParams->mSlotDuration =
                    static_cast<uint8_t>((fixedHdr >> kScaSlotDurationShift) & kScaSlotDurationMask);
                aScaParams->mRamOffsetUs  = (rawOff & kScaRamOffsetMsb)
                                                ? static_cast<int16_t>(rawOff | kScaRamOffsetSignExt)
                                                : static_cast<int16_t>(rawOff);
                aScaParams->mRamAvailable = ((fixedHdr >> kScaRamAvailableShift) & 0x01u) != 0;

                if (aScaParams->mRamAvailable && consumed < ltvLen)
                {
                    uint8_t ramDur           = ltvValue[consumed++];
                    aScaParams->mRamDuration = ramDur;
                    uint8_t ramBitsLen       = (ramDur > 0) ? static_cast<uint8_t>((ramDur + 7u) / 8u) : 0u;

                    for (uint8_t i = 0; i < ramBitsLen && consumed < ltvLen && i < ScaParams::kRamBitsMaxBytes;
                         i++, consumed++)
                    {
                        aScaParams->mRamBits[i] = ltvValue[consumed];
                    }
                }

                if (static_cast<uint8_t>(ltvLen - consumed) >= 4)
                {
                    aScaParams->mSlwPeriodSlots = LittleEndian::ReadUint16(ltvValue + consumed);
                    aScaParams->mSlwPhaseSlots  = LittleEndian::ReadUint16(ltvValue + consumed + 2u);
                    aScaParams->mHasSlw         = true;
                }
            }
        }
        else if (ltvType == ChallengeLtvInfo::kType && aChallenge != nullptr && ltvLen == sizeof(ChallengeLtv))
        {
            *aChallenge = *reinterpret_cast<const ChallengeLtv *>(ltvValue);

            if (aChallengePresent != nullptr)
            {
                *aChallengePresent = true;
            }
        }
    }

exit:
    return error;
}

#endif // (OPENTHREAD_CONFIG_THREAD_DIRECT_WAKE_INITIATOR_ENABLE ||
       // OPENTHREAD_CONFIG_THREAD_DIRECT_WAKE_LISTENER_ENABLE) && (OPENTHREAD_FTD || OPENTHREAD_MTD)

} // namespace Mac
} // namespace ot
