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
from pktverify.addrs import Ipv6Addr
from pktverify.bytes import Bytes


def verify(pv):
    # The purpose of this test is to verify ALOC connectivity and ensure no Address Query is sent.

    pkts = pv.pkts
    pv.summary.show()

    # Get Mesh-Local Prefix
    mesh_local_prefix = pv.vars.get('mesh_local_prefix')
    if mesh_local_prefix:
        prefix = Bytes(Ipv6Addr(mesh_local_prefix.split('/')[0]))[:8]
    else:
        # Fallback to default if not provided
        prefix = consts.DEFAULT_MESH_LOCAL_PREFIX

    # Get ALOCs
    LEADER_ALOC = pv.vars.get('LEADER_ALOC') or Ipv6Addr(prefix + consts.LEADER_ALOC_IID)
    PBBR_ALOC = pv.vars.get('PBBR_ALOC') or Ipv6Addr(prefix + consts.PBBR_ALOC_IID)

    # Verify Leader ALOC ping from ROUTER
    print(f"Verifying Leader ALOC ({LEADER_ALOC}) ping from ROUTER")
    pkts.filter_ping_request().filter_ipv6_dst(LEADER_ALOC).must_next()
    pkts.filter_ping_reply().filter_ipv6_src(LEADER_ALOC).must_next()

    # Verify PBBR ALOC ping from ROUTER
    print(f"Verifying PBBR ALOC ({PBBR_ALOC}) ping from ROUTER")
    pkts.filter_ping_request().filter_ipv6_dst(PBBR_ALOC).must_next()
    pkts.filter_ping_reply().filter_ipv6_src(PBBR_ALOC).must_next()

    # Make sure no ADDR_QUERY.qry is ever sent
    pkts.filter_coap_request(consts.ADDR_QRY_URI).must_not_next()


if __name__ == '__main__':
    verify_utils.run_main(verify)
