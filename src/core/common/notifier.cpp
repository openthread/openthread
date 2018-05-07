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
 *   This file implements the Notifier class.
 */

#define WPP_NAME "notifier.tmh"

#include "notifier.hpp"

#include "common/code_utils.hpp"
#include "common/logging.hpp"
#include "common/owner-locator.hpp"

namespace ot {

Notifier::Callback::Callback(Handler aHandler, void *aOwner)
    : OwnerLocator(aOwner)
    , mHandler(aHandler)
    , mNext(this)
{
}

Notifier::Notifier(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mFlags(0)
    , mTask(aInstance, &Notifier::HandleStateChanged, this)
    , mCallbacks(NULL)
{
    for (unsigned int i = 0; i < kMaxExternalHandlers; i++)
    {
        mExternalCallbacks[i].mHandler = NULL;
        mExternalCallbacks[i].mContext = NULL;
    }
}

otError Notifier::RegisterCallback(Callback &aCallback)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(aCallback.mNext == &aCallback, error = OT_ERROR_ALREADY);

    aCallback.mNext = mCallbacks;
    mCallbacks      = &aCallback;

exit:
    return error;
}

void Notifier::RemoveCallback(Callback &aCallback)
{
    VerifyOrExit(mCallbacks != NULL);

    if (mCallbacks == &aCallback)
    {
        mCallbacks = mCallbacks->mNext;
        ExitNow();
    }

    for (Callback *callback = mCallbacks; callback->mNext != NULL; callback = callback->mNext)
    {
        if (callback->mNext == &aCallback)
        {
            callback->mNext = aCallback.mNext;
            ExitNow();
        }
    }

exit:
    aCallback.mNext = &aCallback;
}

otError Notifier::RegisterCallback(otStateChangedCallback aCallback, void *aContext)
{
    otError           error          = OT_ERROR_NONE;
    ExternalCallback *unusedCallback = NULL;

    VerifyOrExit(aCallback != NULL);

    for (unsigned int i = 0; i < kMaxExternalHandlers; i++)
    {
        ExternalCallback &callback = mExternalCallbacks[i];

        if (callback.mHandler == NULL)
        {
            if (unusedCallback == NULL)
            {
                unusedCallback = &callback;
            }

            continue;
        }

        VerifyOrExit((callback.mHandler != aCallback) || (callback.mContext != aContext), error = OT_ERROR_ALREADY);
    }

    VerifyOrExit(unusedCallback != NULL, error = OT_ERROR_NO_BUFS);

    unusedCallback->mHandler = aCallback;
    unusedCallback->mContext = aContext;

exit:
    return error;
}

void Notifier::RemoveCallback(otStateChangedCallback aCallback, void *aContext)
{
    VerifyOrExit(aCallback != NULL);

    for (unsigned int i = 0; i < kMaxExternalHandlers; i++)
    {
        ExternalCallback &callback = mExternalCallbacks[i];

        if ((callback.mHandler == aCallback) && (callback.mContext == aContext))
        {
            callback.mHandler = NULL;
            callback.mContext = NULL;
        }
    }

exit:
    return;
}

void Notifier::SetFlags(uint32_t aFlags)
{
    mFlags |= aFlags;
    mTask.Post();
}

void Notifier::HandleStateChanged(Tasklet &aTasklet)
{
    aTasklet.GetOwner<Notifier>().HandleStateChanged();
}

void Notifier::HandleStateChanged(void)
{
    uint32_t flags = mFlags;

    VerifyOrExit(flags != 0);

    mFlags = 0;

    LogChangedFlags(flags);

    for (Callback *callback = mCallbacks; callback != NULL; callback = callback->mNext)
    {
        if (callback->mHandler != NULL)
        {
            callback->mHandler(*callback, flags);
        }
    }

    for (unsigned int i = 0; i < kMaxExternalHandlers; i++)
    {
        ExternalCallback &callback = mExternalCallbacks[i];

        if (callback.mHandler != NULL)
        {
            callback.mHandler(flags, callback.mContext);
        }
    }

exit:
    return;
}

#if (OPENTHREAD_CONFIG_LOG_LEVEL >= OT_LOG_LEVEL_INFO) && (OPENTHREAD_CONFIG_LOG_MAC == 1)

void Notifier::LogChangedFlags(uint32_t aFlags) const
{
    uint32_t flags = aFlags;
    char     stringBuffer[kFlagsStringBufferSize];
    char *   buf = stringBuffer;
    int      len = sizeof(stringBuffer) - 1;
    int      charsWritten;

    for (uint8_t bit = 0; bit < 32; bit++)
    {
        VerifyOrExit(flags != 0);

        if (flags & (1 << bit))
        {
            charsWritten = snprintf(buf, static_cast<size_t>(len), "%s ", FlagToString(1 << bit));
            VerifyOrExit(charsWritten >= 0 && charsWritten < len);
            buf += charsWritten;
            len -= charsWritten;

            flags ^= (1 << bit);
        }
    }

exit:
    stringBuffer[sizeof(stringBuffer) - 1] = 0;
    otLogInfoCore(GetInstance(), "Notifier: StateChanged (0x%04x) [ %s] ", aFlags, stringBuffer);
}

const char *Notifier::FlagToString(uint32_t aFlag) const
{
    const char *retval = "(unknown)";

    switch (aFlag)
    {
    case OT_CHANGED_IP6_ADDRESS_ADDED:
        retval = "Ip6+";
        break;

    case OT_CHANGED_IP6_ADDRESS_REMOVED:
        retval = "Ip6-";
        break;

    case OT_CHANGED_THREAD_ROLE:
        retval = "Role";
        break;

    case OT_CHANGED_THREAD_LL_ADDR:
        retval = "LLAddr";
        break;

    case OT_CHANGED_THREAD_ML_ADDR:
        retval = "MLAddr";
        break;

    case OT_CHANGED_THREAD_RLOC_ADDED:
        retval = "Rloc+";
        break;

    case OT_CHANGED_THREAD_RLOC_REMOVED:
        retval = "Rloc-";
        break;

    case OT_CHANGED_THREAD_PARTITION_ID:
        retval = "PartitionId";
        break;

    case OT_CHANGED_THREAD_KEY_SEQUENCE_COUNTER:
        retval = "KeySeqCntr";
        break;

    case OT_CHANGED_THREAD_NETDATA:
        retval = "NetData";
        break;

    case OT_CHANGED_THREAD_CHILD_ADDED:
        retval = "Child+";
        break;

    case OT_CHANGED_THREAD_CHILD_REMOVED:
        retval = "Child-";
        break;

    case OT_CHANGED_IP6_MULTICAST_SUBSRCRIBED:
        retval = "Ip6Mult+";
        break;

    case OT_CHANGED_IP6_MULTICAST_UNSUBSRCRIBED:
        retval = "Ip6Mult-";
        break;

    case OT_CHANGED_COMMISSIONER_STATE:
        retval = "CommissionerState";
        break;

    case OT_CHANGED_JOINER_STATE:
        retval = "JoinerState";
        break;

    case OT_CHANGED_THREAD_CHANNEL:
        retval = "Channel";
        break;

    case OT_CHANGED_THREAD_PANID:
        retval = "PanId";
        break;

    case OT_CHANGED_THREAD_NETWORK_NAME:
        retval = "NetName";
        break;

    case OT_CHANGED_THREAD_EXT_PANID:
        retval = "ExtPanId";
        break;

    case OT_CHANGED_MASTER_KEY:
        retval = "MstrKey";
        break;

    case OT_CHANGED_PSKC:
        retval = "PSKc";
        break;

    case OT_CHANGED_SECURITY_POLICY:
        retval = "SecPolicy";
        break;

    case OT_CHANGED_CHANNEL_MANAGER_NEW_CHANNEL:
        retval = "CMNewChan";
        break;

    default:
        break;
    }

    return retval;
}

#else // #if (OPENTHREAD_CONFIG_LOG_LEVEL >= OT_LOG_LEVEL_INFO) && (OPENTHREAD_CONFIG_LOG_MAC == 1)

void Notifier::LogChangedFlags(uint32_t aFlags) const
{
    OT_UNUSED_VARIABLE(aFlags);
}

const char *Notifier::FlagToString(uint32_t aFlag) const
{
    OT_UNUSED_VARIABLE(aFlag);
    return NULL;
}

#endif // #if (OPENTHREAD_CONFIG_LOG_LEVEL >= OT_LOG_LEVEL_INFO) && (OPENTHREAD_CONFIG_LOG_MAC == 1)

} // namespace ot
