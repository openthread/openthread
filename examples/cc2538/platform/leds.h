/*
 *  Copyright (c) 2016, Nest Labs, Inc.
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
 *   This file implements support for the Zolertia RE-Mote RGB LEDs
 *
 */

#ifndef LEDS_H
#define LEDS_H

#include "gpio.h"

#ifdef __cplusplus
 extern "C" {
#endif

#define LED0_PIN        5
#define LED1_PIN        4
#define LED2_PIN        3

#define LED0_ON         cc2538GpioSetPin(GPIO_D_NUM, LED0_PIN)
#define LED0_OFF        cc2538GpioClearPin(GPIO_D_NUM, LED0_PIN)

#define LED1_ON         cc2538GpioSetPin(GPIO_D_NUM, LED1_PIN)
#define LED1_OFF        cc2538GpioClearPin(GPIO_D_NUM, LED1_PIN)

#define LED2_ON         cc2538GpioSetPin(GPIO_D_NUM, LED2_PIN)
#define LED2_OFF        cc2538GpioClearPin(GPIO_D_NUM, LED2_PIN)

#define LED_ALL_OFF     LED0_OFF;   \
                        LED1_OFF;   \
                        LED2_OFF

// White
#define LED_ALL_ON      LED0_ON;    \
                        LED1_ON;    \
                        LED2_ON

// Yellow
#define LED3_ON         LED2_OFF;    \
                        LED0_ON;     \
                        LED1_ON
#define LED3_OFF        LED1_OFF;    \
                        LED0_OFF

// Purple
#define LED4_ON         LED1_OFF;     \
                        LED2_ON;      \
                        LED0_ON
#define LED4_OFF        LED2_OFF;     \
                        LED0_OFF

// Take LED_COLOR as argument, i.e LED0
#define LED_FADE(led)                         \
  volatile int i;                             \
  int k, j;                                   \
  LED_FADE_EXPAND(led)

#define LED_FADE_EXPAND(led)                  \
  for(k = 0; k < 800; ++k) {                  \
    j = k > 400 ? 800 - k : k;                \
    led##_ON;                                 \
    for(i = 0; i < j; ++i) {                  \
      asm("nop");                             \
    }                                         \
    led##_OFF;                                \
    for(i = 0; i < 400 - j; ++i) {            \
      asm("nop");                             \
    }                                         \
  }

#define LED_RAINBOW()                         \
  volatile int i;                             \
  int k,j;                                    \
  LED_FADE_EXPAND(LED3);                      \
  LED_FADE_EXPAND(LED0);                      \
  LED_FADE_EXPAND(LED4);                      \
  LED_FADE_EXPAND(LED2);                      \
  LED_FADE_EXPAND(LED1);

#ifdef __cplusplus
} // end extern "C"
#endif
#endif // LEDS_H_

