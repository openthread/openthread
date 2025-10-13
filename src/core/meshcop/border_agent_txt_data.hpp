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

#include "common/error.hpp"
#include "common/locator.hpp"

namespace ot {
namespace MeshCoP {
namespace BorderAgent {

#if OPENTHREAD_CONFIG_BORDER_AGENT_ENABLE

class TxtData : public InstanceLocator
{
public:
    typedef otBorderAgentMeshCoPServiceTxtData ServiceTxtData; ///< Service TXT Data.

    /**
     * Initializes the `TxtData` object.
     *
     * @param[in] aInstance  A reference to the OpenThread instance.
     */
    TxtData(Instance &aInstance);

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
    };

    struct StateBitmap
    {
        static uint32_t Determine(Instance &aInstance);

        // --- State Bitmap ConnectionMode ---
        static constexpr uint8_t  kOffsetConnectionMode   = 0;
        static constexpr uint32_t kMaskConnectionMode     = 7 << kOffsetConnectionMode;
        static constexpr uint32_t kConnectionModeDisabled = 0 << kOffsetConnectionMode;
        static constexpr uint32_t kConnectionModePskc     = 1 << kOffsetConnectionMode;
        static constexpr uint32_t kConnectionModePskd     = 2 << kOffsetConnectionMode;
        static constexpr uint32_t kConnectionModeVendor   = 3 << kOffsetConnectionMode;
        static constexpr uint32_t kConnectionModeX509     = 4 << kOffsetConnectionMode;

        // --- State Bitmap ThreadIfStatus ---
        static constexpr uint8_t  kOffsetThreadIfStatus         = 3;
        static constexpr uint32_t kMaskThreadIfStatus           = 3 << kOffsetThreadIfStatus;
        static constexpr uint32_t kThreadIfStatusNotInitialized = 0 << kOffsetThreadIfStatus;
        static constexpr uint32_t kThreadIfStatusInitialized    = 1 << kOffsetThreadIfStatus;
        static constexpr uint32_t kThreadIfStatusActive         = 2 << kOffsetThreadIfStatus;

        // --- State Bitmap Availability ---
        static constexpr uint8_t  kOffsetAvailability     = 5;
        static constexpr uint32_t kMaskAvailability       = 3 << kOffsetAvailability;
        static constexpr uint32_t kAvailabilityInfrequent = 0 << kOffsetAvailability;
        static constexpr uint32_t kAvailabilityHigh       = 1 << kOffsetAvailability;

        // --- State Bitmap BbrIsActive ---
        static constexpr uint8_t  kOffsetBbrIsActive = 7;
        static constexpr uint32_t kFlagBbrIsActive   = 1 << kOffsetBbrIsActive;

        // --- State Bitmap BbrIsPrimary ---
        static constexpr uint8_t  kOffsetBbrIsPrimary = 8;
        static constexpr uint32_t kFlagBbrIsPrimary   = 1 << kOffsetBbrIsPrimary;

        // --- State Bitmap ThreadRole ---
        static constexpr uint8_t  kOffsetThreadRole             = 9;
        static constexpr uint32_t kMaskThreadRole               = 3 << kOffsetThreadRole;
        static constexpr uint32_t kThreadRoleDisabledOrDetached = 0 << kOffsetThreadRole;
        static constexpr uint32_t kThreadRoleChild              = 1 << kOffsetThreadRole;
        static constexpr uint32_t kThreadRoleRouter             = 2 << kOffsetThreadRole;
        static constexpr uint32_t kThreadRoleLeader             = 3 << kOffsetThreadRole;

        // --- State Bitmap EpskcSupported ---
        static constexpr uint8_t  kOffsetEpskcSupported = 11;
        static constexpr uint32_t kFlagEpskcSupported   = 1 << kOffsetEpskcSupported;
    };
};

#endif // OPENTHREAD_CONFIG_BORDER_AGENT_ENABLE

} // namespace BorderAgent
} // namespace MeshCoP
} // namespace ot

#endif // BORDER_AGENT_TXT_DATA_HPP_
