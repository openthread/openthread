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
    # 7.1.8 Network data propagation - Border Router as End Device in Thread network; registers new server data
    #   information after network is formed
    #
    # 7.1.8.1 Topology
    # - FED_1 is configured to require complete network data. (Mode TLV)
    #
    # 7.1.8.2 Purpose and Description
    # The purpose of this test case is to verify that when global prefix information is set on the FED, the DUT
    #   properly disseminates the associated network data. It also verifies that the DUT sends revised server data
    #   information to the Leader when the FED is removed.
    #
    # Spec Reference                             | V1.1 Section       | V1.3.0 Section
    # -------------------------------------------|--------------------|--------------------
    # Thread Network Data / Stable Thread        | 5.13 / 5.14 / 5.15 | 5.13 / 5.14 / 5.15
    #   Network Data / Network Data Propagation  |                    |

    pkts = pv.pkts
    pv.summary.show()

    LEADER = pv.vars['LEADER']
    ROUTER_1 = pv.vars['ROUTER_1']
    FED_1 = pv.vars['FED_1']
    LEADER_RLOC16 = pv.vars['LEADER_RLOC16']
    FED_1_RLOC16 = pv.vars['FED_1_RLOC16']

    PREFIX_1 = Ipv6Addr("2001::")
    PREFIX_2 = Ipv6Addr("2002::")

    # Step 1: All
    # - Description: Topology Ensure topology is formed correctly.
    # - Pass Criteria: N/A.
    print("Step 1: Topology Ensure topology is formed correctly.")
    pkts.filter_wpan_src64(LEADER).\
      filter_mle_cmd(consts.MLE_ADVERTISEMENT).\
      must_next()
    pkts.filter_wpan_src64(ROUTER_1).\
      filter_mle_cmd(consts.MLE_ADVERTISEMENT).\
      must_next()
    pkts.filter_wpan_src64(FED_1).\
      filter_mle_cmd(consts.MLE_CHILD_ID_REQUEST).\
      must_next()

    # Step 2: FED_1
    # - Description: Harness configures device with the following On-Mesh Prefix Set:
    #   - Prefix 1: P_Prefix=2001::/64 P_stable=1 P_default=1 P_slaac=1 P_on_mesh=1 P_preferred=1
    #   - Prefix 2: P_Prefix=2002::/64 P_stable=0 P_default=1 P_slaac=1 P_on_mesh=1 P_preferred=1
    #   - Automatically sends a CoAP Server Data Notification message with the server’s information (Prefix, Border
    #     Router) to the Leader.
    # - Pass Criteria: N/A.
    print("Step 2: FED_1 sends a CoAP Server Data Notification message.")
    pkts.filter_wpan_src64(FED_1).\
      filter_coap_request(consts.SVR_DATA_URI).\
      filter(lambda p: consts.NL_THREAD_NETWORK_DATA_TLV in p.coap.tlv.type).\
      must_next()

    # Step 3: Leader
    # - Description: Automatically transmits a 2.04 Changed CoAP response to the DUT. Automatically transmits
    #   multicast MLE Data Response with the new information collected, adding also 6LoWPAN ID TLV for the prefix set
    #   on FED_1.
    # - Pass Criteria: N/A.
    print("Step 3: Leader transmits a 2.04 Changed CoAP response and multicast MLE Data Response.")
    pkts.filter_wpan_src64(LEADER).\
      filter_LLANMA().\
      filter_mle_cmd(consts.MLE_DATA_RESPONSE).\
      filter(lambda p: {
        PREFIX_1,
        PREFIX_2
      } <= set(p.thread_nwd.tlv.prefix)).\
      must_next()
    pkts.filter_wpan_src64(LEADER).\
      filter_coap_ack(consts.SVR_DATA_URI).\
      must_next()

    # Step 4: Router_1 (DUT)
    # - Description: Automatically transmits multicast MLE Data Response with the new information collected, adding
    #   also 6LoWPAN ID TLV for the prefix set on FED_1.
    # - Pass Criteria: The DUT MUST send a multicast MLE Data Response.
    print("Step 4: Router_1 (DUT) transmits multicast MLE Data Response.")
    pkts.filter_wpan_src64(ROUTER_1).\
      filter_LLANMA().\
      filter_mle_cmd(consts.MLE_DATA_RESPONSE).\
      filter(lambda p: {
        PREFIX_1,
        PREFIX_2
      } <= set(p.thread_nwd.tlv.prefix)).\
      must_next()

    # Step 5: FED_1
    # - Description: Harness silently powers-down FED_1 and waits for Router_1 to remove FED_1 from its neighbor
    #   table.
    # - Pass Criteria: N/A.
    print("Step 5: FED_1 is powered down.")

    # Step 6: Router_1 (DUT)
    # - Description: Automatically notifies Leader of removed server’s (FED_1’s) RLOC16.
    # - Pass Criteria: The DUT MUST send a CoAP Server Data Notification message to the Leader containing only the
    #   removed server’s RLOC16:
    #   - CoAP Request URI: coap://[<leader address>]:MM/a/sd
    #   - CoAP Payload: RLOC16 TLV.
    print("Step 6: Router_1 (DUT) notifies Leader of removed server's RLOC16.")
    pkts.filter_wpan_src64(ROUTER_1).\
      filter_coap_request(consts.SVR_DATA_URI).\
      filter(lambda p: p.coap.tlv.type == [consts.NL_RLOC16_TLV]).\
      filter(lambda p: p.coap.tlv.rloc16 == FED_1_RLOC16).\
      must_next()


if __name__ == '__main__':
    verify_utils.run_main(verify)
