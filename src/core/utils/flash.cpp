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

#include <stdio.h>

#include <openthread/platform/flash.h>

#include "common/code_utils.hpp"
#include "common/instance.hpp"

#if OPENTHREAD_CONFIG_PLATFORM_FLASH_API_ENABLE

namespace ot {

enum
{
    kFlashWordSize = 4, // in bytes
};

OT_TOOL_PACKED_BEGIN
class SwapHeader
{
public:
    inline void Load(otInstance *aInstance, uint8_t aSwapIndex)
    {
        otPlatFlashRead(aInstance, aSwapIndex, 0, this, sizeof(*this));
    }

    inline void Save(otInstance *aInstance, uint8_t aSwapIndex) const
    {
        otPlatFlashWrite(aInstance, aSwapIndex, 0, this, sizeof(*this));
    }

    bool IsActive() const { return mMarker == kActive; }

    void SetActive() { mMarker = kActive; }
    void SetInactive() { mMarker = kInactive; }

    uint8_t GetSize() const { return sizeof(mMarker); }

private:
    enum : uint32_t
    {
        kActive   = 0xbe5cc5ee,
        kInactive = 0xbe5cc5ec,

        kMarkerInit = kActive,
    };

    uint32_t mMarker;
} OT_TOOL_PACKED_END;
static_assert(sizeof(SwapHeader) % kFlashWordSize == 0, "wrong SwapHeader size");

OT_TOOL_PACKED_BEGIN
class RecordHeader
{
public:
    void Init(uint16_t aKey, bool aFirst, uint16_t aLength)
    {
        mKey   = aKey;
        mFlags = kFlagsInit & ~kFlagAddBegin;

        if (aFirst)
        {
            mFlags &= ~kFlagFirst;
        }

        mLength   = aLength;
        mReserved = 0xffff;
    };

    inline void Load(otInstance *aInstance, uint8_t aSwapIndex, uint32_t aOffset)
    {
        otPlatFlashRead(aInstance, aSwapIndex, aOffset, this, sizeof(*this));
    }

    inline void Save(otInstance *aInstance, uint8_t aSwapIndex, uint32_t aOffset) const
    {
        otPlatFlashWrite(aInstance, aSwapIndex, aOffset, this, sizeof(*this));
    }

    inline uint16_t ReadData(otInstance *aInstance,
                             uint8_t     aSwapIndex,
                             uint32_t    aOffset,
                             uint8_t *   aValue,
                             uint16_t    aLength)
    {
        if (aLength > GetLength())
        {
            aLength = GetLength();
        }

        otPlatFlashRead(aInstance, aSwapIndex, aOffset + sizeof(*this), aValue, aLength);
        return aLength;
    }

    uint16_t GetKey(void) const { return mKey; }

    uint16_t GetLength(void) const { return mLength; }

    uint16_t GetDataSize(void) const { return (mLength + 3) & 0xfffc; }

    uint16_t GetSize(void) const { return sizeof(*this) + GetDataSize(); }

    bool IsValid(void) const { return ((mFlags & (kFlagAddComplete | kFlagDelete)) == kFlagDelete); }

    void SetAddBeginFlag(void) { mFlags &= ~kFlagAddBegin; }

    void SetAddCompleteFlag(void) { mFlags &= ~kFlagAddComplete; }

    void SetDeleted(void) { mFlags &= ~kFlagDelete; }

    bool IsFirst(void) const { return (mFlags & kFlagFirst) == 0; }
    void SetFirst(void) { mFlags &= ~kFlagFirst; }

    bool IsFree(void) const { return !IsAddBeginSet(); }

protected:
    bool IsIntegrityOk(void) const
    {
        OT_ASSERT(IsAddBeginSet());
        return IsAddCompleteSet();
    }

private:
    bool IsAddBeginSet(void) const { return (mFlags & kFlagAddBegin) == 0; }
    bool IsAddCompleteSet(void) const { return (mFlags & kFlagAddComplete) == 0; }

    enum
    {
        kFlagsInit       = 0xffff, ///< Flags initialize to all-ones.
        kFlagAddBegin    = 1 << 0, ///< 0 indicates record write has started, 1 otherwise.
        kFlagAddComplete = 1 << 1, ///< 0 indicates record write has completed, 1 otherwise.
        kFlagDelete      = 1 << 2, ///< 0 indicates record was deleted, 1 otherwise.
        kFlagFirst       = 1 << 3, ///< 0 indicates first record for key, 1 otherwise.
    };

    uint16_t mKey;
    uint16_t mFlags;
    uint16_t mLength;
    uint16_t mReserved;
} OT_TOOL_PACKED_END;
static_assert(sizeof(RecordHeader) % kFlashWordSize == 0, "wrong RecordHeader size");

OT_TOOL_PACKED_BEGIN
class Record : public RecordHeader
{
public:
    void Init(uint16_t aKey, bool aFirst, const uint8_t *aData, uint16_t aDataLength)
    {
        OT_ASSERT(aDataLength <= kMaxDataSize);
        RecordHeader::Init(aKey, aFirst, aDataLength);

        memcpy(mData, aData, aDataLength);
    }

    bool IsIntegrityOk(void) const { return RecordHeader::IsIntegrityOk(); }

    inline void LoadHeader(otInstance *aInstance, uint8_t aSwapIndex, uint32_t aOffset)
    {
        RecordHeader::Load(aInstance, aSwapIndex, aOffset);
    }

    inline void LoadData(otInstance *aInstance, uint8_t aSwapIndex, uint32_t aOffset)
    {
        RecordHeader::ReadData(aInstance, aSwapIndex, aOffset, mData, GetDataSize());
    }

    inline void Save(otInstance *aInstance, uint8_t aSwapIndex, uint32_t aOffset) const
    {
        otPlatFlashWrite(aInstance, aSwapIndex, aOffset, this, GetSize());
    }

    inline void SaveHeader(otInstance *aInstance, uint8_t aSwapIndex, uint32_t aOffset)
    {
        RecordHeader::Save(aInstance, aSwapIndex, aOffset);
    }

private:
    enum
    {
        kMaxDataSize = 256,
    };

    uint8_t mData[kMaxDataSize];
} OT_TOOL_PACKED_END;
static_assert(sizeof(Record) % kFlashWordSize == 0, "wrong Record size");

void Flash::Init(void)
{
    Record     record;
    SwapHeader swapHeader;

    otPlatFlashInit(&GetInstance());

    mSwapSize = otPlatFlashGetSwapSize(&GetInstance());

    for (mSwapIndex = 0;; mSwapIndex++)
    {
        if (mSwapIndex >= 2)
        {
            Wipe();
            ExitNow();
        }

        swapHeader.Load(&GetInstance(), mSwapIndex);

        if (swapHeader.IsActive())
        {
            mSwapHeaderSize = swapHeader.GetSize();
            break;
        }
    }

    for (mSwapUsed = mSwapHeaderSize; mSwapUsed <= mSwapSize - sizeof(RecordHeader); mSwapUsed += record.GetSize())
    {
        record.LoadHeader(&GetInstance(), mSwapIndex, mSwapUsed);

        if (record.IsFree())
        {
            break;
        }

        record.LoadData(&GetInstance(), mSwapIndex, mSwapUsed);

        if (!record.IsIntegrityOk())
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
    RecordHeader recordHeader;

    for (offset = mSwapHeaderSize; offset < mSwapUsed; offset += recordHeader.GetSize())
    {
        recordHeader.Load(&GetInstance(), mSwapIndex, offset);

        if ((recordHeader.GetKey() != aKey) || !recordHeader.IsValid())
        {
            continue;
        }

        if (recordHeader.IsFirst())
        {
            index = 0;
        }

        if (index == aIndex)
        {
            if (aValue && aValueLength)
            {
                recordHeader.ReadData(&GetInstance(), mSwapIndex, offset, aValue, *aValueLength);
            }

            valueLength = recordHeader.GetLength();
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
    bool first = (Get(aKey, 0, nullptr, nullptr) == OT_ERROR_NOT_FOUND);

    return Add(aKey, first, aValue, aValueLength);
}

otError Flash::Add(uint16_t aKey, bool aFirst, const uint8_t *aValue, uint16_t aValueLength)
{
    otError  error = OT_ERROR_NONE;
    Record   record;
    uint16_t size;

    record.Init(aKey, aFirst, aValue, aValueLength);
    size = record.GetSize();

    OT_ASSERT((mSwapSize - size) >= mSwapHeaderSize);

    if ((mSwapSize - size) < mSwapUsed)
    {
        Swap();
        VerifyOrExit((mSwapSize - size) >= mSwapUsed, error = OT_ERROR_NO_BUFS);
    }

    record.Save(&GetInstance(), mSwapIndex, mSwapUsed);

    record.SetAddCompleteFlag();
    record.SaveHeader(&GetInstance(), mSwapIndex, mSwapUsed);

    mSwapUsed += size;

exit:
    return error;
}

bool Flash::DoesValidRecordExist(uint32_t aOffset, uint16_t aKey) const
{
    RecordHeader recordHeader;
    bool         rval = false;

    for (; aOffset < mSwapUsed; aOffset += recordHeader.GetSize())
    {
        recordHeader.Load(&GetInstance(), mSwapIndex, aOffset);

        if (recordHeader.IsValid() && recordHeader.IsFirst() && (recordHeader.GetKey() == aKey))
        {
            ExitNow(rval = true);
        }
    }

exit:
    return rval;
}

void Flash::Swap(void)
{
    uint8_t    dstIndex  = !mSwapIndex;
    uint32_t   dstOffset = mSwapHeaderSize;
    Record     record;
    SwapHeader swapHeader;

    otPlatFlashErase(&GetInstance(), dstIndex);

    for (uint32_t srcOffset = mSwapHeaderSize; srcOffset < mSwapUsed; srcOffset += record.GetSize())
    {
        record.LoadHeader(&GetInstance(), mSwapIndex, srcOffset);

        VerifyOrExit(!record.IsFree(), OT_NOOP);

        if (!record.IsValid() || DoesValidRecordExist(srcOffset + record.GetSize(), record.GetKey()))
        {
            continue;
        }

        record.LoadData(&GetInstance(), mSwapIndex, srcOffset);
        record.Save(&GetInstance(), dstIndex, dstOffset);
        dstOffset += record.GetSize();
    }

exit:
    swapHeader.SetActive();
    swapHeader.Save(&GetInstance(), dstIndex);

    swapHeader.SetInactive();
    swapHeader.Save(&GetInstance(), mSwapIndex);

    mSwapIndex = dstIndex;
    mSwapUsed  = dstOffset;
}

otError Flash::Delete(uint16_t aKey, int aIndex)
{
    otError      error = OT_ERROR_NOT_FOUND;
    int          index = 0; // This must be initalized to 0. See [Note] below.
    RecordHeader recordHeader;

    for (uint32_t offset = mSwapHeaderSize; offset < mSwapUsed; offset += recordHeader.GetSize())
    {
        recordHeader.Load(&GetInstance(), mSwapIndex, offset);

        if ((recordHeader.GetKey() != aKey) || !recordHeader.IsValid())
        {
            continue;
        }

        if (recordHeader.IsFirst())
        {
            index = 0;
        }

        if ((aIndex == index) || (aIndex == -1))
        {
            recordHeader.SetDeleted();
            recordHeader.Save(&GetInstance(), mSwapIndex, offset);
            error = OT_ERROR_NONE;
        }

        /* [Note] If the operation gets interrupted here and aIndex is 0, the next record (index == 1) will never get
         * marked as first. However, this is not actually an issue because all the methods that iterate over the
         * settings area initialize the index to 0, without expecting any record to be effectively marked as first. */

        if ((index == 1) && (aIndex == 0))
        {
            recordHeader.SetFirst();
            recordHeader.Save(&GetInstance(), mSwapIndex, offset);
        }

        index++;
    }

    return error;
}

void Flash::Wipe(void)
{
    SwapHeader swapHeader;
    swapHeader.SetActive();

    otPlatFlashErase(&GetInstance(), 0);
    swapHeader.Save(&GetInstance(), 0);

    mSwapIndex      = 0;
    mSwapHeaderSize = swapHeader.GetSize();
    mSwapUsed       = mSwapHeaderSize;
}

} // namespace ot

#endif // OPENTHREAD_CONFIG_PLATFORM_FLASH_API_ENABLE
