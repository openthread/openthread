#!/usr/bin/env python3
#
#  Copyright (c) 2019, The OpenThread Authors.
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
import logging
from typing import Tuple

from pktverify import consts
from pktverify.consts import MLE_CHILD_ID_REQUEST, MLE_ADVERTISEMENT, MLE_CHILD_ID_RESPONSE
from pktverify.pcap_reader import PcapReader
from pktverify.summary import Summary
from pktverify.test_info import TestInfo
from pktverify.verify_result import VerifyResult


class PacketVerifier(object):
    """
    Base class for packet verifiers that runs the packet verification process
    """
    NET_NAME = "OpenThread"
    MC_PORT = 49191
    MM_PORT = 61631
    BB_PORT = 61631
    LLANMA = 'ff02::1'  # Link-Local All Nodes multicast address
    LLARMA = 'ff02::2'  # Link-Local All Routers multicast address
    RLANMA = 'ff03::1'  # realm-local all-nodes multicast address
    RLARMA = 'ff03::2'  # realm-local all-routers multicast address
    RLAMFMA = 'ff03::fc'  # realm-local ALL_MPL_FORWARDERS address
    LLABMA = 'ff32:40:fd00:7d03:7d03:7d03:0:3'  # Link-Local All BBRs multicast address

    def __init__(self, test_info_path, wireshark_prefs=None):
        logging.basicConfig(level=logging.INFO,
                            format='File "%(pathname)s", line %(lineno)d, in %(funcName)s\n'
                            '%(asctime)s - %(levelname)s - %(message)s')

        ti = TestInfo(test_info_path)
        if wireshark_prefs is not None:
            pkts = PcapReader.read(ti.pcap_path, wireshark_prefs)
        else:
            pkts = PcapReader.read(ti.pcap_path)
        print('loaded %d packets from %s' % (len(pkts), ti.pcap_path))
        self.pkts = pkts
        self.test_info = ti

        self.summary = Summary(pkts, ti)
        self._vars = {}
        self._add_initial_vars()

    def add_vars(self, **vars):
        """
        Add new variables.

        :param vars: The new variables.
        """
        self._vars.update(vars)

    @property
    def vars(self):
        """
        :return: the dict of all variables
        """
        return self._vars

    def add_common_vars(self):
        """
        Add common variables that is needed by many test cases.
        """
        self.add_vars(
            NET_NAME=PacketVerifier.NET_NAME,
            MM_PORT=PacketVerifier.MM_PORT,
            MC_PORT=PacketVerifier.MC_PORT,
            BB_PORT=PacketVerifier.BB_PORT,
            LLANMA=PacketVerifier.LLANMA,  # Link-Local All Nodes multicast address
            LLARMA=PacketVerifier.LLARMA,  # Link-Local All Routers multicast address
            RLANMA=PacketVerifier.RLANMA,  # realm-local all-nodes multicast address
            RLARMA=PacketVerifier.RLARMA,  # realm-local all-routers multicast address
            RLAMFMA=PacketVerifier.RLAMFMA,  # realm-local ALL_MPL_FORWARDERS address
            LLABMA=PacketVerifier.LLABMA,  # Link-Local All BBRs multicast address
            MA1=consts.MA1,
            MA2=consts.MA2,
            MA3=consts.MA3,
            MA4=consts.MA4,
            MA5=consts.MA5,
            MA6=consts.MA6,
            MA1g=consts.MA1g,
            MAe1=consts.MAe1,
            MAe2=consts.MAe2,
            MAe3=consts.MAe3,
        )

    def _add_initial_vars(self):
        for i, addr in self.test_info.extaddrs.items():
            name = self.test_info.get_node_name(i)
            self._vars[name] = addr

        for i, addr in self.test_info.ethaddrs.items():
            name = self.test_info.get_node_name(i) + '_ETH'
            self._vars[name] = addr

        for i, addrs in self.test_info.ipaddrs.items():
            name = self.test_info.get_node_name(i)
            self._vars[name + '_IPADDRS'] = addrs
            for addr in addrs:
                if addr.is_backbone_gua:
                    key = name + '_BGUA'
                elif addr.is_link_local and (name + '_BGUA') in self._vars:
                    # FIXME: assume the link-local address after Backbone GUA is the Backbone Link Local address
                    key = name + '_BLLA'
                elif addr.is_link_local:
                    key = name + '_LLA'
                else:
                    logging.warning("IPv6 address ignored: name=%s, addr=%s, is_global=%s, is_link_local=%s", name,
                                    addr, addr.is_global, addr.is_link_local)
                    continue

                if key in self._vars:
                    logging.warning("duplicate IPv6 address type: name=%s, addr=%s,%s", name, addr, self._vars[key])
                    continue

                self._vars[key] = addr

        for i, addr in self.test_info.mleids.items():
            name = self.test_info.get_node_name(i)
            self._vars[name + '_MLEID'] = addr

        for i, rloc16 in self.test_info.rloc16s.items():
            key = self.test_info.get_node_name(i) + '_RLOC16'
            self._vars[key] = rloc16

        for i, rloc in self.test_info.rlocs.items():
            key = self.test_info.get_node_name(i) + '_RLOC'
            self._vars[key] = rloc

        for i, omr in self.test_info.omrs.items():
            key = self.test_info.get_node_name(i) + '_OMR'
            self._vars[key] = omr

        if self.test_info.leader_aloc:
            self._vars['LEADER_ALOC'] = self.test_info.leader_aloc

        for k, v in self.test_info.extra_vars.items():
            assert k not in self._vars, k
            logging.info("add extra var: %s = %s", k, v)
            self._vars[k] = v

        for i, topo in self.test_info.topology.items():
            name = self.test_info.get_node_name(i)
            if topo['version']:
                self._vars[name + '_VERSION'] = {'1.1': 2, '1.2': 3, '1.3': 4, '1.4': 5}[topo['version']]

    def verify_attached(self, child: str, parent: str = None, child_type: str = 'FTD', pkts=None) -> VerifyResult:
        """
        Verify that the device attaches to the Thread network.

        :param child: The child device name.
        :param parent: The parent device name.
        :param child_type: The child device type (FTD, FTD-ED, MTD).
        """
        result = VerifyResult()
        assert self.is_thread_device(child), child
        assert child_type in ('FTD', 'FTD-ED', 'MTD'), child_type
        pkts = pkts or self.pkts
        child_extaddr = self.vars[child]

        src_pkts = pkts.filter_wpan_src64(child_extaddr)
        if parent:
            assert self.is_thread_device(parent), parent
            src_pkts = pkts.filter_wpan_src64(child_extaddr).\
                filter_wpan_dst64(self.vars[parent])
        src_pkts.filter_mle_cmd(MLE_CHILD_ID_REQUEST).must_next()  # Child Id Request
        result.record_last('child_id_request', pkts)

        dst_pkts = pkts.filter_wpan_dst64(child_extaddr)
        if parent:
            dst_pkts = pkts.filter_wpan_src64(self.vars[parent]).\
                filter_wpan_dst64(child_extaddr)
        dst_pkts.filter_mle_cmd(MLE_CHILD_ID_RESPONSE).must_next()  # Child Id Response
        result.record_last('child_id_response', pkts)

        with pkts.save_index():
            if child_type == 'FTD':
                src_pkts = pkts.filter_wpan_src64(child_extaddr)
                src_pkts.filter_mle_cmd(MLE_ADVERTISEMENT).must_next()  # MLE Advertisement
                result.record_last('mle_advertisement', pkts)
            logging.info(f"verify attached: d={child}, result={result}")

        return result

    def is_thread_device(self, name: str) -> bool:
        """
        Returns if the device is an WPAN device.

        :param name: The device name.

        Note that device can be both a WPAN device and an Ethernet device.
        """
        assert isinstance(name, str), name

        return name in self.vars

    def is_backbone_device(self, name: str) -> bool:
        """
        Returns if the device s an Ethernet device.

        :param name: The device name.

        Note that device can be both a WPAN device and an Ethernet device.
        """
        assert isinstance(name, str), name

        return f'{name}_ETH' in self.vars

    def max_index(self, *indexes: Tuple[int, int]) -> Tuple[int, int]:
        wpan_idx = 0
        eth_idx = 0
        for wi, ei in indexes:
            wpan_idx = max(wpan_idx, wi)
            eth_idx = max(eth_idx, ei)

        return wpan_idx, eth_idx
