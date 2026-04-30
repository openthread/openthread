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


def _verify_ping(pkts, pv, src_name, dst_name):
    """Helper to verify a ping request and reply."""
    src_mleid = pv.vars[f'{src_name}_MLEID']
    dst_mleid = pv.vars[f'{dst_name}_MLEID']

    _pkt = pkts.filter_ping_request().\
        filter_ipv6_src(src_mleid).\
        filter_ipv6_dst(dst_mleid).\
        must_next()
    pkts.filter_ping_reply(identifier=_pkt.icmpv6.echo.identifier).\
        filter_ipv6_src(dst_mleid).\
        filter_ipv6_dst(src_mleid).\
        must_next()


def _verify_mle_advertisement(pkts, pv, node_name):
    """Helper to verify an MLE Advertisement packet."""
    node_mac = pv.vars[node_name]
    pkts.copy().\
        filter_wpan_src64(node_mac).\
        filter_LLANMA().\
        filter_mle_cmd(consts.MLE_ADVERTISEMENT).\
        filter(lambda p: {
                          consts.SOURCE_ADDRESS_TLV,
                          consts.LEADER_DATA_TLV,
                          consts.ROUTE64_TLV
                          } <= set(p.mle.tlv.type)).\
        filter(lambda p: p.ipv6.hlim == 255).\
        must_next()


def verify(pv):
    # 5.5.7 Split/Merge Routers: Three-way Separated
    #
    # 5.5.7.1 Topology
    # - Topology A
    # - Topology B
    #
    # 5.5.7.2 Purpose & Description
    # The purpose of this test case is to show that Router_1 will create a new partition once the Leader is removed
    #   from the network for a time period longer than the leader timeout (120 seconds), and the network will merge
    #   once the Leader is reintroduced to the network.
    #
    # Spec Reference            | V1.1 Section | V1.3.0 Section
    # --------------------------|--------------|---------------
    # Thread Network Partitions | 5.16         | 5.16

    pkts = pv.pkts
    pv.summary.show()

    LEADER = pv.vars['LEADER']
    ROUTER_1 = pv.vars['ROUTER_1']
    ROUTER_2 = pv.vars['ROUTER_2']
    ROUTER_3 = pv.vars['ROUTER_3']

    # Step 1: All
    # - Description: Ensure topology is formed correctly.
    # - Pass Criteria: N/A
    print("Step 1: All")

    # Step 2: Leader, Router_1
    # - Description: Transmit MLE advertisements.
    # - Pass Criteria:
    #   - Devices MUST send properly formatted MLE Advertisements.
    #   - Advertisements MUST be sent with an IP Hop Limit of 255 to the Link-Local All Nodes multicast address
    #     (FF02::1).
    #   - The following TLVs MUST be present in the MLE Advertisements:
    #     - Source Address TLV
    #     - Leader Data TLV
    #     - Route64 TLV
    print("Step 2: Leader, Router_1")
    _verify_mle_advertisement(pkts, pv, 'LEADER')
    _verify_mle_advertisement(pkts, pv, 'ROUTER_1')

    # Step 3: Leader
    # - Description: Power off Leader for 140 seconds.
    # - Pass Criteria: The Leader stops sending MLE advertisements.
    print("Step 3: Leader")

    # Step 4: Router_2, Router_3
    # - Description: Each router forms a partition with the lowest possible partition ID.
    # - Pass Criteria: N/A
    print("Step 4: Router_2, Router_3")
    pkts.copy().\
        filter_wpan_src64(ROUTER_2).\
        filter_mle_cmd(consts.MLE_ADVERTISEMENT).\
        must_next()
    pkts.copy().\
        filter_wpan_src64(ROUTER_3).\
        filter_mle_cmd(consts.MLE_ADVERTISEMENT).\
        must_next()

    # Step 5: Router_1
    # - Description: Automatically attempts to reattach to previous partition.
    # - Pass Criteria:
    #   - Router_1 MUST send MLE Parent Requests to the Link-Local All-Routers multicast address with an IP Hop Limit
    #     of 255.
    #   - The following TLVs MUST be present in the Parent Request:
    #     - Challenge TLV
    #     - Mode TLV
    #     - Scan Mask TLV (MUST have E and R flags set)
    #     - Version TLV
    #   - The Router MUST make two separate attempts to reconnect to its current Partition in this manner.
    print("Step 5: Router_1")
    parent_req_filter = pkts.\
        filter_wpan_src64(ROUTER_1).\
        filter_LLARMA().\
        filter_mle_cmd(consts.MLE_PARENT_REQUEST).\
        filter(lambda p: {
                          consts.CHALLENGE_TLV,
                          consts.MODE_TLV,
                          consts.SCAN_MASK_TLV,
                          consts.VERSION_TLV
                          } <= set(p.mle.tlv.type)).\
        filter(lambda p: p.ipv6.hlim == 255).\
        filter(lambda p: p.mle.tlv.scan_mask.r == 1).\
        filter(lambda p: p.mle.tlv.scan_mask.e == 1)
    parent_req_filter.must_next()
    parent_req_filter.must_next()

    # Step 6: Leader
    # - Description: Does NOT respond to MLE Parent Requests.
    # - Pass Criteria: The Leader does not respond to the Parent Requests.
    print("Step 6: Leader")

    # Step 7: Router_1
    # - Description: Automatically attempts to reattach to any partition.
    # - Pass Criteria:
    #   - Router_1 MUST attempt to reattach to any partition by sending MLE Parent Requests to the All-Routers
    #     multicast address with an IP Hop Limit of 255.
    #   - The following TLVs MUST be present in the Parent Request:
    #     - Challenge TLV
    #     - Mode TLV
    #     - Scan Mask TLV
    #     - Version TLV
    print("Step 7: Router_1")
    pkts.filter_wpan_src64(ROUTER_1).\
        filter_LLARMA().\
        filter_mle_cmd(consts.MLE_PARENT_REQUEST).\
        filter(lambda p: {
                          consts.CHALLENGE_TLV,
                          consts.MODE_TLV,
                          consts.SCAN_MASK_TLV,
                          consts.VERSION_TLV
                          } <= set(p.mle.tlv.type)).\
        filter(lambda p: p.ipv6.hlim == 255).\
        must_next()

    # Step 8: Router_1
    # - Description: Automatically takes over leader role of a new Partition and begins transmitting MLE
    #   Advertisements.
    # - Pass Criteria:
    #   - Router_1 MUST send MLE Advertisements.
    #   - MLE Advertisements MUST be sent with an IP Hop Limit of 255, either to a Link-Local unicast address OR to
    #     the Link-Local All-Nodes multicast address (FF02::1).
    #   - The following TLVs MUST be present in the Advertisements:
    #     - Leader Data TLV (DUT MUST choose a new and random initial Partition ID, VN_Version, and VN_Stable_version.)
    #     - Route64 TLV (DUT MUST choose a new and random initial ID sequence number and delete all previous
    #       information from its routing tables.)
    #     - Source Address TLV
    print("Step 8: Router_1")
    pkts.filter_wpan_src64(ROUTER_1).\
        filter_mle_cmd(consts.MLE_ADVERTISEMENT).\
        filter(lambda p: {
                          consts.LEADER_DATA_TLV,
                          consts.ROUTE64_TLV,
                          consts.SOURCE_ADDRESS_TLV
                          } <= set(p.mle.tlv.type)).\
        filter(lambda p: p.ipv6.hlim == 255).\
        must_next()

    # Step 9: Leader
    # - Description: Automatically reattaches to network.
    # - Pass Criteria:
    #   - The Leader MUST send a properly formatted MLE Parent Request to the Link-Local All-Routers multicast
    #     address with an IP Hop Limit of 255.
    #   - The following TLVs MUST be present and valid in the Parent Request:
    #     - Challenge TLV
    #     - Mode TLV
    #     - Scan Mask TLV = 0x80 (active Routers) (If the DUT sends multiple Parent Requests)
    #     - Version TLV
    print("Step 9: Leader")
    pkts.copy().\
        filter_wpan_src64(LEADER).\
        filter_LLARMA().\
        filter_mle_cmd(consts.MLE_PARENT_REQUEST).\
        filter(lambda p: {
                          consts.CHALLENGE_TLV,
                          consts.MODE_TLV,
                          consts.SCAN_MASK_TLV,
                          consts.VERSION_TLV
                          } <= set(p.mle.tlv.type)).\
        filter(lambda p: p.ipv6.hlim == 255).\
        filter(lambda p: p.mle.tlv.scan_mask.r == 1).\
        filter(lambda p: p.mle.tlv.scan_mask.e == 0).\
        must_next()

    # Step 10: Router
    # - Description: Automatically sends MLE Parent Response.
    # - Pass Criteria:
    #   - Router MUST send an MLE Parent Response.
    #   - The following TLVs MUST be present in the MLE Parent Response:
    #     - Challenge TLV
    #     - Connectivity TLV
    #     - Leader Data TLV
    #     - Link-layer Frame Counter TLV
    #     - Link Margin TLV
    #     - Response TLV
    #     - Source Address TLV
    #     - Version TLV
    #     - MLE Frame Counter TLV (optional) (The MLE Frame Counter TLV MAY be omitted if the sender uses the same
    #       internal counter for both link-layer and MLE security)
    print("Step 10: Router")
    pkts.copy().\
        filter(lambda p: p.wpan.src64 and p.wpan.src64 in {ROUTER_1, ROUTER_2, ROUTER_3}).\
        filter_wpan_dst64(LEADER).\
        filter_mle_cmd(consts.MLE_PARENT_RESPONSE).\
        filter(lambda p: {
                          consts.CHALLENGE_TLV,
                          consts.CONNECTIVITY_TLV,
                          consts.LEADER_DATA_TLV,
                          consts.LINK_LAYER_FRAME_COUNTER_TLV,
                          consts.LINK_MARGIN_TLV,
                          consts.RESPONSE_TLV,
                          consts.SOURCE_ADDRESS_TLV,
                          consts.VERSION_TLV
                          } <= set(p.mle.tlv.type)).\
        must_next()

    # Step 11: Leader
    # - Description: Automatically sends MLE Child ID Request (to Router) and Address Solicit Request and rejoins
    #   network.
    # - Pass Criteria: The MLE Child ID Request and Address Solicit Request MUST be properly formatted (See 5.1.1
    #   Attaching for formatting).
    print("Step 11: Leader")
    # Child ID Request
    _pkt = pkts.copy().\
        filter_wpan_src64(LEADER).\
        filter(lambda p: p.wpan.dst64 and p.wpan.dst64 in {ROUTER_1, ROUTER_2, ROUTER_3}).\
        filter_mle_cmd(consts.MLE_CHILD_ID_REQUEST).\
        filter(lambda p: {
                          consts.LINK_LAYER_FRAME_COUNTER_TLV,
                          consts.MODE_TLV,
                          consts.RESPONSE_TLV,
                          consts.TIMEOUT_TLV,
                          consts.TLV_REQUEST_TLV,
                          consts.VERSION_TLV
                          } <= set(p.mle.tlv.type)).\
        must_next()
    PARENT = _pkt.wpan.dst64

    # Verify that the PARENT actually sent a Parent Response
    pkts.copy().\
        filter_wpan_src64(PARENT).\
        filter_wpan_dst64(LEADER).\
        filter_mle_cmd(consts.MLE_PARENT_RESPONSE).\
        must_next()

    # Address Solicit Request
    pkts.copy().\
        filter_wpan_src64(LEADER).\
        filter_coap_request(consts.ADDR_SOL_URI).\
        filter(lambda p: {
                          consts.NL_MAC_EXTENDED_ADDRESS_TLV,
                          consts.NL_STATUS_TLV
                          } <= set(p.coap.tlv.type)).\
        must_next()

    # Step 12: All
    # - Description: Harness verifies connectivity by sending an ICMPv6 Echo Request to the DUT mesh local address.
    # - Pass Criteria: DUT (Router or Leader) MUST respond with a ICMPv6 Echo Reply.
    print("Step 12: All")

    # Verify bidirectional connectivity
    nodes = ['LEADER', 'ROUTER_1', 'ROUTER_2', 'ROUTER_3']
    for sender in nodes:
        for receiver in nodes:
            if sender == receiver:
                continue
            _verify_ping(pkts, pv, sender, receiver)


if __name__ == '__main__':
    verify_utils.run_main(verify)
