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

/**
 * @file
 *   This file includes OpenThread device type checking utilities.
 */

#ifndef CONFIG_DEVICE_TYPE_CHECK_H_
#define CONFIG_DEVICE_TYPE_CHECK_H_

#if defined(OPENTHREAD_FTD) || defined(OPENTRHEAD_MTD) || defined(OPENTHREAD_RADIO)
#define _OPENTHREAD_DEVICE_TYPE_DEFINED 1
#else
#define _OPENTHREAD_DEVICE_TYPE_DEFINED 0
#endif

#if defined(OPENTHREAD_FTD) && OPENTHREAD_FTD
#define _OPENTHREAD_FTD_ 1
#else
#define _OPENTHREAD_FTD_ 0
#endif

#if defined(OPENTHREAD_MTD) && OPENTHREAD_MTD
#define _OPENTHREAD_MTD_ 1
#else
#define _OPENTHREAD_MTD_ 0
#endif

#if defined(OPENTHREAD_RADIO) && OPENTHREAD_RADIO
#define _OPENTHREAD_RADIO_ 1
#else
#define _OPENTHREAD_RADIO_ 0
#endif

#if _OPENTHREAD_DEVICE_TYPE_DEFINED
#if _OPENTHREAD_FTD_ + _OPENTHREAD_MTD_ + _OPENTHREAD_RADIO_ != 1
#error "Invalid definition for device type"
#else
#if _OPENTHREAD_FTD_
#define OPENTHREAD_MTD 0
#define OPENTHREAD_RADIO 0
#elif _OPENTHREAD_MTD_
#define OPENTHREAD_FTD 0
#define OPENTHREAD_RADIO 0
#elif _OPENTHREAD_RADIO_
#define OPENTHREAD_FTD 0
#define OPENTHREAD_MTD 0
#endif
#endif // _OPENTHREAD_FTD_ + _OPENTHREAD_MTD_ + _OPENTHREAD_RADIO_ != 1
#endif // _OPENTHREAD_DEVICE_TYPE_DEFINED

#endif // CONFIG_DEVICE_TYPE_CHECK_H_
