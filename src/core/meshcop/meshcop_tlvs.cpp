/*
 *  Copyright (c) 2016-2017, The OpenThread Authors.
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
 *   This file implements MechCop TLV helper functions.
 */

#include "meshcop_tlvs.hpp"

namespace ot {
namespace MeshCoP {

bool SteeringDataTlv::IsCleared(void) const
{
    bool rval = true;

    for (uint8_t i = 0; i < GetLength(); i++)
    {
        if (mSteeringData[i] != 0)
        {
            rval = false;
            break;
        }
    }

    return rval;
}

void SteeringDataTlv::ComputeBloomFilter(otExtAddress *aExtAddress)
{
    Crc16 ccitt(Crc16::kCcitt);
    Crc16 ansi(Crc16::kAnsi);

    for (size_t j = 0; j < sizeof(otExtAddress); j++)
    {
        uint8_t byte = aExtAddress->m8[j];
        ccitt.Update(byte);
        ansi.Update(byte);
    }

    SetBit(ccitt.Get() % GetNumBits());
    SetBit(ansi.Get() % GetNumBits());
}

}  // namespace MeshCoP
}  // namespace ot
