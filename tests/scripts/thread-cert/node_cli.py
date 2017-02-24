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

import os
import sys
import time
import pexpect

class otCli:
    def __init__(self, nodeid):
        self.nodeid = nodeid
        self.verbose = int(float(os.getenv('VERBOSE', 0)))
        self.node_type = os.getenv('NODE_TYPE', 'sim')

        if self.node_type == 'soc':
            self.__init_soc(nodeid)
        elif self.node_type == 'ncp-sim':
            self.__init_ncp_sim(nodeid)
        else:
            self.__init_sim(nodeid)

        if self.verbose:
            self.pexpect.logfile_read = sys.stdout

    def __init_sim(self, nodeid):
        """ Initialize a simulation node. """
        if "OT_CLI_PATH" in os.environ.keys():
            cmd = os.environ['OT_CLI_PATH']
        elif "top_builddir" in os.environ.keys():
            srcdir = os.environ['top_builddir']
            cmd = '%s/examples/apps/cli/ot-cli-ftd' % srcdir
        else:
            cmd = './ot-cli-ftd'
        cmd += ' %d' % nodeid
        print ("%s" % cmd)

        self.pexpect = pexpect.spawn(cmd, timeout=4)

        # Add delay to ensure that the process is ready to receive commands.
        time.sleep(0.2)


    def __init_ncp_sim(self, nodeid):
        """ Initialize an NCP simulation node. """
        if "top_builddir" in os.environ.keys():
            builddir = os.environ['top_builddir']
            if "top_srcdir" in os.environ.keys():
                srcdir = os.environ['top_srcdir']
            else:
                srcdir = os.path.dirname(os.path.realpath(__file__))
                srcdir += "/../../.."
            cmd = 'python %s/tools/spinel-cli/spinel-cli.py -p %s/examples/apps/ncp/ot-ncp-ftd -n' % (srcdir, builddir)
        else:
            cmd = './ot-ncp-ftd'
        cmd += ' %d' % nodeid
        print ("%s" % cmd)

        self.pexpect = pexpect.spawn(cmd, timeout=4)
        time.sleep(0.2)
        self.pexpect.expect('spinel-cli >')
        self.debug(int(os.getenv('DEBUG', '0')))
 
    def __init_soc(self, nodeid):
        """ Initialize a System-on-a-chip node connected via UART. """
        import fdpexpect
        serialPort = '/dev/ttyUSB%d' % ((nodeid-1)*2)
        self.pexpect = fdpexpect.fdspawn(os.open(serialPort, os.O_RDWR|os.O_NONBLOCK|os.O_NOCTTY))

    def __del__(self):
        if self.pexpect.isalive():
            self.send_command('exit')
            self.pexpect.expect(pexpect.EOF)
            self.pexpect.terminate()
            self.pexpect.close(force=True)

    def send_command(self, cmd):
        print ("%d: %s" % (self.nodeid, cmd))
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

    def debug(self, level):
        self.send_command('debug '+str(level))

    def interface_up(self):
        self.send_command('ifconfig up')
        self.pexpect.expect('Done')

    def interface_down(self):
        self.send_command('ifconfig down')
        self.pexpect.expect('Done')

    def thread_start(self):
        self.send_command('thread start')
        self.pexpect.expect('Done')

    def thread_stop(self):
        self.send_command('thread stop')
        self.pexpect.expect('Done')
            
    def commissioner_start(self):
        cmd = 'commissioner start'
        self.send_command(cmd)
        self.pexpect.expect('Done')

    def commissioner_add_joiner(self, addr, psk):
        cmd = 'commissioner joiner add ' + addr + ' ' + psk
        self.send_command(cmd)
        self.pexpect.expect('Done')

    def joiner_start(self, pskd='', provisioning_url=''):
        cmd = 'joiner start ' + pskd + ' ' + provisioning_url
        self.send_command(cmd)
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
            addr64 = self.pexpect.match.groups()[0].decode("utf-8")
        self.pexpect.expect('Done')
        return addr64

    def get_hashmacaddr(self):
        self.send_command('hashmacaddr')
        i = self.pexpect.expect('([0-9a-fA-F]{16})')
        if i == 0:
            addr = self.pexpect.match.groups()[0].decode("utf-8")
        self.pexpect.expect('Done')
        return addr

    def get_channel(self):
        self.send_command('channel')
        i = self.pexpect.expect('(\d+)\r\n')
        if i == 0:
            channel = int(self.pexpect.match.groups()[0])
        self.pexpect.expect('Done')
        return channel

    def set_channel(self, channel):
        cmd = 'channel %d' % channel
        self.send_command(cmd)
        self.pexpect.expect('Done')

    def get_masterkey(self):
        self.send_command('masterkey')
        i = self.pexpect.expect('([0-9a-fA-F]{32})')
        if i == 0:
            masterkey = self.pexpect.match.groups()[0].decode("utf-8")
        self.pexpect.expect('Done')
        return masterkey

    def set_masterkey(self, masterkey):
        cmd = 'masterkey ' + masterkey
        self.send_command(cmd)
        self.pexpect.expect('Done')

    def get_key_sequence_counter(self):
        self.send_command('keysequence counter')
        i = self.pexpect.expect('(\d+)\r\n')
        if i == 0:
            key_sequence_counter = int(self.pexpect.match.groups()[0])
        self.pexpect.expect('Done')
        return key_sequence_counter

    def set_key_sequence_counter(self, key_sequence_counter):
        cmd = 'keysequence counter %d' % key_sequence_counter
        self.send_command(cmd)
        self.pexpect.expect('Done')

    def set_key_switch_guardtime(self, key_switch_guardtime):
        cmd = 'keysequence guardtime %d' % key_switch_guardtime
        self.send_command(cmd)
        self.pexpect.expect('Done')

    def set_network_id_timeout(self, network_id_timeout):
        cmd = 'networkidtimeout %d' % network_id_timeout
        self.send_command(cmd)
        self.pexpect.expect('Done')

    def get_network_name(self):
        self.send_command('networkname')
        while True:
            i = self.pexpect.expect(['Done', '(\S+)'])
            if i != 0:
                network_name = self.pexpect.match.groups()[0].decode('utf-8')
            else:
                break
        return network_name

    def set_network_name(self, network_name):
        cmd = 'networkname ' + network_name
        self.send_command(cmd)
        self.pexpect.expect('Done')

    def get_panid(self):
        self.send_command('panid')
        i = self.pexpect.expect('([0-9a-fA-F]{4})')
        if i == 0:
            panid = int(self.pexpect.match.groups()[0], 16)
        self.pexpect.expect('Done')
        return panid

    def set_panid(self, panid):
        cmd = 'panid %d' % panid
        self.send_command(cmd)
        self.pexpect.expect('Done')

    def get_partition_id(self):
        self.send_command('leaderpartitionid')
        i = self.pexpect.expect('(\d+)\r\n')
        if i == 0:
            weight = self.pexpect.match.groups()[0]
        self.pexpect.expect('Done')
        return weight

    def set_partition_id(self, partition_id):
        cmd = 'leaderpartitionid %d' % partition_id
        self.send_command(cmd)
        self.pexpect.expect('Done')

    def set_router_upgrade_threshold(self, threshold):
        cmd = 'routerupgradethreshold %d' % threshold
        self.send_command(cmd)
        self.pexpect.expect('Done')

    def set_router_downgrade_threshold(self, threshold):
        cmd = 'routerdowngradethreshold %d' % threshold
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

    def set_max_children(self, number):
        cmd = 'childmax %d' % number
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
                addrs.append(self.pexpect.match.groups()[0].decode("utf-8"))
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

    def energy_scan(self, mask, count, period, scan_duration, ipaddr):
        cmd = 'commissioner energy ' + str(mask) + ' ' + str(count) + ' ' + str(period) + ' ' + str(scan_duration) + ' ' + ipaddr
        self.send_command(cmd)
        self.pexpect.expect('Energy:', timeout=8)

    def panid_query(self, panid, mask, ipaddr):
        cmd = 'commissioner panid ' + str(panid) + ' ' + str(mask) + ' ' + ipaddr
        self.send_command(cmd)
        self.pexpect.expect('Conflict:', timeout=8)

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

        result = True
        try:
            responders = {}
            while len(responders) < num_responses:
                i = self.pexpect.expect(['from (\S+):'])
                if i == 0:
                    responders[self.pexpect.match.groups()[0]] = 1
            self.pexpect.expect('\n')
        except pexpect.TIMEOUT:
            result = False

        return result

    def set_router_selection_jitter(self, jitter):
        cmd = 'routerselectionjitter %d' % jitter
        self.send_command(cmd)
        self.pexpect.expect('Done')

    def set_active_dataset(self, timestamp, panid=None, channel=None, channel_mask=None, master_key=None):
        self.send_command('dataset clear')
        self.pexpect.expect('Done')

        cmd = 'dataset activetimestamp %d' % timestamp
        self.send_command(cmd)
        self.pexpect.expect('Done')

        if panid != None:
            cmd = 'dataset panid %d' % panid
            self.send_command(cmd)
            self.pexpect.expect('Done')

        if channel != None:
            cmd = 'dataset channel %d' % channel
            self.send_command(cmd)
            self.pexpect.expect('Done')

        if channel_mask != None:
            cmd = 'dataset channelmask %d' % channel_mask
            self.send_command(cmd)
            self.pexpect.expect('Done')

        if master_key != None:
            cmd = 'dataset masterkey ' + master_key
            self.send_command(cmd)
            self.pexpect.expect('Done')

        self.send_command('dataset commit active')
        self.pexpect.expect('Done')

    def set_pending_dataset(self, pendingtimestamp, activetimestamp, panid=None, channel=None):
        self.send_command('dataset clear')
        self.pexpect.expect('Done')

        cmd = 'dataset pendingtimestamp %d' % pendingtimestamp
        self.send_command(cmd)
        self.pexpect.expect('Done')

        cmd = 'dataset activetimestamp %d' % activetimestamp
        self.send_command(cmd)
        self.pexpect.expect('Done')

        if panid != None:
            cmd = 'dataset panid %d' % panid
            self.send_command(cmd)
            self.pexpect.expect('Done')

        if channel != None:
            cmd = 'dataset channel %d' % channel
            self.send_command(cmd)
            self.pexpect.expect('Done')

        self.send_command('dataset commit pending')
        self.pexpect.expect('Done')

    def announce_begin(self, mask, count, period, ipaddr):
        cmd = 'commissioner announce ' + str(mask) + ' ' + str(count) + ' ' + str(period) + ' ' + ipaddr
        self.send_command(cmd)
        self.pexpect.expect('Done')

    def send_mgmt_active_set(self, active_timestamp=None, channel=None, channel_mask=None, extended_panid=None,
                             panid=None, master_key=None, mesh_local=None, network_name=None, binary=None):
        cmd = 'dataset mgmtsetcommand active '

        if active_timestamp != None:
            cmd += 'activetimestamp %d ' % active_timestamp

        if channel != None:
            cmd += 'channel %d ' % channel

        if channel_mask != None:
            cmd += 'channelmask %d ' % channel_mask

        if extended_panid != None:
            cmd += 'extpanid ' + extended_panid + ' '

        if panid != None:
            cmd += 'panid %d ' % panid

        if master_key != None:
            cmd += 'masterkey ' + master_key + ' '

        if mesh_local != None:
            cmd += 'localprefix ' + mesh_local + ' '

        if network_name != None:
            cmd += 'networkname ' + network_name + ' '

        if binary != None:
            cmd += 'binary ' + binary + ' '

        self.send_command(cmd)
        self.pexpect.expect('Done')

    def send_mgmt_pending_set(self, pending_timestamp=None, active_timestamp=None, delay_timer=None, channel=None,
                              panid=None, master_key=None, mesh_local=None, network_name=None):
        cmd = 'dataset mgmtsetcommand pending '

        if pending_timestamp != None:
            cmd += 'pendingtimestamp %d ' % pending_timestamp

        if active_timestamp != None:
            cmd += 'activetimestamp %d ' % active_timestamp

        if delay_timer != None:
            cmd += 'delaytimer %d ' % delay_timer

        if channel != None:
            cmd += 'channel %d ' % channel

        if panid != None:
            cmd += 'panid %d ' % panid

        if master_key != None:
            cmd += 'masterkey ' + master_key + ' '

        if mesh_local != None:
            cmd += 'localprefix ' + mesh_local + ' '

        if network_name != None:
            cmd += 'networkname ' + network_name + ' '

        self.send_command(cmd)
        self.pexpect.expect('Done')
