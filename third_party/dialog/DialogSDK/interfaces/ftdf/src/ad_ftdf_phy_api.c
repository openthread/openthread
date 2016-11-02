/**
 ****************************************************************************************
 *
 * @file ftdf.c
 *
 * @brief FTDF FreeRTOS Adapter
 *
 * Copyright (C) 2014. Dialog Semiconductor Ltd, unpublished work. This computer
 * program includes Confidential, Proprietary Information and is a Trade Secret of
 * Dialog Semiconductor Ltd. All use, disclosure, and/or reproduction is prohibited
 * unless authorized in writing. All Rights Reserved.
 *
 * <black.orca.support@diasemi.com> and contributors.
 *
 ****************************************************************************************
 */

#include <string.h>
#include <stdbool.h>

#include "black_orca.h"
#include "ad_ftdf.h"
#include "ad_ftdf_phy_api.h"
#include "internal.h"

#ifndef OS_FREERTOS
static unsigned long uxCriticalNesting = 0x0 ;//0xaaaaaaaa;

void vPortEnterCritical( void )
{
        portDISABLE_INTERRUPTS();
        uxCriticalNesting++;
        __asm volatile( "dsb" );
        __asm volatile( "isb" );
}

/*-----------------------------------------------------------*/

void vPortExitCritical( void )
{

        uxCriticalNesting--;
        if( uxCriticalNesting == 0 )
        {
                portENABLE_INTERRUPTS();
        }
}
#endif /* !OS_FREERTOS */

/**
 * @brief ftdf_gen_irq interrupt service routine
 */
void FTDF_GEN_Handler( void)
{
        FTDF_confirmLmacInterrupt();
        FTDF_eventHandler();
}
/**
 * @brief ftdf_wakup_irq interrupt service routine
 */
void FTDF_WAKEUP_Handler( void)
{
        ad_ftdf_wake_up_async();
}

void ad_ftdf_setExtAddress( FTDF_ExtAddress address)
{
        uExtAddress = address;
}

FTDF_ExtAddress ad_ftdf_getExtAddress( void)
{
        return uExtAddress;
}

void ad_ftdf_wakeUpReady( void)
{
}

FTDF_Status ad_ftdf_send_frame_simple( FTDF_DataLength    frameLength,
                                FTDF_Octet*        frame,
                                FTDF_ChannelNumber channel,
                                FTDF_PTI           pti,
                                FTDF_Boolean       csmaSuppress)
{
        FTDF_criticalVar();
        FTDF_enterCritical();
        if (FTDF_txInProgress == FTDF_TRUE) {
                FTDF_exitCritical();
                return FTDF_TRANSPARENT_OVERFLOW;
        }
        FTDF_txInProgress = FTDF_TRUE;
        FTDF_exitCritical();

        if (sleep_status == BLOCK_SLEEPING) {
                /* Wake-up the block */
                ad_ftdf_wake_up_async();
                sleep_status = BLOCK_ACTIVE;
        }
        return FTDF_sendFrameSimple(frameLength, frame, channel, pti, csmaSuppress);
}


void ad_ftdf_sleep_when_possible( FTDF_Boolean allow_deferred_sleep)
{
        sleep_when_possible(allow_deferred_sleep, 0);
}

void ad_ftdf_wake_up(void)
{
        if (sleep_status == BLOCK_SLEEPING) {
                /* Wake-up the block */
                ad_ftdf_wake_up_async();
                sleep_status = BLOCK_ACTIVE;
        }

}

void ad_ftdf_init_phy_api(void)
{
        NVIC_ClearPendingIRQ(FTDF_WAKEUP_IRQn);
        NVIC_EnableIRQ(FTDF_WAKEUP_IRQn);

        NVIC_ClearPendingIRQ(FTDF_GEN_IRQn);
        NVIC_EnableIRQ(FTDF_GEN_IRQn);
        sleep_status = BLOCK_ACTIVE;
#ifndef OS_FREERTOS
        uxCriticalNesting = 0;
#endif

        FTDF_reset(0);
}
