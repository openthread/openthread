#!/usr/bin/env python
#
# Copyright (c) 2016, Nest Labs, Inc.
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
#


import logging
import os
import subprocess
import time

from autothreadharness import settings

logger = logging.getLogger(__name__)

class HarnessController(object):
    """Harness service control

    This controls harness service, including the harness back-end and front-end.
    """
    harness = None
    """harness back-end"""

    miniweb = None
    """harness front-end"""

    def __init__(self, result_dir=None):
        self.result_dir = result_dir

    def start(self):
        logger.info('Starting harness service')
        if self.harness:
            logger.warning('Harness already started')
        else:
            env = dict(os.environ, PYTHONPATH='%s\\Thread_Harness;%s\\ThirdParty\\hsdk-python\\src'
                       % (settings.HARNESS_HOME, settings.HARNESS_HOME))
            with open('%s\\harness-%s.log' % (self.result_dir, time.strftime('%Y%m%d%H%M%S')), 'w') as fout:
                self.harness = subprocess.Popen([settings.HARNESS_HOME + '\\Python27\\python.exe',
                                                 settings.HARNESS_HOME + '\\Thread_Harness\\Run.py'],
                                                cwd=settings.HARNESS_HOME,
                                                stdout=fout,
                                                stderr=fout,
                                                env=env)

        if self.miniweb:
            logger.warning('Miniweb already started')
        else:
            self.miniweb = subprocess.Popen([settings.HARNESS_HOME + '\\miniweb\\miniweb.exe'],
                                        cwd=settings.HARNESS_HOME + '\\miniweb')

    def stop(self):
        logger.info('Stopping harness service')

        if self.harness:
            self._try_kill(self.harness)
            self.harness = None
        else:
            logger.warning('Harness not started yet')

        if self.miniweb:
            self._try_kill(self.miniweb)
            self.miniweb = None
        else:
            logger.warning('Miniweb not started yet')

    def _try_kill(self, proc):
        logger.info('Try kill process')
        times = 3
        proc.kill()
        while proc.poll() is None:
            logger.info('Trial %d failed', times)
            if times == 0:
                raise SystemExit()
            else:
                times = times - 1
                time.sleep(5)
                proc.kill()

    def __del__(self):
        self.stop()

if __name__ == '__main__':
    hc = HarnessController()
