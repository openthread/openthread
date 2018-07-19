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

#include "cli_dataset.hpp"

#include <stdio.h>
#include <stdlib.h>
#include "utils/wrap_string.h"

#include <openthread/openthread.h>

#if OPENTHREAD_FTD
#include <openthread/dataset_ftd.h>
#endif

#include "cli/cli.hpp"

namespace ot {
namespace Cli {

const DatasetCommand Dataset::sCommands[] = {
    {"help", &ProcessHelp},
    {"active", &ProcessActive},
#if OPENTHREAD_FTD
    {"activetimestamp", &ProcessActiveTimestamp},
    {"channel", &ProcessChannel},
    {"channelmask", &ProcessChannelMask},
    {"clear", &ProcessClear},
    {"commit", &ProcessCommit},
    {"delay", &ProcessDelay},
    {"extpanid", &ProcessExtPanId},
    {"masterkey", &ProcessMasterKey},
    {"meshlocalprefix", &ProcessMeshLocalPrefix},
    {"mgmtgetcommand", &ProcessMgmtGetCommand},
    {"mgmtsetcommand", &ProcessMgmtSetCommand},
    {"networkname", &ProcessNetworkName},
    {"panid", &ProcessPanId},
#endif
    {"pending", &ProcessPending},
#if OPENTHREAD_FTD
    {"pendingtimestamp", &ProcessPendingTimestamp},
    {"pskc", &ProcessPSKc},
    {"securitypolicy", &ProcessSecurityPolicy},
#endif
};

Server *             Dataset::sServer;
otOperationalDataset Dataset::sDataset;

void Dataset::OutputBytes(const uint8_t *aBytes, uint8_t aLength)
{
    for (int i = 0; i < aLength; i++)
    {
        sServer->OutputFormat("%02x", aBytes[i]);
    }
}

otError Dataset::Print(otOperationalDataset &aDataset)
{
    if (aDataset.mComponents.mIsPendingTimestampPresent)
    {
        sServer->OutputFormat("Pending Timestamp: %d\r\n", aDataset.mPendingTimestamp);
    }

    if (aDataset.mComponents.mIsActiveTimestampPresent)
    {
        sServer->OutputFormat("Active Timestamp: %d\r\n", aDataset.mActiveTimestamp);
    }

    if (aDataset.mComponents.mIsChannelPresent)
    {
        sServer->OutputFormat("Channel: %d\r\n", aDataset.mChannel);
    }

    if (aDataset.mComponents.mIsChannelMaskPage0Present)
    {
        sServer->OutputFormat("Channel Mask Page 0: %08x\r\n", aDataset.mChannelMaskPage0);
    }

    if (aDataset.mComponents.mIsDelayPresent)
    {
        sServer->OutputFormat("Delay: %d\r\n", aDataset.mDelay);
    }

    if (aDataset.mComponents.mIsExtendedPanIdPresent)
    {
        sServer->OutputFormat("Ext PAN ID: ");
        OutputBytes(aDataset.mExtendedPanId.m8, sizeof(aDataset.mExtendedPanId));
        sServer->OutputFormat("\r\n");
    }

    if (aDataset.mComponents.mIsMeshLocalPrefixPresent)
    {
        const uint8_t *prefix = aDataset.mMeshLocalPrefix.m8;
        sServer->OutputFormat(
            "Mesh Local Prefix: %x:%x:%x:%x/64\r\n", (static_cast<uint16_t>(prefix[0]) << 8) | prefix[1],
            (static_cast<uint16_t>(prefix[2]) << 8) | prefix[3], (static_cast<uint16_t>(prefix[4]) << 8) | prefix[5],
            (static_cast<uint16_t>(prefix[6]) << 8) | prefix[7]);
    }

    if (aDataset.mComponents.mIsMasterKeyPresent)
    {
        sServer->OutputFormat("Master Key: ");
        OutputBytes(aDataset.mMasterKey.m8, sizeof(aDataset.mMasterKey));
        sServer->OutputFormat("\r\n");
    }

    if (aDataset.mComponents.mIsNetworkNamePresent)
    {
        sServer->OutputFormat("Network Name: ");
        sServer->OutputFormat("%.*s\r\n", sizeof(aDataset.mNetworkName), aDataset.mNetworkName.m8);
    }

    if (aDataset.mComponents.mIsPanIdPresent)
    {
        sServer->OutputFormat("PAN ID: 0x%04x\r\n", aDataset.mPanId);
    }

    if (aDataset.mComponents.mIsPSKcPresent)
    {
        sServer->OutputFormat("PSKc: ");
        OutputBytes(aDataset.mPSKc.m8, sizeof(aDataset.mPSKc.m8));
        sServer->OutputFormat("\r\n");
    }

    if (aDataset.mComponents.mIsSecurityPolicyPresent)
    {
        sServer->OutputFormat("Security Policy: %d, ", aDataset.mSecurityPolicy.mRotationTime);

        if (aDataset.mSecurityPolicy.mFlags & OT_SECURITY_POLICY_OBTAIN_MASTER_KEY)
        {
            sServer->OutputFormat("o");
        }

        if (aDataset.mSecurityPolicy.mFlags & OT_SECURITY_POLICY_NATIVE_COMMISSIONING)
        {
            sServer->OutputFormat("n");
        }

        if (aDataset.mSecurityPolicy.mFlags & OT_SECURITY_POLICY_ROUTERS)
        {
            sServer->OutputFormat("r");
        }

        if (aDataset.mSecurityPolicy.mFlags & OT_SECURITY_POLICY_EXTERNAL_COMMISSIONER)
        {
            sServer->OutputFormat("c");
        }

        if (aDataset.mSecurityPolicy.mFlags & OT_SECURITY_POLICY_BEACONS)
        {
            sServer->OutputFormat("b");
        }

        sServer->OutputFormat("\r\n");
    }

    return OT_ERROR_NONE;
}

otError Dataset::Process(otInstance *aInstance, int argc, char *argv[], Server &aServer)
{
    otError error = OT_ERROR_PARSE;

    sServer = &aServer;

    if (argc == 0)
    {
        ExitNow(error = Print(sDataset));
    }

    for (unsigned int i = 0; i < OT_ARRAY_LENGTH(sCommands); i++)
    {
        if (strcmp(argv[0], sCommands[i].mName) == 0)
        {
            error = sCommands[i].mCommand(aInstance, argc - 1, argv + 1);
            break;
        }
    }

exit:
    return error;
}

otError Dataset::ProcessHelp(otInstance *aInstance, int argc, char *argv[])
{
    for (unsigned int i = 0; i < OT_ARRAY_LENGTH(sCommands); i++)
    {
        sServer->OutputFormat("%s\r\n", sCommands[i].mName);
    }

    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(argc);
    OT_UNUSED_VARIABLE(argv);
    return OT_ERROR_NONE;
}

otError Dataset::ProcessActive(otInstance *aInstance, int argc, char *argv[])
{
    otOperationalDataset dataset;
    otError              error;

    SuccessOrExit(error = otDatasetGetActive(aInstance, &dataset));
    error = Print(dataset);

exit:
    OT_UNUSED_VARIABLE(argc);
    OT_UNUSED_VARIABLE(argv);
    return error;
}

otError Dataset::ProcessPending(otInstance *aInstance, int argc, char *argv[])
{
    otOperationalDataset dataset;
    otError              error;

    SuccessOrExit(error = otDatasetGetPending(aInstance, &dataset));
    error = Print(dataset);

exit:
    OT_UNUSED_VARIABLE(argc);
    OT_UNUSED_VARIABLE(argv);
    return error;
}

#if OPENTHREAD_FTD
otError Dataset::ProcessActiveTimestamp(otInstance *aInstance, int argc, char *argv[])
{
    otError error = OT_ERROR_NONE;
    long    value;

    VerifyOrExit(argc > 0, error = OT_ERROR_INVALID_ARGS);
    SuccessOrExit(error = Interpreter::ParseLong(argv[0], value));
    sDataset.mActiveTimestamp                      = static_cast<uint64_t>(value);
    sDataset.mComponents.mIsActiveTimestampPresent = true;

    OT_UNUSED_VARIABLE(aInstance);

exit:
    return error;
}

otError Dataset::ProcessChannel(otInstance *aInstance, int argc, char *argv[])
{
    otError error = OT_ERROR_NONE;
    long    value;

    VerifyOrExit(argc > 0, error = OT_ERROR_INVALID_ARGS);
    SuccessOrExit(error = Interpreter::ParseLong(argv[0], value));
    sDataset.mChannel                      = static_cast<uint16_t>(value);
    sDataset.mComponents.mIsChannelPresent = true;

    OT_UNUSED_VARIABLE(aInstance);

exit:
    return error;
}

otError Dataset::ProcessChannelMask(otInstance *aInstance, int argc, char *argv[])
{
    otError error = OT_ERROR_NONE;
    long    value;

    VerifyOrExit(argc > 0, error = OT_ERROR_INVALID_ARGS);
    SuccessOrExit(error = Interpreter::ParseLong(argv[0], value));
    sDataset.mChannelMaskPage0                      = static_cast<uint32_t>(value);
    sDataset.mComponents.mIsChannelMaskPage0Present = true;
    OT_UNUSED_VARIABLE(aInstance);

exit:
    return error;
}

otError Dataset::ProcessClear(otInstance *aInstance, int argc, char *argv[])
{
    memset(&sDataset, 0, sizeof(sDataset));
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(argc);
    OT_UNUSED_VARIABLE(argv);
    return OT_ERROR_NONE;
}

otError Dataset::ProcessCommit(otInstance *aInstance, int argc, char *argv[])
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(argc > 0, error = OT_ERROR_INVALID_ARGS);

    if (strcmp(argv[0], "active") == 0)
    {
        SuccessOrExit(error = otDatasetSetActive(aInstance, &sDataset));
    }
    else if (strcmp(argv[0], "pending") == 0)
    {
        SuccessOrExit(error = otDatasetSetPending(aInstance, &sDataset));
    }
    else
    {
        ExitNow(error = OT_ERROR_INVALID_ARGS);
    }

    OT_UNUSED_VARIABLE(aInstance);

exit:
    return error;
}

otError Dataset::ProcessDelay(otInstance *aInstance, int argc, char *argv[])
{
    otError error = OT_ERROR_NONE;
    long    value;

    VerifyOrExit(argc > 0, error = OT_ERROR_INVALID_ARGS);
    SuccessOrExit(error = Interpreter::ParseLong(argv[0], value));
    sDataset.mDelay                      = static_cast<uint32_t>(value);
    sDataset.mComponents.mIsDelayPresent = true;

    OT_UNUSED_VARIABLE(aInstance);

exit:
    return error;
}

otError Dataset::ProcessExtPanId(otInstance *aInstance, int argc, char *argv[])
{
    otError error = OT_ERROR_NONE;
    uint8_t extPanId[OT_EXT_PAN_ID_SIZE];

    VerifyOrExit(argc > 0, error = OT_ERROR_INVALID_ARGS);
    VerifyOrExit(Interpreter::Hex2Bin(argv[0], extPanId, sizeof(extPanId)) >= 0, error = OT_ERROR_PARSE);

    memcpy(sDataset.mExtendedPanId.m8, extPanId, sizeof(sDataset.mExtendedPanId));
    sDataset.mComponents.mIsExtendedPanIdPresent = true;

    OT_UNUSED_VARIABLE(aInstance);

exit:
    return error;
}

otError Dataset::ProcessMasterKey(otInstance *aInstance, int argc, char *argv[])
{
    otError error = OT_ERROR_NONE;
    uint8_t key[OT_MASTER_KEY_SIZE];

    VerifyOrExit(argc > 0, error = OT_ERROR_INVALID_ARGS);
    VerifyOrExit((Interpreter::Hex2Bin(argv[0], key, sizeof(key))) == OT_MASTER_KEY_SIZE, error = OT_ERROR_PARSE);

    memcpy(sDataset.mMasterKey.m8, key, sizeof(sDataset.mMasterKey));
    sDataset.mComponents.mIsMasterKeyPresent = true;

    OT_UNUSED_VARIABLE(aInstance);

exit:
    return error;
}

otError Dataset::ProcessMeshLocalPrefix(otInstance *aInstance, int argc, char *argv[])
{
    otError      error = OT_ERROR_NONE;
    otIp6Address prefix;

    VerifyOrExit(argc > 0, error = OT_ERROR_INVALID_ARGS);
    SuccessOrExit(error = otIp6AddressFromString(argv[0], &prefix));

    memcpy(sDataset.mMeshLocalPrefix.m8, prefix.mFields.m8, sizeof(sDataset.mMeshLocalPrefix.m8));
    sDataset.mComponents.mIsMeshLocalPrefixPresent = true;

    OT_UNUSED_VARIABLE(aInstance);

exit:
    return error;
}

otError Dataset::ProcessNetworkName(otInstance *aInstance, int argc, char *argv[])
{
    otError error = OT_ERROR_NONE;
    size_t  length;

    VerifyOrExit(argc > 0, error = OT_ERROR_INVALID_ARGS);
    VerifyOrExit((length = strlen(argv[0])) <= OT_NETWORK_NAME_MAX_SIZE, error = OT_ERROR_PARSE);

    memset(&sDataset.mNetworkName, 0, sizeof(sDataset.mNetworkName));
    memcpy(sDataset.mNetworkName.m8, argv[0], length);
    sDataset.mComponents.mIsNetworkNamePresent = true;

    OT_UNUSED_VARIABLE(aInstance);

exit:
    return error;
}

otError Dataset::ProcessPanId(otInstance *aInstance, int argc, char *argv[])
{
    otError error = OT_ERROR_NONE;
    long    value;

    VerifyOrExit(argc > 0, error = OT_ERROR_INVALID_ARGS);
    SuccessOrExit(error = Interpreter::ParseLong(argv[0], value));
    sDataset.mPanId                      = static_cast<otPanId>(value);
    sDataset.mComponents.mIsPanIdPresent = true;

    OT_UNUSED_VARIABLE(aInstance);

exit:
    return error;
}

otError Dataset::ProcessPendingTimestamp(otInstance *aInstance, int argc, char *argv[])
{
    otError error = OT_ERROR_NONE;
    long    value;

    VerifyOrExit(argc > 0, error = OT_ERROR_INVALID_ARGS);
    SuccessOrExit(error = Interpreter::ParseLong(argv[0], value));
    sDataset.mPendingTimestamp                      = static_cast<uint64_t>(value);
    sDataset.mComponents.mIsPendingTimestampPresent = true;

    OT_UNUSED_VARIABLE(aInstance);

exit:
    return error;
}

otError Dataset::ProcessMgmtSetCommand(otInstance *aInstance, int argc, char *argv[])
{
    otError              error = OT_ERROR_NONE;
    otOperationalDataset dataset;
    uint8_t              tlvs[128];
    long                 value;
    int                  length = 0;
    otIp6Address         prefix;

    VerifyOrExit(argc > 0, error = OT_ERROR_INVALID_ARGS);

    memset(&dataset, 0, sizeof(dataset));

    for (uint8_t index = 1; index < argc; index++)
    {
        if (strcmp(argv[index], "activetimestamp") == 0)
        {
            VerifyOrExit(index < argc, error = OT_ERROR_INVALID_ARGS);
            dataset.mComponents.mIsActiveTimestampPresent = true;
            SuccessOrExit(error = Interpreter::ParseLong(argv[++index], value));
            dataset.mActiveTimestamp = static_cast<uint64_t>(value);
        }
        else if (strcmp(argv[index], "pendingtimestamp") == 0)
        {
            VerifyOrExit(index < argc, error = OT_ERROR_INVALID_ARGS);
            dataset.mComponents.mIsPendingTimestampPresent = true;
            SuccessOrExit(error = Interpreter::ParseLong(argv[++index], value));
            dataset.mPendingTimestamp = static_cast<uint64_t>(value);
        }
        else if (strcmp(argv[index], "masterkey") == 0)
        {
            VerifyOrExit(index < argc, error = OT_ERROR_INVALID_ARGS);
            dataset.mComponents.mIsMasterKeyPresent = true;
            VerifyOrExit((length = Interpreter::Hex2Bin(argv[++index], dataset.mMasterKey.m8,
                                                        sizeof(dataset.mMasterKey.m8))) == OT_MASTER_KEY_SIZE,
                         error = OT_ERROR_PARSE);
            length = 0;
        }
        else if (strcmp(argv[index], "networkname") == 0)
        {
            VerifyOrExit(index < argc, error = OT_ERROR_INVALID_ARGS);
            dataset.mComponents.mIsNetworkNamePresent = true;
            VerifyOrExit((length = static_cast<int>(strlen(argv[++index]))) <= OT_NETWORK_NAME_MAX_SIZE,
                         error = OT_ERROR_PARSE);
            memset(&dataset.mNetworkName, 0, sizeof(sDataset.mNetworkName));
            memcpy(dataset.mNetworkName.m8, argv[index], static_cast<size_t>(length));
            length = 0;
        }
        else if (strcmp(argv[index], "extpanid") == 0)
        {
            VerifyOrExit(index < argc, error = OT_ERROR_INVALID_ARGS);
            dataset.mComponents.mIsExtendedPanIdPresent = true;
            VerifyOrExit(
                Interpreter::Hex2Bin(argv[++index], dataset.mExtendedPanId.m8, sizeof(dataset.mExtendedPanId.m8)) >= 0,
                error = OT_ERROR_PARSE);
        }
        else if (strcmp(argv[index], "localprefix") == 0)
        {
            VerifyOrExit(index < argc, error = OT_ERROR_INVALID_ARGS);
            dataset.mComponents.mIsMeshLocalPrefixPresent = true;
            SuccessOrExit(error = otIp6AddressFromString(argv[++index], &prefix));
            memcpy(dataset.mMeshLocalPrefix.m8, prefix.mFields.m8, sizeof(dataset.mMeshLocalPrefix.m8));
        }
        else if (strcmp(argv[index], "delaytimer") == 0)
        {
            VerifyOrExit(index < argc, error = OT_ERROR_INVALID_ARGS);
            dataset.mComponents.mIsDelayPresent = true;
            SuccessOrExit(error = Interpreter::ParseLong(argv[++index], value));
            dataset.mDelay = static_cast<uint32_t>(value);
        }
        else if (strcmp(argv[index], "panid") == 0)
        {
            VerifyOrExit(index < argc, error = OT_ERROR_INVALID_ARGS);
            dataset.mComponents.mIsPanIdPresent = true;
            SuccessOrExit(error = Interpreter::ParseLong(argv[++index], value));
            dataset.mPanId = static_cast<otPanId>(value);
        }
        else if (strcmp(argv[index], "channel") == 0)
        {
            VerifyOrExit(index < argc, error = OT_ERROR_INVALID_ARGS);
            dataset.mComponents.mIsChannelPresent = true;
            SuccessOrExit(error = Interpreter::ParseLong(argv[++index], value));
            dataset.mChannel = static_cast<uint16_t>(value);
        }
        else if (strcmp(argv[index], "channelmask") == 0)
        {
            VerifyOrExit(index < argc, error = OT_ERROR_INVALID_ARGS);
            dataset.mComponents.mIsChannelMaskPage0Present = true;
            SuccessOrExit(error = Interpreter::ParseLong(argv[++index], value));
            dataset.mChannelMaskPage0 = static_cast<uint32_t>(value);
        }
        else if (strcmp(argv[index], "binary") == 0)
        {
            VerifyOrExit((index + 1) < argc, error = OT_ERROR_INVALID_ARGS);
            length = static_cast<int>((strlen(argv[++index]) + 1) / 2);
            VerifyOrExit(static_cast<size_t>(length) <= sizeof(tlvs), error = OT_ERROR_NO_BUFS);
            VerifyOrExit(Interpreter::Hex2Bin(argv[index], tlvs, static_cast<uint16_t>(length)) >= 0,
                         error = OT_ERROR_PARSE);
        }
        else
        {
            ExitNow(error = OT_ERROR_INVALID_ARGS);
        }
    }

    if (strcmp(argv[0], "active") == 0)
    {
        SuccessOrExit(error = otDatasetSendMgmtActiveSet(aInstance, &dataset, tlvs, static_cast<uint8_t>(length)));
    }
    else if (strcmp(argv[0], "pending") == 0)
    {
        SuccessOrExit(error = otDatasetSendMgmtPendingSet(aInstance, &dataset, tlvs, static_cast<uint8_t>(length)));
    }
    else
    {
        ExitNow(error = OT_ERROR_INVALID_ARGS);
    }

    OT_UNUSED_VARIABLE(aInstance);

exit:
    return error;
}

otError Dataset::ProcessMgmtGetCommand(otInstance *aInstance, int argc, char *argv[])
{
    otError                        error = OT_ERROR_NONE;
    otOperationalDatasetComponents datasetComponents;
    uint8_t                        tlvs[32];
    long                           value;
    int                            length            = 0;
    bool                           destAddrSpecified = false;
    otIp6Address                   address;

    VerifyOrExit(argc > 0, error = OT_ERROR_INVALID_ARGS);

    memset(&datasetComponents, 0, sizeof(datasetComponents));

    for (uint8_t index = 1; index < argc; index++)
    {
        VerifyOrExit(static_cast<size_t>(length) < sizeof(tlvs), error = OT_ERROR_NO_BUFS);

        if (strcmp(argv[index], "activetimestamp") == 0)
        {
            datasetComponents.mIsActiveTimestampPresent = true;
        }
        else if (strcmp(argv[index], "pendingtimestamp") == 0)
        {
            datasetComponents.mIsPendingTimestampPresent = true;
        }
        else if (strcmp(argv[index], "masterkey") == 0)
        {
            datasetComponents.mIsMasterKeyPresent = true;
        }
        else if (strcmp(argv[index], "networkname") == 0)
        {
            datasetComponents.mIsNetworkNamePresent = true;
        }
        else if (strcmp(argv[index], "extpanid") == 0)
        {
            datasetComponents.mIsExtendedPanIdPresent = true;
        }
        else if (strcmp(argv[index], "localprefix") == 0)
        {
            datasetComponents.mIsMeshLocalPrefixPresent = true;
        }
        else if (strcmp(argv[index], "delaytimer") == 0)
        {
            datasetComponents.mIsDelayPresent = true;
        }
        else if (strcmp(argv[index], "panid") == 0)
        {
            datasetComponents.mIsPanIdPresent = true;
        }
        else if (strcmp(argv[index], "channel") == 0)
        {
            datasetComponents.mIsChannelPresent = true;
        }
        else if (strcmp(argv[index], "binary") == 0)
        {
            VerifyOrExit((index + 1) < argc, error = OT_ERROR_INVALID_ARGS);
            value = static_cast<long>(strlen(argv[++index]) + 1) / 2;
            VerifyOrExit(static_cast<size_t>(value) <= (sizeof(tlvs) - static_cast<size_t>(length)),
                         error = OT_ERROR_NO_BUFS);
            VerifyOrExit(Interpreter::Hex2Bin(argv[index], tlvs + length, static_cast<uint16_t>(value)) >= 0,
                         error = OT_ERROR_PARSE);
            length += value;
        }
        else if (strcmp(argv[index], "address") == 0)
        {
            VerifyOrExit(index < argc, error = OT_ERROR_INVALID_ARGS);
            SuccessOrExit(error = otIp6AddressFromString(argv[++index], &address));
            destAddrSpecified = true;
        }
        else
        {
            ExitNow(error = OT_ERROR_INVALID_ARGS);
        }
    }

    if (strcmp(argv[0], "active") == 0)
    {
        SuccessOrExit(error =
                          otDatasetSendMgmtActiveGet(aInstance, &datasetComponents, tlvs, static_cast<uint8_t>(length),
                                                     destAddrSpecified ? &address : NULL));
    }
    else if (strcmp(argv[0], "pending") == 0)
    {
        SuccessOrExit(error =
                          otDatasetSendMgmtPendingGet(aInstance, &datasetComponents, tlvs, static_cast<uint8_t>(length),
                                                      destAddrSpecified ? &address : NULL));
    }
    else
    {
        ExitNow(error = OT_ERROR_INVALID_ARGS);
    }

    OT_UNUSED_VARIABLE(aInstance);

exit:
    return error;
}

otError Dataset::ProcessPSKc(otInstance *aInstance, int argc, char *argv[])
{
    otError  error = OT_ERROR_NONE;
    uint16_t length;

    VerifyOrExit(argc > 0, error = OT_ERROR_INVALID_ARGS);
    length = static_cast<uint16_t>((strlen(argv[0]) + 1) / 2);
    VerifyOrExit(length <= OT_PSKC_MAX_SIZE, error = OT_ERROR_NO_BUFS);
    VerifyOrExit(Interpreter::Hex2Bin(argv[0], sDataset.mPSKc.m8 + OT_PSKC_MAX_SIZE - length, length) == length,
                 error = OT_ERROR_PARSE);

    sDataset.mComponents.mIsPSKcPresent = true;
    OT_UNUSED_VARIABLE(aInstance);

exit:
    return error;
}

otError Dataset::ProcessSecurityPolicy(otInstance *aInstance, int argc, char *argv[])
{
    otError error = OT_ERROR_NONE;
    long    value;

    VerifyOrExit(argc > 0, error = OT_ERROR_INVALID_ARGS);

    SuccessOrExit(error = Interpreter::ParseLong(argv[0], value));
    sDataset.mSecurityPolicy.mRotationTime = static_cast<uint16_t>(value);
    sDataset.mSecurityPolicy.mFlags        = 0;

    if (argc > 1)
    {
        for (char *arg = argv[1]; *arg != '\0'; arg++)
        {
            switch (*arg)
            {
            case 'o':
                sDataset.mSecurityPolicy.mFlags |= OT_SECURITY_POLICY_OBTAIN_MASTER_KEY;
                break;

            case 'n':
                sDataset.mSecurityPolicy.mFlags |= OT_SECURITY_POLICY_NATIVE_COMMISSIONING;
                break;

            case 'r':
                sDataset.mSecurityPolicy.mFlags |= OT_SECURITY_POLICY_ROUTERS;
                break;

            case 'c':
                sDataset.mSecurityPolicy.mFlags |= OT_SECURITY_POLICY_EXTERNAL_COMMISSIONER;
                break;

            case 'b':
                sDataset.mSecurityPolicy.mFlags |= OT_SECURITY_POLICY_BEACONS;
                break;

            default:
                ExitNow(error = OT_ERROR_PARSE);
            }
        }
    }

    sDataset.mComponents.mIsSecurityPolicyPresent = true;
    OT_UNUSED_VARIABLE(aInstance);

exit:
    return error;
}
#endif // OPENTHREAD_FTD

} // namespace Cli
} // namespace ot
