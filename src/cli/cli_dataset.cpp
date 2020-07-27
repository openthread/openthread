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

#include <openthread/dataset.h>
#include <openthread/dataset_ftd.h>

#include "cli/cli.hpp"
#include "cli/cli_server.hpp"

namespace ot {
namespace Cli {

const Dataset::Command Dataset::sCommands[] = {
    {"help", &Dataset::ProcessHelp},
    {"active", &Dataset::ProcessActive},
    {"activetimestamp", &Dataset::ProcessActiveTimestamp},
    {"channel", &Dataset::ProcessChannel},
    {"channelmask", &Dataset::ProcessChannelMask},
    {"clear", &Dataset::ProcessClear},
    {"commit", &Dataset::ProcessCommit},
    {"delay", &Dataset::ProcessDelay},
    {"extpanid", &Dataset::ProcessExtPanId},
    {"init", &Dataset::ProcessInit},
    {"masterkey", &Dataset::ProcessMasterKey},
    {"meshlocalprefix", &Dataset::ProcessMeshLocalPrefix},
    {"mgmtgetcommand", &Dataset::ProcessMgmtGetCommand},
    {"mgmtsetcommand", &Dataset::ProcessMgmtSetCommand},
    {"networkname", &Dataset::ProcessNetworkName},
    {"panid", &Dataset::ProcessPanId},
    {"pending", &Dataset::ProcessPending},
    {"pendingtimestamp", &Dataset::ProcessPendingTimestamp},
    {"pskc", &Dataset::ProcessPskc},
    {"securitypolicy", &Dataset::ProcessSecurityPolicy},
    {"set", &Dataset::ProcessSet},
};

otOperationalDataset Dataset::sDataset;

void Dataset::OutputBytes(const uint8_t *aBytes, uint8_t aLength)
{
    for (int i = 0; i < aLength; i++)
    {
        mInterpreter.mServer->OutputFormat("%02x", aBytes[i]);
    }
}

otError Dataset::Print(otOperationalDataset &aDataset)
{
    if (aDataset.mComponents.mIsPendingTimestampPresent)
    {
        mInterpreter.mServer->OutputFormat("Pending Timestamp: %lu\r\n", aDataset.mPendingTimestamp);
    }

    if (aDataset.mComponents.mIsActiveTimestampPresent)
    {
        mInterpreter.mServer->OutputFormat("Active Timestamp: %lu\r\n", aDataset.mActiveTimestamp);
    }

    if (aDataset.mComponents.mIsChannelPresent)
    {
        mInterpreter.mServer->OutputFormat("Channel: %d\r\n", aDataset.mChannel);
    }

    if (aDataset.mComponents.mIsChannelMaskPresent)
    {
        mInterpreter.mServer->OutputFormat("Channel Mask: %08x\r\n", aDataset.mChannelMask);
    }

    if (aDataset.mComponents.mIsDelayPresent)
    {
        mInterpreter.mServer->OutputFormat("Delay: %d\r\n", aDataset.mDelay);
    }

    if (aDataset.mComponents.mIsExtendedPanIdPresent)
    {
        mInterpreter.mServer->OutputFormat("Ext PAN ID: ");
        OutputBytes(aDataset.mExtendedPanId.m8, sizeof(aDataset.mExtendedPanId));
        mInterpreter.mServer->OutputFormat("\r\n");
    }

    if (aDataset.mComponents.mIsMeshLocalPrefixPresent)
    {
        const uint8_t *prefix = aDataset.mMeshLocalPrefix.m8;
        mInterpreter.mServer->OutputFormat(
            "Mesh Local Prefix: %x:%x:%x:%x/64\r\n", (static_cast<uint16_t>(prefix[0]) << 8) | prefix[1],
            (static_cast<uint16_t>(prefix[2]) << 8) | prefix[3], (static_cast<uint16_t>(prefix[4]) << 8) | prefix[5],
            (static_cast<uint16_t>(prefix[6]) << 8) | prefix[7]);
    }

    if (aDataset.mComponents.mIsMasterKeyPresent)
    {
        mInterpreter.mServer->OutputFormat("Master Key: ");
        OutputBytes(aDataset.mMasterKey.m8, sizeof(aDataset.mMasterKey));
        mInterpreter.mServer->OutputFormat("\r\n");
    }

    if (aDataset.mComponents.mIsNetworkNamePresent)
    {
        mInterpreter.mServer->OutputFormat("Network Name: ");
        mInterpreter.mServer->OutputFormat("%.*s\r\n", static_cast<uint16_t>(sizeof(aDataset.mNetworkName)),
                                           aDataset.mNetworkName.m8);
    }

    if (aDataset.mComponents.mIsPanIdPresent)
    {
        mInterpreter.mServer->OutputFormat("PAN ID: 0x%04x\r\n", aDataset.mPanId);
    }

    if (aDataset.mComponents.mIsPskcPresent)
    {
        mInterpreter.mServer->OutputFormat("PSKc: ");
        OutputBytes(aDataset.mPskc.m8, sizeof(aDataset.mPskc.m8));
        mInterpreter.mServer->OutputFormat("\r\n");
    }

    if (aDataset.mComponents.mIsSecurityPolicyPresent)
    {
        mInterpreter.mServer->OutputFormat("Security Policy: %d, ", aDataset.mSecurityPolicy.mRotationTime);

        if (aDataset.mSecurityPolicy.mFlags & OT_SECURITY_POLICY_OBTAIN_MASTER_KEY)
        {
            mInterpreter.mServer->OutputFormat("o");
        }

        if (aDataset.mSecurityPolicy.mFlags & OT_SECURITY_POLICY_NATIVE_COMMISSIONING)
        {
            mInterpreter.mServer->OutputFormat("n");
        }

        if (aDataset.mSecurityPolicy.mFlags & OT_SECURITY_POLICY_ROUTERS)
        {
            mInterpreter.mServer->OutputFormat("r");
        }

        if (aDataset.mSecurityPolicy.mFlags & OT_SECURITY_POLICY_EXTERNAL_COMMISSIONER)
        {
            mInterpreter.mServer->OutputFormat("c");
        }

        if (aDataset.mSecurityPolicy.mFlags & OT_SECURITY_POLICY_BEACONS)
        {
            mInterpreter.mServer->OutputFormat("b");
        }

        mInterpreter.mServer->OutputFormat("\r\n");
    }

    return OT_ERROR_NONE;
}

otError Dataset::Process(uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_INVALID_COMMAND;

    if (aArgsLength == 0)
    {
        ExitNow(error = Print(sDataset));
    }

    for (const Command &command : sCommands)
    {
        if (strcmp(aArgs[0], command.mName) == 0)
        {
            error = (this->*command.mCommand)(aArgsLength - 1, aArgs + 1);
            break;
        }
    }

exit:
    return error;
}

otError Dataset::ProcessHelp(uint8_t aArgsLength, char *aArgs[])
{
    OT_UNUSED_VARIABLE(aArgsLength);
    OT_UNUSED_VARIABLE(aArgs);

    for (const Command &command : sCommands)
    {
        mInterpreter.mServer->OutputFormat("%s\r\n", command.mName);
    }

    return OT_ERROR_NONE;
}

otError Dataset::ProcessInit(uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(aArgsLength > 0, error = OT_ERROR_INVALID_ARGS);

    if (strcmp(aArgs[0], "active") == 0)
    {
        SuccessOrExit(error = otDatasetGetActive(mInterpreter.mInstance, &sDataset));
    }
    else if (strcmp(aArgs[0], "pending") == 0)
    {
        SuccessOrExit(error = otDatasetGetPending(mInterpreter.mInstance, &sDataset));
    }
#if OPENTHREAD_FTD
    else if (strcmp(aArgs[0], "new") == 0)
    {
        SuccessOrExit(error = otDatasetCreateNewNetwork(mInterpreter.mInstance, &sDataset));
    }
#endif
    else
    {
        ExitNow(error = OT_ERROR_INVALID_ARGS);
    }

exit:
    return error;
}

otError Dataset::ProcessActive(uint8_t aArgsLength, char *aArgs[])
{
    otError error;

    if (aArgsLength == 0)
    {
        otOperationalDataset dataset;

        SuccessOrExit(error = otDatasetGetActive(mInterpreter.mInstance, &dataset));
        error = Print(dataset);
    }
    else if ((aArgsLength == 1) && (strcmp(aArgs[0], "binary") == 0))
    {
        otOperationalDatasetTlvs dataset;

        VerifyOrExit(strlen(aArgs[0]) <= OT_OPERATIONAL_DATASET_MAX_LENGTH * 2, error = OT_ERROR_NO_BUFS);

        SuccessOrExit(error = otDatasetGetActiveTlvs(mInterpreter.mInstance, &dataset));
        mInterpreter.OutputBytes(dataset.mTlvs, dataset.mLength);
        mInterpreter.mServer->OutputFormat("\r\n");
    }
    else
    {
        ExitNow(error = OT_ERROR_INVALID_ARGS);
    }

exit:
    return error;
}

otError Dataset::ProcessPending(uint8_t aArgsLength, char *aArgs[])
{
    otError error;

    if (aArgsLength == 0)
    {
        otOperationalDataset dataset;

        SuccessOrExit(error = otDatasetGetPending(mInterpreter.mInstance, &dataset));
        error = Print(dataset);
    }
    else if ((aArgsLength == 1) && (strcmp(aArgs[0], "binary") == 0))
    {
        otOperationalDatasetTlvs dataset;

        VerifyOrExit(strlen(aArgs[0]) <= OT_OPERATIONAL_DATASET_MAX_LENGTH * 2, error = OT_ERROR_NO_BUFS);

        SuccessOrExit(error = otDatasetGetPendingTlvs(mInterpreter.mInstance, &dataset));
        mInterpreter.OutputBytes(dataset.mTlvs, dataset.mLength);
        mInterpreter.mServer->OutputFormat("\r\n");
    }
    else
    {
        ExitNow(error = OT_ERROR_INVALID_ARGS);
    }

exit:
    return error;
}

otError Dataset::ProcessActiveTimestamp(uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_NONE;
    long    value;

    VerifyOrExit(aArgsLength > 0, error = OT_ERROR_INVALID_ARGS);
    SuccessOrExit(error = Interpreter::ParseLong(aArgs[0], value));
    sDataset.mActiveTimestamp                      = static_cast<uint64_t>(value);
    sDataset.mComponents.mIsActiveTimestampPresent = true;

exit:
    return error;
}

otError Dataset::ProcessChannel(uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_NONE;
    long    value;

    VerifyOrExit(aArgsLength > 0, error = OT_ERROR_INVALID_ARGS);
    SuccessOrExit(error = Interpreter::ParseLong(aArgs[0], value));
    sDataset.mChannel                      = static_cast<uint16_t>(value);
    sDataset.mComponents.mIsChannelPresent = true;

exit:
    return error;
}

otError Dataset::ProcessChannelMask(uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_NONE;
    long    value;

    VerifyOrExit(aArgsLength > 0, error = OT_ERROR_INVALID_ARGS);
    SuccessOrExit(error = Interpreter::ParseLong(aArgs[0], value));
    sDataset.mChannelMask                      = static_cast<uint32_t>(value);
    sDataset.mComponents.mIsChannelMaskPresent = true;

exit:
    return error;
}

otError Dataset::ProcessClear(uint8_t aArgsLength, char *aArgs[])
{
    OT_UNUSED_VARIABLE(aArgsLength);
    OT_UNUSED_VARIABLE(aArgs);

    memset(&sDataset, 0, sizeof(sDataset));
    return OT_ERROR_NONE;
}

otError Dataset::ProcessCommit(uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(aArgsLength > 0, error = OT_ERROR_INVALID_ARGS);

    if (strcmp(aArgs[0], "active") == 0)
    {
        SuccessOrExit(error = otDatasetSetActive(mInterpreter.mInstance, &sDataset));
    }
    else if (strcmp(aArgs[0], "pending") == 0)
    {
        SuccessOrExit(error = otDatasetSetPending(mInterpreter.mInstance, &sDataset));
    }
    else
    {
        ExitNow(error = OT_ERROR_INVALID_ARGS);
    }

exit:
    return error;
}

otError Dataset::ProcessDelay(uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_NONE;
    long    value;

    VerifyOrExit(aArgsLength > 0, error = OT_ERROR_INVALID_ARGS);
    SuccessOrExit(error = Interpreter::ParseLong(aArgs[0], value));
    sDataset.mDelay                      = static_cast<uint32_t>(value);
    sDataset.mComponents.mIsDelayPresent = true;

exit:
    return error;
}

otError Dataset::ProcessExtPanId(uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_NONE;
    uint8_t extPanId[OT_EXT_PAN_ID_SIZE];

    VerifyOrExit(aArgsLength > 0, error = OT_ERROR_INVALID_ARGS);
    VerifyOrExit(Interpreter::Hex2Bin(aArgs[0], extPanId, sizeof(extPanId)) == sizeof(extPanId),
                 error = OT_ERROR_INVALID_ARGS);

    memcpy(sDataset.mExtendedPanId.m8, extPanId, sizeof(sDataset.mExtendedPanId));
    sDataset.mComponents.mIsExtendedPanIdPresent = true;

exit:
    return error;
}

otError Dataset::ProcessMasterKey(uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_NONE;
    uint8_t key[OT_MASTER_KEY_SIZE];

    VerifyOrExit(aArgsLength > 0, error = OT_ERROR_INVALID_ARGS);
    VerifyOrExit((Interpreter::Hex2Bin(aArgs[0], key, sizeof(key))) == OT_MASTER_KEY_SIZE,
                 error = OT_ERROR_INVALID_ARGS);

    memcpy(sDataset.mMasterKey.m8, key, sizeof(sDataset.mMasterKey));
    sDataset.mComponents.mIsMasterKeyPresent = true;

exit:
    return error;
}

otError Dataset::ProcessMeshLocalPrefix(uint8_t aArgsLength, char *aArgs[])
{
    otError      error = OT_ERROR_NONE;
    otIp6Address prefix;

    VerifyOrExit(aArgsLength > 0, error = OT_ERROR_INVALID_ARGS);
    SuccessOrExit(error = otIp6AddressFromString(aArgs[0], &prefix));

    memcpy(sDataset.mMeshLocalPrefix.m8, prefix.mFields.m8, sizeof(sDataset.mMeshLocalPrefix.m8));
    sDataset.mComponents.mIsMeshLocalPrefixPresent = true;

exit:
    return error;
}

otError Dataset::ProcessNetworkName(uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_NONE;
    size_t  length;

    VerifyOrExit(aArgsLength > 0, error = OT_ERROR_INVALID_ARGS);
    VerifyOrExit((length = strlen(aArgs[0])) <= OT_NETWORK_NAME_MAX_SIZE, error = OT_ERROR_INVALID_ARGS);

    memset(&sDataset.mNetworkName, 0, sizeof(sDataset.mNetworkName));
    memcpy(sDataset.mNetworkName.m8, aArgs[0], length);
    sDataset.mComponents.mIsNetworkNamePresent = true;

exit:
    return error;
}

otError Dataset::ProcessPanId(uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_NONE;
    long    value;

    VerifyOrExit(aArgsLength > 0, error = OT_ERROR_INVALID_ARGS);
    SuccessOrExit(error = Interpreter::ParseLong(aArgs[0], value));
    sDataset.mPanId                      = static_cast<otPanId>(value);
    sDataset.mComponents.mIsPanIdPresent = true;

exit:
    return error;
}

otError Dataset::ProcessPendingTimestamp(uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_NONE;
    long    value;

    VerifyOrExit(aArgsLength > 0, error = OT_ERROR_INVALID_ARGS);
    SuccessOrExit(error = Interpreter::ParseLong(aArgs[0], value));
    sDataset.mPendingTimestamp                      = static_cast<uint64_t>(value);
    sDataset.mComponents.mIsPendingTimestampPresent = true;

exit:
    return error;
}

otError Dataset::ProcessMgmtSetCommand(uint8_t aArgsLength, char *aArgs[])
{
    otError              error = OT_ERROR_NONE;
    otOperationalDataset dataset;
    uint8_t              tlvs[128];
    long                 value;
    int                  length = 0;
    otIp6Address         prefix;

    VerifyOrExit(aArgsLength > 0, error = OT_ERROR_INVALID_ARGS);

    memset(&dataset, 0, sizeof(dataset));

    for (uint8_t index = 1; index < aArgsLength; index++)
    {
        if (strcmp(aArgs[index], "activetimestamp") == 0)
        {
            VerifyOrExit(++index < aArgsLength, error = OT_ERROR_INVALID_ARGS);
            dataset.mComponents.mIsActiveTimestampPresent = true;
            SuccessOrExit(error = Interpreter::ParseLong(aArgs[index], value));
            dataset.mActiveTimestamp = static_cast<uint64_t>(value);
        }
        else if (strcmp(aArgs[index], "pendingtimestamp") == 0)
        {
            VerifyOrExit(++index < aArgsLength, error = OT_ERROR_INVALID_ARGS);
            dataset.mComponents.mIsPendingTimestampPresent = true;
            SuccessOrExit(error = Interpreter::ParseLong(aArgs[index], value));
            dataset.mPendingTimestamp = static_cast<uint64_t>(value);
        }
        else if (strcmp(aArgs[index], "masterkey") == 0)
        {
            VerifyOrExit(++index < aArgsLength, error = OT_ERROR_INVALID_ARGS);
            dataset.mComponents.mIsMasterKeyPresent = true;
            VerifyOrExit((length = Interpreter::Hex2Bin(aArgs[index], dataset.mMasterKey.m8,
                                                        sizeof(dataset.mMasterKey.m8))) == OT_MASTER_KEY_SIZE,
                         error = OT_ERROR_INVALID_ARGS);
            length = 0;
        }
        else if (strcmp(aArgs[index], "networkname") == 0)
        {
            VerifyOrExit(++index < aArgsLength, error = OT_ERROR_INVALID_ARGS);
            dataset.mComponents.mIsNetworkNamePresent = true;
            VerifyOrExit((length = static_cast<int>(strlen(aArgs[index]))) <= OT_NETWORK_NAME_MAX_SIZE,
                         error = OT_ERROR_INVALID_ARGS);
            memset(&dataset.mNetworkName, 0, sizeof(sDataset.mNetworkName));
            memcpy(dataset.mNetworkName.m8, aArgs[index], static_cast<size_t>(length));
            length = 0;
        }
        else if (strcmp(aArgs[index], "extpanid") == 0)
        {
            VerifyOrExit(++index < aArgsLength, error = OT_ERROR_INVALID_ARGS);
            dataset.mComponents.mIsExtendedPanIdPresent = true;
            VerifyOrExit(Interpreter::Hex2Bin(aArgs[index], dataset.mExtendedPanId.m8,
                                              sizeof(dataset.mExtendedPanId.m8)) == sizeof(dataset.mExtendedPanId.m8),
                         error = OT_ERROR_INVALID_ARGS);
        }
        else if (strcmp(aArgs[index], "localprefix") == 0)
        {
            VerifyOrExit(++index < aArgsLength, error = OT_ERROR_INVALID_ARGS);
            dataset.mComponents.mIsMeshLocalPrefixPresent = true;
            SuccessOrExit(error = otIp6AddressFromString(aArgs[index], &prefix));
            memcpy(dataset.mMeshLocalPrefix.m8, prefix.mFields.m8, sizeof(dataset.mMeshLocalPrefix.m8));
        }
        else if (strcmp(aArgs[index], "delaytimer") == 0)
        {
            VerifyOrExit(++index < aArgsLength, error = OT_ERROR_INVALID_ARGS);
            dataset.mComponents.mIsDelayPresent = true;
            SuccessOrExit(error = Interpreter::ParseLong(aArgs[index], value));
            dataset.mDelay = static_cast<uint32_t>(value);
        }
        else if (strcmp(aArgs[index], "panid") == 0)
        {
            VerifyOrExit(++index < aArgsLength, error = OT_ERROR_INVALID_ARGS);
            dataset.mComponents.mIsPanIdPresent = true;
            SuccessOrExit(error = Interpreter::ParseLong(aArgs[index], value));
            dataset.mPanId = static_cast<otPanId>(value);
        }
        else if (strcmp(aArgs[index], "channel") == 0)
        {
            VerifyOrExit(++index < aArgsLength, error = OT_ERROR_INVALID_ARGS);
            dataset.mComponents.mIsChannelPresent = true;
            SuccessOrExit(error = Interpreter::ParseLong(aArgs[index], value));
            dataset.mChannel = static_cast<uint16_t>(value);
        }
        else if (strcmp(aArgs[index], "channelmask") == 0)
        {
            VerifyOrExit(++index < aArgsLength, error = OT_ERROR_INVALID_ARGS);
            dataset.mComponents.mIsChannelMaskPresent = true;
            SuccessOrExit(error = Interpreter::ParseLong(aArgs[index], value));
            dataset.mChannelMask = static_cast<uint32_t>(value);
        }
        else if (strcmp(aArgs[index], "binary") == 0)
        {
            VerifyOrExit(++index < aArgsLength, error = OT_ERROR_INVALID_ARGS);
            length = static_cast<int>((strlen(aArgs[index]) + 1) / 2);
            VerifyOrExit(static_cast<size_t>(length) <= sizeof(tlvs), error = OT_ERROR_NO_BUFS);
            VerifyOrExit(Interpreter::Hex2Bin(aArgs[index], tlvs, static_cast<uint16_t>(length)) == length,
                         error = OT_ERROR_INVALID_ARGS);
        }
        else
        {
            ExitNow(error = OT_ERROR_INVALID_ARGS);
        }
    }

    if (strcmp(aArgs[0], "active") == 0)
    {
        SuccessOrExit(
            error = otDatasetSendMgmtActiveSet(mInterpreter.mInstance, &dataset, tlvs, static_cast<uint8_t>(length)));
    }
    else if (strcmp(aArgs[0], "pending") == 0)
    {
        SuccessOrExit(
            error = otDatasetSendMgmtPendingSet(mInterpreter.mInstance, &dataset, tlvs, static_cast<uint8_t>(length)));
    }
    else
    {
        ExitNow(error = OT_ERROR_INVALID_ARGS);
    }

exit:
    return error;
}

otError Dataset::ProcessMgmtGetCommand(uint8_t aArgsLength, char *aArgs[])
{
    otError                        error = OT_ERROR_NONE;
    otOperationalDatasetComponents datasetComponents;
    uint8_t                        tlvs[32];
    long                           value;
    int                            length            = 0;
    bool                           destAddrSpecified = false;
    otIp6Address                   address;

    VerifyOrExit(aArgsLength > 0, error = OT_ERROR_INVALID_ARGS);

    memset(&datasetComponents, 0, sizeof(datasetComponents));

    for (uint8_t index = 1; index < aArgsLength; index++)
    {
        VerifyOrExit(static_cast<size_t>(length) < sizeof(tlvs), error = OT_ERROR_NO_BUFS);

        if (strcmp(aArgs[index], "activetimestamp") == 0)
        {
            datasetComponents.mIsActiveTimestampPresent = true;
        }
        else if (strcmp(aArgs[index], "pendingtimestamp") == 0)
        {
            datasetComponents.mIsPendingTimestampPresent = true;
        }
        else if (strcmp(aArgs[index], "masterkey") == 0)
        {
            datasetComponents.mIsMasterKeyPresent = true;
        }
        else if (strcmp(aArgs[index], "networkname") == 0)
        {
            datasetComponents.mIsNetworkNamePresent = true;
        }
        else if (strcmp(aArgs[index], "extpanid") == 0)
        {
            datasetComponents.mIsExtendedPanIdPresent = true;
        }
        else if (strcmp(aArgs[index], "localprefix") == 0)
        {
            datasetComponents.mIsMeshLocalPrefixPresent = true;
        }
        else if (strcmp(aArgs[index], "delaytimer") == 0)
        {
            datasetComponents.mIsDelayPresent = true;
        }
        else if (strcmp(aArgs[index], "panid") == 0)
        {
            datasetComponents.mIsPanIdPresent = true;
        }
        else if (strcmp(aArgs[index], "channel") == 0)
        {
            datasetComponents.mIsChannelPresent = true;
        }
        else if (strcmp(aArgs[index], "binary") == 0)
        {
            VerifyOrExit(++index < aArgsLength, error = OT_ERROR_INVALID_ARGS);
            value = static_cast<long>(strlen(aArgs[index]) + 1) / 2;
            VerifyOrExit(static_cast<size_t>(value) <= (sizeof(tlvs) - static_cast<size_t>(length)),
                         error = OT_ERROR_NO_BUFS);
            VerifyOrExit(Interpreter::Hex2Bin(aArgs[index], tlvs + length, static_cast<uint16_t>(value)) == value,
                         error = OT_ERROR_INVALID_ARGS);
            length += value;
        }
        else if (strcmp(aArgs[index], "address") == 0)
        {
            VerifyOrExit(++index < aArgsLength, error = OT_ERROR_INVALID_ARGS);
            SuccessOrExit(error = otIp6AddressFromString(aArgs[index], &address));
            destAddrSpecified = true;
        }
        else
        {
            ExitNow(error = OT_ERROR_INVALID_ARGS);
        }
    }

    if (strcmp(aArgs[0], "active") == 0)
    {
        SuccessOrExit(error = otDatasetSendMgmtActiveGet(mInterpreter.mInstance, &datasetComponents, tlvs,
                                                         static_cast<uint8_t>(length),
                                                         destAddrSpecified ? &address : nullptr));
    }
    else if (strcmp(aArgs[0], "pending") == 0)
    {
        SuccessOrExit(error = otDatasetSendMgmtPendingGet(mInterpreter.mInstance, &datasetComponents, tlvs,
                                                          static_cast<uint8_t>(length),
                                                          destAddrSpecified ? &address : nullptr));
    }
    else
    {
        ExitNow(error = OT_ERROR_INVALID_ARGS);
    }

exit:
    return error;
}

otError Dataset::ProcessPskc(uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_NONE;

    if (aArgsLength == 1)
    {
        VerifyOrExit(Interpreter::Hex2Bin(aArgs[0], sDataset.mPskc.m8, sizeof(sDataset.mPskc)) ==
                         sizeof(sDataset.mPskc),
                     error = OT_ERROR_INVALID_ARGS);
    }
#if OPENTHREAD_FTD
    else if (aArgsLength == 2 && !strcmp(aArgs[0], "-p"))
    {
        SuccessOrExit(
            error = otDatasetGeneratePskc(
                aArgs[1],
                (sDataset.mComponents.mIsNetworkNamePresent
                     ? &sDataset.mNetworkName
                     : reinterpret_cast<const otNetworkName *>(otThreadGetNetworkName(mInterpreter.mInstance))),
                (sDataset.mComponents.mIsExtendedPanIdPresent ? &sDataset.mExtendedPanId
                                                              : otThreadGetExtendedPanId(mInterpreter.mInstance)),
                &sDataset.mPskc));
    }
#endif
    else
    {
        ExitNow(error = OT_ERROR_INVALID_ARGS);
    }

    sDataset.mComponents.mIsPskcPresent = true;

exit:
    return error;
}

otError Dataset::ProcessSecurityPolicy(uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_NONE;
    long    value;

    VerifyOrExit(aArgsLength > 0, error = OT_ERROR_INVALID_ARGS);

    SuccessOrExit(error = Interpreter::ParseLong(aArgs[0], value));
    sDataset.mSecurityPolicy.mRotationTime = static_cast<uint16_t>(value);
    sDataset.mSecurityPolicy.mFlags        = 0;

    if (aArgsLength > 1)
    {
        for (char *arg = aArgs[1]; *arg != '\0'; arg++)
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
                ExitNow(error = OT_ERROR_INVALID_ARGS);
            }
        }
    }

    sDataset.mComponents.mIsSecurityPolicyPresent = true;

exit:
    return error;
}

otError Dataset::ProcessSet(uint8_t aArgsLength, char *aArgs[])
{
    otError                  error = OT_ERROR_NONE;
    otOperationalDatasetTlvs dataset;

    VerifyOrExit(aArgsLength == 2, error = OT_ERROR_INVALID_ARGS);

    if (strcmp(aArgs[0], "active") == 0)
    {
        dataset.mLength = static_cast<uint8_t>(Interpreter::Hex2Bin(aArgs[1], dataset.mTlvs, sizeof(dataset.mTlvs)));
        SuccessOrExit(error = otDatasetSetActiveTlvs(mInterpreter.mInstance, &dataset));
    }
    else if (strcmp(aArgs[0], "pending") == 0)
    {
        dataset.mLength = static_cast<uint8_t>(Interpreter::Hex2Bin(aArgs[1], dataset.mTlvs, sizeof(dataset.mTlvs)));
        SuccessOrExit(error = otDatasetSetPendingTlvs(mInterpreter.mInstance, &dataset));
    }
    else
    {
        ExitNow(error = OT_ERROR_INVALID_ARGS);
    }

exit:
    return error;
}

} // namespace Cli
} // namespace ot
