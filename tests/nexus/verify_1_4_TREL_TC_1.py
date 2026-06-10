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
    # 8.1. [1.4] [CERT] Attach and connectivity (unicast/broadcast) between multi-radio and single-radio devices
    #
    # 8.1.1. Purpose
    # To test the following:
    #   1. Attaching Thread Devices with different radio links.
    #   2. Verifies the detection of radio link types supported by neighbors/children.
    #   3. Verifies connectivity between nodes (unicast and broadcast frame exchange with multi-radio support).
    #
    # Spec Reference           | V1.1 Section | V1.3.0 Section
    # -------------------------|--------------|---------------
    # TREL / Multi-radio Links | N/A          | 8.1

    pkts = pv.pkts
    pv.summary.show()

    BR_DUT = pv.vars['BR_DUT']
    ROUTER_1 = pv.vars['ROUTER_1']
    ROUTER_2 = pv.vars['ROUTER_2']

    BR_DUT_RLOC16 = pv.vars['BR_DUT_RLOC16']
    ROUTER_1_RLOC16 = pv.vars['ROUTER_1_RLOC16']
    ROUTER_2_RLOC16 = pv.vars['ROUTER_2_RLOC16']

    # Step 1
    # - Device: BR (DUT), Router_1, Router_2
    # - Description (TREL-8.1): Form the topology. Wait for Router_1 and Router_2 to become routers. Note: Router_1
    #   and Router_2 MUST NOT be able to communicate directly and not be in each other’s neighbor table (e.g. this
    #   may be realized by using the allow/deny list mechanism).
    # - Pass Criteria:
    #   - Verify that topology is formed.
    #   - The DUT MUST be the network Leader.
    #   - Router_1 MUST only see Leader as its immediate neighbor (Router_2 MUST not be present in Router_1’s
    #     neighbor table).
    #   - Router_2 MUST only see Leader as its immediate neighbor and not Router_1.
    print("Step 1: Form the topology")
    # Verify DUT is leader
    pkts.filter_wpan_src64(BR_DUT).\
        filter_mle_cmd(consts.MLE_ADVERTISEMENT).\
        filter(lambda p: p.mle.tlv.leader_data.router_id == p.mle.tlv.source_addr >> 10).\
        must_next()

    # Verify Router_1 and Router_2 do not communicate directly on 15.4
    pkts.filter_wpan_src64(ROUTER_1).filter_wpan_dst64(ROUTER_2).must_not_next()
    pkts.filter_wpan_src64(ROUTER_2).filter_wpan_dst64(ROUTER_1).must_not_next()

    # Step 2
    # - Device: Router_1
    # - Description (TREL-8.1): Harness instructs device to report its neighbor multi-radio info (which indicates
    #   which radio links are detected per neighbor/parent)
    # - Pass Criteria:
    #   - Router_1 MUST have correctly detected that the DUT supports both TREL and 15.4 radios (multi-radio neighbor
    #     info).
    #   - The DUT MUST send TREL frames using the correct format
    print("Step 2: Router_1 reports its neighbor multi-radio info")
    # Verified by C++ code internally

    # Step 3: Router_1 pings the DUT using link-local address
    print("Step 3: Router_1 pings the DUT using link-local address")
    # Verify ping request from ROUTER_1 to BR_DUT over TREL
    # We can't easily decode ICMPv6 inside TREL if tshark fails, so we filter by UDP port and size.
    pkts.filter_eth_src(pv.vars['ROUTER_1_ETH']).\
        filter(lambda p: p.eth.dst == pv.vars['BR_DUT_ETH']).\
        filter(lambda p: p.udp).\
        filter(lambda p: p.udp.dstport == pv.vars['BR_DUT_TREL_PORT']).\
        filter(lambda p: int(p.udp.length) > 500).\
        must_next()

    _idx_req = pkts.index

    # Verify ping reply from BR_DUT to ROUTER_1 over TREL
    pkts.filter_eth_src(pv.vars['BR_DUT_ETH']).\
        filter(lambda p: p.eth.dst == pv.vars['ROUTER_1_ETH']).\
        filter(lambda p: p.udp).\
        filter(lambda p: p.udp.dstport == pv.vars['ROUTER_1_TREL_PORT']).\
        filter(lambda p: int(p.udp.length) > 500).\
        must_next()

    _idx_reply = pkts.index

    # Verify that there are no ping replies for this exchange on 15.4
    pkts.range(_idx_req, _idx_reply).\
        filter_ping_reply().\
        filter(lambda p: p.wpan).\
        must_not_next()

    # Step 4: Router_2 pings the DUT using link-local address
    print("Step 4: Router_2 pings the DUT using link-local address")
    # Router_2 is 15.4 only, so ping should be over 15.4
    _pkt = pkts.filter_ping_request().\
        filter_ipv6_src(pv.vars['ROUTER_2_LLA']).\
        filter_ipv6_dst(pv.vars['BR_DUT_LLA']).\
        filter(lambda p: p.wpan).\
        must_next()

    pkts.filter_ping_reply(identifier=_pkt.icmpv6.echo.identifier).\
        filter_ipv6_src(pv.vars['BR_DUT_LLA']).\
        filter_ipv6_dst(pv.vars['ROUTER_2_LLA']).\
        filter(lambda p: p.wpan).\
        must_next()

    # Step 5: Router_1 pings Router_2 using mesh-local EID
    print("Step 5: Router_1 pings Router_2 using mesh-local EID")
    # Path: Router_1 -> BR_DUT (TREL) -> Router_2 (15.4)

    # 1. Check forwarded request on 15.4 to get identifier
    _pkt_154_req = pkts.filter_ping_request().\
        filter_ipv6_src(pv.vars['ROUTER_1_MLEID']).\
        filter_ipv6_dst(pv.vars['ROUTER_2_MLEID']).\
        filter_wpan_src16_dst16(BR_DUT_RLOC16, ROUTER_2_RLOC16).\
        filter(lambda p: p.wpan).\
        must_next()

    _identifier = _pkt_154_req.icmpv6.echo.identifier

    # 2. Check original request on TREL (before the 15.4 one)
    pkts.range((0, 0), pkts.index).\
        filter_eth_src(pv.vars['ROUTER_1_ETH']).\
        filter(lambda p: p.eth.dst == pv.vars['BR_DUT_ETH']).\
        filter(lambda p: p.udp).\
        filter(lambda p: p.udp.dstport == pv.vars['BR_DUT_TREL_PORT']).\
        must_next()

    # 3. Check reply from Router_2 back to BR_DUT over 15.4
    _pkt_154_reply = pkts.filter_ping_reply(identifier=_identifier).\
        filter_ipv6_src(pv.vars['ROUTER_2_MLEID']).\
        filter_ipv6_dst(pv.vars['ROUTER_1_MLEID']).\
        filter_wpan_src16_dst16(ROUTER_2_RLOC16, BR_DUT_RLOC16).\
        filter(lambda p: p.wpan).\
        must_next()

    # 4. Check forwarded reply on TREL (after the 15.4 one)
    pkts.range(pkts.index).\
        filter_eth_src(pv.vars['BR_DUT_ETH']).\
        filter(lambda p: p.eth.dst == pv.vars['ROUTER_1_ETH']).\
        filter(lambda p: p.udp).\
        filter(lambda p: p.udp.dstport == pv.vars['ROUTER_1_TREL_PORT']).\
        must_next()

    # Step 6: Router_2 pings Router_1 using mesh-local EID
    print("Step 6: Router_2 pings Router_1 using mesh-local EID")
    # Path: Router_2 -> BR_DUT (15.4) -> Router_1 (TREL)

    # 1. Check request from Router_2 to BR_DUT over 15.4
    _pkt_154_req = pkts.filter_ping_request().\
        filter_ipv6_src(pv.vars['ROUTER_2_MLEID']).\
        filter_ipv6_dst(pv.vars['ROUTER_1_MLEID']).\
        filter_wpan_src16_dst16(ROUTER_2_RLOC16, BR_DUT_RLOC16).\
        filter(lambda p: p.wpan).\
        must_next()

    _identifier = _pkt_154_req.icmpv6.echo.identifier

    # 2. Check forwarded request on TREL
    _idx_154_req = pkts.index
    pkts.range(_idx_154_req).\
        filter_eth_src(pv.vars['BR_DUT_ETH']).\
        filter(lambda p: p.eth.dst == pv.vars['ROUTER_1_ETH']).\
        filter(lambda p: p.udp).\
        filter(lambda p: p.udp.dstport == pv.vars['ROUTER_1_TREL_PORT']).\
        must_next()

    # 3. Check reply from Router_1 back to BR_DUT over TREL
    _idx_trel_req = pkts.index
    pkts.range(_idx_trel_req).\
        filter_eth_src(pv.vars['ROUTER_1_ETH']).\
        filter(lambda p: p.eth.dst == pv.vars['BR_DUT_ETH']).\
        filter(lambda p: p.udp).\
        filter(lambda p: p.udp.dstport == pv.vars['BR_DUT_TREL_PORT']).\
        must_next()

    # 4. Check forwarded reply on 15.4
    _idx_trel_reply = pkts.index
    pkts.range(_idx_trel_reply).\
        filter_ping_reply(identifier=_identifier).\
        filter_ipv6_src(pv.vars['ROUTER_1_MLEID']).\
        filter_ipv6_dst(pv.vars['ROUTER_2_MLEID']).\
        filter_wpan_src16_dst16(BR_DUT_RLOC16, ROUTER_2_RLOC16).\
        filter(lambda p: p.wpan).\
        must_next()


if __name__ == '__main__':
    verify_utils.run_main(verify)
