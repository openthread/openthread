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
 * @brief
 *   This file includes the platform-specific initializers.
 */

#include "asf.h"

#include <openthread/platform/radio.h>

#include "openthread-system.h"

#include "platform-samr21.h"

#include "utils/code_utils.h"

#ifdef CONF_USER_ROW_EXIST

#include "user_row.h"

samr21UserRow *sUserRow = (samr21UserRow *)SAMR21_USER_ROW;

#endif

#ifdef CONF_KIT_DATA_EXIST

#define EDBG_ADDRESS 0x28

#define EDBG_KIT_DATA_TOKEN 0xD2;

#define KIT_DATA_MAX_RETRY 1000

static uint8_t sIeeeEui64[OT_EXT_ADDRESS_SIZE];

struct i2c_master_module sI2cMasterInstance;

static void configureI2cMaster()
{
    /* Create and initialize config structure */
    struct i2c_master_config configI2c;
    i2c_master_get_config_defaults(&configI2c);

    /* Change pins */
    configI2c.pinmux_pad0 = EDBG_I2C_SERCOM_PINMUX_PAD0;
    configI2c.pinmux_pad1 = EDBG_I2C_SERCOM_PINMUX_PAD1;

    /* Initialize and enable device with config */
    i2c_master_init(&sI2cMasterInstance, EDBG_I2C_MODULE, &configI2c);

    i2c_master_enable(&sI2cMasterInstance);
}

static void getKitData()
{
    uint8_t                  requestToken = EDBG_KIT_DATA_TOKEN;
    uint32_t                 timeout;
    struct i2c_master_packet masterPacket;

    /** Send the request token */
    masterPacket.address         = EDBG_ADDRESS;
    masterPacket.data_length     = 1;
    masterPacket.data            = &requestToken;
    masterPacket.ten_bit_address = false;
    masterPacket.high_speed      = false;
    masterPacket.hs_master_code  = 0x0;

    timeout = 0;

    while (i2c_master_write_packet_wait_no_stop(&sI2cMasterInstance, &masterPacket) != STATUS_OK)
    {
        /* Increment timeout counter and check if timed out. */
        otEXPECT(timeout++ < KIT_DATA_MAX_RETRY);
    }

    /** Get the extension boards info */
    masterPacket.data_length = OT_EXT_ADDRESS_SIZE;
    masterPacket.data        = sIeeeEui64;

    timeout = 0;

    while (i2c_master_read_packet_wait(&sI2cMasterInstance, &masterPacket) != STATUS_OK)
    {
        /* Increment timeout counter and check if timed out. */
        otEXPECT(timeout++ < KIT_DATA_MAX_RETRY);
    }

exit:

    return;
}

#endif

otInstance *sInstance;

void boardInit(void)
{
    struct port_config pin_conf;

    port_get_config_defaults(&pin_conf);
    pin_conf.direction = PORT_PIN_DIR_OUTPUT;
    port_pin_set_config(AT86RFX_SPI_SCK, &pin_conf);
    port_pin_set_config(AT86RFX_SPI_MOSI, &pin_conf);
    port_pin_set_config(AT86RFX_SPI_CS, &pin_conf);
    port_pin_set_config(AT86RFX_RST_PIN, &pin_conf);
    port_pin_set_config(AT86RFX_SLP_PIN, &pin_conf);
    port_pin_set_output_level(AT86RFX_SPI_SCK, true);
    port_pin_set_output_level(AT86RFX_SPI_MOSI, true);
    port_pin_set_output_level(AT86RFX_SPI_CS, true);
    port_pin_set_output_level(AT86RFX_RST_PIN, true);
    port_pin_set_output_level(AT86RFX_SLP_PIN, true);

    pin_conf.direction = PORT_PIN_DIR_INPUT;
    port_pin_set_config(AT86RFX_SPI_MISO, &pin_conf);
}

void samr21GetIeeeEui64(otInstance *aInstance, uint8_t *aIeeeEui64)
{
#if defined(CONF_KIT_DATA_EXIST)

    memcpy(aIeeeEui64, sIeeeEui64, OT_EXT_ADDRESS_SIZE);

#elif defined(CONF_USER_ROW_EXIST)

    for (uint8_t i = 0; i < OT_EXT_ADDRESS_SIZE; i++)
    {
        aIeeeEui64[i] = sUserRow->mMacAddress[OT_EXT_ADDRESS_SIZE - i - 1];
    }

#else

#error Platform IEEE EUI-64 shall be provided

#endif
}

void otSysInit(int argc, char *argv[])
{
    system_clock_init();

    boardInit();

#ifdef CONF_KIT_DATA_EXIST
    configureI2cMaster();
    getKitData();
#endif

    samr21AlarmInit();
    samr21RadioInit();
}

bool otSysPseudoResetWasRequested(void)
{
    return false;
}

void otSysDeinit(void)
{
}

void otSysProcessDrivers(otInstance *aInstance)
{
    sInstance = aInstance;

    samr21UartProcess();
    samr21AlarmProcess(aInstance);
    samr21RadioProcess(aInstance);
}
