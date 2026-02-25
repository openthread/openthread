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
def thread_coap_tlv_parse(t, v, layer=None):
    kvs = []

    uri_path = None
    if layer is not None:
        uri_path = layer.uri_path

    # If the URI starts with '/d/', it is likely a Diagnostic message.
    # Otherwise, we assume it's MeshCoP or other Thread TLVs.
    is_diag = uri_path is not None and uri_path.startswith('/d/')

    # MeshCoP TLVs (often overlap with Diagnostic TLVs)
    if t == consts.NM_COMMISSIONER_SESSION_ID_TLV and len(v) == 2 and not is_diag:
        kvs.append(('comm_sess_id', struct.unpack('>H', v)[0]))
    elif t == consts.NM_STATE_TLV and len(v) == 1 and not is_diag:
        kvs.append(('state', v[0]))
    elif t == consts.NM_STEERING_DATA_TLV and not is_diag:  # DG_IPV6_ADDRESS_LIST_TLV is 16*n
        kvs.append(('steering_data', v))
    elif t == consts.NM_BORDER_AGENT_LOCATOR_TLV and len(v) == 2 and not is_diag:  # DG_MAC_COUNTERS_TLV is 4*n
        kvs.append(('border_agent_rloc16', struct.unpack('>H', v)[0]))
    elif t == consts.TLV_REQUEST_TLV:
        kvs.append(('tlv_request', v))
    elif t == consts.NM_CHANNEL_TLV and len(v) == 3 and not is_diag:  # DG_MAC_EXTENDED_ADDRESS_TLV is 8
        kvs.append(('channel', struct.unpack('>H', v[1:3])[0]))
    elif t == consts.NM_ACTIVE_TIMESTAMP_TLV and len(v) == 8 and not is_diag:
        kvs.append(('active_timestamp', struct.unpack('>Q', v)[0] >> 16))
    elif t == consts.NM_PENDING_TIMESTAMP_TLV and len(v) == 8 and not is_diag:
        kvs.append(('pending_timestamp', struct.unpack('>Q', v)[0] >> 16))
    elif t == consts.NM_DELAY_TIMER_TLV and len(v) == 4 and not is_diag:
        kvs.append(('delay_timer', struct.unpack('>I', v)[0]))
    elif t == consts.NM_CHANNEL_MASK_TLV and not is_diag:
        kvs.append(('channel_mask', v))
    elif t == consts.NM_COUNT_TLV and len(v) == 1 and not is_diag:
        kvs.append(('count', v[0]))
    elif t == consts.NM_PERIOD_TLV and len(v) == 2 and not is_diag:
        kvs.append(('period', struct.unpack('>H', v)[0]))
    elif t == consts.NM_SCAN_DURATION and len(v) == 2 and not is_diag:
        kvs.append(('scan_duration', struct.unpack('>H', v)[0]))
    elif t == consts.NM_ENERGY_LIST_TLV and not is_diag:
        kvs.append(('energy_list', v))
    elif t == consts.NM_EXTENDED_PAN_ID_TLV and len(v) == 8 and not is_diag:
        kvs.append(('ext_pan_id', v))
    elif t == consts.NM_NETWORK_NAME_TLV and not is_diag:
        kvs.append(('network_name', v.decode('utf-8', errors='replace')))
    elif t == consts.NM_PSKC_TLV and len(v) == 16 and not is_diag:
        kvs.append(('pskc', v))
    elif t == consts.NM_SECURITY_POLICY_TLV and not is_diag:
        kvs.append(('security_policy', v))
        if len(v) >= 3:
            # v[0:2] is rotation time
            # v[2] is the first byte of flags
            # Bits in first byte of flags (v[2]):
            # Bits in first byte of flags (v[2]):
            # 7: o (obtaining network key)
            # 6: n (native commissioning)
            # 5: r (routers)
            # 4: c (external commissioner)
            # 3: b (beacons)
            # 2: C (commercial commissioning)
            # 1: e (autonomous enrollment)
            # 0: p (network key provisioning)
            flags = v[2]
            kvs.append(('sec_policy_o', (flags >> 7) & 1))
            kvs.append(('sec_policy_n', (flags >> 6) & 1))
            kvs.append(('sec_policy_r', (flags >> 5) & 1))
            kvs.append(('sec_policy_c', (flags >> 4) & 1))
            kvs.append(('sec_policy_b', (flags >> 3) & 1))
            kvs.append(('sec_policy_C', (flags >> 2) & 1))
            kvs.append(('sec_policy_e', (flags >> 1) & 1))
            kvs.append(('sec_policy_p', flags & 1))

    elif t == consts.NM_NETWORK_KEY_TLV and len(v) == 16 and not is_diag:
        kvs.append(('network_key', v))
    elif t == consts.NM_PAN_ID_TLV and len(v) == 2 and not is_diag:
        kvs.append(('pan_id', struct.unpack('>H', v)[0]))
    elif t == consts.NM_NETWORK_MESH_LOCAL_PREFIX_TLV and len(v) == 8 and not is_diag:
        kvs.append(('mesh_local_prefix', v))
    elif t == consts.NM_FUTURE_TLV:
        kvs.append(('future_tlv', v))

    # Other Thread TLVs
    elif t == consts.NL_TARGET_EID_TLV and len(v) == 16:
        kvs.append(('target_eid', str(Ipv6Addr(v))))
    elif t == consts.DG_MAC_EXTENDED_ADDRESS_TLV and len(v) == 8 and is_diag:
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
    elif t == consts.DG_MAC_COUNTERS_TLV and is_diag:
        # MAC counters are a list of 4-byte values
        for i in range(0, len(v), 4):
            if i + 4 <= len(v):
                val = struct.unpack('>I', v[i:i + 4])[0]
                kvs.append(('mac_counter', str(val)))
    elif t == consts.DG_MODE_TLV and len(v) == 1:
        kvs.append(('mode', hex(v[0])))
    elif t == consts.DG_IPV6_ADDRESS_LIST_TLV and is_diag:
        for i in range(0, len(v), 16):
            if i + 16 <= len(v):
                kvs.append(('ipv6_address', str(Ipv6Addr(v[i:i + 16]))))
    elif t == consts.DG_LEADER_DATA_TLV and len(v) == 8:
        # Leader data contains Partition ID (4), Weighting (1), Data Version (1),
        # Stable Data Version (1), Leader Router ID (1)
        kvs.append(('partition_id', hex(struct.unpack('>I', v[0:4])[0])))
        kvs.append(('leader_router_id', str(v[7])))
    elif t == consts.DG_ROUTE64_TLV:
        # Route64 contains Router ID Sequence (1), and Router ID Mask (8), then link qualities
        kvs.append(('router_id_sequence', str(v[0])))
        kvs.append(('router_id_mask', v[1:9].hex()))
    elif t == consts.DG_CHILD_TABLE_TLV and is_diag:
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
        kvs.append(('channel_pages', v))
    elif t == consts.NM_DISCOVERY_RESPONSE_TLV:  # Discovery Response TLV
        # Bits 7-4: Version, Bit 3: Native Commissioner
        if len(v) >= 1:
            kvs.append(('discovery_version', (v[0] >> 4) & 0xf))
            kvs.append(('discovery_native_commissioner', (v[0] >> 3) & 1))
    return kvs


def apply_patches():
    CoapTlvParser.parse = staticmethod(thread_coap_tlv_parse)

    from pktverify import consts, layer_fields
    consts.VALID_LAYER_NAMES.add('wpan_tap')
    consts.VALID_LAYER_NAMES.add('wpan-tap')

    # Patch _get_candidate_layers to map wpan_tap to wpan-tap
    old_get_candidate_layers = layer_fields._get_candidate_layers

    def patched_get_candidate_layers(packet, layer_name):
        if layer_name == 'wpan_tap':
            layer_name = 'wpan-tap'
        return old_get_candidate_layers(packet, layer_name)

    layer_fields._get_candidate_layers = patched_get_candidate_layers

    # Patch Layer.get_field to handle wpan_tap.ch_num
    from pyshark.packet.layer import Layer
    old_get_field = Layer.get_field

    def patched_get_field(self, name):
        v = old_get_field(self, name)
        if v is None and name == 'wpan_tap.ch_num':
            v = old_get_field(self, 'wpan-tap.ch_num')
        return v

    Layer.get_field = patched_get_field

    layer_fields._LAYER_FIELDS['coap.tlv.tlv_request'] = layer_fields._bytes
    layer_fields._LAYER_FIELDS['mle.tlv.active_operational_dataset'] = layer_fields._bytes
    layer_fields._LAYER_FIELDS['mle.tlv.pending_operational_dataset'] = layer_fields._bytes
    layer_fields._LAYER_FIELDS['coap.tlv.ipv6_address'] = layer_fields._list(layer_fields._ipv6_addr)
    layer_fields._LAYER_FIELDS['coap.tlv.rloc16'] = layer_fields._auto
    layer_fields._LAYER_FIELDS['coap.tlv.mode'] = layer_fields._auto
    layer_fields._LAYER_FIELDS['coap.tlv.leader_router_id'] = layer_fields._auto
    layer_fields._LAYER_FIELDS['coap.tlv.child_id'] = layer_fields._list(layer_fields._auto)
    layer_fields._LAYER_FIELDS['coap.tlv.child_mode'] = layer_fields._list(layer_fields._auto)
    layer_fields._LAYER_FIELDS['coap.tlv.channel_pages'] = layer_fields._bytes
    layer_fields._LAYER_FIELDS['coap.tlv.steering_data'] = layer_fields._bytes
    layer_fields._LAYER_FIELDS['coap.tlv.future_tlv'] = layer_fields._bytes
    layer_fields._LAYER_FIELDS['coap.tlv.comm_sess_id'] = layer_fields._auto
    layer_fields._LAYER_FIELDS['coap.tlv.state'] = layer_fields._auto
    layer_fields._LAYER_FIELDS['coap.tlv.border_agent_rloc16'] = layer_fields._auto
    layer_fields._LAYER_FIELDS['coap.tlv.channel'] = layer_fields._auto
    layer_fields._LAYER_FIELDS['coap.tlv.active_timestamp'] = layer_fields._auto
    layer_fields._LAYER_FIELDS['coap.tlv.pending_timestamp'] = layer_fields._auto
    layer_fields._LAYER_FIELDS['coap.tlv.delay_timer'] = layer_fields._auto
    layer_fields._LAYER_FIELDS['coap.tlv.channel_mask'] = layer_fields._bytes
    layer_fields._LAYER_FIELDS['coap.tlv.count'] = layer_fields._auto
    layer_fields._LAYER_FIELDS['coap.tlv.period'] = layer_fields._auto
    layer_fields._LAYER_FIELDS['coap.tlv.scan_duration'] = layer_fields._auto
    layer_fields._LAYER_FIELDS['coap.tlv.energy_list'] = layer_fields._bytes
    layer_fields._LAYER_FIELDS['wpan_tap.ch_num'] = layer_fields._auto
    layer_fields._LAYER_FIELDS['wpan-tap.ch_num'] = layer_fields._auto
    layer_fields._LAYER_FIELDS['coap.tlv.ext_pan_id'] = layer_fields._bytes
    layer_fields._LAYER_FIELDS['coap.tlv.network_name'] = layer_fields._str
    layer_fields._LAYER_FIELDS['coap.tlv.pskc'] = layer_fields._bytes
    layer_fields._LAYER_FIELDS['coap.tlv.security_policy'] = layer_fields._bytes
    layer_fields._LAYER_FIELDS['coap.tlv.sec_policy_o'] = layer_fields._auto
    layer_fields._LAYER_FIELDS['coap.tlv.sec_policy_n'] = layer_fields._auto
    layer_fields._LAYER_FIELDS['coap.tlv.sec_policy_r'] = layer_fields._auto
    layer_fields._LAYER_FIELDS['coap.tlv.sec_policy_c'] = layer_fields._auto
    layer_fields._LAYER_FIELDS['coap.tlv.sec_policy_b'] = layer_fields._auto
    layer_fields._LAYER_FIELDS['coap.tlv.sec_policy_C'] = layer_fields._auto
    layer_fields._LAYER_FIELDS['coap.tlv.sec_policy_e'] = layer_fields._auto
    layer_fields._LAYER_FIELDS['coap.tlv.sec_policy_p'] = layer_fields._auto
    layer_fields._LAYER_FIELDS['coap.tlv.network_key'] = layer_fields._bytes
    layer_fields._LAYER_FIELDS['coap.tlv.pan_id'] = layer_fields._auto
    layer_fields._LAYER_FIELDS['coap.tlv.mesh_local_prefix'] = layer_fields._bytes
    layer_fields._LAYER_FIELDS['thread_meshcop.tlv.discovery_version'] = layer_fields._dec
    layer_fields._LAYER_FIELDS['thread_meshcop.tlv.discovery_native_commissioner'] = layer_fields._dec

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
            existing_keys = wireshark_prefs.get('uat:ieee802154_keys', '')
            new_key = f'"{network_key}","1","Thread hash"'
            if network_key not in existing_keys:
                if existing_keys:
                    wireshark_prefs['uat:ieee802154_keys'] = existing_keys + '\n' + new_key
                else:
                    wireshark_prefs['uat:ieee802154_keys'] = new_key

        mesh_local_prefix = data.get('extra_vars', {}).get('mesh_local_prefix')
        if mesh_local_prefix:
            prefix_addr = mesh_local_prefix.split('/')[0]
            wireshark_prefs['6lowpan.context0'] = f'{prefix_addr}/64'
            # Update the Link-Local All Thread Nodes multicast address constant
            # FF32:40:<MeshLocalPrefix>::1
            prefix = Ipv6Addr(prefix_addr)
            all_thread_nodes_mcast_addr = bytearray(Ipv6Addr('ff32:40::1'))
            all_thread_nodes_mcast_addr[4:12] = prefix[0:8]
            consts.LINK_LOCAL_ALL_THREAD_NODES_MULTICAST_ADDRESS = Ipv6Addr(all_thread_nodes_mcast_addr)

        pv = PacketVerifier(json_file, wireshark_prefs=wireshark_prefs)
        pv.add_common_vars()

        # Add RLOC16 variables for convenience
        for node_id, rloc16 in data.get('rloc16s', {}).items():
            name = pv.test_info.get_node_name(int(node_id))
            pv.add_vars(**{f'{name}_RLOC16': int(rloc16, 16)})

        verify_func(pv)
        print("Verification PASSED")
    except Exception as e:
        print(f"Verification FAILED: {e}")
        traceback.print_exc()
        sys.exit(1)
