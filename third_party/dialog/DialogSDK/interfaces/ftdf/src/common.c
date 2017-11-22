/**
 ****************************************************************************************
 *
 * @file common.c
 *
 * @brief Common FTDF functions
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

#include <string.h>
#include <stdlib.h>

#include <ftdf.h>
#include "internal.h"
#include "sdk_defs.h"

#ifdef CONFIG_USE_FTDF
#if dg_configCOEX_ENABLE_CONFIG
#include "hw_coex.h"
#endif
typedef struct {
        void *addr;
        uint8_t size;
        void (*getFunc)(void);
        void (*setFunc)(void);
} pib_attribute_def_t;

typedef struct {
        pib_attribute_def_t attribute_defs[FTDF_NR_OF_PIB_ATTRIBUTES + 1];
} pib_attribute_table_h;

ftdf_pib_t ftdf_pib __attribute__ ((section(".retention")));

#if FTDF_FP_BIT_MODE == FTDF_FP_BIT_MODE_AUTO
#if FTDF_FPPR_DEFER_INVALIDATION
static struct {
        ftdf_address_mode_t addr_mode;
        ftdf_pan_id_t       pan_id;
        ftdf_address_t      addr;
} ftdf_fppr_pending __attribute__ ((section(".retention")));
#endif /* FTDF_FPPR_DEFER_INVALIDATION */
#endif /* FTDF_FP_BIT_MODE == FTDF_FP_BIT_MODE_AUTO */

#ifndef FTDF_LITE
const ftdf_channel_number_t page_0_channels[FTDF_NR_OF_CHANNELS] = {
        11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26 };
const ftdf_channel_descriptor_t channel_descriptors[1] = {
        { 0, 16, (ftdf_channel_number_t*)page_0_channels } };
const ftdf_channel_descriptor_list_t channels_supported = {
        1, (ftdf_channel_descriptor_t*)channel_descriptors };
#endif

const pib_attribute_table_h pib_attribute_table = {
        .attribute_defs[FTDF_PIB_EXTENDED_ADDRESS].addr = &ftdf_pib.ext_address,
        .attribute_defs[FTDF_PIB_EXTENDED_ADDRESS].size = sizeof(ftdf_pib.ext_address),
        .attribute_defs[FTDF_PIB_EXTENDED_ADDRESS].getFunc = ftdf_get_ext_address,
        .attribute_defs[FTDF_PIB_EXTENDED_ADDRESS].setFunc = ftdf_set_ext_address,
        .attribute_defs[FTDF_PIB_ACK_WAIT_DURATION].addr = &ftdf_pib.ack_wait_duration,
        .attribute_defs[FTDF_PIB_ACK_WAIT_DURATION].size = 0,
        .attribute_defs[FTDF_PIB_ACK_WAIT_DURATION].getFunc = ftdf_get_ack_wait_duration,
        .attribute_defs[FTDF_PIB_ACK_WAIT_DURATION].setFunc = NULL,
#ifndef FTDF_LITE
        .attribute_defs[FTDF_PIB_ASSOCIATION_PAN_COORD].addr = &ftdf_pib.associated_pan_coord,
        .attribute_defs[FTDF_PIB_ASSOCIATION_PAN_COORD].size =  sizeof(ftdf_pib.associated_pan_coord),
        .attribute_defs[FTDF_PIB_ASSOCIATION_PAN_COORD].getFunc = NULL,
        .attribute_defs[FTDF_PIB_ASSOCIATION_PAN_COORD].setFunc = NULL,
        .attribute_defs[FTDF_PIB_ASSOCIATION_PERMIT].addr = &ftdf_pib.association_permit,
        .attribute_defs[FTDF_PIB_ASSOCIATION_PERMIT].size = sizeof(ftdf_pib.association_permit),
        .attribute_defs[FTDF_PIB_ASSOCIATION_PERMIT].getFunc = NULL,
        .attribute_defs[FTDF_PIB_ASSOCIATION_PERMIT].setFunc = NULL,
        .attribute_defs[FTDF_PIB_AUTO_REQUEST].addr = &ftdf_pib.auto_request,
        .attribute_defs[FTDF_PIB_AUTO_REQUEST].size = sizeof(ftdf_pib.auto_request),
        .attribute_defs[FTDF_PIB_AUTO_REQUEST].getFunc = NULL,
        .attribute_defs[FTDF_PIB_AUTO_REQUEST].setFunc = NULL,
        .attribute_defs[FTDF_PIB_BATT_LIFE_EXT].addr = &ftdf_pib.batt_life_ext,
        .attribute_defs[FTDF_PIB_BATT_LIFE_EXT].size = sizeof(ftdf_pib.batt_life_ext),
        .attribute_defs[FTDF_PIB_BATT_LIFE_EXT].getFunc = NULL,
        .attribute_defs[FTDF_PIB_BATT_LIFE_EXT].setFunc = NULL,
        .attribute_defs[FTDF_PIB_BATT_LIFE_EXT_PERIODS].addr = &ftdf_pib.batt_life_ext_periods,
        .attribute_defs[FTDF_PIB_BATT_LIFE_EXT_PERIODS].size = sizeof(ftdf_pib.batt_life_ext_periods),
        .attribute_defs[FTDF_PIB_BATT_LIFE_EXT_PERIODS].getFunc = NULL,
        .attribute_defs[FTDF_PIB_BATT_LIFE_EXT_PERIODS].setFunc = NULL,
        .attribute_defs[FTDF_PIB_BEACON_PAYLOAD].addr = &ftdf_pib.beacon_payload,
        .attribute_defs[FTDF_PIB_BEACON_PAYLOAD].size = sizeof(ftdf_pib.beacon_payload),
        .attribute_defs[FTDF_PIB_BEACON_PAYLOAD].getFunc = NULL,
        .attribute_defs[FTDF_PIB_BEACON_PAYLOAD].setFunc = NULL,
        .attribute_defs[FTDF_PIB_BEACON_PAYLOAD_LENGTH].addr = &ftdf_pib.beacon_payload_length,
        .attribute_defs[FTDF_PIB_BEACON_PAYLOAD_LENGTH].size = sizeof(ftdf_pib.beacon_payload_length),
        .attribute_defs[FTDF_PIB_BEACON_PAYLOAD_LENGTH].getFunc = NULL,
        .attribute_defs[FTDF_PIB_BEACON_PAYLOAD_LENGTH].setFunc = NULL,
        .attribute_defs[FTDF_PIB_BEACON_ORDER].addr = &ftdf_pib.beacon_order,
        .attribute_defs[FTDF_PIB_BEACON_ORDER].size = 0,
        .attribute_defs[FTDF_PIB_BEACON_ORDER].getFunc = NULL,
        .attribute_defs[FTDF_PIB_BEACON_ORDER].setFunc = NULL,
        .attribute_defs[FTDF_PIB_BEACON_TX_TIME].addr = &ftdf_pib.beacon_tx_time,
        .attribute_defs[FTDF_PIB_BEACON_TX_TIME].size = sizeof(ftdf_pib.beacon_tx_time),
        .attribute_defs[FTDF_PIB_BEACON_TX_TIME].getFunc = NULL,
        .attribute_defs[FTDF_PIB_BEACON_TX_TIME].setFunc = NULL,
        .attribute_defs[FTDF_PIB_BSN].addr = &ftdf_pib.bsn,
        .attribute_defs[FTDF_PIB_BSN].size = sizeof(ftdf_pib.bsn),
        .attribute_defs[FTDF_PIB_BSN].getFunc = NULL,
        .attribute_defs[FTDF_PIB_BSN].setFunc = NULL,
        .attribute_defs[FTDF_PIB_COORD_EXTENDED_ADDRESS].addr = &ftdf_pib.coord_ext_address,
        .attribute_defs[FTDF_PIB_COORD_EXTENDED_ADDRESS].size = sizeof(ftdf_pib.coord_ext_address),
        .attribute_defs[FTDF_PIB_COORD_EXTENDED_ADDRESS].getFunc = NULL,
        .attribute_defs[FTDF_PIB_COORD_EXTENDED_ADDRESS].setFunc = NULL,
        .attribute_defs[FTDF_PIB_COORD_SHORT_ADDRESS].addr = &ftdf_pib.coord_short_address,
        .attribute_defs[FTDF_PIB_COORD_SHORT_ADDRESS].size = sizeof(ftdf_pib.coord_short_address),
        .attribute_defs[FTDF_PIB_COORD_SHORT_ADDRESS].getFunc = NULL,
        .attribute_defs[FTDF_PIB_COORD_SHORT_ADDRESS].setFunc = NULL,
        .attribute_defs[FTDF_PIB_DSN].addr = &ftdf_pib.dsn,
        .attribute_defs[FTDF_PIB_DSN].size = sizeof(ftdf_pib.dsn),
        .attribute_defs[FTDF_PIB_DSN].getFunc = NULL,
        .attribute_defs[FTDF_PIB_DSN].setFunc = NULL,
        .attribute_defs[FTDF_PIB_GTS_PERMIT].addr = &ftdf_pib.gts_permit,
        .attribute_defs[FTDF_PIB_GTS_PERMIT].size = 0,
        .attribute_defs[FTDF_PIB_GTS_PERMIT].getFunc = NULL,
        .attribute_defs[FTDF_PIB_GTS_PERMIT].setFunc = NULL,
#endif /* !FTDF_LITE */
        .attribute_defs[FTDF_PIB_MAX_BE].addr = &ftdf_pib.max_be,
        .attribute_defs[FTDF_PIB_MAX_BE].size = sizeof(ftdf_pib.max_be),
        .attribute_defs[FTDF_PIB_MAX_BE].getFunc = NULL,
        .attribute_defs[FTDF_PIB_MAX_BE].setFunc = ftdf_set_max_be,
        .attribute_defs[FTDF_PIB_MAX_CSMA_BACKOFFS].addr = &ftdf_pib.max_csma_backoffs,
        .attribute_defs[FTDF_PIB_MAX_CSMA_BACKOFFS].size = sizeof(ftdf_pib.max_csma_backoffs),
        .attribute_defs[FTDF_PIB_MAX_CSMA_BACKOFFS].getFunc = NULL,
        .attribute_defs[FTDF_PIB_MAX_CSMA_BACKOFFS].setFunc = ftdf_set_max_csma_backoffs,
        .attribute_defs[FTDF_PIB_MAX_FRAME_TOTAL_WAIT_TIME].addr = &ftdf_pib.max_frame_total_wait_time,
        .attribute_defs[FTDF_PIB_MAX_FRAME_TOTAL_WAIT_TIME].size = sizeof(ftdf_pib.max_frame_total_wait_time),
        .attribute_defs[FTDF_PIB_MAX_FRAME_TOTAL_WAIT_TIME].getFunc = ftdf_get_max_frame_total_wait_time,
        .attribute_defs[FTDF_PIB_MAX_FRAME_TOTAL_WAIT_TIME].setFunc = ftdf_set_max_frame_total_wait_time,
        .attribute_defs[FTDF_PIB_MAX_FRAME_RETRIES].addr = &ftdf_pib.max_frame_retries,
        .attribute_defs[FTDF_PIB_MAX_FRAME_RETRIES].size =
            sizeof(ftdf_pib.max_frame_retries),
        .attribute_defs[FTDF_PIB_MAX_FRAME_RETRIES].getFunc = NULL,
        .attribute_defs[FTDF_PIB_MAX_FRAME_RETRIES].setFunc = NULL,
        .attribute_defs[FTDF_PIB_MIN_BE].addr = &ftdf_pib.min_be,
        .attribute_defs[FTDF_PIB_MIN_BE].size = sizeof(ftdf_pib.min_be),
        .attribute_defs[FTDF_PIB_MIN_BE].getFunc = NULL,
        .attribute_defs[FTDF_PIB_MIN_BE].setFunc = ftdf_set_min_be,
#ifndef FTDF_LITE
        .attribute_defs[FTDF_PIB_LIFS_PERIOD].addr = &ftdf_pib.lifs_period,
        .attribute_defs[FTDF_PIB_LIFS_PERIOD].size = 0,
        .attribute_defs[FTDF_PIB_LIFS_PERIOD].getFunc = NULL,
        .attribute_defs[FTDF_PIB_LIFS_PERIOD].setFunc = NULL,
        .attribute_defs[FTDF_PIB_SIFS_PERIOD].addr = &ftdf_pib.sifs_period,
        .attribute_defs[FTDF_PIB_SIFS_PERIOD].size = 0,
        .attribute_defs[FTDF_PIB_SIFS_PERIOD].getFunc = NULL,
        .attribute_defs[FTDF_PIB_SIFS_PERIOD].setFunc = NULL,
#endif
        .attribute_defs[FTDF_PIB_PAN_ID].addr = &ftdf_pib.pan_id,
        .attribute_defs[FTDF_PIB_PAN_ID].size = sizeof(ftdf_pib.pan_id),
        .attribute_defs[FTDF_PIB_PAN_ID].getFunc = ftdf_getpan_id,
        .attribute_defs[FTDF_PIB_PAN_ID].setFunc = ftdf_setpan_id,
#ifndef FTDF_LITE
        .attribute_defs[FTDF_PIB_PROMISCUOUS_MODE].addr = &ftdf_pib.promiscuous_mode,
        .attribute_defs[FTDF_PIB_PROMISCUOUS_MODE].size = sizeof(ftdf_pib.promiscuous_mode),
        .attribute_defs[FTDF_PIB_PROMISCUOUS_MODE].getFunc = NULL,
        .attribute_defs[FTDF_PIB_PROMISCUOUS_MODE].setFunc = NULL,
        .attribute_defs[FTDF_PIB_RESPONSE_WAIT_TIME].addr = &ftdf_pib.response_wait_time,
        .attribute_defs[FTDF_PIB_RESPONSE_WAIT_TIME].size = sizeof(ftdf_pib.response_wait_time),
        .attribute_defs[FTDF_PIB_RESPONSE_WAIT_TIME].getFunc = NULL,
        .attribute_defs[FTDF_PIB_RESPONSE_WAIT_TIME].setFunc = NULL,
#endif /* !FTDF_LITE */
        .attribute_defs[FTDF_PIB_RX_ON_WHEN_IDLE].addr = &ftdf_pib.rx_on_when_idle,
        .attribute_defs[FTDF_PIB_RX_ON_WHEN_IDLE].size = sizeof(ftdf_pib.rx_on_when_idle),
        .attribute_defs[FTDF_PIB_RX_ON_WHEN_IDLE].getFunc = ftdf_get_rx_on_when_idle,
        .attribute_defs[FTDF_PIB_RX_ON_WHEN_IDLE].setFunc = ftdf_set_rx_on_when_idle,
#ifndef FTDF_LITE
        .attribute_defs[FTDF_PIB_SECURITY_ENABLED].addr = &ftdf_pib.security_enabled,
        .attribute_defs[FTDF_PIB_SECURITY_ENABLED].size = sizeof(ftdf_pib.security_enabled),
        .attribute_defs[FTDF_PIB_SECURITY_ENABLED].getFunc = NULL,
        .attribute_defs[FTDF_PIB_SECURITY_ENABLED].setFunc = NULL,
#endif /* !FTDF_LITE */
        .attribute_defs[FTDF_PIB_SHORT_ADDRESS].addr = &ftdf_pib.short_address,
        .attribute_defs[FTDF_PIB_SHORT_ADDRESS].size = sizeof(ftdf_pib.short_address),
        .attribute_defs[FTDF_PIB_SHORT_ADDRESS].getFunc = NULL,
        .attribute_defs[FTDF_PIB_SHORT_ADDRESS].setFunc = ftdf_set_short_address,
#ifndef FTDF_LITE
        .attribute_defs[FTDF_PIB_SUPERFRAME_ORDER].addr = &ftdf_pib.superframe_order,
        .attribute_defs[FTDF_PIB_SUPERFRAME_ORDER].size = 0,
        .attribute_defs[FTDF_PIB_SUPERFRAME_ORDER].getFunc = NULL,
        .attribute_defs[FTDF_PIB_SUPERFRAME_ORDER].setFunc = NULL,
        .attribute_defs[FTDF_PIB_SYNC_SYMBOL_OFFSET].addr = &ftdf_pib.sync_symbol_offset,
        .attribute_defs[FTDF_PIB_SYNC_SYMBOL_OFFSET].size = 0,
        .attribute_defs[FTDF_PIB_SYNC_SYMBOL_OFFSET].getFunc = NULL,
        .attribute_defs[FTDF_PIB_SYNC_SYMBOL_OFFSET].setFunc = NULL,
        .attribute_defs[FTDF_PIB_TIMESTAMP_SUPPORTED].addr = &ftdf_pib.timestamp_supported,
        .attribute_defs[FTDF_PIB_TIMESTAMP_SUPPORTED].size = 0,
        .attribute_defs[FTDF_PIB_TIMESTAMP_SUPPORTED].getFunc = NULL,
        .attribute_defs[FTDF_PIB_TIMESTAMP_SUPPORTED].setFunc = NULL,
        .attribute_defs[FTDF_PIB_TRANSACTION_PERSISTENCE_TIME].addr = &ftdf_pib.transaction_persistence_time,
        .attribute_defs[FTDF_PIB_TRANSACTION_PERSISTENCE_TIME].size = sizeof(ftdf_pib.transaction_persistence_time),
        .attribute_defs[FTDF_PIB_TRANSACTION_PERSISTENCE_TIME].getFunc = NULL,
        .attribute_defs[FTDF_PIB_TRANSACTION_PERSISTENCE_TIME].setFunc = NULL,
        .attribute_defs[FTDF_PIB_ENH_ACK_WAIT_DURATION].addr = &ftdf_pib.enh_ack_wait_duration,
        .attribute_defs[FTDF_PIB_ENH_ACK_WAIT_DURATION].size = sizeof(ftdf_pib.enh_ack_wait_duration),
        .attribute_defs[FTDF_PIB_ENH_ACK_WAIT_DURATION].getFunc = ftdf_get_enh_ack_wait_duration,
        .attribute_defs[FTDF_PIB_ENH_ACK_WAIT_DURATION].setFunc = ftdf_set_enh_ack_wait_duration,
        .attribute_defs[FTDF_PIB_IMPLICIT_BROADCAST].addr = &ftdf_pib.implicit_broadcast,
        .attribute_defs[FTDF_PIB_IMPLICIT_BROADCAST].size = sizeof(ftdf_pib.implicit_broadcast),
        .attribute_defs[FTDF_PIB_IMPLICIT_BROADCAST].getFunc = ftdf_get_implicit_broadcast,
        .attribute_defs[FTDF_PIB_IMPLICIT_BROADCAST].setFunc = ftdf_set_implicit_broadcast,
        .attribute_defs[FTDF_PIB_SIMPLE_ADDRESS].addr = &ftdf_pib.simple_address,
        .attribute_defs[FTDF_PIB_SIMPLE_ADDRESS].size = sizeof(ftdf_pib.simple_address),
        .attribute_defs[FTDF_PIB_SIMPLE_ADDRESS].getFunc = NULL,
        .attribute_defs[FTDF_PIB_SIMPLE_ADDRESS].setFunc = ftdf_set_simple_address,
        .attribute_defs[FTDF_PIB_DISCONNECT_TIME].addr = &ftdf_pib.disconnect_time,
        .attribute_defs[FTDF_PIB_DISCONNECT_TIME].size = sizeof(ftdf_pib.disconnect_time),
        .attribute_defs[FTDF_PIB_DISCONNECT_TIME].getFunc = NULL,
        .attribute_defs[FTDF_PIB_DISCONNECT_TIME].setFunc = NULL,
        .attribute_defs[FTDF_PIB_JOIN_PRIORITY].addr = &ftdf_pib.join_priority,
        .attribute_defs[FTDF_PIB_JOIN_PRIORITY].size = sizeof(ftdf_pib.join_priority),
        .attribute_defs[FTDF_PIB_JOIN_PRIORITY].getFunc = NULL,
        .attribute_defs[FTDF_PIB_JOIN_PRIORITY].setFunc = NULL,
        .attribute_defs[FTDF_PIB_ASN].addr = &ftdf_pib.asn,
        .attribute_defs[FTDF_PIB_ASN].size = sizeof(ftdf_pib.asn),
        .attribute_defs[FTDF_PIB_ASN].getFunc = NULL,
        .attribute_defs[FTDF_PIB_ASN].setFunc = NULL,
        .attribute_defs[FTDF_PIB_NO_HL_BUFFERS].addr = &ftdf_pib.no_hl_buffers,
        .attribute_defs[FTDF_PIB_NO_HL_BUFFERS].size = sizeof(ftdf_pib.no_hl_buffers),
        .attribute_defs[FTDF_PIB_NO_HL_BUFFERS].getFunc = NULL,
        .attribute_defs[FTDF_PIB_NO_HL_BUFFERS].setFunc = NULL,
        .attribute_defs[FTDF_PIB_SLOTFRAME_TABLE].addr = &ftdf_pib.slotframe_table,
        .attribute_defs[FTDF_PIB_SLOTFRAME_TABLE].size = 0,
        .attribute_defs[FTDF_PIB_SLOTFRAME_TABLE].getFunc = NULL,
        .attribute_defs[FTDF_PIB_SLOTFRAME_TABLE].setFunc = NULL,
        .attribute_defs[FTDF_PIB_LINK_TABLE].addr = &ftdf_pib.link_table,
        .attribute_defs[FTDF_PIB_LINK_TABLE].size = 0,
        .attribute_defs[FTDF_PIB_LINK_TABLE].getFunc = NULL,
        .attribute_defs[FTDF_PIB_LINK_TABLE].setFunc = NULL,
        .attribute_defs[FTDF_PIB_TIMESLOT_TEMPLATE].addr = &ftdf_pib.timeslot_template,
        .attribute_defs[FTDF_PIB_TIMESLOT_TEMPLATE].size = sizeof(ftdf_pib.timeslot_template),
        .attribute_defs[FTDF_PIB_TIMESLOT_TEMPLATE].getFunc = NULL,
#ifdef FTDF_NO_TSCH
        .attribute_defs[FTDF_PIB_TIMESLOT_TEMPLATE].setFunc = NULL,
#else
        .attribute_defs[FTDF_PIB_TIMESLOT_TEMPLATE].setFunc = ftdf_set_timeslot_template,
#endif /* FTDF_NO_TSCH */
        .attribute_defs[FTDF_PIB_HOPPINGSEQUENCE_ID].addr = &ftdf_pib.hopping_sequence_id,
        .attribute_defs[FTDF_PIB_HOPPINGSEQUENCE_ID].size = sizeof(ftdf_pib.hopping_sequence_id),
        .attribute_defs[FTDF_PIB_HOPPINGSEQUENCE_ID].getFunc = NULL,
        .attribute_defs[FTDF_PIB_HOPPINGSEQUENCE_ID].setFunc = NULL,
        .attribute_defs[FTDF_PIB_CHANNEL_PAGE].addr = &ftdf_pib.channel_page,
        .attribute_defs[FTDF_PIB_CHANNEL_PAGE].size = sizeof(ftdf_pib.channel_page),
        .attribute_defs[FTDF_PIB_CHANNEL_PAGE].getFunc = NULL,
        .attribute_defs[FTDF_PIB_CHANNEL_PAGE].setFunc = NULL,
        .attribute_defs[FTDF_PIB_NUMBER_OF_CHANNELS].addr = &ftdf_pib.number_of_channels,
        .attribute_defs[FTDF_PIB_NUMBER_OF_CHANNELS].size = sizeof(ftdf_pib.number_of_channels),
        .attribute_defs[FTDF_PIB_NUMBER_OF_CHANNELS].getFunc = NULL,
        .attribute_defs[FTDF_PIB_NUMBER_OF_CHANNELS].setFunc = NULL,
        .attribute_defs[FTDF_PIB_PHY_CONFIGURATION].addr = &ftdf_pib.phy_configuration,
        .attribute_defs[FTDF_PIB_PHY_CONFIGURATION].size = sizeof(ftdf_pib.phy_configuration),
        .attribute_defs[FTDF_PIB_PHY_CONFIGURATION].getFunc = NULL,
        .attribute_defs[FTDF_PIB_PHY_CONFIGURATION].setFunc = NULL,
        .attribute_defs[FTDF_PIB_EXTENTED_BITMAP].addr = &ftdf_pib.extended_bitmap,
        .attribute_defs[FTDF_PIB_EXTENTED_BITMAP].size = sizeof(ftdf_pib.extended_bitmap),
        .attribute_defs[FTDF_PIB_EXTENTED_BITMAP].getFunc = NULL,
        .attribute_defs[FTDF_PIB_EXTENTED_BITMAP].setFunc = NULL,
        .attribute_defs[FTDF_PIB_HOPPING_SEQUENCE_LENGTH].addr = &ftdf_pib.hopping_sequence_length,
        .attribute_defs[FTDF_PIB_HOPPING_SEQUENCE_LENGTH].size = sizeof(ftdf_pib.hopping_sequence_length),
        .attribute_defs[FTDF_PIB_HOPPING_SEQUENCE_LENGTH].getFunc = NULL,
        .attribute_defs[FTDF_PIB_HOPPING_SEQUENCE_LENGTH].setFunc = NULL,
        .attribute_defs[FTDF_PIB_HOPPING_SEQUENCE_LIST].addr = ftdf_pib.hopping_sequence_list,
        .attribute_defs[FTDF_PIB_HOPPING_SEQUENCE_LIST].size = sizeof(ftdf_pib.hopping_sequence_list),
        .attribute_defs[FTDF_PIB_HOPPING_SEQUENCE_LIST].getFunc = NULL,
        .attribute_defs[FTDF_PIB_HOPPING_SEQUENCE_LIST].setFunc = NULL,
        .attribute_defs[FTDF_PIB_CURRENT_HOP].addr = &ftdf_pib.current_hop,
        .attribute_defs[FTDF_PIB_CURRENT_HOP].size = sizeof(ftdf_pib.current_hop),
        .attribute_defs[FTDF_PIB_CURRENT_HOP].getFunc = NULL,
        .attribute_defs[FTDF_PIB_CURRENT_HOP].setFunc = NULL,
        .attribute_defs[FTDF_PIB_DWELL_TIME].addr = &ftdf_pib.dwell_time,
        .attribute_defs[FTDF_PIB_DWELL_TIME].size = sizeof(ftdf_pib.dwell_time),
        .attribute_defs[FTDF_PIB_DWELL_TIME].getFunc = NULL,
        .attribute_defs[FTDF_PIB_DWELL_TIME].setFunc = NULL,
        .attribute_defs[FTDF_PIB_CSL_PERIOD].addr = &ftdf_pib.csl_period,
        .attribute_defs[FTDF_PIB_CSL_PERIOD].size = sizeof(ftdf_pib.csl_period),
        .attribute_defs[FTDF_PIB_CSL_PERIOD].getFunc = NULL,
        .attribute_defs[FTDF_PIB_CSL_PERIOD].setFunc = NULL,
        .attribute_defs[FTDF_PIB_CSL_MAX_PERIOD].addr = &ftdf_pib.csl_max_period,
        .attribute_defs[FTDF_PIB_CSL_MAX_PERIOD].size = sizeof(ftdf_pib.csl_max_period),
        .attribute_defs[FTDF_PIB_CSL_MAX_PERIOD].getFunc = NULL,
        .attribute_defs[FTDF_PIB_CSL_MAX_PERIOD].setFunc = NULL,
        .attribute_defs[FTDF_PIB_CSL_CHANNEL_MASK].addr = &ftdf_pib.csl_channel_mask,
        .attribute_defs[FTDF_PIB_CSL_CHANNEL_MASK].size = sizeof(ftdf_pib.csl_channel_mask),
        .attribute_defs[FTDF_PIB_CSL_CHANNEL_MASK].getFunc = NULL,
        .attribute_defs[FTDF_PIB_CSL_CHANNEL_MASK].setFunc = NULL,
        .attribute_defs[FTDF_PIB_CSL_FRAME_PENDING_WAIT_T].addr = &ftdf_pib.csl_frame_pending_wait,
        .attribute_defs[FTDF_PIB_CSL_FRAME_PENDING_WAIT_T].size = sizeof(ftdf_pib.csl_frame_pending_wait),
#ifdef FTDF_NO_CSL
        .attribute_defs[FTDF_PIB_CSL_FRAME_PENDING_WAIT_T].getFunc = NULL,
        .attribute_defs[FTDF_PIB_CSL_FRAME_PENDING_WAIT_T].setFunc = NULL,
#else
        .attribute_defs[FTDF_PIB_CSL_FRAME_PENDING_WAIT_T].getFunc = ftdf_get_csl_frame_pending_wait,
        .attribute_defs[FTDF_PIB_CSL_FRAME_PENDING_WAIT_T].setFunc = ftdf_set_csl_frame_pending_wait,
#endif /* FTDF_NO_CSL */
        .attribute_defs[FTDF_PIB_LOW_ENERGY_SUPERFRAME_SUPPORTED].addr = &ftdf_pib.low_energy_superframe_supported,
        .attribute_defs[FTDF_PIB_LOW_ENERGY_SUPERFRAME_SUPPORTED].size = sizeof(ftdf_pib.low_energy_superframe_supported),
        .attribute_defs[FTDF_PIB_LOW_ENERGY_SUPERFRAME_SUPPORTED].getFunc = NULL,
        .attribute_defs[FTDF_PIB_LOW_ENERGY_SUPERFRAME_SUPPORTED].setFunc = NULL,
        .attribute_defs[FTDF_PIB_LOW_ENERGY_SUPERFRAME_SYNC_INTERVAL].addr = &ftdf_pib.low_energy_superframe_sync_interval,
        .attribute_defs[FTDF_PIB_LOW_ENERGY_SUPERFRAME_SYNC_INTERVAL].size = sizeof(ftdf_pib.low_energy_superframe_sync_interval),
        .attribute_defs[FTDF_PIB_LOW_ENERGY_SUPERFRAME_SYNC_INTERVAL].getFunc = NULL,
        .attribute_defs[FTDF_PIB_LOW_ENERGY_SUPERFRAME_SYNC_INTERVAL].setFunc = NULL,
#endif /* !FTDF_LITE */
        .attribute_defs[FTDF_PIB_PERFORMANCE_METRICS].addr = &ftdf_pib.performance_metrics,
        .attribute_defs[FTDF_PIB_PERFORMANCE_METRICS].size = sizeof(ftdf_pib.performance_metrics),
        .attribute_defs[FTDF_PIB_PERFORMANCE_METRICS].getFunc = ftdf_get_lmac_pm_data,
        .attribute_defs[FTDF_PIB_PERFORMANCE_METRICS].setFunc = NULL,
#ifndef FTDF_LITE
        .attribute_defs[FTDF_PIB_USE_ENHANCED_BEACON].addr = &ftdf_pib.use_enhanced_becaon,
        .attribute_defs[FTDF_PIB_USE_ENHANCED_BEACON].size = sizeof(ftdf_pib.use_enhanced_becaon),
        .attribute_defs[FTDF_PIB_USE_ENHANCED_BEACON].getFunc = NULL,
        .attribute_defs[FTDF_PIB_USE_ENHANCED_BEACON].setFunc = NULL,
        .attribute_defs[FTDF_PIB_EB_IE_LIST].addr = &ftdf_pib.eb_ie_list,
        .attribute_defs[FTDF_PIB_EB_IE_LIST].size = sizeof(ftdf_pib.eb_ie_list),
        .attribute_defs[FTDF_PIB_EB_IE_LIST].getFunc = NULL,
        .attribute_defs[FTDF_PIB_EB_IE_LIST].setFunc = NULL,
        .attribute_defs[FTDF_PIB_EB_FILTERING_ENABLED].addr = &ftdf_pib.eb_filtering_enabled,
        .attribute_defs[FTDF_PIB_EB_FILTERING_ENABLED].size = sizeof(ftdf_pib.eb_filtering_enabled),
        .attribute_defs[FTDF_PIB_EB_FILTERING_ENABLED].getFunc = NULL,
        .attribute_defs[FTDF_PIB_EB_FILTERING_ENABLED].setFunc = NULL,
        .attribute_defs[FTDF_PIB_EBSN].addr = &ftdf_pib.eb_sn,
        .attribute_defs[FTDF_PIB_EBSN].size = sizeof(ftdf_pib.eb_sn),
        .attribute_defs[FTDF_PIB_EBSN].getFunc = NULL,
        .attribute_defs[FTDF_PIB_EBSN].setFunc = NULL,
        .attribute_defs[FTDF_PIB_EB_AUTO_SA].addr = &ftdf_pib.eb_auto_sa,
        .attribute_defs[FTDF_PIB_EB_AUTO_SA].size = sizeof(ftdf_pib.eb_auto_sa),
        .attribute_defs[FTDF_PIB_EB_AUTO_SA].getFunc = NULL,
        .attribute_defs[FTDF_PIB_EB_AUTO_SA].setFunc = NULL,
        .attribute_defs[FTDF_PIB_EACK_IE_LIST].addr = &ftdf_pib.e_ack_ie_list,
        .attribute_defs[FTDF_PIB_EACK_IE_LIST].size = sizeof(ftdf_pib.e_ack_ie_list),
        .attribute_defs[FTDF_PIB_EACK_IE_LIST].getFunc = NULL,
        .attribute_defs[FTDF_PIB_EACK_IE_LIST].setFunc = NULL,
        .attribute_defs[FTDF_PIB_KEY_TABLE].addr = &ftdf_pib.key_table,
        .attribute_defs[FTDF_PIB_KEY_TABLE].size = sizeof(ftdf_pib.key_table),
        .attribute_defs[FTDF_PIB_KEY_TABLE].getFunc = NULL,
        .attribute_defs[FTDF_PIB_KEY_TABLE].setFunc = NULL,
        .attribute_defs[FTDF_PIB_DEVICE_TABLE].addr = &ftdf_pib.device_table,
        .attribute_defs[FTDF_PIB_DEVICE_TABLE].size = sizeof(ftdf_pib.device_table),
        .attribute_defs[FTDF_PIB_DEVICE_TABLE].getFunc = NULL,
        .attribute_defs[FTDF_PIB_DEVICE_TABLE].setFunc = NULL,
        .attribute_defs[FTDF_PIB_SECURITY_LEVEL_TABLE].addr = &ftdf_pib.security_level_table,
        .attribute_defs[FTDF_PIB_SECURITY_LEVEL_TABLE].size = sizeof(ftdf_pib.security_level_table),
        .attribute_defs[FTDF_PIB_SECURITY_LEVEL_TABLE].getFunc = NULL,
        .attribute_defs[FTDF_PIB_SECURITY_LEVEL_TABLE].setFunc = NULL,
        .attribute_defs[FTDF_PIB_FRAME_COUNTER].addr = &ftdf_pib.frame_counter,
        .attribute_defs[FTDF_PIB_FRAME_COUNTER].size = sizeof(ftdf_pib.frame_counter),
        .attribute_defs[FTDF_PIB_FRAME_COUNTER].getFunc = NULL,
        .attribute_defs[FTDF_PIB_FRAME_COUNTER].setFunc = NULL,
        .attribute_defs[FTDF_PIB_MT_DATA_SECURITY_LEVEL].addr = &ftdf_pib.mt_data_security_level,
        .attribute_defs[FTDF_PIB_MT_DATA_SECURITY_LEVEL].size = sizeof(ftdf_pib.mt_data_security_level),
        .attribute_defs[FTDF_PIB_MT_DATA_SECURITY_LEVEL].getFunc = NULL,
        .attribute_defs[FTDF_PIB_MT_DATA_SECURITY_LEVEL].setFunc = NULL,
        .attribute_defs[FTDF_PIB_MT_DATA_KEY_ID_MODE].addr = &ftdf_pib.mt_data_key_id_mode,
        .attribute_defs[FTDF_PIB_MT_DATA_KEY_ID_MODE].size = sizeof(ftdf_pib.mt_data_key_id_mode),
        .attribute_defs[FTDF_PIB_MT_DATA_KEY_ID_MODE].getFunc = NULL,
        .attribute_defs[FTDF_PIB_MT_DATA_KEY_ID_MODE].setFunc = NULL,
        .attribute_defs[FTDF_PIB_MT_DATA_KEY_SOURCE].addr = &ftdf_pib.mt_data_key_source,
        .attribute_defs[FTDF_PIB_MT_DATA_KEY_SOURCE].size = sizeof(ftdf_pib.mt_data_key_source),
        .attribute_defs[FTDF_PIB_MT_DATA_KEY_SOURCE].getFunc = NULL,
        .attribute_defs[FTDF_PIB_MT_DATA_KEY_SOURCE].setFunc = NULL,
        .attribute_defs[FTDF_PIB_MT_DATA_KEY_INDEX].addr = &ftdf_pib.mt_data_key_index,
        .attribute_defs[FTDF_PIB_MT_DATA_KEY_INDEX].size = sizeof(ftdf_pib.mt_data_key_index),
        .attribute_defs[FTDF_PIB_MT_DATA_KEY_INDEX].getFunc = NULL,
        .attribute_defs[FTDF_PIB_MT_DATA_KEY_INDEX].setFunc = NULL,
        .attribute_defs[FTDF_PIB_DEFAULT_KEY_SOURCE].addr = &ftdf_pib.default_key_source,
        .attribute_defs[FTDF_PIB_DEFAULT_KEY_SOURCE].size = sizeof(ftdf_pib.default_key_source),
        .attribute_defs[FTDF_PIB_DEFAULT_KEY_SOURCE].getFunc = NULL,
        .attribute_defs[FTDF_PIB_DEFAULT_KEY_SOURCE].setFunc = NULL,
        .attribute_defs[FTDF_PIB_PAN_COORD_EXTENDED_ADDRESS].addr = &ftdf_pib.pan_coord_ext_address,
        .attribute_defs[FTDF_PIB_PAN_COORD_EXTENDED_ADDRESS].size = sizeof(ftdf_pib.pan_coord_ext_address),
        .attribute_defs[FTDF_PIB_PAN_COORD_EXTENDED_ADDRESS].getFunc = NULL,
        .attribute_defs[FTDF_PIB_PAN_COORD_EXTENDED_ADDRESS].setFunc = NULL,
        .attribute_defs[FTDF_PIB_PAN_COORD_SHORT_ADDRESS].addr = &ftdf_pib.pan_coord_short_address,
        .attribute_defs[FTDF_PIB_PAN_COORD_SHORT_ADDRESS].size = sizeof(ftdf_pib.pan_coord_short_address),
        .attribute_defs[FTDF_PIB_PAN_COORD_SHORT_ADDRESS].getFunc = NULL,
        .attribute_defs[FTDF_PIB_PAN_COORD_SHORT_ADDRESS].setFunc = NULL,
        .attribute_defs[FTDF_PIB_FRAME_COUNTER_MODE].addr = &ftdf_pib.frame_counter_mode,
        .attribute_defs[FTDF_PIB_FRAME_COUNTER_MODE].size = sizeof(ftdf_pib.frame_counter_mode),
        .attribute_defs[FTDF_PIB_FRAME_COUNTER_MODE].getFunc = NULL,
        .attribute_defs[FTDF_PIB_FRAME_COUNTER_MODE].setFunc = NULL,
        .attribute_defs[FTDF_PIB_CSL_SYNC_TX_MARGIN].addr = &ftdf_pib.csl_sync_tx_margin,
        .attribute_defs[FTDF_PIB_CSL_SYNC_TX_MARGIN].size = sizeof(ftdf_pib.csl_sync_tx_margin),
        .attribute_defs[FTDF_PIB_CSL_SYNC_TX_MARGIN].getFunc = NULL,
        .attribute_defs[FTDF_PIB_CSL_SYNC_TX_MARGIN].setFunc = NULL,
        .attribute_defs[FTDF_PIB_CSL_MAX_AGE_REMOTE_INFO].addr = &ftdf_pib.csl_max_age_remote_info,
        .attribute_defs[FTDF_PIB_CSL_MAX_AGE_REMOTE_INFO].size = sizeof(ftdf_pib.csl_max_age_remote_info),
        .attribute_defs[FTDF_PIB_CSL_MAX_AGE_REMOTE_INFO].getFunc = NULL,
        .attribute_defs[FTDF_PIB_CSL_MAX_AGE_REMOTE_INFO].setFunc = NULL,
        .attribute_defs[FTDF_PIB_TSCH_ENABLED].addr = &ftdf_pib.tsch_enabled,
        .attribute_defs[FTDF_PIB_TSCH_ENABLED].size = 0,
        .attribute_defs[FTDF_PIB_TSCH_ENABLED].getFunc = NULL,
        .attribute_defs[FTDF_PIB_TSCH_ENABLED].setFunc = NULL,
#ifdef FTDF_NO_CSL
        .attribute_defs[FTDF_PIB_LE_ENABLED].addr = &ftdf_pib.le_enabled,
        .attribute_defs[FTDF_PIB_LE_ENABLED].size = 0,
        .attribute_defs[FTDF_PIB_LE_ENABLED].getFunc = NULL,
        .attribute_defs[FTDF_PIB_LE_ENABLED].setFunc = NULL,
#else
        .attribute_defs[FTDF_PIB_LE_ENABLED].addr = &ftdf_pib.le_enabled,
        .attribute_defs[FTDF_PIB_LE_ENABLED].size = sizeof(ftdf_pib.le_enabled),
        .attribute_defs[FTDF_PIB_LE_ENABLED].getFunc = NULL,
        .attribute_defs[FTDF_PIB_LE_ENABLED].setFunc = ftdf_set_le_enabled,
#endif /* FTDF_NO_CSL */
#endif /* !FTDF_LITE */
        .attribute_defs[FTDF_PIB_CURRENT_CHANNEL].addr = &ftdf_pib.current_channel,
        .attribute_defs[FTDF_PIB_CURRENT_CHANNEL].size = sizeof(ftdf_pib.current_channel),
        .attribute_defs[FTDF_PIB_CURRENT_CHANNEL].getFunc = ftdf_get_current_channel,
        .attribute_defs[FTDF_PIB_CURRENT_CHANNEL].setFunc = ftdf_set_current_channel,
#ifndef FTDF_LITE
        .attribute_defs[FTDF_PIB_CHANNELS_SUPPORTED].addr = (void*)&channels_supported,
        .attribute_defs[FTDF_PIB_CHANNELS_SUPPORTED].size = 0,
        .attribute_defs[FTDF_PIB_CHANNELS_SUPPORTED].getFunc = NULL,
        .attribute_defs[FTDF_PIB_CHANNELS_SUPPORTED].setFunc = NULL,
#endif /* !FTDF_LITE */
        .attribute_defs[FTDF_PIB_TX_POWER_TOLERANCE].addr = &ftdf_pib.tx_power_tolerance,
        .attribute_defs[FTDF_PIB_TX_POWER_TOLERANCE].size = sizeof(ftdf_pib.tx_power_tolerance),
        .attribute_defs[FTDF_PIB_TX_POWER_TOLERANCE].getFunc = NULL,
        .attribute_defs[FTDF_PIB_TX_POWER_TOLERANCE].setFunc = NULL,
        .attribute_defs[FTDF_PIB_TX_POWER].addr = &ftdf_pib.tx_power,
        .attribute_defs[FTDF_PIB_TX_POWER].size = sizeof(ftdf_pib.tx_power),
        .attribute_defs[FTDF_PIB_TX_POWER].getFunc = NULL,
        .attribute_defs[FTDF_PIB_TX_POWER].setFunc = NULL,
        .attribute_defs[FTDF_PIB_CCA_MODE].addr = &ftdf_pib.cca_mode,
        .attribute_defs[FTDF_PIB_CCA_MODE].size = sizeof(ftdf_pib.cca_mode),
        .attribute_defs[FTDF_PIB_CCA_MODE].getFunc = NULL,
        .attribute_defs[FTDF_PIB_CCA_MODE].setFunc = NULL,
#ifndef FTDF_LITE
        .attribute_defs[FTDF_PIB_CURRENT_PAGE].addr = &ftdf_pib.current_page,
        .attribute_defs[FTDF_PIB_CURRENT_PAGE].size = 0,
        .attribute_defs[FTDF_PIB_CURRENT_PAGE].getFunc = NULL,
        .attribute_defs[FTDF_PIB_CURRENT_PAGE].setFunc = NULL,
        .attribute_defs[FTDF_PIB_MAX_FRAME_DURATION].addr = &ftdf_pib.max_frame_duration,
        .attribute_defs[FTDF_PIB_MAX_FRAME_DURATION].size = 0,
        .attribute_defs[FTDF_PIB_MAX_FRAME_DURATION].getFunc = NULL,
        .attribute_defs[FTDF_PIB_MAX_FRAME_DURATION].setFunc = NULL,
        .attribute_defs[FTDF_PIB_SHR_DURATION].addr = &ftdf_pib.shr_duration,
        .attribute_defs[FTDF_PIB_SHR_DURATION].size = 0,
        .attribute_defs[FTDF_PIB_SHR_DURATION].getFunc = NULL,
        .attribute_defs[FTDF_PIB_SHR_DURATION].setFunc = NULL,
#endif /* !FTDF_LITE */
        .attribute_defs[FTDF_PIB_TRAFFIC_COUNTERS].addr = &ftdf_pib.traffic_counters,
        .attribute_defs[FTDF_PIB_TRAFFIC_COUNTERS].size = 0,
        .attribute_defs[FTDF_PIB_TRAFFIC_COUNTERS].getFunc = ftdf_get_lmac_traffic_counters,
        .attribute_defs[FTDF_PIB_TRAFFIC_COUNTERS].setFunc = NULL,
#ifndef FTDF_LITE
        .attribute_defs[FTDF_PIB_LE_CAPABLE].addr = &ftdf_pib.le_capable,
        .attribute_defs[FTDF_PIB_LE_CAPABLE].size = 0,
        .attribute_defs[FTDF_PIB_LE_CAPABLE].getFunc = NULL,
        .attribute_defs[FTDF_PIB_LE_CAPABLE].setFunc = NULL,
        .attribute_defs[FTDF_PIB_LL_CAPABLE].addr = &ftdf_pib.ll_capable,
        .attribute_defs[FTDF_PIB_LL_CAPABLE].size = 0,
        .attribute_defs[FTDF_PIB_LL_CAPABLE].getFunc = NULL,
        .attribute_defs[FTDF_PIB_LL_CAPABLE].setFunc = NULL,
        .attribute_defs[FTDF_PIB_DSME_CAPABLE].addr = &ftdf_pib.dsme_capable,
        .attribute_defs[FTDF_PIB_DSME_CAPABLE].size = 0,
        .attribute_defs[FTDF_PIB_DSME_CAPABLE].getFunc = NULL,
        .attribute_defs[FTDF_PIB_DSME_CAPABLE].setFunc = NULL,
        .attribute_defs[FTDF_PIB_RFID_CAPABLE].addr = &ftdf_pib.rfid_capable,
        .attribute_defs[FTDF_PIB_RFID_CAPABLE].size = 0,
        .attribute_defs[FTDF_PIB_RFID_CAPABLE].getFunc = NULL,
        .attribute_defs[FTDF_PIB_RFID_CAPABLE].setFunc = NULL,
        .attribute_defs[FTDF_PIB_AMCA_CAPABLE].addr = &ftdf_pib.amca_capable,
        .attribute_defs[FTDF_PIB_AMCA_CAPABLE].size = 0,
        .attribute_defs[FTDF_PIB_AMCA_CAPABLE].getFunc = NULL,
        .attribute_defs[FTDF_PIB_AMCA_CAPABLE].setFunc = NULL,
#endif /* !FTDF_LITE */
        .attribute_defs[FTDF_PIB_METRICS_CAPABLE].addr = &ftdf_pib.metrics_capable,
        .attribute_defs[FTDF_PIB_METRICS_CAPABLE].size = 0,
        .attribute_defs[FTDF_PIB_METRICS_CAPABLE].getFunc = NULL,
        .attribute_defs[FTDF_PIB_METRICS_CAPABLE].setFunc = NULL,
#ifndef FTDF_LITE
        .attribute_defs[FTDF_PIB_RANGING_SUPPORTED].addr = &ftdf_pib.ranging_supported,
        .attribute_defs[FTDF_PIB_RANGING_SUPPORTED].size = 0,
        .attribute_defs[FTDF_PIB_RANGING_SUPPORTED].getFunc = NULL,
        .attribute_defs[FTDF_PIB_RANGING_SUPPORTED].setFunc = NULL,
#endif /* !FTDF_LITE */
        .attribute_defs[FTDF_PIB_KEEP_PHY_ENABLED].addr = &ftdf_pib.keep_phy_enabled,
        .attribute_defs[FTDF_PIB_KEEP_PHY_ENABLED].size = sizeof(ftdf_pib.keep_phy_enabled),
        .attribute_defs[FTDF_PIB_KEEP_PHY_ENABLED].getFunc = ftdf_get_keep_phy_enabled,
        .attribute_defs[FTDF_PIB_KEEP_PHY_ENABLED].setFunc = ftdf_set_keep_phy_enabled,
        .attribute_defs[FTDF_PIB_METRICS_ENABLED].addr = &ftdf_pib.metrics_enabled,
        .attribute_defs[FTDF_PIB_METRICS_ENABLED].size = sizeof(ftdf_pib.metrics_enabled),
        .attribute_defs[FTDF_PIB_METRICS_ENABLED].getFunc = NULL,
        .attribute_defs[FTDF_PIB_METRICS_ENABLED].setFunc = NULL,
#ifndef FTDF_LITE
        .attribute_defs[FTDF_PIB_BEACON_AUTO_RESPOND].addr = &ftdf_pib.beacon_auto_respond,
        .attribute_defs[FTDF_PIB_BEACON_AUTO_RESPOND].size = sizeof(ftdf_pib.beacon_auto_respond),
        .attribute_defs[FTDF_PIB_BEACON_AUTO_RESPOND].getFunc = NULL,
        .attribute_defs[FTDF_PIB_BEACON_AUTO_RESPOND].setFunc = NULL,
        .attribute_defs[FTDF_PIB_TSCH_CAPABLE].addr = &ftdf_pib.tsch_capable,
        .attribute_defs[FTDF_PIB_TSCH_CAPABLE].size = 0,
        .attribute_defs[FTDF_PIB_TSCH_CAPABLE].getFunc = NULL,
        .attribute_defs[FTDF_PIB_TSCH_CAPABLE].setFunc = NULL,
        .attribute_defs[FTDF_PIB_TS_SYNC_CORRECT_THRESHOLD].addr = &ftdf_pib.ts_sync_correct_threshold,
        .attribute_defs[FTDF_PIB_TS_SYNC_CORRECT_THRESHOLD].size = sizeof(ftdf_pib.ts_sync_correct_threshold),
        .attribute_defs[FTDF_PIB_TS_SYNC_CORRECT_THRESHOLD].getFunc = NULL,
        .attribute_defs[FTDF_PIB_TS_SYNC_CORRECT_THRESHOLD].setFunc = NULL,
#endif /* !FTDF_LITE */
        .attribute_defs[FTDF_PIB_BO_IRQ_THRESHOLD].addr = &ftdf_pib.bo_irq_threshold,
        .attribute_defs[FTDF_PIB_BO_IRQ_THRESHOLD].size = sizeof(ftdf_pib.bo_irq_threshold),
        .attribute_defs[FTDF_PIB_BO_IRQ_THRESHOLD].getFunc = ftdf_get_bo_irq_threshold,
        .attribute_defs[FTDF_PIB_BO_IRQ_THRESHOLD].setFunc = ftdf_set_bo_irq_threshold,
        .attribute_defs[FTDF_PIB_PTI_CONFIG].addr = &ftdf_pib.pti_config,
        .attribute_defs[FTDF_PIB_PTI_CONFIG].size = sizeof(ftdf_pib.pti_config),
        .attribute_defs[FTDF_PIB_PTI_CONFIG].getFunc = NULL,
        .attribute_defs[FTDF_PIB_PTI_CONFIG].setFunc = ftdf_set_pti_config,
        .attribute_defs[FTDF_PIB_LINK_QUALITY_MODE].addr = &ftdf_pib.link_quality_mode,
        .attribute_defs[FTDF_PIB_LINK_QUALITY_MODE].size = sizeof(ftdf_pib.link_quality_mode),
        .attribute_defs[FTDF_PIB_LINK_QUALITY_MODE].getFunc = NULL,
        .attribute_defs[FTDF_PIB_LINK_QUALITY_MODE].setFunc = NULL,
};

ftdf_boolean_t          ftdf_transparent_mode                              __attribute__ ((section(".retention")));
ftdf_bitmap32_t         ftdf_transparent_mode_options                      __attribute__ ((section(".retention")));
#if FTDF_DBG_BUS_ENABLE
ftdf_dbg_mode_t         ftdf_dbg_mode                                      __attribute__ ((section(".retention")));
#endif
#if dg_configUSE_FTDF_DDPHY == 1
uint16_t            ftdf_ddphy_cca_reg                                  __attribute__ ((section(".retention")));
#endif
#ifndef FTDF_LITE
ftdf_buffer_t           ftdf_req_buffers[FTDF_NR_OF_REQ_BUFFERS]           __attribute__ ((section(".retention")));
ftdf_queue_t            ftdf_req_queue                                     __attribute__ ((section(".retention")));
ftdf_queue_t            ftdf_free_queue                                    __attribute__ ((section(".retention")));
ftdf_pending_t          ftdf_tx_pending_list[FTDF_NR_OF_REQ_BUFFERS]       __attribute__ ((section(".retention")));
ftdf_pending_tl_t       ftdf_tx_pending_timer_list[FTDF_NR_OF_REQ_BUFFERS] __attribute__ ((section(".retention")));
ftdf_pending_tl_t       *ftdf_tx_pending_timer_head                        __attribute__ ((section(".retention")));
ftdf_time_t             ftdf_tx_pending_timer_lt                           __attribute__ ((section(".retention")));
ftdf_time_t             ftdf_tx_pending_timer_time                         __attribute__ ((section(".retention")));
#endif /* !FTDF_LITE */
#ifndef FTDF_PHY_API
ftdf_msg_buffer_t       *ftdf_req_current                                  __attribute__ ((section(".retention")));
#endif
ftdf_size_t             ftdf_nr_of_retries                                 __attribute__ ((section(".retention")));
#if FTDF_USE_SLEEP_DURING_BACKOFF
static ftdf_sdb_t       ftdf_sdb                                           __attribute__ ((section(".retention")));
#endif /* FTDF_USE_SLEEP_DURING_BACKOFF */
#ifndef FTDF_LITE
ftdf_boolean_t          ftdf_is_pan_coordinator                            __attribute__ ((section(".retention")));
ftdf_time_t             ftdf_start_csl_sample_time                         __attribute__ ((section(".retention")));
ftdf_rx_address_admin_t ftdf_rxa[FTDF_NR_OF_RX_ADDRS]                      __attribute__ ((section(".retention")));
#endif /* !FTDF_LITE */
ftdf_boolean_t          ftdf_tx_in_progress                                __attribute__ ((section(".retention")));
#ifndef FTDF_LITE
#ifndef FTDF_NO_CSL
ftdf_peer_csl_timing_t  ftdf_peer_csl_timing[FTDF_NR_OF_CSL_PEERS]         __attribute__ ((section(".retention")));
ftdf_boolean_t          ftdf_old_le_enabled                                __attribute__ ((section(".retention")));
ftdf_time_t             ftdf_rz_time                                       __attribute__ ((section(".retention")));
ftdf_short_address_t    ftdf_send_frame_pending                            __attribute__ ((section(".retention")));
#endif /* FTDF_NO_CSL */
#endif /* !FTDF_LITE */
uint32_t                ftdf_cur_time[2]                                   __attribute__ ((section(".retention")));
ftdf_lmac_counters_t    ftdf_lmac_counters                                 __attribute__ ((section(".retention")));
ftdf_frame_header_t     ftdf_fh;
#ifndef FTDF_LITE
ftdf_security_header    ftdf_sh;
ftdf_assoc_admin_t      ftdf_aa;
#endif /* !FTDF_LITE */

#if dg_configCOEX_ENABLE_CONFIG
/* Packet traffic information used when FTDF is in RX enable. */
static ftdf_pti_t ftdf_rx_pti __attribute__ ((section(".retention")));
#endif
static void send_confirm(ftdf_status_t status, ftdf_msg_id_t msg_id);

void ftdf_reset(int set_default_pib)
{
        if (set_default_pib) {
                /* Reset PIB values to their default values */
                memset(&ftdf_pib, 0, sizeof(ftdf_pib));

                ftdf_pib.ext_address = FTDF_GET_EXT_ADDRESS();
                ftdf_pib.ack_wait_duration = 0x36;
#ifndef FTDF_LITE
                ftdf_pib.auto_request = FTDF_TRUE;
                ftdf_pib.beacon_order = 15;
                ftdf_pib.dsn = ftdf_pib.ext_address & 0xff;
                ftdf_pib.bsn = ftdf_pib.ext_address & 0xff;
                ftdf_pib.eb_sn = ftdf_pib.ext_address & 0xff;
                ftdf_pib.coord_short_address = 0xffff;
#endif /* !FTDF_LITE */
                ftdf_pib.max_be = 5;
                ftdf_pib.max_csma_backoffs = 4;
                ftdf_pib.max_frame_total_wait_time = 1026; /* see asic_vol v40.100.2.30 PR2540 */
                ftdf_pib.max_frame_retries = 3;
                ftdf_pib.min_be = 3;
#ifndef FTDF_LITE
                ftdf_pib.lifs_period = 40;
                ftdf_pib.sifs_period = 12;
#endif /* !FTDF_LITE */
                ftdf_pib.pan_id = 0xffff;
#ifndef FTDF_LITE
                ftdf_pib.response_wait_time = 32;
#endif /* !FTDF_LITE */
                ftdf_pib.short_address = 0xffff;
#ifndef FTDF_LITE
                ftdf_pib.superframe_order = 15;
                ftdf_pib.timestamp_supported = FTDF_TRUE;
                ftdf_pib.transaction_persistence_time = 0x1f4;
                ftdf_pib.enh_ack_wait_duration = 0x360;
                ftdf_pib.eb_auto_sa = FTDF_AUTO_FULL;
#endif /* !FTDF_LITE */
                ftdf_pib.current_channel = 11;
                ftdf_pib.cca_mode = FTDF_CCA_MODE_1;
#ifndef FTDF_LITE
                ftdf_pib.max_frame_duration = FTDF_TBD;
                ftdf_pib.shr_duration = FTDF_TBD;
                ftdf_pib.frame_counter_mode = 4;
#endif /* !FTDF_LITE */
                ftdf_pib.metrics_capable = FTDF_TRUE;
#ifndef FTDF_LITE
                ftdf_pib.beacon_auto_respond = FTDF_TRUE;
#endif /* !FTDF_LITE */
                ftdf_pib.performance_metrics.counter_octets = 4; /* 32 bit counters */
#ifndef FTDF_LITE
                ftdf_pib.join_priority = 1;
                ftdf_pib.slotframe_table.slotframe_entries = ftdf_slotframe_table;
                ftdf_pib.link_table.link_entries = ftdf_link_table;
                ftdf_pib.timeslot_template.ts_cca_offset = 1800;
                ftdf_pib.timeslot_template.ts_cca = 128;
                ftdf_pib.timeslot_template.ts_tx_offset = 2120;
                ftdf_pib.timeslot_template.ts_rx_offset = 1020;
                ftdf_pib.timeslot_template.ts_rx_ack_delay = 800;
                ftdf_pib.timeslot_template.ts_tx_ack_delay = 1000;
                ftdf_pib.timeslot_template.ts_rx_wait = 2200;
                ftdf_pib.timeslot_template.ts_ack_wait = 400;
                ftdf_pib.timeslot_template.ts_rx_tx = 192;
                ftdf_pib.timeslot_template.ts_max_ack = 2400;
                ftdf_pib.timeslot_template.ts_max_ts = 4256;
                ftdf_pib.timeslot_template.ts_timeslot_length = 10000;
                ftdf_pib.ts_sync_correct_threshold = 220;
                ftdf_pib.hopping_sequence_length = 16;
                int i;
                for (i = 0; i < FTDF_PTIS; i++) {
                        ftdf_pib.pti_config.ptis[i] = 0;
                }
#ifdef FTDF_NO_CSL
                ftdf_pib.le_capable = FTDF_FALSE;
#else
                ftdf_pib.le_capable = FTDF_TRUE;
#endif /* FTDF_NO_CSL */
#ifdef FTDF_NO_TSCH
                ftdf_pib.tsch_capable = FTDF_FALSE;
#else
                ftdf_pib.tsch_capable = FTDF_TRUE;
#endif /* FTDF_NO_TSCH */

                int n;

                for (n = 0; n < FTDF_MAX_HOPPING_SEQUENCE_LENGTH; n++) {
                        ftdf_pib.hopping_sequence_list[n] = n + 11;
                }
#endif /* !FTDF_LITE */

                ftdf_transparent_mode = FTDF_FALSE;
#ifndef FTDF_LITE
                ftdf_is_pan_coordinator = FTDF_FALSE;
#endif /* !FTDF_LITE */
                ftdf_lmac_counters.fcs_error_cnt = 0;
                ftdf_lmac_counters.tx_std_ack_cnt = 0;
                ftdf_lmac_counters.rx_std_ack_cnt = 0;

#ifndef FTDF_LITE
                memset(ftdf_pib.default_key_source, 0xff, 8);
#endif /* !FTDF_LITE */
                ftdf_pib.bo_irq_threshold = FTDF_BO_IRQ_THRESHOLD;
                ftdf_pib.link_quality_mode = FTDF_LINK_QUALITY_MODE_RSSI;
        }

        ftdf_init_queues();

        FTDF->FTDF_LMACRESET_REG = REG_MSK(FTDF, FTDF_LMACRESET_REG, LMACRESET_CONTROL) |
                    REG_MSK(FTDF, FTDF_LMACRESET_REG, LMACRESET_RX) |
                    REG_MSK(FTDF, FTDF_LMACRESET_REG, LMACRESET_TX) |
                    REG_MSK(FTDF, FTDF_LMACRESET_REG, LMACRESET_AHB) |
                    REG_MSK(FTDF, FTDF_LMACRESET_REG, LMACRESET_OREG) |
                    REG_MSK(FTDF, FTDF_LMACRESET_REG, LMACRESET_TSTIM) |
                    REG_MSK(FTDF, FTDF_LMACRESET_REG, LMACRESET_SEC) |
                    REG_MSK(FTDF, FTDF_LMACRESET_REG, LMACRESET_COUNT) |
                    REG_MSK(FTDF, FTDF_LMACRESET_REG, LMACRESET_TIMCTRL) |
                    REG_MSK(FTDF, FTDF_LMACRESET_REG, LMACGLOBRESET_COUNT);

        uint32_t wait = 0;

        while (REG_GETF(FTDF, FTDF_LMAC_CONTROL_STATUS_REG, LMACREADY4SLEEP) == 0) {
                wait++;
        }

        REG_SETF(FTDF, FTDF_WAKEUP_CONTROL_OS_REG, WAKEUPTIMERENABLE_CLEAR, 1);

        while (REG_GETF(FTDF, FTDF_LMAC_CONTROL_STATUS_REG, WAKEUPTIMERENABLESTATUS)) {}

        REG_SETF(FTDF, FTDF_WAKEUP_CONTROL_OS_REG, WAKEUPTIMERENABLE_SET, 1);

        while ((REG_GETF(FTDF, FTDF_LMAC_CONTROL_STATUS_REG, WAKEUPTIMERENABLESTATUS)) == 0) {}

#ifndef FTDF_LITE
        int n;

#ifndef FTDF_NO_CSL
        for (n = 0; n < FTDF_NR_OF_CSL_PEERS; n++) {
                ftdf_peer_csl_timing[n].addr = 0xffff;
        }

        ftdf_old_le_enabled = FTDF_FALSE;
        ftdf_wake_up_enable_le = FTDF_FALSE;
        ftdf_send_frame_pending = 0xfffe;
#endif /* FTDF_NO_CSL */
#endif /* !FTDF_LITE */
        ftdf_tx_in_progress = FTDF_FALSE;

        ftdf_init_cur_time64();
#ifndef FTDF_NO_TSCH
        ftdf_init_tsch_retries();
        ftdf_init_backoff();
#endif /* FTDF_NO_TSCH */

#ifndef FTDF_LITE
        for (n = 0; n < FTDF_NR_OF_RX_ADDRS; n++) {
                ftdf_rxa[n].addr_mode = FTDF_NO_ADDRESS;
                ftdf_rxa[n].dsn_valid = FTDF_FALSE;
                ftdf_rxa[n].bsn_valid = FTDF_FALSE;
                ftdf_rxa[n].ebsn_valid = FTDF_FALSE;
        }

#ifndef FTDF_NO_TSCH
        for (n = 0; n < FTDF_NR_OF_NEIGHBORS; n++) {
                ftdf_neighbor_table[n].dst_addr = 0xffff;
        }
#endif /* FTDF_NO_TSCH */
#endif /* !FTDF_LITE */
#if FTDF_USE_SLEEP_DURING_BACKOFF
        ftdf_sdb_fsm_reset();
#endif /* FTDF_USE_SLEEP_DURING_BACKOFF */
        ftdf_init_lmac();

#ifndef FTDF_NO_CSL
        ftdf_rz_time = REG_GETF(FTDF, FTDF_SYMBOLTIMESNAPSHOTVAL_REG, SYMBOLTIMESNAPSHOTVAL);
#endif /* FTDF_NO_CSL */

#if dg_configUSE_FTDF_DDPHY == 1
        ftdf_ddphy_set(0);
#endif
}


#ifndef FTDF_LITE
void ftdf_cleanup_queues(void)
{
        while (!ftdf_is_queue_empty(&ftdf_req_queue)) {
                ftdf_msg_buffer_t *request = ftdf_dequeue_req_tail(&ftdf_req_queue);
                if (request) {
                        ad_ftdf_rel_msg_data(request);
                }
        }

        int n;
        for (n = 0; n < FTDF_NR_OF_REQ_BUFFERS; n++) {
                while (!ftdf_is_queue_empty(&ftdf_tx_pending_list[n].queue)) {
                        ftdf_msg_buffer_t *request = ftdf_dequeue_req_tail(&ftdf_tx_pending_list[n].queue);
                        if (request) {
                                ad_ftdf_rel_msg_data(request);
                        }
                }
        }
}
#endif /* !FTDF_LITE */

#ifndef FTDF_PHY_API
void ftdf_process_reset_request(ftdf_reset_request_t *reset_request)
{
#ifndef FTDF_LITE
        ftdf_cleanup_queues();
#endif
        ftdf_reset(reset_request->set_default_pib);
        ftdf_reset_confirm_t* resetConfirm =
            (ftdf_reset_confirm_t*) FTDF_GET_MSG_BUFFER(sizeof(ftdf_reset_confirm_t));

        resetConfirm->msg_id = FTDF_RESET_CONFIRM;
        resetConfirm->status = FTDF_SUCCESS;

        FTDF_REL_MSG_BUFFER((ftdf_msg_buffer_t*)reset_request);

        FTDF_RCV_MSG((ftdf_msg_buffer_t*)resetConfirm);
}
#endif /* FTDF_PHY_API */

void ftdf_init_lmac(void)
{
        ftdf_pib_attribute_t pib_attribute;

        for (pib_attribute = 1; pib_attribute <= FTDF_NR_OF_PIB_ATTRIBUTES; pib_attribute++) {
                if (pib_attribute_table.attribute_defs[pib_attribute].setFunc != NULL) {
                        pib_attribute_table.attribute_defs[pib_attribute].setFunc();
                }
        }

        if (ftdf_transparent_mode == FTDF_TRUE) {
                ftdf_enable_transparent_mode(FTDF_TRUE, ftdf_transparent_mode_options);
        }

#ifndef FTDF_LITE
        if (ftdf_is_pan_coordinator) {
                REG_SETF(FTDF, FTDF_GLOB_CONTROL_0_REG, ISPANCOORDINATOR, 1);
        }
#endif /* !FTDF_LITE */
        REG_SETF(FTDF, FTDF_LMAC_CONTROL_3_REG, CCAIDLEWAIT, 192);

        FTDF->FTDF_TX_CLEAR_OS_REG = REG_MSK(FTDF, FTDF_TX_CLEAR_OS_REG, TX_FLAG_CLEAR);

        uint32_t phy_params = 0;
        REG_SET_FIELD(FTDF, FTDF_PHY_PARAMETERS_2_REG, PHYTXSTARTUP, phy_params, FTDF_PHYTXSTARTUP);
        REG_SET_FIELD(FTDF, FTDF_PHY_PARAMETERS_2_REG, PHYTXLATENCY, phy_params, FTDF_PHYTXLATENCY);
        REG_SET_FIELD(FTDF, FTDF_PHY_PARAMETERS_2_REG, PHYTXFINISH, phy_params, FTDF_PHYTXFINISH);
        REG_SET_FIELD(FTDF, FTDF_PHY_PARAMETERS_2_REG, PHYTRXWAIT, phy_params, FTDF_PHYTRXWAIT);
        FTDF->FTDF_PHY_PARAMETERS_2_REG = phy_params;

        phy_params = 0;
        REG_SET_FIELD(FTDF, FTDF_PHY_PARAMETERS_3_REG, PHYRXSTARTUP, phy_params, FTDF_PHYRXSTARTUP);
        REG_SET_FIELD(FTDF, FTDF_PHY_PARAMETERS_3_REG, PHYRXLATENCY, phy_params, FTDF_PHYRXLATENCY);
        REG_SET_FIELD(FTDF, FTDF_PHY_PARAMETERS_3_REG, PHYENABLE, phy_params, FTDF_PHYENABLE);
        FTDF->FTDF_PHY_PARAMETERS_3_REG = phy_params;


        REG_SETF(FTDF, FTDF_FTDF_CM_REG, FTDF_CM,
                (FTDF_MSK_TX_CE | FTDF_MSK_RX_CE | FTDF_MSK_SYMBOL_TMR_CE));

        uint32_t rx_mask = REG_MSK(FTDF, FTDF_RX_MASK_REG, RXSOF_M) |
                REG_MSK(FTDF, FTDF_RX_MASK_REG, RX_OVERFLOW_M) |
                REG_MSK(FTDF, FTDF_RX_MASK_REG, RX_BUF_AVAIL_M) |
                REG_MSK(FTDF, FTDF_RX_MASK_REG, RXBYTE_M);
        FTDF->FTDF_RX_MASK_REG = rx_mask;

        FTDF->FTDF_LMAC_MASK_REG = REG_MSK(FTDF, FTDF_LMAC_MASK_REG, RXTIMEREXPIRED_M);

        uint32_t lmac_ctrl_mask = REG_MSK(FTDF, FTDF_LMAC_CONTROL_MASK_REG, SYMBOLTIMETHR_M) |
                REG_MSK(FTDF, FTDF_LMAC_CONTROL_MASK_REG, SYMBOLTIME2THR_M) |
                REG_MSK(FTDF, FTDF_LMAC_CONTROL_MASK_REG, SYNCTIMESTAMP_M);
        FTDF->FTDF_LMAC_CONTROL_MASK_REG = lmac_ctrl_mask;

        FTDF->FTDF_TX_FLAG_CLEAR_M_0_REG |= REG_MSK(FTDF, FTDF_TX_FLAG_CLEAR_M_0_REG, TX_FLAG_CLEAR_M);
        FTDF->FTDF_TX_FLAG_CLEAR_M_1_REG |= REG_MSK(FTDF, FTDF_TX_FLAG_CLEAR_M_1_REG, TX_FLAG_CLEAR_M);

#if FTDF_FP_BIT_MODE == FTDF_FP_BIT_MODE_AUTO
    ftdf_fppr_reset();
    ftdf_fppr_set_mode(FTDF_TRUE, FTDF_FALSE, FTDF_FALSE);
#else
    ftdf_fppr_set_mode(FTDF_FALSE, FTDF_TRUE, FTDF_TRUE);
#endif /* #if FTDF_FP_BIT_MODE == FTDF_FP_BIT_MODE_AUTO */
#if FTDF_USE_SLEEP_DURING_BACKOFF
    /* Unmask long BO interrupt. */
    REG_SETF(FTDF, FTDF_LMAC_MASK_REG, CSMA_CA_BO_THR_M, 1);
#else /* FTDF_USE_SLEEP_DURING_BACKOFF */
    /* Set BO threshold. */
    REG_SETF(FTDF, FTDF_LMAC_CONTROL_11_REG, CSMA_CA_BO_THRESHOLD, FTDF_BO_IRQ_THRESHOLD);

    /* Mask long BO interrupt. */
    REG_SETF(FTDF, FTDF_LMAC_MASK_REG, CSMA_CA_BO_THR_M, 0);
#endif /* FTDF_USE_SLEEP_DURING_BACKOFF */
#if FTDF_USE_LPDP == 1
    ftdf_lpdp_enable(FTDF_TRUE);
#endif /* #if FTDF_USE_LPDP == 1 */
}

#ifdef FTDF_PHY_API
ftdf_pib_attribute_value_t *ftdf_get_value(ftdf_pib_attribute_t pib_attribute)
{

        if ((pib_attribute <= FTDF_NR_OF_PIB_ATTRIBUTES) &&
                (pib_attribute_table.attribute_defs[pib_attribute].addr != NULL)) {

                /* Update PIB attribute with current LMAC status if a getFunc is defined */
                if (pib_attribute_table.attribute_defs[pib_attribute].getFunc != NULL) {
                        pib_attribute_table.attribute_defs[pib_attribute].getFunc();
                }

                return pib_attribute_table.attribute_defs[pib_attribute].addr;
        }

        return NULL;
}

ftdf_status_t ftdf_set_value(ftdf_pib_attribute_t pib_attribute, const ftdf_pib_attribute_value_t *pib_attribute_value)
{
        if ((pib_attribute <= FTDF_NR_OF_PIB_ATTRIBUTES) &&
                (pib_attribute_table.attribute_defs[pib_attribute].addr != NULL)) {

                if (pib_attribute_table.attribute_defs[pib_attribute].size != 0) {
                        memcpy(pib_attribute_table.attribute_defs[pib_attribute].addr,
                               pib_attribute_value,
                               pib_attribute_table.attribute_defs[pib_attribute].size);

                        /* Update LMAC with new PIB attribute value if a setFunc is defined */
                        if (pib_attribute_table.attribute_defs[pib_attribute].setFunc != NULL) {
                                pib_attribute_table.attribute_defs[pib_attribute].setFunc();
                        }

                        return FTDF_SUCCESS;

                } else {
                        return FTDF_READ_ONLY;
                }
        }

        return FTDF_UNSUPPORTED_ATTRIBUTE;
}

#else /* FTDF_PHY_API */
void ftdf_process_get_request(ftdf_get_request_t *get_request)
{
        ftdf_get_confirm_t *get_confirm =
            (ftdf_get_confirm_t*) FTDF_GET_MSG_BUFFER(sizeof(ftdf_get_confirm_t));
        ftdf_pib_attribute_t pib_attribute = get_request->pib_attribute;

        get_confirm->msg_id = FTDF_GET_CONFIRM;
        get_confirm->pib_attribute = pib_attribute;

        if ((pib_attribute <= FTDF_NR_OF_PIB_ATTRIBUTES) &&
                (pib_attribute_table.attribute_defs[pib_attribute].addr != NULL)) {

                /* Update PIB attribute with current LMAC status if a getFunc is defined */
                if (pib_attribute_table.attribute_defs[pib_attribute].getFunc != NULL) {
                        pib_attribute_table.attribute_defs[pib_attribute].getFunc();
                }

                get_confirm->status = FTDF_SUCCESS;
                get_confirm->pib_attribute_value =
                    pib_attribute_table.attribute_defs[pib_attribute].addr;

        } else {
                get_confirm->status = FTDF_UNSUPPORTED_ATTRIBUTE;
        }

        FTDF_REL_MSG_BUFFER((ftdf_msg_buffer_t*)get_request);

        FTDF_RCV_MSG((ftdf_msg_buffer_t*)get_confirm);
}

void ftdf_process_set_request(ftdf_set_request_t *set_request)
{
        ftdf_set_confirm_t *set_confirm =
            (ftdf_set_confirm_t*) FTDF_GET_MSG_BUFFER(sizeof(ftdf_set_confirm_t));
        ftdf_pib_attribute_t pib_attribute = set_request->pib_attribute;

        set_confirm->msg_id = FTDF_SET_CONFIRM;
        set_confirm->pib_attribute = pib_attribute;

        if ((pib_attribute <= FTDF_NR_OF_PIB_ATTRIBUTES) &&
                (pib_attribute_table.attribute_defs[pib_attribute].addr != NULL)) {

                if (pib_attribute_table.attribute_defs[pib_attribute].size != 0) {

                        set_confirm->status = FTDF_SUCCESS;
                        memcpy(pib_attribute_table.attribute_defs[pib_attribute].addr,
                               set_request->pib_attribute_value,
                               pib_attribute_table.attribute_defs[pib_attribute].size);

                        /* Update LMAC with new PIB attribute value if a setFunc is defined */
                        if (pib_attribute_table.attribute_defs[pib_attribute].setFunc != NULL) {
                                pib_attribute_table.attribute_defs[pib_attribute].setFunc();
                        }

                } else {
                        set_confirm->status = FTDF_READ_ONLY;
                }

        } else {
                set_confirm->status = FTDF_UNSUPPORTED_ATTRIBUTE;
        }

        FTDF_REL_MSG_BUFFER((ftdf_msg_buffer_t*)set_request);

        FTDF_RCV_MSG((ftdf_msg_buffer_t*)set_confirm);
}

void ftdf_send_comm_status_indication(ftdf_msg_buffer_t *request,
                                      ftdf_status_t status,
                                      ftdf_pan_id_t pan_id,
                                      ftdf_address_mode_t src_addr_mode,
                                      ftdf_address_t src_addr,
                                      ftdf_address_mode_t dst_addr_mode,
                                      ftdf_address_t dst_addr,
                                      ftdf_security_level_t security_level,
                                      ftdf_key_id_mode_t key_id_mode,
                                      ftdf_octet_t *key_source,
                                      ftdf_key_index_t key_index)
{
        ftdf_comm_status_indication_t *comm_status =
                (ftdf_comm_status_indication_t*) FTDF_GET_MSG_BUFFER(sizeof(ftdf_comm_status_indication_t));

        comm_status->msg_id = FTDF_COMM_STATUS_INDICATION;
        comm_status->pan_id = pan_id;
        comm_status->src_addr_mode = src_addr_mode;
        comm_status->src_addr = src_addr;
        comm_status->dst_addr_mode = dst_addr_mode;
        comm_status->dst_addr = dst_addr;
        comm_status->status = status;
        comm_status->security_level = security_level;
        comm_status->key_id_mode = key_id_mode;
        comm_status->key_index = key_index;

        uint8_t n;

        if (security_level != 0) {
                if (key_id_mode == 0x2) {
                        for (n = 0; n < 4; n++) {
                                comm_status->key_source[n] = key_source[n];
                        }
                } else if (key_id_mode == 0x3) {
                        for (n = 0; n < 8; n++) {
                                comm_status->key_source[n] = key_source[n];
                        }
                }
        }
#ifndef FTDF_LITE
        if (request && ((request->msg_id == FTDF_ORPHAN_RESPONSE) ||
                    (request->msg_id == FTDF_ASSOCIATE_RESPONSE))) {

                if (ftdf_req_current == request) {
                        ftdf_req_current = NULL;
                }

                FTDF_REL_MSG_BUFFER(request);
                FTDF_RCV_MSG((ftdf_msg_buffer_t*)comm_status);
#if FTDF_FP_BIT_MODE == FTDF_FP_BIT_MODE_AUTO
                ftdf_fp_fsm_clear_pending();
#endif /* FTDF_FP_BIT_MODE == FTDF_FP_BIT_MODE_AUTO */
                ftdf_process_next_request();
                return;
        }
#endif /* !FTDF_LITE */
        FTDF_RCV_MSG((ftdf_msg_buffer_t*)comm_status);
}
#endif /* FTDF_PHY_API */

#ifndef FTDF_LITE
ftdf_octet_t* ftdf_add_frame_header(ftdf_octet_t *tx_ptr,
                                    ftdf_frame_header_t *frame_header,
                                    ftdf_data_length_t msdu_length)
{
        uint8_t frame_version;
        uint8_t long_frame_control = 0x00;
        ftdf_boolean_t pan_id_compression = FTDF_FALSE;
        ftdf_bitmap8_t options = frame_header->options;
        ftdf_boolean_t secure = options & FTDF_OPT_SECURITY_ENABLED;
        ftdf_boolean_t frame_pending = options & FTDF_OPT_FRAME_PENDING;
        ftdf_boolean_t ack_tx = options & FTDF_OPT_ACK_REQUESTED;
        ftdf_boolean_t pan_id_present = options & FTDF_OPT_PAN_ID_PRESENT;
        ftdf_boolean_t seq_nr_suppressed = options & FTDF_OPT_SEQ_NR_SUPPRESSED;
        ftdf_boolean_t ies_included = options & FTDF_OPT_IES_PRESENT;
        ftdf_frame_type_t frame_type = frame_header->frame_type;
        ftdf_address_mode_t dst_addr_mode = frame_header->dst_addr_mode;
        ftdf_address_mode_t src_addr_mode = frame_header->src_addr_mode;
        ftdf_pan_id_t dstpan_id = frame_header->dst_pan_id;

        if (frame_type == FTDF_MULTIPURPOSE_FRAME) {

                if (options & (FTDF_OPT_SECURITY_ENABLED | FTDF_OPT_ACK_REQUESTED |
                            FTDF_OPT_PAN_ID_PRESENT | FTDF_OPT_IES_PRESENT |
                            FTDF_OPT_SEQ_NR_SUPPRESSED | FTDF_OPT_FRAME_PENDING)) {

                        long_frame_control = 0x08;
                }

                /* Frame control field byte 1 */
                *tx_ptr++ = 0x05 | long_frame_control | dst_addr_mode << 4 | src_addr_mode << 6;

                if (long_frame_control) {
                        /* Frame control field byte 2 */
                        *tx_ptr++ = (pan_id_present ? 0x01 : 0x00) |
                                (secure ? 0x02 : 0x00) |
                                (seq_nr_suppressed ? 0x04 : 0x00) |
                                (frame_pending ? 0x08 : 0x00) |
                                (ack_tx ? 0x40 : 0x00) |
                                (ies_included ? 0x80 : 0x00);
                }

        } else {
                //        if (panIdPresent || iesIncluded || seqNrSuppressed || (options & FTDF_OPT_ENHANCED) || ftdf_pib.tsch_enabled)
                if (pan_id_present || ies_included || seq_nr_suppressed || (options & FTDF_OPT_ENHANCED)) {
                        frame_version = 0x02;
                } else {

                        if (secure || (msdu_length > FTDF_MAX_MAC_SAFE_PAYLOAD_SIZE)) {
                                frame_version = 0x01;
                        } else {
                                frame_version = 0x00;
                        }
                }

                if (frame_version < 0x02) {
                        if ((dst_addr_mode != FTDF_NO_ADDRESS) &&
                                (src_addr_mode != FTDF_NO_ADDRESS) &&
                                (dstpan_id == frame_header->src_pan_id)) {

                                pan_id_compression = FTDF_TRUE;
                        }

                } else {
                        pan_id_compression = pan_id_present;
                }

                /* Frame control field byte 1 */
                *tx_ptr++ = (frame_type & 0x7) |
                        (secure ? 0x08 : 0x00) |
                        (frame_pending ? 0x10 : 0x00) |
                        (ack_tx ? 0x20 : 0x00) |
                        (pan_id_compression ? 0x40 : 0x00);

                /* Frame control field byte 2 */
                *tx_ptr++ = (seq_nr_suppressed ? 0x01 : 0x00) |
                        (ies_included ? 0x02 : 0x00) |
                        (dst_addr_mode << 2) |
                        (frame_version << 4) |
                        (src_addr_mode << 6);
        }

        if (!seq_nr_suppressed) {
                *tx_ptr++ = frame_header->sn;
        }

        ftdf_boolean_t add_dstpan_id = FTDF_FALSE;

        if (frame_type == FTDF_MULTIPURPOSE_FRAME) {
                if (pan_id_present) {
                        add_dstpan_id = FTDF_TRUE;
                }

        } else {

                if (frame_version < 0x02) {
                        if (dst_addr_mode != FTDF_NO_ADDRESS) {
                                add_dstpan_id = FTDF_TRUE;
                        }

                } else {

                        /* See Table 2a "PAN ID Compression" of IEEE 802.15.4-2011 for more details */
                        if (((src_addr_mode == FTDF_NO_ADDRESS) &&
                                (dst_addr_mode == FTDF_NO_ADDRESS) && pan_id_compression) ||
                                ((src_addr_mode == FTDF_NO_ADDRESS) &&
                                        (dst_addr_mode != FTDF_NO_ADDRESS && !pan_id_compression)) ||
                                ((src_addr_mode != FTDF_NO_ADDRESS) &&
                                        (dst_addr_mode != FTDF_NO_ADDRESS && !pan_id_compression))) {

                                add_dstpan_id = FTDF_TRUE;
                        }
                }
        }

        if (add_dstpan_id) {
                ftdf_octet_t *pan_idPtr = (ftdf_octet_t*)&dstpan_id;

                *tx_ptr++ = *pan_idPtr++;
                *tx_ptr++ = *pan_idPtr;
        }

        ftdf_address_t dst_addr = frame_header->dst_addr;

        if (dst_addr_mode == FTDF_SIMPLE_ADDRESS) {
                *tx_ptr++ = dst_addr.simple_address;
        } else if (dst_addr_mode == FTDF_SHORT_ADDRESS) {
                ftdf_octet_t* short_address_ptr = (ftdf_octet_t*)&dst_addr.short_address;

                *tx_ptr++ = *short_address_ptr++;
                *tx_ptr++ = *short_address_ptr;
        } else if (dst_addr_mode == FTDF_EXTENDED_ADDRESS) {
                ftdf_octet_t* ext_address_ptr = (ftdf_octet_t*)&dst_addr.ext_address;
                int n;

                for (n = 0; n < 8; n++) {
                        *tx_ptr++ = *ext_address_ptr++;
                }
        }

        ftdf_boolean_t add_srcpan_id = FTDF_FALSE;

        if (frame_type != FTDF_MULTIPURPOSE_FRAME) {
                if (frame_version < 0x02) {
                        if ((src_addr_mode != FTDF_NO_ADDRESS) && !pan_id_compression) {
                                add_srcpan_id = FTDF_TRUE;
                        }

                } else {

                        /* See Table 2a "PAN ID Compression" of IEEE 802.15.4-2011 for more details */
                        if ((src_addr_mode != FTDF_NO_ADDRESS) && (dst_addr_mode == FTDF_NO_ADDRESS) &&
                                !pan_id_compression) {

                                add_srcpan_id = FTDF_TRUE;
                        }
                }
        }

        if (add_srcpan_id) {
                ftdf_octet_t* pan_idPtr = (ftdf_octet_t*)&frame_header->src_pan_id;

                *tx_ptr++ = *pan_idPtr++;
                *tx_ptr++ = *pan_idPtr;
        }

        if (src_addr_mode == FTDF_SIMPLE_ADDRESS) {
                *tx_ptr++ = ftdf_pib.simple_address;
        }
        else if (src_addr_mode == FTDF_SHORT_ADDRESS) {
                ftdf_octet_t* short_address_ptr = (ftdf_octet_t*)&ftdf_pib.short_address;

                *tx_ptr++ = *short_address_ptr++;
                *tx_ptr++ = *short_address_ptr;
        }
        else if (src_addr_mode == FTDF_EXTENDED_ADDRESS) {
                ftdf_octet_t* ext_address_ptr = (ftdf_octet_t*)&ftdf_pib.ext_address;
                int n;

                for (n = 0; n < 8; n++) {
                        *tx_ptr++ = *ext_address_ptr++;
                }
        }

        return tx_ptr;
}
#endif /* !FTDF_LITE */

ftdf_pti_t ftdf_get_rx_pti(void)
{
#if dg_configCOEX_ENABLE_CONFIG
        ftdf_pti_t rx_pti;
        ftdf_critical_var();
        ftdf_enter_critical();
        rx_pti = ftdf_rx_pti;
        ftdf_exit_critical();

        return rx_pti;
#else
        return 0;
#endif
}

#ifdef FTDF_PHY_API
void ftdf_rx_enable(ftdf_time_t rx_on_duration)
{
#if dg_configCOEX_ENABLE_CONFIG && !FTDF_USE_AUTO_PTI
        /* We do not force decision here. It will be automatically made when FTDF begins
         * transaction.
         */
        hw_coex_update_ftdf_pti((hw_coex_pti_t) ftdf_get_rx_pti(), NULL, false);
#endif

        ftdf_set_link_quality_mode();

        REG_SETF(FTDF, FTDF_LMAC_CONTROL_OS_REG, RXENABLE, 0);
        REG_SETF(FTDF, FTDF_LMAC_CONTROL_0_REG, RXONDURATION, rx_on_duration);
        REG_SETF(FTDF, FTDF_LMAC_CONTROL_OS_REG, RXENABLE, 1);
}
#else
void ftdf_process_rx_enable_request(ftdf_rx_enable_request_t *rx_enable_request)
{
        ftdf_set_link_quality_mode();

        REG_SETF(FTDF, FTDF_LMAC_CONTROL_OS_REG, RXENABLE, 0);
        REG_SETF(FTDF, FTDF_LMAC_CONTROL_0_REG, RXONDURATION, rx_enable_request->rx_on_duration);
        REG_SETF(FTDF, FTDF_LMAC_CONTROL_OS_REG, RXENABLE, 1);

        ftdf_rx_enable_confirm_t *rx_enable_confirm =
                (ftdf_rx_enable_confirm_t *) FTDF_GET_MSG_BUFFER(sizeof(ftdf_rx_enable_confirm_t));

        rx_enable_confirm->msg_id = FTDF_RX_ENABLE_CONFIRM;
        rx_enable_confirm->status = FTDF_SUCCESS;

        FTDF_REL_MSG_BUFFER((ftdf_msg_buffer_t *)rx_enable_request);
        FTDF_RCV_MSG((ftdf_msg_buffer_t *)rx_enable_confirm);
}
#endif /* FTDF_PHY_API */

ftdf_octet_t *ftdf_get_frame_header(ftdf_octet_t *rx_buffer,
                                    ftdf_frame_header_t* frame_header)
{
        ftdf_frame_type_t frame_type = *rx_buffer & 0x07;
        uint8_t frame_version = 0x00;
        ftdf_bitmap8_t options = 0;
        ftdf_address_mode_t dst_addr_mode;
        ftdf_address_mode_t src_addr_mode;
        ftdf_boolean_t pan_id_compression = FTDF_FALSE;
        ftdf_boolean_t pan_id_present = FTDF_FALSE;

        if (frame_type == FTDF_MULTIPURPOSE_FRAME) {
                dst_addr_mode = (*rx_buffer & 0x30) >> 4;
                src_addr_mode = (*rx_buffer & 0xc0) >> 6;

                /* Check Long Frame Control */
                if (*rx_buffer & 0x08) {
                        rx_buffer++;

                        pan_id_present = *rx_buffer & 0x01;

                        if (*rx_buffer & 0x02) {
                                options |= FTDF_OPT_SECURITY_ENABLED;
                        }

                        if (*rx_buffer & 0x04) {
                                options |= FTDF_OPT_SEQ_NR_SUPPRESSED;
                        }

                        if (*rx_buffer & 0x08) {
                                options |= FTDF_OPT_FRAME_PENDING;
                        }

                        if (*rx_buffer & 0x40) {
                                options |= FTDF_OPT_ACK_REQUESTED;
                        }

                        if (*rx_buffer & 0x80) {
                                options |= FTDF_OPT_IES_PRESENT;
                        }

                        frame_version = 0x00;

                        frame_header->frame_version = FTDF_FRAME_VERSION_E;

                        rx_buffer++;
                } else {
                        rx_buffer++;
                }

        } else {

                if (*rx_buffer & 0x08) {
                        options |= FTDF_OPT_SECURITY_ENABLED;
                }

                if (*rx_buffer & 0x10) {
                        options |= FTDF_OPT_FRAME_PENDING;
                }

                if (*rx_buffer & 0x20) {
                        options |= FTDF_OPT_ACK_REQUESTED;
                }

                pan_id_compression = *rx_buffer & 0x40;

                rx_buffer++;

                frame_version = (*rx_buffer & 0x30) >> 4;

                if (frame_version == 0x02) {
                        if (*rx_buffer & 0x01) {
                                options |= FTDF_OPT_SEQ_NR_SUPPRESSED;
                        }

                        if (*rx_buffer & 0x02) {
                                options |= FTDF_OPT_IES_PRESENT;
                        }

                        frame_header->frame_version = FTDF_FRAME_VERSION_E;
                } else if (frame_version == 0x01) {
                        frame_header->frame_version = FTDF_FRAME_VERSION_2011;
                } else if (frame_version == 0x00) {
                        frame_header->frame_version = FTDF_FRAME_VERSION_2003;
                } else {
                        frame_header->frame_version = FTDF_FRAME_VERSION_NOT_SUPPORTED;
                        return rx_buffer;
                }

                dst_addr_mode = (*rx_buffer & 0x0c) >> 2;
                src_addr_mode = (*rx_buffer & 0xc0) >> 6;

                rx_buffer++;
        }

        if ((options & FTDF_OPT_SEQ_NR_SUPPRESSED) == 0) {
                frame_header->sn = *rx_buffer++;
        }

        ftdf_boolean_t has_dstpan_id = FTDF_FALSE;

        if (frame_type == FTDF_MULTIPURPOSE_FRAME) {
                has_dstpan_id = pan_id_present;
        } else {

                if (frame_version < 0x02) {
                        if (dst_addr_mode != FTDF_NO_ADDRESS) {
                                has_dstpan_id = FTDF_TRUE;
                        }

                } else {

                        if (((src_addr_mode == FTDF_NO_ADDRESS) &&
                                (dst_addr_mode == FTDF_NO_ADDRESS) && pan_id_compression) ||
                                ((src_addr_mode == FTDF_NO_ADDRESS) &&
                                        (dst_addr_mode != FTDF_NO_ADDRESS) && !pan_id_compression) ||
                                ((src_addr_mode != FTDF_NO_ADDRESS) &&
                                        (dst_addr_mode != FTDF_NO_ADDRESS) && !pan_id_compression)) {

                                has_dstpan_id = FTDF_TRUE;
                        }
                }
        }

        if (has_dstpan_id) {
                ftdf_octet_t *pan_id_ptr = (ftdf_octet_t*)&frame_header->dst_pan_id;

                *pan_id_ptr++ = *rx_buffer++;
                *pan_id_ptr = *rx_buffer++;
        }

        if (dst_addr_mode == FTDF_SIMPLE_ADDRESS) {
                frame_header->dst_addr.simple_address = *rx_buffer++;
        } else if (dst_addr_mode == FTDF_SHORT_ADDRESS) {
                ftdf_octet_t *short_address_ptr = (ftdf_octet_t*)&frame_header->dst_addr.short_address;

                *short_address_ptr++ = *rx_buffer++;
                *short_address_ptr = *rx_buffer++;
        } else if (dst_addr_mode == FTDF_EXTENDED_ADDRESS) {
                ftdf_octet_t *ext_address_ptr = (ftdf_octet_t*)&frame_header->dst_addr.ext_address;
                int n;

                for (n = 0; n < 8; n++) {
                        *ext_address_ptr++ = *rx_buffer++;
                }
        }

        ftdf_boolean_t has_srcpan_id = FTDF_FALSE;

        if (frame_version < 0x02 && frame_type != FTDF_MULTIPURPOSE_FRAME) {
                if (src_addr_mode != FTDF_NO_ADDRESS && !pan_id_compression) {
                        has_srcpan_id = FTDF_TRUE;
                }

        } else {

                if ((src_addr_mode != FTDF_NO_ADDRESS) && (dst_addr_mode == FTDF_NO_ADDRESS) &&
                        !pan_id_compression) {

                        has_srcpan_id = FTDF_TRUE;
                }
        }

        if (has_srcpan_id) {
                ftdf_octet_t *pan_id_ptr = (ftdf_octet_t*)&frame_header->src_pan_id;

                *pan_id_ptr++ = *rx_buffer++;
                *pan_id_ptr = *rx_buffer++;
        } else {
                frame_header->src_pan_id = frame_header->dst_pan_id;
        }

        if (src_addr_mode == FTDF_SIMPLE_ADDRESS) {
                frame_header->src_addr.simple_address = *rx_buffer++;
        } else if (src_addr_mode == FTDF_SHORT_ADDRESS) {
                ftdf_octet_t *short_address_ptr =
                        (ftdf_octet_t*)&frame_header->src_addr.short_address;

                *short_address_ptr++ = *rx_buffer++;
                *short_address_ptr = *rx_buffer++;
        } else if (src_addr_mode == FTDF_EXTENDED_ADDRESS) {
                ftdf_octet_t* ext_address_ptr = (ftdf_octet_t*)&frame_header->src_addr.ext_address;
                int n;

                for (n = 0; n < 8; n++) {
                        *ext_address_ptr++ = *rx_buffer++;
                }
        }

        frame_header->frame_type = frame_type;
        frame_header->options = options;
        frame_header->dst_addr_mode = dst_addr_mode;
        frame_header->src_addr_mode = src_addr_mode;

        return rx_buffer;
}

#ifndef FTDF_LITE
void ftdf_process_next_request(void)
{
#ifndef FTDF_NO_TSCH
        if (ftdf_pib.tsch_enabled) {
                ftdf_msg_buffer_t *request = ftdf_tsch_get_pending(ftdf_tsch_slot_link->request);

                ftdf_tsch_slot_link->request = NULL;

                ftdf_schedule_tsch(request);

                return;
        }
#endif /* FTDF_NO_TSCH */

        while (ftdf_req_current == NULL) {
                ftdf_msg_buffer_t *request = ftdf_dequeue_req_tail(&ftdf_req_queue);

                if (request) {
                        ftdf_process_request(request);
                } else {
                        break;
                }
        }
}
#endif /* !FTDF_LITE */

static void process_rx_frame(int read_buf)
{
        static ftdf_pan_descriptor_t pan_descriptor;
        static ftdf_address_t pend_addr_list[7];

        ftdf_frame_header_t *frame_header = &ftdf_fh;
#ifndef FTDF_LITE
        ftdf_security_header *security_header = &ftdf_sh;
#endif /* !FTDF_LITE */

        uint8_t pend_addr_spec = 0;

        ftdf_octet_t *rx_buffer =
                (ftdf_octet_t*) (&FTDF->FTDF_RX_FIFO_0_0_REG) + (read_buf * FTDF_BUFFER_LENGTH);
        ftdf_octet_t *rx_ptr = rx_buffer;
        ftdf_data_length_t frameLen = *rx_ptr++;

        if (ftdf_transparent_mode) {
                if (ftdf_pib.metrics_enabled) {
                        ftdf_pib.performance_metrics.rx_success_count++;
                }

                uint32_t rx_meta_1 = 0;

                ftdf_bitmap32_t status = FTDF_TRANSPARENT_RCV_SUCCESSFUL;

                switch (read_buf) {
                case 0:
                        rx_meta_1 = FTDF->FTDF_RX_META_1_0_REG;
                        break;
                case 1:
                        rx_meta_1 = FTDF->FTDF_RX_META_1_1_REG;
                        break;
                case 2:
                        rx_meta_1 = FTDF->FTDF_RX_META_1_2_REG;
                        break;
                case 3:
                        rx_meta_1 = FTDF->FTDF_RX_META_1_3_REG;
                        break;
                case 4:
                        rx_meta_1 = FTDF->FTDF_RX_META_1_4_REG;
                        break;
                case 5:
                        rx_meta_1 = FTDF->FTDF_RX_META_1_5_REG;
                        break;
                case 6:
                        rx_meta_1 = FTDF->FTDF_RX_META_1_6_REG;
                        break;
                case 7:
                        rx_meta_1 = FTDF->FTDF_RX_META_1_7_REG;
                        break;
                default:
                        OS_ASSERT(0);
                }

                status |= rx_meta_1 & REG_MSK(FTDF, FTDF_RX_META_1_0_REG, CRC16_ERROR) ?
                        FTDF_TRANSPARENT_RCV_CRC_ERROR : 0;
                status |= rx_meta_1 & REG_MSK(FTDF, FTDF_RX_META_1_0_REG, RES_FRM_TYPE_ERROR) ?
                        FTDF_TRANSPARENT_RCV_RES_FRAMETYPE : 0;
                status |= rx_meta_1 & REG_MSK(FTDF, FTDF_RX_META_1_0_REG, RES_FRM_VERSION_ERROR) ?
                        FTDF_TRANSPARENT_RCV_RES_FRAME_VERSION : 0;
                status |= rx_meta_1 & REG_MSK(FTDF, FTDF_RX_META_1_0_REG, DPANID_ERROR) ?
                        FTDF_TRANSPARENT_RCV_UNEXP_DST_PAN_ID : 0;
                status |= rx_meta_1 & REG_MSK(FTDF, FTDF_RX_META_1_0_REG, DADDR_ERROR) ?
                        FTDF_TRANSPARENT_RCV_UNEXP_DST_ADDR : 0;

#ifdef FTDF_PASS_ACK_FRAME
                FTDF_RCV_FRAME_TRANSPARENT(frameLen, rx_ptr, status,
                        REG_GET_FIELD(FTDF, FTDF_RX_META_1_0_REG, QUALITY_INDICATOR, rx_meta_1));
#endif /* FTDF_PASS_ACK_FRAME */

#if FTDF_TRANSPARENT_USE_WAIT_FOR_ACK
                if ((ftdf_transparent_mode_options & FTDF_TRANSPARENT_WAIT_FOR_ACK)) {

                        ftdf_get_frame_header(rx_ptr, frame_header);

                        if ((frame_header->frame_type == FTDF_ACKNOWLEDGEMENT_FRAME) &&
                                (status == FTDF_TRANSPARENT_RCV_SUCCESSFUL)) {

#ifndef FTDF_PHY_API
                                while (REG_GETF(FTDF, FTDF_TX_FLAG_S_0_REG, TX_FLAG_STAT)) {}

                                /* It is required to call FTDF_processTxEvent here because an RX ack
                                 * generates two events. The RX event is raised first, then after
                                 * an IFS the TX event is raised. However, the ftdf_process_next_request
                                 * requires that both events have been handled. */
                                ftdf_process_tx_event();
#endif /* !FTDF_PHY_API */

                                ftdf_sn_t SN = REG_GETF(FTDF, FTDF_TX_META_DATA_1_0_REG, MACSN);

#ifdef FTDF_PHY_API
                                ftdf_critical_var();
                                ftdf_enter_critical();
                                if (ftdf_tx_in_progress && (frame_header->sn == SN)) {
                                        ftdf_exit_critical();
                                        return;
                                }

                                ftdf_exit_critical();
#else
                                if (ftdf_req_current && (frame_header->sn == SN)) {

                                        ftdf_transparent_request_t *transparent_request =
                                                (ftdf_transparent_request_t*)ftdf_req_current;

                                        ftdf_critical_var();
                                        ftdf_enter_critical();
                                        ftdf_req_current = NULL;
                                        ftdf_exit_critical();
                                        FTDF_SEND_FRAME_TRANSPARENT_CONFIRM(transparent_request->handle,
                                                                            FTDF_TRANSPARENT_SEND_SUCCESSFUL);

                                        FTDF_REL_MSG_BUFFER((ftdf_msg_buffer_t *)transparent_request);

                                        return;
                                }
#endif /* FTDF_PHY_API */
                        }
                }
#endif /* FTDF_TRANSPARENT_USE_WAIT_FOR_ACK */
#ifndef FTDF_PASS_ACK_FRAME
                FTDF_RCV_FRAME_TRANSPARENT(frameLen, rx_ptr, status,
                        REG_GET_FIELD(FTDF, FTDF_RX_META_1_0_REG, QUALITY_INDICATOR, rx_meta_1));
#endif /* !FTDF_PASS_ACK_FRAME */

                return;
        }

#ifndef FTDF_LITE
        rx_ptr = ftdf_get_frame_header(rx_ptr, frame_header);

#if FTDF_FP_BIT_MODE == FTDF_FP_BIT_MODE_MANUAL
    if (frame_header->options & FTDF_OPT_ACK_REQUESTED) {
            int n;
            ftdf_boolean_t address_found = FTDF_FALSE;
            for (n = 0; n < FTDF_NR_OF_REQ_BUFFERS; n++) {
                    if (ftdf_tx_pending_list[n].addr_mode == frame_header->src_addr_mode &&
                            ftdf_tx_pending_list[n].pan_id == frame_header->src_pan_id) {
                            if (frame_header->src_addr_mode == FTDF_SHORT_ADDRESS) {
                                    if (ftdf_tx_pending_list[n].addr.short_address == frame_header->src_addr.short_address) {
                                            address_found = FTDF_TRUE;
                                            break;
                                    }
                            } else if (frame_header->src_addr_mode == FTDF_EXTENDED_ADDRESS) {
                                    if (ftdf_tx_pending_list[n].addr.ext_address == frame_header->src_addr.ext_address) {
                                            address_found = FTDF_TRUE;
                                            break;
                                    }
                            } else {
                                    // Invalid srcAddrMode
                                    return;
                            }
                    }
            }
            if (address_found) {
                    ftdf_fppr_set_mode(FTDF_FALSE, FTDF_TRUE, FTDF_TRUE);
            } else {
                    ftdf_fppr_set_mode(FTDF_FALSE, FTDF_TRUE, FTDF_FALSE);
            }
    }
#endif

        if (frame_header->frame_version == FTDF_FRAME_VERSION_NOT_SUPPORTED) {
                return;
        }
#if defined(FTDF_NO_CSL) && defined(FTDF_NO_TSCH)
        else if ((frame_header->frame_version == FTDF_FRAME_VERSION_E) ||
                (frame_header->frame_type == FTDF_MULTIPURPOSE_FRAME)) {

                return;
        }
#endif /* FTDF_NO_CSL && FTDF_NO_TSCH */

        ftdf_frame_type_t frame_type = frame_header->frame_type;
        ftdf_boolean_t duplicate = FTDF_FALSE;

        if (((frame_header->options & FTDF_OPT_SEQ_NR_SUPPRESSED) == 0) &&
                (frame_header->src_addr_mode != FTDF_NO_ADDRESS)) {

                ftdf_time_t timestamp;
                ftdf_sn_sel sn_sel = FTDF_SN_SEL_DSN;
                ftdf_boolean_t drop;

                switch(read_buf) {
                case 0:
                        timestamp = REG_GETF(FTDF, FTDF_RX_META_0_0_REG, RX_TIMESTAMP);
                        break;
                case 1:
                        timestamp = REG_GETF(FTDF, FTDF_RX_META_0_1_REG, RX_TIMESTAMP);
                        break;
                case 2:
                        timestamp = REG_GETF(FTDF, FTDF_RX_META_0_2_REG, RX_TIMESTAMP);
                        break;
                case 3:
                        timestamp = REG_GETF(FTDF, FTDF_RX_META_0_3_REG, RX_TIMESTAMP);
                        break;
                case 4:
                        timestamp = REG_GETF(FTDF, FTDF_RX_META_0_4_REG, RX_TIMESTAMP);
                        break;
                case 5:
                        timestamp = REG_GETF(FTDF, FTDF_RX_META_0_5_REG, RX_TIMESTAMP);
                        break;
                case 6:
                        timestamp = REG_GETF(FTDF, FTDF_RX_META_0_6_REG, RX_TIMESTAMP);
                        break;
                case 7:
                        timestamp = REG_GETF(FTDF, FTDF_RX_META_0_7_REG, RX_TIMESTAMP);
                        break;
                default:
                        OS_ASSERT(0);
                }

                if ((ftdf_pib.tsch_enabled || (frame_header->frame_version == FTDF_FRAME_VERSION_E)) &&
                        (frame_header->options & FTDF_OPT_ACK_REQUESTED)) {

                        drop = FTDF_FALSE;
                } else {
                        drop = FTDF_TRUE;
                }

                if (frame_type == FTDF_BEACON_FRAME) {
                        sn_sel = (frame_header->frame_version == FTDF_FRAME_VERSION_E) ?
                                FTDF_SN_SEL_EBSN : FTDF_SN_SEL_BSN;
                }

                uint8_t i;

                for (i = 0; i < FTDF_NR_OF_RX_ADDRS; i++) {

                        /* Check if entry is empty or matches */
                        if ((ftdf_rxa[i].addr_mode == FTDF_NO_ADDRESS) ||
                                ((ftdf_rxa[i].addr_mode == frame_header->src_addr_mode) &&
                                 (((frame_header->src_addr_mode == FTDF_SHORT_ADDRESS) &&
                                   (frame_header->src_addr.short_address == ftdf_rxa[i].addr.short_address)) ||
                                  ((frame_header->src_addr_mode == FTDF_EXTENDED_ADDRESS) &&
                                   (frame_header->src_addr.ext_address == ftdf_rxa[i].addr.ext_address))))) {

                                break;
                        }
                }

                if (i < FTDF_NR_OF_RX_ADDRS) {
                        if (ftdf_rxa[i].addr_mode != FTDF_NO_ADDRESS) {
                                switch (sn_sel) {
                                case FTDF_SN_SEL_DSN:

                                        if (ftdf_rxa[i].dsn_valid == FTDF_TRUE) {
                                                if (frame_header->sn == ftdf_rxa[i].dsn) {
                                                        if (ftdf_pib.metrics_enabled) {
                                                                ftdf_pib.performance_metrics.duplicate_frame_count++;
                                                        }

                                                        if (drop) {
                                                                return;
                                                        }

                                                        duplicate = FTDF_TRUE;
                                                }
                                        } else {
                                                ftdf_rxa[i].dsn_valid = FTDF_TRUE;
                                        }

                                        ftdf_rxa[i].dsn = frame_header->sn;
                                        break;
                                case FTDF_SN_SEL_BSN:

                                        if (ftdf_rxa[i].bsn_valid == FTDF_TRUE) {
                                                if (frame_header->sn == ftdf_rxa[i].bsn) {
                                                        if (ftdf_pib.metrics_enabled) {
                                                                ftdf_pib.performance_metrics.duplicate_frame_count++;
                                                        }

                                                        if (drop) {
                                                                return;
                                                        }

                                                        duplicate = FTDF_TRUE;
                                                }
                                        } else {
                                                ftdf_rxa[i].bsn_valid = FTDF_TRUE;
                                        }

                                        ftdf_rxa[i].bsn = frame_header->sn;
                                        break;
                                case FTDF_SN_SEL_EBSN:

                                        if (ftdf_rxa[i].ebsn_valid == FTDF_TRUE) {
                                                if (frame_header->sn == ftdf_rxa[i].eb_sn) {
                                                        if (ftdf_pib.metrics_enabled) {
                                                                ftdf_pib.performance_metrics.duplicate_frame_count++;
                                                        }

                                                        if (drop) {
                                                                return;
                                                        }

                                                        duplicate = FTDF_TRUE;
                                                }
                                        } else {
                                                ftdf_rxa[i].ebsn_valid = FTDF_TRUE;
                                        }

                                        ftdf_rxa[i].eb_sn = frame_header->sn;
                                        break;
                                }
                        } else {
                                ftdf_rxa[i].addr_mode = frame_header->src_addr_mode;
                                ftdf_rxa[i].addr = frame_header->src_addr;

                                switch (sn_sel) {
                                case FTDF_SN_SEL_DSN:
                                        ftdf_rxa[i].dsn_valid = FTDF_TRUE;
                                        ftdf_rxa[i].dsn = frame_header->sn;
                                        break;
                                case FTDF_SN_SEL_BSN:
                                        ftdf_rxa[i].bsn_valid = FTDF_TRUE;
                                        ftdf_rxa[i].bsn = frame_header->sn;
                                        break;
                                case FTDF_SN_SEL_EBSN:
                                        ftdf_rxa[i].ebsn_valid = FTDF_TRUE;
                                        ftdf_rxa[i].eb_sn = frame_header->sn;
                                        break;
                                }
                        }

                        ftdf_rxa[i].timestamp = timestamp;
                } else {
                        /* find oldest entry and overwrite it */
                        ftdf_time_t cur_time =
                            REG_GETF(FTDF, FTDF_SYMBOLTIMESNAPSHOTVAL_REG, SYMBOLTIMESNAPSHOTVAL);
                        ftdf_time_t delta, greatest_delta = 0;
                        uint8_t entry = 0;

                        for (i = 0; i < FTDF_NR_OF_RX_ADDRS; i++) {
                                delta = cur_time - ftdf_rxa[i].timestamp;

                                if (delta > greatest_delta) {
                                        greatest_delta = delta;
                                        entry = i;
                                }
                        }

                        ftdf_rxa[entry].addr_mode = frame_header->src_addr_mode;
                        ftdf_rxa[entry].addr = frame_header->src_addr;

                        switch (sn_sel) {
                        case FTDF_SN_SEL_DSN:
                                ftdf_rxa[entry].bsn_valid = FTDF_FALSE;
                                ftdf_rxa[entry].ebsn_valid = FTDF_FALSE;
                                ftdf_rxa[entry].dsn_valid = FTDF_TRUE;
                                ftdf_rxa[entry].dsn = frame_header->sn;
                                break;
                        case FTDF_SN_SEL_BSN:
                                ftdf_rxa[entry].dsn_valid = FTDF_FALSE;
                                ftdf_rxa[entry].ebsn_valid = FTDF_FALSE;
                                ftdf_rxa[entry].bsn_valid = FTDF_TRUE;
                                ftdf_rxa[entry].bsn = frame_header->sn;
                                break;
                        case FTDF_SN_SEL_EBSN:
                                ftdf_rxa[entry].dsn_valid = FTDF_FALSE;
                                ftdf_rxa[entry].bsn_valid = FTDF_FALSE;
                                ftdf_rxa[entry].ebsn_valid = FTDF_TRUE;
                                ftdf_rxa[entry].eb_sn = frame_header->sn;
                                break;
                        }
                }
        }

        if (frame_header->options & FTDF_OPT_SECURITY_ENABLED) {
                rx_ptr = ftdf_get_security_header(rx_ptr, frame_header->frame_version, security_header);
        } else {
                security_header->security_level = 0;
                security_header->key_id_mode = 0;
        }

        ftdf_ie_list_t* header_ie_list = NULL;
        ftdf_ie_list_t* payload_ie_list = NULL;
        int mic_length = ftdf_get_mic_length(security_header->security_level);

#if !defined(FTDF_NO_CSL) || !defined(FTDF_NO_TSCH)
        if (frame_header->options & FTDF_OPT_IES_PRESENT) {
                rx_ptr = ftdf_get_ies(rx_ptr, rx_buffer + (frameLen - mic_length - FTDF_FCS_LENGTH),
                                      &header_ie_list, &payload_ie_list);
        }
#endif /* !FTDF_NO_CSL || !FTDF_NO_TSCH */

        /* Get start of private data (needed to unsecure a frame) */
        if (frame_type == FTDF_MAC_COMMAND_FRAME) {

                frame_header->command_frame_id = *rx_ptr++;

        } else if (frame_type == FTDF_BEACON_FRAME) {

                pan_descriptor.coord_addr_mode = frame_header->src_addr_mode;
                pan_descriptor.coord_pan_id = frame_header->src_pan_id;
                pan_descriptor.coord_addr = frame_header->src_addr;
                pan_descriptor.channel_number = REG_GETF(FTDF, FTDF_LMAC_CONTROL_1_REG, PHYRXATTR_CN) + 11;
                pan_descriptor.channel_page = 0;

                switch (read_buf) {
                case 0:
                        pan_descriptor.timestamp = REG_GETF(FTDF, FTDF_RX_META_0_0_REG, RX_TIMESTAMP);
                        pan_descriptor.link_quality = REG_GETF(FTDF, FTDF_RX_META_1_0_REG, QUALITY_INDICATOR);
                        break;
                case 1:
                        pan_descriptor.timestamp = REG_GETF(FTDF, FTDF_RX_META_0_1_REG, RX_TIMESTAMP);
                        pan_descriptor.link_quality = REG_GETF(FTDF, FTDF_RX_META_1_1_REG, QUALITY_INDICATOR);
                        break;
                case 2:
                        pan_descriptor.timestamp = REG_GETF(FTDF, FTDF_RX_META_0_2_REG, RX_TIMESTAMP);
                        pan_descriptor.link_quality = REG_GETF(FTDF, FTDF_RX_META_1_2_REG, QUALITY_INDICATOR);
                        break;
                case 3:
                        pan_descriptor.timestamp = REG_GETF(FTDF, FTDF_RX_META_0_3_REG, RX_TIMESTAMP);
                        pan_descriptor.link_quality = REG_GETF(FTDF, FTDF_RX_META_1_3_REG, QUALITY_INDICATOR);
                        break;
                case 4:
                        pan_descriptor.timestamp = REG_GETF(FTDF, FTDF_RX_META_0_4_REG, RX_TIMESTAMP);
                        pan_descriptor.link_quality = REG_GETF(FTDF, FTDF_RX_META_1_4_REG, QUALITY_INDICATOR);
                        break;
                case 5:
                        pan_descriptor.timestamp = REG_GETF(FTDF, FTDF_RX_META_0_5_REG, RX_TIMESTAMP);
                        pan_descriptor.link_quality = REG_GETF(FTDF, FTDF_RX_META_1_5_REG, QUALITY_INDICATOR);
                        break;
                case 6:
                        pan_descriptor.timestamp = REG_GETF(FTDF, FTDF_RX_META_0_6_REG, RX_TIMESTAMP);
                        pan_descriptor.link_quality = REG_GETF(FTDF, FTDF_RX_META_1_6_REG, QUALITY_INDICATOR);
                        break;
                case 7:
                        pan_descriptor.timestamp = REG_GETF(FTDF, FTDF_RX_META_0_7_REG, RX_TIMESTAMP);
                        pan_descriptor.link_quality = REG_GETF(FTDF, FTDF_RX_META_1_7_REG, QUALITY_INDICATOR);
                        break;
                default:
                        /* Unsupported register index occurred */
                        OS_ASSERT(0);
                }

                ftdf_octet_t* superframe_spec_ptr = (ftdf_octet_t*)&pan_descriptor.superframe_spec;
                *superframe_spec_ptr++ = *rx_ptr++;
                *superframe_spec_ptr = *rx_ptr++;

                uint8_t gts_spec = *rx_ptr++;

                pan_descriptor.gts_permit = (gts_spec & 0x08) ? FTDF_TRUE : FTDF_FALSE;
                uint8_t gts_descr_count = gts_spec & 0x7;

                if (gts_descr_count != 0) {
                        /* GTS is not supported, so just skip the GTS direction and GTS list
                         * fields if present */
                        rx_ptr += (1 + (3 * gts_descr_count));
                }

                pend_addr_spec = *rx_ptr++;
                uint8_t nr_of_short_addrs = pend_addr_spec & 0x07;
                uint8_t nr_of_ext_addrs = (pend_addr_spec & 0x70) >> 4;

                int n;

                for (n = 0; n < (nr_of_short_addrs + nr_of_ext_addrs); n++) {
                        if (n < nr_of_short_addrs) {

                                ftdf_octet_t* short_address_ptr =
                                        (ftdf_octet_t*)&pend_addr_list[n].short_address;
                                *short_address_ptr++ = *rx_buffer++;
                                *short_address_ptr = *rx_buffer++;

                        } else {

                                ftdf_octet_t* ext_address_ptr =
                                        (ftdf_octet_t*)&pend_addr_list[n].ext_address;
                                int m;

                                for (m = 0; m < 8; m++) {
                                        *ext_address_ptr++ = *rx_buffer++;
                                }
                        }
                }
        }  else if ((frame_type == FTDF_ACKNOWLEDGEMENT_FRAME) &&
                (security_header->security_level != 0)) {

                if (ftdf_req_current) {
                        switch (ftdf_req_current->msg_id) {
                        case FTDF_DATA_REQUEST:
                        {
                                ftdf_data_request_t *data_request =
                                        (ftdf_data_request_t*)ftdf_req_current;

                                frame_header->src_pan_id = data_request->dst_pan_id;
                                frame_header->src_addr_mode = data_request->dst_addr_mode;
                                frame_header->src_addr = data_request->dst_addr;
                                break;
                        }
                        case FTDF_POLL_REQUEST:
                        {
                                ftdf_poll_request_t *poll_request =
                                        (ftdf_poll_request_t*)ftdf_req_current;

                                frame_header->src_pan_id = poll_request->coord_pan_id;
                                frame_header->src_addr_mode = poll_request->coord_addr_mode;
                                frame_header->src_addr = poll_request->coord_addr;
                                break;
                        }
                        case FTDF_ASSOCIATE_REQUEST:
                        {
                                ftdf_associate_request_t *associate_request =
                                        (ftdf_associate_request_t*)ftdf_req_current;

                                frame_header->src_pan_id = associate_request->coord_pan_id;
                                frame_header->src_addr_mode = associate_request->coord_addr_mode;
                                frame_header->src_addr = associate_request->coord_addr;
                                break;
                        }
                        case FTDF_DISASSOCIATE_REQUEST:
                        {
                                ftdf_disassociate_request_t *disassociate_request =
                                        (ftdf_disassociate_request_t*)ftdf_req_current;

                                frame_header->src_pan_id = disassociate_request->device_pan_id;
                                frame_header->src_addr_mode =
                                        disassociate_request->device_addr_mode;
                                frame_header->src_addr = disassociate_request->device_address;
                                break;
                        }
                        case FTDF_ASSOCIATE_RESPONSE:
                        {
                                ftdf_associate_response_t *associate_response =
                                        (ftdf_associate_response_t*)ftdf_req_current;

                                frame_header->src_addr_mode = FTDF_EXTENDED_ADDRESS;
                                frame_header->src_addr.ext_address =
                                        associate_response->device_address;
                                break;
                        }
                        }
                }
        }

        ftdf_status_t status = ftdf_unsecure_frame(rx_buffer, rx_ptr, frame_header, security_header);

        if (status != FTDF_SUCCESS) {
                if (ftdf_pib.metrics_enabled) {
                        ftdf_pib.performance_metrics.security_failure_count++;
                }

                FTDF_REL_DATA_BUFFER((ftdf_octet_t*)payload_ie_list);

                /* Since unsecure of acknowledgement frame is always successful,
                 * nothing special has to be done to get the address information correct. */
                ftdf_send_comm_status_indication(ftdf_req_current, status,
                                                 ftdf_pib.pan_id,
                                                 frame_header->src_addr_mode,
                                                 frame_header->src_addr,
                                                 frame_header->dst_addr_mode,
                                                 frame_header->dst_addr,
                                                 security_header->security_level,
                                                 security_header->key_id_mode,
                                                 security_header->key_source,
                                                 security_header->key_index);

                if ((frame_type == FTDF_ACKNOWLEDGEMENT_FRAME) && ftdf_req_current) {

                        send_confirm(FTDF_NO_ACK, ftdf_req_current->msg_id);

                        ftdf_process_next_request();
                }

                return;
        }

        if (ftdf_pib.metrics_enabled && frame_type != FTDF_ACKNOWLEDGEMENT_FRAME) {
                ftdf_pib.performance_metrics.rx_success_count++;
        }

#ifndef FTDF_NO_TSCH
        if (ftdf_pib.tsch_enabled && frame_type != FTDF_ACKNOWLEDGEMENT_FRAME) {
                ftdf_time_t timestamp;

                switch (read_buf) {
                case 0:
                        timestamp = REG_GETF(FTDF, FTDF_RX_META_0_0_REG, RX_TIMESTAMP);
                        break;
                case 1:
                        timestamp = REG_GETF(FTDF, FTDF_RX_META_0_1_REG, RX_TIMESTAMP);
                        break;
                case 2:
                        timestamp = REG_GETF(FTDF, FTDF_RX_META_0_2_REG, RX_TIMESTAMP);
                        break;
                case 3:
                        timestamp = REG_GETF(FTDF, FTDF_RX_META_0_3_REG, RX_TIMESTAMP);
                        break;
                case 4:
                        timestamp = REG_GETF(FTDF, FTDF_RX_META_0_4_REG, RX_TIMESTAMP);
                        break;
                case 5:
                        timestamp = REG_GETF(FTDF, FTDF_RX_META_0_5_REG, RX_TIMESTAMP);
                        break;
                case 6:
                        timestamp = REG_GETF(FTDF, FTDF_RX_META_0_6_REG, RX_TIMESTAMP);
                        break;
                case 7:
                        timestamp = REG_GETF(FTDF, FTDF_RX_META_0_7_REG, RX_TIMESTAMP);
                        break;
                default:
                        /* Unsupported register index occurred */
                        OS_ASSERT(0);
                }

                ftdf_correct_slot_time(timestamp);
        }
#endif /* FTDF_NO_TSCH */

        switch (frame_type) {
        case FTDF_ACKNOWLEDGEMENT_FRAME:
                if (ftdf_pib.metrics_enabled) {
                        if (ftdf_nr_of_retries == 0) {
                                ftdf_pib.performance_metrics.tx_success_count++;
                        } else if (ftdf_nr_of_retries == 1) {
                                ftdf_pib.performance_metrics.retry_count++;
                        } else {
                                ftdf_pib.performance_metrics.multiple_retry_count++;
                        }
                }

                if (frame_header->frame_version == FTDF_FRAME_VERSION_E) {
                        ftdf_pib.traffic_counters.rx_enh_ack_frm_ok_cnt++;
                }
                break;
        case FTDF_BEACON_FRAME:
                ftdf_pib.traffic_counters.rx_beacon_frm_ok_cnt++;
                break;
        case FTDF_DATA_FRAME:
                ftdf_pib.traffic_counters.rx_data_frm_ok_cnt++;
                break;
        case FTDF_MAC_COMMAND_FRAME:
                ftdf_pib.traffic_counters.rx_cmd_frm_ok_cnt++;
                break;
        case FTDF_MULTIPURPOSE_FRAME:
                ftdf_pib.traffic_counters.rx_multi_purp_frm_ok_cnt++;
                break;
        }

        if (frame_type == FTDF_ACKNOWLEDGEMENT_FRAME) {

                while (REG_GETF(FTDF, FTDF_TX_FLAG_S_0_REG, TX_FLAG_STAT)) {}

                /* It is required to call ftdf_process_tx_event here because an RX ack generates two events
                 * The RX event is raised first, then after an IFS the TX event is raised. However,
                 * the ftdf_process_next_request requires that both events have been handled. */
                ftdf_process_tx_event();

                ftdf_sn_t SN = REG_GETF(FTDF, FTDF_TX_META_DATA_1_0_REG, MACSN);

                if (ftdf_req_current && frame_header->sn == SN) {
#ifndef FTDF_NO_CSL
                        if (ftdf_pib.le_enabled == FTDF_TRUE) {
                                ftdf_time_t timestamp;

                                switch (read_buf) {
                                case 0:
                                        timestamp = REG_GETF(FTDF, FTDF_RX_META_0_0_REG, RX_TIMESTAMP);
                                        break;
                                case 1:
                                        timestamp = REG_GETF(FTDF, FTDF_RX_META_0_1_REG, RX_TIMESTAMP);
                                        break;
                                case 2:
                                        timestamp = REG_GETF(FTDF, FTDF_RX_META_0_2_REG, RX_TIMESTAMP);
                                        break;
                                case 3:
                                        timestamp = REG_GETF(FTDF, FTDF_RX_META_0_3_REG, RX_TIMESTAMP);
                                        break;
                                case 4:
                                        timestamp = REG_GETF(FTDF, FTDF_RX_META_0_4_REG, RX_TIMESTAMP);
                                        break;
                                case 5:
                                        timestamp = REG_GETF(FTDF, FTDF_RX_META_0_5_REG, RX_TIMESTAMP);
                                        break;
                                case 6:
                                        timestamp = REG_GETF(FTDF, FTDF_RX_META_0_6_REG, RX_TIMESTAMP);
                                        break;
                                case 7:
                                        timestamp = REG_GETF(FTDF, FTDF_RX_META_0_7_REG, RX_TIMESTAMP);
                                        break;
                                default:
                                        /* Unsupported register index occurred */
                                        OS_ASSERT(0);
                                }

                                ftdf_set_peer_csl_timing(header_ie_list, timestamp);
                        }
#endif /* FTDF_NO_CSL */

#ifndef FTDF_NO_TSCH
                        if (ftdf_pib.tsch_enabled == FTDF_TRUE) {
                                ftdf_correct_slot_time_from_ack(header_ie_list);

                                ftdf_tsch_retry_t* tsch_retry =
                                    ftdf_get_tsch_retry(ftdf_get_request_address(ftdf_req_current));

                                tsch_retry->nr_of_retries = 0;
                                ftdf_tsch_slot_link->request = NULL;
                        }
#endif /* FTDF_NO_TSCH */

                        switch (ftdf_req_current->msg_id) {
                        case FTDF_DATA_REQUEST:
                        {
                                ftdf_time_t timestamp = REG_GETF(FTDF, FTDF_TX_RETURN_STATUS_0_0_REG, TXTIMESTAMP);
                                ftdf_num_of_backoffs_t num_of_backoffs = REG_GETF(FTDF, FTDF_TX_RETURN_STATUS_1_0_REG, CSMACANRRETRIES);

                                ftdf_send_data_confirm((ftdf_data_request_t*)ftdf_req_current,
                                                       FTDF_SUCCESS,
                                                       timestamp,
                                                       SN,
                                                       num_of_backoffs,
                                                       payload_ie_list);
                                break;
                        }
                        case FTDF_POLL_REQUEST:
                        {
                                if (!(frame_header->options & FTDF_OPT_FRAME_PENDING)) {
                                        ftdf_send_poll_confirm((ftdf_poll_request_t*)ftdf_req_current,
                                                               FTDF_NO_DATA);
                                }
                                break;
                        }
                        case FTDF_ASSOCIATE_REQUEST:
                        {
                                ftdf_assoc_admin_t* assoc_admin = &ftdf_aa;

                                if ((assoc_admin->fast_a == FTDF_TRUE) || (assoc_admin->data_r == FTDF_FALSE)) {
                                        uint32_t timestamp =
                                            REG_GETF(FTDF, FTDF_SYMBOLTIMESNAPSHOTVAL_REG, SYMBOLTIMESNAPSHOTVAL);

                                        REG_SETF(FTDF, FTDF_SYMBOLTIME2THR_REG, SYMBOLTIME2THR,
                                                (timestamp + ftdf_pib.response_wait_time * FTDF_BASE_SUPERFRAME_DURATION));

                                } else if (!(frame_header->options & FTDF_OPT_FRAME_PENDING)) {

                                        ftdf_send_associate_confirm((ftdf_associate_request_t*)ftdf_req_current,
                                                                    FTDF_NO_DATA,
                                                                    0xffff);
                                }
                                break;
                        }
                        case FTDF_ASSOCIATE_RESPONSE:
                        {
                                ftdf_associate_response_t* assoc_resp =
                                        (ftdf_associate_response_t*)ftdf_req_current;

                                ftdf_address_t src_addr, dst_addr;
                                src_addr.ext_address = ftdf_pib.ext_address;
                                dst_addr.ext_address = assoc_resp->device_address;

                                ftdf_send_comm_status_indication(ftdf_req_current, FTDF_SUCCESS,
                                                                 ftdf_pib.pan_id,
                                                                 FTDF_EXTENDED_ADDRESS,
                                                                 src_addr,
                                                                 FTDF_EXTENDED_ADDRESS,
                                                                 dst_addr,
                                                                 assoc_resp->security_level,
                                                                 assoc_resp->key_id_mode,
                                                                 assoc_resp->key_source,
                                                                 assoc_resp->key_index);
                                break;
                        }
                        case FTDF_ORPHAN_RESPONSE:
                        {
                                ftdf_orphan_response_t* orphan_resp =
                                        (ftdf_orphan_response_t*)ftdf_req_current;

                                ftdf_address_t src_addr, dst_addr;
                                src_addr.ext_address = ftdf_pib.ext_address;
                                dst_addr.ext_address = orphan_resp->orphan_address;

                                ftdf_send_comm_status_indication(ftdf_req_current, FTDF_SUCCESS,
                                                                 ftdf_pib.pan_id,
                                                                 FTDF_EXTENDED_ADDRESS,
                                                                 src_addr,
                                                                 FTDF_EXTENDED_ADDRESS,
                                                                 dst_addr,
                                                                 orphan_resp->security_level,
                                                                 orphan_resp->key_id_mode,
                                                                 orphan_resp->key_source,
                                                                 orphan_resp->key_index);
                                break;
                        }
                        case FTDF_DISASSOCIATE_REQUEST:
                        {
                                ftdf_send_disassociate_confirm((ftdf_disassociate_request_t*)ftdf_req_current,
                                                               FTDF_SUCCESS);
                                break;
                        }
                        case FTDF_REMOTE_REQUEST:
                        {
                                ftdf_remote_request_t* remote_request =
                                        (ftdf_remote_request_t*)ftdf_req_current;

                                if (remote_request->remote_id ==
                                        FTDF_REMOTE_PAN_ID_CONFLICT_NOTIFICATION) {

                                        ftdf_send_sync_loss_indication(FTDF_PAN_ID_CONFLICT,
                                                                       security_header);
                                }

                                ftdf_req_current = NULL;
                                break;
                        }
                        }

                        if (ftdf_req_current->msg_id != FTDF_DATA_REQUEST) {
                                /* for data request the application owns the memory */
                                FTDF_REL_DATA_BUFFER((ftdf_octet_t*)payload_ie_list);
                        }

                        ftdf_process_next_request();
                } else {
                        FTDF_REL_DATA_BUFFER((ftdf_octet_t*)payload_ie_list);
                }
        } else if (((frame_header->frame_version == FTDF_FRAME_VERSION_E) || ftdf_pib.tsch_enabled) &&
                (frame_header->options & FTDF_OPT_ACK_REQUESTED)) {

#if !defined(FTDF_NO_CSL) || !defined(FTDF_NO_TSCH)
                static ftdf_frame_header_t afh;
                ftdf_frame_header_t *ack_frame_header = &afh;

                ack_frame_header->frame_type = FTDF_ACKNOWLEDGEMENT_FRAME;
                ack_frame_header->options =
                        (frame_header->options & (FTDF_OPT_SECURITY_ENABLED | FTDF_OPT_SEQ_NR_SUPPRESSED)) |
                        FTDF_OPT_ENHANCED;

                if ((ftdf_pib.le_enabled == FTDF_TRUE) || (ftdf_pib.tsch_enabled == FTDF_TRUE) ||
                        (ftdf_pib.e_ack_ie_list.nr_of_ie != 0)) {

                        ack_frame_header->options |= FTDF_OPT_IES_PRESENT;
                }

                ack_frame_header->dst_addr_mode = FTDF_NO_ADDRESS;
                ack_frame_header->src_addr_mode = FTDF_NO_ADDRESS;
                ack_frame_header->sn = frame_header->sn;

                ftdf_octet_t *tx_ptr = (ftdf_octet_t*) &FTDF->FTDF_TX_FIFO_2_0_REG;

                /* Skip PHY header (= MAC length) */
                tx_ptr++;

                tx_ptr = ftdf_add_frame_header(tx_ptr, ack_frame_header, 0);

                if (frame_header->options & FTDF_OPT_SECURITY_ENABLED) {
                        security_header->frame_counter = ftdf_pib.frame_counter;
                        security_header->frame_counter_mode = ftdf_pib.frame_counter_mode;

                        tx_ptr = ftdf_add_security_header(tx_ptr, security_header);
                }

#ifndef FTDF_NO_CSL
                if (ftdf_pib.le_enabled == FTDF_TRUE) {
                        static ftdf_octet_t phase_and_period[4];
                        static ftdf_ie_descriptor_t cslIE = { 0x1a, 4, { phase_and_period } };
                        static ftdf_ie_list_t cslIEList = { 1, &cslIE };
                        ftdf_time_t curTime =
                            REG_GETF(FTDF, FTDF_SYMBOLTIMESNAPSHOTVAL_REG, SYMBOLTIMESNAPSHOTVAL);

                        ftdf_time_t delta =
                            curTime - (ftdf_start_csl_sample_time - ftdf_pib.csl_period * 10);

                        *(ftdf_period_t*)(phase_and_period + 0) = (ftdf_period_t)(delta / 10);
                        *(ftdf_period_t*)(phase_and_period + 2) = ftdf_pib.csl_period;

                        tx_ptr = ftdf_add_ies(tx_ptr, &cslIEList, &ftdf_pib.e_ack_ie_list, FTDF_FALSE);
                }
#endif /* FTDF_NO_CSL */

#ifndef FTDF_NO_TSCH
                if (ftdf_pib.tsch_enabled == FTDF_TRUE) {
                        ftdf_time_t rx_timestamp;

                        switch (read_buf) {
                        case 0:
                                rx_timestamp = REG_GETF(FTDF, FTDF_RX_META_0_0_REG, RX_TIMESTAMP);
                                break;
                        case 1:
                                rx_timestamp = REG_GETF(FTDF, FTDF_RX_META_0_1_REG, RX_TIMESTAMP);
                                break;
                        case 2:
                                rx_timestamp = REG_GETF(FTDF, FTDF_RX_META_0_2_REG, RX_TIMESTAMP);
                                break;
                        case 3:
                                rx_timestamp = REG_GETF(FTDF, FTDF_RX_META_0_3_REG, RX_TIMESTAMP);
                                break;
                        case 4:
                                rx_timestamp = REG_GETF(FTDF, FTDF_RX_META_0_4_REG, RX_TIMESTAMP);
                                break;
                        case 5:
                                rx_timestamp = REG_GETF(FTDF, FTDF_RX_META_0_5_REG, RX_TIMESTAMP);
                                break;
                        case 6:
                                rx_timestamp = REG_GETF(FTDF, FTDF_RX_META_0_6_REG, RX_TIMESTAMP);
                                break;
                        case 7:
                                rx_timestamp = REG_GETF(FTDF, FTDF_RX_META_0_7_REG, RX_TIMESTAMP);
                                break;
                        default:
                                /* Unsupported register index occurred */
                                OS_ASSERT(0);
                        }

                        tx_ptr = ftdf_add_corr_time_ie(tx_ptr, rx_timestamp);
                }
#endif /* FTDF_NO_TSCH */

                if (!ftdf_pib.le_enabled && !ftdf_pib.tsch_enabled) {
                        tx_ptr = ftdf_add_ies(tx_ptr, NULL, &ftdf_pib.e_ack_ie_list, FTDF_FALSE);
                }

                ftdf_send_ack_frame(frame_header, security_header, tx_ptr);
#endif /* !FTDF_NO_CSL || !FTDF_NO_TSCH */

                if (duplicate) {
                        FTDF_REL_DATA_BUFFER((ftdf_octet_t*)payload_ie_list);
                        return;
                }
        }

        if ((frame_type == FTDF_DATA_FRAME) || (frame_type == FTDF_MULTIPURPOSE_FRAME)) {
                ftdf_data_length_t payload_length =
                    frameLen - (rx_ptr - rx_buffer) + 1 - mic_length - FTDF_FCS_LENGTH;

                if (ftdf_req_current && (ftdf_req_current->msg_id == FTDF_POLL_REQUEST)) {
                        ftdf_poll_request_t *poll_request = (ftdf_poll_request_t*)ftdf_req_current;

                        if ((frame_header->src_addr_mode == poll_request->coord_addr_mode) &&
                                (frame_header->src_pan_id == poll_request->coord_pan_id) &&
                                (((frame_header->src_addr_mode == FTDF_SHORT_ADDRESS) &&
                                  (frame_header->src_addr.short_address == poll_request->coord_addr.short_address)) ||
                                 ((frame_header->src_addr_mode == FTDF_EXTENDED_ADDRESS) &&
                                  (frame_header->src_addr.ext_address =  poll_request->coord_addr.ext_address)))) {

                                if (payload_length == 0) {
                                        ftdf_send_poll_confirm(poll_request, FTDF_NO_DATA);
                                } else {
                                        ftdf_send_poll_confirm(poll_request, FTDF_SUCCESS);
                                }
                        }
                } else if (ftdf_req_current && (ftdf_req_current->msg_id == FTDF_ASSOCIATE_REQUEST) &&
                        (payload_length == 0)) {

                        send_confirm(FTDF_NO_DATA, FTDF_ASSOCIATE_REQUEST);
                }

                if (payload_length != 0) {
                        ftdf_link_quality_t mpdu_link_quality;
                        ftdf_time_t timestamp;

                        switch (read_buf) {
                        case 0:
                                mpdu_link_quality = REG_GETF(FTDF, FTDF_RX_META_1_0_REG, QUALITY_INDICATOR);
                                timestamp = REG_GETF(FTDF, FTDF_RX_META_0_0_REG, RX_TIMESTAMP);
                                break;
                        case 1:
                                mpdu_link_quality = REG_GETF(FTDF, FTDF_RX_META_1_1_REG, QUALITY_INDICATOR);
                                timestamp = REG_GETF(FTDF, FTDF_RX_META_0_1_REG, RX_TIMESTAMP);
                                break;
                        case 2:
                                mpdu_link_quality = REG_GETF(FTDF, FTDF_RX_META_1_2_REG, QUALITY_INDICATOR);
                                timestamp = REG_GETF(FTDF, FTDF_RX_META_0_2_REG, RX_TIMESTAMP);
                                break;
                        case 3:
                                mpdu_link_quality = REG_GETF(FTDF, FTDF_RX_META_1_3_REG, QUALITY_INDICATOR);
                                timestamp = REG_GETF(FTDF, FTDF_RX_META_0_3_REG, RX_TIMESTAMP);
                                break;
                        case 4:
                                mpdu_link_quality = REG_GETF(FTDF, FTDF_RX_META_1_4_REG, QUALITY_INDICATOR);
                                timestamp = REG_GETF(FTDF, FTDF_RX_META_0_4_REG, RX_TIMESTAMP);
                                break;
                        case 5:
                                mpdu_link_quality = REG_GETF(FTDF, FTDF_RX_META_1_5_REG, QUALITY_INDICATOR);
                                timestamp = REG_GETF(FTDF, FTDF_RX_META_0_5_REG, RX_TIMESTAMP);
                                break;
                        case 6:
                                mpdu_link_quality = REG_GETF(FTDF, FTDF_RX_META_1_6_REG, QUALITY_INDICATOR);
                                timestamp = REG_GETF(FTDF, FTDF_RX_META_0_6_REG, RX_TIMESTAMP);
                                break;
                        case 7:
                                mpdu_link_quality = REG_GETF(FTDF, FTDF_RX_META_1_7_REG, QUALITY_INDICATOR);
                                timestamp = REG_GETF(FTDF, FTDF_RX_META_0_7_REG, RX_TIMESTAMP);
                                break;
                        default:
                                OS_ASSERT(0);
                        }

                        ftdf_send_data_indication(frame_header,
                                                  security_header,
                                                  payload_ie_list,
                                                  payload_length,
                                                  rx_ptr,
                                                  mpdu_link_quality,
                                                  timestamp);
                }
#ifndef FTDF_NO_CSL
                else if ((header_ie_list->nr_of_ie == 1) && (header_ie_list->ie[0].id == 0x1d)) {
                        ftdf_period_t rz_time = *(uint16_t*)header_ie_list->ie[0].content.raw;

                        ftdf_critical_var();
                        ftdf_enter_critical();

                        ftdf_time_t cur_time =
                            REG_GETF(FTDF, FTDF_SYMBOLTIMESNAPSHOTVAL_REG, SYMBOLTIMESNAPSHOTVAL);

                        ftdf_rz_time = cur_time + (ftdf_time_t)rz_time * 10 + 260; /* 260 length of max frame in symbols */

                        ftdf_time_t csl_period = ftdf_pib.csl_period * 10;
                        ftdf_time_t delta = ftdf_rz_time - ftdf_start_csl_sample_time;

                        /* A delta larger than 0x80000000 is assumed a negative delta */
                        while (delta < 0x80000000) {
                                ftdf_start_csl_sample_time += csl_period;
                                delta = ftdf_rz_time - ftdf_start_csl_sample_time;
                        }

                        REG_SETF(FTDF, FTDF_LMAC_CONTROL_8_REG, MACCSLSTARTSAMPLETIME,
                                 ftdf_start_csl_sample_time);

                        ftdf_exit_critical();

                        FTDF_REL_DATA_BUFFER((ftdf_octet_t*)payload_ie_list);
                }
#endif /* FTDF_NO_CSL */
                else {
                        FTDF_REL_DATA_BUFFER((ftdf_octet_t*)payload_ie_list);
                }
        } else if (frame_type == FTDF_MAC_COMMAND_FRAME) {
                ftdf_process_command_frame(rx_ptr, frame_header, security_header, payload_ie_list);
        } else if (frame_type == FTDF_BEACON_FRAME) {
                ftdf_octet_t* superframe_spec_ptr = (ftdf_octet_t*)&pan_descriptor.superframe_spec;
                superframe_spec_ptr++;

                if (ftdf_is_pan_coordinator) {
                        if ((frame_header->src_pan_id == ftdf_pib.pan_id) && (*superframe_spec_ptr & 0x40)) {

                                FTDF_REL_DATA_BUFFER((ftdf_octet_t*)payload_ie_list);
                                ftdf_send_sync_loss_indication(FTDF_PAN_ID_CONFLICT,
                                                               security_header);

                                return;
                        }
                } else if (ftdf_pib.associated_pan_coord) {
                        if ((frame_header->src_pan_id == ftdf_pib.pan_id) && (*superframe_spec_ptr & 0x40) &&
                                (((frame_header->src_addr_mode == FTDF_SHORT_ADDRESS) &&
                                  (frame_header->src_addr.short_address != ftdf_pib.coord_short_address)) ||
                                 ((frame_header->src_addr_mode == FTDF_EXTENDED_ADDRESS) &&
                                  (frame_header->src_addr.ext_address != ftdf_pib.coord_ext_address)))) {

                                FTDF_REL_DATA_BUFFER((ftdf_octet_t*)payload_ie_list);
                                ftdf_sendpan_id_conflict_notification(frame_header, security_header);
                                return;
                        }
                }

                ftdf_data_length_t beacon_payload_length =
                    frameLen - (rx_ptr - rx_buffer) + 1- mic_length - FTDF_FCS_LENGTH;

                if ((ftdf_pib.auto_request == FTDF_FALSE) || (beacon_payload_length != 0)) {
                        ftdf_time_t timestamp;

                        switch (read_buf) {
                        case 0:
                                timestamp = REG_GETF(FTDF, FTDF_RX_META_0_0_REG, RX_TIMESTAMP);
                                break;
                        case 1:
                                timestamp = REG_GETF(FTDF, FTDF_RX_META_0_1_REG, RX_TIMESTAMP);
                                break;
                        case 2:
                                timestamp = REG_GETF(FTDF, FTDF_RX_META_0_2_REG, RX_TIMESTAMP);
                                break;
                        case 3:
                                timestamp = REG_GETF(FTDF, FTDF_RX_META_0_3_REG, RX_TIMESTAMP);
                                break;
                        case 4:
                                timestamp = REG_GETF(FTDF, FTDF_RX_META_0_4_REG, RX_TIMESTAMP);
                                break;
                        case 5:
                                timestamp = REG_GETF(FTDF, FTDF_RX_META_0_5_REG, RX_TIMESTAMP);
                                break;
                        case 6:
                                timestamp = REG_GETF(FTDF, FTDF_RX_META_0_6_REG, RX_TIMESTAMP);
                                break;
                        case 7:
                                timestamp = REG_GETF(FTDF, FTDF_RX_META_0_7_REG, RX_TIMESTAMP);
                                break;
                        default:
                                /* Unsupported register index occurred */
                                OS_ASSERT(0);
                        }

                        ftdf_beacon_notify_indication_t* beacon_notify_indication =
                                (ftdf_beacon_notify_indication_t*) FTDF_GET_MSG_BUFFER(
                                        sizeof(ftdf_beacon_notify_indication_t));

                        beacon_notify_indication->msg_id = FTDF_BEACON_NOTIFY_INDICATION;
                        beacon_notify_indication->bsn = frame_header->sn;
                        beacon_notify_indication->pan_descriptor = &pan_descriptor;
                        beacon_notify_indication->pend_addr_spec = pend_addr_spec;
                        beacon_notify_indication->addr_list = pend_addr_list;
                        beacon_notify_indication->sdu_length = beacon_payload_length;
                        beacon_notify_indication->sdu = FTDF_GET_DATA_BUFFER(beacon_payload_length);
                        beacon_notify_indication->eb_sn = frame_header->sn;
                        beacon_notify_indication->beacon_type =
                                frame_header->frame_version == FTDF_FRAME_VERSION_E ?
                                        FTDF_ENHANCED_BEACON : FTDF_NORMAL_BEACON;
                        beacon_notify_indication->ie_list = payload_ie_list;
                        beacon_notify_indication->timestamp = timestamp;

                        memcpy(beacon_notify_indication->sdu, rx_ptr, beacon_payload_length);

                        FTDF_RCV_MSG((ftdf_msg_buffer_t*)beacon_notify_indication);

                } else if (ftdf_req_current && (ftdf_req_current->msg_id == FTDF_SCAN_REQUEST)) {

                        FTDF_REL_DATA_BUFFER((ftdf_octet_t*)payload_ie_list);
                        ftdf_add_pan_descriptor(&pan_descriptor);

                } else {
                        FTDF_REL_DATA_BUFFER((ftdf_octet_t*)payload_ie_list);
                }
        }
#if FTDF_USE_LPDP
#if FTDF_FP_BIT_TEST_MODE
        if (ftdf_lpdp_is_enabled() && !ftdf_req_current && (frame_type == FTDF_DATA_FRAME)) {
                ftdf_process_tx_pending(frame_header, security_header);
        }
#else /* FTDF_FP_BIT_TEST_MODE */
        if (!ftdf_req_current && frame_type == FTDF_DATA_FRAME) {
                ftdf_process_tx_pending(frame_header, security_header);
        }
#endif /* FTDF_FP_BIT_TEST_MODE */
#endif /* FTDF_USE_LPDP */
#ifndef FTDF_NO_TSCH
        if (ftdf_pib.tsch_enabled == FTDF_TRUE) {
                ftdf_schedule_tsch(NULL);
        }
#endif /* FTDF_NO_TSCH */
#endif /* !FTDF_LITE */
}

void ftdf_process_rx_event(void)
{
        if (REG_GETF(FTDF, FTDF_RX_EVENT_REG, RXSOF_E)) {
#ifdef SIMULATOR
                REG_CLR_BIT(FTDF, FTDF_RX_EVENT_REG, RXSOF_E);
#else
                FTDF->FTDF_RX_EVENT_REG = REG_MSK(FTDF, FTDF_RX_EVENT_REG, RXSOF_E);
#endif
        }

        if (REG_GETF(FTDF, FTDF_RX_EVENT_REG, RXBYTE_E)) {
#ifdef SIMULATOR
                REG_CLR_BIT(FTDF, FTDF_RX_EVENT_REG, RXBYTE_E);
#else
                FTDF->FTDF_RX_EVENT_REG = REG_MSK(FTDF, FTDF_RX_EVENT_REG, RXBYTE_E);
#endif
        }

        if (REG_GETF(FTDF, FTDF_RX_EVENT_REG, RX_OVERFLOW_E)) {
                // No API defined to report this error to the higher layer, so just clear it.
#ifdef SIMULATOR
                REG_CLR_BIT(FTDF, FTDF_RX_EVENT_REG, RX_OVERFLOW_E);
#else
                FTDF->FTDF_RX_EVENT_REG = REG_MSK(FTDF, FTDF_RX_EVENT_REG, RX_OVERFLOW_E);
#endif
        }

        if (REG_GETF(FTDF, FTDF_RX_EVENT_REG, RX_BUF_AVAIL_E)) {
                int read_buf = REG_GETF(FTDF, FTDF_RX_CONTROL_0_REG, RX_READ_BUF_PTR);
                int write_buf = REG_GETF(FTDF, FTDF_RX_STATUS_REG, RX_WRITE_BUF_PTR);

                while (read_buf != write_buf) {
                        process_rx_frame(read_buf % 8);
                        read_buf = (read_buf + 1) % 16;
                }

                REG_SETF(FTDF, FTDF_RX_CONTROL_0_REG, RX_READ_BUF_PTR, read_buf);

#ifdef SIMULATOR
                REG_CLR_BIT(FTDF, FTDF_RX_EVENT_REG, RX_BUF_AVAIL_E);
#else
                FTDF->FTDF_RX_EVENT_REG = REG_MSK(FTDF, FTDF_RX_EVENT_REG, RX_BUF_AVAIL_E);
#endif
        }

        if (REG_GETF(FTDF, FTDF_LMAC_EVENT_REG, EDSCANREADY_E)) {
#ifdef SIMULATOR
                REG_CLR_BIT(FTDF, FTDF_LMAC_EVENT_REG, EDSCANREADY_E);
#else
                FTDF->FTDF_LMAC_EVENT_REG = REG_MSK(FTDF, FTDF_LMAC_EVENT_REG, EDSCANREADY_E);
#endif

#ifndef FTDF_LITE
                ftdf_msg_buffer_t *request = ftdf_req_current;

                if (request->msg_id == FTDF_SCAN_REQUEST) {
                        ftdf_scan_ready((ftdf_scan_request_t*)request);
                }
#endif /* !FTDF_LITE */
        }

        if (REG_GETF(FTDF, FTDF_LMAC_EVENT_REG, RXTIMEREXPIRED_E)) {
#ifdef SIMULATOR
                REG_CLR_BIT(FTDF, FTDF_LMAC_EVENT_REG, RXTIMEREXPIRED_E);
#else
                FTDF->FTDF_LMAC_EVENT_REG = REG_MSK(FTDF, FTDF_LMAC_EVENT_REG, RXTIMEREXPIRED_E);
#endif

        if ( ftdf_pib.metrics_enabled ) {
                ftdf_pib.performance_metrics.rx_expired_count++;
        }
#ifndef FTDF_LITE
#ifndef FTDF_NO_TSCH
                if (ftdf_pib.tsch_enabled) {
                        ftdf_schedule_tsch(NULL);
                } else
#endif /* FTDF_NO_TSCH */
                if (ftdf_req_current) {
                        ftdf_msg_id_t msg_id = ftdf_req_current->msg_id;

                        if (msg_id == FTDF_POLL_REQUEST) {
                                ftdf_send_poll_confirm((ftdf_poll_request_t*)ftdf_req_current,
                                                        FTDF_NO_DATA);
                        } else if (msg_id == FTDF_SCAN_REQUEST) {
                                ftdf_scan_ready((ftdf_scan_request_t*)ftdf_req_current);
                        } else if (msg_id == FTDF_ASSOCIATE_REQUEST) {
                                ftdf_send_associate_confirm((ftdf_associate_request_t*)ftdf_req_current,
                                                            FTDF_NO_DATA,
                                                            0xffff);
                        }
                }
#endif /* !FTDF_LITE */
        }

        if (REG_GETF(FTDF, FTDF_LMAC_EVENT_REG, CSMA_CA_BO_THR_E)) {
#ifdef SIMULATOR
                REG_CLEAR_BIT(FTDF, FTDF_LMAC_EVENT_REG, CSMA_CA_BO_THR_E);
#else
                FTDF->FTDF_LMAC_EVENT_REG = REG_MSK(FTDF, FTDF_LMAC_EVENT_REG, CSMA_CA_BO_THR_E);

#if FTDF_USE_SLEEP_DURING_BACKOFF
                if (ftdf_pib.metrics_enabled) {
                        ftdf_pib.performance_metrics.bo_irq_count++;
                }
                ftdf_sdb_fsm_backoff_irq();
#endif /* FTDF_USE_SLEEP_DURING_BACKOFF */
        }
#endif
}

static void send_confirm(ftdf_status_t status, ftdf_msg_id_t msg_id)
{

        switch (msg_id) {
#ifndef FTDF_LITE
        case FTDF_DATA_REQUEST:
        {
                ftdf_data_request_t* data_request = (ftdf_data_request_t*)ftdf_req_current;

                ftdf_time_t timestamp = REG_GETF(FTDF, FTDF_TX_RETURN_STATUS_0_0_REG, TXTIMESTAMP);
                ftdf_sn_t sn = REG_GETF(FTDF, FTDF_TX_META_DATA_1_0_REG, MACSN);
                ftdf_num_of_backoffs_t num_of_backoffs = REG_GETF(FTDF, FTDF_TX_RETURN_STATUS_1_0_REG, CSMACANRRETRIES);

                ftdf_send_data_confirm(data_request, status, timestamp, sn, num_of_backoffs, NULL);

                break;
        }

        case FTDF_POLL_REQUEST:
                if (status != FTDF_SUCCESS) {
                        ftdf_send_poll_confirm((ftdf_poll_request_t*)ftdf_req_current, status);
                }

                break;
        case FTDF_ASSOCIATE_REQUEST:
                if (status != FTDF_SUCCESS) {
                        ftdf_send_associate_confirm((ftdf_associate_request_t*)ftdf_req_current,
                                                    status,
                                                    0xffff);
                }

                break;
        case FTDF_ASSOCIATE_RESPONSE:
                if (status != FTDF_SUCCESS) {
                        ftdf_associate_response_t *assoc_resp =
                                (ftdf_associate_response_t*)ftdf_req_current;

                        ftdf_address_t src_addr, dst_addr;
                        src_addr.ext_address = ftdf_pib.ext_address;
                        dst_addr.ext_address = assoc_resp->device_address;

                        ftdf_send_comm_status_indication(ftdf_req_current, status,
                                                         ftdf_pib.pan_id,
                                                         FTDF_EXTENDED_ADDRESS,
                                                         src_addr,
                                                         FTDF_EXTENDED_ADDRESS,
                                                         dst_addr,
                                                         assoc_resp->security_level,
                                                         assoc_resp->key_id_mode,
                                                         assoc_resp->key_source,
                                                         assoc_resp->key_index);
                }

                break;
        case FTDF_ORPHAN_RESPONSE:
                if (status != FTDF_SUCCESS) {
                        ftdf_orphan_response_t *orphan_resp =
                                (ftdf_orphan_response_t*)ftdf_req_current;

                        ftdf_address_t src_addr, dst_addr;
                        src_addr.ext_address = ftdf_pib.ext_address;
                        dst_addr.ext_address = orphan_resp->orphan_address;

                        ftdf_send_comm_status_indication(ftdf_req_current, status,
                                                         ftdf_pib.pan_id,
                                                         FTDF_EXTENDED_ADDRESS,
                                                         src_addr,
                                                         FTDF_EXTENDED_ADDRESS,
                                                         dst_addr,
                                                         orphan_resp->security_level,
                                                         orphan_resp->key_id_mode,
                                                         orphan_resp->key_source,
                                                         orphan_resp->key_index);
                }

                break;
        case FTDF_DISASSOCIATE_REQUEST:
                if (status != FTDF_SUCCESS) {
                        ftdf_send_disassociate_confirm(
                                (ftdf_disassociate_request_t*)ftdf_req_current, status);
                }

                break;
        case FTDF_SCAN_REQUEST:
                if (status != FTDF_SUCCESS) {
                        ftdf_send_scan_confirm((ftdf_scan_request_t*)ftdf_req_current, status);
                }

                break;
        case FTDF_BEACON_REQUEST:
                ftdf_send_beacon_confirm((ftdf_beacon_request_t*)ftdf_req_current, status);
                break;
        case FTDF_REMOTE_REQUEST:
                ftdf_req_current = NULL;
                break;
#endif /* !FTDF_LITE */
        case FTDF_TRANSPARENT_REQUEST:
        {
#ifndef FTDF_PHY_API
                ftdf_transparent_request_t *transparent_request =
                        (ftdf_transparent_request_t*)ftdf_req_current;
#endif
                ftdf_bitmap32_t transparent_status = 0;

                switch (status) {
                case FTDF_SUCCESS:
                        transparent_status = FTDF_TRANSPARENT_SEND_SUCCESSFUL;
                        break;
                case FTDF_CHANNEL_ACCESS_FAILURE:
                        transparent_status = FTDF_TRANSPARENT_CSMACA_FAILURE;
                        break;
#if FTDF_TRANSPARENT_WAIT_FOR_ACK
                case FTDF_NO_ACK:
                        transparent_status = FTDF_TRANSPARENT_NO_ACK;
                        break;
#endif
                }

                if (ftdf_pib.metrics_enabled) {
                        ftdf_pib.performance_metrics.tx_success_count++;
                }
#ifdef FTDF_PHY_API
                ftdf_critical_var();
                ftdf_enter_critical();
                ftdf_tx_in_progress = FTDF_FALSE;
                ftdf_exit_critical();

                FTDF_SEND_FRAME_TRANSPARENT_CONFIRM(NULL, transparent_status);
#else

                ftdf_req_current = NULL;

                FTDF_SEND_FRAME_TRANSPARENT_CONFIRM(transparent_request->handle, transparent_status);

                FTDF_REL_MSG_BUFFER((ftdf_msg_buffer_t*)transparent_request);
#ifndef FTDF_LITE
                ftdf_process_next_request();
#endif /* !FTDF_LITE */

#endif /* FTDF_PHY_API */
                break;
        }
        }
}

void ftdf_process_tx_event(void)
{
        volatile uint32_t *tx_flag_state;
        ftdf_status_t status = FTDF_SUCCESS;

#if dg_configCOEX_ENABLE_CONFIG && !FTDF_USE_AUTO_PTI
        /* Restore Rx PTI in case the Tx transaction that just ended interrupted an Rx-always-on
         * transaction. */
        hw_coex_pti_t tx_pti;
        hw_coex_update_ftdf_pti(ftdf_get_rx_pti(), &tx_pti, true);
#endif

        if (REG_GETF(FTDF, FTDF_TX_FLAG_CLEAR_E_1_REG, TX_FLAG_CLEAR_E)) {
#ifdef SIMULATOR
                REG_CLR_BIT(FTDF, FTDF_TX_FLAG_CLEAR_E_1_REG, TX_FLAG_CLEAR_E);
#else
                FTDF->FTDF_TX_FLAG_CLEAR_E_1_REG = REG_MSK(FTDF, FTDF_TX_FLAG_CLEAR_E_1_REG, TX_FLAG_CLEAR_E);
#endif

                if (REG_GETF(FTDF, FTDF_TX_RETURN_STATUS_1_1_REG, CSMACAFAIL)) {
                        if (ftdf_pib.metrics_enabled) {
                                ftdf_pib.performance_metrics.tx_fail_count++;
                        }

                        status = FTDF_CHANNEL_ACCESS_FAILURE;
                }
        }

        if (REG_GETF(FTDF, FTDF_TX_FLAG_CLEAR_E_0_REG, TX_FLAG_CLEAR_E)) {
#ifdef SIMULATOR
                REG_CLR_BIT(FTDF, FTDF_TX_FLAG_CLEAR_E_0_REG, TX_FLAG_CLEAR_E);
#else
                FTDF->FTDF_TX_FLAG_CLEAR_E_0_REG = REG_MSK(FTDF, FTDF_TX_FLAG_CLEAR_E_0_REG, TX_FLAG_CLEAR_E);
#endif
        } else {
                return;
        }

#ifndef FTDF_PHY_API
        ftdf_tx_in_progress = FTDF_FALSE;

        if (ftdf_req_current == NULL) {
                return;
        }
#else
        ftdf_critical_var();
        ftdf_enter_critical();

        if (ftdf_tx_in_progress == FTDF_FALSE) {
                ftdf_exit_critical();

                return;
        }

        ftdf_exit_critical();
#endif
#if FTDF_USE_SLEEP_DURING_BACKOFF
        ftdf_sdb_fsm_tx_irq();
#endif /* FTDF_USE_SLEEP_DURING_BACKOFF */
        ftdf_boolean_t ack_tx = REG_GETF(FTDF, FTDF_TX_META_DATA_0_0_REG, ACKREQUEST);

        if (status == FTDF_SUCCESS) {
                ftdf_frame_type_t frame_type = REG_GETF(FTDF, FTDF_TX_META_DATA_0_0_REG, FRAMETYPE);

                switch (frame_type) {
                case FTDF_BEACON_FRAME:
                        ftdf_pib.traffic_counters.tx_ceacon_frm_cnt++;
                        break;
                case FTDF_DATA_FRAME:
                        ftdf_pib.traffic_counters.tx_data_frm_cnt++;
                        break;
                case FTDF_MAC_COMMAND_FRAME:
                        ftdf_pib.traffic_counters.tx_cmd_frm_cnt++;
                        break;
                case FTDF_MULTIPURPOSE_FRAME:
                        ftdf_pib.traffic_counters.tx_multi_purp_frm_cnt++;
                        break;
                }

#ifndef FTDF_LITE
#ifndef FTDF_NO_TSCH
                ftdf_tsch_retry_t *tsch_retry = NULL;

                if (ftdf_pib.tsch_enabled) {
                        tsch_retry = ftdf_get_tsch_retry(ftdf_get_request_address(ftdf_req_current));
                }
#endif /* FTDF_NO_TSCH */
#endif /* !FTDF_LITE */
                if (REG_GETF(FTDF, FTDF_TX_RETURN_STATUS_1_0_REG, ACKFAIL)) {
#ifndef FTDF_LITE
#ifndef FTDF_NO_TSCH
                        if (ftdf_pib.tsch_enabled) {
                                tsch_retry->nr_of_retries++;
                                ftdf_tsch_slot_link->request = NULL;
                                status = ftdf_schedule_tsch(ftdf_req_current);

                                if (status == FTDF_SUCCESS) {
                                        /* If ftdf_req_current is not equal to NULL the retried
                                         * request will be queued rather then send again */
                                        ftdf_req_current = NULL;
                                }

                        } else
#endif /* FTDF_NO_TSCH */
#endif /* !FTDF_LITE */

                        if (ftdf_nr_of_retries < ftdf_pib.max_frame_retries) {
                                ftdf_nr_of_retries++;
#ifndef FTDF_LITE
#ifndef FTDF_NO_CSL
                        if (ftdf_pib.le_enabled) {
                                ftdf_set_peer_csl_timing(NULL, 0);

                                ftdf_critical_var();
                                ftdf_enter_critical();

                                ftdf_time_t cur_time =
                                        REG_GETF(FTDF, FTDF_SYMBOLTIMESNAPSHOTVAL_REG, SYMBOLTIMESNAPSHOTVAL);

                                ftdf_tx_in_progress = FTDF_TRUE;
                                REG_SETF(FTDF, FTDF_LMAC_CONTROL_8_REG, MACCSLSTARTSAMPLETIME,
                                         cur_time + 5);
                                REG_SETF(FTDF, FTDF_LMAC_CONTROL_7_REG, MACWUPERIOD,
                                         ftdf_pib.csl_max_period);

                                REG_SETF(FTDF, FTDF_TX_SET_OS_REG, TX_FLAG_SET,
                                         ((1 << FTDF_TX_DATA_BUFFER) | (1 << FTDF_TX_WAKEUP_BUFFER)));

                                ftdf_exit_critical();

                                } else
#endif /* FTDF_NO_CSL */
#endif /* !FTDF_LITE */
                                {
#if dg_configCOEX_ENABLE_CONFIG && !FTDF_USE_AUTO_PTI
                                        hw_coex_update_ftdf_pti(tx_pti, NULL, true);
#endif
                                        REG_SETF(FTDF, FTDF_TX_SET_OS_REG, TX_FLAG_SET, (1 << FTDF_TX_DATA_BUFFER));
                                }

                                return;

                        } else {

                                if (ftdf_pib.metrics_enabled) {
                                        ftdf_pib.performance_metrics.tx_fail_count++;
                                }

                                status = FTDF_NO_ACK;
                        }

                } else if (REG_GETF(FTDF, FTDF_TX_RETURN_STATUS_1_0_REG, CSMACAFAIL)) {
#ifndef FTDF_LITE
#ifndef FTDF_NO_TSCH
                        if (ftdf_pib.tsch_enabled) {
                                tsch_retry->nr_of_cca_retries++;

                                if (tsch_retry->nr_of_cca_retries < ftdf_pib.max_csma_backoffs) {

                                        ftdf_tsch_slot_link->request = NULL;
                                        status = ftdf_schedule_tsch(ftdf_req_current);

                                        if (status == FTDF_SUCCESS) {
                                                /* If ftdf_req_current is not equal to NULL the
                                                 * retried request will be queued rather then send again */
                                                ftdf_req_current = NULL;
                                        }

                                } else {
                                        status = FTDF_CHANNEL_ACCESS_FAILURE;
                                }
                        } else
#endif /* FTDF_NO_TSCH */
#endif /* !FTDF_LITE */
                        {
                                status = FTDF_CHANNEL_ACCESS_FAILURE;
                        }

                        if (ftdf_pib.metrics_enabled && status != FTDF_SUCCESS) {
                                ftdf_pib.performance_metrics.tx_fail_count++;
                        }

                } else {

                        if (ack_tx == FTDF_FALSE && ftdf_pib.metrics_enabled) {
                                ftdf_pib.performance_metrics.tx_success_count++;
                        }

#ifndef FTDF_LITE
#ifndef FTDF_NO_TSCH
                        if (ftdf_pib.tsch_enabled) {
                                tsch_retry->nr_of_cca_retries = 0;
                        }
#endif /* FTDF_NO_TSCH */
#endif /* !FTDF_LITE */
                }
        }

#ifndef FTDF_PHY_API
        if (((ack_tx == FTDF_FALSE) || (status != FTDF_SUCCESS)) && ftdf_req_current) {
                send_confirm(status,  ftdf_req_current->msg_id);
#ifndef FTDF_LITE
                ftdf_process_next_request();
#endif /* !FTDF_LITE */
        }

#else
        if (ftdf_tx_in_progress) {
                send_confirm(status, FTDF_TRANSPARENT_REQUEST);
        }
#endif
}

void ftdf_process_symbol_timer_event(void)
{
#ifdef FTDF_PHY_API
        if (REG_GETF(FTDF, FTDF_LMAC_CONTROL_DELTA_REG, LMACREADY4SLEEP_D)) {
                FTDF->FTDF_LMAC_CONTROL_DELTA_REG = REG_MSK(FTDF, FTDF_LMAC_CONTROL_DELTA_REG, LMACREADY4SLEEP_D);

                /* If lmac ready 4 sleep, call respective callback, after disabling the interrupt */
                if (REG_GETF(FTDF, FTDF_LMAC_CONTROL_STATUS_REG, LMACREADY4SLEEP) == 1) {
                        REG_CLR_BIT(FTDF, FTDF_LMAC_CONTROL_MASK_REG, LMACREADY4SLEEP_M);
                        FTDF_LMACREADY4SLEEP_CB(FTDF_TRUE, 0);
                }
        }
#endif

        /* sync timestamp event */
        if (REG_GETF(FTDF, FTDF_LMAC_CONTROL_DELTA_REG, SYNCTIMESTAMP_E)) {
#ifdef SIMULATOR
                REG_CLR_BIT(FTDF, FTDF_LMAC_CONTROL_DELTA_REG, SYNCTIMESTAMP_E);
#else
                FTDF->FTDF_LMAC_CONTROL_DELTA_REG = REG_MSK(FTDF, FTDF_LMAC_CONTROL_DELTA_REG, SYNCTIMESTAMP_E);
#endif

                REG_SETF(FTDF, FTDF_TIMER_CONTROL_1_REG, SYNCTIMESTAMPENA, 0);
#ifndef FTDF_LITE
#ifndef FTDF_NO_CSL
                ftdf_old_le_enabled = FTDF_FALSE;

                if (ftdf_wake_up_enable_le) {
                        ftdf_pib.le_enabled = FTDF_TRUE;
                        ftdf_set_le_enabled();
                        ftdf_wake_up_enable_le = FTDF_FALSE;
                }
#endif /* FTDF_NO_CSL */

#ifndef FTDF_NO_TSCH
                if (ftdf_wake_up_enable_tsch) {
                        ftdf_set_tsch_enabled();
                }
#endif /* FTDF_NO_TSCH */

                ftdf_restore_tx_pending_timer();
#endif /* !FTDF_LITE */
                FTDF_WAKE_UP_READY();
        }

        /* miscellaneous event
         * - Non-TSCH mode: association timer
         * - TSCH mode: next active link timer */
        if (REG_GETF(FTDF, FTDF_LMAC_CONTROL_DELTA_REG, SYMBOLTIME2THR_E)) {
#ifdef SIMULATOR
                REG_CLR_BIT(FTDF, FTDF_LMAC_CONTROL_DELTA_REG, SYMBOLTIME2THR_E);
#else
                FTDF->FTDF_LMAC_CONTROL_DELTA_REG  = REG_MSK(FTDF, FTDF_LMAC_CONTROL_DELTA_REG, SYMBOLTIME2THR_E);
#endif

#ifndef FTDF_LITE
#ifndef FTDF_NO_TSCH
                if (ftdf_pib.tsch_enabled) {
                        ftdf_tsch_process_request();
                } else
#endif /* FTDF_NO_TSCH */
                if (ftdf_req_current && (ftdf_req_current->msg_id == FTDF_ASSOCIATE_REQUEST)) {
                        ftdf_assoc_admin_t* assoc_admin = &ftdf_aa;

                        /* macResponseWaitTime expired */
                        assoc_admin->data_r = FTDF_TRUE;

                        ftdf_send_associate_data_request();
                }
#endif /* !FTDF_LITE */
        }

        /* pending queue symbol timer event */
        if (REG_GETF(FTDF, FTDF_LMAC_CONTROL_DELTA_REG, SYMBOLTIMETHR_E)) {
#ifdef SIMULATOR
                REG_CLR_BIT(FTDF, FTDF_LMAC_CONTROL_DELTA_REG, SYMBOLTIMETHR_E);
#else
                FTDF->FTDF_LMAC_CONTROL_DELTA_REG = REG_MSK(FTDF, FTDF_LMAC_CONTROL_DELTA_REG, SYMBOLTIMETHR_E);
#endif

#ifndef FTDF_LITE
                ftdf_remove_tx_pending_timer(NULL);
#endif /* !FTDF_LITE */
        }
}

#ifndef FTDF_LITE
ftdf_status_t ftdf_send_frame(ftdf_channel_number_t channel,
                              ftdf_frame_header_t *frame_header,
                              ftdf_security_header *security_header,
                              ftdf_octet_t *tx_ptr,
                              ftdf_data_length_t payload_size,
                              ftdf_octet_t *payload)
{
        ftdf_octet_t *tx_buf_ptr = (ftdf_octet_t*) &FTDF->FTDF_TX_FIFO_0_0_REG;

        ftdf_data_length_t mic_length = ftdf_get_mic_length(security_header->security_level);
        ftdf_data_length_t phy_payload_size =
            (tx_ptr - tx_buf_ptr) - 1 + payload_size + mic_length + FTDF_FCS_LENGTH;

        if (phy_payload_size > (FTDF_BUFFER_LENGTH - 1)) {
                return FTDF_FRAME_TOO_LONG;
        }

        *tx_buf_ptr = phy_payload_size;

        ftdf_octet_t* priv_ptr = tx_ptr;

        int n;

        for (n = 0; n < payload_size; n++) {
                *tx_ptr++ = *payload++;
        }

        ftdf_status_t status = ftdf_secure_frame(tx_buf_ptr, priv_ptr, frame_header, security_header);

        if (status != FTDF_SUCCESS) {
                return status;
        }

        ftdf_bitmap8_t options = frame_header->options;

        uint32_t meta_data_0 = 0;

        REG_SET_FIELD(FTDF, FTDF_TX_META_DATA_0_0_REG, FRAME_LENGTH, meta_data_0, phy_payload_size);
        REG_SET_FIELD(FTDF, FTDF_TX_META_DATA_0_0_REG, PHYATTR_DEM_PTI, meta_data_0, ((ftdf_pib.cca_mode & 0x3) | 0x08));
        REG_SET_FIELD(FTDF, FTDF_TX_META_DATA_0_0_REG, PHYATTR_CN, meta_data_0, (channel - 11));
        REG_SET_FIELD(FTDF, FTDF_TX_META_DATA_0_0_REG, PHYATTR_RF_GPIO_PINS, meta_data_0, (ftdf_pib.tx_power & 0x07));
        REG_SET_FIELD(FTDF, FTDF_TX_META_DATA_0_0_REG, FRAMETYPE, meta_data_0, frame_header->frame_type);

        REG_SET_FIELD(FTDF, FTDF_TX_META_DATA_0_0_REG, CSMACA_ENA, meta_data_0, 1);
        REG_SET_FIELD(FTDF, FTDF_TX_META_DATA_0_0_REG, ACKREQUEST, meta_data_0,
                      ((options & FTDF_OPT_ACK_REQUESTED) ? 1 : 0));
        REG_SET_FIELD(FTDF, FTDF_TX_META_DATA_0_0_REG, CRC16_ENA, meta_data_0, 1);

        FTDF->FTDF_TX_META_DATA_0_0_REG = meta_data_0;

        uint32_t meta_data_1 = 0;
        REG_SET_FIELD(FTDF, FTDF_TX_META_DATA_1_0_REG, MACSN, meta_data_1, frame_header->sn);
        FTDF->FTDF_TX_META_DATA_1_0_REG = meta_data_1;

        uint32_t tmp = FTDF->FTDF_LMAC_CONTROL_5_REG;
        REG_SET_FIELD(FTDF, FTDF_LMAC_CONTROL_5_REG, PHYCSMACAATTR_DEM_PTI, tmp, (ftdf_pib.cca_mode & 0x3));
        REG_SET_FIELD(FTDF, FTDF_LMAC_CONTROL_5_REG, PHYCSMACAATTR_CN, tmp, (channel - 11));
        REG_SET_FIELD(FTDF, FTDF_LMAC_CONTROL_5_REG, PHYCSMACAATTR_CALCAP, tmp, 0);
        REG_SET_FIELD(FTDF, FTDF_LMAC_CONTROL_5_REG, PHYCSMACAATTR_RF_GPIO_PINS, tmp, 0);
        REG_SET_FIELD(FTDF, FTDF_LMAC_CONTROL_5_REG, PHYCSMACAATTR_HSI, tmp, 0);
        FTDF->FTDF_LMAC_CONTROL_5_REG = tmp;

#ifndef FTDF_NO_CSL
        if (ftdf_pib.le_enabled == FTDF_TRUE) {
                if (frame_header->dst_addr_mode != FTDF_SHORT_ADDRESS) {
                        return FTDF_NO_SHORT_ADDRESS;
                }

                /* Clear CSMACA of data frame buffer */
                REG_SETF(FTDF, FTDF_TX_META_DATA_0_0_REG, CSMACA_ENA, 0);

                tx_ptr = tx_buf_ptr = (ftdf_octet_t*) &FTDF->FTDF_TX_FIFO_1_0_REG;

                *tx_ptr++ = 0x0d;
                *tx_ptr++ = 0x2d;
                *tx_ptr++ = 0x81;
                *tx_ptr++ = frame_header->sn;
                *(ftdf_pan_id_t*)tx_ptr = frame_header->dst_pan_id;
                tx_ptr += 2;
                *(ftdf_short_address_t*)tx_ptr = frame_header->dst_addr.short_address;
                tx_ptr += 2;
                *tx_ptr++ = 0x82;
                *tx_ptr++ = 0x0e;

                meta_data_0 = 0;
                REG_SET_FIELD(FTDF, FTDF_TX_META_DATA_0_1_REG, FRAME_LENGTH, meta_data_0, 0x0d);
                REG_SET_FIELD(FTDF, FTDF_TX_META_DATA_0_1_REG, PHYATTR_DEM_PTI, meta_data_0, ((ftdf_pib.cca_mode & 0x3) | 0x08));
                REG_SET_FIELD(FTDF, FTDF_TX_META_DATA_0_1_REG, PHYATTR_CN, meta_data_0, (channel - 11));
                REG_SET_FIELD(FTDF, FTDF_TX_META_DATA_0_1_REG, PHYATTR_RF_GPIO_PINS, meta_data_0, (ftdf_pib.tx_power & 0x07));
                REG_SET_FIELD(FTDF, FTDF_TX_META_DATA_0_1_REG, FRAMETYPE, meta_data_0, FTDF_MULTIPURPOSE_FRAME);
                REG_SET_FIELD(FTDF, FTDF_TX_META_DATA_0_1_REG, CSMACA_ENA, meta_data_0, 1);
                REG_SET_FIELD(FTDF, FTDF_TX_META_DATA_0_1_REG, CRC16_ENA, meta_data_0, 1);
                FTDF->FTDF_TX_META_DATA_0_1_REG = meta_data_0;

                uint32_t meta_data_1 = 0;
                REG_SET_FIELD(FTDF, FTDF_TX_META_DATA_1_1_REG, MACSN, meta_data_1, frame_header->sn);
                FTDF->FTDF_TX_META_DATA_1_1_REG = meta_data_1;

#if FTDF_USE_AUTO_PTI
                FTDF->FTDF_TX_PRIORITY_1_REG |= REG_MSK(FTDF, FTDF_TX_PRIORITY_1_REG, ISWAKEUP);
#else
                FTDF->FTDF_TX_PRIORITY_1_REG = REG_MSK(FTDF, FTDF_TX_PRIORITY_1_REG, ISWAKEUP);
#endif
        }
#endif /* FTDF_NO_CSL */

#ifndef FTDF_NO_CSL
        if (ftdf_pib.le_enabled == FTDF_TRUE) {
                ftdf_time_t cur_time =
                    REG_GETF(FTDF, FTDF_SYMBOLTIMESNAPSHOTVAL_REG, SYMBOLTIMESNAPSHOTVAL);
                ftdf_time_t delta = cur_time - ftdf_rz_time;

                if (delta > 0x80000000) {

                        /* Receiving an wakeup frame sequence, delay sending until RZ has passed. */
                        ftdf_send_frame_pending = frame_header->dst_addr.short_address;

                } else {

                        ftdf_time_t wakeup_start_time;
                        ftdf_period_t wakeup_period;

                        ftdf_critical_var();
                        ftdf_enter_critical();

                        ftdf_get_wakeup_params(frame_header->dst_addr.short_address,
                                               &wakeup_start_time, &wakeup_period);

                        ftdf_tx_in_progress = FTDF_TRUE;
                        REG_SETF(FTDF, FTDF_LMAC_CONTROL_8_REG, MACCSLSTARTSAMPLETIME,
                                wakeup_start_time);
                        REG_SETF(FTDF, FTDF_LMAC_CONTROL_7_REG, MACWUPERIOD, wakeup_period);

                        REG_SETF(FTDF, FTDF_TX_SET_OS_REG, TX_FLAG_SET,
                                ((1 << FTDF_TX_DATA_BUFFER) | (1 << FTDF_TX_WAKEUP_BUFFER)));

                        ftdf_exit_critical();
                }

        } else
#endif /* FTDF_NO_CSL */
        if (!ftdf_pib.tsch_enabled) {
                REG_SETF(FTDF, FTDF_TX_SET_OS_REG, TX_FLAG_SET, (1 << FTDF_TX_DATA_BUFFER));
        }

        return FTDF_SUCCESS;
}

#if !defined(FTDF_NO_CSL) || !defined(FTDF_NO_TSCH)
ftdf_status_t ftdf_send_ack_frame(ftdf_frame_header_t *frame_header,
                                  ftdf_security_header *security_header,
                                  ftdf_octet_t *tx_ptr)
{
        ftdf_octet_t *tx_buf_ptr = ((ftdf_octet_t*) &FTDF->FTDF_TX_FIFO_2_0_REG);
        ftdf_data_length_t mic_length = ftdf_get_mic_length(security_header->security_level);
        ftdf_data_length_t phy_payload_size =
            (tx_ptr - tx_buf_ptr) - 1 + mic_length + FTDF_FCS_LENGTH;

        *tx_buf_ptr = phy_payload_size;

        ftdf_status_t status = ftdf_secure_frame(tx_buf_ptr,
                                                 tx_ptr,
                                                 frame_header,
                                                 security_header);

        if (status != FTDF_SUCCESS) {
                return status;
        }

        uint32_t meta_data_0 = 0;

        REG_SET_FIELD(FTDF, FTDF_TX_META_DATA_0_2_REG, FRAME_LENGTH, meta_data_0, phy_payload_size);
        REG_SET_FIELD(FTDF, FTDF_TX_META_DATA_0_2_REG, PHYATTR_DEM_PTI, meta_data_0, ((ftdf_pib.cca_mode & 0x3) | 0x08));
        REG_SET_FIELD(FTDF, FTDF_TX_META_DATA_0_2_REG, PHYATTR_CN, meta_data_0, REG_GETF(FTDF, FTDF_LMAC_CONTROL_1_REG, PHYRXATTR_CN));
        REG_SET_FIELD(FTDF, FTDF_TX_META_DATA_0_2_REG, PHYATTR_RF_GPIO_PINS, meta_data_0, (ftdf_pib.tx_power & 0x07));
        REG_SET_FIELD(FTDF, FTDF_TX_META_DATA_0_2_REG, FRAMETYPE, meta_data_0, FTDF_ACKNOWLEDGEMENT_FRAME);
        REG_SET_FIELD(FTDF, FTDF_TX_META_DATA_0_2_REG, CRC16_ENA, meta_data_0, 1);

        FTDF->FTDF_TX_META_DATA_0_2_REG = meta_data_0;

        uint32_t meta_data_1 = 0;
        REG_SET_FIELD(FTDF, FTDF_TX_META_DATA_1_2_REG, MACSN, meta_data_1, frame_header->sn);
        FTDF->FTDF_TX_META_DATA_1_2_REG = meta_data_1;

#if FTDF_USE_AUTO_PTI
        REG_SETF(FTDF, FTDF_TX_PRIORITY_2_REG, TX_PRIORITY, 1);
#else
        FTDF->FTDF_TX_PRIORITY_2_REG = 1;
#endif

#ifndef FTDF_NO_TSCH
        if (ftdf_pib.tsch_enabled) {

                ftdf_critical_var();
                ftdf_enter_critical();

                ftdf_period_t tx_ack_delay_val =
                    REG_GETF(FTDF, FTDF_MACTSTXACKDELAYVAL_REG, MACTSTXACKDELAYVAL);

                if (tx_ack_delay_val > 20) {
                        REG_SETF(FTDF, FTDF_TX_SET_OS_REG, TX_FLAG_SET, (1 << FTDF_TX_ACK_BUFFER));
                }

                ftdf_exit_critical();

        } else
        #endif /* FTDF_NO_TSCH */
        {
                REG_SETF(FTDF, FTDF_TX_SET_OS_REG, TX_FLAG_SET, (1 << FTDF_TX_ACK_BUFFER));
        }

        ftdf_pib.traffic_counters.tx_enh_ack_frm_cnt++;

        return FTDF_SUCCESS;
}
#endif /* !FTDF_NO_CSL || !FTDF_NO_TSCH */
#endif /* !FTDF_LITE */

void ftdf_send_transparent_frame(ftdf_data_length_t frame_length,
                                 ftdf_octet_t *frame,
                                 ftdf_channel_number_t channel,
                                 ftdf_pti_t pti,
                                 ftdf_boolean_t cmsa_suppress)
{
        uint32_t meta_data_0 = 0;
        uint32_t tmp;

#if FTDF_TRANSPARENT_USE_WAIT_FOR_ACK
        ftdf_boolean_t use_ack = FTDF_FALSE;
        //ftdf_frame_header_t *frame_header = &ftdf_fh;
        ftdf_frame_header_t frame_header;
        ftdf_sn_t sn;
#endif

#if FTDF_TRANSPARENT_USE_WAIT_FOR_ACK
        if (ftdf_transparent_mode_options & FTDF_TRANSPARENT_WAIT_FOR_ACK) {
                ftdf_get_frame_header(frame, &frame_header);
                if (frame_header.options & FTDF_OPT_ACK_REQUESTED) {
                        use_ack = FTDF_TRUE;
                }
                sn = frame_header.sn;
        }
#endif

        tmp = 0;
        REG_SET_FIELD(FTDF, FTDF_TX_PRIORITY_0_REG, PTI_TX, tmp, pti);
        REG_SET_FIELD(FTDF, FTDF_TX_PRIORITY_0_REG, TX_PRIORITY, tmp, 1);
        FTDF->FTDF_TX_PRIORITY_0_REG = tmp;

        REG_SET_FIELD(FTDF, FTDF_TX_META_DATA_0_0_REG, FRAME_LENGTH, meta_data_0, frame_length);
        REG_SET_FIELD(FTDF, FTDF_TX_META_DATA_0_0_REG, PHYATTR_DEM_PTI, meta_data_0, (ftdf_pib.cca_mode & 0x3) | 0x08);
        REG_SET_FIELD(FTDF, FTDF_TX_META_DATA_0_0_REG, PHYATTR_CN, meta_data_0, (channel - 11));
        REG_SET_FIELD(FTDF, FTDF_TX_META_DATA_0_0_REG, PHYATTR_RF_GPIO_PINS, meta_data_0, (ftdf_pib.tx_power & 0x07));
        REG_SET_FIELD(FTDF, FTDF_TX_META_DATA_0_0_REG, FRAMETYPE, meta_data_0, (*frame & 0x7));
        REG_SET_FIELD(FTDF, FTDF_TX_META_DATA_0_0_REG, CSMACA_ENA, meta_data_0, ((cmsa_suppress) ? 0 : 1));

#if FTDF_TRANSPARENT_USE_WAIT_FOR_ACK
        REG_SET_FIELD(FTDF, FTDF_TX_META_DATA_0_0_REG, ACKREQUEST, meta_data_0, (use_ack ? 1 : 0));
#endif
        REG_SET_FIELD(FTDF, FTDF_TX_META_DATA_0_0_REG, CRC16_ENA, meta_data_0,
                      ((ftdf_transparent_mode_options & FTDF_TRANSPARENT_ENABLE_FCS_GENERATION) ? 1 : 0));

        FTDF->FTDF_TX_META_DATA_0_0_REG = meta_data_0;

        uint32_t meta_data_1 = 0;
#if FTDF_TRANSPARENT_USE_WAIT_FOR_ACK
        if (use_ack) {
                REG_SET_FIELD(FTDF, FTDF_TX_META_DATA_1_0_REG, MACSN, meta_data_1, sn);
        } else {
                REG_SET_FIELD(FTDF, FTDF_TX_META_DATA_1_0_REG, MACSN, meta_data_1, 0);
        }
#else
        REG_SET_FIELD(FTDF, FTDF_TX_META_DATA_1_0_REG, MACSN, meta_data_1, 0);
#endif
        FTDF->FTDF_TX_META_DATA_1_0_REG = meta_data_1;

        tmp = FTDF->FTDF_LMAC_CONTROL_5_REG;
        REG_SET_FIELD(FTDF, FTDF_LMAC_CONTROL_5_REG, PHYCSMACAATTR_DEM_PTI, tmp, (ftdf_pib.cca_mode & 0x3));
        REG_SET_FIELD(FTDF, FTDF_LMAC_CONTROL_5_REG, PHYCSMACAATTR_CN, tmp, (channel - 11));
        REG_SET_FIELD(FTDF, FTDF_LMAC_CONTROL_5_REG, PHYCSMACAATTR_CALCAP, tmp, 0);
        REG_SET_FIELD(FTDF, FTDF_LMAC_CONTROL_5_REG, PHYCSMACAATTR_RF_GPIO_PINS, tmp, (ftdf_pib.tx_power & 0x07));
        REG_SET_FIELD(FTDF, FTDF_LMAC_CONTROL_5_REG, PHYCSMACAATTR_HSI, tmp, 0);
        FTDF->FTDF_LMAC_CONTROL_5_REG = tmp;

#if dg_configCOEX_ENABLE_CONFIG && !FTDF_USE_AUTO_PTI
        hw_coex_update_ftdf_pti((hw_coex_pti_t)pti, NULL, true);
#else
        // REG_SETF(FTDF, FTDF_TX_PRIORITY_0_REG, PTI_TX, pti);
#endif
        REG_SETF(FTDF, FTDF_TX_SET_OS_REG, TX_FLAG_SET, (1 << FTDF_TX_DATA_BUFFER));
}

void ftdf_init_queues(void)
{
#ifndef FTDF_LITE
        ftdf_init_queue(&ftdf_free_queue);
        ftdf_init_queue(&ftdf_req_queue);

        int n;

        for (n = 0; n < FTDF_NR_OF_REQ_BUFFERS; n++) {
                ftdf_queue_buffer_head(&ftdf_req_buffers[n], &ftdf_free_queue);

                ftdf_tx_pending_list[n].addr.ext_address = 0xFFFFFFFFFFFFFFFFLL;
                ftdf_tx_pending_list[n].addr_mode = FTDF_NO_ADDRESS;
                ftdf_tx_pending_list[n].pan_id = 0xFFFF;
                ftdf_init_queue(&ftdf_tx_pending_list[n].queue);

                ftdf_tx_pending_timer_list[n].free = FTDF_TRUE;
                ftdf_tx_pending_timer_list[n].next = NULL;
        }

        ftdf_tx_pending_timer_head = ftdf_tx_pending_timer_list;
        ftdf_tx_pending_timer_time = 0;
#endif /* !FTDF_LITE */
#ifndef FTDF_PHY_API
        ftdf_req_current = NULL;
#endif
}

#ifndef FTDF_LITE
void ftdf_init_queue(ftdf_queue_t *queue)
{
        queue->head.next = (ftdf_buffer_t*)&queue->tail;
        queue->head.prev = NULL;
        queue->tail.next = NULL;
        queue->tail.prev = (ftdf_buffer_t*)&queue->head;
}

void ftdf_queue_buffer_head(ftdf_buffer_t *buffer, ftdf_queue_t *queue)
{
        ftdf_buffer_t *next = queue->head.next;

        queue->head.next = buffer;
        next->header.prev = buffer;
        buffer->header.next = next;
        buffer->header.prev = (ftdf_buffer_t*)&queue->head;
}

ftdf_buffer_t *ftdf_dequeue_buffer_tail(ftdf_queue_t *queue)
{
        ftdf_buffer_t *buffer = queue->tail.prev;

        if (buffer->header.prev == NULL) {
                return NULL;
        }

        queue->tail.prev = buffer->header.prev;
        buffer->header.prev->header.next = (ftdf_buffer_t*)&queue->tail;

        return buffer;
}

ftdf_status_t ftdf_queue_req_head(ftdf_msg_buffer_t *request, ftdf_queue_t *queue)
{
        ftdf_buffer_t *buffer = ftdf_dequeue_buffer_tail(&ftdf_free_queue);

        if (buffer == NULL) {
                return FTDF_TRANSACTION_OVERFLOW;
        }

        ftdf_buffer_t *next = queue->head.next;

        queue->head.next = buffer;
        next->header.prev = buffer;
        buffer->header.next = next;
        buffer->header.prev = (ftdf_buffer_t*)&queue->head;
        buffer->request = request;

        return FTDF_SUCCESS;
}

ftdf_msg_buffer_t *ftdf_dequeue_req_tail(ftdf_queue_t *queue)
{
        ftdf_buffer_t *buffer = queue->tail.prev;

        if (buffer->header.prev == NULL) {
                return NULL;
        }

        queue->tail.prev = buffer->header.prev;
        buffer->header.prev->header.next = (ftdf_buffer_t*)&queue->tail;

        ftdf_msg_buffer_t *request = buffer->request;

        ftdf_queue_buffer_head(buffer, &ftdf_free_queue);

        return request;
}

ftdf_msg_buffer_t *ftdf_dequeue_by_handle(ftdf_handle_t handle, ftdf_queue_t *queue)
{
        ftdf_buffer_t *buffer = queue->head.next;

        while (buffer->header.next != NULL) {
                if (buffer->request && buffer->request->msg_id == FTDF_DATA_REQUEST &&
                        ((ftdf_data_request_t*)buffer->request)->msdu_handle == handle) {
                        buffer->header.prev->header.next = buffer->header.next;
                        buffer->header.next->header.prev = buffer->header.prev;

                        ftdf_msg_buffer_t *request = buffer->request;

                        ftdf_queue_buffer_head(buffer, &ftdf_free_queue);

                        return request;
                }

                buffer = buffer->header.next;
        }

        return NULL;
}

ftdf_msg_buffer_t *ftdf_dequeue_by_request(ftdf_msg_buffer_t *request, ftdf_queue_t *queue)
{
        ftdf_buffer_t *buffer = queue->head.next;

        while (buffer->header.next != NULL) {
                if (buffer->request == request) {
                        buffer->header.prev->header.next = buffer->header.next;
                        buffer->header.next->header.prev = buffer->header.prev;

                        ftdf_msg_buffer_t *req = buffer->request;

                        ftdf_queue_buffer_head(buffer, &ftdf_free_queue);

                        return req;
                }

                buffer = buffer->header.next;
        }

        return NULL;
}

ftdf_boolean_t ftdf_is_queue_empty(ftdf_queue_t *queue)
{
        if (queue->head.next->header.next == NULL) {
                return FTDF_TRUE;
        } else {
                return FTDF_FALSE;
        }
}

static ftdf_pending_tl_t *ftdf_find_free_pending_timer(void)
{
        uint8_t i;

        for (i = 0; i < FTDF_NR_OF_REQ_BUFFERS; i++) {
                if (ftdf_tx_pending_timer_list[i].free == FTDF_TRUE) {
                        break;
                }
        }

        return &ftdf_tx_pending_timer_list[i];
}

void ftdf_add_tx_pending_timer(ftdf_msg_buffer_t *request, uint8_t pendListNr, ftdf_time_t delta,
                               void (*func)(ftdf_pending_tl_t*))
{
        ftdf_critical_var();
        ftdf_enter_critical();

        ftdf_pending_tl_t *ptr = ftdf_tx_pending_timer_head;
        ftdf_time_t timestamp = REG_GETF(FTDF, FTDF_SYMBOLTIMESNAPSHOTVAL_REG,
                SYMBOLTIMESNAPSHOTVAL);

        if (ptr->free == FTDF_FALSE) {
                while (ptr) {
                        ptr->delta -= timestamp - ftdf_tx_pending_timer_lt;
                        ptr = ptr->next;
                }
        }

        ftdf_tx_pending_timer_lt = timestamp;
        ptr = ftdf_tx_pending_timer_head;

        if (ptr->free == FTDF_TRUE) {
                ptr->free = FTDF_FALSE;
                ptr->next = NULL;
                ptr->request = request;
                ptr->delta = delta;
                ptr->pend_list_nr = pendListNr;
                ptr->func = func;

                REG_SETF(FTDF, FTDF_SYMBOLTIMETHR_REG, SYMBOLTIMETHR, delta + timestamp);
                ftdf_tx_pending_timer_time = delta + timestamp;
                ftdf_exit_critical();
                return;
        }

        if (ptr->delta > delta) {
                ftdf_tx_pending_timer_head = ftdf_find_free_pending_timer();
                ftdf_tx_pending_timer_head->free = FTDF_FALSE;
                ftdf_tx_pending_timer_head->next = ptr;
                ftdf_tx_pending_timer_head->request = request;
                ftdf_tx_pending_timer_head->delta = delta;
                ftdf_tx_pending_timer_head->pend_list_nr = pendListNr;
                ftdf_tx_pending_timer_head->func = func;

                REG_SETF(FTDF, FTDF_SYMBOLTIMETHR_REG, SYMBOLTIMETHR, delta + timestamp);
                ftdf_tx_pending_timer_time = delta + timestamp;
                ftdf_exit_critical();

                return;
        } else if (ptr->delta == delta) {
                delta++;
        }

        ftdf_pending_tl_t *prev;

        while (ptr->next) {
                prev = ptr;
                ptr = ptr->next;

                if (ptr->delta == delta) {
                        delta++;
                }

                if (prev->delta < delta && ptr->delta > delta) {
                        prev->next = ftdf_find_free_pending_timer();
                        prev->next->next = ptr;
                        ptr = prev->next;
                        ptr->free = FTDF_FALSE;
                        ptr->request = request;
                        ptr->delta = delta;
                        ptr->pend_list_nr = pendListNr;
                        ptr->func = func;

                        ftdf_exit_critical();

                        return;
                }
        }

        ptr->next = ftdf_find_free_pending_timer();
        ptr = ptr->next;
        ptr->free = FTDF_FALSE;
        ptr->next = NULL;
        ptr->request = request;
        ptr->delta = delta;
        ptr->pend_list_nr = pendListNr;
        ptr->func = func;

        ftdf_exit_critical();
}

void ftdf_remove_tx_pending_timer(ftdf_msg_buffer_t *request)
{
        ftdf_critical_var();
        ftdf_enter_critical();

        ftdf_pending_tl_t *ptr = ftdf_tx_pending_timer_head;
        ftdf_time_t timestamp =
            REG_GETF(FTDF, FTDF_SYMBOLTIMESNAPSHOTVAL_REG, SYMBOLTIMESNAPSHOTVAL);

        if (ptr->free == FTDF_TRUE) {
                REG_SETF(FTDF, FTDF_SYMBOLTIMETHR_REG, SYMBOLTIMETHR, timestamp - 1);
                ftdf_tx_pending_timer_time = timestamp - 1;
                ftdf_exit_critical();

                return;
        }

        while (ptr) {
                ptr->delta -= timestamp - ftdf_tx_pending_timer_lt;
                ptr = ptr->next;
        }

        ftdf_tx_pending_timer_lt = timestamp;
        ptr = ftdf_tx_pending_timer_head;

        if (!request || ptr->request == request) {
                if (ptr->next) {
                        ftdf_pending_tl_t *temp = ptr;

                        if (ptr->next->delta < 75) {
                                while (temp) {
                                        temp->delta += 75;
                                        temp = temp->next;
                                }
                        }

                        REG_SETF(FTDF, FTDF_SYMBOLTIMETHR_REG, SYMBOLTIMETHR,
                                timestamp + ptr->next->delta);
                        ftdf_tx_pending_timer_time = timestamp + ptr->next->delta;
                        ftdf_tx_pending_timer_head = ptr->next;

                        ptr->free = FTDF_TRUE;
                        ptr->next = NULL;
                } else {
                        REG_SETF(FTDF, FTDF_SYMBOLTIMETHR_REG, SYMBOLTIMETHR, timestamp - 1);
                        ftdf_tx_pending_timer_time = timestamp - 1;

                        ptr->free = FTDF_TRUE;
                        ptr->next = NULL;
                }

                ftdf_exit_critical();

                if (!request) {
                        if (ptr->func) {
                                ptr->func(ptr);
                        }
                }

                return;
        }

        ftdf_pending_tl_t *prev = ptr;

        while (ptr->next) {
                prev = ptr;
                ptr = ptr->next;

                if (ptr->request == request) {
                        prev->next = ptr->next;
                        ptr->free = FTDF_TRUE;
                        ptr->next = NULL;

                        ftdf_exit_critical();

                        return;
                }
        }

        REG_SETF(FTDF, FTDF_SYMBOLTIMETHR_REG, SYMBOLTIMETHR, timestamp - 1);
        ftdf_tx_pending_timer_time = timestamp - 1;
        ftdf_exit_critical();
}

void ftdf_restore_tx_pending_timer(void)
{
        REG_SETF(FTDF, FTDF_SYMBOLTIMETHR_REG, SYMBOLTIMETHR, ftdf_tx_pending_timer_time);
}

ftdf_boolean_t ftdf_get_tx_pending_timer_head(ftdf_time_t *time)
{
        ftdf_pending_tl_t *ptr = ftdf_tx_pending_timer_head;

        if (ptr->free == FTDF_TRUE) {
                return FTDF_FALSE;
        }

        *time = ftdf_tx_pending_timer_time;

        return FTDF_TRUE;
}

#ifndef FTDF_NO_TSCH
void ftdf_process_keep_alive_timer_exp(ftdf_pending_tl_t *ptr) {
        ftdf_remote_request_t *remote_request = (ftdf_remote_request_t*)ptr->request;

        remote_request->msg_id = FTDF_REMOTE_REQUEST;
        remote_request->remote_id = FTDF_REMOTE_KEEP_ALIVE;
        remote_request->dst_addr = ftdf_neighbor_table[ptr->pend_list_nr].dst_addr;

        ftdf_process_remote_request(remote_request);
}
#endif /* FTDF_NO_TSCH */

void ftdf_send_transaction_expired(ftdf_pending_tl_t *ptr)
{
        ftdf_msg_buffer_t *req =
                ftdf_dequeue_by_request(ptr->request,
                        &ftdf_tx_pending_list[ptr->pend_list_nr].queue);

        if (!req) {
                return;
        }

        if (ftdf_is_queue_empty(&ftdf_tx_pending_list[ptr->pend_list_nr].queue)) {
#ifndef FTDF_NO_TSCH
                if (ftdf_pib.tsch_enabled) {
                        ftdf_tx_pending_list[ptr->pend_list_nr].addr.short_address = 0xfffe;
                } else
#endif /* FTDF_NO_TSCH */
                {
#if FTDF_FP_BIT_MODE == FTDF_FP_BIT_MODE_AUTO
                if (ftdf_tx_pending_list[ptr->pend_list_nr].addr_mode == FTDF_SHORT_ADDRESS) {
                        uint8_t entry, short_addr_idx;
                        ftdf_boolean_t found = ftdf_fppr_lookup_short_address(
                            ftdf_tx_pending_list[ptr->pend_list_nr].addr.short_address, &entry,
                            &short_addr_idx);
                        ASSERT_WARNING(found);
                        ftdf_fppr_set_short_address_valid(entry, short_addr_idx, FTDF_FALSE);
            } else if (ftdf_tx_pending_list[ptr->pend_list_nr].addr_mode == FTDF_EXTENDED_ADDRESS) {
                    uint8_t entry;
                    ftdf_boolean_t found = ftdf_fppr_lookup_ext_address(
                            ftdf_tx_pending_list[ptr->pend_list_nr].addr.ext_address, &entry);
                    ASSERT_WARNING(found);
                    ftdf_fppr_set_ext_address_valid(entry, FTDF_FALSE);
            } else {
                    ASSERT_WARNING(0);
            }
#endif /* FTDF_FP_BIT_MODE == FTDF_FP_BIT_MODE_AUTO */
                        ftdf_tx_pending_list[ptr->pend_list_nr].addr_mode = FTDF_NO_ADDRESS;
                }
        }

        switch (req->msg_id) {
        case FTDF_DATA_REQUEST:
        {
                ftdf_data_request_t *data_request = (ftdf_data_request_t*)req;

                ftdf_send_data_confirm(data_request, FTDF_TRANSACTION_EXPIRED, 0, 0, 0, NULL);

                break;
        }
        case FTDF_ASSOCIATE_REQUEST:
        {
                ftdf_associate_request_t* associate_request = (ftdf_associate_request_t*)req;

                ftdf_send_associate_confirm(associate_request, FTDF_TRANSACTION_EXPIRED, 0xffff);

                break;
        }
        case FTDF_ASSOCIATE_RESPONSE:
        {
                ftdf_associate_response_t *assoc_resp = (ftdf_associate_response_t*)req;

                ftdf_address_t src_addr, dst_addr;
                src_addr.ext_address = ftdf_pib.ext_address;
                dst_addr.ext_address = assoc_resp->device_address;

                ftdf_send_comm_status_indication(req, FTDF_TRANSACTION_EXPIRED,
                                                 ftdf_pib.pan_id,
                                                 FTDF_EXTENDED_ADDRESS,
                                                 src_addr,
                                                 FTDF_EXTENDED_ADDRESS,
                                                 dst_addr,
                                                 assoc_resp->security_level,
                                                 assoc_resp->key_id_mode,
                                                 assoc_resp->key_source,
                                                 assoc_resp->key_index);

                break;
        }
        case FTDF_DISASSOCIATE_REQUEST:
        {
                ftdf_disassociate_request_t* dis_req = (ftdf_disassociate_request_t*)req;

                ftdf_send_disassociate_confirm(dis_req, FTDF_TRANSACTION_EXPIRED);

                break;
        }
        case FTDF_BEACON_REQUEST:
        {
                ftdf_beacon_request_t* beacon_request = (ftdf_beacon_request_t*)req;

                ftdf_send_beacon_confirm(beacon_request, FTDF_TRANSACTION_EXPIRED);

                break;
        }
        }
}

#ifndef FTDF_NO_TSCH
void ftdf_reset_keep_alive_timer(ftdf_short_address_t dst_addr)
{
        uint8_t n;

        for (n = 0; n < FTDF_NR_OF_NEIGHBORS; n++) {
                if (ftdf_neighbor_table[n].dst_addr == dst_addr) {
                        break;
                }
        }

        if (n == FTDF_NR_OF_NEIGHBORS) {
                return;
        }

        ftdf_remove_tx_pending_timer((ftdf_msg_buffer_t*)&ftdf_neighbor_table[n].msg);

        ftdf_time_t ts_timeslot_length = (ftdf_time_t)ftdf_pib.timeslot_template.ts_timeslot_length
                / 16;
        ftdf_time_t delta = ts_timeslot_length * ftdf_neighbor_table[n].period;

        ftdf_add_tx_pending_timer((ftdf_msg_buffer_t*)&ftdf_neighbor_table[n].msg, n, delta,
                ftdf_process_keep_alive_timer_exp);
}
#endif /* FTDF_NO_TSCH */
#endif /* !FTDF_LITE */

void ftdf_enable_transparent_mode(ftdf_boolean_t enable,
                                  ftdf_bitmap32_t options)
{
#ifndef FTDF_LITE
        if ((ftdf_pib.le_enabled == FTDF_TRUE) || (ftdf_pib.tsch_enabled == FTDF_TRUE)) {
                return;
        }
#endif /* !FTDF_LITE */


        ftdf_transparent_mode = enable;
        ftdf_transparent_mode_options = options;

        uint32_t tmp = FTDF->FTDF_RX_CONTROL_0_REG;

        if (enable) {
                REG_SET_FIELD(FTDF, FTDF_RX_CONTROL_0_REG, MACALWAYSPASSFRMTYPE, tmp,
                        (options & FTDF_TRANSPARENT_PASS_ALL_FRAME_TYPES));
                REG_SET_FIELD(FTDF, FTDF_RX_CONTROL_0_REG, DISRXACKREQUESTCA, tmp,
                        (options & FTDF_TRANSPARENT_AUTO_ACK ? 0 : 1));
                REG_SET_FIELD(FTDF, FTDF_RX_CONTROL_0_REG, MACALWAYSPASSCRCERROR, tmp,
                        (options & FTDF_TRANSPARENT_PASS_CRC_ERROR ? 1 : 0));
                REG_SET_FIELD(FTDF, FTDF_RX_CONTROL_0_REG, MACALWAYSPASSRESFRAMEVERSION, tmp,
                        (options & FTDF_TRANSPARENT_PASS_ALL_FRAME_VERSION ? 1 : 0));
                REG_SET_FIELD(FTDF, FTDF_RX_CONTROL_0_REG, MACALWAYSPASSWRONGDPANID, tmp,
                        (options & FTDF_TRANSPARENT_PASS_ALL_PAN_ID ? 1 : 0));
                REG_SET_FIELD(FTDF, FTDF_RX_CONTROL_0_REG, MACALWAYSPASSWRONGDADDR, tmp,
                        (options & FTDF_TRANSPARENT_PASS_ALL_ADDR ? 1 : 0));
                REG_SET_FIELD(FTDF, FTDF_RX_CONTROL_0_REG, MACALWAYSPASSBEACONWRONGPANID, tmp,
                        (options & FTDF_TRANSPARENT_PASS_ALL_BEACON ? 1 : 0));
                REG_SET_FIELD(FTDF, FTDF_RX_CONTROL_0_REG, MACALWAYSPASSTOPANCOORDINATOR, tmp,
                        (options & FTDF_TRANSPARENT_PASS_ALL_NO_DEST_ADDR ? 1 : 0));
        } else {
                REG_SET_FIELD(FTDF, FTDF_RX_CONTROL_0_REG, MACALWAYSPASSFRMTYPE, tmp, 0);
                REG_SET_FIELD(FTDF, FTDF_RX_CONTROL_0_REG, DISRXACKREQUESTCA, tmp, 0);
                REG_SET_FIELD(FTDF, FTDF_RX_CONTROL_0_REG, MACALWAYSPASSCRCERROR, tmp, 0);
                REG_SET_FIELD(FTDF, FTDF_RX_CONTROL_0_REG, MACALWAYSPASSRESFRAMEVERSION, tmp, 0);
                REG_SET_FIELD(FTDF, FTDF_RX_CONTROL_0_REG, MACALWAYSPASSWRONGDPANID, tmp, 0);
                REG_SET_FIELD(FTDF, FTDF_RX_CONTROL_0_REG, MACALWAYSPASSWRONGDADDR, tmp, 0);
                REG_SET_FIELD(FTDF, FTDF_RX_CONTROL_0_REG, MACALWAYSPASSBEACONWRONGPANID, tmp, 0);
                REG_SET_FIELD(FTDF, FTDF_RX_CONTROL_0_REG, MACALWAYSPASSTOPANCOORDINATOR, tmp, 0);
        }

        FTDF->FTDF_RX_CONTROL_0_REG = tmp;
}

#if FTDF_DBG_BUS_ENABLE
void ftdf_check_dbg_mode(void)
{
        REG_SET_MASKED(FTDF, FTDF_DEBUGCONTROL_REG, 0xFF, ftdf_dbg_mode);
        if (ftdf_dbg_mode) {
                FTDF_DBG_BUS_GPIO_CONFIG();
        }
}

void ftdf_set_dbg_mode(ftdf_dbg_mode_t dbg_mode)
{
        ftdf_dbg_mode = dbg_mode;
        ftdf_check_dbg_mode();
}
#endif /* FTDF_DBG_BUS_ENABLE */

#ifndef FTDF_LITE
#ifndef FTDF_NO_CSL
void ftdf_set_peer_csl_timing(ftdf_ie_list_t *header_ie_list, ftdf_time_t time_stamp)
{
        if (ftdf_req_current->msg_id != FTDF_DATA_REQUEST) {
                /* Only use the CSL timing of data frame acks */
                return;
        }

        ftdf_data_request_t* data_request = (ftdf_data_request_t*)ftdf_req_current;
        ftdf_short_address_t dst_addr = data_request->dst_addr.short_address;

        if ((data_request->dst_addr_mode != FTDF_SHORT_ADDRESS) || (dst_addr == 0xffff)) {
                return;
        }

        int n;

        /* Search for an existing entry */
        for (n = 0; n < FTDF_NR_OF_CSL_PEERS; n++) {
                if (ftdf_peer_csl_timing[n].addr == dst_addr) {
                        break;
                }
        }

        if (header_ie_list == NULL) {
                if (n < FTDF_NR_OF_CSL_PEERS) {
                        /* Delete entry */
                        ftdf_peer_csl_timing[n].addr = 0xffff;
                }

                return;
        }

        int ie_nr = 0;

        while (ie_nr < header_ie_list->nr_of_ie && header_ie_list->ie[ie_nr].id != 0x1a) {
                ie_nr++;
        }

        if (ie_nr == header_ie_list->nr_of_ie) {
                return;
        }

        if (n == FTDF_NR_OF_CSL_PEERS) {
                /* Search for an empty entry */
                for (n = 0; n < FTDF_NR_OF_CSL_PEERS; n++) {
                        if (ftdf_peer_csl_timing[n].addr == 0xffff) {
                                break;
                        }
                }
        }

        if (n == FTDF_NR_OF_CSL_PEERS) {
                /* No free entry, search for the least recently used entry */
                ftdf_time_t max_delta = 0;
                int lru = 0;

                for (n = 0; n < FTDF_NR_OF_CSL_PEERS; n++) {
                        ftdf_time_t delta = time_stamp - ftdf_peer_csl_timing[n].time;

                        if (delta > max_delta) {
                                max_delta = delta;
                                lru = n;
                        }
                }

                n = lru;
        }

        ftdf_period_t phase = (*(ftdf_period_t*)(header_ie_list->ie[0].content.raw + 0));
        ftdf_period_t period = (*(ftdf_period_t*)(header_ie_list->ie[0].content.raw + 2));

        ftdf_peer_csl_timing[n].addr = dst_addr;
        ftdf_peer_csl_timing[n].time = time_stamp - (phase * 10);
        ftdf_peer_csl_timing[n].period = period;
}

void ftdf_get_wakeup_params(ftdf_short_address_t dst_addr, ftdf_time_t *wakeup_start_time,
        ftdf_period_t *wakeup_period)
{
        int n;

        for (n = 0; n < FTDF_NR_OF_CSL_PEERS; n++) {
                if (ftdf_peer_csl_timing[n].addr == dst_addr) {
                        break;
                }
        }

        ftdf_time_t cur_time = REG_GETF(FTDF, FTDF_SYMBOLTIMESNAPSHOTVAL_REG,
                SYMBOLTIMESNAPSHOTVAL);

        if (dst_addr == 0xffff ||
                n == FTDF_NR_OF_CSL_PEERS) {
                *wakeup_start_time = cur_time + 5;
                *wakeup_period = ftdf_pib.csl_max_period;

                return;
        }

        ftdf_time_t peer_time = ftdf_peer_csl_timing[n].time;
        ftdf_time_t peer_period = ftdf_peer_csl_timing[n].period * 10;
        ftdf_time_t delta = cur_time - peer_time;

        if (delta > (uint32_t)(ftdf_pib.csl_max_age_remote_info * 10)) {
                *wakeup_start_time = cur_time + 5;
                *wakeup_period = ftdf_pib.csl_max_period;

                return;
        }

        ftdf_time_t w_start = peer_time + (((delta / peer_period) + 1) * peer_period)
                - ftdf_pib.csl_sync_tx_margin;
        delta = w_start - cur_time;

        /* A delta larger than 0x80000000 is assumed a negative delta */
        if ((delta < 3) || (delta > 0x80000000)) {
                w_start += peer_period;
        }

        *wakeup_period = (ftdf_pib.csl_sync_tx_margin / 10) * 2;
        *wakeup_start_time = w_start;
}

void ftdf_set_csl_sample_time(void)
{
        ftdf_time_t csl_period = ftdf_pib.csl_period * 10;

        ftdf_critical_var();
        ftdf_enter_critical();

        ftdf_time_t cur_time = REG_GETF(FTDF, FTDF_SYMBOLTIMESNAPSHOTVAL_REG, SYMBOLTIMESNAPSHOTVAL);
        ftdf_time_t delta = (cur_time - ftdf_start_csl_sample_time);

        /* A delta larger than 0x80000000 is assumed a negative delta, in this case the sample time
         * does not need to be updated. */
        if (delta < 0x80000000) {
                if (delta < csl_period) {
                        ftdf_start_csl_sample_time += csl_period;

                        if (delta < 3) {
                                /* To avoid to set the CSL sample time to a time stamp in the past
                                 * set it to a sample period later if the next sample would be
                                 * within 3 symbols. */
                                ftdf_start_csl_sample_time += csl_period;
                        }

                } else {

                        ftdf_start_csl_sample_time =
                            ftdf_start_csl_sample_time + ((delta / csl_period) + 1) * csl_period;
                }

                REG_SETF(FTDF, FTDF_LMAC_CONTROL_8_REG, MACCSLSTARTSAMPLETIME, ftdf_start_csl_sample_time);
        }

        ftdf_exit_critical();
}
#endif /* FTDF_NO_CSL */
#endif /* !FTDF_LITE */

ftdf_time64_t ftdf_get_cur_time64(void)
{
        ftdf_time_t new_time = REG_GETF(FTDF, FTDF_SYMBOLTIMESNAPSHOTVAL_REG, SYMBOLTIMESNAPSHOTVAL);

        if (new_time < ftdf_cur_time[0]) {
                ftdf_cur_time[1]++;
        }

        ftdf_cur_time[0] = new_time;

        return *(ftdf_time64_t*)ftdf_cur_time;
}

void ftdf_init_cur_time64(void)
{
        ftdf_cur_time[0] = REG_GETF(FTDF, FTDF_SYMBOLTIMESNAPSHOTVAL_REG, SYMBOLTIMESNAPSHOTVAL);
        ftdf_cur_time[1] = 0;
}

void ftdf_get_ext_address(void)
{
        uint32_t *ext_address = (uint32_t*)&ftdf_pib.ext_address;

        ext_address[0] = REG_GETF(FTDF, FTDF_GLOB_CONTROL_2_REG, AEXTENDEDADDRESS_L);
        ext_address[1] = REG_GETF(FTDF, FTDF_GLOB_CONTROL_3_REG, AEXTENDEDADDRESS_H);
}

void ftdf_set_ext_address(void)
{
        uint32_t *ext_address = (uint32_t*)&ftdf_pib.ext_address;

        REG_SETF(FTDF, FTDF_GLOB_CONTROL_2_REG, AEXTENDEDADDRESS_L, ext_address[0]);
        REG_SETF(FTDF, FTDF_GLOB_CONTROL_3_REG, AEXTENDEDADDRESS_H, ext_address[1]);
}

void ftdf_get_ack_wait_duration(void)
{
        ftdf_pib.ack_wait_duration = REG_GETF(FTDF, FTDF_MACACKWAITDURATION_REG, MACACKWAITDURATION);
}

#ifndef FTDF_LITE
void ftdf_get_enh_ack_wait_duration(void)
{
        ftdf_pib.enh_ack_wait_duration = REG_GETF(FTDF, FTDF_MACENHACKWAITDURATION_REG,
                                                  MACENHACKWAITDURATION);
}

void ftdf_set_enh_ack_wait_duration(void)
{
        REG_SETF(FTDF, FTDF_MACENHACKWAITDURATION_REG, MACENHACKWAITDURATION,
                 ftdf_pib.enh_ack_wait_duration);
}

void ftdf_get_implicit_broadcast(void)
{
        ftdf_pib.implicit_broadcast = REG_GETF(FTDF, FTDF_RX_CONTROL_0_REG, MACIMPLICITBROADCAST);
}

void ftdf_set_implicit_broadcast(void)
{
        REG_SETF(FTDF, FTDF_RX_CONTROL_0_REG, MACIMPLICITBROADCAST, ftdf_pib.implicit_broadcast);
}
#endif /* !FTDF_LITE */

void ftdf_set_short_address(void)
{
        REG_SETF(FTDF, FTDF_GLOB_CONTROL_1_REG, MACSHORTADDRESS, ftdf_pib.short_address);
}

#ifndef FTDF_LITE
void ftdf_set_simple_address(void)
{
        REG_SETF(FTDF, FTDF_GLOB_CONTROL_0_REG, MACSIMPLEADDRESS, ftdf_pib.simple_address);
}
#endif /* !FTDF_LITE */

void ftdf_get_rx_on_when_idle(void)
{
        ftdf_pib.rx_on_when_idle = REG_GETF(FTDF, FTDF_LMAC_CONTROL_0_REG, RXALWAYSON);
}

void ftdf_set_rx_on_when_idle(void)
{
#if dg_configCOEX_ENABLE_CONFIG && !FTDF_USE_AUTO_PTI
        /* We do not force decision here. It will be automatically made when FTDF begins
         * transaction.
         */
        hw_coex_update_ftdf_pti((hw_coex_pti_t)ftdf_get_rx_pti(), NULL, false);
#endif /* dg_configCOEX_ENABLE_CONFIG && !FTDF_USE_AUTO_PTI */
        ftdf_set_link_quality_mode();
        REG_SETF(FTDF, FTDF_LMAC_CONTROL_OS_REG, RXENABLE, 0);
        REG_SETF(FTDF, FTDF_LMAC_CONTROL_0_REG, RXALWAYSON, ftdf_pib.rx_on_when_idle);
        REG_SETF(FTDF, FTDF_LMAC_CONTROL_OS_REG, RXENABLE, 1);

}

void ftdf_getpan_id(void)
{
        ftdf_pib.pan_id = REG_GETF(FTDF, FTDF_GLOB_CONTROL_1_REG, MACPANID);
}

void ftdf_setpan_id(void)
{
        REG_SETF(FTDF, FTDF_GLOB_CONTROL_1_REG, MACPANID, ftdf_pib.pan_id);
}

void ftdf_get_current_channel(void)
{
        ftdf_pib.current_channel = (REG_GETF(FTDF, FTDF_LMAC_CONTROL_1_REG, PHYRXATTR_CN)) + 11;
}

void ftdf_set_current_channel(void)
{
        uint32_t phy_ack_attr = 0;

        REG_SETF(FTDF, FTDF_LMAC_CONTROL_1_REG, PHYRXATTR_CN, (ftdf_pib.current_channel - 11));

        ftdf_set_link_quality_mode();
        REG_SET_FIELD(FTDF, FTDF_LMAC_CONTROL_4_REG, PHYACKATTR_DEM_PTI, phy_ack_attr, 0x08);
        REG_SET_FIELD(FTDF, FTDF_LMAC_CONTROL_4_REG, PHYACKATTR_CN, phy_ack_attr,
                      (ftdf_pib.current_channel - 11));
        REG_SET_FIELD(FTDF, FTDF_LMAC_CONTROL_4_REG, PHYACKATTR_RF_GPIO_PINS, phy_ack_attr,
                      (ftdf_pib.tx_power & 0x07));

        FTDF->FTDF_LMAC_CONTROL_4_REG = phy_ack_attr;
}

void ftdf_set_tx_power(void)
{
        /* Just like setCurrentChannel, this sets pyAckAttr */
        uint32_t phy_ack_attr = 0;

        REG_SET_FIELD(FTDF, FTDF_LMAC_CONTROL_4_REG, PHYACKATTR_DEM_PTI, phy_ack_attr, 0x08);
        REG_SET_FIELD(FTDF, FTDF_LMAC_CONTROL_4_REG, PHYACKATTR_CN, phy_ack_attr,
                      (ftdf_pib.current_channel - 11));
        REG_SET_FIELD(FTDF, FTDF_LMAC_CONTROL_4_REG, PHYACKATTR_RF_GPIO_PINS, phy_ack_attr,
                      (ftdf_pib.tx_power & 0x07));

        FTDF->FTDF_LMAC_CONTROL_4_REG = phy_ack_attr;
}

void ftdf_get_max_frame_total_wait_time(void)
{
        ftdf_pib.max_frame_total_wait_time = REG_GETF(FTDF, FTDF_LMAC_CONTROL_3_REG, MACMAXFRAMETOTALWAITTIME);
}

void ftdf_set_max_frame_total_wait_time(void)
{
        REG_SETF(FTDF, FTDF_LMAC_CONTROL_3_REG, MACMAXFRAMETOTALWAITTIME, ftdf_pib.max_frame_total_wait_time);
}

void ftdf_set_max_csma_backoffs(void)
{
#ifndef FTDF_LITE
        if ((ftdf_pib.le_enabled == FTDF_FALSE) && (ftdf_pib.tsch_enabled == FTDF_FALSE))
#endif /* !FTDF_LITE */
        {
                REG_SETF(FTDF, FTDF_TX_CONTROL_0_REG, MACMAXCSMABACKOFFS, ftdf_pib.max_csma_backoffs);
        }
}

void ftdf_set_max_be(void)
{
        REG_SETF(FTDF, FTDF_TX_CONTROL_0_REG, MACMAXBE, ftdf_pib.max_be);
}

void ftdf_set_min_be(void)
{
#ifndef FTDF_LITE
        if ((ftdf_pib.le_enabled == FTDF_FALSE) && (ftdf_pib.tsch_enabled == FTDF_FALSE))
#endif /* !FTDF_LITE */
        {
                REG_SETF(FTDF, FTDF_TX_CONTROL_0_REG, MACMINBE, ftdf_pib.min_be);
        }
}

#ifndef FTDF_LITE
#ifndef FTDF_NO_CSL
void ftdf_set_le_enabled(void)
{
        REG_SETF(FTDF, FTDF_LMAC_CONTROL_7_REG, MACCSLSAMPLEPERIOD, 66);
        REG_SETF(FTDF, FTDF_LMAC_CONTROL_9_REG, MACCSLDATAPERIOD, 66);
        REG_SETF(FTDF, FTDF_LMAC_CONTROL_10_REG, MACCSLMARGINRZ, 1);

        if (ftdf_pib.le_enabled) {
                REG_SETF(FTDF, FTDF_TX_CONTROL_0_REG, MACMAXCSMABACKOFFS, 0);
                REG_SETF(FTDF, FTDF_TX_CONTROL_0_REG, MACMINBE, 0);

                if (ftdf_old_le_enabled == FTDF_FALSE) {

                        if (ftdf_wake_up_enable_le == FTDF_FALSE) {
                                ftdf_start_csl_sample_time =
                                    REG_GETF(FTDF, FTDF_SYMBOLTIMESNAPSHOTVAL_REG, SYMBOLTIMESNAPSHOTVAL);
                                ftdf_set_csl_sample_time();

                        } else {

                                REG_SETF(FTDF, FTDF_LMAC_CONTROL_8_REG, MACCSLSTARTSAMPLETIME,
                                        ftdf_start_csl_sample_time);
                        }
                }

        } else {

                REG_SETF(FTDF, FTDF_TX_CONTROL_0_REG, MACMAXCSMABACKOFFS, ftdf_pib.max_csma_backoffs);
                REG_SETF(FTDF, FTDF_TX_CONTROL_0_REG, MACMINBE, ftdf_pib.min_be);
        }

        REG_SETF(FTDF, FTDF_GLOB_CONTROL_0_REG, MACLEENABLED, ftdf_pib.le_enabled);

        ftdf_old_le_enabled = ftdf_pib.le_enabled;
}

void ftdf_get_csl_frame_pending_wait(void)
{
        ftdf_pib.csl_frame_pending_wait = REG_GETF(FTDF, FTDF_LMAC_CONTROL_9_REG, MACCSLFRAMEPENDINGWAITT);
}

void ftdf_set_csl_frame_pending_wait(void)
{
        REG_SETF(FTDF, FTDF_LMAC_CONTROL_9_REG, MACCSLFRAMEPENDINGWAITT,
                ftdf_pib.csl_frame_pending_wait);
}
#endif /* FTDF_NO_CSL */
#endif /* !FTDF_LITE */

void ftdf_get_lmac_pm_data(void)
{
        ftdf_pib.performance_metrics.fcs_error_count =
                REG_GETF(FTDF, FTDF_MACFCSERRORCOUNT_REG, MACFCSERRORCOUNT) +
                        ftdf_lmac_counters.fcs_error_cnt;
}

void ftdf_get_lmac_traffic_counters(void)
{
        ftdf_pib.traffic_counters.tx_std_ack_frm_cnt =
                REG_GETF(FTDF, FTDF_MACTXSTDACKFRMCNT_REG, MACTXSTDACKFRMCNT) +
                ftdf_lmac_counters.tx_std_ack_cnt;

        ftdf_pib.traffic_counters.rx_std_ack_frm_ok_cnt =
                REG_GETF(FTDF, FTDF_MACRXSTDACKFRMOKCNT_REG, MACRXSTDACKFRMOKCNT) +
                        ftdf_lmac_counters.rx_std_ack_cnt;
}

void ftdf_get_keep_phy_enabled(void)
{
        ftdf_pib.keep_phy_enabled = REG_GETF(FTDF, FTDF_LMAC_CONTROL_0_REG, KEEP_PHY_EN);
}

void ftdf_set_keep_phy_enabled(void)
{
        REG_SETF(FTDF, FTDF_LMAC_CONTROL_0_REG, KEEP_PHY_EN, ftdf_pib.keep_phy_enabled);
}

void ftdf_set_bo_irq_threshold(void)
{
        REG_SETF(FTDF, FTDF_LMAC_CONTROL_11_REG, CSMA_CA_BO_THRESHOLD, ftdf_pib.bo_irq_threshold);
}
void ftdf_get_bo_irq_threshold(void)
{
        ftdf_pib.bo_irq_threshold = REG_GETF(FTDF, FTDF_LMAC_CONTROL_11_REG, CSMA_CA_BO_THRESHOLD);
}
void ftdf_set_pti_config(void)
{
        REG_SETF(FTDF, FTDF_TX_PRIORITY_0_REG, PTI_TX, ftdf_pib.pti_config.ptis[FTDF_PTI_CONFIG_TX]);
        REG_SETF(FTDF, FTDF_TX_PRIORITY_1_REG, PTI_TX, ftdf_pib.pti_config.ptis[FTDF_PTI_CONFIG_TX]);
        REG_SETF(FTDF, FTDF_TX_PRIORITY_2_REG, PTI_TX, ftdf_pib.pti_config.ptis[FTDF_PTI_CONFIG_RX]);
        REG_SETF(FTDF, FTDF_LMAC_CONTROL_0_REG, PTI_RX, ftdf_pib.pti_config.ptis[FTDF_PTI_CONFIG_RX]);
}

#ifndef FTDF_LITE
#ifndef FTDF_NO_TSCH
void ftdf_set_timeslot_template(void)
{
        REG_SETF(FTDF, FTDF_TSCH_CONTROL_0_REG, MACTSTXACKDELAY,
                ftdf_pib.timeslot_template.ts_tx_ack_delay);
        REG_SETF(FTDF, FTDF_TSCH_CONTROL_0_REG, MACTSRXWAIT, ftdf_pib.timeslot_template.ts_rx_wait);
        REG_SETF(FTDF, FTDF_TSCH_CONTROL_2_REG, MACTSRXACKDELAY,
                ftdf_pib.timeslot_template.ts_rx_ack_delay);
        REG_SETF(FTDF, FTDF_TSCH_CONTROL_2_REG, MACTSACKWAIT,
                ftdf_pib.timeslot_template.ts_ack_wait);
        REG_SETF(FTDF, FTDF_TSCH_CONTROL_1_REG, MACTSRXTX, (ftdf_pib.timeslot_template.ts_rx_tx -
                FTDF_PHYTRXWAIT - FTDF_PHYTXSTARTUP - FTDF_PHYTXLATENCY));
}
#endif /* FTDF_NO_TSCH */
#endif /* !FTDF_LITE */

#if FTDF_FP_BIT_MODE == FTDF_FP_BIT_MODE_AUTO
/************************************ FPPR common functions ***************************************/
#ifndef FTDF_LITE

ftdf_boolean_t ftdf_fp_fsm_short_address_new(ftdf_pan_id_t panId, ftdf_short_address_t short_address)
{
        uint8_t entry;
        uint8_t short_addr_idx;

        if (ftdf_fppr_get_free_short_address(&entry, &short_addr_idx) == FTDF_FALSE) {
                return FTDF_FALSE;
        }

        ftdf_fppr_set_short_address(entry, short_addr_idx, short_address);
        ftdf_fppr_set_short_address_valid(entry, short_addr_idx, FTDF_TRUE);

        return FTDF_TRUE;
}

ftdf_boolean_t ftdf_fp_fsm_rxt_address_new(ftdf_pan_id_t panId, ftdf_ext_address_t ext_address)
{
        uint8_t entry;

        if (ftdf_fppr_get_free_ext_address(&entry) == FTDF_FALSE) {
                return FTDF_FALSE;
        }

        ftdf_fppr_set_ext_address(entry, ext_address);
        ftdf_fppr_set_ext_address_valid(entry, FTDF_TRUE);

        return FTDF_TRUE;
}

void ftdf_fp_fsm_short_address_last_frame_pending(ftdf_pan_id_t pan_id, ftdf_short_address_t short_address) {
#if FTDF_FPPR_DEFER_INVALIDATION
        ftdf_fppr_pending.addr_mode = FTDF_SHORT_ADDRESS;
        ftdf_fppr_pending.pan_id = pan_id;
        ftdf_fppr_pending.addr.short_address = short_address;
#else /* FTDF_FPPR_DEFER_INVALIDATION */
        uint8_t entry;
        uint8_t short_addr_idx;
        ftdf_boolean_t found = ftdf_fppr_lookup_short_address(short_address, &entry, &short_addr_idx);
        ASSERT_WARNING(found);
        ftdf_fppr_set_short_address_valid(entry, short_addr_idx, FTDF_FALSE);
#endif /* FTDF_FPPR_DEFER_INVALIDATION */
}

void ftdf_fp_fsm_ext_address_last_frame_pending(ftdf_pan_id_t pan_id, ftdf_ext_address_t ext_address) {
#if FTDF_FPPR_DEFER_INVALIDATION
        ftdf_fppr_pending.addr_mode = FTDF_EXTENDED_ADDRESS;
        ftdf_fppr_pending.pan_id = pan_id;
        ftdf_fppr_pending.addr.ext_address = ext_address;
#else /* FTDF_FPPR_DEFER_INVALIDATION */
        uint8_t entry;
        ftdf_boolean_t found = ftdf_fppr_lookup_ext_address(ext_address, &entry);
        ASSERT_WARNING(found);
        ftdf_fppr_set_ext_address_valid(entry, FTDF_FALSE);
#endif /* FTDF_FPPR_DEFER_INVALIDATION */
}

void ftdf_fp_fsm_clear_pending(void)
{
#if FTDF_FPPR_DEFER_INVALIDATION
        int n;
        if (ftdf_fppr_pending.addr_mode == FTDF_NO_ADDRESS) {
                return;
        }
        if (ftdf_fppr_pending.addr_mode == FTDF_SHORT_ADDRESS) {
                for (n = 0; n < FTDF_NR_OF_REQ_BUFFERS; n++) {
                        if (ftdf_tx_pending_list[n].addr_mode == FTDF_SHORT_ADDRESS) {
                                if ((ftdf_tx_pending_list[n].pan_id == ftdf_fppr_pending.pan_id) &&
                                        (ftdf_tx_pending_list[n].addr.short_address ==
                                                ftdf_fppr_pending.addr.short_address)) {
                                        return;
                                }
                        }
                }
                // Address not found.
                uint8_t entry;
                uint8_t short_addr_idx;
                ftdf_boolean_t found = ftdf_fppr_lookup_short_address(
                        ftdf_fppr_pending.addr.short_address, &entry, &short_addr_idx);
                ASSERT_WARNING(found);
                ftdf_fppr_set_short_address_valid(entry, short_addr_idx, FTDF_FALSE);
        } else if (ftdf_fppr_pending.addr_mode == FTDF_EXTENDED_ADDRESS) {
                for (n = 0; n < FTDF_NR_OF_REQ_BUFFERS; n++) {
                        if (ftdf_tx_pending_list[n].addr_mode == FTDF_EXTENDED_ADDRESS) {
                                if (ftdf_tx_pending_list[n].addr.ext_address ==
                                        ftdf_fppr_pending.addr.ext_address) {
                                        return;
                                }
                        }
                }
                // Address not found.
                uint8_t entry;
                ftdf_boolean_t found = ftdf_fppr_lookup_ext_address(ftdf_fppr_pending.addr.ext_address,
                        &entry);
                ASSERT_WARNING(found);
                ftdf_fppr_set_ext_address_valid(entry, FTDF_FALSE);
        } else {

        }
        ftdf_fppr_pending.addr_mode = FTDF_NO_ADDRESS;
#endif /* FTDF_FPPR_DEFER_INVALIDATION */
}

#endif /* #ifndef FTDF_LITE */
/*********************************** FPPR low-level access ****************************************/
void ftdf_fppr_reset(void)
{
        unsigned int i;
        for (i = 0; i < FTDF_FPPR_TABLE_ENTRIES; i++) {
                *REG_GET_ADDR_INDEXED(FTDF, FTDF_SIZE_AND_VAL_0_REG, 0x10, i) = 0;
        }
}

ftdf_short_address_t ftdf_fppr_get_short_address(uint8_t entry, uint8_t short_addr_idx)
{
        ASSERT_WARNING(entry < FTDF_FPPR_TABLE_ENTRIES);
        switch (short_addr_idx) {
        case 0:
                return (ftdf_short_address_t)
                        *REG_GET_ADDR_INDEXED(FTDF, FTDF_LONG_ADDR_0_0_REG, 0x10, entry) &
                        0x0000ffff;
        case 1:
                return (ftdf_short_address_t)
                        (*REG_GET_ADDR_INDEXED(FTDF, FTDF_LONG_ADDR_0_0_REG, 0x10, entry)  >> 16) &
                        0x0000ffff;
        case 2:
                return (ftdf_short_address_t)
                        *REG_GET_ADDR_INDEXED(FTDF, FTDF_LONG_ADDR_1_0_REG, 0x10, entry) &
                        0x0000ffff;
        case 3:
                return (ftdf_short_address_t)
                        (*REG_GET_ADDR_INDEXED(FTDF, FTDF_LONG_ADDR_1_0_REG, 0x10, entry) >> 16) &
                        0x0000ffff;
        default:
                ASSERT_WARNING(0);
        }
}

void ftdf_fppr_set_short_address(uint8_t entry, uint8_t short_addr_idx,
                                 ftdf_short_address_t short_address)
{
        ASSERT_WARNING(entry < FTDF_FPPR_TABLE_ENTRIES);
        uint32_t val32;
        volatile uint32_t *long_addr_reg;
        switch (short_addr_idx) {
        case 0:
                long_addr_reg = REG_GET_ADDR_INDEXED(FTDF, FTDF_LONG_ADDR_0_0_REG, 0x10, entry);
                val32 = *long_addr_reg;
                val32 &= 0xffff0000;
                val32 |= (short_address & 0x0000ffff);
                *long_addr_reg = val32;
                break;
        case 1:
                long_addr_reg = REG_GET_ADDR_INDEXED(FTDF, FTDF_LONG_ADDR_0_0_REG, 0x10, entry);
                val32 = *long_addr_reg;
                val32 &= 0x0000ffff;
                val32 |= (short_address & 0x0000ffff) << 16;
                *long_addr_reg = val32;
                break;
        case 2:
                long_addr_reg = REG_GET_ADDR_INDEXED(FTDF, FTDF_LONG_ADDR_1_0_REG, 0x10, entry);
                val32 = *long_addr_reg;
                val32 &= 0xffff0000;
                val32 |= (short_address & 0x0000ffff);
                *long_addr_reg = val32;
                break;
        case 3:
                long_addr_reg = REG_GET_ADDR_INDEXED(FTDF, FTDF_LONG_ADDR_1_0_REG, 0x10, entry);
                val32 = *long_addr_reg;
                val32 &= 0x0000ffff;
                val32 |= (short_address & 0x0000ffff) << 16;
                *long_addr_reg = val32;
                break;
        default:
                ASSERT_WARNING(0);
        }
}

ftdf_boolean_t ftdf_fppr_get_short_address_valid(uint8_t entry, uint8_t short_addr_idx)
{
        ASSERT_WARNING(entry < FTDF_FPPR_TABLE_ENTRIES);
        ASSERT_WARNING(short_addr_idx < 4);
        uint32_t val32 = *REG_GET_ADDR_INDEXED(FTDF, FTDF_SIZE_AND_VAL_0_REG, 0x10, entry);
        return ((val32 & (REG_MSK(FTDF, FTDF_SIZE_AND_VAL_0_REG, SHORT_LONGNOT) | (1 << short_addr_idx))) ==
                (REG_MSK(FTDF, FTDF_SIZE_AND_VAL_0_REG, SHORT_LONGNOT) | (1 << short_addr_idx))) ?
                FTDF_TRUE : FTDF_FALSE;
}

void ftdf_fppr_set_short_address_valid(uint8_t entry, uint8_t short_addr_idx,
        ftdf_boolean_t valid)
{
        ASSERT_WARNING(entry < FTDF_FPPR_TABLE_ENTRIES);
        ASSERT_WARNING(short_addr_idx < 4);
        volatile uint32_t *reg_val = REG_GET_ADDR_INDEXED(FTDF, FTDF_SIZE_AND_VAL_0_REG, 0x10, entry);
        if (valid) {
                *reg_val |= REG_MSK(FTDF, FTDF_SIZE_AND_VAL_0_REG, SHORT_LONGNOT) | (1 << short_addr_idx); /* Also indicate short address. */
        } else {
                *reg_val &= ~(1 << short_addr_idx);
        }
}

ftdf_ext_address_t ftdf_fppr_get_ext_address(uint8_t entry)
{
        ftdf_ext_address_t ext_address;
        ASSERT_WARNING(entry < FTDF_FPPR_TABLE_ENTRIES);
        ext_address = (ftdf_ext_address_t)
                (*REG_GET_ADDR_INDEXED(FTDF, FTDF_LONG_ADDR_1_0_REG, 0x10, entry)) << 32;
        ext_address |= (ftdf_ext_address_t)
                (*REG_GET_ADDR_INDEXED(FTDF, FTDF_LONG_ADDR_0_0_REG, 0x10, entry));

        return ext_address;
}

void ftdf_fppr_set_ext_address(uint8_t entry, ftdf_ext_address_t ext_address)
{
        ASSERT_WARNING(entry < FTDF_FPPR_TABLE_ENTRIES);
        *REG_GET_ADDR_INDEXED(FTDF, FTDF_LONG_ADDR_0_0_REG, 0x10, entry) = (uint32_t) ext_address;
        *REG_GET_ADDR_INDEXED(FTDF, FTDF_LONG_ADDR_1_0_REG, 0x10, entry) = (uint32_t) (ext_address >> 32);
}

ftdf_boolean_t ftdf_fppr_get_ext_address_valid(uint8_t entry)
{
        return (REG_GETF_INDEXED(FTDF, FTDF_SIZE_AND_VAL_0_REG, VALID_SA, 0x10, entry) == 0x1) ?
                FTDF_TRUE : FTDF_FALSE;
}

void ftdf_fppr_set_ext_address_valid(uint8_t entry, ftdf_boolean_t valid)
{
        ASSERT_WARNING(entry < FTDF_FPPR_TABLE_ENTRIES);
        if (valid) {
                /* Also indicate ext address. */
                *REG_GET_ADDR_INDEXED(FTDF, FTDF_SIZE_AND_VAL_0_REG, 0x10, entry) = 0x1;
        } else {
                *REG_GET_ADDR_INDEXED(FTDF, FTDF_SIZE_AND_VAL_0_REG, 0x10, entry) = 0x0;
        }
}

ftdf_boolean_t ftdf_fppr_get_free_short_address(uint8_t *entry, uint8_t *short_addr_idx)
{
        int i, j;
        uint32_t size_and_val;
        int empty_entry;
        bool empty_found = false, non_empty_found = false;
        for (i = 0; i < FTDF_FPPR_TABLE_ENTRIES; i++) {
                size_and_val = *REG_GET_ADDR_INDEXED(FTDF, FTDF_SIZE_AND_VAL_0_REG, 0x10, i);
                if (size_and_val == 0x1) {
                        /* Check if there is a valid extended address.  */
                        continue;
                } else if ((size_and_val & REG_MSK(FTDF, FTDF_SIZE_AND_VAL_0_REG, SHORT_LONGNOT)) == 0) {
                        /* There is an invalid extended address, ignore SA valid bits  */
                        size_and_val = 0;
                } else {
                        /* There is a SA. We are interested in bits V0 - V3. */
                        size_and_val &= 0xf;
                }

                /* Check if entire entry is free. */
                if (size_and_val == 0) {
                        /* We prefer to use partially full entries. Make note of this and
                         * continue. */
                        if (!empty_found) {
                                empty_entry = i;
                                empty_found = true;
                        }
                        continue;
                }
                /* Check for free short address entries. */
                size_and_val = (~size_and_val) & 0xf;
                j = 0;
                while (size_and_val) {
                        if (size_and_val & 0x1) {
                                non_empty_found = true;
                                break;
                        }
                        size_and_val >>= 1;
                        j++;
                }
                if (non_empty_found) {
                        break;
                }
        }
        if (non_empty_found) {
                *entry = i;
                *short_addr_idx = j;
        } else if (empty_found) {
                *entry = empty_entry;
                *short_addr_idx = 0;
        } else {
                return FTDF_FALSE;
        }
        return FTDF_TRUE;
}

ftdf_boolean_t ftdf_fppr_get_free_ext_address(uint8_t * entry)
{
        int i;
        uint32_t size_and_val;
        bool found = false;
        for (i = 0; i < FTDF_FPPR_TABLE_ENTRIES; i++) {
                size_and_val = *REG_GET_ADDR_INDEXED(FTDF, FTDF_SIZE_AND_VAL_0_REG, 0x10, i);
                /* Check if there is no valid extended or short address.  */
                if (!size_and_val || (size_and_val == REG_MSK(FTDF, FTDF_SIZE_AND_VAL_0_REG, SHORT_LONGNOT))) {
                        found = true;
                        break;
                }
        }
        if (found) {
                *entry = i;
        } else {
                return FTDF_FALSE;
        }
        return FTDF_TRUE;
}

ftdf_boolean_t ftdf_fppr_lookup_short_address(ftdf_short_address_t short_addr, uint8_t *entry,
                                              uint8_t *short_addr_idx)
{
        uint8_t i;
        uint32_t size_and_val;
        uint32_t saPart;
        for (i = 0; i < FTDF_FPPR_TABLE_ENTRIES; i++) {
                size_and_val = *REG_GET_ADDR_INDEXED(FTDF, FTDF_SIZE_AND_VAL_0_REG, 0x10, i);
                /* Check if there is a valid short address. */
                if (!(size_and_val & REG_MSK(FTDF, FTDF_SIZE_AND_VAL_0_REG, SHORT_LONGNOT)) ||
                        !(size_and_val & REG_MSK(FTDF, FTDF_SIZE_AND_VAL_0_REG, VALID_SA))) {
                        continue;
                }
                saPart = REG_GETF_INDEXED(FTDF, FTDF_LONG_ADDR_0_0_REG, EXP_SA_L, 0x10, i);
                if (size_and_val & 0x1) {
                        if (short_addr == (ftdf_short_address_t) (saPart & 0x0000ffff)) {
                                *entry = i;
                                *short_addr_idx = 0;
                                return FTDF_TRUE;
                        }
                }
                if (size_and_val & 0x2) {
                        if (short_addr == (ftdf_short_address_t) ((saPart >> 16) & 0x0000ffff)) {
                                *entry = i;
                                *short_addr_idx = 1;
                                return FTDF_TRUE;
                        }
                }
                saPart = REG_GETF_INDEXED(FTDF, FTDF_LONG_ADDR_1_0_REG, EXP_SA_H, 0x10, i);
                if (size_and_val & 0x4) {
                        if (short_addr == (ftdf_short_address_t) (saPart & 0x0000ffff)) {
                                *entry = i;
                                *short_addr_idx = 2;
                                return FTDF_TRUE;
                        }
                }
                if (size_and_val & 0x8) {
                        if (short_addr == ((ftdf_short_address_t) (saPart >> 16) & 0x0000ffff)) {
                                *entry = i;
                                *short_addr_idx = 3;
                                return FTDF_TRUE;
                        }
                }
        }
        return FTDF_FALSE;
}

ftdf_boolean_t ftdf_fppr_lookup_ext_address(ftdf_ext_address_t ext_addr, uint8_t *entry)
{
        uint8_t i;
        uint32_t size_and_val;
        uint32_t ext_addr_hi, ext_addr_lo;
        ext_addr_hi = (uint32_t) ((ext_addr >> 32) & 0xffffffff);
        ext_addr_lo = (uint32_t) (ext_addr & 0xffffffff);
        for (i = 0; i < FTDF_FPPR_TABLE_ENTRIES; i++) {
                size_and_val = *REG_GET_ADDR_INDEXED(FTDF, FTDF_SIZE_AND_VAL_0_REG, 0x10, i);
                /* Check if there is a valid extended address. */
                if (size_and_val != 0x1) {
                        continue;
                }
                if ((ext_addr_lo == REG_GETF_INDEXED(FTDF, FTDF_LONG_ADDR_0_0_REG, EXP_SA_L, 0x10, i)) &&
                        (ext_addr_hi == REG_GETF_INDEXED(FTDF, FTDF_LONG_ADDR_1_0_REG, EXP_SA_H, 0x10, i))) {
                        *entry = i;
                        return FTDF_TRUE;
                }
        }
        return FTDF_FALSE;
}

#endif /* FTDF_FP_BIT_MODE == FTDF_FP_BIT_MODE_AUTO */

void ftdf_fppr_set_mode(ftdf_boolean_t match_fp, ftdf_boolean_t fp_override, ftdf_boolean_t fp_force)
{
        REG_SETF(FTDF, FTDF_LMAC_CONTROL_3_REG, ADDR_TAB_MATCH_FP_VALUE, match_fp ? 1 : 0);
        REG_SETF(FTDF, FTDF_LMAC_CONTROL_3_REG, FP_OVERRIDE,fp_override ? 1 : 0);
        REG_SETF(FTDF, FTDF_LMAC_CONTROL_3_REG, FP_FORCE_VALUE, fp_force ? 1 : 0);
}

#if FTDF_FP_BIT_TEST_MODE
void ftdf_fppr_get_mode(ftdf_boolean_t *match_fp, ftdf_boolean_t *fp_override, ftdf_boolean_t *fp_force)
{
        *match_fp = REG_GETF(FTDF, FTDF_LMAC_CONTROL_3_REG, ADDR_TAB_MATCH_FP_VALUE) ? FTDF_TRUE : FTDF_FALSE;
        *fp_override = REG_GETF(FTDF, FTDF_LMAC_CONTROL_3_REG, FP_OVERRIDE) ? FTDF_TRUE : FTDF_FALSE;
        *fp_force = REG_GETF(FTDF, FTDF_LMAC_CONTROL_3_REG, FP_FORCE_VALUE) ? FTDF_TRUE : FTDF_FALSE;
}
#endif // FTDF_FP_BIT_TEST_MODE

void ftdf_lpdp_enable(ftdf_boolean_t enable)
{
        REG_SETF(FTDF, FTDF_LMAC_CONTROL_3_REG, FTDF_LPDP_ENABLE, enable ? 1 : 0);
}

#if FTDF_FP_BIT_TEST_MODE
ftdf_boolean_t ftdf_lpdp_is_enabled(void)
{
        return REG_GETF(FTDF, FTDF_LMAC_CONTROL_3_REG, FTDF_LPDP_ENABLE) ? FTDF_TRUE : FTDF_FALSE;
}
#endif

#if FTDF_USE_SLEEP_DURING_BACKOFF

static inline void ftdf_sdb_save_state(void)
{
        volatile uint32_t *tx_fifo_ptr = &FTDF->FTDF_TX_FIFO_0_0_REG;
        uint32_t * dstPtr = (uint32_t *) ftdf_sdb.buffer;
        uint8_t word_length_rem;

        ftdf_sdb.nr_of_backoffs = REG_GETF(FTDF, FTDF_LMAC_CONTROL_STATUS_REG, CSMA_CA_NB_STAT);

        /* Read first 4 bytes. */
        *dstPtr++ = *tx_fifo_ptr++;

        ASSERT_WARNING((ftdf_sdb.buffer[0] >= 3) && (ftdf_sdb.buffer[0] < FTDF_BUFFER_LENGTH));
        /* The length is the buffer length excluding the length byte itself */
        word_length_rem = (ftdf_sdb.buffer[0] + 4) / 4 - 1; /* 1 word we already read */

        while (word_length_rem--) {
                *dstPtr++ = *tx_fifo_ptr++;
        }

        ftdf_sdb.metadata_0 = FTDF->FTDF_TX_META_DATA_0_0_REG;
        ftdf_sdb.metadata_1 = FTDF->FTDF_TX_META_DATA_1_0_REG;

        ftdf_sdb.phy_csma_ca_attr = FTDF->FTDF_LMAC_CONTROL_5_REG;
}

static inline void ftdf_sdb_resume(void)
{
        volatile uint32_t *tx_fifo_ptr = &FTDF->FTDF_TX_FIFO_0_0_REG;
        uint32_t * src_ptr = (uint32_t *) ftdf_sdb.buffer;

        uint8_t word_length_rem;

        REG_SETF(FTDF, FTDF_LMAC_CONTROL_11_REG, CSMA_CA_NB_VAL, ftdf_sdb.nr_of_backoffs);

        REG_SETF(FTDF, FTDF_LMAC_CONTROL_OS_REG, CSMA_CA_RESUME_SET, 1);

        ASSERT_WARNING((ftdf_sdb.buffer[0] >= 3) && (ftdf_sdb.buffer[0] < FTDF_BUFFER_LENGTH));

        /* The length is the buffer length excluding the length byte itself */
        word_length_rem = (ftdf_sdb.buffer[0] + 4) / 4;

        while (word_length_rem--) {
                *tx_fifo_ptr++ = *src_ptr++;
        }

        REG_SET_MASKED(FTDF, FTDF_LMAC_CONTROL_5_REG,
               (REG_MSK(FTDF, FTDF_LMAC_CONTROL_5_REG, PHYCSMACAATTR_DEM_PTI) |
               REG_MSK(FTDF, FTDF_LMAC_CONTROL_5_REG, PHYCSMACAATTR_CN) |
               REG_MSK(FTDF, FTDF_LMAC_CONTROL_5_REG, PHYCSMACAATTR_CALCAP) |
               REG_MSK(FTDF, FTDF_LMAC_CONTROL_5_REG, PHYCSMACAATTR_RF_GPIO_PINS) |
               REG_MSK(FTDF, FTDF_LMAC_CONTROL_5_REG, PHYCSMACAATTR_HSI)), ftdf_sdb.phy_csma_ca_attr);

        FTDF->FTDF_TX_META_DATA_0_0_REG = ftdf_sdb.metadata_0;

        FTDF->FTDF_TX_META_DATA_1_0_REG = ftdf_sdb.metadata_1;

        REG_SETF(FTDF, FTDF_TX_SET_OS_REG, TX_FLAG_SET,
                 (REG_GETF(FTDF, FTDF_TX_SET_OS_REG, TX_FLAG_SET) | (1 << FTDF_TX_DATA_BUFFER)));
}

static inline void ftdf_sdb_reset(void)
{
        REG_SETF(FTDF, FTDF_LMAC_CONTROL_OS_REG, CSMA_CA_RESUME_CLEAR, 1);
}

static inline void ftdf_sdb_set_cca_retry_time(void)
{
        ftdf_time_t timestamp = REG_GETF(FTDF, FTDF_SYMBOLTIMESNAPSHOTVAL_REG, SYMBOLTIMESNAPSHOTVAL);
        ftdf_time_t bo_stat = REG_GETF(FTDF, FTDF_LMAC_CONTROL_STATUS_REG, CSMA_CA_BO_STAT) *
                FTDF_UNIT_BACKOFF_PERIOD;
        ftdf_sdb.cca_retry_time = timestamp + bo_stat;
}

void ftdf_sdb_fsm_reset(void) {
        ftdf_sdb_reset();
        ftdf_sdb.state = FTDF_SDB_STATE_INIT;
}

void ftdf_sdb_fsm_backoff_irq(void)
{
#if !defined(FTDF_NO_CSL) || !defined(FTDF_NO_TSCH)
        if (ftdf_pib.le_enabled || ftdf_pib.tsch_enabled) {
                return;
        }
#endif
        switch (ftdf_sdb.state) {
        case FTDF_SDB_STATE_RESUMING:
                ftdf_sdb_reset();
        case FTDF_SDB_STATE_INIT:
        case FTDF_SDB_STATE_BACKING_OFF:
                ftdf_sdb_set_cca_retry_time();
                ftdf_sdb.state = FTDF_SDB_STATE_BACKING_OFF;
                break;
        default:
                ASSERT_WARNING(0);
        }
}

void ftdf_sdb_fsm_sleep(void)
{
#if !defined(FTDF_NO_CSL) || !defined(FTDF_NO_TSCH)
        if (ftdf_pib.le_enabled || ftdf_pib.tsch_enabled) {
                return;
        }
#endif
        switch (ftdf_sdb.state) {
        case FTDF_SDB_STATE_BACKING_OFF:
                ftdf_sdb_save_state();
                ftdf_sdb.state = FTDF_SDB_STATE_WAITING_WAKE_UP_IRQ;
                break;
        case FTDF_SDB_STATE_INIT:
                break;
        default:
                ASSERT_WARNING(0);
        }
}

void ftdf_sdb_fsm_abort_sleep(void)
{
#if !defined(FTDF_NO_CSL) || !defined(FTDF_NO_TSCH)
        if (ftdf_pib.le_enabled || ftdf_pib.tsch_enabled) {
                return;
        }
#endif
        switch (ftdf_sdb.state) {
        case FTDF_SDB_STATE_BACKING_OFF:
                ftdf_sdb.state = FTDF_SDB_STATE_INIT;
                break;
        case FTDF_SDB_STATE_INIT:
        case FTDF_SDB_STATE_WAITING_WAKE_UP_IRQ:
        case FTDF_SDB_STATE_RESUMING:
                break;
        default:
                ASSERT_WARNING(0);
        }
}

void ftdf_sdb_fsm_wake_up_irq(void)
{
#if !defined(FTDF_NO_CSL) || !defined(FTDF_NO_TSCH)
        if (ftdf_pib.le_enabled || ftdf_pib.tsch_enabled) {
                return;
        }
#endif
        switch (ftdf_sdb.state) {
        case FTDF_SDB_STATE_WAITING_WAKE_UP_IRQ:
                ftdf_sdb.state = FTDF_SDB_STATE_RESUMING;
                break;
        case FTDF_SDB_STATE_INIT:
                break;
        default:
                ASSERT_WARNING(0);
        }
}

void ftdf_sdb_fsm_wake_up(void)
{
#if !defined(FTDF_NO_CSL) || !defined(FTDF_NO_TSCH)
        if (ftdf_pib.le_enabled || ftdf_pib.tsch_enabled) {
                return;
        }
#endif
        switch (ftdf_sdb.state) {
        case FTDF_SDB_STATE_RESUMING:
                ftdf_sdb_resume();
                break;
        case FTDF_SDB_STATE_WAITING_WAKE_UP_IRQ:
                break;
        case FTDF_SDB_STATE_INIT:
                break;
        default:
                ASSERT_WARNING(0);
        }
}

void ftdf_sdb_fsm_tx_irq(void)
{
#if !defined(FTDF_NO_CSL) || !defined(FTDF_NO_TSCH)
        if (ftdf_pib.le_enabled || ftdf_pib.tsch_enabled) {
                return;
        }
#endif
        switch (ftdf_sdb.state) {
        case FTDF_SDB_STATE_RESUMING:
                ftdf_sdb_reset();
                ftdf_sdb.state = FTDF_SDB_STATE_INIT;
                break;
        case FTDF_SDB_STATE_BACKING_OFF:
                ftdf_sdb.state = FTDF_SDB_STATE_INIT;
                break;
        case FTDF_SDB_STATE_INIT:
                break;
        default:
                ASSERT_WARNING(0);
        }
}

ftdf_usec_t ftdf_sdb_get_sleep_time(void)
{
        ftdf_usec_t sleep_time = ~((ftdf_usec_t) 0);
#if !defined(FTDF_NO_CSL) || !defined(FTDF_NO_TSCH)
        if (ftdf_pib.le_enabled || ftdf_pib.tsch_enabled) {
                return sleep_time;
        }
#endif
        switch (ftdf_sdb.state) {
        case FTDF_SDB_STATE_INIT:
                if ((REG_GETF(FTDF, FTDF_LMAC_CONTROL_STATUS_REG, LMACREADY4SLEEP) == 0) || ftdf_req_current) {
                        sleep_time = 0;
                } else {
                        sleep_time = ~((ftdf_usec_t) 0);
                }
                break;
        case FTDF_SDB_STATE_BACKING_OFF:
        {
                ftdf_time_t currentTime = REG_GETF(FTDF, FTDF_SYMBOLTIMESNAPSHOTVAL_REG, SYMBOLTIMESNAPSHOTVAL);
                if (currentTime <= ftdf_sdb.cca_retry_time) {
                        sleep_time = (ftdf_sdb.cca_retry_time - currentTime) * 16;
                } else {
                        sleep_time = (1 << 31) - (currentTime + ftdf_sdb.cca_retry_time) * 16;
                }
                if (sleep_time > 256 * FTDF_UNIT_BACKOFF_PERIOD * 16) {
                        /* We have exceeded the CCA retry time. Abort sleep and wait for Tx IRQ. */
                        sleep_time = 0;
                }
                break;
        }
        case FTDF_SDB_STATE_RESUMING:
                sleep_time = 0;
                break;
        case FTDF_SDB_STATE_WAITING_WAKE_UP_IRQ:
                sleep_time = ~((ftdf_usec_t) 0);
                break;
        default:
                ASSERT_WARNING(0);
        }
        return sleep_time;
}

#endif /* FTDF_USE_SLEEP_DURING_BACKOFF */

#if dg_configUSE_FTDF_DDPHY == 1
void ftdf_ddphy_set(uint16_t cca_reg)
{
        ftdf_critical_var();
        ftdf_enter_critical();
        /* We use the critical section here as protection for the global variable and the HW sleep
         * state. */
        ftdf_ddphy_cca_reg = cca_reg;
        /* Apply immediately if block is up. */
        if (REG_GETF(CRG_TOP, PMU_CTRL_REG, FTDF_SLEEP) == 0x0) {
                FTDF_DPHY->DDPHY_CCA_REG = ftdf_ddphy_cca_reg;
        }
        ftdf_exit_critical();
}

void ftdf_ddphy_restore(void)
{
        if (ftdf_ddphy_cca_reg) {
                /* Apply immediately if block is up. */
                ftdf_critical_var();
                ftdf_enter_critical();
                ASSERT_WARNING(REG_GETF(CRG_TOP, PMU_CTRL_REG, FTDF_SLEEP) == 0x0);
                FTDF_DPHY->DDPHY_CCA_REG = ftdf_ddphy_cca_reg;
                ftdf_exit_critical();
        }
}

void ftdf_ddphy_save(void)
{
        /* Apply immediately if block is up. */
        ftdf_critical_var();
        ftdf_enter_critical();
        ASSERT_WARNING(REG_GETF(CRG_TOP, PMU_CTRL_REG, FTDF_SLEEP) == 0x0);
        ftdf_ddphy_cca_reg = FTDF_DPHY->DDPHY_CCA_REG;
        ftdf_exit_critical();
}
#endif
#endif /* CONFIG_USE_FTDF */
