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
 *   This file implements 6LoWPAN header compression.
 */

#include "lowpan.hpp"

#include "common/code_utils.hpp"
#include "common/debug.hpp"
#include "common/encoding.hpp"
#include "common/instance.hpp"
#include "common/locator-getters.hpp"
#include "net/ip6.hpp"
#include "net/udp6.hpp"
#include "thread/network_data_leader.hpp"
#include "thread/thread_netif.hpp"

using ot::Encoding::BigEndian::HostSwap16;
using ot::Encoding::BigEndian::ReadUint16;

namespace ot {
namespace Lowpan {

Lowpan::Lowpan(Instance &aInstance)
    : InstanceLocator(aInstance)
{
}

void Lowpan::CopyContext(const Context &aContext, Ip6::Address &aAddress)
{
    memcpy(&aAddress, aContext.mPrefix, aContext.mPrefixLength / CHAR_BIT);

    for (int i = (aContext.mPrefixLength & ~7); i < aContext.mPrefixLength; i++)
    {
        aAddress.mFields.m8[i / CHAR_BIT] &= ~(0x80 >> (i % CHAR_BIT));
        aAddress.mFields.m8[i / CHAR_BIT] |= aContext.mPrefix[i / CHAR_BIT] & (0x80 >> (i % CHAR_BIT));
    }
}

otError Lowpan::ComputeIid(const Mac::Address &aMacAddr, const Context &aContext, Ip6::Address &aIpAddress)
{
    otError error = OT_ERROR_NONE;

    switch (aMacAddr.GetType())
    {
    case Mac::Address::kTypeShort:
        aIpAddress.mFields.m16[4] = HostSwap16(0x0000);
        aIpAddress.mFields.m16[5] = HostSwap16(0x00ff);
        aIpAddress.mFields.m16[6] = HostSwap16(0xfe00);
        aIpAddress.mFields.m16[7] = HostSwap16(aMacAddr.GetShort());
        break;

    case Mac::Address::kTypeExtended:
        aIpAddress.SetIid(aMacAddr.GetExtended());
        break;

    default:
        ExitNow(error = OT_ERROR_PARSE);
    }

    if (aContext.mPrefixLength > 64)
    {
        for (int i = (aContext.mPrefixLength & ~7); i < aContext.mPrefixLength; i++)
        {
            aIpAddress.mFields.m8[i / CHAR_BIT] &= ~(0x80 >> (i % CHAR_BIT));
            aIpAddress.mFields.m8[i / CHAR_BIT] |= aContext.mPrefix[i / CHAR_BIT] & (0x80 >> (i % CHAR_BIT));
        }
    }

exit:
    return error;
}

otError Lowpan::CompressSourceIid(const Mac::Address &aMacAddr,
                                  const Ip6::Address &aIpAddr,
                                  const Context &     aContext,
                                  uint16_t &          aHcCtl,
                                  BufferWriter &      aBuf)
{
    otError      error = OT_ERROR_NONE;
    BufferWriter buf   = aBuf;
    Ip6::Address ipaddr;
    Mac::Address tmp;

    ComputeIid(aMacAddr, aContext, ipaddr);

    if (memcmp(ipaddr.GetIid(), aIpAddr.GetIid(), Ip6::Address::kInterfaceIdentifierSize) == 0)
    {
        aHcCtl |= kHcSrcAddrMode3;
    }
    else
    {
        tmp.SetShort(HostSwap16(aIpAddr.mFields.m16[7]));
        ComputeIid(tmp, aContext, ipaddr);

        if (memcmp(ipaddr.GetIid(), aIpAddr.GetIid(), Ip6::Address::kInterfaceIdentifierSize) == 0)
        {
            aHcCtl |= kHcSrcAddrMode2;
            SuccessOrExit(error = buf.Write(aIpAddr.mFields.m8 + 14, 2));
        }
        else
        {
            aHcCtl |= kHcSrcAddrMode1;
            SuccessOrExit(error = buf.Write(aIpAddr.GetIid(), Ip6::Address::kInterfaceIdentifierSize));
        }
    }

exit:
    if (error == OT_ERROR_NONE)
    {
        aBuf = buf;
    }

    return error;
}

otError Lowpan::CompressDestinationIid(const Mac::Address &aMacAddr,
                                       const Ip6::Address &aIpAddr,
                                       const Context &     aContext,
                                       uint16_t &          aHcCtl,
                                       BufferWriter &      aBuf)
{
    otError      error = OT_ERROR_NONE;
    BufferWriter buf   = aBuf;
    Ip6::Address ipaddr;
    Mac::Address tmp;

    ComputeIid(aMacAddr, aContext, ipaddr);

    if (memcmp(ipaddr.GetIid(), aIpAddr.GetIid(), Ip6::Address::kInterfaceIdentifierSize) == 0)
    {
        aHcCtl |= kHcDstAddrMode3;
    }
    else
    {
        tmp.SetShort(HostSwap16(aIpAddr.mFields.m16[7]));
        ComputeIid(tmp, aContext, ipaddr);

        if (memcmp(ipaddr.GetIid(), aIpAddr.GetIid(), Ip6::Address::kInterfaceIdentifierSize) == 0)
        {
            aHcCtl |= kHcDstAddrMode2;
            SuccessOrExit(error = buf.Write(aIpAddr.mFields.m8 + 14, 2));
        }
        else
        {
            aHcCtl |= kHcDstAddrMode1;
            SuccessOrExit(error = buf.Write(aIpAddr.GetIid(), Ip6::Address::kInterfaceIdentifierSize));
        }
    }

exit:
    if (error == OT_ERROR_NONE)
    {
        aBuf = buf;
    }

    return error;
}

otError Lowpan::CompressMulticast(const Ip6::Address &aIpAddr, uint16_t &aHcCtl, BufferWriter &aBuf)
{
    otError      error = OT_ERROR_NONE;
    BufferWriter buf   = aBuf;
    Context      multicastContext;

    aHcCtl |= kHcMulticast;

    for (unsigned int i = 2; i < sizeof(Ip6::Address); i++)
    {
        if (aIpAddr.mFields.m8[i])
        {
            // Check if multicast address can be compressed to 8-bits (ff02::00xx)
            if (aIpAddr.mFields.m8[1] == 0x02 && i >= 15)
            {
                aHcCtl |= kHcDstAddrMode3;
                SuccessOrExit(error = buf.Write(aIpAddr.mFields.m8[15]));
            }
            // Check if multicast address can be compressed to 32-bits (ffxx::00xx:xxxx)
            else if (i >= 13)
            {
                aHcCtl |= kHcDstAddrMode2;
                SuccessOrExit(error = buf.Write(aIpAddr.mFields.m8[1]));
                SuccessOrExit(error = buf.Write(aIpAddr.mFields.m8 + 13, 3));
            }
            // Check if multicast address can be compressed to 48-bits (ffxx::00xx:xxxx:xxxx)
            else if (i >= 11)
            {
                aHcCtl |= kHcDstAddrMode1;
                SuccessOrExit(error = buf.Write(aIpAddr.mFields.m8[1]));
                SuccessOrExit(error = buf.Write(aIpAddr.mFields.m8 + 11, 5));
            }
            else
            {
                // Check if multicast address can be compressed using Context ID 0.
                if (Get<NetworkData::Leader>().GetContext(0, multicastContext) == OT_ERROR_NONE &&
                    multicastContext.mPrefixLength == aIpAddr.mFields.m8[3] &&
                    memcmp(multicastContext.mPrefix, aIpAddr.mFields.m8 + 4, 8) == 0)
                {
                    aHcCtl |= kHcDstAddrContext | kHcDstAddrMode0;
                    SuccessOrExit(error = buf.Write(aIpAddr.mFields.m8 + 1, 2));
                    SuccessOrExit(error = buf.Write(aIpAddr.mFields.m8 + 12, 4));
                }
                else
                {
                    SuccessOrExit(error = buf.Write(aIpAddr.mFields.m8, sizeof(Ip6::Address)));
                }
            }

            break;
        }
    }

exit:
    if (error == OT_ERROR_NONE)
    {
        aBuf = buf;
    }

    return error;
}

otError Lowpan::Compress(Message &           aMessage,
                         const Mac::Address &aMacSource,
                         const Mac::Address &aMacDest,
                         BufferWriter &      aBuf)
{
    otError              error       = OT_ERROR_NONE;
    NetworkData::Leader &networkData = Get<NetworkData::Leader>();
    uint16_t             startOffset = aMessage.GetOffset();
    BufferWriter         buf         = aBuf;
    uint16_t             hcCtl;
    Ip6::Header          ip6Header;
    uint8_t *            ip6HeaderBytes = reinterpret_cast<uint8_t *>(&ip6Header);
    Context              srcContext, dstContext;
    bool                 srcContextValid, dstContextValid;
    uint8_t              nextHeader;
    uint8_t              ecn;
    uint8_t              dscp;
    uint8_t              headerDepth;
    uint8_t              headerMaxDepth = 0xff;

compress:

    headerDepth = 0;
    hcCtl       = kHcDispatch;

    VerifyOrExit(aMessage.Read(aMessage.GetOffset(), sizeof(ip6Header), &ip6Header) == sizeof(ip6Header),
                 error = OT_ERROR_PARSE);

    srcContextValid =
        (networkData.GetContext(ip6Header.GetSource(), srcContext) == OT_ERROR_NONE && srcContext.mCompressFlag);

    if (!srcContextValid)
    {
        networkData.GetContext(0, srcContext);
    }

    dstContextValid =
        (networkData.GetContext(ip6Header.GetDestination(), dstContext) == OT_ERROR_NONE && dstContext.mCompressFlag);

    if (!dstContextValid)
    {
        networkData.GetContext(0, dstContext);
    }

    // Lowpan HC Control Bits
    SuccessOrExit(error = buf.Advance(sizeof(hcCtl)));

    // Context Identifier
    if (srcContext.mContextId != 0 || dstContext.mContextId != 0)
    {
        hcCtl |= kHcContextId;
        SuccessOrExit(error = buf.Write(((srcContext.mContextId << 4) | dstContext.mContextId) & 0xff));
    }

    dscp = ((ip6HeaderBytes[0] << 2) & 0x3c) | (ip6HeaderBytes[1] >> 6);
    ecn  = (ip6HeaderBytes[1] << 2) & 0xc0;

    // Flow Label
    if (((ip6HeaderBytes[1] & 0x0f) == 0) && ((ip6HeaderBytes[2]) == 0) && ((ip6HeaderBytes[3]) == 0))
    {
        if (dscp == 0 && ecn == 0)
        {
            // Elide Flow Label and Traffic Class.
            hcCtl |= kHcTrafficClass | kHcFlowLabel;
        }
        else
        {
            // Elide Flow Label and carry Traffic Class in-line.
            hcCtl |= kHcFlowLabel;

            SuccessOrExit(error = buf.Write(ecn | dscp));
        }
    }
    else if (dscp == 0)
    {
        // Carry Flow Label and ECN only with 2-bit padding.
        hcCtl |= kHcTrafficClass;

        SuccessOrExit(error = buf.Write(ecn | (ip6HeaderBytes[1] & 0x0f)));
        SuccessOrExit(error = buf.Write(ip6HeaderBytes + 2, 2));
    }
    else
    {
        // Carry Flow Label and Traffic Class in-line.
        SuccessOrExit(error = buf.Write(ecn | dscp));
        SuccessOrExit(error = buf.Write(ip6HeaderBytes[1] & 0x0f));
        SuccessOrExit(error = buf.Write(ip6HeaderBytes + 2, 2));
    }

    // Next Header
    switch (ip6Header.GetNextHeader())
    {
    case Ip6::kProtoHopOpts:
    case Ip6::kProtoUdp:
    case Ip6::kProtoIp6:
        if (headerDepth + 1 < headerMaxDepth)
        {
            hcCtl |= kHcNextHeader;
            break;
        }
        // fall through

    default:
        SuccessOrExit(error = buf.Write(static_cast<uint8_t>(ip6Header.GetNextHeader())));
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
        SuccessOrExit(error = buf.Write(ip6Header.GetHopLimit()));
        break;
    }

    // Source Address
    if (ip6Header.GetSource().IsUnspecified())
    {
        hcCtl |= kHcSrcAddrContext;
    }
    else if (ip6Header.GetSource().IsLinkLocal())
    {
        SuccessOrExit(error = CompressSourceIid(aMacSource, ip6Header.GetSource(), srcContext, hcCtl, buf));
    }
    else if (srcContextValid)
    {
        hcCtl |= kHcSrcAddrContext;
        SuccessOrExit(error = CompressSourceIid(aMacSource, ip6Header.GetSource(), srcContext, hcCtl, buf));
    }
    else
    {
        SuccessOrExit(error = buf.Write(ip6Header.GetSource().mFields.m8, sizeof(ip6Header.GetSource())));
    }

    // Destination Address
    if (ip6Header.GetDestination().IsMulticast())
    {
        SuccessOrExit(error = CompressMulticast(ip6Header.GetDestination(), hcCtl, buf));
    }
    else if (ip6Header.GetDestination().IsLinkLocal())
    {
        SuccessOrExit(error = CompressDestinationIid(aMacDest, ip6Header.GetDestination(), dstContext, hcCtl, buf));
    }
    else if (dstContextValid)
    {
        hcCtl |= kHcDstAddrContext;
        SuccessOrExit(error = CompressDestinationIid(aMacDest, ip6Header.GetDestination(), dstContext, hcCtl, buf));
    }
    else
    {
        SuccessOrExit(error = buf.Write(&ip6Header.GetDestination(), sizeof(ip6Header.GetDestination())));
    }

    headerDepth++;

    aMessage.MoveOffset(sizeof(ip6Header));

    nextHeader = static_cast<uint8_t>(ip6Header.GetNextHeader());

    while (headerDepth < headerMaxDepth)
    {
        switch (nextHeader)
        {
        case Ip6::kProtoHopOpts:
            SuccessOrExit(error = CompressExtensionHeader(aMessage, buf, nextHeader));
            break;

        case Ip6::kProtoUdp:
            error = CompressUdp(aMessage, buf);
            ExitNow();

        case Ip6::kProtoIp6:
            // For IP-in-IP the NH bit of the LOWPAN_NHC encoding MUST be set to zero.
            SuccessOrExit(error = buf.Write(kExtHdrDispatch | kExtHdrEidIp6));

            error = Compress(aMessage, aMacSource, aMacDest, buf);

            // fall through

        default:
            ExitNow();
        }

        headerDepth++;
    }

exit:
    if (error == OT_ERROR_NONE)
    {
        IgnoreReturnValue(aBuf.Write(hcCtl >> 8));
        IgnoreReturnValue(aBuf.Write(hcCtl & 0xff));
        aBuf = buf;
    }
    else
    {
        aMessage.SetOffset(startOffset);

        if (headerDepth > 0)
        {
            buf            = aBuf;
            headerMaxDepth = headerDepth;
            goto compress;
        }
    }

    return error;
}

otError Lowpan::CompressExtensionHeader(Message &aMessage, BufferWriter &aBuf, uint8_t &aNextHeader)
{
    otError              error       = OT_ERROR_NONE;
    BufferWriter         buf         = aBuf;
    uint16_t             startOffset = aMessage.GetOffset();
    Ip6::ExtensionHeader extHeader;
    Ip6::OptionHeader    optionHeader;
    uint8_t              len;
    uint8_t              padLength = 0;
    uint16_t             offset;
    uint8_t              tmpByte;

    VerifyOrExit(aMessage.Read(aMessage.GetOffset(), sizeof(extHeader), &extHeader) == sizeof(extHeader),
                 error = OT_ERROR_PARSE);
    aMessage.MoveOffset(sizeof(extHeader));

    tmpByte = kExtHdrDispatch | kExtHdrEidHbh;

    switch (extHeader.GetNextHeader())
    {
    case Ip6::kProtoUdp:
    case Ip6::kProtoIp6:
        tmpByte |= kExtHdrNextHeader;
        break;

    default:
        SuccessOrExit(error = buf.Write(tmpByte));
        tmpByte = static_cast<uint8_t>(extHeader.GetNextHeader());
        break;
    }

    SuccessOrExit(error = buf.Write(tmpByte));

    len = (extHeader.GetLength() + 1) * 8 - sizeof(extHeader);

    // RFC 6282 says: "IPv6 Hop-by-Hop and Destination Options Headers may use a trailing
    // Pad1 or PadN to achieve 8-octet alignment. When there is a single trailing Pad1 or PadN
    // option of 7 octets or less and the containing header is a multiple of 8 octets, the trailing
    // Pad1 or PadN option MAY be elided by the compressor."
    if (aNextHeader == Ip6::kProtoHopOpts || aNextHeader == Ip6::kProtoDstOpts)
    {
        offset = aMessage.GetOffset();

        while (offset < len + aMessage.GetOffset())
        {
            VerifyOrExit(aMessage.Read(offset, sizeof(optionHeader), &optionHeader) == sizeof(optionHeader),
                         error = OT_ERROR_PARSE);

            if (optionHeader.GetType() == Ip6::OptionPad1::kType)
            {
                offset += sizeof(Ip6::OptionPad1);
            }
            else
            {
                offset += sizeof(optionHeader) + optionHeader.GetLength();
            }
        }

        // Check if the last option can be compressed.
        if (optionHeader.GetType() == Ip6::OptionPad1::kType)
        {
            padLength = sizeof(Ip6::OptionPad1);
        }
        else if (optionHeader.GetType() == Ip6::OptionPadN::kType)
        {
            padLength = sizeof(optionHeader) + optionHeader.GetLength();
        }

        len -= padLength;
    }

    VerifyOrExit(aMessage.GetOffset() + len + padLength <= aMessage.GetLength(), error = OT_ERROR_PARSE);

    aNextHeader = static_cast<uint8_t>(extHeader.GetNextHeader());

    SuccessOrExit(error = buf.Write(len));
    SuccessOrExit(error = buf.Write(aMessage, len));
    aMessage.MoveOffset(len + padLength);

exit:
    if (error == OT_ERROR_NONE)
    {
        aBuf = buf;
    }
    else
    {
        aMessage.SetOffset(startOffset);
    }

    return error;
}

otError Lowpan::CompressUdp(Message &aMessage, BufferWriter &aBuf)
{
    otError        error       = OT_ERROR_NONE;
    BufferWriter   buf         = aBuf;
    uint16_t       startOffset = aMessage.GetOffset();
    Ip6::UdpHeader udpHeader;
    uint16_t       source;
    uint16_t       destination;

    VerifyOrExit(aMessage.Read(aMessage.GetOffset(), sizeof(udpHeader), &udpHeader) == sizeof(udpHeader),
                 error = OT_ERROR_PARSE);

    source      = udpHeader.GetSourcePort();
    destination = udpHeader.GetDestinationPort();

    if ((source & 0xfff0) == 0xf0b0 && (destination & 0xfff0) == 0xf0b0)
    {
        SuccessOrExit(error = buf.Write(kUdpDispatch | 3));
        SuccessOrExit(error = buf.Write((((source & 0xf) << 4) | (destination & 0xf)) & 0xff));
    }
    else if ((source & 0xff00) == 0xf000)
    {
        SuccessOrExit(error = buf.Write(kUdpDispatch | 2));
        SuccessOrExit(error = buf.Write(source & 0xff));
        SuccessOrExit(error = buf.Write(destination >> 8));
        SuccessOrExit(error = buf.Write(destination & 0xff));
    }
    else if ((destination & 0xff00) == 0xf000)
    {
        SuccessOrExit(error = buf.Write(kUdpDispatch | 1));
        SuccessOrExit(error = buf.Write(source >> 8));
        SuccessOrExit(error = buf.Write(source & 0xff));
        SuccessOrExit(error = buf.Write(destination & 0xff));
    }
    else
    {
        SuccessOrExit(error = buf.Write(kUdpDispatch));
        SuccessOrExit(error = buf.Write(&udpHeader, Ip6::UdpHeader::GetLengthOffset()));
    }

    SuccessOrExit(error = buf.Write(reinterpret_cast<uint8_t *>(&udpHeader) + Ip6::UdpHeader::GetChecksumOffset(), 2));

    aMessage.MoveOffset(sizeof(udpHeader));

exit:
    if (error == OT_ERROR_NONE)
    {
        aBuf = buf;
    }
    else
    {
        aMessage.SetOffset(startOffset);
    }

    return error;
}

otError Lowpan::DispatchToNextHeader(uint8_t aDispatch, uint8_t &aNextHeader)
{
    otError error = OT_ERROR_NONE;

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

    error = OT_ERROR_PARSE;

exit:
    return error;
}

int Lowpan::DecompressBaseHeader(Ip6::Header &       aIp6Header,
                                 bool &              aCompressedNextHeader,
                                 const Mac::Address &aMacSource,
                                 const Mac::Address &aMacDest,
                                 const uint8_t *     aBuf,
                                 uint16_t            aBufLength)
{
    NetworkData::Leader &networkData = Get<NetworkData::Leader>();
    otError              error       = OT_ERROR_PARSE;
    const uint8_t *      cur         = aBuf;
    uint16_t             remaining   = aBufLength;
    uint16_t             hcCtl;
    Context              srcContext, dstContext;
    bool                 srcContextValid = true, dstContextValid = true;
    uint8_t              nextHeader;
    uint8_t *            bytes;

    VerifyOrExit(remaining >= 2);
    hcCtl = ReadUint16(cur);
    cur += 2;
    remaining -= 2;

    // check Dispatch bits
    VerifyOrExit((hcCtl & kHcDispatchMask) == kHcDispatch);

    // Context Identifier
    srcContext.mPrefixLength = 0;
    dstContext.mPrefixLength = 0;

    if ((hcCtl & kHcContextId) != 0)
    {
        VerifyOrExit(remaining >= 1);

        if (networkData.GetContext(cur[0] >> 4, srcContext) != OT_ERROR_NONE)
        {
            srcContextValid = false;
        }

        if (networkData.GetContext(cur[0] & 0xf, dstContext) != OT_ERROR_NONE)
        {
            dstContextValid = false;
        }

        cur++;
        remaining--;
    }
    else
    {
        networkData.GetContext(0, srcContext);
        networkData.GetContext(0, dstContext);
    }

    memset(&aIp6Header, 0, sizeof(aIp6Header));
    aIp6Header.Init();

    // Traffic Class and Flow Label
    if ((hcCtl & kHcTrafficFlowMask) != kHcTrafficFlow)
    {
        VerifyOrExit(remaining >= 1);

        bytes = reinterpret_cast<uint8_t *>(&aIp6Header);
        bytes[1] |= (cur[0] & 0xc0) >> 2;

        if ((hcCtl & kHcTrafficClass) == 0)
        {
            bytes[0] |= (cur[0] >> 2) & 0x0f;
            bytes[1] |= (cur[0] << 6) & 0xc0;
            cur++;
            remaining--;
        }

        if ((hcCtl & kHcFlowLabel) == 0)
        {
            VerifyOrExit(remaining >= 3);
            bytes[1] |= cur[0] & 0x0f;
            bytes[2] |= cur[1];
            bytes[3] |= cur[2];
            cur += 3;
            remaining -= 3;
        }
    }

    // Next Header
    if ((hcCtl & kHcNextHeader) == 0)
    {
        VerifyOrExit(remaining >= 1);
        aIp6Header.SetNextHeader(cur[0]);
        cur++;
        remaining--;
        aCompressedNextHeader = false;
    }
    else
    {
        aCompressedNextHeader = true;
    }

    // Hop Limit
    switch (hcCtl & kHcHopLimitMask)
    {
    case kHcHopLimit1:
        aIp6Header.SetHopLimit(1);
        break;

    case kHcHopLimit64:
        aIp6Header.SetHopLimit(64);
        break;

    case kHcHopLimit255:
        aIp6Header.SetHopLimit(255);
        break;

    default:
        VerifyOrExit(remaining >= 1);
        aIp6Header.SetHopLimit(cur[0]);
        cur++;
        remaining--;
        break;
    }

    // Source Address
    switch (hcCtl & kHcSrcAddrModeMask)
    {
    case kHcSrcAddrMode0:
        if ((hcCtl & kHcSrcAddrContext) == 0)
        {
            VerifyOrExit(remaining >= sizeof(Ip6::Address));
            memcpy(&aIp6Header.GetSource(), cur, sizeof(aIp6Header.GetSource()));
            cur += sizeof(Ip6::Address);
            remaining -= sizeof(Ip6::Address);
        }

        break;

    case kHcSrcAddrMode1:
        VerifyOrExit(remaining >= Ip6::Address::kInterfaceIdentifierSize);
        aIp6Header.GetSource().SetIid(cur);
        cur += Ip6::Address::kInterfaceIdentifierSize;
        remaining -= Ip6::Address::kInterfaceIdentifierSize;
        break;

    case kHcSrcAddrMode2:
        VerifyOrExit(remaining >= 2);
        aIp6Header.GetSource().mFields.m8[11] = 0xff;
        aIp6Header.GetSource().mFields.m8[12] = 0xfe;
        memcpy(aIp6Header.GetSource().mFields.m8 + 14, cur, 2);
        cur += 2;
        remaining -= 2;
        break;

    case kHcSrcAddrMode3:
        ComputeIid(aMacSource, srcContext, aIp6Header.GetSource());
        break;
    }

    if ((hcCtl & kHcSrcAddrModeMask) != kHcSrcAddrMode0)
    {
        if ((hcCtl & kHcSrcAddrContext) == 0)
        {
            aIp6Header.GetSource().mFields.m16[0] = HostSwap16(0xfe80);
        }
        else
        {
            VerifyOrExit(srcContextValid);
            CopyContext(srcContext, aIp6Header.GetSource());
        }
    }

    if ((hcCtl & kHcMulticast) == 0)
    {
        // Unicast Destination Address

        switch (hcCtl & kHcDstAddrModeMask)
        {
        case kHcDstAddrMode0:
            VerifyOrExit((hcCtl & kHcDstAddrContext) == 0);
            VerifyOrExit(remaining >= sizeof(Ip6::Address));
            memcpy(&aIp6Header.GetDestination(), cur, sizeof(aIp6Header.GetDestination()));
            cur += sizeof(Ip6::Address);
            remaining -= sizeof(Ip6::Address);
            break;

        case kHcDstAddrMode1:
            VerifyOrExit(remaining >= Ip6::Address::kInterfaceIdentifierSize);
            aIp6Header.GetDestination().SetIid(cur);
            cur += Ip6::Address::kInterfaceIdentifierSize;
            remaining -= Ip6::Address::kInterfaceIdentifierSize;
            break;

        case kHcDstAddrMode2:
            VerifyOrExit(remaining >= 2);
            aIp6Header.GetDestination().mFields.m8[11] = 0xff;
            aIp6Header.GetDestination().mFields.m8[12] = 0xfe;
            memcpy(aIp6Header.GetDestination().mFields.m8 + 14, cur, 2);
            cur += 2;
            remaining -= 2;
            break;

        case kHcDstAddrMode3:
            SuccessOrExit(ComputeIid(aMacDest, dstContext, aIp6Header.GetDestination()));
            break;
        }

        if ((hcCtl & kHcDstAddrContext) == 0)
        {
            if ((hcCtl & kHcDstAddrModeMask) != 0)
            {
                aIp6Header.GetDestination().mFields.m16[0] = HostSwap16(0xfe80);
            }
        }
        else
        {
            VerifyOrExit(dstContextValid);
            CopyContext(dstContext, aIp6Header.GetDestination());
        }
    }
    else
    {
        // Multicast Destination Address

        aIp6Header.GetDestination().mFields.m8[0] = 0xff;

        if ((hcCtl & kHcDstAddrContext) == 0)
        {
            switch (hcCtl & kHcDstAddrModeMask)
            {
            case kHcDstAddrMode0:
                VerifyOrExit(remaining >= sizeof(Ip6::Address));
                memcpy(aIp6Header.GetDestination().mFields.m8, cur, sizeof(Ip6::Address));
                cur += sizeof(Ip6::Address);
                remaining -= sizeof(Ip6::Address);
                break;

            case kHcDstAddrMode1:
                VerifyOrExit(remaining >= 6);
                aIp6Header.GetDestination().mFields.m8[1] = cur[0];
                memcpy(aIp6Header.GetDestination().mFields.m8 + 11, cur + 1, 5);
                cur += 6;
                remaining -= 6;
                break;

            case kHcDstAddrMode2:
                VerifyOrExit(remaining >= 4);
                aIp6Header.GetDestination().mFields.m8[1] = cur[0];
                memcpy(aIp6Header.GetDestination().mFields.m8 + 13, cur + 1, 3);
                cur += 4;
                remaining -= 4;
                break;

            case kHcDstAddrMode3:
                VerifyOrExit(remaining >= 1);
                aIp6Header.GetDestination().mFields.m8[1]  = 0x02;
                aIp6Header.GetDestination().mFields.m8[15] = cur[0];
                cur++;
                remaining--;
                break;
            }
        }
        else
        {
            switch (hcCtl & kHcDstAddrModeMask)
            {
            case 0:
                VerifyOrExit(remaining >= 6);
                VerifyOrExit(dstContextValid);
                aIp6Header.GetDestination().mFields.m8[1] = cur[0];
                aIp6Header.GetDestination().mFields.m8[2] = cur[1];
                aIp6Header.GetDestination().mFields.m8[3] = dstContext.mPrefixLength;
                memcpy(aIp6Header.GetDestination().mFields.m8 + 4, dstContext.mPrefix, 8);
                memcpy(aIp6Header.GetDestination().mFields.m8 + 12, cur + 2, 4);
                cur += 6;
                remaining -= 6;
                break;

            default:
                ExitNow();
            }
        }
    }

    if ((hcCtl & kHcNextHeader) != 0)
    {
        VerifyOrExit(remaining >= 1);
        SuccessOrExit(DispatchToNextHeader(cur[0], nextHeader));
        aIp6Header.SetNextHeader(nextHeader);
    }

    error = OT_ERROR_NONE;

exit:
    return (error == OT_ERROR_NONE) ? static_cast<int>(cur - aBuf) : -1;
}

int Lowpan::DecompressExtensionHeader(Message &aMessage, const uint8_t *aBuf, uint16_t aBufLength)
{
    otError         error     = OT_ERROR_PARSE;
    const uint8_t * cur       = aBuf;
    uint16_t        remaining = aBufLength;
    uint8_t         hdr[2];
    uint8_t         len;
    uint8_t         nextHeader;
    uint8_t         ctl = cur[0];
    uint8_t         padLength;
    Ip6::OptionPad1 optionPad1;
    Ip6::OptionPadN optionPadN;

    VerifyOrExit(remaining >= 1);
    cur++;
    remaining--;

    // next header
    if (ctl & kExtHdrNextHeader)
    {
        VerifyOrExit(remaining >= 1);

        len = cur[0];
        cur++;
        remaining--;

        VerifyOrExit(remaining >= len);
        SuccessOrExit(DispatchToNextHeader(cur[len], nextHeader));
        hdr[0] = static_cast<uint8_t>(nextHeader);
    }
    else
    {
        VerifyOrExit(remaining >= 2);

        hdr[0] = cur[0];
        len    = cur[1];
        cur += 2;
        remaining -= 2;

        VerifyOrExit(remaining >= len);
    }

    // length
    hdr[1] = BitVectorBytes(sizeof(hdr) + len) - 1;

    SuccessOrExit(aMessage.Append(hdr, sizeof(hdr)));
    aMessage.MoveOffset(sizeof(hdr));

    // payload
    SuccessOrExit(aMessage.Append(cur, len));
    aMessage.MoveOffset(len);
    cur += len;

    // The RFC6282 says: "The trailing Pad1 or PadN option MAY be elided by the compressor.
    // A decompressor MUST ensure that the containing header is padded out to a multiple of 8 octets
    // in length, using a Pad1 or PadN option if necessary."
    padLength = 8 - ((len + sizeof(hdr)) & 0x07);

    if (padLength != 8)
    {
        if (padLength == 1)
        {
            optionPad1.Init();
            SuccessOrExit(aMessage.Append(&optionPad1, padLength));
        }
        else
        {
            optionPadN.Init(padLength);
            SuccessOrExit(aMessage.Append(&optionPadN, padLength));
        }

        aMessage.MoveOffset(padLength);
    }

    error = OT_ERROR_NONE;

exit:
    return (error == OT_ERROR_NONE) ? static_cast<int>(cur - aBuf) : -1;
}

int Lowpan::DecompressUdpHeader(Ip6::UdpHeader &aUdpHeader, const uint8_t *aBuf, uint16_t aBufLength)
{
    otError        error     = OT_ERROR_PARSE;
    const uint8_t *cur       = aBuf;
    uint16_t       remaining = aBufLength;
    uint8_t        udpCtl;

    VerifyOrExit(remaining >= 1);
    udpCtl = cur[0];
    cur++;
    remaining--;

    VerifyOrExit((udpCtl & kUdpDispatchMask) == kUdpDispatch);

    memset(&aUdpHeader, 0, sizeof(aUdpHeader));

    // source and dest ports
    switch (udpCtl & kUdpPortMask)
    {
    case 0:
        VerifyOrExit(remaining >= 4);
        aUdpHeader.SetSourcePort(ReadUint16(cur));
        aUdpHeader.SetDestinationPort(ReadUint16(cur + 2));
        cur += 4;
        remaining -= 4;
        break;

    case 1:
        VerifyOrExit(remaining >= 3);
        aUdpHeader.SetSourcePort(ReadUint16(cur));
        aUdpHeader.SetDestinationPort(0xf000 | cur[2]);
        cur += 3;
        remaining -= 3;
        break;

    case 2:
        VerifyOrExit(remaining >= 3);
        aUdpHeader.SetSourcePort(0xf000 | cur[0]);
        aUdpHeader.SetDestinationPort(ReadUint16(cur + 1));
        cur += 3;
        remaining -= 3;
        break;

    case 3:
        VerifyOrExit(remaining >= 1);
        aUdpHeader.SetSourcePort(0xf0b0 | (cur[0] >> 4));
        aUdpHeader.SetDestinationPort(0xf0b0 | (cur[0] & 0xf));
        cur += 1;
        remaining -= 1;
        break;
    }

    // checksum
    if ((udpCtl & kUdpChecksum) != 0)
    {
        ExitNow();
    }
    else
    {
        VerifyOrExit(remaining >= 2);
        aUdpHeader.SetChecksum(ReadUint16(cur));
        cur += 2;
    }

    error = OT_ERROR_NONE;

exit:
    return (error == OT_ERROR_NONE) ? static_cast<int>(cur - aBuf) : -1;
}

int Lowpan::DecompressUdpHeader(Message &aMessage, const uint8_t *aBuf, uint16_t aBufLength, uint16_t aDatagramLength)
{
    Ip6::UdpHeader udpHeader;
    int            headerLen = -1;

    headerLen = DecompressUdpHeader(udpHeader, aBuf, aBufLength);
    VerifyOrExit(headerLen >= 0);

    // length
    if (aDatagramLength == 0)
    {
        udpHeader.SetLength(sizeof(udpHeader) + static_cast<uint16_t>(aBufLength - headerLen));
    }
    else
    {
        udpHeader.SetLength(aDatagramLength - aMessage.GetOffset());
    }

    VerifyOrExit(aMessage.Append(&udpHeader, sizeof(udpHeader)) == OT_ERROR_NONE, headerLen = -1);
    aMessage.MoveOffset(sizeof(udpHeader));

exit:
    return headerLen;
}

int Lowpan::Decompress(Message &           aMessage,
                       const Mac::Address &aMacSource,
                       const Mac::Address &aMacDest,
                       const uint8_t *     aBuf,
                       uint16_t            aBufLength,
                       uint16_t            aDatagramLength)
{
    otError        error = OT_ERROR_PARSE;
    Ip6::Header    ip6Header;
    const uint8_t *cur       = aBuf;
    uint16_t       remaining = aBufLength;
    bool           compressed;
    int            rval;
    uint16_t       ip6PayloadLength;
    uint16_t       compressedLength = 0;
    uint16_t       currentOffset    = aMessage.GetOffset();

    VerifyOrExit(remaining >= 2);
    VerifyOrExit((rval = DecompressBaseHeader(ip6Header, compressed, aMacSource, aMacDest, cur, remaining)) >= 0);

    cur += rval;
    remaining -= rval;

    SuccessOrExit(aMessage.Append(&ip6Header, sizeof(ip6Header)));
    SuccessOrExit(aMessage.MoveOffset(sizeof(ip6Header)));

    while (compressed)
    {
        VerifyOrExit(remaining >= 1);

        if ((cur[0] & kExtHdrDispatchMask) == kExtHdrDispatch)
        {
            if ((cur[0] & kExtHdrEidMask) == kExtHdrEidIp6)
            {
                compressed = false;

                cur++;
                remaining--;

                VerifyOrExit((rval = Decompress(aMessage, aMacSource, aMacDest, cur, remaining, aDatagramLength)) >= 0);
            }
            else
            {
                compressed = (cur[0] & kExtHdrNextHeader) != 0;
                VerifyOrExit((rval = DecompressExtensionHeader(aMessage, cur, remaining)) >= 0);
            }
        }
        else if ((cur[0] & kUdpDispatchMask) == kUdpDispatch)
        {
            compressed = false;
            VerifyOrExit((rval = DecompressUdpHeader(aMessage, cur, remaining, aDatagramLength)) >= 0);
        }
        else
        {
            ExitNow();
        }

        VerifyOrExit(remaining >= rval);
        cur += rval;
        remaining -= rval;
    }

    compressedLength = static_cast<uint16_t>(cur - aBuf);

    if (aDatagramLength)
    {
        ip6PayloadLength = HostSwap16(aDatagramLength - currentOffset - sizeof(Ip6::Header));
    }
    else
    {
        ip6PayloadLength =
            HostSwap16(aMessage.GetOffset() - currentOffset - sizeof(Ip6::Header) + aBufLength - compressedLength);
    }

    aMessage.Write(currentOffset + Ip6::Header::GetPayloadLengthOffset(), sizeof(ip6PayloadLength), &ip6PayloadLength);

    error = OT_ERROR_NONE;

exit:
    return (error == OT_ERROR_NONE) ? static_cast<int>(compressedLength) : -1;
}

otError MeshHeader::Init(const uint8_t *aFrame, uint16_t aFrameLength)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(aFrameLength >= 1, error = OT_ERROR_PARSE);
    mDispatchHopsLeft = *aFrame++;
    aFrameLength--;

    if (IsDeepHopsLeftField())
    {
        VerifyOrExit(aFrameLength >= 1, error = OT_ERROR_PARSE);
        mDeepHopsLeft = *aFrame++;
        aFrameLength--;
    }
    else
    {
        mDeepHopsLeft = 0;
    }

    VerifyOrExit(aFrameLength >= sizeof(mAddress), error = OT_ERROR_PARSE);
    memcpy(&mAddress, aFrame, sizeof(mAddress));

exit:
    return error;
}

otError MeshHeader::Init(const Message &aMessage)
{
    otError  error  = OT_ERROR_NONE;
    uint16_t offset = 0;
    uint16_t bytesRead;

    bytesRead = aMessage.Read(offset, sizeof(mDispatchHopsLeft), &mDispatchHopsLeft);
    VerifyOrExit(bytesRead == sizeof(mDispatchHopsLeft), error = OT_ERROR_PARSE);
    offset += bytesRead;

    if (IsDeepHopsLeftField())
    {
        bytesRead = aMessage.Read(offset, sizeof(mDeepHopsLeft), &mDeepHopsLeft);
        VerifyOrExit(bytesRead == sizeof(mDeepHopsLeft), error = OT_ERROR_PARSE);
        offset += bytesRead;
    }
    else
    {
        mDeepHopsLeft = 0;
    }

    bytesRead = aMessage.Read(offset, sizeof(mAddress), &mAddress);
    VerifyOrExit(bytesRead == sizeof(mAddress), error = OT_ERROR_PARSE);

exit:
    return error;
}

otError FragmentHeader::Init(const uint8_t *aFrame, uint16_t aFrameLength)
{
    otError error = OT_ERROR_PARSE;

    VerifyOrExit(aFrameLength >= sizeof(mDispatchSize) + sizeof(mTag));
    memcpy(reinterpret_cast<uint8_t *>(&mDispatchSize), aFrame, sizeof(mDispatchSize) + sizeof(mTag));
    aFrame += sizeof(mDispatchSize) + sizeof(mTag);
    aFrameLength -= sizeof(mDispatchSize) + sizeof(mTag);

    if (IsOffsetPresent())
    {
        VerifyOrExit(aFrameLength >= sizeof(mOffset));
        mOffset = *aFrame++;
    }

    error = OT_ERROR_NONE;

exit:
    return error;
}

otError FragmentHeader::Init(const Message &aMessage, uint16_t aOffset)
{
    otError  error = OT_ERROR_NONE;
    uint16_t bytesRead;

    bytesRead = aMessage.Read(aOffset, sizeof(mDispatchSize), reinterpret_cast<void *>(&mDispatchSize));
    VerifyOrExit(bytesRead == sizeof(mDispatchSize), error = OT_ERROR_PARSE);
    aOffset += bytesRead;

    VerifyOrExit(IsFragmentHeader(), error = OT_ERROR_PARSE);

    bytesRead = aMessage.Read(aOffset, sizeof(mTag), reinterpret_cast<void *>(&mTag));
    VerifyOrExit(bytesRead == sizeof(mTag), error = OT_ERROR_PARSE);
    aOffset += bytesRead;

    if (IsOffsetPresent())
    {
        bytesRead = aMessage.Read(aOffset, sizeof(mOffset), &mOffset);
        VerifyOrExit(bytesRead == sizeof(mOffset), error = OT_ERROR_PARSE);
    }

exit:
    return error;
}

} // namespace Lowpan
} // namespace ot
