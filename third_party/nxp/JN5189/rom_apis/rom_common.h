/*
 * Copyright 2018 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef ROM_COMMON_H_
#define ROM_COMMON_H_

#if defined __cplusplus
extern "C" {
#endif

/****************************************************************************/
/***        Include Files                                                 ***/
/****************************************************************************/

#include <stdbool.h>

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

/* Define ROM compilation for ES2 - required for lowpower API as it supports ES1/ES2 compilation */
//#define CPU_JN518X_REV 2

#ifdef ROM_BUILD
#define ROM_API __attribute__((section(".text.api")))
#else
#ifdef __MINGW32__
#define ROM_API
#elif(defined(__CC_ARM) || defined(__ARMCC_VERSION)) || (defined(__ICCARM__))
#define ROM_API
#else
#define ROM_API __attribute__((long_call))
#endif
#endif

#ifndef WEAK
#define WEAK __attribute__((weak))
#endif

#ifdef __CDT_PARSER__
#define STATIC_ASSERT(value, message)
#else
#define STATIC_ASSERT _Static_assert
#endif

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

#if defined __cplusplus
}
#endif

#endif /* ROM_COMMON_H_ */

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
