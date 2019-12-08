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
    nrf_fem_interface_config_t cfg;

    memset(&cfg, 0, sizeof(cfg));

    cfg.fem_config.pa_time_gap_us   = aConfig->mFemPhyCfg.mPaTimeGapUs;
    cfg.fem_config.lna_time_gap_us  = aConfig->mFemPhyCfg.mLnaTimeGapUs;
    cfg.fem_config.pdn_settle_us    = aConfig->mFemPhyCfg.mPdnSettleUs;
    cfg.fem_config.trx_hold_us      = aConfig->mFemPhyCfg.mTrxHoldUs;
    cfg.fem_config.pa_gain_db       = aConfig->mFemPhyCfg.mPaGainDb;
    cfg.fem_config.lna_gain_db      = aConfig->mFemPhyCfg.mLnaGainDb;
    cfg.pa_pin_config.enable        = aConfig->mPaCfg.mEnable;
    cfg.pa_pin_config.active_high   = aConfig->mPaCfg.mActiveHigh;
    cfg.pa_pin_config.gpio_pin      = aConfig->mPaCfg.mGpioPin;
    cfg.pa_pin_config.gpiote_ch_id  = aConfig->mPaCfg.mGpioteChId;
    cfg.lna_pin_config.enable       = aConfig->mLnaCfg.mEnable;
    cfg.lna_pin_config.active_high  = aConfig->mLnaCfg.mActiveHigh;
    cfg.lna_pin_config.gpio_pin     = aConfig->mLnaCfg.mGpioPin;
    cfg.lna_pin_config.gpiote_ch_id = aConfig->mLnaCfg.mGpioteChId;
    cfg.pdn_pin_config.enable       = aConfig->mPdnCfg.mEnable;
    cfg.pdn_pin_config.active_high  = aConfig->mPdnCfg.mActiveHigh;
    cfg.pdn_pin_config.gpio_pin     = aConfig->mPdnCfg.mGpioPin;
    cfg.pdn_pin_config.gpiote_ch_id = aConfig->mPdnCfg.mGpioteChId;
    cfg.ppi_ch_id_clr               = aConfig->mPpiChIdClr;
    cfg.ppi_ch_id_set               = aConfig->mPpiChIdSet;
    cfg.ppi_ch_id_pdn               = aConfig->mPpiChIdPdn;

    nrf_fem_interface_configuration_set(&cfg);
}
