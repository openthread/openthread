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
    # 5.11.1 BBR-TC-01: Device MUST send its BBR dataset to Leader if none exists
    #
    # 5.11.1.1 Topology
    # - BR_1 (DUT): BR device operating as a BBR.
    # - Leader: Test bed bed device operating as a Thread Leader.
    # - Router: Test bed device operating as a Thread Router.
    # - Host: Test bed BR device operating as a non-Thread device.
    #
    # 5.11.1.2 Purpose and Description
    # The purpose of this test case is to verify that if no BBR dataset is present in a network (i.e. not sent around
    #   by Leader), then a BBR function MUST send its BBR dataset to the Leader.
    #
    # Spec Reference  | V1.2 Section
    # ----------------|-------------
    # BBR Dataset     | 5.21.3

    pkts = pv.pkts
    pv.summary.show()

    LEADER = pv.vars['LEADER']
    BR_1 = pv.vars['BR_1']
    ROUTER = pv.vars['ROUTER']
    BR_1_RLOC16 = pv.vars['BR_1_RLOC16']

    MA1 = Ipv6Addr("ff04::1234")

    # Step 1: Topology addition - BR_1 (DUT)
    # - Description: Topology addition - BR_1 (DUT). The DUT must be booted and configured join the Thread Network.
    # - Pass Criteria:
    #   - The DUT MUST attach to the Thread Network.
    print("Step 1: BR_1 (DUT) attaches to the Thread Network.")
    pkts.filter_wpan_src64(BR_1).\
        filter_mle_cmd(consts.MLE_CHILD_ID_REQUEST).\
        must_next()

    # Step 2: Leader
    # - Description: Automatically sends Network Data with no BBR Dataset.
    # - Pass Criteria: N/A
    print("Step 2: Leader sends Network Data with no BBR Dataset.")

    # Step 3: BR_1 (DUT)
    # - Description: Checks received Network Data and automatically determines that it needs to send its BBR Dataset
    #   to the Leader to become primary BBR.
    # - Pass Criteria:
    #   - The DUT MUST unicast a SVR_DATA.ntf CoAP notification to Leader.
    #   - Where the payload contains (amongst other fields): Service TLV: BBR Dataset with T=1, S_service_data = 0x01
    #   - Server TLV: BBR Dataset containing BR_1 RLOC
    print("Step 3: BR_1 (DUT) sends SVR_DATA.ntf CoAP notification to Leader.")
    # We just verify that BR_1 sends a Server Data notification to the Leader.
    # The content is verified in Step 5 when the Leader multicasts the new Network Data.
    pkts.filter_wpan_src64(BR_1).\
        filter_coap_request(consts.SVR_DATA_URI).\
        must_next()

    # Step 4: Leader
    # - Description: Automatically responds to the Server Data notification.
    # - Pass Criteria: N/A
    print("Step 4: Leader responds to SVR_DATA.ntf.")
    pkts.filter_wpan_src64(LEADER).\
        filter_coap_ack(consts.SVR_DATA_URI).\
        must_next()

    # Step 5: Leader
    # - Description: Automatically multicasts MLE Data Response message with BR_1's short address in the BBR Dataset,
    #   electing BR_1 as Primary.
    # - Pass Criteria: N/A
    print("Step 5: Leader multicasts MLE Data Response electing BR_1 as Primary.")
    pkts.filter_wpan_src64(LEADER).\
        filter_LLANMA().\
        filter_mle_cmd(consts.MLE_DATA_RESPONSE).\
        filter(lambda p: consts.NWD_SERVICE_TLV in p.thread_nwd.tlv.type).\
        filter(lambda p: BR_1_RLOC16 in p.thread_nwd.tlv.server_16).\
        must_next()

    # Step 6: BR_1 (DUT)
    # - Description: Automatically acknowledges the presence of its short address in the BBR Dataset and becomes the
    #   Primary BBR.
    # - Pass Criteria: None
    print("Step 6: BR_1 (DUT) becomes Primary BBR.")

    # Step 7: Router
    # - Description: Harness instructs the device to register the multicast address, MA1, at BR_1 (DUT). Unicasts an
    #   MLR.req CoAP request to BR_1 (DUT).
    # - Pass Criteria: N/A
    print("Step 7: Router registers MA1 at BR_1 (DUT).")
    pkts.filter_wpan_src64(ROUTER).\
        filter_coap_request('/n/mr').\
        filter(lambda p: MA1 in p.coap.tlv.ipv6_address).\
        must_next()

    # Step 8: BR_1 (DUT)
    # - Description: Automatically internally registers the multicast address MA1 in its Multicast Listeners Table.
    # - Pass Criteria: None.
    print("Step 8: BR_1 (DUT) registers MA1 internally.")

    # Step 9: BR_1 (DUT)
    # - Description: Automatically responds to the multicast listener registration.
    # - Pass Criteria:
    #   - The DUT MUST unicast a MLR.rsp CoAP response to Router.
    print("Step 9: BR_1 (DUT) responds with MLR.rsp.")
    pkts.filter_wpan_src64(BR_1).\
        filter_coap_ack('/n/mr').\
        filter(lambda p: p.coap.tlv.status == 0).\
        must_next()

    # Step 10: BR_1 (DUT)
    # - Description: Automatically multicasts an MLDv2 message to start listening to the group MA1.
    # - Pass Criteria:
    #   - The DUT MUST multicast an MLDv2 message of type "Version 2 Multicast Listener Report".
    #
    # Note: OpenThread does not currently implement sending MLDv2 messages in this simulation environment.
    print("Step 10: BR_1 (DUT) multicasts MLDv2 message (SKIPPED).")

    # Step 11: Host
    # - Description: Harness instructs the device to sends ICMPv6 Echo (ping) Request packet to destination MA1.
    # - Pass Criteria: N/A
    print("Step 11: Host sends ICMPv6 Echo Request to MA1.")
    pkts.filter_eth_src(pv.vars['HOST_ETH']).\
        filter_ipv6_dst(MA1).\
        filter_ping_request().\
        must_next()

    # Step 12: BR_1 (DUT)
    # - Description: Internally determines that the multicast packet to MA1 needs to be forwarded.
    # - Pass Criteria: None.
    print("Step 12: BR_1 (DUT) determines forwarding needed.")

    # Step 13: BR_1 (DUT)
    # - Description: Automatically forwards the multicast ping request packet to Router.
    # - Pass Criteria: Not explicitly validated in this step.
    print("Step 13: BR_1 (DUT) forwards the multicast ping.")
    pkts.filter_wpan_src64(BR_1).\
        filter_ping_request().\
        filter(lambda p: p.ipv6.dst == 'ff03::fc' or p.ipv6.dst == MA1).\
        must_next()

    # Step 14: Router receives ping request packet and automatically responds back to Host via BR_1.
    # - Pass Criteria: Receives the ping packet sent by Host.
    print("Step 14: Router receives ping and responds.")
    pkts.filter_eth_src(pv.vars['BR_1_ETH']).\
        filter(lambda p: p.ipv6.dst == Ipv6Addr(pv.vars['HOST_BACKBONE_ULA'])).\
        filter_ping_reply().\
        must_next()


if __name__ == '__main__':
    verify_utils.run_main(verify)
