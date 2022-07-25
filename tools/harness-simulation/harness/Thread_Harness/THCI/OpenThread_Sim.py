#!/usr/bin/env python
#
# Copyright (c) 2022, The OpenThread Authors.
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
"""
>> Thread Host Controller Interface
>> Device : OpenThread_Sim THCI
>> Class : OpenThread_Sim
"""

import ipaddress
import os
import paramiko
import socket
import time

from IThci import IThci
from OpenThread import OpenThreadTHCI, watched
from simulation.config import REMOTE_OT_PATH


class SSHHandle(object):

    def __init__(self, ip, port, username, password, device, node_id):
        ipaddress.ip_address(ip)
        self.ip = ip
        self.port = int(port)
        self.username = username
        self.password = password
        self.__handle = None
        self.__stdin = None
        self.__stdout = None
        self.__connect(device, node_id)

    @watched
    def __connect(self, device, node_id):
        self.close()

        self.__handle = paramiko.SSHClient()
        self.__handle.set_missing_host_key_policy(paramiko.AutoAddPolicy())
        try:
            self.log('Connecting to %s:%s with username=%s', self.ip, self.port, self.username)
            self.__handle.connect(self.ip, port=self.port, username=self.username, password=self.password)
        except paramiko.AuthenticationException:
            if not self.password:
                self.__handle.get_transport().auth_none(self.username)
            else:
                raise Exception('Password error')

        self.__stdin, self.__stdout, _ = self.__handle.exec_command(device + ' ' + str(node_id), get_pty=True)
        self.__stdout.channel.setblocking(0)

        # Wait some time for initiation
        time.sleep(0.1)

    @watched
    def close(self):
        if self.__handle is None:
            return
        self.__stdin.write('exit\n')
        # Wait some time for termination
        time.sleep(0.1)
        self.__handle.close()
        self.__stdin = None
        self.__stdout = None
        self.__handle = None

    def send(self, cmd):
        self.__stdin.write(cmd)

    def recv(self):
        try:
            return self.__stdout.readline().rstrip()
        except socket.timeout:
            return ''

    def log(self, fmt, *args):
        try:
            msg = fmt % args
            print('%s - %s - %s' % (self.port, time.strftime('%b %d %H:%M:%S'), msg))
        except Exception:
            pass


class OpenThread_Sim(OpenThreadTHCI, IThci):
    DEFAULT_COMMAND_TIMEOUT = 20

    __handle = None

    device = os.path.join(REMOTE_OT_PATH, 'build/simulation/examples/apps/cli/ot-cli-ftd')

    @watched
    def _connect(self):
        # Only actually connect once.
        if self.__handle is None:
            assert self.connectType == 'ip'
            assert '@' in self.telnetIp
            self.log('SSH connecting ...')
            node_id, ssh_ip = self.telnetIp.split('@')
            self.__handle = SSHHandle(ssh_ip, self.telnetPort, self.telnetUsername, self.telnetPassword, self.device,
                                      node_id)

        self.log('connected to %s successfully', self.telnetIp)

    @watched
    def _disconnect(self):
        pass

    def _cliReadLine(self):
        tail = self.__handle.recv()
        return tail if tail else None

    def _cliWriteLine(self, line):
        self.__handle.send(line + '\n')

    def _onCommissionStart(self):
        pass

    def _onCommissionStop(self):
        pass
