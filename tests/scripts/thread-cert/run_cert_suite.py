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
import multiprocessing
import os
import queue
import subprocess
import time
import traceback
from collections import Counter, defaultdict
from typing import List, Dict

import config

THREAD_VERSION = os.getenv('THREAD_VERSION')
VIRTUAL_TIME = int(os.getenv('VIRTUAL_TIME', '1'))
MAX_JOBS = int(os.getenv('MAX_JOBS', (multiprocessing.cpu_count() * 2 if VIRTUAL_TIME else 10)))

_BACKBONE_TESTS_DIR = 'tests/scripts/thread-cert/backbone'

_COLOR_PASS = '\033[0;32m'
_COLOR_FAIL = '\033[0;31m'
_COLOR_NONE = '\033[0m'

logging.basicConfig(level=logging.DEBUG,
                    format='File "%(pathname)s", line %(lineno)d, in %(funcName)s\n'
                    '%(asctime)s - %(levelname)s - %(message)s')


def bash(cmd: str, check=True, stdout=None):
    subprocess.run(cmd, shell=True, check=check, stdout=stdout)


def run_cert(iteration_id: int, port_offset: int, script: str, run_directory: str):
    if not os.access(script, os.X_OK):
        logging.warning('Skip test %s, not executable', script)
        return

    try:
        test_name = os.path.splitext(os.path.basename(script))[0] + '_' + str(iteration_id)
        logfile = f'{run_directory}/{test_name}.log' if run_directory else f'{test_name}.log'
        env = os.environ.copy()
        env['PORT_OFFSET'] = str(port_offset)
        env['TEST_NAME'] = test_name
        env['PYTHONPATH'] = os.path.dirname(os.path.abspath(__file__))

        try:
            print(f'Running PORT_OFFSET={port_offset} {test_name}')
            with open(logfile, 'wt') as output:
                abs_script = os.path.abspath(script)
                subprocess.check_call(abs_script,
                                      stdout=output,
                                      stderr=output,
                                      stdin=subprocess.DEVNULL,
                                      cwd=run_directory,
                                      env=env)
        except subprocess.CalledProcessError:
            bash(f'cat {logfile} 1>&2')
            logging.error("Run test %s failed, please check the log file: %s", test_name, logfile)
            raise

    except Exception:
        traceback.print_exc()
        raise


pool = multiprocessing.Pool(processes=MAX_JOBS)


def cleanup_backbone_env():
    logging.info("Cleaning up Backbone testing environment ...")
    bash('pkill socat 2>/dev/null || true')
    bash('pkill dumpcap 2>/dev/null || true')
    bash(f'docker rm -f $(docker ps -a -q -f "name=otbr_") 2>/dev/null || true')
    bash(f'docker network rm $(docker network ls -q -f "name=backbone") 2>/dev/null || true')


def setup_backbone_env():
    if THREAD_VERSION == '1.1':
        raise RuntimeError('Backbone tests do not work with THREAD_VERSION=1.1')

    if VIRTUAL_TIME:
        raise RuntimeError('Backbone tests only work with VIRTUAL_TIME=0')

    bash(f'docker image inspect {config.OTBR_DOCKER_IMAGE} >/dev/null')


def parse_args():
    import argparse
    parser = argparse.ArgumentParser(description='Process some integers.')
    parser.add_argument('--multiply', type=int, default=1, help='run each test for multiple times')
    parser.add_argument('--run-directory', type=str, default=None, help='run each test in the specified directory')
    parser.add_argument("scripts", nargs='+', type=str, help='specify Backbone test scripts')

    args = parser.parse_args()
    logging.info("Max jobs: %d", MAX_JOBS)
    logging.info("Run directory: %s", args.run_directory or '.')
    logging.info("Multiply: %d", args.multiply)
    logging.info("Test scripts: %d", len(args.scripts))
    return args


def check_has_backbone_tests(scripts):
    for script in scripts:
        relpath = os.path.relpath(script, _BACKBONE_TESTS_DIR)
        if not relpath.startswith('..'):
            return True

    return False


class PortOffsetPool:

    def __init__(self, size: int):
        self._size = size
        self._pool = queue.Queue(maxsize=size)
        for port_offset in range(0, size):
            self.release(port_offset)

    def allocate(self) -> int:
        return self._pool.get()

    def release(self, port_offset: int):
        assert 0 <= port_offset < self._size, port_offset
        self._pool.put_nowait(port_offset)


def print_summary(scripts: List[str], script_successes: Dict[str, List[int]], script_failures: Dict[str, List[int]]):
    print("---------------------------------------")
    print("Summary")
    print("---------------------------------------")
    for script in scripts:
        success_count = len(script_successes[script])
        failure_count = len(script_failures[script])
        color = _COLOR_PASS if failure_count == 0 else _COLOR_FAIL
        message = f'{color}PASS {success_count} FAIL {failure_count}{_COLOR_NONE} {script}'
        if failure_count > 0:
            message += f' {_COLOR_FAIL}Failed iterations: {script_failures[script]}{_COLOR_NONE}'
        print(message)


def run_tests(scripts: List[str], multiply: int = 1, run_directory: str = None):
    scripts = list(set(scripts))

    # Run each script for multiple times
    script_ids = [(script, i) for script in scripts for i in range(multiply)]
    port_offset_pool = PortOffsetPool(MAX_JOBS)

    # From the test script path to the iteration IDs
    script_failures: Dict[str, List[int]] = defaultdict(list)
    script_successes: Dict[str, List[int]] = defaultdict(list)

    def result_callback(iteration_id, script, dic, port_offset):
        port_offset_pool.release(port_offset)
        dic[script].append(iteration_id)

    for script, i in script_ids:
        port_offset = port_offset_pool.allocate()
        pool.apply_async(run_cert, [i, port_offset, script, run_directory],
                         callback=lambda ret, id=i, script=script, port_offset=port_offset: result_callback(
                             id, script, script_successes, port_offset),
                         error_callback=lambda ret, id=i, script=script, port_offset=port_offset: result_callback(
                             id, script, script_failures, port_offset))

    pool.close()
    pool.join()

    print_summary(scripts, script_successes, script_failures)
    return sum(len(l) for l in script_failures.values())


def main():
    args = parse_args()

    has_backbone_tests = check_has_backbone_tests(args.scripts)
    logging.info('Has Backbone tests: %s', has_backbone_tests)

    if has_backbone_tests:
        cleanup_backbone_env()
        setup_backbone_env()

    try:
        fail_count = run_tests(args.scripts, args.multiply, args.run_directory)
        exit(fail_count)
    finally:
        if has_backbone_tests:
            cleanup_backbone_env()


if __name__ == '__main__':
    main()
