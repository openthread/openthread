/*
 *  Copyright (c) 2017, The OpenThread Authors.
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
 *   This file includes definitions for locator class for OpenThread objects.
 */

#ifndef LOCATOR_HPP_
#define LOCATOR_HPP_

#include "openthread-core-config.h"

#include <openthread/types.h>

#include "openthread-single-instance.h"

namespace ot {

class ThreadNetif;
namespace Ip6 { class Ip6; }

/**
 * @addtogroup core-locator
 *
 * @brief
 *   This module includes definitions for OpenThread instance locator.
 *
 * @{
 *
 */


/**
 * This class implements locator for  otInstance object.
 *
 */
class InstanceLocator
{
public:
    /**
     * This method returns a reference to the parent otInstance structure.
     *
     * @returns A reference to the parent otInstance structure.
     *
     */
#if OPENTHREAD_ENABLE_MULTIPLE_INSTANCES
    otInstance &GetInstance(void) const { return mInstance; }
#else
    otInstance &GetInstance(void) const { return *otGetInstance(); }
#endif

    /**
      * This method returns a reference to the Ip6.
      *
      * @returns   A reference to the Ip6.
      *
      */
    Ip6::Ip6 &GetIp6(void) const;

    /**
     * This method returns a reference to the thread network interface.
     *
     * @returns   A reference to the thread network interface.
     *
     */
    ThreadNetif &GetNetif(void) const;

protected:
    /**
     * This constructor initializes the object.
     *
     * @param[in]  aInstance  A pointer to the otInstance.
     *
     */
    InstanceLocator(otInstance &aInstance)
#if OPENTHREAD_ENABLE_MULTIPLE_INSTANCES
        : mInstance(aInstance)
#endif
    {
        OT_UNUSED_VARIABLE(aInstance);
    }

#if OPENTHREAD_ENABLE_MULTIPLE_INSTANCES
private:
    otInstance &mInstance;
#endif
};

/**
 * @}
 *
 */

}  // namespace ot

#endif  // LOCATOR_HPP_
