#!/usr/bin/env python
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
Tests for spinel.stream and implementation of MockStream class.
"""

import binascii
import logging

import Queue

import spinel.util as util
import spinel.config as CONFIG
from spinel.stream import IStream


class MockStream(IStream):
    """ A pluggable IStream class for mock testing input/output data flows. """

    def __init__(self, vector):
        """
        Pass a test vector as dictionary of hexstream outputs keyed on inputs.
        """
        self.vector = vector
        self.rx_queue = Queue.Queue()
        self.response = None

    def write(self, out_binary):
        """ Write to the MockStream, triggering a lookup for mock response. """
        if CONFIG.DEBUG_STREAM_TX:
            logging.debug("TX Raw: (%d) %s", len(out_binary),
                          util.hexify_bytes(out_binary))
        out_hex = binascii.hexlify(out_binary)
        in_hex = self.vector[out_hex]
        self.rx_queue.put_nowait(binascii.unhexlify(in_hex))

    def read(self, size=None):
        """ Blocking read from the MockStream. """
        if not self.response or len(self.response) == 0:
            self.response = self.rx_queue.get(True)

        if size:
            in_binary = self.response[:size]
            self.response = self.response[size:]
        else:
            in_binary = self.response
            self.response = None

        if CONFIG.DEBUG_STREAM_RX:
            logging.debug("RX Raw: " + util.hexify_bytes(in_binary))
        return in_binary

    def write_child(self, out_binary):
        """ Mock asynchronous write from child process. """
        self.rx_queue.put_nowait(out_binary)

    def write_child_hex(self, out_hex):
        """ Mock asynchronous write from child process. """
        self.write_child(binascii.unhexlify(out_hex))
