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
from pktverify.null_field import nullField


def verify(pv):
    # 5.6.4 Network data propagation (BR registers after attach) - Router as BR
    #
    # 5.6.4.1 Topology
    # - Router_1 is configured as Border Router.
    # - MED_1 is configured to require complete network data.
    # - SED_1 is configured to request only stable network data.
    #
    # 5.6.4.2 Purpose & Description
    # The purpose of this test case is to verify that the DUT, as Leader, collects network data information
    #   (stable/non-stable) from the network and propagates it properly in an already formed network.
    #   (2-hops away).
    #
    # Spec Reference                                     | V1.1 Section | V1.3.0 Section
    # ---------------------------------------------------|--------------|---------------
    # Thread Network Data / Network Data and Propagation | 5.13 / 5.15  | 5.13 / 5.15

    pkts = pv.pkts
    pv.summary.show()

    DUT = pv.vars['DUT']
    ROUTER_1 = pv.vars['ROUTER_1']
    MED_1 = pv.vars['MED_1']
    SED_1 = pv.vars['SED_1']

    DUT_RLOC16 = pv.vars['DUT_RLOC16']
    ROUTER_1_RLOC16 = pv.vars['ROUTER_1_RLOC16']
    MED_1_RLOC16 = pv.vars['MED_1_RLOC16']
    SED_1_RLOC16 = pv.vars['SED_1_RLOC16']

    # Step 1: All
    # - Description: Ensure the topology is formed correctly.
    # - Pass Criteria: N/A
    print("Step 1: All")

    # Step 2: Router_1
    # - Description: Harness configures the device as a Border Router with the following On-Mesh Prefix Set:
    #   - Prefix 1: P_Prefix=2001::/64 P_stable=1 P_on_mesh=1 P_preferred=1 P_slaac=1 P_default=1
    #   - Prefix 2: P_Prefix=2002::/64 P_stable=0 P_on_mesh=1 P_preferred=1 P_slaac=1 P_default=1
    #   - Automatically sends a CoAP Sever Data Notification frame with the server’s information to the Leader (DUT):
    #     - CoAP Request URI: coap://[<DUT address>]:MM/a/sd
    #     - CoAP Payload: Thread Network Data TLV
    # - Pass Criteria: N/A
    print("Step 2: Router_1")
    pkts.filter_wpan_src16(ROUTER_1_RLOC16).\
      filter_wpan_dst16(DUT_RLOC16).\
      filter_coap_request(consts.SVR_DATA_URI).\
      must_next()

    # Step 3: Leader (DUT)
    # - Description: Automatically sends a CoAP Response frame to Router_1.
    # - Pass Criteria: The DUT MUST transmit a 2.04 Changed CoAP response to Router_1.
    print("Step 3: Leader (DUT)")
    pkts.filter_wpan_src16(DUT_RLOC16).\
      filter_wpan_dst16(ROUTER_1_RLOC16).\
      filter_coap_ack(consts.SVR_DATA_URI).\
      filter(lambda p: p.coap.code == consts.COAP_CODE_ACK).\
      must_next()

    # Step 4: Leader (DUT)
    # - Description: Automatically multicasts the new network data to neighbors and rx-on-when-idle Children.
    # - Pass Criteria: The DUT MUST send a multicast MLE Data Response with the new network information collected
    #   from Router_1, including:
    #   - At least two Prefix TLVs (Prefix 1 and Prefix 2), each including:
    #     - 6LoWPAN ID sub-TLV
    #     - Border Router sub-TLV
    print("Step 4: Leader (DUT)")
    pkts.filter_mle_cmd(consts.MLE_DATA_RESPONSE).\
      filter(lambda p: {
        consts.NETWORK_DATA_TLV
      } <= set(p.mle.tlv.type)).\
      filter(lambda p: {
        Ipv6Addr("2001::"),
        Ipv6Addr("2002::")
      } <= set(p.thread_nwd.tlv.prefix)).\
      must_next()

    # Step 5: Router_1
    # - Description: Automatically sets Network Data after receiving multicast MLE Data Response sent by the DUT.
    # - Pass Criteria: N/A
    print("Step 5: Router_1")

    step_4_idx = pkts.index

    # Step 6: MED_1
    # - Description: Automatically sends address configured in the Address Registration TLV to the DUT in a MLE
    #   Child Update Request command.
    # - Pass Criteria: N/A
    print("Step 6: MED_1")
    pkts.filter_mle_cmd(consts.MLE_CHILD_UPDATE_REQUEST).\
      filter(lambda p: p.wpan.src64 == MED_1).\
      must_next()

    # Step 7: Leader (DUT)
    # - Description: Automatically responds to MED_1 with MLE Child Update Response.
    # - Pass Criteria: The DUT MUST send an MLE Child Update Response, which includes the following TLVs:
    #   - Source Address TLV
    #   - Leader Data TLV
    #   - Address Registration TLV
    #     - Echoes back the addresses the child has configured
    #   - Mode TLV
    print("Step 7: Leader (DUT)")
    pkts.filter_mle_cmd(consts.MLE_CHILD_UPDATE_RESPONSE).\
      filter(lambda p: p.wpan.dst64 == MED_1).\
      filter(lambda p: {
        consts.SOURCE_ADDRESS_TLV,
        consts.LEADER_DATA_TLV,
        consts.ADDRESS_REGISTRATION_TLV,
        consts.MODE_TLV
      } <= set(p.mle.tlv.type)).\
      must_next()

    # Step 8: Leader (DUT)
    # - Description: Depending upon the DUT’s device implementation, two different behavior paths (A,B) are
    #   allowable for transmitting the new stable network data to SED_1.
    print("Step 8: Leader (DUT)")

    pkts.index = step_4_idx

    # Step 9: Leader (DUT)
    # - Description: Automatically sends notification of new stable network data to SED_1.
    # - Pass Criteria: The DUT MUST send a unicast MLE Child Update Request or MLE Data Response to SED_1,
    #   which includes the following TLVs:
    #   - Source Address TLV
    #   - Leader Data TLV
    #   - Network Data TLV
    #     - At least one Prefix TLV (Prefix 1 TLV)
    #     - The Prefix 2 TLV MUST NOT be included
    #   - Active Timestamp TLV
    print("Step 9: Leader (DUT)")
    pkts.filter(lambda p: p.mle.cmd is not nullField).\
      filter(lambda p: p.mle.cmd in {consts.MLE_CHILD_UPDATE_REQUEST, consts.MLE_DATA_RESPONSE}).\
      filter(lambda p: p.wpan.src64 == DUT).\
      filter(lambda p: p.wpan.dst64 == SED_1).\
      filter(lambda p: {
        consts.SOURCE_ADDRESS_TLV,
        consts.LEADER_DATA_TLV,
        consts.NETWORK_DATA_TLV
      } <= set(p.mle.tlv.type)).\
      filter(lambda p: Ipv6Addr("2001::") in p.thread_nwd.tlv.prefix).\
      filter(lambda p: Ipv6Addr("2002::") not in p.thread_nwd.tlv.prefix).\
      must_next()

    # Step 10: SED_1
    # - Description: Automatically sends address configured in the Address Registration TLV to the DUT in a MLE
    #   Child Update Request command.
    # - Pass Criteria: N/A
    print("Step 10: SED_1")
    pkts.filter_mle_cmd(consts.MLE_CHILD_UPDATE_REQUEST).\
      filter(lambda p: p.wpan.src64 == SED_1).\
      must_next()

    # Step 11: Leader (DUT)
    # - Description: Automatically responds with MLE Child Update Response to SED_1.
    # - Pass Criteria: The DUT MUST send an MLE Child Update Response, which includes the follwing TLVs:
    #   - Address Registration TLV - Echoes back the addresses the child has configured
    #   - Leader Data TLV
    #   - Mode TLV
    #   - Source Address TLV
    print("Step 11: Leader (DUT)")
    pkts.filter_mle_cmd(consts.MLE_CHILD_UPDATE_RESPONSE).\
      filter(lambda p: p.wpan.src64 == DUT).\
      filter(lambda p: p.wpan.dst64 == SED_1).\
      filter(lambda p: {
        consts.ADDRESS_REGISTRATION_TLV,
        consts.LEADER_DATA_TLV,
        consts.MODE_TLV,
        consts.SOURCE_ADDRESS_TLV
      } <= set(p.mle.tlv.type)).\
      must_next()


if __name__ == '__main__':
    verify_utils.run_main(verify)
