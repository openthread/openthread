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
    # 6.3.1 Orphan Reattach
    #
    # 6.3.1.1 Topology
    #   Topology A: DUT as End Device (ED_1)
    #   Topology B: DUT as Sleepy End Device (SED_1)
    #   Leader
    #   Router_1
    #
    # 6.3.1.2 Purpose & Description
    #   The purpose of this test case is to show that the DUT will attach to the Leader once its parent is removed from
    #     the network.
    #
    # Spec Reference        | V1.1 Section | V1.3.0 Section
    # ----------------------|--------------|---------------
    # Child Update Messages | 4.7.3        | 4.6.1

    pkts = pv.pkts
    pv.summary.show()

    LEADER = pv.vars['LEADER']
    ROUTER_1 = pv.vars['ROUTER_1']

    IS_TOPOLOGY_A = 'ED_1' in pv.vars
    if IS_TOPOLOGY_A:
        DUT = pv.vars['ED_1']
    else:
        DUT = pv.vars['SED_1']

    # Step 1: All
    #   Description: Setup the topology without the DUT. Ensure all routers and leader are sending MLE advertisements.
    #   Pass Criteria: N/A
    print("Step 1: Setup the topology without the DUT.")

    # Step 2: Router_1
    #   Description: Harness silently removes Router_1 from the network.
    #   Pass Criteria: N/A
    print("Step 2: Harness silently removes Router_1 from the network.")

    if IS_TOPOLOGY_A:
        # Step 3: ED_1 (DUT) [Topology A only]
        #   Description: Automatically sends MLE Child Update Request keep-alive message(s) to its parent. [Optional]
        #     The DUT SHOULD send MLE Child Update Requests [FAILED_CHILD_TRANSMISSIONS-1] to its parent (Router_1).
        #   Pass Criteria:
        #     The following TLVs MUST be included in each MLE Child Update Request:
        #       Source Address TLV
        #       Leader Data TLV
        #       Mode TLV
        #       Address Registration TLVs (optional)
        print("Step 3: ED_1 (DUT) sends MLE Child Update Request keep-alive message(s) to its parent. [Optional]")
        # We try to find the packet, but don't fail if it's not there as it is optional.
        _pkt = pkts.filter_wpan_src64(DUT).\
            filter_mle_cmd(consts.MLE_CHILD_UPDATE_REQUEST).\
            next()
        if _pkt:
            _pkt.filter(lambda p: {
                                   consts.SOURCE_ADDRESS_TLV,
                                   consts.LEADER_DATA_TLV,
                                   consts.MODE_TLV
                                   } <= set(p.mle.tlv.type)).\
                must_verify()
    else:
        # Step 4: SED_1 (DUT) [Topology B only]
        #   Description: Automatically sends 802.15.4 Data Request keep-alive message(s) to its parent. [Optional] The
        #     DUT SHOULD send 802.15.4 Data Request commands [FAILED_CHILD_TRANSMISSIONS-1] to its parent (Router_1).
        #   Pass Criteria:
        #     The DUT MUST NOT receive an ACK message in response.
        print("Step 4: SED_1 (DUT) sends 802.15.4 Data Request keep-alive message(s) to its parent. [Optional]")
        pkts.filter_wpan_src64(DUT).\
            filter(lambda p: p.wpan.cmd == consts.WPAN_DATA_REQUEST).\
            next()

    # Step 5: ED_1 / SED_1 (DUT)
    #   Description: Automatically attaches to the Leader.
    #   Pass Criteria:
    #     The DUT MUST perform the attach procedure with the Leader (see section 6.1.1 Attaching to a Router).
    print("Step 5: Automatically attaches to the Leader.")
    # Parent Request
    pkts.filter_wpan_src64(DUT).\
        filter_LLARMA().\
        filter_mle_cmd(consts.MLE_PARENT_REQUEST).\
        must_next()
    # Parent Response from Leader
    pkts.filter_wpan_src64(LEADER).\
        filter_wpan_dst64(DUT).\
        filter_mle_cmd(consts.MLE_PARENT_RESPONSE).\
        must_next()
    # Child ID Request
    pkts.filter_wpan_src64(DUT).\
        filter_wpan_dst64(LEADER).\
        filter_mle_cmd(consts.MLE_CHILD_ID_REQUEST).\
        must_next()
    # Child ID Response
    pkts.filter_wpan_src64(LEADER).\
        filter_wpan_dst64(DUT).\
        filter_mle_cmd(consts.MLE_CHILD_ID_RESPONSE).\
        must_next()

    # Step 6: Leader
    #   Description: Harness verifies connectivity by instructing the device to send an ICMPv6 Echo Request to the DUT
    #     link local address.
    #   Pass Criteria:
    #     The DUT MUST respond with ICMPv6 Echo Reply.
    print("Step 6: Leader verifies connectivity by sending an ICMPv6 Echo Request.")
    _pkt = pkts.filter_ping_request().\
        filter_wpan_src64(LEADER).\
        filter_wpan_dst64(DUT).\
        must_next()
    pkts.filter_ping_reply(identifier=_pkt.icmpv6.echo.identifier).\
        filter_wpan_src64(DUT).\
        filter_wpan_dst64(LEADER).\
        must_next()


if __name__ == '__main__':
    verify_utils.run_main(verify)
