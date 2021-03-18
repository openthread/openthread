/**
 * \file ecjpake_alt.h
 *
 * \brief Elliptic curve J-PAKE
 */
/*
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
 *
 *  This file is part of mbed TLS (https://tls.mbed.org)
 */
#ifndef MBEDTLS_ECJPAKE_ALT_H
#define MBEDTLS_ECJPAKE_ALT_H

#if defined(MBEDTLS_ECJPAKE_ALT)

#ifdef __cplusplus
extern "C" {
#endif

#ifndef MBEDTLS_ECJPAKE_ALT // QORVO
/**
 * Roles in the EC J-PAKE exchange
 */
typedef enum
{
    MBEDTLS_ECJPAKE_CLIENT = 0, /**< Client                         */
    MBEDTLS_ECJPAKE_SERVER,     /**< Server                         */
} mbedtls_ecjpake_role;
#endif // MBEDTLS_ECJPAKE_ALT // QORVO

#ifndef MBEDTLS_ECJPAKE_MAX_BYTES

/** The maximum size in bytes for a ECJPAKE number or coordinate.
 *
 * Default size works for P256 as MbedTLS and the Thread standard officially
 * only support P256. To support larger curves, chose  MBEDTLS_ECP_MAX_BYTES.
 */
#define MBEDTLS_ECJPAKE_MAX_BYTES (256 / 8)
#endif

/**
 * EC J-PAKE context structure.
 *
 * J-PAKE is a symmetric protocol, except for the identifiers used in
 * Zero-Knowledge Proofs, and the serialization of the second message
 * (KeyExchange) as defined by the Thread spec.
 *
 * In order to benefit from this symmetry, we choose a different naming
 * convetion from the Thread v1.0 spec. Correspondance is indicated in the
 * description as a pair C: client name, S: server name
 */
typedef struct
{
    mbedtls_ecp_group    grp;          /**< Elliptic curve                 */
    mbedtls_ecjpake_role role;         /**< Are we client or server?       */
    int                  point_format; /**< Format for point export        */

    mbedtls_ecp_point Xm1; /**< My public key 1   C: X1, S: X3 */
    mbedtls_ecp_point Xm2; /**< My public key 2   C: X2, S: X4 */
    mbedtls_ecp_point Xp1; /**< Peer public key 1 C: X3, S: X1 */
    mbedtls_ecp_point Xp2; /**< Peer public key 2 C: X4, S: X2 */
    mbedtls_ecp_point Xp;  /**< Peer public key   C: Xs, S: Xc */

    mbedtls_mpi xm1; /**< My private key 1  C: x1, S: x3 */
    mbedtls_mpi xm2; /**< My private key 2  C: x2, S: x4 */

    mbedtls_mpi s; /**< Pre-shared secret (passphrase) */

    const struct sx_ecc_curve_t *curve;   /**< Elliptic curve for HW offload  */
    int                          hashalg; /**< Hash algorithm for HW offload  */
} mbedtls_ecjpake_context;

#ifndef MBEDTLS_ECJPAKE_ALT // QORVO
/**
 * \brief           Initialize a context
 *                  (just makes it ready for setup() or free()).
 *
 * \param ctx       context to initialize
 */
void mbedtls_ecjpake_init(mbedtls_ecjpake_context *ctx);

/**
 * \brief           Set up a context for use
 *
 * \note            Currently the only values for hash/curve allowed by the
 *                  standard are MBEDTLS_MD_SHA256/MBEDTLS_ECP_DP_SECP256R1.
 *
 * \param ctx       context to set up
 * \param role      Our role: client or server
 * \param hash      hash function to use (MBEDTLS_MD_XXX)
 * \param curve     elliptic curve identifier (MBEDTLS_ECP_DP_XXX)
 * \param secret    pre-shared secret (passphrase)
 * \param len       length of the shared secret
 *
 * \return          0 if successfull,
 *                  a negative error code otherwise
 */
int mbedtls_ecjpake_setup(mbedtls_ecjpake_context *ctx,
                          mbedtls_ecjpake_role     role,
                          mbedtls_md_type_t        hash,
                          mbedtls_ecp_group_id     curve,
                          const unsigned char *    secret,
                          size_t                   len);

/**
 * \brief           Check if a context is ready for use
 *
 * \param ctx       Context to check
 *
 * \return          0 if the context is ready for use,
 *                  MBEDTLS_ERR_ECP_BAD_INPUT_DATA otherwise
 */
int mbedtls_ecjpake_check(const mbedtls_ecjpake_context *ctx);

/**
 * \brief           Generate and write the first round message
 *                  (TLS: contents of the Client/ServerHello extension,
 *                  excluding extension type and length bytes)
 *
 * \param ctx       Context to use
 * \param buf       Buffer to write the contents to
 * \param len       Buffer size
 * \param olen      Will be updated with the number of bytes written
 * \param f_rng     RNG function
 * \param p_rng     RNG parameter
 *
 * \return          0 if successfull,
 *                  a negative error code otherwise
 */
int mbedtls_ecjpake_write_round_one(mbedtls_ecjpake_context *ctx,
                                    unsigned char *          buf,
                                    size_t                   len,
                                    size_t *                 olen,
                                    int (*f_rng)(void *, unsigned char *, size_t),
                                    void *p_rng);

/**
 * \brief           Read and process the first round message
 *                  (TLS: contents of the Client/ServerHello extension,
 *                  excluding extension type and length bytes)
 *
 * \param ctx       Context to use
 * \param buf       Pointer to extension contents
 * \param len       Extension length
 *
 * \return          0 if successfull,
 *                  a negative error code otherwise
 */
int mbedtls_ecjpake_read_round_one(mbedtls_ecjpake_context *ctx, const unsigned char *buf, size_t len);

/**
 * \brief           Generate and write the second round message
 *                  (TLS: contents of the Client/ServerKeyExchange)
 *
 * \param ctx       Context to use
 * \param buf       Buffer to write the contents to
 * \param len       Buffer size
 * \param olen      Will be updated with the number of bytes written
 * \param f_rng     RNG function
 * \param p_rng     RNG parameter
 *
 * \return          0 if successfull,
 *                  a negative error code otherwise
 */
int mbedtls_ecjpake_write_round_two(mbedtls_ecjpake_context *ctx,
                                    unsigned char *          buf,
                                    size_t                   len,
                                    size_t *                 olen,
                                    int (*f_rng)(void *, unsigned char *, size_t),
                                    void *p_rng);

/**
 * \brief           Read and process the second round message
 *                  (TLS: contents of the Client/ServerKeyExchange)
 *
 * \param ctx       Context to use
 * \param buf       Pointer to the message
 * \param len       Message length
 *
 * \return          0 if successfull,
 *                  a negative error code otherwise
 */
int mbedtls_ecjpake_read_round_two(mbedtls_ecjpake_context *ctx, const unsigned char *buf, size_t len);

/**
 * \brief           Derive the shared secret
 *                  (TLS: Pre-Master Secret)
 *
 * \param ctx       Context to use
 * \param buf       Buffer to write the contents to
 * \param len       Buffer size
 * \param olen      Will be updated with the number of bytes written
 * \param f_rng     RNG function
 * \param p_rng     RNG parameter
 *
 * \return          0 if successfull,
 *                  a negative error code otherwise
 */
int mbedtls_ecjpake_derive_secret(mbedtls_ecjpake_context *ctx,
                                  unsigned char *          buf,
                                  size_t                   len,
                                  size_t *                 olen,
                                  int (*f_rng)(void *, unsigned char *, size_t),
                                  void *p_rng);

/**
 * \brief           Free a context's content
 *
 * \param ctx       context to free
 */
void mbedtls_ecjpake_free(mbedtls_ecjpake_context *ctx);
#endif // MBEDTLS_ECJPAKE_ALT // QORVO

#ifdef __cplusplus
}
#endif

#endif /* MBEDTLS_ECJPAKE_ALT */

#endif /* ecjpake.h */
