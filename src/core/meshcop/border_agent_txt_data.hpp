/*
 *  Copyright (c) 2025, The OpenThread Authors.
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
 *   This file includes definitions for Border Agent MeshCoP service TXT data.
 */

#ifndef BORDER_AGENT_TXT_DATA_HPP_
#define BORDER_AGENT_TXT_DATA_HPP_

#include "openthread-core-config.h"

#include <openthread/border_agent.h>
#include <openthread/border_agent_txt_data.h>

#include "common/as_core_type.hpp"
#include "common/clearable.hpp"
#include "common/encoding.hpp"
#include "common/error.hpp"
#include "common/locator.hpp"
#include "common/type_traits.hpp"
#include "net/dns_types.hpp"
#include "net/ip6_address.hpp"

namespace ot {
namespace MeshCoP {
namespace BorderAgent {

#if OPENTHREAD_CONFIG_BORDER_AGENT_ENABLE || OPENTHREAD_CONFIG_BORDER_AGENT_TXT_DATA_PARSER_ENABLE

class TxtData : public InstanceLocator
{
public:
    typedef otBorderAgentConnMode      ConnMode;     ///< Connection Mode in a Border Agent State Bitmap.
    typedef otBorderAgentThreadIfState IfState;      ///< Thread Interface State in a Border Agent State Bitmap.
    typedef otBorderAgentAvailability  Availability; ///< Availability Status in a Border Agent State Bitmap.
    typedef otBorderAgentThreadRole    Role;         ///< Thread Role in a Border Agent State Bitmap.

    static constexpr ConnMode kConnModeDisabled = OT_BORDER_AGENT_CONN_MODE_DISABLED; ///< Not allowed.
    static constexpr ConnMode kConnModePskc     = OT_BORDER_AGENT_CONN_MODE_PSKC;     ///< DTLS with PSKc.
    static constexpr ConnMode kConnModePskd     = OT_BORDER_AGENT_CONN_MODE_PSKD;     ///< DTLS with PSKd.
    static constexpr ConnMode kConnModeVendor   = OT_BORDER_AGENT_CONN_MODE_VENDOR;   ///< Vendor defined.
    static constexpr ConnMode kConnModeX509     = OT_BORDER_AGENT_CONN_MODE_X509;     ///< DTLS X.509 cert.

    static constexpr IfState kThreadIfNotInit = OT_BORDER_AGENT_THREAD_IF_NOT_INITIALIZED; ///< Not init.
    static constexpr IfState kThreadIfInit    = OT_BORDER_AGENT_THREAD_IF_INITIALIZED;     ///< Init but not active.
    static constexpr IfState kThreadIfActive  = OT_BORDER_AGENT_THREAD_IF_ACTIVE;          ///< Init and active.

    static constexpr Availability kAvailabilityInfreq = OT_BORDER_AGENT_AVAILABILITY_INFREQUENT; ///< Infrequent.
    static constexpr Availability kAvailabilityHigh   = OT_BORDER_AGENT_AVAILABILITY_HIGH;       ///< High.

    static constexpr Role kRoleDisabledDetached = OT_BORDER_AGENT_THREAD_ROLE_DISABLED_OR_DETACHED; ///< Detach.
    static constexpr Role kRoleChild            = OT_BORDER_AGENT_THREAD_ROLE_CHILD;                ///< Child.
    static constexpr Role kRoleRouter           = OT_BORDER_AGENT_THREAD_ROLE_ROUTER;               ///< Router.
    static constexpr Role kRoleLeader           = OT_BORDER_AGENT_THREAD_ROLE_LEADER;               ///< Leader.

    /**
     * Initializes the `TxtData` object.
     *
     * @param[in] aInstance  A reference to the OpenThread instance.
     */
    TxtData(Instance &aInstance);

#if OPENTHREAD_CONFIG_BORDER_AGENT_TXT_DATA_PARSER_ENABLE

    /**
     * Represents information parsed from a Border Agent TXT Data.
     */
    class Info : public otBorderAgentTxtDataInfo, public Clearable<Info>
    {
    public:
        /**
         * Parses a Border Agent's MeshCoP service TXT data.
         *
         * @param[in]  aTxtData        A pointer to the buffer containing the TXT data.
         * @param[in]  aTxtDataLength  The length of the TXT data in bytes.
         *
         * @retval kErrorNone     Successfully parsed the TXT data.
         * @retval kErrorParse    Failed to parse the TXT data.
         */
        Error ParseFrom(const uint8_t *aTxtData, uint16_t aTxtDataLength);

    private:
        void ProcessTxtEntry(const Dns::TxtEntry &aEntry);

        static bool ReadValue(const Dns::TxtEntry &aEntry, void *aBuffer, uint16_t aSize);
        static void ReadStringValue(const Dns::TxtEntry &aEntry, char *aString, uint16_t aStringSize);
        static bool ReadOmrPrefix(const Dns::TxtEntry &aEntry, Ip6::Prefix &aPrefix);

        template <typename ObjectType> bool ReadValue(const Dns::TxtEntry &aEntry, ObjectType &aObject)
        {
            static_assert(!TypeTraits::IsPointer<ObjectType>::kValue, "ObjectType must not be a pointer");

            return ReadValue(aEntry, &aObject, sizeof(aObject));
        }

        template <uint16_t kStringSize>
        static void ReadStringValue(const Dns::TxtEntry &aEntry, char (&aString)[kStringSize])
        {
            ReadStringValue(aEntry, aString, kStringSize);
        }

        template <typename UintType>
        static bool ReadBigEndianUintValue(const Dns::TxtEntry &aEntry, UintType &aUintValue)
        {
            static_assert(TypeTraits::IsUint<UintType>::kValue, "UintType must be a 8, 16, 32, or 64 bit uint");

            bool didRead = false;

            if (aEntry.mValueLength >= sizeof(UintType))
            {
                aUintValue = BigEndian::Read<UintType>(aEntry.mValue);
                didRead    = true;
            }

            return didRead;
        }
    };

#endif // OPENTHREAD_CONFIG_BORDER_AGENT_TXT_DATA_PARSER_ENABLE

#if OPENTHREAD_CONFIG_BORDER_AGENT_ENABLE

    typedef otBorderAgentMeshCoPServiceTxtData ServiceTxtData; ///< Service TXT Data.

    /**
     * Prepares the MeshCoP service TXT data.
     *
     * @param[out]    aBuffer        A pointer to a buffer to store the TXT data.
     * @param[in]     aBufferSize    The size of @p aBuffer.
     * @param[out]    aLength        On exit, the length of the prepared TXT data.
     *
     * @retval kErrorNone      Successfully prepared the TXT data.
     * @retval kErrorNoBufs    The @p aBufferSize is too small.
     */
    Error Prepare(uint8_t *aBuffer, uint16_t aBufferSize, uint16_t &aLength);

    /**
     * Prepares the MeshCoP service TXT data.
     *
     * @param[out] aTxtData   A reference to a MeshCoP Service TXT data struct to get the data.
     *
     * @retval kErrorNone     Successfully prepared the TXT data.
     * @retval kErrorNoBufs   The buffer in @p aTxtData is too small.
     */
    Error Prepare(ServiceTxtData &aTxtData);

#endif // OPENTHREAD_CONFIG_BORDER_AGENT_ENABLE

private:
    static const char kRecordVersion[];

    struct Key
    {
        static const char kRecordVersion[];
        static const char kAgentId[];
        static const char kThreadVersion[];
        static const char kStateBitmap[];
        static const char kNetworkName[];
        static const char kExtendedPanId[];
        static const char kActiveTimestamp[];
        static const char kPartitionId[];
        static const char kDomainName[];
        static const char kBbrSeqNum[];
        static const char kBbrPort[];
        static const char kOmrPrefix[];
        static const char kExtAddress[];
#if OPENTHREAD_CONFIG_BORDER_AGENT_TXT_DATA_PARSER_ENABLE
        static const char kVendorName[];
        static const char kModelName[];
#endif
    };

    struct StateBitmap
    {
        static constexpr uint8_t kOffsetConnMode       = 0;
        static constexpr uint8_t kOffsetIfState        = 3;
        static constexpr uint8_t kOffsetAvailability   = 5;
        static constexpr uint8_t kOffsetBbrIsActive    = 7;
        static constexpr uint8_t kOffsetBbrIsPrimary   = 8;
        static constexpr uint8_t kOffsetRole           = 9;
        static constexpr uint8_t kOffsetEpskcSupported = 11;

        static constexpr uint32_t kMaskConnMode       = 7 << kOffsetConnMode;
        static constexpr uint32_t kMaskIfState        = 3 << kOffsetIfState;
        static constexpr uint32_t kMaskAvailability   = 3 << kOffsetAvailability;
        static constexpr uint32_t kFlagBbrIsActive    = 1 << kOffsetBbrIsActive;
        static constexpr uint32_t kFlagBbrIsPrimary   = 1 << kOffsetBbrIsPrimary;
        static constexpr uint32_t kMaskRole           = 3 << kOffsetRole;
        static constexpr uint32_t kFlagEpskcSupported = 1 << kOffsetEpskcSupported;

        static_assert(kConnModeDisabled == 0, "kConnModeDisabled is incorrect");
        static_assert(kConnModePskc == 1, "kConnModePskc is incorrect");
        static_assert(kConnModePskd == 2, "kConnModePskd is incorrect");
        static_assert(kConnModeVendor == 3, "kConnModeVendor is incorrect");
        static_assert(kConnModeX509 == 4, "kConnModeX509 is incorrect");

        static_assert(kThreadIfNotInit == 0, "kThreadIfNotInit is incorrect");
        static_assert(kThreadIfInit == 1, "kThreadIfInit is incorrect");
        static_assert(kThreadIfActive == 2, "kThreadIfActive is incorrect");

        static_assert(kAvailabilityInfreq == 0, "kAvailabilityInfreq is incorrect");
        static_assert(kAvailabilityHigh == 1, "kAvailabilityHigh is incorrect");

        static_assert(kRoleDisabledDetached == 0, "kRoleDisabledDetached is incorrect");
        static_assert(kRoleChild == 1, "kRoleChild is incorrect");
        static_assert(kRoleRouter == 2, "kRoleRouter is incorrect");
        static_assert(kRoleLeader == 3, "kRoleLeader is incorrect");

#if OPENTHREAD_CONFIG_BORDER_AGENT_TXT_DATA_PARSER_ENABLE
        typedef otBorderAgentStateBitmap Info;

        static void Parse(uint32_t aBitmap, Info &aInfo);
#endif

#if OPENTHREAD_CONFIG_BORDER_AGENT_ENABLE
        static uint32_t Determine(Instance &aInstance);
#endif
    };
};

#endif // OPENTHREAD_CONFIG_BORDER_AGENT_ENABLE || OPENTHREAD_CONFIG_BORDER_AGENT_TXT_DATA_PARSER_ENABLE

} // namespace BorderAgent
} // namespace MeshCoP

#if OPENTHREAD_CONFIG_BORDER_AGENT_TXT_DATA_PARSER_ENABLE
DefineCoreType(otBorderAgentTxtDataInfo, MeshCoP::BorderAgent::TxtData::Info);
#endif

} // namespace ot

#endif // BORDER_AGENT_TXT_DATA_HPP_
