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

OTLWF_OT_IOCTL_FUNC otLwfIoCtl_otInterface;
OTLWF_OT_IOCTL_FUNC otLwfIoCtl_otThread;
OTLWF_OT_IOCTL_FUNC otLwfIoCtl_otActiveScan;
OTLWF_OT_IOCTL_FUNC otLwfIoCtl_otDiscover;
OTLWF_OT_IOCTL_FUNC otLwfIoCtl_otChannel;
OTLWF_OT_IOCTL_FUNC otLwfIoCtl_otChildTimeout;
OTLWF_OT_IOCTL_FUNC otLwfIoCtl_otExtendedAddress;
OTLWF_OT_IOCTL_FUNC otLwfIoCtl_otExtendedPanId;
OTLWF_OT_IOCTL_FUNC otLwfIoCtl_otLeaderRloc;
OTLWF_OT_IOCTL_FUNC otLwfIoCtl_otLinkMode;
OTLWF_OT_IOCTL_FUNC otLwfIoCtl_otMasterKey;
OTLWF_OT_IOCTL_FUNC otLwfIoCtl_otMeshLocalEid;
OTLWF_OT_IOCTL_FUNC otLwfIoCtl_otMeshLocalPrefix;
//OTLWF_OT_IOCTL_FUNC otLwfIoCtl_otNetworkDataLeader
//OTLWF_OT_IOCTL_FUNC otLwfIoCtl_otNetworkDataLocal
OTLWF_OT_IOCTL_FUNC otLwfIoCtl_otNetworkName;
OTLWF_OT_IOCTL_FUNC otLwfIoCtl_otPanId;
OTLWF_OT_IOCTL_FUNC otLwfIoCtl_otRouterRollEnabled;
OTLWF_OT_IOCTL_FUNC otLwfIoCtl_otShortAddress;
//OTLWF_OT_IOCTL_FUNC otLwfIoCtl_otUnicastAddresses
OTLWF_OT_IOCTL_FUNC otLwfIoCtl_otActiveDataset;
OTLWF_OT_IOCTL_FUNC otLwfIoCtl_otPendingDataset;
OTLWF_OT_IOCTL_FUNC otLwfIoCtl_otLocalLeaderWeight;
OTLWF_OT_IOCTL_FUNC otLwfIoCtl_otAddBorderRouter;
OTLWF_OT_IOCTL_FUNC otLwfIoCtl_otRemoveBorderRouter;
OTLWF_OT_IOCTL_FUNC otLwfIoCtl_otAddExternalRoute;
OTLWF_OT_IOCTL_FUNC otLwfIoCtl_otRemoveExternalRoute;
OTLWF_OT_IOCTL_FUNC otLwfIoCtl_otSendServerData;
OTLWF_OT_IOCTL_FUNC otLwfIoCtl_otContextIdReuseDelay;
OTLWF_OT_IOCTL_FUNC otLwfIoCtl_otKeySequenceCounter;
OTLWF_OT_IOCTL_FUNC otLwfIoCtl_otNetworkIdTimeout;
OTLWF_OT_IOCTL_FUNC otLwfIoCtl_otRouterUpgradeThreshold;
OTLWF_OT_IOCTL_FUNC otLwfIoCtl_otReleaseRouterId;
OTLWF_OT_IOCTL_FUNC otLwfIoCtl_otMacWhitelistEnabled;
OTLWF_OT_IOCTL_FUNC otLwfIoCtl_otAddMacWhitelist;
OTLWF_OT_IOCTL_FUNC otLwfIoCtl_otRemoveMacWhitelist;
OTLWF_OT_IOCTL_FUNC otLwfIoCtl_otMacWhitelistEntry;
OTLWF_OT_IOCTL_FUNC otLwfIoCtl_otClearMacWhitelist;
OTLWF_OT_IOCTL_FUNC otLwfIoCtl_otDeviceRole;
OTLWF_OT_IOCTL_FUNC otLwfIoCtl_otChildInfoById;
OTLWF_OT_IOCTL_FUNC otLwfIoCtl_otChildInfoByIndex;
OTLWF_OT_IOCTL_FUNC otLwfIoCtl_otEidCacheEntry;
OTLWF_OT_IOCTL_FUNC otLwfIoCtl_otLeaderData;
OTLWF_OT_IOCTL_FUNC otLwfIoCtl_otLeaderRouterId;
OTLWF_OT_IOCTL_FUNC otLwfIoCtl_otLeaderWeight;
OTLWF_OT_IOCTL_FUNC otLwfIoCtl_otNetworkDataVersion;
OTLWF_OT_IOCTL_FUNC otLwfIoCtl_otPartitionId;
OTLWF_OT_IOCTL_FUNC otLwfIoCtl_otRloc16;
OTLWF_OT_IOCTL_FUNC otLwfIoCtl_otRouterIdSequence;
OTLWF_OT_IOCTL_FUNC otLwfIoCtl_otRouterInfo;
OTLWF_OT_IOCTL_FUNC otLwfIoCtl_otStableNetworkDataVersion;
OTLWF_OT_IOCTL_FUNC otLwfIoCtl_otMacBlacklistEnabled;
OTLWF_OT_IOCTL_FUNC otLwfIoCtl_otAddMacBlacklist;
OTLWF_OT_IOCTL_FUNC otLwfIoCtl_otRemoveMacBlacklist;
OTLWF_OT_IOCTL_FUNC otLwfIoCtl_otMacBlacklistEntry;
OTLWF_OT_IOCTL_FUNC otLwfIoCtl_otClearMacBlacklist;
OTLWF_OT_IOCTL_FUNC otLwfIoCtl_otMaxTransmitPower;
OTLWF_OT_IOCTL_FUNC otLwfIoCtl_otNextOnMeshPrefix;
OTLWF_OT_IOCTL_FUNC otLwfIoCtl_otPollPeriod;
OTLWF_OT_IOCTL_FUNC otLwfIoCtl_otLocalLeaderPartitionId;
OTLWF_OT_IOCTL_FUNC otLwfIoCtl_otAssignLinkQuality;
OTLWF_OT_IOCTL_FUNC otLwfIoCtl_otPlatformReset;
OTLWF_OT_IOCTL_FUNC otLwfIoCtl_otParentInfo;
OTLWF_OT_IOCTL_FUNC otLwfIoCtl_otSingleton;
OTLWF_OT_IOCTL_FUNC otLwfIoCtl_otMacCounters;
OTLWF_OT_IOCTL_FUNC otLwfIoCtl_otMaxChildren;
OTLWF_OT_IOCTL_FUNC otLwfIoCtl_otCommissionerStart;
OTLWF_OT_IOCTL_FUNC otLwfIoCtl_otCommissionerStop;
OTLWF_OT_IOCTL_FUNC otLwfIoCtl_otJoinerStart;
OTLWF_OT_IOCTL_FUNC otLwfIoCtl_otJoinerStop;
OTLWF_OT_IOCTL_FUNC otLwfIoCtl_otFactoryAssignedIeeeEui64;
OTLWF_OT_IOCTL_FUNC otLwfIoCtl_otHashMacAddress;
OTLWF_OT_IOCTL_FUNC otLwfIoCtl_otRouterDowngradeThreshold;
OTLWF_OT_IOCTL_FUNC otLwfIoCtl_otCommissionerPanIdQuery;
OTLWF_OT_IOCTL_FUNC otLwfIoCtl_otCommissionerEnergyScan;
OTLWF_OT_IOCTL_FUNC otLwfIoCtl_otRouterSelectionJitter;
OTLWF_OT_IOCTL_FUNC otLwfIoCtl_otJoinerUdpPort;
OTLWF_OT_IOCTL_FUNC otLwfIoCtl_otSendDiagnosticGet;
OTLWF_OT_IOCTL_FUNC otLwfIoCtl_otSendDiagnosticReset;
OTLWF_OT_IOCTL_FUNC otLwfIoCtl_otCommissionerAddJoiner;
OTLWF_OT_IOCTL_FUNC otLwfIoCtl_otCommissionerRemoveJoiner;
OTLWF_OT_IOCTL_FUNC otLwfIoCtl_otCommissionerProvisioningUrl;
OTLWF_OT_IOCTL_FUNC otLwfIoCtl_otCommissionerAnnounceBegin;
OTLWF_OT_IOCTL_FUNC otLwfIoCtl_otEnergyScan;
OTLWF_OT_IOCTL_FUNC otLwfIoCtl_otSendActiveGet;
OTLWF_OT_IOCTL_FUNC otLwfIoCtl_otSendActiveSet;
OTLWF_OT_IOCTL_FUNC otLwfIoCtl_otSendPendingGet;
OTLWF_OT_IOCTL_FUNC otLwfIoCtl_otSendPendingSet;
OTLWF_OT_IOCTL_FUNC otLwfIoCtl_otSendMgmtCommissionerGet;
OTLWF_OT_IOCTL_FUNC otLwfIoCtl_otSendMgmtCommissionerSet;
OTLWF_OT_IOCTL_FUNC otLwfIoCtl_otKeySwitchGuardtime;

#endif // _IOCONTROL_H
