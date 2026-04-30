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
    pkts = pv.pkts
    pv.summary.show()

    LEADER = pv.vars['LEADER']
    ROUTER_1 = pv.vars['ROUTER_1']
    MED_1 = pv.vars['MED_1']
    SED_1 = pv.vars['SED_1']

    LEADER_RLOC16 = pv.vars['LEADER_RLOC16']
    ROUTER_1_RLOC16 = pv.vars['ROUTER_1_RLOC16']

    # Step 1: Leader forms the network
    print("Step 1: Leader forms the network and sends MLE Advertisements.")
    pkts.filter_wpan_src64(LEADER).\
        filter_mle_cmd(consts.MLE_ADVERTISEMENT).\
        must_next()

    # Step 2: Leader configured as BR
    print("Step 2: Leader configures as a Border Router with 4 prefixes.")

    # Step 3: Router_1 (DUT) attaches to Leader
    print("Step 3: Router_1 (DUT) attaches to the Leader and requests Network Data TLV.")
    pkts.filter_wpan_src64(ROUTER_1).\
        filter_mle_cmd(consts.MLE_CHILD_ID_REQUEST).\
        filter(lambda p: {
            consts.TLV_REQUEST_TLV,
            consts.NETWORK_DATA_TLV
        } <= set(p.mle.tlv.type)).\
        must_next()

    # Step 4: Leader responds with 4 prefixes
    print("Step 4: Leader includes the Network Data TLV in the MLE Child ID Response.")
    pkts.filter_wpan_src64(LEADER).\
        filter_wpan_dst64(ROUTER_1).\
        filter_mle_cmd(consts.MLE_CHILD_ID_RESPONSE).\
        filter(lambda p: {
            Ipv6Addr("2001::"),
            Ipv6Addr("2002::"),
            Ipv6Addr("2003::"),
            Ipv6Addr("2004::")
        } <= set(p.thread_nwd.tlv.prefix)).\
        must_next()

    # Step 5: SED_1 attaches to DUT
    print("Step 5: SED_1 attaches to the DUT and requests only stable Network Data.")

    # Step 6: Router_1 responds to SED_1 with stable net data only
    print("Step 6: Router_1 (DUT) sends MLE Child ID Response to SED_1 with stable Network Data.")
    pkts.filter_wpan_src64(ROUTER_1).\
        filter_wpan_dst64(SED_1).\
        filter_mle_cmd(consts.MLE_CHILD_ID_RESPONSE).\
        filter(lambda p: {
            Ipv6Addr("2001::"),
            Ipv6Addr("2003::"),
            Ipv6Addr("2004::")
        } <= set(p.thread_nwd.tlv.prefix)).\
        filter(lambda p: Ipv6Addr("2002::") not in p.thread_nwd.tlv.prefix).\
        filter(lambda p: all(rloc == 0xfffe for rloc in p.thread_nwd.tlv.border_router_16)).\
        filter(lambda p: hasattr(p, 'lowpan') and hasattr(p.lowpan, 'fragment')).\
        filter(lambda p: p.wpan.aux_sec.sec_level == 5).\
        must_next()

    # Step 7: MED_1 attaches to DUT
    print("Step 7: MED_1 attaches to the DUT and requests full Network Data.")

    # Step 8: Router_1 responds to MED_1 with full net data
    print("Step 8: Router_1 (DUT) sends MLE Child ID Response to MED_1 with full Network Data.")
    pkts.filter_wpan_src64(ROUTER_1).\
        filter_wpan_dst64(MED_1).\
        filter_mle_cmd(consts.MLE_CHILD_ID_RESPONSE).\
        filter(lambda p: {
            Ipv6Addr("2001::"),
            Ipv6Addr("2002::"),
            Ipv6Addr("2003::"),
            Ipv6Addr("2004::")
        } <= set(p.thread_nwd.tlv.prefix)).\
        filter(lambda p: hasattr(p, 'lowpan') and hasattr(p.lowpan, 'fragment')).\
        filter(lambda p: p.wpan.aux_sec.sec_level == 5).\
        must_next()

    # Step 9 & 10: Child Update messages
    print("Step 9 & 10: Verifying MLE Child Update messages.")

    def verify_child_update(child_ext64, expected_addr_count):
        # Find Child Update Request from child
        pkts.copy().\
            filter_wpan_src64(child_ext64).\
            filter_wpan_dst64(ROUTER_1).\
            filter_mle_cmd(consts.MLE_CHILD_UPDATE_REQUEST).\
            filter(lambda p: consts.ADDRESS_REGISTRATION_TLV in p.mle.tlv.type).\
            must_next()

        # Find Child Update Response from DUT
        pkts.copy().\
            filter_wpan_src64(ROUTER_1).\
            filter_wpan_dst64(child_ext64).\
            filter_mle_cmd(consts.MLE_CHILD_UPDATE_RESPONSE).\
            filter(lambda p: {
                consts.SOURCE_ADDRESS_TLV,
                consts.LEADER_DATA_TLV,
                consts.ADDRESS_REGISTRATION_TLV,
                consts.MODE_TLV
            } <= set(p.mle.tlv.type)).\
            filter(lambda p: len(p.mle.tlv.addr_reg_iid) >= expected_addr_count).\
            must_next()

    verify_child_update(SED_1, 3)
    verify_child_update(MED_1, 4)

    # Step 11: Leader pings DUT
    print("Step 11: Leader sends ICMPv6 Echo Request to the DUT GUA addresses.")
    for _ in range(4):
        _pkt = pkts.filter_ping_request().\
            filter(lambda p: not str(p.ipv6.dst).startswith("fe80")).\
            filter_wpan_src16(LEADER_RLOC16).\
            filter_wpan_dst16(ROUTER_1_RLOC16).\
            must_next()
        pkts.filter_ping_reply(identifier=_pkt.icmpv6.echo.identifier).\
            filter_wpan_src16(ROUTER_1_RLOC16).\
            filter_wpan_dst16(LEADER_RLOC16).\
            must_next()


if __name__ == '__main__':
    verify_utils.run_main(verify)
