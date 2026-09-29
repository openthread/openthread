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
from pktverify.addrs import Ipv6Addr, EthAddr


def verify(pv):
    #
    # 2.6. [1.3] [CERT] Name Compression
    #
    # 2.6.1. Purpose
    # To test the following:
    # - 1. Handle RRs without name compression.
    # - 2. Handle RRs with name compression
    #
    # 2.6.2. Topology
    # - 1. BR 1 (DUT) - Thread Border Router and the Leader
    # - 2. ED 1-Test Bed device operating as a Thread End Device, attached to BR_1
    # - 3. Eth 1-Test Bed border router device on an Adjacent Infrastructure Link
    #
    # Spec Reference | V1.1 Section | V1.3.0 Section
    # ---------------|--------------|---------------
    # SRP Server     | N/A          | 1.3
    pkts = pv.pkts
    pv.summary.show()

    ED_1_MLEID = Ipv6Addr(pv.vars['ED_1_MLEID_ADDR'])
    ED_1_OMR = Ipv6Addr(pv.vars['ED_1_OMR_ADDR'])
    BR_1_MLEID = Ipv6Addr(pv.vars['BR_1_MLEID_ADDR'])
    br1_srp_port = int(pv.vars['BR_1_SRP_PORT'])
    BR_1_ETH = EthAddr(pv.vars['BR_1_ETH'])

    SRP_SERVICE_NAME = '_thread-test._udp.default.service.arpa'
    SRP_INSTANCE_NAME = 'service-test-1.' + SRP_SERVICE_NAME
    SRP_HOST_NAME = 'host-test-1.default.service.arpa'
    MDNS_SERVICE_NAME = '_thread-test._udp.local'
    MDNS_INSTANCE_NAME = 'service-test-1.' + MDNS_SERVICE_NAME
    MDNS_HOST_NAME = 'host-test-1.local'

    SRP_SERVICE_PORT = 55555

    SERVICE_ID_SRP_SERVER = 93

    # Step 1: BR 1 (DUT) Enable.
    print("Step 1: BR 1 (DUT) Enable.")

    # Step 2: Eth 1, ED 1 Enable.
    print("Step 2: Eth 1, ED 1 Enable.")

    # Step 3: BR 1 (DUT) adds its SRP Server information in the Thread Network Data.
    print("Step 3: BR 1 (DUT) adds its SRP Server information in the Thread Network Data.")
    pkts.\
        filter_mle_cmd(consts.MLE_DATA_RESPONSE).\
        filter(lambda p: SERVICE_ID_SRP_SERVER in verify_utils.as_list(p.thread_nwd.tlv.service.srp_dataset_identifier)).\
        filter(lambda p: any(l in (1, 19) for l in verify_utils.as_list(p.thread_nwd.tlv.service.s_data_len))).\
        must_next()

    # Step 4: ED 1 sends SRP Update without name compression.
    print("Step 4: ED 1 sends SRP Update without name compression.")
    pkts.\
        filter_ipv6_dst(BR_1_MLEID).\
        filter_ipv6_src(ED_1_MLEID).\
        filter(lambda p: p.udp.dstport == br1_srp_port).\
        filter(lambda p: p.dns.flags.opcode == 5).\
        filter(lambda p: not verify_utils.is_dns_compression_used(p)).\
        filter(lambda p: SRP_INSTANCE_NAME in verify_utils.as_list(p.dns.resp.name)).\
        filter(lambda p: SRP_SERVICE_PORT in verify_utils.as_list(p.dns.srv.port)).\
        filter(lambda p: SRP_HOST_NAME in verify_utils.as_list(p.dns.srv.target)).\
        filter(lambda p: ED_1_OMR in verify_utils.as_list(p.dns.aaaa)).\
        must_next()

    # Step 5: BR 1 (DUT) sends SRP Update Response.
    print("Step 5: BR 1 (DUT) sends SRP Update Response.")
    pkts.\
        filter_ipv6_dst(ED_1_MLEID).\
        filter_ipv6_src(BR_1_MLEID).\
        filter(lambda p: p.udp.srcport == br1_srp_port).\
        filter(lambda p: p.dns.flags.response == 1).\
        filter(lambda p: p.dns.flags.rcode == 0).\
        must_next()

    # Step 6: ED 1 sends unicast DNS QType PTR query.
    print("Step 6: ED 1 sends unicast DNS QType PTR query.")
    pkts.\
        filter_ipv6_dst(BR_1_MLEID).\
        filter_ipv6_src(ED_1_MLEID).\
        filter(lambda p: p.udp.dstport == 53).\
        filter(lambda p: p.dns.flags.response == 0).\
        filter(lambda p: SRP_SERVICE_NAME in verify_utils.as_list(p.dns.qry.name)).\
        must_next()

    # Step 7: BR 1 (DUT) sends DNS response.
    print("Step 7: BR 1 (DUT) sends DNS response.")
    pkts.\
        filter_ipv6_dst(ED_1_MLEID).\
        filter_ipv6_src(BR_1_MLEID).\
        filter(lambda p: p.udp.srcport == 53).\
        filter(lambda p: p.dns.flags.response == 1).\
        filter(lambda p: p.dns.flags.rcode == 0).\
        filter(lambda p: SRP_INSTANCE_NAME in verify_utils.as_list(p.dns.ptr.domain_name)).\
        must_next()

    # Step 8: Eth 1 sends mDNS query QType=PTR.
    print("Step 8: Eth 1 sends mDNS query QType=PTR.")
    pkts.\
        filter(lambda p: p.udp.dstport == 5353).\
        filter(lambda p: p.mdns.flags.response == 0).\
        filter(lambda p: MDNS_SERVICE_NAME in verify_utils.as_list(p.mdns.qry.name)).\
        must_next()

    # Step 9: BR 1 (DUT) sends mDNS Response.
    print("Step 9: BR 1 (DUT) sends mDNS Response.")
    pkts.\
        filter_eth_src(BR_1_ETH).\
        filter(lambda p: p.udp.dstport == 5353).\
        filter(lambda p: p.mdns.flags.response == 1).\
        filter(lambda p: MDNS_SERVICE_NAME in verify_utils.as_list(p.mdns.resp.name)).\
        filter(lambda p: MDNS_INSTANCE_NAME in verify_utils.as_list(p.mdns.ptr.domain_name)).\
        must_next()

    # Step 9b: Eth 1 sends mDNS query QTYPE=SRV.
    print("Step 9b: Eth 1 sends mDNS query QTYPE=SRV.")
    pkts.\
        filter(lambda p: p.udp.dstport == 5353).\
        filter(lambda p: p.mdns.flags.response == 0).\
        filter(lambda p: MDNS_INSTANCE_NAME in verify_utils.as_list(p.mdns.qry.name)).\
        must_next()

    pkts.\
        filter_eth_src(BR_1_ETH).\
        filter(lambda p: p.udp.dstport == 5353).\
        filter(lambda p: p.mdns.flags.response == 1).\
        filter(lambda p: MDNS_INSTANCE_NAME in verify_utils.as_list(p.mdns.resp.name)).\
        filter(lambda p: SRP_SERVICE_PORT in verify_utils.as_list(p.mdns.srv.port)).\
        filter(lambda p: MDNS_HOST_NAME in verify_utils.as_list(p.mdns.srv.target)).\
        must_next()

    # Step 10: Repeat Steps 4 through 9b with name compression.
    print("Step 10: Repeat Steps 4 through 9b with name compression.")

    # Step 4 (Repeat)
    print("Step 4 (Repeat): ED 1 sends SRP Update with name compression.")
    pkts.\
        filter_ipv6_dst(BR_1_MLEID).\
        filter_ipv6_src(ED_1_MLEID).\
        filter(lambda p: p.udp.dstport == br1_srp_port).\
        filter(lambda p: p.dns.flags.opcode == 5).\
        filter(lambda p: verify_utils.is_dns_compression_used(p)).\
        filter(lambda p: SRP_INSTANCE_NAME in verify_utils.as_list(p.dns.resp.name)).\
        filter(lambda p: SRP_SERVICE_PORT in verify_utils.as_list(p.dns.srv.port)).\
        filter(lambda p: SRP_HOST_NAME in verify_utils.as_list(p.dns.srv.target)).\
        filter(lambda p: ED_1_OMR in verify_utils.as_list(p.dns.aaaa)).\
        must_next()

    # Step 5 (Repeat)
    print("Step 5 (Repeat): BR 1 (DUT) sends SRP Update Response.")
    pkts.\
        filter_ipv6_dst(ED_1_MLEID).\
        filter_ipv6_src(BR_1_MLEID).\
        filter(lambda p: p.udp.srcport == br1_srp_port).\
        filter(lambda p: p.dns.flags.response == 1).\
        filter(lambda p: p.dns.flags.rcode == 0).\
        must_next()

    # Step 6 (Repeat)
    print("Step 6 (Repeat): ED 1 sends unicast DNS QType PTR query.")
    pkts.\
        filter_ipv6_dst(BR_1_MLEID).\
        filter_ipv6_src(ED_1_MLEID).\
        filter(lambda p: p.udp.dstport == 53).\
        filter(lambda p: p.dns.flags.response == 0).\
        filter(lambda p: SRP_SERVICE_NAME in verify_utils.as_list(p.dns.qry.name)).\
        must_next()

    # Step 7 (Repeat)
    print("Step 7 (Repeat): BR 1 (DUT) sends DNS response.")
    pkts.\
        filter_ipv6_dst(ED_1_MLEID).\
        filter_ipv6_src(BR_1_MLEID).\
        filter(lambda p: p.udp.srcport == 53).\
        filter(lambda p: p.dns.flags.response == 1).\
        filter(lambda p: p.dns.flags.rcode == 0).\
        filter(lambda p: SRP_INSTANCE_NAME in verify_utils.as_list(p.dns.ptr.domain_name)).\
        must_next()

    # Step 8 (Repeat)
    print("Step 8 (Repeat): Eth 1 sends mDNS query QType=PTR.")
    pkts.\
        filter(lambda p: p.udp.dstport == 5353).\
        filter(lambda p: p.mdns.flags.response == 0).\
        filter(lambda p: MDNS_SERVICE_NAME in verify_utils.as_list(p.mdns.qry.name)).\
        must_next()

    pkts.\
        filter_eth_src(BR_1_ETH).\
        filter(lambda p: p.udp.dstport == 5353).\
        filter(lambda p: p.mdns.flags.response == 1).\
        filter(lambda p: MDNS_SERVICE_NAME in verify_utils.as_list(p.mdns.resp.name)).\
        filter(lambda p: MDNS_INSTANCE_NAME in verify_utils.as_list(p.mdns.ptr.domain_name)).\
        must_next()

    # Step 9 (Repeat)
    print("Step 9 (Repeat): BR 1 (DUT) sends mDNS Response.")

    # Step 9b (Repeat)
    print("Step 9b (Repeat): Eth 1 sends mDNS query QTYPE=SRV.")
    pkts.\
        filter(lambda p: p.udp.dstport == 5353).\
        filter(lambda p: p.mdns.flags.response == 0).\
        filter(lambda p: MDNS_INSTANCE_NAME in verify_utils.as_list(p.mdns.qry.name)).\
        must_next()

    pkts.\
        filter_eth_src(BR_1_ETH).\
        filter(lambda p: p.udp.dstport == 5353).\
        filter(lambda p: p.mdns.flags.response == 1).\
        filter(lambda p: MDNS_INSTANCE_NAME in verify_utils.as_list(p.mdns.resp.name)).\
        filter(lambda p: SRP_SERVICE_PORT in verify_utils.as_list(p.mdns.srv.port)).\
        filter(lambda p: MDNS_HOST_NAME in verify_utils.as_list(p.mdns.srv.target)).\
        must_next()


if __name__ == '__main__':
    verify_utils.run_main(verify)
