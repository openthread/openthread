/*
 *  Copyright (c) 2017, The OpenThread Authors.
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
 *   This file implements the OpenThread platform abstraction for random number generator.
 *
 */

#include <openthread/platform/random.h>

#include "utils/code_utils.h"

#include "em_adc.h"
#include "em_cmu.h"

enum
{
    EFR32_ADC_REF_CLOCK = 7000000,
};

void efr32RandomInit(void)
{
    /* Enable ADC Clock */
    CMU_ClockEnable(cmuClock_ADC0, true);
    ADC_Init_TypeDef       init       = ADC_INIT_DEFAULT;
    ADC_InitSingle_TypeDef singleInit = ADC_INITSINGLE_DEFAULT;

    /* Initialize the ADC with the required values */
    init.timebase = ADC_TimebaseCalc(0);
    init.prescale = ADC_PrescaleCalc(EFR32_ADC_REF_CLOCK, 0);
    ADC_Init(ADC0, &init);

    /* Initialize for single conversion specific to RNG */
    singleInit.reference = adcRefVEntropy;
    singleInit.diff      = true;
    singleInit.posSel    = adcPosSelVSS;
    singleInit.negSel    = adcNegSelVSS;
    ADC_InitSingle(ADC0, &singleInit);

    /* Set VINATT to maximum value and clear FIFO */
    ADC0->SINGLECTRLX |= _ADC_SINGLECTRLX_VINATT_MASK;
    ADC0->SINGLEFIFOCLEAR = ADC_SINGLEFIFOCLEAR_SINGLEFIFOCLEAR;
}

uint32_t otPlatRandomGet(void)
{
    uint8_t  tmp    = 0;
    uint32_t random = 0;

    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 3; j++)
        {
            ADC_Start(ADC0, adcStartSingle);

            while ((ADC0->IF & ADC_IF_SINGLE) == 0)
                ;

            tmp |= ((ADC_DataSingleGet(ADC0) & 0x07) << (j * 3));
        }

        random |= (tmp & 0xff) << (i * 8);
    }

    return random;
}

otError otPlatRandomGetTrue(uint8_t *aOutput, uint16_t aOutputLength)
{
    otError error = OT_ERROR_NONE;

    otEXPECT_ACTION(aOutput, error = OT_ERROR_INVALID_ARGS);

    for (uint16_t length = 0; length < aOutputLength; length++)
    {
        aOutput[length] = (uint8_t)otPlatRandomGet();
    }

exit:
    return error;
}
