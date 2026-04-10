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
    # 2.2. [1.3] [CERT] Name Conflicts - Single Thread Network
    #
    # 2.2.1. Purpose
    # To test the following:
    # - 1. Handle name conflict in Host Description record.
    # - 2. Handle name conflict in Service Description record.
    # - 3. Verify that original service is seen when discovering, and conflicting
    #   service is not seen on the AIL.
    #
    # 2.2.2. Topology
    # - 1. BR_1 (DUT) - Border Router and the Leader.
    # - 2. ED 1-Test Bed device operating as a Thread End Device, attached to BR_1
    # - 3. ED 2-Test Bed device operating as a Thread End Device, attached to BR_1
    # - 4. Eth 1-Test Bed border router device on an Adjacent Infrastructure Link
    #
    # 2.2.3. Initial Conditions
    # - BR_1 (DUT) should automatically start SRP server capabilities at the
    #   beginning of the test.
    pkts = pv.pkts
    pv.summary.show()

    BR_1_MLEID = Ipv6Addr(pv.vars['BR_1_MLEID_ADDR'])
    ED_1_MLEID = Ipv6Addr(pv.vars['ED_1_MLEID_ADDR'])
    ED_2_MLEID = Ipv6Addr(pv.vars['ED_2_MLEID_ADDR'])
    ED_1_OMR = Ipv6Addr(pv.vars['ED_1_OMR_ADDR'])
    ED_2_OMR = Ipv6Addr(pv.vars['ED_2_OMR_ADDR'])
    br1_srp_port = int(pv.vars['BR_1_SRP_PORT'])

    SRP_SERVICE_NAME = '_thread-test._udp.default.service.arpa'
    SRP_INSTANCE_1 = 'service-test-1.' + SRP_SERVICE_NAME
    SRP_INSTANCE_2 = 'service-test-2.' + SRP_SERVICE_NAME
    SRP_HOST_1 = 'host-test-1.default.service.arpa'
    SRP_HOST_2 = 'host-test-2.default.service.arpa'
    MDNS_SERVICE_NAME = '_thread-test._udp.local'
    MDNS_INSTANCE_1 = 'service-test-1.' + MDNS_SERVICE_NAME
    MDNS_INSTANCE_2 = 'service-test-2.' + MDNS_SERVICE_NAME
    MDNS_HOST_1 = 'host-test-1.local'
    MDNS_HOST_2 = 'host-test-2.local'

    SRP_PORT_33333 = 33333
    SRP_PORT_44444 = 44444

    SERVICE_ID_SRP_SERVER = 93

    # Step 1
    # - Device: BR 1 (DUT)
    # - Description (SRP-2.2): Enable: switch on.
    # - Pass Criteria:
    #   - N/A
    print("Step 1: BR 1 (DUT) switch on.")

    # Step 2
    # - Device: ED 1, ED 2, Eth 1
    # - Description (SRP-2.2): Enable
    # - Pass Criteria:
    #   - N/A
    print("Step 2: ED 1, ED 2, Eth 1 Enable.")

    # Step 3
    # - Device: BR 1 (DUT)
    # - Description (SRP-2.2): Automatically adds its SRP Server information in the
    #   Thread Network Data.
    # - Pass Criteria:
    #   - The DUT's SRP Server information MUST appear in the Thread Network Data.
    #     This can be verified as specified in test case 2.1 step 3.
    #   - Specifically, it MUST include a DNS-SD Unicast Dataset.
    print("Step 3: BR 1 (DUT) adds its SRP Server information in the Thread Network Data.")
    pkts. \
      filter_mle_cmd(consts.MLE_DATA_RESPONSE). \
      filter(lambda p: SERVICE_ID_SRP_SERVER in verify_utils.as_list(p.thread_nwd.tlv.service.srp_dataset_identifier)). \
      filter(lambda p: any(l in (1, 19) for l in verify_utils.as_list(p.thread_nwd.tlv.service.s_data_len))). \
      must_next()

    # Step 4
    # - Device: ED 1
    # - Description (SRP-2.2): Harness instructs the device to send SRP Update with
    #   initial service registration: $ORIGIN default.service.arpa.
    #   service-test-1._thread-test._udp ( SRV 0 0 33333 host-test-1 ) host-test-1
    #   AAAA <OMR address of ED_1>
    # - Pass Criteria:
    #   - N/A
    print("Step 4: ED 1 sends SRP Update with initial service registration.")
    pkts. \
      filter_ipv6_dst(BR_1_MLEID). \
      filter_ipv6_src(ED_1_MLEID). \
      filter(lambda p: p.udp.dstport == br1_srp_port). \
      filter(lambda p: p.dns.flags.opcode == 5). \
      filter(lambda p: SRP_INSTANCE_1 in verify_utils.as_list(p.dns.resp.name)). \
      filter(lambda p: SRP_PORT_33333 in verify_utils.as_list(p.dns.srv.port)). \
      filter(lambda p: SRP_HOST_1 in verify_utils.as_list(p.dns.srv.target)). \
      filter(lambda p: ED_1_OMR in verify_utils.as_list(p.dns.aaaa)). \
      must_next()

    # Step 5
    # - Device: BR 1 (DUT)
    # - Description (SRP-2.2): Automatically sends SRP Update Response.
    # - Pass Criteria:
    #   - The DUT MUST send a valid SRP Update Response, with RCODE=0 (NoError).
    print("Step 5: BR 1 (DUT) sends SRP Update Response (NoError).")
    pkts. \
      filter_ipv6_dst(ED_1_MLEID). \
      filter_ipv6_src(BR_1_MLEID). \
      filter(lambda p: p.udp.srcport == br1_srp_port). \
      filter(lambda p: p.dns.flags.response == 1). \
      filter(lambda p: p.dns.flags.rcode == 0). \
      must_next()

    # Step 6
    # - Device: ED 2
    # - Description (SRP-2.2): Harness instructs the device to send SRP Update with
    #   same-name (conflicting) service registration: $ORIGIN
    #   default.service.arpa. service-test-1._thread-test._udp ( SRV 0 0 33333
    #   host-test-2 ) host-test-2 AAAA <OMR address of ED_2>
    # - Pass Criteria:
    #   - N/A
    print("Step 6: ED 2 sends SRP Update with same-name (conflicting) service registration.")
    pkts. \
      filter_ipv6_dst(BR_1_MLEID). \
      filter_ipv6_src(ED_2_MLEID). \
      filter(lambda p: p.udp.dstport == br1_srp_port). \
      filter(lambda p: p.dns.flags.opcode == 5). \
      filter(lambda p: SRP_INSTANCE_1 in verify_utils.as_list(p.dns.resp.name)). \
      filter(lambda p: SRP_PORT_33333 in verify_utils.as_list(p.dns.srv.port)). \
      filter(lambda p: SRP_HOST_2 in verify_utils.as_list(p.dns.srv.target)). \
      filter(lambda p: ED_2_OMR in verify_utils.as_list(p.dns.aaaa)). \
      must_next()

    # Step 7
    # - Device: BR 1 (DUT)
    # - Description (SRP-2.2): Automatically sends SRP Update Response.
    # - Pass Criteria:
    #   - The DUT MUST send a valid SRP Update Response, with RCODE=6 (YXDOMAIN).
    print("Step 7: BR 1 (DUT) sends SRP Update Response (YXDOMAIN).")
    pkts. \
      filter_ipv6_dst(ED_2_MLEID). \
      filter_ipv6_src(BR_1_MLEID). \
      filter(lambda p: p.udp.srcport == br1_srp_port). \
      filter(lambda p: p.dns.flags.response == 1). \
      filter(lambda p: p.dns.flags.rcode == 6). \
      must_next()

    # mDNS checks helper
    def verify_no_ed2_info(p):
        assert ED_2_OMR not in verify_utils.as_list(p.mdns.aaaa), f"Packet {p.index} contains ED_2_OMR"
        assert MDNS_INSTANCE_2 not in verify_utils.as_list(
            p.mdns.ptr.domain_name), f"Packet {p.index} contains MDNS_INSTANCE_2 in PTR"
        assert MDNS_INSTANCE_2 not in verify_utils.as_list(
            p.mdns.resp.name), f"Packet {p.index} contains MDNS_INSTANCE_2 in resp.name"

    def verify_mdns_ptr_response(pkts, instance_name):
        return pkts.\
            filter(lambda p: p.udp.dstport == 5353). \
            filter(lambda p: p.mdns.flags.response == 1). \
            filter(lambda p: instance_name in verify_utils.as_list(p.mdns.ptr.domain_name)). \
            must_next()

    def verify_mdns_srv_response(pkts, instance_name, port):
        return pkts.\
            filter(lambda p: p.udp.dstport == 5353). \
            filter(lambda p: p.mdns.flags.response == 1). \
            filter(lambda p: instance_name in verify_utils.as_list(p.mdns.resp.name)). \
            filter(lambda p: port in verify_utils.as_list(p.mdns.srv.port)). \
            must_next()

    def verify_mdns_aaaa_response(pkts, host_name, omr_addr):
        return pkts.\
            filter(lambda p: p.udp.dstport == 5353). \
            filter(lambda p: p.mdns.flags.response == 1). \
            filter(lambda p: host_name in verify_utils.as_list(p.mdns.resp.name)). \
            filter(lambda p: omr_addr in verify_utils.as_list(p.mdns.aaaa)). \
            must_next()

    # Step 8 era
    index_step8 = pkts.index

    # Step 8
    # - Device: Eth 1
    # - Description (SRP-2.2): Harness instructs device to perform mDNS query QType
    #   PTR for any services of type _thread-test._udp.local..
    # - Pass Criteria:
    #   - N/A
    print("Step 8: Eth 1 performs mDNS query QType PTR.")

    # Step 9
    # - Device: BR 1 (DUT)
    # - Description (SRP-2.2): Automatically sends mDNS response with initial
    #   registered service.
    # - Pass Criteria:
    #   - The DUT MUST send a valid mDNS Response and contain answer record:
    #     _thread-test._udp.local PTR ( service-test-1._thread-test._udp.local. )
    #   - It MAY also contain in the Additional records section:
    #     service-test-1._thread-test._udp.local. ( SRV 0 0 33333 host-test-1.local. )
    #   - It MAY also contain in the Additional records section: host-test-1.local.
    #     AAAA <OMR address of ED_1>
    #   - The mDNS Response MUST NOT contain any answer record or additional
    #     record like: <any-string> AAAA <OMR address of ED_2>
    #   - The mDNS Response MUST NOT contain any answer record or additional
    #     record like: host-test-1.local. AAAA <any non-OMR address>
    #   - The mDNS Response MUST NOT contain any answer record or additional
    #     record like: service-test-2._thread-test._udp.local. SRV <any data>
    #   - The mDNS Response MUST NOT contain any answer record or additional
    #     record like: <any-name> PTR service-test-2._thread-test_udp.local.
    print("Step 9: BR 1 (DUT) sends mDNS response with initial registered service.")
    with pkts.save_index():
        pkts.index = index_step8
        res = verify_mdns_ptr_response(pkts, MDNS_INSTANCE_1)
        verify_no_ed2_info(res)

    # Step 9b
    # - Device: Eth 1
    # - Description (SRP-2.2): Harness instructs the device to send mDNS query
    #   QTYPE SRV QNAME service-test-1._thread-test._udp.local
    # - Pass Criteria:
    #   - The DUT MUST respond RCODE=0 with Answer section record:
    #     service-test-1._thread-test._udp.local ( SRV 0 0 33333 host-test-1.local )
    print("Step 9b: Eth 1 sends mDNS query QTYPE SRV.")
    with pkts.save_index():
        pkts.index = index_step8
        verify_mdns_srv_response(pkts, MDNS_INSTANCE_1, SRP_PORT_33333)

    # Step 9c
    # - Device: Eth 1
    # - Description (SRP-2.2): Harness instructs the device to send mDNS query
    #   QTYPE AAAA QNAME host-test-1.local
    # - Pass Criteria:
    #   - The DUT MUST respond RCODE=0 with Answer section record:
    #     host-test-1.local AAAA <OMR address of ED_1>
    print("Step 9c: Eth 1 sends mDNS query QTYPE AAAA.")
    with pkts.save_index():
        pkts.index = index_step8
        verify_mdns_aaaa_response(pkts, MDNS_HOST_1, ED_1_OMR)

    # Step 10
    # - Device: ED 2
    # - Description (SRP-2.2): Harness instructs the device to send SRP Update with
    #   same-hostname (conflicting) service registration: $ORIGIN
    #   default.service.arpa. service-test-2._thread-test._udp ( SRV 0 0 33333
    #   host-test-1 ) host-test-1 AAAA <OMR address of ED_2>
    # - Pass Criteria:
    #   - N/A
    print("Step 10: ED 2 sends SRP Update with same-hostname (conflicting) service registration.")
    pkts. \
      filter_ipv6_dst(BR_1_MLEID). \
      filter_ipv6_src(ED_2_MLEID). \
      filter(lambda p: p.udp.dstport == br1_srp_port). \
      filter(lambda p: p.dns.flags.opcode == 5). \
      filter(lambda p: SRP_INSTANCE_2 in verify_utils.as_list(p.dns.resp.name)). \
      filter(lambda p: SRP_PORT_33333 in verify_utils.as_list(p.dns.srv.port)). \
      filter(lambda p: SRP_HOST_1 in verify_utils.as_list(p.dns.srv.target)). \
      filter(lambda p: ED_2_OMR in verify_utils.as_list(p.dns.aaaa)). \
      must_next()

    # Step 11
    # - Device: BR 1 (DUT)
    # - Description (SRP-2.2): Automatically sends SRP Update Response.
    # - Pass Criteria:
    #   - The DUT MUST send a valid SRP Update Response, with RCODE=6 (YXDOMAIN).
    print("Step 11: BR 1 (DUT) sends SRP Update Response (YXDOMAIN).")
    pkts. \
      filter_ipv6_dst(ED_2_MLEID). \
      filter_ipv6_src(BR_1_MLEID). \
      filter(lambda p: p.udp.srcport == br1_srp_port). \
      filter(lambda p: p.dns.flags.response == 1). \
      filter(lambda p: p.dns.flags.rcode == 6). \
      must_next()

    # Step 12
    # - Device: Eth 1
    # - Description (SRP-2.2): Repeat Step 8
    # - Pass Criteria:
    #   - Repeat Step 8
    print("Step 12: Eth 1 repeats mDNS query QType PTR.")

    # Step 13
    # - Device: BR 1 (DUT)
    # - Description (SRP-2.2): Repeat Step 9 (including 9b, 9c, etc)
    # - Pass Criteria:
    #   - Repeat Step 9 (including 9b, 9c, etc)
    print("Step 13: BR 1 (DUT) repeats mDNS response verification.")
    with pkts.save_index():
        pkts.index = index_step8
        res = verify_mdns_ptr_response(pkts, MDNS_INSTANCE_1)
        assert ED_1_OMR in verify_utils.as_list(res.mdns.aaaa)
        verify_no_ed2_info(res)

    with pkts.save_index():
        pkts.index = index_step8
        verify_mdns_srv_response(pkts, MDNS_INSTANCE_1, SRP_PORT_33333)

    with pkts.save_index():
        pkts.index = index_step8
        verify_mdns_aaaa_response(pkts, MDNS_HOST_1, ED_1_OMR)

    # Step 14
    # - Device: ED 2
    # - Description (SRP-2.2): Harness instructs the device to send SRP Update with
    #   non-conflicting service registration: $ORIGIN default.service.arpa.
    #   service-test-2._thread-test._udp ( SRV 1 0 44444 host-test-2 ) host-test-2
    #   AAAA <OMR address of ED_2>
    # - Pass Criteria:
    #   - N/A
    print("Step 14: ED 2 sends SRP Update with non-conflicting service registration.")
    pkts. \
      filter_ipv6_dst(BR_1_MLEID). \
      filter_ipv6_src(ED_2_MLEID). \
      filter(lambda p: p.udp.dstport == br1_srp_port). \
      filter(lambda p: p.dns.flags.opcode == 5). \
      filter(lambda p: SRP_INSTANCE_2 in verify_utils.as_list(p.dns.resp.name)). \
      filter(lambda p: SRP_PORT_44444 in verify_utils.as_list(p.dns.srv.port)). \
      filter(lambda p: SRP_HOST_2 in verify_utils.as_list(p.dns.srv.target)). \
      filter(lambda p: ED_2_OMR in verify_utils.as_list(p.dns.aaaa)). \
      must_next()

    # Step 15
    # - Device: BR_1 (DUT)
    # - Description (SRP-2.2): Automatically sends SRP Update Response.
    # - Pass Criteria:
    #   - The DUT MUST send a valid SRP Update Response, with RCODE=0 (NoError).
    print("Step 15: BR 1 (DUT) sends SRP Update Response (NoError).")
    pkts. \
      filter_ipv6_dst(ED_2_MLEID). \
      filter_ipv6_src(BR_1_MLEID). \
      filter(lambda p: p.udp.srcport == br1_srp_port). \
      filter(lambda p: p.dns.flags.response == 1). \
      filter(lambda p: p.dns.flags.rcode == 0). \
      must_next()

    # Step 16
    # - Device: Eth 1
    # - Description (SRP-2.2): Harness instructs the device to perform mDNS query
    #   QType PTR for any services of type _thread-test_udp.local..
    # - Pass Criteria:
    #   - N/A
    print("Step 16: Eth 1 performs mDNS query QType PTR.")

    # Step 17
    # - Device: BR 1 (DUT)
    # - Description (SRP-2.2): Automatically sends mDNS response with both
    #   registered services.
    # - Pass Criteria:
    #   - The DUT MUST send a valid mDNS Response and MUST contain 2 answer
    #     records: _thread-test._udp.local ( PTR service-test-1._thread-test._udp.local )
    #     AND _thread-test._udp.local ( PTR service-test-2._thread-test._udp.local )
    #   - It MAY also contain in the Additional records section:
    #     service-test-1._thread-test._udp.local. ( SRV 0 0 33333 host-test-1.local. )
    #   - It MAY also contain in the Additional records section:
    #     service-test-2._thread-test._udp.local. ( SRV 1 0 44444 host-test-2.local. )
    #   - It MAY also contain in the Additional records section: host-test-1.local.
    #     AAAA <OMR address of ED_1>
    #   - It MAY also contain in the Additional records section: host-test-2.local.
    #     AAAA <OMR address of ED_2>
    #   - The response MAY be sent as two separate mDNS response messages,
    #     splitting over the 2 services.
    print("Step 17: BR 1 (DUT) sends mDNS response with both registered services.")
    with pkts.save_index():
        pkts.index = index_step8
        verify_mdns_ptr_response(pkts, MDNS_INSTANCE_1)
    with pkts.save_index():
        pkts.index = index_step8
        verify_mdns_ptr_response(pkts, MDNS_INSTANCE_2)

    # Step 17b
    # - Device: Eth 1
    # - Description (SRP-2.2): Harness instructs the device to send mDNS query
    #   QTYPE=SRV QNAME service-test-1._thread-test._udp.local
    # - Pass Criteria:
    #   - The DUT MUST respond RCODE=0 with Answer section record:
    #     service-test-1._thread-test._udp.local ( SRV 0 0 33333 host-test-1.local )
    print("Step 17b: Eth 1 sends mDNS query QTYPE SRV instance 1.")
    with pkts.save_index():
        pkts.index = index_step8
        verify_mdns_srv_response(pkts, MDNS_INSTANCE_1, SRP_PORT_33333)

    # Step 17c
    # - Device: Eth_1
    # - Description (SRP-2.2): Harness instructs the device to send mDNS query
    #   QTYPE SRV QNAME service-test-2._thread-test_udp.local
    # - Pass Criteria:
    #   - The DUT MUST respond RCODE=0 with Answer section record:
    #     service-test-2._thread-test._udp.local ( SRV 1 0 44444 host-test-2.local )
    print("Step 17c: Eth 1 sends mDNS query QTYPE SRV instance 2.")
    with pkts.save_index():
        pkts.index = index_step8
        verify_mdns_srv_response(pkts, MDNS_INSTANCE_2, SRP_PORT_44444)

    # Step 17d
    # - Device: Eth 1
    # - Description (SRP-2.2): Harness instructs the device to send mDNS query
    #   QTYPE=AAAA QNAME host-test-1.local
    # - Pass Criteria:
    #   - The DUT MUST respond RCODE=0 with Answer section record:
    #     host-test-1.local AAAA <OMR address of ED_1>
    print("Step 17d: Eth 1 sends mDNS query QTYPE AAAA host 1.")
    with pkts.save_index():
        pkts.index = index_step8
        verify_mdns_aaaa_response(pkts, MDNS_HOST_1, ED_1_OMR)

    # Step 17e
    # - Device: Eth_1
    # - Description (SRP-2.2): Harness instructs the device to send mDNS query
    #   QTYPE AAAA QNAME host-test-2.local
    # - Pass Criteria:
    #   - THE DUT MUST respond RCODE=0 with Answer section record:
    #     host-test-2.local AAAA <OMR address of ED_2>
    print("Step 17e: Eth 1 sends mDNS query QTYPE AAAA host 2.")
    with pkts.save_index():
        pkts.index = index_step8
        verify_mdns_aaaa_response(pkts, MDNS_HOST_2, ED_2_OMR)


if __name__ == '__main__':
    verify_utils.run_main(verify)
