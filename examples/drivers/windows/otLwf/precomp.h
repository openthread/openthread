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
 * @brief
 *  Precompiled header for otLwf project.
 */

#pragma warning(disable:4201)  // nonstandard extension used : nameless struct/union
#pragma warning(disable:4204)  // nonstandard extension used : non-constant aggregate initializer
#pragma warning(disable:28175) // The 'MajorFunction' member of _DRIVER_OBJECT should not be accessed by a driver:
                               // Access to this member may be permitted for certain classes of drivers.
#pragma warning(disable:28301) // No annotations for first declaration of *

#include <ntifs.h>
#include <ndis.h>
#include <wdmsec.h>
#include <rtlrefcount.h>
#include <netiodef.h>
#include <nsihelper.h>
#include <netioapi.h>
#include <bcrypt.h>

VOID  
RtlCopyBufferToMdl(  
    _In_reads_bytes_(BytesToCopy) CONST VOID *Buffer,  
    _Inout_ PMDL MdlChain,  
    _In_ SIZE_T MdlOffset,  
    _In_ SIZE_T BytesToCopy,  
    _Out_ SIZE_T* BytesCopied  
    );

#include <stdio.h>
#include <stdarg.h>

#include <openthread-windows-config.h>
#include <openthread-core-config.h>
#include <openthread/border_router.h>
#include <openthread/dataset_ftd.h>
#include <openthread/thread_ftd.h>
#include <openthread/icmp6.h>
#include <openthread/ip6.h>
#include <openthread/tasklet.h>
#include <openthread/commissioner.h>
#include <openthread/joiner.h>
#include <common/code_utils.hpp>
#include <openthread/platform/logging.h>
#include <openthread/platform/logging-windows.h>
#include <openthread/platform/radio.h>
#include <openthread/platform/misc.h>
#include <openthread/platform/alarm-milli.h>
#include <openthread/platform/settings.h>
#include <openthread/platform/messagepool.h>
#include <ncp/spinel.h>

#include <otLwfIoctl.h>

#ifdef _KERNEL_MODE
#define CODE_SEG(segment) __declspec(code_seg(segment))
#else
#define CODE_SEG(segment) 
#endif

#define PAGED CODE_SEG("PAGE") _IRQL_always_function_max_(PASSIVE_LEVEL)
#define PAGEDX CODE_SEG("PAGE")
#define INITCODE CODE_SEG("INIT")

typedef struct _MS_FILTER MS_FILTER, *PMS_FILTER;

#pragma pack(push)
#pragma pack(1)

typedef struct UDPHeader
{
    USHORT SourcePort;
    USHORT DestinationPort;
    USHORT TotalLength;
    USHORT Checksum;

} UDPHeader;

#pragma pack(pop)

//#define DEBUG_TIMING
//#define DEBUG_ALLOC
#define LOG_BUFFERS
//#define FORCE_SYNCHRONOUS_RECEIVE
#define COMMAND_INIT_RETRY

#include "driver.h"
#include "device.h"
#include "iocontrol.h"
#include "radio.h"
#include "filter.h"
#include "command.h"
#include "thread.h"
#include "tunnel.h"
