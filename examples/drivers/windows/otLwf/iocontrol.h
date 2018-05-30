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

#ifndef _IOCONTROL_H
#define _IOCONTROL_H

//
// Function prototype for general Io Control functions
//

typedef 
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
OTLWF_IOCTL_FUNC(
    _In_reads_bytes_(InBufferLength)
            PVOID           InBuffer,
    _In_    ULONG           InBufferLength,
    _Out_writes_bytes_(*OutBufferLength)
            PVOID           OutBuffer,
    _Inout_ PULONG          OutBufferLength
    );

//
// General Io Control Functions
//

// Handles queries for the current list of Thread interfaces
OTLWF_IOCTL_FUNC otLwfIoCtlEnumerateInterfaces;

// Handles queries for the details of a specific Thread interface
OTLWF_IOCTL_FUNC otLwfIoCtlQueryInterface;

// Handles IOTCLs for OpenThread control
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfIoCtlOpenThreadControl(
    _In_ PIRP Irp
    );

// Handles Irp for IOTCLs for OpenThread control on the OpenThread thread
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
otLwfCompleteOpenThreadIrp(
    _In_ PMS_FILTER     pFilter,
    _In_ PIRP           Irp
    );

// Helper for converting IoCtl to string
const char*
IoCtlString(
    ULONG IoControlCode
    );

//
// Function prototype for OpenThread Io Control functions
//

typedef 
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
OTLWF_OT_IOCTL_FUNC(
    _In_ PMS_FILTER         pFilter,
    _In_reads_bytes_(InBufferLength)
            PUCHAR          InBuffer,
    _In_    ULONG           InBufferLength,
    _Out_writes_bytes_(*OutBufferLength)
            PVOID           OutBuffer,
    _Inout_ PULONG          OutBufferLength
    );

typedef 
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
OTLWF_TUN_IOCTL_FUNC(
    _In_ PMS_FILTER         pFilter,
    _In_ PIRP               Irp,
    _In_reads_bytes_(InBufferLength)
            PUCHAR          InBuffer,
    _In_    ULONG           InBufferLength,
    _In_    ULONG           OutBufferLength
    );

typedef
_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
(SPINEL_IRP_CMD_HANDLER)(
    _In_ spinel_prop_key_t Key,
    _In_reads_bytes_(DataLength) const uint8_t* Data,
    _In_ spinel_size_t DataLength,
    _Out_writes_bytes_(*OutBufferLength)
            PVOID OutBuffer,
    _Inout_ PULONG OutBufferLength
    );

#define DECL_IOCTL_FUNC(X) \
    OTLWF_OT_IOCTL_FUNC otLwfIoCtl_##X

#define DECL_IOCTL_FUNC_WITH_TUN(X) \
    OTLWF_OT_IOCTL_FUNC otLwfIoCtl_##X; \
    OTLWF_TUN_IOCTL_FUNC otLwfTunIoCtl_##X

#define DECL_IOCTL_FUNC_WITH_TUN2(X) \
    OTLWF_OT_IOCTL_FUNC otLwfIoCtl_##X; \
    OTLWF_TUN_IOCTL_FUNC otLwfTunIoCtl_##X; \
    SPINEL_IRP_CMD_HANDLER otLwfTunIoCtl_##X##_Handler

#define REF_IOCTL_FUNC(X) otLwfIoCtl_##X , NULL

#define REF_IOCTL_FUNC_WITH_TUN(X) otLwfIoCtl_##X , otLwfTunIoCtl_##X

DECL_IOCTL_FUNC_WITH_TUN2(otInterface);
DECL_IOCTL_FUNC_WITH_TUN2(otThread);
DECL_IOCTL_FUNC_WITH_TUN2(otActiveScan);
DECL_IOCTL_FUNC(otDiscover);
DECL_IOCTL_FUNC_WITH_TUN2(otChannel);
DECL_IOCTL_FUNC_WITH_TUN2(otChildTimeout);
DECL_IOCTL_FUNC_WITH_TUN2(otExtendedAddress);
DECL_IOCTL_FUNC_WITH_TUN2(otExtendedPanId);
DECL_IOCTL_FUNC_WITH_TUN2(otLeaderRloc);
DECL_IOCTL_FUNC_WITH_TUN2(otLinkMode);
DECL_IOCTL_FUNC_WITH_TUN2(otMasterKey);
DECL_IOCTL_FUNC_WITH_TUN2(otMeshLocalEid);
DECL_IOCTL_FUNC_WITH_TUN2(otMeshLocalPrefix);
//DECL_IOCTL_FUNC(otNetworkDataLeader);
//DECL_IOCTL_FUNC(otNetworkDataLocal);
DECL_IOCTL_FUNC_WITH_TUN2(otNetworkName);
DECL_IOCTL_FUNC_WITH_TUN2(otPanId);
DECL_IOCTL_FUNC_WITH_TUN2(otRouterRollEnabled);
DECL_IOCTL_FUNC_WITH_TUN2(otShortAddress);
DECL_IOCTL_FUNC(otActiveDataset);
DECL_IOCTL_FUNC(otPendingDataset);
DECL_IOCTL_FUNC_WITH_TUN2(otLocalLeaderWeight);
DECL_IOCTL_FUNC_WITH_TUN(otAddBorderRouter);
DECL_IOCTL_FUNC_WITH_TUN(otRemoveBorderRouter);
DECL_IOCTL_FUNC_WITH_TUN(otAddExternalRoute);
DECL_IOCTL_FUNC_WITH_TUN(otRemoveExternalRoute);
DECL_IOCTL_FUNC(otSendServerData);
DECL_IOCTL_FUNC_WITH_TUN2(otContextIdReuseDelay);
DECL_IOCTL_FUNC_WITH_TUN2(otKeySequenceCounter);
DECL_IOCTL_FUNC_WITH_TUN2(otNetworkIdTimeout);
DECL_IOCTL_FUNC_WITH_TUN2(otRouterUpgradeThreshold);
DECL_IOCTL_FUNC_WITH_TUN(otReleaseRouterId);
DECL_IOCTL_FUNC_WITH_TUN2(otMacWhitelistEnabled);
DECL_IOCTL_FUNC_WITH_TUN(otAddMacWhitelist);
DECL_IOCTL_FUNC_WITH_TUN(otRemoveMacWhitelist);
DECL_IOCTL_FUNC(otNextMacWhitelist);
DECL_IOCTL_FUNC_WITH_TUN(otClearMacWhitelist);
DECL_IOCTL_FUNC_WITH_TUN2(otDeviceRole);
DECL_IOCTL_FUNC(otChildInfoById);
DECL_IOCTL_FUNC(otChildInfoByIndex);
DECL_IOCTL_FUNC(otEidCacheEntry);
DECL_IOCTL_FUNC(otLeaderData);
DECL_IOCTL_FUNC_WITH_TUN2(otLeaderRouterId);
DECL_IOCTL_FUNC_WITH_TUN2(otLeaderWeight);
DECL_IOCTL_FUNC_WITH_TUN2(otNetworkDataVersion);
DECL_IOCTL_FUNC_WITH_TUN2(otPartitionId);
DECL_IOCTL_FUNC_WITH_TUN2(otRloc16);
DECL_IOCTL_FUNC(otRouterIdSequence);
DECL_IOCTL_FUNC(otMaxRouterId);
DECL_IOCTL_FUNC(otRouterInfo);
DECL_IOCTL_FUNC_WITH_TUN2(otStableNetworkDataVersion);
DECL_IOCTL_FUNC(otMacBlacklistEnabled);
DECL_IOCTL_FUNC(otAddMacBlacklist);
DECL_IOCTL_FUNC(otRemoveMacBlacklist);
DECL_IOCTL_FUNC(otNextMacBlacklist);
DECL_IOCTL_FUNC(otClearMacBlacklist);
DECL_IOCTL_FUNC(otTransmitPower);
DECL_IOCTL_FUNC(otNextOnMeshPrefix);
DECL_IOCTL_FUNC(otPollPeriod);
DECL_IOCTL_FUNC(otLocalLeaderPartitionId);
DECL_IOCTL_FUNC_WITH_TUN(otPlatformReset);
DECL_IOCTL_FUNC_WITH_TUN2(otParentInfo);
DECL_IOCTL_FUNC(otSingleton);
DECL_IOCTL_FUNC(otMacCounters);
DECL_IOCTL_FUNC_WITH_TUN2(otMaxChildren);
DECL_IOCTL_FUNC(otCommissionerStart);
DECL_IOCTL_FUNC(otCommissionerStop);
DECL_IOCTL_FUNC(otJoinerStart);
DECL_IOCTL_FUNC(otJoinerStop);
DECL_IOCTL_FUNC(otFactoryAssignedIeeeEui64);
DECL_IOCTL_FUNC(otJoinerId);
DECL_IOCTL_FUNC_WITH_TUN2(otRouterDowngradeThreshold);
DECL_IOCTL_FUNC(otCommissionerPanIdQuery);
DECL_IOCTL_FUNC(otCommissionerEnergyScan);
DECL_IOCTL_FUNC_WITH_TUN2(otRouterSelectionJitter);
DECL_IOCTL_FUNC(otJoinerUdpPort);
DECL_IOCTL_FUNC(otSendDiagnosticGet);
DECL_IOCTL_FUNC(otSendDiagnosticReset);
DECL_IOCTL_FUNC(otCommissionerAddJoiner);
DECL_IOCTL_FUNC(otCommissionerRemoveJoiner);
DECL_IOCTL_FUNC(otCommissionerProvisioningUrl);
DECL_IOCTL_FUNC(otCommissionerAnnounceBegin);
DECL_IOCTL_FUNC_WITH_TUN2(otEnergyScan);
DECL_IOCTL_FUNC(otSendActiveGet);
DECL_IOCTL_FUNC(otSendActiveSet);
DECL_IOCTL_FUNC(otSendPendingGet);
DECL_IOCTL_FUNC(otSendPendingSet);
DECL_IOCTL_FUNC(otSendMgmtCommissionerGet);
DECL_IOCTL_FUNC(otSendMgmtCommissionerSet);
DECL_IOCTL_FUNC_WITH_TUN2(otKeySwitchGuardtime);
DECL_IOCTL_FUNC(otFactoryReset);
DECL_IOCTL_FUNC(otThreadAutoStart);
DECL_IOCTL_FUNC(otThreadPreferredRouterId);
DECL_IOCTL_FUNC_WITH_TUN2(otPSKc);
DECL_IOCTL_FUNC(otParentPriority);
DECL_IOCTL_FUNC_WITH_TUN(otAddMacFixedRss);
DECL_IOCTL_FUNC_WITH_TUN(otRemoveMacFixedRss);
DECL_IOCTL_FUNC(otNextMacFixedRss);
DECL_IOCTL_FUNC_WITH_TUN(otClearMacFixedRss);
DECL_IOCTL_FUNC(otNextRoute);

#endif // _IOCONTROL_H
