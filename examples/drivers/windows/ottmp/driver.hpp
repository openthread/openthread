/*
 *    Copyright (c) 2016, The OpenThread Authors.
 *    All rights reserved.
 *
 *    Redistribution and use in source and binary forms, with or without
 *    modification, are permitted provided that the following conditions are met:
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *    3. Neither the name of the copyright holder nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 *    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 *    ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 *    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
 *    DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 *    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 *    ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 *    SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file
 *   Header file for the Driver Load / Unload routines
 */

#pragma once

extern "C" {
DRIVER_INITIALIZE DriverEntry;
}

#ifdef OTTMP_LEGACY
PAGED MINIPORT_UNLOAD MPDriverUnload;

typedef struct _GLOBALS
{
    WDFDRIVER        WdfDriver;
    NDIS_HANDLE      hDriver;
    NDIS_HANDLE      hNblPool;
    NDIS_HANDLE      hNbPool;
} GLOBALS, *PGLOBALS;
#else
PAGED EVT_WDF_DRIVER_UNLOAD EvtDriverUnload;
#endif

//
// Own Version
//
#define NIC_VENDOR_DRIVER_VERSION_MAJOR  1
#define NIC_VENDOR_DRIVER_VERSION_MINOR  0
#define NIC_VENDOR_DRIVER_VERSION ((NIC_VENDOR_DRIVER_VERSION_MAJOR << 16) | NIC_VENDOR_DRIVER_VERSION_MINOR)
