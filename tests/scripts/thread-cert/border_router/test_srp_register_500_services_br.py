#!/usr/bin/env python3
#
#  Copyright (c) 2021, The OpenThread Authors.
#  All rights reserved.
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions are met:
#  1. Redistributions of source code must retain the above copyright
#     notice, this list of conditions and the following disclaimer.
#  2. Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in the
#     documentation and/or other materials provided with the distribution.
#  3. Neither the name of the copyright holder nor the
#     names of its contributors may be used to endorse or promote products
#     derived from this software without specific prior written permission.
#
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
#  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
#  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
#  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
#  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
#  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
#  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
#  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
#  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
#  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
#  POSSIBILITY OF SUCH DAMAGE.
#

import unittest

from test_srp_register_500_services import CLIENT_IDS, INSTANCE_IDS, SrpRegister500Services, SERVER, SERVICE_NAME

# Test description:
#   This test verifies SRP function that SRP clients can register up to 500 services to one SRP server.
#
#   This test is same as test_srp_register_500_services.py, with the only difference that the SRP server
#   is running on an OTBR (Docker).
#

HOST = SERVER + 1


class SrpRegister500ServicesBR(SrpRegister500Services):
    TOPOLOGY = SrpRegister500Services.TOPOLOGY
    TOPOLOGY[SERVER]['is_otbr'] = True
    TOPOLOGY[HOST] = {
        'name': 'Host',
        'is_host': True,
    }

    def test(self):
        self.nodes[HOST].start(start_radvd=False)

        self._test_impl()

        # Verify all SRP services are resolvable via mDNS
        host = self.nodes[HOST]

        browse_retry = 3
        for i in range(browse_retry):
            try:
                service_instances = set(host.browse_mdns_services(SERVICE_NAME, timeout=10))
                print(service_instances)

                for clientid in CLIENT_IDS:
                    for instanceid in INSTANCE_IDS:
                        self.assertIn(f'client{clientid}_{instanceid}', service_instances)

                break

            except Exception:
                if i < browse_retry - 1:
                    self.simulator.go(3)
                    continue


del SrpRegister500Services

if __name__ == '__main__':
    unittest.main()
