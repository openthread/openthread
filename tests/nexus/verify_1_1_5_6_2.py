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
    # 5.6.2 Network data propagation (BR exists during attach) - Router as BR
    #
    # 5.6.2.1 Topology
    # - Router_1 is configured as Border Router.
    # - MED_1 is configured to require complete network data. (Mode TLV)
    # - SED_1 is configured to request only stable network data. (Mode TLV)
    #
    # 5.6.2.2 Purpose & Description
    # The purpose of this test case is to verify that the DUT, as Leader, collects
    #   network data information (stable/non-stable) from the network and
    #   propagates it properly during the attach procedure.
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

    # Step 1: Leader (DUT)
    # - Description: Forms the network and sends MLE Advertisements.
    # - Pass Criteria:
    #   - The DUT MUST send properly formatted MLE Advertisements, with an IP Hop
    #     Limit of 255, to the Link-Local All Nodes multicast address (FF02::1).
    #   - The following TLVs MUST be present in the MLE Advertisements:
    #     - Leader Data TLV
    #     - Route64 TLV
    #     - Source Address TLV
    print("Step 1: Leader (DUT) forms the network and sends MLE Advertisements.")
    pkts.filter_wpan_src64(LEADER).\
        filter_LLANMA().\
        filter_mle_cmd(consts.MLE_ADVERTISEMENT).\
        filter(lambda p: {
            consts.LEADER_DATA_TLV,
            consts.ROUTE64_TLV,
            consts.SOURCE_ADDRESS_TLV
        } <= set(p.mle.tlv.type)).\
        filter(lambda p: p.ipv6.hlim == 255).\
        must_next()

    # Step 2: Router_1
    # - Description: Harness instructs the device to attach to the DUT. Router_1
    #   requests Network Data TLV during the attaching procedure when sending the
    #   MLE Child ID Request frame.
    # - Pass Criteria: N/A
    print("Step 2: Router_1 attaches to the DUT.")
    pkts.filter_wpan_src64(ROUTER_1).\
        filter_mle_cmd(consts.MLE_CHILD_ID_REQUEST).\
        filter(lambda p: {
            consts.TLV_REQUEST_TLV,
            consts.NETWORK_DATA_TLV
        } <= set(p.mle.tlv.type)).\
        must_next()

    # Step 3: Leader (DUT)
    # - Description: Automatically sends MLE Parent Response and MLE Child ID
    #   Response to Router_1.
    # - Pass Criteria:
    #   - The DUT MUST properly attach Router_1 device to the network (See 5.1.1
    #     Attaching for formatting), and transmit Network Data during the attach
    #     phase in the Child ID Response frame of the Network Data TLV.
    print("Step 3: Leader (DUT) sends MLE Parent Response and MLE Child ID Response to Router_1.")
    pkts.filter_wpan_dst64(ROUTER_1).\
        filter_mle_cmd(consts.MLE_CHILD_ID_RESPONSE).\
        filter(lambda p: consts.NETWORK_DATA_TLV in p.mle.tlv.type).\
        must_next()

    # Step 4: Router_1
    # - Description: Harness configures the device as a Border Router with the
    #   following On-Mesh Prefix Set:
    #   - Prefix 1: P_prefix=2001::/64 P_stable=1 P_on_mesh=1 P_preferred=1
    #     P_slaac=1 P_default=1
    #   - Prefix 2: P_prefix=2002::/64 P_stable=0 P_on_mesh=1 P_preferred=1
    #     P_slaac=1 P_default=1
    #   - Router_1 automatically sends a CoAP Server Data Notification frame
    #     with the server’s information to the DUT:
    #     - CoAP Request URI: coap://[<DUT address>]:MM/a/sd
    #     - CoAP Payload: Thread Network Data TLV
    # - Pass Criteria: N/A
    print("Step 4: Router_1 configures as a Border Router and sends CoAP Server Data Notification.")
    pkts.filter_wpan_src64(ROUTER_1).\
        filter_coap_request(consts.SVR_DATA_URI).\
        must_next()

    # Step 5: Leader (DUT)
    # - Description: Automatically sends a CoAP Response frame and MLE Data
    #   Response message.
    # - Pass Criteria:
    #   - The DUT MUST transmit a 2.04 Changed CoAP response code to Router_1.
    #   - The DUT MUST multicast an MLE Data Response message with the new
    #     information collected, adding also the 6LoWPAN ID TLV for the prefix
    #     set on Router_1.
    print("Step 5: Leader (DUT) sends a CoAP Response frame and MLE Data Response message.")
    pkts.filter_coap_ack(consts.SVR_DATA_URI).\
        must_next()

    pkts.filter_LLANMA().\
        filter_mle_cmd(consts.MLE_DATA_RESPONSE).\
        filter(lambda p: {
            Ipv6Addr("2001::"),
            Ipv6Addr("2002::")
        } <= set(p.thread_nwd.tlv.prefix)).\
        must_next()

    # Step 6: SED_1
    # - Description: Harness instructs the device to attach to the DUT. SED_1
    #   requests only the stable Network Data (Mode TLV in Child ID Request
    #   frame has “N” bit set to 0).
    # - Pass Criteria: N/A
    print("Step 6: SED_1 attaches to the DUT and requests only stable Network Data.")
    pkts.filter_wpan_src64(SED_1).\
        filter_mle_cmd(consts.MLE_CHILD_ID_REQUEST).\
        filter(lambda p: p.mle.tlv.mode.network_data == 0).\
        must_next()

    # Step 7: Leader (DUT)
    # - Description: Automatically sends MLE Parent Response and MLE Child ID
    #   Response.
    # - Pass Criteria:
    #   - The DUT MUST send an MLE Child ID Response to SED_1, containing only
    #     stable Network Data, including:
    #     - At least the Prefix 1 TLV - The Prefix 2 TLV MUST NOT be included.
    #     - The required Prefix TLV MUST include the following fields:
    #       - 6LoWPAN ID sub-TLV
    #       - Border Router sub-TLV
    #       - P_border_router_16 <value = 0xFFFE>
    print("Step 7: Leader (DUT) sends MLE Parent Response and MLE Child ID Response to SED_1.")
    pkts.filter_wpan_dst64(SED_1).\
        filter_mle_cmd(consts.MLE_CHILD_ID_RESPONSE).\
        filter(lambda p: Ipv6Addr("2001::") in p.thread_nwd.tlv.prefix).\
        filter(lambda p: Ipv6Addr("2002::") not in p.thread_nwd.tlv.prefix).\
        filter(lambda p: all(rloc == 0xfffe for rloc in p.thread_nwd.tlv.border_router_16)).\
        must_next()

    # Step 8: MED_1
    # - Description: Harness instructs the device to attach to the DUT. MED_1
    #   requests the complete Network Data (Mode TLV in Child ID Request
    #   frame has “N” bit set to 1).
    # - Pass Criteria: N/A
    print("Step 8: MED_1 attaches to the DUT and requests complete Network Data.")
    pkts.filter_wpan_src64(MED_1).\
        filter_mle_cmd(consts.MLE_CHILD_ID_REQUEST).\
        filter(lambda p: p.mle.tlv.mode.network_data == 1).\
        must_next()

    # Step 9: Leader (DUT)
    # - Description: Automatically sends MLE Parent Response and MLE Child ID
    #   Response.
    # - Pass Criteria:
    #   - The DUT MUST send an MLE Child ID Response to MED_1, containing the
    #     full Network Data, including:
    #     - At least two Prefix TLVs (one for Prefix set 1 and 2), each including:
    #       - 6LoWPAN ID sub-TLV
    #       - Border Router sub-TLV
    print("Step 9: Leader (DUT) sends MLE Parent Response and MLE Child ID Response to MED_1.")
    pkts.filter_wpan_dst64(MED_1).\
        filter_mle_cmd(consts.MLE_CHILD_ID_RESPONSE).\
        filter(lambda p: {
            Ipv6Addr("2001::"),
            Ipv6Addr("2002::")
        } <= set(p.thread_nwd.tlv.prefix)).\
        must_next()

    # Step 10: SED_1, MED_1
    # - Description: Automatically send global address configured in the Address
    #   Registration TLV to their parent in a MLE Child Update Request command.
    # - Pass Criteria: N/A
    print("Step 10: SED_1 and MED_1 send MLE Child Update Request with Address Registration TLV.")
    pkts.copy().\
        filter_wpan_src64(SED_1).\
        filter_mle_cmd(consts.MLE_CHILD_UPDATE_REQUEST).\
        filter(lambda p: consts.ADDRESS_REGISTRATION_TLV in p.mle.tlv.type).\
        must_next()

    pkts.copy().\
        filter_wpan_src64(MED_1).\
        filter_mle_cmd(consts.MLE_CHILD_UPDATE_REQUEST).\
        filter(lambda p: consts.ADDRESS_REGISTRATION_TLV in p.mle.tlv.type).\
        must_next()

    # Step 11: Leader (DUT)
    # - Description: Automatically sends MLE Child Update Response to MED_1
    #   and SED_1.
    # - Pass Criteria:
    #   - The following TLVs MUST be present in the MLE Child Update Response:
    #     - Address Registration TLV (Echoes back the addresses the child
    #       has configured)
    #     - Leader Data TLV
    #     - Mode TLV
    #     - Source Address TLV
    print("Step 11: Leader (DUT) sends MLE Child Update Response to MED_1 and SED_1.")

    def verify_child_update_response(child_ext64):
        pkts.copy().\
            filter_wpan_dst64(child_ext64).\
            filter_mle_cmd(consts.MLE_CHILD_UPDATE_RESPONSE).\
            filter(lambda p: {
                consts.ADDRESS_REGISTRATION_TLV,
                consts.LEADER_DATA_TLV,
                consts.MODE_TLV,
                consts.SOURCE_ADDRESS_TLV
            } <= set(p.mle.tlv.type)).\
            must_next()

    verify_child_update_response(SED_1)
    verify_child_update_response(MED_1)


if __name__ == '__main__':
    verify_utils.run_main(verify)
