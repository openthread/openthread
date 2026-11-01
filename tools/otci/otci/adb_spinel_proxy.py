#!/usr/bin/env python3
#
#  Copyright (c) 2026, The OpenThread Authors.
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

import atexit
import logging
import os
import subprocess
import tempfile
import time
from typing import List, Optional


class AdbSpinelProxyManager:
    """Manages an Android ADB Spinel proxy lifecycle and host Unix socket forwarding.

    Automates:
      1. Optional `adb connect` (for TCP targets), `adb root`, and `adb wait-for-device`.
      2. Setting `debug.thread_network.spinel_proxy=1` on Android and restarting `ot-daemon`
         so it enters SpinelProxy diagnostic mode.
      3. Forwarding host Unix domain socket to device abstract socket:
         `adb forward localfilesystem:<socket_path> localabstract:ot_daemon_spinel_proxy`.
      4. Providing `spinel+hdlc+unix://<socket_path>` radio URL for host POSIX `ot-cli`.
      5. Clean teardown on `close()` restoring Android system properties and restarting normal ot-daemon.
    """

    SPINEL_PROXY_PROPERTY = "debug.thread_network.spinel_proxy"
    ABSTRACT_SOCKET_NAME = "ot_daemon_spinel_proxy"

    def __init__(self,
                 device_serial: Optional[str] = None,
                 host: Optional[str] = None,
                 port: Optional[int] = 5555,
                 socket_path: Optional[str] = None,
                 adb_path: str = "adb",
                 auto_setup: bool = True,
                 adb_key: Optional[str] = None):
        self.__device_serial = device_serial
        self.__host = host
        self.__port = port
        self.__socket_path = socket_path
        self.__adb_path = adb_path
        self.__auto_setup = auto_setup
        self.__adb_key = adb_key
        self.__forward_active = False
        self.__is_closed = False
        self.__radio_url: Optional[str] = None
        atexit.register(self.close)

    @property
    def radio_url(self) -> str:
        if self.__radio_url is None:
            raise RuntimeError("AdbSpinelProxyManager has not been set up. Call setup() first.")
        return self.__radio_url

    @property
    def socket_path(self) -> Optional[str]:
        return self.__socket_path

    @property
    def is_active(self) -> bool:
        return self.__forward_active and not self.__is_closed

    def _get_target_args(self) -> List[str]:
        if self.__device_serial:
            return ["-s", self.__device_serial]
        elif self.__host:
            device_id = f"{self.__host}:{self.__port}" if self.__port else self.__host
            return ["-s", device_id]
        return []

    def _run_adb(self,
                 args: List[str],
                 timeout: float = 10.0,
                 check: bool = True,
                 include_target: bool = True) -> subprocess.CompletedProcess:
        target_args = self._get_target_args() if include_target else []
        cmd = [self.__adb_path] + target_args + args
        logging.debug("Running ADB command: %s", " ".join(cmd))
        env = os.environ.copy()
        if self.__adb_key:
            env["ADB_VENDOR_KEYS"] = self.__adb_key
        return subprocess.run(cmd,
                              stdout=subprocess.PIPE,
                              stderr=subprocess.PIPE,
                              encoding="utf-8",
                              timeout=timeout,
                              check=check,
                              env=env)

    def setup(self):
        if self.__is_closed:
            raise RuntimeError("Cannot re-setup a closed AdbSpinelProxyManager.")

        if self.__host and not self.__device_serial:
            device_id = f"{self.__host}:{self.__port}" if self.__port else self.__host
            try:
                self._run_adb(["connect", device_id], timeout=10.0, check=False, include_target=False)
            except Exception as e:
                logging.warning("adb connect %s encountered error: %s", device_id, e)

        if self.__auto_setup:
            logging.info("Configuring Android device for Spinel proxy mode...")
            try:
                self._run_adb(["root"], timeout=10.0, check=False)
                self._run_adb(["wait-for-device"], timeout=15.0, check=False)
                self._run_adb(["shell", "setprop", self.SPINEL_PROXY_PROPERTY, "1"], timeout=5.0)
                self._run_adb(["shell", "stop ot-daemon; start ot-daemon"], timeout=10.0)
                # Allow ot-daemon to bind the abstract Unix socket.
                time.sleep(0.8)
            except Exception as e:
                logging.error("Failed to configure Android Spinel proxy: %s", e)
                raise

        if not self.__socket_path:
            target_tag = (self.__device_serial or self.__host or "default").replace(":", "_").replace("/", "_")
            self.__socket_path = os.path.join(tempfile.gettempdir(), f"ot_spinel_proxy_{target_tag}")

        if os.path.exists(self.__socket_path):
            try:
                os.remove(self.__socket_path)
            except OSError as e:
                logging.warning("Could not remove stale socket %s: %s", self.__socket_path, e)

        logging.info("Setting up adb forward localfilesystem:%s localabstract:%s", self.__socket_path,
                     self.ABSTRACT_SOCKET_NAME)
        self._run_adb(
            ["forward", f"localfilesystem:{self.__socket_path}", f"localabstract:{self.ABSTRACT_SOCKET_NAME}"],
            timeout=10.0)
        self.__forward_active = True

        deadline = time.time() + 2.0
        while time.time() < deadline:
            if os.path.exists(self.__socket_path):
                break
            time.sleep(0.05)
        else:
            raise RuntimeError(f"Timeout waiting for ADB forward socket to be created at {self.__socket_path}")

        self.__radio_url = f"spinel+hdlc+unix://{self.__socket_path}"
        logging.info("Android Spinel proxy ready at %s", self.__radio_url)

    def close(self):
        if self.__is_closed:
            return
        self.__is_closed = True
        try:
            atexit.unregister(self.close)
        except Exception:
            pass

        if self.__forward_active and self.__socket_path:
            try:
                self._run_adb(["forward", "--remove", f"localfilesystem:{self.__socket_path}"], check=False)
            except Exception as e:
                logging.warning("Failed to remove adb forward: %s", e)
            self.__forward_active = False

        if self.__socket_path and os.path.exists(self.__socket_path):
            try:
                os.remove(self.__socket_path)
            except OSError:
                pass

        if self.__auto_setup:
            logging.info("Restoring Android device to normal Thread stack mode...")
            try:
                self._run_adb(["shell", "setprop", self.SPINEL_PROXY_PROPERTY, "0"], check=False)
                self._run_adb(["shell", "stop ot-daemon; start ot-daemon"], check=False)
            except Exception as e:
                logging.warning("Failed to restore Android Spinel proxy property: %s", e)

    def __enter__(self):
        try:
            self.setup()
        except Exception:
            self.close()
            raise
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.close()
