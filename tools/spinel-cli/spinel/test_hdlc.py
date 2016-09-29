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
"""
Unittest for spinel.hdlc module.
"""

import unittest
import binascii

from spinel.hdlc import Hdlc

class TestHdlc(unittest.TestCase):
    """ Unittest class for spinel.hdlc.Hdlc class. """

    VECTOR = {
        # Data    HDLC Encoded
        "810243": "7e810243d3d37e",
        "8103367e7d": "7e8103367d5e7d5d6af97e",
    }

    def test_hdlc_encode(self):
        """ Unit test for Hdle.encode method. """
        hdlc = Hdlc(None)
        for in_hex, out_hex in self.VECTOR.iteritems():
            in_binary = binascii.unhexlify(in_hex)
            out_binary = hdlc.encode(in_binary)
            #print "inHex = "+binascii.hexlify(in_binary)
            #print "outHex = "+binascii.hexlify(out_binary)
            self.failUnless(out_hex == binascii.hexlify(out_binary))

    def test_hdlc_decode(self):
        """ Unit test for Hdle.decode method. """
        pass
