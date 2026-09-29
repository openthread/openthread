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
    # 8.2. [1.4] [CERT] Multi-hop routing with device (in the path) with different radio types and MTUs
    #
    # 8.2.1. Purpose
    # This test covers the use of 6LoWPAN “Mesh Header” messages (messages sent over multi-hop) when
    # underlying routes (in the path) can support different radio link types with different frame MTU sizes.
    # Different types of devices (15.4, TREL, Router, End Device) are connected to the DUT.
    #
    # Spec Reference           | V1.1 Section | V1.3.0 Section
    # -------------------------|--------------|---------------
    # TREL / Multi-radio Links | N/A          | 8.2

    pkts = pv.pkts
    pv.summary.show()

    ROUTER_1 = pv.vars['ROUTER_1']  # DUT
    LEADER = pv.vars['LEADER']
    ROUTER_2 = pv.vars['ROUTER_2']
    ED_1 = pv.vars['ED_1']
    ROUTER_3 = pv.vars['ROUTER_3']
    ROUTER_4 = pv.vars['ROUTER_4']

    ROUTER_1_RLOC16 = pv.vars['ROUTER_1_RLOC16']
    LEADER_RLOC16 = pv.vars['LEADER_RLOC16']
    ROUTER_2_RLOC16 = pv.vars['ROUTER_2_RLOC16']
    ROUTER_3_RLOC16 = pv.vars['ROUTER_3_RLOC16']
    ROUTER_4_RLOC16 = pv.vars['ROUTER_4_RLOC16']
    ED_1_RLOC16 = pv.vars['ED_1_RLOC16']

    ROUTER_1_ETH = pv.vars['ROUTER_1_ETH']
    LEADER_ETH = pv.vars['LEADER_ETH']
    ROUTER_4_ETH = pv.vars['ROUTER_4_ETH']

    ROUTER_1_TREL_PORT = pv.vars['ROUTER_1_TREL_PORT']
    LEADER_TREL_PORT = pv.vars['LEADER_TREL_PORT']
    ROUTER_4_TREL_PORT = pv.vars['ROUTER_4_TREL_PORT']

    ROUTER_1_MLEID = pv.vars['ROUTER_1_MLEID']
    LEADER_MLEID = pv.vars['LEADER_MLEID']
    ROUTER_2_MLEID = pv.vars['ROUTER_2_MLEID']
    ED_1_MLEID = pv.vars['ED_1_MLEID']
    ROUTER_3_MLEID = pv.vars['ROUTER_3_MLEID']
    ROUTER_4_MLEID = pv.vars['ROUTER_4_MLEID']

    # Step 1
    # - Device: Leader, Router_1 (DUT), Router_2, ED_1, Router_3, Router_4
    # - Description (TREL-8.2): Form the topology. Wait for Router_1, Router_2, Router_3 and Router_4 to become
    #   routers. Note: any devices not connected by line in the topology figure MUST NOT be able to communicate
    #   directly via a Thread Link or TREL link and not be in each other’s neighbor table (e.g. this may be realized
    #   by using the allow/deny list mechanism). ED_1 MUST be configured as a non-sleepy MTD child (mode `rn`). Note:
    #   ED_1 MUST attach to the Router_1 (DUT) as its parent and not the other routers (e.g., can be realized by using
    #   denylist on ED_1). Note: The requirement for ED_1 to attach as a non-sleepy MTD child to Router_1 (DUT) is
    #   important to ensure messages from ED_1 are sent to its parent as is without mesh header encapsulation, to
    #   then be forwarded within the mesh and that the fragmentation and adding of 6LoWPAN “mesh header” is performed
    #   by Router_1 (DUT) - which is being verified in the next step.
    print("Step 1: Form the topology")
    pkts.copy().\
      filter_wpan_src64(ROUTER_2).\
      filter_wpan_dst64(ROUTER_1).\
      must_not_next()
    pkts.copy().\
      filter_wpan_src64(ROUTER_1).\
      filter_wpan_dst64(ROUTER_2).\
      must_not_next()

    # Step 2
    # - Device: ED_1
    # - Description (TREL-8.2): Harness instructs device to send a ping of payload size of 500 bytes or more to
    #   destination Router_2 using the ML-EID address of Router_2 as the destination address. Note: this verifies the
    #   DUT behavior of forwarding 6LoWPAN fragmented frames from a Child (incoming over 15.4) over multiple hops.
    # - Pass Criteria:
    #   - ED_1 MUST successfully receive a ping reply from Router_2, routed via DUT.
    #   - TREL UDP frames MUST be used for ping/ping-reply transport between DUT and Leader, and MUST be correct TREL
    #     frames.
    print("Step 2: ED_1 pings Router_2 ML-EID")
    # Path: ED_1 -(15.4)-> ROUTER_1 -(TREL)-> LEADER -(15.4)-> ROUTER_2
    # 1. Request from ED_1 to ROUTER_1 over 15.4
    _pkt_154_req = pkts.copy().\
      filter_ping_request().\
      filter_ipv6_src_dst(ED_1_MLEID, ROUTER_2_MLEID).\
      filter_wpan_src16_dst16(ED_1_RLOC16, ROUTER_1_RLOC16).\
      filter(lambda p: p.wpan).\
      must_next()

    _identifier = _pkt_154_req.icmpv6.echo.identifier

    # 2. Forwarded request on TREL (ROUTER_1 to LEADER)
    pkts.range(pkts.index).\
      filter_eth_src(ROUTER_1_ETH).\
      filter(lambda p: p.eth.dst == LEADER_ETH).\
      filter(lambda p: p.udp.dstport == LEADER_TREL_PORT).\
      filter(lambda p: p.trel.source_addr == ROUTER_1 and p.trel.destination_addr == LEADER).\
      filter(lambda p: p.trel.wpan.src16 == ROUTER_1_RLOC16 and p.trel.wpan.dst16 == LEADER_RLOC16).\
      must_next()

    # 3. Forwarded request on 15.4 (LEADER to ROUTER_2)
    pkts.range(pkts.index).\
      filter_ping_request(identifier=_identifier).\
      filter_ipv6_src_dst(ED_1_MLEID, ROUTER_2_MLEID).\
      filter_wpan_src16_dst16(LEADER_RLOC16, ROUTER_2_RLOC16).\
      filter(lambda p: p.wpan).\
      must_next()

    # 4. Reply from Router_2 to Leader over 15.4
    pkts.copy().\
      filter_ping_reply(identifier=_identifier).\
      filter_ipv6_src_dst(ROUTER_2_MLEID, ED_1_MLEID).\
      filter_wpan_src16_dst16(ROUTER_2_RLOC16, LEADER_RLOC16).\
      filter(lambda p: p.wpan).\
      must_next()

    # 5. Forwarded reply on TREL (LEADER to ROUTER_1)
    pkts.range(pkts.index).\
      filter_eth_src(LEADER_ETH).\
      filter(lambda p: p.eth.dst == ROUTER_1_ETH).\
      filter(lambda p: p.udp.dstport == ROUTER_1_TREL_PORT).\
      filter(lambda p: p.trel.source_addr == LEADER and p.trel.destination_addr == ROUTER_1).\
      filter(lambda p: p.trel.wpan.src16 == LEADER_RLOC16 and p.trel.wpan.dst16 == ROUTER_1_RLOC16).\
      must_next()

    # 6. Forwarded reply on 15.4 (ROUTER_1 to ED_1)
    pkts.range(pkts.index).\
      filter_ping_reply(identifier=_identifier).\
      filter_ipv6_src_dst(ROUTER_2_MLEID, ED_1_MLEID).\
      filter_wpan_src16_dst16(ROUTER_1_RLOC16, ED_1_RLOC16).\
      filter(lambda p: p.wpan).\
      must_next()

    # Step 3
    # - Device: Router_2
    # - Description (TREL-8.2): Harness instructs device to send a ping of payload size of 500 bytes or more from
    #   Router_2 to ED_1 using the ML-EID address of ED_1 as the destination address.
    # - Pass Criteria:
    #   - Router_2 MUST successfully receive a ping reply from ED_1, routed via the DUT.
    #   - TREL UDP frames MUST be used for ping/ping-reply transport between DUT and Leader, and MUST be correct TREL
    #     frames.
    print("Step 3: Router_2 pings ED_1 ML-EID")
    # Path: ROUTER_2 -(15.4)-> LEADER -(TREL)-> ROUTER_1 -(15.4)-> ED_1
    # 1. Request from ROUTER_2 to LEADER over 15.4
    _pkt_154_req = pkts.copy().\
      filter_ping_request().\
      filter_ipv6_src_dst(ROUTER_2_MLEID, ED_1_MLEID).\
      filter_wpan_src16_dst16(ROUTER_2_RLOC16, LEADER_RLOC16).\
      filter(lambda p: p.wpan).\
      must_next()

    _identifier = _pkt_154_req.icmpv6.echo.identifier

    # 2. Forwarded request on TREL (LEADER to ROUTER_1)
    pkts.range(pkts.index).\
      filter_eth_src(LEADER_ETH).\
      filter(lambda p: p.eth.dst == ROUTER_1_ETH).\
      filter(lambda p: p.udp.dstport == ROUTER_1_TREL_PORT).\
      filter(lambda p: p.trel.source_addr == LEADER and p.trel.destination_addr == ROUTER_1).\
      filter(lambda p: p.trel.wpan.src16 == LEADER_RLOC16 and p.trel.wpan.dst16 == ROUTER_1_RLOC16).\
      must_next()

    # 3. Forwarded request on 15.4 (ROUTER_1 to ED_1)
    pkts.range(pkts.index).\
      filter_ping_request(identifier=_identifier).\
      filter_ipv6_src_dst(ROUTER_2_MLEID, ED_1_MLEID).\
      filter_wpan_src16_dst16(ROUTER_1_RLOC16, ED_1_RLOC16).\
      filter(lambda p: p.wpan).\
      must_next()

    # 4. Reply from ED_1 to ROUTER_1 over 15.4
    pkts.copy().\
      filter_ping_reply(identifier=_identifier).\
      filter_ipv6_src_dst(ED_1_MLEID, ROUTER_2_MLEID).\
      filter_wpan_src16_dst16(ED_1_RLOC16, ROUTER_1_RLOC16).\
      filter(lambda p: p.wpan).\
      must_next()

    # 5. Forwarded reply on TREL (ROUTER_1 to LEADER)
    pkts.range(pkts.index).\
      filter_eth_src(ROUTER_1_ETH).\
      filter(lambda p: p.eth.dst == LEADER_ETH).\
      filter(lambda p: p.udp.dstport == LEADER_TREL_PORT).\
      filter(lambda p: p.trel.source_addr == ROUTER_1 and p.trel.destination_addr == LEADER).\
      filter(lambda p: p.trel.wpan.src16 == ROUTER_1_RLOC16 and p.trel.wpan.dst16 == LEADER_RLOC16).\
      must_next()

    # 6. Forwarded reply on 15.4 (LEADER to ROUTER_2)
    pkts.range(pkts.index).\
      filter_ping_reply(identifier=_identifier).\
      filter_ipv6_src_dst(ED_1_MLEID, ROUTER_2_MLEID).\
      filter_wpan_src16_dst16(LEADER_RLOC16, ROUTER_2_RLOC16).\
      filter(lambda p: p.wpan).\
      must_next()

    # Step 4
    # - Device: Router_4
    # - Description (TREL-8.2): Harness instructs device to send a ping of payload size of 500 bytes or more from
    #   Router_4 to destination Router_2 using the ML-EID address of Router_2 as the destination address. Warning: see
    #   TESTPLAN-626 for an open issue (TBD) in this step. Note: this intends to verify the DUT behavior of
    #   forwarding 6LoWPAN “Mesh Header” messages, incoming over TREL, over multi-hop routes.
    # - Pass Criteria:
    #   - Router_4 MUST successfully receive a ping reply from Router_2, routed via the DUT.
    #   - TREL UDP frames MUST be used for ping/ping-reply transport between DUT and Leader, and MUST be correct TREL
    #     frames.
    #   - TREL UDP frames MUST be used for ping/ping-reply transport between the DUT and Router_4, and MUST be correct
    #     TREL frames.
    print("Step 4: Router_4 pings Router_2 ML-EID")
    # Path: ROUTER_4 -(TREL)-> ROUTER_1 -(TREL)-> LEADER -(15.4)-> ROUTER_2
    # 1. Request from ROUTER_4 to ROUTER_1 over TREL
    pkts.copy().\
      filter_eth_src(ROUTER_4_ETH).\
      filter(lambda p: p.eth.dst == ROUTER_1_ETH).\
      filter(lambda p: p.udp.dstport == ROUTER_1_TREL_PORT).\
      filter(lambda p: p.trel.source_addr == ROUTER_4 and p.trel.destination_addr == ROUTER_1).\
      filter(lambda p: p.trel.wpan.src16 == ROUTER_4_RLOC16 and p.trel.wpan.dst16 == ROUTER_1_RLOC16).\
      must_next()

    # 2. Forwarded request from ROUTER_1 to LEADER over TREL
    pkts.range(pkts.index).\
      filter_eth_src(ROUTER_1_ETH).\
      filter(lambda p: p.eth.dst == LEADER_ETH).\
      filter(lambda p: p.udp.dstport == LEADER_TREL_PORT).\
      filter(lambda p: p.trel.source_addr == ROUTER_1 and p.trel.destination_addr == LEADER).\
      filter(lambda p: p.trel.wpan.src16 == ROUTER_1_RLOC16 and p.trel.wpan.dst16 == LEADER_RLOC16).\
      must_next()

    # 3. Forwarded request on 15.4 (LEADER to ROUTER_2)
    _pkt_154_req = pkts.range(pkts.index).\
      filter_ping_request().\
      filter_ipv6_src_dst(ROUTER_4_MLEID, ROUTER_2_MLEID).\
      filter_wpan_src16_dst16(LEADER_RLOC16, ROUTER_2_RLOC16).\
      filter(lambda p: p.wpan).\
      must_next()

    _identifier = _pkt_154_req.icmpv6.echo.identifier

    # 4. Reply from Router_2 to Leader over 15.4
    pkts.copy().\
      filter_ping_reply(identifier=_identifier).\
      filter_ipv6_src_dst(ROUTER_2_MLEID, ROUTER_4_MLEID).\
      filter_wpan_src16_dst16(ROUTER_2_RLOC16, LEADER_RLOC16).\
      filter(lambda p: p.wpan).\
      must_next()

    # 5. Forwarded reply on TREL (LEADER to ROUTER_1)
    pkts.range(pkts.index).\
      filter_eth_src(LEADER_ETH).\
      filter(lambda p: p.eth.dst == ROUTER_1_ETH).\
      filter(lambda p: p.udp.dstport == ROUTER_1_TREL_PORT).\
      filter(lambda p: p.trel.source_addr == LEADER and p.trel.destination_addr == ROUTER_1).\
      filter(lambda p: p.trel.wpan.src16 == LEADER_RLOC16 and p.trel.wpan.dst16 == ROUTER_1_RLOC16).\
      must_next()

    # 6. Forwarded reply on TREL (ROUTER_1 to ROUTER_4)
    pkts.range(pkts.index).\
      filter_eth_src(ROUTER_1_ETH).\
      filter(lambda p: p.eth.dst == ROUTER_4_ETH).\
      filter(lambda p: p.udp.dstport == ROUTER_4_TREL_PORT).\
      filter(lambda p: p.trel.source_addr == ROUTER_1 and p.trel.destination_addr == ROUTER_4).\
      filter(lambda p: p.trel.wpan.src16 == ROUTER_1_RLOC16 and p.trel.wpan.dst16 == ROUTER_4_RLOC16).\
      must_next()

    # Step 5
    # - Device: Router_2
    # - Description (TREL-8.2): Harness instructs device to send a ping of payload size of 500 bytes or more from
    #   Router_2 to Router_4 using the ML-EID address of Router_4 as the destination address.
    # - Pass Criteria:
    #   - Router_2 MUST successfully receive a ping reply from Router_4, routed via the DUT.
    #   - TREL UDP frames MUST be used for ping/ping-reply transport between DUT and Leader, and MUST be correct TREL
    #     frames.
    #   - TREL UDP frames MUST be used for ping/ping-reply transport between the DUT and Router_4, and MUST be correct
    #     TREL frames.
    print("Step 5: Router_2 pings Router_4 ML-EID")
    # Path: ROUTER_2 -(15.4)-> LEADER -(TREL)-> ROUTER_1 -(TREL)-> ROUTER_4
    # 1. Request from ROUTER_2 to LEADER over 15.4
    _pkt_154_req = pkts.copy().\
      filter_ping_request().\
      filter_ipv6_src_dst(ROUTER_2_MLEID, ROUTER_4_MLEID).\
      filter_wpan_src16_dst16(ROUTER_2_RLOC16, LEADER_RLOC16).\
      filter(lambda p: p.wpan).\
      must_next()

    _identifier = _pkt_154_req.icmpv6.echo.identifier

    # 2. Forwarded request from LEADER to ROUTER_1 over TREL
    pkts.range(pkts.index).\
      filter_eth_src(LEADER_ETH).\
      filter(lambda p: p.eth.dst == ROUTER_1_ETH).\
      filter(lambda p: p.udp.dstport == ROUTER_1_TREL_PORT).\
      filter(lambda p: p.trel.source_addr == LEADER and p.trel.destination_addr == ROUTER_1).\
      filter(lambda p: p.trel.wpan.src16 == LEADER_RLOC16 and p.trel.wpan.dst16 == ROUTER_1_RLOC16).\
      must_next()

    # 3. Forwarded request from ROUTER_1 to ROUTER_4 over TREL
    pkts.range(pkts.index).\
      filter_eth_src(ROUTER_1_ETH).\
      filter(lambda p: p.eth.dst == ROUTER_4_ETH).\
      filter(lambda p: p.udp.dstport == ROUTER_4_TREL_PORT).\
      filter(lambda p: p.trel.source_addr == ROUTER_1 and p.trel.destination_addr == ROUTER_4).\
      filter(lambda p: p.trel.wpan.src16 == ROUTER_1_RLOC16 and p.trel.wpan.dst16 == ROUTER_4_RLOC16).\
      must_next()

    # 4. Reply on TREL (ROUTER_4 to ROUTER_1)
    pkts.range(pkts.index).\
      filter_eth_src(ROUTER_4_ETH).\
      filter(lambda p: p.eth.dst == ROUTER_1_ETH).\
      filter(lambda p: p.udp.dstport == ROUTER_1_TREL_PORT).\
      filter(lambda p: p.trel.source_addr == ROUTER_4 and p.trel.destination_addr == ROUTER_1).\
      filter(lambda p: p.trel.wpan.src16 == ROUTER_4_RLOC16 and p.trel.wpan.dst16 == ROUTER_1_RLOC16).\
      must_next()

    # 5. Forwarded reply on TREL (ROUTER_1 to LEADER)
    pkts.range(pkts.index).\
      filter_eth_src(ROUTER_1_ETH).\
      filter(lambda p: p.eth.dst == LEADER_ETH).\
      filter(lambda p: p.udp.dstport == LEADER_TREL_PORT).\
      filter(lambda p: p.trel.source_addr == ROUTER_1 and p.trel.destination_addr == LEADER).\
      filter(lambda p: p.trel.wpan.src16 == ROUTER_1_RLOC16 and p.trel.wpan.dst16 == LEADER_RLOC16).\
      must_next()

    # 6. Forwarded reply on 15.4 (LEADER to ROUTER_2)
    pkts.range(pkts.index).\
      filter_ping_reply(identifier=_identifier).\
      filter_ipv6_src_dst(ROUTER_4_MLEID, ROUTER_2_MLEID).\
      filter_wpan_src16_dst16(LEADER_RLOC16, ROUTER_2_RLOC16).\
      filter(lambda p: p.wpan).\
      must_next()

    # Step 6
    # - Device: Router_3
    # - Description (TREL-8.2): Harness instructs device to send a ping of payload size of 500 bytes or more from
    #   Router_3 to destination Router_2 using the ML-EID address of Router_2 as the destination address. Note: this
    #   verifies the DUT behavior of forwarding 6LoWPAN “Mesh Header” messages, incoming over 15.4, over multi-hop
    #   routes.
    # - Pass Criteria:
    #   - Router_3 MUST successfully receive a ping reply from Router_2, routed via the DUT.
    #   - TREL UDP frames MUST be used for ping/ping-reply transport between the DUT and Leader, and MUST be correct
    #     TREL frames.
    print("Step 6: Router_3 pings Router_2 ML-EID")
    # Path: ROUTER_3 -(15.4)-> ROUTER_1 -(TREL)-> LEADER -(15.4)-> ROUTER_2
    # 1. Request from ROUTER_3 to ROUTER_1 over 15.4
    _pkt_154_req = pkts.copy().\
      filter_ping_request().\
      filter_ipv6_src_dst(ROUTER_3_MLEID, ROUTER_2_MLEID).\
      filter_wpan_src16_dst16(ROUTER_3_RLOC16, ROUTER_1_RLOC16).\
      filter(lambda p: p.wpan).\
      must_next()

    _identifier = _pkt_154_req.icmpv6.echo.identifier

    # 2. Check forwarded request on TREL from ROUTER_1 to LEADER
    pkts.range(pkts.index).\
      filter_eth_src(ROUTER_1_ETH).\
      filter(lambda p: p.eth.dst == LEADER_ETH).\
      filter(lambda p: p.udp.dstport == LEADER_TREL_PORT).\
      filter(lambda p: p.trel.source_addr == ROUTER_1 and p.trel.destination_addr == LEADER).\
      filter(lambda p: p.trel.wpan.src16 == ROUTER_1_RLOC16 and p.trel.wpan.dst16 == LEADER_RLOC16).\
      must_next()

    # 3. Forwarded request on 15.4 (LEADER to ROUTER_2)
    pkts.range(pkts.index).\
      filter_ping_request(identifier=_identifier).\
      filter_ipv6_src_dst(ROUTER_3_MLEID, ROUTER_2_MLEID).\
      filter_wpan_src16_dst16(LEADER_RLOC16, ROUTER_2_RLOC16).\
      filter(lambda p: p.wpan).\
      must_next()

    # 4. Reply from Router_2 to Leader over 15.4
    pkts.copy().\
      filter_ping_reply(identifier=_identifier).\
      filter_ipv6_src_dst(ROUTER_2_MLEID, ROUTER_3_MLEID).\
      filter_wpan_src16_dst16(ROUTER_2_RLOC16, LEADER_RLOC16).\
      filter(lambda p: p.wpan).\
      must_next()

    # 5. Check forwarded reply on TREL from LEADER to ROUTER_1
    pkts.range(pkts.index).\
      filter_eth_src(LEADER_ETH).\
      filter(lambda p: p.eth.dst == ROUTER_1_ETH).\
      filter(lambda p: p.udp.dstport == ROUTER_1_TREL_PORT).\
      filter(lambda p: p.trel.source_addr == LEADER and p.trel.destination_addr == ROUTER_1).\
      filter(lambda p: p.trel.wpan.src16 == LEADER_RLOC16 and p.trel.wpan.dst16 == ROUTER_1_RLOC16).\
      must_next()

    # 6. Forwarded reply on 15.4 (ROUTER_1 to ROUTER_3)
    pkts.range(pkts.index).\
      filter_ping_reply(identifier=_identifier).\
      filter_ipv6_src_dst(ROUTER_2_MLEID, ROUTER_3_MLEID).\
      filter_wpan_src16_dst16(ROUTER_1_RLOC16, ROUTER_3_RLOC16).\
      filter(lambda p: p.wpan).\
      must_next()

    # Step 7
    # - Device: Router_2
    # - Description (TREL-8.2): Harness instructs device to send a ping of payload size of 500 bytes or more from
    #   Router_2 to Router_3 using the ML-EID address of Router_3 as the destination address.
    # - Pass Criteria:
    #   - Router_2 MUST successfully receive a ping reply from Router_3, routed via the DUT.
    #   - TREL UDP frames MUST be used for ping/ping-reply transport between the DUT and Leader, and MUST be correct
    #     TREL frames.
    print("Step 7: Router_2 pings Router_3 ML-EID")
    # Path: ROUTER_2 -(15.4)-> LEADER -(TREL)-> ROUTER_1 -(15.4)-> ROUTER_3
    # 1. Request from ROUTER_2 to LEADER over 15.4
    _pkt_154_req = pkts.copy().\
      filter_ping_request().\
      filter_ipv6_src_dst(ROUTER_2_MLEID, ROUTER_3_MLEID).\
      filter_wpan_src16_dst16(ROUTER_2_RLOC16, LEADER_RLOC16).\
      filter(lambda p: p.wpan).\
      must_next()

    _identifier = _pkt_154_req.icmpv6.echo.identifier

    # 2. Check forwarded request on TREL from LEADER to ROUTER_1
    pkts.range(pkts.index).\
      filter_eth_src(LEADER_ETH).\
      filter(lambda p: p.eth.dst == ROUTER_1_ETH).\
      filter(lambda p: p.udp.dstport == ROUTER_1_TREL_PORT).\
      filter(lambda p: p.trel.source_addr == LEADER and p.trel.destination_addr == ROUTER_1).\
      filter(lambda p: p.trel.wpan.src16 == LEADER_RLOC16 and p.trel.wpan.dst16 == ROUTER_1_RLOC16).\
      must_next()

    # 3. Forwarded request on 15.4 (ROUTER_1 to ROUTER_3)
    pkts.range(pkts.index).\
      filter_ping_request(identifier=_identifier).\
      filter_ipv6_src_dst(ROUTER_2_MLEID, ROUTER_3_MLEID).\
      filter_wpan_src16_dst16(ROUTER_1_RLOC16, ROUTER_3_RLOC16).\
      filter(lambda p: p.wpan).\
      must_next()

    # 4. Reply from Router_3 to Router_1 over 15.4
    pkts.copy().\
      filter_ping_reply(identifier=_identifier).\
      filter_ipv6_src_dst(ROUTER_3_MLEID, ROUTER_2_MLEID).\
      filter_wpan_src16_dst16(ROUTER_3_RLOC16, ROUTER_1_RLOC16).\
      filter(lambda p: p.wpan).\
      must_next()

    # 5. Check forwarded reply on TREL from ROUTER_1 to LEADER
    pkts.range(pkts.index).\
      filter_eth_src(ROUTER_1_ETH).\
      filter(lambda p: p.eth.dst == LEADER_ETH).\
      filter(lambda p: p.udp.dstport == LEADER_TREL_PORT).\
      filter(lambda p: p.trel.source_addr == ROUTER_1 and p.trel.destination_addr == LEADER).\
      filter(lambda p: p.trel.wpan.src16 == ROUTER_1_RLOC16 and p.trel.wpan.dst16 == LEADER_RLOC16).\
      must_next()

    # 6. Forwarded reply on 15.4 (LEADER to ROUTER_2)
    pkts.range(pkts.index).\
      filter_ping_reply(identifier=_identifier).\
      filter_ipv6_src_dst(ROUTER_3_MLEID, ROUTER_2_MLEID).\
      filter_wpan_src16_dst16(LEADER_RLOC16, ROUTER_2_RLOC16).\
      filter(lambda p: p.wpan).\
      must_next()

    print("All verification steps PASSED")


if __name__ == '__main__':
    verify_utils.run_main(verify)
