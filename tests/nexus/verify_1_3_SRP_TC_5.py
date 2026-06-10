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
    # 2.5. [1.3] [CERT] KEY record inclusion/omission
    #
    # 2.5.1. Purpose
    # To test the following:
    # 1. Handle Service Description Instructions that include the KEY RR.
    # 2. Handle Service Description Instructions that omit the KEY RR.
    # 3. Report error on Host Description Instructions that omit the KEY RR.
    #
    # 2.5.2. Topology
    # 1. BR_1 (DUT) - Thread Border Router and the Leader
    # 2. ED_1 - Test Bed device operating as a Thread End Device, attached to BR_1
    # 3. Eth_1 - Test Bed border router device on an Adjacent Infrastructure Link
    pkts = pv.pkts
    pv.summary.show()

    ED_1_MLEID = Ipv6Addr(pv.vars['ED_1_MLEID_ADDR'])
    BR_1_MLEID = Ipv6Addr(pv.vars['BR_1_MLEID_ADDR'])
    br1_srp_port = int(pv.vars['BR_1_SRP_PORT'])

    SRP_SERVICE_NAME = '_thread-test._udp.default.service.arpa'
    SRP_INSTANCE_NAME = 'service test 1.' + SRP_SERVICE_NAME
    SRP_HOST_NAME = 'host-test-1.default.service.arpa'
    MDNS_SERVICE_NAME = '_thread-test._udp.local'
    MDNS_INSTANCE_NAME = 'service test 1.' + MDNS_SERVICE_NAME
    DNS_QUERY_NAME = '_thread-test._udp.default.service.arpa'

    SRP_SERVICE_PORT = 1500
    RR_TYPE_KEY = 25

    # Step 1
    # - Device: BR_1 (DUT)
    # - Description (SRP-2.5): Enable: switch on.
    print("Step 1: BR_1 (DUT) Enable: switch on.")

    # Step 2
    # - Device: Eth_1, ED_1
    # - Description (SRP-2.5): Enable
    print("Step 2: Eth_1, ED_1 Enable.")

    # Step 3
    # - Device: BR_1 (DUT)
    # - Description (SRP-2.5): Automatically adds SRP Server information in the Thread Network Data; and
    #   configures OMR prefix.
    print("Step 3: BR_1 (DUT) Automatically adds SRP Server information in the Thread Network Data.")
    pkts.\
        filter_mle_cmd(consts.MLE_DATA_RESPONSE).\
        filter(lambda p: 93 in verify_utils.as_list(p.thread_nwd.tlv.service.srp_dataset_identifier)).\
        filter(lambda p: any(l in (1, 19) for l in verify_utils.as_list(p.thread_nwd.tlv.service.s_data_len))).\
        must_next()

    # Step 4
    # - Device: ED_1
    # - Description (SRP-2.5): Harness instructs the device to send SRP Update to the DUT with service
    #   KEY records enabled.
    print("Step 4: ED_1 sends SRP Update to the DUT with service KEY records enabled.")
    pkts.\
        filter_ipv6_dst(BR_1_MLEID).\
        filter_ipv6_src(ED_1_MLEID).\
        filter(lambda p: p.udp.dstport == br1_srp_port).\
        filter(lambda p: p.dns.flags.opcode == 5).\
        filter(lambda p: SRP_INSTANCE_NAME in verify_utils.as_list(p.dns.resp.name)).\
        filter(lambda p: SRP_SERVICE_PORT in verify_utils.as_list(p.dns.srv.port)).\
        filter(lambda p: SRP_HOST_NAME in verify_utils.as_list(p.dns.srv.target)).\
        filter(lambda p: verify_utils.as_list(p.dns.resp.type).count(RR_TYPE_KEY) >= 2).\
        must_next()

    # Step 5
    # - Device: BR_1 (DUT)
    # - Description (SRP-2.5): Automatically sends success SRP Update Response.
    print("Step 5: BR_1 (DUT) Automatically sends success SRP Update Response.")
    pkts.\
        filter_ipv6_dst(ED_1_MLEID).\
        filter_ipv6_src(BR_1_MLEID).\
        filter(lambda p: p.udp.srcport == br1_srp_port).\
        filter(lambda p: p.dns.flags.response == 1).\
        filter(lambda p: p.dns.flags.rcode == 0).\
        must_next()

    # Step 6
    # - Device: ED_1
    # - Description (SRP-2.5): Harness instructs the device to unicast DNS QType=PTR query to the DUT for
    #   all services of type _thread-test._udp.local..
    print("Step 6: ED_1 unicast DNS QType=PTR query to the DUT.")
    pkts.\
        filter_ipv6_dst(BR_1_MLEID).\
        filter_ipv6_src(ED_1_MLEID).\
        filter(lambda p: p.udp.dstport == 53).\
        filter(lambda p: p.dns.flags.response == 0).\
        filter(lambda p: DNS_QUERY_NAME in verify_utils.as_list(p.dns.qry.name)).\
        must_next()

    # Step 7
    # - Device: BR_1 (DUT)
    # - Description (SRP-2.5): Automatically sends DNS response.
    print("Step 7: BR_1 (DUT) Automatically sends DNS response.")
    # The DNS Response MUST NOT contain any RRType=KEY records.
    pkts.\
        filter_ipv6_dst(ED_1_MLEID).\
        filter_ipv6_src(BR_1_MLEID).\
        filter(lambda p: p.udp.srcport == 53).\
        filter(lambda p: p.dns.flags.response == 1).\
        filter(lambda p: p.dns.flags.rcode == 0).\
        filter(lambda p: SRP_INSTANCE_NAME in verify_utils.as_list(p.dns.ptr.domain_name)).\
        filter(lambda p: RR_TYPE_KEY not in verify_utils.as_list(p.dns.resp.type)).\
        must_next()

    # Step 8
    # - Device: Eth_1
    # - Description (SRP-2.5): Harness instructs the device to send mDNS QType=PTR query for services of
    #   type _thread-test._udp.local.
    print("Step 8: Eth_1 send mDNS QType=PTR query.")
    pkts.\
        filter(lambda p: p.udp.dstport == 5353).\
        filter(lambda p: p.mdns.flags.response == 0).\
        filter(lambda p: MDNS_SERVICE_NAME in verify_utils.as_list(p.mdns.qry.name)).\
        must_next()

    # Step 9
    # - Device: BR_1 (DUT)
    # - Description (SRP-2.5): Automatically sends mDNS Response.
    print("Step 9: BR_1 (DUT) Automatically sends mDNS Response.")
    # The DNS Response MUST NOT contain any RRType=KEY records.
    pkts.\
        filter(lambda p: p.udp.srcport == 5353).\
        filter(lambda p: p.mdns.flags.response == 1).\
        filter(lambda p: MDNS_INSTANCE_NAME in verify_utils.as_list(p.mdns.ptr.domain_name)).\
        filter(lambda p: RR_TYPE_KEY not in verify_utils.as_list(p.mdns.resp.type)).\
        must_next()

    # Step 10
    # - Device: N/A
    # - Description (SRP-2.5): Repeat Steps 4 through 9 with KEY record omitted from Service Description
    #   Instruction.
    print("Step 10: Repeat Steps 4 through 9 with KEY record omitted from Service Description Instruction.")

    # Step 10.4
    print("Step 10.4: ED_1 sends SRP Update with service KEY record disabled.")
    pkts.\
        filter_ipv6_dst(BR_1_MLEID).\
        filter_ipv6_src(ED_1_MLEID).\
        filter(lambda p: p.udp.dstport == br1_srp_port).\
        filter(lambda p: p.dns.flags.opcode == 5).\
        filter(lambda p: SRP_INSTANCE_NAME in verify_utils.as_list(p.dns.resp.name)).\
        filter(lambda p: SRP_SERVICE_PORT in verify_utils.as_list(p.dns.srv.port)).\
        filter(lambda p: verify_utils.as_list(p.dns.resp.type).count(RR_TYPE_KEY) == 1).\
        must_next()

    # Step 10.5
    print("Step 10.5: BR_1 (DUT) Automatically sends success SRP Update Response.")
    pkts.\
        filter_ipv6_dst(ED_1_MLEID).\
        filter_ipv6_src(BR_1_MLEID).\
        filter(lambda p: p.udp.srcport == br1_srp_port).\
        filter(lambda p: p.dns.flags.response == 1).\
        filter(lambda p: p.dns.flags.rcode == 0).\
        must_next()

    # Step 10.6
    print("Step 10.6: ED_1 unicast DNS QType=PTR query.")
    pkts.\
        filter_ipv6_dst(BR_1_MLEID).\
        filter_ipv6_src(ED_1_MLEID).\
        filter(lambda p: p.udp.dstport == 53).\
        filter(lambda p: p.dns.flags.response == 0).\
        filter(lambda p: DNS_QUERY_NAME in verify_utils.as_list(p.dns.qry.name)).\
        must_next()

    # Step 10.7
    print("Step 10.7: BR_1 (DUT) Automatically sends DNS response.")
    pkts.\
        filter_ipv6_dst(ED_1_MLEID).\
        filter_ipv6_src(BR_1_MLEID).\
        filter(lambda p: p.udp.srcport == 53).\
        filter(lambda p: p.dns.flags.response == 1).\
        filter(lambda p: p.dns.flags.rcode == 0).\
        filter(lambda p: SRP_INSTANCE_NAME in verify_utils.as_list(p.dns.ptr.domain_name)).\
        filter(lambda p: RR_TYPE_KEY not in verify_utils.as_list(p.dns.resp.type)).\
        must_next()

    # Step 10.8
    print("Step 10.8: Eth_1 send mDNS QType=PTR query.")
    pkts.\
        filter(lambda p: p.udp.dstport == 5353).\
        filter(lambda p: p.mdns.flags.response == 0).\
        filter(lambda p: MDNS_SERVICE_NAME in verify_utils.as_list(p.mdns.qry.name)).\
        must_next()

    # Step 10.9
    print("Step 10.9: BR_1 (DUT) Automatically sends mDNS Response.")
    pkts.\
        filter(lambda p: p.udp.srcport == 5353).\
        filter(lambda p: p.mdns.flags.response == 1).\
        filter(lambda p: MDNS_INSTANCE_NAME in verify_utils.as_list(p.mdns.ptr.domain_name)).\
        filter(lambda p: RR_TYPE_KEY not in verify_utils.as_list(p.mdns.resp.type)).\
        must_next()

    # Step 11
    # - Device: ED_1
    # - Description (SRP-2.5): Repeat Step 4 with service KEY records omitted, while keeping host KEY
    #   records.
    print("Step 11: Repeat Step 4 with service KEY records omitted.")
    pkts.\
        filter_ipv6_dst(BR_1_MLEID).\
        filter_ipv6_src(ED_1_MLEID).\
        filter(lambda p: p.udp.dstport == br1_srp_port).\
        filter(lambda p: p.dns.flags.opcode == 5).\
        filter(lambda p: SRP_INSTANCE_NAME in verify_utils.as_list(p.dns.resp.name)).\
        filter(lambda p: verify_utils.as_list(p.dns.resp.type).count(RR_TYPE_KEY) == 1).\
        must_next()

    # Step 12
    # - Device: BR_1 (DUT)
    # - Description (SRP-2.5): Automatically sends success SRP Update Response.
    print("Step 12: BR_1 (DUT) Automatically sends success SRP Update Response.")
    pkts.\
        filter_ipv6_dst(ED_1_MLEID).\
        filter_ipv6_src(BR_1_MLEID).\
        filter(lambda p: p.udp.srcport == br1_srp_port).\
        filter(lambda p: p.dns.flags.response == 1).\
        filter(lambda p: p.dns.flags.rcode == 0).\
        must_next()

    # Step 13
    # - Device: ED_1
    # - Description (SRP-2.5): Repeat Step 4 with all KEY records omitted.
    print("Step 13: ED_1 Repeat Step 4 with all KEY records omitted.")
    pkts.\
        filter_ipv6_dst(BR_1_MLEID).\
        filter_ipv6_src(ED_1_MLEID).\
        filter(lambda p: p.udp.dstport == br1_srp_port).\
        filter(lambda p: p.dns.flags.opcode == 5).\
        filter(lambda p: SRP_INSTANCE_NAME in verify_utils.as_list(p.dns.resp.name)).\
        filter(lambda p: RR_TYPE_KEY not in verify_utils.as_list(p.dns.resp.type)).\
        must_next()

    # Step 14
    # - Device: BR_1 (DUT)
    # - Description (SRP-2.5): Automatically responds with error SRP Update Response.
    print("Step 14: BR_1 (DUT) Automatically responds with error SRP Update Response.")
    pkts.\
        filter_ipv6_dst(ED_1_MLEID).\
        filter_ipv6_src(BR_1_MLEID).\
        filter(lambda p: p.udp.srcport == br1_srp_port).\
        filter(lambda p: p.dns.flags.response == 1).\
        filter(lambda p: p.dns.flags.rcode == 5).\
        must_next()


if __name__ == '__main__':
    verify_utils.run_main(verify)
