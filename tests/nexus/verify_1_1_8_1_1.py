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

# COAP URIs and TLVs not in consts.py
JOIN_FIN_URI = '/c/jf'
JOIN_ENT_URI = '/c/je'

# Constants
NM_PROVISIONING_URL_TLV = 32
MLE_SEC_SUITE_NONE = 255
JOINER_UDP_PORT = 49153

ALERT_DESC_CLOSE_NOTIFY = 0
ALERT_LEVEL_FATAL = 2
DTLS_PORT = 1000

THREAD_VERSION_1_1 = 2
THREAD_VERSION_1_2 = 3
THREAD_VERSION_1_3 = 4
THREAD_VERSION_1_4 = 5


def verify(pv):
    # 8.1.1 On-Mesh Commissioner Joining, no JR, any commissioner, single (correct)
    #
    # 8.1.1.1 Topology
    # (Note: A topology diagram is included in the source material depicting Topology A (DUT as Commissioner) and
    #   Topology B (DUT as Joiner_1), with a point-to-point connection between the Commissioner and Joiner_1.)
    #
    # 8.1.1.2 Purpose & Description
    # The purpose of this test case is to verify the DTLS sessions between the on-mesh Commissioner and a Joiner when
    #   the correct PSKd is used.
    # * Note that many of the messages/records exchanged are encrypted and cannot be observed.
    #
    # Spec Reference   | V1.1 Section  | V1.3.0 Section
    # -----------------|---------------|---------------
    # Joiner Protocol  | 8.4.3 / 8.4.4 | 8.4.3 / 8.4.4

    pkts = pv.pkts
    pv.summary.show()

    COMMISSIONER = pv.vars['Commissioner']
    JOINER = pv.vars['Joiner']

    # Step 1: Joiner_1
    # - Description: Configure Joiner_1 to a correct PSKd same as PSKd configured on Commissioner.
    # - Pass Criteria: N/A
    print("Step 1: Joiner_1 configured with correct PSKd.")

    # Step 2: Commissioner
    # - Description: Begin wireless sniffer and Commissioner.
    # - Pass Criteria: N/A
    print("Step 2: Commissioner started.")

    # Step 3: Joiner_1
    # - Description: Sends MLE Discovery Request.
    # - Pass Criteria: For DUT = Joiner: The DUT MUST send MLE Discovery Request, including the following values:
    #   - MLE Security Suite = 255 (No MLE Security)
    #   - Thread Discovery TLV Sub-TLVs:
    #     - Discovery Request TLV
    #     - Protocol Version = 2, 3, 4 or 5
    #   - Optional: [list of one or more concatenated Extended PAN ID TLVs]
    print("Step 3: Joiner_1 sends MLE Discovery Request.")
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
    JOINER = _pkt.wpan.src64

    # Step 4: Commissioner
    # - Description: Automatically sends MLE Discovery Response.
    # - Pass Criteria: For DUT = Commissioner: The DUT MUST send MLE Discovery Response, including these values:
    #   - MLE Security Suite: 255 (No MLE Security)
    #   - Source Address in IEEE 802.15.4 header MUST be set to the MAC Extended Address (64-bit)
    #   - Destination Address in IEEE 802.15.4 header MUST be set to Discovery Request Source Address
    #   - Thread Discovery TLV Sub-TLVs:
    #     - Discovery Response TLV (Protocol Version = 2, 3, 4 or 5)
    #     - Extended PAN ID TLV
    #     - Joiner UDP Port TLV
    #     - Network Name TLV
    #     - Steering Data TLV
    #     - Commissioner UDP Port TLV (optional)
    print("Step 4: Commissioner sends MLE Discovery Response.")
    _pkt = pkts.filter_wpan_src64(COMMISSIONER).\
        filter_wpan_dst64(JOINER).\
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
        filter(lambda p: p.thread_meshcop.tlv.xpan_id is not nullField).\
        filter(lambda p: p.thread_meshcop.tlv.udp_port is not nullField).\
        filter(lambda p: p.thread_meshcop.tlv.net_name is not nullField).\
        filter(lambda p: p.thread_meshcop.tlv.steering_data is not nullField).\
        must_next()
    COMMISSIONER_UDP_PORTS = _pkt.thread_meshcop.tlv.udp_port

    # Step 5: Joiner_1 and Commissioner
    # - Description: Initiate the DTLS-Handshake by sending a DTLS-ClientHello handshake record to the Commissioner
    #   from Joiner_1.
    # - Pass Criteria: For DUT = Joiner:
    #   - 1. Joiner_1 sends an initial DTLS-ClientHello handshake record to Commissioner. The DUT must send
    #     DTLS_Client_Hello to the Commissioner, using UDP port 49153. If the Commissioner responds with HelloVerify,
    #     the DUT MUST send DTLS-ClientHello on UDP port 49153 and with the same cookie sent by the Commissioner.
    #   - 2. UDP port (Specified by Commissioner in Discovery Response) is used as destination port for UDP datagrams
    #     from Joiner_1 to Commissioner.
    print("Step 5.1: Joiner_1 sends an initial DTLS-ClientHello.")
    _pkt = pkts.filter_wpan_dst64(COMMISSIONER).\
        filter(lambda p: consts.HANDSHAKE_CLIENT_HELLO in p.dtls.handshake.type).\
        filter(lambda p: p.udp.srcport == JOINER_UDP_PORT).\
        filter(lambda p: p.udp.dstport in COMMISSIONER_UDP_PORTS).\
        must_next()

    JOINER = _pkt.wpan.src64

    #   - 4. Commissioner receives the initial DTLS-ClientHello handshake record and sends a DTLS-HelloVerifyRequest
    #     handshake record to Joiner_1.
    print("Step 5.4: Commissioner sends a DTLS-HelloVerifyRequest.")
    hvr = pkts.filter_wpan_src64(COMMISSIONER).\
        filter_wpan_dst64(JOINER).\
        filter(lambda p: consts.HANDSHAKE_HELLO_VERIFY_REQUEST in p.dtls.handshake.type).\
        filter(lambda p: p.udp.srcport in COMMISSIONER_UDP_PORTS).\
        must_next()

    #   - 5. Joiner_1 receives the DTLS-HelloVerifyRequest handshake record and sends a subsequent DTLS-ClientHello
    #     handshake record in one UDP datagram to Commissioner.
    #     - Verify that both DTLS-HelloVerifyRequest and subsequent DTLS-ClientHello contain the same cookie.
    print("Step 5.5: Joiner_1 sends a subsequent DTLS-ClientHello.")
    pkts.filter_wpan_src64(JOINER).\
        filter_wpan_dst64(COMMISSIONER).\
        filter(lambda p: consts.HANDSHAKE_CLIENT_HELLO in p.dtls.handshake.type).\
        filter(lambda p: p.dtls.handshake.cookie == hvr.dtls.handshake.cookie).\
        filter(lambda p: p.udp.dstport in COMMISSIONER_UDP_PORTS).\
        must_next()

    #   - 6. Commissioner receives the subsequent DTLS-ClientHello handshake record and sends DTLS-ServerHello,
    #     DTLS-ServerKeyExchange and DTLS-ServerHelloDone handshake records in that order to Joiner_1.
    print("Step 5.6: Commissioner sends DTLS-ServerHello, DTLS-ServerKeyExchange and DTLS-ServerHelloDone.")
    pkts.filter_wpan_src64(COMMISSIONER).\
        filter_wpan_dst64(JOINER).\
        filter(lambda p: {
                          consts.HANDSHAKE_SERVER_HELLO,
                          consts.HANDSHAKE_SERVER_KEY_EXCHANGE,
                          consts.HANDSHAKE_SERVER_HELLO_DONE
                          } <= set(p.dtls.handshake.type)).\
        filter(lambda p: p.udp.srcport in COMMISSIONER_UDP_PORTS).\
        must_next()

    #   - 7. Joiner_1 receives the DTLS-ServerHello, DTLS-ServerKeyExchange and DTLS-ServerHelloDone handshake records
    #     and sends a DTLS-ClientKeyExchange handshake record, a DTLS-ChangeCipherSpec record and an encrypted
    #     DTLS-Finished handshake record in that order to Commissioner.
    print("Step 5.7: Joiner_1 sends DTLS-ClientKeyExchange, DTLS-ChangeCipherSpec and DTLS-Finished.")
    pkts.filter_wpan_src64(JOINER).\
        filter_wpan_dst64(COMMISSIONER).\
        filter(lambda p: {
                          consts.HANDSHAKE_CLIENT_KEY_EXCHANGE,
                          consts.HANDSHAKE_FINISHED
                          } <= set(p.dtls.handshake.type)).\
        filter(lambda p: consts.CONTENT_CHANGE_CIPHER_SPEC in p.dtls.record.content_type).\
        filter(lambda p: p.udp.dstport in COMMISSIONER_UDP_PORTS).\
        must_next()

    #   - 8. Commissioner receives the DTLS-ClientKeyExchange handshake record, the DTLS-ChangeCipherSpec record and
    #     the encrypted DTLS-Finished handshake record and sends a DTLS-ChangeCipherSpec record and an encrypted
    #     DTLS-Finished handshake record in that order to Joiner_1.
    print("Step 5.8: Commissioner sends DTLS-ChangeCipherSpec and DTLS-Finished.")
    pkts.filter_wpan_src64(COMMISSIONER).\
        filter_wpan_dst64(JOINER).\
        filter(lambda p: consts.HANDSHAKE_FINISHED in p.dtls.handshake.type).\
        filter(lambda p: consts.CONTENT_CHANGE_CIPHER_SPEC in p.dtls.record.content_type).\
        filter(lambda p: p.udp.srcport in COMMISSIONER_UDP_PORTS).\
        must_next()

    # Step 5.9: Joiner_1 receives the DTLS-ChangeCipherSpec record and the encrypted DTLS-Finished handshake record and
    #     sends a JOIN_FIN.req message in an encrypted DTLS-ApplicationData record in a single UDP datagram to
    #     Commissioner.
    #     - The JOIN_FIN.req message must not contain a Provisioning URL TLV.
    print("Step 5.9: Joiner_1 sends a JOIN_FIN.req (Application Data).")
    pkts.filter_wpan_src64(JOINER).\
        filter_wpan_dst64(COMMISSIONER).\
        filter(lambda p: consts.CONTENT_APPLICATION_DATA in p.dtls.record.content_type).\
        filter(lambda p: p.coap.uri_path == JOIN_FIN_URI).\
        filter(lambda p: NM_PROVISIONING_URL_TLV not in p.coap.tlv.type).\
        filter(lambda p: p.udp.dstport in COMMISSIONER_UDP_PORTS).\
        must_next()

    #   - 10. Commissioner receives the encrypted DTLS-ApplicationData record and sends a JOIN_FIN.rsp message in an
    #     encrypted DTLS-ApplicationData record in a single UDP datagram to Joiner_1.
    print("Step 5.10: Commissioner sends a JOIN_FIN.rsp (Application Data).")
    pkts.filter_wpan_src64(COMMISSIONER).\
        filter_wpan_dst64(JOINER).\
        filter(lambda p: consts.CONTENT_APPLICATION_DATA in p.dtls.record.content_type).\
        filter(lambda p: p.coap.code == consts.COAP_CODE_ACK).\
        filter(lambda p: p.udp.srcport in COMMISSIONER_UDP_PORTS).\
        must_next()

    #   - 11. Commissioner sends an encrypted JOIN_ENT.ntf message to Joiner_1.
    print("Step 5.11: Commissioner sends JOIN_ENT.ntf (Encrypted).")
    pkts.copy().filter_wpan_src64(COMMISSIONER).\
        filter_wpan_dst64(JOINER).\
        filter(lambda p: p.wpan.security == True).\
        must_next()

    #   - 12. Joiner_1 receives the encrypted JOIN_ENT.ntf message and sends an encrypted JOIN_ENT.ntf dummy response
    #     to Commissioner.
    print("Step 5.12: Joiner_1 sends JOIN_ENT.ntf response (Encrypted).")
    pkts.copy().filter_wpan_src64(JOINER).\
        filter_wpan_dst64(COMMISSIONER).\
        filter(lambda p: p.wpan.security == True).\
        must_next()

    #   - 13. Joiner_1 sends an encrypted DTLS-Alert record with a code of 0 (close_notify) to Commissioner.
    #   Note: OpenThread Joiner sends close_notify immediately after receiving JOIN_FIN.rsp, while it expects
    #   JOIN_ENT.ntf over the mesh. The spec lists 11, 12, 13 but these are independent exchanges.
    print("Step 5.13: Joiner_1 sends DTLS-Alert (close_notify).")
    pkts.copy().filter_wpan_src64(JOINER).\
        filter_wpan_dst64(COMMISSIONER).\
        filter(lambda p: consts.CONTENT_ALERT in p.dtls.record.content_type).\
        filter(lambda p: p.dtls.alert_message.desc == ALERT_DESC_CLOSE_NOTIFY).\
        filter(lambda p: p.udp.dstport in COMMISSIONER_UDP_PORTS).\
        must_next()

    # Fail Conditions:
    #   - 3. A DTLS-Alert record with a Fatal alert level is sent by either Joiner_1 or Commissioner.
    print("Check for no Fatal DTLS alerts.")
    pkts.filter(lambda p: consts.CONTENT_ALERT in p.dtls.record.content_type).\
        filter(lambda p: p.dtls.alert_message.level == ALERT_LEVEL_FATAL).\
        must_not_next()


if __name__ == '__main__':
    # Add keylog file to Wireshark preferences if it exists.
    keylog_file = os.path.abspath('test_1_1_8_1_1.keys')
    if os.path.exists(keylog_file):
        consts.WIRESHARK_OVERRIDE_PREFS['tls.keylog_file'] = keylog_file
        consts.WIRESHARK_DECODE_AS_ENTRIES[f'udp.port=={DTLS_PORT}'] = 'dtls'
        consts.WIRESHARK_DECODE_AS_ENTRIES[f'dtls.port=={DTLS_PORT}'] = 'coap'
    verify_utils.run_main(verify)
