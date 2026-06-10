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
from pktverify.addrs import Ipv6Addr

ST_MLR_SUCCESS = 0
TIMEOUT_TLV = 11
COMMISSIONER_SESSION_ID_TLV = 15


def verify(pv):
    # 5.10.3 MATN-TC-03: Thread device incorrectly attempts Commissioners (de)registration functions
    #
    # 5.10.3.1 Topology
    # - BR_1 (DUT)
    # - BR_2
    # - Router
    # - Host
    #
    # 5.10.3.2 Purpose & Description
    # Verify that a Primary BBR ignores a Timeout TLV if included in an MLR.req that does not contain a Commissioner
    #   Session ID TLV nor is signed by a Commissioner.
    #
    # Spec Reference   | V1.2 Section
    # -----------------|-------------
    # Multicast Reg.   | 5.10.3

    pkts = pv.pkts
    vars = pv.vars

    # Step 0
    # - Device: Topology
    # - Description: Topology formation - DUT, BR_2, Router
    # - Pass Criteria:
    #   - N/A
    print("Step 0: Topology formation - DUT, BR_2, Router")

    # Step 1
    # - Device: Router
    # - Description: Harness instructs the device to register the multicast address MA1
    # - Pass Criteria:
    #   - N/A
    print("Step 1: Router registers the multicast address MA1")
    pkts.filter_coap_request('/n/mr').\
        filter(lambda p: vars['MA1'] in p.coap.tlv.ipv6_address).\
        must_next()

    # Step 2
    # - Device: Router
    # - Description: Harness instructs the device to deregister the multicast address, MA1, at BR_1. Router unicasts
    #   an MLR.req CoAP request to BR_1 as follows: coap://[<BR_1 RLOC>]:MM/n/mr Where the payload contains: IPv6
    #   Addresses TLV: MA1, Timeout TLV the value 0
    # - Pass Criteria:
    #   - N/A
    print("Step 2: Router attempts to deregister MA1 with Timeout 0")
    pkts.filter_coap_request('/n/mr').\
        filter(lambda p: vars['MA1'] in p.coap.tlv.ipv6_address).\
        filter(lambda p: TIMEOUT_TLV in p.coap.tlv.type).\
        filter(lambda p: p.coap.tlv.timeout == 0).\
        filter(lambda p: COMMISSIONER_SESSION_ID_TLV not in p.coap.tlv.type).\
        must_next()

    # Step 3
    # - Device: BR_1 (DUT)
    # - Description: Automatically responds to the multicast registration successfully, ignoring the timeout value.
    # - Pass Criteria:
    #   - For DUT=BR_1:
    #   - The DUT MUST unicast an MLR.rsp CoAP response to Router as follows: 2.04 changed
    #   - Where the payload contains: Status TLV: 0 [ST_MLR_SUCCESS]
    print("Step 3: BR_1 responds with Success, ignoring the timeout value.")
    pkts.filter_coap_ack('/n/mr').\
        filter(lambda p: p.coap.tlv.status == ST_MLR_SUCCESS).\
        must_next()

    # Step 4
    # - Device: Host
    # - Description: Harness instructs the device to send an ICMPv6 Echo (ping) Request packet to the multicast
    #   address, MA1.
    # - Pass Criteria:
    #   - N/A
    print("Step 4: Host sends an ICMPv6 Echo (ping) Request packet to the multicast address, MA1.")
    pkts.filter_ping_request().\
        filter(lambda p: p.ipv6.dst == Ipv6Addr(vars['MA1'])).\
        filter(lambda p: p.ipv6.src == Ipv6Addr(vars['HOST_BACKBONE_ULA'])).\
        must_next()

    # Step 5
    # - Device: BR_1 (DUT)
    # - Description: Automatically forwards the ping request packet to its Thread Network.
    # - Pass Criteria:
    #   - The DUT MUST forward the ICMPv6 Echo (ping) Request packet with multicast address, MA1, to its Thread
    #     Network encapsulated in an MPL packet.
    print("Step 5: BR_1 forwards the ping request packet to its Thread Network.")
    pkts.filter_wpan_src64(vars['BR_1']).\
        filter_ping_request().\
        filter(lambda p: p.ipv6.dst == 'ff03::fc').\
        must_next()

    # Step 6
    # - Device: Router
    # - Description: Receives the MPL packet containing an encapsulated ping request packet to MA1, sent by Host, and
    #   automatically unicasts an ICMPv6 Echo (ping) Reply packet back to Host.
    # - Pass Criteria:
    #   - N/A
    print("Step 6: Router responds with an ICMPv6 Echo (ping) Reply packet back to Host.")
    pkts.filter_ping_reply().\
        filter(lambda p: p.ipv6.dst == Ipv6Addr(vars['HOST_BACKBONE_ULA'])).\
        must_next()


if __name__ == '__main__':
    verify_utils.run_main(verify)
