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
    # 8.3. [1.4] [CERT] Radio Link (Re)discovery using Probe Mechanism
    #
    # 8.3.1. Purpose
    # This test covers the behavior of the device after TREL connection is temporarily disabled and rediscovery of TREL
    #   radio using the multi-radio Probe mechanism.
    #
    # Spec Reference   | V1.1 Section | V1.3.0 Section
    # -----------------|--------------|---------------
    # TREL Radio Links | N/A          | 8.3

    pkts = pv.pkts
    pv.summary.show()

    BR_DUT = pv.vars['BR_DUT']
    ROUTER = pv.vars['ROUTER']
    ED = pv.vars['ED']

    # - Step 1
    #   - Device: BR (DUT), Router
    #   - Description (TREL-8.3): Form the topology. Wait for Router to become Thread router. ED MUST attach to the
    #     DUT as its parent (e.g. can be realized using allow/deny list).
    #   - Pass Criteria:
    #     - Verify that topology is formed.
    #     - ED MUST attach to the DUT as its parent.
    print(
        "Step 1: Form the topology. Wait for Router to become Thread router. ED MUST attach to the DUT as its parent.")
    # Verify DUT is leader
    pkts.filter_wpan_src64(BR_DUT).\
        filter_mle_cmd(consts.MLE_ADVERTISEMENT).\
        filter(lambda p: p.mle.tlv.leader_data.router_id == p.mle.tlv.source_addr >> 10).\
        must_next()

    # Verify ED attached to BR
    pkts.filter_wpan_src64(ED).\
        filter_mle_cmd(consts.MLE_CHILD_ID_REQUEST).\
        filter_wpan_dst64(BR_DUT).\
        must_next()

    # - Step 2
    #   - Device: ED
    #   - Description (TREL-8.3): Harness instructs device to send 10 pings to Router using its mesh-local IPv6
    #     address as the destination. Note: this verifies the increase of TREL radio link preference due to its
    #     successful use to exchange the ping messages.
    #   - Pass Criteria:
    #     - The ED MUST receive ping responses.
    #     - The Router MUST have correctly detected that the DUT supports both TREL and 15.4 radios (from the
    #       neighbor table entry info).
    #     - Also TREL radio link MUST be preferred; that is, the preference value associated with TREL MUST be higher
    #       than or equal to 15.4 radio for the BR (DUT) entry in the Router’s neighbor table multi-radio info.
    #     - TREL frames sent by the DUT MUST use a correct TREL frame format.
    print("Step 2: Harness instructs device to send 10 pings to Router using its mesh-local IPv6 address.")
    for _ in range(10):
        _pkt = pkts.filter_ping_request().\
            filter_ipv6_src(pv.vars['ED_MLEID']).\
            filter_ipv6_dst(pv.vars['ROUTER_MLEID']).\
            must_next()

        pkts.filter_ping_reply(identifier=_pkt.icmpv6.echo.identifier).\
            filter_ipv6_src(pv.vars['ROUTER_MLEID']).\
            filter_ipv6_dst(pv.vars['ED_MLEID']).\
            must_next()

    # Verify TREL frames sent by DUT to Router
    pkts.filter_eth_src(pv.vars['BR_DUT_ETH']).\
        filter(lambda p: p.eth.dst == pv.vars['ROUTER_ETH']).\
        filter(lambda p: p.udp).\
        filter(lambda p: p.udp.dstport == pv.vars['ROUTER_TREL_PORT']).\
        must_next()

    # - Step 3
    #   - Device: Router
    #   - Description (TREL-8.3): Harness disables the TREL connectivity on the Router. Note: This may be realized by
    #     causing a disconnect on the infrastructure link (e.g. if the infra link is Wi-Fi, the Router device can be
    #     disconnected from Wi-Fi AP). Alternatively this can be realized by specific APIs added on Router device
    #     for the purpose of testing. Note: this may also be realized using the "trel filter" CLI command. See link.
    #   - Pass Criteria:
    #     - N/A
    print("Step 3: Harness disables the TREL connectivity on the Router.")

    # - Step 4
    #   - Device: ED
    #   - Description (TREL-8.3): Harness instructs device to send 10 pings to Router using its mesh-local IPv6
    #     address as the destination. Note: some of the pings may fail, which is expected behavior. This step tests
    #     the detection of a disconnect in a TREL link by the DUT.
    #   - Pass Criteria:
    #     - N/A
    print("Step 4: Harness instructs device to send 10 pings to Router using its mesh-local IPv6 address.")
    # Pings are sent in C++, we just advance the packet cursor
    for _ in range(10):
        pkts.filter_ping_request().\
            filter_ipv6_src(pv.vars['ED_MLEID']).\
            filter_ipv6_dst(pv.vars['ROUTER_MLEID']).\
            must_next()

    # - Step 5
    #   - Device: Router
    #   - Description (TREL-8.3): Harness instructs device to report its radio link preference from its neighbor
    #     table.
    #   - Pass Criteria:
    #     - The Router MUST report that the DUT supports both TREL and 15.4 radios
    #     - The TREL radio link preference MUST be set to zero (the DUT is no longer reachable on the TREL radio
    #       link).
    print("Step 5: Harness instructs device to report its radio link preference from its neighbor table.")

    # - Step 6
    #   - Device: ED
    #   - Description (TREL-8.3): Harness instructs device to send 5 pings to Router using its mesh-local IPv6
    #     address as the destination. Note: this step verifies that the DUT correctly falls back to using 15.4
    #     radio on detection of TREL disconnect.
    #   - Pass Criteria:
    #     - The ED MUST receive all Ping responses from the Router successfully.
    print("Step 6: Harness instructs device to send 5 pings to Router using its mesh-local IPv6 address.")
    for _ in range(5):
        _pkt = pkts.filter_ping_request().\
            filter_ipv6_src(pv.vars['ED_MLEID']).\
            filter_ipv6_dst(pv.vars['ROUTER_MLEID']).\
            must_next()

        # Responses must be on 15.4
        pkts.filter_ping_reply(identifier=_pkt.icmpv6.echo.identifier).\
            filter_ipv6_src(pv.vars['ROUTER_MLEID']).\
            filter_ipv6_dst(pv.vars['ED_MLEID']).\
            filter(lambda p: p.wpan).\
            must_next()

    # - Step 7
    #   - Device: Router
    #   - Description (TREL-8.3): Harness re-enables TREL connectivity on the Router.
    #   - Pass Criteria:
    #     - N/A
    print("Step 7: Harness re-enables TREL connectivity on the Router.")

    # - Step 8
    #   - Device: ED
    #   - Description (TREL-8.3): Harness instructs device to send 300 UDP messages (small payload) to Router using
    #     mesh-local IPv6 addresses as source and destination. Note: this step is intended to trigger and verify
    #     the probe mechanism on the DUT.
    #   - Pass Criteria:
    #     - N/A
    print("Step 8: Harness instructs device to send 300 UDP messages (small payload) to Router.")
    # Verify that some TREL traffic resumed eventually
    pkts.filter_eth_src(pv.vars['BR_DUT_ETH']).\
        filter(lambda p: p.eth.dst == pv.vars['ROUTER_ETH']).\
        filter(lambda p: p.udp).\
        filter(lambda p: p.udp.dstport == pv.vars['ROUTER_TREL_PORT']).\
        must_next()

    # - Step 9
    #   - Device: Router
    #   - Description (TREL-8.3): Harness instructs device to report its radio link preference from its neighbor
    #     table.
    #   - Pass Criteria:
    #     - The Router MUST report that the DUT supports both TREL and 15.4 radios.
    #     - The reported TREL radio link preference MUST be greater than zero (again reachable on the TREL radio
    #       link), and higher or equal to the 15.4 radio link preference.
    #     - TREL frames sent by the DUT MUST use a correct TREL frame format.
    print("Step 9: Harness instructs device to report its radio link preference from its neighbor table.")


if __name__ == '__main__':
    verify_utils.run_main(verify)
