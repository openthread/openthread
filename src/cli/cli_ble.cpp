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
 *   This file implements cli BLE.
 *
 */

#include <openthread/platform/ble-host.h>

#include "cli_ble.hpp"
#include "cli_server.hpp"
#include "cli/cli.hpp"

#include "common/code_utils.hpp"
#include "common/instance.hpp"

#include <string.h>

#include <openthread/platform/ble.h>

namespace ot {
namespace Cli {

class AdvDataHeader
{
public:
    enum
    {
        kAdTypeFlags                       = 0x01,
        kAdTypeIncompleteList16BitService  = 0x02,
        kAdTypeCompleteList16BitService    = 0x03,
        kAdTypeIncompleteList32BitService  = 0x04,
        kAdTypeCompleteList32BitService    = 0x05,
        kAdTypeIncompleteList128BitService = 0x06,
        kAdTypeCompleteList128BitService   = 0x07,
        kAdTypeShortenedLocalName          = 0x08,
        kAdTypeCompleteLocalName           = 0x09,
        kAdTypeAppearance                  = 0x19,
    };

    explicit AdvDataHeader(uint8_t aType)
        : mLength(0)
        , mType(aType)
    {
    }

    void    SetType(uint8_t aType) { mType = aType; }
    uint8_t GetType(void) const { return mType; }

    void    SetLength(uint8_t aLength) { mLength = aLength; }
    uint8_t GetLength(void) const { return mLength; }

private:
    uint8_t mLength;
    uint8_t mType;
};

class AdvDataEntry : public AdvDataHeader
{
public:
    explicit AdvDataEntry(uint8_t aType)
        : AdvDataHeader(aType)
    {
    }

    uint8_t GetSize(void) const { return static_cast<uint8_t>(sizeof(*this) + GetLength()); }

    void SetData(const uint8_t *aAdData, uint8_t aAdDataLength) { memcpy(GetData(), aAdData, aAdDataLength); }

    uint8_t *      GetData(void) { return reinterpret_cast<uint8_t *>(this) + sizeof(*this); }
    const uint8_t *GetData(void) const { return reinterpret_cast<const uint8_t *>(this) + sizeof(*this); }

    uint8_t *      GetDataEnd(void) { return reinterpret_cast<uint8_t *>(this) + GetSize(); }
    const uint8_t *GetDataEnd(void) const { return reinterpret_cast<const uint8_t *>(this) + GetSize(); }

    AdvDataEntry *GetNext(const uint8_t *aAdvDataEnd)
    {
        return (GetDataEnd() < aAdvDataEnd) ? reinterpret_cast<AdvDataEntry *>(GetDataEnd()) : NULL;
    }
};

enum
{
    kFlagsLeLimitedDiscoverable         = 0x01, ///< Discoverable for a limited period of time
    kFlagsLeGeneralDiscoverable         = 0x02, ///< Discoverable at any moment
    kFlagsBrEdrNotSupported             = 0x04, ///< LE only and does not support Bluetooth Enhanced DataRate
    kFlagsSimultaneousLeBrEdrController = 0x08, ///< Not relevant - dual mode only
    kFlagsSimultaneousLeBrEdrHost       = 0x10, ///< Not relevant - dual mode only

    kFlagsDefault = kFlagsLeGeneralDiscoverable | kFlagsBrEdrNotSupported, ///< Defalut flags configuration
};

class AdvData
{
public:
    AdvData(void)
        : mBufLength(0)
    {
    }

    const uint8_t *GetBuf(void) const { return reinterpret_cast<const uint8_t *>(mBuf); }
    uint8_t        GetLength(void) const { return mBufLength; }

    otError SetFlags(uint16_t aFlags = kFlagsDefault)
    {
        return SetAdvDataEntry(AdvDataHeader::kAdTypeFlags, reinterpret_cast<const uint8_t *>(&aFlags), sizeof(aFlags));
    }

    otError SetLocalServiceList(const otPlatBleUuid *aUuid, uint8_t aNumUuid, bool aComplete = true)
    {
        uint8_t shortType;
        uint8_t longType;

        if (aComplete)
        {
            shortType = AdvDataHeader::kAdTypeCompleteList16BitService;
            longType  = AdvDataHeader::kAdTypeCompleteList128BitService;
        }
        else
        {
            shortType = AdvDataHeader::kAdTypeIncompleteList16BitService;
            longType  = AdvDataHeader::kAdTypeIncompleteList128BitService;
        }

        return SetUuidData(aUuid, aNumUuid, shortType, longType);
    }

    otError SetDeviceName(const char *aDeviceName, bool aComplete = true)
    {
        uint8_t type = aComplete ? AdvDataHeader::kAdTypeCompleteLocalName : AdvDataHeader::kAdTypeShortenedLocalName;
        return SetAdvDataEntry(type, reinterpret_cast<const uint8_t *>(aDeviceName),
                               static_cast<uint8_t>(strlen(aDeviceName)));
    }

private:
    enum
    {
        kAdvDataBufSize = 128,
    };

    uint8_t *      GetBufEnd(void) { return reinterpret_cast<uint8_t *>(mBuf + mBufLength); }
    const uint8_t *GetBufEnd(void) const { return reinterpret_cast<const uint8_t *>(mBuf + mBufLength); }

    otError SetAdvDataEntry(uint8_t aType, const uint8_t *aAdData, uint8_t aAdDataLength)
    {
        otError       error = OT_ERROR_NONE;
        AdvDataEntry *entry = reinterpret_cast<AdvDataEntry *>(GetBufEnd());

        VerifyOrExit(mBufLength + sizeof(AdvDataHeader) + aAdDataLength < sizeof(mBuf), error = OT_ERROR_NO_BUFS);

        entry->SetType(aType);
        memcpy(entry->GetData(), aAdData, aAdDataLength);
        entry->SetLength(aAdDataLength);
        mBufLength += entry->GetSize();

    exit:
        return error;
    }

    otError AppendAdvDataEntry(uint8_t aType, const uint8_t *aAdData, uint8_t aAdDataLength)
    {
        otError       error = OT_ERROR_NONE;
        AdvDataEntry *entry = FindAdvDataEntry(aType);
        uint8_t *     entryEnd;

        VerifyOrExit(entry != NULL, error = OT_ERROR_NOT_FOUND);
        VerifyOrExit(mBufLength + aAdDataLength < sizeof(mBuf), error = OT_ERROR_NO_BUFS);

        entryEnd = entry->GetDataEnd();
        memmove(entryEnd + aAdDataLength, entryEnd, static_cast<size_t>(GetBufEnd() - entryEnd));
        memcpy(entry->GetData(), aAdData, aAdDataLength);
        entry->SetLength(entry->GetLength() + aAdDataLength);
        mBufLength += aAdDataLength;

    exit:
        return error;
    }

    otError RemoveAdvDataEntry(uint8_t aType)
    {
        otError       error = OT_ERROR_NONE;
        AdvDataEntry *entry = FindAdvDataEntry(aType);
        uint8_t       len;

        VerifyOrExit(entry != NULL, error = OT_ERROR_NOT_FOUND);

        len = static_cast<uint8_t>(GetBufEnd() - entry->GetDataEnd());

        memmove(entry, entry->GetDataEnd(), len);
        mBufLength -= len;

    exit:
        return error;
    }

    AdvDataEntry *FindAdvDataEntry(uint8_t aType)
    {
        AdvDataEntry *entry = reinterpret_cast<AdvDataEntry *>(mBuf);

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

    otError SetUuidData(const otPlatBleUuid *aUuid, uint8_t aNumUuid, uint8_t aShortType, uint8_t aLongType)
    {
        otError error            = OT_ERROR_NONE;
        uint8_t shortUuidDataLen = 0;
        uint8_t longUuidDataLen  = 0;
        uint8_t headerSize;
        uint8_t i;

        for (i = 0; i < aNumUuid; i++)
        {
            if (aUuid[i].mType == OT_BLE_UUID_TYPE_16)
            {
                shortUuidDataLen += OT_BLE_16BIT_UUID_LENGTH;
            }
            else if (aUuid[i].mType == OT_BLE_UUID_TYPE_128)
            {
                longUuidDataLen += OT_BLE_UUID_LENGTH;
            }
            else
            {
                ExitNow(error = OT_ERROR_INVALID_ARGS);
            }
        }

        headerSize = (shortUuidDataLen ? sizeof(AdvDataHeader) : 0) + (longUuidDataLen ? sizeof(AdvDataHeader) : 0);
        VerifyOrExit(headerSize + shortUuidDataLen + longUuidDataLen <= static_cast<uint8_t>(sizeof(mBuf) - mBufLength),
                     error = OT_ERROR_NO_BUFS);

        RemoveAdvDataEntry(aShortType);
        RemoveAdvDataEntry(aLongType);

        for (i = 0; i < aNumUuid; i++)
        {
            AdvDataEntry * entry;
            uint8_t        type;
            const uint8_t *uuid;
            uint8_t        uuidLen;

            if (aUuid[i].mType == OT_BLE_UUID_TYPE_16)
            {
                type    = aShortType;
                uuid    = reinterpret_cast<const uint8_t *>(&aUuid[i].mValue.mUuid16);
                uuidLen = OT_BLE_16BIT_UUID_LENGTH;
            }
            else
            {
                type    = aLongType;
                uuid    = reinterpret_cast<const uint8_t *>(aUuid[i].mValue.mUuid128);
                uuidLen = OT_BLE_UUID_LENGTH;
            }

            if ((entry = FindAdvDataEntry(type)) == NULL)
            {
                SetAdvDataEntry(type, uuid, uuidLen);
            }
            else
            {
                AppendAdvDataEntry(type, uuid, uuidLen);
            }
        }

    exit:
        return error;
    }

    uint8_t mBuf[kAdvDataBufSize];
    uint8_t mBufLength;
};

const struct Ble::Command Ble::sCommands[] = {{"help", &Ble::ProcessHelp},       {"enable", &Ble::ProcessEnable},
                                              {"disable", &Ble::ProcessDisable}, {"advertise", &Ble::ProcessAdvertise},
                                              {"scan", &Ble::ProcessScan},       {"addr", &Ble::ProcessAddr}};

Ble::Ble(Interpreter &aInterpreter)
    : mInterpreter(aInterpreter)
{
}

otError Ble::Process(int argc, char *argv[])
{
    otError error = OT_ERROR_PARSE;

    if (argc < 1)
    {
        ProcessHelp(0, NULL);
        error = OT_ERROR_INVALID_ARGS;
    }
    else
    {
        for (size_t i = 0; i < OT_ARRAY_LENGTH(sCommands); i++)
        {
            if (strcmp(argv[0], sCommands[i].mName) == 0)
            {
                error = (this->*sCommands[i].mCommand)(argc - 1, argv + 1);
                break;
            }
        }
    }
    return error;
}

otError Ble::ProcessHelp(int argc, char *argv[])
{
    for (unsigned int i = 0; i < OT_ARRAY_LENGTH(sCommands); i++)
    {
        mInterpreter.mServer->OutputFormat("%s\r\n", sCommands[i].mName);
    }

    OT_UNUSED_VARIABLE(argc);
    OT_UNUSED_VARIABLE(argv);

    return OT_ERROR_NONE;
}

otError Ble::ProcessEnable(int argc, char *argv[])
{
    otError error;

    VerifyOrExit(argc == 0, error = OT_ERROR_INVALID_ARGS);
    error = otPlatBleEnable(mInterpreter.mInstance);

    OT_UNUSED_VARIABLE(argv);

exit:
    return error;
}

otError Ble::ProcessDisable(int argc, char *argv[])
{
    otError error;

    VerifyOrExit(argc == 0, error = OT_ERROR_INVALID_ARGS);
    error = otPlatBleDisable(mInterpreter.mInstance);

    OT_UNUSED_VARIABLE(argv);

exit:
    return error;
}

otError Ble::ProcessAdvertise(int argc, char *argv[])
{
    otError error;

    VerifyOrExit(argc == 1, error = OT_ERROR_INVALID_ARGS);

    if (strcmp(argv[0], "start") == 0)
    {
        AdvData       advData;
        otPlatBleUuid uuid      = {.mType = OT_BLE_UUID_TYPE_16, {.mUuid16 = 0xA000}};
        char          devName[] = "OT_TOBLE";

        advData.SetFlags();
        advData.SetDeviceName(devName);
        advData.SetLocalServiceList(&uuid, 1);

        error = otPlatBleGapAdvDataSet(mInterpreter.mInstance, advData.GetBuf(), advData.GetLength());
        error = otPlatBleGapScanResponseSet(mInterpreter.mInstance, advData.GetBuf(), advData.GetLength());
        error =
            otPlatBleGapAdvStart(mInterpreter.mInstance, 0xA0, OT_BLE_ADV_MODE_CONNECTABLE | OT_BLE_ADV_MODE_SCANNABLE);
    }
    else if (strcmp(argv[0], "stop") == 0)
    {
        error = otPlatBleGapAdvStop(mInterpreter.mInstance);
    }
    else
    {
        error = OT_ERROR_INVALID_ARGS;
    }

    OT_UNUSED_VARIABLE(argv);

exit:
    return error;
}

otError Ble::ProcessScan(int argc, char *argv[])
{
    otError error;

    VerifyOrExit(argc == 1, error = OT_ERROR_INVALID_ARGS);

    if (strcmp(argv[0], "start") == 0)
    {
        error = otPlatBleGapScanStart(mInterpreter.mInstance, 0xA0, 0x50);
    }
    else if (strcmp(argv[0], "stop") == 0)
    {
        error = otPlatBleGapScanStop(mInterpreter.mInstance);
    }
    else
    {
        error = OT_ERROR_INVALID_ARGS;
    }

exit:
    return error;
}

otError Ble::ProcessAddr(int argc, char *argv[])
{
    otPlatBleDeviceAddr addr;
    otError             error;

    VerifyOrExit(argc == 0, error = OT_ERROR_INVALID_ARGS);

    if ((error = otPlatBleGapAddressGet(mInterpreter.mInstance, &addr)) == OT_ERROR_NONE)
    {
        mInterpreter.mServer->OutputFormat("%02x ", addr.mAddr[0], addr.mAddr[1], addr.mAddr[2], addr.mAddr[3],
                                           addr.mAddr[4], addr.mAddr[5]);
    }

    OT_UNUSED_VARIABLE(argv);

exit:
    return error;
}

void Ble::OutputBytes(const uint8_t *aBytes, uint8_t aLength) const
{
    for (int i = 0; i < aLength; i++)
    {
        mInterpreter.mServer->OutputFormat("%02x ", aBytes[i]);
    }
}

} // namespace Cli
} // namespace ot
