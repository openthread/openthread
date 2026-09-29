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
from pktverify import consts, errors

MLR_URI = '/n/mr'
ST_MLR_BBR_NOT_PRIMARY = 5


def verify(pv):
    # 5.11.2 BBR-TC-02: BBR MUST remove its BBR Dataset from Network Data
    #
    # 5.11.2.1 Topology
    # - BR_1 (DUT)
    # - BR_2
    # - Router_1
    #
    # 5.11.2.2 Purpose & Description
    # The purpose of this test case is to verify that if two BBR Datasets are present in a network, the DUT BR not
    #   being elected as Primary will delete its own BBR Dataset from the Network Data according to the rules of
    #   [ThreadSpec] Section 5.10.3.6, which are referred from Section 5.21.4.3. Note: in the present test procedure,
    #   only a subset of Section 5.10.3.6/5.21.4.3 rules is checked; other checks should be added in the future.
    #
    # Spec Reference   | V1.2 Section | V1.3.0 Section
    # -----------------|--------------|---------------
    # BBR Role Switch  | 5.10.3.6     | 5.21.4.3

    pkts = pv.pkts
    pv.summary.show()

    BR_1 = pv.vars['BR_1']
    BR_2 = pv.vars['BR_2']
    ROUTER_1 = pv.vars['Router_1']

    # Step 0
    # - Device: N/A
    # - Description: Topology formation - Router_1 (Leader). Topology addition - BR_1 (DUT). The DUT must be booted
    #   and joined to the test network
    # - Pass Criteria:
    #   - N/A
    print("Step 0: Topology formation")

    # Step 1
    # - Device: BR_1 (DUT)
    # - Description: Automatically receives Network Data and determines to send its BBR Dataset to the Leader.
    # - Pass Criteria:
    #   - The DUT MUST unicast SVR_DATA.ntf to the Leader as follows: coap://[<Router_1 RLOC>]:MM/a/sd
    #   - Where the payload contains inside the Thread Network Data TLV: Service TLV: BBR Dataset with
    #     - T=1
    #     - S_service_data Length = 1
    #     - S_service_data = 0x01 (THREAD_SERVICE_DATA_BBR)
    #     - One Server TLV as sub-TLV formatted per 5.21.3.2 [ThreadSpec].
    print("Step 1: BR_1 (DUT) sends its BBR Dataset to the Leader")
    pkts.filter_wpan_src64(BR_1).\
        filter_coap_request(consts.SVR_DATA_URI).\
        filter(lambda p: consts.NL_THREAD_NETWORK_DATA_TLV in p.coap.tlv.type).\
        must_next().\
        must_verify("coap.tlv.bbr_seqno < 127")

    # Step 2
    # - Device: Router_1
    # - Description: Automatically responds to the Server Data notification as follows: 2.04 changed. Also it MUST be
    #   verified here (or alternatively at the end of the test) that the received BBR Dataset contains: BBR Sequence
    #   Number < 127
    # - Pass Criteria:
    #   - The Received BBR Dataset MUST contain: BBR Sequence Number < 127
    print("Step 2: Router_1 responds to the Server Data notification")
    pkts.filter_wpan_src64(ROUTER_1).\
        filter_coap_ack(consts.SVR_DATA_URI).\
        must_next()

    # Step 3
    # - Device: BR_2
    # - Description: Topology addition - BR_2. Harness instructs the device to attach to the network. It automatically
    #   receives the Network Data, detects that there is already a BBR Dataset, and does not send its own BBR Dataset
    #   to Leader.
    # - Pass Criteria:
    #   - N/A
    print("Step 3: Topology addition - BR_2")

    # Step 4
    # - Device: BR_2
    # - Description: Harness instructs the device to set the BBR Sequence Number to 255. [Harness instructs the device
    #   to send a Coap message formatted like SVR_DATA.ntf to Leader with BBR Dataset with highest possible BBR
    #   Sequence Number (255). SVR_DATA.ntf contains a Thread Network Data TLV, with inside: Service TLV: BBR Dataset
    #   with
    #   - T=1
    #   - S_service_data Length = 1
    #   - S_service_data = 0x01 (THREAD_SERVICE_DATA_BBR)
    #   - One Server TLV as sub-TLV formatted per 5.21.3.2 [ThreadSpec] and values as specified below. In above Server
    #     TLV,
    #     - BBR Sequence Number is 255
    #     - Reregistration Delay is 5
    #     - MLR Timeout is 3600]
    # - Pass Criteria:
    #   - N/A
    print("Step 4: BR_2 sets the BBR Sequence Number to 255")
    pkts.filter_wpan_src64(BR_2).\
        filter_coap_request(consts.SVR_DATA_URI).\
        filter(lambda p: consts.NL_THREAD_NETWORK_DATA_TLV in p.coap.tlv.type).\
        must_next().\
        must_verify("coap.tlv.bbr_seqno == 255")

    # Step 5
    # - Device: Router_1
    # - Description: Automatically distributes new Network Data including the "fake" BBR Dataset of BR_2 that was
    #   submitted in the previous step. This means the Network Data now contains 2 BBR Datasets.
    # - Pass Criteria:
    #   - N/A
    print("Step 5: Router_1 distributes new Network Data")

    # Step 6
    # - Device: BR_1 (DUT)
    # - Description: Receives new Network Data, automatically detects that there are two BBR Datasets; and that its own
    #   BBR Sequence Number is lower than the other's i.e. < 255. Automatically switches from Primary to Secondary BBR
    #   role and sends SVR_DATA.ntf to the Leader, this time excluding its BBR Dataset, in order to delete its BBR
    #   Dataset at the Leader.
    # - Pass Criteria:
    #   - The DUT MUST unicast SVR_DATA.ntf to Leader as follows: coap://[<Router_1 RLOC>]:MM/a/sd
    #   - The Thread Network Data TLV MUST NOT contain the following: Service TLV: BBR Dataset
    #     - T=1
    #     - S_service_data Length = 1
    #     - S_service_data = 0x01 (THREAD_SERVICE_DATA_BBR)
    print("Step 6: BR_1 (DUT) switches to Secondary BBR role and deletes its BBR Dataset")
    pkts.filter_wpan_src64(BR_1).\
        filter_coap_request(consts.SVR_DATA_URI).\
        filter(lambda p: consts.NL_THREAD_NETWORK_DATA_TLV in p.coap.tlv.type).\
        must_next().\
        must_not_verify("coap.tlv.bbr_seqno is not null")

    # Step 6b
    # - Device: BR_2
    # - Description: Receives new Network Data, detects there there are two BBR Datasets; and that its own BBR Dataset
    #   is included with BBR Sequence Number higher than the other's (i.e. it is 255). It then automatically switches
    #   from Secondary to Primary BBR role. Informational: the BBR Sequence Number of BR_2 will remain 255 after the
    #   role switch.
    # - Pass Criteria:
    #   - N/A
    print("Step 6b: BR_2 switches to Primary BBR role")

    # Step 7
    # - Device: Router_1
    # - Description: Receives the SVR_DATA.ntf and automatically responds 2.04 Changed. Automatically distributes the
    #   new Network Data with only one BBR Dataset (the one of BR_2).
    # - Pass Criteria:
    #   - N/A
    print("Step 7: Router_1 distributes new Network Data with only BR_2's BBR Dataset")

    # Step 8
    # - Device: BR_1 (DUT)
    # - Description: Receives the new Network Data and automatically determines not to reregister any new BBR Dataset.
    #   It remains in Secondary BBR role.
    # - Pass Criteria:
    #   - The DUT MAY send SVR_DATA.ntf
    #   - In SVR_DATA.ntf, the Thread Network Data TLV MUST NOT contain the following (as in step 6): Service TLV: BBR
    #     Dataset
    #     - T=1
    #     - S_service_data Length = 1
    #     - S_service_data = 0x01 (THREAD_SERVICE_DATA_BBR)
    print("Step 8: BR_1 (DUT) remains in Secondary BBR role")
    # If BR_1 sends SVR_DATA.ntf, it must not contain its BBR dataset.
    with pkts.save_index():
        p8 = pkts.filter_wpan_src64(BR_1).\
            filter_coap_request(consts.SVR_DATA_URI).\
            filter(lambda p: consts.NL_THREAD_NETWORK_DATA_TLV in p.coap.tlv.type).\
            next()
        if p8:
            p8.must_not_verify("coap.tlv.bbr_seqno is not null")

    # Step 9
    # - Device: BR_2
    # - Description: Receives the new Network Data and remains in Primary BBR role.
    # - Pass Criteria:
    #   - N/A
    print("Step 9: BR_2 remains in Primary BBR role")

    # Step 10
    # - Device: Router_1
    # - Description: Harness instructs the device to send a MLR.req message to register for multicast group MA1 to the
    #   Secondary BBR, BR_1 (the DUT).
    # - Pass Criteria:
    #   - N/A
    print("Step 10: Router_1 sends MLR.req to BR_1 (DUT)")
    pkts.filter_wpan_src64(ROUTER_1).\
        filter_coap_request(MLR_URI).\
        must_next()

    # Step 11
    # - Device: BR_1 (DUT)
    # - Description: Automatically responds with an error to the MLR.req, since it is not Primary BBR anymore.
    # - Pass Criteria:
    #   - unicast an MLR.rsp CoAP response to Router_1 as follows: 2.04 changed
    #   - Where the payload contains: Status TLV: 5 [ST_MLR_BBR_NOT_PRIMARY]
    print("Step 11: BR_1 (DUT) responds with an error to the MLR.req")
    # Skip checks for MLDv2 and BLMR.ntf as requested.
    pkts.filter_wpan_src64(BR_1).\
        filter_coap_ack(MLR_URI).\
        filter(lambda p: p.coap.tlv.status == ST_MLR_BBR_NOT_PRIMARY).\
        must_next()

    # Step 12
    # - Device: BR_2
    # - Description: Harness instructs the device to stop being a Router, so that in the coming steps it cannot
    #   assume the Leader role. Note: in OT CLI this can be done by: routereligible disable
    # - Pass Criteria:
    #   - N/A
    print("Step 12: BR_2 stops being a Router")

    # Step 13
    # - Device: Router_1
    # - Description: Harness instructs the device to stop being a Leader. Note: in OT CLI this can be done by:
    #   routereligible disable
    # - Pass Criteria:
    #   - N/A
    print("Step 13: Router_1 stops being a Leader")

    # Step 14
    # - Device: BR_1 (DUT)
    # - Description: After some time, automatically becomes the Leader and Primary BBR.
    # - Pass Criteria:
    #   - MUST become the Leader of the network.
    #   - MUST be Primary BBR. This can be verified by checking that the network data in Router_1 contains the BBR
    #     Dataset of DUT. The OT CLI command "bbr" on Router_1 can also be used for verification.
    print("Step 14: BR_1 (DUT) becomes Leader and Primary BBR")
    # Verify BR_1 becomes Leader
    pkts.filter_wpan_src64(BR_1).\
        filter_mle_advertisement('Leader').\
        must_next()

    # Verify BR_1 distributes its BBR Dataset in Network Data
    pkts.filter_wpan_src64(BR_1).\
        filter_mle_cmd(consts.MLE_DATA_RESPONSE).\
        filter(lambda p: consts.NETWORK_DATA_TLV in p.mle.tlv.type).\
        filter_has_bbr_dataset().\
        must_next()


if __name__ == '__main__':
    verify_utils.run_main(verify)
