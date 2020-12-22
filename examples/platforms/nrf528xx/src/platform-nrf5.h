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
 *   This file includes the platform-specific initializers.
 *
 */

#ifndef PLATFORM_NRF5_H_
#define PLATFORM_NRF5_H_

#include <stdint.h>

#include <openthread/instance.h>

#include "platform-config.h"

void nrf5AlarmInit(void);

/**
 * Deinitialization of Alarm driver.
 *
 */
void nrf5AlarmDeinit(void);

/**
 * Function for processing Alarm.
 *
 */
void nrf5AlarmProcess(otInstance *aInstance);

/**
 * Function for geting current time in mircosecond.
 *
 */
uint64_t nrf5AlarmGetCurrentTime(void);

/**
 * Function for getting raw counter value in RTC ticks.
 *
 */
uint64_t nrf5AlarmGetRawCounter(void);

/**
 * Initialization of Random Number Generator.
 *
 */
void nrf5RandomInit(void);

/**
 * Deinitialization of Random Number Generator.
 *
 */
void nrf5RandomDeinit(void);

/**
 * Initialization of Logger driver.
 *
 */
void nrf5LogInit(void);

/**
 * Deinitialization of Logger driver.
 *
 */
void nrf5LogDeinit(void);

/**
 * Initialization of Misc module.
 *
 */
void nrf5MiscInit(void);

/**
 * Deinitialization of Misc module.
 *
 */
void nrf5MiscDeinit(void);

/**
 * Initialization of Radio driver.
 *
 */
void nrf5RadioInit(void);

/**
 * Deinitialization of Radio driver.
 *
 */
void nrf5RadioDeinit(void);

/**
 * Function for processing Radio.
 *
 */
void nrf5RadioProcess(otInstance *aInstance);

/**
 * Function for clearing Radio driver pending events.
 *
 */
void nrf5RadioClearPendingEvents(void);

/**
 * Initialization of hardware crypto engine.
 *
 */
void nrf5CryptoInit(void);

/**
 * Deinitialization of hardware crypto engine.
 *
 */
void nrf5CryptoDeinit(void);

/**
 * Function for erasing page in flash.
 *
 */
otError nrf5FlashPageErase(uint32_t aAddress);

/**
 * Function for checking state of flash driver.
 *
 */
bool nrf5FlashIsBusy(void);

/**
 * Function for writing data into flash.
 *
 */
otError nrf5FlashWrite(uint32_t aAddress, const uint8_t *aData, uint32_t aSize);

/**
 * Initialization of temperature controller.
 *
 */
void nrf5TempInit(void);

/**
 * Deinitialization of temperature controller.
 *
 */
void nrf5TempDeinit(void);

/**
 * Function for processing temperature controller.
 *
 */
void nrf5TempProcess(void);

/**
 * Function for measuring internal temperature.
 *
 * @return Temperature value measured.
 *
 */
int32_t nrf5TempGet(void);

#if SOFTDEVICE_PRESENT
/**
 * Function for translating SoftDevice error into OpenThread's one.
 *
 */
otError nrf5SdErrorToOtError(uint32_t aSdError);

/**
 * Function for processing SoftDevice SoC events in flash module.
 *
 */
void nrf5SdSocFlashProcess(uint32_t aEvtId);
#endif // SOFTDEVICE_PRESENT

int8_t nrf5GetChannelMaxTransmitPower(uint8_t aChannel);

#endif // PLATFORM_NRF5_H_
