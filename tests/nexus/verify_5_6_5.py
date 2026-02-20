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
    # 5.6.5 Network data updates – Router as BR
    #
    # 5.6.5.1 Topology
    # - Router_1 is configured as Border Router.
    # - MED_1 is configured to require complete network data.
    # - SED_1 is configured to request only stable network data.
    #
    # 5.6.5.2 Purpose & Description
    # The purpose of this test case is to verify that the DUT, as Leader, properly updates the network data - after
    #   receiving new information from the routers in the network containing three Prefix configurations - and
    #   disseminates it correctly throughout the network.
    #
    # Spec Reference                                     | V1.1 Section | V1.3.0 Section
    # ---------------------------------------------------|--------------|---------------
    # Thread Network Data / Network Data and Propagation | 5.13 / 5.15  | 5.13 / 5.15

    pkts = pv.pkts
    pv.summary.show()

    LEADER = pv.vars['LEADER']
    ROUTER_1 = pv.vars['ROUTER_1']
    MED_1 = pv.vars['MED_1']
    SED_1 = pv.vars['SED_1']

    # Step 1: All
    # - Description: Ensure the topology is formed correctly.
    # - Pass Criteria: N/A
    print("Step 1: Ensure the topology is formed correctly.")

    # Step 2: Router_1
    # - Description: Harness configures the device as a Border Router with the following On-Mesh Prefix Set:
    #   - Prefix 1: P_Prefix=2001::/64 P_stable=1 P_on_mesh=1 P_preferred=1 P_slaac=1 P_default=1
    #   - Prefix 2: P_Prefix=2002::/64 P_stable=0 P_on_mesh=1 P_preferred=1 P_slaac=1 P_default=1
    #   - Prefix 3: P_Prefix=2003::/64 P_stable=1 P_on_mesh=1 P_preferred=1 P_slaac=1 P_default=0
    #   - Automatically sends a CoAP Server Data Notification frame with the server’s information to the DUT:
    #     - CoAP Request URI: coap://[<DUT address>]:MM/a/sd
    #     - CoAP Payload: Thread Network Data TLV
    # - Pass Criteria: N/A
    print("Step 2: Router_1 sends a CoAP Server Data Notification frame to the DUT.")
    pkts.filter_wpan_src64(ROUTER_1).\
        filter_coap_request(consts.SVR_DATA_URI).\
        must_next()

    # Step 3: Leader (DUT)
    # - Description: Automatically sends a CoAP Response to Router_1.
    # - Pass Criteria: The DUT MUST transmit a 2.04 Changed CoAP response to Router_1.
    print("Step 3: Leader (DUT) transmits a 2.04 Changed CoAP response to Router_1.")
    pkts.filter_wpan_src64(LEADER).\
        filter_coap_ack(consts.SVR_DATA_URI).\
        must_next()

    # Step 4: Leader (DUT)
    # - Description: Automatically multicasts the new network information to neighbors and rx-on-when-idle Children.
    # - Pass Criteria: The DUT MUST multicast a MLE Data Response with the new network information including:
    #   - At least the Prefix 1, 2 and 3 TLVs, each including:
    #     - 6LoWPAN ID sub-TLV
    #     - Border Router sub-TLV
    #   - Leader Data TLV
    #     - Data Version field <incremented>
    #     - Stable Data Version field <incremented>
    print("Step 4: Leader (DUT) multicasts MLE Data Response with the new network information.")
    pkts.copy().\
        filter_LLANMA().\
        filter_mle_cmd(consts.MLE_DATA_RESPONSE).\
        filter(lambda p: {
            Ipv6Addr("2001::"),
            Ipv6Addr("2002::"),
            Ipv6Addr("2003::")
        } <= set(p.thread_nwd.tlv.prefix)).\
        must_next()

    # Step 5: Router_1
    # - Description: Automatically multicasts the MLE Data Response sent by the DUT.
    # - Pass Criteria: N/A
    print("Step 5: Router_1 multicasts the MLE Data Response sent by the DUT.")
    pkts.copy().\
        filter_wpan_src64(ROUTER_1).\
        filter_LLANMA().\
        filter_mle_cmd(consts.MLE_DATA_RESPONSE).\
        must_next()

    # Step 6: MED_1
    # - Description: Automatically sends address configured in the Address Registration TLV to the DUT in a MLE Child
    #   Update Request command.
    # - Pass Criteria: N/A
    print("Step 6: MED_1 sends MLE Child Update Request to the DUT.")
    pkts.copy().\
        filter_wpan_src64(MED_1).\
        filter_wpan_dst64(LEADER).\
        filter_mle_cmd(consts.MLE_CHILD_UPDATE_REQUEST).\
        filter(lambda p: consts.ADDRESS_REGISTRATION_TLV in p.mle.tlv.type).\
        must_next()

    # Step 7: Leader (DUT)
    # - Description: Automatically responds with MLE Child Update Response to MED_1.
    # - Pass Criteria: The DUT MUST send an MLE Child Update Response, which includes the following TLVs:
    #   - Source Address TLV
    #   - Leader Data TLV
    #   - Address Registration TLV - Echoes back the addresses the child has configured
    #   - Mode TLV
    print("Step 7: Leader (DUT) sends MLE Child Update Response to MED_1.")
    pkts.copy().\
        filter_wpan_src64(LEADER).\
        filter_wpan_dst64(MED_1).\
        filter_mle_cmd(consts.MLE_CHILD_UPDATE_RESPONSE).\
        filter(lambda p: {
            consts.SOURCE_ADDRESS_TLV,
            consts.LEADER_DATA_TLV,
            consts.ADDRESS_REGISTRATION_TLV,
            consts.MODE_TLV
        } <= set(p.mle.tlv.type)).\
        must_next()

    # Step 8: Leader (DUT)
    # - Description: Automatically sends notification of new stable network data to SED_1 via a unicast MLE Child
    #   Update Request or MLE Data Response.
    # - Pass Criteria: The DUT MUST send a unicast MLE Child Update Request or MLE Data Response to SED_1, including:
    #   - Source Address TLV
    #   - Leader Data TLV
    #     - Data Version field <incremented>
    #     - Stable Data Version field <incremented>
    #   - Network Data TLV
    #     - At least the Prefix 1 and 3 TLVs
    #       - Prefix 2 TLV MUST NOT be included
    #     - The required prefix TLVs MUST each include:
    #       - Border Router sub-TLV: P_border_router_16 <value = 0xFFFE>
    #   - Active Timestamp TLV
    print("Step 8: Leader (DUT) sends notification of new stable network data to SED_1.")
    pkts.copy().\
        filter_wpan_src64(LEADER).\
        filter_wpan_dst64(SED_1).\
        filter(lambda p: p.mle.cmd in {
            consts.MLE_CHILD_UPDATE_REQUEST,
            consts.MLE_DATA_RESPONSE
        }).\
        filter(lambda p: {
            consts.SOURCE_ADDRESS_TLV,
            consts.LEADER_DATA_TLV,
            consts.NETWORK_DATA_TLV,
            consts.ACTIVE_TIMESTAMP_TLV
        } <= set(p.mle.tlv.type)).\
        filter(lambda p: Ipv6Addr("2001::") in p.thread_nwd.tlv.prefix).\
        filter(lambda p: Ipv6Addr("2003::") in p.thread_nwd.tlv.prefix).\
        filter(lambda p: Ipv6Addr("2002::") not in p.thread_nwd.tlv.prefix).\
        filter(lambda p: all(rloc == 0xfffe for rloc in p.thread_nwd.tlv.border_router_16)).\
        must_next()

    # Step 9: SED_1
    # - Description: Automatically sends address configured in the Address Registration TLV to the DUT in a MLE Child
    #   Update Request command.
    # - Pass Criteria: N/A
    print("Step 9: SED_1 sends MLE Child Update Request to the DUT.")
    pkts.copy().\
        filter_wpan_src64(SED_1).\
        filter_wpan_dst64(LEADER).\
        filter_mle_cmd(consts.MLE_CHILD_UPDATE_REQUEST).\
        filter(lambda p: consts.ADDRESS_REGISTRATION_TLV in p.mle.tlv.type).\
        must_next()

    # Step 10: Leader (DUT)
    # - Description: Automatically responds with MLE Child Update Response to SED_1.
    # - Pass Criteria: The DUT MUST send an MLE Child Update Response, which includes the following TLVs:
    #   - Source Address TLV
    #   - Leader Data TLV
    #   - Address Registration TLV - Echoes back the addresses the child has configured
    #   - Mode TLV
    print("Step 10: Leader (DUT) sends MLE Child Update Response to SED_1.")
    pkts.copy().\
        filter_wpan_src64(LEADER).\
        filter_wpan_dst64(SED_1).\
        filter_mle_cmd(consts.MLE_CHILD_UPDATE_RESPONSE).\
        filter(lambda p: {
            consts.SOURCE_ADDRESS_TLV,
            consts.LEADER_DATA_TLV,
            consts.ADDRESS_REGISTRATION_TLV,
            consts.MODE_TLV
        } <= set(p.mle.tlv.type)).\
        must_next()


if __name__ == '__main__':
    verify_utils.run_main(verify)
