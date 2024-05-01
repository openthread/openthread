/*
 *  Copyright (c) 2024, The OpenThread Authors.
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
 *   This file implements CLI for mDNS.
 */

#include <string.h>

#include "cli_mdns.hpp"

#include <openthread/nat64.h>
#include "cli/cli.hpp"

#if OPENTHREAD_CONFIG_MULTICAST_DNS_ENABLE && OPENTHREAD_CONFIG_MULTICAST_DNS_PUBLIC_API_ENABLE

namespace ot {
namespace Cli {

template <> otError Mdns::Process<Cmd("enable")>(Arg aArgs[])
{
    otError  error;
    uint32_t infraIfIndex;

    SuccessOrExit(error = aArgs[0].ParseAsUint32(infraIfIndex));
    VerifyOrExit(aArgs[1].IsEmpty(), error = OT_ERROR_INVALID_ARGS);

    SuccessOrExit(error = otMdnsSetEnabled(GetInstancePtr(), true, infraIfIndex));

    mInfraIfIndex = infraIfIndex;

exit:
    return error;
}

template <> otError Mdns::Process<Cmd("disable")>(Arg aArgs[])
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(aArgs[0].IsEmpty(), error = OT_ERROR_INVALID_ARGS);
    error = otMdnsSetEnabled(GetInstancePtr(), false, /* aInfraIfIndex */ 0);

exit:
    return error;
}

template <> otError Mdns::Process<Cmd("state")>(Arg aArgs[])
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(aArgs[0].IsEmpty(), error = OT_ERROR_INVALID_ARGS);
    OutputEnabledDisabledStatus(otMdnsIsEnabled(GetInstancePtr()));

exit:
    return error;
}

template <> otError Mdns::Process<Cmd("unicastquestion")>(Arg aArgs[])
{
    return ProcessEnableDisable(aArgs, otMdnsIsQuestionUnicastAllowed, otMdnsSetQuestionUnicastAllowed);
}

void Mdns::OutputHost(const otMdnsHost &aHost)
{
    OutputLine("Host %s", aHost.mHostName);
    OutputLine(kIndentSize, "%u address:", aHost.mAddressesLength);

    for (uint16_t index = 0; index < aHost.mAddressesLength; index++)
    {
        OutputFormat(kIndentSize, "  ");
        OutputIp6AddressLine(aHost.mAddresses[index]);
    }

    OutputLine(kIndentSize, "ttl: %lu", ToUlong(aHost.mTtl));
}

void Mdns::OutputService(const otMdnsService &aService)
{
    OutputLine("Service %s for %s", aService.mServiceInstance, aService.mServiceType);
    OutputLine(kIndentSize, "host: %s", aService.mHostName);

    if (aService.mSubTypeLabelsLength > 0)
    {
        OutputLine(kIndentSize, "%u sub-type:", aService.mSubTypeLabelsLength);

        for (uint16_t index = 0; index < aService.mSubTypeLabelsLength; index++)
        {
            OutputLine(kIndentSize * 2, "%s", aService.mSubTypeLabels[index]);
        }
    }

    OutputLine(kIndentSize, "port: %u", aService.mPort);
    OutputLine(kIndentSize, "priority: %u", aService.mPriority);
    OutputLine(kIndentSize, "weight: %u", aService.mWeight);
    OutputLine(kIndentSize, "ttl: %lu", ToUlong(aService.mTtl));

    if ((aService.mTxtData == nullptr) || (aService.mTxtDataLength == 0))
    {
        OutputLine(kIndentSize, "txt-data: (empty)");
    }
    else
    {
        OutputFormat(kIndentSize, "txt-data: ");
        OutputBytesLine(aService.mTxtData, aService.mTxtDataLength);
    }
}

void Mdns::OutputKey(const otMdnsKey &aKey)
{
    if (aKey.mServiceType != nullptr)
    {
        OutputLine("Key %s for %s (service)", aKey.mName, aKey.mServiceType);
    }
    else
    {
        OutputLine("Key %s (host)", aKey.mName);
    }

    OutputFormat(kIndentSize, "key-data: ");
    OutputBytesLine(aKey.mKeyData, aKey.mKeyDataLength);

    OutputLine(kIndentSize, "ttl: %lu", ToUlong(aKey.mTtl));
}

void Mdns::OutputState(otMdnsEntryState aState)
{
    const char *stateString = "";

    switch (aState)
    {
    case OT_MDNS_ENTRY_STATE_PROBING:
        stateString = "probing";
        break;
    case OT_MDNS_ENTRY_STATE_REGISTERED:
        stateString = "registered";
        break;
    case OT_MDNS_ENTRY_STATE_CONFLICT:
        stateString = "conflict";
        break;
    case OT_MDNS_ENTRY_STATE_REMOVING:
        stateString = "removing";
        break;
    }

    OutputLine(kIndentSize, "state: %s", stateString);
}

void Mdns::OutputCacheInfo(const otMdnsCacheInfo &aInfo)
{
    OutputLine(kIndentSize, "active: %s", aInfo.mIsActive ? "yes" : "no");
    OutputLine(kIndentSize, "cached-results: %s", aInfo.mHasCachedResults ? "yes" : "no");
}

template <> otError Mdns::Process<Cmd("register")>(Arg aArgs[])
{
    // mdns [async] [host|service|key] <entry specific args>

    otError error   = OT_ERROR_NONE;
    bool    isAsync = false;

    if (aArgs[0] == "async")
    {
        isAsync = true;
        aArgs++;
    }

    if (aArgs[0] == "host")
    {
        SuccessOrExit(error = ProcessRegisterHost(aArgs + 1));
    }
    else if (aArgs[0] == "service")
    {
        SuccessOrExit(error = ProcessRegisterService(aArgs + 1));
    }
    else if (aArgs[0] == "key")
    {
        SuccessOrExit(error = ProcessRegisterKey(aArgs + 1));
    }
    else
    {
        ExitNow(error = OT_ERROR_INVALID_ARGS);
    }

    if (isAsync)
    {
        OutputLine("mDNS request id: %lu", ToUlong(mRequestId));
    }
    else
    {
        error               = OT_ERROR_PENDING;
        mWaitingForCallback = true;
    }

exit:
    return error;
}

otError Mdns::ProcessRegisterHost(Arg aArgs[])
{
    // register host <name> [<zero or more addresses>] [<ttl>]

    otError      error = OT_ERROR_NONE;
    otMdnsHost   host;
    otIp6Address addresses[kMaxAddresses];

    memset(&host, 0, sizeof(host));

    VerifyOrExit(!aArgs->IsEmpty(), error = OT_ERROR_INVALID_ARGS);
    host.mHostName = aArgs->GetCString();
    aArgs++;

    host.mAddresses = addresses;

    for (; !aArgs->IsEmpty(); aArgs++)
    {
        otIp6Address address;
        uint32_t     ttl;

        if (aArgs->ParseAsIp6Address(address) == OT_ERROR_NONE)
        {
            VerifyOrExit(host.mAddressesLength < kMaxAddresses, error = OT_ERROR_NO_BUFS);
            addresses[host.mAddressesLength] = address;
            host.mAddressesLength++;
        }
        else if (aArgs->ParseAsUint32(ttl) == OT_ERROR_NONE)
        {
            host.mTtl = ttl;
            VerifyOrExit(aArgs[1].IsEmpty(), error = OT_ERROR_INVALID_ARGS);
        }
        else
        {
            ExitNow(error = OT_ERROR_INVALID_ARGS);
        }
    }

    OutputHost(host);

    mRequestId++;
    error = otMdnsRegisterHost(GetInstancePtr(), &host, mRequestId, HandleRegisterationDone);

exit:
    return error;
}

otError Mdns::ProcessRegisterService(Arg aArgs[])
{
    otError       error;
    otMdnsService service;
    Buffers       buffers;

    SuccessOrExit(error = ParseServiceArgs(aArgs, service, buffers));

    OutputService(service);

    mRequestId++;
    error = otMdnsRegisterService(GetInstancePtr(), &service, mRequestId, HandleRegisterationDone);

exit:
    return error;
}

otError Mdns::ParseServiceArgs(Arg aArgs[], otMdnsService &aService, Buffers &aBuffers)
{
    // mdns register service <instance-label> <service-type,sub_types> <host-name> <port> [<prio>] [<weight>] [<ttl>]
    // [<txtdata>]

    otError  error = OT_ERROR_INVALID_ARGS;
    char    *label;
    uint16_t len;

    memset(&aService, 0, sizeof(aService));

    VerifyOrExit(!aArgs->IsEmpty());
    aService.mServiceInstance = aArgs->GetCString();
    aArgs++;

    // Copy service type into `aBuffer.mString`, then search for
    // `,` in the string to parse the list of sub-types (if any).

    VerifyOrExit(!aArgs->IsEmpty());
    len = aArgs->GetLength();
    VerifyOrExit(len + 1 < kStringSize, error = OT_ERROR_NO_BUFS);
    memcpy(aBuffers.mString, aArgs->GetCString(), len + 1);

    aService.mServiceType   = aBuffers.mString;
    aService.mSubTypeLabels = aBuffers.mSubTypeLabels;

    label = strchr(aBuffers.mString, ',');

    if (label != nullptr)
    {
        while (true)
        {
            *label++ = '\0';

            VerifyOrExit(aService.mSubTypeLabelsLength < kMaxSubTypes, error = OT_ERROR_NO_BUFS);
            aBuffers.mSubTypeLabels[aService.mSubTypeLabelsLength] = label;
            aService.mSubTypeLabelsLength++;

            label = strchr(label, ',');

            if (label == nullptr)
            {
                break;
            }
        }
    }

    aArgs++;
    VerifyOrExit(!aArgs->IsEmpty());
    aService.mHostName = aArgs->GetCString();

    aArgs++;
    SuccessOrExit(aArgs->ParseAsUint16(aService.mPort));

    // The rest of `Args` are optional.

    error = OT_ERROR_NONE;

    aArgs++;
    VerifyOrExit(!aArgs->IsEmpty());
    SuccessOrExit(error = aArgs->ParseAsUint16(aService.mPriority));

    aArgs++;
    VerifyOrExit(!aArgs->IsEmpty());
    SuccessOrExit(error = aArgs->ParseAsUint16(aService.mWeight));

    aArgs++;
    VerifyOrExit(!aArgs->IsEmpty());
    SuccessOrExit(error = aArgs->ParseAsUint32(aService.mTtl));

    aArgs++;
    VerifyOrExit(!aArgs->IsEmpty());
    len = kMaxTxtDataSize;
    SuccessOrExit(error = aArgs->ParseAsHexString(len, aBuffers.mTxtData));
    aService.mTxtData       = aBuffers.mTxtData;
    aService.mTxtDataLength = len;

    aArgs++;
    VerifyOrExit(aArgs->IsEmpty(), error = OT_ERROR_INVALID_ARGS);

exit:
    return error;
}

otError Mdns::ProcessRegisterKey(Arg aArgs[])
{
    otError   error = OT_ERROR_INVALID_ARGS;
    otMdnsKey key;
    uint16_t  len;
    uint8_t   data[kMaxKeyDataSize];

    memset(&key, 0, sizeof(key));

    VerifyOrExit(!aArgs->IsEmpty());
    key.mName = aArgs->GetCString();

    aArgs++;
    VerifyOrExit(!aArgs->IsEmpty());

    if (aArgs->GetCString()[0] == '_')
    {
        key.mServiceType = aArgs->GetCString();
        aArgs++;
        VerifyOrExit(!aArgs->IsEmpty());
    }

    len = kMaxKeyDataSize;
    SuccessOrExit(error = aArgs->ParseAsHexString(len, data));

    key.mKeyData       = data;
    key.mKeyDataLength = len;

    // ttl is optional

    aArgs++;

    if (!aArgs->IsEmpty())
    {
        SuccessOrExit(error = aArgs->ParseAsUint32(key.mTtl));
        aArgs++;
        VerifyOrExit(aArgs->IsEmpty(), error = kErrorInvalidArgs);
    }

    OutputKey(key);

    mRequestId++;
    error = otMdnsRegisterKey(GetInstancePtr(), &key, mRequestId, HandleRegisterationDone);

exit:
    return error;
}

void Mdns::HandleRegisterationDone(otInstance *aInstance, otMdnsRequestId aRequestId, otError aError)
{
    OT_UNUSED_VARIABLE(aInstance);

    Interpreter::GetInterpreter().mMdns.HandleRegisterationDone(aRequestId, aError);
}

void Mdns::HandleRegisterationDone(otMdnsRequestId aRequestId, otError aError)
{
    if (mWaitingForCallback && (aRequestId == mRequestId))
    {
        mWaitingForCallback = false;
        Interpreter::GetInterpreter().OutputResult(aError);
    }
    else
    {
        OutputLine("mDNS registration for request id %lu outcome: %s", ToUlong(aRequestId),
                   otThreadErrorToString(aError));
    }
}

template <> otError Mdns::Process<Cmd("unregister")>(Arg aArgs[])
{
    otError error = OT_ERROR_INVALID_ARGS;

    if (aArgs[0] == "host")
    {
        otMdnsHost host;

        memset(&host, 0, sizeof(host));
        VerifyOrExit(!aArgs[1].IsEmpty());
        host.mHostName = aArgs[1].GetCString();
        VerifyOrExit(aArgs[2].IsEmpty());

        error = otMdnsUnregisterHost(GetInstancePtr(), &host);
    }
    else if (aArgs[0] == "service")
    {
        otMdnsService service;

        memset(&service, 0, sizeof(service));
        VerifyOrExit(!aArgs[1].IsEmpty());
        service.mServiceInstance = aArgs[1].GetCString();
        VerifyOrExit(!aArgs[2].IsEmpty());
        service.mServiceType = aArgs[2].GetCString();
        VerifyOrExit(aArgs[3].IsEmpty());

        error = otMdnsUnregisterService(GetInstancePtr(), &service);
    }
    else if (aArgs[0] == "key")
    {
        otMdnsKey key;

        memset(&key, 0, sizeof(key));
        VerifyOrExit(!aArgs[1].IsEmpty());
        key.mName = aArgs[1].GetCString();

        if (!aArgs[2].IsEmpty())
        {
            key.mServiceType = aArgs[2].GetCString();
            VerifyOrExit(aArgs[3].IsEmpty());
        }

        error = otMdnsUnregisterKey(GetInstancePtr(), &key);
    }

exit:
    return error;
}

#if OPENTHREAD_CONFIG_MULTICAST_DNS_ENTRY_ITERATION_API_ENABLE

template <> otError Mdns::Process<Cmd("hosts")>(Arg aArgs[])
{
    otError          error    = OT_ERROR_NONE;
    otMdnsIterator  *iterator = nullptr;
    otMdnsHost       host;
    otMdnsEntryState state;

    VerifyOrExit(aArgs[0].IsEmpty(), error = OT_ERROR_INVALID_ARGS);

    iterator = otMdnsAllocateIterator(GetInstancePtr());
    VerifyOrExit(iterator != nullptr, error = OT_ERROR_NO_BUFS);

    while (true)
    {
        error = otMdnsGetNextHost(GetInstancePtr(), iterator, &host, &state);

        if (error == OT_ERROR_NOT_FOUND)
        {
            error = OT_ERROR_NONE;
            ExitNow();
        }

        SuccessOrExit(error);

        OutputHost(host);
        OutputState(state);
    }

exit:
    if (iterator != nullptr)
    {
        otMdnsFreeIterator(GetInstancePtr(), iterator);
    }

    return error;
}

template <> otError Mdns::Process<Cmd("services")>(Arg aArgs[])
{
    otError          error    = OT_ERROR_NONE;
    otMdnsIterator  *iterator = nullptr;
    otMdnsService    service;
    otMdnsEntryState state;

    VerifyOrExit(aArgs[0].IsEmpty(), error = OT_ERROR_INVALID_ARGS);

    iterator = otMdnsAllocateIterator(GetInstancePtr());
    VerifyOrExit(iterator != nullptr, error = OT_ERROR_NO_BUFS);

    while (true)
    {
        error = otMdnsGetNextService(GetInstancePtr(), iterator, &service, &state);

        if (error == OT_ERROR_NOT_FOUND)
        {
            error = OT_ERROR_NONE;
            ExitNow();
        }

        SuccessOrExit(error);

        OutputService(service);
        OutputState(state);
    }

exit:
    if (iterator != nullptr)
    {
        otMdnsFreeIterator(GetInstancePtr(), iterator);
    }

    return error;
}

template <> otError Mdns::Process<Cmd("keys")>(Arg aArgs[])
{
    otError          error    = OT_ERROR_NONE;
    otMdnsIterator  *iterator = nullptr;
    otMdnsKey        key;
    otMdnsEntryState state;

    VerifyOrExit(aArgs[0].IsEmpty(), error = OT_ERROR_INVALID_ARGS);

    iterator = otMdnsAllocateIterator(GetInstancePtr());
    VerifyOrExit(iterator != nullptr, error = OT_ERROR_NO_BUFS);

    while (true)
    {
        error = otMdnsGetNextKey(GetInstancePtr(), iterator, &key, &state);

        if (error == OT_ERROR_NOT_FOUND)
        {
            error = OT_ERROR_NONE;
            ExitNow();
        }

        SuccessOrExit(error);

        OutputKey(key);
        OutputState(state);
    }

exit:
    if (iterator != nullptr)
    {
        otMdnsFreeIterator(GetInstancePtr(), iterator);
    }

    return error;
}

#endif // OPENTHREAD_CONFIG_MULTICAST_DNS_ENTRY_ITERATION_API_ENABLE

otError Mdns::ParseStartOrStop(const Arg &aArg, bool &aIsStart)
{
    otError error = OT_ERROR_NONE;

    if (aArg == "start")
    {
        aIsStart = true;
    }
    else if (aArg == "stop")
    {
        aIsStart = false;
    }
    else
    {
        error = OT_ERROR_INVALID_ARGS;
    }

    return error;
}

template <> otError Mdns::Process<Cmd("browser")>(Arg aArgs[])
{
    // mdns browser start|stop <service-type> [<sub-type>]

    otError       error;
    otMdnsBrowser browser;
    bool          isStart;

    ClearAllBytes(browser);

    SuccessOrExit(error = ParseStartOrStop(aArgs[0], isStart));
    VerifyOrExit(!aArgs[1].IsEmpty(), error = OT_ERROR_INVALID_ARGS);

    browser.mServiceType = aArgs[1].GetCString();

    if (!aArgs[2].IsEmpty())
    {
        browser.mSubTypeLabel = aArgs[2].GetCString();
        VerifyOrExit(aArgs[3].IsEmpty(), error = OT_ERROR_INVALID_ARGS);
    }

    browser.mInfraIfIndex = mInfraIfIndex;
    browser.mCallback     = HandleBrowseResult;

    if (isStart)
    {
        error = otMdnsStartBrowser(GetInstancePtr(), &browser);
    }
    else
    {
        error = otMdnsStopBrowser(GetInstancePtr(), &browser);
    }

exit:
    return error;
}

void Mdns::HandleBrowseResult(otInstance *aInstance, const otMdnsBrowseResult *aResult)
{
    OT_UNUSED_VARIABLE(aInstance);

    Interpreter::GetInterpreter().mMdns.HandleBrowseResult(*aResult);
}

void Mdns::HandleBrowseResult(const otMdnsBrowseResult &aResult)
{
    OutputFormat("mDNS browse result for %s", aResult.mServiceType);

    if (aResult.mSubTypeLabel)
    {
        OutputLine(" sub-type %s", aResult.mSubTypeLabel);
    }
    else
    {
        OutputNewLine();
    }

    OutputLine(kIndentSize, "instance: %s", aResult.mServiceInstance);
    OutputLine(kIndentSize, "ttl: %lu", ToUlong(aResult.mTtl));
    OutputLine(kIndentSize, "if-index: %lu", ToUlong(aResult.mInfraIfIndex));
}

template <> otError Mdns::Process<Cmd("srvresolver")>(Arg aArgs[])
{
    // mdns srvresolver start|stop <service-instance> <service-type>

    otError           error;
    otMdnsSrvResolver resolver;
    bool              isStart;

    ClearAllBytes(resolver);

    SuccessOrExit(error = ParseStartOrStop(aArgs[0], isStart));
    VerifyOrExit(!aArgs[2].IsEmpty(), error = OT_ERROR_INVALID_ARGS);

    resolver.mServiceInstance = aArgs[1].GetCString();
    resolver.mServiceType     = aArgs[2].GetCString();
    resolver.mInfraIfIndex    = mInfraIfIndex;
    resolver.mCallback        = HandleSrvResult;

    if (isStart)
    {
        error = otMdnsStartSrvResolver(GetInstancePtr(), &resolver);
    }
    else
    {
        error = otMdnsStopSrvResolver(GetInstancePtr(), &resolver);
    }

exit:
    return error;
}

void Mdns::HandleSrvResult(otInstance *aInstance, const otMdnsSrvResult *aResult)
{
    OT_UNUSED_VARIABLE(aInstance);

    Interpreter::GetInterpreter().mMdns.HandleSrvResult(*aResult);
}

void Mdns::HandleSrvResult(const otMdnsSrvResult &aResult)
{
    OutputLine("mDNS SRV result for %s for %s", aResult.mServiceInstance, aResult.mServiceType);

    if (aResult.mTtl != 0)
    {
        OutputLine(kIndentSize, "host: %s", aResult.mHostName);
        OutputLine(kIndentSize, "port: %u", aResult.mPort);
        OutputLine(kIndentSize, "priority: %u", aResult.mPriority);
        OutputLine(kIndentSize, "weight: %u", aResult.mWeight);
    }

    OutputLine(kIndentSize, "ttl: %lu", ToUlong(aResult.mTtl));
    OutputLine(kIndentSize, "if-index: %lu", ToUlong(aResult.mInfraIfIndex));
}

template <> otError Mdns::Process<Cmd("txtresolver")>(Arg aArgs[])
{
    // mdns txtresolver start|stop <service-instance> <service-type>

    otError           error;
    otMdnsTxtResolver resolver;
    bool              isStart;

    ClearAllBytes(resolver);

    SuccessOrExit(error = ParseStartOrStop(aArgs[0], isStart));
    VerifyOrExit(!aArgs[2].IsEmpty(), error = OT_ERROR_INVALID_ARGS);

    resolver.mServiceInstance = aArgs[1].GetCString();
    resolver.mServiceType     = aArgs[2].GetCString();
    resolver.mInfraIfIndex    = mInfraIfIndex;
    resolver.mCallback        = HandleTxtResult;

    if (isStart)
    {
        error = otMdnsStartTxtResolver(GetInstancePtr(), &resolver);
    }
    else
    {
        error = otMdnsStopTxtResolver(GetInstancePtr(), &resolver);
    }

exit:
    return error;
}

void Mdns::HandleTxtResult(otInstance *aInstance, const otMdnsTxtResult *aResult)
{
    OT_UNUSED_VARIABLE(aInstance);

    Interpreter::GetInterpreter().mMdns.HandleTxtResult(*aResult);
}

void Mdns::HandleTxtResult(const otMdnsTxtResult &aResult)
{
    OutputLine("mDNS TXT result for %s for %s", aResult.mServiceInstance, aResult.mServiceType);

    if (aResult.mTtl != 0)
    {
        OutputFormat(kIndentSize, "txt-data: ");
        OutputBytesLine(aResult.mTxtData, aResult.mTxtDataLength);
    }

    OutputLine(kIndentSize, "ttl: %lu", ToUlong(aResult.mTtl));
    OutputLine(kIndentSize, "if-index: %lu", ToUlong(aResult.mInfraIfIndex));
}
template <> otError Mdns::Process<Cmd("ip6resolver")>(Arg aArgs[])
{
    // mdns ip6resolver start|stop <host-name>

    otError               error;
    otMdnsAddressResolver resolver;
    bool                  isStart;

    ClearAllBytes(resolver);

    SuccessOrExit(error = ParseStartOrStop(aArgs[0], isStart));
    VerifyOrExit(!aArgs[1].IsEmpty(), error = OT_ERROR_INVALID_ARGS);

    resolver.mHostName     = aArgs[1].GetCString();
    resolver.mInfraIfIndex = mInfraIfIndex;
    resolver.mCallback     = HandleIp6AddressResult;

    if (isStart)
    {
        error = otMdnsStartIp6AddressResolver(GetInstancePtr(), &resolver);
    }
    else
    {
        error = otMdnsStopIp6AddressResolver(GetInstancePtr(), &resolver);
    }

exit:
    return error;
}

void Mdns::HandleIp6AddressResult(otInstance *aInstance, const otMdnsAddressResult *aResult)
{
    OT_UNUSED_VARIABLE(aInstance);

    Interpreter::GetInterpreter().mMdns.HandleAddressResult(*aResult, kIp6Address);
}

void Mdns::HandleAddressResult(const otMdnsAddressResult &aResult, IpAddressType aType)
{
    OutputLine("mDNS %s address result for %s", aType == kIp6Address ? "IPv6" : "IPv4", aResult.mHostName);

    OutputLine(kIndentSize, "%u address:", aResult.mAddressesLength);

    for (uint16_t index = 0; index < aResult.mAddressesLength; index++)
    {
        OutputFormat(kIndentSize, "  ");
        OutputIp6Address(aResult.mAddresses[index].mAddress);
        OutputLine(" ttl:%lu", ToUlong(aResult.mAddresses[index].mTtl));
    }

    OutputLine(kIndentSize, "if-index: %lu", ToUlong(aResult.mInfraIfIndex));
}

template <> otError Mdns::Process<Cmd("ip4resolver")>(Arg aArgs[])
{
    // mdns ip4resolver start|stop <host-name>

    otError               error;
    otMdnsAddressResolver resolver;
    bool                  isStart;

    ClearAllBytes(resolver);

    SuccessOrExit(error = ParseStartOrStop(aArgs[0], isStart));
    VerifyOrExit(!aArgs[1].IsEmpty(), error = OT_ERROR_INVALID_ARGS);

    resolver.mHostName     = aArgs[1].GetCString();
    resolver.mInfraIfIndex = mInfraIfIndex;
    resolver.mCallback     = HandleIp4AddressResult;

    if (isStart)
    {
        error = otMdnsStartIp4AddressResolver(GetInstancePtr(), &resolver);
    }
    else
    {
        error = otMdnsStopIp4AddressResolver(GetInstancePtr(), &resolver);
    }

exit:
    return error;
}

void Mdns::HandleIp4AddressResult(otInstance *aInstance, const otMdnsAddressResult *aResult)
{
    OT_UNUSED_VARIABLE(aInstance);

    Interpreter::GetInterpreter().mMdns.HandleAddressResult(*aResult, kIp4Address);
}

#if OPENTHREAD_CONFIG_MULTICAST_DNS_ENTRY_ITERATION_API_ENABLE

template <> otError Mdns::Process<Cmd("browsers")>(Arg aArgs[])
{
    // mdns browsers

    otError         error;
    otMdnsIterator *iterator = nullptr;
    otMdnsCacheInfo info;
    otMdnsBrowser   browser;

    VerifyOrExit(aArgs[0].IsEmpty(), error = OT_ERROR_INVALID_ARGS);

    iterator = otMdnsAllocateIterator(GetInstancePtr());
    VerifyOrExit(iterator != nullptr, error = OT_ERROR_NO_BUFS);

    while (true)
    {
        error = otMdnsGetNextBrowser(GetInstancePtr(), iterator, &browser, &info);

        if (error == OT_ERROR_NOT_FOUND)
        {
            error = OT_ERROR_NONE;
            ExitNow();
        }

        SuccessOrExit(error);

        OutputFormat("Browser %s", browser.mServiceType);

        if (browser.mSubTypeLabel != nullptr)
        {
            OutputFormat(" for sub-type %s", browser.mSubTypeLabel);
        }

        OutputNewLine();
        OutputCacheInfo(info);
    }

exit:
    if (iterator != nullptr)
    {
        otMdnsFreeIterator(GetInstancePtr(), iterator);
    }

    return error;
}

template <> otError Mdns::Process<Cmd("srvresolvers")>(Arg aArgs[])
{
    // mdns srvresolvers

    otError           error;
    otMdnsIterator   *iterator = nullptr;
    otMdnsCacheInfo   info;
    otMdnsSrvResolver resolver;

    VerifyOrExit(aArgs[0].IsEmpty(), error = OT_ERROR_INVALID_ARGS);

    iterator = otMdnsAllocateIterator(GetInstancePtr());
    VerifyOrExit(iterator != nullptr, error = OT_ERROR_NO_BUFS);

    while (true)
    {
        error = otMdnsGetNextSrvResolver(GetInstancePtr(), iterator, &resolver, &info);

        if (error == OT_ERROR_NOT_FOUND)
        {
            error = OT_ERROR_NONE;
            ExitNow();
        }

        SuccessOrExit(error);

        OutputLine("SRV resolver %s for %s", resolver.mServiceInstance, resolver.mServiceType);
        OutputCacheInfo(info);
    }

exit:
    if (iterator != nullptr)
    {
        otMdnsFreeIterator(GetInstancePtr(), iterator);
    }

    return error;
}

template <> otError Mdns::Process<Cmd("txtresolvers")>(Arg aArgs[])
{
    // mdns txtresolvers

    otError           error;
    otMdnsIterator   *iterator = nullptr;
    otMdnsCacheInfo   info;
    otMdnsTxtResolver resolver;

    VerifyOrExit(aArgs[0].IsEmpty(), error = OT_ERROR_INVALID_ARGS);

    iterator = otMdnsAllocateIterator(GetInstancePtr());
    VerifyOrExit(iterator != nullptr, error = OT_ERROR_NO_BUFS);

    while (true)
    {
        error = otMdnsGetNextTxtResolver(GetInstancePtr(), iterator, &resolver, &info);

        if (error == OT_ERROR_NOT_FOUND)
        {
            error = OT_ERROR_NONE;
            ExitNow();
        }

        SuccessOrExit(error);

        OutputLine("TXT resolver %s for %s", resolver.mServiceInstance, resolver.mServiceType);
        OutputCacheInfo(info);
    }

exit:
    if (iterator != nullptr)
    {
        otMdnsFreeIterator(GetInstancePtr(), iterator);
    }

    return error;
}

template <> otError Mdns::Process<Cmd("ip6resolvers")>(Arg aArgs[])
{
    // mdns ip6resolvers

    otError               error;
    otMdnsIterator       *iterator = nullptr;
    otMdnsCacheInfo       info;
    otMdnsAddressResolver resolver;

    VerifyOrExit(aArgs[0].IsEmpty(), error = OT_ERROR_INVALID_ARGS);

    iterator = otMdnsAllocateIterator(GetInstancePtr());
    VerifyOrExit(iterator != nullptr, error = OT_ERROR_NO_BUFS);

    while (true)
    {
        error = otMdnsGetNextIp6AddressResolver(GetInstancePtr(), iterator, &resolver, &info);

        if (error == OT_ERROR_NOT_FOUND)
        {
            error = OT_ERROR_NONE;
            ExitNow();
        }

        SuccessOrExit(error);

        OutputLine("IPv6 address resolver %s", resolver.mHostName);
        OutputCacheInfo(info);
    }

exit:
    if (iterator != nullptr)
    {
        otMdnsFreeIterator(GetInstancePtr(), iterator);
    }

    return error;
}

template <> otError Mdns::Process<Cmd("ip4resolvers")>(Arg aArgs[])
{
    // mdns ip4resolvers

    otError               error;
    otMdnsIterator       *iterator = nullptr;
    otMdnsCacheInfo       info;
    otMdnsAddressResolver resolver;

    VerifyOrExit(aArgs[0].IsEmpty(), error = OT_ERROR_INVALID_ARGS);

    iterator = otMdnsAllocateIterator(GetInstancePtr());
    VerifyOrExit(iterator != nullptr, error = OT_ERROR_NO_BUFS);

    while (true)
    {
        error = otMdnsGetNextIp4AddressResolver(GetInstancePtr(), iterator, &resolver, &info);

        if (error == OT_ERROR_NOT_FOUND)
        {
            error = OT_ERROR_NONE;
            ExitNow();
        }

        SuccessOrExit(error);

        OutputLine("IPv4 address resolver %s", resolver.mHostName);
        OutputCacheInfo(info);
    }

exit:
    if (iterator != nullptr)
    {
        otMdnsFreeIterator(GetInstancePtr(), iterator);
    }

    return error;
}

#endif // OPENTHREAD_CONFIG_MULTICAST_DNS_ENTRY_ITERATION_API_ENABLE

otError Mdns::Process(Arg aArgs[])
{
#define CmdEntry(aCommandString)                            \
    {                                                       \
        aCommandString, &Mdns::Process<Cmd(aCommandString)> \
    }

    static constexpr Command kCommands[] = {
        CmdEntry("browser"),
#if OPENTHREAD_CONFIG_MULTICAST_DNS_ENTRY_ITERATION_API_ENABLE
        CmdEntry("browsers"),
#endif
        CmdEntry("disable"),
        CmdEntry("enable"),
#if OPENTHREAD_CONFIG_MULTICAST_DNS_ENTRY_ITERATION_API_ENABLE
        CmdEntry("hosts"),
#endif
        CmdEntry("ip4resolver"),
#if OPENTHREAD_CONFIG_MULTICAST_DNS_ENTRY_ITERATION_API_ENABLE
        CmdEntry("ip4resolvers"),
#endif
        CmdEntry("ip6resolver"),
#if OPENTHREAD_CONFIG_MULTICAST_DNS_ENTRY_ITERATION_API_ENABLE
        CmdEntry("ip6resolvers"),
        CmdEntry("keys"),
#endif
        CmdEntry("register"),
#if OPENTHREAD_CONFIG_MULTICAST_DNS_ENTRY_ITERATION_API_ENABLE
        CmdEntry("services"),
#endif
        CmdEntry("srvresolver"),
#if OPENTHREAD_CONFIG_MULTICAST_DNS_ENTRY_ITERATION_API_ENABLE
        CmdEntry("srvresolvers"),
#endif
        CmdEntry("state"),
        CmdEntry("txtresolver"),
#if OPENTHREAD_CONFIG_MULTICAST_DNS_ENTRY_ITERATION_API_ENABLE
        CmdEntry("txtresolvers"),
#endif
        CmdEntry("unicastquestion"),
        CmdEntry("unregister"),
    };

#undef CmdEntry

    static_assert(BinarySearch::IsSorted(kCommands), "kCommands is not sorted");

    otError        error = OT_ERROR_INVALID_COMMAND;
    const Command *command;

    if (aArgs[0].IsEmpty() || (aArgs[0] == "help"))
    {
        OutputCommandTable(kCommands);
        ExitNow(error = aArgs[0].IsEmpty() ? error : OT_ERROR_NONE);
    }

    command = BinarySearch::Find(aArgs[0].GetCString(), kCommands);
    VerifyOrExit(command != nullptr);

    error = (this->*command->mHandler)(aArgs + 1);

exit:
    return error;
}

} // namespace Cli
} // namespace ot

#endif // OPENTHREAD_CONFIG_MULTICAST_DNS_ENABLE && OPENTHREAD_CONFIG_MULTICAST_DNS_PUBLIC_API_ENABLE
