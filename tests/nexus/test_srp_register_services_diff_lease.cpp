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

#include <stdio.h>
#include <string.h>

#include "common/string.hpp"
#include "platform/nexus_core.hpp"
#include "platform/nexus_node.hpp"

#include <openthread/srp_server.h>

namespace ot {
namespace Nexus {

void TestSrpRegisterServicesDiffLease(void)
{
    Core  nexus;
    Node &server = nexus.CreateNode();
    Node &client = nexus.CreateNode();

    server.SetName("SRP_SERVER");
    client.SetName("SRP_CLIENT");

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelNote));

    server.Form();
    nexus.AdvanceTime(10000);
    VerifyOrQuit(server.Get<Mle::Mle>().IsLeader());

    client.Join(server, Node::kAsFtd);
    nexus.AdvanceTime(200000);
    VerifyOrQuit(client.Get<Mle::Mle>().IsRouter());

    server.Get<Srp::Server>().SetEnabled(true);
    client.Get<Srp::Client>().EnableAutoStartMode(nullptr, nullptr);

    nexus.AdvanceTime(15000);

    SuccessOrQuit(client.Get<Srp::Client>().SetHostName("host"));
    SuccessOrQuit(client.Get<Srp::Client>().EnableAutoHostAddress());

    // Add a service with specific lease and key lease
    Srp::Client::Service ins1;
    ClearAllBytes(ins1);
    ins1.mName         = "_test._udp";
    ins1.mInstanceName = "ins1";
    ins1.mPort         = 1111;
    ins1.mLease        = 60;
    ins1.mKeyLease     = 800;
    SuccessOrQuit(client.Get<Srp::Client>().AddService(ins1));

    nexus.AdvanceTime(5000);

    // Check service on server
    {
        const Srp::Server::Host    *host    = server.Get<Srp::Server>().GetNextHost(nullptr);
        const Srp::Server::Service *service = host->GetNextService(nullptr);

        VerifyOrQuit(StringMatch(service->GetInstanceName(), "ins1._test._udp.default.service.arpa.",
                                 kStringCaseInsensitiveMatch));
        VerifyOrQuit(!service->IsDeleted());
        VerifyOrQuit(service->GetTtl() == 60);

        Srp::Server::LeaseInfo leaseInfo;

        service->GetLeaseInfo(leaseInfo);
        VerifyOrQuit(leaseInfo.mLease == 60000);
        VerifyOrQuit(leaseInfo.mKeyLease == 800000);
    }

    // Register two more services with different lease intervals.
    Srp::Client::Service ins2, ins3;
    ClearAllBytes(ins2);
    ins2.mName         = "_test._udp";
    ins2.mInstanceName = "ins2";
    ins2.mPort         = 2222;
    ins2.mLease        = 30;
    ins2.mKeyLease     = 200;
    SuccessOrQuit(client.Get<Srp::Client>().AddService(ins2));

    ClearAllBytes(ins3);
    ins3.mName         = "_test._udp";
    ins3.mInstanceName = "ins3";
    ins3.mPort         = 3333;
    ins3.mLease        = 100;
    ins3.mKeyLease     = 1000;
    SuccessOrQuit(client.Get<Srp::Client>().AddService(ins3));

    nexus.AdvanceTime(10000);

    // Verify all 3 services on server
    {
        const Srp::Server::Host    *host    = server.Get<Srp::Server>().GetNextHost(nullptr);
        const Srp::Server::Service *service = nullptr;
        uint16_t                    count   = 0;

        while ((service = host->GetNextService(service)) != nullptr)
        {
            count++;
            Srp::Server::LeaseInfo leaseInfo;
            service->GetLeaseInfo(leaseInfo);
            if (StringMatch(service->GetInstanceName(), "ins1._test._udp.default.service.arpa.",
                            kStringCaseInsensitiveMatch))
            {
                VerifyOrQuit(service->GetTtl() == 60);
                VerifyOrQuit(leaseInfo.mLease == 60000);
                VerifyOrQuit(leaseInfo.mKeyLease == 800000);
            }
            else if (StringMatch(service->GetInstanceName(), "ins2._test._udp.default.service.arpa.",
                                 kStringCaseInsensitiveMatch))
            {
                VerifyOrQuit(service->GetTtl() == 30);
                VerifyOrQuit(leaseInfo.mLease == 30000);
                VerifyOrQuit(leaseInfo.mKeyLease == 200000);
            }
            else if (StringMatch(service->GetInstanceName(), "ins3._test._udp.default.service.arpa.",
                                 kStringCaseInsensitiveMatch))
            {
                VerifyOrQuit(service->GetTtl() == 100);
                VerifyOrQuit(leaseInfo.mLease == 100000);
                VerifyOrQuit(leaseInfo.mKeyLease == 1000000);
            }
            else
            {
                VerifyOrQuit(false);
            }
        }
        VerifyOrQuit(count == 3);
    }

    // Wait for longest lease time to validate that all services renew their lease successfully.
    nexus.AdvanceTime(105000);
    {
        const Srp::Server::Host    *host    = server.Get<Srp::Server>().GetNextHost(nullptr);
        const Srp::Server::Service *service = nullptr;
        uint16_t                    count   = 0;
        while ((service = host->GetNextService(service)) != nullptr)
        {
            count++;
            VerifyOrQuit(!service->IsDeleted());
        }
        VerifyOrQuit(count == 3);
    }

    // Remove two services
    SuccessOrQuit(client.Get<Srp::Client>().RemoveService(ins2));
    SuccessOrQuit(client.Get<Srp::Client>().RemoveService(ins3));

    nexus.AdvanceTime(10000);
    {
        const Srp::Server::Host    *host    = server.Get<Srp::Server>().GetNextHost(nullptr);
        const Srp::Server::Service *service = nullptr;
        uint16_t                    count   = 0;
        while ((service = host->GetNextService(service)) != nullptr)
        {
            count++;
            if (StringMatch(service->GetInstanceName(), "ins1._test._udp.default.service.arpa.",
                            kStringCaseInsensitiveMatch))
            {
                VerifyOrQuit(!service->IsDeleted());
            }
            else
            {
                VerifyOrQuit(service->IsDeleted());
            }
        }
        VerifyOrQuit(count == 3);
    }

    // Wait for longer than key-lease of `ins2` service and check that it is removed on server.
    nexus.AdvanceTime(201000);
    {
        const Srp::Server::Host    *host    = server.Get<Srp::Server>().GetNextHost(nullptr);
        const Srp::Server::Service *service = nullptr;
        uint16_t                    count   = 0;
        while ((service = host->GetNextService(service)) != nullptr)
        {
            count++;
            if (StringMatch(service->GetInstanceName(), "ins1._test._udp.default.service.arpa.",
                            kStringCaseInsensitiveMatch))
            {
                VerifyOrQuit(!service->IsDeleted());
            }
            else if (StringMatch(service->GetInstanceName(), "ins3._test._udp.default.service.arpa.",
                                 kStringCaseInsensitiveMatch))
            {
                VerifyOrQuit(service->IsDeleted());
            }
            else
            {
                VerifyOrQuit(false);
            }
        }
        VerifyOrQuit(count == 2);
    }

    // Add both services again now with same lease intervals.
    ins2.mLease    = 30;
    ins2.mKeyLease = 100;
    SuccessOrQuit(client.Get<Srp::Client>().AddService(ins2));
    ins3.mLease    = 30;
    ins3.mKeyLease = 100;
    SuccessOrQuit(client.Get<Srp::Client>().AddService(ins3));

    nexus.AdvanceTime(10000);
    {
        const Srp::Server::Host    *host    = server.Get<Srp::Server>().GetNextHost(nullptr);
        const Srp::Server::Service *service = nullptr;
        uint16_t                    count   = 0;
        while ((service = host->GetNextService(service)) != nullptr)
        {
            count++;
            VerifyOrQuit(!service->IsDeleted());
        }
        VerifyOrQuit(count == 3);
    }

    // Remove `ins1` while adding a new service with same key-lease as `ins1` but different lease interval.
    SuccessOrQuit(client.Get<Srp::Client>().RemoveService(ins1));
    Srp::Client::Service ins4;
    ClearAllBytes(ins4);
    ins4.mName         = "_test._udp";
    ins4.mInstanceName = "ins4";
    ins4.mPort         = 4444;
    ins4.mLease        = 90;
    ins4.mKeyLease     = 800;
    SuccessOrQuit(client.Get<Srp::Client>().AddService(ins4));

    nexus.AdvanceTime(5000);
    {
        const Srp::Server::Host    *host    = server.Get<Srp::Server>().GetNextHost(nullptr);
        const Srp::Server::Service *service = nullptr;
        uint16_t                    count   = 0;
        while ((service = host->GetNextService(service)) != nullptr)
        {
            count++;
            if (StringMatch(service->GetInstanceName(), "ins4._test._udp.default.service.arpa.",
                            kStringCaseInsensitiveMatch))
            {
                VerifyOrQuit(!service->IsDeleted());
                VerifyOrQuit(service->GetTtl() == 90);
            }
            else if (StringMatch(service->GetInstanceName(), "ins1._test._udp.default.service.arpa.",
                                 kStringCaseInsensitiveMatch))
            {
                VerifyOrQuit(service->IsDeleted());
            }
            else
            {
                VerifyOrQuit(!service->IsDeleted());
            }
        }
        VerifyOrQuit(count == 4);
    }

    // Remove `ins2` and `ins3`.
    SuccessOrQuit(client.Get<Srp::Client>().RemoveService(ins2));
    SuccessOrQuit(client.Get<Srp::Client>().RemoveService(ins3));

    nexus.AdvanceTime(10000);
    {
        const Srp::Server::Host    *host    = server.Get<Srp::Server>().GetNextHost(nullptr);
        const Srp::Server::Service *service = nullptr;
        uint16_t                    count   = 0;
        while ((service = host->GetNextService(service)) != nullptr)
        {
            count++;
            if (StringMatch(service->GetInstanceName(), "ins4._test._udp.default.service.arpa.",
                            kStringCaseInsensitiveMatch))
            {
                VerifyOrQuit(!service->IsDeleted());
            }
            else
            {
                VerifyOrQuit(service->IsDeleted());
            }
        }
        VerifyOrQuit(count == 4);
    }

    // Add `ins1` with key-lease smaller than lease and check that client handles this properly
    ins1.mLease    = 100;
    ins1.mKeyLease = 90;
    SuccessOrQuit(client.Get<Srp::Client>().AddService(ins1));

    nexus.AdvanceTime(10000);
    {
        const Srp::Server::Host    *host    = server.Get<Srp::Server>().GetNextHost(nullptr);
        const Srp::Server::Service *service = nullptr;
        uint16_t                    count   = 0;

        while ((service = host->GetNextService(service)) != nullptr)
        {
            count++;
            if (StringMatch(service->GetInstanceName(), "ins1._test._udp.default.service.arpa.",
                            kStringCaseInsensitiveMatch))
            {
                VerifyOrQuit(!service->IsDeleted());
                Srp::Server::LeaseInfo leaseInfo;
                service->GetLeaseInfo(leaseInfo);
                VerifyOrQuit(leaseInfo.mLease == 100000);
                VerifyOrQuit(leaseInfo.mKeyLease == 100000);
            }
        }
        VerifyOrQuit(count == 4);
    }

    // Change default lease and key-lease intervals on client.
    client.Get<Srp::Client>().SetLeaseInterval(40);
    client.Get<Srp::Client>().SetKeyLeaseInterval(330);

    // Add `ins2` and `ins3`. `ins2` specifies the key-lease explicitly but leaves lease as default. `ins3` does the
    // opposite.
    ClearAllBytes(ins2);
    ins2.mName         = "_test._udp";
    ins2.mInstanceName = "ins2";
    ins2.mPort         = 2222;
    ins2.mKeyLease     = 330;
    SuccessOrQuit(client.Get<Srp::Client>().AddService(ins2));

    ClearAllBytes(ins3);
    ins3.mName         = "_test._udp";
    ins3.mInstanceName = "ins3";
    ins3.mPort         = 3333;
    ins3.mLease        = 40;
    SuccessOrQuit(client.Get<Srp::Client>().AddService(ins3));

    nexus.AdvanceTime(10000);
    {
        const Srp::Server::Host    *host    = server.Get<Srp::Server>().GetNextHost(nullptr);
        const Srp::Server::Service *service = nullptr;
        uint16_t                    count   = 0;

        while ((service = host->GetNextService(service)) != nullptr)
        {
            count++;
            Srp::Server::LeaseInfo leaseInfo;
            service->GetLeaseInfo(leaseInfo);
            if (StringMatch(service->GetInstanceName(), "ins2._test._udp.default.service.arpa.",
                            kStringCaseInsensitiveMatch) ||
                StringMatch(service->GetInstanceName(), "ins3._test._udp.default.service.arpa.",
                            kStringCaseInsensitiveMatch))
            {
                VerifyOrQuit(leaseInfo.mLease == 40000);
                VerifyOrQuit(leaseInfo.mKeyLease == 330000);
            }
        }
        VerifyOrQuit(count == 4);
    }

    // Change the default lease to 50 and wait for long enough for `ins2` and `ins3` to do lease refresh. Validate that
    // `ins2` now requests new default lease of 50 while `ins3` should stay as before.
    client.Get<Srp::Client>().SetLeaseInterval(50);
    nexus.AdvanceTime(45000);
    {
        const Srp::Server::Host    *host    = server.Get<Srp::Server>().GetNextHost(nullptr);
        const Srp::Server::Service *service = nullptr;
        uint16_t                    count   = 0;

        while ((service = host->GetNextService(service)) != nullptr)
        {
            count++;
            Srp::Server::LeaseInfo leaseInfo;
            service->GetLeaseInfo(leaseInfo);
            if (StringMatch(service->GetInstanceName(), "ins2._test._udp.default.service.arpa.",
                            kStringCaseInsensitiveMatch))
            {
                VerifyOrQuit(leaseInfo.mLease == 50000);
            }
            else if (StringMatch(service->GetInstanceName(), "ins3._test._udp.default.service.arpa.",
                                 kStringCaseInsensitiveMatch))
            {
                VerifyOrQuit(leaseInfo.mLease == 40000);
            }
        }
        VerifyOrQuit(count == 4);
    }

    // Change the default key lease to 35. `ins3` should adopt this but since it is shorter than its explicitly
    // specified lease the client should use same value for both lease and key-lease.
    client.Get<Srp::Client>().SetKeyLeaseInterval(35);
    nexus.AdvanceTime(45000);
    {
        const Srp::Server::Host    *host    = server.Get<Srp::Server>().GetNextHost(nullptr);
        const Srp::Server::Service *service = nullptr;
        uint16_t                    count   = 0;

        while ((service = host->GetNextService(service)) != nullptr)
        {
            count++;
            Srp::Server::LeaseInfo leaseInfo;
            service->GetLeaseInfo(leaseInfo);
            if (StringMatch(service->GetInstanceName(), "ins3._test._udp.default.service.arpa.",
                            kStringCaseInsensitiveMatch))
            {
                VerifyOrQuit(leaseInfo.mLease == 40000);
                VerifyOrQuit(leaseInfo.mKeyLease == 40000);
            }
        }
        VerifyOrQuit(count == 4);
    }

    // Change the requested TTL.
    client.Get<Srp::Client>().SetTtl(65);
    nexus.AdvanceTime(110000);
    {
        const Srp::Server::Host    *host    = server.Get<Srp::Server>().GetNextHost(nullptr);
        const Srp::Server::Service *service = nullptr;
        uint16_t                    count   = 0;

        while ((service = host->GetNextService(service)) != nullptr)
        {
            count++;
            if (StringMatch(service->GetInstanceName(), "ins1._test._udp.default.service.arpa.",
                            kStringCaseInsensitiveMatch))
            {
                VerifyOrQuit(service->GetTtl() == 65);
            }
            else if (StringMatch(service->GetInstanceName(), "ins2._test._udp.default.service.arpa.",
                                 kStringCaseInsensitiveMatch))
            {
                VerifyOrQuit(service->GetTtl() == 50);
            }
            else if (StringMatch(service->GetInstanceName(), "ins3._test._udp.default.service.arpa.",
                                 kStringCaseInsensitiveMatch))
            {
                VerifyOrQuit(service->GetTtl() == 40);
            }
            else if (StringMatch(service->GetInstanceName(), "ins4._test._udp.default.service.arpa.",
                                 kStringCaseInsensitiveMatch))
            {
                VerifyOrQuit(service->GetTtl() == 65);
            }
        }
        VerifyOrQuit(count == 4);
    }
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::TestSrpRegisterServicesDiffLease();
    printf("All tests passed\n");
    return 0;
}
