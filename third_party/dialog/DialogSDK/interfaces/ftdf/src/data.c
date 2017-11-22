/**
 ****************************************************************************************
 *
 * @file data.c
 *
 * @brief FTDF data send/receive functions
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

#include <stdlib.h>

#include <ftdf.h>
#include "internal.h"

#ifdef CONFIG_USE_FTDF
#ifndef FTDF_LITE
void ftdf_process_data_request(ftdf_data_request_t *data_request)
{
#ifndef FTDF_NO_TSCH
        if (ftdf_pib.tsch_enabled && ftdf_tsch_slot_link->request != data_request) {
                ftdf_status_t status;

                if ((data_request->dst_addr_mode == FTDF_SHORT_ADDRESS) && !data_request->indirect_tx) {
                        status = ftdf_schedule_tsch((ftdf_msg_buffer_t*) data_request);

                        if (status == FTDF_SUCCESS) {
                                return;
                        }

                } else {
                        status = FTDF_INVALID_PARAMETER;
                }

                ftdf_send_data_confirm(data_request, status, 0, 0, 0, NULL);

                return;
        }
#endif /* FTDF_NO_TSCH */

        int queue;
        ftdf_address_mode_t dst_addr_mode = data_request->dst_addr_mode;
        ftdf_pan_id_t dst_pan_id = data_request->dst_pan_id;
        ftdf_address_t dst_addr = data_request->dst_addr;

        /* Search for an existing indirect queue */
        for (queue = 0; queue < FTDF_NR_OF_REQ_BUFFERS; queue++) {

                if (dst_addr_mode == FTDF_SHORT_ADDRESS) {

                        if ((ftdf_tx_pending_list[queue].addr_mode == dst_addr_mode) &&
                                (ftdf_tx_pending_list[queue].addr.short_address == dst_addr.short_address)) {
                                break;
                        }

                } else if (dst_addr_mode == FTDF_EXTENDED_ADDRESS) {

                        if ((ftdf_tx_pending_list[queue].addr_mode == dst_addr_mode) &&
                                (ftdf_tx_pending_list[queue].addr.ext_address == dst_addr.ext_address)) {
                                break;
                        }
                }
        }

        if (data_request->indirect_tx) {
                ftdf_status_t status = FTDF_SUCCESS;

                if (queue < FTDF_NR_OF_REQ_BUFFERS) {
                        /* Queue request in existing queue */
                        status = ftdf_queue_req_head((ftdf_msg_buffer_t*) data_request,
                                                     &ftdf_tx_pending_list[queue].queue);

                        if (status == FTDF_SUCCESS) {
                                ftdf_add_tx_pending_timer((ftdf_msg_buffer_t*) data_request, queue,
                                                          (ftdf_pib.transaction_persistence_time *
                                                           FTDF_BASE_SUPERFRAME_DURATION),
                                                          ftdf_send_transaction_expired);
                                return;
                        }
                }

                if ((dst_addr_mode != FTDF_EXTENDED_ADDRESS) && (dst_addr_mode != FTDF_SHORT_ADDRESS)) {
                        status = FTDF_INVALID_PARAMETER;
                }

                if (status != FTDF_SUCCESS) {
                        /* Queueing of indirect transfer was not successful */
                        ftdf_send_data_confirm(data_request, status, 0, 0, 0, NULL);

                        return;
                }
#if FTDF_FP_BIT_MODE == FTDF_FP_BIT_MODE_AUTO
                uint8_t entry, short_addr_idx;
                if (dst_addr_mode == FTDF_SHORT_ADDRESS) {
                        if (ftdf_fppr_get_free_short_address(&entry, &short_addr_idx) == FTDF_FALSE) {
                                goto transaction_overflow;
                        }
                } else if (dst_addr_mode == FTDF_EXTENDED_ADDRESS) {
                        if (ftdf_fppr_get_free_ext_address(&entry) == FTDF_FALSE) {
                                goto transaction_overflow;
                        }
                } else {
                        status = FTDF_INVALID_PARAMETER;
                }
#endif
                /* Search for an empty indirect queue */
                for (queue = 0; queue < FTDF_NR_OF_REQ_BUFFERS; queue++) {
                        if (ftdf_tx_pending_list[queue].addr_mode == FTDF_NO_ADDRESS) {
                                ftdf_tx_pending_list[queue].addr_mode = dst_addr_mode;
                                ftdf_tx_pending_list[queue].pan_id = dst_pan_id;
                                ftdf_tx_pending_list[queue].addr = dst_addr;

                                status  = ftdf_queue_req_head((ftdf_msg_buffer_t*) data_request,
                                                              &ftdf_tx_pending_list[queue].queue);

                                if (status == FTDF_SUCCESS) {
#if FTDF_FP_BIT_MODE == FTDF_FP_BIT_MODE_AUTO
                                if (dst_addr_mode == FTDF_SHORT_ADDRESS) {
                                        ftdf_fppr_set_short_address(entry, short_addr_idx, dst_addr.short_address);
                                        ftdf_fppr_set_short_address_valid(entry, short_addr_idx, FTDF_TRUE);
                                } else if (dst_addr_mode == FTDF_EXTENDED_ADDRESS) {
                                        ftdf_fppr_set_ext_address(entry, dst_addr.short_address);
                                        ftdf_fppr_set_ext_address_valid(entry, FTDF_TRUE);
                                } else {
                                        ASSERT_WARNING(0);
                                }
#endif /* FTDF_FP_BIT_MODE == FTDF_FP_BIT_MODE_AUTO */
                                        ftdf_add_tx_pending_timer((ftdf_msg_buffer_t*) data_request,
                                                                  queue,
                                                                  (ftdf_pib.transaction_persistence_time *
                                                                   FTDF_BASE_SUPERFRAME_DURATION),
                                                                  ftdf_send_transaction_expired);

                                        return;
                                } else {
                                        break;
                                }
                        }
                }

                /* Did not find an existing or an empty queue */
#if FTDF_FP_BIT_MODE == FTDF_FP_BIT_MODE_AUTO
                transaction_overflow:
#endif
                ftdf_send_data_confirm(data_request, FTDF_TRANSACTION_OVERFLOW, 0, 0, 0, NULL);

                return;
        }

        if (ftdf_req_current == NULL) {
                ftdf_req_current = (ftdf_msg_buffer_t*) data_request;
        } else {
                if (ftdf_queue_req_head((ftdf_msg_buffer_t*) data_request, &ftdf_req_queue) ==
                    FTDF_TRANSACTION_OVERFLOW) {

                        ftdf_send_data_confirm(data_request, FTDF_TRANSACTION_OVERFLOW, 0, 0, 0, NULL);
                }

                return;
        }

        ftdf_frame_header_t *frame_header = &ftdf_fh;
        ftdf_security_header *security_header = &ftdf_sh;

        frame_header->frame_type = data_request->send_multi_purpose ? FTDF_MULTIPURPOSE_FRAME : FTDF_DATA_FRAME;

        ftdf_boolean_t frame_pending;

        if (queue < FTDF_NR_OF_REQ_BUFFERS) {
                frame_pending = FTDF_TRUE;
        } else {
                frame_pending = FTDF_FALSE;
        }

        frame_header->options =
            (data_request->security_level > 0 ? FTDF_OPT_SECURITY_ENABLED : 0) |
            (data_request->ack_tx ? FTDF_OPT_ACK_REQUESTED : 0) |
            (frame_pending ? FTDF_OPT_FRAME_PENDING : 0) |
            ((data_request->frame_control_options & FTDF_PAN_ID_PRESENT) ? FTDF_OPT_PAN_ID_PRESENT : 0) |
            ((data_request->frame_control_options & FTDF_IES_INCLUDED) ? FTDF_OPT_IES_PRESENT : 0) |
            ((data_request->frame_control_options & FTDF_SEQ_NR_SUPPRESSED) ? FTDF_OPT_SEQ_NR_SUPPRESSED : 0);

        if (ftdf_pib.le_enabled || ftdf_pib.tsch_enabled) {
                frame_header->options |= FTDF_OPT_ENHANCED;
        }

        frame_header->src_addr_mode = data_request->src_addr_mode;
        frame_header->src_pan_id = ftdf_pib.pan_id;
        frame_header->dst_addr_mode = data_request->dst_addr_mode;
        frame_header->dst_pan_id = data_request->dst_pan_id;
        frame_header->dst_addr = data_request->dst_addr;

        security_header->security_level = data_request->security_level;
        security_header->key_id_mode = data_request->key_id_mode;
        security_header->key_index = data_request->key_index;
        security_header->key_source = data_request->key_source;
        security_header->frame_counter = ftdf_pib.frame_counter;
        security_header->frame_counter_mode = ftdf_pib.frame_counter_mode;

#ifndef FTDF_NO_TSCH
        if (ftdf_pib.tsch_enabled) {
                frame_header->sn = ftdf_process_tsch_sn((ftdf_msg_buffer_t*)data_request,
                                                        ftdf_pib.dsn,
                                                        &data_request->requestSN);
        } else
#endif /* FTDF_NO_TSCH */
        {
                frame_header->sn = ftdf_pib.dsn;
        }

        ftdf_octet_t *tx_ptr = (ftdf_octet_t*) &FTDF->FTDF_TX_FIFO_0_0_REG +
                                                        (FTDF_BUFFER_LENGTH * FTDF_TX_DATA_BUFFER);

        /* Skip PHY header (= MAC length) */
        tx_ptr++;

        ftdf_data_length_t msdu_length = data_request->msdu_length;

        tx_ptr = ftdf_add_frame_header(tx_ptr, frame_header, msdu_length);

        tx_ptr = ftdf_add_security_header(tx_ptr, security_header);

#if !defined(FTDF_NO_CSL) || !defined(FTDF_NO_TSCH)
        if (data_request->frame_control_options & FTDF_IES_INCLUDED) {
                tx_ptr = ftdf_add_ies(tx_ptr,
                                      data_request->header_ie_list,
                                      data_request->payload_ie_list,
                                      data_request->msdu_length);
        }
#endif /* !FTDF_NO_CSL || !FTDF_NO_TSCH */

        ftdf_status_t status = ftdf_send_frame(ftdf_pib.current_channel,
                                               frame_header,
                                               security_header,
                                               tx_ptr,
                                               data_request->msdu_length,
                                               data_request->msdu);

        if (status != FTDF_SUCCESS) {
                ftdf_send_data_confirm(data_request, status, 0, 0, 0, NULL);
                ftdf_req_current = NULL;

                return;
        }

        ftdf_nr_of_retries = 0;

        if (frame_header->sn == ftdf_pib.dsn) {
                ftdf_pib.dsn++;
        }
}

void ftdf_send_data_confirm(ftdf_data_request_t    *data_request,
                            ftdf_status_t          status,
                            ftdf_time_t            timestamp,
                            ftdf_sn_t              dsn,
                            ftdf_num_of_backoffs_t num_of_backoffs,
                            ftdf_ie_list_t         *ack_payload)
{
        FTDF_REL_DATA_BUFFER(data_request->msdu);

        ftdf_data_confirm_t *data_confirm =
            (ftdf_data_confirm_t*) FTDF_GET_MSG_BUFFER(sizeof(ftdf_data_confirm_t));

        data_confirm->msg_id = FTDF_DATA_CONFIRM;
        data_confirm->msdu_handle = data_request->msdu_handle;
        data_confirm->status = status;
        data_confirm->timestamp = timestamp;
        data_confirm->num_of_backoffs = num_of_backoffs;
        data_confirm->dsn = dsn;
        data_confirm->ack_payload = ack_payload;

        if (ftdf_req_current == (ftdf_msg_buffer_t*)data_request) {
                ftdf_req_current = NULL;
        }

        FTDF_REL_MSG_BUFFER((ftdf_msg_buffer_t*) data_request);
        FTDF_RCV_MSG((ftdf_msg_buffer_t*) data_confirm);
#if FTDF_FP_BIT_MODE == FTDF_FP_BIT_MODE_AUTO
        ftdf_fp_fsm_clear_pending();
#endif /* FTDF_FP_BIT_MODE == FTDF_FP_BIT_MODE_AUTO */
        ftdf_process_next_request();
}

void ftdf_send_data_indication(ftdf_frame_header_t  *frame_header,
                               ftdf_security_header *security_header,
                               ftdf_ie_list_t       *payload_ie_list,
                               ftdf_data_length_t   msdu_length,
                               ftdf_octet_t         *msdu,
                               ftdf_link_quality_t  mpdu_link_quality,
                               ftdf_time_t          timestamp)
{
        ftdf_data_indication_t *data_indication =
            (ftdf_data_indication_t*) FTDF_GET_MSG_BUFFER(sizeof(ftdf_data_indication_t));

        ftdf_octet_t *msdu_buf = FTDF_GET_DATA_BUFFER(msdu_length);
        ftdf_octet_t *buf_ptr = msdu_buf;

        int n;

        for (n = 0; n < msdu_length; n++) {
                *msdu_buf++ = *msdu++;
        }

        data_indication->msg_id = FTDF_DATA_INDICATION;
        data_indication->src_addr_mode = frame_header->src_addr_mode;
        data_indication->src_pan_id = frame_header->src_pan_id;
        data_indication->src_addr = frame_header->src_addr;
        data_indication->dst_addr_mode = frame_header->dst_addr_mode;
        data_indication->dst_pan_id = frame_header->dst_pan_id;
        data_indication->dst_addr = frame_header->dst_addr;
        data_indication->msdu_length = msdu_length;
        data_indication->msdu = buf_ptr;
        data_indication->mpdu_link_quality = mpdu_link_quality;
        data_indication->dsn = frame_header->sn;
        data_indication->timestamp = timestamp;
        data_indication->security_level = security_header->security_level;
        data_indication->key_id_mode = security_header->key_id_mode;
        data_indication->key_index = security_header->key_index;
        data_indication->payload_ie_list = payload_ie_list;

        if (data_indication->key_id_mode == 0x2) {
                for (n = 0; n < 4; n++) {
                        data_indication->key_source[n] = security_header->key_source[n];
                }
        } else if (data_indication->key_id_mode == 0x3) {
                for (n = 0; n < 8; n++) {
                        data_indication->key_source[n] = security_header->key_source[n];
                }
        }

        FTDF_RCV_MSG((ftdf_msg_buffer_t*) data_indication);
}

void ftdf_process_poll_request(ftdf_poll_request_t *poll_request)
{
        if (ftdf_req_current == NULL) {
                ftdf_req_current = (ftdf_msg_buffer_t*) poll_request;
        } else {
                if (ftdf_queue_req_head((ftdf_msg_buffer_t*) poll_request, &ftdf_req_queue) ==
                    FTDF_TRANSACTION_OVERFLOW) {
                        ftdf_send_poll_confirm(poll_request, FTDF_TRANSACTION_OVERFLOW);
                }

                return;
        }

        ftdf_frame_header_t *frame_header = &ftdf_fh;
        ftdf_security_header *security_header = &ftdf_sh;

        frame_header->frame_type = FTDF_MAC_COMMAND_FRAME;
        frame_header->options =
            (poll_request->security_level > 0 ? FTDF_OPT_SECURITY_ENABLED : 0) | FTDF_OPT_ACK_REQUESTED;

        if (ftdf_pib.short_address < 0xfffe) {
                frame_header->src_addr_mode = FTDF_SHORT_ADDRESS;
                frame_header->src_addr.short_address = ftdf_pib.short_address;
        } else {
                frame_header->src_addr_mode = FTDF_EXTENDED_ADDRESS;
                frame_header->src_addr.ext_address = ftdf_pib.ext_address;
        }

        frame_header->src_pan_id = ftdf_pib.pan_id;
        frame_header->dst_addr_mode = poll_request->coord_addr_mode;
        frame_header->dst_pan_id = poll_request->coord_pan_id;
        frame_header->dst_addr = poll_request->coord_addr;

        security_header->security_level = poll_request->security_level;
        security_header->key_id_mode = poll_request->key_id_mode;
        security_header->key_index = poll_request->key_index;
        security_header->key_source = poll_request->key_source;
        security_header->frame_counter = ftdf_pib.frame_counter;
        security_header->frame_counter_mode = ftdf_pib.frame_counter_mode;

        ftdf_octet_t *tx_ptr = (ftdf_octet_t*) &FTDF->FTDF_TX_FIFO_0_0_REG + (FTDF_BUFFER_LENGTH * FTDF_TX_DATA_BUFFER);

        /* Skip PHY header (= MAC length) */
        tx_ptr++;

        frame_header->sn = ftdf_pib.dsn;

        tx_ptr = ftdf_add_frame_header(tx_ptr, frame_header, 1);

        tx_ptr = ftdf_add_security_header(tx_ptr, security_header);

        *tx_ptr++ = FTDF_COMMAND_DATA_REQUEST;

        ftdf_status_t status = ftdf_send_frame(ftdf_pib.current_channel,
                                               frame_header,
                                               security_header,
                                               tx_ptr,
                                               0,
                                               NULL);

        if (status != FTDF_SUCCESS) {
                ftdf_send_poll_confirm(poll_request, status);
                return;
        }

        ftdf_nr_of_retries = 0;
        ftdf_pib.dsn++;
}

void ftdf_send_poll_confirm(ftdf_poll_request_t *poll_request, ftdf_status_t status)
{
        ftdf_poll_confirm_t *poll_confirm = (ftdf_poll_confirm_t*) FTDF_GET_MSG_BUFFER(sizeof(ftdf_poll_confirm_t));

        poll_confirm->msg_id  = FTDF_POLL_CONFIRM;
        poll_confirm->status = status;

        if (ftdf_req_current == (ftdf_msg_buffer_t*)poll_request) {
                ftdf_req_current = NULL;
        }

        FTDF_REL_MSG_BUFFER((ftdf_msg_buffer_t*) poll_request);
        FTDF_RCV_MSG((ftdf_msg_buffer_t*) poll_confirm);

        ftdf_process_next_request();
}

void ftdf_process_purge_request(ftdf_purge_request_t *purge_request)
{
        ftdf_handle_t msdu_handle = purge_request->msdu_handle;
        ftdf_status_t status = FTDF_INVALID_HANDLE;

        int n;

        for (n = 0; n < FTDF_NR_OF_REQ_BUFFERS; n++) {
                ftdf_msg_buffer_t *request =
                    ftdf_dequeue_by_handle(msdu_handle, &ftdf_tx_pending_list[n].queue);

                if (request) {
                        ftdf_data_request_t *data_request = (ftdf_data_request_t*) request;

                        if (data_request->indirect_tx == FTDF_TRUE) {
                                ftdf_remove_tx_pending_timer(request);
#if FTDF_FP_BIT_MODE == FTDF_FP_BIT_MODE_AUTO
                        if (ftdf_tx_pending_list[n].addr_mode == FTDF_SHORT_ADDRESS) {
                                uint8_t entry, shortAddrIdx;
                                ftdf_boolean_t found = ftdf_fppr_lookup_short_address(
                                    ftdf_tx_pending_list[n].addr.short_address, &entry,
                                        &shortAddrIdx);
                                ASSERT_WARNING(found);
                                ftdf_fppr_set_short_address_valid(entry, shortAddrIdx, FTDF_FALSE);
                        } else if (ftdf_tx_pending_list[n].addr_mode  == FTDF_EXTENDED_ADDRESS) {
                                uint8_t entry;
                                ftdf_boolean_t found = ftdf_fppr_lookup_ext_address(
                                    ftdf_tx_pending_list[n].addr.ext_address, &entry);
                                ASSERT_WARNING(found);
                                ftdf_fppr_set_ext_address_valid(entry, FTDF_FALSE);
                        } else {
                                ASSERT_WARNING(0);
                        }
#endif /* FTDF_FP_BIT_MODE == FTDF_FP_BIT_MODE_AUTO */
                                if (ftdf_is_queue_empty(&ftdf_tx_pending_list[n].queue)) {
                                        ftdf_tx_pending_list[n].addr_mode = FTDF_NO_ADDRESS;
                                }
                        }

                        FTDF_REL_DATA_BUFFER(data_request->msdu);
                        FTDF_REL_MSG_BUFFER((ftdf_msg_buffer_t*) data_request);

                        status = FTDF_SUCCESS;

                        break;
                }
        }

        ftdf_purge_confirm_t *purge_confirm =
            (ftdf_purge_confirm_t*) FTDF_GET_MSG_BUFFER(sizeof(ftdf_purge_confirm_t));

        purge_confirm->msg_id = FTDF_PURGE_CONFIRM;
        purge_confirm->msdu_handle = msdu_handle;
        purge_confirm->status = status;

        FTDF_REL_MSG_BUFFER((ftdf_msg_buffer_t*) purge_request);
        FTDF_RCV_MSG((ftdf_msg_buffer_t*) purge_confirm);
}
#endif /* !FTDF_LITE */

#ifdef FTDF_PHY_API
ftdf_status_t ftdf_send_frame_simple(ftdf_data_length_t    frame_length,
                                     ftdf_octet_t          *frame,
                                     ftdf_channel_number_t channel,
                                     ftdf_pti_t            pti,
                                     ftdf_boolean_t        csma_suppress)
{

        ftdf_octet_t *fp = frame;

        if ((frame_length > 127) || (ftdf_transparent_mode != FTDF_TRUE)) {
                return FTDF_INVALID_PARAMETER;
        }

        ftdf_octet_t *tx_ptr =
            (ftdf_octet_t*) &FTDF->FTDF_TX_FIFO_0_0_REG + (FTDF_BUFFER_LENGTH * FTDF_TX_DATA_BUFFER);

        /* This data copy could/should be optimized using DMA */
        *tx_ptr++ = frame_length;

        int n;

        for (n = 0; n < frame_length; n++) {
                *tx_ptr++ = *fp++;
        }

        ftdf_critical_var();
        ftdf_enter_critical();
        ftdf_nr_of_retries = 0;
        ftdf_exit_critical();
        ftdf_send_transparent_frame(frame_length,
                                    frame,
                                    channel,
                                    pti,
                                    csma_suppress);

        return FTDF_SUCCESS;
}



#else /* !FTDF_PHY_API */
void ftdf_process_transparent_request(ftdf_transparent_request_t *transparent_request)
{
        ftdf_data_length_t frame_length = transparent_request->frame_length;

        if ((frame_length > 127) || (ftdf_transparent_mode != FTDF_TRUE)) {
                FTDF_SEND_FRAME_TRANSPARENT_CONFIRM(transparent_request->handle,
                                                    FTDF_INVALID_PARAMETER);

                FTDF_REL_MSG_BUFFER((ftdf_msg_buffer_t*) transparent_request);

                return;
        }

        if (ftdf_req_current == NULL) {
                ftdf_req_current = (ftdf_msg_buffer_t*) transparent_request;
        } else {
#ifndef FTDF_LITE
                if (ftdf_queue_req_head((ftdf_msg_buffer_t*) transparent_request, &ftdf_req_queue) ==
                    FTDF_TRANSACTION_OVERFLOW) {
#endif /* !FTDF_LITE */
                        FTDF_SEND_FRAME_TRANSPARENT_CONFIRM(transparent_request->handle,
                                                            FTDF_TRANSPARENT_OVERFLOW);

                        FTDF_REL_MSG_BUFFER((ftdf_msg_buffer_t*) transparent_request);
#ifndef FTDF_LITE
                }
#endif /* !FTDF_LITE */
                return;
        }

        ftdf_octet_t *tx_ptr =
            (ftdf_octet_t*) &FTDF->FTDF_TX_FIFO_0_0_REG + (FTDF_BUFFER_LENGTH * FTDF_TX_DATA_BUFFER);

        *tx_ptr++ = frame_length;

        ftdf_octet_t *frame = transparent_request->frame;

        int n;

        for (n = 0; n < frame_length; n++) {
                *tx_ptr++ = *frame++;
        }

        ftdf_send_transparent_frame(frame_length,
                                    transparent_request->frame,
                                    transparent_request->channel,
                                    transparent_request->pti,
                                    transparent_request->cmsa_suppress);
        ftdf_nr_of_retries = 0;
}

void ftdf_send_frame_transparent(ftdf_data_length_t    frame_length,
                                 ftdf_octet_t          *frame,
                                 ftdf_channel_number_t channel,
                                 ftdf_pti_t            pti,
                                 ftdf_boolean_t        cmsa_suppress,
                                 void                  *handle)
{
        ftdf_transparent_request_t *transparent_request =
            (ftdf_transparent_request_t*) FTDF_GET_MSG_BUFFER(sizeof(ftdf_transparent_request_t));

        transparent_request->msg_id = FTDF_TRANSPARENT_REQUEST;
        transparent_request->frame_length  = frame_length;
        transparent_request->frame = frame;
        transparent_request->channel = channel;
        transparent_request->pti = pti;
        transparent_request->cmsa_suppress = cmsa_suppress;
        transparent_request->handle = handle;

        ftdf_process_transparent_request(transparent_request);
}

#endif /* FTDF_PHY_API */
#endif /* CONFIG_USE_FTDF */
