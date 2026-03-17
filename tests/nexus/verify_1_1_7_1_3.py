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
    # 7.1.3 Network data propagation - Border Router as Leader of Thread network; advertises new network data
    #   information after network is formed
    #
    # 7.1.3.1 Topology
    # - MED_1 is configured to require complete network data. (Mode TLV)
    # - SED_1 is configured to request only stable network data. (Mode TLV)
    #
    # 7.1.3.2 Purpose & Description
    # The purpose of this test case is to verify that global prefix information can be set on the DUT, which is acting
    #   as a Leader in the Thread network. The DUT must also demonstrate that it correctly sets the Network Data
    #   (stable/non-stable) and propagates it properly in an already formed network.
    #
    # Spec Reference                                     | V1.1 Section       | V1.3.0 Section
    # ---------------------------------------------------|--------------------|--------------------
    # Thread Network Data / Stable Thread Network Data / | 5.13 / 5.14 / 5.15 | 5.13 / 5.14 / 5.15
    #   Network Data and Propagation                     |                    |

    pkts = pv.pkts
    pv.summary.show()

    LEADER = pv.vars['LEADER']
    MED_1 = pv.vars['MED_1']
    SED_1 = pv.vars['SED_1']

    PREFIX_1 = Ipv6Addr("2001::")
    PREFIX_2 = Ipv6Addr("2002::")

    # Step 1: All
    # - Description: Ensure topology is formed correctly.
    # - Pass Criteria: N/A
    print("Step 1: Ensure topology is formed correctly.")

    # Step 2: Leader (DUT)
    # - Description: User configures the DUT with the following On-Mesh Prefix Set:
    #   - Prefix 1: P_prefix=2001::/64 P_stable=1 P_on_mesh=1 P_preferred=1 P_slaac=1 P_default=1
    #   - Prefix 2: P_prefix=2002::/64 P_stable=0 P_on_mesh=1 P_preferred=1 P_slaac=1 P_default=1
    # - Pass Criteria: N/A
    print("Step 2: Leader (DUT) configures On-Mesh Prefixes.")

    # Step 3: Leader (DUT)
    # - Description: Automatically sends the new network data to neighbors and rx-on-while-idle Children (MED_1).
    # - Pass Criteria: The DUT MUST send a multicast MLE Data Response with the new network information, which includes
    #   the following TLVs:
    #   - Network Data TLV
    #     - At least two Prefix TLVs (Prefix 1 and Prefix 2):
    #       - 6LoWPAN ID sub-TLV
    #       - Border Router sub-TLV
    print("Step 3: Leader (DUT) automatically sends the new network data to neighbors and rx-on-while-idle Children.")
    pkts.filter_wpan_src64(LEADER).\
        filter_LLANMA().\
        filter_mle_cmd(consts.MLE_DATA_RESPONSE).\
        filter(lambda p: p.thread_nwd.tlv.prefix is not nullField and {
                          PREFIX_1,
                          PREFIX_2
                          } <= set(p.thread_nwd.tlv.prefix)).\
        must_next()

    # Step 4: MED_1
    # - Description: Automatically sends the global address configured to its parent (the DUT), via the Address
    #   Registration TLV included in its Child Update Request keep-alive message.
    # - Pass Criteria: N/A
    print("Step 4: MED_1 automatically sends the global address configured to its parent.")
    pkts.filter_wpan_src64(MED_1).\
        filter_wpan_dst64(LEADER).\
        filter_mle_cmd(consts.MLE_CHILD_UPDATE_REQUEST).\
        filter(lambda p: {
                          consts.ADDRESS_REGISTRATION_TLV
                          } <= set(p.mle.tlv.type)).\
        must_next()

    # Step 5: Leader (DUT)
    # - Description: Automatically sends MLE Child Update Response to MED_1.
    # - Pass Criteria: The DUT MUST unicast MLE Child Update Response to MED_1, containing the following TLVs:
    #   - Source Address TLV
    #   - Address Registration TLV (Echoes back the addresses MED_1 has configured)
    #   - Mode TLV
    print("Step 5: Leader (DUT) automatically sends MLE Child Update Response to MED_1.")
    pkts.filter_wpan_src64(LEADER).\
        filter_wpan_dst64(MED_1).\
        filter_mle_cmd(consts.MLE_CHILD_UPDATE_RESPONSE).\
        filter(lambda p: {
                          consts.SOURCE_ADDRESS_TLV,
                          consts.ADDRESS_REGISTRATION_TLV,
                          consts.MODE_TLV
                          } <= set(p.mle.tlv.type)).\
        must_next()

    # Step 6: Leader (DUT)
    # - Description: Automatically sends notification of new network data to SED_1. Depending upon the DUT's device
    #   implementation, two different behavior paths (A,B) are allowable.
    # - Pass Criteria:
    #   - Path A: The DUT MUST unicast MLE Child Update Request to SED_1, including the following TLVs:
    #     - Source Address TLV
    #     - Leader Data TLV
    #     - Network Data TLV
    #     - Active Timestamp TLV
    #     - Goto step 7
    #   - Path B: The DUT MUST unicast MLE Data Response to SED_1, including the following TLVs:
    #     - Source Address TLV
    #     - Leader Data TLV
    #     - Network Data TLV
    #     - Active Timestamp TLV
    #     - Goto step 7
    print("Step 6: Leader (DUT) automatically sends notification of new network data to SED_1.")
    pkts.filter_wpan_src64(LEADER).\
        filter_wpan_dst64(SED_1).\
        filter_mle_cmd2(consts.MLE_CHILD_UPDATE_REQUEST, consts.MLE_DATA_RESPONSE).\
        filter(lambda p: {
                          consts.SOURCE_ADDRESS_TLV,
                          consts.LEADER_DATA_TLV,
                          consts.NETWORK_DATA_TLV,
                          consts.ACTIVE_TIMESTAMP_TLV
                          } <= set(p.mle.tlv.type)).\
        filter(lambda p: p.thread_nwd.tlv.prefix is not nullField and
                         PREFIX_1 in p.thread_nwd.tlv.prefix and
                         PREFIX_2 not in p.thread_nwd.tlv.prefix).\
        must_next()

    # Step 7: SED_1
    # - Description: After receiving the MLE Data Response or MLE Child Update Request, automatically sends the global
    #   address configured to its parent (DUT), via the Address Registration TLV as part of the Child Update Request
    #   command.
    # - Pass Criteria: N/A
    print("Step 7: SED_1 automatically sends the global address configured to its parent.")
    pkts.filter_wpan_src64(SED_1).\
        filter_wpan_dst64(LEADER).\
        filter_mle_cmd(consts.MLE_CHILD_UPDATE_REQUEST).\
        filter(lambda p: {
                          consts.ADDRESS_REGISTRATION_TLV
                          } <= set(p.mle.tlv.type)).\
        must_next()

    # Step 8: Leader (DUT)
    # - Description: Automatically sends MLE Child Update Response to SED_1.
    # - Pass Criteria: The DUT MUST unicast MLE Child Update Response to SED_1, including the following TLVs:
    #   - Source Address TLV
    #   - Address Registration TLV (Echoes back the addresses SED_1 has configured)
    #   - Mode TLV
    print("Step 8: Leader (DUT) automatically sends MLE Child Update Response to SED_1.")
    pkts.filter_wpan_src64(LEADER).\
        filter_wpan_dst64(SED_1).\
        filter_mle_cmd(consts.MLE_CHILD_UPDATE_RESPONSE).\
        filter(lambda p: {
                          consts.SOURCE_ADDRESS_TLV,
                          consts.ADDRESS_REGISTRATION_TLV,
                          consts.MODE_TLV
                          } <= set(p.mle.tlv.type)).\
        must_next()


if __name__ == '__main__':
    verify_utils.run_main(verify)
