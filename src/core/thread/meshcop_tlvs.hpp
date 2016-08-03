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
 *   This file includes definitions for generating and processing MeshCoP TLVs.
 *
 */

#ifndef MESHCOP_TLVS_HPP_
#define MESHCOP_TLVS_HPP_

#include <string.h>

#include <openthread-types.h>
#include <common/encoding.hpp>

using Thread::Encoding::BigEndian::HostSwap16;

namespace Thread {
namespace MeshCoP {

/**
 * This class implements MeshCoP TLV generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class Tlv
{
public:
    /**
     * MeshCoP TLV Types.
     *
     */
    enum Type
    {
        kExtendedPanId     = 2,    ///< Extended PAN ID TLV
        kNetworkName       = 3,    ///< Newtork Name TLV
        kDiscoveryRequest  = 128,  ///< Discovery Request TLV
        kDiscoveryResponse = 129,  ///< Discovery Response TLV
    };

    /**
     * This method returns the Type value.
     *
     * @returns The Type value.
     *
     */
    Type GetType(void) const { return static_cast<Type>(mType); }

    /**
     * This method sets the Type value.
     *
     * @param[in]  aType  The Type value.
     *
     */
    void SetType(Type aType) { mType = static_cast<uint8_t>(aType); }

    /**
     * This method returns the Length value.
     *
     */
    uint8_t GetLength(void) const { return mLength; }

    /**
     * This method sets the Length value.
     *
     * @param[in]  aLength  The Length value.
     *
     */
    void SetLength(uint8_t aLength) { mLength = aLength; }

    /**
     * This method returns a pointer to the Value.
     *
     * @returns A pointer to the value.
     *
     */
    uint8_t *GetValue() { return reinterpret_cast<uint8_t *>(this) + sizeof(Tlv); }

    /**
     * This method returns a pointer to the next TLV.
     *
     * @returns A pointer to the next TLV.
     *
     */
    Tlv *GetNext() {
        return reinterpret_cast<Tlv *>(reinterpret_cast<uint8_t *>(this) + sizeof(*this) + mLength);
    }

private:
    uint8_t mType;
    uint8_t mLength;
} OT_TOOL_PACKED_END;

/**
 * This class implements Extended PAN ID TLV generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class ExtendedPanIdTlv: public Tlv
{
public:
    /**
     * This method initializes the TLV.
     *
     */
    void Init(void) { SetType(kExtendedPanId); SetLength(sizeof(*this) - sizeof(Tlv)); }

    /**
     * This method indicates whether or not the TLV appears to be well-formed.
     *
     * @retval TRUE   If the TLV appears to be well-formed.
     * @retval FALSE  If the TLV does not appear to be well-formed.
     *
     */
    bool IsValid(void) const { return GetLength() == sizeof(*this) - sizeof(Tlv); }

    /**
     * This method returns the Extended PAN ID value.
     *
     * @returns The Extended PAN ID value.
     *
     */
    const uint8_t *GetExtendedPanId(void) const { return mExtendedPanId; }

    /**
     * This method sets the Extended PAN ID value.
     *
     * @param[in]  aExtendedPanId  A pointer to the Extended PAN ID value.
     *
     */
    void SetExtendedPanId(const uint8_t *aExtendedPanId) {
        memcpy(mExtendedPanId, aExtendedPanId, OT_EXT_PAN_ID_SIZE);
    }

private:
    uint8_t mExtendedPanId[OT_EXT_PAN_ID_SIZE];
} OT_TOOL_PACKED_END;

/**
 * This class implements Network Name TLV generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class NetworkNameTlv: public Tlv
{
public:
    /**
     * This method initializes the TLV.
     *
     */
    void Init(void) { SetType(kNetworkName); SetLength(sizeof(*this) - sizeof(Tlv)); }

    /**
     * This method indicates whether or not the TLV appears to be well-formed.
     *
     * @retval TRUE   If the TLV appears to be well-formed.
     * @retval FALSE  If the TLV does not appear to be well-formed.
     *
     */
    bool IsValid(void) const { return GetLength() <= sizeof(*this) - sizeof(Tlv); }

    /**
     * This method returns the Network Name value.
     *
     * @returns The Network Name value.
     *
     */
    const char *GetNetworkName(void) const { return mNetworkName; }

    /**
     * This method sets the Network Name value.
     *
     * @param[in]  aNetworkName  A pointer to the Network Name value.
     *
     */
    void SetNetworkName(const char *aNetworkName) {
        int length = strnlen(aNetworkName, sizeof(mNetworkName));
        memcpy(mNetworkName, aNetworkName, length);
        SetLength(length);
    }

private:
    char mNetworkName[OT_NETWORK_NAME_SIZE];
} OT_TOOL_PACKED_END;

/**
 * This class implements Discovery Request TLV generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class DiscoveryRequestTlv: public Tlv
{
public:
    /**
     * This method initializes the TLV.
     *
     */
    void Init(void) { SetType(kDiscoveryRequest); SetLength(sizeof(*this) - sizeof(Tlv)); mReserved = 0; }

    /**
     * This method indicates whether or not the TLV appears to be well-formed.
     *
     * @retval TRUE   If the TLV appears to be well-formed.
     * @retval FALSE  If the TLV does not appear to be well-formed.
     *
     */
    bool IsValid(void) const { return GetLength() == sizeof(*this) - sizeof(Tlv); }

    /**
     * This method returns the Version value.
     *
     * @returns The Version value.
     *
     */
    uint8_t GetVersion(void) const { return mFlags >> kVersionOffset; }

    /**
     * This method sets the Version value.
     *
     * @param[in]  aVersion  The Version value.
     *
     */
    void SetVersion(uint8_t aVersion) { mFlags = (mFlags & ~kVersionMask) | (aVersion << kVersionOffset); }

    /**
     * This method indicates whether or not the Joiner flag is set.
     *
     * @retval TRUE   If the Joiner flag is set.
     * @retval FALSE  If the Joiner flag is not set.
     *
     */
    bool IsJoiner(void) { return mFlags & kJoinerMask; }

    /**
     * This method sets the Joiner flag.
     *
     * @param[in]  aJoiner  TRUE if set, FALSE otherwise.
     *
     */
    void SetJoiner(bool aJoiner) {
        if (aJoiner) {
            mFlags |= kJoinerMask;
        }
        else {
            mFlags &= ~kJoinerMask;
        }
    }

private:
    enum
    {
        kVersionOffset = 4,
        kVersionMask = 0xf << kVersionOffset,
        kJoinerOffset = 3,
        kJoinerMask = 1 << kJoinerOffset,
    };
    uint8_t mFlags;
    uint8_t mReserved;
} OT_TOOL_PACKED_END;

/**
 * This class implements Discovery Response TLV generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class DiscoveryResponseTlv: public Tlv
{
public:
    /**
     * This method initializes the TLV.
     *
     */
    void Init(void) { SetType(kDiscoveryResponse); SetLength(sizeof(*this) - sizeof(Tlv)); mReserved = 0; }

    /**
     * This method indicates whether or not the TLV appears to be well-formed.
     *
     * @retval TRUE   If the TLV appears to be well-formed.
     * @retval FALSE  If the TLV does not appear to be well-formed.
     *
     */
    bool IsValid(void) const { return GetLength() == sizeof(*this) - sizeof(Tlv); }

    /**
     * This method returns the Version value.
     *
     * @returns The Version value.
     *
     */
    uint8_t GetVersion(void) const { return mFlags >> kVersionOffset; }

    /**
     * This method sets the Version value.
     *
     * @param[in]  aVersion  The Version value.
     *
     */
    void SetVersion(uint8_t aVersion) { mFlags = (mFlags & ~kVersionMask) | (aVersion << kVersionOffset); }

    /**
     * This method indicates whether or not the Native Commissioner flag is set.
     *
     * @retval TRUE   If the Native Commissioner flag is set.
     * @retval FALSE  If the Native Commissioner flag is not set.
     *
     */
    bool IsNativeCommissioner(void) { return mFlags & kNativeMask; }

    /**
     * This method sets the Native Commissioner flag.
     *
     * @param[in]  aNativeCommissioner  TRUE if set, FALSE otherwise.
     *
     */
    void SetNativeCommissioner(bool aNativeCommissioner) {
        if (aNativeCommissioner) {
            mFlags |= kNativeMask;
        }
        else {
            mFlags &= ~kNativeMask;
        }
    }

private:
    enum
    {
        kVersionOffset = 4,
        kVersionMask = 0xf << kVersionOffset,
        kNativeOffset = 3,
        kNativeMask = 1 << kNativeOffset,
    };
    uint8_t mFlags;
    uint8_t mReserved;
} OT_TOOL_PACKED_END;

}  // namespace MeshCoP

}  // namespace Thread

#endif  // MESHCOP_TLVS_HPP_
