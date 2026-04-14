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
 *   This file includes definitions for the Nexus simulation observer.
 */

#ifndef OT_NEXUS_PLATFORM_NEXUS_OBSERVER_HPP_
#define OT_NEXUS_PLATFORM_NEXUS_OBSERVER_HPP_

#include <stdint.h>

namespace ot {
namespace Nexus {

class Node;

/**
 * Represents an observer for the Nexus simulation.
 *
 * This class defines the interface for receiving simulation events from the Nexus core.
 */
class Observer
{
public:
    virtual ~Observer(void) = default;

    /**
     * This method is called when a node state has changed.
     *
     * @param[in] aNode  A pointer to the node whose state has changed.
     */
    virtual void OnNodeStateChanged(Node *aNode) = 0;

    /**
     * This method is called when a link state between two nodes has been updated.
     *
     * @param[in] aSrcId    The source node ID.
     * @param[in] aDstId    The destination node ID.
     * @param[in] aIsActive TRUE if the link is active, FALSE otherwise.
     */
    virtual void OnLinkUpdate(uint32_t aSrcId, uint32_t aDstId, bool aIsActive) = 0;

    /**
     * This method is called when a packet event occurs.
     *
     * @param[in] aSenderId       The ID of the node that sent the packet.
     * @param[in] aDestinationId  The ID of the destination node, or 0xffff for broadcasts or unresolved nodes.
     * @param[in] aData           A pointer to the packet data.
     * @param[in] aLen            The length of the packet data.
     */
    virtual void OnPacketEvent(uint32_t aSenderId, uint32_t aDestinationId, const uint8_t *aData, uint16_t aLen) = 0;

    /**
     * This method is called to clear all events.
     */
    virtual void OnClearEvents(void) = 0;

    /**
     * This method indicates whether or not the observer is connected.
     *
     * @retval TRUE   If the observer is connected.
     * @retval FALSE  If the observer is not connected.
     */
    virtual bool IsConnected(void) const = 0;
};

} // namespace Nexus
} // namespace ot

#endif // OT_NEXUS_PLATFORM_NEXUS_OBSERVER_HPP_
