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
 *   This file includes definitions for the `direct` CLI command group.
 */

#ifndef OT_CLI_CLI_TD_HPP_
#define OT_CLI_CLI_TD_HPP_

#include "openthread-core-config.h"

#if OPENTHREAD_CONFIG_THREAD_DIRECT_WAKE_INITIATOR_ENABLE || OPENTHREAD_CONFIG_THREAD_DIRECT_WAKE_LISTENER_ENABLE

#include <openthread/thread_direct.h>

#include "cli/cli_utils.hpp"

namespace ot {
namespace Cli {

/**
 * Implements the `direct` CLI command group for Thread Direct.
 */
class ThreadDirect : private Utils
{
public:
    ThreadDirect(otInstance *aInstance, OutputImplementer &aOutputImplementer);

    otError Process(Arg aArgs[]);

private:
    using Command = CommandEntry<ThreadDirect>;

    template <CommandId kCommandId> otError Process(Arg aArgs[]);

    otError ProcessHelp(Arg aArgs[]);
    otError ProcessChannel(Arg aArgs[]);
#if OPENTHREAD_CONFIG_THREAD_DIRECT_WAKE_INITIATOR_ENABLE
    otError ProcessWake(Arg aArgs[]);
#endif
#if OPENTHREAD_CONFIG_THREAD_DIRECT_WAKE_LISTENER_ENABLE
    otError ProcessWakeListen(Arg aArgs[]);
#endif
    otError ProcessLink(Arg aArgs[]);
    otError ProcessUnlink(Arg aArgs[]);

    // Helpers dispatched from ProcessLink.
    otError ProcessLinkSlw(Arg aArgs[]);
    otError ProcessLinkRam(Arg aArgs[]);
    otError ProcessLinkState(Arg aArgs[]);
    otError ProcessLinkPeers(Arg aArgs[]);
    otError ProcessLinkTimeout(Arg aArgs[]);
    otError ProcessLinkKey(Arg aArgs[]);
    otError ProcessLinkKeyRemove(Arg aArgs[]);

    static void HandleDirectEvent(otThreadDirectEvent aEvent, const otThreadDirectPeerInfo *aPeerInfo, void *aContext);
    void        HandleDirectEvent(otThreadDirectEvent aEvent, const otThreadDirectPeerInfo *aPeerInfo);

    void OutputResult(otError aError);

    static const Command sCommands[];
};

} // namespace Cli
} // namespace ot

#endif // OPENTHREAD_CONFIG_THREAD_DIRECT_WAKE_INITIATOR_ENABLE || OPENTHREAD_CONFIG_THREAD_DIRECT_WAKE_LISTENER_ENABLE

#endif // OT_CLI_CLI_TD_HPP_
