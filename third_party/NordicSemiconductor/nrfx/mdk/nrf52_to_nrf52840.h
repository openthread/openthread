/*

Copyright (c) 2010 - 2018, Nordic Semiconductor ASA

All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form, except as embedded into a Nordic
   Semiconductor ASA integrated circuit in a product or a software update for
   such product, must reproduce the above copyright notice, this list of
   conditions and the following disclaimer in the documentation and/or other
   materials provided with the distribution.

3. Neither the name of Nordic Semiconductor ASA nor the names of its
   contributors may be used to endorse or promote products derived from this
   software without specific prior written permission.

4. This software, with or without modification, must only be used with a
   Nordic Semiconductor ASA integrated circuit.

5. Any software provided in binary form under this license must not be reverse
   engineered, decompiled, modified and/or disassembled.

THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND ANY EXPRESS
OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#ifndef NRF52_TO_NRF52840_H
#define NRF52_TO_NRF52840_H

/*lint ++flb "Enter library region */

/* This file is given to prevent your SW from not compiling with the name changes between nRF51 or nRF52832 and nRF52840 devices.
 * It redefines the old nRF51 or nRF52832 names into the new ones as long as the functionality is still supported. If the
 * functionality is gone, there old names are not defined, so compilation will fail. Note that also includes macros
 * from the nrf52_namechange.h file. */
 
/* Differences between latest nRF52 headers and nRF52840 headers. */

/* UART */
/* The registers PSELRTS, PSELTXD, PSELCTS, PSELRXD were restructured into a struct. */
#define PSELRTS       PSEL.RTS
#define PSELTXD       PSEL.TXD
#define PSELCTS       PSEL.CTS
#define PSELRXD       PSEL.RXD

/* TWI */
/* The registers PSELSCL, PSELSDA were restructured into a struct. */
#define PSELSCL       PSEL.SCL
#define PSELSDA       PSEL.SDA


/* LPCOMP */
/* The hysteresis control enumerated values has changed name for nRF52840 devices. */
#define LPCOMP_HYST_HYST_NoHyst     LPCOMP_HYST_HYST_Disabled
#define LPCOMP_HYST_HYST_Hyst50mV   LPCOMP_HYST_HYST_Enabled


/* From nrf52_name_change.h. Several macros changed in different versions of nRF52 headers. By defining the following, any code written for any version of nRF52 headers will still compile. */

/* I2S */
/* Several enumerations changed case. Adding old macros to keep compilation compatibility. */
#define I2S_ENABLE_ENABLE_DISABLE           I2S_ENABLE_ENABLE_Disabled
#define I2S_ENABLE_ENABLE_ENABLE            I2S_ENABLE_ENABLE_Enabled
#define I2S_CONFIG_MODE_MODE_MASTER         I2S_CONFIG_MODE_MODE_Master
#define I2S_CONFIG_MODE_MODE_SLAVE          I2S_CONFIG_MODE_MODE_Slave
#define I2S_CONFIG_RXEN_RXEN_DISABLE        I2S_CONFIG_RXEN_RXEN_Disabled
#define I2S_CONFIG_RXEN_RXEN_ENABLE         I2S_CONFIG_RXEN_RXEN_Enabled
#define I2S_CONFIG_TXEN_TXEN_DISABLE        I2S_CONFIG_TXEN_TXEN_Disabled
#define I2S_CONFIG_TXEN_TXEN_ENABLE         I2S_CONFIG_TXEN_TXEN_Enabled
#define I2S_CONFIG_MCKEN_MCKEN_DISABLE      I2S_CONFIG_MCKEN_MCKEN_Disabled
#define I2S_CONFIG_MCKEN_MCKEN_ENABLE       I2S_CONFIG_MCKEN_MCKEN_Enabled
#define I2S_CONFIG_SWIDTH_SWIDTH_8BIT       I2S_CONFIG_SWIDTH_SWIDTH_8Bit
#define I2S_CONFIG_SWIDTH_SWIDTH_16BIT      I2S_CONFIG_SWIDTH_SWIDTH_16Bit
#define I2S_CONFIG_SWIDTH_SWIDTH_24BIT      I2S_CONFIG_SWIDTH_SWIDTH_24Bit
#define I2S_CONFIG_ALIGN_ALIGN_LEFT         I2S_CONFIG_ALIGN_ALIGN_Left
#define I2S_CONFIG_ALIGN_ALIGN_RIGHT        I2S_CONFIG_ALIGN_ALIGN_Right
#define I2S_CONFIG_FORMAT_FORMAT_ALIGNED    I2S_CONFIG_FORMAT_FORMAT_Aligned
#define I2S_CONFIG_CHANNELS_CHANNELS_STEREO I2S_CONFIG_CHANNELS_CHANNELS_Stereo
#define I2S_CONFIG_CHANNELS_CHANNELS_LEFT   I2S_CONFIG_CHANNELS_CHANNELS_Left
#define I2S_CONFIG_CHANNELS_CHANNELS_RIGHT  I2S_CONFIG_CHANNELS_CHANNELS_Right

/* LPCOMP */
/* Corrected typo in RESULT register. */
#define LPCOMP_RESULT_RESULT_Bellow         LPCOMP_RESULT_RESULT_Below


/*lint --flb "Leave library region" */

#endif /* NRF51_TO_NRF52840_H */

