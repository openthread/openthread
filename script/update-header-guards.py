#!/usr/bin/env python3
#
#  Copyright (c) 2025, The OpenThread Authors.
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

# This script updates the header guards in .hpp and .h files within the `src`
# `include/openthread`, and `tests` directories. It ensures that header
# guards follow a consistent naming convention based on the file path.

import os

#----------------------------------------------------------------------------------------------
# Helper functions


def get_header_file_list(path):
    """Get a sorted list of full file names (with path) in a given `path` folder having either `.h` or `.hpp` extension"""
    return sorted([
        "{}/{}".format(dir_path, file_name)[2:]
        for dir_path, dir_names, file_names in os.walk('./' + path)
        for file_name in file_names
        if file_name.endswith('.hpp') or file_name.endswith('.h')
    ])


def read_txt_file(file_name):
    """Read the content of a text file with name `file_name` and return content as a list of lines"""
    with open(file_name, 'r') as file:
        lines = file.readlines()
    return lines


def write_txt_file(file_name, lines):
    """Write a text file with name `file_name` with the content given as a list of `lines`"""
    with open(file_name, 'w') as file:
        file.writelines(lines)


def update_header_guard_in(file_name, replace_str, new_str):
    """Update header guard in a given file.

    Args:
        file_name (str): The name of the file to update.
        replace_str (str): The prefix string to replace in the file path to generate header guard name
        new_str (str): The new string to use as a prefix for the header guard.
    """
    STATE_SEARCH = 1
    STATE_MATCH_START = 2
    STATE_DONE = 3

    # Strip and replace the `replace_str` with `new_str`.
    # Then replace '/','.', or `-` with '_', then format.

    guard_name = file_name.replace(replace_str, new_str, 1)
    guard_name = guard_name.replace('/', '_').replace('-', '_').replace('.', '_').upper() + '_'

    lines = read_txt_file(file_name)
    new_lines = []
    state = STATE_SEARCH

    for line in lines[:-1]:
        if state == STATE_SEARCH:
            if line.startswith('#ifndef '):
                state = STATE_MATCH_START
                line = '#ifndef ' + guard_name + '\n'
            new_lines.append(line)
        elif state == STATE_MATCH_START:
            if not line.startswith('#define'):
                raise RuntimeError('missing header guard #define in: {}'.format(file_name))
            line = '#define ' + guard_name + '\n'
            new_lines.append(line)
            state = STATE_DONE
        else:
            new_lines.append(line)

    if state != STATE_DONE:
        raise RuntimeError('missing header guard #ifndef in: {}'.format(file_name))

    if not lines[-1].startswith('#endif'):
        raise RuntimeError('missing header guard #endif in: {}'.format(file_name))

    new_lines.append('#endif // ' + guard_name + '\n')

    if new_lines != lines:
        write_txt_file(file_name, new_lines)
        print("- Updated {} ({})".format(file_name, guard_name))


#----------------------------------------------------------------------------------------------

# Check and update header guards in various folders

folders = [('src', 'OT'), ('include/openthread', 'OPENTHREAD'), ('tests', 'OT')]

for folder in folders:
    path = folder[0]
    guard_prefix = folder[1]

    header_file_list = get_header_file_list(path)

    print('Updating header guards in ./{}/'.format(path))

    for file_name in header_file_list:
        update_header_guard_in(file_name, path, guard_prefix)
