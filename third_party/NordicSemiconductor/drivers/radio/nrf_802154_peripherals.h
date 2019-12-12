/* Copyright (c) 2019, Nordic Semiconductor ASA
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   1. Redistributions of source code must retain the above copyright notice, this
 *      list of conditions and the following disclaimer.
 *
 *   2. Redistributions in binary form must reproduce the above copyright notice,
 *      this list of conditions and the following disclaimer in the documentation
 *      and/or other materials provided with the distribution.
 *
 *   3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *      contributors may be used to endorse or promote products derived from
 *      this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

/**
 * @brief Module that defines the 802.15.4 driver peripheral usage.
 *
 */

#ifndef NRF_802154_PERIPHERALS_H__
#define NRF_802154_PERIPHERALS_H__

#include <nrf.h>
#include <nrfx.h>
#include "nrf_802154_config.h"
#include "nrf_802154_debug.h"
#include "nrf_802154_debug_core.h"
#include "fem/nrf_fem_protocol_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @def NRF_802154_HIGH_PRECISION_TIMER_INSTANCE_NO
 *
 * Number of the timer instance used for precise frame timestamps and synchronous radio operations.
 *
 */
#ifndef NRF_802154_HIGH_PRECISION_TIMER_INSTANCE_NO
#define NRF_802154_HIGH_PRECISION_TIMER_INSTANCE_NO 0
#endif

/**
 * @def NRF_802154_HIGH_PRECISION_TIMER_INSTANCE
 *
 * The timer instance used for precise frame timestamps and synchronous radio operations.
 *
 */
#define NRF_802154_HIGH_PRECISION_TIMER_INSTANCE \
    NRFX_CONCAT_2(NRF_TIMER, NRF_802154_HIGH_PRECISION_TIMER_INSTANCE_NO)

/**
 * @def NRF_802154_TIMER_INSTANCE_NO
 *
 * Number of the timer instance used both by the driver for ACK IFS and by the FEM module.
 *
 */
#ifndef NRF_802154_TIMER_INSTANCE_NO
#define NRF_802154_TIMER_INSTANCE_NO 1
#endif

/**
 * @def NRF_802154_TIMER_INSTANCE
 *
 * The timer instance used both by the driver for ACK IFS and by the FEM module.
 *
 */
#define NRF_802154_TIMER_INSTANCE \
    NRFX_CONCAT_2(NRF_TIMER, NRF_802154_TIMER_INSTANCE_NO)

/**
 * @def NRF_802154_COUNTER_TIMER_INSTANCE_NO
 *
 * Number of the timer instance used for detecting when PSDU is being received.
 *
 */
#ifndef NRF_802154_COUNTER_TIMER_INSTANCE_NO
#define NRF_802154_COUNTER_TIMER_INSTANCE_NO 2
#endif

/**
 * @def NRF_802154_COUNTER_TIMER_INSTANCE
 *
 * The timer instance used by the driver for detecting when PSDU is being received.
 *
 * @note This configuration is used only when the NRF_RADIO_EVENT_BCMATCH event handling is disabled
 *       (see @ref NRF_802154_DISABLE_BCC_MATCHING).
 */
#define NRF_802154_COUNTER_TIMER_INSTANCE \
    NRFX_CONCAT_2(NRF_TIMER, NRF_802154_COUNTER_TIMER_INSTANCE_NO)

/**
 * @def NRF_802154_SWI_EGU_INSTANCE_NO
 *
 * Number of the SWI EGU instance used by the driver to synchronize PPIs and for requests and
 * notifications if SWI is in use.
 *
 */
#ifndef NRF_802154_SWI_EGU_INSTANCE_NO

#ifdef NRF52811_XXAA
#define NRF_802154_SWI_EGU_INSTANCE_NO 0
#else
#define NRF_802154_SWI_EGU_INSTANCE_NO 3
#endif

#endif // NRF_802154_SWI_EGU_INSTANCE_NO

/**
 * @def NRF_802154_SWI_EGU_INSTANCE
 *
 * The SWI EGU instance used by the driver to synchronize PPIs and for requests and notifications if
 * SWI is in use.
 *
 * @note This option is used by the core module regardless of the driver configuration.
 *
 */
#define NRF_802154_SWI_EGU_INSTANCE NRFX_CONCAT_2(NRF_EGU, NRF_802154_SWI_EGU_INSTANCE_NO)

/**
 * @def NRF_802154_SWI_IRQ_HANDLER
 *
 * The SWI EGU IRQ handler used by the driver for requests and notifications if SWI is in use.
 *
 * @note This option is used when the driver uses SWI to process requests and notifications.
 *
 */
#define NRF_802154_SWI_IRQ_HANDLER                                          \
    NRFX_CONCAT_3(NRFX_CONCAT_3(SWI, NRF_802154_SWI_EGU_INSTANCE_NO, _EGU), \
                  NRF_802154_SWI_EGU_INSTANCE_NO,                           \
                  _IRQHandler)

/**
 * @def NRF_802154_SWI_IRQN
 *
 * The SWI EGU IRQ number used by the driver for requests and notifications if SWI is in use.
 *
 * @note This option is used when the driver uses SWI to process requests and notifications.
 *
 */
#define NRF_802154_SWI_IRQN                                                 \
    NRFX_CONCAT_3(NRFX_CONCAT_3(SWI, NRF_802154_SWI_EGU_INSTANCE_NO, _EGU), \
                  NRF_802154_SWI_EGU_INSTANCE_NO,                           \
                  _IRQn)

/**
 * @def NRF_802154_RTC_INSTANCE_NO
 *
 * Number of the RTC instance used in the standalone timer driver implementation.
 *
 */
#ifndef NRF_802154_RTC_INSTANCE_NO

#ifdef NRF52811_XXAA
#define NRF_802154_RTC_INSTANCE_NO 0
#else
#define NRF_802154_RTC_INSTANCE_NO 2
#endif

#endif // NRF_802154_RTC_INSTANCE_NO

/**
 * @def NRF_802154_RTC_INSTANCE
 *
 * The RTC instance used in the standalone timer driver implementation.
 *
 * @note This configuration is only applicable for the Low Power Timer Abstraction Layer
 *       implementation in nrf_802154_lp_timer_nodrv.c.
 *
 */
#define NRF_802154_RTC_INSTANCE    NRFX_CONCAT_2(NRF_RTC, NRF_802154_RTC_INSTANCE_NO)

/**
 * @def NRF_802154_RTC_IRQ_HANDLER
 *
 * The RTC interrupt handler name used in the standalone timer driver implementation.
 *
 * @note This configuration is only applicable for Low Power Timer Abstraction Layer implementation
 *       in nrf_802154_lp_timer_nodrv.c.
 *
 */
#define NRF_802154_RTC_IRQ_HANDLER NRFX_CONCAT_3(RTC, NRF_802154_RTC_INSTANCE_NO, _IRQHandler)

/**
 * @def NRF_802154_RTC_IRQN
 *
 * The RTC Interrupt number used in the standalone timer driver implementation.
 *
 * @note This configuration is only applicable for the Low Power Timer Abstraction Layer implementation
 *       in nrf_802154_lp_timer_nodrv.c.
 *
 */
#define NRF_802154_RTC_IRQN        NRFX_CONCAT_3(RTC, NRF_802154_RTC_INSTANCE_NO, _IRQn)

/**
 * @def NRF_802154_PPI_RADIO_DISABLED_TO_EGU
 *
 * The PPI channel that connects RADIO_DISABLED event to EGU task.
 *
 * @note This option is used by the core module regardless of the driver configuration.
 *
 */
#ifndef NRF_802154_PPI_RADIO_DISABLED_TO_EGU
#define NRF_802154_PPI_RADIO_DISABLED_TO_EGU NRF_PPI_CHANNEL6
#endif

/**
 * @def NRF_802154_PPI_EGU_TO_RADIO_RAMP_UP
 *
 * The PPI channel that connects EGU event to RADIO_TXEN or RADIO_RXEN task.
 *
 * @note This option is used by the core module regardless of the driver configuration.
 *
 */
#ifndef NRF_802154_PPI_EGU_TO_RADIO_RAMP_UP
#define NRF_802154_PPI_EGU_TO_RADIO_RAMP_UP NRF_PPI_CHANNEL7
#endif

/**
 * @def NRF_802154_PPI_EGU_TO_TIMER_START
 *
 * The PPI channel that connects EGU event to TIMER_START task.
 *
 * @note This option is used by the core module regardless of the driver configuration.
 *
 */
#ifndef NRF_802154_PPI_EGU_TO_TIMER_START
#define NRF_802154_PPI_EGU_TO_TIMER_START NRF_PPI_CHANNEL8
#endif

/**
 * @def NRF_802154_PPI_RADIO_CRCERROR_TO_TIMER_CLEAR
 *
 * The PPI channel that connects RADIO_CRCERROR event to TIMER_CLEAR task.
 *
 * @note This option is used by the core module regardless of the driver configuration.
 *       The peripheral is shared with @ref NRF_802154_PPI_RADIO_CCAIDLE_TO_FEM_GPIOTE
 *       and @ref NRF_802154_PPI_TIMER_COMPARE_TO_RADIO_TXEN.
 *
 */
#ifndef NRF_802154_PPI_RADIO_CRCERROR_TO_TIMER_CLEAR
#define NRF_802154_PPI_RADIO_CRCERROR_TO_TIMER_CLEAR NRF_PPI_CHANNEL9
#endif

/**
 * @def NRF_802154_PPI_RADIO_CCAIDLE_TO_FEM_GPIOTE
 *
 * The PPI channel that connects RADIO_CCAIDLE event to the GPIOTE tasks used by the Frontend.
 *
 * @note This option is used by the core module regardless of the driver configuration.
 *       The peripheral is shared with @ref NRF_802154_PPI_RADIO_CRCERROR_TO_TIMER_CLEAR
 *       and @ref NRF_802154_PPI_TIMER_COMPARE_TO_RADIO_TXEN.
 *
 */
#ifndef NRF_802154_PPI_RADIO_CCAIDLE_TO_FEM_GPIOTE
#define NRF_802154_PPI_RADIO_CCAIDLE_TO_FEM_GPIOTE NRF_PPI_CHANNEL9
#endif

/**
 * @def NRF_802154_PPI_TIMER_COMPARE_TO_RADIO_TXEN
 *
 * The PPI channel that connects TIMER_COMPARE event to RADIO_TXEN task.
 *
 * @note This option is used by the core module regardless of the driver configuration.
 *       The peripheral is shared with @ref NRF_802154_PPI_RADIO_CRCERROR_TO_TIMER_CLEAR
 *       and @ref NRF_802154_PPI_RADIO_CCAIDLE_TO_FEM_GPIOTE.
 *
 */
#ifndef NRF_802154_PPI_TIMER_COMPARE_TO_RADIO_TXEN
#define NRF_802154_PPI_TIMER_COMPARE_TO_RADIO_TXEN NRF_PPI_CHANNEL9
#endif

/**
 * @def NRF_802154_PPI_RADIO_CRCOK_TO_PPI_GRP_DISABLE
 *
 * The PPI channel that connects RADIO_CRCOK event with the task that disables the whole PPI group.
 *
 * @note This option is used by the core module regardless of the driver configuration.
 *
 */
#ifndef NRF_802154_PPI_RADIO_CRCOK_TO_PPI_GRP_DISABLE
#define NRF_802154_PPI_RADIO_CRCOK_TO_PPI_GRP_DISABLE NRF_PPI_CHANNEL10
#endif

#ifdef NRF_802154_DISABLE_BCC_MATCHING

/**
 * @def NRF_802154_PPI_RADIO_ADDR_TO_COUNTER_COUNT
 *
 * The PPI channel that connects RADIO_ADDRESS event to TIMER_COUNT task.
 *
 * @note This configuration is used only when the NRF_RADIO_EVENT_BCMATCH event handling is disabled
 *       (see @ref NRF_802154_DISABLE_BCC_MATCHING).
 *
 */
#ifndef NRF_802154_PPI_RADIO_ADDR_TO_COUNTER_COUNT
#define NRF_802154_PPI_RADIO_ADDR_TO_COUNTER_COUNT NRF_PPI_CHANNEL11
#endif

/**
 * @def NRF_802154_PPI_RADIO_CRCERROR_TO_COUNTER_CLEAR
 *
 * The PPI channel that connects RADIO_CRCERROR event to TIMER_CLEAR task.
 *
 * @note This option is used only when the NRF_RADIO_EVENT_BCMATCH event handling is disabled
 *       (see @ref NRF_802154_DISABLE_BCC_MATCHING).
 *
 */
#ifndef NRF_802154_PPI_RADIO_CRCERROR_COUNTER_CLEAR
#define NRF_802154_PPI_RADIO_CRCERROR_COUNTER_CLEAR NRF_PPI_CHANNEL12
#endif

/**
 * @def NRF_802154_DISABLE_BCC_MATCHING_PPI_CHANNELS_USED_MASK
 *
 * Helper bit mask of PPI channels used additionally by the 802.15.4 driver when the BCC matching
 * is disabled.
 */
#define NRF_802154_DISABLE_BCC_MATCHING_PPI_CHANNELS_USED_MASK \
    ((1 << NRF_802154_PPI_RADIO_ADDR_TO_COUNTER_COUNT) |       \
     (1 << NRF_802154_PPI_RADIO_CRCERROR_COUNTER_CLEAR))

#else // NRF_802154_DISABLE_BCC_MATCHING

#define NRF_802154_DISABLE_BCC_MATCHING_PPI_CHANNELS_USED_MASK 0

#endif // NRF_802154_DISABLE_BCC_MATCHING

#ifdef NRF_802154_FRAME_TIMESTAMP_ENABLED

/**
 * @def NRF_802154_PPI_RTC_COMPARE_TO_TIMER_CAPTURE
 *
 * The PPI channel that connects LP timer's COMPARE event to HP timer's TIMER_CAPTURE task.
 *
 * @note This option is used only when the timestamping feature is enabled
 *       (see @ref NRF_802154_FRAME_TIMESTAMP_ENABLED).
 *
 */
#ifndef NRF_802154_PPI_RTC_COMPARE_TO_TIMER_CAPTURE
#define NRF_802154_PPI_RTC_COMPARE_TO_TIMER_CAPTURE NRF_PPI_CHANNEL13
#endif

/**
 * @def NRF_802154_PPI_TIMESTAMP_EVENT_TO_TIMER_CAPTURE
 *
 * The PPI channel that connects provided event to HP timer's TIMER_CAPTURE task.
 *
 * @note This option is used only when the timestamping feature is enabled
 *       (see @ref NRF_802154_FRAME_TIMESTAMP_ENABLED).
 *
 */
#ifndef NRF_802154_PPI_TIMESTAMP_EVENT_TO_TIMER_CAPTURE
#define NRF_802154_PPI_TIMESTAMP_EVENT_TO_TIMER_CAPTURE NRF_PPI_CHANNEL14
#endif

/**
 * @def NRF_802154_TIMESTAMP_PPI_CHANNELS_USED_MASK
 *
 * Helper bit mask of PPI channels used by the 802.15.4 driver for timestamping.
 */
#define NRF_802154_TIMESTAMP_PPI_CHANNELS_USED_MASK       \
    ((1 << NRF_802154_PPI_RTC_COMPARE_TO_TIMER_CAPTURE) | \
     (1 << NRF_802154_PPI_TIMESTAMP_EVENT_TO_TIMER_CAPTURE))

#else // NRF_802154_FRAME_TIMESTAMP_ENABLED

#define NRF_802154_TIMESTAMP_PPI_CHANNELS_USED_MASK 0

#endif // NRF_802154_FRAME_TIMESTAMP_ENABLED

/**
 * @def NRF_802154_PPI_CORE_GROUP
 *
 * The PPI channel group used to disable self-disabling PPIs used by the core module.
 *
 * @note This option is used by the core module regardless of the driver configuration.
 *
 */
#ifndef NRF_802154_PPI_CORE_GROUP
#define NRF_802154_PPI_CORE_GROUP NRF_PPI_CHANNEL_GROUP0
#endif

#ifdef NRF_802154_FRAME_TIMESTAMP_ENABLED

/**
 * @def NRF_802154_PPI_TIMESTAMP_GROUP
 *
 * The PPI channel group used to control PPIs used for timestamping.
 *
 * @note This option is used only when the timestamping feature is enabled
 *       (see @ref NRF_802154_FRAME_TIMESTAMP_ENABLED).
 *
 */
#ifndef NRF_802154_PPI_TIMESTAMP_GROUP
#define NRF_802154_PPI_TIMESTAMP_GROUP NRF_PPI_CHANNEL_GROUP1
#endif

#endif // NRF_802154_FRAME_TIMESTAMP_ENABLED

/**
 * @def NRF_802154_TIMERS_USED_MASK
 *
 * Bit mask of instances of timer peripherals used by the 802.15.4 driver.
 */
#ifndef NRF_802154_TIMERS_USED_MASK
#define NRF_802154_TIMERS_USED_MASK ((1 << NRF_802154_HIGH_PRECISION_TIMER_INSTANCE_NO) | \
                                     (1 << NRF_802154_TIMER_INSTANCE_NO) |                \
                                     (1 << NRF_802154_COUNTER_TIMER_INSTANCE_NO))
#endif // NRF_802154_TIMERS_USED_MASK

/**
 * @def NRF_802154_SWI_EGU_USED_MASK
 *
 * Bit mask of instances of SWI/EGU peripherals used by the 802.15.4 driver.
 */
#ifndef NRF_802154_SWI_EGU_USED_MASK
#define NRF_802154_SWI_EGU_USED_MASK (1 << NRF_802154_SWI_EGU_INSTANCE_NO)
#endif

/**
 * @def NRF_802154_RTC_USED_MASK
 *
 * Bit mask of instances of RTC peripherals used by the 802.15.4 driver.
 */
#ifndef NRF_802154_RTC_USED_MASK
#define NRF_802154_RTC_USED_MASK (1 << NRF_802154_RTC_INSTANCE_NO)
#endif

/**
 * @def NRF_802154_GPIO_PINS_USED_MASK
 *
 * Bit mask of GPIO pins used by the 802.15.4 driver.
 */
#ifndef NRF_802154_GPIO_PINS_USED_MASK
#define NRF_802154_GPIO_PINS_USED_MASK (NRF_802154_FEM_PINS_USED_MASK | \
                                        NRF_802154_DEBUG_PINS_USED_MASK)
#endif // NRF_802154_GPIO_PINS_USED_MASK

/**
 * @def NRF_802154_GPIOTE_CHANNELS_USED_MASK
 *
 * Bit mask of GPIOTE peripherals used by the 802.15.4 driver.
 */
#ifndef NRF_802154_GPIOTE_CHANNELS_USED_MASK
#define NRF_802154_GPIOTE_CHANNELS_USED_MASK (NRF_802154_FEM_GPIOTE_CHANNELS_USED_MASK | \
                                              NRF_802154_DEBUG_GPIOTE_CHANNELS_USED_MASK)
#endif // NRF_802154_GPIOTE_CHANNELS_USED_MASK

/**
 * @def NRF_80254_PPI_CHANNELS_USED_MASK
 *
 * Bit mask of PPI channels used by the 802.15.4 driver.
 */
#ifndef NRF_802154_PPI_CHANNELS_USED_MASK
#define NRF_802154_PPI_CHANNELS_USED_MASK ((1 << NRF_802154_PPI_RADIO_DISABLED_TO_EGU) |            \
                                           (1 << NRF_802154_PPI_EGU_TO_RADIO_RAMP_UP) |             \
                                           (1 << NRF_802154_PPI_EGU_TO_TIMER_START) |               \
                                           (1 << NRF_802154_PPI_RADIO_CRCERROR_TO_TIMER_CLEAR) |    \
                                           (1 << NRF_802154_PPI_RADIO_CCAIDLE_TO_FEM_GPIOTE) |      \
                                           (1 << NRF_802154_PPI_TIMER_COMPARE_TO_RADIO_TXEN) |      \
                                           (1 << NRF_802154_PPI_RADIO_CRCOK_TO_PPI_GRP_DISABLE) |   \
                                           NRF_802154_DISABLE_BCC_MATCHING_PPI_CHANNELS_USED_MASK | \
                                           NRF_802154_TIMESTAMP_PPI_CHANNELS_USED_MASK |            \
                                           NRF_802154_FEM_PPI_CHANNELS_USED_MASK |                  \
                                           NRF_802154_DEBUG_PPI_CHANNELS_USED_MASK)
#endif // NRF_802154_PPI_CHANNELS_USED_MASK

/**
 * @def NRF_802154_PPI_GROUPS_USED_MASK
 *
 * Bit mask of PPI groups identifiers used by the 802.15.4 driver.
 */
#ifndef NRF_802154_PPI_GROUPS_USED_MASK

#ifdef NRF_802154_FRAME_TIMESTAMP_ENABLED
#define NRF_802154_PPI_GROUPS_USED_MASK ((1 << NRF_802154_PPI_CORE_GROUP) | \
                                         (1 << NRF_802154_PPI_TIMESTAMP_GROUP))
#else // NRF_802154_FRAME_TIMESTAMP_ENABLED
#define NRF_802154_PPI_GROUPS_USED_MASK (1 << NRF_802154_PPI_CORE_GROUP)
#endif  // NRF_802154_FRAME_TIMESTAMP_ENABLED

#endif // NRF_802154_PPI_GROUPS_USED_MASK

#ifdef __cplusplus
}
#endif

#endif // NRF_802154_PERIPHERALS_H__
