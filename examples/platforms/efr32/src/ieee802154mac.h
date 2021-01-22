/*
 *  Copyright (c) 2020, The OpenThread Authors.
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
 *   This file holds required 802.15.4 definitions
 *
 */

#ifndef __IEEE802154IEEE802154_H__
#define __IEEE802154IEEE802154_H__

#define IEEE802154_MIN_LENGTH \
    4 // Technically, a version 2 packet / ACK
      // can be 4 bytes with seq# suppression
#define IEEE802154_MAX_LENGTH 127
#define IEEE802154_ACK_LENGTH 5

// FCF + DSN + dest PANID + dest addr + src PANID + src addr (without security header)
#define IEEE802154_MAX_MHR_LENGTH (2 + 1 + 2 + 8 + 2 + 8)

#define IEEE802154_DSN_OFFSET 2
#define IEEE802154_FCF_OFFSET 0

//------------------------------------------------------------------------
// 802.15.4 Frame Control Field definitions for Beacon, Ack, Data, Command

#define IEEE802154_FRAME_TYPE_MASK ((uint16_t)0x0007U)              // Bits 0..2
#define IEEE802154_FRAME_TYPE_BEACON ((uint16_t)0x0000U)            // Beacon
#define IEEE802154_FRAME_TYPE_DATA ((uint16_t)0x0001U)              // Data
#define IEEE802154_FRAME_TYPE_ACK ((uint16_t)0x0002U)               // ACK
#define IEEE802154_FRAME_TYPE_COMMAND ((uint16_t)0x0003U)           // Command
#define IEEE802154_FRAME_TYPE_CONTROL IEEE802154_FRAME_TYPE_COMMAND // (synonym)
#define IEEE802154_FRAME_TYPE_RESERVED_MASK ((uint16_t)0x0004U)     // Versions 0/1
// 802.15.4E-2012 introduced MultiPurpose with different Frame Control Field
// layout described in the MultiPurpose section below.
#define IEEE802154_FRAME_TYPE_MULTIPURPOSE ((uint16_t)0x0005U) // MultiPurpose

#define IEEE802154_FRAME_FLAG_SECURITY_ENABLED ((uint16_t)0x0008U) // Bit 3
#define IEEE802154_FRAME_FLAG_FRAME_PENDING ((uint16_t)0x0010U)    // Bit 4
#define IEEE802154_FRAME_FLAG_ACK_REQUIRED ((uint16_t)0x0020U)     // Bit 5
#define IEEE802154_FRAME_FLAG_INTRA_PAN ((uint16_t)0x0040U)        // Bit 6
// 802.15.4-2006 renamed the Intra-Pan flag PanId-Compression
#define IEEE802154_FRAME_FLAG_PANID_COMPRESSION IEEE802154_FRAME_FLAG_INTRA_PAN
#define IEEE802154_FRAME_FLAG_RESERVED ((uint16_t)0x0080U) // Bit 7 reserved
// Use the reserved flag internally to check whether frame pending bit was set in outgoing ACK
#define IEEE802154_FRAME_PENDING_SET_IN_OUTGOING_ACK IEEE802154_FRAME_FLAG_RESERVED
// 802.15.4E-2012 introduced these flags for Frame Version 2 frames
// which are reserved bit positions in earlier Frame Version frames:
#define IEEE802154_FRAME_FLAG_SEQ_SUPPRESSION ((uint16_t)0x0100U) // Bit 8
#define IEEE802154_FRAME_FLAG_IE_LIST_PRESENT ((uint16_t)0x0200U) // Bit 9

#define IEEE802154_FRAME_DESTINATION_MODE_MASK ((uint16_t)0x0C00U)     // Bits 10..11
#define IEEE802154_FRAME_DESTINATION_MODE_NONE ((uint16_t)0x0000U)     // Mode 0
#define IEEE802154_FRAME_DESTINATION_MODE_RESERVED ((uint16_t)0x0400U) // Mode 1
#define IEEE802154_FRAME_DESTINATION_MODE_SHORT ((uint16_t)0x0800U)    // Mode 2
#define IEEE802154_FRAME_DESTINATION_MODE_LONG ((uint16_t)0x0C00U)     // Mode 3
// 802.15.4e-2012 only (not adopted into 802.15.4-2015)
#define IEEE802154_FRAME_DESTINATION_MODE_BYTE IEEE802154_FRAME_DESTINATION_MODE_RESERVED

#define IEEE802154_FRAME_VERSION_MASK ((uint16_t)0x3000U) // Bits 12..13
#define IEEE802154_FRAME_VERSION_2003 ((uint16_t)0x0000U) // Version 0
#define IEEE802154_FRAME_VERSION_2006 ((uint16_t)0x1000U) // Version 1
// In 802.15.4-2015, Version 2 is just called "IEEE STD 802.15.4"
// which can be rather confusing. It was introduced in 802.15.4E-2012.
#define IEEE802154_FRAME_VERSION_2012 ((uint16_t)0x2000U)     // Version 2
#define IEEE802154_FRAME_VERSION_2015 ((uint16_t)0x2000U)     // Version 2
#define IEEE802154_FRAME_VERSION_RESERVED ((uint16_t)0x3000U) // Version 3

#define IEEE802154_FRAME_SOURCE_MODE_MASK ((uint16_t)0xC000U)     // Bits 14..15
#define IEEE802154_FRAME_SOURCE_MODE_NONE ((uint16_t)0x0000U)     // Mode 0
#define IEEE802154_FRAME_SOURCE_MODE_RESERVED ((uint16_t)0x4000U) // Mode 1
#define IEEE802154_FRAME_SOURCE_MODE_SHORT ((uint16_t)0x8000U)    // Mode 2
#define IEEE802154_FRAME_SOURCE_MODE_LONG ((uint16_t)0xC000U)     // Mode 3
// 802.15.4e-2012 only (not adopted into 802.15.4-2015)
#define IEEE802154_FRAME_SOURCE_MODE_BYTE IEEE802154_FRAME_SOURCE_MODE_RESERVED

//------------------------------------------------------------------------
// 802.15.4E-2012 Frame Control Field definitions for MultiPurpose

#define IEEE802154_MP_FRAME_TYPE_MASK IEEE802154_FRAME_TYPE_MASK // Bits 0..2
#define IEEE802154_MP_FRAME_TYPE_MULTIPURPOSE IEEE802154_FRAME_TYPE_MULTIPURPOSE

#define IEEE802154_MP_FRAME_FLAG_LONG_FCF ((uint16_t)0x0008U) // Bit 3

#define IEEE802154_MP_FRAME_DESTINATION_MODE_MASK ((uint16_t)0x0030U)     // Bits 4..5
#define IEEE802154_MP_FRAME_DESTINATION_MODE_NONE ((uint16_t)0x0000U)     // Mode 0
#define IEEE802154_MP_FRAME_DESTINATION_MODE_RESERVED ((uint16_t)0x0010U) // Mode 1
#define IEEE802154_MP_FRAME_DESTINATION_MODE_SHORT ((uint16_t)0x0020U)    // Mode 2
#define IEEE802154_MP_FRAME_DESTINATION_MODE_LONG ((uint16_t)0x0030U)     // Mode 3
// 802.15.4e-2012 only (not adopted into 802.15.4-2015)
#define IEEE802154_MP_FRAME_DESTINATION_MODE_BYTE IEEE802154_MP_FRAME_DESTINATION_MODE_RESERVED

#define IEEE802154_MP_FRAME_SOURCE_MODE_MASK ((uint16_t)0x00C0U)     // Bits 6..7
#define IEEE802154_MP_FRAME_SOURCE_MODE_NONE ((uint16_t)0x0000U)     // Mode 0
#define IEEE802154_MP_FRAME_SOURCE_MODE_RESERVED ((uint16_t)0x0040U) // Mode 1
#define IEEE802154_MP_FRAME_SOURCE_MODE_SHORT ((uint16_t)0x0080U)    // Mode 2
#define IEEE802154_MP_FRAME_SOURCE_MODE_LONG ((uint16_t)0x00C0U)     // Mode 3
// 802.15.4e-2012 only (not adopted into 802.15.4-2015)
#define IEEE802154_MP_FRAME_SOURCE_MODE_BYTE IEEE802154_MP_FRAME_SOURCE_MODE_RESERVED

#define IEEE802154_MP_FRAME_FLAG_PANID_PRESENT ((uint16_t)0x0100U)    // Bit 8
#define IEEE802154_MP_FRAME_FLAG_SECURITY_ENABLED ((uint16_t)0x0200U) // Bit 9
#define IEEE802154_MP_FRAME_FLAG_SEQ_SUPPRESSION ((uint16_t)0x0400U)  // Bit 10
#define IEEE802154_MP_FRAME_FLAG_FRAME_PENDING ((uint16_t)0x0800U)    // Bit 11

#define IEEE802154_MP_FRAME_VERSION_MASK IEEE802154_FRAME_VERSION_MASK // Bits 12..13
#define IEEE802154_MP_FRAME_VERSION_2012 ((uint16_t)0x0000U)           // Zeroed out
#define IEEE802154_MP_FRAME_VERSION_2015 ((uint16_t)0x0000U)           // Zeroed out
// All other MultiPurpose Frame Versions are reserved

#define IEEE802154_MP_FRAME_FLAG_ACK_REQUIRED ((uint16_t)0x4000U)    // Bit 14
#define IEEE802154_MP_FRAME_FLAG_IE_LIST_PRESENT ((uint16_t)0x8000U) // Bit 15

//------------------------------------------------------------------------
// Information Elements fields

#define IEEE802154_KEYID_MODE_0 ((uint8_t)0x0000U)
#define IEEE802154_KEYID_MODE_0_SIZE 0

#define IEEE802154_KEYID_MODE_1 ((uint8_t)0x0008U)
#define IEEE802154_KEYID_MODE_1_SIZE 0

#define IEEE802154_KEYID_MODE_2 ((uint8_t)0x0010U)
#define IEEE802154_KEYID_MODE_2_SIZE 4

#define IEEE802154_KEYID_MODE_3 ((uint8_t)0x0018U)
#define IEEE802154_KEYID_MODE_3_SIZE 8

#define IEEE802154_KEYID_MODE_MASK ((uint8_t)0x0018U)

//------------------------------------------------------------------------
// Information Elements fields

// There are Header IEs and Payload IEs.  Header IEs are authenticated
// if MAC Security is enabled.  Payload IEs are both authenticated and
// encrypted if MAC security is enabled.

// Header and Payload IEs have slightly different formats and different
// contents based on the 802.15.4 spec.

// Both are actually a list of IEs that continues until a termination
// IE is seen.

#define IEEE802154_FRAME_HEADER_INFO_ELEMENT_LENGTH_MASK 0x007F // bits 0-6
#define IEEE802154_FRAME_HEADER_INFO_ELEMENT_ID_MASK 0x7F80     // bits 7-14
#define IEEE802154_FRAME_HEADER_INFO_ELEMENT_TYPE_MASK 0x8000   // bit  15

#define IEEE802154_FRAME_HEADER_INFO_ELEMENT_ID_SHIFT 7

#define IEEE802154_FRAME_PAYLOAD_INFO_ELEMENT_LENGTH_MASK 0x07FF   // bits 0 -10
#define IEEE802154_FRAME_PAYLOAD_INFO_ELEMENT_GROUP_ID_MASK 0x7800 // bits 11-14
#define IEEE802154_FRAME_PAYLOAD_INFO_ELEMENT_TYPE_MASK 0x8000     // bit  15

#define IEEE802154_FRAME_PAYLOAD_INFO_ELEMENT_ID_SHIFT 11

// This "type" field indicates header vs. payload IE.  However there is
// also a Header IE List terminator which would imply the IE list
// that follows is only payload IEs.
#define IEEE802154_FRAME_INFO_ELEMENT_TYPE_MASK 0x8000

// Header Termination ID 1 is used when there are Payload IEs that follow.
// Header Termination ID 2 is used when there are no Payload IEs and the
//   next field is the MAC payload.
#define IEEE802154_FRAME_HEADER_TERMINATION_ID_1 0x7E
#define IEEE802154_FRAME_HEADER_TERMINATION_ID_2 0x7F
#define IEEE802154_FRAME_PAYLOAD_TERMINATION_ID 0x0F

#endif //__IEEE802154IEEE802154_H__
