/**
 ****************************************************************************************
 *
 * @file main.c
 *
 * @brief Main FTDF functions
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

#include <ftdf.h>
#include <string.h>

#include "internal.h"
#include "ftdfInfo.h"

void ftdf_get_release_info(char **lmac_rel_name, char **lmac_build_time, char **umac_rel_name,
                           char **umac_build_time)
{
        static char lrel_name[16];
        static char lbld_time[16];
        static char urel_name[16];
        static char ubld_time[16];
        uint8_t i;

        volatile uint32_t *ptr = &FTDF->FTDF_REL_NAME_0_REG;
        uint32_t *chr_ptr = (uint32_t*)lrel_name;

        for (i = 0; i < 4; i++) {
                *chr_ptr++ = *ptr++;
        }

        ptr = &FTDF->FTDF_BUILDTIME_0_REG;
        chr_ptr = (uint32_t*)lbld_time;

        for (i = 0; i < 4; i++) {
                *chr_ptr++ = *ptr++;
        }

        const char* umac_v_ptr = ftdf_get_umac_rel_name();

        for (i = 0; i < 16; i++) {
                urel_name[i] = umac_v_ptr[i];

                if (umac_v_ptr[i] == '\0') {
                        break;
                }
        }

        umac_v_ptr = ftdf_get_umac_build_time();

        for (i = 0; i < 16; i++) {
                ubld_time[i] = umac_v_ptr[i];

                if (umac_v_ptr[i] == '\0') {
                        break;
                }
        }

        lrel_name[15] = '\0';
        lbld_time[15] = '\0';
        urel_name[15] = '\0';
        ubld_time[15] = '\0';

        *lmac_rel_name = lrel_name;
        *lmac_build_time = lbld_time;
        *umac_rel_name = urel_name;
        *umac_build_time = ubld_time;
}

void ftdf_confirm_lmac_interrupt(void)
{
        REG_SETF(FTDF, FTDF_FTDF_CM_REG, FTDF_CM, 0);
}

void ftdf_event_handler(void)
{
        volatile uint32_t ftdf_ce = FTDF->FTDF_FTDF_CE_REG;

        if (ftdf_ce & FTDF_MSK_RX_CE) {
                ftdf_process_rx_event();
        }

        if (ftdf_ce & FTDF_MSK_TX_CE) {
                ftdf_process_tx_event();
        }

        if (ftdf_ce & FTDF_MSK_SYMBOL_TMR_CE) {
                ftdf_process_symbol_timer_event();
        }

#ifndef FTDF_LITE
#ifndef FTDF_NO_CSL
        if (ftdf_pib.le_enabled) {
                ftdf_time_t cur_time = REG_GETF(FTDF, FTDF_SYMBOLTIMESNAPSHOTVAL_REG,
                                                SYMBOLTIMESNAPSHOTVAL);

                ftdf_time_t delta = cur_time - ftdf_rz_time;

                if (delta < 0x80000000) {
                        /* RZ has passed check if a send frame is pending */
                        if (ftdf_send_frame_pending != 0xfffe) {
                                ftdf_time_t wakeup_start_time;
                                ftdf_period_t wakeup_period;

                                ftdf_critical_var();
                                ftdf_enter_critical();

                                ftdf_get_wakeup_params(ftdf_send_frame_pending, &wakeup_start_time,
                                        &wakeup_period);

                                ftdf_tx_in_progress = FTDF_TRUE;
                                REG_SETF(FTDF, FTDF_LMAC_CONTROL_8_REG, MACCSLSTARTSAMPLETIME,
                                        wakeup_start_time);
                                REG_SETF(FTDF, FTDF_LMAC_CONTROL_7_REG, MACWUPERIOD, wakeup_period);

                                REG_SETF(FTDF, FTDF_TX_SET_OS_REG, TX_FLAG_SET,
                                        ((1 << FTDF_TX_DATA_BUFFER) |
                                        (1 << FTDF_TX_WAKEUP_BUFFER)));

                                ftdf_send_frame_pending = 0xfffe;

                                ftdf_exit_critical();
                        }

                        if (ftdf_tx_in_progress == FTDF_FALSE) {
                                ftdf_set_csl_sample_time();
                        }
                }
        }
#endif /* FTDF_NO_CSL */
#endif /* !FTDF_LITE */
        REG_SETF(FTDF, FTDF_FTDF_CM_REG, FTDF_CM,
                (FTDF_MSK_TX_CE | FTDF_MSK_RX_CE | FTDF_MSK_SYMBOL_TMR_CE));
}

#ifndef FTDF_PHY_API
void ftdf_snd_msg(ftdf_msg_buffer_t* msg_buf)
{
        switch (msg_buf->msg_id) {
#ifndef FTDF_LITE
        case FTDF_DATA_REQUEST:
                ftdf_process_data_request((ftdf_data_request_t*)msg_buf);
                break;
        case FTDF_PURGE_REQUEST:
                ftdf_process_purge_request((ftdf_purge_request_t*)msg_buf);
                break;
        case FTDF_ASSOCIATE_REQUEST:
                ftdf_process_associate_request((ftdf_associate_request_t*)msg_buf);
                break;
        case FTDF_ASSOCIATE_RESPONSE:
                ftdf_process_associate_response((ftdf_associate_response_t*)msg_buf);
                break;
        case FTDF_DISASSOCIATE_REQUEST:
                ftdf_process_disassociate_request((ftdf_disassociate_request_t*)msg_buf);
                break;
#endif /* !FTDF_LITE */
        case FTDF_GET_REQUEST:
                ftdf_process_get_request((ftdf_get_request_t*)msg_buf);
                break;
        case FTDF_SET_REQUEST:
                ftdf_process_set_request((ftdf_set_request_t*)msg_buf);
                break;
#ifndef FTDF_LITE
        case FTDF_ORPHAN_RESPONSE:
                ftdf_process_orphan_response((ftdf_orphan_response_t*)msg_buf);
                break;
#endif /* !FTDF_LITE */
        case FTDF_RESET_REQUEST:
                ftdf_process_reset_request((ftdf_reset_request_t*)msg_buf);
                break;
        case FTDF_RX_ENABLE_REQUEST:
                ftdf_process_rx_enable_request((ftdf_rx_enable_request_t*)msg_buf);
                break;
#ifndef FTDF_LITE
        case FTDF_SCAN_REQUEST:
                ftdf_process_scan_request((ftdf_scan_request_t*)msg_buf);
                break;
        case FTDF_START_REQUEST:
                ftdf_process_start_request((ftdf_start_request_t*)msg_buf);
                break;
        case FTDF_POLL_REQUEST:
                ftdf_process_poll_request((ftdf_poll_request_t*)msg_buf);
                break;
#ifndef FTDF_NO_TSCH
        case FTDF_SET_SLOTFRAME_REQUEST:
                ftdf_process_set_slotframe_request((ftdf_set_slotframe_request_t*)msg_buf);
                break;
        case FTDF_SET_LINK_REQUEST:
                ftdf_process_set_link_request((ftdf_set_link_request_t*)msg_buf);
                break;
        case FTDF_TSCH_MODE_REQUEST:
                ftdf_process_tsch_mode_request((ftdf_tsch_mode_request_t*)msg_buf);
                break;
        case FTDF_KEEP_ALIVE_REQUEST:
                ftdf_process_keep_alive_request((ftdf_keep_alive_request_t*)msg_buf);
                break;
#endif /* FTDF_NO_TSCH */
        case FTDF_BEACON_REQUEST:
                ftdf_process_beacon_request((ftdf_beacon_request_t*)msg_buf);
                break;
#endif /* !FTDF_LITE */
        case FTDF_TRANSPARENT_ENABLE_REQUEST:
                ftdf_enable_transparent_mode(((ftdf_transparent_enable_request_t*)msg_buf)->enable,
                        ((ftdf_transparent_enable_request_t*)msg_buf)->options);
                FTDF_REL_MSG_BUFFER(msg_buf);
                break;
        case FTDF_TRANSPARENT_REQUEST:
                ftdf_process_transparent_request((ftdf_transparent_request_t*)msg_buf);
                break;
        case FTDF_SLEEP_REQUEST:
                FTDF_SLEEP_CALLBACK(((ftdf_sleep_request_t*)msg_buf)->sleep_time);
                FTDF_REL_MSG_BUFFER(msg_buf);
                break;
#ifndef FTDF_LITE
        case FTDF_REMOTE_REQUEST:
                ftdf_process_remote_request((ftdf_remote_request_t*)msg_buf);
                break;
#endif /* !FTDF_LITE */
#if FTDF_DBG_BUS_ENABLE
        case FTDF_DBG_MODE_SET_REQUEST:
                ftdf_set_dbg_mode(((ftdf_dbg_mode_set_request_t *) msg_buf)->dbg_mode);
                FTDF_REL_MSG_BUFFER(msg_buf);
                break;
#endif /* FTDF_DBG_BUS_ENABLE */
        case FTDF_FPPR_MODE_SET_REQUEST:
                ftdf_fppr_set_mode(((ftdf_fppr_mode_set_request_t *) msg_buf)->match_fp,
                        ((ftdf_fppr_mode_set_request_t *) msg_buf)->fp_override,
                        ((ftdf_fppr_mode_set_request_t *) msg_buf)->fp_force);
                FTDF_REL_MSG_BUFFER(msg_buf);
                break;
        default:
                // Silenty release the message buffer
                FTDF_REL_MSG_BUFFER(msg_buf);
                break;
        }
}

void ftdf_send_frame_transparent_confirm(void *handle, ftdf_bitmap32_t status)
{
        ftdf_transparent_confirm_t* confirm =
                (ftdf_transparent_confirm_t*) FTDF_GET_MSG_BUFFER(
                        sizeof(ftdf_transparent_confirm_t));

        confirm->msg_id = FTDF_TRANSPARENT_CONFIRM;
        confirm->handle = handle;
        confirm->status = status;

        FTDF_RCV_MSG((ftdf_msg_buffer_t*)confirm);
}

void ftdf_rcv_frame_transparent(ftdf_data_length_t frame_length, ftdf_octet_t *frame,
                                ftdf_bitmap32_t status, ftdf_link_quality_t link_quality)
{
        ftdf_transparent_indication_t *indication =
                (ftdf_transparent_indication_t*) FTDF_GET_MSG_BUFFER(
                        sizeof(ftdf_transparent_indication_t));

        indication->msg_id = FTDF_TRANSPARENT_INDICATION;
        indication->frame_length = frame_length;
        indication->status = status;
        indication->frame = FTDF_GET_DATA_BUFFER(frame_length);
        memcpy(indication->frame, frame, frame_length);

        FTDF_RCV_MSG((ftdf_msg_buffer_t*)indication);
}
#endif /* !FTDF_PHY_API */
