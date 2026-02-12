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
    # 5.3.8 MTD Child Address Set
    #
    # 5.3.8.1 Topology
    # - Leader (DUT)
    # - Border Router
    # - MED_1
    # - MED_2
    #
    # 5.3.8.2 Purpose & Description
    # The purpose of this test case is to validate that the DUT MTD Child Address Set can hold at least 4 IPv6
    #   non-link-local addresses.
    #
    # Spec Reference          | V1.1 Section | V1.3.0 Section
    # ------------------------|--------------|---------------
    # MTD Child Address Set   | 5.4.1.2      | 5.4.1.2

    pkts = pv.pkts
    pv.summary.show()

    LEADER = pv.vars['LEADER']
    MED_1 = pv.vars['MED_1']
    MED_2 = pv.vars['MED_2']

    # Find GUA addresses of MED_2
    def get_node_id_by_name(name):
        for node_id, info in pv.test_info.topology.items():
            if info['name'] == name:
                return node_id
        return None

    MED_2_ID = get_node_id_by_name('MED_2')
    med2_addrs = [str(Ipv6Addr(addr)) for addr in pv.test_info.ipaddrs[MED_2_ID]]
    MED_2_GUA1 = next(addr for addr in med2_addrs if addr.startswith('2001:'))
    MED_2_GUA2 = next(addr for addr in med2_addrs if addr.startswith('2002:'))
    MED_2_GUA3 = next(addr for addr in med2_addrs if addr.startswith('2003:'))

    # Step 1: Border Router
    # - Description: Harness configures the device to be a DHCPv6 server for prefixes 2001:: & 2002:: & 2003::.
    # - Pass Criteria: N/A
    print("Step 1: Border Router")

    # Step 2: Leader (DUT)
    # - Description: Automatically transmits MLE advertisements.
    # - Pass Criteria:
    #   - The DUT MUST send properly formatted MLE Advertisements.
    #   - MLE Advertisements MUST be sent with an IP Hop Limit of 255 to the Link-Local All Nodes multicast address
    #     (FF02::1).
    #   - The following TLVs MUST be present in the MLE Advertisements,:
    #     - Leader Data TLV
    #     - Route64 TLV
    #     - Source Address TLV
    print("Step 2: Leader (DUT)")
    pkts.filter_mle_cmd(consts.MLE_ADVERTISEMENT).\
        filter_LLANMA().\
        filter_wpan_src64(LEADER).\
        filter(lambda p: {
                          consts.LEADER_DATA_TLV,
                          consts.ROUTE64_TLV,
                          consts.SOURCE_ADDRESS_TLV
                          } <= set(p.mle.tlv.type) and\
               p.ipv6.hlim == 255).\
        must_next()

    # Step 3: MED_1 and MED_2
    # - Description: Harness attaches end devices.
    # - Pass Criteria: N/A
    print("Step 3: MED_1 and MED_2")

    # Step 4: MED_1
    # - Description: Harness instructs device to send an ICMPv6 Echo Request to the MED_2 ML-EID.
    # - Pass Criteria:
    #   - The DUT MUST NOT send an Address Query Request.
    #   - MED_2 MUST respond with an ICMPv6 Echo Reply.
    print("Step 4: MED_1")
    _verify_echo(pv, MED_1, MED_2, pv.vars['MED_2_MLEID'])

    # Step 5: MED_1
    # - Description: Harness instructs device to send an ICMPv6 Echo Request to the MED_2 2001:: GUA.
    # - Pass Criteria:
    #   - The DUT MUST NOT send an Address Query Request.
    #   - MED_2 MUST respond with an ICMPv6 Echo Reply.
    print("Step 5: MED_1")
    _verify_echo(pv, MED_1, MED_2, MED_2_GUA1)

    # Step 6: MED_1
    # - Description: Harness instructs device to send an ICMPv6 Echo Request to the MED_2 2002:: GUA.
    # - Pass Criteria:
    #   - The DUT MUST NOT send an Address Query Request.
    #   - MED_2 MUST respond with an ICMPv6 Echo Reply.
    print("Step 6: MED_1")
    _verify_echo(pv, MED_1, MED_2, MED_2_GUA2)

    # Step 7: MED_1
    # - Description: Harness instructs device to send an ICMPv6 Echo Request to the MED_2 2003:: GUA.
    # - Pass Criteria:
    #   - The DUT MUST NOT send an Address Query Request.
    #   - MED_2 MUST respond with an ICMPv6 Echo Reply.
    print("Step 7: MED_1")
    _verify_echo(pv, MED_1, MED_2, MED_2_GUA3)


def _verify_echo(pv, src_ext, dst_ext, target_ip):
    pkts = pv.pkts
    DUT_RLOC = pv.vars['LEADER_RLOC']
    start_index = pkts.index

    # Find Echo Reply from MED_2 to MED_1
    # We use a suffix match for the IP address to be robust against potential 6LoWPAN decompression issues
    # where the prefix might be lost but the IID is preserved.
    target_ip_addr = Ipv6Addr(target_ip)
    target_iid_bytes = target_ip_addr[8:]
    pkts.filter_wpan_src64(dst_ext).\
        filter(lambda p: p.icmpv6.type == consts.ICMPV6_TYPE_ECHO_REPLY and\
                         (p.ipv6.src == target_ip_addr or p.ipv6.src[8:] == target_iid_bytes)).\
        must_next()
    end_index = pkts.index

    # Verify no Address Query from DUT (Leader) for the target IP in this range
    pkts.range(start_index, end_index).\
        filter_wpan_src64(pv.vars['LEADER']).\
        filter(lambda p: p.ipv6.src == DUT_RLOC).\
        filter_coap_request(consts.ADDR_QRY_URI).\
        filter(lambda p: Ipv6Addr(p.coap.tlv.target_eid) == target_ip_addr or\
                         Ipv6Addr(p.coap.tlv.target_eid)[8:] == target_iid_bytes).\
        must_not_next()


if __name__ == '__main__':
    verify_utils.run_main(verify)
