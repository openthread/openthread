"""
  Copyright (c) 2024, The OpenThread Authors.
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:
  1. Redistributions of source code must retain the above copyright
     notice, this list of conditions and the following disclaimer.
  2. Redistributions in binary form must reproduce the above copyright
     notice, this list of conditions and the following disclaimer in the
     documentation and/or other materials provided with the distribution.
  3. Neither the name of the copyright holder nor the
     names of its contributors may be used to endorse or promote products
     derived from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  POSSIBILITY OF SUCH DAMAGE.
"""

import base64


def get_int_in_range(min_value, max_value):
    while True:
        try:
            user_input = int(input('> '))
            if min_value <= user_input <= max_value:
                return user_input
            else:
                print('The value is out of range. Try again.')
        except ValueError:
            print('The value is not an integer. Try again.')
        except KeyboardInterrupt:
            quit_with_reason('Program interrupted by user. Quitting.')


def quit_with_reason(reason):
    print(reason)
    exit(1)


def select_device_by_user_input(tcat_devices):
    if tcat_devices:
        print('Found devices:\n')
        for i, device in enumerate(tcat_devices):
            print(f'{i + 1}: {device.name} - {device.address}')
    else:
        print('\nNo devices found.')
        return None

    print('\nSelect the target number to connect to it.')
    selected = get_int_in_range(1, len(tcat_devices))
    device = tcat_devices[selected - 1]
    print('Selected ', device)

    return device


def base64_string(bindata):
    return base64.b64encode(bindata).decode('ascii')


def load_cert_pem(fn):
    with open(fn, 'r') as file:
        return file.read()
