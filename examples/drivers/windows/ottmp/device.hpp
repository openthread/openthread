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
 *   Header file for the routines related to WDFDEVICE representing the
 *   Network Adapter
 */

#pragma once

#define OTTMP_DEVICE_CONTEXT_SIGNATURE 'veDt'
typedef struct _OTTMP_DEVICE_CONTEXT {
    //
    // Signature for Sanity Check.
    //
    ULONG                     Signature;

    //
    // Handle to the WDFDEVICE of which this is the context
    //
    WDFDEVICE                 Device;

    //
    // Pointer to the Context of the corresponding NETADAPTER object
    //
    POTTMP_ADAPTER_CONTEXT AdapterContext;

} OTTMP_DEVICE_CONTEXT, *POTTMP_DEVICE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(OTTMP_DEVICE_CONTEXT, GetDeviceContext);

extern "C" {
#ifdef OTTMP_LEGACY
PAGED MINIPORT_INITIALIZE                    MPInitializeEx;
PAGED MINIPORT_HALT                          MPHaltEx;
MINIPORT_SHUTDOWN                            MPShutdownEx;
PAGED MINIPORT_DEVICE_PNP_EVENT_NOTIFY       MPDevicePnpEventNotify;
#else
PAGED EVT_WDF_DRIVER_DEVICE_ADD              EvtDriverDeviceAdd;
PAGED EVT_WDF_DEVICE_D0_EXIT                 EvtDeviceD0Exit;
PAGED EVT_WDF_DEVICE_PREPARE_HARDWARE        EvtDevicePrepareHardware;
PAGED EVT_WDF_DEVICE_RELEASE_HARDWARE        EvtDeviceReleaseHardware;
PAGED EVT_WDF_DEVICE_SELF_MANAGED_IO_INIT    EvtDeviceSelfManagedIoInit;
PAGED EVT_WDF_DEVICE_SELF_MANAGED_IO_CLEANUP EvtDeviceSelfManagedIoCleanup;
EVT_WDF_DEVICE_D0_ENTRY                      EvtDeviceD0Entry;
#endif
}
