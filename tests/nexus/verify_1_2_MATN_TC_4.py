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
import struct

# Add the current directory to sys.path to find verify_utils
CUR_DIR = os.path.dirname(os.path.abspath(__file__))
sys.path.append(CUR_DIR)

import verify_utils
from pktverify.addrs import Ipv6Addr
from pktverify.bytes import Bytes

ST_MLR_SUCCESS = 0


def verify(pv):
    # 5.10.4 MATN-TC-04: Removal of multicast listener by timeout expiry
    #
    # 5.10.4.1 Topology
    # - BR_1 (DUT)
    # - BR_2
    # - Router
    # - Host
    #
    # 5.10.4.2 Purpose & Description
    # The purpose of this test case is to verify that a Primary BBR can remove an entry from a Multicast Listeners
    #   Table, when the entry expires. The test case also verifies that the Primary BBR accepts a new registration to
    #   the previously expired multicast group.
    #
    # Spec Reference           | V1.2 Section
    # -------------------------|-------------
    # Multicast Listener Table | 5.10.4

    pkts = pv.pkts
    vars = pv.vars

    def _is_from_br1(p):
        try:
            if p.ipv6.src == vars['BR_1_RLOC']:
                return True
            if p.ipv6.opt.mpl.flag.s == 1 and\
               p.ipv6.opt.mpl.seed_id == Bytes(struct.pack('>H', vars['BR_1_RLOC16'])):
                return True
        except (AttributeError, KeyError):
            pass
        return False

    def _verify_br1_forwards_ping():
        # The packet is forwarded as MPL-encapsulated ICMPv6 Echo Request to ff03::fc
        pkts.filter_wpan_src64(vars['BR_1']).\
            filter_ping_request().\
            filter(lambda p: p.ipv6.dst == 'ff03::fc' or\
                             p.ipv6.dst == Ipv6Addr(vars['MA1'])).\
            filter(_is_from_br1).\
            must_next()

    def _verify_br1_does_not_forward_ping():
        # We only check for a limited range of packets to avoid finding the forwarded ping from later steps.
        start_idx = pkts.index
        stop_idx = (min(start_idx[0] + 100, len(pkts)), min(start_idx[1] + 100, len(pkts)))
        pkts.range(start_idx, stop_idx).\
            filter_wpan_src64(vars['BR_1']).\
            filter_ping_request().\
            filter(lambda p: p.ipv6.dst == 'ff03::fc' or\
                             p.ipv6.dst == Ipv6Addr(vars['MA1'])).\
            filter(_is_from_br1).\
            must_not_next()

    # Step 0
    # - Device: BR_1 (DUT)
    # - Description: Topology formation – BR_1 (DUT). Boot DUT. Configure the MLR timeout value of the DUT to be
    #   MLR_TIMEOUT_MIN seconds.
    # - Pass Criteria:
    #   - N/A
    print("Step 0: Topology formation - BR_1 (DUT).")

    # Step 0 (cont.)
    # - Device: N/A
    # - Description: Topology formation – BR_2, Router
    # - Pass Criteria:
    #   - N/A
    print("Step 0 (cont.): Topology formation - BR_2, Router.")

    # Step 1
    # - Device: Router
    # - Description: Harness instructs the device to register multicast address, MA1
    # - Pass Criteria:
    #   - N/A
    #   - Note: Checks for BLMR.ntf are intentionally skipped.
    print("Step 1: Router registers multicast address, MA1.")
    pkts.filter_coap_request('/n/mr').\
        filter(lambda p: p.ipv6.dst == Ipv6Addr(vars['BR_1_RLOC']) or\
                         Ipv6Addr(p.ipv6.dst).endswith(Bytes('000000fffe00fc01'))).\
        filter(lambda p: vars['MA1'] in p.coap.tlv.ipv6_address).\
        must_next()

    # Step 2
    # - Device: Host
    # - Description: Harness instructs the device to sends an ICMPv6 Echo (ping) Request packet to the multicast
    #   address, MA1.
    # - Pass Criteria:
    #   - N/A
    print("Step 2: Host sends an ICMPv6 Echo (ping) Request packet to the multicast address, MA1.")
    pkts.filter_ping_request().\
        filter(lambda p: p.ipv6.dst == Ipv6Addr(vars['MA1'])).\
        filter(lambda p: p.ipv6.src == Ipv6Addr(vars['HOST_BACKBONE_ULA'])).\
        must_next()

    # Step 3
    # - Device: BR_1 (DUT)
    # - Description: Automatically forwards the ping request packet to its Thread Network.
    # - Pass Criteria:
    #   - The DUT MUST forward the ICMPv6 Echo (ping) Request packet with multicast address, MA1, to its Thread Network
    #     encapsulated in an MPL packet.
    print("Step 3: BR_1 automatically forwards the ping request packet to its Thread Network.")
    _verify_br1_forwards_ping()

    # Step 4
    # - Device: Router
    # - Description: Receives the MPL packet containing an encapsulated ping request packet to MA1, sent by Host, and
    #   automatically unicasts an ICMPv6 Echo (ping) Reply packet back to Host.
    # - Pass Criteria:
    #   - N/A
    print("Step 4: Router automatically unicasts an ICMPv6 Echo (ping) Reply packet back to Host.")
    pkts.filter_ping_reply().\
        filter(lambda p: p.ipv6.dst == Ipv6Addr(vars['HOST_BACKBONE_ULA'])).\
        must_next()

    # Step 5
    # - Device: Router
    # - Description: Harness instructs the device to stop listening to the multicast address, MA1.
    # - Pass Criteria:
    #   - N/A
    print("Step 5: Router stops listening to the multicast address, MA1.")

    # Step 6
    # - Device: BR_1 (DUT)
    # - Description: After (MLR_TIMEOUT_MIN+2) seconds, the DUT automatically multicasts MLDv2 message to deregister
    #   from multicast group MA1.
    # - Pass Criteria:
    #   - The DUT MUST multicast an MLDv2 message of type “Version 2 Multicast Listener Report” (see [RFC 3810] Section
    #     5.2). Where: Nr of Mcast Address Records (M): at least 1.
    #   - The Multicast Address Record [1] contains the following: Record Type: 3 (CHANGE_TO_INCLUDE_MODE), Number of
    #     Sources (N): 0, Multicast Address: MA1.
    #   - The message MUST occur after MLR_TIMEOUT_MIN+2) seconds [after what].
    #   - Note: Checks for MLDv2 are intentionally skipped.
    print("Step 6: After (MLR_TIMEOUT_MIN+2) seconds, the DUT automatically deregisters from multicast group MA1.")
    # Step 6 is intentionally skipped: checks for MLDv2 are intentionally skipped.

    # Step 7
    # - Device: Host
    # - Description: Harness instructs the device to send an ICMPv6 Echo (ping) Request packet to the multicast
    #   address, MA1.
    # - Pass Criteria:
    #   - N/A
    print("Step 7: Host sends an ICMPv6 Echo (ping) Request packet to the multicast address, MA1.")
    pkts.filter_ping_request().\
        filter(lambda p: p.ipv6.dst == Ipv6Addr(vars['MA1'])).\
        filter(lambda p: p.ipv6.src == Ipv6Addr(vars['HOST_BACKBONE_ULA'])).\
        must_next()

    # Step 8
    # - Device: BR_1 (DUT)
    # - Description: Does not forward the ping request packet to its Thread Network.
    # - Pass Criteria:
    #   - The DUT MUST NOT forward the ICMPv6 Echo (ping) Request packet with multicast address, MA1, to its Thread
    #     Network.
    print("Step 8: BR_1 does not forward the ping request packet to its Thread Network.")
    _verify_br1_does_not_forward_ping()

    # Step 9
    # - Device: Router
    # - Description: Harness instructs the device to register multicast address, MA1, at BR_1
    # - Pass Criteria:
    #   - N/A
    #   - Note: Checks for BLMR.ntf are intentionally skipped.
    print("Step 9: Router registers multicast address, MA1, at BR_1.")
    pkts.filter_coap_request('/n/mr').\
        filter(lambda p: p.ipv6.dst == Ipv6Addr(vars['BR_1_RLOC']) or\
                         Ipv6Addr(p.ipv6.dst).endswith(Bytes('000000fffe00fc01'))).\
        filter(lambda p: vars['MA1'] in p.coap.tlv.ipv6_address).\
        must_next()

    # Step 10
    # - Device: Host
    # - Description: Harness instructs the device to send an ICMPv6 Echo (ping) Request packet to the multicast
    #   address, MA1.
    # - Pass Criteria:
    #   - N/A
    print("Step 10: Host sends an ICMPv6 Echo (ping) Request packet to the multicast address, MA1.")
    pkts.filter_ping_request().\
        filter(lambda p: p.ipv6.dst == Ipv6Addr(vars['MA1'])).\
        filter(lambda p: p.ipv6.src == Ipv6Addr(vars['HOST_BACKBONE_ULA'])).\
        must_next()

    # Step 11
    # - Device: BR_1 (DUT)
    # - Description: Automatically forwards the ping request packet to its Thread Network.
    # - Pass Criteria:
    #   - The DUT MUST forward the ICMPv6 Echo (ping) Request packet with multicast address, MA1, to its Thread Network
    #     encapsulated in an MPL packet.
    print("Step 11: BR_1 automatically forwards the ping request packet to its Thread Network.")
    _verify_br1_forwards_ping()

    # Step 12
    # - Device: Router
    # - Description: Receives the MPL packet containing an encapsulated ping request packet to MA1, sent by Host, and
    #   automatically unicasts an ICMPv6 Echo (ping) Reply packet back to Host.
    # - Pass Criteria:
    #   - N/A
    print("Step 12: Router automatically unicasts an ICMPv6 Echo (ping) Reply packet back to Host.")
    pkts.filter_ping_reply().\
        filter(lambda p: p.ipv6.dst == Ipv6Addr(vars['HOST_BACKBONE_ULA'])).\
        must_next()


if __name__ == '__main__':
    verify_utils.run_main(verify)
