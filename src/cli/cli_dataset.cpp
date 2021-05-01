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
#include <openthread/dataset_updater.h>

#include "cli/cli.hpp"
#include "common/string.hpp"

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
        mInterpreter.OutputFormat("Security Policy: ", aDataset.mSecurityPolicy.mRotationTime);
        OutputSecurityPolicy(aDataset.mSecurityPolicy);
        mInterpreter.OutputLine("");
    }

    return OT_ERROR_NONE;
}

otError Dataset::Process(uint8_t aArgsLength, Arg aArgs[])
{
    otError        error = OT_ERROR_INVALID_COMMAND;
    const Command *command;

    if (aArgsLength == 0)
    {
        ExitNow(error = Print(sDataset));
    }

    command = Utils::LookupTable::Find(aArgs[0].GetCString(), sCommands);
    VerifyOrExit(command != nullptr);

    error = (this->*command->mHandler)(aArgsLength - 1, aArgs + 1);

exit:
    return error;
}

otError Dataset::ProcessHelp(uint8_t aArgsLength, Arg aArgs[])
{
    OT_UNUSED_VARIABLE(aArgsLength);
    OT_UNUSED_VARIABLE(aArgs);

    for (const Command &command : sCommands)
    {
        mInterpreter.OutputLine(command.mName);
    }

    return OT_ERROR_NONE;
}

otError Dataset::ProcessInit(uint8_t aArgsLength, Arg aArgs[])
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(aArgsLength > 0, error = OT_ERROR_INVALID_ARGS);

    if (aArgs[0] == "active")
    {
        SuccessOrExit(error = otDatasetGetActive(mInterpreter.mInstance, &sDataset));
    }
    else if (aArgs[0] == "pending")
    {
        SuccessOrExit(error = otDatasetGetPending(mInterpreter.mInstance, &sDataset));
    }
#if OPENTHREAD_FTD
    else if (aArgs[0] == "new")
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

otError Dataset::ProcessActive(uint8_t aArgsLength, Arg aArgs[])
{
    otError error;

    if (aArgsLength == 0)
    {
        otOperationalDataset dataset;

        SuccessOrExit(error = otDatasetGetActive(mInterpreter.mInstance, &dataset));
        error = Print(dataset);
    }
    else if ((aArgsLength == 1) && (aArgs[0] == "-x"))
    {
        otOperationalDatasetTlvs dataset;

        VerifyOrExit(aArgs[0].GetLength() <= OT_OPERATIONAL_DATASET_MAX_LENGTH * 2, error = OT_ERROR_NO_BUFS);

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

otError Dataset::ProcessPending(uint8_t aArgsLength, Arg aArgs[])
{
    otError error;

    if (aArgsLength == 0)
    {
        otOperationalDataset dataset;

        SuccessOrExit(error = otDatasetGetPending(mInterpreter.mInstance, &dataset));
        error = Print(dataset);
    }
    else if ((aArgsLength == 1) && (aArgs[0] == "-x"))
    {
        otOperationalDatasetTlvs dataset;

        VerifyOrExit(aArgs[0].GetLength() <= OT_OPERATIONAL_DATASET_MAX_LENGTH * 2, error = OT_ERROR_NO_BUFS);

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

otError Dataset::ProcessActiveTimestamp(uint8_t aArgsLength, Arg aArgs[])
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
        SuccessOrExit(error = aArgs[0].ParseAsUint64(sDataset.mActiveTimestamp));
        sDataset.mComponents.mIsActiveTimestampPresent = true;
    }

exit:
    return error;
}

otError Dataset::ProcessChannel(uint8_t aArgsLength, Arg aArgs[])
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
        SuccessOrExit(error = aArgs[0].ParseAsUint16(sDataset.mChannel));
        sDataset.mComponents.mIsChannelPresent = true;
    }

exit:
    return error;
}

otError Dataset::ProcessChannelMask(uint8_t aArgsLength, Arg aArgs[])
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
        SuccessOrExit(error = aArgs[0].ParseAsUint32(sDataset.mChannelMask));
        sDataset.mComponents.mIsChannelMaskPresent = true;
    }

exit:
    return error;
}

otError Dataset::ProcessClear(uint8_t aArgsLength, Arg aArgs[])
{
    OT_UNUSED_VARIABLE(aArgsLength);
    OT_UNUSED_VARIABLE(aArgs);

    memset(&sDataset, 0, sizeof(sDataset));
    return OT_ERROR_NONE;
}

otError Dataset::ProcessCommit(uint8_t aArgsLength, Arg aArgs[])
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(aArgsLength > 0, error = OT_ERROR_INVALID_ARGS);

    if (aArgs[0] == "active")
    {
        SuccessOrExit(error = otDatasetSetActive(mInterpreter.mInstance, &sDataset));
    }
    else if (aArgs[0] == "pending")
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

otError Dataset::ProcessDelay(uint8_t aArgsLength, Arg aArgs[])
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
        SuccessOrExit(error = aArgs[0].ParseAsUint32(sDataset.mDelay));
        sDataset.mComponents.mIsDelayPresent = true;
    }

exit:
    return error;
}

otError Dataset::ProcessExtPanId(uint8_t aArgsLength, Arg aArgs[])
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
        SuccessOrExit(error = aArgs[0].ParseAsHexString(sDataset.mExtendedPanId.m8));
        sDataset.mComponents.mIsExtendedPanIdPresent = true;
    }

exit:
    return error;
}

otError Dataset::ProcessMasterKey(uint8_t aArgsLength, Arg aArgs[])
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
        SuccessOrExit(error = aArgs[0].ParseAsHexString(sDataset.mMasterKey.m8));
        sDataset.mComponents.mIsMasterKeyPresent = true;
    }

exit:
    return error;
}

otError Dataset::ProcessMeshLocalPrefix(uint8_t aArgsLength, Arg aArgs[])
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

        SuccessOrExit(error = aArgs[0].ParseAsIp6Address(prefix));

        memcpy(sDataset.mMeshLocalPrefix.m8, prefix.mFields.m8, sizeof(sDataset.mMeshLocalPrefix.m8));
        sDataset.mComponents.mIsMeshLocalPrefixPresent = true;
    }

exit:
    return error;
}

otError Dataset::ProcessNetworkName(uint8_t aArgsLength, Arg aArgs[])
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
        uint16_t length;

        VerifyOrExit((length = aArgs[0].GetLength()) <= OT_NETWORK_NAME_MAX_SIZE, error = OT_ERROR_INVALID_ARGS);
        VerifyOrExit(IsValidUtf8String(aArgs[0].GetCString()), error = OT_ERROR_INVALID_ARGS);

        memset(&sDataset.mNetworkName, 0, sizeof(sDataset.mNetworkName));
        memcpy(sDataset.mNetworkName.m8, aArgs[0].GetCString(), length);
        sDataset.mComponents.mIsNetworkNamePresent = true;
    }

exit:
    return error;
}

otError Dataset::ProcessPanId(uint8_t aArgsLength, Arg aArgs[])
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
        SuccessOrExit(error = aArgs[0].ParseAsUint16(sDataset.mPanId));
        sDataset.mComponents.mIsPanIdPresent = true;
    }

exit:
    return error;
}

otError Dataset::ProcessPendingTimestamp(uint8_t aArgsLength, Arg aArgs[])
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
        SuccessOrExit(error = aArgs[0].ParseAsUint64(sDataset.mPendingTimestamp));
        sDataset.mComponents.mIsPendingTimestampPresent = true;
    }

exit:
    return error;
}

otError Dataset::ProcessMgmtSetCommand(uint8_t aArgsLength, Arg aArgs[])
{
    otError              error = OT_ERROR_NONE;
    otOperationalDataset dataset;
    uint8_t              tlvs[128];
    uint8_t              tlvsLength = 0;

    VerifyOrExit(aArgsLength > 0, error = OT_ERROR_INVALID_ARGS);

    memset(&dataset, 0, sizeof(dataset));

    for (uint8_t index = 1; index < aArgsLength; index++)
    {
        if (aArgs[index] == "activetimestamp")
        {
            VerifyOrExit(++index < aArgsLength, error = OT_ERROR_INVALID_ARGS);
            dataset.mComponents.mIsActiveTimestampPresent = true;
            SuccessOrExit(error = aArgs[index].ParseAsUint64(dataset.mActiveTimestamp));
        }
        else if (aArgs[index] == "pendingtimestamp")
        {
            VerifyOrExit(++index < aArgsLength, error = OT_ERROR_INVALID_ARGS);
            dataset.mComponents.mIsPendingTimestampPresent = true;
            SuccessOrExit(error = aArgs[index].ParseAsUint64(dataset.mPendingTimestamp));
        }
        else if (aArgs[index] == "masterkey")
        {
            VerifyOrExit(++index < aArgsLength, error = OT_ERROR_INVALID_ARGS);
            dataset.mComponents.mIsMasterKeyPresent = true;
            SuccessOrExit(error = aArgs[index].ParseAsHexString(dataset.mMasterKey.m8));
        }
        else if (aArgs[index] == "networkname")
        {
            uint16_t length;

            VerifyOrExit(++index < aArgsLength, error = OT_ERROR_INVALID_ARGS);
            dataset.mComponents.mIsNetworkNamePresent = true;
            VerifyOrExit((length = aArgs[index].GetLength()) <= OT_NETWORK_NAME_MAX_SIZE,
                         error = OT_ERROR_INVALID_ARGS);
            memset(&dataset.mNetworkName, 0, sizeof(sDataset.mNetworkName));
            memcpy(dataset.mNetworkName.m8, aArgs[index].GetCString(), length);
        }
        else if (aArgs[index] == "extpanid")
        {
            VerifyOrExit(++index < aArgsLength, error = OT_ERROR_INVALID_ARGS);
            dataset.mComponents.mIsExtendedPanIdPresent = true;
            SuccessOrExit(error = aArgs[index].ParseAsHexString(dataset.mExtendedPanId.m8));
        }
        else if (aArgs[index] == "localprefix")
        {
            otIp6Address prefix;

            VerifyOrExit(++index < aArgsLength, error = OT_ERROR_INVALID_ARGS);
            dataset.mComponents.mIsMeshLocalPrefixPresent = true;
            SuccessOrExit(error = aArgs[index].ParseAsIp6Address(prefix));
            memcpy(dataset.mMeshLocalPrefix.m8, prefix.mFields.m8, sizeof(dataset.mMeshLocalPrefix.m8));
        }
        else if (aArgs[index] == "delaytimer")
        {
            VerifyOrExit(++index < aArgsLength, error = OT_ERROR_INVALID_ARGS);
            dataset.mComponents.mIsDelayPresent = true;
            SuccessOrExit(error = aArgs[index].ParseAsUint32(dataset.mDelay));
        }
        else if (aArgs[index] == "panid")
        {
            VerifyOrExit(++index < aArgsLength, error = OT_ERROR_INVALID_ARGS);
            dataset.mComponents.mIsPanIdPresent = true;
            SuccessOrExit(error = aArgs[index].ParseAsUint16(dataset.mPanId));
        }
        else if (aArgs[index] == "channel")
        {
            VerifyOrExit(++index < aArgsLength, error = OT_ERROR_INVALID_ARGS);
            dataset.mComponents.mIsChannelPresent = true;
            SuccessOrExit(error = aArgs[index].ParseAsUint16(dataset.mChannel));
        }
        else if (aArgs[index] == "channelmask")
        {
            VerifyOrExit(++index < aArgsLength, error = OT_ERROR_INVALID_ARGS);
            dataset.mComponents.mIsChannelMaskPresent = true;
            SuccessOrExit(error = aArgs[index].ParseAsUint32(dataset.mChannelMask));
        }
        else if (aArgs[index] == "securitypolicy")
        {
            VerifyOrExit(++index < aArgsLength, error = OT_ERROR_INVALID_ARGS);
            SuccessOrExit(error = ParseSecurityPolicy(dataset.mSecurityPolicy, aArgsLength - index, &aArgs[index]));
            dataset.mComponents.mIsSecurityPolicyPresent = true;
            ++index;
        }
        else if (aArgs[index] == "-x")
        {
            uint16_t length;

            VerifyOrExit(++index < aArgsLength, error = OT_ERROR_INVALID_ARGS);
            length = sizeof(tlvs);
            SuccessOrExit(error = aArgs[index].ParseAsHexString(length, tlvs));
            tlvsLength = static_cast<uint8_t>(length);
        }
        else
        {
            ExitNow(error = OT_ERROR_INVALID_ARGS);
        }
    }

    if (aArgs[0] == "active")
    {
        SuccessOrExit(error = otDatasetSendMgmtActiveSet(mInterpreter.mInstance, &dataset, tlvs, tlvsLength));
    }
    else if (aArgs[0] == "pending")
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

otError Dataset::ProcessMgmtGetCommand(uint8_t aArgsLength, Arg aArgs[])
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
        if (aArgs[index] == "activetimestamp")
        {
            datasetComponents.mIsActiveTimestampPresent = true;
        }
        else if (aArgs[index] == "pendingtimestamp")
        {
            datasetComponents.mIsPendingTimestampPresent = true;
        }
        else if (aArgs[index] == "masterkey")
        {
            datasetComponents.mIsMasterKeyPresent = true;
        }
        else if (aArgs[index] == "networkname")
        {
            datasetComponents.mIsNetworkNamePresent = true;
        }
        else if (aArgs[index] == "extpanid")
        {
            datasetComponents.mIsExtendedPanIdPresent = true;
        }
        else if (aArgs[index] == "localprefix")
        {
            datasetComponents.mIsMeshLocalPrefixPresent = true;
        }
        else if (aArgs[index] == "delaytimer")
        {
            datasetComponents.mIsDelayPresent = true;
        }
        else if (aArgs[index] == "panid")
        {
            datasetComponents.mIsPanIdPresent = true;
        }
        else if (aArgs[index] == "channel")
        {
            datasetComponents.mIsChannelPresent = true;
        }
        else if (aArgs[index] == "securitypolicy")
        {
            datasetComponents.mIsSecurityPolicyPresent = true;
        }
        else if (aArgs[index] == "-x")
        {
            uint16_t length;

            VerifyOrExit(++index < aArgsLength, error = OT_ERROR_INVALID_ARGS);
            length = sizeof(tlvs);
            SuccessOrExit(error = aArgs[index].ParseAsHexString(length, tlvs));
            tlvsLength = static_cast<uint8_t>(length);
        }
        else if (aArgs[index] == "address")
        {
            VerifyOrExit(++index < aArgsLength, error = OT_ERROR_INVALID_ARGS);
            SuccessOrExit(error = aArgs[index].ParseAsIp6Address(address));
            destAddrSpecified = true;
        }
        else
        {
            ExitNow(error = OT_ERROR_INVALID_ARGS);
        }
    }

    if (aArgs[0] == "active")
    {
        SuccessOrExit(error = otDatasetSendMgmtActiveGet(mInterpreter.mInstance, &datasetComponents, tlvs, tlvsLength,
                                                         destAddrSpecified ? &address : nullptr));
    }
    else if (aArgs[0] == "pending")
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

otError Dataset::ProcessPskc(uint8_t aArgsLength, Arg aArgs[])
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
        SuccessOrExit(error = aArgs[0].ParseAsHexString(sDataset.mPskc.m8));
    }
#if OPENTHREAD_FTD
    else if (aArgsLength == 2 && (aArgs[0] == "-p"))
    {
        SuccessOrExit(
            error = otDatasetGeneratePskc(
                aArgs[1].GetCString(),
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

void Dataset::OutputSecurityPolicy(const otSecurityPolicy &aSecurityPolicy)
{
    mInterpreter.OutputFormat("%d ", aSecurityPolicy.mRotationTime);

    if (aSecurityPolicy.mObtainMasterKeyEnabled)
    {
        mInterpreter.OutputFormat("o");
    }

    if (aSecurityPolicy.mNativeCommissioningEnabled)
    {
        mInterpreter.OutputFormat("n");
    }

    if (aSecurityPolicy.mRoutersEnabled)
    {
        mInterpreter.OutputFormat("r");
    }

    if (aSecurityPolicy.mExternalCommissioningEnabled)
    {
        mInterpreter.OutputFormat("c");
    }

    if (aSecurityPolicy.mBeaconsEnabled)
    {
        mInterpreter.OutputFormat("b");
    }

    if (aSecurityPolicy.mCommercialCommissioningEnabled)
    {
        mInterpreter.OutputFormat("C");
    }

    if (aSecurityPolicy.mAutonomousEnrollmentEnabled)
    {
        mInterpreter.OutputFormat("e");
    }

    if (aSecurityPolicy.mMasterKeyProvisioningEnabled)
    {
        mInterpreter.OutputFormat("p");
    }

    if (aSecurityPolicy.mNonCcmRoutersEnabled)
    {
        mInterpreter.OutputFormat("R");
    }
}

Error Dataset::ParseSecurityPolicy(otSecurityPolicy &aSecurityPolicy, uint8_t aArgsLength, Arg aArgs[])
{
    Error            error;
    otSecurityPolicy policy;

    memset(&policy, 0, sizeof(policy));
    SuccessOrExit(error = aArgs[0].ParseAsUint16(policy.mRotationTime));

    VerifyOrExit(aArgsLength >= 2);

    for (const char *flag = aArgs[1].GetCString(); *flag != '\0'; flag++)
    {
        switch (*flag)
        {
        case 'o':
            policy.mObtainMasterKeyEnabled = true;
            break;

        case 'n':
            policy.mNativeCommissioningEnabled = true;
            break;

        case 'r':
            policy.mRoutersEnabled = true;
            break;

        case 'c':
            policy.mExternalCommissioningEnabled = true;
            break;

        case 'b':
            policy.mBeaconsEnabled = true;
            break;

        case 'C':
            policy.mCommercialCommissioningEnabled = true;
            break;

        case 'e':
            policy.mAutonomousEnrollmentEnabled = true;
            break;

        case 'p':
            policy.mMasterKeyProvisioningEnabled = true;
            break;

        case 'R':
            policy.mNonCcmRoutersEnabled = true;
            break;

        default:
            ExitNow(error = OT_ERROR_INVALID_ARGS);
        }
    }

exit:
    if (error == kErrorNone)
    {
        aSecurityPolicy = policy;
    }
    return error;
}

otError Dataset::ProcessSecurityPolicy(uint8_t aArgsLength, Arg aArgs[])
{
    otError error = OT_ERROR_NONE;

    if (aArgsLength == 0)
    {
        if (sDataset.mComponents.mIsSecurityPolicyPresent)
        {
            OutputSecurityPolicy(sDataset.mSecurityPolicy);
            mInterpreter.OutputLine("");
        }
    }
    else
    {
        SuccessOrExit(error = ParseSecurityPolicy(sDataset.mSecurityPolicy, aArgsLength, aArgs));
        sDataset.mComponents.mIsSecurityPolicyPresent = true;
    }

exit:
    return error;
}

otError Dataset::ProcessSet(uint8_t aArgsLength, Arg aArgs[])
{
    otError                error = OT_ERROR_NONE;
    MeshCoP::Dataset::Type datasetType;

    VerifyOrExit(aArgsLength == 2, error = OT_ERROR_INVALID_ARGS);

    if (aArgs[0] == "active")
    {
        datasetType = MeshCoP::Dataset::Type::kActive;
    }
    else if (aArgs[0] == "pending")
    {
        datasetType = MeshCoP::Dataset::Type::kPending;
    }
    else
    {
        ExitNow(error = OT_ERROR_INVALID_ARGS);
    }

    {
        MeshCoP::Dataset       dataset(datasetType);
        MeshCoP::Dataset::Info datasetInfo;
        uint16_t               tlvsLength = MeshCoP::Dataset::kMaxSize;

        SuccessOrExit(error = aArgs[1].ParseAsHexString(tlvsLength, dataset.GetBytes()));
        dataset.SetSize(tlvsLength);
        VerifyOrExit(dataset.IsValid(), error = OT_ERROR_INVALID_ARGS);
        dataset.ConvertTo(datasetInfo);

        switch (datasetType)
        {
        case MeshCoP::Dataset::Type::kActive:
            SuccessOrExit(error = otDatasetSetActive(mInterpreter.mInstance, &datasetInfo));
            break;
        case MeshCoP::Dataset::Type::kPending:
            SuccessOrExit(error = otDatasetSetPending(mInterpreter.mInstance, &datasetInfo));
            break;
        }
    }

exit:
    return error;
}

#if OPENTHREAD_CONFIG_DATASET_UPDATER_ENABLE && OPENTHREAD_FTD

otError Dataset::ProcessUpdater(uint8_t aArgsLength, Arg aArgs[])
{
    otError error = OT_ERROR_NONE;

    if (aArgsLength == 0)
    {
        mInterpreter.OutputEnabledDisabledStatus(otDatasetUpdaterIsUpdateOngoing(mInterpreter.mInstance));
        ExitNow();
    }

    if (aArgs[0] == "start")
    {
        error = otDatasetUpdaterRequestUpdate(mInterpreter.mInstance, &sDataset, &Dataset::HandleDatasetUpdater, this);
    }
    else if (aArgs[0] == "cancel")
    {
        otDatasetUpdaterCancelUpdate(mInterpreter.mInstance);
    }
    else
    {
        error = OT_ERROR_INVALID_ARGS;
    }

exit:
    return error;
}

void Dataset::HandleDatasetUpdater(otError aError, void *aContext)
{
    static_cast<Dataset *>(aContext)->HandleDatasetUpdater(aError);
}

void Dataset::HandleDatasetUpdater(otError aError)
{
    mInterpreter.OutputLine("Dataset update complete: %s", otThreadErrorToString(aError));
}

#endif // OPENTHREAD_CONFIG_DATASET_UPDATER_ENABLE && OPENTHREAD_FTD

} // namespace Cli
} // namespace ot
