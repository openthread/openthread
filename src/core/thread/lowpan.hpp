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
 *   This file includes definitions for 6LoWPAN header compression.
 */

#ifndef LOWPAN_HPP_
#define LOWPAN_HPP_

#include <common/message.hpp>
#include <mac/mac_frame.hpp>
#include <net/ip6.hpp>
#include <net/ip6_address.hpp>

namespace Thread {

class ThreadNetif;

namespace NetworkData { class Leader; }

/**
 * @addtogroup core-6lowpan
 *
 * @brief
 *   This module includes definitions for 6LoWPAN header compression.
 *
 * @{
 */

/**
 * @namespace Thread::Lowpan
 *
 * @brief
 *   This namespace includes definitions for 6LoWPAN message processing.
 *
 */
namespace Lowpan {

/**
 * This structure represents a LOWPAN_IPHC Context.
 *
 */
struct Context
{
    const uint8_t *mPrefix;        ///< A pointer to the prefix.
    uint8_t        mPrefixLength;  ///< The prefix length.
    uint8_t        mContextId;     ///< The Context ID.
};

/**
 * This class implements LOWPAN_IPHC header compression.
 *
 */
class Lowpan
{
public:
    enum
    {
        kHopsLeft = 14,
    };

    /**
     * This constructor initializes the object.
     *
     * @param[in]  aThreadNetif  A reference to the Thread network interface.
     *
     */
    explicit Lowpan(ThreadNetif &aThreadNetif);

    /**
     * This method indicates whether or not the header is a LOWPAN_IPHC header.
     *
     * @param[in]  aHeader  A pointer to the header.
     *
     * @retval TRUE   If the header matches the LOWPAN_IPHC dispatch value.
     * @retval FALSE  If the header does not match the LOWPAN_IPHC dispatch value.
     */
    static bool IsLowpanHc(uint8_t *aHeader) {
        return (aHeader[0] & (Lowpan::kHcDispatchMask >> 8)) == (Lowpan::kHcDispatch >> 8);
    }

    /**
     * This method compresses an IPv6 header.
     *
     * @param[in]   aMessage     A reference to the IPv6 message.
     * @param[in]   aMacSource   The MAC source address.
     * @param[in]   aMacDest     The MAC destination address.
     * @param[out]  aBuf         A pointer where the compressed IPv6 header will be placed.
     *
     * @returns The size of the compressed header in bytes.
     *
     */
    int Compress(Message &aMessage, const Mac::Address &aMacSource, const Mac::Address &aMacDest, uint8_t *aBuf);

    /**
     * This method decompresses a LOWPAN_IPHC header.
     *
     * @param[out]  aMessage      A reference where the IPv6 header will be placed.
     * @param[in]   aMacSource    The MAC source address.
     * @param[in]   aMacDest      The MAC destination address.
     * @param[in]   aBuf          A pointer to the LOWPAN_IPHC header.
     * @param[in]   aBufLen       The number of bytes in @p aBuf.
     * @param[in]   aDatagramLen  The IPv6 datagram length.
     *
     * @returns The size of the compressed header in bytes.
     *
     */
    int Decompress(Message &aMessage, const Mac::Address &aMacSource, const Mac::Address &aMacDest,
                   const uint8_t *aBuf, uint16_t aBufLen, uint16_t aDatagramLen);

    /**
     * This method decompresses a LOWPAN_IPHC header.
     *
     * @param[out]  aHeader       A reference where the IPv6 header will be placed.
     * @param[in]   aMacSource    The MAC source address.
     * @param[in]   aMacDest      The MAC destination address.
     * @param[in]   aBuf          A pointer to the LOWPAN_IPHC header.
     *
     * @returns The size of the compressed header in bytes.
     *
     */
    int DecompressBaseHeader(Ip6::Header &aHeader, const Mac::Address &aMacSource, const Mac::Address &aMacDest,
                             const uint8_t *aBuf);

private:
    enum
    {
        kHcDispatch        = 3 << 13,
        kHcDispatchMask    = 7 << 13,

        kHcTrafficClass    = 1 << 11,
        kHcFlowLabel       = 2 << 11,
        kHcTrafficFlow     = 3 << 11,
        kHcTrafficFlowMask = 3 << 11,
        kHcNextHeader      = 1 << 10,
        kHcHopLimit1       = 1 << 8,
        kHcHopLimit64      = 2 << 8,
        kHcHopLimit255     = 3 << 8,
        kHcHopLimitMask    = 3 << 8,
        kHcContextId       = 1 << 7,
        kHcSrcAddrContext  = 1 << 6,
        kHcSrcAddrMode0    = 0 << 4,
        kHcSrcAddrMode1    = 1 << 4,
        kHcSrcAddrMode2    = 2 << 4,
        kHcSrcAddrMode3    = 3 << 4,
        kHcSrcAddrModeMask = 3 << 4,
        kHcMulticast       = 1 << 3,
        kHcDstAddrContext  = 1 << 2,
        kHcDstAddrMode0    = 0 << 0,
        kHcDstAddrMode1    = 1 << 0,
        kHcDstAddrMode2    = 2 << 0,
        kHcDstAddrMode3    = 3 << 0,
        kHcDstAddrModeMask = 3 << 0,

        kExtHdrDispatch     = 0xe0,
        kExtHdrDispatchMask = 0xf0,

        kExtHdrEidHbh       = 0x00,
        kExtHdrEidRouting   = 0x02,
        kExtHdrEidFragment  = 0x04,
        kExtHdrEidDst       = 0x06,
        kExtHdrEidMobility  = 0x08,
        kExtHdrEidIp6       = 0x0e,
        kExtHdrEidMask      = 0x0e,

        kExtHdrNextHeader   = 0x01,

        kUdpDispatch        = 0xf0,
        kUdpDispatchMask    = 0xf8,
        kUdpChecksum        = 1 << 2,
        kUdpPortMask        = 3 << 0,
    };

    int CompressExtensionHeader(Message &message, uint8_t *aBuf, uint8_t &nextHeader);
    int CompressSourceIid(const Mac::Address &macaddr, const Ip6::Address &ipaddr, const Context &aContext,
                          uint16_t &hcCtl, uint8_t *aBuf);
    int CompressDestinationIid(const Mac::Address &macaddr, const Ip6::Address &ipaddr, const Context &aContext,
                               uint16_t &hcCtl, uint8_t *aBuf);
    int CompressMulticast(const Ip6::Address &ipaddr, uint16_t &hcCtl, uint8_t *aBuf);
    int CompressUdp(Message &message, uint8_t *aBuf);

    int DecompressExtensionHeader(Message &message, const uint8_t *aBuf, uint16_t aBufLength);
    int DecompressUdpHeader(Message &message, const uint8_t *aBuf, uint16_t aBufLength, uint16_t datagramLength);
    ThreadError DispatchToNextHeader(uint8_t dispatch, Ip6::IpProto &nextHeader);

    static ThreadError CopyContext(const Context &aContext, Ip6::Address &aAddress);
    static ThreadError ComputeIid(const Mac::Address &aMacAddr, const Context &aContext, Ip6::Address &aIpAddress);

    NetworkData::Leader &mNetworkData;
};

/**
 * This class implements Mesh Header generation and processing.
 *
 */
OT_TOOL_PACKED_BEGIN
class MeshHeader
{
public:
    /**
     * This method initializes the header.
     *
     */
    void Init() { mDispatchHopsLeft = kDispatch | kSourceShort | kDestinationShort; }

    /**
     * This method indicates whether or not the header is a Mesh Header.
     *
     * @retval TRUE   If the header matches the Mesh Header dispatch value.
     * @retval FALSE  If the header does not match the Mesh Header dispatch value.
     *
     */
    bool IsMeshHeader() { return (mDispatchHopsLeft & kDispatchMask) == kDispatch; }

    /**
     * This method indicates whether or not the Mesh Header appears to be well-formed.
     *
     * @retval TRUE   If the header appears to be well-formed.
     * @retval FALSE  If the header does not appear to be well-formed.
     *
     */
    bool IsValid() { return (mDispatchHopsLeft & kSourceShort) && (mDispatchHopsLeft & kDestinationShort); }

    /**
     * This static method returns the size of the Mesh Header in bytes.
     *
     * @returns The size of the Mesh Header in bytes.
     *
     */
    static uint8_t GetHeaderLength() { return sizeof(MeshHeader); }

    /**
     * This method returns the Hops Left value.
     *
     * @returns The Hops Left value.
     *
     */
    uint8_t GetHopsLeft() { return mDispatchHopsLeft & kHopsLeftMask; }

    /**
     * This method sets the Hops Left value.
     *
     * @param[in]  aHops  The Hops Left value.
     *
     */
    void SetHopsLeft(uint8_t aHops) { mDispatchHopsLeft = (mDispatchHopsLeft & ~kHopsLeftMask) | aHops; }

    /**
     * This method returns the Mesh Source address.
     *
     * @returns The Mesh Source address.
     *
     */
    uint16_t GetSource() { return HostSwap16(mSource); }

    /**
     * This method sets the Mesh Source address.
     *
     * @param[in]  aSource  The Mesh Source address.
     *
     */
    void SetSource(uint16_t aSource) { mSource = HostSwap16(aSource); }

    /**
     * This method returns the Mesh Destination address.
     *
     * @returns The Mesh Destination address.
     *
     */
    uint16_t GetDestination() { return HostSwap16(mDestination); }

    /**
     * This method sets the Mesh Destination address.
     *
     * @param[in]  aDestination  The Mesh Destination address.
     *
     */
    void SetDestination(uint16_t aDestination) { mDestination = HostSwap16(aDestination); }

private:
    enum
    {
        kDispatch         = 2 << 6,
        kDispatchMask     = 3 << 6,
        kHopsLeftMask     = 0xf << 0,
        kSourceShort      = 1 << 5,
        kDestinationShort = 1 << 4,
    };

    uint8_t mDispatchHopsLeft;
    uint16_t mSource;
    uint16_t mDestination;
} OT_TOOL_PACKED_END;

/**
 * This class implements Fragment Header generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class FragmentHeader
{
public:
    /**
     * This method initializes the Fragment Header.
     *
     */
    void Init() { mDispatchOffsetSize = kDispatch; }

    /**
     * This method indicates whether or not the header is a Fragment Header.
     *
     * @retval TRUE   If the header matches the Fragment Header dispatch value.
     * @retval FALSE  If the header does not match the Fragment Header dispatch value.
     *
     */
    bool IsFragmentHeader() { return (mDispatchOffsetSize & kDispatchMask) == kDispatch; }

    /**
     * This method returns the Fragment Header length.
     *
     * @returns The Fragment Header length in bytes.
     *
     */
    uint8_t GetHeaderLength() {
        return (mDispatchOffsetSize & kOffset) ? sizeof(*this) : sizeof(*this) - sizeof(mOffset);
    }

    /**
     * This method returns the Datagram Size value.
     *
     * @returns The Datagram Size value.
     *
     */
    uint16_t GetDatagramSize() { return HostSwap16(mSize) & kSizeMask; }

    /**
     * This method sets the Datagram Size value.
     *
     * @param[in]  aSize  The Datagram Size value.
     *
     */
    void SetDatagramSize(uint16_t aSize) {
        mSize = HostSwap16((HostSwap16(mSize) & ~kSizeMask) | (aSize & kSizeMask));
    }

    /**
     * This method returns the Datagram Tag value.
     *
     * @returns The Datagram Tag value.
     *
     */
    uint16_t GetDatagramTag() { return HostSwap16(mTag); }

    /**
     * This method sets the Datagram Tag value.
     *
     * @param[in]  aTag  The Datagram Tag value.
     *
     */
    void SetDatagramTag(uint16_t aTag) { mTag = HostSwap16(aTag); }

    /**
     * This method returns the Datagram Offset value.
     *
     * @returns The Datagram Offset value.
     *
     */
    uint16_t GetDatagramOffset() { return (mDispatchOffsetSize & kOffset) ? static_cast<uint16_t>(mOffset) * 8 : 0; }

    /**
     * This method sets the Datagram Offset value.
     *
     * @param[in]  aOffset  The Datagram Offset value.
     *
     */
    void SetDatagramOffset(uint16_t aOffset) {
        if (aOffset == 0) {
            mDispatchOffsetSize &= ~kOffset;
        }
        else {
            mDispatchOffsetSize |= kOffset;
            mOffset = (aOffset >> 3) & kOffsetMask;
        }
    }

private:
    enum
    {
        kDispatch     = 3 << 6,
        kDispatchMask = 3 << 6,
        kOffset       = 1 << 5,
        kSizeMask     = 0x7ff,
        kOffsetMask   = 0xff,
    };

    union
    {
        uint8_t mDispatchOffsetSize;
        uint16_t mSize;
    } OT_TOOL_PACKED_FIELD;
    uint16_t mTag;
    uint8_t mOffset;
} OT_TOOL_PACKED_END;

/**
 * @}
 */

}  // namespace Lowpan
}  // namespace Thread

#endif  // LOWPAN_HPP_
