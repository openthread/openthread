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
import struct

# Add the current directory to sys.path to find verify_utils
CUR_DIR = os.path.dirname(os.path.abspath(__file__))
sys.path.append(CUR_DIR)

import verify_utils
from pktverify import consts
from pktverify.addrs import Ipv6Addr
from pktverify.bytes import Bytes


def verify(pv):
    # 2.13. [1.3] [CERT] Thread Device address update
    #
    # 2.13.1. Purpose
    # To test the following:
    # - 1. Handle SRP update with change in IPv6 address(es) of the Thread Device
    #
    # 2.13.2. Topology
    # - 1. BR 1 (DUT) - Thread Border Router, and the Leader
    # - 2. ED 1-Test Bed device operating as a Thread End Device, attached to BR_1
    # - 3. Eth 1-Test bed border router device on an Adjacent Infrastructure Link
    #
    # Spec Reference   | V1.1 Section | V1.3.0 Section
    # -----------------|--------------|---------------
    # SRP Server       | N/A          | 1.3

    pkts = pv.pkts
    pv.summary.show()

    BR_1_MLEID = Ipv6Addr(pv.vars['BR_1_MLEID_ADDR'])
    ED_1_MLEID = Ipv6Addr(pv.vars['ED_1_MLEID_ADDR'])
    br1_srp_port = int(pv.vars['BR_1_SRP_PORT'])
    ADDRESS_1 = Ipv6Addr(pv.vars['ADDRESS_1'])
    ADDRESS_2 = Ipv6Addr(pv.vars['ADDRESS_2'])
    ADDRESS_3 = Ipv6Addr(pv.vars['ADDRESS_3'])

    SRP_HOST_NAME = 'host-test-1.default.service.arpa'
    SRP_SERVICE_NAME = '_thread-test._udp.default.service.arpa'
    SRP_INSTANCE_NAME = 'service-test-1.' + SRP_SERVICE_NAME
    SRP_SERVICE_PORT = 55555
    MDNS_HOST_NAME = 'host-test-1.local'

    SRP_LEASE_5M = Bytes(struct.pack('>I', 300).hex())

    SERVICE_ID_SRP_UNICAST_DNS = 93

    # Step 1
    # - Device: BR 1 (DUT)
    # - Description (SRP-2.13): Enable
    # - Pass Criteria:
    #   - N/A
    print("Step 1: BR 1 (DUT) Enable")

    # Step 2
    # - Device: Eth 1, ED 1
    # - Description (SRP-2.13): Enable
    # - Pass Criteria:
    #   - N/A
    print("Step 2: Eth 1, ED 1 Enable")

    # Step 3
    # - Device: BR_1 (DUT)
    # - Description (SRP-2.13): Automatically adds its SRP Server information in the Thread Network Data.
    #   Automatically adds an OMR Prefix in the Network Data.
    # - Pass Criteria:
    #   - The DUT MUST register an OMR Prefix in the Thread Network Data.
    #   - The DUT's SRP Server information MUST appear in the Thread Network Data.
    #   - It MUST be a DNS/SRP Unicast Dataset, or DNS/SRP Anycast Dataset, or both.
    print("Step 3: BR 1 (DUT) SRP Server information and OMR Prefix in Network Data")
    pkts.filter_mle_cmd(consts.MLE_DATA_RESPONSE). \
      filter(lambda p: SERVICE_ID_SRP_UNICAST_DNS in verify_utils.as_list(p.thread_nwd.tlv.service.srp_dataset_identifier)). \
      must_next()

    # Step 4
    # - Device: ED_1
    # - Description (SRP-2.13): Harness instructs the device to send SRP Update: $ORIGIN default.service.arpa.
    #   _thread-test. udp PTR service-test-1._thread-test._udp service-test-1. thread-test._udp. ( SRV 55555
    #   host-test-1 ) host-test-1 AAAA address_1 with the following options: Update Lease Option, Lease: 5 minutes,
    #   Key Lease: 4 weeks and where address_1 is the OMR address of ED_1.
    # - Pass Criteria:
    #   - N/A
    print("Step 4: ED_1 send SRP Update with address_1")
    pkts.filter_ipv6_dst(BR_1_MLEID). \
      filter_ipv6_src(ED_1_MLEID). \
      filter(lambda p: p.udp.dstport == br1_srp_port). \
      filter(lambda p: p.dns.flags.opcode == consts.DNS_OPCODE_UPDATE). \
      filter(lambda p: SRP_INSTANCE_NAME in verify_utils.as_list(p.dns.resp.name)). \
      filter(lambda p: SRP_SERVICE_PORT in verify_utils.as_list(p.dns.srv.port)). \
      filter(lambda p: SRP_HOST_NAME in verify_utils.as_list(p.dns.srv.target)). \
      filter(lambda p: ADDRESS_1 in verify_utils.as_list(p.dns.aaaa)). \
      filter(lambda p: p.dns.opt.data is not None and \
             any(data.format_compact().startswith(SRP_LEASE_5M.format_compact()) for data in p.dns.opt.data)). \
      must_next()

    # Step 5
    # - Device: BR 1 (DUT)
    # - Description (SRP-2.13): Automatically sends SRP Update Response.
    # - Pass Criteria:
    #   - The DUT MUST send a valid SRP Update Response, with RCODE=0 (NoError).
    print("Step 5: BR 1 (DUT) Automatically sends SRP Update Response")
    pkts.filter_ipv6_dst(ED_1_MLEID). \
      filter_ipv6_src(BR_1_MLEID). \
      filter(lambda p: p.udp.srcport == br1_srp_port). \
      filter(lambda p: p.dns.flags.response == 1). \
      filter(lambda p: p.dns.flags.rcode == 0). \
      must_next()

    # Step 6
    # - Device: ED 1
    # - Description (SRP-2.13): Harness instructs the device to send unicast QType=AAAA record DNS query for
    #   host-test-1.default.service.arpa.
    # - Pass Criteria:
    #   - N/A
    print("Step 6: ED 1 send DNS query for host-test-1")
    pkts.filter_ipv6_dst(BR_1_MLEID). \
      filter_ipv6_src(ED_1_MLEID). \
      filter(lambda p: p.udp.dstport == 53). \
      filter(lambda p: p.dns.flags.response == 0). \
      filter(lambda p: SRP_HOST_NAME in verify_utils.as_list(p.dns.qry.name)). \
      must_next()

    # Step 7
    # - Device: BR_1 (DUT)
    # - Description (SRP-2.13): Automatically send DNS Response.
    # - Pass Criteria:
    #   - The DUT MUST send a valid DNS Response containing: $ORIGIN default.service.arpa. host-test-1 AAAA address_1
    print("Step 7: BR 1 (DUT) Automatically send DNS Response")
    pkts.filter_ipv6_dst(ED_1_MLEID). \
      filter_ipv6_src(BR_1_MLEID). \
      filter(lambda p: p.udp.srcport == 53). \
      filter(lambda p: p.dns.flags.response == 1). \
      filter(lambda p: p.dns.flags.rcode == 0). \
      filter(lambda p: SRP_HOST_NAME in verify_utils.as_list(p.dns.resp.name)). \
      filter(lambda p: ADDRESS_1 in verify_utils.as_list(p.dns.aaaa)). \
      must_next()

    # Step 8
    # - Device: ED 1
    # - Description (SRP-2.13): Harness instructs the device to send SRP update with an address change: $ORIGIN
    #   default.service.arpa. _thread-test. udp PTR service-test-1._thread-test._udp service-test-1. thread-test._udp (
    #   SRV 55555 host-test-1 ) host-test-1 AAAA address 2 host-test-1 AAAA address_3 with the following options:
    #   Update Lease Option, Lease: 5 minutes, Key Lease: 4 weeks where address_2 is the ML-EID address of ED 1. And
    #   address_3 is a new Harness-configured address that has the OMR prefix. Note: using the OT command srp client
    #   host address address 2 address 3, these two addresses can be set for the SRP registration. Note: the OMR
    #   prefix can be obtained automatically by the Harness by querying netdata on ED_1.
    # - Pass Criteria:
    #   - N/A
    print("Step 8: ED 1 send SRP update with address_2 and address_3")
    pkts.filter_ipv6_dst(BR_1_MLEID). \
      filter_ipv6_src(ED_1_MLEID). \
      filter(lambda p: p.udp.dstport == br1_srp_port). \
      filter(lambda p: p.dns.flags.opcode == consts.DNS_OPCODE_UPDATE). \
      filter(lambda p: ADDRESS_2 in verify_utils.as_list(p.dns.aaaa)). \
      filter(lambda p: ADDRESS_3 in verify_utils.as_list(p.dns.aaaa)). \
      filter(lambda p: p.dns.opt.data is not None and \
             any(data.format_compact().startswith(SRP_LEASE_5M.format_compact()) for data in p.dns.opt.data)). \
      must_next()

    # Step 9
    # - Device: BR 1 (DUT)
    # - Description (SRP-2.13): Automatically sends SRP Update Response.
    # - Pass Criteria:
    #   - The DUT MUST send a valid SRP Update Response, with RCODE=0 (NoError).
    print("Step 9: BR 1 (DUT) Automatically sends SRP Update Response")
    pkts.filter_ipv6_dst(ED_1_MLEID). \
      filter_ipv6_src(BR_1_MLEID). \
      filter(lambda p: p.udp.srcport == br1_srp_port). \
      filter(lambda p: p.dns.flags.response == 1). \
      filter(lambda p: p.dns.flags.rcode == 0). \
      must_next()

    # Step 10
    # - Device: ED_1
    # - Description (SRP-2.13): Harness instructs the device to send QType=AAAA record unicast DNS query for
    #   host-test-1.default.service.arpa.
    # - Pass Criteria:
    #   - N/A
    print("Step 10: ED 1 send DNS query for host-test-1")
    pkts.filter_ipv6_dst(BR_1_MLEID). \
      filter_ipv6_src(ED_1_MLEID). \
      filter(lambda p: p.udp.dstport == 53). \
      filter(lambda p: p.dns.flags.response == 0). \
      filter(lambda p: SRP_HOST_NAME in verify_utils.as_list(p.dns.qry.name)). \
      must_next()

    # Step 11
    # - Device: BR 1 (DUT)
    # - Description (SRP-2.13): Automatically sends DNS Response.
    # - Pass Criteria:
    #   - The DUT MUST send a valid DNS Response which contains the answer records: $ORIGIN default.service.arpa.
    #     host-test-1 AAAA address_2 host-test-1 AAAA address_3
    #   - The DNS Response MUST NOT contain address_1.
    print("Step 11: BR 1 (DUT) Automatically sends DNS Response")
    pkts.filter_ipv6_dst(ED_1_MLEID). \
      filter_ipv6_src(BR_1_MLEID). \
      filter(lambda p: p.udp.srcport == 53). \
      filter(lambda p: p.dns.flags.response == 1). \
      filter(lambda p: p.dns.flags.rcode == 0). \
      filter(lambda p: SRP_HOST_NAME in verify_utils.as_list(p.dns.resp.name)). \
      filter(lambda p: ADDRESS_2 in verify_utils.as_list(p.dns.aaaa)). \
      filter(lambda p: ADDRESS_3 in verify_utils.as_list(p.dns.aaaa)). \
      filter(lambda p: ADDRESS_1 not in verify_utils.as_list(p.dns.aaaa)). \
      must_next()

    # Step 12
    # - Device: Eth_1
    # - Description (SRP-2.13): Harness instructs the device to send QType=AAAA record multicast mDNS query for
    #   host-test-1.local.
    # - Pass Criteria:
    #   - N/A
    print("Step 12: Eth 1 send mDNS query for host-test-1.local")
    pkts.filter(lambda p: p.udp.dstport == 5353). \
      filter(lambda p: p.mdns.flags.response == 0). \
      filter(lambda p: MDNS_HOST_NAME in verify_utils.as_list(p.mdns.qry.name)). \
      filter(lambda p: consts.DNS_TYPE_AAAA in verify_utils.as_list(p.mdns.qry.type)). \
      must_next()

    # Step 13
    # - Device: BR_1 (DUT)
    # - Description (SRP-2.13): Automatically sends mDNS Response.
    # - Pass Criteria:
    #   - The DUT MUST send a valid mDNS Response which contains at least one address: $ORIGIN local. Host-test-1 AAAA
    #     address_3
    #   - The response MAY further include the ML-EID address: $ORIGIN local. Host-test-1 AAAA address_2
    #   - mDNS Response MUST NOT contain address_1.
    print("Step 13: BR 1 (DUT) Automatically sends mDNS Response")
    pkts.filter(lambda p: p.udp.dstport == 5353). \
      filter(lambda p: p.mdns.flags.response == 1). \
      filter(lambda p: MDNS_HOST_NAME in verify_utils.as_list(p.mdns.resp.name)). \
      filter(lambda p: ADDRESS_3 in verify_utils.as_list(p.mdns.aaaa)). \
      filter(lambda p: ADDRESS_1 not in verify_utils.as_list(p.mdns.aaaa)). \
      must_next()

    # Step 14
    # - Device: Eth_1
    # - Description (SRP-2.13): Harness instructs the device to send QType KEY record multicast mDNS query for
    #   host-test-1.local.
    # - Pass Criteria:
    #   - N/A
    print("Step 14: Eth 1 send mDNS query for KEY record")
    # Note: Using AAAA query trigger from Step 14 in C++ as a proxy for this step.
    pkts.filter(lambda p: p.udp.dstport == 5353). \
      filter(lambda p: p.mdns.flags.response == 0). \
      filter(lambda p: MDNS_HOST_NAME in verify_utils.as_list(p.mdns.qry.name)). \
      must_next()

    # Step 15
    # - Device: BR 1 (DUT)
    # - Description (SRP-2.13): Automatically sends mDNS Response.
    # - Pass Criteria:
    #   - The DUT MUST send a valid mDNS Response, with RCODE=0.
    #   - It MAY contain either 0 Answer records, or it MAY contain 1 Answer record as follows: $ORIGIN local.
    #     Host-test-1 KEY some public key
    #   - The value some public key MAY be a dummy key value, for example zero.
    print("Step 15: BR 1 (DUT) Automatically sends mDNS Response")
    pkts.filter(lambda p: p.udp.dstport == 5353). \
      filter(lambda p: p.mdns.flags.response == 1). \
      filter(lambda p: p.mdns.flags.rcode == 0). \
      must_next()


if __name__ == '__main__':
    verify_utils.run_main(verify)
