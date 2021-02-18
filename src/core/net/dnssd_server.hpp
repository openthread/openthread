/*
 *  Copyright (c) 2021, The OpenThread Authors.
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

#ifndef DNS_SERVER_HPP_
#define DNS_SERVER_HPP_

#include "openthread-core-config.h"

#if OPENTHREAD_CONFIG_DNSSD_SERVER_ENABLE

#include <openthread/dns.h>

#include "common/message.hpp"
#include "common/non_copyable.hpp"
#include "common/timer.hpp"
#include "net/dns_headers.hpp"
#include "net/ip6.hpp"
#include "net/netif.hpp"
#include "net/srp_server.hpp"

/**
 * @file
 *   This file includes definitions for the DNS-SD server.
 */

namespace ot {
namespace Dns {
namespace ServiceDiscovery {

/**
 * This class implements DNS-SD server.
 *
 */
class Server : public InstanceLocator, private NonCopyable
{
public:
    /**
     * This constructor initializes the object.
     *
     * @param[in]  aInstance     A reference to the OpenThread instance.
     *
     */
    explicit Server(Instance &aInstance);

    /**
     * This method starts the DNS-SD server.
     *
     * @retval OT_ERROR_NONE     Successfully started the DNS-SD server.
     * @retval OT_ERROR_FAILED   If failed to open or bind the UDP socket.
     *
     */
    otError Start(void);

    /**
     * This method stops the DNS-SD server.
     *
     */
    void Stop(void);

private:
    enum
    {
        kPort                = OPENTHREAD_CONFIG_DNSSD_SERVER_PORT,
        kProtocolLabelLength = 4,
    };

    class NameCompressInfo : public Clearable<NameCompressInfo>
    {
    public:
        enum : uint16_t
        {
            kUnknownOffset = 0, // Unknown offset value (used when offset is not yet set).
        };

        explicit NameCompressInfo(void) { Clear(); }

        uint16_t GetDomainNameOffset(void) const
        {
            OT_ASSERT(mDomainNameOffset != kUnknownOffset);

            return mDomainNameOffset;
        }

        void SetDomainNameOffset(uint16_t aOffset, const char *aName)
        {
            OT_ASSERT(mDomainName == nullptr);

            mDomainName       = aName;
            mDomainNameOffset = aOffset;
        }

        const char *GetDomainName(void) const { return mDomainName; }

        uint16_t GetServiceNameOffset(void) const
        {
            OT_ASSERT(mServiceNameOffset != kUnknownOffset);

            return mServiceNameOffset;
        };

        void SetServiceNameOffset(uint16_t aOffset, const char *aName)
        {
            OT_ASSERT(mServiceName == nullptr);

            mServiceName       = aName;
            mServiceNameOffset = aOffset;
        }

        const char *GetServiceName() const { return mServiceName; }

        uint16_t GetInstanceNameOffset(const char *aName) const
        {
            uint16_t offset = mInstanceNameOffset;

            if (offset != kUnknownOffset && strcmp(aName, mInstanceName) != 0)
            {
                offset = kUnknownOffset;
            }

            return offset;
        }

        void SetInstanceNameOffset(uint16_t aOffset, const char *aName)
        {
            if (mInstanceName == nullptr)
            {
                mInstanceName       = aName;
                mInstanceNameOffset = aOffset;
            }
        }

        uint16_t GetHostNameOffset(const char *aName) const
        {
            uint16_t offset = mHostNameOffset;

            if (offset != kUnknownOffset && strcmp(aName, mHostName) != 0)
            {
                offset = kUnknownOffset;
            }

            return offset;
        }

        void SetHostNameOffset(uint16_t aOffset, const char *aName)
        {
            if (mHostName == nullptr)
            {
                mHostName       = aName;
                mHostNameOffset = aOffset;
            }
        }

    private:
        const char *mDomainName;         // The serialized domain name (must NOT be nullptr)
        const char *mServiceName;        // The serialized service name (must NOT be nullptr for PTR/SRV/TXT queries)
        const char *mInstanceName;       // The serialized instance name or nullptr (only support one instance name)
        const char *mHostName;           // The serialized host name or nullptr (only support one host name)
        uint16_t    mDomainNameOffset;   // Offset of domain name serialization into the response message.
        uint16_t    mServiceNameOffset;  // Offset of service name serialization into the response message.
        uint16_t    mInstanceNameOffset; // Offset of instance name serialization into the response message.
        uint16_t    mHostNameOffset;     // Offset of host name serialization into the response message.
    };

    // This structure represents the splitting information of a full name.
    struct NameComponentsOffsetInfo
    {
        enum : uint8_t
        {
            kNotPresent = 0xff, // Indicates the component is not present.
        };

        explicit NameComponentsOffsetInfo(void)
            : mDomainOffset(kNotPresent)
            , mProtocolOffset(kNotPresent)
            , mServiceOffset(kNotPresent)
            , mInstanceOffset(kNotPresent)
        {
        }

        bool IsServiceInstanceName(void) const { return mInstanceOffset != kNotPresent; }

        bool IsServiceName(void) const { return mServiceOffset != kNotPresent && mInstanceOffset == kNotPresent; }

        bool IsHostName(void) const { return mProtocolOffset == kNotPresent && mDomainOffset != 0; }

        uint8_t mDomainOffset;   // The offset to the beginning of <Domain>.
        uint8_t mProtocolOffset; // The offset to the beginning of <Protocol> (i.e. _tcp or _udp) or `kNotPresent` if
                                 // the name is not a service or instance.
        uint8_t mServiceOffset;  // The offset to the beginning of <Service> or `kNotPresent` if the name is not a
                                 // service or instance.
        uint8_t mInstanceOffset; // The offset to the beginning of <Instance> or `kNotPresent` if the name is not a
                                 // instance.
    };

    bool           IsRunning(void) const { return mSocket.IsBound(); }
    static void    HandleUdpReceive(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo);
    void           HandleUdpReceive(Message &aMessage, const Ip6::MessageInfo &aMessageInfo);
    void           ProcessQuery(Message &aMessage, Message &aResponse, const Header &aRequestHeader);
    static otError AddQuestionToResponse(const char *    aName,
                                         const Question &aQuestion,
                                         Message &       aResponse,
                                         Header &        aResponseHeader);
    otError        PrepareCompressInfo(uint16_t                  aQueryType,
                                       const char *              aName,
                                       uint16_t                  aNameSerializeOffset,
                                       Server::NameCompressInfo &aCompressInfo);
    otError        ResolveQuestion(const char *      aName,
                                   const Question &  aQuestion,
                                   Header &          aResponseHeader,
                                   Message &         aResponseMessage,
                                   NameCompressInfo &aCompressInfo);
    static otError AppendPtrRecord(Message &         aMessage,
                                   const char *      aServiceName,
                                   const char *      aInstanceName,
                                   uint32_t          aTtl,
                                   NameCompressInfo &aCompressInfo);
    static otError AppendSrvRecord(Message &         aMessage,
                                   const char *      aInstanceName,
                                   const char *      aHostName,
                                   uint32_t          aTtl,
                                   uint16_t          aPriority,
                                   uint16_t          aWeight,
                                   uint16_t          aPort,
                                   NameCompressInfo &aCompressInfo);
    static otError AppendAaaaRecord(Message &           aMessage,
                                    const char *        aHostName,
                                    const Ip6::Address &aAddress,
                                    uint32_t            aTtl,
                                    NameCompressInfo &  aCompressInfo);
    static otError AppendServiceName(Message &aMessage, const char *aName, NameCompressInfo &aCompressInfo);
    static otError AppendInstanceName(Message &aMessage, const char *aName, NameCompressInfo &aCompressInfo);
    static otError AppendHostName(Message &aMessage, const char *aName, NameCompressInfo &aCompressInfo);
    static void    IncResourceRecordCount(Header &aHeader, bool aAdditional);
    static otError FindNameComponents(const char *aName, const char *aDomain, NameComponentsOffsetInfo &aInfo);
    static otError FindPreviousLabel(const char *aName, uint8_t &aStart, uint8_t &aStop);

#if OPENTHREAD_CONFIG_SRP_SERVER_ENABLE
    otError                     ResolveQuestionBySrp(const char *      aName,
                                                     const Question &  aQuestion,
                                                     Header &          aResponseHeader,
                                                     Message &         aResponseMessage,
                                                     bool              aAdditional,
                                                     NameCompressInfo &aCompressInfo);
    const Srp::Server::Host *   GetNextSrpHost(const Srp::Server::Host *aHost);
    const Srp::Server::Service *GetNextSrpService(const Srp::Server::Host &aHost, const Srp::Server::Service *aService);
    static otError              AppendTxtRecord(Message &                   aMessage,
                                                const char *                aInstanceName,
                                                const Srp::Server::Service &aService,
                                                uint32_t                    aTtl,
                                                NameCompressInfo &          aCompressInfo);
#endif

    static const char kDnssdProtocolUdp[4];
    static const char kDnssdProtocolTcp[4];
    static const char kDefaultDomainName[];

    Ip6::Udp::Socket mSocket;
};

} // namespace ServiceDiscovery
} // namespace Dns
} // namespace ot

#endif // OPENTHREAD_CONFIG_DNSSD_SERVER_ENABLE

#endif // DNS_SERVER_HPP_
