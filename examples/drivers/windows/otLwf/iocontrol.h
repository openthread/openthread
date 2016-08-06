/*
 *  Copyright (c) 2016, Microsoft Corporation.
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

#ifndef _IOCONTROL_H
#define _IOCONTROL_H

//
// Function prototype for all Io Control functions
//
typedef 
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
OTLWF_IOCTL_FUNC(
    _In_reads_bytes_(InBufferLength)
            PVOID           InBuffer,
    _In_    ULONG           InBufferLength,
    _Out_writes_bytes_opt_(*OutBufferLength)
            PVOID           OutBuffer,
    _Inout_ PULONG          OutBufferLength
    );

//
// Io Control Functions
//

// Handles queries for the current list of Thread interfaces
OTLWF_IOCTL_FUNC otLwfIoCtlEnumerateInterfaces;

// Handles requests to start a new Thread network
OTLWF_IOCTL_FUNC otLwfIoCtlCreateNetwork;

// Handles requests to join an existing Thread network
OTLWF_IOCTL_FUNC otLwfIoCtlJoinNetwork;

// Handles requests to force a router ID request to the Thread network
OTLWF_IOCTL_FUNC otLwfIoCtlSendRouterIDRequest;

// Handles requests to disconnect an existing Thread network
OTLWF_IOCTL_FUNC otLwfIoCtlDisconnectNetwork;

// Handles requests to query the network addresses of an Interface
OTLWF_IOCTL_FUNC otLwfIoCtlQueryNetworkAddresses;

// Handles requests to query the mesh state of an Interface
OTLWF_IOCTL_FUNC otLwfIoCtlQueryMeshState;

#endif // _IOCONTROL_H
