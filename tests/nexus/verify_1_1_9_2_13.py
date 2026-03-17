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

PRIMARY_CHANNEL = 11
SECONDARY_CHANNEL = 12


def verify(pv):
    # 9.2.13 Energy Scan Requests
    #
    # 9.2.13.1 Topology
    # - Two sniffers are required to run this test case!
    # - Leader_2 & SED_2 formed a separate network on another channel (Secondary channel).
    # - SED_2 is configured to use a short Poll Period (e.g. 500ms).
    # - Router_1 & FED_1 & Commissioner joined the network on the Primary channel.
    # - Commissioner & Leader_1 & Router_1 are on the same Primary channel.
    #
    # 9.2.13.2 Purpose & Description
    # - The purpose of this test case is to verify the behavior of the DUT when receiving MGMT_ED_SCAN.qry.
    #
    # Spec Reference                          | V1.1 Section | V1.3.0 Section
    # ----------------------------------------|--------------|---------------
    # Collecting Energy Scan Information      | 8.7.10       | 8.7.10

    pkts = pv.pkts
    pv.summary.show()

    # Step 1: DUT
    # - Description: Ensure topology is formed correctly.
    # - Pass Criteria: N/A.
    print("Step 1: DUT")

    # Step 2: Commissioner
    # - Description: Commissioner sends unicast MGMT_ED_SCAN.qry to the DUT.
    # - Pass Criteria: Commissioner MUST send MGMT_ED_SCAN.qry to the DUT with the following format:
    #   - CoAP Request URI: coap://[DUT]:MM/c/es
    #   - CoAP Payload:
    #     - Commissioner Session ID TLV (valid)
    #     - Channel Mask TLV (Primary and Secondary)
    #     - Count TLV <0x02>
    #     - Period TLV <0x00c8> (200 ms)
    #     - Scan Duration TLV <0x20> (32 ms)
    print("Step 2: Commissioner sends unicast MGMT_ED_SCAN.qry to the DUT.")
    mask = (1 << (31 - PRIMARY_CHANNEL)) | (1 << (31 - SECONDARY_CHANNEL))
    expected_channel_mask = '0004' + f'{mask:08x}'
    pkts.filter(lambda p: p.wpan.frame_type == consts.WPAN_DATA and\
                p.wpan.src16 == pv.vars['Commissioner_RLOC16'] and\
                p.wpan.dst16 != 0xffff and\
                p.coap.uri_path == consts.MGMT_ED_SCAN).\
        must_next().\
        must_verify(lambda p: p.coap.tlv.channel_mask == expected_channel_mask and\
                               p.coap.tlv.count == 2 and\
                               p.coap.tlv.period == 200 and\
                               p.coap.tlv.scan_duration == 32)

    # Step 3: DUT
    # - Description: Automatically sends a MGMT_ED_REPORT.ans response to the Commissioner.
    # - Pass Criteria: The DUT MUST send MGMT_ED_REPORT.ans to the Commissioner and report energy measurements for the
    #   Primary and Secondary channels:
    #   - CoAP Request URI: coap://[Commissioner]:MM/c/er
    #   - CoAP Payload:
    #     - Channel Mask TLV (Primary and Secondary)
    #     - Energy List TLV (4 bytes)
    print("Step 3: DUT sends a MGMT_ED_REPORT.ans response to the Commissioner.")
    pkts.filter(lambda p: p.wpan.frame_type == consts.WPAN_DATA and\
                p.wpan.src16 == pv.vars['Router_1_RLOC16'] and\
                p.wpan.dst16 != 0xffff and\
                p.coap.uri_path == consts.MGMT_ED_REPORT).\
        must_next().\
        must_verify(lambda p: p.coap.tlv.channel_mask == expected_channel_mask and\
                               len(p.coap.tlv.energy_list) == 4)

    # Step 4: Commissioner
    # - Description: Commissioner sends multicast MGMT_ED_SCAN.qry.
    # - Pass Criteria: Commissioner MUST send MGMT_ED_SCAN.qry to the Link-Local All Thread Nodes multicast address
    #   (FF32:40:<MeshLocalPrefix>::1) with the following format:
    #   - CoAP Request URI: coap://[FF32:40:<MeshLocalPrefix>::1]:MM/c/es
    #   - CoAP Payload:
    #     - Commissioner Session ID TLV (valid)
    #     - Channel Mask TLV (Primary and Secondary)
    #     - Count TLV <0x02>
    #     - Period TLV <0x00c8> (200 ms)
    #     - Scan Duration TLV <0x20> (32 ms)
    print("Step 4: Commissioner sends multicast MGMT_ED_SCAN.qry.")
    pkts.filter(lambda p: p.wpan.frame_type == consts.WPAN_DATA and\
                p.wpan.src16 == pv.vars['Commissioner_RLOC16'] and\
                p.wpan.dst16 == 0xffff and\
                p.coap.uri_path == consts.MGMT_ED_SCAN).\
        must_next().\
        must_verify(lambda p: p.coap.tlv.channel_mask == expected_channel_mask and\
                               p.coap.tlv.count == 2 and\
                               p.coap.tlv.period == 200 and\
                               p.coap.tlv.scan_duration == 32)

    # Step 5: DUT
    # - Description: Automatically sends a MGMT_ED_REPORT.ans response to the Commissioner.
    # - Pass Criteria: The DUT MUST send MGMT_ED_REPORT.ans to the Commissioner and report energy measurements for the
    #   Primary and Secondary channels:
    #   - CoAP Request URI: coap://[Commissioner]:MM/c/er
    #   - CoAP Payload:
    #     - Channel Mask TLV (Primary and Secondary)
    #     - Energy List TLV (length of 4 bytes)
    print("Step 5: DUT sends a MGMT_ED_REPORT.ans response to the Commissioner.")
    pkts.filter(lambda p: p.wpan.frame_type == consts.WPAN_DATA and\
                p.wpan.src16 == pv.vars['Router_1_RLOC16'] and\
                p.wpan.dst16 != 0xffff and\
                p.coap.uri_path == consts.MGMT_ED_REPORT).\
        must_next().\
        must_verify(lambda p: p.coap.tlv.channel_mask == expected_channel_mask and\
                               len(p.coap.tlv.energy_list) == 4)

    # Step 6: Commissioner
    # - Description: Harness verifies connectivity by instructing the Commissioner to send an ICMPv6 Echo Request to
    #   the DUT mesh local address.
    # - Pass Criteria: The DUT MUST respond with an ICMPv6 Echo Reply.
    print("Step 6: Commissioner sends an ICMPv6 Echo Request to the DUT.")
    pkts.filter(lambda p: p.wpan.frame_type == consts.WPAN_DATA and\
                p.wpan.src16 == pv.vars['Commissioner_RLOC16'] and\
                p.icmpv6.type == consts.ICMPV6_TYPE_ECHO_REQUEST).\
        must_next()
    pkts.filter(lambda p: p.wpan.frame_type == consts.WPAN_DATA and\
                p.wpan.src16 == pv.vars['Router_1_RLOC16'] and\
                p.icmpv6.type == consts.ICMPV6_TYPE_ECHO_REPLY).\
        must_next()


if __name__ == '__main__':
    verify_utils.run_main(verify)
