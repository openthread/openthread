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
import subprocess


class Node:

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
        print("%s" % cmd)

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
            cmd = 'python %s/tools/spinel-cli/spinel-cli.py -p %s/examples/apps/ncp/ot-ncp -n' % (srcdir, builddir)
        else:
            cmd = './ot-ncp'
        cmd += ' %d' % nodeid
        print("%s" % cmd)

        self.pexpect = pexpect.spawn(cmd, timeout=4)
        time.sleep(0.2)
        self.pexpect.expect('spinel-cli >')
        self.debug(int(os.getenv('DEBUG', '0')))

    def __init_soc(self, nodeid):
        """ Initialize a System-on-a-chip node connected via UART. """
        import fdpexpect
        serialPort = '/dev/ttyUSB%d' % ((nodeid - 1) * 2)
        self.pexpect = fdpexpect.fdspawn(os.open(serialPort, os.O_RDWR | os.O_NONBLOCK | os.O_NOCTTY))

    def __del__(self):
        if self.pexpect.isalive():
            self.send_command('exit')
            self.pexpect.expect(pexpect.EOF)
            self.pexpect.terminate()
            self.pexpect.close(force=True)

    @classmethod
    def setUp(klass, count):
        if "top_builddir" in os.environ.keys():
            srcdir = os.environ['top_builddir']
        else:
            srcdir = os.path.dirname(os.path.realpath(__file__))
            srcdir += "/../../.."

        if "NO_BTVIRT_SETUP" not in os.environ.keys():

            btvirt = "/usr/local/bin/btvirt -l%d -L" % (count + 1)

            print "SETUP: %s" % (btvirt)

            if (os.geteuid() != 0):
                raise Exception("ERROR: sudo required to run " + btvirt)

            if not hasattr(klass, 'hciconfig'):
                print btvirt.split(' ')
                klass.hciconfig = subprocess.Popen(btvirt.split(' '))

        nodes = {}
        for i in range(1, count + 1):
            nodes[i] = Node(i)
        return nodes

    @classmethod
    def tearDown(klass):
        if hasattr(klass, 'hciconfig'):
            klass.hciconfig.kill()
            klass.hciconfig.wait()
            klass.hciconfig = None

    def send_command(self, cmd):
        print("%d: %s" % (self.nodeid, cmd))
        self.pexpect.sendline(cmd)

    def get_commands(self):
        self.send_command('?')
        self.pexpect.expect('Commands:')
        commands = []
        while True:
            i = self.pexpect.expect(['Done', r'(\S+)'])
            if i != 0:
                commands.append(self.pexpect.match.groups()[0])
            else:
                break
        return commands

    def ble_start(self):
        self.send_command('ble start')
        self.pexpect.expect('Done')

    def ble_stop(self):
        self.send_command('ble stop')
        self.pexpect.expect('Done')

    def ble_adv_start(self, interval):
        self.send_command('ble adv start ' + str(interval))
        self.pexpect.expect('Done')

    def ble_adv_stop(self):
        self.send_command('ble adv stop')
        self.pexpect.expect('Done')

    def ble_adv_data(self, adv_data):
        self.send_command('ble adv data ' + adv_data)
        self.pexpect.expect('Done')

    def ble_scan_start(self):
        self.send_command('ble scan start')
        self.pexpect.expect('Done')

    def ble_scan_stop(self):
        self.send_command('ble scan stop')
        self.pexpect.expect('Done')

    def ble_scan_response(self, scan_rsp):
        self.send_command('ble scan rsp ' + scan_rsp)
        self.pexpect.expect('Done')

    def ble_conn_start(self, addr, addr_type=0):
        self.send_command('ble conn start ' + addr + ' ' + str(addr_type))
        self.pexpect.expect('Done')

    def ble_conn_stop(self):
        self.send_command('ble conn stop')
        self.pexpect.expect('Done')

    def ble_conn_params(self):
        self.send_command('ble conn params')
        self.pexpect.expect('Done')

    def ble_get_bdaddr(self):
        self.send_command('ble bdaddr')
        i = self.pexpect.expect('([0-9a-fA-F]{12})')
        if i == 0:
            bdaddr = self.pexpect.match.groups()[0].decode("utf-8")
        i = self.pexpect.expect(r'\[([0-9]{1})\]')
        if i == 0:
            bdtype = self.pexpect.match.groups()[0].decode("utf-8")
            self.pexpect.expect('Done')
        return bdaddr, bdtype

    def ble_gatt_discover_services(self):
        self.send_command('ble gatt discover services')
        self.pexpect.expect('Done')

    def ble_gatt_discover_service(self, uuid):
        self.send_command('ble gatt discover service ' + str(uuid))
        self.pexpect.expect('Done')

    def ble_gatt_discover_characteristic(self, start_handle, end_handle):
        self.send_command('ble gatt disc char %s %s' % (str(start_handle), str(end_handle)))
        self.pexpect.expect('Done')

    def ble_gatt_discover_descriptor(self, start_handle, end_handle):
        self.send_command('ble gatt disc desc %s %s' % (str(start_handle), str(end_handle)))
        self.pexpect.expect('Done')

    def ble_gatt_read(self, handle):
        self.send_command('ble gatt read ' + str(handle))
        self.pexpect.expect('Done')

    def ble_gatt_write(self, handle, value):
        self.send_command('ble gatt write %s %s' % (str(handle), value))
        self.pexpect.expect('Done')

    def ble_ch_start(self, psm):
        self.send_command('ble ch start %d' % (psm))
        self.pexpect.expect('Done')

    def ble_ch_stop(self):
        self.send_command('ble ch stop')
        self.pexpect.expect('Done')

    def ble_ch_tx(self, data):
        self.send_command('ble ch tx %s' % (data))
        self.pexpect.expect('Done')
