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
 *   This file implements the CLI interpreter.
 */

#include "cli_ble.hpp"

#include <openthread/config.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <openthread/error.h>
#include <openthread/platform/ble.h>

#ifndef OTDLL
#include <openthread/diag.h>
#include <openthread/instance.h>
#endif

#include <common/logging.hpp>

#include "cli/cli.hpp"
#include "cli/cli_server.hpp"

/// Default interval of 100ms.
#define DEFAULT_ADV_INTERVAL 160

/// Default interval of 100ms.
#define DEFAULT_CONN_INTERVAL 160

/// Default scan interval of 2560ms.
#define DEFAULT_SCAN_INTERVAL 0x1000

/// Default scan window of 800ms.
#define DEFAULT_SCAN_WINDOW 0x0500

extern "C" void OutputBytes(const uint8_t *aBytes, uint8_t aLength);

ot::Cli::Interpreter *sInterpreterRef;

namespace ot {
namespace Cli {

const Ble::Command Ble::sCommands[] = {
    {"help", &Ble::ProcessHelp},    {"bdaddr", &Ble::ProcessBdAddr}, {"start", &Ble::ProcessStart},
    {"stop", &Ble::ProcessStop},    {"adv", &Ble::ProcessAdvertise}, {"scan", &Ble::ProcessScan},
    {"conn", &Ble::ProcessConnect}, {"mtu", &Ble::ProcessMtu},       {"gatt", &Ble::ProcessGatt},
    {"ch", &Ble::ProcessChannel},   {"reset", &Ble::ProcessReset},
};

Ble::Ble(Interpreter &aInterpreter)
    : mInterpreter(aInterpreter)
{
    sInterpreterRef = &aInterpreter;
}

otError Ble::ProcessHelp(int argc, char *argv[])
{
    for (unsigned int i = 0; i < sizeof(sCommands) / sizeof(sCommands[0]); i++)
    {
        Server::sServer->OutputFormat("%s\r\n", sCommands[i].mName);
    }

    OT_UNUSED_VARIABLE(argc);
    OT_UNUSED_VARIABLE(argv);

    return OT_ERROR_NONE;
}

otError Ble::ProcessStart(int argc, char *argv[])
{
    OT_UNUSED_VARIABLE(argc);
    OT_UNUSED_VARIABLE(argv);

    return otPlatBleEnable(mInterpreter.mInstance);
}

otError Ble::ProcessStop(int argc, char *argv[])
{
    OT_UNUSED_VARIABLE(argc);
    OT_UNUSED_VARIABLE(argv);

    return otPlatBleDisable(mInterpreter.mInstance);
}

otError Ble::ProcessReset(int argc, char *argv[])
{
    OT_UNUSED_VARIABLE(argc);
    OT_UNUSED_VARIABLE(argv);

    return otPlatBleReset(mInterpreter.mInstance);
}

otError Ble::ProcessAdvertise(int argc, char *argv[])
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(argc > 0, error = OT_ERROR_PARSE);

    if (strcmp(argv[0], "start") == 0)
    {
        long value = DEFAULT_ADV_INTERVAL;

        if (argc > 1)
        {
            SuccessOrExit(error = Interpreter::ParseLong(argv[1], value));
        }

        error = otPlatBleGapAdvStart(mInterpreter.mInstance, value,
                                     OT_BLE_ADV_MODE_CONNECTABLE | OT_BLE_ADV_MODE_SCANNABLE);
    }
    else if (strcmp(argv[0], "stop") == 0)
    {
        error = otPlatBleGapAdvStop(mInterpreter.mInstance);
    }
    else if (strcmp(argv[0], "data") == 0)
    {
        int     advDataLength = 0;
        uint8_t advData[OT_BLE_ADV_DATA_MAX_LENGTH];

        VerifyOrExit(argc > 1, error = OT_ERROR_PARSE);
        VerifyOrExit((advDataLength = Interpreter::Hex2Bin(argv[1], advData, sizeof(advData))) > 0,
                     error = OT_ERROR_PARSE);

        error = otPlatBleGapAdvDataSet(mInterpreter.mInstance, advData, advDataLength);
    }
    else
    {
        error = OT_ERROR_PARSE;
    }

exit:
    return error;
}

otError Ble::ProcessScan(int argc, char *argv[])
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(argc > 0, error = OT_ERROR_PARSE);

    if (strcmp(argv[0], "start") == 0)
    {
        error = otPlatBleGapScanStart(mInterpreter.mInstance, DEFAULT_SCAN_INTERVAL, DEFAULT_SCAN_WINDOW);
    }
    else if (strcmp(argv[0], "stop") == 0)
    {
        error = otPlatBleGapScanStop(mInterpreter.mInstance);
    }
    else if (strcmp(argv[0], "rsp") == 0)
    {
        int     scanResponseLength = 0;
        uint8_t scanResponse[OT_BLE_SCAN_RESPONSE_MAX_LENGTH];

        VerifyOrExit(argc > 1, error = OT_ERROR_PARSE);
        VerifyOrExit((scanResponseLength = Interpreter::Hex2Bin(argv[1], scanResponse, sizeof(scanResponse))) > 0,
                     error = OT_ERROR_PARSE);

        error = otPlatBleGapScanResponseSet(mInterpreter.mInstance, scanResponse, scanResponseLength);
    }
    else
    {
        error = OT_ERROR_PARSE;
    }

exit:
    return error;
}

otError Ble::ProcessConnect(int argc, char *argv[])
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(argc > 0, error = OT_ERROR_PARSE);

    if (strcmp(argv[0], "start") == 0)
    {
        VerifyOrExit(argc > 1, error = OT_ERROR_PARSE);

        otPlatBleDeviceAddr destAddr;
        char *              type = (argc > 2) ? argv[2] : NULL;

        error = Interpreter::ParseBda(argv[1], type, &destAddr);
        VerifyOrExit(error == OT_ERROR_NONE, );

        error = otPlatBleGapConnect(mInterpreter.mInstance, &destAddr, DEFAULT_CONN_INTERVAL, DEFAULT_CONN_INTERVAL);
    }
    else if (strcmp(argv[0], "stop") == 0)
    {
        error = otPlatBleGapDisconnect(mInterpreter.mInstance);
    }
    else if (strcmp(argv[0], "params") == 0)
    {
        otPlatBleGapConnParams connParams;
        error = otPlatBleGapConnParamsSet(mInterpreter.mInstance, &connParams);
    }
    else
    {
        error = OT_ERROR_PARSE;
    }

exit:
    return error;
}

otError Ble::ProcessMtu(int argc, char *argv[])
{
    otError error = OT_ERROR_NONE;
    long    mtu;

    VerifyOrExit(argc > 0, error = OT_ERROR_PARSE);
    SuccessOrExit(error = Interpreter::ParseLong(argv[0], mtu));
    error = otPlatBleGattClientMtuExchangeRequest(mInterpreter.mInstance, mtu);

exit:
    return error;
}

otError Ble::ProcessBdAddr(int argc, char *argv[])
{
    otError             error = OT_ERROR_NONE;
    otPlatBleDeviceAddr bdAddr;
    long                field;

    OT_UNUSED_VARIABLE(argv);

    if (argc == 0)
    {
        error = otPlatBleGapAddressGet(mInterpreter.mInstance, &bdAddr);
        OutputBytes(bdAddr.mAddr, sizeof(bdAddr.mAddr));
        Server::sServer->OutputFormat(" [%d]", bdAddr.mAddrType);
        Server::sServer->OutputFormat("\r\n");
    }
    else
    {
        VerifyOrExit(argc == 2, error = OT_ERROR_PARSE);
        VerifyOrExit(Interpreter::Hex2Bin(argv[0], bdAddr.mAddr, sizeof(bdAddr.mAddr)) == OT_BLE_ADDRESS_LENGTH,
                     error = OT_ERROR_PARSE);
        SuccessOrExit(error = Interpreter::ParseLong(argv[1], field));
        bdAddr.mAddrType = field;

        error = otPlatBleGapAddressSet(mInterpreter.mInstance, &bdAddr);
    }

exit:
    return error;
}

otError Ble::ProcessGatt(int argc, char *argv[])
{
    otError error = OT_ERROR_NONE;
    long    handle;

    VerifyOrExit(argc > 0, error = OT_ERROR_PARSE);

    if (strcmp(argv[0], "write") == 0)
    {
        uint8_t          value[128];
        otBleRadioPacket pkt;
        pkt.mValue = value;
        VerifyOrExit(argc > 2, error = OT_ERROR_PARSE);
        SuccessOrExit(error = Interpreter::ParseLong(argv[1], handle));
        VerifyOrExit((pkt.mLength = Interpreter::Hex2Bin(argv[2], value, sizeof(value))) > 0, error = OT_ERROR_PARSE);
        error = otPlatBleGattClientWrite(mInterpreter.mInstance, handle, &pkt);
    }
    else if (strcmp(argv[0], "read") == 0)
    {
        VerifyOrExit(argc > 1, error = OT_ERROR_PARSE);
        SuccessOrExit(error = Interpreter::ParseLong(argv[1], handle));
        error = otPlatBleGattClientRead(mInterpreter.mInstance, handle);
    }
    else if (strncmp(argv[0], "disc", sizeof("disc") - 1) == 0)
    {
        VerifyOrExit(argc > 1, error = OT_ERROR_PARSE);

        if (strcmp(argv[1], "services") == 0)
        {
            error = otPlatBleGattClientServicesDiscover(mInterpreter.mInstance);
        }
        else if (strncmp(argv[1], "serv", sizeof("serv") - 1) == 0)
        {
            otPlatBleUuid uuid;

            VerifyOrExit(argc > 2, error = OT_ERROR_PARSE);
            SuccessOrExit(error = Interpreter::ParseLong(argv[2], handle));
            // TODO: Handle longer uuid types

            uuid.mType          = OT_BLE_UUID_TYPE_16;
            uuid.mValue.mUuid16 = handle;

            error = otPlatBleGattClientServiceDiscover(mInterpreter.mInstance, &uuid);
        }
        else if (strncmp(argv[1], "char", sizeof("char") - 1) == 0)
        {
            long handle2;

            VerifyOrExit(argc > 3, error = OT_ERROR_PARSE);
            SuccessOrExit(error = Interpreter::ParseLong(argv[2], handle));
            SuccessOrExit(error = Interpreter::ParseLong(argv[3], handle2));

            error = otPlatBleGattClientCharacteristicsDiscover(mInterpreter.mInstance, handle, handle2);
        }
        else if (strncmp(argv[1], "desc", sizeof("desc") - 1) == 0)
        {
            long handle2;

            VerifyOrExit(argc > 3, error = OT_ERROR_PARSE);
            SuccessOrExit(error = Interpreter::ParseLong(argv[2], handle));
            SuccessOrExit(error = Interpreter::ParseLong(argv[3], handle2));

            error = otPlatBleGattClientDescriptorsDiscover(mInterpreter.mInstance, handle, handle2);
        }
        else if (strncmp(argv[1], "sub", sizeof("sub") - 1) == 0)
        {
            VerifyOrExit(argc > 2, error = OT_ERROR_PARSE);
            SuccessOrExit(error = Interpreter::ParseLong(argv[2], handle));

            error = otPlatBleGattClientSubscribeRequest(mInterpreter.mInstance, handle, true);
        }
        else if (strncmp(argv[1], "unsub", sizeof("unsub") - 1) == 0)
        {
            VerifyOrExit(argc > 2, error = OT_ERROR_PARSE);
            SuccessOrExit(error = Interpreter::ParseLong(argv[2], handle));

            error = otPlatBleGattClientSubscribeRequest(mInterpreter.mInstance, handle, false);
        }
        else
        {
            error = OT_ERROR_PARSE;
        }
    }
    else
    {
        error = OT_ERROR_PARSE;
    }

exit:
    return error;
}

otError Ble::ProcessChannel(int argc, char *argv[])
{
    otError error = OT_ERROR_NONE;

    OT_UNUSED_VARIABLE(argc);
    OT_UNUSED_VARIABLE(argv);

    VerifyOrExit(argc > 0, error = OT_ERROR_PARSE);

    if (strcmp(argv[0], "start") == 0)
    {
        long     psm = 0x0080;
        long     mtu = 1280;
        uint16_t cid;
        VerifyOrExit(argc > 1, error = OT_ERROR_PARSE);
        SuccessOrExit(error = Interpreter::ParseLong(argv[1], psm));
        otPlatBleL2capConnectionRequest(mInterpreter.mInstance, psm, mtu, &cid);
    }
    else if (strcmp(argv[0], "tx") == 0)
    {
        uint8_t          data[512];
        size_t           dataLen;
        otBleRadioPacket pkt;

        uint16_t scid = 1, dcid = 1;

        VerifyOrExit(argc > 1, error = OT_ERROR_PARSE);

        dataLen = Interpreter::Hex2Bin(argv[1], data, sizeof(data));

        VerifyOrExit(0 < dataLen && dataLen <= 512, error = OT_ERROR_PARSE);

        pkt.mValue  = data;
        pkt.mLength = dataLen;
        otPlatBleL2capSduSend(mInterpreter.mInstance, scid, dcid, &pkt);
    }
    else if (strcmp(argv[0], "stop") == 0)
    {
        int scid = 1, dcid = 1;
        otPlatBleL2capDisconnect(mInterpreter.mInstance, scid, dcid);
    }
    else
    {
        error = OT_ERROR_PARSE;
    }

exit:
    return error;
}

otError Ble::Process(otInstance *aInstance, int argc, char *argv[])
{
    otError error = OT_ERROR_PARSE;

    (void)aInstance;

    if (argc == 0)
    {
        ExitNow(error = ProcessHelp(argc, argv));
    }

    for (unsigned int i = 0; i < OT_ARRAY_LENGTH(sCommands); i++)
    {
        if (strcmp(argv[0], sCommands[i].mName) == 0)
        {
            error = (this->*sCommands[i].mCommand)(argc - 1, argv + 1);
            break;
        }
    }

exit:
    return error;
}

} // namespace Cli
} // namespace ot

#ifdef __cplusplus
extern "C" {
#endif

void OutputBytes(const uint8_t *aBytes, uint8_t aLength)
{
    for (int i = 0; i < aLength; i++)
    {
        ot::Cli::Server::sServer->OutputFormat("%02x", aBytes[i]);
    }
}

void otPlatBleGapOnAdvReceived(otInstance *aInstance, otPlatBleDeviceAddr *aAddress, otBleRadioPacket *aPacket)
{
    (void)aInstance;

    ot::Cli::Server::sServer->OutputFormat("Got BLE_ADV from ");
    OutputBytes(aAddress->mAddr, sizeof(aAddress->mAddr));
    ot::Cli::Server::sServer->OutputFormat(" [%d] - ", aAddress->mAddrType);
    OutputBytes(aPacket->mValue, aPacket->mLength);
    ot::Cli::Server::sServer->OutputFormat("\r\n");
}

void otPlatBleGapOnScanRespReceived(otInstance *aInstance, otPlatBleDeviceAddr *aAddress, otBleRadioPacket *aPacket)
{
    (void)aInstance;

    ot::Cli::Server::sServer->OutputFormat("Got BLE_SCAN_RSP from ");
    OutputBytes(aAddress->mAddr, sizeof(aAddress->mAddr));
    ot::Cli::Server::sServer->OutputFormat(" [%d] - ", aAddress->mAddrType);
    OutputBytes(aPacket->mValue, aPacket->mLength);
    ot::Cli::Server::sServer->OutputFormat("\r\n");
}

void otPlatBleGapOnConnected(otInstance *aInstance, uint16_t aConnectionId)
{
    (void)aInstance;

    ot::Cli::Server::sServer->OutputFormat("BLE connected: @id=%d\r\n", aConnectionId);
}

void otPlatBleGapOnDisconnected(otInstance *aInstance, uint16_t aConnectionId)
{
    (void)aInstance;

    ot::Cli::Server::sServer->OutputFormat("BLE disconnected: @id=%d\r\n", aConnectionId);
}

void otPlatBleGattClientOnServiceDiscovered(otInstance *aInstance,
                                            uint16_t    aStartHandle,
                                            uint16_t    aEndHandle,
                                            uint16_t    aServiceUuid,
                                            otError     aError)
{
    (void)aInstance;

    SuccessOrExit(aError);

    ot::Cli::Server::sServer->OutputFormat("service: uuid=0x%04x @start=%d @end=%d\r\n", aServiceUuid, aStartHandle,
                                           aEndHandle);

exit:
    sInterpreterRef->AppendResult(aError);
}

void otPlatBleGattClientOnCharacteristicsDiscoverDone(otInstance *                 aInstance,
                                                      otPlatBleGattCharacteristic *aChars,
                                                      uint16_t                     aCount,
                                                      otError                      aError)
{
    (void)aInstance;

    SuccessOrExit(aError);

    for (int i = 0; i < aCount; i++)
    {
        // TODO: Generic helper to display variable length Uuid

        ot::Cli::Server::sServer->OutputFormat("characteristic: @value=%d @cccd=%d "
                                               "properties=0x%02x uuid=0x%04x\r\n",
                                               aChars[i].mHandleValue, aChars[i].mHandleCccd, aChars[i].mProperties,
                                               aChars[i].mUuid.mValue.mUuid16);
    }

exit:
    sInterpreterRef->AppendResult(aError);
}

void otPlatBleGattClientOnDescriptorsDiscoverDone(otInstance *             aInstance,
                                                  otPlatBleGattDescriptor *aDescs,
                                                  uint16_t                 aCount,
                                                  otError                  aError)
{
    (void)aInstance;

    SuccessOrExit(aError);

    for (int i = 0; i < aCount; i++)
    {
        ot::Cli::Server::sServer->OutputFormat("descriptor: @handle=%d uuid=0x%04x\r\n", aDescs[i].mHandle,
                                               aDescs[i].mUuid.mValue.mUuid16);
    }

exit:
    sInterpreterRef->AppendResult(aError);
}

void otPlatBleGattServerOnWriteRequest(otInstance *aInstance, uint16_t aHandle, otBleRadioPacket *aPacket)
{
    (void)aInstance;

    ot::Cli::Server::sServer->OutputFormat("Got BLE_WRT_REQ: @handle=%d data=", aHandle);
    OutputBytes(aPacket->mValue, aPacket->mLength);
}

void otPlatBleGattClientOnReadResponse(otInstance *aInstance, otBleRadioPacket *aPacket)
{
    (void)aInstance;

    OutputBytes(aPacket->mValue, aPacket->mLength);
    ot::Cli::Server::sServer->OutputFormat("\r\n");
}

void otPlatBleL2capOnConnectionRequest(otInstance *aInstance, uint16_t aPsm, uint16_t aMtu, uint16_t aPeerCid)
{
    (void)aInstance;

    ot::Cli::Server::sServer->OutputFormat("L2CAP connect request: @psm=%d mtu=%d dcid=%d", aPsm, aMtu, aPeerCid);
}

void otPlatBleL2capOnConnectionResponse(otInstance *        aInstance,
                                        otPlatBleL2capError aError,
                                        uint16_t            aMtu,
                                        uint16_t            aPeerCid)
{
    (void)aInstance;
    (void)aError;

    ot::Cli::Server::sServer->OutputFormat("L2CAP connect response: @mtu=%d dcid=%d", aMtu, aPeerCid);
}

void otPlatBleL2capOnDisconnect(otInstance *aInstance, uint16_t aLocalCid, uint16_t aPeerCid)
{
    (void)aInstance;

    ot::Cli::Server::sServer->OutputFormat("L2CAP disconnect: @scid=%d dcid=%d", aLocalCid, aPeerCid);
}

void otPlatBleL2capOnSduReceived(otInstance *      aInstance,
                                 uint16_t          aLocalCid,
                                 uint16_t          aPeerCid,
                                 otBleRadioPacket *aPacket)
{
    (void)aInstance;
    (void)aPacket;

    ot::Cli::Server::sServer->OutputFormat("L2CAP sdu rx: @scid=%d dcid=%d data=", aLocalCid, aPeerCid);
    OutputBytes(aPacket->mValue, aPacket->mLength);
    ot::Cli::Server::sServer->OutputFormat("\r\n");
}

void otPlatBleL2capOnSduSent(otInstance *aInstance)
{
    (void)aInstance;

    ot::Cli::Server::sServer->OutputFormat("L2CAP sdu tx done");
}

#ifdef __cplusplus
}
#endif
