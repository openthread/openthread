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
 
//
// system headers
//
extern "C" {
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <ntverp.h>
#include <ntddk.h>
#include <ntstrsafe.h>
#include <ntintsafe.h>
#include <ndis.h>
#include <WppRecorder.h>
#include <wdf.h>
#ifndef OTTMP_LEGACY
#include <netadaptercx.h>
#endif
#include <WdfMiniport.h>
#include <wdm.h>
#include <ntddser.h>
}

// Intellisense definition for DbgRaiseAssertionFailure because for some reason Visual Studio can't
// find it.
#ifdef __INTELLISENSE__
#define DbgRaiseAssertionFailure() ((void) 0)
#endif

#include "otOID.h"
 
#define CODE_SEG(seg) __declspec(code_seg(seg))
#define INITCODE CODE_SEG("INIT")  
#define PAGED  CODE_SEG("PAGE")

typedef struct _OTTMP_ADAPTER_CONTEXT *POTTMP_ADAPTER_CONTEXT;
typedef struct _OTTMP_DEVICE_CONTEXT *POTTMP_DEVICE_CONTEXT;

#include "hardware.hpp"
#include "hdlc.hpp"
#include "driver.hpp"
#include "adapter.hpp"
#include "device.hpp"
#include "serial.hpp"
#include "oid.hpp"
#include "openthread/platform/logging-windows.h"
