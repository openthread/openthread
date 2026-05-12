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
    # 5.10.7 MATN-TC-09: Failure of Primary BBR – Outbound Multicast
    #
    # 5.10.7.1 Topology
    # - BR_1
    # - BR_2 (DUT)
    # - Router
    #
    # 5.10.7.2 Purpose & Description
    # Verify that a Secondary BBR can take over forwarding of outbound multicast transmissions from the Thread Network
    #   when the Primary BBR disconnects. The Secondary in that case becomes Primary.
    #
    # Spec Reference | V1.2 Section
    # ---------------|-------------
    # Multicast      | 5.10.1

    pkts = pv.pkts
    vars = pv.vars
    pv.summary.show()

    ECHO_ID1 = 0x1234
    ECHO_ID2 = 0x1235

    # Step 0
    # - Device: N/A
    # - Description: Topology formation – BR_1, Router, BR_2 (DUT)
    # - Pass Criteria:
    #   - N/A
    print("Step 0: Topology formation – BR_1, Router, BR_2 (DUT)")

    # Step 1
    # - Device: Router
    # - Description: Harness instructs the device to send a ICMPv6 Echo (ping) Request packet to the multicast address,
    #   MA1, encapsulated in an MPL packet
    # - Pass Criteria:
    #   - N/A
    print("Step 1: Router sends ICMPv6 Echo (ping) Request packet to MA1")
    pkts.filter_ping_request(identifier=ECHO_ID1).\
        filter_wpan().\
        filter(lambda p: p.ipv6.dst == Ipv6Addr(vars['MA1']) or\
               p.ipv6.dst == 'ff03::fc').\
        must_next()

    # Step 2
    # - Device: BR_1
    # - Description: Automatically forwards the multicast ping request packet with multicast address, MA1, to the LAN.
    # - Pass Criteria:
    #   - N/A
    print("Step 2: BR_1 forwards the multicast ping request packet to the LAN")
    pkts.filter_ping_request(identifier=ECHO_ID1).\
        filter(lambda p: not p.wpan).\
        filter(lambda p: p.eth.src == vars['BR_1_ETH']).\
        filter(lambda p: p.ipv6.dst == Ipv6Addr(vars['MA1'])).\
        must_next()

    # Step 3
    # - Device: BR_2 (DUT)
    # - Description: Does not forward the multicast ping request packet to the LAN.
    # - Pass Criteria:
    #   - The DUT MUST NOT forward the multicast ICMPv6 Echo (ping) Request packet with multicast address, MA1, to the
    #     LAN.
    print("Step 3: BR_2 (DUT) does not forward the multicast ping request packet to the LAN")
    pkts.copy().\
        filter_ping_request(identifier=ECHO_ID1).\
        filter(lambda p: not p.wpan).\
        filter(lambda p: p.eth.src == vars['BR_2_ETH']).\
        filter(lambda p: p.ipv6.dst == Ipv6Addr(vars['MA1'])).\
        must_not_next()

    # Step 4a
    # - Device: BR_1
    # - Description: Harness instructs the device to power-down
    # - Pass Criteria:
    #   - N/A
    print("Step 4a: BR_1 powers down")

    # Step 4b: BR_2 (DUT) becomes Leader/PBBR
    # - Device: BR_2 (DUT)
    # - Description: Detects the missing Primary BBR and automatically becomes the Leader/PBBR of the Thread Network,
    #   distributing its BBR dataset.
    # - Pass Criteria:
    #   - The DUT MUST become the Leader of the Thread Network by checking the Leader data.
    #   - Router MUST receive the new BBR Dataset from BR_2, where:
    #   - • RLOC16 in Server TLV == The RLOC16 of BR_2
    #   - All fields in the BBR Dataset Service TLV MUST contain valid values.
    print("Step 4b: BR_2 (DUT) becomes Leader/PBBR")
    adv_pkts = pkts.copy()
    adv_pkts.filter_wpan_src64(vars['BR_2']).\
        filter_mle_cmd(consts.MLE_ADVERTISEMENT).\
        filter(lambda p: p.mle.tlv.leader_data.router_id == vars['BR_2_RLOC16'] >> 10).\
        must_next()

    dr_pkts = pkts.copy()
    dr_pkts.filter_wpan_src64(vars['BR_2']).\
        filter_mle_cmd(consts.MLE_DATA_RESPONSE).\
        filter_has_bbr_dataset().\
        filter(lambda p: vars['BR_2_RLOC16'] in p.thread_nwd.tlv.server_16).\
        must_next()

    pkts.index = pv.max_index(adv_pkts.index, dr_pkts.index)

    # Step 5
    # - Device: Router
    # - Description: Harness instructs the device to send a ICMPv6 Echo (ping) Request packet to the multicast address,
    #   MA1, encapsulated in an MPL packet.
    # - Pass Criteria:
    #   - N/A
    print("Step 5: Router sends ICMPv6 Echo (ping) Request packet to MA1")
    pkts.filter_ping_request(identifier=ECHO_ID2).\
        filter_wpan().\
        filter(lambda p: p.ipv6.dst == Ipv6Addr(vars['MA1']) or\
               p.ipv6.dst == 'ff03::fc').\
        must_next()

    # Step 6
    # - Device: BR_2 (DUT)
    # - Description: Automatically forwards the multicast ping request packet to the LAN.
    # - Pass Criteria:
    #   - The DUT MUST forward the multicast ICMPv6 Echo (ping) Request packet with multicast address, MA1, to the LAN.
    print("Step 6: BR_2 (DUT) forwards the multicast ping request packet to the LAN")
    pkts.filter_ping_request(identifier=ECHO_ID2).\
        filter(lambda p: not p.wpan).\
        filter(lambda p: p.eth.src == vars['BR_2_ETH']).\
        filter(lambda p: p.ipv6.dst == Ipv6Addr(vars['MA1'])).\
        must_next()


if __name__ == '__main__':
    verify_utils.run_main(verify)
