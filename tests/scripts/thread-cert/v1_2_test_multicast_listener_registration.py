#!/usr/bin/env python3
#
#  Copyright (c) 2020, The OpenThread Authors.
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
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 'AS IS'
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

import ipaddress
import unittest

import config
import network_layer
import thread_cert

_, BBR_1, BBR_2, ROUTER_1_2, ROUTER_1_1, SED_1, MED_1, FED_1 = range(8)

WAIT_ATTACH = 5
WAIT_REDUNDANCE = 3
ROUTER_SELECTION_JITTER = 1
BBR_REGISTRATION_JITTER = 5
SED_POLL_PERIOD = 1000  # 1s

REREG_DELAY = 10
MLR_TIMEOUT = 300

MA1 = 'ff04::1234:777a:1'
MA1g = 'ff0e::1234:777a:1'
MA2 = 'ff05::1234:777a:2'
MA3 = 'ff0e::1234:777a:3'
MA4 = 'ff05::1234:777a:4'

MA5 = 'ff03::1234:777a:5'
MA6 = 'ff02::10'
"""
 Initial topology:

   BBR_1---BBR_2
     |     /   \                       |
     |    /     \                      |
   ROUTER_1_2  ROUTER_1_1
    |     |  \                         |
    |     |   \                        |
  SED_1  MED_1 FED_1
"""


class TestMulticastListenerRegistration(thread_cert.TestCase):
    TOPOLOGY = {
        BBR_1: {
            'version': '1.2',
            'whitelist': [BBR_2, ROUTER_1_2],
            'is_bbr': True,
            'router_selection_jitter': 1,
        },
        BBR_2: {
            'version': '1.2',
            'whitelist': [BBR_1, ROUTER_1_2, ROUTER_1_1],
            'is_bbr': True,
            'router_selection_jitter': 1,
        },
        ROUTER_1_2: {
            'version': '1.2',
            'whitelist': [BBR_1, BBR_2, SED_1, MED_1, FED_1],
            'router_selection_jitter': 1,
        },
        ROUTER_1_1: {
            'version': '1.1',
            'whitelist': [BBR_2],
            'router_selection_jitter': 1,
        },
        MED_1: {
            'mode': 'rsn',
            'version': '1.2',
            'whitelist': [ROUTER_1_2],
            'timeout': config.DEFAULT_CHILD_TIMEOUT,
        },
        SED_1: {
            'mode': 'sn',
            'version': '1.2',
            'whitelist': [ROUTER_1_2],
            'timeout': config.DEFAULT_CHILD_TIMEOUT,
        },
        FED_1: {
            'mode': 'rsdn',
            'version': '1.2',
            'whitelist': [ROUTER_1_2],
            'router_upgrade_threshold': 0,
            'timeout': config.DEFAULT_CHILD_TIMEOUT,
        },
    }
    """All nodes are created with default configurations"""

    def test(self):
        # starting context id
        context_id = 1

        # Bring up BBR_1, BBR_1 becomes Leader and Primary Backbone Router
        self.nodes[BBR_1].set_router_selection_jitter(ROUTER_SELECTION_JITTER)
        self.nodes[BBR_1].set_bbr_registration_jitter(BBR_REGISTRATION_JITTER)
        self.nodes[BBR_1].set_backbone_router(seqno=1,
                                              reg_delay=REREG_DELAY,
                                              mlr_timeout=MLR_TIMEOUT)
        self.nodes[BBR_1].start()
        WAIT_TIME = WAIT_ATTACH + ROUTER_SELECTION_JITTER
        self.simulator.go(WAIT_TIME)
        self.assertEqual(self.nodes[BBR_1].get_state(), 'leader')
        self.nodes[BBR_1].enable_backbone_router()
        WAIT_TIME = BBR_REGISTRATION_JITTER + WAIT_REDUNDANCE
        self.simulator.go(WAIT_TIME)
        self.assertEqual(self.nodes[BBR_1].get_backbone_router_state(),
                         'Primary')

        self.pbbr_seq = 1

        # Bring up BBR_2, BBR_2 becomes Router and Secondary Backbone Router
        self.nodes[BBR_2].set_router_selection_jitter(ROUTER_SELECTION_JITTER)
        self.nodes[BBR_2].set_bbr_registration_jitter(BBR_REGISTRATION_JITTER)
        self.nodes[BBR_2].set_backbone_router(seqno=2,
                                              reg_delay=REREG_DELAY,
                                              mlr_timeout=MLR_TIMEOUT)
        self.nodes[BBR_2].start()
        WAIT_TIME = WAIT_ATTACH + ROUTER_SELECTION_JITTER
        self.simulator.go(WAIT_TIME)
        self.assertEqual(self.nodes[BBR_2].get_state(), 'router')
        self.nodes[BBR_2].enable_backbone_router()
        WAIT_TIME = BBR_REGISTRATION_JITTER + WAIT_REDUNDANCE
        self.simulator.go(WAIT_TIME)
        self.assertEqual(self.nodes[BBR_2].get_backbone_router_state(),
                         'Secondary')

        self.simulator.set_lowpan_context(context_id, config.DOMAIN_PREFIX)
        domain_prefix_cid = context_id

        # Bring up ROUTER_1_2
        self.nodes[ROUTER_1_2].set_router_selection_jitter(
            ROUTER_SELECTION_JITTER)
        self.nodes[ROUTER_1_2].start()
        WAIT_TIME = WAIT_ATTACH + ROUTER_SELECTION_JITTER
        self.simulator.go(WAIT_TIME)
        self.assertEqual(self.nodes[ROUTER_1_2].get_state(), 'router')

        # Bring up ROUTER_1_1
        self.nodes[ROUTER_1_1].set_router_selection_jitter(
            ROUTER_SELECTION_JITTER)
        self.nodes[ROUTER_1_1].start()
        WAIT_TIME = WAIT_ATTACH + ROUTER_SELECTION_JITTER
        self.simulator.go(WAIT_TIME)
        self.assertEqual(self.nodes[ROUTER_1_1].get_state(), 'router')

        # Bring up FED_1
        self.nodes[FED_1].start()
        self.simulator.go(WAIT_ATTACH)
        self.assertEqual(self.nodes[FED_1].get_state(), 'child')

        # Bring up MED_1
        self.nodes[MED_1].start()
        self.simulator.go(WAIT_ATTACH)
        self.assertEqual(self.nodes[MED_1].get_state(), 'child')

        # Bring up SED_1
        self.nodes[SED_1].set_pollperiod(SED_POLL_PERIOD)
        self.nodes[SED_1].start()
        self.simulator.go(WAIT_ATTACH)
        self.assertEqual(self.nodes[SED_1].get_state(), 'child')

        # Verify MLR.req for each device when parent is 1.2
        self.__check_mlr_ok(ROUTER_1_2, is_ftd=True)
        self.__check_mlr_ok(FED_1, is_ftd=True)
        self.__check_mlr_ok(MED_1, is_ftd=False)
        self.__check_mlr_ok(SED_1, is_ftd=False)

        # Switch to parent 1.1
        self.__switch_to_1_1_parent()

        # Verify MLR.req for each device when parent is 1.1
        self.__check_mlr_ok(FED_1, is_ftd=True, is_parent_1p1=True)
        self.__check_mlr_ok(MED_1, is_ftd=False, is_parent_1p1=True)
        self.__check_mlr_ok(SED_1, is_ftd=False, is_parent_1p1=True)

        # Switch to parent 1.2
        self.__switch_to_1_2_parent()

    def __check_mlr_ok(self, id, is_ftd, is_parent_1p1=False):
        """Check if MLR works for the node"""
        # Add MA1 and send MLR.req
        print(
            "======== checking MLR: Node%d (%s), Parent=%s ========" %
            (id, 'FTD' if is_ftd else 'MTD', '1.1' if is_parent_1p1 else '1.2'))
        expect_mlr_req = is_ftd or is_parent_1p1

        for addr in [MA1, MA1g, MA2, MA3, MA4]:
            self.__check_ipmaddr_add(id, addr, expect_mlr_req=expect_mlr_req)

        for addr in [MA5, MA6]:
            self.__check_ipmaddr_add(id, addr, expect_mlr_req=False)
        print('=' * 120)

    def __check_ipmaddr_add(self, id, addr, expect_mlr_req=True):
        """Check MLR works for the added multicast address"""
        self.flush_all()
        self.nodes[id].add_ipmaddr(addr)
        self.assertTrue(self.nodes[id].has_ipmaddr(addr))
        self.simulator.go(3)

        reg_mas = self.__get_registered_MAs(id)
        if expect_mlr_req:
            assert ipaddress.IPv6Address(addr) in reg_mas, (addr, reg_mas)

            messages = self.simulator.get_messages_sent_by(BBR_1)
            messages.next_coap_message('2.04')

            self.__check_rereg(id, addr)
            self.__check_renewing(id, addr)
        else:
            assert ipaddress.IPv6Address(addr) not in reg_mas, (addr, reg_mas)

        self.nodes[id].del_ipmaddr(addr)
        self.simulator.go(1)

    def __get_registered_MAs(self, id):
        """Get MAs registered via MLR.req by the node"""
        messages = self.simulator.get_messages_sent_by(id)
        reg_mas = []
        while True:
            msg = messages.next_coap_message('0.02',
                                             '/n/mr',
                                             assert_enabled=False)
            if not msg:
                break
            addrs = msg.get_coap_message_tlv(network_layer.IPv6Addresses)
            reg_mas.extend(addrs)

        print('Node %d registered MAs: %s' % (id, reg_mas))
        return reg_mas

    def __check_renewing(self, id, addr):
        """Check if MLR works that a node can renew it's registered MAs"""
        self.flush_all()
        self.simulator.go(MLR_TIMEOUT + WAIT_REDUNDANCE)

        reg_mas = self.__get_registered_MAs(id)
        assert ipaddress.IPv6Address(addr) in reg_mas, (addr, reg_mas)

    def __check_rereg(self, id, addr):
        """Check if MLR works that a node can do MLR reregistration when necessary"""
        self.__check_rereg_seqno(id, addr)
        self.__check_rereg_pbbr_change(id, addr)

    def __check_rereg_seqno(self, id, addr):
        """Check if MLR works that a node can do MLR reregistration when PBBR seqno changes"""
        # Change seq on PBBR and expect MLR.req within REREG_DELAY
        self.flush_all()
        self.pbbr_seq = (self.pbbr_seq + 10) % 256
        self.nodes[BBR_1].set_backbone_router(seqno=self.pbbr_seq)
        self.simulator.go(REREG_DELAY + WAIT_REDUNDANCE)

        reg_mas = self.__get_registered_MAs(id)
        assert ipaddress.IPv6Address(addr) in reg_mas, (addr, reg_mas)

    def __check_rereg_pbbr_change(self, id, addr):
        """Check if MLR works that a node can do MLR reregistration when PBBR changes"""
        # Make BBR_2 to be Primary and expect MLR.req within REREG_DELAY
        self.flush_all()
        self.nodes[BBR_1].disable_backbone_router()
        self.simulator.go(BBR_REGISTRATION_JITTER + WAIT_REDUNDANCE)
        self.assertEqual(self.nodes[BBR_2].get_backbone_router_state(),
                         'Primary')
        self.simulator.go(REREG_DELAY + WAIT_REDUNDANCE)
        reg_mas = self.__get_registered_MAs(id)
        assert ipaddress.IPv6Address(addr) in reg_mas, (addr, reg_mas)

        # Restore BBR_1 to be Primary and BBR_2 to be Secondary
        self.nodes[BBR_2].disable_backbone_router()
        self.nodes[BBR_1].enable_backbone_router()
        self.simulator.go(BBR_REGISTRATION_JITTER + WAIT_REDUNDANCE)
        self.assertEqual(self.nodes[BBR_1].get_backbone_router_state(),
                         'Primary')
        self.nodes[BBR_2].enable_backbone_router()
        self.simulator.go(BBR_REGISTRATION_JITTER + WAIT_REDUNDANCE)
        self.assertEqual(self.nodes[BBR_2].get_backbone_router_state(),
                         'Secondary')

    def __switch_to_1_1_parent(self):
        """Check if MLR works when nodes are switching to a 1.1 parent"""
        # Add MA1 to EDs to prepare for parent switching
        print("=" * 10, "switching to 1.1 parent", '=' * 10)

        self.flush_all()

        self.nodes[FED_1].add_ipmaddr(MA1)
        self.nodes[MED_1].add_ipmaddr(MA1)
        self.nodes[SED_1].add_ipmaddr(MA1)

        self.simulator.go(REREG_DELAY + WAIT_REDUNDANCE)
        self.assertIn(ipaddress.IPv6Address(MA1),
                      self.__get_registered_MAs(FED_1))
        self.assertNotIn(ipaddress.IPv6Address(MA1),
                         self.__get_registered_MAs(MED_1))
        self.assertNotIn(ipaddress.IPv6Address(MA1),
                         self.__get_registered_MAs(SED_1))

        self.flush_all()

        # Turn off Router 1.2 and turn on Router 1.1
        self.nodes[ROUTER_1_2].stop()
        for id in [FED_1, MED_1, SED_1]:
            self.nodes[ROUTER_1_1].add_whitelist(self.nodes[id].get_addr64())
            self.nodes[id].add_whitelist(self.nodes[ROUTER_1_1].get_addr64())
            self.simulator.go(config.DEFAULT_CHILD_TIMEOUT + WAIT_REDUNDANCE)

            self.assertEqual(self.nodes[id].get_state(), 'child')
            self.assertEqual(self.nodes[id].get_router_id(),
                             self.nodes[ROUTER_1_1].get_router_id())

        self.simulator.go(REREG_DELAY + WAIT_REDUNDANCE)

        # Verify all FED send MLR.req within REREG_DELAY when parent is 1.1
        self.assertIn(ipaddress.IPv6Address(MA1),
                      self.__get_registered_MAs(FED_1))
        self.assertIn(ipaddress.IPv6Address(MA1),
                      self.__get_registered_MAs(MED_1))
        self.assertIn(ipaddress.IPv6Address(MA1),
                      self.__get_registered_MAs(SED_1))

        self.nodes[FED_1].del_ipmaddr(MA1)
        self.nodes[MED_1].del_ipmaddr(MA1)
        self.nodes[SED_1].del_ipmaddr(MA1)

        self.simulator.go(WAIT_REDUNDANCE)

    def __switch_to_1_2_parent(self):
        """Check if MLR works when nodes are switching to a 1.2 parent"""
        # Add MA1 to EDs to prepare for parent switching
        print("=" * 10, "switching to 1.2 parent", '=' * 10)

        self.flush_all()

        self.nodes[FED_1].add_ipmaddr(MA1)
        self.nodes[MED_1].add_ipmaddr(MA1)
        self.nodes[SED_1].add_ipmaddr(MA1)

        self.simulator.go(REREG_DELAY + WAIT_REDUNDANCE)
        self.assertIn(ipaddress.IPv6Address(MA1),
                      self.__get_registered_MAs(FED_1))
        self.assertIn(ipaddress.IPv6Address(MA1),
                      self.__get_registered_MAs(MED_1))
        self.assertIn(ipaddress.IPv6Address(MA1),
                      self.__get_registered_MAs(SED_1))

        self.flush_all()

        # Turn off Router 1.1 and turn on Router 1.2
        self.nodes[ROUTER_1_1].stop()
        self.nodes[ROUTER_1_2].start()
        for id in [FED_1, MED_1, SED_1]:
            self.simulator.go(config.DEFAULT_CHILD_TIMEOUT + WAIT_REDUNDANCE)

            self.assertEqual(self.nodes[id].get_state(), 'child')
            self.assertEqual(self.nodes[id].get_router_id(),
                             self.nodes[ROUTER_1_2].get_router_id())

        self.simulator.go(REREG_DELAY + WAIT_REDUNDANCE)

        # Verify only FTD sends MLR.req within REREG_DELAY when parent is 1.2
        self.assertIn(ipaddress.IPv6Address(MA1),
                      self.__get_registered_MAs(FED_1))

        # MED and SED might still send MLR.req during this period because it could be sending to it's 1.2 parent.

        self.nodes[FED_1].del_ipmaddr(MA1)
        self.nodes[MED_1].del_ipmaddr(MA1)
        self.nodes[SED_1].del_ipmaddr(MA1)

        self.simulator.go(WAIT_REDUNDANCE)


if __name__ == '__main__':
    unittest.main()
