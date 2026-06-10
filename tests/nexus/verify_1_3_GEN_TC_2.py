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

import verify_utils
import ipaddress


def parse_txt(txt_records):
    txt_dict = {}
    for entry in txt_records:
        if not entry:
            continue
        try:
            # entry is expected to be a Bytes object (from pktverify)
            if b'=' in entry:
                k, v = entry.split(b'=', 1)
                txt_dict[k.decode('ascii', errors='ignore')] = v
            else:
                txt_dict[entry.decode('ascii', errors='ignore')] = None
        except Exception:
            continue
    return txt_dict


def verify(pv):
    # Spec Reference | V1.3.0 Section
    # ---------------|---------------
    # Thread 1.3.0   | 9.2

    agent_ids = []

    last_idx = (0, 0)
    pkts = pv.pkts

    for i in range(2):
        print(f"Iteration {i}")

        BR_1_EXT_ADDR = pv.vars[f'BR_1_EXT_ADDR_{i}'].replace(':', '').lower()
        BR_1_OMR_PREFIX = pv.vars[f'BR_1_OMR_PREFIX_{i}']
        expected_pfx = ipaddress.IPv6Network(BR_1_OMR_PREFIX)
        # OMR value is 8 bytes of the network address in hex
        expected_omr_value = expected_pfx.network_address.packed[:8].hex().lower()

        print(f"Iteration {i} Expected ExtAddr: {BR_1_EXT_ADDR}")
        print(f"Iteration {i} Expected OMR Value: {expected_omr_value}")

        print("Step 1: Form topology. DUT becomes Leader. Configures OMR prefix.")

        def match_ra(p):
            if not (hasattr(p, 'icmpv6') and p.icmpv6.type == 134):
                return False
            if not hasattr(p.icmpv6, 'opt') or not hasattr(p.icmpv6.opt, 'prefix'):
                return False
            for pfx in p.icmpv6.opt.prefix:
                try:
                    actual_pfx = ipaddress.IPv6Network(str(pfx) + '/64', strict=False)
                    if actual_pfx.network_address == expected_pfx.network_address:
                        return True
                except ValueError:
                    continue
            return False

        ra = pkts.range(last_idx).filter(match_ra).must_next()
        print(f"Recorded OMR prefix: {BR_1_OMR_PREFIX} (found in packet {ra.number})")
        last_idx = pkts.index

        print("Step 2: Eth_1 sends mDNS PTR query for _meshcop._udp.local.")
        ptr_query = pkts.range(last_idx).filter(lambda p: (hasattr(p, 'mdns') and p.mdns.flags.response == 0 and any(
            '_meshcop._udp.local' in name for name in p.mdns.qry.name))).must_next()
        last_idx = pkts.index

        print("Step 3: BR_1 (DUT) responds with mDNS response with PTR records.")
        ptr_resp = pkts.range(last_idx).filter(lambda p: (hasattr(p, 'mdns') and p.mdns.flags.response == 1 and any(
            '_meshcop._udp.local' in name for name in p.mdns.resp.name))).must_next()
        last_idx = pkts.index

        print("Step 5: BR_1 (DUT) responds with mDNS response with TXT record.")

        txt_resp = None
        for pkt in pkts.range(last_idx):
            if not (hasattr(pkt, 'mdns') and pkt.mdns.flags.response == 1 and hasattr(pkt.mdns, 'txt')):
                continue

            txt_dict = parse_txt(pkt.mdns.txt)
            if 'omr' in txt_dict and txt_dict['omr'].hex().endswith(expected_omr_value):
                txt_resp = pkt
                break

        assert txt_resp is not None, f"Could not find TXT response for iteration {i} with OMR value {expected_omr_value}"
        print(f"Found TXT response in packet {txt_resp.number}")
        last_idx = (txt_resp.number, txt_resp.number)

        print("Step 6: Verify TXT record keys.")
        txt_dict = parse_txt(txt_resp.mdns.txt)

        # omr: First byte 0x40, the rest is OMR prefix 8 bytes. Total 9 bytes.
        assert 'omr' in txt_dict, "omr TXT record missing"
        omr_val = txt_dict['omr']
        assert omr_val[0] == 0x40, f"omr value first byte should be 0x40, got {omr_val[0]}"
        assert omr_val[1:].hex(
        ) == expected_omr_value, f"omr value mismatch, expected {expected_omr_value}, got {omr_val[1:].hex()}"
        assert len(omr_val) == 9, f"omr value length should be 9 bytes, got {len(omr_val)}"

        # id: Random number (16 bytes)
        assert 'id' in txt_dict, "id TXT record missing"
        agent_id = txt_dict['id'].hex()
        assert len(agent_id) == 32, f"id length should be 16 bytes, got {len(agent_id)//2}"
        agent_ids.append(agent_id)

        # nn: "Thread Cert 9.2"
        assert 'nn' in txt_dict, "nn TXT record missing"
        nn_val = txt_dict['nn'].decode('ascii')
        assert nn_val == "Thread Cert 9.2", f"nn value mismatch, expected 'Thread Cert 9.2', got '{nn_val}'"

        # tv: "1.3.0" or "1.4.0"
        assert 'tv' in txt_dict, "tv TXT record missing"
        tv_val = txt_dict['tv'].decode('ascii')
        assert tv_val in ["1.3.0", "1.4.0"], f"tv value mismatch, expected '1.3.0' or '1.4.0', got '{tv_val}'"

        # sb: Bit 9 and 10 MUST be '1'.
        assert 'sb' in txt_dict, "sb TXT record missing"
        # sb is 4 bytes.
        sb_val = int.from_bytes(txt_dict['sb'], 'big')
        assert (sb_val & 0x00000600) == 0x00000600, f"sb bits 9 and 10 must be '1', got {hex(sb_val)}"

        # xa: MAC Extended Address
        assert 'xa' in txt_dict, "xa TXT record missing"
        xa_val = txt_dict['xa'].hex()
        assert xa_val == BR_1_EXT_ADDR, f"xa value mismatch, expected {BR_1_EXT_ADDR}, got {xa_val}"

        # at: Active Timestamp 0x000000666af13100
        assert 'at' in txt_dict, "at TXT record missing"
        at_val = txt_dict['at'].hex()
        assert at_val == '000000666af13100', f"at value mismatch, expected '000000666af13100', got '{at_val}'"

        # vn: "NexusVendor"
        assert 'vn' in txt_dict, "vn TXT record missing"
        vn_val = txt_dict['vn'].decode('ascii')
        assert vn_val == "NexusVendor", f"vn value mismatch, expected 'NexusVendor', got '{vn_val}'"

        # mn: "NexusModel"
        assert 'mn' in txt_dict, "mn TXT record missing"
        mn_val = txt_dict['mn'].decode('ascii')
        assert mn_val == "NexusModel", f"mn value mismatch, expected 'NexusModel', got '{mn_val}'"

        # vo: 0x000001
        assert 'vo' in txt_dict, "vo TXT record missing"
        vo_val = txt_dict['vo'].hex()
        assert vo_val == '000001', f"vo value mismatch, expected '000001', got '{vo_val}'"

        if i == 1:
            assert agent_ids[0] != agent_ids[1], f"agent id should be different after factory reset: {agent_ids}"

    print("Verification SUCCESS")


if __name__ == '__main__':
    verify_utils.run_main(verify)
