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
#include "regmap.h"
#include "sdk_defs.h"

#if dg_configCOEX_ENABLE_CONFIG
#include "hw_coex.h"
#endif
typedef struct
{
    void*   addr;
    uint8_t size;
    void ( * getFunc )( void );
    void ( * setFunc )( void );
} PIBAttributeDef;

typedef struct
{
    PIBAttributeDef attributeDefs[ FTDF_NR_OF_PIB_ATTRIBUTES + 1 ];
} PIBAttributeTable;

struct FTDF_Pib                  FTDF_pib __attribute__ ( ( section( ".retention" ) ) );

#if FTDF_FP_BIT_MODE == FTDF_FP_BIT_MODE_AUTO
#if FTDF_FPPR_DEFER_INVALIDATION
static struct {
        FTDF_AddressMode        addrMode;
        FTDF_PANId              PANId;
        FTDF_Address            addr;
} FTDF_fpprPending __attribute__ ( ( section( ".retention" ) ) );
#endif /* FTDF_FPPR_DEFER_INVALIDATION */
#endif /* FTDF_FP_BIT_MODE == FTDF_FP_BIT_MODE_AUTO */

#ifndef FTDF_LITE
const FTDF_ChannelNumber         page0Channels[ FTDF_NR_OF_CHANNELS ] =
{ 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26 };
const FTDF_ChannelDescriptor     channelDescriptors[ 1 ] = { { 0, 16, (FTDF_ChannelNumber*) page0Channels } };
const FTDF_ChannelDescriptorList channelsSupported = { 1, (FTDF_ChannelDescriptor*) channelDescriptors };
#endif

const PIBAttributeTable          pibAttributeTable =
{
    .attributeDefs[ FTDF_PIB_EXTENDED_ADDRESS ].addr                       = &FTDF_pib.extAddress,
    .attributeDefs[ FTDF_PIB_EXTENDED_ADDRESS ].size                       = sizeof( FTDF_pib.extAddress ),
    .attributeDefs[ FTDF_PIB_EXTENDED_ADDRESS ].getFunc                    = FTDF_getExtAddress,
    .attributeDefs[ FTDF_PIB_EXTENDED_ADDRESS ].setFunc                    = FTDF_setExtAddress,
    .attributeDefs[ FTDF_PIB_ACK_WAIT_DURATION ].addr                      = &FTDF_pib.ackWaitDuration,
    .attributeDefs[ FTDF_PIB_ACK_WAIT_DURATION ].size                      = 0,
    .attributeDefs[ FTDF_PIB_ACK_WAIT_DURATION ].getFunc                   = FTDF_getAckWaitDuration,
    .attributeDefs[ FTDF_PIB_ACK_WAIT_DURATION ].setFunc                   = NULL,
#ifndef FTDF_LITE
    .attributeDefs[ FTDF_PIB_ASSOCIATION_PAN_COORD ].addr                  = &FTDF_pib.associatedPANCoord,
    .attributeDefs[ FTDF_PIB_ASSOCIATION_PAN_COORD ].size                  = sizeof( FTDF_pib.associatedPANCoord ),
    .attributeDefs[ FTDF_PIB_ASSOCIATION_PAN_COORD ].getFunc               = NULL,
    .attributeDefs[ FTDF_PIB_ASSOCIATION_PAN_COORD ].setFunc               = NULL,
    .attributeDefs[ FTDF_PIB_ASSOCIATION_PERMIT ].addr                     = &FTDF_pib.associationPermit,
    .attributeDefs[ FTDF_PIB_ASSOCIATION_PERMIT ].size                     = sizeof( FTDF_pib.associationPermit ),
    .attributeDefs[ FTDF_PIB_ASSOCIATION_PERMIT ].getFunc                  = NULL,
    .attributeDefs[ FTDF_PIB_ASSOCIATION_PERMIT ].setFunc                  = NULL,
    .attributeDefs[ FTDF_PIB_AUTO_REQUEST ].addr                           = &FTDF_pib.autoRequest,
    .attributeDefs[ FTDF_PIB_AUTO_REQUEST ].size                           = sizeof( FTDF_pib.autoRequest ),
    .attributeDefs[ FTDF_PIB_AUTO_REQUEST ].getFunc                        = NULL,
    .attributeDefs[ FTDF_PIB_AUTO_REQUEST ].setFunc                        = NULL,
    .attributeDefs[ FTDF_PIB_BATT_LIFE_EXT ].addr                          = &FTDF_pib.battLifeExt,
    .attributeDefs[ FTDF_PIB_BATT_LIFE_EXT ].size                          = sizeof( FTDF_pib.battLifeExt ),
    .attributeDefs[ FTDF_PIB_BATT_LIFE_EXT ].getFunc                       = NULL,
    .attributeDefs[ FTDF_PIB_BATT_LIFE_EXT ].setFunc                       = NULL,
    .attributeDefs[ FTDF_PIB_BATT_LIFE_EXT_PERIODS ].addr                  = &FTDF_pib.battLifeExtPeriods,
    .attributeDefs[ FTDF_PIB_BATT_LIFE_EXT_PERIODS ].size                  = sizeof( FTDF_pib.battLifeExtPeriods ),
    .attributeDefs[ FTDF_PIB_BATT_LIFE_EXT_PERIODS ].getFunc               = NULL,
    .attributeDefs[ FTDF_PIB_BATT_LIFE_EXT_PERIODS ].setFunc               = NULL,
    .attributeDefs[ FTDF_PIB_BEACON_PAYLOAD ].addr                         = &FTDF_pib.beaconPayload,
    .attributeDefs[ FTDF_PIB_BEACON_PAYLOAD ].size                         = sizeof( FTDF_pib.beaconPayload ),
    .attributeDefs[ FTDF_PIB_BEACON_PAYLOAD ].getFunc                      = NULL,
    .attributeDefs[ FTDF_PIB_BEACON_PAYLOAD ].setFunc                      = NULL,
    .attributeDefs[ FTDF_PIB_BEACON_PAYLOAD_LENGTH ].addr                  = &FTDF_pib.beaconPayloadLength,
    .attributeDefs[ FTDF_PIB_BEACON_PAYLOAD_LENGTH ].size                  = sizeof( FTDF_pib.beaconPayloadLength ),
    .attributeDefs[ FTDF_PIB_BEACON_PAYLOAD_LENGTH ].getFunc               = NULL,
    .attributeDefs[ FTDF_PIB_BEACON_PAYLOAD_LENGTH ].setFunc               = NULL,
    .attributeDefs[ FTDF_PIB_BEACON_ORDER ].addr                           = &FTDF_pib.beaconOrder,
    .attributeDefs[ FTDF_PIB_BEACON_ORDER ].size                           = 0,
    .attributeDefs[ FTDF_PIB_BEACON_ORDER ].getFunc                        = NULL,
    .attributeDefs[ FTDF_PIB_BEACON_ORDER ].setFunc                        = NULL,
    .attributeDefs[ FTDF_PIB_BEACON_TX_TIME ].addr                         = &FTDF_pib.beaconTxTime,
    .attributeDefs[ FTDF_PIB_BEACON_TX_TIME ].size                         = sizeof( FTDF_pib.beaconTxTime ),
    .attributeDefs[ FTDF_PIB_BEACON_TX_TIME ].getFunc                      = NULL,
    .attributeDefs[ FTDF_PIB_BEACON_TX_TIME ].setFunc                      = NULL,
    .attributeDefs[ FTDF_PIB_BSN ].addr                                    = &FTDF_pib.BSN,
    .attributeDefs[ FTDF_PIB_BSN ].size                                    = sizeof( FTDF_pib.BSN ),
    .attributeDefs[ FTDF_PIB_BSN ].getFunc                                 = NULL,
    .attributeDefs[ FTDF_PIB_BSN ].setFunc                                 = NULL,
    .attributeDefs[ FTDF_PIB_COORD_EXTENDED_ADDRESS ].addr                 = &FTDF_pib.coordExtAddress,
    .attributeDefs[ FTDF_PIB_COORD_EXTENDED_ADDRESS ].size                 = sizeof( FTDF_pib.coordExtAddress ),
    .attributeDefs[ FTDF_PIB_COORD_EXTENDED_ADDRESS ].getFunc              = NULL,
    .attributeDefs[ FTDF_PIB_COORD_EXTENDED_ADDRESS ].setFunc              = NULL,
    .attributeDefs[ FTDF_PIB_COORD_SHORT_ADDRESS ].addr                    = &FTDF_pib.coordShortAddress,
    .attributeDefs[ FTDF_PIB_COORD_SHORT_ADDRESS ].size                    = sizeof( FTDF_pib.coordShortAddress ),
    .attributeDefs[ FTDF_PIB_COORD_SHORT_ADDRESS ].getFunc                 = NULL,
    .attributeDefs[ FTDF_PIB_COORD_SHORT_ADDRESS ].setFunc                 = NULL,
    .attributeDefs[ FTDF_PIB_DSN ].addr                                    = &FTDF_pib.DSN,
    .attributeDefs[ FTDF_PIB_DSN ].size                                    = sizeof( FTDF_pib.DSN ),
    .attributeDefs[ FTDF_PIB_DSN ].getFunc                                 = NULL,
    .attributeDefs[ FTDF_PIB_DSN ].setFunc                                 = NULL,
    .attributeDefs[ FTDF_PIB_GTS_PERMIT ].addr                             = &FTDF_pib.GTSPermit,
    .attributeDefs[ FTDF_PIB_GTS_PERMIT ].size                             = 0,
    .attributeDefs[ FTDF_PIB_GTS_PERMIT ].getFunc                          = NULL,
    .attributeDefs[ FTDF_PIB_GTS_PERMIT ].setFunc                          = NULL,
#endif /* !FTDF_LITE */
    .attributeDefs[ FTDF_PIB_MAX_BE ].addr                                 = &FTDF_pib.maxBE,
    .attributeDefs[ FTDF_PIB_MAX_BE ].size                                 = sizeof( FTDF_pib.maxBE ),
    .attributeDefs[ FTDF_PIB_MAX_BE ].getFunc                              = NULL,
    .attributeDefs[ FTDF_PIB_MAX_BE ].setFunc                              = FTDF_setMaxBE,
    .attributeDefs[ FTDF_PIB_MAX_CSMA_BACKOFFS ].addr                      = &FTDF_pib.maxCSMABackoffs,
    .attributeDefs[ FTDF_PIB_MAX_CSMA_BACKOFFS ].size                      = sizeof( FTDF_pib.maxCSMABackoffs ),
    .attributeDefs[ FTDF_PIB_MAX_CSMA_BACKOFFS ].getFunc                   = NULL,
    .attributeDefs[ FTDF_PIB_MAX_CSMA_BACKOFFS ].setFunc                   = FTDF_setMaxCSMABackoffs,
    .attributeDefs[ FTDF_PIB_MAX_FRAME_TOTAL_WAIT_TIME ].addr              = &FTDF_pib.maxFrameTotalWaitTime,
    .attributeDefs[ FTDF_PIB_MAX_FRAME_TOTAL_WAIT_TIME ].size              = sizeof( FTDF_pib.maxFrameTotalWaitTime ),
    .attributeDefs[ FTDF_PIB_MAX_FRAME_TOTAL_WAIT_TIME ].getFunc           = FTDF_getMaxFrameTotalWaitTime,
    .attributeDefs[ FTDF_PIB_MAX_FRAME_TOTAL_WAIT_TIME ].setFunc           = FTDF_setMaxFrameTotalWaitTime,
    .attributeDefs[ FTDF_PIB_MAX_FRAME_RETRIES ].addr                      = &FTDF_pib.maxFrameRetries,
    .attributeDefs[ FTDF_PIB_MAX_FRAME_RETRIES ].size                      = sizeof( FTDF_pib.maxFrameRetries ),
    .attributeDefs[ FTDF_PIB_MAX_FRAME_RETRIES ].getFunc                   = NULL,
    .attributeDefs[ FTDF_PIB_MAX_FRAME_RETRIES ].setFunc                   = NULL,
    .attributeDefs[ FTDF_PIB_MIN_BE ].addr                                 = &FTDF_pib.minBE,
    .attributeDefs[ FTDF_PIB_MIN_BE ].size                                 = sizeof( FTDF_pib.minBE ),
    .attributeDefs[ FTDF_PIB_MIN_BE ].getFunc                              = NULL,
    .attributeDefs[ FTDF_PIB_MIN_BE ].setFunc                              = FTDF_setMinBE,
#ifndef FTDF_LITE
    .attributeDefs[ FTDF_PIB_LIFS_PERIOD ].addr                            = &FTDF_pib.LIFSPeriod,
    .attributeDefs[ FTDF_PIB_LIFS_PERIOD ].size                            = 0,
    .attributeDefs[ FTDF_PIB_LIFS_PERIOD ].getFunc                         = NULL,
    .attributeDefs[ FTDF_PIB_LIFS_PERIOD ].setFunc                         = NULL,
    .attributeDefs[ FTDF_PIB_SIFS_PERIOD ].addr                            = &FTDF_pib.SIFSPeriod,
    .attributeDefs[ FTDF_PIB_SIFS_PERIOD ].size                            = 0,
    .attributeDefs[ FTDF_PIB_SIFS_PERIOD ].getFunc                         = NULL,
    .attributeDefs[ FTDF_PIB_SIFS_PERIOD ].setFunc                         = NULL,
#endif
    .attributeDefs[ FTDF_PIB_PAN_ID ].addr                                 = &FTDF_pib.PANId,
    .attributeDefs[ FTDF_PIB_PAN_ID ].size                                 = sizeof( FTDF_pib.PANId ),
    .attributeDefs[ FTDF_PIB_PAN_ID ].getFunc                              = FTDF_getPANId,
    .attributeDefs[ FTDF_PIB_PAN_ID ].setFunc                              = FTDF_setPANId,
#ifndef FTDF_LITE
    .attributeDefs[ FTDF_PIB_PROMISCUOUS_MODE ].addr                       = &FTDF_pib.promiscuousMode,
    .attributeDefs[ FTDF_PIB_PROMISCUOUS_MODE ].size                       = sizeof( FTDF_pib.promiscuousMode ),
    .attributeDefs[ FTDF_PIB_PROMISCUOUS_MODE ].getFunc                    = NULL,
    .attributeDefs[ FTDF_PIB_PROMISCUOUS_MODE ].setFunc                    = NULL,
    .attributeDefs[ FTDF_PIB_RESPONSE_WAIT_TIME ].addr                     = &FTDF_pib.responseWaitTime,
    .attributeDefs[ FTDF_PIB_RESPONSE_WAIT_TIME ].size                     = sizeof( FTDF_pib.responseWaitTime ),
    .attributeDefs[ FTDF_PIB_RESPONSE_WAIT_TIME ].getFunc                  = NULL,
    .attributeDefs[ FTDF_PIB_RESPONSE_WAIT_TIME ].setFunc                  = NULL,
#endif /* !FTDF_LITE */
    .attributeDefs[ FTDF_PIB_RX_ON_WHEN_IDLE ].addr                        = &FTDF_pib.rxOnWhenIdle,
    .attributeDefs[ FTDF_PIB_RX_ON_WHEN_IDLE ].size                        = sizeof( FTDF_pib.rxOnWhenIdle ),
    .attributeDefs[ FTDF_PIB_RX_ON_WHEN_IDLE ].getFunc                     = FTDF_getRxOnWhenIdle,
    .attributeDefs[ FTDF_PIB_RX_ON_WHEN_IDLE ].setFunc                     = FTDF_setRxOnWhenIdle,
#ifndef FTDF_LITE
    .attributeDefs[ FTDF_PIB_SECURITY_ENABLED ].addr                       = &FTDF_pib.securityEnabled,
    .attributeDefs[ FTDF_PIB_SECURITY_ENABLED ].size                       = sizeof( FTDF_pib.securityEnabled ),
    .attributeDefs[ FTDF_PIB_SECURITY_ENABLED ].getFunc                    = NULL,
    .attributeDefs[ FTDF_PIB_SECURITY_ENABLED ].setFunc                    = NULL,
#endif /* !FTDF_LITE */
    .attributeDefs[ FTDF_PIB_SHORT_ADDRESS ].addr                          = &FTDF_pib.shortAddress,
    .attributeDefs[ FTDF_PIB_SHORT_ADDRESS ].size                          = sizeof( FTDF_pib.shortAddress ),
    .attributeDefs[ FTDF_PIB_SHORT_ADDRESS ].getFunc                       = NULL,
    .attributeDefs[ FTDF_PIB_SHORT_ADDRESS ].setFunc                       = FTDF_setShortAddress,
#ifndef FTDF_LITE
    .attributeDefs[ FTDF_PIB_SUPERFRAME_ORDER ].addr                       = &FTDF_pib.superframeOrder,
    .attributeDefs[ FTDF_PIB_SUPERFRAME_ORDER ].size                       = 0,
    .attributeDefs[ FTDF_PIB_SUPERFRAME_ORDER ].getFunc                    = NULL,
    .attributeDefs[ FTDF_PIB_SUPERFRAME_ORDER ].setFunc                    = NULL,
    .attributeDefs[ FTDF_PIB_SYNC_SYMBOL_OFFSET ].addr                     = &FTDF_pib.syncSymbolOffset,
    .attributeDefs[ FTDF_PIB_SYNC_SYMBOL_OFFSET ].size                     = 0,
    .attributeDefs[ FTDF_PIB_SYNC_SYMBOL_OFFSET ].getFunc                  = NULL,
    .attributeDefs[ FTDF_PIB_SYNC_SYMBOL_OFFSET ].setFunc                  = NULL,
    .attributeDefs[ FTDF_PIB_TIMESTAMP_SUPPORTED ].addr                    = &FTDF_pib.timestampSupported,
    .attributeDefs[ FTDF_PIB_TIMESTAMP_SUPPORTED ].size                    = 0,
    .attributeDefs[ FTDF_PIB_TIMESTAMP_SUPPORTED ].getFunc                 = NULL,
    .attributeDefs[ FTDF_PIB_TIMESTAMP_SUPPORTED ].setFunc                 = NULL,
    .attributeDefs[ FTDF_PIB_TRANSACTION_PERSISTENCE_TIME ].addr           = &FTDF_pib.transactionPersistenceTime,
    .attributeDefs[ FTDF_PIB_TRANSACTION_PERSISTENCE_TIME ].size           =
        sizeof( FTDF_pib.transactionPersistenceTime ),
    .attributeDefs[ FTDF_PIB_TRANSACTION_PERSISTENCE_TIME ].getFunc        = NULL,
    .attributeDefs[ FTDF_PIB_TRANSACTION_PERSISTENCE_TIME ].setFunc        = NULL,
    .attributeDefs[ FTDF_PIB_ENH_ACK_WAIT_DURATION ].addr                  = &FTDF_pib.enhAckWaitDuration,
    .attributeDefs[ FTDF_PIB_ENH_ACK_WAIT_DURATION ].size                  = sizeof( FTDF_pib.enhAckWaitDuration ),
    .attributeDefs[ FTDF_PIB_ENH_ACK_WAIT_DURATION ].getFunc               = FTDF_getEnhAckWaitDuration,
    .attributeDefs[ FTDF_PIB_ENH_ACK_WAIT_DURATION ].setFunc               = FTDF_setEnhAckWaitDuration,
    .attributeDefs[ FTDF_PIB_IMPLICIT_BROADCAST ].addr                     = &FTDF_pib.implicitBroadcast,
    .attributeDefs[ FTDF_PIB_IMPLICIT_BROADCAST ].size                     = sizeof( FTDF_pib.implicitBroadcast ),
    .attributeDefs[ FTDF_PIB_IMPLICIT_BROADCAST ].getFunc                  = FTDF_getImplicitBroadcast,
    .attributeDefs[ FTDF_PIB_IMPLICIT_BROADCAST ].setFunc                  = FTDF_setImplicitBroadcast,
    .attributeDefs[ FTDF_PIB_SIMPLE_ADDRESS ].addr                         = &FTDF_pib.simpleAddress,
    .attributeDefs[ FTDF_PIB_SIMPLE_ADDRESS ].size                         = sizeof( FTDF_pib.simpleAddress ),
    .attributeDefs[ FTDF_PIB_SIMPLE_ADDRESS ].getFunc                      = NULL,
    .attributeDefs[ FTDF_PIB_SIMPLE_ADDRESS ].setFunc                      = FTDF_setSimpleAddress,
    .attributeDefs[ FTDF_PIB_DISCONNECT_TIME ].addr                        = &FTDF_pib.disconnectTime,
    .attributeDefs[ FTDF_PIB_DISCONNECT_TIME ].size                        = sizeof( FTDF_pib.disconnectTime ),
    .attributeDefs[ FTDF_PIB_DISCONNECT_TIME ].getFunc                     = NULL,
    .attributeDefs[ FTDF_PIB_DISCONNECT_TIME ].setFunc                     = NULL,
    .attributeDefs[ FTDF_PIB_JOIN_PRIORITY ].addr                          = &FTDF_pib.joinPriority,
    .attributeDefs[ FTDF_PIB_JOIN_PRIORITY ].size                          = sizeof( FTDF_pib.joinPriority ),
    .attributeDefs[ FTDF_PIB_JOIN_PRIORITY ].getFunc                       = NULL,
    .attributeDefs[ FTDF_PIB_JOIN_PRIORITY ].setFunc                       = NULL,
    .attributeDefs[ FTDF_PIB_ASN ].addr                                    = &FTDF_pib.ASN,
    .attributeDefs[ FTDF_PIB_ASN ].size                                    = sizeof( FTDF_pib.ASN ),
    .attributeDefs[ FTDF_PIB_ASN ].getFunc                                 = NULL,
    .attributeDefs[ FTDF_PIB_ASN ].setFunc                                 = NULL,
    .attributeDefs[ FTDF_PIB_NO_HL_BUFFERS ].addr                          = &FTDF_pib.noHLBuffers,
    .attributeDefs[ FTDF_PIB_NO_HL_BUFFERS ].size                          = sizeof( FTDF_pib.noHLBuffers ),
    .attributeDefs[ FTDF_PIB_NO_HL_BUFFERS ].getFunc                       = NULL,
    .attributeDefs[ FTDF_PIB_NO_HL_BUFFERS ].setFunc                       = NULL,
    .attributeDefs[ FTDF_PIB_SLOTFRAME_TABLE ].addr                        = &FTDF_pib.slotframeTable,
    .attributeDefs[ FTDF_PIB_SLOTFRAME_TABLE ].size                        = 0,
    .attributeDefs[ FTDF_PIB_SLOTFRAME_TABLE ].getFunc                     = NULL,
    .attributeDefs[ FTDF_PIB_SLOTFRAME_TABLE ].setFunc                     = NULL,
    .attributeDefs[ FTDF_PIB_LINK_TABLE ].addr                             = &FTDF_pib.linkTable,
    .attributeDefs[ FTDF_PIB_LINK_TABLE ].size                             = 0,
    .attributeDefs[ FTDF_PIB_LINK_TABLE ].getFunc                          = NULL,
    .attributeDefs[ FTDF_PIB_LINK_TABLE ].setFunc                          = NULL,
    .attributeDefs[ FTDF_PIB_TIMESLOT_TEMPLATE ].addr                      = &FTDF_pib.timeslotTemplate,
    .attributeDefs[ FTDF_PIB_TIMESLOT_TEMPLATE ].size                      = sizeof( FTDF_pib.timeslotTemplate ),
    .attributeDefs[ FTDF_PIB_TIMESLOT_TEMPLATE ].getFunc                   = NULL,
#ifdef FTDF_NO_TSCH
    .attributeDefs[ FTDF_PIB_TIMESLOT_TEMPLATE ].setFunc                   = NULL,
#else
    .attributeDefs[ FTDF_PIB_TIMESLOT_TEMPLATE ].setFunc                   = FTDF_setTimeslotTemplate,
#endif /* FTDF_NO_TSCH */
    .attributeDefs[ FTDF_PIB_HOPPINGSEQUENCE_ID ].addr                     = &FTDF_pib.HoppingSequenceId,
    .attributeDefs[ FTDF_PIB_HOPPINGSEQUENCE_ID ].size                     = sizeof( FTDF_pib.HoppingSequenceId ),
    .attributeDefs[ FTDF_PIB_HOPPINGSEQUENCE_ID ].getFunc                  = NULL,
    .attributeDefs[ FTDF_PIB_HOPPINGSEQUENCE_ID ].setFunc                  = NULL,
    .attributeDefs[ FTDF_PIB_CHANNEL_PAGE ].addr                           = &FTDF_pib.channelPage,
    .attributeDefs[ FTDF_PIB_CHANNEL_PAGE ].size                           = sizeof( FTDF_pib.channelPage ),
    .attributeDefs[ FTDF_PIB_CHANNEL_PAGE ].getFunc                        = NULL,
    .attributeDefs[ FTDF_PIB_CHANNEL_PAGE ].setFunc                        = NULL,
    .attributeDefs[ FTDF_PIB_NUMBER_OF_CHANNELS ].addr                     = &FTDF_pib.numberOfChannels,
    .attributeDefs[ FTDF_PIB_NUMBER_OF_CHANNELS ].size                     = sizeof( FTDF_pib.numberOfChannels ),
    .attributeDefs[ FTDF_PIB_NUMBER_OF_CHANNELS ].getFunc                  = NULL,
    .attributeDefs[ FTDF_PIB_NUMBER_OF_CHANNELS ].setFunc                  = NULL,
    .attributeDefs[ FTDF_PIB_PHY_CONFIGURATION ].addr                      = &FTDF_pib.phyConfiguration,
    .attributeDefs[ FTDF_PIB_PHY_CONFIGURATION ].size                      = sizeof( FTDF_pib.phyConfiguration ),
    .attributeDefs[ FTDF_PIB_PHY_CONFIGURATION ].getFunc                   = NULL,
    .attributeDefs[ FTDF_PIB_PHY_CONFIGURATION ].setFunc                   = NULL,
    .attributeDefs[ FTDF_PIB_EXTENTED_BITMAP ].addr                        = &FTDF_pib.extendedBitmap,
    .attributeDefs[ FTDF_PIB_EXTENTED_BITMAP ].size                        = sizeof( FTDF_pib.extendedBitmap ),
    .attributeDefs[ FTDF_PIB_EXTENTED_BITMAP ].getFunc                     = NULL,
    .attributeDefs[ FTDF_PIB_EXTENTED_BITMAP ].setFunc                     = NULL,
    .attributeDefs[ FTDF_PIB_HOPPING_SEQUENCE_LENGTH ].addr                = &FTDF_pib.hoppingSequenceLength,
    .attributeDefs[ FTDF_PIB_HOPPING_SEQUENCE_LENGTH ].size                = sizeof( FTDF_pib.hoppingSequenceLength ),
    .attributeDefs[ FTDF_PIB_HOPPING_SEQUENCE_LENGTH ].getFunc             = NULL,
    .attributeDefs[ FTDF_PIB_HOPPING_SEQUENCE_LENGTH ].setFunc             = NULL,
    .attributeDefs[ FTDF_PIB_HOPPING_SEQUENCE_LIST ].addr                  = FTDF_pib.hoppingSequenceList,
    .attributeDefs[ FTDF_PIB_HOPPING_SEQUENCE_LIST ].size                  = sizeof( FTDF_pib.hoppingSequenceList ),
    .attributeDefs[ FTDF_PIB_HOPPING_SEQUENCE_LIST ].getFunc               = NULL,
    .attributeDefs[ FTDF_PIB_HOPPING_SEQUENCE_LIST ].setFunc               = NULL,
    .attributeDefs[ FTDF_PIB_CURRENT_HOP ].addr                            = &FTDF_pib.currentHop,
    .attributeDefs[ FTDF_PIB_CURRENT_HOP ].size                            = sizeof( FTDF_pib.currentHop ),
    .attributeDefs[ FTDF_PIB_CURRENT_HOP ].getFunc                         = NULL,
    .attributeDefs[ FTDF_PIB_CURRENT_HOP ].setFunc                         = NULL,
    .attributeDefs[ FTDF_PIB_DWELL_TIME ].addr                             = &FTDF_pib.dwellTime,
    .attributeDefs[ FTDF_PIB_DWELL_TIME ].size                             = sizeof( FTDF_pib.dwellTime ),
    .attributeDefs[ FTDF_PIB_DWELL_TIME ].getFunc                          = NULL,
    .attributeDefs[ FTDF_PIB_DWELL_TIME ].setFunc                          = NULL,
    .attributeDefs[ FTDF_PIB_CSL_PERIOD ].addr                             = &FTDF_pib.CSLPeriod,
    .attributeDefs[ FTDF_PIB_CSL_PERIOD ].size                             = sizeof( FTDF_pib.CSLPeriod ),
    .attributeDefs[ FTDF_PIB_CSL_PERIOD ].getFunc                          = NULL,
    .attributeDefs[ FTDF_PIB_CSL_PERIOD ].setFunc                          = NULL,
    .attributeDefs[ FTDF_PIB_CSL_MAX_PERIOD ].addr                         = &FTDF_pib.CSLMaxPeriod,
    .attributeDefs[ FTDF_PIB_CSL_MAX_PERIOD ].size                         = sizeof( FTDF_pib.CSLMaxPeriod ),
    .attributeDefs[ FTDF_PIB_CSL_MAX_PERIOD ].getFunc                      = NULL,
    .attributeDefs[ FTDF_PIB_CSL_MAX_PERIOD ].setFunc                      = NULL,
    .attributeDefs[ FTDF_PIB_CSL_CHANNEL_MASK ].addr                       = &FTDF_pib.CSLChannelMask,
    .attributeDefs[ FTDF_PIB_CSL_CHANNEL_MASK ].size                       = sizeof( FTDF_pib.CSLChannelMask ),
    .attributeDefs[ FTDF_PIB_CSL_CHANNEL_MASK ].getFunc                    = NULL,
    .attributeDefs[ FTDF_PIB_CSL_CHANNEL_MASK ].setFunc                    = NULL,
    .attributeDefs[ FTDF_PIB_CSL_FRAME_PENDING_WAIT_T ].addr               = &FTDF_pib.CSLFramePendingWaitT,
    .attributeDefs[ FTDF_PIB_CSL_FRAME_PENDING_WAIT_T ].size               = sizeof( FTDF_pib.CSLFramePendingWaitT ),
#ifdef FTDF_NO_CSL
    .attributeDefs[ FTDF_PIB_CSL_FRAME_PENDING_WAIT_T ].getFunc            = NULL,
    .attributeDefs[ FTDF_PIB_CSL_FRAME_PENDING_WAIT_T ].setFunc            = NULL,
#else
    .attributeDefs[ FTDF_PIB_CSL_FRAME_PENDING_WAIT_T ].getFunc            = FTDF_getCslFramePendingWaitT,
    .attributeDefs[ FTDF_PIB_CSL_FRAME_PENDING_WAIT_T ].setFunc            = FTDF_setCslFramePendingWaitT,
#endif /* FTDF_NO_CSL */
    .attributeDefs[ FTDF_PIB_LOW_ENERGY_SUPERFRAME_SUPPORTED ].addr        = &FTDF_pib.lowEnergySuperframeSupported,
    .attributeDefs[ FTDF_PIB_LOW_ENERGY_SUPERFRAME_SUPPORTED ].size        =
        sizeof( FTDF_pib.lowEnergySuperframeSupported ),
    .attributeDefs[ FTDF_PIB_LOW_ENERGY_SUPERFRAME_SUPPORTED ].getFunc     = NULL,
    .attributeDefs[ FTDF_PIB_LOW_ENERGY_SUPERFRAME_SUPPORTED ].setFunc     = NULL,
    .attributeDefs[ FTDF_PIB_LOW_ENERGY_SUPERFRAME_SYNC_INTERVAL ].addr    = &FTDF_pib.lowEnergySuperframeSyncInterval,
    .attributeDefs[ FTDF_PIB_LOW_ENERGY_SUPERFRAME_SYNC_INTERVAL ].size    =
        sizeof( FTDF_pib.lowEnergySuperframeSyncInterval ),
    .attributeDefs[ FTDF_PIB_LOW_ENERGY_SUPERFRAME_SYNC_INTERVAL ].getFunc = NULL,
    .attributeDefs[ FTDF_PIB_LOW_ENERGY_SUPERFRAME_SYNC_INTERVAL ].setFunc = NULL,
#endif /* !FTDF_LITE */
    .attributeDefs[ FTDF_PIB_PERFORMANCE_METRICS ].addr                    = &FTDF_pib.performanceMetrics,
    .attributeDefs[ FTDF_PIB_PERFORMANCE_METRICS ].size                    = sizeof( FTDF_pib.performanceMetrics ),
    .attributeDefs[ FTDF_PIB_PERFORMANCE_METRICS ].getFunc                 = FTDF_getLmacPmData,
    .attributeDefs[ FTDF_PIB_PERFORMANCE_METRICS ].setFunc                 = NULL,
#ifndef FTDF_LITE
    .attributeDefs[ FTDF_PIB_USE_ENHANCED_BEACON ].addr                    = &FTDF_pib.useEnhancedBecaon,
    .attributeDefs[ FTDF_PIB_USE_ENHANCED_BEACON ].size                    = sizeof( FTDF_pib.useEnhancedBecaon ),
    .attributeDefs[ FTDF_PIB_USE_ENHANCED_BEACON ].getFunc                 = NULL,
    .attributeDefs[ FTDF_PIB_USE_ENHANCED_BEACON ].setFunc                 = NULL,
    .attributeDefs[ FTDF_PIB_EB_IE_LIST ].addr                             = &FTDF_pib.EBIEList,
    .attributeDefs[ FTDF_PIB_EB_IE_LIST ].size                             = sizeof( FTDF_pib.EBIEList ),
    .attributeDefs[ FTDF_PIB_EB_IE_LIST ].getFunc                          = NULL,
    .attributeDefs[ FTDF_PIB_EB_IE_LIST ].setFunc                          = NULL,
    .attributeDefs[ FTDF_PIB_EB_FILTERING_ENABLED ].addr                   = &FTDF_pib.EBFilteringEnabled,
    .attributeDefs[ FTDF_PIB_EB_FILTERING_ENABLED ].size                   = sizeof( FTDF_pib.EBFilteringEnabled ),
    .attributeDefs[ FTDF_PIB_EB_FILTERING_ENABLED ].getFunc                = NULL,
    .attributeDefs[ FTDF_PIB_EB_FILTERING_ENABLED ].setFunc                = NULL,
    .attributeDefs[ FTDF_PIB_EBSN ].addr                                   = &FTDF_pib.EBSN,
    .attributeDefs[ FTDF_PIB_EBSN ].size                                   = sizeof( FTDF_pib.EBSN ),
    .attributeDefs[ FTDF_PIB_EBSN ].getFunc                                = NULL,
    .attributeDefs[ FTDF_PIB_EBSN ].setFunc                                = NULL,
    .attributeDefs[ FTDF_PIB_EB_AUTO_SA ].addr                             = &FTDF_pib.EBAutoSA,
    .attributeDefs[ FTDF_PIB_EB_AUTO_SA ].size                             = sizeof( FTDF_pib.EBAutoSA ),
    .attributeDefs[ FTDF_PIB_EB_AUTO_SA ].getFunc                          = NULL,
    .attributeDefs[ FTDF_PIB_EB_AUTO_SA ].setFunc                          = NULL,
    .attributeDefs[ FTDF_PIB_EACK_IE_LIST ].addr                           = &FTDF_pib.EAckIEList,
    .attributeDefs[ FTDF_PIB_EACK_IE_LIST ].size                           = sizeof( FTDF_pib.EAckIEList ),
    .attributeDefs[ FTDF_PIB_EACK_IE_LIST ].getFunc                        = NULL,
    .attributeDefs[ FTDF_PIB_EACK_IE_LIST ].setFunc                        = NULL,
    .attributeDefs[ FTDF_PIB_KEY_TABLE ].addr                              = &FTDF_pib.keyTable,
    .attributeDefs[ FTDF_PIB_KEY_TABLE ].size                              = sizeof( FTDF_pib.keyTable ),
    .attributeDefs[ FTDF_PIB_KEY_TABLE ].getFunc                           = NULL,
    .attributeDefs[ FTDF_PIB_KEY_TABLE ].setFunc                           = NULL,
    .attributeDefs[ FTDF_PIB_DEVICE_TABLE ].addr                           = &FTDF_pib.deviceTable,
    .attributeDefs[ FTDF_PIB_DEVICE_TABLE ].size                           = sizeof( FTDF_pib.deviceTable ),
    .attributeDefs[ FTDF_PIB_DEVICE_TABLE ].getFunc                        = NULL,
    .attributeDefs[ FTDF_PIB_DEVICE_TABLE ].setFunc                        = NULL,
    .attributeDefs[ FTDF_PIB_SECURITY_LEVEL_TABLE ].addr                   = &FTDF_pib.securityLevelTable,
    .attributeDefs[ FTDF_PIB_SECURITY_LEVEL_TABLE ].size                   = sizeof( FTDF_pib.securityLevelTable ),
    .attributeDefs[ FTDF_PIB_SECURITY_LEVEL_TABLE ].getFunc                = NULL,
    .attributeDefs[ FTDF_PIB_SECURITY_LEVEL_TABLE ].setFunc                = NULL,
    .attributeDefs[ FTDF_PIB_FRAME_COUNTER ].addr                          = &FTDF_pib.frameCounter,
    .attributeDefs[ FTDF_PIB_FRAME_COUNTER ].size                          = sizeof( FTDF_pib.frameCounter ),
    .attributeDefs[ FTDF_PIB_FRAME_COUNTER ].getFunc                       = NULL,
    .attributeDefs[ FTDF_PIB_FRAME_COUNTER ].setFunc                       = NULL,
    .attributeDefs[ FTDF_PIB_MT_DATA_SECURITY_LEVEL ].addr                 = &FTDF_pib.mtDataSecurityLevel,
    .attributeDefs[ FTDF_PIB_MT_DATA_SECURITY_LEVEL ].size                 =
        sizeof( FTDF_pib.mtDataSecurityLevel ),
    .attributeDefs[ FTDF_PIB_MT_DATA_SECURITY_LEVEL ].getFunc              = NULL,
    .attributeDefs[ FTDF_PIB_MT_DATA_SECURITY_LEVEL ].setFunc              = NULL,
    .attributeDefs[ FTDF_PIB_MT_DATA_KEY_ID_MODE ].addr                    = &FTDF_pib.mtDataKeyIdMode,
    .attributeDefs[ FTDF_PIB_MT_DATA_KEY_ID_MODE ].size                    = sizeof( FTDF_pib.mtDataKeyIdMode ),
    .attributeDefs[ FTDF_PIB_MT_DATA_KEY_ID_MODE ].getFunc                 = NULL,
    .attributeDefs[ FTDF_PIB_MT_DATA_KEY_ID_MODE ].setFunc                 = NULL,
    .attributeDefs[ FTDF_PIB_MT_DATA_KEY_SOURCE ].addr                     = &FTDF_pib.mtDataKeySource,
    .attributeDefs[ FTDF_PIB_MT_DATA_KEY_SOURCE ].size                     = sizeof( FTDF_pib.mtDataKeySource ),
    .attributeDefs[ FTDF_PIB_MT_DATA_KEY_SOURCE ].getFunc                  = NULL,
    .attributeDefs[ FTDF_PIB_MT_DATA_KEY_SOURCE ].setFunc                  = NULL,
    .attributeDefs[ FTDF_PIB_MT_DATA_KEY_INDEX ].addr                      = &FTDF_pib.mtDataKeyIndex,
    .attributeDefs[ FTDF_PIB_MT_DATA_KEY_INDEX ].size                      = sizeof( FTDF_pib.mtDataKeyIndex ),
    .attributeDefs[ FTDF_PIB_MT_DATA_KEY_INDEX ].getFunc                   = NULL,
    .attributeDefs[ FTDF_PIB_MT_DATA_KEY_INDEX ].setFunc                   = NULL,
    .attributeDefs[ FTDF_PIB_DEFAULT_KEY_SOURCE ].addr                     = &FTDF_pib.defaultKeySource,
    .attributeDefs[ FTDF_PIB_DEFAULT_KEY_SOURCE ].size                     = sizeof( FTDF_pib.defaultKeySource ),
    .attributeDefs[ FTDF_PIB_DEFAULT_KEY_SOURCE ].getFunc                  = NULL,
    .attributeDefs[ FTDF_PIB_DEFAULT_KEY_SOURCE ].setFunc                  = NULL,
    .attributeDefs[ FTDF_PIB_PAN_COORD_EXTENDED_ADDRESS ].addr             = &FTDF_pib.PANCoordExtAddress,
    .attributeDefs[ FTDF_PIB_PAN_COORD_EXTENDED_ADDRESS ].size             = sizeof( FTDF_pib.PANCoordExtAddress ),
    .attributeDefs[ FTDF_PIB_PAN_COORD_EXTENDED_ADDRESS ].getFunc          = NULL,
    .attributeDefs[ FTDF_PIB_PAN_COORD_EXTENDED_ADDRESS ].setFunc          = NULL,
    .attributeDefs[ FTDF_PIB_PAN_COORD_SHORT_ADDRESS ].addr                = &FTDF_pib.PANCoordShortAddress,
    .attributeDefs[ FTDF_PIB_PAN_COORD_SHORT_ADDRESS ].size                = sizeof( FTDF_pib.PANCoordShortAddress ),
    .attributeDefs[ FTDF_PIB_PAN_COORD_SHORT_ADDRESS ].getFunc             = NULL,
    .attributeDefs[ FTDF_PIB_PAN_COORD_SHORT_ADDRESS ].setFunc             = NULL,
    .attributeDefs[ FTDF_PIB_FRAME_COUNTER_MODE ].addr                     = &FTDF_pib.frameCounterMode,
    .attributeDefs[ FTDF_PIB_FRAME_COUNTER_MODE ].size                     = sizeof( FTDF_pib.frameCounterMode ),
    .attributeDefs[ FTDF_PIB_FRAME_COUNTER_MODE ].getFunc                  = NULL,
    .attributeDefs[ FTDF_PIB_FRAME_COUNTER_MODE ].setFunc                  = NULL,
    .attributeDefs[ FTDF_PIB_CSL_SYNC_TX_MARGIN ].addr                     = &FTDF_pib.CSLSyncTxMargin,
    .attributeDefs[ FTDF_PIB_CSL_SYNC_TX_MARGIN ].size                     = sizeof( FTDF_pib.CSLSyncTxMargin ),
    .attributeDefs[ FTDF_PIB_CSL_SYNC_TX_MARGIN ].getFunc                  = NULL,
    .attributeDefs[ FTDF_PIB_CSL_SYNC_TX_MARGIN ].setFunc                  = NULL,
    .attributeDefs[ FTDF_PIB_CSL_MAX_AGE_REMOTE_INFO ].addr                = &FTDF_pib.CSLMaxAgeRemoteInfo,
    .attributeDefs[ FTDF_PIB_CSL_MAX_AGE_REMOTE_INFO ].size                = sizeof( FTDF_pib.CSLMaxAgeRemoteInfo ),
    .attributeDefs[ FTDF_PIB_CSL_MAX_AGE_REMOTE_INFO ].getFunc             = NULL,
    .attributeDefs[ FTDF_PIB_CSL_MAX_AGE_REMOTE_INFO ].setFunc             = NULL,
    .attributeDefs[ FTDF_PIB_TSCH_ENABLED ].addr                           = &FTDF_pib.tschEnabled,
    .attributeDefs[ FTDF_PIB_TSCH_ENABLED ].size                           = 0,
    .attributeDefs[ FTDF_PIB_TSCH_ENABLED ].getFunc                        = NULL,
    .attributeDefs[ FTDF_PIB_TSCH_ENABLED ].setFunc                        = NULL,
#ifdef FTDF_NO_CSL
    .attributeDefs[ FTDF_PIB_LE_ENABLED ].addr                             = &FTDF_pib.leEnabled,
    .attributeDefs[ FTDF_PIB_LE_ENABLED ].size                             = 0,
    .attributeDefs[ FTDF_PIB_LE_ENABLED ].getFunc                          = NULL,
    .attributeDefs[ FTDF_PIB_LE_ENABLED ].setFunc                          = NULL,
#else
    .attributeDefs[ FTDF_PIB_LE_ENABLED ].addr                             = &FTDF_pib.leEnabled,
    .attributeDefs[ FTDF_PIB_LE_ENABLED ].size                             = sizeof( FTDF_pib.leEnabled ),
    .attributeDefs[ FTDF_PIB_LE_ENABLED ].getFunc                          = NULL,
    .attributeDefs[ FTDF_PIB_LE_ENABLED ].setFunc                          = FTDF_setLeEnabled,
#endif /* FTDF_NO_CSL */
#endif /* !FTDF_LITE */
    .attributeDefs[ FTDF_PIB_CURRENT_CHANNEL ].addr                        = &FTDF_pib.currentChannel,
    .attributeDefs[ FTDF_PIB_CURRENT_CHANNEL ].size                        = sizeof( FTDF_pib.currentChannel ),
    .attributeDefs[ FTDF_PIB_CURRENT_CHANNEL ].getFunc                     = FTDF_getCurrentChannel,
    .attributeDefs[ FTDF_PIB_CURRENT_CHANNEL ].setFunc                     = FTDF_setCurrentChannel,
#ifndef FTDF_LITE
    .attributeDefs[ FTDF_PIB_CHANNELS_SUPPORTED ].addr                     = (void*) &channelsSupported,
    .attributeDefs[ FTDF_PIB_CHANNELS_SUPPORTED ].size                     = 0,
    .attributeDefs[ FTDF_PIB_CHANNELS_SUPPORTED ].getFunc                  = NULL,
    .attributeDefs[ FTDF_PIB_CHANNELS_SUPPORTED ].setFunc                  = NULL,
#endif /* !FTDF_LITE */
    .attributeDefs[ FTDF_PIB_TX_POWER_TOLERANCE ].addr                     = &FTDF_pib.TXPowerTolerance,
    .attributeDefs[ FTDF_PIB_TX_POWER_TOLERANCE ].size                     = sizeof( FTDF_pib.TXPowerTolerance ),
    .attributeDefs[ FTDF_PIB_TX_POWER_TOLERANCE ].getFunc                  = NULL,
    .attributeDefs[ FTDF_PIB_TX_POWER_TOLERANCE ].setFunc                  = NULL,
    .attributeDefs[ FTDF_PIB_TX_POWER ].addr                               = &FTDF_pib.TXPower,
    .attributeDefs[ FTDF_PIB_TX_POWER ].size                               = sizeof( FTDF_pib.TXPower ),
    .attributeDefs[ FTDF_PIB_TX_POWER ].getFunc                            = NULL,
    .attributeDefs[ FTDF_PIB_TX_POWER ].setFunc                            = NULL,
    .attributeDefs[ FTDF_PIB_CCA_MODE ].addr                               = &FTDF_pib.CCAMode,
    .attributeDefs[ FTDF_PIB_CCA_MODE ].size                               = sizeof( FTDF_pib.CCAMode ),
    .attributeDefs[ FTDF_PIB_CCA_MODE ].getFunc                            = NULL,
    .attributeDefs[ FTDF_PIB_CCA_MODE ].setFunc                            = FTDF_setTXPower,
#ifndef FTDF_LITE
    .attributeDefs[ FTDF_PIB_CURRENT_PAGE ].addr                           = &FTDF_pib.currentPage,
    .attributeDefs[ FTDF_PIB_CURRENT_PAGE ].size                           = 0,
    .attributeDefs[ FTDF_PIB_CURRENT_PAGE ].getFunc                        = NULL,
    .attributeDefs[ FTDF_PIB_CURRENT_PAGE ].setFunc                        = NULL,
    .attributeDefs[ FTDF_PIB_MAX_FRAME_DURATION ].addr                     = &FTDF_pib.maxFrameDuration,
    .attributeDefs[ FTDF_PIB_MAX_FRAME_DURATION ].size                     = 0,
    .attributeDefs[ FTDF_PIB_MAX_FRAME_DURATION ].getFunc                  = NULL,
    .attributeDefs[ FTDF_PIB_MAX_FRAME_DURATION ].setFunc                  = NULL,
    .attributeDefs[ FTDF_PIB_SHR_DURATION ].addr                           = &FTDF_pib.SHRDuration,
    .attributeDefs[ FTDF_PIB_SHR_DURATION ].size                           = 0,
    .attributeDefs[ FTDF_PIB_SHR_DURATION ].getFunc                        = NULL,
    .attributeDefs[ FTDF_PIB_SHR_DURATION ].setFunc                        = NULL,
#endif /* !FTDF_LITE */
    .attributeDefs[ FTDF_PIB_TRAFFIC_COUNTERS ].addr                       = &FTDF_pib.trafficCounters,
    .attributeDefs[ FTDF_PIB_TRAFFIC_COUNTERS ].size                       = 0,
    .attributeDefs[ FTDF_PIB_TRAFFIC_COUNTERS ].getFunc                    = FTDF_getLmacTrafficCounters,
    .attributeDefs[ FTDF_PIB_TRAFFIC_COUNTERS ].setFunc                    = NULL,
#ifndef FTDF_LITE
    .attributeDefs[ FTDF_PIB_LE_CAPABLE ].addr                             = &FTDF_pib.LECapable,
    .attributeDefs[ FTDF_PIB_LE_CAPABLE ].size                             = 0,
    .attributeDefs[ FTDF_PIB_LE_CAPABLE ].getFunc                          = NULL,
    .attributeDefs[ FTDF_PIB_LE_CAPABLE ].setFunc                          = NULL,
    .attributeDefs[ FTDF_PIB_LL_CAPABLE ].addr                             = &FTDF_pib.LLCapable,
    .attributeDefs[ FTDF_PIB_LL_CAPABLE ].size                             = 0,
    .attributeDefs[ FTDF_PIB_LL_CAPABLE ].getFunc                          = NULL,
    .attributeDefs[ FTDF_PIB_LL_CAPABLE ].setFunc                          = NULL,
    .attributeDefs[ FTDF_PIB_DSME_CAPABLE ].addr                           = &FTDF_pib.DSMECapable,
    .attributeDefs[ FTDF_PIB_DSME_CAPABLE ].size                           = 0,
    .attributeDefs[ FTDF_PIB_DSME_CAPABLE ].getFunc                        = NULL,
    .attributeDefs[ FTDF_PIB_DSME_CAPABLE ].setFunc                        = NULL,
    .attributeDefs[ FTDF_PIB_RFID_CAPABLE ].addr                           = &FTDF_pib.RFIDCapable,
    .attributeDefs[ FTDF_PIB_RFID_CAPABLE ].size                           = 0,
    .attributeDefs[ FTDF_PIB_RFID_CAPABLE ].getFunc                        = NULL,
    .attributeDefs[ FTDF_PIB_RFID_CAPABLE ].setFunc                        = NULL,
    .attributeDefs[ FTDF_PIB_AMCA_CAPABLE ].addr                           = &FTDF_pib.AMCACapable,
    .attributeDefs[ FTDF_PIB_AMCA_CAPABLE ].size                           = 0,
    .attributeDefs[ FTDF_PIB_AMCA_CAPABLE ].getFunc                        = NULL,
    .attributeDefs[ FTDF_PIB_AMCA_CAPABLE ].setFunc                        = NULL,
#endif /* !FTDF_LITE */
    .attributeDefs[ FTDF_PIB_METRICS_CAPABLE ].addr                        = &FTDF_pib.metricsCapable,
    .attributeDefs[ FTDF_PIB_METRICS_CAPABLE ].size                        = 0,
    .attributeDefs[ FTDF_PIB_METRICS_CAPABLE ].getFunc                     = NULL,
    .attributeDefs[ FTDF_PIB_METRICS_CAPABLE ].setFunc                     = NULL,
#ifndef FTDF_LITE
    .attributeDefs[ FTDF_PIB_RANGING_SUPPORTED ].addr                      = &FTDF_pib.rangingSupported,
    .attributeDefs[ FTDF_PIB_RANGING_SUPPORTED ].size                      = 0,
    .attributeDefs[ FTDF_PIB_RANGING_SUPPORTED ].getFunc                   = NULL,
    .attributeDefs[ FTDF_PIB_RANGING_SUPPORTED ].setFunc                   = NULL,
#endif /* !FTDF_LITE */
    .attributeDefs[ FTDF_PIB_KEEP_PHY_ENABLED ].addr                       = &FTDF_pib.keepPhyEnabled,
    .attributeDefs[ FTDF_PIB_KEEP_PHY_ENABLED ].size                       = sizeof( FTDF_pib.keepPhyEnabled ),
    .attributeDefs[ FTDF_PIB_KEEP_PHY_ENABLED ].getFunc                    = FTDF_getKeepPhyEnabled,
    .attributeDefs[ FTDF_PIB_KEEP_PHY_ENABLED ].setFunc                    = FTDF_setKeepPhyEnabled,
    .attributeDefs[ FTDF_PIB_METRICS_ENABLED ].addr                        = &FTDF_pib.metricsEnabled,
    .attributeDefs[ FTDF_PIB_METRICS_ENABLED ].size                        = sizeof( FTDF_pib.metricsEnabled ),
    .attributeDefs[ FTDF_PIB_METRICS_ENABLED ].getFunc                     = NULL,
    .attributeDefs[ FTDF_PIB_METRICS_ENABLED ].setFunc                     = NULL,
#ifndef FTDF_LITE
    .attributeDefs[ FTDF_PIB_BEACON_AUTO_RESPOND ].addr                    = &FTDF_pib.beaconAutoRespond,
    .attributeDefs[ FTDF_PIB_BEACON_AUTO_RESPOND ].size                    = sizeof( FTDF_pib.beaconAutoRespond ),
    .attributeDefs[ FTDF_PIB_BEACON_AUTO_RESPOND ].getFunc                 = NULL,
    .attributeDefs[ FTDF_PIB_BEACON_AUTO_RESPOND ].setFunc                 = NULL,
    .attributeDefs[ FTDF_PIB_TSCH_CAPABLE ].addr                           = &FTDF_pib.tschCapable,
    .attributeDefs[ FTDF_PIB_TSCH_CAPABLE ].size                           = 0,
    .attributeDefs[ FTDF_PIB_TSCH_CAPABLE ].getFunc                        = NULL,
    .attributeDefs[ FTDF_PIB_TSCH_CAPABLE ].setFunc                        = NULL,
    .attributeDefs[ FTDF_PIB_TS_SYNC_CORRECT_THRESHOLD ].addr              = &FTDF_pib.tsSyncCorrectThreshold,
    .attributeDefs[ FTDF_PIB_TS_SYNC_CORRECT_THRESHOLD ].size              = sizeof( FTDF_pib.tsSyncCorrectThreshold ),
    .attributeDefs[ FTDF_PIB_TS_SYNC_CORRECT_THRESHOLD ].getFunc           = NULL,
    .attributeDefs[ FTDF_PIB_TS_SYNC_CORRECT_THRESHOLD ].setFunc           = NULL,
#endif /* !FTDF_LITE */
#if dg_configBLACK_ORCA_IC_REV != BLACK_ORCA_IC_REV_A
    .attributeDefs[ FTDF_PIB_BO_IRQ_THRESHOLD ].addr                       = &FTDF_pib.boIrqThreshold,
    .attributeDefs[ FTDF_PIB_BO_IRQ_THRESHOLD ].size                       = sizeof( FTDF_pib.boIrqThreshold ),
    .attributeDefs[ FTDF_PIB_BO_IRQ_THRESHOLD ].getFunc                    = FTDF_getBoIrqThreshold,
    .attributeDefs[ FTDF_PIB_BO_IRQ_THRESHOLD ].setFunc                    = FTDF_setBoIrqThreshold,
    .attributeDefs[ FTDF_PIB_PTI_CONFIG ].addr                             = &FTDF_pib.ptiConfig,
    .attributeDefs[ FTDF_PIB_PTI_CONFIG ].size                             = sizeof( FTDF_pib.ptiConfig ),
    .attributeDefs[ FTDF_PIB_PTI_CONFIG ].getFunc                          = NULL,
    .attributeDefs[ FTDF_PIB_PTI_CONFIG ].setFunc                          = FTDF_setPtiConfig,
#endif /* dg_configBLACK_ORCA_IC_REV != BLACK_ORCA_IC_REV_A */
};

FTDF_Boolean        FTDF_transparentMode                              __attribute__ ( ( section( ".retention" ) ) );
FTDF_Bitmap32       FTDF_transparentModeOptions                       __attribute__ ( ( section( ".retention" ) ) );
#if FTDF_DBG_BUS_ENABLE
FTDF_DbgMode        FTDF_dbgMode                                      __attribute__ ( ( section( ".retention" ) ) );
#endif
#if dg_configUSE_FTDF_DDPHY == 1
uint16_t            FTDF_ddphyCcaReg                                  __attribute__ ( ( section( ".retention" ) ) );
#endif
#ifndef FTDF_LITE
FTDF_Buffer         FTDF_reqBuffers[ FTDF_NR_OF_REQ_BUFFERS ]         __attribute__ ( ( section( ".retention" ) ) );
FTDF_Queue          FTDF_reqQueue                                     __attribute__ ( ( section( ".retention" ) ) );
FTDF_Queue          FTDF_freeQueue                                    __attribute__ ( ( section( ".retention" ) ) );
FTDF_Pending        FTDF_txPendingList[ FTDF_NR_OF_REQ_BUFFERS ]      __attribute__ ( ( section( ".retention" ) ) );
FTDF_PendingTL      FTDF_txPendingTimerList[ FTDF_NR_OF_REQ_BUFFERS ] __attribute__ ( ( section( ".retention" ) ) );
FTDF_PendingTL*     FTDF_txPendingTimerHead                           __attribute__ ( ( section( ".retention" ) ) );
FTDF_Time           FTDF_txPendingTimerLT                             __attribute__ ( ( section( ".retention" ) ) );
FTDF_Time           FTDF_txPendingTimerTime                           __attribute__ ( ( section( ".retention" ) ) );
#endif /* !FTDF_LITE */
#ifndef FTDF_PHY_API
FTDF_MsgBuffer*     FTDF_reqCurrent                                   __attribute__ ( ( section( ".retention" ) ) );
#endif
FTDF_Size           FTDF_nrOfRetries                                  __attribute__ ( ( section( ".retention" ) ) );
#if FTDF_USE_SLEEP_DURING_BACKOFF
static FTDF_Sdb     FTDF_sdb                                          __attribute__ ( ( section( ".retention" ) ) );
#endif /* FTDF_USE_SLEEP_DURING_BACKOFF */
#ifndef FTDF_LITE
FTDF_Boolean        FTDF_isPANCoordinator                             __attribute__ ( ( section( ".retention" ) ) );
FTDF_Time           FTDF_startCslSampleTime                           __attribute__ ( ( section( ".retention" ) ) );
FTDF_RxAddressAdmin FTDF_rxa[ FTDF_NR_OF_RX_ADDRS ]                   __attribute__ ( ( section( ".retention" ) ) );
#endif /* !FTDF_LITE */
FTDF_Boolean        FTDF_txInProgress                                 __attribute__ ( ( section( ".retention" ) ) );
#ifndef FTDF_LITE
#ifndef FTDF_NO_CSL
FTDF_PeerCslTiming  FTDF_peerCslTiming[ FTDF_NR_OF_CSL_PEERS ]        __attribute__ ( ( section( ".retention" ) ) );
FTDF_Boolean        FTDF_oldLeEnabled                                 __attribute__ ( ( section( ".retention" ) ) );
FTDF_Time           FTDF_rzTime                                       __attribute__ ( ( section( ".retention" ) ) );
FTDF_ShortAddress   FTDF_sendFramePending                             __attribute__ ( ( section( ".retention" ) ) );
#endif /* FTDF_NO_CSL */
#endif /* !FTDF_LITE */
uint32_t            FTDF_curTime[ 2 ]                                 __attribute__ ( ( section( ".retention" ) ) );
FTDF_LmacCounters   FTDF_lmacCounters                                 __attribute__ ( ( section( ".retention" ) ) );
FTDF_FrameHeader    FTDF_fh;
#ifndef FTDF_LITE
FTDF_SecurityHeader FTDF_sh;
FTDF_AssocAdmin     FTDF_aa;
#endif /* !FTDF_LITE */

#if FTDF_USE_PTI
/* Packet traffic information used when FTDF is in RX enable. */
static FTDF_PTI  FTDF_RxPti __attribute__ ( ( section( ".retention" ) ) );
#endif
static void sendConfirm( FTDF_Status status,
                         FTDF_MsgId  msgId );

void FTDF_reset(int setDefaultPIB)
{
    if ( setDefaultPIB )
    {
        // Reset PIB values to their default values
        memset( &FTDF_pib, 0, sizeof( FTDF_pib ) );

        FTDF_pib.extAddress                        = FTDF_GET_EXT_ADDRESS( );
        FTDF_pib.ackWaitDuration                   = 0x36;
#ifndef FTDF_LITE
        FTDF_pib.autoRequest                       = FTDF_TRUE;
        FTDF_pib.beaconOrder                       = 15;
        FTDF_pib.DSN                               = FTDF_pib.extAddress & 0xff;
        FTDF_pib.BSN                               = FTDF_pib.extAddress & 0xff;
        FTDF_pib.EBSN                              = FTDF_pib.extAddress & 0xff;
        FTDF_pib.coordShortAddress                 = 0xffff;
#endif /* !FTDF_LITE */
        FTDF_pib.maxBE                             = 5;
        FTDF_pib.maxCSMABackoffs                   = 4;
        FTDF_pib.maxFrameTotalWaitTime             = 1026; // see asic_vol v40.100.2.30 PR2540
        FTDF_pib.maxFrameRetries                   = 3;
        FTDF_pib.minBE                             = 3;
#ifndef FTDF_LITE
        FTDF_pib.LIFSPeriod                        = 40;
        FTDF_pib.SIFSPeriod                        = 12;
#endif /* !FTDF_LITE */
        FTDF_pib.PANId                             = 0xffff;
#ifndef FTDF_LITE
        FTDF_pib.responseWaitTime                  = 32;
#endif /* !FTDF_LITE */
        FTDF_pib.shortAddress                      = 0xffff;
#ifndef FTDF_LITE
        FTDF_pib.superframeOrder                   = 15;
        FTDF_pib.timestampSupported                = FTDF_TRUE;
        FTDF_pib.transactionPersistenceTime        = 0x1f4;
        FTDF_pib.enhAckWaitDuration                = 0x360;
        FTDF_pib.EBAutoSA                          = FTDF_AUTO_FULL;
#endif /* !FTDF_LITE */
        FTDF_pib.currentChannel                    = 11;
        FTDF_pib.CCAMode                           = FTDF_CCA_MODE_1;
#ifndef FTDF_LITE
        FTDF_pib.maxFrameDuration                  = FTDF_TBD;
        FTDF_pib.SHRDuration                       = FTDF_TBD;
        FTDF_pib.frameCounterMode                  = 4;
#endif /* !FTDF_LITE */
        FTDF_pib.metricsCapable                    = FTDF_TRUE;
#ifndef FTDF_LITE
        FTDF_pib.beaconAutoRespond                 = FTDF_TRUE;
#endif /* !FTDF_LITE */
        FTDF_pib.performanceMetrics.counterOctets  = 4; // 32 bit counters
#ifndef FTDF_LITE
        FTDF_pib.joinPriority                      = 1;
        FTDF_pib.slotframeTable.slotframeEntries   = FTDF_slotframeTable;
        FTDF_pib.linkTable.linkEntries             = FTDF_linkTable;
        FTDF_pib.timeslotTemplate.tsCCAOffset      = 1800;
        FTDF_pib.timeslotTemplate.tsCCA            = 128;
        FTDF_pib.timeslotTemplate.tsTxOffset       = 2120;
        FTDF_pib.timeslotTemplate.tsRxOffset       = 1020;
        FTDF_pib.timeslotTemplate.tsRxAckDelay     = 800;
        FTDF_pib.timeslotTemplate.tsTxAckDelay     = 1000;
        FTDF_pib.timeslotTemplate.tsRxWait         = 2200;
        FTDF_pib.timeslotTemplate.tsAckWait        = 400;
        FTDF_pib.timeslotTemplate.tsRxTx           = 192;
        FTDF_pib.timeslotTemplate.tsMaxAck         = 2400;
        FTDF_pib.timeslotTemplate.tsMaxTs          = 4256;
        FTDF_pib.timeslotTemplate.tsTimeslotLength = 10000;
        FTDF_pib.tsSyncCorrectThreshold            = 220;
        FTDF_pib.hoppingSequenceLength             = 16;
#if dg_configBLACK_ORCA_IC_REV != BLACK_ORCA_IC_REV_A
        int i;
        for (i = 0; i < FTDF_PTIS; i++) {
                FTDF_pib.ptiConfig.ptis[i] = 0;
        }
#endif
#ifdef FTDF_NO_CSL
        FTDF_pib.LECapable                         = FTDF_FALSE;
#else
        FTDF_pib.LECapable                         = FTDF_TRUE;
#endif /* FTDF_NO_CSL */
#ifdef FTDF_NO_TSCH
        FTDF_pib.tschCapable                       = FTDF_FALSE;
#else
        FTDF_pib.tschCapable                       = FTDF_TRUE;
#endif /* FTDF_NO_TSCH */

        int n;

        for ( n = 0; n < FTDF_MAX_HOPPING_SEQUENCE_LENGTH; n++ )
        {
            FTDF_pib.hoppingSequenceList[ n ] = n + 11;
        }
#endif /* !FTDF_LITE */

        FTDF_transparentMode  = FTDF_FALSE;
#ifndef FTDF_LITE
        FTDF_isPANCoordinator = FTDF_FALSE;
#endif /* !FTDF_LITE */
        FTDF_lmacCounters.fcsErrorCnt = 0;
        FTDF_lmacCounters.txStdAckCnt = 0;
        FTDF_lmacCounters.rxStdAckCnt = 0;

#ifndef FTDF_LITE
        memset( FTDF_pib.defaultKeySource, 0xff, 8 );
#endif /* !FTDF_LITE */
#if dg_configBLACK_ORCA_IC_REV != BLACK_ORCA_IC_REV_A
        FTDF_pib.boIrqThreshold                         = FTDF_BO_IRQ_THRESHOLD;
#endif /* #if dg_configBLACK_ORCA_IC_REV != BLACK_ORCA_IC_REV_A */
    }

    FTDF_initQueues( );

    volatile uint32_t* lmacReset = FTDF_GET_REG_ADDR( ON_OFF_REGMAP_LMACRESET );
    *lmacReset = MSK_R_FTDF_ON_OFF_REGMAP_LMACRESET;

    volatile uint32_t* controlStatus = FTDF_GET_REG_ADDR( ON_OFF_REGMAP_LMAC_CONTROL_STATUS );
    uint32_t           wait          = 0;

    while ( ( *controlStatus & MSK_F_FTDF_ON_OFF_REGMAP_LMACREADY4SLEEP ) == 0 )
    {
        wait++;
    }

    volatile uint32_t* wakeupTimerEnableStatus = FTDF_GET_FIELD_ADDR( ON_OFF_REGMAP_WAKEUPTIMERENABLESTATUS );

#if dg_configBLACK_ORCA_IC_REV == BLACK_ORCA_IC_REV_A
    FTDF_SET_FIELD( ALWAYS_ON_REGMAP_WAKEUPTIMERENABLE, 0 );

#else
    FTDF_SET_FIELD( ON_OFF_REGMAP_WAKEUPTIMERENABLE_CLEAR, 1 );
#endif
    while ( *wakeupTimerEnableStatus & MSK_F_FTDF_ON_OFF_REGMAP_WAKEUPTIMERENABLESTATUS )
    { }

#if dg_configBLACK_ORCA_IC_REV == BLACK_ORCA_IC_REV_A
    FTDF_SET_FIELD( ALWAYS_ON_REGMAP_WAKEUPTIMERENABLE, 1 );

#else
    FTDF_SET_FIELD( ON_OFF_REGMAP_WAKEUPTIMERENABLE_SET, 1 );
#endif
    while ( ( *wakeupTimerEnableStatus & MSK_F_FTDF_ON_OFF_REGMAP_WAKEUPTIMERENABLESTATUS ) == 0 )
    { }

#ifndef FTDF_LITE
    int n;

#ifndef FTDF_NO_CSL
    for ( n = 0; n < FTDF_NR_OF_CSL_PEERS; n++ )
    {
        FTDF_peerCslTiming[ n ].addr = 0xffff;
    }

    FTDF_oldLeEnabled     = FTDF_FALSE;
    FTDF_wakeUpEnableLe   = FTDF_FALSE;
    FTDF_sendFramePending = 0xfffe;
#endif /* FTDF_NO_CSL */
#endif /* !FTDF_LITE */
    FTDF_txInProgress     = FTDF_FALSE;

    FTDF_initCurTime64( );
#ifndef FTDF_NO_TSCH
    FTDF_initTschRetries( );
    FTDF_initBackoff( );
#endif /* FTDF_NO_TSCH */

#ifndef FTDF_LITE
    for ( n = 0; n < FTDF_NR_OF_RX_ADDRS; n++ )
    {
        FTDF_rxa[ n ].addrMode  = FTDF_NO_ADDRESS;
        FTDF_rxa[ n ].dsnValid  = FTDF_FALSE;
        FTDF_rxa[ n ].bsnValid  = FTDF_FALSE;
        FTDF_rxa[ n ].ebsnValid = FTDF_FALSE;
    }

#ifndef FTDF_NO_TSCH
    for ( n = 0; n < FTDF_NR_OF_NEIGHBORS; n++ )
    {
        FTDF_neighborTable[ n ].dstAddr = 0xffff;
    }
#endif /* FTDF_NO_TSCH */
#endif /* !FTDF_LITE */
#if FTDF_USE_SLEEP_DURING_BACKOFF
    FTDF_sdbFsmReset();
#endif /* FTDF_USE_SLEEP_DURING_BACKOFF */
    FTDF_initLmac( );

#ifndef FTDF_NO_CSL
    FTDF_rzTime = FTDF_GET_FIELD( ON_OFF_REGMAP_SYMBOLTIMESNAPSHOTVAL );
#endif /* FTDF_NO_CSL */

#if dg_configUSE_FTDF_DDPHY == 1
    FTDF_ddphySet(0);
#endif
}

#ifndef FTDF_PHY_API
void FTDF_processResetRequest( FTDF_ResetRequest* resetRequest )
{

    FTDF_reset(resetRequest->setDefaultPIB);
    FTDF_ResetConfirm* resetConfirm = (FTDF_ResetConfirm*) FTDF_GET_MSG_BUFFER( sizeof( FTDF_ResetConfirm ) );

    resetConfirm->msgId  = FTDF_RESET_CONFIRM;
    resetConfirm->status = FTDF_SUCCESS;

    FTDF_REL_MSG_BUFFER( (FTDF_MsgBuffer*) resetRequest );

    FTDF_RCV_MSG( (FTDF_MsgBuffer*) resetConfirm );
}
#endif /* FTDF_PHY_API */

void FTDF_initLmac( void )
{
    FTDF_PIBAttribute PIBAttribute;

    for ( PIBAttribute = 1; PIBAttribute <= FTDF_NR_OF_PIB_ATTRIBUTES; PIBAttribute++ )
    {
        if ( pibAttributeTable.attributeDefs[ PIBAttribute ].setFunc != NULL )
        {
            pibAttributeTable.attributeDefs[ PIBAttribute ].setFunc( );
        }
    }

    if ( FTDF_transparentMode == FTDF_TRUE )
    {
        FTDF_enableTransparentMode( FTDF_TRUE, FTDF_transparentModeOptions );
    }

#ifndef FTDF_LITE
    if ( FTDF_isPANCoordinator )
    {
        FTDF_SET_FIELD( ON_OFF_REGMAP_ISPANCOORDINATOR, 1 );
    }
#endif /* !FTDF_LITE */
    FTDF_SET_FIELD( ON_OFF_REGMAP_CCAIDLEWAIT, 192 );

    volatile uint32_t* txFlagClear = FTDF_GET_FIELD_ADDR( ON_OFF_REGMAP_TX_FLAG_CLEAR );
    *txFlagClear = MSK_F_FTDF_ON_OFF_REGMAP_TX_FLAG_CLEAR;

    volatile uint32_t* phyParams = FTDF_GET_REG_ADDR( ON_OFF_REGMAP_PHY_PARAMETERS_2 );
    *phyParams = ( FTDF_PHYTXSTARTUP << OFF_F_FTDF_ON_OFF_REGMAP_PHYTXSTARTUP ) |
                 ( FTDF_PHYTXLATENCY << OFF_F_FTDF_ON_OFF_REGMAP_PHYTXLATENCY ) |
                 ( FTDF_PHYTXFINISH << OFF_F_FTDF_ON_OFF_REGMAP_PHYTXFINISH ) |
                 ( FTDF_PHYTRXWAIT << OFF_F_FTDF_ON_OFF_REGMAP_PHYTRXWAIT );

    phyParams = FTDF_GET_REG_ADDR( ON_OFF_REGMAP_PHY_PARAMETERS_3 );
    *phyParams = ( FTDF_PHYRXSTARTUP << OFF_F_FTDF_ON_OFF_REGMAP_PHYRXSTARTUP ) |
                 ( FTDF_PHYRXLATENCY << OFF_F_FTDF_ON_OFF_REGMAP_PHYRXLATENCY ) |
                 ( FTDF_PHYENABLE << OFF_F_FTDF_ON_OFF_REGMAP_PHYENABLE );

    volatile uint32_t* ftdfCm = FTDF_GET_REG_ADDR( ON_OFF_REGMAP_FTDF_CM );
    *ftdfCm = FTDF_MSK_TX_CE | FTDF_MSK_RX_CE | FTDF_MSK_SYMBOL_TMR_CE;

    volatile uint32_t* rxMask = FTDF_GET_REG_ADDR( ON_OFF_REGMAP_RX_MASK );
    *rxMask = MSK_R_FTDF_ON_OFF_REGMAP_RX_MASK;

    volatile uint32_t* lmacMask = FTDF_GET_REG_ADDR( ON_OFF_REGMAP_LMAC_MASK );
    *lmacMask = MSK_F_FTDF_ON_OFF_REGMAP_RXTIMEREXPIRED_M;

    volatile uint32_t* lmacCtrlMask = FTDF_GET_REG_ADDR( ON_OFF_REGMAP_LMAC_CONTROL_MASK );
    *lmacCtrlMask = MSK_F_FTDF_ON_OFF_REGMAP_SYMBOLTIMETHR_M |
                    MSK_F_FTDF_ON_OFF_REGMAP_SYMBOLTIME2THR_M |
                    MSK_F_FTDF_ON_OFF_REGMAP_SYNCTIMESTAMP_M;

    volatile uint32_t* txFlagClearM;
    txFlagClearM   = FTDF_GET_FIELD_ADDR_INDEXED( ON_OFF_REGMAP_TX_FLAG_CLEAR_M, FTDF_TX_DATA_BUFFER );
    *txFlagClearM |= MSK_F_FTDF_ON_OFF_REGMAP_TX_FLAG_CLEAR_M;
    txFlagClearM   = FTDF_GET_FIELD_ADDR_INDEXED( ON_OFF_REGMAP_TX_FLAG_CLEAR_M, FTDF_TX_WAKEUP_BUFFER );
    *txFlagClearM |= MSK_F_FTDF_ON_OFF_REGMAP_TX_FLAG_CLEAR_M;
#if dg_configBLACK_ORCA_IC_REV != BLACK_ORCA_IC_REV_A
#if FTDF_FP_BIT_MODE == FTDF_FP_BIT_MODE_AUTO
    FTDF_fpprReset();
    FTDF_fpprSetMode(FTDF_TRUE, FTDF_FALSE, FTDF_FALSE);
#else
    FTDF_fpprSetMode(FTDF_FALSE, FTDF_TRUE, FTDF_TRUE);
#endif /* #if FTDF_FP_BIT_MODE == FTDF_FP_BIT_MODE_AUTO */
#if FTDF_USE_SLEEP_DURING_BACKOFF
    /* Unmask long BO interrupt. */
    FTDF_SET_FIELD(ON_OFF_REGMAP_CSMA_CA_BO_THR_M, 1);
#else /* FTDF_USE_SLEEP_DURING_BACKOFF */
    /* Set BO threshold. */
    FTDF_SET_FIELD(ON_OFF_REGMAP_CSMA_CA_BO_THRESHOLD, FTDF_BO_IRQ_THRESHOLD);

    /* Mask long BO interrupt. */
    FTDF_SET_FIELD(ON_OFF_REGMAP_CSMA_CA_BO_THR_M, 0);
#endif /* FTDF_USE_SLEEP_DURING_BACKOFF */
#if FTDF_USE_LPDP == 1
    FTDF_lpdpEnable(FTDF_TRUE);
#endif /* #if FTDF_USE_LPDP == 1 */
#endif /* #if dg_configBLACK_ORCA_IC_REV != BLACK_ORCA_IC_REV_A */
}

#ifdef FTDF_PHY_API
FTDF_PIBAttributeValue *FTDF_getValue(FTDF_PIBAttribute PIBAttribute)
{

    if ( PIBAttribute <= FTDF_NR_OF_PIB_ATTRIBUTES &&
         pibAttributeTable.attributeDefs[ PIBAttribute ].addr != NULL )
    {
        // Update PIB attribute with current LMAC status if a getFunc is defined
        if ( pibAttributeTable.attributeDefs[ PIBAttribute ].getFunc != NULL )
        {
            pibAttributeTable.attributeDefs[ PIBAttribute ].getFunc( );
        }

        return pibAttributeTable.attributeDefs[ PIBAttribute ].addr;
    }

    return NULL;
}

FTDF_Status FTDF_setValue(FTDF_PIBAttribute PIBAttribute, const FTDF_PIBAttributeValue *PIBAttributeValue)
{
    if ( PIBAttribute <= FTDF_NR_OF_PIB_ATTRIBUTES &&
         pibAttributeTable.attributeDefs[ PIBAttribute ].addr != NULL )
    {
        if ( pibAttributeTable.attributeDefs[ PIBAttribute ].size != 0 )
        {
            memcpy( pibAttributeTable.attributeDefs[ PIBAttribute ].addr,
                    PIBAttributeValue,
                    pibAttributeTable.attributeDefs[ PIBAttribute ].size );

            // Update LMAC with new PIB attribute value if a setFunc is defined
            if ( pibAttributeTable.attributeDefs[ PIBAttribute ].setFunc != NULL )
            {
                pibAttributeTable.attributeDefs[ PIBAttribute ].setFunc( );
            }
            return FTDF_SUCCESS;

        }
        else
        {
            return FTDF_READ_ONLY;
        }
    }
    return FTDF_UNSUPPORTED_ATTRIBUTE;
}

#else /* FTDF_PHY_API */
void FTDF_processGetRequest( FTDF_GetRequest* getRequest )
{
    FTDF_GetConfirm*  getConfirm   = (FTDF_GetConfirm*) FTDF_GET_MSG_BUFFER( sizeof( FTDF_GetConfirm ) );
    FTDF_PIBAttribute PIBAttribute = getRequest->PIBAttribute;

    getConfirm->msgId        = FTDF_GET_CONFIRM;
    getConfirm->PIBAttribute = PIBAttribute;

    if ( PIBAttribute <= FTDF_NR_OF_PIB_ATTRIBUTES &&
         pibAttributeTable.attributeDefs[ PIBAttribute ].addr != NULL )
    {
        // Update PIB attribute with current LMAC status if a getFunc is defined
        if ( pibAttributeTable.attributeDefs[ PIBAttribute ].getFunc != NULL )
        {
            pibAttributeTable.attributeDefs[ PIBAttribute ].getFunc( );
        }

        getConfirm->status            = FTDF_SUCCESS;
        getConfirm->PIBAttributeValue = pibAttributeTable.attributeDefs[ PIBAttribute ].addr;
    }
    else
    {
        getConfirm->status = FTDF_UNSUPPORTED_ATTRIBUTE;
    }

    FTDF_REL_MSG_BUFFER( (FTDF_MsgBuffer*) getRequest );

    FTDF_RCV_MSG( (FTDF_MsgBuffer*) getConfirm );
}

void FTDF_processSetRequest( FTDF_SetRequest* setRequest )
{
    FTDF_SetConfirm*  setConfirm   = (FTDF_SetConfirm*) FTDF_GET_MSG_BUFFER( sizeof( FTDF_SetConfirm ) );
    FTDF_PIBAttribute PIBAttribute = setRequest->PIBAttribute;

    setConfirm->msgId        = FTDF_SET_CONFIRM;
    setConfirm->PIBAttribute = PIBAttribute;

    if ( PIBAttribute <= FTDF_NR_OF_PIB_ATTRIBUTES &&
         pibAttributeTable.attributeDefs[ PIBAttribute ].addr != NULL )
    {
        if ( pibAttributeTable.attributeDefs[ PIBAttribute ].size != 0 )
        {
            setConfirm->status = FTDF_SUCCESS;
            memcpy( pibAttributeTable.attributeDefs[ PIBAttribute ].addr,
                    setRequest->PIBAttributeValue,
                    pibAttributeTable.attributeDefs[ PIBAttribute ].size );

            // Update LMAC with new PIB attribute value if a setFunc is defined
            if ( pibAttributeTable.attributeDefs[ PIBAttribute ].setFunc != NULL )
            {
                pibAttributeTable.attributeDefs[ PIBAttribute ].setFunc( );
            }
        }
        else
        {
            setConfirm->status = FTDF_READ_ONLY;
        }
    }
    else
    {
        setConfirm->status = FTDF_UNSUPPORTED_ATTRIBUTE;
    }

    FTDF_REL_MSG_BUFFER( (FTDF_MsgBuffer*) setRequest );

    FTDF_RCV_MSG( (FTDF_MsgBuffer*) setConfirm );
}


void FTDF_sendCommStatusIndication( FTDF_MsgBuffer*     request,
                                    FTDF_Status         status,
                                    FTDF_PANId          PANId,
                                    FTDF_AddressMode    srcAddrMode,
                                    FTDF_Address        srcAddr,
                                    FTDF_AddressMode    dstAddrMode,
                                    FTDF_Address        dstAddr,
                                    FTDF_SecurityLevel  securityLevel,
                                    FTDF_KeyIdMode      keyIdMode,
                                    FTDF_Octet*         keySource,
                                    FTDF_KeyIndex       keyIndex )
{
    FTDF_CommStatusIndication* commStatus =
        (FTDF_CommStatusIndication*) FTDF_GET_MSG_BUFFER( sizeof( FTDF_CommStatusIndication ) );

    commStatus->msgId         = FTDF_COMM_STATUS_INDICATION;
    commStatus->PANId         = PANId;
    commStatus->srcAddrMode   = srcAddrMode;
    commStatus->srcAddr       = srcAddr;
    commStatus->dstAddrMode   = dstAddrMode;
    commStatus->dstAddr       = dstAddr;
    commStatus->status        = status;
    commStatus->securityLevel = securityLevel;
    commStatus->keyIdMode     = keyIdMode;
    commStatus->keyIndex      = keyIndex;

    uint8_t n;

    if ( securityLevel != 0 )
    {
        if ( keyIdMode == 0x2 )
        {
            for ( n = 0; n < 4; n++ )
            {
                commStatus->keySource[ n ] = keySource[ n ];
            }
        }
        else if ( keyIdMode == 0x3 )
        {
            for ( n = 0; n < 8; n++ )
            {
                commStatus->keySource[ n ] = keySource[ n ];
            }
        }
    }
#ifndef FTDF_LITE
    if ( request &&
         ( request->msgId == FTDF_ORPHAN_RESPONSE ||
           request->msgId == FTDF_ASSOCIATE_RESPONSE ) )
    {
        if ( FTDF_reqCurrent == request )
        {
            FTDF_reqCurrent = NULL;
        }

        FTDF_REL_MSG_BUFFER( request );
        FTDF_RCV_MSG( (FTDF_MsgBuffer*) commStatus );
        /* Check for orphan response. */
#if FTDF_FP_BIT_MODE == FTDF_FP_BIT_MODE_AUTO
        FTDF_fpFsmClearPending();
#endif /* FTDF_FP_BIT_MODE == FTDF_FP_BIT_MODE_AUTO */
        FTDF_processNextRequest( );
        return;
    }
#endif /* !FTDF_LITE */
    FTDF_RCV_MSG( (FTDF_MsgBuffer*) commStatus );
}
#endif /* FTDF_PHY_API */

#ifndef FTDF_LITE
FTDF_Octet* FTDF_addFrameHeader( FTDF_Octet*       txPtr,
                                 FTDF_FrameHeader* frameHeader,
                                 FTDF_DataLength   msduLength )
{
    uint8_t          frameVersion;
    uint8_t          longFrameControl = 0x00;
    FTDF_Boolean     panIdCompression = FTDF_FALSE;
    FTDF_Bitmap8     options          = frameHeader->options;
    FTDF_Boolean     secure           = options & FTDF_OPT_SECURITY_ENABLED;
    FTDF_Boolean     framePending     = options & FTDF_OPT_FRAME_PENDING;
    FTDF_Boolean     ackTX            = options & FTDF_OPT_ACK_REQUESTED;
    FTDF_Boolean     panIdPresent     = options & FTDF_OPT_PAN_ID_PRESENT;
    FTDF_Boolean     seqNrSuppressed  = options & FTDF_OPT_SEQ_NR_SUPPRESSED;
    FTDF_Boolean     iesIncluded      = options & FTDF_OPT_IES_PRESENT;
    FTDF_FrameType   frameType        = frameHeader->frameType;
    FTDF_AddressMode dstAddrMode      = frameHeader->dstAddrMode;
    FTDF_AddressMode srcAddrMode      = frameHeader->srcAddrMode;
    FTDF_PANId       dstPANId         = frameHeader->dstPANId;

    if ( frameType == FTDF_MULTIPURPOSE_FRAME )
    {
        if ( options & ( FTDF_OPT_SECURITY_ENABLED | FTDF_OPT_ACK_REQUESTED | FTDF_OPT_PAN_ID_PRESENT |
                         FTDF_OPT_IES_PRESENT | FTDF_OPT_SEQ_NR_SUPPRESSED | FTDF_OPT_FRAME_PENDING ) )
        {
            longFrameControl = 0x08;
        }

        // Frame control field byte 1
        *txPtr++ = 0x05 | longFrameControl | dstAddrMode << 4 | srcAddrMode << 6;

        if ( longFrameControl )
        {
            // Frame control field byte 2
            *txPtr++ =
                ( panIdPresent ? 0x01 : 0x00 ) |
                ( secure ? 0x02 : 0x00 ) |
                ( seqNrSuppressed ? 0x04 : 0x00 ) |
                ( framePending ? 0x08 : 0x00 ) |
                ( ackTX ? 0x40 : 0x00 ) |
                ( iesIncluded ? 0x80 : 0x00 );
        }
    }
    else
    {
        //        if ( panIdPresent || iesIncluded || seqNrSuppressed || ( options & FTDF_OPT_ENHANCED ) || FTDF_pib.tschEnabled )
        if ( panIdPresent || iesIncluded || seqNrSuppressed || ( options & FTDF_OPT_ENHANCED ) )
        {
            frameVersion = 0b10;
        }
        else
        {
            if ( secure ||
                 msduLength > FTDF_MAX_MAC_SAFE_PAYLOAD_SIZE )
            {
                frameVersion = 0b01;
            }
            else
            {
                frameVersion = 0b00;
            }
        }

        if ( frameVersion < 0b10 )
        {
            if ( dstAddrMode != FTDF_NO_ADDRESS &&
                 srcAddrMode != FTDF_NO_ADDRESS &&
                 dstPANId == frameHeader->srcPANId )
            {
                panIdCompression = FTDF_TRUE;
            }
        }
        else
        {
            panIdCompression = panIdPresent;
        }

        // Frame control field byte 1
        *txPtr++ =
            ( frameType & 0x7 ) |
            ( secure ? 0x08 : 0x00 ) |
            ( framePending ? 0x10 : 0x00 ) |
            ( ackTX ? 0x20 : 0x00 ) |
            ( panIdCompression ? 0x40 : 0x00 );

        // Frame control field byte 2
        *txPtr++ =
            ( seqNrSuppressed ? 0x01 : 0x00 ) |
            ( iesIncluded ? 0x02 : 0x00 ) |
            dstAddrMode << 2 |
            frameVersion << 4 |
            srcAddrMode << 6;
    }

    if ( !seqNrSuppressed )
    {
        *txPtr++ = frameHeader->SN;
    }

    FTDF_Boolean addDstPANId = FTDF_FALSE;

    if ( frameType == FTDF_MULTIPURPOSE_FRAME )
    {
        if ( panIdPresent )
        {
            addDstPANId = FTDF_TRUE;
        }
    }
    else
    {
        if ( frameVersion < 0b10 )
        {
            if ( dstAddrMode != FTDF_NO_ADDRESS )
            {
                addDstPANId = FTDF_TRUE;
            }
        }
        else
        {
            // See Table 2a "PAN ID Compression" of IEEE 802.15.4-2011 for more details
            if ( ( srcAddrMode == FTDF_NO_ADDRESS && dstAddrMode == FTDF_NO_ADDRESS && panIdCompression ) ||
                 ( srcAddrMode == FTDF_NO_ADDRESS && dstAddrMode != FTDF_NO_ADDRESS && !panIdCompression ) ||
                 ( srcAddrMode != FTDF_NO_ADDRESS && dstAddrMode != FTDF_NO_ADDRESS && !panIdCompression ) )
            {
                addDstPANId = FTDF_TRUE;
            }
        }
    }

    if ( addDstPANId )
    {
        FTDF_Octet* PANIdPtr = (FTDF_Octet*)&dstPANId;

        *txPtr++ = *PANIdPtr++;
        *txPtr++ = *PANIdPtr;
    }

    FTDF_Address dstAddr = frameHeader->dstAddr;

    if ( dstAddrMode == FTDF_SIMPLE_ADDRESS )
    {
        *txPtr++ = dstAddr.simpleAddress;
    }
    else if ( dstAddrMode == FTDF_SHORT_ADDRESS )
    {
        FTDF_Octet* shortAddressPtr = (FTDF_Octet*)&dstAddr.shortAddress;

        *txPtr++ = *shortAddressPtr++;
        *txPtr++ = *shortAddressPtr;
    }
    else if ( dstAddrMode == FTDF_EXTENDED_ADDRESS )
    {
        FTDF_Octet* extAddressPtr = (FTDF_Octet*)&dstAddr.extAddress;
        int         n;

        for ( n = 0; n < 8; n++ )
        {
            *txPtr++ = *extAddressPtr++;
        }
    }

    FTDF_Boolean addSrcPANId = FTDF_FALSE;

    if ( frameType != FTDF_MULTIPURPOSE_FRAME )
    {
        if ( frameVersion < 0b10 )
        {
            if ( srcAddrMode != FTDF_NO_ADDRESS && !panIdCompression )
            {
                addSrcPANId = FTDF_TRUE;
            }
        }
        else
        {
            // See Table 2a "PAN ID Compression" of IEEE 802.15.4-2011 for more details
            if ( srcAddrMode != FTDF_NO_ADDRESS && dstAddrMode == FTDF_NO_ADDRESS && !panIdCompression )
            {
                addSrcPANId = FTDF_TRUE;
            }
        }
    }

    if ( addSrcPANId )
    {
        FTDF_Octet* PANIdPtr = (FTDF_Octet*)&frameHeader->srcPANId;

        *txPtr++ = *PANIdPtr++;
        *txPtr++ = *PANIdPtr;
    }

    if ( srcAddrMode == FTDF_SIMPLE_ADDRESS )
    {
        *txPtr++ = FTDF_pib.simpleAddress;
    }
    else if ( srcAddrMode == FTDF_SHORT_ADDRESS )
    {
        FTDF_Octet* shortAddressPtr = (FTDF_Octet*)&FTDF_pib.shortAddress;

        *txPtr++ = *shortAddressPtr++;
        *txPtr++ = *shortAddressPtr;
    }
    else if ( srcAddrMode == FTDF_EXTENDED_ADDRESS )
    {
        FTDF_Octet* extAddressPtr = (FTDF_Octet*)&FTDF_pib.extAddress;
        int         n;

        for ( n = 0; n < 8; n++ )
        {
            *txPtr++ = *extAddressPtr++;
        }
    }

    return txPtr;
}
#endif /* !FTDF_LITE */

FTDF_PTI FTDF_getRxPti( void )
{
#if FTDF_USE_PTI
        FTDF_PTI rx_pti;
        FTDF_criticalVar();
        FTDF_enterCritical();
        rx_pti = FTDF_RxPti;
        FTDF_exitCritical();
        return rx_pti;
#else
        return 0;
#endif
}

#ifdef FTDF_PHY_API
void FTDF_rxEnable(FTDF_Time rxOnDuration)
{
#if dg_configCOEX_ENABLE_CONFIG
    /* We do not force decision here. It will be automatically made when FTDF begins 
     * transaction.
     */
    hw_coex_update_ftdf_pti((hw_coex_pti_t) FTDF_getRxPti(), NULL, false);
#endif
    FTDF_SET_FIELD( ON_OFF_REGMAP_RXENABLE, 0 );
    FTDF_SET_FIELD( ON_OFF_REGMAP_RXONDURATION, rxOnDuration );
    FTDF_SET_FIELD( ON_OFF_REGMAP_RXENABLE, 1 );
}
#else
void FTDF_processRxEnableRequest( FTDF_RxEnableRequest* rxEnableRequest )
{
    FTDF_SET_FIELD( ON_OFF_REGMAP_RXENABLE, 0 );
    FTDF_SET_FIELD( ON_OFF_REGMAP_RXONDURATION, rxEnableRequest->rxOnDuration );
    FTDF_SET_FIELD( ON_OFF_REGMAP_RXENABLE, 1 );

    FTDF_RxEnableConfirm* rxEnableConfirm =
        (FTDF_RxEnableConfirm*) FTDF_GET_MSG_BUFFER( sizeof( FTDF_RxEnableConfirm ) );

    rxEnableConfirm->msgId  = FTDF_RX_ENABLE_CONFIRM;
    rxEnableConfirm->status = FTDF_SUCCESS;

    FTDF_REL_MSG_BUFFER( (FTDF_MsgBuffer*) rxEnableRequest );
    FTDF_RCV_MSG( (FTDF_MsgBuffer*) rxEnableConfirm );
}
#endif /* FTDF_PHY_API */

FTDF_Octet* FTDF_getFrameHeader( FTDF_Octet*       rxBuffer,
                                 FTDF_FrameHeader* frameHeader )
{
    FTDF_FrameType   frameType    = *rxBuffer & 0x07;
    uint8_t          frameVersion = 0;
    FTDF_Bitmap8     options      = 0;
    FTDF_AddressMode dstAddrMode;
    FTDF_AddressMode srcAddrMode;
    FTDF_Boolean     panIdCompression = FTDF_FALSE;
    FTDF_Boolean     panIdPresent     = FTDF_FALSE;

    if ( frameType == FTDF_MULTIPURPOSE_FRAME )
    {
        dstAddrMode = ( *rxBuffer & 0x30 ) >> 4;
        srcAddrMode = ( *rxBuffer & 0xc0 ) >> 6;

        // Check Long Frame Control
        if ( *rxBuffer & 0x08 )
        {
            rxBuffer++;

            panIdPresent = *rxBuffer & 0x01;

            if ( *rxBuffer & 0x02 )
            {
                options |= FTDF_OPT_SECURITY_ENABLED;
            }

            if ( *rxBuffer & 0x04 )
            {
                options |= FTDF_OPT_SEQ_NR_SUPPRESSED;
            }

            if ( *rxBuffer & 0x08 )
            {
                options |= FTDF_OPT_FRAME_PENDING;
            }

            if ( *rxBuffer & 0x40 )
            {
                options |= FTDF_OPT_ACK_REQUESTED;
            }

            if ( *rxBuffer & 0x80 )
            {
                options |= FTDF_OPT_IES_PRESENT;
            }

            frameVersion              = 0;

            frameHeader->frameVersion = FTDF_FRAME_VERSION_E;

            rxBuffer++;
        }
        else
        {
            rxBuffer++;
        }
    }
    else
    {
        if ( *rxBuffer & 0x08 )
        {
            options |= FTDF_OPT_SECURITY_ENABLED;
        }

        if ( *rxBuffer & 0x10 )
        {
            options |= FTDF_OPT_FRAME_PENDING;
        }

        if ( *rxBuffer & 0x20 )
        {
            options |= FTDF_OPT_ACK_REQUESTED;
        }

        panIdCompression = *rxBuffer & 0x40;

        rxBuffer++;

        frameVersion = ( *rxBuffer & 0x30 ) >> 4;

        if ( frameVersion == 0x02 )
        {
            if ( *rxBuffer & 0x01 )
            {
                options |= FTDF_OPT_SEQ_NR_SUPPRESSED;
            }

            if ( *rxBuffer & 0x02 )
            {
                options |= FTDF_OPT_IES_PRESENT;
            }

            frameHeader->frameVersion = FTDF_FRAME_VERSION_E;
        }
        else if ( frameVersion == 0x01 )
        {
            frameHeader->frameVersion = FTDF_FRAME_VERSION_2011;
        }
        else if ( frameVersion == 0x00 )
        {
            frameHeader->frameVersion = FTDF_FRAME_VERSION_2003;
        }
        else
        {
            frameHeader->frameVersion = FTDF_FRAME_VERSION_NOT_SUPPORTED;
            return rxBuffer;
        }

        dstAddrMode = ( *rxBuffer & 0x0c ) >> 2;
        srcAddrMode = ( *rxBuffer & 0xc0 ) >> 6;

        rxBuffer++;
    }

    if ( ( options & FTDF_OPT_SEQ_NR_SUPPRESSED ) == 0 )
    {
        frameHeader->SN = *rxBuffer++;
    }

    FTDF_Boolean hasDstPANId = FTDF_FALSE;

    if ( frameType == FTDF_MULTIPURPOSE_FRAME )
    {
        hasDstPANId = panIdPresent;
    }
    else
    {
        if ( frameVersion < 0x02 )
        {
            if ( dstAddrMode != FTDF_NO_ADDRESS )
            {
                hasDstPANId = FTDF_TRUE;
            }
        }
        else
        {
            if ( ( srcAddrMode == FTDF_NO_ADDRESS && dstAddrMode == FTDF_NO_ADDRESS && panIdCompression ) ||
                 ( srcAddrMode == FTDF_NO_ADDRESS && dstAddrMode != FTDF_NO_ADDRESS && !panIdCompression ) ||
                 ( srcAddrMode != FTDF_NO_ADDRESS && dstAddrMode != FTDF_NO_ADDRESS && !panIdCompression ) )
            {
                hasDstPANId = FTDF_TRUE;
            }
        }
    }

    if ( hasDstPANId )
    {
        FTDF_Octet* PANIdPtr = (FTDF_Octet*) &frameHeader->dstPANId;

        *PANIdPtr++ = *rxBuffer++;
        *PANIdPtr   = *rxBuffer++;
    }

    if ( dstAddrMode == FTDF_SIMPLE_ADDRESS )
    {
        frameHeader->dstAddr.simpleAddress = *rxBuffer++;
    }
    else if ( dstAddrMode == FTDF_SHORT_ADDRESS )
    {
        FTDF_Octet* shortAddressPtr = (FTDF_Octet*)&frameHeader->dstAddr.shortAddress;

        *shortAddressPtr++ = *rxBuffer++;
        *shortAddressPtr   = *rxBuffer++;
    }
    else if ( dstAddrMode == FTDF_EXTENDED_ADDRESS )
    {
        FTDF_Octet* extAddressPtr = (FTDF_Octet*)&frameHeader->dstAddr.extAddress;
        int         n;

        for ( n = 0; n < 8; n++ )
        {
            *extAddressPtr++ = *rxBuffer++;
        }
    }

    FTDF_Boolean hasSrcPANId = FTDF_FALSE;

    if ( frameVersion < 0x02 && frameType != FTDF_MULTIPURPOSE_FRAME )
    {
        if ( srcAddrMode != FTDF_NO_ADDRESS && !panIdCompression )
        {
            hasSrcPANId = FTDF_TRUE;
        }
    }
    else
    {
        if ( srcAddrMode != FTDF_NO_ADDRESS && dstAddrMode == FTDF_NO_ADDRESS && !panIdCompression )
        {
            hasSrcPANId = FTDF_TRUE;
        }
    }

    if ( hasSrcPANId )
    {
        FTDF_Octet* PANIdPtr = (FTDF_Octet*) &frameHeader->srcPANId;

        *PANIdPtr++ = *rxBuffer++;
        *PANIdPtr   = *rxBuffer++;
    }
    else
    {
        frameHeader->srcPANId = frameHeader->dstPANId;
    }

    if ( srcAddrMode == FTDF_SIMPLE_ADDRESS )
    {
        frameHeader->srcAddr.simpleAddress = *rxBuffer++;
    }
    else if ( srcAddrMode == FTDF_SHORT_ADDRESS )
    {
        FTDF_Octet* shortAddressPtr = (FTDF_Octet*)&frameHeader->srcAddr.shortAddress;

        *shortAddressPtr++ = *rxBuffer++;
        *shortAddressPtr   = *rxBuffer++;
    }
    else if ( srcAddrMode == FTDF_EXTENDED_ADDRESS )
    {
        FTDF_Octet* extAddressPtr = (FTDF_Octet*)&frameHeader->srcAddr.extAddress;
        int         n;

        for ( n = 0; n < 8; n++ )
        {
            *extAddressPtr++ = *rxBuffer++;
        }
    }

    frameHeader->frameType   = frameType;
    frameHeader->options     = options;
    frameHeader->dstAddrMode = dstAddrMode;
    frameHeader->srcAddrMode = srcAddrMode;

    return rxBuffer;
}

#ifndef FTDF_LITE
void FTDF_processNextRequest( void )
{
#ifndef FTDF_NO_TSCH
    if ( FTDF_pib.tschEnabled )
    {
        FTDF_MsgBuffer* request = FTDF_tschGetPending( FTDF_tschSlotLink->request );

        FTDF_tschSlotLink->request = NULL;

        FTDF_scheduleTsch( request );

        return;
    }
#endif /* FTDF_NO_TSCH */

    while ( FTDF_reqCurrent == NULL )
    {
        FTDF_MsgBuffer* request = FTDF_dequeueReqTail( &FTDF_reqQueue );

        if ( request )
        {
            FTDF_processRequest( request );
        }
        else
        {
            break;
        }
    }
}
#endif /* !FTDF_LITE */

static void processRxFrame( int readBuf )
{
    static FTDF_PANDescriptor PANDescriptor;
    static FTDF_Address       pendAddrList[ 7 ];

    FTDF_FrameHeader*         frameHeader    = &FTDF_fh;
#ifndef FTDF_LITE
    FTDF_SecurityHeader*      securityHeader = &FTDF_sh;
#endif /* !FTDF_LITE */

    uint8_t                   pendAddrSpec   = 0;

    FTDF_Octet*               rxBuffer       =
        (FTDF_Octet*) ( IND_R_FTDF_RX_RAM_RX_FIFO + (intptr_t) readBuf * FTDF_BUFFER_LENGTH );
    FTDF_Octet*               rxPtr          = rxBuffer;
    FTDF_DataLength           frameLen       = *rxPtr++;

    if ( FTDF_transparentMode )
    {
        if ( FTDF_pib.metricsEnabled )
        {
            FTDF_pib.performanceMetrics.RXSuccessCount++;
        }

        uint32_t           rxMeta1 = *FTDF_GET_REG_ADDR_INDEXED(RETENTION_RAM_RX_META_1, (intptr_t)readBuf);
        FTDF_LinkQuality   lqi     = FTDF_GET_FIELD_INDEXED(RETENTION_RAM_QUALITY_INDICATOR, readBuf);
        FTDF_Bitmap32      status  = FTDF_TRANSPARENT_RCV_SUCCESSFUL;

        status |= rxMeta1 & MSK_F_FTDF_RETENTION_RAM_CRC16_ERROR ? FTDF_TRANSPARENT_RCV_CRC_ERROR : 0;
        status |= rxMeta1 & MSK_F_FTDF_RETENTION_RAM_RES_FRM_TYPE_ERROR ? FTDF_TRANSPARENT_RCV_RES_FRAMETYPE : 0;
        status |= rxMeta1 & MSK_F_FTDF_RETENTION_RAM_RES_FRM_VERSION_ERROR ? FTDF_TRANSPARENT_RCV_RES_FRAME_VERSION : 0;
        status |= rxMeta1 & MSK_F_FTDF_RETENTION_RAM_DPANID_ERROR ? FTDF_TRANSPARENT_RCV_UNEXP_DST_PAN_ID : 0;
        status |= rxMeta1 & MSK_F_FTDF_RETENTION_RAM_DADDR_ERROR ? FTDF_TRANSPARENT_RCV_UNEXP_DST_ADDR : 0;

        FTDF_RCV_FRAME_TRANSPARENT(frameLen, rxPtr, status, lqi);

#if FTDF_TRANSPARENT_USE_WAIT_FOR_ACK
        if ((FTDF_transparentModeOptions & FTDF_TRANSPARENT_WAIT_FOR_ACK) )
        {
            FTDF_getFrameHeader( rxPtr, frameHeader );
            if (frameHeader->frameType == FTDF_ACKNOWLEDGEMENT_FRAME &&
                    (status == FTDF_TRANSPARENT_RCV_SUCCESSFUL))
            {

#ifndef FTDF_PHY_API
                volatile uint32_t* txFlagS = FTDF_GET_REG_ADDR_INDEXED( ON_OFF_REGMAP_TX_FLAG_S, FTDF_TX_DATA_BUFFER );
                while ( *txFlagS & MSK_F_FTDF_ON_OFF_REGMAP_TX_FLAG_STAT )
                { }

                // It is required to call FTDF_processTxEvent here because an RX ack generates two events
                // The RX event is raised first, then after an IFS the TX event is raised. However,
                // the FTDF_processNextRequest requires that both events have been handled.
                FTDF_processTxEvent( );
#endif /* !FTDF_PHY_API */

                FTDF_SN SN = FTDF_GET_FIELD_INDEXED( RETENTION_RAM_MACSN, FTDF_TX_DATA_BUFFER );

#ifdef FTDF_PHY_API
                FTDF_criticalVar();
                FTDF_enterCritical();
                if ( FTDF_txInProgress &&
                     frameHeader->SN == SN )
                {
                    FTDF_exitCritical();
                    return;
                }
                FTDF_exitCritical();
#else
                if ( FTDF_reqCurrent &&
                     frameHeader->SN == SN )
                {
                    FTDF_TransparentRequest* transparentRequest = (FTDF_TransparentRequest*) FTDF_reqCurrent;

                    FTDF_criticalVar();
                    FTDF_enterCritical();
                    FTDF_reqCurrent = NULL;
                    FTDF_exitCritical();
                    FTDF_SEND_FRAME_TRANSPARENT_CONFIRM( transparentRequest->handle, FTDF_TRANSPARENT_SEND_SUCCESSFUL);

                    FTDF_REL_MSG_BUFFER((FTDF_MsgBuffer *) transparentRequest);

                    return;
                }
#endif /* FTDF_PHY_API */
            }
        }
#endif /* FTDF_TRANSPARENT_USE_WAIT_FOR_ACK */
        return;
    }

#ifndef FTDF_LITE
    rxPtr = FTDF_getFrameHeader( rxPtr, frameHeader );

#if FTDF_FP_BIT_MODE == FTDF_FP_BIT_MODE_MANUAL
    if (frameHeader->options & FTDF_OPT_ACK_REQUESTED) {
            int n;
            FTDF_Boolean addressFound = FTDF_FALSE;
            for ( n = 0; n < FTDF_NR_OF_REQ_BUFFERS; n++ )
            {
                    if ( FTDF_txPendingList[ n ].addrMode == frameHeader->srcAddrMode &&
                            FTDF_txPendingList[ n ].PANId == frameHeader->srcPANId )
                    {
                            if ( frameHeader->srcAddrMode == FTDF_SHORT_ADDRESS )
                            {
                                    if ( FTDF_txPendingList[ n ].addr.shortAddress ==
                                            frameHeader->srcAddr.shortAddress )
                                    {
                                            addressFound = FTDF_TRUE;
                                            break;
                                    }
                            }
                            else if ( frameHeader->srcAddrMode == FTDF_EXTENDED_ADDRESS )
                            {
                                    if ( FTDF_txPendingList[ n ].addr.extAddress ==
                                            frameHeader->srcAddr.extAddress )
                                    {
                                            addressFound = FTDF_TRUE;
                                            break;
                                    }
                            }
                            else
                            {
                                    // Invalid srcAddrMode
                                    return;
                            }
                    }
            }
            if (addressFound) {
                    FTDF_fpprSetMode(FTDF_FALSE, FTDF_TRUE, FTDF_TRUE);
            } else {
                    FTDF_fpprSetMode(FTDF_FALSE, FTDF_TRUE, FTDF_FALSE);
            }
    }
#endif

    if ( frameHeader->frameVersion == FTDF_FRAME_VERSION_NOT_SUPPORTED )
    {
        return;
    }
#if defined(FTDF_NO_CSL) && defined(FTDF_NO_TSCH)
    else if ( frameHeader->frameVersion == FTDF_FRAME_VERSION_E ||
              frameHeader->frameType == FTDF_MULTIPURPOSE_FRAME )
    {
        return;
    }
#endif /* FTDF_NO_CSL && FTDF_NO_TSCH */

    FTDF_FrameType frameType = frameHeader->frameType;
    FTDF_Boolean   duplicate = FTDF_FALSE;

    if ( ( frameHeader->options & FTDF_OPT_SEQ_NR_SUPPRESSED ) == 0 &&
         frameHeader->srcAddrMode != FTDF_NO_ADDRESS )
    {
        FTDF_Time    timestamp = FTDF_GET_FIELD_INDEXED( RETENTION_RAM_RX_TIMESTAMP, readBuf );
        FTDF_SNSel   snSel     = FTDF_SN_SEL_DSN;
        FTDF_Boolean drop;
        
        if ( ( FTDF_pib.tschEnabled || frameHeader->frameVersion == FTDF_FRAME_VERSION_E ) &&
             ( frameHeader->options & FTDF_OPT_ACK_REQUESTED ) )
        {
            drop = FTDF_FALSE;
        }
        else
        {
            drop = FTDF_TRUE;
        }

        if ( frameType == FTDF_BEACON_FRAME )
        {
            snSel = frameHeader->frameVersion == FTDF_FRAME_VERSION_E ? FTDF_SN_SEL_EBSN : FTDF_SN_SEL_BSN;
        }

        uint8_t i;

        for ( i = 0; i < FTDF_NR_OF_RX_ADDRS; i++ )
        {
            // Check if entry is empty or matches
            if ( FTDF_rxa[ i ].addrMode == FTDF_NO_ADDRESS ||
                 ( FTDF_rxa[ i ].addrMode == frameHeader->srcAddrMode &&
                   ( ( frameHeader->srcAddrMode == FTDF_SHORT_ADDRESS &&
                       frameHeader->srcAddr.shortAddress == FTDF_rxa[ i ].addr.shortAddress ) ||
                     ( frameHeader->srcAddrMode == FTDF_EXTENDED_ADDRESS &&
                       frameHeader->srcAddr.extAddress == FTDF_rxa[ i ].addr.extAddress ) ) ) )
            {
                break;
            }
        }

        if ( i < FTDF_NR_OF_RX_ADDRS )
        {
            if ( FTDF_rxa[ i ].addrMode != FTDF_NO_ADDRESS )
            {
                switch ( snSel )
                {
                case FTDF_SN_SEL_DSN:

                    if ( FTDF_rxa[ i ].dsnValid == FTDF_TRUE )
                    {
                        if ( frameHeader->SN == FTDF_rxa[ i ].dsn )
                        {
                            if ( FTDF_pib.metricsEnabled )
                            {
                                FTDF_pib.performanceMetrics.duplicateFrameCount++;
                            }

                            if ( drop )
                            {
                                return;
                            }

                            duplicate = FTDF_TRUE;
                        }
                    }
                    else
                    {
                        FTDF_rxa[ i ].dsnValid = FTDF_TRUE;
                    }

                    FTDF_rxa[ i ].dsn = frameHeader->SN;
                    break;
                case FTDF_SN_SEL_BSN:

                    if ( FTDF_rxa[ i ].bsnValid == FTDF_TRUE )
                    {
                        if ( frameHeader->SN == FTDF_rxa[ i ].bsn )
                        {
                            if ( FTDF_pib.metricsEnabled )
                            {
                                FTDF_pib.performanceMetrics.duplicateFrameCount++;
                            }

                            if ( drop )
                            {
                                return;
                            }

                            duplicate = FTDF_TRUE;
                        }
                    }
                    else
                    {
                        FTDF_rxa[ i ].bsnValid = FTDF_TRUE;
                    }

                    FTDF_rxa[ i ].bsn = frameHeader->SN;
                    break;
                case FTDF_SN_SEL_EBSN:

                    if ( FTDF_rxa[ i ].ebsnValid == FTDF_TRUE )
                    {
                        if ( frameHeader->SN == FTDF_rxa[ i ].ebsn )
                        {
                            if ( FTDF_pib.metricsEnabled )
                            {
                                FTDF_pib.performanceMetrics.duplicateFrameCount++;
                            }

                            if ( drop )
                            {
                                return;
                            }

                            duplicate = FTDF_TRUE;
                        }
                    }
                    else
                    {
                        FTDF_rxa[ i ].ebsnValid = FTDF_TRUE;
                    }

                    FTDF_rxa[ i ].ebsn = frameHeader->SN;
                    break;
                }
            }
            else
            {
                FTDF_rxa[ i ].addrMode = frameHeader->srcAddrMode;
                FTDF_rxa[ i ].addr     = frameHeader->srcAddr;

                switch ( snSel )
                {
                case FTDF_SN_SEL_DSN:
                    FTDF_rxa[ i ].dsnValid = FTDF_TRUE;
                    FTDF_rxa[ i ].dsn      = frameHeader->SN;
                    break;
                case FTDF_SN_SEL_BSN:
                    FTDF_rxa[ i ].bsnValid = FTDF_TRUE;
                    FTDF_rxa[ i ].bsn      = frameHeader->SN;
                    break;
                case FTDF_SN_SEL_EBSN:
                    FTDF_rxa[ i ].ebsnValid = FTDF_TRUE;
                    FTDF_rxa[ i ].ebsn      = frameHeader->SN;
                    break;
                }
            }

            FTDF_rxa[ i ].timestamp = timestamp;
        }
        else
        {
            // find oldest entry and overwrite it
            FTDF_Time curTime = FTDF_GET_FIELD( ON_OFF_REGMAP_SYMBOLTIMESNAPSHOTVAL );
            FTDF_Time delta, greatestDelta = 0;
            uint8_t   entry   = 0;

            for ( i = 0; i < FTDF_NR_OF_RX_ADDRS; i++ )
            {
                delta = curTime - FTDF_rxa[ i ].timestamp;

                if ( delta > greatestDelta )
                {
                    greatestDelta = delta;
                    entry         = i;
                }
            }

            FTDF_rxa[ entry ].addrMode = frameHeader->srcAddrMode;
            FTDF_rxa[ entry ].addr     = frameHeader->srcAddr;

            switch ( snSel )
            {
            case FTDF_SN_SEL_DSN:
                FTDF_rxa[ entry ].bsnValid  = FTDF_FALSE;
                FTDF_rxa[ entry ].ebsnValid = FTDF_FALSE;
                FTDF_rxa[ entry ].dsnValid  = FTDF_TRUE;
                FTDF_rxa[ entry ].dsn       = frameHeader->SN;
                break;
            case FTDF_SN_SEL_BSN:
                FTDF_rxa[ entry ].dsnValid  = FTDF_FALSE;
                FTDF_rxa[ entry ].ebsnValid = FTDF_FALSE;
                FTDF_rxa[ entry ].bsnValid  = FTDF_TRUE;
                FTDF_rxa[ entry ].bsn       = frameHeader->SN;
                break;
            case FTDF_SN_SEL_EBSN:
                FTDF_rxa[ entry ].dsnValid  = FTDF_FALSE;
                FTDF_rxa[ entry ].bsnValid  = FTDF_FALSE;
                FTDF_rxa[ entry ].ebsnValid = FTDF_TRUE;
                FTDF_rxa[ entry ].ebsn      = frameHeader->SN;
                break;
            }
        }
    }

    if ( frameHeader->options & FTDF_OPT_SECURITY_ENABLED )
    {
        rxPtr = FTDF_getSecurityHeader( rxPtr, frameHeader->frameVersion, securityHeader );
    }
    else
    {
        securityHeader->securityLevel = 0;
        securityHeader->keyIdMode     = 0;
    }

    FTDF_IEList* headerIEList  = NULL;
    FTDF_IEList* payloadIEList = NULL;
    int          micLength     = FTDF_getMicLength( securityHeader->securityLevel );

#if !defined(FTDF_NO_CSL) || !defined(FTDF_NO_TSCH)
    if ( frameHeader->options & FTDF_OPT_IES_PRESENT )
    {
        rxPtr =
            FTDF_getIes( rxPtr, rxBuffer + ( frameLen - micLength - FTDF_FCS_LENGTH ), &headerIEList, &payloadIEList );
    }
#endif /* !FTDF_NO_CSL || !FTDF_NO_TSCH */

    // Get start of private data (needed to unsecure a frame)
    if ( frameType == FTDF_MAC_COMMAND_FRAME )
    {
        frameHeader->commandFrameId = *rxPtr++;
    }
    else if ( frameType == FTDF_BEACON_FRAME )
    {
        PANDescriptor.coordAddrMode = frameHeader->srcAddrMode;
        PANDescriptor.coordPANId    = frameHeader->srcPANId;
        PANDescriptor.coordAddr     = frameHeader->srcAddr;
        PANDescriptor.channelNumber = ( ( FTDF_GET_FIELD( ON_OFF_REGMAP_PHYRXATTR ) >> 4 ) & 0xf ) + 11;
        PANDescriptor.channelPage   = 0;
        PANDescriptor.timestamp     = FTDF_GET_FIELD_INDEXED( RETENTION_RAM_RX_TIMESTAMP, readBuf );
        PANDescriptor.linkQuality   = FTDF_GET_FIELD_INDEXED( RETENTION_RAM_QUALITY_INDICATOR, readBuf );

        FTDF_Octet* superframeSpecPtr = (FTDF_Octet*) &PANDescriptor.superframeSpec;
        *superframeSpecPtr++ = *rxPtr++;
        *superframeSpecPtr   = *rxPtr++;

        uint8_t gtsSpec = *rxPtr++;

        PANDescriptor.GTSPermit = ( gtsSpec & 0x08 ) ? FTDF_TRUE : FTDF_FALSE;
        uint8_t gtsDescrCount = gtsSpec & 0x7;

        if ( gtsDescrCount != 0 )
        {
            // GTS is not supported, so just skip the GTS direction and GTS list fields if present
            rxPtr += ( 1 + ( 3 * gtsDescrCount ) );
        }

        pendAddrSpec = *rxPtr++;
        uint8_t nrOfShortAddrs = pendAddrSpec & 0x07;
        uint8_t nrOfExtAddrs   = ( pendAddrSpec & 0x70 ) >> 4;

        int     n;

        for ( n = 0; n < ( nrOfShortAddrs + nrOfExtAddrs ); n++ )
        {
            if ( n < nrOfShortAddrs )
            {
                FTDF_Octet* shortAddressPtr = (FTDF_Octet*) &pendAddrList[ n ].shortAddress;
                *shortAddressPtr++ = *rxBuffer++;
                *shortAddressPtr   = *rxBuffer++;
            }
            else
            {
                FTDF_Octet* extAddressPtr = (FTDF_Octet*) &pendAddrList[ n ].extAddress;
                int         m;

                for ( m = 0; m < 8; m++ )
                {
                    *extAddressPtr++ = *rxBuffer++;
                }
            }
        }
    }
    else if ( frameType == FTDF_ACKNOWLEDGEMENT_FRAME &&
              securityHeader->securityLevel != 0 )
    {
        if ( FTDF_reqCurrent )
        {
            switch ( FTDF_reqCurrent->msgId )
            {
            case FTDF_DATA_REQUEST:
            {
                FTDF_DataRequest* dataRequest = (FTDF_DataRequest*) FTDF_reqCurrent;
                frameHeader->srcPANId    = dataRequest->dstPANId;
                frameHeader->srcAddrMode = dataRequest->dstAddrMode;
                frameHeader->srcAddr     = dataRequest->dstAddr;
                break;
            }
            case FTDF_POLL_REQUEST:
            {
                FTDF_PollRequest* pollRequest = (FTDF_PollRequest*) FTDF_reqCurrent;
                frameHeader->srcPANId    = pollRequest->coordPANId;
                frameHeader->srcAddrMode = pollRequest->coordAddrMode;
                frameHeader->srcAddr     = pollRequest->coordAddr;
                break;
            }
            case FTDF_ASSOCIATE_REQUEST:
            {
                FTDF_AssociateRequest* associateRequest = (FTDF_AssociateRequest*) FTDF_reqCurrent;
                frameHeader->srcPANId    = associateRequest->coordPANId;
                frameHeader->srcAddrMode = associateRequest->coordAddrMode;
                frameHeader->srcAddr     = associateRequest->coordAddr;
                break;
            }
            case FTDF_DISASSOCIATE_REQUEST:
            {
                FTDF_DisassociateRequest* disassociateRequest =
                    (FTDF_DisassociateRequest*) FTDF_reqCurrent;
                frameHeader->srcPANId    = disassociateRequest->devicePANId;
                frameHeader->srcAddrMode = disassociateRequest->deviceAddrMode;
                frameHeader->srcAddr     = disassociateRequest->deviceAddress;
                break;
            }
            case FTDF_ASSOCIATE_RESPONSE:
            {
                FTDF_AssociateResponse* associateResponse = (FTDF_AssociateResponse*) FTDF_reqCurrent;
                frameHeader->srcAddrMode        = FTDF_EXTENDED_ADDRESS;
                frameHeader->srcAddr.extAddress = associateResponse->deviceAddress;
                break;
            }
            }
        }
    }

    FTDF_Status status = FTDF_unsecureFrame( rxBuffer, rxPtr, frameHeader, securityHeader );

    if ( status != FTDF_SUCCESS )
    {
        if ( FTDF_pib.metricsEnabled )
        {
            FTDF_pib.performanceMetrics.securityFailureCount++;
        }

        FTDF_REL_DATA_BUFFER( (FTDF_Octet*)payloadIEList );

        // Since unsecure of acknowledgement frame is always successful,
        // nothing special has to be done to get the address information correct.
        FTDF_sendCommStatusIndication( FTDF_reqCurrent, status,
                                       FTDF_pib.PANId,
                                       frameHeader->srcAddrMode,
                                       frameHeader->srcAddr,
                                       frameHeader->dstAddrMode,
                                       frameHeader->dstAddr,
                                       securityHeader->securityLevel,
                                       securityHeader->keyIdMode,
                                       securityHeader->keySource,
                                       securityHeader->keyIndex );

        if ( frameType == FTDF_ACKNOWLEDGEMENT_FRAME && FTDF_reqCurrent )
        {
            sendConfirm( FTDF_NO_ACK,
                         FTDF_reqCurrent->msgId );

            FTDF_processNextRequest( );
        }

        return;
    }

    if ( FTDF_pib.metricsEnabled && frameType != FTDF_ACKNOWLEDGEMENT_FRAME )
    {
        FTDF_pib.performanceMetrics.RXSuccessCount++;
    }

#ifndef FTDF_NO_TSCH
    if ( FTDF_pib.tschEnabled && frameType != FTDF_ACKNOWLEDGEMENT_FRAME )
    {
        FTDF_Time timestamp = FTDF_GET_FIELD_INDEXED( RETENTION_RAM_RX_TIMESTAMP, readBuf );

        FTDF_correctSlotTime( timestamp );
    }
#endif /* FTDF_NO_TSCH */

    switch ( frameType )
    {
    case FTDF_ACKNOWLEDGEMENT_FRAME:

        if ( FTDF_pib.metricsEnabled )
        {
            if ( FTDF_nrOfRetries == 0 )
            {
                FTDF_pib.performanceMetrics.TXSuccessCount++;
            }
            else if ( FTDF_nrOfRetries == 1 )
            {
                FTDF_pib.performanceMetrics.retryCount++;
            }
            else
            {
                FTDF_pib.performanceMetrics.multipleRetryCount++;
            }
        }

        if ( frameHeader->frameVersion == FTDF_FRAME_VERSION_E )
        {
            FTDF_pib.trafficCounters.rxEnhAckFrmOkCnt++;
        }

        break;
    case FTDF_BEACON_FRAME:
        FTDF_pib.trafficCounters.rxBeaconFrmOkCnt++;
        break;
    case FTDF_DATA_FRAME:
        FTDF_pib.trafficCounters.rxDataFrmOkCnt++;
        break;
    case FTDF_MAC_COMMAND_FRAME:
        FTDF_pib.trafficCounters.rxCmdFrmOkCnt++;
        break;
    case FTDF_MULTIPURPOSE_FRAME:
        FTDF_pib.trafficCounters.rxMultiPurpFrmOkCnt++;
        break;
    }

    if ( frameType == FTDF_ACKNOWLEDGEMENT_FRAME )
    {
        volatile uint32_t* txFlagS = FTDF_GET_REG_ADDR_INDEXED( ON_OFF_REGMAP_TX_FLAG_S, FTDF_TX_DATA_BUFFER );

        while ( *txFlagS & MSK_F_FTDF_ON_OFF_REGMAP_TX_FLAG_STAT )
        { }

        // It is required to call FTDF_processTxEvent here because an RX ack generates two events
        // The RX event is raised first, then after an IFS the TX event is raised. However,
        // the FTDF_processNextRequest requires that both events have been handled.
        FTDF_processTxEvent( );

        FTDF_SN SN = FTDF_GET_FIELD_INDEXED( RETENTION_RAM_MACSN, FTDF_TX_DATA_BUFFER );

        if ( FTDF_reqCurrent &&
             frameHeader->SN == SN )
        {
#ifndef FTDF_NO_CSL
            if ( FTDF_pib.leEnabled == FTDF_TRUE )
            {
                FTDF_Time timestamp = FTDF_GET_FIELD_INDEXED( RETENTION_RAM_RX_TIMESTAMP,
                                                              readBuf );

                FTDF_setPeerCslTiming( headerIEList, timestamp );
            }
#endif /* FTDF_NO_CSL */

#ifndef FTDF_NO_TSCH
            if ( FTDF_pib.tschEnabled == FTDF_TRUE )
            {
                FTDF_correctSlotTimeFromAck( headerIEList );

                FTDF_TschRetry* tschRetry = FTDF_getTschRetry( FTDF_getRequestAddress( FTDF_reqCurrent ) );

                tschRetry->nrOfRetries     = 0;
                FTDF_tschSlotLink->request = NULL;
            }
#endif /* FTDF_NO_TSCH */

            switch ( FTDF_reqCurrent->msgId )
            {
            case FTDF_DATA_REQUEST:
            {
                FTDF_Time          timestamp     = FTDF_GET_FIELD_INDEXED( RETENTION_RAM_TXTIMESTAMP,
                                                                           FTDF_TX_DATA_BUFFER );
                FTDF_NumOfBackoffs numOfBackoffs = FTDF_GET_FIELD_INDEXED( RETENTION_RAM_CSMACANRRETRIES,
                                                                           FTDF_TX_DATA_BUFFER );

                FTDF_sendDataConfirm( (FTDF_DataRequest*)FTDF_reqCurrent,
                                      FTDF_SUCCESS,
                                      timestamp,
                                      SN,
                                      numOfBackoffs,
                                      payloadIEList );

                break;
            }
            case FTDF_POLL_REQUEST:
            {
                if ( !( frameHeader->options & FTDF_OPT_FRAME_PENDING ) )
                {
                    FTDF_sendPollConfirm( (FTDF_PollRequest*)FTDF_reqCurrent, FTDF_NO_DATA );
                }

                break;
            }
            case FTDF_ASSOCIATE_REQUEST:
            {
                FTDF_AssocAdmin* assocAdmin = &FTDF_aa;

                if ( assocAdmin->fastA == FTDF_TRUE ||
                     assocAdmin->dataR == FTDF_FALSE )
                {
                    uint32_t timestamp = FTDF_GET_FIELD( ON_OFF_REGMAP_SYMBOLTIMESNAPSHOTVAL );
                    FTDF_SET_FIELD( ON_OFF_REGMAP_SYMBOLTIME2THR,
                                    ( timestamp + FTDF_pib.responseWaitTime * FTDF_BASE_SUPERFRAME_DURATION ) );
                }
                else if ( !( frameHeader->options & FTDF_OPT_FRAME_PENDING ) )
                {
                    FTDF_sendAssociateConfirm( (FTDF_AssociateRequest*)FTDF_reqCurrent,
                                               FTDF_NO_DATA,
                                               0xffff );
                }

                break;
            }
            case FTDF_ASSOCIATE_RESPONSE:
            {
                FTDF_AssociateResponse* assocResp = (FTDF_AssociateResponse*)FTDF_reqCurrent;

                FTDF_Address srcAddr, dstAddr;
                srcAddr.extAddress = FTDF_pib.extAddress;
                dstAddr.extAddress = assocResp->deviceAddress;

                FTDF_sendCommStatusIndication( FTDF_reqCurrent, FTDF_SUCCESS,
                                               FTDF_pib.PANId,
                                               FTDF_EXTENDED_ADDRESS,
                                               srcAddr,
                                               FTDF_EXTENDED_ADDRESS,
                                               dstAddr,
                                               assocResp->securityLevel,
                                               assocResp->keyIdMode,
                                               assocResp->keySource,
                                               assocResp->keyIndex );
                break;
            }
            case FTDF_ORPHAN_RESPONSE:
            {
                FTDF_OrphanResponse* orphanResp = (FTDF_OrphanResponse*)FTDF_reqCurrent;

                FTDF_Address srcAddr, dstAddr;
                srcAddr.extAddress = FTDF_pib.extAddress;
                dstAddr.extAddress = orphanResp->orphanAddress;

                FTDF_sendCommStatusIndication( FTDF_reqCurrent, FTDF_SUCCESS,
                                               FTDF_pib.PANId,
                                               FTDF_EXTENDED_ADDRESS,
                                               srcAddr,
                                               FTDF_EXTENDED_ADDRESS,
                                               dstAddr,
                                               orphanResp->securityLevel,
                                               orphanResp->keyIdMode,
                                               orphanResp->keySource,
                                               orphanResp->keyIndex );
                break;
            }
            case FTDF_DISASSOCIATE_REQUEST:
            {
                FTDF_sendDisassociateConfirm( (FTDF_DisassociateRequest*)FTDF_reqCurrent, FTDF_SUCCESS );
                break;
            }
            case FTDF_REMOTE_REQUEST:
            {
                FTDF_RemoteRequest* remoteRequest = (FTDF_RemoteRequest*) FTDF_reqCurrent;

                if ( remoteRequest->remoteId == FTDF_REMOTE_PAN_ID_CONFLICT_NOTIFICATION )
                {
                    FTDF_sendSyncLossIndication( FTDF_PAN_ID_CONFLICT, securityHeader );
                }

                FTDF_reqCurrent = NULL;

                break;
            }
            }

            if ( FTDF_reqCurrent->msgId != FTDF_DATA_REQUEST )
            {
                // for data request the application owns the memory
                FTDF_REL_DATA_BUFFER( (FTDF_Octet*)payloadIEList );
            }

            FTDF_processNextRequest( );
        }
        else
        {
            FTDF_REL_DATA_BUFFER( (FTDF_Octet*)payloadIEList );
        }
    }
    else if ( ( frameHeader->frameVersion == FTDF_FRAME_VERSION_E || FTDF_pib.tschEnabled ) &&
              (frameHeader->options & FTDF_OPT_ACK_REQUESTED) )
    {
#if !defined(FTDF_NO_CSL) || !defined(FTDF_NO_TSCH)
        static FTDF_FrameHeader afh;
        FTDF_FrameHeader*       ackFrameHeader = &afh;

        ackFrameHeader->frameType = FTDF_ACKNOWLEDGEMENT_FRAME;
        ackFrameHeader->options   =
            ( frameHeader->options &
              ( FTDF_OPT_SECURITY_ENABLED | FTDF_OPT_SEQ_NR_SUPPRESSED ) ) | FTDF_OPT_ENHANCED;

        if ( FTDF_pib.leEnabled == FTDF_TRUE ||
             FTDF_pib.tschEnabled == FTDF_TRUE ||
             FTDF_pib.EAckIEList.nrOfIEs != 0 )
        {
            ackFrameHeader->options |= FTDF_OPT_IES_PRESENT;
        }

        ackFrameHeader->dstAddrMode = FTDF_NO_ADDRESS;
        ackFrameHeader->srcAddrMode = FTDF_NO_ADDRESS;
        ackFrameHeader->SN          = frameHeader->SN;

        FTDF_Octet* txPtr = (FTDF_Octet*) FTDF_GET_REG_ADDR( RETENTION_RAM_TX_FIFO ) +
                            ( FTDF_BUFFER_LENGTH * FTDF_TX_ACK_BUFFER );

        // Skip PHY header (= MAC length)
        txPtr++;

        txPtr = FTDF_addFrameHeader( txPtr,
                                     ackFrameHeader,
                                     0 );

        if ( frameHeader->options & FTDF_OPT_SECURITY_ENABLED )
        {
            securityHeader->frameCounter     = FTDF_pib.frameCounter;
            securityHeader->frameCounterMode = FTDF_pib.frameCounterMode;

            txPtr                            = FTDF_addSecurityHeader( txPtr,
                                                                       securityHeader );
        }

#ifndef FTDF_NO_CSL
        if ( FTDF_pib.leEnabled == FTDF_TRUE )
        {
            static FTDF_Octet        phaseAndPeriod[ 4 ];
            static FTDF_IEDescriptor cslIE     = { 0x1a, 4, { phaseAndPeriod } };
            static FTDF_IEList       cslIEList = { 1, &cslIE };
            FTDF_Time                curTime   = FTDF_GET_FIELD( ON_OFF_REGMAP_SYMBOLTIMESNAPSHOTVAL );

            FTDF_Time                delta     = curTime -
                                                 ( FTDF_startCslSampleTime - FTDF_pib.CSLPeriod * 10 );

            *(FTDF_Period*) ( phaseAndPeriod + 0 ) = (FTDF_Period) ( delta / 10 );
            *(FTDF_Period*) ( phaseAndPeriod + 2 ) = FTDF_pib.CSLPeriod;

            txPtr                                  = FTDF_addIes( txPtr,
                                                                  &cslIEList,
                                                                  &FTDF_pib.EAckIEList,
                                                                  FTDF_FALSE );
        }
#endif /* FTDF_NO_CSL */

#ifndef FTDF_NO_TSCH
        if ( FTDF_pib.tschEnabled == FTDF_TRUE )
        {
            FTDF_Time rxTimestamp = FTDF_GET_FIELD_INDEXED( RETENTION_RAM_RX_TIMESTAMP, readBuf );

            txPtr = FTDF_addCorrTimeIE( txPtr, rxTimestamp );
        }
#endif /* FTDF_NO_TSCH */

        if ( !FTDF_pib.leEnabled && !FTDF_pib.tschEnabled )
        {
            txPtr = FTDF_addIes( txPtr,
                                 NULL,
                                 &FTDF_pib.EAckIEList,
                                 FTDF_FALSE );
        }

        FTDF_sendAckFrame( frameHeader,
                           securityHeader,
                           txPtr );
#endif /* !FTDF_NO_CSL || !FTDF_NO_TSCH */

        if ( duplicate )
        {
            FTDF_REL_DATA_BUFFER( (FTDF_Octet*)payloadIEList );
            return;
        }
    }

    if ( frameType == FTDF_DATA_FRAME || frameType == FTDF_MULTIPURPOSE_FRAME )
    {
        FTDF_DataLength payloadLength = frameLen -
                                        ( rxPtr - rxBuffer ) + 1 - micLength - FTDF_FCS_LENGTH;

        if ( FTDF_reqCurrent &&
             FTDF_reqCurrent->msgId == FTDF_POLL_REQUEST )
        {
            FTDF_PollRequest* pollRequest = (FTDF_PollRequest*) FTDF_reqCurrent;

            if ( frameHeader->srcAddrMode == pollRequest->coordAddrMode &&
                 frameHeader->srcPANId == pollRequest->coordPANId &&
                 ( ( frameHeader->srcAddrMode == FTDF_SHORT_ADDRESS &&
                     frameHeader->srcAddr.shortAddress == pollRequest->coordAddr.shortAddress ) ||
                   ( frameHeader->srcAddrMode == FTDF_EXTENDED_ADDRESS &&
                     frameHeader->srcAddr.extAddress == pollRequest->coordAddr.extAddress ) ) )
            {
                if ( payloadLength == 0 )
                {
                    FTDF_sendPollConfirm( pollRequest, FTDF_NO_DATA );
                }
                else
                {
                    FTDF_sendPollConfirm( pollRequest, FTDF_SUCCESS );
                }
            }
        }
        else if ( FTDF_reqCurrent &&
                  FTDF_reqCurrent->msgId == FTDF_ASSOCIATE_REQUEST &&
                  payloadLength == 0 )
        {
            sendConfirm( FTDF_NO_DATA, FTDF_ASSOCIATE_REQUEST );
        }

        if ( payloadLength != 0 )
        {
            FTDF_LinkQuality mpduLinkQuality = FTDF_GET_FIELD_INDEXED( RETENTION_RAM_QUALITY_INDICATOR,
                                                                       readBuf );
            FTDF_Time        timestamp       = FTDF_GET_FIELD_INDEXED( RETENTION_RAM_RX_TIMESTAMP,
                                                                       readBuf );

            FTDF_sendDataIndication( frameHeader,
                                     securityHeader,
                                     payloadIEList,
                                     payloadLength,
                                     rxPtr,
                                     mpduLinkQuality,
                                     timestamp );
        }
#ifndef FTDF_NO_CSL
        else if ( headerIEList->nrOfIEs == 1 &&
                  headerIEList->IEs[ 0 ].ID == 0x1d )
        {
            FTDF_Period rzTime = *(uint16_t*) headerIEList->IEs[ 0 ].content.raw;

            FTDF_criticalVar( );
            FTDF_enterCritical( );

            FTDF_Time curTime = FTDF_GET_FIELD( ON_OFF_REGMAP_SYMBOLTIMESNAPSHOTVAL );

            FTDF_rzTime = curTime + (FTDF_Time) rzTime * 10 + 260;  // 260 length of max frame in symbols

            FTDF_Time CSLPeriod = FTDF_pib.CSLPeriod * 10;
            FTDF_Time delta     = FTDF_rzTime - FTDF_startCslSampleTime;

            while ( delta < 0x80000000 )  // A delta larger than 0x80000000 is assumed a negative delta
            {
                FTDF_startCslSampleTime += CSLPeriod;
                delta                    = FTDF_rzTime - FTDF_startCslSampleTime;
            }

            FTDF_SET_FIELD( ON_OFF_REGMAP_MACCSLSTARTSAMPLETIME, FTDF_startCslSampleTime );

            FTDF_exitCritical( );

            FTDF_REL_DATA_BUFFER( (FTDF_Octet*)payloadIEList );
        }
#endif /* FTDF_NO_CSL */
        else
        {
            FTDF_REL_DATA_BUFFER( (FTDF_Octet*)payloadIEList );
        }
    }
    else if ( frameType == FTDF_MAC_COMMAND_FRAME )
    {
        FTDF_processCommandFrame( rxPtr, frameHeader, securityHeader, payloadIEList );
    }
    else if ( frameType == FTDF_BEACON_FRAME )
    {
        FTDF_Octet* superframeSpecPtr = (FTDF_Octet*)&PANDescriptor.superframeSpec;
        superframeSpecPtr++;

        if ( FTDF_isPANCoordinator )
        {
            if ( frameHeader->srcPANId == FTDF_pib.PANId && ( *superframeSpecPtr & 0x40 ) )
            {
                FTDF_REL_DATA_BUFFER( (FTDF_Octet*)payloadIEList );
                FTDF_sendSyncLossIndication( FTDF_PAN_ID_CONFLICT, securityHeader );
                return;
            }
        }
        else if ( FTDF_pib.associatedPANCoord )
        {
            if ( frameHeader->srcPANId == FTDF_pib.PANId && ( *superframeSpecPtr & 0x40 ) &&
                 ( ( frameHeader->srcAddrMode == FTDF_SHORT_ADDRESS &&
                     frameHeader->srcAddr.shortAddress != FTDF_pib.coordShortAddress ) ||
                   ( frameHeader->srcAddrMode == FTDF_EXTENDED_ADDRESS &&
                     frameHeader->srcAddr.extAddress != FTDF_pib.coordExtAddress ) ) )
            {
                FTDF_REL_DATA_BUFFER( (FTDF_Octet*)payloadIEList );
                FTDF_sendPANIdConflictNotification( frameHeader, securityHeader );
                return;
            }
        }

        FTDF_DataLength beaconPayloadLength = frameLen - ( rxPtr - rxBuffer ) + 1 - micLength - FTDF_FCS_LENGTH;

        if ( FTDF_pib.autoRequest == FTDF_FALSE ||
             beaconPayloadLength != 0 )
        {
            FTDF_Time timestamp = FTDF_GET_FIELD_INDEXED( RETENTION_RAM_RX_TIMESTAMP,
                                                          readBuf );

            FTDF_BeaconNotifyIndication* beaconNotifyIndication =
                (FTDF_BeaconNotifyIndication*) FTDF_GET_MSG_BUFFER(
                    sizeof( FTDF_BeaconNotifyIndication ) );

            beaconNotifyIndication->msgId         = FTDF_BEACON_NOTIFY_INDICATION;
            beaconNotifyIndication->BSN           = frameHeader->SN;
            beaconNotifyIndication->PANDescriptor = &PANDescriptor;
            beaconNotifyIndication->pendAddrSpec  = pendAddrSpec;
            beaconNotifyIndication->addrList      = pendAddrList;
            beaconNotifyIndication->sduLength     = beaconPayloadLength;
            beaconNotifyIndication->sdu           = FTDF_GET_DATA_BUFFER( beaconPayloadLength );
            beaconNotifyIndication->EBSN          = frameHeader->SN;
            beaconNotifyIndication->beaconType    =
                frameHeader->frameVersion == FTDF_FRAME_VERSION_E ? FTDF_ENHANCED_BEACON : FTDF_NORMAL_BEACON;
            beaconNotifyIndication->IEList        = payloadIEList;
            beaconNotifyIndication->timestamp     = timestamp;

            memcpy( beaconNotifyIndication->sdu, rxPtr, beaconPayloadLength );

            FTDF_RCV_MSG( (FTDF_MsgBuffer*) beaconNotifyIndication );
        }
        else if ( FTDF_reqCurrent &&
                  FTDF_reqCurrent->msgId == FTDF_SCAN_REQUEST )
        {
            FTDF_REL_DATA_BUFFER( (FTDF_Octet*)payloadIEList );
            FTDF_addPANdescriptor( &PANDescriptor );
        }
        else
        {
            FTDF_REL_DATA_BUFFER( (FTDF_Octet*)payloadIEList );
        }
    }
#if FTDF_USE_LPDP
#if FTDF_FP_BIT_TEST_MODE
    if (FTDF_lpdpIsEnabled() && !FTDF_reqCurrent && frameType == FTDF_DATA_FRAME) {
            FTDF_processTxPending(frameHeader, securityHeader);
    }
#else /* FTDF_FP_BIT_TEST_MODE */
    if (!FTDF_reqCurrent && frameType == FTDF_DATA_FRAME) {
            FTDF_processTxPending(frameHeader, securityHeader);
    }
#endif /* FTDF_FP_BIT_TEST_MODE */
#endif /* FTDF_USE_LPDP */
#ifndef FTDF_NO_TSCH
    if ( FTDF_pib.tschEnabled == FTDF_TRUE )
    {
        FTDF_scheduleTsch( NULL );
    }
#endif /* FTDF_NO_TSCH */
#endif /* !FTDF_LITE */
}



void FTDF_processRxEvent( void )
{
    volatile uint32_t* rxEvent = (volatile uint32_t*) IND_R_FTDF_ON_OFF_REGMAP_RX_EVENT;

    if ( *rxEvent & MSK_F_FTDF_ON_OFF_REGMAP_RXSOF_E )
    {
#ifdef SIMULATOR
        *rxEvent &= ~MSK_F_FTDF_ON_OFF_REGMAP_RXSOF_E;
#else
        *rxEvent  = MSK_F_FTDF_ON_OFF_REGMAP_RXSOF_E;
#endif
    }

    if ( *rxEvent & MSK_F_FTDF_ON_OFF_REGMAP_RXBYTE_E )
    {
#ifdef SIMULATOR
        *rxEvent &= ~MSK_F_FTDF_ON_OFF_REGMAP_RXBYTE_E;
#else
        *rxEvent  = MSK_F_FTDF_ON_OFF_REGMAP_RXBYTE_E;
#endif
    }

    if ( *rxEvent & MSK_F_FTDF_ON_OFF_REGMAP_RX_OVERFLOW_E )
    {
        // No API defined to report this error to the higher layer, so just clear it.
#ifdef SIMULATOR
        *rxEvent &= ~MSK_F_FTDF_ON_OFF_REGMAP_RX_OVERFLOW_E;
#else
        *rxEvent  = MSK_F_FTDF_ON_OFF_REGMAP_RX_OVERFLOW_E;
#endif
    }

    if ( *rxEvent & MSK_F_FTDF_ON_OFF_REGMAP_RX_BUF_AVAIL_E )
    {
        int readBuf  = FTDF_GET_FIELD( ON_OFF_REGMAP_RX_READ_BUF_PTR );
        int writeBuf = FTDF_GET_FIELD( ON_OFF_REGMAP_RX_WRITE_BUF_PTR );

        while ( readBuf != writeBuf )
        {
            processRxFrame( readBuf % 8 );
            readBuf = ( readBuf + 1 ) % 16;
        }

        FTDF_SET_FIELD( ON_OFF_REGMAP_RX_READ_BUF_PTR, readBuf );

#ifdef SIMULATOR
        *rxEvent &= ~MSK_F_FTDF_ON_OFF_REGMAP_RX_BUF_AVAIL_E;
#else
        *rxEvent  = MSK_F_FTDF_ON_OFF_REGMAP_RX_BUF_AVAIL_E;
#endif
    }

    volatile uint32_t* lmacEvent = (volatile uint32_t*) IND_R_FTDF_ON_OFF_REGMAP_LMAC_EVENT;

    if ( *lmacEvent & MSK_F_FTDF_ON_OFF_REGMAP_EDSCANREADY_E )
    {
#ifdef SIMULATOR
        *lmacEvent &= ~MSK_F_FTDF_ON_OFF_REGMAP_EDSCANREADY_E;
#else
        *lmacEvent  = MSK_F_FTDF_ON_OFF_REGMAP_EDSCANREADY_E;
#endif

#ifndef FTDF_LITE
        FTDF_MsgBuffer* request = FTDF_reqCurrent;

        if ( request->msgId == FTDF_SCAN_REQUEST )
        {
            FTDF_scanReady( (FTDF_ScanRequest*) request );
        }
#endif /* !FTDF_LITE */
    }

    if ( *lmacEvent & MSK_F_FTDF_ON_OFF_REGMAP_RXTIMEREXPIRED_E )
    {
#ifdef SIMULATOR
        *lmacEvent &= ~MSK_F_FTDF_ON_OFF_REGMAP_RXTIMEREXPIRED_E;
#else
        *lmacEvent  = MSK_F_FTDF_ON_OFF_REGMAP_RXTIMEREXPIRED_E;
#endif

        if ( FTDF_pib.metricsEnabled )
        {
            FTDF_pib.performanceMetrics.RxExpiredCount++;
        }
#ifndef FTDF_LITE
#ifndef FTDF_NO_TSCH
        if ( FTDF_pib.tschEnabled )
        {
            FTDF_scheduleTsch( NULL );
        }
        else
#endif /* FTDF_NO_TSCH */
        if ( FTDF_reqCurrent )
        {
            FTDF_MsgId msgId = FTDF_reqCurrent->msgId;

            if ( msgId == FTDF_POLL_REQUEST )
            {
                FTDF_sendPollConfirm( (FTDF_PollRequest*)FTDF_reqCurrent, FTDF_NO_DATA );
            }
            else if ( msgId == FTDF_SCAN_REQUEST )
            {
                FTDF_scanReady( (FTDF_ScanRequest*) FTDF_reqCurrent );
            }
            else if ( msgId == FTDF_ASSOCIATE_REQUEST )
            {
                FTDF_sendAssociateConfirm( (FTDF_AssociateRequest*)FTDF_reqCurrent,
                                           FTDF_NO_DATA,
                                           0xffff );
            }
        }
#endif /* !FTDF_LITE */
    }
#if dg_configBLACK_ORCA_IC_REV != BLACK_ORCA_IC_REV_A
    if ( *lmacEvent & MSK_F_FTDF_ON_OFF_REGMAP_CSMA_CA_BO_THR_E ) {
#ifdef SIMULATOR
        *lmacEvent &= ~MSK_F_FTDF_ON_OFF_REGMAP_CSMA_CA_BO_THR_E;
#else
        *lmacEvent  = MSK_F_FTDF_ON_OFF_REGMAP_CSMA_CA_BO_THR_E;
#endif
#if FTDF_USE_SLEEP_DURING_BACKOFF
        if ( FTDF_pib.metricsEnabled )
        {
                FTDF_pib.performanceMetrics.BOIrqCount++;
        }
        FTDF_sdbFsmBackoffIRQ();
#endif /* FTDF_USE_SLEEP_DURING_BACKOFF */
    }
#endif
}

static void sendConfirm( FTDF_Status status,
                         FTDF_MsgId  msgId )
{
    switch ( msgId )
    {
#ifndef FTDF_LITE
    case FTDF_DATA_REQUEST:
    {
        FTDF_DataRequest*  dataRequest   = (FTDF_DataRequest*) FTDF_reqCurrent;

        FTDF_Time          timestamp     = FTDF_GET_FIELD_INDEXED( RETENTION_RAM_TXTIMESTAMP,
                                                                   FTDF_TX_DATA_BUFFER );
        FTDF_SN            SN            = FTDF_GET_FIELD_INDEXED( RETENTION_RAM_MACSN, FTDF_TX_DATA_BUFFER );
        FTDF_NumOfBackoffs numOfBackoffs = FTDF_GET_FIELD_INDEXED( RETENTION_RAM_CSMACANRRETRIES,
                                                                   FTDF_TX_DATA_BUFFER );

        FTDF_sendDataConfirm( dataRequest, status,
                              timestamp,
                              SN,
                              numOfBackoffs,
                              NULL );

        break;
    }

    case FTDF_POLL_REQUEST:

        if ( status != FTDF_SUCCESS )
        {
            FTDF_sendPollConfirm( (FTDF_PollRequest*)FTDF_reqCurrent, status );
        }

        break;

    case FTDF_ASSOCIATE_REQUEST:

        if ( status != FTDF_SUCCESS )
        {
            FTDF_sendAssociateConfirm( (FTDF_AssociateRequest*)FTDF_reqCurrent,
                                       status,
                                       0xffff );
        }

        break;

    case FTDF_ASSOCIATE_RESPONSE:

        if ( status != FTDF_SUCCESS )
        {
            FTDF_AssociateResponse* assocResp = (FTDF_AssociateResponse*)FTDF_reqCurrent;

            FTDF_Address srcAddr, dstAddr;
            srcAddr.extAddress = FTDF_pib.extAddress;
            dstAddr.extAddress = assocResp->deviceAddress;

            FTDF_sendCommStatusIndication( FTDF_reqCurrent, status,
                                           FTDF_pib.PANId,
                                           FTDF_EXTENDED_ADDRESS,
                                           srcAddr,
                                           FTDF_EXTENDED_ADDRESS,
                                           dstAddr,
                                           assocResp->securityLevel,
                                           assocResp->keyIdMode,
                                           assocResp->keySource,
                                           assocResp->keyIndex );
        }

        break;

    case FTDF_ORPHAN_RESPONSE:

        if ( status != FTDF_SUCCESS )
        {
            FTDF_OrphanResponse* orphanResp = (FTDF_OrphanResponse*)FTDF_reqCurrent;

            FTDF_Address srcAddr, dstAddr;
            srcAddr.extAddress = FTDF_pib.extAddress;
            dstAddr.extAddress = orphanResp->orphanAddress;

            FTDF_sendCommStatusIndication( FTDF_reqCurrent, status,
                                           FTDF_pib.PANId,
                                           FTDF_EXTENDED_ADDRESS,
                                           srcAddr,
                                           FTDF_EXTENDED_ADDRESS,
                                           dstAddr,
                                           orphanResp->securityLevel,
                                           orphanResp->keyIdMode,
                                           orphanResp->keySource,
                                           orphanResp->keyIndex );
        }

        break;

    case FTDF_DISASSOCIATE_REQUEST:

        if ( status != FTDF_SUCCESS )
        {
            FTDF_sendDisassociateConfirm( (FTDF_DisassociateRequest*)FTDF_reqCurrent, status );
        }
        
        break;

    case FTDF_SCAN_REQUEST:

        if ( status != FTDF_SUCCESS )
        {
            FTDF_sendScanConfirm( (FTDF_ScanRequest*)FTDF_reqCurrent, status );
        }

        break;

    case FTDF_BEACON_REQUEST:
        FTDF_sendBeaconConfirm( (FTDF_BeaconRequest*)FTDF_reqCurrent, status );
        break;
    case FTDF_REMOTE_REQUEST:
        FTDF_reqCurrent = NULL;
        break;
#endif /* !FTDF_LITE */
    case FTDF_TRANSPARENT_REQUEST:
    {
#ifndef FTDF_PHY_API
        FTDF_TransparentRequest* transparentRequest = (FTDF_TransparentRequest*) FTDF_reqCurrent;
#endif
        FTDF_Bitmap32            transparentStatus  = 0;

        switch ( status )
        {
        case FTDF_SUCCESS:
            transparentStatus = FTDF_TRANSPARENT_SEND_SUCCESSFUL;
            break;
        case FTDF_CHANNEL_ACCESS_FAILURE:
            transparentStatus = FTDF_TRANSPARENT_CSMACA_FAILURE;
            break;
#if FTDF_TRANSPARENT_WAIT_FOR_ACK
        case FTDF_NO_ACK:
            transparentStatus = FTDF_TRANSPARENT_NO_ACK;
            break;
#endif
        }

        if ( FTDF_pib.metricsEnabled )
        {
            FTDF_pib.performanceMetrics.TXSuccessCount++;
        }
#ifdef FTDF_PHY_API
        FTDF_criticalVar();
        FTDF_enterCritical();
        FTDF_txInProgress = FTDF_FALSE;
        FTDF_exitCritical();

        FTDF_SEND_FRAME_TRANSPARENT_CONFIRM( NULL, transparentStatus );
#else

        FTDF_reqCurrent = NULL;

        FTDF_SEND_FRAME_TRANSPARENT_CONFIRM( transparentRequest->handle,
                                             transparentStatus );

        FTDF_REL_MSG_BUFFER( (FTDF_MsgBuffer*) transparentRequest );
#ifndef FTDF_LITE
        FTDF_processNextRequest( );
#endif /* !FTDF_LITE */

#endif /* FTDF_PHY_API */
        break;
    }
    }
}

void FTDF_processTxEvent( void )
{
    volatile uint32_t* txFlagStatE;
    FTDF_Status        status = FTDF_SUCCESS;

#if FTDF_USE_PTI && !FTDF_USE_AUTO_PTI
    /* Restore Rx PTI in case the Tx transaction that just ended interrupted an Rx-always-on
     * transaction. */
    hw_coex_pti_t tx_pti;
    hw_coex_update_ftdf_pti(FTDF_getRxPti(), &tx_pti, true);
#endif

    txFlagStatE = FTDF_GET_FIELD_ADDR_INDEXED( ON_OFF_REGMAP_TX_FLAG_CLEAR_E, FTDF_TX_WAKEUP_BUFFER );

    if ( *txFlagStatE & MSK_F_FTDF_ON_OFF_REGMAP_TX_FLAG_CLEAR_E )
    {
#ifdef SIMULATOR
        *txFlagStatE &= ~MSK_F_FTDF_ON_OFF_REGMAP_TX_FLAG_CLEAR_E;
#else
        *txFlagStatE  = MSK_F_FTDF_ON_OFF_REGMAP_TX_FLAG_CLEAR_E;
#endif

        volatile uint32_t* txStatus =
            FTDF_GET_REG_ADDR_INDEXED( RETENTION_RAM_TX_RETURN_STATUS_1, FTDF_TX_WAKEUP_BUFFER );

        if ( *txStatus & MSK_F_FTDF_RETENTION_RAM_CSMACAFAIL )
        {
            if ( FTDF_pib.metricsEnabled )
            {
                FTDF_pib.performanceMetrics.TXFailCount++;
            }

            status = FTDF_CHANNEL_ACCESS_FAILURE;
        }
    }

    txFlagStatE = FTDF_GET_FIELD_ADDR_INDEXED( ON_OFF_REGMAP_TX_FLAG_CLEAR_E, FTDF_TX_DATA_BUFFER );

    if ( *txFlagStatE & MSK_F_FTDF_ON_OFF_REGMAP_TX_FLAG_CLEAR_E )
    {
#ifdef SIMULATOR
        *txFlagStatE &= ~MSK_F_FTDF_ON_OFF_REGMAP_TX_FLAG_CLEAR_E;
#else
        *txFlagStatE  = MSK_F_FTDF_ON_OFF_REGMAP_TX_FLAG_CLEAR_E;
#endif
    }
    else
    {
        return;
    }

#ifndef FTDF_PHY_API
    FTDF_txInProgress = FTDF_FALSE;

    if ( FTDF_reqCurrent == NULL )
    {
        return;
    }
#else
    FTDF_criticalVar();
    FTDF_enterCritical();
    if (FTDF_txInProgress == FTDF_FALSE) {
            FTDF_exitCritical();
            return;
    }
    FTDF_exitCritical();
#endif
#if FTDF_USE_SLEEP_DURING_BACKOFF
    FTDF_sdbFsmTxIRQ();
#endif /* FTDF_USE_SLEEP_DURING_BACKOFF */
    FTDF_Boolean ackTX = FTDF_GET_FIELD_INDEXED( RETENTION_RAM_ACKREQUEST, FTDF_TX_DATA_BUFFER );

    if ( status == FTDF_SUCCESS )
    {
        volatile uint32_t* txMeta    = FTDF_GET_REG_ADDR_INDEXED( RETENTION_RAM_TX_META_DATA_0, FTDF_TX_DATA_BUFFER );
        FTDF_FrameType     frameType =
            ( *txMeta & MSK_F_FTDF_RETENTION_RAM_FRAMETYPE ) >> OFF_F_FTDF_RETENTION_RAM_FRAMETYPE;

        switch ( frameType )
        {
        case FTDF_BEACON_FRAME:
            FTDF_pib.trafficCounters.txBeaconFrmCnt++;
            break;
        case FTDF_DATA_FRAME:
            FTDF_pib.trafficCounters.txDataFrmCnt++;
            break;
        case FTDF_MAC_COMMAND_FRAME:
            FTDF_pib.trafficCounters.txCmdFrmCnt++;
            break;
        case FTDF_MULTIPURPOSE_FRAME:
            FTDF_pib.trafficCounters.txMultiPurpFrmCnt++;
            break;
        }

        volatile uint32_t* txStatus =
            FTDF_GET_REG_ADDR_INDEXED( RETENTION_RAM_TX_RETURN_STATUS_1, FTDF_TX_DATA_BUFFER );
#ifndef FTDF_LITE
#ifndef FTDF_NO_TSCH
        FTDF_TschRetry* tschRetry = NULL;

        if ( FTDF_pib.tschEnabled )
        {
            tschRetry = FTDF_getTschRetry( FTDF_getRequestAddress( FTDF_reqCurrent ) );
        }
#endif /* FTDF_NO_TSCH */
#endif /* !FTDF_LITE */
        if ( *txStatus & MSK_F_FTDF_RETENTION_RAM_ACKFAIL )
        {
#ifndef FTDF_LITE
#ifndef FTDF_NO_TSCH
            if ( FTDF_pib.tschEnabled )
            {
                tschRetry->nrOfRetries++;
                FTDF_tschSlotLink->request = NULL;
                status                     = FTDF_scheduleTsch( FTDF_reqCurrent );

                if ( status == FTDF_SUCCESS )
                {
                    // If FTDF_reqCurrent is not equal to NULL the retried request will be queued
                    // rather then send again
                    FTDF_reqCurrent = NULL;
                }
            }
            else
#endif /* FTDF_NO_TSCH */
#endif /* !FTDF_LITE */

            if ( FTDF_nrOfRetries < FTDF_pib.maxFrameRetries )
            {
                FTDF_nrOfRetries++;
#ifndef FTDF_LITE
#ifndef FTDF_NO_CSL
                if ( FTDF_pib.leEnabled )
                {
                    FTDF_setPeerCslTiming( NULL, 0 );

                    FTDF_criticalVar( );
                    FTDF_enterCritical( );

                    FTDF_Time curTime = FTDF_GET_FIELD( ON_OFF_REGMAP_SYMBOLTIMESNAPSHOTVAL );

                    FTDF_txInProgress = FTDF_TRUE;
                    FTDF_SET_FIELD( ON_OFF_REGMAP_MACCSLSTARTSAMPLETIME, curTime + 5 );
                    FTDF_SET_FIELD( ON_OFF_REGMAP_MACWUPERIOD, FTDF_pib.CSLMaxPeriod );

                    volatile uint32_t* txFlagSet = FTDF_GET_FIELD_ADDR( ON_OFF_REGMAP_TX_FLAG_SET );
                    *txFlagSet |= ( ( 1 << FTDF_TX_DATA_BUFFER ) | ( 1 << FTDF_TX_WAKEUP_BUFFER ) );

                    FTDF_exitCritical( );
                }
                else
#endif /* FTDF_NO_CSL */
#endif /* !FTDF_LITE */
                {
#if FTDF_USE_PTI && !FTDF_USE_AUTO_PTI
                    hw_coex_update_ftdf_pti(tx_pti, NULL, true);
#endif
                    volatile uint32_t* txFlagSet = FTDF_GET_FIELD_ADDR( ON_OFF_REGMAP_TX_FLAG_SET );
                    *txFlagSet |= ( 1 << FTDF_TX_DATA_BUFFER );
                }

                return;
            }
            else
            {
                if ( FTDF_pib.metricsEnabled )
                {
                    FTDF_pib.performanceMetrics.TXFailCount++;
                }

                status = FTDF_NO_ACK;
            }
        }
        else if ( *txStatus & MSK_F_FTDF_RETENTION_RAM_CSMACAFAIL )
        {
#ifndef FTDF_LITE
#ifndef FTDF_NO_TSCH
            if ( FTDF_pib.tschEnabled )
            {
                tschRetry->nrOfCcaRetries++;

                if ( tschRetry->nrOfCcaRetries < FTDF_pib.maxCSMABackoffs )
                {
                    FTDF_tschSlotLink->request = NULL;
                    status                     = FTDF_scheduleTsch( FTDF_reqCurrent );

                    if ( status == FTDF_SUCCESS )
                    {
                        // If FTDF_reqCurrent is not equal to NULL the retried request will be queued
                        // rather then send again
                        FTDF_reqCurrent = NULL;
                    }
                }
                else
                {
                    status = FTDF_CHANNEL_ACCESS_FAILURE;
                }
            }
            else
#endif /* FTDF_NO_TSCH */
#endif /* !FTDF_LITE */
            {
                status = FTDF_CHANNEL_ACCESS_FAILURE;
            }

            if ( FTDF_pib.metricsEnabled && status != FTDF_SUCCESS )
            {
                FTDF_pib.performanceMetrics.TXFailCount++;
            }
        }
        else
        {
            if ( ackTX == FTDF_FALSE && FTDF_pib.metricsEnabled )
            {
                FTDF_pib.performanceMetrics.TXSuccessCount++;
            }

#ifndef FTDF_LITE
#ifndef FTDF_NO_TSCH
            if ( FTDF_pib.tschEnabled )
            {
                tschRetry->nrOfCcaRetries = 0;
            }
#endif /* FTDF_NO_TSCH */
#endif /* !FTDF_LITE */
        }
    }

#ifndef FTDF_PHY_API
    if ( ( ackTX == FTDF_FALSE || status != FTDF_SUCCESS ) && FTDF_reqCurrent )
    {
        sendConfirm( status,
                     FTDF_reqCurrent->msgId );
#ifndef FTDF_LITE
        FTDF_processNextRequest( );
#endif /* !FTDF_LITE */
    }

#else
    if ( FTDF_txInProgress )
    {
        sendConfirm(status, FTDF_TRANSPARENT_REQUEST);
    }
#endif
}

void FTDF_processSymbolTimerEvent( void )
{
    volatile uint32_t* symbolTimeThrEvent = FTDF_GET_FIELD_ADDR( ON_OFF_REGMAP_SYMBOLTIMETHR_E );

#ifdef FTDF_PHY_API
    volatile uint32_t* lmacReady4sleepEvent = FTDF_GET_FIELD_ADDR( ON_OFF_REGMAP_LMACREADY4SLEEP_D);

    if (*lmacReady4sleepEvent & MSK_F_FTDF_ON_OFF_REGMAP_LMACREADY4SLEEP_D) {
            *lmacReady4sleepEvent = MSK_F_FTDF_ON_OFF_REGMAP_LMACREADY4SLEEP_D;

            /* If lmac ready 4 sleep, call respective callback, after disabling the interrupt */
            if ( FTDF_GET_FIELD( ON_OFF_REGMAP_LMACREADY4SLEEP ) == 1) {
                    volatile uint32_t* lmacCtrlMask = FTDF_GET_REG_ADDR( ON_OFF_REGMAP_LMAC_CONTROL_MASK );
                    *lmacCtrlMask &= ~MSK_F_FTDF_ON_OFF_REGMAP_LMACREADY4SLEEP_M;
                    FTDF_LMACREADY4SLEEP_CB(FTDF_TRUE, 0);
            }
    }
#endif

    // sync timestamp event
    if ( *symbolTimeThrEvent & MSK_F_FTDF_ON_OFF_REGMAP_SYNCTIMESTAMP_E )
    {
#ifdef SIMULATOR
        *symbolTimeThrEvent &= ~MSK_F_FTDF_ON_OFF_REGMAP_SYNCTIMESTAMP_E;
#else
        *symbolTimeThrEvent  = MSK_F_FTDF_ON_OFF_REGMAP_SYNCTIMESTAMP_E;
#endif

        FTDF_SET_FIELD( ON_OFF_REGMAP_SYNCTIMESTAMPENA, 0 );
#ifndef FTDF_LITE
#ifndef FTDF_NO_CSL
        FTDF_oldLeEnabled = FTDF_FALSE;

        if ( FTDF_wakeUpEnableLe )
        {
            FTDF_pib.leEnabled  = FTDF_TRUE;
            FTDF_setLeEnabled( );
            FTDF_wakeUpEnableLe = FTDF_FALSE;
        }
#endif /* FTDF_NO_CSL */

#ifndef FTDF_NO_TSCH
        if ( FTDF_wakeUpEnableTsch )
        {
            FTDF_setTschEnabled( );
        }
#endif /* FTDF_NO_TSCH */

        FTDF_restoreTxPendingTimer( );
#endif /* !FTDF_LITE */
        FTDF_WAKE_UP_READY( );
    }

    // miscellaneous event
    // - Non-TSCH mode: association timer
    // - TSCH mode: next active link timer
    if ( *symbolTimeThrEvent & MSK_F_FTDF_ON_OFF_REGMAP_SYMBOLTIME2THR_E )
    {
#ifdef SIMULATOR
        *symbolTimeThrEvent &= ~MSK_F_FTDF_ON_OFF_REGMAP_SYMBOLTIME2THR_E;
#else
        *symbolTimeThrEvent  = MSK_F_FTDF_ON_OFF_REGMAP_SYMBOLTIME2THR_E;
#endif

#ifndef FTDF_LITE
#ifndef FTDF_NO_TSCH
        if ( FTDF_pib.tschEnabled )
        {
            FTDF_tschProcessRequest( );
        }
        else
#endif /* FTDF_NO_TSCH */
        if ( FTDF_reqCurrent &&
             FTDF_reqCurrent->msgId == FTDF_ASSOCIATE_REQUEST )
        {
            FTDF_AssocAdmin* assocAdmin = &FTDF_aa;

            // macResponseWaitTime expired
            assocAdmin->dataR = FTDF_TRUE;

            FTDF_sendAssociateDataRequest( );
        }
#endif /* !FTDF_LITE */
    }

    // pending queue symbol timer event
    if ( *symbolTimeThrEvent & MSK_F_FTDF_ON_OFF_REGMAP_SYMBOLTIMETHR_E )
    {
#ifdef SIMULATOR
        *symbolTimeThrEvent &= ~MSK_F_FTDF_ON_OFF_REGMAP_SYMBOLTIMETHR_E;
#else
        *symbolTimeThrEvent  = MSK_F_FTDF_ON_OFF_REGMAP_SYMBOLTIMETHR_E;
#endif

#ifndef FTDF_LITE
        FTDF_removeTxPendingTimer( NULL );
#endif /* !FTDF_LITE */
    }
}

#ifndef FTDF_LITE
FTDF_Status FTDF_sendFrame( FTDF_ChannelNumber   channel,
                            FTDF_FrameHeader*    frameHeader,
                            FTDF_SecurityHeader* securityHeader,
                            FTDF_Octet*          txPtr,
                            FTDF_DataLength      payloadSize,
                            FTDF_Octet*          payload )
{
    FTDF_Octet*     txBufPtr       = (FTDF_Octet*) FTDF_GET_REG_ADDR( RETENTION_RAM_TX_FIFO );

    FTDF_DataLength micLength      = FTDF_getMicLength( securityHeader->securityLevel );
    FTDF_DataLength phyPayloadSize = ( txPtr - txBufPtr ) - 1 + payloadSize + micLength + FTDF_FCS_LENGTH;

    if ( phyPayloadSize > ( FTDF_BUFFER_LENGTH - 1 ) )
    {
        return FTDF_FRAME_TOO_LONG;
    }

    *txBufPtr = phyPayloadSize;

    FTDF_Octet* privPtr = txPtr;

    int         n;

    for ( n = 0; n < payloadSize; n++ )
    {
        *txPtr++ = *payload++;
    }

    FTDF_Status status = FTDF_secureFrame( txBufPtr,
                                           privPtr,
                                           frameHeader,
                                           securityHeader );

    if ( status != FTDF_SUCCESS )
    {
        return status;
    }

    FTDF_Bitmap8       options   = frameHeader->options;

    volatile uint32_t* metaData0 = FTDF_GET_REG_ADDR_INDEXED( RETENTION_RAM_TX_META_DATA_0, FTDF_TX_DATA_BUFFER );
    volatile uint32_t* metaData1 = FTDF_GET_REG_ADDR_INDEXED( RETENTION_RAM_TX_META_DATA_1, FTDF_TX_DATA_BUFFER );

    uint16_t           phyAttr   = ( FTDF_pib.CCAMode & 0x3 ) | 0x08 | ( ( channel - 11 ) & 0x0F ) << 4 |
                                   ( FTDF_pib.TXPower & 0x07) << 12;

    metaData0  = FTDF_GET_REG_ADDR_INDEXED( RETENTION_RAM_TX_META_DATA_0, FTDF_TX_DATA_BUFFER );
    metaData1  = FTDF_GET_REG_ADDR_INDEXED( RETENTION_RAM_TX_META_DATA_1, FTDF_TX_DATA_BUFFER );

    *metaData0 =
        ( ( phyPayloadSize << OFF_F_FTDF_RETENTION_RAM_FRAME_LENGTH ) & MSK_F_FTDF_RETENTION_RAM_FRAME_LENGTH ) |
        ( ( phyAttr << OFF_F_FTDF_RETENTION_RAM_PHYATTR ) & MSK_F_FTDF_RETENTION_RAM_PHYATTR ) |
        ( ( frameHeader->frameType << OFF_F_FTDF_RETENTION_RAM_FRAMETYPE ) & MSK_F_FTDF_RETENTION_RAM_FRAMETYPE ) |
        MSK_F_FTDF_RETENTION_RAM_CSMACA_ENA |
        ( ( options & FTDF_OPT_ACK_REQUESTED ) ? MSK_F_FTDF_RETENTION_RAM_ACKREQUEST : 0 ) |
        MSK_F_FTDF_RETENTION_RAM_CRC16_ENA;

    *metaData1 =
        ( ( frameHeader->SN << OFF_F_FTDF_RETENTION_RAM_MACSN ) & MSK_F_FTDF_RETENTION_RAM_MACSN );

    uint32_t phyCsmaCaAttr = ( FTDF_pib.CCAMode & 0x3 ) | ( ( channel - 11 ) & 0xf ) << 4 |
            ( FTDF_pib.TXPower & 0x07) << 12;
    FTDF_SET_FIELD( ON_OFF_REGMAP_PHYCSMACAATTR, phyCsmaCaAttr );

#ifndef FTDF_NO_CSL
    if ( FTDF_pib.leEnabled == FTDF_TRUE )
    {
        if ( frameHeader->dstAddrMode != FTDF_SHORT_ADDRESS )
        {
            return FTDF_NO_SHORT_ADDRESS;
        }

        // Clear CSMACA of data frame buffer
        *metaData0 &= ~MSK_F_FTDF_RETENTION_RAM_CSMACA_ENA;

        txPtr       = txBufPtr = ( (FTDF_Octet*) FTDF_GET_REG_ADDR( RETENTION_RAM_TX_FIFO ) ) +
                                 ( FTDF_BUFFER_LENGTH * FTDF_TX_WAKEUP_BUFFER );

        *txPtr++                    = 0x0d;
        *txPtr++                    = 0x2d;
        *txPtr++                    = 0x81;
        *txPtr++                    = frameHeader->SN;
        *(FTDF_PANId*) txPtr        = frameHeader->dstPANId;
        txPtr                      += 2;
        *(FTDF_ShortAddress*) txPtr = frameHeader->dstAddr.shortAddress;
        txPtr                      += 2;
        *txPtr++                    = 0x82;
        *txPtr++                    = 0x0e;

        metaData0                   = FTDF_GET_REG_ADDR_INDEXED( RETENTION_RAM_TX_META_DATA_0, FTDF_TX_WAKEUP_BUFFER );
        metaData1                   = FTDF_GET_REG_ADDR_INDEXED( RETENTION_RAM_TX_META_DATA_1, FTDF_TX_WAKEUP_BUFFER );
        volatile uint32_t* txPriority =
            FTDF_GET_REG_ADDR_INDEXED( ON_OFF_REGMAP_TX_PRIORITY, FTDF_TX_WAKEUP_BUFFER );

        *metaData0 =
            ( ( 0x0d << OFF_F_FTDF_RETENTION_RAM_FRAME_LENGTH ) & MSK_F_FTDF_RETENTION_RAM_FRAME_LENGTH ) |
            ( ( phyAttr << OFF_F_FTDF_RETENTION_RAM_PHYATTR ) & MSK_F_FTDF_RETENTION_RAM_PHYATTR ) |
            ( ( FTDF_MULTIPURPOSE_FRAME << OFF_F_FTDF_RETENTION_RAM_FRAMETYPE ) & MSK_F_FTDF_RETENTION_RAM_FRAMETYPE ) |
            MSK_F_FTDF_RETENTION_RAM_CSMACA_ENA |
            MSK_F_FTDF_RETENTION_RAM_CRC16_ENA;

        *metaData1 =
            ( ( frameHeader->SN << OFF_F_FTDF_RETENTION_RAM_MACSN ) & MSK_F_FTDF_RETENTION_RAM_MACSN );
#if FTDF_USE_PTI && FTDF_USE_AUTO_PTI
        *txPriority |= MSK_F_FTDF_ON_OFF_REGMAP_ISWAKEUP;
#else
        *txPriority = MSK_F_FTDF_ON_OFF_REGMAP_ISWAKEUP;
#endif
    }
#endif /* FTDF_NO_CSL */

    volatile uint32_t* txFlagSet = FTDF_GET_FIELD_ADDR( ON_OFF_REGMAP_TX_FLAG_SET );

#ifndef FTDF_NO_CSL
    if ( FTDF_pib.leEnabled == FTDF_TRUE )
    {
        FTDF_Time curTime = FTDF_GET_FIELD( ON_OFF_REGMAP_SYMBOLTIMESNAPSHOTVAL );
        FTDF_Time delta   = curTime - FTDF_rzTime;

        if ( delta > 0x80000000 )
        {
            // Receiving an wakeup frame sequence, delay sending until RZ has passed.
            FTDF_sendFramePending = frameHeader->dstAddr.shortAddress;
        }
        else
        {
            FTDF_Time   wakeupStartTime;
            FTDF_Period wakeupPeriod;

            FTDF_criticalVar( );
            FTDF_enterCritical( );

            FTDF_getWakeupParams( frameHeader->dstAddr.shortAddress, &wakeupStartTime, &wakeupPeriod );

            FTDF_txInProgress = FTDF_TRUE;
            FTDF_SET_FIELD( ON_OFF_REGMAP_MACCSLSTARTSAMPLETIME, wakeupStartTime );
            FTDF_SET_FIELD( ON_OFF_REGMAP_MACWUPERIOD, wakeupPeriod );

            *txFlagSet |= ( ( 1 << FTDF_TX_DATA_BUFFER ) | ( 1 << FTDF_TX_WAKEUP_BUFFER ) );

            FTDF_exitCritical( );
        }
    }
    else
#endif /* FTDF_NO_CSL */
    if ( !FTDF_pib.tschEnabled )
    {
        *txFlagSet |= ( 1 << FTDF_TX_DATA_BUFFER );
//                SetWord16(P2_SET_DATA_REG, (1 << 3));
//                        SetWord16(P23_MODE_REG, 0x300); // SW trigger - output
//                        for (volatile int k = 0; k < 100; k++) {
//                        }
//                        SetWord16(P23_MODE_REG, 0x000);// SW trigger - high Z


    }

    return FTDF_SUCCESS;
}

#if !defined(FTDF_NO_CSL) || !defined(FTDF_NO_TSCH)
FTDF_Status FTDF_sendAckFrame( FTDF_FrameHeader*    frameHeader,
                               FTDF_SecurityHeader* securityHeader,
                               FTDF_Octet*          txPtr )
{
    FTDF_Octet*     txBufPtr       = ( (FTDF_Octet*) FTDF_GET_REG_ADDR( RETENTION_RAM_TX_FIFO ) ) +
                                     2 * FTDF_BUFFER_LENGTH;
    FTDF_DataLength micLength      = FTDF_getMicLength( securityHeader->securityLevel );
    FTDF_DataLength phyPayloadSize = ( txPtr - txBufPtr ) - 1 + micLength + FTDF_FCS_LENGTH;

    *txBufPtr = phyPayloadSize;

    FTDF_Status status = FTDF_secureFrame( txBufPtr,
                                           txPtr,
                                           frameHeader,
                                           securityHeader );

    if ( status != FTDF_SUCCESS )
    {
        return status;
    }

    volatile uint32_t* metaData0  = FTDF_GET_REG_ADDR_INDEXED( RETENTION_RAM_TX_META_DATA_0, FTDF_TX_ACK_BUFFER );
    volatile uint32_t* metaData1  = FTDF_GET_REG_ADDR_INDEXED( RETENTION_RAM_TX_META_DATA_1, FTDF_TX_ACK_BUFFER );
    volatile uint32_t* txPriority = FTDF_GET_REG_ADDR_INDEXED( ON_OFF_REGMAP_TX_PRIORITY, FTDF_TX_ACK_BUFFER );

    uint16_t           phyAttr    =
        ( FTDF_pib.CCAMode & 0x3 ) | 0x08 | ( FTDF_GET_FIELD( ON_OFF_REGMAP_PHYRXATTR ) & 0x00f0 ) |
        ( FTDF_pib.TXPower & 0x07 ) << 12;

    *metaData0 =
        ( ( phyPayloadSize << OFF_F_FTDF_RETENTION_RAM_FRAME_LENGTH ) & MSK_F_FTDF_RETENTION_RAM_FRAME_LENGTH ) |
        ( ( phyAttr << OFF_F_FTDF_RETENTION_RAM_PHYATTR ) & MSK_F_FTDF_RETENTION_RAM_PHYATTR ) |
        ( ( FTDF_ACKNOWLEDGEMENT_FRAME << OFF_F_FTDF_RETENTION_RAM_FRAMETYPE ) & MSK_F_FTDF_RETENTION_RAM_FRAMETYPE ) |
        MSK_F_FTDF_RETENTION_RAM_CRC16_ENA;

    *metaData1 =
        ( ( frameHeader->SN << OFF_F_FTDF_RETENTION_RAM_MACSN ) & MSK_F_FTDF_RETENTION_RAM_MACSN );

#if FTDF_USE_PTI && FTDF_USE_AUTO_PTI
    *txPriority |= 1;
#else
    *txPriority = 1;
#endif

    volatile uint32_t* txFlagSet = FTDF_GET_FIELD_ADDR( ON_OFF_REGMAP_TX_FLAG_SET );

#ifndef FTDF_NO_TSCH
    if ( FTDF_pib.tschEnabled )
    {
        FTDF_criticalVar( );
        FTDF_enterCritical( );

        FTDF_Period txAckDelayVal = FTDF_GET_FIELD( ON_OFF_REGMAP_MACTSTXACKDELAYVAL );

        if ( txAckDelayVal > 20 )
        {
            *txFlagSet |= 1 << FTDF_TX_ACK_BUFFER;
        }

        FTDF_exitCritical( );
    }
    else
#endif /* FTDF_NO_TSCH */
    {
        *txFlagSet |= 1 << FTDF_TX_ACK_BUFFER;
    }

    FTDF_pib.trafficCounters.txEnhAckFrmCnt++;

    return FTDF_SUCCESS;
}
#endif /* !FTDF_NO_CSL || !FTDF_NO_TSCH */
#endif /* !FTDF_LITE */

void FTDF_sendTransparentFrame( FTDF_DataLength    frameLength,
                                FTDF_Octet*        frame,
                                FTDF_ChannelNumber channel,
                                FTDF_PTI           pti,
                                FTDF_Boolean       cmsaSuppress )
{
    volatile uint32_t* metaData0 = FTDF_GET_REG_ADDR_INDEXED( RETENTION_RAM_TX_META_DATA_0, FTDF_TX_DATA_BUFFER );
    volatile uint32_t* metaData1 = FTDF_GET_REG_ADDR_INDEXED( RETENTION_RAM_TX_META_DATA_1, FTDF_TX_DATA_BUFFER );
    volatile uint32_t* txPriority = FTDF_GET_REG_ADDR_INDEXED( ON_OFF_REGMAP_TX_PRIORITY, FTDF_TX_DATA_BUFFER );

#if FTDF_TRANSPARENT_USE_WAIT_FOR_ACK
    FTDF_Boolean useAck = FTDF_FALSE;
    //FTDF_FrameHeader*         frameHeader    = &FTDF_fh;
    FTDF_FrameHeader frameHeader;
    FTDF_SN SN;
#endif
    uint16_t           phyAttr   = ( FTDF_pib.CCAMode & 0x3 ) | 0x08 | ( ( channel - 11 ) & 0x0F ) << 4 |
                                   ( FTDF_pib.TXPower & 0x07) << 12;
#if FTDF_TRANSPARENT_USE_WAIT_FOR_ACK
    if (FTDF_transparentModeOptions & FTDF_TRANSPARENT_WAIT_FOR_ACK)
    {
            FTDF_getFrameHeader(frame, &frameHeader);
            if (frameHeader.options & FTDF_OPT_ACK_REQUESTED)
            {
                    useAck = FTDF_TRUE;
            }
            SN = frameHeader.SN;
    }
#endif

#if dg_configBLACK_ORCA_IC_REV != BLACK_ORCA_IC_REV_A
    *txPriority = ((pti << OFF_F_FTDF_ON_OFF_REGMAP_PTI_TX) &
            MSK_F_FTDF_ON_OFF_REGMAP_PTI_TX) | 1;
#endif
    *metaData0 =
        ( ( frameLength << OFF_F_FTDF_RETENTION_RAM_FRAME_LENGTH ) & MSK_F_FTDF_RETENTION_RAM_FRAME_LENGTH ) |
        ( ( phyAttr << OFF_F_FTDF_RETENTION_RAM_PHYATTR ) & MSK_F_FTDF_RETENTION_RAM_PHYATTR ) |
        ( ( ( *frame & 0x7 ) << OFF_F_FTDF_RETENTION_RAM_FRAMETYPE ) & MSK_F_FTDF_RETENTION_RAM_FRAMETYPE ) |
        ( ( cmsaSuppress ) ? 0 : MSK_F_FTDF_RETENTION_RAM_CSMACA_ENA ) |
#if FTDF_TRANSPARENT_USE_WAIT_FOR_ACK
        ( ( useAck ) ? MSK_F_FTDF_RETENTION_RAM_ACKREQUEST : 0 ) |
#endif
        ( ( FTDF_transparentModeOptions &
            FTDF_TRANSPARENT_ENABLE_FCS_GENERATION ) ? MSK_F_FTDF_RETENTION_RAM_CRC16_ENA : 0 );

#if FTDF_TRANSPARENT_USE_WAIT_FOR_ACK
    if (useAck)
    {
        *metaData1 = ( ( SN << OFF_F_FTDF_RETENTION_RAM_MACSN ) & MSK_F_FTDF_RETENTION_RAM_MACSN );
    }
    else
    {
        *metaData1 = ( ( 0 << OFF_F_FTDF_RETENTION_RAM_MACSN ) & MSK_F_FTDF_RETENTION_RAM_MACSN );
    }
#else
    *metaData1 =
        ( ( 0 << OFF_F_FTDF_RETENTION_RAM_MACSN ) & MSK_F_FTDF_RETENTION_RAM_MACSN );
#endif

    uint32_t phyCsmaCaAttr = ( FTDF_pib.CCAMode & 0x3 ) | ( ( channel - 11 ) & 0xf ) << 4 |
            ( FTDF_pib.TXPower & 0x07) << 12;
    FTDF_SET_FIELD( ON_OFF_REGMAP_PHYCSMACAATTR, phyCsmaCaAttr );
#if FTDF_USE_PTI && !FTDF_USE_AUTO_PTI
    hw_coex_update_ftdf_pti((hw_coex_pti_t) pti, NULL, true);
#else
    // FTDF_SET_FIELD(ON_OFF_REGMAP_PTI_TX, pti);
#endif
    volatile uint32_t* txFlagSet = FTDF_GET_FIELD_ADDR( ON_OFF_REGMAP_TX_FLAG_SET );
    *txFlagSet |= ( 1 << FTDF_TX_DATA_BUFFER );
}

void FTDF_initQueues( void )
{
#ifndef FTDF_LITE
    FTDF_initQueue( &FTDF_freeQueue );
    FTDF_initQueue( &FTDF_reqQueue );

    int n;

    for ( n = 0; n < FTDF_NR_OF_REQ_BUFFERS; n++ )
    {
        FTDF_queueBufferHead( &FTDF_reqBuffers[ n ], &FTDF_freeQueue );

        FTDF_txPendingList[ n ].addr.extAddress = 0xFFFFFFFFFFFFFFFFLL;
        FTDF_txPendingList[ n ].addrMode        = FTDF_NO_ADDRESS;
        FTDF_txPendingList[ n ].PANId           = 0xFFFF;
        FTDF_initQueue( &FTDF_txPendingList[ n ].queue );

        FTDF_txPendingTimerList[ n ].free = FTDF_TRUE;
        FTDF_txPendingTimerList[ n ].next = NULL;
    }

    FTDF_txPendingTimerHead = FTDF_txPendingTimerList;
    FTDF_txPendingTimerTime = 0;
#endif /* !FTDF_LITE */
#ifndef FTDF_PHY_API
    FTDF_reqCurrent         = NULL;
#endif
}

#ifndef FTDF_LITE
void FTDF_initQueue( FTDF_Queue* queue )
{
    queue->head.next = (FTDF_Buffer*) &queue->tail;
    queue->head.prev = NULL;
    queue->tail.next = NULL;
    queue->tail.prev = (FTDF_Buffer*) &queue->head;
}

void FTDF_queueBufferHead( FTDF_Buffer* buffer, FTDF_Queue* queue )
{
    FTDF_Buffer* next = queue->head.next;

    queue->head.next    = buffer;
    next->header.prev   = buffer;
    buffer->header.next = next;
    buffer->header.prev = (FTDF_Buffer*) &queue->head;
}

FTDF_Buffer* FTDF_dequeueBufferTail( FTDF_Queue* queue )
{
    FTDF_Buffer* buffer = queue->tail.prev;

    if ( buffer->header.prev == NULL )
    {
        return NULL;
    }

    queue->tail.prev                 = buffer->header.prev;
    buffer->header.prev->header.next = (FTDF_Buffer*) &queue->tail;

    return buffer;
}

FTDF_Status FTDF_queueReqHead( FTDF_MsgBuffer* request, FTDF_Queue* queue )
{
    FTDF_Buffer* buffer = FTDF_dequeueBufferTail( &FTDF_freeQueue );

    if ( buffer == NULL )
    {
        return FTDF_TRANSACTION_OVERFLOW;
    }

    FTDF_Buffer* next = queue->head.next;

    queue->head.next    = buffer;
    next->header.prev   = buffer;
    buffer->header.next = next;
    buffer->header.prev = (FTDF_Buffer*) &queue->head;
    buffer->request     = request;

    return FTDF_SUCCESS;
}

FTDF_MsgBuffer* FTDF_dequeueReqTail( FTDF_Queue* queue )
{
    FTDF_Buffer* buffer = queue->tail.prev;

    if ( buffer->header.prev == NULL )
    {
        return NULL;
    }

    queue->tail.prev                 = buffer->header.prev;
    buffer->header.prev->header.next = (FTDF_Buffer*) &queue->tail;

    FTDF_MsgBuffer* request = buffer->request;

    FTDF_queueBufferHead( buffer, &FTDF_freeQueue );

    return request;
}

FTDF_MsgBuffer* FTDF_dequeueByHandle( FTDF_Handle handle, FTDF_Queue* queue )
{
    FTDF_Buffer* buffer = queue->head.next;

    while ( buffer->header.next != NULL )
    {
        if ( buffer->request &&
             buffer->request->msgId == FTDF_DATA_REQUEST &&
             ( (FTDF_DataRequest*) buffer->request )->msduHandle == handle )
        {
            buffer->header.prev->header.next = buffer->header.next;
            buffer->header.next->header.prev = buffer->header.prev;

            FTDF_MsgBuffer* request = buffer->request;

            FTDF_queueBufferHead( buffer, &FTDF_freeQueue );

            return request;
        }

        buffer = buffer->header.next;
    }

    return NULL;
}

FTDF_MsgBuffer* FTDF_dequeueByRequest( FTDF_MsgBuffer* request, FTDF_Queue* queue )
{
    FTDF_Buffer* buffer = queue->head.next;

    while ( buffer->header.next != NULL )
    {
        if ( buffer->request == request )
        {
            buffer->header.prev->header.next = buffer->header.next;
            buffer->header.next->header.prev = buffer->header.prev;

            FTDF_MsgBuffer* req = buffer->request;

            FTDF_queueBufferHead( buffer, &FTDF_freeQueue );

            return req;
        }

        buffer = buffer->header.next;
    }

    return NULL;
}

FTDF_Boolean FTDF_isQueueEmpty( FTDF_Queue* queue )
{
    if ( queue->head.next->header.next == NULL )
    {
        return FTDF_TRUE;
    }
    else
    {
        return FTDF_FALSE;
    }
}

static FTDF_PendingTL* FTDF_findFreePendingTimer( void )
{
    uint8_t i;

    for ( i = 0; i < FTDF_NR_OF_REQ_BUFFERS; i++ )
    {
        if ( FTDF_txPendingTimerList[ i ].free == FTDF_TRUE )
        {
            break;
        }
    }

    return &FTDF_txPendingTimerList[ i ];
}

void FTDF_addTxPendingTimer( FTDF_MsgBuffer* request, uint8_t pendListNr, FTDF_Time delta, void ( * func )(
                                 FTDF_PendingTL* ) )
{
    FTDF_criticalVar( );
    FTDF_enterCritical( );

    FTDF_PendingTL* ptr       = FTDF_txPendingTimerHead;
    FTDF_Time       timestamp = FTDF_GET_FIELD( ON_OFF_REGMAP_SYMBOLTIMESNAPSHOTVAL );

    if ( ptr->free == FTDF_FALSE )
    {
        while ( ptr )
        {
            ptr->delta -= timestamp - FTDF_txPendingTimerLT;
            ptr         = ptr->next;
        }
    }

    FTDF_txPendingTimerLT = timestamp;
    ptr                   = FTDF_txPendingTimerHead;

    if ( ptr->free == FTDF_TRUE )
    {
        ptr->free       = FTDF_FALSE;
        ptr->next       = NULL;
        ptr->request    = request;
        ptr->delta      = delta;
        ptr->pendListNr = pendListNr;
        ptr->func       = func;

        FTDF_SET_FIELD( ON_OFF_REGMAP_SYMBOLTIMETHR, delta + timestamp );
        FTDF_txPendingTimerTime = delta + timestamp;
        FTDF_exitCritical( );
        return;
    }

    if ( ptr->delta > delta )
    {
        FTDF_txPendingTimerHead             = FTDF_findFreePendingTimer( );
        FTDF_txPendingTimerHead->free       = FTDF_FALSE;
        FTDF_txPendingTimerHead->next       = ptr;
        FTDF_txPendingTimerHead->request    = request;
        FTDF_txPendingTimerHead->delta      = delta;
        FTDF_txPendingTimerHead->pendListNr = pendListNr;
        FTDF_txPendingTimerHead->func       = func;

        FTDF_SET_FIELD( ON_OFF_REGMAP_SYMBOLTIMETHR, delta + timestamp );
        FTDF_txPendingTimerTime = delta + timestamp;
        FTDF_exitCritical( );
        return;
    }
    else if ( ptr->delta == delta )
    {
        delta++;
    }

    FTDF_PendingTL* prev;

    while ( ptr->next )
    {
        prev = ptr;
        ptr  = ptr->next;

        if ( ptr->delta == delta )
        {
            delta++;
        }

        if ( prev->delta < delta && ptr->delta > delta )
        {
            prev->next       = FTDF_findFreePendingTimer( );
            prev->next->next = ptr;
            ptr              = prev->next;
            ptr->free        = FTDF_FALSE;
            ptr->request     = request;
            ptr->delta       = delta;
            ptr->pendListNr  = pendListNr;
            ptr->func        = func;

            FTDF_exitCritical( );
            return;
        }
    }

    ptr->next       = FTDF_findFreePendingTimer( );
    ptr             = ptr->next;
    ptr->free       = FTDF_FALSE;
    ptr->next       = NULL;
    ptr->request    = request;
    ptr->delta      = delta;
    ptr->pendListNr = pendListNr;
    ptr->func       = func;

    FTDF_exitCritical( );
}

void FTDF_removeTxPendingTimer( FTDF_MsgBuffer* request )
{
    FTDF_criticalVar( );
    FTDF_enterCritical( );

    FTDF_PendingTL* ptr       = FTDF_txPendingTimerHead;
    FTDF_Time       timestamp = FTDF_GET_FIELD( ON_OFF_REGMAP_SYMBOLTIMESNAPSHOTVAL );

    if ( ptr->free == FTDF_TRUE )
    {
        FTDF_SET_FIELD( ON_OFF_REGMAP_SYMBOLTIMETHR, timestamp - 1 );
        FTDF_txPendingTimerTime = timestamp - 1;
        FTDF_exitCritical( );
        return;
    }

    while ( ptr )
    {
        ptr->delta -= timestamp - FTDF_txPendingTimerLT;
        ptr         = ptr->next;
    }

    FTDF_txPendingTimerLT = timestamp;
    ptr                   = FTDF_txPendingTimerHead;

    if ( !request || ptr->request == request )
    {
        if ( ptr->next )
        {
            FTDF_PendingTL* temp = ptr;

            if ( ptr->next->delta < 75 )
            {
                while ( temp )
                {
                    temp->delta += 75;
                    temp         = temp->next;
                }
            }

            FTDF_SET_FIELD( ON_OFF_REGMAP_SYMBOLTIMETHR, timestamp + ptr->next->delta );
            FTDF_txPendingTimerTime = timestamp + ptr->next->delta;
            FTDF_txPendingTimerHead = ptr->next;

            ptr->free               = FTDF_TRUE;
            ptr->next               = NULL;
        }
        else
        {
            FTDF_SET_FIELD( ON_OFF_REGMAP_SYMBOLTIMETHR, timestamp - 1 );
            FTDF_txPendingTimerTime = timestamp - 1;

            ptr->free = FTDF_TRUE;
            ptr->next = NULL;
        }

        FTDF_exitCritical( );

        if ( !request )
        {
            if ( ptr->func )
            {
                ptr->func( ptr );
            }
        }

        return;
    }

    FTDF_PendingTL* prev = ptr;

    while ( ptr->next )
    {
        prev = ptr;
        ptr  = ptr->next;

        if ( ptr->request == request )
        {
            prev->next = ptr->next;
            ptr->free  = FTDF_TRUE;
            ptr->next  = NULL;

            FTDF_exitCritical( );
            return;
        }
    }

    FTDF_SET_FIELD( ON_OFF_REGMAP_SYMBOLTIMETHR, timestamp - 1 );
    FTDF_txPendingTimerTime = timestamp - 1;
    FTDF_exitCritical( );
}

void FTDF_restoreTxPendingTimer( void )
{
    FTDF_SET_FIELD( ON_OFF_REGMAP_SYMBOLTIMETHR, FTDF_txPendingTimerTime );
}

FTDF_Boolean FTDF_getTxPendingTimerHead( FTDF_Time* time )
{
    FTDF_PendingTL* ptr = FTDF_txPendingTimerHead;

    if ( ptr->free == FTDF_TRUE )
    {
        return FTDF_FALSE;
    }

    *time = FTDF_txPendingTimerTime;
    return FTDF_TRUE;
}

#ifndef FTDF_NO_TSCH
void FTDF_processKeepAliveTimerExp( FTDF_PendingTL* ptr )
{
    FTDF_RemoteRequest* remoteRequest = (FTDF_RemoteRequest*)ptr->request;

    remoteRequest->msgId    = FTDF_REMOTE_REQUEST;
    remoteRequest->remoteId = FTDF_REMOTE_KEEP_ALIVE;
    remoteRequest->dstAddr  = FTDF_neighborTable[ ptr->pendListNr ].dstAddr;

    FTDF_processRemoteRequest( remoteRequest );
}
#endif /* FTDF_NO_TSCH */

void FTDF_sendTransactionExpired( FTDF_PendingTL* ptr )
{
    FTDF_MsgBuffer* req =
        FTDF_dequeueByRequest( ptr->request, &FTDF_txPendingList[ ptr->pendListNr ].queue );

    if ( !req )
    {
        return;
    }

    if ( FTDF_isQueueEmpty( &FTDF_txPendingList[ ptr->pendListNr ].queue ) )
    {
#ifndef FTDF_NO_TSCH
        if ( FTDF_pib.tschEnabled )
        {
            FTDF_txPendingList[ ptr->pendListNr ].addr.shortAddress = 0xfffe;
        }
        else
#endif /* FTDF_NO_TSCH */
        {
#if FTDF_FP_BIT_MODE == FTDF_FP_BIT_MODE_AUTO
            if (FTDF_txPendingList[ ptr->pendListNr ].addrMode == FTDF_SHORT_ADDRESS)
            {
                uint8_t entry, shortAddrIdx;
                FTDF_Boolean found = FTDF_fpprLookupShortAddress(
                        FTDF_txPendingList[ ptr->pendListNr ].addr.shortAddress, &entry,
                        &shortAddrIdx);
                ASSERT_WARNING(found);
                FTDF_fpprSetShortAddressValid(entry, shortAddrIdx, FTDF_FALSE);
            }
            else if (FTDF_txPendingList[ ptr->pendListNr ].addrMode == FTDF_EXTENDED_ADDRESS)
            {
                uint8_t entry;
                FTDF_Boolean found = FTDF_fpprLookupExtAddress(
                        FTDF_txPendingList[ ptr->pendListNr ].addr.extAddress, &entry);
                ASSERT_WARNING(found);
                FTDF_fpprSetExtAddressValid(entry, FTDF_FALSE);
            }
            else
            {
                ASSERT_WARNING(0);
            }
#endif /* FTDF_FP_BIT_MODE == FTDF_FP_BIT_MODE_AUTO */
            FTDF_txPendingList[ ptr->pendListNr ].addrMode = FTDF_NO_ADDRESS;
        }
    }

    switch ( req->msgId )
    {
    case FTDF_DATA_REQUEST:
    {
        FTDF_DataRequest* dataRequest = (FTDF_DataRequest*)req;

        FTDF_sendDataConfirm( dataRequest, FTDF_TRANSACTION_EXPIRED, 0, 0, 0, NULL );

        break;
    }
    case FTDF_ASSOCIATE_REQUEST:
    {
        FTDF_AssociateRequest* associateRequest = (FTDF_AssociateRequest*)req;

        FTDF_sendAssociateConfirm( associateRequest, FTDF_TRANSACTION_EXPIRED, 0xffff );

        break;
    }
    case FTDF_ASSOCIATE_RESPONSE:
    {
        FTDF_AssociateResponse* assocResp = (FTDF_AssociateResponse*)req;

        FTDF_Address srcAddr, dstAddr;
        srcAddr.extAddress = FTDF_pib.extAddress;
        dstAddr.extAddress = assocResp->deviceAddress;

        FTDF_sendCommStatusIndication( req, FTDF_TRANSACTION_EXPIRED,
                                       FTDF_pib.PANId,
                                       FTDF_EXTENDED_ADDRESS,
                                       srcAddr,
                                       FTDF_EXTENDED_ADDRESS,
                                       dstAddr,
                                       assocResp->securityLevel,
                                       assocResp->keyIdMode,
                                       assocResp->keySource,
                                       assocResp->keyIndex );

        break;
    }
    case FTDF_DISASSOCIATE_REQUEST:
    {
        FTDF_DisassociateRequest* disReq = (FTDF_DisassociateRequest*)req;

        FTDF_sendDisassociateConfirm( disReq, FTDF_TRANSACTION_EXPIRED );

        break;
    }
    case FTDF_BEACON_REQUEST:
    {
        FTDF_BeaconRequest* beaconRequest = (FTDF_BeaconRequest*)req;

        FTDF_sendBeaconConfirm( beaconRequest, FTDF_TRANSACTION_EXPIRED );

        break;
    }
    }
}

#ifndef FTDF_NO_TSCH
void FTDF_resetKeepAliveTimer( FTDF_ShortAddress dstAddr )
{
    uint8_t n;

    for ( n = 0; n < FTDF_NR_OF_NEIGHBORS; n++ )
    {
        if ( FTDF_neighborTable[ n ].dstAddr == dstAddr )
        {
            break;
        }
    }

    if ( n == FTDF_NR_OF_NEIGHBORS )
    {
        return;
    }

    FTDF_removeTxPendingTimer( (FTDF_MsgBuffer*)&FTDF_neighborTable[ n ].msg );

    FTDF_Time tsTimeslotLength = (FTDF_Time) FTDF_pib.timeslotTemplate.tsTimeslotLength / 16;
    FTDF_Time delta            = tsTimeslotLength * FTDF_neighborTable[ n ].period;

    FTDF_addTxPendingTimer( (FTDF_MsgBuffer*)&FTDF_neighborTable[ n ].msg, n, delta, FTDF_processKeepAliveTimerExp );
}
#endif /* FTDF_NO_TSCH */
#endif /* !FTDF_LITE */

void FTDF_enableTransparentMode( FTDF_Boolean  enable,
                                 FTDF_Bitmap32 options )
{
#ifndef FTDF_LITE
    if ( FTDF_pib.leEnabled == FTDF_TRUE ||
         FTDF_pib.tschEnabled == FTDF_TRUE )
    {
        return;
    }
#endif /* !FTDF_LITE */

    FTDF_transparentMode        = enable;
    FTDF_transparentModeOptions = options;

    if ( enable )
    {
        FTDF_SET_FIELD( ON_OFF_REGMAP_MACALWAYSPASSFRMTYPE,
                        ( options & FTDF_TRANSPARENT_PASS_ALL_FRAME_TYPES ) );
        FTDF_SET_FIELD( ON_OFF_REGMAP_DISRXACKREQUESTCA, ( options & FTDF_TRANSPARENT_AUTO_ACK ? 0 : 1 ) );
        FTDF_SET_FIELD( ON_OFF_REGMAP_MACALWAYSPASSCRCERROR, ( options & FTDF_TRANSPARENT_PASS_CRC_ERROR ? 1 : 0 ) );
        FTDF_SET_FIELD( ON_OFF_REGMAP_MACALWAYSPASSRESFRAMEVERSION,
                        ( options & FTDF_TRANSPARENT_PASS_ALL_FRAME_VERSION ? 1 : 0 ) );
        FTDF_SET_FIELD( ON_OFF_REGMAP_MACALWAYSPASSWRONGDPANID,
                        ( options & FTDF_TRANSPARENT_PASS_ALL_PAN_ID ? 1 : 0 ) );
        FTDF_SET_FIELD( ON_OFF_REGMAP_MACALWAYSPASSWRONGDADDR, ( options & FTDF_TRANSPARENT_PASS_ALL_ADDR ? 1 : 0 ) );
        FTDF_SET_FIELD( ON_OFF_REGMAP_MACALWAYSPASSBEACONWRONGPANID,
                        ( options & FTDF_TRANSPARENT_PASS_ALL_BEACON ? 1 : 0 ) );
        FTDF_SET_FIELD( ON_OFF_REGMAP_MACALWAYSPASSTOPANCOORDINATOR,
                        ( options & FTDF_TRANSPARENT_PASS_ALL_NO_DEST_ADDR ? 1 : 0 ) );
    }
    else
    {
        FTDF_SET_FIELD( ON_OFF_REGMAP_MACALWAYSPASSFRMTYPE, 0 );
        FTDF_SET_FIELD( ON_OFF_REGMAP_DISRXACKREQUESTCA, 0 );
        FTDF_SET_FIELD( ON_OFF_REGMAP_MACALWAYSPASSCRCERROR, 0 );
        FTDF_SET_FIELD( ON_OFF_REGMAP_MACALWAYSPASSRESFRAMEVERSION, 0 );
        FTDF_SET_FIELD( ON_OFF_REGMAP_MACALWAYSPASSWRONGDPANID, 0 );
        FTDF_SET_FIELD( ON_OFF_REGMAP_MACALWAYSPASSWRONGDADDR, 0 );
        FTDF_SET_FIELD( ON_OFF_REGMAP_MACALWAYSPASSBEACONWRONGPANID, 0 );
        FTDF_SET_FIELD( ON_OFF_REGMAP_MACALWAYSPASSTOPANCOORDINATOR, 0 );
    }
}

#if FTDF_DBG_BUS_ENABLE
void FTDF_checkDbgMode(void)
{
        FTDF_SET_FIELD(ON_OFF_REGMAP_DBG_CONTROL, FTDF_dbgMode);
        if (FTDF_dbgMode) {
                FTDF_DBG_BUS_GPIO_CONFIG();
        }
}

void FTDF_setDbgMode(FTDF_DbgMode dbgMode)
{
        FTDF_dbgMode = dbgMode;
        FTDF_checkDbgMode();
}
#endif /* FTDF_DBG_BUS_ENABLE */

#ifndef FTDF_LITE
#ifndef FTDF_NO_CSL
void FTDF_setPeerCslTiming( FTDF_IEList* headerIEList, FTDF_Time timeStamp )
{
    if ( FTDF_reqCurrent->msgId != FTDF_DATA_REQUEST )
    {
        // Only use the CSL timing of data frame acks
        return;
    }

    FTDF_DataRequest* dataRequest = (FTDF_DataRequest*) FTDF_reqCurrent;
    FTDF_ShortAddress dstAddr     = dataRequest->dstAddr.shortAddress;

    if ( dataRequest->dstAddrMode != FTDF_SHORT_ADDRESS ||
         dstAddr == 0xffff )
    {
        return;
    }

    int n;

    // Search for an existing entry
    for ( n = 0; n < FTDF_NR_OF_CSL_PEERS; n++ )
    {
        if ( FTDF_peerCslTiming[ n ].addr == dstAddr )
        {
            break;
        }
    }

    if ( headerIEList == NULL )
    {
        if ( n < FTDF_NR_OF_CSL_PEERS )
        {
            // Delete entry
            FTDF_peerCslTiming[ n ].addr = 0xffff;
        }

        return;
    }

    int ieNr = 0;

    while ( ieNr < headerIEList->nrOfIEs &&
            headerIEList->IEs[ ieNr ].ID != 0x1a )
    {
        ieNr++;
    }

    if ( ieNr == headerIEList->nrOfIEs )
    {
        return;
    }

    if ( n == FTDF_NR_OF_CSL_PEERS )
    {
        // Search for an empty entry
        for ( n = 0; n < FTDF_NR_OF_CSL_PEERS; n++ )
        {
            if ( FTDF_peerCslTiming[ n ].addr == 0xffff )
            {
                break;
            }
        }
    }

    if ( n == FTDF_NR_OF_CSL_PEERS )
    {
        // No free entry, search for the least recently used entry
        FTDF_Time maxDelta = 0;
        int       lru      = 0;

        for ( n = 0; n < FTDF_NR_OF_CSL_PEERS; n++ )
        {
            FTDF_Time delta = timeStamp - FTDF_peerCslTiming[ n ].time;

            if ( delta > maxDelta )
            {
                maxDelta = delta;
                lru      = n;
            }
        }

        n = lru;
    }

    FTDF_Period phase  = ( *(FTDF_Period*) ( headerIEList->IEs[ 0 ].content.raw + 0 ) );
    FTDF_Period period = ( *(FTDF_Period*) ( headerIEList->IEs[ 0 ].content.raw + 2 ) );

    FTDF_peerCslTiming[ n ].addr   = dstAddr;
    FTDF_peerCslTiming[ n ].time   = timeStamp - ( phase * 10 );
    FTDF_peerCslTiming[ n ].period = period;
}

void FTDF_getWakeupParams( FTDF_ShortAddress dstAddr,
                           FTDF_Time*        wakeupStartTime,
                           FTDF_Period*      wakeupPeriod )
{
    int n;

    for ( n = 0; n < FTDF_NR_OF_CSL_PEERS; n++ )
    {
        if ( FTDF_peerCslTiming[ n ].addr == dstAddr )
        {
            break;
        }
    }

    FTDF_Time curTime = FTDF_GET_FIELD( ON_OFF_REGMAP_SYMBOLTIMESNAPSHOTVAL );

    if ( dstAddr == 0xffff ||
         n == FTDF_NR_OF_CSL_PEERS )
    {
        *wakeupStartTime = curTime + 5;
        *wakeupPeriod    = FTDF_pib.CSLMaxPeriod;

        return;
    }

    FTDF_Time peerTime   = FTDF_peerCslTiming[ n ].time;
    FTDF_Time peerPeriod = FTDF_peerCslTiming[ n ].period * 10;
    FTDF_Time delta      = curTime - peerTime;

    if ( delta > (uint32_t)( FTDF_pib.CSLMaxAgeRemoteInfo * 10 ) )
    {
        *wakeupStartTime = curTime + 5;
        *wakeupPeriod    = FTDF_pib.CSLMaxPeriod;

        return;
    }

    FTDF_Time wStart = peerTime + ( ( ( delta / peerPeriod ) + 1 ) * peerPeriod ) - FTDF_pib.CSLSyncTxMargin;
    delta = wStart - curTime;

    if ( delta < 3 || delta > 0x80000000 )  // A delta larger than 0x80000000 is assumed a negative delta
    {
        wStart += peerPeriod;
    }

    *wakeupPeriod    = ( FTDF_pib.CSLSyncTxMargin / 10 ) * 2;
    *wakeupStartTime = wStart;
}

void FTDF_setCslSampleTime( void )
{
    FTDF_Time CSLPeriod = FTDF_pib.CSLPeriod * 10;

    FTDF_criticalVar( );
    FTDF_enterCritical( );

    FTDF_Time curTime = FTDF_GET_FIELD( ON_OFF_REGMAP_SYMBOLTIMESNAPSHOTVAL );
    FTDF_Time delta   = ( curTime - FTDF_startCslSampleTime );

    // A delta larger than 0x80000000 is assumed a negative delta, in this case the sample time does
    // not need to be updated.
    if ( delta < 0x80000000 )
    {
        if ( delta < CSLPeriod )
        {
            FTDF_startCslSampleTime += CSLPeriod;

            if ( delta < 3 )
            {
                // To avoid to set the CSL sample time to a time stamp in the past set it to a sample period later
                // if the next sample would be within 3 symbols.
                FTDF_startCslSampleTime += CSLPeriod;
            }
        }
        else
        {
            FTDF_startCslSampleTime = FTDF_startCslSampleTime + ( ( delta / CSLPeriod ) + 1 ) * CSLPeriod;
        }

        FTDF_SET_FIELD( ON_OFF_REGMAP_MACCSLSTARTSAMPLETIME, FTDF_startCslSampleTime );
    }

    FTDF_exitCritical( );
}
#endif /* FTDF_NO_CSL */
#endif /* !FTDF_LITE */

FTDF_Time64 FTDF_getCurTime64( void )
{
    FTDF_Time newTime = FTDF_GET_FIELD( ON_OFF_REGMAP_SYMBOLTIMESNAPSHOTVAL );

    if ( newTime < FTDF_curTime[ 0 ] )
    {
        FTDF_curTime[ 1 ]++;
    }

    FTDF_curTime[ 0 ] = newTime;

    return *(FTDF_Time64*)FTDF_curTime;
}

void FTDF_initCurTime64( void )
{
    FTDF_curTime[ 0 ] = FTDF_GET_FIELD( ON_OFF_REGMAP_SYMBOLTIMESNAPSHOTVAL );
    FTDF_curTime[ 1 ] = 0;
}

void FTDF_getExtAddress( void )
{
    uint32_t* extAddress = (uint32_t*) &FTDF_pib.extAddress;
    extAddress[ 0 ] = FTDF_GET_FIELD( ON_OFF_REGMAP_AEXTENDEDADDRESS_L );
    extAddress[ 1 ] = FTDF_GET_FIELD( ON_OFF_REGMAP_AEXTENDEDADDRESS_H );
}

void FTDF_setExtAddress( void )
{
    uint32_t* extAddress = (uint32_t*) &FTDF_pib.extAddress;
    FTDF_SET_FIELD( ON_OFF_REGMAP_AEXTENDEDADDRESS_L, extAddress[ 0 ] );
    FTDF_SET_FIELD( ON_OFF_REGMAP_AEXTENDEDADDRESS_H, extAddress[ 1 ] );
}

void FTDF_getAckWaitDuration( void )
{
    FTDF_pib.ackWaitDuration = FTDF_GET_FIELD( ON_OFF_REGMAP_MACACKWAITDURATION );
}

#ifndef FTDF_LITE
void FTDF_getEnhAckWaitDuration( void )
{
    FTDF_pib.enhAckWaitDuration = FTDF_GET_FIELD( ON_OFF_REGMAP_MACENHACKWAITDURATION );
}

void FTDF_setEnhAckWaitDuration( void )
{
    FTDF_SET_FIELD( ON_OFF_REGMAP_MACENHACKWAITDURATION, FTDF_pib.enhAckWaitDuration );
}

void FTDF_getImplicitBroadcast( void )
{
    FTDF_pib.implicitBroadcast = FTDF_GET_FIELD( ON_OFF_REGMAP_MACIMPLICITBROADCAST );
}

void FTDF_setImplicitBroadcast( void )
{
    FTDF_SET_FIELD( ON_OFF_REGMAP_MACIMPLICITBROADCAST, FTDF_pib.implicitBroadcast );
}
#endif /* !FTDF_LITE */

void FTDF_setShortAddress( void )
{
    FTDF_SET_FIELD( ON_OFF_REGMAP_MACSHORTADDRESS, FTDF_pib.shortAddress );
}

#ifndef FTDF_LITE
void FTDF_setSimpleAddress( void )
{
    FTDF_SET_FIELD( ON_OFF_REGMAP_MACSIMPLEADDRESS, FTDF_pib.simpleAddress );
}
#endif /* !FTDF_LITE */

void FTDF_getRxOnWhenIdle( void )
{
    FTDF_pib.rxOnWhenIdle = FTDF_GET_FIELD( ON_OFF_REGMAP_RXALWAYSON );
}

void FTDF_setRxOnWhenIdle( void )
{
#if FTDF_USE_PTI && !FTDF_USE_AUTO_PTI
    /* We do not force decision here. It will be automatically made when FTDF begins 
     * transaction.
     */
    hw_coex_update_ftdf_pti((hw_coex_pti_t) FTDF_getRxPti(), NULL, false);
#endif /* FTDF_USE_PTI && !FTDF_USE_AUTO_PTI */
    FTDF_SET_FIELD( ON_OFF_REGMAP_RXENABLE, 0 );
    FTDF_SET_FIELD( ON_OFF_REGMAP_RXALWAYSON, FTDF_pib.rxOnWhenIdle );
    FTDF_SET_FIELD( ON_OFF_REGMAP_RXENABLE, 1 );

}

void FTDF_getPANId( void )
{
    FTDF_pib.PANId = FTDF_GET_FIELD( ON_OFF_REGMAP_MACPANID );
}

void FTDF_setPANId( void )
{
    FTDF_SET_FIELD( ON_OFF_REGMAP_MACPANID, FTDF_pib.PANId );
}

void FTDF_getCurrentChannel( void )
{
    FTDF_pib.currentChannel = ( ( FTDF_GET_FIELD( ON_OFF_REGMAP_PHYRXATTR ) & 0x00f0 ) >> 4 ) + 11;
}

void FTDF_setCurrentChannel( void )
{
    uint32_t phyAckAttr = 0x08 | ( ( FTDF_pib.currentChannel - 11 ) & 0xf ) << 4 | ( FTDF_pib.TXPower & 0x7 ) << 12;

    FTDF_SET_FIELD( ON_OFF_REGMAP_PHYRXATTR, ( ( ( FTDF_pib.currentChannel - 11 ) & 0xf ) << 4 ) );
    FTDF_SET_FIELD( ON_OFF_REGMAP_PHYACKATTR, phyAckAttr );
}

void FTDF_setTXPower( void )
{
    /* Just like setCurrentChannel, this sets pyAckAttr */
    uint32_t phyAckAttr = 0x08 | ( ( FTDF_pib.currentChannel - 11 ) & 0xf ) << 4 | ( FTDF_pib.TXPower & 0x7 ) << 12;

    FTDF_SET_FIELD( ON_OFF_REGMAP_PHYACKATTR, phyAckAttr );
}

void FTDF_getMaxFrameTotalWaitTime( void )
{
    FTDF_pib.maxFrameTotalWaitTime = FTDF_GET_FIELD( ON_OFF_REGMAP_MACMAXFRAMETOTALWAITTIME );
}

void FTDF_setMaxFrameTotalWaitTime( void )
{
    FTDF_SET_FIELD( ON_OFF_REGMAP_MACMAXFRAMETOTALWAITTIME, FTDF_pib.maxFrameTotalWaitTime );
}

void FTDF_setMaxCSMABackoffs( void )
{
#ifndef FTDF_LITE
    if ( FTDF_pib.leEnabled == FTDF_FALSE && FTDF_pib.tschEnabled == FTDF_FALSE )
#endif /* !FTDF_LITE */
    {
        FTDF_SET_FIELD( ON_OFF_REGMAP_MACMAXCSMABACKOFFS, FTDF_pib.maxCSMABackoffs );
    }
}

void FTDF_setMaxBE( void )
{
    FTDF_SET_FIELD( ON_OFF_REGMAP_MACMAXBE, FTDF_pib.maxBE );
}

void FTDF_setMinBE( void )
{
#ifndef FTDF_LITE
    if ( FTDF_pib.leEnabled == FTDF_FALSE && FTDF_pib.tschEnabled == FTDF_FALSE )
#endif /* !FTDF_LITE */
    {
        FTDF_SET_FIELD( ON_OFF_REGMAP_MACMINBE, FTDF_pib.minBE );
    }
}

#ifndef FTDF_LITE
#ifndef FTDF_NO_CSL
void FTDF_setLeEnabled( void )
{
    FTDF_SET_FIELD( ON_OFF_REGMAP_MACCSLSAMPLEPERIOD, 66 );
    FTDF_SET_FIELD( ON_OFF_REGMAP_MACCSLDATAPERIOD, 66 );
    FTDF_SET_FIELD( ON_OFF_REGMAP_MACCSLMARGINRZ, 1 );

    if ( FTDF_pib.leEnabled )
    {
        FTDF_SET_FIELD( ON_OFF_REGMAP_MACMAXCSMABACKOFFS, 0 );
        FTDF_SET_FIELD( ON_OFF_REGMAP_MACMINBE, 0 );

        if ( FTDF_oldLeEnabled == FTDF_FALSE )
        {
            if ( FTDF_wakeUpEnableLe == FTDF_FALSE )
            {
                FTDF_startCslSampleTime = FTDF_GET_FIELD( ON_OFF_REGMAP_SYMBOLTIMESNAPSHOTVAL );
                FTDF_setCslSampleTime( );
            }
            else
            {
                FTDF_SET_FIELD( ON_OFF_REGMAP_MACCSLSTARTSAMPLETIME, FTDF_startCslSampleTime );
            }
        }
    }
    else
    {
        FTDF_SET_FIELD( ON_OFF_REGMAP_MACMAXCSMABACKOFFS, FTDF_pib.maxCSMABackoffs );
        FTDF_SET_FIELD( ON_OFF_REGMAP_MACMINBE, FTDF_pib.minBE );
    }

    FTDF_SET_FIELD( ON_OFF_REGMAP_MACLEENABLED, FTDF_pib.leEnabled );

    FTDF_oldLeEnabled = FTDF_pib.leEnabled;
}

void FTDF_getCslFramePendingWaitT( void )
{
    FTDF_pib.CSLFramePendingWaitT = FTDF_GET_FIELD( ON_OFF_REGMAP_MACCSLFRAMEPENDINGWAITT );
}

void FTDF_setCslFramePendingWaitT( void )
{
    FTDF_SET_FIELD( ON_OFF_REGMAP_MACCSLFRAMEPENDINGWAITT, FTDF_pib.CSLFramePendingWaitT );
}
#endif /* FTDF_NO_CSL */
#endif /* !FTDF_LITE */

void FTDF_getLmacPmData( void )
{
    FTDF_pib.performanceMetrics.FCSErrorCount = FTDF_GET_FIELD( ON_OFF_REGMAP_MACFCSERRORCOUNT ) +
                                                FTDF_lmacCounters.fcsErrorCnt;
}

void FTDF_getLmacTrafficCounters( void )
{
    FTDF_pib.trafficCounters.txStdAckFrmCnt   = FTDF_GET_FIELD( ON_OFF_REGMAP_MACTXSTDACKFRMCNT ) +
                                                FTDF_lmacCounters.txStdAckCnt;
    FTDF_pib.trafficCounters.rxStdAckFrmOkCnt = FTDF_GET_FIELD( ON_OFF_REGMAP_MACRXSTDACKFRMOKCNT ) +
                                                FTDF_lmacCounters.rxStdAckCnt;
}

void FTDF_getKeepPhyEnabled( void )
{
    FTDF_pib.keepPhyEnabled = FTDF_GET_FIELD( ON_OFF_REGMAP_KEEP_PHY_EN );
}

void FTDF_setKeepPhyEnabled( void )
{
    FTDF_SET_FIELD( ON_OFF_REGMAP_KEEP_PHY_EN, FTDF_pib.keepPhyEnabled );
}

#if dg_configBLACK_ORCA_IC_REV != BLACK_ORCA_IC_REV_A
void FTDF_setBoIrqThreshold( void )
{
        FTDF_SET_FIELD( ON_OFF_REGMAP_CSMA_CA_BO_THRESHOLD, FTDF_pib.boIrqThreshold );
}
void FTDF_getBoIrqThreshold( void )
{
        FTDF_pib.boIrqThreshold = FTDF_GET_FIELD( ON_OFF_REGMAP_CSMA_CA_BO_THRESHOLD );
}
void FTDF_setPtiConfig( void )
{
        FTDF_SET_FIELD_INDEXED(ON_OFF_REGMAP_PTI_TX, FTDF_pib.ptiConfig.ptis[FTDF_PTI_CONFIG_TX],
                FTDF_TX_DATA_BUFFER);
        FTDF_SET_FIELD_INDEXED(ON_OFF_REGMAP_PTI_TX, FTDF_pib.ptiConfig.ptis[FTDF_PTI_CONFIG_TX],
                FTDF_TX_WAKEUP_BUFFER);
        FTDF_SET_FIELD_INDEXED(ON_OFF_REGMAP_PTI_TX, FTDF_pib.ptiConfig.ptis[FTDF_PTI_CONFIG_RX],
                FTDF_TX_ACK_BUFFER);
        FTDF_SET_FIELD(ON_OFF_REGMAP_PTI_RX, FTDF_pib.ptiConfig.ptis[FTDF_PTI_CONFIG_RX]);
}
#endif /* #if dg_configBLACK_ORCA_IC_REV != BLACK_ORCA_IC_REV_A */

#ifndef FTDF_LITE
#ifndef FTDF_NO_TSCH
void FTDF_setTimeslotTemplate( void )
{
    FTDF_SET_FIELD( ON_OFF_REGMAP_MACTSTXACKDELAY, FTDF_pib.timeslotTemplate.tsTxAckDelay );
    FTDF_SET_FIELD( ON_OFF_REGMAP_MACTSRXWAIT, FTDF_pib.timeslotTemplate.tsRxWait );
    FTDF_SET_FIELD( ON_OFF_REGMAP_MACTSRXACKDELAY, FTDF_pib.timeslotTemplate.tsRxAckDelay );
    FTDF_SET_FIELD( ON_OFF_REGMAP_MACTSACKWAIT, FTDF_pib.timeslotTemplate.tsAckWait );
    FTDF_SET_FIELD( ON_OFF_REGMAP_MACTSRXTX, FTDF_pib.timeslotTemplate.tsRxTx -
                    FTDF_PHYTRXWAIT - FTDF_PHYTXSTARTUP - FTDF_PHYTXLATENCY );
}
#endif /* FTDF_NO_TSCH */
#endif /* !FTDF_LITE */

#if dg_configBLACK_ORCA_IC_REV != BLACK_ORCA_IC_REV_A
#if FTDF_FP_BIT_MODE == FTDF_FP_BIT_MODE_AUTO
/************************************ FPPR common functions ***************************************/
#ifndef FTDF_LITE

FTDF_Boolean FTDF_fpFsmShortAddressNew(FTDF_PANId panId, FTDF_ShortAddress shortAddress)
{
        uint8_t entry;
        uint8_t shortAddrIdx;

        if (FTDF_fpprGetFreeShortAddress(&entry, &shortAddrIdx) == FTDF_FALSE) {
                return FTDF_FALSE;
        }

        FTDF_fpprSetShortAddress(entry, shortAddrIdx, shortAddress);
        FTDF_fpprSetShortAddressValid(entry, shortAddrIdx, FTDF_TRUE);

        return FTDF_TRUE;
}

FTDF_Boolean FTDF_fpFsmExtAddressNew(FTDF_PANId panId, FTDF_ExtAddress extAddress)
{
        uint8_t entry;

        if (FTDF_fpprGetFreeExtAddress(&entry) == FTDF_FALSE) {
                return FTDF_FALSE;
        }

        FTDF_fpprSetExtAddress(entry, extAddress);
        FTDF_fpprSetExtAddressValid(entry, FTDF_TRUE);

        return FTDF_TRUE;
}

void FTDF_fpFsmShortAddressLastFramePending(FTDF_PANId PANId, FTDF_ShortAddress shortAddress) {
#if FTDF_FPPR_DEFER_INVALIDATION
        FTDF_fpprPending.addrMode = FTDF_SHORT_ADDRESS;
        FTDF_fpprPending.PANId = PANId;
        FTDF_fpprPending.addr.shortAddress = shortAddress;
#else /* FTDF_FPPR_DEFER_INVALIDATION */
        uint8_t entry;
        uint8_t shortAddrIdx;
        FTDF_Boolean found = FTDF_fpprLookupShortAddress(shortAddress, &entry, &shortAddrIdx);
        ASSERT_WARNING(found);
        FTDF_fpprSetShortAddressValid(entry, shortAddrIdx, FTDF_FALSE);
#endif /* FTDF_FPPR_DEFER_INVALIDATION */
}

void FTDF_fpFsmExtAddressLastFramePending(FTDF_PANId PANId, FTDF_ExtAddress extAddress) {
#if FTDF_FPPR_DEFER_INVALIDATION
        FTDF_fpprPending.addrMode = FTDF_EXTENDED_ADDRESS;
        FTDF_fpprPending.PANId = PANId;
        FTDF_fpprPending.addr.extAddress = extAddress;
#else /* FTDF_FPPR_DEFER_INVALIDATION */
        uint8_t entry;
        FTDF_Boolean found = FTDF_fpprLookupExtAddress(extAddress, &entry);
        ASSERT_WARNING(found);
        FTDF_fpprSetExtAddressValid(entry, FTDF_FALSE);
#endif /* FTDF_FPPR_DEFER_INVALIDATION */
}

void FTDF_fpFsmClearPending(void)
{
#if FTDF_FPPR_DEFER_INVALIDATION
        int n;
        if (FTDF_fpprPending.addrMode == FTDF_NO_ADDRESS) {
                return;
        }
        if ( FTDF_fpprPending.addrMode == FTDF_SHORT_ADDRESS ) {
                for (n = 0; n < FTDF_NR_OF_REQ_BUFFERS; n++) {
                        if (FTDF_txPendingList[ n ].addrMode == FTDF_SHORT_ADDRESS) {
                                if ((FTDF_txPendingList[ n ].PANId == FTDF_fpprPending.PANId) &&
                                        (FTDF_txPendingList[ n ].addr.shortAddress ==
                                                FTDF_fpprPending.addr.shortAddress)) {
                                        return;
                                }
                        }
                }
                // Address not found.
                uint8_t entry;
                uint8_t shortAddrIdx;
                FTDF_Boolean found = FTDF_fpprLookupShortAddress(
                        FTDF_fpprPending.addr.shortAddress, &entry, &shortAddrIdx);
                ASSERT_WARNING(found);
                FTDF_fpprSetShortAddressValid(entry, shortAddrIdx, FTDF_FALSE);
        } else if ( FTDF_fpprPending.addrMode == FTDF_EXTENDED_ADDRESS ) {
                for (n = 0; n < FTDF_NR_OF_REQ_BUFFERS; n++) {
                        if (FTDF_txPendingList[ n ].addrMode == FTDF_EXTENDED_ADDRESS) {
                                if (FTDF_txPendingList[ n ].addr.extAddress ==
                                        FTDF_fpprPending.addr.extAddress) {
                                        return;
                                }
                        }
                }
                // Address not found.
                uint8_t entry;
                FTDF_Boolean found = FTDF_fpprLookupExtAddress(FTDF_fpprPending.addr.extAddress,
                        &entry);
                ASSERT_WARNING(found);
                FTDF_fpprSetExtAddressValid(entry, FTDF_FALSE);
        } else {

        }
        FTDF_fpprPending.addrMode = FTDF_NO_ADDRESS;
#endif /* FTDF_FPPR_DEFER_INVALIDATION */
}

#endif /* #ifndef FTDF_LITE */
/*********************************** FPPR low-level access ****************************************/
void FTDF_fpprReset(void)
{
        unsigned int i;
        for (i = 0; i < FTDF_FPPR_TABLE_ENTRIES; i++) {
                *FTDF_GET_REG_ADDR_INDEXED(FP_PROCESSING_RAM_SIZE_AND_VAL, i) = 0;
        }
}

FTDF_ShortAddress FTDF_fpprGetShortAddress(uint8_t entry, uint8_t shortAddrIdx)
{
        ASSERT_WARNING(entry < FTDF_FPPR_TABLE_ENTRIES);
        switch (shortAddrIdx)
        {
        case 0:
                return (FTDF_ShortAddress)
                        *FTDF_GET_FIELD_ADDR_INDEXED(FP_PROCESSING_RAM_EXP_SA_L, entry) &
                        0x0000ffff;
        case 1:
                return (FTDF_ShortAddress)
                        (*FTDF_GET_FIELD_ADDR_INDEXED(FP_PROCESSING_RAM_EXP_SA_L, entry) >> 16) &
                        0x0000ffff;
        case 2:
                return (FTDF_ShortAddress)
                        *FTDF_GET_FIELD_ADDR_INDEXED(FP_PROCESSING_RAM_EXP_SA_H, entry) &
                        0x0000ffff;
        case 3:
                return (FTDF_ShortAddress)
                        (*FTDF_GET_FIELD_ADDR_INDEXED(FP_PROCESSING_RAM_EXP_SA_H, entry) >> 16) &
                        0x0000ffff;
        default:
                ASSERT_WARNING(0);
        }
}

void FTDF_fpprSetShortAddress(uint8_t entry, uint8_t shortAddrIdx,
        FTDF_ShortAddress shortAddress)
{
        ASSERT_WARNING(entry < FTDF_FPPR_TABLE_ENTRIES);
        uint32_t val32;
        switch (shortAddrIdx)
        {
        case 0:
                val32 = *FTDF_GET_FIELD_ADDR_INDEXED(FP_PROCESSING_RAM_EXP_SA_L, entry);
                val32 &= 0xffff0000;
                val32 |= (shortAddress & 0x0000ffff);
                *FTDF_GET_FIELD_ADDR_INDEXED(FP_PROCESSING_RAM_EXP_SA_L, entry) = val32;
                break;
        case 1:
                val32 = *FTDF_GET_FIELD_ADDR_INDEXED(FP_PROCESSING_RAM_EXP_SA_L, entry);
                val32 &= 0x0000ffff;
                val32 |= (shortAddress & 0x0000ffff) << 16;
                *FTDF_GET_FIELD_ADDR_INDEXED(FP_PROCESSING_RAM_EXP_SA_L, entry) = val32;
                break;
        case 2:
                val32 = *FTDF_GET_FIELD_ADDR_INDEXED(FP_PROCESSING_RAM_EXP_SA_H, entry);
                val32 &= 0xffff0000;
                val32 |= (shortAddress & 0x0000ffff);
                *FTDF_GET_FIELD_ADDR_INDEXED(FP_PROCESSING_RAM_EXP_SA_H, entry) = val32;
                break;
        case 3:
                val32 = *FTDF_GET_FIELD_ADDR_INDEXED(FP_PROCESSING_RAM_EXP_SA_H, entry);
                val32 &= 0x0000ffff;
                val32 |= (shortAddress & 0x0000ffff) << 16;
                *FTDF_GET_FIELD_ADDR_INDEXED(FP_PROCESSING_RAM_EXP_SA_H, entry) = val32;
                break;
        default:
                ASSERT_WARNING(0);
        }
}

FTDF_Boolean FTDF_fpprGetShortAddressValid(uint8_t entry, uint8_t shortAddrIdx)
{
        ASSERT_WARNING(entry < FTDF_FPPR_TABLE_ENTRIES);
        ASSERT_WARNING(shortAddrIdx < 4);
        uint32_t val32;
        val32 = *FTDF_GET_FIELD_ADDR_INDEXED(FP_PROCESSING_RAM_VALID_SA, entry);
        return ((val32 & (MSK_F_FTDF_FP_PROCESSING_RAM_SHORT_LONGNOT | (1 << shortAddrIdx))) ==
                (MSK_F_FTDF_FP_PROCESSING_RAM_SHORT_LONGNOT | (1 << shortAddrIdx))) ?
                FTDF_TRUE : FTDF_FALSE;
}

void FTDF_fpprSetShortAddressValid(uint8_t entry, uint8_t shortAddrIdx,
        FTDF_Boolean valid)
{
        ASSERT_WARNING(entry < FTDF_FPPR_TABLE_ENTRIES);
        ASSERT_WARNING(shortAddrIdx < 4);
        uint32_t val32;
        val32 = *FTDF_GET_FIELD_ADDR_INDEXED(FP_PROCESSING_RAM_VALID_SA, entry);
        if (valid) {
                val32 |= MSK_F_FTDF_FP_PROCESSING_RAM_SHORT_LONGNOT | (1 << shortAddrIdx); /* Also indicate short address. */
        } else {
                val32 &= ~(1 << shortAddrIdx);
        }
        *FTDF_GET_FIELD_ADDR_INDEXED(FP_PROCESSING_RAM_VALID_SA, entry) = val32;
}

FTDF_ExtAddress FTDF_fpprGetExtAddress(uint8_t entry)
{
        FTDF_ExtAddress extAddress;
        ASSERT_WARNING(entry < FTDF_FPPR_TABLE_ENTRIES);
        extAddress = (FTDF_ExtAddress) *FTDF_GET_FIELD_ADDR_INDEXED(FP_PROCESSING_RAM_EXP_SA_H,
                entry) << 32;
        extAddress |= (FTDF_ExtAddress) *FTDF_GET_FIELD_ADDR_INDEXED(FP_PROCESSING_RAM_EXP_SA_L,
                entry);
        return extAddress;
}

void FTDF_fpprSetExtAddress(uint8_t entry, FTDF_ExtAddress extAddress)
{
        ASSERT_WARNING(entry < FTDF_FPPR_TABLE_ENTRIES);
        *FTDF_GET_FIELD_ADDR_INDEXED(FP_PROCESSING_RAM_EXP_SA_L, entry) =
                (uint32_t) (extAddress);
        *FTDF_GET_FIELD_ADDR_INDEXED(FP_PROCESSING_RAM_EXP_SA_H, entry) =
                (uint32_t) ((extAddress >> 32));
}

FTDF_Boolean FTDF_fpprGetExtAddressValid(uint8_t entry)
{
        return (*FTDF_GET_FIELD_ADDR_INDEXED(FP_PROCESSING_RAM_VALID_SA, entry) == 0x1) ?
                FTDF_TRUE : FTDF_FALSE;
}

void FTDF_fpprSetExtAddressValid(uint8_t entry, FTDF_Boolean valid)
{
        ASSERT_WARNING(entry < FTDF_FPPR_TABLE_ENTRIES);
        if (valid) {
                /* Also indicate ext address. */
                *FTDF_GET_FIELD_ADDR_INDEXED(FP_PROCESSING_RAM_VALID_SA, entry) = 0x1;
        } else {
                *FTDF_GET_FIELD_ADDR_INDEXED(FP_PROCESSING_RAM_VALID_SA, entry) = 0x0;
        }
}

FTDF_Boolean FTDF_fpprGetFreeShortAddress(uint8_t * entry, uint8_t * shortAddrIdx)
{
        int i, j;
        uint32_t sizeAndVal;
        int emptyEntry;
        bool emptyFound = false, nonEmptyFound = false;
        for (i = 0; i < FTDF_FPPR_TABLE_ENTRIES; i++) {
                sizeAndVal = *FTDF_GET_REG_ADDR_INDEXED(FP_PROCESSING_RAM_SIZE_AND_VAL, i);
                if (sizeAndVal == 0x1) {
                        /* Check if there is a valid extended address.  */
                        continue;
                } else if ((sizeAndVal & MSK_F_FTDF_FP_PROCESSING_RAM_SHORT_LONGNOT) == 0) {
                        /* There is an invalid extended address, ignore SA valid bits  */
                        sizeAndVal = 0;
                } else {
                        /* There is a SA. We are interested in bits V0 - V3. */
                        sizeAndVal &= 0xf;
                }

                /* Check if entire entry is free. */
                if (sizeAndVal == 0) {
                        /* We prefer to use partially full entries. Make note of this and
                         * continue. */
                        if (!emptyFound) {
                                emptyEntry = i;
                                emptyFound = true;
                        }
                        continue;
                }
                /* Check for free short address entries. */
                sizeAndVal = (~sizeAndVal) & 0xf;
                j = 0;
                while (sizeAndVal) {
                        if (sizeAndVal & 0x1) {
                                nonEmptyFound = true;
                                break;
                        }
                        sizeAndVal >>= 1;
                        j++;
                }
                if (nonEmptyFound) {
                        break;
                }
        }
        if (nonEmptyFound) {
                *entry = i;
                *shortAddrIdx = j;
        } else if (emptyFound) {
                *entry = emptyEntry;
                *shortAddrIdx = 0;
        } else {
                return FTDF_FALSE;
        }
        return FTDF_TRUE;
}

FTDF_Boolean FTDF_fpprGetFreeExtAddress(uint8_t * entry)
{
        int i;
        uint32_t sizeAndVal;
        bool found = false;
        for (i = 0; i < FTDF_FPPR_TABLE_ENTRIES; i++) {
                sizeAndVal = *FTDF_GET_REG_ADDR_INDEXED(FP_PROCESSING_RAM_SIZE_AND_VAL, i);
                /* Check if there is no valid extended or short address.  */
                if (!sizeAndVal || (sizeAndVal == MSK_F_FTDF_FP_PROCESSING_RAM_SHORT_LONGNOT)) {
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

FTDF_Boolean FTDF_fpprLookupShortAddress(FTDF_ShortAddress shortAddr, uint8_t * entry,
        uint8_t * shortAddrIdx)
{
        uint8_t i;
        uint32_t sizeAndVal;
        uint32_t saPart;
        for (i = 0; i < FTDF_FPPR_TABLE_ENTRIES; i++) {
                sizeAndVal = *FTDF_GET_REG_ADDR_INDEXED(FP_PROCESSING_RAM_SIZE_AND_VAL, i);
                /* Check if there is a valid short address. */
                if (!(sizeAndVal & MSK_F_FTDF_FP_PROCESSING_RAM_SHORT_LONGNOT) ||
                        !(sizeAndVal & MSK_F_FTDF_FP_PROCESSING_RAM_VALID_SA)) {
                        continue;
                }
                saPart = FTDF_GET_FIELD_INDEXED(FP_PROCESSING_RAM_EXP_SA_L, i);
                if (sizeAndVal & 0x1) {
                        if (shortAddr == (FTDF_ShortAddress) (saPart & 0x0000ffff)) {
                                *entry = i;
                                *shortAddrIdx = 0;
                                return FTDF_TRUE;
                        }
                }
                if (sizeAndVal & 0x2) {
                        if (shortAddr == (FTDF_ShortAddress) ((saPart >> 16) & 0x0000ffff)) {
                                *entry = i;
                                *shortAddrIdx = 1;
                                return FTDF_TRUE;
                        }
                }
                saPart = FTDF_GET_FIELD_INDEXED(FP_PROCESSING_RAM_EXP_SA_H, i);
                if (sizeAndVal & 0x4) {
                        if (shortAddr == (FTDF_ShortAddress) (saPart & 0x0000ffff)) {
                                *entry = i;
                                *shortAddrIdx = 2;
                                return FTDF_TRUE;
                        }
                }
                if (sizeAndVal & 0x8) {
                        if (shortAddr == ((FTDF_ShortAddress) (saPart >> 16) & 0x0000ffff)) {
                                *entry = i;
                                *shortAddrIdx = 3;
                                return FTDF_TRUE;
                        }
                }
        }
        return FTDF_FALSE;
}

FTDF_Boolean FTDF_fpprLookupExtAddress(FTDF_ExtAddress extAddr, uint8_t * entry)
{
        uint8_t i;
        uint32_t sizeAndVal;
        uint32_t extAddrHi, extAddrLo;
        extAddrHi = (uint32_t) ((extAddr >> 32) & 0xffffffff);
        extAddrLo = (uint32_t) (extAddr & 0xffffffff);
        for (i = 0; i < FTDF_FPPR_TABLE_ENTRIES; i++) {
                sizeAndVal = *FTDF_GET_REG_ADDR_INDEXED(FP_PROCESSING_RAM_SIZE_AND_VAL, i);
                /* Check if there is a valid extended address. */
                if (sizeAndVal != 0x1) {
                        continue;
                }
                if ((extAddrLo == FTDF_GET_FIELD_INDEXED(FP_PROCESSING_RAM_EXP_SA_L, i)) &&
                        (extAddrHi == FTDF_GET_FIELD_INDEXED(FP_PROCESSING_RAM_EXP_SA_H, i))) {
                        *entry = i;
                        return FTDF_TRUE;
                }
        }
        return FTDF_FALSE;
}

#endif /* FTDF_FP_BIT_MODE == FTDF_FP_BIT_MODE_AUTO */

void FTDF_fpprSetMode(FTDF_Boolean matchFp, FTDF_Boolean fpOverride, FTDF_Boolean fpForce)
{
        FTDF_SET_FIELD(ON_OFF_REGMAP_ADDR_TAB_MATCH_FP_VALUE, matchFp ? 1 : 0);
        FTDF_SET_FIELD(ON_OFF_REGMAP_FP_OVERRIDE, fpOverride ? 1 : 0);
        FTDF_SET_FIELD(ON_OFF_REGMAP_FP_FORCE_VALUE, fpForce ? 1 : 0);
}

#if FTDF_FP_BIT_TEST_MODE
void FTDF_fpprGetMode(FTDF_Boolean * matchFp, FTDF_Boolean * fpOverride, FTDF_Boolean * fpForce)
{
        *matchFp = FTDF_GET_FIELD(ON_OFF_REGMAP_ADDR_TAB_MATCH_FP_VALUE) ? FTDF_TRUE : FTDF_FALSE;
        *fpOverride = FTDF_GET_FIELD(ON_OFF_REGMAP_FP_OVERRIDE) ? FTDF_TRUE : FTDF_FALSE;
        *fpForce = FTDF_GET_FIELD(ON_OFF_REGMAP_FP_FORCE_VALUE) ? FTDF_TRUE : FTDF_FALSE;
}
#endif // FTDF_FP_BIT_TEST_MODE

void FTDF_lpdpEnable(FTDF_Boolean enable)
{
        FTDF_SET_FIELD(ON_OFF_REGMAP_FTDF_LPDP_ENABLE, enable ? 1 : 0);
}

#if FTDF_FP_BIT_TEST_MODE
FTDF_Boolean FTDF_lpdpIsEnabled(void)
{
        return FTDF_GET_FIELD(ON_OFF_REGMAP_FTDF_LPDP_ENABLE) ? FTDF_TRUE : FTDF_FALSE;
}
#endif

#if FTDF_USE_SLEEP_DURING_BACKOFF

static inline void FTDF_sdbSaveState(void)
{
        volatile uint32_t * txFifoPtr = FTDF_GET_REG_ADDR( RETENTION_RAM_TX_FIFO);
        uint32_t * dstPtr = (uint32_t *) FTDF_sdb.buffer;
        uint8_t word_length_rem;

        FTDF_sdb.nrOfBackoffs = FTDF_GET_FIELD(ON_OFF_REGMAP_CSMA_CA_NB_STAT);

        /* Read first 4 bytes. */
        *dstPtr++ = *txFifoPtr++;

        ASSERT_WARNING((FTDF_sdb.buffer[0] >= 3) && (FTDF_sdb.buffer[0] < FTDF_BUFFER_LENGTH));
        /* The length is the buffer length excluding the length byte itself */
        word_length_rem = (FTDF_sdb.buffer[0] + 4) / 4 - 1; /* 1 word we already read */

        while (word_length_rem--) {
                *dstPtr++ = *txFifoPtr++;
        }

        FTDF_sdb.metadata0 = *FTDF_GET_REG_ADDR_INDEXED( RETENTION_RAM_TX_META_DATA_0,
                FTDF_TX_DATA_BUFFER );
        FTDF_sdb.metadata1 = *FTDF_GET_REG_ADDR_INDEXED( RETENTION_RAM_TX_META_DATA_1,
                FTDF_TX_DATA_BUFFER );
        FTDF_sdb.phyCsmaCaAttr = FTDF_GET_FIELD( ON_OFF_REGMAP_PHYCSMACAATTR );
}

static inline void FTDF_sdbResume(void)
{
        volatile uint32_t * txFifoPtr = FTDF_GET_REG_ADDR( RETENTION_RAM_TX_FIFO);
        volatile uint32_t* txFlagSet = FTDF_GET_FIELD_ADDR( ON_OFF_REGMAP_TX_FLAG_SET);
        uint32_t * srcPtr = (uint32_t *) FTDF_sdb.buffer;

        uint8_t word_length_rem;

        FTDF_SET_FIELD(ON_OFF_REGMAP_CSMA_CA_NB_VAL, FTDF_sdb.nrOfBackoffs);

        FTDF_SET_FIELD(ON_OFF_REGMAP_CSMA_CA_RESUME_SET, 1);

        ASSERT_WARNING((FTDF_sdb.buffer[0] >= 3) && (FTDF_sdb.buffer[0] < FTDF_BUFFER_LENGTH));

        /* The length is the buffer length excluding the length byte itself */
        word_length_rem = (FTDF_sdb.buffer[0] + 4) / 4;

        while (word_length_rem--) {
                *txFifoPtr++ = *srcPtr++;
        }

        FTDF_SET_FIELD(ON_OFF_REGMAP_PHYCSMACAATTR, FTDF_sdb.phyCsmaCaAttr);

        *FTDF_GET_REG_ADDR(RETENTION_RAM_TX_META_DATA_0) = FTDF_sdb.metadata0;

        *FTDF_GET_REG_ADDR(RETENTION_RAM_TX_META_DATA_1) = FTDF_sdb.metadata1;

        *txFlagSet |= ( 1 << FTDF_TX_DATA_BUFFER );
}

static inline void FTDF_sdbReset(void)
{
        FTDF_SET_FIELD(ON_OFF_REGMAP_CSMA_CA_RESUME_CLEAR, 1);
}

static inline void FTDF_sdbSetCCARetryTime(void)
{
        FTDF_Time timestamp = FTDF_GET_FIELD( ON_OFF_REGMAP_SYMBOLTIMESNAPSHOTVAL );
        FTDF_Time boStat = FTDF_GET_FIELD(ON_OFF_REGMAP_CSMA_CA_BO_STAT) *
                FTDF_UNIT_BACKOFF_PERIOD;
        FTDF_sdb.ccaRetryTime = timestamp + boStat;
}

void FTDF_sdbFsmReset(void) {
        FTDF_sdbReset();
        FTDF_sdb.state = FTDF_SDB_STATE_INIT;
}

void FTDF_sdbFsmBackoffIRQ(void)
{
#if !defined(FTDF_NO_CSL) || !defined(FTDF_NO_TSCH)
        if (FTDF_pib.leEnabled || FTDF_pib.tschEnabled) {
                return;
        }
#endif
        switch (FTDF_sdb.state)
        {
        case FTDF_SDB_STATE_RESUMING:
                FTDF_sdbReset();
        case FTDF_SDB_STATE_INIT:
        case FTDF_SDB_STATE_BACKING_OFF:
                FTDF_sdbSetCCARetryTime();
                FTDF_sdb.state = FTDF_SDB_STATE_BACKING_OFF;
                break;
        default:
                ASSERT_WARNING(0);
        }
}

void FTDF_sdbFsmSleep(void)
{
#if !defined(FTDF_NO_CSL) || !defined(FTDF_NO_TSCH)
        if (FTDF_pib.leEnabled || FTDF_pib.tschEnabled) {
                return;
        }
#endif
        switch (FTDF_sdb.state)
        {
        case FTDF_SDB_STATE_BACKING_OFF:
                FTDF_sdbSaveState();
                FTDF_sdb.state = FTDF_SDB_STATE_WAITING_WAKE_UP_IRQ;
                break;
        case FTDF_SDB_STATE_INIT:
                break;
        default:
                ASSERT_WARNING(0);
        }
}

void FTDF_sdbFsmAbortSleep(void)
{
#if !defined(FTDF_NO_CSL) || !defined(FTDF_NO_TSCH)
        if (FTDF_pib.leEnabled || FTDF_pib.tschEnabled) {
                return;
        }
#endif
        switch (FTDF_sdb.state)
        {
        case FTDF_SDB_STATE_BACKING_OFF:
                FTDF_sdb.state = FTDF_SDB_STATE_INIT;
                break;
        case FTDF_SDB_STATE_INIT:
        case FTDF_SDB_STATE_WAITING_WAKE_UP_IRQ:
        case FTDF_SDB_STATE_RESUMING:
                break;
        default:
                ASSERT_WARNING(0);
        }
}

void FTDF_sdbFsmWakeUpIRQ(void)
{
#if !defined(FTDF_NO_CSL) || !defined(FTDF_NO_TSCH)
        if (FTDF_pib.leEnabled || FTDF_pib.tschEnabled) {
                return;
        }
#endif
        switch (FTDF_sdb.state)
        {
        case FTDF_SDB_STATE_WAITING_WAKE_UP_IRQ:
                FTDF_sdb.state = FTDF_SDB_STATE_RESUMING;
                break;
        case FTDF_SDB_STATE_INIT:
                break;
        default:
                ASSERT_WARNING(0);
        }
}

void FTDF_sdbFsmWakeUp(void)
{
#if !defined(FTDF_NO_CSL) || !defined(FTDF_NO_TSCH)
        if (FTDF_pib.leEnabled || FTDF_pib.tschEnabled) {
                return;
        }
#endif
        switch (FTDF_sdb.state)
        {
        case FTDF_SDB_STATE_RESUMING:
                FTDF_sdbResume();
                break;
        case FTDF_SDB_STATE_WAITING_WAKE_UP_IRQ:
                break;
        case FTDF_SDB_STATE_INIT:
                break;
        default:
                ASSERT_WARNING(0);
        }
}

void FTDF_sdbFsmTxIRQ(void)
{
#if !defined(FTDF_NO_CSL) || !defined(FTDF_NO_TSCH)
        if (FTDF_pib.leEnabled || FTDF_pib.tschEnabled) {
                return;
        }
#endif
        switch (FTDF_sdb.state)
        {
        case FTDF_SDB_STATE_RESUMING:
                FTDF_sdbReset();
                FTDF_sdb.state = FTDF_SDB_STATE_INIT;
                break;
        case FTDF_SDB_STATE_BACKING_OFF:
                FTDF_sdb.state = FTDF_SDB_STATE_INIT;
                break;
        case FTDF_SDB_STATE_INIT:
                break;
        default:
                ASSERT_WARNING(0);
        }
}

FTDF_USec FTDF_sdbGetSleepTime(void)
{
        FTDF_USec sleepTime = ~((FTDF_USec) 0);
#if !defined(FTDF_NO_CSL) || !defined(FTDF_NO_TSCH)
        if (FTDF_pib.leEnabled || FTDF_pib.tschEnabled) {
                return sleepTime;
        }
#endif
        switch (FTDF_sdb.state)
        {
        case FTDF_SDB_STATE_INIT:
        {
                if ((FTDF_GET_FIELD( ON_OFF_REGMAP_LMACREADY4SLEEP ) == 0) || FTDF_reqCurrent) {
                        sleepTime = 0;
                } else {
                        sleepTime = ~((FTDF_USec) 0);
                }
                break;
        }
        case FTDF_SDB_STATE_BACKING_OFF:
        {
                FTDF_Time currentTime = FTDF_GET_FIELD( ON_OFF_REGMAP_SYMBOLTIMESNAPSHOTVAL );
                if (currentTime <= FTDF_sdb.ccaRetryTime) {
                        sleepTime = (FTDF_sdb.ccaRetryTime - currentTime) * 16;
                } else {
                        sleepTime = (1 << SIZE_F_FTDF_ON_OFF_REGMAP_SYMBOLTIMESNAPSHOTVAL - 1) -
                                (currentTime + FTDF_sdb.ccaRetryTime) * 16;
                }
                if (sleepTime > 256 * FTDF_UNIT_BACKOFF_PERIOD * 16) {
                        /* We have exceeded the CCA retry time. Abort sleep and wait for Tx IRQ. */
                        sleepTime = 0;
                }
                break;
        }
        case FTDF_SDB_STATE_RESUMING:
                sleepTime = 0;
                break;
        case FTDF_SDB_STATE_WAITING_WAKE_UP_IRQ:
                sleepTime = ~((FTDF_USec) 0);
                break;
        default:
                ASSERT_WARNING(0);
        }
        return sleepTime;
}

#endif /* FTDF_USE_SLEEP_DURING_BACKOFF */

#endif /* #if dg_configBLACK_ORCA_IC_REV != BLACK_ORCA_IC_REV_A */

#if dg_configUSE_FTDF_DDPHY == 1
void FTDF_ddphySet(uint16_t ccaReg)
{
        FTDF_criticalVar();
        FTDF_enterCritical();
        /* We use the critical section here as protection for the global variable and the HW sleep
         * state. */
        FTDF_ddphyCcaReg = ccaReg;
        /* Apply immediately if block is up. */
        if (REG_GETF(CRG_TOP, PMU_CTRL_REG, FTDF_SLEEP) == 0x0) {
                FTDF_DPHY->DDPHY_CCA_REG = FTDF_ddphyCcaReg;
        }
        FTDF_exitCritical();
}

void FTDF_ddphyRestore(void)
{
        if (FTDF_ddphyCcaReg) {
                /* Apply immediately if block is up. */
                FTDF_criticalVar();
                FTDF_enterCritical();
                ASSERT_WARNING(REG_GETF(CRG_TOP, PMU_CTRL_REG, FTDF_SLEEP) == 0x0);
                FTDF_DPHY->DDPHY_CCA_REG = FTDF_ddphyCcaReg;
                FTDF_exitCritical();
        }
}

void FTDF_ddphySave(void)
{
        /* Apply immediately if block is up. */
        FTDF_criticalVar();
        FTDF_enterCritical();
        ASSERT_WARNING(REG_GETF(CRG_TOP, PMU_CTRL_REG, FTDF_SLEEP) == 0x0);
        FTDF_ddphyCcaReg = FTDF_DPHY->DDPHY_CCA_REG;
        FTDF_exitCritical();
}
#endif
