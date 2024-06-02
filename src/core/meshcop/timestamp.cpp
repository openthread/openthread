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
 *   This file implements common MeshCoP timestamp processing.
 */

#include "timestamp.hpp"

#include "common/code_utils.hpp"
#include "common/num_utils.hpp"
#include "common/numeric_limits.hpp"

namespace ot {
namespace MeshCoP {

void Timestamp::ConvertTo(Info &aInfo) const
{
    aInfo.mSeconds       = GetSeconds();
    aInfo.mTicks         = GetTicks();
    aInfo.mAuthoritative = GetAuthoritative();
}

void Timestamp::SetFrom(const Info &aInfo)
{
    SetSeconds(aInfo.mSeconds);
    SetTicks(aInfo.mTicks);
    SetAuthoritative(aInfo.mAuthoritative);
}

void Timestamp::SetToInvalid(void)
{
    mSeconds16        = NumericLimits<uint16_t>::kMax;
    mSeconds32        = NumericLimits<uint32_t>::kMax;
    mTicksAndAuthFlag = NumericLimits<uint16_t>::kMax;
}

bool Timestamp::IsValid(void) const
{
    return (mSeconds16 != NumericLimits<uint16_t>::kMax) || (mSeconds32 != NumericLimits<uint32_t>::kMax) ||
           (mTicksAndAuthFlag != NumericLimits<uint16_t>::kMax);
}

void Timestamp::SetToOrphanAnnounce(void)
{
    mSeconds16 = 0;
    mSeconds32 = 0;
    SetTicksAndAuthFlag(kAuthoritativeFlag);
}

bool Timestamp::IsOrphanAnnounce(void) const
{
    return (mSeconds16 == 0) && (mSeconds32 == 0) && (GetTicksAndAuthFlag() == kAuthoritativeFlag);
}

uint64_t Timestamp::GetSeconds(void) const
{
    return (static_cast<uint64_t>(BigEndian::HostSwap16(mSeconds16)) << 32) + BigEndian::HostSwap32(mSeconds32);
}

void Timestamp::SetSeconds(uint64_t aSeconds)
{
    mSeconds16 = BigEndian::HostSwap16(static_cast<uint16_t>(aSeconds >> 32));
    mSeconds32 = BigEndian::HostSwap32(static_cast<uint32_t>(aSeconds & 0xffffffff));
}

void Timestamp::SetTicks(uint16_t aTicks)
{
    SetTicksAndAuthFlag((GetTicksAndAuthFlag() & ~kTicksMask) | ((aTicks << kTicksOffset) & kTicksMask));
}

void Timestamp::SetAuthoritative(bool aAuthoritative)
{
    SetTicksAndAuthFlag((GetTicksAndAuthFlag() & kTicksMask) | (aAuthoritative ? kAuthoritativeFlag : 0));
}

int Timestamp::Compare(const Timestamp &aFirst, const Timestamp &aSecond)
{
    int rval;

    rval = ThreeWayCompare(aFirst.IsValid(), aSecond.IsValid());
    VerifyOrExit(rval == 0);

    rval = ThreeWayCompare(aFirst.GetSeconds(), aSecond.GetSeconds());
    VerifyOrExit(rval == 0);

    rval = ThreeWayCompare(aFirst.GetTicks(), aSecond.GetTicks());
    VerifyOrExit(rval == 0);

    rval = ThreeWayCompare(aFirst.GetAuthoritative(), aSecond.GetAuthoritative());

exit:
    return rval;
}

void Timestamp::AdvanceRandomTicks(void)
{
    uint16_t ticks = GetTicks();

    ticks += Random::NonCrypto::GetUint32InRange(1, kMaxTicks + 1);

    if (ticks > kMaxTicks)
    {
        ticks -= (kMaxTicks + 1);
        SetSeconds(GetSeconds() + 1);
    }

    SetTicks(ticks);
}

} // namespace MeshCoP
} // namespace ot
