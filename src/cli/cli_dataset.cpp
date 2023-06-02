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

otOperationalDatasetTlvs Dataset::sDatasetTlvs;

otError Dataset::Print(otOperationalDatasetTlvs &aDatasetTlvs)
{
    otError              error;
    otOperationalDataset dataset;

    SuccessOrExit(error = otDatasetParseTlvs(&aDatasetTlvs, &dataset));

    if (dataset.mComponents.mIsPendingTimestampPresent)
    {
        OutputFormat("Pending Timestamp: ");
        OutputUint64Line(dataset.mPendingTimestamp.mSeconds);
    }

    if (dataset.mComponents.mIsActiveTimestampPresent)
    {
        OutputFormat("Active Timestamp: ");
        OutputUint64Line(dataset.mActiveTimestamp.mSeconds);
    }

    if (dataset.mComponents.mIsChannelPresent)
    {
        OutputLine("Channel: %d", dataset.mChannel);
    }

    if (dataset.mComponents.mIsChannelMaskPresent)
    {
        OutputLine("Channel Mask: 0x%08lx", ToUlong(dataset.mChannelMask));
    }

    if (dataset.mComponents.mIsDelayPresent)
    {
        OutputLine("Delay: %lu", ToUlong(dataset.mDelay));
    }

    if (dataset.mComponents.mIsExtendedPanIdPresent)
    {
        OutputFormat("Ext PAN ID: ");
        OutputBytesLine(dataset.mExtendedPanId.m8);
    }

    if (dataset.mComponents.mIsMeshLocalPrefixPresent)
    {
        OutputFormat("Mesh Local Prefix: ");
        OutputIp6PrefixLine(dataset.mMeshLocalPrefix);
    }

    if (dataset.mComponents.mIsNetworkKeyPresent)
    {
        OutputFormat("Network Key: ");
        OutputBytesLine(dataset.mNetworkKey.m8);
    }

    if (dataset.mComponents.mIsNetworkNamePresent)
    {
        OutputFormat("Network Name: ");
        OutputLine("%s", dataset.mNetworkName.m8);
    }

    if (dataset.mComponents.mIsPanIdPresent)
    {
        OutputLine("PAN ID: 0x%04x", dataset.mPanId);
    }

    if (dataset.mComponents.mIsPskcPresent)
    {
        OutputFormat("PSKc: ");
        OutputBytesLine(dataset.mPskc.m8);
    }

    if (dataset.mComponents.mIsSecurityPolicyPresent)
    {
        OutputFormat("Security Policy: ");
        OutputSecurityPolicy(dataset.mSecurityPolicy);
    }

exit:
    return error;
}

/**
 * @cli dataset init (active,new,pending,tlvs)
 * @code
 * dataset init new
 * Done
 * @endcode
 * @cparam dataset init {@ca{active}|@ca{new}|@ca{pending}|@ca{tlvs}} [@ca{hex-encoded-tlvs}]
 * Use `new` to initialize a new dataset, then enter the command `dataset commit active`.
 * Use `tlvs` for hex-encoded TLVs.
 * @par
 * OT CLI checks for `active`, `pending`, or `tlvs` and returns the corresponding values. Otherwise,
 * OT CLI creates a new, random network and returns a new dataset.
 * @csa{dataset commit active}
 * @csa{dataset active}
 */
template <> otError Dataset::Process<Cmd("init")>(Arg aArgs[])
{
    otError error = OT_ERROR_INVALID_ARGS;

    if (aArgs[0] == "active")
    {
        error = otDatasetGetActiveTlvs(GetInstancePtr(), &sDatasetTlvs);
    }
    else if (aArgs[0] == "pending")
    {
        error = otDatasetGetPendingTlvs(GetInstancePtr(), &sDatasetTlvs);
    }
#if OPENTHREAD_FTD
    else if (aArgs[0] == "new")
    {
        otOperationalDataset dataset;

        SuccessOrExit(error = otDatasetCreateNewNetwork(GetInstancePtr(), &dataset));
        SuccessOrExit(error = otDatasetConvertToTlvs(&dataset, &sDatasetTlvs));
    }
#endif
    else if (aArgs[0] == "tlvs")
    {
        uint16_t size = sizeof(sDatasetTlvs.mTlvs);

        SuccessOrExit(error = aArgs[1].ParseAsHexString(size, sDatasetTlvs.mTlvs));
        sDatasetTlvs.mLength = static_cast<uint8_t>(size);
    }

exit:
    return error;
}

/**
 * @cli dataset active
 * @code
 * dataset active
 * Active Timestamp: 1
 * Channel: 13
 * Channel Mask: 0x07fff800
 * Ext PAN ID: d63e8e3e495ebbc3
 * Mesh Local Prefix: fd3d:b50b:f96d:722d::/64
 * Network Key: dfd34f0f05cad978ec4e32b0413038ff
 * Network Name: OpenThread-8f28
 * PAN ID: 0x8f28
 * PSKc: c23a76e98f1a6483639b1ac1271e2e27
 * Security Policy: 0, onrcb
 * Done
 * @endcode
 * @code
 * dataset active -x
 * 0e08000000000001000000030000103506000...3023d82c841eff0e68db86f35740c030000ff
 * Done
 * @endcode
 * @cparam dataset active [-x]
 * The optional `-x` argument prints the Active Operational %Dataset values as hex-encoded TLVs.
 * @par api_copy
 * #otDatasetGetActive
 * @par
 * OT CLI uses #otOperationalDataset members to return dataset values to the console.
 */
template <> otError Dataset::Process<Cmd("active")>(Arg aArgs[])
{
    otError                  error;
    otOperationalDatasetTlvs dataset;

    SuccessOrExit(error = otDatasetGetActiveTlvs(GetInstancePtr(), &dataset));

    if (aArgs[0].IsEmpty())
    {
        error = Print(dataset);
    }
    else if (aArgs[0] == "-x")
    {
        OutputBytesLine(dataset.mTlvs, dataset.mLength);
    }
    else
    {
        error = OT_ERROR_INVALID_ARGS;
    }

exit:
    return error;
}

template <> otError Dataset::Process<Cmd("pending")>(Arg aArgs[])
{
    otError                  error;
    otOperationalDatasetTlvs datasetTlvs;

    SuccessOrExit(error = otDatasetGetPendingTlvs(GetInstancePtr(), &datasetTlvs));

    if (aArgs[0].IsEmpty())
    {
        error = Print(datasetTlvs);
    }
    else if (aArgs[0] == "-x")
    {
        OutputBytesLine(datasetTlvs.mTlvs, datasetTlvs.mLength);
    }
    else
    {
        error = OT_ERROR_INVALID_ARGS;
    }

exit:
    return error;
}

/**
 * @cli dataset activetimestamp (get, set)
 * @code
 * dataset activetimestamp
 * 123456789
 * Done
 * @endcode
 * @code
 * dataset activetimestamp 123456789
 * Done
 * @endcode
 * @cparam dataset activetimestamp [@ca{timestamp}]
 * Pass the optional `timestamp` argument to set the active timestamp.
 * @par
 * Gets or sets #otOperationalDataset::mActiveTimestamp.
 */
template <> otError Dataset::Process<Cmd("activetimestamp")>(Arg aArgs[])
{
    otError              error = OT_ERROR_NONE;
    otOperationalDataset dataset;

    if (aArgs[0].IsEmpty())
    {
        SuccessOrExit(error = otDatasetParseTlvs(&sDatasetTlvs, &dataset));
        if (dataset.mComponents.mIsActiveTimestampPresent)
        {
            OutputUint64Line(dataset.mActiveTimestamp.mSeconds);
        }
    }
    else
    {
        memset(&dataset, 0, sizeof(dataset));
        SuccessOrExit(error = aArgs[0].ParseAsUint64(dataset.mActiveTimestamp.mSeconds));
        dataset.mActiveTimestamp.mTicks               = 0;
        dataset.mActiveTimestamp.mAuthoritative       = false;
        dataset.mComponents.mIsActiveTimestampPresent = true;
        SuccessOrExit(error = otDatasetUpdateTlvs(&dataset, &sDatasetTlvs));
    }

exit:
    return error;
}

/**
 * @cli dataset channel (get,set)
 * @code
 * dataset channel
 * 12
 * Done
 * @endcode
 * @code
 * dataset channel 12
 * Done
 * @endcode
 * @cparam dataset channel [@ca{channel-num}]
 * Use the optional `channel-num` argument to set the channel.
 * @par
 * Gets or sets #otOperationalDataset::mChannel.
 */
template <> otError Dataset::Process<Cmd("channel")>(Arg aArgs[])
{
    otError              error = OT_ERROR_NONE;
    otOperationalDataset dataset;

    if (aArgs[0].IsEmpty())
    {
        SuccessOrExit(error = otDatasetParseTlvs(&sDatasetTlvs, &dataset));
        if (dataset.mComponents.mIsChannelPresent)
        {
            OutputLine("%d", dataset.mChannel);
        }
    }
    else
    {
        memset(&dataset, 0, sizeof(dataset));
        SuccessOrExit(error = aArgs[0].ParseAsUint16(dataset.mChannel));
        dataset.mComponents.mIsChannelPresent = true;
        SuccessOrExit(error = otDatasetUpdateTlvs(&dataset, &sDatasetTlvs));
    }

exit:
    return error;
}

/**
 * @cli dataset channelmask (get,set)
 * @code
 * dataset channelmask
 * 0x07fff800
 * Done
 * @endcode
 * @code
 * dataset channelmask 0x07fff800
 * Done
 * @endcode
 * @cparam dataset channelmask [@ca{channel-mask}]
 * Use the optional `channel-mask` argument to set the channel mask.
 * @par
 * Gets or sets #otOperationalDataset::mChannelMask
 */
template <> otError Dataset::Process<Cmd("channelmask")>(Arg aArgs[])
{
    otError              error = OT_ERROR_NONE;
    otOperationalDataset dataset;

    if (aArgs[0].IsEmpty())
    {
        SuccessOrExit(error = otDatasetParseTlvs(&sDatasetTlvs, &dataset));
        if (dataset.mComponents.mIsChannelMaskPresent)
        {
            OutputLine("0x%08lx", ToUlong(dataset.mChannelMask));
        }
    }
    else
    {
        memset(&dataset, 0, sizeof(dataset));
        SuccessOrExit(error = aArgs[0].ParseAsUint32(dataset.mChannelMask));
        dataset.mComponents.mIsChannelMaskPresent = true;
        SuccessOrExit(error = otDatasetUpdateTlvs(&dataset, &sDatasetTlvs));
    }

exit:
    return error;
}

/**
 * @cli dataset clear
 * @code
 * dataset clear
 * Done
 * @endcode
 * @par
 * Reset the Operational %Dataset buffer.
 */
template <> otError Dataset::Process<Cmd("clear")>(Arg aArgs[])
{
    OT_UNUSED_VARIABLE(aArgs);

    memset(&sDatasetTlvs, 0, sizeof(sDatasetTlvs));
    return OT_ERROR_NONE;
}

template <> otError Dataset::Process<Cmd("commit")>(Arg aArgs[])
{
    otError error = OT_ERROR_INVALID_ARGS;

    /**
     * @cli dataset commit active
     * @code
     * dataset commit active
     * Done
     * @endcode
     * @par
     * Commit the Operational %Dataset buffer to Active Operational %Dataset.
     * @csa{dataset commit pending}
     * @sa #otDatasetSetPending
     */
    if (aArgs[0] == "active")
    {
        error = otDatasetSetActiveTlvs(GetInstancePtr(), &sDatasetTlvs);
    }
    /**
     * @cli dataset commit pending
     * @code
     * dataset commit pending
     * Done
     * @endcode
     * @par
     * Commit the Operational %Dataset buffer to Pending Operational %Dataset.
     * @csa{dataset commit active}
     * @sa #otDatasetSetActive
     */
    else if (aArgs[0] == "pending")
    {
        error = otDatasetSetPendingTlvs(GetInstancePtr(), &sDatasetTlvs);
    }

    return error;
}

/**
 * @cli dataset delay (get,set)
 * @code
 * dataset delay
 * 1000
 * Done
 * @endcode
 * @code
 * dataset delay 1000
 * Done
 * @endcode
 * @cparam dataset delay [@ca{delay}]
 * Use the optional `delay` argument to set the delay timer value.
 * @par
 * Gets or sets #otOperationalDataset::mDelay.
 * @sa otDatasetSetDelayTimerMinimal
 */
template <> otError Dataset::Process<Cmd("delay")>(Arg aArgs[])
{
    otError              error = OT_ERROR_NONE;
    otOperationalDataset dataset;

    if (aArgs[0].IsEmpty())
    {
        SuccessOrExit(error = otDatasetParseTlvs(&sDatasetTlvs, &dataset));
        if (dataset.mComponents.mIsDelayPresent)
        {
            OutputLine("%lu", ToUlong(dataset.mDelay));
        }
    }
    else
    {
        memset(&dataset, 0, sizeof(dataset));
        SuccessOrExit(error = aArgs[0].ParseAsUint32(dataset.mDelay));
        dataset.mComponents.mIsDelayPresent = true;
        SuccessOrExit(error = otDatasetUpdateTlvs(&dataset, &sDatasetTlvs));
    }

exit:
    return error;
}

/**
 * @cli dataset extpanid (get,set)
 * @code
 * dataset extpanid
 * 000db80123456789
 * Done
 * @endcode
 * @code
 * dataset extpanid 000db80123456789
 * Done
 * @endcode
 * @cparam dataset extpanid [@ca{extpanid}]
 * Use the optional `extpanid` argument to set the Extended Personal Area Network ID.
 * @par
 * Gets or sets #otOperationalDataset::mExtendedPanId.
 * @note The commissioning credential in the dataset buffer becomes stale after changing
 * this value. Use `dataset pskc` to reset.
 * @csa{dataset pskc (get,set)}
 */
template <> otError Dataset::Process<Cmd("extpanid")>(Arg aArgs[])
{
    otError              error = OT_ERROR_NONE;
    otOperationalDataset dataset;

    if (aArgs[0].IsEmpty())
    {
        SuccessOrExit(error = otDatasetParseTlvs(&sDatasetTlvs, &dataset));
        if (dataset.mComponents.mIsExtendedPanIdPresent)
        {
            OutputBytesLine(dataset.mExtendedPanId.m8);
        }
    }
    else
    {
        memset(&dataset, 0, sizeof(dataset));
        SuccessOrExit(error = aArgs[0].ParseAsHexString(dataset.mExtendedPanId.m8));
        dataset.mComponents.mIsExtendedPanIdPresent = true;
        SuccessOrExit(error = otDatasetUpdateTlvs(&dataset, &sDatasetTlvs));
    }

exit:
    return error;
}

/**
 * @cli dataset meshlocalprefix (get,set)
 * @code
 * dataset meshlocalprefix
 * fd00:db8:0:0::/64
 * Done
 * @endcode
 * @code
 * dataset meshlocalprefix fd00:db8:0:0::/64
 * Done
 * @endcode
 * @cparam dataset meshlocalprefix [@ca{meshlocalprefix}]
 * Use the optional `meshlocalprefix` argument to set the Mesh-Local Prefix.
 * @par
 * Gets or sets #otOperationalDataset::mMeshLocalPrefix.
 */
template <> otError Dataset::Process<Cmd("meshlocalprefix")>(Arg aArgs[])
{
    otError              error = OT_ERROR_NONE;
    otOperationalDataset dataset;

    if (aArgs[0].IsEmpty())
    {
        SuccessOrExit(error = otDatasetParseTlvs(&sDatasetTlvs, &dataset));
        if (dataset.mComponents.mIsMeshLocalPrefixPresent)
        {
            OutputFormat("Mesh Local Prefix: ");
            OutputIp6PrefixLine(dataset.mMeshLocalPrefix);
        }
    }
    else
    {
        otIp6Address prefix;

        SuccessOrExit(error = aArgs[0].ParseAsIp6Address(prefix));

        memset(&dataset, 0, sizeof(dataset));
        memcpy(dataset.mMeshLocalPrefix.m8, prefix.mFields.m8, sizeof(dataset.mMeshLocalPrefix.m8));
        dataset.mComponents.mIsMeshLocalPrefixPresent = true;
        SuccessOrExit(error = otDatasetUpdateTlvs(&dataset, &sDatasetTlvs));
    }

exit:
    return error;
}

/**
 * @cli dataset networkkey (get,set)
 * @code
 * dataset networkkey
 * 00112233445566778899aabbccddeeff
 * Done
 * @endcode
 * @code
 * dataset networkkey 00112233445566778899aabbccddeeff
 * Done
 * @endcode
 * @cparam dataset networkkey [@ca{key}]
 * Use the optional `key` argument to set the Network Key.
 * @par
 * Gets or sets #otOperationalDataset::mNetworkKey.
 */
template <> otError Dataset::Process<Cmd("networkkey")>(Arg aArgs[])
{
    otError              error = OT_ERROR_NONE;
    otOperationalDataset dataset;

    if (aArgs[0].IsEmpty())
    {
        SuccessOrExit(error = otDatasetParseTlvs(&sDatasetTlvs, &dataset));
        if (dataset.mComponents.mIsNetworkKeyPresent)
        {
            OutputBytesLine(dataset.mNetworkKey.m8);
        }
    }
    else
    {
        memset(&dataset, 0, sizeof(dataset));
        SuccessOrExit(error = aArgs[0].ParseAsHexString(dataset.mNetworkKey.m8));
        dataset.mComponents.mIsNetworkKeyPresent = true;
        SuccessOrExit(error = otDatasetUpdateTlvs(&dataset, &sDatasetTlvs));
    }

exit:
    return error;
}

/**
 * @cli dataset networkname (get,set)
 * @code
 * dataset networkname
 * OpenThread
 * Done
 * @endcode
 * @code
 * dataset networkname OpenThread
 * Done
 * @endcode
 * @cparam dataset networkname [@ca{name}]
 * Use the optional `name` argument to set the Network Name.
 * @par
 * Gets or sets #otOperationalDataset::mNetworkName.
 * @note The Commissioning Credential in the dataset buffer becomes stale after changing this value.
 * Use `dataset pskc` to reset.
 * @csa{dataset pskc (get,set)}
 */
template <> otError Dataset::Process<Cmd("networkname")>(Arg aArgs[])
{
    otError              error = OT_ERROR_NONE;
    otOperationalDataset dataset;

    if (aArgs[0].IsEmpty())
    {
        SuccessOrExit(error = otDatasetParseTlvs(&sDatasetTlvs, &dataset));
        if (dataset.mComponents.mIsNetworkNamePresent)
        {
            OutputLine("%s", dataset.mNetworkName.m8);
        }
    }
    else
    {
        memset(&dataset, 0, sizeof(dataset));
        SuccessOrExit(error = otNetworkNameFromString(&dataset.mNetworkName, aArgs[0].GetCString()));
        dataset.mComponents.mIsNetworkNamePresent = true;
        SuccessOrExit(error = otDatasetUpdateTlvs(&dataset, &sDatasetTlvs));
    }

exit:
    return error;
}

/**
 * @cli dataset panid (get,set)
 * @code
 * dataset panid
 * 0x1234
 * Done
 * @endcode
 * @code
 * dataset panid 0x1234
 * Done
 * @endcode
 * @cparam dataset panid [@ca{panid}]
 * Use the optional `panid` argument to set the PAN ID.
 * @par
 * Gets or sets #otOperationalDataset::mPanId.
 */
template <> otError Dataset::Process<Cmd("panid")>(Arg aArgs[])
{
    otError              error = OT_ERROR_NONE;
    otOperationalDataset dataset;

    if (aArgs[0].IsEmpty())
    {
        SuccessOrExit(error = otDatasetParseTlvs(&sDatasetTlvs, &dataset));
        if (dataset.mComponents.mIsPanIdPresent)
        {
            OutputLine("0x%04x", dataset.mPanId);
        }
    }
    else
    {
        memset(&dataset, 0, sizeof(dataset));
        SuccessOrExit(error = aArgs[0].ParseAsUint16(dataset.mPanId));
        dataset.mComponents.mIsPanIdPresent = true;
        SuccessOrExit(error = otDatasetUpdateTlvs(&dataset, &sDatasetTlvs));
    }

exit:
    return error;
}

/**
 * @cli dataset pendingtimestamp (get,set)
 * @code
 * dataset pendingtimestamp
 * 123456789
 * Done
 * @endcode
 * @code
 * dataset pendingtimestamp 123456789
 * Done
 * @endcode
 * @cparam dataset pendingtimestamp [@ca{timestamp}]
 * Use the optional `timestamp` argument to set the pending timestamp seconds.
 * @par
 * Gets or sets #otOperationalDataset::mPendingTimestamp.
 */
template <> otError Dataset::Process<Cmd("pendingtimestamp")>(Arg aArgs[])
{
    otError              error = OT_ERROR_NONE;
    otOperationalDataset dataset;

    if (aArgs[0].IsEmpty())
    {
        SuccessOrExit(error = otDatasetParseTlvs(&sDatasetTlvs, &dataset));
        if (dataset.mComponents.mIsPendingTimestampPresent)
        {
            OutputUint64Line(dataset.mPendingTimestamp.mSeconds);
        }
    }
    else
    {
        memset(&dataset, 0, sizeof(dataset));
        SuccessOrExit(error = aArgs[0].ParseAsUint64(dataset.mPendingTimestamp.mSeconds));
        dataset.mPendingTimestamp.mTicks               = 0;
        dataset.mPendingTimestamp.mAuthoritative       = false;
        dataset.mComponents.mIsPendingTimestampPresent = true;
        SuccessOrExit(error = otDatasetUpdateTlvs(&dataset, &sDatasetTlvs));
    }

exit:
    return error;
}

template <> otError Dataset::Process<Cmd("mgmtsetcommand")>(Arg aArgs[])
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
            SuccessOrExit(error = arg->ParseAsUint64(dataset.mActiveTimestamp.mSeconds));
            dataset.mActiveTimestamp.mTicks               = 0;
            dataset.mActiveTimestamp.mAuthoritative       = false;
            dataset.mComponents.mIsActiveTimestampPresent = true;
        }
        else if (*arg == "pendingtimestamp")
        {
            arg++;
            SuccessOrExit(error = arg->ParseAsUint64(dataset.mPendingTimestamp.mSeconds));
            dataset.mPendingTimestamp.mTicks               = 0;
            dataset.mPendingTimestamp.mAuthoritative       = false;
            dataset.mComponents.mIsPendingTimestampPresent = true;
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

    /**
     * @cli dataset mgmtsetcommand active
     * @code
     * dataset mgmtsetcommand active activetimestamp 123 securitypolicy 1 onrcb
     * Done
     * @endcode
     * @cparam dataset mgmtsetcommand active [@ca{dataset-components}] [-x @ca{tlv-list}]
     * To learn more about these parameters and argument mappings, refer to @dataset.
     * @par
     * @note This command is primarily used for testing only.
     * @par api_copy
     * #otDatasetSendMgmtActiveSet
     * @csa{dataset mgmtgetcommand active}
     * @csa{dataset mgmtgetcommand pending}
     * @csa{dataset mgmtsetcommand pending}
     */
    if (aArgs[0] == "active")
    {
        error = otDatasetSendMgmtActiveSet(GetInstancePtr(), &dataset, tlvs, tlvsLength, /* aCallback */ nullptr,
                                           /* aContext */ nullptr);
    }
    /**
     * @cli dataset mgmtsetcommand pending
     * @code
     * dataset mgmtsetcommand pending activetimestamp 123 securitypolicy 1 onrcb
     * Done
     * @endcode
     * @cparam dataset mgmtsetcommand pending [@ca{dataset-components}] [-x @ca{tlv-list}]
     * To learn more about these parameters and argument mappings, refer to @dataset.
     * @par
     * @note This command is primarily used for testing only.
     * @par api_copy
     * #otDatasetSendMgmtPendingSet
     * @csa{dataset mgmtgetcommand active}
     * @csa{dataset mgmtgetcommand pending}
     * @csa{dataset mgmtsetcommand active}
     */
    else if (aArgs[0] == "pending")
    {
        error = otDatasetSendMgmtPendingSet(GetInstancePtr(), &dataset, tlvs, tlvsLength, /* aCallback */ nullptr,
                                            /* aContext */ nullptr);
    }
    else
    {
        error = OT_ERROR_INVALID_ARGS;
    }

exit:
    return error;
}

template <> otError Dataset::Process<Cmd("mgmtgetcommand")>(Arg aArgs[])
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

    /**
     * @cli dataset mgmtgetcommand active
     * @code
     * dataset mgmtgetcommand active address fdde:ad00:beef:0:558:f56b:d688:799 activetimestamp securitypolicy
     * Done
     * @endcode
     * @code
     * dataset mgmtgetcommand active networkname
     * Done
     * @endcode
     * @cparam dataset mgmtgetcommand active [address @ca{leader-address}] [@ca{dataset-components}] [-x @ca{tlv-list}]
     * *    Use `address` to specify the IPv6 destination; otherwise, the Leader ALOC is used as default.
     * *    For `dataset-components`, you can pass any combination of #otOperationalDatasetComponents, for
     *      example `activetimestamp`, `pendingtimestamp`, or `networkkey`.
     * *    The optional `-x` argument specifies raw TLVs to be requested.
     * @par
     * OT CLI sends a MGMT_ACTIVE_GET with the relevant arguments.
     * To learn more about these parameters and argument mappings, refer to @dataset.
     * @note This command is primarily used for testing only.
     * @par api_copy
     * #otDatasetSendMgmtActiveGet
     * @csa{dataset mgmtgetcommand pending}
     * @csa{dataset mgmtsetcommand active}
     * @csa{dataset mgmtsetcommand pending}
     */
    if (aArgs[0] == "active")
    {
        error = otDatasetSendMgmtActiveGet(GetInstancePtr(), &datasetComponents, tlvs, tlvsLength,
                                           destAddrSpecified ? &address : nullptr);
    }
    /**
     * @cli dataset mgmtgetcommand pending
     * @code
     * dataset mgmtgetcommand pending address fdde:ad00:beef:0:558:f56b:d688:799 activetimestamp securitypolicy
     * Done
     * @endcode
     * @code
     * dataset mgmtgetcommand pending networkname
     * Done
     * @endcode
     * @cparam dataset mgmtgetcommand pending [address @ca{leader-address}] [@ca{dataset-components}] [-x @ca{tlv-list}]
     * To learn more about these parameters and argument mappings, refer to @dataset.
     * @par
     * @note This command is primarily used for testing only.
     * @par api_copy
     * #otDatasetSendMgmtPendingGet
     * @csa{dataset mgmtgetcommand active}
     * @csa{dataset mgmtsetcommand active}
     * @csa{dataset mgmtsetcommand pending}
     */
    else if (aArgs[0] == "pending")
    {
        error = otDatasetSendMgmtPendingGet(GetInstancePtr(), &datasetComponents, tlvs, tlvsLength,
                                            destAddrSpecified ? &address : nullptr);
    }
    else
    {
        error = OT_ERROR_INVALID_ARGS;
    }

exit:
    return error;
}

/**
 * @cli dataset pskc (get,set)
 * @code
 * dataset pskc
 * 67c0c203aa0b042bfb5381c47aef4d9e
 * Done
 * @endcode
 * @code
 * dataset pskc -p 123456
 * Done
 * @endcode
 * @code
 * dataset pskc 67c0c203aa0b042bfb5381c47aef4d9e
 * Done
 * @endcode
 * @cparam dataset pskc [@ca{-p} @ca{passphrase}] | [@ca{key}]
 * For FTD only, use `-p` with the `passphrase` argument. `-p` generates a pskc from
 * the UTF-8 encoded `passphrase` that you provide, together with
 * the network name and extended PAN ID. If set, `-p` uses the dataset buffer;
 * otherwise, it uses the current stack.
 * Alternatively, you can set pskc as `key` (hex format).
 * @par
 * Gets or sets #otOperationalDataset::mPskc.
 */
template <> otError Dataset::Process<Cmd("pskc")>(Arg aArgs[])
{
    otError              error = OT_ERROR_NONE;
    otOperationalDataset dataset;

    if (aArgs[0].IsEmpty())
    {
        SuccessOrExit(error = otDatasetParseTlvs(&sDatasetTlvs, &dataset));
        // dataset holds the key as a literal string, we don't
        // need to export it from PSA ITS.
        if (dataset.mComponents.mIsPskcPresent)
        {
            OutputBytesLine(dataset.mPskc.m8);
        }
    }
    else
    {
        memset(&dataset, 0, sizeof(dataset));
#if OPENTHREAD_FTD
        if (aArgs[0] == "-p")
        {
            VerifyOrExit(!aArgs[1].IsEmpty(), error = OT_ERROR_INVALID_ARGS);

            SuccessOrExit(
                error = otDatasetGeneratePskc(
                    aArgs[1].GetCString(),
                    (dataset.mComponents.mIsNetworkNamePresent
                         ? &dataset.mNetworkName
                         : reinterpret_cast<const otNetworkName *>(otThreadGetNetworkName(GetInstancePtr()))),
                    (dataset.mComponents.mIsExtendedPanIdPresent ? &dataset.mExtendedPanId
                                                                 : otThreadGetExtendedPanId(GetInstancePtr())),
                    &dataset.mPskc));
        }
        else
#endif
        {
            SuccessOrExit(error = aArgs[0].ParseAsHexString(dataset.mPskc.m8));
        }

        dataset.mComponents.mIsPskcPresent = true;
        SuccessOrExit(error = otDatasetUpdateTlvs(&dataset, &sDatasetTlvs));
    }

exit:
    return error;
}

void Dataset::OutputSecurityPolicy(const otSecurityPolicy &aSecurityPolicy)
{
    OutputFormat("%d ", aSecurityPolicy.mRotationTime);

    if (aSecurityPolicy.mObtainNetworkKeyEnabled)
    {
        OutputFormat("o");
    }

    if (aSecurityPolicy.mNativeCommissioningEnabled)
    {
        OutputFormat("n");
    }

    if (aSecurityPolicy.mRoutersEnabled)
    {
        OutputFormat("r");
    }

    if (aSecurityPolicy.mExternalCommissioningEnabled)
    {
        OutputFormat("c");
    }

    if (aSecurityPolicy.mCommercialCommissioningEnabled)
    {
        OutputFormat("C");
    }

    if (aSecurityPolicy.mAutonomousEnrollmentEnabled)
    {
        OutputFormat("e");
    }

    if (aSecurityPolicy.mNetworkKeyProvisioningEnabled)
    {
        OutputFormat("p");
    }

    if (aSecurityPolicy.mNonCcmRoutersEnabled)
    {
        OutputFormat("R");
    }

    OutputNewLine();
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

/**
 * @cli dataset securitypolicy (get,set)
 * @code
 * dataset securitypolicy
 * 672 onrc
 * Done
 * @endcode
 * @code
 * dataset securitypolicy 672 onrc
 * Done
 * @endcode
 * @cparam dataset securitypolicy [@ca{rotationtime} [@ca{onrcCepR}]]
 * *   Use `rotationtime` for `thrKeyRotation`, in units of hours.
 * *   Security Policy commands use the `onrcCepR` argument mappings to get and set
 * #otSecurityPolicy members, for example `o` represents
 * #otSecurityPolicy::mObtainNetworkKeyEnabled.
 * @moreinfo{@dataset}.
 * @par
 * Gets or sets the %Dataset security policy.
 */
template <> otError Dataset::Process<Cmd("securitypolicy")>(Arg aArgs[])
{
    otError              error = OT_ERROR_NONE;
    otOperationalDataset dataset;

    if (aArgs[0].IsEmpty())
    {
        SuccessOrExit(error = otDatasetParseTlvs(&sDatasetTlvs, &dataset));
        if (dataset.mComponents.mIsSecurityPolicyPresent)
        {
            OutputSecurityPolicy(dataset.mSecurityPolicy);
        }
    }
    else
    {
        Arg *arg = &aArgs[0];

        memset(&dataset, 0, sizeof(dataset));
        SuccessOrExit(error = ParseSecurityPolicy(dataset.mSecurityPolicy, arg));
        dataset.mComponents.mIsSecurityPolicyPresent = true;
        SuccessOrExit(error = otDatasetUpdateTlvs(&dataset, &sDatasetTlvs));
    }

exit:
    return error;
}

/**
 * @cli dataset set (active,pending)
 * @code
 * dataset set active 0e08000000000001000000030000103506000...3023d82c841eff0e68db86f35740c030000ff
 * Done
 * @endcode
 * @code
 * dataset set pending 0e08000000000001000000030000103506000...3023d82c841eff0e68db86f35740c030000ff
 * Done
 * @endcode
 * @cparam dataset set {active|pending} @ca{tlvs}
 * @par
 * The CLI `dataset set` command sets the Active Operational %Dataset using hex-encoded TLVs.
 * @par api_copy
 * #otDatasetSetActive
 */
template <> otError Dataset::Process<Cmd("set")>(Arg aArgs[])
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
        otOperationalDataset     dataset;
        otOperationalDatasetTlvs datasetTlvs;
        uint16_t                 tlvsLength = MeshCoP::Dataset::kMaxSize;

        SuccessOrExit(error = aArgs[1].ParseAsHexString(tlvsLength, datasetTlvs.mTlvs));
        datasetTlvs.mLength = static_cast<uint8_t>(tlvsLength);

        SuccessOrExit(error = otDatasetParseTlvs(&datasetTlvs, &dataset));

        switch (datasetType)
        {
        case MeshCoP::Dataset::Type::kActive:
            SuccessOrExit(error = otDatasetSetActiveTlvs(GetInstancePtr(), &datasetTlvs));
            break;
        case MeshCoP::Dataset::Type::kPending:
            SuccessOrExit(error = otDatasetSetPendingTlvs(GetInstancePtr(), &datasetTlvs));
            break;
        }
    }

exit:
    return error;
}

/**
 * @cli dataset tlvs
 * @code
 * dataset tlvs
 * 0e080000000000010000000300001635060004001fffe0020...f7f8
 * Done
 * @endcode
 * @par api_copy
 * #otDatasetConvertToTlvs
 */
template <> otError Dataset::Process<Cmd("tlvs")>(Arg aArgs[])
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(aArgs[0].IsEmpty(), error = OT_ERROR_INVALID_ARGS);
    OutputBytesLine(sDatasetTlvs.mTlvs, sDatasetTlvs.mLength);

exit:
    return error;
}

#if OPENTHREAD_CONFIG_DATASET_UPDATER_ENABLE && OPENTHREAD_FTD

template <> otError Dataset::Process<Cmd("updater")>(Arg aArgs[])
{
    otError error = OT_ERROR_NONE;

    if (aArgs[0].IsEmpty())
    {
        OutputEnabledDisabledStatus(otDatasetUpdaterIsUpdateOngoing(GetInstancePtr()));
    }
    else if (aArgs[0] == "start")
    {
        otOperationalDataset dataset;

        SuccessOrExit(error = otDatasetParseTlvs(&sDatasetTlvs, &dataset));
        SuccessOrExit(
            error = otDatasetUpdaterRequestUpdate(GetInstancePtr(), &dataset, &Dataset::HandleDatasetUpdater, this));
    }
    else if (aArgs[0] == "cancel")
    {
        otDatasetUpdaterCancelUpdate(GetInstancePtr());
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
    OutputLine("Dataset update complete: %s", otThreadErrorToString(aError));
}

#endif // OPENTHREAD_CONFIG_DATASET_UPDATER_ENABLE && OPENTHREAD_FTD

otError Dataset::Process(Arg aArgs[])
{
#define CmdEntry(aCommandString)                               \
    {                                                          \
        aCommandString, &Dataset::Process<Cmd(aCommandString)> \
    }

    static constexpr Command kCommands[] = {
        CmdEntry("active"),
        CmdEntry("activetimestamp"),
        CmdEntry("channel"),
        CmdEntry("channelmask"),
        CmdEntry("clear"),
        CmdEntry("commit"),
        CmdEntry("delay"),
        CmdEntry("extpanid"),
        CmdEntry("init"),
        CmdEntry("meshlocalprefix"),
        CmdEntry("mgmtgetcommand"),
        CmdEntry("mgmtsetcommand"),
        CmdEntry("networkkey"),
        CmdEntry("networkname"),
        CmdEntry("panid"),
        CmdEntry("pending"),
        CmdEntry("pendingtimestamp"),
        CmdEntry("pskc"),
        CmdEntry("securitypolicy"),
        CmdEntry("set"),
        CmdEntry("tlvs"),
#if OPENTHREAD_CONFIG_DATASET_UPDATER_ENABLE && OPENTHREAD_FTD
        CmdEntry("updater"),
#endif
    };

#undef CmdEntry

    static_assert(BinarySearch::IsSorted(kCommands), "kCommands is not sorted");

    otError        error = OT_ERROR_INVALID_COMMAND;
    const Command *command;

    if (aArgs[0].IsEmpty())
    {
        ExitNow(error = Print(sDatasetTlvs));
    }

    /**
     * @cli dataset help
     * @code
     * dataset help
     * help
     * active
     * activetimestamp
     * channel
     * channelmask
     * clear
     * commit
     * delay
     * extpanid
     * init
     * meshlocalprefix
     * mgmtgetcommand
     * mgmtsetcommand
     * networkkey
     * networkname
     * panid
     * pending
     * pendingtimestamp
     * pskc
     * securitypolicy
     * set
     * tlvs
     * Done
     * @endcode
     * @par
     * Gets a list of `dataset` CLI commands. @moreinfo{@dataset}.
     */
    if (aArgs[0] == "help")
    {
        OutputCommandTable(kCommands);
        ExitNow(error = OT_ERROR_NONE);
    }

    command = BinarySearch::Find(aArgs[0].GetCString(), kCommands);
    VerifyOrExit(command != nullptr);

    error = (this->*command->mHandler)(aArgs + 1);

exit:
    return error;
}

} // namespace Cli
} // namespace ot
