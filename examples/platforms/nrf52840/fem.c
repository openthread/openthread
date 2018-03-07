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
 *   This file implements the OpenThread FEM helper functions.
 *
 */

#include <openthread-core-config.h>
#include <openthread/config.h>

#include <stdint.h>
#include <string.h>

#include "platform-fem.h"

#define ENABLE_FEM 1
#include <nrf_802154.h>

void PlatformFemSetConfigParams(const PlatformFemConfigParams *aConfig)
{
    nrf_802154_fem_control_cfg_t cfg;

    memset(&cfg, 0, sizeof(cfg));

    cfg.pa_cfg.enable       = aConfig->mPaCfg.mEnable;
    cfg.pa_cfg.active_high  = aConfig->mPaCfg.mActiveHigh;
    cfg.pa_cfg.gpio_pin     = aConfig->mPaCfg.mGpioPin;
    cfg.lna_cfg.enable      = aConfig->mLnaCfg.mEnable;
    cfg.lna_cfg.active_high = aConfig->mLnaCfg.mActiveHigh;
    cfg.lna_cfg.gpio_pin    = aConfig->mLnaCfg.mGpioPin;
    cfg.ppi_ch_id_clr       = aConfig->mPpiChIdClr;
    cfg.ppi_ch_id_set       = aConfig->mPpiChIdSet;
    cfg.pa_gpiote_ch_id     = aConfig->mGpiotePaChId;
    cfg.lna_gpiote_ch_id    = aConfig->mGpioteLnaChId;

    nrf_802154_fem_control_cfg_set(&cfg);
}
