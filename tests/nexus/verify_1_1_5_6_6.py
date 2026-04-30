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
    # 5.6.6 Network data expiration
    #
    # 5.6.6.1 Topology
    # - Router_1 is configured as Border Router.
    # - MED_1 is configured to require complete network data.
    # - SED_1 is configured to request only stable network data.
    #
    # 5.6.6.2 Purpose and Description
    # The purpose of this test case is to verify that network data is properly updated when deleting a prefix or
    # removing a server from the network.
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

    LEADER_RLOC16 = pv.vars['LEADER_RLOC16']
    ROUTER_1_RLOC16 = pv.vars['ROUTER_1_RLOC16']

    # Step 1: All
    # - Description: Ensure the topology is formed correctly.
    # - Pass Criteria: N/A.
    print("Step 1: All")

    # Step 2: Router_1
    # - Description: Harness configures Router_1 as Border Router with the following On-Mesh Prefix Set:
    #   - Prefix 1: P_Prefix=2001::/64 P_stable=1 P_on_mesh=1 P_preferred=1 P_slaac=1 P_default=1.
    #   - Prefix 2: P_Prefix=2002::/64 P_stable=0 P_on_mesh=1 P_preferred=1 P_slaac=1 P_default=1.
    #   - Prefix 3: P_Prefix=2003::/64 P_stable=1 P_on_mesh=1 P_preferred=1 P_slaac=1 P_default=0.
    #   - Automatically sends a CoAP Server Data Notification frame with the server’s information to the DUT:
    #     - CoAP Request URI: coap://[<DUT address>]:MM/a/sd.
    #     - CoAP Payload: Thread Network Data TLV.
    # - Pass Criteria: N/A.
    print("Step 2: Router_1")
    pkts.filter_wpan_src16(ROUTER_1_RLOC16).\
        filter_wpan_dst16(LEADER_RLOC16).\
        filter_coap_request(consts.SVR_DATA_URI).\
        must_next()

    # Step 3: Router_1
    # - Description: Harness instructs the device to remove Prefix 3 from its Prefix Set. Automatically sends a CoAP
    #   Server Data Notification frame with the server’s information to the DUT:
    #   - CoAP Request URI: coap://[<DUT address>]:MM/a/sd.
    #   - CoAP Payload: Thread Network Data TLV.
    # - Pass Criteria: N/A.
    print("Step 3: Router_1")
    pkts.filter_wpan_src16(ROUTER_1_RLOC16).\
        filter_wpan_dst16(LEADER_RLOC16).\
        filter_coap_request(consts.SVR_DATA_URI).\
        must_next()

    # Step 4: Leader (DUT)
    # - Description: Automatically sends a CoAP Response to Router_1.
    # - Pass Criteria: The DUT MUST send a 2.04 Changed CoAP response to Router_1.
    print("Step 4: Leader (DUT)")
    pkts.copy().filter_wpan_src16(LEADER_RLOC16).\
        filter_wpan_dst16(ROUTER_1_RLOC16).\
        filter_coap_ack(consts.SVR_DATA_URI).\
        filter(lambda p: p.coap.code == consts.COAP_CODE_ACK).\
        must_next()

    # Step 5: Leader (DUT)
    # - Description: Automatically multicasts new network information to neighbors and rx-on-when-idle Children.
    # - Pass Criteria: The DUT MUST multicast a MLE Data Response with the new network information collected from
    #   Router_1 including:
    #   - Leader Data TLV.
    #     - Data Version field <incremented>.
    #     - Stable Data Version field <incremented>.
    #   - Network Data TLV.
    #     - At least three Prefix TLVs (Prefix 1, 2 and 3).
    #     - The Prefix 1 and Prefix 2 TLVs MUST include: 6LoWPAN ID sub-TLV, Border Router sub-TLV.
    #     - The Prefix 3 TLV MUST include: 6LoWPAN ID sub-TLV <Compression flag = 0>.
    print("Step 5: Leader (DUT)")
    pkts.filter_wpan_src64(LEADER).\
        filter_LLANMA().\
        filter_mle_cmd(consts.MLE_DATA_RESPONSE).\
        filter(lambda p: {
            consts.LEADER_DATA_TLV,
            consts.NETWORK_DATA_TLV
        } <= set(p.mle.tlv.type)).\
        filter(lambda p: p.thread_nwd.tlv.prefix and Ipv6Addr('2001::') in p.thread_nwd.tlv.prefix).\
        filter(lambda p: p.thread_nwd.tlv.prefix and Ipv6Addr('2002::') in p.thread_nwd.tlv.prefix).\
        filter(lambda p: p.thread_nwd.tlv.prefix and Ipv6Addr('2003::') in p.thread_nwd.tlv.prefix).\
        filter(lambda p: len(p.thread_nwd.tlv.border_router_16) == 2).\
        filter(lambda p: len(p.thread_nwd.tlv['6co.flag.c']) == 3).\
        filter(lambda p: 0 in p.thread_nwd.tlv['6co.flag.c']).\
        must_next()

    # Step 6 and 7 are for MED_1. Step 8, 9, 10 are for SED_1.
    # These two sequences can happen in any order after Step 5.
    sed_pkts = pkts.copy()

    # Step 6: MED_1
    # - Description: Automatically sends address configured in the Address Registration TLV to the DUT in a MLE Child
    #   Update Request command.
    # - Pass Criteria: N/A.
    print("Step 6: MED_1")
    pkts.filter_wpan_src64(MED_1).\
        filter_wpan_dst64(LEADER).\
        filter_mle_cmd(consts.MLE_CHILD_UPDATE_REQUEST).\
        must_next()

    # Step 7: Leader (DUT)
    # - Description: Automatically responds with MLE Child Update Response to MED_1.
    # - Pass Criteria: The DUT MUST send an MLE Child Update Response, which includes the following TLVs:
    #   - Source Address TLV.
    #   - Leader Data TLV.
    #   - Address Registration TLV (Echoes back the addresses the Child has configured).
    #   - Mode TLV.
    print("Step 7: Leader (DUT)")
    pkts.filter_wpan_src64(LEADER).\
        filter_wpan_dst64(MED_1).\
        filter_mle_cmd(consts.MLE_CHILD_UPDATE_RESPONSE).\
        filter(lambda p: {
            consts.SOURCE_ADDRESS_TLV,
            consts.LEADER_DATA_TLV,
            consts.ADDRESS_REGISTRATION_TLV,
            consts.MODE_TLV
        } <= set(p.mle.tlv.type)).\
        must_next()

    # Leader (DUT) Note: Depending on the DUT’s device implementation, two different behavior paths (A, B) are allowable
    # for transmitting the new stable network data to SED_1:
    # - Path A: Notification via MLE Child Update, steps 8A-9.
    # - Path B: Notification via MLE Data Response, steps 8B-9.
    #
    # Step 8A: Leader (DUT)
    # - Description: Automatically sends notification of new stable network data to SED_1 via a unicast MLE Child
    #   Update Request.
    # - Pass Criteria: The DUT MUST send a unicast MLE Child Update Request to SED_1, which includes the following TLVs:
    #   - Source Address TLV.
    #   - Leader Data TLV.
    #     - Data Version field <incremented>.
    #     - Stable Data Version field <incremented>.
    #   - Network Data TLV:.
    #     - At least two Prefix TLVs (Prefix 1 and Prefix 3).
    #     - The Prefix 2 TLV MUST NOT be included.
    #     - The Prefix 1 TLV MUST include: 6LoWPAN ID sub-TLV, Border Router sub-TLV: P_border_router_16 <value = 0xFFFE>.
    #     - The Prefix 3 TLV MUST include: 6LoWPAN ID sub-TLV <compression flag set to 0>.
    #   - Active Timestamp TLV.
    #   - Goto Step 9.
    #
    # Step 8B: Leader (DUT)
    # - Description: Automatically sends notification of new stable network data to SED_1 via a unicast MLE Data
    #   Response.
    # - Pass Criteria: The DUT MUST send a unicast MLE Child Update Request to SED_1, which includes the following TLVs:
    #   - Source Address TLV.
    #   - Leader Data TLV.
    #     - Data Version field <incremented>.
    #     - Stable Data Version field <incremented>.
    #   - Network Data TLV:.
    #     - At least two Prefix TLVs (Prefix 1 and Prefix 3).
    #     - The Prefix 2 TLV MUST NOT be included.
    #     - The Prefix 1 TLV MUST include: 6LoWPAN ID sub-TLV, Border Router sub-TLV: P_border_router_16 <value = 0xFFFE>.
    #     - The Prefix 3 TLV MUST include: 6LoWPAN ID sub-TLV <compression flag set to 0>.
    #   - Active Timestamp TLV.
    print("Step 8: Leader (DUT)")
    sed_pkts.filter_wpan_src64(LEADER).\
        filter_wpan_dst64(SED_1).\
        filter(lambda p: p.mle.cmd and p.mle.cmd in {
            consts.MLE_CHILD_UPDATE_REQUEST,
            consts.MLE_DATA_RESPONSE
        }).\
        filter(lambda p: {
            consts.SOURCE_ADDRESS_TLV,
            consts.LEADER_DATA_TLV,
            consts.NETWORK_DATA_TLV,
            consts.ACTIVE_TIMESTAMP_TLV
        } <= set(p.mle.tlv.type)).\
        filter(lambda p: p.thread_nwd.tlv.prefix and Ipv6Addr('2001::') in p.thread_nwd.tlv.prefix).\
        filter(lambda p: p.thread_nwd.tlv.prefix and Ipv6Addr('2003::') in p.thread_nwd.tlv.prefix).\
        filter(lambda p: not p.thread_nwd.tlv.prefix or Ipv6Addr('2002::') not in p.thread_nwd.tlv.prefix).\
        filter(lambda p: len(p.thread_nwd.tlv.border_router_16) == 1).\
        filter(lambda p: p.thread_nwd.tlv.border_router_16[0] == 0xfffe).\
        filter(lambda p: len(p.thread_nwd.tlv['6co.flag.c']) == 2).\
        filter(lambda p: 0 in p.thread_nwd.tlv['6co.flag.c']).\
        must_next()

    # Step 9: SED_1
    # - Description: Automatically sends address configured in the Address Registration TLV to the DUT in a MLE Child
    #   Update Request command.
    # - Pass Criteria: N/A.
    print("Step 9: SED_1")
    sed_pkts.filter_wpan_src64(SED_1).\
        filter_wpan_dst64(LEADER).\
        filter_mle_cmd(consts.MLE_CHILD_UPDATE_REQUEST).\
        must_next()

    # Step 10: Leader (DUT)
    # - Description: Automatically responds with MLE Child Update Response to SED_1.
    # - Pass Criteria: The DUT MUST send an MLE Child Update Response, which includes the following TLVs:
    #   - Source Address TLV.
    #   - Leader Data TLV.
    #   - Address Registration TLV (Echoes back the addresses the child has configured).
    #   - Mode TLV.
    print("Step 10: Leader (DUT)")
    sed_pkts.filter_wpan_src64(LEADER).\
        filter_wpan_dst64(SED_1).\
        filter_mle_cmd(consts.MLE_CHILD_UPDATE_RESPONSE).\
        filter(lambda p: {
            consts.SOURCE_ADDRESS_TLV,
            consts.LEADER_DATA_TLV,
            consts.ADDRESS_REGISTRATION_TLV,
            consts.MODE_TLV
        } <= set(p.mle.tlv.type)).\
        must_next()

    # Step 11: Router_1
    # - Description: Harness silently powers-off the device.
    # - Pass Criteria: N/A.
    print("Step 11: Router_1")

    # Step 12: Leader (DUT)
    # - Description: Automatically updates Router ID Set and removes Router_1 from Network Data TLV.
    # - Pass Criteria: The DUT MUST detect that Router_1 is removed from the network and update the Router ID Set
    #   accordingly:
    #   - Remove the Network Data sections corresponding to Router_1.
    #   - Increment the Data Version and Stable Data Version fields.
    print("Step 12: Leader (DUT)")

    # Step 13: Leader (DUT)
    # - Description: Automatically multicasts new network information to neighbors and rx-on-when-idle Children.
    # - Pass Criteria: The DUT MUST multicast a MLE Data Response with the new network information including:
    #   - Leader Data TLV (Data Version field <incremented>, Stable Data Version field <incremented>).
    #   - Network Data TLV.
    print("Step 13: Leader (DUT)")
    pkts.filter_wpan_src64(LEADER).\
        filter_LLANMA().\
        filter_mle_cmd(consts.MLE_DATA_RESPONSE).\
        filter(lambda p: {
            consts.LEADER_DATA_TLV,
            consts.NETWORK_DATA_TLV
        } <= set(p.mle.tlv.type)).\
        filter(lambda p: p.thread_nwd.tlv.prefix is nullField).\
        must_next()

    # Step 14 and 15 are for MED_1. Step 16, 17, 18 are for SED_1.
    # These two sequences can happen in any order after Step 13.
    sed_pkts = pkts.copy()

    # Step 14: MED_1
    # - Description: Automatically sends address configured in the Address Registration TLV to the DUT in a MLE Child
    #   Update Request command.
    # - Pass Criteria: N/A.
    print("Step 14: MED_1")
    pkts.filter_wpan_src64(MED_1).\
        filter_wpan_dst64(LEADER).\
        filter_mle_cmd(consts.MLE_CHILD_UPDATE_REQUEST).\
        must_next()

    # Step 15: Leader (DUT)
    # - Description: Automatically responds with MLE Child Update Response to MED_1.
    # - Pass Criteria: The DUT MUST send an MLE Child Update Response, which includes the following TLVs:
    #   - Source Address TLV.
    #   - Leader Data TLV.
    #   - Address Registration TLV (Echoes back the addresses the child has configured).
    #   - Mode TLV.
    print("Step 15: Leader (DUT)")
    pkts.filter_wpan_src64(LEADER).\
        filter_wpan_dst64(MED_1).\
        filter_mle_cmd(consts.MLE_CHILD_UPDATE_RESPONSE).\
        filter(lambda p: {
            consts.SOURCE_ADDRESS_TLV,
            consts.LEADER_DATA_TLV,
            consts.ADDRESS_REGISTRATION_TLV,
            consts.MODE_TLV
        } <= set(p.mle.tlv.type)).\
        must_next()

    # Leader (DUT) Note: Depending upon the DUT’s device implementation, two different behavior paths (A,B) are allowable
    # for transmitting the new stable network data to SED_1:
    # - Path A: Notification via MLE Child Update Request, steps 16A-17.
    # - Path B: Notification via MLE Data Response, steps 16B-17.
    #
    # Step 16A: Leader (DUT)
    # - Description: Automatically sends notification of new stable network data to SED_1 via a unicast MLE Child
    #   Update Request.
    # - Pass Criteria: The DUT MUST send a unicast MLE Child Update Request to SED_1, which includes the following TLVs:
    #   - Source Address TLV.
    #   - Leader Data TLV (Data Version field <incremented>, Stable Data Version field <incremented>).
    #   - Network Data TLV.
    #   - Active Timestamp TLV.
    #   - Goto Step 17.
    #
    # Step 16B: Leader (DUT)
    # - Description: Automatically sends notification of new stable network data to SED_1 via a unicast MLE Data
    #   Response.
    # - Pass Criteria: The DUT MUST send a unicast MLE Child Update Request to SED_1, which includes the following TLVs:
    #   - Source Address TLV.
    #   - Leader Data TLV (Data Version field <incremented>, Stable Data Version field <incremented>).
    #   - Network Data TLV.
    #   - Active Timestamp TLV.
    print("Step 16: Leader (DUT)")
    sed_pkts.filter_wpan_src64(LEADER).\
        filter_wpan_dst64(SED_1).\
        filter(lambda p: p.mle.cmd and p.mle.cmd in {
            consts.MLE_CHILD_UPDATE_REQUEST,
            consts.MLE_DATA_RESPONSE
        }).\
        filter(lambda p: {
            consts.SOURCE_ADDRESS_TLV,
            consts.LEADER_DATA_TLV,
            consts.NETWORK_DATA_TLV,
            consts.ACTIVE_TIMESTAMP_TLV
        } <= set(p.mle.tlv.type)).\
        filter(lambda p: p.thread_nwd.tlv.prefix is nullField).\
        must_next()

    # Step 17: SED_1
    # - Description: Automatically sends address configured in the Address Registration TLV to the DUT in a MLE Child
    #   Update Request command.
    # - Pass Criteria: N/A.
    print("Step 17: SED_1")
    sed_pkts.filter_wpan_src64(SED_1).\
        filter_wpan_dst64(LEADER).\
        filter_mle_cmd(consts.MLE_CHILD_UPDATE_REQUEST).\
        must_next()

    # Step 18: Leader (DUT)
    # - Description: Automatically responds with MLE Child Update Response to SED_1.
    # - Pass Criteria: The DUT MUST send an MLE Child Update Response, which includes the following TLVs:
    #   - Source Address TLV.
    #   - Leader Data TLV.
    #   - Address Registration TLV (Echoes back the addresses the child has configured).
    #   - Mode TLV.
    print("Step 18: Leader (DUT)")
    sed_pkts.filter_wpan_src64(LEADER).\
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
