/*
 *  Copyright (c) 2019, The OpenThread Authors.
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

#ifndef CORDIO_CONFIG_H
#define CORDIO_CONFIG_H

#include <openthread/config.h>
#include "openthread-core-config.h"
#include "ble_config.h"

#define INIT_BROADCASTER
#define INIT_OBSERVER
#define INIT_CENTRAL
#define INIT_PERIPHERAL

#define MBED_CONF_CORDIO_DESIRED_ATT_MTU BLE_STACK_ATT_MTU

#define MBED_CONF_CORDIO_MAX_PREPARED_WRITES 4

#define WSF_MS_PER_TICK OPENTHREAD_CONFIG_BLE_MS_PER_TICKS

#define LHCI_ENABLE_VS 0

#define BB_CLK_RATE_HZ OPENTHREAD_CONFIG_BLE_BB_CLK_RATE_HZ

#define LL_MAX_CONN BLE_STACK_MAX_BLE_CONNECTIONS

#endif /* CORDIO_CONFIG_H */
