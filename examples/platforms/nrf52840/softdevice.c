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
 *   This file implements the OpenThread softdevice helper functions.
 *
 */

#include <openthread/config.h>
#include <openthread-core-config.h>

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "softdevice.h"
#include "platform-softdevice.h"
#include "platform-nrf5.h"

#include <nrf_raal_softdevice.h>

otError nrf5SdErrorToOtError(uint32_t aSdError)
{
    switch (aSdError)
    {
    case NRF_SUCCESS:
        return OT_ERROR_NONE;
        break;

    case NRF_ERROR_INVALID_STATE:
    case NRF_ERROR_BUSY:
        return OT_ERROR_INVALID_STATE;
        break;

    case NRF_ERROR_INVALID_PARAM:
    case NRF_ERROR_INVALID_ADDR:
        return OT_ERROR_INVALID_ARGS;
        break;

    case NRF_ERROR_NO_MEM:
        return OT_ERROR_NO_BUFS;
        break;

    case NRF_ERROR_NOT_FOUND:
        return OT_ERROR_NOT_FOUND;
        break;

    case NRF_ERROR_NOT_SUPPORTED:
        return OT_ERROR_NOT_IMPLEMENTED;
        break;

    default:
        return OT_ERROR_FAILED;
        break;
    }
}

void otSysSoftdeviceSocEvtHandler(uint32_t aEvtId)
{
    nrf5SdSocFlashProcess(aEvtId);
    nrf_raal_softdevice_soc_evt_handler(aEvtId);
}

void otSysSoftdeviceRaalConfig(const otSysSoftdeviceRaalConfigParams *aConfig)
{
    nrf_raal_softdevice_cfg_t cfg;
    memset(&cfg, 0, sizeof(cfg));

    cfg.timeslot_length      = aConfig->timeslotLength;
    cfg.timeslot_timeout     = aConfig->timeslotTimeout;
    cfg.timeslot_max_length  = aConfig->timeslotMaxLength;
    cfg.timeslot_alloc_iters = aConfig->timeslotAllocIters;
    cfg.timeslot_safe_margin = aConfig->timeslotSafeMargin;
    cfg.lf_clk_accuracy_ppm  = aConfig->lfClkAccuracyPpm;

    nrf_raal_softdevice_config(&cfg);
}
