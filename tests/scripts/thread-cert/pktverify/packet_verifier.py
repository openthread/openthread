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
from pktverify.consts import DUA_RECENT_TIME, MLE_CHILD_ID_REQUEST, MLE_ADVERTISEMENT, MLE_CHILD_ID_RESPONSE
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

    def __init__(self, test_info_path):
        logging.basicConfig(level=logging.INFO,
                            format='File "%(pathname)s", line %(lineno)d, in %(funcName)s\n'
                            '%(asctime)s - %(levelname)s - %(message)s')

        ti = TestInfo(test_info_path)
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
            for addr in addrs:
                if addr.is_dua:
                    key = name + '_DUA'
                elif addr.is_backbone_gua:
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

        if self.test_info.leader_aloc:
            self._vars['LEADER_ALOC'] = self.test_info.leader_aloc

        for k, v in self.test_info.extra_vars.items():
            assert k not in self._vars, k
            logging.info("add extra var: %s = %s", k, v)
            self._vars[k] = v

    def verify_dua_registration(self,
                                td: str,
                                bbr: str,
                                pkts=None,
                                dua_deadline=None,
                                DAD=False,
                                sbbr=None) -> VerifyResult:
        """
        Run the packet verification for the while DAD registration, including optional steps
        This is commonly used in many test cases.

        :param pkts: The packet filter to verify, or self.pkts if None
        :param td: TB's name.
        :param bbr: BBR's name.
        """
        assert self.is_thread_device(td)
        assert self.is_thread_device(bbr) and self.is_backbone_device(bbr), bbr

        if pkts is None:
            pkts = self.pkts

        logging.info("verifying DUA registration from %s to %s ...", td, bbr)
        result = VerifyResult()

        TD = self.vars[td]
        BBR = self.vars[bbr]
        BBR_ETH = self.vars[bbr + '_ETH']
        if sbbr:
            SBBR_ETH = self.vars[sbbr + '_ETH']
        # BBR_DUA = self.vars[bbr + '_DUA']
        p = pkts.filter_wpan_src64(TD) \
            .filter_coap_request("/n/dr", port=self.MM_PORT) \
            .must_next()

        result.record_last("/n/dr", pkts)

        if dua_deadline is not None:
            p.must_verify(lambda p: p.sniff_timestamp <= dua_deadline)

        idx_after_n_dr = pkts.index

        p.must_verify(lambda p: p.coap.tlv.target_eid and p.coap.tlv.ml_eid)
        DUA = p.coap.tlv.target_eid
        MLEID = p.coap.tlv.ml_eid
        self.add_vars(**{td + "_DUA": DUA, td + "_MLEID": MLEID})
        logging.info(f"DUA={DUA}, MLEID={MLEID}")

        if DAD:
            # DAD (DUA_DATA_REPEAT+1) TIMES
            before_dad_index = pkts.index
            after_dad_index = before_dad_index
            with pkts.save_index():
                for i in range(consts.DUA_DAD_REPEATS + 1):
                    # Step 3: PBBR - Performs DAD on the backbone link - Multicasts a BB.qry CoAP request
                    filter = pkts.filter_eth_src(BBR_ETH) \
                        .filter_LLABMA() \
                        .filter_coap_request("/b/bq") \
                        .filter(lambda p: p.coap.tlv.target_eid == DUA)
                    p = filter.must_next()
                    after_dad_index = self.max_index(after_dad_index, pkts.index)

                    with pkts.save_index():
                        # try to find the next multicast
                        # start_index = pkts.index
                        if filter.next():
                            pe = pkts.last()
                            time_gap = pe.sniff_timestamp - p.sniff_timestamp
                            # PBBR Waits for DUA_DAD_QUERY_TIMEOUT: Verify that DUA_DAD_QUERY_TIMEOUT time passes
                            assert time_gap >= consts.DUA_DAD_QUERY_TIMEOUT - 0.01, time_gap

            # SBBR: Does not respond: SBBR does not respond to the BB.qry message.
            if sbbr is not None:
                dad_pkts_range = pkts.range(before_dad_index, after_dad_index, cascade=False)
                dad_pkts_range.filter_eth_src(SBBR_ETH) \
                    .filter_LLABMA() \
                    .filter_coap_ack("/b/bq") \
                    .must_not_next()

        # BBR updates the corresponding entry in its DUA device table.
        # No pass criteria

        # Step 7: BBR informs other BBRs on the network of the DUA registration.
        # FIXME: Test plan requires that last_transaction_time <= 3,
        #  however real OT implementation can have last_transaction_time == 4
        expected_last_transaction_time = 0 if not DAD else 4
        pkts.filter_eth_src(BBR_ETH) \
            .filter_LLABMA() \
            .filter_coap_request("/b/ba") \
            .filter("coap.tlv.target_eid == {DUA}", DUA=DUA) \
            .must_next() \
            .must_verify("""
                    coap.tlv.ml_eid == {MLEID}
                    and coap.tlv.last_transaction_time <= {expected_last_transaction_time}
                    and coap.tlv.net_name == {NET_NAME}
                """, MLEID=MLEID, NET_NAME=self.NET_NAME,
                         expected_last_transaction_time=expected_last_transaction_time)

        idx1 = pkts.index

        # SBBR receives PRO_BB.ntf and optionally updates the corresponding entry
        # in its Backup DUA Devices Table. No pass criteria.

        # BBR announces itself as the new ND proxy for the roaming device
        pkts.seek_back(0.2, eth=True) \
            .filter_eth_src(BBR_ETH) \
            .filter_LLANMA() \
            .filter_icmpv6_nd_na(DUA) \
            .must_next() \
            .must_verify("""
                    icmpv6.nd.na.flag.s == 0
                    and icmpv6.nd.na.flag.o == 1
                    and icmpv6.nd.na.flag.r == 1
                    and icmpv6.opt.target_linkaddr == {BBR_ETH}
                """, BBR_ETH=BBR_ETH)

        idx2 = pkts.index
        # BBR responds to the DUA registration
        pkts.index = idx_after_n_dr  # reset index to just after /n/dr request
        pkts.filter_wpan_src64(BBR) \
            .filter_coap_ack("/n/dr") \
            .filter("coap.tlv.target_eid == {DUA}", DUA=DUA) \
            .must_next() \
            .must_verify("""
                    coap.tlv.target_eid == {DUA}
                    and coap.tlv.status == 0
                    """, DUA=DUA)

        pkts.index = self.max_index(idx1, idx2, pkts.index)
        # BBR optionally repeats the unsolicited neighbor advertisement.
        # Optional 1 or 2 times
        with pkts.save_index():
            filter = pkts.filter_eth_src(BBR_ETH) \
                .filter_LLANMA() \
                .filter_icmpv6_nd_na(DUA)

            for i in range(2):
                if not filter.next():
                    break

                filter.last().must_verify("""
                        icmpv6.nd.na.flag.s == 0
                        and icmpv6.nd.na.flag.o == 1
                        and icmpv6.nd.na.flag.r == 1
                        and icmpv6.opt.target_linkaddr == {BBR_ETH}
                    """,
                                          BBR_ETH=BBR_ETH)

        # BBR Optionally repeats the DUA registration notification
        # Optional
        with pkts.save_index():
            filter = pkts.filter_eth_src(BBR_ETH) \
                .filter_LLABMA() \
                .filter_coap_request("/b/ba") \
                .filter("coap.tlv.target_eid == {DUA}", DUA=DUA)

            if filter.next():
                p = filter.last()
                p.must_verify("""
                    coap.tlv.ml_eid == {MLEID}
                    and coap.tlv.net_name == {NET_NAME}
                    and 3 < coap.tlv.last_transaction_time < DUA_RECENT_TIME
                    """,
                              DUA_RECENT_TIME=DUA_RECENT_TIME,
                              MLEID=MLEID,
                              NET_NAME=self.NET_NAME)

        if sbbr is not None:
            # SBBR: Does not respond to the ND Neighbor Solicitation message.
            SBBR_ETH = self.vars[sbbr + '_ETH']
            pkts_in_range = pkts.range(idx_after_n_dr, pkts.index, cascade=False)
            pkts_in_range.filter_eth_src(SBBR_ETH).filter_LLANMA().filter_icmpv6_nd_na(DUA).must_not_next()

        return result

    def verify_attached(self, name: str, pkts=None) -> VerifyResult:
        """
        Verify that the device attaches to the Thread network.

        :param name: The device name.
        """
        result = VerifyResult()
        assert self.is_thread_device(name), name
        pkts = pkts or self.pkts
        extaddr = self.vars[name]

        src_pkts = pkts.filter_wpan_src64(extaddr)
        src_pkts.filter_mle_cmd(MLE_CHILD_ID_REQUEST).must_next()  # Child Id Request
        result.record_last('child_id_request', pkts)

        dst_pkts = pkts.filter_wpan_dst64(extaddr)
        dst_pkts.filter_mle_cmd(MLE_CHILD_ID_RESPONSE).must_next()  # Child Id Response
        result.record_last('child_id_response', pkts)

        with pkts.save_index():
            src_pkts.filter_mle_cmd(MLE_ADVERTISEMENT).must_next()  # MLE Advertisement
            result.record_last('mle_advertisement', pkts)
            logging.info(f"verify attached: d={name}, result={result}")

        return result

    def verify_ping(self, src: str, dst: str, bbr: str = None, pkts: 'PacketVerifier' = None) -> VerifyResult:
        """
        Verify the ping process.

        :param src: The source device name.
        :param dst: The destination device name.
        :param bbr: The Backbone Router name.
                    If specified, this method also verifies that the ping request and reply be forwarded by the Backbone Router.
        :param pkts: The PacketFilter to search.

        :return: The verification result.
        """
        if bbr:
            assert not (self.is_thread_device(src) and self.is_thread_device(dst)), \
                f"both {src} and {dst} are WPAN devices"
            assert not (self.is_backbone_device(src) and self.is_backbone_device(dst)), \
                f"both {src} and {dst} are ETH devices"

        if pkts is None:
            pkts = self.pkts

        src_dua = self.vars[src + '_DUA']
        dst_dua = self.vars[dst + '_DUA']
        if bbr:
            bbr_ext = self.vars[bbr]
            bbr_eth = self.vars[bbr + '_ETH']

        result = VerifyResult()
        ping_req = pkts.filter_ping_request().filter_ipv6_dst(dst_dua)
        if self.is_backbone_device(src):
            p = ping_req.filter_eth_src(self.vars[src + '_ETH']).must_next()
        else:
            p = ping_req.filter_wpan_src64(self.vars[src]).must_next()

        # pkts.last().show()
        ping_id = p.icmpv6.echo.identifier
        logging.info("verify_ping: ping_id=%x", ping_id)
        result.record_last('ping_request', pkts)
        ping_req = ping_req.filter(lambda p: p.icmpv6.echo.identifier == ping_id)

        # BBR unicasts the ping packet to TD.
        if bbr:
            if self.is_backbone_device(src):
                ping_req.filter_wpan_src64(bbr_ext).must_next()
            else:
                ping_req.filter_eth_src(bbr_eth).must_next()

        ping_reply = pkts.filter_ping_reply().filter_ipv6_dst(src_dua).filter(
            lambda p: p.icmpv6.echo.identifier == ping_id)
        # TD receives ping packet and responds back to Host via SBBR.
        if self.is_thread_device(dst):
            ping_reply.filter_wpan_src64(self.vars[dst]).must_next()
        else:
            ping_reply.filter_eth_src(self.vars[dst + '_ETH']).must_next()

        result.record_last('ping_reply', pkts)

        if bbr:
            # SBBR forwards the ping response packet to Host.
            if self.is_thread_device(dst):
                ping_reply.filter_eth_src(bbr_eth).must_next()
            else:
                ping_reply.filter_wpan_src64(bbr_ext).must_next()

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
