/**
 ****************************************************************************************
 *
 * @file ftdf_config_phy_api.h
 *
 * @brief FTDF PHY API configuration template file
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

#ifndef FTDF_CONFIG_PHY_API_H_
#define FTDF_CONFIG_PHY_API_H_

#include "osal.h" // OS_FREERTOS macro is checked within the header file.

/**
 * \remark      phy configuration values in microseconds
 */
#define FTDF_PHYTXSTARTUP 0x4c
#define FTDF_PHYTXLATENCY 0x02
#define FTDF_PHYTXFINISH  0x00
#define FTDF_PHYTRXWAIT   0x22
#define FTDF_PHYRXSTARTUP 0x54
#define FTDF_PHYRXLATENCY 0
#define FTDF_PHYENABLE    0x20

#ifndef FTDF_LITE
#define FTDF_LITE
#endif

/**
 * \remark      See FTDF_GET_MSG_BUFFER in ftdf.h
 */
#define FTDF_GET_MSG_BUFFER                 ad_ftdf_get_msg_buffer

/**
 * \remark      See FTDF_REL_MSG_BUFFER in ftdf.h
 */
#define FTDF_REL_MSG_BUFFER                 ad_ftdf_rel_msg_buffer

/**
 * \remark      See FTDF_REC_MSG in ftdf.h
 */
#define FTDF_RCV_MSG                        ad_ftdf_rcv_msg

/**
 * \remark      See FTDF_GET_DATA_BUFFER in ftdf.h
 */
#define FTDF_GET_DATA_BUFFER                ad_ftdf_get_data_buffer

/**
 * \remark      See FTDF_REL_DATA_BUFFER in ftdf.h
 */
#define FTDF_REL_DATA_BUFFER                ad_ftdf_rel_data_buffer

/**
 * \remark      See FTDF_GET_EXT_ADDRESS in ftdf.h
 */
#define FTDF_GET_EXT_ADDRESS                ad_ftdf_get_ext_address

/**
 * \remark      See FTDF_RCV_FRAME_TRANSPARENT in ftdf.h
 */
#define FTDF_RCV_FRAME_TRANSPARENT          ftdf_rcv_frame_transparent

/**
 * \remark      See FTDF_SEND_FRAME_TRANSPARENT_CONFIRM in ftdf.h
 */
#define FTDF_SEND_FRAME_TRANSPARENT_CONFIRM ftdf_send_frame_transparent_confirm

/**
 * \remark      See FTDF_WAKE_UP_READY in ftdf.h
 */
#define FTDF_WAKE_UP_READY                  ad_ftdf_wake_up_ready

/**
 * \remark      See FTDF_SLEEP_CALLBACK in ftdf.h
 */
#define FTDF_SLEEP_CALLBACK                 ad_ftdf_sleep_cb

/* Forward declaration */
void sleep_when_possible(uint8_t explicit_request, uint32_t sleep_time);

#define FTDF_LMACREADY4SLEEP_CB             sleep_when_possible

#ifndef OS_FREERTOS
#define portDISABLE_INTERRUPTS() \
        do { \
                __asm volatile  (" cpsid i "); \
                DBG_CONFIGURE_HIGH(CMN_TIMING_DEBUG, CMNDBG_CRITICAL_SECTION); \
        } while (0)
#define portENABLE_INTERRUPTS() \
        do { \
                DBG_CONFIGURE_LOW(CMN_TIMING_DEBUG, CMNDBG_CRITICAL_SECTION); \
                __asm volatile  (" cpsie i "); \
        } while (0)
void vPortEnterCritical(void);
void vPortExitCritical(void);
#define OS_ENTER_CRITICAL_SECTION vPortEnterCritical
#define OS_LEAVE_CRITICAL_SECTION vPortExitCritical
#endif

/**
 * \remark      Critical section
 */

#define ftdf_critical_var( )
#define ftdf_enter_critical( ) OS_ENTER_CRITICAL_SECTION()
#define ftdf_exit_critical( ) OS_LEAVE_CRITICAL_SECTION()

#ifndef FTDF_DBG_BUS_ENABLE
/**
 * \brief Whether FTDF debug bus will be available or not.
 *
 * Define to 0 for production software.
 *
 * Refer to \ref ad_ftdf_dbgBusGpioConfig for the GPIO pins used for the debug bus.
 */
#define FTDF_DBG_BUS_ENABLE         0
#endif

#if FTDF_DBG_BUS_ENABLE
#define FTDF_DBG_BUS_GPIO_CONFIG   ad_ftdf_dbg_bus_gpio_config

#ifndef FTDF_DBG_BUS_USE_GPIO_P1_3_P2_2
/**
 * \brief Enables FTDF diagnostics on diagnostic pins 6 and 7 on GPIO P1_3 and P2_3.
 *
 * When enabled, UART must use pins other than the default P1_3, P2_3.
 */
#define FTDF_DBG_BUS_USE_GPIO_P1_3_P2_2         (0)
#endif

#ifndef FTDF_DBG_BUS_USE_SWDIO_PIN
/**
 * \brief Enables diagnostics on diagnostic pin 4 on GPIO P0_6.
 *
 * When enabled, the debugger must be disabled since SWD uses the same pin for SWDIO.
 */
#define FTDF_DBG_BUS_USE_SWDIO_PIN              (0)
#endif

#ifndef FTDF_DBG_BUS_USE_PORT_4
/**
 * \brief Uses Port 4 (instead of GPIOs at Ports 0, 1 and 2) for diagnostics
 *
 * When enabled, FTDF diagnostics pins uses P4_0 to P4_7
 */
#define FTDF_DBG_BUS_USE_PORT_4                 (0)
#endif

#endif /* FTDF_DBG_BUS_ENABLE */

#ifndef FTDF_USE_AUTO_PTI
#define FTDF_USE_AUTO_PTI                       0
#endif

#ifndef FTDF_USE_FP_PROCESSING_RAM
/**
 * \brief Whether to use HW acceleration for indirect sending.
 *
 */
#define FTDF_USE_FP_PROCESSING_RAM              1
#endif /* FTDF_USE_FP_PROCESSING_RAM */

#endif /* FTDF_CONFIG_PHY_API_H_ */

