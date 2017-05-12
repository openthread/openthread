/*
 *    Copyright (c) 2017, The OpenThread Authors.
 *    All rights reserved.
 *
 *    Redistribution and use in source and binary forms, with or without
 *    modification, are permitted provided that the following conditions are met:
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *    3. Neither the name of the copyright holder nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 *    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 *    ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 *    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
 *    DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 *    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 *    ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 *    SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef SPIENL_PLATFROM_CONFIG_HEADER_INCLUDED
#define SPIENL_PLATFROM_CONFIG_HEADER_INCLUDED  1

#ifndef SPINEL_HEADER_INCLUDED
#error The "spinel_platform_config.h" MUST be included only from "spinel.h"
#else

/**
 * `spinel.h`, `spinel.c` are intended to be platform independent.
 *
 * This header file defines the platform related configuration settings used by spinel.
 *
 */


/**
 * Indicates whether the spinel files are being complied/built as part of OpenThread NCP library build.

 * Setting `SPINEL_PLATFORM_IS_OPENTHREAD` as 1 (or non-zero) ensures that the correct include files are used, e.g.,
 * the wraped header files such as "utils/wrap_string.h" instead of <string.h>, or the `<openthread-config.h>` will be
 * included in `spinel.c`.
 *
 */
#ifndef SPINEL_PLATFORM_IS_OPENTHREAD
#define SPINEL_PLATFORM_IS_OPENTHREAD                   1
#endif

/**
 * Indicates whether `errno` is not implemented.
 *
 */
#ifndef SPINEL_PLATFORM_DOESNT_IMPLEMENT_ERRNO_VAR
#define SPINEL_PLATFORM_DOESNT_IMPLEMENT_ERRNO_VAR      0
#endif

 /**
 * Indicates whether the `fprintf` is not implemented.
 *
 */
#ifndef SPINEL_PLATFORM_DOESNT_IMPLEMENT_FPRINTF
#define SPINEL_PLATFORM_DOESNT_IMPLEMENT_FPRINTF        0
#endif

/**
 * Indicates whether `strnlen` is not implemented.
 *
 */
#ifndef SPINEL_PLATFORM_DOESNT_IMPLEMENT_STRNLEN
#define SPINEL_PLATFORM_DOESNT_IMPLEMENT_STRNLEN        0
#endif

/**
 * Indicates whether spinel should log asserts.
 *
 */
#ifndef SPINEL_PLATFORM_SHOULD_LOG_ASSERTS
#define SPINEL_PLATFORM_SHOULD_LOG_ASSERTS              0
#endif

#endif /* #ifndef SPINEL_HEADER_INCLUDED */

#endif /* #ifndef SPIENL_PLATFROM_CONFIG_HEADER_INCLUDED */
