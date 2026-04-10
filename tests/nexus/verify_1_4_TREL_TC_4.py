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
    # 8.4. [1.4] [CERT] Radio Link (Re)discovery through Receive
    #
    # 8.4.1. Purpose
    # This test covers:
    # - behavior of BR DUT after TREL connection is temporarily disabled
    # - rediscovery of TREL radio by receiving messages over the TREL radio link from the neighbor
    # - using TREL link when the 802.15.4 link becomes unavailable
    #
    # 8.4.2. Topology
    # - BR Leader (DUT) - Support multi-radio (TREL and 15.4)
    # - Router - Reference device that supports multi-radio (TREL and 15.4).
    # - ED - Reference device that supports 15.4 radio only.
    #
    # Spec Reference                 | V1.1 Section | V1.4 Section
    # -------------------------------|--------------|-------------
    # Radio Link (Re)discovery (1.4) | N/A          | 8.4

    pkts = pv.pkts
    pv.summary.show()

    BR_ETH = pv.vars['BR_ETH']
    ROUTER_ETH = pv.vars['ROUTER_ETH']

    BR_MLEID = pv.vars['BR_MLEID']
    ROUTER_MLEID = pv.vars['ROUTER_MLEID']
    ED_MLEID = pv.vars['ED_MLEID']

    BR_TREL_PORT = pv.vars['BR_TREL_PORT']
    ROUTER_TREL_PORT = pv.vars['ROUTER_TREL_PORT']

    # Step 1
    # - Device: BR (DUT), Router, ED
    # - Description (TREL-8.4): Form the topology. Wait for Router and BR (DUT) to become routers.
    #   ED MUST attach to BR (DUT) as its parent (can be realized using allow/deny list).
    # - Pass Criteria:
    #   - Verify that topology is formed.
    #   - ED MUST attach to the DUT as its parent.
    print("Step 1: Form the topology.")
    # Verify any Child ID Request exists to confirm topology formation started
    pkts.copy().\
        filter_mle_cmd(consts.MLE_CHILD_ID_REQUEST).\
        must_next()

    # Step 2
    # - Device: Router
    # - Description (TREL-8.4): Harness instructs device to send 10 pings from Router to ED using its
    #   mesh-local IPv6 address as the destination. Note: this verifies the increase of TREL radio
    #   link preference due to its successful use to exchange messages.
    # - Pass Criteria:
    #   - The Router MUST receive ping responses from ED.
    #   - The Router MUST have correctly detected that the DUT supports both TREL and 15.4 radios
    #     (from the neighbor table entry info).
    #   - The TREL radio link MUST be preferred, i.e. the preference value associated with TREL
    #     higher than 15.4 for DUR entry in the Router’s neighbor table.
    print("Step 2: Send 10 pings from Router to ED.")
    # Verify pings from Router to BR use TREL
    pkts.copy().\
        filter_eth_src(ROUTER_ETH).\
        filter(lambda p: p.eth.dst == BR_ETH).\
        filter(lambda p: hasattr(p, 'udp')).\
        filter(lambda p: p.udp.dstport == BR_TREL_PORT).\
        must_next()

    # Step 3
    # - Device: Router
    # - Description (TREL-8.4): Harness disables the TREL connectivity on the Router.
    # - Pass Criteria:
    #   - N/A
    print("Step 3: Disable TREL connectivity on the Router.")

    # Step 4
    # - Device: Router
    # - Description (TREL-8.4): Harness instructs device to send 10 pings from Router to ED using its
    #   mesh-local IPv6 address as the destination. Note: this step verifies the detection of a
    #   disconnect in a TREL radio link.
    # - Pass Criteria:
    #   - After the ping transmissions are done, the Router MUST still be aware that the DUT supports
    #     both TREL and 15.4 radios (from the neighbor table entry info).
    #   - The TREL radio link preference MUST however be set to zero i.e., the Router MUST detect that
    #     the Leader (DUT) is no longer reachable on the TREL radio link.
    print("Step 4: Send 10 pings from Router to ED.")
    # Pings should switch back to 15.4 (WPAN)
    pkts.copy().\
        filter_ping_request().\
        filter_ipv6_src(ROUTER_MLEID).\
        filter_ipv6_dst(ED_MLEID).\
        filter(lambda p: hasattr(p, 'wpan')).\
        must_next()

    # Step 5
    # - Device: Router
    # - Description (TREL-8.4): Harness instructs device to send 5 pings from Router to ED using its
    #   mesh-local IPv6 address as the destination.
    # - Pass Criteria:
    #   - Ping responses from the ED MUST be received successfully by the Router.
    print("Step 5: Send 5 pings from Router to ED.")
    pkts.copy().\
        filter_ping_request().\
        filter_ipv6_src(ROUTER_MLEID).\
        filter_ipv6_dst(ED_MLEID).\
        must_next()

    # Step 6
    # - Device: Router
    # - Description (TREL-8.4): Harness re-enables TREL connectivity on the Router.
    # - Pass Criteria:
    #   - N/A
    print("Step 6: Re-enable TREL connectivity on the Router.")

    # Step 7
    # - Device: Router
    # - Description (TREL-8.4): Harness instructs device to send 300 UDP messages from Router to ED
    #   using mesh-local IPv6 addresses as source and destination.
    # - Pass Criteria:
    #   - N/A
    print("Step 7: Send 300 UDP messages from Router to ED.")
    # Verify UDP messages (pings in our test) from Router to BR use TREL again (due to probe)
    pkts.copy().\
        filter_eth_src(ROUTER_ETH).\
        filter(lambda p: p.eth.dst == BR_ETH).\
        filter(lambda p: hasattr(p, 'udp')).\
        filter(lambda p: p.udp.dstport == BR_TREL_PORT).\
        must_next()

    # Step 8
    # - Device: Router
    # - Description (TREL-8.4): Harness disables 15.4 radio link on Router.
    # - Pass Criteria:
    #   - N/A
    print("Step 8: Disable 15.4 radio link on Router.")

    # Step 9
    # - Device: ED
    # - Description (TREL-8.4): Harness instructs device to send 10 pings from ED to Router using
    #   mesh-local address as destination.
    # - Pass Criteria:
    #   - The ED MUST receive ping responses from Router.
    print("Step 9: Send 10 pings from ED to Router.")
    # Path: ED -> BR (15.4) -> Router (TREL)
    # BR -> Router (TREL)
    pkts.copy().\
        filter_eth_src(BR_ETH).\
        filter(lambda p: p.eth.dst == ROUTER_ETH).\
        filter(lambda p: hasattr(p, 'udp')).\
        filter(lambda p: p.udp.dstport == ROUTER_TREL_PORT).\
        must_next()
    # Router -> BR (TREL)
    pkts.copy().\
        filter_eth_src(ROUTER_ETH).\
        filter(lambda p: p.eth.dst == BR_ETH).\
        filter(lambda p: hasattr(p, 'udp')).\
        filter(lambda p: p.udp.dstport == BR_TREL_PORT).\
        must_next()


if __name__ == '__main__':
    verify_utils.run_main(verify)
