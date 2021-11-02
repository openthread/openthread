/*
 *  Copyright (c) 2021, The OpenThread Authors.
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
 *   This file implements CLI for the SRP Replication (SRPL)
 */

#include "cli_srp_replication.hpp"

#if OPENTHREAD_CONFIG_SRP_REPLICATION_ENABLE

#include <string.h>

#include "cli/cli.hpp"

namespace ot {
namespace Cli {

constexpr SrpReplication::Command SrpReplication::sCommands[];

SrpReplication::SrpReplication(Output &aOutput)
    : OutputWrapper(aOutput)
{
}

otError SrpReplication::Process(Arg aArgs[])
{
    otError        error = OT_ERROR_INVALID_COMMAND;
    const Command *command;

    if (aArgs[0].IsEmpty())
    {
        IgnoreError(ProcessHelp(aArgs));
        ExitNow();
    }

    command = BinarySearch::Find(aArgs[0].GetCString(), sCommands);
    VerifyOrExit(command != nullptr);

    error = (this->*command->mHandler)(aArgs + 1);

exit:
    return error;
}

otError SrpReplication::ProcessHelp(Arg aArgs[])
{
    OT_UNUSED_VARIABLE(aArgs);

    for (const Command &command : sCommands)
    {
        OutputLine(command.mName);
    }

    return OT_ERROR_NONE;
}

otError SrpReplication::ProcessEnable(Arg aArgs[])
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(aArgs[0].IsEmpty(), error = OT_ERROR_INVALID_ARGS);
    error = otSrpReplicationSetEnabled(GetInstancePtr(), /* aEnable */ true);

exit:
    return error;
}

otError SrpReplication::ProcessDisable(Arg aArgs[])
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(aArgs[0].IsEmpty(), error = OT_ERROR_INVALID_ARGS);
    error = otSrpReplicationSetEnabled(GetInstancePtr(), /* aEnable */ false);

exit:
    return error;
}

otError SrpReplication::ProcessState(Arg aArgs[])
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(aArgs[0].IsEmpty(), error = OT_ERROR_INVALID_ARGS);
    OutputEnabledDisabledStatus(otSrpReplicationIsEnabled(GetInstancePtr()));

exit:
    return error;
}

otError SrpReplication::ProcessDomain(Arg aArgs[])
{
    otError error = OT_ERROR_NONE;

    if (aArgs[0].IsEmpty())
    {
        const char *domain = otSrpReplicationGetDomain(GetInstancePtr());

        OutputLine("%s", domain != nullptr ? domain : "(none)");
    }
    else if (aArgs[0] == "clear")
    {
        VerifyOrExit(aArgs[1].IsEmpty(), error = OT_ERROR_INVALID_ARGS);
        error = otSrpReplicationSetDomain(GetInstancePtr(), nullptr);
    }
    else if (aArgs[0] == "set")
    {
        VerifyOrExit(!aArgs[1].IsEmpty() && aArgs[2].IsEmpty(), error = OT_ERROR_INVALID_ARGS);
        error = otSrpReplicationSetDomain(GetInstancePtr(), aArgs[1].GetCString());
    }
    else if (aArgs[0] == "default")
    {
        if (aArgs[1].IsEmpty())
        {
            OutputLine("%s", otSrpReplicationGetDefaultDomain(GetInstancePtr()));
        }
        else
        {
            VerifyOrExit(aArgs[2].IsEmpty(), error = OT_ERROR_INVALID_ARGS);
            error = otSrpReplicationSetDefaultDomain(GetInstancePtr(), aArgs[1].GetCString());
        }
    }
    else
    {
        error = OT_ERROR_INVALID_COMMAND;
    }

exit:
    return error;
}

otError SrpReplication::ProcessId(Arg aArgs[])
{
    otError  error = OT_ERROR_NONE;
    uint32_t id;

    VerifyOrExit(aArgs[0].IsEmpty(), error = OT_ERROR_INVALID_ARGS);

    if (otSrpReplicationGetId(GetInstancePtr(), &id) == OT_ERROR_NONE)
    {
        OutputLine("%u", id);
    }
    else
    {
        OutputLine("(none)");
    }

exit:
    return error;
}

otError SrpReplication::ProcessPartners(Arg aArgs[])
{
    otError                         error = OT_ERROR_NONE;
    otSrpReplicationPartner         partner;
    otSrpReplicationPartnerIterator iterator;
    bool                            isTable = true;

    if (aArgs[0] == "list")
    {
        isTable = false;
        aArgs++;
    }

    VerifyOrExit(aArgs->IsEmpty(), error = OT_ERROR_INVALID_ARGS);

    if (isTable)
    {
        static const char *const kTableTitles[]       = {"Partner SockAddr", "ID", "Session State"};
        static const uint8_t     kTableColumnWidths[] = {50, 12, 18};

        OutputTableHeader(kTableTitles, kTableColumnWidths);
    }

    otSrpReplicationInitPartnerIterator(&iterator);

    while (otSrpReplicationGetNextPartner(GetInstancePtr(), &iterator, &partner) == OT_ERROR_NONE)
    {
        if (isTable)
        {
            char sockAddrString[OT_IP6_SOCK_ADDR_STRING_SIZE];

            otIp6SockAddrToString(&partner.mSockAddr, sockAddrString, sizeof(sockAddrString));

            OutputFormat("| %-48s | ", sockAddrString);

            if (partner.mHasId)
            {
                OutputFormat("%-10u", partner.mId);
            }
            else
            {
                OutputFormat("%-10s", "(none)");
            }

            OutputLine(" | %-16s |", SessionStateToString(partner.mSessionState));
        }
        else
        {
            OutputFormat("sockaddr:");
            OutputSockAddr(partner.mSockAddr);

            if (partner.mHasId)
            {
                OutputFormat(", id:%u", partner.mId);
            }
            else
            {
                OutputFormat(", id:(none)");
            }

            OutputLine(", state:%s", SessionStateToString(partner.mSessionState));
        }
    }

exit:
    return error;
}

const char *SrpReplication::SessionStateToString(otSrpReplicationSessionState aState)
{
    static const char *const kSessionStateStrings[] = {
        "Disconnected",     // (0) OT_SRP_REPLICATION_SESSION_DISCONNECTED
        "Connecting",       // (1) OT_SRP_REPLICATION_SESSION_CONNECTING
        "Establishing",     // (2) OT_SRP_REPLICATION_SESSION_ESTABLISHING
        "InitalSync",       // (3) OT_SRP_REPLICATION_SESSION_INITIAL_SYNC
        "RoutineOperation", // (4) OT_SRP_REPLICATION_SESSION_ROUTINE_OPERATION
        "Errored",          // (5) OT_SRP_REPLICATION_SESSION_ERRORED
    };

    static_assert(0 == OT_SRP_REPLICATION_SESSION_DISCONNECTED, "SSION_DISCONNECTED value is incorrect");
    static_assert(1 == OT_SRP_REPLICATION_SESSION_CONNECTING, "SESSION_CONNECTING value is incorrect");
    static_assert(2 == OT_SRP_REPLICATION_SESSION_ESTABLISHING, "SESSION_ESTABLISHING value is incorrect");
    static_assert(3 == OT_SRP_REPLICATION_SESSION_INITIAL_SYNC, "SESSION_INITIAL_SYNC value is incorrect");
    static_assert(4 == OT_SRP_REPLICATION_SESSION_ROUTINE_OPERATION, "SESSION_ROUTINE_OPERATION value is incorrect");
    static_assert(5 == OT_SRP_REPLICATION_SESSION_ERRORED, "SESSION_RRORED value is incorrect");

    return Stringify(aState, kSessionStateStrings);
}

} // namespace Cli
} // namespace ot

#endif // OPENTHREAD_CONFIG_SRP_REPLICATION_ENABLE
