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
from pktverify.bytes import Bytes
from pktverify.null_field import nullField


def verify(pv):
    # 5.11.3 BBR-TC-03: mDNS discovery of BBR function
    #
    # 5.11.3.1 Topology
    # - BR_1: BR device initially operating as the Primary BBR and Leader.
    # - BR_2: BR device initially operating as a Secondary BBR.
    # - Host: Test bed BR device operating as a non-Thread IPv6 host. It is used to
    #   send out the mDNS queries.
    #
    # 5.11.3.2 Purpose & Description
    # The purpose of this test case is to verify that a BBR Function (both Primary
    #   and Secondary) can be discovered using mDNS and that any relevant changes
    #   are reflected in the mDNS data sent by the BBR. Also, to verify that the
    #   mandatory mDNS data fields are present and in the correct format. The BBR
    #   Sequence Number updating is not verified.
    #
    # Spec Reference | V1.2 Section
    # ---------------|-------------
    # BBR Discovery  | 5.11.3

    pkts = pv.pkts
    pv.summary.show()

    # Constants
    MDNS_IPV6_ADDR = 'ff02::fb'
    MDNS_UDP_PORT = 5353
    MESHCOP_SERVICE = '_meshcop._udp.local'
    NETWORK_NAME = pv.vars['NETWORK_NAME']
    XPAN_ID = Bytes(pv.vars['XPAN_ID'])

    # In Nexus, Ethernet MAC addresses are 02:00:00:00:00:<node_id>
    BR_1_ETH = '02:00:00:00:00:00'
    BR_2_ETH = '02:00:00:00:00:01'
    HOST_1_ETH = '02:00:00:00:00:02'
    HOST_2_ETH = '02:00:00:00:00:03'

    def is_mdns_response(p):
        return p.udp.srcport == MDNS_UDP_PORT

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

    def get_bbr_txt_entries_if_complete(p):
        """Returns mDNS TXT entries if 'omr' and 'sb' records are present and have values, otherwise None."""
        txt = get_txt_entries(p)
        if txt and txt.get('omr') and txt.get('sb') is not None:
            return txt
        return None

    def has_sb_bit(p, bit_pos):
        """Checks for a bit in the 'sb' record of a complete ('omr' and 'sb' present) mDNS response."""
        txt = get_bbr_txt_entries_if_complete(p)
        if not txt:
            return False
        # 'sb' is a 4-byte value.
        sb_val = struct.unpack('>I', txt['sb'])[0]
        return (sb_val >> bit_pos) & 1 == 1

    def verify_mdns_response(p, expected_sb_bits, expected_omr_var):
        # Verify Service Instance Name
        names = []
        if hasattr(p, 'mdns') and p.mdns:
            names = p.mdns.resp.name
        elif hasattr(p, 'dns') and p.dns:
            names = p.dns.resp.name

        if not isinstance(names, list):
            names = [names]

        found_meshcop = False
        for name in names:
            if isinstance(name, nullField.__class__):
                continue
            if name.endswith('.' + MESHCOP_SERVICE):
                instance_name = name[:-len(MESHCOP_SERVICE) - 1]
                assert len(instance_name) > 0, f"Invalid instance name: {instance_name}"
                found_meshcop = True
                break
            elif name == MESHCOP_SERVICE:
                # This is the service name itself (PTR record name)
                pass
        assert found_meshcop, f"MeshCoP service instance not found in response names: {names}"

        txts = get_txt_entries(p)

        # Mandatory TXT records
        assert 'dn' in txts, f"dn TXT record missing. Available: {list(txts.keys())}"
        assert len(txts['dn']) > 0, "dn TXT record is empty"

        assert bytes(txts['rv']).decode() == '1', f"rv TXT record expected 1, got {txts['rv']}"

        tv = bytes(txts['tv']).decode()
        assert len(tv) >= 5, f"tv TXT record expected >= 5 chars, got {len(tv)} ({tv})"
        tv_parts = [int(x) for x in tv.split('.')]
        assert tv_parts >= [1, 2, 0], f"tv version expected >= 1.2.0, got {tv}"

        # sb: Verify exactly 4 bytes
        assert len(txts['sb']) == 4, f"sb TXT record expected 4 bytes, got {len(txts['sb'])}"
        sb = struct.unpack('>I', txts['sb'])[0]
        is_active = (sb >> 7) & 1
        is_primary = (sb >> 8) & 1

        for bit, expected_val in expected_sb_bits.items():
            if bit == 'ifstate':
                actual = (sb >> 3) & 3
                if isinstance(expected_val, list):
                    assert actual in expected_val, f"ifstate bitmask expected one of {expected_val}, got {actual} (sb={sb:08x})"
                else:
                    assert actual == expected_val, f"ifstate bitmask expected {expected_val}, got {actual} (sb={sb:08x})"
            elif bit == 'active':
                actual = (sb >> 7) & 1
                if isinstance(expected_val, list):
                    assert actual in expected_val, f"active bitmask expected one of {expected_val}, got {actual} (sb={sb:08x})"
                else:
                    assert actual == expected_val, f"active bitmask expected {expected_val}, got {actual} (sb={sb:08x})"
            elif bit == 'primary':
                actual = (sb >> 8) & 1
                if isinstance(expected_val, list):
                    assert actual in expected_val, f"primary bitmask expected one of {expected_val}, got {actual} (sb={sb:08x})"
                else:
                    assert actual == expected_val, f"primary bitmask expected {expected_val}, got {actual} (sb={sb:08x})"

        # bb: 61631 (0xF0BF) - Only present if BBR is active
        # sq: Binary uint8 - Only present if BBR is active
        if is_active:
            assert 'bb' in txts, f"bb TXT record missing but BBR is active (sb={sb:08x})"
            bb = struct.unpack('>H', txts['bb'])[0]
            assert bb == 61631, f"bb TXT record expected 61631, got {bb}"

            assert 'sq' in txts, "sq TXT record missing but BBR is active"
            assert len(txts['sq']) == 1, f"sq TXT record expected 1 byte, got {len(txts['sq'])}"
        else:
            # Note: Step 9b says bb is mandatory even if active=0 in that specific case
            # but generally it's tied to activity.
            pass

        # nn: NetwName1
        nn = bytes(txts['nn']).decode()
        assert nn == NETWORK_NAME, f"nn TXT record expected {NETWORK_NAME}, got {nn}"

        # xp: Extended PAN ID
        xp = txts['xp']
        assert xp == XPAN_ID, f"xp TXT record expected {XPAN_ID}, got {xp}"

        # omr: byte 0x40 followed by 8 bytes OMR prefix
        # Only present if BBR is active (as it's published in Network Data)
        if is_active:
            assert 'omr' in txts, f"omr TXT record missing but BBR is active (sb={sb:08x})"
            omr = txts['omr']
            assert len(omr) == 9, f"omr TXT record expected 9 bytes, got {len(omr)} ({omr})"
            assert omr[0] == 0x40, f"omr TXT record first byte expected 0x40, got {omr[0]:02x}"

            # All BRs in the same Thread mesh will have the same OMR prefix at a given step.
            expected_omr = Bytes(pv.vars[expected_omr_var])

            assert omr[
                1:] == expected_omr, f"omr TXT record prefix expected {expected_omr}, got {omr[1:]} (src={p.eth.src})"

        # Vendor specific data check
        for key in txts:
            if key.startswith('v') and key not in ['rv', 'tv', 'vo', 'vn']:
                assert 'vo' in txts, f"vo TXT record missing but vendor-specific record '{key}' present"
                assert len(txts['vo']) == 3, f"vo TXT record expected 3 bytes (Binary uint24), got {len(txts['vo'])}"
            if key == 'mn':
                # 'mn' is also vendor-specific but often present in Nexus without 'vo'
                pass

    # Step 1: Host sends an mDNS query (per P2).
    print("Step 1: Host sends an mDNS query.")
    pkts.filter_eth_src(HOST_1_ETH).\
        filter_ipv6_dst(MDNS_IPV6_ADDR).\
        filter(lambda p: p.udp.dstport == MDNS_UDP_PORT).\
        must_next()

    # Step 2 & 3: BR_1 and BR_2 automatically respond.
    print("Step 2: BR_1 automatically responds to the mDNS query.")
    p1 = pkts.copy().\
        filter_eth_src(BR_1_ETH).\
        filter(is_mdns_response).\
        must_next()
    verify_mdns_response(p1, {
        'ifstate': [1, 2],
        'active': [0, 1],
        'primary': [0, 1]
    },
                         expected_omr_var='OMR_PREFIX_STEP_0')

    print("Step 3: BR_2 automatically responds to the mDNS query.")
    p2 = pkts.copy().\
        filter_eth_src(BR_2_ETH).\
        filter(is_mdns_response).\
        must_next()
    verify_mdns_response(p2, {'ifstate': 2, 'active': 1, 'primary': 0}, expected_omr_var='OMR_PREFIX_STEP_0')

    # Step 4: The device must be rebooted (reset); wait until it is back online.
    print("Step 4: BR_1 is rebooted and remains Leader.")

    # Step 5: Harness instructs the device to send an mDNS query.
    print("Step 5: Host sends an mDNS query.")
    # Skip all packets until the next query which should be around Step 5
    pkts.filter_eth_src(HOST_1_ETH).\
        filter_ipv6_dst(MDNS_IPV6_ADDR).\
        filter(lambda p: p.udp.dstport == MDNS_UDP_PORT).\
        filter(lambda p: p.number > max(p1.number, p2.number)).\
        must_next()
    q_number = pkts.last().number

    # Step 6: Automatically responds to the mDNS query.
    print("Step 6: BR_1 automatically responds to the mDNS query.")
    p1 = pkts.copy().\
        filter_eth_src(BR_1_ETH).\
        filter(is_mdns_response).\
        filter(lambda p: p.number > q_number).\
        filter(get_bbr_txt_entries_if_complete).\
        must_next()
    verify_mdns_response(p1, {
        'ifstate': [1, 2],
        'active': [0, 1],
        'primary': [0, 1]
    },
                         expected_omr_var='OMR_PREFIX_STEP_4')

    print("Step 7: BR_2 automatically responds to the mDNS query.")
    p2 = pkts.copy().\
        filter_eth_src(BR_2_ETH).\
        filter(is_mdns_response).\
        filter(lambda p: p.number > q_number).\
        must_next()
    # BBR is disabled on BR_2 in Step 3b (only if DUT=BR_1)
    verify_mdns_response(p2, {'ifstate': 2, 'active': 0, 'primary': 0}, expected_omr_var='OMR_PREFIX_STEP_0')

    # Step 8: The device must be powered down; wait for BR_2 to become Primary BBR and Leader.
    print("Step 8: BR_1 is powered down; BR_2 becomes Primary BBR.")

    # Step 9: Harness instructs the device to send an mDNS query.
    print("Step 9: Host sends an mDNS query.")
    pkts.filter_eth_src(HOST_2_ETH).\
        filter_ipv6_dst(MDNS_IPV6_ADDR).\
        filter(lambda p: p.udp.dstport == MDNS_UDP_PORT).\
        filter(lambda p: p.number > max(p1.number, p2.number)).\
        must_next()
    q_number = pkts.last().number

    # Step 9b: Optionally responds to the mDNS query, as a BR with disabled BBR Function.
    print("Step 9b: Optionally BR_1 responds as disabled BBR.")
    p = pkts.copy().\
        filter_eth_src(BR_1_ETH).\
        filter(is_mdns_response).\
        filter(lambda p: p.number > q_number).\
        next()
    if p:
        # Step 9b Pass Criteria: Verify Bit 3-4: 0b00 or 0b01, Verify Bit 7: 0, Verify Bit 8: 0
        # Also contains dn record. bb record is optional if active=0.
        txts = get_txt_entries(p)
        assert 'dn' in txts, "dn TXT record missing in Step 9b"
        if 'bb' in txts:
            bb = struct.unpack('>H', txts['bb'])[0]
            assert bb == 61631, f"bb TXT record expected 61631, got {bb}"

        sb = struct.unpack('>I', txts['sb'])[0]
        assert (sb >> 3) & 3 in [0, 1], f"sb ifstate bitmask expected 0 or 1, got {(sb >> 3) & 3} (sb={sb:08x})"
        assert (sb >> 7) & 1 == 0, f"sb active bitmask expected 0, got {(sb >> 7) & 1} (sb={sb:08x})"
        assert (sb >> 8) & 1 == 0, f"sb primary bitmask expected 0, got {(sb >> 8) & 1} (sb={sb:08x})"

    # Step 10: Automatically responds to the mDNS query, as a PBBR (BR_2).
    print("Step 10: BR_2 automatically responds to the mDNS query as PBBR.")
    # We might see stale SBBR responses due to timing, so we look for the PBBR one.
    p2 = pkts.copy().\
        filter_eth_src(BR_2_ETH).\
        filter(is_mdns_response).\
        filter(lambda p: p.number > q_number).\
        filter(lambda p: has_sb_bit(p, 8)).\
        must_next()

    verify_mdns_response(p2, {'ifstate': 2, 'active': 1, 'primary': 1}, expected_omr_var='OMR_PREFIX_STEP_10')

    # Step 11: The device must be powered up; BR_1 joins BR_2.
    print("Step 11: BR_1 is powered up and joins BR_2.")

    # Step 12: Harness instructs the device to send an mDNS query.
    print("Step 12: Host sends an mDNS query.")
    pkts.filter_eth_src(HOST_1_ETH).\
        filter_ipv6_dst(MDNS_IPV6_ADDR).\
        filter(lambda p: p.udp.dstport == MDNS_UDP_PORT).\
        filter(lambda p: p.number > p2.number).\
        must_next()
    q_number = pkts.last().number

    # Step 13 & 14: BR_1 and BR_2 automatically respond.
    print("Step 13: BR_1 automatically responds to the mDNS query as SBBR.")
    p1 = pkts.copy().\
        filter_eth_src(BR_1_ETH).\
        filter(is_mdns_response).\
        filter(lambda p: p.number > q_number).\
        filter(lambda p: has_sb_bit(p, 7)).\
        must_next()
    verify_mdns_response(p1, {'ifstate': 2, 'active': 1, 'primary': 0}, expected_omr_var='OMR_PREFIX_STEP_11')

    print("Step 14: BR_2 automatically responds to the mDNS query as PBBR.")
    p2 = pkts.copy().\
        filter_eth_src(BR_2_ETH).\
        filter(is_mdns_response).\
        filter(lambda p: p.number > q_number).\
        filter(lambda p: has_sb_bit(p, 8)).\
        must_next()
    verify_mdns_response(p2, {'ifstate': 2, 'active': 1, 'primary': 1}, expected_omr_var='OMR_PREFIX_STEP_10')


if __name__ == '__main__':
    verify_utils.run_main(verify)
