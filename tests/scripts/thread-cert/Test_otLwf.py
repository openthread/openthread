#!/usr/bin/env python
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

import unittest
import ctypes

class GUID(ctypes.Structure):
    _fields_ = [("Data1", ctypes.c_uint),
                ("Data2", ctypes.c_ushort),
                ("Data3", ctypes.c_ushort),
                ("Data4", ctypes.c_ubyte * 8)]
    
class otDeviceList(ctypes.Structure):
    _fields_ = [("aDevicesLength", ctypes.c_ushort),
                ("aDevices", GUID * 64)]

class Cert_otLwf(unittest.TestCase):
    def setUp(self):

        # Load the DLL
        self.Api = ctypes.WinDLL("otApi.dll")
        if self.Api == None:
            raise OSError("Failed to load otApi.dll!")
        
        # Define the functions
        self.Api.otApiInit.restype = ctypes.c_void_p
        self.Api.otApiFinalize.argtypes = [ctypes.c_void_p]
        self.Api.otApiFinalize.restype = None
        self.Api.otFreeMemory.argtypes = [ctypes.c_void_p]
        self.Api.otFreeMemory.restype = None
        self.Api.otEnumerateDevices.argtypes = [ctypes.c_void_p]
        self.Api.otEnumerateDevices.restype = ctypes.POINTER(otDeviceList)

    def tearDown(self):
        if self.ApiInstance:
            self.Api.otApiFinalize(self.ApiInstance)

    def test(self):
        # Instantiate the API
        self.ApiInstance = self.Api.otApiInit()
        # Assert that it didn't return NULL
        self.assertNotEqual(self.ApiInstance, None)
        # Query the device list
        devices = self.Api.otEnumerateDevices(self.ApiInstance)
        # Assert that it didn't return NULL
        self.assertNotEqual(devices, None)
        # Print the number of devices
        print("devices found: %d" % devices.contents.aDevicesLength)

if __name__ == '__main__':
    unittest.main()
