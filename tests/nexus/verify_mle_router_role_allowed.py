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


def verify(pv: verify_utils.PacketVerifier):
    # Test differing behaviors based on the Router role allowed
    #
    # Purpose & Description
    # This test validates that correctly-formatted transmissions occur based on whether the Router
    # role is allowed.  This includes Advertisements (only from REEDs and Active Routers),
    # which should not occur for devices with Router role disallowed.
    # This test verifies Router role disallowed on FED and MED devices, with a duplicate
    # pair used to demonstrate that these transmissions begin in the correct format when
    # re-configured so that the Router role becomes allowed.

    pv.summary.show()

    LEADER = pv.vars['Leader']
    ROUTER = pv.vars['Router']
    REED = pv.vars['REED']
    FED_ONLY = pv.vars['FED_ONLY']
    MED_ONLY = pv.vars['MED_ONLY']
    FED_ROUTER = pv.vars['FED_Router']
    MED_ROUTER = pv.vars['MED_Router']

    print("Node: Leader")
    leader_packet_filter = pv.pkts.copy().filter_wpan_src64(LEADER)
    # There was at least one advertisement from the Leader
    leader_mle_advertisement_packet_filter = leader_packet_filter.copy().filter_mle_cmd(consts.MLE_ADVERTISEMENT)
    leader_mle_advertisement_packets = [pkt for pkt in iter(leader_mle_advertisement_packet_filter.next, None)]
    assert len(leader_mle_advertisement_packets) >= 1
    # And all advertisements had a Router source address (with child id of 0)
    assert all(pkt.mle.tlv.source_addr & 0x1FF == 0 for pkt in leader_mle_advertisement_packets)
    # And contain a route64 TLV
    assert all(
        pkt.must_verify(lambda p: (consts.ROUTE64_TLV) in p.mle.tlv.type) for pkt in leader_mle_advertisement_packets)

    print("Node: Router")
    router_packet_filter = pv.pkts.copy().filter_wpan_src64(ROUTER)
    # There was at least one advertisement from the Router
    router_mle_advertisement_packet_filter = router_packet_filter.copy().filter_mle_cmd(consts.MLE_ADVERTISEMENT)
    router_mle_advertisement_packets = [pkt for pkt in iter(router_mle_advertisement_packet_filter.next, None)]
    assert len(router_mle_advertisement_packets) >= 1
    # And all advertisements had a Router source address (with child id of 0)
    assert all(pkt.mle.tlv.source_addr & 0x1FF == 0 for pkt in router_mle_advertisement_packets)
    # And contain a route64 TLV
    assert all(
        pkt.must_verify(lambda p: (consts.ROUTE64_TLV) in p.mle.tlv.type) for pkt in router_mle_advertisement_packets)

    print("Node: REED")
    reed_packet_filter = pv.pkts.copy().filter_wpan_src64(REED)
    reed_mle_advertisement_packet_filter = reed_packet_filter.copy().filter_mle_cmd(consts.MLE_ADVERTISEMENT)
    reed_mle_advertisement_packets = [pkt for pkt in iter(reed_mle_advertisement_packet_filter.next, None)]
    # There was at least one advertisement from the REED
    assert len(reed_mle_advertisement_packets) >= 1
    # And all advertisements had a Child source address (with child id > 0)
    assert all(pkt.mle.tlv.source_addr & 0x1FF > 0 for pkt in reed_mle_advertisement_packets)
    # And not contain a route64 TLV
    assert all(
        pkt.must_not_verify(lambda p: (consts.ROUTE64_TLV) in p.mle.tlv.type)
        for pkt in reed_mle_advertisement_packets)

    print("Node: FED only")
    fed_only_packet_filter = pv.pkts.copy().filter_wpan_src64(FED_ONLY)
    # When only a FED, no advertisements should be sent
    fed_only_packet_filter.copy().filter_mle_cmd(consts.MLE_ADVERTISEMENT).must_not_next()

    print("Node: MED only")
    med_only_packet_filter = pv.pkts.copy().filter_wpan_src64(MED_ONLY)
    # When only a FED, no advertisements should be sent
    med_only_packet_filter.copy().filter_mle_cmd(consts.MLE_ADVERTISEMENT).must_not_next()

    print("Node: FED and Router")
    fed_and_router_packet_filter = pv.pkts.copy()
    # Exactly one advertisement should have been sent when configured as a router
    fed_and_router_mle_advertisement_packet_filter = fed_and_router_packet_filter.filter_mle_cmd(
        consts.MLE_ADVERTISEMENT).filter_wpan_src64(FED_ROUTER)
    fed_and_router_mle_advertisement_packets = [
        pkt for pkt in iter(fed_and_router_mle_advertisement_packet_filter.next, None)
    ]
    # There was at least one advertisement as a Router
    assert len(fed_and_router_mle_advertisement_packets) >= 1, f"{len(fed_and_router_mle_advertisement_packets)}"
    # And all advertisements had a Router source address (with child id of 0)
    assert all(pkt.mle.tlv.source_addr & 0x1FF == 0 for pkt in fed_and_router_mle_advertisement_packets)
    # And contain a route64 TLV
    assert all(
        pkt.must_verify(lambda p: (consts.ROUTE64_TLV) in p.mle.tlv.type)
        for pkt in fed_and_router_mle_advertisement_packets)

    print("Node: MED and Router")
    med_and_router_packet_filter = pv.pkts.copy()
    # Exactly one advertisement should have been sent when configured as a router
    med_and_router_mle_advertisement_packet_filter = med_and_router_packet_filter.filter_mle_cmd(
        consts.MLE_ADVERTISEMENT).filter_wpan_src64(MED_ROUTER)
    med_and_router_mle_advertisement_packets = [
        pkt for pkt in iter(med_and_router_mle_advertisement_packet_filter.next, None)
    ]
    # There was at least one advertisement as a Router
    assert len(med_and_router_mle_advertisement_packets) >= 1, f"{len(med_and_router_mle_advertisement_packets)}"
    # And all advertisements had a Router source address (with child id of 0)
    assert all(pkt.mle.tlv.source_addr & 0x1FF == 0 for pkt in med_and_router_mle_advertisement_packets)
    # And contain a route64 TLV
    assert all(
        pkt.must_verify(lambda p: (consts.ROUTE64_TLV) in p.mle.tlv.type)
        for pkt in med_and_router_mle_advertisement_packets)


if __name__ == '__main__':
    verify_utils.run_main(verify)
