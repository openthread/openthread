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

#include "instance/instance.hpp"

namespace ot {
namespace Lowpan {

Lowpan::Lowpan(Instance &aInstance)
    : InstanceLocator(aInstance)
{
}

void Lowpan::FindContextForId(uint8_t aContextId, Context &aContext) const
{
    if (Get<NetworkData::Leader>().GetContext(aContextId, aContext) != kErrorNone)
    {
        aContext.Clear();
    }
}

void Lowpan::FindContextToCompressAddress(const Ip6::Address &aIp6Address, Context &aContext) const
{
    Error error = Get<NetworkData::Leader>().GetContext(aIp6Address, aContext);

    if ((error != kErrorNone) || !aContext.mCompressFlag)
    {
        aContext.Clear();
    }
}

Error Lowpan::ComputeIid(const Mac::Address &aMacAddr, const Context &aContext, Ip6::InterfaceIdentifier &aIid)
{
    Error error = kErrorNone;

    switch (aMacAddr.GetType())
    {
    case Mac::Address::kTypeShort:
        aIid.SetToLocator(aMacAddr.GetShort());
        break;

    case Mac::Address::kTypeExtended:
        aIid.SetFromExtAddress(aMacAddr.GetExtended());
        break;

    default:
        ExitNow(error = kErrorParse);
    }

    aIid.ApplyPrefix(aContext.mPrefix);

exit:
    return error;
}

Error Lowpan::CompressSourceIid(const Mac::Address &aMacAddr,
                                const Ip6::Address &aIpAddr,
                                const Context      &aContext,
                                uint16_t           &aHcCtl,
                                FrameBuilder       &aFrameBuilder)
{
    Error                    error = kErrorNone;
    Ip6::InterfaceIdentifier iid;

    IgnoreError(ComputeIid(aMacAddr, aContext, iid));

    if (iid == aIpAddr.GetIid())
    {
        aHcCtl |= kHcSrcAddrMode3;
    }
    else if (aIpAddr.GetIid().IsLocator())
    {
        aHcCtl |= kHcSrcAddrMode2;
        error = aFrameBuilder.AppendBigEndianUint16(aIpAddr.GetIid().GetLocator());
    }
    else
    {
        aHcCtl |= kHcSrcAddrMode1;
        error = aFrameBuilder.Append(aIpAddr.GetIid());
    }

    return error;
}

Error Lowpan::CompressDestinationIid(const Mac::Address &aMacAddr,
                                     const Ip6::Address &aIpAddr,
                                     const Context      &aContext,
                                     uint16_t           &aHcCtl,
                                     FrameBuilder       &aFrameBuilder)
{
    Error                    error = kErrorNone;
    Ip6::InterfaceIdentifier iid;

    IgnoreError(ComputeIid(aMacAddr, aContext, iid));

    if (iid == aIpAddr.GetIid())
    {
        aHcCtl |= kHcDstAddrMode3;
    }
    else if (aIpAddr.GetIid().IsLocator())
    {
        aHcCtl |= kHcDstAddrMode2;
        error = aFrameBuilder.AppendBigEndianUint16(aIpAddr.GetIid().GetLocator());
    }
    else
    {
        aHcCtl |= kHcDstAddrMode1;
        error = aFrameBuilder.Append(aIpAddr.GetIid());
    }

    return error;
}

Error Lowpan::CompressMulticast(const Ip6::Address &aIpAddr, uint16_t &aHcCtl, FrameBuilder &aFrameBuilder)
{
    Error   error = kErrorNone;
    Context multicastContext;

    aHcCtl |= kHcMulticast;

    for (unsigned int i = 2; i < sizeof(Ip6::Address); i++)
    {
        if (aIpAddr.mFields.m8[i])
        {
            // Check if multicast address can be compressed to 8-bits (ff02::00xx)
            if (aIpAddr.mFields.m8[1] == 0x02 && i >= 15)
            {
                aHcCtl |= kHcDstAddrMode3;
                SuccessOrExit(error = aFrameBuilder.AppendUint8(aIpAddr.mFields.m8[15]));
            }
            // Check if multicast address can be compressed to 32-bits (ffxx::00xx:xxxx)
            else if (i >= 13)
            {
                aHcCtl |= kHcDstAddrMode2;
                SuccessOrExit(error = aFrameBuilder.AppendUint8(aIpAddr.mFields.m8[1]));
                SuccessOrExit(error = aFrameBuilder.AppendBytes(aIpAddr.mFields.m8 + 13, 3));
            }
            // Check if multicast address can be compressed to 48-bits (ffxx::00xx:xxxx:xxxx)
            else if (i >= 11)
            {
                aHcCtl |= kHcDstAddrMode1;
                SuccessOrExit(error = aFrameBuilder.AppendUint8(aIpAddr.mFields.m8[1]));
                SuccessOrExit(error = aFrameBuilder.AppendBytes(aIpAddr.mFields.m8 + 11, 5));
            }
            else
            {
                // Check if multicast address can be compressed using Context ID 0 (mesh local prefix)
                FindContextForId(0, multicastContext);

                if (multicastContext.mPrefix.GetLength() == aIpAddr.mFields.m8[3] &&
                    memcmp(multicastContext.mPrefix.GetBytes(), aIpAddr.mFields.m8 + 4, 8) == 0)
                {
                    aHcCtl |= kHcDstAddrContext | kHcDstAddrMode0;
                    SuccessOrExit(error = aFrameBuilder.AppendBytes(aIpAddr.mFields.m8 + 1, 2));
                    SuccessOrExit(error = aFrameBuilder.AppendBytes(aIpAddr.mFields.m8 + 12, 4));
                }
                else
                {
                    SuccessOrExit(error = aFrameBuilder.Append(aIpAddr));
                }
            }

            break;
        }
    }

exit:
    return error;
}

Error Lowpan::Compress(Message &aMessage, const Mac::Addresses &aMacAddrs, FrameBuilder &aFrameBuilder)
{
    Error   error       = kErrorNone;
    uint8_t headerDepth = 0xff;

    while (headerDepth > 0)
    {
        FrameBuilder frameBuilder = aFrameBuilder;

        error = Compress(aMessage, aMacAddrs, aFrameBuilder, headerDepth);

        // We exit if `Compress()` is successful. Otherwise we reset
        // the `aFrameBuidler` to its earlier state (remove all
        // appended content from the failed `Compress()` call) and
        // try again with a different `headerDepth`.

        VerifyOrExit(error != kErrorNone);
        aFrameBuilder = frameBuilder;
    }

exit:
    return error;
}

Error Lowpan::Compress(Message              &aMessage,
                       const Mac::Addresses &aMacAddrs,
                       FrameBuilder         &aFrameBuilder,
                       uint8_t              &aHeaderDepth)
{
    Error       error       = kErrorNone;
    uint16_t    startOffset = aMessage.GetOffset();
    uint16_t    hcCtl       = kHcDispatch;
    uint16_t    hcCtlOffset = 0;
    Ip6::Header ip6Header;
    uint8_t    *ip6HeaderBytes = reinterpret_cast<uint8_t *>(&ip6Header);
    Context     srcContext, dstContext;
    uint8_t     nextHeader;
    uint8_t     ecn;
    uint8_t     dscp;
    uint8_t     headerDepth    = 0;
    uint8_t     headerMaxDepth = aHeaderDepth;

    SuccessOrExit(error = aMessage.Read(aMessage.GetOffset(), ip6Header));

    FindContextToCompressAddress(ip6Header.GetSource(), srcContext);
    FindContextToCompressAddress(ip6Header.GetDestination(), dstContext);

    // Lowpan HC Control Bits
    hcCtlOffset = aFrameBuilder.GetLength();
    SuccessOrExit(error = aFrameBuilder.AppendBigEndianUint16(hcCtl));

    // Context Identifier
    if (srcContext.mContextId != 0 || dstContext.mContextId != 0)
    {
        hcCtl |= kHcContextId;
        SuccessOrExit(error = aFrameBuilder.AppendUint8(((srcContext.mContextId << 4) | dstContext.mContextId) & 0xff));
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

            SuccessOrExit(error = aFrameBuilder.AppendUint8(ecn | dscp));
        }
    }
    else if (dscp == 0)
    {
        // Carry Flow Label and ECN only with 2-bit padding.
        hcCtl |= kHcTrafficClass;

        SuccessOrExit(error = aFrameBuilder.AppendUint8(ecn | (ip6HeaderBytes[1] & 0x0f)));
        SuccessOrExit(error = aFrameBuilder.AppendBytes(ip6HeaderBytes + 2, 2));
    }
    else
    {
        // Carry Flow Label and Traffic Class in-line.
        SuccessOrExit(error = aFrameBuilder.AppendUint8(ecn | dscp));
        SuccessOrExit(error = aFrameBuilder.AppendUint8(ip6HeaderBytes[1] & 0x0f));
        SuccessOrExit(error = aFrameBuilder.AppendBytes(ip6HeaderBytes + 2, 2));
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
        OT_FALL_THROUGH;

    default:
        SuccessOrExit(error = aFrameBuilder.AppendUint8(static_cast<uint8_t>(ip6Header.GetNextHeader())));
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
        SuccessOrExit(error = aFrameBuilder.AppendUint8(ip6Header.GetHopLimit()));
        break;
    }

    // Source Address
    if (ip6Header.GetSource().IsUnspecified())
    {
        hcCtl |= kHcSrcAddrContext;
    }
    else if (ip6Header.GetSource().IsLinkLocalUnicast())
    {
        SuccessOrExit(
            error = CompressSourceIid(aMacAddrs.mSource, ip6Header.GetSource(), srcContext, hcCtl, aFrameBuilder));
    }
    else if (srcContext.mIsValid)
    {
        hcCtl |= kHcSrcAddrContext;
        SuccessOrExit(
            error = CompressSourceIid(aMacAddrs.mSource, ip6Header.GetSource(), srcContext, hcCtl, aFrameBuilder));
    }
    else
    {
        SuccessOrExit(error = aFrameBuilder.Append(ip6Header.GetSource()));
    }

    // Destination Address
    if (ip6Header.GetDestination().IsMulticast())
    {
        SuccessOrExit(error = CompressMulticast(ip6Header.GetDestination(), hcCtl, aFrameBuilder));
    }
    else if (ip6Header.GetDestination().IsLinkLocalUnicast())
    {
        SuccessOrExit(error = CompressDestinationIid(aMacAddrs.mDestination, ip6Header.GetDestination(), dstContext,
                                                     hcCtl, aFrameBuilder));
    }
    else if (dstContext.mIsValid)
    {
        hcCtl |= kHcDstAddrContext;
        SuccessOrExit(error = CompressDestinationIid(aMacAddrs.mDestination, ip6Header.GetDestination(), dstContext,
                                                     hcCtl, aFrameBuilder));
    }
    else
    {
        SuccessOrExit(error = aFrameBuilder.Append(ip6Header.GetDestination()));
    }

    headerDepth++;

    aMessage.MoveOffset(sizeof(ip6Header));

    nextHeader = static_cast<uint8_t>(ip6Header.GetNextHeader());

    while (headerDepth < headerMaxDepth)
    {
        switch (nextHeader)
        {
        case Ip6::kProtoHopOpts:
            SuccessOrExit(error = CompressExtensionHeader(aMessage, aFrameBuilder, nextHeader));
            break;

        case Ip6::kProtoUdp:
            error = CompressUdp(aMessage, aFrameBuilder);
            ExitNow();

        case Ip6::kProtoIp6:
            // For IP-in-IP the NH bit of the LOWPAN_NHC encoding MUST be set to zero.
            SuccessOrExit(error = aFrameBuilder.AppendUint8(kExtHdrDispatch | kExtHdrEidIp6));

            error = Compress(aMessage, aMacAddrs, aFrameBuilder);

            OT_FALL_THROUGH;

        default:
            ExitNow();
        }

        headerDepth++;
    }

exit:
    aHeaderDepth = headerDepth;

    if (error == kErrorNone)
    {
        aFrameBuilder.Write<uint16_t>(hcCtlOffset, BigEndian::HostSwap16(hcCtl));
    }
    else
    {
        aMessage.SetOffset(startOffset);
    }

    return error;
}

Error Lowpan::CompressExtensionHeader(Message &aMessage, FrameBuilder &aFrameBuilder, uint8_t &aNextHeader)
{
    Error                error       = kErrorNone;
    uint16_t             startOffset = aMessage.GetOffset();
    Ip6::ExtensionHeader extHeader;
    uint16_t             len;
    uint16_t             padLength = 0;
    uint8_t              tmpByte;

    SuccessOrExit(error = aMessage.Read(aMessage.GetOffset(), extHeader));
    aMessage.MoveOffset(sizeof(extHeader));

    tmpByte = kExtHdrDispatch | kExtHdrEidHbh;

    switch (extHeader.GetNextHeader())
    {
    case Ip6::kProtoUdp:
    case Ip6::kProtoIp6:
        tmpByte |= kExtHdrNextHeader;
        break;

    default:
        SuccessOrExit(error = aFrameBuilder.AppendUint8(tmpByte));
        tmpByte = static_cast<uint8_t>(extHeader.GetNextHeader());
        break;
    }

    SuccessOrExit(error = aFrameBuilder.AppendUint8(tmpByte));

    len = extHeader.GetSize() - sizeof(extHeader);

    // RFC 6282 does not support compressing large extension headers
    VerifyOrExit(len <= kExtHdrMaxLength, error = kErrorFailed);

    // RFC 6282 says: "IPv6 Hop-by-Hop and Destination Options Headers may use a trailing
    // Pad1 or PadN to achieve 8-octet alignment. When there is a single trailing Pad1 or PadN
    // option of 7 octets or less and the containing header is a multiple of 8 octets, the trailing
    // Pad1 or PadN option MAY be elided by the compressor."
    if (aNextHeader == Ip6::kProtoHopOpts || aNextHeader == Ip6::kProtoDstOpts)
    {
        OffsetRange offsetRange;
        bool        hasOption = false;
        Ip6::Option option;

        offsetRange.Init(aMessage.GetOffset(), len);

        for (; !offsetRange.IsEmpty(); offsetRange.AdvanceOffset(option.GetSize()))
        {
            SuccessOrExit(error = option.ParseFrom(aMessage, offsetRange));
            hasOption = true;
        }

        // Check if the last option can be compressed.
        if (hasOption && option.IsPadding())
        {
            padLength = option.GetSize();
            len -= padLength;
        }
    }

    VerifyOrExit(aMessage.GetOffset() + len + padLength <= aMessage.GetLength(), error = kErrorParse);

    aNextHeader = static_cast<uint8_t>(extHeader.GetNextHeader());

    SuccessOrExit(error = aFrameBuilder.AppendUint8(static_cast<uint8_t>(len)));
    SuccessOrExit(error = aFrameBuilder.AppendBytesFromMessage(aMessage, aMessage.GetOffset(), len));
    aMessage.MoveOffset(len + padLength);

exit:
    if (error != kErrorNone)
    {
        aMessage.SetOffset(startOffset);
    }

    return error;
}

Error Lowpan::CompressUdp(Message &aMessage, FrameBuilder &aFrameBuilder)
{
    Error            error       = kErrorNone;
    uint16_t         startOffset = aMessage.GetOffset();
    Ip6::Udp::Header udpHeader;
    uint16_t         source;
    uint16_t         destination;

    SuccessOrExit(error = aMessage.Read(aMessage.GetOffset(), udpHeader));

    source      = udpHeader.GetSourcePort();
    destination = udpHeader.GetDestinationPort();

    if ((source & 0xfff0) == 0xf0b0 && (destination & 0xfff0) == 0xf0b0)
    {
        SuccessOrExit(error = aFrameBuilder.AppendUint8(kUdpDispatch | 3));
        SuccessOrExit(error = aFrameBuilder.AppendUint8((((source & 0xf) << 4) | (destination & 0xf)) & 0xff));
    }
    else if ((source & 0xff00) == 0xf000)
    {
        SuccessOrExit(error = aFrameBuilder.AppendUint8(kUdpDispatch | 2));
        SuccessOrExit(error = aFrameBuilder.AppendUint8(source & 0xff));
        SuccessOrExit(error = aFrameBuilder.AppendBigEndianUint16(destination));
    }
    else if ((destination & 0xff00) == 0xf000)
    {
        SuccessOrExit(error = aFrameBuilder.AppendUint8(kUdpDispatch | 1));
        SuccessOrExit(error = aFrameBuilder.AppendBigEndianUint16(source));
        SuccessOrExit(error = aFrameBuilder.AppendUint8(destination & 0xff));
    }
    else
    {
        SuccessOrExit(error = aFrameBuilder.AppendUint8(kUdpDispatch));
        SuccessOrExit(error = aFrameBuilder.AppendBytes(&udpHeader, Ip6::Udp::Header::kLengthFieldOffset));
    }

    SuccessOrExit(error = aFrameBuilder.AppendBigEndianUint16(udpHeader.GetChecksum()));

    aMessage.MoveOffset(sizeof(udpHeader));

exit:
    if (error != kErrorNone)
    {
        aMessage.SetOffset(startOffset);
    }

    return error;
}

Error Lowpan::DispatchToNextHeader(uint8_t aDispatch, uint8_t &aNextHeader)
{
    Error error = kErrorNone;

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

    error = kErrorParse;

exit:
    return error;
}

Error Lowpan::DecompressBaseHeader(Ip6::Header          &aIp6Header,
                                   bool                 &aCompressedNextHeader,
                                   const Mac::Addresses &aMacAddrs,
                                   FrameData            &aFrameData)
{
    Error    error = kErrorParse;
    uint16_t hcCtl;
    uint8_t  byte;
    uint8_t  srcContextId = 0;
    uint8_t  dstContextId = 0;
    Context  srcContext;
    Context  dstContext;
    uint8_t  nextHeader;

    SuccessOrExit(aFrameData.ReadBigEndianUint16(hcCtl));

    // check Dispatch bits
    VerifyOrExit((hcCtl & kHcDispatchMask) == kHcDispatch);

    // Context Identifier
    if ((hcCtl & kHcContextId) != 0)
    {
        SuccessOrExit(aFrameData.ReadUint8(byte));

        srcContextId = (byte >> 4);
        dstContextId = (byte & 0xf);
    }

    FindContextForId(srcContextId, srcContext);
    FindContextForId(dstContextId, dstContext);

    aIp6Header.Clear();
    aIp6Header.InitVersionTrafficClassFlow();

    // Traffic Class and Flow Label
    if ((hcCtl & kHcTrafficFlowMask) != kHcTrafficFlow)
    {
        uint8_t *ip6HeaderBytes = reinterpret_cast<uint8_t *>(&aIp6Header);

        VerifyOrExit(aFrameData.GetLength() > 0);

        ip6HeaderBytes[1] |= (aFrameData.GetBytes()[0] & 0xc0) >> 2;

        if ((hcCtl & kHcTrafficClass) == 0)
        {
            IgnoreError(aFrameData.ReadUint8(byte));
            ip6HeaderBytes[0] |= (byte >> 2) & 0x0f;
            ip6HeaderBytes[1] |= (byte << 6) & 0xc0;
        }

        if ((hcCtl & kHcFlowLabel) == 0)
        {
            VerifyOrExit(aFrameData.GetLength() >= 3);
            ip6HeaderBytes[1] |= aFrameData.GetBytes()[0] & 0x0f;
            ip6HeaderBytes[2] |= aFrameData.GetBytes()[1];
            ip6HeaderBytes[3] |= aFrameData.GetBytes()[2];
            aFrameData.SkipOver(3);
        }
    }

    // Next Header
    if ((hcCtl & kHcNextHeader) == 0)
    {
        SuccessOrExit(aFrameData.ReadUint8(byte));

        aIp6Header.SetNextHeader(byte);
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
        SuccessOrExit(aFrameData.ReadUint8(byte));
        aIp6Header.SetHopLimit(byte);
        break;
    }

    // Source Address
    switch (hcCtl & kHcSrcAddrModeMask)
    {
    case kHcSrcAddrMode0:
        if ((hcCtl & kHcSrcAddrContext) == 0)
        {
            SuccessOrExit(aFrameData.Read(aIp6Header.GetSource()));
        }

        break;

    case kHcSrcAddrMode1:
        SuccessOrExit(aFrameData.Read(aIp6Header.GetSource().GetIid()));
        break;

    case kHcSrcAddrMode2:
        aIp6Header.GetSource().mFields.m8[11] = 0xff;
        aIp6Header.GetSource().mFields.m8[12] = 0xfe;
        SuccessOrExit(aFrameData.ReadBytes(aIp6Header.GetSource().mFields.m8 + 14, 2));
        break;

    case kHcSrcAddrMode3:
        IgnoreError(ComputeIid(aMacAddrs.mSource, srcContext, aIp6Header.GetSource().GetIid()));
        break;
    }

    if ((hcCtl & kHcSrcAddrModeMask) != kHcSrcAddrMode0)
    {
        if ((hcCtl & kHcSrcAddrContext) == 0)
        {
            aIp6Header.GetSource().mFields.m16[0] = BigEndian::HostSwap16(0xfe80);
        }
        else
        {
            VerifyOrExit(srcContext.mIsValid);
            aIp6Header.GetSource().SetPrefix(srcContext.mPrefix);
        }
    }

    if ((hcCtl & kHcMulticast) == 0)
    {
        // Unicast Destination Address

        switch (hcCtl & kHcDstAddrModeMask)
        {
        case kHcDstAddrMode0:
            VerifyOrExit((hcCtl & kHcDstAddrContext) == 0);
            SuccessOrExit(aFrameData.Read(aIp6Header.GetDestination()));
            break;

        case kHcDstAddrMode1:
            SuccessOrExit(aFrameData.Read(aIp6Header.GetDestination().GetIid()));
            break;

        case kHcDstAddrMode2:
            aIp6Header.GetDestination().mFields.m8[11] = 0xff;
            aIp6Header.GetDestination().mFields.m8[12] = 0xfe;
            SuccessOrExit(aFrameData.ReadBytes(aIp6Header.GetDestination().mFields.m8 + 14, 2));
            break;

        case kHcDstAddrMode3:
            SuccessOrExit(ComputeIid(aMacAddrs.mDestination, dstContext, aIp6Header.GetDestination().GetIid()));
            break;
        }

        if ((hcCtl & kHcDstAddrContext) == 0)
        {
            if ((hcCtl & kHcDstAddrModeMask) != 0)
            {
                aIp6Header.GetDestination().mFields.m16[0] = BigEndian::HostSwap16(0xfe80);
            }
        }
        else
        {
            VerifyOrExit(dstContext.mIsValid);
            aIp6Header.GetDestination().SetPrefix(dstContext.mPrefix);
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
                SuccessOrExit(aFrameData.Read(aIp6Header.GetDestination()));
                break;

            case kHcDstAddrMode1:
                SuccessOrExit(aFrameData.ReadUint8(aIp6Header.GetDestination().mFields.m8[1]));
                SuccessOrExit(aFrameData.ReadBytes(aIp6Header.GetDestination().mFields.m8 + 11, 5));
                break;

            case kHcDstAddrMode2:
                SuccessOrExit(aFrameData.ReadUint8(aIp6Header.GetDestination().mFields.m8[1]));
                SuccessOrExit(aFrameData.ReadBytes(aIp6Header.GetDestination().mFields.m8 + 13, 3));
                break;

            case kHcDstAddrMode3:
                aIp6Header.GetDestination().mFields.m8[1] = 0x02;
                SuccessOrExit(aFrameData.ReadUint8(aIp6Header.GetDestination().mFields.m8[15]));
                break;
            }
        }
        else
        {
            switch (hcCtl & kHcDstAddrModeMask)
            {
            case 0:
                VerifyOrExit(dstContext.mIsValid);
                SuccessOrExit(aFrameData.ReadBytes(aIp6Header.GetDestination().mFields.m8 + 1, 2));
                aIp6Header.GetDestination().mFields.m8[3] = dstContext.mPrefix.GetLength();
                memcpy(aIp6Header.GetDestination().mFields.m8 + 4, dstContext.mPrefix.GetBytes(), 8);
                SuccessOrExit(aFrameData.ReadBytes(aIp6Header.GetDestination().mFields.m8 + 12, 4));
                break;

            default:
                ExitNow();
            }
        }
    }

    if ((hcCtl & kHcNextHeader) != 0)
    {
        VerifyOrExit(aFrameData.GetLength() > 0);
        SuccessOrExit(DispatchToNextHeader(*aFrameData.GetBytes(), nextHeader));
        aIp6Header.SetNextHeader(nextHeader);
    }

    error = kErrorNone;

exit:
    return error;
}

Error Lowpan::DecompressExtensionHeader(Message &aMessage, FrameData &aFrameData)
{
    Error          error = kErrorParse;
    uint8_t        hdr[2];
    uint8_t        len;
    uint8_t        ctl;
    Ip6::PadOption padOption;

    SuccessOrExit(aFrameData.ReadUint8(ctl));

    // next header
    if (ctl & kExtHdrNextHeader)
    {
        SuccessOrExit(aFrameData.ReadUint8(len));

        VerifyOrExit(aFrameData.CanRead(len + 1));
        SuccessOrExit(DispatchToNextHeader(aFrameData.GetBytes()[len], hdr[0]));
    }
    else
    {
        SuccessOrExit(aFrameData.ReadUint8(hdr[0]));
        SuccessOrExit(aFrameData.ReadUint8(len));

        VerifyOrExit(aFrameData.CanRead(len));
    }

    // length
    hdr[1] = BytesForBitSize(sizeof(hdr) + len) - 1;

    SuccessOrExit(aMessage.AppendBytes(hdr, sizeof(hdr)));
    aMessage.MoveOffset(sizeof(hdr));

    // payload
    SuccessOrExit(aMessage.AppendBytes(aFrameData.GetBytes(), len));
    aMessage.MoveOffset(len);
    aFrameData.SkipOver(len);

    // The RFC6282 says: "The trailing Pad1 or PadN option MAY be elided by the compressor.
    // A decompressor MUST ensure that the containing header is padded out to a multiple of 8 octets
    // in length, using a Pad1 or PadN option if necessary."

    if (padOption.InitToPadHeaderWithSize(len + sizeof(hdr)) == kErrorNone)
    {
        SuccessOrExit(aMessage.AppendBytes(&padOption, padOption.GetSize()));
        aMessage.MoveOffset(padOption.GetSize());
    }

    error = kErrorNone;

exit:
    return error;
}

Error Lowpan::DecompressUdpHeader(Ip6::Udp::Header &aUdpHeader, FrameData &aFrameData)
{
    Error    error = kErrorParse;
    uint8_t  udpCtl;
    uint8_t  byte;
    uint16_t srcPort = 0;
    uint16_t dstPort = 0;

    SuccessOrExit(aFrameData.ReadUint8(udpCtl));

    VerifyOrExit((udpCtl & kUdpDispatchMask) == kUdpDispatch);

    aUdpHeader.Clear();

    switch (udpCtl & kUdpPortMask)
    {
    case 0:
        SuccessOrExit(aFrameData.ReadBigEndianUint16(srcPort));
        SuccessOrExit(aFrameData.ReadBigEndianUint16(dstPort));
        break;

    case 1:
        SuccessOrExit(aFrameData.ReadBigEndianUint16(srcPort));
        SuccessOrExit(aFrameData.ReadUint8(byte));
        dstPort = (0xf000 | byte);
        break;

    case 2:
        SuccessOrExit(aFrameData.ReadUint8(byte));
        srcPort = (0xf000 | byte);
        SuccessOrExit(aFrameData.ReadBigEndianUint16(dstPort));
        break;

    case 3:
        SuccessOrExit(aFrameData.ReadUint8(byte));
        srcPort = (0xf0b0 | (byte >> 4));
        dstPort = (0xf0b0 | (byte & 0xf));
        break;
    }

    aUdpHeader.SetSourcePort(srcPort);
    aUdpHeader.SetDestinationPort(dstPort);

    if ((udpCtl & kUdpChecksum) != 0)
    {
        ExitNow();
    }
    else
    {
        uint16_t checksum;

        SuccessOrExit(aFrameData.ReadBigEndianUint16(checksum));
        aUdpHeader.SetChecksum(checksum);
    }

    error = kErrorNone;

exit:
    return error;
}

Error Lowpan::DecompressUdpHeader(Message &aMessage, FrameData &aFrameData, uint16_t aDatagramLength)
{
    Error            error;
    Ip6::Udp::Header udpHeader;

    SuccessOrExit(error = DecompressUdpHeader(udpHeader, aFrameData));

    // length
    if (aDatagramLength == 0)
    {
        udpHeader.SetLength(sizeof(udpHeader) + aFrameData.GetLength());
    }
    else
    {
        udpHeader.SetLength(aDatagramLength - aMessage.GetOffset());
    }

    SuccessOrExit(error = aMessage.Append(udpHeader));
    aMessage.MoveOffset(sizeof(udpHeader));

exit:
    return error;
}

Error Lowpan::Decompress(Message              &aMessage,
                         const Mac::Addresses &aMacAddrs,
                         FrameData            &aFrameData,
                         uint16_t              aDatagramLength)
{
    Error       error = kErrorParse;
    Ip6::Header ip6Header;
    bool        compressed;
    uint16_t    ip6PayloadLength;
    uint16_t    currentOffset = aMessage.GetOffset();

    SuccessOrExit(DecompressBaseHeader(ip6Header, compressed, aMacAddrs, aFrameData));

    SuccessOrExit(aMessage.Append(ip6Header));
    aMessage.MoveOffset(sizeof(ip6Header));

    while (compressed)
    {
        uint8_t byte;

        VerifyOrExit(aFrameData.GetLength() > 0);
        byte = *aFrameData.GetBytes();

        if ((byte & kExtHdrDispatchMask) == kExtHdrDispatch)
        {
            if ((byte & kExtHdrEidMask) == kExtHdrEidIp6)
            {
                compressed = false;

                aFrameData.SkipOver(sizeof(uint8_t));

                SuccessOrExit(Decompress(aMessage, aMacAddrs, aFrameData, aDatagramLength));
            }
            else
            {
                compressed = (byte & kExtHdrNextHeader) != 0;
                SuccessOrExit(DecompressExtensionHeader(aMessage, aFrameData));
            }
        }
        else if ((byte & kUdpDispatchMask) == kUdpDispatch)
        {
            compressed = false;
            SuccessOrExit(DecompressUdpHeader(aMessage, aFrameData, aDatagramLength));
        }
        else
        {
            ExitNow();
        }
    }

    if (aDatagramLength)
    {
        ip6PayloadLength = BigEndian::HostSwap16(aDatagramLength - currentOffset - sizeof(Ip6::Header));
    }
    else
    {
        ip6PayloadLength =
            BigEndian::HostSwap16(aMessage.GetOffset() - currentOffset - sizeof(Ip6::Header) + aFrameData.GetLength());
    }

    aMessage.Write(currentOffset + Ip6::Header::kPayloadLengthFieldOffset, ip6PayloadLength);

    error = kErrorNone;

exit:
    return error;
}

Ip6::Ecn Lowpan::DecompressEcn(const Message &aMessage, uint16_t aOffset) const
{
    Ip6::Ecn ecn = Ip6::kEcnNotCapable;
    uint16_t hcCtl;
    uint8_t  byte;

    SuccessOrExit(aMessage.Read(aOffset, hcCtl));
    hcCtl = BigEndian::HostSwap16(hcCtl);

    VerifyOrExit((hcCtl & kHcDispatchMask) == kHcDispatch);
    aOffset += sizeof(uint16_t);

    if ((hcCtl & kHcTrafficFlowMask) == kHcTrafficFlow)
    {
        // ECN is elided and is zero (`kEcnNotCapable`).
        ExitNow();
    }

    // When ECN is not elided, it is always included as the
    // first two bits of the next byte.
    SuccessOrExit(aMessage.Read(aOffset, byte));
    ecn = static_cast<Ip6::Ecn>((byte & kEcnMask) >> kEcnOffset);

exit:
    return ecn;
}

void Lowpan::MarkCompressedEcn(Message &aMessage, uint16_t aOffset)
{
    uint8_t byte;

    aOffset += sizeof(uint16_t);
    IgnoreError(aMessage.Read(aOffset, byte));

    byte &= ~kEcnMask;
    byte |= static_cast<uint8_t>(Ip6::kEcnMarked << kEcnOffset);

    aMessage.Write(aOffset, byte);
}

//---------------------------------------------------------------------------------------------------------------------
// MeshHeader

void MeshHeader::Init(uint16_t aSource, uint16_t aDestination, uint8_t aHopsLeft)
{
    mSource      = aSource;
    mDestination = aDestination;
    mHopsLeft    = aHopsLeft;
}

bool MeshHeader::IsMeshHeader(const FrameData &aFrameData)
{
    return (aFrameData.GetLength() >= kMinHeaderLength) && ((*aFrameData.GetBytes() & kDispatchMask) == kDispatch);
}

Error MeshHeader::ParseFrom(FrameData &aFrameData)
{
    Error    error;
    uint16_t headerLength;

    SuccessOrExit(error = ParseFrom(aFrameData.GetBytes(), aFrameData.GetLength(), headerLength));
    aFrameData.SkipOver(headerLength);

exit:
    return error;
}

Error MeshHeader::ParseFrom(const uint8_t *aFrame, uint16_t aFrameLength, uint16_t &aHeaderLength)
{
    Error   error = kErrorParse;
    uint8_t dispatch;

    VerifyOrExit(aFrameLength >= kMinHeaderLength);
    dispatch = *aFrame++;

    VerifyOrExit((dispatch & (kDispatchMask | kSourceShort | kDestShort)) == (kDispatch | kSourceShort | kDestShort));

    mHopsLeft = (dispatch & kHopsLeftMask);

    if (mHopsLeft == kDeepHopsLeft)
    {
        VerifyOrExit(aFrameLength >= kDeepHopsHeaderLength);
        mHopsLeft     = *aFrame++;
        aHeaderLength = kDeepHopsHeaderLength;
    }
    else
    {
        aHeaderLength = kMinHeaderLength;
    }

    mSource      = BigEndian::ReadUint16(aFrame);
    mDestination = BigEndian::ReadUint16(aFrame + 2);

    error = kErrorNone;

exit:
    return error;
}

Error MeshHeader::ParseFrom(const Message &aMessage)
{
    uint16_t headerLength;

    return ParseFrom(aMessage, headerLength);
}

Error MeshHeader::ParseFrom(const Message &aMessage, uint16_t &aHeaderLength)
{
    uint8_t  frame[kDeepHopsHeaderLength];
    uint16_t frameLength;

    frameLength = aMessage.ReadBytes(/* aOffset */ 0, frame, sizeof(frame));

    return ParseFrom(frame, frameLength, aHeaderLength);
}

uint16_t MeshHeader::GetHeaderLength(void) const
{
    return (mHopsLeft >= kDeepHopsLeft) ? kDeepHopsHeaderLength : kMinHeaderLength;
}

void MeshHeader::DecrementHopsLeft(void)
{
    if (mHopsLeft > 0)
    {
        mHopsLeft--;
    }
}

Error MeshHeader::AppendTo(FrameBuilder &aFrameBuilder) const
{
    Error   error;
    uint8_t dispatch = (kDispatch | kSourceShort | kDestShort);

    if (mHopsLeft < kDeepHopsLeft)
    {
        SuccessOrExit(error = aFrameBuilder.AppendUint8(dispatch | mHopsLeft));
    }
    else
    {
        SuccessOrExit(error = aFrameBuilder.AppendUint8(dispatch | kDeepHopsLeft));
        SuccessOrExit(error = aFrameBuilder.AppendUint8(mHopsLeft));
    }

    SuccessOrExit(error = aFrameBuilder.AppendBigEndianUint16(mSource));
    SuccessOrExit(error = aFrameBuilder.AppendBigEndianUint16(mDestination));

exit:
    return error;
}

Error MeshHeader::AppendTo(Message &aMessage) const
{
    uint8_t      frame[kDeepHopsHeaderLength];
    FrameBuilder frameBuilder;

    frameBuilder.Init(frame, sizeof(frame));

    IgnoreError(AppendTo(frameBuilder));

    return aMessage.AppendBytes(frameBuilder.GetBytes(), frameBuilder.GetLength());
}

//---------------------------------------------------------------------------------------------------------------------
// FragmentHeader

bool FragmentHeader::IsFragmentHeader(const FrameData &aFrameData)
{
    return IsFragmentHeader(aFrameData.GetBytes(), aFrameData.GetLength());
}

bool FragmentHeader::IsFragmentHeader(const uint8_t *aFrame, uint16_t aFrameLength)
{
    return (aFrameLength >= sizeof(FirstFrag)) && ((*aFrame & kDispatchMask) == kDispatch);
}

Error FragmentHeader::ParseFrom(FrameData &aFrameData)
{
    Error    error;
    uint16_t headerLength;

    SuccessOrExit(error = ParseFrom(aFrameData.GetBytes(), aFrameData.GetLength(), headerLength));
    aFrameData.SkipOver(headerLength);

exit:
    return error;
}

Error FragmentHeader::ParseFrom(const uint8_t *aFrame, uint16_t aFrameLength, uint16_t &aHeaderLength)
{
    Error error = kErrorParse;

    VerifyOrExit(IsFragmentHeader(aFrame, aFrameLength));

    mSize = BigEndian::ReadUint16(aFrame + kSizeIndex) & kSizeMask;
    mTag  = BigEndian::ReadUint16(aFrame + kTagIndex);

    if ((*aFrame & kOffsetFlag) == kOffsetFlag)
    {
        VerifyOrExit(aFrameLength >= sizeof(NextFrag));
        mOffset       = aFrame[kOffsetIndex] * 8;
        aHeaderLength = sizeof(NextFrag);
    }
    else
    {
        mOffset       = 0;
        aHeaderLength = sizeof(FirstFrag);
    }

    error = kErrorNone;

exit:
    return error;
}

Error FragmentHeader::ParseFrom(const Message &aMessage, uint16_t aOffset, uint16_t &aHeaderLength)
{
    uint8_t  frame[sizeof(NextFrag)];
    uint16_t frameLength;

    frameLength = aMessage.ReadBytes(aOffset, frame, sizeof(frame));

    return ParseFrom(frame, frameLength, aHeaderLength);
}

} // namespace Lowpan
} // namespace ot
