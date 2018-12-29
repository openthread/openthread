#!/usr/bin/env python
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
import pexpect.popen_spawn
import re
import socket
import ipaddress

import config
import simulator

class otCli:
    def __init__(self, nodeid, is_mtd, simulator=None):
        self.nodeid = nodeid
        self.verbose = int(float(os.getenv('VERBOSE', 0)))
        self.node_type = os.getenv('NODE_TYPE', 'sim')
        self.simulator = simulator

        mode = os.environ.get('USE_MTD') is '1' and is_mtd and 'mtd' or 'ftd'

        if self.node_type == 'soc':
            self.__init_soc(nodeid)
        elif self.node_type == 'ncp-sim':
            # TODO use mode after ncp-mtd is available.
            self.__init_ncp_sim(nodeid, 'ftd')
        else:
            self.__init_sim(nodeid, mode)

        if self.verbose:
            if sys.version_info[0] == 2:
                self.pexpect.logfile_read = sys.stdout
            else:
                self.pexpect.logfile_read = sys.stdout.buffer

    def __init_sim(self, nodeid, mode):
        """ Initialize a simulation node. """
        if 'OT_CLI_PATH' in os.environ.keys():
            cmd = os.environ['OT_CLI_PATH']
        elif 'top_builddir' in os.environ.keys():
            srcdir = os.environ['top_builddir']
            cmd = '%s/examples/apps/cli/ot-cli-%s' % (srcdir, mode)
        else:
            cmd = './ot-cli-%s' % mode

        if 'RADIO_DEVICE' in os.environ:
            cmd += ' %s' % os.environ['RADIO_DEVICE']
            os.environ['NODE_ID'] = str(nodeid)

        cmd += ' %d' % nodeid
        print ("%s" % cmd)

        self.pexpect = pexpect.popen_spawn.PopenSpawn(cmd, timeout=4)

        # Add delay to ensure that the process is ready to receive commands.
        timeout = 0.4
        while timeout > 0:
            self.pexpect.send('\r\n')
            try:
                self.pexpect.expect('> ', timeout=0.1)
                break
            except pexpect.TIMEOUT:
                timeout -= 0.1

    def __init_ncp_sim(self, nodeid, mode):
        """ Initialize an NCP simulation node. """
        if 'RADIO_DEVICE' in os.environ:
            args = ' %s' % os.environ['RADIO_DEVICE']
            os.environ['NODE_ID'] = str(nodeid)
        else:
            args = ''

        if 'OT_NCP_PATH' in os.environ.keys():
            cmd = 'spinel-cli.py -p "%s%s" -n' % (os.environ['OT_NCP_PATH'], args)
        elif "top_builddir" in os.environ.keys():
            builddir = os.environ['top_builddir']
            cmd = 'spinel-cli.py -p "%s/examples/apps/ncp/ot-ncp-%s%s" -n' % (builddir, mode, args)
        else:
            cmd = 'spinel-cli.py -p "./ot-ncp-%s%s" -n' % (mode, args)
        cmd += ' %d' % nodeid
        print ("%s" % cmd)

        self.pexpect = pexpect.spawn(cmd, timeout=4)

        # Add delay to ensure that the process is ready to receive commands.
        time.sleep(0.2)
        self._expect('spinel-cli >')
        self.debug(int(os.getenv('DEBUG', '0')))

    def _expect(self, pattern, timeout=-1, *args, **kwargs):
        """ Process simulator events until expected the pattern. """
        if timeout == -1:
            timeout = self.pexpect.timeout

        assert timeout > 0

        while timeout > 0:
            try:
                return self.pexpect.expect(pattern, 0.1, *args, **kwargs)
            except pexpect.TIMEOUT:
                timeout -= 0.1
                self.simulator.go(0)
                if timeout <= 0:
                    raise

    def __init_soc(self, nodeid):
        """ Initialize a System-on-a-chip node connected via UART. """
        import fdpexpect
        serialPort = '/dev/ttyUSB%d' % ((nodeid-1)*2)
        self.pexpect = fdpexpect.fdspawn(os.open(serialPort, os.O_RDWR|os.O_NONBLOCK|os.O_NOCTTY))

    def __del__(self):
        self.destroy()

    def destroy(self):
        if not self.pexpect:
            return

        if hasattr(self.pexpect, 'proc') and self.pexpect.proc.poll() is None or \
                not hasattr(self.pexpect, 'proc') and self.pexpect.isalive():
            print("%d: exit" % self.nodeid)
            self.pexpect.send('exit\n')
            self.pexpect.expect(pexpect.EOF)
            self.pexpect = None

    def send_command(self, cmd, go=True):
        print("%d: %s" % (self.nodeid, cmd))
        self.pexpect.send(cmd + '\n')
        if go:
            self.simulator.go(0, nodeid=self.nodeid)
        sys.stdout.flush()

    def get_commands(self):
        self.send_command('?')
        self._expect('Commands:')
        commands = []
        while True:
            i = self._expect(['Done', '(\S+)'])
            if i != 0:
                commands.append(self.pexpect.match.groups()[0])
            else:
                break
        return commands

    def set_mode(self, mode):
        cmd = 'mode ' + mode
        self.send_command(cmd)
        self._expect('Done')

    def debug(self, level):
        # `debug` command will not trigger interaction with simulator
        self.send_command('debug '+ str(level), go=False)

    def interface_up(self):
        self.send_command('ifconfig up')
        self._expect('Done')

    def interface_down(self):
        self.send_command('ifconfig down')
        self._expect('Done')

    def thread_start(self):
        self.send_command('thread start')
        self._expect('Done')

    def thread_stop(self):
        self.send_command('thread stop')
        self._expect('Done')

    def commissioner_start(self):
        cmd = 'commissioner start'
        self.send_command(cmd)
        self._expect('Done')

    def commissioner_add_joiner(self, addr, psk):
        cmd = 'commissioner joiner add ' + addr + ' ' + psk
        self.send_command(cmd)
        self._expect('Done')

    def joiner_start(self, pskd='', provisioning_url=''):
        cmd = 'joiner start ' + pskd + ' ' + provisioning_url
        self.send_command(cmd)
        self._expect('Done')

    def clear_whitelist(self):
        cmd = 'macfilter addr clear'
        self.send_command(cmd)
        self._expect('Done')

    def enable_whitelist(self):
        cmd = 'macfilter addr whitelist'
        self.send_command(cmd)
        self._expect('Done')

    def disable_whitelist(self):
        cmd = 'macfilter addr disable'
        self.send_command(cmd)
        self._expect('Done')

    def add_whitelist(self, addr, rssi=None):
        cmd = 'macfilter addr add ' + addr

        if rssi != None:
            cmd += ' ' + str(rssi)

        self.send_command(cmd)
        self._expect('Done')

    def remove_whitelist(self, addr):
        cmd = 'macfilter addr remove ' + addr
        self.send_command(cmd)
        self._expect('Done')

    def get_addr16(self):
        self.send_command('rloc16')
        i = self._expect('([0-9a-fA-F]{4})')
        if i == 0:
            addr16 = int(self.pexpect.match.groups()[0], 16)
        self._expect('Done')
        return addr16

    def get_router_id(self):
        rloc16 = self.get_addr16()
        return (rloc16 >> 10)

    def get_addr64(self):
        self.send_command('extaddr')
        i = self._expect('([0-9a-fA-F]{16})')
        if i == 0:
            addr64 = self.pexpect.match.groups()[0].decode("utf-8")

        self._expect('Done')
        return addr64

    def get_eui64(self):
        self.send_command('eui64')
        i = self._expect('([0-9a-fA-F]{16})')
        if i == 0:
            addr64 = self.pexpect.match.groups()[0].decode("utf-8")

        self._expect('Done')
        return addr64

    def get_joiner_id(self):
        self.send_command('joinerid')
        i = self._expect('([0-9a-fA-F]{16})')
        if i == 0:
            addr = self.pexpect.match.groups()[0].decode("utf-8")
        self._expect('Done')
        return addr

    def get_channel(self):
        self.send_command('channel')
        i = self._expect('(\d+)\r?\n')
        if i == 0:
            channel = int(self.pexpect.match.groups()[0])
        self._expect('Done')
        return channel

    def set_channel(self, channel):
        cmd = 'channel %d' % channel
        self.send_command(cmd)
        self._expect('Done')

    def get_masterkey(self):
        self.send_command('masterkey')
        i = self._expect('([0-9a-fA-F]{32})')
        if i == 0:
            masterkey = self.pexpect.match.groups()[0].decode("utf-8")
        self._expect('Done')
        return masterkey

    def set_masterkey(self, masterkey):
        cmd = 'masterkey ' + masterkey
        self.send_command(cmd)
        self._expect('Done')

    def get_key_sequence_counter(self):
        self.send_command('keysequence counter')
        i = self._expect('(\d+)\r?\n')
        if i == 0:
            key_sequence_counter = int(self.pexpect.match.groups()[0])
        self._expect('Done')
        return key_sequence_counter

    def set_key_sequence_counter(self, key_sequence_counter):
        cmd = 'keysequence counter %d' % key_sequence_counter
        self.send_command(cmd)
        self._expect('Done')

    def set_key_switch_guardtime(self, key_switch_guardtime):
        cmd = 'keysequence guardtime %d' % key_switch_guardtime
        self.send_command(cmd)
        self._expect('Done')

    def set_network_id_timeout(self, network_id_timeout):
        cmd = 'networkidtimeout %d' % network_id_timeout
        self.send_command(cmd)
        self._expect('Done')

    def get_network_name(self):
        self.send_command('networkname')
        while True:
            i = self._expect(['Done', '(\S+)'])
            if i != 0:
                network_name = self.pexpect.match.groups()[0].decode('utf-8')
            else:
                break
        return network_name

    def set_network_name(self, network_name):
        cmd = 'networkname ' + network_name
        self.send_command(cmd)
        self._expect('Done')

    def get_panid(self):
        self.send_command('panid')
        i = self._expect('([0-9a-fA-F]{4})')
        if i == 0:
            panid = int(self.pexpect.match.groups()[0], 16)
        self._expect('Done')
        return panid

    def set_panid(self, panid):
        cmd = 'panid %d' % panid
        self.send_command(cmd)
        self._expect('Done')

    def get_partition_id(self):
        self.send_command('leaderpartitionid')
        i = self._expect('(\d+)\r?\n')
        if i == 0:
            weight = self.pexpect.match.groups()[0]
        self._expect('Done')
        return weight

    def set_partition_id(self, partition_id):
        cmd = 'leaderpartitionid %d' % partition_id
        self.send_command(cmd)
        self._expect('Done')

    def set_router_upgrade_threshold(self, threshold):
        cmd = 'routerupgradethreshold %d' % threshold
        self.send_command(cmd)
        self._expect('Done')

    def set_router_downgrade_threshold(self, threshold):
        cmd = 'routerdowngradethreshold %d' % threshold
        self.send_command(cmd)
        self._expect('Done')

    def release_router_id(self, router_id):
        cmd = 'releaserouterid %d' % router_id
        self.send_command(cmd)
        self._expect('Done')

    def get_state(self):
        states = ['detached', 'child', 'router', 'leader']
        self.send_command('state')
        match = self._expect(states)
        self._expect('Done')
        return states[match]

    def set_state(self, state):
        cmd = 'state ' + state
        self.send_command(cmd)
        self._expect('Done')

    def get_timeout(self):
        self.send_command('childtimeout')
        i = self._expect('(\d+)\r?\n')
        if i == 0:
            timeout = self.pexpect.match.groups()[0]
        self._expect('Done')
        return timeout

    def set_timeout(self, timeout):
        cmd = 'childtimeout %d' % timeout
        self.send_command(cmd)
        self._expect('Done')

    def set_max_children(self, number):
        cmd = 'childmax %d' % number
        self.send_command(cmd)
        self._expect('Done')

    def get_weight(self):
        self.send_command('leaderweight')
        i = self._expect('(\d+)\r?\n')
        if i == 0:
            weight = self.pexpect.match.groups()[0]
        self._expect('Done')
        return weight

    def set_weight(self, weight):
        cmd = 'leaderweight %d' % weight
        self.send_command(cmd)
        self._expect('Done')

    def add_ipaddr(self, ipaddr):
        cmd = 'ipaddr add ' + ipaddr
        self.send_command(cmd)
        self._expect('Done')

    def get_addrs(self):
        addrs = []
        self.send_command('ipaddr')

        while True:
            i = self._expect(['(\S+(:\S*)+)\r?\n', 'Done'])
            if i == 0:
                addrs.append(self.pexpect.match.groups()[0].decode("utf-8"))
            elif i == 1:
                break

        return addrs

    def get_addr(self, prefix):
        network = ipaddress.ip_network(u'%s' % str(prefix))
        addrs = self.get_addrs()

        for addr in addrs:
            if isinstance(addr, bytearray):
                addr = bytes(addr)
            elif isinstance(addr, str) and sys.version_info[0] == 2:
                addr = addr.decode("utf-8")
            ipv6_address = ipaddress.ip_address(addr)
            if ipv6_address in network:
                return ipv6_address.exploded

        return None

    def get_eidcaches(self):
        eidcaches = []
        self.send_command('eidcache')

        while True:
            i = self._expect(['([a-fA-F0-9\:]+) ([a-fA-F0-9]+)\r?\n', 'Done'])
            if i == 0:
                eid = self.pexpect.match.groups()[0].decode("utf-8")
                rloc = self.pexpect.match.groups()[1].decode("utf-8")
                eidcaches.append((eid, rloc))
            elif i == 1:
                break

        return eidcaches

    def add_service(self, enterpriseNumber, serviceData, serverData):
        cmd = 'service add ' + enterpriseNumber + ' ' + serviceData+ ' '  + serverData
        self.send_command(cmd)
        self._expect('Done')

    def remove_service(self, enterpriseNumber, serviceData):
        cmd = 'service remove ' + enterpriseNumber + ' ' + serviceData
        self.send_command(cmd)
        self._expect('Done')

    def __getLinkLocalAddress(self):
        for ip6Addr in self.get_addrs():
            if re.match(config.LINK_LOCAL_REGEX_PATTERN, ip6Addr, re.I):
                return ip6Addr

        return None

    def __getGlobalAddress(self):
        global_address = []
        for ip6Addr in self.get_addrs():
            if (not re.match(config.LINK_LOCAL_REGEX_PATTERN, ip6Addr, re.I)) and \
                    (not re.match(config.MESH_LOCAL_PREFIX_REGEX_PATTERN, ip6Addr, re.I)) and \
                    (not re.match(config.ROUTING_LOCATOR_REGEX_PATTERN, ip6Addr, re.I)):
                global_address.append(ip6Addr)

        return global_address

    def __getRloc(self):
        for ip6Addr in self.get_addrs():
            if re.match(config.MESH_LOCAL_PREFIX_REGEX_PATTERN, ip6Addr, re.I) and \
                    re.match(config.ROUTING_LOCATOR_REGEX_PATTERN, ip6Addr, re.I) and \
                    not(re.match(config.ALOC_FLAG_REGEX_PATTERN, ip6Addr, re.I)):
                return ip6Addr
        return None

    def __getAloc(self):
        aloc = []
        for ip6Addr in self.get_addrs():
            if re.match(config.MESH_LOCAL_PREFIX_REGEX_PATTERN, ip6Addr, re.I) and \
                    re.match(config.ROUTING_LOCATOR_REGEX_PATTERN, ip6Addr, re.I) and \
                    re.match(config.ALOC_FLAG_REGEX_PATTERN, ip6Addr, re.I):
                aloc.append(ip6Addr)

        return aloc

    def __getMleid(self):
        for ip6Addr in self.get_addrs():
            if re.match(config.MESH_LOCAL_PREFIX_REGEX_PATTERN, ip6Addr, re.I) and \
                    not(re.match(config.ROUTING_LOCATOR_REGEX_PATTERN, ip6Addr, re.I)):
                return ip6Addr

        return None

    def get_ip6_address(self, address_type):
        """Get specific type of IPv6 address configured on thread device.

        Args:
            address_type: the config.ADDRESS_TYPE type of IPv6 address.

        Returns:
            IPv6 address string.
        """
        if address_type == config.ADDRESS_TYPE.LINK_LOCAL:
            return self.__getLinkLocalAddress()
        elif address_type == config.ADDRESS_TYPE.GLOBAL:
            return self.__getGlobalAddress()
        elif address_type == config.ADDRESS_TYPE.RLOC:
            return self.__getRloc()
        elif address_type == config.ADDRESS_TYPE.ALOC:
            return self.__getAloc()
        elif address_type == config.ADDRESS_TYPE.ML_EID:
            return self.__getMleid()
        else:
            return None

        return None

    def get_context_reuse_delay(self):
        self.send_command('contextreusedelay')
        i = self._expect('(\d+)\r?\n')
        if i == 0:
            timeout = self.pexpect.match.groups()[0]
        self._expect('Done')
        return timeout

    def set_context_reuse_delay(self, delay):
        cmd = 'contextreusedelay %d' % delay
        self.send_command(cmd)
        self._expect('Done')

    def add_prefix(self, prefix, flags, prf = 'med'):
        cmd = 'prefix add ' + prefix + ' ' + flags + ' ' + prf
        self.send_command(cmd)
        self._expect('Done')

    def remove_prefix(self, prefix):
        cmd = 'prefix remove ' + prefix
        self.send_command(cmd)
        self._expect('Done')

    def add_route(self, prefix, prf = 'med'):
        cmd = 'route add ' + prefix + ' ' + prf
        self.send_command(cmd)
        self._expect('Done')

    def remove_route(self, prefix):
        cmd = 'route remove ' + prefix
        self.send_command(cmd)
        self._expect('Done')

    def register_netdata(self):
        self.send_command('netdataregister')
        self._expect('Done')

    def energy_scan(self, mask, count, period, scan_duration, ipaddr):
        cmd = 'commissioner energy ' + str(mask) + ' ' + str(count) + ' ' + str(period) + ' ' + str(scan_duration) + ' ' + ipaddr
        self.send_command(cmd)

        if isinstance(self.simulator, simulator.VirtualTime):
            self.simulator.go(8)
            timeout = 1
        else:
            timeout = 8

        self._expect('Energy:', timeout=timeout)

    def panid_query(self, panid, mask, ipaddr):
        cmd = 'commissioner panid ' + str(panid) + ' ' + str(mask) + ' ' + ipaddr
        self.send_command(cmd)

        if isinstance(self.simulator, simulator.VirtualTime):
            self.simulator.go(8)
            timeout = 1
        else:
            timeout = 8

        self._expect('Conflict:', timeout=timeout)

    def scan(self):
        self.send_command('scan')

        results = []
        while True:
            i = self._expect(['\|\s(\S+)\s+\|\s(\S+)\s+\|\s([0-9a-fA-F]{4})\s\|\s([0-9a-fA-F]{16})\s\|\s(\d+)\r?\n',
                                        'Done'])
            if i == 0:
                results.append(self.pexpect.match.groups())
            else:
                break

        return results

    def ping(self, ipaddr, num_responses=1, size=None, timeout=5):
        cmd = 'ping ' + ipaddr
        if size != None:
            cmd += ' ' + str(size)

        self.send_command(cmd)

        if isinstance(self.simulator, simulator.VirtualTime):
            self.simulator.go(timeout)

        result = True
        try:
            responders = {}
            while len(responders) < num_responses:
                i = self._expect(['from (\S+):'])
                if i == 0:
                    responders[self.pexpect.match.groups()[0]] = 1
            self._expect('\n')
        except (pexpect.TIMEOUT, socket.timeout):
            result = False
            if isinstance(self.simulator, simulator.VirtualTime):
                self.simulator.sync_devices()

        return result

    def reset(self):
        self.send_command('reset')
        time.sleep(0.1)

    def set_router_selection_jitter(self, jitter):
        cmd = 'routerselectionjitter %d' % jitter
        self.send_command(cmd)
        self._expect('Done')

    def set_active_dataset(self, timestamp, panid=None, channel=None, channel_mask=None, master_key=None):
        self.send_command('dataset clear')
        self._expect('Done')

        cmd = 'dataset activetimestamp %d' % timestamp
        self.send_command(cmd)
        self._expect('Done')

        if panid != None:
            cmd = 'dataset panid %d' % panid
            self.send_command(cmd)
            self._expect('Done')

        if channel != None:
            cmd = 'dataset channel %d' % channel
            self.send_command(cmd)
            self._expect('Done')

        if channel_mask != None:
            cmd = 'dataset channelmask %d' % channel_mask
            self.send_command(cmd)
            self._expect('Done')

        if master_key != None:
            cmd = 'dataset masterkey ' + master_key
            self.send_command(cmd)
            self._expect('Done')

        self.send_command('dataset commit active')
        self._expect('Done')

    def set_pending_dataset(self, pendingtimestamp, activetimestamp, panid=None, channel=None):
        self.send_command('dataset clear')
        self._expect('Done')

        cmd = 'dataset pendingtimestamp %d' % pendingtimestamp
        self.send_command(cmd)
        self._expect('Done')

        cmd = 'dataset activetimestamp %d' % activetimestamp
        self.send_command(cmd)
        self._expect('Done')

        if panid != None:
            cmd = 'dataset panid %d' % panid
            self.send_command(cmd)
            self._expect('Done')

        if channel != None:
            cmd = 'dataset channel %d' % channel
            self.send_command(cmd)
            self._expect('Done')

        self.send_command('dataset commit pending')
        self._expect('Done')

    def announce_begin(self, mask, count, period, ipaddr):
        cmd = 'commissioner announce ' + str(mask) + ' ' + str(count) + ' ' + str(period) + ' ' + ipaddr
        self.send_command(cmd)
        self._expect('Done')

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
        self._expect('Done')

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
        self._expect('Done')

    def coaps_start_psk(self, psk, pskIdentity):
        cmd = 'coaps set psk ' + psk + ' ' + pskIdentity
        self.send_command(cmd)
        self._expect('Done')

        cmd = 'coaps start'
        self.send_command(cmd)
        self._expect('Done')

    def coaps_start_x509(self):
        cmd = 'coaps set x509'
        self.send_command(cmd)
        self._expect('Done')

        cmd = 'coaps start'
        self.send_command(cmd)
        self._expect('Done')

    def coaps_set_resource_path(self, path):
        cmd = 'coaps resource ' + path
        self.send_command(cmd)
        self._expect('Done')

    def coaps_stop(self):
        cmd = 'coaps stop'
        self.send_command(cmd)

        if isinstance(self.simulator, simulator.VirtualTime):
            self.simulator.go(5)
            timeout = 1
        else:
            timeout = 5

        self._expect('Done', timeout=timeout)

    def coaps_connect(self, ipaddr):
        cmd = 'coaps connect ' + ipaddr
        self.send_command(cmd)

        if isinstance(self.simulator, simulator.VirtualTime):
            self.simulator.go(5)
            timeout = 1
        else:
            timeout = 5

        self._expect('CoAP Secure connected!', timeout=timeout)

    def coaps_disconnect(self):
        cmd = 'coaps disconnect'
        self.send_command(cmd)
        self._expect('Done')
        self.simulator.go(5)

    def coaps_get(self):
        cmd = 'coaps get test'
        self.send_command(cmd)

        if isinstance(self.simulator, simulator.VirtualTime):
            self.simulator.go(5)
            timeout = 1
        else:
            timeout = 5

        self._expect('Received coap secure response', timeout=timeout)
