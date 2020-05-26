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
 *   This file includes definitions for MeshCoP.
 *
 */

#ifndef MESHCOP_HPP_
#define MESHCOP_HPP_

#include "openthread-core-config.h"

#include <openthread/instance.h>

#include "coap/coap.hpp"
#include "common/message.hpp"
#include "mac/mac_types.hpp"
#include "meshcop/meshcop_tlvs.hpp"

namespace ot {

class ThreadNetif;

namespace MeshCoP {

enum
{
    kBorderAgentUdpPort = 49191, ///< UDP port of border agent service.
};

/**
 * This function creates Message for MeshCoP.
 *
 */
inline Coap::Message *NewMeshCoPMessage(Coap::CoapBase &aCoap)
{
    return aCoap.NewMessage(Message::Settings(Message::kWithLinkSecurity, Message::kPriorityNet));
}

/**
 * This function generates PSKc.
 *
 * PSKc is used to establish the Commissioner Session.
 *
 * @param[in]  aPassPhrase   The commissioning passphrase.
 * @param[in]  aNetworkName  The network name for PSKc computation.
 * @param[in]  aExtPanId     The extended PAN ID for PSKc computation.
 * @param[out] aPskc         A reference to a PSKc where the generated PSKc will be placed.
 *
 * @retval OT_ERROR_NONE          Successfully generate PSKc.
 * @retval OT_ERROR_INVALID_ARGS  If the length of passphrase is out of range.
 *
 */
otError GeneratePskc(const char *              aPassPhrase,
                     const Mac::NetworkName &  aNetworkName,
                     const Mac::ExtendedPanId &aExtPanId,
                     Pskc &                    aPskc);

/**
 * This function computes the Joiner ID from a factory-assigned IEEE EUI-64.
 *
 * @param[in]   aEui64     The factory-assigned IEEE EUI-64.
 * @param[out]  aJoinerId  The Joiner ID.
 *
 */
void ComputeJoinerId(const Mac::ExtAddress &aEui64, Mac::ExtAddress &aJoinerId);

/**
 * This function gets the border agent RLOC.
 *
 * @param[in]   aNetif  A reference to the thread interface.
 * @param[out]  aRloc   Border agent RLOC.
 *
 * @retval OT_ERROR_NONE        Successfully got the Border Agent Rloc.
 * @retval OT_ERROR_NOT_FOUND   Border agent is not available.
 *
 */
otError GetBorderAgentRloc(ThreadNetif &aNetIf, uint16_t &aRloc);

} // namespace MeshCoP

} // namespace ot

#endif // MESHCOP_HPP_
