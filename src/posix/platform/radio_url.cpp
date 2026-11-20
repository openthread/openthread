/*
 *  Copyright (c) 2020, The OpenThread Authors.
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

#include "posix/platform/radio_url.hpp"

#include <stdio.h>

#include <openthread/openthread-system.h>

#include "common/code_utils.hpp"
#include "posix/platform/platform-posix.h"

const char *otSysGetRadioUrlHelpString(void)
{
#define OT_RADIO_URL_HELP_BUS                            \
    "Radio URL format:"                                  \
    "    {Protocol}://${PATH_TO_DEVICE}?${Parameters}\n" \
    "\n"

#if OPENTHREAD_POSIX_CONFIG_SPINEL_SPI_INTERFACE_ENABLE
#define OT_SPINEL_SPI_RADIO_URL_HELP_BUS                                                                       \
    "Protocol=[spinel+spi*]            Specify the Spinel interface as the Spinel SPI interface\n"             \
    "    spinel+spi://${PATH_TO_SPI_DEVICE}?${Parameters}\n"                                                   \
    "Parameters:\n"                                                                                            \
    "    gpio-int-device[=gpio-device-path]\n"                                                                 \
    "                                  Specify a path to the Linux GPIO character device for the\n"            \
    "                                  `I̅N̅T̅` pin.\n"                                                           \
    "    gpio-int-line[=line-offset]\n"                                                                        \
    "                                  The offset index of `I̅N̅T̅` pin for the associated GPIO device.\n"        \
    "    gpio-reset-device[=gpio-device-path]\n"                                                               \
    "                                  Specify a path to the Linux GPIO character device for the\n"            \
    "                                  `R̅E̅S̅` pin. If not specified, the process will directly exit when an\n"  \
    "                                  RCP reset is required.\n"                                               \
    "    gpio-reset-line[=line-offset]\n"                                                                      \
    "                                  The offset index of `R̅E̅S̅` pin for the associated GPIO device. This\n"   \
    "                                  is required if `gpio-reset-device` is specified.\n"                     \
    "    spi-mode[=mode]               Specify the SPI mode to use (0-3), default is 0.\n"                     \
    "    spi-speed[=hertz]             Specify the SPI speed in hertz.\n"                                      \
    "    spi-cs-delay[=usec]           Specify the delay after C̅S̅ assertion, in µsec, default is 20.\n"        \
    "    spi-reset-delay[=ms]          Specify the delay after R̅E̅S̅E̅T̅ deassertion, in milliseconds,\n"          \
    "                                  default is 0.\n"                                                        \
    "    spi-align-allowance[=n]       Specify the maximum number of 0xFF/0x00 bytes to clip from start of\n"  \
    "                                  MISO frame. Max value is 16, default is 16.\n"                          \
    "    spi-small-packet[=n]          Specify the smallest packet we can receive in a single transaction\n"   \
    "                                  (larger packets will require two transactions). Default value is 32.\n" \
    "\n"
#else
#define OT_SPINEL_SPI_RADIO_URL_HELP_BUS
#endif // OPENTHREAD_POSIX_CONFIG_SPINEL_SPI_INTERFACE_ENABLE

#if OPENTHREAD_POSIX_CONFIG_SPINEL_HDLC_INTERFACE_ENABLE
#if OPENTHREAD_POSIX_CONFIG_RCP_PTY_ENABLE
#define OT_SPINEL_HDLC_FORKPTY_URL_HELP \
    "    spinel+hdlc+forkpty://${PATH_TO_EXECUTABLE}?${Parameters} for forking a pty subprocess.\n"
#define OT_SPINEL_HDLC_FORKPTY_PARAM_HELP \
    "    forkpty-arg[=argument]        Command line arguments for subprocess, can be repeated.\n"
#else
#define OT_SPINEL_HDLC_FORKPTY_URL_HELP
#define OT_SPINEL_HDLC_FORKPTY_PARAM_HELP
#endif

#define OT_SPINEL_HDLC_RADIO_URL_HELP_BUS                                                                            \
    "Protocol=[spinel+hdlc*]           Specify the Spinel interface as the Spinel HDLC interface\n"                  \
    "    spinel+hdlc+uart://${PATH_TO_UART_DEVICE}?${Parameters} for a real UART "                                   \
    "device.\n" OT_SPINEL_HDLC_FORKPTY_URL_HELP                                                                      \
    "    spinel+hdlc+unix://${PATH_TO_UNIX_SOCKET}?${Parameters} for unix stream socket.\n"                          \
    "Parameters:\n"                                                                                                  \
    "    uart-parity[=even|odd]        UART parity config, optional.\n"                                              \
    "    uart-stop[=number-of-bits]    UART stop bit, default is 1.\n"                                               \
    "    uart-baudrate[=baudrate]      UART baud rate, default is 460800.\n"                                         \
    "    uart-flow-control             Enable flow control, disabled by default.\n"                                  \
    "    uart-init-deassert            Deassert DTR/RTS lines on init when flow control is disabled.\n"              \
    "    uart-reset                    Reopen UART device after hard resetting RCP (USB CDC ACM).\n"                 \
    "    uart-exclusive                Lock UART device using flock / TIOCEXCL.\n" OT_SPINEL_HDLC_FORKPTY_PARAM_HELP \
    "\n"
#else
#define OT_SPINEL_HDLC_RADIO_URL_HELP_BUS
#endif // OPENTHREAD_POSIX_CONFIG_SPINEL_HDLC_INTERFACE_ENABLE

#if OPENTHREAD_POSIX_CONFIG_SPINEL_VENDOR_INTERFACE_ENABLE

#ifndef OT_VENDOR_RADIO_URL_HELP_BUS
#define OT_VENDOR_RADIO_URL_HELP_BUS "\n"
#endif // OT_VENDOR_RADIO_URL_HELP_BUS

#define OT_SPINEL_VENDOR_RADIO_URL_HELP_BUS OT_VENDOR_RADIO_URL_HELP_BUS
#else
#define OT_SPINEL_VENDOR_RADIO_URL_HELP_BUS
#endif // OPENTHREAD_POSIX_CONFIG_SPINEL_VENDOR_INTERFACE_ENABLE

#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE && !OPENTHREAD_POSIX_CONFIG_TREL_SELECT_INFRA_IF
#define OT_TREL_RADIO_URL_HELP_BUS                                                                 \
    "\n"                                                                                           \
    "Protocol=[trel]                   Specify the TREL interface\n"                               \
    "    trel://${INFRA_IF_NAME}       TREL over the specified infrastructure network interface\n" \
    "                                  (no query parameters supported).\n"
#else
#define OT_TREL_RADIO_URL_HELP_BUS
#endif

#if OPENTHREAD_POSIX_CONFIG_MAX_POWER_TABLE_ENABLE
#define OT_RADIO_URL_HELP_MAX_POWER_TABLE                                                                \
    "    max-power-table-mbm           Max power (in 0.01 dBm) for channels in ascending order\n"        \
    "                                  separated by commas. If the number of values is less than that\n" \
    "                                  of supported channels, the last value will be applied to all\n"   \
    "                                  remaining channels. Special value 0x7fff disables a channel.\n"

// Only meaningful while max-power-table-mbm is compiled in; without it the note
// would point at a parameter the help does not list.
#define OT_RADIO_URL_HELP_TX_POWER_CAP                                                                    \
    "                                  max-power-table-mbm only caps the power; the radio transmits at\n" \
    "                                  the lower of tx-power and the channel's max-power-table-mbm\n"     \
    "                                  limit.\n"
#else
#define OT_RADIO_URL_HELP_MAX_POWER_TABLE
#define OT_RADIO_URL_HELP_TX_POWER_CAP
#endif

    return "RadioURL:\n" OT_RADIO_URL_HELP_BUS OT_SPINEL_SPI_RADIO_URL_HELP_BUS
        OT_SPINEL_HDLC_RADIO_URL_HELP_BUS OT_SPINEL_VENDOR_RADIO_URL_HELP_BUS
           "Common Spinel Parameters:\n" OT_RADIO_URL_HELP_MAX_POWER_TABLE
           "    region[=region-code]          Set the radio's region code. The region code must be an\n"
           "                                  ISO 3166 alpha-2 code.\n"
           "    cca-threshold[=dbm]           Set the radio's CCA ED threshold in dBm measured at antenna connector.\n"
           "    tx-power[=dbm]                Set the radio's transmit power in dBm.\n" OT_RADIO_URL_HELP_TX_POWER_CAP
#if OPENTHREAD_CONFIG_PLATFORM_RADIO_COEX_ENABLE
           "    enable-coex[=1|0]             If not specified, RCP coex operates with its default configuration.\n"
           "                                  Disable coex with 0, and enable it with other values.\n"
#endif
           "    fem-lnagain[=dbm]             Set the Rx LNA gain in dBm of the external FEM.\n"
           "    no-reset                      Do not send Spinel reset command to RCP on initialization.\n"
           "    skip-rcp-compatibility-check  Skip checking RCP API version and capabilities during initialization.\n"
           "    bus-latency[=usec]            Communication latency in usec, default is 0.\n"
#if OPENTHREAD_CONFIG_MULTIPAN_RCP_ENABLE
           "    iid                           Set the Spinel Interface ID for this process. Valid values are 0-3.\n"
           "    iid-list                      List of IIDs a host can subscribe to in order to receive Spinel\n"
           "                                  frames other than the one provided in the 'iid' argument. If not\n"
           "                                  specified, the host will subscribe to the interface ID provided in\n"
           "                                  the 'iid' argument. Valid values are 0-3.\n"
           "                                  Up to three IIDs can be provided with each IID separated by ',',\n"
           "                                  e.g. iid-list=1,2,3.\n"
#endif
#if OPENTHREAD_POSIX_CONFIG_CONFIGURATION_FILE_ENABLE
           "    product-config-file[=path]    Specify a custom path to the openthread.conf product configuration\n"
           "                                  file.\n"
           "                                  If not specified, the default path set at build time is used.\n"
           "    factory-config-file[=path]    Specify a custom path to the openthread.conf factory configuration\n"
           "                                  file.\n"
           "                                  If not specified, the default path set at build time is used.\n"
#endif
        OT_TREL_RADIO_URL_HELP_BUS;
}

namespace ot {
namespace Posix {

void RadioUrl::Init(const char *aUrl)
{
    if (aUrl != nullptr)
    {
        size_t len = strlen(aUrl);

        VerifyOrDie(len + 1 < sizeof(mUrl), OT_EXIT_INVALID_ARGUMENTS);
        strcpy(mUrl, aUrl);
        SuccessOrDie(Url::Url::Init(mUrl));
    }
}

} // namespace Posix
} // namespace ot
