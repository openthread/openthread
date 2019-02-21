/*
 *  Copyright (c) 2019, The OpenThread Authors.
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
 *   This file includes definitions for generating and processing BLE avertising and scan response data.
 */

#ifndef ADV_DATA_HPP_
#define ADV_DATA_HPP_

#include "common/code_utils.hpp"
#include <string.h>

#include <openthread/platform/ble.h>
namespace ot {
namespace Cli {

OT_TOOL_PACKED_BEGIN
class AdvDataHeader
{
public:
    enum Type
    {
        kFlags                       = 0x01,
        kIncompleteList16BitService  = 0x02,
        kCompleteList16BitService    = 0x03,
        kIncompleteList32BitService  = 0x04,
        kCompleteList32BitService    = 0x05,
        kIncompleteList128BitService = 0x06,
        kCompleteList128BitService   = 0x07,
        kShortenedLocalName          = 0x08,
        kCompleteLocalName           = 0x09,
        kServiceData                 = 0x16,
    };

    AdvDataHeader(uint8_t aType)
        : mLength(1)
        , mType(aType)
    {
    }

    void    SetType(uint8_t aType) { mType = aType; }
    uint8_t GetType(void) const { return mType; }

    void    SetLength(uint8_t aLength) { mLength = aLength + sizeof(mType); }
    uint8_t GetLength(void) const { return mLength - sizeof(mType); }

    uint8_t GetSize(void) const { return static_cast<uint8_t>(sizeof(*this) + GetLength()); }

    AdvDataHeader *GetNext(const uint8_t *aAdvDataEnd)
    {
        uint8_t *end = reinterpret_cast<uint8_t *>(this) + GetSize();
        return (end < aAdvDataEnd) ? reinterpret_cast<AdvDataHeader *>(end) : NULL;
    }

private:
    uint8_t mLength;
    uint8_t mType;
} OT_TOOL_PACKED_END;

class FlagsAdvData : public AdvDataHeader
{
public:
    enum
    {
        kLeLimitedDiscoverable = 0x01, ///< Discoverable for a limited period of time
        kLeGeneralDiscoverable = 0x02, ///< Discoverable at any moment
        kBrEdrNotSupported     = 0x04, ///< LE only and does not support Bluetooth Enhanced DataRate
    };

    FlagsAdvData(void)
        : AdvDataHeader(kFlags)
    {
        SetLength(sizeof(mFlags));
    }

    void     SetFlags(uint16_t aFlags) { mFlags = aFlags; }
    uint16_t GetFlags(void) const { return mFlags; }

private:
    uint16_t mFlags;
};

class DeviceNameAdvData : public AdvDataHeader
{
public:
    DeviceNameAdvData(void)
        : AdvDataHeader(kCompleteLocalName)
    {
    }

    const char *GetDeviceName(void) const { return mDeviceName; }
    otError     SetDeviceName(const char *aDeviceName)
    {
        otError error = OT_ERROR_NONE;
        uint8_t len;

        VerifyOrExit(aDeviceName != NULL,  error = OT_ERROR_INVALID_ARGS);

        len = static_cast<uint8_t>(strlen(aDeviceName));
        VerifyOrExit((len != 0) && (len <= kMaxDeviceNameSize), error = OT_ERROR_INVALID_ARGS);

        SetLength(len);
        memcpy(mDeviceName, aDeviceName, len);
        mDeviceName[len] = '\0';

    exit:
        return error;
    }

private:
    enum
    {
        kMaxDeviceNameSize = 29,
    };

    char mDeviceName[kMaxDeviceNameSize + 1];
};

class Uuid16AdvData : public AdvDataHeader
{
public:
    Uuid16AdvData(void)
        : AdvDataHeader(kCompleteList16BitService)
    {
    }

    otError AddUuid(const otPlatBleUuid &aUuid)
    {
        otError error = OT_ERROR_NONE;

        VerifyOrExit(aUuid.mType == OT_BLE_UUID_TYPE_16, error = OT_ERROR_INVALID_ARGS);
        VerifyOrExit(GetLength() + OT_BLE_16BIT_UUID_LENGTH <= kMaxAdvDataLength, error = OT_ERROR_NO_BUFS);

        memcpy(mBuf + GetLength(), &aUuid.mValue.mUuid16, OT_BLE_16BIT_UUID_LENGTH);
        SetLength(GetLength() + OT_BLE_16BIT_UUID_LENGTH);

    exit:
        return error;
    }

    otError RemoveUuid(otPlatBleUuid &aUuid)
    {
        otError error  = OT_ERROR_NONE;
        uint8_t offset = 0;
        uint8_t len;

        VerifyOrExit(aUuid.mType == OT_BLE_UUID_TYPE_16, error = OT_ERROR_NOT_FOUND);

        while (offset < GetLength())
        {
            if (memcmp(&aUuid.mValue.mUuid16, mBuf + offset, OT_BLE_16BIT_UUID_LENGTH) == 0)
            {
                len = GetLength() - offset - OT_BLE_16BIT_UUID_LENGTH;
                if (len != 0)
                {
                    memmove(mBuf + offset, mBuf + offset + OT_BLE_16BIT_UUID_LENGTH, len);
                }

                ExitNow();
            }

            offset += OT_BLE_16BIT_UUID_LENGTH;
        }

        error = OT_ERROR_NOT_FOUND;

    exit:
        return error;
    }

    otError GetNextUuid(uint8_t &aIterator, otPlatBleUuid &aUuid)
    {
        otError error = OT_ERROR_NONE;

        VerifyOrExit(aIterator + OT_BLE_16BIT_UUID_LENGTH <= GetLength(), error = OT_ERROR_NOT_FOUND);

        aUuid.mType = OT_BLE_UUID_TYPE_16;
        memcpy(&aUuid.mValue.mUuid16, mBuf + aIterator, OT_BLE_16BIT_UUID_LENGTH);

        aIterator += OT_BLE_16BIT_UUID_LENGTH;

    exit:
        return error;
    }

private:
    enum
    {
        kMaxAdvDataLength = 29,
    };

    uint8_t mBuf[kMaxAdvDataLength];
};

class ServiceDataAdvData : public AdvDataHeader
{
public:
    ServiceDataAdvData(void)
        : AdvDataHeader(kServiceData)
    {
        SetLength(sizeof(mUuid16));
    }

    uint16_t GetUuid16(void) const { return mUuid16; }
    void SetUuid16(uint16_t aUuid) { mUuid16 = aUuid; }

    otError SetServiceData(const uint8_t *aServiceData, uint8_t aLength)
    {
        otError error = OT_ERROR_NONE;
        VerifyOrExit(aLength <= kMaxServiceDataLength, error = OT_ERROR_NO_BUFS);

        memcpy(mServiceData, aServiceData, aLength);
        SetLength(sizeof(mUuid16) + aLength);

    exit:
        return error; 
    }

    const uint8_t *GetServiceData(void) const { return mServiceData; }
    uint8_t        GetServiceDataLength(void) { return GetLength() - sizeof(mUuid16); }

private:
    enum
    {
        kMaxServiceDataLength = 27,
    };

    uint16_t mUuid16;
    uint8_t  mServiceData[kMaxServiceDataLength];
};

class AdvData
{
public:
    AdvData(void)
        : mBufLength(0)
    {
    }

    otError Init(const uint8_t *aBuf, uint8_t aLength)
    {
        otError error = OT_ERROR_NONE;
        VerifyOrExit(aLength <= sizeof(mBuf), error = OT_ERROR_NO_BUFS);

        memcpy(mBuf, aBuf, aLength);
        mBufLength = aLength;

    exit:
        return error;
    }

    const uint8_t *GetBuf(void) const { return reinterpret_cast<const uint8_t *>(mBuf); }
    uint8_t        GetLength(void) const { return mBufLength; }

    otError AddAdvDataEntry(const AdvDataHeader *aAdvData)
    {
        otError        error = OT_ERROR_NONE;
        AdvDataHeader *entry = FindAdvDataEntry(aAdvData->GetType());

        VerifyOrExit(entry == NULL, error = OT_ERROR_FAILED);
        VerifyOrExit(mBufLength + aAdvData->GetSize() < sizeof(mBuf), error = OT_ERROR_NO_BUFS);

        memcpy(mBuf + mBufLength, aAdvData, aAdvData->GetSize());
        mBufLength += aAdvData->GetSize();

    exit:
        return error;
    }

    otError GetAdvDataEntry(uint8_t aType, AdvDataHeader *aAdvData)
    {
        otError        error = OT_ERROR_NONE;
        AdvDataHeader *entry = FindAdvDataEntry(aType);
        
        VerifyOrExit(entry != NULL, error = OT_ERROR_NOT_FOUND);

        memcpy(aAdvData, entry, entry->GetSize());

    exit:
        return error;
    }

    otError RemoveAdvDataEntry(uint8_t aType)
    {
        otError        error = OT_ERROR_NONE;
        AdvDataHeader *entry = FindAdvDataEntry(aType);
        uint8_t       *entryEnd;
        uint8_t        len;

        VerifyOrExit(entry != NULL, error = OT_ERROR_NOT_FOUND);

        entryEnd = reinterpret_cast<uint8_t *>(entry) + entry->GetSize();
        len = static_cast<uint8_t>(GetBufEnd() - entryEnd);

        memmove(entry, entryEnd, len);
        mBufLength -= len;

    exit:
        return error;
    }

private:
    enum
    {
        kAdvDataBufSize = 31, // Core 4.2, Vol3, Part C, Section 11
    };

    uint8_t *      GetBufEnd(void) { return reinterpret_cast<uint8_t *>(mBuf + mBufLength); }
    const uint8_t *GetBufEnd(void) const { return reinterpret_cast<const uint8_t *>(mBuf + mBufLength); }

    AdvDataHeader *FindAdvDataEntry(uint8_t aType)
    {
        AdvDataHeader *entry = reinterpret_cast<AdvDataHeader *>(mBuf);

        VerifyOrExit(mBufLength > 0, entry = NULL);

        while (entry != NULL)
        {
            if (entry->GetType() == aType)
            {
                ExitNow();
            }

            entry = entry->GetNext(GetBufEnd());
        }

    exit:
        return entry;
    }

    uint8_t mBuf[kAdvDataBufSize];
    uint8_t mBufLength;
};
} // namespace Cli
} // namespace ot

#endif // ADV_DATA_HPP_
