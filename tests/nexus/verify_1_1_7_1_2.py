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
from pktverify.null_field import nullField


def verify(pv):
    # 7.1.2 Network data propagation - Border Router as Router in a Thread Network; correctly sets and propagates
    #   network data information during attach
    #
    # 7.1.2.1 Topology
    # - MED_1 is configured to require complete network data. (Mode TLV)
    # - SED_1 is configured to request only stable network data. (Mode TLV)
    #
    # 7.1.2.2 Purpose & Description
    # The purpose of this test case is to verify that when global prefix information is set on the DUT, it properly
    #   unicasts information to the Leader using CoAP (Server Data Notification). This test case also verifies that
    #   the DUT correctly sets Network Data aggregated and disseminated by the Leader (stable/non-stable) of the
    #   network and transmits it properly to devices during the attach procedure.
    #
    # Spec Reference                                                            | V1.1 Section       | V1.3.0 Section
    # --------------------------------------------------------------------------|--------------------|--------------------
    # Thread Network Data / Stable Thread Network Data / Network Data and Prop. | 5.13 / 5.14 / 5.15 | 5.13 / 5.14 / 5.15

    pkts = pv.pkts
    pv.summary.show()

    LEADER = pv.vars['LEADER']
    ROUTER_1 = pv.vars['ROUTER_1']
    MED_1 = pv.vars['MED_1']
    SED_1 = pv.vars['SED_1']

    # Step 1: Leader
    # - Description: Forms the network.
    # - Pass Criteria: N/A.
    print("Step 1: Leader")

    # Step 2: Router_1 (DUT)
    # - Description: User joins the DUT to the network and configures On-Mesh Prefix Set (it is also allowed to
    #   configure the On-Mesh Prefix Set on the DUT first and then attach DUT to Leader).
    #   - Prefix 1: P_prefix=2001::/64 P_stable=1 P_on_mesh=1 P_preferred=1 P_slaac=1 P_default=1.
    #   - Prefix 2: P_prefix=2002::/64 P_stable=0 P_on_mesh=1 P_preferred=1 P_slaac=1 P_default=1.
    # - Pass Criteria:
    #   - The DUT MUST send MLE Advertisements properly.
    #   - The DUT MUST send a CoAP Server Data Notification message with the server’s information (Prefix, Border
    #     Router) to the Leader:
    #     - CoAP Request URI: coap://[<leader address>]:MM/a/sd
    #     - CoAP Payload: Thread Network Data TLV
    print("Step 2: Router_1 (DUT)")
    pkts.filter_wpan_src64(ROUTER_1).\
        filter_LLANMA().\
        filter_mle_cmd(consts.MLE_ADVERTISEMENT).\
        must_next()

    pkts.filter_wpan_src64(ROUTER_1).\
        filter_coap_request(consts.SVR_DATA_URI).\
        filter(lambda p: {
                          consts.NL_THREAD_NETWORK_DATA_TLV
                         } <= set(p.coap.tlv.type)).\
        must_next()

    # Step 3: Leader
    # - Description: Automatically transmits a 2.04 Changed CoAP response to the DUT. Automatically transmits
    #   multicast MLE Data Response with the new information collected, adding also 6LoWPAN ID TLV for the prefix set
    #   on the DUT.
    # - Pass Criteria: N/A.
    print("Step 3: Leader")

    # Step 4: Router_1 (DUT)
    # - Description: Automatically broadcasts new network data.
    # - Pass Criteria: The DUT MUST multicast the MLE Data Response message sent by the Leader.
    print("Step 4: Router_1 (DUT)")
    pkts.filter_wpan_src64(ROUTER_1).\
        filter_LLANMA().\
        filter_mle_cmd(consts.MLE_DATA_RESPONSE).\
        filter(lambda p: {
                          consts.NETWORK_DATA_TLV
                         } <= set(p.mle.tlv.type)).\
        must_next()

    # Step 5: MED_1
    # - Description: Harness instructs MED_1 to attach to Router_1 (DUT) and request complete network data.
    # - Pass Criteria: N/A.
    print("Step 5: MED_1")

    # Step 6: Router_1 (DUT)
    # - Description: Automatically sends the new network data to MED_1.
    # - Pass Criteria:
    #   - The DUT MUST unicast MLE Child ID Response to MED_1, including the Network Data TLV with the following:
    #     - At least two Prefix TLVs (Prefix 1 and Prefix 2), each including:
    #       - 6LoWPAN ID sub-TLV
    #       - Border Router sub-TLV
    print("Step 6: Router_1 (DUT)")
    pkts.filter_wpan_src64(ROUTER_1).\
        filter_wpan_dst64(MED_1).\
        filter_mle_cmd(consts.MLE_CHILD_ID_RESPONSE).\
        filter(lambda p: {
                          consts.NETWORK_DATA_TLV
                         } <= set(p.mle.tlv.type) and\
                         '2001::' in p.thread_nwd.tlv.prefix and\
                         '2002::' in p.thread_nwd.tlv.prefix and\
                         p.thread_nwd.tlv.type.count(consts.NWD_PREFIX_TLV) >= 2 and\
                         p.thread_nwd.tlv.type.count(consts.NWD_BORDER_ROUTER_TLV) >= 2 and\
                         p.thread_nwd.tlv.type.count(consts.NWD_6LOWPAN_ID_TLV) >= 2).\
        must_next()

    # Step 7: SED_1
    # - Description: Harness instructs SED_1 to attach to Router_1 (DUT) and request only stable data.
    # - Pass Criteria: N/A.
    print("Step 7: SED_1")

    # Step 8: Router_1 (DUT)
    # - Description: Automatically sends the stable Network Data to SED_1.
    # - Pass Criteria:
    #   - The DUT MUST unicast MLE Child ID Response to SED_1, including the Network Data TLV (only stable Network
    #     Data) with the following:
    #     - At least one Prefix TLV (Prefix 1), including
    #       - Border Router TLV
    #         - P_border_router_16 <0xFFFE>
    #     - Prefix 2 TLV MUST NOT be included.
    print("Step 8: Router_1 (DUT)")
    pkts.filter_wpan_src64(ROUTER_1).\
        filter_wpan_dst64(SED_1).\
        filter_mle_cmd(consts.MLE_CHILD_ID_RESPONSE).\
        filter(lambda p: {
                          consts.NETWORK_DATA_TLV
                         } <= set(p.mle.tlv.type) and\
                         '2001::' in p.thread_nwd.tlv.prefix and\
                         '2002::' not in p.thread_nwd.tlv.prefix and\
                         p.thread_nwd.tlv.border_router_16 == [0xfffe]).\
        must_next()

    # Step 9: MED_1, SED_1
    # - Description: After attaching, each device automatically sends its configured global address to the DUT, in
    #   the Address Registration TLV via the MLE Child Update Request command.
    # - Pass Criteria: N/A.
    print("Step 9: MED_1, SED_1")

    # Step 10: Leader (DUT)
    # - Description: Automatically replies to each Child with a Child Update Response.
    # - Pass Criteria:
    #   - The DUT MUST unicast MLE Child Update Responses, each to MED_1 & SED_1, each including the following TLVs:
    #     - Address Registration TLV
    #       - Echoes back addresses configured by the Child in step 9
    #     - Mode TLV
    #     - Source Address TLV
    print("Step 10: Leader (DUT)")
    pkts.filter_wpan_src64(ROUTER_1).\
        filter_wpan_dst64(MED_1).\
        filter_mle_cmd(consts.MLE_CHILD_UPDATE_RESPONSE).\
        filter(lambda p: {
                          consts.ADDRESS_REGISTRATION_TLV,
                          consts.MODE_TLV,
                          consts.SOURCE_ADDRESS_TLV
                         } <= set(p.mle.tlv.type)).\
        must_next()

    pkts.filter_wpan_src64(ROUTER_1).\
        filter_wpan_dst64(SED_1).\
        filter_mle_cmd(consts.MLE_CHILD_UPDATE_RESPONSE).\
        filter(lambda p: {
                          consts.ADDRESS_REGISTRATION_TLV,
                          consts.MODE_TLV,
                          consts.SOURCE_ADDRESS_TLV
                         } <= set(p.mle.tlv.type)).\
        must_next()


if __name__ == '__main__':
    verify_utils.run_main(verify)
