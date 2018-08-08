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

#include <openthread/platform/toolchain.h>

namespace ot {

class Instance;
class ThreadNetif;
class Notifier;
namespace Ip6 {
class Ip6;
}

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
 * This class implements a locator for an OpenThread Instance object.
 *
 * The `InstanceLocator` is used as base class of almost all other OpenThread classes. It provides a way for an object
 * to get to its owning/parent OpenThread `Instance` and other objects within the OpenTread hierarchy (e.g. the
 * `ThreadNetif` or `Ip6::Ip6` objects).
 *
 * If multiple-instance feature is supported, the owning/parent OpenThread `Instance` is tracked as a reference. In the
 * single-instance case, this class becomes an empty base class.
 *
 */
class InstanceLocator
{
public:
    /**
     * This method returns a reference to the parent OpenThread Instance.
     *
     * @returns A reference to the parent otInstance.
     *
     */
#if OPENTHREAD_ENABLE_MULTIPLE_INSTANCES
    Instance &GetInstance(void) const { return mInstance; }
#else
    Instance &GetInstance(void) const;
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

    /**
     * This method returns a reference to the Notifier.
     *
     * @returns   A reference to the Notifier.
     *
     */
    Notifier &GetNotifier(void) const;

protected:
    /**
     * This constructor initializes the object.
     *
     * @param[in]  aInstance  A pointer to the otInstance.
     *
     */
    InstanceLocator(Instance &aInstance)
#if OPENTHREAD_ENABLE_MULTIPLE_INSTANCES
        : mInstance(aInstance)
#endif
    {
        OT_UNUSED_VARIABLE(aInstance);
    }

#if OPENTHREAD_ENABLE_MULTIPLE_INSTANCES
private:
    Instance &mInstance;
#endif
};

/**
 * This class implements a locator for owner of an object.
 *
 * This is used as the base class for objects that provide a callback (e.g., `Timer` or `Tasklet`).
 *
 */
class OwnerLocator
{
public:
    /**
     * This template method returns a reference to the owner object.
     *
     * The caller needs to provide the `OwnerType` as part of the template type.
     *
     * @returns A reference to the owner of this object.
     *
     */
    template <typename OwnerType> OwnerType &GetOwner(void)
#if OPENTHREAD_ENABLE_MULTIPLE_INSTANCES
    {
        return *static_cast<OwnerType *>(mOwner);
    }
#else
    // Implemented in `owner-locator.hpp`
    ;
#endif

protected:
    /**
     * This constructor initializes the object
     *
     * @param[in]  aOwner   A pointer to the owner object (as `void *`).
     *
     */
    OwnerLocator(void *aOwner)
#if OPENTHREAD_ENABLE_MULTIPLE_INSTANCES
        : mOwner(aOwner)
#endif
    {
        OT_UNUSED_VARIABLE(aOwner);
    }

#if OPENTHREAD_ENABLE_MULTIPLE_INSTANCES
private:
    void *mOwner;
#endif
};

/**
 * @}
 *
 */

} // namespace ot

#endif // LOCATOR_HPP_
