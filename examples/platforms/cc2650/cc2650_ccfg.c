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

/*
 * Configure the Customer Configuration Area.
 */

// clang-format off

// enable bootloader backdoor
#define SET_CCFG_BL_CONFIG_BOOTLOADER_ENABLE            0xC5       // Enable ROM boot loader

#define SET_CCFG_BL_CONFIG_BL_LEVEL                     0x0        // Active low to open boot loader backdoor

#define SET_CCFG_BL_CONFIG_BL_PIN_NUMBER                0x0D       // DIO13 (BTN-1 button) on CC2650 LaunchPad Board for boot loader backdoor
// #define SET_CCFG_BL_CONFIG_BL_PIN_NUMBER             0x0B       // DIO11 (SELECT button) on CC2650DK (QFN48/7*7) for boot loader backdoor

#define SET_CCFG_BL_CONFIG_BL_ENABLE                    0xC5       // Enabled boot loader backdoor

#define SET_CCFG_IMAGE_VALID_CONF_IMAGE_VALID           0x00000000 // Flash image is valid

/*
 * Include the default ccfg struct and configuration code.
 */
#include <startup_files/ccfg.c>

// clang-format on
