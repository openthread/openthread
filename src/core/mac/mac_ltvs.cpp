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

/**
 * @file
 *   This file implements support for LTVs.
 */

#include "mac_ltvs.hpp"

#include "common/bit_utils.hpp"
#include "common/numeric_limits.hpp"

namespace ot {
namespace Mac {

//---------------------------------------------------------------------------------------------------------------------
// Ltv::ParsedInfo

Error Ltv::ParsedInfo::ParseFromAndAdvance(FrameData &aFrameData)
{
    Error   error = kErrorNone;
    bool    packed;
    uint8_t lenBitSize;
    uint8_t headerByte;

    if (aFrameData.IsEmpty())
    {
        error = kErrorNotFound;
        ExitNow();
    }

    IgnoreError(aFrameData.ReadUint8(headerByte));

    if (aFrameData.IsEmpty())
    {
        // If no bytes remain after reading the header byte, it
        // corresponds to a zero-length packed LTV, where all 8 bits
        // of `headerByte` represent the Type. Note that no escape
        // mask check applies here, so even Type value 255 (all 1s)
        // is valid and packed.

        mLength = 0;
        mType   = headerByte;
        mValue  = aFrameData.GetBytes();
        ExitNow();
    }

    packed = false;

    lenBitSize = DetermineMinBitSizeFor(aFrameData.GetLength());

    if (lenBitSize >= BitSizeOf(uint8_t))
    {
        mLength = headerByte;
    }
    else
    {
        uint8_t lenMask  = MaskForBitSize<uint8_t>(lenBitSize);
        uint8_t typeMask = static_cast<uint8_t>(~lenMask);

        mLength = static_cast<uint8_t>(headerByte & lenMask);

        // Check if the Type bits match the escape mask (all 1s in
        // upper bits), which indicates that the LTV uses base
        // (unpacked) format.

        if ((headerByte & typeMask) != typeMask)
        {
            mType  = static_cast<uint8_t>((headerByte & typeMask) >> lenBitSize);
            packed = true;
        }
    }

    if (!packed)
    {
        SuccessOrExit(error = aFrameData.ReadUint8(mType));
    }

    VerifyOrExit(aFrameData.CanRead(mLength), error = kErrorParse);

    mValue = aFrameData.GetBytes();
    aFrameData.SkipOver(mLength);

exit:
    return error;
}

Error Ltv::ParsedInfo::FindIn(const FrameData &aFrameData, uint8_t aType)
{
    FrameData frameData = aFrameData;

    return FindInAndAdvance(frameData, aType);
}

Error Ltv::ParsedInfo::FindInAndAdvance(FrameData &aFrameData, uint8_t aType)
{
    Error error;

    do
    {
        SuccessOrExit(error = ParseFromAndAdvance(aFrameData));
    } while (mType != aType);

exit:
    return error;
}

//---------------------------------------------------------------------------------------------------------------------
// Ltv::AppendInfo

void Ltv::AppendInfo::InitTypeLengthId(uint8_t aType, uint8_t aLength, uint16_t aId)
{
    mType   = aType;
    mLength = aLength;
    mValue  = nullptr;
    mId     = aId;
}

bool Ltv::AppendInfo::CanUsePacked(uint8_t aLenBitSize) const
{
    // Determines whether the LTV can be encoded in packed format given
    // the length field bit size `aLenBitSize`.

    bool    canPack = false;
    uint8_t remainingBitSize;

    VerifyOrExit(aLenBitSize < BitSizeOf(uint8_t));

    remainingBitSize = BitSizeOf(uint8_t) - aLenBitSize;

    VerifyOrExit(DetermineMinBitSizeFor(mType) <= remainingBitSize);

    // Type value of all `1` bits cannot be used in the packed format,
    // since that value is the "escape mask" that indicates base LTV
    // format is used.

    VerifyOrExit(mType != MaskForBitSize<uint8_t>(remainingBitSize));

    canPack = true;

exit:
    return canPack;
}

void Ltv::AppendInfo::DetermineIfPackable(uint32_t &aTotalLength)
{
    // Determines whether the LTV can be encoded in packed format
    // based on its Type value and the cumulative length at this point
    // (after this LTV). Also updates `aTotalLength` to include the
    // total encoded size of the LTV.

    uint8_t lenBitSize;

    aTotalLength += mLength;

    if (aTotalLength == 0)
    {
        // If no bytes remain after reading the header byte, it
        // corresponds to the special zero-length packed LTV format
        // where 8 bits of `headerByte` represent the Type.
        // Importantly, the escape mask check does not apply, so even
        // Type value 255 (all 1s) is packed.

        mIsPackable = true;
        mHeaderByte = mType;
        aTotalLength++;
        ExitNow();
    }

    lenBitSize = DetermineMinBitSizeFor(aTotalLength);

    if (CanUsePacked(lenBitSize))
    {
        mIsPackable = true;
        mHeaderByte = static_cast<uint8_t>(mLength | (mType << lenBitSize));
        aTotalLength++;
        ExitNow();
    }

    // Base format is used, which requires a separate 1-byte Type field.
    // We account for the extra byte in `aTotalLength` and re-evaluate
    // `lenBitSize` before using it to set the escape mask (all 1s in
    // upper bits).

    aTotalLength++;
    lenBitSize = DetermineMinBitSizeFor(aTotalLength);

    mIsPackable = false;
    mHeaderByte = static_cast<uint8_t>(mLength | ~MaskForBitSize<uint8_t>(lenBitSize));
    aTotalLength++;

exit:
    return;
}

//---------------------------------------------------------------------------------------------------------------------
// Ltv

Error Ltv::ValidateAllIn(const FrameData &aFrameData)
{
    FrameData  frameData = aFrameData;
    ParsedInfo ltvInfo;
    Error      error;

    do
    {
        error = ltvInfo.ParseFromAndAdvance(frameData);
    } while (error == kErrorNone);

    return (error == kErrorNotFound) ? kErrorNone : error;
}

Error Ltv::EncodeAndAppend(AppendInfo *aLtvList, uint16_t aNumLtvs, FrameBuilder &aBuilder)
{
    Error    error       = kErrorNone;
    uint32_t totalLength = 0;

    VerifyOrExit(aNumLtvs > 0);

    // We first iterate backward from last to first LTV to evaluate
    // packability based on cumulative length. Then append all LTVs
    // in `aBuilder`.

    for (uint16_t i = aNumLtvs; i > 0; i--)
    {
        aLtvList[i - 1].DetermineIfPackable(totalLength);
    }

    for (uint16_t i = 0; i < aNumLtvs; i++)
    {
        AppendInfo *ltv = &aLtvList[i];

        SuccessOrExit(error = aBuilder.AppendUint8(ltv->mHeaderByte));

        if (!ltv->mIsPackable)
        {
            SuccessOrExit(error = aBuilder.AppendUint8(ltv->mType));
        }

        ltv->mValue = static_cast<uint8_t *>(aBuilder.AppendLength(ltv->mLength));
        VerifyOrExit(ltv->mValue != nullptr, error = kErrorNoBufs);
    }

exit:
    return error;
}

void Ltv::OptimizeListOrder(AppendInfo *aLtvList, uint16_t aNumLtvs)
{
    uint32_t curLength = 0;

    VerifyOrExit(aNumLtvs > 1);

    // We determine the optimal order backward from the last LTV position
    // (`aNumLtvs - 1`) down to the first (`0`). At each position `curLtv`,
    // we evaluate all candidate entries from `aLtvList[0]` up to `curLtv`
    // given the cumulative length `curLength` of the entries already placed
    // after `curLtv`. We select the candidate that can be packed and yields
    // the smallest cumulative length, placing it into `curLtv`.

    for (AppendInfo *curLtv = &aLtvList[aNumLtvs - 1]; curLtv > &aLtvList[0]; curLtv--)
    {
        AppendInfo *bestLtv    = nullptr;
        uint32_t    bestLength = NumericLimits<uint32_t>::kMax;

        for (AppendInfo *ltv = &aLtvList[0]; ltv <= curLtv; ltv++)
        {
            uint32_t length   = curLength;
            bool     isBetter = true;

            ltv->DetermineIfPackable(length);

            // We compare candidate `ltv` against the current `bestLtv`:
            //
            // - If `bestLtv` is packable (`bestLtv->mIsPackable` is true),
            //   candidate `ltv` is preferred only if it is also packable and
            //   yields a smaller or equal cumulative length (`length <=
            //   bestLength`). Using `<=` prefers later entries on ties,
            //   preserving the caller's original list order.
            //
            // - Otherwise (`bestLtv` is null or unpackable), we always prefer
            //   the new candidate `ltv`: If `ltv` is packable, it improves
            //   packability; if `ltv` is also unpackable, preferring a later
            //   candidate preserves the caller's original list order.

            if ((bestLtv != nullptr) && bestLtv->mIsPackable)
            {
                isBetter = ltv->mIsPackable && (length <= bestLength);
            }

            if (isBetter)
            {
                bestLtv    = ltv;
                bestLength = length;
            }
        }

        // Move `bestLtv` to position `curLtv` by shifting the elements
        // between `bestLtv + 1` and `curLtv` one position up. Using a shift
        // rather than a direct swap preserves the relative order of all
        // remaining entries.

        if ((bestLtv != nullptr) && (bestLtv != curLtv))
        {
            AppendInfo best = *bestLtv;

            for (AppendInfo *ltv = bestLtv; ltv < curLtv; ltv++)
            {
                *ltv = ltv[1];
            }

            *curLtv = best;
        }

        curLength = bestLength;
    }

exit:
    return;
}

} // namespace Mac
} // namespace ot
