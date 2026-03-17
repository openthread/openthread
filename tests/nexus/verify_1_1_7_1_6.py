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

PREFIX = Ipv6Addr('2001:db8:1::')


def verify(pv):
    # 7.1.6 Network data propagation when BR Leaves the network, rejoins and updates server data
    #
    # 7.1.6.1 Topology
    # - Router_1 is configured as Border Router for prefix 2001:db8:1::/64.
    # - Router_2 is configured as Border Router for prefix 2001:db8:1::/64.
    # - MED_1 is configured to require complete network data.
    # - SED_1 is configured to request only stable network data.
    #
    # 7.1.6.2 Purpose & Description
    # The purpose of this test case is to verify that network data is properly updated when a server from the network
    #   leaves and rejoins.
    #
    # Spec Reference   | V1.1 Section | V1.3.0 Section
    # -----------------|--------------|---------------
    # Server Behavior  | 5.15.6       | 5.15.6

    pkts = pv.pkts
    pv.summary.show()

    DUT = pv.vars['DUT']
    ROUTER_1 = pv.vars['ROUTER_1']
    ROUTER_2 = pv.vars['ROUTER_2']
    MED_1 = pv.vars['MED_1']
    SED_1 = pv.vars['SED_1']

    # Step 1: All
    # - Description: Ensure topology is formed correctly.
    # - Pass Criteria: N/A.
    print("Step 1: Ensure topology is formed correctly.")
    pv.verify_attached('ROUTER_1', 'DUT')
    pv.verify_attached('ROUTER_2', 'DUT')
    pv.verify_attached('MED_1', 'DUT', 'MTD')
    pv.verify_attached('SED_1', 'DUT', 'MTD')

    # Step 2: Router_1
    # - Description: Harness configures the device with the following On-Mesh Prefix Set:
    #   - Prefix 1: P_prefix = 2001:db8:1::/64 P_stable = 1 P_on_mesh = 1 P_slaac = 1 P_default = 1
    #   - Automatically sends a CoAP Server Data Notification message with the server’s information (Prefix, Border Router) to the Leader:
    #     - CoAP Request URI: coap://[<leader address>]:MM/a/sd
    #     - CoAP Payload: Thread Network Data TLV
    # - Pass Criteria: N/A.
    print("Step 2: Router_1 sends a CoAP Server Data Notification.")
    req1 = pkts.filter_wpan_src64(ROUTER_1).\
        filter_coap_request(consts.SVR_DATA_URI).\
        must_next()
    index_after_req1 = pkts.index

    # Step 3: Router_2
    # - Description: Harness configures the device with the following On-Mesh Prefix Set:
    #   - Prefix 1: P_Prefix = 2001:db8:1::/64 P_stable = 0 P_on_mesh = 1 P_slaac = 1 P_default = 1
    #   - Automatically sends a CoAP Server Data Notification message with the server’s information (Prefix, Border Router) to the Leader:
    #     - CoAP Request URI: coap://[<leader address>]:MM/a/sd
    #     - CoAP Payload: Thread Network Data TLV
    # - Pass Criteria: N/A.
    print("Step 3: Router_2 sends a CoAP Server Data Notification.")
    req2 = pkts.filter_wpan_src64(ROUTER_2).\
        filter_coap_request(consts.SVR_DATA_URI).\
        must_next()
    index_after_req2 = pkts.index

    # Step 4: Leader (DUT)
    # - Description: Automatically sends a CoAP ACK frame to each of Router_1 and Router_2.
    # - Pass Criteria:
    #   - The DUT MUST send a CoAP ACK frame (2.04 Changed) to Router_1.
    #   - The DUT MUST send a CoAP ACK frame (2.04 Changed) to Router_2.
    print("Step 4: DUT MUST send a CoAP ACK frame (2.04 Changed) to Router_1 and Router_2.")

    # We find the ACKs which might be out of order with respect to Step 3's request.
    pkts.index = index_after_req1
    pkts.filter_ipv6_src(str(req1.ipv6.dst)).\
        filter_ipv6_dst(str(req1.ipv6.src)).\
        filter(lambda p: p.coap.code == consts.COAP_CODE_ACK).\
        must_next()

    pkts.index = index_after_req2
    pkts.filter_ipv6_src(str(req2.ipv6.dst)).\
        filter_ipv6_dst(str(req2.ipv6.src)).\
        filter(lambda p: p.coap.code == consts.COAP_CODE_ACK).\
        must_next()

    # Step 5: Leader (DUT)
    # - Description: Automatically sends new network data to neighbors and rx-on-when idle Children (MED_1) via a multicast MLE Data Response to address FF02::1.
    # - Pass Criteria: The DUT MUST multicast MLE Data Response with the new information collected from Router_1 and Router_2, including the following TLVs:
    #   - Source Address TLV
    #   - Leader Data TLV
    #     - Data Version field <incremented>
    #     - Stable Data Version field <incremented>
    #   - Network Data TLV
    #     - At least one Prefix TLV (Prefix 1)
    #       - Two Border Router sub-TLVs
    #       - 6LoWPAN ID sub-TLV
    print("Step 5: DUT MUST multicast MLE Data Response with new information.")
    pkts.filter_LLANMA().\
        filter_mle_cmd(consts.MLE_DATA_RESPONSE).\
        filter(lambda p: {
                          consts.SOURCE_ADDRESS_TLV,
                          consts.LEADER_DATA_TLV,
                          consts.NETWORK_DATA_TLV
                          } <= set(p.mle.tlv.type) and\
               PREFIX in p.thread_nwd.tlv.prefix and\
               len(p.thread_nwd.tlv.border_router_16) == 2).\
        must_next()

    # Step 6: Leader (DUT)
    # - Description: Automatically sends notification of new network data to SED_1 via a unicast MLE Child Update Request or MLE Data Response.
    # - Pass Criteria: The DUT MUST send MLE Child Update Request or Data Response to SED_1, which contains the stable Network Data and includes the following TLVs:
    #   - Source Address TLV
    #   - Leader Data TLV
    #     - Data version numbers must be the same as the ones sent in the multicast data response in step 5
    #   - Network Data TLV
    #     - At least one TLV (Prefix 1) TLV, including:
    #       - Border Router sub-TLV (corresponding to Router_1)
    #       - 6LoWPAN ID sub-TLV
    #       - P_border_router_16 <0xFFFE>
    #   - Active Timestamp TLV
    print("Step 6: DUT MUST send MLE Child Update Request, Response or Data Response to SED_1.")
    pkts.filter_wpan_dst64(SED_1).\
        filter(lambda p: p.mle.cmd in (consts.MLE_CHILD_UPDATE_REQUEST,
                                      consts.MLE_CHILD_UPDATE_RESPONSE,
                                      consts.MLE_DATA_RESPONSE)).\
        filter(lambda p: {
                          consts.SOURCE_ADDRESS_TLV,
                          consts.LEADER_DATA_TLV,
                          consts.NETWORK_DATA_TLV,
                          consts.ACTIVE_TIMESTAMP_TLV
                          } <= set(p.mle.tlv.type)).\
        must_next()

    # Step 7: Router_1
    # - Description: Harness silently powers-off Router_1 and waits 720 seconds to allow Leader (DUT) to detect the change.
    # - Pass Criteria: N/A.
    print("Step 7: Router_1 is powered off.")

    # Step 8: Leader (DUT)
    # - Description: Automatically detects removal of Router_1 and updates the Router ID Set accordingly.
    # - Pass Criteria:
    #   - The DUT MUST detect that Router_1 is removed from the network and update the Router ID Set.
    #   - The DUT MUST remove the Network Data section corresponding to Router_1 and increment the Data Version and Stable Data Version.
    print("Step 8: DUT MUST detect removal of Router_1 and update network data.")

    # Step 9: Leader (DUT)
    # - Description: Automatically sends new updated network data to neighbors and rx-on-when idle Children (MED_1).
    # - Pass Criteria: The DUT MUST send MLE Data Response to the Link-Local All Nodes multicast address (FF02::1), including the following TLVs:
    #   - Source Address TLV
    #   - Leader Data TLV
    #     - Data version field <incremented>
    #     - Stable Version field <incremented>
    #   - Network Data TLV
    #     - Router_1’s Network Data section MUST be removed
    print("Step 9: DUT MUST multicast MLE Data Response with Router_1 data removed.")
    pkts.filter_LLANMA().\
        filter_mle_cmd(consts.MLE_DATA_RESPONSE).\
        filter(lambda p: {
                          consts.SOURCE_ADDRESS_TLV,
                          consts.LEADER_DATA_TLV,
                          consts.NETWORK_DATA_TLV
                          } <= set(p.mle.tlv.type) and\
               PREFIX in p.thread_nwd.tlv.prefix and\
               len(p.thread_nwd.tlv.border_router_16) == 1).\
        must_next()

    # Step 10: Leader (DUT)
    # - Description: Automatically sends notification of new network data to SED_1 via a unicast MLE Child Update Request or MLE Data Response.
    # - Pass Criteria: The DUT MUST unicast MLE Child Update Request or Data Response to SED_1, containing the updated Network Data:
    #   - Source Address TLV
    #   - Network Data TLV
    #   - Active Timestamp TLV
    print("Step 10: DUT MUST unicast MLE Child Update Request, Response or Data Response to SED_1.")
    pkts.filter_wpan_dst64(SED_1).\
        filter(lambda p: p.mle.cmd in (consts.MLE_CHILD_UPDATE_REQUEST,
                                      consts.MLE_CHILD_UPDATE_RESPONSE,
                                      consts.MLE_DATA_RESPONSE)).\
        filter(lambda p: {
                          consts.SOURCE_ADDRESS_TLV,
                          consts.LEADER_DATA_TLV,
                          consts.NETWORK_DATA_TLV,
                          consts.ACTIVE_TIMESTAMP_TLV
                          } <= set(p.mle.tlv.type)).\
        must_next()

    # Step 11: Router_1
    # - Description: Harness silently powers-up Router_1; it automatically begins the attach procedure.
    # - Pass Criteria: N/A.
    print("Step 11: Router_1 is powered up and begins attach procedure.")

    # Step 12: Leader (DUT)
    # - Description: Automatically attaches Router_1 as a Child.
    # - Pass Criteria: The DUT MUST send MLE Child ID Response to Router_1, which includes the following TLVs:
    #   - Source Address TLV
    #   - Leader Data TLV
    #   - Address16 TLV
    #   - Route64 TLV
    #   - Network Data TLV
    #     - At least one Prefix TLV (Prefix 1) including:
    #       - Border Router sub-TLV corresponding to Router_2
    #       - 6LoWPAN ID sub-TLV
    print("Step 12: DUT MUST send MLE Child ID Response to Router_1.")
    pkts.filter_wpan_dst64(ROUTER_1).\
        filter_mle_cmd(consts.MLE_CHILD_ID_RESPONSE).\
        filter(lambda p: {
                          consts.SOURCE_ADDRESS_TLV,
                          consts.LEADER_DATA_TLV,
                          consts.ADDRESS16_TLV,
                          consts.ROUTE64_TLV,
                          consts.NETWORK_DATA_TLV
                          } <= set(p.mle.tlv.type)).\
        must_next()

    # Step 13: Router_1
    # - Description: Harness (re)configures the device with the following On-Mesh Prefix Set:
    #   - Prefix 1: P_prefix = 2001:db8:1::/64 P_stable = 1 P_on_mesh = 1 P_slaac = 1 P_default = 1
    #   - Automatically sends a CoAP Server Data Notification message with the server’s information (Prefix, Border Router) to the Leader:
    #     - CoAP Request URI: coap://[<leader address>]:MM/a/sd
    #     - CoAP Payload: Thread Network Data TLV
    # - Pass Criteria: N/A.
    print("Step 13: Router_1 sends a CoAP Server Data Notification.")
    req3 = pkts.filter_wpan_src64(ROUTER_1).\
        filter_coap_request(consts.SVR_DATA_URI).\
        must_next()
    index_after_req3 = pkts.index

    # Step 14: Leader (DUT)
    # - Description: Automatically sends a CoAP ACK frame to Router_1.
    # - Pass Criteria: The DUT MUST send a CoAP ACK frame (2.04 Changed) to Router_1.
    print("Step 14: DUT MUST send a CoAP ACK frame (2.04 Changed) to Router_1.")
    pkts.index = index_after_req3
    pkts.filter_ipv6_src(str(req3.ipv6.dst)).\
        filter_ipv6_dst(str(req3.ipv6.src)).\
        filter(lambda p: p.coap.code == consts.COAP_CODE_ACK).\
        must_next()

    # Step 15: Leader (DUT)
    # - Description: Automatically sends new updated network data to neighbors and rx-on-when idle Children (MED_1).
    # - Pass Criteria: The DUT MUST multicast a MLE Data Response with the new information collected from Router_1, including the following fields:
    #   - Source Address TLV
    #   - Leader Data TLV
    #     - Data version field <incremented>
    #     - Stable Version field <incremented>
    #   - Network Data TLV
    #     - At least one Prefix TLV (Prefix 1) including:
    #       - Two Border Router sub-TLVs – corresponding to Router_1 and Router_2
    #       - 6LoWPAN ID sub-TLV
    print("Step 15: DUT MUST multicast MLE Data Response with updated information.")
    pkts.filter_LLANMA().\
        filter_mle_cmd(consts.MLE_DATA_RESPONSE).\
        filter(lambda p: {
                          consts.SOURCE_ADDRESS_TLV,
                          consts.LEADER_DATA_TLV,
                          consts.NETWORK_DATA_TLV
                          } <= set(p.mle.tlv.type) and\
               PREFIX in p.thread_nwd.tlv.prefix and\
               len(p.thread_nwd.tlv.border_router_16) == 2).\
        must_next()

    # Step 16: Leader (DUT)
    # - Description: Automatically sends notification of new network data to SED_1 via a unicast MLE Child Update Request or MLE Data Response.
    # - Pass Criteria: The DUT MUST send a unicast MLE Child Update Request or Data Response to SED_1, containing the stable Network Data and including the following TLVs:
    #   - Source Address TLV
    #   - Leader Data TLV
    #     - Data version numbers must be the same as those sent in the multicast data response in step 15
    #   - Network Data TLV
    #     - At least one Prefix TLV (Prefix 1), including:
    #       - Border Router sub-TLV (corresponding to Router_1)
    #       - 6LoWPAN ID sub-TLV
    #       - P_border_router_16 <0xFFFE>
    #   - Active Timestamp TLV
    print("Step 16: DUT MUST send a unicast MLE Child Update Request, Response or Data Response to SED_1.")
    pkts.filter_wpan_dst64(SED_1).\
        filter(lambda p: p.mle.cmd in (consts.MLE_CHILD_UPDATE_REQUEST,
                                      consts.MLE_CHILD_UPDATE_RESPONSE,
                                      consts.MLE_DATA_RESPONSE)).\
        filter(lambda p: {
                          consts.SOURCE_ADDRESS_TLV,
                          consts.LEADER_DATA_TLV,
                          consts.NETWORK_DATA_TLV,
                          consts.ACTIVE_TIMESTAMP_TLV
                          } <= set(p.mle.tlv.type)).\
        must_next()

    # Step 17: Router_1, SED_1
    # - Description: Harness verifies connectivity by sending ICMPv6 Echo Requests from Router_1 and SED_1 to the DUT Prefix_1 based address.
    # - Pass Criteria: The DUT MUST respond with ICMPv6 Echo Replies.
    print("Step 17: ICMPv6 Echo Request/Reply.")

    # Verify pings from Router_1 and SED_1 to DUT
    pkts.filter_wpan_src64(ROUTER_1).\
        filter_ping_request(identifier=1).\
        must_next()
    pkts.filter_wpan_src64(DUT).\
        filter_ping_reply(identifier=1).\
        must_next()

    pkts.filter_wpan_src64(SED_1).\
        filter_ping_request(identifier=2).\
        must_next()
    pkts.filter_wpan_src64(DUT).\
        filter_ping_reply(identifier=2).\
        must_next()


if __name__ == '__main__':
    verify_utils.run_main(verify)
