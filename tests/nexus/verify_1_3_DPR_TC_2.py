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


def verify(pv):
    # 5.2. 1.3 CERT Service discovery of services on Thread and Infrastructure - Multiple BRs - Multiple Thread -
    #   Single infrastructure
    #
    # 5.2.1. Purpose
    #   To test the following:
    #   - 1. Discovery of on-mesh services between multiple Thread networks attached via an adjacent
    #     infrastructure link.
    #   - 2. BR Discovery Proxy function can discover services advertised by another BR’s Advertising Proxy
    #     function, and vice versa.
    #   - 3. Test both PTR and SRV queries from Thread Network, and finding service in other Network.
    #
    # 5.2.2. Topology
    #   - BR_1 (DUT) - Thread Border Router, and Leader
    #   - BR_2 - Test Bed border router device operating as a Thread Border Router and Leader of an adjacent Thread
    #     network
    #   - ED_1 - Test Bed device operating as a Thread End Device, attached to BR_1
    #   - ED_2 - Test Bed device operating as a Thread End Device, attached to BR_2
    #
    # Spec Reference   | V1.1 Section | V1.3.0 Section
    # -----------------|--------------|---------------
    # Discovery Proxy  | N/A          | 5.2

    pkts = pv.pkts
    pv.summary.show()

    BR_1 = pv.vars['BR_1']
    BR_2 = pv.vars['BR_2']
    BR_1_ETH = pv.vars['BR_1_ETH']
    BR_2_ETH = pv.vars['BR_2_ETH']
    ED_1 = pv.vars['ED_1']
    ED_2 = pv.vars['ED_2']

    # Step 1
    # - Device: BR_1 (DUT), ED_1
    # - Description (DPR-5.2): Enable
    # - Pass Criteria:
    #   - N/A
    print("Step 1: Device: BR_1 (DUT), ED_1 Description (DPR-5.2): Enable")

    # Step 2
    # - Device: BR_1 (DUT)
    # - Description (DPR-5.2): Automatically adds its SRP Server information in the Thread Network Data of first
    #     Thread network.
    # - Pass Criteria:
    #   - The DUT’s SRP Server information MUST appear in the Thread Network Data of the first Thread network.
    print("Step 2: Device: BR_1 (DUT) Description (DPR-5.2): Automatically adds its SRP Server information...")
    pkts.filter_wpan_src64(BR_1). \
      filter_mle_cmd(consts.MLE_DATA_RESPONSE). \
      filter(lambda p: consts.NWD_SERVICE_TLV in p.thread_nwd.tlv.type). \
      filter(lambda p: consts.NWD_SERVER_TLV in p.thread_nwd.tlv.type). \
      must_next()

    # Step 3
    # - Device: ED_1
    # - Description (DPR-5.2): Harness instructs the device to send SRP Update $ORIGIN default.service.arpa.
    #     service-test-1._thread-type-1._udp ( SRV 0 4 55555 host-test-1 ) host-test-1 AAAA <ML-EID address of ED_1>
    #     AAAA <OMR address of ED_1> Note: normally an SRP client would only register the OMR address in this case.
    #     For testing purposes, both OMR/ML-EID are registered which deviates from usual client behavior.
    # - Pass Criteria:
    #   - N/A
    print("Step 3: Device: ED_1 Description (DPR-5.2): Harness instructs the device to send SRP Update...")
    pkts.filter_wpan_src64(ED_1). \
      filter(lambda p: p.dns). \
      filter(lambda p: p.dns.flags.opcode == 5). \
      must_next()

    # Step 4
    # - Device: BR_1 (DUT)
    # - Description (DPR-5.2): Automatically sends SRP Update Response.
    # - Pass Criteria:
    #   - The DUT MUST send a valid SRP Update Response with RCODE=0 (NoError).
    print("Step 4: Device: BR_1 (DUT) Description (DPR-5.2): Automatically sends SRP Update Response")
    pkts.filter_wpan_src64(BR_1). \
      filter(lambda p: p.dns). \
      filter(lambda p: p.dns.flags.opcode == 5). \
      filter(lambda p: p.dns.flags.rcode == consts.DNS_RCODE_NOERROR). \
      must_next()

    # Step 5
    # - Device: BR_2, ED_2
    # - Description (DPR-5.2): Enable
    # - Pass Criteria:
    #   - N/A
    print("Step 5: Device: BR_2, ED_2 Description (DPR-5.2): Enable")

    # Step 6
    # - Device: BR_2
    # - Description (DPR-5.2): Automatically adds its SRP Server information in the Thread Network Data of the
    #     second Thread network.
    # - Pass Criteria:
    #   - BR_2’s SRP Server information MUST appear in the Thread Network Data of the second Thread network.
    print("Step 6: Device: BR_2 Description (DPR-5.2): Automatically adds its SRP Server information...")
    pkts.filter_wpan_src64(BR_2). \
      filter_mle_cmd(consts.MLE_DATA_RESPONSE). \
      filter(lambda p: consts.NWD_SERVICE_TLV in p.thread_nwd.tlv.type). \
      filter(lambda p: consts.NWD_SERVER_TLV in p.thread_nwd.tlv.type). \
      must_next()

    # Step 7
    # - Device: ED_2
    # - Description (DPR-5.2): Harness instructs the device to send SRP Update $ORIGIN default.service.arpa.
    #     service-test-2._thread-type-2._udp ( SRV 0 0 55555 host-test-2 ) host-test-2 AAAA <OMR address of ED_2>
    # - Pass Criteria:
    #   - N/A
    print("Step 7: Device: ED_2 Description (DPR-5.2): Harness instructs the device to send SRP Update...")
    pkts.filter_wpan_src64(ED_2). \
      filter(lambda p: p.dns). \
      filter(lambda p: p.dns.flags.opcode == 5). \
      must_next()

    # Step 8
    # - Device: BR_2
    # - Description (DPR-5.2): Automatically sends SRP Update Response.
    # - Pass Criteria:
    #   - SRP Update Response is valid and reports NoError (RCODE=0).
    print("Step 8: Device: BR_2 Description (DPR-5.2): Automatically sends SRP Update Response")
    pkts.filter_wpan_src64(BR_2). \
      filter(lambda p: p.dns). \
      filter(lambda p: p.dns.flags.opcode == 5). \
      filter(lambda p: p.dns.flags.rcode == consts.DNS_RCODE_NOERROR). \
      must_next()

    # Step 9
    # - Device: ED_1
    # - Description (DPR-5.2): Harness instructs the device to unicast DNS query QType=PTR to the DUT for all
    #     services of type “_thread-type-2._udp.default.service.arpa.”.
    # - Pass Criteria:
    #   - N/A
    print("Step 9: Device: ED_1 Description (DPR-5.2): Harness instructs the device to unicast DNS query...")
    pkts.filter_wpan_src64(ED_1). \
      filter(lambda p: p.dns). \
      filter(lambda p: p.dns.flags.response == 0). \
      filter(lambda p: "_thread-type-2._udp.default.service.arpa" in verify_utils.as_list(p.dns.qry.name)). \
      filter(lambda p: any(t == consts.DNS_TYPE_PTR for t in verify_utils.as_list(p.dns.qry.type))). \
      must_next()

    # Step 10
    # - Device: BR_1 (DUT)
    # - Description (DPR-5.2): Automatically sends DNS Response. Note: during this step, DUT performs mDNS query to
    #     BR_2 which responds with service-test-2. This isn’t shown in detail.
    # - Pass Criteria:
    #   - The DUT MUST send a valid DNS Response, containing service-test-2 as Answer record: $ORIGIN
    #     default.service.arpa. _thread-type-2._udp ( PTR service-test-2._thread-type-2._udp )
    #   - It MAY contain in the Additional records section: $ORIGIN default.service.arpa.
    #     service-test-2._thread-type-2._udp ( SRV 0 0 55555 host-test-2 ) host-test-2 AAAA <OMR address of ED_2>
    print("Step 10: Device: BR_1 (DUT) Description (DPR-5.2): Automatically sends DNS Response")
    pkts.filter_wpan_src64(BR_1). \
      filter(lambda p: p.dns). \
      filter(lambda p: p.dns.flags.response == 1). \
      filter(lambda p: any("service-test-2" in str(name)
                           for name in verify_utils.as_list(p.dns.ptr.domain_name))). \
      must_next()

    # Step 11
    # - Device: ED_2
    # - Description (DPR-5.2): Harness instructs the device to unicast DNS query QType=PTR to BR_2 for all services
    #     of type “_thread-type-1._udp.default.service.arpa.”.
    # - Pass Criteria:
    #   - N/A
    print("Step 11: Device: ED_2 Description (DPR-5.2): Harness instructs the device to unicast DNS query...")
    pkts.filter_wpan_src64(ED_2). \
      filter(lambda p: p.dns). \
      filter(lambda p: p.dns.flags.response == 0). \
      filter(lambda p: "_thread-type-1._udp.default.service.arpa" in verify_utils.as_list(p.dns.qry.name)). \
      filter(lambda p: any(t == consts.DNS_TYPE_PTR for t in verify_utils.as_list(p.dns.qry.type))). \
      must_next()

    # Step 12
    # - Device: BR_2
    # - Description (DPR-5.2): Automatically sends mDNS query.
    # - Pass Criteria:
    #   - N/A
    print("Step 12: Device: BR_2 Description (DPR-5.2): Automatically sends mDNS query")
    pkts.filter_eth_src(BR_2_ETH). \
      filter(lambda p: p.mdns). \
      filter(lambda p: "_thread-type-1._udp.local" in verify_utils.as_list(p.mdns.qry.name)). \
      must_next()

    # Step 13
    # - Device: BR_1 (DUT)
    # - Description (DPR-5.2): Automatically responds to mDNS query with “service-test-1”.
    # - Pass Criteria:
    #   - N/A (verified in step 14)
    print("Step 13: Device: BR_1 (DUT) Description (DPR-5.2): Automatically responds to mDNS query...")
    pkts.filter_eth_src(BR_1_ETH). \
      filter(lambda p: p.mdns). \
      filter(lambda p: any("service-test-1" in str(name)
                           for name in verify_utils.as_list(p.mdns.resp.name))). \
      must_next()

    # Step 14
    # - Device: BR_2
    # - Description (DPR-5.2): Automatically sends a DNS response to ED_2.
    # - Pass Criteria:
    #   - The DNS Response is received at ED_2, is valid and contains only service-test-1 in the Answers section:
    #     $ORIGIN default.service.arpa. _thread-type-1._udp ( PTR service-test-1._thread-type-1._udp )
    #   - It MAY contain in the Additional records section: $ORIGIN default.service.arpa.
    #     service-test-1._thread-type-1._udp ( SRV 0 4 55555 host-test-1 ) host-test-1 AAAA <OMR address of ED_1>
    #   - Also it MAY contain in the Additional records section, but only if the above Additional records are also
    #     included: $ORIGIN default.service.arpa. host-test-1 AAAA <ML-EID address of ED_1>
    print("Step 14: Device: BR_2 Description (DPR-5.2): Automatically sends a DNS response to ED_2")
    pkts.filter_wpan_src64(BR_2). \
      filter(lambda p: p.dns). \
      filter(lambda p: p.dns.flags.response == 1). \
      filter(lambda p: any("service-test-1" in str(name)
                           for name in verify_utils.as_list(p.dns.ptr.domain_name))). \
      must_next()

    # Step 15
    # - Device: ED_1
    # - Description (DPR-5.2): Harness instructs the device to unicast DNS query QType=SRV to the DUT for service
    #     “service-test-2._thread-type-2._udp.default.service.arpa.”.
    # - Pass Criteria:
    #   - N/A
    print("Step 15: Device: ED_1 Description (DPR-5.2): Harness instructs the device to unicast DNS query...")
    pkts.filter_wpan_src64(ED_1). \
      filter(lambda p: p.dns). \
      filter(lambda p: p.dns.flags.response == 0). \
      filter(lambda p: "service-test-2._thread-type-2._udp.default.service.arpa"
             in verify_utils.as_list(p.dns.qry.name)). \
      filter(lambda p: any(t == consts.DNS_TYPE_SRV for t in verify_utils.as_list(p.dns.qry.type))). \
      must_next()

    # Step 16
    # - Device: BR_1 (DUT)
    # - Description (DPR-5.2): Automatically sends DNS Response. Note: during this step, DUT MAY perform mDNS query
    #     to BR_2 which responds with service-test-2. This isn’t shown in detail.
    # - Pass Criteria:
    #   - The DUT MUST send a valid DNS Response is valid, containing service-test-2 answer record: $ORIGIN
    #     default.service.arpa. service-test-2._thread-type-2._udp ( SRV 0 0 55555 host-test-2 )
    #   - It MAY contain in the Additional records section: host-test-2 AAAA <OMR address of ED_2>
    print("Step 16: Device: BR_1 (DUT) Description (DPR-5.2): Automatically sends DNS Response")
    pkts.filter_wpan_src64(BR_1). \
      filter(lambda p: p.dns). \
      filter(lambda p: p.dns.flags.response == 1). \
      filter(lambda p: any("service-test-2" in str(name)
                           for name in verify_utils.as_list(p.dns.resp.name))). \
      must_next()


if __name__ == '__main__':
    verify_utils.run_main(verify)
