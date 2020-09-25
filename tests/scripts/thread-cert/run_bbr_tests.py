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
import subprocess
import sys
import traceback
from collections import Counter

import config

MULTIPLE_JOBS = 10

logging.basicConfig(level=logging.DEBUG,
                    format='File "%(pathname)s", line %(lineno)d, in %(funcName)s\n'
                    '%(asctime)s - %(levelname)s - %(message)s')


def bash(cmd: str, check=True, stdout=None):
    subprocess.run(cmd, shell=True, check=check, stdout=stdout)


def run_bbr_test(port_offset: int, script: str):
    try:
        logging.info("Running BBR test: %s ...", script)
        test_name = os.path.splitext(os.path.basename(script))[0] + '_' + str(port_offset)
        logfile = test_name + '.log'
        env = os.environ.copy()
        env['PORT_OFFSET'] = str(port_offset)
        env['TEST_NAME'] = test_name

        try:
            with open(logfile, 'wt') as output:
                subprocess.check_call(["python3", script],
                                      stdout=output,
                                      stderr=output,
                                      stdin=subprocess.DEVNULL,
                                      env=env)
        except subprocess.CalledProcessError:
            bash(f'cat {logfile} 1>&2')
            logging.error("Run test %s failed, please check the log file: %s", test_name, logfile)
            raise

    except Exception:
        traceback.print_exc()
        raise


pool = multiprocessing.Pool(processes=MULTIPLE_JOBS)


def cleanup_env():
    logging.info("Cleaning up Backbone testing environment ...")
    bash('pkill socat 2>/dev/null || true')
    bash('pkill dumpcap 2>/dev/null || true')
    bash(f'docker rm -f $(docker ps -a -q -f "name=otbr_") 2>/dev/null || true')
    bash(f'docker network rm $(docker network ls -q -f "name=backbone") 2>/dev/null || true')


def setup_env():
    bash(f'docker image inspect {config.OTBR_DOCKER_IMAGE} >/dev/null')
    bash('mkdir build || true')


def parse_args():
    import argparse
    parser = argparse.ArgumentParser(description='Process some integers.')
    parser.add_argument('--multiply', type=int, default=1, help='run each test for multiple times')
    parser.add_argument("scripts", nargs='+', type=str, help='specify Backbone test scripts')

    args = parser.parse_args()
    logging.info("Multiply: %d", args.multiply)
    logging.info("Test scripts: %s", args.scripts)
    return args


def main():
    args = parse_args()
    cleanup_env()
    setup_env()

    script_fail_count = Counter()

    def error_callback(script, err):
        logging.error("Test %s failed: %s", script, err)
        script_fail_count[script] += 1

    # Run each script for multiple times
    scripts = args.scripts * args.multiply

    for i, script in enumerate(scripts):
        pool.apply_async(run_bbr_test, [i, script],
                         error_callback=lambda err, script=script: error_callback(script, err))

    pool.close()

    logging.info("Waiting for tests to complete ...")
    pool.join()

    cleanup_env()

    for script in args.scripts:
        logging.info("Test %s: %d PASS/%d TOTAL", script, args.multiply - script_fail_count[script], args.multiply)

    exit(len(script_fail_count))


if __name__ == '__main__':
    main()
