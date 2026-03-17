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
 *   This file implements MeshCoP Steering Data.
 */

#include "steering_data.hpp"

#include "common/clearable.hpp"
#include "common/code_utils.hpp"
#include "common/crc.hpp"
#include "common/num_utils.hpp"
#include "meshcop/meshcop.hpp"

namespace ot {
namespace MeshCoP {

Error SteeringData::Init(uint8_t aLength)
{
    Error error = kErrorNone;

    VerifyOrExit(IsValueInRange(aLength, kMinLength, kMaxLength), error = kErrorInvalidArgs);

    mLength = aLength;
    ClearAllBytes(m8);

exit:
    return error;
}

Error SteeringData::Init(uint8_t aLength, const uint8_t *aData)
{
    Error error;

    SuccessOrExit(error = Init(aLength));
    memcpy(m8, aData, mLength);

exit:
    return error;
}

void SteeringData::SetToPermitAllJoiners(void)
{
    IgnoreError(Init(1));
    m8[0] = kPermitAll;
}

Error SteeringData::UpdateBloomFilter(const Mac::ExtAddress &aJoinerId)
{
    HashBitIndexes indexes;

    CalculateHashBitIndexes(aJoinerId, indexes);

    return UpdateBloomFilter(indexes);
}

Error SteeringData::UpdateBloomFilter(const JoinerDiscerner &aDiscerner)
{
    HashBitIndexes indexes;

    CalculateHashBitIndexes(aDiscerner, indexes);

    return UpdateBloomFilter(indexes);
}

Error SteeringData::UpdateBloomFilter(const HashBitIndexes &aIndexes)
{
    Error error = kErrorNone;

    VerifyOrExit(IsValid(), error = kErrorInvalidArgs);

    SetBit(aIndexes.mIndex[0] % GetNumBits());
    SetBit(aIndexes.mIndex[1] % GetNumBits());

exit:
    return error;
}

Error SteeringData::MergeBloomFilterWith(const SteeringData &aOther)
{
    Error error = kErrorNone;

    VerifyOrExit(IsValid(), error = kErrorInvalidArgs);
    VerifyOrExit(aOther.IsValid(), error = kErrorInvalidArgs);

    VerifyOrExit(GetLength() % aOther.GetLength() == 0, error = kErrorInvalidArgs);

    for (uint8_t index = 0; index < GetLength(); index++)
    {
        m8[index] |= aOther.m8[index % aOther.GetLength()];
    }

exit:
    return error;
}

bool SteeringData::Contains(const Mac::ExtAddress &aJoinerId) const
{
    HashBitIndexes indexes;

    CalculateHashBitIndexes(aJoinerId, indexes);

    return Contains(indexes);
}

bool SteeringData::Contains(const JoinerDiscerner &aDiscerner) const
{
    HashBitIndexes indexes;

    CalculateHashBitIndexes(aDiscerner, indexes);

    return Contains(indexes);
}

bool SteeringData::Contains(const HashBitIndexes &aIndexes) const
{
    bool contains = false;

    VerifyOrExit(IsValid());
    contains = GetBit(aIndexes.mIndex[0] % GetNumBits()) && GetBit(aIndexes.mIndex[1] % GetNumBits());

exit:
    return contains;
}

bool SteeringData::operator==(const SteeringData &aOther) const
{
    return (GetLength() == aOther.GetLength()) && (memcmp(GetData(), aOther.GetData(), GetLength()) == 0);
}

void SteeringData::CalculateHashBitIndexes(const Mac::ExtAddress &aJoinerId, HashBitIndexes &aIndexes)
{
    aIndexes.mIndex[0] = CrcCalculator<uint16_t>(kCrc16CcittPolynomial).Feed(aJoinerId);
    aIndexes.mIndex[1] = CrcCalculator<uint16_t>(kCrc16AnsiPolynomial).Feed(aJoinerId);
}

void SteeringData::CalculateHashBitIndexes(const JoinerDiscerner &aDiscerner, HashBitIndexes &aIndexes)
{
    Mac::ExtAddress address;

    address.Clear();
    aDiscerner.CopyTo(address);

    CalculateHashBitIndexes(address, aIndexes);
}

bool SteeringData::DoesAllMatch(uint8_t aMatch) const
{
    bool matches = false;

    VerifyOrExit(IsValid());

    for (uint8_t i = 0; i < mLength; i++)
    {
        if (m8[i] != aMatch)
        {
            ExitNow();
        }
    }

    matches = true;

exit:
    return matches;
}

SteeringData::InfoString SteeringData::ToString(void) const
{
    InfoString string;

    string.Append("[");
    string.AppendHexBytes(GetData(), Min(GetLength(), kMaxLength));
    string.Append("]");

    return string;
}

} // namespace MeshCoP
} // namespace ot
