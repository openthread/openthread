/*
 *  Copyright (c) 2018, The OpenThread Authors.
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
 *   This file includes definitions for manipulating IPv6 flow label table.
 *
 */

#ifndef IP6_FLOW_LABELS_HPP_
#define IP6_FLOW_LABELS_HPP_

#include <stdint.h>

#include "openthread-core-config.h"
#include <openthread/ip6.h>

#include "common/code_utils.hpp"
#include "common/locator.hpp"
#include "common/timer.hpp"

#if OPENTHREAD_ENABLE_IP6_FLOW_LABELS

namespace ot {
namespace Ip6 {

/**
 * @addtogroup core-ip6
 *
 * @{
 *
 */

/**
 * This class manages IP6 flow labels.
 *
 */
class FlowLabels : public InstanceLocator
{
public:
    /**
     * This constructor initializes the object.
     *
     * @param[in]  aInstance  A reference to the OpenThread instance.
     *
     */
    explicit FlowLabels(Instance &aInstance);

    /**
     * This method returns whether or not the flow label table contains the given flow label.
     *
     * @retval TRUE   If flow label table contains the given flow label.
     * @retval FALSE  If flow label table doesn't contain the given flow label.
     *
     */
    bool ContainsFlowLabel(uint32_t aFlowLabel);

    /**
     * This method adds an IPv6 flow label.
     *
     * @param[in]  aFlowLabel  A flow label value.
     *
     * @retval OT_ERROR_NONE    Successfully added the flow label.
     * @retval OT_ERROR_ALREADY The flow label was already added.
     * @retval OT_ERROR_NO_BUFS Insufficient buffer space available to add flow label.
     *
     */
    otError AddFlowLabel(uint32_t aFlowLabel);

    /**
     * This method removes an IPv6 flow label.
     *
     * @param[in]  aFlowLabel  A flow label value.
     * @param[in]  aDelay      The delay to remove flow label (in seconds).
     *
     * @retval OT_ERROR_NONE       Successfully removed the flow label.
     * @retval OT_ERROR_NOT_FOUND  The flow label was not added.
     *
     */
    otError RemoveFlowLabel(uint32_t aFlowLabel, uint8_t aDelay);

    /**
     * This method gets an in-use IP6 flow label.
     *
     * @param[inout]  aIterator  A reference to the IP6 flow label iterator. To get the first in-use IP6
     *                           flow label, it should be set to OT_IP6_FLOW_LABEL_ITERATOR_INIT.
     * @param[out]    aFlowLabel A flow label value.
     *
     * @retval OT_ERROR_NONE       Successfully retrieved an in-use IP6 flow label.
     * @retval OT_ERROR_NOT_FOUND  No subsequent flow label exists.
     *
     */
    otError GetNextFlowLabel(otIp6FlowLabelIterator &aIterator, uint32_t &aFlowLabel);

private:
    typedef struct FlowLabelEntry
    {
        uint32_t mFlowLabel : 20; ///< The IP6 flow label value.
        bool     mValid : 1;      ///< Indicates whether the entry is valid.
        uint8_t  mDelay : 8;      ///< The delay to remove flow label (in seconds).
    } FlowLabelEntry;

    enum
    {
        kNumFlowLabelEntries = OPENTHREAD_CONFIG_IP6_FLOW_LABELS_SIZE, ///< The number of flow label entries.
        kUpdatePeriod        = 1000,                                   ///< Update period in milliseconds.
        kFlowLabelMask       = 0x000fffff,                             ///< The mask for flow label.
    };

    static void HandleTimer(Timer &aTimer);
    void        HandleTimer(void);

    FlowLabelEntry *FindFlowLabelEntry(uint32_t aFlowLabel, bool aValid);

    FlowLabelEntry mEntries[kNumFlowLabelEntries];
    TimerMilli     mTimer;
};

/**
 * @}
 *
 */

} // namespace Ip6
} // namespace ot

#endif // OPENTHREAD_ENABLE_IP6_FLOW_LABELS

#endif // IP6_FLOW_LABELS_HPP_
