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
#include "otNodeApi.tmh"

#define DEBUG_PING 1

#define GUID_FORMAT "{%08lX-%04hX-%04hX-%02hhX%02hhX-%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX}"
#define GUID_ARG(guid) guid.Data1, guid.Data2, guid.Data3, guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3], guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]

typedef DWORD (*fp_otvmpOpenHandle)(_Out_ HANDLE* phandle);
typedef VOID  (*fp_otvmpCloseHandle)(_In_ HANDLE handle);
typedef DWORD (*fp_otvmpAddVirtualBus)(_In_ HANDLE handle, _Inout_ ULONG* pBusNumber, _Out_ ULONG* pIfIndex);
typedef DWORD (*fp_otvmpRemoveVirtualBus)(_In_ HANDLE handle, ULONG BusNumber);
typedef DWORD (*fp_otvmpSetAdapterTopologyGuid)(_In_ HANDLE handle, DWORD BusNumber, _In_ const GUID* pTopologyGuid);
typedef void (*fp_otvmpListenerCallback)(_In_opt_ PVOID aContext, _In_ ULONG SourceInterfaceIndex, _In_reads_bytes_(FrameLength) PUCHAR FrameBuffer, _In_ UCHAR FrameLength, _In_ UCHAR Channel);
typedef HANDLE (*fp_otvmpListenerCreate)(_In_ const GUID* pAdapterTopologyGuid);
typedef void (*fp_otvmpListenerDestroy)(_In_opt_ HANDLE pHandle);
typedef void(*fp_otvmpListenerRegister)(_In_ HANDLE pHandle, _In_opt_ fp_otvmpListenerCallback Callback, _In_opt_ PVOID Context);

fp_otvmpOpenHandle              otvmpOpenHandle = nullptr;
fp_otvmpCloseHandle             otvmpCloseHandle = nullptr;
fp_otvmpAddVirtualBus           otvmpAddVirtualBus = nullptr;
fp_otvmpRemoveVirtualBus        otvmpRemoveVirtualBus = nullptr;
fp_otvmpSetAdapterTopologyGuid  otvmpSetAdapterTopologyGuid = nullptr;
fp_otvmpListenerCreate          otvmpListenerCreate = nullptr;
fp_otvmpListenerDestroy         otvmpListenerDestroy = nullptr;
fp_otvmpListenerRegister        otvmpListenerRegister = nullptr;

HMODULE gVmpModule = nullptr;
HANDLE  gVmpHandle = nullptr;

ULONG gNextBusNumber = 1;
GUID gTopologyGuid = {0};

volatile LONG gNumberOfInterfaces = 0;
CRITICAL_SECTION gCS;
vector<otNode*> gNodes;
HANDLE gDeviceArrivalEvent = nullptr;

otApiInstance *gApiInstance = nullptr;

_Success_(return == OT_ERROR_NONE)
otError otNodeParsePrefix(const char *aStrPrefix, _Out_ otIp6Prefix *aPrefix)
{
    char *prefixLengthStr;
    char *endptr;

    if ((prefixLengthStr = (char*)strchr(aStrPrefix, '/')) == NULL)
    {
        printf("invalid prefix (%s)!\r\n", aStrPrefix);
        return OT_ERROR_INVALID_ARGS;
    }

    *prefixLengthStr++ = '\0';
    
    auto error = otIp6AddressFromString(aStrPrefix, &aPrefix->mPrefix);
    if (error != OT_ERROR_NONE)
    {
        printf("ipaddr (%s) to string failed, 0x%x!\r\n", aStrPrefix, error);
        return error;
    }

    aPrefix->mLength = static_cast<uint8_t>(strtol(prefixLengthStr, &endptr, 0));
    
    if (*endptr != '\0')
    {
        printf("invalid prefix ending (%s)!\r\n", aStrPrefix);
        return OT_ERROR_PARSE;
    }

    return OT_ERROR_NONE;
}

void OTCALL otNodeDeviceAvailabilityChanged(bool aAdded, const GUID *, void *)
{
    if (aAdded) SetEvent(gDeviceArrivalEvent);
}

otApiInstance* GetApiInstance()
{
    if (gApiInstance == nullptr)
    { 
        WSADATA wsaData;
        int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
        if (result != 0)
        {
            printf("WSAStartup failed!\r\n");
            return nullptr;
        }

        gApiInstance = otApiInit();
        if (gApiInstance == nullptr)
        {
            printf("otApiInit failed!\r\n");
            Unload();
            return nullptr;
        }

        InitializeCriticalSection(&gCS);

        gVmpModule = LoadLibrary(TEXT("otvmpapi.dll"));
        if (gVmpModule == nullptr)
        {
            printf("LoadLibrary(\"otvmpapi\") failed!\r\n");
            Unload();
            return nullptr;
        }

        otvmpOpenHandle             = (fp_otvmpOpenHandle)GetProcAddress(gVmpModule, "otvmpOpenHandle");
        otvmpCloseHandle            = (fp_otvmpCloseHandle)GetProcAddress(gVmpModule, "otvmpCloseHandle");
        otvmpAddVirtualBus          = (fp_otvmpAddVirtualBus)GetProcAddress(gVmpModule, "otvmpAddVirtualBus");
        otvmpRemoveVirtualBus       = (fp_otvmpRemoveVirtualBus)GetProcAddress(gVmpModule, "otvmpRemoveVirtualBus");
        otvmpSetAdapterTopologyGuid = (fp_otvmpSetAdapterTopologyGuid)GetProcAddress(gVmpModule, "otvmpSetAdapterTopologyGuid");
        otvmpListenerCreate         = (fp_otvmpListenerCreate)GetProcAddress(gVmpModule, "otvmpListenerCreate");
        otvmpListenerDestroy        = (fp_otvmpListenerDestroy)GetProcAddress(gVmpModule, "otvmpListenerDestroy");
        otvmpListenerRegister       = (fp_otvmpListenerRegister)GetProcAddress(gVmpModule, "otvmpListenerRegister");

        assert(otvmpOpenHandle);
        assert(otvmpCloseHandle);
        assert(otvmpAddVirtualBus);
        assert(otvmpRemoveVirtualBus);
        assert(otvmpSetAdapterTopologyGuid);
        assert(otvmpListenerCreate);
        assert(otvmpListenerDestroy);
        assert(otvmpListenerRegister);

        if (otvmpOpenHandle == nullptr) printf("otvmpOpenHandle is null!\r\n");
        if (otvmpCloseHandle == nullptr) printf("otvmpCloseHandle is null!\r\n");
        if (otvmpAddVirtualBus == nullptr) printf("otvmpAddVirtualBus is null!\r\n");
        if (otvmpRemoveVirtualBus == nullptr) printf("otvmpRemoveVirtualBus is null!\r\n");
        if (otvmpSetAdapterTopologyGuid == nullptr) printf("otvmpSetAdapterTopologyGuid is null!\r\n");
        if (otvmpListenerCreate == nullptr) printf("otvmpListenerCreate is null!\r\n");
        if (otvmpListenerDestroy == nullptr) printf("otvmpListenerDestroy is null!\r\n");
        if (otvmpListenerRegister == nullptr) printf("otvmpListenerRegister is null!\r\n");

        (VOID)otvmpOpenHandle(&gVmpHandle);
        if (gVmpHandle == nullptr)
        {
            printf("otvmpOpenHandle failed!\r\n");
            Unload();
            return nullptr;
        }

        auto status = UuidCreate(&gTopologyGuid);
        if (status != NO_ERROR)
        {
            printf("UuidCreate failed, 0x%x!\r\n", status);
            Unload();
            return nullptr;
        }

        auto offset = getenv("INSTANCE");
        if (offset)
        {
            gNextBusNumber = (atoi(offset) * 32) % 1000 + 1;
        }
        else
        {
            srand(gTopologyGuid.Data1);
            gNextBusNumber = rand() % 1000 + 1;
        }

        gDeviceArrivalEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);

        // Set callback to wait for device arrival notifications
        otSetDeviceAvailabilityChangedCallback(gApiInstance, otNodeDeviceAvailabilityChanged, nullptr);

        printf("New topology created\r\n" GUID_FORMAT " [%d]\r\n\r\n", GUID_ARG(gTopologyGuid), gNextBusNumber);
    }

    InterlockedIncrement(&gNumberOfInterfaces);

    return gApiInstance;
}

void ReleaseApiInstance()
{
    if (0 == InterlockedDecrement(&gNumberOfInterfaces))
    {
        // Uninitialize everything else if this is the last ref
        Unload();
    }
}

void Unload()
{
    if (gNumberOfInterfaces != 0)
    {
        printf("Unloaded with %d outstanding nodes!\r\n", gNumberOfInterfaces);
    }

    if (gApiInstance)
    {
        otSetDeviceAvailabilityChangedCallback(gApiInstance, nullptr, nullptr);

        if (gDeviceArrivalEvent != nullptr)
        {
            CloseHandle(gDeviceArrivalEvent);
            gDeviceArrivalEvent = nullptr;
        }

        if (gVmpHandle != nullptr)
        {
            otvmpCloseHandle(gVmpHandle);
            gVmpHandle = nullptr;
        }

        if (gVmpModule != nullptr)
        {
            FreeLibrary(gVmpModule);
            gVmpModule = nullptr;
        }

        DeleteCriticalSection(&gCS);

        otApiFinalize(gApiInstance);
        gApiInstance = nullptr;

        WSACleanup();

        printf("Topology destroyed\r\n");
    }
}

int Hex2Bin(const char *aHex, uint8_t *aBin, uint16_t aBinLength)
{
    size_t hexLength = strlen(aHex);
    const char *hexEnd = aHex + hexLength;
    uint8_t *cur = aBin;
    uint8_t numChars = hexLength & 1;
    uint8_t byte = 0;

    if ((hexLength + 1) / 2 > aBinLength)
    {
        return -1;
    }

    while (aHex < hexEnd)
    {
        if ('A' <= *aHex && *aHex <= 'F')
        {
            byte |= 10 + (*aHex - 'A');
        }
        else if ('a' <= *aHex && *aHex <= 'f')
        {
            byte |= 10 + (*aHex - 'a');
        }
        else if ('0' <= *aHex && *aHex <= '9')
        {
            byte |= *aHex - '0';
        }
        else
        {
            return -1;
        }

        aHex++;
        numChars++;

        if (numChars >= 2)
        {
            numChars = 0;
            *cur++ = byte;
            byte = 0;
        }
        else
        {
            byte <<= 4;
        }
    }

    return static_cast<int>(cur - aBin);
}

typedef struct otPingHandler
{
    otNode*         mParentNode;
    bool            mActive;
    otIp6Address    mAddress;
    SOCKET          mSocket;
    CHAR            mRecvBuffer[1500];
    WSAOVERLAPPED   mOverlapped;
    PTP_WAIT        mThreadpoolWait;
    WSABUF          mWSARecvBuffer;
    DWORD           mNumBytesReceived;
    SOCKADDR_IN6    mSourceAddr6;
    int             mSourceAddr6Len;

} otPingHandler;

typedef struct otNode
{
    uint32_t                mId;
    DWORD                   mBusIndex;
    DWORD                   mInterfaceIndex;
    otInstance*             mInstance;
    HANDLE                  mEnergyScanEvent;
    HANDLE                  mPanIdConflictEvent;
    CRITICAL_SECTION        mCS;
    vector<otPingHandler*>  mPingHandlers;
    vector<void*>           mMemoryToFree;
} otNode;

const char* otDeviceRoleToString(otDeviceRole role)
{
    switch (role)
    {
    case OT_DEVICE_ROLE_DISABLED: return "disabled";
    case OT_DEVICE_ROLE_DETACHED: return "detached";
    case OT_DEVICE_ROLE_CHILD:    return "child";
    case OT_DEVICE_ROLE_ROUTER:   return "router";
    case OT_DEVICE_ROLE_LEADER:   return "leader";
    default:                      return "invalid";
    }
}

const USHORT CertificationPingPort = htons(12345);

const IN6_ADDR LinkLocalAllNodesAddress    = { { 0xFF, 0x02, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x01 } };
const IN6_ADDR LinkLocalAllRoutersAddress  = { { 0xFF, 0x02, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x02 } };
const IN6_ADDR RealmLocalAllNodesAddress   = { { 0xFF, 0x03, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x01 } };
const IN6_ADDR RealmLocalAllRoutersAddress = { { 0xFF, 0x03, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x02 } };
const IN6_ADDR RealmLocalSpecialAddress    = { { 0xFF, 0x33, 0, 0x40, 0xfd, 0xde, 0xad, 0, 0xbe, 0xef, 0, 0, 0, 0, 0, 0x01 } };

void
CALLBACK 
PingHandlerRecvCallback(
    _Inout_     PTP_CALLBACK_INSTANCE /* Instance */,
    _Inout_opt_ PVOID                 Context,
    _Inout_     PTP_WAIT              /* Wait */,
    _In_        TP_WAIT_RESULT        /* WaitResult */
    )
{
    otPingHandler *aPingHandler = (otPingHandler*)Context;
    if (aPingHandler == NULL) return;
    
    // Get the result of the IO operation
    DWORD cbTransferred = 0;
    DWORD dwFlags = 0;
    if (!WSAGetOverlappedResult(
            aPingHandler->mSocket,
            &aPingHandler->mOverlapped,
            &cbTransferred,
            TRUE,
            &dwFlags))
    {
        int result = WSAGetLastError();
        // Only log if we are shutting down
        if (result != WSAENOTSOCK && result != ERROR_OPERATION_ABORTED)
            printf("WSAGetOverlappedResult failed, 0x%x\r\n", result);
        return;
    }

    int result;

    // Make sure it didn't come from our address
    if (memcmp(&aPingHandler->mSourceAddr6.sin6_addr, &aPingHandler->mAddress, sizeof(IN6_ADDR)) != 0)
    {
        bool shouldReply = true;

        // TODO - Fix this hack...
        auto RecvDest = (const otIp6Address*)aPingHandler->mRecvBuffer;
        if (memcmp(RecvDest, &LinkLocalAllRoutersAddress, sizeof(IN6_ADDR)) == 0 ||
            memcmp(RecvDest, &RealmLocalAllRoutersAddress, sizeof(IN6_ADDR)) == 0)
        {
            auto Role = otThreadGetDeviceRole(aPingHandler->mParentNode->mInstance);
            if (Role != OT_DEVICE_ROLE_LEADER && Role != OT_DEVICE_ROLE_ROUTER)
                shouldReply = false;
        }

        if (shouldReply)
        {
#if DEBUG_PING
            CHAR szIpAddress[46] = { 0 };
            RtlIpv6AddressToStringA(&aPingHandler->mSourceAddr6.sin6_addr, szIpAddress);
            printf("%d: received ping (%d bytes) from %s\r\n", aPingHandler->mParentNode->mId, cbTransferred, szIpAddress);
#endif

            // Send the received data back
            result = 
                sendto(
                    aPingHandler->mSocket, 
                    aPingHandler->mRecvBuffer, cbTransferred, 0, 
                    (SOCKADDR*)&aPingHandler->mSourceAddr6, aPingHandler->mSourceAddr6Len
                    );
            if (result == SOCKET_ERROR)
            {
                printf("sendto failed, 0x%x\r\n", WSAGetLastError());
            }
        }
    }
    
    // Start the otpool waiting on the overlapped event
    SetThreadpoolWait(aPingHandler->mThreadpoolWait, aPingHandler->mOverlapped.hEvent, nullptr);

    // Post another recv
    dwFlags = MSG_PARTIAL;
    aPingHandler->mSourceAddr6Len = sizeof(aPingHandler->mSourceAddr6);
    result = 
        WSARecvFrom(
            aPingHandler->mSocket, 
            &aPingHandler->mWSARecvBuffer, 1, &aPingHandler->mNumBytesReceived, &dwFlags, 
            (SOCKADDR*)&aPingHandler->mSourceAddr6, &aPingHandler->mSourceAddr6Len, 
            &aPingHandler->mOverlapped, nullptr
            );
    if (result != SOCKET_ERROR)
    {
        // Not pending, so manually trigger the event for the Threadpool to execute
        SetEvent(aPingHandler->mOverlapped.hEvent);
    }
    else
    {
        result = WSAGetLastError();
        if (result != WSA_IO_PENDING)
        {
            printf("WSARecvFrom failed, 0x%x\r\n", result);
        }
    }
}

bool IsMeshLocalEID(otNode *aNode, const otIp6Address *aAddress)
{
    auto ML_EID = otThreadGetMeshLocalEid(aNode->mInstance);
    if (ML_EID == nullptr) return false;
    bool result = memcmp(ML_EID->mFields.m8, aAddress->mFields.m8, sizeof(otIp6Address)) == 0;
    otFreeMemory(ML_EID);
    return result;
}

void AddPingHandler(otNode *aNode, const otIp6Address *aAddress)
{
    otPingHandler *aPingHandler = new otPingHandler();
    aPingHandler->mParentNode = aNode;
    aPingHandler->mAddress = *aAddress;
    aPingHandler->mSocket = INVALID_SOCKET;
    aPingHandler->mOverlapped.hEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    aPingHandler->mWSARecvBuffer = { 1500, aPingHandler->mRecvBuffer };
    aPingHandler->mActive = true;
    aPingHandler->mThreadpoolWait = 
        CreateThreadpoolWait(
            PingHandlerRecvCallback,
            aPingHandler,
            nullptr
            );
    
    SOCKADDR_IN6 addr6 = { 0 };
    addr6.sin6_family = AF_INET6;
    addr6.sin6_port = CertificationPingPort;
    memcpy(&addr6.sin6_addr, aAddress, sizeof(IN6_ADDR));
    
#if DEBUG_PING
    CHAR szIpAddress[46] = { 0 };
    RtlIpv6AddressToStringA(&addr6.sin6_addr, szIpAddress);

    printf("%d: starting ping handler for %s\r\n", aNode->mId, szIpAddress);
#endif

    // Put the current thead in the correct compartment
    bool RevertCompartmentOnExit = false;
    ULONG OriginalCompartmentID = GetCurrentThreadCompartmentId();
    if (OriginalCompartmentID != otGetCompartmentId(aNode->mInstance))
    {
        DWORD dwError = ERROR_SUCCESS;
        if ((dwError = SetCurrentThreadCompartmentId(otGetCompartmentId(aNode->mInstance))) != ERROR_SUCCESS)
        {
            printf("SetCurrentThreadCompartmentId failed, 0x%x\r\n", dwError);
        }
        RevertCompartmentOnExit = true;
    }

    int result;
    DWORD Flag = FALSE;
    IPV6_MREQ MCReg;
    MCReg.ipv6mr_interface = otGetDeviceIfIndex(aNode->mInstance);

    if (aPingHandler->mOverlapped.hEvent == nullptr ||
        aPingHandler->mThreadpoolWait == nullptr)
    {
        goto exit;
    }
    
    // Create the socket
    aPingHandler->mSocket = WSASocketW(AF_INET6, SOCK_DGRAM, IPPROTO_UDP, NULL, 0, WSA_FLAG_OVERLAPPED);
    if (aPingHandler->mSocket == INVALID_SOCKET)
    {
        printf("WSASocket failed, 0x%x\r\n", WSAGetLastError());
        goto exit;
    }

    // Bind the socket to the address
    result = bind(aPingHandler->mSocket, (sockaddr*)&addr6, sizeof(addr6));
    if (result == SOCKET_ERROR)
    {
        printf("bind failed, 0x%x\r\n", WSAGetLastError());
        goto exit;
    }
    
    // Block our own sends from getting called as receives
    result = setsockopt(aPingHandler->mSocket, IPPROTO_IPV6, IPV6_MULTICAST_LOOP, (char *)&Flag, sizeof(Flag));
    if (result == SOCKET_ERROR)
    {
        printf("setsockopt (IPV6_MULTICAST_LOOP) failed, 0x%x\r\n", WSAGetLastError());
        goto exit;
    }

    // Bind to the multicast addresses
    if (IN6_IS_ADDR_LINKLOCAL(&addr6.sin6_addr))
    {
        // All nodes address
        MCReg.ipv6mr_multiaddr = LinkLocalAllNodesAddress;
        result = setsockopt(aPingHandler->mSocket, IPPROTO_IPV6, IPV6_ADD_MEMBERSHIP, (char *)&MCReg, sizeof(MCReg));
        if (result == SOCKET_ERROR)
        {
            printf("setsockopt (IPV6_ADD_MEMBERSHIP) failed, 0x%x\r\n", WSAGetLastError());
            goto exit;
        }

        // All routers address
        MCReg.ipv6mr_multiaddr = LinkLocalAllRoutersAddress;
        result = setsockopt(aPingHandler->mSocket, IPPROTO_IPV6, IPV6_ADD_MEMBERSHIP, (char *)&MCReg, sizeof(MCReg));
        if (result == SOCKET_ERROR)
        {
            printf("setsockopt (IPV6_ADD_MEMBERSHIP) failed, 0x%x\r\n", WSAGetLastError());
            goto exit;
        }
    }
    else if (IsMeshLocalEID(aNode, aAddress))
    {
        // All nodes address
        MCReg.ipv6mr_multiaddr = RealmLocalAllNodesAddress;
        result = setsockopt(aPingHandler->mSocket, IPPROTO_IPV6, IPV6_ADD_MEMBERSHIP, (char *)&MCReg, sizeof(MCReg));
        if (result == SOCKET_ERROR)
        {
            printf("setsockopt (IPV6_ADD_MEMBERSHIP) failed, 0x%x\r\n", WSAGetLastError());
            goto exit;
        }
        
        // All routers address
        MCReg.ipv6mr_multiaddr = RealmLocalAllRoutersAddress;
        result = setsockopt(aPingHandler->mSocket, IPPROTO_IPV6, IPV6_ADD_MEMBERSHIP, (char *)&MCReg, sizeof(MCReg));
        if (result == SOCKET_ERROR)
        {
            printf("setsockopt (IPV6_ADD_MEMBERSHIP) failed, 0x%x\r\n", WSAGetLastError());
            goto exit;
        }
        
        // Special realm local address
        MCReg.ipv6mr_multiaddr = RealmLocalSpecialAddress;
        result = setsockopt(aPingHandler->mSocket, IPPROTO_IPV6, IPV6_ADD_MEMBERSHIP, (char *)&MCReg, sizeof(MCReg));
        if (result == SOCKET_ERROR)
        {
            printf("setsockopt (IPV6_ADD_MEMBERSHIP) failed, 0x%x\r\n", WSAGetLastError());
            goto exit;
        }
    }
    
    // Start the otpool waiting on the overlapped event
    SetThreadpoolWait(aPingHandler->mThreadpoolWait, aPingHandler->mOverlapped.hEvent, nullptr);

    // Start the receive
    Flag = MSG_PARTIAL;
    aPingHandler->mSourceAddr6Len = sizeof(aPingHandler->mSourceAddr6);
    result = 
        WSARecvFrom(
            aPingHandler->mSocket, 
            &aPingHandler->mWSARecvBuffer, 1, &aPingHandler->mNumBytesReceived, &Flag, 
            (SOCKADDR*)&aPingHandler->mSourceAddr6, &aPingHandler->mSourceAddr6Len, 
            &aPingHandler->mOverlapped, nullptr
            );
    if (result != SOCKET_ERROR)
    {
        // Not pending, so manually trigger the event for the Threadpool to execute
        SetEvent(aPingHandler->mOverlapped.hEvent);
    }
    else
    {
        result = WSAGetLastError();
        if (result != WSA_IO_PENDING)
        {
            printf("WSARecvFrom failed, 0x%x\r\n", result);
            goto exit;
        }
    }

    aNode->mPingHandlers.push_back(aPingHandler);
    aPingHandler = nullptr;

exit:
    
    // Revert the comparment if necessary
    if (RevertCompartmentOnExit)
    {
        (VOID)SetCurrentThreadCompartmentId(OriginalCompartmentID);
    }

    // Clean up ping handler if necessary
    if (aPingHandler)
    {
        if (aPingHandler->mThreadpoolWait != nullptr)
        {
            if (aPingHandler->mSocket != INVALID_SOCKET) 
                closesocket(aPingHandler->mSocket);
            WaitForThreadpoolWaitCallbacks(aPingHandler->mThreadpoolWait, TRUE);
            CloseThreadpoolWait(aPingHandler->mThreadpoolWait);
        }
        if (aPingHandler->mOverlapped.hEvent)
        {
            CloseHandle(aPingHandler->mOverlapped.hEvent);
        }
        delete aPingHandler;
    }
}

void HandleAddressChanges(otNode *aNode)
{
    otLogFuncEntry();
    auto addrs = otIp6GetUnicastAddresses(aNode->mInstance);

    EnterCriticalSection(&aNode->mCS);
        
    // Invalidate all handlers
    for (ULONG i = 0; i < aNode->mPingHandlers.size(); i++)
        aNode->mPingHandlers[i]->mActive = false;

    // Search for matches
    for (auto addr = addrs; addr; addr = addr->mNext)
    {
        bool found = false;
        for (ULONG i = 0; i < aNode->mPingHandlers.size(); i++)
            if (!aNode->mPingHandlers[i]->mActive &&
                memcmp(&addr->mAddress, &aNode->mPingHandlers[i]->mAddress, sizeof(otIp6Address)) == 0)
            {
                found = true;
                aNode->mPingHandlers[i]->mActive = true;
                break;
            }
        if (!found) AddPingHandler(aNode, &addr->mAddress);
    }

    vector<otPingHandler*> pingHandlersToDelete;
        
    // Release all left over handlers
    for (int i = aNode->mPingHandlers.size() - 1; i >= 0; i--)
        if (aNode->mPingHandlers[i]->mActive == false)
        {
            auto aPingHandler = aNode->mPingHandlers[i];

#if DEBUG_PING
            CHAR szIpAddress[46] = { 0 };
            RtlIpv6AddressToStringA((PIN6_ADDR)&aPingHandler->mAddress, szIpAddress);
            printf("%d: removing ping handler for %s\r\n", aNode->mId, szIpAddress);
#endif

            aNode->mPingHandlers.erase(aNode->mPingHandlers.begin() + i);
                
            shutdown(aPingHandler->mSocket, SD_BOTH);
            closesocket(aPingHandler->mSocket);
            
            pingHandlersToDelete.push_back(aPingHandler);
        }

    LeaveCriticalSection(&aNode->mCS);

    for each (auto aPingHandler in pingHandlersToDelete)
    {
        WaitForThreadpoolWaitCallbacks(aPingHandler->mThreadpoolWait, TRUE);
        CloseThreadpoolWait(aPingHandler->mThreadpoolWait);
        CloseHandle(aPingHandler->mOverlapped.hEvent);

        delete aPingHandler;
    }

    if (addrs) otFreeMemory(addrs);

    otLogFuncExit();
}

void OTCALL otNodeStateChangedCallback(uint32_t aFlags, void *aContext)
{
    otLogFuncEntry();
    otNode* aNode = (otNode*)aContext;

    if ((aFlags & OT_CHANGED_THREAD_ROLE) != 0)
    {
        auto Role = otThreadGetDeviceRole(aNode->mInstance);
        printf("%d: new role: %s\r\n", aNode->mId, otDeviceRoleToString(Role));
    }

    if ((aFlags & OT_CHANGED_IP6_ADDRESS_ADDED) != 0 || (aFlags & OT_CHANGED_IP6_ADDRESS_REMOVED) != 0 ||
        (aFlags & OT_CHANGED_THREAD_RLOC_ADDED) != 0 || (aFlags & OT_CHANGED_THREAD_RLOC_REMOVED) != 0)
    {
        HandleAddressChanges(aNode);
    }
    otLogFuncExit();
}

OTNODEAPI int32_t OTCALL otNodeLog(const char *aMessage)
{
    LogInfo(OT_API, "%s", aMessage);
    return 0;
}

OTNODEAPI otNode* OTCALL otNodeInit(uint32_t id)
{
    otLogFuncEntry();

    auto ApiInstance = GetApiInstance();
    if (ApiInstance == nullptr)
    {
        printf("GetApiInstance failed!\r\n");
        otLogFuncExitMsg("GetApiInstance failed");
        return nullptr;
    }

    bool BusAdded = false;
    DWORD newBusIndex;
    NET_IFINDEX ifIndex = {};
    NET_LUID ifLuid = {};
    GUID ifGuid = {};
    otNode *node = nullptr;
    
    DWORD dwError;
    DWORD tries = 0;
    while (tries < 1000)
    {
        newBusIndex = (gNextBusNumber + tries) % 1000;
        if (newBusIndex == 0) newBusIndex++;

        dwError = otvmpAddVirtualBus(gVmpHandle, &newBusIndex, &ifIndex);
        if (dwError == ERROR_SUCCESS)
        {
            BusAdded = true;
            gNextBusNumber = newBusIndex + 1;
            break;
        }
        else if (dwError == ERROR_INVALID_PARAMETER || dwError == ERROR_FILE_NOT_FOUND)
        {
            tries++;
        }
        else
        {
            printf("otvmpAddVirtualBus failed, 0x%x!\r\n", dwError);
            otLogFuncExitMsg("otvmpAddVirtualBus failed");
            goto error;
        }
    }

    if (tries == 1000)
    {
        printf("otvmpAddVirtualBus failed to find an empty bus!\r\n");
        otLogFuncExitMsg("otvmpAddVirtualBus failed to find an empty bus");
        goto error;
    }

    if ((dwError = otvmpSetAdapterTopologyGuid(gVmpHandle, newBusIndex, &gTopologyGuid)) != ERROR_SUCCESS)
    {
        printf("otvmpSetAdapterTopologyGuid failed, 0x%x!\r\n", dwError);
        otLogFuncExitMsg("otvmpSetAdapterTopologyGuid failed");
        goto error;
    }

    if (ERROR_SUCCESS != ConvertInterfaceIndexToLuid(ifIndex, &ifLuid))
    {
        printf("ConvertInterfaceIndexToLuid(%u) failed!\r\n", ifIndex);
        otLogFuncExitMsg("ConvertInterfaceIndexToLuid failed");
        goto error;
    }

    if (ERROR_SUCCESS != ConvertInterfaceLuidToGuid(&ifLuid, &ifGuid))
    {
        printf("ConvertInterfaceLuidToGuid failed!\r\n");
        otLogFuncExitMsg("ConvertInterfaceLuidToGuid failed");
        goto error;
    }

    // Keep trying for up to 30 seconds
    auto StartTick = GetTickCount64();
    otInstance *instance = nullptr;
    do
    {
        instance = otInstanceInit(ApiInstance, &ifGuid);
        if (instance != nullptr) break;

        auto waitTimeMs = (30 * 1000 - (LONGLONG)(GetTickCount64() - StartTick));
        if (waitTimeMs <= 0) break;
        auto waitResult = WaitForSingleObject(gDeviceArrivalEvent, (DWORD)waitTimeMs);
        if (waitResult != WAIT_OBJECT_0) break;

    } while (true);

    if (instance == nullptr)
    {
        printf("otInstanceInit failed!\r\n");
        otLogFuncExitMsg("otInstanceInit failed");
        goto error;
    }

    GUID DeviceGuid = otGetDeviceGuid(instance);
    uint32_t Compartment = otGetCompartmentId(instance);

    node = new otNode();
    printf("%d: New Device " GUID_FORMAT " in compartment %d\r\n", id, GUID_ARG(DeviceGuid), Compartment);

    node->mId = id;
    node->mInterfaceIndex = ifIndex;
    node->mBusIndex = newBusIndex;
    node->mInstance = instance;

    node->mEnergyScanEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
    node->mPanIdConflictEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);

    EnterCriticalSection(&gCS);
    gNodes.push_back(node);
    LeaveCriticalSection(&gCS);

    InitializeCriticalSection(&node->mCS);

    // Reset any previously saved settings
    otInstanceFactoryReset(instance);

    otSetStateChangedCallback(instance, otNodeStateChangedCallback, node);

    HandleAddressChanges(node);

    otLogFuncExitMsg("success. [%d] = %!GUID!", id, &DeviceGuid);

error:

    if (node == nullptr)
    {
        if (BusAdded)
        {
            otvmpRemoveVirtualBus(gVmpHandle, newBusIndex);
        }

        ReleaseApiInstance();
    }

    return node;
}

OTNODEAPI int32_t OTCALL otNodeFinalize(otNode* aNode)
{
    otLogFuncEntry();
    if (aNode != nullptr)
    {
        printf("%d: Removing Device\r\n", aNode->mId);

        // Free any memory that we allocated now
        for each (auto mem in aNode->mMemoryToFree)
            free(mem);

        // Clean up callbacks
        CloseHandle(aNode->mPanIdConflictEvent);
        CloseHandle(aNode->mEnergyScanEvent);
        otSetStateChangedCallback(aNode->mInstance, nullptr, nullptr);

        EnterCriticalSection(&gCS);
        for (uint32_t i = 0; i < gNodes.size(); i++)
        {
            if (gNodes[i] == aNode)
            {
                gNodes.erase(gNodes.begin() + i);
                break;
            }
        }
        LeaveCriticalSection(&gCS);

        // Free the instance
        otFreeMemory(aNode->mInstance);
        aNode->mInstance = nullptr;

        // Free the ping handlers
        HandleAddressChanges(aNode);
        assert(aNode->mPingHandlers.size() == 0);
        if (aNode->mPingHandlers.size() != 0) printf("%d left over ping handlers!!!", (int)aNode->mPingHandlers.size());
        
        DeleteCriticalSection(&aNode->mCS);

        // Delete the virtual bus
        otvmpRemoveVirtualBus(gVmpHandle, aNode->mBusIndex);
        delete aNode;
        
        ReleaseApiInstance();
    }
    otLogFuncExit();
    return 0;
}

OTNODEAPI int32_t OTCALL otNodeSetMode(otNode* aNode, const char *aMode)
{
    otLogFuncEntryMsg("[%d] %s", aNode->mId, aMode);
    printf("%d: mode %s\r\n", aNode->mId, aMode);

    otLinkModeConfig linkMode = {0};

    const char *index = aMode;
    while (*index)
    {
        switch (*index)
        {
        case 'r':
            linkMode.mRxOnWhenIdle = true;
            break;
        case 's':
            linkMode.mSecureDataRequests = true;
            break;
        case 'd':
            linkMode.mDeviceType = true;
            break;
        case 'n':
            linkMode.mNetworkData = true;
            break;
        }

        index++;
    }

    auto result = otThreadSetLinkMode(aNode->mInstance, linkMode);
    
    otLogFuncExit();
    return result;
}

OTNODEAPI int32_t OTCALL otNodeInterfaceUp(otNode* aNode)
{
    otLogFuncEntryMsg("[%d]", aNode->mId);
    printf("%d: ifconfig up\r\n", aNode->mId);

    auto error = otIp6SetEnabled(aNode->mInstance, true);
    
    otLogFuncExit();
    return error;
}

OTNODEAPI int32_t OTCALL otNodeInterfaceDown(otNode* aNode)
{
    otLogFuncEntryMsg("[%d]", aNode->mId);
    printf("%d: ifconfig down\r\n", aNode->mId);

    (void)otIp6SetEnabled(aNode->mInstance, false);
    
    otLogFuncExit();
    return 0;
}

OTNODEAPI int32_t OTCALL otNodeThreadStart(otNode* aNode)
{
    otLogFuncEntryMsg("[%d]", aNode->mId);
    printf("%d: thread start\r\n", aNode->mId);

    auto error = otThreadSetEnabled(aNode->mInstance, true);
    
    otLogFuncExit();
    return error;
}

OTNODEAPI int32_t OTCALL otNodeThreadStop(otNode* aNode)
{
    otLogFuncEntryMsg("[%d]", aNode->mId);
    printf("%d: thread stop\r\n", aNode->mId);

    (void)otThreadSetEnabled(aNode->mInstance, false);
    
    otLogFuncExit();
    return 0;
}

OTNODEAPI int32_t OTCALL otNodeCommissionerStart(otNode* aNode)
{
    otLogFuncEntryMsg("[%d]", aNode->mId);
    printf("%d: commissioner start\r\n", aNode->mId);

    auto error = otCommissionerStart(aNode->mInstance);
    
    otLogFuncExit();
    return error;
}

OTNODEAPI int32_t OTCALL otNodeCommissionerJoinerAdd(otNode* aNode, const char *aExtAddr, const char *aPSKd)
{
    otLogFuncEntryMsg("[%d] %s %s", aNode->mId, aExtAddr, aPSKd);
    printf("%d: commissioner joiner add %s %s\r\n", aNode->mId, aExtAddr, aPSKd);

    const uint32_t kDefaultJoinerTimeout = 120;

    otError error;
    
    if (strcmp(aExtAddr, "*") == 0)
    {
        error = otCommissionerAddJoiner(aNode->mInstance, nullptr, aPSKd, kDefaultJoinerTimeout);
    }
    else
    {
        otExtAddress extAddr;
        if (Hex2Bin(aExtAddr, extAddr.m8, sizeof(extAddr)) != sizeof(extAddr))
            return OT_ERROR_PARSE;

        error = otCommissionerAddJoiner(aNode->mInstance, &extAddr, aPSKd, kDefaultJoinerTimeout);
    }
    
    otLogFuncExit();
    return error;
}

OTNODEAPI int32_t OTCALL otNodeCommissionerStop(otNode* aNode)
{
    otLogFuncEntryMsg("[%d]", aNode->mId);
    printf("%d: commissioner stop\r\n", aNode->mId);

    (void)otCommissionerStop(aNode->mInstance);
    
    otLogFuncExit();
    return 0;
}

OTNODEAPI int32_t OTCALL otNodeJoinerStart(otNode* aNode, const char *aPSKd, const char *aProvisioningUrl)
{
    otLogFuncEntryMsg("[%d] %s %s", aNode->mId, aPSKd, aProvisioningUrl);
    printf("%d: joiner start %s %s\r\n", aNode->mId, aPSKd, aProvisioningUrl);

    // TODO: handle the joiner completion callback
    auto error = otJoinerStart(aNode->mInstance, aPSKd, aProvisioningUrl, NULL, NULL, NULL, NULL, NULL, NULL);
    
    otLogFuncExit();
    return error;
}

OTNODEAPI int32_t OTCALL otNodeJoinerStop(otNode* aNode)
{
    otLogFuncEntryMsg("[%d]", aNode->mId);
    printf("%d: joiner stop\r\n", aNode->mId);

    (void)otJoinerStop(aNode->mInstance);
    
    otLogFuncExit();
    return 0;
}

OTNODEAPI int32_t OTCALL otNodeClearWhitelist(otNode* aNode)
{
    otLogFuncEntryMsg("[%d]", aNode->mId);
    printf("%d: whitelist clear\r\n", aNode->mId);

    otLinkFilterClearAddresses(aNode->mInstance);
    otLogFuncExit();
    return 0;
}

OTNODEAPI int32_t OTCALL otNodeEnableWhitelist(otNode* aNode)
{
    otLogFuncEntryMsg("[%d]", aNode->mId);
    printf("%d: whitelist enable\r\n", aNode->mId);

    otError error = otLinkFilterSetAddressMode(aNode->mInstance, OT_MAC_FILTER_ADDRESS_MODE_WHITELIST);
    otLogFuncExit();
    return error;
}

OTNODEAPI int32_t OTCALL otNodeDisableWhitelist(otNode* aNode)
{
    otLogFuncEntryMsg("[%d]", aNode->mId);
    printf("%d: whitelist disable\r\n", aNode->mId);

    otError error = otLinkFilterSetAddressMode(aNode->mInstance, OT_MAC_FILTER_ADDRESS_MODE_DISABLED);
    otLogFuncExit();
    return error;
}

OTNODEAPI int32_t OTCALL otNodeAddWhitelist(otNode* aNode,
        const char *aExtAddr, int8_t aRssi = OT_MAC_FILTER_FIXED_RSS_DISABLED)
{
    otLogFuncEntryMsg("[%d]", aNode->mId);
    otError error = OT_ERROR_NONE;

    otExtAddress extAddress;
    if (Hex2Bin(aExtAddr, extAddress.m8, OT_EXT_ADDRESS_SIZE) != OT_EXT_ADDRESS_SIZE)
        return OT_ERROR_PARSE;

    printf("%d: whitelist add %s", aNode->mId, aExtAddr);

    error = otLinkFilterAddAddress(aNode->mInstance, &extAddress);

    if (error == OT_ERROR_NONE || error == OT_ERROR_ALREADY)
    {
        if (aRssi != OT_MAC_FILTER_FIXED_RSS_DISABLED)
        {
            error = otLinkFilterAddRssIn(aNode->mInstance, &extAddress, aRssi);
            printf(" %d", aRssi);
        }
    }

    printf("\r\n");

    otLogFuncExit();
    return error;
}

OTNODEAPI int32_t OTCALL otNodeRemoveWhitelist(otNode* aNode, const char *aExtAddr)
{
    otLogFuncEntryMsg("[%d]", aNode->mId);
    printf("%d: whitelist remove %s\r\n", aNode->mId, aExtAddr);

    otExtAddress extAddress;
    if (Hex2Bin(aExtAddr, extAddress.m8, OT_EXT_ADDRESS_SIZE) != OT_EXT_ADDRESS_SIZE)
        return OT_ERROR_PARSE;

    otError error = otLinkFilterRemoveAddress(aNode->mInstance, &extAddress);
    otLogFuncExit();
    return error;
}

OTNODEAPI uint16_t OTCALL otNodeGetAddr16(otNode* aNode)
{
    otLogFuncEntryMsg("[%d]", aNode->mId);
    auto result = otThreadGetRloc16(aNode->mInstance);
    printf("%d: rloc16\r\n%04x\r\n", aNode->mId, result);
    otLogFuncExit();
    return result;
}

OTNODEAPI const char* OTCALL otNodeGetAddr64(otNode* aNode)
{
    otLogFuncEntryMsg("[%d]", aNode->mId);
    auto extAddr = otLinkGetExtendedAddress(aNode->mInstance);
    char* str = (char*)malloc(18);
    if (str != nullptr)
    {
        aNode->mMemoryToFree.push_back(str);
        for (int i = 0; i < 8; i++)
            sprintf_s(str + i * 2, 18 - (2 * i), "%02x", extAddr->m8[i]);
        printf("%d: extaddr\r\n%s\r\n", aNode->mId, str);
    }
    otFreeMemory(extAddr);
    otLogFuncExit();
    return str;
}

OTNODEAPI const char* OTCALL otNodeGetEui64(otNode* aNode)
{
    otLogFuncEntryMsg("[%d]", aNode->mId);
    otExtAddress aEui64 = {};
    otLinkGetFactoryAssignedIeeeEui64(aNode->mInstance, &aEui64);
    char* str = (char*)malloc(18);
    if (str != nullptr)
    {
        aNode->mMemoryToFree.push_back(str);
        for (int i = 0; i < 8; i++)
            sprintf_s(str + i * 2, 18 - (2 * i), "%02x", aEui64.m8[i]);
        printf("%d: eui64\r\n%s\r\n", aNode->mId, str);
    }
    otLogFuncExit();
    return str;
}

OTNODEAPI const char* OTCALL otNodeGetJoinerId(otNode* aNode)
{
    otLogFuncEntryMsg("[%d]", aNode->mId);
    otExtAddress aJoinerId = {};
    otJoinerGetId(aNode->mInstance, &aJoinerId);
    char* str = (char*)malloc(18);
    if (str != nullptr)
    {
        aNode->mMemoryToFree.push_back(str);
        for (int i = 0; i < 8; i++)
            sprintf_s(str + i * 2, 18 - (2 * i), "%02x", aJoinerId.m8[i]);
        printf("%d: joinerid\r\n%s\r\n", aNode->mId, str);
    }
    otLogFuncExit();
    return str;
}

OTNODEAPI int32_t OTCALL otNodeSetChannel(otNode* aNode, uint8_t aChannel)
{
    otLogFuncEntryMsg("[%d]", aNode->mId);
    printf("%d: channel %d\r\n", aNode->mId, aChannel);
    auto result = otLinkSetChannel(aNode->mInstance, aChannel);
    otLogFuncExit();
    return result;
}

OTNODEAPI uint8_t OTCALL otNodeGetChannel(otNode* aNode)
{
    otLogFuncEntryMsg("[%d]", aNode->mId);
    auto result = otLinkGetChannel(aNode->mInstance);
    printf("%d: channel\r\n%d\r\n", aNode->mId, result);
    otLogFuncExit();
    return result;
}

OTNODEAPI int32_t OTCALL otNodeSetMasterkey(otNode* aNode, const char *aMasterkey)
{
    otLogFuncEntryMsg("[%d] %s", aNode->mId, aMasterkey);
    printf("%d: masterkey %s\r\n", aNode->mId, aMasterkey);

    int keyLength;
    otMasterKey key;
    if ((keyLength = Hex2Bin(aMasterkey, key.m8, sizeof(key.m8))) != OT_MASTER_KEY_SIZE)
    {
        printf("invalid length key %d\r\n", keyLength);
        return OT_ERROR_PARSE;
    }

    auto error = otThreadSetMasterKey(aNode->mInstance, &key);
    otLogFuncExit();
    return error;
}

OTNODEAPI const char* OTCALL otNodeGetMasterkey(otNode* aNode)
{
    otLogFuncEntryMsg("[%d]", aNode->mId);
    auto aMasterKey = otThreadGetMasterKey(aNode->mInstance);
    uint8_t strLength = 2*sizeof(otMasterKey) + 1;
    char* str = (char*)malloc(strLength);
    if (str != nullptr)
    {
        aNode->mMemoryToFree.push_back(str);
        for (int i = 0; i < sizeof(otMasterKey); i++)
            sprintf_s(str + i * 2, strLength - (2 * i), "%02x", aMasterKey->m8[i]);
        printf("%d: masterkey\r\n%s\r\n", aNode->mId, str);
    }
    otFreeMemory(aMasterKey);
    otLogFuncExit();
    return str;
}

OTNODEAPI int32_t OTCALL otNodeSetPSKc(otNode* aNode, const char *aPSKc)
{
    otLogFuncEntryMsg("[%d] %s", aNode->mId, aPSKc);
    printf("%d: pskc %s\r\n", aNode->mId, aPSKc);

    uint8_t pskc[OT_PSKC_MAX_SIZE];
    if (Hex2Bin(aPSKc, pskc, sizeof(pskc)) != OT_PSKC_MAX_SIZE)
    {
        printf("invalid pskc %s\r\n", aPSKc);
        return OT_ERROR_PARSE;
    }

    auto error = otThreadSetPSKc(aNode->mInstance, pskc);
    otLogFuncExit();
    return error;
}

OTNODEAPI const char* OTCALL otNodeGetPSKc(otNode* aNode)
{
    otLogFuncEntryMsg("[%d]", aNode->mId);
    auto aPSKc = otThreadGetPSKc(aNode->mInstance);
    uint8_t strLength = 2 * OT_PSKC_MAX_SIZE + 1;
    char* str = (char*)malloc(strLength);
    if (str != nullptr)
    {
        aNode->mMemoryToFree.push_back(str);
        for (int i = 0; i < OT_PSKC_MAX_SIZE; i++)
            sprintf_s(str + i * 2, strLength - (2 * i), "%02x", aPSKc[i]);
        printf("%d: pskc\r\n%s\r\n", aNode->mId, str);
    }
    otFreeMemory(aPSKc);
    otLogFuncExit();
    return str;
}

OTNODEAPI uint32_t OTCALL otNodeGetKeySequenceCounter(otNode* aNode)
{
    otLogFuncEntryMsg("[%d]", aNode->mId);
    auto result = otThreadGetKeySequenceCounter(aNode->mInstance);
    printf("%d: keysequence\r\n%d\r\n", aNode->mId, result);
    otLogFuncExit();
    return result;
}

OTNODEAPI int32_t OTCALL otNodeSetKeySequenceCounter(otNode* aNode, uint32_t aSequence)
{
    otLogFuncEntryMsg("[%d]", aNode->mId);
    printf("%d: keysequence counter %d\r\n", aNode->mId, aSequence);
    otThreadSetKeySequenceCounter(aNode->mInstance, aSequence);
    otLogFuncExit();
    return 0;
}

OTNODEAPI int32_t OTCALL otNodeSetKeySwitchGuardTime(otNode* aNode, uint32_t aKeySwitchGuardTime)
{
    otLogFuncEntryMsg("[%d]", aNode->mId);
    printf("%d: keysequence guardtime %d\r\n", aNode->mId, aKeySwitchGuardTime);
    otThreadSetKeySwitchGuardTime(aNode->mInstance, aKeySwitchGuardTime);
    otLogFuncExit();
    return 0;
}

OTNODEAPI int32_t OTCALL otNodeSetNetworkIdTimeout(otNode* aNode, uint8_t aTimeout)
{
    otLogFuncEntryMsg("[%d]", aNode->mId);
    printf("%d: networkidtimeout %d\r\n", aNode->mId, aTimeout);
    otThreadSetNetworkIdTimeout(aNode->mInstance, aTimeout);
    otLogFuncExit();
    return 0;
}

OTNODEAPI int32_t OTCALL otNodeSetNetworkName(otNode* aNode, const char *aName)
{
    otLogFuncEntryMsg("[%d]", aNode->mId);
    printf("%d: networkname %s\r\n", aNode->mId, aName);
    auto result = otThreadSetNetworkName(aNode->mInstance, aName);
    otLogFuncExit();
    return result;
}

OTNODEAPI const char* OTCALL otNodeGetNetworkName(otNode* aNode)
{
    otLogFuncEntryMsg("[%d]", aNode->mId);
    auto result = otThreadGetNetworkName(aNode->mInstance);
    aNode->mMemoryToFree.push_back((char*)result);
    printf("%d: networkname\r\n%s\r\n", aNode->mId, result);
    otLogFuncExit();
    return result;
}

OTNODEAPI uint16_t OTCALL otNodeGetPanId(otNode* aNode)
{
    otLogFuncEntryMsg("[%d]", aNode->mId);
    auto result = otLinkGetPanId(aNode->mInstance);
    printf("%d: panid\r\n0x%04x\r\n", aNode->mId, result);
    otLogFuncExit();
    return result;
}

OTNODEAPI int32_t OTCALL otNodeSetPanId(otNode* aNode, uint16_t aPanId)
{
    otLogFuncEntryMsg("[%d]", aNode->mId);
    printf("%d: panid 0x%04x\r\n", aNode->mId, aPanId);
    auto result = otLinkSetPanId(aNode->mInstance, aPanId);
    otLogFuncExit();
    return result;
}

OTNODEAPI uint32_t OTCALL otNodeGetPartitionId(otNode* aNode)
{
    otLogFuncEntryMsg("[%d]", aNode->mId);
    auto result = otThreadGetLocalLeaderPartitionId(aNode->mInstance);
    printf("%d: leaderpartitionid\r\n0x%04x\r\n", aNode->mId, result);
    otLogFuncExit();
    return result;
}

OTNODEAPI int32_t OTCALL otNodeSetPartitionId(otNode* aNode, uint32_t aPartitionId)
{
    otLogFuncEntryMsg("[%d]", aNode->mId);
    printf("%d: leaderpartitionid 0x%04x\r\n", aNode->mId, aPartitionId);
    otThreadSetLocalLeaderPartitionId(aNode->mInstance, aPartitionId);
    otLogFuncExit();
    return 0;
}

OTNODEAPI int32_t OTCALL otNodeSetRouterUpgradeThreshold(otNode* aNode, uint8_t aThreshold)
{
    otLogFuncEntryMsg("[%d]", aNode->mId);
    printf("%d: routerupgradethreshold %d\r\n", aNode->mId, aThreshold);
    otThreadSetRouterUpgradeThreshold(aNode->mInstance, aThreshold);
    otLogFuncExit();
    return 0;
}

OTNODEAPI int32_t OTCALL otNodeSetRouterDowngradeThreshold(otNode* aNode, uint8_t aThreshold)
{
    otLogFuncEntryMsg("[%d]", aNode->mId);
    printf("%d: routerdowngradethreshold %d\r\n", aNode->mId, aThreshold);
    otThreadSetRouterDowngradeThreshold(aNode->mInstance, aThreshold);
    otLogFuncExit();
    return 0;
}

OTNODEAPI int32_t OTCALL otNodeReleaseRouterId(otNode* aNode, uint8_t aRouterId)
{
    otLogFuncEntryMsg("[%d]", aNode->mId);
    printf("%d: releaserouterid %d\r\n", aNode->mId, aRouterId);
    auto result = otThreadReleaseRouterId(aNode->mInstance, aRouterId);
    otLogFuncExit();
    return result;
}

OTNODEAPI const char* OTCALL otNodeGetState(otNode* aNode)
{
    otLogFuncEntryMsg("[%d]", aNode->mId);
    auto role = otThreadGetDeviceRole(aNode->mInstance);
    auto result = _strdup(otDeviceRoleToString(role));
    aNode->mMemoryToFree.push_back(result);
    printf("%d: state\r\n%s\r\n", aNode->mId, result);
    otLogFuncExit();
    return result;
}

OTNODEAPI int32_t OTCALL otNodeSetState(otNode* aNode, const char *aState)
{
    otLogFuncEntryMsg("[%d]", aNode->mId);
    printf("%d: state %s\r\n", aNode->mId, aState);

    otError error;
    if (strcmp(aState, "detached") == 0)
    {
        error = otThreadBecomeDetached(aNode->mInstance);
    }
    else if (strcmp(aState, "child") == 0)
    {
        error = otThreadBecomeChild(aNode->mInstance);
    }
    else if (strcmp(aState, "router") == 0)
    {
        error = otThreadBecomeRouter(aNode->mInstance);
    }
    else if (strcmp(aState, "leader") == 0)
    {
        error = otThreadBecomeLeader(aNode->mInstance);
    }
    else
    {
        error = OT_ERROR_INVALID_ARGS;
    }
    otLogFuncExit();
    return error;
}

OTNODEAPI uint32_t OTCALL otNodeGetTimeout(otNode* aNode)
{
    otLogFuncEntryMsg("[%d]", aNode->mId);
    auto result = otThreadGetChildTimeout(aNode->mInstance);
    printf("%d: childtimeout\r\n%d\r\n", aNode->mId, result);
    otLogFuncExit();
    return result;
}

OTNODEAPI int32_t OTCALL otNodeSetTimeout(otNode* aNode, uint32_t aTimeout)
{
    otLogFuncEntryMsg("[%d]", aNode->mId);
    printf("%d: childtimeout %d\r\n", aNode->mId, aTimeout);
    otThreadSetChildTimeout(aNode->mInstance, aTimeout);
    otLogFuncExit();
    return 0;
}

OTNODEAPI uint8_t OTCALL otNodeGetWeight(otNode* aNode)
{
    otLogFuncEntryMsg("[%d]", aNode->mId);
    auto result = otThreadGetLeaderWeight(aNode->mInstance);
    printf("%d: leaderweight\r\n%d\r\n", aNode->mId, result);
    otLogFuncExit();
    return result;
}

OTNODEAPI int32_t OTCALL otNodeSetWeight(otNode* aNode, uint8_t aWeight)
{
    otLogFuncEntryMsg("[%d]", aNode->mId);
    printf("%d: leaderweight %d\r\n", aNode->mId, aWeight);
    otThreadSetLocalLeaderWeight(aNode->mInstance, aWeight);
    otLogFuncExit();
    return 0;
}

OTNODEAPI int32_t OTCALL otNodeAddIpAddr(otNode* aNode, const char *aAddr)
{
    otLogFuncEntryMsg("[%d]", aNode->mId);
    printf("%d: add ipaddr %s\r\n", aNode->mId, aAddr);

    otNetifAddress aAddress;
    auto error = otIp6AddressFromString(aAddr, &aAddress.mAddress);
    if (error != OT_ERROR_NONE) return error;

    aAddress.mPrefixLength = 64;
    aAddress.mPreferred = true;
    aAddress.mValid = true;
    auto result = otIp6AddUnicastAddress(aNode->mInstance, &aAddress);
    otLogFuncExit();
    return result;
}

inline uint16_t Swap16(uint16_t v)
{
    return
        (((v & 0x00ffU) << 8) & 0xff00) |
        (((v & 0xff00U) >> 8) & 0x00ff);
}

OTNODEAPI const char* OTCALL otNodeGetAddrs(otNode* aNode)
{
    otLogFuncEntryMsg("[%d]", aNode->mId);
    printf("%d: ipaddr\r\n", aNode->mId);

    auto addrs = otIp6GetUnicastAddresses(aNode->mInstance);
    if (addrs == nullptr) return nullptr;

    char* str = (char*)malloc(512);
    if (str != nullptr)
    {
        aNode->mMemoryToFree.push_back(str);
        RtlZeroMemory(str, 512);

        char* cur = str;
    
        for (const otNetifAddress *addr = addrs; addr; addr = addr->mNext)
        {
            if (cur != str)
            {
                *cur = '\n';
                cur++;
            }

            auto last = cur;

            cur += 
                sprintf_s(
                    cur, 512 - (cur - str),
                    "%x:%x:%x:%x:%x:%x:%x:%x",
                    Swap16(addr->mAddress.mFields.m16[0]),
                    Swap16(addr->mAddress.mFields.m16[1]),
                    Swap16(addr->mAddress.mFields.m16[2]),
                    Swap16(addr->mAddress.mFields.m16[3]),
                    Swap16(addr->mAddress.mFields.m16[4]),
                    Swap16(addr->mAddress.mFields.m16[5]),
                    Swap16(addr->mAddress.mFields.m16[6]),
                    Swap16(addr->mAddress.mFields.m16[7]));

            printf("%s\r\n", last);
        }
    }

    otFreeMemory(addrs);
    otLogFuncExit();

    return str;
}

OTNODEAPI uint32_t OTCALL otNodeGetContextReuseDelay(otNode* aNode)
{
    otLogFuncEntryMsg("[%d]", aNode->mId);
    auto result = otThreadGetContextIdReuseDelay(aNode->mInstance);
    printf("%d: contextreusedelay\r\n%d\r\n", aNode->mId, result);
    otLogFuncExit();
    return result;
}

OTNODEAPI int32_t OTCALL otNodeSetContextReuseDelay(otNode* aNode, uint32_t aDelay)
{
    otLogFuncEntryMsg("[%d] %d", aNode->mId, aDelay);
    printf("%d: contextreusedelay %d\r\n", aNode->mId, aDelay);
    otThreadSetContextIdReuseDelay(aNode->mInstance, aDelay);
    otLogFuncExit();
    return 0;
}

OTNODEAPI int32_t OTCALL otNodeAddPrefix(otNode* aNode, const char *aPrefix, const char *aFlags, const char *aPreference)
{
    otLogFuncEntryMsg("[%d]", aNode->mId);
    printf("%d: prefix add %s %s %s\r\n", aNode->mId, aPrefix, aFlags, aPreference);

    otBorderRouterConfig config = {0};

    auto error = otNodeParsePrefix(aPrefix, &config.mPrefix);
    if (error != OT_ERROR_NONE) return error;
    
    const char *index = aFlags;
    while (*index)
    {
        switch (*index)
        {
        case 'p':
            config.mPreferred = true;
            break;
        case 'a':
            config.mSlaac = true;
            break;
        case 'd':
            config.mDhcp = true;
            break;
        case 'c':
            config.mConfigure = true;
            break;
        case 'r':
            config.mDefaultRoute = true;
            break;
        case 'o':
            config.mOnMesh = true;
            break;
        case 's':
            config.mStable = true;
            break;
        default:
            return OT_ERROR_INVALID_ARGS;
        }

        index++;
    }
    
    if (strcmp(aPreference, "high") == 0)
    {
        config.mPreference = OT_ROUTE_PREFERENCE_HIGH;
    }
    else if (strcmp(aPreference, "med") == 0)
    {
        config.mPreference = OT_ROUTE_PREFERENCE_MED;
    }
    else if (strcmp(aPreference, "low") == 0)
    {
        config.mPreference = OT_ROUTE_PREFERENCE_LOW;
    }
    else
    {
        return OT_ERROR_INVALID_ARGS;
    }

    auto result = otBorderRouterAddOnMeshPrefix(aNode->mInstance, &config);
    otLogFuncExit();
    return result;
}

OTNODEAPI int32_t OTCALL otNodeRemovePrefix(otNode* aNode, const char *aPrefix)
{
    otLogFuncEntryMsg("[%d]", aNode->mId);

    otIp6Prefix prefix;
    auto error = otNodeParsePrefix(aPrefix, &prefix);
    if (error != OT_ERROR_NONE) return error;

    auto result = otBorderRouterRemoveOnMeshPrefix(aNode->mInstance, &prefix);
    otLogFuncExit();
    return result;
}

OTNODEAPI int32_t OTCALL otNodeAddRoute(otNode* aNode, const char *aPrefix, const char *aPreference)
{
    otLogFuncEntryMsg("[%d]", aNode->mId);
    otExternalRouteConfig config = {0};

    auto error = otNodeParsePrefix(aPrefix, &config.mPrefix);
    if (error != OT_ERROR_NONE) return error;
    
    if (strcmp(aPreference, "high") == 0)
    {
        config.mPreference = OT_ROUTE_PREFERENCE_HIGH;
    }
    else if (strcmp(aPreference, "med") == 0)
    {
        config.mPreference = OT_ROUTE_PREFERENCE_MED;
    }
    else if (strcmp(aPreference, "low") == 0)
    {
        config.mPreference = OT_ROUTE_PREFERENCE_LOW;
    }
    else
    {
        return OT_ERROR_INVALID_ARGS;
    }

    auto result = otBorderRouterAddRoute(aNode->mInstance, &config);
    otLogFuncExit();
    return result;
}

OTNODEAPI int32_t OTCALL otNodeRemoveRoute(otNode* aNode, const char *aPrefix)
{
    otLogFuncEntryMsg("[%d]", aNode->mId);

    otIp6Prefix prefix;
    auto error = otNodeParsePrefix(aPrefix, &prefix);
    if (error != OT_ERROR_NONE) return error;

    auto result = otBorderRouterRemoveRoute(aNode->mInstance, &prefix);
    otLogFuncExit();
    return result;
}

OTNODEAPI int32_t OTCALL otNodeRegisterNetdata(otNode* aNode)
{
    otLogFuncEntryMsg("[%d]", aNode->mId);
    printf("%d: registernetdata\r\n", aNode->mId);
    auto result = otBorderRouterRegister(aNode->mInstance);
    otLogFuncExit();
    return result;
}

void OTCALL otNodeCommissionerEnergyReportCallback(uint32_t aChannelMask, const uint8_t *aEnergyList, uint8_t aEnergyListLength, void *aContext)
{
    otNode* aNode = (otNode*)aContext;

    printf("Energy: 0x%08x\r\n", aChannelMask);
    for (uint8_t i = 0; i < aEnergyListLength; i++)
        printf("%d ", aEnergyList[i]);
    printf("\r\n");

    SetEvent(aNode->mEnergyScanEvent);
}

OTNODEAPI int32_t OTCALL otNodeEnergyScan(otNode* aNode, uint32_t aMask, uint8_t aCount, uint16_t aPeriod, uint16_t aDuration, const char *aAddr)
{
    otLogFuncEntryMsg("[%d]", aNode->mId);
    printf("%d: energy scan 0x%x %d %d %d %s\r\n", aNode->mId, aMask, aCount, aPeriod, aDuration, aAddr);

    otIp6Address address = {0};
    auto error = otIp6AddressFromString(aAddr, &address);
    if (error != OT_ERROR_NONE)
    {
        printf("otIp6AddressFromString(%s) failed, 0x%x!\r\n", aAddr, error);
        return error;
    }
    
    ResetEvent(aNode->mEnergyScanEvent);

    error = otCommissionerEnergyScan(aNode->mInstance, aMask, aCount, aPeriod, aDuration, &address, otNodeCommissionerEnergyReportCallback, aNode);
    if (error != OT_ERROR_NONE)
    {
        printf("otCommissionerEnergyScan failed, 0x%x!\r\n", error);
        return error;
    }

    auto result = WaitForSingleObject(aNode->mEnergyScanEvent, 8000) == WAIT_OBJECT_0 ? OT_ERROR_NONE : OT_ERROR_NOT_FOUND;
    otLogFuncExit();
    return result;
}

void OTCALL otNodeCommissionerPanIdConflictCallback(uint16_t aPanId, uint32_t aChannelMask, void *aContext)
{
    otNode* aNode = (otNode*)aContext;
    printf("Conflict: 0x%04x, 0x%08x\r\n", aPanId, aChannelMask);
    SetEvent(aNode->mPanIdConflictEvent);
}

OTNODEAPI int32_t OTCALL otNodePanIdQuery(otNode* aNode, uint16_t aPanId, uint32_t aMask, const char *aAddr)
{
    otLogFuncEntryMsg("[%d]", aNode->mId);
    printf("%d: panid query 0x%04x 0x%x %s\r\n", aNode->mId, aPanId, aMask, aAddr);

    otIp6Address address = {0};
    auto error = otIp6AddressFromString(aAddr, &address);
    if (error != OT_ERROR_NONE)
    {
        printf("otIp6AddressFromString(%s) failed, 0x%x!\r\n", aAddr, error);
        return error;
    }
    
    ResetEvent(aNode->mPanIdConflictEvent);

    error = otCommissionerPanIdQuery(aNode->mInstance, aPanId, aMask, &address, otNodeCommissionerPanIdConflictCallback, aNode);
    if (error != OT_ERROR_NONE)
    {
        printf("otCommissionerPanIdQuery failed, 0x%x!\r\n", error);
        return error;
    }

    auto result = WaitForSingleObject(aNode->mPanIdConflictEvent, 8000) == WAIT_OBJECT_0 ? OT_ERROR_NONE : OT_ERROR_NOT_FOUND;
    otLogFuncExit();
    return result;
}

OTNODEAPI const char* OTCALL otNodeScan(otNode* aNode)
{
    otLogFuncEntryMsg("[%d]", aNode->mId);
    UNREFERENCED_PARAMETER(aNode);
    otLogFuncExit();
    return nullptr;
}

OTNODEAPI uint32_t OTCALL otNodePing(otNode* aNode, const char *aAddr, uint16_t aSize, uint32_t aMinReplies, uint16_t aTimeout)
{
    otLogFuncEntryMsg("[%d] %s (%d bytes)", aNode->mId, aAddr, aSize);
    printf("%d: ping %s (%d bytes)\r\n", aNode->mId, aAddr, aSize);

    // Convert string to destination address
    otIp6Address otDestinationAddress = {0};
    auto error = otIp6AddressFromString(aAddr, &otDestinationAddress);
    if (error != OT_ERROR_NONE)
    {
        printf("otIp6AddressFromString(%s) failed!\r\n", aAddr);
        return 0;
    }
    
    // Get ML-EID as source address for ping
    auto otSourceAddress = otThreadGetMeshLocalEid(aNode->mInstance);

    sockaddr_in6 SourceAddress = { AF_INET6, (USHORT)(CertificationPingPort + 1) };
    sockaddr_in6 DestinationAddress = { AF_INET6, CertificationPingPort };

    memcpy(&SourceAddress.sin6_addr, otSourceAddress, sizeof(IN6_ADDR));
    memcpy(&DestinationAddress.sin6_addr, &otDestinationAddress, sizeof(IN6_ADDR));

    otFreeMemory(otSourceAddress);
    otSourceAddress = nullptr;
    
    // Put the current thead in the correct compartment
    bool RevertCompartmentOnExit = false;
    ULONG OriginalCompartmentID = GetCurrentThreadCompartmentId();
    if (OriginalCompartmentID != otGetCompartmentId(aNode->mInstance))
    {
        DWORD dwError = ERROR_SUCCESS;
        if ((dwError = SetCurrentThreadCompartmentId(otGetCompartmentId(aNode->mInstance))) != ERROR_SUCCESS)
        {
            printf("SetCurrentThreadCompartmentId failed, 0x%x\r\n", dwError);
        }
        RevertCompartmentOnExit = true;
    }

    int result = 0;

    auto SendBuffer = (PCHAR)malloc(aSize);
    auto RecvBuffer = (PCHAR)malloc(aSize);

    WSABUF WSARecvBuffer = { aSize, RecvBuffer };
    
    WSAOVERLAPPED Overlapped = { 0 };
    Overlapped.hEvent = WSACreateEvent();

    DWORD numberOfReplies = 0;
    bool isPending = false;
    DWORD Flags;
    DWORD cbReceived;
    int cbDestinationAddress = sizeof(DestinationAddress);
    DWORD hopLimit = 64;

    SOCKET Socket = WSASocketW(AF_INET6, SOCK_DGRAM, IPPROTO_UDP, NULL, 0, WSA_FLAG_OVERLAPPED);
    if (Socket == INVALID_SOCKET)
    {
        printf("WSASocket failed, 0x%x\r\n", WSAGetLastError());
        goto exit;
    }

    // Bind the socket to the address
    result = bind(Socket, (sockaddr*)&SourceAddress, sizeof(SourceAddress));
    if (result == SOCKET_ERROR)
    {
        printf("bind failed, 0x%x\r\n", WSAGetLastError());
        goto exit;
    }
    
    // Set the multicast hop limit to 64
    result = setsockopt(Socket, IPPROTO_IPV6, IPV6_MULTICAST_HOPS, (char *)&hopLimit, sizeof(hopLimit));
    if (result == SOCKET_ERROR)
    {
        printf("setsockopt (IPV6_MULTICAST_HOPS) failed, 0x%x\r\n", WSAGetLastError());
        goto exit;
    }

    // Initialize the send buffer pattern.
    for (uint32_t i = 0; i < aSize; i++)
        SendBuffer[i] = (char)('a' + (i % 23));

    // Hack to retrieve destination on other end
    memcpy_s(SendBuffer, aSize, &otDestinationAddress, sizeof(IN6_ADDR));

    // Send the buffer
    result = sendto(Socket, SendBuffer, aSize, 0, (SOCKADDR*)&DestinationAddress, sizeof(DestinationAddress));
    if (result == SOCKET_ERROR)
    {
        printf("sendto failed, 0x%x\r\n", WSAGetLastError());
        goto exit;
    }

    auto StartTick = GetTickCount64();
    
    while (numberOfReplies < aMinReplies)
    {
        Flags = 0; //MSG_PARTIAL;
        result = WSARecvFrom(Socket, &WSARecvBuffer, 1, &cbReceived, &Flags, (SOCKADDR*)&DestinationAddress, &cbDestinationAddress, &Overlapped, NULL);
        if (result == SOCKET_ERROR)
        {
            result = WSAGetLastError();
            if (result == WSA_IO_PENDING)
            {
                isPending = true;
            }
            else
            {
                printf("WSARecvFrom failed, 0x%x\r\n", result);
                goto exit;
            }
        }

        if (isPending)
        {
            //printf("waiting for completion event...\r\n");
            // Wait for the receive to complete
            ULONGLONG elapsed = (GetTickCount64() - StartTick);
            result = WSAWaitForMultipleEvents(1, &Overlapped.hEvent, TRUE, (DWORD)(aTimeout - min(aTimeout, elapsed)), TRUE);
            if (result == WSA_WAIT_TIMEOUT)
            {
                //printf("recv timeout\r\n");
                goto exit;
            }
            else if (result == WSA_WAIT_FAILED)
            {
                printf("recv failed\r\n");
                goto exit;
            }
        }

        result = WSAGetOverlappedResult(Socket, &Overlapped, &cbReceived, TRUE, &Flags);
        if (result == FALSE)
        {
            printf("WSAGetOverlappedResult failed, 0x%x\r\n", WSAGetLastError());
            goto exit;
        }

        numberOfReplies++;
    }

exit:
    
    // Revert the comparment if necessary
    if (RevertCompartmentOnExit)
    {
        (VOID)SetCurrentThreadCompartmentId(OriginalCompartmentID);
    }

    free(RecvBuffer);
    free(SendBuffer);

    WSACloseEvent(Overlapped.hEvent);

    if (Socket != INVALID_SOCKET) closesocket(Socket);

    otLogFuncExit();

    return numberOfReplies;
}

OTNODEAPI int32_t OTCALL otNodeSetRouterSelectionJitter(otNode* aNode, uint8_t aRouterJitter)
{
    otLogFuncEntryMsg("[%d] %d", aNode->mId, aRouterJitter);
    printf("%d: routerselectionjitter %d\r\n", aNode->mId, aRouterJitter);
    otThreadSetRouterSelectionJitter(aNode->mInstance, aRouterJitter);
    otLogFuncExit();
    return 0;
}

OTNODEAPI int32_t OTCALL otNodeCommissionerAnnounceBegin(otNode* aNode, uint32_t aChannelMask, uint8_t aCount, uint16_t aPeriod, const char *aAddr)
{
    otLogFuncEntryMsg("[%d] 0x%08x %d %d %s", aNode->mId, aChannelMask, aCount, aPeriod, aAddr);
    printf("%d: commissioner announce 0x%08x %d %d %s\r\n", aNode->mId, aChannelMask, aCount, aPeriod, aAddr);

    otIp6Address aAddress;
    auto error = otIp6AddressFromString(aAddr, &aAddress);
    if (error != OT_ERROR_NONE) return error;

    auto result = otCommissionerAnnounceBegin(aNode->mInstance, aChannelMask, aCount, aPeriod, &aAddress);
    otLogFuncExit();
    return result;
}

OTNODEAPI int32_t OTCALL otNodeSetActiveDataset(otNode* aNode, uint64_t aTimestamp, uint16_t aPanId, uint16_t aChannel, uint32_t aChannelMask, const char *aMasterKey)
{
    otLogFuncEntryMsg("[%d] 0x%llX %d %d", aNode->mId, aTimestamp, aPanId, aChannel);
    printf("%d: dataset set active 0x%llX %d %d\r\n", aNode->mId, aTimestamp, aPanId, aChannel);

    otOperationalDataset aDataset = {};

    aDataset.mActiveTimestamp = aTimestamp;
    aDataset.mComponents.mIsActiveTimestampPresent = true;

    if (aPanId != 0)
    {
        aDataset.mPanId = aPanId;
        aDataset.mComponents.mIsPanIdPresent = true;
    }

    if (aChannel != 0)
    {
        aDataset.mChannel = aChannel;
        aDataset.mComponents.mIsChannelPresent = true;
    }

    if (aChannelMask != 0)
    {
        aDataset.mChannelMaskPage0 = aChannelMask;
        aDataset.mComponents.mIsChannelMaskPage0Present = true;
    }

    if (aMasterKey != NULL && strlen(aMasterKey) != 0)
    {
        int keyLength;
        if ((keyLength = Hex2Bin(aMasterKey, aDataset.mMasterKey.m8, sizeof(aDataset.mMasterKey))) != OT_MASTER_KEY_SIZE)
        {
            printf("invalid length key %d\r\n", keyLength);
            return OT_ERROR_PARSE;
        }
        aDataset.mComponents.mIsMasterKeyPresent = true;
    }

    auto result = otDatasetSetActive(aNode->mInstance, &aDataset);
    otLogFuncExit();
    return result;
}

OTNODEAPI int32_t OTCALL otNodeSetPendingDataset(otNode* aNode, uint64_t aActiveTimestamp, uint64_t aPendingTimestamp, uint16_t aPanId, uint16_t aChannel)
{
    otLogFuncEntryMsg("[%d] 0x%llX 0x%llX %d %d", aNode->mId, aActiveTimestamp, aPendingTimestamp, aPanId, aChannel);
    printf("%d: dataset set pending 0x%llX 0x%llX %d %d\r\n", aNode->mId, aActiveTimestamp, aPendingTimestamp, aPanId, aChannel);

    otOperationalDataset aDataset = {};

    if (aActiveTimestamp != 0)
    {
        aDataset.mActiveTimestamp = aActiveTimestamp;
        aDataset.mComponents.mIsActiveTimestampPresent = true;
    }

    if (aPendingTimestamp != 0)
    {
        aDataset.mPendingTimestamp = aPendingTimestamp;
        aDataset.mComponents.mIsPendingTimestampPresent = true;
    }

    if (aPanId != 0)
    {
        aDataset.mPanId = aPanId;
        aDataset.mComponents.mIsPanIdPresent = true;
    }

    if (aChannel != 0)
    {
        aDataset.mChannel = aChannel;
        aDataset.mComponents.mIsChannelPresent = true;
    }

    auto result = otDatasetSetPending(aNode->mInstance, &aDataset);
    otLogFuncExit();
    return result;
}

OTNODEAPI int32_t OTCALL otNodeSendPendingSet(otNode* aNode, uint64_t aActiveTimestamp, uint64_t aPendingTimestamp, uint32_t aDelayTimer, uint16_t aPanId, uint16_t aChannel, const char *aMasterKey, const char *aMeshLocal, const char *aNetworkName)
{
    otLogFuncEntryMsg("[%d] 0x%llX 0x%llX %d %d", aNode->mId, aActiveTimestamp, aPendingTimestamp, aPanId, aChannel);
    printf("%d: dataset send pending 0x%llX 0x%llX %d %d\r\n", aNode->mId, aActiveTimestamp, aPendingTimestamp, aPanId, aChannel);

    otOperationalDataset aDataset = {};

    if (aActiveTimestamp != 0)
    {
        aDataset.mActiveTimestamp = aActiveTimestamp;
        aDataset.mComponents.mIsActiveTimestampPresent = true;
    }

    if (aPendingTimestamp != 0)
    {
        aDataset.mPendingTimestamp = aPendingTimestamp;
        aDataset.mComponents.mIsPendingTimestampPresent = true;
    }

    if (aDelayTimer != 0)
    {
        aDataset.mDelay = aDelayTimer;
        aDataset.mComponents.mIsDelayPresent = true;
    }

    if (aPanId != 0)
    {
        aDataset.mPanId = aPanId;
        aDataset.mComponents.mIsPanIdPresent = true;
    }

    if (aChannel != 0)
    {
        aDataset.mChannel = aChannel;
        aDataset.mComponents.mIsChannelPresent = true;
    }

    if (aMasterKey != NULL && strlen(aMasterKey) != 0)
    {
        int keyLength;
        if ((keyLength = Hex2Bin(aMasterKey, aDataset.mMasterKey.m8, sizeof(aDataset.mMasterKey))) != OT_MASTER_KEY_SIZE)
        {
            printf("invalid length key %d\r\n", keyLength);
            return OT_ERROR_PARSE;
        }
        aDataset.mComponents.mIsMasterKeyPresent = true;
    }

    if (aMeshLocal != NULL && strlen(aMeshLocal) != 0)
    {
        otIp6Address prefix;
        auto error = otIp6AddressFromString(aMeshLocal, &prefix);
        if (error != OT_ERROR_NONE) return error;
        memcpy(aDataset.mMeshLocalPrefix.m8, prefix.mFields.m8, sizeof(aDataset.mMeshLocalPrefix.m8));
        aDataset.mComponents.mIsMeshLocalPrefixPresent = true;
    }

    if (aNetworkName != NULL && strlen(aNetworkName) != 0)
    {
        strcpy_s(aDataset.mNetworkName.m8, sizeof(aDataset.mNetworkName.m8), aNetworkName);
        aDataset.mComponents.mIsNetworkNamePresent = true;
    }

    auto result = otDatasetSendMgmtPendingSet(aNode->mInstance, &aDataset, nullptr, 0);
    otLogFuncExit();
    return result;
}

OTNODEAPI int32_t OTCALL otNodeSendActiveSet(otNode* aNode, uint64_t aActiveTimestamp, uint16_t aPanId, uint16_t aChannel, uint32_t aChannelMask, const char *aExtPanId, const char *aMasterKey, const char *aMeshLocal, const char *aNetworkName, const char *aBinary)
{
    otLogFuncEntryMsg("[%d] 0x%llX %d %d", aNode->mId, aActiveTimestamp, aPanId, aChannel);
    printf("%d: dataset send active 0x%llX %d %d\r\n", aNode->mId, aActiveTimestamp, aPanId, aChannel);

    otOperationalDataset aDataset = {};
    uint8_t tlvs[128];
    uint8_t tlvsLength = 0;

    if (aActiveTimestamp != 0)
    {
        aDataset.mActiveTimestamp = aActiveTimestamp;
        aDataset.mComponents.mIsActiveTimestampPresent = true;
    }
    if (aPanId != 0)
    {
        aDataset.mPanId = aPanId;
        aDataset.mComponents.mIsPanIdPresent = true;
    }

    if (aChannel != 0)
    {
        aDataset.mChannel = aChannel;
        aDataset.mComponents.mIsChannelPresent = true;
    }

    if (aChannelMask != 0)
    {
        aDataset.mChannelMaskPage0 = aChannelMask;
        aDataset.mComponents.mIsChannelMaskPage0Present = true;
    }

    if (aExtPanId != NULL && strlen(aExtPanId) != 0)
    {
        int keyLength;
        if ((keyLength = Hex2Bin(aExtPanId, aDataset.mExtendedPanId.m8, sizeof(aDataset.mExtendedPanId))) != OT_EXT_PAN_ID_SIZE)
        {
            printf("invalid length ext pan id %d\r\n", keyLength);
            return OT_ERROR_PARSE;
        }
        aDataset.mComponents.mIsExtendedPanIdPresent = true;
    }

    if (aMasterKey != NULL && strlen(aMasterKey) != 0)
    {
        int keyLength;
        if ((keyLength = Hex2Bin(aMasterKey, aDataset.mMasterKey.m8, sizeof(aDataset.mMasterKey))) != OT_MASTER_KEY_SIZE)
        {
            printf("invalid length key %d\r\n", keyLength);
            return OT_ERROR_PARSE;
        }
        aDataset.mComponents.mIsMasterKeyPresent = true;
    }

    if (aMeshLocal != NULL && strlen(aMeshLocal) != 0)
    {
        otIp6Address prefix;
        auto error = otIp6AddressFromString(aMeshLocal, &prefix);
        if (error != OT_ERROR_NONE) return error;
        memcpy(aDataset.mMeshLocalPrefix.m8, prefix.mFields.m8, sizeof(aDataset.mMeshLocalPrefix.m8));
        aDataset.mComponents.mIsMeshLocalPrefixPresent = true;
    }

    if (aNetworkName != NULL && strlen(aNetworkName) != 0)
    {
        strcpy_s(aDataset.mNetworkName.m8, sizeof(aDataset.mNetworkName.m8), aNetworkName);
        aDataset.mComponents.mIsNetworkNamePresent = true;
    }

    if (aBinary != NULL && strlen(aBinary) != 0)
    {
        int length;
        if ((length = Hex2Bin(aBinary,tlvs, sizeof(tlvs))) < 0)
        {
            printf("invalid length tlvs %d\r\n", length);
            return OT_ERROR_PARSE;
        }
        tlvsLength = (uint8_t)length;
    }

    auto result = otDatasetSendMgmtActiveSet(aNode->mInstance, &aDataset, tlvsLength == 0 ? nullptr : tlvs, tlvsLength);
    otLogFuncExit();
    return result;
}

OTNODEAPI int32_t OTCALL otNodeSetMaxChildren(otNode* aNode, uint8_t aMaxChildren)
{
    otLogFuncEntryMsg("[%d] %d", aNode->mId, aMaxChildren);
    printf("%d: childmax %d\r\n", aNode->mId, aMaxChildren);
    auto result = otThreadSetMaxAllowedChildren(aNode->mInstance, aMaxChildren);
    otLogFuncExit();
    return result;
}

typedef struct otMacFrameEntry
{
    otMacFrame  Frame;
    LIST_ENTRY  Link;
} otMacFrameEntry;

typedef struct otListener
{
    HANDLE              mListener;
    CRITICAL_SECTION    mCS;
    HANDLE              mStopEvent;
    HANDLE              mFramesUpdatedEvent;
    LIST_ENTRY          mFrames; // List of otMacFrameEntry
} otListener;

void
otListenerCallback(
    _In_opt_ PVOID aContext,
    _In_ ULONG SourceInterfaceIndex,
    _In_reads_bytes_(FrameLength) PUCHAR FrameBuffer,
    _In_ UCHAR FrameLength,
    _In_ UCHAR Channel
)
{
    otListener* aListener = (otListener*)aContext;
    assert(aListener);

    if (FrameLength)
    {
        otMacFrameEntry* entry = new otMacFrameEntry;
        entry->Frame.buffer[0] = Channel;
        memcpy_s(entry->Frame.buffer + 1, sizeof(entry->Frame.buffer) - 1, FrameBuffer, FrameLength);
        entry->Frame.length = FrameLength + 1;
        entry->Frame.nodeid = (uint32_t)-1;

        // Look up the Node ID from by interface guid
        EnterCriticalSection(&gCS);
        for (uint32_t i = 0; i < gNodes.size(); i++)
        {
            if (gNodes[i]->mInterfaceIndex == SourceInterfaceIndex)
            {
                entry->Frame.nodeid = gNodes[i]->mId;
                break;
            }
        }
        LeaveCriticalSection(&gCS);

        // Push the frame on the list to process
        EnterCriticalSection(&aListener->mCS);
        InsertTailList(&aListener->mFrames, &entry->Link);
        LeaveCriticalSection(&aListener->mCS);

        // Set event indicating we have a new frame to process
        SetEvent(aListener->mFramesUpdatedEvent);
    }
}

OTNODEAPI otListener* OTCALL otListenerInit(uint32_t /* nodeid */)
{
    otLogFuncEntry();

    auto ApiInstance = GetApiInstance();
    if (ApiInstance == nullptr)
    {
        printf("GetApiInstance failed!\r\n");
        otLogFuncExitMsg("GetApiInstance failed");
        return nullptr;
    }

    otListener *listener = new otListener();
    assert(listener);

    InitializeCriticalSection(&listener->mCS);
    listener->mStopEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
    listener->mFramesUpdatedEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    InitializeListHead(&listener->mFrames);

    // Create the listener
    listener->mListener = otvmpListenerCreate(&gTopologyGuid);
    if (listener->mListener == nullptr) goto error;

    // Register for callbacks
    otvmpListenerRegister(listener->mListener, otListenerCallback, listener);

    printf("S: Sniffer started\r\n");

error:

    // Clean up on failure
    if (listener)
    {
        if (listener->mListener == nullptr)
        {
            otListenerFinalize(listener);
        }
    }

    otLogFuncExit();

    return listener;
}

OTNODEAPI int32_t OTCALL otListenerFinalize(otListener* aListener)
{
    otLogFuncEntry();

    if (aListener != nullptr)
    {
        // Set stop event to prevent cancel any pending otListenerRead calls
        SetEvent(aListener->mStopEvent);

        if (aListener->mListener)
        {
            // Unregisters (and waits for callbacks to complete) and cleans up the handle
            otvmpListenerDestroy(aListener->mListener);
            aListener->mListener = nullptr;

            // Clean up left over frames
            PLIST_ENTRY Link = aListener->mFrames.Flink;
            while (Link != &aListener->mFrames)
            {
                otMacFrameEntry *entry = CONTAINING_RECORD(Link, otMacFrameEntry, Link);
                Link = Link->Flink;
                delete entry;
            }

            printf("S: Sniffer stopped\r\n");
        }

        // Clean up everything else
        CloseHandle(aListener->mFramesUpdatedEvent);
        aListener->mFramesUpdatedEvent = nullptr;
        CloseHandle(aListener->mStopEvent);
        aListener->mStopEvent = nullptr;
        DeleteCriticalSection(&aListener->mCS);
        delete aListener;

        ReleaseApiInstance();
    }

    otLogFuncExit();

    return 0;
}

OTNODEAPI int32_t OTCALL otListenerRead(otListener* aListener, otMacFrame *aFrame)
{
    do
    {
        bool exit = false;
        
        EnterCriticalSection(&aListener->mCS);

        // If we have a pending frame, return it now
        if (!IsListEmpty(&aListener->mFrames))
        {
            PLIST_ENTRY Link = RemoveHeadList(&aListener->mFrames);
            otMacFrameEntry *entry = CONTAINING_RECORD(Link, otMacFrameEntry, Link);
            *aFrame = entry->Frame;
            delete entry;
            exit = true;
        }

        LeaveCriticalSection(&aListener->mCS);
        
        if (exit) break;
        
        // Wait for the shutdown or frames updated event
        auto waitResult = WaitForMultipleObjects(2, &aListener->mStopEvent, FALSE, INFINITE);

        if (waitResult == WAIT_OBJECT_0 + 1) // mFramesUpdatedEvent
        {
            continue;
        }
        else // mStopEvent
        {
            return 1;
        }
        
    } while (true);

    //printf("S: Sniffer read %d bytes from node %d\r\n", aFrame->length, aFrame->nodeid);

    return 0;
}
