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

#include "precomp.h"
#include "address.tmh"

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID 
otLwfOnAddressAdded(
    _In_ PMS_FILTER pFilter, 
    _In_ PIN6_ADDR Addr
    )
{
    if (pFilter->otCachedAddrCount < OT_MAX_ADDRESSES)
    {
        NTSTATUS status;
        MIB_UNICASTIPADDRESS_ROW newRow;
        InitializeUnicastIpAddressEntry(&newRow);

        newRow.InterfaceIndex = pFilter->InterfaceIndex;
        newRow.InterfaceLuid = pFilter->InterfaceLuid;
        newRow.OnLinkPrefixLength = 64; // First half
        newRow.Address.si_family = AF_INET6;
        newRow.Address.Ipv6.sin6_family = AF_INET6;
        
        newRow.Address.Ipv6.sin6_addr = *Addr;
        newRow.PrefixOrigin = IpPrefixOriginOther;  // Derived from network XPANID
        newRow.SkipAsSource = FALSE;                // Allow automatic binding to this address (default)

        if (IN6_IS_ADDR_LINKLOCAL(Addr))
        {
            newRow.SuffixOrigin = IpSuffixOriginLinkLayerAddress;   // Derived from Extended MAC address
        }
        else
        {
            newRow.SuffixOrigin = IpSuffixOriginRandom;             // Was created randomly
        }

        LogInfo(DRIVER_DEFAULT, "Interface %!GUID! adding address: %!IPV6ADDR!", &pFilter->InterfaceGuid, Addr);
        status = CreateUnicastIpAddressEntry(&newRow);
        if (!NT_SUCCESS(status))
        {
            LogError(DRIVER_DEFAULT, "CreateUnicastIpAddressEntry failed %!STATUS!", status);
        }
        else
        {
            memcpy(pFilter->otCachedAddr + pFilter->otCachedAddrCount, Addr, sizeof(IN6_ADDR));
            pFilter->otCachedAddrCount++;

            if (IN6_IS_ADDR_LINKLOCAL(Addr))
            {
                memcpy(&pFilter->otLinkLocalAddr, Addr, sizeof(IN6_ADDR));
            }
        }
    }
}

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID 
otLwfOnAddressRemoved(
    _In_ PMS_FILTER pFilter, 
    _In_ ULONG CachedIndex
    )
{
    MIB_UNICASTIPADDRESS_ROW deleteRow;
    InitializeUnicastIpAddressEntry(&deleteRow);
    
    NT_ASSERT(pFilter->otCachedAddrCount != 0);
    NT_ASSERT(CachedIndex < pFilter->otCachedAddrCount);

    deleteRow.InterfaceIndex = pFilter->InterfaceIndex;
    deleteRow.InterfaceLuid = pFilter->InterfaceLuid;
    deleteRow.OnLinkPrefixLength = 16;
    deleteRow.Address.si_family = AF_INET6;

    deleteRow.Address.Ipv6.sin6_addr = pFilter->otCachedAddr[CachedIndex];
    
    // Best effort remove address from TCPIP
    LogInfo(DRIVER_DEFAULT, "Interface %!GUID! removing address: %!IPV6ADDR!", &pFilter->InterfaceGuid, &pFilter->otCachedAddr[CachedIndex]);
    (VOID)DeleteUnicastIpAddressEntry(&deleteRow);

    // Remove the cached entry
    if (CachedIndex + 1 != pFilter->otCachedAddrCount)
        memmove(pFilter->otCachedAddr + CachedIndex, 
                pFilter->otCachedAddr + CachedIndex + 1, 
                (pFilter->otCachedAddrCount - CachedIndex - 1) * sizeof(IN6_ADDR)
                );
    pFilter->otCachedAddrCount--;
}

int 
otLwfFindCachedAddrIndex(
    _In_ PMS_FILTER pFilter, 
    _In_ PIN6_ADDR addr
    )
{
    for (ULONG i = 0; i < pFilter->otCachedAddrCount; i++)
        if (memcmp(pFilter->otCachedAddr + i, addr, sizeof(IN6_ADDR)) == 0)
            return (int)i;
    return -1;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS 
otLwfInitializeAddresses(
    _In_ PMS_FILTER pFilter
    )
{
    NTSTATUS                    status = STATUS_SUCCESS;
    
    PMIB_UNICASTIPADDRESS_TABLE pMibUnicastAddressTable = NULL;
    COMPARTMENT_ID              OriginalCompartmentID;
    BOOLEAN                     MustRevertCompartmentID = FALSE;

    LogFuncEntry(DRIVER_DEFAULT);
    
    pFilter->otCachedAddrCount = 0;

    // Make sure we are in the right compartment
    OriginalCompartmentID = NdisGetCurrentThreadCompartmentId();
    if (OriginalCompartmentID != pFilter->InterfaceCompartmentID)
    {
        status = NdisSetCurrentThreadCompartmentId(pFilter->InterfaceCompartmentID);
        if (NT_SUCCESS(status))
        {
            MustRevertCompartmentID = TRUE;
        }
        else
        {
            LogError(DRIVER_DEFAULT, "NdisSetCurrentThreadCompartmentId failed, %!STATUS!", status);
        }
    }

    // Query the table for the current compartment
    status = GetUnicastIpAddressTable(AF_INET6, &pMibUnicastAddressTable);

    // Revert the compartment, now that we have the table
    if (MustRevertCompartmentID)
    {
        (VOID)NdisSetCurrentThreadCompartmentId(OriginalCompartmentID);
    }

    if (!NT_SUCCESS(status))
    {
        LogError(DRIVER_DEFAULT, "GetUnicastIpAddressTable failed, %!STATUS!", status);
        goto error;
    }
    
    // Iterate through the addresses and delete (best effort) the ones for our interface
    for (ULONG Index = 0; Index < pMibUnicastAddressTable->NumEntries; Index++)
    {
        MIB_UNICASTIPADDRESS_ROW* row = &pMibUnicastAddressTable->Table[Index];

        if ((0 == memcmp(&row->InterfaceLuid, &pFilter->InterfaceLuid, sizeof(NET_LUID))))
        {
            LogInfo(DRIVER_DEFAULT, "Caching initial address: %!IPV6ADDR!", &row->Address.Ipv6.sin6_addr);
            memcpy(pFilter->otCachedAddr + pFilter->otCachedAddrCount, &row->Address.Ipv6.sin6_addr, sizeof(IN6_ADDR));
            pFilter->otCachedAddrCount++;
        }
    }

error:
    
    if (NULL != pMibUnicastAddressTable)
    {
        FreeMibTable(pMibUnicastAddressTable);
    }

    LogFuncExitNT(DRIVER_DEFAULT, status);

    return status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID 
otLwfAddressesUpdated(
    _In_ PMS_FILTER pFilter
    )
{
    LogFuncEntry(DRIVER_DEFAULT);

    ULONG FoundInOpenThread = 0; // Bit field
    ULONG OriginalCacheLength = pFilter->otCachedAddrCount;
    
    const otNetifAddress* addr = otGetUnicastAddresses(pFilter->otCtx);

    // Process the addresses
    while (addr)
    {
        int index = otLwfFindCachedAddrIndex(pFilter, (PIN6_ADDR)&addr->mAddress);
        if (index == -1)
        {
            otLwfOnAddressAdded(pFilter, (PIN6_ADDR)&addr->mAddress);
        }
        else
        {
            NT_ASSERT(index < 8 * sizeof(FoundInOpenThread));
            FoundInOpenThread |= 1 << index;
        }
        addr = addr->mNext;
    }

    // Look for missing addresses and mark them as removed
    for (int i = OriginalCacheLength - 1; i >= 0; i--)
    {
        if ((FoundInOpenThread & (1 << i)) == 0)
        {
            otLwfOnAddressRemoved(pFilter, (ULONG)i);
        }
    }
    
    LogFuncExit(DRIVER_DEFAULT);
}
