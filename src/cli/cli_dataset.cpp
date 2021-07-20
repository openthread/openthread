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
        mInterpreter.OutputFormat("Mesh Local Prefix: ");
        mInterpreter.OutputPrefix(aDataset.mMeshLocalPrefix);
        mInterpreter.OutputLine("");
    }

    if (aDataset.mComponents.mIsNetworkKeyPresent)
    {
        mInterpreter.OutputFormat("Network Key: ");
        mInterpreter.OutputBytes(aDataset.mNetworkKey.m8);
        mInterpreter.OutputLine("");
    }

    if (aDataset.mComponents.mIsNetworkNamePresent)
    {
        mInterpreter.OutputFormat("Network Name: ");
        mInterpreter.OutputLine("%s", aDataset.mNetworkName.m8);
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
        mInterpreter.OutputFormat("Security Policy: ");
        OutputSecurityPolicy(aDataset.mSecurityPolicy);
        mInterpreter.OutputLine("");
    }

    return OT_ERROR_NONE;
}

otError Dataset::Process(Arg aArgs[])
{
    otError        error = OT_ERROR_INVALID_COMMAND;
    const Command *command;

    if (aArgs[0].IsEmpty())
    {
        ExitNow(error = Print(sDataset));
    }

    command = Utils::LookupTable::Find(aArgs[0].GetCString(), sCommands);
    VerifyOrExit(command != nullptr);

    error = (this->*command->mHandler)(aArgs + 1);

exit:
    return error;
}

otError Dataset::ProcessHelp(Arg aArgs[])
{
    OT_UNUSED_VARIABLE(aArgs);

    for (const Command &command : sCommands)
    {
        mInterpreter.OutputLine(command.mName);
    }

    return OT_ERROR_NONE;
}

otError Dataset::ProcessInit(Arg aArgs[])
{
    otError error = OT_ERROR_INVALID_ARGS;

    if (aArgs[0] == "active")
    {
        error = otDatasetGetActive(mInterpreter.mInstance, &sDataset);
    }
    else if (aArgs[0] == "pending")
    {
        error = otDatasetGetPending(mInterpreter.mInstance, &sDataset);
    }
#if OPENTHREAD_FTD
    else if (aArgs[0] == "new")
    {
        error = otDatasetCreateNewNetwork(mInterpreter.mInstance, &sDataset);
    }
#endif

    return error;
}

otError Dataset::ProcessActive(Arg aArgs[])
{
    otError error = OT_ERROR_INVALID_ARGS;

    if (aArgs[0].IsEmpty())
    {
        otOperationalDataset dataset;

        SuccessOrExit(error = otDatasetGetActive(mInterpreter.mInstance, &dataset));
        error = Print(dataset);
    }
    else if (aArgs[0] == "-x")
    {
        otOperationalDatasetTlvs dataset;

        SuccessOrExit(error = otDatasetGetActiveTlvs(mInterpreter.mInstance, &dataset));
        mInterpreter.OutputBytes(dataset.mTlvs, dataset.mLength);
        mInterpreter.OutputLine("");
    }

exit:
    return error;
}

otError Dataset::ProcessPending(Arg aArgs[])
{
    otError error = OT_ERROR_INVALID_ARGS;

    if (aArgs[0].IsEmpty())
    {
        otOperationalDataset dataset;

        SuccessOrExit(error = otDatasetGetPending(mInterpreter.mInstance, &dataset));
        error = Print(dataset);
    }
    else if (aArgs[0] == "-x")
    {
        otOperationalDatasetTlvs dataset;

        SuccessOrExit(error = otDatasetGetPendingTlvs(mInterpreter.mInstance, &dataset));
        mInterpreter.OutputBytes(dataset.mTlvs, dataset.mLength);
        mInterpreter.OutputLine("");
    }

exit:
    return error;
}

otError Dataset::ProcessActiveTimestamp(Arg aArgs[])
{
    otError error = OT_ERROR_NONE;

    if (aArgs[0].IsEmpty())
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

otError Dataset::ProcessChannel(Arg aArgs[])
{
    otError error = OT_ERROR_NONE;

    if (aArgs[0].IsEmpty())
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

otError Dataset::ProcessChannelMask(Arg aArgs[])
{
    otError error = OT_ERROR_NONE;

    if (aArgs[0].IsEmpty())
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

otError Dataset::ProcessClear(Arg aArgs[])
{
    OT_UNUSED_VARIABLE(aArgs);

    memset(&sDataset, 0, sizeof(sDataset));
    return OT_ERROR_NONE;
}

otError Dataset::ProcessCommit(Arg aArgs[])
{
    otError error = OT_ERROR_INVALID_ARGS;

    if (aArgs[0] == "active")
    {
        error = otDatasetSetActive(mInterpreter.mInstance, &sDataset);
    }
    else if (aArgs[0] == "pending")
    {
        error = otDatasetSetPending(mInterpreter.mInstance, &sDataset);
    }

    return error;
}

otError Dataset::ProcessDelay(Arg aArgs[])
{
    otError error = OT_ERROR_NONE;

    if (aArgs[0].IsEmpty())
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

otError Dataset::ProcessExtPanId(Arg aArgs[])
{
    otError error = OT_ERROR_NONE;

    if (aArgs[0].IsEmpty())
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

otError Dataset::ProcessMeshLocalPrefix(Arg aArgs[])
{
    otError error = OT_ERROR_NONE;

    if (aArgs[0].IsEmpty())
    {
        if (sDataset.mComponents.mIsMeshLocalPrefixPresent)
        {
            mInterpreter.OutputFormat("Mesh Local Prefix: ");
            mInterpreter.OutputPrefix(sDataset.mMeshLocalPrefix);
            mInterpreter.OutputLine("");
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

otError Dataset::ProcessNetworkKey(Arg aArgs[])
{
    otError error = OT_ERROR_NONE;

    if (aArgs[0].IsEmpty())
    {
        if (sDataset.mComponents.mIsNetworkKeyPresent)
        {
            mInterpreter.OutputBytes(sDataset.mNetworkKey.m8);
            mInterpreter.OutputLine("");
        }
    }
    else
    {
        SuccessOrExit(error = aArgs[0].ParseAsHexString(sDataset.mNetworkKey.m8));
        sDataset.mComponents.mIsNetworkKeyPresent = true;
    }

exit:
    return error;
}

otError Dataset::ProcessNetworkName(Arg aArgs[])
{
    otError error = OT_ERROR_NONE;

    if (aArgs[0].IsEmpty())
    {
        if (sDataset.mComponents.mIsNetworkNamePresent)
        {
            mInterpreter.OutputLine("%s", sDataset.mNetworkName.m8);
        }
    }
    else
    {
        SuccessOrExit(error = otNetworkNameFromString(&sDataset.mNetworkName, aArgs[0].GetCString()));
        sDataset.mComponents.mIsNetworkNamePresent = true;
    }

exit:
    return error;
}

otError Dataset::ProcessPanId(Arg aArgs[])
{
    otError error = OT_ERROR_NONE;

    if (aArgs[0].IsEmpty())
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

otError Dataset::ProcessPendingTimestamp(Arg aArgs[])
{
    otError error = OT_ERROR_NONE;

    if (aArgs[0].IsEmpty())
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

otError Dataset::ProcessMgmtSetCommand(Arg aArgs[])
{
    otError              error = OT_ERROR_NONE;
    otOperationalDataset dataset;
    uint8_t              tlvs[128];
    uint8_t              tlvsLength = 0;

    memset(&dataset, 0, sizeof(dataset));

    for (Arg *arg = &aArgs[1]; !arg->IsEmpty(); arg++)
    {
        if (*arg == "activetimestamp")
        {
            arg++;
            dataset.mComponents.mIsActiveTimestampPresent = true;
            SuccessOrExit(error = arg->ParseAsUint64(dataset.mActiveTimestamp));
        }
        else if (*arg == "pendingtimestamp")
        {
            arg++;
            dataset.mComponents.mIsPendingTimestampPresent = true;
            SuccessOrExit(error = arg->ParseAsUint64(dataset.mPendingTimestamp));
        }
        else if (*arg == "networkkey")
        {
            arg++;
            dataset.mComponents.mIsNetworkKeyPresent = true;
            SuccessOrExit(error = arg->ParseAsHexString(dataset.mNetworkKey.m8));
        }
        else if (*arg == "networkname")
        {
            arg++;
            dataset.mComponents.mIsNetworkNamePresent = true;
            VerifyOrExit(!arg->IsEmpty(), error = OT_ERROR_INVALID_ARGS);
            SuccessOrExit(error = otNetworkNameFromString(&dataset.mNetworkName, arg->GetCString()));
        }
        else if (*arg == "extpanid")
        {
            arg++;
            dataset.mComponents.mIsExtendedPanIdPresent = true;
            SuccessOrExit(error = arg->ParseAsHexString(dataset.mExtendedPanId.m8));
        }
        else if (*arg == "localprefix")
        {
            otIp6Address prefix;

            arg++;
            dataset.mComponents.mIsMeshLocalPrefixPresent = true;
            SuccessOrExit(error = arg->ParseAsIp6Address(prefix));
            memcpy(dataset.mMeshLocalPrefix.m8, prefix.mFields.m8, sizeof(dataset.mMeshLocalPrefix.m8));
        }
        else if (*arg == "delaytimer")
        {
            arg++;
            dataset.mComponents.mIsDelayPresent = true;
            SuccessOrExit(error = arg->ParseAsUint32(dataset.mDelay));
        }
        else if (*arg == "panid")
        {
            arg++;
            dataset.mComponents.mIsPanIdPresent = true;
            SuccessOrExit(error = arg->ParseAsUint16(dataset.mPanId));
        }
        else if (*arg == "channel")
        {
            arg++;
            dataset.mComponents.mIsChannelPresent = true;
            SuccessOrExit(error = arg->ParseAsUint16(dataset.mChannel));
        }
        else if (*arg == "channelmask")
        {
            arg++;
            dataset.mComponents.mIsChannelMaskPresent = true;
            SuccessOrExit(error = arg->ParseAsUint32(dataset.mChannelMask));
        }
        else if (*arg == "securitypolicy")
        {
            arg++;
            SuccessOrExit(error = ParseSecurityPolicy(dataset.mSecurityPolicy, arg));
            dataset.mComponents.mIsSecurityPolicyPresent = true;
        }
        else if (*arg == "-x")
        {
            uint16_t length;

            arg++;
            length = sizeof(tlvs);
            SuccessOrExit(error = arg->ParseAsHexString(length, tlvs));
            tlvsLength = static_cast<uint8_t>(length);
        }
        else
        {
            ExitNow(error = OT_ERROR_INVALID_ARGS);
        }
    }

    if (aArgs[0] == "active")
    {
        error = otDatasetSendMgmtActiveSet(mInterpreter.mInstance, &dataset, tlvs, tlvsLength);
    }
    else if (aArgs[0] == "pending")
    {
        error = otDatasetSendMgmtPendingSet(mInterpreter.mInstance, &dataset, tlvs, tlvsLength);
    }
    else
    {
        error = OT_ERROR_INVALID_ARGS;
    }

exit:
    return error;
}

otError Dataset::ProcessMgmtGetCommand(Arg aArgs[])
{
    otError                        error = OT_ERROR_NONE;
    otOperationalDatasetComponents datasetComponents;
    uint8_t                        tlvs[32];
    uint8_t                        tlvsLength        = 0;
    bool                           destAddrSpecified = false;
    otIp6Address                   address;

    memset(&datasetComponents, 0, sizeof(datasetComponents));

    for (Arg *arg = &aArgs[1]; !arg->IsEmpty(); arg++)
    {
        if (*arg == "activetimestamp")
        {
            datasetComponents.mIsActiveTimestampPresent = true;
        }
        else if (*arg == "pendingtimestamp")
        {
            datasetComponents.mIsPendingTimestampPresent = true;
        }
        else if (*arg == "networkkey")
        {
            datasetComponents.mIsNetworkKeyPresent = true;
        }
        else if (*arg == "networkname")
        {
            datasetComponents.mIsNetworkNamePresent = true;
        }
        else if (*arg == "extpanid")
        {
            datasetComponents.mIsExtendedPanIdPresent = true;
        }
        else if (*arg == "localprefix")
        {
            datasetComponents.mIsMeshLocalPrefixPresent = true;
        }
        else if (*arg == "delaytimer")
        {
            datasetComponents.mIsDelayPresent = true;
        }
        else if (*arg == "panid")
        {
            datasetComponents.mIsPanIdPresent = true;
        }
        else if (*arg == "channel")
        {
            datasetComponents.mIsChannelPresent = true;
        }
        else if (*arg == "securitypolicy")
        {
            datasetComponents.mIsSecurityPolicyPresent = true;
        }
        else if (*arg == "-x")
        {
            uint16_t length;

            arg++;
            length = sizeof(tlvs);
            SuccessOrExit(error = arg->ParseAsHexString(length, tlvs));
            tlvsLength = static_cast<uint8_t>(length);
        }
        else if (*arg == "address")
        {
            arg++;
            SuccessOrExit(error = arg->ParseAsIp6Address(address));
            destAddrSpecified = true;
        }
        else
        {
            ExitNow(error = OT_ERROR_INVALID_ARGS);
        }
    }

    if (aArgs[0] == "active")
    {
        error = otDatasetSendMgmtActiveGet(mInterpreter.mInstance, &datasetComponents, tlvs, tlvsLength,
                                           destAddrSpecified ? &address : nullptr);
    }
    else if (aArgs[0] == "pending")
    {
        error = otDatasetSendMgmtPendingGet(mInterpreter.mInstance, &datasetComponents, tlvs, tlvsLength,
                                            destAddrSpecified ? &address : nullptr);
    }
    else
    {
        error = OT_ERROR_INVALID_ARGS;
    }

exit:
    return error;
}

otError Dataset::ProcessPskc(Arg aArgs[])
{
    otError error = OT_ERROR_NONE;

    if (aArgs[0].IsEmpty())
    {
        if (sDataset.mComponents.mIsPskcPresent)
        {
            mInterpreter.OutputBytes(sDataset.mPskc.m8);
            mInterpreter.OutputLine("");
        }
    }
    else
    {
#if OPENTHREAD_FTD
        if (aArgs[0] == "-p")
        {
            VerifyOrExit(!aArgs[1].IsEmpty(), error = OT_ERROR_INVALID_ARGS);

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
        else
#endif
        {
            SuccessOrExit(error = aArgs[0].ParseAsHexString(sDataset.mPskc.m8));
        }

        sDataset.mComponents.mIsPskcPresent = true;
    }

exit:
    return error;
}

void Dataset::OutputSecurityPolicy(const otSecurityPolicy &aSecurityPolicy)
{
    mInterpreter.OutputFormat("%d ", aSecurityPolicy.mRotationTime);

    if (aSecurityPolicy.mObtainNetworkKeyEnabled)
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

    if (aSecurityPolicy.mNetworkKeyProvisioningEnabled)
    {
        mInterpreter.OutputFormat("p");
    }

    if (aSecurityPolicy.mNonCcmRoutersEnabled)
    {
        mInterpreter.OutputFormat("R");
    }
}

otError Dataset::ParseSecurityPolicy(otSecurityPolicy &aSecurityPolicy, Arg *&aArgs)
{
    otError          error;
    otSecurityPolicy policy;

    memset(&policy, 0, sizeof(policy));

    SuccessOrExit(error = aArgs->ParseAsUint16(policy.mRotationTime));
    aArgs++;

    VerifyOrExit(!aArgs->IsEmpty());

    for (const char *flag = aArgs->GetCString(); *flag != '\0'; flag++)
    {
        switch (*flag)
        {
        case 'o':
            policy.mObtainNetworkKeyEnabled = true;
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
            policy.mNetworkKeyProvisioningEnabled = true;
            break;

        case 'R':
            policy.mNonCcmRoutersEnabled = true;
            break;

        default:
            ExitNow(error = OT_ERROR_INVALID_ARGS);
        }
    }

    aArgs++;

exit:
    if (error == OT_ERROR_NONE)
    {
        aSecurityPolicy = policy;
    }

    return error;
}

otError Dataset::ProcessSecurityPolicy(Arg aArgs[])
{
    otError error = OT_ERROR_NONE;

    if (aArgs[0].IsEmpty())
    {
        if (sDataset.mComponents.mIsSecurityPolicyPresent)
        {
            OutputSecurityPolicy(sDataset.mSecurityPolicy);
            mInterpreter.OutputLine("");
        }
    }
    else
    {
        Arg *arg = &aArgs[0];

        SuccessOrExit(error = ParseSecurityPolicy(sDataset.mSecurityPolicy, arg));
        sDataset.mComponents.mIsSecurityPolicyPresent = true;
    }

exit:
    return error;
}

otError Dataset::ProcessSet(Arg aArgs[])
{
    otError                error = OT_ERROR_NONE;
    MeshCoP::Dataset::Type datasetType;

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
        MeshCoP::Dataset       dataset;
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

otError Dataset::ProcessUpdater(Arg aArgs[])
{
    otError error = OT_ERROR_NONE;

    if (aArgs[0].IsEmpty())
    {
        mInterpreter.OutputEnabledDisabledStatus(otDatasetUpdaterIsUpdateOngoing(mInterpreter.mInstance));
    }
    else if (aArgs[0] == "start")
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
