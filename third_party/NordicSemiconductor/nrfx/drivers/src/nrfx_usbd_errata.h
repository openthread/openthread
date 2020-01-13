/*
 * Copyright (c) 2016 - 2019, Nordic Semiconductor ASA
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef NRFX_USBD_ERRATA_H__
#define NRFX_USBD_ERRATA_H__

#include <stdbool.h>

#ifndef NRFX_USBD_ERRATA_ENABLE
/**
 * @brief The constant that informs if errata should be enabled at all.
 *
 * If this constant is set to 0, all the Errata bug fixes will be automatically disabled.
 */
#define NRFX_USBD_ERRATA_ENABLE 1
#endif

static inline bool nrfx_usbd_errata_type_52840(void)
{
    return (*(uint32_t const *)0x10000130UL == 0x8UL);
}

static inline bool nrfx_usbd_errata_type_52840_eng_a_or_later(void)
{
    return nrfx_usbd_errata_type_52840();
}

static inline bool nrfx_usbd_errata_type_52840_eng_b_or_later(void)
{
    return (nrfx_usbd_errata_type_52840() && (*(uint32_t const *)0x10000134UL >= 0x1UL));
}

static inline bool nrfx_usbd_errata_type_52840_eng_c_or_later(void)
{
    return (nrfx_usbd_errata_type_52840() && (*(uint32_t const *)0x10000134UL >= 0x2UL));
}

static inline bool nrfx_usbd_errata_type_52840_eng_d_or_later(void)
{
    return (nrfx_usbd_errata_type_52840() && (*(uint32_t const *)0x10000134UL >= 0x3UL));
}

static inline bool nrfx_usbd_errata_type_52833(void)
{
    return (*(uint32_t const *)0x10000130UL == 0x0DUL);
}

static inline bool nrfx_usbd_errata_type_52833_eng_a_or_later(void)
{
    return nrfx_usbd_errata_type_52833();
}

/* Errata: USBD: EPDATA event is not always generated.
 *
 * Applies to nRF52840 Engineering A.
 **/
static inline bool nrfx_usbd_errata_104(void)
{
    return (NRFX_USBD_ERRATA_ENABLE && (nrfx_usbd_errata_type_52840()
                                        && !nrfx_usbd_errata_type_52840_eng_b_or_later()));
}

/* Errata: During setup read/write transfer USBD acknowledges setup stage without SETUP task.
 *
 * Applies to nRF52840 Engineering A.
 **/
static inline bool nrfx_usbd_errata_154(void)
{
    return (NRFX_USBD_ERRATA_ENABLE && (nrfx_usbd_errata_type_52840()
                                        && !nrfx_usbd_errata_type_52840_eng_b_or_later()));
}

/* Errata: ISO double buffering not functional.
 *
 * Applies to nRF52840.
 **/
static inline bool nrfx_usbd_errata_166(void)
{
    return (NRFX_USBD_ERRATA_ENABLE && nrfx_usbd_errata_type_52840());
}

/* Errata: USBD might not reach its active state.
 *
 * Applies to nRF52840.
 **/
static inline bool nrfx_usbd_errata_171(void)
{
    return (NRFX_USBD_ERRATA_ENABLE && nrfx_usbd_errata_type_52840());
}

/* Errata: USB cannot be enabled.
 *
 * Applies to nRF52840 Engineering B or later and nRF52833.
 **/
static inline bool nrfx_usbd_errata_187(void)
{
    return (NRFX_USBD_ERRATA_ENABLE && (nrfx_usbd_errata_type_52840_eng_b_or_later()
                                        || nrfx_usbd_errata_type_52833()));
}

/* Errata: USBD cannot receive tasks during DMA.
 *
 * Applies to nRF52840.
 **/
static inline bool nrfx_usbd_errata_199(void)
{
    return (NRFX_USBD_ERRATA_ENABLE && nrfx_usbd_errata_type_52840());
}

/* Errata: SIZE.EPOUT not writable.
 *
 * Applies to nRF52840 Engineering A.
 **/
static inline bool nrfx_usbd_errata_200(void)
{
    return (NRFX_USBD_ERRATA_ENABLE && (nrfx_usbd_errata_type_52840()
                                        && !nrfx_usbd_errata_type_52840_eng_b_or_later()));
}

#endif // NRFX_USBD_ERRATA_H__
