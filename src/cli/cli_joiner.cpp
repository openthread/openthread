/*
 *  Copyright (c) 2019, The OpenThread Authors.
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
 *   This file implements a simple CLI for the Joiner role.
 */

#include "cli_joiner.hpp"

#include <inttypes.h>

#include "cli/cli.hpp"

#if OPENTHREAD_CONFIG_JOINER_ENABLE

namespace ot {
namespace Cli {

template <> otError Joiner::Process<Cmd("discerner")>(Arg aArgs[])
{
    otError error = OT_ERROR_INVALID_ARGS;

    /**
     * @cli joiner discerner
     * @code
     * joiner discerner
     * 0xabc/12
     * Done
     * @endcode
     * @par api_copy
     * #otJoinerGetDiscerner
     */
    if (aArgs[0].IsEmpty())
    {
        const otJoinerDiscerner *discerner = otJoinerGetDiscerner(GetInstancePtr());

        VerifyOrExit(discerner != nullptr, error = OT_ERROR_NOT_FOUND);

        if (discerner->mValue <= 0xffffffff)
        {
            OutputLine("0x%lx/%u", static_cast<unsigned long>(discerner->mValue & 0xffffffff), discerner->mLength);
        }
        else
        {
            OutputLine("0x%lx%08lx/%u", static_cast<unsigned long>(discerner->mValue >> 32),
                       static_cast<unsigned long>(discerner->mValue & 0xffffffff), discerner->mLength);
        }

        error = OT_ERROR_NONE;
    }
    else
    {
        otJoinerDiscerner discerner;

        memset(&discerner, 0, sizeof(discerner));

        /**
         * @cli joiner discerner clear
         * @code
         * joiner discerner clear
         * Done
         * @endcode
         * @par
         * Clear the %Joiner discerner.
         */
        if (aArgs[0] == "clear")
        {
            error = otJoinerSetDiscerner(GetInstancePtr(), nullptr);
        }
        /**
         * @cli joiner discerner (set)
         * @code
         * joiner discerner 0xabc/12
         * Done
         * @endcode
         * @cparam joiner discerner @ca{discerner}
         * *   Use `{number}/{length}` to set the `discerner`.
         * *   `joiner discerner clear` sets `aDiscerner` to `nullptr`.
         * @par api_copy
         * #otJoinerSetDiscerner
         */
        else
        {
            VerifyOrExit(aArgs[1].IsEmpty());
            SuccessOrExit(Interpreter::ParseJoinerDiscerner(aArgs[0], discerner));
            error = otJoinerSetDiscerner(GetInstancePtr(), &discerner);
        }
    }

exit:
    return error;
}

/**
 * @cli joiner id
 * @code
 * joiner id
 * d65e64fa83f81cf7
 * Done
 * @endcode
 * @par api_copy
 * #otJoinerGetId
 */
template <> otError Joiner::Process<Cmd("id")>(Arg aArgs[])
{
    OT_UNUSED_VARIABLE(aArgs);

    OutputExtAddressLine(*otJoinerGetId(GetInstancePtr()));

    return OT_ERROR_NONE;
}

/**
 * @cli joiner start
 * @code
 * joiner start J01NM3
 * Done
 * @endcode
 * @cparam joiner start @ca{joining-device-credential} [@ca{provisioning-url}]
 * *   `joining-device-credential`: %Joiner Passphrase. Must be a string of all uppercase alphanumeric
 *     characters (0-9 and A-Y, excluding I, O, Q, and Z for readability), with a length between 6 and
 *     32 characters.
 * *   `provisioning-url`: Provisioning URL for the %Joiner (optional).
 * @par api_copy
 * #otJoinerStart
 */
template <> otError Joiner::Process<Cmd("start")>(Arg aArgs[])
{
    otError error;

    VerifyOrExit(!aArgs[0].IsEmpty(), error = OT_ERROR_INVALID_ARGS);

    error = otJoinerStart(GetInstancePtr(),
                          aArgs[0].GetCString(),           // aPskd
                          aArgs[1].GetCString(),           // aProvisioningUrl (`nullptr` if aArgs[1] is empty)
                          PACKAGE_NAME,                    // aVendorName
                          OPENTHREAD_CONFIG_PLATFORM_INFO, // aVendorModel
                          PACKAGE_VERSION,                 // aVendorSwVersion
                          nullptr,                         // aVendorData
                          &Joiner::HandleCallback, this);

exit:
    return error;
}

/**
 * @cli joiner stop
 * @code
 * joiner stop
 * Done
 * @endcode
 * @par api_copy
 * #otJoinerStop
 */
template <> otError Joiner::Process<Cmd("stop")>(Arg aArgs[])
{
    OT_UNUSED_VARIABLE(aArgs);

    otJoinerStop(GetInstancePtr());

    return OT_ERROR_NONE;
}

/**
 * @cli joiner state
 * @code
 * joiner state
 * Idle
 * Done
 * @endcode
 * @par api_copy
 * #otJoinerGetState
 * @par
 * Returns one of the following states:
 * *   `Idle`
 * *   `Discover`
 * *   `Connecting`
 * *   `Connected`
 * *   `Entrust`
 * *   `Joined`
 */
template <> otError Joiner::Process<Cmd("state")>(Arg aArgs[])
{
    OT_UNUSED_VARIABLE(aArgs);

    OutputLine("%s", otJoinerStateToString(otJoinerGetState(GetInstancePtr())));

    return OT_ERROR_NONE;
}

otError Joiner::Process(Arg aArgs[])
{
#define CmdEntry(aCommandString)                              \
    {                                                         \
        aCommandString, &Joiner::Process<Cmd(aCommandString)> \
    }

    static constexpr Command kCommands[] = {
        CmdEntry("discerner"), CmdEntry("id"), CmdEntry("start"), CmdEntry("state"), CmdEntry("stop"),
    };

#undef CmdEntry

    static_assert(BinarySearch::IsSorted(kCommands), "kCommands is not sorted");

    otError        error = OT_ERROR_INVALID_COMMAND;
    const Command *command;

    /**
     * @cli joiner help
     * @code
     * joiner help
     * help
     * id
     * start
     * state
     * stop
     * Done
     * @endcode
     * @par
     * Print the `joiner` help menu.
     */
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

void Joiner::HandleCallback(otError aError, void *aContext) { static_cast<Joiner *>(aContext)->HandleCallback(aError); }

void Joiner::HandleCallback(otError aError)
{
    switch (aError)
    {
    case OT_ERROR_NONE:
        OutputLine("Join success");
        break;

    default:
        OutputLine("Join failed [%s]", otThreadErrorToString(aError));
        break;
    }
}

} // namespace Cli
} // namespace ot

#endif // OPENTHREAD_CONFIG_JOINER_ENABLE
