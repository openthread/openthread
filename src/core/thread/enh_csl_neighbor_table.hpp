/*
 *  Copyright (c) 2024, The OpenThread Authors.
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
 *   This file includes definitions for the CSL neighbors table.
 */

#ifndef ENH_CSL_NEIGHBOR_TABLE_HPP_
#define ENH_CSL_NEIGHBOR_TABLE_HPP_

#include "openthread-core-config.h"

#if OPENTHREAD_CONFIG_WAKEUP_END_DEVICE_ENABLE

#include "common/const_cast.hpp"
#include "common/iterator_utils.hpp"
#include "common/locator.hpp"
#include "common/non_copyable.hpp"
#include "thread/neighbor.hpp"

namespace ot {

/**
 * Represents the CSL neighbors table.
 */
class CslNeighborTable : private NonCopyable
{
public:
    /**
     * Initializes a `CslNeighborTable` instance.
     *
     * @param[in]  aInstance     A reference to the OpenThread instance.
     */
    explicit CslNeighborTable(Instance &aInstance);

    /**
     * Gets a new/unused `CslNeighbor` entry from the enhanced CSL neighbor table.
     *
     * @note The returned neighbor entry will be cleared (`memset` to zero).
     *
     * @returns A pointer to a new `CslNeighbor` entry, or `nullptr` if all `CslNeighbor` entries are in use.
     */
    CslNeighbor *GetNewCslNeighbor(void);

    /**
     * Gets the first `CslNeighbor` entry in the enhanced CSL neighbor table.
     *
     * @returns A pointer to the first `CslNeighbor` entry, or `nullptr` if the table is empty.
     */
    CslNeighbor *GetFirstCslNeighbor(void);

private:
    // static constexpr uint16_t kMaxCslNeighbors = OPENTHREAD_CONFIG_MLE_MAX_ENH_CSL_NEIGHBORS;
    static constexpr uint16_t kMaxCslNeighbors = 1;

    CslNeighbor    mCslNeighbors[kMaxCslNeighbors];
};

} // namespace ot

#endif // OPENTHREAD_CONFIG_WAKEUP_END_DEVICE_ENABLE

#endif // ENH_CSL_NEIGHBOR_TABLE_HPP_
