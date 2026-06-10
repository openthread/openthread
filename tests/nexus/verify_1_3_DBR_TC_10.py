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

# Protocol Constants
BR_PREFERENCE_LOW = 3
BR_PREFERENCE_MEDIUM = 0
BR_FLAG_R_TRUE = 1
BR_FLAG_R_FALSE = 0
BR_FLAG_P_TRUE = 1
BR_FLAG_S_TRUE = 1
BR_FLAG_O_TRUE = 1
BR_FLAG_D_FALSE = 0
BR_FLAG_DP_FALSE = 0


def verify(pv):
    # 1.10. [1.3] [CERT] Reachability - Single BR - Configure OMR address - OMR Routing - Default routes
    #
    # 1.10.1. Purpose
    # To test the following:
    # - Thread End Device DUT accepts OMR prefix and configures OMR address
    # - Thread Router DUT accepts route to off-mesh prefix and routes packets to it
    # - Reachability of Thread Device DUT from infrastructure devices
    # - Thread Router DUT can process default route published by a BR and route packets based on it.
    # - Note: current test is not so elaborate yet for route determination/default-route. TBD: In the future,
    #   another BR (BR_2) may be added and it can be checked the DUT Router picks the appropriate/best route to
    #   either BR_1 or BR_2. For this, a new topology with two different AILs for BR_1 and BR_2 would be most useful.
    #
    # 1.10.2. Topology
    # - BR_1-Test bed border router device operating as a Thread Border Router and the Leader
    # - ED_1 - Device operating as a Thread End Device DUT
    # - Eth_1-Test bed border router device on an Adjacent Infrastructure Link
    # - Router 1-Device operating as a Thread Router DUT
    #
    # Spec Reference | V1.3.0 Section
    # ---------------|---------------
    # Reachability   | 1.3

    pkts = pv.pkts
    pv.summary.show()

    BR_1 = pv.vars['BR1']
    ROUTER_1 = pv.vars['ROUTER1']
    ED_1 = pv.vars['ED1']
    ETH_1_GUA = pv.vars['ETH1_GUA']
    ED_1_OMR = pv.vars['ED1_OMR']
    ROUTER_1_OMR = pv.vars['ROUTER1_OMR']
    OMR_PREFIX = pv.vars['OMR_PREFIX'].split('/')[0]

    # Step 0
    # Device: Eth 1
    # Description (DBR-1.10): Harness enables device and configures Ethernet link with an on-link IPv6 GUA prefix
    #   GUA_1. Eth_1 is configured to multicast ND RA. The ND RA contains a PIO option as follows: Prefix: GUA 1,
    #   L=1 (on-link prefix), A=1 (SLAAC is enabled). Device has a global address "Eth_1 GUA" automatically
    #   configured from prefix GUA_1.
    # Pass Criteria:
    # - N/A
    print("Step 0: Eth_1 configured with GUA_1.")

    # Step 1
    # Device: BR_1
    # Description (DBR-1.10): Enable. Automatically, it configures an OMR prefix for the Thread Network and also
    #   automatically advertises the external (default) route to GUA 1 using the ::/0 prefix in the Thread Network
    #   and using a Has Route TLV.
    # Pass Criteria:
    # - N/A
    print("Step 1: BR_1 enables and configures OMR prefix and external route ::/0.")
    pkts.filter_wpan_src64(BR_1).\
        filter(lambda p: hasattr(p, 'mle')).\
        filter(lambda p: p.mle.cmd == consts.MLE_DATA_RESPONSE).\
        filter(lambda p: verify_utils.check_nwd_prefix_flags(p,
                                                            OMR_PREFIX,
                                                            stable=1,
                                                            r=BR_FLAG_R_TRUE,
                                                            o=BR_FLAG_O_TRUE,
                                                            p=BR_FLAG_P_TRUE,
                                                            s=BR_FLAG_S_TRUE,
                                                            d=BR_FLAG_D_FALSE,
                                                            dp=BR_FLAG_DP_FALSE)).\
        filter(lambda p: verify_utils.check_nwd_has_route(p, "::")).\
        must_next()

    # Step 2
    # Device: Router 1, ED 1 (DUT)
    # Description (DBR-1.10): Enable. Automatically, topology is formed.
    # Pass Criteria:
    # - N/A
    print("Step 2: Router_1 and ED_1 enable and form topology.")

    # Step 3
    # Device: ED 1 (DUT)
    # Description (DBR-1.10): Automatically configures an OMR address based on OMR prefix. Note: the verification
    #   that the new address works is done in step 4.
    # Pass Criteria:
    # - N/A
    print("Step 3: ED_1 automatically configures OMR address.")

    # Step 4
    # Device: Eth 1
    # Description (DBR-1.10): Harness instructs device to send ICMPv6 Echo Request to ED 1. IPv6 Source: Eth 1
    #   GUA, IPv6 Destination: ED 1 OMR
    # Pass Criteria:
    # - Eth 1 receives an ICMPv6 Echo Reply from ED_1.
    # - IPv6 Source: ED_1 OMR
    # - IPv6 Destination: Eth_1 GUA
    print("Step 4: Eth_1 sends ICMPv6 Echo Request to ED_1 OMR.")
    _pkt = pkts.filter_ping_request().\
        filter_ipv6_src(ETH_1_GUA).\
        filter_ipv6_dst(ED_1_OMR).\
        must_next()
    pkts.filter_ping_reply(identifier=_pkt.icmpv6.echo.identifier).\
        filter_ipv6_src(ED_1_OMR).\
        filter_ipv6_dst(ETH_1_GUA).\
        must_next()

    # Step 5
    # Device: Router 1 (DUT)
    # Description (DBR-1.10): Automatically routes the packets of step 4.
    # Pass Criteria:
    # - N/A (Already verified in step 4.)
    print("Step 5: Router_1 automatically routes the packets of step 4.")

    # Step 6
    # Device: ED_1
    # Description (DBR-1.10): Only in the topology where Router_1 is DUT: Harness instructs device to send ICMPv6
    #   Echo Request to Eth_1. IPv6 Source: ED 1 OMR, IPv6 Destination: Eth 1 GUA
    # Pass Criteria:
    # - Only in the topology where Router_1 is DUT: ED_1 receives an ICMPv6 Echo Reply from Eth_1.
    # - IPv6 Source: Eth 1 GUA
    # - IPv6 Destination: ED_1 OMR
    print("Step 6: ED_1 sends ICMPv6 Echo Request to Eth_1 GUA.")
    _pkt = pkts.filter_ping_request().\
        filter_ipv6_src(ED_1_OMR).\
        filter_ipv6_dst(ETH_1_GUA).\
        must_next()
    pkts.filter_ping_reply(identifier=_pkt.icmpv6.echo.identifier).\
        filter_ipv6_src(ETH_1_GUA).\
        filter_ipv6_dst(ED_1_OMR).\
        must_next()

    # Step 7
    # Device: Router_1 (DUT)
    # Description (DBR-1.10): Automatically routes the packets of step 6.
    # Pass Criteria:
    # - N/A (Already verified in step 6.)
    print("Step 7: Router_1 automatically routes the packets of step 6.")

    # Step 8
    # Device: Eth 1
    # Description (DBR-1.10): Only in the topology where Router _1 is DUT: Harness instructs device to send ICMPv6
    #   Echo Request to Router 1. IPv6 Source: Eth_1 GUA, IPv6 Destination: Router_1 OMR
    # Pass Criteria:
    # - Only in the topology where Router_1 is DUT: Eth_1 receives an ICMPv6 Echo Reply from Router_1.
    # - IPv6 Source: Router 1 OMR
    # - IPv6 Destination: Eth 1 GUA
    print("Step 8: Eth_1 sends ICMPv6 Echo Request to Router_1 OMR.")
    _pkt = pkts.filter_ping_request().\
        filter_ipv6_src(ETH_1_GUA).\
        filter_ipv6_dst(ROUTER_1_OMR).\
        must_next()
    pkts.filter_ping_reply(identifier=_pkt.icmpv6.echo.identifier).\
        filter_ipv6_src(ROUTER_1_OMR).\
        filter_ipv6_dst(ETH_1_GUA).\
        must_next()

    # Step 9
    # Device: BR_1
    # Description (DBR-1.10): Harness configures device to: Stop advertising the external route to prefix GUA_1
    #   by removing the Prefix TLV with ::/0 completely. Keep publishing the flag P_default = true in the Border
    #   Router sub-TLV. This should normally happen automatically if the device is 1.4-compliant. Wait some time for
    #   the Network Data change to propagate throughout the network. Note: this emulates a legacy BR that doesn't
    #   advertise the default route using an external ::/0 route, but only uses the P_default flag (R flag) in the
    #   Border Router TLV for this.
    # Pass Criteria:
    # - BR 1 reference device MUST publish its OMR prefix with Border Router TLV with P_default = true (R flag =
    #   true).
    # - Note: although this is not a validation on the DUT, it could be useful as a sanity check. Pre-1.4 BRs may
    #   behave differently; and it's important to use the correct value here.
    print("Step 9: BR_1 stops advertising ::/0 external route but keeps P_default=true in OMR prefix.")
    pkts.filter_wpan_src64(BR_1).\
        filter(lambda p: hasattr(p, 'mle')).\
        filter(lambda p: p.mle.cmd == consts.MLE_DATA_RESPONSE).\
        filter(lambda p: verify_utils.check_nwd_prefix_flags(p,
                                                            OMR_PREFIX,
                                                            stable=1,
                                                            pref=BR_PREFERENCE_MEDIUM,
                                                            r=BR_FLAG_R_TRUE,
                                                            o=BR_FLAG_O_TRUE,
                                                            p=BR_FLAG_P_TRUE,
                                                            s=BR_FLAG_S_TRUE,
                                                            d=BR_FLAG_D_FALSE,
                                                            dp=BR_FLAG_DP_FALSE)).\
        filter(lambda p: hasattr(p, 'thread_nwd') and not verify_utils.check_nwd_has_route(p, "::")).\
        must_next()

    # Step 10
    # Device: Eth 1
    # Description (DBR-1.10): Harness instructs device to send ICMPv6 Echo Request to ED_1. IPv6 Source: Eth 1
    #   GUA, IPv6 Destination: ED 1 OMR
    # Pass Criteria:
    # - Eth_1 receives an ICMPv6 Echo Reply from ED_1.
    # - IPv6 Source: ED_1 OMR
    # - IPv6 Destination: Eth 1 GUA
    print("Step 10: Eth_1 sends ICMPv6 Echo Request to ED_1 OMR.")
    _pkt = pkts.filter_ping_request().\
        filter_ipv6_src(ETH_1_GUA).\
        filter_ipv6_dst(ED_1_OMR).\
        must_next()
    pkts.filter_ping_reply(identifier=_pkt.icmpv6.echo.identifier).\
        filter_ipv6_src(ED_1_OMR).\
        filter_ipv6_dst(ETH_1_GUA).\
        must_next()

    # Step 11
    # Device: ED_1
    # Description (DBR-1.10): Only in the topology where Router_1 is DUT: Harness instructs device to send ICMPv6
    #   Echo Request to Eth 1. IPv6 Source: ED_1 OMR, IPv6 Destination: Eth 1 GUA
    # Pass Criteria:
    # - Only in the topology where Router_1 is DUT: ED_1 receives an ICMPv6 Echo Reply from Eth_1.
    # - IPv6 Source: Eth_1 GUA
    # - IPv6 Destination: ED 1 OMR
    print("Step 11: ED_1 sends ICMPv6 Echo Request to Eth_1 GUA.")
    _pkt = pkts.filter_ping_request().\
        filter_ipv6_src(ED_1_OMR).\
        filter_ipv6_dst(ETH_1_GUA).\
        must_next()
    pkts.filter_ping_reply(identifier=_pkt.icmpv6.echo.identifier).\
        filter_ipv6_src(ETH_1_GUA).\
        filter_ipv6_dst(ED_1_OMR).\
        must_next()

    # Step 12
    # Device: BR 1
    # Description (DBR-1.10): Harness configures device to publish for its OMR prefix: flag P_default = false in
    #   the Border Router sub-TLV; and By setting a Prefix TLV with the zero-length prefix ::/0, which includes Has
    #   Route TLV preference value (Prf) 11 (low). Wait some time for the Network Data change to propagate
    #   throughout the network.
    # Pass Criteria:
    # - BR_1 reference device MUST publish its OMR prefix with Border Router TLV with P_default = false (R flag =
    #   false).
    # - Note: although this is not a validation on the DUT, it could be useful as a sanity check. Pre-1.4 BRs may
    #   behave differently; and it's important to use the correct value here.
    print("Step 12: BR_1 publishes OMR prefix with P_default=false and ::/0 external route with low preference.")
    pkts.filter_wpan_src64(BR_1).\
        filter(lambda p: hasattr(p, 'mle')).\
        filter(lambda p: p.mle.cmd == consts.MLE_DATA_RESPONSE).\
        filter(lambda p: verify_utils.check_nwd_prefix_flags(p,
                                                            OMR_PREFIX,
                                                            stable=1,
                                                            pref=BR_PREFERENCE_MEDIUM,
                                                            r=BR_FLAG_R_FALSE,
                                                            o=BR_FLAG_O_TRUE,
                                                            p=BR_FLAG_P_TRUE,
                                                            s=BR_FLAG_S_TRUE,
                                                            d=BR_FLAG_D_FALSE,
                                                            dp=BR_FLAG_DP_FALSE)).\
        filter(lambda p: verify_utils.check_nwd_has_route(p, "::", pref=BR_PREFERENCE_LOW)).\
        must_next()

    # Step 13
    # Device: Eth 1
    # Description (DBR-1.10): Repeat Step 10.
    # Pass Criteria:
    # - Repeat Step 10.
    print("Step 13: Eth_1 sends ICMPv6 Echo Request to ED_1 OMR.")
    _pkt = pkts.filter_ping_request().\
        filter_ipv6_src(ETH_1_GUA).\
        filter_ipv6_dst(ED_1_OMR).\
        must_next()
    pkts.filter_ping_reply(identifier=_pkt.icmpv6.echo.identifier).\
        filter_ipv6_src(ED_1_OMR).\
        filter_ipv6_dst(ETH_1_GUA).\
        must_next()

    # Step 14
    # Device: ED 1
    # Description (DBR-1.10): Repeat Step 11.
    # Pass Criteria:
    # - Repeat Step 11.
    print("Step 14: ED_1 sends ICMPv6 Echo Request to Eth_1 GUA.")
    _pkt = pkts.filter_ping_request().\
        filter_ipv6_src(ED_1_OMR).\
        filter_ipv6_dst(ETH_1_GUA).\
        must_next()
    pkts.filter_ping_reply(identifier=_pkt.icmpv6.echo.identifier).\
        filter_ipv6_src(ETH_1_GUA).\
        filter_ipv6_dst(ED_1_OMR).\
        must_next()


if __name__ == '__main__':
    verify_utils.run_main(verify)
