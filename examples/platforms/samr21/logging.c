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
 * @file logging.c
 * Platform abstraction for the logging
 *
 */

#include "openthread-core-config.h"

#include <utils/code_utils.h>
#include <openthread/platform/logging.h>
#include <openthread/platform/toolchain.h>

#include "board.h"
#include "spi.h"

#if (OPENTHREAD_CONFIG_LOG_OUTPUT == OPENTHREAD_CONFIG_LOG_OUTPUT_PLATFORM_DEFINED)

#if BOARD == SAMR21_XPLAINED_PRO

#define LOG_PARSE_BUFFER_SIZE 128
#define LOG_TIMESTAMP_ENABLE 1

struct spi_module     sMaster;
struct spi_slave_inst sSlave;

char sLogString[LOG_PARSE_BUFFER_SIZE + 1];

static void logOutput(otLogLevel aLogLevel, otLogRegion aLogRegion, const char *aFormat, va_list ap)
{
    int len = 0;

    len = vsnprintf(sLogString, LOG_PARSE_BUFFER_SIZE, aFormat, ap);

    otEXPECT(len >= 0);

exit:

    if (len >= LOG_PARSE_BUFFER_SIZE)
    {
        len = LOG_PARSE_BUFFER_SIZE - 1;
    }

    sLogString[len++] = '\n';

    spi_select_slave(&sMaster, &sSlave, true);

    spi_write_buffer_wait(&sMaster, (uint8_t *)sLogString, len);

    spi_select_slave(&sMaster, &sSlave, false);

    return;
}

#endif

void samr21LogInit(void)
{
#if BOARD == SAMR21_XPLAINED_PRO

    struct spi_slave_inst_config slaveConfig;
    struct spi_config            config;

    spi_slave_inst_get_config_defaults(&slaveConfig);

    slaveConfig.ss_pin = EDBG_SPI_SLAVE_SELECT_PIN;

    spi_attach_slave(&sSlave, &slaveConfig);

    spi_get_config_defaults(&config);

    config.mux_setting                   = EDBG_SPI_SERCOM_MUX_SETTING;
    config.mode_specific.master.baudrate = 8000000UL;
    config.pinmux_pad0                   = EDBG_SPI_SERCOM_PINMUX_PAD0;
    config.pinmux_pad1                   = EDBG_SPI_SERCOM_PINMUX_PAD1;
    config.pinmux_pad2                   = EDBG_SPI_SERCOM_PINMUX_PAD2;
    config.pinmux_pad3                   = EDBG_SPI_SERCOM_PINMUX_PAD3;

    spi_init(&sMaster, EDBG_SPI_MODULE, &config);
    spi_enable(&sMaster);

#endif
}

OT_TOOL_WEAK void otPlatLog(otLogLevel aLogLevel, otLogRegion aLogRegion, const char *aFormat, ...)
{
#if BOARD == SAMR21_XPLAINED_PRO
    va_list ap;

    va_start(ap, aFormat);

    logOutput(aLogLevel, aLogRegion, aFormat, ap);

    va_end(ap);

#else

    OT_UNUSED_VARIABLE(aLogLevel);
    OT_UNUSED_VARIABLE(aLogRegion);
    OT_UNUSED_VARIABLE(aFormat);

#endif
}

#endif
