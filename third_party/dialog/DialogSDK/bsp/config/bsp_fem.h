/**
 * \addtogroup BSP
 * \{
 * \addtogroup BSP_CONFIG
 * \{
 * \addtogroup RF_FEM
 *
 * \brief RF Front-End module confguration
 *
 *\{
 */

/**
 ***************************************************************************************
 *
 * @file bsp_fem.h
 *
 * @brief Board Support Package. RF Front-End Module definitions.
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
 ***************************************************************************************
 */

#ifndef BSP_FEM_H_
#define BSP_FEM_H_

/* ------------------------------- RF FEM driver (SKY66112-11) configuration settings ----------- */

#ifdef dg_configFEM_DLG_REF_BOARD

#define dg_configFEM FEM_SKY66112_11

#if dg_configPOWER_1V8P == 0
#warning Setting dg_configPOWER_1V8P to 1 for FEM to work
#undef dg_configPOWER_1V8P
#define dg_configPOWER_1V8P 1
#endif

#define dg_configFEM_SKY66112_11_FEM_BIAS_V18P
#define dg_configFEM_SKY66112_11_FEM_BIAS2_V18

#ifndef dg_configFEM_SKY66112_11_CSD_PORT
#define dg_configFEM_SKY66112_11_CSD_PORT HW_GPIO_PORT_4
#endif

#ifndef dg_configFEM_SKY66112_11_CSD_PIN
#define dg_configFEM_SKY66112_11_CSD_PIN HW_GPIO_PIN_3
#endif

#ifndef dg_configFEM_SKY66112_11_CPS_PORT
#define dg_configFEM_SKY66112_11_CPS_PORT HW_GPIO_PORT_4
#endif

#ifndef dg_configFEM_SKY66112_11_CPS_PIN
#define dg_configFEM_SKY66112_11_CPS_PIN HW_GPIO_PIN_6
#endif

#ifndef dg_configFEM_SKY66112_11_CRX_PORT
#define dg_configFEM_SKY66112_11_CRX_PORT HW_GPIO_PORT_4
#endif

#ifndef dg_configFEM_SKY66112_11_CRX_PIN
#define dg_configFEM_SKY66112_11_CRX_PIN HW_GPIO_PIN_2
#endif

#ifndef dg_configFEM_SKY66112_11_CTX_PORT
#define dg_configFEM_SKY66112_11_CTX_PORT HW_GPIO_PORT_4
#endif

#ifndef dg_configFEM_SKY66112_11_CTX_PIN
#define dg_configFEM_SKY66112_11_CTX_PIN HW_GPIO_PIN_5
#endif

#ifndef dg_configFEM_SKY66112_11_CHL_PORT
#define dg_configFEM_SKY66112_11_CHL_PORT HW_GPIO_PORT_4
#endif

#ifndef dg_configFEM_SKY66112_11_CHL_PIN
#define dg_configFEM_SKY66112_11_CHL_PIN HW_GPIO_PIN_4
#endif

#ifndef dg_configFEM_SKY66112_11_ANTSEL_PORT
#define dg_configFEM_SKY66112_11_ANTSEL_PORT HW_GPIO_PORT_4
#endif

#ifndef dg_configFEM_SKY66112_11_ANTSEL_PIN
#define dg_configFEM_SKY66112_11_ANTSEL_PIN HW_GPIO_PIN_0
#endif

#endif /* dg_configFEM_DLG_REF_BOARD */

#ifndef dg_configFEM
#define dg_configFEM FEM_NOFEM
#endif

#if dg_configFEM == FEM_SKY66112_11
#ifndef dg_configFEM_SKY66112_11_CSD_USE_DCF
#define dg_configFEM_SKY66112_11_CSD_USE_DCF 1
#endif

#ifndef dg_configFEM_SKY66112_11_TXSET_DCF
#define dg_configFEM_SKY66112_11_TXSET_DCF 47
#endif

#ifndef dg_configFEM_SKY66112_11_TXRESET_DCF
#define dg_configFEM_SKY66112_11_TXRESET_DCF 0
#endif

#ifndef dg_configFEM_SKY66112_11_RXSET_DCF
#define dg_configFEM_SKY66112_11_RXSET_DCF 1
#endif

#ifndef dg_configFEM_SKY66112_11_RXRESET_DCF
#define dg_configFEM_SKY66112_11_RXRESET_DCF 20
#endif
#endif /* dg_configFEM == FEM_SKY66112_11 */

#endif /* BSP_FEM_H_ */

/**
\}
\}
\}
*/

