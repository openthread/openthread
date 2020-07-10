/*
 *  Copyright (c) 2020, The OpenThread Authors.
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
 *   This file includes definitions for a child mask.
 */

#ifndef CHILD_MASK_HPP_
#define CHILD_MASK_HPP_

#include "openthread-core-config.h"

#include <openthread/error.h>

#include "common/code_utils.hpp"
#include "common/debug.hpp"
#include "common/encoding.hpp"

namespace ot {

/**
 * @addtogroup core-child-mask
 *
 * @brief
 *   This module includes definitions for OpenThread Child Mask.
 *
 * @{
 *
 */

/**
 * This class represents a bit-vector of child mask.
 *
 */
class ChildMask
{
public:
    /**
     * This method returns if a given Child index is masked.
     *
     * @param[in] aChildIndex  The Child index.
     *
     * @retval TRUE   If the given Child index is set.
     * @retval FALSE  If the given Child index is clear.
     *
     */
    bool Get(uint16_t aChildIndex) const
    {
        OT_ASSERT(aChildIndex < OPENTHREAD_CONFIG_MLE_MAX_CHILDREN);
        return (mMask[aChildIndex / 8] & (0x80 >> (aChildIndex % 8))) != 0;
    }

    /**
     * This method sets the mask of a given Child index.
     *
     * @param[in] aChildIndex  The Child index.
     *
     */
    void Set(uint16_t aChildIndex)
    {
        OT_ASSERT(aChildIndex < OPENTHREAD_CONFIG_MLE_MAX_CHILDREN);
        mMask[aChildIndex / 8] |= 0x80 >> (aChildIndex % 8);
    }

    /**
     * This method clears the mask of a given Child index.
     *
     * @param[in] aChildIndex  The Child index.
     *
     */
    void Clear(uint16_t aChildIndex)
    {
        OT_ASSERT(aChildIndex < OPENTHREAD_CONFIG_MLE_MAX_CHILDREN);
        mMask[aChildIndex / 8] &= ~(0x80 >> (aChildIndex % 8));
    }

    /**
     * This method returns if any Child mask is set.
     *
     * @retval TRUE   If any Child index is set.
     * @retval FALSE  If all Child indexes are clear.
     *
     */
    bool HasAny(void) const
    {
        bool rval = false;

        for (size_t i = 0; i < sizeof(mMask); i++)
        {
            if (mMask[i] != 0)
            {
                ExitNow(rval = true);
            }
        }

    exit:
        return rval;
    }

private:
    enum
    {
        kChildMaskBytes = BitVectorBytes(OPENTHREAD_CONFIG_MLE_MAX_CHILDREN)
    };

    uint8_t mMask[kChildMaskBytes];
};

/**
 * @}
 *
 */

} // namespace ot

#endif // CHILD_MASK_HPP_
