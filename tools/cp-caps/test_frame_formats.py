#!/usr/bin/env python3
#
#  Copyright (c) 2025, The OpenThread Authors.
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

import unittest

from otci import OTCI
from device_manager import DeviceManager, Frame


class TestFrameFormats(unittest.TestCase):
    """Test whether the DUT supports all different 15.4 frame formats."""

    @classmethod
    def setUpClass(cls):
        cls.__device_manager = DeviceManager()
        cls.__dut = cls.__device_manager.dut
        cls.__ref = cls.__device_manager.ref

        cls.__dut.factory_reset()
        cls.__ref.factory_reset()

    def setUp(self):
        pass

    def test_frame_format_01(self):
        frame = Frame(name='ver:2003,Cmd,seq,dst[addr:short,pan:id],src[addr:no,pan:no],sec:no,ie:no,plen:0',
                      tx_frame='030800ffffffff070000',
                      dst_address='0xffff')
        self.__test_frame_format(1, frame)

    def test_frame_format_02(self):
        frame = Frame(
            name='ver:2003,Bcon,seq,dst[addr:no,pan:no],src[addr:extd,pan:id],sec:no,ie:no,plen:30',
            tx_frame='00c000eeee0102030405060708ff0f000003514f70656e54687265616400000000000001020304050607080000',
            dst_address='-')
        self.__test_frame_format(2, frame)

    def test_frame_format_03(self):
        frame = Frame(
            name='ver:2003,MP,noseq,dst[addr:extd,pan:id],src[addr:extd,pan:no],sec:l5,ie[ren con],plen:0',
            tx_frame='fd87dddd1020304050607080010203040506070815000000007265616401820ee80305009bb8ea011c807aa1120000',
            dst_address='8070605040302010')
        self.__test_frame_format(3, frame)

    def test_frame_format_04(self):
        frame = Frame(name='ver:2006,Cmd,seq,dst[addr:short,pan:id],src[addr:short,pan:no],sec:l5,ie:no,plen:0',
                      tx_frame='4b9800ddddaaaabbbb0d0000000001043daa1aea0000',
                      dst_address='0xaaaa')
        self.__test_frame_format(4, frame)

    def test_frame_format_05(self):
        frame = Frame(name='ver:2006,Cmd,seq,dst[addr:extd,pan:id],src[addr:extd,pan:no],sec:l5,ie:no,plen:0',
                      tx_frame='4bdc00dddd102030405060708001020304050607080d000000000104483cb8a90000',
                      dst_address='8070605040302010')
        self.__test_frame_format(5, frame)

    def test_frame_format_06(self):
        frame = Frame(name='ver:2006,Data,seq,dst[addr:extd,pan:id],src[addr:extd,pan:id],sec:no,ie:no,plen:0',
                      tx_frame='41dc00dddd102030405060708001020304050607080000',
                      dst_address='8070605040302010')
        self.__test_frame_format(6, frame)

    def test_frame_format_07(self):
        frame = Frame(name='ver:2006,Data,seq,dst[addr:short,pan:id],src[addr:short,pan:id],sec:no,ie:no,plen:0',
                      tx_frame='019800ddddaaaaeeeebbbb0000',
                      dst_address='0xaaaa')
        self.__test_frame_format(7, frame)

    def test_frame_format_08(self):
        frame = Frame(name='ver:2006,Data,seq,dst[addr:extd,pan:id],src[addr:no,pan:no],sec:no,ie:no,plen:0',
                      tx_frame='011c00dddd10203040506070800000',
                      dst_address='8070605040302010')
        self.__test_frame_format(8, frame)

    def test_frame_format_09(self):
        frame = Frame(name='ver:2006,Data,seq,dst[addr:short,pan:id],src[addr:no,pan:no],sec:no,ie:no,plen:0',
                      tx_frame='011800ddddaaaa0000',
                      dst_address='0xaaaa')
        self.__test_frame_format(9, frame)

    def test_frame_format_10(self):
        frame = Frame(name='ver:2015,Data,seq,dst[addr:no,pan:no],src[addr:no,pan:no],sec:no,ie:no,plen:0',
                      tx_frame='0120000000',
                      dst_address='-')
        self.__test_frame_format(10, frame)

    def test_frame_format_11(self):
        frame = Frame(name='ver:2015,Data,seq,dst[addr:no,pan:id],src[addr:no,pan:no],sec:no,ie:no,plen:0',
                      tx_frame='412000dddd0000',
                      dst_address='-')
        self.__test_frame_format(11, frame)

    def test_frame_format_12(self):
        frame = Frame(name='ver:2015,Data,seq,dst[addr:extd,pan:id],src[addr:no,pan:no],sec:no,ie:no,plen:0',
                      tx_frame='012c00dddd10203040506070800000',
                      dst_address='8070605040302010')
        self.__test_frame_format(12, frame)

    def test_frame_format_13(self):
        frame = Frame(name='ver:2015,Data,seq,dst[addr:extd,pan:no],src[addr:no,pan:no],sec:no,ie:no,plen:0',
                      tx_frame='412c0010203040506070800000',
                      dst_address='8070605040302010')
        self.__test_frame_format(13, frame)

    def test_frame_format_14(self):
        frame = Frame(name='ver:2015,Data,seq,dst[addr:no,pan:no],src[addr:extd,pan:id],sec:no,ie:no,plen:0',
                      tx_frame='01e000eeee01020304050607080000',
                      dst_address='-')
        self.__test_frame_format(14, frame)

    def test_frame_format_15(self):
        frame = Frame(name='ver:2015,Data,seq,dst[addr:no,pan:no],src[addr:extd,pan:no],sec:no,ie:no,plen:0',
                      tx_frame='41e00001020304050607080000',
                      dst_address='-')
        self.__test_frame_format(15, frame)

    def test_frame_format_16(self):
        frame = Frame(name='ver:2015,Data,seq,dst[addr:extd,pan:id],src[addr:extd,pan:no],sec:no,ie:no,plen:0',
                      tx_frame='01ec00dddd102030405060708001020304050607080000',
                      dst_address='8070605040302010')
        self.__test_frame_format(16, frame)

    def test_frame_format_17(self):
        frame = Frame(name='ver:2015,Data,seq,dst[addr:extd,pan:no],src[addr:extd,pan:no],sec:no,ie:no,plen:0',
                      tx_frame='41ec00102030405060708001020304050607080000',
                      dst_address='8070605040302010')
        self.__test_frame_format(17, frame)

    def test_frame_format_18(self):
        frame = Frame(name='ver:2015,Data,seq,dst[addr:short,pan:id],src[addr:short,pan:id],sec:no,ie:no,plen:0',
                      tx_frame='01a800ddddaaaaeeeebbbb0000',
                      dst_address='0xaaaa')
        self.__test_frame_format(18, frame)

    def test_frame_format_19(self):
        frame = Frame(name='ver:2015,Data,seq,dst[addr:short,pan:id],src[addr:extd,pan:id],sec:no,ie:no,plen:0',
                      tx_frame='01e800ddddaaaaeeee01020304050607080000',
                      dst_address='0xaaaa')
        self.__test_frame_format(19, frame)

    def test_frame_format_20(self):
        frame = Frame(name='ver:2015,Data,seq,dst[addr:extd,pan:id],src[addr:short,pan:id],sec:no,ie:no,plen:0',
                      tx_frame='01ac00dddd1020304050607080eeeebbbb0000',
                      dst_address='8070605040302010')
        self.__test_frame_format(20, frame)

    def test_frame_format_21(self):
        frame = Frame(name='ver:2015,Data,seq,dst[addr:short,pan:id],src[addr:short,pan:id],sec:no,ie[csl],plen:0',
                      tx_frame='01aa00ddddaaaaeeeebbbb040dc800e8030000',
                      dst_address='0xaaaa')
        self.__test_frame_format(21, frame)

    def test_frame_format_22(self):
        frame = Frame(name='ver:2015,Data,noseq,dst[addr:short,pan:id],src[addr:short,pan:id],sec:no,ie:no,plen:0',
                      tx_frame='01a9ddddaaaaeeeebbbb0000',
                      dst_address='0xaaaa')
        self.__test_frame_format(22, frame)

    def __test_frame_format(self, number: int, frame: Frame):
        ret = self.__device_manager.send_formatted_frames_retries(self.__dut, self.__ref, frame)
        self.__device_manager.output_test_result_bool(f'{number} TX {frame.name}', ret, align_length=100)
        ret = self.__device_manager.send_formatted_frames_retries(self.__ref, self.__dut, frame)
        self.__device_manager.output_test_result_bool(f'{number} RX {frame.name}', ret, align_length=100)

    def tearDown(self):
        pass

    @classmethod
    def tearDownClass(cls):
        cls.__dut.close()
        cls.__ref.close()


if __name__ == '__main__':
    unittest.main()
