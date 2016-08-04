/*
 *  Copyright (c) 2016, Nest Labs, Inc.
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
 *   This file implements common methods for manipulating MeshCoP Datasets.
 *
 */

#include <stdio.h>

#include <common/code_utils.hpp>
#include <thread/meshcop_tlvs.hpp>
#include <thread/meshcop_dataset.hpp>

namespace Thread {
namespace MeshCoP {

Dataset::Dataset(void) :
    mLength(0)
{
    mTimestamp.Init();
}

void Dataset::Clear(void)
{
    mTimestamp.Init();
    mLength = 0;
}

Tlv *Dataset::Get(Tlv::Type aType)
{
    Tlv *cur = reinterpret_cast<Tlv *>(mTlvs);
    Tlv *end = reinterpret_cast<Tlv *>(mTlvs + mLength);
    Tlv *rval = NULL;

    while (cur < end)
    {
        if (cur->GetType() == aType)
        {
            ExitNow(rval = cur);
        }

        cur = cur->GetNext();
    }

exit:
    return rval;
}

const Tlv *Dataset::Get(Tlv::Type aType) const
{
    const Tlv *cur = reinterpret_cast<const Tlv *>(mTlvs);
    const Tlv *end = reinterpret_cast<const Tlv *>(mTlvs + mLength);
    const Tlv *rval = NULL;

    while (cur < end)
    {
        if (cur->GetType() == aType)
        {
            ExitNow(rval = cur);
        }

        cur = cur->GetNext();
    }

exit:
    return rval;
}

const Timestamp &Dataset::GetTimestamp(void) const
{
    return mTimestamp;
}

void Dataset::SetTimestamp(const Timestamp &aTimestamp)
{
    mTimestamp = aTimestamp;
}

ThreadError Dataset::Set(const Tlv &aTlv)
{
    ThreadError error = kThreadError_None;
    uint16_t bytesAvailable = sizeof(mTlvs) - mLength;
    Tlv *old = Get(aTlv.GetType());

    if (old != NULL)
    {
        bytesAvailable += sizeof(Tlv) + old->GetLength();
    }

    VerifyOrExit(sizeof(Tlv) + aTlv.GetLength() <= bytesAvailable, error = kThreadError_NoBufs);

    // remove old TLV
    if (old != NULL)
    {
        Remove(reinterpret_cast<uint8_t *>(old), sizeof(Tlv) + old->GetLength());
    }

    // add new TLV
    memcpy(mTlvs + mLength, &aTlv, sizeof(Tlv) + aTlv.GetLength());
    mLength += sizeof(Tlv) + aTlv.GetLength();

exit:
    return error;
}

ThreadError Dataset::Set(const Message &aMessage, uint16_t aOffset, uint16_t aLength)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(aLength <= kMaxSize, error = kThreadError_InvalidArgs);
    aMessage.Read(aOffset, aLength, mTlvs);
    mLength = aLength;

exit:
    return error;
}

void Dataset::Remove(Tlv::Type aType)
{
    Tlv *tlv;

    VerifyOrExit((tlv = Get(aType)) != NULL, ;);
    Remove(reinterpret_cast<uint8_t *>(tlv), sizeof(Tlv) + tlv->GetLength());

exit:
    return;
}

void Dataset::Remove(uint8_t *aStart, uint8_t aLength)
{
    memmove(aStart, aStart + aLength, mLength - ((aStart - mTlvs) + aLength));
    mLength -= aLength;
}

}  // namespace MeshCoP
}  // namespace Thread
