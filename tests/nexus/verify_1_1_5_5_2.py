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
    # 5.5.2 Leader Reboot > timeout (3 nodes)
    #
    # 5.5.2.1 Topology
    # - Leader
    # - Router_1
    # - MED (attached to Router_1)
    #
    # 5.5.2.2 Purpose & Description
    # The purpose of this test case is to show that the Router will become the leader of a new partition when the
    #   Leader is restarted, and remains rebooted longer than the leader timeout, and when the Leader is brought back
    #   it reattaches to the Router.
    #
    # Spec Reference   | V1.1 Section | V1.3.0 Section
    # -----------------|--------------|---------------
    # Losing           | 5.16.1       | 5.16.1
    #   Connectivity   |              |

    pkts = pv.pkts
    pv.summary.show()

    LEADER = pv.vars['LEADER']
    ROUTER_1 = pv.vars['ROUTER_1']
    MED_1 = pv.vars['MED_1']

    # Step 1: All
    # - Description: Ensure topology is formed correctly.
    # - Pass Criteria: N/A
    print("Step 1: All")
    pkts.copy().\
        filter_mle_cmd(consts.MLE_ADVERTISEMENT).\
        filter_wpan_src64(LEADER).\
        must_next()
    pkts.copy().\
        filter_mle_cmd(consts.MLE_ADVERTISEMENT).\
        filter_wpan_src64(ROUTER_1).\
        must_next()
    pkts.copy().\
        filter_mle_cmd(consts.MLE_CHILD_ID_RESPONSE).\
        filter_wpan_src64(ROUTER_1).\
        filter_wpan_dst64(MED_1).\
        must_next()

    # Step 2: Leader, Router_1
    # - Description: Automatically transmit MLE advertisements.
    # - Pass Criteria:
    #   - The DUT MUST send properly formatted MLE Advertisements.
    #   - MLE Advertisements MUST be sent with an IP Hop Limit of 255 to the Link-Local All Nodes multicast address
    #     (FF02::1).
    #   - The following TLVs MUST be present in the Advertisements:
    #     - Leader Data TLV
    #     - Route64 TLV
    #     - Source Address TLV
    print("Step 2: Leader, Router_1")
    pkts.copy().\
        filter_mle_cmd(consts.MLE_ADVERTISEMENT).\
        filter_wpan_src64(LEADER).\
        filter_LLANMA().\
        filter(lambda p: {
                          consts.LEADER_DATA_TLV,
                          consts.ROUTE64_TLV,
                          consts.SOURCE_ADDRESS_TLV
                          } <= set(p.mle.tlv.type) and\
               p.ipv6.hlim == 255).\
        must_next()
    pkts.copy().\
        filter_mle_cmd(consts.MLE_ADVERTISEMENT).\
        filter_wpan_src64(ROUTER_1).\
        filter_LLANMA().\
        filter(lambda p: {
                          consts.LEADER_DATA_TLV,
                          consts.ROUTE64_TLV,
                          consts.SOURCE_ADDRESS_TLV
                          } <= set(p.mle.tlv.type) and\
               p.ipv6.hlim == 255).\
        must_next()

    # Step 3: Leader
    # - Description: Restart Leader.
    #   - If DUT=Leader and testing is manual, this is a UI pop-up box interaction.
    #   - Allowed Leader reboot time is 125 seconds (must be greater than Leader Timeout value [default 120
    #     seconds]).
    # - Pass Criteria:
    #   - For DUT = Leader: The DUT MUST stop sending MLE advertisements.
    print("Step 3: Leader")

    # Step 4: Router_1
    # - Description: Automatically attempts to reattach to partition.
    # - Pass Criteria:
    #   - For DUT = Router: The DUT MUST attempt to reattach to its original partition by sending MLE Parent Requests
    #     to the Link-Local All-Routers multicast address (FF02::2) with an IP Hop Limit of 255.
    #   - The following TLVs MUST be present in the MLE Parent Request:
    #     - Challenge TLV
    #     - Mode TLV
    #     - Scan Mask TLV (The E and R flags MUST be set)
    #     - Version TLV
    #   - The DUT MUST make two separate attempts to reconnect to its current Partition in this manner.
    print("Step 4: Router_1")
    parent_req_filter = pkts.copy().\
        filter_mle_cmd(consts.MLE_PARENT_REQUEST).\
        filter_wpan_src64(ROUTER_1).\
        filter_LLARMA().\
        filter(lambda p: {
                          consts.CHALLENGE_TLV,
                          consts.MODE_TLV,
                          consts.SCAN_MASK_TLV,
                          consts.VERSION_TLV
                          } <= set(p.mle.tlv.type) and\
               p.ipv6.hlim == 255 and\
               p.mle.tlv.scan_mask.r == 1 and\
               p.mle.tlv.scan_mask.e == 1)
    parent_req_filter.copy().must_next()
    parent_req_filter.copy().must_next()

    # Step 5: Leader
    # - Description: Does not respond to MLE Parent Requests.
    # - Pass Criteria:
    #   - For DUT = Leader: The DUT MUST NOT respond to the MLE Parent Requests.
    print("Step 5: Leader")

    # Step 6: Router_1
    # - Description: Automatically attempts to attach to any other Partition.
    # - Pass Criteria:
    #   - For DUT = Router: The DUT MUST attempt to attach to any other Partition within range by sending a MLE Parent
    #     Request.
    #   - The MLE Parent Request MUST be sent to the All-Routers multicast address (FF02::2) with an IP Hop Limit of
    #     255.
    #   - The following TLVs MUST be present and valid in the MLE Parent Request:
    #     - Challenge TLV
    #     - Mode TLV
    #     - Scan Mask TLV
    #     - Version TLV
    print("Step 6: Router_1")

    # Step 7: Router_1
    # - Description: Automatically takes over Leader role of a new Partition and begins transmitting MLE
    #   Advertisements.
    # - Pass Criteria:
    #   - For DUT = Router: The DUT MUST send MLE Advertisements.
    #   - MLE Advertisements MUST be sent with an IP Hop Limit of 255.
    #   - MLE Advertisements MUST be sent to a Link-Local unicast address OR to the Link-Local All Nodes multicast
    #     address (FF02::1).
    #   - The following TLVs MUST be present in the MLE Advertisement:
    #     - Leader Data TLV: The DUT MUST choose a new and random initial Partition ID, VN_Version, and
    #       VN_Stable_version.
    #     - Route64 TLV: The DUT MUST choose a new and random initial ID sequence number and delete all previous
    #       information from its routing table.
    #     - Source Address TLV
    print("Step 7: Router_1")
    pkts.copy().\
        filter_mle_cmd(consts.MLE_ADVERTISEMENT).\
        filter_wpan_src64(ROUTER_1).\
        filter(lambda p: {
                          consts.LEADER_DATA_TLV,
                          consts.ROUTE64_TLV,
                          consts.SOURCE_ADDRESS_TLV
                          } <= set(p.mle.tlv.type) and\
               p.ipv6.hlim == 255).\
        must_next()

    # Step 8: Router_1
    # - Description: The MED automatically sends MLE Child Update to Router_1. Router_1 automatically responds with
    #   MLE Child Update Response.
    # - Pass Criteria:
    #   - For DUT = Router: The DUT MUST respond with an MLE Child Update Response, with the updated TLVs of the new
    #     partition.
    #   - The following TLVs MUST be present in the MLE Child Update Response:
    #     - Leader Data TLV
    #     - Mode TLV
    #     - Source Address TLV
    #     - Address Registration TLV (optional)
    print("Step 8: Router_1")
    pkts.copy().\
        filter_mle_cmd(consts.MLE_CHILD_UPDATE_REQUEST).\
        filter_wpan_src64(MED_1).\
        filter_wpan_dst64(ROUTER_1).\
        must_next()
    pkts.copy().\
        filter_mle_cmd(consts.MLE_CHILD_UPDATE_RESPONSE).\
        filter_wpan_src64(ROUTER_1).\
        filter_wpan_dst64(MED_1).\
        filter(lambda p: {
                          consts.LEADER_DATA_TLV,
                          consts.MODE_TLV,
                          consts.SOURCE_ADDRESS_TLV
                          } <= set(p.mle.tlv.type)).\
        must_next()

    # Step 9: Leader
    # - Description: Automatically reattaches to network.
    # - Pass Criteria:
    #   - For DUT = Leader: The DUT MUST send properly formatted MLE Parent Requests to the All-Routers multicast
    #     address (FF02:2) with an IP Hop Limit of 255.
    #   - The following TLVs MUST be present and valid in the Parent Request:
    #     - Challenge TLV
    #     - Mode TLV
    #     - Scan Mask TLV (If the DUT sends multiple Parent Requests, the first one MUST be sent ONLY to All
    #       Routers; subsequent Parent Requests MAY be sent to All Routers and REEDS)
    #     - Version TLV
    #   - The Key Identifier Mode of the Security Control field of the MAC frame Auxiliary Security Header MUST be set
    #     to ‘0x02’.
    print("Step 9: Leader")
    pkts.copy().\
        filter_mle_cmd(consts.MLE_PARENT_REQUEST).\
        filter_wpan_src64(LEADER).\
        filter_LLARMA().\
        filter(lambda p: {
                          consts.CHALLENGE_TLV,
                          consts.MODE_TLV,
                          consts.SCAN_MASK_TLV,
                          consts.VERSION_TLV
                          } <= set(p.mle.tlv.type) and\
               p.ipv6.hlim == 255 and\
               p.wpan.aux_sec.key_id_mode == 0x02).\
        must_next()

    # Step 10: Router_1
    # - Description: Automatically sends MLE Parent Response.
    # - Pass Criteria:
    #   - For DUT = Router: The DUT MUST send an MLE Parent Response.
    #   - The following TLVs MUST be present in the MLE Parent Response:
    #     - Connectivity TLV
    #     - Challenge TLV
    #     - Leader Data TLV
    #     - Link-layer Frame Counter TLV
    #     - Link Margin TLV
    #     - Response TLV
    #     - Source Address TLV
    #     - Version TLV
    #     - MLE Frame Counter TLV (optional; MAY be omitted if the sender uses the same internal counter for both
    #       link-layer and MLE security)
    #   - The Key Identifier Mode of the Security Control field of the MAC frame Auxiliary Security Header MUST be set
    #     to ‘0x02’.
    print("Step 10: Router_1")
    pkts.copy().\
        filter_mle_cmd(consts.MLE_PARENT_RESPONSE).\
        filter_wpan_src64(ROUTER_1).\
        filter_wpan_dst64(LEADER).\
        filter(lambda p: {
                          consts.CONNECTIVITY_TLV,
                          consts.CHALLENGE_TLV,
                          consts.LEADER_DATA_TLV,
                          consts.LINK_LAYER_FRAME_COUNTER_TLV,
                          consts.LINK_MARGIN_TLV,
                          consts.RESPONSE_TLV,
                          consts.SOURCE_ADDRESS_TLV,
                          consts.VERSION_TLV
                          } <= set(p.mle.tlv.type) and\
               p.wpan.aux_sec.key_id_mode == 0x02).\
        must_next()

    # Step 11: Leader
    # - Description: Automatically sends MLE Child ID Request.
    # - Pass Criteria:
    #   - For DUT = Leader: The following TLVs MUST be present in the MLE Child ID Request:
    #     - Link-layer Frame Counter TLV
    #     - Mode TLV
    #     - Response TLV
    #     - Timeout TLV
    #     - TLV Request TLV: Address16 TLV, Network Data, and/or Route64 TLV (optional)
    #     - Version TLV
    #     - MLE Frame Counter TLV (optional; MAY be omitted if the sender uses the same internal counter for both
    #       link-layer and MLE security)
    #   - A REED MAY request a Route64 TLV as an aid in determining whether or not it should become an active
    #     Router.
    #   - The Key Identifier Mode of the Security Control field of the MAC frame Auxiliary Security Header MUST be set
    #     to ‘0x02’.
    print("Step 11: Leader")
    pkts.copy().\
        filter_mle_cmd(consts.MLE_CHILD_ID_REQUEST).\
        filter_wpan_src64(LEADER).\
        filter_wpan_dst64(ROUTER_1).\
        filter(lambda p: {
                          consts.LINK_LAYER_FRAME_COUNTER_TLV,
                          consts.MODE_TLV,
                          consts.RESPONSE_TLV,
                          consts.TIMEOUT_TLV,
                          consts.TLV_REQUEST_TLV,
                          consts.VERSION_TLV
                          } <= set(p.mle.tlv.type) and\
               p.wpan.aux_sec.key_id_mode == 0x02).\
        must_next()

    # Step 12: Router_1
    # - Description: Automatically sends MLE Child ID Response.
    # - Pass Criteria:
    #   - For DUT = Router: The following TLVs MUST be present in the MLE Child ID Response:
    #     - Address16 TLV
    #     - Leader Data TLV
    #     - Source Address TLV
    #     - Network Data TLV (provided if requested in MLE Child ID Request)
    #     - Route64 TLV (provided if requested in MLE Child ID Request)
    print("Step 12: Router_1")
    pkts.copy().\
        filter_mle_cmd(consts.MLE_CHILD_ID_RESPONSE).\
        filter_wpan_src64(ROUTER_1).\
        filter_wpan_dst64(LEADER).\
        filter(lambda p: {
                          consts.ADDRESS16_TLV,
                          consts.LEADER_DATA_TLV,
                          consts.SOURCE_ADDRESS_TLV
                          } <= set(p.mle.tlv.type)).\
        must_next()

    # Step 13: Leader
    # - Description: Automatically sends Address Solicit Request.
    # - Pass Criteria:
    #   - For DUT = Leader: The Address Solicit Request message MUST be properly formatted:
    #     - CoAP Request URI: coap://[<leader address>]:MM/a/as
    #     - CoAP Payload:
    #       - MAC Extended Address TLV
    #       - RLOC16 TLV (optional)
    #       - Status TLV
    print("Step 13: Leader")
    pkts.copy().\
        filter_wpan_src64(LEADER).\
        filter_coap_request(consts.ADDR_SOL_URI).\
        filter(lambda p: {
                          consts.NL_MAC_EXTENDED_ADDRESS_TLV,
                          consts.NL_STATUS_TLV
                          } <= set(p.coap.tlv.type)).\
        must_next()

    # Step 14: Router_1
    # - Description: Automatically sends Address Solicit Response.
    # - Pass Criteria:
    #   - For DUT = Router: The Address Solicit Response message MUST be properly formatted:
    #     - CoAP Response Code: 2.04 Changed
    #     - CoAP Payload:
    #       - Status TLV (value = Success)
    #       - RLOC16 TLV
    #       - Router Mask TLV
    print("Step 14: Router_1")
    pkts.copy().\
        filter_wpan_src64(ROUTER_1).\
        filter_coap_ack(consts.ADDR_SOL_URI).\
        filter(lambda p: {
                          consts.NL_STATUS_TLV,
                          consts.NL_RLOC16_TLV,
                          consts.NL_ROUTER_MASK_TLV
                          } <= set(p.coap.tlv.type)).\
        must_next()

    # Step 15: Leader
    # - Description: Optionally sends a multicast Link Request.
    # - Pass Criteria:
    #   - For DUT = Leader: The DUT MAY send a multicast Link Request message.
    #   - If sent, the following TLVs MUST be present in the Link Request Message:
    #     - Challenge TLV
    #     - Leader Data TLV
    #     - Request TLV: RSSI
    #     - Source Address TLV
    #     - Version TLV
    print("Step 15: Leader")

    # Step 16: Router_1
    # - Description: Conditionally (automatically) sends a unicast Link Accept.
    # - Pass Criteria:
    #   - For DUT = Router: If the Leader in the prior step sent a multicast Link Request, the DUT MUST send a unicast
    #     Link Accept Message to the Leader.
    #   - If sent, the following TLVs MUST be present in the Link Accept message:
    #     - Leader Data TLV
    #     - Link-layer Frame Counter TLV
    #     - Link Margin TLV
    #     - Response TLV
    #     - Source Address TLV
    #     - Version TLV
    #     - Challenge TLV (optional)
    #     - MLE Frame Counter TLV (optional)
    print("Step 16: Router_1")

    # Step 17: All
    # - Description: Verify connectivity by sending ICMPv6 Echo Request to the Router_1 link local address.
    # - Pass Criteria:
    #   - For DUT = Router: The DUT MUST respond with an ICMPv6 Echo Reply.
    print("Step 17: All")
    pkts.copy().\
        filter_ipv6_dst(pkts.copy().\
                        filter_mle_cmd(consts.MLE_ADVERTISEMENT).\
                        filter_wpan_src64(ROUTER_1).\
                        must_next().ipv6.src).\
        filter(lambda p: p.icmpv6.type == consts.ICMPV6_TYPE_ECHO_REQUEST).\
        must_next()
    pkts.copy().\
        filter_wpan_src64(ROUTER_1).\
        filter(lambda p: p.icmpv6.type == consts.ICMPV6_TYPE_ECHO_REPLY).\
        must_next()


if __name__ == '__main__':
    verify_utils.run_main(verify)
