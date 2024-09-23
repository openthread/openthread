/*
 *    Copyright (c) 2023, The OpenThread Authors.
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
 *   This file contains definitions of the SPI frame.
 */

#ifndef SPINEL_SPI_FRAME_HPP_
#define SPINEL_SPI_FRAME_HPP_

#include <stdint.h>

#include "lib/utils/endian.hpp"

namespace ot {
namespace Spinel {

/*
 *   SPI Framing Protocol
 *
 *   Each SPI frame starts with a 5-byte frame header:
 *
 *                  +---------+-----+----------+----------+
 *                  | Octets: |  1  |    2     |    2     |
 *                  +---------+-----+----------+----------+
 *                  | Fields: | HDR | RECV_LEN | DATA_LEN |
 *                  +---------+-----+----------+----------+
 *
 *   -  "HDR": The first byte is the header byte (defined below)
 *   -  "RECV_LEN": The second and third bytes indicate the largest frame
 *      size that that device is ready to receive.  If zero, then the
 *      other device must not send any data. (Little endian)
 *   -  "DATA_LEN": The fourth and fifth bytes indicate the size of the
 *      pending data frame to be sent to the other device.  If this value
 *      is equal-to or less-than the number of bytes that the other device
 *      is willing to receive, then the data of the frame is immediately
 *      after the header. (Little Endian)
 *
 *   The "HDR" byte is defined as:
 *
 *                       0   1   2   3   4   5   6   7
 *                     +---+---+---+---+---+---+---+---+
 *                     |RST|CRC|CCF|  RESERVED |PATTERN|
 *                     +---+---+---+---+---+---+---+---+
 *
 *   -  "RST": This bit is set when that device has been reset since the
 *      last time `CS` (chip select) was asserted.
 *   -  "CRC": This bit is set when that device supports writing a 16-bit
 *      CRC at the end of the data.  The CRC length is NOT included in
 *      DATA_LEN.
 *   -  "CCF": "CRC Check Failure".  Set if the CRC check on the last
 *      received frame failed, cleared to zero otherwise.  This bit is
 *      only used if both sides support CRC.
 *   -  "RESERVED": These bits are all reserved for future used.  They
 *      MUST be cleared to zero and MUST be ignored if set.
 *   -  "PATTERN": These bits are set to a fixed value to help distinguish
 *      valid SPI frames from garbage (by explicitly making "0xFF" and
 *      "0x00" invalid values).  Bit 6 MUST be set to be one and bit 7
 *      MUST be cleared (0).  A frame received that has any other values
 *      for these bits MUST be dropped.
 *
 *   Prior to a sending or receiving a frame, the master MAY send a
 *   5-octet frame with zeros for both the max receive frame size and the
 *   the contained frame length.  This will induce the slave device to
 *   indicate the length of the frame it wants to send (if any) and
 *   indicate the largest frame it is capable of receiving at the moment.
 *   This allows the master to calculate the size of the next transaction.
 *   Alternatively, if the master has a frame to send it can just go ahead
 *   and send a frame of that length and determine if the frame was
 *   accepted by checking that the "RECV_LEN" from the slave frame is
 *   larger than the frame the master just tried to send.  If the
 *   "RECV_LEN" is smaller then the frame wasn't accepted and will need to
 *   be transmitted again.
 *
 *   This protocol can be used either unidirectionally or bidirectionally,
 *   determined by the behavior of the master and the slave.
 *
 *   If the the master notices "PATTERN" is not set correctly, the master
 *   should consider the transaction to have failed and try again after 10
 *   milliseconds, retrying up to 200 times.  After unsuccessfully trying
 *   200 times in a row, the master MAY take appropriate remedial action
 *   (like a NCP hardware reset, or indicating a communication failure to
 *   a user interface).
 *
 *   At the end of the data of a frame is an optional 16-bit CRC, support
 *   for which is indicated by the "CRC" bit of the "HDR" byte being set.
 *   If these bits are set for both the master and slave frames, then CRC
 *   checking is enabled on both sides, effectively requiring that frame
 *   sizes be two bytes longer than would be otherwise required.  The CRC
 *   is calculated using the same mechanism used for the CRC calculation
 *   in HDLC-Lite (See Appendix A.1.2).  When both of the "CRC" bits are
 *   set, both sides must verify that the "CRC" is valid before accepting
 *   the frame.  If not enough bytes were clocked out for the CRC to be
 *   read, then the frame must be ignored.  If enough bytes were clocked
 *   out to perform a CRC check, but the CRC check fails, then the frame
 *   must be rejected and the "CRC_FAIL" bit on the next frame (and ONLY
 *   the next frame) MUST be set.
 */

/**
 * Defines a SPI frame.
 */
class SpiFrame
{
public:
    enum
    {
        kHeaderSize = 5, ///< SPI header size (in bytes).
    };

    /**
     * Initializes an `SpiFrame` instance.
     *
     * @param[in] aBuffer     Pointer to buffer containing the frame.
     */
    explicit SpiFrame(uint8_t *aBuffer)
        : mBuffer(aBuffer)
    {
    }

    /**
     * Gets a pointer to data portion in the SPI frame skipping the header.
     *
     * @returns  A pointer to data in the SPI frame.
     */
    uint8_t *GetData(void) { return mBuffer + kHeaderSize; }

    /**
     * Indicates whether or not the frame is valid.
     *
     * In a valid frame the flag byte should contain the pattern bits.
     *
     * @returns TRUE if the frame is valid, FALSE otherwise.
     */
    bool IsValid(void) const { return ((mBuffer[kIndexFlagByte] & kFlagPatternMask) == kFlagPattern); }

    /**
     * Indicates whether or not the "RST" bit is set.
     *
     * @returns TRUE if the "RST" bit is set, FALSE otherwise.
     */
    bool IsResetFlagSet(void) const { return ((mBuffer[kIndexFlagByte] & kFlagReset) == kFlagReset); }

    /**
     * Sets the "flag byte" field in the SPI frame header.
     *
     * @param[in] aResetFlag     The status of reset flag (TRUE to set the flag, FALSE to clear flag).
     */
    void SetHeaderFlagByte(bool aResetFlag) { mBuffer[kIndexFlagByte] = kFlagPattern | (aResetFlag ? kFlagReset : 0); }

    /**
     * Gets the "flag byte" field in the SPI frame header.
     *
     * @returns  The flag byte.
     */
    uint8_t GetHeaderFlagByte(void) const { return mBuffer[kIndexFlagByte]; }

    /**
     * Sets the "accept len" field in the SPI frame header.
     *
     * "accept len" specifies number of bytes the sender of the SPI frame can receive.
     *
     * @param[in] aAcceptLen    The accept length in bytes.
     */
    void SetHeaderAcceptLen(uint16_t aAcceptLen)
    {
        Lib::Utils::LittleEndian::WriteUint16(aAcceptLen, mBuffer + kIndexAcceptLen);
    }

    /**
     * Gets the "accept len" field in the SPI frame header.
     *
     * @returns  The accept length in bytes.
     */
    uint16_t GetHeaderAcceptLen(void) const { return Lib::Utils::LittleEndian::ReadUint16(mBuffer + kIndexAcceptLen); }

    /**
     * Sets the "data len" field in the SPI frame header.
     *
     * "Data len" specifies number of data bytes in the transmitted SPI frame.
     *
     * @param[in] aDataLen    The data length in bytes.
     */
    void SetHeaderDataLen(uint16_t aDataLen)
    {
        Lib::Utils::LittleEndian::WriteUint16(aDataLen, mBuffer + kIndexDataLen);
    }

    /**
     * Gets the "data len" field in the SPI frame header.
     *
     * @returns  The data length in bytes.
     */
    uint16_t GetHeaderDataLen(void) const { return Lib::Utils::LittleEndian::ReadUint16(mBuffer + kIndexDataLen); }

private:
    enum
    {
        kIndexFlagByte  = 0, // flag byte  (uint8_t).
        kIndexAcceptLen = 1, // accept len (uint16_t little-endian encoding).
        kIndexDataLen   = 3, // data len   (uint16_t little-endian encoding).

        kFlagReset       = (1 << 7), // Flag byte RESET bit.
        kFlagPattern     = 0x02,     // Flag byte PATTERN bits.
        kFlagPatternMask = 0x03,     // Flag byte PATTERN mask.
    };

    uint8_t *mBuffer;
};

} // namespace Spinel
} // namespace ot
#endif // SPINEL_SPI_FRAME_HPP_
