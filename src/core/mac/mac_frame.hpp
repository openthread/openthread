/*
 *  Copyright (c) 2016, Nest Labs, Inc.
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
 *   This file includes definitions for generating and processing IEEE 802.15.4 MAC frames.
 */

#ifndef MAC_FRAME_HPP_
#define MAC_FRAME_HPP_

#include <limits.h>
#include <stdint.h>
#include <string.h>

#include <common/encoding.hpp>
#include <openthread-types.h>
#include <platform/radio.h>

namespace Thread {

namespace Ip6 { class Address; }

namespace Mac {

/**
 * @addtogroup core-mac
 *
 * @{
 *
 */

enum
{
    kShortAddrBroadcast = 0xffff,
    kShortAddrInvalid = 0xfffe,
    kPanIdBroadcast = 0xffff,
};

/**
 * This type represents the IEEE 802.15.4 PAN ID.
 *
 */
typedef otPanId PanId;

/**
 * This type represents the IEEE 802.15.4 Short Address.
 *
 */
typedef otShortAddress ShortAddress;

/**
 * This structure represents an IEEE 802.15.4 Extended Address.
 *
 */
class ExtAddress: public otExtAddress
{
public:
    /**
     * This method indicates whether or not the Group bit is set.
     *
     * @retval TRUE   If the group bit is set.
     * @retval FALSE  If the group bit is not set.
     *
     */
    bool IsGroup(void) const { return (m8[0] & kGroupFlag) != 0; }

    /**
     * This method sets the Group bit.
     *
     * @param[in]  aLocal  TRUE if group address, FALSE otherwise.
     *
     */
    void SetGroup(bool aGroup) {
        if (aGroup) {
            m8[0] |= kGroupFlag;
        }
        else {
            m8[0] &= ~kGroupFlag;
        }
    }

    /**
     * This method indicates whether or not the Local bit is set.
     *
     * @retval TRUE   If the local bit is set.
     * @retval FALSE  If the local bit is not set.
     *
     */
    bool IsLocal(void) const { return (m8[0] & kLocalFlag) != 0; }

    /**
     * This method sets the Local bit.
     *
     * @param[in]  aLocal  TRUE if locally administered, FALSE otherwise.
     *
     */
    void SetLocal(bool aLocal) {
        if (aLocal) {
            m8[0] |= kLocalFlag;
        }
        else {
            m8[0] &= ~kLocalFlag;
        }
    }

    /**
     * This method converts an IPv6 Interface Identifier to an IEEE 802.15.4 Extended Address.
     *
     * @param[in]  aIpAddress  A reference to the IPv6 address.
     *
     */
    void Set(const Ip6::Address &aIpAddress);

private:
    enum
    {
        kGroupFlag = 1 << 0,
        kLocalFlag = 1 << 1,
    };
};

/**
 * This structure represents an IEEE 802.15.4 Short or Extended Address.
 *
 */
struct Address
{
    uint8_t mLength;                 ///< Length of address in bytes.
    union
    {
        ShortAddress mShortAddress;  ///< The IEEE 802.15.4 Short Address.
        ExtAddress mExtAddress;      ///< The IEEE 802.15.4 Extended Address.
    };
};

/**
 * This class implements IEEE 802.15.4 MAC frame generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class Frame: public RadioPacket
{
public:
    enum
    {
        kMTU                  = 127,
        kFcfSize              = sizeof(uint16_t),
        kDsnSize              = sizeof(uint8_t),
        kSecurityControlSize  = sizeof(uint8_t),
        kFrameCounterSize     = sizeof(uint32_t),
        kCommandIdSize        = sizeof(uint8_t),
        kFcsSize              = sizeof(uint16_t),

        kFcfFrameBeacon       = 0 << 0,
        kFcfFrameData         = 1 << 0,
        kFcfFrameAck          = 2 << 0,
        kFcfFrameMacCmd       = 3 << 0,
        kFcfFrameTypeMask     = 7 << 0,
        kFcfSecurityEnabled   = 1 << 3,
        kFcfFramePending      = 1 << 4,
        kFcfAckRequest        = 1 << 5,
        kFcfPanidCompression  = 1 << 6,
        kFcfDstAddrNone       = 0 << 10,
        kFcfDstAddrShort      = 2 << 10,
        kFcfDstAddrExt        = 3 << 10,
        kFcfDstAddrMask       = 3 << 10,
        kFcfFrameVersion2006  = 1 << 12,
        kFcfFrameVersionMask  = 3 << 13,
        kFcfSrcAddrNone       = 0 << 14,
        kFcfSrcAddrShort      = 2 << 14,
        kFcfSrcAddrExt        = 3 << 14,
        kFcfSrcAddrMask       = 3 << 14,

        kSecNone              = 0 << 0,
        kSecMic32             = 1 << 0,
        kSecMic64             = 2 << 0,
        kSecMic128            = 3 << 0,
        kSecEnc               = 4 << 0,
        kSecEncMic32          = 5 << 0,
        kSecEncMic64          = 6 << 0,
        kSecEncMic128         = 7 << 0,
        kSecLevelMask         = 7 << 0,

        kMic0Size             = 0,
        kMic32Size            = 32 / CHAR_BIT,
        kMic64Size            = 64 / CHAR_BIT,
        kMic128Size           = 128 / CHAR_BIT,
        kMaxMicSize           = kMic128Size,

        kKeyIdMode0           = 0 << 3,
        kKeyIdMode1           = 1 << 3,
        kKeyIdMode2           = 2 << 3,
        kKeyIdMode3           = 3 << 3,
        kKeyIdModeMask        = 3 << 3,

        kMacCmdAssociationRequest          = 1,
        kMacCmdAssociationResponse         = 2,
        kMacCmdDisassociationNotification  = 3,
        kMacCmdDataRequest                 = 4,
        kMacCmdPanidConflictNotification   = 5,
        kMacCmdOrphanNotification          = 6,
        kMacCmdBeaconRequest               = 7,
        kMacCmdCoordinatorRealignment      = 8,
        kMacCmdGtsRequest                  = 9,
    };

    /**
     * This method initializes the MAC header.
     *
     * @param[in]  aFcf     The Frame Control field.
     * @param[in]  aSecCtl  The Security Control field.
     *
     * @retval kThreadError_None         Successfully initialized the MAC header.
     * @retval kThreadError_InvalidArgs  Invalid values for @p aFcf and/or @p aSecCtl.
     *
     */
    ThreadError InitMacHeader(uint16_t aFcf, uint8_t aSecCtl);

    /**
     * This method returns the IEEE 802.15.4 Frame Type.
     *
     * @returns The IEEE 802.15.4 Frame Type.
     *
     */
    uint8_t GetType(void);

    /**
     * This method indicates whether or not security is enabled.
     *
     * @retval TRUE   If security is enabled.
     * @retval FALSE  If security is not enabled.
     *
     */
    bool GetSecurityEnabled(void);

    /**
     * This method indicates whether or not the Frame Pending bit is set.
     *
     * @retval TRUE   If the Frame Pending bit is set.
     * @retval FALSE  If the Frame Pending bit is not set.
     *
     */
    bool GetFramePending(void);

    /**
     * This method sets the Frame Pending bit.
     *
     * @param[in]  aFramePending  The Frame Pending bit.
     *
     */
    void SetFramePending(bool aFramePending);

    /**
     * This method indicates whether or not the Ack Request bit is set.
     *
     * @retval TRUE   If the Ack Request bit is set.
     * @retval FALSE  If the Ack Request bit is not set.
     *
     */
    bool GetAckRequest(void);

    /**
     * This method sets the Ack Request bit.
     *
     * @param[in]  aAckRequest  The Ack Request bit.
     *
     */
    void SetAckRequest(bool aAckRequest);

    /**
     * This method returns the Sequence Number value.
     *
     * @returns The Sequence Number value.
     *
     */
    uint8_t GetSequence(void);

    /**
     * This method sets the Sequence Number value.
     *
     * @param[in]  aSequence  The Sequence Number value.
     *
     */
    void SetSequence(uint8_t aSequence);

    /**
     * This method gets the Destination PAN Identifier.
     *
     * @param[out]  aPanId  The Destination PAN Identifier.
     *
     * @retval kThreadError_None   Successfully retrieved the Destination PAN Identifier.
     *
     */
    ThreadError GetDstPanId(PanId &aPanId);

    /**
     * This method sets the Destination PAN Identifier.
     *
     * @param[in]  aPanId  The Destination PAN Identifier.
     *
     * @retval kThreadError_None   Successfully set the Destination PAN Identifier.
     *
     */
    ThreadError SetDstPanId(PanId aPanId);

    /**
     * This method gets the Destination Address.
     *
     * @param[out]  aAddress  The Destination Address.
     *
     * @retval kThreadError_None  Successfully retrieved the Destination Address.
     *
     */
    ThreadError GetDstAddr(Address &aAddress);

    /**
     * This method sets the Destination Address.
     *
     * @param[in]  aShortAddress  The Destination Address.
     *
     * @retval kThreadError_None  Successfully set the Destination Address.
     *
     */
    ThreadError SetDstAddr(ShortAddress aShortAddress);

    /**
     * This method sets the Destination Address.
     *
     * @param[in]  aExtAddress  The Destination Address.
     *
     * @retval kThreadError_None  Successfully set the Destination Address.
     *
     */
    ThreadError SetDstAddr(const ExtAddress &aExtAddress);

    /**
     * This method gets the Source PAN Identifier.
     *
     * @param[out]  aPanId  The Source PAN Identifier.
     *
     * @retval kThreadError_None   Successfully retrieved the Source PAN Identifier.
     *
     */
    ThreadError GetSrcPanId(PanId &aPanId);

    /**
     * This method sets the Source PAN Identifier.
     *
     * @param[in]  aPanId  The Source PAN Identifier.
     *
     * @retval kThreadError_None   Successfully set the Source PAN Identifier.
     *
     */
    ThreadError SetSrcPanId(PanId aPanId);

    /**
     * This method gets the Source Address.
     *
     * @param[out]  aAddress  The Source Address.
     *
     * @retval kThreadError_None  Successfully retrieved the Source Address.
     *
     */
    ThreadError GetSrcAddr(Address &aAddress);

    /**
     * This method gets the Source Address.
     *
     * @param[in]  aShortAddress  The Source Address.
     *
     * @retval kThreadError_None  Successfully set the Source Address.
     *
     */
    ThreadError SetSrcAddr(ShortAddress aShortAddress);

    /**
     * This method gets the Source Address.
     *
     * @param[in]  aExtAddress  The Source Address.
     *
     * @retval kThreadError_None  Successfully set the Source Address.
     *
     */
    ThreadError SetSrcAddr(const ExtAddress &aExtAddress);

    /**
     * This method gets the Security Level Identifier.
     *
     * @param[out]  aSecurityLevel  The Security Level Identifier.
     *
     * @retval kThreadError_None  Successfully retrieved the Security Level Identifier.
     *
     */
    ThreadError GetSecurityLevel(uint8_t &aSecurityLevel);

    /**
     * This method gets the Frame Counter.
     *
     * @param[out]  aFrameCounter  The Frame Counter.
     *
     * @retval kThreadError_None  Successfully retrieved the Frame Counter.
     *
     */
    ThreadError GetFrameCounter(uint32_t &aFrameCounter);

    /**
     * This method sets the Frame Counter.
     *
     * @param[in]  aFrameCounter  The Frame Counter.
     *
     * @retval kThreadError_None  Successfully set the Frame Counter.
     *
     */
    ThreadError SetFrameCounter(uint32_t aFrameCounter);

    /**
     * This method gets the Key Identifier.
     *
     * @param[out]  aKeyId  The Key Identifier.
     *
     * @retval kThreadError_None  Successfully retrieved the Key Identifier.
     *
     */
    ThreadError GetKeyId(uint8_t &aKeyId);

    /**
     * This method sets the Key Identifier.
     *
     * @param[in]  aKeyId  The Key Identifier.
     *
     * @retval kThreadError_None  Successfully set the Key Identifier.
     *
     */
    ThreadError SetKeyId(uint8_t aKeyId);

    /**
     * This method gets the Command ID.
     *
     * @param[out]  aCommandId  The Command ID.
     *
     * @retval kThreadError_None  Successfully retrieved the Command ID.
     *
     */
    ThreadError GetCommandId(uint8_t &aCommandId);

    /**
     * This method sets the Command ID.
     *
     * @param[in]  aCommandId  The Command ID.
     *
     * @retval kThreadError_None  Successfully set the Command ID.
     *
     */
    ThreadError SetCommandId(uint8_t aCommandId);

    /**
     * This method returns the MAC Frame Length.
     *
     * @returns The MAC Frame Length.
     *
     */
    uint8_t GetLength(void) const;

    /**
     * This method sets the MAC Frame Length.
     *
     * @param[in]  aLength  The MAC Frame Length.
     *
     * @retval kThreadError_None         Successfully set the MAC Frame Length.
     * @retval kThreadError_InvalidArgs  The @p aLength value was invalid.
     *
     */
    ThreadError SetLength(uint8_t aLength);

    /**
     * This method returns the MAC header size.
     *
     * @returns The MAC header size.
     *
     */
    uint8_t GetHeaderLength(void);

    /**
     * This method returns the MAC footer size.
     *
     * @returns The MAC footer size.
     *
     */
    uint8_t GetFooterLength(void);

    /**
     * This method returns the current MAC Payload length.
     *
     * @returns The current MAC Payload length.
     *
     */
    uint8_t GetPayloadLength(void);

    /**
     * This method returns the maximum MAC Payload length for the given MAC header and footer.
     *
     * @returns The maximum MAC Payload length for the given MAC header and footer.
     *
     */
    uint8_t GetMaxPayloadLength(void);

    /**
     * This method sets the MAC Payload length.
     *
     * @retval kThreadError_None         Successfully set the MAC Payload length.
     * @retval kThreadError_InvalidArgs  The @p aLength value was invalid.
     *
     */
    ThreadError SetPayloadLength(uint8_t aLength);

    /**
     * This method returns the IEEE 802.15.4 channel used for transmission or reception.
     *
     * @returns The IEEE 802.15.4 channel used for transmission or reception.
     *
     */
    uint8_t GetChannel(void) const { return mChannel; }

    /**
     * This method sets the IEEE 802.15.4 channel used for transmission or reception.
     *
     * @param[in]  aChannel  The IEEE 802.15.4 channel used for transmission or reception.
     *
     */
    void SetChannel(uint8_t aChannel) { mChannel = aChannel; }

    /**
     * This method returns the transmit/receive power in dBm used for transmission or reception.
     *
     * @returns The transmit/receive power in dBm used for transmission or reception.
     *
     */
    int8_t GetPower(void) const { return mPower; }

    /**
     * This method sets the transmit/receive power in dBm used for transmission or reception.
     *
     * @param[in]  aPower  The transmit/receive power in dBm used for transmission or reception.
     *
     */
    void SetPower(int8_t aPower) { mPower = aPower; }

    /**
     * This method returns the receive Link Quality Indicator.
     *
     * @returns The receive Link Quality Indicator.
     *
     */
    int8_t GetLqi(void) const { return mLqi; }

    /**
     * This method sets the receive Link Quality Indicator.
     *
     * @param[in]  aLqi  The receive Link Quality Indicator.
     *
     */
    void SetLqi(int8_t aLqi) { mLqi = aLqi; }

    /**
     * This method indicates whether or not frame security was enabled and passed security validation.
     *
     * @retval TRUE   Frame security was enabled and passed security validation.
     * @retval FALSE  Frame security was not enabled or did not pass security validation.
     *
     */
    bool GetSecurityValid(void) const { return mSecurityValid; }

    /**
     * This method sets the security valid attribute.
     *
     * @param[in]  aSecurityValid  TRUE if frame security was enabled and passed security validation, FALSE otherwise.
     *
     */
    void SetSecurityValid(bool aSecurityValid) { mSecurityValid = aSecurityValid; }

    /**
     * This method returns the IEEE 802.15.4 PSDU length.
     *
     * @returns The IEEE 802.15.4 PSDU length.
     *
     */
    uint8_t GetPsduLength(void) const { return mLength; }

    /**
     * This method sets the IEEE 802.15.4 PSDU length.
     *
     * @param[in]  aLength  The IEEE 802.15.4 PSDU length.
     *
     */
    void SetPsduLength(uint8_t aLength) { mLength = aLength; }

    /**
     * This method returns a pointer to the PSDU.
     *
     * @returns A pointer to the PSDU.
     *
     */
    uint8_t *GetPsdu(void) { return mPsdu; }

    /**
     * This method returns a pointer to the MAC Header.
     *
     * @returns A pointer to the MAC Header.
     *
     */
    uint8_t *GetHeader(void);

    /**
     * This method returns a pointer to the MAC Payload.
     *
     * @returns A pointer to the MAC Payload.
     *
     */
    uint8_t *GetPayload(void);

    /**
     * This method returns a pointer to the MAC Footer.
     *
     * @returns A pointer to the MAC Footer.
     *
     */
    uint8_t *GetFooter(void);

private:
    enum
    {
        kKeyIdLengthMode0 = 0,   ///< Mode 0 Key ID Length in bytes (IEEE 802.15.4-2006)
        kKeyIdLengthMode1 = 1,   ///< Mode 1 Key ID Length in bytes (IEEE 802.15.4-2006)
        kKeyIdLengthMode2 = 5,   ///< Mode 2 Key ID Length in bytes (IEEE 802.15.4-2006)
        kKeyIdLengthMode3 = 9,   ///< Mode 3 Key ID Length in bytes (IEEE 802.15.4-2006)
    };

    uint8_t *FindSequence(void);
    uint8_t *FindDstPanId(void);
    uint8_t *FindDstAddr(void);
    uint8_t *FindSrcPanId(void);
    uint8_t *FindSrcAddr(void);
    uint8_t *FindSecurityHeader(void);
};

/**
 * This class implements IEEE 802.15.4 Beacon generation and parsing.
 *
 */
class Beacon
{
public:
    enum
    {
        kSuperFrameSpec   = 0x0fff,                 ///< Superframe Specification value.
        kProtocolId       = 3,                      ///< Thread Protocol ID.
        kNetworkNameSize  = 16,                     ///< Size of Thread Network Name (bytes).
        kExtPanIdSize     = 8,                      ///< Size of Thread Extended PAN ID.
    };

    enum
    {
        kProtocolVersion  = 1,                      ///< Thread Protocol version.
        kVersionOffset    = 4,                      ///< Version field bit offset.
        kVersionMask      = 0xf << kVersionOffset,  ///< Version field mask.
        kNativeFlag       = 1 << 3,                 ///< Native Commissioner flag.
        kJoiningFlag      = 1 << 0,                 ///< Joining Permitted flag.
    };

    /**
     * This method initializes the Beacon message.
     *
     */
    void Init(void) {
        mSuperframeSpec = Thread::Encoding::LittleEndian::HostSwap16(kSuperFrameSpec);
        mGtsSpec = 0;
        mPendingAddressSpec = 0;
        mProtocolId = kProtocolId;
        mFlags = kProtocolVersion << kVersionOffset;
    }

    /**
     * This method indicates whether or not the beacon appears to be a valid Thread Beacon message.
     *
     * @retval TRUE  if the beacon appears to be a valid Thread Beacon message.
     * @retval FALSE if the beacon does not appear to be a valid Thread Beacon message.
     *
     */
    bool IsValid(void) {
        return (mSuperframeSpec == Thread::Encoding::LittleEndian::HostSwap16(kSuperFrameSpec)) &&
               (mGtsSpec == 0) && (mPendingAddressSpec == 0) && (mProtocolId == kProtocolId);
    }

    /**
     * This method returns the Protocol ID value.
     *
     * @returns the Protocol ID value.
     *
     */
    uint8_t GetProtocolId(void) const { return mProtocolId; }

    /**
     * This method returns the Protocol Version value.
     *
     * @returns The Protocol Version value.
     *
     */
    uint8_t GetProtocolVersion(void) const { return mFlags >> kVersionOffset; }

    /**
     * This method indicates whether or not the Native Commissioner flag is set.
     *
     * @retval TRUE   if the Native Commissioner flag is set.
     * @retval FALSE  if the Native Commissioner flag is not set.
     *
     */
    bool IsNative(void) const { return (mFlags & kNativeFlag) != 0; }

    /**
     * This method clears the Native Commissioner flag.
     *
     */
    void ClearNative(void) { mFlags &= ~kNativeFlag; }

    /**
     * This method sets the Native Commissioner flag.
     *
     */
    void SetNative(void) { mFlags |= kNativeFlag; }

    /**
     * This method indicates whether or not the Joining Permitted flag is set.
     *
     * @retval TRUE   if the Joining Permitted flag is set.
     * @retval FALSE  if the Joining Permitted flag is not set.
     *
     */
    bool IsJoiningPermitted(void) const { return (mFlags & kJoiningFlag) != 0; }

    /**
     * This method clears the Joining Permitted flag.
     *
     */
    void ClearJoiningPermitted(void) { mFlags &= ~kJoiningFlag; }

    /**
     * This method sets the Joining Permitted flag.
     *
     */
    void SetJoiningPermitted(void) { mFlags |= kJoiningFlag; }

    /**
     * This method returns a pointer to the Network Name field.
     *
     * @returns A pointer to the network name field.
     *
     */
    const char *GetNetworkName(void) const { return mNetworkName; }

    /**
     * This method sets the Network Name field.
     *
     * @param[in]  aNetworkName  A pointer to the Network Name.
     *
     */
    void SetNetworkName(const char *aNetworkName) {
        int length = strnlen(aNetworkName, sizeof(mNetworkName));
        memset(mNetworkName, 0, sizeof(mNetworkName));
        memcpy(mNetworkName, aNetworkName, length);
    }

    /**
     * This method returns a pointer to the Extended PAN ID field.
     *
     * @returns A pointer to the Extended PAN ID field.
     *
     */
    const uint8_t *GetExtendedPanId(void) const { return mExtendedPanId; }

    /**
     * This method sets the Extended PAN ID field.
     *
     * @param[in]  aExtPanId  A pointer to the Extended PAN ID.
     *
     */
    void SetExtendedPanId(const uint8_t *aExtPanId) { memcpy(mExtendedPanId, aExtPanId, sizeof(mExtendedPanId)); }

private:
    uint16_t mSuperframeSpec;
    uint8_t  mGtsSpec;
    uint8_t  mPendingAddressSpec;
    uint8_t  mProtocolId;
    uint8_t  mFlags;
    char     mNetworkName[kNetworkNameSize];
    uint8_t  mExtendedPanId[kExtPanIdSize];
} OT_TOOL_PACKED_END;

/**
 * @}
 *
 */

}  // namespace Mac
}  // namespace Thread

#endif  // MAC_FRAME_HPP_
