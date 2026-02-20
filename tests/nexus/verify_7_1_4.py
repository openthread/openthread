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
    # 7.1.4 Network data propagation – Border Router as Router in Thread network; registers new server data information
    #   after network is formed
    #
    # 7.1.4.1 Topology
    # - MED_1 is configured to require complete network data. (Mode TLV)
    # - SED_1 is configured to request only stable network data. (Mode TLV)
    #
    # 7.1.4.2 Purpose & Description
    # The purpose of this test case is to verify that when global prefix information is set on the DUT, the DUT
    #   properly unicasts information to the Leader using COAP frame (Server Data Notification). In addition, the DUT
    #   must correctly set Network Data (stable/non-stable) aggregated and disseminated by the Leader and transmit it
    #   properly to all devices already attached to it.
    #
    # Spec Reference                             | V1.1 Section    | V1.3.0 Section
    # -------------------------------------------|-----------------|-----------------
    # Thread Network Data / Stable Thread        | 5.13 / 5.14 /   | 5.13 / 5.14 /
    #   Network Data / Network Data and          | 5.15            | 5.15
    #   Propagation                              |                 |

    pkts = pv.pkts
    pv.summary.show()

    LEADER = pv.vars['LEADER']
    ROUTER_1 = pv.vars['ROUTER_1']
    MED_1 = pv.vars['MED_1']
    SED_1 = pv.vars['SED_1']

    # Step 1: All
    # - Description: Topology Ensure topology is formed correctly.
    # - Pass Criteria: N/A
    print("Step 1: All")
    pkts.filter_wpan_src64(LEADER).\
        filter_LLANMA().\
        filter_mle_cmd(consts.MLE_ADVERTISEMENT).\
        must_next()

    pkts.filter_wpan_src64(ROUTER_1).\
        filter_mle_cmd(consts.MLE_ADVERTISEMENT).\
        must_next()

    # Step 2: Router_1 (DUT)
    # - Description: User configures the DUT with the following On-Mesh Prefix Set:
    #   - Prefix 1: P_prefix=2001::/64 P_stable=1 P_on_mesh=1 P_preferred=1 P_slaac=1 P_default=1
    #   - Prefix 2: P_prefix=2002::/64 P_stable=0 P_on_mesh=1 P_preferred=1 P_slaac=1 P_default=1
    # - Pass Criteria: N/A
    print("Step 2: Router_1 (DUT)")

    # Step 3: Router_1 (DUT)
    # - Description: Automatically transmits a CoAP Server Data Notification to the Leader
    # - Pass Criteria: The DUT MUST send a CoAP Server Data Notification frame with the server’s information (Prefix,
    #   Border Router) to the Leader:
    #   - CoAP Request URI: coap://[<Leader address>]:MM/a/sd
    #   - CoAP Payload: Network Data TLV
    print("Step 3: Router_1 (DUT)")
    pkts.filter_wpan_src64(ROUTER_1).\
        filter_coap_request(consts.SVR_DATA_URI).\
        filter(lambda p: consts.NL_THREAD_NETWORK_DATA_TLV in p.coap.tlv.type).\
        must_next()

    # Step 4: Leader
    # - Description: Automatically transmits a 2.04 Changed CoAP response to the DUT. Automatically multicasts a MLE
    #   Data Response, including the new information collected from the DUT.
    # - Pass Criteria: N/A
    print("Step 4: Leader")

    # The MLE Data Response and CoAP ACK from Leader might be out of order.
    # We find both and update the cursor to the later one.
    _pkts = pkts.copy()

    _mle_data_rsp = _pkts.filter_wpan_src64(LEADER).\
        filter_LLANMA().\
        filter_mle_cmd(consts.MLE_DATA_RESPONSE).\
        filter(lambda p: p.thread_nwd.tlv.prefix is not nullField and\
                         '2001::' in p.thread_nwd.tlv.prefix and\
                         '2002::' in p.thread_nwd.tlv.prefix).\
        must_next()

    _coap_ack = pkts.filter_wpan_src64(LEADER).\
        filter_coap_ack(consts.SVR_DATA_URI).\
        must_next()

    pkts.index = (max(pkts.index[0], _pkts.index[0]), max(pkts.index[1], _pkts.index[1]))

    # Step 5: Router_1 (DUT)
    # - Description: Automatically sends new network data to MED_1
    # - Pass Criteria: The DUT MUST send a multicast MLE Data Response, including the following TLVs:
    #   - At least two Prefix TLVs (Prefix 1 and Prefix 2):
    #     - 6LowPAN ID TLV
    #     - Border Router TLV
    print("Step 5: Router_1 (DUT)")
    index_before_step5 = pkts.index
    pkts.filter_wpan_src64(ROUTER_1).\
        filter_LLANMA().\
        filter_mle_cmd(consts.MLE_DATA_RESPONSE).\
        filter(lambda p: p.thread_nwd.tlv.prefix is not nullField and\
                         '2001::' in p.thread_nwd.tlv.prefix and\
                         '2002::' in p.thread_nwd.tlv.prefix).\
        must_next()

    # Step 6: MED_1
    # - Description: Automatically sends the address configured to Router_1 (DUT) via the Address Registration TLV,
    #   included as part of the Child Update request command.
    # - Pass Criteria: The DUT MUST unicast MLE Child Update Response to MED_1, including the following TLVs:
    #   - Source Address TLV
    #   - Address Registration TLV (Echoes back the addresses MED_1 has configured)
    #   - Mode TLV
    print("Step 6: MED_1")
    pkts.filter_wpan_src64(MED_1).\
        filter_mle_cmd(consts.MLE_CHILD_UPDATE_REQUEST).\
        filter(lambda p: consts.ADDRESS_REGISTRATION_TLV in p.mle.tlv.type).\
        must_next()

    pkts.filter_wpan_src64(ROUTER_1).\
        filter_mle_cmd(consts.MLE_CHILD_UPDATE_RESPONSE).\
        filter(lambda p: {
                          consts.SOURCE_ADDRESS_TLV,
                          consts.ADDRESS_REGISTRATION_TLV,
                          consts.MODE_TLV
                         } <= set(p.mle.tlv.type)).\
        must_next()

    # Step 7: Router_1 (DUT)
    # - Description: Automatically sends notification of new network data to SED_1 via a unicast MLE Child Update
    #   Request or MLE Data Response.
    # - Pass Criteria: The DUT MUST unicast MLE Child Update Request or MLE Data Response to SED_1.
    print("Step 7: Router_1 (DUT)")
    pkts.index = index_before_step5
    pkts.filter_wpan_src64(ROUTER_1).\
        filter_mle_cmd2(consts.MLE_CHILD_UPDATE_REQUEST, consts.MLE_DATA_RESPONSE).\
        filter(lambda p: p.thread_nwd.tlv.prefix is not nullField and\
                         '2001::' in p.thread_nwd.tlv.prefix and\
                         '2002::' not in p.thread_nwd.tlv.prefix).\
        must_next()

    # Step 8: SED_1
    # - Description: After receiving the MLE Data Response or MLE Child Update Request, automatically sends the global
    #   address configured to Router_1 (DUT), via the Address Registration TLV, included as part of the Child Update
    #   request command.
    # - Pass Criteria: N/A
    print("Step 8: SED_1")
    pkts.filter_wpan_src64(SED_1).\
        filter_mle_cmd(consts.MLE_CHILD_UPDATE_REQUEST).\
        filter(lambda p: consts.ADDRESS_REGISTRATION_TLV in p.mle.tlv.type).\
        must_next()

    # Step 9: Router_1 (DUT)
    # - Description: Automatically sends a Child Update Response to SED_1, echoing back the configured addresses
    #   reported by SED_1
    # - Pass Criteria: The DUT MUST unicast MLE Child Update Response to SED_1. The following TLVs MUST be included in
    #   the Child Update Response:
    #   - Source Address TLV
    #   - Address Registration TLV (Echoes back the addresses SED_1 has configured)
    #   - Mode TLV
    print("Step 9: Router_1 (DUT)")
    pkts.filter_wpan_src64(ROUTER_1).\
        filter_mle_cmd(consts.MLE_CHILD_UPDATE_RESPONSE).\
        filter(lambda p: {
                          consts.SOURCE_ADDRESS_TLV,
                          consts.ADDRESS_REGISTRATION_TLV,
                          consts.MODE_TLV
                         } <= set(p.mle.tlv.type)).\
        must_next()


if __name__ == '__main__':
    verify_utils.run_main(verify)
