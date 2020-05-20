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

#include <stdbool.h>
#include <stdint.h>

#include <openthread/instance.h>
#include <openthread/platform/toolchain.h>

#include "common/linked_list.hpp"
#include "common/locator.hpp"
#include "common/non_copyable.hpp"
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
 * It can be used to register callbacks that notify of state or configuration changes within OpenThread.
 *
 * Two callback models are provided:
 *
 * - A `Notifier::Callback` object that upon initialization (from its constructor) auto-registers itself with
 *   the `Notifier`. This model is mainly used by OpenThread core modules.
 *
 * - A `otStateChangedCallback` callback handler which needs to be explicitly registered with the `Notifier`. This is
 *   commonly used by external users (provided as an OpenThread public API). Max number of such callbacks that can be
 *   registered at the same time is specified by `OPENTHREAD_CONFIG_MAX_STATECHANGE_HANDLERS` configuration parameter.
 *
 */
class Notifier : public InstanceLocator, private NonCopyable
{
public:
    /**
     * This class defines a `Notifier` callback instance.
     *
     */
    class Callback : public OwnerLocator, public LinkedListEntry<Callback>
    {
        friend class Notifier;
        friend class LinkedListEntry<Callback>;

    public:
        /**
         * This type defines the function pointer which is called to notify of state or configuration changes.
         *
         * @param[in] aCallback    A reference to callback instance.
         * @param[in] aFlags       A bit-field indicating specific state or configuration that has changed.
         *
         */
        typedef void (*Handler)(Callback &aCallback, otChangedFlags aFlags);

        /**
         * This constructor initializes a `Callback` instance and registers it with `Notifier`.
         *
         * @param[in] aInstance   A reference to OpenThread instance.
         * @param[in] aHandler    A function pointer to the callback handler.
         * @param[in] aOwner      A pointer to the owner of the `Callback` instance.
         *
         */
        Callback(Instance &aInstance, Handler aHandler, void *aOwner);

    private:
        void Invoke(otChangedFlags aFlags) { mHandler(*this, aFlags); }

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
     * This method schedules signaling of changed flags.
     *
     * @param[in]  aFlags       A bit-field indicating what configuration or state has changed.
     *
     */
    void Signal(otChangedFlags aFlags);

    /**
     * This method schedules signaling of changed flags only if the set of flags has not been signaled before (first
     * time signal).
     *
     * @param[in]  aFlags       A bit-field indicating what configuration or state has changed.
     *
     */
    void SignalIfFirst(otChangedFlags aFlags);

    /**
     * This method indicates whether or not a changed callback is pending.
     *
     * @returns TRUE if a state changed callback is pending, FALSE otherwise.
     *
     */
    bool IsPending(void) const { return (mFlagsToSignal != 0); }

    /**
     * This method indicates whether or not a changed notification for a given set of flags has been signaled before.
     *
     * @param[in]  aFlags   A bit-field containing the flag-bits to check.
     *
     * @retval TRUE    All flag bits in @p aFlags have been signaled before.
     * @retval FALSE   At least one flag bit in @p aFlags has not been signaled before.
     *
     */
    bool HasSignaled(otChangedFlags aFlags) const { return (mSignaledFlags & aFlags) == aFlags; }

    /**
     * This template method updates a variable of a type `Type` with a new value and signals the given changed flags.
     *
     * If the variable is already set to the same value, this method returns `OT_ERROR_ALREADY` and the changed flags
     * is signaled using `SignalIfFirst()` (i.e. signal is scheduled only if the flag has not been signaled before).
     *
     * The template `Type` should support comparison operator `==` and assignment operator `=`.
     *
     * @param[inout] aVariable    A reference to the variable to update.
     * @param[in]    aNewValue    The new value.
     * @param[in]    aFlags       The changed flags to signal.
     *
     * @retval OT_ERROR_NONE      The variable was update successfully and @p aFlags was signaled.
     * @retval OT_ERROR_ALREADY   The variable was already set to the same value.
     *
     */
    template <typename Type> otError Update(Type &aVariable, const Type &aNewValue, otChangedFlags aFlags)
    {
        otError error = OT_ERROR_NONE;

        if (aVariable == aNewValue)
        {
            SignalIfFirst(aFlags);
            error = OT_ERROR_ALREADY;
        }
        else
        {
            aVariable = aNewValue;
            Signal(aFlags);
        }

        return error;
    }

private:
    enum
    {
        kMaxExternalHandlers   = OPENTHREAD_CONFIG_MAX_STATECHANGE_HANDLERS,
        kFlagsStringLineLimit  = 70, // Character limit to divide the log into multiple lines in `LogChangedFlags()`.
        kMaxFlagNameLength     = 25, // Max length for string representation of a flag by `FlagToString()`.
        kFlagsStringBufferSize = kFlagsStringLineLimit + kMaxFlagNameLength,
    };

    struct ExternalCallback
    {
        otStateChangedCallback mHandler;
        void *                 mContext;
    };

    void        RegisterCallback(Callback &aCallback);
    static void HandleStateChanged(Tasklet &aTasklet);
    void        HandleStateChanged(void);

    void        LogChangedFlags(otChangedFlags aFlags) const;
    const char *FlagToString(otChangedFlags aFlag) const;

    otChangedFlags       mFlagsToSignal;
    otChangedFlags       mSignaledFlags;
    Tasklet              mTask;
    LinkedList<Callback> mCallbacks;
    ExternalCallback     mExternalCallbacks[kMaxExternalHandlers];
};

/**
 * @}
 *
 */

} // namespace ot

#endif // NOTIFIER_HPP_
