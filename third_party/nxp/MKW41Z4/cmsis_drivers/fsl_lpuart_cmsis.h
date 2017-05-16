/*
 * Copyright (c) 2013-2016 ARM Limited. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.

 * Copyright (c) 2016, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this list
 *   of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * o Neither the name of the copyright holder nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
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
 */

#ifndef _FSL_LPUART_CMSIS_H_
#define _FSL_LPUART_CMSIS_H_

#include "fsl_common.h"
#include "Driver_USART.h"
#include "RTE_Device.h"
#include "fsl_lpuart.h"
#if (defined(FSL_FEATURE_SOC_DMAMUX_COUNT) && FSL_FEATURE_SOC_DMAMUX_COUNT)
#include "fsl_dmamux.h"
#endif
#if (defined(FSL_FEATURE_SOC_DMA_COUNT) && FSL_FEATURE_SOC_DMA_COUNT)
#include "fsl_lpuart_dma.h"
#endif
#if (defined(FSL_FEATURE_SOC_EDMA_COUNT) && FSL_FEATURE_SOC_EDMA_COUNT)
#include "fsl_lpuart_edma.h"
#endif

#if defined(LPUART0)
extern ARM_DRIVER_USART Driver_USART0;
#endif /* LPUART0 */

#if defined(LPUART1)
extern ARM_DRIVER_USART Driver_USART1;
#endif /* LPUART1 */

#if defined(LPUART2)
extern ARM_DRIVER_USART Driver_USART2;
#endif /* LPUART2 */

#if defined(LPUART3)
extern ARM_DRIVER_USART Driver_USART3;
#endif /* LPUART3 */

#if defined(LPUART4)
extern ARM_DRIVER_USART Driver_USART4;
#endif /* LPUART4 */

#if defined(LPUART5)
extern ARM_DRIVER_USART Driver_USART5;
#endif /* LPUART5 */

#if (FSL_FEATURE_SOC_LPUART_COUNT == 1) && (FSL_FEATURE_SOC_UART_COUNT == 3)

extern ARM_DRIVER_USART Driver_USART3;

#endif

#if (FSL_FEATURE_SOC_LPUART_COUNT == 1) && (FSL_FEATURE_SOC_UART_COUNT == 4)

extern ARM_DRIVER_USART Driver_USART4;

#endif

#if (FSL_FEATURE_SOC_LPUART_COUNT == 1) && (FSL_FEATURE_SOC_UART_COUNT == 5)
extern ARM_DRIVER_USART Driver_USART5;

#endif

#endif /* _FSL_LPUART_CMSIS_H_ */
