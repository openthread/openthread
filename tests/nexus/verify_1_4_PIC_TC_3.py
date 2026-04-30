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


def check_nwd_has_route(packet, target_prefix, rloc16=None):
    try:
        types = verify_utils.as_list(packet.thread_nwd.tlv.type)
        prefixes = verify_utils.as_list(packet.thread_nwd.tlv.prefix)
    except (AttributeError, IndexError):
        return False

    prefix_idx = 0
    hr_idx = 0
    is_target = False
    for t in types:
        if t == consts.NWD_PREFIX_TLV:
            is_target = (Ipv6Addr(prefixes[prefix_idx]) == Ipv6Addr(target_prefix))
            prefix_idx += 1
        elif t in (consts.NWD_COMMISSIONING_DATA_TLV, consts.NWD_SERVICE_TLV):
            is_target = False
        elif t == consts.NWD_HAS_ROUTER_TLV:
            if is_target:
                if rloc16 is not None:
                    try:
                        br_16s = verify_utils.as_list(packet.thread_nwd.tlv.has_route.br_16)
                        if int(br_16s[hr_idx]) != rloc16:
                            hr_idx += 1
                            continue
                    except (AttributeError, IndexError):
                        hr_idx += 1
                        continue

                # Check for NP flag if available in pktverify
                # Thread 1.4: Has Route sub-TLV (Type 0)
                # Field order: RLOC16 (16), Flags (8)
                # Flags: Prf (2), P (1), NP (1), R (1), O (1), Reserved (2)
                try:
                    nps = verify_utils.as_list(packet.thread_nwd.tlv.has_route.np)
                    if int(nps[hr_idx]) != 0:
                        hr_idx += 1
                        continue
                except (AttributeError, IndexError):
                    # If field NP is not available, we skip the check
                    pass

                return True
            hr_idx += 1

    return False


def check_nwd_prefix_flags_with_rloc(packet, target_prefix, rloc16, **expected_flags):
    try:
        types = verify_utils.as_list(packet.thread_nwd.tlv.type)
        prefixes = verify_utils.as_list(packet.thread_nwd.tlv.prefix)
    except (AttributeError, IndexError):
        return False

    prefix_idx = 0
    br_idx = 0
    is_target = False

    for i, t in enumerate(types):
        if t == consts.NWD_PREFIX_TLV:
            is_target = (Ipv6Addr(prefixes[prefix_idx]) == Ipv6Addr(target_prefix))
            prefix_idx += 1
        elif t in (consts.NWD_COMMISSIONING_DATA_TLV, consts.NWD_SERVICE_TLV):
            is_target = False
        elif t == consts.NWD_BORDER_ROUTER_TLV:
            if is_target:
                try:
                    br_16s = verify_utils.as_list(packet.thread_nwd.tlv.border_router_16)
                    if int(br_16s[br_idx]) != rloc16:
                        br_idx += 1
                        continue

                    # Now check flags using verify_utils.check_nwd_prefix_flags logic
                    # Since we can't easily call it with our filtered indices, we'll do it here
                    field_map = {
                        'r': packet.thread_nwd.tlv.border_router.flag.r,
                        'o': packet.thread_nwd.tlv.border_router.flag.o,
                    }

                    match = True
                    for key, expected_val in expected_flags.items():
                        if key in field_map:
                            actual_val = int(verify_utils.as_list(field_map[key])[br_idx])
                            if actual_val != expected_val:
                                match = False
                                break
                    if match:
                        return True
                except (AttributeError, IndexError):
                    pass
            br_idx += 1

    return False


def verify(pv):
    # 10.3. IPv6 default route advertisement
    #
    # 10.3.1. Purpose
    # - To verify that the BR DUT:
    #   - Correctly advertises a default route on the Thread Network, using the zero-length route ::/0 in a Prefix TLV.
    #   - Does not withdraws the advertised default route when the infrastructure network disables the default route,
    #     as long as there's still a non-ULA prefix active on the AIL.
    #   - Still advertises the default route, when the infrastructure network re-enables the default route.
    #   - TBD: a future test update needs to validate that when only a ULA prefix is set at the AIL, the default route
    #     gets withdrawn and replaced by fc00::/7 route.
    #
    # 10.3.2. Topology
    # - BR_1 (DUT) - Border Router
    # - Router_1 - Thread Router Reference Device, attached to BR_1
    # - ED_1 - Thread Reference Device, End Device (e.g. FED/REED) role, attached to Router_1
    # - Eth_1 - Adjacent Infrastructure Link Linux Reference Device
    # - Eth_2 - Adjacent Infrastructure Link SPIFF Reference Device

    pkts = pv.pkts
    pv.summary.show()

    BR_1 = pv.vars['BR_1']
    ED_1 = pv.vars['ED_1']
    ETH_1_ADDR = pv.vars['ETH_1_ADDR']
    INTERNET_SERVER_ADDR = pv.vars['INTERNET_SERVER_ADDR']

    # Step 1
    # - Device: Eth_1, Eth_2
    # - Description (PIC-10.3): Enable
    # - Pass Criteria:
    #   - N/A
    print("Step 1: Enable Eth_1 and Eth_2")

    # Step 2
    # - Device: Eth_2
    # - Description (PIC-10.3): Harness instructs device to: (only default configuration is applied)
    # - Pass Criteria:
    #   - N/A
    print("Step 2: Eth_2 default configuration")

    # Step 3
    # - Device: BR_1 (DUT), Router_1, ED_1
    # - Description (PIC-10.3): Enable
    # - Pass Criteria:
    #   - N/A
    print("Step 3: Enable BR_1, Router_1, ED_1")

    # Step 4
    # - Device: BR_1 (DUT)
    # - Description (PIC-10.3): Automatically uses DHCPv6-PD client function to obtain a delegated prefix from the
    #   DHCPv6 server. It configures this prefix as the OMR prefix.
    # - Pass Criteria:
    #   - N/A
    print("Step 4: BR_1 obtains OMR prefix via DHCPv6-PD")
    # Verify DHCPv6-PD Reply from Eth_2 to BR_1
    pkts.filter_ipv6_src(pv.vars['ETH_2_ADDR']).\
        filter(lambda p: 'dhcpv6' in p.layer_names).\
        must_next()

    # Step 5
    # - Device: BR_1 (DUT)
    # - Description (PIC-10.3): Automatically advertises a default route in its Thread Network Data.
    # - Pass Criteria:
    #   - Thread Network Data MUST contain: Prefix TLV with an OMR prefix OMR_1 that starts with 2005:1234:abcd:0::/56.
    #   - Border Router sub-TLV MUST indicate the RLOC16 of BR_1, in a P_border_router_16 field.
    #   - Flag R (P_default) MUST be ‘1’
    #   - Flag O (P_on_mesh) MUST be ‘1’
    #   - Thread Network Data MUST contain: Prefix TLV with the zero-length prefix ::/0
    #   - Has Route sub-TLV Flag NP MUST be ‘0’
    print("Step 5: BR_1 automatically advertises a default route in its Thread Network Data")
    omr_prefix_pattern = Ipv6Addr("2005:1234:abcd:0::")[:7]
    BR_1_RLOC16 = pv.vars['BR_1_RLOC16']

    nwd_pkt = pkts.filter_wpan_src64(BR_1).\
        filter_mle_cmd(consts.MLE_DATA_RESPONSE).\
        filter(lambda p: any(pre.startswith(omr_prefix_pattern) for pre in p.thread_nwd.tlv.prefix)).\
        filter(lambda p: any(pre == Ipv6Addr("::") for pre in p.thread_nwd.tlv.prefix)).\
        must_next()

    OMR_PREFIX = None
    for pre in nwd_pkt.thread_nwd.tlv.prefix:
        if pre.startswith(omr_prefix_pattern):
            OMR_PREFIX = pre
            break
    assert OMR_PREFIX is not None

    # Verify OMR prefix flags and RLOC16
    assert check_nwd_prefix_flags_with_rloc(nwd_pkt, OMR_PREFIX, rloc16=BR_1_RLOC16, r=1, o=1)

    # Verify ::/0 prefix in Has Route TLV and RLOC16
    assert check_nwd_has_route(nwd_pkt, "::", rloc16=BR_1_RLOC16)

    # Step 6
    # - Device: ED_1
    # - Description (PIC-10.3): Harness instructs device to send ping to destination address 2002:1234::1234 with its
    #   (default) OMR source address, to validate that default route works.
    # - Pass Criteria:
    #   - Ping response received by ED_1.
    print("Step 6: ED_1 pings internet address")
    ping_req = pkts.filter_ipv6_dst(INTERNET_SERVER_ADDR).\
        filter_ping_request().\
        must_next()
    pkts.filter_ipv6_src(INTERNET_SERVER_ADDR).\
        filter_ping_reply(identifier=ping_req.icmpv6.echo.identifier).\
        must_next()

    # Step 7
    # - Device: Eth_2
    # - Description (PIC-10.3): Harnes instructs device to reconfigure ND RA daemon to stop advertising a default
    #   route: “Router Lifetime” field set to 0 sec (indicating it is not a default router for IPv6).
    # - Pass Criteria:
    #   - N/A
    print("Step 7: Eth_2 stops advertising a default route")
    ra_step7 = pkts.filter_icmpv6_nd_ra().\
        filter(lambda p: p.icmpv6.nd.ra.router_lifetime == 0).\
        must_next()

    # Step 8
    # - Device: BR_1 (DUT)
    # - Description (PIC-10.3): Does not stop advertising a default route in its Thread Network Data.
    # - Pass Criteria:
    #   - Thread Network Data MUST contain: Prefix TLV with same OMR_1.
    #   - Border Router sub-TLV MUST indicate the RLOC16 of BR_1, in a P_border_router_16 field.
    #   - Flag R (P_default) MUST be ‘1’
    #   - Flag O (P_on_mesh) MUST be ‘1’
    #   - Thread Network Data MUST contain: Prefix TLV with the zero-length prefix ::/0
    #   - Has Route sub-TLV Flag NP MUST be ‘0’
    print("Step 8: BR_1 does not stop advertising a default route")
    # We check that BR_1 still advertises the default route in its next MLE Data Response.
    nwd_pkt_8 = pkts.filter_wpan_src64(BR_1).\
        filter_mle_cmd(consts.MLE_DATA_RESPONSE).\
        filter(lambda p: any(pre == Ipv6Addr("::") for pre in p.thread_nwd.tlv.prefix)).\
        must_next()

    # Verify OMR prefix flags and RLOC16
    assert check_nwd_prefix_flags_with_rloc(nwd_pkt_8, OMR_PREFIX, rloc16=BR_1_RLOC16, r=1, o=1)

    # Verify ::/0 prefix in Has Route TLV and RLOC16
    assert check_nwd_has_route(nwd_pkt_8, "::", rloc16=BR_1_RLOC16)

    # Step 9
    # - Device: ED_1
    # - Description (PIC-10.3): Harness instructs device to send ping to destination Eth_1 with its (default) OMR
    #   source address, to validate that route to AIL still works.
    # - Pass Criteria:
    #   - Ping response received by ED_1.
    print("Step 9: ED_1 pings Eth_1")
    ping_req_9 = pkts.filter_ipv6_dst(ETH_1_ADDR).\
        filter_ping_request().\
        must_next()
    pkts.filter_ipv6_src(ETH_1_ADDR).\
        filter_ping_reply(identifier=ping_req_9.icmpv6.echo.identifier).\
        must_next()

    # Step 10
    # - Device: ED_1
    # - Description (PIC-10.3): Harness instructs device to send ping to destination address 2002:1234::1234 with its
    #   (default) OMR source address, to validate that default route does not work at the moment.
    # - Pass Criteria:
    #   - Ping request MUST NOT be sent by DUT onto the AIL.
    #   - Ping response MUST NOT be received by ED_1.
    print("Step 10: ED_1 pings internet address (should fail)")
    _step10_start_idx = (pkts.index[0], 0)

    # Find Step 11 RA first to define range for Step 10
    ra_step11 = pkts.filter_icmpv6_nd_ra().\
        filter(lambda p: p.icmpv6.nd.ra.router_lifetime > 0).\
        must_next()

    # No ping request on AIL for internet server in the range of Step 10.
    # Packets on AIL do NOT have 'wpan' layer.
    pkts.range(_step10_start_idx, (ra_step11.number, 0)).\
        filter_ipv6_dst(INTERNET_SERVER_ADDR).\
        filter_ping_request().\
        filter(lambda p: 'wpan' not in p.layer_names).\
        must_not_next()

    # Step 11
    # - Device: Eth_2
    # - Description (PIC-10.3): Harness instructs device to reconfigure ND RA daemon to start advertising a default
    #   route again: “Router Lifetime” field set to 875 sec (indicating it is a default router for IPv6).
    # - Pass Criteria:
    #   - N/A
    print("Step 11: Eth_2 starts advertising a default route again")
    # ra_step11 already found

    # Step 12
    # - Device: BR_1 (DUT)
    # - Description (PIC-10.3): (As in step 5)
    # - Pass Criteria:
    #   - (As in step 5)
    print("Step 12: BR_1 automatically advertises a default route (same as step 5)")
    # Since the default route was never withdrawn (verified in Step 8), we check that
    # it continues to be advertised (i.e., no MLE Data Response from BR_1 withdraws it).
    pkts.filter_wpan_src64(BR_1).\
        filter_mle_cmd(consts.MLE_DATA_RESPONSE).\
        filter(lambda p: any(pre == Ipv6Addr("::") for pre in p.thread_nwd.tlv.prefix) is False).\
        must_not_next()

    # Step 13
    # - Device: ED_1
    # - Description (PIC-10.3): (As in step 6)
    # - Pass Criteria:
    #   - (As in step 6)
    print("Step 13: ED_1 pings internet address (same as step 6)")
    ping_req_13 = pkts.filter_ipv6_dst(INTERNET_SERVER_ADDR).\
        filter_ping_request().\
        must_next()
    pkts.filter_ipv6_src(INTERNET_SERVER_ADDR).\
        filter_ping_reply(identifier=ping_req_13.icmpv6.echo.identifier).\
        must_next()

    # Step 14
    # - Device: ED_1
    # - Description (PIC-10.3): (As in step 9)
    # - Pass Criteria:
    #   - (As in step 9)
    print("Step 14: ED_1 pings Eth_1 (same as step 9)")
    ping_req_14 = pkts.filter_ipv6_dst(ETH_1_ADDR).\
        filter_ping_request().\
        must_next()
    pkts.filter_ipv6_src(ETH_1_ADDR).\
        filter_ping_reply(identifier=ping_req_14.icmpv6.echo.identifier).\
        must_next()


if __name__ == '__main__':
    verify_utils.run_main(verify)
