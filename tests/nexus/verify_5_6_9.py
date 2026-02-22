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
    # 5.6.9 Router Behavior - External Route
    #
    # 5.6.9.1 Topology
    #   Leader and Router_2 are configured as Border Routers.
    #
    # 5.6.9.2 Purpose & Description
    # The purpose of this test case is to verify that the DUT properly forwards data packets to a Border Router based
    #   on Network Data information.
    #
    # Spec Reference | V1.1 Section | V1.3.0 Section
    # ---------------|--------------|---------------
    # Server Behavior| 5.15.6       | 5.15.6

    pkts = pv.pkts
    pv.summary.show()

    LEADER = pv.vars['LEADER']
    LEADER_RLOC16 = pv.vars['LEADER_RLOC16']
    ROUTER_1 = pv.vars['ROUTER_1']
    ROUTER_1_RLOC16 = pv.vars['ROUTER_1_RLOC16']
    ROUTER_2 = pv.vars['ROUTER_2']
    ROUTER_2_RLOC16 = pv.vars['ROUTER_2_RLOC16']
    MED_1 = pv.vars['MED_1']
    SED_1 = pv.vars['SED_1']
    SED_1_RLOC16 = pv.vars['SED_1_RLOC16']

    PREFIX_1 = Ipv6Addr("2001::")
    PREFIX_2 = Ipv6Addr("2002::")

    # Step 1: All
    # - Description: Ensure topology is formed correctly.
    # - Pass Criteria: N/A.
    print("Step 1: Ensure topology is formed correctly.")

    # Step 2: Leader
    # - Description: Harness configures the device with the following On-Mesh Prefix Set:
    #   - Prefix 1: P_prefix=2001::/64 P_stable=1 P_on_mesh=1 P_preferred=0 (Medium) P_slaac_=1
    #     P_default = 1 (True).
    #   - Harness configures the device with the following External Route Set:
    #   - Prefix 2: R_prefix=2002::/64 R_stable=1 R_preference=0 (Medium).
    #   - The device automatically sends multicast MLE Data Response with the new information, including the
    #     Network Data TLV with the following TLVs:
    #     - Prefix 1 TLV, including: 6LoWPAN ID sub-TLV, Border Router sub-TLV.
    #     - Prefix 2 TLV, including: Has Route sub-TLV.
    # - Pass Criteria: N/A.
    print("Step 2: Leader configures Prefix 1 (On-Mesh) and Prefix 2 (External Route).")

    # Step 3: Router_2
    # - Description: Harness configures the device with the following On-Mesh Prefix Set:
    #   - Prefix 1: P_prefix=2001::/64 P_stable=1 P_on_mesh=1 P_preferred = 0 (Medium) P_slaac = 1
    #     P_default=0 (false).
    #   - Harness configures the device with the following External Route Set:
    #   - Prefix 2: R_prefix=2002::/64 R_stable=1 R_preference=1 (High).
    #   - The device automatically sends a CoAP Server Data Notification frame with the new server information
    #     (Prefix) to the Leader:
    #     - CoAP Request URI: coap://[<leader address>]:MM/a/sd
    #     - CoAP Payload: Thread Network Data TLV
    # - Pass Criteria: N/A.
    print("Step 3: Router_2 configures Prefix 1 (On-Mesh) and Prefix 2 (External Route High.")

    # Step 4: Router_1 (DUT)
    # - Description: Automatically multicasts the new network data to neighbors and rx-on-when-idle Children.
    # - Pass Criteria: The DUT MUST multicast a MLE Data Response containing the full Network Data, including: At
    #   least two Prefix TLVs (Prefix 1 & Prefix 2).
    print("Step 4: Router_1 (DUT) multicasts the new network data.")
    pkts.copy().\
        filter_wpan_src64(ROUTER_1).\
        filter_LLANMA().\
        filter_mle_cmd(consts.MLE_DATA_RESPONSE).\
        filter(lambda p: {\
                          PREFIX_1,\
                          PREFIX_2\
                          } <= set(p.thread_nwd.tlv.prefix)).\
        must_next()

    # Step 5: Router_1 (DUT)
    # - Description: Depending on the DUT’s implementation, automatically sends new stable network data to SED_1 via
    #   a unicast MLE Child Update Request or MLE Data Response.
    # - Pass Criteria: The DUT MUST send EITHER a unicast MLE Child Update Request OR a unicast MLE Data Response to
    #   SED_1, which includes the following TLVs:
    #   - Source Address TLV
    #   - Leader Data TLV
    #   - Network Data TLV:
    #     - At least 2 Prefix TLVs (Prefix 1 and Prefix 2)
    #     - Prefix 1 TLV MUST include: 6LoWPAN ID TLV, Border Router TLV: P_border_router_16 <value = 0xFFFE>
    #     - Prefix 2 TLV MUST include: 6LoWPAN ID TLV, Has Route TLV: P_border_router_16 <value = 0xFFFE>
    #   - Active Timestamp TLV
    print("Step 5: Router_1 (DUT) unicasts the new stable network data to SED_1.")
    pkts.copy().\
        filter_wpan_src64(ROUTER_1).\
        filter(lambda p: p.wpan.dst64 == SED_1 or p.wpan.dst16 == SED_1_RLOC16).\
        filter(lambda p: p.mle.cmd in (consts.MLE_CHILD_UPDATE_REQUEST, consts.MLE_DATA_RESPONSE)).\
        filter(lambda p: {\
                          consts.SOURCE_ADDRESS_TLV,\
                          consts.LEADER_DATA_TLV,\
                          consts.NETWORK_DATA_TLV,\
                          consts.ACTIVE_TIMESTAMP_TLV\
                          } <= set(p.mle.tlv.type)).\
        filter(lambda p: {\
                          PREFIX_1,\
                          PREFIX_2\
                          } <= set(p.thread_nwd.tlv.prefix)).\
        must_next()

    # Step 6: SED_1
    # - Description: Harness instructs the device to send an ICMPv6 Echo Request with Destination address of 2002::0
    #   (address not allocated inside the Thread network).
    # - Pass Criteria:
    #   - The DUT MUST add mesh header with: Destination: R2 RLOC16, Source: R1 RLOC16.
    #   - The DUT MUST forward the ICMPv6 Echo Request to Router_2 due to higher preference.
    print("Step 6: SED_1 sends an ICMPv6 Echo Request with Destination address of 2002::0.")
    pkts.copy().\
        filter_wpan_src16(ROUTER_1_RLOC16).\
        filter_wpan_dst16(LEADER_RLOC16).\
        filter_ping_request().\
        filter(lambda p: p.ipv6.dst == '2002::0').\
        filter(lambda p: p.lowpan.mesh.orig16 == ROUTER_1_RLOC16).\
        filter(lambda p: p.lowpan.mesh.dest16 == ROUTER_2_RLOC16).\
        must_next()

    # Step 7: MED_1
    # - Description: Harness instructs the device to send an ICMPv6 Echo Request with Destination address of 2007::0
    #   (address not allocated inside the Thread network).
    # - Pass Criteria: The DUT MUST forward the ICMPv6 Echo Request to the Leader due to default route.
    print("Step 7: MED_1 sends an ICMPv6 Echo Request with Destination address of 2007::0.")
    pkts.copy().\
        filter_wpan_src16(ROUTER_1_RLOC16).\
        filter_wpan_dst16(LEADER_RLOC16).\
        filter_ping_request().\
        filter(lambda p: p.ipv6.dst == '2007::0').\
        must_next()

    # Step 8: Router_2
    # - Description: Harness configures the device with the following updated On-Mesh Prefix Set:
    #   - Prefix 1: P_prefix=2001::/64 P_stable=1 P_on_mesh=1 P_preferred=1 (High) P_slaac=1 P_default = 1 (True).
    #   - The device automatically sends a CoAP Server Data Notification frame with the new server information
    #     (Prefix) to the Leader:
    #     - CoAP Request URI: coap://[<Leader address>]:MM/n/sd
    #     - CoAP Payload: Thread Network Data TLV
    # - Pass Criteria: N/A.
    print("Step 8: Router_2 updates Prefix 1 (On-Mesh High Default).")

    # Step 9: Router_1 (DUT)
    # - Description: Automatically multicasts the new network information to neighbors and rx-on-when-idle Children
    #   (MED_1).
    # - Pass Criteria: The DUT MUST multicast a MLE Data Response containing the full Network Data, including: At
    #   least two Prefix TLVs (Prefix 1 & Prefix 2).
    print("Step 9: Router_1 (DUT) multicasts the new network information.")
    pkts.copy().\
        filter_wpan_src64(ROUTER_1).\
        filter_LLANMA().\
        filter_mle_cmd(consts.MLE_DATA_RESPONSE).\
        filter(lambda p: {\
                          PREFIX_1,\
                          PREFIX_2\
                          } <= set(p.thread_nwd.tlv.prefix)).\
        must_next()

    # Step 10: Router_1 (DUT)
    # - Description: Depending on the the DUT’s implementation, automatically sends new stable network data to SED_1
    #   via a unicast MLE Child Update Request or MLE Data Response.
    # - Pass Criteria: The DUT MUST send EITHER a unicast MLE Child Update Request OR a unicast MLE Data Response to
    #   SED_1, which includes the following TLVs:
    #   - Source Address TLV
    #   - Leader Data TLV
    #   - Network Data TLV:
    #     - At least two Prefix TLVs (Prefix 1 and Prefix 2)
    #     - Prefix 1 TLV MUST include: 6LoWPAN ID TLV, Border Router TLV: P_border_router_16 <value = 0xFFFE>
    #     - Prefix 2 TLV MUST include: 6LoWPAN ID TLV, Has Route TLV: P_border_router_16 <value = 0xFFFE>
    #   - Active Timestamp TLV
    print("Step 10: Router_1 (DUT) unicasts the new stable network data to SED_1.")
    pkts.copy().\
        filter_wpan_src64(ROUTER_1).\
        filter(lambda p: p.wpan.dst64 == SED_1 or p.wpan.dst16 == SED_1_RLOC16).\
        filter(lambda p: p.mle.cmd in (consts.MLE_CHILD_UPDATE_REQUEST, consts.MLE_DATA_RESPONSE)).\
        filter(lambda p: {\
                          consts.SOURCE_ADDRESS_TLV,\
                          consts.LEADER_DATA_TLV,\
                          consts.NETWORK_DATA_TLV,\
                          consts.ACTIVE_TIMESTAMP_TLV\
                          } <= set(p.mle.tlv.type)).\
        filter(lambda p: {\
                          PREFIX_1,\
                          PREFIX_2\
                          } <= set(p.thread_nwd.tlv.prefix)).\
        must_next()

    # Step 11: SED_1
    # - Description: Harness instructs SED_1 to send an ICMPv6 Echo Request with Destination address of 2007::0
    #   (Address not allocated inside the Thread network).
    # - Pass Criteria:
    #   - The DUT MUST add mesh header with: Destination: Router_2 RLOC16, Source: Router_1 RLOC16.
    #   - The DUT MUST forward the ICMPv6 Echo Request to Router_2 due to default route with higher preference.
    print("Step 11: SED_1 sends an ICMPv6 Echo Request with Destination address of 2007::0.")
    pkts.copy().\
        filter_wpan_src16(ROUTER_1_RLOC16).\
        filter_wpan_dst16(LEADER_RLOC16).\
        filter_ping_request().\
        filter(lambda p: p.ipv6.dst == '2007::0').\
        filter(lambda p: p.lowpan.mesh.orig16 == ROUTER_1_RLOC16).\
        filter(lambda p: p.lowpan.mesh.dest16 == ROUTER_2_RLOC16).\
        must_next()

    # Step 12: Router_2
    # - Description: Harness configures the device with the following updated On-Mesh Prefix Set:
    #   - Prefix 1: P_prefix=2001::/64 P_stable=1 P_preference=0 (Medium) P_on_mesh=1 P_slaac=1 P_default = 1 (True).
    #   - The device automatically sends a CoAP Server Data Notification frame with the new server information
    #     (Prefix) to the Leader:
    #     - CoAP Request URI: coap://[<Leader address>]:MM/a/sd
    #     - CoAP Payload: Thread Network Data TLV
    # - Pass Criteria: N/A.
    print("Step 12: Router_2 updates Prefix 1 (On-Mesh Medium Default).")

    # Step 13: Router_1 (DUT)
    # - Description: Automatically multicasts the new network information to neighbors and rx-on-when-idle Children.
    # - Pass Criteria: The DUT MUST multicast a MLE Data Response, including: At least two Prefix TLVs (Prefix 1 &
    #   Prefix 2).
    print("Step 13: Router_1 (DUT) multicasts the new network information.")
    pkts.copy().\
        filter_wpan_src64(ROUTER_1).\
        filter_LLANMA().\
        filter_mle_cmd(consts.MLE_DATA_RESPONSE).\
        filter(lambda p: {\
                          PREFIX_1,\
                          PREFIX_2\
                          } <= set(p.thread_nwd.tlv.prefix)).\
        must_next()

    # Step 14: Router_1 (DUT)
    # - Description: Automatically unicasts the new network information to SED_1.
    # - Pass Criteria: Depending on its implementation, the DUT MUST send EITHER a unicast MLE Data Response OR a
    #   unicast MLE Child Update Request to SED_1, containing only stable Network Data, which includes:
    #   - At least two Prefix TLVs (Prefix 1 & Prefix 2)
    #   - Prefix 1 TLV MUST include: 6LoWPAN ID TLV, Border Router TLV: P_border_router_16 <value = 0xFFFE>
    #   - Prefix 2 TLV MUST include: 6LoWPAN ID TLV, Has Route TLV: P_border_router_16 <value = 0xFFFE>
    print("Step 14: Router_1 (DUT) unicasts the new network information to SED_1.")
    pkts.copy().\
        filter_wpan_src64(ROUTER_1).\
        filter(lambda p: p.wpan.dst64 == SED_1 or p.wpan.dst16 == SED_1_RLOC16).\
        filter(lambda p: p.mle.cmd in (consts.MLE_CHILD_UPDATE_REQUEST, consts.MLE_DATA_RESPONSE)).\
        filter(lambda p: {\
                          consts.SOURCE_ADDRESS_TLV,\
                          consts.LEADER_DATA_TLV,\
                          consts.NETWORK_DATA_TLV,\
                          consts.ACTIVE_TIMESTAMP_TLV\
                          } <= set(p.mle.tlv.type)).\
        filter(lambda p: {\
                          PREFIX_1,\
                          PREFIX_2\
                          } <= set(p.thread_nwd.tlv.prefix)).\
        must_next()

    # Step 15: SED_1
    # - Description: Harness instructs the device to send an ICMPv6 Echo Request with a Destination address of
    #   2007::0 (Address not allocated inside the Thread network).
    # - Pass Criteria: The DUT MUST forward the ICMPv6 Echo Request to Leader due to default route with lowest mesh
    #   path cost.
    print("Step 15: SED_1 sends an ICMPv6 Echo Request with Destination address of 2007::0.")
    pkts.copy().\
        filter_wpan_src16(ROUTER_1_RLOC16).\
        filter_wpan_dst16(LEADER_RLOC16).\
        filter_ping_request().\
        filter(lambda p: p.ipv6.dst == '2007::0').\
        must_next()


if __name__ == '__main__':
    verify_utils.run_main(verify)
