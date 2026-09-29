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


def Verify(pv):
    # 6.3.2 Network Data Update
    #
    # 6.3.2.1 Topology
    # - Topology A: DUT as Minimal End Device (MED_1) attached to Leader. (RF Isolation required)
    # - Topology B: DUT as Sleepy End Device (SED_1) attached to Leader. (No RF Isolation required)
    # - Leader: Configured as Border Router.
    #
    # 6.3.2.2 Purpose & Description
    # - For a MED (Minimal End Device) DUT: This test case verifies that the DUT identifies it has an old version of the
    #   Network Data and then requests an update from its parent. This scenario requires short-term RF isolation for
    #   one device.
    # - For a SED (Sleepy End Device) DUT: This test case verifies that the DUT will receive new Network Data and
    #   respond with a MLE Child Update Request. This scenario does not require RF isolation.
    #
    # Spec Reference      | V1.1 Section | V1.3.0 Section
    # --------------------|--------------|---------------
    # Thread Network Data | 5.13         | 5.13

    pkts = pv.pkts
    pv.summary.show()

    leader = pv.vars['LEADER']

    if 'MED_1' in pv.vars:
        dut = pv.vars['MED_1']
        name = 'MED_1'
    elif 'SED_1' in pv.vars:
        dut = pv.vars['SED_1']
        name = 'SED_1'
    else:
        raise ValueError("Neither MED_1 nor SED_1 found in topology vars")

    # Step 1: All
    # - Description: Ensure topology is formed correctly.
    # - Pass Criteria: N/A
    print("Step 1: Ensure topology is formed correctly.")

    # Step 2: Leader
    # - Description: Harness updates the Network Data by configuring the device with the following Prefix Set:
    #   - Prefix 1: P_prefix=2001::/64 P_stable=1 P_on_mesh=1 P_preferred=1 P_slaac=1 P_default=1
    #   - Leader automatically sends new network information to the Child (DUT) using the appropriate method:
    #     - For DUT = MED: The Leader multicasts a MLE Data Response.
    #     - For DUT = SED: Depending on its own implementation, the Leader automatically sends new network data to the
    #       DUT using EITHER a MLE Data Response OR a MLE Child Update Request.
    # - Pass Criteria: N/A
    print(f"Step 2: Leader updates the Network Data and automatically sends new information to the {name} (DUT).")
    pkts.filter_wpan_src64(leader).\
        filter(lambda p: \
               (p.mle.cmd == consts.MLE_DATA_RESPONSE) or \
               (p.mle.cmd == consts.MLE_CHILD_UPDATE_REQUEST)).\
        must_next()

    # Step 3: MED_1 / SED_1 (DUT)
    # - Description: Automatically sends MLE Child Update Request to the Leader.
    # - Pass Criteria:
    #   - The DUT MUST send a MLE Child Update Request to the Leader, which includes the following TLVs:
    #     - Leader Data TLV
    #     - Address Registration TLV (contains the global address configured by DUT device based on advertised prefix
    #       2001:: by checking the CID and ML-EID)
    #     - Mode TLV
    #     - Timeout TLV
    print(f"Step 3: {name} (DUT) automatically sends MLE Child Update Request to the Leader.")
    pkts.filter_wpan_src64(dut).\
        filter_wpan_dst64(leader).\
        filter_mle_cmd(consts.MLE_CHILD_UPDATE_REQUEST).\
        filter(lambda p: \
               {
                   consts.LEADER_DATA_TLV,
                   consts.ADDRESS_REGISTRATION_TLV,
                   consts.MODE_TLV,
                   consts.TIMEOUT_TLV
               } <= set(p.mle.tlv.type)).\
        filter(lambda p: any(iid == int(addr[8:].hex(), 16) \
                             for iid in p.mle.tlv.addr_reg_iid \
                             for addr in pv.vars[name + '_IPADDRS'] if str(addr).startswith('2001:'))).\
        must_next()

    # Step 4: Leader
    # - Description: Automatically sends MLE Child Update response frame to the DUT.
    # - Pass Criteria: N/A
    print(f"Step 4: Leader automatically sends MLE Child Update response frame to the {name} (DUT).")
    pkts.filter_wpan_src64(leader).\
        filter_wpan_dst64(dut).\
        filter_mle_cmd(consts.MLE_CHILD_UPDATE_RESPONSE).\
        must_next()

    # Step 5: SED_1 (DUT)
    # - Description: For DUT = SED: The SED test ends here, the MED test continues.
    # - Pass Criteria: N/A
    print("Step 5: For DUT = SED: The SED test ends here.")

    if name == 'SED_1':
        return

    # Step 6: User
    # - Description: The user places the Leader OR the DUT in RF isolation to disable communication between them.
    # - Pass Criteria: N/A
    print("Step 6: The user places the Leader OR the DUT in RF isolation.")

    # Step 7: Test Harness
    # - Description: Test Harness prompt reads: Place DUT in RF Enclosure for time: t < child timeout, Press “OK”
    #   immediately after placing DUT in RF enclosure.
    # - Pass Criteria: N/A
    print("Step 7: Test Harness prompt.")

    # Step 8: Leader
    # - Description: Harness updates the Network Data by configuring the Leader with the following changes to the
    #   Prefix Set:
    #   - Prefix 2: P_prefix=2002::/64 P_stable=1 P_on_mesh=1 P_slaac=1 P_default=1
    #   - The Leader automatically sends a multicast MLE Data Response with the new network information.
    #   - The DUT is currently isolated from the Leader, so it does not hear this Data Response.
    # - Pass Criteria: N/A
    print("Step 8: Leader updates the Network Data and automatically sends a multicast MLE Data Response.")
    pkts.filter_wpan_src64(leader).\
        filter_LLANMA().\
        filter_mle_cmd(consts.MLE_DATA_RESPONSE).\
        must_next()

    # Step 9: User
    # - Description: The user must remove the RF isolation between the Leader and the DUT after the MLE Data Response
    #   is sent by the Leader for Prefix 2, but before the DUT timeout expires. (If the DUT timeout expires while in
    #   RF isolation, the test will fail because the DUT will go through re-attachment when it emerges.)
    # - Pass Criteria: N/A
    print("Step 9: The user must remove the RF isolation between the Leader and the DUT.")

    # Step 10: MED_1 (DUT)
    # - Description: Detects the updated Data Version in the Leader advertisement and automatically sends MLE Data
    #   Request to the Leader.
    # - Pass Criteria:
    #   - The DUT MUST send a MLE Data Request frame to request the updated Network Data.
    #   - The following TLVs MUST be included in the MLE Data Request:
    #     - TLV Request TLV: Network Data TLV
    print("Step 10: MED_1 (DUT) automatically sends MLE Data Request to the Leader.")
    pkts.filter_wpan_src64(dut).\
        filter_wpan_dst64(leader).\
        filter_mle_cmd(consts.MLE_DATA_REQUEST).\
        filter(lambda p: \
               {
                   consts.TLV_REQUEST_TLV,
                   consts.NETWORK_DATA_TLV
               } <= set(p.mle.tlv.type)).\
        must_next()

    # Step 11: Leader
    # - Description: Automatically sends the network data to the DUT.
    # - Pass Criteria: N/A
    print("Step 11: Leader automatically sends the network data to the DUT.")
    pkts.filter_wpan_src64(leader).\
        filter_wpan_dst64(dut).\
        filter_mle_cmd(consts.MLE_DATA_RESPONSE).\
        must_next()

    # Step 12: MED_1 (DUT)
    # - Description: Automatically sends MLE Child Update Request to the Leader.
    # - Pass Criteria:
    #   - The DUT MUST send a MLE Child Update Request to the Leader, which includes the following TLVs:
    #     - Address Registration TLV (contains the global addresses configured by DUT based on both advertised prefixes
    #       - 2001:: and 2002:: - and ML-EID address by checking the CID)
    #     - Leader Data TLV
    #     - Mode TLV
    #     - Timeout TLV
    print("Step 12: MED_1 (DUT) automatically sends MLE Child Update Request to the Leader.")
    pkts.filter_wpan_src64(dut).\
        filter_wpan_dst64(leader).\
        filter_mle_cmd(consts.MLE_CHILD_UPDATE_REQUEST).\
        filter(lambda p: \
               {
                   consts.ADDRESS_REGISTRATION_TLV,
                   consts.LEADER_DATA_TLV,
                   consts.MODE_TLV,
                   consts.TIMEOUT_TLV
               } <= set(p.mle.tlv.type)).\
        filter(lambda p: all(any(iid == int(addr[8:].hex(), 16) for iid in p.mle.tlv.addr_reg_iid) \
                             for prefix in ['2001:', '2002:'] \
                             for addr in pv.vars[name + '_IPADDRS'] if str(addr).startswith(prefix))).\
        must_next()


if __name__ == '__main__':
    verify_utils.run_main(Verify)
