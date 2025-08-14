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
 *   This file includes definitions for `MessageFramer`.
 */

#ifndef MESSAGE_FRAMER_HPP_
#define MESSAGE_FRAMER_HPP_

#include "openthread-core-config.h"

#include "common/locator.hpp"
#include "common/message.hpp"
#include "common/non_copyable.hpp"
#include "mac/mac_frame.hpp"
#include "mac/mac_types.hpp"

namespace ot {

class DataPollSender;

class MessageFramer : public InstanceLocator, private NonCopyable
{
    friend class DataPollSender;

public:
    /**
     * Initializes the `MessageFramer`.
     *
     * @param[in]  aInstance     The OpenThread instance.
     */
    explicit MessageFramer(Instance &aInstance);

    /**
     * Determines the MAC source address for an IPv6 message transmission based on the source IPv6 address used.
     *
     * @param[in]  aIp6Addr    The message's source IPv6 address.
     * @param[out] aMacAddrs   The `Mac::Addresses` object to update. Only the `mSource` field will be updated.
     */
    void DetermineMacSourceAddress(const Ip6::Address &aIp6Addr, Mac::Addresses &aMacAddrs) const;

    /**
     * Prepares an empty MAC data frame.
     *
     * For MAC source, device's MAC address will be used.
     *
     * @param[out] aFrame        A MAC `TxFrame` to populate.
     * @param[in]  aMacDest      The MAC destination address to use.
     * @param[in] aAckRequest    A boolean to indicate whether or not set the `AckRequest` flag.
     */
    void PrepareEmptyFrame(Mac::TxFrame &aFrame, const Mac::Address &aMacDest, bool aAckRequest);

    /**
     * Prepares a MAC data frame from a given IPv6 message.
     *
     * This method handles the generation of the MAC headers, mesh header (if requested), 6LoWPAN header compression,
     * and fragmentation header.
     *
     * If the message requires fragmentation or if @p aAddFragHeader is set to `true`, a fragmentation header will be
     * included. The method uses the `aMessage.GetOffset()` to construct subsequent fragments.
     *
     * This method also handles enabling link-layer security. If the message is an MLE message and requires
     * fragmentation, link-layer security is enabled on the message, and the frame to be prepared again.
     *
     * @param[out] aFrame            A MAC `TxFrame` to populate.
     * @param[in]  aMessage          The IPv6 message.
     * @param[in]  aMacAddrs         The MAC source and destination addresses.
     * @param[in]  aAddMeshHeader    A boolean indicating whether to add a mesh header.
     * @param[in]  aMeshSource       The Mesh Header source RLOC16 (used if @p aAddMeshHeader is true).
     * @param[in]  aMeshDest         The Mesh Header destination RLOC16 (used if @p aAddMeshHeader is true).
     * @param[in]  aAddFragHeader    A boolean to force adding a fragmentation header.
     *
     * @returns The next offset into @p aMessage after the prepared frame.
     */
    uint16_t PrepareFrame(Mac::TxFrame         &aFrame,
                          Message              &aMessage,
                          const Mac::Addresses &aMacAddrs,
                          bool                  aAddMeshHeader = false,
                          uint16_t              aMeshSource    = 0,
                          uint16_t              aMeshDest      = 0,
                          bool                  aAddFragHeader = false);

#if OPENTHREAD_FTD
    /**
     * Prepares a MAC data frame from a given 6LoWPAN Mesh message.
     *
     * @param[out] aFrame            A MAC `TxFrame` to populate.
     * @param[in]  aMessage          The 6LoWPAN Mesh message.
     * @param[in]  aMacAddrs         The MAC source and destination addresses.
     *
     * @returns The next offset into @p aMessage after the prepared frame.
     */
    uint16_t PrepareMeshFrame(Mac::TxFrame &aFrame, Message &aMessage, const Mac::Addresses &aMacAddrs);

#endif // OPENTHREAD_FTD

private:
    static constexpr uint8_t kMeshHeaderFrameMtu     = OT_RADIO_FRAME_MAX_SIZE; // Max MTU with a Mesh Header frame.
    static constexpr uint8_t kMeshHeaderFrameFcsSize = sizeof(uint16_t);        // Frame FCS size for Mesh Header frame.

    // Hops left to use in lowpan mesh header: We use `kMaxRouteCost` as
    // max hops between routers within Thread  mesh. We then add two
    // for possibility of source or destination being a child
    // (requiring one hop) and one as additional guard increment.
    static constexpr uint8_t kMeshHeaderHopsLeft = Mle::kMaxRouteCost + 3;

    void PrepareMacHeaders(Mac::TxFrame &aTxFrame, Mac::TxFrame::Info &aTxFrameInfo, const Message *aMessage);

    uint16_t mFragTag;
};

} // namespace ot

#endif // MESSAGE_FRAMER_HPP_
