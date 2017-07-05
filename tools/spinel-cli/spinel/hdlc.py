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
""" High-Level Data Link Control (HDLC) module. """

import logging

from struct import pack

import spinel.config as CONFIG
from spinel.stream import IStream
from spinel.util import hexify_int
from spinel.util import hexify_bytes

HDLC_FLAG = 0x7e
HDLC_ESCAPE = 0x7d

# RFC 1662 Appendix C

HDLC_FCS_INIT = 0xFFFF
HDLC_FCS_POLY = 0x8408
HDLC_FCS_GOOD = 0xF0B8


class Hdlc(IStream):
    """ Utility class for HDLC encoding and decoding. """

    def __init__(self, stream):
        self.stream = stream
        self.fcstab = self.mkfcstab()

    @classmethod
    def mkfcstab(cls):
        """ Make a static lookup table for byte value to FCS16 result. """
        polynomial = HDLC_FCS_POLY

        def valiter():
            """ Helper to yield FCS16 table entries for each byte value. """
            for byte in range(256):
                fcs = byte
                i = 8
                while i:
                    fcs = (fcs >> 1) ^ polynomial if fcs & 1 else fcs >> 1
                    i -= 1

                yield fcs & 0xFFFF

        return tuple(valiter())

    def fcs16(self, byte, fcs):
        """
        Return the next iteration of an fcs16 calculation
        given the next data byte and current fcs accumulator.
        """
        fcs = (fcs >> 8) ^ self.fcstab[(fcs ^ byte) & 0xff]
        return fcs

    def collect(self):
        """ Return the next valid packet to pass HDLC decoding on the stream. """
        fcs = HDLC_FCS_INIT
        packet = []
        raw = []

        # Synchronize
        while 1:
            byte = self.stream.read()
            if CONFIG.DEBUG_HDLC:
                raw.append(byte)
            if byte == HDLC_FLAG:
                break

        # Read packet, updating fcs, and escaping bytes as needed
        while 1:
            byte = self.stream.read()
            if CONFIG.DEBUG_HDLC:
                raw.append(byte)
            if byte == HDLC_FLAG:
                if len(packet) != 0:
                    break
                else:
                    # If multiple FLAG bytes in a row, keep looking for data.
                    continue
            if byte == HDLC_ESCAPE:
                byte = self.stream.read()
                if CONFIG.DEBUG_HDLC:
                    raw.append(byte)
                byte ^= 0x20
            packet.append(byte)
            fcs = self.fcs16(byte, fcs)

        if CONFIG.DEBUG_HDLC:
            logging.debug("RX Hdlc: " + str(map(hexify_int, raw)))

        if fcs != HDLC_FCS_GOOD:
            packet = None
        else:
            packet = packet[:-2]        # remove FCS16 from end

        return packet

    @classmethod
    def encode_byte(cls, byte, packet=[]):
        """ HDLC encode and append a single byte to the given packet. """
        if (byte == HDLC_ESCAPE) or (byte == HDLC_FLAG):
            packet.append(HDLC_ESCAPE)
            packet.append(byte ^ 0x20)
        else:
            packet.append(byte)
        return packet

    def encode(self, payload=""):
        """ Return the HDLC encoding of the given packet. """
        fcs = HDLC_FCS_INIT
        packet = []
        packet.append(HDLC_FLAG)
        for byte in payload:
            byte = ord(byte)
            fcs = self.fcs16(byte, fcs)
            packet = self.encode_byte(byte, packet)

        fcs ^= 0xffff
        byte = fcs & 0xFF
        packet = self.encode_byte(byte, packet)
        byte = fcs >> 8
        packet = self.encode_byte(byte, packet)
        packet.append(HDLC_FLAG)
        packet = pack("%dB" % len(packet), *packet)

        if CONFIG.DEBUG_HDLC:
            logging.debug("TX Hdlc: " + hexify_bytes(packet))
        return packet

    def write(self, data):
        """ HDLC encode and write the given data to this stream. """
        pkt = self.encode(data)
        self.stream.write(pkt)

    def read(self, _size=None):
        """ Read and HDLC decode the next packet from this stream. """
        pkt = self.collect()
        return pkt
