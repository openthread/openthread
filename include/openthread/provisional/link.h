/*
 *  Copyright (c) 2025, The OpenThread Authors.
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
 *  This file defines the OpenThread provisional IEEE 802.15.4 Link Layer API.
 */
#ifndef OPENTHREAD_PROVISIONAL_LINK_H_
#define OPENTHREAD_PROVISIONAL_LINK_H_

#include <openthread/link.h>
#include <openthread/platform/radio.h>

#ifdef __cplusplus
extern "C" {
#endif
/**
 * @addtogroup api-provisional-link
 *
 * @brief
 *   This module includes provisional functions that control link-layer configuration.
 *
 * @{
 */

/**
 * Represents the wake-up identifier.
 */
typedef uint64_t otWakeupId;

/**
 * Represents the wake-up request type.
 */
typedef enum otWakeupType
{
    OT_WAKEUP_TYPE_EXT_ADDRESS      = 0, ///< Wake up the peer by the extended address.
    OT_WAKEUP_TYPE_IDENTIFIER       = 1, ///< Wake up the peer by the wake-up identifier.
    OT_WAKEUP_TYPE_GROUP_IDENTIFIER = 2, ///< Wake up peers by the group wake-up identifier.
} otWakeupType;

/**
 * Represents the request to wake up the peer.
 */
typedef struct otWakeupRequest
{
    union
    {
        otWakeupId   mWakeupId;   ///< Wake-up identifier of the Wake-up Listener.
        otExtAddress mExtAddress; ///< IEEE 802.15.4 Extended Address of the Wake-up Listener.
    } mShared;

    otWakeupType mType; ///< Indicates the wake-up request type (`OT_WAKEUP_TYPE_*` enumeration).
} otWakeupRequest;

/**
 * @}
 */

#ifdef __cplusplus
} // extern "C"
#endif

#endif // OPENTHREAD_PROVISIONAL_LINK_H_
