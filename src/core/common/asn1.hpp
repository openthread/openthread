/*
 *  Copyright (c) 2019, The OpenThread Authors.
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
 *   This file includes definitions for ASN.1 computations.
 */

#ifndef ASN1_HPP_
#define ASN1_HPP_

#include "openthread-core-config.h"

#if OPENTHREAD_ENABLE_EST_CLIENT

#include <stdint.h>

#include <mbedtls/asn1.h>
#include <mbedtls/asn1write.h>

#define otAsn1Buffer    mbedtls_asn1_buf
#define otAsn1Bitstring mbedtls_asn1_bitstring
#define otAsn1Sequence  mbedtls_asn1_sequence
#define otAsn1NamedData mbedtls_asn1_named_data

#define otAsn1GetLength         mbedtls_asn1_get_len
#define otAsn1GetTag            mbedtls_asn1_get_tag
#define otAsn1GetBool           mbedtls_asn1_get_bool
#define otAsn1GetInteger        mbedtls_asn1_get_int
#define otAsn1GetBitstring      mbedtls_asn1_get_bitstring
#define otAsn1GetBitstringNull  mbedtls_asn1_get_bitstring_null
#define otAsn1GetSequenceOf     mbedtls_asn1_get_sequence_of
#define otAsn1GetMpi            mbedtls_asn1_get_mpi
#define otAsn1GetAlgorithm      mbedtls_asn1_get_alg
#define otAsn1GetAlgorithmNull  mbedtls_asn1_get_alg_null
#define otAsn1FindNamedData     mbedtls_asn1_find_named_data
#define otAsn1FreeNamedData     mbedtls_asn1_free_named_data
#define otAsn1FreeNamedDataList mbedtls_asn1_free_named_data_list

namespace ot {
namespace Asn1 {

void Init(void);
void Deinit(void);

} // namespace Asn1
} // namespace ot

#endif // OPENTHREAD_ENABLE_EST_CLIENT
#endif // ASN1_HPP_
