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
    # 6.6.2 Key Increment of 1 with Roll-over
    #
    # 6.6.2.1 Topology
    # - Topology A: DUT as End Device (ED_1)
    # - Topology B: DUT as Sleepy End Device (SED_1)
    # - Leader
    #
    # 6.6.2.2 Purpose & Description
    # The purpose of this test case is to verify that the DUT properly decrypts MAC and MLE packets secured with a Key
    #   Index incremented by 1 (which causes a rollover) and switches to the new key.
    #
    # Spec Reference                  | V1.1 Section | V1.3.0 Section
    # --------------------------------|--------------|---------------
    # MLE Message Security Processing | 7.3.1        | 7.3.1

    pkts = pv.pkts
    pv.summary.show()

    LEADER = pv.vars['LEADER']

    if 'ED_1' in pv.vars:
        dut = pv.vars['ED_1']
        name = 'ED_1'
    elif 'SED_1' in pv.vars:
        dut = pv.vars['SED_1']
        name = 'SED_1'
    else:
        raise ValueError("Neither ED_1 nor SED_1 found in topology vars")

    # Step 1: Leader
    # - Description: Harness instructs the device to form the network using thrKeySequenceCounter = 0x7F (127).
    # - Pass Criteria: N/A
    print("Step 1: Leader forms the network using thrKeySequenceCounter = 0x7F (127).")

    # Step 2: ED_1 / SED_1 (DUT)
    # - Description: Attach the DUT to the network.
    # - Pass Criteria:
    #   - The MLE Auxiliary Security Header of the MLE Child ID Request MUST contain:
    #     - Key Source = 0x7F (127)
    #     - Key Index = 0x80 (128)
    #     - Key ID Mode = 2
    print(f"Step 2: {name} (DUT) sends a MLE Child ID Request.")

    pkts.filter_wpan_src64(dut).\
        filter_wpan_dst64(LEADER).\
        filter_mle_cmd(consts.MLE_CHILD_ID_REQUEST).\
        filter(lambda p: p.wpan.aux_sec.key_id_mode == 2).\
        filter(lambda p: p.wpan.aux_sec.key_index == 128).\
        filter(lambda p: p.wpan.aux_sec.key_source == 127).\
        must_next()

    # Step 3: Leader
    # - Description: Harness instructs the device to send an ICMPv6 Echo Request to the DUT. The MAC Auxiliary security
    #   header contains:
    #   - Key Index = 0x80 (128)
    #   - Key ID Mode = 1
    # - Pass Criteria: N/A
    print("Step 3: Leader sends an ICMPv6 Echo Request to the DUT.")

    pkts.filter_ping_request().\
        filter_wpan_src64(LEADER).\
        filter_wpan_dst64(dut).\
        filter(lambda p: p.wpan.aux_sec.key_id_mode == 1).\
        filter(lambda p: p.wpan.aux_sec.key_index == 128).\
        must_next()

    # Step 4: ED_1 / SED_1 (DUT)
    # - Description: Automatically replies with ICMPv6 Echo Reply.
    # - Pass Criteria:
    #   - The DUT MUST reply with ICMPv6 Echo Reply.
    #   - The MAC Auxiliary Security Header MUST contain:
    #     - Key Index = 0x80 (128)
    #     - Key ID Mode = 1
    print(f"Step 4: {name} (DUT) MUST reply with ICMPv6 Echo Reply.")

    pkts.filter_ping_reply().\
        filter_wpan_src64(dut).\
        filter_wpan_dst64(LEADER).\
        filter(lambda p: p.wpan.aux_sec.key_id_mode == 1).\
        filter(lambda p: p.wpan.aux_sec.key_index == 128).\
        must_next()

    # Step 5: Leader
    # - Description: Harness instructs the device to increment thrKeySequenceCounter by 1 to force a key switch.
    #   Incoming frame counters shall be set to 0 for all existing devices. All subsequent MLE and MAC frames are sent
    #   with Key Index = 1.
    print("Step 5: Leader increments thrKeySequenceCounter by 1 to force a key switch.")

    # Step 6: Leader
    # - Description: Harness instructs the device to send an ICMPv6 Echo Request to the DUT. The MAC Auxiliary Security
    #   Header contains:
    #   - Key Index = 1
    #   - Key ID Mode = 1
    # - Pass Criteria: N/A
    print("Step 6: Leader sends an ICMPv6 Echo Request to the DUT.")

    pkts.filter_ping_request().\
        filter_wpan_src64(LEADER).\
        filter_wpan_dst64(dut).\
        filter(lambda p: p.wpan.aux_sec.key_id_mode == 1).\
        filter(lambda p: p.wpan.aux_sec.key_index == 1).\
        must_next()

    # Step 7: ED_1 / SED_1 (DUT)
    # - Description: Automatically replies with ICMPv6 Echo Reply.
    # - Pass Criteria:
    #   - The DUT MUST reply with ICMPv6 Echo Reply.
    #   - The MAC Auxiliary Security Header MUST contain:
    #     - Key Index = 1
    #     - Key ID Mode = 1
    print(f"Step 7: {name} (DUT) MUST reply with ICMPv6 Echo Reply.")

    pkts.filter_ping_reply().\
        filter_wpan_src64(dut).\
        filter_wpan_dst64(LEADER).\
        filter(lambda p: p.wpan.aux_sec.key_id_mode == 1).\
        filter(lambda p: p.wpan.aux_sec.key_index == 1).\
        must_next()


if __name__ == '__main__':
    verify_utils.run_main(verify)
