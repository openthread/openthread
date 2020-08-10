#!/usr/bin/env python
#
# Copyright (c) 2020, The OpenThread Authors.
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
"""
>> Thread Host Controller Interface
>> Device : OpenThread_BR THCI
>> Class : OpenThread_BR
"""
import re

import logging
import serial
import time

from IThci import IThci
from THCI.OpenThread import OpenThreadTHCI, watched

RPI_FULL_PROMPT = 'pi@raspberrypi:~$ '
RPI_USERNAME_PROMPT = 'raspberrypi login: '
RPI_PASSWORD_PROMPT = 'Password: '
"""regex: used to split lines"""
LINESEPX = re.compile(r'\r\n|\n')

LOGX = re.compile(r'.*Under-voltage detected!')
"""regex: used to filter logging"""

assert LOGX.match('[57522.618196] Under-voltage detected! (0x00050005)')

OTBR_AGENT_SYSLOG_PATTERN = re.compile(r'raspberrypi otbr-agent\[\d+\]: (.*)')
assert OTBR_AGENT_SYSLOG_PATTERN.search(
    'Jun 23 05:21:22 raspberrypi otbr-agent[323]: =========[[THCI] direction=send | type=JOIN_FIN.req | len=039]==========]'
).group(1) == '=========[[THCI] direction=send | type=JOIN_FIN.req | len=039]==========]'


class OpenThread_BR(OpenThreadTHCI, IThci):
    DEFAULT_COMMAND_TIMEOUT = 20

    def _connect(self):
        self.log("logining Raspberry Pi ...")
        self.__cli_output_lines = []
        self.__syslog_skip_lines = None
        self.__syslog_last_read_ts = 0

        self.__handle = serial.Serial(self.port, 115200, timeout=0)
        self.__lines = ['']
        assert len(self.__lines) >= 1, self.__lines

        self.log("inputing username ...")
        self.__bashWriteLine('pi')
        deadline = time.time() + 20
        loginOk = False
        while time.time() < deadline:
            self.sleep(1)

            lastLine = None
            while True:
                line = self.__bashReadLine(timeout=1)

                if not line:
                    break

                lastLine = line

            if lastLine == RPI_FULL_PROMPT:
                self.log("prompt found, login success!")
                loginOk = True
                break

            if lastLine == RPI_PASSWORD_PROMPT:
                self.log("inputing password ...")
                self.__bashWriteLine('raspberry')
            elif lastLine == RPI_USERNAME_PROMPT:
                self.log("inputing username ...")
                self.__bashWriteLine('pi')
            elif not lastLine:
                self.log("inputing username ...")
                self.__bashWriteLine('pi')

        if not loginOk:
            raise Exception('login fail')

        self.bash('stty cols 256')
        self.__truncateSyslog()

    def _disconnect(self):
        if self.__handle:
            self.__handle.close()
            self.__handle = None

    def _cliReadLine(self):
        # read commissioning log if it's commissioning
        if not self.__cli_output_lines:
            self.__readSyslogToCli()

        if self.__cli_output_lines:
            return self.__cli_output_lines.pop(0)

        return None

    @watched
    def _onCommissionStart(self):
        assert self.__syslog_skip_lines is None
        self.__syslog_skip_lines = int(self.bash('wc -l /var/log/syslog')[0].split()[0])
        self.__syslog_last_read_ts = 0

    @watched
    def _onCommissionStop(self):
        assert self.__syslog_skip_lines is not None
        self.__syslog_skip_lines = None

    def __readSyslogToCli(self):
        if self.__syslog_skip_lines is None:
            return 0

        # read syslog once per second
        if time.time() < self.__syslog_last_read_ts + 1:
            return 0

        self.__syslog_last_read_ts = time.time()

        lines = self.bash('tail +%d /var/log/syslog' % self.__syslog_skip_lines)
        for line in lines:
            m = OTBR_AGENT_SYSLOG_PATTERN.search(line)
            if not m:
                continue

            self.__cli_output_lines.append(m.group(1))

        self.__syslog_skip_lines += len(lines)
        return len(lines)

    def _cliWriteLine(self, line):
        cmd = 'sudo ot-ctl "%s"' % line
        output = self.bash(cmd)
        # fake the line echo back
        self.__cli_output_lines.append(line)
        for line in output:
            self.__cli_output_lines.append(line)

    @watched
    def bash(self, cmd, timeout=DEFAULT_COMMAND_TIMEOUT):
        """
        Execute the command in bash.
        """
        self.__bashClearLines()
        self.__bashWriteLine(cmd)
        self.__bashExpect(cmd, endswith=True)

        response = []

        deadline = time.time() + timeout
        while time.time() < deadline:
            line = self.__bashReadLine()
            if line is None:
                self.sleep(0.01)
                continue

            if line == RPI_FULL_PROMPT:
                # return response lines without prompt
                return response

            response.append(line)

        self.__bashWrite('\x03')
        raise Exception('%s: failed to find end of response' % self.port)

    def __bashExpect(self, expected, timeout=DEFAULT_COMMAND_TIMEOUT, endswith=False):
        print('[%s] Expecting [%r]' % (self.port, expected))

        deadline = time.time() + timeout
        while time.time() < deadline:
            line = self.__bashReadLine()
            if line is None:
                self.sleep(0.01)
                continue

            print('[%s] Got line [%r]' % (self.port, line))

            if endswith:
                matched = line.endswith(expected)
            else:
                matched = line == expected

            if matched:
                print('[%s] Expected [%r]' % (self.port, expected))
                return

        # failed to find the expected string
        # send Ctrl+C to terminal
        self.__bashWrite('\x03')
        raise Exception('failed to find expected string[%s]' % expected)

    def __bashRead(self, timeout=1):
        deadline = time.time() + timeout
        data = ''
        while True:
            piece = self.__handle.read(self.__handle.inWaiting())
            data = data + piece
            if piece:
                continue

            if data or time.time() >= deadline:
                break

        if data:
            self.log('>>> %r', data)

        return data

    def __bashReadLine(self, timeout=1):
        line = self.__bashGetNextLine()
        if line is not None:
            return line

        assert len(self.__lines) == 1, self.__lines
        tail = self.__lines.pop()

        try:
            tail += self.__bashRead(timeout=timeout)
            tail = tail.replace(RPI_FULL_PROMPT, RPI_FULL_PROMPT + '\r\n')
            tail = tail.replace(RPI_USERNAME_PROMPT, RPI_USERNAME_PROMPT + '\r\n')
            tail = tail.replace(RPI_PASSWORD_PROMPT, RPI_PASSWORD_PROMPT + '\r\n')
        finally:
            self.__lines += [l.rstrip('\r') for l in LINESEPX.split(tail)]
            assert len(self.__lines) >= 1, self.__lines

        return self.__bashGetNextLine()

    def __bashGetNextLine(self):
        assert len(self.__lines) >= 1, self.__lines
        while len(self.__lines) > 1:
            line = self.__lines.pop(0)
            assert len(self.__lines) >= 1, self.__lines
            if LOGX.match(line):
                logging.info('LOG: %s', line)
                continue
            else:
                return line
        assert len(self.__lines) >= 1, self.__lines
        return None

    def __bashWrite(self, data):
        self.__handle.write(data)
        self.log("<<< %r", data)

    def __bashClearLines(self):
        assert len(self.__lines) >= 1, self.__lines
        while self.__bashReadLine(timeout=0) is not None:
            pass
        assert len(self.__lines) >= 1, self.__lines

    def __bashWriteLine(self, line):
        self.__bashWrite(line + '\n')

    def __truncateSyslog(self):
        self.bash('sudo truncate -s 0 /var/log/syslog')
