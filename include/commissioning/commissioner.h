/*
 *  Copyright (c) 2016, Nest Labs, Inc.
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
 *   This file includes the platform abstraction for the Thread Commissioner role.
 */

#ifndef OPENTHREAD_COMMISSIONER_H_
#define OPENTHREAD_COMMISSIONER_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup core-commissioning
 *
 * @{
 *
 */

/**
 * This function enables the Thread Commissioner role.
 *
 * @param[in]  aInstance  A pointer to an OpenThread instance.
 * @param[in]  aPSKd      A pointer to the PSKd.
 *
 * @retval kThreadError_None         Successfully started the Commissioner role.
 * @retval kThreadError_InvalidArgs  @p aPSKd is invalid.
 *
 */
ThreadError otCommissionerStart(otInstance *aInstance, const char *aPSKd);

/**
 * This function disables the Thread Commissioner role.
 *
 * @param[in]  aInstance  A pointer to an OpenThread instance.
 *
 */
ThreadError otCommissionerStop(otInstance *aInstance);

typedef void (*otCommissionerPanIdConflictCallback)(uint16_t aPanId, uint32_t aChannelMask, void *aContext);

ThreadError otCommissionerPanIdQuery(otInstance *, uint16_t aPanId, uint32_t aChannelMask,
                                     const otIp6Address *aAddress,
                                     otCommissionerPanIdConflictCallback aCallback, void *aContext);

/**
 * @}
 *
 */

#ifdef __cplusplus
}  // end of extern "C"
#endif

#endif  // OPENTHREAD_COMMISSIONER_H_
