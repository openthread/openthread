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

JOIN_FIN_URI = '/c/jf'
JOIN_ENT_URI = '/c/je'

# Constants for ports to avoid magic numbers
DEFAULT_JOINER_PORT = 49153
TMF_PORT = 61631
NM_PROVISIONING_URL_TLV = 32


def verify_rly_rx(p, router_rloc16, leader_rloc16, joiner_port=None, joiner_iid=None):
    pkt = p.must_next()
    assert pkt.coap.opt.uri_path_recon == consts.RLY_RX_URI, f"Expected URI {consts.RLY_RX_URI}, got {pkt.coap.opt.uri_path_recon}"
    assert pkt.coap.tlv.joiner_dtls_encap, "joiner_dtls_encap missing"
    if joiner_port is not None:
        assert pkt.coap.tlv.joiner_udp_port == joiner_port, f"Expected port {joiner_port}, got {pkt.coap.tlv.joiner_udp_port}"
    if joiner_iid is not None:
        assert pkt.coap.tlv.joiner_iid == joiner_iid, f"Expected IID {joiner_iid}, got {pkt.coap.tlv.joiner_iid}"
    assert pkt.coap.tlv.joiner_router_locator == router_rloc16, f"Expected locator {router_rloc16}, got {pkt.coap.tlv.joiner_router_locator}"
    return pkt


def verify_rly_tx(p, leader_rloc16, router_rloc16, joiner_port=None, joiner_iid=None, joiner_router_kek=False):
    pkt = p.must_next()
    assert pkt.coap.opt.uri_path_recon == consts.RLY_TX_URI, f"Expected URI {consts.RLY_TX_URI}, got {pkt.coap.opt.uri_path_recon}"
    assert pkt.coap.tlv.joiner_dtls_encap, "joiner_dtls_encap missing"
    if joiner_port is not None:
        assert pkt.coap.tlv.joiner_udp_port == joiner_port, f"Expected port {joiner_port}, got {pkt.coap.tlv.joiner_udp_port}"
    if joiner_iid is not None:
        assert pkt.coap.tlv.joiner_iid == joiner_iid, f"Expected IID {joiner_iid}, got {pkt.coap.tlv.joiner_iid}"
    assert pkt.coap.tlv.joiner_router_locator == router_rloc16, f"Expected locator {router_rloc16}, got {pkt.coap.tlv.joiner_router_locator}"
    if joiner_router_kek:
        assert pkt.coap.tlv.joiner_router_kek, "joiner_router_kek missing"
    return pkt


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


def verify(pv):
    # 8.2.1 On Mesh Commissioner Joining with JR, any commissioner, single (correct)
    #
    # 8.2.1.1 Topology
    # - Commissioner (Leader)
    # - Joiner Router
    # - Joiner_1
    #
    # 8.2.1.2 Purpose & Description
    # The purpose of this test case is to verify that the DUT sends and receives relayed DTLS traffic correctly when
    #   the correct PSKd is used by the Joiner. It also verifies that the DUT (as a Joiner Router) sends JOIN_ENT.ntf
    #   encrypted with KEK.
    #
    # Spec Reference | V1.1 Section  | V1.3.0 Section
    # ---------------|---------------|---------------
    # Relay traffic  | 8.4.3 / 8.4.4 | 8.4.3 / 8.4.4

    pkts = pv.pkts
    pv.summary.show()

    # Fail Condition 3: A DTLS-Alert record with a Fatal alert level is sent by either Joiner_1 or the Commissioner.
    for p in pkts.filter(lambda p: 'dtls' in p and\
                         hasattr(p.dtls, 'record_content_type') and\
                         field_contains(p.dtls.record_content_type, 21)):
        if hasattr(p.dtls, 'alert_message_level'):
            levels = p.dtls.alert_message_level
            if hasattr(levels, 'all_fields'):
                levels = [f.get_default_value() for f in levels.all_fields]
            else:
                levels = [str(levels)]

            for level in levels:
                if level == '2':
                    raise ValueError(f"Fatal DTLS Alert found in packet {p.number}")

    # Fail Condition 4: The Content-Format option in CoAP messages is present and is not application/octet-stream (42).
    for p in pkts.filter(lambda p: 'coap' in p):
        if hasattr(p.coap, 'opt_content_format'):
            fmt = str(p.coap.opt_content_format)
            if fmt != 'nullField' and fmt != '42':
                raise ValueError(f"Invalid CoAP Content-Format {fmt} in packet {p.number}")

    LEADER = pv.vars['LEADER']
    ROUTER = pv.vars['ROUTER']
    LEADER_RLOC16 = pv.vars['LEADER_RLOC16']
    ROUTER_RLOC16 = pv.vars['ROUTER_RLOC16']

    # Step 3.1: Discover Joiner Port from MLE Discovery Response
    print("Step 3.1: Discover Joiner Port from MLE Discovery Response")
    discovery_rsp = pkts.filter_wpan_src64(LEADER).\
        filter_mle_cmd(consts.MLE_DISCOVERY_RESPONSE).\
        must_next()
    JOINER_PORTS = discovery_rsp.thread_meshcop.tlv.udp_port
    if not isinstance(JOINER_PORTS, list):
        JOINER_PORTS = [JOINER_PORTS]
    JOINER_PORTS = [int(p) for p in JOINER_PORTS]
    # We expect 49153 to be one of the ports
    assert DEFAULT_JOINER_PORT in JOINER_PORTS, f"Expected {DEFAULT_JOINER_PORT} in {JOINER_PORTS}"
    JOINER_PORT = DEFAULT_JOINER_PORT
    pv.add_vars(JOINER_PORT=JOINER_PORT)

    # Find the Joiner by its initial ClientHello
    print("Finding Joiner from initial DTLS ClientHello")
    initial_hello_pkt = pkts.filter(lambda p: p.udp.dstport == JOINER_PORT or p.udp.srcport == JOINER_PORT).\
        must_next()
    JOINER = initial_hello_pkt.wpan.src64
    pv.add_vars(JOINER=JOINER)

    # Step 3.2: Joiner_1 sends an initial DTLS-ClientHello
    print("Step 3.2: Joiner_1 sends an initial DTLS-ClientHello")
    pkts.index = (0, 0)
    pkts.filter_wpan_src64(JOINER).\
        filter_wpan_dst64(ROUTER).\
        filter(lambda p: p.udp.dstport == JOINER_PORT).\
        filter(lambda p: field_contains(p.dtls.handshake.type, 1)).\
        must_next()

    # Step 3.3: Joiner_Router receives the initial DTLS-ClientHello record and sends a RLY_RX.ntf message
    print("Step 3.3: Joiner_Router sends RLY_RX.ntf to Commissioner")
    rly_rx_filter = pkts.filter('wpan.src16 == {ROUTER_RLOC16}', ROUTER_RLOC16=ROUTER_RLOC16).\
        filter('wpan.dst16 == {LEADER_RLOC16}', LEADER_RLOC16=LEADER_RLOC16).\
        filter(lambda p: p.udp.dstport == TMF_PORT).\
        filter(lambda p: p.coap.opt.uri_path_recon == consts.RLY_RX_URI)
    rly_rx_pkt = verify_rly_rx(rly_rx_filter, ROUTER_RLOC16, LEADER_RLOC16, joiner_port=JOINER_PORT)
    JOINER_IID = rly_rx_pkt.coap.tlv.joiner_iid
    pv.add_vars(JOINER_IID=JOINER_IID)

    # Step 3.4: The Commissioner receives the RLY_RX.ntf message and sends a RLY_TX.ntf message
    print("Step 3.4: Commissioner sends RLY_TX.ntf to Joiner_Router")
    rly_tx = pkts.filter('wpan.src16 == {LEADER_RLOC16}', LEADER_RLOC16=LEADER_RLOC16).\
        filter('wpan.dst16 == {ROUTER_RLOC16}', ROUTER_RLOC16=ROUTER_RLOC16).\
        filter(lambda p: p.udp.dstport == TMF_PORT).\
        filter(lambda p: p.coap.opt.uri_path_recon == consts.RLY_TX_URI)
    verify_rly_tx(rly_tx, LEADER_RLOC16, ROUTER_RLOC16, joiner_port=JOINER_PORT, joiner_iid=JOINER_IID)

    # Step 3.5: The Joiner_Router receives the RLY_TX.ntf message and sends the DTLS-HelloVerifyRequest
    print("Step 3.5: Joiner_Router sends DTLS-HelloVerifyRequest to Joiner_1")
    pkts.index = (0, 0)
    hvr_pkt = pkts.filter_wpan_src64(ROUTER).\
        filter_wpan_dst64(JOINER).\
        filter(lambda p: p.udp.srcport == JOINER_PORT).\
        filter(lambda p: field_contains(p.dtls.handshake.type, 3)).\
        must_next()
    COOKIE = hvr_pkt.dtls.handshake.cookie
    pv.add_vars(COOKIE=COOKIE)

    # Step 3.6: Joiner_1 receives the DTLS-HelloVerifyRequest and sends subsequent DTLS-ClientHello
    print("Step 3.6: Joiner_1 sends subsequent DTLS-ClientHello")
    pkts.filter_wpan_src64(JOINER).\
        filter_wpan_dst64(ROUTER).\
        filter(lambda p: p.udp.dstport == JOINER_PORT).\
        filter(lambda p: field_contains(p.dtls.handshake.type, 1) and\
               p.dtls.handshake.cookie == COOKIE).\
        must_next()

    # Step 3.7: The Joiner_Router receives the subsequent DTLS-ClientHello and sends a RLY_RX.ntf
    print("Step 3.7: Joiner_Router sends subsequent RLY_RX.ntf")
    rly_rx = pkts.filter('wpan.src16 == {ROUTER_RLOC16}', ROUTER_RLOC16=ROUTER_RLOC16).\
        filter('wpan.dst16 == {LEADER_RLOC16}', LEADER_RLOC16=LEADER_RLOC16).\
        filter(lambda p: p.udp.dstport == TMF_PORT).\
        filter(lambda p: p.coap.opt.uri_path_recon == consts.RLY_RX_URI)
    verify_rly_rx(rly_rx, ROUTER_RLOC16, LEADER_RLOC16, joiner_port=JOINER_PORT, joiner_iid=JOINER_IID)

    # Step 3.8: The Commissioner receives the RLY_RX.ntf and sends a RLY_TX.ntf (Server Hello etc.)
    print("Step 3.8: Commissioner sends subsequent RLY_TX.ntf")
    rly_tx = pkts.filter('wpan.src16 == {LEADER_RLOC16}', LEADER_RLOC16=LEADER_RLOC16).\
        filter('wpan.dst16 == {ROUTER_RLOC16}', ROUTER_RLOC16=ROUTER_RLOC16).\
        filter(lambda p: p.udp.dstport == TMF_PORT).\
        filter(lambda p: p.coap.opt.uri_path_recon == consts.RLY_TX_URI)
    verify_rly_tx(rly_tx, LEADER_RLOC16, ROUTER_RLOC16, joiner_port=JOINER_PORT, joiner_iid=JOINER_IID)

    # Step 3.9: Joiner_Router receives the RLY_TX.ntf and sends DTLS-ServerHello etc. to Joiner_1
    print("Step 3.9: Joiner_Router sends DTLS-ServerHello etc. to Joiner_1")
    pkts.index = (0, 0)
    pkts.filter_wpan_src64(ROUTER).\
        filter_wpan_dst64(JOINER).\
        filter(lambda p: p.udp.srcport == JOINER_PORT).\
        filter(lambda p: field_contains(p.dtls.handshake.type, 2)).\
        must_next()

    # Step 3.10: Joiner_1 receives the DTLS-ServerHello etc. and sends DTLS-ClientKeyExchange etc.
    print("Step 3.10: Joiner_1 sends DTLS-ClientKeyExchange etc.")
    pkts.filter_wpan_src64(JOINER).\
        filter_wpan_dst64(ROUTER).\
        filter(lambda p: p.udp.dstport == JOINER_PORT).\
        filter(lambda p: field_contains(p.dtls.record.content_type, 20) or\
               field_contains(p.dtls.record.content_type, 22)).\
        must_next()

    # Step 3.11: Joiner_Router receives the DTLS records and sends a RLY_RX.ntf message
    print("Step 3.11: Joiner_Router sends RLY_RX.ntf (handshake completion)")
    rly_rx = pkts.filter('wpan.src16 == {ROUTER_RLOC16}', ROUTER_RLOC16=ROUTER_RLOC16).\
        filter('wpan.dst16 == {LEADER_RLOC16}', LEADER_RLOC16=LEADER_RLOC16).\
        filter(lambda p: p.udp.dstport == TMF_PORT).\
        filter(lambda p: p.coap.opt.uri_path_recon == consts.RLY_RX_URI)
    verify_rly_rx(rly_rx, ROUTER_RLOC16, LEADER_RLOC16, joiner_port=JOINER_PORT, joiner_iid=JOINER_IID)

    # Step 3.12: The Commissioner receives the RLY_RX.ntf and sends a RLY_TX.ntf
    print("Step 3.12: Commissioner sends RLY_TX.ntf (handshake completion)")
    rly_tx = pkts.filter('wpan.src16 == {LEADER_RLOC16}', LEADER_RLOC16=LEADER_RLOC16).\
        filter('wpan.dst16 == {ROUTER_RLOC16}', ROUTER_RLOC16=ROUTER_RLOC16).\
        filter(lambda p: p.udp.dstport == TMF_PORT).\
        filter(lambda p: p.coap.opt.uri_path_recon == consts.RLY_TX_URI)
    verify_rly_tx(rly_tx, LEADER_RLOC16, ROUTER_RLOC16, joiner_port=JOINER_PORT, joiner_iid=JOINER_IID)

    # Step 3.13: Joiner_Router receives the RLY_TX.ntf and sends DTLS-ChangeCipherSpec etc. to Joiner_1
    print("Step 3.13: Joiner_Router sends DTLS-ChangeCipherSpec etc. to Joiner_1")
    pkts.index = (0, 0)
    pkts.filter_wpan_src64(ROUTER).\
        filter_wpan_dst64(JOINER).\
        filter(lambda p: p.udp.srcport == JOINER_PORT).\
        filter(lambda p: field_contains(p.dtls.record.content_type, 20) or\
               field_contains(p.dtls.record.content_type, 22)).\
        must_next()

    # Step 3.14: Joiner_1 receives the records and sends JOIN_FIN.req in DTLS-ApplicationData
    print("Step 3.14: Joiner_1 sends JOIN_FIN.req")
    pkt = pkts.filter_wpan_src64(JOINER).\
        filter_wpan_dst64(ROUTER).\
        filter(lambda p: p.udp.dstport == JOINER_PORT).\
        filter(lambda p: field_contains(p.dtls.record.content_type, 23)).\
        must_next()
    # Verify JOIN_FIN.req does not contain Provisioning URL TLV
    assert pkt.coap.opt.uri_path_recon == JOIN_FIN_URI, f"Expected URI {JOIN_FIN_URI}, got {pkt.coap.opt.uri_path_recon}"
    assert NM_PROVISIONING_URL_TLV not in pkt.coap.tlv.type, "JOIN_FIN.req contains Provisioning URL TLV"

    # Step 3.15: Joiner_Router receives the record and sends a RLY_RX.ntf message
    print("Step 3.15: Joiner_Router sends RLY_RX.ntf (JOIN_FIN.req)")
    rly_rx = pkts.filter('wpan.src16 == {ROUTER_RLOC16}', ROUTER_RLOC16=ROUTER_RLOC16).\
        filter('wpan.dst16 == {LEADER_RLOC16}', LEADER_RLOC16=LEADER_RLOC16).\
        filter(lambda p: p.udp.dstport == TMF_PORT).\
        filter(lambda p: p.coap.opt.uri_path_recon == consts.RLY_RX_URI)
    verify_rly_rx(rly_rx, ROUTER_RLOC16, LEADER_RLOC16, joiner_port=JOINER_PORT, joiner_iid=JOINER_IID)

    # Step 3.16: The Commissioner receives the RLY_RX.ntf and sends a RLY_TX.ntf (JOIN_FIN.rsp)
    print("Step 3.16: Commissioner sends RLY_TX.ntf (JOIN_FIN.rsp)")
    rly_tx = pkts.filter('wpan.src16 == {LEADER_RLOC16}', LEADER_RLOC16=LEADER_RLOC16).\
        filter('wpan.dst16 == {ROUTER_RLOC16}', ROUTER_RLOC16=ROUTER_RLOC16).\
        filter(lambda p: p.udp.dstport == TMF_PORT).\
        filter(lambda p: p.coap.opt.uri_path_recon == consts.RLY_TX_URI)
    verify_rly_tx(rly_tx,
                  LEADER_RLOC16,
                  ROUTER_RLOC16,
                  joiner_port=JOINER_PORT,
                  joiner_iid=JOINER_IID,
                  joiner_router_kek=True)

    # Step 3.17: The Joiner_Router receives the RLY_TX.ntf message and sends a JOIN_FIN.rsp
    print("Step 3.17: Joiner_Router sends JOIN_FIN.rsp to Joiner_1")
    pkts.filter_wpan_src64(ROUTER).\
        filter_wpan_dst64(JOINER).\
        filter(lambda p: p.udp.srcport == JOINER_PORT).\
        filter(lambda p: field_contains(p.dtls.record.content_type, 23)).\
        must_next()

    # Step 3.18: The Joiner_Router sends an encrypted JOIN_ENT.ntf message to Joiner_1.
    print("Step 3.18: Joiner_Router sends encrypted JOIN_ENT.ntf")
    pkts.index = (0, 0)
    pkts.filter_wpan_src64(ROUTER).\
        filter_wpan_dst64(JOINER).\
        filter(lambda p: p.wpan.security == True).\
        must_next()

    # Step 3.19: Joiner_1 receives the JOIN_ENT.ntf and sends response
    print("Step 3.19: Joiner_1 sends JOIN_ENT.ntf response")
    pkts.filter_wpan_src64(JOINER).\
        filter_wpan_dst64(ROUTER).\
        filter(lambda p: p.wpan.security == True).\
        must_next()

    # Step 3.20: Joiner_1 sends an encrypted DTLS-Alert record
    print("Step 3.20: Joiner_1 sends DTLS-Alert (close_notify)")
    pkts.index = (0, 0)
    pkts.filter_wpan_src64(JOINER).\
        filter_wpan_dst64(ROUTER).\
        filter(lambda p: p.udp.dstport == JOINER_PORT).\
        filter(lambda p: field_contains(p.dtls.record.content_type, 21)).\
        filter(lambda p: field_contains(p.dtls.alert_message.desc, 0)).\
        must_next()

    # Step 3.21: The Joiner_Router receives the record and sends a RLY_RX.ntf message
    print("Step 3.21: Joiner_Router sends RLY_RX.ntf (DTLS-Alert)")
    rly_rx = pkts.filter('wpan.src16 == {ROUTER_RLOC16}', ROUTER_RLOC16=ROUTER_RLOC16).\
        filter('wpan.dst16 == {LEADER_RLOC16}', LEADER_RLOC16=LEADER_RLOC16).\
        filter(lambda p: p.udp.dstport == TMF_PORT).\
        filter(lambda p: p.coap.opt.uri_path_recon == consts.RLY_RX_URI)
    verify_rly_rx(rly_rx, ROUTER_RLOC16, LEADER_RLOC16, joiner_port=JOINER_PORT, joiner_iid=JOINER_IID)

    # Step 3.22: The Commissioner receives the RLY_RX.ntf and sends a RLY_TX.ntf
    print("Step 3.22: Commissioner sends RLY_TX.ntf (DTLS-Alert)")
    rly_tx = pkts.filter('wpan.src16 == {LEADER_RLOC16}', LEADER_RLOC16=LEADER_RLOC16).\
        filter('wpan.dst16 == {ROUTER_RLOC16}', ROUTER_RLOC16=ROUTER_RLOC16).\
        filter(lambda p: p.udp.dstport == TMF_PORT).\
        filter(lambda p: p.coap.opt.uri_path_recon == consts.RLY_TX_URI)
    verify_rly_tx(rly_tx, LEADER_RLOC16, ROUTER_RLOC16, joiner_port=JOINER_PORT, joiner_iid=JOINER_IID)

    # Step 3.23: The Joiner_Router receives the RLY_TX.ntf and sends the DTLS-Alert to Joiner_1
    print("Step 3.23: Joiner_Router sends DTLS-Alert to Joiner_1")
    pkts.filter_wpan_src64(ROUTER).\
        filter_wpan_dst64(JOINER).\
        filter(lambda p: p.udp.srcport == JOINER_PORT).\
        filter(lambda p: field_contains(p.dtls.record.content_type, 21)).\
        filter(lambda p: field_contains(p.dtls.alert_message.desc, 0)).\
        must_next()


if __name__ == '__main__':
    verify_utils.apply_patches()

    json_file = os.path.abspath(sys.argv[1])
    keys_file = json_file.replace('.json', '.keys')

    with open(json_file, 'rt') as f:
        data = json.load(f)

    # Add Joiner port to DECODE_AS entries
    consts.WIRESHARK_DECODE_AS_ENTRIES[f'udp.port=={DEFAULT_JOINER_PORT}'] = 'dtls'
    consts.WIRESHARK_DECODE_AS_ENTRIES[f'dtls.port=={DEFAULT_JOINER_PORT}'] = 'coap'

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
