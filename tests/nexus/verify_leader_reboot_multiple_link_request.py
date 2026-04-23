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
from pktverify.null_field import nullField


def verify(pv):
    # Test Leader Reboot Multiple Link Request
    #
    # Topology
    # - Leader
    # - Router
    #
    # Purpose & Description
    # The purpose of this test case is to show that when the Leader is rebooted, it sends
    # MLE_MAX_RESTORING_TRANSMISSION_COUNT MLE link request packets if no response is received.

    pkts = pv.pkts
    pv.summary.show()

    LEADER = pv.vars['LEADER']
    ROUTER = pv.vars['ROUTER']

    # Step 1: Form topology
    # - Verify topology is formed correctly.
    pv.verify_attached('ROUTER', 'LEADER')

    # Step 2: Reset Leader and isolate it from Router
    # - The Leader MUST send MLE_MAX_RESTORING_TRANSMISSION_COUNT multicast Link Request
    #   since it doesn't receive Link Accept from Router.

    # The value of MLE_MAX_RESTORING_TRANSMISSION_COUNT is 4 by default.
    MLE_MAX_RESTORING_TRANSMISSION_COUNT = 4

    for i in range(MLE_MAX_RESTORING_TRANSMISSION_COUNT):
        pkts.filter_wpan_src64(LEADER).\
            filter_LLARMA().\
            filter_mle_cmd(consts.MLE_LINK_REQUEST).\
            filter(lambda p: {
                              consts.CHALLENGE_TLV,
                              consts.VERSION_TLV,
                              consts.TLV_REQUEST_TLV,
                              consts.ADDRESS16_TLV,
                              consts.ROUTE64_TLV
                              } <= set(p.mle.tlv.type) and\
                   p.mle.tlv.addr16 is nullField and\
                   p.mle.tlv.route64.id_mask is nullField).\
            must_next()


if __name__ == '__main__':
    verify_utils.run_main(verify)
