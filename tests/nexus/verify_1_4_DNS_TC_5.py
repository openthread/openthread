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
    # 11.5. [1.4] [CERT] DNS record types and special cases
    #
    # 11.5.1. Purpose
    #   To verify that the BR DUT:
    #   - Can resolve A records
    #   - Can resolve A records without doing IPv6 AAAA synthesis
    #   - Can resolve AAAA records
    #   - Can resolve records contributed by different sources (on-mesh, on-AIL, and upstream DNS server)
    #   - Supports non-typical record types (RRTypes) for queries
    #   - Supports queries using RR type 0xFF00-0xFFFE (65280-65534) “Private Use”
    #   - Blocks the “ipv4only.arpa” query

    pkts = pv.pkts
    pv.summary.show()

    # Step 1
    #   Device: Eth_1, BR_1, ED_1
    #   Description (DNS-11.5): Form topology. Thread network is formed.
    #   Pass Criteria:
    #     - N/A
    print("Step 1: Form topology. Thread network is formed.")

    # Step 2
    #   Device: Eth_1
    #   Description (DNS-11.5): Harness instructs device to configure DNS server with records:
    #     threadgroup1.org AAAA 2002:1234::E1:1, threadgroup1.org A 192.168.217.1,
    #     threadgroup2.org A 192.168.217.2, threadgroup2.org TYPE65285 \# 4 12345678.
    #     Note: the record type 0xFF05 is a private range record type, hence it has the
    #     symbolic name TYPE65285 associated.
    #   Pass Criteria:
    #     - N/A
    print("Step 2: Eth_1 configures DNS server with records.")

    # Step 3
    #   Device: Eth_1
    #   Description (DNS-11.5): Harness instructs device to start advertising mDNS services on the AIL:
    #     Note: the PTR records for services are not shown below; however these are present as usual.
    #     $ORIGIN local. Service-test-1._infra-test._udp ( SRV 0 0 55551 host-test-eth TXT
    #     dummy=abcd;thread-test=a49 ) service-test-2._infra-test._udp ( SRV 0 0 55552
    #     host-test-eth TXT a=2;thread-test=b50_test ) ) host-test-eth A <IPv4 address of Eth_1>.
    #   Pass Criteria:
    #     - N/A
    print("Step 3: Eth_1 starts advertising mDNS services on the AIL.")

    # Step 4
    #   Device: ED_1
    #   Description (DNS-11.5): Harness instructs device to send a DNS query to the DUT with
    #     Qtype=AAAA for name “threadgroup1.org”.
    #   Pass Criteria:
    #     - N/A
    print("Step 4: ED_1 sends a DNS query to the DUT with Qtype=AAAA for name threadgroup1.org.")
    pkts.filter(lambda p: getattr(p, 'dns', None) and p.dns.flags.response == 0).\
        filter(lambda p: any(name in ("threadgroup1.org", "threadgroup1.org.") for name in verify_utils.as_list(p.dns.qry.name))).\
        filter(lambda p: consts.DNS_TYPE_AAAA in verify_utils.as_list(p.dns.qry.type)).\
        must_next()

    # Step 5
    #   Device: BR_1 (DUT)
    #   Description (DNS-11.5): Automatically performs the DNS query via the Eth_1 DNS server,
    #     and provides the answer back to ED_1.
    #   Pass Criteria:
    #     - The DUT MUST respond Rcode=0 (NoError) with answer record: threadgroup1.org
    #       AAAA 2002:1234::E1:1
    print("Step 5: BR_1 performs the DNS query via Eth_1 and provides the answer to ED_1.")
    pkts.filter(lambda p: getattr(p, 'dns', None) and p.dns.flags.response == 1).\
        filter(lambda p: consts.DNS_TYPE_AAAA in verify_utils.as_list(p.dns.qry.type)).\
        filter(lambda p: p.dns.flags.rcode == consts.DNS_RCODE_NOERROR).\
        filter(lambda p: int(p.dns.count.answers) > 0).\
        filter(lambda p: any(verify_utils.Ipv6Addr("2002:1234::e1:1") == addr
                             for addr in verify_utils.as_list(p.dns.aaaa))).\
        must_next()

    # Step 6
    #   Device: ED_1
    #   Description (DNS-11.5): Harness instructs device to send a DNS query to the DUT, with
    #     Qtype=A for name “threadgroup1.org”.
    #   Pass Criteria:
    #     - N/A
    print("Step 6: ED_1 sends a DNS query to the DUT with Qtype=A for name threadgroup1.org.")
    pkts.filter(lambda p: getattr(p, 'dns', None) and p.dns.flags.response == 0).\
        filter(lambda p: any(name in ("threadgroup1.org", "threadgroup1.org.") for name in verify_utils.as_list(p.dns.qry.name))).\
        filter(lambda p: consts.DNS_TYPE_A in verify_utils.as_list(p.dns.qry.type)).\
        must_next()

    # Step 7
    #   Device: BR_1 (DUT)
    #   Description (DNS-11.5): Automatically performs the DNS query via the Eth_1 DNS server,
    #     and provides the answer back to ED_1.
    #   Pass Criteria:
    #     - The DUT MUST respond Rcode=0 (NoError) with answer record: threadgroup1.org A 192.168.217.1
    print("Step 7: BR_1 performs the DNS query via Eth_1 and provides the answer to ED_1.")
    pkts.filter(lambda p: getattr(p, 'dns', None) and p.dns.flags.response == 1).\
        filter(lambda p: consts.DNS_TYPE_A in verify_utils.as_list(p.dns.qry.type)).\
        filter(lambda p: p.dns.flags.rcode == consts.DNS_RCODE_NOERROR).\
        filter(lambda p: int(p.dns.count.answers) > 0).\
        filter(lambda p: any("192.168.217.1" == str(a) for a in verify_utils.as_list(p.dns.a))).\
        must_next()

    # Step 8
    #   Device: ED_1
    #   Description (DNS-11.5): Harness instructs device to send a DNS query to the DUT, with
    #     Qtype=AAAA for name “threadgroup2.org”.
    #   Pass Criteria:
    #     - N/A
    print("Step 8: ED_1 sends a DNS query to the DUT with Qtype=AAAA for name threadgroup2.org.")
    pkts.filter(lambda p: getattr(p, 'dns', None) and p.dns.flags.response == 0).\
        filter(lambda p: any(name in ("threadgroup2.org", "threadgroup2.org.") for name in verify_utils.as_list(p.dns.qry.name))).\
        filter(lambda p: consts.DNS_TYPE_AAAA in verify_utils.as_list(p.dns.qry.type)).\
        must_next()

    # Step 9
    #   Device: BR_1 (DUT)
    #   Description (DNS-11.5): Automatically performs the DNS query via the Eth_1 DNS server,
    #     and provides the error answer back to ED_1. Note: this checks that the DUT does not
    #     perform AAAA synthesis from the A record.
    #   Pass Criteria:
    #     - The DUT MUST respond Rcode=0 (NoError) with zero answer records (ANCOUNT=0).
    print("Step 9: BR_1 performs the DNS query via Eth_1 and provides the error answer to ED_1.")
    pkts.filter(lambda p: getattr(p, 'dns', None) and p.dns.flags.response == 1).\
        filter(lambda p: consts.DNS_TYPE_AAAA in verify_utils.as_list(p.dns.qry.type)).\
        filter(lambda p: p.dns.flags.rcode == consts.DNS_RCODE_NOERROR).\
        filter(lambda p: int(p.dns.count.answers) == 0).\
        must_next()

    # Step 10
    #   Device: ED_1
    #   Description (DNS-11.5): Harness instructs device to send a DNS query, with Qtype=A for
    #     name “host-test-eth.default.service.arpa”.
    #   Pass Criteria:
    #     - N/A
    print("Step 10: ED_1 sends a DNS query with Qtype=A for host-test-eth.default.service.arpa.")
    pkts.filter(lambda p: getattr(p, 'dns', None) and p.dns.flags.response == 0).\
        filter(lambda p: any(name in ("host-test-eth.default.service.arpa", "host-test-eth.default.service.arpa.") for name in verify_utils.as_list(p.dns.qry.name))).\
        filter(lambda p: consts.DNS_TYPE_A in verify_utils.as_list(p.dns.qry.type)).\
        must_next()

    # Step 11
    #   Device: BR_1 (DUT)
    #   Description (DNS-11.5): Automatically performs the mDNS query on AIL (optionally, if
    #     still needed), and provides the answer back to ED_1.
    #   Pass Criteria:
    #     - The DUT MUST respond Rcode=0 (NoError) with answer record:
    #       host-test-eth.default.service.arpa A <IPv4 address of Eth_1>
    print("Step 11: BR_1 performs mDNS query on AIL and provides the answer to ED_1.")
    pkts.filter(lambda p: getattr(p, 'dns', None) and p.dns.flags.response == 1).\
        filter(lambda p: consts.DNS_TYPE_A in verify_utils.as_list(p.dns.qry.type)).\
        filter(lambda p: p.dns.flags.rcode == consts.DNS_RCODE_NOERROR).\
        filter(lambda p: int(p.dns.count.answers) > 0).\
        filter(lambda p: any("192.168.217.100" == str(a) for a in verify_utils.as_list(p.dns.a))).\
        must_next()

    # Step 12
    #   Device: ED_1
    #   Description (DNS-11.5): Harness instructs device to send DNS query Qtype=A for name “ipv4only.arpa”
    #   Pass Criteria:
    #     - N/A
    print("Step 12: ED_1 sends DNS query Qtype=A for name ipv4only.arpa.")
    pkts.filter(lambda p: getattr(p, 'dns', None) and p.dns.flags.response == 0).\
        filter(lambda p: any(name in ("ipv4only.arpa", "ipv4only.arpa.") for name in verify_utils.as_list(p.dns.qry.name))).\
        filter(lambda p: consts.DNS_TYPE_A in verify_utils.as_list(p.dns.qry.type)).\
        must_next()

    # Step 13
    #   Device: BR_1 (DUT)
    #   Description (DNS-11.5): Automatically blocks the ipv4only.arpa query and responds NXDomain error.
    #   Pass Criteria:
    #     - The DUT MUST respond Rcode=3 (NXDomain) with zero answer records (ANCOUNT=0).
    #     - The DUT MUST NOT send a DNS query to Eth_1.
    print("Step 13: BR_1 blocks the ipv4only.arpa query and responds NXDomain error.")
    pkts.filter(lambda p: getattr(p, 'dns', None) and p.dns.flags.response == 1).\
        filter(lambda p: any(name in ("ipv4only.arpa", "ipv4only.arpa.") for name in verify_utils.as_list(p.dns.qry.name))).\
        filter(lambda p: p.dns.flags.rcode == consts.DNS_RCODE_NXDOMAIN).\
        filter(lambda p: int(p.dns.count.answers) == 0).\
        must_next()

    # Check that no query was sent to Eth_1 (upstream)
    pkts.copy().filter(lambda p: getattr(p, 'dns', None) and p.dns.flags.response == 0).\
        filter(lambda p: any(name in ("ipv4only.arpa", "ipv4only.arpa.") for name in verify_utils.as_list(p.dns.qry.name))).\
        filter(lambda p: p.eth.dst == "02:00:00:00:00:02").\
        must_not_next()

    # Step 14
    #   Device: ED_1
    #   Description (DNS-11.5): Harness instructs device to send DNS query with RR type in the
    #     private use range Qtype=0xFF05 for name “threadgroup2.org”. To do this with OT CLI,
    #     use the following: udp send <BR_1 ML-EID address> 53 -x
    #     e13c010000010000000000000c74687265616467726f757032036f726700ff050001 “”
    #   Pass Criteria:
    #     - N/A
    print("Step 14: ED_1 sends DNS query with RR type in private use range Qtype=0xFF05.")
    pkts.filter(lambda p: getattr(p, 'dns', None) and p.dns.flags.response == 0).\
        filter(lambda p: any(name in ("threadgroup2.org", "threadgroup2.org.") for name in verify_utils.as_list(p.dns.qry.name))).\
        filter(lambda p: any(65285 == t for t in verify_utils.as_list(p.dns.qry.type))).\
        must_next()

    # Step 15
    #   Device: BR_1 (DUT)
    #   Description (DNS-11.5): Automatically responds with the requested record.
    #   Pass Criteria:
    #     - The DUT MUST respond Rcode=0 (NoError) with answer record with binary data (4 bytes):
    #       threadgroup2.org 0xFF05 0x12345678
    print("Step 15: BR_1 responds with the requested record.")
    pkts.filter(lambda p: getattr(p, 'dns', None) and p.dns.flags.response == 1).\
        filter(lambda p: any(65285 == t for t in verify_utils.as_list(p.dns.qry.type))).\
        filter(lambda p: p.dns.flags.rcode == consts.DNS_RCODE_NOERROR).\
        filter(lambda p: int(p.dns.count.answers) > 0).\
        filter(lambda p: any("12345678" in str(getattr(p.dns, 'data', '')) for _ in [0])).\
        must_next()


if __name__ == '__main__':
    verify_utils.run_main(verify)
