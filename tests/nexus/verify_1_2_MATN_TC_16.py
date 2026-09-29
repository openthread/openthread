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


def verify(pv):
    # 5.10.12 MATN-TC-16: Large number of multicast group subscriptions to BBR
    #
    # 5.10.12.1 Topology
    # - BR_1 (DUT)
    # - Router
    # - Host
    #
    # 5.10.12.2 Purpose & Description
    # The purpose of this test case is to verify that the Primary BBR can handle a large number (75) of subscriptions
    #   to different multicast groups. Multicast registrations are performed each time with 15 multicast addresses
    #   per registration request.
    #
    # Spec Reference | V1.3.0 Section
    # ---------------|---------------
    # Multicast      | 5.10.12

    pkts = pv.pkts
    pv.summary.show()

    BR_1 = pv.vars['BR_1']
    BR_1_RLOC = pv.vars['BR_1_RLOC']
    BR_1_RLOC16 = pv.vars['BR_1_RLOC16']
    ROUTER = pv.vars['ROUTER']
    MA2 = Ipv6Addr(pv.vars['MA2'])

    NUM_BATCHES = 5
    NUM_ADDRESSES_PER_BATCH = 15
    MLR_URI = '/n/mr'
    ST_MLR_SUCCESS = 0

    # Step 0
    # - Device: N/A
    # - Description: Topology formation – BR_1 (DUT), Router
    # - Pass Criteria:
    #   - N/A
    print("Step 0: Topology formation – BR_1 (DUT), Router")

    for i in range(1, NUM_BATCHES + 1):
        mas_i = [
            Ipv6Addr(pv.vars['MAS%d' % ((i - 1) * NUM_ADDRESSES_PER_BATCH + j)])
            for j in range(NUM_ADDRESSES_PER_BATCH)
        ]

        # Step 1
        # - Device: Router
        # - Description: Harness instructs the device to register for 15 multicast addresses, MASi. The device
        #   automatically unicasts an MLR.req CoAP request to the DUT (BR_1) as follows:
        #   coap://[<PBBR ALOC>]:MM/n/mr Where the payload contains: IPv6 Addresses TLV: MASi (15 addresses)
        # - Pass Criteria:
        #   - N/A
        print("Step 1: Router registers 15 multicast addresses, MASi.")
        pkts.filter_wpan_src64(ROUTER).\
            filter_coap_request(MLR_URI).\
            filter(lambda p: all(addr in p.coap.tlv.ipv6_address for addr in mas_i)).\
            must_next()

        # Step 2
        # - Device: BR_1 (DUT)
        # - Description: Automatically responds to the multicast registration.
        # - Pass Criteria:
        #   - The DUT MUST unicast an MLR.rsp CoAP response to Router_1 as follows: 2.04 changed
        #   - Where the payload contains: Status TLV: 0 [ST_MLR_SUCCESS]
        print("Step 2: BR_1 (DUT) automatically responds to the multicast registration.")
        pkts.filter_wpan_src64(BR_1).\
            filter_coap_ack(MLR_URI).\
            filter(lambda p: p.coap.tlv.status == ST_MLR_SUCCESS).\
            must_next()

        # Step 3
        # - Device: BR_1 (DUT)
        # - Description: Auotmatically informs any other BBRs on the network of the multicast registrations.
        # - Pass Criteria:
        #   - The DUT MUST multicast a BMLR.ntf CoAP request to the Backbone Link, as follows:
        #     coap://[<All network BBRs multicast>]:BB/b/bmr
        #   - Where the payload contains: IPv6 Addresses TLV: MASi (15 addresses)
        #   - Timeout TLV: default MLR timeout of BR_1
        print("Step 3: BR_1 (DUT) automatically informs any other BBRs on the network of the multicast registrations.")
        # Checks for BLMR.ntf (BMLR.ntf) are intentionally skipped.

        # Step 4
        # - Device: BR_1 (DUT)
        # - Description: Automatically multicasts an MLDv2 message. The MLDv2 message may also be sent as multiple
        #   MLDv2 messages with content distributed across these multiple messages.
        # - Pass Criteria:
        #   - The DUT MUST multicast an MLDv2 message of type “Version 2 Multicast Listener Report” (see [RFC 3810]
        #     Section 5.2).
        #   - Where: Nr of Mcast Address Records (M): >= 15
        #   - Multicast Address Record [j]: See below
        #   - Each of the j := 0 … 14 Multicast Address Record containing an address of the set MASi contains the
        #     following: Record Type: 4 (CHANGE_TO_EXCLUDE_MODE), Number of Sources (N): 0, Multicast Address: MASi[j]
        #   - Alternatively, the DUT MAY also send multiple of above messages each with a portion of the 15
        #     addresses MASi. In this case the Nr of Mcast Address Records can be < 15 but the sum over all messages
        #     MUST be >= 15.
        print("Step 4: BR_1 (DUT) automatically multicasts an MLDv2 message.")
        # Checks for MLDv2 are intentionally skipped.

    for i in range(1, NUM_BATCHES + 1):
        mas_i_j = Ipv6Addr(pv.vars['MAS%d' % ((i - 1) * NUM_ADDRESSES_PER_BATCH + (3 * i - 1))])

        # Step 5
        # - Device: Host
        # - Description: Harness instructs the device to send an ICMPv6 Echo (ping) Request packet to the multicast
        #   address, MASi[ 3 * i - 1], on the backbone link.
        # - Pass Criteria:
        #   - N/A
        print("Step 5: Host sends an ICMPv6 Echo (ping) Request packet to the multicast address, MASi[ 3 * i - 1].")
        pkts.filter_ping_request().\
            filter_ipv6_dst(mas_i_j).\
            must_next()

        # Step 6
        # - Device: BR_1 (DUT)
        # - Description: Automatically forwards the ping request packet to its Thread Network.
        # - Pass Criteria:
        #   - The DUT MUST forward the ICMPv6 Echo (ping) Rquest packet of the previous step to its Thread Network
        #     encapsulated in an MPL packet, where:
        #   - MPL Option: If Source outer IP header == BR_1 RLOC Then S == 0 Else S == 1 and seed-id == BR_1 RLOC16
        print("Step 6: BR_1 (DUT) automatically forwards the ping request packet to its Thread Network.")

        def check_step6(p):
            if p.ipv6.src == BR_1_RLOC:
                return p.ipv6.opt.mpl.flag.s == 0
            else:
                return p.ipv6.opt.mpl.flag.s == 1 and \
                       p.ipv6.opt.mpl.seed_id == Bytes(struct.pack('>H', BR_1_RLOC16))

        pkts.filter_wpan_src64(BR_1).\
            filter_ping_request().\
            filter(lambda p: p.ipv6.dst == 'ff03::fc' or p.ipv6.dst == mas_i_j).\
            filter(check_step6).\
            must_next()

    # Step 7
    # - Device: Host
    # - Description: Harness instructs the device to multicast a packet to the multicast addresses, MA2.
    # - Pass Criteria:
    #   - N/A
    print("Step 7: Host multicasts a packet to the multicast addresses, MA2.")
    pkts.filter_ping_request().\
        filter_ipv6_dst(MA2).\
        must_next()

    # Step 8
    # - Device: BR_1 (DUT)
    # - Description: Does not forward the packet to its Thread Network.
    # - Pass Criteria:
    #   - The DUT MUST NOT forward the packet with multicast address, MA2, to the Thread Network.
    print("Step 8: BR_1 (DUT) does not forward the packet to its Thread Network.")
    pkts.copy().\
        filter_wpan_src64(BR_1).\
        filter_ping_request().\
        filter(lambda p: p.ipv6.dst == 'ff03::fc' or p.ipv6.dst == MA2).\
        must_not_next()


if __name__ == '__main__':
    verify_utils.run_main(verify)
