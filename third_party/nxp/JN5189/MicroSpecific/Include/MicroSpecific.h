/*
* Copyright (c) 2014 - 2015, Freescale Semiconductor, Inc.
* Copyright 2016-2019 NXP
* All rights reserved.
*
* SPDX-License-Identifier: BSD-3-Clause
*/

/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/

#define EXPAND1(x) x
#define EXPAND2(x, y)    EXPAND1(x)y
#define EXPAND3(x, y, z) EXPAND2(x, y)z

/* Convoluted way to #include <MicroSpecific_JN51xx.h> */
#undef INCLUDE_NAME
#define INCLUDE_NAME <EXPAND3(MicroSpecific,JENNIC_CHIP_FAMILY_NAME,.h)>
#include INCLUDE_NAME

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
