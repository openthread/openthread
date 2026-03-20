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
BR_FLAG_S_TRUE = 1
BR_FLAG_D_FALSE = 0
BR_FLAG_DP_FALSE = 0

# Step 4 RA constants
ICMPV6_TYPE_ROUTER_ADVERTISEMENT = 134
RA_FLAG_M_FALSE = 0
RA_FLAG_O_FALSE = 0
RA_ROUTER_LIFETIME_ZERO = 0

ICMPV6_OPT_TYPE_PIO = 3
ICMPV6_OPT_TYPE_RIO = 24

PIO_FLAG_A_TRUE = 1

# EXT_PAN_ID mapping offsets and lengths
EXT_PAN_ID_GLOBAL_ID_OFFSET = 1
EXT_PAN_ID_GLOBAL_ID_LEN = 5
EXT_PAN_ID_SUBNET_ID_OFFSET = 6
EXT_PAN_ID_SUBNET_ID_LEN = 2


def verify(pv):
    # 1.1. [1.3] [CERT] Reachability - Single BR / Single Infrastructure Link
    #
    # 1.1.1. Purpose
    # To test the following situation
    # 1. Bi-directional reachability between Thread devices and infrastructure devices
    # 2. No existing IPv6 infrastructure
    # 3. Single BR
    # 4. Verify that the BR DUT does not forward IPv6 packets that must not be forwarded (e.g. link-local)
    # 5. Verify that the BR DUT sends the right ND RA messages on AIL, and includes the right prefixes in Network Data
    #
    # 1.1.2. Topology
    # - BR 1 (DUT) - Thread Border Router and Leader
    # - ED 1 - Test bed device operating as a Thread End Device
    # - Eth 1 - Test bed border router device on an Adjacent Infrastructure Link
    #
    # Spec Reference | V1.3.0 Section
    # ---------------|---------------
    # Reachability   | 1.3

    pkts = pv.pkts
    pv.summary.show()

    BR_1 = pv.vars['BR_1']
    ED_1 = pv.vars['ED_1']
    Eth_1 = pv.vars['Eth_1']

    OMR_PREFIX = Ipv6Addr(pv.vars['OMR_PREFIX'].split('/')[0])
    OMR_PREFIX_LEN = int(pv.vars['OMR_PREFIX'].split('/')[1])
    PRE_1_PREFIX = Ipv6Addr(pv.vars['PRE_1_PREFIX'].split('/')[0])
    PRE_1_PREFIX_LEN = int(pv.vars['PRE_1_PREFIX'].split('/')[1])
    EXT_PAN_ID = pv.vars['EXT_PAN_ID_VAR']

    ETH_1_ULA = Ipv6Addr(pv.vars['ETH_1_ULA_ADDR'])
    ED_1_OMR = Ipv6Addr(pv.vars['ED_1_OMR_ADDR'])
    ED_1_MLEID = Ipv6Addr(pv.vars['ED_1_MLEID_ADDR'])

    def as_list(x):
        return x if isinstance(x, list) else [x]

    # Step 1
    #   Device: Eth 1, ED 1
    #   Description (DBR-1.1): Enable.
    #   Pass Criteria: N/A
    print("Step 1: Enable Eth 1, ED 1.")

    # Step 2
    #   Device: BR 1 (DUT)
    #   Description (DBR-1.1): Enable.
    #   Pass Criteria: N/A
    print("Step 2: Enable BR 1 (DUT).")

    # Step 3
    #   Device: BR 1 (DUT)
    #   Description (DBR-1.1): Automatically registers itself as a Border Router in the Thread Network Data.
    #     The DUT: adds an OMR prefix OMR_1 to its Thread Network Data; adds an on-link ULA prefix PRE_1 on the
    #     adjacent infrastructure link (AIL); adds an external route in the Thread Network Data based on PRE_1.
    #   Pass Criteria:
    #     - Note: pass criteria are identical to DBR-1.3 step 2.
    #     - The DUT internally registers an OMR Prefix OMR_1 in the Thread Network Data.
    #     - The DUT MUST send a multicast MLE Data Response with Thread Network Data containing at least two
    #       Prefix TLVs:
    #     - 1. Prefix TLV: OMR prefix OMR_1.
    #       - MUST include a Border Router sub-TLV. Flags in the Border Router TLV MUST be:
    #         - P_preference = 11 (Low)
    #         - P_default=true (Note: OpenThread currently sets this to false if no infra default route)
    #         - P_stable=true
    #         - P_on_mesh=true
    #         - P_preferred=true
    #         - P_slaac = true
    #         - P_dhcp=false
    #         - P_dp=false
    #     - 2. Prefix TLV: ULA prefix fc00::/7. (This is used as the shortened version of PRE_1)
    #       - Includes Has Route sub-TLV
    #     - OMR 1 MUST be 64 bits long and start with 0xFD.
    print("Step 3: BR 1 (DUT) registers itself as a Border Router.")

    def check_step3(p):
        try:
            prefixes = as_list(p.thread_nwd.tlv.prefix)
            types = as_list(p.thread_nwd.tlv.type)
            stables = as_list(p.thread_nwd.tlv.stable)
        except AttributeError:
            return False

        # 1. Prefix TLV for OMR_1
        try:
            omr_idx = prefixes.index(OMR_PREFIX)
        except ValueError:
            return False

        # Check OMR prefix properties: 64 bits long and starts with ULA_PREFIX_START_BYTE
        if OMR_PREFIX_LEN != 64 or OMR_PREFIX[0] != ULA_PREFIX_START_BYTE:
            return False

        # Find the NWD_PREFIX_TLV entry index corresponding to omr_idx
        prefix_indices = [i for i, t in enumerate(types) if t == consts.NWD_PREFIX_TLV]
        if omr_idx >= len(prefix_indices):
            return False

        t_idx = prefix_indices[omr_idx]
        if stables[t_idx] != 1:
            return False

        # Flags for OMR prefix
        # Note: We expect P_default (r flag) to be 0 for now to match OpenThread behavior
        try:
            if not (as_list(p.thread_nwd.tlv.border_router.pref)[0] == BR_PREFERENCE_LOW and \
                    as_list(p.thread_nwd.tlv.border_router.flag.r)[0] == BR_FLAG_R_FALSE and \
                    as_list(p.thread_nwd.tlv.border_router.flag.o)[0] == BR_FLAG_O_TRUE and \
                    as_list(p.thread_nwd.tlv.border_router.flag.p)[0] == BR_FLAG_P_TRUE and \
                    as_list(p.thread_nwd.tlv.border_router.flag.s)[0] == BR_FLAG_S_TRUE and \
                    as_list(p.thread_nwd.tlv.border_router.flag.d)[0] == BR_FLAG_D_FALSE and \
                    as_list(p.thread_nwd.tlv.border_router.flag.dp)[0] == BR_FLAG_DP_FALSE):
                return False
        except AttributeError:
            return False

        # 2. Prefix TLV for fc00::/7 with Has Route sub-TLV
        try:
            ula_idx = prefixes.index(Ipv6Addr("fc00::"))
        except ValueError:
            return False

        if not hasattr(p.thread_nwd.tlv, 'has_route'):
            return False

        return True

    pkts.filter_wpan_src64(BR_1).\
        filter_mle_cmd(consts.MLE_DATA_RESPONSE).\
        filter(check_step3).\
        must_next()

    # Step 4
    #   Device: BR 1 (DUT)
    #   Description (DBR-1.1): Automatically multicasts ND RAs on Adjacent Infrastructure Link.
    #   Pass Criteria:
    #     - The DUT MUST multicast ND RAs.
    #     - IPv6 destination MUST be ff02::1
    #     - M bit and O bit MUST be '0'
    #     - MUST contain "Router Lifetime" = 0. (indicating it's not a default router)
    #     - MUST contain a Prefix Information Option (PIO) with a ULA prefix PRE_1.
    #       - A bit MUST be '1'
    #       - L bit SHOULD be '1'
    #       - Preferred Lifetime MUST be non-zero
    #       - Valid Lifetime MUST be non-zero
    #     - MUST contain a Route Information Option (RIO) with the OMR prefix OMR 1.
    #     - PRE_1 MUST be 64 bits long and start with 0xFD and MUST differ from OMR_1.
    #     - PRE_1 MUST contain the Extended PAN ID as follows:
    #       - Global ID equals the 40 most significant bits of the Extended PAN ID
    #       - Subnet ID equals the 16 least significant bits of the Extended PAN ID
    print("Step 4: BR 1 (DUT) multicasts ND RA on AIL.")

    def check_step4(p):
        if p.icmpv6.type != ICMPV6_TYPE_ROUTER_ADVERTISEMENT:
            return False
        if p.icmpv6.nd.ra.flag.m != RA_FLAG_M_FALSE or p.icmpv6.nd.ra.flag.o != RA_FLAG_O_FALSE:
            return False
        if p.icmpv6.nd.ra.router_lifetime != RA_ROUTER_LIFETIME_ZERO:
            return False

        # Check PIO (type ICMPV6_OPT_TYPE_PIO) and RIO (type ICMPV6_OPT_TYPE_RIO)
        opts = as_list(p.icmpv6.opt.type)
        if ICMPV6_OPT_TYPE_PIO not in opts or ICMPV6_OPT_TYPE_RIO not in opts:
            return False

        # Find PRE_1 in PIO
        try:
            pio_idx = as_list(p.icmpv6.opt.type).index(ICMPV6_OPT_TYPE_PIO)
        except ValueError:
            return False

        pre1 = Ipv6Addr(p.icmpv6.opt.prefix[pio_idx])
        if pre1 != PRE_1_PREFIX:
            return False

        # Check PRE_1 properties: starts with ULA_PREFIX_START_BYTE, differs from OMR_1
        if PRE_1_PREFIX_LEN != 64 or pre1[0] != ULA_PREFIX_START_BYTE:
            return False
        if pre1 == OMR_PREFIX:
            return False

        # Check PIO A bit
        if as_list(p.icmpv6.opt.pio_flag.a)[0] != PIO_FLAG_A_TRUE:
            return False

        # Check RIO contains OMR_1
        if OMR_PREFIX not in [Ipv6Addr(prefix) for prefix in p.icmpv6.opt.prefix]:
            return False

        # Check EXT_PAN_ID mapping in PRE_1
        ext_pan_id_bytes = bytes.fromhex(EXT_PAN_ID)
        # Global ID equals the 40 most significant bits of the Extended PAN ID
        if pre1[EXT_PAN_ID_GLOBAL_ID_OFFSET:EXT_PAN_ID_GLOBAL_ID_OFFSET + EXT_PAN_ID_GLOBAL_ID_LEN] != \
                ext_pan_id_bytes[:EXT_PAN_ID_GLOBAL_ID_LEN]:
            return False
        # Subnet ID equals the 16 least significant bits of the Extended PAN ID
        if pre1[EXT_PAN_ID_SUBNET_ID_OFFSET:EXT_PAN_ID_SUBNET_ID_OFFSET + EXT_PAN_ID_SUBNET_ID_LEN] != \
                ext_pan_id_bytes[EXT_PAN_ID_SUBNET_ID_OFFSET:EXT_PAN_ID_SUBNET_ID_OFFSET + EXT_PAN_ID_SUBNET_ID_LEN]:
            return False

        return True

    pkts.filter_eth_src(pv.vars['BR_1_ETH']).\
        filter_ipv6_dst("ff02::1").\
        filter(check_step4).\
        must_next()

    # Step 5
    #   Device: Eth 1
    #   Description (DBR-1.1): Harness instructs device to send ICMPv6 Echo Request to ED 1.
    #     1. IPv6 Source: Eth_1 ULA 2. IPv6 Destination: ED_1 OMR address
    #   Pass Criteria:
    #     - Eth 1 receives an ICMPv6 Echo Reply from ED_1.
    #     - 1. IPv6 Source: ED_1 OMR address
    #     - 2. IPv6 Destination: Eth_1 ULA
    print("Step 5: Eth 1 pings ED 1 OMR.")
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
    #   Device: ED 1
    #   Description (DBR-1.1): Harness instructs device to send ICMPv6 Echo Request to Eth 1.
    #     1. IPv6 Source: ED_1 OMR address 2. IPv6 Destination: Eth_1 ULA
    #   Pass Criteria:
    #     - ED 1 receives an ICMPv6 Echo Reply from Eth 1.
    #     - 1. IPv6 Source: Eth_1 ULA
    #     - 2. IPv6 Destination: ED_1 OMR address
    print("Step 6: ED 1 OMR pings Eth 1 ULA.")
    _pkt = pkts.filter_ipv6_src(ED_1_OMR).\
        filter_ipv6_dst(ETH_1_ULA).\
        filter_ping_request().\
        must_next()

    pkts.filter_ipv6_src(ETH_1_ULA).\
        filter_ipv6_dst(ED_1_OMR).\
        filter_ping_reply(identifier=_pkt.icmpv6.echo.identifier).\
        must_next()

    # Step 7
    #   Device: Eth 1
    #   Description (DBR-1.1): Harness instructs device to send ICMPv6 Echo Request to ED_1.
    #     1. IPv6 Source: Eth_1 link-local 2. IPv6 Destination: ED 1 OMR address
    #   Pass Criteria:
    #     - BR_1 (DUT) MUST NOT forward the ICMPv6 Echo Request to the Thread network.
    print("Step 7: Eth 1 link-local pings ED 1 OMR (must not forward).")
    pkts.filter_eth_src(pv.vars['Eth_1_ETH']).\
        filter_ipv6_src(Ipv6Addr(pv.vars['ETH_1_LL_ADDR'])).\
        filter_ipv6_dst(ED_1_OMR).\
        filter_ping_request().\
        must_next()

    pkts.filter_wpan_src64(BR_1).\
        filter_ipv6_src(Ipv6Addr(pv.vars['ETH_1_LL_ADDR'])).\
        filter_ipv6_dst(ED_1_OMR).\
        filter_ping_request().\
        must_not_next()

    # Step 8
    #   Device: ED 1
    #   Description (DBR-1.1): Harness instructs device to send ICMPv6 Echo Request to Eth 1.
    #     1. IPv6 Source: ED 1 link-local 2. IPv6 Destination: Eth_1 ULA
    #   Pass Criteria:
    #     - BR_1 (DUT) MUST NOT forward the ICMPv6 Echo Request to the Adjacent Infrastructure Network.
    print("Step 8: ED 1 link-local pings Eth 1 ULA (must not forward).")
    pkts.filter_eth_src(pv.vars['BR_1_ETH']).\
        filter_ipv6_src(Ipv6Addr(pv.vars['ED_1_LL_ADDR'])).\
        filter_ipv6_dst(ETH_1_ULA).\
        filter_ping_request().\
        must_not_next()

    # Step 9
    #   Device: Eth 1
    #   Description (DBR-1.1): Harness instructs device to add a route to the Thread Network's mesh-local prefix
    #     via the infrastructure link. Harness then instructs device to send ICMPv6 Echo Request to ED 1.
    #     1. IPv6 Source: Eth 1 ULA 2. IPv6 Destination: ED_1-ML-EID
    #   Pass Criteria:
    #     - BR_1 (DUT) MUST NOT forward the ICMPv6 Echo Request to the Thread network.
    print("Step 9: Eth 1 pings ED 1 ML-EID (must not forward).")
    pkts.filter_eth_src(pv.vars['Eth_1_ETH']).\
        filter_ipv6_src(ETH_1_ULA).\
        filter_ipv6_dst(ED_1_MLEID).\
        filter_ping_request().\
        must_next()

    pkts.filter_wpan_src64(BR_1).\
        filter_ipv6_src(ETH_1_ULA).\
        filter_ipv6_dst(ED_1_MLEID).\
        filter_ping_request().\
        must_not_next()

    # Step 10
    #   Device: ED 1
    #   Description (DBR-1.1): Harness instructs device to send ICMPv6 Echo Request to Eth 1.
    #     1. IPv6 Source: ED 1 ML-EID 2. IPv6 Destination: Eth 1 ULA
    #   Pass Criteria:
    #     - BR_1 (DUT) MUST NOT forward the ICMPv6 Echo Request to the Adjacent Infrastructure Network.
    print("Step 10: ED 1 ML-EID pings Eth 1 ULA (must not forward).")
    pkts.filter_eth_src(pv.vars['BR_1_ETH']).\
        filter_ipv6_src(ED_1_MLEID).\
        filter_ipv6_dst(ETH_1_ULA).\
        filter_ping_request().\
        must_not_next()


if __name__ == '__main__':
    verify_utils.run_main(verify)
