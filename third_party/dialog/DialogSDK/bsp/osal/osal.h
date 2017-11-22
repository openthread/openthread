/**
 * \addtogroup BSP
 * \{
 * \addtogroup OSAL
 * 
 * \brief OS Abstraction Layer
 *
 * \{
 */

/**
 ****************************************************************************************
 *
 * @file osal.h
 *
 * @brief OS abstraction layer API
 *
 * @brief Access QSPI flash when running in auto mode
 *
 * Copyright (c) 2016, Dialog Semiconductor
 * All rights reserved.
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its contributors
 *    may be used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL,  SPECIAL,  EXEMPLARY,  OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *
 ****************************************************************************************
 */

#ifndef OSAL_H_
#define OSAL_H_

#if defined OS_BAREMETAL

#include <sdk_defs.h>

/*
 * Basic set of macros that can be used in non OS environment.
 */
# define PRIVILEGED_DATA
# define OS_MALLOC malloc
# define OS_FREE free
# ifndef RELEASE_BUILD
#  define OS_ASSERT(a) do { if (!(a)) {__BKPT(0);} } while (0)
# else
#  define OS_ASSERT(a) ((void) (a))
# endif

#endif

/**
 * \brief Cast any pointer to unsigned int value
 */
#define OS_PTR_TO_UINT(p) ((unsigned) (void *) (p))

/**
 * \brief Cast any pointer to signed int value
 */
#define OS_PTR_TO_INT(p) ((int) (void *) (p))

/**
 * \brief Cast any unsigned int value to pointer
 */
#define OS_UINT_TO_PTR(u) ((void *) (unsigned) (u))

/**
 * \brief Cast any signed int value to pointer
 */
#define OS_INT_TO_PTR(i) ((void *) (int) (i))

#endif /* OSAL_H_ */

/**
 * \}
 * \}
 * \}
 */
