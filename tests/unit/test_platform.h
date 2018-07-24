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

#ifndef TEST_PLATFORM_H
#define TEST_PLATFORM_H

#if _WIN32
#define _CRT_SECURE_NO_WARNINGS
#include <time.h>
#include <windows.h>
#endif

#include <string.h>

#include <openthread/config.h>
#include <openthread/platform/alarm-milli.h>
#include <openthread/platform/logging.h>
#include <openthread/platform/misc.h>
#include <openthread/platform/radio.h>
#include <openthread/platform/random.h>

#include "common/code_utils.hpp"
#include "common/instance.hpp"

#include "test_util.h"

//
// Alarm Platform
//

typedef void (*testPlatAlarmStop)(otInstance *);
typedef void (*testPlatAlarmStartAt)(otInstance *, uint32_t, uint32_t);
typedef uint32_t (*testPlatAlarmGetNow)(void);

extern bool                 g_testPlatAlarmSet;
extern uint32_t             g_testPlatAlarmNext;
extern testPlatAlarmStop    g_testPlatAlarmStop;
extern testPlatAlarmStartAt g_testPlatAlarmStartAt;
extern testPlatAlarmGetNow  g_testPlatAlarmGetNow;

//
// Radio Platform
//

typedef void (*testPlatRadioSetPanId)(otInstance *, uint16_t);
typedef void (*testPlatRadioSetExtendedAddress)(otInstance *, const otExtAddress *);
typedef void (*testPlatRadioSetShortAddress)(otInstance *, uint16_t);

typedef bool (*testPlatRadioIsEnabled)(otInstance *);
typedef otError (*testPlatRadioEnable)(otInstance *);
typedef otError (*testPlatRadioDisable)(otInstance *);
typedef otError (*testPlatRadioReceive)(otInstance *, uint8_t);
typedef otError (*testPlatRadioTransmit)(otInstance *);
typedef otRadioFrame *(*testPlatRadioGetTransmitBuffer)(otInstance *);

extern otRadioCaps                     g_testPlatRadioCaps;
extern testPlatRadioSetPanId           g_testPlatRadioSetPanId;
extern testPlatRadioSetExtendedAddress g_testPlatRadioSetExtendedAddress;
extern testPlatRadioSetShortAddress    g_testPlatRadioSetShortAddress;
extern testPlatRadioIsEnabled          g_testPlatRadioIsEnabled;
extern testPlatRadioEnable             g_testPlatRadioEnable;
extern testPlatRadioDisable            g_testPlatRadioDisable;
extern testPlatRadioReceive            g_testPlatRadioReceive;
extern testPlatRadioTransmit           g_testPlatRadioTransmit;
extern testPlatRadioGetTransmitBuffer  g_testPlatRadioGetTransmitBuffer;

ot::Instance *testInitInstance(void);
void          testFreeInstance(otInstance *aInstance);

// Resets platform functions to defaults
void testPlatResetToDefaults(void);

#endif // TEST_PLATFORM_H
