#!/usr/bin/env python3
#
#  Copyright (c) 2026, The OpenThread Authors.
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

import sys
import os

# Add the current directory to sys.path to find verify_utils
CUR_DIR = os.path.dirname(os.path.abspath(__file__))
sys.path.append(CUR_DIR)

import verify_utils
from pktverify import consts
from pktverify.addrs import Ipv6Addr


def verify(pv):
    # 3.5. [1.3] [CERT] [COMPONENT] DNS-SD client discovers services
    #
    # 3.5.1. Purpose
    # To test the following:
    # - DNS-SD client discovers services and sends out correctly formatted DNS query
    # - Client handling multiple service responses
    # - Client handling of both mesh-local discovered services and off-mesh services
    # - Note: Thread Product DUTs must skip this test.
    #
    # 3.5.2. Topology
    # - BR 1-Border Router Reference Device and Leader
    # - TD_1 (DUT)-Thread End Device or Thread Router DUT (Component DUT)
    # - ED 2-Thread Reference End Device
    # - Eth 1-Host Reference Device on AIL
    #
    # Spec Reference   | V1.1 Section | V1.3.0 Section
    # -----------------|--------------|---------------
    # SRP Client       | N/A          | 2.3.2

    pkts = pv.pkts
    pv.summary.show()

    BR_1_MLEID = Ipv6Addr(pv.vars['BR_1_MLEID_ADDR'])
    TD_1_MLEID = Ipv6Addr(pv.vars['TD_1_MLEID_ADDR'])
    ED_2_MLEID = Ipv6Addr(pv.vars['ED_2_MLEID_ADDR'])
    ED_2_OMR_ADDR = Ipv6Addr(pv.vars['ED_2_OMR_ADDR'])

    SERVICE_TYPE = '_thread-test._udp.default.service.arpa'
    DNS_PORT = 53

    SRP_SERVICE_PORT1 = 55551
    SRP_SERVICE_PORT2 = 55552
    SRP_SERVICE_PORT3 = 55553
    SRP_SERVICE_PORT4 = 55554
    SRP_SERVICE_PORT5 = 55555

    PORTS = [SRP_SERVICE_PORT1, SRP_SERVICE_PORT2, SRP_SERVICE_PORT3, SRP_SERVICE_PORT4, SRP_SERVICE_PORT5]

    # Step 1
    # - Device: BR 1. ED 2, Eth 1
    # - Description (SRPC-3.5): Enable
    # - Pass Criteria:
    #   - N/A
    print("Step 1: BR 1, ED 2, Eth 1 enable.")

    # Step 2
    # - Device: BR 1
    # - Description (SRPC-3.5): Automatically adds its SRP Server information in the DNS/SRP Unicast Dataset in the
    #   Thread Network Data. Automatically configures an OMR prefix in Thread Network Data.
    # - Pass Criteria:
    #   - N/A
    print("Step 2: BR 1 adds SRP Server information and OMR prefix.")

    # Step 3
    # - Device: ED 2
    # - Description (SRPC-3.5): Harness instructs device to register multiple N=5 services, each with TXT record per
    #   DNS-SD key/value formatting:
    #   - service-test-1. thread-test._udp ( SRV 8 8 55551 host-test-2 TXT dummy=abcd; thread-test=a49 )
    #   - service-test-2._thread-test._udp ( SRV 8 1 55552 host-test-2 TXT a=2; thread-test=b50_test )
    #   - service-test-3. thread-test._udp ( SRV 1  55553 host-test-2 TXT a_dummy=42; thread-test= )
    #   - service-test-4. thread-test._udp ( SRV 1 1 55554 host-test-2 TXT thread-test=1;ignorethis; thread-test= )
    #   - service-test-5. thread-test.udp ( SRV 2 2 55555 host-test-2 TXT a=88;thread- THREAD-TEST=C51; ignorethis;
    #     thread-test )
    #   - host-test-2 AAAA <non-routable address 2001::1234>
    #   - host-test-2 AAAA <OMR address of ED 2>
    #   - host-test-2 AAAA <ML-EID address of ED_2>
    #   - with the following options: Update Lease Option Lease: 5 minutes Key Lease: 10 minutes.
    # - Pass Criteria:
    #   - N/A
    print("Step 3: ED 2 registers 5 services.")

    # Step 4
    # - Device: TD 1 (DUT)
    # - Description (SRPC-3.5): Enable
    # - Pass Criteria:
    #   - N/A
    print("Step 4: TD 1 (DUT) enable.")

    # Step 5
    # - Device: TD_1 (DUT)
    # - Description (SRPC-3.5): Harness instructs device to query for any services of type
    #   "_thread-test._udp.default.service.arpa." using unicast DNS query QType=PTR
    # - Pass Criteria:
    #   - The DUT MUST send a correctly formatted DNS query.
    print("Step 5: TD 1 (DUT) sends DNS query PTR.")
    pkts.filter_ipv6_src(TD_1_MLEID).\
        filter_ipv6_dst(BR_1_MLEID).\
        filter(lambda p: p.udp.dstport == DNS_PORT).\
        filter(lambda p: SERVICE_TYPE in verify_utils.as_list(p.dns.qry.name)).\
        filter(lambda p: consts.DNS_TYPE_PTR in verify_utils.as_list(p.dns.qry.type)).\
        must_next()

    # Step 6
    # - Device: BR 1
    # - Description (SRPC-3.5): Automatically responds with all N services.
    # - Pass Criteria:
    #   - Responds with DNS answer with all N services as indicated in step 3 in the Answer records section.
    #   - DNS response Rcode MUST be 'NoError' (0).
    #   - DNS response MUST contain N=5 answer records.
    print("Step 6: BR 1 responds with all 5 services.")
    pkts.filter_ipv6_src(BR_1_MLEID).\
        filter_ipv6_dst(TD_1_MLEID).\
        filter(lambda p: p.udp.srcport == DNS_PORT).\
        filter(lambda p: p.dns.flags.response == 1).\
        filter(lambda p: p.dns.flags.rcode == 0).\
        filter(lambda p: len(verify_utils.as_list(p.dns.ptr.domain_name)) == 5).\
        must_next()

    # Step 7
    # - Device: TD_1 (DUT)
    # - Description (SRPC-3.5): Harness receives each of the N service responses via API from the DUT Component.
    #   For each service, it uses the API to extract the TXT record key "thread-test" and compares against the
    #   expected value.
    # - Pass Criteria:
    #   - The DUT MUST return the correct values for the "thread-test" key as defined below for each of the
    #     respective N services:
    #     - 1. a49
    #     - 2. b50 test
    #     - 3. (empty string of 0 characters)
    #     - 4. 1
    #     - 5. C51
    print("Step 7: TD 1 (DUT) extracts and compares TXT record values.")

    # Step 8
    # - Device: TD 1 (DUT)
    # - Description (SRPC-3.5): Harness uses API to resolve for each of the N services the IPv6 address and port of
    #   the service. For each returned/resolved service, Harness uses the API to send a UDP packet to the service's
    #   IPv6 address at the respective returned port number.
    # - Pass Criteria:
    #   - The DUT MUST transmit 5 UDP packets to ED_2, to the following port numbers, which may be in any order:
    #     - 1. 55551
    #     - 2. 55552
    #     - 3. 55553
    #     - 4. 55554
    #     - 5. 55555
    #   - The DUT MAY send to either the ML-EID or OMR address of ED 2.
    print("Step 8: TD 1 (DUT) transmits 5 UDP packets to ED 2.")
    for port in PORTS:
        pkts.filter_ipv6_src(TD_1_MLEID).\
            filter(lambda p: p.ipv6.dst == ED_2_MLEID or p.ipv6.dst == ED_2_OMR_ADDR).\
            filter(lambda p: p.udp.dstport == port).\
            must_next()


if __name__ == '__main__':
    verify_utils.run_main(verify)
