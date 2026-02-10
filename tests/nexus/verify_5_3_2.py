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
    # 5.3.2 Realm-Local Addressing
    #
    # 5.3.2.1 Topology
    # - Leader
    # - Router 1
    # - Router 2 (DUT)
    # - SED 1
    #
    # 5.3.2.2 Purpose & Description
    # The purpose of this test case is to validate the Realm-Local addresses that the DUT configures.
    #
    # Spec Reference   | V1.1 Section | V1.3.0 Section
    # -----------------|--------------|---------------
    # Realm-Local Scope| 5.2.3.2      | 5.2.1.2

    pkts = pv.pkts
    pv.summary.show()

    LEADER = pv.vars['LEADER']
    DUT = pv.vars['DUT']
    SED_1 = pv.vars['SED_1']
    DUT_MLEID = pv.vars['DUT_MLEID']
    LEADER_MLEID = pv.vars['LEADER_MLEID']
    DUT_RLOC16 = pv.vars['DUT_RLOC16']
    SED_1_RLOC16 = pv.vars['SED_1_RLOC16']

    mesh_local_prefix_str = pv.vars['mesh_local_prefix']
    prefix = mesh_local_prefix_str.split('/')[0]
    # ff33:00:40:prefix:00000001
    realm_local_all_thread_nodes = Ipv6Addr(
        bytearray([0xff, 0x33, 0x00, 64]) + Ipv6Addr(prefix)[:8] + bytearray([0, 0, 0, 1]))

    ECHO_ID = 0x1234

    # Step 1: All
    # - Description: Build the topology as described and begin the wireless sniffer.
    # - Pass Criteria: N/A
    print("Step 1: All")
    pkts.filter_wpan_src64(LEADER).filter_mle_cmd(consts.MLE_ADVERTISEMENT).must_next()
    pkts.filter_wpan_src64(DUT).filter_mle_cmd(consts.MLE_ADVERTISEMENT).must_next()

    # Step 2: Leader
    # - Description: Harness instructs the device to send an ICMPv6 Echo Request to the DUT ML-EID.
    # - Pass Criteria: The DUT MUST respond with an ICMPv6 Echo Reply.
    print("Step 2: Leader")
    pkts.filter_wpan_src64(LEADER) \
        .filter_ipv6_dst(DUT_MLEID) \
        .filter_ping_request(identifier=ECHO_ID) \
        .must_next()
    pkts.filter_wpan_src64(DUT) \
        .filter_ipv6_dst(LEADER_MLEID) \
        .filter_ping_reply(identifier=ECHO_ID) \
        .must_next()

    # Step 3: Leader
    # - Description: Harness instructs the device to send a fragmented ICMPv6 Echo Request to the DUT ML-EID.
    # - Pass Criteria: The DUT MUST respond with an ICMPv6 Echo Reply.
    print("Step 3: Leader")
    pkts.filter_wpan_src64(LEADER) \
        .filter_ipv6_dst(DUT_MLEID) \
        .filter_ping_request(identifier=ECHO_ID) \
        .filter(lambda p: hasattr(p, 'lowpan') and hasattr(p.lowpan, 'fragment')) \
        .must_next()
    pkts.filter_wpan_src64(DUT) \
        .filter_ipv6_dst(LEADER_MLEID) \
        .filter_ping_reply(identifier=ECHO_ID) \
        .must_next()

    # Step 4: Leader
    # - Description: Harness instructs the device to send an ICMPv6 Echo Request to the Realm-Local All-Nodes
    #   multicast address (FF03::1).
    # - Pass Criteria:
    #   - The DUT MUST respond with an ICMPv6 Echo Reply.
    #   - The DUT MUST NOT forward the ICMPv6 Echo Request to SED_1.
    print("Step 4: Leader")
    pkts.filter_wpan_src64(LEADER) \
        .filter_ipv6_dst(consts.REALM_LOCAL_ALL_NODES_ADDRESS) \
        .filter_ping_request(identifier=ECHO_ID) \
        .must_next()
    pkts.filter_wpan_src64(DUT) \
        .filter_ipv6_dst(LEADER_MLEID) \
        .filter_ping_reply(identifier=ECHO_ID) \
        .must_next()
    pkts.copy().filter_wpan_src64(DUT) \
        .filter_wpan_dst64(SED_1) \
        .filter_ipv6_dst(consts.REALM_LOCAL_ALL_NODES_ADDRESS) \
        .must_not_next()

    # Step 5: Leader
    # - Description: Harness instructs the device to send a fragmented ICMPv6 Echo Request to the Realm-Local
    #   All-Nodes multicast address (FF03::1).
    # - Pass Criteria:
    #   - The DUT MUST respond with an ICMPv6 Echo Reply.
    #   - The DUT MUST NOT forward the ICMPv6 Echo Request to SED_1.
    print("Step 5: Leader")
    pkts.filter_wpan_src64(LEADER) \
        .filter_ipv6_dst(consts.REALM_LOCAL_ALL_NODES_ADDRESS) \
        .filter_ping_request(identifier=ECHO_ID) \
        .filter(lambda p: hasattr(p, 'lowpan') and hasattr(p.lowpan, 'fragment')) \
        .must_next()
    pkts.filter_wpan_src64(DUT) \
        .filter_ipv6_dst(LEADER_MLEID) \
        .filter_ping_reply(identifier=ECHO_ID) \
        .must_next()
    pkts.copy().filter_wpan_src64(DUT) \
        .filter_wpan_dst64(SED_1) \
        .filter_ipv6_dst(consts.REALM_LOCAL_ALL_NODES_ADDRESS) \
        .must_not_next()

    # Step 6: Leader
    # - Description: Harness instructs the device to send an ICMPv6 Echo Request to the Realm-Local All-Routers
    #   multicast address (FF03::2).
    # - Pass Criteria:
    #   - The DUT MUST respond with an ICMPv6 Echo Reply.
    #   - The DUT MUST NOT forward the ICMPv6 Echo Request to SED_1.
    print("Step 6: Leader")
    pkts.filter_wpan_src64(LEADER) \
        .filter_ipv6_dst(consts.REALM_LOCAL_ALL_ROUTERS_ADDRESS) \
        .filter_ping_request(identifier=ECHO_ID) \
        .must_next()
    pkts.filter_wpan_src64(DUT) \
        .filter_ipv6_dst(LEADER_MLEID) \
        .filter_ping_reply(identifier=ECHO_ID) \
        .must_next()
    pkts.copy().filter_wpan_src64(DUT) \
        .filter_wpan_dst64(SED_1) \
        .filter_ipv6_dst(consts.REALM_LOCAL_ALL_ROUTERS_ADDRESS) \
        .must_not_next()

    # Step 7: Leader
    # - Description: Harness instructs the device to send a fragmented ICMPv6 Echo Request to the Realm-Local
    #   All-Routers multicast address (FF03::2).
    # - Pass Criteria:
    #   - The DUT MUST respond with an ICMPv6 Echo Reply.
    #   - The DUT MUST NOT forward the ICMPv6 Echo Request to SED_1.
    print("Step 7: Leader")
    pkts.filter_wpan_src64(LEADER) \
        .filter_ipv6_dst(consts.REALM_LOCAL_ALL_ROUTERS_ADDRESS) \
        .filter_ping_request(identifier=ECHO_ID) \
        .filter(lambda p: hasattr(p, 'lowpan') and hasattr(p.lowpan, 'fragment')) \
        .must_next()
    pkts.filter_wpan_src64(DUT) \
        .filter_ipv6_dst(LEADER_MLEID) \
        .filter_ping_reply(identifier=ECHO_ID) \
        .must_next()
    pkts.copy().filter_wpan_src64(DUT) \
        .filter_wpan_dst64(SED_1) \
        .filter_ipv6_dst(consts.REALM_LOCAL_ALL_ROUTERS_ADDRESS) \
        .must_not_next()

    # Step 8: Leader
    # - Description: Harness instructs the device to send a Fragmented ICMPv6 Echo Request to the Realm-Local All
    #   Thread Nodes multicast address.
    # - Pass Criteria:
    #   - The Realm-Local All Thread Nodes multicast address MUST be a realm-local Unicast Prefix-Based Multicast
    #     Address [RFC 3306], with:
    #     - flgs set to 3 (P = 1 and T = 1)
    #     - scop set to 3
    #     - plen set to the Mesh Local Prefix length
    #     - network prefix set to the Mesh Local Prefix
    #     - group ID set to 1
    #   - The DUT MUST use IEEE 802.15.4 indirect transmissions to forward packet to SED_1.
    print("Step 8: Leader")
    pkts.filter_wpan_src64(LEADER) \
        .filter_ipv6_dst(realm_local_all_thread_nodes) \
        .filter_ping_request(identifier=ECHO_ID) \
        .filter(lambda p: hasattr(p, 'lowpan') and hasattr(p.lowpan, 'fragment')) \
        .must_next()
    pkts.filter(lambda p: p.wpan.src16 == SED_1_RLOC16) \
        .filter_wpan_cmd(consts.WPAN_DATA_REQUEST) \
        .must_next()
    # DUT forwards the packet to SED_1 responding to the Data Request.
    # We use RLOC16 filters because decryption might fail for these packets.
    pkts.filter(lambda p: p.wpan.src16 == DUT_RLOC16 and p.wpan.dst16 == SED_1_RLOC16) \
        .must_next()
    # SED_1 sends Echo Reply (unicast to Leader).
    # Unicast packets are usually mapped and decrypted correctly if mapping was learned.
    pkts.filter(lambda p: p.wpan.src16 == SED_1_RLOC16) \
        .filter_ipv6_dst(LEADER_MLEID) \
        .filter_ping_reply(identifier=ECHO_ID) \
        .must_next()


if __name__ == '__main__':
    verify_utils.run_main(verify)
