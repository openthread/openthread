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
>> Device : OpenThread_BR_Sim THCI
>> Class : OpenThread_BR_Sim
"""

import ipaddress
import logging
import paramiko
import pipes
import sys
import time

from THCI.IThci import IThci
from THCI.OpenThread import watched
from THCI.OpenThread_BR import OpenThread_BR
from simulation.config import load_config

logging.getLogger('paramiko').setLevel(logging.WARNING)

config = load_config()


class SSHHandle(object):
    # Unit: second
    KEEPALIVE_INTERVAL = 30

    def __init__(self, ip, port, username, password, docker_name):
        self.ip = ip
        self.port = int(port)
        self.username = username
        self.password = password
        self.docker_name = docker_name
        self.__handle = None

        self.__connect()

    def __connect(self):
        self.close()

        self.__handle = paramiko.SSHClient()
        self.__handle.set_missing_host_key_policy(paramiko.AutoAddPolicy())
        try:
            self.__handle.connect(self.ip, port=self.port, username=self.username, password=self.password)
        except paramiko.AuthenticationException:
            if not self.password:
                self.__handle.get_transport().auth_none(self.username)
            else:
                raise Exception('Password error')

        # Avoid SSH disconnection after idle for a long time
        self.__handle.get_transport().set_keepalive(self.KEEPALIVE_INTERVAL)

    def close(self):
        if self.__handle is not None:
            self.__handle.close()
            self.__handle = None

    def bash(self, cmd, timeout):
        # It is necessary to quote the command when there is stdin/stdout redirection
        cmd = pipes.quote(cmd)

        retry = 3
        for i in range(retry):
            try:
                stdin, stdout, stderr = self.__handle.exec_command('docker exec %s bash -c %s' %
                                                                   (self.docker_name, cmd),
                                                                   timeout=timeout)
                stdout._set_mode('rb')

                sys.stderr.write(stderr.read())
                output = [r.rstrip() for r in stdout.readlines()]
                return output

            except paramiko.SSHException:
                if i < retry - 1:
                    print('SSH connection is lost, try reconnect after 1 second.')
                    time.sleep(1)
                    self.__connect()
                else:
                    raise ConnectionError('SSH connection is lost')


class OpenThread_BR_Sim(OpenThread_BR):

    def _getHandle(self):
        self.log('SSH connecting ...')
        return SSHHandle(self.ssh_ip, self.telnetPort, self.telnetUsername, self.telnetPassword, self.docker_name)

    @watched
    def _parseConnectionParams(self, params):
        discovery_add = params.get('SerialPort')
        if '@' not in discovery_add:
            raise ValueError('%r in the field `add` is invalid' % discovery_add)

        self.docker_name, self.ssh_ip = discovery_add.split('@')
        self.tag, self.node_id = self.docker_name.split('_')
        self.node_id = int(self.node_id)
        # Let it crash if it is an invalid IP address
        ipaddress.ip_address(self.ssh_ip)

        self.connectType = 'ip'
        self.telnetIp = self.port = discovery_add

        global config
        ssh = config['ssh']
        self.telnetPort = ssh['port']
        self.telnetUsername = ssh['username']
        self.telnetPassword = ssh['password']

        self.extraParams = {
            'cmd-start-otbr-agent': 'service otbr-agent start',
            'cmd-stop-otbr-agent': 'service otbr-agent stop',
            'cmd-restart-otbr-agent': 'service otbr-agent restart',
            'cmd-restart-radvd': 'service radvd stop; service radvd start',
        }


assert issubclass(OpenThread_BR_Sim, IThci)
