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

#include "cli_ble.hpp"
#include "cli_server.hpp"
#include "cli/cli.hpp"
#include "cli/cli_server.hpp"

#include "common/code_utils.hpp"
#include "common/instance.hpp"

#include <string.h>

#if OPENTHREAD_ENABLE_CLI_BLE && !OPENTHREAD_ENABLE_TOBLE

namespace ot {
namespace Cli {

#define BLE_FILTER_ADV_RSSI_THRESHOLD -40
#define MAX_RD_WR_BUFFER_SIZE          20
static uint8_t sRdWrBuffer[MAX_RD_WR_BUFFER_SIZE] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa,
                                                     0xbb, 0xcc, 0xdd, 0xee, 0xff, 0x11, 0x22, 0x33, 0x44, 0x55};
static uint8_t sRdWrBufferLength = MAX_RD_WR_BUFFER_SIZE;

const struct Ble::Command Ble::sCommands[] = {{"help", &Ble::ProcessHelp},       {"bdaddr", &Ble::ProcessBdAddr},
                                              {"enable", &Ble::ProcessEnable},   {"disable", &Ble::ProcessDisable},
                                              {"adv", &Ble::ProcessAdvertise},   {"scan", &Ble::ProcessScan},
                                              {"connect", &Ble::ProcessConnect}, {"disconnect", &Ble::ProcessDisconnect},
                                              {"l2cap", &Ble::ProcessL2cap},     {"gatt", &Ble::ProcessGatt}};

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
        error = OT_ERROR_NONE;
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

    VerifyOrExit(argc >= 1, error = OT_ERROR_INVALID_ARGS);

    if ((argc == 2) && (strcmp(argv[0], "advdata") == 0))
    {
        uint8_t advData[OT_BLE_ADV_DATA_MAX_LENGTH];
        int     advDataLength;

        VerifyOrExit((advDataLength = Interpreter::Hex2Bin(argv[1], advData, sizeof(advData))) > 0,
                     error = OT_ERROR_INVALID_ARGS);

        error = otPlatBleGapAdvDataSet(mInterpreter.mInstance, advData, advDataLength);
    }
    else if ((argc == 1) && (strcmp(argv[0], "start") == 0))
    {
        error = otPlatBleGapAdvStart(mInterpreter.mInstance, kAdvInterval,
                                     OT_BLE_ADV_MODE_CONNECTABLE | OT_BLE_ADV_MODE_SCANNABLE);
    }
    else if ((argc == 1) && (strcmp(argv[0], "stop") == 0))
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

    if ((argc == 2) && (strcmp(argv[0], "rspdata") == 0))
    {
        uint8_t rspData[OT_BLE_ADV_DATA_MAX_LENGTH];
        int     rspDataLength;

        VerifyOrExit((rspDataLength = Interpreter::Hex2Bin(argv[1], rspData, sizeof(rspData))) > 0,
                     error = OT_ERROR_INVALID_ARGS);

        error = otPlatBleGapScanResponseSet(mInterpreter.mInstance, rspData, rspDataLength);
    }
    else if ((argc == 1) && (strcmp(argv[0], "start") == 0))
    {
        error = otPlatBleGapScanStart(mInterpreter.mInstance, kScanInterval, kScanWindow);
        if (error == OT_ERROR_NONE)
        {
            mInterpreter.mServer->OutputFormat("\r\n| advType | addrType |   address    | rssi | AD or Scan Rsp Data |\r\n");
            mInterpreter.mServer->OutputFormat("+---------+----------+--------------+------+---------------------|\r\n");
        }
    }
    else if ((argc == 1) && (strcmp(argv[0], "stop") == 0))
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

otError Ble::ProcessBdAddr(int argc, char *argv[])
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

otError Ble::ProcessConnect(int argc, char *argv[])
{
    otError                error;
    long                   value;
    otPlatBleDeviceAddr    devAddr;
    otPlatBleGapConnParams connParams;

    VerifyOrExit(argc == 2, error = OT_ERROR_INVALID_ARGS);

    SuccessOrExit(error = Interpreter::ParseLong(argv[0], value));
    devAddr.mAddrType = static_cast<uint8_t>(value);
    VerifyOrExit(devAddr.mAddrType <= OT_BLE_ADDRESS_TYPE_RANDOM_PRIVATE_NON_RESOLVABLE,
                 error = OT_ERROR_INVALID_ARGS);
    VerifyOrExit(Interpreter::Hex2Bin(argv[1], devAddr.mAddr, OT_BLE_ADDRESS_LENGTH) == OT_BLE_ADDRESS_LENGTH,
                 error = OT_ERROR_INVALID_ARGS);
    ReverseBuf(devAddr.mAddr, OT_BLE_ADDRESS_LENGTH);

    connParams.mConnMinInterval        = kConnInterval;
    connParams.mConnMaxInterval        = kConnInterval;
    connParams.mConnSlaveLatency       = 0;
    connParams.mConnSupervisionTimeout = kConnSupTimeout;

    SuccessOrExit(error = otPlatBleGapConnParamsSet(mInterpreter.mInstance, &connParams));
    error = otPlatBleGapConnect(mInterpreter.mInstance, &devAddr, kScanInterval, kScanWindow);

exit:
    return error;
}

otError Ble::ProcessDisconnect(int argc, char *argv[])
{
    otError error;

    VerifyOrExit(argc == 0, error = OT_ERROR_INVALID_ARGS);
    error = otPlatBleGapDisconnect(mInterpreter.mInstance);

    OT_UNUSED_VARIABLE(argv);

exit:
    return error;
}

otError Ble::ProcessL2cap(int argc, char *argv[])
{
    otError error;
    long    value;
    uint8_t l2capHandle;

    VerifyOrExit(argc >= 1, error = OT_ERROR_INVALID_ARGS);

    if ((argc == 4) && (strcmp(argv[0], "register") == 0))
    {
        uint16_t           connId;
        uint16_t           psm = 0x0080;
        uint16_t           mtu;
        otPlatBleL2capRole role;

        SuccessOrExit(error = Interpreter::ParseLong(argv[1], value));
        connId = static_cast<uint16_t>(value);

        SuccessOrExit(error = Interpreter::ParseLong(argv[2], value));
        role = static_cast<otPlatBleL2capRole>(value);

        SuccessOrExit(error = Interpreter::ParseLong(argv[3], value));
        mtu = static_cast<otPlatBleL2capRole>(value);

        error = otPlatBleL2capConnectionRegister(mInterpreter.mInstance, connId, psm, mtu, role, &l2capHandle);
        if (error == OT_ERROR_NONE)
        {
            mInterpreter.mServer->OutputFormat("L2cap Handle: %d\r\n", l2capHandle);
        }
    }
    else if ((argc == 2) && (strcmp(argv[0], "deregister") == 0))
    {
        SuccessOrExit(error = Interpreter::ParseLong(argv[1], value));
        l2capHandle = static_cast<uint8_t>(value);

        error = otPlatBleL2capConnectionDeregister(mInterpreter.mInstance, l2capHandle);
    }
    else if ((argc == 2) && (strcmp(argv[0], "connect") == 0))
    {
        SuccessOrExit(error = Interpreter::ParseLong(argv[1], value));
        l2capHandle = static_cast<uint8_t>(value);

        error = otPlatBleL2capConnectionRequest(mInterpreter.mInstance, l2capHandle);
    }
    else if ((argc == 2) && (strcmp(argv[0], "disconnect") == 0))
    {
        SuccessOrExit(error = Interpreter::ParseLong(argv[1], value));
        l2capHandle = static_cast<uint8_t>(value);

        error = otPlatBleL2capDisconnect(mInterpreter.mInstance, l2capHandle);
    }
    else if ((argc == 3) && (strcmp(argv[0], "send") == 0))
    {
        otBleRadioPacket packet;

        SuccessOrExit(error = Interpreter::ParseLong(argv[1], value));
        l2capHandle    = static_cast<uint8_t>(value);
        packet.mValue  = reinterpret_cast<uint8_t *>(argv[2]);
        packet.mLength = static_cast<uint16_t>(strlen(argv[2]));

        error = otPlatBleL2capSduSend(mInterpreter.mInstance, l2capHandle, &packet);
    }
    else
    {
        error = OT_ERROR_INVALID_ARGS;
    }

exit:
    return error;
}

otError Ble::ProcessGatt(int argc, char *argv[])
{
    otError error;
    long    value;

    VerifyOrExit(argc >= 1, error = OT_ERROR_INVALID_ARGS);

    if (strcmp(argv[0], "server") == 0)
    {
        if ((argc == 2) && (strcmp(argv[1], "register") == 0))
        {
            const uint8_t                kGattCharsSize = 3;
            char                         deviceName[]   = "ThreadBLE";
            otPlatBleGattCharacteristic *characteristic;
            static uint8_t sRxBufUuid[] = {0x11, 0x9D, 0x9F, 0x42, 0x9C, 0x4F, 0x9F, 0x95,
                                           0x59, 0x45, 0x3D, 0x26, 0xF5, 0x2E, 0xEE, 0x18};
            static uint8_t sTxBufUuid[] = {0x12, 0x9D, 0x9F, 0x42, 0x9C, 0x4F, 0x9F, 0x95,
                                           0x59, 0x45, 0x3D, 0x26, 0xF5, 0x2E, 0xEE, 0x18};

            // clang-format off
            static otPlatBleGattCharacteristic sCharacteristics[kGattCharsSize] =
            {
                {
                    .mUuid               =
                    {
                        .mType           = OT_BLE_UUID_TYPE_128,
                        .mValue          =
                        {
                            .mUuid128    = sRxBufUuid,
                        },
                    },
                    .mHandleValue        = 0,
                    .mHandleCccd         = 0,
                    .mProperties         = OT_BLE_CHAR_PROP_WRITE,
                    .mMaxAttrLength      = 128,
                },
                {
                    .mUuid               =
                    {
                        .mType           = OT_BLE_UUID_TYPE_128,
                        .mValue          =
                        {
                            .mUuid128    = sTxBufUuid,
                        },
                    },
                    .mHandleValue        = 0,
                    .mHandleCccd         = 0,
                    .mProperties         = OT_BLE_CHAR_PROP_READ | OT_BLE_CHAR_PROP_INDICATE,
                    .mMaxAttrLength      = 128,
                },
                {
                    .mUuid               =
                    {
                        .mType           = OT_BLE_UUID_TYPE_NONE,
                        .mValue          =
                        {
                            .mUuid128    = NULL,
                        },
                    },
                    .mHandleValue        = 0,
                    .mHandleCccd         = 0,
                    .mProperties         = 0,
                    .mMaxAttrLength      = 0,
                }
            };

            // 6LoWPAN GATT Profile
            static otPlatBleGattService sService =
            {
                .mUuid            =
                {
                    .mType        = OT_BLE_UUID_TYPE_16,
                    .mValue       =
                    {
                        .mUuid16  = 0xFFFB, // BTP service UUID
                    }
                },
                .mHandle          = 0,
                .mCharacteristics = sCharacteristics,
            };
            // clang-format on

            SuccessOrExit(error = otPlatBleGapServiceSet(mInterpreter.mInstance, deviceName, 0));
            SuccessOrExit(error = otPlatBleGattServerServicesRegister(mInterpreter.mInstance, &sService));

            characteristic = sService.mCharacteristics;

            mInterpreter.mServer->OutputFormat("service       : handle = %2d, uuid = ", sService.mHandle);
            Ble::PrintUuid(&sService.mUuid);
            mInterpreter.mServer->OutputFormat("\r\n");

            while (characteristic->mUuid.mType != OT_BLE_UUID_TYPE_NONE)
            {
                mInterpreter.mServer->OutputFormat("characteristic: handle = %2d, properties = 0x%02x, ",
                                                   characteristic->mHandleValue, characteristic->mProperties);
                mInterpreter.mServer->OutputFormat("handleCccd = %2d, ", characteristic->mHandleCccd);
                mInterpreter.mServer->OutputFormat("uuid = ");
                Ble::PrintUuid(&characteristic->mUuid);
                mInterpreter.mServer->OutputFormat("\r\n");

                characteristic++;
            }
        }
        else if ((argc == 4) && (strcmp(argv[1], "ind") == 0))
        {
            uint8_t          data[kAttMtu - 3];
            uint16_t         handle;
            int              len;
            otBleRadioPacket packet;

            SuccessOrExit(error = Interpreter::ParseLong(argv[2], value));
            handle = static_cast<uint16_t>(value);

            VerifyOrExit((len = Interpreter::Hex2Bin(argv[3], data, sizeof(data))) > 0, error = OT_ERROR_INVALID_ARGS);
            packet.mLength = static_cast<uint16_t>(len);
            packet.mValue  = data;

            error = otPlatBleGattServerIndicate(mInterpreter.mInstance, handle, &packet);
        }
        else
        {
            error = OT_ERROR_INVALID_ARGS;
        }
    }
    else if ((argc >= 2) && (strcmp(argv[0], "client") == 0))
    {
        if (strcmp(argv[1], "mtu") == 0)
        {
            if ((argc == 4) && (strcmp(argv[2], "exchange") == 0))
            {
                SuccessOrExit(error = Interpreter::ParseLong(argv[3], value));
                error = otPlatBleGattClientMtuExchangeRequest(mInterpreter.mInstance, static_cast<uint16_t>(value));
            }
            else if (argc == 2)
            {
                uint16_t mtu;

                SuccessOrExit(error = otPlatBleGattMtuGet(mInterpreter.mInstance, &mtu));
                mInterpreter.mServer->OutputFormat("mtu: %d\r\n", mtu);
            }
            else
            {
                error = OT_ERROR_INVALID_ARGS;
            }
        }
        else if ((argc >= 4) && (strcmp(argv[1], "find") == 0))
        {
            if (strcmp(argv[2], "service") == 0)
            {
                if (strcmp(argv[3], "all") == 0)
                {
                    SuccessOrExit(error = otPlatBleGattClientServicesDiscover(mInterpreter.mInstance));

                    mInterpreter.mServer->OutputFormat("\r\n| startHandle |   endHandle  | uuid |\r\n");
                    mInterpreter.mServer->OutputFormat("+-------------+--------------+------+\r\n");
                }
                else
                {
                    uint8_t       data[OT_BLE_UUID_LENGTH];
                    uint8_t       length;
                    otPlatBleUuid uuid;

                    length = Interpreter::Hex2Bin(argv[3], data, OT_BLE_UUID_LENGTH);

                    VerifyOrExit((length == OT_BLE_UUID16_LENGTH) || (length == OT_BLE_UUID_LENGTH),
                                 error = OT_ERROR_INVALID_ARGS);

                    if (length == OT_BLE_UUID16_LENGTH)
                    {
                        uuid.mType          = OT_BLE_UUID_TYPE_16;
                        uuid.mValue.mUuid16 = (static_cast<uint16_t>(data[0]) << 8) + static_cast<uint16_t>(data[1]);
                    }
                    else if (length == OT_BLE_UUID_LENGTH)
                    {
                        ReverseBuf(data, OT_BLE_UUID_LENGTH);
                        uuid.mType           = OT_BLE_UUID_TYPE_128;
                        uuid.mValue.mUuid128 = data;
                    }
                    else
                    {
                        ExitNow(error = OT_ERROR_INVALID_ARGS);
                    }

                    SuccessOrExit(error = otPlatBleGattClientServiceDiscover(mInterpreter.mInstance, &uuid));

                    mInterpreter.mServer->OutputFormat("\r\n| startHandle |   endHandle  | uuid |\r\n");
                    mInterpreter.mServer->OutputFormat("+-------------+--------------+------+\r\n");
                }
            }
            else if ((argc == 5) && (strcmp(argv[2], "chars") == 0))
            {
                uint16_t startHandle;
                uint16_t endHandle;

                SuccessOrExit(error = Interpreter::ParseLong(argv[3], value));
                startHandle = static_cast<uint16_t>(value);

                SuccessOrExit(error = Interpreter::ParseLong(argv[4], value));
                endHandle = static_cast<uint16_t>(value);

                SuccessOrExit(
                    error = otPlatBleGattClientCharacteristicsDiscover(mInterpreter.mInstance, startHandle, endHandle));

                mInterpreter.mServer->OutputFormat(
                    "\r\n| handle |  properties |               uuid               |\r\n");
                mInterpreter.mServer->OutputFormat("+--------+-------------+----------------------------------+\r\n");
            }
            else if ((argc == 5) && (strcmp(argv[2], "desc") == 0))
            {
                uint16_t startHandle;
                uint16_t endHandle;

                SuccessOrExit(error = Interpreter::ParseLong(argv[3], value));
                startHandle = static_cast<uint16_t>(value);

                SuccessOrExit(error = Interpreter::ParseLong(argv[4], value));
                endHandle = static_cast<uint16_t>(value);

                SuccessOrExit(
                    error = otPlatBleGattClientDescriptorsDiscover(mInterpreter.mInstance, startHandle, endHandle));

                mInterpreter.mServer->OutputFormat("\r\n| handle |               uuid               |\r\n");
                mInterpreter.mServer->OutputFormat("+--------+----------------------------------+\r\n");
            }
            else
            {
                error = OT_ERROR_INVALID_ARGS;
            }
        }
        else if ((argc == 4) && (strcmp(argv[1], "subs") == 0))
        {
            uint16_t handle;
            bool     subscribe;

            SuccessOrExit(error = Interpreter::ParseLong(argv[2], value));
            handle = static_cast<uint16_t>(value);

            SuccessOrExit(error = Interpreter::ParseLong(argv[3], value));
            subscribe = (value == 0) ? false : true;

            error = otPlatBleGattClientSubscribeRequest(mInterpreter.mInstance, handle, subscribe);
        }
        else if ((argc == 3) && (strcmp(argv[1], "read") == 0))
        {
            SuccessOrExit(error = Interpreter::ParseLong(argv[2], value));
            error = otPlatBleGattClientRead(mInterpreter.mInstance, static_cast<uint16_t>(value));
        }
        else if ((argc == 4) && (strcmp(argv[1], "write") == 0))
        {
            uint8_t          data[kAttMtu - 3];
            int              len;
            otBleRadioPacket packet;

            SuccessOrExit(error = Interpreter::ParseLong(argv[2], value));
            VerifyOrExit((len = Interpreter::Hex2Bin(argv[3], data, sizeof(data))) > 0, error = OT_ERROR_INVALID_ARGS);
            packet.mLength = static_cast<uint16_t>(len);
            packet.mValue  = data;

            error = otPlatBleGattClientWrite(mInterpreter.mInstance, static_cast<uint16_t>(value), &packet);
        }
        else
        {
            error = OT_ERROR_INVALID_ARGS;
        }
    }
    else
    {
        error = OT_ERROR_INVALID_ARGS;
    }

exit:
    return error;
}

void Ble::PrintUuid(const otPlatBleUuid *aUuid)
{
    int8_t i;

    if (aUuid->mType == OT_BLE_UUID_TYPE_16)
    {
        Server::sServer->OutputFormat("%04x", aUuid->mValue.mUuid16);
    }
    else if (aUuid->mType == OT_BLE_UUID_TYPE_128)
    {
        for (i = OT_BLE_UUID_LENGTH - 1; i >= 0; i--)
        {
            Server::sServer->OutputFormat("%02x", aUuid->mValue.mUuid128[i]);
        }
    }
}

void Ble::PrintBytes(const uint8_t *aBuffer, uint8_t aLength)
{
    for (uint8_t i = 0; i < aLength; i++)
    {
        Server::sServer->OutputFormat("%02x", aBuffer[i]);
    }
}

void Ble::ReverseBuf(uint8_t *aBuffer, uint8_t aLength)
{
    uint8_t temp;

    for (uint8_t i = 0; i < aLength / 2; i++)
    {
        temp                     = aBuffer[i];
        aBuffer[i]               = aBuffer[aLength - 1 - i];
        aBuffer[aLength - 1 - i] = temp;
    }
}

extern "C" void otPlatBleOnEnabled(otInstance *aInstance)
{
    Server::sServer->OutputFormat("BLE is enabled\r\n");
    OT_UNUSED_VARIABLE(aInstance);
}

extern "C" void otPlatBleGapOnConnected(otInstance *aInstance, uint16_t aConnectionId)
{
    Server::sServer->OutputFormat("GapOnConnected: connectionId = %d\r\n", aConnectionId);
    OT_UNUSED_VARIABLE(aInstance);
}

extern "C" void otPlatBleGapOnDisconnected(otInstance *aInstance, uint16_t aConnectionId)
{
    Server::sServer->OutputFormat("GapOnDisconnected: connectionId = %d\r\n", aConnectionId);
    OT_UNUSED_VARIABLE(aInstance);
}

extern "C" void otPlatBleGapOnAdvReceived(otInstance *         aInstance,
                                          otPlatBleDeviceAddr *aAddress,
                                          otBleRadioPacket *   aPacket)
{
    // avoid displaying too many packets
    if (aPacket->mPower > BLE_FILTER_ADV_RSSI_THRESHOLD)
    {
        Server::sServer->OutputFormat("| %-8s|    %d     | %02x%02x%02x%02x%02x%02x | %3d  | ", "ADV",
                                      aAddress->mAddrType, aAddress->mAddr[5], aAddress->mAddr[4], aAddress->mAddr[3],
                                      aAddress->mAddr[2], aAddress->mAddr[1], aAddress->mAddr[0], aPacket->mPower);
        Ble::PrintBytes(aPacket->mValue, aPacket->mLength);
        Server::sServer->OutputFormat("\r\n");
    }

    OT_UNUSED_VARIABLE(aInstance);
}

extern "C" void otPlatBleGapOnScanRespReceived(otInstance *         aInstance,
                                               otPlatBleDeviceAddr *aAddress,
                                               otBleRadioPacket *   aPacket)
{
    // avoid displaying too many packets
    if (aPacket->mPower > BLE_FILTER_ADV_RSSI_THRESHOLD)
    {
        Server::sServer->OutputFormat("| %-8s|    %d     | %02x%02x%02x%02x%02x%02x | %3d  | ", "SCAN_RSP",
                                      aAddress->mAddrType, aAddress->mAddr[5], aAddress->mAddr[4], aAddress->mAddr[3],
                                      aAddress->mAddr[2], aAddress->mAddr[1], aAddress->mAddr[0], aPacket->mPower);
        Ble::PrintBytes(aPacket->mValue, aPacket->mLength);
        Server::sServer->OutputFormat("\r\n");
    }

    OT_UNUSED_VARIABLE(aInstance);
}

extern "C" void otPlatBleL2capOnConnectionRequest(otInstance *aInstance, uint8_t aL2capHandle, uint16_t aMtu)
{
    Server::sServer->OutputFormat("L2capOnConnectionRequestReceived: aL2capHandle = %d, aMtu = %d\r\n", aL2capHandle,
                                  aMtu);
    OT_UNUSED_VARIABLE(aInstance);
}

extern "C" void otPlatBleL2capOnConnectionResponse(otInstance *aInstance, uint8_t aL2capHandle, uint16_t aMtu)
{
    Server::sServer->OutputFormat("L2capOnConnectionResponseReceived: aL2capHandle = %d, aMtu = %d\r\n", aL2capHandle,
                                  aMtu);
    OT_UNUSED_VARIABLE(aInstance);
}

extern "C" void otPlatBleL2capOnSduSent(otInstance *aInstance, uint8_t aL2capHandle, otError aError)
{
    Server::sServer->OutputFormat("L2capOnSduSent: aL2capHandle = %d, error = %d\r\n", aL2capHandle, aError);
    OT_UNUSED_VARIABLE(aInstance);
}

extern "C" void otPlatBleL2capOnSduReceived(otInstance *aInstance, uint8_t aL2capHandle, otBleRadioPacket *aPacket)
{
    Server::sServer->OutputFormat("L2capOnSduReceived: aL2capHandle = %d, length = %d\r\n", aL2capHandle,
                                  aPacket->mLength);
    OT_UNUSED_VARIABLE(aInstance);
}

extern "C" void otPlatBleL2capOnDisconnect(otInstance *aInstance, uint8_t aL2capHandle)
{
    Server::sServer->OutputFormat("L2capOnDisconnected: aL2capHandle = %d\r\n", aL2capHandle);
    OT_UNUSED_VARIABLE(aInstance);
}

extern "C" void otPlatBleGattClientOnMtuExchangeResponse(otInstance *aInstance, uint16_t aMtu, otError aError)
{
    if (aError == OT_ERROR_NONE)
    {
        Server::sServer->OutputFormat("MTU : %d\r\n", aMtu);
    }
    else
    {
        Server::sServer->OutputFormat("Error %d: %s\r\n", aError, otThreadErrorToString(aError));
    }

    OT_UNUSED_VARIABLE(aInstance);
}

extern "C" void otPlatBleGattClientOnServiceDiscovered(otInstance *aInstance,
                                                       uint16_t    aStartHandle,
                                                       uint16_t    aEndHandle,
                                                       uint16_t    aServiceUuid,
                                                       otError     aError)
{
    if (aError == OT_ERROR_NONE)
    {
        Server::sServer->OutputFormat("|  %6d     | %6d       | %04x |\r\n", aStartHandle, aEndHandle, aServiceUuid);
    }
    else
    {
        Server::sServer->OutputFormat("Error %d: %s\r\n", aError, otThreadErrorToString(aError));
    }

    OT_UNUSED_VARIABLE(aInstance);
}

extern "C" void otPlatBleGattClientOnCharacteristicsDiscoverDone(otInstance *                 aInstance,
                                                                 otPlatBleGattCharacteristic *aChars,
                                                                 uint16_t                     aCount,
                                                                 otError                      aError)
{
    if (aError == OT_ERROR_NONE)
    {
        for (uint16_t i = 0; i < aCount; i++)
        {
            Server::sServer->OutputFormat("| %6d |    0x%02x     | ", aChars[i].mHandleValue, aChars[i].mProperties);
            Ble::PrintUuid(&aChars[i].mUuid);
            Server::sServer->OutputFormat(" |\r\n");
        }
    }
    else
    {
        Server::sServer->OutputFormat("Error %d: %s\r\n", aError, otThreadErrorToString(aError));
    }

    OT_UNUSED_VARIABLE(aInstance);
}

extern "C" void otPlatBleGattClientOnDescriptorsDiscoverDone(otInstance *             aInstance,
                                                             otPlatBleGattDescriptor *aDescs,
                                                             uint16_t                 aCount,
                                                             otError                  aError)
{
    if (aError == OT_ERROR_NONE)
    {
        for (uint16_t i = 0; i < aCount; i++)
        {
            Server::sServer->OutputFormat("| %6d | ", aDescs[i].mHandle);
            Ble::PrintUuid(&aDescs[i].mUuid);
            Server::sServer->OutputFormat(" |\r\n");
        }
    }
    else
    {
        Server::sServer->OutputFormat("Error %d: %s\r\n", aError, otThreadErrorToString(aError));
    }

    OT_UNUSED_VARIABLE(aInstance);
}

extern "C" void otPlatBleGattClientOnReadResponse(otInstance *aInstance, otBleRadioPacket *aPacket)
{
    if (aPacket != NULL)
    {
        Server::sServer->OutputFormat("GattClientOnReadResponse: ");
        for (uint16_t i = 0; i < aPacket->mLength; i++)
        {
            Server::sServer->OutputFormat("%02x", aPacket->mValue[i]);
        }

        Server::sServer->OutputFormat("\r\n");
    }
    else
    {
        Server::sServer->OutputFormat("GattClientOnReadResponse: Failed\r\n");
    }

    OT_UNUSED_VARIABLE(aInstance);
}

extern "C" void otPlatBleGattClientOnWriteResponse(otInstance *aInstance, uint16_t aHandle)
{
    Server::sServer->OutputFormat("GattClientOnWriteResponse: handle = %d\r\n", aHandle);
    OT_UNUSED_VARIABLE(aInstance);
}

extern "C" void otPlatBleGattClientOnSubscribeResponse(otInstance *aInstance, uint16_t aHandle)
{
    Server::sServer->OutputFormat("GattClientOnSubscribeResponse: handle = %d\r\n", aHandle);
    OT_UNUSED_VARIABLE(aInstance);
}

extern "C" void otPlatBleGattClientOnIndication(otInstance *aInstance, uint16_t aHandle, otBleRadioPacket *aPacket)
{
    Server::sServer->OutputFormat("GattClientOnIndication: handle = %d, value = ", aHandle);

    for (uint8_t i = 0; i < aPacket->mLength; i++)
    {
        Server::sServer->OutputFormat("%02x", aPacket->mValue[i]);
    }

    Server::sServer->OutputFormat("\r\n");

    OT_UNUSED_VARIABLE(aInstance);
}

extern "C" void otPlatBleGattServerOnWriteRequest(otInstance *aInstance, uint16_t aHandle, otBleRadioPacket *aPacket)
{
    Server::sServer->OutputFormat("GattServerOnWriteRequest: handle = %d, value = ", aHandle);
    Ble::PrintBytes(aPacket->mValue, aPacket->mLength);
    Server::sServer->OutputFormat("\r\n");

    sRdWrBufferLength = (aPacket->mLength < sizeof(sRdWrBuffer)) ? aPacket->mLength : sizeof(sRdWrBuffer);
    memcpy(sRdWrBuffer, aPacket->mValue, sRdWrBufferLength);

    OT_UNUSED_VARIABLE(aInstance);
}

extern "C" void otPlatBleGattServerOnReadRequest(otInstance *aInstance, uint16_t aHandle, otBleRadioPacket *aPacket)
{
    Server::sServer->OutputFormat("GattServerOnReadRequest: handle = %d\r\n", aHandle);
    aPacket->mValue  = sRdWrBuffer;
    aPacket->mLength = sRdWrBufferLength;

    OT_UNUSED_VARIABLE(aInstance);
}

extern "C" void otPlatBleGattServerOnSubscribeRequest(otInstance *aInstance, uint16_t aHandle, bool aSubscribing)
{
    Server::sServer->OutputFormat("GattServerOnSubscribeRequest: handle = %d, subscribing = %d\r\n", aHandle,
                                  aSubscribing);
    OT_UNUSED_VARIABLE(aInstance);
}

extern "C" void otPlatBleGattServerOnIndicationConfirmation(otInstance *aInstance, uint16_t aHandle)
{
    Server::sServer->OutputFormat("GattServerOnIndicationConfirmation: handle = %d\r\n", aHandle);
    OT_UNUSED_VARIABLE(aInstance);
}

} // namespace Cli
} // namespace ot

#endif // OPENTHREAD_ENABLE_COMMISSIONER && OPENTHREAD_FTD
