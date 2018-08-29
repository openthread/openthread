/**
 * Copyright (c) 2017 - 2018, Nordic Semiconductor ASA
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form, except as embedded into a Nordic
 *    Semiconductor ASA integrated circuit in a product or a software update for
 *    such product, must reproduce the above copyright notice, this list of
 *    conditions and the following disclaimer in the documentation and/or other
 *    materials provided with the distribution.
 *
 * 3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * 4. This software, with or without modification, must only be used with a
 *    Nordic Semiconductor ASA integrated circuit.
 *
 * 5. Any software provided in binary form under this license must not be reverse
 *    engineered, decompiled, modified and/or disassembled.
 *
 * THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef NRFX_IRQS_NRF51_H__
#define NRFX_IRQS_NRF51_H__

#ifdef __cplusplus
extern "C" {
#endif


// POWER_CLOCK_IRQn
#define nrfx_power_clock_irq_handler    POWER_CLOCK_IRQHandler

// RADIO_IRQn

// UART0_IRQn
#define nrfx_uart_0_irq_handler     UART0_IRQHandler

// SPI0_TWI0_IRQn
#if NRFX_CHECK(NRFX_PRS_ENABLED) && NRFX_CHECK(NRFX_PRS_BOX_0_ENABLED)
#define nrfx_prs_box_0_irq_handler  SPI0_TWI0_IRQHandler
#else
#define nrfx_spi_0_irq_handler      SPI0_TWI0_IRQHandler
#define nrfx_twi_0_irq_handler      SPI0_TWI0_IRQHandler
#endif

// SPI1_TWI1_IRQn
#if NRFX_CHECK(NRFX_PRS_ENABLED) && NRFX_CHECK(NRFX_PRS_BOX_1_ENABLED)
#define nrfx_prs_box_1_irq_handler  SPI1_TWI1_IRQHandler
#else
#define nrfx_spi_1_irq_handler      SPI1_TWI1_IRQHandler
#define nrfx_spis_1_irq_handler     SPI1_TWI1_IRQHandler
#define nrfx_twi_1_irq_handler      SPI1_TWI1_IRQHandler
#endif

// GPIOTE_IRQn
#define nrfx_gpiote_irq_handler     GPIOTE_IRQHandler

// ADC_IRQn
#define nrfx_adc_irq_handler        ADC_IRQHandler

// TIMER0_IRQn
#define nrfx_timer_0_irq_handler    TIMER0_IRQHandler

// TIMER1_IRQn
#define nrfx_timer_1_irq_handler    TIMER1_IRQHandler

// TIMER2_IRQn
#define nrfx_timer_2_irq_handler    TIMER2_IRQHandler

// RTC0_IRQn
#define nrfx_rtc_0_irq_handler      RTC0_IRQHandler

// TEMP_IRQn

// RNG_IRQn
#define nrfx_rng_irq_handler        RNG_IRQHandler

// ECB_IRQn

// CCM_AAR_IRQn

// WDT_IRQn
#define nrfx_wdt_irq_handler        WDT_IRQHandler

// RTC1_IRQn
#define nrfx_rtc_1_irq_handler      RTC1_IRQHandler

// QDEC_IRQn
#define nrfx_qdec_irq_handler       QDEC_IRQHandler

// LPCOMP_IRQn
#define nrfx_lpcomp_irq_handler     LPCOMP_IRQHandler

// SWI0_IRQn
#define nrfx_swi_0_irq_handler      SWI0_IRQHandler

// SWI1_IRQn
#define nrfx_swi_1_irq_handler      SWI1_IRQHandler

// SWI2_IRQn
#define nrfx_swi_2_irq_handler      SWI2_IRQHandler

// SWI3_IRQn
#define nrfx_swi_3_irq_handler      SWI3_IRQHandler

// SWI4_IRQn
#define nrfx_swi_4_irq_handler      SWI4_IRQHandler

// SWI5_IRQn
#define nrfx_swi_5_irq_handler      SWI5_IRQHandler


#ifdef __cplusplus
}
#endif

#endif // NRFX_IRQS_NRF51_H__
