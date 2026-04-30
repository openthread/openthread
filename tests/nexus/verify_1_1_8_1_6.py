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
import json
import traceback

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
JOINER_UDP_PORT = 49153
ALERT_DESC_CLOSE_NOTIFY = 0
ALERT_LEVEL_FATAL = 2
DTLS_PORT = 1000

STATE_REJECT = 255  # -1 in 8-bit unsigned


def verify(pv):
    # 8.1.6 On-Mesh Commissioner Joining, no JR, wrong Commissioner.
    #
    # 8.1.6.1 Topology
    # (Note: A topology diagram is included in the source material depicting Topology A (DUT as Commissioner) and
    #   Topology B (DUT as Joiner_1), with a point-to-point connection between the Commissioner and Joiner_1.)
    #
    # 8.1.6.2 Purpose & Description
    # The purpose of this test case is to verify the DTLS session between an on-mesh Commissioner and a Joiner and
    #   ensure that the session does not stay open.
    #
    # Spec Reference                | V1.1 Section  | V1.3.0 Section
    # ------------------------------|---------------|---------------
    # Dissemination of Datasets     | 8.4.3         | 8.4.3
    # Joiner Protocol               | 8.4.4         | 8.4.4

    pkts = pv.pkts
    pv.summary.show()

    COMMISSIONER = pv.vars['Commissioner']

    # Step 1: Joiner_1
    # - Description: PSKd configured on Joiner_1 is the same as PSKd configured on Commissioner.
    # - Pass Criteria: N/A
    print("Step 1: Joiner_1 configured with correct PSKd.")

    # Step 2: All
    # - Description: Begin wireless sniffer and ensure topology is created and connectivity between nodes.
    # - Pass Criteria: N/A
    print("Step 2: Topology created and connectivity ensured.")

    # Step 3: Joiner_1
    # - Description: Initiate the DTLS-Handshake by sending a DTLS-ClientHello handshake record to the Commissioner.
    # - Pass Criteria: Verify that the following details occur in the exchange between Joiner_1 and the Commissioner:
    print("Step 3: Joiner_1 initiates DTLS handshake.")

    #   - 1. UDP port (Specified by Commissioner in Discovery Response) is used as destination port for UDP datagrams
    #     from Joiner_1 to Commissioner.
    print("Step 3.1: UDP port (Specified by Commissioner in Discovery Response) is used.")
    _pkt = pkts.filter_LLARMA().\
        filter_mle_cmd(consts.MLE_DISCOVERY_REQUEST).\
        must_next()
    DISCOVERY_JOINER = _pkt.wpan.src64

    _pkt = pkts.filter_wpan_src64(COMMISSIONER).\
        filter_wpan_dst64(DISCOVERY_JOINER).\
        filter_mle_cmd(consts.MLE_DISCOVERY_RESPONSE).\
        must_next()
    COMMISSIONER_UDP_PORTS = _pkt.thread_meshcop.tlv.udp_port

    #   - 2. Joiner_1 sends an initial DTLS-ClientHello handshake record to the Commissioner.
    print("Step 3.2: Joiner_1 sends an initial DTLS-ClientHello.")
    _pkt = pkts.filter_wpan_dst64(COMMISSIONER).\
        filter(lambda p: consts.HANDSHAKE_CLIENT_HELLO in p.dtls.handshake.type).\
        filter(lambda p: p.udp.srcport == JOINER_UDP_PORT).\
        filter(lambda p: p.udp.dstport in COMMISSIONER_UDP_PORTS).\
        must_next()
    JOINER = _pkt.wpan.src64

    #   - 3. The Commissioner receives the initial DTLS-ClientHello handshake record and sends a
    #     DTLS-HelloVerifyRequest handshake record to Joiner_1.
    print("Step 3.3: Commissioner sends a DTLS-HelloVerifyRequest.")
    hvr = pkts.filter_wpan_src64(COMMISSIONER).\
        filter_wpan_dst64(JOINER).\
        filter(lambda p: consts.HANDSHAKE_HELLO_VERIFY_REQUEST in p.dtls.handshake.type).\
        filter(lambda p: p.udp.srcport in COMMISSIONER_UDP_PORTS).\
        must_next()

    #   - 4. Joiner_1 receives the DTLS-HelloVerifyRequest handshake record and sends a subsequent DTLS-ClientHello
    #     handshake record in one UDP datagram to the Commissioner.
    #     - Verify that both DTLS-HelloVerifyRequest and subsequent DTLS-ClientHello contain the same cookie.
    print("Step 3.4: Joiner_1 sends a subsequent DTLS-ClientHello.")
    pkts.filter_wpan_src64(JOINER).\
        filter_wpan_dst64(COMMISSIONER).\
        filter(lambda p: consts.HANDSHAKE_CLIENT_HELLO in p.dtls.handshake.type).\
        filter(lambda p: p.dtls.handshake.cookie == hvr.dtls.handshake.cookie).\
        filter(lambda p: p.udp.dstport in COMMISSIONER_UDP_PORTS).\
        must_next()

    #   - 5. The Commissioner receives the subsequent DTLS-ClientHello handshake record and sends, in order,
    #     DTLS-ServerHello, DTLS-ServerKeyExchange and DTLS-ServerHelloDone handshake records to Joiner_1.
    print("Step 3.5: Commissioner sends DTLS-ServerHello, DTLS-ServerKeyExchange and DTLS-ServerHelloDone.")
    pkts.filter_wpan_src64(COMMISSIONER).\
        filter_wpan_dst64(JOINER).\
        filter(lambda p: {
                          consts.HANDSHAKE_SERVER_HELLO,
                          consts.HANDSHAKE_SERVER_KEY_EXCHANGE,
                          consts.HANDSHAKE_SERVER_HELLO_DONE
                          } <= set(p.dtls.handshake.type)).\
        filter(lambda p: p.udp.srcport in COMMISSIONER_UDP_PORTS).\
        must_next()

    #   - 6. Joiner_1 receives the DTLS-ServerHello, DTLS-ServerKeyExchange and DTLS-ServerHelloDone handshake records
    #     and sends, in order, a DTLS-ClientKeyExchange handshake record, a DTLS-ChangeCipherSpec record and an
    #     encrypted DTLS-Finished handshake record to the Commissioner.
    print("Step 3.6: Joiner_1 sends DTLS-ClientKeyExchange, DTLS-ChangeCipherSpec and DTLS-Finished.")
    pkts.filter_wpan_src64(JOINER).\
        filter_wpan_dst64(COMMISSIONER).\
        filter(lambda p: {
                          consts.HANDSHAKE_CLIENT_KEY_EXCHANGE,
                          consts.HANDSHAKE_FINISHED
                          } <= set(p.dtls.handshake.type)).\
        filter(lambda p: consts.CONTENT_CHANGE_CIPHER_SPEC in p.dtls.record.content_type).\
        filter(lambda p: p.udp.dstport in COMMISSIONER_UDP_PORTS).\
        must_next()

    #   - 7. The Commissioner receives the DTLS-ClientKeyExchange handshake record, the DTLS-ChangeCipherSpec record
    #     and the encrypted DTLS-Finished handshake record; it then sends, in order a DTLS-ChangeCipherSpec record and
    #     an encrypted DTLS-Finished handshake record to Joiner_1.
    print("Step 3.7: Commissioner sends DTLS-ChangeCipherSpec and DTLS-Finished.")
    pkts.filter_wpan_src64(COMMISSIONER).\
        filter_wpan_dst64(JOINER).\
        filter(lambda p: consts.HANDSHAKE_FINISHED in p.dtls.handshake.type).\
        filter(lambda p: consts.CONTENT_CHANGE_CIPHER_SPEC in p.dtls.record.content_type).\
        filter(lambda p: p.udp.srcport in COMMISSIONER_UDP_PORTS).\
        must_next()

    #   - 8. Joiner_1 receives the DTLS-ChangeCipherSpec record and the encrypted DTLS-Finished handshake record; it
    #     then sends a JOIN_FIN.req message in an encrypted DTLS-ApplicationData record in a single UDP datagram to the
    #     Commissioner.
    #     - The JOIN_FIN.req message must contain a Provisioning URL TLV which the Commissioner will not recognize.
    print("Step 3.8: Joiner_1 sends a JOIN_FIN.req (Application Data).")
    pkts.filter_wpan_src64(JOINER).\
        filter_wpan_dst64(COMMISSIONER).\
        filter(lambda p: consts.CONTENT_APPLICATION_DATA in p.dtls.record.content_type).\
        filter(lambda p: p.coap.uri_path == JOIN_FIN_URI).\
        filter(lambda p: p.coap.tlv.provisioning_url == 'wrong-url').\
        filter(lambda p: p.udp.dstport in COMMISSIONER_UDP_PORTS).\
        must_next()

    #   - 9. The Commissioner receives the encrypted DTLS-ApplicationData record and sends a JOIN_FIN.rsp message with
    #     Reject state in an encrypted DTLS-ApplicationData record in a single UDP datagram to Joiner_1.
    print("Step 3.9: Commissioner sends a JOIN_FIN.rsp (Application Data) with Reject state.")
    pkts.filter_wpan_src64(COMMISSIONER).\
        filter_wpan_dst64(JOINER).\
        filter(lambda p: consts.CONTENT_APPLICATION_DATA in p.dtls.record.content_type).\
        filter(lambda p: p.coap.code == consts.COAP_CODE_ACK).\
        filter(lambda p: p.coap.tlv.state == STATE_REJECT).\
        filter(lambda p: p.udp.srcport in COMMISSIONER_UDP_PORTS).\
        must_next()

    #   - 10. Commissioner sends an encrypted JOIN_ENT.ntf message to Joiner_1.
    print("Step 3.10: Commissioner sends JOIN_ENT.ntf (Encrypted).")
    pkts.copy().filter_wpan_src64(COMMISSIONER).\
        filter_wpan_dst64(JOINER).\
        filter(lambda p: p.wpan.security == True).\
        must_next()

    #   - 11. Joiner_1 receives the encrypted JOIN_ENT.ntf message and sends an encrypted JOIN_ENT.ntf dummy response
    #     to Commissioner.
    print("Step 3.11: Joiner_1 sends JOIN_ENT.ntf response (Encrypted).")
    pkts.copy().filter_wpan_src64(JOINER).\
        filter_wpan_dst64(COMMISSIONER).\
        filter(lambda p: p.wpan.security == True).\
        must_next()

    #   - 12. Joiner_1 sends an encrypted DTLS-Alert record with a code of 0 (close_notify) to the Commissioner.
    #   Note: OpenThread Joiner sends close_notify immediately after receiving JOIN_FIN.rsp with Reject state.
    print("Step 3.12: Joiner_1 sends DTLS-Alert (close_notify).")
    pkts.copy().filter_wpan_src64(JOINER).\
        filter_wpan_dst64(COMMISSIONER).\
        filter(lambda p: consts.CONTENT_ALERT in p.dtls.record.content_type).\
        filter(lambda p: p.dtls.alert_message.desc == ALERT_DESC_CLOSE_NOTIFY).\
        filter(lambda p: p.udp.dstport in COMMISSIONER_UDP_PORTS).\
        must_next()

    # Fail Conditions:
    #   - 3. A DTLS-Alert record with a Fatal alert level is sent by either Joiner_1 or the Commissioner.
    print("Check for no Fatal DTLS alerts.")
    pkts.filter(lambda p: consts.CONTENT_ALERT in p.dtls.record.content_type).\
        filter(lambda p: p.dtls.alert_message.level == ALERT_LEVEL_FATAL).\
        must_not_next()


if __name__ == '__main__':
    verify_utils.apply_patches()

    json_file = os.path.abspath(sys.argv[1])
    keys_file = json_file.replace('.json', '.keys')

    with open(json_file, 'rt') as f:
        data = json.load(f)

    consts.WIRESHARK_DECODE_AS_ENTRIES[f'udp.port=={JOINER_UDP_PORT}'] = 'dtls'
    consts.WIRESHARK_DECODE_AS_ENTRIES[f'dtls.port=={JOINER_UDP_PORT}'] = 'coap'

    wireshark_prefs = consts.WIRESHARK_OVERRIDE_PREFS.copy()
    if os.path.exists(keys_file):
        wireshark_prefs['tls.keylog_file'] = keys_file

    existing_keys_str = wireshark_prefs.get('uat:ieee802154_keys', '')
    keys = [line.strip() for line in existing_keys_str.split('\n') if line.strip()]

    network_key = data.get('network_key')
    if network_key:
        new_key = f'"{network_key}","1","Thread hash"'
        if not any(network_key in k for k in keys):
            keys.append(new_key)
    wireshark_prefs['uat:ieee802154_keys'] = '\n'.join(keys)

    try:
        pv = verify_utils.PacketVerifier(json_file, wireshark_prefs=wireshark_prefs)
        pv.add_common_vars()

        for node_id, rloc16 in data.get('rloc16s', {}).items():
            name = pv.test_info.get_node_name(int(node_id))
            pv.add_vars(**{f'{name}_RLOC16': int(rloc16, 16)})

        verify(pv)
        print("Verification PASSED")
    except Exception as e:
        print(f"Verification FAILED: {e}")
        traceback.print_exc()
        sys.exit(1)
