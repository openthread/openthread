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

#include "test_platform.h"

#include <openthread/config.h>
#include <openthread/instance.h>

#include "test_util.h"
#include "instance/instance.hpp"

namespace ot {

#if !OPENTHREAD_CONFIG_MULTIPLE_INSTANCE_ENABLE || OPENTHREAD_CONFIG_MULTIPLE_STATIC_INSTANCE_ENABLE

static otInstance *InitStaticInstance(void)
{
#if OPENTHREAD_CONFIG_MULTIPLE_INSTANCE_ENABLE
    return otInstanceInitMultiple(0);
#else
    return otInstanceInitSingle();
#endif
}

void TestStaticInstanceLifecycle(void)
{
    otInstance *instance;
    uint32_t    id;

    // Repeated init returns the existing instance without re-constructing it.
    // The ID is randomly generated on construction, so it must stay the same.

    instance = InitStaticInstance();
    VerifyOrQuit(instance != nullptr);
    id = otInstanceGetId(instance);

    VerifyOrQuit(InitStaticInstance() == instance);
    VerifyOrQuit(otInstanceGetId(instance) == id);

    // Repeated finalize must be a no-op instead of destructing twice.

    otInstanceFinalize(instance);
    otInstanceFinalize(instance);

    // The instance can be initialized again after being finalized.

    instance = InitStaticInstance();
    VerifyOrQuit(instance != nullptr);
    VerifyOrQuit(InitStaticInstance() == instance);

    otInstanceFinalize(instance);

    printf("TestStaticInstanceLifecycle() passed\n");
}

#endif

} // namespace ot

int main(void)
{
#if !OPENTHREAD_CONFIG_MULTIPLE_INSTANCE_ENABLE || OPENTHREAD_CONFIG_MULTIPLE_STATIC_INSTANCE_ENABLE
    ot::TestStaticInstanceLifecycle();
#endif
    printf("\nAll tests passed.\n");
    return 0;
}
