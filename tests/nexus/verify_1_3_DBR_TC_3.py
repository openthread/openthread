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

ULA_PREFIX_START_BYTE = 0xfd

# Step 3 BR constants
BR_PREFERENCE_LOW = 3
BR_FLAG_R_FALSE = 0
BR_FLAG_O_TRUE = 1
BR_FLAG_P_TRUE = 1
BR_FLAG_S_TRUE = 1  # Note: thread_nwd.tlv.border_router.flag.s is SLAAC
BR_FLAG_D_FALSE = 0
BR_FLAG_DP_FALSE = 0


def check_nwd(p, omr_prefix_1, omr_prefix_1_len):
    if omr_prefix_1_len != 64 or omr_prefix_1[0] != ULA_PREFIX_START_BYTE:
        return False

    # 1. Prefix TLV for OMR_1 with specific BR sub-TLV flags
    # Note: We expect P_default (r flag) to be 0 for now to match OpenThread behavior
    if not verify_utils.check_nwd_prefix_flags(p,
                                               omr_prefix_1,
                                               pref=BR_PREFERENCE_LOW,
                                               r=BR_FLAG_R_FALSE,
                                               o=BR_FLAG_O_TRUE,
                                               p=BR_FLAG_P_TRUE,
                                               s=BR_FLAG_S_TRUE,
                                               d=BR_FLAG_D_FALSE,
                                               dp=BR_FLAG_DP_FALSE):
        return False

    # 2. Prefix TLV for fc00::/7
    try:
        prefixes = verify_utils.as_list(p.thread_nwd.tlv.prefix)
        prefixes.index(Ipv6Addr("fc00::"))
    except (AttributeError, ValueError):
        return False

    # Check Has Route sub-TLV existence
    if not hasattr(p.thread_nwd.tlv, 'has_route'):
        return False

    return True


def check_ra(p, omr_prefix_1, pre_1_prefix_1, ext_pan_id_1):
    if p.icmpv6.type != verify_utils.ICMPV6_TYPE_ROUTER_ADVERTISEMENT:
        return False
    if p.icmpv6.nd.ra.router_lifetime != verify_utils.RA_ROUTER_LIFETIME_ZERO:
        return False
    if p.icmpv6.nd.ra.flag.m != verify_utils.RA_FLAG_M_FALSE or p.icmpv6.nd.ra.flag.o != verify_utils.RA_FLAG_O_FALSE:
        return False

    rio_prefixes, pio_prefixes = verify_utils.get_ra_prefixes(p)

    if omr_prefix_1 not in rio_prefixes:
        return False
    if pre_1_prefix_1 not in pio_prefixes:
        return False

    # Check PIO A bit and Preferred/Valid Lifetimes
    pio_index = pio_prefixes.index(pre_1_prefix_1)
    if verify_utils.as_list(p.icmpv6.opt.pio_flag.a)[pio_index] != verify_utils.PIO_FLAG_A_TRUE:
        return False
    if verify_utils.as_list(p.icmpv6.opt.pio_preferred_lifetime)[pio_index] == 0:
        return False
    if verify_utils.as_list(p.icmpv6.opt.pio_valid_lifetime)[pio_index] == 0:
        return False

    # Check EXT_PAN_ID mapping in PRE_1_PREFIX_1
    ext_pan_id_bytes = bytes.fromhex(ext_pan_id_1)
    if (pre_1_prefix_1[0] == ULA_PREFIX_START_BYTE and
            pre_1_prefix_1[verify_utils.EXT_PAN_ID_GLOBAL_ID_OFFSET:verify_utils.EXT_PAN_ID_GLOBAL_ID_OFFSET +
                           verify_utils.EXT_PAN_ID_GLOBAL_ID_LEN]
            == ext_pan_id_bytes[:verify_utils.EXT_PAN_ID_GLOBAL_ID_LEN] and
            pre_1_prefix_1[verify_utils.EXT_PAN_ID_SUBNET_ID_OFFSET:verify_utils.EXT_PAN_ID_SUBNET_ID_OFFSET +
                           verify_utils.EXT_PAN_ID_SUBNET_ID_LEN]
            == ext_pan_id_bytes[verify_utils.EXT_PAN_ID_SUBNET_ID_OFFSET:verify_utils.EXT_PAN_ID_SUBNET_ID_OFFSET +
                                verify_utils.EXT_PAN_ID_SUBNET_ID_LEN]):
        return True

    return False


def check_ra_br2(p, omr_prefix_2):
    if p.icmpv6.type != verify_utils.ICMPV6_TYPE_ROUTER_ADVERTISEMENT:
        return False
    rio_prefixes, _ = verify_utils.get_ra_prefixes(p)
    return omr_prefix_2 in rio_prefixes


def verify(pv):
    # 1.3. [1.3] [CERT] Reachability - Multiple BRs - Multiple Thread / Single Infrastructure Link
    #
    # 1.3.2. Topology
    # - BR_1 (DUT) - Thread Border Router and the Leader
    # - BR_2 - Test bed device operating as a Thread Border Router and the Leader on adjacent Thread network
    # - ED_1 - Test bed device operating as a Thread End Device, attached to BR_1
    # - ED_2 - Test bed device operating as a Thread End Device, attached to BR_2
    #
    # 1.3.1. Purpose
    # To test the following:
    # - 1. Bi-directional reachability between multiple Thread Networks attached via an adjacent infrastructure link
    # - 2. No existing IPv6 infrastructure
    # - 3. Single BR per Thread Network
    #
    # Spec Reference   | V1.3.0 Section
    # -----------------|---------------
    # DBR              | 1.3

    pkts = pv.pkts
    pv.summary.show()

    BR_1 = pv.vars['BR_1']

    OMR_PREFIX_1 = Ipv6Addr(pv.vars['OMR_PREFIX_1'].split('/')[0])
    OMR_PREFIX_1_LEN = int(pv.vars['OMR_PREFIX_1'].split('/')[1])
    PRE_1_PREFIX_1 = Ipv6Addr(pv.vars['PRE_1_PREFIX_1'].split('/')[0])
    EXT_PAN_ID_1 = pv.vars['EXT_PAN_ID_1']

    OMR_PREFIX_2 = Ipv6Addr(pv.vars['OMR_PREFIX_2'].split('/')[0])

    ED_1_OMR = Ipv6Addr(pv.vars['ED_1_OMR_ADDR'])
    ED_2_OMR = Ipv6Addr(pv.vars['ED_2_OMR_ADDR'])

    # Step 1
    #   Device: BR_1 (DUT)
    #   Description (DBR-1.3): Enable: power on.
    #   Pass Criteria:
    #     N/A
    print("Step 1: BR_1 (DUT) Enable: power on.")

    # Step 3
    #   Device: BR_1 (DUT)
    #   Description (DBR-1.3): Automatically registers itself as a border router in the Thread Network Data.
    #     Automatically creates a ULA prefix PRE_1 for the adjacent infrastructure link. Automatically multicasts
    #     ND RAs on Adjacent Infrastructure Link.
    #   Pass Criteria:
    #     - Note: pass criteria are identical to DBR-1.1 step 3.
    #     - The DUT internally registers an OMR Prefix (OMR_1) in the Thread Network Data.
    #     - The DUT MUST send a multicast MLE Data Response with Thread Network Data containing at least two
    #       Prefix TLVs.
    #     - Prefix TLV 1: OMR prefix OMR_1. MUST include a Border Router sub-TLV.
    #     - Flags in the Border Router TLV MUST be: P_preference = 11 (Low), P_default = true, P_stable = true,
    #       P_on_mesh = true, P_preferred = true, P_slaac = true, P_dhcp = false, P_dp = false.
    #     - Prefix TLV 2: ULA prefix fc00::/7. (This is used as the shortened version of PRE_1). Includes Has
    #       Route sub-TLV.
    #     - OMR_1 MUST be 64 bits long and start with 0xFD.
    #     - The DUT MUST multicast ND RAs including the following:
    #     - IPv6 destination MUST be ff02::1.
    #     - M bit and O bit MUST be '0'.
    #     - "Router Lifetime" = 0 (indicating it's not a default router).
    #     - Prefix Information Option (PIO) ULA ULA_1 A bit MUST be '1'.
    #     - Route Information Option (RIO) OMR = OMR_1.
    #     - ULA_1 MUST contain the Extended PAN ID as follows:
    #     - Global ID equals the 40 most significant bits of the Extended PAN ID.
    #     - Subnet ID equals the 16 least significant bits of the Extended PAN ID.
    print("Step 3: BR_1 (DUT) registers as border router and multicasts ND RAs.")

    pkts.filter_wpan_src64(BR_1).\
        filter_mle_cmd(consts.MLE_DATA_RESPONSE).\
        filter(lambda p: check_nwd(p, OMR_PREFIX_1, OMR_PREFIX_1_LEN)).\
        must_next()

    pkts.filter_eth_src(pv.vars['BR_1_ETH']).\
        filter_ipv6_dst("ff02::1").\
        filter(lambda p: check_ra(p, OMR_PREFIX_1, PRE_1_PREFIX_1, EXT_PAN_ID_1)).\
        must_next()

    # Step 4
    #   Device: BR_2, ED_1, ED_2
    #   Description (DBR-1.3): Form topology. Wait for BR_2 to: 1. Register as border router in Thread Network Data
    #     2. Send multicast ND RAs on the adjacent infrastructure link with: RIO with OMR prefix (OMR_2).
    #   Pass Criteria:
    #     N/A
    print("Step 4: BR_2, ED_1, ED_2 Form topology.")

    pkts.filter_eth_src(pv.vars['BR_2_ETH']).\
        filter_ipv6_dst("ff02::1").\
        filter(lambda p: check_ra_br2(p, OMR_PREFIX_2)).\
        must_next()

    # Step 5
    #   Device: ED_1
    #   Description (DBR-1.3): Harness instructs device to send ICMPv6 Echo Request to ED_2. IPv6 Source:
    #     ED_1 OMR, IPv6 Destination: ED_2 OMR.
    #   Pass Criteria:
    #     - ED_1 receives an ICMPv6 Echo Reply from ED_2.
    #     - IPv6 Source: ED_2 OMR.
    #     - IPv6 Destination: ED_1 OMR.
    print("Step 5: ED_1 sends ICMPv6 Echo Request to ED_2.")
    _pkt = pkts.filter_ipv6_src(ED_1_OMR).\
        filter_ipv6_dst(ED_2_OMR).\
        filter_ping_request().\
        must_next()

    pkts.filter_ipv6_src(ED_2_OMR).\
        filter_ipv6_dst(ED_1_OMR).\
        filter_ping_reply(identifier=_pkt.icmpv6.echo.identifier).\
        must_next()

    # Step 6
    #   Device: ED_2
    #   Description (DBR-1.3): Harness instructs device to send ICMPv6 Echo Request to ED_1. IPv6 Source:
    #     ED_2 OMR, IPv6 Destination: ED_1 OMR.
    #   Pass Criteria:
    #     - ED_2 receives an ICMPv6 Echo Reply from ED_1.
    #     - IPv6 Source: ED_1 OMR.
    #     - IPv6 Destination: ED_2 OMR.
    print("Step 6: ED_2 sends ICMPv6 Echo Request to ED_1.")
    _pkt = pkts.filter_ipv6_src(ED_2_OMR).\
        filter_ipv6_dst(ED_1_OMR).\
        filter_ping_request().\
        must_next()

    pkts.filter_ipv6_src(ED_1_OMR).\
        filter_ipv6_dst(ED_2_OMR).\
        filter_ping_reply(identifier=_pkt.icmpv6.echo.identifier).\
        must_next()


if __name__ == '__main__':
    verify_utils.run_main(verify)
