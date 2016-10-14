/*
 *  Copyright (c) 2016, The OpenThread Authors.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. Neither the name of the copyright holder nor the
 *     names of its contributors may be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file
 * @brief
 *  This file defines the top-level functions and variables for driver initialization
 *  and clean up.
 */

#ifndef _DRIVER_H
#define _DRIVER_H

// Legal values include:
//    6.0  Available starting with Windows Vista RTM
//    6.1  Available starting with Windows Vista SP1 / Windows Server 2008
//    6.20 Available starting with Windows 7 / Windows Server 2008 R2
//    6.30 Available starting with Windows 8 / Windows Server "8"
#define FILTER_MAJOR_NDIS_VERSION   6

#if defined(NDIS60)
#define FILTER_MINOR_NDIS_VERSION   0
#elif defined(NDIS620)
#define FILTER_MINOR_NDIS_VERSION   20
#elif defined(NDIS630)
#define FILTER_MINOR_NDIS_VERSION   30
#endif

//
// Global variables
//

// Global Driver Object from DriverEntry
extern PDRIVER_OBJECT      FilterDriverObject;

// NDIS Filter handle from NdisFRegisterFilterDriver
extern NDIS_HANDLE         FilterDriverHandle;

// IoControl Device Object from IoCreateDeviceSecure
extern PDEVICE_OBJECT      IoDeviceObject;

// Global list of THREAD_FILTER instances
extern NDIS_SPIN_LOCK      FilterListLock;
extern LIST_ENTRY          FilterModuleList;

// Cached performance frequency of the system
extern LARGE_INTEGER       FilterPerformanceFrequency;

#define FILTER_FRIENDLY_NAME        L"OpenThread NDIS LightWeight Filter"
#define FILTER_UNIQUE_NAME          L"{B3A3845A-164E-4727-B12E-32B8DCE1F6CD}" //unique name, quid name
#define FILTER_SERVICE_NAME         L"OTLWF"

//
// Function prototypes
//
INITCODE DRIVER_INITIALIZE DriverEntry;

PAGEDX DRIVER_UNLOAD DriverUnload;

#endif // _DRIVER_H
