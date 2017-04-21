/*
 *    Copyright (c) 2016, The OpenThread Authors.
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

/* system provided string.h */
#include <string.h>


/* These are C functions */
#if defined(__cplusplus)
#define WRAP_EXTERN_C  extern "C"
#else
#define WRAP_EXTERN_C  extern
#endif

/* Prototypes for our missing function replacements */

/* See: https://www.freebsd.org/cgi/man.cgi?query=strlcpy */
WRAP_EXTERN_C size_t missing_strlcpy(char   *dst, const char *src, size_t dstsize);
/* See: https://www.freebsd.org/cgi/man.cgi?query=strlcat */
WRAP_EXTERN_C size_t missing_strlcat(char   *dst, const char *src, size_t dstsize);
/* See: https://www.freebsd.org/cgi/man.cgi?query=strnlen */
WRAP_EXTERN_C size_t missing_strnlen(const char *s, size_t maxlen);

#undef WRAP_EXTERN_C

/* undefine any compiler supplied macro for these functions */
#undef strlcat
#undef strlcpy
#undef strnlen

/* Goal: By default it just works...
 * so define our replacements */
#define strlcat missing_strlcat
#define strlcpy missing_strlcpy
#define strnlen missing_strnlen
/* Note: Add more here as needed */

/*
 * Given the above "just works"
 * The next step is to "undef" things that do exist.
 * Thus, 'undef' is a platform specific optimization.
 */
#if defined(_MSV_VER)
#undef strnlen /* provided by visual studio */
/* Others are not provided by visual studio */
#endif

#if defined(__TI_ARM__)
/* TI_ARM compiler is missing all */
#endif

#if __linux__ || __APPLE__
/* present on Linux & MAC  */
#undef strnlen
/* strlcat is missing */
/* strlcpy is missing */
#endif

#if defined(__IAR_SYSTEMS_ICC__)
#undef strnlen /* provided by IAR work bench */
/* Others are not provided by IAR */
#endif

#endif  // WRAP_STRING_H
