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
    # 5.1. [1.3] [CERT] Service discovery of services on Thread and Infrastructure - Single BR
    #
    # 5.1.1 Purpose
    # To test the following:
    # 1. Discovery proxy on the BR to perform mDNS query on the infrastructure link in the case where it doesn’t have
    #   any answer.
    # 2. Respond to unicast DNS queries made by a Thread Device on the Thread interface.
    # 3. Thread Device can discover services advertised by infrastructure device (mDNS) via BR DUT.
    # 4. Single BR case
    #
    # 5.1.2 Topology
    # - BR_1 (DUT) - Thread Border Router, and the Leader
    # - ED_1 - Test Bed device operating as a Thread End Device, attached to BR_1
    # - Eth_1 - Test Bed border router device on an Adjacent Infrastructure Link
    #
    # Spec Reference      | V1.1 Section | V1.3.0 Section
    # --------------------|--------------|---------------
    # Discovery Proxy     | N/A          | 5.1

    pkts = pv.pkts
    pv.summary.show()

    BR_1 = pv.vars['BR_1']
    BR_1_MLEID = pv.vars['BR_1_MLEID']
    ED_1 = pv.vars['ED_1']
    ED_1_MLEID = pv.vars['ED_1_MLEID']
    Eth_1_ETH = pv.vars['Eth_1_ETH']

    # Step 1: Device: BR_1 (DUT) Description (DPR-5.1): Enable
    # - Pass Criteria: N/A
    print("Step 1: BR_1 (DUT) enabled and became Leader.")

    # Step 2: Device: Eth_1, ED_1 Description (DPR-5.1): Enable
    # - Pass Criteria: N/A
    print("Step 2: Eth_1 and ED_1 enabled and joined the network.")

    # Step 3: Device: BR_1 (DUT) Description (DPR-5.1): Automatically adds its SRP Server information...
    # - Pass Criteria:
    #   - The DUT’s SRP Server information MUST appear in the Thread Network Data.
    #   - It MUST be a DNS/SRP Unicast Dataset, or DNS/SRP Anycast Dataset, or both.
    print("Step 3: BR_1 (DUT) adds its SRP Server information to the Thread Network Data.")
    pkts.filter_wpan_src64(BR_1).\
        filter_mle_cmd(consts.MLE_DATA_RESPONSE).\
        filter(lambda p: {
            consts.NWD_SERVICE_TLV,
            consts.NWD_SERVER_TLV
        } <= set(p.thread_nwd.tlv.type)).\
        must_next()

    # Step 4: Device: Eth_1 Description (DPR-5.1): Harness instructs device to advertise N=5 services...
    # - Pass Criteria: N/A
    print("Step 4: Eth_1 advertises 5 services using mDNS.")
    pkts.filter_eth_src(Eth_1_ETH).\
        filter(lambda p: p.mdns).\
        filter(lambda p: p.mdns.count.answers > 0).\
        must_next()

    # Step 5: Device: ED_1 Description (DPR-5.1): Harness instructs the device to send an SRP Update...
    # - Pass Criteria: N/A
    print("Step 5: ED_1 sends an SRP Update to BR_1 (DUT).")
    pkts.filter_ipv6_src(ED_1_MLEID).\
        filter_ipv6_dst(BR_1_MLEID).\
        filter(lambda p: p.dns).\
        filter(lambda p: p.dns.flags.opcode == 5).\
        must_next()

    # Step 6: Device: BR_1 (DUT) Description (DPR-5.1): Automatically sends SRP Update Response with SUCCESS
    # - Pass Criteria:
    #   - The DUT MUST send SRP Update Response with status 0 ( “NoError” ).
    print("Step 6: BR_1 (DUT) sends SRP Update Response with SUCCESS.")
    pkts.filter_ipv6_src(BR_1_MLEID).\
        filter_ipv6_dst(ED_1_MLEID).\
        filter(lambda p: p.dns).\
        filter(lambda p: p.dns.flags.opcode == 5).\
        filter(lambda p: p.dns.flags.rcode == consts.DNS_RCODE_NOERROR).\
        must_next()

    # Step 7: Device: ED_1 Description (DPR-5.1): Harness instructs the device to unicast DNS query QType=PTR...
    # - Pass Criteria: N/A
    print("Step 7: ED_1 sends unicast DNS query QType=PTR to BR_1 (DUT).")
    pkts.filter_ipv6_src(ED_1_MLEID).\
        filter_ipv6_dst(BR_1_MLEID).\
        filter(lambda p: p.dns).\
        filter(lambda p: p.dns.flags.response == 0).\
        filter(lambda p: "_infra-test._udp.default.service.arpa" in verify_utils.as_list(p.dns.qry.name)).\
        filter(lambda p: any(t == consts.DNS_TYPE_PTR for t in verify_utils.as_list(p.dns.qry.type))).\
        must_next()

    # Step 8: Device: BR_1 (DUT) Description (DPR-5.1): Automatically MAY send mDNS query...
    # - Pass Criteria: N/A
    print("Step 8: BR_1 (DUT) MAY send mDNS query on the infrastructure link.")

    # Step 9: Device: Eth_1 Description (DPR-5.1): In case DUT queried in step 8, it automatically sends mDNS...
    # - Pass Criteria: N/A
    print("Step 9: Eth_1 sends mDNS response with its active services.")
    pkts.filter_eth_src(Eth_1_ETH).\
        filter(lambda p: p.mdns).\
        filter(lambda p: p.mdns.count.answers > 0).\
        must_next()

    # Step 10: Device: BR_1 (DUT) Description (DPR-5.1): Automatically sends a DNS response to ED_1
    # - Pass Criteria: N/A
    print("Step 10: BR_1 (DUT) sends a DNS response to ED_1.")

    # Step 11: Device: ED_1 Description (DPR-5.1): Receives the DNS response with at least one of the N services
    # - Pass Criteria:
    #   - The DUT must send a valid DNS Response, containing in the Answer records section at least one of the services
    #     advertised by Eth_1, specifically the below where N is a number out of 1, 2, 3, 4, 5:
    #     $ORIGIN default.service.arpa. _infra-test._udp ( PTR service-test-N._infra-test._udp )
    #   - It MAY contain in the Additional records section one or more of the below services identified by number N
    #     plus the host (AAAA) record:
    #     $ORIGIN default.service.arpa. service-test-1._infra-test._udp ( SRV 0 0 55551 host-test-eth TXT
    #     dummy=abcd;thread-test=a49 ) ... host-test-eth AAAA <ULA address of Eth_1>
    #   - Response MUST NOT contain any AAAA record with a link-local address.
    #   - Response MUST NOT contain “service-test-6” records.
    #   - Response MUST NOT contain any “host-test-1” records.
    print("Step 11: ED_1 receives the DNS response with at least one of the 5 services.")
    pkts.filter_ipv6_src(BR_1_MLEID).\
        filter_ipv6_dst(ED_1_MLEID).\
        filter(lambda p: p.dns).\
        filter(lambda p: p.dns.flags.response == 1).\
        filter(lambda p: p.dns.count.answers > 0).\
        filter(lambda p: any(name.startswith("service-test-")
                             for name in verify_utils.as_list(p.dns.ptr.domain_name))).\
        filter(lambda p: any(name.endswith("._infra-test._udp.default.service.arpa")
                             for name in verify_utils.as_list(p.dns.ptr.domain_name))).\
        filter(lambda p: not any(addr.startswith("fe80")
                                 for addr in verify_utils.as_list(p.dns.aaaa))).\
        filter(lambda p: all("service-test-6" not in str(name)
                             for name in verify_utils.as_list(p.dns.ptr.domain_name))).\
        filter(lambda p: all("host-test-1" not in str(name)
                             for name in verify_utils.as_list(p.dns.resp.name))).\
        must_next()


if __name__ == '__main__':
    verify_utils.run_main(verify)
