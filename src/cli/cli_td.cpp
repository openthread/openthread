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
 *   This file implements the `direct` CLI command group for Thread Direct.
 */

#include "cli_td.hpp"

#if OPENTHREAD_CONFIG_THREAD_DIRECT_WAKE_INITIATOR_ENABLE || OPENTHREAD_CONFIG_THREAD_DIRECT_WAKE_LISTENER_ENABLE

#include <openthread/link.h>
#include <openthread/thread_direct.h>

#include "cli/cli.hpp"

namespace ot {
namespace Cli {

// clang-format off
const ThreadDirect::Command ThreadDirect::sCommands[] = {
    {"channel", &ThreadDirect::ProcessChannel},
    {"help",    &ThreadDirect::ProcessHelp},
    {"link",    &ThreadDirect::ProcessLink},
    {"unlink",  &ThreadDirect::ProcessUnlink},
#if OPENTHREAD_CONFIG_THREAD_DIRECT_WAKE_INITIATOR_ENABLE
    {"wake", &ThreadDirect::ProcessWake},
#endif
#if OPENTHREAD_CONFIG_THREAD_DIRECT_WAKE_LISTENER_ENABLE
    {"wakelisten", &ThreadDirect::ProcessWakeListen},
#endif
};
// clang-format on

ThreadDirect::ThreadDirect(otInstance *aInstance, OutputImplementer &aOutputImplementer)
    : Utils(aInstance, aOutputImplementer)
{
}

otError ThreadDirect::ProcessHelp(Arg aArgs[])
{
    OT_UNUSED_VARIABLE(aArgs);

    for (const Command &command : sCommands)
    {
        OutputLine("%s", command.mName);
    }

    return OT_ERROR_NONE;
}

otError ThreadDirect::ProcessChannel(Arg aArgs[])
{
    return ProcessGetSet(aArgs, otLinkGetWakeupChannel, otLinkSetWakeupChannel);
}

#if OPENTHREAD_CONFIG_THREAD_DIRECT_WAKE_INITIATOR_ENABLE
otError ThreadDirect::ProcessWake(Arg aArgs[])
{
    otError                error = OT_ERROR_NONE;
    otExtAddress           extAddress;
    otThreadDirectWakeType wakeType   = OT_THREAD_DIRECT_WAKE_TYPE_LINK;
    uint16_t               intervalUs = 0;
    uint16_t               durationMs = 0;
    uint8_t                keyIndex   = 0;

    SuccessOrExit(error = aArgs[0].ParseAsHexString(extAddress.m8));

    if (!aArgs[1].IsEmpty())
    {
        SuccessOrExit(error = aArgs[1].ParseAsUint8(keyIndex));

        VerifyOrExit(keyIndex == 0 || keyIndex == OT_MAC_FRAME_WAKE_KEY_INDEX ||
                         (keyIndex >= OT_MAC_FRAME_GUEST_WAKE_KEY_INDEX_MIN &&
                          keyIndex <= OT_MAC_FRAME_GUEST_WAKE_KEY_INDEX_MAX),
                     error = OT_ERROR_INVALID_ARGS);

        if (!aArgs[2].IsEmpty())
        {
            uint8_t typeVal;

            SuccessOrExit(error = aArgs[2].ParseAsUint8(typeVal));
            VerifyOrExit(typeVal <= 2, error = OT_ERROR_INVALID_ARGS);
            wakeType = static_cast<otThreadDirectWakeType>(typeVal);
        }

        if (!aArgs[3].IsEmpty())
        {
            SuccessOrExit(error = aArgs[3].ParseAsUint16(intervalUs));
        }

        if (!aArgs[4].IsEmpty())
        {
            SuccessOrExit(error = aArgs[4].ParseAsUint16(durationMs));
        }
    }

    // Register for the duration of the burst; the handler unregisters on the terminal event.
    SuccessOrExit(error =
                      otThreadDirectWakeup(GetInstancePtr(), &extAddress, wakeType, intervalUs, durationMs, keyIndex));
    otThreadDirectSetEventCallback(GetInstancePtr(), HandleDirectEvent, this);
    error = OT_ERROR_PENDING;

exit:
    return error;
}
#endif // OPENTHREAD_CONFIG_THREAD_DIRECT_WAKE_INITIATOR_ENABLE

void ThreadDirect::HandleDirectEvent(otThreadDirectEvent           aEvent,
                                     const otThreadDirectPeerInfo *aPeerInfo,
                                     void                         *aContext)
{
    static_cast<ThreadDirect *>(aContext)->HandleDirectEvent(aEvent, aPeerInfo);
}

void ThreadDirect::HandleDirectEvent(otThreadDirectEvent aEvent, const otThreadDirectPeerInfo *aPeerInfo)
{
    switch (aEvent)
    {
    case OT_THREAD_DIRECT_EVENT_WAKE_RECEIVED:
        if (aPeerInfo != nullptr)
        {
            OutputFormat("TD Wake Command received from: ");
            OutputExtAddress(aPeerInfo->mExtAddress);
            OutputLine(" type: %u rv-time: %lu us retries: %u x %u", aPeerInfo->mWakeType,
                       static_cast<unsigned long>(aPeerInfo->mWakeRvTimeUs), aPeerInfo->mWakeRetryCount,
                       aPeerInfo->mWakeRetryInterval);
        }
        break;

    case OT_THREAD_DIRECT_EVENT_LINK_FAILED:
        otThreadDirectSetEventCallback(GetInstancePtr(), nullptr, nullptr);
        OutputResult(OT_ERROR_FAILED);
        break;
    }
}

#if OPENTHREAD_CONFIG_THREAD_DIRECT_WAKE_LISTENER_ENABLE
otError ThreadDirect::ProcessWakeListen(Arg aArgs[])
{
    return ProcessEnableDisable(aArgs, otThreadDirectIsWakeListenerEnabled, otThreadDirectWakeListenerEnable);
}
#endif // OPENTHREAD_CONFIG_THREAD_DIRECT_WAKE_LISTENER_ENABLE

otError ThreadDirect::ProcessLink(Arg aArgs[])
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(!aArgs[0].IsEmpty(), error = OT_ERROR_INVALID_ARGS);

    if (aArgs[0] == "key")
        error = ProcessLinkKey(aArgs + 1);
    else if (aArgs[0] == "keyremove")
        error = ProcessLinkKeyRemove(aArgs + 1);
    else
    {
        OT_UNUSED_VARIABLE(aArgs);
        error = OT_ERROR_NOT_IMPLEMENTED;
    }

exit:
    return error;
}

otError ThreadDirect::ProcessUnlink(Arg aArgs[])
{
    OT_UNUSED_VARIABLE(aArgs);
    return OT_ERROR_NOT_IMPLEMENTED;
}

otError ThreadDirect::ProcessLinkKey(Arg aArgs[])
{
    otError               error = OT_ERROR_NONE;
    uint8_t               keyIndex;
    otThreadDirectWakeKey key;

    SuccessOrExit(error = aArgs[0].ParseAsUint8(keyIndex));
    VerifyOrExit(keyIndex >= OT_MAC_FRAME_GUEST_WAKE_KEY_INDEX_MIN && keyIndex <= OT_MAC_FRAME_GUEST_WAKE_KEY_INDEX_MAX,
                 error = OT_ERROR_INVALID_ARGS);
    SuccessOrExit(error = aArgs[1].ParseAsHexString(key.m8));

    error = otThreadDirectSetGuestWakeKey(GetInstancePtr(), keyIndex, &key);

exit:
    return error;
}

otError ThreadDirect::ProcessLinkKeyRemove(Arg aArgs[])
{
    otError error = OT_ERROR_NONE;
    uint8_t keyIndex;

    SuccessOrExit(error = aArgs[0].ParseAsUint8(keyIndex));
    VerifyOrExit(keyIndex >= OT_MAC_FRAME_GUEST_WAKE_KEY_INDEX_MIN && keyIndex <= OT_MAC_FRAME_GUEST_WAKE_KEY_INDEX_MAX,
                 error = OT_ERROR_INVALID_ARGS);

    error = otThreadDirectRemoveGuestWakeKey(GetInstancePtr(), keyIndex);

exit:
    return error;
}

otError ThreadDirect::Process(Arg aArgs[])
{
    otError        error = OT_ERROR_INVALID_COMMAND;
    const Command *command;

    if (aArgs[0].IsEmpty() || (strcmp(aArgs[0].GetCString(), "help") == 0))
    {
        IgnoreError(ProcessHelp(aArgs));
        ExitNow(error = OT_ERROR_NONE);
    }

    command = BinarySearch::Find(aArgs[0].GetCString(), sCommands);

    VerifyOrExit(command != nullptr);
    error = (this->*command->mHandler)(aArgs + 1);

exit:
    return error;
}

void ThreadDirect::OutputResult(otError aError) { Interpreter::GetInterpreter().OutputResult(aError); }

} // namespace Cli
} // namespace ot

#endif // OPENTHREAD_CONFIG_THREAD_DIRECT_WAKE_INITIATOR_ENABLE || OPENTHREAD_CONFIG_THREAD_DIRECT_WAKE_LISTENER_ENABLE
