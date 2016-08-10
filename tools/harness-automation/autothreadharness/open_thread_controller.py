#!/usr/bin/env python
#
# Copyright (c) 2016, Nest Labs, Inc.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
# 3. Neither the name of the copyright holder nor the
#    names of its contributors may be used to endorse or promote products
#    derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#


import logging
import time

import serial

from pexpect_serial import SerialSpawn

__all__ = ['OpenThreadController']
logger = logging.getLogger(__name__)

class OpenThreadController(object):
    """This is an simple wrapper to communicate with openthread"""
    def __init__(self, port):
        """Initialize the controller

        Args:
            port (str): serial port's path or name(windows)
        """
        self.port = port
        self.ss = None
        self._init()

    def _init(self):
        ser = serial.Serial(self.port, 115200, timeout=2)
        self.ss = SerialSpawn(ser, timeout=2)

    def __del__(self):
        if self.ss:
            self.ss.close()

    def is_started(self):
        """check if openthread is started

        Returns:
            bool: started or not
        """
        state = self._req('state')[0]
        return state != 'disabled'

    def start(self):
        """Start openthread
        """
        self._req('ifconfig up')
        self._req('thread start')

    def stop(self):
        """Stop openthread
        """
        self._req('thread stop')
        self._req('ifconfig down')

    def reset(self):
        """Reset openthread device, not equivalent to stop and start
        """
        self.ss.sendline('reset')
        time.sleep(1)
        self.ss.close()
        time.sleep(1)
        self._init()

    def _req(self, req):
        """Send command and wait for response.

        The command will be repeated 3 times at most in case data loss of serial port.

        Args:
            req (str): Command to send, please do not include new line in the end.

        Returns:
            [str]: The output lines
        """
        logger.info('DUT> %s', req)
        times = 3

        while times:
            times = times - 1
            try:
                self.ss.sendline(req)
                self.ss.expect(req + self.ss.linesep)
            except:
                logger.exception('Failed to send command')
            else:
                break

        line = None
        res = []

        while True:
            line = self.ss.readline().strip('\0\r\n\t ')
            logger.debug(line)

            if line:
                if line == 'Done':
                    break
                res.append(line)

        return res

    @property
    def networkname(self):
        """str: Thread network name."""
        return self._req('networkname')[0]

    @networkname.setter
    def networkname(self, value):
        self._req('networkname %s' % value)

    @property
    def mode(self):
        """str: Thread mode."""
        return self._req('mode')[0]

    @mode.setter
    def mode(self, value):
        self._req('mode %s' % value)

    @property
    def mac(self):
        """str: MAC address of the device"""
        return self._req('extaddr')[0]

    @property
    def addrs(self):
        """[str]: IP addresses of the devices"""
        return self._req('ipaddr')

    @property
    def short_addr(self):
        """str: Short address"""
        return self._req('rloc16')[0]

    @property
    def channel(self):
        """int: Channel number of openthread"""
        return int(self._req('channel')[0])

    @channel.setter
    def channel(self, value):
        self._req('channel %d' % value)

    @property
    def panid(self):
        """str: Thread panid"""
        return self._req('panid')[0]

    @panid.setter
    def panid(self, value):
        self._req('panid %s' % value)

    @property
    def extpanid(self):
        """str: Thread extpanid"""
        return self._req('extpanid')[0]

    @extpanid.setter
    def extpanid(self, value):
        self._req('extpanid %s' % value)

    @property
    def child_timeout(self):
        """str: Thread child timeout in seconds"""
        return self._req('childtimeout')[0]

    @child_timeout.setter
    def child_timeout(self, value):
        self._req('childtimeout %d' % value)

    @property
    def version(self):
        """str: Open thread version"""
        return self._req('version')[0]

    def add_prefix(self, prefix, flags, prf):
        """Add network prefix.

        Args:
            prefix (str): network prefix.
            flags (str): network prefix flags, please refer thread documentation for details
            prf (str): network prf, please refer thread documentation for details
        """
        self._req('prefix add %s %s %s' % (prefix, flags, prf))
        time.sleep(1)
        self._req('netdataregister')

    def remove_prefix(self, prefix):
        """Remove network prefix.
        """
        self._req('prefix remove %s' % prefix)
        time.sleep(1)
        self._req('netdataregister')
