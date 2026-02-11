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


def verify(pv):
    pkts = pv.pkts
    pv.summary.show()

    LEADER = pv.vars['LEADER']
    ROUTER_1 = pv.vars['ROUTER_1']
    ROUTER_24 = pv.vars['ROUTER_24']

    # 5.2.6 Router Downgrade Threshold - REED
    #
    # 5.2.6.1 Topology
    # - Build a topology with 23 active routers, including the Leader, with no communication constraints and links of
    #   highest quality (quality=3)
    # - Set Router Downgrade Threshold and Router Upgrade Threshold on all test bed routers and Leader to 32
    #
    # 5.2.6.2 Purpose & Description
    # The purpose of this test case is to verify that the DUT will downgrade to a REED when the network becomes too
    #   dense and the Router Downgrade Threshold conditions are met.
    #
    # Spec Reference        | V1.1 Section | V1.3.0 Section
    # ----------------------|--------------|---------------
    # Router ID Management  | 5.9.9        | 5.9.9
    print("5.2.6 Router Downgrade Threshold - REED")

    # Step 1: All
    # - Description: Ensure topology is formed correctly without Router_24,
    # - Pass Criteria: N/A
    print("Step 1: All - Ensure topology is formed correctly without Router_24")

    # Step 2: Router_24
    # - Description: Harness causes Router_24 to attach to the network and ensures it has a link of quality 2 or better
    #   to Router_1 and Router_2
    # - Pass Criteria: N/A
    print("Step 2: Router_24 - Harness causes Router_24 to attach to the network")
    pkts.filter_wpan_src64(ROUTER_24).\
      filter_mle_cmd(consts.MLE_PARENT_REQUEST).\
      must_next()

    # Step 3: Router_1 (DUT)
    # - Description: Allow enough time for the DUT to get Network Data Updates and resign its Router ID.
    # - Pass Criteria:
    #   - The DUT MUST first reconnect to the network as a Child by sending properly formatted Parent Request and
    #     Child ID Request messages.
    #   - Once the DUT attaches as a Child, it MUST send an Address Release Message to the Leader:
    #     - CoAP Request URI: coap://[<leader address>]:MM/a/ar
    #     - CoAP Payload:
    #       - MAC Extended Address TLV
    #       - RLOC16 TLV,
    print(
        "Step 3: Router_1 (DUT) - Allow enough time for the DUT to get Network Data Updates and resign its Router ID.")

    # Verify DUT sends Parent Request
    pkts.filter_wpan_src64(ROUTER_1).\
      filter_mle_cmd(consts.MLE_PARENT_REQUEST).\
      must_next()

    # Verify DUT sends Child ID Request
    pkts.filter_wpan_src64(ROUTER_1).\
      filter_mle_cmd(consts.MLE_CHILD_ID_REQUEST).\
      must_next()

    # Verify DUT sends Address Release with required TLVs
    pkts.filter_wpan_src64(ROUTER_1).\
      filter_coap_request(consts.ADDR_REL_URI).\
      filter(lambda p: {
        consts.NL_MAC_EXTENDED_ADDRESS_TLV,
        consts.NL_RLOC16_TLV
      } <= {int(t, 16) if isinstance(t, str) else t for t in p.coap.tlv.type}).\
      must_next()

    # Step 4: Leader
    # - Description: Receives Address Release message and automatically sends a 2.04 Changed CoAP response.
    # - Pass Criteria: N/A
    print("Step 4: Leader - Receives Address Release message and automatically sends a 2.04 Changed CoAP response.")
    pkts.filter_wpan_src64(LEADER).\
      filter_coap_ack(consts.ADDR_REL_URI).\
      filter(lambda p: p.coap.code == consts.COAP_CODE_ACK).\
      must_next()

    # Step 5: Leader
    # - Description: Harness verifies connectivity by instructing the device to send an ICMPv6 Echo Request to the DUT
    # - Pass Criteria: The DUT MUST respond with an ICMPv6 Echo Reply
    print("Step 5: Leader - Harness verifies connectivity by instructing the device to send an ICMPv6 Echo Request")
    _pkt = pkts.filter_ping_request().\
      filter_wpan_src64(LEADER).\
      must_next()
    pkts.filter_ping_reply(identifier=_pkt.icmpv6.echo.identifier).\
      must_next()


if __name__ == '__main__':
    verify_utils.run_main(verify)
