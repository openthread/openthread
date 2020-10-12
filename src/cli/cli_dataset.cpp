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
#include "utils/parse_cmdline.hpp"

using ot::Utils::CmdLineParser::ParseAsHexString;
using ot::Utils::CmdLineParser::ParseAsIp6Address;
using ot::Utils::CmdLineParser::ParseAsUint16;
using ot::Utils::CmdLineParser::ParseAsUint32;
using ot::Utils::CmdLineParser::ParseAsUint64;
using ot::Utils::CmdLineParser::ParseAsUint8;

namespace ot {
namespace Cli {

constexpr Dataset::Command Dataset::sCommands[];
otOperationalDataset       Dataset::sDataset;

otError Dataset::Print(otOperationalDataset &aDataset)
{
    if (aDataset.mComponents.mIsPendingTimestampPresent)
    {
        mInterpreter.OutputLine("Pending Timestamp: %lu", aDataset.mPendingTimestamp);
    }

    if (aDataset.mComponents.mIsActiveTimestampPresent)
    {
        mInterpreter.OutputLine("Active Timestamp: %lu", aDataset.mActiveTimestamp);
    }

    if (aDataset.mComponents.mIsChannelPresent)
    {
        mInterpreter.OutputLine("Channel: %d", aDataset.mChannel);
    }

    if (aDataset.mComponents.mIsChannelMaskPresent)
    {
        mInterpreter.OutputLine("Channel Mask: 0x%08x", aDataset.mChannelMask);
    }

    if (aDataset.mComponents.mIsDelayPresent)
    {
        mInterpreter.OutputLine("Delay: %d", aDataset.mDelay);
    }

    if (aDataset.mComponents.mIsExtendedPanIdPresent)
    {
        mInterpreter.OutputFormat("Ext PAN ID: ");
        mInterpreter.OutputBytes(aDataset.mExtendedPanId.m8);
        mInterpreter.OutputLine("");
    }

    if (aDataset.mComponents.mIsMeshLocalPrefixPresent)
    {
        const uint8_t *prefix = aDataset.mMeshLocalPrefix.m8;
        mInterpreter.OutputLine(
            "Mesh Local Prefix: %x:%x:%x:%x::/64", (static_cast<uint16_t>(prefix[0]) << 8) | prefix[1],
            (static_cast<uint16_t>(prefix[2]) << 8) | prefix[3], (static_cast<uint16_t>(prefix[4]) << 8) | prefix[5],
            (static_cast<uint16_t>(prefix[6]) << 8) | prefix[7]);
    }

    if (aDataset.mComponents.mIsMasterKeyPresent)
    {
        mInterpreter.OutputFormat("Master Key: ");
        mInterpreter.OutputBytes(aDataset.mMasterKey.m8);
        mInterpreter.OutputLine("");
    }

    if (aDataset.mComponents.mIsNetworkNamePresent)
    {
        mInterpreter.OutputFormat("Network Name: ");
        mInterpreter.OutputLine("%.*s", static_cast<uint16_t>(sizeof(aDataset.mNetworkName)), aDataset.mNetworkName.m8);
    }

    if (aDataset.mComponents.mIsPanIdPresent)
    {
        mInterpreter.OutputLine("PAN ID: 0x%04x", aDataset.mPanId);
    }

    if (aDataset.mComponents.mIsPskcPresent)
    {
        mInterpreter.OutputFormat("PSKc: ");
        mInterpreter.OutputBytes(aDataset.mPskc.m8);
        mInterpreter.OutputLine("");
    }

    if (aDataset.mComponents.mIsSecurityPolicyPresent)
    {
        mInterpreter.OutputFormat("Security Policy: %d, ", aDataset.mSecurityPolicy.mRotationTime);

        if (aDataset.mSecurityPolicy.mFlags & OT_SECURITY_POLICY_OBTAIN_MASTER_KEY)
        {
            mInterpreter.OutputFormat("o");
        }

        if (aDataset.mSecurityPolicy.mFlags & OT_SECURITY_POLICY_NATIVE_COMMISSIONING)
        {
            mInterpreter.OutputFormat("n");
        }

        if (aDataset.mSecurityPolicy.mFlags & OT_SECURITY_POLICY_ROUTERS)
        {
            mInterpreter.OutputFormat("r");
        }

        if (aDataset.mSecurityPolicy.mFlags & OT_SECURITY_POLICY_EXTERNAL_COMMISSIONER)
        {
            mInterpreter.OutputFormat("c");
        }

        if (aDataset.mSecurityPolicy.mFlags & OT_SECURITY_POLICY_BEACONS)
        {
            mInterpreter.OutputFormat("b");
        }

        mInterpreter.OutputLine("");
    }

    return OT_ERROR_NONE;
}

otError Dataset::Process(uint8_t aArgsLength, char *aArgs[])
{
    otError        error = OT_ERROR_INVALID_COMMAND;
    const Command *command;

    if (aArgsLength == 0)
    {
        ExitNow(error = Print(sDataset));
    }

    command = Utils::LookupTable::Find(aArgs[0], sCommands);
    VerifyOrExit(command != nullptr, OT_NOOP);

    error = (this->*command->mHandler)(aArgsLength - 1, aArgs + 1);

exit:
    return error;
}

otError Dataset::ProcessHelp(uint8_t aArgsLength, char *aArgs[])
{
    OT_UNUSED_VARIABLE(aArgsLength);
    OT_UNUSED_VARIABLE(aArgs);

    for (const Command &command : sCommands)
    {
        mInterpreter.OutputLine(command.mName);
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
    else if ((aArgsLength == 1) && (strcmp(aArgs[0], "-x") == 0))
    {
        otOperationalDatasetTlvs dataset;

        VerifyOrExit(strlen(aArgs[0]) <= OT_OPERATIONAL_DATASET_MAX_LENGTH * 2, error = OT_ERROR_NO_BUFS);

        SuccessOrExit(error = otDatasetGetActiveTlvs(mInterpreter.mInstance, &dataset));
        mInterpreter.OutputBytes(dataset.mTlvs, dataset.mLength);
        mInterpreter.OutputLine("");
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
    else if ((aArgsLength == 1) && (strcmp(aArgs[0], "-x") == 0))
    {
        otOperationalDatasetTlvs dataset;

        VerifyOrExit(strlen(aArgs[0]) <= OT_OPERATIONAL_DATASET_MAX_LENGTH * 2, error = OT_ERROR_NO_BUFS);

        SuccessOrExit(error = otDatasetGetPendingTlvs(mInterpreter.mInstance, &dataset));
        mInterpreter.OutputBytes(dataset.mTlvs, dataset.mLength);
        mInterpreter.OutputLine("");
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

    if (aArgsLength == 0)
    {
        if (sDataset.mComponents.mIsActiveTimestampPresent)
        {
            mInterpreter.OutputLine("%lu", sDataset.mActiveTimestamp);
        }
    }
    else
    {
        SuccessOrExit(error = ParseAsUint64(aArgs[0], sDataset.mActiveTimestamp));
        sDataset.mComponents.mIsActiveTimestampPresent = true;
    }

exit:
    return error;
}

otError Dataset::ProcessChannel(uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_NONE;

    if (aArgsLength == 0)
    {
        if (sDataset.mComponents.mIsChannelPresent)
        {
            mInterpreter.OutputLine("%d", sDataset.mChannel);
        }
    }
    else
    {
        SuccessOrExit(error = ParseAsUint16(aArgs[0], sDataset.mChannel));
        sDataset.mComponents.mIsChannelPresent = true;
    }

exit:
    return error;
}

otError Dataset::ProcessChannelMask(uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_NONE;

    if (aArgsLength == 0)
    {
        if (sDataset.mComponents.mIsChannelMaskPresent)
        {
            mInterpreter.OutputLine("0x%08x", sDataset.mChannelMask);
        }
    }
    else
    {
        SuccessOrExit(error = ParseAsUint32(aArgs[0], sDataset.mChannelMask));
        sDataset.mComponents.mIsChannelMaskPresent = true;
    }

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

    if (aArgsLength == 0)
    {
        if (sDataset.mComponents.mIsDelayPresent)
        {
            mInterpreter.OutputLine("%d", sDataset.mDelay);
        }
    }
    else
    {
        SuccessOrExit(error = ParseAsUint32(aArgs[0], sDataset.mDelay));
        sDataset.mComponents.mIsDelayPresent = true;
    }

exit:
    return error;
}

otError Dataset::ProcessExtPanId(uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_NONE;

    if (aArgsLength == 0)
    {
        if (sDataset.mComponents.mIsExtendedPanIdPresent)
        {
            mInterpreter.OutputBytes(sDataset.mExtendedPanId.m8);
            mInterpreter.OutputLine("");
        }
    }
    else
    {
        SuccessOrExit(error = ParseAsHexString(aArgs[0], sDataset.mExtendedPanId.m8));
        sDataset.mComponents.mIsExtendedPanIdPresent = true;
    }

exit:
    return error;
}

otError Dataset::ProcessMasterKey(uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_NONE;

    if (aArgsLength == 0)
    {
        if (sDataset.mComponents.mIsMasterKeyPresent)
        {
            mInterpreter.OutputBytes(sDataset.mMasterKey.m8);
            mInterpreter.OutputLine("");
        }
    }
    else
    {
        SuccessOrExit(error = ParseAsHexString(aArgs[0], sDataset.mMasterKey.m8));
        sDataset.mComponents.mIsMasterKeyPresent = true;
    }

exit:
    return error;
}

otError Dataset::ProcessMeshLocalPrefix(uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_NONE;

    if (aArgsLength == 0)
    {
        if (sDataset.mComponents.mIsMeshLocalPrefixPresent)
        {
            const uint8_t *prefix = sDataset.mMeshLocalPrefix.m8;
            mInterpreter.OutputLine("Mesh Local Prefix: %x:%x:%x:%x::/64",
                                    (static_cast<uint16_t>(prefix[0]) << 8) | prefix[1],
                                    (static_cast<uint16_t>(prefix[2]) << 8) | prefix[3],
                                    (static_cast<uint16_t>(prefix[4]) << 8) | prefix[5],
                                    (static_cast<uint16_t>(prefix[6]) << 8) | prefix[7]);
        }
    }
    else
    {
        otIp6Address prefix;

        SuccessOrExit(error = ParseAsIp6Address(aArgs[0], prefix));

        memcpy(sDataset.mMeshLocalPrefix.m8, prefix.mFields.m8, sizeof(sDataset.mMeshLocalPrefix.m8));
        sDataset.mComponents.mIsMeshLocalPrefixPresent = true;
    }

exit:
    return error;
}

otError Dataset::ProcessNetworkName(uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_NONE;

    if (aArgsLength == 0)
    {
        if (sDataset.mComponents.mIsNetworkNamePresent)
        {
            mInterpreter.OutputLine("%.*s", static_cast<uint16_t>(sizeof(sDataset.mNetworkName)),
                                    sDataset.mNetworkName.m8);
        }
    }
    else
    {
        size_t length;

        VerifyOrExit((length = strlen(aArgs[0])) <= OT_NETWORK_NAME_MAX_SIZE, error = OT_ERROR_INVALID_ARGS);

        memset(&sDataset.mNetworkName, 0, sizeof(sDataset.mNetworkName));
        memcpy(sDataset.mNetworkName.m8, aArgs[0], length);
        sDataset.mComponents.mIsNetworkNamePresent = true;
    }

exit:
    return error;
}

otError Dataset::ProcessPanId(uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_NONE;

    if (aArgsLength == 0)
    {
        if (sDataset.mComponents.mIsPanIdPresent)
        {
            mInterpreter.OutputLine("0x%04x", sDataset.mPanId);
        }
    }
    else
    {
        SuccessOrExit(error = ParseAsUint16(aArgs[0], sDataset.mPanId));
        sDataset.mComponents.mIsPanIdPresent = true;
    }

exit:
    return error;
}

otError Dataset::ProcessPendingTimestamp(uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_NONE;

    if (aArgsLength == 0)
    {
        if (sDataset.mComponents.mIsPendingTimestampPresent)
        {
            mInterpreter.OutputLine("%lu", sDataset.mPendingTimestamp);
        }
    }
    else
    {
        SuccessOrExit(error = ParseAsUint64(aArgs[0], sDataset.mPendingTimestamp));
        sDataset.mComponents.mIsPendingTimestampPresent = true;
    }

exit:
    return error;
}

otError Dataset::ProcessMgmtSetCommand(uint8_t aArgsLength, char *aArgs[])
{
    otError              error = OT_ERROR_NONE;
    otOperationalDataset dataset;
    uint8_t              tlvs[128];
    uint8_t              tlvsLength = 0;

    VerifyOrExit(aArgsLength > 0, error = OT_ERROR_INVALID_ARGS);

    memset(&dataset, 0, sizeof(dataset));

    for (uint8_t index = 1; index < aArgsLength; index++)
    {
        if (strcmp(aArgs[index], "activetimestamp") == 0)
        {
            VerifyOrExit(++index < aArgsLength, error = OT_ERROR_INVALID_ARGS);
            dataset.mComponents.mIsActiveTimestampPresent = true;
            SuccessOrExit(error = ParseAsUint64(aArgs[index], dataset.mActiveTimestamp));
        }
        else if (strcmp(aArgs[index], "pendingtimestamp") == 0)
        {
            VerifyOrExit(++index < aArgsLength, error = OT_ERROR_INVALID_ARGS);
            dataset.mComponents.mIsPendingTimestampPresent = true;
            SuccessOrExit(error = ParseAsUint64(aArgs[index], dataset.mPendingTimestamp));
        }
        else if (strcmp(aArgs[index], "masterkey") == 0)
        {
            VerifyOrExit(++index < aArgsLength, error = OT_ERROR_INVALID_ARGS);
            dataset.mComponents.mIsMasterKeyPresent = true;
            SuccessOrExit(error = ParseAsHexString(aArgs[index], dataset.mMasterKey.m8));
        }
        else if (strcmp(aArgs[index], "networkname") == 0)
        {
            size_t length;

            VerifyOrExit(++index < aArgsLength, error = OT_ERROR_INVALID_ARGS);
            dataset.mComponents.mIsNetworkNamePresent = true;
            VerifyOrExit((length = strlen(aArgs[index])) <= OT_NETWORK_NAME_MAX_SIZE, error = OT_ERROR_INVALID_ARGS);
            memset(&dataset.mNetworkName, 0, sizeof(sDataset.mNetworkName));
            memcpy(dataset.mNetworkName.m8, aArgs[index], length);
        }
        else if (strcmp(aArgs[index], "extpanid") == 0)
        {
            VerifyOrExit(++index < aArgsLength, error = OT_ERROR_INVALID_ARGS);
            dataset.mComponents.mIsExtendedPanIdPresent = true;
            SuccessOrExit(error = ParseAsHexString(aArgs[index], dataset.mExtendedPanId.m8));
        }
        else if (strcmp(aArgs[index], "localprefix") == 0)
        {
            otIp6Address prefix;

            VerifyOrExit(++index < aArgsLength, error = OT_ERROR_INVALID_ARGS);
            dataset.mComponents.mIsMeshLocalPrefixPresent = true;
            SuccessOrExit(error = ParseAsIp6Address(aArgs[index], prefix));
            memcpy(dataset.mMeshLocalPrefix.m8, prefix.mFields.m8, sizeof(dataset.mMeshLocalPrefix.m8));
        }
        else if (strcmp(aArgs[index], "delaytimer") == 0)
        {
            VerifyOrExit(++index < aArgsLength, error = OT_ERROR_INVALID_ARGS);
            dataset.mComponents.mIsDelayPresent = true;
            SuccessOrExit(error = ParseAsUint32(aArgs[index], dataset.mDelay));
        }
        else if (strcmp(aArgs[index], "panid") == 0)
        {
            VerifyOrExit(++index < aArgsLength, error = OT_ERROR_INVALID_ARGS);
            dataset.mComponents.mIsPanIdPresent = true;
            SuccessOrExit(error = ParseAsUint16(aArgs[index], dataset.mPanId));
        }
        else if (strcmp(aArgs[index], "channel") == 0)
        {
            VerifyOrExit(++index < aArgsLength, error = OT_ERROR_INVALID_ARGS);
            dataset.mComponents.mIsChannelPresent = true;
            SuccessOrExit(error = ParseAsUint16(aArgs[index], dataset.mChannel));
        }
        else if (strcmp(aArgs[index], "channelmask") == 0)
        {
            VerifyOrExit(++index < aArgsLength, error = OT_ERROR_INVALID_ARGS);
            dataset.mComponents.mIsChannelMaskPresent = true;
            SuccessOrExit(error = ParseAsUint32(aArgs[index], dataset.mChannelMask));
        }
        else if (strcmp(aArgs[index], "-x") == 0)
        {
            uint16_t length;

            VerifyOrExit(++index < aArgsLength, error = OT_ERROR_INVALID_ARGS);
            length = sizeof(tlvs);
            SuccessOrExit(error = ParseAsHexString(aArgs[index], length, tlvs));
            tlvsLength = static_cast<uint8_t>(length);
        }
        else
        {
            ExitNow(error = OT_ERROR_INVALID_ARGS);
        }
    }

    if (strcmp(aArgs[0], "active") == 0)
    {
        SuccessOrExit(error = otDatasetSendMgmtActiveSet(mInterpreter.mInstance, &dataset, tlvs, tlvsLength));
    }
    else if (strcmp(aArgs[0], "pending") == 0)
    {
        SuccessOrExit(error = otDatasetSendMgmtPendingSet(mInterpreter.mInstance, &dataset, tlvs, tlvsLength));
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
    uint8_t                        tlvsLength        = 0;
    bool                           destAddrSpecified = false;
    otIp6Address                   address;

    VerifyOrExit(aArgsLength > 0, error = OT_ERROR_INVALID_ARGS);

    memset(&datasetComponents, 0, sizeof(datasetComponents));

    for (uint8_t index = 1; index < aArgsLength; index++)
    {
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
        else if (strcmp(aArgs[index], "-x") == 0)
        {
            uint16_t length;

            VerifyOrExit(++index < aArgsLength, error = OT_ERROR_INVALID_ARGS);
            length = sizeof(tlvs);
            SuccessOrExit(error = ParseAsHexString(aArgs[index], length, tlvs));
            tlvsLength = static_cast<uint8_t>(length);
        }
        else if (strcmp(aArgs[index], "address") == 0)
        {
            VerifyOrExit(++index < aArgsLength, error = OT_ERROR_INVALID_ARGS);
            SuccessOrExit(error = ParseAsIp6Address(aArgs[index], address));
            destAddrSpecified = true;
        }
        else
        {
            ExitNow(error = OT_ERROR_INVALID_ARGS);
        }
    }

    if (strcmp(aArgs[0], "active") == 0)
    {
        SuccessOrExit(error = otDatasetSendMgmtActiveGet(mInterpreter.mInstance, &datasetComponents, tlvs, tlvsLength,
                                                         destAddrSpecified ? &address : nullptr));
    }
    else if (strcmp(aArgs[0], "pending") == 0)
    {
        SuccessOrExit(error = otDatasetSendMgmtPendingGet(mInterpreter.mInstance, &datasetComponents, tlvs, tlvsLength,
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

    if (aArgsLength == 0)
    {
        if (sDataset.mComponents.mIsPskcPresent)
        {
            mInterpreter.OutputBytes(sDataset.mPskc.m8);
            mInterpreter.OutputLine("");
        }
    }
    else if (aArgsLength == 1)
    {
        SuccessOrExit(error = ParseAsHexString(aArgs[0], sDataset.mPskc.m8));
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

    if (aArgsLength == 0)
    {
        if (sDataset.mComponents.mIsSecurityPolicyPresent)
        {
            mInterpreter.OutputFormat("%d ", sDataset.mSecurityPolicy.mRotationTime);

            if (sDataset.mSecurityPolicy.mFlags & OT_SECURITY_POLICY_OBTAIN_MASTER_KEY)
            {
                mInterpreter.OutputFormat("o");
            }

            if (sDataset.mSecurityPolicy.mFlags & OT_SECURITY_POLICY_NATIVE_COMMISSIONING)
            {
                mInterpreter.OutputFormat("n");
            }

            if (sDataset.mSecurityPolicy.mFlags & OT_SECURITY_POLICY_ROUTERS)
            {
                mInterpreter.OutputFormat("r");
            }

            if (sDataset.mSecurityPolicy.mFlags & OT_SECURITY_POLICY_EXTERNAL_COMMISSIONER)
            {
                mInterpreter.OutputFormat("c");
            }

            if (sDataset.mSecurityPolicy.mFlags & OT_SECURITY_POLICY_BEACONS)
            {
                mInterpreter.OutputFormat("b");
            }

            mInterpreter.OutputLine("");
        }
    }
    else
    {
        SuccessOrExit(error = ParseAsUint16(aArgs[0], sDataset.mSecurityPolicy.mRotationTime));
        sDataset.mSecurityPolicy.mFlags = 0;

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
    }

exit:
    return error;
}

otError Dataset::ProcessSet(uint8_t aArgsLength, char *aArgs[])
{
    otError                  error = OT_ERROR_NONE;
    otOperationalDatasetTlvs dataset;
    uint16_t                 tlvsLength;

    VerifyOrExit(aArgsLength == 2, error = OT_ERROR_INVALID_ARGS);

    tlvsLength = sizeof(dataset.mTlvs);
    SuccessOrExit(error = ParseAsHexString(aArgs[1], tlvsLength, dataset.mTlvs));
    dataset.mLength = static_cast<uint8_t>(tlvsLength);

    if (strcmp(aArgs[0], "active") == 0)
    {
        SuccessOrExit(error = otDatasetSetActiveTlvs(mInterpreter.mInstance, &dataset));
    }
    else if (strcmp(aArgs[0], "pending") == 0)
    {
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
