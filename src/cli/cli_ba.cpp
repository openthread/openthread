/*
 *  Copyright (c) 2025, The OpenThread Authors.
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
 *   This file implements CLI for Border Agent.
 */

#include "cli_ba.hpp"

#if OPENTHREAD_CONFIG_BORDER_AGENT_ENABLE

#include <string.h>

#include "cli/cli.hpp"

namespace ot {
namespace Cli {

/**
 * @cli ba enable
 * @code
 * ba enable
 * Done
 * @endcode
 * @par api_copy
 * #otBorderAgentSetEnabled
 */
template <> otError Ba::Process<Cmd("enable")>(Arg aArgs[])
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(aArgs[0].IsEmpty(), error = OT_ERROR_INVALID_ARGS);
    otBorderAgentSetEnabled(GetInstancePtr(), true);

exit:
    return error;
}

/**
 * @cli ba disable
 * @code
 * ba disable
 * Done
 * @endcode
 * @par api_copy
 * #otBorderAgentSetEnabled
 */
template <> otError Ba::Process<Cmd("disable")>(Arg aArgs[])
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(aArgs[0].IsEmpty(), error = OT_ERROR_INVALID_ARGS);
    otBorderAgentSetEnabled(GetInstancePtr(), false);

exit:
    return error;
}

/**
 * @cli ba port
 * @code
 * ba port
 * 49153
 * Done
 * @endcode
 * @par api_copy
 * #otBorderAgentGetUdpPort
 */
template <> otError Ba::Process<Cmd("port")>(Arg aArgs[])
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(aArgs[0].IsEmpty(), error = OT_ERROR_INVALID_ARGS);
    OutputLine("%u", otBorderAgentGetUdpPort(GetInstancePtr()));

exit:
    return error;
}

/**
 * @cli ba state
 * @code
 * ba state
 * Active
 * Done
 * @endcode
 * @par
 * Prints the current state of the Border Agent service. Possible states are:
 * - `Disabled`: Border Agent service is disabled.
 * - `Inactive`: Border Agent service is enabled but not yet active.
 * - `Active`: Border Agent service is enabled and active. External commissioner can connect and establish secure
 *   DTLS sessions with the Border Agent using PSKc
 * @sa #otBorderAgentIsActive
 */
template <> otError Ba::Process<Cmd("state")>(Arg aArgs[])
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(aArgs[0].IsEmpty(), error = OT_ERROR_INVALID_ARGS);

    if (!otBorderAgentIsEnabled(GetInstancePtr()))
    {
        OutputLine("Disabled");
    }
    else
    {
        OutputLine("%s", otBorderAgentIsActive(GetInstancePtr()) ? "Active" : "Inactive");
    }

exit:
    return error;
}

/**
 * @cli ba sessions
 * @code
 * ba sessions
 * [fe80:0:0:0:cc79:2a29:d311:1aea]:9202 connected:yes commissioner:no lifetime:1860
 * Done
 * @endcode
 * @par
 * Prints the list of Border Agent's sessions. Information per session:
 * * Peer socket address (IPv6 address and port).
 * * Whether or not the session is connected.
 * * Whether or not the session is accepted as full commissioner.
 * * Session lifetime in milliseconds (calculated from the time the session was first established).
 */
template <> otError Ba::Process<Cmd("sessions")>(Arg aArgs[])
{
    otError                      error = OT_ERROR_NONE;
    otBorderAgentSessionIterator iterator;
    otBorderAgentSessionInfo     info;
    char                         sockAddrString[OT_IP6_SOCK_ADDR_STRING_SIZE];
    Uint64StringBuffer           lifetimeString;

    VerifyOrExit(aArgs[0].IsEmpty(), error = OT_ERROR_INVALID_ARGS);

    otBorderAgentInitSessionIterator(GetInstancePtr(), &iterator);

    while (otBorderAgentGetNextSessionInfo(&iterator, &info) == OT_ERROR_NONE)
    {
        otIp6SockAddrToString(&info.mPeerSockAddr, sockAddrString, sizeof(sockAddrString));

        OutputLine("%s connected:%s commissioner:%s lifetime:%s", sockAddrString, info.mIsConnected ? "yes" : "no",
                   info.mIsCommissioner ? "yes" : "no", Uint64ToString(info.mLifetime, lifetimeString));
    }
exit:
    return error;
}

/**
 * @cli ba counters
 * @code
 * ba counters
 * epskcActivation: 0
 * epskcApiDeactivation: 0
 * epskcTimeoutDeactivation: 0
 * epskcMaxAttemptDeactivation: 0
 * epskcDisconnectDeactivation: 0
 * epskcInvalidBaStateError: 0
 * epskcInvalidArgsError: 0
 * epskcStartSecureSessionError: 0
 * epskcSecureSessionSuccess: 0
 * epskcSecureSessionFailure: 0
 * epskcCommissionerPetition: 0
 * pskcSecureSessionSuccess: 0
 * pskcSecureSessionFailure: 0
 * pskcCommissionerPetition: 0
 * mgmtActiveGet: 0
 * mgmtPendingGet: 0
 * Done
 * @endcode
 * @par
 * Gets the border agent counters.
 * @sa otBorderAgentGetCounters
 */
template <> otError Ba::Process<Cmd("counters")>(Arg aArgs[])
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(aArgs[0].IsEmpty(), error = OT_ERROR_INVALID_ARGS);
    OutputBorderAgentCounters(*otBorderAgentGetCounters(GetInstancePtr()));

exit:
    return error;
}

void Ba::OutputBorderAgentCounters(const otBorderAgentCounters &aCounters)
{
    struct CounterName
    {
        const uint32_t otBorderAgentCounters::*mValuePtr;
        const char                            *mName;
    };

    static const CounterName kCounterNames[] = {
#if OPENTHREAD_CONFIG_BORDER_AGENT_EPHEMERAL_KEY_ENABLE
        {&otBorderAgentCounters::mEpskcActivations, "epskcActivation"},
        {&otBorderAgentCounters::mEpskcDeactivationClears, "epskcApiDeactivation"},
        {&otBorderAgentCounters::mEpskcDeactivationTimeouts, "epskcTimeoutDeactivation"},
        {&otBorderAgentCounters::mEpskcDeactivationMaxAttempts, "epskcMaxAttemptDeactivation"},
        {&otBorderAgentCounters::mEpskcDeactivationDisconnects, "epskcDisconnectDeactivation"},
        {&otBorderAgentCounters::mEpskcInvalidBaStateErrors, "epskcInvalidBaStateError"},
        {&otBorderAgentCounters::mEpskcInvalidArgsErrors, "epskcInvalidArgsError"},
        {&otBorderAgentCounters::mEpskcStartSecureSessionErrors, "epskcStartSecureSessionError"},
        {&otBorderAgentCounters::mEpskcSecureSessionSuccesses, "epskcSecureSessionSuccess"},
        {&otBorderAgentCounters::mEpskcSecureSessionFailures, "epskcSecureSessionFailure"},
        {&otBorderAgentCounters::mEpskcCommissionerPetitions, "epskcCommissionerPetition"},
#endif
        {&otBorderAgentCounters::mPskcSecureSessionSuccesses, "pskcSecureSessionSuccess"},
        {&otBorderAgentCounters::mPskcSecureSessionFailures, "pskcSecureSessionFailure"},
        {&otBorderAgentCounters::mPskcCommissionerPetitions, "pskcCommissionerPetition"},
        {&otBorderAgentCounters::mMgmtActiveGets, "mgmtActiveGet"},
        {&otBorderAgentCounters::mMgmtPendingGets, "mgmtPendingGet"},
    };

    for (const CounterName &counter : kCounterNames)
    {
        OutputLine("%s: %lu", counter.mName, ToUlong(aCounters.*counter.mValuePtr));
    }
}

#if OPENTHREAD_CONFIG_BORDER_AGENT_MESHCOP_SERVICE_ENABLE
/**
 * @cli ba servicebasename
 * @code
 * ba servicebasename OpenThreadBorderAgent
 * Done
 * @endcode
 * @par api_copy
 * #otBorderAgentSetMeshCoPServiceBaseName
 */
template <> otError Ba::Process<Cmd("servicebasename")>(Arg aArgs[])
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(!aArgs[0].IsEmpty() && aArgs[1].IsEmpty(), error = OT_ERROR_INVALID_ARGS);
    error = otBorderAgentSetMeshCoPServiceBaseName(GetInstancePtr(), aArgs[0].GetCString());

exit:
    return error;
}
#endif

#if OPENTHREAD_CONFIG_BORDER_AGENT_ID_ENABLE
/**
 * @cli ba id (get,set)
 * @code
 * ba id
 * cb6da1e0c0448aaec39fa90f3d58f45c
 * Done
 * @endcode
 * @code
 * ba id 00112233445566778899aabbccddeeff
 * Done
 * @endcode
 * @cparam ba id [@ca{border-agent-id}]
 * Use the optional `border-agent-id` argument to set the Border Agent ID.
 * @par
 * Gets or sets the 16 bytes Border Router ID which can uniquely identifies the device among multiple BRs.
 * @sa otBorderAgentGetId
 * @sa otBorderAgentSetId
 */
template <> otError Ba::Process<Cmd("id")>(Arg aArgs[])
{
    otError         error = OT_ERROR_NONE;
    otBorderAgentId id;

    if (aArgs[0].IsEmpty())
    {
        SuccessOrExit(error = otBorderAgentGetId(GetInstancePtr(), &id));
        OutputBytesLine(id.mId);
    }
    else
    {
        uint16_t idLength = sizeof(id);

        SuccessOrExit(error = aArgs[0].ParseAsHexString(idLength, id.mId));
        VerifyOrExit(idLength == sizeof(id), error = OT_ERROR_INVALID_ARGS);
        VerifyOrExit(aArgs[1].IsEmpty(), error = OT_ERROR_INVALID_ARGS);

        error = otBorderAgentSetId(GetInstancePtr(), &id);
    }

exit:
    return error;
}
#endif // OPENTHREAD_CONFIG_BORDER_AGENT_ID_ENABLE

#if OPENTHREAD_CONFIG_BORDER_AGENT_COMMISSIONER_EVICTION_API_ENABLE
/**
 * @cli ba evictcommissioner
 * @code
 * ba evictcommissioner
 * Done
 * @endcode
 * @par api_copy
 * #otBorderAgentEvictActiveCommissioner
 */
template <> otError Ba::Process<Cmd("evictcommissioner")>(Arg aArgs[])
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(aArgs[0].IsEmpty(), error = OT_ERROR_INVALID_ARGS);
    error = otBorderAgentEvictActiveCommissioner(GetInstancePtr());

exit:
    return error;
}
#endif

#if OPENTHREAD_CONFIG_BORDER_AGENT_EPHEMERAL_KEY_ENABLE

template <> otError Ba::Process<Cmd("ephemeralkey")>(Arg aArgs[])
{
    otError error = OT_ERROR_NONE;
    bool    enable;

    /**
     * @cli ba ephemeralkey
     * @code
     * ba ephemeralkey
     * Stopped
     * Done
     * @endcode
     * @par api_copy
     * #otBorderAgentEphemeralKeyGetState
     */
    if (aArgs[0].IsEmpty())
    {
        otBorderAgentEphemeralKeyState state = otBorderAgentEphemeralKeyGetState(GetInstancePtr());

        OutputLine("%s", otBorderAgentEphemeralKeyStateToString(state));
    }
    /**
     * @cli ba ephemeralkey (enable, disable)
     * @code
     * ba ephemeralkey enable
     * Done
     * @endcode
     * @code
     * ba ephemeralkey
     * Enabled
     * Done
     * @endcode
     * @cparam ba ephemeralkey @ca{enable|disable}
     * @par api_copy
     * #otBorderAgentEphemeralKeySetEnabled
     */
    else if (ProcessEnableDisable(aArgs, otBorderAgentEphemeralKeySetEnabled) == OT_ERROR_NONE)
    {
    }
    /**
     * @cli ba ephemeralkey start <keystring> [timeout-in-msec] [port]
     * @code
     * ba ephemeralkey start Z10X20g3J15w1000P60m16 5000 1234
     * Done
     * @endcode
     * @cparam ba ephemeralkey start @ca{keystring} [@ca{timeout-in-msec}] [@ca{port}]
     * @par api_copy
     * #otBorderAgentEphemeralKeyStart
     */
    else if (aArgs[0] == "start")
    {
        uint32_t timeout = 0;
        uint16_t port    = 0;

        VerifyOrExit(!aArgs[1].IsEmpty(), error = OT_ERROR_INVALID_ARGS);

        if (!aArgs[2].IsEmpty())
        {
            SuccessOrExit(error = aArgs[2].ParseAsUint32(timeout));
        }

        if (!aArgs[3].IsEmpty())
        {
            SuccessOrExit(error = aArgs[3].ParseAsUint16(port));

            VerifyOrExit(aArgs[4].IsEmpty(), error = OT_ERROR_INVALID_ARGS);
        }

        error = otBorderAgentEphemeralKeyStart(GetInstancePtr(), aArgs[1].GetCString(), timeout, port);
    }
    /**
     * @cli ba ephemeralkey stop
     * @code
     * ba ephemeralkey stop
     * Done
     * @endcode
     * @par api_copy
     * #otBorderAgentEphemeralKeyStop
     */
    else if (aArgs[0] == "stop")
    {
        VerifyOrExit(aArgs[1].IsEmpty(), error = OT_ERROR_INVALID_ARGS);

        otBorderAgentEphemeralKeyStop(GetInstancePtr());
    }
    /**
     * @cli ba ephemeralkey port
     * @code
     * ba ephemeralkey port
     * 49153
     * Done
     * @endcode
     * @par api_copy
     * #otBorderAgentEphemeralKeyGetUdpPort
     */
    else if (aArgs[0] == "port")
    {
        VerifyOrExit(aArgs[1].IsEmpty(), error = OT_ERROR_INVALID_ARGS);

        OutputLine("%u", otBorderAgentEphemeralKeyGetUdpPort(GetInstancePtr()));
    }
    /**
     * @cli ba ephemeralkey callback (enable, disable)
     * @code
     * ba ephemeralkey callback enable
     * Done
     * @endcode
     * @code
     * ba ephemeralkey start W10X10 5000 49155
     * Done
     * BorderAgentEphemeralKey callback - state:Started
     * BorderAgentEphemeralKey callback - state:Connected
     * @endcode
     * @cparam ba ephemeralkey callback @ca{enable|disable}
     * @par api_copy
     * #otBorderAgentEphemeralKeySetCallback
     */
    else if (aArgs[0] == "callback")
    {
        SuccessOrExit(error = ParseEnableOrDisable(aArgs[1], enable));

        VerifyOrExit(aArgs[2].IsEmpty(), error = OT_ERROR_INVALID_ARGS);

        if (enable)
        {
            otBorderAgentEphemeralKeySetCallback(GetInstancePtr(), HandleBorderAgentEphemeralKeyStateChange, this);
        }
        else
        {
            otBorderAgentEphemeralKeySetCallback(GetInstancePtr(), nullptr, nullptr);
        }
    }
    else
    {
        error = OT_ERROR_INVALID_ARGS;
    }

exit:
    return error;
}

void Ba::HandleBorderAgentEphemeralKeyStateChange(void *aContext)
{
    reinterpret_cast<Ba *>(aContext)->HandleBorderAgentEphemeralKeyStateChange();
}

void Ba::HandleBorderAgentEphemeralKeyStateChange(void)
{
    otBorderAgentEphemeralKeyState state = otBorderAgentEphemeralKeyGetState(GetInstancePtr());

    OutputLine("BorderAgentEphemeralKey callback - state:%s", otBorderAgentEphemeralKeyStateToString(state));
}

#endif // OPENTHREAD_CONFIG_BORDER_AGENT_EPHEMERAL_KEY_ENABLE

otError Ba::Process(Arg aArgs[])
{
#define CmdEntry(aCommandString) {aCommandString, &Ba::Process<Cmd(aCommandString)>}

    static constexpr Command kCommands[] = {
        CmdEntry("counters"),
        CmdEntry("disable"),
        CmdEntry("enable"),
#if OPENTHREAD_CONFIG_BORDER_AGENT_EPHEMERAL_KEY_ENABLE
        CmdEntry("ephemeralkey"),
#endif
#if OPENTHREAD_CONFIG_BORDER_AGENT_COMMISSIONER_EVICTION_API_ENABLE
        CmdEntry("evictcommissioner"),
#endif
#if OPENTHREAD_CONFIG_BORDER_AGENT_ID_ENABLE
        CmdEntry("id"),
#endif
        CmdEntry("port"),
#if OPENTHREAD_CONFIG_BORDER_AGENT_MESHCOP_SERVICE_ENABLE
        CmdEntry("servicebasename"),
#endif
        CmdEntry("sessions"),
        CmdEntry("state"),
    };

#undef CmdEntry

    static_assert(BinarySearch::IsSorted(kCommands), "kCommands is not sorted");

    otError        error = OT_ERROR_INVALID_COMMAND;
    const Command *command;

    if (aArgs[0].IsEmpty() || (aArgs[0] == "help"))
    {
        OutputCommandTable(kCommands);
        ExitNow(error = aArgs[0].IsEmpty() ? error : OT_ERROR_NONE);
    }

    command = BinarySearch::Find(aArgs[0].GetCString(), kCommands);
    VerifyOrExit(command != nullptr);

    error = (this->*command->mHandler)(aArgs + 1);

exit:
    return error;
}

} // namespace Cli
} // namespace ot

#endif // OPENTHREAD_CONFIG_BORDER_AGENT_ENABLE
