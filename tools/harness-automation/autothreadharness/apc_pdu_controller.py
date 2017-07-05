#!/usr/bin/env python
#
# Copyright (c) 2016, The OpenThread Authors.
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
import telnetlib
import time
import re

logger = logging.getLogger(__name__)

class ApcPduController(object):
    def __init__(self, ip, port=23):
        """Create APC PDU controller

        Args:
            ip (str), ip address or hostname
            port (int), port number
        """
        self.port = port
        self.ip = ip
        self.tn = None
        self._init()

    def __del__(self):
        self.close()

    def _init(self):
        """Initialize the telnet connection
        """
        self.tn = telnetlib.Telnet(self.ip, self.port)
        self.tn.read_until('User Name')
        self.tn.write('apc\r\n')
        self.tn.read_until('Password')
        self.tn.write('apc\r\n')
        self.until_done()

    def close(self):
        """Close telnet connection"""
        logger.info('closing telnet')
        if self.tn:
            self.tn.close()

    def until_done(self):
        """Wait until the prompt encountered
        """
        self.until(r'^>')

    def until(self, regex):
        """Wait until the regex encountered
        """
        logger.debug('waiting for %s', regex)
        r = re.compile(regex, re.M)
        self.tn.expect([r])

    def reboot(self, outlet=1):
        """Reboot outlet
        """
        # main menu
        self.tn.write('\x1b\r\n')
        self.until_done()
        # Device Manager
        self.tn.write('1\r\n')
        self.until_done()
        # Outlet Management
        self.tn.write('2\r\n')
        self.until_done()
        # Outlet Control
        self.tn.write('1\r\n')
        self.until_done()
        # Select outlet
        self.tn.write('%d\r\n' % outlet)
        self.until_done()
        # Control
        self.tn.write('1\r\n')
        self.until_done()
        # off
        self.tn.write('2\r\n')
        self.until('to cancel')
        self.tn.write('YES\r\n')
        self.until('to continue')
        self.tn.write('\r\n')
        self.until_done()

        time.sleep(5)
        # on
        self.tn.write('1\r\n')
        self.until('to cancel')
        self.tn.write('YES\r\n')
        self.until('to continue')
        self.tn.write('\r\n')
        self.until_done()

if __name__ == '__main__':
    apc = ApcPduController('192.168.1.88')
    apc.reboot()
