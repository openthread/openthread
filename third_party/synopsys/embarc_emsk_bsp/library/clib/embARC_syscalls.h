/* ------------------------------------------
 * Copyright (c) 2017, Synopsys, Inc. All rights reserved.

 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:

 * 1) Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.

 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation and/or
 * other materials provided with the distribution.

 * 3) Neither the name of the Synopsys, Inc., nor the names of its contributors may
 * be used to endorse or promote products derived from this software without
 * specific prior written permission.

 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * \version 2017.03
 * \date 2016-01-18
 * \author Huaqi Fang(Huaqi.Fang@synopsys.com)
--------------------------------------------- */

/**
 * \file
 * \ingroup EMBARC_SYSCALL
 * \brief Syscall support header file
 */
#ifndef _EMBARC_SYSCALLS_
#define _EMBARC_SYSCALLS_

#include "embARC_BSP_config.h"
#include "board/board.h"

#ifdef __cplusplus
extern "C" {
#endif

#if defined(__GNU__) && !defined(_HAVE_LIBGLOSS_)
/* _HAVE_LIBGLOSS_ is defined in options/toolchain/toolchain_gnu.mk */
#include <sys/syscall.h>
#define SYSCALL_PREFIX(x)       syscall##x

#endif /* __GNU__ && !_HAVE_LIBGLOSS_ */

#ifndef SYSCALL_PREFIX
#define SYSCALL_PREFIX(x)       x
#endif

#if defined(__GNU__) && !defined(_HAVE_LIBGLOSS_) && !defined(_HOSTLINK_)
extern void syscall_swi(void *ptr);
#endif

#include "embARC_target.h"

#ifdef __cplusplus
}
#endif

#endif  /* _EMBARC_SYSCALLS_ */
