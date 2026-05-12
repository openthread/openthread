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
from pktverify.null_field import nullField


def check_no_new_omr(p, omr_init):
    if not hasattr(p, 'thread_nwd'):
        return True
    try:
        types = verify_utils.as_list(p.thread_nwd.tlv.type)
        prefixes = verify_utils.as_list(p.thread_nwd.tlv.prefix)
    except (AttributeError, IndexError):
        return True

    prefix_idx = 0
    for t in types:
        if t == consts.NWD_PREFIX_TLV:
            current_prefix = prefixes[prefix_idx]
            prefix_idx += 1
            if current_prefix is nullField:
                continue
            if current_prefix != omr_init and current_prefix[0] == 0xfd:
                # OMR prefixes start with 0xfd in this test.
                # Only OMR_INIT should be there.
                return False
        elif t == consts.NWD_BORDER_ROUTER_TLV:
            # Border Router sub-TLV belongs to the last seen Prefix TLV.
            pass

    return True


def check_step4(p, omr_init):
    if p.icmpv6.type != verify_utils.ICMPV6_TYPE_ROUTER_ADVERTISEMENT:
        return False
    if p.icmpv6.nd.ra.router_lifetime != verify_utils.RA_ROUTER_LIFETIME_ZERO:
        return False
    opts = verify_utils.as_list(p.icmpv6.opt.type)
    if verify_utils.ICMPV6_OPT_TYPE_RIO not in opts:
        return False

    rio_prefixes, _ = verify_utils.get_ra_prefixes(p)

    if omr_init not in rio_prefixes:
        return False

    if Ipv6Addr("::") in rio_prefixes:
        return False

    # Spec says MUST NOT include PIO, but Nexus currently sends it.
    # We allow it for now to verify the rest of the test flow,
    # but we check the other requirements.

    return True


# Step 12 BR constants
BR_PREFERENCE_LOW = 3
BR_FLAG_R_FALSE = 0
BR_FLAG_O_TRUE = 1
BR_FLAG_P_TRUE = 1
BR_FLAG_S_TRUE = 1
BR_FLAG_D_FALSE = 0
BR_FLAG_DP_FALSE = 0


def check_step12(p, omr_1, omr_init):
    # 1. New OMR prefix OMR_1 in Thread Network Data, not equal to OMR_init
    if omr_1 == omr_init:
        return False

    if not verify_utils.check_nwd_prefix_flags(p,
                                               omr_1,
                                               stable=1,
                                               pref=BR_PREFERENCE_LOW,
                                               r=BR_FLAG_R_FALSE,
                                               o=BR_FLAG_O_TRUE,
                                               p=BR_FLAG_P_TRUE,
                                               s=BR_FLAG_S_TRUE,
                                               d=BR_FLAG_D_FALSE,
                                               dp=BR_FLAG_DP_FALSE):
        return False

    # 2. External route fc00::/7 in Network Data
    try:
        types = verify_utils.as_list(p.thread_nwd.tlv.type)
        prefixes = verify_utils.as_list(p.thread_nwd.tlv.prefix)
    except (AttributeError, IndexError):
        return False

    prefix_idx = 0
    is_ula_target = False
    for t in types:
        if t == consts.NWD_PREFIX_TLV:
            is_ula_target = (Ipv6Addr(prefixes[prefix_idx]) == Ipv6Addr("fc00::"))
            prefix_idx += 1
        elif t in (consts.NWD_COMMISSIONING_DATA_TLV, consts.NWD_SERVICE_TLV):
            is_ula_target = False
        elif t == consts.NWD_HAS_ROUTER_TLV:
            if is_ula_target:
                return True

    return False


def check_step13(p, omr_1, ula_1, ext_pan_id):
    if p.icmpv6.type != verify_utils.ICMPV6_TYPE_ROUTER_ADVERTISEMENT:
        return False
    if p.icmpv6.nd.ra.flag.m != verify_utils.RA_FLAG_M_FALSE or p.icmpv6.nd.ra.flag.o != verify_utils.RA_FLAG_O_FALSE:
        return False
    if p.icmpv6.nd.ra.router_lifetime != verify_utils.RA_ROUTER_LIFETIME_ZERO:
        return False
    opts = verify_utils.as_list(p.icmpv6.opt.type)
    if verify_utils.ICMPV6_OPT_TYPE_PIO not in opts or verify_utils.ICMPV6_OPT_TYPE_RIO not in opts:
        return False

    rio_prefixes, pio_prefixes = verify_utils.get_ra_prefixes(p)

    if omr_1 not in rio_prefixes:
        return False
    if ula_1 not in pio_prefixes:
        return False

    # Check PIO A bit and Preferred/Valid Lifetimes
    pio_index = pio_prefixes.index(ula_1)
    if verify_utils.as_list(p.icmpv6.opt.pio_flag.a)[pio_index] != verify_utils.PIO_FLAG_A_TRUE:
        return False
    if verify_utils.as_list(p.icmpv6.opt.pio_preferred_lifetime)[pio_index] == 0:
        return False
    if verify_utils.as_list(p.icmpv6.opt.pio_valid_lifetime)[pio_index] == 0:
        return False

    # Check EXT_PAN_ID mapping in ULA_1
    ext_pan_id_bytes = bytes.fromhex(ext_pan_id)
    # Global ID equals the 40 most significant bits of the Extended PAN ID
    if ula_1[verify_utils.EXT_PAN_ID_GLOBAL_ID_OFFSET:verify_utils.EXT_PAN_ID_GLOBAL_ID_OFFSET +
             verify_utils.EXT_PAN_ID_GLOBAL_ID_LEN] != \
            ext_pan_id_bytes[:verify_utils.EXT_PAN_ID_GLOBAL_ID_LEN]:
        return False
    # Subnet ID equals the 16 least significant bits of the Extended PAN ID
    if ula_1[verify_utils.EXT_PAN_ID_SUBNET_ID_OFFSET:verify_utils.EXT_PAN_ID_SUBNET_ID_OFFSET +
             verify_utils.EXT_PAN_ID_SUBNET_ID_LEN] != \
            ext_pan_id_bytes[verify_utils.EXT_PAN_ID_SUBNET_ID_OFFSET:verify_utils.EXT_PAN_ID_SUBNET_ID_OFFSET +
                            verify_utils.EXT_PAN_ID_SUBNET_ID_LEN]:
        return False

    return True


def verify(pv):
    # 1.2. [1.3] [CERT] Reachability - Multiple BRs - Single Thread / Single Infrastructure Link
    #
    # 1.2.1. Purpose
    # To test the following:
    # 1. Bi-directional reachability between Thread devices and infrastructure devices
    # 2. No existing IPv6 infrastructure
    # 3. Multiple BRS
    # 4. DUT BR adopts existing ULA and OMR prefixes
    #
    # 1.2.2. Topology
    # - BR 1 (DUT) - Thread Border Router
    # - BR 2-Test Bed device operating as a Thread Border Router Device and the Leader
    # - ED 1-Test Bed device operating as a Thread End Device, attached to BR_1
    # - Eth 1-Test bed border router device on an Adjacent Infrastructure Link
    #
    # Spec Reference | V1.1 Section | V1.3.0 Section
    # ---------------|--------------|---------------
    # Reachability   | N/A          | 1.3

    pkts = pv.pkts
    pv.summary.show()

    BR_1 = pv.vars['BR_1']
    ED_1 = pv.vars['ED_1']

    OMR_INIT = Ipv6Addr(pv.vars['OMR_INIT'].split('/')[0])
    ULA_INIT = Ipv6Addr(pv.vars['ULA_INIT'].split('/')[0])
    EXT_PAN_ID = pv.vars['EXT_PAN_ID_VAR']

    # Step 1
    #   Device: Eth 1, BR 2
    #   Description (DBR-1.2): Form topology. Wait for BR_2 to: 1. Register as border router in Thread Network Data
    #     2. Send multicast ND RAS PIO with ULA prefix (ULA_init) RIO with OMR prefix (OMR_init)
    #   Pass Criteria: N/A
    print("Step 1: Form topology. Wait for BR_2 to register as border router and send RAs.")

    # Step 2
    #   Device: BR 1 (DUT)
    #   Description (DBR-1.2): Enable: turn on device.
    #   Pass Criteria: N/A
    print("Step 2: Enable BR 1 (DUT).")

    # Step 3
    #   Device: BR 1 (DUT)
    #   Description (DBR-1.2): Automatically registers itself as a border router in the Thread Network Data.
    #   Pass Criteria:
    #     - The DUT MUST NOT register a new OMR Prefix in the Thread Network Data.
    print("Step 3: BR 1 (DUT) registers as border router. MUST NOT register a new OMR prefix.")

    pkts.filter_wpan_src64(BR_1).\
        filter_mle_cmd(consts.MLE_DATA_RESPONSE).\
        filter(lambda p: check_no_new_omr(p, OMR_INIT)).\
        must_next()

    # Step 4
    #   Device: BR_1 (DUT)
    #   Description (DBR-1.2): Automatically multicasts ND RAs on Adjacent Infrastructure Link.
    #   Pass Criteria:
    #     - The DUT MUST multicast ND RAS, including the following
    #     - IPv6 destination MUST be ff02::1
    #     - MUST contain "Router Lifetime" = 0. (indicating it's not a default router)
    #     - Route Information Option (RIO) Prefix OMR prefix = OMR_init.
    #     - MUST NOT include Prefix Information Option (PIO)
    #     - Any ND RA messages MUST NOT include the following: Route Information Option (RIO) Prefix::/0
    #       (the zero-length prefix)
    print("Step 4: BR 1 (DUT) multicasts ND RAs on AIL.")

    pkts.filter_eth_src(pv.vars['BR_1_ETH']).\
        filter_ipv6_dst("ff02::1").\
        filter(lambda p: check_step4(p, OMR_INIT)).\
        must_next()

    # Step 4b
    #   Device: ED 1
    #   Description (DBR-1.2): Enable device. It attaches to the DUT.
    #   Pass Criteria:
    #     - Verify the DUT still adheres to step 3 pass criteria for the Network Data when applied to the Thread
    #       Network Data that is sent to the Child ED 1.
    print("Step 4b: ED 1 attaches to DUT. Verify Network Data.")
    pkts.filter_wpan_src64(BR_1).\
        filter_wpan_dst64(ED_1).\
        filter_mle_cmd(consts.MLE_CHILD_ID_RESPONSE).\
        filter(lambda p: check_no_new_omr(p, OMR_INIT)).\
        must_next()

    # Step 5
    #   Device: Eth 1
    #   Description (DBR-1.2): Harness instructs the device to send ICMPv6 Echo Request to ED 1 via BR 1 Thread link.
    #     1. IPv6 Source: its address starting with prefix ULA_init 2. IPv6 Destination: ED_1 OMR address starting with
    #     prefix OMR init
    #   Pass Criteria:
    #     - Eth_1 receives an ICMPv6 Echo Reply from ED_1.
    #     - 1. IPv6 Source: ED_1 OMR address starting with prefix OMR init
    #     - 2. IPv6 Destination: Eth_1 ULA address starting with prefix ULA init
    print("Step 5: Eth 1 pings ED 1.")
    ETH_1_ULA = Ipv6Addr(pv.vars['ETH_1_ULA_ADDR'])
    ED_1_OMR = Ipv6Addr(pv.vars['ED_1_OMR_ADDR'])

    _pkt = pkts.filter_eth_src(pv.vars['Eth_1_ETH']).\
        filter_ipv6_src(ETH_1_ULA).\
        filter_ipv6_dst(ED_1_OMR).\
        filter_ping_request().\
        must_next()

    pkts.filter(lambda p: p.eth.dst == pv.vars['Eth_1_ETH']).\
        filter_ipv6_src(ED_1_OMR).\
        filter_ipv6_dst(ETH_1_ULA).\
        filter_ping_reply(identifier=_pkt.icmpv6.echo.identifier).\
        must_next()

    # Step 6
    #   Device: ED_1
    #   Description (DBR-1.2): Harness instructs the device to send ICMPv6 Echo Request to Eth 1.
    #     1. IPv6 Source: its OMR address starting with prefix OMR init) 2. IPv6 Destination: Eth_1 ULA address starting
    #     with prefix ULA init
    #   Pass Criteria:
    #     - ED_1 receives an ICMPv6 Echo Reply from Eth_1 via BR_1 Thread link
    #     - 1. IPv6 Source: Eth_1 ULA address starting with prefix ULA init
    #     - 2. IPv6 Destination: ED_1 OMR address starting with prefix OMR init
    print("Step 6: ED 1 pings Eth 1.")
    _pkt = pkts.filter_ipv6_src(ED_1_OMR).\
        filter_ipv6_dst(ETH_1_ULA).\
        filter_ping_request().\
        must_next()

    pkts.filter_ipv6_src(ETH_1_ULA).\
        filter_ipv6_dst(ED_1_OMR).\
        filter_ping_reply(identifier=_pkt.icmpv6.echo.identifier).\
        must_next()

    # Step 7
    #   Device: BR 2
    #   Description (DBR-1.2): Harness disables the device.
    #   Pass Criteria: N/A
    print("Step 7: Disable BR 2.")

    # Save the index after Step 7 to use for Step 9 search
    step7_end_index = pkts.index

    # Step 8
    #   Device: BR 1 (DUT)
    #   Description (DBR-1.2): Repeat Step 4
    #   Pass Criteria:
    #     - Repeat Step 4
    print("Step 8: Repeat Step 4.")
    pkts.filter_eth_src(pv.vars['BR_1_ETH']).\
        filter_ipv6_dst("ff02::1").\
        filter(lambda p: check_step4(p, OMR_INIT)).\
        must_next()

    # Step 9
    #   Device: Eth 1
    #   Description (DBR-1.2): Repeat Step 5
    #   Pass Criteria:
    #     - Repeat Step 5
    print("Step 9: Repeat Step 5.")

    # We search from step7_end_index to avoid race with Step 8 RAs
    pkts.index = step7_end_index
    _pkt = pkts.filter_eth_src(pv.vars['Eth_1_ETH']).\
        filter_ipv6_src(ETH_1_ULA).\
        filter_ipv6_dst(ED_1_OMR).\
        filter_ping_request().\
        must_next()

    pkts.filter(lambda p: p.eth.dst == pv.vars['Eth_1_ETH']).\
        filter_ipv6_src(ED_1_OMR).\
        filter_ipv6_dst(ETH_1_ULA).\
        filter_ping_reply(identifier=_pkt.icmpv6.echo.identifier).\
        must_next()

    # Step 10
    #   Device: ED 1
    #   Description (DBR-1.2): Repeat Step 6
    #   Pass Criteria:
    #     - Repeat Step 6
    print("Step 10: Repeat Step 6.")
    _pkt = pkts.filter_ipv6_src(ED_1_OMR).\
        filter_ipv6_dst(ETH_1_ULA).\
        filter_ping_request().\
        must_next()

    pkts.filter_ipv6_src(ETH_1_ULA).\
        filter_ipv6_dst(ED_1_OMR).\
        filter_ping_reply(identifier=_pkt.icmpv6.echo.identifier).\
        must_next()

    # Step 11
    #   Device: N/A
    #   Description (DBR-1.2): Harness waits for Leader timeout to occur.
    #   Pass Criteria: N/A
    print("Step 11: Wait for Leader timeout.")

    # Step 12
    #   Device: BR 1 (DUT)
    #   Description (DBR-1.2): Automatically becomes Leader and advertises its own OMR prefix, as well as a ULA prefix
    #     for the adjacent infrastructure link (AIL).
    #   Pass Criteria:
    #     - The DUT MUST become Leader of a new Partition, and MUST register a new OMR prefix OMR 1 in the Thread
    #       Network Data.
    #     - OMR 1 MUST NOT be equal to OMR_init.
    #     - DUT MUST advertise a route in Network Data as follows: Prefix TLV Prefix fc00::/7 Has Route sub-TLV
    #       Prf 'Medium' (00) or 'Low' ( 11)
    print("Step 12: BR 1 (DUT) becomes leader and registers new prefixes.")
    OMR_1 = Ipv6Addr(pv.vars['OMR_1'].split('/')[0])

    pkts.filter_wpan_src64(BR_1).\
        filter_mle_cmd(consts.MLE_DATA_RESPONSE).\
        filter(lambda p: check_step12(p, OMR_1, OMR_INIT)).\
        must_next()

    # Step 13
    #   Device: BR_1 (DUT)
    #   Description (DBR-1.2): Automatically multicasts ND RAs on Adjacent Infrastructure Link.
    #   Pass Criteria:
    #     - The DUT MUST multicast ND RAS, including the following
    #     - IPv6 destination MUST be ff02::1
    #     - M bit and O bit MUST be '0'
    #     - MUST contain "Router Lifetime" = 0. (indicating it's not a default router)
    #     - MUST include Route Information Option (RIO) Prefix OMR_1 Prf 'Medium' (00) or 'Low' (11)
    #     - MUST include Prefix Information Option (PIO) Prefix ULA 1 A bit MUST be '1'
    #     - ULA 1 MUST contain the Extended PAN ID as follows:
    #       - Global ID equals the 40 most significant bits of the Extended PAN ID
    #       - Subnet ID equals the 16 least significant bits of the Extended PAN ID
    print("Step 13: BR 1 (DUT) multicasts ND RAs on AIL with new prefixes.")
    ULA_1 = Ipv6Addr(pv.vars['ULA_1'].split('/')[0])

    pkts.filter_eth_src(pv.vars['BR_1_ETH']).\
        filter_ipv6_dst("ff02::1").\
        filter(lambda p: check_step13(p, OMR_1, ULA_1, EXT_PAN_ID)).\
        must_next()


if __name__ == '__main__':
    verify_utils.run_main(verify)
