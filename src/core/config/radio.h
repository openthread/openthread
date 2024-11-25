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
 *   This file includes compile-time configurations for the radio.
 */

#ifndef CONFIG_RADIO_H_
#define CONFIG_RADIO_H_

/**
 * @addtogroup config-radio
 *
 * @brief
 *   This module includes configuration variables for radio.
 *
 * @{
 */

#include "config/mac.h"
#include "config/wakeup.h"

/**
 * @def OPENTHREAD_CONFIG_RADIO_CONTROLLER_ENABLE
 *
 * Indicates whether the radio controller is enabled.
 */

#ifdef OPENTHREAD_CONFIG_RADIO_CONTROLLER_ENABLE
#error "OPENTHREAD_CONFIG_RADIO_CONTROLLER_ENABLE should not be defined directly." \
       "It is derived from OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE and " \
       "OPENTHREAD_CONFIG_WAKEUP_END_DEVICE_ENABLE options."
#endif

#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE || OPENTHREAD_CONFIG_WAKEUP_END_DEVICE_ENABLE
#define OPENTHREAD_CONFIG_RADIO_CONTROLLER_ENABLE 1
#else
#define OPENTHREAD_CONFIG_RADIO_CONTROLLER_ENABLE 0
#endif

/**
 * @}
 */

#endif // CONFIG_RADIO_H_
