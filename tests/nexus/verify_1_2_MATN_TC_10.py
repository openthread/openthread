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
    # 5.10.8 MATN-TC-10: Failure of Primary BBR – Inbound Multicast
    #
    # 5.10.8.1 Topology
    # - BR_1
    # - BR_2 (DUT)
    # - ROUTER
    # - HOST
    #
    # 5.10.8.2 Purpose & Description
    # Verify that a Secondary BBR can take over forwarding of inbound multicast transmissions to the Thread Network
    #   when the Primary BBR disconnects. The Secondary in that case becomes Primary.
    #
    # Spec Reference | V1.2 Section
    # ---------------|-------------
    # Multicast      | 5.10.1

    pkts = pv.pkts
    vars = pv.vars
    pv.summary.show()

    MA1 = Ipv6Addr(vars['MA1'])
    ECHO_ID1 = 0x1234
    ECHO_ID2 = 0x1235

    # Step 0
    # - Device: N/A
    # - Description: Topology formation – BR_1, Router, BR_2 (DUT)
    # - Pass Criteria:
    #   - N/A
    print("Step 0: Topology formation – BR_1, ROUTER, BR_2 (DUT)")

    # Step 1
    # - Device: Router
    # - Description: Harness instructs the device to register multicast address, MA1, at BR_1.
    # - Pass Criteria:
    #   - N/A
    print("Step 1: ROUTER registers multicast address, MA1, at BR_1.")
    pkts.filter_wpan_src64(vars['ROUTER']).\
        filter_coap_request("/n/mr").\
        filter(lambda p: MA1 in p.coap.tlv.ipv6_address).\
        must_next()

    # Step 2
    # - Device: BR_1
    # - Description: Automatically responds to Router’s multicast registration.
    # - Pass Criteria:
    #   - N/A
    print("Step 2: BR_1 automatically responds to ROUTER’s multicast registration.")
    pkts.filter_coap_ack("/n/mr").\
        filter(lambda p: p.coap.tlv.status == 0).\
        must_next()

    # Step 3
    # - Device: BR_1
    # - Description: Automatically informs other BBRs on the network of the multicast registration. Informative:
    #   Multicasts a BMLR.ntf CoAP request to the Backbone Link including to BR_2, as follows:
    #   coap://[<All network BBR multicast>]:BB/b/bmr Where the payload contains: IPv6 Addresses TLV: MA1, Timeout TLV:
    #   3600
    # - Pass Criteria:
    #   - N/A
    print("Step 3: BR_1 automatically informs other BBRs on the network (SKIPPED).")
    # Step 3 is intentionally skipped: OpenThread sends BMLR.ntf, but we skip the BLMR.ntf check as per instructions.

    # Step 4
    # - Device: BR_2 (DUT)
    # - Description: Receives BMLR.ntf and optionally updates its Backup Multicast Listener Table.
    # - Pass Criteria:
    #   - None
    print("Step 4: BR_2 (DUT) receives BMLR.ntf.")

    # Step 5
    # - Device: Host
    # - Description: Harness instructs the device to send an ICMPv6 Echo (ping) Request packet to the multicast
    #   address, MA1.
    # - Pass Criteria:
    #   - N/A
    print("Step 5: HOST sends an ICMPv6 Echo (ping) Request packet to the multicast address, MA1.")
    pkts.filter_eth_src(vars['HOST_ETH']).\
        filter_ping_request(identifier=ECHO_ID1).\
        filter(lambda p: p.ipv6.dst == MA1).\
        must_next()

    # Step 6
    # - Device: BR_1
    # - Description: Automatically forwards the ping request packet to its Thread Network. Informative: Forwards the
    #   ping request packet with multicast address, MA1, to its Thread Network encapsulated in an MPL packet, where:
    #   MPL Option: If Source outer IP header == BR_1 RLOC Then S == 0 Else S == 1 and seed-id == BR_1 RLOC16
    # - Pass Criteria:
    #   - N/A
    print("Step 6: BR_1 automatically forwards the ping request packet to its Thread Network.")
    pkts.filter_wpan_src64(vars['BR_1']).\
        filter_ping_request(identifier=ECHO_ID1).\
        filter_AMPLFMA().\
        must_next()

    # Step 7
    # - Device: BR_2 (DUT)
    # - Description: Does not forward the ping request packet to its Thread Network.
    # - Pass Criteria:
    #   - The DUT MUST NOT forward the ICMPv6 Echo (ping) Request packet with multicast address, MA1, to the Thread
    #     Network, in whatever way, e.g. as a new MPL packet listing itself as the MPL Seed.
    print("Step 7: BR_2 (DUT) does not forward the ping request packet to its Thread Network.")
    pkts.copy().\
        filter_wpan_src64(vars['BR_2']).\
        filter_ping_request(identifier=ECHO_ID1).\
        filter(lambda p: p.ipv6.src == Ipv6Addr(vars['BR_2_RLOC'])).\
        must_not_next()

    # Step 8
    # - Device: Router
    # - Description: Receives the MPL packet containing an encapsulated ping request packet to MA1, sent by Host, and
    #   unicasts an ICMPv6 Echo (ping) Reply packet back to Host.
    # - Pass Criteria:
    #   - N/A
    print("Step 8: ROUTER receives the MPL packet and unicasts an ICMPv6 Echo (ping) Reply.")
    # Router may reply over backbone directly in Nexus if it sees the destination GUA as on-link.
    pkts.filter(lambda p: p.wpan.src64 == vars['ROUTER'] or p.eth.src == vars['ROUTER_ETH']).\
        filter_ping_reply(identifier=ECHO_ID1).\
        must_next()

    # Step 8a
    # - Device: BR_1
    # - Description: Harness instructs the device to powerdown
    # - Pass Criteria:
    #   - N/A
    print("Step 8a: BR_1 powers down")

    # Step 8b
    # - Device: BR_2 (DUT)
    # - Description: Detects the missing Primary BBR and automatically becomes the Primary BBR of the Thread Network,
    #   distributing its BBR dataset.
    # - Pass Criteria:
    #   - The DUT MUST it distribute its BBR Dataset (with BR_2’s RLOC16) to Router.
    #   - Note the value of the Reregistration Delay of BR_2.
    print("Step 8b: BR_2 (DUT) becomes Primary BBR")
    pkts.filter_wpan_src64(vars['BR_2']).\
        filter_mle_cmd(consts.MLE_DATA_RESPONSE).\
        filter_has_bbr_dataset().\
        filter(lambda p: int(vars['BR_2_RLOC16']) in p.thread_nwd.tlv.server_16).\
        must_next()

    # Step 9
    # - Device: Router
    # - Description: Detects new BBR RLOC and automatically starts its re-registration timer.
    # - Pass Criteria:
    #   - N/A
    print("Step 9: ROUTER detects new BBR RLOC.")

    # Step 10
    # - Device: Host
    # - Description: Harness instructs the device to send a ICMPv6 Echo (ping) Request packet to the multicast address,
    #   MA1.
    # - Pass Criteria:
    #   - N/A
    print("Step 10: HOST sends an ICMPv6 Echo (ping) Request packet to the multicast address, MA1.")
    pkts.filter_eth_src(vars['HOST_ETH']).\
        filter_ping_request(identifier=ECHO_ID2).\
        filter(lambda p: p.ipv6.dst == MA1).\
        must_next()

    # Step 11
    # - Device: BR_2 (DUT)
    # - Description: Optionally forwards the ping request packet to its Thread Network. Note: forwarding may occur if
    #   Router did already register (step 14), or if the DUT kept a Backup Multicast Listeners Table which is an
    #   optional feature.
    # - Pass Criteria:
    #   - The DUT MAY forward the ICMPv6 Echo (ping) Request packet with multicast address, MA1, to its Thread Network
    #     encapsulated in an MPL packet, where:
    #   - MPL Option: If Source outer IP header == BR_1 RLOC Then S == 0 Else S == 1 and seed-id == BR_1 RLOC16
    print("Step 11: BR_2 (DUT) optionally forwards the ping request packet.")

    # Step 12
    # - Device: Router
    # - Description: Optional: Receives the multicast ping request packet and sends a ICMPv6 Echo (ping) Reply packet
    #   back to Host. Informative: Only if BR_2 forwarded in step 11: Receives the MPL packet containing an
    #   encapsulated ping request packet to MA1, sent by Host, and unicasts a ping response packet back to Host.
    # - Pass Criteria:
    #   - N/A
    print("Step 12: ROUTER optional: receives and sends a ping reply.")

    # Step 13
    # - Device: Router_1
    # - Description: Wait for the re-registration timer to expire (according to the value of the Reregistration Delay
    #   distributed in the BBR Dataset of the DUT (BR_2) and note earlier).
    # - Pass Criteria:
    #   - N/A
    print("Step 13: ROUTER waits for the re-registration timer to expire.")

    # Step 14
    # - Device: Router
    # - Description: Automatically re-registers for multicast address, MA1, at the DUT (BR_2). Note: this step may have
    #   happened already any time after step 9.
    # - Pass Criteria:
    #   - N/A
    print("Step 14: ROUTER automatically re-registers for multicast address, MA1, at the DUT (BR_2).")
    pkts.filter_wpan_src64(vars['ROUTER']).\
        filter_coap_request("/n/mr").\
        filter(lambda p: MA1 in p.coap.tlv.ipv6_address).\
        must_next()

    # Step 15
    # - Device: BR_2 (DUT)
    # - Description: Automatically responds to the multicast registration.
    # - Pass Criteria:
    #   - The DUT MUST unicast an MLR.rsp CoAP response to Router as follows: 2.04 changed
    #   - Where the payload contains: Status TLV: 0 [ST_MLR_SUCCESS]
    print("Step 15: BR_2 (DUT) automatically responds to the multicast registration.")
    pkts.filter_coap_ack("/n/mr").\
        filter(lambda p: p.coap.tlv.status == 0).\
        must_next()

    # Step 16
    # - Device: BR_2 (DUT)
    # - Description: Automatically informs other BBRs on the network of the multicast registration.
    # - Pass Criteria:
    #   - The DUT MUST multicast a BMLR.ntf CoAP request to the Backbone Link, as follows:
    #     coap://[<All network BBR multicast>]:BB/b/bmr
    #   - Where the payload contains: IPv6 Addresses TLV: MA1
    #   - Timeout TLV: MLR timeout of BR_2
    print("Step 16: BR_2 (DUT) automatically informs other BBRs on the network (SKIPPED).")
    # Step 16 is intentionally skipped: OpenThread sends BMLR.ntf, but we skip the BLMR.ntf check as per instructions.

    # Step 17
    # - Device: BR_1
    # - Description: Does not receive the message nor respond, as it is turned off.
    # - Pass Criteria:
    #   - N/A
    print("Step 17: BR_1 does not receive the message.")

    # Step 18
    # - Device: BR_2 (DUT)
    # - Description: Automatically multicasts an MLDv2 message.
    # - Pass Criteria:
    #   - The DUT MUST multicast an MLDv2 message of type “Version 2 Multicast Listener Report” (see [RFC 3810] Section
    #     5.2). Where: Nr of Mcast Address Records (M): at least 1
    #   - Multicast Address Record [1]: See below
    #   - The Multicast Address Record contains the following: Record Type: 4 (CHANGE_TO_EXCLUDE_MODE)
    #   - Number of Sources (N): 0
    #   - Multicast Address: MA1
    print("Step 18: BR_2 (DUT) automatically multicasts an MLDv2 message (SKIPPED).")
    # Step 18 is intentionally skipped: OpenThread does not implement MLDv2.

    # Step 19
    # - Device: BR_1
    # - Description: Does not receive the message nor does it respond, as it is turned off.
    # - Pass Criteria:
    #   - N/A
    print("Step 19: BR_1 does not receive.")


if __name__ == '__main__':
    verify_utils.run_main(verify)
