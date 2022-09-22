#!/usr/bin/env python
#
# Copyright (c) 2022, The OpenThread Authors.
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

# This script merges the device fields in the argument file to `deviceInputFields.xml` in Harness
# If a device field appears in both files, it will only keep that in the argument file

import os
import sys
import xml.etree.ElementTree as ET

HARNESS_XML_PATH = r'%s\GRL\Thread1.2\Web\data\deviceInputFields.xml' % os.environ['systemdrive']


def main():
    tree = ET.parse(HARNESS_XML_PATH)
    root = tree.getroot()
    added = ET.parse(sys.argv[1]).getroot()

    added_names = set(device.attrib['name'] for device in added.iter('DEVICE'))
    # If some devices already exist, remove them first, and then add them back in case of update
    removed_devices = filter(lambda x: x.attrib['name'] in added_names, root.iter('DEVICE'))

    for device in removed_devices:
        root.remove(device)
    for device in added.iter('DEVICE'):
        root.append(device)

    tree.write(HARNESS_XML_PATH)


if __name__ == '__main__':
    main()
