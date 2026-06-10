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
import logging
import re
import sys
import time
import ipaddress

import serial
from IThci import IThci
from THCI.OpenThread import OpenThreadTHCI, watched, API

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

logging.getLogger('paramiko').setLevel(logging.WARNING)


class SSHHandle(object):
    # Unit: second
    KEEPALIVE_INTERVAL = 30

    def __init__(self, ip, port, username, password):
        self.ip = ip
        self.port = int(port)
        self.username = username
        self.password = password
        self.__handle = None

        self.__connect()

    def __connect(self):
        import paramiko

        self.close()

        self.__handle = paramiko.SSHClient()
        self.__handle.set_missing_host_key_policy(paramiko.AutoAddPolicy())
        try:
            self.__handle.connect(self.ip, port=self.port, username=self.username, password=self.password)
        except paramiko.ssh_exception.AuthenticationException:
            if not self.password:
                self.__handle.get_transport().auth_none(self.username)
            else:
                raise

        # Avoid SSH disconnection after idle for a long time
        self.__handle.get_transport().set_keepalive(self.KEEPALIVE_INTERVAL)

    def close(self):
        if self.__handle is not None:
            self.__handle.close()
            self.__handle = None

    def bash(self, cmd, timeout):
        from paramiko import SSHException
        retry = 3
        for i in range(retry):
            try:
                stdin, stdout, stderr = self.__handle.exec_command(cmd, timeout=timeout)

                sys.stderr.write(stderr.read())
                output = [r.encode('utf8').rstrip('\r\n') for r in stdout.readlines()]
                return output

            except Exception:
                if i < retry - 1:
                    print('SSH connection is lost, try reconnect after 1 second.')
                    time.sleep(1)
                    self.__connect()
                else:
                    raise

    def log(self, fmt, *args):
        try:
            msg = fmt % args
            print('%s - %s - %s' % (self.port, time.strftime('%b %d %H:%M:%S'), msg))
        except Exception:
            pass


class SerialHandle:

    def __init__(self, port, baudrate):
        self.port = port
        self.__handle = serial.Serial(port, baudrate, timeout=0)

        self.__lines = ['']
        assert len(self.__lines) >= 1, self.__lines

        self.log("inputing username ...")
        self.__bashWriteLine('pi')
        deadline = time.time() + 20
        loginOk = False
        while time.time() < deadline:
            time.sleep(1)

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

    def log(self, fmt, *args):
        try:
            msg = fmt % args
            print('%s - %s - %s' % (self.port, time.strftime('%b %d %H:%M:%S'), msg))
        except Exception:
            pass

    def close(self):
        self.__handle.close()

    def bash(self, cmd, timeout=10):
        """
        Execute the command in bash.
        """
        self.__bashClearLines()
        self.__bashWriteLine(cmd)
        self.__bashExpect(cmd, timeout=timeout, endswith=True)

        response = []

        deadline = time.time() + timeout
        while time.time() < deadline:
            line = self.__bashReadLine()
            if line is None:
                time.sleep(0.01)
                continue

            if line == RPI_FULL_PROMPT:
                # return response lines without prompt
                return response

            response.append(line)

        self.__bashWrite('\x03')
        raise Exception('%s: failed to find end of response' % self.port)

    def __bashExpect(self, expected, timeout=20, endswith=False):
        self.log('Expecting [%r]' % (expected))

        deadline = time.time() + timeout
        while time.time() < deadline:
            line = self.__bashReadLine()
            if line is None:
                time.sleep(0.01)
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
            piece = self.__handle.read()
            data = data + piece.decode('utf8')
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


class OpenThread_BR(OpenThreadTHCI, IThci):
    DEFAULT_COMMAND_TIMEOUT = 20

    IsBorderRouter = True
    __is_root = False

    def _getHandle(self):
        if self.connectType == 'ip':
            return SSHHandle(self.telnetIp, self.telnetPort, self.telnetUsername, self.telnetPassword)
        else:
            return SerialHandle(self.port, 115200)

    def _connect(self):
        self.log("logging in to Raspberry Pi ...")
        self.__cli_output_lines = []
        self.__syslog_skip_lines = None
        self.__syslog_last_read_ts = 0

        self.__handle = self._getHandle()
        if self.connectType == 'ip':
            self.__is_root = self.telnetUsername == 'root'

    def _disconnect(self):
        if self.__handle:
            self.__handle.close()
            self.__handle = None

    def _deviceBeforeReset(self):
        if self.isPowerDown:
            self.log('Powering up the device')
            self.powerUp()
        if self.IsHost:
            self.__stopRadvdService()
            self.bash('ip -6 addr del 910b::1 dev %s || true' % self.backboneNetif)
            self.bash('ip -6 addr del fd00:7d03:7d03:7d03::1 dev %s || true' % self.backboneNetif)

        self.stopListeningToAddrAll()

    def _deviceAfterReset(self):
        self.__dumpSyslog()
        self.__truncateSyslog()
        self.__enableAcceptRa()
        if not self.IsHost:
            self._restartAgentService()
            time.sleep(2)

    def __enableAcceptRa(self):
        self.bash('sysctl net.ipv6.conf.%s.accept_ra=2' % self.backboneNetif)

    def _beforeRegisterMulticast(self, sAddr='ff04::1234:777a:1', timeout=300):
        """subscribe to the given ipv6 address (sAddr) in interface and send MLR.req OTA

        Args:
            sAddr   : str : Multicast address to be subscribed and notified OTA.
        """

        if self.externalCommissioner is not None:
            self.externalCommissioner.MLR([sAddr], timeout)
            return True

        cmd = 'nohup ~/repo/openthread/tests/scripts/thread-cert/mcast6.py wpan0 %s' % sAddr
        cmd = cmd + ' > /dev/null 2>&1 &'
        self.bash(cmd)

    @API
    def setupHost(self, setDp=False, setDua=False):
        self.IsHost = True

        self.bash('ip -6 addr add 910b::1 dev %s' % self.backboneNetif)

        if setDua:
            self.bash('ip -6 addr add fd00:7d03:7d03:7d03::1 dev %s' % self.backboneNetif)

        self.__startRadvdService(setDp)

    def _deviceEscapeEscapable(self, string):
        """Escape CLI escapable characters in the given string.

        Args:
            string (str): UTF-8 input string.

        Returns:
            [str]: The modified string with escaped characters.
        """
        return '"' + string + '"'

    @watched
    def bash(self, cmd, timeout=DEFAULT_COMMAND_TIMEOUT, sudo=True):
        return self.bash_unwatched(cmd, timeout=timeout, sudo=sudo)

    def bash_unwatched(self, cmd, timeout=DEFAULT_COMMAND_TIMEOUT, sudo=True):
        if sudo and not self.__is_root:
            cmd = 'sudo ' + cmd

        return self.__handle.bash(cmd, timeout=timeout)

    # Override send_udp
    @API
    def send_udp(self, interface, dst, port, payload):
        if interface == 0:  # Thread Interface
            super(OpenThread_BR, self).send_udp(interface, dst, port, payload)
            return

        if interface == 1:
            ifname = self.backboneNetif
        else:
            raise AssertionError('Invalid interface set to send UDP: {} '
                                 'Available interface options: 0 - Thread; 1 - Ethernet'.format(interface))
        cmd = '/home/pi/reference-device/send_udp.py %s %s %s %s' % (ifname, dst, port, payload)
        self.bash(cmd)

    @API
    def mldv2_query(self):
        ifname = self.backboneNetif
        dst = 'ff02::1'

        cmd = '/home/pi/reference-device/send_mld_query.py %s %s' % (ifname, dst)
        self.bash(cmd)

    @API
    def ip_neighbors_flush(self):
        # clear neigh cache on linux
        cmd1 = 'sudo ip -6 neigh flush nud all nud failed nud noarp dev %s' % self.backboneNetif
        cmd2 = ('sudo ip -6 neigh list nud all dev %s '
                '| cut -d " " -f1 '
                '| sudo xargs -I{} ip -6 neigh delete {} dev %s') % (self.backboneNetif, self.backboneNetif)
        cmd = '%s ; %s' % (cmd1, cmd2)
        self.bash(cmd, sudo=False)

    @API
    def ip_neighbors_add(self, addr, lladdr, nud='noarp'):
        cmd1 = 'sudo ip -6 neigh delete %s dev %s' % (addr, self.backboneNetif)
        cmd2 = 'sudo ip -6 neigh add %s dev %s lladdr %s nud %s' % (addr, self.backboneNetif, lladdr, nud)
        cmd = '%s ; %s' % (cmd1, cmd2)
        self.bash(cmd, sudo=False)

    @API
    def get_eth_ll(self):
        cmd = "ip -6 addr list dev %s | grep 'inet6 fe80' | awk '{print $2}'" % self.backboneNetif
        ret = self.bash(cmd)[0].split('/')[0]
        return ret

    @API
    def ping(self, strDestination, ilength=0, hop_limit=5, timeout=5):
        """ send ICMPv6 echo request with a given length to a unicast destination
            address

        Args:
            strDestination: the unicast destination address of ICMPv6 echo request
            ilength: the size of ICMPv6 echo request payload
            hop_limit: the hop limit
            timeout: time before ping() stops
        """
        if hop_limit is None:
            hop_limit = 5

        if self.IsHost or self.IsBorderRouter:
            ifName = self.backboneNetif
        else:
            ifName = 'wpan0'

        cmd = 'ping -6 -I %s %s -c 1 -s %d -W %d -t %d' % (
            ifName,
            strDestination,
            int(ilength),
            int(timeout),
            int(hop_limit),
        )

        self.bash(cmd, sudo=False)
        time.sleep(timeout)

    def multicast_Ping(self, destination, length=20):
        """send ICMPv6 echo request with a given length to a multicast destination
           address

        Args:
            destination: the multicast destination address of ICMPv6 echo request
            length: the size of ICMPv6 echo request payload
        """
        hop_limit = 5

        if self.IsHost or self.IsBorderRouter:
            ifName = self.backboneNetif
        else:
            ifName = 'wpan0'

        cmd = 'ping -6 -I %s %s -c 1 -s %d -t %d' % (ifName, destination, str(length), hop_limit)

        self.bash(cmd, sudo=False)

    @API
    def getGUA(self, filterByPrefix=None, eth=False):
        """get expected global unicast IPv6 address of Thread device

        note: existing filterByPrefix are string of in lowercase. e.g.
        '2001' or '2001:0db8:0001:0000".

        Args:
            filterByPrefix: a given expected global IPv6 prefix to be matched

        Returns:
            a global IPv6 address
        """
        # get global addrs set if multiple
        if eth:
            return self.__getEthGUA(filterByPrefix=filterByPrefix)
        else:
            return super(OpenThread_BR, self).getGUA(filterByPrefix=filterByPrefix)

    def __getEthGUA(self, filterByPrefix=None):
        globalAddrs = []

        cmd = 'ip -6 addr list dev %s | grep inet6' % self.backboneNetif
        output = self.bash(cmd, sudo=False)
        for line in output:
            # example: inet6 2401:fa00:41:23:274a:1329:3ab9:d953/64 scope global dynamic noprefixroute
            line = line.strip().split()

            if len(line) < 4 or line[2] != 'scope':
                continue

            if line[3] != 'global':
                continue

            addr = line[1].split('/')[0]
            addr = str(ipaddress.IPv6Address(addr.decode()).exploded)
            globalAddrs.append(addr)

        if not filterByPrefix:
            return globalAddrs[0]
        else:
            if filterByPrefix[-2:] != '::':
                filterByPrefix = '%s::' % filterByPrefix
            prefix = ipaddress.IPv6Network((filterByPrefix + '/64').decode())
            for fullIp in globalAddrs:
                address = ipaddress.IPv6Address(fullIp.decode())
                if address in prefix:
                    return fullIp

    def _cliReadLine(self):
        # read commissioning log if it's commissioning
        if not self.__cli_output_lines:
            self.__readSyslogToCli()

        if self.__cli_output_lines:
            return self.__cli_output_lines.pop(0)

        return None

    @watched
    def _deviceGetEtherMac(self):
        # Harness wants it in string. Because wireshark filter for eth
        # cannot be applies in hex
        return self.bash('ip addr list dev %s | grep ether' % self.backboneNetif, sudo=False)[0].strip().split()[1]

    @watched
    def _onCommissionStart(self):
        assert self.__syslog_skip_lines is None
        self.__syslog_skip_lines = int(self.bash('wc -l /var/log/syslog', sudo=False)[0].split()[0])
        self.__syslog_last_read_ts = 0

    @watched
    def _onCommissionStop(self):
        assert self.__syslog_skip_lines is not None
        self.__syslog_skip_lines = None

    @watched
    def __startRadvdService(self, setDp=False):
        assert self.IsHost, "radvd service runs on Host only"

        conf = "EOF"
        conf += "\ninterface %s" % self.backboneNetif
        conf += "\n{"
        conf += "\n    AdvSendAdvert on;"
        conf += "\n"
        conf += "\n    MinRtrAdvInterval 3;"
        conf += "\n    MaxRtrAdvInterval 30;"
        conf += "\n    AdvDefaultPreference low;"
        conf += "\n"
        conf += "\n    prefix 910b::/64"
        conf += "\n    {"
        conf += "\n        AdvOnLink on;"
        conf += "\n        AdvAutonomous on;"
        conf += "\n        AdvRouterAddr on;"
        conf += "\n    };"
        if setDp:
            conf += "\n"
            conf += "\n    prefix fd00:7d03:7d03:7d03::/64"
            conf += "\n    {"
            conf += "\n        AdvOnLink on;"
            conf += "\n        AdvAutonomous off;"
            conf += "\n        AdvRouterAddr off;"
            conf += "\n    };"
        conf += "\n};"
        conf += "\nEOF"
        cmd = 'sh -c "cat >/etc/radvd.conf <<%s"' % conf

        self.bash(cmd)
        self.bash(self.extraParams.get('cmd-restart-radvd', 'service radvd restart'))
        self.bash('service radvd status')

    @watched
    def __stopRadvdService(self):
        assert self.IsHost, "radvd service runs on Host only"
        self.bash('service radvd stop')

    def __readSyslogToCli(self):
        if self.__syslog_skip_lines is None:
            return 0

        # read syslog once per second
        if time.time() < self.__syslog_last_read_ts + 1:
            return 0

        self.__syslog_last_read_ts = time.time()

        lines = self.bash_unwatched('tail +%d /var/log/syslog' % self.__syslog_skip_lines, sudo=False)
        for line in lines:
            m = OTBR_AGENT_SYSLOG_PATTERN.search(line)
            if not m:
                continue

            self.__cli_output_lines.append(m.group(1))

        self.__syslog_skip_lines += len(lines)
        return len(lines)

    def _cliWriteLine(self, line):
        cmd = 'ot-ctl -- %s' % line
        output = self.bash(cmd)
        # fake the line echo back
        self.__cli_output_lines.append(line)
        for line in output:
            self.__cli_output_lines.append(line)

    def _restartAgentService(self):
        restart_cmd = self.extraParams.get('cmd-restart-otbr-agent', 'systemctl restart otbr-agent')
        self.bash(restart_cmd)

    def __truncateSyslog(self):
        self.bash('truncate -s 0 /var/log/syslog')

    def __dumpSyslog(self):
        cmd = self.extraParams.get('cmd-dump-otbr-log', 'grep "otbr-agent" /var/log/syslog')
        output = self.bash_unwatched(cmd)
        for line in output:
            self.log('%s', line)

    @API
    def get_eth_addrs(self):
        cmd = "ip -6 addr list dev %s | grep 'inet6 ' | awk '{print $2}'" % self.backboneNetif
        addrs = self.bash(cmd)
        return [addr.split('/')[0] for addr in addrs]

    @API
    def mdns_query(self, service='_meshcop._udp.local', addrs_allowlist=(), addrs_denylist=()):
        try:
            for deny_addr in addrs_denylist:
                self.bash('ip6tables -A INPUT -p udp --dport 5353 -s %s -j DROP' % deny_addr)

            if addrs_allowlist:
                for allow_addr in addrs_allowlist:
                    self.bash('ip6tables -A INPUT -p udp --dport 5353 -s %s -j ACCEPT' % allow_addr)

                self.bash('ip6tables -A INPUT -p udp --dport 5353 -j DROP')

            return self._mdns_query_impl(service, find_active=(addrs_allowlist or addrs_denylist))

        finally:
            self.bash('ip6tables -F INPUT')
            time.sleep(1)

    def _mdns_query_impl(self, service, find_active):
        # For BBR-TC-03 or DH test cases (empty arguments) just send a query
        output = self.bash('python3 ~/repo/openthread/tests/scripts/thread-cert/find_border_agents.py')

        if not find_active:
            return

        # For MATN-TC-17 and MATN-TC-18 use Zeroconf to get the BBR address and border agent port
        for line in output:
            print(line)
            alias, addr, port, thread_status = eval(line)
            if thread_status == 2 and addr:
                if ipaddress.IPv6Address(addr.decode()).is_link_local:
                    addr = '%s%%%s' % (addr, self.backboneNetif)
                return addr, port

        raise Exception('No active Border Agents found')

    # Override powerDown
    @API
    def powerDown(self):
        self.log('Powering down BBR')
        super(OpenThread_BR, self).powerDown()
        stop_cmd = self.extraParams.get('cmd-stop-otbr-agent', 'systemctl stop otbr-agent')
        self.bash(stop_cmd)

    # Override powerUp
    @API
    def powerUp(self):
        self.log('Powering up BBR')
        start_cmd = self.extraParams.get('cmd-start-otbr-agent', 'systemctl start otbr-agent')
        self.bash(start_cmd)
        super(OpenThread_BR, self).powerUp()

    # Override forceSetSlaac
    @API
    def forceSetSlaac(self, slaacAddress):
        self.bash('ip -6 addr add %s/64 dev wpan0' % slaacAddress)

    # Override stopListeningToAddr
    @API
    def stopListeningToAddr(self, sAddr):
        """
        Unsubscribe to a given IPv6 address which was subscribed earlier with `registerMulticast`.

        Args:
            sAddr   : str : Multicast address to be unsubscribed. Use an empty string to unsubscribe
                            all the active multicast addresses.
        """
        cmd = 'pkill -f mcast6.*%s' % sAddr
        self.bash(cmd)

    def stopListeningToAddrAll(self):
        return self.stopListeningToAddr('')

    @API
    def deregisterMulticast(self, sAddr):
        """
        Unsubscribe to a given IPv6 address.
        Only used by External Commissioner.

        Args:
            sAddr   : str : Multicast address to be unsubscribed.
        """
        self.externalCommissioner.MLR([sAddr], 0)
        return True

    @watched
    def _waitBorderRoutingStabilize(self):
        """
        Wait for Network Data to stabilize if BORDER_ROUTING is enabled.
        """
        if not self.isBorderRoutingEnabled():
            return

        MAX_TIMEOUT = 30
        MIN_TIMEOUT = 15
        CHECK_INTERVAL = 3

        time.sleep(MIN_TIMEOUT)

        lastNetData = self.getNetworkData()
        for i in range((MAX_TIMEOUT - MIN_TIMEOUT) // CHECK_INTERVAL):
            time.sleep(CHECK_INTERVAL)
            curNetData = self.getNetworkData()

            # Wait until the Network Data is not changing, and there is OMR Prefix and External Routes available
            if curNetData == lastNetData and len(curNetData['Prefixes']) > 0 and len(curNetData['Routes']) > 0:
                break

            lastNetData = curNetData

        return lastNetData
