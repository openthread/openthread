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
from pktverify.layer_fields_container import LayerFieldsContainer


def get_val(field):
    if field is None:
        return None
    s = str(field)
    if s.lower() == 'null' or s == '':
        return None
    return field


def get_int(field):
    val = get_val(field)
    if val is None:
        return None
    try:
        return int(str(val), 0)
    except ValueError:
        return None


def verify(pv):
    pkts = pv.pkts

    LEADER = pv.vars['LEADER']
    SSED_1 = pv.vars['SSED_1']

    LEADER_RLOC16 = get_int(pv.vars.get('LEADER_RLOC16'))
    SSED_1_RLOC16 = get_int(pv.vars.get('SSED_1_RLOC16'))

    LEADER_STR = str(LEADER).lower()
    SSED_1_STR = str(SSED_1).lower()

    print(f"LEADER: {LEADER}, RLOC16: {hex(LEADER_RLOC16) if LEADER_RLOC16 is not None else 'None'}")
    print(f"SSED_1: {SSED_1}, RLOC16: {hex(SSED_1_RLOC16) if SSED_1_RLOC16 is not None else 'None'}")

    groups = {}

    # 1. Collect all MAC Data frames
    for i in range(len(pkts)):
        p = pkts[i]
        if not hasattr(p, 'wpan') or get_int(p.wpan.frame_type) != consts.MAC_FRAME_TYPE_DATA:
            continue

        src16 = get_int(getattr(p.wpan, 'src16', None))
        src64 = get_val(getattr(p.wpan, 'src64', None))
        dst16 = get_int(getattr(p.wpan, 'dst16', None))
        seq = get_int(p.wpan.seq_no)

        if dst16 == 0xffff:  # Skip broadcast
            continue

        # Check if from LEADER or SSED_1
        is_relevant_src = False
        if src64 is not None:
            src_str = str(src64).lower()
            if src_str == LEADER_STR or src_str == SSED_1_STR:
                is_relevant_src = True
        elif src16 is not None:
            if src16 == LEADER_RLOC16 or src16 == SSED_1_RLOC16:
                is_relevant_src = True

        if not is_relevant_src:
            continue

        # Group key identifies the (source, sequence) pair.
        # Destination might change between RLOC16 and ExtAddr in some edge cases,
        # but sequence numbers are usually unique enough within a short window.
        src_id = str(src64).lower() if src64 is not None else hex(src16)
        group_key = (src_id, seq)
        if group_key not in groups:
            groups[group_key] = []
        groups[group_key].append(p)

    # 2. Filter groups that are retransmissions (> 1 packet)
    retry_groups = [g for k, g in groups.items() if len(g) > 1]
    print(f"Found {len(retry_groups)} retransmission groups.")

    if not retry_groups:
        raise RuntimeError("No retransmissions detected in pcap. The test might not have triggered retries correctly.")

    found_csl_retry = False

    # 3. Verify each group
    for group in retry_groups:
        p0 = group[0]
        src_id = str(p0.wpan.src64).lower() if get_val(getattr(p0.wpan, 'src64', None)) else hex(get_int(
            p0.wpan.src16))
        seq = get_int(p0.wpan.seq_no)

        # Check for CSL IE in any packet of the group (it should be in all if it's a CSL retry)
        has_csl = False
        for p in group:
            has_csl = p.wpan.has('header_ie.csl.period')
            if has_csl:
                csl_period = p.wpan.header_ie.csl.period
                has_csl = str(csl_period) != 'null'

            if has_csl:
                break

        type_str = "CSL" if has_csl else "Non-CSL"
        print(f"Verifying {type_str} seq={seq} from {src_id} ({len(group)} attempts)")

        if has_csl:
            found_csl_retry = True

        last_counter = -1
        last_phase = -1
        for i, p in enumerate(group):
            counter = get_int(getattr(getattr(p.wpan, 'aux_sec', None), 'frame_counter', None))
            has_header_ie = get_val(getattr(p.wpan, 'header_ie', None)) is not None

            if has_csl:
                print(f"  Attempt {i}: Counter={counter}, HeaderIE={has_header_ie}")
                if counter is not None and has_header_ie:
                    if last_counter != -1 and counter <= last_counter:
                        raise RuntimeError(
                            f"Nonce reuse detected! Frame counter {counter} did not increment for CSL seq {seq}")
                    last_counter = counter

                # Check CSL phase
                try:
                    phase = get_int(p.wpan.header_ie.csl.phase)
                    if phase is not None:
                        print(f"  Attempt {i}: CSL Phase={phase}")
                        if phase == last_phase:
                            print(
                                f"  Warning: CSL phase did not update between attempts for seq {seq} (likely fast retry)"
                            )
                        last_phase = phase
                    else:
                        print(f"  Attempt {i}: CSL Phase parsed as None")
                except (AttributeError, ValueError):
                    print(f"  Attempt {i}: CSL Phase not parsed")
            else:
                # For non-CSL, same counter is expected/allowed in standard 15.4 retries
                print(f"  Attempt {i}: Counter={counter} (Allowed to be same as previous)")
                if counter is not None:
                    if counter < last_counter:
                        raise RuntimeError(f"Frame counter {counter} went backwards for Non-CSL seq {seq}")
                    last_counter = counter

    if not found_csl_retry:
        raise RuntimeError("No CSL retransmissions found. Expected SSED to Leader retries with CSL IEs.")

    print("All verification steps passed!")


if __name__ == '__main__':
    verify_utils.run_main(verify)
