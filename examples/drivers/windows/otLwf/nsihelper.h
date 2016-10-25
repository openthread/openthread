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

typedef enum _NSI_SET_ACTION {  
    NsiSetDefault,  
    NsiSetCreateOnly,  
    NsiSetCreateOrSet,  
    NsiSetDelete,  
    NsiSetReset,  
    NsiSetClear,  
    NsiSetCreateOrSetWithReference,  
    NsiSetDeleteWithReference,  
} NSI_SET_ACTION;  

typedef enum _NSI_STRUCT_TYPE {
    NsiStructRw,
    NsiStructRoDynamic,
    NsiStructRoStatic,
    NsiMaximumStructType
} NSI_STRUCT_TYPE;

typedef struct _NL_INTERFACE_KEY {  
    IF_LUID Luid;  
} NL_INTERFACE_KEY, *PNL_INTERFACE_KEY;  

typedef enum _NL_TYPE_OF_INTERFACE {  
    InterfaceAllowAll = 0,  
    InterfaceDisallowUnicast,  
    InterfaceDisallowMulticast,  
    InterfaceDisallowAll,  
    InterfaceUnchanged = -1  
} NL_TYPE_OF_INTERFACE;  
  
typedef enum _NL_DOMAIN_NETWORK_LOCATION {  
    DomainNetworkLocationRemote = 0,   // connect to a domain network remotely via DA i.e. outside corp network.  
    DomainNetworkCategoryLink = 1,     // connect to a domain network directly i.e. inside corp network.  
    DomainNetworkUnchanged = -1  
} NL_DOMAIN_NETWORK_LOCATION;  
  
typedef enum _NL_DOMAIN_TYPE {  
    DomainTypeNonDomainNetwork = 0,    // connected to non-domain network.  
    DomainTypeDomainNetwork = 1,       // connected to a network that has active directory.  
    DomainTypeDomainAuthenticated = 2, // connected to AD network and machine is authenticated against it.  
    DomainTypeUnchanged = -1  
} NL_DOMAIN_TYPE;  
  
typedef enum _NL_INTERFACE_ECN_CAPABILITY {  
    NlInterfaceEcnUnchanged = -1,  
    NlInterfaceEcnDisabled = 0,  
    NlInterfaceEcnUseEct1 = 1,  
    NlInterfaceEcnUseEct0 = 2,  
    NlInterfaceEcnAppDecide = 3  
} NL_INTERFACE_ECN_CAPABILITY, *PNL_INTERFACE_ECN_CAPABILITY;  
  
typedef enum _NL_INTERNET_CONNECTIVITY_STATUS {  
    NlNoInternetConnectivity,  
    NlNoInternetDnsResolutionSucceeded,  
    NlInternetConnectivityDetected,  
    NlInternetConnectivityUnknown = -1  
} NL_INTERNET_CONNECTIVITY_STATUS, *PNL_INTERNET_CONNECTIVITY_STATUS;  

typedef union _IP_ADDRESS_STORAGE {  
    IN_ADDR Ipv4;  
    IN6_ADDR Ipv6;  
    UCHAR Buffer[sizeof(IN6_ADDR)];  
} IP_ADDRESS_STORAGE, *PIP_ADDRESS_STORAGE;  

typedef struct _NL_INTERFACE_RW {  
    BOOLEAN AdvertisingEnabled;  
    BOOLEAN ForwardingEnabled;  
    BOOLEAN MulticastForwardingEnabled;  
    BOOLEAN WeakHostSend;  
    BOOLEAN WeakHostReceive;  
    BOOLEAN UseNeighborUnreachabilityDetection;  
    BOOLEAN UseAutomaticMetric;
    BOOLEAN UseZeroBroadcastAddress;  
    BOOLEAN UseBroadcastForRouterDiscovery;  
    BOOLEAN DhcpRouterDiscoveryEnabled;  
    BOOLEAN ManagedAddressConfigurationSupported;  
    BOOLEAN OtherStatefulConfigurationSupported;  
    BOOLEAN AdvertiseDefaultRoute;  
    NL_NETWORK_CATEGORY NetworkCategory;  
    NL_ROUTER_DISCOVERY_BEHAVIOR RouterDiscoveryBehavior;  
    NL_TYPE_OF_INTERFACE TypeOfInterface;  
    ULONG Metric;  
    ULONG BaseReachableTime;    // Base for random ReachableTime (in ms).  
    ULONG RetransmitTime;       // Neighbor Solicitation timeout (in ms).  
    ULONG PathMtuDiscoveryTimeout; // Path MTU discovery timeout (in ms).  
    ULONG DadTransmits;         // DupAddrDetectTransmits in RFC 2462.  
    NL_LINK_LOCAL_ADDRESS_BEHAVIOR LinkLocalAddressBehavior;  
    ULONG LinkLocalAddressTimeout; // In ms.  
    ULONG ZoneIndices[ScopeLevelCount]; // Zone part of a SCOPE_ID.  
    ULONG NlMtu;  
    ULONG SitePrefixLength;  
    ULONG MulticastForwardingHopLimit;  
    ULONG CurrentHopLimit; 
    IP_ADDRESS_STORAGE LinkLocalAddress;  
    BOOLEAN DisableDefaultRoutes;  
    ULONG AdvertisedRouterLifetime;  
    BOOLEAN SendUnsolicitedNeighborAdvertisementOnDad;  
    BOOLEAN LimitedLinkConnectivity;  
    BOOLEAN ForceARPNDPattern;  
    BOOLEAN EnableDirectMACPattern;  
    BOOLEAN EnableWol;  
    BOOLEAN ForceTunneling;  
    NL_DOMAIN_NETWORK_LOCATION DomainNetworkLocation;  
    ULONGLONG RandomizedEpoch;  
    NL_INTERFACE_ECN_CAPABILITY EcnCapability;  
    NL_DOMAIN_TYPE DomainType;  
    GUID NetworkSignature;  
    NL_INTERNET_CONNECTIVITY_STATUS InternetConnectivityDetected;  
    BOOLEAN ProxyDetected;  
    ULONG DadRetransmitTime;  
    BOOLEAN PrefixSharing;  
    BOOLEAN DisableUnconstrainedRouteLookup;  
    ULONG NetworkContext;  
    BOOLEAN ResetAutoconfigurationOnOperStatusDown;   
    BOOLEAN ClampMssEnabled;  
  
} NL_INTERFACE_RW, *PNL_INTERFACE_RW;  

__inline  
VOID  
NlInitializeInterfaceRw(  
    IN OUT PNL_INTERFACE_RW Rw  
    )  
{  
    //  
    // Initialize all fields to values that indicate "no change".  
    //  
    memset(Rw, 0xff, sizeof(*Rw));  
    Rw->BaseReachableTime = 0;  
    Rw->RetransmitTime = 0;  
    Rw->PathMtuDiscoveryTimeout = 0;  
    Rw->NlMtu = 0;  
    Rw->DadRetransmitTime = 0;  
}  

typedef enum {  
    NlBestRouteObject,  
    NlCompartmentForwardingObject,  
    NlCompartmentObject,  
    NlControlProtocolObject,  
    NlEchoRequestObject,  
    NlEchoSequenceRequestObject,  
    NlGlobalObject,  
    NlInterfaceObject,  
    NlLocalAnycastAddressObject,  
    NlLocalMulticastAddressObject,  
    NlLocalUnicastAddressObject,  
    NlNeighborObject,  
    NlPathObject,  
    NlPotentialRouterObject,  
    NlPrefixPolicyObject,  
    NlProxyNeighborObject,  
    NlRouteObject,  
    NlSitePrefixObject,  
    NlSubInterfaceObject,  
    NlWakeUpPatternObject,  
    NlResolveNeighborObject,  
    NlSortAddressesObject,  
    NlMfeObject,  
    NlMfeNotifyObject,  
    NlInterfaceHopObject,  
    NlInterfaceUnprivilegedObject,  
    NlTunnelPhysicalInterfaceObject,  
    NlLocalityObject,  
    NlLocalityDataObject,  
    NlLocalityPrivateObject,  
    NlLocalBottleneckObject,  
    NlTimerObject,  
    NlDisconnectInterface,  
    NlMaximumObject  
} NL_OBJECT_TYPE, *PNL_OBJECT_TYPE;  

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

NSISTATUS  
NsiSetAllParameters(  
    __in NSI_STORE      Store,  
    __in NSI_SET_ACTION Action,  
    __in PNPI_MODULEID  ModuleId,  
    __in ULONG          ObjectIndex,  
    __in_bcount_opt(KeyStructLength) PVOID KeyStruct,  
    __in ULONG          KeyStructLength,  
    __in_bcount_opt(RwParameterStructLength) PVOID RwParameterStruct,  
    __in ULONG          RwParameterStructLength  
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
