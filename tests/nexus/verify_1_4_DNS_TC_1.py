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
    # 11.1. [1.3] [CERT] Handling of multi-question DNS queries
    #
    # 11.1.1. Purpose
    #   - To verify that the Thread Border Router DUT can successfully handle DNS queries with
    #     multiple questions (QDCOUNT > 1) by either:
    #   - Responding with an error status (signaling to the querier that it needs to retry with
    #     single-question queries)
    #   - Providing the answers to the multiple questions (as defined by Thread) if the questions
    #     are for the same QNAME but different record types.
    #
    # 11.1.2. Topology
    #   - Eth_1 host registering a service
    #   - BR_1 Border Router DUT
    #   - Thread Router_1 registering service(s) (Connected to BR DUT)
    #   - Thread ED_1 sending QDCOUNT > 1 queries (Child of Router_1)

    pkts = pv.pkts
    pv.summary.show()

    BR_1 = pv.vars['BR_1']
    ED_1 = pv.vars['ED_1']
    BR_1_MLEID = pv.vars['BR_1_MLEID']
    ED_1_MLEID = pv.vars['ED_1_MLEID']

    # Step 1
    #   Device: All
    #   Description (DNS-11.1): Start topology
    #   Pass Criteria:
    #     - N/A
    print("Step 1: Topology: Start Eth_1, BR_1, Router_1, and ED_1.")

    # Step 2
    #   Device: Eth_1
    #   Description (DNS-11.1): Harness instructs device to advertise a test service using
    #     mDNS: $ORIGIN local. Service-test-2._thread-test._udp ( SRV 0 0 55556 host-eth-1
    #     TXT key2=value2 ) host-eth-1 AAAA <ULA address of Eth_1>
    #   Pass Criteria:
    #     - mDNS service MUST be registered without name conflict or service name change.
    print("Step 2: Eth_1 host registers a service using mDNS.")

    # Step 3
    #   Device: Router_1
    #   Description (DNS-11.1): Harness instructs device to register a test service using
    #     SRP: $ORIGIN default.service.arpa. service-test-1._thread-test._udp ( SRV 0 0
    #     55556 host-router-1 TXT key1=value1 ) host-router-1 AAAA <OMR address of
    #     Router_1>
    #   Pass Criteria:
    #     - The DUT MUST respond with success status (RCODE=0) to the SRP registration.
    print("Step 3: Router_1 host registers a service using SRP.")

    # Step 4
    #   Device: ED_1
    #   Description (DNS-11.1): Harness instructs device to perform DNS query with QDCOUNT=2:
    #     First question QNAME=service-test-1._thread-test._udp. Default.service.arpa QTYPE
    #     =SRV Second question QNAME=service-test-1._thread-test._udp. Default.service.arpa
    #     QTYPE =TXT
    #   Pass Criteria:
    #     - The DUT MUST either: 3. Respond with error RCODE=1 (FormErr) and 0 answers; or 3.
    #       Respond with success RCODE=0 (NoError) and two answer records in the Answers
    #       section as follows: $ORIGIN default.service.arpa.
    #       service-test-1._thread-test._udp ( SRV 0 0 55556 host-router-1 TXT key1=value1 )
    #     - Furthermore the Additional records section MAY contain : host-router-1 AAAA <OMR
    #       address of Router_1>
    #     - If this record is present, address MUST be same as in step 9.
    print("Step 4: ED_1 performs a DNS query with two questions (QDCOUNT=2).")
    pkts.filter_wpan_src64(ED_1).\
        filter_ipv6_dst(BR_1_MLEID).\
        filter(lambda p: p.dns.flags.response == 0).\
        filter(lambda p: int(p.dns.count.queries) == 2).\
        must_next()

    pkts.filter_ipv6_src(BR_1_MLEID).\
        filter_ipv6_dst(ED_1_MLEID).\
        filter(lambda p: p.dns.flags.response == 1).\
        filter(lambda p: (p.dns.flags.rcode == consts.DNS_RCODE_NOERROR and
                          int(p.dns.count.answers) == 2 and
                          55556 in verify_utils.as_list(p.dns.srv.port) and
                          "host-router-1.default.service.arpa" in verify_utils.as_list(p.dns.srv.target) and
                          any(b"key1=value1" in t for t in verify_utils.as_list(p.dns.txt))) or
                         (p.dns.flags.rcode == 1)).\
        must_next()

    # Step 5
    #   Device: ED_1
    #   Description (DNS-11.1): Harness instructs devie to perform DNS query with QDCOUNT=2:
    #     First question QNAME=service-test-2._thread-test._udp. Default.service.arpa QTYPE
    #     =SRV Second question QNAME=service-test-2._thread-test._udp. Default.service.arpa
    #     QTYPE =TXT
    #   Pass Criteria:
    #     - The DUT MUST either: 3. 3. Respond with error RCODE=1 (FormErr) and 0 answers;
    #       or Respond with success RCODE=0 (NoError) and two answer records in the
    #       Answers section as follows: $ORIGIN default.service.arpa.
    #       service-test-2._thread-test._udp ( SRV 0 0 55556 host-eth-1 TXT key2=value2 )
    #     - Furthermore the Additional records section MAY contain : host-eth-1 AAAA <ULA
    #       address of Eth_1>
    #     - If this record is present, address MUST be same as in step 8.
    print("Step 5: ED_1 performs a DNS query with two questions (QDCOUNT=2).")
    pkts.filter_wpan_src64(ED_1).\
        filter_ipv6_dst(BR_1_MLEID).\
        filter(lambda p: p.dns.flags.response == 0).\
        filter(lambda p: int(p.dns.count.queries) == 2).\
        must_next()

    pkts.filter_ipv6_src(BR_1_MLEID).\
        filter_ipv6_dst(ED_1_MLEID).\
        filter(lambda p: p.dns.flags.response == 1).\
        filter(lambda p: (p.dns.flags.rcode == consts.DNS_RCODE_NOERROR and
                          int(p.dns.count.answers) == 2 and
                          55556 in verify_utils.as_list(p.dns.srv.port) and
                          "host-eth-1.default.service.arpa" in verify_utils.as_list(p.dns.srv.target) and
                          any(b"key2=value2" in t for t in verify_utils.as_list(p.dns.txt))) or
                         (p.dns.flags.rcode == 1)).\
        must_next()

    # Step 6
    #   Device: ED_1
    #   Description (DNS-11.1): Harness instructs device to perform DNS query with
    #     QDCOUNT=1: First question QNAME=service-test-1_thread-test.
    #     _udp.default.service.arpa QTYPE=SRV
    #   Pass Criteria:
    #     - The DUT MUST respond with success RCODE=0 (NoError) and one answer record in
    #       the Answers section as follows: $ORIGIN default.service.arpa.
    #       service-test-1._thread-test._udp ( SRV 0 0 55556 host-router-1 )
    #     - Furthermore the Additional records section MAY contain : host-router-1 AAAA
    #       <OMR address of Router_1>
    print("Step 6: ED_1 performs a DNS query for service-test-1 (SRV).")
    pkts.filter_wpan_src64(ED_1).\
        filter_ipv6_dst(BR_1_MLEID).\
        filter(lambda p: p.dns.flags.response == 0).\
        filter(lambda p: "service-test-1._thread-test._udp.default.service.arpa" in
                         verify_utils.as_list(p.dns.qry.name) and
                         consts.DNS_TYPE_SRV in verify_utils.as_list(p.dns.qry.type)).\
        must_next()

    pkts.filter_ipv6_src(BR_1_MLEID).\
        filter_ipv6_dst(ED_1_MLEID).\
        filter(lambda p: p.dns.flags.response == 1).\
        filter(lambda p: p.dns.flags.rcode == consts.DNS_RCODE_NOERROR and
                         int(p.dns.count.answers) >= 1 and
                         55556 in verify_utils.as_list(p.dns.srv.port) and
                         "host-router-1.default.service.arpa" in verify_utils.as_list(p.dns.srv.target)).\
        must_next()

    # Step 7
    #   Device: ED_1
    #   Description (DNS-11.1): Harness instructs device to perform DNS query with
    #     QDCOUNT=1: First question QNAME=service-test-2_thread-test.
    #     _udp.default.service.arpa QTYPE=TXT
    #   Pass Criteria:
    #     - The DUT MUST respond with success RCODE=0 (NoError) and one answer record in
    #       the Answers section as follows: $ORIGIN default.service.arpa.
    #       service-test-2._thread-test._udp ( TXT key2=value2 )
    print("Step 7: ED_1 performs a DNS query for service-test-2 (TXT).")
    pkts.filter_wpan_src64(ED_1).\
        filter_ipv6_dst(BR_1_MLEID).\
        filter(lambda p: p.dns.flags.response == 0).\
        filter(lambda p: "service-test-2._thread-test._udp.default.service.arpa" in
                         verify_utils.as_list(p.dns.qry.name) and
                         consts.DNS_TYPE_TXT in verify_utils.as_list(p.dns.qry.type)).\
        must_next()

    pkts.filter_ipv6_src(BR_1_MLEID).\
        filter_ipv6_dst(ED_1_MLEID).\
        filter(lambda p: p.dns.flags.response == 1).\
        filter(lambda p: p.dns.flags.rcode == consts.DNS_RCODE_NOERROR and
                         int(p.dns.count.answers) >= 1 and
                         any(b"key2=value2" in t for t in verify_utils.as_list(p.dns.txt))).\
        must_next()

    # Step 8
    #   Device: ED_1
    #   Description (DNS-11.1): Harness instructs device to perform DNS query with
    #     QDCOUNT=1: First question QNAME=host-eth-1.default.service.arpa QTYPE=AAAA
    #   Pass Criteria:
    #     - The DUT MUST respond with success RCODE=0 (NoError) and one answer record in
    #       the Answers section as follows: $ORIGIN default.service.arpa. host-eth-1 AAAA
    #       <ULA address of Eth_1>
    #     - The DUT MUST NOT include a link-local address in an AAAA record in the
    #       answer(s).
    print("Step 8: ED_1 performs a DNS query for host-eth-1 (AAAA).")
    pkts.filter_wpan_src64(ED_1).\
        filter_ipv6_dst(BR_1_MLEID).\
        filter(lambda p: p.dns.flags.response == 0).\
        filter(lambda p: "host-eth-1.default.service.arpa" in
                         verify_utils.as_list(p.dns.qry.name) and
                         consts.DNS_TYPE_AAAA in verify_utils.as_list(p.dns.qry.type)).\
        must_next()

    pkts.filter_ipv6_src(BR_1_MLEID).\
        filter_ipv6_dst(ED_1_MLEID).\
        filter(lambda p: p.dns.flags.response == 1).\
        filter(lambda p: p.dns.flags.rcode == consts.DNS_RCODE_NOERROR).\
        filter(lambda p: all(not verify_utils.Ipv6Addr(addr).is_link_local
                             for addr in verify_utils.as_list(p.dns.aaaa))).\
        must_next()

    # Step 9
    #   Device: ED_1
    #   Description (DNS-11.1): Harness instructs device to perform DNS query with
    #     QDCOUNT=1: First question QNAME=host-router-1.default.service.arpa QTYPE=AAAA
    #   Pass Criteria:
    #     - The DUT MUST respond with success RCODE=0 (NoError) and one answer record in
    #       the Answers section as follows: $ORIGIN default.service.arpa. host-router-1
    #       AAAA <OMR address of Router_1>
    #     - The DUT MUST NOT include a link-local address in an AAAA record in the
    #       answer(s).
    print("Step 9: ED_1 performs a DNS query for host-router-1 (AAAA).")
    pkts.filter_wpan_src64(ED_1).\
        filter_ipv6_dst(BR_1_MLEID).\
        filter(lambda p: p.dns.flags.response == 0).\
        filter(lambda p: "host-router-1.default.service.arpa" in
                         verify_utils.as_list(p.dns.qry.name) and
                         consts.DNS_TYPE_AAAA in verify_utils.as_list(p.dns.qry.type)).\
        must_next()

    pkts.filter_ipv6_src(BR_1_MLEID).\
        filter_ipv6_dst(ED_1_MLEID).\
        filter(lambda p: p.dns.flags.response == 1).\
        filter(lambda p: p.dns.flags.rcode == consts.DNS_RCODE_NOERROR).\
        filter(lambda p: all(not verify_utils.Ipv6Addr(addr).is_link_local
                             for addr in verify_utils.as_list(p.dns.aaaa))).\
        must_next()


if __name__ == '__main__':
    verify_utils.run_main(verify)
