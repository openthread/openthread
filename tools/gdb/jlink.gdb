#
#  Copyright (c) 2016, The OpenThread Authors.
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

#
# Helper script to ease debugging with JLink and gdb.
#
# USAGE:
#
# # In one terminal, start the JLinkGDBServer
#
# JLinkGDBServer -if swd -device <device_name> &
#
# # Device name examples: CC2538SF53, CC2650F128, DA14680, NRF52
# # Full list found here: https://www.segger.com/jlink_supported_devices.html
#
# # In a second terminal, start gdb with the image name, and run this script.
#
# arm-none-eabi-gdb build/cortex-m4-arm-none-eabi/examples/apps/cli/ot-cli-ftd
# > source tools/gdb/jlink.gdb
#
#
# INSTRUCTIONS FOR USING MULTIPLE PODS:
#
# # Discover USB device numbers
#
# JLinkExe
# J-Link>ShowEmuList
# J-Link[0]: Connection: USB, Serial number: 480065113, ProductName: J-Link-OB
# J-Link[1]: Connection: USB, Serial number: 480065300, ProductName: J-Link-OB
# J-Link>exit
#
# # Then pass extra flags to select USB and gdb port to JLinkGDBServer
#
# JLinkGDBServer -select USB=480065113 -if swd -port 2331
# JLinkGDBServer -select USB=480065300 -if swd -port 2345
# 

target remote:2331
load
monitor reset 
monitor halt
bt
