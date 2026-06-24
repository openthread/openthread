/*
 *  Copyright (c) 2026, The OpenThread Authors.
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

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "platform/nexus_core.hpp"
#include "platform/nexus_node.hpp"

namespace ot {
namespace Nexus {

void TestLogOverride(void)
{
    Core  nexus;
    Node &node = nexus.CreateNode();

    Log("---------------------------------------------------------------------------------------");
    Log("TestLogOverride");

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Check initial log level");
    VerifyOrQuit(node.Get<Instance>().GetLogLevel() == kLogLevelCrit);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Check OverrideLogLevel to a higher level");

    node.Get<Instance>().OverrideLogLevel(kLogLevelInfo);
    VerifyOrQuit(node.Get<Instance>().GetLogLevel() == kLogLevelInfo);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Check RestoreLogLevel");

    node.Get<Instance>().RestoreLogLevel();
    VerifyOrQuit(node.Get<Instance>().GetLogLevel() == kLogLevelCrit);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Check OverrideLogLevel with a level lower than the current level");

    SuccessOrQuit(node.SetLogLevel(kLogLevelWarn));
    VerifyOrQuit(node.Get<Instance>().GetLogLevel() == kLogLevelWarn);

    node.Get<Instance>().OverrideLogLevel(kLogLevelCrit);

    VerifyOrQuit(node.Get<Instance>().GetLogLevel() == kLogLevelWarn);

    node.Get<Instance>().RestoreLogLevel();
    VerifyOrQuit(node.Get<Instance>().GetLogLevel() == kLogLevelWarn);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Check user SetLogLevel while an override is active");

    node.Get<Instance>().OverrideLogLevel(kLogLevelInfo);
    VerifyOrQuit(node.Get<Instance>().GetLogLevel() == kLogLevelInfo);

    SuccessOrQuit(node.SetLogLevel(kLogLevelCrit));

    VerifyOrQuit(node.Get<Instance>().GetLogLevel() == kLogLevelInfo);

    node.Get<Instance>().RestoreLogLevel();
    VerifyOrQuit(node.Get<Instance>().GetLogLevel() == kLogLevelCrit);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Check multiple OverrideLogLevel calls");

    node.Get<Instance>().OverrideLogLevel(kLogLevelWarn);
    VerifyOrQuit(node.Get<Instance>().GetLogLevel() == kLogLevelWarn);

    node.Get<Instance>().OverrideLogLevel(kLogLevelInfo);
    VerifyOrQuit(node.Get<Instance>().GetLogLevel() == kLogLevelInfo);

    node.Get<Instance>().RestoreLogLevel();
    VerifyOrQuit(node.Get<Instance>().GetLogLevel() == kLogLevelCrit);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Check OverrideLogLevel and then SetLogLevel to a higher level than override");

    node.Get<Instance>().OverrideLogLevel(kLogLevelWarn);
    VerifyOrQuit(node.Get<Instance>().GetLogLevel() == kLogLevelWarn);

    SuccessOrQuit(node.SetLogLevel(kLogLevelInfo));
    VerifyOrQuit(node.Get<Instance>().GetLogLevel() == kLogLevelInfo);

    node.Get<Instance>().RestoreLogLevel();
    VerifyOrQuit(node.Get<Instance>().GetLogLevel() == kLogLevelInfo);
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::TestLogOverride();
    printf("All tests passed\n");
    return 0;
}
