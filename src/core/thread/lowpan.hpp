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

#include "openthread-core-config.h"

#include "common/clearable.hpp"
#include "common/debug.hpp"
#include "common/frame_builder.hpp"
#include "common/frame_data.hpp"
#include "common/locator.hpp"
#include "common/message.hpp"
#include "common/non_copyable.hpp"
#include "mac/mac_types.hpp"
#include "net/ip6.hpp"
#include "net/ip6_address.hpp"
#include "net/ip6_types.hpp"

namespace ot {

/**
 * @addtogroup core-6lowpan
 *
 * @brief
 *   This module includes definitions for 6LoWPAN header compression.
 *
 * @{
 */

/**
 * @namespace ot::Lowpan
 *
 * @brief
 *   This namespace includes definitions for 6LoWPAN message processing.
 */
namespace Lowpan {

/**
 * Represents a LOWPAN_IPHC Context.
 */
struct Context : public Clearable<Context>
{
    Ip6::Prefix mPrefix;       ///< The Prefix
    uint8_t     mContextId;    ///< The Context ID.
    bool        mCompressFlag; ///< The Context compression flag.
    bool        mIsValid;      ///< Indicates whether the context is valid.
};

/**
 * Implements LOWPAN_IPHC header compression.
 */
class Lowpan : public InstanceLocator, private NonCopyable
{
public:
    /**
     * Initializes the object.
     *
     * @param[in]  aInstance     A reference to the OpenThread instance.
     */
    explicit Lowpan(Instance &aInstance);

    /**
     * Indicates whether or not the header is a LOWPAN_IPHC header.
     *
     * @param[in]  aHeader  A pointer to the header.
     *
     * @retval TRUE   If the header matches the LOWPAN_IPHC dispatch value.
     * @retval FALSE  If the header does not match the LOWPAN_IPHC dispatch value.
     */
    static bool IsLowpanHc(const uint8_t *aHeader)
    {
        return (aHeader[0] & (Lowpan::kHcDispatchMask >> 8)) == (Lowpan::kHcDispatch >> 8);
    }

    /**
     * Indicates whether or not header in a given frame is a LOWPAN_IPHC header.
     *
     * @param[in] aFrameData    The frame data.
     *
     * @retval TRUE   If the header matches the LOWPAN_IPHC dispatch value.
     * @retval FALSE  If the header does not match the LOWPAN_IPHC dispatch value.
     */
    static bool IsLowpanHc(const FrameData &aFrameData)
    {
        return (aFrameData.GetLength() > 0) && IsLowpanHc(aFrameData.GetBytes());
    }

    /**
     * Compresses an IPv6 header.
     *
     * @param[in]   aMessage       A reference to the IPv6 message.
     * @param[in]   aMacAddrs      The MAC source and destination addresses.
     * @param[in]   aFrameBuilder  The `FrameBuilder` to use to append the compressed headers.
     *
     * @returns The size of the compressed header in bytes.
     */
    Error Compress(Message &aMessage, const Mac::Addresses &aMacAddrs, FrameBuilder &aFrameBuilder);

    /**
     * Decompresses a LOWPAN_IPHC header.
     *
     * If the header is parsed successfully the @p aFrameData is updated to skip over the parsed header bytes.
     *
     * @param[out]    aMessage         A reference where the IPv6 header will be placed.
     * @param[in]     aMacAddrs        The MAC source and destination addresses.
     * @param[in,out] aFrameData       A frame data containing the LOWPAN_IPHC header.
     * @param[in]     aDatagramLength  The IPv6 datagram length.
     *
     * @retval kErrorNone    The header was decompressed successfully. @p aMessage and @p aFrameData are updated.
     * @retval kErrorParse   Failed to parse the lowpan header.
     * @retval kErrorNoBufs  Could not grow @p aMessage to write the parsed IPv6 header.
     */
    Error Decompress(Message              &aMessage,
                     const Mac::Addresses &aMacAddrs,
                     FrameData            &aFrameData,
                     uint16_t              aDatagramLength);

    /**
     * Decompresses a LOWPAN_IPHC header.
     *
     * If the header is parsed successfully the @p aFrameData is updated to skip over the parsed header bytes.
     *
     * @param[out]    aIp6Header             A reference where the IPv6 header will be placed.
     * @param[out]    aCompressedNextHeader  A boolean reference to output whether next header is compressed or not.
     * @param[in]     aMacAddrs              The MAC source and destination addresses
     * @param[in,out] aFrameData             A frame data containing the LOWPAN_IPHC header.
     *
     * @retval kErrorNone    The header was decompressed successfully. @p aIp6Header and @p aFrameData are updated.
     * @retval kErrorParse   Failed to parse the lowpan header.
     */
    Error DecompressBaseHeader(Ip6::Header          &aIp6Header,
                               bool                 &aCompressedNextHeader,
                               const Mac::Addresses &aMacAddrs,
                               FrameData            &aFrameData);

    /**
     * Decompresses a LOWPAN_NHC UDP header.
     *
     * If the header is parsed successfully the @p aFrameData is updated to skip over the parsed header bytes.
     *
     * @param[out]    aUdpHeader    A reference where the UDP header will be placed.
     * @param[in,out] aFrameData    A frame data containing the LOWPAN_NHC header.
     *
     * @retval kErrorNone    The header was decompressed successfully. @p aUdpHeader and @p aFrameData are updated.
     * @retval kErrorParse   Failed to parse the lowpan header.
     */
    Error DecompressUdpHeader(Ip6::Udp::Header &aUdpHeader, FrameData &aFrameData);

    /**
     * Decompresses the IPv6 ECN field in a LOWPAN_IPHC header.
     *
     * @param[in] aMessage  The message to read the IPHC header from.
     * @param[in] aOffset   The offset in @p aMessage to start of IPHC header.
     *
     * @returns The decompressed ECN field. If the IPHC header is not valid `kEcnNotCapable` is returned.
     */
    Ip6::Ecn DecompressEcn(const Message &aMessage, uint16_t aOffset) const;

    /**
     * Updates the compressed ECN field in a LOWPAN_IPHC header to `kEcnMarked`.
     *
     * MUST be used when the ECN field is not elided in the IPHC header. Note that the ECN is not elided
     * when it is not zero (`kEcnNotCapable`).
     *
     * @param[in,out] aMessage  The message containing the IPHC header and to update.
     * @param[in]     aOffset   The offset in @p aMessage to start of IPHC header.
     */
    void MarkCompressedEcn(Message &aMessage, uint16_t aOffset);

private:
    static constexpr uint16_t kHcDispatch     = 3 << 13;
    static constexpr uint16_t kHcDispatchMask = 7 << 13;

    static constexpr uint16_t kHcTrafficClass    = 1 << 11;
    static constexpr uint16_t kHcFlowLabel       = 2 << 11;
    static constexpr uint16_t kHcTrafficFlow     = 3 << 11;
    static constexpr uint16_t kHcTrafficFlowMask = 3 << 11;
    static constexpr uint16_t kHcNextHeader      = 1 << 10;
    static constexpr uint16_t kHcHopLimit1       = 1 << 8;
    static constexpr uint16_t kHcHopLimit64      = 2 << 8;
    static constexpr uint16_t kHcHopLimit255     = 3 << 8;
    static constexpr uint16_t kHcHopLimitMask    = 3 << 8;
    static constexpr uint16_t kHcContextId       = 1 << 7;
    static constexpr uint16_t kHcSrcAddrContext  = 1 << 6;
    static constexpr uint16_t kHcSrcAddrMode0    = 0 << 4;
    static constexpr uint16_t kHcSrcAddrMode1    = 1 << 4;
    static constexpr uint16_t kHcSrcAddrMode2    = 2 << 4;
    static constexpr uint16_t kHcSrcAddrMode3    = 3 << 4;
    static constexpr uint16_t kHcSrcAddrModeMask = 3 << 4;
    static constexpr uint16_t kHcMulticast       = 1 << 3;
    static constexpr uint16_t kHcDstAddrContext  = 1 << 2;
    static constexpr uint16_t kHcDstAddrMode0    = 0 << 0;
    static constexpr uint16_t kHcDstAddrMode1    = 1 << 0;
    static constexpr uint16_t kHcDstAddrMode2    = 2 << 0;
    static constexpr uint16_t kHcDstAddrMode3    = 3 << 0;
    static constexpr uint16_t kHcDstAddrModeMask = 3 << 0;

    static constexpr uint8_t kEcnOffset = 6;
    static constexpr uint8_t kEcnMask   = 3 << kEcnOffset;

    static constexpr uint8_t kExtHdrDispatch     = 0xe0;
    static constexpr uint8_t kExtHdrDispatchMask = 0xf0;

    static constexpr uint8_t kExtHdrEidHbh      = 0x00;
    static constexpr uint8_t kExtHdrEidRouting  = 0x02;
    static constexpr uint8_t kExtHdrEidFragment = 0x04;
    static constexpr uint8_t kExtHdrEidDst      = 0x06;
    static constexpr uint8_t kExtHdrEidMobility = 0x08;
    static constexpr uint8_t kExtHdrEidIp6      = 0x0e;
    static constexpr uint8_t kExtHdrEidMask     = 0x0e;

    static constexpr uint8_t  kExtHdrNextHeader = 0x01;
    static constexpr uint16_t kExtHdrMaxLength  = 255;

    static constexpr uint8_t kUdpDispatch     = 0xf0;
    static constexpr uint8_t kUdpDispatchMask = 0xf8;

    static constexpr uint8_t kUdpChecksum = 1 << 2;
    static constexpr uint8_t kUdpPortMask = 3 << 0;

    void  FindContextForId(uint8_t aContextId, Context &aContext) const;
    void  FindContextToCompressAddress(const Ip6::Address &aIp6Address, Context &aContext) const;
    Error Compress(Message              &aMessage,
                   const Mac::Addresses &aMacAddrs,
                   FrameBuilder         &aFrameBuilder,
                   uint8_t              &aHeaderDepth);

    Error CompressExtensionHeader(Message &aMessage, FrameBuilder &aFrameBuilder, uint8_t &aNextHeader);
    Error CompressSourceIid(const Mac::Address &aMacAddr,
                            const Ip6::Address &aIpAddr,
                            const Context      &aContext,
                            uint16_t           &aHcCtl,
                            FrameBuilder       &aFrameBuilder);
    Error CompressDestinationIid(const Mac::Address &aMacAddr,
                                 const Ip6::Address &aIpAddr,
                                 const Context      &aContext,
                                 uint16_t           &aHcCtl,
                                 FrameBuilder       &aFrameBuilder);
    Error CompressMulticast(const Ip6::Address &aIpAddr, uint16_t &aHcCtl, FrameBuilder &aFrameBuilder);
    Error CompressUdp(Message &aMessage, FrameBuilder &aFrameBuilder);

    Error DecompressExtensionHeader(Message &aMessage, FrameData &aFrameData);
    Error DecompressUdpHeader(Message &aMessage, FrameData &aFrameData, uint16_t aDatagramLength);
    Error DispatchToNextHeader(uint8_t aDispatch, uint8_t &aNextHeader);

    static Error ComputeIid(const Mac::Address &aMacAddr, const Context &aContext, Ip6::InterfaceIdentifier &aIid);
};

/**
 * Implements Mesh Header generation and processing.
 */
class MeshHeader
{
public:
    /**
     * Initializes the Mesh Header with a given Mesh Source, Mesh Destination and Hops Left value.
     *
     * @param[in]  aSource       The Mesh Source address.
     * @param[in]  aDestination  The Mesh Destination address.
     * @param[in]  aHopsLeft     The Hops Left value.
     */
    void Init(uint16_t aSource, uint16_t aDestination, uint8_t aHopsLeft);

    /**
     * Indicates whether or not the header (in a given frame) is a Mesh Header.
     *
     * @note This method checks whether the first byte in header/frame (dispatch byte) matches the Mesh Header dispatch
     * It does not fully parse and validate the Mesh Header. `ParseFrom()` method can be used to fully parse and
     * validate the header.
     *
     * @retval TRUE   If the header matches the Mesh Header dispatch value.
     * @retval FALSE  If the header does not match the Mesh Header dispatch value.
     */
    static bool IsMeshHeader(const FrameData &aFrameData);

    /**
     * Parses the Mesh Header from a frame @p aFrame.
     *
     * @param[in]  aFrame        The pointer to the frame.
     * @param[in]  aFrameLength  The length of the frame.
     * @param[out] aHeaderLength A reference to a variable to output the parsed header length (on success).
     *
     * @retval kErrorNone     Mesh Header parsed successfully.
     * @retval kErrorParse    Mesh Header could not be parsed.
     */
    Error ParseFrom(const uint8_t *aFrame, uint16_t aFrameLength, uint16_t &aHeaderLength);

    /**
     * Parses the Mesh Header from a given frame data.
     *
     * If the Mesh Header is parsed successfully the @p aFrameData is updated to skip over the parsed header bytes.
     *
     * @param[in,out]  aFrameData    The frame data to parse from.
     *
     * @retval kErrorNone     Mesh Header parsed successfully. @p aFrameData is updated to skip over parsed header.
     * @retval kErrorParse    Mesh Header could not be parsed.
     */
    Error ParseFrom(FrameData &aFrameData);

    /**
     * Parses the Mesh Header from a given message.
     *
     * @note The Mesh Header is read from offset zero within the @p aMessage.
     *
     * @param[in]  aMessage    The message to read from.
     *
     * @retval kErrorNone   Mesh Header parsed successfully.
     * @retval kErrorParse  Mesh Header could not be parsed.
     */
    Error ParseFrom(const Message &aMessage);

    /**
     * Parses the Mesh Header from a given message.
     *
     * @note The Mesh Header is read from offset zero within the @p aMessage.
     *
     * @param[in]  aMessage       The message to read from.
     * @param[out] aHeaderLength  A reference to a variable to output the parsed header length (on success).
     *
     * @retval kErrorNone   Mesh Header parsed successfully.
     * @retval kErrorParse  Mesh Header could not be parsed.
     */
    Error ParseFrom(const Message &aMessage, uint16_t &aHeaderLength);

    /**
     * Returns the the Mesh Header length when written to a frame.
     *
     * @note The returned value from this method gives the header length (number of bytes) when the header is written
     * to a frame or message. This should not be used to determine the parsed length (number of bytes read) when the
     * Mesh Header is parsed from a frame/message (using `ParseFrom()` methods).
     *
     * @returns The length of the Mesh Header (in bytes) when written to a frame.
     */
    uint16_t GetHeaderLength(void) const;

    /**
     * Returns the Hops Left value.
     *
     * @returns The Hops Left value.
     */
    uint8_t GetHopsLeft(void) const { return mHopsLeft; }

    /**
     * Decrements the Hops Left value (if it is not zero).
     */
    void DecrementHopsLeft(void);

    /**
     * Returns the Mesh Source address.
     *
     * @returns The Mesh Source address.
     */
    uint16_t GetSource(void) const { return mSource; }

    /**
     * Returns the Mesh Destination address.
     *
     * @returns The Mesh Destination address.
     */
    uint16_t GetDestination(void) const { return mDestination; }

    /**
     * Appends the Mesh Header into a given frame.
     *
     * @param[out]  aFrameBuilder  The `FrameBuilder` to append to.
     *
     * @retval kErrorNone    Successfully appended the MeshHeader to @p aFrameBuilder.
     * @retval kErrorNoBufs  Insufficient available buffers.
     */
    Error AppendTo(FrameBuilder &aFrameBuilder) const;

    /**
     * Appends the Mesh Header to a given message.
     *
     *
     * @param[out] aMessage  A message to append the Mesh Header to.
     *
     * @retval kErrorNone    Successfully appended the Mesh Header to @p aMessage.
     * @retval kErrorNoBufs  Insufficient available buffers to grow @p aMessage.
     */
    Error AppendTo(Message &aMessage) const;

private:
    static constexpr uint8_t kDispatch     = 2 << 6;
    static constexpr uint8_t kDispatchMask = 3 << 6;
    static constexpr uint8_t kHopsLeftMask = 0x0f;
    static constexpr uint8_t kSourceShort  = 1 << 5;
    static constexpr uint8_t kDestShort    = 1 << 4;
    static constexpr uint8_t kDeepHopsLeft = 0x0f;

    // Dispatch byte + src + dest
    static constexpr uint16_t kMinHeaderLength      = sizeof(uint8_t) + sizeof(uint16_t) + sizeof(uint16_t);
    static constexpr uint16_t kDeepHopsHeaderLength = kMinHeaderLength + sizeof(uint8_t); // min header + deep hops

    uint16_t mSource;
    uint16_t mDestination;
    uint8_t  mHopsLeft;
};

/**
 * Implements Fragment Header generation and parsing.
 */
class FragmentHeader
{
public:
    OT_TOOL_PACKED_BEGIN
    class FirstFrag
    {
    public:
        /**
         * Initializes the `FirstFrag`.
         *
         * @param[in] aSize  The Datagram Size value.
         * @param[in] aTag   The Datagram Tag value.
         */
        void Init(uint16_t aSize, uint16_t aTag)
        {
            mDispatchSize = BigEndian::HostSwap16(kFirstDispatch | (aSize & kSizeMask));
            mTag          = BigEndian::HostSwap16(aTag);
        }

    private:
        //                       1                   2                   3
        //   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
        //  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        //  |1 1 0 0 0|    datagram_size    |         datagram_tag          |
        //  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

        static constexpr uint16_t kFirstDispatch = 0xc000; // 0b11000_0000_0000_0000

        uint16_t mDispatchSize;
        uint16_t mTag;
    } OT_TOOL_PACKED_END;

    OT_TOOL_PACKED_BEGIN
    class NextFrag
    {
    public:
        /**
         * Initializes the `NextFrag`.
         *
         * @param[in] aSize    The Datagram Size value.
         * @param[in] aTag     The Datagram Tag value.
         * @param[in] aOffset  The Datagram Offset value.
         */
        void Init(uint16_t aSize, uint16_t aTag, uint16_t aOffset)
        {
            mDispatchSize = BigEndian::HostSwap16(kNextDispatch | (aSize & kSizeMask));
            mTag          = BigEndian::HostSwap16(aTag);
            mOffset       = static_cast<uint8_t>(aOffset >> 3);
        }

    private:
        //                       1                   2                   3
        //   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
        //  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        //  |1 1 1 0 0|    datagram_size    |         datagram_tag          |
        //  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        //  |datagram_offset|
        //  +-+-+-+-+-+-+-+-+

        static constexpr uint16_t kNextDispatch = 0xe000; // 0b11100_0000_0000_0000

        uint16_t mDispatchSize;
        uint16_t mTag;
        uint8_t  mOffset;
    } OT_TOOL_PACKED_END;

    /**
     * Indicates whether or not the header (in a given frame) is a Fragment Header.
     *
     * @note This method checks whether the frame has the minimum required length and that the first byte in
     * header (dispatch byte) matches the Fragment Header dispatch value. It does not fully parse and validate the
     * Fragment Header. `ParseFrom()` method can be used to fully parse and validate the header.
     *
     * @param[in] aFrameData   The frame data.
     *
     * @retval TRUE   If the header matches the Fragment Header dispatch value.
     * @retval FALSE  If the header does not match the Fragment Header dispatch value.
     */
    static bool IsFragmentHeader(const FrameData &aFrameData);

    /**
     * Parses the Fragment Header from a given frame data.
     *
     * If the Fragment Header is parsed successfully the @p aFrameData is updated to skip over the parsed header bytes.
     *
     * @param[in,out]  aFrameData    The frame data to parse from.
     *
     * @retval kErrorNone     Fragment Header parsed successfully. @p aFrameData is updated to skip over parsed header.
     * @retval kErrorParse    Fragment header could not be parsed.
     */
    Error ParseFrom(FrameData &aFrameData);

    /**
     * Parses the Fragment Header from a message.
     *
     * @param[in]  aMessage      The message to read from.
     * @param[in]  aOffset       The offset within the message to start reading from.
     * @param[out] aHeaderLength A reference to a variable to output the parsed header length (on success).
     *
     * @retval kErrorNone     Fragment Header parsed successfully.
     * @retval kErrorParse    Fragment header could not be parsed from @p aFrame.
     */
    Error ParseFrom(const Message &aMessage, uint16_t aOffset, uint16_t &aHeaderLength);

    /**
     * Returns the Datagram Size value.
     *
     * @returns The Datagram Size value.
     */
    uint16_t GetDatagramSize(void) const { return mSize; }

    /**
     * Returns the Datagram Tag value.
     *
     * @returns The Datagram Tag value.
     */
    uint16_t GetDatagramTag(void) const { return mTag; }

    /**
     * Returns the Datagram Offset value.
     *
     * The returned offset value is always multiple of 8.
     *
     * @returns The Datagram Offset value (multiple of 8).
     */
    uint16_t GetDatagramOffset(void) const { return mOffset; }

private:
    static constexpr uint8_t kDispatch     = 0xc0;   // 0b1100_0000
    static constexpr uint8_t kDispatchMask = 0xd8;   // 0b1101_1000 accepts first (0b1100_0xxx) and next (0b1110_0xxx).
    static constexpr uint8_t kOffsetFlag   = 1 << 5; // Indicate first (no offset) vs. next (offset present) fragment.

    static constexpr uint16_t kSizeMask   = 0x7ff;  // 0b0111_1111_1111 (first 11 bits).
    static constexpr uint16_t kOffsetMask = 0xfff8; // Clears the last 3 bits to ensure offset is a multiple of 8.

    static constexpr uint8_t kSizeIndex   = 0; // Start index of Size field in the Fragment Header byte sequence.
    static constexpr uint8_t kTagIndex    = 2; // Start index of Tag field in the Fragment Header byte sequence.
    static constexpr uint8_t kOffsetIndex = 4; // Start index of Offset field in the Fragment Header byte sequence.

    static bool IsFragmentHeader(const uint8_t *aFrame, uint16_t aFrameLength);

    Error ParseFrom(const uint8_t *aFrame, uint16_t aFrameLength, uint16_t &aHeaderLength);

    uint16_t mSize;
    uint16_t mTag;
    uint16_t mOffset;
};

/**
 * @}
 */

} // namespace Lowpan
} // namespace ot

#endif // LOWPAN_HPP_
