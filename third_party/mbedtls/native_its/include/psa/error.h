/*
 *  Copyright (c) 2025, The OpenThread Authors.
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

#ifndef PSA_ERROR_H__
#define PSA_ERROR_H__

#include <stdint.h>

typedef int32_t psa_status_t;

#define PSA_SUCCESS                    ((psa_status_t)0)
#define PSA_ERROR_GENERIC_ERROR        ((psa_status_t)-132)
#define PSA_ERROR_NOT_PERMITTED        ((psa_status_t)-133)
#define PSA_ERROR_NOT_SUPPORTED        ((psa_status_t)-134)
#define PSA_ERROR_INVALID_ARGUMENT     ((psa_status_t)-135)
#define PSA_ERROR_ALREADY_EXISTS       ((psa_status_t)-139)
#define PSA_ERROR_DOES_NOT_EXIST       ((psa_status_t)-140)
#define PSA_ERROR_INSUFFICIENT_STORAGE ((psa_status_t)-142)
#define PSA_ERROR_STORAGE_FAILURE      ((psa_status_t)-146)
#define PSA_ERROR_INVALID_SIGNATURE    ((psa_status_t)-149)
#define PSA_ERROR_DATA_CORRUPT         ((psa_status_t)-152)

#endif /* PSA_ERROR_H__ */
