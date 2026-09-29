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
import struct

# Add the current directory to sys.path to find verify_utils
CUR_DIR = os.path.dirname(os.path.abspath(__file__))
sys.path.append(CUR_DIR)

import verify_utils
from pktverify import consts
from pktverify.null_field import nullField

# Constants
EPSKC_PORT = 1234
ALERT_DESC_CLOSE_NOTIFY = 0
ALERT_LEVEL_FATAL = 2
ALERT_DESC_DECRYPT_ERROR = 51

MESHCOP_SERVICE = '_meshcop._udp.local'
MESHCOP_E_SERVICE = '_meshcop-e._udp.local'


def verify(pv):
    pkts = pv.pkts
    pv.summary.show()

    COMM_1_LL = 'fe80::ff:fe00:2'
    BR_1_LL = 'fe80::ff:fe00:1'
    BR_1_INFRA = 'fe80::ff:fe00:1'

    def get_txt_entries(p):
        entries = {}
        txt_list = []

        # Try to get all values if it's a list
        layer = None
        if hasattr(p, 'mdns') and p.mdns:
            layer = p.mdns
        elif hasattr(p, 'dns') and p.dns:
            layer = p.dns

        if layer:
            txt_list = layer.txt
            if not isinstance(txt_list, list):
                txt_list = [txt_list]

        for txt in txt_list:
            if isinstance(txt, nullField.__class__):
                continue
            # txt is now a Bytes object (hex representation)
            # Find the '=' character (0x3d)
            try:
                eq_idx = txt.index(0x3d)
                key = bytes(txt[:eq_idx]).decode()
                value = txt[eq_idx + 1:]
                entries[key] = value
            except ValueError:
                entries[bytes(txt).decode()] = None

        return entries

    # Step 1: Enable and form network.
    print("Step 1: Enable and form network.")

    # Step 2: Comm_1 discovers BR DUT service.
    print("Step 2: Comm_1 discovers BR DUT service.")
    pkts.filter_ipv6_src(COMM_1_LL).\
        filter_ipv6_dst('ff02::fb').\
        filter(lambda p: p.udp.dstport == 5353).\
        filter(lambda p: MESHCOP_SERVICE in p.mdns.qry.name).\
        must_next()

    mdns_resp = pkts.filter_ipv6_src(BR_1_LL).\
        filter_ipv6_dst('ff02::fb').\
        filter(lambda p: p.udp.dstport == 5353).\
        filter(lambda p: MESHCOP_SERVICE in p.mdns.resp.name).\
        must_next()

    # In the TXT record:
    # - ePSKc Support flag (Bit 11) in State Bitmap (sb) MUST be ‘1’ (ePSKc supported)
    # - ‘vn’ field MUST be present and contain >= 1 characters.
    # - ‘mn’ field MUST be present and contain >= 1 characters.
    txts = get_txt_entries(mdns_resp)
    assert 'sb' in txts, "sb field missing in TXT record"
    sb = struct.unpack('>I', txts['sb'])[0]
    assert (sb >> 11) & 1 == 1, f"ePSKc support flag (bit 11) in sb is not 1 (sb={sb:08x})"
    assert 'vn' in txts and len(txts['vn']) >= 1, "vn field missing or empty in TXT record"
    assert 'mn' in txts and len(txts['mn']) >= 1, "mn field missing or empty in TXT record"

    # Step 3 & 4: Passcode generation and validation.
    print("Step 3 & 4: Passcode generation and validation.")
    # (Validated by C++ test)

    # Step 5: Comm_1 discovers meshcop-e service.
    print("Step 5: Comm_1 discovers meshcop-e service.")
    pkts.filter_ipv6_src(COMM_1_LL).\
        filter_ipv6_dst('ff02::fb').\
        filter(lambda p: p.udp.dstport == 5353).\
        filter(lambda p: MESHCOP_E_SERVICE in p.mdns.qry.name).\
        must_next()

    mdns_resp = pkts.filter_ipv6_src(BR_1_LL).\
        filter_ipv6_dst('ff02::fb').\
        filter(lambda p: p.udp.dstport == 5353).\
        filter(lambda p: MESHCOP_E_SERVICE in p.mdns.resp.name).\
        must_next()

    port_step5 = mdns_resp.mdns.srv.port[0]
    print(f"Step 5 Discovered port: {port_step5}")

    # Step 6: Comm_1 connects with incorrect ePSKc.
    print("Step 6: Comm_1 connects with incorrect ePSKc.")
    pkts.filter_ipv6_src(COMM_1_LL).\
        filter_ipv6_dst(BR_1_INFRA).\
        filter(lambda p: p.udp.dstport == port_step5).\
        filter(lambda p: consts.CONTENT_HANDSHAKE in p.dtls.record.content_type).\
        filter(lambda p: consts.HANDSHAKE_CLIENT_HELLO in p.dtls.handshake.type).\
        filter(lambda p: p.dtls.handshake.cookie is not nullField).\
        must_next()

    pkts.filter_ipv6_src(BR_1_INFRA).\
        filter_ipv6_dst(COMM_1_LL).\
        filter(lambda p: p.udp.srcport == port_step5).\
        filter(lambda p: consts.CONTENT_ALERT in p.dtls.record.content_type).\
        must_next()

    # Step 7: Comm_1 connects with correct ePSKc.
    print("Step 7: Comm_1 connects with correct ePSKc.")
    pkts.filter_ipv6_src(COMM_1_LL).\
        filter_ipv6_dst(BR_1_INFRA).\
        filter(lambda p: p.udp.dstport == port_step5).\
        filter(lambda p: consts.CONTENT_HANDSHAKE in p.dtls.record.content_type).\
        filter(lambda p: consts.HANDSHAKE_CLIENT_HELLO in p.dtls.handshake.type).\
        filter(lambda p: p.dtls.handshake.cookie is not nullField).\
        must_next()

    pkts.filter_ipv6_src(BR_1_INFRA).\
        filter_ipv6_dst(COMM_1_LL).\
        filter(lambda p: p.udp.srcport == port_step5).\
        filter(lambda p: consts.CONTENT_HANDSHAKE in p.dtls.record.content_type).\
        filter(lambda p: consts.HANDSHAKE_FINISHED in p.dtls.handshake.type).\
        must_next()

    # Step 8: Comm_1 sends MGMT_ACTIVE_GET.req.
    print("Step 8: Comm_1 sends MGMT_ACTIVE_GET.req.")
    pkts.filter_ipv6_src(COMM_1_LL).\
        filter_ipv6_dst(BR_1_INFRA).\
        filter(lambda p: p.udp.dstport == port_step5).\
        filter(lambda p: p.coap.opt.uri_path == ['c', 'ag']).\
        must_next()

    active_get_rsp = pkts.filter_ipv6_src(BR_1_INFRA).\
        filter_ipv6_dst(COMM_1_LL).\
        filter(lambda p: p.udp.srcport == port_step5).\
        filter(lambda p: p.coap.type == 2).\
        must_next()

    # Response payload MUST contain Network Key, Ext PAN ID, Network Name, PAN ID, PSKc, Security Policy,
    # Active Timestamp, Mesh-Local Prefix, Channel, Channel Mask.
    # Note: pktverify uses 'master_key' for Network Key TLV.
    active_get_rsp.must_verify(lambda p: all(
        p.coap.tlv.has(field) for field in [
            'network_key',
            'ext_pan_id',
            'network_name',
            'pan_id',
            'pskc',
            'security_policy',
            'active_timestamp',
            'mesh_local_prefix',
            'channel',
            'channel_mask',
        ]))

    # Step 9: Comm_1 sends MGMT_PENDING_GET.req.
    print("Step 9: Comm_1 sends MGMT_PENDING_GET.req.")
    pkts.filter_ipv6_src(COMM_1_LL).\
        filter_ipv6_dst(BR_1_INFRA).\
        filter(lambda p: p.udp.dstport == port_step5).\
        filter(lambda p: p.coap.opt.uri_path == ['c', 'pg']).\
        must_next()

    pending_get_rsp = pkts.filter_ipv6_src(BR_1_INFRA).\
        filter_ipv6_dst(COMM_1_LL).\
        filter(lambda p: p.udp.srcport == port_step5).\
        filter(lambda p: p.coap.type == 2).\
        must_next()

    # Response CoAP payload MUST be empty.
    assert pending_get_rsp.coap.payload is nullField, "MGMT_PENDING_GET response payload is not empty"

    # Step 10: Comm_1 closes the DTLS connection.
    print("Step 10: Comm_1 closes the DTLS connection.")
    pkts.filter_ipv6_src(COMM_1_LL).\
        filter_ipv6_dst(BR_1_INFRA).\
        filter(lambda p: p.udp.dstport == port_step5).\
        filter(lambda p: consts.CONTENT_ALERT in p.dtls.record.content_type).\
        must_next()

    # Step 11: Comm_1 connects again (should fail).
    print("Step 11: Comm_1 connects again (should fail).")
    pkts.filter_ipv6_src(COMM_1_LL).\
        filter_ipv6_dst(BR_1_INFRA).\
        filter(lambda p: p.udp.dstport == port_step5).\
        filter(lambda p: consts.CONTENT_HANDSHAKE in p.dtls.record.content_type).\
        filter(lambda p: consts.HANDSHAKE_CLIENT_HELLO in p.dtls.handshake.type).\
        must_next()

    # The DUT may either send an ALERT or simply not respond (since ephemeral key is stopped).
    # We check if there's an ALERT, but don't fail if there isn't one as long as connection doesn't succeed.
    pkts.copy().filter_ipv6_src(BR_1_INFRA).\
        filter_ipv6_dst(COMM_1_LL).\
        filter(lambda p: p.udp.srcport == port_step5).\
        filter(lambda p: consts.CONTENT_HANDSHAKE in p.dtls.record.content_type).\
        filter(lambda p: consts.HANDSHAKE_FINISHED in p.dtls.handshake.type).\
        must_not_next()

    # Step 13: Comm_1 discovers meshcop-e service (should fail).
    print("Step 13: Comm_1 discovers meshcop-e service (should fail).")
    pkts.filter_ipv6_src(COMM_1_LL).\
        filter_ipv6_dst('ff02::fb').\
        filter(lambda p: p.udp.dstport == 5353).\
        filter(lambda p: MESHCOP_E_SERVICE in p.mdns.qry.name).\
        must_next()

    # Step 16: Comm_1 discovers meshcop-e service.
    print("Step 16: Comm_1 discovers meshcop-e service.")
    pkts.filter_ipv6_src(COMM_1_LL).\
        filter_ipv6_dst('ff02::fb').\
        filter(lambda p: p.udp.dstport == 5353).\
        filter(lambda p: MESHCOP_E_SERVICE in p.mdns.qry.name).\
        must_next()

    mdns_resp = pkts.filter_ipv6_src(BR_1_LL).\
        filter_ipv6_dst('ff02::fb').\
        filter(lambda p: p.udp.dstport == 5353).\
        filter(lambda p: MESHCOP_E_SERVICE in p.mdns.resp.name).\
        must_next()

    port_step16 = mdns_resp.mdns.srv.port[0]
    print(f"Step 16 Discovered port: {port_step16}")

    # Step 17: Comm_1 connects with correct ePSKc.
    print("Step 17: Comm_1 connects with correct ePSKc.")
    pkts.filter_ipv6_src(COMM_1_LL).\
        filter_ipv6_dst(BR_1_INFRA).\
        filter(lambda p: p.udp.dstport == port_step16).\
        filter(lambda p: consts.CONTENT_HANDSHAKE in p.dtls.record.content_type).\
        filter(lambda p: consts.HANDSHAKE_CLIENT_HELLO in p.dtls.handshake.type).\
        filter(lambda p: p.dtls.handshake.cookie is not nullField).\
        must_next()

    pkts.filter_ipv6_src(BR_1_INFRA).\
        filter_ipv6_dst(COMM_1_LL).\
        filter(lambda p: p.udp.srcport == port_step16).\
        filter(lambda p: consts.CONTENT_HANDSHAKE in p.dtls.record.content_type).\
        filter(lambda p: consts.HANDSHAKE_FINISHED in p.dtls.handshake.type).\
        must_next()

    # Step 18: Comm_1 sends MGMT_ACTIVE_GET.req.
    print("Step 18: Comm_1 sends MGMT_ACTIVE_GET.req.")
    pkts.filter_ipv6_src(COMM_1_LL).\
        filter_ipv6_dst(BR_1_INFRA).\
        filter(lambda p: p.udp.dstport == port_step16).\
        filter(lambda p: p.coap.opt.uri_path == ['c', 'ag']).\
        must_next()

    active_get_rsp = pkts.filter_ipv6_src(BR_1_INFRA).\
        filter_ipv6_dst(COMM_1_LL).\
        filter(lambda p: p.udp.srcport == port_step16).\
        filter(lambda p: p.coap.type == 2).\
        must_next()

    active_get_rsp.must_verify(lambda p: all(
        p.coap.tlv.has(field) for field in [
            'network_key',
            'ext_pan_id',
            'network_name',
            'pan_id',
            'pskc',
            'security_policy',
            'active_timestamp',
            'mesh_local_prefix',
            'channel',
            'channel_mask',
        ]))

    # Step 19: Comm_1 closes the DTLS connection.
    print("Step 19: Comm_1 closes the DTLS connection.")
    pkts.filter_ipv6_src(COMM_1_LL).\
        filter_ipv6_dst(BR_1_INFRA).\
        filter(lambda p: p.udp.dstport == port_step16).\
        filter(lambda p: consts.CONTENT_ALERT in p.dtls.record.content_type).\
        must_next()

    # Step 22: Comm_1 discovers meshcop-e service.
    print("Step 22: Comm_1 discovers meshcop-e service.")
    pkts.filter_ipv6_src(COMM_1_LL).\
        filter_ipv6_dst('ff02::fb').\
        filter(lambda p: p.udp.dstport == 5353).\
        filter(lambda p: MESHCOP_E_SERVICE in p.mdns.qry.name).\
        must_next()

    mdns_resp = pkts.filter_ipv6_src(BR_1_LL).\
        filter_ipv6_dst('ff02::fb').\
        filter(lambda p: p.udp.dstport == 5353).\
        filter(lambda p: MESHCOP_E_SERVICE in p.mdns.resp.name).\
        must_next()

    port_step22 = mdns_resp.mdns.srv.port[0]
    print(f"Step 22 Discovered port: {port_step22}")

    # Step 23: Comm_1 connects with incorrect ePSKc 10 times.
    print("Step 23: Comm_1 connects with incorrect ePSKc 10 times.")
    for i in range(10):
        pkts.filter_ipv6_src(COMM_1_LL).\
            filter_ipv6_dst(BR_1_INFRA).\
            filter(lambda p: p.udp.dstport == port_step22).\
            filter(lambda p: consts.CONTENT_HANDSHAKE in p.dtls.record.content_type).\
            filter(lambda p: consts.HANDSHAKE_CLIENT_HELLO in p.dtls.handshake.type).\
            filter(lambda p: p.dtls.handshake.cookie is not nullField).\
            must_next()

        pkts.filter_ipv6_src(BR_1_INFRA).\
            filter_ipv6_dst(COMM_1_LL).\
            filter(lambda p: p.udp.srcport == port_step22).\
            filter(lambda p: consts.CONTENT_ALERT in p.dtls.record.content_type).\
            must_next()

    # Step 24: Comm_1 connects with correct ePSKc (should fail).
    print("Step 24: Comm_1 connects with correct ePSKc (should fail).")
    pkts.filter_ipv6_src(COMM_1_LL).\
        filter_ipv6_dst(BR_1_INFRA).\
        filter(lambda p: p.udp.dstport == port_step22).\
        filter(lambda p: consts.CONTENT_HANDSHAKE in p.dtls.record.content_type).\
        filter(lambda p: consts.HANDSHAKE_CLIENT_HELLO in p.dtls.handshake.type).\
        must_next()

    # Similar to Step 11, the DUT may either send an ALERT or not respond.
    pkts.copy().filter_ipv6_src(BR_1_INFRA).\
        filter_ipv6_dst(COMM_1_LL).\
        filter(lambda p: p.udp.srcport == port_step22).\
        filter(lambda p: consts.CONTENT_HANDSHAKE in p.dtls.record.content_type).\
        filter(lambda p: consts.HANDSHAKE_FINISHED in p.dtls.handshake.type).\
        must_not_next()

    # Step 26: Comm_1 discovers meshcop-e service (should fail).
    print("Step 26: Comm_1 discovers meshcop-e service (should fail).")
    pkts.filter_ipv6_src(COMM_1_LL).\
        filter_ipv6_dst('ff02::fb').\
        filter(lambda p: p.udp.dstport == 5353).\
        filter(lambda p: MESHCOP_E_SERVICE in p.mdns.qry.name).\
        must_next()


if __name__ == '__main__':
    # Add keylog file to Wireshark preferences if it exists.
    keylog_file = '/tmp/test_1_4_CS_TC_3.keys'
    if os.path.exists(keylog_file):
        consts.WIRESHARK_OVERRIDE_PREFS['tls.keylog_file'] = keylog_file
        for port in [1234, 1235, 1236]:
            consts.WIRESHARK_DECODE_AS_ENTRIES[f'udp.port=={port}'] = 'dtls'
            consts.WIRESHARK_DECODE_AS_ENTRIES[f'dtls.port=={port}'] = 'coap'
    verify_utils.run_main(verify)
