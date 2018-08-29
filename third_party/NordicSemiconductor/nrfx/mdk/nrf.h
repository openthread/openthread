/*

Copyright (c) 2010 - 2018, Nordic Semiconductor ASA

All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form, except as embedded into a Nordic
   Semiconductor ASA integrated circuit in a product or a software update for
   such product, must reproduce the above copyright notice, this list of
   conditions and the following disclaimer in the documentation and/or other
   materials provided with the distribution.

3. Neither the name of Nordic Semiconductor ASA nor the names of its
   contributors may be used to endorse or promote products derived from this
   software without specific prior written permission.

4. This software, with or without modification, must only be used with a
   Nordic Semiconductor ASA integrated circuit.

5. Any software provided in binary form under this license must not be reverse
   engineered, decompiled, modified and/or disassembled.

THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND ANY EXPRESS
OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#ifndef NRF_H
#define NRF_H

/* MDK version */
#define MDK_MAJOR_VERSION   8
#define MDK_MINOR_VERSION   17
#define MDK_MICRO_VERSION   0

/* Redefine "old" too-generic name NRF52 to NRF52832_XXAA to keep backwards compatibility. */
#if defined (NRF52)
    #ifndef NRF52832_XXAA
        #define NRF52832_XXAA
    #endif
#endif

/* Define NRF52_SERIES for common use in nRF52 series devices. Only if not previously defined. */
#if defined (NRF52810_XXAA) || defined (NRF52832_XXAA) || defined (NRF52832_XXAB) || defined (NRF52840_XXAA)
    #ifndef NRF52_SERIES
        #define NRF52_SERIES
    #endif
#endif


#if defined(_WIN32)
    /* Do not include nrf specific files when building for PC host */
#elif defined(__unix)
    /* Do not include nrf specific files when building for PC host */
#elif defined(__APPLE__)
    /* Do not include nrf specific files when building for PC host */
#else

    /* Device selection for device includes. */
    #if defined (NRF51)
        #include "nrf51.h"
        #include "nrf51_bitfields.h"
        #include "nrf51_deprecated.h"
    
    #elif defined (NRF52810_XXAA)
        #include "nrf52810.h"
        #include "nrf52810_bitfields.h"
        #include "nrf51_to_nrf52810.h"
        #include "nrf52_to_nrf52810.h"
    #elif defined (NRF52832_XXAA) || defined (NRF52832_XXAB)
        #include "nrf52.h"
        #include "nrf52_bitfields.h"
        #include "nrf51_to_nrf52.h"
        #include "nrf52_name_change.h"
    #elif defined (NRF52840_XXAA)
        #include "nrf52840.h"
        #include "nrf52840_bitfields.h"
        #include "nrf51_to_nrf52840.h"
        #include "nrf52_to_nrf52840.h"
    
    #else
        #error "Device must be defined. See nrf.h."
    #endif /* NRF51, NRF52810_XXAA, NRF52832_XXAA, NRF52832_XXAB, NRF52840_XXAA */

    #include "compiler_abstraction.h"

#endif /* _WIN32 || __unix || __APPLE__ */

#endif /* NRF_H */

