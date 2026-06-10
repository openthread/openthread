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
#  ARISING IN ANY WAY OUT OF THE USE of THIS SOFTWARE, EVEN IF ADVISED OF THE
#  POSSIBILITY OF SUCH DAMAGE.
#

import sys
import os

# Add the current directory to sys.path to find verify_utils
CUR_DIR = os.path.dirname(os.path.abspath(__file__))
sys.path.append(CUR_DIR)

import verify_utils
from pktverify import consts

# Identifiers for ICMPv6 Echo Requests
ECHO_REQUEST_IDENTIFIER_1 = 0x1234
ECHO_REQUEST_IDENTIFIER_2 = 0x5678
ECHO_REQUEST_IDENTIFIER_3 = 0x90ab


def verify(pv):
    # 5.3.4 SSED modifies CSL IE
    #
    # 5.3.4.1 Topology
    # - Leader (DUT)
    # - Router_1
    # - SSED_1
    #
    # 5.3.4.2 Purpose & Description
    # The purpose of this test is to validate that a Router is able to successfully maintain a robust CSL connection
    #   with a SSED even when the SSED modifies the CSL IEs during its synchronization.
    #
    # Spec Reference | Section
    # ---------------|--------
    # CSL            | 4.10

    pkts = pv.pkts
    pv.summary.show()

    LEADER_NAME = pv.vars['LEADER']
    LEADER_RLOC16 = pv.vars['LEADER_RLOC16']
    ROUTER_1_NAME = pv.vars['ROUTER_1']
    ROUTER_1_RLOC16 = pv.vars['ROUTER_1_RLOC16']
    SSED_1_NAME = pv.vars['SSED_1']
    SSED_1_RLOC16 = pv.vars['SSED_1_RLOC16']

    def _verify_csl_echo_exchange(checkpoint_index, expected_csl_period=None, identifier=None):
        """Verifies a CSL-based ICMPv6 echo request/reply exchange."""

        # The ICMPv6 Echo Request from Router_1 to the DUT (Leader).
        _router_echo_req = pkts.range(checkpoint_index).filter_ping_request(identifier=identifier).\
            filter_wpan_src16(ROUTER_1_RLOC16).\
            filter_wpan_dst16(LEADER_RLOC16).\
            must_next()
        _router_echo_req_index = pkts.index

        # The DUT MUST buffer the ICMPv6 Echo Request frame and relay it to SSED_1.
        _echo_req = pkts.range(pkts.index).filter_ping_request(identifier=identifier).\
            filter_wpan_src16(LEADER_RLOC16).\
            filter_wpan_dst16(SSED_1_RLOC16).\
            must_next()
        _echo_req_index = pkts.index

        # SSED_1 MUST NOT send a MAC Data Request prior to receiving the ICMPv6 Echo Request from the DUT.
        # We check from the moment the Echo Request was sent by Router_1.
        pkts.range(_router_echo_req_index, (pkts.index[0] - 1, pkts.index[1] - 1)).\
            filter_wpan_src64(SSED_1_NAME).\
            filter_wpan_dst16(LEADER_RLOC16).\
            filter_wpan_cmd(consts.WPAN_DATA_REQUEST).\
            must_not_next()

        # SSED_1 MUST send an ICMPv6 Echo Reply to Router_1 (relayed by DUT).
        _echo_reply = pkts.range(_echo_req_index).filter_ping_reply(identifier=_echo_req.icmpv6.echo.identifier).\
            filter_wpan_src64(SSED_1_NAME).\
            filter_wpan_dst16(LEADER_RLOC16).\
            must_next()

        if expected_csl_period is not None:
            # SSED_1 SHOULD include the CSL IE with the expected period.
            if _echo_reply.wpan.header_ie.csl.period != expected_csl_period:
                raise ValueError(
                    f"Expected CSL period {expected_csl_period}, but found {_echo_reply.wpan.header_ie.csl.period}")

        # The DUT MUST forward the Echo Reply to Router_1.
        pkts.range(pkts.index).filter_ping_reply(identifier=_echo_req.icmpv6.echo.identifier).\
            filter_wpan_src16(LEADER_RLOC16).\
            filter_wpan_dst16(ROUTER_1_RLOC16).\
            must_next()

        return pkts.index

    # Step 0: SSED_1
    # - Description: Preconditions: Set CSL Synchronized Timeout = 20s, Set CSL Period = 500ms.
    # - Pass Criteria: N/A.
    print("Step 0: SSED_1")

    # Step 1: All
    # - Description: Topology formation: DUT, Router_1, SSED_1.
    # - Pass Criteria: N/A.
    print("Step 1: All")

    # Step 2: SSED_1
    # - Description: Automatically attaches to the DUT and establishes CSL synchronization.
    # - Pass Criteria:
    #   - 2.1: The DUT MUST send MLE Child ID Response to SSED_1.
    #   - 2.2: The DUT MUST unicast MLE Child Update Response to SSED_1.
    print("Step 2: SSED_1")

    # 2.1: The DUT MUST send MLE Child ID Response to SSED_1.
    pkts.filter_wpan_src64(LEADER_NAME).\
        filter_wpan_dst64(SSED_1_NAME).\
        filter_mle_cmd(consts.MLE_CHILD_ID_RESPONSE).\
        must_next()

    # 2.2: The DUT MUST unicast MLE Child Update Response to SSED_1.
    pkts.filter_wpan_src64(LEADER_NAME).\
        filter_wpan_dst64(SSED_1_NAME).\
        filter_mle_cmd(consts.MLE_CHILD_UPDATE_RESPONSE).\
        must_next()
    _checkpoint_step2 = pkts.index

    # Step 3: Router_1
    # - Description: Harness verifies connectivity by sending an ICMPv6 Echo Request to the SSED_1 mesh-local address.
    # - Pass Criteria:
    #   - 3.1: The DUT MUST buffer the ICMPv6 Echo Request frame and relay it to SSED_1 within a subsequent CSL slot.
    #   - 3.2: SSED_1 MUST NOT send a MAC Data Request prior to receiving the ICMPv6 Echo Request from the DUT.
    print("Step 3: Router_1")

    # Step 4: SSED_1
    # - Description: Automatically sends ICMPv6 Echo Reply to the Router.
    # - Pass Criteria:
    #   - 4.1: SSED_1 MUST send an ICMPv6 Echo Reply to Router_1.
    print("Step 4: SSED_1")

    _last_index = _verify_csl_echo_exchange(_checkpoint_step2, verify_utils.CSL_PERIOD_500MS,
                                            ECHO_REQUEST_IDENTIFIER_1)

    # Step 5: SSED_1
    # - Description: Harness instructs the device to change its CSL Period IE to 3300ms.
    # - Pass Criteria: N/A.
    print("Step 5: SSED_1")

    # SSED_1 SHOULD inform the parent about the CSL Period change.
    pkts.range(_last_index).filter_wpan_src64(SSED_1_NAME).\
        filter_wpan_dst64(LEADER_NAME).\
        filter_mle_cmd(consts.MLE_CHILD_UPDATE_REQUEST).\
        must_next()

    # The DUT MUST respond to the MLE Child Update Request.
    pkts.filter_wpan_src64(LEADER_NAME).\
        filter_wpan_dst64(SSED_1_NAME).\
        filter_mle_cmd(consts.MLE_CHILD_UPDATE_RESPONSE).\
        must_next()
    _checkpoint_step7 = pkts.index

    # Step 6: Harness
    # - Description: Harness waits for 15 seconds.
    # - Pass Criteria: N/A.
    print("Step 6: Harness")

    # Step 7: Router_1
    # - Description: Harness verifies connectivity by instructing the device to send an ICMPv6 Echo Request to the
    #   SSED_1 mesh-local address.
    # - Pass Criteria:
    #   - 7.1: The DUT MUST buffer the ICMPv6 Echo Request frame and relay it to SSED_1 within a subsequent CSL slot.
    #   - 7.2: SSED_1 MUST NOT send a MAC Data Request prior to receiving the ICMPv6 Echo Request from the DUT.
    print("Step 7: Router_1")

    # Step 8: SSED_1
    # - Description: Automatically sends ICMPv6 Echo Reply to the Router.
    # - Pass Criteria:
    #   - 8.1: SSED_1 MUST send an ICMPv6 Echo Reply to Router_1.
    print("Step 8: SSED_1")

    _last_index = _verify_csl_echo_exchange(_checkpoint_step7, verify_utils.CSL_PERIOD_3300MS,
                                            ECHO_REQUEST_IDENTIFIER_2)

    # Step 9: SSED_1
    # - Description: Harness instructs the device to change its CSL Period IE to 400ms.
    # - Pass Criteria: N/A.
    print("Step 9: SSED_1")

    # SSED_1 SHOULD inform the parent about the CSL Period change.
    pkts.range(_last_index).filter_wpan_src64(SSED_1_NAME).\
        filter_wpan_dst64(LEADER_NAME).\
        filter_mle_cmd(consts.MLE_CHILD_UPDATE_REQUEST).\
        must_next()

    # The DUT MUST respond to the MLE Child Update Request.
    pkts.filter_wpan_src64(LEADER_NAME).\
        filter_wpan_dst64(SSED_1_NAME).\
        filter_mle_cmd(consts.MLE_CHILD_UPDATE_RESPONSE).\
        must_next()
    _checkpoint_step11 = pkts.index

    # Step 10: Harness
    # - Description: Harness waits for 18 seconds.
    # - Pass Criteria: N/A.
    print("Step 10: Harness")

    # Step 11: Router_1
    # - Description: Harness verifies connectivity by instructing the device to send an ICMPv6 Echo Request to SSED_1
    #   mesh-local address.
    # - Pass Criteria:
    #   - 11.1: The DUT MUST buffer the ICMPv6 Echo Request frame and relay it to SSED_1 within a subsequent CSL slot.
    #   - 11.2: SSED_1 MUST NOT send a MAC Data Request prior to receiving the ICMPv6 Echo Request from the DUT.
    print("Step 11: Router_1")

    # Step 12: SSED_1
    # - Description: Automatically sends ICMPv6 Echo Reply to the Router.
    # - Pass Criteria:
    #   - 12.1: SSED_1 MUST send an ICMPv6 Echo Reply to Router_1.
    print("Step 12: SSED_1")

    _verify_csl_echo_exchange(_checkpoint_step11, verify_utils.CSL_PERIOD_400MS, ECHO_REQUEST_IDENTIFIER_3)


if __name__ == '__main__':
    verify_utils.run_main(verify)
