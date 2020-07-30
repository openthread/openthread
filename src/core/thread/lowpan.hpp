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

#include "common/debug.hpp"
#include "common/locator.hpp"
#include "common/message.hpp"
#include "mac/mac_types.hpp"
#include "net/ip6.hpp"
#include "net/ip6_address.hpp"

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
 *
 */
namespace Lowpan {

using ot::Encoding::BigEndian::HostSwap16;

/**
 * This structure represents a LOWPAN_IPHC Context.
 *
 */
struct Context
{
    Ip6::Prefix mPrefix;       ///< The Prefix
    uint8_t     mContextId;    ///< The Context ID.
    bool        mCompressFlag; ///< The Context compression flag.
};

/**
 * This class defines a buffer writer used by the 6LoWPAN compressor.
 *
 */
class BufferWriter
{
public:
    /**
     * This constructor initializes the buffer writer.
     *
     * @param[in]  aBuf     A pointer to the write buffer.
     * @param[in]  aLength  The size of the write buffer.
     *
     */
    BufferWriter(uint8_t *aBuf, uint16_t aLength)
        : mWritePointer(aBuf)
        , mEndPointer(aBuf + aLength)
    {
    }

    /**
     * This method indicates whether there is buffer space available to write @p aLength bytes.
     *
     * @param[in]  aLength  Number of bytes to write.
     *
     * @retval  TRUE   Enough buffer space is available to write the requested number of bytes.
     * @retval  FALSE  Insufficient buffer space to write the requested number of bytes.
     *
     */
    bool CanWrite(uint8_t aLength) const { return (mWritePointer + aLength) <= mEndPointer; }

    /**
     * This method returns the current write pointer value.
     *
     * @returns the current write pointer value.
     *
     */
    uint8_t *GetWritePointer(void) { return mWritePointer; }

    /**
     * This method advances the write pointer.
     *
     * @param[in]  aLength  Number of bytes to advance.
     *
     * @retval OT_ERROR_NONE     Enough buffer space is available to advance the requested number of bytes.
     * @retval OT_ERROR_NO_BUFS  Insufficient buffer space to advance the requested number of bytes.
     *
     */
    otError Advance(uint8_t aLength)
    {
        otError error = OT_ERROR_NONE;

        VerifyOrExit(CanWrite(aLength), error = OT_ERROR_NO_BUFS);
        mWritePointer += aLength;

    exit:
        return error;
    }

    /**
     * This method writes a byte into the buffer and updates the write pointer, if space is available.
     *
     * @param[in]  aByte  Byte to write.
     *
     * @retval  OT_ERROR_NONE     Successfully wrote the byte and updated the pointer.
     * @retval  OT_ERROR_NO_BUFS  Insufficient buffer space to write the byte.
     *
     */
    otError Write(uint8_t aByte)
    {
        otError error = OT_ERROR_NONE;

        VerifyOrExit(CanWrite(sizeof(aByte)), error = OT_ERROR_NO_BUFS);

        *mWritePointer++ = aByte;

    exit:
        return error;
    }

    /**
     * This method writes a byte sequence into the buffer and updates the write pointer, if space is available.
     *
     * @param[in]  aBuf     A pointer to the byte sequence.
     * @param[in]  aLength  Number of bytes to write.
     *
     * @retval OT_ERROR_NONE     Successfully wrote the byte sequence and updated the pointer.
     * @retval OT_ERROR_NO_BUFS  Insufficient buffer space to write the byte sequence.
     *
     */
    otError Write(const void *aBuf, uint8_t aLength)
    {
        otError error = OT_ERROR_NONE;

        VerifyOrExit(CanWrite(aLength), error = OT_ERROR_NO_BUFS);

        memcpy(mWritePointer, aBuf, aLength);
        mWritePointer += aLength;

    exit:
        return error;
    }

    /**
     * This method writes a byte sequence into the buffer and updates the write pointer, if space is available.
     *
     * The byte sequence is taken from a message buffer at the current message buffer's offset.
     *
     * @param[in]  aMessage  A message buffer.
     * @param[in]  aLength   Number of bytes to write.
     *
     * @retval OT_ERROR_NONE     Successfully wrote the byte sequence and updated the pointer.
     * @retval OT_ERROR_NO_BUFS  Insufficient buffer space to write the byte sequence.
     *
     */
    otError Write(const Message &aMessage, uint8_t aLength)
    {
        otError error = OT_ERROR_NONE;
        int     rval;

        OT_UNUSED_VARIABLE(rval);

        VerifyOrExit(CanWrite(aLength), error = OT_ERROR_NO_BUFS);

        rval = aMessage.Read(aMessage.GetOffset(), aLength, mWritePointer);
        OT_ASSERT(rval == aLength);

        mWritePointer += aLength;

    exit:
        return error;
    }

private:
    uint8_t *mWritePointer;
    uint8_t *mEndPointer;
};

/**
 * This class implements LOWPAN_IPHC header compression.
 *
 */
class Lowpan : public InstanceLocator
{
public:
    /**
     * This constructor initializes the object.
     *
     * @param[in]  aInstance     A reference to the OpenThread instance.
     *
     */
    explicit Lowpan(Instance &aInstance);

    /**
     * This method indicates whether or not the header is a LOWPAN_IPHC header.
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
    otError Compress(Message &           aMessage,
                     const Mac::Address &aMacSource,
                     const Mac::Address &aMacDest,
                     BufferWriter &      aBuf);

    /**
     * This method decompresses a LOWPAN_IPHC header.
     *
     * @param[out]  aMessage         A reference where the IPv6 header will be placed.
     * @param[in]   aMacSource       The MAC source address.
     * @param[in]   aMacDest         The MAC destination address.
     * @param[in]   aBuf             A pointer to the LOWPAN_IPHC header.
     * @param[in]   aBufLength       The number of bytes in @p aBuf.
     * @param[in]   aDatagramLength  The IPv6 datagram length.
     *
     * @returns The size of the compressed header in bytes.
     *
     */
    int Decompress(Message &           aMessage,
                   const Mac::Address &aMacSource,
                   const Mac::Address &aMacDest,
                   const uint8_t *     aBuf,
                   uint16_t            aBufLength,
                   uint16_t            aDatagramLength);

    /**
     * This method decompresses a LOWPAN_IPHC header.
     *
     * @param[out]  aIp6Header              A reference where the IPv6 header will be placed.
     * @param[out]  aCommpressedNextHeader  A boolean reference to output whether next header is compressed or not.
     * @param[in]   aMacSource              The MAC source address.
     * @param[in]   aMacDest                The MAC destination address.
     * @param[in]   aBuf                    A pointer to the LOWPAN_IPHC header.
     * @param[in]   aBufLength              The number of bytes in @p aBuf.
     *
     * @returns The size of the compressed header in bytes or -1 if decompression fails.
     *
     */
    int DecompressBaseHeader(Ip6::Header &       aIp6Header,
                             bool &              aCompressedNextHeader,
                             const Mac::Address &aMacSource,
                             const Mac::Address &aMacDest,
                             const uint8_t *     aBuf,
                             uint16_t            aBufLength);

    /**
     * This method decompresses a LOWPAN_NHC UDP header.
     *
     * @param[out]  aUdpHeader    A reference where the UDP header will be placed.
     * @param[in]   aBuf          A pointer to the LOWPAN_NHC header.
     * @param[in]   aBufLength    The number of bytes in @p aBuf.
     *
     * @returns The size of the compressed header in bytes or -1 if decompression fails.
     *
     */
    int DecompressUdpHeader(Ip6::Udp::Header &aUdpHeader, const uint8_t *aBuf, uint16_t aBufLength);

private:
    enum
    {
        kHcDispatch     = 3 << 13,
        kHcDispatchMask = 7 << 13,

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

        kExtHdrEidHbh      = 0x00,
        kExtHdrEidRouting  = 0x02,
        kExtHdrEidFragment = 0x04,
        kExtHdrEidDst      = 0x06,
        kExtHdrEidMobility = 0x08,
        kExtHdrEidIp6      = 0x0e,
        kExtHdrEidMask     = 0x0e,

        kExtHdrNextHeader = 0x01,
        kExtHdrMaxLength  = 255,

        kUdpDispatch     = 0xf0,
        kUdpDispatchMask = 0xf8,
        kUdpChecksum     = 1 << 2,
        kUdpPortMask     = 3 << 0,
    };

    otError Compress(Message &           aMessage,
                     const Mac::Address &aMacSource,
                     const Mac::Address &aMacDest,
                     BufferWriter &      aBuf,
                     uint8_t &           aHeaderDepth);

    otError CompressExtensionHeader(Message &aMessage, BufferWriter &aBuf, uint8_t &aNextHeader);
    otError CompressSourceIid(const Mac::Address &aMacAddr,
                              const Ip6::Address &aIpAddr,
                              const Context &     aContext,
                              uint16_t &          aHcCtl,
                              BufferWriter &      aBuf);
    otError CompressDestinationIid(const Mac::Address &aMacAddr,
                                   const Ip6::Address &aIpAddr,
                                   const Context &     aContext,
                                   uint16_t &          aHcCtl,
                                   BufferWriter &      aBuf);
    otError CompressMulticast(const Ip6::Address &aIpAddr, uint16_t &aHcCtl, BufferWriter &aBuf);
    otError CompressUdp(Message &aMessage, BufferWriter &aBuf);

    int     DecompressExtensionHeader(Message &aMessage, const uint8_t *aBuf, uint16_t aBufLength);
    int     DecompressUdpHeader(Message &aMessage, const uint8_t *aBuf, uint16_t aBufLength, uint16_t aDatagramLength);
    otError DispatchToNextHeader(uint8_t aDispatch, uint8_t &aNextHeader);

    static void    CopyContext(const Context &aContext, Ip6::Address &aAddress);
    static otError ComputeIid(const Mac::Address &aMacAddr, const Context &aContext, Ip6::Address &aIpAddress);
};

/**
 * This class implements Mesh Header generation and processing.
 *
 */
class MeshHeader
{
public:
    enum
    {
        kAdditionalHopsLeft = 1 ///< The additional value that is added to predicted value of the route cost.
    };

    /**
     * This method initializes the Mesh Header with a given Mesh Source, Mesh Destination and Hops Left value.
     *
     * @param[in]  aSource       The Mesh Source address.
     * @param[in]  aDestination  The Mesh Destination address.
     * @param[in]  aHopsLeft     The Hops Left value.
     *
     */
    void Init(uint16_t aSource, uint16_t aDestination, uint8_t aHopsLeft);

    /**
     * This static method indicates whether or not the header (in a given frame) is a Mesh Header.
     *
     * @note This method checks whether the first byte in header/frame (dispatch byte) matches the Mesh Header dispatch
     * It does not fully parse and validate the Mesh Header. `ParseFrom()` method can be used to fully parse and
     * validate the header.
     *
     * @retval TRUE   If the header matches the Mesh Header dispatch value.
     * @retval FALSE  If the header does not match the Mesh Header dispatch value.
     *
     */
    static bool IsMeshHeader(const uint8_t *aFrame, uint16_t aFrameLength);

    /**
     * This method parses the Mesh Header from a frame @p aFrame.
     *
     * @param[in]  aFrame        The pointer to the frame.
     * @param[in]  aFrameLength  The length of the frame.
     * @param[out] aHeaderLength A reference to a variable to output the parsed header length (on success).
     *
     * @retval OT_ERROR_NONE     Mesh Header parsed successfully.
     * @retval OT_ERROR_PARSE    Mesh Header could not be parsed.
     *
     */
    otError ParseFrom(const uint8_t *aFrame, uint16_t aFrameLength, uint16_t &aHeaderLength);

    /**
     * This method parses the Mesh Header from a given message.
     *
     * @note The Mesh Header is read from offset zero within the @p aMessage.
     *
     * @param[in]  aMessage    The message to read from.
     *
     * @retval OT_ERROR_NONE   Mesh Header parsed successfully.
     * @retval OT_ERROR_PARSE  Mesh Header could not be parsed.
     *
     */
    otError ParseFrom(const Message &aMessage);

    /**
     * This method parses the Mesh Header from a given message.
     *
     * @note The Mesh Header is read from offset zero within the @p aMessage.
     *
     * @param[in]  aMessage       The message to read from.
     * @param[out] aHeaderLength  A reference to a variable to output the parsed header length (on success).
     *
     * @retval OT_ERROR_NONE   Mesh Header parsed successfully.
     * @retval OT_ERROR_PARSE  Mesh Header could not be parsed.
     *
     */
    otError ParseFrom(const Message &aMessage, uint16_t &aHeaderLength);

    /**
     * This method returns the the Mesh Header length when written to a frame.
     *
     * @note The returned value from this method gives the header length (number of bytes) when the header is written
     * to a frame or message. This should not be used to determine the parsed length (number of bytes read) when the
     * Mesh Header is parsed from a frame/message (using `ParseFrom()` methods).
     *
     * @returns The length of the Mesh Header (in bytes) when written to a frame.
     *
     */
    uint16_t GetHeaderLength(void) const;

    /**
     * This method returns the Hops Left value.
     *
     * @returns The Hops Left value.
     *
     */
    uint8_t GetHopsLeft(void) const { return mHopsLeft; }

    /**
     * This method decrements the Hops Left value (if it is not zero).
     *
     */
    void DecrementHopsLeft(void);

    /**
     * This method returns the Mesh Source address.
     *
     * @returns The Mesh Source address.
     *
     */
    uint16_t GetSource(void) const { return mSource; }

    /**
     * This method returns the Mesh Destination address.
     *
     * @returns The Mesh Destination address.
     *
     */
    uint16_t GetDestination(void) const { return mDestination; }

    /**
     * This method writes the Mesh Header into a given frame.
     *
     * @note This method expects the frame buffer to have enough space for the entire Mesh Header.
     *
     * @param[out]  aFrame  The pointer to the frame buffer to write to.
     *
     * @returns The header length (number of bytes written).
     *
     */
    uint16_t WriteTo(uint8_t *aFrame) const;

    /**
     * This method writes the Mesh Header to a message at a given offset.
     *
     * @note This method expects the @p aMessage length to be already set such that there is enough space for the
     * entire Mesh Header to be written.
     *
     * @param[out] aMessage  A message to write the Mesh Header into.
     * @param[in]  aOffset   The offset at which to write the header.
     *
     * @returns The header length (number of bytes written).
     *
     */
    uint16_t WriteTo(Message &aMessage, uint16_t aOffset) const;

private:
    enum
    {
        kDispatch             = 2 << 6,
        kDispatchMask         = 3 << 6,
        kHopsLeftMask         = 0x0f,
        kSourceShort          = 1 << 5,
        kDestShort            = 1 << 4,
        kDeepHopsLeft         = 0x0f,
        kMinHeaderLength      = sizeof(uint8_t) + sizeof(uint16_t) + sizeof(uint16_t), // dispatch byte + src + dest
        kDeepHopsHeaderLength = kMinHeaderLength + sizeof(uint8_t),                    // min header + deep hops
    };

    uint16_t mSource;
    uint16_t mDestination;
    uint8_t  mHopsLeft;
};

/**
 * This class implements Fragment Header generation and parsing.
 *
 */
class FragmentHeader
{
public:
    enum
    {
        kFirstFragmentHeaderSize      = 4, ///< First fragment header size in octets.
        kSubsequentFragmentHeaderSize = 5, ///< Subsequent fragment header size in octets.
    };

    /**
     * This method initializes the Fragment Header as a first fragment.
     *
     * A first fragment header starts at offset zero.
     *
     * @param[in] aSize   The Datagram Size value.
     * @param[in] aTag    The Datagram Tag value.
     *
     */
    void InitFirstFragment(uint16_t aSize, uint16_t aTag) { Init(aSize, aTag, 0); }

    /**
     * This method initializes the Fragment Header.
     *
     * The @p aOffset value will be truncated to become a multiple of 8.
     *
     * @param[in] aSize   The Datagram Size value.
     * @param[in] aTag    The Datagram Tag value.
     * @param[in] aOffset The Datagram Offset value.
     *
     */
    void Init(uint16_t aSize, uint16_t aTag, uint16_t aOffset);

    /**
     * This static method indicates whether or not the header (in a given frame) is a Fragment Header.
     *
     * @note This method checks whether the frame has the minimum required length and that the first byte in
     * header (dispatch byte) matches the Fragment Header dispatch value. It does not fully parse and validate the
     * Fragment Header. `ParseFrom()` method can be used to fully parse and validate the header.
     *
     * @retval TRUE   If the header matches the Fragment Header dispatch value.
     * @retval FALSE  If the header does not match the Fragment Header dispatch value.
     *
     */
    static bool IsFragmentHeader(const uint8_t *aFrame, uint16_t aFrameLength);

    /**
     * This method parses the Fragment Header from a frame @p aFrame.
     *
     * @param[in]  aFrame          The pointer to the frame.
     * @param[in]  aFrameLength    The length of the frame.
     * @param[out] aHeaderLength   A reference to a variable to output the parsed header length (on success).
     *
     * @retval OT_ERROR_NONE     Fragment Header parsed successfully.
     * @retval OT_ERROR_PARSE    Fragment header could not be parsed from @p aFrame.
     *
     */
    otError ParseFrom(const uint8_t *aFrame, uint16_t aFrameLength, uint16_t &aHeaderLength);

    /**
     * This method parses the Fragment Header from a message.
     *
     * @param[in]  aMessage      The message to read from.
     * @param[in]  aOffset       The offset within the message to start reading from.
     * @param[out] aHeaderLength A reference to a variable to output the parsed header length (on success).
     *
     * @retval OT_ERROR_NONE     Fragment Header parsed successfully.
     * @retval OT_ERROR_PARSE    Fragment header could not be parsed from @p aFrame.
     *
     */
    otError ParseFrom(const Message &aMessage, uint16_t aOffset, uint16_t &aHeaderLength);

    /**
     * This method returns the Datagram Size value.
     *
     * @returns The Datagram Size value.
     *
     */
    uint16_t GetDatagramSize(void) const { return mSize; }

    /**
     * This method returns the Datagram Tag value.
     *
     * @returns The Datagram Tag value.
     *
     */
    uint16_t GetDatagramTag(void) const { return mTag; }

    /**
     * This method returns the Datagram Offset value.
     *
     * The returned offset value is always multiple of 8.
     *
     * @returns The Datagram Offset value (multiple of 8).
     *
     */
    uint16_t GetDatagramOffset(void) const { return mOffset; }

    /**
     * This method writes the Fragment Header into a given frame.
     *
     * @note This method expects the frame buffer to have enough space for the entire Fragment Header
     *
     * @param[out]  aFrame  The pointer to the frame buffer to write to.
     *
     * @returns The header length (number of bytes written).
     *
     */
    uint16_t WriteTo(uint8_t *aFrame) const;

private:
    enum
    {
        kDispatch     = 0xc0,   // 0b1100_0000
        kDispatchMask = 0xd8,   // 0b1101_1000 which accepts first frag (0b1100_0xxx) and next frag (0b1110_0xxx).
        kOffsetFlag   = 1 << 5, // Dispatch flag to indicate first (no offset) vs. next (offset is present) fragment.
        kSizeMask     = 0x7ff,  // 0b0111_1111_1111 (first 11 bits).
        kOffsetMask   = 0xfff8, // Clears the last 3 bits to ensure offset is a multiple of 8.
        kSizeIndex    = 0,      // Start index of Size field in the Fragment Header byte sequence.
        kTagIndex     = 2,      // Start index of Tag field in the Fragment Header byte sequence.
        kOffsetIndex  = 4,      // Start index of Offset field in the Fragment Header byte sequence.
    };

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
