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

const Dataset::ComponentMapper *Dataset::LookupMapper(const char *aName) const
{
    static constexpr ComponentMapper kMappers[] = {
        {
            "activetimestamp",
            &Components::mIsActiveTimestampPresent,
            &Dataset::OutputActiveTimestamp,
            &Dataset::ParseActiveTimestamp,
        },
        {
            "channel",
            &Components::mIsChannelPresent,
            &Dataset::OutputChannel,
            &Dataset::ParseChannel,
        },
        {
            "channelmask",
            &Components::mIsChannelMaskPresent,
            &Dataset::OutputChannelMask,
            &Dataset::ParseChannelMask,
        },
        {
            "delay",
            &Components::mIsDelayPresent,
            &Dataset::OutputDelay,
            &Dataset::ParseDelay,
        },
        {
            "delaytimer", // Alias for "delay "to ensure backward compatibility for "mgmtsetcommand" command
            &Components::mIsDelayPresent,
            &Dataset::OutputDelay,
            &Dataset::ParseDelay,
        },
        {
            "extpanid",
            &Components::mIsExtendedPanIdPresent,
            &Dataset::OutputExtendedPanId,
            &Dataset::ParseExtendedPanId,
        },
        {
            "localprefix", // Alias for "meshlocalprefix" to ensure backward compatibility in "mgmtsetcommand" command
            &Components::mIsMeshLocalPrefixPresent,
            &Dataset::OutputMeshLocalPrefix,
            &Dataset::ParseMeshLocalPrefix,
        },
        {
            "meshlocalprefix",
            &Components::mIsMeshLocalPrefixPresent,
            &Dataset::OutputMeshLocalPrefix,
            &Dataset::ParseMeshLocalPrefix,
        },
        {
            "networkkey",
            &Components::mIsNetworkKeyPresent,
            &Dataset::OutputNetworkKey,
            &Dataset::ParseNetworkKey,
        },
        {
            "networkname",
            &Components::mIsNetworkNamePresent,
            &Dataset::OutputNetworkName,
            &Dataset::ParseNetworkName,
        },
        {
            "panid",
            &Components::mIsPanIdPresent,
            &Dataset::OutputPanId,
            &Dataset::ParsePanId,
        },
        {
            "pendingtimestamp",
            &Components::mIsPendingTimestampPresent,
            &Dataset::OutputPendingTimestamp,
            &Dataset::ParsePendingTimestamp,
        },
        {
            "pskc",
            &Components::mIsPskcPresent,
            &Dataset::OutputPskc,
            &Dataset::ParsePskc,
        },
        {
            "securitypolicy",
            &Components::mIsSecurityPolicyPresent,
            &Dataset::OutputSecurityPolicy,
            &Dataset::ParseSecurityPolicy,
        },
        {
            "wakeupchannel",
            &Components::mIsWakeupChannelPresent,
            &Dataset::OutputWakeupChannel,
            &Dataset::ParseWakeupChannel,
        },
    };

    static_assert(BinarySearch::IsSorted(kMappers), "kMappers is not sorted");

    return BinarySearch::Find(aName, kMappers);
}

//---------------------------------------------------------------------------------------------------------------------

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
void Dataset::OutputActiveTimestamp(const otOperationalDataset &aDataset)
{
    OutputUint64Line(aDataset.mActiveTimestamp.mSeconds);
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
void Dataset::OutputChannel(const otOperationalDataset &aDataset) { OutputLine("%u", aDataset.mChannel); }

/**
 * @cli dataset wakeupchannel (get,set)
 * @code
 * dataset wakeupchannel
 * 13
 * Done
 * @endcode
 * @code
 * dataset wakeupchannel 13
 * Done
 * @endcode
 * @cparam dataset wakeupchannel [@ca{channel-num}]
 * Use the optional `channel-num` argument to set the wake-up channel.
 * @par
 * Gets or sets #otOperationalDataset::mWakeupChannel.
 */
void Dataset::OutputWakeupChannel(const otOperationalDataset &aDataset) { OutputLine("%u", aDataset.mWakeupChannel); }

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
void Dataset::OutputChannelMask(const otOperationalDataset &aDataset)
{
    OutputLine("0x%08lx", ToUlong(aDataset.mChannelMask));
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
void Dataset::OutputDelay(const otOperationalDataset &aDataset) { OutputLine("%lu", ToUlong(aDataset.mDelay)); }

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
void Dataset::OutputExtendedPanId(const otOperationalDataset &aDataset) { OutputBytesLine(aDataset.mExtendedPanId.m8); }

/**
 * @cli dataset meshlocalprefix (get,set)
 * @code
 * dataset meshlocalprefix
 * fd00:db8:0:0::/64
 * Done
 * @endcode
 * @code
 * dataset meshlocalprefix fd00:db8:0:0::
 * Done
 * @endcode
 * @cparam dataset meshlocalprefix [@ca{meshlocalprefix}]
 * Use the optional `meshlocalprefix` argument to set the Mesh-Local Prefix.
 * @par
 * Gets or sets #otOperationalDataset::mMeshLocalPrefix.
 */
void Dataset::OutputMeshLocalPrefix(const otOperationalDataset &aDataset)
{
    OutputIp6PrefixLine(aDataset.mMeshLocalPrefix);
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
void Dataset::OutputNetworkKey(const otOperationalDataset &aDataset) { OutputBytesLine(aDataset.mNetworkKey.m8); }

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
void Dataset::OutputNetworkName(const otOperationalDataset &aDataset) { OutputLine("%s", aDataset.mNetworkName.m8); }

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
void Dataset::OutputPanId(const otOperationalDataset &aDataset) { OutputLine("0x%04x", aDataset.mPanId); }

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
void Dataset::OutputPendingTimestamp(const otOperationalDataset &aDataset)
{
    OutputUint64Line(aDataset.mPendingTimestamp.mSeconds);
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
void Dataset::OutputPskc(const otOperationalDataset &aDataset) { OutputBytesLine(aDataset.mPskc.m8); }

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
void Dataset::OutputSecurityPolicy(const otOperationalDataset &aDataset)
{
    OutputSecurityPolicy(aDataset.mSecurityPolicy);
}

//---------------------------------------------------------------------------------------------------------------------

otError Dataset::ParseActiveTimestamp(Arg *&aArgs, otOperationalDataset &aDataset)
{
    otError error;

    SuccessOrExit(error = aArgs++->ParseAsUint64(aDataset.mActiveTimestamp.mSeconds));
    aDataset.mActiveTimestamp.mTicks         = 0;
    aDataset.mActiveTimestamp.mAuthoritative = false;

exit:
    return error;
}

otError Dataset::ParseChannel(Arg *&aArgs, otOperationalDataset &aDataset)
{
    return aArgs++->ParseAsUint16(aDataset.mChannel);
}

otError Dataset::ParseWakeupChannel(Arg *&aArgs, otOperationalDataset &aDataset)
{
    return aArgs++->ParseAsUint16(aDataset.mWakeupChannel);
}

otError Dataset::ParseChannelMask(Arg *&aArgs, otOperationalDataset &aDataset)
{
    return aArgs++->ParseAsUint32(aDataset.mChannelMask);
}

otError Dataset::ParseDelay(Arg *&aArgs, otOperationalDataset &aDataset)
{
    return aArgs++->ParseAsUint32(aDataset.mDelay);
}

otError Dataset::ParseExtendedPanId(Arg *&aArgs, otOperationalDataset &aDataset)
{
    return aArgs++->ParseAsHexString(aDataset.mExtendedPanId.m8);
}

otError Dataset::ParseMeshLocalPrefix(Arg *&aArgs, otOperationalDataset &aDataset)
{
    otError      error;
    otIp6Address prefix;

    SuccessOrExit(error = aArgs++->ParseAsIp6Address(prefix));

    memcpy(aDataset.mMeshLocalPrefix.m8, prefix.mFields.m8, sizeof(aDataset.mMeshLocalPrefix.m8));

exit:
    return error;
}

otError Dataset::ParseNetworkKey(Arg *&aArgs, otOperationalDataset &aDataset)
{
    return aArgs++->ParseAsHexString(aDataset.mNetworkKey.m8);
}

otError Dataset::ParseNetworkName(Arg *&aArgs, otOperationalDataset &aDataset)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(!aArgs->IsEmpty(), error = OT_ERROR_INVALID_ARGS);
    error = otNetworkNameFromString(&aDataset.mNetworkName, aArgs++->GetCString());

exit:
    return error;
}

otError Dataset::ParsePanId(Arg *&aArgs, otOperationalDataset &aDataset)
{
    return aArgs++->ParseAsUint16(aDataset.mPanId);
}

otError Dataset::ParsePendingTimestamp(Arg *&aArgs, otOperationalDataset &aDataset)
{
    otError error;

    SuccessOrExit(error = aArgs++->ParseAsUint64(aDataset.mPendingTimestamp.mSeconds));
    aDataset.mPendingTimestamp.mTicks         = 0;
    aDataset.mPendingTimestamp.mAuthoritative = false;

exit:
    return error;
}

otError Dataset::ParsePskc(Arg *&aArgs, otOperationalDataset &aDataset)
{
    otError error;

#if OPENTHREAD_FTD
    if (*aArgs == "-p")
    {
        aArgs++;
        VerifyOrExit(!aArgs->IsEmpty(), error = OT_ERROR_INVALID_ARGS);

        SuccessOrExit(error = otDatasetGeneratePskc(
                          aArgs->GetCString(),
                          (aDataset.mComponents.mIsNetworkNamePresent
                               ? &aDataset.mNetworkName
                               : reinterpret_cast<const otNetworkName *>(otThreadGetNetworkName(GetInstancePtr()))),
                          (aDataset.mComponents.mIsExtendedPanIdPresent ? &aDataset.mExtendedPanId
                                                                        : otThreadGetExtendedPanId(GetInstancePtr())),
                          &aDataset.mPskc));
        aArgs++;
    }
    else
#endif
    {
        ExitNow(error = aArgs++->ParseAsHexString(aDataset.mPskc.m8));
    }

exit:
    return error;
}

otError Dataset::ParseSecurityPolicy(Arg *&aArgs, otOperationalDataset &aDataset)
{
    return ParseSecurityPolicy(aDataset.mSecurityPolicy, aArgs);
}

otError Dataset::ParseTlvs(Arg &aArg, otOperationalDatasetTlvs &aDatasetTlvs)
{
    otError  error;
    uint16_t length;

    length = sizeof(aDatasetTlvs.mTlvs);
    SuccessOrExit(error = aArg.ParseAsHexString(length, aDatasetTlvs.mTlvs));
    aDatasetTlvs.mLength = static_cast<uint8_t>(length);

exit:
    return error;
}

//---------------------------------------------------------------------------------------------------------------------

otError Dataset::ProcessCommand(const ComponentMapper &aMapper, Arg aArgs[])
{
    otError              error = OT_ERROR_NONE;
    otOperationalDataset dataset;

    if (aArgs[0].IsEmpty())
    {
        SuccessOrExit(error = otDatasetParseTlvs(&sDatasetTlvs, &dataset));

        if (dataset.mComponents.*aMapper.mIsPresentPtr)
        {
            (this->*aMapper.mOutput)(dataset);
        }
    }
    else
    {
        ClearAllBytes(dataset);
        SuccessOrExit(error = (this->*aMapper.mParse)(aArgs, dataset));
        dataset.mComponents.*aMapper.mIsPresentPtr = true;
        SuccessOrExit(error = otDatasetUpdateTlvs(&dataset, &sDatasetTlvs));
    }

exit:
    return error;
}

otError Dataset::Print(otOperationalDatasetTlvs &aDatasetTlvs)
{
    struct ComponentTitle
    {
        const char *mTitle; // Title to output.
        const char *mName;  // To use with `LookupMapper()`.
    };

    static const ComponentTitle kTitles[] = {
        {"Pending Timestamp", "pendingtimestamp"},
        {"Active Timestamp", "activetimestamp"},
        {"Channel", "channel"},
        {"Wake-up Channel", "wakeupchannel"},
        {"Channel Mask", "channelmask"},
        {"Delay", "delay"},
        {"Ext PAN ID", "extpanid"},
        {"Mesh Local Prefix", "meshlocalprefix"},
        {"Network Key", "networkkey"},
        {"Network Name", "networkname"},
        {"PAN ID", "panid"},
        {"PSKc", "pskc"},
        {"Security Policy", "securitypolicy"},
    };

    otError              error;
    otOperationalDataset dataset;

    SuccessOrExit(error = otDatasetParseTlvs(&aDatasetTlvs, &dataset));

    for (const ComponentTitle &title : kTitles)
    {
        const ComponentMapper *mapper = LookupMapper(title.mName);

        if (dataset.mComponents.*mapper->mIsPresentPtr)
        {
            OutputFormat("%s: ", title.mTitle);
            (this->*mapper->mOutput)(dataset);
        }
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
        otDatasetConvertToTlvs(&dataset, &sDatasetTlvs);
    }
#endif
    else if (aArgs[0] == "tlvs")
    {
        ExitNow(error = ParseTlvs(aArgs[1], sDatasetTlvs));
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

    ClearAllBytes(sDatasetTlvs);
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

template <> otError Dataset::Process<Cmd("mgmtsetcommand")>(Arg aArgs[])
{
    otError                  error = OT_ERROR_NONE;
    otOperationalDataset     dataset;
    otOperationalDatasetTlvs tlvs;

    ClearAllBytes(dataset);
    ClearAllBytes(tlvs);

    for (Arg *arg = &aArgs[1]; !arg->IsEmpty();)
    {
        const ComponentMapper *mapper = LookupMapper(arg->GetCString());

        if (mapper != nullptr)
        {
            arg++;
            SuccessOrExit(error = (this->*mapper->mParse)(arg, dataset));
            dataset.mComponents.*mapper->mIsPresentPtr = true;
        }
        else if (*arg == "-x")
        {
            arg++;
            SuccessOrExit(error = ParseTlvs(*arg, tlvs));
            arg++;
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
        error =
            otDatasetSendMgmtActiveSet(GetInstancePtr(), &dataset, tlvs.mTlvs, tlvs.mLength, /* aCallback */ nullptr,
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
        error =
            otDatasetSendMgmtPendingSet(GetInstancePtr(), &dataset, tlvs.mTlvs, tlvs.mLength, /* aCallback */ nullptr,
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
    otOperationalDatasetTlvs       tlvs;
    bool                           destAddrSpecified = false;
    otIp6Address                   address;

    ClearAllBytes(datasetComponents);
    ClearAllBytes(tlvs);

    for (Arg *arg = &aArgs[1]; !arg->IsEmpty(); arg++)
    {
        const ComponentMapper *mapper = LookupMapper(arg->GetCString());

        if (mapper != nullptr)
        {
            datasetComponents.*mapper->mIsPresentPtr = true;
        }
        else if (*arg == "-x")
        {
            arg++;
            SuccessOrExit(error = ParseTlvs(*arg, tlvs));
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
        error = otDatasetSendMgmtActiveGet(GetInstancePtr(), &datasetComponents, tlvs.mTlvs, tlvs.mLength,
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
        error = otDatasetSendMgmtPendingGet(GetInstancePtr(), &datasetComponents, tlvs.mTlvs, tlvs.mLength,
                                            destAddrSpecified ? &address : nullptr);
    }
    else
    {
        error = OT_ERROR_INVALID_ARGS;
    }

exit:
    return error;
}

void Dataset::OutputSecurityPolicy(const otSecurityPolicy &aSecurityPolicy)
{
    OutputFormat("%u ", aSecurityPolicy.mRotationTime);

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

    OutputLine(" %u", aSecurityPolicy.mVersionThresholdForRouting);
}

otError Dataset::ParseSecurityPolicy(otSecurityPolicy &aSecurityPolicy, Arg *&aArgs)
{
    static constexpr uint8_t kMaxVersionThreshold = 7;

    otError          error;
    otSecurityPolicy policy;
    uint8_t          versionThreshold;

    ClearAllBytes(policy);

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
    VerifyOrExit(!aArgs->IsEmpty());

    SuccessOrExit(error = aArgs->ParseAsUint8(versionThreshold));
    aArgs++;
    VerifyOrExit(versionThreshold <= kMaxVersionThreshold, error = OT_ERROR_INVALID_ARGS);
    policy.mVersionThresholdForRouting = versionThreshold;

exit:
    if (error == OT_ERROR_NONE)
    {
        aSecurityPolicy = policy;
    }

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
    otError                  error = OT_ERROR_NONE;
    otOperationalDatasetTlvs datasetTlvs;

    SuccessOrExit(error = ParseTlvs(aArgs[1], datasetTlvs));

    if (aArgs[0] == "active")
    {
        error = otDatasetSetActiveTlvs(GetInstancePtr(), &datasetTlvs);
    }
    else if (aArgs[0] == "pending")
    {
        error = otDatasetSetPendingTlvs(GetInstancePtr(), &datasetTlvs);
    }
    else
    {
        error = OT_ERROR_INVALID_ARGS;
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

    /**
     * @cli dataset updater
     * @code
     * dataset updater
     * Enabled
     * Done
     * @endcode
     * @par api_copy
     * #otDatasetUpdaterIsUpdateOngoing
     */
    if (aArgs[0].IsEmpty())
    {
        OutputEnabledDisabledStatus(otDatasetUpdaterIsUpdateOngoing(GetInstancePtr()));
    }
    /**
     * @cli dataset updater start
     * @code
     * channel
     * 19
     * Done
     * dataset clear
     * Done
     * dataset channel 15
     * Done
     * dataset
     * Channel: 15
     * Done
     * dataset updater start
     * Done
     * dataset updater
     * Enabled
     * Done
     * Dataset update complete: OK
     * channel
     * 15
     * Done
     * @endcode
     * @par api_copy
     * #otDatasetUpdaterRequestUpdate
     */
    else if (aArgs[0] == "start")
    {
        otOperationalDataset dataset;

        SuccessOrExit(error = otDatasetParseTlvs(&sDatasetTlvs, &dataset));
        SuccessOrExit(
            error = otDatasetUpdaterRequestUpdate(GetInstancePtr(), &dataset, &Dataset::HandleDatasetUpdater, this));
    }
    /**
     * @cli dataset updater cancel
     * @code
     * @dataset updater cancel
     * Done
     * @endcode
     * @par api_copy
     * #otDatasetUpdaterCancelUpdate
     */
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
        CmdEntry("clear"),
        CmdEntry("commit"),
        CmdEntry("init"),
        CmdEntry("mgmtgetcommand"),
        CmdEntry("mgmtsetcommand"),
        CmdEntry("pending"),
        CmdEntry("set"),
        CmdEntry("tlvs"),
#if OPENTHREAD_CONFIG_DATASET_UPDATER_ENABLE && OPENTHREAD_FTD
        CmdEntry("updater"),
#endif
    };

#undef CmdEntry

    static_assert(BinarySearch::IsSorted(kCommands), "kCommands is not sorted");

    otError                error = OT_ERROR_INVALID_COMMAND;
    const Command         *command;
    const ComponentMapper *mapper;

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

    mapper = LookupMapper(aArgs[0].GetCString());

    if (mapper != nullptr)
    {
        error = ProcessCommand(*mapper, aArgs + 1);
        ExitNow();
    }

    command = BinarySearch::Find(aArgs[0].GetCString(), kCommands);
    VerifyOrExit(command != nullptr);

    error = (this->*command->mHandler)(aArgs + 1);

exit:
    return error;
}

} // namespace Cli
} // namespace ot
