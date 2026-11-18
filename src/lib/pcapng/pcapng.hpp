/*
 *  Copyright (c) 2026, The OpenThread Authors.
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
 *   This file includes definitions for a PCAPNG block encoder.
 */

#ifndef OT_LIB_PCAPNG_PCAPNG_HPP_
#define OT_LIB_PCAPNG_PCAPNG_HPP_

#include <stdint.h>

#include <openthread/error.h>
#include <openthread/platform/radio.h>

namespace ot {

/**
 * @namespace ot::Pcapng
 *
 * @brief
 *   This namespace includes definitions for the PCAPNG block encoder.
 */
namespace Pcapng {

/**
 * Represents a link type of an interface (`LINKTYPE_*` values from the tcpdump link-layer header types registry).
 */
enum LinkType : uint16_t
{
    kLinkTypeEthernet      = 1,   ///< Ethernet II frames.
    kLinkTypeIeee802154Tap = 283, ///< IEEE 802.15.4 frames with a TAP header (channel, FCS type, RSSI, LQI).
};

/**
 * Specifies how the FCS of an IEEE 802.15.4 frame is encoded.
 */
enum FcsMode : uint8_t
{
    kFcsAsIs,      ///< Copy the FCS from the PSDU.
    kFcsRecompute, ///< Recompute the FCS, for radios which overwrite the FCS with metadata or leave it uninitialized.
};

/**
 * Represents an Ethernet II header.
 */
struct EthernetHeader
{
    static constexpr uint8_t kAddressSize = 6; ///< Size of an Ethernet MAC address.

    uint8_t  mDestination[kAddressSize]; ///< The destination MAC address.
    uint8_t  mSource[kAddressSize];      ///< The source MAC address.
    uint16_t mEtherType;                 ///< The EtherType (in host byte order).
};

/**
 * Returns the size of an Enhanced Packet Block carrying a packet of a given length.
 *
 * @param[in] aPacketLength  The packet length in bytes.
 *
 * @returns The size of the Enhanced Packet Block in bytes.
 */
constexpr uint32_t GetEnhancedPacketBlockSize(uint32_t aPacketLength)
{
    // Block type, block total length, interface ID, timestamp (2 words), captured and original packet lengths,
    // the packet padded to 32 bits, and the trailing block total length.
    return 7 * sizeof(uint32_t) + ((aPacketLength + 3) & ~3u) + sizeof(uint32_t);
}

/**
 * Encodes PCAPNG blocks into a caller-provided buffer.
 *
 * The encoder performs no I/O. Each `Append*()` method either appends a complete block to the buffer, or leaves the
 * buffer unchanged and returns an error, so callers can write the encoded blocks in a single write.
 *
 * All multi-byte values are encoded in little endian, matching the byte-order magic of the Section Header Block.
 */
class Encoder
{
public:
    static constexpr uint32_t kSnapLength = 65535; ///< Maximum packet length (snap length) of every interface.

    static constexpr uint32_t kSectionHeaderBlockSize        = 28; ///< Size of a Section Header Block.
    static constexpr uint32_t kInterfaceDescriptionBlockSize = 20; ///< Size of an Interface Description Block.

    static constexpr uint16_t kEthernetHeaderSize = 14; ///< Size of an Ethernet II header.

    /**
     * Maximum size of the TAP header of an IEEE 802.15.4 frame (with Channel, FCS type, RSSI, and LQI TLVs).
     */
    static constexpr uint16_t kMaxIeee802154TapHeaderSize = 36;

    /**
     * Maximum size of an Enhanced Packet Block carrying an IEEE 802.15.4 frame.
     */
    static constexpr uint32_t kMaxIeee802154FrameBlockSize =
        GetEnhancedPacketBlockSize(kMaxIeee802154TapHeaderSize + OT_RADIO_FRAME_MAX_SIZE);

    /**
     * Maximum size of an Enhanced Packet Block carrying an Ethernet frame.
     */
    static constexpr uint32_t kMaxEthernetFrameBlockSize = GetEnhancedPacketBlockSize(kSnapLength);

    /**
     * Initializes the encoder.
     *
     * @param[in] aBuffer      A pointer to the buffer to encode blocks into.
     * @param[in] aBufferSize  The size of @p aBuffer in bytes.
     */
    Encoder(uint8_t *aBuffer, uint32_t aBufferSize);

    /**
     * Gets the number of bytes encoded into the buffer.
     *
     * @returns The number of encoded bytes.
     */
    uint32_t GetLength(void) const { return mLength; }

    /**
     * Appends a Section Header Block.
     *
     * The section length is left unspecified, so the section may hold any number of blocks.
     *
     * @retval OT_ERROR_NONE     Successfully appended the block.
     * @retval OT_ERROR_NO_BUFS  Insufficient buffer space.
     */
    otError AppendSectionHeader(void);

    /**
     * Appends an Interface Description Block.
     *
     * Interfaces are numbered in the order of their Interface Description Blocks within a section, starting at 0.
     *
     * @param[in] aLinkType  The link type of the interface.
     *
     * @retval OT_ERROR_NONE     Successfully appended the block.
     * @retval OT_ERROR_NO_BUFS  Insufficient buffer space.
     */
    otError AppendInterfaceDescription(LinkType aLinkType);

    /**
     * Appends an Enhanced Packet Block carrying an IEEE 802.15.4 frame for a `kLinkTypeIeee802154Tap` interface.
     *
     * Only `mPsdu`, `mLength`, and `mChannel` are read from @p aFrame. The RSSI and LQI are passed explicitly since
     * `mInfo` of a transmitted frame does not hold them.
     *
     * @param[in] aInterfaceId  The interface ID.
     * @param[in] aTimeUs       The capture timestamp in microseconds.
     * @param[in] aFrame        The frame (`mLength` includes the FCS).
     * @param[in] aRssi         The RSSI in dBm, or `OT_RADIO_RSSI_INVALID` to omit the RSSI TLV.
     * @param[in] aLqi          The LQI, or `OT_RADIO_LQI_NONE` to omit the LQI TLV.
     * @param[in] aFcsMode      How to encode the FCS.
     *
     * @retval OT_ERROR_NONE          Successfully appended the block.
     * @retval OT_ERROR_INVALID_ARGS  The frame is invalid.
     * @retval OT_ERROR_NO_BUFS       Insufficient buffer space.
     */
    otError AppendIeee802154Frame(uint32_t            aInterfaceId,
                                  uint64_t            aTimeUs,
                                  const otRadioFrame &aFrame,
                                  int8_t              aRssi,
                                  uint8_t             aLqi,
                                  FcsMode             aFcsMode);

    /**
     * Appends an Enhanced Packet Block carrying an Ethernet frame for a `kLinkTypeEthernet` interface.
     *
     * @param[in] aInterfaceId     The interface ID.
     * @param[in] aTimeUs          The capture timestamp in microseconds.
     * @param[in] aHeader          The Ethernet header.
     * @param[in] aPayload         A pointer to the payload.
     * @param[in] aPayloadLength   The payload length in bytes.
     *
     * @retval OT_ERROR_NONE          Successfully appended the block.
     * @retval OT_ERROR_INVALID_ARGS  The frame is longer than `kSnapLength`, or @p aPayload is null.
     * @retval OT_ERROR_NO_BUFS       Insufficient buffer space.
     */
    otError AppendEthernetFrame(uint32_t              aInterfaceId,
                                uint64_t              aTimeUs,
                                const EthernetHeader &aHeader,
                                const uint8_t        *aPayload,
                                uint16_t              aPayloadLength);

private:
    uint8_t *BeginEnhancedPacket(uint32_t aInterfaceId, uint64_t aTimeUs, uint32_t aPacketLength);
    void     EndBlock(uint8_t *aCursor);

    uint8_t *mBuffer;
    uint32_t mBufferSize;
    uint32_t mLength;
};

} // namespace Pcapng
} // namespace ot

#endif // OT_LIB_PCAPNG_PCAPNG_HPP_
