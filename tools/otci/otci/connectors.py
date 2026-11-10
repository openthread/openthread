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
import os
import select
import subprocess
import threading
import time
from abc import abstractmethod, ABC
from typing import Any, List, Optional


class OtCliHandler(ABC):
    """This abstract class defines interfaces for a OT CLI Handler."""

    @abstractmethod
    def readline(self) -> Optional[str]:
        """Method readline should return the next line read from OT CLI."""

    @abstractmethod
    def writeline(self, s: str) -> None:
        """Method writeline should write a line to the OT CLI.

        It should block until all characters are written to OT CLI.
        """

    @abstractmethod
    def wait(self, duration: float, **kwargs) -> None:
        """Method wait should wait for a given duration.

        A normal implementation should just call `time.sleep(duration)`. This is intended for proceeding Virtual Time
        Simulation instances.
        """

    @abstractmethod
    def close(self) -> None:
        """Method close should close the OT CLI Handler."""

    def wait_for_line(self) -> bool:
        """Block until a complete line (or EOF) is ready for `readline()` without blocking.

        Returns False if pre-waiting is not supported by this handler.
        """
        return False

    def has_pending_lines(self) -> bool:
        """Return True if a complete line or unread output bytes are pending."""
        return False


class Simulator(ABC):
    """This abstract class defines interfaces for a Virtual Time Simulator."""

    @abstractmethod
    def go(self, duration: float, **kwargs):
        """Proceed the simulator for a given duration (in seconds)."""
        pass


class OtCliPopen(OtCliHandler):
    """Connector for OT CLI process (a Popen instance)."""

    def __init__(self, proc: subprocess.Popen[Any], nodeid: int, simulator: Optional[Simulator]):
        assert proc.stdout is not None
        self.__otcli_proc = proc
        self.__nodeid = nodeid
        self.__simulator = simulator
        self.__read_lock = threading.Lock()
        self.__select_cond = threading.Condition(self.__read_lock)
        self.__selecting = 0
        self.__read_buf = bytearray()
        self.__eof = False
        self.__closed = False
        self.__fds_closed = False
        self.__wake_r, self.__wake_w = os.pipe()
        os.set_blocking(self.__wake_r, False)
        os.set_blocking(self.__wake_w, False)
        self.__stdout_fd = proc.stdout.fileno()
        os.set_blocking(self.__stdout_fd, False)

        if self.__simulator is not None:
            self.wait(0)

    def __repr__(self):
        return 'OTCli<%d>' % self.__nodeid

    def __fill_read_buf_locked(self) -> None:
        if self.__eof or self.__closed:
            return
        while True:
            try:
                chunk = os.read(self.__stdout_fd, 4096)
                break
            except BlockingIOError:
                return
            except InterruptedError:
                continue
            except OSError:
                self.__eof = True
                return

        if not chunk:
            self.__eof = True
            return

        self.__read_buf.extend(chunk)

    def wait_for_line(self) -> bool:
        while True:
            with self.__read_lock:
                if b'\n' in self.__read_buf or self.__eof or self.__closed:
                    return True
                self.__fill_read_buf_locked()
                if b'\n' in self.__read_buf or self.__eof or self.__closed:
                    return True
                stdout_fd = self.__stdout_fd
                wake_r = self.__wake_r
                self.__selecting += 1

            try:
                select.select([stdout_fd, wake_r], [], [])
            except InterruptedError:
                continue
            except (OSError, ValueError):
                return True
            finally:
                with self.__read_lock:
                    self.__selecting -= 1
                    if self.__selecting == 0:
                        self.__select_cond.notify_all()

    def readline(self) -> Optional[str]:
        self.wait_for_line()
        with self.__read_lock:
            if self.__closed:
                return None

            newline_idx = self.__read_buf.find(b'\n')
            if newline_idx != -1:
                raw_line = bytes(self.__read_buf[:newline_idx + 1])
                del self.__read_buf[:newline_idx + 1]
                return raw_line.decode('utf-8', errors='replace').rstrip('\r\n')

            if self.__read_buf:
                raw_line = bytes(self.__read_buf)
                self.__read_buf.clear()
                return raw_line.decode('utf-8', errors='replace').rstrip('\r\n')

            return None

    def writeline(self, s: str):
        assert self.__otcli_proc.stdin is not None
        self.__otcli_proc.stdin.write(s + '\n')
        self.__otcli_proc.stdin.flush()

    def has_pending_lines(self) -> bool:
        with self.__read_lock:
            if self.__closed:
                return False
            if b'\n' in self.__read_buf:
                return True
            if self.__eof:
                return bool(self.__read_buf)
            try:
                readable, _, _ = select.select([self.__stdout_fd], [], [], 0)
                return bool(readable)
            except (OSError, ValueError):
                return False

    def wait(self, duration: float, **kwargs):
        if self.__simulator is not None:
            if duration == 0 or kwargs.get('maybeoff', False):
                kwargs['nodeid'] = self.__nodeid

            # Virtual time simulation
            self.__simulator.go(duration, **kwargs)
        else:
            # Real time simulation
            time.sleep(duration)

    def close(self):
        assert self.__otcli_proc.stdin is not None
        assert self.__otcli_proc.stdout is not None
        with self.__read_lock:
            self.__closed = True
            if not self.__fds_closed:
                try:
                    os.write(self.__wake_w, b'\x00')
                except OSError:
                    pass
        try:
            self.__otcli_proc.terminate()
            self.__otcli_proc.wait(timeout=2)
        except (subprocess.TimeoutExpired, OSError):
            self.__otcli_proc.kill()
            self.__otcli_proc.wait()
        finally:
            with self.__select_cond:
                while self.__selecting > 0:
                    self.__select_cond.wait()
                if not self.__otcli_proc.stdin.closed:
                    self.__otcli_proc.stdin.close()
                if not self.__otcli_proc.stdout.closed:
                    self.__otcli_proc.stdout.close()
                if not self.__fds_closed:
                    self.__fds_closed = True
                    os.close(self.__wake_r)
                    os.close(self.__wake_w)


class OtCliSim(OtCliPopen):
    """Connector for OT CLI Simulation instances."""

    def __init__(self, executable: str, nodeid: int, simulator: Optional[Simulator]):
        logging.info('%s: executable=%s', self.__class__.__name__, executable)

        proc = subprocess.Popen(args=[executable, str(nodeid)],
                                executable=executable,
                                stdin=subprocess.PIPE,
                                stdout=subprocess.PIPE,
                                encoding='utf-8',
                                bufsize=1024)
        super().__init__(proc, nodeid, simulator)


class OtCliPosix(OtCliPopen):
    """Connector for host POSIX OT CLI instances using Radio URL."""

    def __init__(self,
                 radio_url: str,
                 executable: str = 'ot-cli',
                 extra_args: Optional[List[str]] = None,
                 nodeid: int = 1,
                 simulator: Optional[Simulator] = None):
        logging.info('%s: executable=%s radio_url=%s', self.__class__.__name__, executable, radio_url)
        self._radio_url = radio_url

        args = [executable]
        if extra_args:
            args.extend(extra_args)
        args.append(radio_url)

        proc = subprocess.Popen(args=args,
                                executable=executable,
                                stdin=subprocess.PIPE,
                                stdout=subprocess.PIPE,
                                encoding='utf-8',
                                bufsize=1024)
        super().__init__(proc, nodeid, simulator)

    def __repr__(self):
        return f'OTCliPosix<{self._radio_url}>'


class OtCliAdbSpinelProxy(OtCliPosix):
    """Connector for host POSIX OT CLI over an Android ADB Spinel proxy."""

    def __init__(self,
                 proxy_manager: Any,
                 executable: str = 'ot-cli',
                 extra_args: Optional[List[str]] = None,
                 nodeid: int = 1,
                 simulator: Optional[Simulator] = None):
        self.__proxy_manager = proxy_manager
        super().__init__(radio_url=proxy_manager.radio_url,
                         executable=executable,
                         extra_args=extra_args,
                         nodeid=nodeid,
                         simulator=simulator)

    @property
    def proxy_manager(self) -> Any:
        return self.__proxy_manager

    def close(self):
        try:
            super().close()
        finally:
            self.__proxy_manager.close()


class OtNcpSim(OtCliPopen):
    """Connector for OT NCP Simulation instances."""

    def __init__(self, executable: str, nodeid: int, simulator: Optional[Simulator]):
        logging.info('%s: executable=%s', self.__class__.__name__, executable)

        proc = subprocess.Popen(args=f'spinel-cli.py -p "{executable}" -n {nodeid} 2>&1',
                                stdin=subprocess.PIPE,
                                stdout=subprocess.PIPE,
                                encoding='utf-8',
                                bufsize=1024,
                                shell=True)
        super().__init__(proc, nodeid, simulator)


class OtCliSerial(OtCliHandler):
    """Connector for OT CLI SOC devices via Serial."""

    def __init__(self, dev: str, baudrate: int, timeout: float = 0.1):
        self.__dev = dev
        self.__baudrate = baudrate

        import serial
        self.__serial = serial.Serial(self.__dev, self.__baudrate, timeout=timeout, exclusive=True)
        self.writeline('\r\n')
        self.__linebuffer = b''

    def __repr__(self):
        return self.__dev

    def readline(self) -> Optional[str]:
        while self.__serial.is_open:
            line = self.__serial.readline()

            if not line.endswith(b'\n'):
                self.__linebuffer += line
            else:
                line = self.__linebuffer + line
                self.__linebuffer = b''

                return line.decode('utf-8', errors='ignore').rstrip('\r\n')

        return None

    def writeline(self, s: str):
        self.__serial.write((s + '\r\n').encode('utf-8'))

    def wait(self, duration: float, **kwargs):
        time.sleep(duration)

    def close(self):
        self.__serial.close()
