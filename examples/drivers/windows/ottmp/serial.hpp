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

#pragma once

PAGED
_No_competing_thread_
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
SerialInitialize(
    _In_ POTTMP_ADAPTER_CONTEXT     AdapterContext
    );

PAGED
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
SerialUninitialize(
    _In_ POTTMP_ADAPTER_CONTEXT     AdapterContext
    );

PAGED
_No_competing_thread_
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
SerialInitializeTarget(
    _In_ POTTMP_ADAPTER_CONTEXT     AdapterContext,
    _In_ PCWSTR                     TargetName
);

PAGED
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
SerialUninitializeTarget(
    _In_ POTTMP_ADAPTER_CONTEXT     AdapterContext
);

PAGED
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
SerialConfigure(
    _In_ POTTMP_ADAPTER_CONTEXT     AdapterContext
    );

PAGED
_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
SerialCheckStatus(
    _In_ POTTMP_ADAPTER_CONTEXT     AdapterContext,
    _In_ bool                       DataExpected
    );

PAGED
_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
SerialFlushAndCheckStatus(
    _In_ POTTMP_ADAPTER_CONTEXT     AdapterContext
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
SerialSendData(
    _In_ POTTMP_ADAPTER_CONTEXT     AdapterContext,
    _In_ PNET_BUFFER_LIST           NetBufferLists
    );

PAGED
_Function_class_(EVT_WDF_WORKITEM)
_IRQL_requires_same_
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
SerialSendLoop(
    _In_ WDFWORKITEM                WorkItem
    );

PAGED
_Function_class_(EVT_WDF_WORKITEM)
_IRQL_requires_same_
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
SerialRecvLoop(
    _In_ WDFWORKITEM                WorkItem
    );

_Function_class_(EVT_WDF_REQUEST_COMPLETION_ROUTINE)
_IRQL_requires_same_
VOID
SerialRecvComplete(
    _In_ WDFREQUEST                 Request,
    _In_ WDFIOTARGET                Target,
    _In_ PWDF_REQUEST_COMPLETION_PARAMS Params,
    _In_ WDFCONTEXT                 Context
    );

VOID 
DumpBuffer(
    _In_reads_bytes_(aLength) PCUCHAR aBuf, 
    _In_ size_t                     aLength
    );
