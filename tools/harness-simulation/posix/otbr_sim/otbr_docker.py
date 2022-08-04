#!/usr/bin/env python3
#
#  Copyright (c) 2022, The OpenThread Authors.
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
import re
import subprocess
import time


class OtbrDocker:
    device_pattern = re.compile('(?<=PTY is )/dev/.+$')

    def __init__(self, nodeid: int, ot_path: str, ot_rcp_path: str, docker_image: str, docker_name: str):
        self.nodeid = nodeid
        self.ot_path = ot_path
        self.ot_rcp_path = ot_rcp_path
        self.docker_image = docker_image
        self.docker_name = docker_name

        self.logger = logging.getLogger('otbr_docker.OtbrDocker')
        self.logger.setLevel(logging.INFO)

        self._socat_proc = None
        self._ot_rcp_proc = None

        self._rcp_device_pty = None
        self._rcp_device = None

        self._launch()

    def __repr__(self) -> str:
        return f'OTBR<{self.nodeid}>'

    def _launch(self):
        self.logger.info('Launching %r ...', self)
        self._launch_socat()
        self._launch_ot_rcp()
        self._launch_docker()
        self.logger.info('Launched %r successfully', self)

    def close(self):
        self.logger.info('Shutting down %r ...', self)
        self._shutdown_docker()
        self._shutdown_ot_rcp()
        self._shutdown_socat()
        self.logger.info('Shut down %r successfully', self)

    def _launch_socat(self):
        self._socat_proc = subprocess.Popen(['socat', '-d', '-d', 'pty,raw,echo=0', 'pty,raw,echo=0'],
                                            stderr=subprocess.PIPE,
                                            stdin=subprocess.DEVNULL,
                                            stdout=subprocess.DEVNULL)

        line = self._socat_proc.stderr.readline().decode('ascii').strip()
        self._rcp_device_pty = self.device_pattern.findall(line)[0]
        line = self._socat_proc.stderr.readline().decode('ascii').strip()
        self._rcp_device = self.device_pattern.findall(line)[0]
        self.logger.info(f"socat running: device PTY: {self._rcp_device_pty}, device: {self._rcp_device}")

    def _shutdown_socat(self):
        if self._socat_proc is None:
            return

        self._socat_proc.stderr.close()
        self._socat_proc.terminate()
        self._socat_proc.wait()
        self._socat_proc = None

        self._rcp_device_pty = None
        self._rcp_device = None

    def _launch_ot_rcp(self):
        self._ot_rcp_proc = subprocess.Popen(
            f'{self.ot_rcp_path} {self.nodeid} > {self._rcp_device_pty} < {self._rcp_device_pty}',
            shell=True,
            stdin=subprocess.DEVNULL,
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL)
        try:
            self._ot_rcp_proc.wait(1)
        except subprocess.TimeoutExpired:
            # We expect ot-rcp not to quit in 1 second.
            pass
        else:
            raise Exception(f"ot-rcp {self.nodeid} exited unexpectedly!")

    def _shutdown_ot_rcp(self):
        if self._ot_rcp_proc is None:
            return

        self._ot_rcp_proc.terminate()
        self._ot_rcp_proc.wait()
        self._ot_rcp_proc = None

    def _launch_docker(self):
        local_cmd_path = f'/tmp/{self.docker_name}'
        os.makedirs(local_cmd_path, exist_ok=True)

        cmd = [
            'docker',
            'run',
            '--rm',
            '--name',
            self.docker_name,
            '-d',
            '--sysctl',
            'net.ipv6.conf.all.disable_ipv6=0 net.ipv4.conf.all.forwarding=1 net.ipv6.conf.all.forwarding=1',
            '--privileged',
            '-v',
            f'{self._rcp_device}:/dev/ttyUSB0',
            '-v',
            f'{self.ot_path.rstrip("/")}:/home/pi/repo/openthread',
            self.docker_image,
        ]
        self.logger.info('Launching docker:  %s', ' '.join(cmd))
        launch_proc = subprocess.Popen(cmd,
                                       stdin=subprocess.DEVNULL,
                                       stdout=subprocess.DEVNULL,
                                       stderr=subprocess.DEVNULL)

        launch_docker_deadline = time.time() + 60
        launch_ok = False
        time.sleep(5)

        while time.time() < launch_docker_deadline:
            try:
                subprocess.check_call(['docker', 'exec', self.docker_name, 'ot-ctl', 'state'],
                                      stdin=subprocess.DEVNULL,
                                      stdout=subprocess.DEVNULL,
                                      stderr=subprocess.DEVNULL)
                launch_ok = True
                logging.info("OTBR Docker %s is ready!", self.docker_name)
                break
            except subprocess.CalledProcessError:
                time.sleep(5)
                continue

        if not launch_ok:
            raise RuntimeError('Cannot start OTBR Docker %s!' % self.docker_name)
        launch_proc.wait()

    def _shutdown_docker(self):
        subprocess.run(['docker', 'stop', self.docker_name])
