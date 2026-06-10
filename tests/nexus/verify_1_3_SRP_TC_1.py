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
    # 5.10.1 SRP_TC_1: SRP Registration and Discovery (Single BR)
    #
    # 5.10.1.2 Purpose & Description
    # The purpose of this test case is to verify the SRP registration and discovery in a single BR topology.
    #
    # 5.10.1.3 Topology
    # BR 1 (DUT)
    # ED 1
    # Eth 1
    #
    # Spec Reference   | V1.1 Section | V1.3.0 Section
    # -----------------|--------------|---------------
    # SRP Server       | N/A          | 1.3
    pkts = pv.pkts
    pv.summary.show()

    # Use ML-EIDs for SRP traffic verification
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
    SRP_UPDATED_PORT = 55556

    SERVICE_ID_SRP_SERVER = 93

    # Step 1: BR 1 (DUT) power on.
    print("Step 1: BR 1 (DUT) power on.")

    # Step 2: Eth 1, ED 1 Enable.
    print("Step 2: Eth 1, ED 1 Enable.")

    # Step 3: BR 1 (DUT) adds its SRP Server information in the Thread Network Data.
    print("Step 3: BR 1 (DUT) adds its SRP Server information in the Thread Network Data.")
    pkts.\
        filter_mle_cmd(consts.MLE_DATA_RESPONSE).\
        filter(lambda p: SERVICE_ID_SRP_SERVER in verify_utils.as_list(p.thread_nwd.tlv.service.srp_dataset_identifier)).\
        filter(lambda p: any(l in (1, 19) for l in verify_utils.as_list(p.thread_nwd.tlv.service.s_data_len))).\
        must_next()

    # Step 4: ED 1 sends SRP Update to register a service.
    print("Step 4: ED 1 sends SRP Update to register a service.")
    index_era_1 = pkts.index
    pkts.\
        filter_ipv6_dst(BR_1_MLEID).\
        filter_ipv6_src(ED_1_MLEID).\
        filter(lambda p: p.udp.dstport == br1_srp_port).\
        filter(lambda p: p.dns.flags.opcode == 5).\
        filter(lambda p: SRP_INSTANCE_NAME in verify_utils.as_list(p.dns.resp.name)).\
        filter(lambda p: SRP_SERVICE_PORT in verify_utils.as_list(p.dns.srv.port)).\
        filter(lambda p: SRP_HOST_NAME in verify_utils.as_list(p.dns.srv.target)).\
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

    # Step 6: ED 1 sends unicast DNS query QType PTR.
    print("Step 6: ED 1 sends unicast DNS query QType PTR.")
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

    # Step 7b: ED 1 sends unicast DNS query QTYPE SRV.
    print("Step 7b: ED 1 sends unicast DNS query QTYPE SRV.")
    pkts.\
        filter_ipv6_dst(BR_1_MLEID).\
        filter_ipv6_src(ED_1_MLEID).\
        filter(lambda p: p.udp.dstport == 53).\
        filter(lambda p: p.dns.flags.response == 0).\
        filter(lambda p: SRP_INSTANCE_NAME in verify_utils.as_list(p.dns.qry.name)).\
        must_next()
    pkts.\
        filter_ipv6_dst(ED_1_MLEID).\
        filter_ipv6_src(BR_1_MLEID).\
        filter(lambda p: p.udp.srcport == 53).\
        filter(lambda p: p.dns.flags.response == 1).\
        filter(lambda p: SRP_SERVICE_PORT in verify_utils.as_list(p.dns.srv.port)).\
        must_next()

    # Step 7c: ED 1 sends unicast DNS query QTYPE=AAAA.
    print("Step 7c: ED 1 sends unicast DNS query QTYPE=AAAA.")
    pkts.\
        filter_ipv6_dst(BR_1_MLEID).\
        filter_ipv6_src(ED_1_MLEID).\
        filter(lambda p: p.udp.dstport == 53).\
        filter(lambda p: p.dns.flags.response == 0).\
        filter(lambda p: SRP_HOST_NAME in verify_utils.as_list(p.dns.qry.name)).\
        must_next()
    pkts.\
        filter_ipv6_dst(ED_1_MLEID).\
        filter_ipv6_src(BR_1_MLEID).\
        filter(lambda p: p.udp.srcport == 53).\
        filter(lambda p: p.dns.flags.response == 1).\
        filter(lambda p: ED_1_MLEID in verify_utils.as_list(p.dns.aaaa) and ED_1_OMR in verify_utils.as_list(p.dns.aaaa)).\
        must_next()

    # mDNS Discovery 1 (Step 8/9 era)
    print("Step 8/9: mDNS discovery on AIL.")
    with pkts.save_index():
        pkts.index = index_era_1
        era_mdns_55555 = pkts.\
            filter(lambda p: p.eth.src == BR_1_ETH).\
            filter(lambda p: p.udp.dstport == 5353).\
            filter(lambda p: p.mdns.flags.response == 1).\
            filter(lambda p: SRP_SERVICE_PORT in verify_utils.as_list(p.mdns.srv.port)).\
            must_next()
        assert MDNS_INSTANCE_NAME in verify_utils.as_list(era_mdns_55555.mdns.ptr.domain_name)
        assert ED_1_OMR in verify_utils.as_list(era_mdns_55555.mdns.aaaa)

    # Step 10: ED 1 sends SRP Update to update the service parameters and address.
    print("Step 10: ED 1 sends SRP Update to update the service parameters and address.")
    index_era_2 = pkts.index
    pkts.\
        filter_ipv6_dst(BR_1_MLEID).\
        filter_ipv6_src(ED_1_MLEID).\
        filter(lambda p: p.udp.dstport == br1_srp_port).\
        filter(lambda p: p.dns.flags.opcode == 5).\
        filter(lambda p: SRP_INSTANCE_NAME in verify_utils.as_list(p.dns.resp.name)).\
        filter(lambda p: SRP_UPDATED_PORT in verify_utils.as_list(p.dns.srv.port)).\
        must_next()

    # Step 11: BR 1 (DUT) sends SRP Update Response.
    print("Step 11: BR 1 (DUT) sends SRP Update Response.")
    pkts.\
        filter_ipv6_dst(ED_1_MLEID).\
        filter_ipv6_src(BR_1_MLEID).\
        filter(lambda p: p.udp.srcport == br1_srp_port).\
        filter(lambda p: p.dns.flags.response == 1).\
        filter(lambda p: p.dns.flags.rcode == 0).\
        must_next()

    # Step 12: ED 1 sends unicast DNS query QType PTR.
    print("Step 12: ED 1 sends unicast DNS query QType PTR.")
    pkts.\
        filter_ipv6_dst(BR_1_MLEID).\
        filter_ipv6_src(ED_1_MLEID).\
        filter(lambda p: p.udp.dstport == 53).\
        filter(lambda p: p.dns.flags.response == 0).\
        filter(lambda p: SRP_SERVICE_NAME in verify_utils.as_list(p.dns.qry.name)).\
        must_next()

    # Step 13: BR 1 (DUT) sends DNS response.
    print("Step 13: BR 1 (DUT) sends DNS response.")
    pkts.\
        filter_ipv6_dst(ED_1_MLEID).\
        filter_ipv6_src(BR_1_MLEID).\
        filter(lambda p: p.udp.srcport == 53).\
        filter(lambda p: p.dns.flags.response == 1).\
        filter(lambda p: p.dns.flags.rcode == 0).\
        filter(lambda p: SRP_INSTANCE_NAME in verify_utils.as_list(p.dns.ptr.domain_name)).\
        must_next()

    # Step 13b: ED 1 sends unicast DNS query QTYPE SRV.
    print("Step 13b: ED 1 sends unicast DNS query QTYPE SRV.")
    pkts.\
        filter_ipv6_dst(BR_1_MLEID).\
        filter_ipv6_src(ED_1_MLEID).\
        filter(lambda p: p.udp.dstport == 53).\
        filter(lambda p: p.dns.flags.response == 0).\
        filter(lambda p: SRP_INSTANCE_NAME in verify_utils.as_list(p.dns.qry.name)).\
        must_next()
    pkts.\
        filter_ipv6_dst(ED_1_MLEID).\
        filter_ipv6_src(BR_1_MLEID).\
        filter(lambda p: p.udp.srcport == 53).\
        filter(lambda p: p.dns.flags.response == 1).\
        filter(lambda p: SRP_UPDATED_PORT in verify_utils.as_list(p.dns.srv.port)).\
        must_next()

    # Step 13c: ED 1 sends unicast DNS query QTYPE=AAAA.
    print("Step 13c: ED 1 sends unicast DNS query QTYPE=AAAA.")
    pkts.\
        filter_ipv6_dst(BR_1_MLEID).\
        filter_ipv6_src(ED_1_MLEID).\
        filter(lambda p: p.udp.dstport == 53).\
        filter(lambda p: p.dns.flags.response == 0).\
        filter(lambda p: SRP_HOST_NAME in verify_utils.as_list(p.dns.qry.name)).\
        must_next()
    pkts.\
        filter_ipv6_dst(ED_1_MLEID).\
        filter_ipv6_src(BR_1_MLEID).\
        filter(lambda p: p.udp.srcport == 53).\
        filter(lambda p: p.dns.flags.response == 1).\
        filter(lambda p: all(addr == ED_1_MLEID for addr in verify_utils.as_list(p.dns.aaaa))).\
        must_next()

    # mDNS Discovery 2 (Step 14/15 era)
    print("Step 14/15: updated mDNS discovery on AIL.")
    with pkts.save_index():
        pkts.index = index_era_2
        pkts.\
            filter(lambda p: p.eth.src == BR_1_ETH).\
            filter(lambda p: p.udp.dstport == 5353).\
            filter(lambda p: p.mdns.flags.response == 1).\
            filter(lambda p: SRP_UPDATED_PORT in verify_utils.as_list(p.mdns.srv.port)).\
            must_next()
    with pkts.save_index():
        pkts.index = index_era_2
        pkts.\
            filter(lambda p: p.eth.src == BR_1_ETH).\
            filter(lambda p: p.udp.dstport == 5353).\
            filter(lambda p: p.mdns.flags.response == 1).\
            filter(lambda p: MDNS_INSTANCE_NAME in verify_utils.as_list(p.mdns.ptr.domain_name)).\
            must_next()


if __name__ == '__main__':
    verify_utils.run_main(verify)
