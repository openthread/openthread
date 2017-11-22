/**
 ****************************************************************************************
 *
 * @file internal.h
 *
 * @brief Internal header file
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

#define FTDF_TBD                   0

#define FTDF_FCS_LENGTH            2
#define FTDF_BUFFER_LENGTH         128
#ifndef FTDF_NR_OF_REQ_BUFFERS
#define FTDF_NR_OF_REQ_BUFFERS     96
#endif
#define FTDF_NR_OF_RX_BUFFERS      8
#define FTDF_NR_OF_CHANNELS        16
#define FTDF_NR_OF_SCAN_RESULTS    16
#define FTDF_NR_OF_CSL_PEERS       16
#define FTDF_MAX_HEADER_IES        8
#define FTDF_MAX_PAYLOAD_IES       4
#define FTDF_MAX_SUB_IES           8
#define FTDF_MAX_IE_CONTENT        128
#define FTDF_NR_OF_RX_ADDRS        16
#define FTDF_NR_OF_NEIGHBORS       16

#define FTDF_TX_DATA_BUFFER        0
#define FTDF_TX_WAKEUP_BUFFER      1
#define FTDF_TX_ACK_BUFFER         2

#define FTDF_OPT_SECURITY_ENABLED  0x01
#define FTDF_OPT_FRAME_PENDING     0x02
#define FTDF_OPT_SEQ_NR_SUPPRESSED 0x04
#define FTDF_OPT_ACK_REQUESTED     0x08
#define FTDF_OPT_IES_PRESENT       0x10
#define FTDF_OPT_PAN_ID_PRESENT    0x20
#define FTDF_OPT_ENHANCED          0x40

#define FTDF_MSK_RX_CE             0x00000002
#define FTDF_MSK_SYMBOL_TMR_CE     0x00000008
#define FTDF_MSK_TX_CE             0x00000010

#define FTDF_TX_FLAG_CLEAR_M_0_REG_INTV                 0x20
#define FTDF_TX_FLAG_CLEAR_E_0_REG_INTV                 0x20
#define FTDF_TX_PRIORITY_0_REG_INTV                     0x20
#define FTDF_RX_META_1_0_REG_INTV                       0x10
#define FTDF_TX_FLAG_S_0_REG_INTV                       0x20
#define FTDF_TX_RETURN_STATUS_1_0_REG_INTV              0x10
#define FTDF_TX_META_DATA_0_0_REG_INTV                  0x10
#define FTDF_TX_RETURN_STATUS_1_0_REG_INTV              0x10
#define FTDF_TX_META_DATA_1_0_REG_INTV                  0x10

// The maximum time in symbols that is takes to process request. After succesful
// processing a request the TX buffer is fully initialized and ready to be sent.
#define FTDF_TSCH_MAX_PROCESS_REQUEST_TIME 35

// The maximum time in symbols needed to search for the next slot for which a
// TX or RX needs to be done
#define FTDF_TSCH_MAX_SCHEDULE_TIME 30

#define ftdf_process_request ftdf_snd_msg

typedef struct {
    ftdf_ext_address_t          ext_address;
    ftdf_period_t               ack_wait_duration;
#ifndef FTDF_LITE
    ftdf_boolean_t              associated_pan_coord;
    ftdf_boolean_t              association_permit;
    ftdf_boolean_t              auto_request;
    ftdf_boolean_t              batt_life_ext;
    ftdf_num_of_backoffs_t      batt_life_ext_periods;
    ftdf_octet_t                beacon_payload[FTDF_MAX_BEACON_PAYLOAD_LENGTH];
    ftdf_size_t                 beacon_payload_length;
    ftdf_order_t                beacon_order;
    ftdf_time_t                 beacon_tx_time;
    ftdf_sn_t                   bsn;
    ftdf_ext_address_t          coord_ext_address;
    ftdf_short_address_t        coord_short_address;
    ftdf_sn_t                   dsn;
    ftdf_boolean_t              gts_permit;
#endif /* !FTDF_LITE */
    ftdf_be_exponent_t          max_be;
    ftdf_size_t                 max_csma_backoffs;
    ftdf_period_t               max_frame_total_wait_time;
    ftdf_size_t                 max_frame_retries;
    ftdf_be_exponent_t          min_be;
#ifndef FTDF_LITE
    ftdf_period_t               lifs_period;
    ftdf_period_t               sifs_period;
#endif /* !FTDF_LITE */
    ftdf_pan_id_t               pan_id;
#ifndef FTDF_LITE
    ftdf_boolean_t              promiscuous_mode; /* Not supported. Use transparent mode instead */
    ftdf_size_t                 response_wait_time;
#endif /* !FTDF_LITE */
    ftdf_boolean_t              rx_on_when_idle;
#ifndef FTDF_LITE
    ftdf_boolean_t              security_enabled;
#endif /* !FTDF_LITE */
    ftdf_short_address_t        short_address;
#ifndef FTDF_LITE
    ftdf_size_t                 superframe_order;
    ftdf_period_t               sync_symbol_offset;
    ftdf_boolean_t              timestamp_supported;
    ftdf_period_t               transaction_persistence_time;
    ftdf_period_t               enh_ack_wait_duration;
    ftdf_boolean_t              implicit_broadcast;
    ftdf_simple_address_t       simple_address;
    ftdf_period_t               disconnect_time;
    ftdf_priority_t             join_priority;
    ftdf_asn_t                  asn;
    ftdf_boolean_t              no_hl_buffers;
    ftdf_slotframe_table_t      slotframe_table;
    FTDF_LinkTable              link_table;
    ftdf_timeslot_template_t    timeslot_template;
    ftdf_hopping_sequence_id_t  hopping_sequence_id;
    ftdf_channel_page_t         channel_page;
    ftdf_channel_number_t       number_of_channels;
    ftdf_bitmap32_t             phy_configuration;
    ftdf_bitmap8_t              extended_bitmap;
    ftdf_length_t               hopping_sequence_length;
    ftdf_channel_number_t       hopping_sequence_list[FTDF_MAX_HOPPING_SEQUENCE_LENGTH];
    ftdf_length_t               current_hop;
    ftdf_period_t               dwell_time;
    ftdf_period_t               csl_period;
    ftdf_period_t               csl_max_period;
    ftdf_bitmap32_t             csl_channel_mask;
    ftdf_period_t               csl_frame_pending_wait;
    ftdf_boolean_t              low_energy_superframe_supported;
    ftdf_boolean_t              low_energy_superframe_sync_interval;
#endif /* !FTDF_LITE */
    ftdf_performance_metrics_t  performance_metrics;
#ifndef FTDF_LITE
    ftdf_boolean_t              use_enhanced_becaon;
    ftdf_ie_list_t              eb_ie_list;
    ftdf_boolean_t              eb_filtering_enabled;
    ftdf_sn_t                   eb_sn;
    ftdf_auto_sa_t              eb_auto_sa;
    ftdf_ie_list_t              e_ack_ie_list;
    ftdf_boolean_t              tsch_enabled;
    ftdf_boolean_t              le_enabled;
#endif /* !FTDF_LITE */
    ftdf_channel_number_t       current_channel;
    ftdf_tx_power_tolerance_t   tx_power_tolerance;
    ftdf_dbm                    tx_power;
    ftdf_cca_mode_t             cca_mode;
#ifndef FTDF_LITE
    ftdf_channel_page_t         current_page;
    ftdf_period_t               max_frame_duration;
    ftdf_period_t               shr_duration;
    ftdf_key_table_t            key_table;
    ftdf_device_table_t         device_table;
    ftdf_security_level_table_t security_level_table;
    ftdf_frame_counter_t        frame_counter;
    ftdf_security_level_t       mt_data_security_level;
    ftdf_key_id_mode_t          mt_data_key_id_mode;
    ftdf_octet_t                mt_data_key_source[8];
    ftdf_key_index_t            mt_data_key_index;
    ftdf_octet_t                default_key_source[8];
    ftdf_ext_address_t          pan_coord_ext_address;
    ftdf_short_address_t        pan_coord_short_address;
    ftdf_frame_counter_mode_t   frame_counter_mode;
    ftdf_period_t               csl_sync_tx_margin;
    ftdf_period_t               csl_max_age_remote_info;
#endif /* !FTDF_LITE */
    ftdf_traffic_counters_t     traffic_counters;
#ifndef FTDF_LITE
    ftdf_boolean_t              le_capable;
    ftdf_boolean_t              ll_capable;
    ftdf_boolean_t              dsme_capable;
    ftdf_boolean_t              rfid_capable;
    ftdf_boolean_t              amca_capable;
#endif /* !FTDF_LITE */
    ftdf_boolean_t              metrics_capable;
#ifndef FTDF_LITE
    ftdf_boolean_t              ranging_supported;
#endif /* !FTDF_LITE */
    ftdf_boolean_t              keep_phy_enabled;
    ftdf_boolean_t              metrics_enabled;
#ifndef FTDF_LITE
    ftdf_boolean_t              beacon_auto_respond;
    ftdf_boolean_t              tsch_capable;
    ftdf_period_t               ts_sync_correct_threshold;
#endif /* !FTDF_LITE */
    ftdf_nr_backoff_periods_t   bo_irq_threshold;
    ftdf_pti_config_t           pti_config;
    ftdf_link_quality_mode_t    link_quality_mode;
} ftdf_pib_t;

typedef uint8_t ftdf_frame_version_t;
#define FTDF_FRAME_VERSION_2003          0
#define FTDF_FRAME_VERSION_2011          1
#define FTDF_FRAME_VERSION_E             2
#define FTDF_FRAME_VERSION_NOT_SUPPORTED 3

typedef struct {
    ftdf_frame_type_t       frame_type;
    ftdf_frame_version_t    frame_version;
    ftdf_bitmap8_t          options;
    ftdf_sn_t               sn;
    ftdf_address_mode_t     src_addr_mode;
    ftdf_pan_id_t           src_pan_id;
    ftdf_address_t          src_addr;
    ftdf_address_mode_t     dst_addr_mode;
    ftdf_pan_id_t           dst_pan_id;
    ftdf_address_t          dst_addr;
    ftdf_command_frame_id_t command_frame_id;
} ftdf_frame_header_t;

typedef struct {
    ftdf_security_level_t     security_level;
    ftdf_key_id_mode_t        key_id_mode;
    ftdf_key_index_t          key_index;
    ftdf_frame_counter_mode_t frame_counter_mode;
    ftdf_octet_t              *key_source;
    ftdf_frame_counter_t      frame_counter;
} ftdf_security_header;

typedef struct {
    ftdf_boolean_t fast_a;
    ftdf_boolean_t data_r;
} ftdf_assoc_admin_t;

typedef uint8_t ftdf_sn_sel;
#define FTDF_SN_SEL_DSN  0
#define FTDF_SN_SEL_BSN  1
#define FTDF_SN_SEL_EBSN 2

typedef struct {
    ftdf_address_mode_t addr_mode;
    ftdf_address_t      addr;
    ftdf_time_t         timestamp;
    ftdf_boolean_t      dsn_valid;
    ftdf_sn_t           dsn;
    ftdf_boolean_t      bsn_valid;
    ftdf_sn_t           bsn;
    ftdf_boolean_t      ebsn_valid;
    ftdf_sn_t           eb_sn;
} ftdf_rx_address_admin_t;

struct ftdf_buffer_;

typedef struct {
    struct ftdf_buffer_ *next;
    struct ftdf_buffer_ *prev;
} ftdf_buffer_header_t;

typedef struct ftdf_buffer_ {
    ftdf_buffer_header_t header;
    ftdf_msg_buffer_t    *request;
} ftdf_buffer_t;

typedef struct {
    ftdf_buffer_header_t head;
    ftdf_buffer_header_t tail;
} ftdf_queue_t;

typedef struct {
    ftdf_address_t      addr;
    ftdf_address_mode_t addr_mode;
    ftdf_pan_id_t       pan_id;
    ftdf_queue_t        queue;
} ftdf_pending_t;

typedef struct _ftdf_pending_tl_ {
    struct _ftdf_pending_tl_ *next;
    ftdf_boolean_t           free;
    ftdf_msg_buffer_t        *request;
    ftdf_time_t              delta;
    uint8_t                  pend_list_nr;
    void                     (* func)(struct _ftdf_pending_tl_*);
} ftdf_pending_tl_t;

typedef uint8_t ftdf_remote_id;
#define FTDF_REMOTE_DATA_REQUEST                 0
#define FTDF_REMOTE_PAN_ID_CONFLICT_NOTIFICATION 1
#define FTDF_REMOTE_BEACON                       2
#define FTDF_REMOTE_KEEP_ALIVE                   3

typedef struct {
    ftdf_msg_id_t        msg_id;
    ftdf_remote_id       remote_id;
    ftdf_short_address_t dst_addr;
} ftdf_remote_request_t;

#ifndef FTDF_NO_CSL
typedef struct {
    ftdf_short_address_t addr;
    ftdf_time_t          time;
    ftdf_period_t        period;
} ftdf_peer_csl_timing_t;
#endif /* FTDF_NO_CSL */

typedef uint8_t FTDF_State;
#define FTDF_STATE_UNINITIALIZED 0
#define FTDF_STATE_IDLE          1
#define FTDF_STATE_DATA_REQUEST  2
#define FTDF_STATE_POLL_REQUEST  3
#define FTDF_STATE_SCANNING      4

#ifndef FTDF_NO_TSCH
typedef struct {
    ftdf_short_address_t nodeAddr;
    ftdf_size_t          nr_of_retries;
    ftdf_size_t          nr_of_cca_retries;
    ftdf_be_exponent_t   BE;
} ftdf_tsch_retry_t;

typedef struct {
    ftdf_short_address_t     dst_addr;
    ftdf_keep_alive_period_t period;
    ftdf_remote_request_t    msg;
} FTDF_NeighborEntry;
#endif /* FTDF_NO_TSCH */

typedef struct {
    ftdf_count_t fcs_error_cnt;
    ftdf_count_t tx_std_ack_cnt;
    ftdf_count_t rx_std_ack_cnt;
} ftdf_lmac_counters_t;

extern ftdf_pib_t             ftdf_pib;
extern FTDF_State             *FTDF_state;
extern ftdf_boolean_t         ftdf_transparent_mode;
extern ftdf_bitmap32_t        ftdf_transparent_mode_options;
extern ftdf_queue_t           ftdf_req_queue;
extern ftdf_queue_t           ftdf_freeQueue;
extern ftdf_pending_t         ftdf_tx_pending_list[FTDF_NR_OF_REQ_BUFFERS];
extern ftdf_pending_tl_t      ftdf_tx_pending_timer_list[FTDF_NR_OF_REQ_BUFFERS];
extern ftdf_pending_tl_t      *ftdf_tx_pending_timer_head;
extern ftdf_time_t            ftdf_tx_pending_timer_lt;
extern ftdf_time_t            ftdf_tx_pending_timer_time;
extern ftdf_msg_buffer_t      *ftdf_req_current;
extern ftdf_size_t            ftdf_nr_of_retries;
extern ftdf_boolean_t         ftdf_is_pan_coordinator;
extern ftdf_slotframe_entry_t ftdf_slotframe_table[FTDF_MAX_SLOTFRAMES];
extern ftdf_link_entry_t      ftdf_link_table[FTDF_MAX_LINKS];
#ifndef FTDF_NO_TSCH
extern ftdf_link_entry_t      *ftdf_start_links[FTDF_MAX_SLOTFRAMES];
extern ftdf_link_entry_t      *ftdf_end_links[FTDF_MAX_SLOTFRAMES];
extern FTDF_NeighborEntry     ftdf_neighbor_table[FTDF_NR_OF_NEIGHBORS];
extern ftdf_boolean_t         ftdf_wake_up_enable_tsch;
#endif /* FTDF_NO_TSCH */
#ifndef FTDF_NO_CSL
extern ftdf_boolean_t         ftdf_wake_up_enable_le;
#endif /* FTDF_NO_CSL */
extern ftdf_lmac_counters_t   ftdf_lmac_counters;

extern ftdf_frame_header_t    ftdf_fh;
extern ftdf_security_header   ftdf_sh;
extern ftdf_assoc_admin_t     ftdf_aa;
extern ftdf_boolean_t         ftdf_tx_in_progress;
#ifndef FTDF_NO_CSL
extern ftdf_time_t            ftdf_rz_time;
extern ftdf_short_address_t   ftdf_send_frame_pending;
#endif /* FTDF_NO_CSL */
extern ftdf_time_t            ftdf_start_csl_sample_time;

#ifndef FTDF_NO_TSCH
extern ftdf_link_entry_t      *ftdf_tsch_slot_link;
extern ftdf_time64_t          ftdf_tsch_slot_time;
extern ftdf_asn_t             ftdf_tsch_slot_asn;
#endif /* FTDF_NO_TSCH */

void ftdf_process_data_request(ftdf_data_request_t *req);
void ftdf_process_purge_request(ftdf_purge_request_t *req);
void ftdf_process_associate_request(ftdf_associate_request_t *req);
void ftdf_process_associate_response(ftdf_associate_response_t *req);
void ftdf_process_disassociate_request(ftdf_disassociate_request_t *req);
void ftdf_process_get_request(ftdf_get_request_t *req);
void ftdf_process_set_request(ftdf_set_request_t *req);
void ftdf_process_orphan_response(ftdf_orphan_response_t *req);
void ftdf_process_reset_request(ftdf_reset_request_t *req);
void ftdf_process_rx_enable_request(ftdf_rx_enable_request_t *req);
void ftdf_process_scan_request(ftdf_scan_request_t *req);
void ftdf_process_start_request(ftdf_start_request_t *req);
void ftdf_process_poll_request(ftdf_poll_request_t *req);
#ifndef FTDF_NO_TSCH
void ftdf_process_set_slotframe_request(ftdf_set_slotframe_request_t *req);
void ftdf_process_set_link_request(ftdf_set_link_request_t *req);
void ftdf_process_tsch_mode_request(ftdf_tsch_mode_request_t *req);
void ftdf_process_keep_alive_request(ftdf_keep_alive_request_t *req);
#endif /* FTDF_NO_TSCH */
void ftdf_process_beacon_request(ftdf_beacon_request_t *req);
void ftdf_process_transparent_request(ftdf_transparent_request_t *req);
void ftdf_process_remote_request(ftdf_remote_request_t *req);

void ftdf_process_next_request(void);
void ftdf_process_rx_event(void);
void ftdf_process_tx_event(void);
void ftdf_process_symbol_timer_event(void);

void ftdf_send_data_confirm(ftdf_data_request_t    *data_request,
                            ftdf_status_t          status,
                            ftdf_time_t            timestamp,
                            ftdf_sn_t              dsn,
                            ftdf_num_of_backoffs_t num_of_backoffs,
                            ftdf_ie_list_t         *ack_payload);

void ftdf_send_data_indication(ftdf_frame_header_t  *frame_header,
                               ftdf_security_header *security_header,
                               ftdf_ie_list_t       *payload_ie_list,
                               ftdf_data_length_t   msdu_length,
                               ftdf_octet_t         *msdu,
                               ftdf_link_quality_t  mpdu_link_quality,
                               ftdf_time_t          timestamp);

void ftdf_send_poll_confirm(ftdf_poll_request_t *poll_request, ftdf_status_t status);

void ftdf_send_scan_confirm(ftdf_scan_request_t *scan_request, ftdf_status_t status);

void ftdf_send_beacon_notify_indication(ftdf_pan_descriptor_t *pan_descriptor);

void ftdf_send_beacon_confirm(ftdf_beacon_request_t *beacon_request, ftdf_status_t status);

void ftdf_send_associate_confirm(ftdf_associate_request_t  *associate_request,
                                 ftdf_status_t             status,
                                 ftdf_short_address_t      assoc_short_addr);

void ftdf_send_associate_data_request(void);

void ftdf_send_disassociate_confirm(ftdf_disassociate_request_t *dis_req, ftdf_status_t status);

void ftdf_send_sync_loss_indication(ftdf_loss_reason_t loss_reason, ftdf_security_header *security_header);

void ftdf_sendpan_id_conflict_notification(ftdf_frame_header_t *frame_header, ftdf_security_header *security_header);

void ftdf_send_comm_status_indication(ftdf_msg_buffer_t     *request,
                                      ftdf_status_t         status,
                                      ftdf_pan_id_t         pan_Id,
                                      ftdf_address_mode_t   src_addr_mode,
                                      ftdf_address_t        src_addr,
                                      ftdf_address_mode_t   dst_addr_mode,
                                      ftdf_address_t        dst_addr,
                                      ftdf_security_level_t security_level,
                                      ftdf_key_id_mode_t    key_id_mode,
                                      ftdf_octet_t          *key_source,
                                      ftdf_key_index_t      key_index);

void ftdf_start_timer(ftdf_period_t period, void (* timer_func)(void *params), void *params);

ftdf_octet_t *ftdf_get_tx_buf_ptr(ftdf_buffer_t *tx_buffer);

void ftdf_set_tx_meta_data(ftdf_buffer_t         *tx_buffer,
                           ftdf_data_length_t    frame_length,
                           ftdf_channel_number_t channel,
                           ftdf_frame_type_t     frame_type,
                           ftdf_boolean_t        ack_tx,
                           ftdf_sn_t             sn);

ftdf_status_t ftdf_send_frame(ftdf_channel_number_t channel,
                              ftdf_frame_header_t   *frame_header,
                              ftdf_security_header  *security_header,
                              ftdf_octet_t          *tx_ptr,
                              ftdf_data_length_t    payload_size,
                              ftdf_octet_t          *payload);

#if !defined(FTDF_NO_CSL) || !defined(FTDF_NO_TSCH)
ftdf_status_t ftdf_send_ack_frame(ftdf_frame_header_t  *frame_header,
                                  ftdf_security_header *security_header,
                                  ftdf_octet_t         *tx_ptr);
#endif /* !FTDF_NO_CSL || !FTDF_NO_TSCH */

void ftdf_send_transparent_frame(ftdf_data_length_t    frame_length,
                                 ftdf_octet_t          *frame,
                                 ftdf_channel_number_t channel,
                                 ftdf_pti_t            pti,
                                 ftdf_boolean_t        cmsa_suppress);

ftdf_octet_t *ftdf_get_rx_buffer(void);

void ftdf_process_rx_event(void);

ftdf_octet_t *ftdf_add_frame_header(ftdf_octet_t        *tx_ptr,
                                    ftdf_frame_header_t *frame_header,
                                    ftdf_data_length_t  msdu_length);

ftdf_octet_t *ftdf_get_frame_header(ftdf_octet_t *rx_ptr, ftdf_frame_header_t *frame_header);

ftdf_data_length_t ftdf_get_mic_length(ftdf_security_level_t security_level);

ftdf_octet_t *ftdf_get_security_header(ftdf_octet_t         *rx_ptr,
                                       uint8_t              frame_version,
                                       ftdf_security_header *security_header);

ftdf_octet_t *ftdf_add_security_header(ftdf_octet_t         *tx_ptr,
                                       ftdf_security_header *security_header);

ftdf_status_t ftdf_secure_frame(ftdf_octet_t         *buf_ptr,
                                ftdf_octet_t         *priv_ptr,
                                ftdf_frame_header_t  *frame_header,
                                ftdf_security_header *security_header);

ftdf_status_t ftdf_unsecure_frame(ftdf_octet_t         *buf_ptr,
                                  ftdf_octet_t         *priv_ptr,
                                  ftdf_frame_header_t  *frame_header,
                                  ftdf_security_header *security_header);

ftdf_key_descriptor_t *ftdf_lookup_key(ftdf_address_mode_t dev_addr_mode,
                                       ftdf_pan_id_t       dev_pan_id,
                                       ftdf_address_t      dev_addr,
                                       ftdf_frame_type_t   frame_type,
                                       ftdf_key_id_mode_t  key_id_mode,
                                       ftdf_key_index_t    key_index,
                                       ftdf_octet_t        *key_source);

ftdf_device_descriptor_t *ftdf_lookup_device(ftdf_size_t                     nr_of_device_descriptor_handles,
                                             ftdf_device_descriptor_handle_t *device_descriptor_handles,
                                             ftdf_address_mode_t             dev_addr_mode,
                                             ftdf_pan_id_t                   dev_pan_id,
                                             ftdf_address_t                  dev_addr);

ftdf_security_level_descriptor_t *ftdf_get_security_level_descr(ftdf_frame_type_t       frame_type,
                                                                ftdf_command_frame_id_t command_frame_id);

#if !defined(FTDF_NO_CSL) || !defined(FTDF_NO_TSCH)
ftdf_octet_t *ftdf_add_ies(ftdf_octet_t   *tx_ptr,
                           ftdf_ie_list_t *header_ie_list,
                           ftdf_ie_list_t *payload_ie_list,
                           ftdf_boolean_t with_termination_ie);

ftdf_octet_t *ftdf_get_ies(ftdf_octet_t   *rx_ptr,
                           ftdf_octet_t   *frame_end_ptr,
                           ftdf_ie_list_t **header_ie_list,
                           ftdf_ie_list_t **payload_ie_list);
#endif /* !FTDF_NO_CSL || !FTDF_NO_TSCH */

#ifndef FTDF_NO_CSL
void ftdf_set_peer_csl_timing(ftdf_ie_list_t *header_ie_list, ftdf_time_t    timeStamp);
void ftdf_set_csl_sample_time(void);
void ftdf_get_wakeup_params(ftdf_short_address_t dst_addr,
                            ftdf_time_t          *wakeup_start_time,
                            ftdf_period_t        *wakeup_period);
#endif /* FTDF_NO_CSL */

void ftdf_get_ext_address(void);
void ftdf_set_ext_address(void);
void ftdf_get_ack_wait_duration(void);
void ftdf_get_enh_ack_wait_duration(void);
void ftdf_set_enh_ack_wait_duration(void);
void ftdf_get_implicit_broadcast(void);
void ftdf_set_implicit_broadcast(void);
void ftdf_set_short_address(void);
void ftdf_set_simple_address(void);
void ftdf_get_rx_on_when_idle(void);
void ftdf_set_rx_on_when_idle(void);
void ftdf_getpan_id(void);
void ftdf_setpan_id(void);
void ftdf_get_current_channel(void);
void ftdf_set_current_channel(void);
void            FTDF_setTXPower( void );
void ftdf_get_max_frame_total_wait_time(void);
void ftdf_set_max_frame_total_wait_time(void);
#ifndef FTDF_NO_CSL
void ftdf_set_le_enabled(void);
void ftdf_get_csl_frame_pending_wait(void);
void ftdf_set_csl_frame_pending_wait(void);
#endif /* FTDF_NO_CSL */
void ftdf_get_lmac_pm_data(void);
void ftdf_get_lmac_traffic_counters(void);
void ftdf_set_max_csma_backoffs(void);
void ftdf_set_max_be(void);
void ftdf_set_min_be(void);
void ftdf_get_keep_phy_enabled(void);
void ftdf_set_keep_phy_enabled(void);
void ftdf_set_bo_irq_threshold(void);
void ftdf_get_bo_irq_threshold(void);
void ftdf_set_pti_config(void);

#ifndef FTDF_NO_TSCH
void ftdf_set_timeslot_template(void);
#endif /* FTDF_NO_TSCH */

void ftdf_init_lmac(void);
void ftdf_init_queues(void);
void ftdf_init_queue(ftdf_queue_t *queue);
void ftdf_queue_buffer_head(ftdf_buffer_t *buffer, ftdf_queue_t *queue);
ftdf_status_t ftdf_queue_req_head(ftdf_msg_buffer_t *request, ftdf_queue_t *queue);
ftdf_buffer_t *ftdf_dequeue_buffer_tail(ftdf_queue_t *queue);
ftdf_msg_buffer_t *ftdf_dequeue_req_tail(ftdf_queue_t *queue);
ftdf_msg_buffer_t *ftdf_dequeue_by_handle(ftdf_handle_t handle, ftdf_queue_t *queue);
ftdf_buffer_t *ftdf_dequeue_by_buffer(ftdf_buffer_t *buffer, ftdf_queue_t *queue);
ftdf_boolean_t ftdf_is_queue_empty(ftdf_queue_t *queue);

void ftdf_add_tx_pending_timer(ftdf_msg_buffer_t *request,
                               uint8_t           pend_list_nr,
                               ftdf_time_t       delta,
                               void              (* func)(ftdf_pending_tl_t*));

void ftdf_remove_tx_pending_timer(ftdf_msg_buffer_t *request);
void ftdf_restore_tx_pending_timer(void);
ftdf_boolean_t ftdf_get_tx_pending_timer_head(ftdf_time_t *time);
void ftdf_send_transaction_expired(ftdf_pending_tl_t *ptr);
#ifndef FTDF_NO_TSCH
void ftdf_process_keep_alive_timer_exp(ftdf_pending_tl_t *ptr);
void ftdf_reset_keep_alive_timer(ftdf_short_address_t dst_addr);
#endif /* FTDF_NO_TSCH */

void ftdf_process_tx_pending(ftdf_frame_header_t  *frame_hader, ftdf_security_header *security_header);

void ftdf_process_command_frame(ftdf_octet_t         *rx_buffer,
                                ftdf_frame_header_t  *frame_header,
                                ftdf_security_header *security_header,
                                ftdf_ie_list_t       *payload_ie_list);

void ftdf_scan_ready(ftdf_scan_request_t*);
void ftdf_add_pan_descriptor(ftdf_pan_descriptor_t*);
void ftdf_send_beacon_request(ftdf_channel_number_t channel);
void ftdf_send_beacon_request_indication(ftdf_frame_header_t *frame_header, ftdf_ie_list_t *payload_ie_list);
void ftdf_send_orphan_notification(ftdf_channel_number_t channel);

#ifndef FTDF_NO_TSCH
void ftdf_set_tsch_enabled(void);
ftdf_status_t ftdf_schedule_tsch(ftdf_msg_buffer_t *request);
void ftdf_tsch_process_request(void);
ftdf_size_t ftdf_get_tsch_sync_sub_ie(void);
ftdf_octet_t *ftdf_add_tsch_sync_sub_ie(ftdf_octet_t *tx_ptr);
ftdf_short_address_t ftdf_get_request_address(ftdf_msg_buffer_t *request);
ftdf_msg_buffer_t *ftdf_tsch_get_pending(ftdf_msg_buffer_t *request);
ftdf_octet_t *ftdf_add_corr_time_ie(ftdf_octet_t *tx_ptr, ftdf_time_t rx_timestamp);
void ftdf_correct_slot_time(ftdf_time_t rx_timestamp);
void ftdf_correct_slot_time_from_ack(ftdf_ie_list_t *header_ie_list);
void ftdf_init_tsch_retries(void);
ftdf_tsch_retry_t *ftdf_get_tsch_retry(ftdf_short_address_t node_addr);
ftdf_num_of_backoffs_t ftdf_get_num_of_backoffs(ftdf_be_exponent_t be);
void ftdf_init_backoff(void);
ftdf_sn_t ftdf_process_tsch_sn(ftdf_msg_buffer_t *msg, ftdf_sn_t sn, uint8_t *priv);
#endif /* FTDF_NO_TSCH */

ftdf_time64_t ftdf_get_cur_time64(void);
void ftdf_init_cur_time64(void);

#if FTDF_USE_SLEEP_DURING_BACKOFF
typedef enum
{
        FTDF_SDB_STATE_INIT = 0,
        FTDF_SDB_STATE_BACKING_OFF,
        FTDF_SDB_STATE_WAITING_WAKE_UP_IRQ,
        FTDF_SDB_STATE_RESUMING,
} ftdf_sdb_state_t;

typedef struct
{
        ftdf_sdb_state_t state;
        ftdf_size_t nr_of_backoffs;
        ftdf_time_t cca_retry_time;
        ftdf_octet_t buffer[FTDF_BUFFER_LENGTH];
        uint32_t metadata_0;
        uint32_t metadata_1;
        uint32_t phy_csma_ca_attr;
} ftdf_sdb_t;

void ftdf_sdb_fsm_reset(void);

void ftdf_sdb_fsm_backoff_irq(void);

void ftdf_sdb_fsm_sleep(void);

void ftdf_sdb_fsm_abort_sleep(void);

void ftdf_sdb_fsm_wake_up_irq(void);

void ftdf_sdb_fsm_wake_up(void);

void ftdf_sdb_fsm_tx_irq(void);

ftdf_usec_t ftdf_sdb_get_sleep_time(void);

#endif /* FTDF_USE_SLEEP_DURING_BACKOFF */

static inline void ftdf_set_link_quality_mode(void)
{
#ifdef FTDF_PIB_LINK_QUALITY_MODE
        REG_SETF(FTDF, FTDF_LMAC_CONTROL_1_REG, PHYRXATTR_DEM_PTI,
                (ftdf_pib.link_quality_mode == FTDF_LINK_QUALITY_MODE_RSSI) ? 0x8 : 0);

        REG_SETF(DEM, RF_FTDF_CTRL4_REG, LQI_SCALE, 0);

        if (ftdf_pib.link_quality_mode == FTDF_LINK_QUALITY_MODE_RSSI) {
                REG_SETF(DEM, RF_FTDF_CTRL4_REG, LQI_PREAMBLE_EN, 0);
                REG_SETF(DEM, RF_FTDF_CTRL4_REG, LQI_CORRTH_EN, 1);
        } else {
                REG_SETF(DEM, RF_FTDF_CTRL4_REG, LQI_PREAMBLE_EN, 1);
                REG_SETF(DEM, RF_FTDF_CTRL4_REG, LQI_CORRTH_EN, 0);
        }

#endif
}

#if dg_configUSE_FTDF_DDPHY == 1

void ftdf_ddphy_restore(void);

void ftdf_ddphy_save(void);

#endif
