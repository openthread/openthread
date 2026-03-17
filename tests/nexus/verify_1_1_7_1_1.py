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
    # 7.1.1 Network data propagation - Border Router as Leader of Thread Network; correctly sends Network Data
    #   information during attach
    #
    # 7.1.1.1 Topology
    # - MED_1 is configured to require complete network data. (Mode TLV)
    # - SED_1 is configured to request only stable network data. (Mode TLV)
    #
    # 7.1.1.2 Purpose & Description
    # The purpose of this test case is to verify that the DUT, as a Border Router, acts properly as a Leader device
    #   in a Thread network, correctly sets the Network Data (stable/non-stable) and successfully propagates the
    #   Network Data to the devices that attach to it.
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

    PREFIX_1 = Ipv6Addr("2001::")
    PREFIX_2 = Ipv6Addr("2002::")

    # Step 1: Leader (DUT)
    # - Description: Forms the network.
    # - Pass Criteria: The DUT MUST properly send MLE Advertisements.
    print("Step 1: Leader (DUT) forms the network.")
    pkts.filter_wpan_src64(LEADER).\
        filter_LLANMA().\
        filter_mle_cmd(consts.MLE_ADVERTISEMENT).\
        must_next()

    # Step 2: Leader (DUT)
    # - Description: The user must configure the following On-Mesh Prefix Set on the device:
    #   - Prefix 1: P_prefix=2001::/64 P_stable=1 P_on_mesh=1 P_preferred=1 P_slaac=1 P_default=1
    #   - Prefix 2: P_prefix=2002::/64 P_stable=0 P_on_mesh=1 P_preferred=1 P_slaac=1 P_default=1
    # - Pass Criteria: The DUT MUST correctly aggregate configured information to create the Network Data (No OTA
    #   validation).
    print("Step 2: Leader (DUT) configures On-Mesh Prefixes.")

    # Step 3: Router_1
    # - Description: Harness instructs device to join the network; it requests complete network data.
    # - Pass Criteria: N/A
    print("Step 3: Router_1 joins the network.")
    pkts.filter_wpan_src64(ROUTER_1).\
        filter_mle_cmd(consts.MLE_CHILD_ID_REQUEST).\
        filter(lambda p: consts.NETWORK_DATA_TLV in p.mle.tlv.type).\
        must_next()

    # Step 4: Leader (DUT)
    # - Description: Automatically sends the requested network data to Router_1.
    # - Pass Criteria:
    #   - The DUT MUST send a MLE Child ID Response to Router_1, including the following TLVs:
    #     - Network Data TLV
    #       - At least two Prefix TLVs (Prefix 1 and Prefix 2), each including:
    #         - 6LoWPAN ID sub-TLV
    #         - Border Router sub-TLV
    print("Step 4: Leader (DUT) automatically sends requested network data to Router_1.")
    pkts.filter_wpan_src64(LEADER).\
        filter_wpan_dst64(ROUTER_1).\
        filter_mle_cmd(consts.MLE_CHILD_ID_RESPONSE).\
        filter(lambda p: {
            PREFIX_1,
            PREFIX_2
        } <= set(p.thread_nwd.tlv.prefix)).\
        must_next()

    # Step 5: SED_1
    # - Description: Harness instructs device to join the network; it requests only stable data.
    # - Pass Criteria: N/A
    print("Step 5: SED_1 joins the network.")
    pkts.filter_wpan_src64(SED_1).\
        filter_mle_cmd(consts.MLE_CHILD_ID_REQUEST).\
        filter(lambda p: consts.NETWORK_DATA_TLV in p.mle.tlv.type).\
        must_next()

    # Step 6: Leader (DUT)
    # - Description: Automatically sends the requested stable network data to SED_1.
    # - Pass Criteria:
    #   - The DUT MUST send a MLE Child ID Response to SED_1, including the Network Data TLV (only stable Network Data)
    #     and the following TLVs:
    #     - At least one Prefix TLV (Prefix 1), including:
    #       - 6LoWPAN ID sub-TLV
    #       - Border Router sub-TLV
    #       - P_border_router_16 <0xFFFE>
    #     - Prefix 2 TLV MUST NOT be included.
    print("Step 6: Leader (DUT) automatically sends requested stable network data to SED_1.")
    pkts.filter_wpan_src64(LEADER).\
        filter_wpan_dst64(SED_1).\
        filter_mle_cmd(consts.MLE_CHILD_ID_RESPONSE).\
        filter(lambda p: {PREFIX_1} <= set(p.thread_nwd.tlv.prefix)).\
        filter(lambda p: PREFIX_2 not in p.thread_nwd.tlv.prefix).\
        filter(lambda p: all(rloc == 0xfffe for rloc in p.thread_nwd.tlv.border_router_16)).\
        must_next()

    # Step 7: MED_1
    # - Description: Harness instructs device to join the network; it requests complete network data.
    # - Pass Criteria: N/A
    print("Step 7: MED_1 joins the network.")
    pkts.filter_wpan_src64(MED_1).\
        filter_mle_cmd(consts.MLE_CHILD_ID_REQUEST).\
        filter(lambda p: consts.NETWORK_DATA_TLV in p.mle.tlv.type).\
        must_next()

    # Step 8: Leader (DUT)
    # - Description: Automatically sends the requested network data to MED_1.
    # - Pass Criteria:
    #   - The DUT MUST send a MLE Child ID Response to MED_1, which includes the following TLVs:
    #     - Network Data TLV
    #       - At least two prefix TLVs (Prefix 1 and Prefix 2), each including:
    #         - 6LoWPAN ID sub-TLV
    #         - Border Router sub-TLV
    print("Step 8: Leader (DUT) automatically sends requested network data to MED_1.")
    pkts.filter_wpan_src64(LEADER).\
        filter_wpan_dst64(MED_1).\
        filter_mle_cmd(consts.MLE_CHILD_ID_RESPONSE).\
        filter(lambda p: {
            PREFIX_1,
            PREFIX_2
        } <= set(p.thread_nwd.tlv.prefix)).\
        must_next()

    # Step 9: MED_1, SED_1
    # - Description: After attaching, each Child automatically sends its global address configured to the Leader, in
    #   the Address Registration TLV from the Child Update request command.
    # - Pass Criteria: N/A
    print("Step 9: MED_1 and SED_1 automatically send Address Registration.")

    def verify_child_update(child_ext64):
        # Step 9: Child Update Request from child
        pkts.copy().\
            filter_wpan_src64(child_ext64).\
            filter_wpan_dst64(LEADER).\
            filter_mle_cmd(consts.MLE_CHILD_UPDATE_REQUEST).\
            filter(lambda p: consts.ADDRESS_REGISTRATION_TLV in p.mle.tlv.type).\
            must_next()

        # Step 10: Child Update Response from Leader
        pkts.copy().\
            filter_wpan_src64(LEADER).\
            filter_wpan_dst64(child_ext64).\
            filter_mle_cmd(consts.MLE_CHILD_UPDATE_RESPONSE).\
            filter(lambda p: {
                consts.SOURCE_ADDRESS_TLV,
                consts.ADDRESS_REGISTRATION_TLV,
                consts.MODE_TLV
            } <= set(p.mle.tlv.type)).\
            must_next()

    print("Step 10: Leader (DUT) automatically replies to each Child with a Child Update Response.")
    verify_child_update(SED_1)
    verify_child_update(MED_1)


if __name__ == '__main__':
    verify_utils.run_main(verify)
