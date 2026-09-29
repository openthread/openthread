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
    # 6.2.1 Connectivity when Parent Creates Partition
    #
    # 6.2.1.1 Topology
    #   Topology A: DUT as End Device (ED_1) attached to Router_1.
    #   Topology B: DUT as Sleepy End Device (SED_1) attached to Router_1.
    #   Leader: Connected to Router_1.
    #
    # 6.2.1.2 Purpose & Description
    #   The purpose of this test case is to show that the DUT upholds connectivity, or reattaches with its parent, when
    #   the Leader is removed and the Router creates a new partition.
    #
    # Spec Reference   | V1.1 Section | V1.3.0 Section
    # -----------------|--------------|---------------
    # Children         | 5.16.6       | 5.16.6

    pkts = pv.pkts
    pv.summary.show()

    LEADER = pv.vars['LEADER']
    ROUTER_1 = pv.vars['ROUTER_1']

    if 'ED_1' in pv.vars:
        dut = pv.vars['ED_1']
    elif 'SED_1' in pv.vars:
        dut = pv.vars['SED_1']
    else:
        raise ValueError("Neither ED_1 nor SED_1 found in topology vars")

    # Step 1: All
    # - Description: Ensure topology is formed correctly.
    # - Pass Criteria: N/A
    print("Step 1: All")

    # Leader Advertisement
    pkts.filter_wpan_src64(LEADER).\
        filter_LLANMA().\
        filter_mle_cmd(consts.MLE_ADVERTISEMENT).\
        must_next()

    # Router 1 Advertisement (pointing to Leader)
    _pkt = pkts.filter_wpan_src64(ROUTER_1).\
        filter_LLANMA().\
        filter_mle_cmd(consts.MLE_ADVERTISEMENT).\
        must_next()
    leader_partition_id = _pkt.mle.tlv.leader_data.partition_id

    # DUT attached (Child ID Request)
    pkts.filter_wpan_src64(dut).\
        filter_wpan_dst64(ROUTER_1).\
        filter_mle_cmd(consts.MLE_CHILD_ID_REQUEST).\
        must_next()

    # Step 2: Leader
    # - Description: Harness silently powers-down the Leader.
    # - Pass Criteria: N/A
    print("Step 2: Leader")

    # Step 3: Router_1
    # - Description: Automatically creates new partition and begins transmitting MLE Advertisements.
    # - Pass Criteria: N/A
    print("Step 3: Router_1")

    # Router 1 Advertisement with new Partition ID
    pkts.filter_wpan_src64(ROUTER_1).\
        filter_LLANMA().\
        filter_mle_cmd(consts.MLE_ADVERTISEMENT).\
        filter(lambda p: p.mle.tlv.leader_data.partition_id != leader_partition_id).\
        must_next()

    # Step 4: MED_1 / SED_1 (DUT)
    # - Description: Automatically remains attached or reattaches to Router_1.
    # - Pass Criteria: N/A
    print("Step 4: MED_1 / SED_1 (DUT)")

    # Step 5: Router_1
    # - Description: To verify connectivity, Harness instructs the device to send an ICMPv6 Echo Request to the DUT
    #   link local address.
    # - Pass Criteria:
    #   - The DUT MUST respond with ICMPv6 Echo Reply.
    print("Step 5: Router_1")

    _pkt = pkts.filter_ping_request().\
        filter_wpan_src64(ROUTER_1).\
        filter_wpan_dst64(dut).\
        must_next()

    pkts.filter_ping_reply(identifier=_pkt.icmpv6.echo.identifier).\
        filter_wpan_src64(dut).\
        filter_wpan_dst64(ROUTER_1).\
        must_next()


if __name__ == '__main__':
    verify_utils.run_main(verify)
