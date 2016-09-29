#
#  Copyright (c) 2016, The OpenThread Authors.
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
""" Unittest for spinel.codec module. """

import unittest

import spinel.util as util
from spinel.const import SPINEL
from spinel.codec import WpanApi
from spinel.test_stream import MockStream


class TestSniffer(unittest.TestCase):
    """ Unit TestCase class for sniffer relevant portions of spinel.codec.SpinelCodec. """

    HEADER = "800671"     # CMD_PROP_IS RAW_STREAM
    VECTOR = [
        # Some raw 6lo packets: ICMPv6EchoRequest to ff02::1, fe80::1, and MLE Advertisement
        "2d00499880fffffffffeff0d0100000001a7acdf3be9272c2d88765ff76f0bf08a7c3df0a78e9c1b23eb019c58740300800000",
        "3200699c81ffff0100000000000002feff0d030000000198e80cac00f8e0754e7542f5cb1171069f5c9689ef8d1d45a75e26b3f600800000",
        "450041d8980100ffffa8cb25ab2c32a0227f3b01f04d4c4d4cdc3b0015060000000000000001f226cce17968521d92904fec1adb0b94777030b944df65450bc955f05737e3901700800000"
    ]

    def test_prop_get(self):
        """ Unit test of SpinelCodec.prop_get_value. """

        mock_stream = MockStream({})

        nodeid = 1
        use_hdlc = False
        tid = SPINEL.HEADER_ASYNC
        prop_id = SPINEL.PROP_STREAM_RAW

        wpan_api = WpanApi(mock_stream, nodeid, use_hdlc)
        wpan_api.queue_register(tid)

        for truth in self.VECTOR:
            mock_stream.write_child_hex(self.HEADER+truth)
            result = wpan_api.queue_wait_for_prop(prop_id, tid)
            packet = util.hexify_str(result.value,"")
            self.failUnless(packet == truth)
