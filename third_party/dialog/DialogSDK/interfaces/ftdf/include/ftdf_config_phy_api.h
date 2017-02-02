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

/**
 * \remark      phy configuration values in microseconds
 */
#define FTDF_PHYTXSTARTUP 0x4c
#define FTDF_PHYTXLATENCY 0x01
#define FTDF_PHYTXFINISH  0x00
#define FTDF_PHYTRXWAIT   0x20
#define FTDF_PHYRXSTARTUP 0
#define FTDF_PHYRXLATENCY 0
#define FTDF_PHYENABLE    0x21

#ifndef FTDF_LITE
#define FTDF_LITE
#endif

/**
 * \remark      See FTDF_GET_MSG_BUFFER in ftdf.h
 */
#define FTDF_GET_MSG_BUFFER        ad_ftdf_getMsgBuffer

/**
 * \remark      See FTDF_REL_MSG_BUFFER in ftdf.h
 */
#define FTDF_REL_MSG_BUFFER        ad_ftdf_relMsgBuffer

/**
 * \remark      See FTDF_REC_MSG in ftdf.h
 */
#define FTDF_RCV_MSG               ad_ftdf_rcvMsg

/**
 * \remark      See FTDF_GET_DATA_BUFFER in ftdf.h
 */
#define FTDF_GET_DATA_BUFFER       ad_ftdf_getDataBuffer

/**
 * \remark      See FTDF_REL_DATA_BUFFER in ftdf.h
 */
#define FTDF_REL_DATA_BUFFER       ad_ftdf_relDataBuffer

/**
 * \remark      See FTDF_GET_EXT_ADDRESS in ftdf.h
 */
#define FTDF_GET_EXT_ADDRESS       ad_ftdf_getExtAddress

/**
 * \remark      See FTDF_RCV_FRAME_TRANSPARENT in ftdf.h
 */
#define FTDF_RCV_FRAME_TRANSPARENT FTDF_rcvFrameTransparent

/**
 * \remark      See FTDF_SEND_FRAME_TRANSPARENT_CONFIRM in ftdf.h
 */
#define FTDF_SEND_FRAME_TRANSPARENT_CONFIRM FTDF_sendFrameTransparentConfirm

/**
 * \remark      See FTDF_WAKE_UP_READY in ftdf.h
 */
#define FTDF_WAKE_UP_READY         ad_ftdf_wakeUpReady

/**
 * \remark      See FTDF_SLEEP_CALLBACK in ftdf.h
 */
#define FTDF_SLEEP_CALLBACK        ad_ftdf_sleepCb

/* Forward declaration */
void sleep_when_possible( uint8_t explicit_request, uint32_t sleepTime );

#define FTDF_LMACREADY4SLEEP_CB    sleep_when_possible

#ifndef OS_FREERTOS
#define portDISABLE_INTERRUPTS() \
        do { \
                __asm volatile  ( " cpsid i " ); \
                DBG_CONFIGURE_HIGH(CMN_TIMING_DEBUG, CMNDBG_CRITICAL_SECTION); \
        } while (0)
#define portENABLE_INTERRUPTS() \
        do { \
                DBG_CONFIGURE_LOW(CMN_TIMING_DEBUG, CMNDBG_CRITICAL_SECTION); \
                __asm volatile  ( " cpsie i " ); \
        } while (0)
void vPortEnterCritical( void );
void vPortExitCritical( void );
#endif

/**
 * \remark      Critical section
 */


#define FTDF_criticalVar( )
#define FTDF_enterCritical( ) vPortEnterCritical()
#define FTDF_exitCritical( ) vPortExitCritical()

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
#define FTDF_DBG_BUS_GPIO_CONFIG   ad_ftdf_dbgBusGpioConfig
#endif /* FTDF_DBG_BUS_ENABLE */

#ifndef FTDF_USE_FP_PROCESSING_RAM
/**
 * \brief Whether to use HW acceleration for indirect sending.
 *
 * Feature is supported as of IC revision 14683-00.
 */
#if dg_configBLACK_ORCA_IC_REV != BLACK_ORCA_IC_REV_A
#define FTDF_USE_FP_PROCESSING_RAM              1
#else
#define FTDF_USE_FP_PROCESSING_RAM              0
#endif
#endif /* FTDF_USE_FP_PROCESSING_RAM */

#endif /* FTDF_CONFIG_PHY_API_H_ */

