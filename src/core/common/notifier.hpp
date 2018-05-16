/*
 *  Copyright (c) 2016-2017, The OpenThread Authors.
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
 *  This file defines OpenThread Notifier class.
 */

#ifndef NOTIFIER_HPP_
#define NOTIFIER_HPP_

#include "openthread-core-config.h"

#include "utils/wrap_stdbool.h"
#include "utils/wrap_stdint.h"

#include <openthread/instance.h>
#include <openthread/types.h>

#include "common/locator.hpp"
#include "common/tasklet.hpp"

namespace ot {

/**
 * @addtogroup core-notifier
 *
 * @brief
 *   This module includes definitions for OpenThread Notifier class.
 *
 * @{
 *
 */

/**
 * This class implements the OpenThread Notifier.
 *
 * It can be used to register callbacks to be notified of state or configuration changes within OpenThread.
 *
 */
class Notifier : public InstanceLocator
{
public:
    /**
     * This class defines a callback instance that can be registered with the `Notifier`.
     *
     */
    class Callback : public OwnerLocator
    {
        friend class Notifier;

    public:
        /**
         * This type defines the function pointer which is called to notify of state or configuration changes.
         *
         * @param[in] aCallback    A reference to callback instance.
         * @param[in] aFlags       A bit-field indicating specific state that has changed. See `OT_CHANGED_<STATE>`
         *                         definitions in `instance.h`.
         *
         */
        typedef void (*Handler)(Callback &aCallback, uint32_t aFlags);

        /**
         * This constructor initializes a `Callback` instance
         *
         * @param[in] aHandler    A function pointer to the callback handler.
         * @param[in] aOwner      A pointer to the owner of the `Callback` instance.
         *
         */
        Callback(Handler aHandler, void *aOwner);

    private:
        Handler   mHandler;
        Callback *mNext;
    };

    /**
     * This constructor initializes a `Notifier` instance.
     *
     *  @param[in] aInstance     A reference to OpenThread instance.
     *
     */
    explicit Notifier(Instance &aInstance);

    /**
     * This method registers a callback.
     *
     * @param[in]  aCallback     A reference to the callback instance.
     *
     * @retval OT_ERROR_NONE     Successfully registered the callback.
     * @retval OT_ERROR_ALREADY  The callback was already registered.
     *
     */
    otError RegisterCallback(Callback &aCallback);

    /**
     * This method removes a previously registered callback.
     *
     * @param[in]  aCallback     A reference to the callback instance.
     *
     */
    void RemoveCallback(Callback &aCallback);

    /**
     * This method registers an `otStateChangedCallback` handler.
     *
     * @param[in]  aCallback     A pointer to the handler function that is called to notify of the changes.
     * @param[in]  aContext      A pointer to arbitrary context information.
     *
     * @retval OT_ERROR_NONE     Successfully registered the callback.
     * @retval OT_ERROR_ALREADY  The callback was already registered.
     * @retval OT_ERROR_NO_BUFS  Could not add the callback due to resource constraints.
     *
     */
    otError RegisterCallback(otStateChangedCallback aCallback, void *aContext);

    /**
     * This method removes/unregisters a previously registered `otStateChangedCallback` handler.
     *
     * @param[in]  aCallback     A pointer to the callback function pointer.
     * @param[in]  aContex       A pointer to arbitrary context information.
     *
     */
    void RemoveCallback(otStateChangedCallback aCallback, void *aContext);

    /**
     * This method schedules notification of changed flags.
     *
     * The @p aFlags are combined (bitwise-or) with other flags that have not been provided in a callback yet.
     *
     * @param[in]  aFlags       A bit-field indicating what configuration or state has changed.
     *
     */
    void SetFlags(uint32_t aFlags);

    /**
     * This method indicates whether or not a state changed callback is pending.
     *
     * @retval TRUE if a state changed callback is pending, FALSE otherwise.
     *
     */
    bool IsPending(void) const { return (mFlags != 0); }

private:
    enum
    {
        kMaxExternalHandlers   = OPENTHREAD_CONFIG_MAX_STATECHANGE_HANDLERS,
        kFlagsStringBufferSize = 128,
    };

    struct ExternalCallback
    {
        otStateChangedCallback mHandler;
        void *                 mContext;
    };

    static void HandleStateChanged(Tasklet &aTasklet);
    void        HandleStateChanged(void);

    void        LogChangedFlags(uint32_t aFlags) const;
    const char *FlagToString(uint32_t aFlag) const;

    uint32_t         mFlags;
    Tasklet          mTask;
    Callback *       mCallbacks;
    ExternalCallback mExternalCallbacks[kMaxExternalHandlers];
};

/**
 * @}
 *
 */

} // namespace ot

#endif // NOTIFIER_HPP_
