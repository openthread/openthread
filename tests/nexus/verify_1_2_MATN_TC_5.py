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

# CoAP port.
COAP_PORT = 5683

# Status SUCCESS
ST_MLR_SUCCESS = 0


def verify(pv):
    # 5.10.5 MATN-TC-05: Re-registration to same Multicast Group
    #
    # 5.10.5.1 Topology
    # - BR_1 (DUT)
    # - BR_2
    # - Router
    # - Host
    #
    # 5.10.5.2 Purpose & Description
    # Verify that a Primary BBR (DUT) can manage a re-registration of a device on its network to remain receiving
    #   multicasts. The test also verifies the usage of UDP multicast packets across backbone and internal Thread
    #   network.
    #
    # Spec Reference   | V1.2 Section | V1.3.0 Section
    # -----------------|--------------|---------------
    # Multicast        | 5.10.5       | 5.10.5

    pkts = pv.pkts
    pv.summary.show()

    BR_1 = pv.vars['BR_1']
    MA1 = pv.vars['MA1']
    HOST_ULA = pv.vars['HOST_ULA']
    BR_1_RLOC = pv.vars['BR_1_RLOC']
    BR_1_RLOC16 = pv.vars['BR_1_RLOC16']

    def check_mpl(p):
        if p.ipv6.src == Ipv6Addr(BR_1_RLOC):
            return p.ipv6.opt.mpl.flag.s == 0
        else:
            return p.ipv6.opt.mpl.flag.s == 1 and \
                   p.ipv6.opt.mpl.seed_id == Bytes(struct.pack('>H', BR_1_RLOC16))

    # Step 0
    # - Device: BR_1 (DUT)
    # - Description: Topology formation – BR_1 (DUT). Boot DUT. Configure the MLR timeout value of the DUT to be
    #   MLR_TIMEOUT_MIN seconds.
    # - Pass Criteria:
    #   - N/A
    print("Step 0: Topology formation – BR_1 (DUT)")

    # Step 0 (cont.)
    # - Device: N/A
    # - Description: Topology formation – BR_2, Router
    # - Pass Criteria:
    #   - N/A
    print("Step 0 (cont.): Topology formation – BR_2, Router")

    # Step 1
    # - Device: Router
    # - Description: Harness instructs the device to register multicast address, MA1
    # - Pass Criteria:
    #   - N/A
    print("Step 1: Router registers multicast address, MA1")
    pkts.filter_coap_request('/n/mr').\
        filter(lambda p: p.ipv6.dst == Ipv6Addr(BR_1_RLOC) or \
                         Ipv6Addr(p.ipv6.dst).endswith(Bytes('000000fffe00fc00'))).\
        filter(lambda p: MA1 in p.coap.tlv.ipv6_address).\
        must_next()

    pkts.filter_coap_ack('/n/mr').\
        filter(lambda p: p.coap.tlv.status == ST_MLR_SUCCESS).\
        must_next()

    # Step 2
    # - Device: Host
    # - Description: Harness instructs the device to send an ICMPv6 Echo (ping) Request packet to the multicast
    #   address, MA1.
    # - Pass Criteria:
    #   - N/A
    print("Step 2: Host sends an ICMPv6 Echo (ping) Request packet to the multicast address, MA1.")
    pkts.filter_ping_request().\
        filter(lambda p: p.ipv6.src == Ipv6Addr(HOST_ULA)).\
        filter(lambda p: p.ipv6.dst == Ipv6Addr(MA1)).\
        must_next()

    # Step 3
    # - Device: BR_1 (DUT)
    # - Description: Automatically forwards the ping request packet to its Thread Network.
    # - Pass Criteria:
    #   - The DUT MUST forward the ICMPv6 Echo (ping) Request packet with multicast address, MA1, to its Thread
    #     Network encapsulated in an MPL packet.
    print("Step 3: BR_1 (DUT) automatically forwards the ping request packet to its Thread Network.")
    pkts.filter_wpan_src64(BR_1).\
        filter_ping_request().\
        filter(lambda p: p.ipv6.dst == 'ff03::fc' or \
                         p.ipv6.dst == Ipv6Addr(MA1)).\
        filter(check_mpl).\
        must_next()

    # Step 4
    # - Device: Router
    # - Description: Receives the MPL packet containing an encapsulated ping request packet to MA1, sent by Host, and
    #   automatically unicasts an ICMPv6 (ping) Reply packet back to Host.
    # - Pass Criteria:
    #   - N/A
    print("Step 4: Router automatically unicasts an ICMPv6 (ping) Reply packet back to Host.")

    # Step 4a
    # - Device: Router
    # - Description: Within MLR_TIMEOUT_MIN seconds of initial registration, Harness instructs the device to
    #   re-register for multicast address, MA1, at BR_1 by performing test case MATN-TC-02 (see 5.10.2) using Router
    #   as TD with the respective pass criteria.
    # - Pass Criteria:
    #   - N/A
    # - Note: MLDv2 and BLMR.ntf checks are intentionally skipped because they are part of MATN-TC-02.
    print("Step 4a: Router re-registers for multicast address, MA1, at BR_1.")
    pkts.filter_coap_request('/n/mr').\
        filter(lambda p: p.ipv6.dst == Ipv6Addr(BR_1_RLOC) or \
                         Ipv6Addr(p.ipv6.dst).endswith(Bytes('000000fffe00fc00'))).\
        filter(lambda p: MA1 in p.coap.tlv.ipv6_address).\
        must_next()

    pkts.filter_coap_ack('/n/mr').\
        filter(lambda p: p.coap.tlv.status == ST_MLR_SUCCESS).\
        must_next()

    # Step 5
    # - Device: Host
    # - Description: Within MLR_TIMEOUT_MIN seconds, Harness instructs the device to send an ICMPv6 Echo (ping) Request
    #   packet to the multicast address, MA1. The destination port 5683 is used for the UDP Multicast packet
    #   transmission.
    # - Pass Criteria:
    #   - N/A
    print("Step 5: Host sends an ICMPv6 Echo (ping) Request packet (UDP) to the multicast address, MA1, port 5683.")
    pkts.filter(lambda p: p.ipv6.src == Ipv6Addr(HOST_ULA)).\
        filter(lambda p: p.ipv6.dst == Ipv6Addr(MA1)).\
        filter(lambda p: p.udp.dstport == COAP_PORT).\
        must_next()

    # Step 6
    # - Device: BR_1 (DUT)
    # - Description: Automatically forwards the UDP ping request packet to its Thread Network.
    # - Pass Criteria:
    #   - The DUT MUST forward the UDP Echo (ping) Request packet with multicast address, MA1, to its Thread Network
    #     encapsulated in an MPL packet.
    print("Step 6: BR_1 (DUT) automatically forwards the UDP ping request packet to its Thread Network.")
    pkts.filter_wpan_src64(BR_1).\
        filter(lambda p: p.ipv6.dst == 'ff03::fc' or \
                         p.ipv6.dst == Ipv6Addr(MA1)).\
        filter(lambda p: p.udp.dstport == COAP_PORT).\
        filter(check_mpl).\
        must_next()

    # Step 7
    # - Device: Router
    # - Description: Receives the ping request packet. Automatically uses the port 5683 (CoAP port) to verify that the
    #   UDP Multicast packet is received.
    # - Pass Criteria:
    #   - N/A
    print("Step 7: Router receives the UDP multicast packet.")

    # Step 7a
    # - Device: Router
    # - Description: Harness instructs the device to stop listening to the multicast address, MA1.
    # - Pass Criteria:
    #   - N/A
    print("Step 7a: Router stops listening to the multicast address, MA1.")

    # Step 8
    # - Device: Host
    # - Description: After (MLR_TIMEOUT_MIN+2) seconds, harness instructs the device to multicast an ICMPv6 Echo (ping)
    #   Request packet to multicast address, MA1, on the backbone link.
    # - Pass Criteria:
    #   - N/A
    print("Step 8: After (MLR_TIMEOUT_MIN+2) seconds, Host multicasts an ICMPv6 Echo Request to MA1.")
    pkts.filter_ping_request().\
        filter(lambda p: p.ipv6.src == Ipv6Addr(HOST_ULA)).\
        filter(lambda p: p.ipv6.dst == Ipv6Addr(MA1)).\
        must_next()

    # Step 9
    # - Device: BR_1 (DUT)
    # - Description: Does not forward the ping request packet to its Thread Network.
    # - Pass Criteria:
    #   - The DUT MUST NOT forward the ICMPv6 Echo (ping) Request packet with multicast address, MA1, to its Thread
    #     Network.
    print("Step 9: BR_1 (DUT) does not forward the ping request packet to its Thread Network.")
    pkts.filter_wpan_src64(BR_1).\
        filter_ping_request().\
        filter(lambda p: p.ipv6.dst == 'ff03::fc' or p.ipv6.dst == Ipv6Addr(MA1)).\
        must_not_next()


if __name__ == '__main__':
    verify_utils.run_main(verify)
