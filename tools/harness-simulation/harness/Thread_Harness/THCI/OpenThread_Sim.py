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
import paramiko
import socket
import time
import win32api

from simulation.config import load_config
from THCI.IThci import IThci
from THCI.OpenThread import OpenThreadTHCI, watched

config = load_config()
ot_subpath = {item['tag']: item['subpath'] for item in config['ot_build']['ot']}


class SSHHandle(object):
    KEEPALIVE_INTERVAL = 30

    def __init__(self, ip, port, username, password, device, node_id):
        self.ip = ip
        self.port = int(port)
        self.username = username
        self.password = password
        self.node_id = node_id
        self.__handle = None
        self.__stdin = None
        self.__stdout = None

        self.__connect(device)

        # Close the SSH connection only when Harness exits
        win32api.SetConsoleCtrlHandler(self.__disconnect, True)

    @watched
    def __connect(self, device):
        if self.__handle is not None:
            return

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

        # Avoid SSH connection lost after inactivity for a while
        self.__handle.get_transport().set_keepalive(self.KEEPALIVE_INTERVAL)

        self.__stdin, self.__stdout, _ = self.__handle.exec_command(device + ' ' + str(self.node_id))

        # Receive the output in non-blocking mode
        self.__stdout.channel.setblocking(0)

        # Some commands such as `udp send <ip> -x <hex>` send binary data
        # The UDP packet receiver will output the data in binary to stdout
        self.__stdout._set_mode('rb')

    def __disconnect(self, dwCtrlType):
        if self.__handle is None:
            return

        # Exit ot-cli-ftd and close the SSH connection
        self.send('exit\n')
        self.__stdin.close()
        self.__stdout.close()
        self.__handle.close()

    def close(self):
        # Do nothing, because disconnecting and then connecting will automatically factory reset all states
        # compared to real devices, which is not the intended behavior
        pass

    def send(self, cmd):
        self.__stdin.write(cmd)
        self.__stdin.flush()

    def recv(self):
        outputs = []
        while True:
            try:
                outputs.append(self.__stdout.read(1))
            except socket.timeout:
                break
        return ''.join(outputs)

    def log(self, fmt, *args):
        try:
            msg = fmt % args
            print('%d@%s - %s - %s' % (self.node_id, self.ip, time.strftime('%b %d %H:%M:%S'), msg))
        except Exception:
            pass


class OpenThread_Sim(OpenThreadTHCI, IThci):
    __handle = None

    @watched
    def _connect(self):
        self.__lines = []

        # Only actually connect once.
        if self.__handle is None:
            self.log('SSH connecting ...')
            self.__handle = SSHHandle(self.ssh_ip, self.telnetPort, self.telnetUsername, self.telnetPassword,
                                      self.device, self.node_id)

        self.log('connected to %s successfully', self.telnetIp)

    @watched
    def _disconnect(self):
        pass

    @watched
    def _parseConnectionParams(self, params):
        discovery_add = params.get('SerialPort')
        if '@' not in discovery_add:
            raise ValueError('%r in the field `add` is invalid' % discovery_add)

        prefix, self.ssh_ip = discovery_add.split('@')
        self.tag, self.node_id = prefix.split('_')
        self.node_id = int(self.node_id)
        # Let it crash if it is an invalid IP address
        ipaddress.ip_address(self.ssh_ip)

        # Do not use `os.path.join` as it uses backslash as the separator on Windows
        global config
        self.device = '/'.join([config['ot_path'], ot_subpath[self.tag], 'examples/apps/cli/ot-cli-ftd'])

        self.connectType = 'ip'
        self.telnetIp = self.port = discovery_add

        ssh = config['ssh']
        self.telnetPort = ssh['port']
        self.telnetUsername = ssh['username']
        self.telnetPassword = ssh['password']

    def _cliReadLine(self):
        if len(self.__lines) > 1:
            return self.__lines.pop(0)

        tail = ''
        if len(self.__lines) != 0:
            tail = self.__lines.pop()

        tail += self.__handle.recv()

        self.__lines += self._lineSepX.split(tail)
        if len(self.__lines) > 1:
            return self.__lines.pop(0)

        return None

    def _cliWriteLine(self, line):
        self.__handle.send(line + '\n')

    def _onCommissionStart(self):
        pass

    def _onCommissionStop(self):
        pass
