/*
 *  Copyright (c) 2020, The OpenThread Authors.
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

#include "flash.hpp"

#include <openthread/platform/flash.h>

#include "common/code_utils.hpp"
#include "common/instance.hpp"

namespace ot {

const uint32_t ot::Flash::sSwapActive;
const uint32_t ot::Flash::sSwapInactive;

void Flash::Init(void)
{
    RecordHeader record;

    otPlatFlashInit(&GetInstance());

    mSwapSize = otPlatFlashGetSwapSize(&GetInstance());

    for (mSwapIndex = 0;; mSwapIndex++)
    {
        uint32_t swapMarker;

        if (mSwapIndex >= 2)
        {
            Wipe();
            ExitNow();
        }

        otPlatFlashRead(&GetInstance(), mSwapIndex, 0, &swapMarker, sizeof(swapMarker));

        if (swapMarker == sSwapActive)
        {
            break;
        }
    }

    for (mSwapUsed = kSwapMarkerSize; mSwapUsed <= mSwapSize - sizeof(record); mSwapUsed += record.GetSize())
    {
        record.ReadHeader(GetInstance(), mSwapIndex, mSwapUsed);

        if (!record.IsAddBeginSet())
        {
            break;
        }

        if (!record.IsAddCompleteSet())
        {
            break;
        }
    }

    SanitizeFreeSpace();

exit:
    return;
}

void Flash::SanitizeFreeSpace(void)
{
    uint32_t temp;
    bool     sanitizeNeeded = false;

    if (mSwapUsed & 3)
    {
        ExitNow(sanitizeNeeded = true);
    }

    for (uint32_t offset = mSwapUsed; offset < mSwapSize; offset += sizeof(temp))
    {
        otPlatFlashRead(&GetInstance(), mSwapIndex, offset, &temp, sizeof(temp));
        if (temp != ~0U)
        {
            ExitNow(sanitizeNeeded = true);
        }
    }

exit:
    if (sanitizeNeeded)
    {
        Swap();
    }
}

otError Flash::Get(uint16_t aKey, int aIndex, uint8_t *aValue, uint16_t *aValueLength) const
{
    otError      error       = OT_ERROR_NOT_FOUND;
    uint16_t     valueLength = 0;
    int          index       = 0; // This must be initalized to 0. See [Note] in Delete().
    uint32_t     offset;
    RecordHeader record;

    for (offset = kSwapMarkerSize; offset < mSwapUsed; offset += record.GetSize())
    {
        record.ReadHeader(GetInstance(), mSwapIndex, offset);

        if (record.GetKey() != aKey)
        {
            continue;
        }

        if (record.IsFirst())
        {
            index = 0;
        }

        if (!record.IsValid())
        {
            continue;
        }

        if (index == aIndex)
        {
            if (aValue && aValueLength)
            {
                uint16_t readLength = *aValueLength;

                if (readLength > record.GetLength())
                {
                    readLength = record.GetLength();
                }

                record.ReadData(GetInstance(), mSwapIndex, offset, aValue, readLength);
            }

            valueLength = record.GetLength();
            error       = OT_ERROR_NONE;
        }

        index++;
    }

    if (aValueLength)
    {
        *aValueLength = valueLength;
    }

    return error;
}

otError Flash::Set(uint16_t aKey, const uint8_t *aValue, uint16_t aValueLength)
{
    return Add(aKey, true, aValue, aValueLength);
}

otError Flash::Add(uint16_t aKey, const uint8_t *aValue, uint16_t aValueLength)
{
    bool first = (Get(aKey, 0, NULL, NULL) == OT_ERROR_NOT_FOUND);

    return Add(aKey, first, aValue, aValueLength);
}

otError Flash::Add(uint16_t aKey, bool aFirst, const uint8_t *aValue, uint16_t aValueLength)
{
    otError error = OT_ERROR_NONE;
    Record  record;

    record.Init(aKey, aFirst);
    record.SetData(aValue, aValueLength);

    OT_ASSERT((mSwapSize - record.GetSize()) >= kSwapMarkerSize);

    if ((mSwapSize - record.GetSize()) < mSwapUsed)
    {
        Swap();
        VerifyOrExit((mSwapSize - record.GetSize()) >= mSwapUsed, error = OT_ERROR_NO_BUFS);
    }

    record.Write(GetInstance(), mSwapIndex, mSwapUsed);
    record.SetAddCompleteFlag();
    record.WriteProgressMarkers(GetInstance(), mSwapIndex, mSwapUsed);

    mSwapUsed += record.GetSize();

exit:
    return error;
}

bool Flash::DoesValidRecordExist(uint32_t aOffset, uint16_t aKey) const
{
    RecordHeader record;
    bool         rval = false;

    for (; aOffset < mSwapUsed; aOffset += record.GetSize())
    {
        record.ReadHeader(GetInstance(), mSwapIndex, aOffset);

        if (record.IsAddCompleteSet() && record.IsFirst() && (record.GetKey() == aKey))
        {
            ExitNow(rval = true);
        }
    }

exit:
    return rval;
}

void Flash::Swap(void)
{
    uint8_t  dstIndex  = !mSwapIndex;
    uint32_t dstOffset = kSwapMarkerSize;
    Record   record;

    otPlatFlashErase(&GetInstance(), dstIndex);

    for (uint32_t srcOffset = kSwapMarkerSize; srcOffset < mSwapUsed; srcOffset += record.GetSize())
    {
        record.ReadHeader(GetInstance(), mSwapIndex, srcOffset);

        VerifyOrExit(record.IsAddBeginSet(), OT_NOOP);

        if (!record.IsValid() || DoesValidRecordExist(srcOffset + record.GetSize(), record.GetKey()))
        {
            continue;
        }

        record.Read(GetInstance(), mSwapIndex, srcOffset);
        record.Write(GetInstance(), dstIndex, dstOffset);
        dstOffset += record.GetSize();
    }

exit:
    otPlatFlashWrite(&GetInstance(), dstIndex, 0, &sSwapActive, sizeof(sSwapActive));
    otPlatFlashWrite(&GetInstance(), mSwapIndex, 0, &sSwapInactive, sizeof(sSwapInactive));

    mSwapIndex = dstIndex;
    mSwapUsed  = dstOffset;
}

otError Flash::Delete(uint16_t aKey, int aIndex)
{
    otError      error = OT_ERROR_NOT_FOUND;
    int          index = 0;
    RecordHeader record;

    for (uint32_t offset = kSwapMarkerSize; offset < mSwapUsed; offset += record.GetSize())
    {
        record.ReadHeader(GetInstance(), mSwapIndex, offset);

        if (record.GetKey() != aKey)
        {
            continue;
        }

        if (record.IsFirst())
        {
            index = 0;
        }

        if (!record.IsValid())
        {
            continue;
        }

        if ((aIndex == index) || (aIndex == -1))
        {
            record.SetDeleted();
            record.WriteFirstAndDeletedFlags(GetInstance(), mSwapIndex, offset);
            error = OT_ERROR_NONE;
        }

        index++;
    }

    return error;
}

void Flash::Wipe(void)
{
    otPlatFlashErase(&GetInstance(), 0);
    otPlatFlashWrite(&GetInstance(), 0, 0, &sSwapActive, sizeof(sSwapActive));

    mSwapIndex = 0;
    mSwapUsed  = sizeof(sSwapActive);
}

void Flash::Record::Write(Instance &aInstance, uint8_t aSwapIndex, uint32_t aOffset)
{
    otPlatFlashWrite(&aInstance, aSwapIndex, aOffset, this, GetSize());
}

void Flash::RecordHeader::WriteProgressMarkers(Instance &aInstance, uint8_t aSwapIndex, uint32_t aOffset)
{
    otPlatFlashWrite(&aInstance, aSwapIndex, aOffset + offsetof(RecordHeader, mWord1), &mWord1, sizeof(mWord1));
}

void Flash::RecordHeader::WriteFirstAndDeletedFlags(Instance &aInstance, uint8_t aSwapIndex, uint32_t aOffset)
{
    otPlatFlashWrite(&aInstance, aSwapIndex, aOffset + offsetof(RecordHeader, mWord2), &mWord2, sizeof(mWord2));
}

void Flash::RecordHeader::ReadHeader(Instance &aInstance, uint8_t aSwapIndex, uint32_t aOffset)
{
    otPlatFlashRead(&aInstance, aSwapIndex, aOffset, this, sizeof(*this));
}

void Flash::RecordHeader::ReadData(Instance &aInstance,
                                   uint8_t   aSwapIndex,
                                   uint32_t  aOffset,
                                   void *    aData,
                                   uint32_t  aSize)
{
    otPlatFlashRead(&aInstance, aSwapIndex, aOffset + sizeof(*this), aData, aSize);
}

void Flash::Record::Read(Instance &aInstance, uint8_t aSwapIndex, uint32_t aOffset)
{
    otPlatFlashRead(&aInstance, aSwapIndex, aOffset, this, GetSize());
}

} // namespace ot
