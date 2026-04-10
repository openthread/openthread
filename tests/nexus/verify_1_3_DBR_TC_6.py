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
    # 1.6. [1.3] [CERT] Reachability - Multiple BRs - Single Thread - Single IPv6 Infrastructure
    #
    # 1.6.1. Purpose
    #   To test the following:
    #   - 1. Bi-directional reachability between single Thread Network and infrastructure devices
    #   - 2. Multiple BRs
    #   - 3. IPv6 infrastructure is already existing
    #   - 4. DUT BR adopts existing OMR prefixes and doesn't advertise PIO
    #
    # 1.6.2. Topology
    #   - 1. BR 1 (DUT) - Thread Border Router
    #   - 2. BR 2-Test Bed border router device operating as a Thread Border Router and the Leader
    #   - 3. ED 1-Test Bed device operating as a Thread End Device, attached to BR_1
    #   - 4. Eth 1-Test Bed border router device on an Adjacent Infrastructure Link
    #
    # Spec Reference | V1.3.0 Section
    # ---------------|---------------
    # Reachability   | 1.3

    pkts = pv.pkts
    pv.summary.show()

    BR1 = pv.vars['BR1']
    ED1 = pv.vars['ED1']
    OMR_PREFIX = Ipv6Addr(pv.vars['OMR_PREFIX'].split('/')[0])
    ETH1_GUA = Ipv6Addr(pv.vars['ETH1_GUA'])
    ED1_OMR = Ipv6Addr(pv.vars['ED1_OMR'])

    # Step 0
    #   Device: Eth 1
    #   Description (DBR-1.6): Harness configures Ethernet link with an on-link IPv6 GUA prefix GUA 1.
    #     Eth 1 is configured to multicast ND RAS.
    #   Pass Criteria:
    #     N/A
    print("Step 0: Harness configures Ethernet link with an on-link IPv6 GUA prefix GUA 1.")

    # Step 1
    #   Device: Eth 1, BR 2
    #   Description (DBR-1.6): Form topology. Wait for BR_2 to: 1. Register as border router in Thread
    #     Network Data with an OMR prefix OMR_1 2. Send multicast ND RAS
    #   Pass Criteria:
    #     N/A
    print("Step 1: Form topology. Wait for BR_2 to register as border router and send ND RAs.")

    # Step 2
    #   Device: BR 1 (DUT)
    #   Description (DBR-1.6): Enable: switch on.
    #   Pass Criteria:
    #     N/A
    print("Step 2: Enable: switch on.")

    # Step 3
    #   Device: BR 1 (DUT)
    #   Description (DBR-1.6): Automatically registers itself as a border router in the Thread Network Data.
    #   Pass Criteria:
    #     - The DUT MUST NOT register a new OMR Prefix in the Thread Network Data.
    #     - The DUT MUST advertise an external route in the Thread Network Data as follows:
    #     - Prefix: ::/0 (zero-length prefix)
    #     - Has Route TLV
    #     - Prf 'Medium' (00) or 'Low' (11)
    print("Step 3: BR 1 (DUT) MUST NOT register a new OMR Prefix. MUST advertise an external route ::/0.")

    def check_step3_nwd(p):
        if not (hasattr(p, 'mle') and p.mle.cmd == consts.MLE_DATA_RESPONSE):
            return False

        prefixes = verify_utils.as_list(p.thread_nwd.tlv.prefix)
        # Check for ::/0
        if Ipv6Addr('::') not in prefixes:
            return False

        # Check that ONLY ONE OMR prefix exists (the one from BR2)
        omr_prefixes = [pref for pref in prefixes if pref != Ipv6Addr('::')]
        if len(omr_prefixes) != 1:
            return False

        # Check Preference of ::/0 (Has Route TLV)
        try:
            # Find the index of :: prefix
            idx = prefixes.index(Ipv6Addr('::'))
            # Preference should be Medium (0) or Low (3 in some dissectors, or 11 binary)
            pref = verify_utils.as_list(p.thread_nwd.tlv.has_route.pref)[idx]
            if pref not in (0, 3):
                return False
        except (AttributeError, IndexError):
            pass

        return True

    pkts.filter_wpan_src64(BR1).\
        filter(check_step3_nwd).\
        must_next()

    # Step 2b
    #   Device: ED 1
    #   Description (DBR-1.6): Harness enables device.
    #   Pass Criteria:
    #     - ED_1 successfully attaches to the DUT as its Parent.
    print("Step 2b: ED_1 successfully attaches to the DUT as its Parent.")
    pkts.filter_wpan_src64(ED1).\
        filter(lambda p: hasattr(p, 'mle') and p.mle.cmd == consts.MLE_PARENT_REQUEST).\
        must_next()
    pkts.filter_wpan_src64(BR1).\
        filter(lambda p: hasattr(p, 'mle') and p.mle.cmd == consts.MLE_PARENT_RESPONSE).\
        must_next()
    pkts.filter_wpan_src64(ED1).\
        filter(lambda p: hasattr(p, 'mle') and p.mle.cmd == consts.MLE_CHILD_ID_REQUEST).\
        must_next()
    pkts.filter_wpan_src64(BR1).\
        filter(lambda p: hasattr(p, 'mle') and p.mle.cmd == consts.MLE_CHILD_ID_RESPONSE).\
        must_next()

    # Step 4
    #   Device: BR 1 (DUT)
    #   Description (DBR-1.6): Automatically multicasts ND RAs on Adjacent Infrastructure Link.
    #   Pass Criteria:
    #     - The DUT MUST multicast ND RAS:
    #     - IPv6 destination MUST be ff02::1
    #     - MUST NOT contain a Prefix Information Option (PIO).
    #     - MUST contain a Route Information Option (RIO) with the OMR prefix OMR 1.
    print("Step 4: BR 1 (DUT) MUST multicast ND RAs: ff02::1, NO PIO, RIO with OMR_1.")
    pkts.filter_eth_src(pv.vars['BR1_ETH']).\
        filter_ipv6_dst("ff02::1").\
        filter(lambda p: p.icmpv6.type == verify_utils.ICMPV6_TYPE_ROUTER_ADVERTISEMENT).\
        filter(lambda p: OMR_PREFIX in verify_utils.get_ra_prefixes(p)[0]).\
        filter(lambda p: len(verify_utils.get_ra_prefixes(p)[1]) == 0).\
        must_next()

    # Step 5
    #   Device: Eth_1
    #   Description (DBR-1.6): Harness instructs the device to send an ICMPv6 Echo Request to ED 1 via BR 1 or
    #     BR 2. 1. IPv6 Source: Eth 1 GUA 2. IPv6 Destination: ED_1 OMR
    #   Pass Criteria:
    #     - Eth_1 receives an ICMPv6 Echo Reply from ED_1.
    #     - IPv6 Source: ED_1 OMR
    #     - IPv6 Destination: Eth 1 GUA
    print("Step 5: Eth_1 pings ED_1 OMR.")
    _pkt = pkts.filter_ipv6_src(ETH1_GUA).\
        filter_ipv6_dst(ED1_OMR).\
        filter_ping_request().\
        must_next()

    pkts.filter_ipv6_src(ED1_OMR).\
        filter_ipv6_dst(ETH1_GUA).\
        filter_ping_reply(identifier=_pkt.icmpv6.echo.identifier).\
        must_next()

    # Step 6
    #   Device: ED_1
    #   Description (DBR-1.6): Harness instructs the device to send an ICMPv6 Echo Request to Eth_1.
    #     1. IPv6 Source: ED 1 OMR 2. IPv6 Destination: Eth_1 GUA
    #   Pass Criteria:
    #     - ED_1 receives an ICMPv6 Echo Reply from Eth_1.
    #     - IPv6 Source: Eth_1 GUA
    #     - IPv6 Destination: ED 1 OMR
    print("Step 6: ED_1 pings Eth_1 GUA.")
    _pkt = pkts.filter_ipv6_src(ED1_OMR).\
        filter_ipv6_dst(ETH1_GUA).\
        filter_ping_request().\
        must_next()

    pkts.filter_ipv6_src(ETH1_GUA).\
        filter_ipv6_dst(ED1_OMR).\
        filter_ping_reply(identifier=_pkt.icmpv6.echo.identifier).\
        must_next()

    # Step 7
    #   Device: BR 2
    #   Description (DBR-1.6): Harness disables the device. Note: automatically, the network data of BR_2
    #     including the prefix OMR 1 data will remain active for the remainder of this test procedure.
    #   Pass Criteria:
    #     N/A
    print("Step 7: Harness disables BR 2.")

    # Step 8
    #   Device: BR 1 (DUT)
    #   Description (DBR-1.6): Repeat Step 4
    #   Pass Criteria:
    #     - Repeat Step 4
    print("Step 8: Repeat Step 4")
    pkts.filter_eth_src(pv.vars['BR1_ETH']).\
        filter_ipv6_dst("ff02::1").\
        filter(lambda p: p.icmpv6.type == verify_utils.ICMPV6_TYPE_ROUTER_ADVERTISEMENT).\
        filter(lambda p: OMR_PREFIX in verify_utils.get_ra_prefixes(p)[0]).\
        filter(lambda p: len(verify_utils.get_ra_prefixes(p)[1]) == 0).\
        must_next()

    # Step 9
    #   Device: Eth 1
    #   Description (DBR-1.6): Repeat Step 5
    #   Pass Criteria:
    #     - Repeat Step 5
    print("Step 9: Repeat Step 5")
    _pkt = pkts.filter_ipv6_src(ETH1_GUA).\
        filter_ipv6_dst(ED1_OMR).\
        filter_ping_request().\
        must_next()

    pkts.filter_ipv6_src(ED1_OMR).\
        filter_ipv6_dst(ETH1_GUA).\
        filter_ping_reply(identifier=_pkt.icmpv6.echo.identifier).\
        must_next()

    # Step 10
    #   Device: ED 1
    #   Description (DBR-1.6): Repeat Step 6
    #   Pass Criteria:
    #     - Repeat Step 6
    print("Step 10: Repeat Step 6")
    _pkt = pkts.filter_ipv6_src(ED1_OMR).\
        filter_ipv6_dst(ETH1_GUA).\
        filter_ping_request().\
        must_next()

    pkts.filter_ipv6_src(ETH1_GUA).\
        filter_ipv6_dst(ED1_OMR).\
        filter_ping_reply(identifier=_pkt.icmpv6.echo.identifier).\
        must_next()


if __name__ == '__main__':
    verify_utils.run_main(verify)
