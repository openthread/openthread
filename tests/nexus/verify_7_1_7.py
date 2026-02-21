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
    # 7.1.7 Network data updates – BR device rejoins network
    #
    # 7.1.7.1 Topology
    # - Router_1 is configured as Border Router for prefix 2001:db8:1::/64.
    # - Router_2 is configured as Border Router for prefix 2001:db8:1::/64.
    # - MED_1 is configured to require complete network data.
    # - SED_1 is configured to request only stable network data.
    # - Set on Router_2 device:
    #   - NETWORK_ID_TIMEOUT = 50s.
    #   - generated Partition ID to min.
    #
    # 7.1.7.2 Purpose & Description
    # The purpose of this test case is to verify that network data is properly updated when a server from the network
    #   leaves and rejoins.
    #
    # Spec Reference   | V1.1 Section | V1.3.0 Section
    # -----------------|--------------|---------------
    # Server Behavior  | 5.15.6       | 5.15.6

    pkts = pv.pkts
    pv.summary.show()

    LEADER = pv.vars['LEADER']
    ROUTER_1 = pv.vars['ROUTER_1']
    ROUTER_2 = pv.vars['ROUTER_2']
    MED_1 = pv.vars['MED_1']
    SED_1 = pv.vars['SED_1']

    PREFIX_1 = Ipv6Addr("2001:db8:1::")
    PREFIX_2 = Ipv6Addr("2001:db8:2::")
    LEADER_ALOC = pv.vars['LEADER_ALOC']

    # Step 1: All
    # - Description: Ensure topology is formed correctly.
    # - Pass Criteria: N/A.
    print("Step 1: Ensure topology is formed correctly.")

    # Step 2: Router_1
    # - Description: Harness configures the device with the following On-Mesh Prefix Set: Prefix 1: P_prefix =
    #   2001:db8:1::/64 P_stable=1 P_on_mesh=1 P_slaac=1 P_default=1. Automatically sends a CoAP Server Data Notification
    #   message with the server’s information (Prefix, Border Router) to the Leader:
    #   - CoAP Request URI: coap://[<leader address>]:MM/a/sd.
    #   - CoAP Payload: Thread Network Data TLV.
    # - Pass Criteria: N/A.
    print("Step 2: Router_1 sends a CoAP Server Data Notification message.")
    pkts.filter_coap_request(consts.SVR_DATA_URI).\
      must_next()

    # Step 3: Router_2
    # - Description: Harness configures the device with the following On-Mesh Prefix Set: Prefix 1: P_prefix =
    #   2001:db8:1::/64 P_stable=0 P_on_mesh=1 P_slaac=1 P_default=1. Automatically sends a CoAP Server Data Notification
    #   message with the server’s information (Prefix, Border Router) to the Leader:
    #   - CoAP Request URI: coap://[<leader address>]:MM/a/sd.
    #   - CoAP Payload: Thread Network Data TLV.
    # - Pass Criteria: N/A.
    print("Step 3: Router_2 sends a CoAP Server Data Notification message.")
    pkts.filter_coap_request(consts.SVR_DATA_URI).\
      must_next()

    # Step 4: Leader (DUT)
    # - Description: Automatically sends a CoAP ACK frame to each of Router_1 and Router_2.
    # - Pass Criteria:
    #   - The DUT MUST send a CoAP ACK frame (2.04 Changed) to Router_1.
    #   - The DUT MUST send a CoAP ACK frame (2.04 Changed) to Router_2.
    print("Step 4: Leader (DUT) sends a CoAP ACK frame to Router_1 and Router_2.")
    pkts.filter_coap_ack(consts.SVR_DATA_URI).\
      must_next()
    pkts.filter_coap_ack(consts.SVR_DATA_URI).\
      must_next()

    # Step 5: Leader (DUT)
    # - Description: Automatically sends new network data to neighbors and rx-on-when idle Children (MED_1).
    # - Pass Criteria: The DUT MUST multicast a MLE Data Response with the new information collected from Router_1 and
    #   Router_2, including the following TLVs:
    #   - Source Address TLV.
    #   - Leader Data TLV.
    #     - Data Version field <incremented>.
    #     - Stable Data Version field <incremented>.
    #   - Network Data TLV.
    #     - At least one Prefix TLV (Prefix 1).
    #       - Stable Flag set.
    #       - Two Border Router sub-TLVs.
    #         - Border Router1 TLV: Stable Flag set.
    #         - Border Router2 TLV : Stable Flag not set.
    #       - 6LoWPAN ID sub-TLV.
    #       - Stable Flag set.
    print("Step 5: Leader (DUT) multicasts a MLE Data Response with new network data.")
    pkts.filter_LLANMA().\
      filter_mle_cmd(consts.MLE_DATA_RESPONSE).\
      filter(lambda p: {
        consts.SOURCE_ADDRESS_TLV,
        consts.LEADER_DATA_TLV,
        consts.NETWORK_DATA_TLV
      } <= set(p.mle.tlv.type)).\
      filter(lambda p: PREFIX_1 in set(p.thread_nwd.tlv.prefix)).\
      must_next()

    # Step 6: Leader (DUT)
    # - Description: Automatically sends notification of new network data to SED_1 via a unicast MLE Child Update Request
    #   or MLE Data Response.
    # - Pass Criteria: The DUT MUST send a unicast MLE Child Update Request or MLE Data Response to SED_1, containing the
    #   stable Network Data and including the following TLVs:
    #   - Source Address TLV.
    #   - Leader Data TLV.
    #   - Network Data TLV.
    #     - At least one Prefix TLV (Prefix 1), including:
    #       - Stable Flag set.
    #       - Border Router sub-TLV (corresponding to Router_1).
    #         - Stable flag set.
    #         - P_border_router_16 <0xFFFE>.
    #       - 6LoWPAN ID sub-TLV.
    #       - Stable flag set.
    #   - Active Timestamp TLV.
    print("Step 6: Leader (DUT) sends notification of new network data to SED_1.")
    with pkts.save_index():
        pkts.filter_wpan_dst64(SED_1).\
          filter_mle_cmd2(consts.MLE_CHILD_UPDATE_REQUEST, consts.MLE_DATA_RESPONSE).\
          filter(lambda p: {
            consts.SOURCE_ADDRESS_TLV,
            consts.LEADER_DATA_TLV,
            consts.NETWORK_DATA_TLV,
            consts.ACTIVE_TIMESTAMP_TLV
          } <= set(p.mle.tlv.type)).\
          filter(lambda p: PREFIX_1 in set(p.thread_nwd.tlv.prefix)).\
          must_next()

    # Step 7: Router_2
    # - Description: Harness removes connectivity between Router_2 and the Leader (DUT), and waits ~50s.
    # - Pass Criteria: N/A.
    print("Step 7: Harness removes connectivity between Router_2 and the Leader (DUT).")

    # Step 8: Router_2
    # - Description: After Router_2 starts its own partition, the harness modifies Router_2’s network data information:
    #   - Removes the 2001:db8:1::/64 prefix.
    #   - Adds the 2001:db8:2::/64 prefix.
    #   - Prefix 2: P_prefix = 2001:db8:2::/64 P_stable=1 P_on_mesh=1 P_slaac=1 P_default=1.
    # - Pass Criteria: N/A.
    print("Step 8: Router_2 starts its own partition and modifies its network data.")

    # Step 9: Router_2
    # - Description: Harness enables connectivity between Router_2 and the Leader (DUT).
    # - Pass Criteria: N/A.
    print("Step 9: Harness enables connectivity between Router_2 and the Leader (DUT).")

    # Step 10: Router_2
    # - Description: Automatically reattaches to the Leader and sends a CoAP Server Data Notification message with the
    #   server’s information (Prefix, Border Router) to the Leader:
    #   - CoAP Request URI: coap://[<leader address>]:MM/a/sd.
    #   - CoAP Payload: Thread Network Data TLV.
    # - Pass Criteria: N/A.
    print("Step 10: Router_2 reattaches and sends a CoAP Server Data Notification.")
    pkts.filter_coap_request(consts.SVR_DATA_URI).\
      must_next()

    # Step 11: Leader (DUT)
    # - Description: Automatically sends a CoAP ACK frame to Router_2.
    # - Pass Criteria: The DUT MUST send a CoAP ACK frame (2.04 Changed) to Router_2.
    print("Step 11: Leader (DUT) sends a CoAP ACK frame to Router_2.")
    pkts.filter_coap_ack(consts.SVR_DATA_URI).\
      must_next()

    # Step 12: Leader (DUT)
    # - Description: Automatically sends new updated network data to neighbors and rx-on-when idle Children (MED_1).
    # - Pass Criteria: The DUT MUST multicast a MLE Data Response with the new information collected from Router_2,
    #   including the following TLVs:
    #   - Source Address TLV.
    #   - Leader Data TLV.
    #     - Data Version field <incremented>.
    #     - Stable Data Version field <incremented>.
    #   - Network Data TLV.
    #     - At least two Prefix TLVs (Prefix 1 and Prefix 2).
    #     - Prefix 1 TLV.
    #       - Stable Flag set.
    #       - Only one Border Router sub-TLV – corresponding to Router_1.
    #       - 6LoWPAN ID sub-TLV.
    #       - Stable Flag set.
    #     - Prefix 2 TLV.
    #       - Stable Flag set.
    #       - One Border Router sub-TLV – corresponding to Router_2.
    #       - 6LoWPAN ID sub-TLV.
    #       - Stable Flag set.
    print("Step 12: Leader (DUT) multicasts updated network data.")
    pkts.filter_LLANMA().\
      filter_mle_cmd(consts.MLE_DATA_RESPONSE).\
      filter(lambda p: {PREFIX_1, PREFIX_2} <= set(p.thread_nwd.tlv.prefix)).\
      must_next()

    # Step 13: Leader (DUT)
    # - Description: Automatically sends notification of new network data to SED_1 via a unicast MLE Child Update Request
    #   or MLE Data Response.
    # - Pass Criteria: The DUT MUST send a unicast MLE Child Update Request or MLE Data Response to SED_1, containing the
    #   stable Network Data and including the following TLVs:
    #   - Source Address TLV.
    #   - Leader Data TLV.
    #   - Network Data TLV.
    #     - At least two Prefix TLVs (Prefix 1 and Prefix 2).
    #     - Prefix 1 TLV.
    #       - Stable Flag set.
    #       - Border Router sub-TLV – corresponding to Router_1.
    #         - P_border_router_16 <0xFFFE>.
    #         - Stable flag set.
    #       - 6LoWPAN ID sub-TLV.
    #       - Stable Flag set.
    #     - Prefix 2 TLV.
    #       - Stable Flag set.
    #       - Border Router sub-TLV – corresponding to Router_2.
    #         - P_border_router_16 <0xFFFE>.
    #         - Stable flag set.
    #       - 6LoWPAN ID sub-TLV.
    #       - Stable Flag set.
    #   - Active Timestamp TLV.
    print("Step 13: Leader (DUT) sends notification of new network data to SED_1.")
    with pkts.save_index():
        pkts.filter_wpan_dst64(SED_1).\
          filter_mle_cmd2(consts.MLE_CHILD_UPDATE_REQUEST, consts.MLE_DATA_RESPONSE).\
          filter(lambda p: {PREFIX_1, PREFIX_2} <= set(p.thread_nwd.tlv.prefix)).\
          must_next()

    # Step 14: Router_1, SED_1
    # - Description: Harness verifies connectivity by sending ICMPv6 Echo Requests from Router_1 and SED_1 to the DUT
    #   Prefix_1 and Prefix_2-based addresses.
    # - Pass Criteria: The DUT MUST respond with ICMPv6 Echo Replies.
    print("Step 14: Verify connectivity via ICMPv6 Echo Requests/Replies.")
    with pkts.save_index():
        for src_node_name, prefix in [('ROUTER_1', PREFIX_1), ('ROUTER_1', PREFIX_2), ('SED_1', PREFIX_1),
                                      ('SED_1', PREFIX_2)]:
            rloc16 = pv.vars[src_node_name + '_RLOC16']
            p = pkts.filter_ping_request().\
                filter(lambda p: p.wpan.src16 == rloc16).\
                filter(lambda p: p.ipv6.dst.startswith(prefix[:8]) or
                                 p.ipv6.dst.startswith(bytes(8))).\
                must_next()
            # The 'startswith(bytes(8))' check above handles cases where the 6LoWPAN dissector cannot fully decompress
            # the IPv6 address (e.g., missing context), resulting in an address starting with null bytes.
            pkts.filter_ping_reply(identifier=p.icmpv6.echo.identifier).\
                filter(lambda p: p.wpan.src16 == pv.vars['LEADER_RLOC16']).\
                must_next()

    # Step 15: Router_2
    # - Description: Harness removes the 2001:db8:2::/64 address from Router_2. Router_2 sends a CoAP Server Data
    #   Notification (SVR_DATA.ntf) with empty server data payload to the Leader:
    #   - CoAP Request URI: coap://[<leader RLOC or ALOC>]:MM/a/sd.
    #   - CoAP Payload: zero-length Thread Network Data TLV.
    # - Pass Criteria: N/A.
    print("Step 15: Router_2 sends a CoAP Server Data Notification with empty payload.")
    pkts.filter_coap_request(consts.SVR_DATA_URI).\
      must_next()

    # Step 16: Leader (DUT)
    # - Description: Automatically sends a CoAP Response to Router_2.
    # - Pass Criteria: The DUT MUST send a CoAP response (2.04 Changed) to Router_2.
    print("Step 16: Leader (DUT) sends a CoAP Response to Router_2.")
    pkts.filter_coap_ack(consts.SVR_DATA_URI).\
      must_next()

    # Step 17: Leader (DUT)
    # - Description: Automatically sends new updated network data to neighbors and rx-on-when idle Children (MED_1).
    # - Pass Criteria: The DUT MUST multicast a MLE Data Response with the new information collected from Router_2,
    #   including the following TLVs:
    #   - Source Address TLV.
    #   - Leader Data TLV.
    #     - Data Version field <incremented>.
    #     - Stable Data Version field <incremented>.
    #   - Network Data TLV.
    #     - At least two Prefix TLVs (Prefix 1 and Prefix 2).
    #     - Prefix 1 TLV.
    #       - Stable Flag set.
    #       - Only one Border Router sub-TLV – corresponding to Router_1.
    #       - 6LoWPAN ID sub-TLV.
    #       - Stable Flag set.
    #     - Prefix 2 TLV.
    #       - Stable Flag set.
    #       - 6LoWPAN ID sub-TLV.
    #       - Stable Flag set.
    #       - compression flag set to 0.
    print("Step 17: Leader (DUT) multicasts updated network data.")
    pkts.filter_LLANMA().\
      filter_mle_cmd(consts.MLE_DATA_RESPONSE).\
      filter(lambda p: {PREFIX_1, PREFIX_2} <= set(p.thread_nwd.tlv.prefix)).\
      must_next()

    # Step 18: Leader (DUT)
    # - Description: Automatically sends notification of new network data to SED_1 via a unicast MLE Child Update Request
    #   or MLE Data Response.
    # - Pass Criteria: The DUT MUST send a unicast MLE Child Update Request or MLE Data Response to SED_1, containing the
    #   stable Network Data and including the following TLVs:
    #   - Source Address TLV.
    #   - Leader Data TLV.
    #   - Network Data TLV.
    #     - At least two Prefix TLVs (Prefix 1 and Prefix 2).
    #     - Prefix 1 TLV.
    #       - Stable Flag set.
    #       - Border Router sub-TLV – corresponding to Router_1.
    #         - P_border_router_16 <0xFFFE>.
    #         - Stable flag set.
    #       - 6LoWPAN ID sub-TLV.
    #       - Stable Flag set.
    #     - Prefix 2 TLV.
    #       - Stable Flag set.
    #       - 6LoWPAN ID sub-TLV.
    #       - Stable Flag set.
    #       - Compression flag set to 0.
    #   - Active Timestamp TLV.
    print("Step 18: Leader (DUT) sends notification of new network data to SED_1.")
    with pkts.save_index():
        pkts.filter_wpan_dst64(SED_1).\
          filter_mle_cmd2(consts.MLE_CHILD_UPDATE_REQUEST, consts.MLE_DATA_RESPONSE).\
          filter(lambda p: {PREFIX_1, PREFIX_2} <= set(p.thread_nwd.tlv.prefix)).\
          must_next()

    # Step 19: End of test
    # - Description: End of test.
    # - Pass Criteria: N/A.
    print("Step 19: End of test.")


if __name__ == '__main__':
    verify_utils.run_main(verify)
