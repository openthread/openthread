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
 *   This file includes definitions and methods for generating and processing Thread Network Layer TLVs.
 */

#ifndef OT_CORE_THREAD_THREAD_TLVS_HPP_
#define OT_CORE_THREAD_THREAD_TLVS_HPP_

#include "openthread-core-config.h"

#include "common/encoding.hpp"
#include "common/message.hpp"
#include "common/tlvs.hpp"
#include "meshcop/network_name.hpp"
#include "net/ip6_address.hpp"
#include "thread/mle_types.hpp"

namespace ot {

/**
 * Implements Network Layer TLV generation and parsing.
 */
OT_TOOL_PACKED_BEGIN
class ThreadTlv : public ot::Tlv
{
public:
    /**
     * Network Layer TLV Types.
     */
    enum Type : uint8_t
    {
        kTarget                = 0,  ///< Target EID TLV
        kExtMacAddress         = 1,  ///< Extended MAC Address TLV
        kRloc16                = 2,  ///< RLOC16 TLV
        kMeshLocalEid          = 3,  ///< ML-EID TLV
        kStatus                = 4,  ///< Status TLV
        kLastTransactionTime   = 6,  ///< Time Since Last Transaction TLV
        kRouterMask            = 7,  ///< Router Mask TLV
        kNdOption              = 8,  ///< ND Option TLV
        kNdData                = 9,  ///< ND Data TLV
        kThreadNetworkData     = 10, ///< Thread Network Data TLV
        kTimeout               = 11, ///< Timeout TLV
        kNetworkName           = 12, ///< Network Name TLV
        kIp6Addresses          = 14, ///< IPv6 Addresses TLV
        kCommissionerSessionId = 15, ///< Commissioner Session ID TLV
    };

    /**
     * Returns the Type value.
     *
     * @returns The Type value.
     */
    Type GetType(void) const { return static_cast<Type>(ot::Tlv::GetType()); }

    /**
     * Sets the Type value.
     *
     * @param[in]  aType  The Type value.
     */
    void SetType(Type aType) { ot::Tlv::SetType(static_cast<uint8_t>(aType)); }

} OT_TOOL_PACKED_END;

/**
 * Defines Target TLV constants and types.
 */
typedef SimpleTlvInfo<ThreadTlv::kTarget, Ip6::Address> ThreadTargetTlv;

/**
 * Defines Extended MAC Address TLV constants and types.
 */
typedef SimpleTlvInfo<ThreadTlv::kExtMacAddress, Mac::ExtAddress> ThreadExtMacAddressTlv;

/**
 * Defines RLOC16 TLV constants and types.
 */
typedef UintTlvInfo<ThreadTlv::kRloc16, uint16_t> ThreadRloc16Tlv;

/**
 * Defines ML-EID TLV constants and types.
 */
typedef SimpleTlvInfo<ThreadTlv::kMeshLocalEid, Ip6::InterfaceIdentifier> ThreadMeshLocalEidTlv;

/**
 * Defines Time Since Last Transaction TLV constants and types.
 */
typedef UintTlvInfo<ThreadTlv::kLastTransactionTime, uint32_t> ThreadLastTransactionTimeTlv;

/**
 * Defines Timeout TLV constants and types.
 */
typedef UintTlvInfo<ThreadTlv::kTimeout, uint32_t> ThreadTimeoutTlv;

/**
 * Defines Network Name TLV constants and types.
 */
typedef StringTlvInfo<ThreadTlv::kNetworkName, MeshCoP::NetworkName::kMaxSize> ThreadNetworkNameTlv;

/**
 * Defines Commissioner Session ID TLV constants and types.
 */
typedef UintTlvInfo<ThreadTlv::kCommissionerSessionId, uint16_t> ThreadCommissionerSessionIdTlv;

/**
 * Defines Status TLV constants and types.
 *
 * The definition of Status values in this TLV depends on the TMF message in which it is used.
 */
typedef UintTlvInfo<ThreadTlv::kStatus, uint8_t> ThreadStatusTlv;

/**
 * Defines Router Mask TLV  constants and types.
 */
typedef SimpleTlvInfo<ThreadTlv::kRouterMask, Mle::RouterIdMask> ThreadRouterMaskTlv;

/**
 * Defines Thread Network Data TLV constants and types.
 */
typedef TlvInfo<ThreadTlv::kThreadNetworkData> ThreadNetworkDataTlv;

#if OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2

/**
 * Defines IPv6 Addresses TLV constants and types and helper methods.
 */
class Ip6AddressesTlv : public TlvInfo<ThreadTlv::kIp6Addresses>
{
public:
    /**
     * Appends an IPv6 Addresses TLV to a message.
     *
     * @param[in,out] aMessage       The message to append to.
     * @param[in]     aAddresses     A pointer to an array of IPv6 addresses.
     * @param[in]     aNumAddresses  The number of IPv6 addresses in the @p aAddresses array.
     *
     * @retval kErrorNone     Successfully appended the TLV.
     * @retval kErrorNoBufs   Insufficient available buffers to grow the message.
     */
    static Error AppendTo(Message &aMessage, const Ip6::Address *aAddresses, uint16_t aNumAddresses);

    Ip6AddressesTlv(void) = delete;
};

#endif // OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2

} // namespace ot

#endif // OT_CORE_THREAD_THREAD_TLVS_HPP_
