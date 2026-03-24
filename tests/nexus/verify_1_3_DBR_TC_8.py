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
from pktverify import consts
from pktverify.null_field import nullField

# Protocol Constants
ULA_PREFIX_START_BYTE = 0xfd
BR_PREFERENCE_LOW = 3
BR_PREFERENCE_MEDIUM = 0
BR_FLAG_R_TRUE = 1
BR_FLAG_O_TRUE = 1
BR_FLAG_P_TRUE = 1
BR_FLAG_P_FALSE = 0
BR_FLAG_S_TRUE = 1
BR_FLAG_D_FALSE = 0
BR_FLAG_DP_FALSE = 0


def check_nwd_has_route(packet, prefix):
    """Checks if Network Data has an External Route with the given prefix."""
    try:
        if not hasattr(packet, 'thread_nwd'):
            return False
        types = verify_utils.as_list(packet.thread_nwd.tlv.type)
        prefixes = verify_utils.as_list(packet.thread_nwd.tlv.prefix)
    except (AttributeError, IndexError):
        return False

    prefix_idx = 0
    is_target = False

    for t in types:
        if t == consts.NWD_PREFIX_TLV:
            if prefix_idx < len(prefixes):
                current_prefix = prefixes[prefix_idx]
                prefix_idx += 1
                if current_prefix:
                    is_target = (Ipv6Addr(current_prefix) == Ipv6Addr(prefix))
                else:
                    is_target = False
        elif t == consts.NWD_HAS_ROUTER_TLV:
            if is_target:
                return True
        elif t in (consts.NWD_COMMISSIONING_DATA_TLV, consts.NWD_SERVICE_TLV):
            is_target = False

    return False


def check_ra_rio(packet, prefix, not_prefixes=None):
    """Checks if an ICMPv6 RA contains a Route Information Option (RIO) with the given prefix."""
    rio_prefixes, _ = verify_utils.get_ra_prefixes(packet)
    target_addr = Ipv6Addr(prefix)
    found = any(Ipv6Addr(p) == target_addr for p in rio_prefixes)
    if not found:
        return False
    if not_prefixes:
        for np in not_prefixes:
            if any(Ipv6Addr(p) == Ipv6Addr(np) for p in rio_prefixes):
                return False
    return True


def check_nwd_contains_prefix(packet, prefix):
    """Simple check if Network Data contains the given prefix in a Prefix TLV."""
    try:
        if not hasattr(packet, 'thread_nwd') or not hasattr(packet.thread_nwd.tlv, 'prefix'):
            return False
        prefixes = verify_utils.as_list(packet.thread_nwd.tlv.prefix)
        target_addr = Ipv6Addr(prefix)
        return any(p and Ipv6Addr(p) == target_addr for p in prefixes)
    except (AttributeError, IndexError):
        return False


def verify(pv):
    pkts = pv.pkts

    BR_1 = pv.vars['BR_1']
    ED_1 = pv.vars['ED_1']
    ETH_1_GUA = pv.vars['ETH_1_GUA_ADDR']

    # Strip /len from prefixes for Ipv6Addr matching
    OMR_1_PREFIX = pv.vars['OMR_1_PREFIX'].split('/')[0]
    OMR_2_PREFIX = pv.vars['OMR_2_PREFIX'].split('/')[0]
    OMR_3_PREFIX = pv.vars['OMR_3_PREFIX'].split('/')[0]
    OMR_4_PREFIX = pv.vars['OMR_4_PREFIX'].split('/')[0]

    ED_1_OMR_2 = pv.vars['ED_1_OMR_2_ADDR']
    ED_1_OMR_3 = pv.vars['ED_1_OMR_3_ADDR']
    ED_1_OMR_4 = pv.vars['ED_1_OMR_4_ADDR']

    # Step 0
    # - Device: Eth_1
    # - Description (DBR-1.8):
    #   - Harness configures Ethernet link with an on-link IPv6 GUA prefix GUA_1. Eth_1 is configured to multicast ND
    #     RAs.
    #   - Automatically configures a global address “Eth_1 GUA”.
    # - Pass Criteria
    #   - N/A
    print("Step 0: Eth_1 configured with GUA_1.")

    # Step 1
    # - Device: Eth_1, BR_2, ED_1, ED_2
    # - Description (DBR-1.8):
    #   - Enable; BR_2 automatically configures an OMR prefix for the mesh, OMR_1. Harness records the value of OMR_1
    #     and saves it for verification use.
    #   - Note: the OT command ‘br omrprefix’ should provide the prefix from BR_2.
    #   - Form topology. Wait for BR_2 to:
    #     - Become Leader
    #     - Register as border router in Thread Network Data with its OMR prefix OMR_1
    #     - Send multicast ND RAs on AIL
    #     - Automatically advertise a route to GUA_1 in the Thread Network Data, using a Prefix TLV:
    #       - Prefix = ::/0
    #       - Domain ID = <self-selected value>
    #       - Has Route sub-TLV
    # - Pass Criteria
    #   - N/A
    print("Step 1: BR_2 becomes Leader and registers OMR_1.")

    # Step 2
    # - Device: BR_1 (DUT)
    # - Description (DBR-1.8): Enable: switch on.
    # - Pass Criteria
    #   - N/A
    print("Step 2: Enable BR_1 (DUT).")

    # Step 3
    # - Device: BR_1 (DUT)
    # - Description (DBR-1.8):
    #   - Automatically registers itself as a border router in the Thread Network Data.
    # - Pass Criteria:
    #   - The DUT MUST register a route to GUA_1 in the Thread Network Data as follows:
    #     - Prefix TLV: Prefix = ::/0
    #     - Domain ID = <same value as that of BR_2 in step 1>
    #     - Has Route sub-TLV
    #       - R_preference = 00 (medium) or 11 (low)
    #   - DUT MUST NOT register an OMR prefix (e.g. OMR_1, or other) in Thread Network Data.
    print("Step 3: BR_1 (DUT) registers route to GUA_1.")
    pkts.filter_wpan_src64(BR_1).\
        filter(lambda p: hasattr(p, 'mle') and p.mle.cmd == consts.MLE_DATA_RESPONSE).\
        filter(lambda p: check_nwd_has_route(p, "::")).\
        must_next()

    # Step 4
    # - Device: BR_1 (DUT)
    # - Description (DBR-1.8):
    #   - Automatically announces the route to OMR_1 on the AIL.
    # - Pass Criteria:
    #   - The DUT MUST multicast ND RAs on the infrastructure link:
    #     - IPv6 destination MUST be ff02::1
    #     - MUST NOT contain a Prefix Information Option (PIO) with a ULA prefix.
    #     - MUST contain a Route Information Option (RIO) with OMR_1.
    #       - "Prf" bits MUST be 00 (medium) or 11 (low)
    print("Step 4: BR_1 (DUT) announces OMR_1 on AIL.")
    pkts.filter_eth_src(pv.vars['BR_1_ETH']).\
        filter_ipv6_dst("ff02::1").\
        filter(lambda p: check_ra_rio(p, OMR_1_PREFIX)).\
        filter(lambda p: not any(Ipv6Addr(pref)[0] == ULA_PREFIX_START_BYTE
                                 for pref in verify_utils.get_ra_prefixes(p)[1])).\
        must_next()

    # Step 5
    # - Device: BR_2
    # - Description (DBR-1.8):
    #   - Harness instructs device to remove the existing OMR prefix (OMR_1) being advertised and send out new network
    #     data without any prefix.
    #   - Note: for 1.3.x cert, this could be implemented by 'br disable' or 'netdata unpublish <OMR_1>' on BR_2.
    # - Pass Criteria:
    #   - N/A
    print("Step 5: BR_2 removes OMR_1.")

    # Step 6
    # - Device: BR_1 (DUT)
    # - Description (DBR-1.8):
    #   - Automatically chooses a new OMR prefix OMR_2 and includes it in the Network Data.
    # - Pass Criteria:
    #   - The DUT MUST register a new OMR prefix (OMR_2) in the Thread Network Data using Prefix TLV as follows:
    #     - Prefix = OMR_2
    #       - Border Router sub-TLV
    #         - P_preference = 11 (Low)
    #         - P_default = true
    #         - P_stable = true
    #         - P_on_mesh = true
    #         - P_preferred = true
    #         - P_slaac = true
    #         - P_dhcp = false
    #         - P_dp = false
    #   - OMR_2 MUST be 64 bits long and start with 0xFD.
    #   - OMR_2 MUST differ from OMR_1.
    #   - Also MUST contain a Prefix TLV as follows:
    #     - Prefix = ::/0
    #     - Domain ID <same value as BR_2 Domain ID>
    #     - Has Route TLV
    #       - R_preference = 00 (medium) or 11 (low)
    #   - Note: one possible way to check this is to verify the contents of the SVR_DATA.ntf message sent by DUT to
    #     Leader.
    print("Step 6: BR_1 (DUT) chooses OMR_2 and registers in Network Data.")
    if Ipv6Addr(OMR_2_PREFIX) == Ipv6Addr(OMR_1_PREFIX):
        raise verify_utils.VerificationError(f"OMR_2 ({OMR_2_PREFIX}) must differ from OMR_1 ({OMR_1_PREFIX})")

    # OMR_2 MUST be 64 bits long and start with 0xFD.
    omr2_addr = Ipv6Addr(OMR_2_PREFIX)
    if omr2_addr[0] != 0xfd:
        raise verify_utils.VerificationError(f"OMR_2 ({OMR_2_PREFIX}) must start with 0xFD")
    if int(pv.vars['OMR_2_PREFIX'].split('/')[-1]) != 64:
        raise verify_utils.VerificationError(f"OMR_2 ({pv.vars['OMR_2_PREFIX']}) must be 64 bits long")

    pkts.filter_wpan_src64(BR_1).\
        filter(lambda p: hasattr(p, 'mle') and p.mle.cmd == consts.MLE_DATA_RESPONSE).\
        filter(lambda p: verify_utils.check_nwd_prefix_flags(p,
                                                            OMR_2_PREFIX,
                                                            stable=1,
                                                            pref=BR_PREFERENCE_LOW,
                                                            r=BR_FLAG_R_TRUE,
                                                            o=BR_FLAG_O_TRUE,
                                                            p=BR_FLAG_P_TRUE,
                                                            s=BR_FLAG_S_TRUE,
                                                            d=BR_FLAG_D_FALSE,
                                                            dp=BR_FLAG_DP_FALSE)).\
        filter(lambda p: check_nwd_has_route(p, "::")).\
        must_next()

    # Step 7
    # - Device: BR_1 (DUT)
    # - Description (DBR-1.8):
    #   - Automatically multicasts ND RAs on Adjacent Infrastructure Link.
    # - Pass Criteria:
    #   - The DUT MUST multicast ND RAs on the infrastructure link:
    #     - IPv6 destination MUST be ff02::1
    #     - MUST NOT contain a Prefix Information Option (PIO) with a ULA prefix.
    #     - MUST contain a Route Information Option (RIO) with OMR_2.
    #       - "Prf" bits MUST be 00 (medium) or 11 (low).
    #     - MUST NOT contain a Route Information Option (RIO) with OMR_1.
    print("Step 7: BR_1 (DUT) multicasts ND RA with OMR_2, no OMR_1.")
    pkts.filter_eth_src(pv.vars['BR_1_ETH']).\
        filter_ipv6_dst("ff02::1").\
        filter(lambda p: check_ra_rio(p, OMR_2_PREFIX, not_prefixes=[OMR_1_PREFIX])).\
        must_next()

    # Step 8
    # - Device: Eth_1
    # - Description (DBR-1.8):
    #   - Harness instructs the device to send an ICMPv6 Echo Request to ED_1 via either one of BR_1 or BR_2.
    #     - IPv6 Source: Eth_1 GUA
    #     - IPv6 Destination: ED_1 OMR address (using prefix OMR_2)
    # - Pass Criteria:
    #   - Eth_1 receives an ICMPv6 Echo Reply from ED_1.
    #   - IPv6 Source: ED_1 OMR (using prefix OMR_2)
    #   - IPv6 Destination: Eth_1 GUA
    print("Step 8: Eth_1 pings ED_1 OMR_2.")
    _pkt = pkts.filter_eth_src(pv.vars['Eth_1_ETH']).\
        filter_ipv6_src(ETH_1_GUA).\
        filter_ipv6_dst(ED_1_OMR_2).\
        filter_ping_request().\
        must_next()
    pkts.filter(lambda p: p.eth.dst == pv.vars['Eth_1_ETH']).\
        filter_ipv6_src(ED_1_OMR_2).\
        filter_ipv6_dst(ETH_1_GUA).\
        filter_ping_reply(identifier=_pkt.icmpv6.echo.identifier).\
        must_next()

    # Step 9
    # - Device: ED_1
    # - Description (DBR-1.8):
    #   - Harness instructs the device to send an ICMPv6 Echo Request to Eth_1.
    #     - IPv6 Source: ED_1 OMR address (using prefix OMR_2)
    #     - IPv6 Destination: Eth_1 GUA
    # - Pass Criteria:
    #   - ED_1 receives an ICMPv6 Echo Reply from Eth_1.
    #     - IPv6 Source: Eth_1 GUA
    #     - IPv6 Destination: ED_1 OMR address (using prefix OMR_2)
    print("Step 9: ED_1 OMR_2 pings Eth_1 GUA.")
    _pkt = pkts.filter_ipv6_src(ED_1_OMR_2).\
        filter_ipv6_dst(ETH_1_GUA).\
        filter_ping_request().\
        must_next()
    pkts.filter(lambda p: p.ipv6.src == ETH_1_GUA).\
        filter_ipv6_dst(ED_1_OMR_2).\
        filter_ping_reply(identifier=_pkt.icmpv6.echo.identifier).\
        must_next()

    # Step 10
    # - Device: BR_2
    # - Description (DBR-1.8):
    #   - Harness instructs BR_2 to add an OMR prefix OMR_3 numerically lower than OMR_2, but with same preference
    #     "Low".
    #   - Note: can use OT command 'netdata publish prefix' for this.
    # - Pass Criteria:
    #   - N/A
    print("Step 10: BR_2 adds OMR_3.")
    # Check if OMR_3 is present in ANY node's Network Data (MLE) or BR_1's RAs.
    # Search from beginning of pcap as it might appear early due to propagation.
    _pkt_step10 = pkts.copy().filter(lambda p: (hasattr(p, 'thread_nwd') and\
                                                verify_utils.check_nwd_prefix_flags(p, OMR_3_PREFIX)) or\
                                               check_ra_rio(p, OMR_3_PREFIX)).\
        must_next()

    # Step 11
    # - Device: BR_1 (DUT)
    # - Description (DBR-1.8):
    #   - Automatically withdraws its own OMR prefix OMR_2 from the Thread Network Data.
    # - Pass Criteria:
    #   - The DUT MUST register new Network Data:
    #   - MUST NOT contain any of the OMR prefixes OMR_1, OMR_2, or OMR_3 in a Prefix TLV.
    #   - MUST contain a Prefix TLV:
    #     - Prefix = ::/0
    #     - Domain ID <same value as BR_2 Domain ID>
    #     - Has Route TLV
    #       - R_preference = 00 (medium) or 11 (low)
    print("Step 11: BR_1 (DUT) withdraws OMR_2 from Network Data.")
    # Step 11 must happen after Step 10.
    pkts.index = (_pkt_step10.number, _pkt_step10.number)
    pkts.filter(lambda p: hasattr(p, 'thread_nwd') and not check_nwd_contains_prefix(p, OMR_2_PREFIX)).must_next()

    # Step 12
    # - Device: BR_1 (DUT)
    # - Description (DBR-1.8):
    #   - Automatically multicasts ND RAs on Adjacent Infrastructure Link including OMR_2/OMR_3.
    #   - Note: the reason that OMR_2 is still included initially, is that OMR_2 has been created by BR_1 originally
    #     and as seen by BR_1, it stays in a deprecated state for at most OMR_ADDR_DEPRECATION_TIME
    # - Pass Criteria:
    #   - The DUT MUST multicast ND RAs on the infrastructure link:
    #     - IPv6 destination MUST be ff02::1
    #     - MUST NOT contain a Prefix Information Option (PIO) with a ULA prefix.
    #     - MUST NOT contain a Route Information Option (RIO) with OMR_1.
    #     - MUST contain a Route Information Option (RIO) with OMR_2.
    #       - "Prf" bits MUST be 00 (medium) or 11 (low)
    #     - MUST contain a Route Information Option (RIO) with OMR_3.
    #       - "Prf" bits MUST be 00 (medium) or 11 (low)
    print("Step 12: BR_1 (DUT) multicasts ND RA with OMR_2 and OMR_3.")
    pkts.filter_eth_src(pv.vars['BR_1_ETH']).\
        filter_ipv6_dst("ff02::1").\
        filter(lambda p: check_ra_rio(p, OMR_2_PREFIX)).\
        filter(lambda p: check_ra_rio(p, OMR_3_PREFIX)).\
        must_next()

    # Step 13
    # - Device: Eth_1
    # - Description (DBR-1.8):
    #   - Harness instructs the device to send an ICMPv6 Echo Request to ED_1 via BR_1 or BR_2.
    #     - IPv6 Source: Eth_1 GUA
    #     - IPv6 Destination: ED_1 OMR address (using prefix OMR_3)
    # - Pass Criteria:
    #   - Eth_1 receives an ICMPv6 Echo Reply from ED_1.
    #     - IPv6 Source: ED_1 OMR (using prefix OMR_3)
    #     - IPv6 Destination: Eth_1 GUA
    print("Step 13: Eth_1 pings ED_1 OMR_3.")
    _pkt = pkts.filter_eth_src(pv.vars['Eth_1_ETH']).\
        filter_ipv6_src(ETH_1_GUA).\
        filter_ipv6_dst(ED_1_OMR_3).\
        filter_ping_request().\
        must_next()
    pkts.filter(lambda p: p.eth.dst == pv.vars['Eth_1_ETH']).\
        filter_ipv6_src(ED_1_OMR_3).\
        filter_ipv6_dst(ETH_1_GUA).\
        filter_ping_reply(identifier=_pkt.icmpv6.echo.identifier).\
        must_next()

    # Step 14
    # - Device: ED_1
    # - Description (DBR-1.8):
    #   - Harness instructs the device to send an ICMPv6 Echo Request to Eth_1.
    #     - IPv6 Source: ED_1 OMR address (using prefix OMR_3)
    #     - IPv6 Destination: Eth_1 GUA
    # - Pass Criteria:
    #   - ED_1 receives an ICMPv6 Echo Reply from Eth_1.
    #     - IPv6 Source: Eth_1 GUA
    #     - IPv6 Destination: ED_1 OMR address (using prefix OMR_3)
    print("Step 14: ED_1 OMR_3 pings Eth_1 GUA.")
    _pkt = pkts.filter_ipv6_src(ED_1_OMR_3).\
        filter_ipv6_dst(ETH_1_GUA).\
        filter_ping_request().\
        must_next()
    pkts.filter(lambda p: p.ipv6.src == ETH_1_GUA).\
        filter_ipv6_dst(ED_1_OMR_3).\
        filter_ping_reply(identifier=_pkt.icmpv6.echo.identifier).\
        must_next()

    # Step 15
    # - Device: BR_2
    # - Description (DBR-1.8):
    #   - Harness instructs device to remove existing OMR prefix (OMR_3) that is being advertised and sends out new
    #     network data without the prefix OMR_3.
    #   - Note: can use OT command 'netdata unpublish <prefix>' for this.
    # - Pass Criteria:
    #   - N/A
    print("Step 15: BR_2 removes OMR_3.")

    # Step 16
    # - Device: BR_1 (DUT)
    # - Description (DBR-1.8):
    #   - Automatically starts advertising its own OMR prefix OMR_2 again.
    # - Pass Criteria:
    #   - The DUT MUST register its OMR prefix OMR_2 in the Thread Network Data. Flags in the Border Router sub-TLV
    #     MUST be:
    #     - P_preference = 11 (Low)
    #     - P_default = true
    #     - P_stable = true
    #     - P_on_mesh = true
    #     - P_preferred = true
    #     - P_slaac = true
    #     - P_dhcp = false
    #     - P_dp = false
    #   - OMR_2 MUST be 64 bits long and start with 0xFD.
    #   - OMR_2 MUST be equal to the OMR_2 values as used in previous test steps.
    print("Step 16: BR_1 (DUT) re-registers OMR_2.")
    # OMR_2 MUST be 64 bits long and start with 0xFD.
    omr2_addr = Ipv6Addr(OMR_2_PREFIX)
    if omr2_addr[0] != 0xfd:
        raise verify_utils.VerificationError(f"OMR_2 ({OMR_2_PREFIX}) must start with 0xFD")
    if int(pv.vars['OMR_2_PREFIX'].split('/')[-1]) != 64:
        raise verify_utils.VerificationError(f"OMR_2 ({pv.vars['OMR_2_PREFIX']}) must be 64 bits long")

    pkts.filter(lambda p: verify_utils.check_nwd_prefix_flags(p,
                                                            OMR_2_PREFIX,
                                                            stable=1,
                                                            pref=BR_PREFERENCE_LOW,
                                                            r=BR_FLAG_R_TRUE,
                                                            o=BR_FLAG_O_TRUE,
                                                            p=BR_FLAG_P_TRUE,
                                                            s=BR_FLAG_S_TRUE,
                                                            d=BR_FLAG_D_FALSE,
                                                            dp=BR_FLAG_DP_FALSE)).\
        must_next()

    # Step 17
    # - Device: BR_1 (DUT)
    # - Description (DBR-1.8):
    #   - Automatically multicasts ND RAs on Adjacent Infrastructure Link with OMR_2.
    #   - Note: the reason that OMR_3 is not advertised, is that OMR_3 was withdrawn and was not originally created by
    #     BR_1. So BR_1 has no responsibility to advertise this deprecated prefix.
    # - Pass Criteria:
    #   - The DUT MUST multicast ND RAs on the infrastructure link:
    #     - IPv6 destination MUST be ff02::1
    #     - MUST NOT contain a Prefix Information Option (PIO) with a ULA prefix.
    #     - MUST NOT contain a Route Information Option (RIO) with OMR_1.
    #     - MUST contain a Route Information Option (RIO) with OMR_2.
    #       - "Prf" bits MUST be 00 (medium) or 11 (low)
    #     - MUST NOT contain a Route Information Option (RIO) with OMR_3.
    print("Step 17: BR_1 (DUT) multicasts ND RA with OMR_2, no OMR_3.")
    pkts.filter_eth_src(pv.vars['BR_1_ETH']).\
        filter_ipv6_dst("ff02::1").\
        filter(lambda p: check_ra_rio(p, OMR_2_PREFIX)).\
        filter(lambda p: not check_ra_rio(p, OMR_3_PREFIX)).\
        must_next()

    # Step 18
    # - Device: BR_2
    # - Description (DBR-1.8):
    #   - Harness instructs the device to add new OMR prefix (OMR_4) numerically higher than OMR_2, but with same
    #     preference "Low".
    # - Pass Criteria:
    #   - N/A
    print("Step 18: BR_2 adds OMR_4.")
    pkts.filter(lambda p: check_nwd_contains_prefix(p, OMR_4_PREFIX)).\
        must_next()

    # Step 19
    # - Device: BR_1 (DUT)
    # - Description (DBR-1.8):
    #   - No change in behavior for advertising its OMR prefix, OMR_2.
    #   - Automatically starts advertising a route for the new OMR_4 prefix.
    # - Pass Criteria:
    #   - The DUT MUST NOT send a Network Data update with any one of the following prefixes in a Prefix TLV:
    #     - OMR_1, OMR_2, OMR_3, OMR_4
    #   - The DUT MUST multicast ND RAs on the infrastructure link:
    #     - IPv6 destination MUST be ff02::1
    #     - MUST NOT contain a Prefix Information Option (PIO) with a ULA prefix.
    #     - MUST contain a Route Information Option (RIO) with OMR_2.
    #       - "Prf" bits MUST be 00 (medium) or 11 (low).
    #     - MUST contain a Route Information Option (RIO) with OMR_4.
    #       - "Prf" bits MUST be 00 (medium) or 11 (low).
    #     - MUST NOT contain RIO with any one of OMR_1, OMR_3.
    print("Step 19: BR_1 (DUT) continues OMR_2, adds OMR_4 on AIL.")
    pkts.filter_eth_src(pv.vars['BR_1_ETH']).\
        filter_ipv6_dst("ff02::1").\
        filter(lambda p: check_ra_rio(p, OMR_2_PREFIX)).\
        filter(lambda p: check_ra_rio(p, OMR_4_PREFIX)).\
        must_next()

    # Step 20
    # - Device: BR_2
    # - Description (DBR-1.8):
    #   - Harness instructs the device to update the numerically higher OMR_4 with a higher preference value 00
    #     ('Medium').
    # - Pass Criteria:
    #   - N/A
    print("Step 20: BR_2 updates OMR_4 to Medium pref.")
    pkts.filter(lambda p: verify_utils.check_nwd_prefix_flags(p, OMR_4_PREFIX, pref=BR_PREFERENCE_MEDIUM)).\
        must_next()

    # Step 21
    # - Device: BR_1 (DUT)
    # - Description (DBR-1.8): Automatically withdraws its current OMR prefix OMR_2.
    # - Pass Criteria:
    #   - The DUT MUST send a Network Data update. In the new network data:
    #     - the following prefixes MUST NOT be present in a Prefix TLV: OMR_1, OMR_2, OMR_3
    #   - The DUT MUST multicast ND RAs on the infrastructure link:
    #     - IPv6 destination MUST be ff02::1
    #     - MUST NOT contain a Prefix Information Option (PIO) with a ULA prefix.
    #     - MUST contain a Route Information Option (RIO) with OMR_2:
    #       - "Prf" bits MUST be 00 (medium) or 11 (low).
    #     - MUST contain a Route Information Option (RIO) with OMR_4:
    #       - "Prf" bits MUST be 00 (medium) or 11 (low).
    #     - MUST NOT contain a Route Information Option (RIO) with OMR_1 or OMR_3.
    print("Step 21: BR_1 (DUT) withdraws OMR_2.")
    pkts.filter(lambda p: not check_nwd_contains_prefix(p, OMR_2_PREFIX)).\
        must_next()

    # Step 22
    # - Device: BR_2
    # - Description (DBR-1.8):
    #   - Harness instructs the device to update the prefix OMR_4 to a deprecated state by setting P_preferred =
    #     'false'.
    #   - Note: can use OT command 'netdata publish prefix' for this where the flags do not contain the 'p' (Preferred)
    #     flag.
    # - Pass Criteria:
    #   - N/A
    print("Step 22: BR_2 deprecates OMR_4.")
    pkts.filter(lambda p: verify_utils.check_nwd_prefix_flags(p, OMR_4_PREFIX, p=BR_FLAG_P_FALSE)).\
        must_next()

    # Step 23
    # - Device: BR_1 (DUT)
    # - Description (DBR-1.8): Automatically starts advertising its own OMR prefix OMR_2 again.
    # - Pass Criteria:
    #   - The DUT MUST register its OMR prefix OMR_2 in the Thread Network Data. Flags in the Border Router sub-TLV
    #     MUST be:
    #     - P_preference = 11 (Low)
    #     - P_default = true
    #     - P_stable = true
    #     - P_on_mesh = true
    #     - P_preferred = true
    #     - P_slaac = true
    #     - P_dhcp = false
    #     - P_dp = false
    #   - OMR_2 MUST be 64 bits long and start with 0xFD.
    #   - OMR_2 MUST be equal to the OMR_2 values as used in previous test steps.
    print("Step 23: BR_1 (DUT) re-registers OMR_2.")
    # OMR_2 MUST be 64 bits long and start with 0xFD.
    omr2_addr = Ipv6Addr(OMR_2_PREFIX)
    if omr2_addr[0] != 0xfd:
        raise verify_utils.VerificationError(f"OMR_2 ({OMR_2_PREFIX}) must start with 0xFD")
    if int(pv.vars['OMR_2_PREFIX'].split('/')[-1]) != 64:
        raise verify_utils.VerificationError(f"OMR_2 ({pv.vars['OMR_2_PREFIX']}) must be 64 bits long")

    pkts.filter(lambda p: verify_utils.check_nwd_prefix_flags(p,
                                                            OMR_2_PREFIX,
                                                            stable=1,
                                                            pref=BR_PREFERENCE_LOW,
                                                            r=BR_FLAG_R_TRUE,
                                                            o=BR_FLAG_O_TRUE,
                                                            p=BR_FLAG_P_TRUE,
                                                            s=BR_FLAG_S_TRUE,
                                                            d=BR_FLAG_D_FALSE,
                                                            dp=BR_FLAG_DP_FALSE)).\
        must_next()

    # Step 24
    # - Device: BR_1 (DUT)
    # - Description (DBR-1.8):
    #   - Automatically multicasts ND RAs on Adjacent Infrastructure Link.
    #   - Note: even though OMR_4 is deprecated, it is still valid so it is advertised on the AIL in a RIO.
    # - Pass Criteria:
    #   - The DUT MUST multicast ND RAs on the infrastructure link:
    #     - IPv6 destination MUST be ff02::1
    #     - MUST NOT contain a Prefix Information Option (PIO) with a ULA prefix.
    #     - MUST NOT contain a Route Information Option (RIO) with OMR_1.
    #     - MUST contain a Route Information Option (RIO) with OMR_2.
    #       - "Prf" bits MUST be 00 (medium) or 11 (low)
    #     - MUST NOT contain a Route Information Option (RIO) with OMR_3.
    #     - MUST contain a Route Information Option (RIO) with OMR_4.
    #       - "Prf" bits MUST be 00 (medium) or 11 (low)
    print("Step 24: BR_1 (DUT) multicasts ND RA with OMR_2 and OMR_4.")
    pkts.filter_eth_src(pv.vars['BR_1_ETH']).\
        filter_ipv6_dst("ff02::1").\
        filter(lambda p: check_ra_rio(p, OMR_2_PREFIX)).\
        filter(lambda p: check_ra_rio(p, OMR_4_PREFIX)).\
        must_next()

    # Step 25
    # - Device: Eth_1
    # - Description (DBR-1.8):
    #   - Harness instructs the device to send an ICMPv6 Echo Request to ED_1 via BR_1.
    #     - IPv6 Source: Eth_1 GUA
    #     - IPv6 Destination: ED_1 OMR address (using prefix OMR_2)
    # - Pass Criteria:
    #   - Eth_1 receives an ICMPv6 Echo Reply from ED_1.
    #     - IPv6 Source: ED_1 OMR (using prefix OMR_2)
    #     - IPv6 Destination: Eth_1 GUA
    print("Step 25: Eth_1 pings ED_1 OMR_2.")
    _pkt = pkts.filter_eth_src(pv.vars['Eth_1_ETH']).\
        filter_ipv6_src(ETH_1_GUA).\
        filter_ipv6_dst(ED_1_OMR_2).\
        filter_ping_request().\
        must_next()
    pkts.filter(lambda p: p.eth.dst == pv.vars['Eth_1_ETH']).\
        filter_ipv6_src(ED_1_OMR_2).\
        filter_ipv6_dst(ETH_1_GUA).\
        filter_ping_reply(identifier=_pkt.icmpv6.echo.identifier).\
        must_next()

    # Step 26
    # - Device: ED_1
    # - Description (DBR-1.8):
    #   - Harness instructs the device to send an ICMPv6 Echo Request to Eth_1.
    #     - IPv6 Source: ED_1 OMR address (using prefix OMR_2)
    #     - IPv6 Destination: Eth_1 GUA
    # - Pass Criteria:
    #   - ED_1 receives an ICMPv6 Echo Reply from Eth_1.
    #     - IPv6 Source: Eth_1 GUA
    #     - IPv6 Destination: ED_1 OMR address (using prefix OMR_2)
    print("Step 26: ED_1 OMR_2 pings Eth_1 GUA.")
    _pkt = pkts.filter_ipv6_src(ED_1_OMR_2).\
        filter_ipv6_dst(ETH_1_GUA).\
        filter_ping_request().\
        must_next()
    pkts.filter(lambda p: p.ipv6.src == ETH_1_GUA).\
        filter_ipv6_dst(ED_1_OMR_2).\
        filter_ping_reply(identifier=_pkt.icmpv6.echo.identifier).\
        must_next()

    # Step 27
    # - Device: BR_2
    # - Description (DBR-1.8):
    #   - Harness disables the BR function of the device. Or if that is not possible, switch it off.
    #   - Note: disabling BR may use 'br disable' CLI command.
    #   - Note: in case of switching off BR_2, the OMR prefix OMR_4 that was advertised by BR_2 remains active for some
    #     time. The Leader timeout of Leader BR_2 does not happen yet during the below steps of this test.
    # - Pass Criteria:
    #   - N/A
    print("Step 27: BR_2 disabled.")

    # Step 28
    # - Device: Eth_1
    # - Description (DBR-1.8):
    #   - Harness instructs the device to send an ICMPv6 Echo Request to ED_1 via BR_1.
    #     - IPv6 Source: Eth_1 GUA
    #     - IPv6 Destination: ED_1 OMR address (using prefix OMR_2)
    # - Pass Criteria:
    #   - Eth_1 receives an ICMPv6 Echo Reply from ED_1.
    #     - IPv6 Source: ED_1 OMR (using prefix OMR_2)
    #     - IPv6 Destination: Eth_1 GUA
    print("Step 28: Eth_1 pings ED_1 OMR_2.")
    _pkt = pkts.filter_eth_src(pv.vars['Eth_1_ETH']).\
        filter_ipv6_src(ETH_1_GUA).\
        filter_ipv6_dst(ED_1_OMR_2).\
        filter_ping_request().\
        must_next()
    pkts.filter(lambda p: p.eth.dst == pv.vars['Eth_1_ETH']).\
        filter_ipv6_src(ED_1_OMR_2).\
        filter_ipv6_dst(ETH_1_GUA).\
        filter_ping_reply(identifier=_pkt.icmpv6.echo.identifier).\
        must_next()

    # Step 29
    # - Device: ED_1
    # - Description (DBR-1.8):
    #   - Harness instructs the device to send an ICMPv6 Echo Request to Eth_1.
    #     - IPv6 Source: ED_1 OMR address (using prefix OMR_4)
    #     - IPv6 Destination: Eth_1 GUA.
    #   - Note: ED_1 is forced here to use a source address based on prefix OMR_4, even though that prefix is
    #     deprecated. In OT CLI this can most likely be achieved using the command:
    #     - ping -I <ED_1-OMR_4-address> <Eth_1-GUA>
    # - Pass Criteria:
    #   - N/A
    print("Step 29: ED_1 pings Eth_1 using OMR_4.")
    _pkt = pkts.filter_ipv6_src(ED_1_OMR_4).\
        filter_ipv6_dst(ETH_1_GUA).\
        filter_ping_request().\
        must_next()

    # Step 30
    # - Device: Eth_1
    # - Description (DBR-1.8):
    #   - Automatically replies to the ICMPv6 Echo Request with Echo Reply, that is sent to BR_1.
    # - Pass Criteria:
    # Step 30: Eth_1 replies to OMR_4.
    print("Step 30: Eth_1 replies to OMR_4.")
    _pkt_step30 = pkts.filter_ping_reply(identifier=_pkt.icmpv6.echo.identifier).filter(lambda p: (
        hasattr(p, 'eth') and p.eth.src == pv.vars['Eth_1_ETH'] and \
        hasattr(p, 'ipv6') and \
        getattr(p.ipv6, 'dst', None) and Ipv6Addr(p.ipv6.dst)[:8] == Ipv6Addr(OMR_4_PREFIX)[:8] and \
        getattr(p.ipv6, 'src', None) and Ipv6Addr(p.ipv6.src) == ETH_1_GUA
    )).must_next()

    # Step 31
    # - Device: BR_1 (DUT)
    # - Description (DBR-1.8):
    #   - Automatically attempts to deliver the Echo Reply to a node on the mesh.
    # - Pass Criteria:
    #   - The DUT MUST attempt to deliver the packet to a node on the mesh, either:
    #     - 1. BR_1 sends a multicast Address Query for ED_1 OMR_4 based address into the mesh;
    #     - 2. or ED_1 receives the Echo Reply from Eth_1 as follows:
    #       - IPv6 Source: Eth_1 GUA
    #       - IPv6 Destination: ED_1 OMR address (using prefix OMR_4)
    print("Step 31: BR_1 delivers Echo Reply for OMR_4.")

    def check_step_31(p):
        is_aq = (hasattr(p, 'thread_address') and \
                 hasattr(p.thread_address, 'tlv') and \
                 getattr(p.thread_address.tlv, 'target_eid', None) and \
                 Ipv6Addr(p.thread_address.tlv.target_eid)[:8] == Ipv6Addr(OMR_4_PREFIX)[:8])

        # We allow matching by IID as well because 6LoWPAN context IDs might
        # not be correctly mapped in the verifier's Wireshark preferences.
        is_er = (hasattr(p, 'wpan') and \
                 hasattr(p, 'ipv6') and \
                 getattr(p.ipv6, 'dst', None) and \
                 (Ipv6Addr(p.ipv6.dst) == ED_1_OMR_4 or \
                  Ipv6Addr(p.ipv6.dst)[8:] == Ipv6Addr(ED_1_OMR_4)[8:]) and\
                 hasattr(p, 'icmpv6') and p.icmpv6.type == consts.ICMPV6_TYPE_ECHO_REPLY)

        return is_aq or (is_er and p.wpan.src64 == BR_1 and p.icmpv6.echo.identifier == _pkt.icmpv6.echo.identifier)

    pkts.filter(check_step_31).must_next()
    # Note: Step 31 must be observed. Even though BR_2 is disabled, the route to OMR_4
    # prefix remains in Network Data, and BR_1 should still be able to forward the
    # Echo Reply using the EID-to-RLOC cache.


if __name__ == '__main__':
    verify_utils.run_main(verify)
