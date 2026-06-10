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
    # 5.6.3 Network data propagation (BR registers after attach) - Leader as BR
    #
    # 5.6.3.1 Topology
    # - Leader is configured as Border Router.
    # - MED_1 is configured to require complete network data.
    # - SED_1 is configured to request only stable network data.
    #
    # 5.6.3.2 Purpose & Description
    # The purpose of this test case is to show that the DUT correctly sets the Network Data (stable/non-stable)
    #   propagated by the Leader and sends it properly to devices already attached to it.
    #
    # Spec Reference   | V1.1 Section | V1.3.0 Section
    # -----------------|--------------|---------------
    # NetData Propag   | 5.13 / 5.15  | 5.13 / 5.15

    pkts = pv.pkts
    pv.summary.show()

    LEADER = pv.vars['LEADER']
    DUT = pv.vars['DUT']
    MED_1 = pv.vars['MED_1']
    SED_1 = pv.vars['SED_1']

    # Step 1: All
    # - Description: Ensure the topology is formed correctly
    # - Pass Criteria: N/A
    print("Step 1: All")

    # We create a copy here to search for the Step 6 notification later,
    # as it may occur earlier than MED_1 update steps.
    pkts_from_step1 = pkts.copy()

    # Step 2: Leader
    # - Description: Harness configures the device as a Border Router with the following On-Mesh Prefix Set:
    #   - Prefix 1: P_Prefix=2001::/64 P_stable=1 P_on_mesh=1 P_preferred=1 P_slaac=1 P_default=1
    #   - Prefix 2: P_Prefix=2002::/64 P_stable=0 P_on_mesh=1 P_preferred=1 P_slaac=1 P_default=1
    #   - Automatically sends multicast MLE Data Response with the new information, including the Network Data TLV
    #     with the following fields:
    #     - Prefix 1 and 2 TLVs, each including:
    #       - 6LoWPAN ID sub-TLV
    #       - Border Router sub-TLV
    # - Pass Criteria: N/A
    print("Step 2: Leader")
    pkts.filter_wpan_src64(LEADER).\
      filter_LLANMA().\
      filter_mle_cmd(consts.MLE_DATA_RESPONSE).\
      filter(lambda p: {
        consts.SOURCE_ADDRESS_TLV,
        consts.LEADER_DATA_TLV,
        consts.NETWORK_DATA_TLV
      } <= set(p.mle.tlv.type)).\
      filter(lambda p: p.thread_nwd.tlv.prefix is not nullField and {
        Ipv6Addr("2001::"),
        Ipv6Addr("2002::")
      } <= set(p.thread_nwd.tlv.prefix)).\
      must_next()

    # Step 3: Router_1 (DUT)
    # - Description: Automatically multicasts the new network data to neighbors and rx-on-when-idle Children
    # - Pass Criteria: The DUT MUST multicast a MLE Data Response for each prefix sent by the Leader (Prefix 1 and
    #   Prefix 2)
    print("Step 3: Router_1 (DUT)")
    pkts.filter_wpan_src64(DUT).\
      filter_LLANMA().\
      filter_mle_cmd(consts.MLE_DATA_RESPONSE).\
      filter(lambda p: {
        consts.SOURCE_ADDRESS_TLV,
        consts.LEADER_DATA_TLV,
        consts.NETWORK_DATA_TLV
      } <= set(p.mle.tlv.type)).\
      filter(lambda p: p.thread_nwd.tlv.prefix is not nullField and {
        Ipv6Addr("2001::"),
        Ipv6Addr("2002::")
      } <= set(p.thread_nwd.tlv.prefix)).\
      must_next()

    # Step 4: MED_1
    # - Description: Automatically sends a MLE Child Update Request to the DUT, which includes the newly configured
    #   addresses in the Address Registration TLV
    # - Pass Criteria: N/A
    print("Step 4: MED_1")
    pkts.filter_wpan_src64(MED_1).\
      filter_wpan_dst64(DUT).\
      filter_mle_cmd(consts.MLE_CHILD_UPDATE_REQUEST).\
      filter(lambda p: consts.ADDRESS_REGISTRATION_TLV in p.mle.tlv.type).\
      must_next()

    # Step 5: Router_1 (DUT)
    # - Description: Automatically sends a MLE Child Update Response to MED_1
    # - Pass Criteria:
    #   - The DUT MUST send a unicast MLE Child Update Response to MED_1, which includes the following TLVs:
    #     - Source Address TLV
    #     - Leader Data TLV
    #     - Address Registration TLV
    #       - Echoes back the addresses the child has configured
    #     - Mode TLV
    print("Step 5: Router_1 (DUT)")
    pkts.filter_wpan_src64(DUT).\
      filter_wpan_dst64(MED_1).\
      filter_mle_cmd(consts.MLE_CHILD_UPDATE_RESPONSE).\
      filter(lambda p: {
        consts.SOURCE_ADDRESS_TLV,
        consts.LEADER_DATA_TLV,
        consts.ADDRESS_REGISTRATION_TLV,
        consts.MODE_TLV
      } <= set(p.mle.tlv.type)).\
      must_next()

    # Step 6: Router_1 (DUT)
    # - Description: Automatically sends notification of new stable network data to SED_1 via a unicast MLE Child
    #   Update Request or MLE Data Response
    # - Pass Criteria:
    #   - The DUT MUST send a unicast MLE Child Update Request or Data Response to SED_1, including the following TLVs:
    #     - Source Address TLV
    #     - Leader Data TLV
    #     - Network Data TLV
    #       - At least, the Prefix 1 TLV
    #       - The Prefix 2 TLV MUST NOT be included
    #       - The required prefix TLV MUST include the following:
    #         - P_border_router_16 <value = 0xFFFE>
    #     - Active Timestamp TLV
    print("Step 6: Router_1 (DUT)")
    # We search from the copy made after Step 1 for this notification
    pkts_notification = pkts_from_step1
    pkts_notification.filter_wpan_src64(DUT).\
      filter_wpan_dst64(SED_1).\
      filter(lambda p: p.mle.cmd in (consts.MLE_CHILD_UPDATE_REQUEST, consts.MLE_DATA_RESPONSE)).\
      filter(lambda p: {
        consts.SOURCE_ADDRESS_TLV,
        consts.LEADER_DATA_TLV,
        consts.NETWORK_DATA_TLV,
        consts.ACTIVE_TIMESTAMP_TLV
      } <= set(p.mle.tlv.type)).\
      filter(lambda p: p.thread_nwd.tlv.prefix is not nullField and Ipv6Addr("2001::") in p.thread_nwd.tlv.prefix).\
      filter(lambda p: p.thread_nwd.tlv.prefix is not nullField and Ipv6Addr("2002::") not in p.thread_nwd.tlv.prefix).\
      filter(lambda p: 0xfffe in p.thread_nwd.tlv.border_router_16).\
      must_next()

    # Step 7: SED_1
    # - Description: Automatically sends address configured in the Address Registration TLV to the DUT in a MLE Child
    #   Update Request command
    # - Pass Criteria: N/A
    print("Step 7: SED_1")
    pkts_notification.filter_wpan_src64(SED_1).\
      filter_wpan_dst64(DUT).\
      filter_mle_cmd(consts.MLE_CHILD_UPDATE_REQUEST).\
      filter(lambda p: consts.ADDRESS_REGISTRATION_TLV in p.mle.tlv.type).\
      must_next()

    # Step 8: Router_1 (DUT)
    # - Description: Automatically responds with MLE Child Update Response to SED_1
    # - Pass Criteria:
    #   - The DUT MUST send a MLE Child Update Response, which includes the following TLVs:
    #     - Source Address TLV
    #     - Leader Data TLV
    #     - Address Registration TLV
    #       - Echoes back the addresses the child has configured
    #     - Mode TLV
    print("Step 8: Router_1 (DUT)")
    pkts_notification.filter_wpan_src64(DUT).\
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
