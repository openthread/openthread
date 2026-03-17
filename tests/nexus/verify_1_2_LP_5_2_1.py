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
    # 5.2.1 Enhanced Frame Pending with Thread V1.2 End Device & V1.1 End Device
    #
    # 5.2.1.1 Topology
    # - SED_1: is Thread 1.1 SED.
    # - SED_2: is Thread 1.2 SED with Enhanced Frame Pending (EFP) support; polling via 802.15.4 MAC Data Requests is
    #   disabled.
    # - SED_3: is Thread 1.2 SED with Enhanced Frame Pending (EFP) support; polling via 802.15.4 MAC Data Requests is
    #   enabled.
    # - Router_1: is a Thread Router.
    # - DUT: is Thread Leader.
    #
    # 5.2.1.2 Purpose & Description
    # This test case verifies that the DUT correctly manages the Frame Pending bit in acknowledgments to MAC Data and
    #   Data Request frames for Thread V1.2 and V1.1 sleepy end devices.
    #
    # Spec Reference          | V1.2 Section
    # ------------------------|--------------
    # Enhanced Frame Pending  | 3.2.6.2

    FRAME_PENDING_SET = 1
    FRAME_PENDING_CLEAR = 0

    pkts = pv.pkts
    pv.summary.show()

    LEADER = pv.vars['LEADER']
    ROUTER_1 = pv.vars['ROUTER_1']
    SED_1 = pv.vars['SED_1']
    SED_2 = pv.vars['SED_2']
    SED_3 = pv.vars['SED_3']

    LEADER_RLOC16 = pv.vars['LEADER_RLOC16']
    ROUTER_1_RLOC16 = pv.vars['ROUTER_1_RLOC16']
    SED_1_RLOC16 = pv.vars['SED_1_RLOC16']
    SED_2_RLOC16 = pv.vars['SED_2_RLOC16']
    SED_3_RLOC16 = pv.vars['SED_3_RLOC16']

    def is_from(p, addr, rloc16):
        return p.wpan.src64 == addr or p.wpan.src16 == rloc16

    def is_to(p, addr, rloc16):
        return p.wpan.dst16 == rloc16 or p.wpan.dst64 == addr

    def _verify_ping_echo_for_sed(sed, sed_rloc16):
        """Helper to verify ping echo from ROUTER_1 to a SED."""
        _echo_req = pkts.filter_ping_request().\
            filter(lambda p: is_from(p, ROUTER_1, ROUTER_1_RLOC16)).\
            filter(lambda p: is_to(p, LEADER, LEADER_RLOC16)).\
            must_next()

        pkts.filter_ping_reply(identifier=_echo_req.icmpv6.echo.identifier).\
            filter(lambda p: is_from(p, sed, sed_rloc16)).\
            filter(lambda p: is_to(p, LEADER, LEADER_RLOC16)).\
            must_next()
        pkts.filter_ping_reply(identifier=_echo_req.icmpv6.echo.identifier).\
            filter(lambda p: is_from(p, LEADER, LEADER_RLOC16)).\
            filter(lambda p: is_to(p, ROUTER_1, ROUTER_1_RLOC16)).\
            must_next()

    # Step 1: All Devices
    # - Topology formation.
    print("Step 1: Topology formation")

    # Step 2: SED_2
    # - Harness instructs the device to send an empty MAC Data frame to the DUT (Leader).
    # - Pass Criteria: DUT MUST send an acknowledgment with Frame Pending bit set to 0.
    print("Step 2: SED_2 sends empty MAC Data frame to Leader")
    _pkt = pkts.filter_wpan_data().\
        filter(lambda p: is_from(p, SED_2, SED_2_RLOC16)).\
        filter(lambda p: is_to(p, LEADER, LEADER_RLOC16)).\
        filter(lambda p: 'ipv6' not in p.layer_names).\
        must_next()

    pkts.filter_wpan_ack().\
        filter_wpan_seq(_pkt.wpan.seq_no).\
        must_next().\
        must_verify(lambda p: p.wpan.pending == FRAME_PENDING_CLEAR)

    # Step 3: Router_1
    # - Harness instructs the device to send an ICMPv6 Echo Request to SED_2.
    # - Pass Criteria: N/A.
    print("Step 3: Router_1 sends Echo Request to SED_2")
    _echo_req = pkts.filter_ping_request().\
        filter(lambda p: is_from(p, ROUTER_1, ROUTER_1_RLOC16)).\
        filter(lambda p: is_to(p, LEADER, LEADER_RLOC16)).\
        must_next()

    # Step 4: SED_2
    # - Harness instructs the device to send an empty MAC Data frame to the DUT (Leader).
    # - Pass Criteria: DUT MUST send an acknowledgment with Frame Pending bit set to 1.
    print("Step 4: SED_2 sends empty MAC Data frame to Leader")
    _pkt = pkts.filter_wpan_data().\
        filter(lambda p: is_from(p, SED_2, SED_2_RLOC16)).\
        filter(lambda p: is_to(p, LEADER, LEADER_RLOC16)).\
        filter(lambda p: 'ipv6' not in p.layer_names).\
        must_next()

    pkts.filter_wpan_ack().\
        filter_wpan_seq(_pkt.wpan.seq_no).\
        must_next().\
        must_verify(lambda p: p.wpan.pending == FRAME_PENDING_SET)

    # Step 5: SED_2
    # - Harness instructs the device to send an 802.15.4 Data Request message to the DUT (Leader).
    # - Pass Criteria: N/A.
    print("Step 5: SED_2 sends Data Request to Leader")
    _pkt = pkts.filter_wpan_cmd(consts.WPAN_DATA_REQUEST).\
        filter(lambda p: is_from(p, SED_2, SED_2_RLOC16)).\
        filter(lambda p: is_to(p, LEADER, LEADER_RLOC16)).\
        must_next()

    # Step 6: DUT
    # - DUT sends an acknowledgment with Frame Pending bit set to 1.
    # - Pass Criteria: N/A.
    print("Step 6: DUT sends acknowledgment with Frame Pending bit set to 1")
    pkts.filter_wpan_ack().\
        filter_wpan_seq(_pkt.wpan.seq_no).\
        must_next().\
        must_verify(lambda p: p.wpan.pending == FRAME_PENDING_SET)

    # Step 7: DUT
    # Step 7: DUT forwards Echo Request to SED_2
    # - Pass Criteria: N/A.
    print("Step 7: DUT forwards Echo Request to SED_2")
    _pkt = pkts.filter_ping_request(identifier=_echo_req.icmpv6.echo.identifier).\
        filter(lambda p: is_from(p, LEADER, LEADER_RLOC16)).\
        filter(lambda p: is_to(p, SED_2, SED_2_RLOC16)).\
        must_next()

    # Step 8: SED_2
    # - Acknowledges the ICMPv6 Echo Request frame.
    # - Pass Criteria: SED_2 MUST send 802.15.4 ACK to the DUT.
    print("Step 8: SED_2 acknowledges Echo Request")
    pkts.filter_wpan_ack().\
        filter_wpan_seq(_pkt.wpan.seq_no).\
        must_next()

    # Step 9: SED_2
    # Step 9: SED_2 sends Echo Reply to Router_1
    # - Pass Criteria: Router_1 MUST receive the ICMPv6 Echo Reply.
    print("Step 9: SED_2 sends Echo Reply to Router_1")
    _reply1 = pkts.filter_ping_reply(identifier=_echo_req.icmpv6.echo.identifier).\
        filter(lambda p: is_from(p, SED_2, SED_2_RLOC16)).\
        filter(lambda p: is_to(p, LEADER, LEADER_RLOC16)).\
        must_next()

    # Step 10: DUT sends acknowledgment with Frame Pending bit set to 0 to SED_2
    # - Pass Criteria: The DUT MUST transmit a 802.15.4 ACK to SED_2 with the Frame Pending bit set to 0.
    print("Step 10: DUT sends acknowledgment with Frame Pending bit set to 0 to SED_2")
    pkts.filter_wpan_ack().\
        filter_wpan_seq(_reply1.wpan.seq_no).\
        must_next().\
        must_verify(lambda p: p.wpan.pending == FRAME_PENDING_CLEAR)

    # Continue Step 9 verification: verify the forwarded Echo Reply to Router_1
    pkts.filter_ping_reply(identifier=_echo_req.icmpv6.echo.identifier).\
        filter(lambda p: is_from(p, LEADER, LEADER_RLOC16)).\
        filter(lambda p: is_to(p, ROUTER_1, ROUTER_1_RLOC16)).\
        must_next()

    # Step 11: Router_1
    # - Harness instructs the device to send an ICMPv6 Echo Request to SED_3.
    # - Pass Criteria: N/A.
    print("Step 11: Router_1 sends Echo Request to SED_3")

    # Step 12: SED_3
    # - Automatically replies with an ICMPv6 Echo Reply to Router_1.
    # - Pass Criteria: Router_1 MUST receive the Echo Reply.
    print("Step 12: SED_3 sends Echo Reply to Router_1")
    _verify_ping_echo_for_sed(SED_3, SED_3_RLOC16)

    # Step 13: Router_1
    # - Harness instructs the device to send an ICMPv6 Echo Request to SED_1.
    # - Pass Criteria: N/A.
    print("Step 13: Router_1 sends Echo Request to SED_1")

    # Step 14: SED_1
    # - Automatically replies with an ICMPv6 Echo Reply to Router_1.
    # - Pass Criteria: Router_1 MUST receive the Echo Reply.
    print("Step 14: SED_1 sends Echo Reply to Router_1")
    _verify_ping_echo_for_sed(SED_1, SED_1_RLOC16)


if __name__ == '__main__':
    verify_utils.run_main(verify)
