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
 *  This file defines the functions for the otLwf Filter Thread mode.
 */

#ifndef _THREAD_H_
#define _THREAD_H_

// Helper function that converts an otInstance pointer to a MS_FILTER pointer
__inline PMS_FILTER otCtxToFilter(_In_ otInstance* otCtx)
{
    return *(PMS_FILTER*)((PUCHAR)otCtx - sizeof(PMS_FILTER));
}

// Helper function to indicate if a role means it is attached or not
_inline BOOLEAN IsAttached(_In_ otDeviceRole role)
{
    return role > OT_DEVICE_ROLE_DETACHED;
}

//
// Initialization functions
//

_IRQL_requires_max_(PASSIVE_LEVEL)
NDIS_STATUS 
otLwfInitializeThreadMode(
    _In_ PMS_FILTER pFilter
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
void 
otLwfUninitializeThreadMode(
    _In_ PMS_FILTER pFilter
    );

//
// Clean up otInstance
//

_IRQL_requires_max_(PASSIVE_LEVEL)
void
otLwfReleaseInstance(
    _In_ PMS_FILTER pFilter
    );

//
// Event Processing Functions
//

EXT_CALLBACK otLwfEventProcessingTimer;

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfEventProcessingStart(
    _In_ PMS_FILTER             pFilter
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
otLwfEventProcessingStop(
    _In_ PMS_FILTER             pFilter
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
otLwfEventProcessingIndicateNewWaitTime(
    _In_ PMS_FILTER             pFilter,
    _In_ ULONG                  waitTime
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
otLwfEventProcessingIndicateNewTasklet(
    _In_ PMS_FILTER             pFilter
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
otLwfEventProcessingIndicateAddressChange(
    _In_ PMS_FILTER             pFilter,
    _In_ MIB_NOTIFICATION_TYPE  NotificationType,
    _In_ PIN6_ADDR              pAddr
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
otLwfEventProcessingIndicateNewNetBufferLists(
    _In_ PMS_FILTER             pFilter,
    _In_ BOOLEAN                DispatchLevel,
    _In_ PNET_BUFFER_LIST       NetBufferLists
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
otLwfEventProcessingIndicateNewMacFrameCommand(
    _In_ PMS_FILTER             pFilter,
    _In_ BOOLEAN                DispatchLevel,
    _In_reads_bytes_(BufferLength) 
         const uint8_t*         Buffer,
    _In_ uint8_t                BufferLength
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
otLwfEventProcessingIndicateNetBufferListsCancelled(
    _In_ PMS_FILTER             pFilter,
    _In_ PVOID                  CancelId
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
otLwfEventProcessingIndicateIrp(
    _In_ PMS_FILTER pFilter,
    _In_ PIRP       Irp
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
otLwfEventProcessingIndicateEnergyScanResult(
    _In_ PMS_FILTER pFilter,
    _In_ CHAR       MaxRssi
    );

//
// OpenThread callbacks
//

void otLwfStateChangedCallback(uint32_t aFlags, _In_ void *aContext);
void otLwfReceiveIp6DatagramCallback(_In_ otMessage *aMessage, _In_ void *aContext);
void otLwfActiveScanCallback(_In_ otActiveScanResult *aResult, _In_ void *aContext);
void otLwfEnergyScanCallback(_In_ otEnergyScanResult *aResult, _In_ void *aContext);
void otLwfDiscoverCallback(_In_ otActiveScanResult *aResult, _In_ void *aContext);
void otLwfCommissionerEnergyReportCallback(uint32_t aChannelMask, const uint8_t *aEnergyList, uint8_t aEnergyListLength, void *aContext);
void otLwfCommissionerPanIdConflictCallback(uint16_t aPanId, uint32_t aChannelMask, _In_ void *aContext);
void otLwfJoinerCallback(otError aError, _In_ void *aContext);

//
// Value Callbacks
//

_IRQL_requires_max_(DISPATCH_LEVEL)
void
otLwfThreadValueIs(
    _In_ PMS_FILTER pFilter,
    _In_ BOOLEAN DispatchLevel,
    _In_ spinel_prop_key_t key,
    _In_reads_bytes_(value_data_len) const uint8_t* value_data_ptr,
    _In_ spinel_size_t value_data_len
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
void
otLwfThreadValueInserted(
    _In_ PMS_FILTER pFilter,
    _In_ BOOLEAN DispatchLevel,
    _In_ spinel_prop_key_t key,
    _In_reads_bytes_(value_data_len) const uint8_t* value_data_ptr,
    _In_ spinel_size_t value_data_len
    );

#endif  //_THREAD_H_
