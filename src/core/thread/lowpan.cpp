/*
 *    Copyright (c) 2016, Nest Labs, Inc.
 *    All rights reserved.
 *
 *    Redistribution and use in source and binary forms, with or without
 *    modification, are permitted provided that the following conditions are met:
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *    3. Neither the name of the copyright holder nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 *    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 *    ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 *    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
 *    DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 *    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 *    ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 *    SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file
 *   This file implements 6LoWPAN header compression.
 */

#include <common/code_utils.hpp>
#include <common/debug.hpp>
#include <common/encoding.hpp>
#include <net/ip6.hpp>
#include <net/udp6.hpp>
#include <thread/lowpan.hpp>
#include <thread/network_data_leader.hpp>
#include <thread/thread_netif.hpp>

using Thread::Encoding::BigEndian::HostSwap16;

namespace Thread {
namespace Lowpan {

Lowpan::Lowpan(ThreadNetif &aThreadNetif):
    mNetworkData(aThreadNetif.GetNetworkDataLeader())
{
}

ThreadError Lowpan::CopyContext(const Context &aContext, Ip6::Address &aAddress)
{
    memcpy(&aAddress, aContext.mPrefix, aContext.mPrefixLength / CHAR_BIT);

    for (int i = (aContext.mPrefixLength & ~7); i < aContext.mPrefixLength; i++)
    {
        aAddress.m8[i / CHAR_BIT] &= ~(0x80 >> (i % CHAR_BIT));
        aAddress.m8[i / CHAR_BIT] |= aContext.mPrefix[i / CHAR_BIT] & (0x80 >> (i % CHAR_BIT));
    }

    return kThreadError_None;
}

ThreadError Lowpan::ComputeIid(const Mac::Address &aMacAddr, const Context &aContext, Ip6::Address &aIpAddress)
{
    switch (aMacAddr.mLength)
    {
    case 2:
        aIpAddress.m16[4] = HostSwap16(0x0000);
        aIpAddress.m16[5] = HostSwap16(0x00ff);
        aIpAddress.m16[6] = HostSwap16(0xfe00);
        aIpAddress.m16[7] = HostSwap16(aMacAddr.mShortAddress);
        break;

    case Ip6::Address::kInterfaceIdentifierSize:
        aIpAddress.SetIid(aMacAddr.mExtAddress);
        break;

    default:
        assert(false);
    }

    if (aContext.mPrefixLength > 64)
    {
        for (int i = (aContext.mPrefixLength & ~7); i < aContext.mPrefixLength; i++)
        {
            aIpAddress.m8[i / CHAR_BIT] &= ~(0x80 >> (i % CHAR_BIT));
            aIpAddress.m8[i / CHAR_BIT] |= aContext.mPrefix[i / CHAR_BIT] & (0x80 >> (i % CHAR_BIT));
        }
    }

    return kThreadError_None;
}

int Lowpan::CompressSourceIid(const Mac::Address &aMacAddr, const Ip6::Address &aIpAddr, const Context &aContext,
                              uint16_t &aHcCtl, uint8_t *aBuf)
{
    uint8_t *cur = aBuf;
    Ip6::Address ipaddr;
    Mac::Address tmp;

    ComputeIid(aMacAddr, aContext, ipaddr);

    if (memcmp(ipaddr.GetIid(), aIpAddr.GetIid(), Ip6::Address::kInterfaceIdentifierSize) == 0)
    {
        aHcCtl |= kHcSrcAddrMode3;
    }
    else
    {
        tmp.mLength = sizeof(tmp.mShortAddress);
        tmp.mShortAddress = HostSwap16(aIpAddr.m16[7]);
        ComputeIid(tmp, aContext, ipaddr);

        if (memcmp(ipaddr.GetIid(), aIpAddr.GetIid(), Ip6::Address::kInterfaceIdentifierSize) == 0)
        {
            aHcCtl |= kHcSrcAddrMode2;
            cur[0] = aIpAddr.m8[14];
            cur[1] = aIpAddr.m8[15];
            cur += 2;
        }
        else
        {
            aHcCtl |= kHcSrcAddrMode1;
            memcpy(cur, aIpAddr.GetIid(), Ip6::Address::kInterfaceIdentifierSize);
            cur += Ip6::Address::kInterfaceIdentifierSize;
        }
    }

    return cur - aBuf;
}

int Lowpan::CompressDestinationIid(const Mac::Address &aMacAddr, const Ip6::Address &aIpAddr, const Context &aContext,
                                   uint16_t &aHcCtl, uint8_t *aBuf)
{
    uint8_t *cur = aBuf;
    Ip6::Address ipaddr;
    Mac::Address tmp;

    ComputeIid(aMacAddr, aContext, ipaddr);

    if (memcmp(ipaddr.GetIid(), aIpAddr.GetIid(), Ip6::Address::kInterfaceIdentifierSize) == 0)
    {
        aHcCtl |= kHcDstAddrMode3;
    }
    else
    {
        tmp.mLength = sizeof(tmp.mShortAddress);
        tmp.mShortAddress = HostSwap16(aIpAddr.m16[7]);
        ComputeIid(tmp, aContext, ipaddr);

        if (memcmp(ipaddr.GetIid(), aIpAddr.GetIid(), Ip6::Address::kInterfaceIdentifierSize) == 0)
        {
            aHcCtl |= kHcDstAddrMode2;
            cur[0] = aIpAddr.m8[14];
            cur[1] = aIpAddr.m8[15];
            cur += 2;
        }
        else
        {
            aHcCtl |= kHcDstAddrMode1;
            memcpy(cur, aIpAddr.GetIid(), Ip6::Address::kInterfaceIdentifierSize);
            cur += Ip6::Address::kInterfaceIdentifierSize;
        }
    }

    return cur - aBuf;
}

int Lowpan::CompressMulticast(const Ip6::Address &aIpAddr, uint16_t &aHcCtl, uint8_t *aBuf)
{
    uint8_t *cur = aBuf;

    aHcCtl |= kHcMulticast;

    for (unsigned int i = 2; i < sizeof(Ip6::Address); i++)
    {
        if (aIpAddr.m8[i])
        {
            if (aIpAddr.m8[1] == 0x02 && i >= 15)
            {
                aHcCtl |= kHcDstAddrMode3;
                cur[0] = aIpAddr.m8[15];
                cur++;
            }
            else if (i >= 13)
            {
                aHcCtl |= kHcDstAddrMode2;
                cur[0] = aIpAddr.m8[1];
                memcpy(cur + 1, aIpAddr.m8 + 13, 3);
                cur += 4;
            }
            else if (i >= 9)
            {
                aHcCtl |= kHcDstAddrMode1;
                cur[0] = aIpAddr.m8[1];
                memcpy(cur + 1, aIpAddr.m8 + 11, 5);
                cur += 6;
            }
            else
            {
                memcpy(cur, aIpAddr.m8, sizeof(Ip6::Address));
                cur += sizeof(Ip6::Address);
            }

            break;
        }
    }

    return cur - aBuf;
}

int Lowpan::Compress(Message &aMessage, const Mac::Address &aMacSource, const Mac::Address &aMacDest, uint8_t *aBuf)
{
    uint8_t *cur = aBuf;
    uint16_t hcCtl = 0;
    Ip6::Header ip6Header;
    uint8_t *ip6HeaderBytes = reinterpret_cast<uint8_t *>(&ip6Header);
    Context srcContext, dstContext;
    bool srcContextValid = true, dstContextValid = true;
    uint8_t nextHeader;

    aMessage.Read(0, sizeof(ip6Header), &ip6Header);

    if (mNetworkData.GetContext(ip6Header.GetSource(), srcContext) != kThreadError_None)
    {
        mNetworkData.GetContext(0, srcContext);
        srcContextValid = false;
    }

    if (mNetworkData.GetContext(ip6Header.GetDestination(), dstContext) != kThreadError_None)
    {
        mNetworkData.GetContext(0, dstContext);
        dstContextValid = false;
    }

    hcCtl = kHcDispatch;

    // Lowpan HC Control Bits
    cur += 2;

    // Context Identifier
    if (srcContext.mContextId != 0 || dstContext.mContextId != 0)
    {
        hcCtl |= kHcContextId;
        cur[0] = (srcContext.mContextId << 4) | dstContext.mContextId;
        cur++;
    }

    // Traffic Class
    if (((ip6HeaderBytes[0] & 0x0f) == 0) && ((ip6HeaderBytes[1] & 0xf0) == 0))
    {
        hcCtl |= kHcTrafficClass;
    }

    // Flow Label
    if (((ip6HeaderBytes[1] & 0x0f) == 0) && ((ip6HeaderBytes[2]) == 0) && ((ip6HeaderBytes[3]) == 0))
    {
        hcCtl |= kHcFlowLabel;
    }

    if ((hcCtl & kHcTrafficFlowMask) != kHcTrafficFlow)
    {
        cur[0] = (ip6HeaderBytes[1] >> 4) << 6;

        if ((hcCtl & kHcTrafficClass) == 0)
        {
            cur[0] |= ((ip6HeaderBytes[0] & 0x0f) << 2) | (ip6HeaderBytes[1] >> 6);
            cur++;
        }

        if ((hcCtl & kHcFlowLabel) == 0)
        {
            cur[0] |= ip6HeaderBytes[1] & 0x0f;
            cur[1] = ip6HeaderBytes[2];
            cur[2] = ip6HeaderBytes[3];
            cur += 3;
        }
    }

    // Next Header
    switch (ip6Header.GetNextHeader())
    {
    case Ip6::kProtoHopOpts:
    case Ip6::kProtoUdp:
        hcCtl |= kHcNextHeader;
        break;

    default:
        cur[0] = ip6Header.GetNextHeader();
        cur++;
        break;
    }

    // Hop Limit
    switch (ip6Header.GetHopLimit())
    {
    case 1:
        hcCtl |= kHcHopLimit1;
        break;

    case 64:
        hcCtl |= kHcHopLimit64;
        break;

    case 255:
        hcCtl |= kHcHopLimit255;
        break;

    default:
        cur[0] = ip6Header.GetHopLimit();
        cur++;
        break;
    }

    // Source Address
    if (ip6Header.GetSource().IsUnspecified())
    {
        hcCtl |= kHcSrcAddrContext;
    }
    else if (ip6Header.GetSource().IsLinkLocal())
    {
        cur += CompressSourceIid(aMacSource, ip6Header.GetSource(), srcContext, hcCtl, cur);
    }
    else if (srcContextValid)
    {
        hcCtl |= kHcSrcAddrContext;
        cur += CompressSourceIid(aMacSource, ip6Header.GetSource(), srcContext, hcCtl, cur);
    }
    else
    {
        memcpy(cur, ip6Header.GetSource().m8, sizeof(ip6Header.GetSource()));
        cur += sizeof(Ip6::Address);
    }

    // Destination Address
    if (ip6Header.GetDestination().IsMulticast())
    {
        cur += CompressMulticast(ip6Header.GetDestination(), hcCtl, cur);
    }
    else if (ip6Header.GetDestination().IsLinkLocal())
    {
        cur += CompressDestinationIid(aMacDest, ip6Header.GetDestination(), dstContext, hcCtl, cur);
    }
    else if (dstContextValid)
    {
        hcCtl |= kHcDstAddrContext;
        cur += CompressDestinationIid(aMacDest, ip6Header.GetDestination(), dstContext, hcCtl, cur);
    }
    else
    {
        memcpy(cur, &ip6Header.GetDestination(), sizeof(ip6Header.GetDestination()));
        cur += sizeof(Ip6::Address);
    }

    aBuf[0] = hcCtl >> 8;
    aBuf[1] = hcCtl;
    aMessage.SetOffset(sizeof(ip6Header));

    nextHeader = ip6Header.GetNextHeader();

    while (1)
    {
        switch (nextHeader)
        {
        case Ip6::kProtoHopOpts:
            cur += CompressExtensionHeader(aMessage, cur, nextHeader);
            break;

        case Ip6::kProtoUdp:
            cur += CompressUdp(aMessage, cur);
            ExitNow();

        default:
            ExitNow();
        }
    }

exit:
    return cur - aBuf;
}

int Lowpan::CompressExtensionHeader(Message &aMessage, uint8_t *aBuf, uint8_t &aNextHeader)
{
    Ip6::ExtensionHeader extHeader;
    uint8_t *cur = aBuf;
    uint8_t len;

    aMessage.Read(aMessage.GetOffset(), sizeof(extHeader), &extHeader);
    aMessage.MoveOffset(sizeof(extHeader));

    cur[0] = kExtHdrDispatch | kExtHdrEidHbh;
    aNextHeader = extHeader.GetNextHeader();

    switch (extHeader.GetNextHeader())
    {
    case Ip6::kProtoUdp:
        cur[0] |= kExtHdrNextHeader;
        break;

    default:
        cur++;
        cur[0] = extHeader.GetNextHeader();
        break;
    }

    cur++;

    len = (extHeader.GetLength() + 1) * 8 - sizeof(extHeader);
    cur[0] = len;
    cur++;

    aMessage.Read(aMessage.GetOffset(), len, cur);
    aMessage.MoveOffset(len);
    cur += len;

    return cur - aBuf;
}

int Lowpan::CompressUdp(Message &aMessage, uint8_t *aBuf)
{
    Ip6::UdpHeader udpHeader;
    uint8_t *cur = aBuf;

    aMessage.Read(aMessage.GetOffset(), sizeof(udpHeader), &udpHeader);

    cur[0] = kUdpDispatch;
    cur++;

    memcpy(cur, &udpHeader, Ip6::UdpHeader::GetLengthOffset());
    cur += Ip6::UdpHeader::GetLengthOffset();
    memcpy(cur, reinterpret_cast<uint8_t *>(&udpHeader) + Ip6::UdpHeader::GetChecksumOffset(), 2);
    cur += 2;

    aMessage.MoveOffset(sizeof(udpHeader));

    return cur - aBuf;
}

ThreadError Lowpan::DispatchToNextHeader(uint8_t aDispatch, Ip6::IpProto &aNextHeader)
{
    ThreadError error = kThreadError_None;

    if ((aDispatch & kExtHdrDispatchMask) == kExtHdrDispatch)
    {
        switch (aDispatch & kExtHdrEidMask)
        {
        case kExtHdrEidHbh:
            aNextHeader = Ip6::kProtoHopOpts;
            ExitNow();

        case kExtHdrEidRouting:
            aNextHeader = Ip6::kProtoRouting;
            ExitNow();

        case kExtHdrEidFragment:
            aNextHeader = Ip6::kProtoFragment;
            ExitNow();

        case kExtHdrEidDst:
            aNextHeader = Ip6::kProtoDstOpts;
            ExitNow();

        case kExtHdrEidIp6:
            aNextHeader = Ip6::kProtoIp6;
            ExitNow();
        }
    }
    else if ((aDispatch & kUdpDispatchMask) == kUdpDispatch)
    {
        aNextHeader = Ip6::kProtoUdp;
        ExitNow();
    }

    error = kThreadError_Parse;

exit:
    return error;
}

int Lowpan::DecompressBaseHeader(Ip6::Header &ip6Header, const Mac::Address &aMacSource, const Mac::Address &aMacDest,
                                 const uint8_t *aBuf)
{
    ThreadError error = kThreadError_None;
    const uint8_t *cur = aBuf;
    uint16_t hcCtl;
    Context srcContext, dstContext;
    bool srcContextValid = true, dstContextValid = true;
    Ip6::IpProto nextHeader;
    uint8_t *bytes;

    hcCtl = (static_cast<uint16_t>(cur[0]) << 8) | cur[1];
    cur += 2;

    // check Dispatch bits
    VerifyOrExit((hcCtl & kHcDispatchMask) == kHcDispatch, error = kThreadError_Parse);

    // Context Identifier
    srcContext.mPrefixLength = 0;
    dstContext.mPrefixLength = 0;

    if ((hcCtl & kHcContextId) != 0)
    {
        if (mNetworkData.GetContext(cur[0] >> 4, srcContext) != kThreadError_None)
        {
            srcContextValid = false;
        }

        if (mNetworkData.GetContext(cur[0] & 0xf, dstContext) != kThreadError_None)
        {
            dstContextValid = false;
        }

        cur++;
    }
    else
    {
        mNetworkData.GetContext(0, srcContext);
        mNetworkData.GetContext(0, dstContext);
    }

    memset(&ip6Header, 0, sizeof(ip6Header));
    ip6Header.Init();

    // Traffic Class and Flow Label
    if ((hcCtl & kHcTrafficFlowMask) != kHcTrafficFlow)
    {
        bytes = reinterpret_cast<uint8_t *>(&ip6Header);
        bytes[1] |= (cur[0] & 0xc0) >> 2;

        if ((hcCtl & kHcTrafficClass) == 0)
        {
            bytes[0] |= (cur[0] >> 2) & 0x0f;
            bytes[1] |= (cur[0] << 6) & 0xc0;
            cur++;
        }

        if ((hcCtl & kHcFlowLabel) == 0)
        {
            bytes[1] |= cur[0] & 0x0f;
            bytes[2] |= cur[1];
            bytes[3] |= cur[2];
            cur += 3;
        }
    }

    // Next Header
    if ((hcCtl & kHcNextHeader) == 0)
    {
        ip6Header.SetNextHeader(static_cast<Ip6::IpProto>(cur[0]));
        cur++;
    }

    // Hop Limit
    switch (hcCtl & kHcHopLimitMask)
    {
    case kHcHopLimit1:
        ip6Header.SetHopLimit(1);
        break;

    case kHcHopLimit64:
        ip6Header.SetHopLimit(64);
        break;

    case kHcHopLimit255:
        ip6Header.SetHopLimit(255);
        break;

    default:
        ip6Header.SetHopLimit(cur[0]);
        cur++;
        break;
    }

    // Source Address
    switch (hcCtl & kHcSrcAddrModeMask)
    {
    case kHcSrcAddrMode0:
        if ((hcCtl & kHcSrcAddrContext) == 0)
        {
            memcpy(&ip6Header.GetSource(), cur, sizeof(ip6Header.GetSource()));
            cur += sizeof(Ip6::Address);
        }

        break;

    case kHcSrcAddrMode1:
        ip6Header.GetSource().SetIid(cur);
        cur += Ip6::Address::kInterfaceIdentifierSize;
        break;

    case kHcSrcAddrMode2:
        ip6Header.GetSource().m8[11] = 0xff;
        ip6Header.GetSource().m8[12] = 0xfe;
        memcpy(ip6Header.GetSource().m8 + 14, cur, 2);
        cur += 2;
        break;

    case kHcSrcAddrMode3:
        ComputeIid(aMacSource, srcContext, ip6Header.GetSource());
        break;
    }

    if ((hcCtl & kHcSrcAddrContext) == 0)
    {
        if ((hcCtl & kHcSrcAddrModeMask) != 0)
        {
            ip6Header.GetSource().m16[0] = HostSwap16(0xfe80);
        }
    }
    else
    {
        VerifyOrExit(srcContextValid, error = kThreadError_Parse);
        CopyContext(srcContext, ip6Header.GetSource());
    }

    if ((hcCtl & kHcMulticast) == 0)
    {
        // Unicast Destination Address

        switch (hcCtl & kHcDstAddrModeMask)
        {
        case kHcDstAddrMode0:
            memcpy(&ip6Header.GetDestination(), cur, sizeof(ip6Header.GetDestination()));
            cur += sizeof(Ip6::Address);
            break;

        case kHcDstAddrMode1:
            ip6Header.GetDestination().SetIid(cur);
            cur += Ip6::Address::kInterfaceIdentifierSize;
            break;

        case kHcDstAddrMode2:
            ip6Header.GetDestination().m8[11] = 0xff;
            ip6Header.GetDestination().m8[12] = 0xfe;
            memcpy(ip6Header.GetDestination().m8 + 14, cur, 2);
            cur += 2;
            break;

        case kHcDstAddrMode3:
            ComputeIid(aMacDest, dstContext, ip6Header.GetDestination());
            break;
        }

        if ((hcCtl & kHcDstAddrContext) == 0)
        {
            if ((hcCtl & kHcDstAddrModeMask) != 0)
            {
                ip6Header.GetDestination().m16[0] = HostSwap16(0xfe80);
            }
        }
        else
        {
            VerifyOrExit(dstContextValid, error = kThreadError_Parse);
            CopyContext(dstContext, ip6Header.GetDestination());
        }
    }
    else
    {
        // Multicast Destination Address

        ip6Header.GetDestination().m8[0] = 0xff;

        if ((hcCtl & kHcDstAddrContext) == 0)
        {
            switch (hcCtl & kHcDstAddrModeMask)
            {
            case kHcDstAddrMode0:
                memcpy(ip6Header.GetDestination().m8, cur, sizeof(Ip6::Address));
                cur += sizeof(Ip6::Address);
                break;

            case kHcDstAddrMode1:
                ip6Header.GetDestination().m8[1] = cur[0];
                memcpy(ip6Header.GetDestination().m8 + 11, cur + 1, 5);
                cur += 6;
                break;

            case kHcDstAddrMode2:
                ip6Header.GetDestination().m8[1] = cur[0];
                memcpy(ip6Header.GetDestination().m8 + 13, cur + 1, 3);
                cur += 4;
                break;

            case kHcDstAddrMode3:
                ip6Header.GetDestination().m8[1] = 0x02;
                ip6Header.GetDestination().m8[15] = cur[0];
                cur++;
                break;
            }
        }
        else
        {
            switch (hcCtl & kHcDstAddrModeMask)
            {
            case 0:
                VerifyOrExit(dstContextValid, error = kThreadError_Parse);
                ip6Header.GetDestination().m8[1] = cur[0];
                ip6Header.GetDestination().m8[2] = cur[1];
                memcpy(ip6Header.GetDestination().m8 + 4, dstContext.mPrefix, 8);
                memcpy(ip6Header.GetDestination().m8 + 12, cur + 2, 4);
                cur += 6;
                break;

            default:
                ExitNow(error = kThreadError_Parse);
            }
        }
    }

    if ((hcCtl & kHcNextHeader) != 0)
    {
        SuccessOrExit(error = DispatchToNextHeader(cur[0], nextHeader));
        ip6Header.SetNextHeader(nextHeader);
    }

exit:
    return (error == kThreadError_None) ? cur - aBuf : -1;
}

int Lowpan::DecompressExtensionHeader(Message &aMessage, const uint8_t *aBuf, uint16_t aBufLength)
{
    const uint8_t *cur = aBuf;
    uint8_t hdr[2];
    uint8_t len;
    Ip6::IpProto nextHeader;
    int rval = -1;
    uint8_t ctl = cur[0];

    cur++;

    // next header
    if (ctl & kExtHdrNextHeader)
    {
        len = cur[0];
        cur++;

        SuccessOrExit(DispatchToNextHeader(cur[len], nextHeader));
        hdr[0] = static_cast<uint8_t>(nextHeader);
    }
    else
    {
        hdr[0] = cur[0];
        cur++;

        len = cur[0];
        cur++;
    }

    // length
    hdr[1] = BitVectorBytes(sizeof(hdr) + len) - 1;

    SuccessOrExit(aMessage.Append(hdr, sizeof(hdr)));
    aMessage.MoveOffset(sizeof(hdr));

    // payload
    SuccessOrExit(aMessage.Append(cur, len));
    aMessage.MoveOffset(len);
    cur += len;

    rval = cur - aBuf;

exit:
    return rval;
}

int Lowpan::DecompressUdpHeader(Message &aMessage, const uint8_t *aBuf, uint16_t aBufLength, uint16_t aDatagramLength)
{
    ThreadError error = kThreadError_None;
    const uint8_t *cur = aBuf;
    Ip6::UdpHeader udpHeader;
    uint8_t udpCtl = cur[0];

    cur++;

    memset(&udpHeader, 0, sizeof(udpHeader));

    // source and dest ports
    switch (udpCtl & kUdpPortMask)
    {
    case 0:
        udpHeader.SetSourcePort((static_cast<uint16_t>(cur[0]) << 8) | cur[1]);
        udpHeader.SetDestinationPort((static_cast<uint16_t>(cur[2]) << 8) | cur[3]);
        cur += 4;
        break;

    case 1:
        udpHeader.SetSourcePort((static_cast<uint16_t>(cur[0]) << 8) | cur[1]);
        udpHeader.SetDestinationPort(0xf000 | cur[2]);
        cur += 3;
        break;

    case 2:
        udpHeader.SetSourcePort(0xf000 | cur[0]);
        udpHeader.SetDestinationPort((static_cast<uint16_t>(cur[2]) << 8) | cur[1]);
        cur += 3;
        break;

    case 3:
        udpHeader.SetSourcePort(0xf000 | cur[0]);
        udpHeader.SetDestinationPort(0xf000 | cur[1]);
        cur += 2;
        break;
    }

    // checksum
    if ((udpCtl & kUdpChecksum) != 0)
    {
        ExitNow(error = kThreadError_Parse);
    }
    else
    {
        udpHeader.SetChecksum((static_cast<uint16_t>(cur[0]) << 8) | cur[1]);
        cur += 2;
    }

    // length
    if (aDatagramLength == 0)
    {
        udpHeader.SetLength(sizeof(udpHeader) + (aBufLength - (cur - aBuf)));
    }
    else
    {
        udpHeader.SetLength(aDatagramLength - aMessage.GetOffset());
    }

    aMessage.Append(&udpHeader, sizeof(udpHeader));
    aMessage.MoveOffset(sizeof(udpHeader));

exit:

    if (error != kThreadError_None)
    {
        return -1;
    }

    return cur - aBuf;
}

int Lowpan::Decompress(Message &aMessage, const Mac::Address &aMacSource, const Mac::Address &aMacDest,
                       const uint8_t *aBuf, uint16_t aBufLen, uint16_t aDatagramLength)
{
    ThreadError error = kThreadError_None;
    Ip6::Header ip6Header;
    const uint8_t *cur = aBuf;
    bool compressed;
    int rval;

    compressed = (((static_cast<uint16_t>(cur[0]) << 8) | cur[1]) & kHcNextHeader) != 0;

    VerifyOrExit((rval = DecompressBaseHeader(ip6Header, aMacSource, aMacDest, aBuf)) >= 0, ;);
    cur += rval;

    SuccessOrExit(error = aMessage.Append(&ip6Header, sizeof(ip6Header)));
    SuccessOrExit(error = aMessage.SetOffset(sizeof(ip6Header)));

    while (compressed)
    {
        if ((cur[0] & kExtHdrDispatchMask) == kExtHdrDispatch)
        {
            compressed = (cur[0] & kExtHdrNextHeader) != 0;
            VerifyOrExit((rval = DecompressExtensionHeader(aMessage, cur, aBufLen - (cur - aBuf))) >= 0,
                         error = kThreadError_Parse);
        }
        else if ((cur[0] & kUdpDispatchMask) == kUdpDispatch)
        {
            compressed = false;
            VerifyOrExit((rval = DecompressUdpHeader(aMessage, cur, aBufLen - (cur - aBuf), aDatagramLength)) >= 0,
                         error = kThreadError_Parse);
        }
        else
        {
            ExitNow(error = kThreadError_Parse);
        }

        cur += rval;
    }

exit:

    if (error != kThreadError_None)
    {
        return -1;
    }

    return cur - aBuf;
}

}  // namespace Lowpan
}  // namespace Thread
