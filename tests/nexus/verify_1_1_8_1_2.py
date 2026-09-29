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
from pktverify.null_field import nullField

# Constants
JOINER_UDP_PORT = 49153
ALERT_LEVEL_FATAL = 2
DTLS_PORT = 1000
MLE_SEC_SUITE_NONE = 255

THREAD_VERSION_1_1 = 2
THREAD_VERSION_1_2 = 3
THREAD_VERSION_1_3 = 4
THREAD_VERSION_1_4 = 5


def verify(pv):
    # 8.1.2 On-Mesh Commissioner Joining, no JR, any commissioner, single (incorrect)
    #
    # 8.1.2.1 Topology
    # (Note: A topology diagram is included in the source material depicting Topology A (DUT as Commissioner) and
    #   Topology B (DUT as Joiner_1), with a point-to-point connection between the Commissioner and Joiner_1.)
    #
    # 8.1.2.2 Purpose & Description
    # The purpose of this test case is to verify that the DUT - as an on-mesh Commissioner - detects the use of an
    #   incorrect PSKd by a Joiner.
    # Note that many of the messages/records exchanged are encrypted and cannot be observed.
    #
    # Spec Reference                             | V1.1 Section  | V1.3.0 Section
    # -------------------------------------------|---------------|---------------
    # Dissemination of Datasets / Joiner Protocol | 8.4.3 / 8.4.4 | 8.4.3 / 8.4.4

    pkts = pv.pkts
    pv.summary.show()

    COMMISSIONER = pv.vars['Commissioner']
    JOINER_1 = pv.vars['Joiner_1']

    # Step 1: Joiner_1
    # - Description: PSKd configured on Joiner_1 is different to PSKd configured on the DUT.
    # - Pass Criteria: N/A.
    print("Step 1: Joiner_1 configured with incorrect PSKd.")

    # Step 2: Commissioner (DUT)
    # - Description: Begin wireless sniffer and power on the DUT.
    # - Pass Criteria: N/A.
    print("Step 2: Commissioner started.")

    # Step 3: Joiner_1
    # - Description: Initiate the DTLS-Handshake by sending a DTLS-ClientHello handshake record to the DUT.
    # - Pass Criteria: Verify the following details occur in the exchange between Joiner_1 and the DUT:
    print("Step 3: Joiner_1 initiates the DTLS handshake.")

    # Discovery: Joiner_1 sends MLE Discovery Request.
    print("Step 3.Discovery: Joiner_1 sends MLE Discovery Request.")
    _pkt = pkts.filter_LLARMA().\
        filter_mle_cmd(consts.MLE_DISCOVERY_REQUEST).\
        filter(lambda p: {
                          consts.THREAD_DISCOVERY_TLV
                          } <= set(p.mle.tlv.type)).\
        filter(lambda p: p.mle.sec_suite == MLE_SEC_SUITE_NONE).\
        filter(lambda p: p.thread_meshcop.tlv.discovery_req_ver in [
            THREAD_VERSION_1_1,
            THREAD_VERSION_1_2,
            THREAD_VERSION_1_3,
            THREAD_VERSION_1_4
        ]).\
        must_next()
    JOINER_1 = _pkt.wpan.src64

    # Discovery Response: Commissioner sends MLE Discovery Response.
    print("Step 3.DiscoveryResponse: Commissioner sends MLE Discovery Response.")
    _pkt = pkts.filter_wpan_src64(COMMISSIONER).\
        filter_wpan_dst64(JOINER_1).\
        filter_mle_cmd(consts.MLE_DISCOVERY_RESPONSE).\
        filter(lambda p: {
                          consts.THREAD_DISCOVERY_TLV,
                          } <= set(p.mle.tlv.type)).\
        filter(lambda p: p.mle.sec_suite == MLE_SEC_SUITE_NONE).\
        filter(lambda p: p.thread_meshcop.tlv.discovery_rsp_ver in [
            THREAD_VERSION_1_1,
            THREAD_VERSION_1_2,
            THREAD_VERSION_1_3,
            THREAD_VERSION_1_4
        ]).\
        filter(lambda p: p.thread_meshcop.tlv.udp_port is not nullField).\
        must_next()
    COMMISSIONER_UDP_PORTS = _pkt.thread_meshcop.tlv.udp_port

    # 1. UDP port (Specified by the DUT in Discovery Response) is used as destination port for UDP datagrams from
    #    Joiner_1 to the DUT.
    # 2. Joiner_1 sends an initial DTLS-ClientHello handshake record to the DUT.
    print("Step 3.1 & 3.2: Joiner_1 sends an initial DTLS-ClientHello.")
    _pkt = pkts.filter_wpan_dst64(COMMISSIONER).\
        filter(lambda p: consts.HANDSHAKE_CLIENT_HELLO in p.dtls.handshake.type).\
        filter(lambda p: p.udp.srcport == JOINER_UDP_PORT).\
        filter(lambda p: p.udp.dstport in COMMISSIONER_UDP_PORTS).\
        must_next()

    JOINER_1 = _pkt.wpan.src64
    COMMISSIONER_UDP_PORT = _pkt.udp.dstport

    # 3. The DUT must correctly receive the initial DTLS-ClientHello handshake record and send a
    #    DTLS-HelloVerifyRequest handshake record to Joiner_1.
    print("Step 3.3: Commissioner sends a DTLS-HelloVerifyRequest.")
    hvr = pkts.filter_wpan_src64(COMMISSIONER).\
        filter_wpan_dst64(JOINER_1).\
        filter(lambda p: consts.HANDSHAKE_HELLO_VERIFY_REQUEST in p.dtls.handshake.type).\
        filter(lambda p: p.udp.srcport == COMMISSIONER_UDP_PORT).\
        must_next()

    # 4. Joiner_1 receives the DTLS-HelloVerifyRequest handshake record and sends a subsequent DTLS-ClientHello
    #    handshake record in one UDP datagram to the DUT.
    # 5. Verify that both DTLS-HelloVerifyRequest and subsequent DTLS-ClientHello contain the same cookie.
    print("Step 3.4 & 3.5: Joiner_1 sends a subsequent DTLS-ClientHello with the same cookie.")
    pkts.filter_wpan_src64(JOINER_1).\
        filter_wpan_dst64(COMMISSIONER).\
        filter(lambda p: consts.HANDSHAKE_CLIENT_HELLO in p.dtls.handshake.type).\
        filter(lambda p: p.dtls.handshake.cookie == hvr.dtls.handshake.cookie).\
        filter(lambda p: p.udp.dstport == COMMISSIONER_UDP_PORT).\
        must_next()

    # 6. The DUT must correctly receive the subsequent DTLS-ClientHello handshake record and then send, in order,
    #    DTLS-ServerHello, DTLS-ServerKeyExchange and DTLS-ServerHelloDone handshake records to Joiner_1.
    print("Step 3.6: Commissioner sends DTLS-ServerHello, DTLS-ServerKeyExchange and DTLS-ServerHelloDone.")
    pkts.filter_wpan_src64(COMMISSIONER).\
        filter_wpan_dst64(JOINER_1).\
        filter(lambda p: {
                          consts.HANDSHAKE_SERVER_HELLO,
                          consts.HANDSHAKE_SERVER_KEY_EXCHANGE,
                          consts.HANDSHAKE_SERVER_HELLO_DONE
                          } <= set(p.dtls.handshake.type)).\
        filter(lambda p: p.udp.srcport == COMMISSIONER_UDP_PORT).\
        must_next()

    # 7. Joiner_1 receives the DTLS-ServerHello, DTLS-ServerKeyExchange and DTLS-ServerHelloDone handshake records
    #    and sends, in order, a DTLS-ClientKeyExchange handshake record, a DTLS-ChangeCipherSpec record and an
    #    encrypted DTLS-Finished handshake record to the DUT.
    print("Step 3.7: Joiner_1 sends DTLS-ClientKeyExchange, DTLS-ChangeCipherSpec and DTLS-Finished.")
    step7_pkt = pkts.filter_wpan_src64(JOINER_1).\
        filter_wpan_dst64(COMMISSIONER).\
        filter(lambda p: {
                          consts.HANDSHAKE_CLIENT_KEY_EXCHANGE,
                          consts.HANDSHAKE_FINISHED
                          } <= set(p.dtls.handshake.type)).\
        filter(lambda p: consts.CONTENT_CHANGE_CIPHER_SPEC in p.dtls.record.content_type).\
        filter(lambda p: p.udp.dstport == COMMISSIONER_UDP_PORT).\
        must_next()

    # Fail Conditions:
    # 11. A DTLS-Alert record with a Fatal alert level is sent by either Joiner_1 or the DUT before step 7.
    print("Step 3.FailCondition.11: Check for no Fatal DTLS alerts before Step 3.7.")
    with pv.pkts.save_index():
        pv.pkts.filter(lambda p: p.number < step7_pkt.number).\
            filter(lambda p: consts.CONTENT_ALERT in p.dtls.record.content_type).\
            filter(lambda p: p.dtls.alert_message.level == ALERT_LEVEL_FATAL).\
            must_not_next()

    # 8. The DUT must receive the DTLS-ClientKeyExchange handshake record, the DTLS-ChangeCipherSpec and the
    #    encrypted DTLS-Finished handshake record, and then send a DTLS-Alert record with error code 40 (handshake
    #    failure) or 20 (bad record MAC) - in one UDP datagram - to Joiner_1.
    print("Step 3.8: Commissioner sends DTLS-Alert (handshake failure or bad record MAC).")
    pkts.filter_wpan_src64(COMMISSIONER).\
        filter_wpan_dst64(JOINER_1).\
        filter(lambda p: consts.CONTENT_ALERT in p.dtls.record.content_type).\
        filter(lambda p: p.dtls.alert_message.desc in [20, 40]).\
        filter(lambda p: p.udp.srcport == COMMISSIONER_UDP_PORT).\
        must_next()

    # Fail Conditions:
    # 12. Further traffic between Joiner_1 and the DUT is observed after step 7. (Actually after Step 3.8)
    print("Step 3.FailCondition.12: Check for no further traffic.")
    pkts.filter_wpan_src64(JOINER_1).\
        filter_wpan_dst64(COMMISSIONER).\
        filter(lambda p: p.wpan.frame_type != consts.MAC_FRAME_TYPE_ACK).\
        must_not_next()
    pkts.filter_wpan_src64(COMMISSIONER).\
        filter_wpan_dst64(JOINER_1).\
        filter(lambda p: p.wpan.frame_type != consts.MAC_FRAME_TYPE_ACK).\
        must_not_next()


if __name__ == '__main__':
    # Add keylog file to Wireshark preferences if it exists.
    keylog_file = os.path.abspath('test_1_1_8_1_2.keys')
    if os.path.exists(keylog_file):
        consts.WIRESHARK_OVERRIDE_PREFS['tls.keylog_file'] = keylog_file
        consts.WIRESHARK_DECODE_AS_ENTRIES[f'udp.port=={DTLS_PORT}'] = 'dtls'
        consts.WIRESHARK_DECODE_AS_ENTRIES[f'dtls.port=={DTLS_PORT}'] = 'coap'
    verify_utils.run_main(verify)
