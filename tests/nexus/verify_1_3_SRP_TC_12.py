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


def verify(pv):
    pkts = pv.pkts
    pv.summary.show()

    BR_1_RLOC16 = pv.vars['BR_1_RLOC16']
    BR_2_RLOC16 = pv.vars['BR_2_RLOC16']
    BR_3_RLOC16 = pv.vars['BR_3_RLOC16']

    SERVICE_ID_SRP_ANYCAST_DNS = 92
    SERVICE_ID_SRP_UNICAST_DNS = 93

    # Step 1
    print("Step 1: BR 1 (DUT) Enable; switch on.")

    # Step 2
    print("Step 2: BR 1 (DUT) adds SRP Server information in the Thread Network Data.")
    pkts.\
      filter_mle_cmd(consts.MLE_DATA_RESPONSE).\
      filter(lambda p: any(id in (SERVICE_ID_SRP_ANYCAST_DNS, SERVICE_ID_SRP_UNICAST_DNS) \
                           for id in verify_utils.as_list(p.thread_nwd.tlv.service.srp_dataset_identifier))).\
      must_next()

    # Step 3
    print("Step 3: BR 2 Enable.")

    # Step 4
    print("Step 4: BR 2 adds its SRP Server information with a (faked) high numerical address.")

    # Step 5
    print("Step 5: BR 1 (DUT) integrates BR 2's SRP Server information.")
    pkts.\
      filter_mle_cmd(consts.MLE_DATA_RESPONSE).\
      filter(lambda p: SERVICE_ID_SRP_UNICAST_DNS in verify_utils.as_list(p.thread_nwd.tlv.service.srp_dataset_identifier)).\
      filter(lambda p: len(verify_utils.as_list(p.thread_nwd.tlv.server_16)) >= 2).\
      must_next()

    # Step 6
    print("Step 6: BR 3 Enable.")

    # Step 7
    print("Step 7: BR 3 adds its SRP Server information with a (faked) low numerical address.")

    # Step 8
    print("Step 8: BR 1 (DUT) integrates BR 3's SRP Server information and withdraws its own Unicast Dataset.")
    pkts.\
      filter_mle_cmd(consts.MLE_DATA_RESPONSE).\
      filter(lambda p: SERVICE_ID_SRP_UNICAST_DNS in verify_utils.as_list(p.thread_nwd.tlv.service.srp_dataset_identifier)).\
      filter(lambda p: all(int(rloc) != BR_1_RLOC16 for rloc in verify_utils.as_list(p.thread_nwd.tlv.server_16))).\
      filter(lambda p: len(verify_utils.as_list(p.thread_nwd.tlv.server_16)) >= 2).\
      must_next()

    # Step 9
    print("Step 9: BR 2 adds an additional Anycast Dataset.")

    # Step 10
    print("Step 10: BR 1 (DUT) integrates BR 2's new Anycast Dataset.")
    pkts.\
      filter_mle_cmd(consts.MLE_DATA_RESPONSE).\
      filter(lambda p: SERVICE_ID_SRP_ANYCAST_DNS in verify_utils.as_list(p.thread_nwd.tlv.service.srp_dataset_identifier)).\
      filter(lambda p: SERVICE_ID_SRP_UNICAST_DNS in verify_utils.as_list(p.thread_nwd.tlv.service.srp_dataset_identifier)).\
      filter(lambda p: all(int(rloc) != BR_1_RLOC16 for rloc in verify_utils.as_list(p.thread_nwd.tlv.server_16))).\
      must_next()


if __name__ == '__main__':
    verify_utils.run_main(verify)
