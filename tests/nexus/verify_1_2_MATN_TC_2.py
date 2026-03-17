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
MLR_TIMEOUT = 3600


def verify(pv):
    pkts = pv.pkts
    vars = pv.vars

    def _is_from_br(p, br_num):
        try:
            if p.ipv6.src == vars[f'BR_{br_num}_RLOC']:
                return True
            if p.ipv6.opt.mpl.flag.s == 1 and \
               p.ipv6.opt.mpl.seed_id == Bytes(struct.pack('>H', vars[f'BR_{br_num}_RLOC16'])):
                return True
        except (AttributeError, KeyError):
            pass
        return False

    def _verify_br_does_not_forward_ping(br_to_check, mcast_addr_var, other_br_num):
        """Verifies that `br_to_check` does not forward a ping to `mcast_addr_var`."""
        pkts.copy().\
            filter_wpan_src64(vars[br_to_check]).\
            filter_ping_request().\
            filter(lambda p: p.ipv6.dst == Ipv6Addr(vars[mcast_addr_var]) or p.ipv6.dst == 'ff03::fc').\
            filter(lambda p: not _is_from_br(p, other_br_num)).\
            must_not_next()

    # Step 1
    # - Device: TD
    # - Description: User or Harness instructs the device to register multicast address, MA1, at BR_1.
    # - Pass Criteria:
    #   - For DUT = TD: The DUT MUST unicast an MLR.req CoAP request to BR_1 as follows: coap://[<BR_1 RLOC or PBBR ALOC>]:MM/n/mr
    #   - Where the payload contains: IPv6 Addresses TLV: MA1
    print("Step 1: TD registers multicast address, MA1, at BR_1.")
    pkts.filter_coap_request('/n/mr').\
        filter(lambda p: p.ipv6.dst == Ipv6Addr(vars['BR_1_RLOC']) or
                         Ipv6Addr(p.ipv6.dst).endswith(Bytes('000000fffe00fc01'))).\
        filter(lambda p: vars['MA1'] in p.coap.tlv.ipv6_address).\
        must_next()

    # Step 2
    # - Device: BR_1
    # - Description: Automatically updates its multicast listener table.
    # - Pass Criteria:
    #   - None.
    print("Step 2: BR_1 automatically updates its multicast listener table.")

    # Step 3
    # - Device: BR_1
    # - Description: Automatically responds to the multicast registration.
    # - Pass Criteria:
    #   - For DUT = BR_1: The DUT MUST unicast an MLR.rsp CoAP response to TD as follows: 2.04 changed
    #   - Where the payload contains: Status TLV: 0 (ST_MLR_SUCCESS)
    print("Step 3: BR_1 automatically responds to the multicast registration.")
    pkts.filter_coap_ack('/n/mr').\
        filter(lambda p: p.coap.tlv.status == ST_MLR_SUCCESS).\
        must_next()

    # Step 3a
    # - Device: BR_2
    # - Description: Does not respond to the multicast registration.
    # - Pass Criteria:
    #   - For DUT = BR_2: The DUT MUST NOT respond to the multicast registration for address, MA1.
    #   - Note: in principle BR_2 should not even receive the request message. However this check is left in as an overall integrity check for the test setup.
    print("Step 3a: BR_2 does not respond to the multicast registration.")
    pkts.copy().\
        filter_wpan_src64(vars['BR_2']).\
        filter_coap_ack('/n/mr').\
        must_not_next()

    # Step 4
    # - Device: BR_1
    # - Description: Automatically informs other BBRs on the network of the multicast registration.
    # - Pass Criteria:
    #   - For DUT = BR_1: The DUT MUST multicast a BMLR.ntf CoAP request to the Backbone Link including to BR_2, as follows: coap://[<All network BBR multicast>]:BB/b/bmr
    #   - Where the payload contains: IPv6 Addresses TLV: MA1
    #   - Timeout TLV: The value 3600
    print("Step 4: BR_1 informs other BBRs on the network of the multicast registration.")
    # Step 4 is intentionally skipped: OpenThread sends BMLR.ntf, but we skip the BLMR.ntf check as per instructions.

    # Step 5
    # - Device: BR_1
    # - Description: Automatically multicasts an MLDv2 message to start listening to the group MA1.
    # - Pass Criteria:
    #   - For DUT = BR_1: The DUT MUST multicast an MLDv2 message of type "Version 2 Multicast Listener Report" (see [RFC 3810] Section 5.2).
    #   - Where: Nr of Mcast Address Records (M): at least 1
    #   - Multicast Address Record [1] contains the following: Record Type: 4 (CHANGE_TO_EXCLUDE_MODE)
    #   - Number of Sources (N): 0
    #   - Multicast Address: MA1
    print("Step 5: BR_1 multicasts an MLDv2 message to start listening to the group MA1.")
    # Step 5 is intentionally skipped: OpenThread does not implement MLDv2.

    # Step 6
    # - Device: Host
    # - Description: Harness instructs the device to send an ICMPv6 Echo (ping) Request packet to the multicast address, MA1.
    # - Pass Criteria:
    #   - N/A
    print("Step 6: Host sends an ICMPv6 Echo (ping) Request packet to the multicast address, MA1.")
    pkts.filter_ping_request().\
        filter(lambda p: p.ipv6.dst == Ipv6Addr(vars['MA1']) and p.ipv6.src == Ipv6Addr(vars['HOST_BACKBONE_ULA'])).\
        must_next()

    # Step 7
    # - Device: BR_2
    # - Description: Does not forward the ping request packet to its Thread Network.
    # - Pass Criteria:
    #   - For DUT = BR_2: The DUT MUST NOT forward the ICMPv6 Echo (ping) Request packet with multicast address, MA1, to the Thread Network - e.g. as a new MPL packet listing itself as the MPL Seed, or in other ways.
    print("Step 7: BR_2 does not forward the ping request packet to its Thread Network.")
    _verify_br_does_not_forward_ping('BR_2', 'MA1', 1)

    # Step 8
    # - Device: BR_1
    # - Description: Automatically forwards the ping request packet to its Thread Network.
    # - Pass Criteria:
    #   - For DUT = BR_1: The DUT MUST forward the ICMPv6 Echo (ping) Request packet with multicast address, MA1, to its Thread Network encapsulated in an MPL packet.
    #   - Where MPL Option: If Source outer IP header == BR_1 RLOC Then S == 0 Else S == 1 and seed-id == BR_1 RLOC16
    print("Step 8: BR_1 automatically forwards the ping request packet to its Thread Network.")

    # The packet is forwarded as MPL-encapsulated ICMPv6 Echo Request to ff03::fc
    def check_step8(p):
        if p.ipv6.src == Ipv6Addr(vars['BR_1_RLOC']):
            return p.ipv6.opt.mpl.flag.s == 0
        else:
            return p.ipv6.opt.mpl.flag.s == 1 and \
                   p.ipv6.opt.mpl.seed_id == Bytes(struct.pack('>H', vars['BR_1_RLOC16']))

    pkts.filter_wpan_src64(vars['BR_1']).\
        filter_ping_request().\
        filter(lambda p: p.ipv6.dst == 'ff03::fc' or p.ipv6.dst == Ipv6Addr(vars['MA1'])).\
        filter(check_step8).\
        must_next()

    # Step 9
    # - Device: TD
    # - Description: Receives the multicast ping request packet and automatically sends an ICMPv6 Echo (ping) Reply packet back to Host.
    # - Pass Criteria:
    #   - For DUT = BR_1: Host MUST receive the ICMPv6 Echo (ping) Reply packet from TD.
    #   - For DUT = BR_2: Host MUST receive the ICMPv6 Echo (ping) Reply packet from TD.
    #   - For DUT = TD: Host SHOULD receive the ICMPv6 Echo (ping) Reply packet from TD. If it is not received, a warning is generated in the test report for this step, but the validation passes.
    #   - Note: if the TD DUT is a Posix device, it may not be able to process multicast ping requests.
    print("Step 9: TD sends an ICMPv6 Echo (ping) Reply packet back to Host.")
    pkts.filter(lambda p: p.ipv6.dst == Ipv6Addr(vars['HOST_BACKBONE_ULA'])).\
        filter_ping_reply().\
        must_next()

    # Step 10
    # - Device: Host
    # - Description: Harness instructs the device to send an ICMPv6 Echo (ping) Request packet to the multicast address, MA2.
    # - Pass Criteria:
    #   - N/A
    print("Step 10: Host sends an ICMPv6 Echo (ping) Request packet to the multicast address, MA2.")
    pkts.filter_ping_request().\
        filter(lambda p: p.ipv6.dst == Ipv6Addr(vars['MA2']) and p.ipv6.src == Ipv6Addr(vars['HOST_BACKBONE_ULA'])).\
        must_next()

    # Step 11
    # - Device: BR_2
    # - Description: Does not forward the ping request packet to its Thread Network.
    # - Pass Criteria:
    #   - For DUT = BR_2: The DUT MUST NOT forward the ICMPv6 Echo (ping) Request packet with multicast address, MA2, to the Thread Network in whatever way.
    print("Step 11: BR_2 does not forward the ping request packet to its Thread Network.")
    _verify_br_does_not_forward_ping('BR_2', 'MA2', 1)

    # Step 12
    # - Device: BR_1
    # - Description: Does not forward the ping request packet to its Thread Network.
    # - Pass Criteria:
    #   - For DUT = BR_1: The DUT MUST NOT forward the ICMPv6 Echo (ping) Request packet with multicast address, MA2, to its Thread Network in whatever way.
    print("Step 12: BR_1 does not forward the ping request packet to its Thread Network.")
    _verify_br_does_not_forward_ping('BR_1', 'MA2', 2)

    # Step 13
    # - Device: Host
    # - Description: Harness instructs the device to send an ICMPv6 Echo (ping) Request packet to the multicast address, MA1g.
    # - Pass Criteria:
    #   - N/A
    print("Step 13: Host sends an ICMPv6 Echo (ping) Request packet to the multicast address, MA1g.")
    pkts.filter_ping_request().\
        filter(lambda p: p.ipv6.dst == Ipv6Addr(vars['MA1g']) and p.ipv6.src == Ipv6Addr(vars['HOST_BACKBONE_ULA'])).\
        must_next()

    # Step 14
    # - Device: BR_2
    # - Description: Does not forward the ping request packet to its Thread Network.
    # - Pass Criteria:
    #   - For DUT = BR_2: The DUT MUST NOT forward the ICMPv6 Echo (ping) Request packet with multicast address, MA1g, to the Thread Network in whatever way.
    print("Step 14: BR_2 does not forward the ping request packet to its Thread Network.")
    _verify_br_does_not_forward_ping('BR_2', 'MA1g', 1)

    # Step 15
    # - Device: BR_1
    # - Description: Does not forward the ping request packet to its Thread Network.
    # - Pass Criteria:
    #   - For DUT = BR_1: The DUT MUST NOT forward the ICMPv6 Echo (ping) Request packet with multicast address, MA1g, to its Thread Network in whatever way.
    print("Step 15: BR_1 does not forward the ping request packet to its Thread Network.")
    _verify_br_does_not_forward_ping('BR_1', 'MA1g', 2)

    # Step 16
    # - Device: Host
    # - Description: Harness instructs the device to send an ICMPv6 Echo (ping) Request packet to the global unicast address, Gg.
    # - Pass Criteria:
    #   - N/A
    print("Step 16: Host sends an ICMPv6 Echo (ping) Request packet to the backbone address of BR_2.")
    pkts.filter_ping_request().\
        filter(lambda p: p.ipv6.dst == Ipv6Addr(vars['BR_2_BACKBONE_ULA']) and p.ipv6.src == Ipv6Addr(vars['HOST_BACKBONE_ULA'])).\
        must_next()

    # Step 17
    # - Device: BR_2
    # - Description: Receives and provides the ICMPv6 Echo (ping) Reply. Note: this step is included as a very important test topology connectivity test. Because the previous steps only validate negative outcomes ("not forward"), it means the testing is only useful if the DUT BR_2 was actually up and running during these test steps. This ping aims to verify this.
    # - Pass Criteria:
    #   - For DUT = BR_2: The DUT MUST send ICMPv6 Echo (ping) Reply to the Host.
    print("Step 17: BR_2 receives and provides the ICMPv6 Echo (ping) Reply.")
    pkts.filter(lambda p: p.ipv6.dst == Ipv6Addr(vars['HOST_BACKBONE_ULA'])).\
        filter_ping_reply().\
        must_next()


if __name__ == '__main__':
    verify_utils.run_main(verify)
