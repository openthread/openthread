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
import struct

# Add the thread-cert directory to sys.path to find pktverify
CUR_DIR = os.path.dirname(os.path.abspath(__file__))
OT_ROOT = os.path.abspath(os.path.join(CUR_DIR, '../..'))
THREAD_CERT_DIR = os.path.join(OT_ROOT, 'tests/scripts/thread-cert')
if THREAD_CERT_DIR not in sys.path:
    sys.path.append(THREAD_CERT_DIR)

from pktverify import consts
from pktverify.packet_verifier import PacketVerifier
from pktverify import utils as pvutils
from pktverify.coap import CoapTlvParser
from pktverify.addrs import Ipv6Addr
from pktverify.bytes import Bytes


# Monkey-patch CoapTlvParser to parse Thread TLVs in CoAP payload
def thread_coap_tlv_parse(t, v):
    kvs = []
    if t == consts.NL_TARGET_EID_TLV and len(v) == 16:
        kvs.append(('target_eid', str(Ipv6Addr(v))))
    elif t == consts.DG_MAC_EXTENDED_ADDRESS_TLV and len(v) == 8:
        kvs.append(('mac_addr', v.hex()))
    elif t == consts.DG_MAC_ADDRESS_TLV and len(v) == 2:
        kvs.append(('rloc16', hex(struct.unpack('>H', v)[0])))
    elif t == consts.NL_MAC_EXTENDED_ADDRESS_TLV and len(v) == 8:
        kvs.append(('mac_addr', v.hex()))
    elif t == consts.NL_ML_EID_TLV and len(v) == 8:
        kvs.append(('ml_eid', Bytes(v).format_hextets()))
    elif t == consts.NL_RLOC16_TLV and len(v) == 2:
        kvs.append(('rloc16', hex(struct.unpack('>H', v)[0])))
    elif t == consts.NL_STATUS_TLV and len(v) == 1:
        kvs.append(('status', str(v[0])))
    elif t == consts.NL_ROUTER_MASK_TLV:
        kvs.append(('router_mask', v.hex()))
    elif t == consts.DG_MAC_COUNTERS_TLV:
        # MAC counters are a list of 4-byte values
        for i in range(0, len(v), 4):
            val = struct.unpack('>I', v[i:i + 4])[0]
            kvs.append(('mac_counter', str(val)))
    elif t == consts.DG_MODE_TLV and len(v) == 1:
        kvs.append(('mode', hex(v[0])))
    elif t == consts.DG_IPV6_ADDRESS_LIST_TLV:
        for i in range(0, len(v), 16):
            kvs.append(('ipv6_address', str(Ipv6Addr(v[i:i + 16]))))
    elif t == consts.DG_LEADER_DATA_TLV and len(v) == 8:
        # Leader data contains Partition ID (4), Weighting (1), Data Version (1), Stable Data Version (1), Leader Router ID (1)
        kvs.append(('partition_id', hex(struct.unpack('>I', v[0:4])[0])))
        kvs.append(('leader_router_id', str(v[7])))
    elif t == consts.DG_ROUTE64_TLV:
        # Route64 contains Router ID Sequence (1), and Router ID Mask (8), then link qualities
        kvs.append(('router_id_sequence', str(v[0])))
        kvs.append(('router_id_mask', v[1:9].hex()))
    elif t == consts.DG_CHILD_TABLE_TLV:
        # Child table contains a list of child entries.
        # Each entry: [Timeout(5 bits), LQI(2 bits), Child ID(9 bits), Mode(8 bits)] -> total 3 bytes
        for i in range(0, len(v), 3):
            if i + 3 <= len(v):
                timeout_child_id = struct.unpack('>H', v[i:i + 2])[0]
                child_id = timeout_child_id & 0x1ff
                mode = v[i + 2]
                kvs.append(('child_id', hex(child_id)))
                kvs.append(('child_mode', hex(mode)))
    elif t == consts.DG_CHANNEL_PAGES_TLV:
        kvs.append(('channel_pages', v.hex()))
    return kvs


def apply_patches():
    CoapTlvParser.parse = staticmethod(thread_coap_tlv_parse)

    from pktverify import layer_fields
    layer_fields._LAYER_FIELDS['coap.tlv.ipv6_address'] = layer_fields._list(layer_fields._ipv6_addr)
    layer_fields._LAYER_FIELDS['coap.tlv.rloc16'] = layer_fields._auto
    layer_fields._LAYER_FIELDS['coap.tlv.mode'] = layer_fields._auto
    layer_fields._LAYER_FIELDS['coap.tlv.leader_router_id'] = layer_fields._auto
    layer_fields._LAYER_FIELDS['coap.tlv.child_id'] = layer_fields._list(layer_fields._auto)
    layer_fields._LAYER_FIELDS['coap.tlv.child_mode'] = layer_fields._list(layer_fields._auto)
    layer_fields._LAYER_FIELDS['coap.tlv.channel_pages'] = layer_fields._bytes

    def which_tshark_patch():
        default_path = '/tmp/thread-wireshark/tshark'
        if os.path.exists(default_path):
            return default_path
        import shutil
        return shutil.which('tshark') or '/usr/local/bin/tshark'

    pvutils.which_tshark = which_tshark_patch


def get_vars(pv, prefix, count, suffix=''):
    return [pv.vars[f'{prefix}_{i}{suffix}'] for i in range(1, count + 1)]


def run_main(verify_func):
    if len(sys.argv) < 2:
        print(f"Usage: python3 {sys.argv[0]} <json_file>")
        sys.exit(1)

    apply_patches()

    json_file = os.path.abspath(sys.argv[1])

    with open(json_file, 'rt') as f:
        data = json.load(f)

    try:
        wireshark_prefs = consts.WIRESHARK_OVERRIDE_PREFS.copy()

        network_key = data.get('network_key')
        if network_key:
            wireshark_prefs['uat:ieee802154_keys'] = f'"{network_key}","1","Thread hash"'

        mesh_local_prefix = data.get('extra_vars', {}).get('mesh_local_prefix')
        if mesh_local_prefix:
            prefix_addr = mesh_local_prefix.split('/')[0]
            wireshark_prefs['6lowpan.context0'] = f'{prefix_addr}/64'
            # Update the Link-Local All Thread Nodes multicast address constant
            # FF32:40:<MeshLocalPrefix>::1
            prefix = Ipv6Addr(prefix_addr)
            all_thread_nodes_mcast_addr = bytearray(Ipv6Addr('ff32:40::1'))
            all_thread_nodes_mcast_addr[4:12] = prefix[0:8]
            consts.LINK_LOCAL_All_THREAD_NODES_MULTICAST_ADDRESS = Ipv6Addr(all_thread_nodes_mcast_addr)

        pv = PacketVerifier(json_file, wireshark_prefs=wireshark_prefs)
        pv.add_common_vars()
        verify_func(pv)
        print("Verification PASSED")
    except Exception as e:
        print(f"Verification FAILED: {e}")
        traceback.print_exc()
        sys.exit(1)
