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
 * @brief
 *  This file implements the settings functions required for the OpenThread library.
 */

#include "precomp.h"
#include "settings.tmh"

void otPlatSettingsInit(otInstance *otCtx)
{
    NT_ASSERT(otCtx);
    PMS_FILTER pFilter = otCtxToFilter(otCtx);
    UNREFERENCED_PARAMETER(pFilter);
}

ThreadError otPlatSettingsBeginChange(otInstance *otCtx)
{
    NT_ASSERT(otCtx);
    PMS_FILTER pFilter = otCtxToFilter(otCtx);
    UNREFERENCED_PARAMETER(pFilter);
    return kThreadError_NotImplemented;
}

ThreadError otPlatSettingsCommitChange(otInstance *otCtx)
{
    NT_ASSERT(otCtx);
    PMS_FILTER pFilter = otCtxToFilter(otCtx);
    UNREFERENCED_PARAMETER(pFilter);
    return kThreadError_NotImplemented;
}

ThreadError otPlatSettingsAbandonChange(otInstance *otCtx)
{
    NT_ASSERT(otCtx);
    PMS_FILTER pFilter = otCtxToFilter(otCtx);
    UNREFERENCED_PARAMETER(pFilter);
    return kThreadError_NotImplemented;
}

ThreadError otPlatSettingsGet(otInstance *otCtx, uint16_t aKey, int aIndex, uint8_t *aValue, uint16_t *aValueLength)
{
    NT_ASSERT(otCtx);
    PMS_FILTER pFilter = otCtxToFilter(otCtx);
    UNREFERENCED_PARAMETER(pFilter);
    UNREFERENCED_PARAMETER(aKey);
    UNREFERENCED_PARAMETER(aIndex);
    UNREFERENCED_PARAMETER(aValue);
    UNREFERENCED_PARAMETER(aValueLength);
    return kThreadError_NotImplemented;
}

ThreadError otPlatSettingsSet(otInstance *otCtx, uint16_t aKey, const uint8_t *aValue, uint16_t aValueLength)
{
    NT_ASSERT(otCtx);
    PMS_FILTER pFilter = otCtxToFilter(otCtx);
    UNREFERENCED_PARAMETER(pFilter);
    UNREFERENCED_PARAMETER(aKey);
    UNREFERENCED_PARAMETER(aValue);
    UNREFERENCED_PARAMETER(aValueLength);
    return kThreadError_NotImplemented;
}

ThreadError otPlatSettingsAdd(otInstance *otCtx, uint16_t aKey, const uint8_t *aValue, uint16_t aValueLength)
{
    NT_ASSERT(otCtx);
    PMS_FILTER pFilter = otCtxToFilter(otCtx);
    UNREFERENCED_PARAMETER(pFilter);
    UNREFERENCED_PARAMETER(aKey);
    UNREFERENCED_PARAMETER(aValue);
    UNREFERENCED_PARAMETER(aValueLength);
    return kThreadError_NotImplemented;
}

ThreadError otPlatSettingsDelete(otInstance *otCtx, uint16_t aKey, int aIndex)
{
    NT_ASSERT(otCtx);
    PMS_FILTER pFilter = otCtxToFilter(otCtx);
    UNREFERENCED_PARAMETER(pFilter);
    UNREFERENCED_PARAMETER(aKey);
    UNREFERENCED_PARAMETER(aIndex);
    return kThreadError_NotImplemented;
}

void otPlatSettingsWipe(otInstance *otCtx)
{
    NT_ASSERT(otCtx);
    PMS_FILTER pFilter = otCtxToFilter(otCtx);
    UNREFERENCED_PARAMETER(pFilter);
}
