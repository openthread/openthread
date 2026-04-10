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
    # 2.8. [1.3] [CERT] Removing Some Published Services
    #
    # 2.8.1. Purpose
    # To test the following:
    # - 1. Remove only selected service instances, not all.
    #
    # 2.8.2. Topology
    # - BR 1 (DUT) - Thread Border Router and the Leader
    # - ED 1-Test Bed device operating as a Thread End Device, attached to BR_1
    # - Eth 1-Test Bed border router device on an Adjacent Infrastructure Link
    #
    # Spec Reference   | V1.1 Section | V1.3.0 Section
    # -----------------|--------------|---------------
    # SRP Server       | N/A          | 1.3

    pkts = pv.pkts
    pv.summary.show()

    ED_1_MLEID = Ipv6Addr(pv.vars['ED_1_MLEID_ADDR'])
    BR_1_MLEID = Ipv6Addr(pv.vars['BR_1_MLEID_ADDR'])
    br1_srp_port = int(pv.vars['BR_1_SRP_PORT'])

    SRP_SERVICE_NAME = '_thread-test._udp.default.service.arpa'
    SRP_INSTANCE_NAME_1 = 'service-test-1.' + SRP_SERVICE_NAME
    SRP_INSTANCE_NAME_2 = 'service-test-2.' + SRP_SERVICE_NAME
    SRP_HOST_NAME = 'host-test-1.default.service.arpa'
    MDNS_SERVICE_NAME = '_thread-test._udp.local'
    MDNS_INSTANCE_NAME_1 = 'service-test-1.' + MDNS_SERVICE_NAME
    MDNS_INSTANCE_NAME_2 = 'service-test-2.' + MDNS_SERVICE_NAME

    SRP_SERVICE_PORT_1 = 55555
    SRP_SERVICE_PORT_2 = 55556
    SRP_SERVICE_PRIORITY_2 = 8

    SRP_LEASE_10M_20M = Bytes(struct.pack('>II', 600, 1200).hex())
    SRP_LEASE_10M_10M = Bytes(struct.pack('>II', 600, 600).hex())

    SERVICE_ID_SRP_UNICAST_DNS = 93

    # Step 1
    #   Device: BR 1 (DUT)
    #   Description (SRP-2.8): Enable
    #   Pass Criteria:
    #     - N/A
    print("Step 1: BR 1 (DUT) Enable")

    # Step 2
    #   Device: Eth 1, ED 1
    #   Description (SRP-2.8): Enable
    #   Pass Criteria:
    #     - N/A
    print("Step 2: Eth 1, ED 1 Enable")

    # Step 3
    #   Device: BR_1 (DUT)
    #   Description (SRP-2.8): Automatically adds SRP Server information in the Thread Network Data. Automatically adds
    #     OMR prefix in Network Data.
    #   Pass Criteria:
    #     - [T1-T6] The DUT's SRP Server information MUST appear in the Thread Network Data.
    print("Step 3: BR 1 (DUT) SRP Server info in Network Data")
    pkts. \
      filter_mle_cmd(consts.MLE_DATA_RESPONSE). \
      filter(lambda p: SERVICE_ID_SRP_UNICAST_DNS in verify_utils.as_list(p.thread_nwd.tlv.service.srp_dataset_identifier)). \
      must_next()

    # Step 4
    #   Device: ED 1
    #   Description (SRP-2.8): Harness instructs the device to send SRP Update: $ORIGIN default.service.arpa.
    #     service-test-1._thread-test._udp ( SRV 55555 host-test-1 ) host-test-1 AAAA <OMR address of ED_1> with the
    #     following options: Update Lease Option, Lease: 10 minutes, Key Lease: 20 minutes
    #   Pass Criteria:
    #     - N/A
    print("Step 4: ED 1 sends SRP Update for service 1")
    pkts. \
      filter_ipv6_dst(BR_1_MLEID). \
      filter_ipv6_src(ED_1_MLEID). \
      filter(lambda p: p.udp.dstport == br1_srp_port). \
      filter(lambda p: p.dns.flags.opcode == consts.DNS_OPCODE_UPDATE). \
      filter(lambda p: SRP_INSTANCE_NAME_1 in verify_utils.as_list(p.dns.resp.name)). \
      filter(lambda p: SRP_SERVICE_PORT_1 in verify_utils.as_list(p.dns.srv.port)). \
      filter(lambda p: SRP_HOST_NAME in verify_utils.as_list(p.dns.srv.target)). \
      filter(lambda p: p.dns.opt.data is not None and
             any(data.format_compact() == SRP_LEASE_10M_20M.format_compact() for data in p.dns.opt.data)). \
      must_next()

    # Step 5
    #   Device: BR 1 (DUT)
    #   Description (SRP-2.8): Automatically sends SRP Update Response.
    #   Pass Criteria:
    #     - [T7] The DUT MUST send a valid SRP Update Response, with RCODE=0 (NoError).
    print("Step 5: BR 1 (DUT) sends SRP Update Response")
    pkts. \
      filter_ipv6_dst(ED_1_MLEID). \
      filter_ipv6_src(BR_1_MLEID). \
      filter(lambda p: p.udp.srcport == br1_srp_port). \
      filter(lambda p: p.dns.flags.response == 1). \
      filter(lambda p: p.dns.flags.rcode == consts.DNS_RCODE_NOERROR). \
      must_next()

    # Step 6
    #   Device: ED 1
    #   Description (SRP-2.8): Harness instructs the device to unicast DNS query QType SRV for
    #     "service-test-1._thread-test. udp.default.service.arpa." to the DUT.
    #   Pass Criteria:
    #     - N/A
    print("Step 6: ED 1 unicast DNS query SRV for service 1")
    pkts. \
      filter_ipv6_dst(BR_1_MLEID). \
      filter_ipv6_src(ED_1_MLEID). \
      filter(lambda p: p.udp.dstport == 53). \
      filter(lambda p: p.dns.flags.response == 0). \
      filter(lambda p: SRP_INSTANCE_NAME_1 in verify_utils.as_list(p.dns.qry.name)). \
      must_next()

    # Step 7
    #   Device: BR_1 (DUT)
    #   Description (SRP-2.8): Automatically sends DNS response.
    #   Pass Criteria:
    #     - [T8] The DUT MUST send a valid DNS Response, with RCODE=0 (NoError) and
    #     - [T9] contains answer record: $ORIGIN default.service.arpa. service-test-1. thread-test._udp ( SRV 55555
    #       host-test-1 )
    #     - [T10] The DNS Response MAY contain in Additional records section: SORIGIN default.service.arpa. host-test-1
    #       AAAA <OMR address of ED 1>
    print("Step 7: BR 1 (DUT) sends DNS response")
    pkts. \
      filter_ipv6_dst(ED_1_MLEID). \
      filter_ipv6_src(BR_1_MLEID). \
      filter(lambda p: p.udp.srcport == 53). \
      filter(lambda p: p.dns.flags.response == 1). \
      filter(lambda p: p.dns.flags.rcode == consts.DNS_RCODE_NOERROR). \
      filter(lambda p: SRP_INSTANCE_NAME_1 in verify_utils.as_list(p.dns.resp.name)). \
      filter(lambda p: SRP_SERVICE_PORT_1 in verify_utils.as_list(p.dns.srv.port)). \
      must_next()

    # Step 8
    #   Device: Eth_1
    #   Description (SRP-2.8): Harness instructs the device to send mDNS query QType SRV for "service-test-1.
    #     thread-test. udp.local.".
    #   Pass Criteria:
    #     - N/A
    print("Step 8: Eth 1 sends mDNS query SRV for service 1")
    pkts. \
      filter_mDNS_query(MDNS_INSTANCE_NAME_1). \
      filter(lambda p: p.dns.qry.type == consts.DNS_TYPE_SRV). \
      must_next()

    # Step 9
    #   Device: BR_1 (DUT)
    #   Description (SRP-2.8): Automatically sends mDNS Response.
    #   Pass Criteria:
    #     - [T11] The DUT MUST send a valid mDNS Response, with RCODE=0 (NoError) and
    #     - [T12] contains answer record: SORIGIN local. service-test-1. thread-test._udp ( SRV 55555 host-test-1 )
    #     - [T13] DNS Response MAY contain in Additional records section: SORIGIN local. Host-test-1 AAAA <OMR address of
    #       ED 1>
    print("Step 9: BR 1 (DUT) sends mDNS Response")
    pkts. \
      filter_mDNS_response(). \
      filter(lambda p: MDNS_INSTANCE_NAME_1 in verify_utils.as_list(p.dns.resp.name)). \
      filter(lambda p: SRP_SERVICE_PORT_1 in verify_utils.as_list(p.dns.srv.port)). \
      must_next()

    # Step 10
    #   Device: ED_1
    #   Description (SRP-2.8): Harness instructs the device to send SRP Update, adding one more service: $ORIGIN
    #     default.service.arpa. service-test-2._thread-test._udp ( SRV 8 55556 host-test-1 ) host-test-1 AAAA <OMR address
    #     of ED_1> with the following options: Update Lease Option, Lease: 10 minutes, Key Lease: 20 minutes
    #   Pass Criteria:
    #     - N/A
    print("Step 10: ED 1 sends SRP Update adding service 2")
    pkts. \
      filter_ipv6_dst(BR_1_MLEID). \
      filter_ipv6_src(ED_1_MLEID). \
      filter(lambda p: p.udp.dstport == br1_srp_port). \
      filter(lambda p: p.dns.flags.opcode == consts.DNS_OPCODE_UPDATE). \
      filter(lambda p: SRP_INSTANCE_NAME_2 in verify_utils.as_list(p.dns.resp.name)). \
      filter(lambda p: SRP_SERVICE_PORT_2 in verify_utils.as_list(p.dns.srv.port)). \
      filter(lambda p: SRP_SERVICE_PRIORITY_2 in verify_utils.as_list(p.dns.srv.priority)). \
      filter(lambda p: p.dns.opt.data is not None and
             any(data.format_compact() == SRP_LEASE_10M_20M.format_compact() for data in p.dns.opt.data)). \
      must_next()

    # Step 11
    #   Device: BR 1 (DUT)
    #   Description (SRP-2.8): Automatically sends SRP Update Response.
    #   Pass Criteria:
    #     - [T14] The DUT MUST send a valid SRP Update Response, with RCODE=0 (NoError).
    print("Step 11: BR 1 (DUT) sends SRP Update Response")
    pkts. \
      filter_ipv6_dst(ED_1_MLEID). \
      filter_ipv6_src(BR_1_MLEID). \
      filter(lambda p: p.udp.srcport == br1_srp_port). \
      filter(lambda p: p.dns.flags.response == 1). \
      filter(lambda p: p.dns.flags.rcode == consts.DNS_RCODE_NOERROR). \
      must_next()

    # Step 12
    #   Device: ED_1
    #   Description (SRP-2.8): Harness instructs the device to unicast DNS query QType=SRV for service-test-2 to the DUT
    #   Pass Criteria:
    #     - N/A
    print("Step 12: ED 1 unicast DNS query SRV for service 2")
    pkts. \
      filter_ipv6_dst(BR_1_MLEID). \
      filter_ipv6_src(ED_1_MLEID). \
      filter(lambda p: p.udp.dstport == 53). \
      filter(lambda p: p.dns.flags.response == 0). \
      filter(lambda p: SRP_INSTANCE_NAME_2 in verify_utils.as_list(p.dns.qry.name)). \
      must_next()

    # Step 13
    #   Device: BR 1 (DUT)
    #   Description (SRP-2.8): Automatically sends DNS response.
    #   Pass Criteria:
    #     - [T15] The DUT MUST send a valid DNS Response.
    #     - [T16] containing service-test-2 answer record: $ORIGIN default.service.arpa. service-test-2._thread-test._udp
    #       ( SRV 55556 host-test-1 )
    #     - [T17] DNS Response MAY contain in Additional records section: $ORIGIN default.service.arpa. host-test-1 AAAA
    #       <OMR address of ED_1>
    print("Step 13: BR 1 (DUT) sends DNS response")
    pkts. \
      filter_ipv6_dst(ED_1_MLEID). \
      filter_ipv6_src(BR_1_MLEID). \
      filter(lambda p: p.udp.srcport == 53). \
      filter(lambda p: p.dns.flags.response == 1). \
      filter(lambda p: p.dns.flags.rcode == consts.DNS_RCODE_NOERROR). \
      filter(lambda p: SRP_INSTANCE_NAME_2 in verify_utils.as_list(p.dns.resp.name)). \
      filter(lambda p: SRP_SERVICE_PORT_2 in verify_utils.as_list(p.dns.srv.port)). \
      must_next()

    # Step 14
    #   Device: Eth_1
    #   Description (SRP-2.8): Repeat Step 8. Note: this is repeated to verify that the first service is still there.
    #   Pass Criteria:
    #     - Repeat Step 8
    print("Step 14: Eth 1 sends mDNS query SRV for service 1 (repeat)")
    pkts. \
      filter_mDNS_query(MDNS_INSTANCE_NAME_1). \
      filter(lambda p: p.dns.qry.type == consts.DNS_TYPE_SRV). \
      must_next()

    # Step 15
    #   Device: BR 1 (DUT)
    #   Description (SRP-2.8): Repeat Step 9
    #   Pass Criteria:
    #     - [T18-T20] Repeat Step 9
    print("Step 15: BR 1 (DUT) sends mDNS Response (repeat)")
    pkts. \
      filter_mDNS_response(). \
      filter(lambda p: MDNS_INSTANCE_NAME_1 in verify_utils.as_list(p.dns.resp.name)). \
      filter(lambda p: SRP_SERVICE_PORT_1 in verify_utils.as_list(p.dns.srv.port)). \
      must_next()

    # Step 16
    #   Device: ED 1
    #   Description (SRP-2.8): Harness instructs device to send SRP Update, removing the first service only: 1. Service
    #     Discovery Instruction Delete an RR from an RRset semantics using data: $ORIGIN default.service.arpa. PTR
    #     service-test-1. thread-test._udp 2. Service Description Instruction Delete All RRsets From a Name semantics:
    #     $ORIGIN default.service.arpa. service-test-1._thread-test._udp ANY <tt1=0> with the following options: Update
    #     Lease Option, Lease: 10 minutes (unchanged), Key Lease: 10 minutes (shorter). Note: this method to remove a
    #     service follows 3.2.5.5.2 of [draft-srp-23]. Note: for OT reference device, the command 'srp client
    #     keyleaseinterval 600' is used to shorten the key lease interval as indicated.
    #   Pass Criteria:
    #     - N/A
    print("Step 16: ED 1 sends SRP Update removing service 1")
    pkts. \
      filter_ipv6_dst(BR_1_MLEID). \
      filter_ipv6_src(ED_1_MLEID). \
      filter(lambda p: p.udp.dstport == br1_srp_port). \
      filter(lambda p: p.dns.flags.opcode == consts.DNS_OPCODE_UPDATE). \
      filter(lambda p: SRP_SERVICE_NAME in verify_utils.as_list(p.dns.resp.name)). \
      filter(lambda p: SRP_INSTANCE_NAME_1 in verify_utils.as_list(p.dns.ptr.domain_name)). \
      filter(lambda p: SRP_INSTANCE_NAME_1 in verify_utils.as_list(p.dns.resp.name)). \
      filter(lambda p: any(ttl == 0 for ttl in verify_utils.as_list(p.dns.resp.ttl))). \
      filter(lambda p: p.dns.opt.data is not None and
             any(data.format_compact() == SRP_LEASE_10M_10M.format_compact() for data in p.dns.opt.data)). \
      must_next()

    # Step 17
    #   Device: BR 1 (DUT)
    #   Description (SRP-2.8): Automatically sends SRP Update Response.
    #   Pass Criteria:
    #     - [T21] The DUT MUST send a valid SRP Update Response, with RCODE=0 (NoError).
    print("Step 17: BR 1 (DUT) sends SRP Update Response")
    pkts. \
      filter_ipv6_dst(ED_1_MLEID). \
      filter_ipv6_src(BR_1_MLEID). \
      filter(lambda p: p.udp.srcport == br1_srp_port). \
      filter(lambda p: p.dns.flags.response == 1). \
      filter(lambda p: p.dns.flags.rcode == consts.DNS_RCODE_NOERROR). \
      must_next()

    # Step 18
    #   Device: ED 1
    #   Description (SRP-2.8): Harness instructs the device to unicast DNS query QType SRV for "service-test-1.
    #     thread-test. udp.default.service.arpa." to the DUT
    #   Pass Criteria:
    #     - N/A
    print("Step 18: ED 1 unicast DNS query SRV for service 1")
    pkts. \
      filter_ipv6_dst(BR_1_MLEID). \
      filter_ipv6_src(ED_1_MLEID). \
      filter(lambda p: p.udp.dstport == 53). \
      filter(lambda p: p.dns.flags.response == 0). \
      filter(lambda p: SRP_INSTANCE_NAME_1 in verify_utils.as_list(p.dns.qry.name)). \
      must_next()

    # Step 19
    #   Device: BR 1 (DUT)
    #   Description (SRP-2.8): Automatically sends DNS response. Note: the service instance name with all associated
    #     records has been deleted in step 16.
    #   Pass Criteria:
    #     - [T22] The DUT MUST send a valid DNS Response, with RCODE=0 (NoError); and
    #     - [T23] MUST contain 0 answer records.
    print("Step 19: BR 1 (DUT) sends DNS response (0 answers)")
    pkts. \
      filter_ipv6_dst(ED_1_MLEID). \
      filter_ipv6_src(BR_1_MLEID). \
      filter(lambda p: p.udp.srcport == 53). \
      filter(lambda p: p.dns.flags.response == 1). \
      filter(lambda p: p.dns.flags.rcode in (consts.DNS_RCODE_NOERROR, consts.DNS_RCODE_NXDOMAIN)). \
      filter(lambda p: p.dns.count.answers == 0). \
      must_next()

    # Step 20
    #   Device: ED 1
    #   Description (SRP-2.8): Harness instructs the device to unicast DNS query QType SRV for "service-test-2.
    #     thread-test. udp.default.service.arpa." to the DUT.
    #   Pass Criteria:
    #     - N/A
    print("Step 20: ED 1 unicast DNS query SRV for service 2")
    pkts. \
      filter_ipv6_dst(BR_1_MLEID). \
      filter_ipv6_src(ED_1_MLEID). \
      filter(lambda p: p.udp.dstport == 53). \
      filter(lambda p: p.dns.flags.response == 0). \
      filter(lambda p: SRP_INSTANCE_NAME_2 in verify_utils.as_list(p.dns.qry.name)). \
      must_next()

    # Step 21
    #   Device: BR_1 (DUT)
    #   Description (SRP-2.8): Automatically sends DNS response.
    #   Pass Criteria:
    #     - [T24] The DUT MUST send a valid DNS Response, with RCODE=0 (NoError) and
    #     - [T25] contains answer record: $ORIGIN default.service.arpa. service-test-2. thread-test._udp ( SRV 55556
    #       host-test-1 )
    #     - [T26] DNS Response MAY contain in Additional records section: $ORIGIN default.service.arpa. host-test-1 AAAA
    #       <OMR address of ED 1>
    print("Step 21: BR 1 (DUT) sends DNS response (service 2)")
    pkts. \
      filter_ipv6_dst(ED_1_MLEID). \
      filter_ipv6_src(BR_1_MLEID). \
      filter(lambda p: p.udp.srcport == 53). \
      filter(lambda p: p.dns.flags.response == 1). \
      filter(lambda p: p.dns.flags.rcode == consts.DNS_RCODE_NOERROR). \
      filter(lambda p: SRP_INSTANCE_NAME_2 in verify_utils.as_list(p.dns.resp.name)). \
      filter(lambda p: SRP_SERVICE_PORT_2 in verify_utils.as_list(p.dns.srv.port)). \
      must_next()

    # Step 22
    #   Device: Eth_1
    #   Description (SRP-2.8): Harness instructs the device to send mDNS query QType SRV for "service-test-1.
    #     thread-test. udp.local".
    #   Pass Criteria:
    #     - N/A
    print("Step 22: Eth 1 sends mDNS query SRV for service 1")
    pkts. \
      filter_mDNS_query(MDNS_INSTANCE_NAME_1). \
      filter(lambda p: p.dns.qry.type == consts.DNS_TYPE_SRV). \
      must_next()

    # Step 23
    #   Device: BR_1 (DUT)
    #   Description (SRP-2.8): Does not send a mDNS response.
    #   Pass Criteria:
    #     - [T27] The DUT MUST NOT send an mDNS response.
    print("Step 23: BR 1 (DUT) does NOT send mDNS response")
    pkts. \
      filter_mDNS_response(). \
      filter(lambda p: MDNS_INSTANCE_NAME_1 in verify_utils.as_list(p.dns.resp.name)). \
      must_not_next()

    # Step 24
    #   Device: Eth 1
    #   Description (SRP-2.8): Harness instructs the device to send mDNS query QType=PTR any services of type
    #     "_thread-test._udp.local".
    #   Pass Criteria:
    #     - N/A
    print("Step 24: Eth 1 sends mDNS query PTR for services")
    pkts. \
      filter_mDNS_query(MDNS_SERVICE_NAME). \
      filter(lambda p: p.dns.qry.type == consts.DNS_TYPE_PTR). \
      must_next()

    # Step 25
    #   Device: BR_1 (DUT)
    #   Description (SRP-2.8): Automatically sends mDNS response with service-test-2.
    #   Pass Criteria:
    #     - [T28] The DUT MUST send a valid mDNS Response, with RCODE=0 (NoError) and
    #     - [T29] contains service-test-2 answer record: $ORIGIN local. _thread-test._udp ( PTR
    #       service-test-2._thread-test._udp )
    #     - [T30] Response MUST NOT contain any further records in the Answers section.
    #     - [T31-32] DNS Response MAY contain in Additional records section: SORIGIN local.
    #       Service-test-2._thread-test._udp ( SRV 8 0 55556 host-test-1 ) host-test-1 AAAA <OMR address of ED_1>
    print("Step 25: BR 1 (DUT) sends mDNS response with service 2 only")
    pkts. \
      filter_mDNS_response(). \
      filter(lambda p: MDNS_SERVICE_NAME in verify_utils.as_list(p.dns.resp.name)). \
      filter(lambda p: MDNS_INSTANCE_NAME_2 in verify_utils.as_list(p.dns.ptr.domain_name)). \
      filter(lambda p: p.dns.count.answers == 1). \
      must_next()
