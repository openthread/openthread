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

"""
>> Thread Host Controller Interface
>> Device : OpenThread THCI
>> Class : OpenThread
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

LINESEPX = re.compile(r'\r\n|\n')
"""regex: used to split lines"""

class OpenThread(IThci):
    LOWEST_POSSIBLE_PARTATION_ID = 0x1
    LINK_QUALITY_CHANGE_TIME = 100

    # Used for reference firmware version control for Test Harness.
    # This variable will be updated to match the OpenThread reference firmware officially released.
    firmwarePrefix = "OPENTHREAD/"

    #def __init__(self, SerialPort=COMPortName, EUI=MAC_Address):
    def __init__(self, **kwargs):
        """initialize the serial port and default network parameters
        Args:
            **kwargs: Arbitrary keyword arguments
                      Includes 'EUI' and 'SerialPort'
        """
        try:
            self.UIStatusMsg = ''
            self.mac = kwargs.get('EUI')
            self.port = kwargs.get('SerialPort')
            self.handle = None
            self.AutoDUTEnable = False
            self._is_net = False                  # whether device is through ser2net
            self.logStatus = {'stop':'stop', 'running':'running', "pauseReq":'pauseReq', 'paused':'paused'}
            self.joinStatus = {'notstart':'notstart', 'ongoing':'ongoing', 'succeed':'succeed', "failed":'failed'}
            self.logThreadStatus = self.logStatus['stop']
            self.intialize()
        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger("initialize() Error: " + str(e))

    def __del__(self):
        """close the serial port connection"""
        try:
            self.closeConnection()
            self.deviceConnected = False
        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger("delete() Error: " + str(e))

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

        # wait for write to complete
        time.sleep(0.1)

    def __sendCommand(self, cmd):
        """send specific command to reference unit over serial port

        Args:
            cmd: OpenThread CLI string

        Returns:
            Done: successfully send the command to reference unit and parse it
            Value: successfully retrieve the desired value from reference unit
            Error: some errors occur, indicates by the followed specific error number
        """
        logging.info('%s: sendCommand[%s]', self.port, cmd)
        if self.logThreadStatus == self.logStatus['running']:
            self.logThreadStatus = self.logStatus['pauseReq']
            while self.logThreadStatus != self.logStatus['paused'] and self.logThreadStatus != self.logStatus['stop']:
                pass

        try:
            # command retransmit times
            retry_times = 3
            while retry_times > 0:
                retry_times -= 1
                try:
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
            retry_times = 10
            while retry_times > 0:
                line = self._readline()
                logging.info('%s: the read line is[%s]', self.port, line)
                if line:
                    response.append(line)
                    if line == 'Done':
                        break
                else:
                    retry_times -= 1
                    time.sleep(0.2)
            if line != 'Done':
                raise Exception('%s: failed to find end of response' % self.port)
            logging.info('%s: send command[%s] done!', self.port, cmd)
            return response
        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger("sendCommand() Error: " + str(e))
            raise

    def __getIp6Address(self, addressType):
        """get specific type of IPv6 address configured on thread device

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

        addrs = self.__sendCommand('ipaddr')
        for ip6Addr in addrs:
            if ip6Addr == 'Done':
                break

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
            else:
                # global ipv6 address
                if ip6Addr != None:
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
           mode: thread device mode
           r: rx-on-when-idle
           s: secure IEEE 802.15.4 data request
           d: full thread device
           n: full network data

        Returns:
           True: successful to set the device mode
           False: fail to set the device mode
        """
        print 'call __setDeviceMode'
        print mode
        try:
            cmd = 'mode %s' % mode
            return self.__sendCommand(cmd)[0] == 'Done'
        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger("setDeviceMode() Error: " + str(e))

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
            cmd = 'routerupgradethreshold %s' % str(iThreshold)
            print cmd
            return self.__sendCommand(cmd) == 'Done'
        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger("setRouterUpgradeThreshold() Error: " + str(e))

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
            cmd = 'routerdowngradethreshold %s' % str(iThreshold)
            print cmd
            return self.__sendCommand(cmd) == 'Done'
        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger("setRouterDowngradeThreshold() Error: " + str(e))

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
            cmd = 'routerselectionjitter %s' % str(iRouterJitter)
            print cmd
            return self.__sendCommand(cmd) == 'Done'
        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger("setRouterSelectionJitter() Error: " + str(e))

    def __setAddressfilterMode(self, mode):
        """set address filter mode

        Returns:
            True: successful to set address filter mode.
            False: fail to set address filter mode.
        """
        print 'call setAddressFilterMode() ' +  mode
        try:
            cmd = 'macfilter addr ' + mode
            if self.__sendCommand(cmd)[0] == 'Done':
                return True
            return False
        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger("__setAddressFilterMode() Error: " + str(e))

    def __startOpenThread(self):
        """start OpenThread stack

        Returns:
            True: successful to start OpenThread stack and thread interface up
            False: fail to start OpenThread stack
        """
        print 'call startOpenThread'
        try:
            if self.hasActiveDatasetToCommit:
                if self.__sendCommand('dataset commit active')[0] != 'Done':
                    raise Exception('failed to commit active dataset')
                else:
                    self.hasActiveDatasetToCommit = False

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

            if self.deviceRole in [Thread_Device_Role.Leader, Thread_Device_Role.Router, Thread_Device_Role.REED]:
                self.__setRouterSelectionJitter(1)

            if self.__sendCommand('ifconfig up')[0] == 'Done':
                if self.__sendCommand('thread start')[0] == 'Done':
                    self.isPowerDown = False
                    return True
            else:
                return False
        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger("startOpenThread() Error: " + str(e))

    def __stopOpenThread(self):
        """stop OpenThread stack

        Returns:
            True: successful to stop OpenThread stack and thread interface down
            False: fail to stop OpenThread stack
        """
        print 'call stopOpenThread'
        try:
            if self.__sendCommand('thread stop')[0] == 'Done':
                return self.__sendCommand('ifconfig down')[0] == 'Done'
            else:
                return False
        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger("stopOpenThread() Error: " + str(e))

    def __isOpenThreadRunning(self):
        """check whether or not OpenThread is running

        Returns:
            True: OpenThread is running
            False: OpenThread is not running
        """
        print 'call isOpenThreadRunning'
        return self.__sendCommand('state')[0] != 'disabled'

    # rloc16 might be hex string or integer, need to return actual allocated router id
    def __convertRlocToRouterId(self, xRloc16):
        """mapping Rloc16 to router id

        Args:
            xRloc16: hex rloc16 short address

        Returns:
            actual router id allocated by leader
        """
        routerList = []
        routerList = self.__sendCommand('router list')[0].split()
        print routerList
        print xRloc16

        for index in routerList:
            router = []
            cmd = 'router %s' % index
            router = self.__sendCommand(cmd)

            for line in router:
                if 'Done' in line:
                    break
                elif 'Router ID' in line:
                    routerid = line.split()[2]
                elif 'Rloc' in line:
                    rloc16 = line.split()[1]
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
           2001000000000000 -> 2001::

        Args:
            strIp6Prefix: IPv6 address string

        Returns:
            IPv6 address dotted-quad format
        """
        prefix1 = strIp6Prefix.rstrip('L')
        prefix2 = prefix1.lstrip("0x")
        hexPrefix = str(prefix2).ljust(16,'0')
        hexIter = iter(hexPrefix)
        finalMac = ':'.join(a + b + c + d for a,b,c,d in zip(hexIter, hexIter,hexIter,hexIter))
        prefix = str(finalMac)
        strIp6Prefix = prefix[:20]
        return strIp6Prefix +':'

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

    def __readCommissioningLogs(self, durationInSeconds):
        """read logs during the commissioning process

        Args:
            durationInSeconds: time duration for reading commissioning logs

        Returns:
            Commissioning logs
        """
        self.logThreadStatus = self.logStatus['running']
        logs = Queue()
        t_end = time.time() + durationInSeconds
        while time.time() < t_end:
            time.sleep(0.3)

            if self.logThreadStatus == self.logStatus['pauseReq']:
                self.logThreadStatus = self.logStatus['paused']

            if self.logThreadStatus != self.logStatus['running']:
                continue

            try:
                line = self._readline()
                if line:
                    print line
                    logs.put(line)

                    if "Join success" in line:
                        self.joinCommissionedStatus = self.joinStatus['succeed']
                        break
                    elif "Join failed" in line:
                        self.joinCommissionedStatus = self.joinStatus['failed']
                        break

            except Exception:
                pass

        self.logThreadStatus = self.logStatus['stop']
        return logs

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

    def __setChannelMask(self, channelMask):
        print 'call _setChannelMask'
        try:
            cmd = 'dataset channelmask %s' % channelMask
            self.hasActiveDatasetToCommit = True
            return self.__sendCommand(cmd) == 'Done'
        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger("setChannelMask() Error: " + str(e))

    def __setSecurityPolicy(self, securityPolicySecs, securityPolicyFlags):
        print 'call _setSecurityPolicy'
        try:
            cmd = 'dataset securitypolicy %s %s' % (str(securityPolicySecs), securityPolicyFlags)
            self.hasActiveDatasetToCommit = True
            return self.__sendCommand(cmd) == 'Done'
        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger("setSecurityPolicy() Error: " + str(e))

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
            cmd = 'keysequence guardtime %s' % str(iKeySwitchGuardTime)
            if self.__sendCommand(cmd)[0] == 'Done':
                time.sleep(1)
                return True
            else:
                return False
        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger("setKeySwitchGuardTime() Error; " + str(e))

    def __getCommissionerSessionId(self):
        """ get the commissioner session id allocated from Leader """
        print '%s call getCommissionerSessionId' % self.port
        return self.__sendCommand('commissioner sessionid')[0]

    def _connect(self):
        print 'My port is %s' % self.port
        if self.port.startswith('COM'):
            self.handle = serial.Serial(self.port, 115200, timeout=0)
            time.sleep(1)
            self.handle.write('\r\n')
            time.sleep(0.1)
            self._is_net = False
        elif ':' in self.port:
            host, port = self.port.split(':')
            self.handle = socket.create_connection((host, port))
            self.handle.setblocking(0)
            self._is_net = True
        else:
            raise Exception('Unknown port schema')
        self.UIStatusMsg = self.getVersionNumber()

    def closeConnection(self):
        """close current serial port connection"""
        print '%s call closeConnection' % self.port
        try:
            if self.handle:
                self.handle.close()
                self.handle = None
        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger("closeConnection() Error: " + str(e))

    def intialize(self):
        """initialize the serial port with baudrate, timeout parameters"""
        print '%s call intialize' % self.port
        try:
            self.deviceConnected = False

            # init serial port
            self._connect()

            if self.firmwarePrefix in self.UIStatusMsg:
                self.deviceConnected = True
            else:
                self.UIStatusMsg = "Firmware Not Matching Expecting " + self.firmwarePrefix + " Now is " + self.UIStatusMsg
                ModuleHelper.WriteIntoDebugLogger("Err: OpenThread device Firmware not matching..")

        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger("intialize() Error: " + str(e))
            self.deviceConnected = False

    def setNetworkName(self, networkName='GRL'):
        """set Thread Network name

        Args:
            networkName: the networkname string to be set

        Returns:
            True: successful to set the Thread Networkname
            False: fail to set the Thread Networkname
        """
        print '%s call setNetworkName' % self.port
        print networkName
        try:
            cmd = 'networkname %s' % networkName
            datasetCmd = 'dataset networkname %s' % networkName
            self.hasActiveDatasetToCommit = True
            return self.__sendCommand(cmd)[0] == 'Done' and self.__sendCommand(datasetCmd)[0] == 'Done'
        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger("setNetworkName() Error: " + str(e))

    def getNetworkName(self):
        """get Thread Network name"""
        print '%s call getNetworkname' % self.port
        return self.__sendCommand('networkname')[0]

    def setChannel(self, channel=11):
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
        print channel
        try:
            cmd = 'channel %s' % channel
            datasetCmd = 'dataset channel %s' % channel
            self.hasActiveDatasetToCommit = True
            return self.__sendCommand(cmd)[0] == 'Done' and self.__sendCommand(datasetCmd)[0] == 'Done'
        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger("setChannel() Error: " + str(e))

    def getChannel(self):
        """get current channel"""
        print '%s call getChannel' % self.port
        return self.__sendCommand('channel')[0]

    def setMAC(self, xEUI):
        """set the extended addresss of Thread device

        Args:
            xEUI: extended address in hex format

        Returns:
            True: successful to set the extended address
            False: fail to set the extended address
        """
        print '%s call setMAC' % self.port
        print xEUI
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

            cmd = 'extaddr %s' % address64
            print cmd
            if self.__sendCommand(cmd)[0] == 'Done':
                self.mac = address64
                return True
            else:
                return False
        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger("setMAC() Error: " + str(e))

    def getMAC(self, bType=MacType.RandomMac):
        """get one specific type of MAC address
           currently OpenThread only supports Random MAC address

        Args:
            bType: indicate which kind of MAC address is required

        Returns:
            specific type of MAC address
        """
        print '%s call getMAC' % self.port
        print bType
        # if power down happens, return extended address assigned previously
        if self.isPowerDown:
            macAddr64 = self.mac
        else:
            if bType == MacType.FactoryMac:
                macAddr64 = self.__sendCommand('eui64')[0]
            elif bType == MacType.HashMac:
                macAddr64 = self.__sendCommand('joinerid')[0]
            else:
                macAddr64 = self.__sendCommand('extaddr')[0]
        print macAddr64

        return int(macAddr64, 16)

    def getLL64(self):
        """get link local unicast IPv6 address"""
        print '%s call getLL64' % self.port
        return self.__getIp6Address('link local')

    def getMLEID(self):
        """get mesh local endpoint identifier address"""
        print '%s call getMLEID' % self.port
        return self.__getIp6Address('mesh EID')

    def getRloc16(self):
        """get rloc16 short address"""
        print '%s call getRloc16' % self.port
        rloc16 = self.__sendCommand('rloc16')[0]
        return int(rloc16, 16)

    def getRloc(self):
        """get router locator unicast IPv6 address"""
        print '%s call getRloc' % self.port
        return self.__getIp6Address('rloc')

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
        print key
        try:
            if not isinstance(key, str):
                masterKey = self.__convertLongToString(key)

                # prpend '0' at the beginning
                if len(masterKey) < 32:
                    masterKey = masterKey.zfill(32)
                    print masterKey

                cmd = 'masterkey %s' %masterKey
                datasetCmd = 'dataset masterkey %s' % masterKey
            else:
                masterKey = key
                cmd = 'masterkey %s' % masterKey
                datasetCmd = 'dataset masterkey %s' % masterKey

            self.networkKey = masterKey
            self.hasActiveDatasetToCommit = True
            return self.__sendCommand(cmd)[0] == 'Done' and self.__sendCommand(datasetCmd)[0] == 'Done'
        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger("setNetworkkey() Error: " + str(e))

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
                if self.__setAddressfilterMode('blacklist'):
                    self._addressfilterMode = 'blacklist'

            cmd = 'macfilter addr add %s' % macAddr
            print cmd
            ret = self.__sendCommand(cmd)[0] == 'Done'

            self._addressfilterSet.add(macAddr)
            print 'current blacklist entries:'
            for addr in self._addressfilterSet:
                print addr

            return ret
        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger("addBlockedMAC() Error: " + str(e))

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
                if self.__setAddressfilterMode('whitelist'):
                    self._addressfilterMode = 'whitelist'

            cmd = 'macfilter addr add %s' % macAddr
            print cmd
            ret = self.__sendCommand(cmd)[0] == 'Done'

            self._addressfilterSet.add(macAddr)
            print 'current whitelist entries:'
            for addr in self._addressfilterSet:
                print addr
            return ret

        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger("addAllowMAC() Error: " + str(e))

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
                cmd = 'macfilter addr clear'
                if self.__sendCommand(cmd)[0] == 'Done':
                    self._addressfilterSet.clear()
                    return True
            return False
        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger("clearBlockList() Error: " + str(e))

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
                cmd = 'macfilter addr clear'
                if self.__sendCommand(cmd)[0] == 'Done':
                    self._addressfilterSet.clear()
                    return True
            return False
        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger("clearAllowList() Error: " + str(e))

    def getDeviceRole(self):
        """get current device role in Thread Network"""
        print '%s call getDeviceRole' % self.port
        return self.__sendCommand('state')[0]

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
        mode = ''
        try:
            if ModuleHelper.LeaderDutChannelFound:
                self.channel = ModuleHelper.Default_Channel

            # FIXME: when Harness call setNetworkDataRequirement()?
            # only sleep end device requires stable networkdata now
            if eRoleId == Thread_Device_Role.Leader:
                print 'join as leader'
                mode = 'rsdn'
                if self.AutoDUTEnable is False:
                    # set ROUTER_DOWNGRADE_THRESHOLD
                    self.__setRouterDowngradeThreshold(33)
            elif eRoleId == Thread_Device_Role.Router:
                print 'join as router'
                mode = 'rsdn'
                if self.AutoDUTEnable is False:
                    # set ROUTER_DOWNGRADE_THRESHOLD
                    self.__setRouterDowngradeThreshold(33)
            elif eRoleId == Thread_Device_Role.SED:
                print 'join as sleepy end device'
                mode = 's'
                self.setPollingRate(self.sedPollingRate)
            elif eRoleId == Thread_Device_Role.EndDevice:
                print 'join as end device'
                mode = 'rsn'
            elif eRoleId == Thread_Device_Role.REED:
                print 'join as REED'
                mode = 'rsdn'
                # set ROUTER_UPGRADE_THRESHOLD
                self.__setRouterUpgradeThreshold(0)
            elif eRoleId == Thread_Device_Role.EndDevice_FED:
                # always remain an ED, never request to be a router
                print 'join as FED'
                mode = 'rsdn'
                # set ROUTER_UPGRADE_THRESHOLD
                self.__setRouterUpgradeThreshold(0)
            elif eRoleId == Thread_Device_Role.EndDevice_MED:
                print 'join as MED'
                mode = 'rsn'
            else:
                pass

            # set Thread device mode with a given role
            self.__setDeviceMode(mode)
            self.__setKeySwitchGuardTime(0)

            # start OpenThread
            self.__startOpenThread()
            time.sleep(3)

            return True
        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger("joinNetwork() Error: " + str(e))

    def getNetworkFragmentID(self):
        """get current partition id of Thread Network Partition from LeaderData

        Returns:
            The Thread network Partition Id
        """
        print '%s call getNetworkFragmentID' % self.port
        if not self.__isOpenThreadRunning():
            print 'OpenThread is not running'
            return None

        leaderData = []
        leaderData = self.__sendCommand('leaderdata')
        return int(leaderData[0].split()[2], 16)

    def getParentAddress(self):
        """get Thread device's parent extended address and rloc16 short address

        Returns:
            The extended address of parent in hex format
        """
        print '%s call getParentAddress' % self.port
        parentInfo = []
        parentInfo = self.__sendCommand('parent')

        for line in parentInfo:
            if 'Done' in line:
                break
            elif 'Ext Addr' in line:
                eui = line.split()[2]
                print eui
            #elif 'Rloc' in line:
            #    rloc16 = line.split()[1]
            #    print rloc16
            else:
                pass

        return int(eui, 16)

    def powerDown(self):
        """power down the Thread device"""
        print '%s call powerDown' % self.port
        self._sendline('reset')
        self.isPowerDown = True

    def powerUp(self):
        """power up the Thread device"""
        print '%s call powerUp' % self.port
        if not self.handle:
            self._connect()

        self.isPowerDown = False

        if not self.__isOpenThreadRunning():
            self.__startOpenThread()

    def reboot(self):
        """reset and rejoin to Thread Network without any timeout

        Returns:
            True: successful to reset and rejoin the Thread Network
            False: fail to reset and rejoin the Thread Network
        """
        print '%s call reboot' % self.port
        try:
            self._sendline('reset')
            self.isPowerDown = True
            time.sleep(3)

            self.__startOpenThread()
            time.sleep(3)

            if self.__sendCommand('state')[0] == 'disabled':
                print '[FAIL] reboot'
                return False
            else:
                return True
        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger("reboot() Error: " + str(e))

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
            cmd = 'ping %s %s' % (destination, str(length))
            print cmd
            self._sendline(cmd)
            self._expect(cmd)
            # wait echo reply
            time.sleep(1)
        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger("ping() Error: " + str(e))

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
            cmd = 'ping %s %s' % (destination, str(length))
            print cmd
            self._sendline(cmd)
            self._expect(cmd)
            # wait echo reply
            time.sleep(1)
        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger("multicast_ping() Error: " + str(e))

    def getVersionNumber(self):
        """get OpenThread stack firmware version number"""
        print '%s call getVersionNumber' % self.port
        return self.__sendCommand('version')[0]

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

            cmd = 'panid %s' % panid
            datasetCmd = 'dataset panid %s' % panid
            self.hasActiveDatasetToCommit = True
            return self.__sendCommand(cmd)[0] == 'Done' and self.__sendCommand(datasetCmd)[0] == 'Done'
        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger("setPANID() Error: " + str(e))

    def getPANID(self):
        """get current Thread Network PAN ID"""
        print '%s call getPANID' % self.port
        return self.__sendCommand('panid')[0]

    def reset(self):
        """factory reset"""
        print '%s call reset' % self.port
        try:
            self._sendline('factoryreset')
            self._read()

        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger("reset() Error: " + str(e))

    def removeRouter(self, xRouterId):
        """kickoff router with a given router id from the Thread Network

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
            return self.__sendCommand(cmd)[0] == 'Done'
        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger("removeRouter() Error: " + str(e))

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
        self.joinCommissionedStatus = self.joinStatus['notstart']
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
            ModuleHelper.WriteIntoDebugLogger("setDefaultValue() Error: " + str(e))

    def getDeviceConncetionStatus(self):
        """check if serial port connection is ready or not"""
        print '%s call getDeviceConnectionStatus' % self.port
        return self.deviceConnected

    def getPollingRate(self):
        """get data polling rate for sleepy end device"""
        print '%s call getPollingRate' % self.port
        sPollingRate = self.__sendCommand('pollperiod')[0]
        try:
            iPollingRate = int(sPollingRate)/1000
            fPollingRate = round(float(sPollingRate)/1000, 3)
            return fPollingRate if fPollingRate > iPollingRate else iPollingRate
        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger("getPollingRate() Error: " + str(e))

    def setPollingRate(self, iPollingRate):
        """set data polling rate for sleepy end device

        Args:
            iPollingRate: data poll period of sleepy end device

        Returns:
            True: successful to set the data polling rate for sleepy end device
            False: fail to set the data polling rate for sleepy end device
        """
        print '%s call setPollingRate' % self.port
        # convert s to ms
        iPollingRate *= 1000
        print int(iPollingRate)
        try:
            cmd = 'pollperiod %d' % int(iPollingRate)
            print cmd
            return self.__sendCommand(cmd)[0] == 'Done'
        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger("setPollingRate() Error: " + str(e))

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
        """
        print '%s call setLinkQuality' % self.port
        print EUIadr
        print LinkQuality
        try:
            # process EUIadr
            euiHex = hex(EUIadr)
            euiStr = str(euiHex)
            euiStr = euiStr.rstrip('L')
            address64 = ''
            if '0x' in euiStr:
                address64 = euiStr.lstrip('0x')
                # prepend 0 at the beginning
                if len(address64) < 16:
                   address64 = address64.zfill(16)
                   print address64

            cmd = 'macfilter rss add-lqi %s %s' % (address64, str(LinkQuality))
            print cmd
            return self.__sendCommand(cmd)[0] == 'Done'
        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger("setLinkQuality() Error: " + str(e))

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
        """
        print '%s call setOutBoundLinkQuality' % self.port
        print LinkQuality
        try:
            cmd = 'macfilter rss add-lqi * %s' % str(LinkQuality)
            print cmd
            return self.__sendCommand(cmd)[0] == 'Done'
        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger("setOutBoundLinkQuality() Error: " + str(e))

    def removeRouterPrefix(self, prefixEntry):
        """remove the configured prefix on a border router

        Args:
            prefixEntry: a on-mesh prefix entry

        Returns:
            True: successful to remove the prefix entry from border router
            False: fail to remove the prefix entry from border router
        """
        print '%s call removeRouterPrefix' % self.port
        print prefixEntry
        prefix = self.__convertIp6PrefixStringToIp6Address(str(prefixEntry))
        try:
            prefixLen = 64
            cmd = 'prefix remove %s/%d' % (prefix, prefixLen)
            print cmd
            if self.__sendCommand(cmd)[0] == 'Done':
                # send server data ntf to leader
                return self.__sendCommand('netdataregister')[0] == 'Done'
            else:
                return False
        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger("removeRouterPrefix() Error: " + str(e))

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
            self._sendline('reset')
            self.isPowerDown = True
            time.sleep(timeout)

            if self.deviceRole == Thread_Device_Role.SED:
                self.setPollingRate(self.sedPollingRate)

            self.__startOpenThread()
            time.sleep(3)

            if self.__sendCommand('state')[0] == 'disabled':
                print '[FAIL] reset and rejoin'
                return False
            return True
        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger("resetAndRejoin() Error: " + str(e))

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
            prf = ''

            if P_slaac_preferred == 1:
                parameter += 'p'
                parameter += 'a'

            if P_stable == 1:
                parameter += 's'

            if P_default == 1:
                parameter += 'r'

            if P_Dhcp == 1:
                parameter += 'd'

            if P_on_mesh == 1:
                parameter += 'o'

            if P_preference == 1:
                prf = 'high'
            elif P_preference == 0:
                prf = 'med'
            elif P_preference == -1:
                prf = 'low'
            else:
                pass

            cmd = 'prefix add %s/64 %s %s' % (prefix, parameter, prf)
            print cmd
            if self.__sendCommand(cmd)[0] == 'Done':
                # if prefix configured before starting OpenThread stack
                # do not send out server data ntf pro-actively
                if not self.__isOpenThreadRunning():
                    return True
                else:
                    # send server data ntf to leader
                    return self.__sendCommand('netdataregister')[0] == 'Done'
            else:
                return False
        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger("configBorderRouter() Error: " + str(e))

    def setNetworkIDTimeout(self, iNwkIDTimeOut):
        """set networkid timeout for Thread device

        Args:
            iNwkIDTimeOut: a given NETWORK_ID_TIMEOUT

        Returns:
            True: successful to set NETWORK_ID_TIMEOUT
            False: fail to set NETWORK_ID_TIMEOUT
        """
        print '%s call setNetworkIDTimeout' % self.port
        print iNwkIDTimeOut
        iNwkIDTimeOut /= 1000
        try:
            cmd = 'networkidtimeout %s' % str(iNwkIDTimeOut)
            print cmd
            return self.__sendCommand(cmd)[0] == 'Done'
        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger("setNetworkIDTimeout() Error: " + str(e))

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
        iTimeOut *= 1000
        print int(iTimeOut)
        try:
            cmd = 'pollperiod %d' % int(iTimeOut)
            print cmd
            return self.__sendCommand(cmd)[0] == 'Done'
        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger("setKeepAliveTimeOut() Error: " + str(e))

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
            cmd = 'keysequence counter %s' % str(iKeySequenceValue)
            if self.__sendCommand(cmd)[0] == 'Done':
                time.sleep(1)
                return True
            else:
                return False
        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger("setKeySequenceCounter() Error; " + str(e))

    def getKeySequenceCounter(self):
        """get current Thread Network key sequence"""
        print '%s call getKeySequenceCounter' % self.port
        keySequence = ''
        keySequence = self.__sendCommand('keysequence counter')[0]
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
            ModuleHelper.WriteIntoDebugLogger("incrementKeySequenceCounter() Error: " + str(e))

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
            if R_Preference == 1:
                prf = 'high'
            elif R_Preference == 0:
                prf = 'med'
            elif R_Preference == -1:
                prf = 'low'
            else:
                pass

            if P_stable:
                stable += 's'
                cmd = 'route add %s/64 %s %s' % (prefix, stable, prf)
            else:
                cmd = 'route add %s/64 %s' % (prefix, prf)
            print cmd

            if self.__sendCommand(cmd)[0] == 'Done':
                # send server data ntf to leader
                return self.__sendCommand('netdataregister')[0] == 'Done'
        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger("configExternalRouter() Error: " + str(e))

    def getNeighbouringRouters(self):
        """get neighboring routers information

        Returns:
            neighboring routers' extended address
        """
        print '%s call getNeighbouringRouters' % self.port
        try:
            routerInfo = []
            routerList = []
            routerList = self.__sendCommand('router list')[0].split()
            print routerList

            if 'Done' in routerList:
                print 'no neighbouring routers'
                return None

            for index in routerList:
                router = []
                cmd = 'router %s' % index
                router = self.__sendCommand(cmd)

                for line in router:
                    if 'Done' in line:
                        break
                    #elif 'Rloc' in line:
                    #    rloc16 = line.split()[1]
                    elif 'Ext Addr' in line:
                        eui = line.split()[2]
                        routerInfo.append(int(eui, 16))
                    #elif 'LQI In' in line:
                    #    lqi_in = line.split()[1]
                    #elif 'LQI Out' in line:
                    #    lqi_out = line.split()[1]
                    else:
                        pass

            print routerInfo
            return routerInfo
        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger("getNeighbouringDevice() Error: " + str(e))

    def getChildrenInfo(self):
        """get all children information

        Returns:
            children's extended address
        """
        print '%s call getChildrenInfo' % self.port
        try:
            childrenInfoAll = []
            childrenInfo = {'EUI': 0, 'Rloc16': 0, 'MLEID': ''}
            childrenList = self.__sendCommand('child list')[0].split()
            print childrenList

            if 'Done' in childrenList:
                print 'no children'
                return None

            for index in childrenList:
                cmd = 'child %s' % index
                child = []
                child = self.__sendCommand(cmd)

                for line in child:
                    if 'Done' in line:
                        break
                    elif 'Rloc' in line:
                        rloc16 = line.split()[1]
                    elif 'Ext Addr' in line:
                        eui = line.split()[2]
                    #elif 'Child ID' in line:
                    #    child_id = line.split()[2]
                    #elif 'Mode' in line:
                    #    mode = line.split()[1]
                    else:
                        pass

                childrenInfo['EUI'] = int(eui, 16)
                childrenInfo['Rloc16'] = int(rloc16, 16)
                #children_info['MLEID'] = self.getMLEID()

                childrenInfoAll.append(childrenInfo['EUI'])
                #childrenInfoAll.append(childrenInfo)

            print childrenInfoAll
            return childrenInfoAll
        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger("getChildrenInfo() Error: " + str(e))

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
                    cmd = 'extpanid %s' % xpanid
                    datasetCmd = 'dataset extpanid %s' % xpanid
            else:
                xpanid = xPanId
                cmd = 'extpanid %s' % xpanid
                datasetCmd = 'dataset extpanid %s' % xpanid

            self.xpanId = xpanid
            self.hasActiveDatasetToCommit = True
            return self.__sendCommand(cmd)[0] == 'Done' and self.__sendCommand(datasetCmd)[0] == 'Done'
        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger("setXpanId() Error: " + str(e))

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

        cmd = 'leaderpartitionid %s' %(str(hex(partationId)).rstrip('L'))
        print cmd
        return self.__sendCommand(cmd)[0] == 'Done'

    def getGUA(self, filterByPrefix=None):
        """get expected global unicast IPv6 address of Thread device

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
                return globalAddrs[0]
            else:
                for line in globalAddrs:
                    fullIp = ModuleHelper.GetFullIpv6Address(line)
                    if fullIp.startswith(filterByPrefix):
                        return fullIp
                print 'no global address matched'
                return str(globalAddrs[0])
        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger("getGUA() Error: " + str(e))

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
            cmd = 'dataset meshlocalprefix %s' % sMeshLocalPrefix
            self.hasActiveDatasetToCommit = True
            return self.__sendCommand(cmd)[0] == 'Done'
        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger("setMLPrefix() Error: " + str(e))

    def getML16(self):
        """get mesh local 16 unicast address (Rloc)"""
        print '%s call getML16' % self.port
        return self.getRloc()

    def downgradeToDevice(self):
        pass

    def upgradeToRouter(self):
        pass

    def forceSetSlaac(self, slaacAddress):
        """force to set a slaac IPv6 address to Thread interface

        Args:
            slaacAddress: a slaac IPv6 address to be set

        Returns:
            True: successful to set slaac address to Thread interface
            False: fail to set slaac address to Thread interface
        """
        print '%s call forceSetSlaac' % self.port
        print slaacAddress
        try:
            cmd = 'ipaddr add %s' % str(slaacAddress)
            print cmd
            return self.__sendCommand(cmd)[0] == 'Done'
        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger("forceSetSlaac() Error: " + str(e))

    def setSleepyNodePollTime(self):
        pass

    def enableAutoDUTObjectFlag(self):
        """set AutoDUTenable flag"""
        print '%s call enableAutoDUTObjectFlag' % self.port
        self.AutoDUTEnable = True

    def getChildTimeoutValue(self):
        """get child timeout"""
        print '%s call getChildTimeoutValue' % self.port
        childTimeout = self.__sendCommand('childtimeout')[0]
        return int(childTimeout)

    def diagnosticGet(self, strDestinationAddr, listTLV_ids=[]):
        if not listTLV_ids:
            return

        if not len(listTLV_ids):
            return

        cmd = 'networkdiagnostic get %s %s' % (strDestinationAddr, ' '.join([str(tlv) for tlv in listTLV_ids]))
        print cmd

        return self._sendline(cmd)

    def diagnosticReset(self, strDestinationAddr, listTLV_ids=[]):
        if not listTLV_ids:
            return

        if not len(listTLV_ids):
            return

        cmd = 'networkdiagnostic reset %s %s' % (strDestinationAddr, ' '.join([str(tlv) for tlv in listTLV_ids]))
        print cmd

        return self.__sendCommand(cmd)

    def diagnosticQuery(self,strDestinationAddr, listTLV_ids = []):
        self.diagnosticGet(strDestinationAddr, listTLV_ids)

    def startNativeCommissioner(self, strPSKc='GRLpassWord'):
        #TODO: Support the whole Native Commissioner functionality
        #      Currently it only aims to trigger a Discovery Request message to pass Certification test 5.8.4
        print '%s call startNativeCommissioner' % self.port
        self.__sendCommand('ifconfig up')
        cmd = 'joiner start %s' %(strPSKc)
        print cmd
        if self.__sendCommand(cmd)[0] == "Done":
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
        if self.__startOpenThread():
            time.sleep(20)
            cmd = 'commissioner start'
            print cmd
            if self.__sendCommand(cmd)[0] == 'Done':
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

        # long timeout value to avoid automatic joiner removal (in seconds)
        timeout = 500

        if not isinstance(xEUI, str):
            eui64 = self.__convertLongToString(xEUI)

            # prepend 0 at the beginning
            if len(eui64) < 16:
                eui64 = eui64.zfill(16)
                print eui64
        else:
            eui64 = xEUI

        cmd = 'commissioner joiner add %s %s %s' % (eui64, strPSKd, str(timeout))
        print cmd
        if self.__sendCommand(cmd)[0] == 'Done':
            if self.logThreadStatus == self.logStatus['stop']:
                self.logThread = ThreadRunner.run(target=self.__readCommissioningLogs, args=(120,))
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
            cmd = 'commissioner provisioningurl %s' %(strURL)
            return self.__sendCommand(cmd)[0] == "Done"
        return True

    def allowCommission(self):
        """start commissioner candidate petition process

        Returns:
            True: successful to start commissioner candidate petition process
            False: fail to start commissioner candidate petition process
        """
        print '%s call allowCommission' % self.port
        try:
            cmd = 'commissioner start'
            print cmd
            if self.__sendCommand(cmd)[0] == 'Done':
                self.isActiveCommissioner = True
                time.sleep(40) # time for petition process and at least one keep alive
                return True
            else:
                return False
        except Exception, e:
            ModuleHelper.writeintodebuglogger("allowcommission() error: " + str(e))

    def joinCommissioned(self, strPSKd='threadjpaketest', waitTime=20):
        """start joiner

        Args:
            strPSKd: Joiner's PSKd

        Returns:
            True: successful to start joiner
            False: fail to start joiner
        """
        print '%s call joinCommissioned' % self.port
        self.__sendCommand('ifconfig up')
        cmd = 'joiner start %s %s' %(strPSKd, self.provisioningUrl)
        print cmd
        if self.__sendCommand(cmd)[0] == "Done":
            maxDuration = 150 # seconds
            self.joinCommissionedStatus = self.joinStatus['ongoing']

            if self.logThreadStatus == self.logStatus['stop']:
                self.logThread = ThreadRunner.run(target=self.__readCommissioningLogs, args=(maxDuration,))

            t_end = time.time() + maxDuration
            while time.time() < t_end:
                if self.joinCommissionedStatus == self.joinStatus['succeed']:
                    break
                elif self.joinCommissionedStatus == self.joinStatus['failed']:
                    return False

                time.sleep(1)

            self.__sendCommand('thread start')
            time.sleep(30)
            return True
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
        channelMask = '0x' + self.__convertLongToString(self.__convertChannelMask(listChannelMask))
        try:
            cmd = 'commissioner energy %s %s %s %s %s' % (channelMask, xCount, xPeriod, xScanDuration, sAddr)
            print cmd
            return self.__sendCommand(cmd) == 'Done'
        except Exception, e:
            ModuleHelper.writeintodebuglogger("MGMT_ED_SCAN() error: " + str(e))

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
        channelMask = '0x' + self.__convertLongToString(self.__convertChannelMask(listChannelMask))

        if not isinstance(xPanId, str):
            panid = str(hex(xPanId))

        try:
            cmd = 'commissioner panid %s %s %s' % (panid, channelMask, sAddr)
            print cmd
            return self.__sendCommand(cmd) == 'Done'
        except Exception, e:
            ModuleHelper.writeintodebuglogger("MGMT_PANID_QUERY() error: " + str(e))

    def MGMT_ANNOUNCE_BEGIN(self, sAddr, xCommissionerSessionId, listChannelMask, xCount, xPeriod):
        """send MGMT_ANNOUNCE_BEGIN message to a given destination

        Returns:
            True: successful to send MGMT_ANNOUNCE_BEGIN message.
            False: fail to send MGMT_ANNOUNCE_BEGIN message.
        """
        print '%s call MGMT_ANNOUNCE_BEGIN' % self.port
        channelMask = ''
        channelMask = '0x' + self.__convertLongToString(self.__convertChannelMask(listChannelMask))
        try:
            cmd = 'commissioner announce %s %s %s %s' % (channelMask, xCount, xPeriod, sAddr)
            print cmd
            return self.__sendCommand(cmd) == 'Done'
        except Exception, e:
            ModuleHelper.writeintodebuglogger("MGMT_ANNOUNCE_BEGIN() error: " + str(e))

    def MGMT_ACTIVE_GET(self, Addr='', TLVs=[]):
        """send MGMT_ACTIVE_GET command

        Returns:
            True: successful to send MGMT_ACTIVE_GET
            False: fail to send MGMT_ACTIVE_GET
        """
        print '%s call MGMT_ACTIVE_GET' % self.port
        try:
            cmd = 'dataset mgmtgetcommand active'

            if Addr != '':
                cmd += ' address '
                cmd += Addr

            if len(TLVs) != 0:
                tlvs = "".join(hex(tlv).lstrip("0x").zfill(2) for tlv in TLVs)
                cmd += ' binary '
                cmd += tlvs

            print cmd

            return self.__sendCommand(cmd)[0] == 'Done'

        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger("MGMT_ACTIVE_GET() Error: " + str(e))

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
            cmd = 'dataset mgmtsetcommand active'

            if listActiveTimestamp != None:
                cmd += ' activetimestamp '
                cmd += str(listActiveTimestamp[0])

            if xExtendedPanId != None:
                cmd += ' extpanid '
                xpanid = self.__convertLongToString(xExtendedPanId)

                if len(xpanid) < 16:
                    xpanid = xpanid.zfill(16)

                cmd += xpanid

            if sNetworkName != None:
                cmd += ' networkname '
                cmd += str(sNetworkName)

            if xChannel != None:
                cmd += ' channel '
                cmd += str(xChannel)

            if sMeshLocalPrefix != None:
                cmd += ' localprefix '
                cmd += str(sMeshLocalPrefix)

            if xMasterKey != None:
                cmd += ' masterkey '
                key = self.__convertLongToString(xMasterKey)

                if len(key) < 32:
                    key = key.zfill(32)

                cmd += key

            if xPanId != None:
                cmd += ' panid '
                cmd += str(xPanId)

            if listChannelMask != None:
                cmd += ' channelmask '
                cmd += '0x' + self.__convertLongToString(self.__convertChannelMask(listChannelMask))

            if sPSKc != None or listSecurityPolicy != None or \
               xCommissioningSessionId != None or xTmfPort != None or xSteeringData != None or xBorderRouterLocator != None or \
               BogusTLV != None:
                cmd += ' binary '

            if sPSKc != None:
                cmd += '0410'
                stretchedPskc = Thread_PBKDF2.get(sPSKc,ModuleHelper.Default_XpanId,ModuleHelper.Default_NwkName)
                pskc = hex(stretchedPskc).rstrip('L').lstrip('0x')

                if len(pskc) < 32:
                    pskc = pskc.zfill(32)

                cmd += pskc

            if listSecurityPolicy != None:
                cmd += '0c03'

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

                cmd += policy

                cmd += str(hex(policyBits))[2:]

            if xCommissioningSessionId != None:
                cmd += '0b02'
                sessionid = str(hex(xCommissioningSessionId))[2:]

                if len(sessionid) < 4:
                    sessionid = sessionid.zfill(4)

                cmd += sessionid

            if xBorderRouterLocator != None:
                cmd += '0902'
                locator = str(hex(xBorderRouterLocator))[2:]

                if len(locator) < 4:
                    locator = locator.zfill(4)

                cmd += locator

            if xSteeringData != None:
                steeringData = self.__convertLongToString(xSteeringData)
                cmd += '08' + str(len(steeringData)/2).zfill(2)
                cmd += steeringData

            if BogusTLV != None:
                cmd += "8202aa55"

            print cmd

            return self.__sendCommand(cmd)[0] == 'Done'

        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger("MGMT_ACTIVE_SET() Error: " + str(e))

    def MGMT_PENDING_GET(self, Addr='', TLVs=[]):
        """send MGMT_PENDING_GET command

        Returns:
            True: successful to send MGMT_PENDING_GET
            False: fail to send MGMT_PENDING_GET
        """
        print '%s call MGMT_PENDING_GET' % self.port
        try:
            cmd = 'dataset mgmtgetcommand pending'

            if Addr != '':
                cmd += ' address '
                cmd += Addr

            if len(TLVs) != 0:
                tlvs = "".join(hex(tlv).lstrip("0x").zfill(2) for tlv in TLVs)
                cmd += ' binary '
                cmd += tlvs

            print cmd

            return self.__sendCommand(cmd)[0] == 'Done'

        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger("MGMT_PENDING_GET() Error: " + str(e))

    def MGMT_PENDING_SET(self, sAddr='', xCommissionerSessionId=None, listPendingTimestamp=None, listActiveTimestamp=None, xDelayTimer=None,
                         xChannel=None, xPanId=None, xMasterKey=None, sMeshLocalPrefix=None, sNetworkName=None):
        """send MGMT_PENDING_SET command

        Returns:
            True: successful to send MGMT_PENDING_SET
            False: fail to send MGMT_PENDING_SET
        """
        print '%s call MGMT_PENDING_SET' % self.port
        try:
            cmd = 'dataset mgmtsetcommand pending'

            if listPendingTimestamp != None:
                cmd += ' pendingtimestamp '
                cmd += str(listPendingTimestamp[0])

            if listActiveTimestamp != None:
                cmd += ' activetimestamp '
                cmd += str(listActiveTimestamp[0])

            if xDelayTimer != None:
                cmd += ' delaytimer '
                cmd += str(xDelayTimer)
                #cmd += ' delaytimer 3000000'

            if xChannel != None:
                cmd += ' channel '
                cmd += str(xChannel)

            if xPanId != None:
                cmd += ' panid '
                cmd += str(xPanId)

            if xMasterKey != None:
                cmd += ' masterkey '
                key = self.__convertLongToString(xMasterKey)

                if len(key) < 32:
                    key = key.zfill(32)

                cmd += key

            if sMeshLocalPrefix != None:
                cmd += ' localprefix '
                cmd += str(sMeshLocalPrefix)

            if sNetworkName != None:
                cmd += ' networkname '
                cmd += str(sNetworkName)

            if xCommissionerSessionId != None:
                cmd += ' binary '
                cmd += '0b02'
                sessionid = str(hex(xCommissionerSessionId))[2:]

                if len(sessionid) < 4:
                    sessionid = sessionid.zfill(4)

                cmd += sessionid

            print cmd

            return self.__sendCommand(cmd)[0] == 'Done'

        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger("MGMT_PENDING_SET() Error: " + str(e))

    def MGMT_COMM_GET(self, Addr='ff02::1', TLVs=[]):
        """send MGMT_COMM_GET command

        Returns:
            True: successful to send MGMT_COMM_GET
            False: fail to send MGMT_COMM_GET
        """
        print '%s call MGMT_COMM_GET' % self.port
        try:
            cmd = 'commissioner mgmtget'

            if len(TLVs) != 0:
                tlvs = "".join(hex(tlv).lstrip("0x").zfill(2) for tlv in TLVs)
                cmd += ' binary '
                cmd += tlvs

            print cmd

            return self.__sendCommand(cmd)[0] == 'Done'

        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger("MGMT_COMM_GET() Error: " + str(e))

    def MGMT_COMM_SET(self, Addr='ff02::1', xCommissionerSessionID=None, xSteeringData=None, xBorderRouterLocator=None,
                      xChannelTlv=None, ExceedMaxPayload=False):
        """send MGMT_COMM_SET command

        Returns:
            True: successful to send MGMT_COMM_SET
            False: fail to send MGMT_COMM_SET
        """
        print '%s call MGMT_COMM_SET' % self.port
        try:
            cmd = 'commissioner mgmtset'

            if xCommissionerSessionID != None:
                # use assigned session id
                cmd += ' sessionid '
                cmd += str(xCommissionerSessionID)
            elif xCommissionerSessionID is None:
                # use original session id
                if self.isActiveCommissioner is True:
                    cmd += ' sessionid '
                    cmd += self.__getCommissionerSessionId()
                else:
                    pass

            if xSteeringData != None:
                cmd += ' steeringdata '
                cmd += str(hex(xSteeringData)[2:])

            if xBorderRouterLocator != None:
                cmd += ' locator '
                cmd += str(hex(xBorderRouterLocator))

            if xChannelTlv != None:
                cmd += ' binary '
                cmd += '000300' + hex(xChannelTlv).lstrip('0x').zfill(4)

            print cmd

            return self.__sendCommand(cmd)[0] == 'Done'

        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger("MGMT_COMM_SET() Error: " + str(e))

    def setActiveDataset(self, listActiveDataset=[]):
        print '%s call setActiveDataset' % self.port

    def setCommisionerMode(self):
        print '%s call setCommissionerMode' % self.port

    def setPSKc(self, strPSKc):
        print '%s call setPSKc' % self.port
        try:
            cmd = 'dataset pskc %s' % strPSKc
            self.hasActiveDatasetToCommit = True
            return self.__sendCommand(cmd)[0] == 'Done'
        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger("setPSKc() Error: " + str(e))

    def setActiveTimestamp(self, xActiveTimestamp):
        print '%s call setActiveTimestamp' % self.port
        try:
            self.activetimestamp = xActiveTimestamp
            cmd = 'dataset activetimestamp %s' % str(xActiveTimestamp)
            self.hasActiveDatasetToCommit = True
            return self.__sendCommand(cmd)[0] == 'Done'
        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger("setActiveTimestamp() Error: " + str(e))

    def setUdpJoinerPort(self, portNumber):
        """set Joiner UDP Port

        Args:
            portNumber: Joiner UDP Port number

        Returns:
            True: successful to set Joiner UDP Port
            False: fail to set Joiner UDP Port
        """
        print '%s call setUdpJoinerPort' % self.port
        cmd = 'joinerport %d' % portNumber
        print cmd
        return self.__sendCommand(cmd)[0] == 'Done'

    def commissionerUnregister(self):
        """stop commissioner

        Returns:
            True: successful to stop commissioner
            False: fail to stop commissioner
        """
        print '%s call commissionerUnregister' % self.port
        cmd = 'commissioner stop'
        print cmd
        return self.__sendCommand(cmd)[0] == 'Done'

    def sendBeacons(self, sAddr, xCommissionerSessionId, listChannelMask, xPanId):
        print '%s call sendBeacons' % self.port
        self._sendline('scan')
        return True

    def updateRouterStatus(self):
        """force update to router as if there is child id request"""
        print '%s call updateRouterStatus' % self.port
        cmd = 'state'
        while True:
            state = self.__sendCommand(cmd)[0]
            if state == 'detached':
                continue
            elif state == 'child':
                break
            else:
                return False

        cmd = 'state router'
        return self.__sendCommand(cmd)[0] == 'Done'

    def setRouterThresholdValues(self, upgradeThreshold, downgradeThreshold):
        print '%s call setRouterThresholdValues' % self.port
        self.__setRouterUpgradeThreshold(upgradeThreshold)
        self.__setRouterDowngradeThreshold(downgradeThreshold)

    def setMinDelayTimer(self, iSeconds):
        print '%s call setMinDelayTimer' % self.port
        cmd = 'delaytimermin %s' % iSeconds
        print cmd
        return self.__sendCommand(cmd)[0] == 'Done'

    def ValidateDeviceFirmware(self):
        print '%s call ValidateDeviceFirmware' % self.port
        if "OPENTHREAD" in self.UIStatusMsg:
           return True
        else:
           return False
