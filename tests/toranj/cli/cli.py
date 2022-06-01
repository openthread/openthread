#!/usr/bin/env python3
#
#  Copyright (c) 2021, The OpenThread Authors.
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

import sys
import os
import time
import re
import random
import string
import subprocess
import pexpect
import pexpect.popen_spawn
import signal
import inspect
import weakref

# ----------------------------------------------------------------------------------------------------------------------
# Constants

JOIN_TYPE_ROUTER = 'router'
JOIN_TYPE_END_DEVICE = 'ed'
JOIN_TYPE_SLEEPY_END_DEVICE = 'sed'

# ----------------------------------------------------------------------------------------------------------------------


def _log(text, new_line=True, flush=True):
    sys.stdout.write(text)
    if new_line:
        sys.stdout.write('\n')
    if flush:
        sys.stdout.flush()


# ----------------------------------------------------------------------------------------------------------------------
# CliError class


class CliError(Exception):

    def __init__(self, error_code, message):
        self._error_code = error_code
        self._message = message

    @property
    def error_code(self):
        return self._error_code

    @property
    def message(self):
        return self._message


# ----------------------------------------------------------------------------------------------------------------------
# Node class


class Node(object):
    """ An OT CLI instance """

    # defines the default verbosity setting (can be changed per `Node`)
    _VERBOSE = os.getenv('TORANJ_VERBOSE', 'no').lower() in ['true', '1', 't', 'y', 'yes', 'on']

    _SPEED_UP_FACTOR = 1  # defines the default time speed up factor

    # Determine whether to save logs in a file.
    _SAVE_LOGS = True

    # name of  log file (if _SAVE_LOGS is `True`)
    _LOG_FNAME = 'ot-logs'

    _OT_BUILDDIR = os.getenv('top_builddir', '../../..')

    _OT_CLI_FTD = '%s/examples/apps/cli/ot-cli-ftd' % _OT_BUILDDIR

    _WAIT_TIME = 10

    _START_INDEX = 1
    _cur_index = _START_INDEX

    _all_nodes = weakref.WeakSet()

    def __init__(self, verbose=_VERBOSE):
        """Creates a new `Node` instance"""

        index = Node._cur_index
        Node._cur_index += 1

        self._index = index
        self._verbose = verbose

        if Node._SAVE_LOGS:
            self._log_file = open(self._LOG_FNAME + str(index) + '.log', 'wb')
        else:
            self._log_file = None

        cmd = f'{self._OT_CLI_FTD} --time-speed={self._SPEED_UP_FACTOR} {self._index}'

        if self._verbose:
            _log(f'$ Node{index}.__init__() cmd: `{cmd}`')

        self._cli_process = pexpect.popen_spawn.PopenSpawn(cmd, logfile=self._log_file)
        Node._all_nodes.add(self)

    def __del__(self):
        self._finalize()

    def __repr__(self):
        return f'Node(index={self._index})'

    @property
    def index(self):
        return self._index

    # ------------------------------------------------------------------------------------------------------------------
    # Executing a `cli` command

    def cli(self, *args):
        """ Issues a CLI command on the given node and returns the resulting output.

            The returned result is a list of strings (with `\r\n` removed) as outputted by the CLI.
            If executing the command fails, `CliError` is raised with error code and error message.
        """

        cmd = ' '.join([f'{arg}' for arg in args if arg is not None]).strip()

        if self._verbose:
            _log(f'$ Node{self._index}.cli(\'{cmd}\')', new_line=False)

        self._cli_process.send(cmd + '\n')
        index = self._cli_process.expect(['(.*)Done\r\n', '.*Error (\d+):(.*)\r\n'])

        if index == 0:
            result = [
                line for line in self._cli_process.match.group(1).decode().splitlines()
                if not self._is_ot_logg_line(line) if not line.strip().endswith(cmd)
            ]

            if self._verbose:
                if len(result) > 1:
                    _log(':')
                    for line in result:
                        _log('     ' + line)
                elif len(result) == 1:
                    _log(f' -> {result[0]}')
                else:
                    _log('')

            return result
        else:
            match = self._cli_process.match
            e = CliError(int(match.group(1).decode()), match.group(2).decode().strip())
            if self._verbose:
                _log(f': Error {e.message} ({e.error_code})')
            raise e

    def _is_ot_logg_line(self, line):
        return any(level in line for level in [' [D] ', ' [I] ', ' [N] ', ' [W] ', ' [C] ', ' [-] '])

    def _cli_no_output(self, cmd, *args):
        outputs = self.cli(cmd, *args)
        verify(len(outputs) == 0)

    def _cli_single_output(self, cmd, expected_outputs=None):
        outputs = self.cli(cmd)
        verify(len(outputs) == 1)
        verify((expected_outputs is None) or (outputs[0] in expected_outputs))
        return outputs[0]

    def _finalize(self):
        if self._cli_process.proc.poll() is None:
            if self._verbose:
                _log(f'$ Node{self.index} terminating')
            self._cli_process.send('exit\n')
            self._cli_process.wait()

    # ------------------------------------------------------------------------------------------------------------------
    # cli commands

    def get_state(self):
        return self._cli_single_output('state', ['detached', 'child', 'router', 'leader', 'disabled'])

    def get_channel(self):
        return self._cli_single_output('channel')

    def set_channel(self, channel):
        self._cli_no_output('channel', channel)

    def get_ext_addr(self):
        return self._cli_single_output('extaddr')

    def set_ext_addr(self, ext_addr):
        self._cli_no_output('extaddr', ext_addr)

    def get_ext_panid(self):
        return self._cli_single_output('extpanid')

    def set_ext_panid(self, ext_panid):
        self._cli_no_output('extpanid', ext_panid)

    def get_mode(self):
        return self._cli_single_output('mode')

    def set_mode(self, mode):
        self._cli_no_output('mode', mode)

    def get_network_key(self):
        return self._cli_single_output('networkkey')

    def set_network_key(self, networkkey):
        self._cli_no_output('networkkey', networkkey)

    def get_network_name(self):
        return self._cli_single_output('networkname')

    def set_network_name(self, network_name):
        self._cli_no_output('networkname', network_name)

    def get_panid(self):
        return self._cli_single_output('panid')

    def set_panid(self, panid):
        self._cli_no_output('panid', panid)

    def get_router_upgrade_threshold(self):
        return self._cli_single_output('routerupgradethreshold')

    def set_router_upgrade_threshold(self, threshold):
        self._cli_no_output('routerupgradethreshold', threshold)

    def get_router_selection_jitter(self):
        return self._cli_single_output('routerselectionjitter')

    def set_router_selection_jitter(self, jitter):
        self._cli_no_output('routerselectionjitter', jitter)

    def interface_up(self):
        self._cli_no_output('ifconfig up')

    def interface_down(self):
        self._cli_no_output('ifconfig down')

    def get_interface_state(self):
        return self._cli_single_output('ifconfig')

    def thread_start(self):
        self._cli_no_output('thread start')

    def thread_stop(self):
        self._cli_no_output('thread stop')

    def get_ip_addrs(self):
        return self.cli('ipaddr')

    def get_mleid_ip_addr(self):
        return self._cli_single_output('ipaddr mleid')

    def get_linklocal_ip_addr(self):
        return self._cli_single_output('ipaddr linklocal')

    def get_rloc_ip_addr(self):
        return self._cli_single_output('ipaddr rloc')

    #- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    # SRP client

    def srp_client_start(self, server_address, server_port):
        self._cli_no_output('srp client start', server_address, server_port)

    def srp_client_stop(self):
        self._cli_no_output('srp client stop')

    def srp_client_get_state(self):
        return self._cli_single_output('srp client state', ['Enabled', 'Disabled'])

    def srp_client_get_auto_start_mode(self):
        return self._cli_single_output('srp client autostart', ['Enabled', 'Disabled'])

    def srp_client_enable_auto_start_mode(self):
        self._cli_no_output('srp client autostart enable')

    def srp_client_disable_auto_start_mode(self):
        self._cli_no_output('srp client autostart disable')

    def srp_client_get_server_address(self):
        return self._cli_single_output('srp client server address')

    def srp_client_get_server_port(self):
        return self._cli_single_output('srp client server port')

    def srp_client_get_host_state(self):
        return self._cli_single_output('srp client host state')

    def srp_client_set_host_name(self, name):
        self._cli_no_output('srp client host name', name)

    def srp_client_get_host_name(self):
        return self._cli_single_output('srp client host name')

    def srp_client_remove_host(self, remove_key=False, send_unreg_to_server=False):
        self._cli_no_output('srp client host remove', int(remove_key), int(send_unreg_to_server))

    def srp_client_clear_host(self):
        self._cli_no_output('srp client host clear')

    def srp_client_set_host_address(self, *addrs):
        self._cli_no_output('srp client host address', *addrs)

    def srp_client_get_host_address(self):
        return self.cli('srp client host address')

    def srp_client_add_service(self, instance_name, service_name, port, priority=0, weight=0, txt_entries=[]):
        txt_record = "".join(self._encode_txt_entry(entry) for entry in txt_entries)
        self._cli_no_output('srp client service add', instance_name, service_name, port, priority, weight, txt_record)

    def srp_client_remove_service(self, instance_name, service_name):
        self._cli_no_output('srp client service remove', instance_name, service_name)

    def srp_client_clear_service(self, instance_name, service_name):
        self._cli_no_output('srp client service clear', instance_name, service_name)

    def srp_client_get_services(self):
        outputs = self.cli('srp client service')
        return [self._parse_srp_client_service(line) for line in outputs]

    def _encode_txt_entry(self, entry):
        """Encodes the TXT entry to the DNS-SD TXT record format as a HEX string.

           Example usage:
           self._encode_txt_entries(['abc'])     -> '03616263'
           self._encode_txt_entries(['def='])    -> '046465663d'
           self._encode_txt_entries(['xyz=XYZ']) -> '0778797a3d58595a'
        """
        return '{:02x}'.format(len(entry)) + "".join("{:02x}".format(ord(c)) for c in entry)

    def _parse_srp_client_service(self, line):
        """Parse one line of srp service list into a dictionary which
           maps string keys to string values.

           Example output for input
           'instance:\"%s\", name:\"%s\", state:%s, port:%d, priority:%d, weight:%d"'
           {
               'instance': 'my-service',
               'name': '_ipps._udp',
               'state': 'ToAdd',
               'port': '12345',
               'priority': '0',
               'weight': '0'
           }

           Note that value of 'port', 'priority' and 'weight' are represented
           as strings but not integers.
        """
        key_values = [word.strip().split(':') for word in line.split(', ')]
        return {key_value[0].strip(): key_value[1].strip('"') for key_value in key_values}

    #- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    # SRP server

    def srp_server_get_state(self):
        return self._cli_single_output('srp server state', ['disabled', 'running', 'stopped'])

    def srp_server_get_addr_mode(self):
        return self._cli_single_output('srp server addrmode', ['unicast', 'anycast'])

    def srp_server_set_addr_mode(self, mode):
        self._cli_no_output('srp server addrmode', mode)

    def srp_server_get_anycast_seq_num(self):
        return self._cli_single_output('srp server seqnum')

    def srp_server_set_anycast_seq_num(self, seqnum):
        self._cli_no_output('srp server seqnum', seqnum)

    def srp_server_enable(self):
        self._cli_no_output('srp server enable')

    def srp_server_disable(self):
        self._cli_no_output('srp server disable')

    def srp_server_set_lease(self, min_lease, max_lease, min_key_lease, max_key_lease):
        self._cli_no_output('srp server lease', min_lease, max_lease, min_key_lease, max_key_lease)

    def srp_server_get_hosts(self):
        """Returns the host list on the SRP server as a list of property
           dictionary.

           Example output:
           [{
               'fullname': 'my-host.default.service.arpa.',
               'name': 'my-host',
               'deleted': 'false',
               'addresses': ['2001::1', '2001::2']
           }]
        """
        outputs = self.cli('srp server host')
        host_list = []
        while outputs:
            host = {}
            host['fullname'] = outputs.pop(0).strip()
            host['name'] = host['fullname'].split('.')[0]
            host['deleted'] = outputs.pop(0).strip().split(':')[1].strip()
            if host['deleted'] == 'true':
                host_list.append(host)
                continue
            addresses = outputs.pop(0).strip().split('[')[1].strip(' ]').split(',')
            map(str.strip, addresses)
            host['addresses'] = [addr for addr in addresses if addr]
            host_list.append(host)
        return host_list

    def srp_server_get_host(self, host_name):
        """Returns host on the SRP server that matches given host name.

           Example usage:
           self.srp_server_get_host("my-host")
        """
        for host in self.srp_server_get_hosts():
            if host_name == host['name']:
                return host

    def srp_server_get_services(self):
        """Returns the service list on the SRP server as a list of property
           dictionary.

           Example output:
           [{
               'fullname': 'my-service._ipps._tcp.default.service.arpa.',
               'instance': 'my-service',
               'name': '_ipps._tcp',
               'deleted': 'false',
               'port': '12345',
               'priority': '0',
               'weight': '0',
               'ttl': '7200',
               'TXT': ['abc=010203'],
               'host_fullname': 'my-host.default.service.arpa.',
               'host': 'my-host',
               'addresses': ['2001::1', '2001::2']
           }]

           Note that the TXT data is output as a HEX string.
        """
        outputs = self.cli('srp server service')
        service_list = []
        while outputs:
            service = {}
            service['fullname'] = outputs.pop(0).strip()
            name_labels = service['fullname'].split('.')
            service['instance'] = name_labels[0]
            service['name'] = '.'.join(name_labels[1:3])
            service['deleted'] = outputs.pop(0).strip().split(':')[1].strip()
            if service['deleted'] == 'true':
                service_list.append(service)
                continue
            # 'subtypes', port', 'priority', 'weight', 'ttl'
            for i in range(0, 5):
                key_value = outputs.pop(0).strip().split(':')
                service[key_value[0].strip()] = key_value[1].strip()
            txt_entries = outputs.pop(0).strip().split('[')[1].strip(' ]').split(',')
            txt_entries = map(str.strip, txt_entries)
            service['TXT'] = [txt for txt in txt_entries if txt]
            service['host_fullname'] = outputs.pop(0).strip().split(':')[1].strip()
            service['host'] = service['host_fullname'].split('.')[0]
            addresses = outputs.pop(0).strip().split('[')[1].strip(' ]').split(',')
            addresses = map(str.strip, addresses)
            service['addresses'] = [addr for addr in addresses if addr]
            service_list.append(service)
        return service_list

    def srp_server_get_service(self, instance_name, service_name):
        """Returns service on the SRP server that matches given instance
           name and service name.

           Example usage:
           self.srp_server_get_service("my-service", "_ipps._tcp")
        """
        for service in self.srp_server_get_services():
            if (instance_name == service['instance'] and service_name == service['name']):
                return service

    # ------------------------------------------------------------------------------------------------------------------
    # Helper methods

    def form(self, network_name=None, network_key=None, channel=None, panid=0x1234, xpanid=None):
        if network_name is not None:
            self.set_network_name(network_name)
        if network_key is not None:
            self.set_network_key(network_key)
        if channel is not None:
            self.set_channel(channel)
        if xpanid is not None:
            self.set_ext_panid(xpanid)
        self.set_panid(panid)
        self.interface_up()
        self.thread_start()
        verify_within(_check_node_is_leader, self._WAIT_TIME, arg=self)

    def join(self, node, type=JOIN_TYPE_ROUTER):
        self.set_network_name(node.get_network_name())
        self.set_network_key(node.get_network_key())
        self.set_channel(node.get_channel())
        self.set_panid(node.get_panid())
        if type == JOIN_TYPE_END_DEVICE:
            self.set_mode('rn')
        elif type == JOIN_TYPE_SLEEPY_END_DEVICE:
            self.set_mode('-')
        else:
            self.set_mode('rdn')
            self.set_router_selection_jitter(1)
        self.interface_up()
        self.thread_start()
        if type == JOIN_TYPE_ROUTER:
            verify_within(_check_node_is_router, self._WAIT_TIME, arg=self)
        else:
            verify_within(_check_node_is_child, self._WAIT_TIME, arg=self)

    def allowlist_node(self, node):
        """Adds a given node to the allowlist of `self` and enables allowlisting on `self`"""
        self._cli_no_output('macfilter addr add', node.get_ext_addr())
        self._cli_no_output('macfilter addr allowlist')

    def un_allowlist_node(self, node):
        """Removes a given node (of node `Node) from the allowlist"""
        self._cli_no_output('macfilter addr remove', node.get_ext_addr())

    # ------------------------------------------------------------------------------------------------------------------
    # class methods

    @classmethod
    def finalize_all_nodes(cls):
        """Finalizes all previously created `Node` instances (stops the CLI process)"""
        for node in Node._all_nodes:
            node._finalize()

    @classmethod
    def set_time_speedup_factor(cls, factor):
        """Sets up the time speed up factor - should be set before creating any `Node` objects"""
        if len(Node._all_nodes) != 0:
            raise Node._NodeError('set_time_speedup_factor() cannot be called after creating a `Node`')
        Node._SPEED_UP_FACTOR = factor


def _check_node_is_leader(node):
    verify(node.get_state() == 'leader')


def _check_node_is_router(node):
    verify(node.get_state() == 'router')


def _check_node_is_child(node):
    verify(node.get_state() == 'child')


# ----------------------------------------------------------------------------------------------------------------------


class VerifyError(Exception):
    pass


_is_in_verify_within = False


def verify(condition):
    """Verifies that a `condition` is true, otherwise raises a VerifyError"""
    global _is_in_verify_within
    if not condition:
        calling_frame = inspect.currentframe().f_back
        error_message = 'verify() failed at line {} in "{}"'.format(calling_frame.f_lineno,
                                                                    calling_frame.f_code.co_filename)
        if not _is_in_verify_within:
            print(error_message)
        raise VerifyError(error_message)


def verify_within(condition_checker_func, wait_time, arg=None, delay_time=0.1):
    """Verifies that a given function `condition_checker_func` passes successfully within a given wait timeout.
       `wait_time` is maximum time waiting for condition_checker to pass (in seconds).
       `arg` is optional parameter and if it s not None, will be passed to `condition_checker_func()`
       `delay_time` specifies a delay interval added between failed attempts (in seconds).
    """
    global _is_in_verify_within
    start_time = time.time()
    old_is_in_verify_within = _is_in_verify_within
    _is_in_verify_within = True
    while True:
        try:
            if arg is None:
                condition_checker_func()
            else:
                condition_checker_func(arg)
        except VerifyError as e:
            if time.time() - start_time > wait_time:
                print('Took too long to pass the condition ({}>{} sec)'.format(time.time() - start_time, wait_time))
                print(e.message)
                raise e
        except BaseException:
            raise
        else:
            break
        if delay_time != 0:
            time.sleep(delay_time)
    _is_in_verify_within = old_is_in_verify_within
