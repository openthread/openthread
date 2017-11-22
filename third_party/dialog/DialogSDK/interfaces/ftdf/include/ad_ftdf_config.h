/**
 \addtogroup INTERFACES
 \{
 \addtogroup FTDF
 \{
 */

/**
 ****************************************************************************************
 *
 * @file ad_ftdf_config.h
 *
 * @brief FTDF Adapter API
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
 ****************************************************************************************
 */

#ifndef AD_FTDF_CONFIG_H
#define AD_FTDF_CONFIG_H

#ifndef FTDF_PHY_API
#include "queue.h"
#endif

#include "ftdf.h"


/**
 * \brief Sleep When Idle
 *
 * If set, the block will sleep when Idle
 *
 */
#define AD_FTDF_SLEEP_WHEN_IDLE 1

/**
 * \brief Idle Timeout
 *
 * Idle Timeout, in OS scheduler ticks, after which, if still Idle, FTDF will
 * be put to sleep. Only applicable if AD_FTDF_SLEEP_WHEN_IDLE 1
 *
 */
#define AD_FTDF_IDLE_TIMEOUT 1

/**
 * \brief UP Queue size
 *
 * Defines the UP (UMAC to client app) Queue length, in number of messages
 *
 */
#define AD_FTDF_UP_QUEUE_LENGTH    16

/**
 * \brief DOWN Queue size
 *
 * Defines the DOWN (client app to UMAC) Queue length, in number of messages
 *
 */
#define AD_FTDF_DOWN_QUEUE_LENGTH  16

/**
 * \brief Low Power Clock cycle
 *
 * Defines the Low Power clock cycle in pico secs. Used for computation needed
 * for sleeping / waking up FTDF
 *
 */
#define AD_FTDF_LP_CLOCK_CYCLE 30517578

/**
 * \brief Wake Up Latency
 *
 * Defines the Wake Up Latency expressed in Low Power clock cycles, that is
 * the number of LP clock cycles needed for the FTDF to be fully operational
 * (calculations and FTDF timer synchronization)
 *
 */
#define AD_FTDF_WUP_LATENCY 10

/**
 * \brief Sleep value compensation
 *
 * Defines the sleep value compensation expressed in microseconds. This value
 * is subtracted from the sleep time returned by ftdf_can_sleep()
 */
#define AD_FTDF_SLEEP_COMPENSATION 1500

#endif /* AD_FTDF_CONFIG_H */
/**
 \}
 \}
 \}
 */
