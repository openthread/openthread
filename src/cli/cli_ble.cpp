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
#include "cli/adv_data.hpp"
#include "cli/cli.hpp"

#include "common/code_utils.hpp"
#include "common/instance.hpp"

#include <string.h>

#include <openthread/platform/ble.h>

namespace ot {
namespace Cli {

const struct Ble::Command Ble::sCommands[] = {{"help", &Ble::ProcessHelp},       {"enable", &Ble::ProcessEnable},
                                              {"disable", &Ble::ProcessDisable}, {"adv", &Ble::ProcessAdvertise},
                                              {"scan", &Ble::ProcessScan},       {"bdaddr", &Ble::ProcessAddr},
                                              {"conn", &Ble::ProcessConnection}, {"test", &Ble::ProcessTest}};

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

otError Ble::ProcessTest(int argc, char *argv[])
{
    AdvData            advData;
    otPlatBleUuid      uuids[2];
    char               devName[] = "TOBLE";
    FlagsAdvData       flagsAdvData;
    DeviceNameAdvData  devNameAdvData;
    Uuid16AdvData      uuid16AdvData;
    ServiceDataAdvData serviceDataAdvData;
    FlagsAdvData       flagsAdvData2;
    DeviceNameAdvData  devNameAdvData2;
    Uuid16AdvData      uuid16AdvData2;
    ServiceDataAdvData serviceDataAdvData2;

    uuids[0].mType          = OT_BLE_UUID_TYPE_16;
    uuids[0].mValue.mUuid16 = 0xA000;
    uuids[1].mType          = OT_BLE_UUID_TYPE_16;
    uuids[1].mValue.mUuid16 = 0xB000;

    flagsAdvData.SetFlags(FlagsAdvData::kLeGeneralDiscoverable | FlagsAdvData::kBrEdrNotSupported);
    devNameAdvData.SetDeviceName(devName);
    uuid16AdvData.AddUuid(uuids[0]);
    uuid16AdvData.AddUuid(uuids[1]);
    serviceDataAdvData.SetUuid16(0xCC00);
    serviceDataAdvData.SetServiceData(reinterpret_cast<const uint8_t *>(devName), strlen(devName));

    advData.AddAdvDataEntry(reinterpret_cast<const AdvDataHeader *>(&flagsAdvData));
    advData.AddAdvDataEntry(reinterpret_cast<const AdvDataHeader *>(&devNameAdvData));
    advData.AddAdvDataEntry(reinterpret_cast<const AdvDataHeader *>(&uuid16AdvData));
    advData.AddAdvDataEntry(reinterpret_cast<const AdvDataHeader *>(&serviceDataAdvData));

    printf("\r\nAdvData: ");
    OutputBytes(advData.GetBuf(), advData.GetLength());
    printf("\r\n");

    if (advData.GetAdvDataEntry(AdvDataHeader::kFlags, &flagsAdvData2) == OT_ERROR_NONE)
    {
        printf("Flags: %04x\r\n", flagsAdvData2.GetFlags());
    }
    else
    {
        printf("Get Flags error\r\n");
    }

    if (advData.GetAdvDataEntry(AdvDataHeader::kCompleteLocalName, &devNameAdvData2) == OT_ERROR_NONE)
    {
        printf("DevName: %s\r\n", devNameAdvData2.GetDeviceName());
    }
    else
    {
        printf("Get DevName error\r\n");
    }

    if (advData.GetAdvDataEntry(AdvDataHeader::kCompleteList16BitService, &uuid16AdvData2) == OT_ERROR_NONE)
    {
        uint8_t       iterator = 0;
        otPlatBleUuid uuid;

        while (uuid16AdvData2.GetNextUuid(iterator, uuid) == OT_ERROR_NONE)
        {
            printf("Uuid16: %04x\r\n", uuid.mValue.mUuid16);
        }
    }
    else
    {
        printf("Get Uuid error\r\n");
    }

    if (advData.GetAdvDataEntry(AdvDataHeader::kServiceData, &serviceDataAdvData2) == OT_ERROR_NONE)
    {
        printf("ServiceData: uuid16 = %04x, data = ", serviceDataAdvData2.GetUuid16());
        OutputBytes(serviceDataAdvData2.GetServiceData(), serviceDataAdvData2.GetServiceDataLength());
        printf("\r\n\r\n");
    }
    else
    {
        printf("Get ServiceData error\r\n");
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
        otPlatBleUuid uuid      = {.mType = OT_BLE_UUID_TYPE_16, {.mUuid16 = 0xA111}};
        char          devName[] = "TOBLE";

        FlagsAdvData       flagsAdvData;
        DeviceNameAdvData  devNameAdvData;
        Uuid16AdvData      uuid16AdvData;
        ServiceDataAdvData serviceDataAdvData;

        flagsAdvData.SetFlags(FlagsAdvData::kLeGeneralDiscoverable | FlagsAdvData::kBrEdrNotSupported);
        devNameAdvData.SetDeviceName(devName);
        uuid16AdvData.AddUuid(uuid);
        serviceDataAdvData.SetUuid16(0xCC00);

        advData.AddAdvDataEntry(reinterpret_cast<const AdvDataHeader *>(&flagsAdvData));
        advData.AddAdvDataEntry(reinterpret_cast<const AdvDataHeader *>(&devNameAdvData));
        advData.AddAdvDataEntry(reinterpret_cast<const AdvDataHeader *>(&uuid16AdvData));
        advData.AddAdvDataEntry(reinterpret_cast<const AdvDataHeader *>(&serviceDataAdvData));

        printf("\r\nAdvData: ");
        OutputBytes(advData.GetBuf(), advData.GetLength());
        printf("\r\n\r\n");

        error = otPlatBleGapAdvDataSet(mInterpreter.mInstance, advData.GetBuf(), advData.GetLength());
        error = otPlatBleGapScanResponseSet(mInterpreter.mInstance, advData.GetBuf(), advData.GetLength());
        error = otPlatBleGapAdvStart(mInterpreter.mInstance, kAdvInterval,
                                     OT_BLE_ADV_MODE_CONNECTABLE | OT_BLE_ADV_MODE_SCANNABLE);
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
        error = otPlatBleGapScanStart(mInterpreter.mInstance, kScanInterval, kScanWindow);
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
    otError             error = OT_ERROR_NONE;
    otPlatBleDeviceAddr addr;

    VerifyOrExit(argc == 0, error = OT_ERROR_INVALID_ARGS);

    if ((error = otPlatBleGapAddressGet(mInterpreter.mInstance, &addr)) == OT_ERROR_NONE)
    {
        mInterpreter.mServer->OutputFormat("%02x%02x%02x%02x%02x%02x\r\n", addr.mAddr[5], addr.mAddr[4], addr.mAddr[3],
                                           addr.mAddr[2], addr.mAddr[1], addr.mAddr[0]);
    }

    OT_UNUSED_VARIABLE(argv);
exit:
    return error;
}

otError Ble::ProcessConnection(int argc, char *argv[])
{
    otError error;
    long    value;

    VerifyOrExit(argc >= 1, error = OT_ERROR_INVALID_ARGS);

    if ((argc == 3) && (strcmp(argv[0], "start") == 0))
    {
        otPlatBleDeviceAddr    devAddr;
        uint8_t                addr[OT_BLE_ADDRESS_LENGTH];
        otPlatBleGapConnParams connParams;

        SuccessOrExit(error = Interpreter::ParseLong(argv[1], value));
        devAddr.mAddrType = static_cast<uint8_t>(value);
        VerifyOrExit(devAddr.mAddrType <= OT_BLE_ADDRESS_TYPE_RANDOM_PRIVATE_NON_RESOLVABLE,
                     error = OT_ERROR_INVALID_ARGS);
        VerifyOrExit(Interpreter::Hex2Bin(argv[2], addr, OT_BLE_ADDRESS_LENGTH) == OT_BLE_ADDRESS_LENGTH,
                     error = OT_ERROR_INVALID_ARGS);

        devAddr.mAddr[0] = addr[5];
        devAddr.mAddr[1] = addr[4];
        devAddr.mAddr[2] = addr[3];
        devAddr.mAddr[3] = addr[2];
        devAddr.mAddr[4] = addr[1];
        devAddr.mAddr[5] = addr[0];

        connParams.mConnMinInterval        = kConnInterval;
        connParams.mConnMaxInterval        = kConnInterval;
        connParams.mConnSlaveLatency       = 0;
        connParams.mConnSupervisionTimeout = kConnSupTimeout;

        SuccessOrExit(error = otPlatBleGapConnParamsSet(mInterpreter.mInstance, &connParams));
        error = otPlatBleGapConnect(mInterpreter.mInstance, &devAddr, kScanInterval, kScanWindow);
    }
    else if (strcmp(argv[0], "stop") == 0)
    {
        error = otPlatBleGapDisconnect(mInterpreter.mInstance);
    }
    else
    {
        error = OT_ERROR_INVALID_ARGS;
    }

exit:
    return error;
}

void Ble::OutputBytes(const uint8_t *aBytes, uint8_t aLength) const
{
    for (int i = 0; i < aLength; i++)
    {
        printf("%02x ", aBytes[i]);
    }
}

void StdOutputBytes(const uint8_t *aBytes, uint8_t aLength)
{
    for (int i = 0; i < aLength; i++)
    {
        printf("%02x ", aBytes[i]);
    }
}

extern "C" void otPlatBleGapOnConnected(otInstance *aInstance, uint16_t aConnectionId)
{
    printf("BleConnected   : connectionId = %d\r\n", aConnectionId);
    OT_UNUSED_VARIABLE(aInstance);
}

extern "C" void otPlatBleGapOnDisconnected(otInstance *aInstance, uint16_t aConnectionId)
{
    printf("BleDisconnected: connectionId = %d\r\n", aConnectionId);
    OT_UNUSED_VARIABLE(aInstance);
}

extern "C" void otPlatBleGapOnAdvReceived(otInstance *         aInstance,
                                          otPlatBleDeviceAddr *aAddress,
                                          otBleRadioPacket *   aPacket)
{
    AdvData           advData;
    DeviceNameAdvData devNameAdvData;

    if (advData.Init(aPacket->mValue, aPacket->mLength) == OT_ERROR_NONE)
    {
        if (advData.GetAdvDataEntry(AdvDataHeader::kCompleteLocalName, &devNameAdvData) == OT_ERROR_NONE)
        {
            printf("AdvReceived    : ");
            printf("peerAddrType = %d, ", aAddress->mAddrType);
            printf("peerAddr = %02x%02x%02x%02x%02x%02x, ", aAddress->mAddr[5], aAddress->mAddr[4], aAddress->mAddr[3],
                   aAddress->mAddr[2], aAddress->mAddr[1], aAddress->mAddr[0]);
            printf("rssi = %d, ", aPacket->mPower);
            printf("DevName = %s\r\n", devNameAdvData.GetDeviceName());

            /*
            if (aPacket->mLength != 0)
            {
                printf("AdvData    : ");
                StdOutputBytes(aPacket->mValue, aPacket->mLength);
                printf("\r\n");
            }
            */
        }
    }

    OT_UNUSED_VARIABLE(aInstance);
}

extern "C" void otPlatBleGapOnScanRespReceived(otInstance *         aInstance,
                                               otPlatBleDeviceAddr *aAddress,
                                               otBleRadioPacket *   aPacket)
{
    if (aPacket->mPower > -40)
    {
        printf("ScanRspReceived: ");
        printf("peerAddrType = %d, ", aAddress->mAddrType);
        printf("peerAddr = %02x%02x%02x%02x%02x%02x, ", aAddress->mAddr[5], aAddress->mAddr[4], aAddress->mAddr[3],
               aAddress->mAddr[2], aAddress->mAddr[1], aAddress->mAddr[0]);
        printf("rssi = %d\r\n", aPacket->mPower);

        // printf("    AdvData: ");
        // StdOutputBytes(aPacket->mValue, aPacket->mLength);
        // printf("\r\n");
    }

    OT_UNUSED_VARIABLE(aInstance);
}
} // namespace Cli
} // namespace ot
