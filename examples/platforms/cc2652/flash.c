/*
 *  Copyright (c) 2018, The OpenThread Authors.
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

#include <string.h>

#include <openthread/instance.h>

#include <driverlib/aon_batmon.h>
#include <driverlib/flash.h>
#include <driverlib/interrupt.h>
#include <driverlib/vims.h>

#include <utils/code_utils.h>

#include "platform-cc2652.h"

#define FLASH_BASE_ADDRESS 0x52000
#define FLASH_PAGE_SIZE 0x2000
#define FLASH_PAGE_NUM 2 /* must be a multiple of 2 */
#define FLASH_SWAP_SIZE (FLASH_PAGE_SIZE * (FLASH_PAGE_NUM / 2))

enum
{
    MIN_VDD_FLASH       = 0x18, /* 1.50 volts (0.50=128/256 -> 128=0x80) */
    MAX_WRITE_INCREMENT = 8,    /* maximum number of bytes to write at a time to
                                 * avoid interrupt latency while in ROM
                                 */
};

/**
 * Check if the Battery Monitor measurements and calculations are enabled.
 *
 * @return If the Battery Monitor is enabled.
 * @retval true  The Battery Monitor is on.
 * @retval false The Battery Monitor is off.
 */
static bool isBatMonOn(void)
{
    uint32_t batMonCtl = HWREG(AON_BATMON_BASE + AON_BATMON_O_CTL);
    return ((batMonCtl & AON_BATMON_CTL_CALC_EN_M) == AON_BATMON_CTL_CALC_EN &&
            (batMonCtl & AON_BATMON_CTL_MEAS_EN_M) == AON_BATMON_CTL_MEAS_EN);
}

/**
 * Check if the supply voltage is high enough to support flash programming.
 *
 * @return If the Voltage is too low to support flash programming.
 * @retval false The supply voltage is too low.
 * @retval true  The supply voltage is sufficient.
 */
static bool checkVoltage(void)
{
    bool batMonWasOff = !isBatMonOn();
    bool ret          = false;

    if (batMonWasOff)
    {
        AONBatMonEnable();
    }

    if (AONBatMonBatteryVoltageGet() >= MIN_VDD_FLASH)
    {
        ret = true;
    }

    if (batMonWasOff)
    {
        AONBatMonDisable();
    }

    return ret;
}

/**
 * Disable Flash data caching and instruction pre-fetching.
 *
 * It is necessary to disable the caching and VIMS to ensure the cache has
 * valid data while the program is executing.
 *
 * @return The VIMS state before being disabled.
 */
static uint32_t disableFlashCache(void)
{
    uint32_t mode = VIMSModeGet(VIMS_BASE);

    VIMSLineBufDisable(VIMS_BASE);

    if (mode != VIMS_MODE_DISABLED)
    {
        VIMSModeSet(VIMS_BASE, VIMS_MODE_DISABLED);

        while (VIMSModeGet(VIMS_BASE) != VIMS_MODE_DISABLED)
            ;
    }

    return mode;
}

/**
 * Restore the Flash data caching and instruction pre-fetching.
 *
 * @param [in] mode The VIMS mode returned by @ref disableFlashCache.
 */
static void restoreFlashCache(uint32_t mode)
{
    if (mode != VIMS_MODE_DISABLED)
    {
        VIMSModeSet(VIMS_BASE, mode);
    }

    VIMSLineBufEnable(VIMS_BASE);
}

static uint32_t mapAddress(uint8_t aSwapIndex, uint32_t aOffset)
{
    uint32_t address = FLASH_BASE_ADDRESS + aOffset;

    if (aSwapIndex)
    {
        address += FLASH_SWAP_SIZE;
    }

    return address;
}

/**
 * Function documented in platforms/utils/flash.h
 */
void otPlatFlashInit(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
}

/**
 * Function documented in platforms/utils/flash.h
 */
uint32_t otPlatFlashGetSwapSize(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    return FLASH_SWAP_SIZE;
}

/**
 * Function documented in platforms/utils/flash.h
 */
void otPlatFlashErase(otInstance *aInstance, uint8_t aSwapIndex)
{
    OT_UNUSED_VARIABLE(aInstance);

    uint32_t mode;

    otEXPECT(checkVoltage());

    mode = disableFlashCache();

    for (uint8_t page = 0; page < FLASH_PAGE_NUM; page++)
    {
        FlashSectorErase(mapAddress(aSwapIndex, (page * FLASH_PAGE_SIZE)));
    }

    restoreFlashCache(mode);

    while (FlashCheckFsmForReady() != FAPI_STATUS_FSM_READY)
    {
    }

exit:
    return;
}

/**
 * Function documented in platforms/utils/flash.h
 */
void otPlatFlashWrite(otInstance *aInstance, uint8_t aSwapIndex, uint32_t aOffset, const void *aData, uint32_t aSize)
{
    OT_UNUSED_VARIABLE(aInstance);

    uint32_t mode;
    uint32_t written = 0;
    uint32_t address;

    otEXPECT(checkVoltage());

    mode = disableFlashCache();

    address = mapAddress(aSwapIndex, aOffset);

    while (written < aSize)
    {
        uint32_t       toWrite = aSize - written;
        const uint8_t *data    = (uint8_t *)aData + written;
        uint32_t       fsmRet;
        bool           interruptsWereDisabled;

        if (toWrite > MAX_WRITE_INCREMENT)
        {
            toWrite = MAX_WRITE_INCREMENT;
        }

        /* The CPU may not execute code from flash while a program is
         * happening. We disable interrupts to ensure one does not preempt the
         * ROM fsm.
         */
        interruptsWereDisabled = IntMasterDisable();

        fsmRet = FlashProgram((uint8_t *)data, address + written, toWrite);

        if (!interruptsWereDisabled)
        {
            IntMasterEnable();
        }

        if (fsmRet != FAPI_STATUS_SUCCESS)
        {
            break;
        }

        written += toWrite;
    }

    restoreFlashCache(mode);

exit:
    return;
}

/**
 * Function documented in platforms/utils/flash.h
 */
void otPlatFlashRead(otInstance *aInstance, uint8_t aSwapIndex, uint32_t aOffset, void *aData, uint32_t aSize)
{
    OT_UNUSED_VARIABLE(aInstance);

    memcpy(aData, (void *)mapAddress(aSwapIndex, aOffset), (size_t)aSize);
}
