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
 *   This file includes the common SoftDevice headers.
 *
 */

#ifndef SOFTDEVICE_H_
#define SOFTDEVICE_H_

// clang-format off
#if defined(__GNUC__)
    _Pragma("GCC diagnostic push")
    _Pragma("GCC diagnostic ignored \"-Wreturn-type\"")
    _Pragma("GCC diagnostic ignored \"-Wunused-parameter\"")
    _Pragma("GCC diagnostic ignored \"-Wpedantic\"")
#endif

#include <nrf_svc.h>
#include <nrf_sdm.h>
#include <nrf_soc.h>
#include <nrf_nvic.h>

#if defined(__GNUC__)
    _Pragma("GCC diagnostic pop")
#endif

/*******************************************************************************
 * @section nRF SoftDevice Handler declarations.
 *
 * @note Definitions for below functions are placed in nRF5 SDK.
 ******************************************************************************/

/**
 * @brief Function for stopping the incoming stack events.
 *
 * This function disables the SoftDevice interrupt. To resume polling for events,
 * call @ref nrf_sdh_resume.
 */
void nrf_sdh_suspend(void);

/**
 * @brief Function for resuming polling incoming events from the SoftDevice.
 */
void nrf_sdh_resume(void);

/**
 * @brief Function for polling stack events from the SoftDevice.
 *
 * The events are passed to the application using the registered event handlers.
 */
void nrf_sdh_evts_poll(void);

// clang-format on
#endif // SOFTDEVICE_H_
