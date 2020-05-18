/*
 * Copyright 2017 NXP
 * All rights reserved.
 *
 * 
 * SPDX-License-Identifier: BSD-3-Clause
 */
 
#ifndef KSDK_MBEDTLS_H
#define KSDK_MBEDTLS_H

#ifdef __cplusplus
extern "C" {
#endif

int fsl_mbedtls_printf(const char *fmt_s, ...);
void CRYPTO_InitHardware(void);

#ifdef __cplusplus
}
#endif

#endif /* KSDK_MBEDTLS_H */
