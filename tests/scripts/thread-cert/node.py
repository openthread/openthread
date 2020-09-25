#!/usr/bin/env python3
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

import binascii
import ipaddress
import logging
import os
import re
import socket
import subprocess
import sys
import time
import traceback
import unittest
from typing import Union, Dict, Optional, List

import pexpect
import pexpect.popen_spawn

import config
import simulator

PORT_OFFSET = int(os.getenv('PORT_OFFSET', "0"))


class OtbrDocker:
    _socat_proc = None
    _ot_rcp_proc = None
    _docker_proc = None

    def __init__(self, nodeid: int, **kwargs):
        try:
            self._docker_name = config.OTBR_DOCKER_NAME_PREFIX + str(nodeid)
            self._prepare_ot_rcp_sim(nodeid)
            self._launch_docker()
        except Exception:
            traceback.print_exc()
            self.destroy()
            raise

    def _prepare_ot_rcp_sim(self, nodeid: int):
        self._socat_proc = subprocess.Popen(['socat', '-d', '-d', 'pty,raw,echo=0', 'pty,raw,echo=0'],
                                            stderr=subprocess.PIPE,
                                            stdin=subprocess.DEVNULL,
                                            stdout=subprocess.DEVNULL)

        line = self._socat_proc.stderr.readline().decode('ascii').strip()
        self._rcp_device_pty = rcp_device_pty = line[line.index('PTY is /dev') + 7:]
        line = self._socat_proc.stderr.readline().decode('ascii').strip()
        self._rcp_device = rcp_device = line[line.index('PTY is /dev') + 7:]
        logging.info(f"socat running: device PTY: {rcp_device_pty}, device: {rcp_device}")

        ot_rcp_path = self._get_ot_rcp_path()
        self._ot_rcp_proc = subprocess.Popen(f"{ot_rcp_path} {nodeid} > {rcp_device_pty} < {rcp_device_pty}",
                                             shell=True,
                                             stdin=subprocess.DEVNULL,
                                             stdout=subprocess.DEVNULL,
                                             stderr=subprocess.DEVNULL)

    def _get_ot_rcp_path(self) -> str:
        srcdir = os.environ['top_builddir']
        path = '%s/examples/apps/ncp/ot-rcp' % srcdir
        logging.info("ot-rcp path: %s", path)
        return path

    def _launch_docker(self):
        subprocess.check_call(f"docker rm -f {self._docker_name} || true", shell=True)
        CI_ENV = os.getenv('CI_ENV', '').split()
        self._docker_proc = subprocess.Popen(['docker', 'run'] + CI_ENV + [
            '--rm',
            '--name',
            self._docker_name,
            '--network',
            config.BACKBONE_DOCKER_NETWORK_NAME,
            '-i',
            '--sysctl',
            'net.ipv6.conf.all.disable_ipv6=0 net.ipv4.conf.all.forwarding=1 net.ipv6.conf.all.forwarding=1',
            '--privileged',
            '--cap-add=NET_ADMIN',
            '--volume',
            f'{self._rcp_device}:/dev/ttyUSB0',
            config.OTBR_DOCKER_IMAGE,
        ],
                                             stdin=subprocess.DEVNULL,
                                             stdout=sys.stdout,
                                             stderr=sys.stderr)

        launch_docker_deadline = time.time() + 60
        launch_ok = False

        while time.time() < launch_docker_deadline:
            try:
                subprocess.check_call(f'docker exec -i {self._docker_name} ot-ctl state', shell=True)
                launch_ok = True
                logging.info("OTBR Docker %s Is Ready!", self._docker_name)
                break
            except subprocess.CalledProcessError:
                time.sleep(0.2)
                continue

        assert launch_ok

        cmd = f'docker exec -i {self._docker_name} ot-ctl'
        self.pexpect = pexpect.popen_spawn.PopenSpawn(cmd, timeout=10)

        # Add delay to ensure that the process is ready to receive commands.
        timeout = 0.4
        while timeout > 0:
            self.pexpect.send('\r\n')
            try:
                self.pexpect.expect('> ', timeout=0.1)
                break
            except pexpect.TIMEOUT:
                timeout -= 0.1

    def __repr__(self):
        return f'OtbrDocker<{self.nodeid}>'

    def destroy(self):
        logging.info("Destroying %s", self)
        self._shutdown_docker()
        self._shutdown_ot_rcp()
        self._shutdown_socat()

    def _shutdown_docker(self):
        if self._docker_proc is not None:
            COVERAGE = int(os.getenv('COVERAGE', '0'))
            OTBR_COVERAGE = int(os.getenv('OTBR_COVERAGE', '0'))
            if COVERAGE or OTBR_COVERAGE:
                self.bash('service otbr-agent stop')

                self.bash('curl https://codecov.io/bash -o codecov_bash --retry 5')
                codecov_cmd = 'bash codecov_bash -Z'
                # Upload OTBR code coverage if OTBR_COVERAGE=1, otherwise OpenThread code coverage.
                if not OTBR_COVERAGE:
                    codecov_cmd += ' -R third_party/openthread/repo'

                self.bash(codecov_cmd)

            subprocess.check_call(f"docker rm -f {self._docker_name}", shell=True)
            self._docker_proc.wait()
            del self._docker_proc

    def _shutdown_ot_rcp(self):
        if self._ot_rcp_proc is not None:
            self._ot_rcp_proc.kill()
            self._ot_rcp_proc.wait()
            del self._ot_rcp_proc

    def _shutdown_socat(self):
        if self._socat_proc is not None:
            self._socat_proc.stderr.close()
            self._socat_proc.kill()
            self._socat_proc.wait()
            del self._socat_proc

    def bash(self, cmd: str) -> List[str]:
        logging.info("%s $ %s", self, cmd)
        proc = subprocess.Popen(['docker', 'exec', '-i', self._docker_name, 'bash', '-c', cmd],
                                stdin=subprocess.DEVNULL,
                                stdout=subprocess.PIPE,
                                stderr=sys.stderr,
                                encoding='ascii')

        with proc:

            lines = []

            while True:
                line = proc.stdout.readline()

                if not line:
                    break

                lines.append(line)
                logging.info("%s $ %s", self, line.rstrip('\r\n'))

            proc.wait()

            if proc.returncode != 0:
                raise subprocess.CalledProcessError(proc.returncode, cmd, ''.join(lines))
            else:
                return lines


class OtCli:

    def __init__(self, nodeid, is_mtd=False, version=None, is_bbr=False, **kwargs):
        self.verbose = int(float(os.getenv('VERBOSE', 0)))
        self.node_type = os.getenv('NODE_TYPE', 'sim')
        self.env_version = os.getenv('THREAD_VERSION', '1.1')
        self.is_bbr = is_bbr
        self._initialized = False

        if version is not None:
            self.version = version
        else:
            self.version = self.env_version

        mode = os.environ.get('USE_MTD') == '1' and is_mtd and 'mtd' or 'ftd'

        if self.node_type == 'soc':
            self.__init_soc(nodeid)
        elif self.node_type == 'ncp-sim':
            # TODO use mode after ncp-mtd is available.
            self.__init_ncp_sim(nodeid, 'ftd')
        else:
            self.__init_sim(nodeid, mode)

        if self.verbose:
            self.pexpect.logfile_read = sys.stdout.buffer

        self._initialized = True

    def __init_sim(self, nodeid, mode):
        """ Initialize a simulation node. """

        # Default command if no match below, will be overridden if below conditions are met.
        cmd = './ot-cli-%s' % (mode)

        # If Thread version of node matches the testing environment version.
        if self.version == self.env_version:
            # Load Thread 1.2 BBR device when testing Thread 1.2 scenarios
            # which requires device with Backbone functionality.
            if self.version == '1.2' and self.is_bbr:
                if 'OT_CLI_PATH_1_2_BBR' in os.environ:
                    cmd = os.environ['OT_CLI_PATH_1_2_BBR']
                elif 'top_builddir_1_2_bbr' in os.environ:
                    srcdir = os.environ['top_builddir_1_2_bbr']
                    cmd = '%s/examples/apps/cli/ot-cli-%s' % (srcdir, mode)

            # Load Thread device of the testing environment version (may be 1.1 or 1.2)
            else:
                if 'OT_CLI_PATH' in os.environ:
                    cmd = os.environ['OT_CLI_PATH']
                elif 'top_builddir' in os.environ:
                    srcdir = os.environ['top_builddir']
                    cmd = '%s/examples/apps/cli/ot-cli-%s' % (srcdir, mode)

            if 'RADIO_DEVICE' in os.environ:
                cmd += ' --real-time-signal=+1 -v spinel+hdlc+uart://%s?forkpty-arg=%d' % (os.environ['RADIO_DEVICE'],
                                                                                           nodeid)
            else:
                cmd += ' %d' % nodeid

        # Load Thread 1.1 node when testing Thread 1.2 scenarios for interoperability
        elif self.version == '1.1':
            # Posix app
            if 'OT_CLI_PATH_1_1' in os.environ:
                cmd = os.environ['OT_CLI_PATH_1_1']
            elif 'top_builddir_1_1' in os.environ:
                srcdir = os.environ['top_builddir_1_1']
                cmd = '%s/examples/apps/cli/ot-cli-%s' % (srcdir, mode)

            if 'RADIO_DEVICE_1_1' in os.environ:
                cmd += ' --real-time-signal=+1 -v spinel+hdlc+uart://%s?forkpty-arg=%d' % (
                    os.environ['RADIO_DEVICE_1_1'], nodeid)
            else:
                cmd += ' %d' % nodeid

        print("%s" % cmd)

        self.pexpect = pexpect.popen_spawn.PopenSpawn(cmd, timeout=10)

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

        # Default command if no match below, will be overridden if below conditions are met.
        cmd = 'spinel-cli.py -p ./ot-ncp-%s -n' % mode

        # If Thread version of node matches the testing environment version.
        if self.version == self.env_version:
            if 'RADIO_DEVICE' in os.environ:
                args = ' --real-time-signal=+1 spinel+hdlc+uart://%s?forkpty-arg=%d' % (os.environ['RADIO_DEVICE'],
                                                                                        nodeid)
            else:
                args = ''

            # Load Thread 1.2 BBR device when testing Thread 1.2 scenarios
            # which requires device with Backbone functionality.
            if self.version == '1.2' and self.is_bbr:
                if 'OT_NCP_PATH_1_2_BBR' in os.environ:
                    cmd = 'spinel-cli.py -p "%s%s" -n' % (
                        os.environ['OT_NCP_PATH_1_2_BBR'],
                        args,
                    )
                elif 'top_builddir_1_2_bbr' in os.environ:
                    srcdir = os.environ['top_builddir_1_2_bbr']
                    cmd = '%s/examples/apps/ncp/ot-ncp-%s' % (srcdir, mode)
                    cmd = 'spinel-cli.py -p "%s%s" -n' % (
                        cmd,
                        args,
                    )

            # Load Thread device of the testing environment version (may be 1.1 or 1.2).
            else:
                if 'OT_NCP_PATH' in os.environ:
                    cmd = 'spinel-cli.py -p "%s%s" -n' % (
                        os.environ['OT_NCP_PATH'],
                        args,
                    )
                elif 'top_builddir' in os.environ:
                    srcdir = os.environ['top_builddir']
                    cmd = '%s/examples/apps/ncp/ot-ncp-%s' % (srcdir, mode)
                    cmd = 'spinel-cli.py -p "%s%s" -n' % (
                        cmd,
                        args,
                    )

        # Load Thread 1.1 node when testing Thread 1.2 scenarios for interoperability.
        elif self.version == '1.1':
            if 'RADIO_DEVICE_1_1' in os.environ:
                args = ' --real-time-signal=+1 spinel+hdlc+uart://%s?forkpty-arg=%d' % (os.environ['RADIO_DEVICE_1_1'],
                                                                                        nodeid)
            else:
                args = ''

            if 'OT_NCP_PATH_1_1' in os.environ:
                cmd = 'spinel-cli.py -p "%s%s" -n' % (
                    os.environ['OT_NCP_PATH_1_1'],
                    args,
                )
            elif 'top_builddir_1_1' in os.environ:
                srcdir = os.environ['top_builddir_1_1']
                cmd = '%s/examples/apps/ncp/ot-ncp-%s' % (srcdir, mode)
                cmd = 'spinel-cli.py -p "%s%s" -n' % (
                    cmd,
                    args,
                )

        cmd += ' %d' % nodeid
        print("%s" % cmd)

        self.pexpect = pexpect.spawn(cmd, timeout=10)

        # Add delay to ensure that the process is ready to receive commands.
        time.sleep(0.2)
        self._expect('spinel-cli >')
        self.debug(int(os.getenv('DEBUG', '0')))

    def __init_soc(self, nodeid):
        """ Initialize a System-on-a-chip node connected via UART. """
        import fdpexpect

        serialPort = '/dev/ttyUSB%d' % ((nodeid - 1) * 2)
        self.pexpect = fdpexpect.fdspawn(os.open(serialPort, os.O_RDWR | os.O_NONBLOCK | os.O_NOCTTY))

    def __del__(self):
        self.destroy()

    def destroy(self):
        if not self._initialized:
            return

        if (hasattr(self.pexpect, 'proc') and self.pexpect.proc.poll() is None or
                not hasattr(self.pexpect, 'proc') and self.pexpect.isalive()):
            print("%d: exit" % self.nodeid)
            self.pexpect.send('exit\n')
            self.pexpect.expect(pexpect.EOF)
            self.pexpect.wait()
            self._initialized = False


class NodeImpl:
    is_host = False
    is_otbr = False

    def __init__(self, nodeid, name=None, simulator=None, **kwargs):
        self.nodeid = nodeid
        self.name = name or ('Node%d' % nodeid)

        self.simulator = simulator
        if self.simulator:
            self.simulator.add_node(self)

        super().__init__(nodeid, **kwargs)

        self.set_extpanid(config.EXTENDED_PANID)

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

    def _prepare_pattern(self, pattern):
        """Build a new pexpect pattern matching line by line.

        Adds lookahead and lookbehind to make each pattern match a whole line,
        and add 'Done' as the first pattern.

        Args:
            pattern: a single regex or a list of regex.

        Returns:
            A list of regex.
        """
        EXPECT_LINE_FORMAT = r'(?<=[\r\n])%s(?=[\r\n])'

        if isinstance(pattern, list):
            pattern = [EXPECT_LINE_FORMAT % p for p in pattern]
        else:
            pattern = [EXPECT_LINE_FORMAT % pattern]

        return [EXPECT_LINE_FORMAT % 'Done'] + pattern

    def _expect_result(self, pattern, *args, **kwargs):
        """Expect a single matching result.

        The arguments are identical to pexpect.expect().

        Returns:
            The matched line.
        """
        results = self._expect_results(pattern, *args, **kwargs)
        assert len(results) == 1, results
        return results[0]

    def _expect_results(self, pattern, *args, **kwargs):
        """Expect multiple matching results.

        The arguments are identical to pexpect.expect().

        Returns:
            The matched lines.
        """
        results = []
        pattern = self._prepare_pattern(pattern)

        while self._expect(pattern, *args, **kwargs):
            results.append(self.pexpect.match.group(0).decode('utf8'))

        return results

    def _expect_command_output(self, cmd: str):
        lines = []
        cmd_output_started = False

        while True:
            self._expect(r"[^\n]+")
            line = self.pexpect.match.group(0).decode('utf8').strip()

            if line.startswith('> '):
                line = line[2:]

            if line == '':
                continue

            if line == cmd:
                cmd_output_started = True
                continue

            if not cmd_output_started:
                continue

            if line == 'Done':
                break
            elif line.startswith('Error '):
                raise Exception(line)
            else:
                lines.append(line)

        print(f'_expect_command_output({cmd!r}) returns {lines!r}')
        return lines

    def read_cert_messages_in_commissioning_log(self, timeout=-1):
        """Get the log of the traffic after DTLS handshake.
        """
        format_str = br"=+?\[\[THCI\].*?type=%s.*?\].*?=+?[\s\S]+?-{40,}"
        join_fin_req = format_str % br"JOIN_FIN\.req"
        join_fin_rsp = format_str % br"JOIN_FIN\.rsp"
        dummy_format_str = br"\[THCI\].*?type=%s.*?"
        join_ent_ntf = dummy_format_str % br"JOIN_ENT\.ntf"
        join_ent_rsp = dummy_format_str % br"JOIN_ENT\.rsp"
        pattern = (b"(" + join_fin_req + b")|(" + join_fin_rsp + b")|(" + join_ent_ntf + b")|(" + join_ent_rsp + b")")

        messages = []
        # There are at most 4 cert messages both for joiner and commissioner
        for _ in range(0, 4):
            try:
                self._expect(pattern, timeout=timeout)
                log = self.pexpect.match.group(0)
                messages.append(self._extract_cert_message(log))
            except BaseException:
                break
        return messages

    def _extract_cert_message(self, log):
        res = re.search(br"direction=\w+", log)
        assert res
        direction = res.group(0).split(b'=')[1].strip()

        res = re.search(br"type=\S+", log)
        assert res
        type = res.group(0).split(b'=')[1].strip()

        payload = bytearray([])
        payload_len = 0
        if type in [b"JOIN_FIN.req", b"JOIN_FIN.rsp"]:
            res = re.search(br"len=\d+", log)
            assert res
            payload_len = int(res.group(0).split(b'=')[1].strip())

            hex_pattern = br"\|(\s([0-9a-fA-F]{2}|\.\.))+?\s+?\|"
            while True:
                res = re.search(hex_pattern, log)
                if not res:
                    break
                data = [int(hex, 16) for hex in res.group(0)[1:-1].split(b' ') if hex and hex != b'..']
                payload += bytearray(data)
                log = log[res.end() - 1:]

        assert len(payload) == payload_len
        return (direction, type, payload)

    def send_command(self, cmd, go=True):
        print("%d: %s" % (self.nodeid, cmd))
        self.pexpect.send(cmd + '\n')
        if go:
            self.simulator.go(0, nodeid=self.nodeid)
        sys.stdout.flush()

    def get_commands(self):
        self.send_command('?')
        self._expect('Commands:')
        return self._expect_results(r'\S+')

    def set_mode(self, mode):
        cmd = 'mode %s' % mode
        self.send_command(cmd)
        self._expect('Done')

    def debug(self, level):
        # `debug` command will not trigger interaction with simulator
        self.send_command('debug %d' % level, go=False)

    def start(self):
        self.interface_up()
        self.thread_start()

    def stop(self):
        self.thread_stop()
        self.interface_down()

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
        cmd = 'commissioner joiner add %s %s' % (addr, psk)
        self.send_command(cmd)
        self._expect('Done')

    def joiner_start(self, pskd='', provisioning_url=''):
        cmd = 'joiner start %s %s' % (pskd, provisioning_url)
        self.send_command(cmd)
        self._expect('Done')

    def clear_allowlist(self):
        cmd = 'macfilter addr clear'
        self.send_command(cmd)
        self._expect('Done')

    def enable_allowlist(self):
        cmd = 'macfilter addr allowlist'
        self.send_command(cmd)
        self._expect('Done')

    def disable_allowlist(self):
        cmd = 'macfilter addr disable'
        self.send_command(cmd)
        self._expect('Done')

    def add_allowlist(self, addr, rssi=None):
        cmd = 'macfilter addr add %s' % addr

        if rssi is not None:
            cmd += ' %s' % rssi

        self.send_command(cmd)
        self._expect('Done')

    def get_bbr_registration_jitter(self):
        self.send_command('bbr jitter')
        return int(self._expect_result(r'\d+'))

    def set_bbr_registration_jitter(self, jitter):
        cmd = 'bbr jitter %d' % jitter
        self.send_command(cmd)
        self._expect('Done')

    def enable_backbone_router(self):
        cmd = 'bbr enable'
        self.send_command(cmd)
        self._expect('Done')

    def disable_backbone_router(self):
        cmd = 'bbr disable'
        self.send_command(cmd)
        self._expect('Done')

    def register_backbone_router(self):
        cmd = 'bbr register'
        self.send_command(cmd)
        self._expect('Done')

    def get_backbone_router_state(self):
        states = [r'Disabled', r'Primary', r'Secondary']
        self.send_command('bbr state')
        return self._expect_result(states)

    @property
    def is_primary_backbone_router(self) -> bool:
        return self.get_backbone_router_state() == 'Primary'

    def get_backbone_router(self):
        cmd = 'bbr config'
        self.send_command(cmd)
        self._expect(r'(.*)Done')
        g = self.pexpect.match.groups()
        output = g[0].decode("utf-8")
        lines = output.strip().split('\n')
        lines = [l.strip() for l in lines]
        ret = {}
        for l in lines:
            z = re.search(r'seqno:\s+([0-9]+)', l)
            if z:
                ret['seqno'] = int(z.groups()[0])

            z = re.search(r'delay:\s+([0-9]+)', l)
            if z:
                ret['delay'] = int(z.groups()[0])

            z = re.search(r'timeout:\s+([0-9]+)', l)
            if z:
                ret['timeout'] = int(z.groups()[0])

        return ret

    def set_backbone_router(self, seqno=None, reg_delay=None, mlr_timeout=None):
        cmd = 'bbr config'

        if seqno is not None:
            cmd += ' seqno %d' % seqno

        if reg_delay is not None:
            cmd += ' delay %d' % reg_delay

        if mlr_timeout is not None:
            cmd += ' timeout %d' % mlr_timeout

        self.send_command(cmd)
        self._expect('Done')

    def set_domain_prefix(self, prefix, flags='prosD'):
        self.add_prefix(prefix, flags)
        self.register_netdata()

    def remove_domain_prefix(self, prefix):
        self.remove_prefix(prefix)
        self.register_netdata()

    def set_next_dua_response(self, status, iid=None):
        cmd = 'bbr mgmt dua {}'.format(status)
        if iid is not None:
            cmd += ' ' + str(iid)
        self.send_command(cmd)
        self._expect('Done')

    def set_dua_iid(self, iid):
        cmd = 'dua iid {}'.format(iid)
        self.send_command(cmd)
        self._expect('Done')

    def clear_dua_iid(self):
        cmd = 'dua iid clear'
        self.send_command(cmd)
        self._expect('Done')

    def multicast_listener_list(self) -> Dict[ipaddress.IPv6Address, int]:
        cmd = 'bbr mgmt mlr listener'
        self.send_command(cmd)

        table = {}
        for line in self._expect_results("\S+ \d+"):
            line = line.split()
            assert len(line) == 2, line
            ip = ipaddress.IPv6Address(line[0])
            timeout = int(line[1])
            assert ip not in table

            table[ip] = timeout

        return table

    def multicast_listener_clear(self):
        cmd = f'bbr mgmt mlr listener clear'
        self.send_command(cmd)
        self._expect("Done")

    def multicast_listener_add(self, ip: Union[ipaddress.IPv6Address, str], timeout: int = 0):
        if not isinstance(ip, ipaddress.IPv6Address):
            ip = ipaddress.IPv6Address(ip)

        cmd = f'bbr mgmt mlr listener add {ip.compressed} {timeout}'
        self.send_command(cmd)
        self._expect(r"(Done|Error .*)")

    def set_next_mlr_response(self, status: int):
        cmd = 'bbr mgmt mlr response {}'.format(status)
        self.send_command(cmd)
        self._expect('Done')

    def register_multicast_listener(self, *ipaddrs: Union[ipaddress.IPv6Address, str], timeout=None):
        assert len(ipaddrs) > 0, ipaddrs

        cmd = f'mlr reg {" ".join(ipaddrs)}'
        if timeout is not None:
            cmd += f' {int(timeout)}'
        self.send_command(cmd)
        self.simulator.go(3)
        lines = self._expect_command_output(cmd)
        m = re.match(r'status (\d+), (\d+) failed', lines[0])
        assert m is not None, lines
        status = int(m.group(1))
        failed_num = int(m.group(2))
        assert failed_num == len(lines) - 1
        failed_ips = list(map(ipaddress.IPv6Address, lines[1:]))
        print(f"register_multicast_listener {ipaddrs} => status: {status}, failed ips: {failed_ips}")
        return status, failed_ips

    def set_link_quality(self, addr, lqi):
        cmd = 'macfilter rss add-lqi %s %s' % (addr, lqi)
        self.send_command(cmd)
        self._expect('Done')

    def set_outbound_link_quality(self, lqi):
        cmd = 'macfilter rss add-lqi * %s' % (lqi)
        self.send_command(cmd)
        self._expect('Done')

    def remove_allowlist(self, addr):
        cmd = 'macfilter addr remove %s' % addr
        self.send_command(cmd)
        self._expect('Done')

    def get_addr16(self):
        self.send_command('rloc16')
        rloc16 = self._expect_result(r'[0-9a-fA-F]{4}')
        return int(rloc16, 16)

    def get_router_id(self):
        rloc16 = self.get_addr16()
        return rloc16 >> 10

    def get_addr64(self):
        self.send_command('extaddr')
        return self._expect_result('[0-9a-fA-F]{16}')

    def set_addr64(self, addr64):
        self.send_command('extaddr %s' % addr64)
        self._expect('Done')

    def get_eui64(self):
        self.send_command('eui64')
        return self._expect_result('[0-9a-fA-F]{16}')

    def set_extpanid(self, extpanid):
        self.send_command('extpanid %s' % extpanid)
        self._expect('Done')

    def get_joiner_id(self):
        self.send_command('joiner id')
        return self._expect_result('[0-9a-fA-F]{16}')

    def get_channel(self):
        self.send_command('channel')
        return int(self._expect_result(r'\d+'))

    def set_channel(self, channel):
        cmd = 'channel %d' % channel
        self.send_command(cmd)
        self._expect('Done')

    def get_masterkey(self):
        self.send_command('masterkey')
        return self._expect_result('[0-9a-fA-F]{32}')

    def set_masterkey(self, masterkey):
        cmd = 'masterkey %s' % masterkey
        self.send_command(cmd)
        self._expect('Done')

    def get_key_sequence_counter(self):
        self.send_command('keysequence counter')
        result = self._expect_result(r'\d+')
        return int(result)

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

    def _escape_escapable(self, string):
        """Escape CLI escapable characters in the given string.

        Args:
            string (str): UTF-8 input string.

        Returns:
            [str]: The modified string with escaped characters.
        """
        escapable_chars = '\\ \t\r\n'
        for char in escapable_chars:
            string = string.replace(char, '\\%s' % char)
        return string

    def get_network_name(self):
        self.send_command('networkname')
        return self._expect_result([r'\S+'])

    def set_network_name(self, network_name):
        cmd = 'networkname %s' % self._escape_escapable(network_name)
        self.send_command(cmd)
        self._expect('Done')

    def get_panid(self):
        self.send_command('panid')
        result = self._expect_result('0x[0-9a-fA-F]{4}')
        return int(result, 16)

    def set_panid(self, panid=config.PANID):
        cmd = 'panid %d' % panid
        self.send_command(cmd)
        self._expect('Done')

    def set_parent_priority(self, priority):
        cmd = 'parentpriority %d' % priority
        self.send_command(cmd)
        self._expect('Done')

    def get_partition_id(self):
        self.send_command('leaderpartitionid')
        return self._expect_result(r'\d+')

    def set_partition_id(self, partition_id):
        cmd = 'leaderpartitionid %d' % partition_id
        self.send_command(cmd)
        self._expect('Done')

    def get_pollperiod(self):
        self.send_command('pollperiod')
        return self._expect_result(r'\d+')

    def set_pollperiod(self, pollperiod):
        self.send_command('pollperiod %d' % pollperiod)
        self._expect('Done')

    def get_csl_info(self):
        self.send_command('csl')
        self._expect('Done')

    def set_csl_channel(self, csl_channel):
        self.send_command('csl channel %d' % csl_channel)
        self._expect('Done')

    def set_csl_period(self, csl_period):
        self.send_command('csl period %d' % csl_period)
        self._expect('Done')

    def set_csl_timeout(self, csl_timeout):
        self.send_command('csl timeout %d' % csl_timeout)
        self._expect('Done')

    def set_router_upgrade_threshold(self, threshold):
        cmd = 'routerupgradethreshold %d' % threshold
        self.send_command(cmd)
        self._expect('Done')

    def set_router_downgrade_threshold(self, threshold):
        cmd = 'routerdowngradethreshold %d' % threshold
        self.send_command(cmd)
        self._expect('Done')

    def get_router_downgrade_threshold(self) -> int:
        self.send_command('routerdowngradethreshold')
        return int(self._expect_result(r'\d+'))

    def prefer_router_id(self, router_id):
        cmd = 'preferrouterid %d' % router_id
        self.send_command(cmd)
        self._expect('Done')

    def release_router_id(self, router_id):
        cmd = 'releaserouterid %d' % router_id
        self.send_command(cmd)
        self._expect('Done')

    def get_state(self):
        states = [r'detached', r'child', r'router', r'leader', r'disabled']
        self.send_command('state')
        return self._expect_result(states)

    def set_state(self, state):
        cmd = 'state %s' % state
        self.send_command(cmd)
        self._expect('Done')

    def get_timeout(self):
        self.send_command('childtimeout')
        return self._expect_result(r'\d+')

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
        return self._expect_result(r'\d+')

    def set_weight(self, weight):
        cmd = 'leaderweight %d' % weight
        self.send_command(cmd)
        self._expect('Done')

    def add_ipaddr(self, ipaddr):
        cmd = 'ipaddr add %s' % ipaddr
        self.send_command(cmd)
        self._expect('Done')

    def add_ipmaddr(self, ipmaddr):
        cmd = 'ipmaddr add %s' % ipmaddr
        self.send_command(cmd)
        self._expect('Done')

    def del_ipmaddr(self, ipmaddr):
        cmd = 'ipmaddr del %s' % ipmaddr
        self.send_command(cmd)
        self._expect('Done')

    def get_addrs(self):
        self.send_command('ipaddr')

        return self._expect_results(r'\S+(:\S*)+')

    def get_mleid(self):
        self.send_command('ipaddr mleid')
        return self._expect_result(r'\S+(:\S*)+')

    def get_linklocal(self):
        self.send_command('ipaddr linklocal')
        return self._expect_result(r'\S+(:\S*)+')

    def get_rloc(self):
        self.send_command('ipaddr rloc')
        return self._expect_result(r'\S+(:\S*)+')

    def get_addr(self, prefix):
        network = ipaddress.ip_network(u'%s' % str(prefix))
        addrs = self.get_addrs()

        for addr in addrs:
            if isinstance(addr, bytearray):
                addr = bytes(addr)
            ipv6_address = ipaddress.ip_address(addr)
            if ipv6_address in network:
                return ipv6_address.exploded

        return None

    def has_ipaddr(self, address):
        ipaddr = ipaddress.ip_address(address)
        ipaddrs = self.get_addrs()
        for addr in ipaddrs:
            if isinstance(addr, bytearray):
                addr = bytes(addr)
            if ipaddress.ip_address(addr) == ipaddr:
                return True
        return False

    def get_ipmaddrs(self):
        self.send_command('ipmaddr')
        return self._expect_results(r'\S+(:\S*)+')

    def has_ipmaddr(self, address):
        ipmaddr = ipaddress.ip_address(address)
        ipmaddrs = self.get_ipmaddrs()
        for addr in ipmaddrs:
            if isinstance(addr, bytearray):
                addr = bytes(addr)
            if ipaddress.ip_address(addr) == ipmaddr:
                return True
        return False

    def get_addr_leader_aloc(self):
        addrs = self.get_addrs()
        for addr in addrs:
            segs = addr.split(':')
            if (segs[4] == '0' and segs[5] == 'ff' and segs[6] == 'fe00' and segs[7] == 'fc00'):
                return addr
        return None

    def get_eidcaches(self):
        eidcaches = []
        self.send_command('eidcache')

        pattern = self._prepare_pattern(r'([a-fA-F0-9\:]+) ([a-fA-F0-9]+)')
        while self._expect(pattern):
            eid = self.pexpect.match.groups()[0].decode("utf-8")
            rloc = self.pexpect.match.groups()[1].decode("utf-8")
            eidcaches.append((eid, rloc))

        return eidcaches

    def add_service(self, enterpriseNumber, serviceData, serverData):
        cmd = 'service add %s %s %s' % (
            enterpriseNumber,
            serviceData,
            serverData,
        )
        self.send_command(cmd)
        self._expect('Done')

    def remove_service(self, enterpriseNumber, serviceData):
        cmd = 'service remove %s %s' % (enterpriseNumber, serviceData)
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
            if ((not re.match(config.LINK_LOCAL_REGEX_PATTERN, ip6Addr, re.I)) and
                (not re.match(config.MESH_LOCAL_PREFIX_REGEX_PATTERN, ip6Addr, re.I)) and
                (not re.match(config.ROUTING_LOCATOR_REGEX_PATTERN, ip6Addr, re.I))):
                global_address.append(ip6Addr)

        return global_address

    def __getRloc(self):
        for ip6Addr in self.get_addrs():
            if (re.match(config.MESH_LOCAL_PREFIX_REGEX_PATTERN, ip6Addr, re.I) and
                    re.match(config.ROUTING_LOCATOR_REGEX_PATTERN, ip6Addr, re.I) and
                    not (re.match(config.ALOC_FLAG_REGEX_PATTERN, ip6Addr, re.I))):
                return ip6Addr
        return None

    def __getAloc(self):
        aloc = []
        for ip6Addr in self.get_addrs():
            if (re.match(config.MESH_LOCAL_PREFIX_REGEX_PATTERN, ip6Addr, re.I) and
                    re.match(config.ROUTING_LOCATOR_REGEX_PATTERN, ip6Addr, re.I) and
                    re.match(config.ALOC_FLAG_REGEX_PATTERN, ip6Addr, re.I)):
                aloc.append(ip6Addr)

        return aloc

    def __getMleid(self):
        for ip6Addr in self.get_addrs():
            if re.match(config.MESH_LOCAL_PREFIX_REGEX_PATTERN, ip6Addr,
                        re.I) and not (re.match(config.ROUTING_LOCATOR_REGEX_PATTERN, ip6Addr, re.I)):
                return ip6Addr

        return None

    def __getDua(self) -> Optional[str]:
        for ip6Addr in self.get_addrs():
            if re.match(config.DOMAIN_PREFIX_REGEX_PATTERN, ip6Addr, re.I):
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
        elif address_type == config.ADDRESS_TYPE.DUA:
            return self.__getDua()
        elif address_type == config.ADDRESS_TYPE.BACKBONE_GUA:
            return self._getBackboneGua()
        else:
            return None

    def get_context_reuse_delay(self):
        self.send_command('contextreusedelay')
        return self._expect_result(r'\d+')

    def set_context_reuse_delay(self, delay):
        cmd = 'contextreusedelay %d' % delay
        self.send_command(cmd)
        self._expect('Done')

    def add_prefix(self, prefix, flags='paosr', prf='med'):
        cmd = 'prefix add %s %s %s' % (prefix, flags, prf)
        self.send_command(cmd)
        self._expect('Done')

    def remove_prefix(self, prefix):
        cmd = 'prefix remove %s' % prefix
        self.send_command(cmd)
        self._expect('Done')

    def add_route(self, prefix, stable=False, prf='med'):
        cmd = 'route add %s ' % prefix
        if stable:
            cmd += 's'
        cmd += ' %s' % prf
        self.send_command(cmd)
        self._expect('Done')

    def remove_route(self, prefix):
        cmd = 'route remove %s' % prefix
        self.send_command(cmd)
        self._expect('Done')

    def register_netdata(self):
        self.send_command('netdata register')
        self._expect('Done')

    def send_network_diag_get(self, addr, tlv_types):
        self.send_command('networkdiagnostic get %s %s' % (addr, ' '.join([str(t.value) for t in tlv_types])))

        if isinstance(self.simulator, simulator.VirtualTime):
            self.simulator.go(8)
            timeout = 1
        else:
            timeout = 8

        self._expect('Done', timeout=timeout)

    def send_network_diag_reset(self, addr, tlv_types):
        self.send_command('networkdiagnostic reset %s %s' % (addr, ' '.join([str(t.value) for t in tlv_types])))

        if isinstance(self.simulator, simulator.VirtualTime):
            self.simulator.go(8)
            timeout = 1
        else:
            timeout = 8

        self._expect('Done', timeout=timeout)

    def energy_scan(self, mask, count, period, scan_duration, ipaddr):
        cmd = 'commissioner energy %d %d %d %d %s' % (
            mask,
            count,
            period,
            scan_duration,
            ipaddr,
        )
        self.send_command(cmd)

        if isinstance(self.simulator, simulator.VirtualTime):
            self.simulator.go(8)
            timeout = 1
        else:
            timeout = 8

        self._expect('Energy:', timeout=timeout)

    def panid_query(self, panid, mask, ipaddr):
        cmd = 'commissioner panid %d %d %s' % (panid, mask, ipaddr)
        self.send_command(cmd)

        if isinstance(self.simulator, simulator.VirtualTime):
            self.simulator.go(8)
            timeout = 1
        else:
            timeout = 8

        self._expect('Conflict:', timeout=timeout)

    def scan(self):
        self.send_command('scan')

        return self._expect_results(r'\|\s(\S+)\s+\|\s(\S+)\s+\|\s([0-9a-fA-F]{4})\s\|\s([0-9a-fA-F]{16})\s\|\s(\d+)')

    def ping(self, ipaddr, num_responses=1, size=None, timeout=5):
        cmd = 'ping %s' % ipaddr
        if size is not None:
            cmd += ' %d' % size

        self.send_command(cmd)

        end = self.simulator.now() + timeout

        responders = {}

        result = True
        # ncp-sim doesn't print Done
        done = (self.node_type == 'ncp-sim')
        while len(responders) < num_responses or not done:
            self.simulator.go(1)
            try:
                i = self._expect([r'from (\S+):', r'Done'], timeout=0.1)
            except (pexpect.TIMEOUT, socket.timeout):
                if self.simulator.now() < end:
                    continue
                result = False
                if isinstance(self.simulator, simulator.VirtualTime):
                    self.simulator.sync_devices()
                break
            else:
                if i == 0:
                    responders[self.pexpect.match.groups()[0]] = 1
                elif i == 1:
                    done = True

        return result

    def reset(self):
        self.send_command('reset')
        time.sleep(0.1)

    def set_router_selection_jitter(self, jitter):
        cmd = 'routerselectionjitter %d' % jitter
        self.send_command(cmd)
        self._expect('Done')

    def set_active_dataset(
        self,
        timestamp,
        panid=None,
        channel=None,
        channel_mask=None,
        master_key=None,
    ):
        self.send_command('dataset clear')
        self._expect('Done')

        cmd = 'dataset activetimestamp %d' % timestamp
        self.send_command(cmd)
        self._expect('Done')

        if panid is not None:
            cmd = 'dataset panid %d' % panid
            self.send_command(cmd)
            self._expect('Done')

        if channel is not None:
            cmd = 'dataset channel %d' % channel
            self.send_command(cmd)
            self._expect('Done')

        if channel_mask is not None:
            cmd = 'dataset channelmask %d' % channel_mask
            self.send_command(cmd)
            self._expect('Done')

        if master_key is not None:
            cmd = 'dataset masterkey %s' % master_key
            self.send_command(cmd)
            self._expect('Done')

        # Set the meshlocal prefix in config.py
        self.send_command('dataset meshlocalprefix %s' % config.MESH_LOCAL_PREFIX.split('/')[0])
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

        if panid is not None:
            cmd = 'dataset panid %d' % panid
            self.send_command(cmd)
            self._expect('Done')

        if channel is not None:
            cmd = 'dataset channel %d' % channel
            self.send_command(cmd)
            self._expect('Done')

        # Set the meshlocal prefix in config.py
        self.send_command('dataset meshlocalprefix %s' % config.MESH_LOCAL_PREFIX.split('/')[0])
        self._expect('Done')

        self.send_command('dataset commit pending')
        self._expect('Done')

    def announce_begin(self, mask, count, period, ipaddr):
        cmd = 'commissioner announce %d %d %d %s' % (
            mask,
            count,
            period,
            ipaddr,
        )
        self.send_command(cmd)
        self._expect('Done')

    def send_mgmt_active_set(
        self,
        active_timestamp=None,
        channel=None,
        channel_mask=None,
        extended_panid=None,
        panid=None,
        master_key=None,
        mesh_local=None,
        network_name=None,
        binary=None,
    ):
        cmd = 'dataset mgmtsetcommand active '

        if active_timestamp is not None:
            cmd += 'activetimestamp %d ' % active_timestamp

        if channel is not None:
            cmd += 'channel %d ' % channel

        if channel_mask is not None:
            cmd += 'channelmask %d ' % channel_mask

        if extended_panid is not None:
            cmd += 'extpanid %s ' % extended_panid

        if panid is not None:
            cmd += 'panid %d ' % panid

        if master_key is not None:
            cmd += 'masterkey %s ' % master_key

        if mesh_local is not None:
            cmd += 'localprefix %s ' % mesh_local

        if network_name is not None:
            cmd += 'networkname %s ' % self._escape_escapable(network_name)

        if binary is not None:
            cmd += '-x %s ' % binary

        self.send_command(cmd)
        self._expect('Done')

    def send_mgmt_active_get(
        self,
        active_timestamp=None,
        channel=None,
        channel_mask=None,
        extended_panid=None,
        panid=None,
        master_key=None,
        mesh_local=None,
        network_name=None,
        binary=None,
    ):
        cmd = 'dataset mgmtgetcommand active '

        if active_timestamp is not None:
            cmd += 'activetimestamp %d ' % active_timestamp

        if channel is not None:
            cmd += 'channel %d ' % channel

        if channel_mask is not None:
            cmd += 'channelmask %d ' % channel_mask

        if extended_panid is not None:
            cmd += 'extpanid %s ' % extended_panid

        if panid is not None:
            cmd += 'panid %d ' % panid

        if master_key is not None:
            cmd += 'masterkey %s ' % master_key

        if mesh_local is not None:
            cmd += 'localprefix %s ' % mesh_local

        if network_name is not None:
            cmd += 'networkname %s ' % self._escape_escapable(network_name)

        if binary is not None:
            cmd += 'binary %s ' % binary

        self.send_command(cmd)

    def send_mgmt_pending_set(
        self,
        pending_timestamp=None,
        active_timestamp=None,
        delay_timer=None,
        channel=None,
        panid=None,
        master_key=None,
        mesh_local=None,
        network_name=None,
    ):
        cmd = 'dataset mgmtsetcommand pending '
        if pending_timestamp is not None:
            cmd += 'pendingtimestamp %d ' % pending_timestamp

        if active_timestamp is not None:
            cmd += 'activetimestamp %d ' % active_timestamp

        if delay_timer is not None:
            cmd += 'delaytimer %d ' % delay_timer

        if channel is not None:
            cmd += 'channel %d ' % channel

        if panid is not None:
            cmd += 'panid %d ' % panid

        if master_key is not None:
            cmd += 'masterkey %s ' % master_key

        if mesh_local is not None:
            cmd += 'localprefix %s ' % mesh_local

        if network_name is not None:
            cmd += 'networkname %s ' % self._escape_escapable(network_name)

        self.send_command(cmd)
        self._expect('Done')

    def coap_cancel(self):
        """
        Cancel a CoAP subscription.
        """
        cmd = 'coap cancel'
        self.send_command(cmd)
        self._expect('Done')

    def coap_delete(self, ipaddr, uri, con=False, payload=None):
        """
        Send a DELETE request via CoAP.
        """
        return self._coap_rq('delete', ipaddr, uri, con, payload)

    def coap_get(self, ipaddr, uri, con=False, payload=None):
        """
        Send a GET request via CoAP.
        """
        return self._coap_rq('get', ipaddr, uri, con, payload)

    def coap_observe(self, ipaddr, uri, con=False, payload=None):
        """
        Send a GET request via CoAP with Observe set.
        """
        return self._coap_rq('observe', ipaddr, uri, con, payload)

    def coap_post(self, ipaddr, uri, con=False, payload=None):
        """
        Send a POST request via CoAP.
        """
        return self._coap_rq('post', ipaddr, uri, con, payload)

    def coap_put(self, ipaddr, uri, con=False, payload=None):
        """
        Send a PUT request via CoAP.
        """
        return self._coap_rq('put', ipaddr, uri, con, payload)

    def _coap_rq(self, method, ipaddr, uri, con=False, payload=None):
        """
        Issue a GET/POST/PUT/DELETE/GET OBSERVE request.
        """
        cmd = 'coap %s %s %s' % (method, ipaddr, uri)
        if con:
            cmd += ' con'
        else:
            cmd += ' non'

        if payload is not None:
            cmd += ' %s' % payload

        self.send_command(cmd)
        return self.coap_wait_response()

    def coap_wait_response(self):
        """
        Wait for a CoAP response, and return it.
        """
        if isinstance(self.simulator, simulator.VirtualTime):
            self.simulator.go(5)
            timeout = 1
        else:
            timeout = 5

        self._expect(r'coap response from ([\da-f:]+)(?: OBS=(\d+))?'
                     r'(?: with payload: ([\da-f]+))?\b',
                     timeout=timeout)
        (source, observe, payload) = self.pexpect.match.groups()
        source = source.decode('UTF-8')

        if observe is not None:
            observe = int(observe, base=10)

        if payload is not None:
            payload = binascii.a2b_hex(payload).decode('UTF-8')

        # Return the values received
        return dict(source=source, observe=observe, payload=payload)

    def coap_wait_request(self):
        """
        Wait for a CoAP request to be made.
        """
        if isinstance(self.simulator, simulator.VirtualTime):
            self.simulator.go(5)
            timeout = 1
        else:
            timeout = 5

        self._expect(r'coap request from ([\da-f:]+)(?: OBS=(\d+))?'
                     r'(?: with payload: ([\da-f]+))?\b',
                     timeout=timeout)
        (source, observe, payload) = self.pexpect.match.groups()
        source = source.decode('UTF-8')

        if observe is not None:
            observe = int(observe, base=10)

        if payload is not None:
            payload = binascii.a2b_hex(payload).decode('UTF-8')

        # Return the values received
        return dict(source=source, observe=observe, payload=payload)

    def coap_wait_subscribe(self):
        """
        Wait for a CoAP client to be subscribed.
        """
        if isinstance(self.simulator, simulator.VirtualTime):
            self.simulator.go(5)
            timeout = 1
        else:
            timeout = 5

        self._expect(r'Subscribing client\b', timeout=timeout)

    def coap_wait_ack(self):
        """
        Wait for a CoAP notification ACK.
        """
        if isinstance(self.simulator, simulator.VirtualTime):
            self.simulator.go(5)
            timeout = 1
        else:
            timeout = 5

        self._expect(r'Received ACK in reply to notification ' r'from ([\da-f:]+)\b', timeout=timeout)
        (source,) = self.pexpect.match.groups()
        source = source.decode('UTF-8')

        return source

    def coap_set_resource_path(self, path):
        """
        Set the path for the CoAP resource.
        """
        cmd = 'coap resource %s' % path
        self.send_command(cmd)
        self._expect('Done')

    def coap_set_content(self, content):
        """
        Set the content of the CoAP resource.
        """
        cmd = 'coap set %s' % content
        self.send_command(cmd)
        self._expect('Done')

    def coap_start(self):
        """
        Start the CoAP service.
        """
        cmd = 'coap start'
        self.send_command(cmd)
        self._expect('Done')

    def coap_stop(self):
        """
        Stop the CoAP service.
        """
        cmd = 'coap stop'
        self.send_command(cmd)

        if isinstance(self.simulator, simulator.VirtualTime):
            self.simulator.go(5)
            timeout = 1
        else:
            timeout = 5

        self._expect('Done', timeout=timeout)

    def coaps_start_psk(self, psk, pskIdentity):
        cmd = 'coaps psk %s %s' % (psk, pskIdentity)
        self.send_command(cmd)
        self._expect('Done')

        cmd = 'coaps start'
        self.send_command(cmd)
        self._expect('Done')

    def coaps_start_x509(self):
        cmd = 'coaps x509'
        self.send_command(cmd)
        self._expect('Done')

        cmd = 'coaps start'
        self.send_command(cmd)
        self._expect('Done')

    def coaps_set_resource_path(self, path):
        cmd = 'coaps resource %s' % path
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
        cmd = 'coaps connect %s' % ipaddr
        self.send_command(cmd)

        if isinstance(self.simulator, simulator.VirtualTime):
            self.simulator.go(5)
            timeout = 1
        else:
            timeout = 5

        self._expect('coaps connected', timeout=timeout)

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

        self._expect('coaps response', timeout=timeout)

    def commissioner_mgmtset(self, tlvs_binary):
        cmd = 'commissioner mgmtset -x %s' % tlvs_binary
        self.send_command(cmd)
        self._expect('Done')

    def bytes_to_hex_str(self, src):
        return ''.join(format(x, '02x') for x in src)

    def commissioner_mgmtset_with_tlvs(self, tlvs):
        payload = bytearray()
        for tlv in tlvs:
            payload += tlv.to_hex()
        self.commissioner_mgmtset(self.bytes_to_hex_str(payload))

    def udp_start(self, local_ipaddr, local_port):
        cmd = 'udp open'
        self.send_command(cmd)
        self._expect('Done')

        cmd = 'udp bind %s %s' % (local_ipaddr, local_port)
        self.send_command(cmd)
        self._expect('Done')

    def udp_stop(self):
        cmd = 'udp close'
        self.send_command(cmd)
        self._expect('Done')

    def udp_send(self, bytes, ipaddr, port, success=True):
        cmd = 'udp send %s %d -s %d ' % (ipaddr, port, bytes)
        self.send_command(cmd)
        if success:
            self._expect('Done')
        else:
            self._expect('Error')

    def udp_check_rx(self, bytes_should_rx):
        self._expect('%d bytes' % bytes_should_rx)

    def set_routereligible(self, enable: bool):
        cmd = f'routereligible {"enable" if enable else "disable"}'
        self.send_command(cmd)
        self._expect('Done')

    def router_list(self):
        cmd = 'router list'
        self.send_command(cmd)
        self._expect([r'(\d+)((\s\d+)*)'])

        g = self.pexpect.match.groups()
        router_list = g[0] + ' ' + g[1]
        router_list = [int(x) for x in router_list.split()]
        self._expect('Done')
        return router_list

    def router_table(self):
        cmd = 'router table'
        self.send_command(cmd)

        self._expect(r'(.*)Done')
        g = self.pexpect.match.groups()
        output = g[0]
        lines = output.strip().split('\n')
        lines = [l.strip() for l in lines]
        router_table = {}
        for i, line in enumerate(lines):
            if not line.startswith('|') or not line.endswith('|'):
                if i not in (0, 2):
                    # should not happen
                    print("unexpected line %d: %s" % (i, line))

                continue

            line = line[1:][:-1]
            line = [x.strip() for x in line.split('|')]
            if len(line) != 8:
                print("unexpected line %d: %s" % (i, line))
                continue

            try:
                int(line[0])
            except ValueError:
                if i != 1:
                    print("unexpected line %d: %s" % (i, line))
                continue

            id = int(line[0])
            rloc16 = int(line[1], 16)
            nexthop = int(line[2])
            pathcost = int(line[3])
            lqin = int(line[4])
            lqout = int(line[5])
            age = int(line[6])
            emac = str(line[7])

            router_table[id] = {
                'rloc16': rloc16,
                'nexthop': nexthop,
                'pathcost': pathcost,
                'lqin': lqin,
                'lqout': lqout,
                'age': age,
                'emac': emac,
            }

        return router_table

    def link_metrics_query_single_probe(self, dst_addr: str, linkmetrics_flags: str):
        cmd = 'linkmetrics query %s single %s' % (dst_addr, linkmetrics_flags)
        self.send_command(cmd)
        self._expect('Done')

    def send_address_notification(self, dst: str, target: str, mliid: str):
        cmd = f'fake /a/an {dst} {target} {mliid}'
        self.send_command(cmd)
        self._expect("Done")


class Node(NodeImpl, OtCli):
    pass


class LinuxHost():
    PING_RESPONSE_PATTERN = re.compile(r'\d+ bytes from .*:.*')
    ETH_DEV = 'eth0'

    def get_ether_addrs(self):
        output = self.bash(f'ip -6 addr list dev {self.ETH_DEV}')

        addrs = []
        for line in output:
            # line example: "inet6 fe80::42:c0ff:fea8:903/64 scope link"
            line = line.strip().split()

            if line and line[0] == 'inet6':
                addr = line[1]
                if '/' in addr:
                    addr = addr.split('/')[0]
                addrs.append(addr)

        logging.debug('%s: get_ether_addrs: %r', self, addrs)
        return addrs

    def get_ether_mac(self):
        output = self.bash(f'ip addr list dev {self.ETH_DEV}')
        for line in output:
            # link/ether 02:42:ac:11:00:02 brd ff:ff:ff:ff:ff:ff link-netnsid 0
            line = line.strip().split()
            if line and line[0] == 'link/ether':
                return line[1]

        assert False, output

    def ping_ether(self, ipaddr, num_responses=1, size=None, timeout=5) -> int:
        cmd = f'ping -6 {ipaddr} -I eth0 -c {num_responses} -W {timeout}'
        if size is not None:
            cmd += f' -s {size}'

        resp_count = 0

        try:
            for line in self.bash(cmd):
                if self.PING_RESPONSE_PATTERN.match(line):
                    resp_count += 1
        except subprocess.CalledProcessError:
            pass

        return resp_count

    def _getBackboneGua(self) -> Optional[str]:
        for ip6Addr in self.get_addrs():
            if re.match(config.BACKBONE_PREFIX_REGEX_PATTERN, ip6Addr, re.I):
                return ip6Addr

        return None

    def ping(self, *args, **kwargs):
        backbone = kwargs.pop('backbone', False)
        if backbone:
            return self.ping_ether(*args, **kwargs)
        else:
            return super().ping(*args, **kwargs)


class OtbrNode(LinuxHost, NodeImpl, OtbrDocker):
    is_otbr = True
    is_bbr = True  # OTBR is also BBR

    def __repr__(self):
        return f'Otbr<{self.nodeid}>'

    def get_addrs(self) -> List[str]:
        return super().get_addrs() + self.get_ether_addrs()


class HostNode(LinuxHost, OtbrDocker):
    is_host = True

    def __init__(self, nodeid, name=None, **kwargs):
        self.nodeid = nodeid
        self.name = name or ('Host%d' % nodeid)
        super().__init__(nodeid, **kwargs)

    def start(self):
        # TODO: Use radvd to advertise the Domain Prefix on the Backbone link.
        pass

    def stop(self):
        pass

    def get_addrs(self) -> List[str]:
        return self.get_ether_addrs()

    def __repr__(self):
        return f'Host<{self.nodeid}>'

    def get_ip6_address(self, address_type: config.ADDRESS_TYPE):
        """Get specific type of IPv6 address configured on thread device.

        Args:
            address_type: the config.ADDRESS_TYPE type of IPv6 address.

        Returns:
            IPv6 address string.
        """
        assert address_type == config.ADDRESS_TYPE.BACKBONE_GUA

        if address_type == config.ADDRESS_TYPE.BACKBONE_GUA:
            return self._getBackboneGua()
        else:
            return None


if __name__ == '__main__':
    unittest.main()
