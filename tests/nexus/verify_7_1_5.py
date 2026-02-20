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
from pktverify.null_field import nullField


def verify(pv):
    # 7.1.5 Network data updates - 3 Prefixes
    #
    # 7.1.5.1 Topology
    # - MED is configured to require complete network data. (Mode TLV)
    # - SED is configured to request only stable network data. (Mode TLV)
    #
    # 7.1.5.2 Purpose & Description
    # The purpose of this test case is to verify that the DUT sends properly formatted Server Data Notification CoAP
    #   frame when a third global prefix information is set on the DUT. The DUT must also correctly set Network Data
    #   aggregated and disseminated by the Leader and transmit it properly to all child devices already attached to it.
    #
    # Spec Reference                   | V1.1 Section       | V1.3.0 Section
    # ---------------------------------|--------------------|--------------------
    # Thread Network Data / Stable     | 5.13 / 5.14 / 5.15 | 5.13 / 5.14 / 5.15
    #   Thread Network Data / Network  |                    |
    #   Data and Propagation           |                    |

    pkts = pv.pkts
    pv.summary.show()

    LEADER = pv.vars['LEADER']
    LEADER_RLOC16 = pv.vars['LEADER_RLOC16']
    ROUTER_1 = pv.vars['ROUTER_1']
    ROUTER_1_RLOC16 = pv.vars['ROUTER_1_RLOC16']
    MED_1 = pv.vars['MED_1']
    SED_1 = pv.vars['SED_1']

    PREFIX_1 = Ipv6Addr('2001::')
    PREFIX_2 = Ipv6Addr('2002::')
    PREFIX_3 = Ipv6Addr('2003::')

    # Step 1: All
    # - Description: Topology Ensure topology is formed correctly.
    # - Pass Criteria: N/A
    print("Step 1: All")

    # Step 2: Router_1 (DUT)
    # - Description: User configures the DUT with the following On-Mesh Prefix Set:
    #   - Prefix 1: P_prefix=2001::/64 P_stable=1 P_on_mesh=1 P_preferred=1 P_slaac=1 P_default=1
    #   - Prefix 2: P_prefix=2002::/64 P_stable=0 P_on_mesh=1 P_preferred=1 P_slaac=1 P_default=1
    #   - Prefix 3: P_prefix=2003::/64 P_stable=1 P_on_mesh=1 P_preferred=1 P_slaac=1 P_default=0
    # - Pass Criteria: N/A
    print("Step 2: Router_1 (DUT)")

    # Step 3: Router_1 (DUT)
    # - Description: Automatically transmits a CoAP Server Data Notification to the Leader
    # - Pass Criteria: The DUT MUST send a CoAP Server Data Notification frame to the Leader including the server's
    #   information (Prefix, Border Router) for all three prefixes (Prefix 1, 2 and 3):
    #   - CoAP Request URI: coap://[<Leader address>]:MM/a/sd
    #   - CoAP Payload: Thread Network Data TLV
    print("Step 3: Router_1 (DUT)")
    pkts.filter_wpan_src64(ROUTER_1).\
      filter_wpan_dst16(LEADER_RLOC16).\
      filter_coap_request(consts.SVR_DATA_URI).\
      filter(lambda p: {
                        consts.NL_THREAD_NETWORK_DATA_TLV
                        } <= set(p.coap.tlv.type) and
                        # Verify the prefixes are present in the CoAP payload (Thread Network Data TLV)
                        all(bytes(prefix)[:8] in p.coap.payload for prefix in [PREFIX_1, PREFIX_2, PREFIX_3])).\
      must_next()

    # Step 4: Leader
    # - Description: Automatically transmits a 2.04 Changed CoAP response to the DUT for each of the three Prefixes
    #   configured in Step 2. Automatically transmits multicast MLE Data Response with the new information collected
    #   from the DUT.
    # - Pass Criteria: N/A
    print("Step 4: Leader")
    pkts.filter_wpan_src64(LEADER).\
      filter_wpan_dst16(ROUTER_1_RLOC16).\
      filter_coap_ack(consts.SVR_DATA_URI).\
      must_next()

    index_after_ack = pkts.index

    # Confirm the data version updated.
    pkts.copy().\
      filter_wpan_src64(LEADER).\
      filter_mle_cmd(consts.MLE_ADVERTISEMENT).\
      filter(lambda p: p.mle.tlv.leader_data.data_version > 0).\
      must_next()

    # Step 5: Router_1 (DUT)
    # - Description: Automatically sends new network data to MED_1
    # - Pass Criteria: The DUT MUST multicast an MLE Data Response, including at least three Prefix TLVs (Prefix 1,
    #   Prefix2, and Prefix 3).
    print("Step 5: Router_1 (DUT)")
    # Check MLE Data Response from Router 1.
    pkts.copy().\
      filter_wpan_src64(ROUTER_1).\
      filter_mle_cmd(consts.MLE_DATA_RESPONSE).\
      filter(lambda p: {
                        consts.NETWORK_DATA_TLV
                       } <= set(p.mle.tlv.type) and
                       p.thread_nwd.tlv.prefix is not nullField and
                       {
                           PREFIX_1,
                           PREFIX_2,
                           PREFIX_3
                       } <= set(p.thread_nwd.tlv.prefix)).\
      must_next()

    # Step 6: MED_1
    # - Description: Automatically sends MLE Child Update Request to its parent (DUT), reporting its configured global
    #   addresses in the Address Registration TLV
    # - Pass Criteria: N/A
    print("Step 6: MED_1")
    pkts.copy().\
      filter_wpan_src64(MED_1).\
      filter_wpan_dst64(ROUTER_1).\
      filter_mle_cmd(consts.MLE_CHILD_UPDATE_REQUEST).\
      filter(lambda p: {
                        consts.ADDRESS_REGISTRATION_TLV
                        } <= set(p.mle.tlv.type) and
                        # ML-EID + 3 global addresses = 4 IIDs.
                        len(p.mle.tlv.addr_reg_iid) == 4).\
      must_next()

    # Step 7: Router_1 (DUT)
    # - Description: Automatically sends a MLE Child Update Response to MED_1, echoing back the configured addresses
    #   reported by MED_1
    # - Pass Criteria: The DUT MUST send a unicast MLE Child Update Response to MED_1, which includes the following
    #   TLVs:
    #   - Source Address TLV
    #   - Address Registration TLV
    #     - Echoes back the addresses configured by MED_1
    #   - Mode TLV
    print("Step 7: Router_1 (DUT)")
    pkts.copy().\
      filter_wpan_src64(ROUTER_1).\
      filter_wpan_dst64(MED_1).\
      filter_mle_cmd(consts.MLE_CHILD_UPDATE_RESPONSE).\
      filter(lambda p: {
                        consts.SOURCE_ADDRESS_TLV,
                        consts.ADDRESS_REGISTRATION_TLV,
                        consts.MODE_TLV
                        } <= set(p.mle.tlv.type)).\
      must_next()

    # Step 8A/B: Router_1 (DUT)
    # - Description: Automatically sends notification of new network data to SED_1
    # - Pass Criteria: The DUT MUST send MLE Child Update Request or Data Response to SED_1
    print("Step 8A/B: Router_1 (DUT)")
    # Reset index to after Ack because SED notification can happen before or after MED update.
    pkts.index = index_after_ack
    # Unicast packets should be decrypted correctly. We allow some leeway for SED poll timing.
    pkts.filter_wpan_src64(ROUTER_1).\
      filter_wpan_dst64(SED_1).\
      filter(lambda p: p.mle.cmd in {
                                    consts.MLE_CHILD_UPDATE_REQUEST,
                                    consts.MLE_DATA_RESPONSE
                                    }).\
      filter(lambda p: {
                        consts.SOURCE_ADDRESS_TLV,
                        consts.LEADER_DATA_TLV,
                        consts.NETWORK_DATA_TLV,
                        consts.ACTIVE_TIMESTAMP_TLV
                        } <= set(p.mle.tlv.type)).\
      filter(lambda p: p.thread_nwd.tlv.prefix is not nullField and
                       PREFIX_1 in p.thread_nwd.tlv.prefix and
                       PREFIX_3 in p.thread_nwd.tlv.prefix and
                       PREFIX_2 not in p.thread_nwd.tlv.prefix).\
      must_next()

    # Step 9: SED_1
    # - Description: Automatically sends global address configured to parent, in the Address Registration TLV from the
    #   Child Update request command.
    # - Pass Criteria: N/A
    print("Step 9: SED_1")
    pkts.filter_wpan_src64(SED_1).\
      filter_wpan_dst64(ROUTER_1).\
      filter_mle_cmd(consts.MLE_CHILD_UPDATE_REQUEST).\
      filter(lambda p: {
                        consts.ADDRESS_REGISTRATION_TLV
                        } <= set(p.mle.tlv.type) and
                        # Verify that the registered addresses correspond to Prefix 1 and 3, but not 2.
                        # ML-EID + 2 global addresses = 3 IIDs.
                        len(p.mle.tlv.addr_reg_iid) == 3 and
                        all(any(iid == int(addr[8:].hex(), 16) for iid in p.mle.tlv.addr_reg_iid)
                            for prefix in [PREFIX_1, PREFIX_3]
                            for addr in pv.vars['SED_1_IPADDRS'] if str(addr).startswith(str(prefix)[:5])) and
                        not any(any(iid == int(addr[8:].hex(), 16) for iid in p.mle.tlv.addr_reg_iid)
                                for addr in pv.vars['SED_1_IPADDRS'] if str(addr).startswith('2002:'))).\
      must_next()

    # Step 10: Router_1 (DUT)
    # - Description: Automatically sends a Child Update Response to SED_1, echoing back the configured addresses reported
    #   by SED_1
    # - Pass Criteria: The DUT MUST send a unicast MLE Child Update Response to SED_1, including the following TLVs:
    #   - Source Address TLV
    #   - Address Registration TLV
    #     - Echoes back the addresses configured by SED_1
    #   - Mode TLV
    print("Step 10: Router_1 (DUT)")
    pkts.filter_wpan_src64(ROUTER_1).\
      filter_wpan_dst64(SED_1).\
      filter_mle_cmd(consts.MLE_CHILD_UPDATE_RESPONSE).\
      filter(lambda p: {
                        consts.SOURCE_ADDRESS_TLV,
                        consts.ADDRESS_REGISTRATION_TLV,
                        consts.MODE_TLV
                        } <= set(p.mle.tlv.type)).\
      must_next()

    print("Verification PASSED")


if __name__ == '__main__':
    verify_utils.run_main(verify)
