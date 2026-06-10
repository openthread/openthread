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

# IPv6 header size in octets.
IPV6_HEADER_SIZE = 40

# Small ICMPv6 Echo Request datagram size in octets.
SMALL_DATAGRAM_SIZE = 106

# Large ICMPv6 Echo Request datagram size in octets.
LARGE_DATAGRAM_SIZE = 1280

# Number of MED children.
NUM_MEDS = 4

# Number of SED children.
NUM_SEDS = 6

# ICMPv6 Echo Request identifier for MED children.
MED_ECHO_ID = 1001

# ICMPv6 Echo Request identifier for SED children.
SED_ECHO_ID = 2001


def verify_ping_forwarding(pv, start_idx, children_rloc16, children_mleid, base_echo_id):
    """Verify that the Router (DUT) properly forwards ICMPv6 Echo Requests and Replies.
    """
    pkts = pv.pkts
    LEADER_RLOC16 = pv.vars['LEADER_RLOC16']
    ROUTER_1_RLOC16 = pv.vars['ROUTER_1_RLOC16']

    max_idx = pkts.index

    for i, (child_rloc16, child_mleid) in enumerate(zip(children_rloc16, children_mleid)):
        f = pkts.range(start_idx).copy()

        # Router to child
        f.filter_ping_request(identifier=base_echo_id + i).\
            filter_wpan_src16(ROUTER_1_RLOC16).\
            filter_wpan_dst16(child_rloc16).\
            filter_ipv6_dst(child_mleid).\
            must_next()

        # Child to Router
        f.filter_ping_reply(identifier=base_echo_id + i).\
            filter_wpan_src16(child_rloc16).\
            filter_wpan_dst16(ROUTER_1_RLOC16).\
            must_next()

        # Router to Leader
        f.filter_ping_reply(identifier=base_echo_id + i).\
            filter_wpan_src16(ROUTER_1_RLOC16).\
            filter_wpan_dst16(LEADER_RLOC16).\
            must_next()

        max_idx = max(max_idx, f.index)

    return max_idx


def verify(pv):
    # 5.1.7 Minimum Supported Children – IPv6 Datagram Buffering
    #
    # 5.1.7.1 Topology
    # - Leader
    # - Router_1 (DUT)
    # - MED_1 through MED_4
    # - SED_1 through SED_6
    #
    # 5.1.7.2 Purpose & Description
    # The purpose of this test case is to validate the minimum conformance requirements for router-capable devices:
    # - a) Minimum number of supported children.
    # - b) Minimum MTU requirement when sending/forwarding an IPv6 datagram to a SED.
    # - c) Minimum number of sent/forwarded IPv6 datagrams to SED children.
    #
    # Spec Reference       | V1.1 Section | V1.3.0 Section
    # ---------------------|--------------|---------------
    # Conformance Document | 2.2          | 2.2

    pkts = pv.pkts
    pv.summary.show()

    LEADER = pv.vars['LEADER']
    LEADER_RLOC16 = pv.vars['LEADER_RLOC16']
    ROUTER_1 = pv.vars['ROUTER_1']
    ROUTER_1_RLOC16 = pv.vars['ROUTER_1_RLOC16']
    MEDS_RLOC16 = verify_utils.get_vars(pv, 'MED', NUM_MEDS, '_RLOC16')
    MEDS_MLEID = verify_utils.get_vars(pv, 'MED', NUM_MEDS, '_MLEID')
    SEDS_RLOC16 = verify_utils.get_vars(pv, 'SED', NUM_SEDS, '_RLOC16')
    SEDS_MLEID = verify_utils.get_vars(pv, 'SED', NUM_SEDS, '_MLEID')

    # Step 1: Leader, Router_1 (DUT), Children
    # - Description: Create topology and attach MED_1, MED_2…MED_4, SED_1, SED_2…SED_6 children to the Router.
    # - Pass Criteria:
    #   - The DUT MUST send properly formatted MLE Parent Response and MLE Child ID Response to each child.
    print("Step 1: Leader, Router_1 (DUT), Children")

    # Use copy to allow interleaving between different children
    start_pkts = pkts.copy()
    max_idx = pkts.index

    children = verify_utils.get_vars(pv, 'MED', NUM_MEDS) + verify_utils.get_vars(pv, 'SED', NUM_SEDS)

    for child in children:
        # Parent Response
        f = start_pkts.copy().\
            filter_wpan_src64(ROUTER_1).\
            filter_wpan_dst64(child).\
            filter_mle_cmd(consts.MLE_PARENT_RESPONSE)
        f.must_next()
        max_idx = max(max_idx, f.index)

        # Child ID Response
        f = start_pkts.copy().\
            filter_wpan_src64(ROUTER_1).\
            filter_wpan_dst64(child).\
            filter_mle_cmd(consts.MLE_CHILD_ID_RESPONSE)
        f.must_next()
        max_idx = max(max_idx, f.index)

    pkts.index = max_idx

    # Step 2: Leader
    # - Description: Harness instructs the Leader to send an ICMPv6 Echo Request with IPv6 datagram size of 106 octets
    #   to each MED.
    # - Pass Criteria:
    #   - The DUT MUST properly forward ICMPv6 Echo Requests to all MED children.
    #   - The DUT MUST properly forward ICMPv6 Echo Replies to the Leader.
    print("Step 2: Leader")

    step2_start_idx = pkts.index

    # Leader sends all requests to Router first (identifiers 1001..1004)
    for i, med_mleid in enumerate(MEDS_MLEID):
        pkts.filter_ping_request(identifier=MED_ECHO_ID + i).\
            filter_wpan_src16(LEADER_RLOC16).\
            filter_wpan_dst16(ROUTER_1_RLOC16).\
            filter_ipv6_dst(med_mleid).\
            filter(lambda p: p.ipv6.plen == SMALL_DATAGRAM_SIZE - IPV6_HEADER_SIZE).\
            must_next()

    # Verify Router forwards to MEDs and receives replies (allow interleaving with requests)
    pkts.index = verify_ping_forwarding(pv, step2_start_idx, MEDS_RLOC16, MEDS_MLEID, MED_ECHO_ID)

    # Step 3: Leader
    # - Description: Harness instructs the Leader to send an ICMPv6 Echo Request with IPv6 datagram size of 1280 octets
    #   to SED_1 and ICMPv6 Echo Requests with IPv6 datagram size of 106 octets to SED_2, SED_3, SED_4, SED_5 and SED_6
    #   without waiting for ICMPv6 Echo Replies.
    # - Pass Criteria:
    #   - The DUT MUST buffer all IPv6 datagrams.
    #   - The DUT MUST properly forward ICMPv6 Echo Requests to all SED children.
    #   - The DUT MUST properly forward ICMPv6 Echo Replies to the Leader.
    print("Step 3: Leader")

    step3_start_idx = pkts.index

    # Leader sends all requests to Router first (identifiers 2001..2006)
    # SED_1 (1280 octets)
    pkts.filter_ping_request(identifier=SED_ECHO_ID).\
        filter_wpan_src16(LEADER_RLOC16).\
        filter_wpan_dst16(ROUTER_1_RLOC16).\
        filter_ipv6_dst(SEDS_MLEID[0]).\
        filter(lambda p: p.ipv6.plen == LARGE_DATAGRAM_SIZE - IPV6_HEADER_SIZE).\
        must_next()

    # SED_2..6 (106 octets)
    for i in range(1, NUM_SEDS):
        pkts.filter_ping_request(identifier=SED_ECHO_ID + i).\
            filter_wpan_src16(LEADER_RLOC16).\
            filter_wpan_dst16(ROUTER_1_RLOC16).\
            filter_ipv6_dst(SEDS_MLEID[i]).\
            filter(lambda p: p.ipv6.plen == SMALL_DATAGRAM_SIZE - IPV6_HEADER_SIZE).\
            must_next()

    # Verify Router forwards to SEDs and receives replies (allow interleaving)
    pkts.index = verify_ping_forwarding(pv, step3_start_idx, SEDS_RLOC16, SEDS_MLEID, SED_ECHO_ID)


if __name__ == '__main__':
    verify_utils.run_main(verify)
