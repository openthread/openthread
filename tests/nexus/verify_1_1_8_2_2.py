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

# Add the current directory to sys.path to find verify_utils
CUR_DIR = os.path.dirname(os.path.abspath(__file__))
sys.path.append(CUR_DIR)

import verify_utils
from pktverify import consts
from pktverify.packet_verifier import PacketVerifier
from pktverify.null_field import nullField

# Constants
JOINER_UDP_PORT = 49153
TMF_PORT = 61631
ALERT_LEVEL_FATAL = 2


def field_contains(field, value):
    if hasattr(field, 'all_fields'):
        return value in [int(f.get_default_value()) for f in field.all_fields]
    elif isinstance(field, list):
        return value in field
    else:
        try:
            return int(field) == value
        except (TypeError, ValueError):
            return False


def _verify_rly(p, uri, router_rloc16, joiner_port=None, joiner_iid=None):
    pkt = p.must_next()
    assert pkt.coap.type == 1,\
        f"Expected CoAP type NON (1), got {pkt.coap.type}"
    assert pkt.coap.opt.uri_path_recon == uri,\
        f"Expected URI {uri}, got {pkt.coap.opt.uri_path_recon}"
    assert pkt.coap.tlv.joiner_dtls_encap,\
        "joiner_dtls_encap missing"
    if joiner_port is not None:
        assert pkt.coap.tlv.joiner_udp_port == joiner_port,\
            f"Expected port {joiner_port}, got {pkt.coap.tlv.joiner_udp_port}"
    if joiner_iid is not None:
        assert pkt.coap.tlv.joiner_iid == joiner_iid,\
            f"Expected IID {joiner_iid.hex()}, got {pkt.coap.tlv.joiner_iid.hex()}"
    assert pkt.coap.tlv.joiner_router_locator == router_rloc16,\
        f"Expected locator {router_rloc16}, got {pkt.coap.tlv.joiner_router_locator}"
    return pkt


def verify_rly_rx(p, router_rloc16, joiner_port=None, joiner_iid=None):
    return _verify_rly(p, consts.RLY_RX_URI, router_rloc16, joiner_port, joiner_iid)


def verify_rly_tx(p, router_rloc16, joiner_port=None, joiner_iid=None):
    return _verify_rly(p, consts.RLY_TX_URI, router_rloc16, joiner_port, joiner_iid)


def verify(pv):
    # 8.2.2 On Mesh Commissioner Joining with JR, any commissioner, single (incorrect).
    #
    # 8.2.2.1 Topology
    # (Note: A topology diagram is implicitly referenced, showing Topology A with the DUT as the
    #   Commissioner/Leader, a Joiner Router, and Joiner_1, and Topology B with the Commissioner/Leader, the DUT as
    #   the Joiner Router, and Joiner_1.)
    #
    # 8.2.2.2 Purpose & Description
    # The purpose of this test case is to verify that the DUT sends and receives relayed DTLS traffic correctly, when
    #   an incorrect PSKd is used by the Joiner.
    #
    # Spec Reference                          | V1.1 Section  | V1.3.0 Section
    # ----------------------------------------|---------------|---------------
    # Relay DTLS Traffic / Joiner Protocol    | 8.4.3 / 8.4.4 | 8.4.3 / 8.4.4

    pkts = pv.pkts
    pv.summary.show()

    COMMISSIONER = pv.vars['Commissioner']
    JOINER_ROUTER = pv.vars['Joiner_Router']

    COMMISSIONER_RLOC16 = pv.vars['Commissioner_RLOC16']
    JOINER_ROUTER_RLOC16 = pv.vars['Joiner_Router_RLOC16']

    # Step 0: All
    # - Description: Form topology excluding the Joiner and including the DUT. The DUT may be joined to the network
    #   using either Thread Commissioning or OOB (out-of-band) Commissioning.
    # - Pass Criteria: N/A.
    print("Step 0: Topology formed.")

    # Step 1: Joiner_1
    # - Description: PSKd configured on Joiner_1 is different to PSKd configured on the Commissioner.
    # - Pass Criteria: N/A.
    print("Step 1: Joiner_1 configured with incorrect PSKd.")

    # Step 2: Commissioner
    # - Description: Begin wireless sniffer and power on the Commissioner and Joiner Router.
    # - Pass Criteria: N/A.
    print("Step 2: Commissioner and Joiner Router started.")

    # Step 3: Joiner_1
    # - Description: Initiate the DTLS-Handshake by sending a DTLS-ClientHello handshake record to the Commissioner.
    # - Pass Criteria: Verify that the following details occur in the exchange between the Joiner, Joiner_Router and
    #   the Commissioner:
    print("Step 3: Joiner_1 initiates the DTLS handshake.")

    # - 1. UDP port (Specified by Commissioner in Discovery Response) is used as destination port for UDP datagrams
    #   from Joiner_1 to Commissioner.
    print("Step 3.1: Discover Joiner Port from MLE Discovery Response.")
    discovery_rsp = pkts.filter_wpan_src64(COMMISSIONER).\
        filter_mle_cmd(consts.MLE_DISCOVERY_RESPONSE).\
        must_next()
    JOINER_PORTS = discovery_rsp.thread_meshcop.tlv.udp_port
    if not isinstance(JOINER_PORTS, list):
        JOINER_PORTS = [JOINER_PORTS]
    JOINER_PORTS = [int(p) for p in JOINER_PORTS]
    assert JOINER_UDP_PORT in JOINER_PORTS,\
        f"Expected {JOINER_UDP_PORT} in {JOINER_PORTS}"

    # Find the Joiner by its initial ClientHello
    print("Finding Joiner from initial DTLS ClientHello")
    initial_hello_pkt = pkts.filter(lambda p: p.udp.dstport == JOINER_UDP_PORT or p.udp.srcport == JOINER_UDP_PORT).\
        must_next()
    JOINER_1 = initial_hello_pkt.wpan.src64
    pv.add_vars(JOINER_1=JOINER_1)

    # - 2. Joiner_1 sends an initial DTLS-ClientHello handshake record to Joiner_Router.
    print("Step 3.2: Joiner_1 sends an initial DTLS-ClientHello.")
    pkts.index = (0, 0)
    pkts.filter_wpan_src64(JOINER_1).\
        filter_wpan_dst64(JOINER_ROUTER).\
        filter(lambda p: p.udp.dstport == JOINER_UDP_PORT).\
        filter(lambda p: field_contains(p.dtls.handshake.type, 1)).\
        must_next()

    # - 3. The Joiner_Router receives the initial DTLS-ClientHello handshake record and sends a RLY_RX.ntf message to
    #   the Commissioner containing:
    #   - CoAP URI-Path: NON POST coap://<C>/c/rx
    #   - CoAP Payload:
    #     - Joiner_1 DTLS Encapsulation TLV: Type = 17 (0x11), Length = Variable (8/16 bit size), Value = initial
    #       DTLS-ClientHello handshake record received from Joiner_1.
    #     - Joiner_1 UDP Port TLV: Type = 18 (0x12), Length = 2, Value = UDP port of Joiner_1.
    #     - Joiner_1 IID TLV: Type = 19 (0x13), Length = 8, Value = The IID based on the EUI-64 of Joiner_1.
    #     - Joiner_Router Locator TLV: Type = 20 (0x14), Length = 2, Value = RLOC of Joiner_Router.
    print("Step 3.3: Joiner_Router sends RLY_RX.ntf to Commissioner.")
    rly_rx_filter = pkts.filter(lambda p: p.wpan.src16 == JOINER_ROUTER_RLOC16).\
        filter(lambda p: p.wpan.dst16 == COMMISSIONER_RLOC16).\
        filter(lambda p: p.udp.dstport == TMF_PORT).\
        filter(lambda p: p.coap.opt.uri_path_recon == consts.RLY_RX_URI)
    rly_rx_pkt = verify_rly_rx(rly_rx_filter, JOINER_ROUTER_RLOC16, joiner_port=JOINER_UDP_PORT)
    EXPECTED_JOINER_IID = rly_rx_pkt.coap.tlv.joiner_iid
    pv.add_vars(EXPECTED_JOINER_IID=EXPECTED_JOINER_IID)

    # - 4. The Commissioner receives the RLY_RX.ntf message and sends a RLY_TX.ntf message to the Joiner_Router
    #   containing:
    #   - CoAP URI-Path: NON POST coap://<C>/c/tx
    #   - CoAP Payload:
    #     - Joiner_1 DTLS Encapsulation TLV: Type = 17 (0x011), Length = Variable (8 bit size), Value =
    #       DTLS-HelloVerifyRequest handshake record destined for Joiner_1.
    #     - Joiner_1 UDP Port TLV: Type = 18 (0x12), Length = 2, Value = UDP port of Joiner_1.
    #     - Joiner_1 IID TLV: Type = 19 (0x13), Length = 8, Value = The IID based on the EUI-64 of Joiner_1.
    #     - Joiner_Router Locator TLV: Type = 20 (0x14), Length = 2, Value = RLOC of Joiner_Router.
    print("Step 3.4: Commissioner sends RLY_TX.ntf to Joiner_Router.")
    rly_tx_filter = pkts.filter(lambda p: p.wpan.src16 == COMMISSIONER_RLOC16).\
        filter(lambda p: p.wpan.dst16 == JOINER_ROUTER_RLOC16).\
        filter(lambda p: p.udp.dstport == TMF_PORT).\
        filter(lambda p: p.coap.opt.uri_path_recon == consts.RLY_TX_URI)
    verify_rly_tx(rly_tx_filter, JOINER_ROUTER_RLOC16, joiner_port=JOINER_UDP_PORT, joiner_iid=EXPECTED_JOINER_IID)

    # - 5. The Joiner_Router receives the RLY_TX.ntf message and sends the encapsulated DTLS-HelloVerifyRequest
    #   handshake record in one UDP datagram to Joiner_1.
    print("Step 3.5: Joiner_Router sends DTLS-HelloVerifyRequest to Joiner_1.")
    pkts.index = (0, 0)
    hvr_pkt = pkts.filter_wpan_src64(JOINER_ROUTER).\
        filter_wpan_dst64(JOINER_1).\
        filter(lambda p: p.udp.srcport == JOINER_UDP_PORT).\
        filter(lambda p: field_contains(p.dtls.handshake.type, 3)).\
        must_next()
    COOKIE = hvr_pkt.dtls.handshake.cookie

    # - 6. Joiner_1 receives the DTLS-HelloVerifyRequest handshake record and sends a subsequent DTLS-ClientHello
    #   handshake record in one UDP datagram to the Commissioner.
    #   - Verify that both DTLS-HelloVerifyRequest and subsequent DTLS-ClientHello contain the same cookie.
    print("Step 3.6: Joiner_1 sends subsequent DTLS-ClientHello.")
    pkts.filter_wpan_src64(JOINER_1).\
        filter_wpan_dst64(JOINER_ROUTER).\
        filter(lambda p: p.udp.dstport == JOINER_UDP_PORT).\
        filter(lambda p: field_contains(p.dtls.handshake.type, 1)).\
        filter(lambda p: p.dtls.handshake.cookie == COOKIE).\
        must_next()

    # - 7. The Joiner_Router receives the subsequent DTLS-ClientHello handshake record and sends a RLY_RX.ntf message
    #   to the Commissioner containing:
    #   - CoAP URI-Path: NON POST coap://<C>/c/rx
    #   - CoAP Payload:
    #     - Joiner_1 DTLS Encapsulation TLV: Type = 17 (0x11), Length = Variable (8/16 bit size), Value = subsequent
    #       DTLS-ClientHello handshake record received from Joiner_1.
    #     - Joiner_1 UDP Port TLV: Type = 18 (0x12), Length = 2, Value = UDP port of Joiner_1.
    #     - Joiner_1 IID TLV: Type = 19 (0x13), Length = 8, Value = The IID based on the EUI-64 of Joiner_1.
    #     - Joiner_Router Locator TLV: Type = 20 (0x14), Length = 2, Value = RLOC of Joiner_Router.
    print("Step 3.7: Joiner_Router sends subsequent RLY_RX.ntf.")
    rly_rx_filter = pkts.filter(lambda p: p.wpan.src16 == JOINER_ROUTER_RLOC16).\
        filter(lambda p: p.wpan.dst16 == COMMISSIONER_RLOC16).\
        filter(lambda p: p.udp.dstport == TMF_PORT).\
        filter(lambda p: p.coap.opt.uri_path_recon == consts.RLY_RX_URI)
    verify_rly_rx(rly_rx_filter, JOINER_ROUTER_RLOC16, joiner_port=JOINER_UDP_PORT, joiner_iid=EXPECTED_JOINER_IID)

    # - 8. The Commissioner receives the RLY_RX.ntf message and sends a RLY_TX.ntf message to the Joiner_Router
    #   containing:
    #   - CoAP URI-Path: NON POST coap://<C>/c/tx
    #   - CoAP Payload:
    #     - Joiner_1 DTLS Encapsulation TLV: Type = 17 (0x11), Length = Variable (8/16 bit size), Value =
    #       DTLS-ServerHello, DTLS-ServerKeyExchange and DTLS-ServerHelloDone handshake records, in that order,
    #       destined for Joiner_1.
    #     - Joiner_1 UDP Port TLV: Type = 18 (0x12), Length = 2, Value = UDP port of Joiner_1.
    #     - Joiner_1 IID TLV: Type = 19 (0x13), Length = 8, Value = The IID based on the EUI-64 of Joiner_1.
    #     - Joiner_Router Locator TLV: Type = 20 (0x14), Length = 2, Value = RLOC of Joiner_Router.
    print("Step 3.8: Commissioner sends subsequent RLY_TX.ntf.")
    rly_tx_filter = pkts.filter(lambda p: p.wpan.src16 == COMMISSIONER_RLOC16).\
        filter(lambda p: p.wpan.dst16 == JOINER_ROUTER_RLOC16).\
        filter(lambda p: p.udp.dstport == TMF_PORT).\
        filter(lambda p: p.coap.opt.uri_path_recon == consts.RLY_TX_URI)
    verify_rly_tx(rly_tx_filter, JOINER_ROUTER_RLOC16, joiner_port=JOINER_UDP_PORT, joiner_iid=EXPECTED_JOINER_IID)

    # - 9. The Joiner_Router receives the RLY_TX.ntf message and sends the encapsulated DTLS-ServerHello,
    #   DTLS-ServerKeyExchange and DTLS-ServerHelloDone handshake records in one UDP datagram to Joiner_1.
    print("Step 3.9: Joiner_Router sends DTLS-ServerHello etc. to Joiner_1.")
    pkts.index = (0, 0)
    pkts.filter_wpan_src64(JOINER_ROUTER).\
        filter_wpan_dst64(JOINER_1).\
        filter(lambda p: p.udp.srcport == JOINER_UDP_PORT).\
        filter(lambda p: field_contains(p.dtls.handshake.type, 2)).\
        must_next()

    # - 10. Joiner_1 receives the DTLS-ServerHello, DTLS-ServerKeyExchange and DTLS-ServerHelloDone handshake records
    #   and sends a DTLS-ClientKeyExchange handshake record, a DTLS-ChangeCipherSpec record and an encrypted
    #   DTLS-Finished handshake record, in that order, to Joiner_Router.
    print("Step 3.10: Joiner_1 sends DTLS-ClientKeyExchange etc.")
    pkts.filter_wpan_src64(JOINER_1).\
        filter_wpan_dst64(JOINER_ROUTER).\
        filter(lambda p: p.udp.dstport == JOINER_UDP_PORT).\
        filter(lambda p: field_contains(p.dtls.record.content_type, 20) or\
               field_contains(p.dtls.record.content_type, 22)).\
        must_next()

    # - 11. The Joiner_Router receives the DTLS-ClientKeyExchange handshake record, the DTLS-ChangeCipherSpec record
    #   and the encrypted DTLS-Finished handshake record; it then sends a RLY_RX.ntf message to the Commissioner
    #   containing:
    #   - CoAP URI-Path: NON POST coap://<C>/c/rx
    #   - CoAP Payload:
    #     - Joiner_1 DTLS Encapsulation TLV: Type = 17 (0x11), Length = Variable (8/16 bit size), Value =
    #       DTLS-ClientKeyExchange handshake record, DTLS-ChangeCipherSpec record and encrypted DTLS-Finished
    #       handshake record received from Joiner_1.
    #     - Joiner_1 UDP Port TLV: Type = 18 (0x12), Length = 2, Value = UDP port of Joiner_1.
    #     - Joiner_1 IID TLV: Type = 19 (0x13), Length = 8, Value = The IID based on the EUI-64 of Joiner_1.
    #     - Joiner_Router Locator TLV: Type = 20 (0x14), Length = 2, Value = RLOC of Joiner_Router.
    print("Step 3.11: Joiner_Router sends RLY_RX.ntf (handshake completion).")
    rly_rx_filter = pkts.filter(lambda p: p.wpan.src16 == JOINER_ROUTER_RLOC16).\
        filter(lambda p: p.wpan.dst16 == COMMISSIONER_RLOC16).\
        filter(lambda p: p.udp.dstport == TMF_PORT).\
        filter(lambda p: p.coap.opt.uri_path_recon == consts.RLY_RX_URI)
    verify_rly_rx(rly_rx_filter, JOINER_ROUTER_RLOC16, joiner_port=JOINER_UDP_PORT, joiner_iid=EXPECTED_JOINER_IID)

    # - 12. The Commissioner receives the RLY_RX.ntf message and sends a RLY_TX.ntf message to the Joiner_Router
    #   containing:
    #   - CoAP URI-Path: NON POST coap://<C>/c/tx
    #   - CoAP Payload:
    #     - Joiner_1 DTLS Encapsulation TLV: Type = 17 (0x11), Length = Variable (8/16 bit size), Value = DTLS-Alert
    #       record with error code 40 (handshake failure) or 20 (bad record MAC) destined for Joiner_1.
    #     - Joiner_1 UDP Port TLV: Type = 18 (0x12), Length = 2, Value = UDP port of Joiner_1.
    #     - Joiner_1 IID TLV: Type = 19 (0x13), Length = 8, Value = The IID based on the EUI-64 of Joiner_1.
    #     - Joiner_Router Locator TLV: Type = 20 (0x14), Length = 2, Value = RLOC of Joiner_Router.
    print("Step 3.12: Commissioner sends RLY_TX.ntf (DTLS-Alert).")
    rly_tx_filter = pkts.filter(lambda p: p.wpan.src16 == COMMISSIONER_RLOC16).\
        filter(lambda p: p.wpan.dst16 == JOINER_ROUTER_RLOC16).\
        filter(lambda p: p.udp.dstport == TMF_PORT).\
        filter(lambda p: p.coap.opt.uri_path_recon == consts.RLY_TX_URI)
    verify_rly_tx(rly_tx_filter, JOINER_ROUTER_RLOC16, joiner_port=JOINER_UDP_PORT, joiner_iid=EXPECTED_JOINER_IID)

    # - 13. The Joiner_Router receives the RLY_TX.ntf message and sends the encapsulated DTLS-Alert record with error
    #   code 40 (handshake failure) or 20 (bad record MAC) handshake record in one UDP datagram to Joiner_1.
    print("Step 3.13: Joiner_Router sends DTLS-Alert to Joiner_1.")
    pkts.index = (0, 0)
    expected_alert = pkts.filter_wpan_src64(JOINER_ROUTER).\
        filter_wpan_dst64(JOINER_1).\
        filter(lambda p: p.udp.srcport == JOINER_UDP_PORT).\
        filter(lambda p: field_contains(p.dtls.record.content_type, 21)).\
        filter(lambda p: field_contains(p.dtls.alert_message_desc, 20) or\
               field_contains(p.dtls.alert_message_desc, 40)).\
        must_next()

    # Fail Conditions:
    # - 3. A DTLS-Alert record with a Fatal alert level is sent by either Joiner_1 or the Commissioner.
    # Note: We check for fatal alerts *before* the expected failure alert.
    print("FailCondition 3: Check for unexpected Fatal DTLS alerts.")
    pkts.index = (0, 0)
    pkts.filter(lambda p: hasattr(p, 'dtls')).\
        filter(lambda p: p.number < expected_alert.number).\
        filter(lambda p: field_contains(p.dtls.record.content_type, 21)).\
        filter(lambda p: field_contains(p.dtls.alert_message_level, ALERT_LEVEL_FATAL)).\
        must_not_next()

    # - 4. The Content-Format option in CoAP messages is present and is not application/octet-stream (42).
    print("FailCondition 4: Check for CoAP Content-Format.")
    pkts.index = (0, 0)
    pkts.filter(lambda p: hasattr(p, 'coap')).\
        filter(lambda p: p.coap.opt_content_format is not nullField).\
        filter(lambda p: int(p.coap.opt_content_format) != 42).\
        must_not_next()


if __name__ == '__main__':
    verify_utils.apply_patches()

    json_file = os.path.abspath(sys.argv[1])
    keys_file = json_file.replace('.json', '.keys')

    with open(json_file, 'rt') as f:
        data = json.load(f)

    # Add Joiner port to DECODE_AS entries
    consts.WIRESHARK_DECODE_AS_ENTRIES[f'udp.port=={JOINER_UDP_PORT}'] = 'dtls'
    consts.WIRESHARK_DECODE_AS_ENTRIES[f'dtls.port=={JOINER_UDP_PORT}'] = 'coap'

    wireshark_prefs = consts.WIRESHARK_OVERRIDE_PREFS.copy()
    if os.path.exists(keys_file):
        wireshark_prefs['tls.keylog_file'] = keys_file

    existing_keys_str = wireshark_prefs.get('uat:ieee802154_keys', '')
    keys = []
    for line in existing_keys_str.split('\n'):
        line = line.strip()
        if line:
            keys.append(line)

    network_key = data.get('network_key')
    if network_key:
        new_key = f'"{network_key}","1","Thread hash"'
        if not any(network_key in k for k in keys):
            keys.append(new_key)
    wireshark_prefs['uat:ieee802154_keys'] = '\n'.join(keys)

    mesh_local_prefix = data.get('extra_vars', {}).get('mesh_local_prefix')
    if mesh_local_prefix:
        prefix_addr = mesh_local_prefix.split('/')[0]
        wireshark_prefs['6lowpan.context0'] = f'{prefix_addr}/64'

    try:
        pv = PacketVerifier(json_file, wireshark_prefs=wireshark_prefs)
        pv.add_common_vars()

        for node_id, rloc16 in data.get('rloc16s', {}).items():
            name = pv.test_info.get_node_name(int(node_id))
            pv.add_vars(**{f'{name}_RLOC16': int(rloc16, 16)})

        verify(pv)
        print("Verification PASSED")
    except Exception as e:
        print(f"Verification FAILED: {e}")
        import traceback
        traceback.print_exc()
        sys.exit(1)
