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
 *   This file implements the CLI interpreter.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <openthread.h>
#include <openthread-config.h>

#include "cli.hpp"
#include "cli_dataset.hpp"

namespace Thread {
namespace Cli {

const DatasetCommand Dataset::sCommands[] =
{
    { "help", &ProcessHelp },
    { "active", &ProcessActive },
    { "activetimestamp", &ProcessActiveTimestamp },
    { "channel", &ProcessChannel },
    { "clear", &ProcessClear },
    { "commit", &ProcessCommit },
    { "delay", &ProcessDelay },
    { "extpanid", &ProcessExtPanId },
    { "masterkey", &ProcessMasterKey },
    { "meshlocalprefix", &ProcessMeshLocalPrefix },
    { "mgmtcommand", &ProcessMgmtCommand },
    { "networkname", &ProcessNetworkName },
    { "panid", &ProcessPanId },
    { "pending", &ProcessPending },
    { "pendingtimestamp", &ProcessPendingTimestamp },
};

Server *Dataset::sServer;
otOperationalDataset Dataset::sDataset;

void Dataset::OutputBytes(const uint8_t *aBytes, uint8_t aLength)
{
    for (int i = 0; i < aLength; i++)
    {
        sServer->OutputFormat("%02x", aBytes[i]);
    }
}

ThreadError Dataset::Print(otOperationalDataset &aDataset)
{
    if (aDataset.mIsPendingTimestampSet)
    {
        sServer->OutputFormat("Pending Timestamp: %d\r\n", aDataset.mPendingTimestamp);
    }

    if (aDataset.mIsActiveTimestampSet)
    {
        sServer->OutputFormat("Active Timestamp: %d\r\n", aDataset.mActiveTimestamp);
    }

    if (aDataset.mIsChannelSet)
    {
        sServer->OutputFormat("Channel: %d\r\n", aDataset.mChannel);
    }

    if (aDataset.mIsDelaySet)
    {
        sServer->OutputFormat("Delay: %d\r\n", aDataset.mDelay);
    }

    if (aDataset.mIsExtendedPanIdSet)
    {
        sServer->OutputFormat("Ext PAN ID: ");
        OutputBytes(aDataset.mExtendedPanId.m8, sizeof(aDataset.mExtendedPanId));
        sServer->OutputFormat("\r\n");
    }

    if (aDataset.mIsMeshLocalPrefixSet)
    {
        const uint8_t *prefix = aDataset.mMeshLocalPrefix.m8;
        sServer->OutputFormat("Mesh Local Prefix: %x:%x:%x:%x/64\r\n",
                              (static_cast<uint16_t>(prefix[0]) << 16) | prefix[1],
                              (static_cast<uint16_t>(prefix[2]) << 16) | prefix[3],
                              (static_cast<uint16_t>(prefix[4]) << 16) | prefix[5],
                              (static_cast<uint16_t>(prefix[6]) << 16) | prefix[7]);
    }

    if (aDataset.mIsMasterKeySet)
    {
        sServer->OutputFormat("Master Key: ");
        OutputBytes(aDataset.mMasterKey.m8, sizeof(aDataset.mMasterKey));
        sServer->OutputFormat("\r\n");
    }

    if (aDataset.mIsNetworkNameSet)
    {
        sServer->OutputFormat("Network Name: ");
        sServer->OutputFormat("%.*s\r\n", sizeof(aDataset.mNetworkName), aDataset.mNetworkName.m8);
    }

    if (aDataset.mIsPanIdSet)
    {
        sServer->OutputFormat("PAN ID: 0x%04x\r\n", aDataset.mPanId);
    }

    return kThreadError_None;
}

ThreadError Dataset::Process(int argc, char *argv[], Server &aServer)
{
    ThreadError error = kThreadError_None;

    sServer = &aServer;

    if (argc == 0)
    {
        ExitNow(error = Print(sDataset));
    }

    for (unsigned int i = 0; i < sizeof(sCommands) / sizeof(sCommands[0]); i++)
    {
        if (strcmp(argv[0], sCommands[i].mName) == 0)
        {
            error = sCommands[i].mCommand(argc - 1, argv + 1);
            break;
        }
    }

exit:
    return error;
}

ThreadError Dataset::ProcessHelp(int argc, char *argv[])
{
    for (unsigned int i = 0; i < sizeof(sCommands) / sizeof(sCommands[0]); i++)
    {
        sServer->OutputFormat("%s\r\n", sCommands[i].mName);
    }

    (void)argc;
    (void)argv;
    return kThreadError_None;
}

ThreadError Dataset::ProcessActive(int argc, char *argv[])
{
    otOperationalDataset dataset;
    otGetActiveDataset(&dataset);

    (void)argc;
    (void)argv;
    return Print(dataset);
}

ThreadError Dataset::ProcessActiveTimestamp(int argc, char *argv[])
{
    ThreadError error = kThreadError_None;
    long value;

    VerifyOrExit(argc > 0, ;);

    SuccessOrExit(error = Interpreter::ParseLong(argv[0], value));
    sDataset.mActiveTimestamp = static_cast<uint64_t>(value);
    sDataset.mIsActiveTimestampSet = true;

exit:
    return error;
}

ThreadError Dataset::ProcessChannel(int argc, char *argv[])
{
    ThreadError error = kThreadError_None;
    long value;

    VerifyOrExit(argc > 0, error = kThreadError_Parse);
    SuccessOrExit(error = Interpreter::ParseLong(argv[0], value));
    sDataset.mChannel = static_cast<uint16_t>(value);
    sDataset.mIsChannelSet = true;

exit:
    return error;
}

ThreadError Dataset::ProcessClear(int argc, char *argv[])
{
    memset(&sDataset, 0, sizeof(sDataset));
    (void)argc;
    (void)argv;
    return kThreadError_None;
}

ThreadError Dataset::ProcessCommit(int argc, char *argv[])
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(argc > 0, ;);

    if (strcmp(argv[0], "active") == 0)
    {
        SuccessOrExit(error = otSetActiveDataset(&sDataset));
    }
    else if (strcmp(argv[0], "pending") == 0)
    {
        SuccessOrExit(error = otSetPendingDataset(&sDataset));
    }
    else
    {
        ExitNow(error = kThreadError_Parse);
    }

exit:
    return error;
}

ThreadError Dataset::ProcessDelay(int argc, char *argv[])
{
    ThreadError error = kThreadError_None;
    long value;

    VerifyOrExit(argc > 0, error = kThreadError_Parse);
    SuccessOrExit(error = Interpreter::ParseLong(argv[0], value));
    sDataset.mDelay = static_cast<uint32_t>(value);
    sDataset.mIsDelaySet = true;

exit:
    return error;
}

ThreadError Dataset::ProcessExtPanId(int argc, char *argv[])
{
    ThreadError error = kThreadError_None;
    uint8_t extPanId[OT_EXT_PAN_ID_SIZE];

    VerifyOrExit(argc > 0, error = kThreadError_Parse);
    VerifyOrExit(Interpreter::Hex2Bin(argv[0], extPanId, sizeof(extPanId)) >= 0, error = kThreadError_Parse);

    memcpy(sDataset.mExtendedPanId.m8, extPanId, sizeof(sDataset.mExtendedPanId));
    sDataset.mIsExtendedPanIdSet = true;

exit:
    return error;
}

ThreadError Dataset::ProcessMasterKey(int argc, char *argv[])
{
    ThreadError error = kThreadError_None;
    int keyLength;
    uint8_t key[OT_MASTER_KEY_SIZE];

    VerifyOrExit(argc > 0, error = kThreadError_Parse);
    VerifyOrExit((keyLength = Interpreter::Hex2Bin(argv[0], key, sizeof(key))) == OT_MASTER_KEY_SIZE,
                 error = kThreadError_Parse);

    memcpy(sDataset.mMasterKey.m8, key, sizeof(sDataset.mMasterKey));
    sDataset.mIsMasterKeySet = true;

exit:
    return error;
}

ThreadError Dataset::ProcessMeshLocalPrefix(int argc, char *argv[])
{
    ThreadError error = kThreadError_None;
    otIp6Address prefix;

    VerifyOrExit(argc > 0, error = kThreadError_Parse);
    SuccessOrExit(error = otIp6AddressFromString(argv[0], &prefix));

    memcpy(sDataset.mMeshLocalPrefix.m8, prefix.mFields.m8, sizeof(sDataset.mMeshLocalPrefix.m8));
    sDataset.mIsMeshLocalPrefixSet = true;

exit:
    return error;
}

ThreadError Dataset::ProcessNetworkName(int argc, char *argv[])
{
    ThreadError error = kThreadError_None;
    size_t length;

    VerifyOrExit(argc > 0, error = kThreadError_Parse);
    VerifyOrExit((length = strlen(argv[0])) <= OT_NETWORK_NAME_MAX_SIZE, error = kThreadError_Parse);

    memset(&sDataset.mNetworkName, 0, sizeof(sDataset.mNetworkName));
    memcpy(sDataset.mNetworkName.m8, argv[0], length);
    sDataset.mIsNetworkNameSet = true;

exit:
    return error;
}

ThreadError Dataset::ProcessPanId(int argc, char *argv[])
{
    ThreadError error = kThreadError_None;
    long value;

    VerifyOrExit(argc > 0, error = kThreadError_Parse);
    SuccessOrExit(error = Interpreter::ParseLong(argv[0], value));
    sDataset.mPanId = static_cast<otPanId>(value);
    sDataset.mIsPanIdSet = true;

exit:
    return error;
}

ThreadError Dataset::ProcessPending(int argc, char *argv[])
{
    otOperationalDataset dataset;
    otGetPendingDataset(&dataset);

    (void)argc;
    (void)argv;
    return Print(dataset);
}

ThreadError Dataset::ProcessPendingTimestamp(int argc, char *argv[])
{
    ThreadError error = kThreadError_None;
    long value;

    VerifyOrExit(argc > 0, error = kThreadError_Parse);
    SuccessOrExit(error = Interpreter::ParseLong(argv[0], value));
    sDataset.mPendingTimestamp = static_cast<uint64_t>(value);
    sDataset.mIsPendingTimestampSet = true;

exit:
    return error;
}

ThreadError Dataset::ProcessMgmtCommand(int argc, char *argv[])
{
    ThreadError error = kThreadError_None;
    otOperationalDataset dataset;
    uint8_t tlvs[32];
    uint8_t size = 0;
    uint8_t index;
    long value;
    int length;
    otIp6Address prefix;

    VerifyOrExit(argc > 0, error = kThreadError_Parse);

    index = 2;

    if (strcmp(argv[0], "get") == 0)
    {
        while (index < argc)
        {
            if (strcmp(argv[index], "activetimestamp") == 0)
            {
                tlvs[size++] = OT_MESHCOP_TLV_ACTIVETIMESTAMP;
            }

            if (strcmp(argv[index], "pendingtimestamp") == 0)
            {
                tlvs[size++] = OT_MESHCOP_TLV_PENDINGTIMESTAMP;
            }

            if (strcmp(argv[index], "masterkey") == 0)
            {
                tlvs[size++] = OT_MESHCOP_TLV_MASTERKEY;
            }

            if (strcmp(argv[index], "networkname") == 0)
            {
                tlvs[size++] = OT_MESHCOP_TLV_NETWORKNAME;
            }

            if (strcmp(argv[index], "extpanid") == 0)
            {
                tlvs[size++] = OT_MESHCOP_TLV_EXTPANID;
            }

            if (strcmp(argv[index], "localprefix") == 0)
            {
                tlvs[size++] = OT_MESHCOP_TLV_LOCALPREFIX;
            }

            if (strcmp(argv[index], "delaytimer") == 0)
            {
                tlvs[size++] = OT_MESHCOP_TLV_DELAYTIMER;
            }

            if (strcmp(argv[index], "panid") == 0)
            {
                tlvs[size++] = OT_MESHCOP_TLV_PANID;
            }

            if (strcmp(argv[index], "channel") == 0)
            {
                tlvs[size++] = OT_MESHCOP_TLV_CHANNEL;
            }

            if (strcmp(argv[index], "binary") == 0)
            {
                VerifyOrExit((index + 1) < argc, error = kThreadError_Parse);
                SuccessOrExit(error = Interpreter::ParseLong(argv[++index], value));
                VerifyOrExit(Interpreter::Hex2Bin(argv[index + 1], tlvs + size, value) >= 0, error = kThreadError_Parse);
                size += value;
            }

            index++;
        }

        if (strcmp(argv[1], "active") == 0)
        {
            otSendActiveGet(tlvs, size);
        }
        else if (strcmp(argv[1], "pending") == 0)
        {
            otSendPendingGet(tlvs, size);
        }
        else
        {
            ExitNow(error = kThreadError_Parse);
        }
    }
    else if (strcmp(argv[0], "set") == 0)
    {
        memset(&dataset, 0, sizeof(dataset));

        while (index < argc)
        {
            if (strcmp(argv[index], "activetimestamp") == 0)
            {
                VerifyOrExit(index < argc, error = kThreadError_Parse);
                dataset.mIsActiveTimestampSet = true;
                SuccessOrExit(error = Interpreter::ParseLong(argv[++index], value));
                dataset.mActiveTimestamp = static_cast<uint64_t>(value);
            }

            if (strcmp(argv[index], "pendingtimestamp") == 0)
            {
                VerifyOrExit(index < argc, error = kThreadError_Parse);
                dataset.mIsPendingTimestampSet = true;
                SuccessOrExit(error = Interpreter::ParseLong(argv[++index], value));
                dataset.mPendingTimestamp = static_cast<uint64_t>(value);
            }

            if (strcmp(argv[index], "masterkey") == 0)
            {
                VerifyOrExit(index < argc, error = kThreadError_Parse);
                dataset.mIsMasterKeySet = true;
                VerifyOrExit((length = Interpreter::Hex2Bin(argv[++index], dataset.mMasterKey.m8,
                                                            sizeof(dataset.mMasterKey.m8))) == OT_MASTER_KEY_SIZE, error = kThreadError_Parse);
            }

            if (strcmp(argv[index], "networkname") == 0)
            {
                VerifyOrExit(index < argc, error = kThreadError_Parse);
                dataset.mIsNetworkNameSet = true;
                VerifyOrExit((length = strlen(argv[++index])) <= OT_NETWORK_NAME_MAX_SIZE, error = kThreadError_Parse);
                memset(&dataset.mNetworkName, 0, sizeof(sDataset.mNetworkName));
                memcpy(dataset.mNetworkName.m8, argv[0], length);
            }

            if (strcmp(argv[index], "extpanid") == 0)
            {
                VerifyOrExit(index < argc, error = kThreadError_Parse);
                dataset.mIsExtendedPanIdSet = true;
                VerifyOrExit(Interpreter::Hex2Bin(argv[++index], dataset.mExtendedPanId.m8,
                                                  sizeof(dataset.mExtendedPanId.m8)) >= 0, error = kThreadError_Parse);
            }

            if (strcmp(argv[index], "localprefix") == 0)
            {
                VerifyOrExit(index < argc, error = kThreadError_Parse);
                dataset.mIsMeshLocalPrefixSet = true;
                SuccessOrExit(error = otIp6AddressFromString(argv[++index], &prefix));
                memcpy(dataset.mMeshLocalPrefix.m8, prefix.mFields.m8, sizeof(dataset.mMeshLocalPrefix.m8));
            }

            if (strcmp(argv[index], "delaytimer") == 0)
            {
                VerifyOrExit(index < argc, error = kThreadError_Parse);
                dataset.mIsDelaySet = true;
                SuccessOrExit(error = Interpreter::ParseLong(argv[++index], value));
                dataset.mDelay = static_cast<uint32_t>(value);
            }

            if (strcmp(argv[index], "panid") == 0)
            {
                VerifyOrExit(index < argc, error = kThreadError_Parse);
                dataset.mIsPanIdSet = true;
                SuccessOrExit(error = Interpreter::ParseLong(argv[++index], value));
                dataset.mPanId = static_cast<otPanId>(value);
            }

            if (strcmp(argv[index], "channel") == 0)
            {
                VerifyOrExit(index < argc, error = kThreadError_Parse);
                dataset.mIsChannelSet = true;
                SuccessOrExit(error = Interpreter::ParseLong(argv[++index], value));
                dataset.mChannel = static_cast<uint16_t>(value);
            }

            if (strcmp(argv[index], "binary") == 0)
            {
                VerifyOrExit((index + 1) < argc, error = kThreadError_Parse);
                SuccessOrExit(error = Interpreter::ParseLong(argv[++index], value));
                VerifyOrExit(Interpreter::Hex2Bin(argv[++index], tlvs + size, value) >= 0, error = kThreadError_Parse);
                size += value;
            }

            index++;
        }

        if (strcmp(argv[1], "active") == 0)
        {
            otSendActiveSet(&dataset, tlvs, size);
        }
        else if (strcmp(argv[1], "pending") == 0)
        {
            otSendPendingSet(&dataset, tlvs, size);
        }
        else
        {
            ExitNow(error = kThreadError_Parse);
        }
    }
    else
    {
        error = kThreadError_Parse;
    }

exit:
    return error;
}

}  // namespace Cli
}  // namespace Thread
