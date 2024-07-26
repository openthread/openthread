#!/usr/bin/env python3
#
#  Copyright (c) 2020, The OpenThread Authors.
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
import logging
import queue
import re
import threading
import time
from abc import abstractmethod, ABC
from typing import Any, Callable, Optional, Union, List, Pattern

from .connectors import OtCliHandler
from .errors import ExpectLineTimeoutError, CommandError
from .utils import match_line


class OTCommandHandler(ABC):
    """This abstract class defines interfaces of a OT Command Handler."""

    @abstractmethod
    def execute_command(self, cmd: str, timeout: float) -> List[str]:
        """Method execute_command should execute the OT CLI command within a timeout (in seconds) and return the
        command output as a list of lines.

        Note: each line SHOULD NOT contain '\r\n' at the end. The last line of output should be 'Done' or
        'Error <code>: <msg>' following OT CLI conventions.
        """

    @abstractmethod
    def execute_platform_command(self, cmd: str, timeout: float) -> List[str]:
        """Method execute_platform_command should execute the platform command within a timeout (in seconds) and
        return the command output as a list of lines.

        Note: each line of the command output MUST NOT contain '\r\n' at the end.
        """

    @abstractmethod
    def close(self):
        """Method close should close the OT Command Handler."""

    @abstractmethod
    def wait(self, duration: float) -> List[str]:
        """Method wait should wait for a given duration and return the OT CLI output during this period.

        Normally, OT CLI does not output when it's not executing any command. But OT CLI can also output
        asynchronously in some cases (e.g. `Join Success` when Joiner joins successfully).
        """

    @abstractmethod
    def set_line_read_callback(self, callback: Optional[Callable[[str], Any]]):
        """Method set_line_read_callback should register a callback that will be called for every line
        output by the OT CLI.

        This is useful for handling asynchronous command output while still being able to execute
        other commands.
        """
        pass

    def shell(self, cmd: str, timeout: float) -> List[str]:
        raise NotImplementedError("shell command is not supported on %s" % self.__class__.__name__)


class OtCliCommandRunner(OTCommandHandler):
    __PATTERN_COMMAND_DONE_OR_ERROR = re.compile(
        r'(Done|Error|Error \d+:.*|.*: command not found)$')  # "Error" for spinel-cli.py

    __PATTERN_LOG_LINE = re.compile(r'((\[(NONE|CRIT|WARN|NOTE|INFO|DEBG)\])'
                                    r'|(-.*-+: )'  # e.g. -CLI-----:
                                    r'|(\[[DINWC\-]\] (?=[\w\-]{14}:)\w+-*:)'  # e.g. [I] Mac-----------:
                                    r')')
    """regex used to filter logs"""

    assert __PATTERN_LOG_LINE.match('[I] ChannelMonitor: debug log')
    assert __PATTERN_LOG_LINE.match('[I] Mac-----------: info log')
    assert __PATTERN_LOG_LINE.match('[N] MeshForwarder-: note    log')
    assert __PATTERN_LOG_LINE.match('[W] Notifier------: warn log')
    assert __PATTERN_LOG_LINE.match('[C] Mle-----------: critical log')
    assert __PATTERN_LOG_LINE.match('[-] Settings------: none log')
    assert not __PATTERN_LOG_LINE.match('[-] Settings-----: none log')  # not enough `-` after module name

    __ASYNC_COMMANDS = {'scan', 'ping', 'discover'}

    def __init__(self, otcli: OtCliHandler, is_spinel_cli=False):
        self.__otcli: OtCliHandler = otcli
        self.__is_spinel_cli = is_spinel_cli
        self.__expect_command_echoback = not self.__is_spinel_cli
        self.__line_read_callback = None

        self.__pending_lines = queue.Queue()
        self.__should_close = threading.Event()
        self.__otcli_reader = threading.Thread(target=self.__otcli_read_routine, daemon=True)
        self.__otcli_reader.start()

    def __repr__(self):
        return repr(self.__otcli)

    def execute_command(self, cmd, timeout=10) -> List[str]:
        assert not self.__should_close.is_set(), "OT CLI is already closed."
        self.__otcli.writeline(cmd)

        if cmd in ('reset', 'factoryreset'):
            self.wait(3)
            self.__otcli.writeline('extaddr')
            self.wait(1)
            return []

        if self.__expect_command_echoback:
            self.__expect_line(timeout, cmd)

        output = self.__expect_line(timeout,
                                    OtCliCommandRunner.__PATTERN_COMMAND_DONE_OR_ERROR,
                                    asynchronous=cmd.split()[0] in OtCliCommandRunner.__ASYNC_COMMANDS)
        return output

    def execute_platform_command(self, cmd, timeout=10) -> List[str]:
        raise NotImplementedError(f'Platform command is not supported on {self.__class__.__name__}')

    def wait(self, duration: float) -> List[str]:
        self.__otcli.wait(duration)

        output = []
        try:
            while True:
                line = self.__pending_lines.get_nowait()
                output.append(line)

        except queue.Empty:
            pass

        return output

    def close(self):
        self.__should_close.set()
        self.__otcli.close()
        self.__otcli_reader.join()

    def set_line_read_callback(self, callback: Optional[Callable[[str], Any]]):
        self.__line_read_callback = callback

    #
    # Private methods
    #

    def __expect_line(self, timeout: float, expect_line: Union[str, Pattern], asynchronous=False) -> List[str]:
        output = []

        if not asynchronous:
            while True:
                try:
                    line = self.__pending_lines.get(timeout=timeout)
                except queue.Empty:
                    raise ExpectLineTimeoutError(expect_line)

                output.append(line)

                if match_line(line, expect_line):
                    break
        else:
            done = False
            while not done and timeout > 0:
                lines = self.wait(1)
                timeout -= 1

                for line in lines:
                    output.append(line)

                    if match_line(line, expect_line):
                        done = True
                        break

            if not done:
                raise ExpectLineTimeoutError(expect_line)

        return output

    def __otcli_read_routine(self):
        while not self.__should_close.is_set():
            try:
                line = self.__otcli.readline()
            except Exception:
                if self.__should_close.is_set():
                    break
                else:
                    raise

            logging.debug('%s: %r', self.__otcli, line)

            if line is None:
                break

            if line.startswith('> '):
                line = line[2:]

            if self.__line_read_callback is not None:
                self.__line_read_callback(line)

            logging.debug('%s: %s', self.__otcli, line)

            if not OtCliCommandRunner.__PATTERN_LOG_LINE.match(line):
                self.__pending_lines.put(line)


class OtbrSshCommandRunner(OTCommandHandler):

    def __init__(self, host, port, username, password, sudo):
        import paramiko

        self.__host = host
        self.__port = port
        self.__sudo = sudo
        self.__ssh = paramiko.SSHClient()
        self.__ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())

        self.__line_read_callback = None

        try:
            self.__ssh.connect(host,
                               port=port,
                               username=username,
                               password=password,
                               allow_agent=False,
                               look_for_keys=False)
        except paramiko.ssh_exception.AuthenticationException:
            if not password:
                transport = self.__ssh.get_transport()
                assert transport is not None
                transport.auth_none(username)
            else:
                raise

    def __repr__(self):
        return f'{self.__host}:{self.__port}'

    def execute_command(self, cmd: str, timeout: float) -> List[str]:
        sh_cmd = f'ot-ctl {cmd}'
        if self.__sudo:
            sh_cmd = 'sudo ' + sh_cmd

        output = self.shell(sh_cmd, timeout=timeout)

        if self.__line_read_callback is not None:
            for line in output:
                self.__line_read_callback(line)

        if cmd in ('reset', 'factoryreset'):
            self.wait(3)

        return output

    def execute_platform_command(self, cmd, timeout=10) -> List[str]:
        if self.__sudo:
            cmd = 'sudo ' + cmd

        return self.shell(cmd, timeout=timeout)

    def shell(self, cmd: str, timeout: float) -> List[str]:
        cmd_in, cmd_out, cmd_err = self.__ssh.exec_command(cmd, timeout=int(timeout), bufsize=1024)
        errput = [l.rstrip('\r\n') for l in cmd_err.readlines()]
        output = [l.rstrip('\r\n') for l in cmd_out.readlines()]

        if errput:
            raise CommandError(cmd, errput)

        return output

    def close(self):
        self.__ssh.close()

    def wait(self, duration: float) -> List[str]:
        time.sleep(duration)
        return []

    def set_line_read_callback(self, callback: Optional[Callable[[str], Any]]):
        self.__line_read_callback = callback


class OtbrAdbCommandRunner(OTCommandHandler):

    from adb_shell.adb_device import AdbDevice

    def __init__(self, adb: AdbDevice):
        self.__adb = adb
        self.__line_read_callback = None
        self.__adb.connect(rsa_keys=None, auth_timeout_s=0.1)

    def execute_command(self, cmd: str, timeout: float) -> List[str]:
        sh_cmd = f'ot-ctl {cmd}'

        output = self.shell(sh_cmd, timeout=timeout)

        if self.__line_read_callback is not None:
            for line in output:
                self.__line_read_callback(line)

        if cmd in ('reset', 'factoryreset'):
            self.wait(3)

        return output

    def execute_platform_command(self, cmd: str, timeout: float = 10) -> List[str]:
        return self.shell(cmd, timeout=timeout)

    def shell(self, cmd: str, timeout: float) -> List[str]:
        return self.__adb.shell(cmd, transport_timeout_s=timeout, read_timeout_s=timeout,
                                timeout_s=timeout).splitlines()

    def close(self):
        self.__adb.close()

    def wait(self, duration: float) -> List[str]:
        time.sleep(duration)
        return []

    def set_line_read_callback(self, callback: Optional[Callable[[str], Any]]):
        self.__line_read_callback = callback


class OtbrAdbTcpCommandRunner(OtbrAdbCommandRunner):

    def __init__(self, host: str, port: int):
        from adb_shell.adb_device import AdbDeviceTcp

        self.__host = host
        self.__port = port

        adb = AdbDeviceTcp(host, port, default_transport_timeout_s=9.0)
        super(OtbrAdbTcpCommandRunner, self).__init__(adb)

    def __repr__(self):
        return f'{self.__host}:{self.__port}'


class OtbrAdbUsbCommandRunner(OtbrAdbCommandRunner):

    def __init__(self, serial: str):
        from adb_shell.adb_device import AdbDeviceUsb

        self.__serial = serial

        adb = AdbDeviceUsb(serial, port_path=None, default_transport_timeout_s=9.0)
        super(OtbrAdbUsbCommandRunner, self).__init__(adb)

    def __repr__(self):
        return f'USB:{self.__serial}'
