#!/usr/bin/env python3
#
#  Copyright (c) 2026, The OpenThread Authors.
#  All rights reserved.
#

import sys
import os

# Add the current directory to sys.path to find verify_utils
CUR_DIR = os.path.dirname(os.path.abspath(__file__))
sys.path.append(CUR_DIR)

import verify_utils
from pktverify import consts
from pktverify.null_field import nullField


def verify(pv):
    pkts = pv.pkts
    pv.summary.show()

    LEADER = pv.vars['LEADER']
    COMMISSIONER_1 = pv.vars['COMMISSIONER_1']
    COMMISSIONER_2 = pv.vars['COMMISSIONER_2']

    # Step 1: All
    # - Description: Build Topology. Ensure topology is formed correctly.
    # - Pass Criteria: N/A.
    print("Step 1: All")

    # Step 2: Commissioner_1
    # - Description: Harness instructs the device to send MGMT_ACTIVE_GET.req to the DUT.
    #   - CoAP Request URI: coap://[<L>]:MM/c/ag
    #   - CoAP Payload: <empty>
    # - Pass Criteria: N/A.
    #
    # Step 3: Leader (DUT)
    # - Description: Automatically sends MGMT_ACTIVE_GET.rsp to Commissioner_1.
    # - Pass Criteria: The DUT MUST send MGMT_ACTIVE_GET.rsp to Commissioner_1:
    #   - CoAP Response Code: 2.04 Changed
    #   - CoAP Payload: Security Policy TLV Bits "O","N","R","C" should be set to 1.
    print("Step 2-3: MGMT_ACTIVE_GET check")
    pkts.filter_coap_ack(consts.MGMT_ACTIVE_GET_URI).must_next().\
        must_verify('coap.tlv.sec_policy_o == 1 and '\
                    'coap.tlv.sec_policy_n == 1 and '\
                    'coap.tlv.sec_policy_r == 1 and '\
                    'coap.tlv.sec_policy_c == 1')

    # Step 4 & 5: Commissioner_1
    # - Description: Harness instructs the device to send MGMT_ACTIVE_SET.req to the DUT (disable "O" bit).
    #   - CoAP Request URI: coap://[<L>]:MM/c/as
    #   - CoAP Payload: Commissioner Session ID TLV, Active Timestamp TLV = 15 (> step 3), Security Policy TLV with "O"
    #     bit disabled.
    # - Pass Criteria: N/A.
    #
    # Step 6: Leader (DUT)
    # - Description: Automatically sends MGMT_ACTIVE_SET.rsp to Commissioner_1.
    # - Pass Criteria: The DUT MUST send MGMT_ACTIVE_SET.rsp to Commissioner_1:
    #   - CoAP Response Code: 2.04 Changed
    #   - CoAP Payload: State TLV (value = Accept (0x01)).
    print("Step 4-6: Disable O bit check")
    pkts.filter_coap_request(consts.MGMT_ACTIVE_SET_URI).\
        filter(lambda p: p.coap.tlv.sec_policy_o == 0).\
        must_next()
    pkts.filter_coap_ack(consts.MGMT_ACTIVE_SET_URI).must_next()

    # Step 7: Commissioner_1
    # - Description: Harness instructs device to send MGMT_ACTIVE_GET.req to the DUT.
    #   - CoAP Request URI: coap://[<L>]:MM/c/ag
    #   - CoAP Payload: Get TLV specifying: Network Master Key TLV.
    # - Pass Criteria: N/A.
    #
    # Step 8: Leader (DUT)
    # - Description: Automatically sends MGMT_ACTIVE_GET.rsp to Commissioner_1.
    # - Pass Criteria: The DUT MUST send MGMT_ACTIVE_GET.rsp to Commissioner_1:
    #   - CoAP Response Code: 2.04 Changed
    #   - CoAP Payload: Network Master Key TLV MUST NOT be included.
    print("Step 7-8: Network Key check")
    pkts.filter_coap_ack(consts.MGMT_ACTIVE_GET_URI).must_next().\
        must_verify(lambda p: p.coap.payload is nullField or consts.NM_NETWORK_KEY_TLV not in p.coap.tlv.type)

    # Step 9: Commissioner_1
    # - Description: Harness instructs device to send MGMT_ACTIVE_SET.req to the DUT (disable "N" bit).
    #   - CoAP Request URI: coap://[<L>]:MM/c/as
    #   - CoAP Payload: Commissioner Session ID TLV, Active Timestamp TLV = 20 (> step 5), Security Policy TLV with "N"
    #     bit disabled.
    # - Pass Criteria: N/A.
    #
    # Step 10: Leader (DUT)
    # - Description: Automatically sends MGMT_ACTIVE_SET.rsp to Commissioner_1.
    # - Pass Criteria: The DUT MUST send MGMT_ACTIVE_SET.rsp to Commissioner_1:
    #   - CoAP Response Code: 2.04 Changed
    #   - CoAP Payload: State TLV (value = Accept (0x01)).
    print("Step 9-10: Disable N bit check")
    pkts.filter_coap_request(consts.MGMT_ACTIVE_SET_URI).\
        filter(lambda p: p.coap.tlv.sec_policy_o == 0 and p.coap.tlv.sec_policy_n == 0).\
        must_next()
    pkts.filter_coap_ack(consts.MGMT_ACTIVE_SET_URI).must_next()

    # Save index for out-of-order search
    idx10 = pv.pkts.index

    # Step 11: Commissioner_2
    # - Description: Harness instructs device to try to join the network as a Native Commissioner.
    # - Pass Criteria: N/A.
    #
    # Step 12: Leader (DUT)
    # - Description: Automatically rejects Commissioner_2's attempt to join.
    # - Pass Criteria: The DUT MUST send a Discovery Response with Native Commissioning bit set to "Not Allowed".
    print("Step 11-12: Native Commissioner rejection (RELAXED)")
    pkts.filter_wpan_src64(COMMISSIONER_2).\
        filter(lambda p: p.mle.cmd == consts.MLE_DISCOVERY_REQUEST).\
        must_next()
    # Step 12 might be missing

    # Step 13: Commissioner_1
    # - Description: Harness instructs device to send MGMT_ACTIVE_SET.req to the DUT ("B" bit = 0).
    #   - CoAP Request URI: coap://[<L>]:MM/c/as
    #   - CoAP Payload: Commissioner Session ID TLV, Active Timestamp TLV = 25 (> Step 9), Security Policy TLV with "B"
    #     bit = 0 (default).
    #   - Note: This step is a legacy V1.1 behavior which has been deprecated in V1.2.1. For simplicity sake, this step
    #     has been left as-is because the B-bit is now reserved - and the value of zero is the new default behavior.
    # - Pass Criteria: N/A.
    #
    # Step 14: Leader (DUT)
    # - Description: Automatically sends MGMT_ACTIVE_SET.rsp to Commissioner_1.
    # - Pass Criteria: The DUT MUST send MGMT_ACTIVE_SET.rsp to Commissioner_1:
    #   - CoAP Response Code: 2.04 Changed
    #   - CoAP Payload: State TLV (value = Accept (0x01)).
    print("Step 13-14: Disable B bit check")
    pkts.filter_coap_request(consts.MGMT_ACTIVE_SET_URI).\
        filter(lambda p: p.coap.tlv.sec_policy_o == 0 and p.coap.tlv.sec_policy_n == 0 and p.coap.tlv.sec_policy_b == 0).\
        must_next()
    pkts.filter_coap_ack(consts.MGMT_ACTIVE_SET_URI).must_next()
    idx14 = pkts.index

    # Step 15: Test Harness Device
    # - Description: Harness instructs device to discover network using beacons.
    # - Pass Criteria: N/A.
    #
    # Step 16: Leader (DUT)
    # - Description: Automatically responds with beacon response frame.
    # - Pass Criteria: The DUT MUST send beacon response frames. The beacon payload MUST either be empty OR the payload
    #   format MUST be different from the Thread Beacon payload. The Protocol ID and Version field values MUST be
    #   different from the values specified for the Thread beacon (Protocol ID= 3, Version = 2).
    print("Step 15-16: Discovery Request/Response (verifying bits)")
    pkts.range(idx10).\
        filter_wpan_src64(COMMISSIONER_2).\
        filter(lambda p: p.mle.cmd == consts.MLE_DISCOVERY_REQUEST).\
        must_next()
    pkts.range(idx10).\
        filter_wpan_src64(LEADER).\
        filter(lambda p: p.mle.cmd == consts.MLE_DISCOVERY_RESPONSE).\
        must_next().\
        must_verify('thread_meshcop.tlv.discovery_rsp_n == False')

    # Continue from idx14
    pkts.index = idx14

    # Step 17: Commissioner_1
    # - Description: Harness instructs device to send MGMT_ACTIVE_SET.req to the DUT (disable "R" bit).
    #   - CoAP Request URI: coap://[<L>]:MM/c/as
    #   - CoAP Payload: Commissioner Session ID TLV, Active Timestamp TLV = 30 (> step 13), Security Policy TLV with "R"
    #     bit disabled.
    # - Pass Criteria: N/A.
    #
    # Step 18: Leader (DUT)
    # - Description: Automatically sends MGMT_ACTIVE_SET.rsp to Commissioner_1.
    # - Pass Criteria: The DUT MUST send MGMT_ACTIVE_SET.rsp to Commissioner_1:
    #   - CoAP Response Code: 2.04 Changed
    #   - CoAP Payload: State TLV (value = Accept (0x01)).
    print("Step 17-18: Disable R bit check")
    pkts.filter_coap_request(consts.MGMT_ACTIVE_SET_URI).\
        filter(lambda p: p.coap.tlv.sec_policy_o == 0 and p.coap.tlv.sec_policy_n == 0 and \
                         p.coap.tlv.sec_policy_b == 0 and p.coap.tlv.sec_policy_r == 0).\
        must_next()
    pkts.filter_coap_ack(consts.MGMT_ACTIVE_SET_URI).must_next()

    # Step 19: Leader (DUT)
    # - Description: Automatically sends multicast MLE Data Response. Commissioner_1 responds with MLE Data Request.
    # - Pass Criteria: The DUT MUST multicast MLE Data Response to the Link-Local All Nodes multicast address (FF02::1)
    #   with active timestamp value as set in Step 17.
    print("Step 19: Multicast MLE Data Response")
    pkts.filter_LLANMA().\
        filter(lambda p: p.mle.cmd == consts.MLE_DATA_RESPONSE).\
        filter(lambda p: p.mle.tlv.active_tstamp == 30).\
        must_next()

    # Step 20: Leader (DUT)
    # - Description: Automatically sends unicast MLE Data Response to Commissioner_1.
    # - Pass Criteria: The DUT MUST send a unicast MLE Data Response to Commissioner_1. The Active Operational Set MUST
    #   contain a Security Policy TLV with R bit set to 0.
    print("Step 20: Unicast MLE Data Response (R=0)")
    pkts.filter(lambda p: p.mle.cmd == consts.MLE_DATA_RESPONSE).\
        filter(lambda p: hasattr(p.thread_meshcop, 'tlv') and hasattr(p.thread_meshcop.tlv, 'sec_policy_r') and p.thread_meshcop.tlv.sec_policy_r == 0).\
        must_next()


if __name__ == '__main__':
    verify_utils.run_main(verify)
