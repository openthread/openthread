#!/usr/bin/env python
#
# Copyright (c) 2018, The OpenThread Authors.
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
>> Device : OpenThread_WpanCtl THCI
>> Class : OpenThread_WpanCtl
"""

import re
import time
import socket
import logging
from Queue import Queue

import serial
from IThci import IThci
from GRLLibs.UtilityModules.Test import Thread_Device_Role, Device_Data_Requirement, MacType
from GRLLibs.UtilityModules.enums import PlatformDiagnosticPacket_Direction, PlatformDiagnosticPacket_Type
from GRLLibs.UtilityModules.ModuleHelper import ModuleHelper, ThreadRunner
from GRLLibs.ThreadPacket.PlatformPackets import PlatformDiagnosticPacket, PlatformPackets
from GRLLibs.UtilityModules.Plugins.AES_CMAC import Thread_PBKDF2

"""wpanctl carrier info and wpanctl command prefix"""
WPAN_CARRIER_USER = 'pi'
WPAN_CARRIER_PASSWD = 'raspberry'
WPAN_CARRIER_PROMPT = 'pi@raspberrypi'
WPAN_INTERFACE = 'wpan0'
WPANCTL_CMD = 'sudo wpanctl -I ' + WPAN_INTERFACE + ' '

"""regex: used to split lines"""
LINESEPX = re.compile(r'\r\n|\n')

class OpenThread_WpanCtl(IThci):
    LOWEST_POSSIBLE_PARTATION_ID = 0x1
    LINK_QUALITY_CHANGE_TIME = 100

    # Used for reference firmware version control for Test Harness.
    # This variable will be updated to match the OpenThread reference firmware officially released.
    firmwarePrefix = "OPENTHREAD/"

    def __init__(self, **kwargs):
        """initialize the serial port and default network parameters
        Args:
            **kwargs: Arbitrary keyword arguments
                      Includes 'EUI' and 'SerialPort'
        """
        try:
            self.UIStatusMsg = ''
            self.mac = kwargs.get('EUI')
            self.handle = None
            self.AutoDUTEnable = False
            self._is_net = False                  # whether device is through ser2net
            self.logStatus = {'stop':'stop', 'running':'running', 'pauseReq':'pauseReq', 'paused':'paused'}
            self.logThreadStatus = self.logStatus['stop']
            self.connectType = (kwargs.get('Param5')).strip().lower() if kwargs.get('Param5') is not None else 'usb'
            if self.connectType == 'ip':
                self.dutIpv4 = kwargs.get('TelnetIP')
                self.dutPort = kwargs.get('TelnetPort')
                self.port = self.dutIpv4 + ':' + self.dutPort
            else:
                self.port = kwargs.get('SerialPort')
            self.intialize()
        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger('initialize() Error: ' + str(e))

    def __del__(self):
        """close the serial port connection"""
        try:
            self.closeConnection()
            self.deviceConnected = False
        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger('delete() Error: ' + str(e))

    def _expect(self, expected, times=50):
        """Find the `expected` line within `times` trials.

        Args:
            expected    str: the expected string
            times       int: number of trials
        """
        print '[%s] Expecting [%s]' % (self.port, expected)

        retry_times = 10
        while times > 0 and retry_times > 0:
            line = self._readline()
            print '[%s] Got line [%s]' % (self.port, line)

            if line == expected:
                print '[%s] Expected [%s]' % (self.port, expected)
                return

            if not line:
                retry_times -= 1
                time.sleep(0.1)

            times -= 1

        raise Exception('failed to find expected string[%s]' % expected)

    def _read(self, size=512):
        logging.info('%s: reading', self.port)
        if self._is_net:
            return self.handle.recv(size)
        else:
            return self.handle.read(size)

    def _write(self, data):
        logging.info('%s: writing', self.port)
        if self._is_net:
            self.handle.sendall(data)
        else:
            self.handle.write(data)

    def _readline(self):
        """Read exactly one line from the device

        Returns:
            None on no data
        """
        logging.info('%s: reading line', self.port)
        if len(self._lines) > 1:
            return self._lines.pop(0)

        tail = ''
        if len(self._lines):
            tail = self._lines.pop()

        try:
            tail += self._read()
        except socket.error:
            logging.exception('%s: No new data', self.port)
            time.sleep(0.1)

        self._lines += LINESEPX.split(tail)
        if len(self._lines) > 1:
            return self._lines.pop(0)

    def _sendline(self, line):
        """Send exactly one line to the device

        Args:
            line str: data send to device
        """
        logging.info('%s: sending line', self.port)
        # clear buffer
        self._lines = []
        try:
            self._read()
        except socket.error:
            logging.debug('%s: Nothing cleared', self.port)

        print 'sending [%s]' % line
        self._write(line + '\r\n')
        self._lines = []
        # wait for write to complete
        time.sleep(0.1)

    def __sendCommand(self, cmd):
        """send specific command to reference unit over serial port

        Args:
            cmd: OpenThread_WpanCtl command string

        Returns:
            Fail: Failed to send the command to reference unit and parse it
            Value: successfully retrieve the desired value from reference unit
            Error: some errors occur, indicates by the followed specific error number
        """
        logging.info('%s: sendCommand[%s]', self.port, cmd)
        if self.logThreadStatus == self.logStatus['running']:
            self.logThreadStatus = self.logStatus['pauseReq']
            while self.logThreadStatus != self.logStatus['paused'] and self.logThreadStatus != self.logStatus['stop']:
                pass

        ssh_stdin = None
        ssh_stdout = None
        ssh_stderr = None
        try:
            # command retransmit times
            retry_times = 3
            while retry_times > 0:
                retry_times -= 1
                try:
                    if self._is_net:
                        ssh_stdin, ssh_stdout, ssh_stderr = self.handle.exec_command(cmd)
                    else:
                        self._sendline(cmd)
                        self._expect(cmd)
                except Exception as e:
                    logging.exception('%s: failed to send command[%s]: %s', self.port, cmd, str(e))
                    if retry_times == 0:
                        raise
                else:
                    break

            line = None
            response = []
            retry_times = 20
            stdout_lines = []
            stderr_lines = []
            if self._is_net:
                stdout_lines = ssh_stdout.readlines()
                stderr_lines = ssh_stderr.readlines()
                if stderr_lines:
                    for stderr_line in stderr_lines:
                        if re.search(r'Not\s+Found|failed\s+with\s+error', stderr_line.strip(), re.M | re.I):
                            print "Command failed:" + stderr_line
                            return 'Fail'
                        print "Got line: " + stderr_line
                        logging.info('%s: the read line is[%s]', self.port, stderr_line)
                        response.append(str(stderr_line.strip()))
                elif stdout_lines:
                    for stdout_line in stdout_lines:
                        logging.info('%s: the read line is[%s]', self.port, stdout_line)
                        if re.search(r'Not\s+Found|failed\s+with\s+error', stdout_line.strip(), re.M | re.I):
                            print "Command failed"
                            return 'Fail'
                        print "Got line: " + stdout_line
                        logging.info('%s: send command[%s] done!', self.port, cmd)
                        response.append(str(stdout_line.strip()))
                response.append(WPAN_CARRIER_PROMPT)
                return response
            else:
                while retry_times > 0:
                    line = self._readline()
                    print "read line: %s" % line
                    logging.info('%s: the read line is[%s]', self.port, line)
                    if line:
                        response.append(line)
                        if re.match(WPAN_CARRIER_PROMPT, line):
                            break
                        elif re.search(r'Not\s+Found|failed\s+with\s+error', line, re.M | re.I):
                            print "Command failed"
                            return 'Fail'

                    retry_times -= 1
                    time.sleep(0.1)

                if retry_times == 0:
                    raise Exception('%s: failed to find end of response' % self.port)
                logging.info('%s: send command[%s] done!', self.port, cmd)
                return response
        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger('sendCommand() Error: ' + str(e))
            raise

    def __stripValue(self, value):
        """strip the special characters in the value

                Args:
                    value: value string

                Returns:
                    value string without special characters
        """
        if isinstance(value, str):
            if ( value[0] == '"' and value[-1] == '"' ) or ( value[0] == '[' and value[-1] == ']' ):
                return value[1:-1]
        return value

    def __padIp6Addr(self, ip6Addr):
        segments = ip6Addr.split(':')
        empty = None
        for i, element in enumerate(segments):
            if empty is None and len(element) == 0:
                empty = i
            elif len(element) < 4:
                segments[i] = '0' * (4 - len(element)) + element

        if empty is not None:
            segments = segments[:empty] + ['0000', ] * (8 - len(segments) + 1) + segments[empty + 1:]

        return ':'.join(segments)

    def __getIp6Address(self, addressType):
        """get specific type of IPv6 address configured on OpenThread_WpanCtl

        Args:
            addressType: the specific type of IPv6 address

            link local: link local unicast IPv6 address that's within one-hop scope
            global: global unicast IPv6 address
            rloc: mesh local unicast IPv6 address for routing in thread network
            mesh EID: mesh Endpoint Identifier

        Returns:
            IPv6 address string
        """
        addrType = ['link local', 'global', 'rloc', 'mesh EID']
        addrs = []
        globalAddr = []
        linkLocal64Addr = ''
        rlocAddr = ''
        meshEIDAddr = ''

        addrs = self.__sendCommand(WPANCTL_CMD + 'getprop -v IPv6:AllAddresses')
        for ip6AddrItem in addrs:
            if re.match('\[|\]', ip6AddrItem):
                continue
            if re.match(WPAN_CARRIER_PROMPT, ip6AddrItem, re.M|re.I):
                break
            ip6AddrItem = ip6AddrItem.strip()
            ip6Addr = self.__stripValue(ip6AddrItem).split(' ')[0]
            ip6AddrPrefix = ip6Addr.split(':')[0]
            if ip6AddrPrefix == 'fe80':
                # link local address
                if ip6Addr.split(':')[4] != '0':
                    linkLocal64Addr = ip6Addr
            elif ip6AddrPrefix == 'fd00':
                # mesh local address
                if ip6Addr.split(':')[4] == '0':
                    # rloc
                    rlocAddr = ip6Addr
                else:
                    # mesh EID
                    meshEIDAddr = ip6Addr
                    print 'meshEIDAddr:' + meshEIDAddr
            else:
                # global ipv6 address
                if ip6Addr:
                    print 'globalAddr: ' + ip6Addr
                    globalAddr.append(ip6Addr)
                else:
                    pass

        if addressType == addrType[0]:
            return linkLocal64Addr
        elif addressType == addrType[1]:
            return globalAddr
        elif addressType == addrType[2]:
            return rlocAddr
        elif addressType == addrType[3]:
            return meshEIDAddr
        else:
            pass

    def __setDeviceMode(self, mode):
        """set thread device mode:

        Args:
           mode: thread device mode. 15=rsdn, 13=rsn, 4=s
           r: rx-on-when-idle
           s: secure IEEE 802.15.4 data request
           d: full thread device
           n: full network data

        Returns:
           True: successful to set the device mode
           False: fail to set the device mode
        """
        print 'call __setDeviceMode'

        try:
            cmd = WPANCTL_CMD + 'setprop Thread:DeviceMode %d' % mode
            return self.__sendCommand(cmd)[0] != 'Fail'
        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger('setDeviceMode() Error: ' + str(e))

    def __setRouterUpgradeThreshold(self, iThreshold):
        """set router upgrade threshold

        Args:
            iThreshold: the number of active routers on the Thread network
                        partition below which a REED may decide to become a Router.

        Returns:
            True: successful to set the ROUTER_UPGRADE_THRESHOLD
            False: fail to set ROUTER_UPGRADE_THRESHOLD
        """
        print 'call __setRouterUpgradeThreshold'
        try:
            cmd = WPANCTL_CMD + 'setprop Thread:RouterUpgradeThreshold %s' % str(iThreshold)
            return self.__sendCommand(cmd)[0] != 'Fail'
        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger('setRouterUpgradeThreshold() Error: ' + str(e))

    def __setRouterDowngradeThreshold(self, iThreshold):
        """set router downgrade threshold

        Args:
            iThreshold: the number of active routers on the Thread network
                        partition above which an active router may decide to
                        become a child.

        Returns:
            True: successful to set the ROUTER_DOWNGRADE_THRESHOLD
            False: fail to set ROUTER_DOWNGRADE_THRESHOLD
        """
        print 'call __setRouterDowngradeThreshold'
        try:
            cmd = WPANCTL_CMD + 'setprop Thread:RouterDowngradeThreshold %s' % str(iThreshold)
            return self.__sendCommand(cmd)[0] != 'Fail'
        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger('setRouterDowngradeThreshold() Error: ' + str(e))

    def __setRouterSelectionJitter(self, iRouterJitter):
        """set ROUTER_SELECTION_JITTER parameter for REED to upgrade to Router

        Args:
            iRouterJitter: a random period prior to request Router ID for REED

        Returns:
            True: successful to set the ROUTER_SELECTION_JITTER
            False: fail to set ROUTER_SELECTION_JITTER
        """
        print 'call _setRouterSelectionJitter'
        try:
            cmd = WPANCTL_CMD + 'setprop Thread:RouterSelectionJitter %s' % str(iRouterJitter)
            return self.__sendCommand(cmd) != 'Fail'
        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger('setRouterSelectionJitter() Error: ' + str(e))

    def __setAddressfilterMode(self, mode):
        """set address filter mode

        Returns:
            True: successful to set address filter mode.
            False: fail to set address filter mode.
        """
        print 'call setAddressFilterMode() ' +  mode
        try:
            if re.match('list', mode, re.M|re.I):
                cmd = WPANCTL_CMD + 'setprop MAC:' + mode + ':Enabled 1'
            elif mode == 'disabled':
                cmd = WPANCTL_CMD + 'setprop MAC:' + mode + ':Enabled 0'
            else:
                print 'no such option'
                return False
            if self.__sendCommand(cmd)[0] != 'Fail':
                return True
            return False
        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger('__setAddressFilterMode() Error: ' + str(e))

    def __startOpenThreadWpan(self):
        """start OpenThreadWpan

        Returns:
            True: successful to start OpenThreadWpan up
            False: fail to start OpenThreadWpan
        """
        print 'call startOpenThreadWpan'
        try:

            # restore whitelist/blacklist address filter mode if rejoin after reset
            if self.isPowerDown:
                if self._addressfilterMode == 'whitelist':
                    if self.__setAddressfilterMode('whitelist'):
                        for addr in self._addressfilterSet:
                            self.addAllowMAC(addr)
                elif self._addressfilterMode == 'blacklist':
                    if self.__setAddressfilterMode('blacklist'):
                        for addr in self._addressfilterSet:
                            self.addBlockedMAC(addr)
            time.sleep(1)
            if ModuleHelper.LeaderDutChannelFound:
                self.channel = ModuleHelper.Default_Channel
            nodeType = 'router'
            startType = 'join'
            if self.deviceRole == Thread_Device_Role.Leader:
                nodeType = 'router'
                startType = 'form'
            elif self.deviceRole == Thread_Device_Role.Router:
                nodeType = 'router'
            elif self.deviceRole == Thread_Device_Role.SED:
                nodeType = 'sed'
            elif self.deviceRole == Thread_Device_Role.EndDevice:
                nodeType = 'end'
            elif self.deviceRole == Thread_Device_Role.REED:
                nodeType = 'router'
            elif self.deviceRole == Thread_Device_Role.EndDevice_FED:
                nodeType = 'router'
            elif self.deviceRole == Thread_Device_Role.EndDevice_MED:
                nodeType = 'end'
            else:
                pass

            if self.deviceRole in [Thread_Device_Role.Leader, Thread_Device_Role.Router, Thread_Device_Role.REED]:
                self.__setRouterSelectionJitter(1)

            if startType == 'form':
                startCmd = WPANCTL_CMD + '%s %s -c %s -T %s ' % (startType, self.networkName, str(self.channel), nodeType)
            else:
                startCmd = WPANCTL_CMD + '%s %s -p %s -c %s -T %s ' % (startType, self.networkName, str(hex(self.panId)), str(self.channel), nodeType)
            if self.__sendCommand(startCmd)[0] != 'Fail':
                if self.__isOpenThreadWpanRunning():
                    self.isPowerDown = False
                    if self.hasActiveDatasetToCommit:
                        if self.__sendCommand(WPANCTL_CMD + 'setprop Dataset:Command SetActive')[0] == 'Fail':
                            raise Exception('failed to commit active dataset')
                        else:
                            self.hasActiveDatasetToCommit = False

                    return True
            else:
                return False
        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger('startOpenThreadWpan() Error: ' + str(e))

    def __stopOpenThreadWpan(self):
        """stop OpenThreadWpan

        Returns:
            True: successfully stop OpenThreadWpan
            False: failed to stop OpenThreadWpan
        """
        print 'call stopOpenThreadWpan'
        try:
            if self.__sendCommand(WPANCTL_CMD + 'leave')[0] != 'Fail' and self.__sendCommand(WPANCTL_CMD + 'dataset erase')[0] != 'Fail':
                return True
            else:
                return False
        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger('stopOpenThreadWpan() Error: ' + str(e))

    def __isOpenThreadWpanRunning(self):
        """check whether or not OpenThreadWpan is running

        Returns:
            True: OpenThreadWpan is running
            False: OpenThreadWpan is not running
        """
        print 'call __isOpenThreadWpanRunning'
        if self.__stripValue(self.__sendCommand(WPANCTL_CMD + 'getprop -v NCP:State')[0]) == 'associated':
            print '*****OpenThreadWpan is running'
            return True
        else:
            print '*****Wrong OpenThreadWpan state'
            return False

    # rloc16 might be hex string or integer, need to return actual allocated router id
    def __convertRlocToRouterId(self, xRloc16):
        """mapping Rloc16 to router id

        Args:
            xRloc16: hex rloc16 short address

        Returns:
            actual router id allocated by leader
        """
        routerList = []
        routerList = self.__sendCommand(WPANCTL_CMD + 'getprop -v Thread:RouterTable')
        print routerList
        print xRloc16

        for line in routerList:
            if re.match('\[|\]', line):
                continue
            if re.match(WPAN_CARRIER_PROMPT, line, re.M|re.I):
                break
            router = []
            router = self.__stripValue(line).split(',')

            for item in router:
                if 'RouterId' in item:
                    routerid = item.split(':')[1]
                elif 'RLOC16' in line:
                    rloc16 = line.split(':')[1]
                else:
                    pass

            # process input rloc16
            if isinstance(xRloc16, str):
                rloc16 = '0x' + rloc16
                if rloc16 == xRloc16:
                    return routerid
            elif isinstance(xRloc16, int):
                if int(rloc16, 16) == xRloc16:
                    return routerid
            else:
                pass

        return None

    def __convertIp6PrefixStringToIp6Address(self, strIp6Prefix):
        """convert IPv6 prefix string to IPv6 dotted-quad format
           for example:
           2001000000000000 -> 2001:0000:0000:0000::

        Args:
            strIp6Prefix: IPv6 address string

        Returns:
            IPv6 address dotted-quad format
        """
        prefix1 = strIp6Prefix.rstrip('L')
        prefix2 = prefix1.lstrip("0x")
        hexPrefix = str(prefix2).ljust(16, '0')
        hexIter = iter(hexPrefix)
        finalMac = ':'.join(a + b + c + d for a, b, c, d in zip(hexIter, hexIter, hexIter, hexIter))
        prefix = str(finalMac)
        strIp6Prefix = prefix[:20]
        return strIp6Prefix + '::'

    def __convertLongToString(self, iValue):
        """convert a long hex integer to string
           remove '0x' and 'L' return string

        Args:
            iValue: long integer in hex format

        Returns:
            string of this long integer without "0x" and "L"
        """
        string = ''
        strValue = str(hex(iValue))

        string = strValue.lstrip('0x')
        string = string.rstrip('L')

        return string

    def __convertChannelMask(self, channelsArray):
        """convert channelsArray to bitmask format

        Args:
            channelsArray: channel array (i.e. [21, 22])

        Returns:
            bitmask format corresponding to a given channel array
        """
        maskSet = 0

        for eachChannel in channelsArray:
            mask = 1 << eachChannel
            maskSet = (maskSet | mask)

        return maskSet

    def __ChannelMaskListToStr(self, channelList):
        """convert a channel list to a string

        Args:
            channelList: channel list (i.e. [21, 22, 23])

        Returns:
            a comma separated channel string (i.e. '21, 22, 23')
        """
        chan_mask_range = ''
        if isinstance(channelList, list):
            if len(channelList):
                chan_mask_range = ",".join(str(chan) for chan in channelList)
            else:
                print 'empty list'
        else:
            print 'not a valid channel list:', channelList

        return chan_mask_range

    def __setChannelMask(self, channelMask):
        print 'call _setChannelMask'
        try:
            cmd = WPANCTL_CMD + 'setprop NCP:ChannelMask %s' % channelMask
            datasetCmd = WPANCTL_CMD + 'setprop Dataset:ChannelMaskPage0 %s' % channelMask
            self.hasActiveDatasetToCommit = True
            return self.__sendCommand(cmd)[0] != 'Fail' and self.__sendCommand(datasetCmd)[0] != 'Fail'
        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger('setChannelMask() Error: ' + str(e))

    def __setSecurityPolicy(self, securityPolicySecs, securityPolicyFlags):
        print 'call _setSecurityPolicy'
        try:
            cmd1 = WPANCTL_CMD + 'setprop Dataset:SecPolicy:KeyRotation %s' % str(securityPolicySecs)
            if securityPolicyFlags == 'onrcb':
                cmd2 = WPANCTL_CMD + 'setprop Dataset:SecPolicy:Flags 0xff'
            else:
                print 'unknown policy flag :' + securityPolicyFlags
                return False
            self.hasActiveDatasetToCommit = True
            return self.__sendCommand(cmd1) != 'Fail' and self.__sendCommand(cmd2) != 'Fail'
        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger('setSecurityPolicy() Error: ' + str(e))

    def __setKeySwitchGuardTime(self, iKeySwitchGuardTime):
        """ set the Key switch guard time

        Args:
            iKeySwitchGuardTime: key switch guard time

        Returns:
            True: successful to set key switch guard time
            False: fail to set key switch guard time
        """
        print '%s call setKeySwitchGuardTime' % self.port
        print iKeySwitchGuardTime
        try:
            cmd = WPANCTL_CMD + 'setprop Network:KeySwitchGuardTime %s' % str(iKeySwitchGuardTime)
            if self.__sendCommand(cmd)[0] != 'Fail':
                time.sleep(1)
                return True
            else:
                return False
        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger('setKeySwitchGuardTime() Error; ' + str(e))

    def __getCommissionerSessionId(self):
        """ get the commissioner session id allocated from Leader """
        print '%s call getCommissionerSessionId' % self.port
        return self.__sendCommand(WPANCTL_CMD + 'getprop -v Commissioner:SessionId')[0]

    def __getJoinerState(self):
        """ get joiner state """
        maxDuration = 150  # seconds
        t_end = time.time() + maxDuration
        while time.time() < t_end:
            joinerState = self.__stripValue(self.__sendCommand('sudo wpanctl getprop -v NCP:State')[0])
            if joinerState == 'offline:commissioned':
                return True
            elif joinerState == 'associating:credentials-needed':
                return False
            else:
                time.sleep(5)
                continue
        return False

    def _connect(self):
        if self.connectType == 'usb':
            print 'My port is %s' % self.port
            try:
                self.handle = serial.Serial(self.port, 115200, timeout=0.2)
            except Exception as e:
                ModuleHelper.WriteIntoDebugLogger('open serial error ' + str(e))

            try:
                attempts = 0
                user_prompted = False
                pwd_prompted = False
                while attempts < 10 or pwd_prompted == True:
                    time.sleep(0.5)
                    attempts = attempts + 1
                    print 'attempts...', attempts

                    input_data = self.handle.read(self.handle.inWaiting())

                    if (not input_data):
                        if user_prompted == False:
                            self.handle.write('\n')
                        time.sleep(0.5)
                        continue;

                    if 'login' in input_data:
                        self.handle.write(WPAN_CARRIER_USER + '\n')
                        time.sleep(0.3)
                        print 'user prompted'
                        user_prompted = True

                    elif 'Password' in input_data:
                        print 'password prompted'
                        time.sleep(0.3)
                        self.handle.write(WPAN_CARRIER_PASSWD + '\n')
                        pwd_prompted = True

                    elif WPAN_CARRIER_PROMPT in input_data:
                        print 'login success (serial)'
                        time.sleep(0.3)
                        self.deviceConnected = True
                        self.handle.write('stty cols 256\n')
                        time.sleep(1)
                        break
                if self.deviceConnected == False:
                    raise Exception('login fail')
                else:
                    self._is_net = False
            except Exception as e:
                ModuleHelper.WriteIntoDebugLogger('connect to serial Error: ' + str(e))

        elif self.connectType == 'ip':
            print "My IP: %s Port: %s" % (self.dutIpv4, self.dutPort)
            try:
                import paramiko
                self.handle = paramiko.SSHClient()
                self.handle.set_missing_host_key_policy(paramiko.AutoAddPolicy())
                self.handle.connect(self.dutIpv4, port=self.dutPort, username=WPAN_CARRIER_USER, password=WPAN_CARRIER_PASSWD)
                print 'login success (ssh)'
                self.deviceConnected = True
                self.handle.exec_command('stty cols 256\n')
                time.sleep(1)
                self._is_net = True
            except Exception as e:
                ModuleHelper.WriteIntoDebugLogger('connect to ssh Error: ' + str(e))
        else:
            raise Exception('Unknown port schema')

    def closeConnection(self):
        """close current serial port connection"""
        print '%s call closeConnection' % self.port
        try:
            if self.handle:
                self.handle.close()
                self.handle = None
        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger('closeConnection() Error: ' + str(e))

    def intialize(self):
        """initialize the serial port with baudrate, timeout parameters"""
        print '%s call intialize' % self.port
        try:
            # init serial port
            self.deviceConnected = False
            self._connect()

            if self.deviceConnected:
                self.UIStatusMsg = self.getVersionNumber()
                if self.firmwarePrefix not in self.UIStatusMsg:
                    self.deviceConnected = False
                    self.UIStatusMsg = 'Firmware Not Matching Expecting ' + self.firmwarePrefix + ' Now is ' + self.UIStatusMsg
                    raise Exception('Err: OpenThread device Firmware not matching..')
                self.__sendCommand(WPANCTL_CMD + 'leave')
                self.__sendCommand(WPANCTL_CMD + 'dataset erase')
            else:
                raise Exception('Err: Device not connected ..')

        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger('intialize() Error: ' + str(e))

    def setNetworkName(self, networkName='GRL'):
        """set Thread Network name

        Args:
            networkName: the networkname string to be set

        Returns:
            True: successful to set the Thread Networkname
            False: fail to set the Thread Networkname
        """
        print '%s call setNetworkName' % self.port

        try:
            cmd = WPANCTL_CMD + 'setprop -s Network:Name %s' % networkName
            datasetCmd = WPANCTL_CMD + 'setprop Dataset:NetworkName %s' % networkName
            self.hasActiveDatasetToCommit = True
            return self.__sendCommand(cmd)[0] != 'Fail' and self.__sendCommand(datasetCmd)[0] != 'Fail'
        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger('setNetworkName() Error: ' + str(e))

    def getNetworkName(self):
        """get Thread Network name"""
        print '%s call getNetworkname' % self.port
        networkName = self.__sendCommand(WPANCTL_CMD + 'getprop -v Network:Name')[0]
        return self.__stripValue(networkName)

    def setChannel(self, channel=15):
        """set channel of Thread device operates on.

        Args:
            channel:
                    (0  - 10: Reserved)
                    (11 - 26: 2.4GHz channels)
                    (27 - 65535: Reserved)

        Returns:
            True: successful to set the channel
            False: fail to set the channel
        """
        print '%s call setChannel' % self.port

        try:
            cmd = WPANCTL_CMD + 'setprop NCP:Channel %s' % channel
            datasetCmd = WPANCTL_CMD + 'setprop Dataset:Channel %s' % channel
            self.hasActiveDatasetToCommit = True
            return self.__sendCommand(cmd)[0] != 'Fail' and self.__sendCommand(datasetCmd)[0] != 'Fail'
        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger('setChannel() Error: ' + str(e))

    def getChannel(self):
        """get current channel"""
        print '%s call getChannel' % self.port
        return self.__sendCommand(WPANCTL_CMD + 'getprop -v NCP:Channel')[0]

    def setMAC(self, xEUI):
        """set the extended addresss of Thread device

        Args:
            xEUI: extended address in hex format

        Returns:
            True: successful to set the extended address
            False: fail to set the extended address
        """
        print '%s call setMAC' % self.port

        address64 = ''
        try:
            if not xEUI:
                address64 = self.mac

            if not isinstance(xEUI, str):
                address64 = self.__convertLongToString(xEUI)

                # prepend 0 at the beginning
                if len(address64) < 16:
                    address64 = address64.zfill(16)
                    print address64
            else:
                address64 = xEUI

            cmd = WPANCTL_CMD + 'setprop NCP:MACAddress %s' % address64

            if self.__sendCommand(cmd)[0] != 'Fail':
                self.mac = address64
                return True
            else:
                return False
        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger('setMAC() Error: ' + str(e))

    def getMAC(self, bType=MacType.RandomMac):
        """get one specific type of MAC address
           currently OpenThreadWpan only supports Random MAC address

        Args:
            bType: indicate which kind of MAC address is required

        Returns:
            specific type of MAC address
        """
        print '%s call getMAC' % self.port

        # if power down happens, return extended address assigned previously
        if self.isPowerDown:
            macAddr64 = self.mac
        else:
            if bType == MacType.FactoryMac:
                macAddr64 = self.__stripValue(self.__sendCommand(WPANCTL_CMD + 'getprop -v NCP:HardwareAddress')[0])
            elif bType == MacType.HashMac:
                macAddr64 = self.__stripValue(self.__sendCommand(WPANCTL_CMD + 'getprop -v NCP:MACAddress')[0])
            else:
                macAddr64 = self.__stripValue(self.__sendCommand(WPANCTL_CMD + 'getprop -v NCP:ExtendedAddress')[0])

        return int(macAddr64, 16)

    def getLL64(self):
        """get link local unicast IPv6 address"""
        print '%s call getLL64' % self.port
        return self.__stripValue(self.__sendCommand(WPANCTL_CMD + 'getprop -v IPv6:LinkLocalAddress')[0])

    def getMLEID(self):
        """get mesh local endpoint identifier address"""
        print '%s call getMLEID' % self.port
        return self.__getIp6Address('mesh EID')

    def getRloc16(self):
        """get rloc16 short address"""
        print '%s call getRloc16' % self.port
        rloc16 = self.__sendCommand(WPANCTL_CMD + 'getprop -v Thread:RLOC16')[0]
        return int(rloc16, 16)

    def getRloc(self):
        """get router locator unicast IPv6 address"""
        print '%s call getRloc' % self.port
        prefix = self.__stripValue(self.__sendCommand(WPANCTL_CMD + 'getprop -v IPv6:MeshLocalPrefix')[0])
        mlprefix = prefix.split('/')[0]
        rloc16 = self.__sendCommand(WPANCTL_CMD + 'getprop -v Thread:RLOC16')[0].lstrip('0x')
        print "prefix: " + prefix
        print "mlprefix: " + mlprefix
        print "rloc16: " + rloc16

        rloc = self.__padIp6Addr(mlprefix + '00ff:fe00:' + rloc16)
        print "rloc: " + rloc
        return rloc

    def getGlobal(self):
        """get global unicast IPv6 address set
           if configuring multiple entries
        """
        print '%s call getGlobal' % self.port
        return self.__getIp6Address('global')

    def setNetworkKey(self, key):
        """set Thread Network master key

        Args:
            key: Thread Network master key used in secure the MLE/802.15.4 packet

        Returns:
            True: successful to set the Thread Network master key
            False: fail to set the Thread Network master key
        """
        masterKey = ''
        print '%s call setNetworkKey' % self.port

        try:
            if not isinstance(key, str):
                masterKey = self.__convertLongToString(key)

                # prpend '0' at the beginning
                if len(masterKey) < 32:
                    masterKey = masterKey.zfill(32)

                cmd = WPANCTL_CMD + 'setprop Network:Key %s' % masterKey
                datasetCmd = WPANCTL_CMD + 'setprop Dataset:MasterKey %s' % masterKey
            else:
                masterKey = key
                cmd = WPANCTL_CMD + 'setprop Network:Key %s' % masterKey
                datasetCmd = WPANCTL_CMD + 'setprop Dataset:MasterKey %s' % masterKey

            self.networkKey = masterKey
            self.hasActiveDatasetToCommit = True
            return self.__sendCommand(cmd)[0] != 'Fail' and self.__sendCommand(datasetCmd)[0] != 'Fail'
        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger('setNetworkkey() Error: ' + str(e))

    def getNetworkKey(self):
        """get the current Thread Network master key"""
        print '%s call getNetwokKey' % self.port
        return self.networkKey

    def addBlockedMAC(self, xEUI):
        """add a given extended address to the blacklist entry

        Args:
            xEUI: extended address in hex format

        Returns:
            True: successful to add a given extended address to the blacklist entry
            False: fail to add a given extended address to the blacklist entry
        """
        print '%s call addBlockedMAC' % self.port
        print xEUI
        if isinstance(xEUI, str):
            macAddr = xEUI
        else:
            macAddr = self.__convertLongToString(xEUI)

        try:
            # if blocked device is itself
            if macAddr == self.mac:
                print 'block device itself'
                return True

            if self._addressfilterMode != 'blacklist':
                if self.__setAddressfilterMode('Blacklist'):
                    self._addressfilterMode = 'blacklist'

            cmd = WPANCTL_CMD + 'insert MAC:Blacklist:Entries %s' % macAddr
            ret = self.__sendCommand(cmd)[0] != 'Fail'

            self._addressfilterSet.add(macAddr)
            print 'current blacklist entries:'
            for addr in self._addressfilterSet:
                print addr

            return ret
        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger('addBlockedMAC() Error: ' + str(e))

    def addAllowMAC(self, xEUI):
        """add a given extended address to the whitelist addressfilter

        Args:
            xEUI: a given extended address in hex format

        Returns:
            True: successful to add a given extended address to the whitelist entry
            False: fail to add a given extended address to the whitelist entry
        """
        print '%s call addAllowMAC' % self.port
        print xEUI
        if isinstance(xEUI, str):
            macAddr = xEUI
        else:
            macAddr = self.__convertLongToString(xEUI)

        try:
            if self._addressfilterMode != 'whitelist':
                if self.__setAddressfilterMode('Whitelist'):
                    self._addressfilterMode = 'whitelist'

            cmd = WPANCTL_CMD + 'insert MAC:Whitelist:Entries %s' % macAddr
            ret = self.__sendCommand(cmd)[0] != 'Fail'

            self._addressfilterSet.add(macAddr)
            print 'current whitelist entries:'
            for addr in self._addressfilterSet:
                print addr
            return ret

        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger('addAllowMAC() Error: ' + str(e))

    def clearBlockList(self):
        """clear all entries in blacklist table

        Returns:
            True: successful to clear the blacklist
            False: fail to clear the blacklist
        """
        print '%s call clearBlockList' % self.port

        # remove all entries in blacklist
        try:
            print 'clearing blacklist entries:'
            for addr in self._addressfilterSet:
                print addr

            # disable blacklist
            if self.__setAddressfilterMode('disable'):
                self._addressfilterMode = 'disable'
                # clear ops
                cmd = WPANCTL_CMD + 'remove MAC:Blocklist:Entries'
                if self.__sendCommand(cmd)[0] != 'Fail':
                    self._addressfilterSet.clear()
                    return True
            return False
        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger('clearBlockList() Error: ' + str(e))

    def clearAllowList(self):
        """clear all entries in whitelist table

        Returns:
            True: successful to clear the whitelist
            False: fail to clear the whitelist
        """
        print '%s call clearAllowList' % self.port

        # remove all entries in whitelist
        try:
            print 'clearing whitelist entries:'
            for addr in self._addressfilterSet:
                print addr

            # disable whitelist
            if self.__setAddressfilterMode('disable'):
                self._addressfilterMode = 'disable'
                # clear ops
                cmd = WPANCTL_CMD + 'insert MAC:Whitelist:Entries'
                if self.__sendCommand(cmd)[0] != 'Fail':
                    self._addressfilterSet.clear()
                    return True
            return False
        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger('clearAllowList() Error: ' + str(e))

    def getDeviceRole(self):
        """get current device role in Thread Network"""
        print '%s call getDeviceRole' % self.port
        return self.__stripValue(self.__sendCommand(WPANCTL_CMD + 'getprop -v Network:NodeType')[0])

    def joinNetwork(self, eRoleId):
        """make device ready to join the Thread Network with a given role

        Args:
            eRoleId: a given device role id

        Returns:
            True: ready to set Thread Network parameter for joining desired Network
        """
        print '%s call joinNetwork' % self.port
        print eRoleId

        self.deviceRole = eRoleId
        mode = 15
        try:
            if ModuleHelper.LeaderDutChannelFound:
                self.channel = ModuleHelper.Default_Channel

            # FIXME: when Harness call setNetworkDataRequirement()?
            # only sleep end device requires stable networkdata now
            if eRoleId == Thread_Device_Role.Leader:
                print 'join as leader'
                #rsdn
                mode = 15
                if self.AutoDUTEnable is False:
                    # set ROUTER_DOWNGRADE_THRESHOLD
                    self.__setRouterDowngradeThreshold(33)
            elif eRoleId == Thread_Device_Role.Router:
                print 'join as router'
                #rsdn
                mode = 15
                if self.AutoDUTEnable is False:
                    # set ROUTER_DOWNGRADE_THRESHOLD
                    self.__setRouterDowngradeThreshold(33)
            elif eRoleId == Thread_Device_Role.SED:
                print 'join as sleepy end device'
                #s
                mode = 4
                self.setPollingRate(self.sedPollingRate)
            elif eRoleId == Thread_Device_Role.EndDevice:
                print 'join as end device'
                #rsn
                mode = 13
            elif eRoleId == Thread_Device_Role.REED:
                print 'join as REED'
                #rsdn
                mode = 15
                # set ROUTER_UPGRADE_THRESHOLD
                self.__setRouterUpgradeThreshold(0)
            elif eRoleId == Thread_Device_Role.EndDevice_FED:
                # always remain an ED, never request to be a router
                print 'join as FED'
                #rsdn
                mode = 15
                # set ROUTER_UPGRADE_THRESHOLD
                self.__setRouterUpgradeThreshold(0)
            elif eRoleId == Thread_Device_Role.EndDevice_MED:
                print 'join as MED'
                #rsn
                mode = 13
            else:
                pass

            # set Thread device mode with a given role
            self.__setDeviceMode(mode)
            self.__setKeySwitchGuardTime(0)  #temporally
            time.sleep(0.1)
            # start OpenThreadWpan
            self.__startOpenThreadWpan()
            time.sleep(3)
            return True
        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger('joinNetwork() Error: ' + str(e))

    def getNetworkFragmentID(self):
        """get current partition id of Thread Network Partition from LeaderData

        Returns:
            The Thread network Partition Id
        """
        print '%s call getNetworkFragmentID' % self.port
        if not self.____isOpenThreadWpanRunning():
            print 'OpenThreadWpan is not running'
            return None

        return self.__sendCommand(WPANCTL_CMD + 'getprop -v Network:PartitionId')[0]

    def getParentAddress(self):
        """get Thread device's parent extended address and rloc16 short address

        Returns:
            The extended address of parent in hex format
        """
        print '%s call getParentAddress' % self.port
        parentInfo = []
        parentInfo = self.__stripValue(self.__sendCommand(WPANCTL_CMD + 'getporp -v Thread:Parent')).split(' ')

        return parentInfo[0]

    def powerDown(self):
        """power down the OpenThreadWpan"""
        print '%s call powerDown' % self.port
        if self.__sendCommand(WPANCTL_CMD + 'setprop Daemon:AutoAssociateAfterReset false')[0] != 'Fail':
            time.sleep(0.5)
            if self.__sendCommand(WPANCTL_CMD + 'reset')[0] != 'Fail':
                self.isPowerDown = True
                return True
            else:
                return False
        else:
            return False

    def powerUp(self):
        """power up the Thread device"""
        print '%s call powerUp' % self.port
        if not self.handle:
            self._connect()

        self.isPowerDown = False
        if self.__sendCommand(WPANCTL_CMD + 'attach')[0] != 'Fail':
            time.sleep(3)
        else:
            return False

        if self.__sendCommand(WPANCTL_CMD + 'setprop Daemon:AutoAssociateAfterReset true')[0] == 'Fail':
            return False

        if self.__stripValue(self.__sendCommand(WPANCTL_CMD + 'getprop -v NCP:State')[0]) != 'associated':
            print 'powerUp failed'
            return False
        else:
            return True

    def reboot(self):
        """reset and rejoin to Thread Network without any timeout

        Returns:
            True: successful to reset and rejoin the Thread Network
            False: fail to reset and rejoin the Thread Network
        """
        print '%s call reboot' % self.port
        try:
            self._sendline(WPANCTL_CMD + 'reset')
            self.isPowerDown = True


            if self.__sendCommand(WPANCTL_CMD + 'getprop -v NCP:State')[0] != 'associated':
                print '[FAIL] reboot'
                return False
            else:
                return True
        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger('reboot() Error: ' + str(e))

    def ping(self, destination, length=20):
        """ send ICMPv6 echo request with a given length to a unicast destination
            address

        Args:
            destination: the unicast destination address of ICMPv6 echo request
            length: the size of ICMPv6 echo request payload
        """
        print '%s call ping' % self.port
        print 'destination: %s' %destination
        try:
            cmd = 'ping %s -c 1 -s %s -I %s'  % (destination, str(length), WPAN_INTERFACE)
            if self._is_net:
                ssh_stdin, ssh_stdout, ssh_stderr = self.handle.exec_command(cmd)
            else:
                self._sendline(cmd)
                self._expect(cmd)
            # wait echo reply
            time.sleep(1)
        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger('ping() Error: ' + str(e))

    def multicast_Ping(self, destination, length=20):
        """send ICMPv6 echo request with a given length to a multicast destination
           address

        Args:
            destination: the multicast destination address of ICMPv6 echo request
            length: the size of ICMPv6 echo request payload
        """
        print '%s call multicast_Ping' % self.port
        print 'destination: %s' % destination
        try:
            cmd = 'ping %s -c 1 -s %s -I %s' % (destination, str(length), WPAN_INTERFACE)
            if self._is_net:
                ssh_stdin, ssh_stdout, ssh_stderr = self.handle.exec_command(cmd)
            else:
                self._sendline(cmd)
                self._expect(cmd)
            # wait echo reply
            time.sleep(1)
        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger('multicast_ping() Error: ' + str(e))

    def getVersionNumber(self):
        """get OpenThreadWpan stack firmware version number"""
        print '%s call getVersionNumber' % self.port
        versionStr = self.__sendCommand(WPANCTL_CMD + 'getprop -v NCP:Version')[0]

        return self.__stripValue(versionStr)

    def setPANID(self, xPAN):
        """set Thread Network PAN ID

        Args:
            xPAN: a given PAN ID in hex format

        Returns:
            True: successful to set the Thread Network PAN ID
            False: fail to set the Thread Network PAN ID
        """
        print '%s call setPANID' % self.port
        print xPAN
        panid = ''
        try:
            if not isinstance(xPAN, str):
                panid = str(hex(xPAN))
                print panid

            cmd = WPANCTL_CMD + 'setprop -s Network:PANID %s' % panid
            datasetCmd = WPANCTL_CMD + 'setprop Dataset:PanId %s' % panid
            self.hasActiveDatasetToCommit = True
            return self.__sendCommand(cmd)[0] != 'Fail' and self.__sendCommand(datasetCmd)[0] != 'Fail'
        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger('setPANID() Error: ' + str(e))

    def getPANID(self):
        """get current Thread Network PAN ID"""
        print '%s call getPANID' % self.port
        return self.__sendCommand(WPANCTL_CMD + 'getprop -v Network:PANID')[0]

    def reset(self):
        """factory reset"""
        print '%s call reset' % self.port
        try:
            if self._is_net:
                self.__sendCommand(WPANCTL_CMD + 'leave')
            else:
                self._sendline(WPANCTL_CMD + 'leave')

            self.__sendCommand(WPANCTL_CMD + 'dataset erase')
            time.sleep(2)
            if not self._is_net:
                self._read()
        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger('reset() Error: ' + str(e))

    def removeRouter(self, xRouterId):
        """kick router with a given router id from the Thread Network

        Args:
            xRouterId: a given router id in hex format

        Returns:
            True: successful to remove the router from the Thread Network
            False: fail to remove the router from the Thread Network
        """
        print '%s call removeRouter' % self.port
        print xRouterId
        routerId = ''
        routerId = self.__convertRlocToRouterId(xRouterId)
        print routerId

        if routerId == None:
            print 'no matched xRouterId'
            return False

        try:
            cmd = 'releaserouterid %s' % routerId
            return self.__sendCommand(cmd)[0] != 'Fail'
        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger('removeRouter() Error: ' + str(e))

    def setDefaultValues(self):
        """set default mandatory Thread Network parameter value"""
        print '%s call setDefaultValues' % self.port

        # initialize variables
        self.networkName = ModuleHelper.Default_NwkName
        self.networkKey = ModuleHelper.Default_NwkKey
        self.channel = ModuleHelper.Default_Channel
        self.channelMask = "0x7fff800" #(0xffff << 11)
        self.panId = ModuleHelper.Default_PanId
        self.xpanId = ModuleHelper.Default_XpanId
        self.localprefix = ModuleHelper.Default_MLPrefix
        self.pskc = "00000000000000000000000000000000"  # OT only accept hex format PSKc for now
        self.securityPolicySecs = ModuleHelper.Default_SecurityPolicy
        self.securityPolicyFlags = "onrcb"
        self.activetimestamp = ModuleHelper.Default_ActiveTimestamp
        #self.sedPollingRate = ModuleHelper.Default_Harness_SED_Polling_Rate
        self.sedPollingRate = 3
        self.deviceRole = None
        self.provisioningUrl = ''
        self.hasActiveDatasetToCommit = False
        self.logThread = Queue()
        self.logThreadStatus = self.logStatus['stop']
        self.networkDataRequirement = ''      # indicate Thread device requests full or stable network data
        self.isPowerDown = False              # indicate if Thread device experiences a power down event
        self._addressfilterMode = 'disable'   # indicate AddressFilter mode ['disable', 'whitelist', 'blacklist']
        self._addressfilterSet = set()        # cache filter entries
        self.isActiveCommissioner = False     # indicate if Thread device is an active commissioner
        self._lines = None                    # buffered lines read from device

        # initialize device configuration
        try:
            self.setMAC(self.mac)
            self.__setChannelMask(self.channelMask)
            self.__setSecurityPolicy(self.securityPolicySecs, self.securityPolicyFlags)
            self.setChannel(self.channel)
            self.setPANID(self.panId)
            self.setXpanId(self.xpanId)
            self.setNetworkName(self.networkName)
            self.setNetworkKey(self.networkKey)
            self.setMLPrefix(self.localprefix)
            self.setPSKc(self.pskc)
            self.setActiveTimestamp(self.activetimestamp)
        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger('setDefaultValue() Error: ' + str(e))

    def getDeviceConncetionStatus(self):
        """check if serial port connection is ready or not"""
        print '%s call getDeviceConnectionStatus' % self.port
        return self.deviceConnected

    def getPollingRate(self):
        """get data polling rate for sleepy end device"""
        print '%s call getPollingRate' % self.port
        return self.__sendCommand(WPANCTL_CMD + 'getprop -v NCP:SleepyPollInterval')[0]

    def setPollingRate(self, iPollingRate):
        """set data polling rate for sleepy end device

        Args:
            iPollingRate: data poll period of sleepy end device

        Returns:
            True: successful to set the data polling rate for sleepy end device
            False: fail to set the data polling rate for sleepy end device
        """
        print '%s call setPollingRate' % self.port
        print iPollingRate
        try:
            cmd = WPANCTL_CMD + 'setprop NCP:SleepyPollInterval %s' % str(iPollingRate*1000)
            print cmd
            return self.__sendCommand(cmd)[0] != 'Fail'
        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger('setPollingRate() Error: ' + str(e))

    def setLinkQuality(self, EUIadr, LinkQuality):
        """set custom LinkQualityIn for all receiving messages from the specified EUIadr

        Args:
            EUIadr: a given extended address
            LinkQuality: a given custom link quality
                         link quality/link margin mapping table
                         3: 21 - 255 (dB)
                         2: 11 - 20 (dB)
                         1: 3 - 9 (dB)
                         0: 0 - 2 (dB)

        Returns:
            True: successful to set the link quality
            False: fail to set the link quality

        @todo: required if as reference device
        """
        pass

    def setOutBoundLinkQuality(self, LinkQuality):
        """set custom LinkQualityIn for all receiving messages from the any address

        Args:
            LinkQuality: a given custom link quality
                         link quality/link margin mapping table
                         3: 21 - 255 (dB)
                         2: 11 - 20 (dB)
                         1: 3 - 9 (dB)
                         0: 0 - 2 (dB)

        Returns:
            True: successful to set the link quality
            False: fail to set the link quality

        @todo: required if as reference device
        """
        pass

    def removeRouterPrefix(self, prefixEntry):
        """remove the configured prefix on a border router

        Args:
            prefixEntry: a on-mesh prefix entry

        Returns:
            True: successful to remove the prefix entry from border router
            False: fail to remove the prefix entry from border router

        @todo: required if as reference device
        """
        pass

    def resetAndRejoin(self, timeout):
        """reset and join back Thread Network with a given timeout delay

        Args:
            timeout: a timeout interval before rejoin Thread Network

        Returns:
            True: successful to reset and rejoin Thread Network
            False: fail to reset and rejoin the Thread Network
        """
        print '%s call resetAndRejoin' % self.port
        print timeout
        try:
            if self.__sendCommand(WPANCTL_CMD + 'setprop Daemon:AutoAssociateAfterReset false')[0] != 'Fail':
                time.sleep(0.5)
                if self.__sendCommand(WPANCTL_CMD + 'reset')[0] != 'Fail':
                    self.isPowerDown = True
                else:
                    return False
            else:
                return False
            time.sleep(timeout)

            if self.deviceRole == Thread_Device_Role.SED:
                self.setPollingRate(self.sedPollingRate)

            if self.__sendCommand(WPANCTL_CMD + 'attach')[0] != 'Fail':
                time.sleep(3)
            else:
                return False

            if self.__sendCommand(WPANCTL_CMD + 'setprop Daemon:AutoAssociateAfterReset true')[0] == 'Fail':
                return False

            if self.__stripValue(self.__sendCommand(WPANCTL_CMD + 'getprop -v NCP:State')[0]) != 'associated':
                print '[FAIL] reset and rejoin'
                return False
            return True
        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger('resetAndRejoin() Error: ' + str(e))

    def configBorderRouter(self, P_Prefix, P_stable=1, P_default=1, P_slaac_preferred=0, P_Dhcp=0, P_preference=0, P_on_mesh=1, P_nd_dns=0):
        """configure the border router with a given prefix entry parameters

        Args:
            P_Prefix: IPv6 prefix that is available on the Thread Network
            P_stable: is true if the default router is expected to be stable network data
            P_default: is true if border router offers the default route for P_Prefix
            P_slaac_preferred: is true if Thread device is allowed auto-configure address using P_Prefix
            P_Dhcp: is true if border router is a DHCPv6 Agent
            P_preference: is two-bit signed integer indicating router preference
            P_on_mesh: is true if P_Prefix is considered to be on-mesh
            P_nd_dns: is true if border router is able to supply DNS information obtained via ND

        Returns:
            True: successful to configure the border router with a given prefix entry
            False: fail to configure the border router with a given prefix entry
        """
        print '%s call configBorderRouter' % self.port
        prefix = self.__convertIp6PrefixStringToIp6Address(str(P_Prefix))
        print prefix
        try:
            parameter = ''

            if P_slaac_preferred == 1:
                parameter += ' -a -f'

            if P_stable == 1:
                parameter += ' -s'

            if P_default == 1:
                parameter += ' -r'

            if P_Dhcp == 1:
                parameter += ' -d'

            if P_on_mesh == 1:
                parameter += ' -o'

            cmd = WPANCTL_CMD + 'add-prefix %s %s -P %d' % (prefix, parameter, P_preference)
            print parameter
            print cmd
            if self.__sendCommand(cmd)[0] != 'Fail':
                return True
            else:
                return False
        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger('configBorderRouter() Error: ' + str(e))

    def setNetworkIDTimeout(self, iNwkIDTimeOut):
        """set networkid timeout for OpenThreadWpan

        Args:
            iNwkIDTimeOut: a given NETWORK_ID_TIMEOUT

        Returns:
            True: successful to set NETWORK_ID_TIMEOUT
            False: fail to set NETWORK_ID_TIMEOUT

        @todo: required if as reference device
        """
        pass

    def setKeepAliveTimeOut(self, iTimeOut):
        """set keep alive timeout for device
           has been deprecated and also set SED polling rate

        Args:
            iTimeOut: data poll period for sleepy end device

        Returns:
            True: successful to set the data poll period for SED
            False: fail to set the data poll period for SED
        """
        print '%s call setKeepAliveTimeOut' % self.port
        print iTimeOut
        try:
            cmd = WPANCTL_CMD + 'setprop NCP:SleepyPollInterval %s' % str(iTimeOut*1000)
            print cmd
            return self.__sendCommand(cmd)[0] != 'Fail'
        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger('setKeepAliveTimeOut() Error: ' + str(e))

    def setKeySequenceCounter(self, iKeySequenceValue):
        """ set the Key sequence counter corresponding to Thread Network master key

        Args:
            iKeySequenceValue: key sequence value

        Returns:
            True: successful to set the key sequence
            False: fail to set the key sequence
        """
        print '%s call setKeySequenceCounter' % self.port
        print iKeySequenceValue
        try:
            cmd = WPANCTL_CMD + 'setprop Network:KeyIndex %s' % str(iKeySequenceValue)
            if self.__sendCommand(cmd)[0] != 'Fail':
                time.sleep(1)
                return True
            else:
                return False
        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger('setKeySequenceCounter() Error: ' + str(e))

    def getKeySequenceCounter(self):
        """get current Thread Network key sequence"""
        print '%s call getKeySequenceCounter' % self.port
        keySequence = ''
        keySequence = self.__sendCommand(WPANCTL_CMD + 'getprop -v Network:KeyIndex')[0]
        return keySequence

    def incrementKeySequenceCounter(self, iIncrementValue=1):
        """increment the key sequence with a given value

        Args:
            iIncrementValue: specific increment value to be added

        Returns:
            True: successful to increment the key sequence with a given value
            False: fail to increment the key sequence with a given value
        """
        print '%s call incrementKeySequenceCounter' % self.port
        print iIncrementValue
        currentKeySeq = ''
        try:
            currentKeySeq = self.getKeySequenceCounter()
            keySequence = int(currentKeySeq, 10) + iIncrementValue
            print keySequence
            return self.setKeySequenceCounter(keySequence)
        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger('incrementKeySequenceCounter() Error: ' + str(e))

    def setNetworkDataRequirement(self, eDataRequirement):
        """set whether the Thread device requires the full network data
           or only requires the stable network data

        Args:
            eDataRequirement: is true if requiring the full network data

        Returns:
            True: successful to set the network requirement
        """
        print '%s call setNetworkDataRequirement' % self.port
        print eDataRequirement

        if eDataRequirement == Device_Data_Requirement.ALL_DATA:
            self.networkDataRequirement = 'n'
        return True

    def configExternalRouter(self, P_Prefix, P_stable, R_Preference=0):
        """configure border router with a given external route prefix entry

        Args:
            P_Prefix: IPv6 prefix for the route
            P_Stable: is true if the external route prefix is stable network data
            R_Preference: a two-bit signed integer indicating Router preference
                          1: high
                          0: medium
                         -1: low

        Returns:
            True: successful to configure the border router with a given external route prefix
            False: fail to configure the border router with a given external route prefix
        """
        print '%s call configExternalRouter' % self.port
        print P_Prefix
        stable = ''
        prefix = self.__convertIp6PrefixStringToIp6Address(str(P_Prefix))
        try:
            if P_stable:
                cmd = WPANCTL_CMD + 'add-route %s -l 64 -p %d' % (prefix, R_Preference)
            else:
                cmd = WPANCTL_CMD + 'add-route %s -l 64 -p %d -n' % (prefix, R_Preference)
            print cmd

            if self.__sendCommand(cmd)[0] != 'Fail':
                return True
            else:
                return False
        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger('configExternalRouter() Error: ' + str(e))

    def getNeighbouringRouters(self):
        """get neighboring routers information

        Returns:
            neighboring routers' extended address

        @todo: required if as reference device
        """
        pass

    def getChildrenInfo(self):
        """get all children information

        Returns:
            children's extended address

        @todo: required if as reference device
        """
        pass

    def setXpanId(self, xPanId):
        """set extended PAN ID of Thread Network

        Args:
            xPanId: extended PAN ID in hex format

        Returns:
            True: successful to set the extended PAN ID
            False: fail to set the extended PAN ID
        """
        xpanid = ''
        print '%s call setXpanId' % self.port
        print xPanId
        try:
            if not isinstance(xPanId, str):
                xpanid = self.__convertLongToString(xPanId)

                # prepend '0' at the beginning
                if len(xpanid) < 16:
                    xpanid = xpanid.zfill(16)
                    print xpanid
                    cmd = WPANCTL_CMD + 'setprop Network:XPANID %s' % xpanid
                    datasetCmd = WPANCTL_CMD + 'setprop Dataset:ExtendedPanId %s' % xpanid
            else:
                xpanid = xPanId
                cmd = WPANCTL_CMD + 'setprop Network:XPANID %s' % xpanid
                datasetCmd = WPANCTL_CMD + 'setprop Dataset:ExtendedPanId %s' % xpanid

            self.xpanId = xpanid
            self.hasActiveDatasetToCommit = True
            return self.__sendCommand(cmd)[0] != 'Fail' and self.__sendCommand(datasetCmd)[0] != 'Fail'
        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger('setXpanId() Error: ' + str(e))

    def getNeighbouringDevices(self):
        """gets the neighboring devices' extended address to compute the DUT
           extended address automatically

        Returns:
            A list including extended address of neighboring routers, parent
            as well as children
        """
        print '%s call getNeighbouringDevices' % self.port
        neighbourList = []

        # get parent info
        parentAddr = self.getParentAddress()
        if parentAddr != 0:
            neighbourList.append(parentAddr)

        # get ED/SED children info
        childNeighbours = self.getChildrenInfo()
        if childNeighbours != None and len(childNeighbours) > 0:
            for entry in childNeighbours:
                neighbourList.append(entry)

        # get neighboring routers info
        routerNeighbours = self.getNeighbouringRouters()
        if routerNeighbours != None and len(routerNeighbours) > 0:
            for entry in routerNeighbours:
                neighbourList.append(entry)

        print neighbourList
        return neighbourList

    def setPartationId(self, partationId):
        """set Thread Network Partition ID

        Args:
            partitionId: partition id to be set by leader

        Returns:
            True: successful to set the Partition ID
            False: fail to set the Partition ID
        """
        print '%s call setPartationId' % self.port
        print partationId

        cmd = WPANCTL_CMD + 'setprop Network:PartitionId %s' %(str(hex(partationId)).rstrip('L'))
        print cmd
        return self.__sendCommand(cmd)[0] != 'Fail'

    def getGUA(self, filterByPrefix=None):
        """get expected global unicast IPv6 address of OpenThreadWpan

        Args:
            filterByPrefix: a given expected global IPv6 prefix to be matched

        Returns:
            a global IPv6 address
        """
        print '%s call getGUA' % self.port
        print filterByPrefix
        globalAddrs = []
        try:
            # get global addrs set if multiple
            globalAddrs = self.getGlobal()

            if filterByPrefix is None:
                return self.__padIp6Addr(globalAddrs[0])
            else:
                for line in globalAddrs:
                    line = self.__padIp6Addr(line)
                    print "Padded IPv6 Address:" + line
                    if line.startswith(filterByPrefix):
                        return line
                print 'no global address matched'
                return str(globalAddrs[0])
        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger('getGUA() Error: ' + str(e))
            return e

    def getShortAddress(self):
        """get Rloc16 short address of Thread device"""
        print '%s call getShortAddress' % self.port
        return self.getRloc16()

    def getULA64(self):
        """get mesh local EID of Thread device"""
        print '%s call getULA64' % self.port
        return self.getMLEID()

    def setMLPrefix(self, sMeshLocalPrefix):
        """set mesh local prefix"""
        print '%s call setMLPrefix' % self.port
        try:
            cmd = WPANCTL_CMD + 'setprop IPv6:MeshLocalPrefix %s' % sMeshLocalPrefix
            datasetCmd = WPANCTL_CMD + 'setprop Dataset:MeshLocalPrefix %s' % sMeshLocalPrefix
            self.hasActiveDatasetToCommit = True
            return self.__sendCommand(cmd)[0] != 'Fail' and self.__sendCommand(datasetCmd)[0] != 'Fail'
        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger('setMLPrefix() Error: ' + str(e))

    def getML16(self):
        """get mesh local 16 unicast address (Rloc)"""
        print '%s call getML16' % self.port
        return self.getRloc()

    def downgradeToDevice(self):
        pass

    def upgradeToRouter(self):
        pass

    def forceSetSlaac(self, slaacAddress):
        """@todo : required if as reference device"""
        pass

    def setSleepyNodePollTime(self):
        pass

    def enableAutoDUTObjectFlag(self):
        """set AutoDUTenable flag"""
        print '%s call enableAutoDUTObjectFlag' % self.port
        self.AutoDUTEnable = True

    def getChildTimeoutValue(self):
        """get child timeout"""
        print '%s call getChildTimeoutValue' % self.port
        childTimeout = self.__sendCommand(WPANCTL_CMD + 'getporp -v Thread:ChildTimeout')[0]
        return int(childTimeout)

    def diagnosticGet(self, strDestinationAddr, listTLV_ids=[]):
        """@todo : required if as reference device"""
        pass


    def diagnosticQuery(self,strDestinationAddr, listTLV_ids = []):
        """@todo : required if as reference device"""
        self.diagnosticGet(strDestinationAddr, listTLV_ids)

    def diagnosticReset(self, strDestinationAddr, listTLV_ids=[]):
        """@todo : required if as reference device"""
        pass

    def startNativeCommissioner(self, strPSKc='GRLpassWord'):
        #TODO: Support the whole Native Commissioner functionality
        #      Currently it only aims to trigger a Discovery Request message to pass Certification test 5.8.4
        print '%s call startNativeCommissioner' % self.port
        cmd = WPANCTL_CMD + 'joiner --start %s' %(strPSKc)
        print cmd
        if self.__sendCommand(cmd)[0] != "Fail":
            return True
        else:
            return False

    def startCollapsedCommissioner(self):
        """start Collapsed Commissioner

        Returns:
            True: successful to start Commissioner
            False: fail to start Commissioner
        """
        print '%s call startCollapsedCommissioner' % self.port
        startCmd = WPANCTL_CMD + 'form %s -c %s -T router' % (self.networkName, str(self.channel))
        if self.__sendCommand(startCmd) != 'Fail':
            time.sleep(2)
            cmd = WPANCTL_CMD + 'commissioner start'
            print cmd
            if self.__sendCommand(cmd)[0] != 'Fail':
                self.isActiveCommissioner = True
                time.sleep(20) # time for petition process
                return True
        return False

    def setJoinKey(self, strPSKc):
        pass

    def scanJoiner(self, xEUI='*', strPSKd='threadjpaketest'):
        """scan Joiner

        Args:
            xEUI: Joiner's EUI-64
            strPSKd: Joiner's PSKd for commissioning

        Returns:
            True: successful to add Joiner's steering data
            False: fail to add Joiner's steering data
        """
        print '%s call scanJoiner' % self.port
        if not isinstance(xEUI, str):
            eui64 = self.__convertLongToString(xEUI)

            # prepend 0 at the beginning
            if len(eui64) < 16:
                eui64 = eui64.zfill(16)
                print eui64
        else:
            eui64 = xEUI

        # long timeout value to avoid automatic joiner removal (in seconds)
        timeout = 500

        cmd = WPANCTL_CMD + 'commissioner joiner-add %s %s %s' % (eui64, str(timeout), strPSKd)
        print cmd
        if not self.isActiveCommissioner:
            self.startCollapsedCommissioner()
        if self.__sendCommand(cmd)[0] != 'Fail':
            return True
        else:
            return False

    def setProvisioningUrl(self, strURL='grl.com'):
        """set provisioning Url

        Args:
            strURL: Provisioning Url string

        Returns:
            True: successful to set provisioning Url
            False: fail to set provisioning Url
        """
        print '%s call setProvisioningUrl' % self.port
        self.provisioningUrl = strURL
        if self.deviceRole == Thread_Device_Role.Commissioner:
            cmd = WPANCTL_CMD + 'setprop Commissioner:ProvisioningUrl %s' %(strURL)
            print cmd
            return self.__sendCommand(cmd)[0] != "Fail"
        return True

    def allowCommission(self):
        """start commissioner candidate petition process

        Returns:
            True: successful to start commissioner candidate petition process
            False: fail to start commissioner candidate petition process
        """
        print '%s call allowCommission' % self.port
        try:
            cmd = WPANCTL_CMD + 'commissioner start'
            print cmd
            if self.isActiveCommissioner:
                return True
            if self.__sendCommand(cmd)[0] != 'Fail':
                self.isActiveCommissioner = True
                time.sleep(40) # time for petition process and at least one keep alive
                return True
            else:
                return False
        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger('allowcommission() error: ' + str(e))

    def joinCommissioned(self, strPSKd='threadjpaketest', waitTime=20):
        """start joiner

        Args:
            strPSKd: Joiner's PSKd

        Returns:
            True: successful to start joiner
            False: fail to start joiner
        """
        print '%s call joinCommissioned' % self.port
        cmd = WPANCTL_CMD + 'joiner --start %s %s' %(strPSKd, self.provisioningUrl)
        print cmd
        if self.__sendCommand(cmd)[0] != "Fail":
            if self.__getJoinerState():
                self.__sendCommand(WPANCTL_CMD + 'joiner --attach')
                time.sleep(30)
                return True
            else:
                return False
        else:
            return False

    def getCommissioningLogs(self):
        """get Commissioning logs

        Returns:
           Commissioning logs
        """
        rawLogs = self.logThread.get()
        ProcessedLogs = []
        payload = []
        while not rawLogs.empty():
            rawLogEach = rawLogs.get()
            print rawLogEach
            if "[THCI]" not in rawLogEach:
                continue

            EncryptedPacket = PlatformDiagnosticPacket()
            infoList = rawLogEach.split('[THCI]')[1].split(']')[0].split('|')
            for eachInfo in infoList:
                print eachInfo
                info = eachInfo.split("=")
                infoType = info[0].strip()
                infoValue = info[1].strip()
                if "direction" in infoType:
                    EncryptedPacket.Direction = PlatformDiagnosticPacket_Direction.IN if 'recv' in infoValue \
                        else PlatformDiagnosticPacket_Direction.OUT if 'send' in infoValue \
                        else PlatformDiagnosticPacket_Direction.UNKNOWN
                elif "type" in infoType:
                    EncryptedPacket.Type = PlatformDiagnosticPacket_Type.JOIN_FIN_req if 'JOIN_FIN.req' in infoValue \
                        else PlatformDiagnosticPacket_Type.JOIN_FIN_rsp if 'JOIN_FIN.rsp' in infoValue \
                        else PlatformDiagnosticPacket_Type.JOIN_ENT_req if 'JOIN_ENT.ntf' in infoValue \
                        else PlatformDiagnosticPacket_Type.JOIN_ENT_rsp if 'JOIN_ENT.rsp' in infoValue \
                        else PlatformDiagnosticPacket_Type.UNKNOWN
                elif "len" in infoType:
                    bytesInEachLine = 16
                    EncryptedPacket.TLVsLength = int(infoValue)
                    payloadLineCount = (int(infoValue) + bytesInEachLine - 1)/bytesInEachLine
                    while payloadLineCount > 0:
                        payloadLineCount = payloadLineCount - 1
                        payloadLine = rawLogs.get()
                        payloadSplit = payloadLine.split('|')
                        for block in range(1, 3):
                            payloadBlock = payloadSplit[block]
                            payloadValues = payloadBlock.split(' ')
                            for num in range(1, 9):
                                if ".." not in payloadValues[num]:
                                    payload.append(int(payloadValues[num], 16))

                    EncryptedPacket.TLVs = PlatformPackets.read(EncryptedPacket.Type, payload) if payload != [] else []

            ProcessedLogs.append(EncryptedPacket)
        return ProcessedLogs

    def MGMT_ED_SCAN(self, sAddr, xCommissionerSessionId, listChannelMask, xCount, xPeriod, xScanDuration):
        """send MGMT_ED_SCAN message to a given destinaition.

        Args:
            sAddr: IPv6 destination address for this message
            xCommissionerSessionId: commissioner session id
            listChannelMask: a channel array to indicate which channels to be scaned
            xCount: number of IEEE 802.15.4 ED Scans (milliseconds)
            xPeriod: Period between successive IEEE802.15.4 ED Scans (milliseconds)
            xScanDuration: IEEE 802.15.4 ScanDuration to use when performing an IEEE 802.15.4 ED Scan (milliseconds)

        Returns:
            True: successful to send MGMT_ED_SCAN message.
            False: fail to send MGMT_ED_SCAN message
        """
        print '%s call MGMT_ED_SCAN' % self.port
        channelMask = ''
        channelMask = self.__ChannelMaskListToStr(listChannelMask)
        try:
            cmd = WPANCTL_CMD + 'commissioner energy-scan %s %s %s %s %s' % (channelMask, xCount, xPeriod, xScanDuration, sAddr)
            print cmd
            return self.__sendCommand(cmd) != 'Fail'
        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger('MGMT_ED_SCAN() error: ' + str(e))

    def MGMT_PANID_QUERY(self, sAddr, xCommissionerSessionId, listChannelMask, xPanId):
        """send MGMT_PANID_QUERY message to a given destination

        Args:
            xPanId: a given PAN ID to check the conflicts

        Returns:
            True: successful to send MGMT_PANID_QUERY message.
            False: fail to send MGMT_PANID_QUERY message.
        """
        print '%s call MGMT_PANID_QUERY' % self.port
        panid = ''
        channelMask = ''
        channelMask = self.__ChannelMaskListToStr(listChannelMask)

        if not isinstance(xPanId, str):
            panid = str(hex(xPanId))

        try:
            cmd = WPANCTL_CMD + 'commissioner pan-id-query %s %s %s' % (panid, channelMask, sAddr)
            print cmd
            return self.__sendCommand(cmd) != 'Fail'
        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger('MGMT_PANID_QUERY() error: ' + str(e))

    def MGMT_ANNOUNCE_BEGIN(self, sAddr, xCommissionerSessionId, listChannelMask, xCount, xPeriod):
        """send MGMT_ANNOUNCE_BEGIN message to a given destination

        Returns:
            True: successful to send MGMT_ANNOUNCE_BEGIN message.
            False: fail to send MGMT_ANNOUNCE_BEGIN message.
        """
        print '%s call MGMT_ANNOUNCE_BEGIN' % self.port
        channelMask = ''
        channelMask = self.__ChannelMaskListToStr(listChannelMask)
        try:
            cmd = WPANCTL_CMD + 'commissioner announce-begin %s %s %s %s' % (channelMask, xCount, xPeriod, sAddr)
            print cmd
            return self.__sendCommand(cmd) != 'Fail'
        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger('MGMT_ANNOUNCE_BEGIN() error: ' + str(e))

    def MGMT_ACTIVE_GET(self, Addr='', TLVs=[]):
        """send MGMT_ACTIVE_GET command

        Returns:
            True: successful to send MGMT_ACTIVE_GET
            False: fail to send MGMT_ACTIVE_GET
        """
        print '%s call MGMT_ACTIVE_GET' % self.port
        try:

            cmd = WPANCTL_CMD + 'dataset mgmt-get-active'

            if len(TLVs) != 0:
                tlvs = "".join(hex(tlv).lstrip("0x").zfill(2) for tlv in TLVs)
                setTLVCmd = WPANCTL_CMD + 'setprop Dataset:RawTlvs ' + tlvs
                if self.__sendCommand(setTLVCmd)[0] == 'Fail':
                    return False
            else:
                if self.__sendCommand(WPANCTL_CMD + 'dataset erase')[0] == 'Fail':
                    return False

            if Addr != '':
                setAddressCmd = WPANCTL_CMD + 'setprop Dataset:DestIpAddress ' + Addr
                if self.__sendCommand(setAddressCmd)[0] == 'Fail':
                    return False

            print cmd

            return self.__sendCommand(cmd)[0] != 'Fail'

        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger('MGMT_ACTIVE_GET() Error: ' + str(e))

    def MGMT_ACTIVE_SET(self, sAddr='', xCommissioningSessionId=None, listActiveTimestamp=None, listChannelMask=None, xExtendedPanId=None,
                        sNetworkName=None, sPSKc=None, listSecurityPolicy=None, xChannel=None, sMeshLocalPrefix=None, xMasterKey=None,
                        xPanId=None, xTmfPort=None, xSteeringData=None, xBorderRouterLocator=None, BogusTLV=None, xDelayTimer=None):
        """send MGMT_ACTIVE_SET command

        Returns:
            True: successful to send MGMT_ACTIVE_SET
            False: fail to send MGMT_ACTIVE_SET
        """
        print '%s call MGMT_ACTIVE_SET' % self.port
        try:
            cmd = WPANCTL_CMD + 'dataset mgmt-set-active'

            if self.__sendCommand(WPANCTL_CMD + 'dataset erase')[0] == 'Fail':
                return False

            if listActiveTimestamp != None:
                sActiveTimestamp = str(hex(listActiveTimestamp[0]))
                if len(sActiveTimestamp) < 18:
                    sActiveTimestamp = sActiveTimestamp.lstrip('0x').zfill(16)
                setActiveTimeCmd = WPANCTL_CMD + 'setprop Dataset:ActiveTimestamp ' + sActiveTimestamp
                if self.__sendCommand(setActiveTimeCmd)[0] == 'Fail':
                    return False

            if xExtendedPanId != None:
                xpanid = self.__convertLongToString(xExtendedPanId)
                if len(xpanid) < 16:
                    xpanid = xpanid.zfill(16)
                setExtendedPanIdCmd = WPANCTL_CMD + 'setprop Dataset:ExtendedPanId ' + xpanid
                if self.__sendCommand(setExtendedPanIdCmd)[0] == 'Fail':
                    return False

            if sNetworkName != None:
                setNetworkNameCmd = WPANCTL_CMD + 'setprop Dataset:NetworkName ' + str(sNetworkName)
                if self.__sendCommand(setNetworkNameCmd)[0] == 'Fail':
                    return False

            if xChannel != None:
                setChannelCmd = WPANCTL_CMD + 'setprop Dataset:Channel ' + str(xChannel)
                if self.__sendCommand(setChannelCmd)[0] == 'Fail':
                    return False

            if sMeshLocalPrefix != None:
                setMLPrefixCmd = WPANCTL_CMD + 'setprop Dataset:MeshLocalPrefix ' + str(sMeshLocalPrefix)
                if self.__sendCommand(setMLPrefixCmd)[0] == 'Fail':
                    return False

            if xMasterKey != None:
                key = self.__convertLongToString(xMasterKey)

                if len(key) < 32:
                    key = key.zfill(32)

                setMasterKeyCmd = WPANCTL_CMD + 'setprop Dataset:MasterKey ' + key
                if self.__sendCommand(setMasterKeyCmd)[0] == 'Fail':
                    return False

            if xPanId != None:
                setPanIdCmd = WPANCTL_CMD + 'setprop Dataset:PanId ' + str(xPanId)
                if self.__sendCommand(setPanIdCmd)[0] == 'Fail':
                    return False

            if listChannelMask != None:
                setChannelMaskCmd = WPANCTL_CMD + 'setprop Dataset:ChannelMaskPage0 ' \
                                    + '0x' + self.__convertLongToString(self.__convertChannelMask(listChannelMask))
                if self.__sendCommand(setChannelMaskCmd)[0] == 'Fail':
                    return False

            if sPSKc != None or listSecurityPolicy != None or \
                    xCommissioningSessionId != None or xTmfPort != None or xSteeringData != None or xBorderRouterLocator != None or \
                    BogusTLV != None:
                setRawTLVCmd = WPANCTL_CMD + 'setprop Dataset:RawTlvs '

            if sPSKc != None:
                setRawTLVCmd += '0410'
                stretchedPskc = Thread_PBKDF2.get(sPSKc, ModuleHelper.Default_XpanId, ModuleHelper.Default_NwkName)
                pskc = hex(stretchedPskc).rstrip('L').lstrip('0x')

                if len(pskc) < 32:
                    pskc = pskc.zfill(32)

                setRawTLVCmd += pskc

            if listSecurityPolicy != None:
                setRawTLVCmd += '0c03'

                rotationTime = 0
                policyBits = 0

                # previous passing way listSecurityPolicy=[True, True, 3600, False, False, True]
                if (len(listSecurityPolicy) == 6):
                    rotationTime = listSecurityPolicy[2]

                    # the last three reserved bits must be 1
                    policyBits = 0b00000111

                    if listSecurityPolicy[0]:
                        policyBits = policyBits | 0b10000000
                    if listSecurityPolicy[1]:
                        policyBits = policyBits | 0b01000000
                    if listSecurityPolicy[3]:
                        policyBits = policyBits | 0b00100000
                    if listSecurityPolicy[4]:
                        policyBits = policyBits | 0b00010000
                    if listSecurityPolicy[5]:
                        policyBits = policyBits | 0b00001000
                else:
                    # new passing way listSecurityPolicy=[3600, 0b11001111]
                    rotationTime = listSecurityPolicy[0]
                    policyBits = listSecurityPolicy[1]

                policy = str(hex(rotationTime))[2:]

                if len(policy) < 4:
                    policy = policy.zfill(4)

                setRawTLVCmd += policy

                setRawTLVCmd += str(hex(policyBits))[2:]

            if xCommissioningSessionId != None:
                setRawTLVCmd += '0b02'
                sessionid = str(hex(xCommissioningSessionId))[2:]

                if len(sessionid) < 4:
                    sessionid = sessionid.zfill(4)

                setRawTLVCmd += sessionid

            if xBorderRouterLocator != None:
                setRawTLVCmd += '0902'
                locator = str(hex(xBorderRouterLocator))[2:]

                if len(locator) < 4:
                    locator = locator.zfill(4)

                setRawTLVCmd += locator

            if xSteeringData != None:
                steeringData = self.__convertLongToString(xSteeringData)
                setRawTLVCmd += '08' + str(len(steeringData)/2).zfill(2)
                setRawTLVCmd += steeringData

            if BogusTLV != None:
                setRawTLVCmd += "8202aa55"

            print setRawTLVCmd
            print cmd

            if self.__sendCommand(setRawTLVCmd)[0] == 'Fail':
                return False

            return self.__sendCommand(cmd)[0] != 'Fail'

        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger('MGMT_ACTIVE_SET() Error: ' + str(e))

    def MGMT_PENDING_GET(self, Addr='', TLVs=[]):
        """send MGMT_PENDING_GET command

        Returns:
            True: successful to send MGMT_PENDING_GET
            False: fail to send MGMT_PENDING_GET
        """
        print '%s call MGMT_PENDING_GET' % self.port
        try:
            cmd = WPANCTL_CMD + 'dataset mgmt-get-pending'

            if len(TLVs) != 0:
                tlvs = "".join(hex(tlv).lstrip("0x").zfill(2) for tlv in TLVs)
                setTLVCmd = WPANCTL_CMD + 'setprop Dataset:RawTlvs ' + tlvs
                if self.__sendCommand(setTLVCmd)[0] == 'Fail':
                    return False
            else:
                if self.__sendCommand(WPANCTL_CMD + 'dataset erase')[0] == 'Fail':
                    return False

            if Addr != '':
                setAddressCmd = WPANCTL_CMD + 'setprop Dataset:DestIpAddress ' + Addr
                if self.__sendCommand(setAddressCmd)[0] == 'Fail':
                    return False

            print cmd

            return self.__sendCommand(cmd)[0] != 'Fail'

        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger('MGMT_PENDING_GET() Error: ' + str(e))

    def MGMT_PENDING_SET(self, sAddr='', xCommissionerSessionId=None, listPendingTimestamp=None, listActiveTimestamp=None, xDelayTimer=None,
                         xChannel=None, xPanId=None, xMasterKey=None, sMeshLocalPrefix=None, sNetworkName=None):
        """send MGMT_PENDING_SET command

        Returns:
            True: successful to send MGMT_PENDING_SET
            False: fail to send MGMT_PENDING_SET
        """
        print '%s call MGMT_PENDING_SET' % self.port
        try:
            cmd = WPANCTL_CMD + 'dataset mgmt-set-pending'
            if self.__sendCommand(WPANCTL_CMD + 'dataset erase')[0] == 'Fail':
                return False

            if listPendingTimestamp != None:
                sActiveTimestamp = str(hex(listPendingTimestamp[0]))
                if len(sActiveTimestamp) < 18:
                    sActiveTimestamp = sActiveTimestamp.lstrip('0x').zfill(16)
                setPendingTimeCmd = WPANCTL_CMD + 'setprop Dataset:PendingTimestamp ' + sActiveTimestamp
                if self.__sendCommand(setPendingTimeCmd)[0] == 'Fail':
                    return False

            if listActiveTimestamp != None:
                sActiveTimestamp = str(hex(listActiveTimestamp[0]))
                if len(sActiveTimestamp) < 18:
                    sActiveTimestamp = sActiveTimestamp.lstrip('0x').zfill(16)
                setActiveTimeCmd = WPANCTL_CMD + 'setprop Dataset:ActiveTimestamp ' + sActiveTimestamp
                if self.__sendCommand(setActiveTimeCmd)[0] == 'Fail':
                    return False

            if xDelayTimer != None:
                setDelayTimerCmd = WPANCTL_CMD + 'setprop Dataset:Delay ' + str(xDelayTimer)
                if self.__sendCommand(setDelayTimerCmd)[0] == 'Fail':
                    return False

            if sNetworkName != None:
                setNetworkNameCmd = WPANCTL_CMD + 'setprop Dataset:NetworkName ' + str(sNetworkName)
                if self.__sendCommand(setNetworkNameCmd)[0] == 'Fail':
                    return False

            if xChannel != None:
                setChannelCmd = WPANCTL_CMD + 'setprop Dataset:Channel ' + str(xChannel)
                if self.__sendCommand(setChannelCmd)[0] == 'Fail':
                    return False

            if sMeshLocalPrefix != None:
                setMLPrefixCmd = WPANCTL_CMD + 'setprop Dataset:MeshLocalPrefix ' + str(sMeshLocalPrefix)
                if self.__sendCommand(setMLPrefixCmd)[0] == 'Fail':
                    return False

            if xMasterKey != None:
                key = self.__convertLongToString(xMasterKey)

                if len(key) < 32:
                    key = key.zfill(32)

                setMasterKeyCmd = WPANCTL_CMD + 'setprop Dataset:MasterKey ' + key
                if self.__sendCommand(setMasterKeyCmd)[0] == 'Fail':
                    return False

            if xPanId != None:
                setPanIdCmd = WPANCTL_CMD + 'setprop Dataset:PanId ' + str(xPanId)
                if self.__sendCommand(setPanIdCmd)[0] == 'Fail':
                    return False

            if xCommissionerSessionId != None:
               print 'not handle xCommissionerSessionId'

            print cmd

            return self.__sendCommand(cmd)[0] != 'Fail'

        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger('MGMT_PENDING_SET() Error: ' + str(e))

    def MGMT_COMM_GET(self, Addr='ff02::1', TLVs=[]):
        """send MGMT_COMM_GET command

        Returns:
            True: successful to send MGMT_COMM_GET
            False: fail to send MGMT_COMM_GET
        """
        print '%s call MGMT_COMM_GET' % self.port
        try:
            cmd = WPANCTL_CMD + 'commissioner mgmt-get '
            print 'TLVs:'
            print TLVs

            if len(TLVs) != 0:
                tlvs = "".join(hex(tlv).lstrip("0x").zfill(2) for tlv in TLVs)
                cmd += tlvs

            print cmd

            return self.__sendCommand(cmd)[0] != 'Fail'

        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger('MGMT_COMM_GET() Error: ' + str(e))

    def MGMT_COMM_SET(self, Addr='ff02::1', xCommissionerSessionID=None, xSteeringData=None, xBorderRouterLocator=None,
                      xChannelTlv=None, ExceedMaxPayload=False):
        """send MGMT_COMM_SET command

        Returns:
            True: successful to send MGMT_COMM_SET
            False: fail to send MGMT_COMM_SET
        """
        print '%s call MGMT_COMM_SET' % self.port
        try:
            cmd = WPANCTL_CMD + 'commissioner mgmt-set '
            print "-------------------------------"
            print xCommissionerSessionID
            print xSteeringData
            print str(xSteeringData) + '   ' + str(hex(xSteeringData)[2:])
            print xBorderRouterLocator
            print xChannelTlv
            print ExceedMaxPayload
            print "-------------------------------"

            if xCommissionerSessionID != None:
                # use assigned session id
                cmd += '0b02' + str(xCommissionerSessionID)
            elif xCommissionerSessionID is None:
                # use original session id
                if self.isActiveCommissioner is True:
                    cmd += '0b02' + self.__getCommissionerSessionId().lstrip('0x')
                else:
                    pass

            if xSteeringData != None:
                cmd += '08' + str(len(hex(xSteeringData)[2:])) + str(hex(xSteeringData)[2:])

            if xBorderRouterLocator != None:
                cmd += '0902' + str(hex(xBorderRouterLocator))

            if xChannelTlv != None:
                cmd += '000300' + hex(xChannelTlv).lstrip('0x').zfill(4)

            print cmd

            return self.__sendCommand(cmd)[0] != 'Fail'

        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger('MGMT_COMM_SET() Error: ' + str(e))

    def setActiveDataset(self, listActiveDataset=[]):
        print '%s call setActiveDataset' % self.port

    def setCommisionerMode(self):
        print '%s call setCommissionerMode' % self.port

    def setPSKc(self, strPSKc):
        print '%s call setPSKc' % self.port
        try:
            cmd = WPANCTL_CMD + 'setprop Network:PSKc %s' % strPSKc
            datasetCmd =  WPANCTL_CMD + 'setprop Dataset:PSKc %s' % strPSKc
            self.hasActiveDatasetToCommit = True
            return self.__sendCommand(cmd)[0] != 'Fail' and self.__sendCommand(datasetCmd)[0] != 'Fail'
        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger('setPSKc() Error: ' + str(e))

    def setActiveTimestamp(self, xActiveTimestamp):
        print '%s call setActiveTimestamp' % self.port
        try:
            sActiveTimestamp = str(xActiveTimestamp)
            if len(sActiveTimestamp) < 16:
                sActiveTimestamp = sActiveTimestamp.zfill(16)
            self.activetimestamp = sActiveTimestamp
            cmd = WPANCTL_CMD + 'setprop Dataset:ActiveTimestamp %s' % sActiveTimestamp
            self.hasActiveDatasetToCommit = True
            return self.__sendCommand(cmd)[0] != 'Fail'
        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger('setActiveTimestamp() Error: ' + str(e))

    def setUdpJoinerPort(self, portNumber):
        """set Joiner UDP Port

        Args:
            portNumber: Joiner UDP Port number

        Returns:
            True: successful to set Joiner UDP Port
            False: fail to set Joiner UDP Port

        @todo : required if as reference device
        """
        pass

    def commissionerUnregister(self):
        """stop commissioner

        Returns:
            True: successful to stop commissioner
            False: fail to stop commissioner
        """
        print '%s call commissionerUnregister' % self.port
        cmd = WPANCTL_CMD + 'commissioner stop'
        print cmd
        if self.__sendCommand(cmd)[0] != 'Fail':
            self.isActiveCommissioner = False
            return True
        else:
            return False

    def sendBeacons(self, sAddr, xCommissionerSessionId, listChannelMask, xPanId):
        print '%s call sendBeacons' % self.port
        self._sendline(WPANCTL_CMD + 'scan')
        return True

    def updateRouterStatus(self):
        """force update to router as if there is child id request
            @todo : required if as reference device
        """
        pass

    def setRouterThresholdValues(self, upgradeThreshold, downgradeThreshold):
        print '%s call setRouterThresholdValues' % self.port
        self.__setRouterUpgradeThreshold(upgradeThreshold)
        self.__setRouterDowngradeThreshold(downgradeThreshold)

    def setMinDelayTimer(self, iSeconds):
       pass

    def ValidateDeviceFirmware(self):
        print '%s call ValidateDeviceFirmware' % self.port
        if "OPENTHREAD" in self.UIStatusMsg:
           return True
        else:
           return False
