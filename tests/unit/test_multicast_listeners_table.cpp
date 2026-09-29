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

#include "openthread-core-config.h"

#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_BACKBONE_ROUTER_MULTICAST_ROUTING_ENABLE

#include "test_platform.h"

#include <openthread/config.h>
#include <openthread/ip6.h>

#include "test_util.h"
#include "backbone_router/multicast_listeners_table.hpp"
#include "common/code_utils.hpp"
#include "instance/instance.hpp"

namespace ot {

using namespace ot::BackboneRouter;

void TestMulticastListenersTable(void)
{
    static constexpr uint16_t kMaxSize = OPENTHREAD_CONFIG_MAX_MULTICAST_LISTENERS;

    MulticastListenersTable *table;
    Instance                *instance;
    Ip6::Address             kMa201;
    Ip6::Address             kMa301;
    Ip6::Address             kMa401;
    Ip6::Address             kMa501;
    TimeMilli                now(0);

    SuccessOrQuit(kMa201.FromString("ff02::01"));
    SuccessOrQuit(kMa301.FromString("ff03::01"));
    SuccessOrQuit(kMa401.FromString("ff04::01"));
    SuccessOrQuit(kMa501.FromString("ff05::01"));

    instance = testInitInstance();
    VerifyOrQuit(instance != nullptr);

    table = &instance->Get<MulticastListenersTable>();

    VerifyOrQuit(table->Count() == 0);

    table->Remove(kMa201);

    VerifyOrQuit(table->Count() == 0);

    SuccessOrQuit(table->Add(kMa401, now));
    SuccessOrQuit(table->Add(kMa501, now));
    VerifyOrQuit(table->Count() == 2);

    // Add invalid MAs should fail with kErrorInvalidArgs
    VerifyOrQuit(table->Add(kMa201, now) == kErrorInvalidArgs);
    VerifyOrQuit(table->Add(kMa301, now) == kErrorInvalidArgs);
    VerifyOrQuit(table->Count() == 2);

    // Re-add an existing entry
    SuccessOrQuit(table->Add(kMa501, now));
    VerifyOrQuit(table->Count() == 2);

    VerifyOrQuit(table->Has(kMa401));
    VerifyOrQuit(table->Has(kMa501));
    VerifyOrQuit(!table->Has(kMa201));
    VerifyOrQuit(!table->Has(kMa301));

    table->Clear();
    VerifyOrQuit(table->Count() == 0);

    for (uint16_t i = 0; i < kMaxSize; i++)
    {
        Ip6::Address address;

        address                = kMa401;
        address.mFields.m16[7] = BigEndian::HostSwap16(i);

        SuccessOrQuit(table->Add(address, now));

        VerifyOrQuit(table->Count() == i + 1);
        VerifyOrQuit(table->Has(address));
    }

    // Now the table is full, adding more entries should fail
    VerifyOrQuit(table->Add(kMa501, now) == kErrorNoBufs);

    VerifyOrQuit(table->Count() == kMaxSize);

    for (uint16_t i = 0; i < kMaxSize; i++)
    {
        Ip6::Address address;

        address                = kMa401;
        address.mFields.m16[7] = BigEndian::HostSwap16(i);

        VerifyOrQuit(table->Has(address));
    }
}

} // namespace ot

int main(void)
{
    ot::TestMulticastListenersTable();
    printf("\nAll tests passed.\n");
    return 0;
}

#else
int main(void) { return 0; }
#endif // OPENTHREAD_FTD && OPENTHREAD_CONFIG_BACKBONE_ROUTER_MULTICAST_ROUTING_ENABLE
