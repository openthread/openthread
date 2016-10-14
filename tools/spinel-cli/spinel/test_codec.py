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

from spinel.const import SPINEL
from spinel.codec import WpanApi
from spinel.test_stream import MockStream


class TestCodec(unittest.TestCase):
    """ Unit TestCase class for spinel.codec.SpinelCodec class. """

    # Tests parsing and format demuxing of various properties with canned
    # values.
    VECTOR = {
        SPINEL.PROP_MAC_15_4_PANID: 65535,
        SPINEL.PROP_NCP_VERSION: "OPENTHREAD",
        SPINEL.PROP_NET_ROLE: 0,
        SPINEL.PROP_NET_KEY_SEQUENCE: 5,
        SPINEL.PROP_NET_NETWORK_NAME: "OpenThread",
        SPINEL.PROP_THREAD_MODE: 0xF,
    }

    def test_prop_get(self):
        """ Unit test of SpinelCodec.prop_get_value. """

        mock_stream = MockStream({
            # Request:  Response
            "810236": "810636ffff",                    # get panid = 65535
            "810243": "81064300",                      # get state = detached
            "81025e": "81065e0f",                      # mode = 0xF
            "810202": "8106024f50454e54485245414400",  # get version
            "810247": "81064705000000",                # get keysequence
            "810244": "8106444f70656e54687265616400",  # get networkname
        })
        nodeid = 1
        use_hdlc = False
        wpan_api = WpanApi(mock_stream, nodeid, use_hdlc)

        for prop_id, truth_value in self.VECTOR.iteritems():
            value = wpan_api.prop_get_value(prop_id)
            # print "value "+util.hexify_str(value)
            # print "truth "+util.hexify_str(truth_value)
            self.failUnless(value == truth_value)
