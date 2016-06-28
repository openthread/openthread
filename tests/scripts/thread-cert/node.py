#!/usr/bin/python
#
#  Copyright (c) 2016, Nest Labs, Inc.
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

import os
import sys
import pexpect
import unittest

class Node:
    def __init__(self, nodeid):
        self.nodeid = nodeid
        self.verbose = int(float(os.getenv('VERBOSE', 0)))
        self.node_type = os.getenv('NODE_TYPE', 'sim')

        if self.node_type == 'soc':
            self.__init_soc(nodeid)
        else:
            self.__init_sim(nodeid)

        if self.verbose:
            self.pexpect.logfile_read = sys.stdout

        self.clear_whitelist()
        self.disable_whitelist()
        self.set_timeout(100)

    def __init_sim(self, nodeid):
        """ Initialize a simulation node. """
        if "top_builddir" in os.environ.keys():
            srcdir = os.environ['top_builddir']
            cmd = '%s/examples/apps/cli/ot-cli' % srcdir
        else:
            cmd = './ot-cli'
        cmd += ' %d' % nodeid
        print cmd

        self.pexpect = pexpect.spawn(cmd, timeout=2)

    def __init_soc(self, nodeid):
        """ Initialize a System-on-a-chip node connected via UART. """
        import fdpexpect
        serialPort = '/dev/ttyUSB%d' % ((nodeid-1)*2)
        self.pexpect = fdpexpect.fdspawn(os.open(serialPort, os.O_RDWR|os.O_NONBLOCK|os.O_NOCTTY))

    def __del__(self):
        self.pexpect.terminate()
        self.pexpect.close(force=True)

    def send_command(self, cmd):
        print self.nodeid, ":", cmd
        self.pexpect.sendline(cmd)

    def get_commands(self):
        self.send_command('?')
        self.pexpect.expect('Commands:')
        commands = []
        while True:
            i = self.pexpect.expect(['Done', '(\S+)'])
            if i != 0:
                commands.append(self.pexpect.match.groups()[0])
            else:
                break
        return commands

    def set_mode(self, mode):        
        cmd = 'mode ' + mode
        self.send_command(cmd)
        self.pexpect.expect('Done')

    def start(self):
        self.send_command('start')
        self.pexpect.expect('Done')

    def stop(self):
        self.send_command('stop')
        self.pexpect.expect('Done')

    def clear_whitelist(self):
        self.send_command('whitelist clear')
        self.pexpect.expect('Done')

    def enable_whitelist(self):
        self.send_command('whitelist enable')
        self.pexpect.expect('Done')

    def disable_whitelist(self):
        self.send_command('whitelist disable')
        self.pexpect.expect('Done')

    def add_whitelist(self, addr, rssi=None):
        cmd = 'whitelist add ' + addr
        if rssi != None:
            cmd += ' ' + str(rssi)
        self.send_command(cmd)
        self.pexpect.expect('Done')

    def remove_whitelist(self, addr):
        cmd = 'whitelist remove ' + addr
        self.send_command(cmd)
        self.pexpect.expect('Done')

    def get_addr16(self):
        self.send_command('rloc16')
        i = self.pexpect.expect('([0-9a-fA-F]{4})')
        if i == 0:
            addr16 = int(self.pexpect.match.groups()[0], 16)
        self.pexpect.expect('Done')
        return addr16

    def get_addr64(self):
        self.send_command('extaddr')
        i = self.pexpect.expect('([0-9a-fA-F]{16})')
        if i == 0:
            addr64 = self.pexpect.match.groups()[0]
        self.pexpect.expect('Done')
        return addr64

    def set_channel(self, channel):
        cmd = 'channel %d' % channel
        self.send_command(cmd)
        self.pexpect.expect('Done')

    def get_key_sequence(self):
        self.send_command('keysequence')
        i = self.pexpect.expect('(\d+)\r\n')
        if i == 0:
            key_sequence = int(self.pexpect.match.groups()[0])
        self.pexpect.expect('Done')
        return key_sequence

    def set_key_sequence(self, key_sequence):
        cmd = 'keysequence %d' % key_sequence
        self.send_command(cmd)
        self.pexpect.expect('Done')

    def set_network_id_timeout(self, network_id_timeout):
        cmd = 'networkidtimeout %d' % network_id_timeout
        self.send_command(cmd)
        self.pexpect.expect('Done')

    def set_network_name(self, network_name):
        cmd = 'networkname ' + network_name
        self.send_command(cmd)
        self.pexpect.expect('Done')

    def get_panid(self):
        self.send_command('panid')
        i = self.pexpect.expect('([0-9a-fA-F]{16})')
        if i == 0:
            panid = self.pexpect.match.groups()[0]
        self.pexpect.expect('Done')

    def set_panid(self, panid):
        cmd = 'panid %d' % panid
        self.send_command(cmd)
        self.pexpect.expect('Done')

    def set_router_upgrade_threshold(self, threshold):
        cmd = 'routerupgradethreshold %d' % threshold
        self.send_command(cmd)
        self.pexpect.expect('Done')

    def release_router_id(self, router_id):
        cmd = 'releaserouterid %d' % router_id
        self.send_command(cmd)
        self.pexpect.expect('Done')

    def get_state(self):
        states = ['detached', 'child', 'router', 'leader']
        self.send_command('state')
        match = self.pexpect.expect(states)
        self.pexpect.expect('Done')
        return states[match]

    def set_state(self, state):
        cmd = 'state ' + state
        self.send_command(cmd)
        self.pexpect.expect('Done')

    def get_timeout(self):
        self.send_command('childtimeout')
        i = self.pexpect.expect('(\d+)\r\n')
        if i == 0:
            timeout = self.pexpect.match.groups()[0]
        self.pexpect.expect('Done')
        return timeout

    def set_timeout(self, timeout):
        cmd = 'childtimeout %d' % timeout
        self.send_command(cmd)
        self.pexpect.expect('Done')

    def get_weight(self):
        self.send_command('leaderweight')
        i = self.pexpect.expect('(\d+)\r\n')
        if i == 0:
            weight = self.pexpect.match.groups()[0]
        self.pexpect.expect('Done')
        return weight

    def set_weight(self, weight):
        cmd = 'leaderweight %d' % weight
        self.send_command(cmd)
        self.pexpect.expect('Done')

    def add_ipaddr(self, ipaddr):
        cmd = 'ipaddr add ' + ipaddr
        self.send_command(cmd)
        self.pexpect.expect('Done')

    def get_addrs(self):
        addrs = []
        self.send_command('ipaddr')

        while True:
            i = self.pexpect.expect(['(\S+:\S+)\r\n', 'Done'])
            if i == 0:
                addrs.append(self.pexpect.match.groups()[0])
            elif i == 1:
                break

        return addrs

    def get_context_reuse_delay(self):
        self.send_command('contextreusedelay')
        i = self.pexpect.expect('(\d+)\r\n')
        if i == 0:
            timeout = self.pexpect.match.groups()[0]
        self.pexpect.expect('Done')
        return timeout

    def set_context_reuse_delay(self, delay):
        cmd = 'contextreusedelay %d' % delay
        self.send_command(cmd)
        self.pexpect.expect('Done')

    def add_prefix(self, prefix, flags, prf = 'med'):
        cmd = 'prefix add ' + prefix + ' ' + flags + ' ' + prf
        self.send_command(cmd)
        self.pexpect.expect('Done')

    def remove_prefix(self, prefix):
        cmd = ' prefix remove ' + prefix
        self.send_command(cmd)
        self.pexpect.expect('Done')

    def add_route(self, prefix, prf = 'med'):
        cmd = 'route add ' + prefix + ' ' + prf
        self.send_command(cmd)
        self.pexpect.expect('Done')

    def remove_route(self, prefix):
        cmd = 'route remove ' + prefix
        self.send_command(cmd)
        self.pexpect.expect('Done')

    def register_netdata(self):
        self.send_command('netdataregister')
        self.pexpect.expect('Done')

    def scan(self):
        self.send_command('scan')

        results = []
        while True:
            i = self.pexpect.expect(['\|\s(\S+)\s+\|\s(\S+)\s+\|\s([0-9a-fA-F]{4})\s\|\s([0-9a-fA-F]{16})\s\|\s(\d+)\r\n',
                                     'Done'])
            if i == 0:
                results.append(self.pexpect.match.groups())
            else:
                break

        return results

    def ping(self, ipaddr, num_responses=1, size=None):
        cmd = 'ping ' + ipaddr
        if size != None:
            cmd += ' ' + str(size)

        self.send_command(cmd)
        responders = {}
        while len(responders) < num_responses:
            i = self.pexpect.expect(['from (\S+):'])
            if i == 0:
                responders[self.pexpect.match.groups()[0]] = 1
        self.pexpect.expect('\n')
        return responders

if __name__ == '__main__':
    unittest.main()
