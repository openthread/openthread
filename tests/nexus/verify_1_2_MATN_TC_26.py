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

# MLR Status values
ST_MLR_SUCCESS = 0
ST_MLR_NO_RESOURCES = 4
ST_MLR_GENERAL_FAILURE = 6


def verify(pv):
    # 5.10.22 MATN-TC-26: Multicast registrations error handling by Thread Device
    #
    # 5.10.22.1 Topology
    # - BR_1: Test bed BR device operating as the Primary BBR
    # - TD (DUT): Device operating as a Thread device - FED or Router
    #
    # 5.10.22.2 Purpose & Description
    # The purpose of this test case is to verify that a Thread Device can correctly handle multicast registration
    #   errors, whether that is due to a BBR running out of resources or a BBR responding with general failure.
    #
    # Spec Reference   | V1.2 Section
    # -----------------|-------------
    # Multicast        | 5.10.1

    pkts = pv.pkts
    pv.summary.show()

    BR_1 = pv.vars['BR_1']
    TD = pv.vars['TD']
    MA1 = pv.vars['MA1']
    BR_1_RLOC = pv.vars['BR_1_RLOC']

    # Step 0
    # - Device: N/A
    # - Description: Topology formation - BR_1. Topology formation - TD (DUT). The DUT must be booted and joined to
    #   the network.
    # - Pass Criteria:
    # - N/A
    print("Step 0: Topology formation – BR_1. Topology formation – TD (DUT).")

    # Step 1
    # - Device: BR_1
    # - Description: Harness instructs the device to not update its multicast listener table, and to prepare a
    #   MLE.rsp error condition.
    # - Pass Criteria:
    # - N/A
    print("Step 1: Harness instructs the device to not update its multicast listener table, and to prepare a MLR.rsp "
          "error condition.")

    # Step 2
    # - Device: TD (DUT)
    # - Description: The DUT must be configured to request registration of Multicast address MA1.
    # - Pass Criteria:
    # - The DUT MUST unicast an MLR.req CoAP request to BR_1 as follows: coap://[<BR_1 RLOC or PBBR ALOC>]:MM/n/mr
    # - Where the payload contains: IPv6 Addresses TLV: MA1
    print("Step 2: The DUT must be configured to request registration of Multicast address MA1.")
    pkts.filter_wpan_src64(TD).\
        filter_coap_request('/n/mr').\
        filter(lambda p: p.ipv6.dst == Ipv6Addr(BR_1_RLOC) or \
                         Ipv6Addr(p.ipv6.dst).endswith(Bytes('000000fffe00fc38'))).\
        filter(lambda p: MA1 in p.coap.tlv.ipv6_address).\
        must_next()

    # Step 3
    # - Device: BR_1
    # - Description: Automatically responds to the Multicast registration with an error. Informative: Both the
    #   actions together: set_next_mlr_status_rsp() Unicasts a MLR.rsp CoAP response to TD: 2.04 Changed Where the
    #   payload contains: Status TLV: 4 [ST_MLR_NO_RESOURCES] IPv6 Addresses TLV: MA1
    # - Pass Criteria:
    # - N/A
    print("Step 3: Automatically responds to the Multicast registration with an error.")
    pkts.filter_wpan_src64(BR_1).\
        filter_coap_ack('/n/mr').\
        filter(lambda p: p.coap.code == consts.COAP_CODE_ACK).\
        filter(lambda p: p.coap.tlv.status == ST_MLR_NO_RESOURCES).\
        filter(lambda p: MA1 in p.coap.tlv.ipv6_address).\
        must_next()

    # Step 3b
    # - Device: TD (DUT)
    # - Description: Receives above MLR.rsp CoAP response from BR_1. Within <Reregistration Delay> seconds,
    #   automatically retries the registration.
    # - Pass Criteria:
    # - Within <Reregistration Delay> seconds after receiving the MLR.rsp from BR_1, the DUT MUST retry the
    #   registration using the CoAP message as in step 1 pass criteria.
    print("Step 3b: Receives above MLR.rsp CoAP response from BR_1. Within <Reregistration Delay> seconds, "
          "automatically retries the registration.")
    pkts.filter_wpan_src64(TD).\
        filter_coap_request('/n/mr').\
        filter(lambda p: p.ipv6.dst == Ipv6Addr(BR_1_RLOC) or \
                         Ipv6Addr(p.ipv6.dst).endswith(Bytes('000000fffe00fc38'))).\
        filter(lambda p: MA1 in p.coap.tlv.ipv6_address).\
        must_next()

    # Step 3c
    # - Device: BR_1
    # - Description: Automatically updates its multicast listeners table and responds to the multicast registration.
    #   Unicasts an MLR.rsp CoAP response to TD as follows: 2.04 changed Where the payload contains: Status TLV: 0
    #   [ST_MLR_SUCCESS]
    # - Pass Criteria:
    # - N/A
    print("Step 3c: Automatically updates its multicast listeners table and responds to the multicast registration.")
    pkts.filter_wpan_src64(BR_1).\
        filter_coap_ack('/n/mr').\
        filter(lambda p: p.coap.code == consts.COAP_CODE_ACK).\
        filter(lambda p: p.coap.tlv.status == ST_MLR_SUCCESS).\
        must_next()

    # Step 4
    # - Device: BR_1
    # - Description: Harness instructs the device to not update its multicast listener table, and to prepare a
    #   MLE.rsp error condition.
    # - Pass Criteria:
    # - N/A
    print("Step 4: Harness instructs the device to not update its multicast listener table, and to prepare a MLR.rsp "
          "error condition.")

    # Step 4 (cont.)
    # - Device: BR_1
    # - Description: Harness instructs the device to updates the network data (BBR Dataset) as follows: BBR Sequence
    #   number: <New value = Previous Value +1>. The device then automatically starts MLE dissemination of the new
    #   dataset.
    # - Pass Criteria:
    # - N/A
    # - Note: MLDv2 and BLMR.ntf checks are intentionally skipped as they are not the focus of this test.
    print("Step 4 (cont.): Harness instructs the device to updates the network data (BBR Dataset).")

    # Step 5
    # - Device: TD (DUT)
    # - Description: After noticing the change in BBR Sequence number, the DUT automatically requests to register
    #   multicast address, MA1, at BR_1.
    # - Pass Criteria:
    # - Within <Reregistration Delay> seconds of receiving the new network data, the DUT MUST unicasts an MLR.req
    #   CoAP request to BR_1 as follows: coap://[<BR_1 RLOC or PBBR ALOC>]:MM/n/mr
    # - Where the payload contains: IPv6 Addresses TLV: MA1
    print("Step 5: After noticing the change in BBR Sequence number, the DUT automatically requests to register "
          "multicast address, MA1, at BR_1.")
    pkts.filter_wpan_src64(TD).\
        filter_coap_request('/n/mr').\
        filter(lambda p: p.ipv6.dst == Ipv6Addr(BR_1_RLOC) or \
                         Ipv6Addr(p.ipv6.dst).endswith(Bytes('000000fffe00fc38'))).\
        filter(lambda p: MA1 in p.coap.tlv.ipv6_address).\
        must_next()

    # Step 7
    # - Device: BR_1
    # - Description: Automatically responds to the DUT’s Multicast registration with another error. Informative:
    #   Both the actions together: set_next_mlr_status_rsp() Automatically unicasts a MLR.rsp CoAP response to TD:
    #   2.04 Changed Where the payload contains: Status TLV: 6 [ST_MLR_GENERAL_FAILURE]
    # - Pass Criteria:
    # - N/A
    print("Step 7: Automatically responds to the DUT’s Multicast registration with another error.")
    pkts.filter_wpan_src64(BR_1).\
        filter_coap_ack('/n/mr').\
        filter(lambda p: p.coap.code == consts.COAP_CODE_ACK).\
        filter(lambda p: p.coap.tlv.status == ST_MLR_GENERAL_FAILURE).\
        filter(lambda p: MA1 in p.coap.tlv.ipv6_address).\
        must_next()

    # Step 7b
    # - Device: TD (DUT)
    # - Description: The DUT receives above CoAP response from BR_1. Within <Reregistration Delay> seconds, it
    #   retries the registration.
    # - Pass Criteria:
    # - Within <Reregistration Delay> seconds after receiving the MLR.rsp from BR_1, the DUT MUST retries the
    #   registration using the CoAP message as in step 5 of pass criteria.
    print("Step 7b: The DUT receives above CoAP response from BR_1. Within <Reregistration Delay> seconds, it retries "
          "the registration.")
    pkts.filter_wpan_src64(TD).\
        filter_coap_request('/n/mr').\
        filter(lambda p: p.ipv6.dst == Ipv6Addr(BR_1_RLOC) or \
                         Ipv6Addr(p.ipv6.dst).endswith(Bytes('000000fffe00fc38'))).\
        filter(lambda p: MA1 in p.coap.tlv.ipv6_address).\
        must_next()

    # Step 7c
    # - Device: BR_1
    # - Description: Automatically updates its multicast listeners table and responds to the multicast registration.
    #   Unicasts an MLR.rsp CoAP response to TD as follows: 2.04 changed Where the payload contains: Status TLV: 0
    #   [ST_MLR_SUCCESS]
    # - Pass Criteria:
    # - N/A
    print("Step 7c: Automatically updates its multicast listeners table and responds to the multicast registration.")
    pkts.filter_wpan_src64(BR_1).\
        filter_coap_ack('/n/mr').\
        filter(lambda p: p.coap.code == consts.COAP_CODE_ACK).\
        filter(lambda p: p.coap.tlv.status == ST_MLR_SUCCESS).\
        must_next()

    # Step 8
    # - Device: BR_1
    # - Description: Harness instructs the device to Updates the network data (BBR dataset) as follows: BBR Sequence
    #   number: <New value := Previous Value +1>. The device then automatically starts MLE dissemination of the new
    #   dataset.
    # - Pass Criteria:
    # - N/A
    print("Step 8: Harness instructs the device to Updates the network data (BBR dataset).")

    # Step 9
    # - Device: TD (DUT)
    # - Description: After noticing the change in BBR Sequence number, the DUT automatically registers for
    #   multicast address, MA1, at BR_1.
    # - Pass Criteria:
    # - Within <Reregistration Delay> seconds of receiving the new network data, the DUT MUST unicast an MLR.req
    #   CoAP request to BR_1 as follows: coap://[<BR_1 RLOC or PBBR ALOC>]:MM/n/mr
    # - Where the payload contains: IPv6 Addresses TLV: MA1
    print("Step 9: After noticing the change in BBR Sequence number, the DUT automatically registers for multicast "
          "address, MA1, at BR_1.")
    pkts.filter_wpan_src64(TD).\
        filter_coap_request('/n/mr').\
        filter(lambda p: p.ipv6.dst == Ipv6Addr(BR_1_RLOC) or \
                         Ipv6Addr(p.ipv6.dst).endswith(Bytes('000000fffe00fc38'))).\
        filter(lambda p: MA1 in p.coap.tlv.ipv6_address).\
        must_next()

    # Step 10
    # - Device: BR_1
    # - Description: Automatically updates its multicast listener table.
    # - Pass Criteria:
    # - N/A
    print("Step 10: Automatically updates its multicast listener table.")

    # Step 11
    # - Device: BR_1
    # - Description: Automatically responds to the multicast registration. Unicasts an MLR.rsp CoAP response to TD
    #   as follows: 2.04 changed Where the payload contains: Status TLV: 0 [ST_MLR_SUCCESS]
    # - Pass Criteria:
    # - N/A
    print("Step 11: Automatically responds to the multicast registration.")
    pkts.filter_wpan_src64(BR_1).\
        filter_coap_ack('/n/mr').\
        filter(lambda p: p.coap.code == consts.COAP_CODE_ACK).\
        filter(lambda p: p.coap.tlv.status == ST_MLR_SUCCESS).\
        must_next()

    # Step 12
    # - Device: TD (DUT)
    # - Description: Does not retry the registration within the next (0.5 * MLR Timeout).
    # - Pass Criteria:
    # - The DUT MUST NOT send an MLR.req message containing MA1 for at least 150 seconds.
    # - Note: MLDv2 and BLMR.ntf checks are intentionally skipped as they are not the focus of this test.
    print("Step 12: Does not retry the registration within the next (0.5 * MLR Timeout).")
    pkts.filter_wpan_src64(TD).\
        filter_coap_request('/n/mr').\
        filter(lambda p: MA1 in p.coap.tlv.ipv6_address).\
        must_not_next()


if __name__ == '__main__':
    verify_utils.run_main(verify)
