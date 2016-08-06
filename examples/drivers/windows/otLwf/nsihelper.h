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

/**
 * @file
 * @brief
 *  This file defines the various types and function required to use NSI to
 *  query an interface's compartment ID.
 */

#ifndef _NSI_HELPER_H
#define _NSI_HELPER_H

#define NSISTATUS NTSTATUS

typedef enum _NSI_STORE {
    NsiPersistent,
    // Persists as long as module exists.
    NsiActive,
    NsiBoth,
    NsiCurrent,
    NsiBootFirmwareTable
} NSI_STORE;

typedef enum _NSI_STRUCT_TYPE {
    NsiStructRw,
    NsiStructRoDynamic,
    NsiStructRoStatic,
    NsiMaximumStructType
} NSI_STRUCT_TYPE;

NSISTATUS
NsiGetParameter(
    __in NSI_STORE Store,
    __in PNPI_MODULEID ModuleId,
    __in ULONG ObjectIndex,
    __in_bcount_opt(KeyStructLength) PVOID KeyStruct,
    __in ULONG KeyStructLength,
    __in NSI_STRUCT_TYPE StructType,
    __out_bcount(ParameterLen) PVOID Parameter,
    __in ULONG ParameterLen,
    __in ULONG ParameterOffset
    );

extern CONST NPI_MODULEID NPI_MS_NDIS_MODULEID;

typedef enum _NDIS_NSI_OBJECT_INDEX
{
    NdisNsiObjectInterfaceInformation,
    NdisNsiObjectInterfaceEnum,
    NdisNsiObjectInterfaceLookUp,
    NdisNsiObjectIfRcvAddress,
    NdisNsiObjectStackIfEntry,
    NdisNsiObjectInvertedIfStackEntry,
    NdisNsiObjectNetwork,
    NdisNsiObjectCompartment,
    NdisNsiObjectThread,
    NdisNsiObjectSession,
    NdisNsiObjectInterfacePersist,
    NdisNsiObjectCompartmentLookup,
    NdisNsiObjectInterfaceInformationRaw,
    NdisNsiObjectInterfaceEnumRaw,
    NdisNsiObjectStackIfEnum,
    NdisNsiObjectInterfaceIsolationInfo,
    NdisNsiObjectJob,
    NdisNsiObjectMaximum
} NDIS_NSI_OBJECT_INDEX, *PNDIS_NSI_OBJECT_INDEX;

typedef struct _NDIS_NSI_INTERFACE_INFORMATION_RW
{
    // rw fields
    GUID                        NetworkGuid;
    NET_IF_ADMIN_STATUS         ifAdminStatus;
    NDIS_IF_COUNTED_STRING      ifAlias;
    NDIS_IF_PHYSICAL_ADDRESS    ifPhysAddress;
    NDIS_IF_COUNTED_STRING      ifL2NetworkInfo;
}NDIS_NSI_INTERFACE_INFORMATION_RW, *PNDIS_NSI_INTERFACE_INFORMATION_RW;

#define NDIS_SIZEOF_NSI_INTERFACE_INFORMATION_RW_REVISION_1      \
        RTL_SIZEOF_THROUGH_FIELD(NDIS_NSI_INTERFACE_INFORMATION_RW, ifPhysAddress)

typedef NDIS_INTERFACE_INFORMATION NDIS_NSI_INTERFACE_INFORMATION_ROD, *PNDIS_NSI_INTERFACE_INFORMATION_ROD;

//
// Copied from ndiscomp.h
//

_IRQL_requires_max_(DISPATCH_LEVEL)
EXPORT
COMPARTMENT_ID
NdisGetThreadObjectCompartmentId(
    _In_ PETHREAD ThreadObject
    );

_IRQL_requires_(PASSIVE_LEVEL)
EXPORT
NTSTATUS
NdisSetThreadObjectCompartmentId(
    _In_ PETHREAD ThreadObject,
    _In_ NET_IF_COMPARTMENT_ID CompartmentId
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
__inline
COMPARTMENT_ID
NdisGetCurrentThreadCompartmentId(
    VOID
    )
{
    return NdisGetThreadObjectCompartmentId(PsGetCurrentThread());
}

_IRQL_requires_(PASSIVE_LEVEL)
__inline
NTSTATUS
NdisSetCurrentThreadCompartmentId(
    _In_ COMPARTMENT_ID CompartmentId
    )
{
    return
        NdisSetThreadObjectCompartmentId(PsGetCurrentThread(), CompartmentId);
}

#endif // _NSI_HELPER_H
