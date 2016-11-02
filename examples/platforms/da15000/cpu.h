/**
 * CPU abstraction -- maps to raw register file.
 *
 * @file     cpu.h
 * @author   Martin Turon
 * @date     March 24, 2014
 *
 * Copyright (c) 2014 Nest Labs, Inc.  All rights reserved.
 *
 * $Id$
 */

#ifndef _CPU_H_
#define _CPU_H_

#include <tool.h>
#include <errno.h>

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// =========== CPU SELECTION : START ===========

#ifdef CPU_POSIX

#include <pthread.h>
#include <sys/signal.h>
#include <sys/time.h>

#elif CPU_DA15100
#include "bsp/include/black_orca.h"
#include "bsp/include/core_cm0.h"

#else

#error "Error: No valid CPU specified"

#endif

// =========== CPU SELECTION : END ===========

#ifdef __cplusplus
}
#endif

#endif // _CPU_H_
