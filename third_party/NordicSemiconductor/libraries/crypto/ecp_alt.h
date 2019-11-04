/*
 *  Copyright (c) 2018, The OpenThread Authors.
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
 *
 *  Copyright (C) 2006-2015, ARM Limited, All Rights Reserved
 *  SPDX-License-Identifier: Apache-2.0
 *
 *  Licensed under the Apache License, Version 2.0 (the "License"); you may
 *  not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 *  WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#if defined(MBEDTLS_ECP_ALT)

/*
 * default mbed TLS elliptic curve arithmetic implementation
 *
 * (in case MBEDTLS_ECP_ALT is defined then the developer has to provide an
 * alternative implementation for the whole module and it will replace this
 * one.)
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \brief           ECP group structure
 *
 * We consider two types of curves equations:
 * 1. Short Weierstrass y^2 = x^3 + A x + B     mod P   (SEC1 + RFC 4492)
 * 2. Montgomery,       y^2 = x^3 + A x^2 + x   mod P   (Curve25519 + draft)
 * In both cases, a generator G for a prime-order subgroup is fixed. In the
 * short weierstrass, this subgroup is actually the whole curve, and its
 * cardinal is denoted by N.
 *
 * In the case of Short Weierstrass curves, our code requires that N is an odd
 * prime. (Use odd in mbedtls_ecp_mul() and prime in mbedtls_ecdsa_sign() for blinding.)
 *
 * In the case of Montgomery curves, we don't store A but (A + 2) / 4 which is
 * the quantity actually used in the formulas. Also, nbits is not the size of N
 * but the required size for private keys.
 *
 * If modp is NULL, reduction modulo P is done using a generic algorithm.
 * Otherwise, it must point to a function that takes an mbedtls_mpi in the range
 * 0..2^(2*pbits)-1 and transforms it in-place in an integer of little more
 * than pbits, so that the integer may be efficiently brought in the 0..P-1
 * range by a few additions or substractions. It must return 0 on success and
 * non-zero on failure.
 */
typedef struct
{
    mbedtls_ecp_group_id id;    /*!<  internal group identifier                     */
    mbedtls_mpi P;              /*!<  prime modulus of the base field               */
    mbedtls_mpi A;              /*!<  1. A in the equation, or 2. (A + 2) / 4       */
    mbedtls_mpi B;              /*!<  1. B in the equation, or 2. unused            */
    mbedtls_ecp_point G;        /*!<  generator of the (sub)group used              */
    mbedtls_mpi N;              /*!<  1. the order of G, or 2. unused               */
    size_t pbits;       /*!<  number of bits in P                           */
    size_t nbits;       /*!<  number of bits in 1. P, or 2. private keys    */
    unsigned int h;     /*!<  internal: 1 if the constants are static       */
    int (*modp)(mbedtls_mpi *); /*!<  function for fast reduction mod P             */
    int (*t_pre)(mbedtls_ecp_point *, void *);  /*!< unused                         */
    int (*t_post)(mbedtls_ecp_point *, void *); /*!< unused                         */
    void *t_data;                       /*!< unused                         */
    mbedtls_ecp_point *T;       /*!<  pre-computed points for ecp_mul_comb()        */
    size_t T_size;      /*!<  number for pre-computed points                */
}
mbedtls_ecp_group;

/**
 * \name SECTION: Module settings
 *
 * The configuration options you can set for this module are in this section.
 * Either change them in config.h or define them on the compiler command line.
 * \{
 */

#if !defined(MBEDTLS_ECP_MAX_BITS)
/**
 * Maximum size of the groups (that is, of N and P)
 */
#define MBEDTLS_ECP_MAX_BITS     521   /**< Maximum bit size of groups */
#endif

#define MBEDTLS_ECP_MAX_BYTES    ( ( MBEDTLS_ECP_MAX_BITS + 7 ) / 8 )
#define MBEDTLS_ECP_MAX_PT_LEN   ( 2 * MBEDTLS_ECP_MAX_BYTES + 1 )

#if !defined(MBEDTLS_ECP_WINDOW_SIZE)
/*
 * Maximum "window" size used for point multiplication.
 * Default: 6.
 * Minimum value: 2. Maximum value: 7.
 *
 * Result is an array of at most ( 1 << ( MBEDTLS_ECP_WINDOW_SIZE - 1 ) )
 * points used for point multiplication. This value is directly tied to EC
 * peak memory usage, so decreasing it by one should roughly cut memory usage
 * by two (if large curves are in use).
 *
 * Reduction in size may reduce speed, but larger curves are impacted first.
 * Sample performances (in ECDHE handshakes/s, with FIXED_POINT_OPTIM = 1):
 *      w-size:     6       5       4       3       2
 *      521       145     141     135     120      97
 *      384       214     209     198     177     146
 *      256       320     320     303     262     226

 *      224       475     475     453     398     342
 *      192       640     640     633     587     476
 */
#define MBEDTLS_ECP_WINDOW_SIZE    6   /**< Maximum window size used */
#endif /* MBEDTLS_ECP_WINDOW_SIZE */

#if !defined(MBEDTLS_ECP_FIXED_POINT_OPTIM)
/*
 * Trade memory for speed on fixed-point multiplication.
 *
 * This speeds up repeated multiplication of the generator (that is, the
 * multiplication in ECDSA signatures, and half of the multiplications in
 * ECDSA verification and ECDHE) by a factor roughly 3 to 4.
 *
 * The cost is increasing EC peak memory usage by a factor roughly 2.
 *
 * Change this value to 0 to reduce peak memory usage.
 */
#define MBEDTLS_ECP_FIXED_POINT_OPTIM  1   /**< Enable fixed-point speed-up */
#endif /* MBEDTLS_ECP_FIXED_POINT_OPTIM */

/* \} name SECTION: Module settings */

#ifdef __cplusplus
}
#endif

#endif // MBEDTLS_ECP_ALT
