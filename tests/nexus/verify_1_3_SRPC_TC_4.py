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
    # 3.4. [1.3] [CERT] [COMPONENT] Remove one service only by SRP client
    #
    # 3.4.1. Purpose
    # To test the following:
    # - SRP client needs to remove one service while other service remains registered.
    # - Correct usage of SRP Update to remove 1 service.
    # - Note: a Thread Product DUT must skip this test. Thread Product DUTs do not have a test case equivalent to this
    #   one, since they would then need to be instructed to register 2 services and then remove 1 service again. This
    #   seems unlikely to be practical.
    #
    # 3.4.2. Topology
    # - BR_1 - Test Bed border router device operating as a Thread Border Router, and the Leader
    # - Eth_1 - Test Bed border router device on an Adjacent Infrastructure Link
    # - TD_1 (DUT) - Thread End Device or Thread Router Component DUT
    #
    # Spec Reference   | V1.1 Section | V1.3.0 Section
    # -----------------|--------------|---------------
    # SRP Client       | N/A          | 2.3.2

    pkts = pv.pkts
    pv.summary.show()

    BR_1_MLEID = Ipv6Addr(pv.vars['BR_1_MLEID_ADDR'])
    TD_1_MLEID = Ipv6Addr(pv.vars['TD_1_MLEID_ADDR'])
    TD_1_OMR_ADDR = Ipv6Addr(pv.vars['TD_1_OMR_ADDR'])
    BR_1_SRP_PORT = int(pv.vars['BR_1_SRP_PORT'])

    SRP_SERVICE_NAME = '_thread-test._udp.default.service.arpa'
    SRP_INSTANCE_NAME1 = 'service-test-1.' + SRP_SERVICE_NAME
    SRP_INSTANCE_NAME2 = 'service-test-2.' + SRP_SERVICE_NAME
    SRP_HOST_NAME1 = 'host-test-1.default.service.arpa'

    SRP_SERVICE_PORT1 = 55555
    SRP_SERVICE_PORT2 = 55556

    SRP_LEASE_5M_10M = Bytes(struct.pack('>II', 300, 600).hex())
    SRP_LEASE_3M_10M = Bytes(struct.pack('>II', 180, 600).hex())

    # Step 1
    # - Device: BR_1
    # - Description (SRPC-3.4): Enable
    # - Pass Criteria:
    #   - N/A
    print("Step 1: BR_1 enable.")

    # Step 2
    # - Device: TD_1 (DUT)
    # - Description (SRPC-3.4): Enable
    # - Pass Criteria:
    #   - N/A
    print("Step 2: TD_1 enable.")

    # Step 3
    # - Device: BR 1
    # - Description (SRPC-3.4): Automatically adds its SRP Server information in the DNSSRP/ Unicast Dataset in the
    #   Thread Network Data. Automatically configures an OMR prefix in Thread Network Data.
    # - Pass Criteria:
    #   - N/A
    print("Step 3: BR_1 adds SRP Server information and OMR prefix.")

    # Step 4
    # - Device: TD 1 (DUT)
    # - Description (SRPC-3.4): Harness instructs device to register a new service: $ORIGIN default.service.arpa.
    #   service-test-1. thread-test._udp ( SRV 1 1 55555 host-test-1 ) host-test-1 AAAA <OMR address of TD_1> with the
    #   following options: Update Lease Option Lease: 5 minutes Key Lease: 10 minutes.
    # - Pass Criteria:
    #   - N/A (see step 5 instead)
    print("Step 4: TD_1 registers service 1.")
    pkts.filter_ipv6_dst(BR_1_MLEID).\
        filter_ipv6_src(TD_1_MLEID).\
        filter(lambda p: p.udp.dstport == BR_1_SRP_PORT).\
        filter(lambda p: p.dns.flags.opcode == 5).\
        filter(lambda p: SRP_INSTANCE_NAME1 in verify_utils.as_list(p.dns.resp.name)).\
        filter(lambda p: SRP_SERVICE_PORT1 in verify_utils.as_list(p.dns.srv.port)).\
        filter(lambda p: 1 in verify_utils.as_list(p.dns.srv.priority)).\
        filter(lambda p: 1 in verify_utils.as_list(p.dns.srv.weight)).\
        filter(lambda p: TD_1_OMR_ADDR in verify_utils.as_list(p.dns.aaaa)).\
        filter(lambda p: p.dns.opt.data is not None).\
        filter(lambda p: any(data.format_compact() == SRP_LEASE_5M_10M.format_compact() for data in p.dns.opt.data)).\
        must_next()

    # Step 5
    # - Device: BR 1
    # - Description (SRPC-3.4): Automatically responds 'SUCCESS'.
    # - Pass Criteria:
    #   - MUST respond 'SUCCESS'.
    #   - Note: this verification step is done on the BR 1 response to ensure that the DUT in step 4 sent a correct SRP
    #     Update. This is a quick way to verify the Update was correct.
    print("Step 5: BR_1 responds SUCCESS.")
    pkts.filter_ipv6_src(BR_1_MLEID).\
        filter_ipv6_dst(TD_1_MLEID).\
        filter(lambda p: p.udp.srcport == BR_1_SRP_PORT).\
        filter(lambda p: p.dns.flags.response == 1).\
        filter(lambda p: p.dns.flags.rcode == 0).\
        must_next()

    # Step 6
    # - Device: TD 1 (DUT)
    # - Description (SRPC-3.4): Harness instructs device to register a new, second service: $ORIGIN
    #   default.service.arpa. service-test-2._thread-test._udp ( SRV 1 1 55556 host-test-1 ) host-test-1 AAAA <OMR
    #   address of TD 1> with the following options: Update Lease Option Lease: 3 minutes Key Lease: 10 minutes
    # - Pass Criteria:
    #   - N/A (see step 7 instead)
    print("Step 6: TD_1 registers service 2.")
    pkts.filter_ipv6_dst(BR_1_MLEID).\
        filter_ipv6_src(TD_1_MLEID).\
        filter(lambda p: p.udp.dstport == BR_1_SRP_PORT).\
        filter(lambda p: p.dns.flags.opcode == 5).\
        filter(lambda p: SRP_INSTANCE_NAME2 in verify_utils.as_list(p.dns.resp.name)).\
        filter(lambda p: SRP_SERVICE_PORT2 in verify_utils.as_list(p.dns.srv.port)).\
        filter(lambda p: 1 in verify_utils.as_list(p.dns.srv.priority)).\
        filter(lambda p: 1 in verify_utils.as_list(p.dns.srv.weight)).\
        filter(lambda p: TD_1_OMR_ADDR in verify_utils.as_list(p.dns.aaaa)).\
        filter(lambda p: p.dns.opt.data is not None).\
        filter(lambda p: any(data.format_compact() == SRP_LEASE_3M_10M.format_compact() for data in p.dns.opt.data)).\
        must_next()

    # Step 7
    # - Device: BR 1
    # - Description (SRPC-3.4): Automatically responds SUCCESS.
    # - Pass Criteria:
    #   - MUST respond 'SUCCESS'.
    #   - Note: this verification step is done on the BR_1 response to ensure that the DUT in step 6 sent a correct SRP
    #     Update. This is a quick way to verify the Update was correct.
    print("Step 7: BR_1 responds SUCCESS.")
    pkts.filter_ipv6_src(BR_1_MLEID).\
        filter_ipv6_dst(TD_1_MLEID).\
        filter(lambda p: p.udp.srcport == BR_1_SRP_PORT).\
        filter(lambda p: p.dns.flags.response == 1).\
        filter(lambda p: p.dns.flags.rcode == 0).\
        must_next()

    # Step 8
    # - Device: TD 1 (DUT)
    # - Description (SRPC-3.4): Harness instructs device to remove its first service.
    # - Pass Criteria:
    #   - The DUT MUST send an SRP Update message such that the Service Discovery Instruction contains a single Delete
    #     an RR from an RRset ([RFC 2136], Section 2.5.4) update that deletes named records: $ORIGIN
    #     default.service.arpa. service-test-1._thread-test. udp ANY <ttl=0>
    #   - Note: if this is difficult to verify here then instead the SRP server response in step 11 can be used as
    #     validation that the correct service was removed.
    print("Step 8: TD_1 removes service 1.")
    pkts.filter_ipv6_dst(BR_1_MLEID).\
        filter_ipv6_src(TD_1_MLEID).\
        filter(lambda p: p.udp.dstport == BR_1_SRP_PORT).\
        filter(lambda p: p.dns.flags.opcode == 5).\
        filter(lambda p: SRP_INSTANCE_NAME1 in verify_utils.as_list(p.dns.resp.name)).\
        filter(lambda p: any(ttl == 0 for ttl in verify_utils.as_list(p.dns.resp.ttl))).\
        must_next()

    # Step 9
    # - Device: BR_1
    # - Description (SRPC-3.4): Automatically responds 'SUCCESS'.
    # - Pass Criteria:
    #   - MUST respond 'SUCCESS'.
    print("Step 9: BR_1 responds SUCCESS.")
    pkts.filter_ipv6_src(BR_1_MLEID).\
        filter_ipv6_dst(TD_1_MLEID).\
        filter(lambda p: p.udp.srcport == BR_1_SRP_PORT).\
        filter(lambda p: p.dns.flags.response == 1).\
        filter(lambda p: p.dns.flags.rcode == 0).\
        must_next()

    # Step 10
    # - Device: TD 1 (DUT)
    # - Description (SRPC-3.4): Harness instructs device via TH API to send unicast DNS query QType PTR to BR_1 for any
    #   services of type_thread-test._udp.default.service.arpa.". Note: this is done to validate that the SRP server
    #   removed the first service and kept the second service.
    # - Pass Criteria:
    #   - N/A
    print("Step 10: TD_1 sends DNS query PTR.")
    pkts.filter_ipv6_dst(BR_1_MLEID).\
        filter_ipv6_src(TD_1_MLEID).\
        filter(lambda p: p.udp.dstport == 53).\
        filter(lambda p: p.dns.flags.response == 0).\
        filter(lambda p: SRP_SERVICE_NAME in verify_utils.as_list(p.dns.qry.name)).\
        must_next()

    # Step 11
    # - Device: BR_1
    # - Description (SRPC-3.4): Responds with the second service only, not the first service.
    # - Pass Criteria:
    #   - MUST respond with the following service DNS-SD response to requesting client in the Answer records section:
    #     $ORIGIN default.service.arpa. _thread-test._udp ( PTR service-test-2._thread-test._udp )
    #   - DUT MUST NOT respond with further records in the Answer records section.
    #   - Response MAY contain in the Additional records section: $ORIGIN default.service.arpa.
    #     service-test-2._thread-test._udp ( SRV 1 1 55556 host-test-1 ) host-test-1 AAAA <OMR address of TD_1>
    print("Step 11: BR_1 responds with service 2 only.")
    pkts.filter_ipv6_src(BR_1_MLEID).\
        filter_ipv6_dst(TD_1_MLEID).\
        filter(lambda p: p.udp.srcport == 53).\
        filter(lambda p: p.dns.flags.response == 1).\
        filter(lambda p: p.dns.flags.rcode == 0).\
        filter(lambda p: SRP_SERVICE_NAME in verify_utils.as_list(p.dns.resp.name)).\
        filter(lambda p: SRP_INSTANCE_NAME2 in verify_utils.as_list(p.dns.ptr.domain_name)).\
        filter(lambda p: int(p.dns.count.answers) == 1).\
        must_next()


if __name__ == '__main__':
    verify_utils.run_main(verify)
