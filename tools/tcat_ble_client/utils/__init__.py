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

from tlv import advertised_tlv


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
        for i, (device, adv) in enumerate(tcat_devices):
            print(f'{i + 1}: {device.name} - {device.address}')
            print(str(adv))
    else:
        print('\nNo devices found.')
        return None

    print('\nSelect the target number to connect to it.')
    selected = get_int_in_range(1, len(tcat_devices))
    device = tcat_devices[selected - 1][0]
    print('Selected ', device)

    return device


def base64_string(bindata):
    return base64.b64encode(bindata).decode('ascii')


def load_cert_pem(fn):
    with open(fn, 'r') as file:
        return file.read()


def hexdump_ot(text: str, data: bytes, line_prefix: str = "") -> str:
    """
    Formats a byte string into a hex dump format similar to OpenThread logs.

    Args:
        text: String that is printed in the header.
        data: The byte array to format.
        line_prefix: A string to prepend to each line of the output, such as a log timestamp.

    Returns:
        A multi-line string containing the formatted hex dump.
    """
    lines = []
    data_len = len(data)

    # 1. Header
    header = superimpose_centered_string("=" * 72, f"[{text} len={data_len:03d}]")
    lines.append(line_prefix + header)

    # 2. Process data in 16-byte chunks
    chunk_size = 16
    for i in range(0, data_len, chunk_size):
        chunk = data[i:i + chunk_size]

        # Split into two 8-byte hex groups
        hex_part1 = ' '.join(f'{b:02X}' for b in chunk[0:8])
        hex_part2 = ' '.join(f'{b:02X}' for b in chunk[8:16])

        # Create the ASCII representation (replace non-printables with '.')
        ascii_part = ''.join(chr(b) if 32 <= b <= 126 else '.' for b in chunk)

        # Format the complete line with fixed-width padding
        # Width of each hex half: 8 bytes * 2 chars/byte + 7 spaces = 23 characters
        line = f"| {hex_part1:<23} | {hex_part2:<23} | {ascii_part:<16} |"
        lines.append(line_prefix + line)

    # 3. Create the footer line
    footer = "-" * 72
    lines.append(line_prefix + footer)

    return "\n".join(lines)


def superimpose_centered_string(background: str, foreground: str) -> str:
    """
    Superimposes a foreground string onto the center of a background string.

    Args:
        background: The string to use as the background.
        foreground: The string to place on top of the background.

    Returns:
        A new string with the foreground centered within the background.
        If the foreground is longer than or equal to the background's length,
        the foreground string is returned on its own.
    """
    len_bg = len(background)
    len_fg = len(foreground)

    if len_fg >= len_bg:
        return foreground

    start_index = (len_bg - len_fg) // 2
    end_index = start_index + len_fg

    return background[:start_index] + foreground + background[end_index:]
