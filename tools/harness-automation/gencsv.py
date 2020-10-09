#!/usr/bin/env python
#
# Copyright (c) 2020, The OpenThread Authors.
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

import argparse
import json


def get_key(item):
    ds = item[0].split('_')
    no = int(ds[1]) * 10000 + int(ds[2]) * 100 + int(ds[3])
    ro = ds[0]
    return '%d-%s' % (no, ro)


def get_case_id(scenario):
    sg = scenario.split('_')
    case_id = '.'.join(x for x in sg[1:])
    return case_id


def get_case_role(scenario):
    sg = scenario.split('_')
    return sg[0]


def main():
    parser = argparse.ArgumentParser(description='convert result.json to result.csv')
    parser.add_argument(
        '-t',
        dest='test_date',
        help='Date of testing',
    )

    parser.add_argument(
        '-c',
        dest='commit_id',
        help='OpenThread commit id',
    )

    parser.add_argument(
        '-v',
        dest='harness_version',
        help='Test Harness Version',
    )

    parser.add_argument(
        '-d',
        dest='dut',
        help='DUT device',
    )

    parser.add_argument(
        '-l',
        dest='topo_file',
        help='Topology configuration file name',
    )

    parser.add_argument(
        '-f',
        dest='result_json',
        default='./result.json',
        help='Result json file',
    )

    args = parser.parse_args()
    result = json.load(open(args.result_json, 'r'))
    with open('./result.csv', 'w') as o:
        test_info = ''
        empty_field = ''
        if args.test_date:
            o.write('Test Date,')
            test_info += args.test_date + ','
            empty_field += ','

        if args.commit_id:
            o.write('OT Commit ID,')
            test_info += args.commit_id + ','
            empty_field += ','

        if args.harness_version:
            o.write('Harness Version,')
            test_info += args.harness_version + ','
            empty_field += ','

        if args.dut:
            o.write('DUT,')
            test_info += args.dut + ','
            empty_field += ','

        if args.topo_file:
            o.write('Topology File,')
            test_info += args.topo_file + ','
            empty_field += ','

        o.write('Test Case,DUT Role,Status,Logs,Started,Stopped\n')
        if test_info is not None:
            o.write('%s' % test_info)
        for k, v in sorted(result.items(), key=get_key):
            o.write('%s,%s,%s,%s,%s,%s\n' % (
                get_case_id(k),
                get_case_role(k),
                (v['passed'] and 'Pass') or 'Fail',
                (v['error'] or '').replace('\n', ' '),
                v['started'],
                v['stopped'],
            ))
            if empty_field is not None:
                o.write('%s' % empty_field)


if __name__ == '__main__':
    main()
