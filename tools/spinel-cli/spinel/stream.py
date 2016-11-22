#!/usr/bin/python
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
Module providing a generic stream interface.
Also includes adapter implementations for serial, socket, and pipes.
"""

from __future__ import print_function

import sys
import logging
import traceback

import subprocess
import socket
import serial

import spinel.util
import spinel.config as CONFIG


class IStream(object):
    """ Abstract base class for a generic Stream Interface. """

    def read(self, size):
        """ Read an array of byte integers of the given size from the stream. """
        pass

    def write(self, data):
        """ Write the given packed data to the stream. """
        pass

    def close(self):
        """ Close the stream cleanly as needed. """
        pass


class StreamSerial(IStream):
    """ An IStream interface implementation for serial devices. """

    def __init__(self, dev, baudrate=115200):
        try:
            self.serial = serial.Serial(dev, baudrate)
        except:
            logging.error("Couldn't open " + dev)
            traceback.print_exc()

    def write(self, data):
        self.serial.write(data)
        if CONFIG.DEBUG_STREAM_TX:
            logging.debug("TX Raw: " + str(map(spinel.util.hexify_chr, data)))

    def read(self, size=1):
        pkt = self.serial.read(size)
        if CONFIG.DEBUG_STREAM_RX:
            logging.debug("RX Raw: " + str(map(spinel.util.hexify_chr, pkt)))
        return map(ord, pkt)[0]


class StreamSocket(IStream):
    """ An IStream interface implementation over an internet socket. """

    def __init__(self, hostname, port):
        # Open socket
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.connect((hostname, port))

    def write(self, data):
        self.sock.send(data)
        if CONFIG.DEBUG_STREAM_TX:
            logging.debug("TX Raw: " + str(map(spinel.util.hexify_chr, data)))

    def read(self, size=1):
        pkt = self.sock.recv(size)
        if CONFIG.DEBUG_STREAM_RX:
            logging.debug("RX Raw: " + str(map(spinel.util.hexify_chr, pkt)))
        return map(ord, pkt)[0]


class StreamPipe(IStream):
    """ An IStream interface implementation to stdin/out of a piped process. """

    def __init__(self, filename):
        """ Create a stream object from a piped system call """
        try:
            self.pipe = subprocess.Popen(filename, shell=True,
                                         stdin=subprocess.PIPE,
                                         stdout=subprocess.PIPE,
                                         stderr=sys.stdout.fileno())
        except:
            logging.error("Couldn't open " + filename)
            traceback.print_exc()

    def write(self, data):
        if CONFIG.DEBUG_STREAM_TX:
            logging.debug("TX Raw: (%d) %s",
                          len(data), spinel.util.hexify_bytes(data))
        self.pipe.stdin.write(data)

    def read(self, size=1):
        """ Blocking read on stream object """
        pkt = self.pipe.stdout.read(size)
        if CONFIG.DEBUG_STREAM_RX:
            logging.debug("RX Raw: " + str(map(spinel.util.hexify_chr, pkt)))
        return map(ord, pkt)[0]

    def close(self):
        if self.pipe:
            self.pipe.kill()
            self.pipe = None


def StreamOpen(stream_type, descriptor, verbose=True):
    """
    Factory function that creates and opens a stream connection.

    stream_type:
        'u' = uart (/dev/tty#)
        's' = socket (port #)
        'p' = pipe (stdin/stdout)

    descriptor:
        uart - filename of device (/dev/tty#)
        socket - port to open connection to on localhost
        pipe - filename of command to execute and bind via stdin/stdout
    """

    if stream_type == 'p':
        if verbose:
            print("Opening pipe to " + str(descriptor))
        return StreamPipe(descriptor)

    elif stream_type == 's':
        port = int(descriptor)
        hostname = "localhost"
        if verbose:
            print("Opening socket to " + hostname + ":" + str(port))
        return StreamSocket(hostname, port)

    elif stream_type == 'u':
        dev = str(descriptor)
        baudrate = 115200
        if verbose:
            print("Opening serial to " + dev + " @ " + str(baudrate))
        return StreamSerial(dev, baudrate)

    else:
        return None
