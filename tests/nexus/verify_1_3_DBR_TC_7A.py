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

# Constants
ULA_PREFIX_START_BYTE = 0xfd
BR_PREFERENCE_HIGH = 1
BR_PREFERENCE_MEDIUM = 0
BR_PREFERENCE_LOW = 3
BR_FLAG_R_TRUE = 1
BR_FLAG_O_TRUE = 1
BR_FLAG_P_TRUE = 1
BR_FLAG_S_TRUE = 1
BR_FLAG_D_FALSE = 0
BR_FLAG_DP_FALSE = 0

ICMPV6_TYPE_ROUTER_SOLICITATION = 133


def check_step4(p, omr_prefix, pre_1_prefix, expect_high_pref=True):
    if p.icmpv6.type != verify_utils.ICMPV6_TYPE_ROUTER_ADVERTISEMENT:
        return False

    _, pio_prefixes = verify_utils.get_ra_prefixes(p)

    # MUST NOT contain a PIO with a ULA prefix.
    for prefix in pio_prefixes:
        if prefix[0] == ULA_PREFIX_START_BYTE:
            return False

    # MUST contain a Route Information Option (RIO) with PRE_1.
    if not verify_utils.check_ra_has_rio(p, pre_1_prefix):
        return False

    # MUST contain a Route Information Option (RIO) with OMR_1 with correct preference.
    prf_values = (BR_PREFERENCE_HIGH, BR_PREFERENCE_MEDIUM) if expect_high_pref else None
    if not verify_utils.check_ra_has_rio(p, omr_prefix, prf=prf_values):
        return False

    return True


def verify(pv):
    # 1.7(a). [1.3] [CERT] Reachability - Multiple BRs - Single Thread / Single IPv6 Infrastructure - Presence of
    #   non-OMR prefixes
    #
    # 1.7a.1. Purpose
    # To test the following:
    # 1. Bi-directional reachability between single Thread Network and infrastructure devices
    # 2. DUT BR creates own OMR prefix when existing prefixes are not usable (e.g. no SLAAC, or deprecated)
    #
    # 1.7a.2. Topology
    # 1. Eth 1-Adjacent Infrastructure Link Reference Device
    # 2. BR 1 (DUT) - Border Router
    # 3. BR 2-Border Router Reference Device
    # 4. Rtr 1-Thread Router Reference Device and Leader
    #
    # Spec Reference | V1.1 Section | V1.3.0 Section
    # ---------------|--------------|---------------
    # Reachability   | N/A          | 1.3

    pkts = pv.pkts
    pv.summary.show()

    GUA_1 = Ipv6Addr(pv.vars['GUA_1'].split('/')[0])
    PRE_1 = Ipv6Addr(pv.vars['PRE_1'].split('/')[0])
    OMR_1 = Ipv6Addr(pv.vars['OMR_1'].split('/')[0])
    OMR_1_LEN = 64

    ETH_1_GUA_ADDR = Ipv6Addr(pv.vars['ETH_1_GUA_ADDR'])
    RTR_1_OMR_ADDR = Ipv6Addr(pv.vars['RTR_1_OMR_ADDR'])

    # Step 0
    #   Device: Eth 1
    #   Description (DBR-1.7a): Enable. Harness configures Ethernet link with an on-link IPv6 GUA prefix GUA_1.
    #     Eth_1 is configured to multicast ND RAs.
    #   Pass Criteria:
    #     - N/A
    print("Step 0: Enable Eth 1 and configure GUA_1.")

    # Step 0b
    #   Device: Rtr 1
    #   Description (DBR-1.7a): Enable. Becomes Leader.
    #   Pass Criteria:
    #     - N/A
    print("Step 0b: Enable Rtr 1 and form network.")

    # Step 1
    #   Device: Eth 1, BR 2, Rtr 1
    #   Description (DBR-1.7a): Enable; connects to Rtr_1. Harness configures BR_2 Network Data with prefix PRE_1 as
    #     below. fd12:3456:abcd:1234::/64 (ULA prefix) P_slaac = false (SLAAC disabled for prefix) P_on_mesh = true
    #     P_stable = true P_preferred = true P_dhcp = false P_default = true P_preference = 01 (high) P_dp = false.
    #     Note: this looks like an OMR prefix, but due to P_slaac = 0 it is not usable by Thread Devices for
    #     connectivity. Note 1: the automatic creation of an OMR prefix as a BR would normally do, is disabled by
    #     the Harness for BR_2. Instead, the administratively configured PRE_1 is used only. Note 2: to disable the
    #     automatic OMR prefix creation as stated above, one method is (if no better method is available) to disable
    #     the BR_2 border routing using \"br disable\" OT CLI command to stop the automatic prefix creation. Then
    #     using \"netdata publish prefix\" the device could configure different PRE_1 prefixes as needed per test run.
    #     Form topology (if not already formed). Wait for BR_2 to: Register as border router in Thread Network Data
    #     with prefix PRE_1. Send multicast ND RAs on AIL.
    #   Pass Criteria:
    #     - N/A
    print("Step 1: Enable BR 2 and configure PRE_1 in Network Data.")

    # Step 2
    #   Device: BR 1 (DUT)
    #   Description (DBR-1.7a): Enable: switch on.
    #   Pass Criteria:
    #     - N/A
    print("Step 2: Enable BR 1 (DUT).")

    # Step 3
    #   Device: BR 1 (DUT)
    #   Description (DBR-1.7a): Automatically registers itself as a border router in the Thread Network Data and
    #     provides OMR prefix.
    #   Pass Criteria:
    #     - The DUT MUST register a new OMR Prefix (OMR_1) in the Thread Network Data, in a Prefix TLV.
    #     - Flags in the Border Router sub-TLV MUST be:
    #       - 1. P_preference = 11 (Low)
    #       - 2. P_default=true
    #       - 3. P_stable = true
    #       - 4. P_on_mesh=true
    #       - 5. P_preferred = true
    #       - 6. P_slaac=true
    #       - 7. P_dhcp=false
    #       - 8. P_dp=false
    #     - OMR_1 MUST be 64 bits long and start with 0xFD.
    print("Step 3: BR 1 (DUT) registers OMR_1 in Network Data.")
    if OMR_1_LEN != 64 or OMR_1[0] != ULA_PREFIX_START_BYTE:
        raise verify_utils.VerificationError(f"OMR_1 ({OMR_1}) must be 64 bits long and start with 0xFD")

    pkts.filter(lambda p: hasattr(p, 'mle') and\
                          p.mle.cmd in (consts.MLE_DATA_RESPONSE, consts.MLE_ADVERTISEMENT)).\
        filter(lambda p: verify_utils.check_nwd_prefix_flags(p,
                                                            OMR_1,
                                                            stable=1,
                                                            pref=BR_PREFERENCE_LOW,
                                                            r=BR_FLAG_R_TRUE,
                                                            o=BR_FLAG_O_TRUE,
                                                            p=BR_FLAG_P_TRUE,
                                                            s=BR_FLAG_S_TRUE,
                                                            d=BR_FLAG_D_FALSE,
                                                            dp=BR_FLAG_DP_FALSE)).\
        must_next()

    # Step 4
    #   Device: BR 1 (DUT)
    #   Description (DBR-1.7a): Automatically multicasts ND RAs on Adjacent Infrastructure Link.
    #   Pass Criteria:
    #     - The DUT MUST multicast ND RAs on the infrastructure link:
    #       - IPv6 destination MUST be ff02::1
    #       - MUST NOT contain a Prefix Information Option (PIO) with a ULA prefix.
    #       - MUST contain a Route Information Option (RIO) with OMR_1.
    #       - MUST contain a Route Information Option (RIO) with PRE_1.
    print("Step 4: BR 1 (DUT) multicasts ND RAs on AIL.")
    pkts.filter_eth_src(pv.vars['BR_1_ETH']).\
        filter_ipv6_dst("ff02::1").\
        filter(lambda p: check_step4(p, OMR_1, PRE_1)).\
        must_next()

    # Step 5
    #   Device: Eth 1
    #   Description (DBR-1.7a): Harness instructs the device to send an ICMPv6 Echo Request to Rtr 1 via BR 1 or BR 2.
    #     IPv6 Source: Eth 1 GUA IPv6 Destination: Rtr 1 OMR address
    #   Pass Criteria:
    #     - Eth 1 receives an ICMPv6 Echo Reply from Rtr_1.
    #     - IPv6 Source: Rtr 1 OMR
    #     - IPv6 Destination: Eth 1 GUA
    print("Step 5: Eth 1 pings RTR 1 OMR.")
    _pkt = pkts.filter_eth_src(pv.vars['Eth_1_ETH']).\
        filter_ipv6_src(ETH_1_GUA_ADDR).\
        filter_ipv6_dst(RTR_1_OMR_ADDR).\
        filter_ping_request().\
        must_next()

    pkts.filter(lambda p: p.eth.dst == pv.vars['Eth_1_ETH']).\
        filter_ipv6_src(RTR_1_OMR_ADDR).\
        filter_ipv6_dst(ETH_1_GUA_ADDR).\
        filter_ping_reply(identifier=_pkt.icmpv6.echo.identifier).\
        must_next()

    # Step 6
    #   Device: Rtr 1
    #   Description (DBR-1.7a): Harness instructs the device to send an ICMPv6 Echo Request to Eth_1.
    #     IPv6 Source: Rtr 1 OMR address. IPv6 Destination: Eth 1 GUA
    #   Pass Criteria:
    #     - Rtr_1 receives an ICMPv6 Echo Reply from Eth_1.
    #     - IPv6 Source: Eth_1 GUA
    #     - IPv6 Destination: Rtr 1 OMR address
    print("Step 6: RTR 1 OMR pings Eth 1 GUA.")
    _pkt = pkts.filter_ipv6_src(RTR_1_OMR_ADDR).\
        filter_ipv6_dst(ETH_1_GUA_ADDR).\
        filter_ping_request().\
        must_next()

    pkts.filter_ipv6_src(ETH_1_GUA_ADDR).\
        filter_ipv6_dst(RTR_1_OMR_ADDR).\
        filter_ping_reply(identifier=_pkt.icmpv6.echo.identifier).\
        must_next()

    # Step 7
    #   Device: BR 2
    #   Description (DBR-1.7a): Harness disables the device
    #   Pass Criteria:
    #     - N/A
    print("Step 7: Disable BR 2.")

    # Step 7a
    #   Device: N/A
    #   Description (DBR-1.7a): Harness waits -20 seconds
    #   Pass Criteria:
    #     - N/A
    print("Step 7a: Wait 20 seconds.")

    # Step 7b
    #   Device: Eth 1
    #   Description (DBR-1.7a): Harness instructs device to send an ND RS message to trigger the DUT to send a ND RA
    #     for the next step. Note: in Linux, this is done with the command rdisc6v eth0.
    #     The output of the command is captured in the test log.
    #   Pass Criteria:
    #     - N/A
    print("Step 7b: Eth 1 sends RS.")
    pkts.filter_eth_src(pv.vars['Eth_1_ETH']).\
        filter_ipv6_dst("ff02::2").\
        filter(lambda p: p.icmpv6.type == ICMPV6_TYPE_ROUTER_SOLICITATION).\
        must_next()

    # Step 8
    #   Device: BR 1 (DUT)
    #   Description (DBR-1.7a): Repeat Step 4 Note: since the prefix PRE_1 is still present in the Leader's
    #     Network Data for some time, BR 1 will continue to advertise the route for PRE_1 as indicated in Step 4
    #     criteria.
    #   Pass Criteria:
    #     - Repeat Step 4
    print("Step 8: BR 1 (DUT) repeats Step 4.")
    pkts.filter_eth_src(pv.vars['BR_1_ETH']).\
        filter_ipv6_dst("ff02::1").\
        filter(lambda p: check_step4(p, OMR_1, PRE_1)).\
        must_next()

    # Step 9
    #   Device: Eth 1
    #   Description (DBR-1.7a): Repeat Step 5
    #   Pass Criteria:
    #     - Repeat Step 5
    print("Step 9: Eth 1 repeats Step 5.")
    _pkt = pkts.filter_eth_src(pv.vars['Eth_1_ETH']).\
        filter_ipv6_src(ETH_1_GUA_ADDR).\
        filter_ipv6_dst(RTR_1_OMR_ADDR).\
        filter_ping_request().\
        must_next()

    pkts.filter(lambda p: p.eth.dst == pv.vars['Eth_1_ETH']).\
        filter_ipv6_src(RTR_1_OMR_ADDR).\
        filter_ipv6_dst(ETH_1_GUA_ADDR).\
        filter_ping_reply(identifier=_pkt.icmpv6.echo.identifier).\
        must_next()

    # Step 10
    #   Device: Rtr 1
    #   Description (DBR-1.7a): Repeat Step 6
    #   Pass Criteria:
    #     - Repeat Step 6
    print("Step 10: RTR 1 repeats Step 6.")
    _pkt = pkts.filter_ipv6_src(RTR_1_OMR_ADDR).\
        filter_ipv6_dst(ETH_1_GUA_ADDR).\
        filter_ping_request().\
        must_next()

    pkts.filter_ipv6_src(ETH_1_GUA_ADDR).\
        filter_ipv6_dst(RTR_1_OMR_ADDR).\
        filter_ping_reply(identifier=_pkt.icmpv6.echo.identifier).\
        must_next()


if __name__ == '__main__':
    verify_utils.run_main(verify)
