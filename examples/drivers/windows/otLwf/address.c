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

#include "precomp.h"
#include "address.tmh"

_IRQL_requires_max_(PASSIVE_LEVEL)
BOOLEAN 
otLwfOnAddressAdded(
    _In_ PMS_FILTER pFilter, 
    _In_ const otNetifAddress* Addr,
    _In_ BOOLEAN UpdateWindows
    )
{
    if (pFilter->otCachedAddrCount >= OT_MAX_ADDRESSES)
    {
        LogError(DRIVER_DEFAULT, "Failing to add new address as we have reached our max!");
        return FALSE;
    }

    LogInfo(DRIVER_DEFAULT, "Interface %!GUID! adding address: %!IPV6ADDR! (%u-bit prefix)", 
        &pFilter->InterfaceGuid, 
        (PIN6_ADDR)&Addr->mAddress,
        Addr->mPrefixLength
        );

    // Update local cache
    memcpy(pFilter->otCachedAddr + pFilter->otCachedAddrCount, Addr, sizeof(IN6_ADDR));
    pFilter->otCachedAddrCount++;

    // If this is link local, cache it as our link local address
    if (IN6_IS_ADDR_LINKLOCAL((PIN6_ADDR)&Addr->mAddress))
    {
        memcpy(&pFilter->otLinkLocalAddr, Addr, sizeof(IN6_ADDR));
    }
        
    // Update Windows if necessary
    if (UpdateWindows)
    {
        NTSTATUS status;
        MIB_UNICASTIPADDRESS_ROW newRow;
        MIB_IPFORWARD_ROW2 newRouteRow;
        COMPARTMENT_ID OriginalCompartmentID;
        InitializeUnicastIpAddressEntry(&newRow);
        InitializeIpForwardEntry(&newRouteRow); 

        newRow.InterfaceIndex = pFilter->InterfaceIndex;
        newRow.InterfaceLuid = pFilter->InterfaceLuid;
        newRow.Address.si_family = AF_INET6;
        newRow.Address.Ipv6.sin6_family = AF_INET6;
        
        static_assert(sizeof(IN6_ADDR) == sizeof(otIp6Address), "Windows and OpenThread IPv6 Addr Structs must be same size");

        memcpy(&newRow.Address.Ipv6.sin6_addr, &Addr->mAddress, sizeof(IN6_ADDR));
        newRow.OnLinkPrefixLength = Addr->mPrefixLength;
        newRow.PreferredLifetime = Addr->mPreferred ? 0xffffffff : 0;
        newRow.ValidLifetime = Addr->mValid ? 0xffffffff : 0;
        newRow.PrefixOrigin = IpPrefixOriginOther;  // Derived from network XPANID
        newRow.SkipAsSource = FALSE;                // Allow automatic binding to this address (default)

        if (IN6_IS_ADDR_LINKLOCAL(&newRow.Address.Ipv6.sin6_addr))
        {
            newRow.SuffixOrigin = IpSuffixOriginLinkLayerAddress;   // Derived from Extended MAC address
        }
        else
        {
            newRow.SuffixOrigin = IpSuffixOriginRandom;             // Was created randomly
        }

        // Make sure we are in the right compartment
        (VOID)otLwfSetCompartment(pFilter, &OriginalCompartmentID);

        status = CreateUnicastIpAddressEntry(&newRow);
        //NT_ASSERT(NT_SUCCESS(status));
        if (!NT_SUCCESS(status))
        {
            LogError(DRIVER_DEFAULT, "CreateUnicastIpAddressEntry failed %!STATUS!", status);
        }

        newRouteRow.InterfaceIndex = pFilter->InterfaceIndex;
        newRouteRow.InterfaceLuid = pFilter->InterfaceLuid;
        newRouteRow.DestinationPrefix.Prefix.si_family = AF_INET6;
        newRouteRow.DestinationPrefix.PrefixLength = 0;

        status = CreateIpForwardEntry2(&newRouteRow);
        if (!NT_SUCCESS(status) && status != STATUS_DUPLICATE_OBJECTID)
        {
            LogVerbose(DRIVER_DEFAULT, "CreateIpForwardEntry2 failed %!STATUS!", status);
        }

        // Revert back to original compartment
        otLwfRevertCompartment(OriginalCompartmentID);
    }

    return TRUE;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID 
otLwfOnAddressRemoved(
    _In_ PMS_FILTER pFilter, 
    _In_ ULONG CachedIndex,
    _In_ BOOLEAN UpdateWindows
    )
{
    // Cache address before we delete local cache
    IN6_ADDR Addr = pFilter->otCachedAddr[CachedIndex];

    LogInfo(DRIVER_DEFAULT, "Interface %!GUID! removing address: %!IPV6ADDR!", &pFilter->InterfaceGuid, &Addr);
    
    NT_ASSERT(pFilter->otCachedAddrCount != 0);
    NT_ASSERT(CachedIndex < pFilter->otCachedAddrCount);

    // Remove the cached entry
    if (CachedIndex + 1 != pFilter->otCachedAddrCount)
        memmove(pFilter->otCachedAddr + CachedIndex, 
                pFilter->otCachedAddr + CachedIndex + 1, 
                (pFilter->otCachedAddrCount - CachedIndex - 1) * sizeof(IN6_ADDR)
                );
    pFilter->otCachedAddrCount--;

    // Update Windows if necessary
    if (UpdateWindows)
    {
        MIB_UNICASTIPADDRESS_ROW deleteRow;
        COMPARTMENT_ID OriginalCompartmentID;
        InitializeUnicastIpAddressEntry(&deleteRow);

        deleteRow.InterfaceIndex = pFilter->InterfaceIndex;
        deleteRow.InterfaceLuid = pFilter->InterfaceLuid;
        deleteRow.Address.si_family = AF_INET6;

        deleteRow.Address.Ipv6.sin6_addr = Addr;

        // Make sure we are in the right compartment
        (VOID)otLwfSetCompartment(pFilter, &OriginalCompartmentID);
    
        // Best effort remove address from TCPIP
        (VOID)DeleteUnicastIpAddressEntry(&deleteRow);

        // Revert back to original compartment
        otLwfRevertCompartment(OriginalCompartmentID);
    }
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

    LogFuncEntry(DRIVER_DEFAULT);
    
    pFilter->otCachedAddrCount = 0;

    // Make sure we are in the right compartment
    (VOID)otLwfSetCompartment(pFilter, &OriginalCompartmentID);

    // Query the table for the current compartment
    status = GetUnicastIpAddressTable(AF_INET6, &pMibUnicastAddressTable);

    // Revert the compartment, now that we have the table
    otLwfRevertCompartment(OriginalCompartmentID);

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

// Callback from Windows TCPIP stack when an address change occurs
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
NETIOAPI_API_ 
otLwfAddressChangeCallback(
    _In_ PVOID CallerContext,
    _In_opt_ PMIB_UNICASTIPADDRESS_ROW Row,
    _In_ MIB_NOTIFICATION_TYPE NotificationType
    )
{
    PMS_FILTER pFilter = (PMS_FILTER)CallerContext;
    if (Row == NULL || pFilter == NULL) return;

    // Ignore notifications that aren't for our interface
    if (Row->InterfaceIndex != pFilter->InterfaceIndex) return;

    LogFuncEntryMsg(DRIVER_DEFAULT, "%p (%u) %!IPV6ADDR!", pFilter, NotificationType, &Row->Address.Ipv6.sin6_addr);

    // Since we don't pass in the initial flag, we shouldn't get this type
    NT_ASSERT(NotificationType != MibInitialNotification);

    // Make sure we can reference the interface
    if (ExAcquireRundownProtection(&pFilter->ExternalRefs))
    {
        if (pFilter->DeviceStatus == OTLWF_DEVICE_STATUS_RADIO_MODE)
        {
            // Queue up the event for processing
            otLwfEventProcessingIndicateAddressChange(
                pFilter,
                NotificationType,
                &Row->Address.Ipv6.sin6_addr
                );
        }
        else
        {
            NT_ASSERT(FALSE); // Need to add support for this in tunnel mode
        }

        // Release reference on the interface
        ExReleaseRundownProtection(&pFilter->ExternalRefs);
    }

    LogFuncExit(DRIVER_DEFAULT);
}

// Callback on the OpenThread thread for processing an address change
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
otLwfEventProcessingAddressChanged(
    _In_ PMS_FILTER             pFilter,
    _In_ MIB_NOTIFICATION_TYPE  NotificationType,
    _In_ PIN6_ADDR              pAddr
    )
{
    LogFuncEntryMsg(DRIVER_DEFAULT, "%p (%u)", pFilter, NotificationType);

    if (NotificationType == MibAddInstance ||
        NotificationType == MibParameterNotification)
    {
        MIB_UNICASTIPADDRESS_ROW Row;
        InitializeUnicastIpAddressEntry(&Row);

        Row.Address.si_family = AF_INET6;
        Row.Address.Ipv6.sin6_addr = *pAddr;
        Row.InterfaceIndex = pFilter->InterfaceIndex;
        Row.InterfaceLuid = pFilter->InterfaceLuid;

        NTSTATUS status = GetUnicastIpAddressEntry(&Row);
        if (!NT_SUCCESS(status))
        {
            LogError(DRIVER_DEFAULT, "GetUnicastIpAddressEntry failed, %!STATUS!", status);
        }
        else
        {
            otNetifAddress otAddr = {0};
            memcpy(&otAddr.mAddress, pAddr, sizeof(IN6_ADDR));
            otAddr.mPreferred = Row.PreferredLifetime != 0;
            otAddr.mPrefixLength = Row.OnLinkPrefixLength;
            otAddr.mValid = Row.ValidLifetime != 0;

            BOOLEAN ShouldDelete = FALSE;
            BOOLEAN AddedToCache = FALSE;
            BOOLEAN IsCached = otLwfFindCachedAddrIndex(pFilter, pAddr) != -1;

            // Ignore link local addresses
            if (IN6_IS_ADDR_LINKLOCAL(pAddr) && !IsCached)
            {
                ShouldDelete = TRUE;
                goto add_complete;
            }

            // Add to the cache if this is a new address
            if (NotificationType == MibAddInstance && !IsCached)
            {
                AddedToCache = otLwfOnAddressAdded(pFilter, &otAddr, FALSE);
                if (AddedToCache == FALSE)
                {
                    ShouldDelete = TRUE;
                    goto add_complete;
                }
            }

            // Update OpenThread if we don't have this cached or it is being updated
            if (!IsCached/* || NotificationType == MibParameterNotification*/)
            {
                LogInfo(DRIVER_DEFAULT, "Filter %p trying to add/update address: %!IPV6ADDR!", pFilter, pAddr);

                // Add (or update) the address to OpenThread
                otError otError = otIp6AddUnicastAddress(pFilter->otCtx, &otAddr);
                if (otError != OT_ERROR_NONE)
                {
                    LogError(DRIVER_DEFAULT, "otIp6AddUnicastAddress failed, %!otError!", otError);
                    ShouldDelete = otError == OT_ERROR_NO_BUFS ? TRUE : FALSE;
                }
            }

        add_complete:

            // Remove it from TCPIP if necessary
            if (ShouldDelete)
            {
                LogInfo(DRIVER_DEFAULT, "Filter %p deleting recently added address: %!IPV6ADDR!", pFilter, pAddr);

                // Best effort remove address from TCPIP
                (VOID)DeleteUnicastIpAddressEntry(&Row);
            }
        }
    }
    else if (NotificationType == MibDeleteInstance)
    {
        // Look for the address in our cache
        int index = otLwfFindCachedAddrIndex(pFilter, pAddr);

        // If it's not already deleted from our cache, then Windows
        // is deleting the adddress and we need to update OpenThread.
        if (index != -1)
        {
            // Update our cache
            otLwfOnAddressRemoved(pFilter, (ULONG)index, FALSE);
            
            LogInfo(DRIVER_DEFAULT, "Filter %p trying to remove address: %!IPV6ADDR!", pFilter, pAddr);

            // Find the correct address from OpenThread to remove (best effort)
            (void)otIp6RemoveUnicastAddress(pFilter->otCtx, (otIp6Address*)pAddr);
        }
    }

    LogFuncExit(DRIVER_DEFAULT);
}

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID 
otLwfRadioAddressesUpdated(
    _In_ PMS_FILTER pFilter
    )
{
    LogFuncEntry(DRIVER_DEFAULT);

    ULONG FoundInOpenThread = 0; // Bit field
    ULONG OriginalCacheLength = pFilter->otCachedAddrCount;
    
    NT_ASSERT(pFilter->DeviceStatus == OTLWF_DEVICE_STATUS_RADIO_MODE);

    const otNetifAddress* addr = otIp6GetUnicastAddresses(pFilter->otCtx);

    // Process the addresses
    while (addr)
    {
        int index = otLwfFindCachedAddrIndex(pFilter, (PIN6_ADDR)&addr->mAddress);
        if (index == -1)
        {
            otLwfOnAddressAdded(pFilter, addr, TRUE);
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
            otLwfOnAddressRemoved(pFilter, (ULONG)i, TRUE);
        }
    }
    
    LogFuncExit(DRIVER_DEFAULT);
}

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID 
otLwfTunAddressesUpdated(
    _In_ PMS_FILTER pFilter,
    _In_reads_bytes_(value_data_len) const uint8_t* value_data_ptr,
    _In_ spinel_size_t value_data_len,
    _Out_ uint32_t *aNotifFlags
    )
{
    ULONG FoundInOpenThread = 0; // Bit field
    ULONG OriginalCacheLength = pFilter->otCachedAddrCount;

    LogFuncEntry(DRIVER_DEFAULT);
    
    NT_ASSERT (pFilter->DeviceStatus == OTLWF_DEVICE_STATUS_THREAD_MODE);

    *aNotifFlags = 0;

    while (value_data_len > 0)
    {
        const uint8_t *entry_ptr = NULL;
        spinel_size_t entry_len = 0;

        spinel_ssize_t len = spinel_datatype_unpack(value_data_ptr, value_data_len, "d", &entry_ptr, &entry_len);
        if (len < 1) break;

        {
            PIN6_ADDR pAddr = NULL;
            otNetifAddress addr = { { 0 }, 0, 1, 1, 0, 0, 0, NULL };
            uint32_t preferredLifetime = 0xFFFFFFFF;
            uint32_t validLifetime = 0xFFFFFFFF;

            spinel_datatype_unpack(
                entry_ptr, 
                entry_len, 
                "6CLL", 
                &pAddr, 
                &addr.mPrefixLength, 
                &validLifetime,
                &preferredLifetime);

            addr.mPreferred = preferredLifetime != 0;
            addr.mValid = validLifetime != 0;

            if (pAddr)
            {
                int index = otLwfFindCachedAddrIndex(pFilter, pAddr);
                if (index == -1)
                {
                    memcpy_s(&addr.mAddress, sizeof(addr.mAddress), pAddr, sizeof(IN6_ADDR));
                    otLwfOnAddressAdded(pFilter, &addr, TRUE);
                    *aNotifFlags |= OT_CHANGED_IP6_ADDRESS_ADDED;
                }
                else
                {
                    NT_ASSERT(index < 8 * sizeof(FoundInOpenThread));
                    FoundInOpenThread |= 1 << index;
                }
            }
        }

        value_data_len -= len;
        value_data_ptr += len;
    }

    // Look for missing addresses and mark them as removed
    for (int i = OriginalCacheLength - 1; i >= 0; i--)
    {
        if ((FoundInOpenThread & (1 << i)) == 0)
        {
            otLwfOnAddressRemoved(pFilter, (ULONG)i, TRUE);
            *aNotifFlags |= OT_CHANGED_IP6_ADDRESS_REMOVED;
        }
    }
    
    LogFuncExit(DRIVER_DEFAULT);
}
