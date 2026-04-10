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
from pktverify.addrs import Ipv6Addr, EthAddr
from pktverify.bytes import Bytes


def verify(pv):
    # 2.4. [1.3] [CERT] Key Lease
    #
    # 2.4.1. Purpose
    # To test the following:
    # - 1. Key lease handling on SRP server on DUT.
    # - 2. Verify that a service instance name cannot be claimed even after the service instance lease expired, if the
    #   key lease is still active.
    #
    # 2.4.2. Topology
    # - BR 1 (DUT) - Thread Border Router and the Leader
    # - ED 1-Test Bed device operating as a Thread End Device, attached to BR_1
    # - ED 2-Test Bed device operating as a Thread End Device, attached to BR_1
    # - Eth 1-Test Bed border router device on an Adjacent Infrastructure Link
    #
    # Spec Reference   | V1.1 Section | V1.3.0 Section
    # -----------------|--------------|---------------
    # SRP Server       | N/A          | 1.3

    pkts = pv.pkts
    pv.summary.show()

    BR_1_MLEID = Ipv6Addr(pv.vars['BR_1_MLEID_ADDR'])
    ED_1_MLEID = Ipv6Addr(pv.vars['ED_1_MLEID_ADDR'])
    ED_2_MLEID = Ipv6Addr(pv.vars['ED_2_MLEID_ADDR'])
    BR_1_ETH = EthAddr(pv.vars['BR_1_ETH'])
    ETH_1_ETH = EthAddr(pv.vars['Eth_1_ETH'])
    BR_1_SRP_PORT = int(pv.vars['BR_1_SRP_PORT'])

    ED_1_OMR_ADDR = Ipv6Addr(pv.vars['ED_1_OMR_ADDR'])
    ED_2_OMR_ADDR = Ipv6Addr(pv.vars['ED_2_OMR_ADDR'])

    SRP_SERVICE_NAME = '_thread-test._udp.default.service.arpa'
    SRP_INSTANCE_NAME1 = 'service-test-1.' + SRP_SERVICE_NAME
    SRP_INSTANCE_NAME3 = 'service-test-3.' + SRP_SERVICE_NAME
    SRP_HOST_NAME1 = 'host-test-1.default.service.arpa'
    SRP_HOST_NAME2 = 'host-test-2.default.service.arpa'

    MDNS_SERVICE_NAME = '_thread-test._udp.local'
    MDNS_INSTANCE_NAME1 = 'service-test-1.' + MDNS_SERVICE_NAME
    MDNS_HOST_NAME1 = 'host-test-1.local'

    SRP_SERVICE_PORT = 55555

    SRP_LEASE_30S_60S = Bytes(struct.pack('>II', 30, 60).hex())
    SRP_LEASE_30S_30S = Bytes(struct.pack('>II', 30, 30).hex())
    SRP_LEASE_10S_60S = Bytes(struct.pack('>II', 10, 60).hex())

    def verify_srp_update(pkts, dst, src, srp_port, instance_name, host_name, omr_addr, lease_data):
        return pkts.\
            filter_ipv6_dst(dst).\
            filter_ipv6_src(src).\
            filter(lambda p: p.udp.dstport == srp_port).\
            filter(lambda p: p.dns.flags.opcode == 5).\
            filter(lambda p: instance_name in verify_utils.as_list(p.dns.resp.name)).\
            filter(lambda p: host_name is None or host_name in verify_utils.as_list(p.dns.srv.target)).\
            filter(lambda p: omr_addr in verify_utils.as_list(p.dns.aaaa)).\
            filter(lambda p: p.dns.opt.data is not None and
                   any(data.format_compact() == lease_data.format_compact() for data in p.dns.opt.data)).\
            must_next()

    # Step 1
    # - Device: BR_1 (DUT)
    # - Description (SRP-2.4): Enable: switch on.
    # - Pass Criteria:
    #   - N/A
    print("Step 1: BR_1 (DUT) enable.")

    # Step 2
    # - Device: Eth 1, ED 1, ED 2
    # - Description (SRP-2.4): Enable
    # - Pass Criteria:
    #   - N/A
    print("Step 2: Eth 1, ED 1, ED 2 enable.")

    # Step 3
    # - Device: BR_1 (DUT)
    # - Description (SRP-2.4): Automatically adds its SRP Server information in the Thread Network Data.
    # - Pass Criteria:
    #   - The DUT's SRP Server information MUST appear in the Thread Network Data: it MUST contain a DNS/SRP Unicast
    #     Dataset, or a DNS/SRP Anycast Dataset, or both.
    print("Step 3: BR_1 (DUT) adds its SRP Server information in the Thread Network Data.")
    pkts.\
        filter_mle_cmd(consts.MLE_DATA_RESPONSE).\
        filter(lambda p: 93 in verify_utils.as_list(p.thread_nwd.tlv.service.srp_dataset_identifier)).\
        filter(lambda p: any(l in (1, 19) for l in verify_utils.as_list(p.thread_nwd.tlv.service.s_data_len))).\
        must_next()

    # Step 4
    # - Device: ED 1
    # - Description (SRP-2.4): Harness instructs the device to send SRP Update: $ORIGIN default.service.arpa.
    #   service-test-1._thread-test._udp ( SRV 1 1 55555 host-test-1 ) host-test-1 AAAA <OMR address of ED_1> with the
    #   following options: Update Lease Option Lease: 30 seconds Key Lease: 60 seconds
    # - Pass Criteria:
    #   - N/A
    print("Step 4: ED 1 sends SRP Update.")
    verify_srp_update(pkts, BR_1_MLEID, ED_1_MLEID, BR_1_SRP_PORT, SRP_INSTANCE_NAME1, SRP_HOST_NAME1, ED_1_OMR_ADDR,
                      SRP_LEASE_30S_60S)

    # Step 5
    # - Device: BR 1 (DUT)
    # - Description (SRP-2.4): Automatically sends SRP Response.
    # - Pass Criteria:
    #   - The DUT MUST send a valid SRP Response with RCODE=0 (NoError).
    print("Step 5: BR 1 (DUT) sends SRP Response.")
    last_response = pkts.\
        filter_ipv6_dst(ED_1_MLEID).\
        filter_ipv6_src(BR_1_MLEID).\
        filter(lambda p: p.dns.flags.response == 1).\
        filter(lambda p: p.dns.flags.rcode == 0).\
        must_next()

    # Step 6
    # - Device: ED_1
    # - Description (SRP-2.4): Harness instructs the device to unicast DNS query QType=SRV for
    #   "service-test-1._thread-test_udp.default.service.arpa".
    # - Pass Criteria:
    #   - N/A
    print("Step 6: ED 1 sends unicast DNS query QType=SRV.")
    pkts.\
        filter_ipv6_dst(BR_1_MLEID).\
        filter_ipv6_src(ED_1_MLEID).\
        filter(lambda p: p.udp.dstport == 53).\
        filter(lambda p: p.dns.flags.response == 0).\
        filter(lambda p: SRP_INSTANCE_NAME1 in verify_utils.as_list(p.dns.qry.name)).\
        must_next()

    # Step 7
    # - Device: BR 1 (DUT)
    # - Description (SRP-2.4): Automatically sends DNS response.
    # - Pass Criteria:
    #   - The DUT MUST send a valid DNS Response containing answer record: $ORIGIN default.service.arpa.
    #     service-test-1._thread-test._udp ( SRV 1 1 55555 host-test-1 )
    #   - The DNS Response MAY contain in Additional records section: $ORIGIN default.service.arpa. host-test-1 AAAA
    #     <OMR address of ED_1>
    print("Step 7: BR 1 (DUT) sends DNS response.")
    pkts.\
        filter_ipv6_dst(ED_1_MLEID).\
        filter_ipv6_src(BR_1_MLEID).\
        filter(lambda p: p.udp.srcport == 53).\
        filter(lambda p: p.dns.flags.response == 1).\
        filter(lambda p: p.dns.flags.rcode == 0).\
        filter(lambda p: SRP_SERVICE_PORT in verify_utils.as_list(p.dns.srv.port)).\
        filter(lambda p: SRP_HOST_NAME1 in verify_utils.as_list(p.dns.srv.target)).\
        must_next()

    # Step 8
    # - Device: ED 1
    # - Description (SRP-2.4): Harness instructs the device to send unicast DNS query QType=PTR for any services of type
    #   _thread-test_udp.default.service.arpa.
    # - Pass Criteria:
    #   - N/A
    print("Step 8: ED 1 sends unicast DNS query QType=PTR.")
    pkts.\
        filter_ipv6_dst(BR_1_MLEID).\
        filter_ipv6_src(ED_1_MLEID).\
        filter(lambda p: p.udp.dstport == 53).\
        filter(lambda p: p.dns.flags.response == 0).\
        filter(lambda p: SRP_SERVICE_NAME in verify_utils.as_list(p.dns.qry.name)).\
        must_next()

    # Step 9
    # - Device: BR 1 (DUT)
    # - Description (SRP-2.4): Automatically sends DNS Response.
    # - Pass Criteria:
    #   - The DUT MUST send a valid DNS Response containing answer record: $ORIGIN default.service.arpa.
    #     _thread-test._udp ( PTR service-test-1._thread-test._udp )
    #   - The DNS Response MAY contain in Additional records section: $ORIGIN default.service.arpa.
    #     service-test-1._thread-test._udp ( SRV 1 1 55555 host-test-1 ) host-test-1 AAAA <OMR address of ED_1>
    print("Step 9: BR 1 (DUT) sends DNS Response.")
    last_response = pkts.\
        filter_ipv6_dst(ED_1_MLEID).\
        filter_ipv6_src(BR_1_MLEID).\
        filter(lambda p: p.udp.srcport == 53).\
        filter(lambda p: p.dns.flags.response == 1).\
        filter(lambda p: p.dns.flags.rcode == 0).\
        filter(lambda p: SRP_INSTANCE_NAME1 in verify_utils.as_list(p.dns.ptr.domain_name)).\
        must_next()

    # Step 10
    # - Device: N/A
    # - Description (SRP-2.4): Harness waits for 35 seconds. Note: during this time there must be no automatic
    #   reregistration of the service done by ED 1. This can be implemented on (OpenThread) ED_1 by either disabling
    #   automatic service reregistration or by internally removing the service without doing the deregistration
    #   request.
    # - Pass Criteria:
    #   - N/A
    print("Step 10: Wait 35 seconds.")
    pkts.filter(lambda p: p.sniff_timestamp > last_response.sniff_timestamp + 34).next()

    # Step 11
    # - Device: ED 2
    # - Description (SRP-2.4): Harness instructs the device to send SRP Update: $ORIGIN default.service.arpa.
    #   service-test-1._thread-test._udp ( SRV 0 0 55555 host-test-2 ) host-test-2 AAAA <OMR address of ED_2> with the
    #   following options: Update Lease Option Lease: 30 seconds Key Lease: 30 seconds
    # - Pass Criteria:
    #   - N/A
    print("Step 11: ED 2 sends SRP Update.")
    verify_srp_update(pkts, BR_1_MLEID, ED_2_MLEID, BR_1_SRP_PORT, SRP_INSTANCE_NAME1, SRP_HOST_NAME2, ED_2_OMR_ADDR,
                      SRP_LEASE_30S_30S)

    # Step 12
    # - Device: BR 1. (DUT)
    # - Description (SRP-2.4): Automatically sends SRP Update Response, denying the registration because the key lease
    #   for the expired service 'service-test-1' is still active. Note: spec for this is 3.2.5.2, 3.3.3, 3.3.5 and 5.1
    #   of [draft-srp-23].
    # - Pass Criteria:
    #   - The DUT MUST send a valid SRP Update Response with RCODE=6 (YXDOMAIN).
    print("Step 12: BR 1 (DUT) sends SRP Update Response (YXDOMAIN).")
    last_response = pkts.\
        filter_ipv6_dst(ED_2_MLEID).\
        filter_ipv6_src(BR_1_MLEID).\
        filter(lambda p: p.dns.flags.response == 1).\
        filter(lambda p: p.dns.flags.rcode == 6).\
        must_next()

    # Step 13
    # - Device: N/A
    # - Description (SRP-2.4): Harness waits for 30 seconds. Note: during this time there must be no automatic
    #   reregistration of the service done by ED 2. This can be implemented on (Open Thread) ED_2 by either disabling
    #   automatic service reregistration or by internally removing the service without doing the deregistration
    #   request.
    # - Pass Criteria:
    #   - N/A
    print("Step 13: Wait 30 seconds.")
    pkts.filter(lambda p: p.sniff_timestamp > last_response.sniff_timestamp + 29).next()

    # Step 14
    # - Device: ED 2
    # - Description (SRP-2.4): Harness instructs the device to send SRP Update: $ORIGIN default.service.arpa.
    #   service-test-1._thread-test._udp ( SRV 1 1 55555 host-test-1 ) host-test-1 AAAA <OMR address of ED_2> Update
    #   Lease Option Lease: 10 seconds Key Lease: 60 seconds Note: ED 2 here uses the hostname of ED 1 to verify that
    #   the name host-test-1 has become available again, after key expiry thereaf. Note: the requested Lease time of
    #   10 seconds will be automatically adjusted by the SRP Server to 30 seconds (the minimum allowed).
    # - Pass Criteria:
    #   - N/A
    print("Step 14: ED 2 sends SRP Update.")
    verify_srp_update(pkts, BR_1_MLEID, ED_2_MLEID, BR_1_SRP_PORT, SRP_INSTANCE_NAME1, SRP_HOST_NAME1, ED_2_OMR_ADDR,
                      SRP_LEASE_10S_60S)

    # Step 15
    # - Device: BR 1 (DUT)
    # - Description (SRP-2.4): Automatically sends SRP Update Response.
    # - Pass Criteria:
    #   - The DUT MUST send a valid SRP Response with RCODE = 0 (NoError).
    print("Step 15: BR 1 (DUT) sends SRP Response.")
    pkts.\
        filter_ipv6_dst(ED_2_MLEID).\
        filter_ipv6_src(BR_1_MLEID).\
        filter(lambda p: p.dns.flags.response == 1).\
        filter(lambda p: p.dns.flags.rcode == 0).\
        must_next()

    # Step 16
    # - Device: Eth_1
    # - Description (SRP-2.4): Harness instructs the device to send mDNS query QType PTR for any services of type
    #   "_thread-test._udp.local."
    # - Pass Criteria:
    #   - N/A
    print("Step 16: Eth 1 sends mDNS query QType PTR.")
    pkts.\
        filter(lambda p: p.eth.src == ETH_1_ETH).\
        filter(lambda p: p.udp.dstport == 5353).\
        filter(lambda p: p.mdns.flags.response == 0).\
        filter(lambda p: MDNS_SERVICE_NAME in verify_utils.as_list(p.mdns.qry.name)).\
        must_next()

    # Step 17
    # - Device: BR 1 (DUT)
    # - Description (SRP-2.4): Automatically sends mDNS Response.
    # - Pass Criteria:
    #   - The DUT MUST send a valid mDNS Response containing only answer record: $ORIGIN local. _thread-test._udp (
    #     PTR service-test-1._thread-test._udp )
    #   - The mDNS Response MAY contain in Additional records section: $ORIGIN local.
    #     Service-test-1._thread-test._udp ( SRV 1 1 55555 host-test-1 ) host-test-1 AAAA <OMR address of ED_2>
    print("Step 17: BR 1 (DUT) sends mDNS Response.")
    last_response = pkts.\
        filter(lambda p: p.eth.src == BR_1_ETH).\
        filter(lambda p: p.udp.dstport == 5353).\
        filter(lambda p: p.mdns.flags.response == 1).\
        filter(lambda p: MDNS_INSTANCE_NAME1 in verify_utils.as_list(p.mdns.ptr.domain_name)).\
        filter(lambda p: SRP_SERVICE_PORT in verify_utils.as_list(p.mdns.srv.port)).\
        filter(lambda p: MDNS_HOST_NAME1 in verify_utils.as_list(p.mdns.srv.target)).\
        filter(lambda p: ED_2_OMR_ADDR in verify_utils.as_list(p.mdns.aaaa)).\
        must_next()

    # Step 18
    # - Device: N/A
    # - Description (SRP-2.4): Harness waits for 35 seconds, for the service lease to expire.
    # - Pass Criteria:
    #   - N/A
    print("Step 18: Wait 35 seconds.")
    pkts.filter(lambda p: p.sniff_timestamp > last_response.sniff_timestamp + 34).next()

    # Step 19
    # - Device: Eth_1
    # - Description (SRP-2.4): Harness instructs the device to send mDNS query QType PTR for any services of type
    #   "_thread-test_udp.local."
    # - Pass Criteria:
    #   - N/A
    print("Step 19: Eth 1 sends mDNS query QType PTR.")
    pkts.\
        filter(lambda p: p.eth.src == ETH_1_ETH).\
        filter(lambda p: p.udp.dstport == 5353).\
        filter(lambda p: p.mdns.flags.response == 0).\
        filter(lambda p: MDNS_SERVICE_NAME in verify_utils.as_list(p.mdns.qry.name)).\
        must_next()

    # Step 20
    # - Device: BR 1 (DUT)
    # - Description (SRP-2.4): Does not send mDNS Response, since the service with matching type has expired. Note:
    #   although the key leases on service-test-1 and host-test-1 are still active now, the generic service pointer is
    #   expired and therefore no result to the PTR query is given. There is no record with the generic name stored,
    #   anymore.
    # - Pass Criteria:
    #   - The DUT MUST NOT send an mDNS Response.
    print("Step 20: BR 1 (DUT) should not send mDNS Response.")
    pkts.\
        filter(lambda p: p.eth.src == BR_1_ETH).\
        filter(lambda p: p.udp.dstport == 5353).\
        filter(lambda p: p.mdns.flags.response == 1).\
        filter(lambda p: MDNS_INSTANCE_NAME1 in verify_utils.as_list(p.mdns.ptr.domain_name)).\
        must_not_next()

    # Step 21
    # - Device: ED 1
    # - Description (SRP-2.4): Harness instructs the device to send SRP Update with different service name but
    #   conflicting host name: $ORIGIN default.service.arpa. service-test-3._thread-test._udp ( SRV 0 0 55555
    #   host-test-1 ) host-test-1 AAAA <OMR address of ED_1> With the following options: Update Lease Option Lease:
    #   30 seconds Key Lease: 30 seconds
    # - Pass Criteria:
    #   - N/A
    print("Step 21: ED 1 sends SRP Update with conflicting host name.")
    verify_srp_update(pkts, BR_1_MLEID, ED_1_MLEID, BR_1_SRP_PORT, SRP_INSTANCE_NAME3, SRP_HOST_NAME1, ED_1_OMR_ADDR,
                      SRP_LEASE_30S_30S)

    # Step 22
    # - Device: BR 1 (DUT)
    # - Description (SRP-2.4): Automatically sends SRP Update Response, denying the registration because host
    #   'host-test-1' is already claimed by ED_2 for its key lease time.
    # - Pass Criteria:
    #   - The DUT MUST send a valid SRP Update Response with RCODE = 6 (YXDOMAIN).
    print("Step 22: BR 1 (DUT) sends SRP Update Response (YXDOMAIN).")
    pkts.\
        filter_ipv6_dst(ED_1_MLEID).\
        filter_ipv6_src(BR_1_MLEID).\
        filter(lambda p: p.dns.flags.response == 1).\
        filter(lambda p: p.dns.flags.rcode == 6).\
        must_next()


if __name__ == '__main__':
    verify_utils.run_main(verify)
