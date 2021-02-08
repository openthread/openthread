/*
 *  Copyright (c) 2020, The OpenThread Authors.
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
 *   This file contains definitions for a simple CLI to control SRP Client.
 */

#ifndef CLI_SRP_CLIENT_HPP_
#define CLI_SRP_CLIENT_HPP_

#include "openthread-core-config.h"

#include <openthread/srp_client.h>

#include "cli/cli_config.h"
#include "utils/lookup_table.hpp"

#if OPENTHREAD_CONFIG_SRP_CLIENT_ENABLE

namespace ot {
namespace Cli {

class Interpreter;

/**
 * This class implements the SRP Client CLI interpreter.
 *
 */
class SrpClient
{
public:
    /**
     * Constructor
     *
     * @param[in]  aInterpreter  The CLI interpreter.
     *
     */
    explicit SrpClient(Interpreter &aInterpreter);

    /**
     * This method interprets a list of CLI arguments.
     *
     * @param[in]  aArgsLength  The number of elements in @p aArgs.
     * @param[in]  aArgs        A pointer to an array of command line arguments.
     *
     */
    otError Process(uint8_t aArgsLength, char *aArgs[]);

private:
    enum : uint8_t
    {
        kMaxServices      = OPENTHREAD_CONFIG_CLI_SRP_CLIENT_MAX_SERVICES,
        kMaxHostAddresses = OPENTHREAD_CONFIG_CLI_SRP_CLIENT_MAX_HOST_ADDRESSES,
        kNameSize         = 64,
        kTxtSize          = 255,
        kIndentSize       = 4,
    };

    struct Service
    {
        void MarkAsNotInUse(void) { mService.mNext = &mService; }
        bool IsInUse(void) const { return (mService.mNext != &mService); }

        otSrpClientService mService;
        otDnsTxtEntry      mTxtEntry;
        char               mInstanceName[kNameSize];
        char               mServiceName[kNameSize];
        uint8_t            mTxtBuffer[kTxtSize];
    };

    struct Command
    {
        const char *mName;
        otError (SrpClient::*mHandler)(uint8_t aArgsLength, char *aArgs[]);
    };

    otError ProcessCallback(uint8_t aArgsLength, char *aArgs[]);
    otError ProcessHelp(uint8_t aArgsLength, char *aArgs[]);
    otError ProcessHost(uint8_t aArgsLength, char *aArgs[]);
    otError ProcessLeaseInterval(uint8_t aArgsLength, char *aArgs[]);
    otError ProcessKeyLeaseInterval(uint8_t aArgsLength, char *aArgs[]);
    otError ProcessServer(uint8_t aArgsLength, char *aArgs[]);
    otError ProcessService(uint8_t aArgsLength, char *aArgs[]);
    otError ProcessStart(uint8_t aArgsLength, char *aArgs[]);
    otError ProcessStop(uint8_t aArgsLength, char *aArgs[]);

    void OutputHostInfo(uint8_t aIndentSize, const otSrpClientHostInfo &aHostInfo);
    void OutputServiceList(uint8_t aIndentSize, const otSrpClientService *aServices);
    void OutputService(uint8_t aIndentSize, const otSrpClientService &aService);

    static void HandleCallback(otError                    aError,
                               const otSrpClientHostInfo *aHostInfo,
                               const otSrpClientService * aServices,
                               const otSrpClientService * aRemovedServices,
                               void *                     aContext);
    void        HandleCallback(otError                    aError,
                               const otSrpClientHostInfo *aHostInfo,
                               const otSrpClientService * aServices,
                               const otSrpClientService * aRemovedServices);

    static constexpr Command sCommands[] = {
        {"callback", &SrpClient::ProcessCallback},
        {"help", &SrpClient::ProcessHelp},
        {"host", &SrpClient::ProcessHost},
        {"keyleaseinterval", &SrpClient::ProcessKeyLeaseInterval},
        {"leaseinterval", &SrpClient::ProcessLeaseInterval},
        {"server", &SrpClient::ProcessServer},
        {"service", &SrpClient::ProcessService},
        {"start", &SrpClient::ProcessStart},
        {"stop", &SrpClient::ProcessStop},
    };

    static_assert(Utils::LookupTable::IsSorted(sCommands), "Command Table is not sorted");

    Interpreter &mInterpreter;
    bool         mCallbackEnabled;
    char         mHostName[kNameSize];
    otIp6Address mHostAddresses[kMaxHostAddresses];
    Service      mServicePool[kMaxServices];
};

} // namespace Cli
} // namespace ot

#endif // OPENTHREAD_CONFIG_SRP_CLIENT_ENABLE

#endif // CLI_SRP_CLIENT_HPP_
