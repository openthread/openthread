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

/**
 * @file
 *   This file is a wrapper for the standard "string.h" file
 *   Some platforms provide all required functions, some do not.
 *   This solves the missing functions in #include <string.h>
 */

#if !defined(WRAP_STRING_H)
#define WRAP_STRING_H

#include "openthread-core-config.h"

/* system provided string.h */
#include <string.h>

/* These are C functions */
#if defined(__cplusplus)
#define WRAP_EXTERN_C extern "C"
#else
#define WRAP_EXTERN_C extern
#endif

/* Prototypes for our missing function replacements */

/* See: https://www.freebsd.org/cgi/man.cgi?query=strlcpy */
WRAP_EXTERN_C size_t missing_strlcpy(char *dst, const char *src, size_t dstsize);
/* See: https://www.freebsd.org/cgi/man.cgi?query=strlcat */
WRAP_EXTERN_C size_t missing_strlcat(char *dst, const char *src, size_t dstsize);
/* See: https://www.freebsd.org/cgi/man.cgi?query=strnlen */
WRAP_EXTERN_C size_t missing_strnlen(const char *s, size_t maxlen);

#undef WRAP_EXTERN_C

#if (!HAVE_STRNLEN)
#define strnlen(S, N) missing_strnlen(S, N)
#endif

#if (!HAVE_STRLCPY)
#define strlcpy(D, S, N) missing_strlcpy(D, S, N)
#endif

#if (!HAVE_STRLCAT)
#define strlcat(D, S, N) missing_strlcat(D, S, N)
#endif

#endif // WRAP_STRING_H
