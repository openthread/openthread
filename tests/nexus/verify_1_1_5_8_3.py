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
    # 5.8.3 Key Increment of 1 With Roll-over
    #
    # 5.8.3.1 Topology
    # - Leader
    # - Router_1 (DUT)
    #
    # 5.8.3.2 Purpose & Description
    # The purpose of this test case is to verify that the DUT properly decrypts MAC and MLE packets secured with a Key
    #   Index incremented by 1 (that causes a rollover) and switches to the new key.
    #
    # Spec Reference                  | V1.1 Section | V1.3.0 Section
    # --------------------------------|--------------|---------------
    # MLE Message Security Processing | 7.3.1        | 7.3.1

    pkts = pv.pkts
    pv.summary.show()

    LEADER = pv.vars['LEADER']
    ROUTER_1 = pv.vars['ROUTER_1']

    # Step 1: Leader
    # - Description: Harness instructs the device to form the network. Starts the network using KeySequenceCounter =
    #   0x7F (127).
    # - Pass Criteria: N/A
    print("Step 1: Leader forms the network using KeySequenceCounter = 0x7F (127).")

    # Step 2: Router_1 (DUT)
    # - Description: Automatically attaches to the network and sends MLE Advertisements.
    # - Pass Criteria:
    #   - The DUT MUST send MLE Advertisements with the MLE Auxiliary Security Header containing the following:
    #     - Key ID Mode = 0x02 (2)
    #     - Key Source = 0x7F (127)
    #     - Key Index = 0x80 (128)
    print("Step 2: DUT MUST send MLE Advertisements with Key ID Mode = 2, Key Source = 127, Key Index = 128.")
    pkts.filter_wpan_src64(ROUTER_1).\
        filter_LLANMA().\
        filter_mle_cmd(consts.MLE_ADVERTISEMENT).\
        filter(lambda p:
               p.wpan.aux_sec.key_id_mode == 2 and
               p.wpan.aux_sec.key_source == 127 and
               p.wpan.aux_sec.key_index == 128).\
        must_next()

    # Step 3: Leader
    # - Description: Harness instructs the device to send an ICMPv6 Echo Request to the DUT.
    # - Pass Criteria:
    #   - The DUT MUST respond with an ICMPv6 Echo Reply with MAC Auxiliary Security Header containing:
    #     - Key ID Mode = 0x01 (1)
    #     - Key Index = 0x80 (128)
    print("Step 3: DUT MUST respond with an ICMPv6 Echo Reply with Key ID Mode = 1, Key Index = 128.")
    pkts.filter_ping_reply().\
        filter_wpan_src64(ROUTER_1).\
        filter_wpan_dst64(LEADER).\
        filter(lambda p:
               p.wpan.aux_sec.key_id_mode == 1 and
               p.wpan.aux_sec.key_index == 128).\
        must_next()

    # Step 4: Leader
    # - Description: Harness instructs the device to increment KeySequenceCounter by 1 to force a key switch. The DUT
    #   is expected to set incoming frame counters to 0 for all existing devices and to send subsequent MAC and MLE
    #   frames with Key Index = 1.
    # - Pass Criteria: N/A
    print("Step 4: Harness increments KeySequenceCounter by 1 to force a key switch.")

    # Step 5: Leader
    # - Description: Harness instructs the device to send an ICMPv6 Echo Request to the DUT.
    # - Pass Criteria:
    #   - The DUT MUST respond with an ICMPv6 Echo Reply with MAC Auxiliary Security Header containing:
    #     - Key ID Mode = 0x01 (1)
    #     - Key Index = 0x01 (1)
    print("Step 5: DUT MUST respond with an ICMPv6 Echo Reply with Key ID Mode = 1, Key Index = 1.")
    pkts.filter_ping_reply().\
        filter_wpan_src64(ROUTER_1).\
        filter_wpan_dst64(LEADER).\
        filter(lambda p:
               p.wpan.aux_sec.key_id_mode == 1 and
               p.wpan.aux_sec.key_index == 1).\
        must_next()

    # Step 6: Router_1 (DUT)
    # - Description: Automatically reflects the Key Index update in its Advertisements.
    # - Pass Criteria:
    #   - The DUT MUST send MLE Advertisements with MLE Auxiliary Security Header containing:
    #     - Key ID Mode = 0x02 (2)
    #     - Key Index = 0x01 (1)
    print("Step 6: DUT MUST send MLE Advertisements with Key ID Mode = 2, Key Index = 1.")
    pkts.filter_wpan_src64(ROUTER_1).\
        filter_LLANMA().\
        filter_mle_cmd(consts.MLE_ADVERTISEMENT).\
        filter(lambda p:
               p.wpan.aux_sec.key_id_mode == 2 and
               p.wpan.aux_sec.key_source == 128 and
               p.wpan.aux_sec.key_index == 1).\
        must_next()


if __name__ == '__main__':
    verify_utils.run_main(verify)
