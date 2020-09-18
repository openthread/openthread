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
    kFlashWordSizeV1 = 4, // in bytes
    kFlashWordSizeV2 = 8, // in bytes
};

enum
{
    kFormatV1 = 0,
    kFormatV2 = 1,
};

OT_TOOL_PACKED_BEGIN
class SwapHeader
{
public:
    inline void Init(void)
    {
        mMarker   = kMarkerInit;
        mReserved = 0xffffffff;
    }

    inline void Load(otInstance *aInstance, uint8_t aSwapIndex)
    {
        otPlatFlashRead(aInstance, aSwapIndex, 0, this, sizeof(*this));
    }

    inline void Save(otInstance *aInstance, uint8_t aSwapIndex) const
    {
        otPlatFlashWrite(aInstance, aSwapIndex, 0, this, sizeof(*this));
    }

    bool IsActive(void) const { return (mMarker & kActiveMask) == kActive; }

    void SetInactive(void)
    {
        mMarker   = ~mMarker;
        mReserved = 0xffffffff;
    }

    uint8_t GetSize(void) const { return GetFormat() == kFormatV1 ? sizeof(mMarker) : sizeof(*this); }

    uint8_t GetFormat(void) const { return kFormatTop - (mMarker >> kFormatShift); }

private:
    enum : uint32_t
    {
        kActive = 0x0e5cc5ee,

        kActiveMask = 0xfffffff,

        kFormatTop   = 0xb,
        kFormatShift = 28,

        kMarkerInit = ((uint32_t)(kFormatTop - kFormatV2) << kFormatShift) | kActive,
    };

    uint32_t mMarker;
    uint32_t mReserved;
} OT_TOOL_PACKED_END;
static_assert(sizeof(SwapHeader) % kFlashWordSizeV2 == 0, "wrong SwapHeader size");

OT_TOOL_PACKED_BEGIN
class RecordHeader
{
protected:
    void Init(uint16_t aKey, uint16_t aLength)
    {
        mKey      = aKey;
        mFlags    = kFlagsInit;
        mLength   = aLength;
        mReserved = 0xffff;
    };

public:
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

    uint16_t GetDataSize(void) const
    {
        uint16_t wordMask = GetAlignmentMask();
        return (mLength + wordMask) & ~wordMask;
    }

    static uint16_t GetAlignmentMask(uint8_t aFormat)
    {
        return (aFormat == kFormatV1) ? (kFlashWordSizeV1 - 1) : (kFlashWordSizeV2 - 1);
    }

    uint16_t GetSize(void) const { return sizeof(*this) + GetDataSize(); }

    bool IsValid(void) const { return ((mFlags & (kFlagAddComplete | kFlagDelete)) == kFlagDelete); }

    void SetDeleted(void)
    {
        memset(this, 0xff, sizeof(*this));
        mFlags &= ~kFlagDelete;
    }

    bool IsFirst(void) const { return (mFlags & kFlagFirst) == 0; }

    bool IsFree(void) const { return !IsAddBeginSet(); }

protected:
    bool IsIntegrityOk(void) const
    {
        OT_ASSERT(IsAddBeginSet());
        return IsAddCompleteSet();
        // TODO: validate GetFormat() == 0 && mReserved == 0xffff;
    }

    uint8_t GetFormat(void) const { return kFormatTop - (mFlags >> kFormatShift); }

private:
    uint16_t GetAlignmentMask(void) const { return GetAlignmentMask(GetFormat()); }

    bool IsAddBeginSet(void) const { return (mFlags & kFlagAddBegin) == 0; }
    bool IsAddCompleteSet(void) const { return (mFlags & kFlagAddComplete) == 0; }

    enum
    {
        kFlagAddBegin    = 1 << 0, ///< 0 indicates record write has started, 1 otherwise.
        kFlagAddComplete = 1 << 1, ///< 0 indicates record write has completed, 1 otherwise.
        kFlagDelete      = 1 << 2, ///< 0 indicates record was deleted, 1 otherwise.
        kFlagFirst       = 1 << 3, ///< 0 indicates first record for key, 1 otherwise.

        /* The record's format version is in the upper nibble of mFlags */
        kFormatTop   = 0xf,
        kFormatShift = 12,

        /* Initial value of flags: kFlagAddBegin, kFlagAddComplete and format version */
        kFlagsInit = ((kFormatTop - kFormatV2) << kFormatShift) | (0xfff & ~(kFlagAddBegin | kFlagAddComplete)),
    };

    uint16_t mKey;
    uint16_t mFlags;
    uint16_t mLength;
    uint16_t mReserved;
} OT_TOOL_PACKED_END;
static_assert(sizeof(RecordHeader) % kFlashWordSizeV2 == 0, "wrong RecordHeader size");

OT_TOOL_PACKED_BEGIN
class Record : public RecordHeader
{
public:
    void Init(uint16_t aKey, const uint8_t *aData, uint16_t aDataLength)
    {
        OT_ASSERT(aDataLength <= kMaxDataSize);
        RecordHeader::Init(aKey, aDataLength);

        memcpy(mData, aData, aDataLength);

        /* fill slack space with 0xff to avoid writing 0 more than once to each bit */
        memset(mData + aDataLength, 0xff, GetDataSize() - aDataLength);

        // TODO: calculate CRC
    }

    bool IsIntegrityOk(void) const
    {
        if (!RecordHeader::IsIntegrityOk())
        {
            return false;
        }

        if (GetFormat() >= kFormatV2)
        {
            // TODO: check CRC
        }

        return true;
    }

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

    void Fixup(void)
    {
        uint16_t dataLength = GetLength();
        RecordHeader::Init(GetKey(), dataLength);

        /* fill slack space with 0xff to avoid writing 0 more than once to each bit */
        memset(mData + dataLength, 0xff, GetDataSize() - dataLength);

        // TODO: calculate CRC
    }

private:
    enum
    {
        kMaxDataSize = 256,
    };

    uint8_t mData[kMaxDataSize];
} OT_TOOL_PACKED_END;
static_assert(sizeof(Record) % kFlashWordSizeV2 == 0, "wrong Record size");

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

    mFormat = swapHeader.GetFormat();

    for (mSwapUsed = mSwapHeaderSize; mSwapUsed <= mSwapSize - sizeof(RecordHeader); mSwapUsed += record.GetSize())
    {
        record.LoadHeader(&GetInstance(), mSwapIndex, mSwapUsed);

        if (record.IsFree())
        {
            break;
        }

        record.LoadData(&GetInstance(), mSwapIndex, mSwapUsed);

        if (!record.IsIntegrityOk(/* TODO expect format == mFormat */))
        {
            break;
        }
    }

    SanitizeFreeSpace();

exit:
    return;
}

void Flash::SanitizeFreeSpace()
{
    uint32_t temp;
    bool     sanitizeNeeded = false;
    uint16_t wordMask       = RecordHeader::GetAlignmentMask(mFormat);

    if ((mFormat == kFormatV1) || (mSwapUsed & wordMask))
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
    int          index       = 0;
    uint32_t     offset;
    RecordHeader recordHeader;

    for (offset = mSwapHeaderSize; offset < mSwapUsed; offset += recordHeader.GetSize())
    {
        recordHeader.Load(&GetInstance(), mSwapIndex, offset);

        if ((recordHeader.GetKey() != aKey) || !recordHeader.IsValid())
        {
            continue;
        }

        if (index == aIndex)
        {
            if (aValue && aValueLength)
            {
                recordHeader.ReadData(&GetInstance(), mSwapIndex, offset, aValue, *aValueLength);
            }

            valueLength = recordHeader.GetLength();
            error       = OT_ERROR_NONE;
            break;
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
    IgnoreError(Delete(aKey, -1));
    return Add(aKey, aValue, aValueLength);
}

otError Flash::Add(uint16_t aKey, const uint8_t *aValue, uint16_t aValueLength)
{
    otError  error = OT_ERROR_NONE;
    Record   record;
    uint16_t size;

    record.Init(aKey, aValue, aValueLength);
    size = record.GetSize();

    OT_ASSERT((mSwapSize - size) >= mSwapHeaderSize);

    if ((mSwapSize - size) < mSwapUsed)
    {
        Swap();
        VerifyOrExit((mSwapSize - size) >= mSwapUsed, error = OT_ERROR_NO_BUFS);
    }

    record.Save(&GetInstance(), mSwapIndex, mSwapUsed);

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
    SwapHeader swapHeader;
    uint8_t    dstIndex  = !mSwapIndex;
    uint32_t   dstOffset = sizeof(swapHeader);
    Record     record;
    uint16_t   size;

    otPlatFlashErase(&GetInstance(), dstIndex);

    for (uint32_t srcOffset = mSwapHeaderSize; srcOffset < mSwapUsed; srcOffset += size)
    {
        record.LoadHeader(&GetInstance(), mSwapIndex, srcOffset);

        VerifyOrExit(!record.IsFree(), OT_NOOP);

        size = record.GetSize();

        if (!record.IsValid() || DoesValidRecordExist(srcOffset + record.GetSize(), record.GetKey()))
        {
            continue;
        }

        record.LoadData(&GetInstance(), mSwapIndex, srcOffset);
        record.Fixup();
        record.Save(&GetInstance(), dstIndex, dstOffset);
        dstOffset += record.GetSize();
    }

exit:
    swapHeader.Init();
    swapHeader.Save(&GetInstance(), dstIndex);

    swapHeader.Load(&GetInstance(), mSwapIndex);
    if (swapHeader.GetFormat() == kFormatV1)
    {
        otPlatFlashErase(&GetInstance(), mSwapIndex);
    }
    else
    {
        swapHeader.SetInactive();
        swapHeader.Save(&GetInstance(), mSwapIndex);
    }

    mSwapIndex      = dstIndex;
    mSwapUsed       = dstOffset;
    mSwapHeaderSize = sizeof(swapHeader);
}

otError Flash::Delete(uint16_t aKey, int aIndex)
{
    otError      error = OT_ERROR_NOT_FOUND;
    int          index = 0;
    RecordHeader recordHeader;
    uint16_t     size;

    for (uint32_t offset = mSwapHeaderSize; offset < mSwapUsed; offset += size)
    {
        recordHeader.Load(&GetInstance(), mSwapIndex, offset);

        size = recordHeader.GetSize();

        if ((recordHeader.GetKey() != aKey) || !recordHeader.IsValid())
        {
            continue;
        }

        if ((aIndex == index) || (aIndex == -1))
        {
            recordHeader.SetDeleted();
            recordHeader.Save(&GetInstance(), mSwapIndex, offset);
            error = OT_ERROR_NONE;

            if (aIndex != -1)
            {
                break;
            }
        }

        index++;
    }

    return error;
}

void Flash::Wipe(void)
{
    SwapHeader swapHeader;

    otPlatFlashErase(&GetInstance(), 0);

    swapHeader.Init();
    swapHeader.Save(&GetInstance(), 0);

    mSwapIndex      = 0;
    mSwapHeaderSize = swapHeader.GetSize();
    mSwapUsed       = sizeof(swapHeader);
}

} // namespace ot

#endif // OPENTHREAD_CONFIG_PLATFORM_FLASH_API_ENABLE
