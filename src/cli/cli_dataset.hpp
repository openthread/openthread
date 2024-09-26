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
 *   This file contains definitions for the CLI interpreter.
 */

#ifndef CLI_DATASET_HPP_
#define CLI_DATASET_HPP_

#include "openthread-core-config.h"

#include <stdarg.h>

#include <openthread/dataset.h>

#include "cli/cli_utils.hpp"

namespace ot {
namespace Cli {

/**
 * Implements the Dataset CLI interpreter.
 */
class Dataset : private Utils
{
public:
    Dataset(otInstance *aInstance, OutputImplementer &aOutputImplementer)
        : Utils(aInstance, aOutputImplementer)
    {
    }

    /**
     * Processes a CLI sub-command.
     *
     * @param[in]  aArgs     An array of command line arguments.
     *
     * @retval OT_ERROR_NONE              Successfully executed the CLI command.
     * @retval OT_ERROR_PENDING           The CLI command was successfully started but final result is pending.
     * @retval OT_ERROR_INVALID_COMMAND   Invalid or unknown CLI command.
     * @retval OT_ERROR_INVALID_ARGS      Invalid arguments.
     * @retval ...                        Error during execution of the CLI command.
     */
    otError Process(Arg aArgs[]);

private:
    using Command    = CommandEntry<Dataset>;
    using Components = otOperationalDatasetComponents;

    struct ComponentMapper
    {
        int Compare(const char *aName) const { return strcmp(aName, mName); }

        constexpr static bool AreInOrder(const ComponentMapper &aFirst, const ComponentMapper &aSecond)
        {
            return AreStringsInOrder(aFirst.mName, aSecond.mName);
        }

        const char *mName;
        bool Components::*mIsPresentPtr;
        void (Dataset::*mOutput)(const otOperationalDataset &aDataset);
        otError (Dataset::*mParse)(Arg *&aArgs, otOperationalDataset &aDataset);
    };

    const ComponentMapper *LookupMapper(const char *aName) const;

    void OutputActiveTimestamp(const otOperationalDataset &aDataset);
    void OutputChannel(const otOperationalDataset &aDataset);
    void OutputWakeupChannel(const otOperationalDataset &aDataset);
    void OutputChannelMask(const otOperationalDataset &aDataset);
    void OutputDelay(const otOperationalDataset &aDataset);
    void OutputExtendedPanId(const otOperationalDataset &aDataset);
    void OutputMeshLocalPrefix(const otOperationalDataset &aDataset);
    void OutputNetworkKey(const otOperationalDataset &aDataset);
    void OutputNetworkName(const otOperationalDataset &aDataset);
    void OutputPanId(const otOperationalDataset &aDataset);
    void OutputPendingTimestamp(const otOperationalDataset &aDataset);
    void OutputPskc(const otOperationalDataset &aDataset);
    void OutputSecurityPolicy(const otOperationalDataset &aDataset);

    otError ParseActiveTimestamp(Arg *&aArgs, otOperationalDataset &aDataset);
    otError ParseChannel(Arg *&aArgs, otOperationalDataset &aDataset);
    otError ParseWakeupChannel(Arg *&aArgs, otOperationalDataset &aDataset);
    otError ParseChannelMask(Arg *&aArgs, otOperationalDataset &aDataset);
    otError ParseDelay(Arg *&aArgs, otOperationalDataset &aDataset);
    otError ParseExtendedPanId(Arg *&aArgs, otOperationalDataset &aDataset);
    otError ParseMeshLocalPrefix(Arg *&aArgs, otOperationalDataset &aDataset);
    otError ParseNetworkKey(Arg *&aArgs, otOperationalDataset &aDataset);
    otError ParseNetworkName(Arg *&aArgs, otOperationalDataset &aDataset);
    otError ParsePanId(Arg *&aArgs, otOperationalDataset &aDataset);
    otError ParsePendingTimestamp(Arg *&aArgs, otOperationalDataset &aDataset);
    otError ParsePskc(Arg *&aArgs, otOperationalDataset &aDataset);
    otError ParseSecurityPolicy(Arg *&aArgs, otOperationalDataset &aDataset);

    otError ParseTlvs(Arg &aArg, otOperationalDatasetTlvs &aDatasetTlvs);

    otError ProcessCommand(const ComponentMapper &aMapper, Arg aArgs[]);

    template <CommandId kCommandId> otError Process(Arg aArgs[]);

    otError Print(otOperationalDatasetTlvs &aDatasetTlvs);

#if OPENTHREAD_CONFIG_DATASET_UPDATER_ENABLE && OPENTHREAD_FTD
    otError     ProcessUpdater(Arg aArgs[]);
    static void HandleDatasetUpdater(otError aError, void *aContext);
    void        HandleDatasetUpdater(otError aError);
#endif

    void    OutputSecurityPolicy(const otSecurityPolicy &aSecurityPolicy);
    otError ParseSecurityPolicy(otSecurityPolicy &aSecurityPolicy, Arg *&aArgs);

    static otOperationalDatasetTlvs sDatasetTlvs;
};

} // namespace Cli
} // namespace ot

#endif // CLI_DATASET_HPP_
