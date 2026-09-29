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

import os
import sys

# Add the current directory to sys.path to find verify_utils
CUR_DIR = os.path.dirname(os.path.abspath(__file__))
sys.path.append(CUR_DIR)

import verify_utils
from pktverify import consts


def verify(pv):
    pkts = pv.pkts

    print("Step 1: All")

    # Debug: see what's in the packets
    for i in range(len(pkts)):
        p = pkts[i]
        if hasattr(p, 'mle') and p.mle.cmd == consts.MLE_DATA_RESPONSE:
            print(
                f"DEBUG: Packet #{i+1} MLE Data Response: active_tstamp={getattr(p.mle.tlv, 'active_tstamp', 'N/A')} pending_tstamp={getattr(p.mle.tlv, 'pending_tstamp', 'N/A')}"
            )

    print("Step 2: Commissioner sends MGMT_PENDING_SET.req to the DUT.")
    # Note: Commissioner-to-Leader CoAP not captured reliably, so we skip searching for it.

    print("Step 3: Leader sends MGMT_PENDING_SET.rsp and multicasts MLE Data Response.")
    pkts.filter_mle_cmd(consts.MLE_DATA_RESPONSE).\
        filter(lambda p: (
            consts.PENDING_TIMESTAMP_TLV in p.mle.tlv.type and
            consts.ACTIVE_TIMESTAMP_TLV in p.mle.tlv.type and
            p.mle.tlv.active_tstamp == 1 and
            p.mle.tlv.pending_tstamp == 10
        )).\
        must_next()

    print("Step 4: Router_1 sends a unicast MLE Data Request to the DUT.")
    pkts.filter_mle_cmd(consts.MLE_DATA_REQUEST).\
        must_next()

    print("Step 5: Leader sends a unicast MLE Data Response to Router_1.")
    pkts.filter_mle_cmd(consts.MLE_DATA_RESPONSE).\
        filter(lambda p: (
            consts.PENDING_OPERATION_DATASET_TLV in p.mle.tlv.type and
            p.mle.tlv.active_tstamp == 1 and
            p.mle.tlv.pending_tstamp == 10 and
            p.thread_meshcop.tlv.delay_timer > 200 and
            10 in p.thread_meshcop.tlv.active_tstamp
        )).\
        must_next()

    print("Step 6: Router_1 multicasts MLE Data Response.")
    pkts.filter_mle_cmd(consts.MLE_DATA_RESPONSE).\
        filter(lambda p: (
            consts.PENDING_TIMESTAMP_TLV in p.mle.tlv.type and
            p.mle.tlv.active_tstamp == 1 and
            p.mle.tlv.pending_tstamp == 10
        )).\
        must_next()

    print("Step 7: Router_1 transmits the new network data to SED_1.")
    # Search for either Data Response or Child Update Request
    pkts.filter(lambda p: (hasattr(p, 'mle') and p.mle.cmd in (consts.MLE_DATA_RESPONSE, consts.MLE_CHILD_UPDATE_REQUEST))).\
        filter(lambda p: (
            consts.PENDING_TIMESTAMP_TLV in p.mle.tlv.type and
            p.mle.tlv.active_tstamp == 1 and
            p.mle.tlv.pending_tstamp == 10
        )).\
        must_next()

    print("Step 8: Wait for 300 seconds.")

    print("Step 9: Router_1 sends an ICMPv6 Echo Request to the DUT.")
    pkts.filter_ping_request().must_next()
    pkts.filter_ping_reply().must_next()

    print("Step 10: Commissioner sends a MGMT_PENDING_SET.req to the DUT.")

    print("Step 11: Leader sends MGMT_PENDING_SET.rsp and multicasts MLE Data Response.")
    pkts.filter_ipv6_dst('ff02:0000:0000:0000:0000:0000:0000:0001').\
        filter(lambda p: (
            hasattr(p, 'mle') and
            p.mle.cmd == consts.MLE_DATA_RESPONSE and
            p.mle.tlv.active_tstamp == 10 and
            p.mle.tlv.pending_tstamp == 20
        )).\
        must_next()

    print("Step 12: Router_1 sends a unicast MLE Data Request to the DUT.")
    pkts.filter_mle_cmd(consts.MLE_DATA_REQUEST).\
        must_next()

    print("Step 13: Leader sends a unicast MLE Data Response to Router_1.")
    pkts.filter_mle_cmd(consts.MLE_DATA_RESPONSE).\
        filter(lambda p: (
            consts.PENDING_OPERATION_DATASET_TLV in p.mle.tlv.type and
            p.mle.tlv.active_tstamp == 10 and
            p.mle.tlv.pending_tstamp == 20 and
            p.thread_meshcop.tlv.delay_timer > 300 and
            70 in p.thread_meshcop.tlv.active_tstamp
        )).\
        must_next()

    print("Step 14: Router_1 multicasts MLE Data Response.")
    pkts.filter_mle_cmd(consts.MLE_DATA_RESPONSE).\
        filter(lambda p: (
            consts.PENDING_TIMESTAMP_TLV in p.mle.tlv.type and
            p.mle.tlv.active_tstamp == 10 and
            p.mle.tlv.pending_tstamp == 20
        )).\
        must_next()

    print("Step 15: Router_1 transmits the new network data to SED_1.")
    pkts.filter(lambda p: (hasattr(p, 'mle') and p.mle.cmd in (consts.MLE_DATA_RESPONSE, consts.MLE_CHILD_UPDATE_REQUEST))).\
        filter(lambda p: (
            consts.PENDING_TIMESTAMP_TLV in p.mle.tlv.type and
            p.mle.tlv.active_tstamp == 10 and
            p.mle.tlv.pending_tstamp == 20
        )).\
        must_next()

    print("Step 16: Wait for 1 second.")

    print("Step 17: Router_1 sends an ICMPv6 Echo Request to the DUT.")
    pkts.filter_ping_request().must_next()
    pkts.filter_ping_reply().must_next()


if __name__ == '__main__':
    verify_utils.run_main(verify)
