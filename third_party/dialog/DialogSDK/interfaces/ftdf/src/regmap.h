//------------------------------------------------------------------------------
// Created at 25.11.2014, 12:42 by mkregmap
//   release: v2_42 (build date Oct  9 2014)
//   options:  -i ftdf_src.rgm -hdr -all
//------------------------------------------------------------------------------
#ifndef _ftdf
#define _ftdf

#if dg_configBLACK_ORCA_IC_REV == BLACK_ORCA_IC_REV_A
/*
 *
 * REGISTER: ftdf_retention_ram_tx_fifo
 *
 * Initialization value: none.
 * Access mode: C-RW (control - read/write)
 ************************************************************************************
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
 ****************************************************************************************
 */
#define IND_R_FTDF_RETENTION_RAM_TX_FIFO    0x40080000
#define SIZE_R_FTDF_RETENTION_RAM_TX_FIFO   0x4
#define MSK_R_FTDF_RETENTION_RAM_TX_FIFO    0xffffffff
#define FTDF_RETENTION_RAM_TX_FIFO_NUM_1D   0x4
#define FTDF_RETENTION_RAM_TX_FIFO_INTVL_1D 0x80
#define FTDF_RETENTION_RAM_TX_FIFO_NUM_2D   0x20
#define FTDF_RETENTION_RAM_TX_FIFO_INTVL_2D 0x4
#define FTDF_RETENTION_RAM_TX_FIFO_NUM_1D   0x4
#define FTDF_RETENTION_RAM_TX_FIFO_INTVL_1D 0x80
#define FTDF_RETENTION_RAM_TX_FIFO_NUM_2D   0x20
#define FTDF_RETENTION_RAM_TX_FIFO_INTVL_2D 0x4

/*
 *
 * Field: ftdf_retention_ram_tx_fifo
 *
 * Transmit fifo buffer, contains 32 addresses per entry (32b x 32a = 128B). There are 4 entries supported.
 */
#define IND_F_FTDF_RETENTION_RAM_TX_FIFO  IND_R_FTDF_RETENTION_RAM_TX_FIFO
#define MSK_F_FTDF_RETENTION_RAM_TX_FIFO  0xffffffff
#define OFF_F_FTDF_RETENTION_RAM_TX_FIFO  0
#define SIZE_F_FTDF_RETENTION_RAM_TX_FIFO 32

/*
 *
 * REGISTER: ftdf_retention_ram_tx_meta_data_0
 *
 * Initialization value: none.
 * Access mode: C-RW (control - read/write)
 */
#define IND_R_FTDF_RETENTION_RAM_TX_META_DATA_0  0x40080200
#define SIZE_R_FTDF_RETENTION_RAM_TX_META_DATA_0 0x4
#define MSK_R_FTDF_RETENTION_RAM_TX_META_DATA_0  0x57ffffff
#define FTDF_RETENTION_RAM_TX_META_DATA_0_NUM    0x4
#define FTDF_RETENTION_RAM_TX_META_DATA_0_INTVL  0x10
#define FTDF_RETENTION_RAM_FRAME_LENGTH_NUM      0x4
#define FTDF_RETENTION_RAM_FRAME_LENGTH_INTVL    0x10
#define FTDF_RETENTION_RAM_PHYATTR_NUM           0x4
#define FTDF_RETENTION_RAM_PHYATTR_INTVL         0x10
#define FTDF_RETENTION_RAM_FRAMETYPE_NUM         0x4
#define FTDF_RETENTION_RAM_FRAMETYPE_INTVL       0x10
#define FTDF_RETENTION_RAM_CSMACA_ENA_NUM        0x4
#define FTDF_RETENTION_RAM_CSMACA_ENA_INTVL      0x10
#define FTDF_RETENTION_RAM_ACKREQUEST_NUM        0x4
#define FTDF_RETENTION_RAM_ACKREQUEST_INTVL      0x10
#define FTDF_RETENTION_RAM_CRC16_ENA_NUM         0x4
#define FTDF_RETENTION_RAM_CRC16_ENA_INTVL       0x10

/*
 *
 * Field: ftdf_retention_ram_frame_length
 *
 * Frame length
 */
#define IND_F_FTDF_RETENTION_RAM_FRAME_LENGTH  IND_R_FTDF_RETENTION_RAM_TX_META_DATA_0
#define MSK_F_FTDF_RETENTION_RAM_FRAME_LENGTH  0x7f
#define OFF_F_FTDF_RETENTION_RAM_FRAME_LENGTH  0
#define SIZE_F_FTDF_RETENTION_RAM_FRAME_LENGTH 7
/*
 *
 * Field: ftdf_retention_ram_phyAttr
 *
 * During the RX-ON phase, the control register phyRxAttr[16] sources the PHYATTR output signal to the DPHY.
 * This is also applicable during a CCA or Energy detect scan phase.
 */
#define IND_F_FTDF_RETENTION_RAM_PHYATTR       IND_R_FTDF_RETENTION_RAM_TX_META_DATA_0
#define MSK_F_FTDF_RETENTION_RAM_PHYATTR       0x7fff80
#define OFF_F_FTDF_RETENTION_RAM_PHYATTR       7
#define SIZE_F_FTDF_RETENTION_RAM_PHYATTR      16
/*
 *
 * Field: ftdf_retention_ram_frametype
 *
 * Data/Cmd/Ack etc. Also indicate wakeup frame
 */
#define IND_F_FTDF_RETENTION_RAM_FRAMETYPE     IND_R_FTDF_RETENTION_RAM_TX_META_DATA_0
#define MSK_F_FTDF_RETENTION_RAM_FRAMETYPE     0x3800000
#define OFF_F_FTDF_RETENTION_RAM_FRAMETYPE     23
#define SIZE_F_FTDF_RETENTION_RAM_FRAMETYPE    3
/*
 *
 * Bit: ftdf_retention_ram_CsmaCa_ena
 *
 * Indicates whether a CSMA-CA is required for the transmission of this packet.
 */
#define IND_F_FTDF_RETENTION_RAM_CSMACA_ENA    IND_R_FTDF_RETENTION_RAM_TX_META_DATA_0
#define MSK_F_FTDF_RETENTION_RAM_CSMACA_ENA    0x4000000
#define OFF_F_FTDF_RETENTION_RAM_CSMACA_ENA    26
#define SIZE_F_FTDF_RETENTION_RAM_CSMACA_ENA   1
/*
 *
 * Bit: ftdf_retention_ram_AckRequest
 *
 * Indicates whether an acknowledge is expected from the recipient of this packet.
 */
#define IND_F_FTDF_RETENTION_RAM_ACKREQUEST    IND_R_FTDF_RETENTION_RAM_TX_META_DATA_0
#define MSK_F_FTDF_RETENTION_RAM_ACKREQUEST    0x10000000
#define OFF_F_FTDF_RETENTION_RAM_ACKREQUEST    28
#define SIZE_F_FTDF_RETENTION_RAM_ACKREQUEST   1
/*
 *
 * Bit: ftdf_retention_ram_CRC16_ena
 *
 * Indicates whether CRC16 insertion must be enabled or not.
 *  0 : No hardware inserted CRC16
 *  1 : Hardware inserts CRC16
 */
#define IND_F_FTDF_RETENTION_RAM_CRC16_ENA     IND_R_FTDF_RETENTION_RAM_TX_META_DATA_0
#define MSK_F_FTDF_RETENTION_RAM_CRC16_ENA     0x40000000
#define OFF_F_FTDF_RETENTION_RAM_CRC16_ENA     30
#define SIZE_F_FTDF_RETENTION_RAM_CRC16_ENA    1

/*
 *
 * REGISTER: ftdf_retention_ram_tx_meta_data_1
 *
 * Initialization value: none.
 * Access mode: C-RW (control - read/write)
 */
#define IND_R_FTDF_RETENTION_RAM_TX_META_DATA_1  0x40080204
#define SIZE_R_FTDF_RETENTION_RAM_TX_META_DATA_1 0x4
#define MSK_R_FTDF_RETENTION_RAM_TX_META_DATA_1  0xff
#define FTDF_RETENTION_RAM_TX_META_DATA_1_NUM    0x4
#define FTDF_RETENTION_RAM_TX_META_DATA_1_INTVL  0x10
#define FTDF_RETENTION_RAM_MACSN_NUM             0x4
#define FTDF_RETENTION_RAM_MACSN_INTVL           0x10

/*
 *
 * Field: ftdf_retention_ram_macSN
 *
 * Sequence Number of this packet.
 */
#define IND_F_FTDF_RETENTION_RAM_MACSN  IND_R_FTDF_RETENTION_RAM_TX_META_DATA_1
#define MSK_F_FTDF_RETENTION_RAM_MACSN  0xff
#define OFF_F_FTDF_RETENTION_RAM_MACSN  0
#define SIZE_F_FTDF_RETENTION_RAM_MACSN 8

/*
 *
 * REGISTER: ftdf_retention_ram_tx_return_status_0
 *
 * Initialization value: none.
 * Access mode: S-R (state - read)
 */
#define IND_R_FTDF_RETENTION_RAM_TX_RETURN_STATUS_0  0x40080240
#define SIZE_R_FTDF_RETENTION_RAM_TX_RETURN_STATUS_0 0x4
#define MSK_R_FTDF_RETENTION_RAM_TX_RETURN_STATUS_0  0xffffffff
#define FTDF_RETENTION_RAM_TX_RETURN_STATUS_0_NUM    0x4
#define FTDF_RETENTION_RAM_TX_RETURN_STATUS_0_INTVL  0x10
#define FTDF_RETENTION_RAM_TXTIMESTAMP_NUM           0x4
#define FTDF_RETENTION_RAM_TXTIMESTAMP_INTVL         0x10

/*
 *
 * Field: ftdf_retention_ram_TxTimestamp
 *
 * Transmit Timestamp
 * The TimeStamp of the transmitted packet.
 */
#define IND_F_FTDF_RETENTION_RAM_TXTIMESTAMP  IND_R_FTDF_RETENTION_RAM_TX_RETURN_STATUS_0
#define MSK_F_FTDF_RETENTION_RAM_TXTIMESTAMP  0xffffffff
#define OFF_F_FTDF_RETENTION_RAM_TXTIMESTAMP  0
#define SIZE_F_FTDF_RETENTION_RAM_TXTIMESTAMP 32

/*
 *
 * REGISTER: ftdf_retention_ram_tx_return_status_1
 *
 * Initialization value: none.
 * Access mode: S-R (state - read)
 */
#define IND_R_FTDF_RETENTION_RAM_TX_RETURN_STATUS_1  0x40080244
#define SIZE_R_FTDF_RETENTION_RAM_TX_RETURN_STATUS_1 0x4
#define MSK_R_FTDF_RETENTION_RAM_TX_RETURN_STATUS_1  0x1f
#define FTDF_RETENTION_RAM_TX_RETURN_STATUS_1_NUM    0x4
#define FTDF_RETENTION_RAM_TX_RETURN_STATUS_1_INTVL  0x10
#define FTDF_RETENTION_RAM_ACKFAIL_NUM               0x4
#define FTDF_RETENTION_RAM_ACKFAIL_INTVL             0x10
#define FTDF_RETENTION_RAM_CSMACAFAIL_NUM            0x4
#define FTDF_RETENTION_RAM_CSMACAFAIL_INTVL          0x10
#define FTDF_RETENTION_RAM_CSMACANRRETRIES_NUM       0x4
#define FTDF_RETENTION_RAM_CSMACANRRETRIES_INTVL     0x10

/*
 *
 * Bit: ftdf_retention_ram_AckFail
 *
 * Acknowledgement status
 *  0 : SUCCESS
 *  1 : FAIL
 */
#define IND_F_FTDF_RETENTION_RAM_ACKFAIL          IND_R_FTDF_RETENTION_RAM_TX_RETURN_STATUS_1
#define MSK_F_FTDF_RETENTION_RAM_ACKFAIL          0x1
#define OFF_F_FTDF_RETENTION_RAM_ACKFAIL          0
#define SIZE_F_FTDF_RETENTION_RAM_ACKFAIL         1
/*
 *
 * Bit: ftdf_retention_ram_CsmaCaFail
 *
 * CSMA-CA status
 *  0 : SUCCESS
 *  1 : FAIL
 */
#define IND_F_FTDF_RETENTION_RAM_CSMACAFAIL       IND_R_FTDF_RETENTION_RAM_TX_RETURN_STATUS_1
#define MSK_F_FTDF_RETENTION_RAM_CSMACAFAIL       0x2
#define OFF_F_FTDF_RETENTION_RAM_CSMACAFAIL       1
#define SIZE_F_FTDF_RETENTION_RAM_CSMACAFAIL      1
/*
 *
 * Field: ftdf_retention_ram_CsmaCaNrRetries
 *
 * Number of CSMA-CA retries
 */
#define IND_F_FTDF_RETENTION_RAM_CSMACANRRETRIES  IND_R_FTDF_RETENTION_RAM_TX_RETURN_STATUS_1
#define MSK_F_FTDF_RETENTION_RAM_CSMACANRRETRIES  0x1c
#define OFF_F_FTDF_RETENTION_RAM_CSMACANRRETRIES  2
#define SIZE_F_FTDF_RETENTION_RAM_CSMACANRRETRIES 3

/*
 *
 * REGISTER: ftdf_retention_ram_rx_meta_0
 *
 * Initialization value: none.
 * Access mode: S-R (state - read)
 */
#define IND_R_FTDF_RETENTION_RAM_RX_META_0    0x40080280
#define SIZE_R_FTDF_RETENTION_RAM_RX_META_0   0x4
#define MSK_R_FTDF_RETENTION_RAM_RX_META_0    0xffffffff
#define FTDF_RETENTION_RAM_RX_META_0_NUM      0x8
#define FTDF_RETENTION_RAM_RX_META_0_INTVL    0x10
#define FTDF_RETENTION_RAM_RX_TIMESTAMP_NUM   0x8
#define FTDF_RETENTION_RAM_RX_TIMESTAMP_INTVL 0x10

/*
 *
 * Field: ftdf_retention_ram_rx_timestamp
 *
 * Timestamp taken when frame was received
 */
#define IND_F_FTDF_RETENTION_RAM_RX_TIMESTAMP  IND_R_FTDF_RETENTION_RAM_RX_META_0
#define MSK_F_FTDF_RETENTION_RAM_RX_TIMESTAMP  0xffffffff
#define OFF_F_FTDF_RETENTION_RAM_RX_TIMESTAMP  0
#define SIZE_F_FTDF_RETENTION_RAM_RX_TIMESTAMP 32

/*
 *
 * REGISTER: ftdf_retention_ram_rx_meta_1
 *
 * Initialization value: none.
 * Access mode: S-R (state - read)
 */
#define IND_R_FTDF_RETENTION_RAM_RX_META_1             0x40080284
#define SIZE_R_FTDF_RETENTION_RAM_RX_META_1            0x4
#define MSK_R_FTDF_RETENTION_RAM_RX_META_1             0xfffd
#define FTDF_RETENTION_RAM_RX_META_1_NUM               0x8
#define FTDF_RETENTION_RAM_RX_META_1_INTVL             0x10
#define FTDF_RETENTION_RAM_CRC16_ERROR_NUM             0x8
#define FTDF_RETENTION_RAM_CRC16_ERROR_INTVL           0x10
#define FTDF_RETENTION_RAM_RES_FRM_TYPE_ERROR_NUM      0x8
#define FTDF_RETENTION_RAM_RES_FRM_TYPE_ERROR_INTVL    0x10
#define FTDF_RETENTION_RAM_RES_FRM_VERSION_ERROR_NUM   0x8
#define FTDF_RETENTION_RAM_RES_FRM_VERSION_ERROR_INTVL 0x10
#define FTDF_RETENTION_RAM_DPANID_ERROR_NUM            0x8
#define FTDF_RETENTION_RAM_DPANID_ERROR_INTVL          0x10
#define FTDF_RETENTION_RAM_DADDR_ERROR_NUM             0x8
#define FTDF_RETENTION_RAM_DADDR_ERROR_INTVL           0x10
#define FTDF_RETENTION_RAM_SPANID_ERROR_NUM            0x8
#define FTDF_RETENTION_RAM_SPANID_ERROR_INTVL          0x10
#define FTDF_RETENTION_RAM_ISPANID_COORD_ERROR_NUM     0x8
#define FTDF_RETENTION_RAM_ISPANID_COORD_ERROR_INTVL   0x10
#define FTDF_RETENTION_RAM_QUALITY_INDICATOR_NUM       0x8
#define FTDF_RETENTION_RAM_QUALITY_INDICATOR_INTVL     0x10

/*
 *
 * Bit: ftdf_retention_ram_crc16_error
 *
 * CRC error, applicable for transparent mode only
 */
#define IND_F_FTDF_RETENTION_RAM_CRC16_ERROR            IND_R_FTDF_RETENTION_RAM_RX_META_1
#define MSK_F_FTDF_RETENTION_RAM_CRC16_ERROR            0x1
#define OFF_F_FTDF_RETENTION_RAM_CRC16_ERROR            0
#define SIZE_F_FTDF_RETENTION_RAM_CRC16_ERROR           1
/*
 *
 * Bit: ftdf_retention_ram_res_frm_type_error
 *
 * Not supported frame type error, applicable when frame is not discarded
 */
#define IND_F_FTDF_RETENTION_RAM_RES_FRM_TYPE_ERROR     IND_R_FTDF_RETENTION_RAM_RX_META_1
#define MSK_F_FTDF_RETENTION_RAM_RES_FRM_TYPE_ERROR     0x4
#define OFF_F_FTDF_RETENTION_RAM_RES_FRM_TYPE_ERROR     2
#define SIZE_F_FTDF_RETENTION_RAM_RES_FRM_TYPE_ERROR    1
/*
 *
 * Bit: ftdf_retention_ram_res_frm_version_error
 *
 * Not supported frame version error, applicable when frame is not discarded.
 */
#define IND_F_FTDF_RETENTION_RAM_RES_FRM_VERSION_ERROR  IND_R_FTDF_RETENTION_RAM_RX_META_1
#define MSK_F_FTDF_RETENTION_RAM_RES_FRM_VERSION_ERROR  0x8
#define OFF_F_FTDF_RETENTION_RAM_RES_FRM_VERSION_ERROR  3
#define SIZE_F_FTDF_RETENTION_RAM_RES_FRM_VERSION_ERROR 1
/*
 *
 * Bit: ftdf_retention_ram_dpanid_error
 *
 * D PAN ID error, applicable when frame is not discarded
 */
#define IND_F_FTDF_RETENTION_RAM_DPANID_ERROR           IND_R_FTDF_RETENTION_RAM_RX_META_1
#define MSK_F_FTDF_RETENTION_RAM_DPANID_ERROR           0x10
#define OFF_F_FTDF_RETENTION_RAM_DPANID_ERROR           4
#define SIZE_F_FTDF_RETENTION_RAM_DPANID_ERROR          1
/*
 *
 * Bit: ftdf_retention_ram_daddr_error
 *
 * D Address error, applicable when frame is not discarded
 */
#define IND_F_FTDF_RETENTION_RAM_DADDR_ERROR            IND_R_FTDF_RETENTION_RAM_RX_META_1
#define MSK_F_FTDF_RETENTION_RAM_DADDR_ERROR            0x20
#define OFF_F_FTDF_RETENTION_RAM_DADDR_ERROR            5
#define SIZE_F_FTDF_RETENTION_RAM_DADDR_ERROR           1
/*
 *
 * Bit: ftdf_retention_ram_spanid_error
 *
 * PAN ID error, applicable when frame is not discarded
 */
#define IND_F_FTDF_RETENTION_RAM_SPANID_ERROR           IND_R_FTDF_RETENTION_RAM_RX_META_1
#define MSK_F_FTDF_RETENTION_RAM_SPANID_ERROR           0x40
#define OFF_F_FTDF_RETENTION_RAM_SPANID_ERROR           6
#define SIZE_F_FTDF_RETENTION_RAM_SPANID_ERROR          1
/*
 *
 * Bit: ftdf_retention_ram_ispanid_coord_error
 *
 * Received frame not for PAN coordinator, applicable when frame is not discarded
 */
#define IND_F_FTDF_RETENTION_RAM_ISPANID_COORD_ERROR    IND_R_FTDF_RETENTION_RAM_RX_META_1
#define MSK_F_FTDF_RETENTION_RAM_ISPANID_COORD_ERROR    0x80
#define OFF_F_FTDF_RETENTION_RAM_ISPANID_COORD_ERROR    7
#define SIZE_F_FTDF_RETENTION_RAM_ISPANID_COORD_ERROR   1
/*
 *
 * Field: ftdf_retention_ram_quality_indicator
 *
 * Link Quality Indication
 *
 * # $software_scratch@retention_ram
 * # TX ram not used by hardware, can be used by software as scratch ram with retention.
 */
#define IND_F_FTDF_RETENTION_RAM_QUALITY_INDICATOR      IND_R_FTDF_RETENTION_RAM_RX_META_1
#define MSK_F_FTDF_RETENTION_RAM_QUALITY_INDICATOR      0xff00
#define OFF_F_FTDF_RETENTION_RAM_QUALITY_INDICATOR      8
#define SIZE_F_FTDF_RETENTION_RAM_QUALITY_INDICATOR     8

/*
 *
 * REGISTER: ftdf_rx_ram_rx_fifo
 *
 * Initialization value: none.
 * Access mode: C-RW (control - read/write)
 */
#define IND_R_FTDF_RX_RAM_RX_FIFO    0x40088000
#define SIZE_R_FTDF_RX_RAM_RX_FIFO   0x4
#define MSK_R_FTDF_RX_RAM_RX_FIFO    0xffffffff
#define FTDF_RX_RAM_RX_FIFO_NUM_1D   0x8
#define FTDF_RX_RAM_RX_FIFO_INTVL_1D 0x80
#define FTDF_RX_RAM_RX_FIFO_NUM_2D   0x20
#define FTDF_RX_RAM_RX_FIFO_INTVL_2D 0x4
#define FTDF_RX_RAM_RX_FIFO_NUM_1D   0x8
#define FTDF_RX_RAM_RX_FIFO_INTVL_1D 0x80
#define FTDF_RX_RAM_RX_FIFO_NUM_2D   0x20
#define FTDF_RX_RAM_RX_FIFO_INTVL_2D 0x4

/*
 *
 * Field: ftdf_rx_ram_rx_fifo
 *
 * Receive fifo ram, contains 32 addresses per entry (32b x 32a = 128B). There are 8 entries supported.
 */
#define IND_F_FTDF_RX_RAM_RX_FIFO  IND_R_FTDF_RX_RAM_RX_FIFO
#define MSK_F_FTDF_RX_RAM_RX_FIFO  0xffffffff
#define OFF_F_FTDF_RX_RAM_RX_FIFO  0
#define SIZE_F_FTDF_RX_RAM_RX_FIFO 32

/*
 *
 * REGISTER: ftdf_on_off_regmap_Rel_Name
 *
 * Initialization value: none.
 * Access mode: S-R (state - read)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_REL_NAME  0x40090000
#define SIZE_R_FTDF_ON_OFF_REGMAP_REL_NAME 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_REL_NAME  0xffffffff
#define FTDF_ON_OFF_REGMAP_REL_NAME_NUM    0x4
#define FTDF_ON_OFF_REGMAP_REL_NAME_INTVL  0x4
#define FTDF_ON_OFF_REGMAP_REL_NAME_NUM    0x4
#define FTDF_ON_OFF_REGMAP_REL_NAME_INTVL  0x4

/*
 *
 * Field: ftdf_on_off_regmap_Rel_Name
 *
 * Name of the release
 */
#define IND_F_FTDF_ON_OFF_REGMAP_REL_NAME  IND_R_FTDF_ON_OFF_REGMAP_REL_NAME
#define MSK_F_FTDF_ON_OFF_REGMAP_REL_NAME  0xffffffff
#define OFF_F_FTDF_ON_OFF_REGMAP_REL_NAME  0
#define SIZE_F_FTDF_ON_OFF_REGMAP_REL_NAME 32

/*
 *
 * REGISTER: ftdf_on_off_regmap_BuildTime
 *
 * Initialization value: none.
 * Access mode: S-R (state - read)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_BUILDTIME  0x40090010
#define SIZE_R_FTDF_ON_OFF_REGMAP_BUILDTIME 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_BUILDTIME  0xffffffff
#define FTDF_ON_OFF_REGMAP_BUILDTIME_NUM    0x4
#define FTDF_ON_OFF_REGMAP_BUILDTIME_INTVL  0x4
#define FTDF_ON_OFF_REGMAP_BUILDTIME_NUM    0x4
#define FTDF_ON_OFF_REGMAP_BUILDTIME_INTVL  0x4

/*
 *
 * Field: ftdf_on_off_regmap_BuildTime
 *
 * Build time of device
 */
#define IND_F_FTDF_ON_OFF_REGMAP_BUILDTIME  IND_R_FTDF_ON_OFF_REGMAP_BUILDTIME
#define MSK_F_FTDF_ON_OFF_REGMAP_BUILDTIME  0xffffffff
#define OFF_F_FTDF_ON_OFF_REGMAP_BUILDTIME  0
#define SIZE_F_FTDF_ON_OFF_REGMAP_BUILDTIME 32

/*
 *
 * REGISTER: ftdf_on_off_regmap_glob_control_0
 *
 * Initialization value: 0xff00  Initialization mask: 0x6ff0e
 * Access mode: C-RW (control - read/write)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_GLOB_CONTROL_0  0x40090020
#define SIZE_R_FTDF_ON_OFF_REGMAP_GLOB_CONTROL_0 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_GLOB_CONTROL_0  0x6ff0e

/*
 *
 * Bit: ftdf_on_off_regmap_IsPANCoordinator
 *
 * Enable/disable receiver check on address
 * fields (0=enabled, 1=disabled)
 */
#define IND_F_FTDF_ON_OFF_REGMAP_ISPANCOORDINATOR  IND_R_FTDF_ON_OFF_REGMAP_GLOB_CONTROL_0
#define MSK_F_FTDF_ON_OFF_REGMAP_ISPANCOORDINATOR  0x2
#define OFF_F_FTDF_ON_OFF_REGMAP_ISPANCOORDINATOR  1
#define SIZE_F_FTDF_ON_OFF_REGMAP_ISPANCOORDINATOR 1
/*
 *
 * Bit: ftdf_on_off_regmap_rx_dma_req
 *
 * Source of the RX_DMA_REQ output of this block.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_RX_DMA_REQ        IND_R_FTDF_ON_OFF_REGMAP_GLOB_CONTROL_0
#define MSK_F_FTDF_ON_OFF_REGMAP_RX_DMA_REQ        0x4
#define OFF_F_FTDF_ON_OFF_REGMAP_RX_DMA_REQ        2
#define SIZE_F_FTDF_ON_OFF_REGMAP_RX_DMA_REQ       1
/*
 *
 * Bit: ftdf_on_off_regmap_tx_dma_req
 *
 * Source of the TX_DMA_REQ output of this block.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_TX_DMA_REQ        IND_R_FTDF_ON_OFF_REGMAP_GLOB_CONTROL_0
#define MSK_F_FTDF_ON_OFF_REGMAP_TX_DMA_REQ        0x8
#define OFF_F_FTDF_ON_OFF_REGMAP_TX_DMA_REQ        3
#define SIZE_F_FTDF_ON_OFF_REGMAP_TX_DMA_REQ       1
/*
 *
 * Field: ftdf_on_off_regmap_macSimpleAddress
 *
 * Simple address of the PAN coordinator
 */
#define IND_F_FTDF_ON_OFF_REGMAP_MACSIMPLEADDRESS  IND_R_FTDF_ON_OFF_REGMAP_GLOB_CONTROL_0
#define MSK_F_FTDF_ON_OFF_REGMAP_MACSIMPLEADDRESS  0xff00
#define OFF_F_FTDF_ON_OFF_REGMAP_MACSIMPLEADDRESS  8
#define SIZE_F_FTDF_ON_OFF_REGMAP_MACSIMPLEADDRESS 8
/*
 *
 * Bit: ftdf_on_off_regmap_macLEenabled
 *
 * If set, Low Energy mode is enabled
 */
#define IND_F_FTDF_ON_OFF_REGMAP_MACLEENABLED      IND_R_FTDF_ON_OFF_REGMAP_GLOB_CONTROL_0
#define MSK_F_FTDF_ON_OFF_REGMAP_MACLEENABLED      0x20000
#define OFF_F_FTDF_ON_OFF_REGMAP_MACLEENABLED      17
#define SIZE_F_FTDF_ON_OFF_REGMAP_MACLEENABLED     1
/*
 *
 * Bit: ftdf_on_off_regmap_macTSCHenabled
 *
 * If set, TSCH mode is enabled
 */
#define IND_F_FTDF_ON_OFF_REGMAP_MACTSCHENABLED    IND_R_FTDF_ON_OFF_REGMAP_GLOB_CONTROL_0
#define MSK_F_FTDF_ON_OFF_REGMAP_MACTSCHENABLED    0x40000
#define OFF_F_FTDF_ON_OFF_REGMAP_MACTSCHENABLED    18
#define SIZE_F_FTDF_ON_OFF_REGMAP_MACTSCHENABLED   1

/*
 *
 * REGISTER: ftdf_on_off_regmap_glob_control_1
 *
 * Initialization value: 0xffffffff  Initialization mask: 0xffffffff
 * Access mode: C-RW (control - read/write)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_GLOB_CONTROL_1  0x40090024
#define SIZE_R_FTDF_ON_OFF_REGMAP_GLOB_CONTROL_1 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_GLOB_CONTROL_1  0xffffffff

/*
 *
 * Field: ftdf_on_off_regmap_macPANId
 *
 * The values 0xFFFF indicates that the device
 * is not associated
 */
#define IND_F_FTDF_ON_OFF_REGMAP_MACPANID         IND_R_FTDF_ON_OFF_REGMAP_GLOB_CONTROL_1
#define MSK_F_FTDF_ON_OFF_REGMAP_MACPANID         0xffff
#define OFF_F_FTDF_ON_OFF_REGMAP_MACPANID         0
#define SIZE_F_FTDF_ON_OFF_REGMAP_MACPANID        16
/*
 *
 * Field: ftdf_on_off_regmap_macShortAddress
 *
 * The values 0xFFFF and 0xFFFE indicate
 * that no IEEE Short Address is available.
 * The latter one is used if the device i
 */
#define IND_F_FTDF_ON_OFF_REGMAP_MACSHORTADDRESS  IND_R_FTDF_ON_OFF_REGMAP_GLOB_CONTROL_1
#define MSK_F_FTDF_ON_OFF_REGMAP_MACSHORTADDRESS  0xffff0000
#define OFF_F_FTDF_ON_OFF_REGMAP_MACSHORTADDRESS  16
#define SIZE_F_FTDF_ON_OFF_REGMAP_MACSHORTADDRESS 16

/*
 *
 * REGISTER: ftdf_on_off_regmap_glob_control_2
 *
 * Initialization value: 0xffffffff  Initialization mask: 0xffffffff
 * Access mode: C-RW (control - read/write)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_GLOB_CONTROL_2  0x40090028
#define SIZE_R_FTDF_ON_OFF_REGMAP_GLOB_CONTROL_2 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_GLOB_CONTROL_2  0xffffffff

/*
 *
 * Field: ftdf_on_off_regmap_aExtendedAddress_l
 *
 * Unique device address, lower 32 bit
 */
#define IND_F_FTDF_ON_OFF_REGMAP_AEXTENDEDADDRESS_L  IND_R_FTDF_ON_OFF_REGMAP_GLOB_CONTROL_2
#define MSK_F_FTDF_ON_OFF_REGMAP_AEXTENDEDADDRESS_L  0xffffffff
#define OFF_F_FTDF_ON_OFF_REGMAP_AEXTENDEDADDRESS_L  0
#define SIZE_F_FTDF_ON_OFF_REGMAP_AEXTENDEDADDRESS_L 32

/*
 *
 * REGISTER: ftdf_on_off_regmap_glob_control_3
 *
 * Initialization value: 0xffffffff  Initialization mask: 0xffffffff
 * Access mode: C-RW (control - read/write)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_GLOB_CONTROL_3  0x4009002c
#define SIZE_R_FTDF_ON_OFF_REGMAP_GLOB_CONTROL_3 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_GLOB_CONTROL_3  0xffffffff

/*
 *
 * Field: ftdf_on_off_regmap_aExtendedAddress_h
 *
 * Unique device address, higher 16 bit
 */
#define IND_F_FTDF_ON_OFF_REGMAP_AEXTENDEDADDRESS_H  IND_R_FTDF_ON_OFF_REGMAP_GLOB_CONTROL_3
#define MSK_F_FTDF_ON_OFF_REGMAP_AEXTENDEDADDRESS_H  0xffffffff
#define OFF_F_FTDF_ON_OFF_REGMAP_AEXTENDEDADDRESS_H  0
#define SIZE_F_FTDF_ON_OFF_REGMAP_AEXTENDEDADDRESS_H 32

/*
 *
 * REGISTER: ftdf_on_off_regmap_lmac_control_0
 *
 * Initialization value: 0x0  Initialization mask: 0xfbfffffe
 * Access mode: C-RW (control - read/write)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_0  0x40090030
#define SIZE_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_0 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_0  0xfbfffffe

/*
 *
 * Field: ftdf_on_off_regmap_RxonDuration
 *
 * Time the Rx must be on
 */
#define IND_F_FTDF_ON_OFF_REGMAP_RXONDURATION  IND_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_0
#define MSK_F_FTDF_ON_OFF_REGMAP_RXONDURATION  0x1fffffe
#define OFF_F_FTDF_ON_OFF_REGMAP_RXONDURATION  1
#define SIZE_F_FTDF_ON_OFF_REGMAP_RXONDURATION 24
/*
 *
 * Bit: ftdf_on_off_regmap_RxAlwaysOn
 *
 * If set, the receiver shall be always on if
 * RxEnable is set
 */
#define IND_F_FTDF_ON_OFF_REGMAP_RXALWAYSON    IND_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_0
#define MSK_F_FTDF_ON_OFF_REGMAP_RXALWAYSON    0x2000000
#define OFF_F_FTDF_ON_OFF_REGMAP_RXALWAYSON    25
#define SIZE_F_FTDF_ON_OFF_REGMAP_RXALWAYSON   1
/*
 *
 * Field: ftdf_on_off_regmap_pti
 *
 * Info to arbiter if phy_en is set
 */
#define IND_F_FTDF_ON_OFF_REGMAP_PTI           IND_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_0
#define MSK_F_FTDF_ON_OFF_REGMAP_PTI           0x78000000
#define OFF_F_FTDF_ON_OFF_REGMAP_PTI           27
#define SIZE_F_FTDF_ON_OFF_REGMAP_PTI          4
/*
 *
 * Bit: ftdf_on_off_regmap_keep_phy_en
 *
 * When the transmit or receive action is ready (LmacReady4Sleep will is set), the phy_en signal is cleared unless the control register keep_phy_en is set.
 * When the control register keep_phy_en is set, the signal phy_en shall remain being set until the keep_phy_en is cleared.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_KEEP_PHY_EN   IND_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_0
#define MSK_F_FTDF_ON_OFF_REGMAP_KEEP_PHY_EN   0x80000000
#define OFF_F_FTDF_ON_OFF_REGMAP_KEEP_PHY_EN   31
#define SIZE_F_FTDF_ON_OFF_REGMAP_KEEP_PHY_EN  1

/*
 *
 * REGISTER: ftdf_on_off_regmap_TxPipePropDelay
 *
 * Initialization value: 0x0  Initialization mask: 0xff
 * Access mode: C-RW (control - read/write)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_TXPIPEPROPDELAY  0x40090034
#define SIZE_R_FTDF_ON_OFF_REGMAP_TXPIPEPROPDELAY 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_TXPIPEPROPDELAY  0xff

/*
 *
 * Field: ftdf_on_off_regmap_TxPipePropDelay
 *
 * Prop delay of tx pipe, start to DPHY
 */
#define IND_F_FTDF_ON_OFF_REGMAP_TXPIPEPROPDELAY  IND_R_FTDF_ON_OFF_REGMAP_TXPIPEPROPDELAY
#define MSK_F_FTDF_ON_OFF_REGMAP_TXPIPEPROPDELAY  0xff
#define OFF_F_FTDF_ON_OFF_REGMAP_TXPIPEPROPDELAY  0
#define SIZE_F_FTDF_ON_OFF_REGMAP_TXPIPEPROPDELAY 8

/*
 *
 * REGISTER: ftdf_on_off_regmap_MacAckWaitDuration
 *
 * Initialization value: 0x36  Initialization mask: 0xff
 * Access mode: C-RW (control - read/write)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_MACACKWAITDURATION  0x40090038
#define SIZE_R_FTDF_ON_OFF_REGMAP_MACACKWAITDURATION 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_MACACKWAITDURATION  0xff

/*
 *
 * Field: ftdf_on_off_regmap_MacAckWaitDuration
 *
 * Max time to wait for a (normal) ACK
 */
#define IND_F_FTDF_ON_OFF_REGMAP_MACACKWAITDURATION  IND_R_FTDF_ON_OFF_REGMAP_MACACKWAITDURATION
#define MSK_F_FTDF_ON_OFF_REGMAP_MACACKWAITDURATION  0xff
#define OFF_F_FTDF_ON_OFF_REGMAP_MACACKWAITDURATION  0
#define SIZE_F_FTDF_ON_OFF_REGMAP_MACACKWAITDURATION 8

/*
 *
 * REGISTER: ftdf_on_off_regmap_MacEnhAckWaitDuration
 *
 * Initialization value: 0x360  Initialization mask: 0xffff
 * Access mode: C-RW (control - read/write)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_MACENHACKWAITDURATION  0x4009003c
#define SIZE_R_FTDF_ON_OFF_REGMAP_MACENHACKWAITDURATION 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_MACENHACKWAITDURATION  0xffff

/*
 *
 * Field: ftdf_on_off_regmap_MacEnhAckWaitDuration
 *
 * The maximum time (in ??s) to wait for an
 * enhanced acknowledgement frame
 */
#define IND_F_FTDF_ON_OFF_REGMAP_MACENHACKWAITDURATION  IND_R_FTDF_ON_OFF_REGMAP_MACENHACKWAITDURATION
#define MSK_F_FTDF_ON_OFF_REGMAP_MACENHACKWAITDURATION  0xffff
#define OFF_F_FTDF_ON_OFF_REGMAP_MACENHACKWAITDURATION  0
#define SIZE_F_FTDF_ON_OFF_REGMAP_MACENHACKWAITDURATION 16

/*
 *
 * REGISTER: ftdf_on_off_regmap_lmac_control_1
 *
 * Initialization value: 0x0  Initialization mask: 0xffff
 * Access mode: C-RW (control - read/write)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_1  0x40090040
#define SIZE_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_1 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_1  0xffff

/*
 *
 * Field: ftdf_on_off_regmap_phyRxAttr
 *
 * phyattr to DPHY during Rx operation
 */
#define IND_F_FTDF_ON_OFF_REGMAP_PHYRXATTR  IND_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_1
#define MSK_F_FTDF_ON_OFF_REGMAP_PHYRXATTR  0xffff
#define OFF_F_FTDF_ON_OFF_REGMAP_PHYRXATTR  0
#define SIZE_F_FTDF_ON_OFF_REGMAP_PHYRXATTR 16

/*
 *
 * REGISTER: ftdf_on_off_regmap_lmac_control_2
 *
 * Initialization value: 0x0  Initialization mask: 0xffffff01
 * Access mode: C-RW (control - read/write)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_2  0x40090044
#define SIZE_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_2 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_2  0xffffff01

/*
 *
 * Bit: ftdf_on_off_regmap_EdScanEnable
 *
 * if set, Energy Detect scan will be done
 */
#define IND_F_FTDF_ON_OFF_REGMAP_EDSCANENABLE    IND_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_2
#define MSK_F_FTDF_ON_OFF_REGMAP_EDSCANENABLE    0x1
#define OFF_F_FTDF_ON_OFF_REGMAP_EDSCANENABLE    0
#define SIZE_F_FTDF_ON_OFF_REGMAP_EDSCANENABLE   1
/*
 *
 * Field: ftdf_on_off_regmap_EdScanDuration
 *
 * Length of ED scan
 */
#define IND_F_FTDF_ON_OFF_REGMAP_EDSCANDURATION  IND_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_2
#define MSK_F_FTDF_ON_OFF_REGMAP_EDSCANDURATION  0xffffff00
#define OFF_F_FTDF_ON_OFF_REGMAP_EDSCANDURATION  8
#define SIZE_F_FTDF_ON_OFF_REGMAP_EDSCANDURATION 24

/*
 *
 * REGISTER: ftdf_on_off_regmap_lmac_control_3
 *
 * Initialization value: 0x4c4  Initialization mask: 0xffffff
 * Access mode: C-RW (control - read/write)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_3  0x40090048
#define SIZE_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_3 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_3  0xffffff

/*
 *
 * Field: ftdf_on_off_regmap_macMaxFrameTotalWaitTime
 *
 * Max time to wait for a requested Data Frame or an announced broadcast frame
 */
#define IND_F_FTDF_ON_OFF_REGMAP_MACMAXFRAMETOTALWAITTIME  IND_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_3
#define MSK_F_FTDF_ON_OFF_REGMAP_MACMAXFRAMETOTALWAITTIME  0xffff
#define OFF_F_FTDF_ON_OFF_REGMAP_MACMAXFRAMETOTALWAITTIME  0
#define SIZE_F_FTDF_ON_OFF_REGMAP_MACMAXFRAMETOTALWAITTIME 16
/*
 *
 * Field: ftdf_on_off_regmap_CcaIdleWait
 *
 * Time to wait after CCA returned "medium idle" before starting TX-ON (in us).
 * Note: not applicable in TSCH mode since there macTSRxTx shall be used.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_CCAIDLEWAIT               IND_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_3
#define MSK_F_FTDF_ON_OFF_REGMAP_CCAIDLEWAIT               0xff0000
#define OFF_F_FTDF_ON_OFF_REGMAP_CCAIDLEWAIT               16
#define SIZE_F_FTDF_ON_OFF_REGMAP_CCAIDLEWAIT              8

/*
 *
 * REGISTER: ftdf_on_off_regmap_lmac_control_os
 *
 * Initialization value: 0x0  Initialization mask: 0x7
 * Access mode: OS-W (one-shot - write)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_OS  0x40090050
#define SIZE_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_OS 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_OS  0x7

/*
 *
 * Bit: ftdf_on_off_regmap_getGeneratorVal
 *
 * If set, the current values of WU gen and TS gen will be captured.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_GETGENERATORVAL  IND_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_OS
#define MSK_F_FTDF_ON_OFF_REGMAP_GETGENERATORVAL  0x1
#define OFF_F_FTDF_ON_OFF_REGMAP_GETGENERATORVAL  0
#define SIZE_F_FTDF_ON_OFF_REGMAP_GETGENERATORVAL 1
/*
 *
 * Bit: ftdf_on_off_regmap_RxEnable
 *
 * If set, receiving data may be done
 */
#define IND_F_FTDF_ON_OFF_REGMAP_RXENABLE         IND_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_OS
#define MSK_F_FTDF_ON_OFF_REGMAP_RXENABLE         0x2
#define OFF_F_FTDF_ON_OFF_REGMAP_RXENABLE         1
#define SIZE_F_FTDF_ON_OFF_REGMAP_RXENABLE        1
/*
 *
 * Bit: ftdf_on_off_regmap_SingleCCA
 *
 * If set, a single CCA will be performed.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_SINGLECCA        IND_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_OS
#define MSK_F_FTDF_ON_OFF_REGMAP_SINGLECCA        0x4
#define OFF_F_FTDF_ON_OFF_REGMAP_SINGLECCA        2
#define SIZE_F_FTDF_ON_OFF_REGMAP_SINGLECCA       1

/*
 *
 * REGISTER: ftdf_on_off_regmap_lmac_control_status
 *
 * Initialization value: none.
 * Access mode: S-R (state - read)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_STATUS  0x40090054
#define SIZE_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_STATUS 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_STATUS  0xff46

/*
 *
 * Bit: ftdf_on_off_regmap_LmacReady4sleep
 *
 * Indicates that the LMAC is ready to go to sleep.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_LMACREADY4SLEEP          IND_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_STATUS
#define MSK_F_FTDF_ON_OFF_REGMAP_LMACREADY4SLEEP          0x2
#define OFF_F_FTDF_ON_OFF_REGMAP_LMACREADY4SLEEP          1
#define SIZE_F_FTDF_ON_OFF_REGMAP_LMACREADY4SLEEP         1
/*
 *
 * Bit: ftdf_on_off_regmap_CCAStat
 *
 * Value single CCA when CCAstat_e is set.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_CCASTAT                  IND_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_STATUS
#define MSK_F_FTDF_ON_OFF_REGMAP_CCASTAT                  0x4
#define OFF_F_FTDF_ON_OFF_REGMAP_CCASTAT                  2
#define SIZE_F_FTDF_ON_OFF_REGMAP_CCASTAT                 1
/*
 *
 * Bit: ftdf_on_off_regmap_WakeupTimerEnableStatus
 *
 * Status of WakeupTimerEnable after being clocked by LP_CLK (showing it's effective value).
 */
#define IND_F_FTDF_ON_OFF_REGMAP_WAKEUPTIMERENABLESTATUS  IND_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_STATUS
#define MSK_F_FTDF_ON_OFF_REGMAP_WAKEUPTIMERENABLESTATUS  0x40
#define OFF_F_FTDF_ON_OFF_REGMAP_WAKEUPTIMERENABLESTATUS  6
#define SIZE_F_FTDF_ON_OFF_REGMAP_WAKEUPTIMERENABLESTATUS 1
/*
 *
 * Field: ftdf_on_off_regmap_EdScanValue
 *
 * Result of ED scan.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_EDSCANVALUE              IND_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_STATUS
#define MSK_F_FTDF_ON_OFF_REGMAP_EDSCANVALUE              0xff00
#define OFF_F_FTDF_ON_OFF_REGMAP_EDSCANVALUE              8
#define SIZE_F_FTDF_ON_OFF_REGMAP_EDSCANVALUE             8

/*
 *
 * REGISTER: ftdf_on_off_regmap_EventCurrVal
 *
 * Initialization value: none.
 * Access mode: S-R (state - read)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_EVENTCURRVAL  0x40090058
#define SIZE_R_FTDF_ON_OFF_REGMAP_EVENTCURRVAL 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_EVENTCURRVAL  0xffffffff

/*
 *
 * Field: ftdf_on_off_regmap_EventCurrVal
 *
 * Value of captured Event generator
 */
#define IND_F_FTDF_ON_OFF_REGMAP_EVENTCURRVAL  IND_R_FTDF_ON_OFF_REGMAP_EVENTCURRVAL
#define MSK_F_FTDF_ON_OFF_REGMAP_EVENTCURRVAL  0xffffffff
#define OFF_F_FTDF_ON_OFF_REGMAP_EVENTCURRVAL  0
#define SIZE_F_FTDF_ON_OFF_REGMAP_EVENTCURRVAL 32

/*
 *
 * REGISTER: ftdf_on_off_regmap_TimeStampCurrVal
 *
 * Initialization value: none.
 * Access mode: S-R (state - read)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_TIMESTAMPCURRVAL  0x4009005c
#define SIZE_R_FTDF_ON_OFF_REGMAP_TIMESTAMPCURRVAL 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_TIMESTAMPCURRVAL  0xffffffff

/*
 *
 * Field: ftdf_on_off_regmap_TimeStampCurrVal
 *
 * Value of captured TS gen
 */
#define IND_F_FTDF_ON_OFF_REGMAP_TIMESTAMPCURRVAL  IND_R_FTDF_ON_OFF_REGMAP_TIMESTAMPCURRVAL
#define MSK_F_FTDF_ON_OFF_REGMAP_TIMESTAMPCURRVAL  0xffffffff
#define OFF_F_FTDF_ON_OFF_REGMAP_TIMESTAMPCURRVAL  0
#define SIZE_F_FTDF_ON_OFF_REGMAP_TIMESTAMPCURRVAL 32

/*
 *
 * REGISTER: ftdf_on_off_regmap_TimeStampCurrPhaseVal
 *
 * Initialization value: none.
 * Access mode: S-R (state - read)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_TIMESTAMPCURRPHASEVAL  0x40090074
#define SIZE_R_FTDF_ON_OFF_REGMAP_TIMESTAMPCURRPHASEVAL 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_TIMESTAMPCURRPHASEVAL  0xff

/*
 *
 * Field: ftdf_on_off_regmap_TimeStampCurrPhaseVal
 *
 * Value of captured TS gen phase within a symbol
 */
#define IND_F_FTDF_ON_OFF_REGMAP_TIMESTAMPCURRPHASEVAL  IND_R_FTDF_ON_OFF_REGMAP_TIMESTAMPCURRPHASEVAL
#define MSK_F_FTDF_ON_OFF_REGMAP_TIMESTAMPCURRPHASEVAL  0xff
#define OFF_F_FTDF_ON_OFF_REGMAP_TIMESTAMPCURRPHASEVAL  0
#define SIZE_F_FTDF_ON_OFF_REGMAP_TIMESTAMPCURRPHASEVAL 8

/*
 *
 * REGISTER: ftdf_on_off_regmap_macTSTxAckDelayVal
 *
 * Initialization value: none.
 * Access mode: S-R (state - read)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_MACTSTXACKDELAYVAL  0x40090078
#define SIZE_R_FTDF_ON_OFF_REGMAP_MACTSTXACKDELAYVAL 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_MACTSTXACKDELAYVAL  0xffff

/*
 *
 * Field: ftdf_on_off_regmap_macTSTxAckDelayVal
 *
 * The time in us left until the ack frame is sent by the lmac
 */
#define IND_F_FTDF_ON_OFF_REGMAP_MACTSTXACKDELAYVAL  IND_R_FTDF_ON_OFF_REGMAP_MACTSTXACKDELAYVAL
#define MSK_F_FTDF_ON_OFF_REGMAP_MACTSTXACKDELAYVAL  0xffff
#define OFF_F_FTDF_ON_OFF_REGMAP_MACTSTXACKDELAYVAL  0
#define SIZE_F_FTDF_ON_OFF_REGMAP_MACTSTXACKDELAYVAL 16

/*
 *
 * REGISTER: ftdf_on_off_regmap_lmac_control_4
 *
 * Initialization value: 0x0  Initialization mask: 0xffffffff
 * Access mode: C-RW (control - read/write)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_4  0x40090060
#define SIZE_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_4 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_4  0xffffffff

/*
 *
 * Field: ftdf_on_off_regmap_phySleepWait
 *
 * Time between negate and assert PHY_EN
 * When the signal phy_en is deasserted, it will not be asserted within the time phySleepWait.
 * This time is indicated by the control register phySleepWait (resolution: ~s).
 */
#define IND_F_FTDF_ON_OFF_REGMAP_PHYSLEEPWAIT     IND_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_4
#define MSK_F_FTDF_ON_OFF_REGMAP_PHYSLEEPWAIT     0xff
#define OFF_F_FTDF_ON_OFF_REGMAP_PHYSLEEPWAIT     0
#define SIZE_F_FTDF_ON_OFF_REGMAP_PHYSLEEPWAIT    8
/*
 *
 * Field: ftdf_on_off_regmap_RxPipePropDelay
 *
 * The control register RxPipePropDelay indicates the propagation delay in ~s of the Rx pipeline between the last symbol being captured at the DPHY interface and the "data valid" indication to the LMAC controller.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_RXPIPEPROPDELAY  IND_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_4
#define MSK_F_FTDF_ON_OFF_REGMAP_RXPIPEPROPDELAY  0xff00
#define OFF_F_FTDF_ON_OFF_REGMAP_RXPIPEPROPDELAY  8
#define SIZE_F_FTDF_ON_OFF_REGMAP_RXPIPEPROPDELAY 8
/*
 *
 * Field: ftdf_on_off_regmap_phyAckAttr
 *
 * During transmission of an ACK frame, phyAckAttr is used as phyAttr on the DPHY interface.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_PHYACKATTR       IND_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_4
#define MSK_F_FTDF_ON_OFF_REGMAP_PHYACKATTR       0xffff0000
#define OFF_F_FTDF_ON_OFF_REGMAP_PHYACKATTR       16
#define SIZE_F_FTDF_ON_OFF_REGMAP_PHYACKATTR      16

/*
 *
 * REGISTER: ftdf_on_off_regmap_lmac_control_5
 *
 * Initialization value: 0x8c0  Initialization mask: 0xffff0fff
 * Access mode: C-RW (control - read/write)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_5  0x40090064
#define SIZE_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_5 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_5  0xffff0fff

/*
 *
 * Field: ftdf_on_off_regmap_Ack_Response_Delay
 *
 * In order to have some flexibility the control register Ack_Response_Delay indicates the Acknowledge response time in ~s.
 * The default value shall is 192 ~s (12 symbols).
 */
#define IND_F_FTDF_ON_OFF_REGMAP_ACK_RESPONSE_DELAY  IND_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_5
#define MSK_F_FTDF_ON_OFF_REGMAP_ACK_RESPONSE_DELAY  0xff
#define OFF_F_FTDF_ON_OFF_REGMAP_ACK_RESPONSE_DELAY  0
#define SIZE_F_FTDF_ON_OFF_REGMAP_ACK_RESPONSE_DELAY 8
/*
 *
 * Field: ftdf_on_off_regmap_CcaStatWait
 *
 * The output CCASTAT is valid after 8 symbols + phyRxStartup.
 * The 8 symbols are programmable by control registerCcaStatWait[4] in symbol timesl.
 * Default value is 8d.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_CCASTATWAIT         IND_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_5
#define MSK_F_FTDF_ON_OFF_REGMAP_CCASTATWAIT         0xf00
#define OFF_F_FTDF_ON_OFF_REGMAP_CCASTATWAIT         8
#define SIZE_F_FTDF_ON_OFF_REGMAP_CCASTATWAIT        4
/*
 *
 * Field: ftdf_on_off_regmap_phyCsmaCaAttr
 *
 * The control register phyCsmaCaAttr is used to source PHYATT on the DPHY interface.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_PHYCSMACAATTR       IND_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_5
#define MSK_F_FTDF_ON_OFF_REGMAP_PHYCSMACAATTR       0xffff0000
#define OFF_F_FTDF_ON_OFF_REGMAP_PHYCSMACAATTR       16
#define SIZE_F_FTDF_ON_OFF_REGMAP_PHYCSMACAATTR      16

/*
 *
 * REGISTER: ftdf_on_off_regmap_lmac_control_6
 *
 * Initialization value: 0xc0c28  Initialization mask: 0xffffff
 * Access mode: C-RW (control - read/write)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_6  0x40090068
#define SIZE_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_6 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_6  0xffffff

/*
 *
 * Field: ftdf_on_off_regmap_LifsPeriod
 *
 * The LIFS period is programmable by LifsPeriod (in symbols).
 * The default is 40 symbols (640 ~s),
 */
#define IND_F_FTDF_ON_OFF_REGMAP_LIFSPERIOD   IND_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_6
#define MSK_F_FTDF_ON_OFF_REGMAP_LIFSPERIOD   0xff
#define OFF_F_FTDF_ON_OFF_REGMAP_LIFSPERIOD   0
#define SIZE_F_FTDF_ON_OFF_REGMAP_LIFSPERIOD  8
/*
 *
 * Field: ftdf_on_off_regmap_SifsPeriod
 *
 * The SIFS period is programmable by SifsPeriod (in symbols).
 * The default is 12 symbols (192 ~s).
 */
#define IND_F_FTDF_ON_OFF_REGMAP_SIFSPERIOD   IND_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_6
#define MSK_F_FTDF_ON_OFF_REGMAP_SIFSPERIOD   0xff00
#define OFF_F_FTDF_ON_OFF_REGMAP_SIFSPERIOD   8
#define SIZE_F_FTDF_ON_OFF_REGMAP_SIFSPERIOD  8
/*
 *
 * Field: ftdf_on_off_regmap_WUifsPeriod
 *
 * The WakeUp IFS period is programmable by WUifsPeriod (in symbols).
 * The default is 12 symbols (192 ~s).
 */
#define IND_F_FTDF_ON_OFF_REGMAP_WUIFSPERIOD  IND_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_6
#define MSK_F_FTDF_ON_OFF_REGMAP_WUIFSPERIOD  0xff0000
#define OFF_F_FTDF_ON_OFF_REGMAP_WUIFSPERIOD  16
#define SIZE_F_FTDF_ON_OFF_REGMAP_WUIFSPERIOD 8

/*
 *
 * REGISTER: ftdf_on_off_regmap_lmac_control_11
 *
 * Initialization value: 0x0  Initialization mask: 0x1ffff
 * Access mode: C-RW (control - read/write)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_11  0x4009006c
#define SIZE_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_11 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_11  0x1ffff

/*
 *
 * Field: ftdf_on_off_regmap_macRxTotalCycleTime
 *
 * In order to make it easier to calculate if it is efficient to disable and enable the PHY Rx until the RZ time is reached, a control register indicates the time needed to disable and enable the PHY Rx: macRxTotalCycleTime (resolution in 10 sym)
 */
#define IND_F_FTDF_ON_OFF_REGMAP_MACRXTOTALCYCLETIME  IND_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_11
#define MSK_F_FTDF_ON_OFF_REGMAP_MACRXTOTALCYCLETIME  0xffff
#define OFF_F_FTDF_ON_OFF_REGMAP_MACRXTOTALCYCLETIME  0
#define SIZE_F_FTDF_ON_OFF_REGMAP_MACRXTOTALCYCLETIME 16
/*
 *
 * Bit: ftdf_on_off_regmap_macDisCaRxOfftoRZ
 *
 * This switching off and on of the PHY Rx can be disabled whith the control register macDisCaRxOfftoRZ.
 *  0 : Disabled
 *  1 : Enabled
 */
#define IND_F_FTDF_ON_OFF_REGMAP_MACDISCARXOFFTORZ    IND_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_11
#define MSK_F_FTDF_ON_OFF_REGMAP_MACDISCARXOFFTORZ    0x10000
#define OFF_F_FTDF_ON_OFF_REGMAP_MACDISCARXOFFTORZ    16
#define SIZE_F_FTDF_ON_OFF_REGMAP_MACDISCARXOFFTORZ   1

/*
 *
 * REGISTER: ftdf_on_off_regmap_lmac_control_delta
 *
 * Initialization value: 0x0  Initialization mask: 0x7e
 * Access mode: D-RCW (delta read/clear on write 1)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_DELTA  0x40090070
#define SIZE_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_DELTA 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_DELTA  0x7e

/*
 *
 * Bit: ftdf_on_off_regmap_LmacReady4sleep_d
 *
 * Delta bit for register "LmacReady4sleep"
 */
#define IND_F_FTDF_ON_OFF_REGMAP_LMACREADY4SLEEP_D          IND_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_DELTA
#define MSK_F_FTDF_ON_OFF_REGMAP_LMACREADY4SLEEP_D          0x2
#define OFF_F_FTDF_ON_OFF_REGMAP_LMACREADY4SLEEP_D          1
#define SIZE_F_FTDF_ON_OFF_REGMAP_LMACREADY4SLEEP_D         1
/*
 *
 * Bit: ftdf_on_off_regmap_SyncTimeStamp_e
 *
 * The SyncTimeStamp_e event is set when the TimeStampgenerator is loaded with SyncTimeStampVal.
 * This occurs at the rising edge of lp_clk when SyncTimeStampEna is set and the value of the Event generator is equal to the value SyncTimestampThr.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_SYNCTIMESTAMP_E            IND_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_DELTA
#define MSK_F_FTDF_ON_OFF_REGMAP_SYNCTIMESTAMP_E            0x4
#define OFF_F_FTDF_ON_OFF_REGMAP_SYNCTIMESTAMP_E            2
#define SIZE_F_FTDF_ON_OFF_REGMAP_SYNCTIMESTAMP_E           1
/*
 *
 * Bit: ftdf_on_off_regmap_SymbolTimeThr_e
 *
 * Event that symboltime counter matched SymbolTimeThr
 */
#define IND_F_FTDF_ON_OFF_REGMAP_SYMBOLTIMETHR_E            IND_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_DELTA
#define MSK_F_FTDF_ON_OFF_REGMAP_SYMBOLTIMETHR_E            0x8
#define OFF_F_FTDF_ON_OFF_REGMAP_SYMBOLTIMETHR_E            3
#define SIZE_F_FTDF_ON_OFF_REGMAP_SYMBOLTIMETHR_E           1
/*
 *
 * Bit: ftdf_on_off_regmap_SymbolTime2Thr_e
 *
 * Event that symboltime counter matched SymbolTime2Thr
 */
#define IND_F_FTDF_ON_OFF_REGMAP_SYMBOLTIME2THR_E           IND_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_DELTA
#define MSK_F_FTDF_ON_OFF_REGMAP_SYMBOLTIME2THR_E           0x10
#define OFF_F_FTDF_ON_OFF_REGMAP_SYMBOLTIME2THR_E           4
#define SIZE_F_FTDF_ON_OFF_REGMAP_SYMBOLTIME2THR_E          1
/*
 *
 * Bit: ftdf_on_off_regmap_getGeneratorVal_e
 *
 * Event which indicates that WakeupTimerEnableStatus has changed
 */
#define IND_F_FTDF_ON_OFF_REGMAP_GETGENERATORVAL_E          IND_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_DELTA
#define MSK_F_FTDF_ON_OFF_REGMAP_GETGENERATORVAL_E          0x20
#define OFF_F_FTDF_ON_OFF_REGMAP_GETGENERATORVAL_E          5
#define SIZE_F_FTDF_ON_OFF_REGMAP_GETGENERATORVAL_E         1
/*
 *
 * Bit: ftdf_on_off_regmap_WakeupTimerEnableStatus_d
 *
 * Delta which indicates the getGeneratorVal request is completed
 */
#define IND_F_FTDF_ON_OFF_REGMAP_WAKEUPTIMERENABLESTATUS_D  IND_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_DELTA
#define MSK_F_FTDF_ON_OFF_REGMAP_WAKEUPTIMERENABLESTATUS_D  0x40
#define OFF_F_FTDF_ON_OFF_REGMAP_WAKEUPTIMERENABLESTATUS_D  6
#define SIZE_F_FTDF_ON_OFF_REGMAP_WAKEUPTIMERENABLESTATUS_D 1

/*
 *
 * REGISTER: ftdf_on_off_regmap_lmac_control_mask
 *
 * Initialization value: 0x0  Initialization mask: 0x7e
 * Access mode: DM-RW (delta mask)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_MASK  0x40090080
#define SIZE_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_MASK 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_MASK  0x7e

/*
 *
 * Bit: ftdf_on_off_regmap_LmacReady4sleep_m
 *
 * Mask bit for delta bit "LmacReady4sleep_d"
 */
#define IND_F_FTDF_ON_OFF_REGMAP_LMACREADY4SLEEP_M          IND_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_MASK
#define MSK_F_FTDF_ON_OFF_REGMAP_LMACREADY4SLEEP_M          0x2
#define OFF_F_FTDF_ON_OFF_REGMAP_LMACREADY4SLEEP_M          1
#define SIZE_F_FTDF_ON_OFF_REGMAP_LMACREADY4SLEEP_M         1
/*
 *
 * Bit: ftdf_on_off_regmap_SyncTimeStamp_m
 *
 * Mask bit for event register SyncTimeStamp_e.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_SYNCTIMESTAMP_M            IND_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_MASK
#define MSK_F_FTDF_ON_OFF_REGMAP_SYNCTIMESTAMP_M            0x4
#define OFF_F_FTDF_ON_OFF_REGMAP_SYNCTIMESTAMP_M            2
#define SIZE_F_FTDF_ON_OFF_REGMAP_SYNCTIMESTAMP_M           1
/*
 *
 * Bit: ftdf_on_off_regmap_SymbolTimeThr_m
 *
 * Mask for SymbolTimeThr_e
 */
#define IND_F_FTDF_ON_OFF_REGMAP_SYMBOLTIMETHR_M            IND_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_MASK
#define MSK_F_FTDF_ON_OFF_REGMAP_SYMBOLTIMETHR_M            0x8
#define OFF_F_FTDF_ON_OFF_REGMAP_SYMBOLTIMETHR_M            3
#define SIZE_F_FTDF_ON_OFF_REGMAP_SYMBOLTIMETHR_M           1
/*
 *
 * Bit: ftdf_on_off_regmap_SymbolTime2Thr_m
 *
 * Mask for SymbolTime2Thr_e
 */
#define IND_F_FTDF_ON_OFF_REGMAP_SYMBOLTIME2THR_M           IND_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_MASK
#define MSK_F_FTDF_ON_OFF_REGMAP_SYMBOLTIME2THR_M           0x10
#define OFF_F_FTDF_ON_OFF_REGMAP_SYMBOLTIME2THR_M           4
#define SIZE_F_FTDF_ON_OFF_REGMAP_SYMBOLTIME2THR_M          1
/*
 *
 * Bit: ftdf_on_off_regmap_getGeneratorVal_m
 *
 * Mask for getGeneratorVal_e
 */
#define IND_F_FTDF_ON_OFF_REGMAP_GETGENERATORVAL_M          IND_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_MASK
#define MSK_F_FTDF_ON_OFF_REGMAP_GETGENERATORVAL_M          0x20
#define OFF_F_FTDF_ON_OFF_REGMAP_GETGENERATORVAL_M          5
#define SIZE_F_FTDF_ON_OFF_REGMAP_GETGENERATORVAL_M         1
/*
 *
 * Bit: ftdf_on_off_regmap_WakeupTimerEnableStatus_m
 *
 * Mask for WakeupTimerEnableStatus_d
 */
#define IND_F_FTDF_ON_OFF_REGMAP_WAKEUPTIMERENABLESTATUS_M  IND_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_MASK
#define MSK_F_FTDF_ON_OFF_REGMAP_WAKEUPTIMERENABLESTATUS_M  0x40
#define OFF_F_FTDF_ON_OFF_REGMAP_WAKEUPTIMERENABLESTATUS_M  6
#define SIZE_F_FTDF_ON_OFF_REGMAP_WAKEUPTIMERENABLESTATUS_M 1

/*
 *
 * REGISTER: ftdf_on_off_regmap_lmac_event
 *
 * Initialization value: 0x0  Initialization mask: 0x7
 * Access mode: E-RCW (event read/clear on write 1)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_LMAC_EVENT  0x40090090
#define SIZE_R_FTDF_ON_OFF_REGMAP_LMAC_EVENT 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_LMAC_EVENT  0x7

/*
 *
 * Bit: ftdf_on_off_regmap_EdScanReady_e
 *
 * The event EdScanReady_e is set to notify that the ED scan is ready.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_EDSCANREADY_E     IND_R_FTDF_ON_OFF_REGMAP_LMAC_EVENT
#define MSK_F_FTDF_ON_OFF_REGMAP_EDSCANREADY_E     0x1
#define OFF_F_FTDF_ON_OFF_REGMAP_EDSCANREADY_E     0
#define SIZE_F_FTDF_ON_OFF_REGMAP_EDSCANREADY_E    1
/*
 *
 * Bit: ftdf_on_off_regmap_CCAstat_e
 *
 * If set, the single CCA is ready
 */
#define IND_F_FTDF_ON_OFF_REGMAP_CCASTAT_E         IND_R_FTDF_ON_OFF_REGMAP_LMAC_EVENT
#define MSK_F_FTDF_ON_OFF_REGMAP_CCASTAT_E         0x2
#define OFF_F_FTDF_ON_OFF_REGMAP_CCASTAT_E         1
#define SIZE_F_FTDF_ON_OFF_REGMAP_CCASTAT_E        1
/*
 *
 * Bit: ftdf_on_off_regmap_RxTimerExpired_e
 *
 * Set if one of the timers enabling the RX-ON mode expires without having received any valid frame
 */
#define IND_F_FTDF_ON_OFF_REGMAP_RXTIMEREXPIRED_E  IND_R_FTDF_ON_OFF_REGMAP_LMAC_EVENT
#define MSK_F_FTDF_ON_OFF_REGMAP_RXTIMEREXPIRED_E  0x4
#define OFF_F_FTDF_ON_OFF_REGMAP_RXTIMEREXPIRED_E  2
#define SIZE_F_FTDF_ON_OFF_REGMAP_RXTIMEREXPIRED_E 1

/*
 *
 * REGISTER: ftdf_on_off_regmap_lmac_mask
 *
 * Initialization value: 0x0  Initialization mask: 0x7
 * Access mode: EM-RW (event mask)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_LMAC_MASK  0x40090094
#define SIZE_R_FTDF_ON_OFF_REGMAP_LMAC_MASK 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_LMAC_MASK  0x7

/*
 *
 * Bit: ftdf_on_off_regmap_EdScanReady_m
 *
 * Mask bit for event "EdScanReady_e"
 */
#define IND_F_FTDF_ON_OFF_REGMAP_EDSCANREADY_M     IND_R_FTDF_ON_OFF_REGMAP_LMAC_MASK
#define MSK_F_FTDF_ON_OFF_REGMAP_EDSCANREADY_M     0x1
#define OFF_F_FTDF_ON_OFF_REGMAP_EDSCANREADY_M     0
#define SIZE_F_FTDF_ON_OFF_REGMAP_EDSCANREADY_M    1
/*
 *
 * Bit: ftdf_on_off_regmap_CCAstat_m
 *
 * Mask bit for event "CCAstat_e"
 */
#define IND_F_FTDF_ON_OFF_REGMAP_CCASTAT_M         IND_R_FTDF_ON_OFF_REGMAP_LMAC_MASK
#define MSK_F_FTDF_ON_OFF_REGMAP_CCASTAT_M         0x2
#define OFF_F_FTDF_ON_OFF_REGMAP_CCASTAT_M         1
#define SIZE_F_FTDF_ON_OFF_REGMAP_CCASTAT_M        1
/*
 *
 * Bit: ftdf_on_off_regmap_RxTimerExpired_m
 *
 * Mask bit for event "RxTimerExpired_e"
 */
#define IND_F_FTDF_ON_OFF_REGMAP_RXTIMEREXPIRED_M  IND_R_FTDF_ON_OFF_REGMAP_LMAC_MASK
#define MSK_F_FTDF_ON_OFF_REGMAP_RXTIMEREXPIRED_M  0x4
#define OFF_F_FTDF_ON_OFF_REGMAP_RXTIMEREXPIRED_M  2
#define SIZE_F_FTDF_ON_OFF_REGMAP_RXTIMEREXPIRED_M 1

/*
 *
 * REGISTER: ftdf_on_off_regmap_lmac_manual_1
 *
 * Initialization value: 0x0  Initialization mask: 0xffff0fff
 * Access mode: C-RW (control - read/write)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_LMAC_MANUAL_1  0x400900a0
#define SIZE_R_FTDF_ON_OFF_REGMAP_LMAC_MANUAL_1 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_LMAC_MANUAL_1  0xffff0fff

/*
 *
 * Bit: ftdf_on_off_regmap_lmac_manual_mode
 *
 * If the control register lmac_manual_mode is set, the LMAC controller control
 * signals should be controlled by the lmac_manual_control registers
 */
#define IND_F_FTDF_ON_OFF_REGMAP_LMAC_MANUAL_MODE        IND_R_FTDF_ON_OFF_REGMAP_LMAC_MANUAL_1
#define MSK_F_FTDF_ON_OFF_REGMAP_LMAC_MANUAL_MODE        0x1
#define OFF_F_FTDF_ON_OFF_REGMAP_LMAC_MANUAL_MODE        0
#define SIZE_F_FTDF_ON_OFF_REGMAP_LMAC_MANUAL_MODE       1
/*
 *
 * Bit: ftdf_on_off_regmap_lmac_manual_phy_en
 *
 * lmac_manual_phy_en controls the PHY_EN interface signal when lmac_manual_mode is set
 */
#define IND_F_FTDF_ON_OFF_REGMAP_LMAC_MANUAL_PHY_EN      IND_R_FTDF_ON_OFF_REGMAP_LMAC_MANUAL_1
#define MSK_F_FTDF_ON_OFF_REGMAP_LMAC_MANUAL_PHY_EN      0x2
#define OFF_F_FTDF_ON_OFF_REGMAP_LMAC_MANUAL_PHY_EN      1
#define SIZE_F_FTDF_ON_OFF_REGMAP_LMAC_MANUAL_PHY_EN     1
/*
 *
 * Bit: ftdf_on_off_regmap_lmac_manual_tx_en
 *
 * lmac_manual_tx_en controls the TX_EN interface signal when lmac_manual_mode is set
 */
#define IND_F_FTDF_ON_OFF_REGMAP_LMAC_MANUAL_TX_EN       IND_R_FTDF_ON_OFF_REGMAP_LMAC_MANUAL_1
#define MSK_F_FTDF_ON_OFF_REGMAP_LMAC_MANUAL_TX_EN       0x4
#define OFF_F_FTDF_ON_OFF_REGMAP_LMAC_MANUAL_TX_EN       2
#define SIZE_F_FTDF_ON_OFF_REGMAP_LMAC_MANUAL_TX_EN      1
/*
 *
 * Bit: ftdf_on_off_regmap_lmac_manual_rx_en
 *
 * lmac_manual_rx_en controls the RX_EN interface signal when lmac_manual_mode is set
 */
#define IND_F_FTDF_ON_OFF_REGMAP_LMAC_MANUAL_RX_EN       IND_R_FTDF_ON_OFF_REGMAP_LMAC_MANUAL_1
#define MSK_F_FTDF_ON_OFF_REGMAP_LMAC_MANUAL_RX_EN       0x8
#define OFF_F_FTDF_ON_OFF_REGMAP_LMAC_MANUAL_RX_EN       3
#define SIZE_F_FTDF_ON_OFF_REGMAP_LMAC_MANUAL_RX_EN      1
/*
 *
 * Bit: ftdf_on_off_regmap_lmac_manual_rx_pipe_en
 *
 * lmac_manual_rx_pipe_en controls the rx_enable signal towards the rx pipeline when lmac_manual_mode is set
 */
#define IND_F_FTDF_ON_OFF_REGMAP_LMAC_MANUAL_RX_PIPE_EN  IND_R_FTDF_ON_OFF_REGMAP_LMAC_MANUAL_1
#define MSK_F_FTDF_ON_OFF_REGMAP_LMAC_MANUAL_RX_PIPE_EN  0x10
#define OFF_F_FTDF_ON_OFF_REGMAP_LMAC_MANUAL_RX_PIPE_EN  4
#define SIZE_F_FTDF_ON_OFF_REGMAP_LMAC_MANUAL_RX_PIPE_EN 1
/*
 *
 * Bit: ftdf_on_off_regmap_lmac_manual_ed_request
 *
 * lmac_manual_ed_request controls the ED_REQUEST interface signal when lmac_manual_mode is set
 */
#define IND_F_FTDF_ON_OFF_REGMAP_LMAC_MANUAL_ED_REQUEST  IND_R_FTDF_ON_OFF_REGMAP_LMAC_MANUAL_1
#define MSK_F_FTDF_ON_OFF_REGMAP_LMAC_MANUAL_ED_REQUEST  0x20
#define OFF_F_FTDF_ON_OFF_REGMAP_LMAC_MANUAL_ED_REQUEST  5
#define SIZE_F_FTDF_ON_OFF_REGMAP_LMAC_MANUAL_ED_REQUEST 1
/*
 *
 * Field: ftdf_on_off_regmap_lmac_manual_tx_frm_nr
 *
 * lmac_manual_tx_frm_nr controls the entry in the tx buffer to be transmitted.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_LMAC_MANUAL_TX_FRM_NR   IND_R_FTDF_ON_OFF_REGMAP_LMAC_MANUAL_1
#define MSK_F_FTDF_ON_OFF_REGMAP_LMAC_MANUAL_TX_FRM_NR   0xc0
#define OFF_F_FTDF_ON_OFF_REGMAP_LMAC_MANUAL_TX_FRM_NR   6
#define SIZE_F_FTDF_ON_OFF_REGMAP_LMAC_MANUAL_TX_FRM_NR  2
/*
 *
 * Field: ftdf_on_off_regmap_lmac_manual_pti
 *
 * lmac_manual_pti controls the PTI interface signal when lmac_manual_mode is set
 */
#define IND_F_FTDF_ON_OFF_REGMAP_LMAC_MANUAL_PTI         IND_R_FTDF_ON_OFF_REGMAP_LMAC_MANUAL_1
#define MSK_F_FTDF_ON_OFF_REGMAP_LMAC_MANUAL_PTI         0xf00
#define OFF_F_FTDF_ON_OFF_REGMAP_LMAC_MANUAL_PTI         8
#define SIZE_F_FTDF_ON_OFF_REGMAP_LMAC_MANUAL_PTI        4
/*
 *
 * Field: ftdf_on_off_regmap_lmac_manual_phy_attr
 *
 * lmac_manual_phy_attr controls the PHY_ATTR interface signal when lmac_manual_mode is set
 */
#define IND_F_FTDF_ON_OFF_REGMAP_LMAC_MANUAL_PHY_ATTR    IND_R_FTDF_ON_OFF_REGMAP_LMAC_MANUAL_1
#define MSK_F_FTDF_ON_OFF_REGMAP_LMAC_MANUAL_PHY_ATTR    0xffff0000
#define OFF_F_FTDF_ON_OFF_REGMAP_LMAC_MANUAL_PHY_ATTR    16
#define SIZE_F_FTDF_ON_OFF_REGMAP_LMAC_MANUAL_PHY_ATTR   16

/*
 *
 * REGISTER: ftdf_on_off_regmap_lmac_manual_os
 *
 * Initialization value: 0x0  Initialization mask: 0x1
 * Access mode: OS-W (one-shot - write)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_LMAC_MANUAL_OS  0x400900a4
#define SIZE_R_FTDF_ON_OFF_REGMAP_LMAC_MANUAL_OS 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_LMAC_MANUAL_OS  0x1

/*
 *
 * Bit: ftdf_on_off_regmap_lmac_manual_tx_start
 *
 * One shot register which triggers the transmission of a frame from the tx buffer in lmac_manual_mode
 */
#define IND_F_FTDF_ON_OFF_REGMAP_LMAC_MANUAL_TX_START  IND_R_FTDF_ON_OFF_REGMAP_LMAC_MANUAL_OS
#define MSK_F_FTDF_ON_OFF_REGMAP_LMAC_MANUAL_TX_START  0x1
#define OFF_F_FTDF_ON_OFF_REGMAP_LMAC_MANUAL_TX_START  0
#define SIZE_F_FTDF_ON_OFF_REGMAP_LMAC_MANUAL_TX_START 1

/*
 *
 * REGISTER: ftdf_on_off_regmap_lmac_manual_status
 *
 * Initialization value: none.
 * Access mode: S-R (state - read)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_LMAC_MANUAL_STATUS  0x400900a8
#define SIZE_R_FTDF_ON_OFF_REGMAP_LMAC_MANUAL_STATUS 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_LMAC_MANUAL_STATUS  0xff01

/*
 *
 * Bit: ftdf_on_off_regmap_lmac_manual_cca_stat
 *
 * lmac_manual_cca_stat shows the status of the CCA_STAT
 */
#define IND_F_FTDF_ON_OFF_REGMAP_LMAC_MANUAL_CCA_STAT  IND_R_FTDF_ON_OFF_REGMAP_LMAC_MANUAL_STATUS
#define MSK_F_FTDF_ON_OFF_REGMAP_LMAC_MANUAL_CCA_STAT  0x1
#define OFF_F_FTDF_ON_OFF_REGMAP_LMAC_MANUAL_CCA_STAT  0
#define SIZE_F_FTDF_ON_OFF_REGMAP_LMAC_MANUAL_CCA_STAT 1
/*
 *
 * Field: ftdf_on_off_regmap_lmac_manual_ed_stat
 *
 * lmac_manual_ed_stat shows the status of the ED_STAT interface signal.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_LMAC_MANUAL_ED_STAT   IND_R_FTDF_ON_OFF_REGMAP_LMAC_MANUAL_STATUS
#define MSK_F_FTDF_ON_OFF_REGMAP_LMAC_MANUAL_ED_STAT   0xff00
#define OFF_F_FTDF_ON_OFF_REGMAP_LMAC_MANUAL_ED_STAT   8
#define SIZE_F_FTDF_ON_OFF_REGMAP_LMAC_MANUAL_ED_STAT  8

/*
 *
 * REGISTER: ftdf_on_off_regmap_lmac_control_7
 *
 * Initialization value: 0x420000  Initialization mask: 0xffffffff
 * Access mode: C-RW (control - read/write)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_7  0x40090100
#define SIZE_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_7 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_7  0xffffffff

/*
 *
 * Field: ftdf_on_off_regmap_macWUPeriod
 *
 * Wake-up duration in symbols.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_MACWUPERIOD         IND_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_7
#define MSK_F_FTDF_ON_OFF_REGMAP_MACWUPERIOD         0xffff
#define OFF_F_FTDF_ON_OFF_REGMAP_MACWUPERIOD         0
#define SIZE_F_FTDF_ON_OFF_REGMAP_MACWUPERIOD        16
/*
 *
 * Field: ftdf_on_off_regmap_macCSLsamplePeriod
 *
 * When performing a idle listening, the receiver is enabled for at least macCSLsamplePeriod (in symbols).
 */
#define IND_F_FTDF_ON_OFF_REGMAP_MACCSLSAMPLEPERIOD  IND_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_7
#define MSK_F_FTDF_ON_OFF_REGMAP_MACCSLSAMPLEPERIOD  0xffff0000
#define OFF_F_FTDF_ON_OFF_REGMAP_MACCSLSAMPLEPERIOD  16
#define SIZE_F_FTDF_ON_OFF_REGMAP_MACCSLSAMPLEPERIOD 16

/*
 *
 * REGISTER: ftdf_on_off_regmap_lmac_control_8
 *
 * Initialization value: 0x0  Initialization mask: 0xffffffff
 * Access mode: C-RW (control - read/write)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_8  0x40090104
#define SIZE_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_8 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_8  0xffffffff

/*
 *
 * Field: ftdf_on_off_regmap_macCSLstartSampleTime
 *
 * The control register macCSLstartSampleTime indicates the TimeStamp generator time (in symbols) when to start listening (called "idle listening").
 */
#define IND_F_FTDF_ON_OFF_REGMAP_MACCSLSTARTSAMPLETIME  IND_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_8
#define MSK_F_FTDF_ON_OFF_REGMAP_MACCSLSTARTSAMPLETIME  0xffffffff
#define OFF_F_FTDF_ON_OFF_REGMAP_MACCSLSTARTSAMPLETIME  0
#define SIZE_F_FTDF_ON_OFF_REGMAP_MACCSLSTARTSAMPLETIME 32

/*
 *
 * REGISTER: ftdf_on_off_regmap_lmac_control_9
 *
 * Initialization value: 0x42  Initialization mask: 0xffffffff
 * Access mode: C-RW (control - read/write)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_9  0x40090108
#define SIZE_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_9 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_9  0xffffffff

/*
 *
 * Field: ftdf_on_off_regmap_macCSLdataPeriod
 *
 * After the wake-up sequence a frame is expected, the receiver will be enabled for at least a period of macCSLdataPeriod (in symbols).
 */
#define IND_F_FTDF_ON_OFF_REGMAP_MACCSLDATAPERIOD         IND_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_9
#define MSK_F_FTDF_ON_OFF_REGMAP_MACCSLDATAPERIOD         0xffff
#define OFF_F_FTDF_ON_OFF_REGMAP_MACCSLDATAPERIOD         0
#define SIZE_F_FTDF_ON_OFF_REGMAP_MACCSLDATAPERIOD        16
/*
 *
 * Field: ftdf_on_off_regmap_macCSLFramePendingWaitT
 *
 * If a non Wake-up frame with Frame Pending bit = '1' is received, the receiver is enabled for at least an extra period of macCSLFramePendingWaitT (in symbols) after the end of the received frame.
 * The time the Enhanced ACK transmission lasts (if applicable) is included in this time.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_MACCSLFRAMEPENDINGWAITT  IND_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_9
#define MSK_F_FTDF_ON_OFF_REGMAP_MACCSLFRAMEPENDINGWAITT  0xffff0000
#define OFF_F_FTDF_ON_OFF_REGMAP_MACCSLFRAMEPENDINGWAITT  16
#define SIZE_F_FTDF_ON_OFF_REGMAP_MACCSLFRAMEPENDINGWAITT 16

/*
 *
 * REGISTER: ftdf_on_off_regmap_lmac_control_10
 *
 * Initialization value: 0x20000000  Initialization mask: 0xf00f00ff
 * Access mode: C-RW (control - read/write)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_10  0x4009010c
#define SIZE_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_10 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_10  0xf00f00ff

/*
 *
 * Field: ftdf_on_off_regmap_macRZzeroVal
 *
 * If the current RZtime is less or Equal to macRZzeroVal an RZtime with value zero is inserted in the wakeup frame
 */
#define IND_F_FTDF_ON_OFF_REGMAP_MACRZZEROVAL       IND_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_10
#define MSK_F_FTDF_ON_OFF_REGMAP_MACRZZEROVAL       0xf0000000
#define OFF_F_FTDF_ON_OFF_REGMAP_MACRZZEROVAL       28
#define SIZE_F_FTDF_ON_OFF_REGMAP_MACRZZEROVAL      4
/*
 *
 * Field: ftdf_on_off_regmap_macCSLmarginRZ
 *
 * The UMAC can set the margin for the expected frame by control register macCSLmarginRZ (in 10 sym).
 * So the LMAC will make sure the receiver is ready to receive data this amount of time earlier than to be expected by the received RZ time
 */
#define IND_F_FTDF_ON_OFF_REGMAP_MACCSLMARGINRZ     IND_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_10
#define MSK_F_FTDF_ON_OFF_REGMAP_MACCSLMARGINRZ     0xf0000
#define OFF_F_FTDF_ON_OFF_REGMAP_MACCSLMARGINRZ     16
#define SIZE_F_FTDF_ON_OFF_REGMAP_MACCSLMARGINRZ    4
/*
 *
 * Field: ftdf_on_off_regmap_macWURZCorrection
 *
 * This register shall be used if the Wake-up frame to be transmitted is larger than 15 octets.
 * It shall indicate the amount of extra data in a Wake-up frame after the RZ position in the frame (in 10 sym).
 */
#define IND_F_FTDF_ON_OFF_REGMAP_MACWURZCORRECTION  IND_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_10
#define MSK_F_FTDF_ON_OFF_REGMAP_MACWURZCORRECTION  0xff
#define OFF_F_FTDF_ON_OFF_REGMAP_MACWURZCORRECTION  0
#define SIZE_F_FTDF_ON_OFF_REGMAP_MACWURZCORRECTION 8

/*
 *
 * REGISTER: ftdf_on_off_regmap_security_0
 *
 * Initialization value: 0x0  Initialization mask: 0xff7f0f02
 * Access mode: C-RW (control - read/write)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_SECURITY_0  0x40090110
#define SIZE_R_FTDF_ON_OFF_REGMAP_SECURITY_0 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_SECURITY_0  0xff7f0f02

/*
 *
 * Bit: ftdf_on_off_regmap_secTxRxn
 *
 * See register "secEntry"
 */
#define IND_F_FTDF_ON_OFF_REGMAP_SECTXRXN    IND_R_FTDF_ON_OFF_REGMAP_SECURITY_0
#define MSK_F_FTDF_ON_OFF_REGMAP_SECTXRXN    0x2
#define OFF_F_FTDF_ON_OFF_REGMAP_SECTXRXN    1
#define SIZE_F_FTDF_ON_OFF_REGMAP_SECTXRXN   1
/*
 *
 * Field: ftdf_on_off_regmap_secEntry
 *
 * The UMAC shall indicate by control registers secEntry and secTxRxn which entry to use and if it's from the Tx or Rx buffer ('1' resp. '0').
 */
#define IND_F_FTDF_ON_OFF_REGMAP_SECENTRY    IND_R_FTDF_ON_OFF_REGMAP_SECURITY_0
#define MSK_F_FTDF_ON_OFF_REGMAP_SECENTRY    0xf00
#define OFF_F_FTDF_ON_OFF_REGMAP_SECENTRY    8
#define SIZE_F_FTDF_ON_OFF_REGMAP_SECENTRY   4
/*
 *
 * Field: ftdf_on_off_regmap_secAlength
 *
 * The length of the a_data is indicated by control register secAlength.
 * The end of the a_data is the start point of the m_data. (So secAlength must also be set if security level==4)
 */
#define IND_F_FTDF_ON_OFF_REGMAP_SECALENGTH  IND_R_FTDF_ON_OFF_REGMAP_SECURITY_0
#define MSK_F_FTDF_ON_OFF_REGMAP_SECALENGTH  0x7f0000
#define OFF_F_FTDF_ON_OFF_REGMAP_SECALENGTH  16
#define SIZE_F_FTDF_ON_OFF_REGMAP_SECALENGTH 7
/*
 *
 * Field: ftdf_on_off_regmap_secMlength
 *
 * The length of the m_data is indicated by control register secMlength.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_SECMLENGTH  IND_R_FTDF_ON_OFF_REGMAP_SECURITY_0
#define MSK_F_FTDF_ON_OFF_REGMAP_SECMLENGTH  0x7f000000
#define OFF_F_FTDF_ON_OFF_REGMAP_SECMLENGTH  24
#define SIZE_F_FTDF_ON_OFF_REGMAP_SECMLENGTH 7
/*
 *
 * Bit: ftdf_on_off_regmap_secEncDecn
 *
 * The control register secEncDecn indicates whether to encrypt ('1') or decrypt ('0') the data.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_SECENCDECN  IND_R_FTDF_ON_OFF_REGMAP_SECURITY_0
#define MSK_F_FTDF_ON_OFF_REGMAP_SECENCDECN  0x80000000
#define OFF_F_FTDF_ON_OFF_REGMAP_SECENCDECN  31
#define SIZE_F_FTDF_ON_OFF_REGMAP_SECENCDECN 1

/*
 *
 * REGISTER: ftdf_on_off_regmap_security_1
 *
 * Initialization value: 0x0  Initialization mask: 0xffff
 * Access mode: C-RW (control - read/write)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_SECURITY_1  0x40090114
#define SIZE_R_FTDF_ON_OFF_REGMAP_SECURITY_1 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_SECURITY_1  0xffff

/*
 *
 * Field: ftdf_on_off_regmap_secAuthFlags
 *
 * Register secAuthFlags contains the authentication flags fields.
 * bit[7] is '0'
 * bit[6] is "A_data present"
 * bit[5:3]: 3-bit security level of m_data
 * bit[2:0]: 3-bit security level of a_data.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_SECAUTHFLAGS  IND_R_FTDF_ON_OFF_REGMAP_SECURITY_1
#define MSK_F_FTDF_ON_OFF_REGMAP_SECAUTHFLAGS  0xff
#define OFF_F_FTDF_ON_OFF_REGMAP_SECAUTHFLAGS  0
#define SIZE_F_FTDF_ON_OFF_REGMAP_SECAUTHFLAGS 8
/*
 *
 * Field: ftdf_on_off_regmap_secEncrFlags
 *
 * Register secEncrFlags contain the encryption flags field.
 * Bits [2:0] are the 3-bit encoding flags of a_data, the other bits msut be set to '0'.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_SECENCRFLAGS  IND_R_FTDF_ON_OFF_REGMAP_SECURITY_1
#define MSK_F_FTDF_ON_OFF_REGMAP_SECENCRFLAGS  0xff00
#define OFF_F_FTDF_ON_OFF_REGMAP_SECENCRFLAGS  8
#define SIZE_F_FTDF_ON_OFF_REGMAP_SECENCRFLAGS 8

/*
 *
 * REGISTER: ftdf_on_off_regmap_secKey_0
 *
 * Initialization value: 0x0  Initialization mask: 0xffffffff
 * Access mode: C-RW (control - read/write)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_SECKEY_0  0x40090118
#define SIZE_R_FTDF_ON_OFF_REGMAP_SECKEY_0 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_SECKEY_0  0xffffffff

/*
 *
 * Field: ftdf_on_off_regmap_secKey_0
 *
 * Registers secKey[0..3]  contain the key to be used.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_SECKEY_0  IND_R_FTDF_ON_OFF_REGMAP_SECKEY_0
#define MSK_F_FTDF_ON_OFF_REGMAP_SECKEY_0  0xffffffff
#define OFF_F_FTDF_ON_OFF_REGMAP_SECKEY_0  0
#define SIZE_F_FTDF_ON_OFF_REGMAP_SECKEY_0 32

/*
 *
 * REGISTER: ftdf_on_off_regmap_secKey_1
 *
 * Initialization value: 0x0  Initialization mask: 0xffffffff
 * Access mode: C-RW (control - read/write)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_SECKEY_1  0x4009011c
#define SIZE_R_FTDF_ON_OFF_REGMAP_SECKEY_1 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_SECKEY_1  0xffffffff

/*
 *
 * Field: ftdf_on_off_regmap_secKey_1
 *
 * See register "secKey_0"
 */
#define IND_F_FTDF_ON_OFF_REGMAP_SECKEY_1  IND_R_FTDF_ON_OFF_REGMAP_SECKEY_1
#define MSK_F_FTDF_ON_OFF_REGMAP_SECKEY_1  0xffffffff
#define OFF_F_FTDF_ON_OFF_REGMAP_SECKEY_1  0
#define SIZE_F_FTDF_ON_OFF_REGMAP_SECKEY_1 32

/*
 *
 * REGISTER: ftdf_on_off_regmap_secKey_2
 *
 * Initialization value: 0x0  Initialization mask: 0xffffffff
 * Access mode: C-RW (control - read/write)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_SECKEY_2  0x40090120
#define SIZE_R_FTDF_ON_OFF_REGMAP_SECKEY_2 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_SECKEY_2  0xffffffff

/*
 *
 * Field: ftdf_on_off_regmap_secKey_2
 *
 * See register "secKey_0"
 */
#define IND_F_FTDF_ON_OFF_REGMAP_SECKEY_2  IND_R_FTDF_ON_OFF_REGMAP_SECKEY_2
#define MSK_F_FTDF_ON_OFF_REGMAP_SECKEY_2  0xffffffff
#define OFF_F_FTDF_ON_OFF_REGMAP_SECKEY_2  0
#define SIZE_F_FTDF_ON_OFF_REGMAP_SECKEY_2 32

/*
 *
 * REGISTER: ftdf_on_off_regmap_secKey_3
 *
 * Initialization value: 0x0  Initialization mask: 0xffffffff
 * Access mode: C-RW (control - read/write)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_SECKEY_3  0x40090124
#define SIZE_R_FTDF_ON_OFF_REGMAP_SECKEY_3 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_SECKEY_3  0xffffffff

/*
 *
 * Field: ftdf_on_off_regmap_secKey_3
 *
 * See register "secKey_0"
 */
#define IND_F_FTDF_ON_OFF_REGMAP_SECKEY_3  IND_R_FTDF_ON_OFF_REGMAP_SECKEY_3
#define MSK_F_FTDF_ON_OFF_REGMAP_SECKEY_3  0xffffffff
#define OFF_F_FTDF_ON_OFF_REGMAP_SECKEY_3  0
#define SIZE_F_FTDF_ON_OFF_REGMAP_SECKEY_3 32

/*
 *
 * REGISTER: ftdf_on_off_regmap_secNonce_0
 *
 * Initialization value: 0x0  Initialization mask: 0xffffffff
 * Access mode: C-RW (control - read/write)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_SECNONCE_0  0x40090128
#define SIZE_R_FTDF_ON_OFF_REGMAP_SECNONCE_0 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_SECNONCE_0  0xffffffff

/*
 *
 * Field: ftdf_on_off_regmap_secNonce_0
 *
 * Register secNonce[0..3] contains the Nonce to be used for encryption/decryption.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_SECNONCE_0  IND_R_FTDF_ON_OFF_REGMAP_SECNONCE_0
#define MSK_F_FTDF_ON_OFF_REGMAP_SECNONCE_0  0xffffffff
#define OFF_F_FTDF_ON_OFF_REGMAP_SECNONCE_0  0
#define SIZE_F_FTDF_ON_OFF_REGMAP_SECNONCE_0 32

/*
 *
 * REGISTER: ftdf_on_off_regmap_secNonce_1
 *
 * Initialization value: 0x0  Initialization mask: 0xffffffff
 * Access mode: C-RW (control - read/write)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_SECNONCE_1  0x4009012c
#define SIZE_R_FTDF_ON_OFF_REGMAP_SECNONCE_1 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_SECNONCE_1  0xffffffff

/*
 *
 * Field: ftdf_on_off_regmap_secNonce_1
 *
 * See register "Nonce_0"
 */
#define IND_F_FTDF_ON_OFF_REGMAP_SECNONCE_1  IND_R_FTDF_ON_OFF_REGMAP_SECNONCE_1
#define MSK_F_FTDF_ON_OFF_REGMAP_SECNONCE_1  0xffffffff
#define OFF_F_FTDF_ON_OFF_REGMAP_SECNONCE_1  0
#define SIZE_F_FTDF_ON_OFF_REGMAP_SECNONCE_1 32

/*
 *
 * REGISTER: ftdf_on_off_regmap_secNonce_2
 *
 * Initialization value: 0x0  Initialization mask: 0xffffffff
 * Access mode: C-RW (control - read/write)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_SECNONCE_2  0x40090130
#define SIZE_R_FTDF_ON_OFF_REGMAP_SECNONCE_2 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_SECNONCE_2  0xffffffff

/*
 *
 * Field: ftdf_on_off_regmap_secNonce_2
 *
 * See register "Nonce_0"
 */
#define IND_F_FTDF_ON_OFF_REGMAP_SECNONCE_2  IND_R_FTDF_ON_OFF_REGMAP_SECNONCE_2
#define MSK_F_FTDF_ON_OFF_REGMAP_SECNONCE_2  0xffffffff
#define OFF_F_FTDF_ON_OFF_REGMAP_SECNONCE_2  0
#define SIZE_F_FTDF_ON_OFF_REGMAP_SECNONCE_2 32

/*
 *
 * REGISTER: ftdf_on_off_regmap_secNonce_3
 *
 * Initialization value: 0x0  Initialization mask: 0xff
 * Access mode: C-RW (control - read/write)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_SECNONCE_3  0x40090134
#define SIZE_R_FTDF_ON_OFF_REGMAP_SECNONCE_3 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_SECNONCE_3  0xff

/*
 *
 * Field: ftdf_on_off_regmap_secNonce_3
 *
 * See register "Nonce_0"
 */
#define IND_F_FTDF_ON_OFF_REGMAP_SECNONCE_3  IND_R_FTDF_ON_OFF_REGMAP_SECNONCE_3
#define MSK_F_FTDF_ON_OFF_REGMAP_SECNONCE_3  0xff
#define OFF_F_FTDF_ON_OFF_REGMAP_SECNONCE_3  0
#define SIZE_F_FTDF_ON_OFF_REGMAP_SECNONCE_3 8

/*
 *
 * REGISTER: ftdf_on_off_regmap_security_os
 *
 * Initialization value: 0x0  Initialization mask: 0x3
 * Access mode: OS-W (one-shot - write)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_SECURITY_OS  0x40090138
#define SIZE_R_FTDF_ON_OFF_REGMAP_SECURITY_OS 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_SECURITY_OS  0x3

/*
 *
 * Bit: ftdf_on_off_regmap_secAbort
 *
 * See register "Nonce_0"
 */
#define IND_F_FTDF_ON_OFF_REGMAP_SECABORT  IND_R_FTDF_ON_OFF_REGMAP_SECURITY_OS
#define MSK_F_FTDF_ON_OFF_REGMAP_SECABORT  0x1
#define OFF_F_FTDF_ON_OFF_REGMAP_SECABORT  0
#define SIZE_F_FTDF_ON_OFF_REGMAP_SECABORT 1
/*
 *
 * Bit: ftdf_on_off_regmap_secStart
 *
 * One_shot register to start the encryption, decryption and authentication support task.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_SECSTART  IND_R_FTDF_ON_OFF_REGMAP_SECURITY_OS
#define MSK_F_FTDF_ON_OFF_REGMAP_SECSTART  0x2
#define OFF_F_FTDF_ON_OFF_REGMAP_SECSTART  1
#define SIZE_F_FTDF_ON_OFF_REGMAP_SECSTART 1

/*
 *
 * REGISTER: ftdf_on_off_regmap_security_status
 *
 * Initialization value: none.
 * Access mode: S-R (state - read)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_SECURITY_STATUS  0x40090140
#define SIZE_R_FTDF_ON_OFF_REGMAP_SECURITY_STATUS 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_SECURITY_STATUS  0x3

/*
 *
 * Bit: ftdf_on_off_regmap_secBusy
 *
 * Register "secBusy" indicates if the encryption/decryption process is still running.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_SECBUSY      IND_R_FTDF_ON_OFF_REGMAP_SECURITY_STATUS
#define MSK_F_FTDF_ON_OFF_REGMAP_SECBUSY      0x1
#define OFF_F_FTDF_ON_OFF_REGMAP_SECBUSY      0
#define SIZE_F_FTDF_ON_OFF_REGMAP_SECBUSY     1
/*
 *
 * Bit: ftdf_on_off_regmap_secAuthFail
 *
 * In case of decryption, the status bit secAuthFail will be set when the authentication has failed.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_SECAUTHFAIL  IND_R_FTDF_ON_OFF_REGMAP_SECURITY_STATUS
#define MSK_F_FTDF_ON_OFF_REGMAP_SECAUTHFAIL  0x2
#define OFF_F_FTDF_ON_OFF_REGMAP_SECAUTHFAIL  1
#define SIZE_F_FTDF_ON_OFF_REGMAP_SECAUTHFAIL 1

/*
 *
 * REGISTER: ftdf_on_off_regmap_security_event
 *
 * Initialization value: 0x0  Initialization mask: 0x1
 * Access mode: E-RCW (event read/clear on write 1)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_SECURITY_EVENT  0x40090150
#define SIZE_R_FTDF_ON_OFF_REGMAP_SECURITY_EVENT 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_SECURITY_EVENT  0x1

/*
 *
 * Bit: ftdf_on_off_regmap_secReady_e
 *
 * The Event bit secReady_e is set when the authentication process is ready (i.e. secBusy is cleared).
 * This Event shall contribute to the gen_irq.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_SECREADY_E  IND_R_FTDF_ON_OFF_REGMAP_SECURITY_EVENT
#define MSK_F_FTDF_ON_OFF_REGMAP_SECREADY_E  0x1
#define OFF_F_FTDF_ON_OFF_REGMAP_SECREADY_E  0
#define SIZE_F_FTDF_ON_OFF_REGMAP_SECREADY_E 1

/*
 *
 * REGISTER: ftdf_on_off_regmap_security_eventmask
 *
 * Initialization value: 0x0  Initialization mask: 0x1
 * Access mode: EM-RW (event mask)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_SECURITY_EVENTMASK  0x40090154
#define SIZE_R_FTDF_ON_OFF_REGMAP_SECURITY_EVENTMASK 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_SECURITY_EVENTMASK  0x1

/*
 *
 * Bit: ftdf_on_off_regmap_secReady_m
 *
 * Mask bit for event "secReady_e".
 */
#define IND_F_FTDF_ON_OFF_REGMAP_SECREADY_M  IND_R_FTDF_ON_OFF_REGMAP_SECURITY_EVENTMASK
#define MSK_F_FTDF_ON_OFF_REGMAP_SECREADY_M  0x1
#define OFF_F_FTDF_ON_OFF_REGMAP_SECREADY_M  0
#define SIZE_F_FTDF_ON_OFF_REGMAP_SECREADY_M 1

/*
 *
 * REGISTER: ftdf_on_off_regmap_tsch_control_0
 *
 * Initialization value: 0x89803e8  Initialization mask: 0xffffffff
 * Access mode: C-RW (control - read/write)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_TSCH_CONTROL_0  0x40090160
#define SIZE_R_FTDF_ON_OFF_REGMAP_TSCH_CONTROL_0 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_TSCH_CONTROL_0  0xffffffff

/*
 *
 * Field: ftdf_on_off_regmap_macTSTxAckDelay
 *
 * End of Rx frame to start of Ack
 */
#define IND_F_FTDF_ON_OFF_REGMAP_MACTSTXACKDELAY  IND_R_FTDF_ON_OFF_REGMAP_TSCH_CONTROL_0
#define MSK_F_FTDF_ON_OFF_REGMAP_MACTSTXACKDELAY  0xffff
#define OFF_F_FTDF_ON_OFF_REGMAP_MACTSTXACKDELAY  0
#define SIZE_F_FTDF_ON_OFF_REGMAP_MACTSTXACKDELAY 16
/*
 *
 * Field: ftdf_on_off_regmap_macTSRxWait
 *
 * The times to wait for start of frame
 */
#define IND_F_FTDF_ON_OFF_REGMAP_MACTSRXWAIT      IND_R_FTDF_ON_OFF_REGMAP_TSCH_CONTROL_0
#define MSK_F_FTDF_ON_OFF_REGMAP_MACTSRXWAIT      0xffff0000
#define OFF_F_FTDF_ON_OFF_REGMAP_MACTSRXWAIT      16
#define SIZE_F_FTDF_ON_OFF_REGMAP_MACTSRXWAIT     16

/*
 *
 * REGISTER: ftdf_on_off_regmap_tsch_control_1
 *
 * Initialization value: 0xc0  Initialization mask: 0xffff
 * Access mode: C-RW (control - read/write)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_TSCH_CONTROL_1  0x40090164
#define SIZE_R_FTDF_ON_OFF_REGMAP_TSCH_CONTROL_1 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_TSCH_CONTROL_1  0xffff

/*
 *
 * Field: ftdf_on_off_regmap_macTSRxTx
 *
 * The time between the CCA and the TX of a frame
 */
#define IND_F_FTDF_ON_OFF_REGMAP_MACTSRXTX  IND_R_FTDF_ON_OFF_REGMAP_TSCH_CONTROL_1
#define MSK_F_FTDF_ON_OFF_REGMAP_MACTSRXTX  0xffff
#define OFF_F_FTDF_ON_OFF_REGMAP_MACTSRXTX  0
#define SIZE_F_FTDF_ON_OFF_REGMAP_MACTSRXTX 16

/*
 *
 * REGISTER: ftdf_on_off_regmap_tsch_control_2
 *
 * Initialization value: 0x1900320  Initialization mask: 0xffffffff
 * Access mode: C-RW (control - read/write)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_TSCH_CONTROL_2  0x40090168
#define SIZE_R_FTDF_ON_OFF_REGMAP_TSCH_CONTROL_2 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_TSCH_CONTROL_2  0xffffffff

/*
 *
 * Field: ftdf_on_off_regmap_macTSRxAckDelay
 *
 * End of frame to when the transmitter shall listen for Acknowledgement
 */
#define IND_F_FTDF_ON_OFF_REGMAP_MACTSRXACKDELAY  IND_R_FTDF_ON_OFF_REGMAP_TSCH_CONTROL_2
#define MSK_F_FTDF_ON_OFF_REGMAP_MACTSRXACKDELAY  0xffff
#define OFF_F_FTDF_ON_OFF_REGMAP_MACTSRXACKDELAY  0
#define SIZE_F_FTDF_ON_OFF_REGMAP_MACTSRXACKDELAY 16
/*
 *
 * Field: ftdf_on_off_regmap_macTSAckWait
 *
 * The minimum time to wait for start of an Acknowledgement
 */
#define IND_F_FTDF_ON_OFF_REGMAP_MACTSACKWAIT     IND_R_FTDF_ON_OFF_REGMAP_TSCH_CONTROL_2
#define MSK_F_FTDF_ON_OFF_REGMAP_MACTSACKWAIT     0xffff0000
#define OFF_F_FTDF_ON_OFF_REGMAP_MACTSACKWAIT     16
#define SIZE_F_FTDF_ON_OFF_REGMAP_MACTSACKWAIT    16

/*
 *
 * REGISTER: ftdf_on_off_regmap_phy_parameters_0
 *
 * Initialization value: 0x76543210  Initialization mask: 0x77777777
 * Access mode: C-RW (control - read/write)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_PHY_PARAMETERS_0  0x40090180
#define SIZE_R_FTDF_ON_OFF_REGMAP_PHY_PARAMETERS_0 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_PHY_PARAMETERS_0  0x77777777

/*
 *
 * Field: ftdf_on_off_regmap_rxBitPos_0
 *
 * Control rxBitPos(8)(3) controls the position that a bit should have at the DPHY interface.
 * So the default values are rxBitPos_0 = 0, rxBitPos_1 = 1, rxBitPos_2 = 2, etc.
 *
 * Note1 that this is a conversion from rx DPHY interface to the internal data byte
 * So
 *  for(n=7;n>=0;n--)
 *    rx_data(n) = dphy_bit(tx_BitPos(n))
 *  endfor
 *
 * Note2 that rxBitPos and txBitPos must have inverse functions.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_RXBITPOS_0  IND_R_FTDF_ON_OFF_REGMAP_PHY_PARAMETERS_0
#define MSK_F_FTDF_ON_OFF_REGMAP_RXBITPOS_0  0x7
#define OFF_F_FTDF_ON_OFF_REGMAP_RXBITPOS_0  0
#define SIZE_F_FTDF_ON_OFF_REGMAP_RXBITPOS_0 3
/*
 *
 * Field: ftdf_on_off_regmap_rxBitPos_1
 *
 * See rxBitPos_0
 */
#define IND_F_FTDF_ON_OFF_REGMAP_RXBITPOS_1  IND_R_FTDF_ON_OFF_REGMAP_PHY_PARAMETERS_0
#define MSK_F_FTDF_ON_OFF_REGMAP_RXBITPOS_1  0x70
#define OFF_F_FTDF_ON_OFF_REGMAP_RXBITPOS_1  4
#define SIZE_F_FTDF_ON_OFF_REGMAP_RXBITPOS_1 3
/*
 *
 * Field: ftdf_on_off_regmap_rxBitPos_2
 *
 * See rxBitPos_0
 */
#define IND_F_FTDF_ON_OFF_REGMAP_RXBITPOS_2  IND_R_FTDF_ON_OFF_REGMAP_PHY_PARAMETERS_0
#define MSK_F_FTDF_ON_OFF_REGMAP_RXBITPOS_2  0x700
#define OFF_F_FTDF_ON_OFF_REGMAP_RXBITPOS_2  8
#define SIZE_F_FTDF_ON_OFF_REGMAP_RXBITPOS_2 3
/*
 *
 * Field: ftdf_on_off_regmap_rxBitPos_3
 *
 * See rxBitPos_0
 */
#define IND_F_FTDF_ON_OFF_REGMAP_RXBITPOS_3  IND_R_FTDF_ON_OFF_REGMAP_PHY_PARAMETERS_0
#define MSK_F_FTDF_ON_OFF_REGMAP_RXBITPOS_3  0x7000
#define OFF_F_FTDF_ON_OFF_REGMAP_RXBITPOS_3  12
#define SIZE_F_FTDF_ON_OFF_REGMAP_RXBITPOS_3 3
/*
 *
 * Field: ftdf_on_off_regmap_rxBitPos_4
 *
 * See rxBitPos_0
 */
#define IND_F_FTDF_ON_OFF_REGMAP_RXBITPOS_4  IND_R_FTDF_ON_OFF_REGMAP_PHY_PARAMETERS_0
#define MSK_F_FTDF_ON_OFF_REGMAP_RXBITPOS_4  0x70000
#define OFF_F_FTDF_ON_OFF_REGMAP_RXBITPOS_4  16
#define SIZE_F_FTDF_ON_OFF_REGMAP_RXBITPOS_4 3
/*
 *
 * Field: ftdf_on_off_regmap_rxBitPos_5
 *
 * See rxBitPos_0
 */
#define IND_F_FTDF_ON_OFF_REGMAP_RXBITPOS_5  IND_R_FTDF_ON_OFF_REGMAP_PHY_PARAMETERS_0
#define MSK_F_FTDF_ON_OFF_REGMAP_RXBITPOS_5  0x700000
#define OFF_F_FTDF_ON_OFF_REGMAP_RXBITPOS_5  20
#define SIZE_F_FTDF_ON_OFF_REGMAP_RXBITPOS_5 3
/*
 *
 * Field: ftdf_on_off_regmap_rxBitPos_6
 *
 * See rxBitPos_0
 */
#define IND_F_FTDF_ON_OFF_REGMAP_RXBITPOS_6  IND_R_FTDF_ON_OFF_REGMAP_PHY_PARAMETERS_0
#define MSK_F_FTDF_ON_OFF_REGMAP_RXBITPOS_6  0x7000000
#define OFF_F_FTDF_ON_OFF_REGMAP_RXBITPOS_6  24
#define SIZE_F_FTDF_ON_OFF_REGMAP_RXBITPOS_6 3
/*
 *
 * Field: ftdf_on_off_regmap_rxBitPos_7
 *
 * See rxBitPos_0
 */
#define IND_F_FTDF_ON_OFF_REGMAP_RXBITPOS_7  IND_R_FTDF_ON_OFF_REGMAP_PHY_PARAMETERS_0
#define MSK_F_FTDF_ON_OFF_REGMAP_RXBITPOS_7  0x70000000
#define OFF_F_FTDF_ON_OFF_REGMAP_RXBITPOS_7  28
#define SIZE_F_FTDF_ON_OFF_REGMAP_RXBITPOS_7 3

/*
 *
 * REGISTER: ftdf_on_off_regmap_phy_parameters_1
 *
 * Initialization value: 0x76543210  Initialization mask: 0x77777777
 * Access mode: C-RW (control - read/write)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_PHY_PARAMETERS_1  0x40090184
#define SIZE_R_FTDF_ON_OFF_REGMAP_PHY_PARAMETERS_1 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_PHY_PARAMETERS_1  0x77777777

/*
 *
 * Field: ftdf_on_off_regmap_txBitPos_0
 *
 * Control txBitPos(8)(3) controls the position that a bit should have at the DPHY interface.
 * So the default values are txBitPos_0 = 0, txBitPos_1 = 1, txBitPos_2 = 2, etc.
 *
 * Note1 that this is a conversion from internal data byte to the DPHY interface.
 * So
 *  for(n=7;n>=0;n--)
 *    tx_dphy_bit(n) = tx_data(tx_BitPos(n))
 *  endfor
 *
 * Note2 that txBitPos and rxBitPos must have inverse functions.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_TXBITPOS_0  IND_R_FTDF_ON_OFF_REGMAP_PHY_PARAMETERS_1
#define MSK_F_FTDF_ON_OFF_REGMAP_TXBITPOS_0  0x7
#define OFF_F_FTDF_ON_OFF_REGMAP_TXBITPOS_0  0
#define SIZE_F_FTDF_ON_OFF_REGMAP_TXBITPOS_0 3
/*
 *
 * Field: ftdf_on_off_regmap_txBitPos_1
 *
 * See txBitPos_0
 */
#define IND_F_FTDF_ON_OFF_REGMAP_TXBITPOS_1  IND_R_FTDF_ON_OFF_REGMAP_PHY_PARAMETERS_1
#define MSK_F_FTDF_ON_OFF_REGMAP_TXBITPOS_1  0x70
#define OFF_F_FTDF_ON_OFF_REGMAP_TXBITPOS_1  4
#define SIZE_F_FTDF_ON_OFF_REGMAP_TXBITPOS_1 3
/*
 *
 * Field: ftdf_on_off_regmap_txBitPos_2
 *
 * See txBitPos_0
 */
#define IND_F_FTDF_ON_OFF_REGMAP_TXBITPOS_2  IND_R_FTDF_ON_OFF_REGMAP_PHY_PARAMETERS_1
#define MSK_F_FTDF_ON_OFF_REGMAP_TXBITPOS_2  0x700
#define OFF_F_FTDF_ON_OFF_REGMAP_TXBITPOS_2  8
#define SIZE_F_FTDF_ON_OFF_REGMAP_TXBITPOS_2 3
/*
 *
 * Field: ftdf_on_off_regmap_txBitPos_3
 *
 * See txBitPos_0
 */
#define IND_F_FTDF_ON_OFF_REGMAP_TXBITPOS_3  IND_R_FTDF_ON_OFF_REGMAP_PHY_PARAMETERS_1
#define MSK_F_FTDF_ON_OFF_REGMAP_TXBITPOS_3  0x7000
#define OFF_F_FTDF_ON_OFF_REGMAP_TXBITPOS_3  12
#define SIZE_F_FTDF_ON_OFF_REGMAP_TXBITPOS_3 3
/*
 *
 * Field: ftdf_on_off_regmap_txBitPos_4
 *
 * See txBitPos_0
 */
#define IND_F_FTDF_ON_OFF_REGMAP_TXBITPOS_4  IND_R_FTDF_ON_OFF_REGMAP_PHY_PARAMETERS_1
#define MSK_F_FTDF_ON_OFF_REGMAP_TXBITPOS_4  0x70000
#define OFF_F_FTDF_ON_OFF_REGMAP_TXBITPOS_4  16
#define SIZE_F_FTDF_ON_OFF_REGMAP_TXBITPOS_4 3
/*
 *
 * Field: ftdf_on_off_regmap_txBitPos_5
 *
 * See txBitPos_0
 */
#define IND_F_FTDF_ON_OFF_REGMAP_TXBITPOS_5  IND_R_FTDF_ON_OFF_REGMAP_PHY_PARAMETERS_1
#define MSK_F_FTDF_ON_OFF_REGMAP_TXBITPOS_5  0x700000
#define OFF_F_FTDF_ON_OFF_REGMAP_TXBITPOS_5  20
#define SIZE_F_FTDF_ON_OFF_REGMAP_TXBITPOS_5 3
/*
 *
 * Field: ftdf_on_off_regmap_txBitPos_6
 *
 * See txBitPos_0
 */
#define IND_F_FTDF_ON_OFF_REGMAP_TXBITPOS_6  IND_R_FTDF_ON_OFF_REGMAP_PHY_PARAMETERS_1
#define MSK_F_FTDF_ON_OFF_REGMAP_TXBITPOS_6  0x7000000
#define OFF_F_FTDF_ON_OFF_REGMAP_TXBITPOS_6  24
#define SIZE_F_FTDF_ON_OFF_REGMAP_TXBITPOS_6 3
/*
 *
 * Field: ftdf_on_off_regmap_txBitPos_7
 *
 * See txBitPos_0
 */
#define IND_F_FTDF_ON_OFF_REGMAP_TXBITPOS_7  IND_R_FTDF_ON_OFF_REGMAP_PHY_PARAMETERS_1
#define MSK_F_FTDF_ON_OFF_REGMAP_TXBITPOS_7  0x70000000
#define OFF_F_FTDF_ON_OFF_REGMAP_TXBITPOS_7  28
#define SIZE_F_FTDF_ON_OFF_REGMAP_TXBITPOS_7 3

/*
 *
 * REGISTER: ftdf_on_off_regmap_phy_parameters_2
 *
 * Initialization value: 0x0  Initialization mask: 0xffffffff
 * Access mode: C-RW (control - read/write)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_PHY_PARAMETERS_2  0x40090188
#define SIZE_R_FTDF_ON_OFF_REGMAP_PHY_PARAMETERS_2 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_PHY_PARAMETERS_2  0xffffffff

/*
 *
 * Field: ftdf_on_off_regmap_phyTxStartup
 *
 * Phy wait time before transmission
 */
#define IND_F_FTDF_ON_OFF_REGMAP_PHYTXSTARTUP  IND_R_FTDF_ON_OFF_REGMAP_PHY_PARAMETERS_2
#define MSK_F_FTDF_ON_OFF_REGMAP_PHYTXSTARTUP  0xff
#define OFF_F_FTDF_ON_OFF_REGMAP_PHYTXSTARTUP  0
#define SIZE_F_FTDF_ON_OFF_REGMAP_PHYTXSTARTUP 8
/*
 *
 * Field: ftdf_on_off_regmap_phyTxLatency
 *
 * Phy delay between DPHY i/f and air
 */
#define IND_F_FTDF_ON_OFF_REGMAP_PHYTXLATENCY  IND_R_FTDF_ON_OFF_REGMAP_PHY_PARAMETERS_2
#define MSK_F_FTDF_ON_OFF_REGMAP_PHYTXLATENCY  0xff00
#define OFF_F_FTDF_ON_OFF_REGMAP_PHYTXLATENCY  8
#define SIZE_F_FTDF_ON_OFF_REGMAP_PHYTXLATENCY 8
/*
 *
 * Field: ftdf_on_off_regmap_phyTxFinish
 *
 * Phy wait time before deasserting TX_EN
 */
#define IND_F_FTDF_ON_OFF_REGMAP_PHYTXFINISH   IND_R_FTDF_ON_OFF_REGMAP_PHY_PARAMETERS_2
#define MSK_F_FTDF_ON_OFF_REGMAP_PHYTXFINISH   0xff0000
#define OFF_F_FTDF_ON_OFF_REGMAP_PHYTXFINISH   16
#define SIZE_F_FTDF_ON_OFF_REGMAP_PHYTXFINISH  8
/*
 *
 * Field: ftdf_on_off_regmap_phyTRxWait
 *
 * Phy wait time between TX_EN/RX_EN
 */
#define IND_F_FTDF_ON_OFF_REGMAP_PHYTRXWAIT    IND_R_FTDF_ON_OFF_REGMAP_PHY_PARAMETERS_2
#define MSK_F_FTDF_ON_OFF_REGMAP_PHYTRXWAIT    0xff000000
#define OFF_F_FTDF_ON_OFF_REGMAP_PHYTRXWAIT    24
#define SIZE_F_FTDF_ON_OFF_REGMAP_PHYTRXWAIT   8

/*
 *
 * REGISTER: ftdf_on_off_regmap_phy_parameters_3
 *
 * Initialization value: 0x0  Initialization mask: 0xffffff
 * Access mode: C-RW (control - read/write)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_PHY_PARAMETERS_3  0x4009018c
#define SIZE_R_FTDF_ON_OFF_REGMAP_PHY_PARAMETERS_3 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_PHY_PARAMETERS_3  0xffffff

/*
 *
 * Field: ftdf_on_off_regmap_phyRxStartup
 *
 * Phy wait time before receiving
 */
#define IND_F_FTDF_ON_OFF_REGMAP_PHYRXSTARTUP  IND_R_FTDF_ON_OFF_REGMAP_PHY_PARAMETERS_3
#define MSK_F_FTDF_ON_OFF_REGMAP_PHYRXSTARTUP  0xff
#define OFF_F_FTDF_ON_OFF_REGMAP_PHYRXSTARTUP  0
#define SIZE_F_FTDF_ON_OFF_REGMAP_PHYRXSTARTUP 8
/*
 *
 * Field: ftdf_on_off_regmap_phyRxLatency
 *
 * Phy delay between air and DPHY i/f
 */
#define IND_F_FTDF_ON_OFF_REGMAP_PHYRXLATENCY  IND_R_FTDF_ON_OFF_REGMAP_PHY_PARAMETERS_3
#define MSK_F_FTDF_ON_OFF_REGMAP_PHYRXLATENCY  0xff00
#define OFF_F_FTDF_ON_OFF_REGMAP_PHYRXLATENCY  8
#define SIZE_F_FTDF_ON_OFF_REGMAP_PHYRXLATENCY 8
/*
 *
 * Field: ftdf_on_off_regmap_phyEnable
 *
 * Asserting the DPHY interface signals TX_EN or RX_EN does not take place within the time phyEnable after asserting the signal phy_en.
 * (resolution: ~s).
 */
#define IND_F_FTDF_ON_OFF_REGMAP_PHYENABLE     IND_R_FTDF_ON_OFF_REGMAP_PHY_PARAMETERS_3
#define MSK_F_FTDF_ON_OFF_REGMAP_PHYENABLE     0xff0000
#define OFF_F_FTDF_ON_OFF_REGMAP_PHYENABLE     16
#define SIZE_F_FTDF_ON_OFF_REGMAP_PHYENABLE    8

/*
 *
 * REGISTER: ftdf_on_off_regmap_rx_control_0
 *
 * Initialization value: 0x0  Initialization mask: 0xfffffff
 * Access mode: C-RW (control - read/write)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_RX_CONTROL_0  0x40090200
#define SIZE_R_FTDF_ON_OFF_REGMAP_RX_CONTROL_0 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_RX_CONTROL_0  0xfffffff

/*
 *
 * Bit: ftdf_on_off_regmap_DbgRxTransparentMode
 *
 * If set, Rx pipe is fully set in transparent mode (for debug purpose).
 */
#define IND_F_FTDF_ON_OFF_REGMAP_DBGRXTRANSPARENTMODE           IND_R_FTDF_ON_OFF_REGMAP_RX_CONTROL_0
#define MSK_F_FTDF_ON_OFF_REGMAP_DBGRXTRANSPARENTMODE           0x1
#define OFF_F_FTDF_ON_OFF_REGMAP_DBGRXTRANSPARENTMODE           0
#define SIZE_F_FTDF_ON_OFF_REGMAP_DBGRXTRANSPARENTMODE          1
/*
 *
 * Bit: ftdf_on_off_regmap_RxBeaconOnly
 *
 * If set, only Beacons frames are accepted
 */
#define IND_F_FTDF_ON_OFF_REGMAP_RXBEACONONLY                   IND_R_FTDF_ON_OFF_REGMAP_RX_CONTROL_0
#define MSK_F_FTDF_ON_OFF_REGMAP_RXBEACONONLY                   0x2
#define OFF_F_FTDF_ON_OFF_REGMAP_RXBEACONONLY                   1
#define SIZE_F_FTDF_ON_OFF_REGMAP_RXBEACONONLY                  1
/*
 *
 * Bit: ftdf_on_off_regmap_RxCoordRealignOnly
 *
 * If set, only Coordinator Realignment frames are accepted
 */
#define IND_F_FTDF_ON_OFF_REGMAP_RXCOORDREALIGNONLY             IND_R_FTDF_ON_OFF_REGMAP_RX_CONTROL_0
#define MSK_F_FTDF_ON_OFF_REGMAP_RXCOORDREALIGNONLY             0x4
#define OFF_F_FTDF_ON_OFF_REGMAP_RXCOORDREALIGNONLY             2
#define SIZE_F_FTDF_ON_OFF_REGMAP_RXCOORDREALIGNONLY            1
/*
 *
 * Field: ftdf_on_off_regmap_rx_read_buf_ptr
 *
 * Indication where new data will be read
 * All four bits shall be used when using these pointer values (0d - 15d).
 * However, the Receive Packet buffer has a size of 8 entries.
 * So reading the Receive Packet buffer entries shall use the mod8 of the pointer values.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_RX_READ_BUF_PTR                IND_R_FTDF_ON_OFF_REGMAP_RX_CONTROL_0
#define MSK_F_FTDF_ON_OFF_REGMAP_RX_READ_BUF_PTR                0x78
#define OFF_F_FTDF_ON_OFF_REGMAP_RX_READ_BUF_PTR                3
#define SIZE_F_FTDF_ON_OFF_REGMAP_RX_READ_BUF_PTR               4
/*
 *
 * Bit: ftdf_on_off_regmap_DisRxFrmPendingca
 *
 * Whan the control register DisRxFrmPendingCa is set, the notification of the received FP bit to the LMAC Controller is disabled.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_DISRXFRMPENDINGCA              IND_R_FTDF_ON_OFF_REGMAP_RX_CONTROL_0
#define MSK_F_FTDF_ON_OFF_REGMAP_DISRXFRMPENDINGCA              0x80
#define OFF_F_FTDF_ON_OFF_REGMAP_DISRXFRMPENDINGCA              7
#define SIZE_F_FTDF_ON_OFF_REGMAP_DISRXFRMPENDINGCA             1
/*
 *
 * Bit: ftdf_on_off_regmap_DisRxAckRequestca
 *
 * When the control register DisRxAckRequestca is set all consequent actions for a received Acknowledge Request bit are disabled.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_DISRXACKREQUESTCA              IND_R_FTDF_ON_OFF_REGMAP_RX_CONTROL_0
#define MSK_F_FTDF_ON_OFF_REGMAP_DISRXACKREQUESTCA              0x100
#define OFF_F_FTDF_ON_OFF_REGMAP_DISRXACKREQUESTCA              8
#define SIZE_F_FTDF_ON_OFF_REGMAP_DISRXACKREQUESTCA             1
/*
 *
 * Bit: ftdf_on_off_regmap_macAlwaysPassCrcerror
 *
 * If set, a FCS error will not drop the frame
 */
#define IND_F_FTDF_ON_OFF_REGMAP_MACALWAYSPASSCRCERROR          IND_R_FTDF_ON_OFF_REGMAP_RX_CONTROL_0
#define MSK_F_FTDF_ON_OFF_REGMAP_MACALWAYSPASSCRCERROR          0x200
#define OFF_F_FTDF_ON_OFF_REGMAP_MACALWAYSPASSCRCERROR          9
#define SIZE_F_FTDF_ON_OFF_REGMAP_MACALWAYSPASSCRCERROR         1
/*
 *
 * Bit: ftdf_on_off_regmap_DisDataRequestca
 *
 * When the control register DisDataRequestCa is set, the notification of the received Data Request is disabled.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_DISDATAREQUESTCA               IND_R_FTDF_ON_OFF_REGMAP_RX_CONTROL_0
#define MSK_F_FTDF_ON_OFF_REGMAP_DISDATAREQUESTCA               0x400
#define OFF_F_FTDF_ON_OFF_REGMAP_DISDATAREQUESTCA               10
#define SIZE_F_FTDF_ON_OFF_REGMAP_DISDATAREQUESTCA              1
/*
 *
 * Bit: ftdf_on_off_regmap_macAlwaysPassResFrameVersion
 *
 * If set, a packet with a reserved FrameVersion shall not be dropped
 */
#define IND_F_FTDF_ON_OFF_REGMAP_MACALWAYSPASSRESFRAMEVERSION   IND_R_FTDF_ON_OFF_REGMAP_RX_CONTROL_0
#define MSK_F_FTDF_ON_OFF_REGMAP_MACALWAYSPASSRESFRAMEVERSION   0x800
#define OFF_F_FTDF_ON_OFF_REGMAP_MACALWAYSPASSRESFRAMEVERSION   11
#define SIZE_F_FTDF_ON_OFF_REGMAP_MACALWAYSPASSRESFRAMEVERSION  1
/*
 *
 * Bit: ftdf_on_off_regmap_macAlwaysPassWrongDPANId
 *
 * If register macAlwaysPassWrongDPANId is set, packet with a wrong Destiantion PanID will not be dropped.
 * However, in case of an FCS error, the packet is dropped.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_MACALWAYSPASSWRONGDPANID       IND_R_FTDF_ON_OFF_REGMAP_RX_CONTROL_0
#define MSK_F_FTDF_ON_OFF_REGMAP_MACALWAYSPASSWRONGDPANID       0x1000
#define OFF_F_FTDF_ON_OFF_REGMAP_MACALWAYSPASSWRONGDPANID       12
#define SIZE_F_FTDF_ON_OFF_REGMAP_MACALWAYSPASSWRONGDPANID      1
/*
 *
 * Bit: ftdf_on_off_regmap_macAlwaysPassWrongDAddr
 *
 * If set, a packet with a wrong DAddr is  not dropped
 */
#define IND_F_FTDF_ON_OFF_REGMAP_MACALWAYSPASSWRONGDADDR        IND_R_FTDF_ON_OFF_REGMAP_RX_CONTROL_0
#define MSK_F_FTDF_ON_OFF_REGMAP_MACALWAYSPASSWRONGDADDR        0x2000
#define OFF_F_FTDF_ON_OFF_REGMAP_MACALWAYSPASSWRONGDADDR        13
#define SIZE_F_FTDF_ON_OFF_REGMAP_MACALWAYSPASSWRONGDADDR       1
/*
 *
 * Bit: ftdf_on_off_regmap_macAlwaysPassBeaconWrongPANId
 *
 * If the control register macAlwaysPassBeaconWrongPANId is set, the frame is not dropped in case of a mismatch in PAN-ID, irrespective of the setting of RxBeaconOnly.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_MACALWAYSPASSBEACONWRONGPANID  IND_R_FTDF_ON_OFF_REGMAP_RX_CONTROL_0
#define MSK_F_FTDF_ON_OFF_REGMAP_MACALWAYSPASSBEACONWRONGPANID  0x4000
#define OFF_F_FTDF_ON_OFF_REGMAP_MACALWAYSPASSBEACONWRONGPANID  14
#define SIZE_F_FTDF_ON_OFF_REGMAP_MACALWAYSPASSBEACONWRONGPANID 1
/*
 *
 * Bit: ftdf_on_off_regmap_macAlwaysPassToPanCoordinator
 *
 * When the control register macAlwaysPassToPanCoordinator is set, the frame is not dropped due to a span_coord_error.
 * However, in case of an FCS error, the packet is dropped.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_MACALWAYSPASSTOPANCOORDINATOR  IND_R_FTDF_ON_OFF_REGMAP_RX_CONTROL_0
#define MSK_F_FTDF_ON_OFF_REGMAP_MACALWAYSPASSTOPANCOORDINATOR  0x8000
#define OFF_F_FTDF_ON_OFF_REGMAP_MACALWAYSPASSTOPANCOORDINATOR  15
#define SIZE_F_FTDF_ON_OFF_REGMAP_MACALWAYSPASSTOPANCOORDINATOR 1
/*
 *
 * Field: ftdf_on_off_regmap_macAlwaysPassFrmType
 *
 * The control registers macAlwaysPassFrmType[7:0], shall control if this Frame Type shall be dropped.
 * If a bit is set, the Frame Type corresponding with the bit position is not dropped, even in case of a CRC error.
 * Example: if bit 1 is set, Frame Type 1 shall not be dropped.
 * The error shall be reported in the Rx meta data
 */
#define IND_F_FTDF_ON_OFF_REGMAP_MACALWAYSPASSFRMTYPE           IND_R_FTDF_ON_OFF_REGMAP_RX_CONTROL_0
#define MSK_F_FTDF_ON_OFF_REGMAP_MACALWAYSPASSFRMTYPE           0xff0000
#define OFF_F_FTDF_ON_OFF_REGMAP_MACALWAYSPASSFRMTYPE           16
#define SIZE_F_FTDF_ON_OFF_REGMAP_MACALWAYSPASSFRMTYPE          8
/*
 *
 * Bit: ftdf_on_off_regmap_macAlwaysPassWakeUp
 *
 * If the control register macAlwaysPassWakeUp is set, received Wake- up frames for this device are put into the Rx packet buffer without notifying the LMAC Controller.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_MACALWAYSPASSWAKEUP            IND_R_FTDF_ON_OFF_REGMAP_RX_CONTROL_0
#define MSK_F_FTDF_ON_OFF_REGMAP_MACALWAYSPASSWAKEUP            0x1000000
#define OFF_F_FTDF_ON_OFF_REGMAP_MACALWAYSPASSWAKEUP            24
#define SIZE_F_FTDF_ON_OFF_REGMAP_MACALWAYSPASSWAKEUP           1
/*
 *
 * Bit: ftdf_on_off_regmap_macPassWakeUp
 *
 * If set, WakeUp frames will not be reported but will be put into the Rx buffer.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_MACPASSWAKEUP                  IND_R_FTDF_ON_OFF_REGMAP_RX_CONTROL_0
#define MSK_F_FTDF_ON_OFF_REGMAP_MACPASSWAKEUP                  0x2000000
#define OFF_F_FTDF_ON_OFF_REGMAP_MACPASSWAKEUP                  25
#define SIZE_F_FTDF_ON_OFF_REGMAP_MACPASSWAKEUP                 1
/*
 *
 * Bit: ftdf_on_off_regmap_macImplicitBroadcast
 *
 * If set, Frame Version 2 frames without Daddr or DPANId shall be accepted.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_MACIMPLICITBROADCAST           IND_R_FTDF_ON_OFF_REGMAP_RX_CONTROL_0
#define MSK_F_FTDF_ON_OFF_REGMAP_MACIMPLICITBROADCAST           0x4000000
#define OFF_F_FTDF_ON_OFF_REGMAP_MACIMPLICITBROADCAST           26
#define SIZE_F_FTDF_ON_OFF_REGMAP_MACIMPLICITBROADCAST          1
/*
 *
 * Bit: ftdf_on_off_regmap_DisRxAckReceivedca
 *
 * If set, the LMAC controller shall ignore all consequent actions upon a set AR bit in the transmitted frame
 */
#define IND_F_FTDF_ON_OFF_REGMAP_DISRXACKRECEIVEDCA             IND_R_FTDF_ON_OFF_REGMAP_RX_CONTROL_0
#define MSK_F_FTDF_ON_OFF_REGMAP_DISRXACKRECEIVEDCA             0x8000000
#define OFF_F_FTDF_ON_OFF_REGMAP_DISRXACKRECEIVEDCA             27
#define SIZE_F_FTDF_ON_OFF_REGMAP_DISRXACKRECEIVEDCA            1

/*
 *
 * REGISTER: ftdf_on_off_regmap_rx_event
 *
 * Initialization value: 0x0  Initialization mask: 0xf
 * Access mode: E-RCW2 (event read/clear on write 1)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_RX_EVENT  0x40090204
#define SIZE_R_FTDF_ON_OFF_REGMAP_RX_EVENT 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_RX_EVENT  0xf

/*
 *
 * Bit: ftdf_on_off_regmap_RxSof_e
 *
 * Set when RX_SOF has been detected.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_RXSOF_E         IND_R_FTDF_ON_OFF_REGMAP_RX_EVENT
#define MSK_F_FTDF_ON_OFF_REGMAP_RXSOF_E         0x1
#define OFF_F_FTDF_ON_OFF_REGMAP_RXSOF_E         0
#define SIZE_F_FTDF_ON_OFF_REGMAP_RXSOF_E        1
/*
 *
 * Bit: ftdf_on_off_regmap_rx_overflow_e
 *
 * Indicates that the Rx packet buffer has an
 * overflowl
 */
#define IND_F_FTDF_ON_OFF_REGMAP_RX_OVERFLOW_E   IND_R_FTDF_ON_OFF_REGMAP_RX_EVENT
#define MSK_F_FTDF_ON_OFF_REGMAP_RX_OVERFLOW_E   0x2
#define OFF_F_FTDF_ON_OFF_REGMAP_RX_OVERFLOW_E   1
#define SIZE_F_FTDF_ON_OFF_REGMAP_RX_OVERFLOW_E  1
/*
 *
 * Bit: ftdf_on_off_regmap_rx_buf_avail_e
 *
 * Indicates that a new packet is received
 */
#define IND_F_FTDF_ON_OFF_REGMAP_RX_BUF_AVAIL_E  IND_R_FTDF_ON_OFF_REGMAP_RX_EVENT
#define MSK_F_FTDF_ON_OFF_REGMAP_RX_BUF_AVAIL_E  0x4
#define OFF_F_FTDF_ON_OFF_REGMAP_RX_BUF_AVAIL_E  2
#define SIZE_F_FTDF_ON_OFF_REGMAP_RX_BUF_AVAIL_E 1
/*
 *
 * Bit: ftdf_on_off_regmap_rxbyte_e
 *
 * Indicates the first byte of a new packet is received
 */
#define IND_F_FTDF_ON_OFF_REGMAP_RXBYTE_E        IND_R_FTDF_ON_OFF_REGMAP_RX_EVENT
#define MSK_F_FTDF_ON_OFF_REGMAP_RXBYTE_E        0x8
#define OFF_F_FTDF_ON_OFF_REGMAP_RXBYTE_E        3
#define SIZE_F_FTDF_ON_OFF_REGMAP_RXBYTE_E       1

/*
 *
 * REGISTER: ftdf_on_off_regmap_rx_mask
 *
 * Initialization value: 0x0  Initialization mask: 0xf
 * Access mode: EM-RW (event mask)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_RX_MASK  0x40090208
#define SIZE_R_FTDF_ON_OFF_REGMAP_RX_MASK 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_RX_MASK  0xf

/*
 *
 * Bit: ftdf_on_off_regmap_RxSof_m
 *
 * Mask bit for event "RxSof_e".
 */
#define IND_F_FTDF_ON_OFF_REGMAP_RXSOF_M         IND_R_FTDF_ON_OFF_REGMAP_RX_MASK
#define MSK_F_FTDF_ON_OFF_REGMAP_RXSOF_M         0x1
#define OFF_F_FTDF_ON_OFF_REGMAP_RXSOF_M         0
#define SIZE_F_FTDF_ON_OFF_REGMAP_RXSOF_M        1
/*
 *
 * Bit: ftdf_on_off_regmap_rx_overflow_m
 *
 * Mask bit for event " rx_overflow_e".
 */
#define IND_F_FTDF_ON_OFF_REGMAP_RX_OVERFLOW_M   IND_R_FTDF_ON_OFF_REGMAP_RX_MASK
#define MSK_F_FTDF_ON_OFF_REGMAP_RX_OVERFLOW_M   0x2
#define OFF_F_FTDF_ON_OFF_REGMAP_RX_OVERFLOW_M   1
#define SIZE_F_FTDF_ON_OFF_REGMAP_RX_OVERFLOW_M  1
/*
 *
 * Bit: ftdf_on_off_regmap_rx_buf_avail_m
 *
 * Mask bit for event "rx_buf_avail_e".
 */
#define IND_F_FTDF_ON_OFF_REGMAP_RX_BUF_AVAIL_M  IND_R_FTDF_ON_OFF_REGMAP_RX_MASK
#define MSK_F_FTDF_ON_OFF_REGMAP_RX_BUF_AVAIL_M  0x4
#define OFF_F_FTDF_ON_OFF_REGMAP_RX_BUF_AVAIL_M  2
#define SIZE_F_FTDF_ON_OFF_REGMAP_RX_BUF_AVAIL_M 1
/*
 *
 * Bit: ftdf_on_off_regmap_rxbyte_m
 *
 * Mask bit for event "rxbyte_e".
 */
#define IND_F_FTDF_ON_OFF_REGMAP_RXBYTE_M        IND_R_FTDF_ON_OFF_REGMAP_RX_MASK
#define MSK_F_FTDF_ON_OFF_REGMAP_RXBYTE_M        0x8
#define OFF_F_FTDF_ON_OFF_REGMAP_RXBYTE_M        3
#define SIZE_F_FTDF_ON_OFF_REGMAP_RXBYTE_M       1

/*
 *
 * REGISTER: ftdf_on_off_regmap_rx_status
 *
 * Initialization value: none.
 * Access mode: S-R (state - read)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_RX_STATUS  0x4009020c
#define SIZE_R_FTDF_ON_OFF_REGMAP_RX_STATUS 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_RX_STATUS  0x1f

/*
 *
 * Bit: ftdf_on_off_regmap_rx_buff_is_full
 *
 * Indicates that the Rx packet buffer is full
 */
#define IND_F_FTDF_ON_OFF_REGMAP_RX_BUFF_IS_FULL   IND_R_FTDF_ON_OFF_REGMAP_RX_STATUS
#define MSK_F_FTDF_ON_OFF_REGMAP_RX_BUFF_IS_FULL   0x1
#define OFF_F_FTDF_ON_OFF_REGMAP_RX_BUFF_IS_FULL   0
#define SIZE_F_FTDF_ON_OFF_REGMAP_RX_BUFF_IS_FULL  1
/*
 *
 * Field: ftdf_on_off_regmap_rx_write_buf_ptr
 *
 * Indication where new data will be written.
 * All four bits shall be used when using these pointer values (0d - 15d).
 * However, the Receive Packet buffer has a size of 8 entries.
 * So reading the Receive Packet buffer entries shall use the mod8 of the pointer values.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_RX_WRITE_BUF_PTR  IND_R_FTDF_ON_OFF_REGMAP_RX_STATUS
#define MSK_F_FTDF_ON_OFF_REGMAP_RX_WRITE_BUF_PTR  0x1e
#define OFF_F_FTDF_ON_OFF_REGMAP_RX_WRITE_BUF_PTR  1
#define SIZE_F_FTDF_ON_OFF_REGMAP_RX_WRITE_BUF_PTR 4

/*
 *
 * REGISTER: ftdf_on_off_regmap_SymbolTimeSnapshotVal
 *
 * Initialization value: none.
 * Access mode: S-R (state - read)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_SYMBOLTIMESNAPSHOTVAL  0x40090210
#define SIZE_R_FTDF_ON_OFF_REGMAP_SYMBOLTIMESNAPSHOTVAL 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_SYMBOLTIMESNAPSHOTVAL  0xffffffff

/*
 *
 * Field: ftdf_on_off_regmap_SymbolTimeSnapshotVal
 *
 * The Status register SymbolTimeSnapshotVal indicates the actual value of the TimeStamp generator.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_SYMBOLTIMESNAPSHOTVAL  IND_R_FTDF_ON_OFF_REGMAP_SYMBOLTIMESNAPSHOTVAL
#define MSK_F_FTDF_ON_OFF_REGMAP_SYMBOLTIMESNAPSHOTVAL  0xffffffff
#define OFF_F_FTDF_ON_OFF_REGMAP_SYMBOLTIMESNAPSHOTVAL  0
#define SIZE_F_FTDF_ON_OFF_REGMAP_SYMBOLTIMESNAPSHOTVAL 32

/*
 *
 * REGISTER: ftdf_on_off_regmap_rx_status_delta
 *
 * Initialization value: 0x0  Initialization mask: 0x1
 * Access mode: D-RCW (delta read/clear on write 1)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_RX_STATUS_DELTA  0x40090220
#define SIZE_R_FTDF_ON_OFF_REGMAP_RX_STATUS_DELTA 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_RX_STATUS_DELTA  0x1

/*
 *
 * Bit: ftdf_on_off_regmap_rx_buff_is_full_d
 *
 * Delta bit of status "rx_buff_is_full"
 */
#define IND_F_FTDF_ON_OFF_REGMAP_RX_BUFF_IS_FULL_D  IND_R_FTDF_ON_OFF_REGMAP_RX_STATUS_DELTA
#define MSK_F_FTDF_ON_OFF_REGMAP_RX_BUFF_IS_FULL_D  0x1
#define OFF_F_FTDF_ON_OFF_REGMAP_RX_BUFF_IS_FULL_D  0
#define SIZE_F_FTDF_ON_OFF_REGMAP_RX_BUFF_IS_FULL_D 1

/*
 *
 * REGISTER: ftdf_on_off_regmap_rx_status_mask
 *
 * Initialization value: 0x0  Initialization mask: 0x1
 * Access mode: DM-RW (delta mask)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_RX_STATUS_MASK  0x40090224
#define SIZE_R_FTDF_ON_OFF_REGMAP_RX_STATUS_MASK 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_RX_STATUS_MASK  0x1

/*
 *
 * Bit: ftdf_on_off_regmap_rx_buff_is_full_m
 *
 * Mask bit of status "rx_buff_is_full"
 */
#define IND_F_FTDF_ON_OFF_REGMAP_RX_BUFF_IS_FULL_M  IND_R_FTDF_ON_OFF_REGMAP_RX_STATUS_MASK
#define MSK_F_FTDF_ON_OFF_REGMAP_RX_BUFF_IS_FULL_M  0x1
#define OFF_F_FTDF_ON_OFF_REGMAP_RX_BUFF_IS_FULL_M  0
#define SIZE_F_FTDF_ON_OFF_REGMAP_RX_BUFF_IS_FULL_M 1

/*
 *
 * REGISTER: ftdf_on_off_regmap_tx_control_0
 *
 * Initialization value: 0x4350  Initialization mask: 0x7ff1
 * Access mode: C-RW (control - read/write)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_TX_CONTROL_0  0x40090240
#define SIZE_R_FTDF_ON_OFF_REGMAP_TX_CONTROL_0 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_TX_CONTROL_0  0x7ff1

/*
 *
 * Bit: ftdf_on_off_regmap_DbgTxTransparentMode
 *
 * If 1, the MPDU octets pass transparently through the MAC in the transmit direction (for debug purpose).
 */
#define IND_F_FTDF_ON_OFF_REGMAP_DBGTXTRANSPARENTMODE  IND_R_FTDF_ON_OFF_REGMAP_TX_CONTROL_0
#define MSK_F_FTDF_ON_OFF_REGMAP_DBGTXTRANSPARENTMODE  0x1
#define OFF_F_FTDF_ON_OFF_REGMAP_DBGTXTRANSPARENTMODE  0
#define SIZE_F_FTDF_ON_OFF_REGMAP_DBGTXTRANSPARENTMODE 1
/*
 *
 * Field: ftdf_on_off_regmap_macMaxBE
 *
 * Maximum Backoff Exponent (range 3-8)
 */
#define IND_F_FTDF_ON_OFF_REGMAP_MACMAXBE              IND_R_FTDF_ON_OFF_REGMAP_TX_CONTROL_0
#define MSK_F_FTDF_ON_OFF_REGMAP_MACMAXBE              0xf0
#define OFF_F_FTDF_ON_OFF_REGMAP_MACMAXBE              4
#define SIZE_F_FTDF_ON_OFF_REGMAP_MACMAXBE             4
/*
 *
 * Field: ftdf_on_off_regmap_macMinBE
 *
 * Minimum Backoff Exponent (range 0-macMaxBE)
 */
#define IND_F_FTDF_ON_OFF_REGMAP_MACMINBE              IND_R_FTDF_ON_OFF_REGMAP_TX_CONTROL_0
#define MSK_F_FTDF_ON_OFF_REGMAP_MACMINBE              0xf00
#define OFF_F_FTDF_ON_OFF_REGMAP_MACMINBE              8
#define SIZE_F_FTDF_ON_OFF_REGMAP_MACMINBE             4
/*
 *
 * Field: ftdf_on_off_regmap_macMaxCSMABackoffs
 *
 * Maximum number of CSMA-CA backoffs
 * (range 0-5)
 */
#define IND_F_FTDF_ON_OFF_REGMAP_MACMAXCSMABACKOFFS    IND_R_FTDF_ON_OFF_REGMAP_TX_CONTROL_0
#define MSK_F_FTDF_ON_OFF_REGMAP_MACMAXCSMABACKOFFS    0x7000
#define OFF_F_FTDF_ON_OFF_REGMAP_MACMAXCSMABACKOFFS    12
#define SIZE_F_FTDF_ON_OFF_REGMAP_MACMAXCSMABACKOFFS   3

/*
 *
 * REGISTER: ftdf_on_off_regmap_ftdf_ce
 *
 * Initialization value: none.
 * Access mode: EC-R (composite event)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_FTDF_CE  0x40090250
#define SIZE_R_FTDF_ON_OFF_REGMAP_FTDF_CE 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_FTDF_CE  0x3f

/*
 *
 * Field: ftdf_on_off_regmap_ftdf_ce
 *
 * Composite serveice request from ftdf macro (see FR0400 in v40.100.2.41.pdf)
 * Bit 0 = unused
 * Bit 1 = rx interrupts
 * Bit 2 = tx interrupts
 * Bit 3 = miscelaneous interrupts
 * Bit 4 = tx interrupts from pages below (0x3 and 0x4)
 * Bit 5 = Reserved
 */
#define IND_F_FTDF_ON_OFF_REGMAP_FTDF_CE  IND_R_FTDF_ON_OFF_REGMAP_FTDF_CE
#define MSK_F_FTDF_ON_OFF_REGMAP_FTDF_CE  0x3f
#define OFF_F_FTDF_ON_OFF_REGMAP_FTDF_CE  0
#define SIZE_F_FTDF_ON_OFF_REGMAP_FTDF_CE 6

/*
 *
 * REGISTER: ftdf_on_off_regmap_ftdf_cm
 *
 * Initialization value: 0x0  Initialization mask: 0x3f
 * Access mode: ECM-RW (composite event mask)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_FTDF_CM  0x40090254
#define SIZE_R_FTDF_ON_OFF_REGMAP_FTDF_CM 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_FTDF_CM  0x3f

/*
 *
 * Field: ftdf_on_off_regmap_ftdf_cm
 *
 * mask bits for ftf_ce
 */
#define IND_F_FTDF_ON_OFF_REGMAP_FTDF_CM  IND_R_FTDF_ON_OFF_REGMAP_FTDF_CM
#define MSK_F_FTDF_ON_OFF_REGMAP_FTDF_CM  0x3f
#define OFF_F_FTDF_ON_OFF_REGMAP_FTDF_CM  0
#define SIZE_F_FTDF_ON_OFF_REGMAP_FTDF_CM 6

/*
 *
 * REGISTER: ftdf_on_off_regmap_SyncTimeStampThr
 *
 * Initialization value: 0x0  Initialization mask: 0xffffffff
 * Access mode: C-RW (control - read/write)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_SYNCTIMESTAMPTHR  0x40090304
#define SIZE_R_FTDF_ON_OFF_REGMAP_SYNCTIMESTAMPTHR 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_SYNCTIMESTAMPTHR  0xffffffff

/*
 *
 * Field: ftdf_on_off_regmap_SyncTimeStampThr
 *
 * Threshold for synchronize TS gen.
 * Note that due to implementation this register may only be written once per two LP_CLK periods.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_SYNCTIMESTAMPTHR  IND_R_FTDF_ON_OFF_REGMAP_SYNCTIMESTAMPTHR
#define MSK_F_FTDF_ON_OFF_REGMAP_SYNCTIMESTAMPTHR  0xffffffff
#define OFF_F_FTDF_ON_OFF_REGMAP_SYNCTIMESTAMPTHR  0
#define SIZE_F_FTDF_ON_OFF_REGMAP_SYNCTIMESTAMPTHR 32

/*
 *
 * REGISTER: ftdf_on_off_regmap_SyncTimeStampVal
 *
 * Initialization value: 0x0  Initialization mask: 0xffffffff
 * Access mode: C-RW (control - read/write)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_SYNCTIMESTAMPVAL  0x40090308
#define SIZE_R_FTDF_ON_OFF_REGMAP_SYNCTIMESTAMPVAL 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_SYNCTIMESTAMPVAL  0xffffffff

/*
 *
 * Field: ftdf_on_off_regmap_SyncTimeStampVal
 *
 * Value to sync TS gen with.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_SYNCTIMESTAMPVAL  IND_R_FTDF_ON_OFF_REGMAP_SYNCTIMESTAMPVAL
#define MSK_F_FTDF_ON_OFF_REGMAP_SYNCTIMESTAMPVAL  0xffffffff
#define OFF_F_FTDF_ON_OFF_REGMAP_SYNCTIMESTAMPVAL  0
#define SIZE_F_FTDF_ON_OFF_REGMAP_SYNCTIMESTAMPVAL 32

/*
 *
 * REGISTER: ftdf_on_off_regmap_SyncTimeStampPhaseVal
 *
 * Initialization value: 0x0  Initialization mask: 0xff
 * Access mode: C-RW (control - read/write)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_SYNCTIMESTAMPPHASEVAL  0x40090320
#define SIZE_R_FTDF_ON_OFF_REGMAP_SYNCTIMESTAMPPHASEVAL 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_SYNCTIMESTAMPPHASEVAL  0xff

/*
 *
 * Field: ftdf_on_off_regmap_SyncTimeStampPhaseVal
 *
 * Value to sync TS gen phase within a symbol with.
 * Please note the +1 correction needed for most accurate result (+0.5 is than the average error, resulting is a just too fast clock).
 */
#define IND_F_FTDF_ON_OFF_REGMAP_SYNCTIMESTAMPPHASEVAL  IND_R_FTDF_ON_OFF_REGMAP_SYNCTIMESTAMPPHASEVAL
#define MSK_F_FTDF_ON_OFF_REGMAP_SYNCTIMESTAMPPHASEVAL  0xff
#define OFF_F_FTDF_ON_OFF_REGMAP_SYNCTIMESTAMPPHASEVAL  0
#define SIZE_F_FTDF_ON_OFF_REGMAP_SYNCTIMESTAMPPHASEVAL 8

/*
 *
 * REGISTER: ftdf_on_off_regmap_timer_control_1
 *
 * Initialization value: 0x0  Initialization mask: 0x2
 * Access mode: C-RW (control - read/write)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_TIMER_CONTROL_1  0x4009030c
#define SIZE_R_FTDF_ON_OFF_REGMAP_TIMER_CONTROL_1 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_TIMER_CONTROL_1  0x2

/*
 *
 * Bit: ftdf_on_off_regmap_SyncTimeStampEna
 *
 * If set, the TimeStampThr is enabled to generate a sync of the TS gen.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_SYNCTIMESTAMPENA  IND_R_FTDF_ON_OFF_REGMAP_TIMER_CONTROL_1
#define MSK_F_FTDF_ON_OFF_REGMAP_SYNCTIMESTAMPENA  0x2
#define OFF_F_FTDF_ON_OFF_REGMAP_SYNCTIMESTAMPENA  1
#define SIZE_F_FTDF_ON_OFF_REGMAP_SYNCTIMESTAMPENA 1

/*
 *
 * REGISTER: ftdf_on_off_regmap_macTxStdAckFrmCnt
 *
 * Initialization value: none.
 * Access mode: S-R (state - read)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_MACTXSTDACKFRMCNT  0x40090310
#define SIZE_R_FTDF_ON_OFF_REGMAP_MACTXSTDACKFRMCNT 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_MACTXSTDACKFRMCNT  0xffffffff

/*
 *
 * Field: ftdf_on_off_regmap_macTxStdAckFrmCnt
 *
 * Standard Acknowledgment frames transmitted
 */
#define IND_F_FTDF_ON_OFF_REGMAP_MACTXSTDACKFRMCNT  IND_R_FTDF_ON_OFF_REGMAP_MACTXSTDACKFRMCNT
#define MSK_F_FTDF_ON_OFF_REGMAP_MACTXSTDACKFRMCNT  0xffffffff
#define OFF_F_FTDF_ON_OFF_REGMAP_MACTXSTDACKFRMCNT  0
#define SIZE_F_FTDF_ON_OFF_REGMAP_MACTXSTDACKFRMCNT 32

/*
 *
 * REGISTER: ftdf_on_off_regmap_macRxStdAckFrmOkCnt
 *
 * Initialization value: none.
 * Access mode: S-R (state - read)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_MACRXSTDACKFRMOKCNT  0x40090314
#define SIZE_R_FTDF_ON_OFF_REGMAP_MACRXSTDACKFRMOKCNT 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_MACRXSTDACKFRMOKCNT  0xffffffff

/*
 *
 * Field: ftdf_on_off_regmap_macRxStdAckFrmOkCnt
 *
 * Standard Acknowledgment frames received
 */
#define IND_F_FTDF_ON_OFF_REGMAP_MACRXSTDACKFRMOKCNT  IND_R_FTDF_ON_OFF_REGMAP_MACRXSTDACKFRMOKCNT
#define MSK_F_FTDF_ON_OFF_REGMAP_MACRXSTDACKFRMOKCNT  0xffffffff
#define OFF_F_FTDF_ON_OFF_REGMAP_MACRXSTDACKFRMOKCNT  0
#define SIZE_F_FTDF_ON_OFF_REGMAP_MACRXSTDACKFRMOKCNT 32

/*
 *
 * REGISTER: ftdf_on_off_regmap_macRxAddrFailFrmCnt
 *
 * Initialization value: none.
 * Access mode: S-R (state - read)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_MACRXADDRFAILFRMCNT  0x40090318
#define SIZE_R_FTDF_ON_OFF_REGMAP_MACRXADDRFAILFRMCNT 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_MACRXADDRFAILFRMCNT  0xffffffff

/*
 *
 * Field: ftdf_on_off_regmap_macRxAddrFailFrmCnt
 *
 * Frames discarded due to incorrect address or PAN Id
 */
#define IND_F_FTDF_ON_OFF_REGMAP_MACRXADDRFAILFRMCNT  IND_R_FTDF_ON_OFF_REGMAP_MACRXADDRFAILFRMCNT
#define MSK_F_FTDF_ON_OFF_REGMAP_MACRXADDRFAILFRMCNT  0xffffffff
#define OFF_F_FTDF_ON_OFF_REGMAP_MACRXADDRFAILFRMCNT  0
#define SIZE_F_FTDF_ON_OFF_REGMAP_MACRXADDRFAILFRMCNT 32

/*
 *
 * REGISTER: ftdf_on_off_regmap_macRxUnsupFrmCnt
 *
 * Initialization value: none.
 * Access mode: S-R (state - read)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_MACRXUNSUPFRMCNT  0x4009031c
#define SIZE_R_FTDF_ON_OFF_REGMAP_MACRXUNSUPFRMCNT 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_MACRXUNSUPFRMCNT  0xffffffff

/*
 *
 * Field: ftdf_on_off_regmap_macRxUnsupFrmCnt
 *
 * Frames which do pass the checks but are not supported
 */
#define IND_F_FTDF_ON_OFF_REGMAP_MACRXUNSUPFRMCNT  IND_R_FTDF_ON_OFF_REGMAP_MACRXUNSUPFRMCNT
#define MSK_F_FTDF_ON_OFF_REGMAP_MACRXUNSUPFRMCNT  0xffffffff
#define OFF_F_FTDF_ON_OFF_REGMAP_MACRXUNSUPFRMCNT  0
#define SIZE_F_FTDF_ON_OFF_REGMAP_MACRXUNSUPFRMCNT 32

/*
 *
 * REGISTER: ftdf_on_off_regmap_macFCSErrorCount
 *
 * Initialization value: none.
 * Access mode: S-R (state - read)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_MACFCSERRORCOUNT  0x40090340
#define SIZE_R_FTDF_ON_OFF_REGMAP_MACFCSERRORCOUNT 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_MACFCSERRORCOUNT  0xffffffff

/*
 *
 * Field: ftdf_on_off_regmap_macFCSErrorCount
 *
 * The number of received frames that were discarded due to an incorrect FCS.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_MACFCSERRORCOUNT  IND_R_FTDF_ON_OFF_REGMAP_MACFCSERRORCOUNT
#define MSK_F_FTDF_ON_OFF_REGMAP_MACFCSERRORCOUNT  0xffffffff
#define OFF_F_FTDF_ON_OFF_REGMAP_MACFCSERRORCOUNT  0
#define SIZE_F_FTDF_ON_OFF_REGMAP_MACFCSERRORCOUNT 32

/*
 *
 * REGISTER: ftdf_on_off_regmap_LmacReset
 *
 * Initialization value: 0x0  Initialization mask: 0x106df
 * Access mode: OS-W (one-shot - write)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_LMACRESET  0x40090360
#define SIZE_R_FTDF_ON_OFF_REGMAP_LMACRESET 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_LMACRESET  0x106df

/*
 *
 * Bit: ftdf_on_off_regmap_LmacReset_control
 *
 * LmacReset_control: A '1' Resets LMAC Controller (for debug and MLME-reset)
 */
#define IND_F_FTDF_ON_OFF_REGMAP_LMACRESET_CONTROL    IND_R_FTDF_ON_OFF_REGMAP_LMACRESET
#define MSK_F_FTDF_ON_OFF_REGMAP_LMACRESET_CONTROL    0x1
#define OFF_F_FTDF_ON_OFF_REGMAP_LMACRESET_CONTROL    0
#define SIZE_F_FTDF_ON_OFF_REGMAP_LMACRESET_CONTROL   1
/*
 *
 * Bit: ftdf_on_off_regmap_LmacReset_rx
 *
 * LmacReset_rx: A '1' Resets LMAC rx pipeline (for debug and MLME-reset)
 */
#define IND_F_FTDF_ON_OFF_REGMAP_LMACRESET_RX         IND_R_FTDF_ON_OFF_REGMAP_LMACRESET
#define MSK_F_FTDF_ON_OFF_REGMAP_LMACRESET_RX         0x2
#define OFF_F_FTDF_ON_OFF_REGMAP_LMACRESET_RX         1
#define SIZE_F_FTDF_ON_OFF_REGMAP_LMACRESET_RX        1
/*
 *
 * Bit: ftdf_on_off_regmap_LmacReset_tx
 *
 * LmacReset_tx: A '1' Resets LMAC tx pipeline (for debug and MLME-reset)
 */
#define IND_F_FTDF_ON_OFF_REGMAP_LMACRESET_TX         IND_R_FTDF_ON_OFF_REGMAP_LMACRESET
#define MSK_F_FTDF_ON_OFF_REGMAP_LMACRESET_TX         0x4
#define OFF_F_FTDF_ON_OFF_REGMAP_LMACRESET_TX         2
#define SIZE_F_FTDF_ON_OFF_REGMAP_LMACRESET_TX        1
/*
 *
 * Bit: ftdf_on_off_regmap_LmacReset_ahb
 *
 * LmacReset_ahb: A '1' Resets LMAC ahb interface (for debug and MLME-reset)
 */
#define IND_F_FTDF_ON_OFF_REGMAP_LMACRESET_AHB        IND_R_FTDF_ON_OFF_REGMAP_LMACRESET
#define MSK_F_FTDF_ON_OFF_REGMAP_LMACRESET_AHB        0x8
#define OFF_F_FTDF_ON_OFF_REGMAP_LMACRESET_AHB        3
#define SIZE_F_FTDF_ON_OFF_REGMAP_LMACRESET_AHB       1
/*
 *
 * Bit: ftdf_on_off_regmap_LmacReset_oreg
 *
 * LmacReset_oreg: A '1' Resets LMAC on_off regmap (for debug and MLME-reset)
 *
 * #$LmacReset_areg@on_off_regmap
 * #LmacReset_areg: A '1' Resets LMAC always_on regmap (for debug and MLME-reset)
 */
#define IND_F_FTDF_ON_OFF_REGMAP_LMACRESET_OREG       IND_R_FTDF_ON_OFF_REGMAP_LMACRESET
#define MSK_F_FTDF_ON_OFF_REGMAP_LMACRESET_OREG       0x10
#define OFF_F_FTDF_ON_OFF_REGMAP_LMACRESET_OREG       4
#define SIZE_F_FTDF_ON_OFF_REGMAP_LMACRESET_OREG      1
/*
 *
 * Bit: ftdf_on_off_regmap_LmacReset_tstim
 *
 * LmacReset_tstim: A '1' Resets LMAC timestamp timer (for debug and MLME-reset)
 */
#define IND_F_FTDF_ON_OFF_REGMAP_LMACRESET_TSTIM      IND_R_FTDF_ON_OFF_REGMAP_LMACRESET
#define MSK_F_FTDF_ON_OFF_REGMAP_LMACRESET_TSTIM      0x40
#define OFF_F_FTDF_ON_OFF_REGMAP_LMACRESET_TSTIM      6
#define SIZE_F_FTDF_ON_OFF_REGMAP_LMACRESET_TSTIM     1
/*
 *
 * Bit: ftdf_on_off_regmap_LmacReset_sec
 *
 * LmacReset_sec: A '1' Resets LMAC security (for debug and MLME-reset)
 *
 * #$LmacReset_wutim@on_off_regmap
 * #LmacReset_wutim: A '1' Resets LMAC wake-up timer (for debug and MLME-reset)
 */
#define IND_F_FTDF_ON_OFF_REGMAP_LMACRESET_SEC        IND_R_FTDF_ON_OFF_REGMAP_LMACRESET
#define MSK_F_FTDF_ON_OFF_REGMAP_LMACRESET_SEC        0x80
#define OFF_F_FTDF_ON_OFF_REGMAP_LMACRESET_SEC        7
#define SIZE_F_FTDF_ON_OFF_REGMAP_LMACRESET_SEC       1
/*
 *
 * Bit: ftdf_on_off_regmap_LmacReset_count
 *
 * LmacReset_count: A '1' Resets LMAC mac counters (for debug and MLME-reset)
 */
#define IND_F_FTDF_ON_OFF_REGMAP_LMACRESET_COUNT      IND_R_FTDF_ON_OFF_REGMAP_LMACRESET
#define MSK_F_FTDF_ON_OFF_REGMAP_LMACRESET_COUNT      0x200
#define OFF_F_FTDF_ON_OFF_REGMAP_LMACRESET_COUNT      9
#define SIZE_F_FTDF_ON_OFF_REGMAP_LMACRESET_COUNT     1
/*
 *
 * Bit: ftdf_on_off_regmap_LmacReset_timctrl
 *
 * LmacReset_count: A '1' Resets LMAC timing control block (for debug and MLME-reset)
 */
#define IND_F_FTDF_ON_OFF_REGMAP_LMACRESET_TIMCTRL    IND_R_FTDF_ON_OFF_REGMAP_LMACRESET
#define MSK_F_FTDF_ON_OFF_REGMAP_LMACRESET_TIMCTRL    0x400
#define OFF_F_FTDF_ON_OFF_REGMAP_LMACRESET_TIMCTRL    10
#define SIZE_F_FTDF_ON_OFF_REGMAP_LMACRESET_TIMCTRL   1
/*
 *
 * Bit: ftdf_on_off_regmap_LmacGlobReset_count
 *
 * If set, the LMAC performance and traffic counters will be reset.
 * Use this register for functionally reset these counters.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_LMACGLOBRESET_COUNT  IND_R_FTDF_ON_OFF_REGMAP_LMACRESET
#define MSK_F_FTDF_ON_OFF_REGMAP_LMACGLOBRESET_COUNT  0x10000
#define OFF_F_FTDF_ON_OFF_REGMAP_LMACGLOBRESET_COUNT  16
#define SIZE_F_FTDF_ON_OFF_REGMAP_LMACGLOBRESET_COUNT 1

/*
 *
 * REGISTER: ftdf_on_off_regmap_SymbolTimeThr
 *
 * Initialization value: 0x0  Initialization mask: 0xffffffff
 * Access mode: C-RW (control - read/write)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_SYMBOLTIMETHR  0x40090380
#define SIZE_R_FTDF_ON_OFF_REGMAP_SYMBOLTIMETHR 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_SYMBOLTIMETHR  0xffffffff

/*
 *
 * Field: ftdf_on_off_regmap_SymbolTimeThr
 *
 * Symboltime Threshold to generate a general interrupt
 */
#define IND_F_FTDF_ON_OFF_REGMAP_SYMBOLTIMETHR  IND_R_FTDF_ON_OFF_REGMAP_SYMBOLTIMETHR
#define MSK_F_FTDF_ON_OFF_REGMAP_SYMBOLTIMETHR  0xffffffff
#define OFF_F_FTDF_ON_OFF_REGMAP_SYMBOLTIMETHR  0
#define SIZE_F_FTDF_ON_OFF_REGMAP_SYMBOLTIMETHR 32

/*
 *
 * REGISTER: ftdf_on_off_regmap_SymbolTime2Thr
 *
 * Initialization value: 0x0  Initialization mask: 0xffffffff
 * Access mode: C-RW (control - read/write)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_SYMBOLTIME2THR  0x40090384
#define SIZE_R_FTDF_ON_OFF_REGMAP_SYMBOLTIME2THR 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_SYMBOLTIME2THR  0xffffffff

/*
 *
 * Field: ftdf_on_off_regmap_SymbolTime2Thr
 *
 * Symboltime 2 Threshold to generate a general interrupt
 */
#define IND_F_FTDF_ON_OFF_REGMAP_SYMBOLTIME2THR  IND_R_FTDF_ON_OFF_REGMAP_SYMBOLTIME2THR
#define MSK_F_FTDF_ON_OFF_REGMAP_SYMBOLTIME2THR  0xffffffff
#define OFF_F_FTDF_ON_OFF_REGMAP_SYMBOLTIME2THR  0
#define SIZE_F_FTDF_ON_OFF_REGMAP_SYMBOLTIME2THR 32

/*
 *
 * REGISTER: ftdf_on_off_regmap_DebugControl
 *
 * Initialization value: 0x0  Initialization mask: 0x1ff
 * Access mode: C-RW (control - read/write)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_DEBUGCONTROL  0x40090390
#define SIZE_R_FTDF_ON_OFF_REGMAP_DEBUGCONTROL 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_DEBUGCONTROL  0x1ff

/*
 *
 * Bit: ftdf_on_off_regmap_dbg_rx_input
 *
 * If set, the Rx debug interface will be selected as input for the Rx pipeline.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_DBG_RX_INPUT  IND_R_FTDF_ON_OFF_REGMAP_DEBUGCONTROL
#define MSK_F_FTDF_ON_OFF_REGMAP_DBG_RX_INPUT  0x100
#define OFF_F_FTDF_ON_OFF_REGMAP_DBG_RX_INPUT  8
#define SIZE_F_FTDF_ON_OFF_REGMAP_DBG_RX_INPUT 1
/*
 *
 * Field: ftdf_on_off_regmap_dbg_control
 *
 * Control of the debug interface
 *
 * dbg_control 0x00 Not used
 *              diagnose bus bit 7 - 0 = all '0'
 *
 * dbg_control 0x01 Legacy debug mode
 *              diagnose bus bit 7 = ed_request
 *              diagnose bus bit 6 = gen_irq
 *              diagnose bus bit 5 = tx_en
 *              diagnose bus bit 4 = rx_en
 *              diagnose bus bit 3 = phy_en
 *              diagnose bus bit 2 = tx_valid
 *              diagnose bus bit 1 = rx_sof
 *              diagnose bus bit 0 = rx_eof
 *
 * dbg_control 0x02 Clocks and reset
 *              diagnose bus bit 7-3 = "00000"
 *              diagnose bus bit 2   = lp_clk divide by 2
 *              diagnose bus bit 1   = mac_clk divide by 2
 *              diagnose bus bit 0   = hclk divided by 2
 *
 * dbg_control in range 0x02 - 0x0f Not used
 *              diagnose bus bit 7 - 0 = all '0'
 *
 * dbg_control in range 0x10 - 0x1f Debug rx pipeline
 *
 *      0x10	Rx phy signals
 *              diagnose_bus bit 7   = rx_enable towards the rx_pipeline
 *              diagnose bus bit 6   = rx_sof from phy
 *              diagnose bus bit 5-2 = rx_data from phy
 *              diagnose bus bit 1   = rx_eof to phy
 *              diagnose bus bit 0   = rx_sof event from rx_mac_timing block
 *
 *      0x11	Rx sof and length
 *              diagnose_bus bit 7   = rx_sof
 *              diagnose bus bit 6-0 = rx_mac_length from rx_mac_timing block
 *
 *      0x12	Rx mac timing output
 *              diagnose_bus bit 7   = Enable from rx_mac_timing block
 *              diagnose bus bit 6   = Sop    from rx_mac_timing block
 *              diagnose bus bit 5   = Eop    from rx_mac_timing block
 *              diagnose bus bit 4   = Crc    from rx_mac_timing block
 *              diagnose bus bit 3   = Err    from rx_mac_timing block
 *              diagnose bus bit 2   = Drop   from rx_mac_timing block
 *              diagnose bus bit 1   = Forward from rx_mac_timing block
 *              diagnose bus bit 0   = rx_sof
 *
 *      0x13	Rx mac frame parser output
 *              diagnose_bus bit 7   = Enable from mac_frame_parser block
 *              diagnose bus bit 6   = Sop    from  mac_frame_parser block
 *              diagnose bus bit 5   = Eop    from  mac_frame_parser block
 *              diagnose bus bit 4   = Crc    from  mac_frame_parser block
 *              diagnose bus bit 3   = Err    from  mac_frame_parser block
 *              diagnose bus bit 2   = Drop   from  mac_frame_parser block
 *              diagnose bus bit 1   = Forward from mac_frame_parser block
 *              diagnose bus bit 0   = rx_sof
 *
 *      0x14	Rx status to Umac
 *              diagnose_bus bit 7   = rx_frame_stat_umac.crc16_error
 *              diagnose bus bit 6   = rx_frame_stat_umac.res_frm_type_error
 *              diagnose bus bit 5   = rx_frame_stat_umac.res_frm_version_error
 *              diagnose bus bit 4   = rx_frame_stat_umac.dpanid_error
 *              diagnose bus bit 3   = rx_frame_stat_umac.daddr_error
 *              diagnose bus bit 2   = rx_frame_stat_umac.spanid_error
 *              diagnose bus bit 1   = rx_frame_stat_umac.ispan_coord_error
 *              diagnose bus bit 0   = rx_sof
 *
 *      0x15	Rx status to lmac
 *              diagnose_bus bit 7   = rx_frame_stat.packet_valid
 *              diagnose bus bit 6   = rx_frame_stat.rx_sof_e
 *              diagnose bus bit 5   = rx_frame_stat.rx_eof_e
 *              diagnose bus bit 4   = rx_frame_stat.wakeup_frame
 *              diagnose bus bit 3-1 = rx_frame_stat.frame_type
 *              diagnose bus bit 0   = rx_frame_stat.rx_sqnr_valid
 *
 *      0x16	Rx buffer control
 *              diagnose_bus bit 7-4 = prov_from_swio.rx_read_buf_ptr
 *              diagnose bus bit 3-0 = stat_to_swio.rx_write_buf_ptr
 *
 *      0x17	Rx interrupts
 *              diagnose_bus bit 7   = stat_to_swio.rx_buf_avail_e
 *              diagnose bus bit 6   = stat_to_swio.rx_buf_full
 *              diagnose bus bit 5   = stat_to_swio.rx_buf_overflow_e
 *              diagnose bus bit 4   = stat_to_swio.rx_sof_e
 *              diagnose bus bit 3   = stat_to_swio.rxbyte_e
 *              diagnose bus bit 2   = rx_frame_ctrl.rx_enable
 *              diagnose bus bit 1   = rx_eof
 *              diagnose bus bit 0   = rx_sof
 *
 *      0x18	diagnose_bus bit 7-3 =
 *                      Prt statements mac frame parser
 *                         0 Reset
 *                      1 Multipurpose frame with no dpanid and macImplicitBroadcast is false and not pan cooordinator dropped
 *                      2 Multipurpose frame with no dest address and macImplicitBroadcast is false and not pan cooordinator dropped
 *                      3 RX unsupported frame type detected
 *                      4 RX non Beacon frame detected and dropped in RxBeaconOnly mode
 *                      5 RX frame passed due to macAlwaysPassFrmType set
 *                      6 RX unsupported da_mode = 01 for frame_version 0x0- detected
 *                      7 RX unsupported sa_mode = 01 for frame_version 0x0- detected
 *                      8 Data or command frame with no dpanid and macImplicitBroadcast is false and not pan cooordinator dropped
 *                      9 Data or command frame with no dest address and macImplicitBroadcast is false and not pan cooordinator dropped
 *                      10 RX unsupported frame_version detected
 *                      11 Multipurpose frame with no dpanid and macImplicitBroadcast is false and not pan cooordinator dropped
 *                      12 Multipurpose frame with no dest address and macImplicitBroadcast is false and not pan cooordinator dropped
 *                      13 RX unsupported frame_version detected
 *                      14 RX Destination PAN Id check failed
 *                      15 RX Destination address (1 byte)  check failed
 *                      16 RX Destination address (2 byte)  check failed
 *                      17 RX Destination address (8 byte)  check failed
 *                      18 RX Source PAN Id check for Beacon frames failed
 *                      19 RX Source PAN Id check failed
 *                      20 Auxiliary Security Header security control word received
 *                      21 Auxiliary Security Header unsupported_legacy received frame_version 00
 *                      22 Auxiliary Security Header unsupported_legacy security frame_version 11
 *                      23 Auxiliary Security Header frame counter field received
 *                      24 Auxiliary Security Header Key identifier field received
 *                      25 Errored Destination  Wakeup frame found send to Umac, not indicated towards lmac controller
 *                      26 MacAlwaysPassWakeUpFrames = 1, Wakeup frame found send to Umac, not indicated towards lmac controller
 *                      27 Wakeup frame found send to lmac controller and Umac
 *                      28 Wakeup frame found send to lmac controller, dropped towards Umac
 *                      29 Command frame with data_request command received
 *                      30 Coordinator realignment frame received
 *                         31 Not Used
 *              diagnose bus bit 2-0 = frame_type
 *
 *      0x19	diagnose_bus bit 7-4 =
 *                      Framestates mac frame parser
 *                         0 idle_state
 *                      1 fr_ctrl_state
 *                      2 fr_ctrl_long_state
 *                      3 seq_nr_state
 *                      4 dest_pan_id_state
 *                      5 dest_addr_state
 *                      6 src_pan_id_state
 *                      7 src_addr_state
 *                      8 aux_sec_hdr_state
 *                      9 hdr_ie_state
 *                      10 pyld_ie_state
 *                      11 ignore_state
 *                      12-16  Not used
 *              diagnose bus bit 3   = rx_mac_sof_e
 *              diagnose bus bit 2-0 = frame_type
 *
 *      0x1a-0x1f	Not Used
 *              diagnose bus bit 7 - 0 = all '0'
 *
 * dbg_control in range 0x20 - 0x2f Debug tx pipeline
 *      0x20	Top and bottom interfaces tx datapath
 *              diagnose_bus bit 7   = tx_mac_ld_strb
 *              diagnose bus bit 6   = tx_mac_drdy
 *              diagnose bus bit 5   = tx_valid
 *              diagnose bus bit 4-1 = tx_data
 *              diagnose bus bit 0   = symbol_ena
 *
 *      0x21	Control signals for tx datapath
 *              diagnose_bus bit 7   = TxTransparentMode
 *              diagnose bus bit 6   = ctrl_lmac_to_tx.CRC16_ena
 *              diagnose bus bit 5   = ctrl_lmac_to_tx.tx_start
 *              diagnose bus bit 4   = ctrl_lmac_to_tx.gen_ack
 *              diagnose bus bit 3   = ctrl_lmac_to_tx.gen_wu
 *              diagnose bus bit 2-1 = ctrl_lmac_to_tx.tx_frame_nmbr
 *              diagnose bus bit 0   = tx_valid
 *
 *      0x22	Control signals for wakeup frames
 *              diagnose_bus bit 7   = ctrl_lmac_to_tx.tx_start
 *              diagnose bus bit 6   = ctrl_lmac_to_tx.gen_wu
 *              diagnose bus bit 5-1 = ctrl_lmac_to_tx.gen_wu_RZ(4 DOWNTO 0)
 *              diagnose bus bit 0   = ctrl_tx_to_lmac.tx_rz_zero
 *
 *      0x23	Data signals for wakeup frame
 *              diagnose_bus bit 7   = ctrl_lmac_to_tx.tx_start
 *              diagnose bus bit 6   = ctrl_lmac_to_tx.gen_wu
 *              diagnose bus bit 5   = tx_valid
 *              diagnose bus bit 4-1 = tx_data
 *              diagnose bus bit 0   = ctrl_tx_to_lmac.tx_rz_zero
 *
 *      0x24	Data and control ack frame
 *              diagnose_bus bit 7   = ctrl_lmac_to_tx.tx_start
 *              diagnose bus bit 6   = ctrl_lmac_to_tx.gen_ack
 *              diagnose bus bit 5   = tx_valid
 *              diagnose bus bit 4-1 = tx_data
 *              diagnose bus bit 0   = symbol_ena
 *
 *      0x25-0x2f	Not Used
 *              diagnose bus bit 7 - 0 = all '0'
 *
 * dbg_control in range 0x30 - 0x3f Debug lmac_controller
 *      0x30	Phy signals
 *              diagnose_bus bit 7   = phy_en
 *              diagnose bus bit 6   = tx_en
 *              diagnose bus bit 5   = rx_en
 *              diagnose bus bit 4-1 = phyattr(3 downto 0)
 *              diagnose bus bit 0   = ed_request
 *
 *      0x31	Phy enable and detailed statemachine info
 *              diagnose_bus bit 7   = phy_en
 *              diagnose bus bit 6-0 = prt statements detailed statemachine info
 *                      0 Reset
 *                      1 MAIN_STATE_IDLE TX_ACKnowledgement frame skip CSMA_CA
 *                      2 MAIN_STATE_IDLE : Wake-Up frame resides in buffer no
 *                      3 MAIN_STATE_IDLE : TX frame in TSCH mode
 *                      4 MAIN_STATE_IDLE : TX frame
 *                      5 MAIN_STATE_IDLE : goto SINGLE_CCA
 *                      6 MAIN_STATE_IDLE : goto Energy Detection
 *                      7 MAIN_STATE_IDLE : goto RX
 *                      8 MAIN_STATE_IDLE de-assert phy_en wait for
 *                      9 MAIN_STATE_IDLE Start phy_en and assert phy_en wait for :
 *                      10 MAIN_STATE_CSMA_CA_WAIT_FOR_METADATA  framenr :
 *                      11 MAIN_STATE_CSMA_CA framenr :
 *                      12 MAIN_STATE_CSMA_CA start CSMA_CA
 *                      14 MAIN_STATE_CSMA_CA skip CSMA_CA
 *                      15 MAIN_STATE_WAIT_FOR_CSMA_CA csma_ca Failed for CSL mode
 *                      16 MAIN_STATE_WAIT_FOR_CSMA_CA csma_ca Failed
 *                      17 MAIN_STATE_WAIT_FOR_CSMA_CA csma_ca Success
 *                      18 MAIN_STATE_SINGLE_CCA cca failed
 *                      19 MAIN_STATE_SINGLE_CCA cca ok
 *                      20 MAIN_STATE_WAIT_ACK_DELAY wait time elapsed
 *                      21 MAIN_STATE_WAIT ACK_DELAY : Send Ack in TSCH mode
 *                      22 MAIN_STATE_WAIT ACK_DELAY : Ack not scheduled in time in TSCH mode
 *                      23 MAIN_STATE_TSCH_TX_FRAME macTSRxTx time elapsed
 *                      24 MAIN_STATE_TX_FRAME it is CSL mode, start the WU seq for macWUPeriod
 *                      25 MAIN_STATE_TX_FRAME1
 *                      26 MAIN_STATE_TX_FRAME_W1 exit, waited for :
 *                      27 MAIN_STATE_TX_FRAME_W2 exit, waited for :
 *                      28 MAIN_STATE_TX_WAIT_LAST_SYMBOL exit
 *                      29 MAIN_STATE_TX_RAMPDOWN_W exit, waited for :
 *                      30 MAIN_STATE_TX_PHYTRXWAIT exit, waited for :
 *                      31 MAIN_STATE_GEN_IFS , Ack with frame pending received
 *                      32 MAIN_STATE_GEN_IFS , Ack requested but DisRXackReceivedca is set
 *                      33 MAIN_STATE_GEN_IFS , instead of generating an IFS, switch the rx-on for an ACK
 *                      34 MAIN_STATE_GEN_IFS , generating an SIFS (Short Inter Frame Space)
 *                      35 MAIN_STATE_GEN_IFS , generating an LIFS (Long Inter Frame Space)
 *                      36 MAIN_STATE_GEN_IFS_FINISH in CSL mode, 'cslperiod_timer_ready' is ready sent the data frame
 *                      37 MAIN_STATE_GEN_IFS_FINISH in CSL mode, 'Rendezvous zero time not yet transmitted sent another WU frame
 *                      38 MAIN_STATE_GEN_IFS_FINISH exit, corrected for :
 *                      39 MAIN_STATE_GEN_IFS_FINISH in CSL mode, rx_resume_after_tx is set
 *                      40 MAIN_STATE_SENT_STATUS CSLMode wu_sequence_done_flag :
 *                      41 MAIN_STATE_ADD_DELAY exit, goto RX for frame pending time
 *                      42 MAIN_STATE_ADD_DELAY exit, goto IDLE
 *                      43 MAIN_STATE_RX switch the RX from off to on
 *                      44 MAIN_STATE_RX_WAIT_PHYRXSTART waited for
 *                      45 MAIN_STATE_RX_WAIT_PHYRXLATENCY start the MacAckDuration timer
 *                      46 MAIN_STATE_RX_WAIT_PHYRXLATENCY start the ack wait timer
 *                      47 MAIN_STATE_RX_WAIT_PHYRXLATENCY NORMAL mode : start rx_duration
 *                      48 MAIN_STATE_RX_WAIT_PHYRXLATENCY TSCH mode : start macTSRxWait (macTSRxWait) timer
 *                      49 MAIN_STATE_RX_WAIT_PHYRXLATENCY CSL mode : start the csl_rx_duration (macCSLsamplePeriod) timer
 *                      50 MAIN_STATE_RX_WAIT_PHYRXLATENCY CSL mode : start the csl_rx_duration (macCSLdataPeriod) timer
 *                      51 MAIN_STATE_RX_CHK_EXPIRED 1 ackwait_timer_ready or ack_received_ind
 *                      52 MAIN_STATE_RX_CHK_EXPIRED 2 normal_mode. ACK with FP bit set, start frame pending timer
 *                      54 MAIN_STATE_RX_CHK_EXPIRED 4 RxAlwaysOn is set to 0
 *                      55 MAIN_STATE_RX_CHK_EXPIRED 5 TSCH mode valid frame rxed
 *                      56 MAIN_STATE_RX_CHK_EXPIRED 6 TSCH mode macTSRxwait expired
 *                      57 MAIN_STATE_RX_CHK_EXPIRED 7 macCSLsamplePeriod timer ready
 *                      58 MAIN_STATE_RX_CHK_EXPIRED 8 csl_mode. Data frame recieved with FP bit set, increment RX duration time
 *                      59 MAIN_STATE_RX_CHK_EXPIRED 9 CSL mode : Received wu frame in data listening period restart macCSLdataPeriod timer
 *                      60 MAIN_STATE_RX_CHK_EXPIRED 10 csl_rx_duration_ready or csl data frame received:
 *                      61 MAIN_STATE_RX_CHK_EXPIRED 11 normal_mode. FP bit is set, start frame pending timer
 *                      62 MAIN_STATE_RX_CHK_EXPIRED 12 normal mode rx_duration_timer_ready and frame_pending_timer_ready
 *                      63 MAIN_STATE_RX_CHK_EXPIRED 13 acknowledge frame requested
 *                      64 MAIN_STATE_RX_CHK_EXPIRED 14 Interrupt to sent a frame. found_tx_packet
 *                      65 MAIN_STATE_CSL_RX_IDLE_END RZ time is far away, switch receiver Off and wait for csl_rx_rz_time before switching on
 *                      66 MAIN_STATE_CSL_RX_IDLE_END RZ time is not so far away, keep receiver On and wait for
 *                      67 MAIN_STATE_RX_START_PHYRXSTOP ack with fp received keep rx on
 *                      68 MAIN_STATE_RX_START_PHYRXSTOP switch the RX from on to off
 *                      69 MAIN_STATE_RX_WAIT_PHYRXSTOP acknowledge done : switch back to finish TX
 *                      70 MAIN_STATE_RX_WAIT_PHYRXSTOP switch the RX off
 *                      71 MAIN_STATE_ED switch the RX from off to on
 *                      72 MAIN_STATE_ED_WAIT_PHYRXSTART waited for phyRxStartup
 *                      73 MAIN_STATE_ED_WAIT_EDSCANDURATION waited for EdScanDuration
 *                      74 MAIN_STATE_ED_WAIT_PHYRXSTOP end of Energy Detection, got IDLE
 *                         75 - 127 Not used
 *
 *      0x32	RX enable and detailed statemachine info
 *              diagnose_bus bit 7   = rx_en
 *              diagnose bus bit 6-0 = detailed statemachine info see dbg_control = 0x31
 *
 *      0x33	TX enable and detailed statemachine info
 *              diagnose_bus bit 7   = tx_en
 *              diagnose bus bit 6-0 = detailed statemachine info see dbg_control = 0x31
 *
 *      0x34	Phy enable, TX enable and Rx enable and state machine states
 *              diagnose_bus bit 7   = phy_en
 *              diagnose_bus bit 6   = tx_en
 *              diagnose_bus bit 5   = rx_en
 *              diagnose bus bit 4-0 = State machine states
 *                       1  MAIN_STATE_IDLE
 *                       2  MAIN_STATE_CSMA_CA_WAIT_FOR_METADATA
 *                       3  MAIN_STATE_CSMA_CA
 *                       4  MAIN_STATE_WAIT_FOR_CSMA_CA_0
 *                       5  MAIN_STATE_WAIT_FOR_CSMA_CA
 *                       6  MAIN_STATE_SINGLE_CCA
 *                       7  MAIN_STATE_WAIT_ACK_DELAY
 *                       8  MAIN_STATE_TSCH_TX_FRAME
 *                       9  MAIN_STATE_TX_FRAME
 *                       10 MAIN_STATE_TX_FRAME1
 *                       11 MAIN_STATE_TX_FRAME_W1
 *                       12 MAIN_STATE_TX_FRAME_W2
 *                       13 MAIN_STATE_TX_WAIT_LAST_SYMBOL
 *                       14 MAIN_STATE_TX_RAMPDOWN
 *                       15 MAIN_STATE_TX_RAMPDOWN_W
 *                       16 MAIN_STATE_TX_PHYTRXWAIT
 *                       17 MAIN_STATE_GEN_IFS
 *                       18 MAIN_STATE_GEN_IFS_FINISH
 *                       19 MAIN_STATE_SENT_STATUS
 *                       20 MAIN_STATE_ADD_DELAY
 *                       21 MAIN_STATE_TSCH_RX_ACKDELAY
 *                       22 MAIN_STATE_RX
 *                       23 MAIN_STATE_RX_WAIT_PHYRXSTART
 *                       24 MAIN_STATE_RX_WAIT_PHYRXLATENCY
 *                       25 MAIN_STATE_RX_CHK_EXPIRED
 *                       26 MAIN_STATE_CSL_RX_IDLE_END
 *                       27 MAIN_STATE_RX_START_PHYRXSTOP
 *                       28 MAIN_STATE_RX_WAIT_PHYRXSTOP
 *                       29 MAIN_STATE_ED
 *                       30 MAIN_STATE_ED_WAIT_PHYRXSTART
 *                       31 MAIN_STATE_ED_WAIT_EDSCANDURATION
 *
 *      0x35	lmac controller to tx interface with wakeup
 *              diagnose_bus bit 7   = tx_en
 *              diagnose_bus bit 6   = rx_en
 *              diagnose_bus bit 5   = ctrl_lmac_to_tx.gen_wu
 *              diagnose_bus bit 4   = ctrl_lmac_to_tx.gen_ack
 *              diagnose_bus bit 3-2 = ctrl_lmac_to_tx.tx_frame_nmbr
 *              diagnose_bus bit 1   = ctrl_lmac_to_tx.tx_start
 *              diagnose_bus bit 0   = ctrl_tx_to_lmac.tx_last_symbol
 *
 *      0x36	lmac controller to tx interface with frame pending
 *              diagnose_bus bit 7   = tx_en
 *              diagnose_bus bit 6   = rx_en
 *              diagnose_bus bit 5   = ctrl_lmac_to_tx.gen.fp_bit
 *              diagnose_bus bit 4   = ctrl_lmac_to_tx.gen_ack
 *              diagnose_bus bit 3-2 = ctrl_lmac_to_tx.tx_frame_nmbr
 *              diagnose_bus bit 1   = ctrl_lmac_to_tx.tx_start
 *              diagnose_bus bit 0   = ctrl_tx_to_lmac.tx_last_symbol
 *
 *      0x37	rx pipepline to lmac controller interface
 *              diagnose_bus bit 7   = tx_en
 *              diagnose_bus bit 6   = rx_en
 *              diagnose_bus bit 5   = ctrl_rx_to_lmac.rx_sof_e
 *              diagnose_bus bit 4   = ctrl_rx_to_lmac.rx_eof_e
 *              diagnose_bus bit 3   = ctrl_rx_to_lmac.packet_valid
 *              diagnose_bus bit 2   = ack_received_ind
 *              diagnose_bus bit 1   = ack_request_ind
 *              diagnose_bus bit 0   = ctrl_rx_to_lmac.data_request_frame
 *
 *      0x38	csl start
 *              diagnose_bus bit 7   = tx_en
 *              diagnose_bus bit 6   = rx_en
 *              diagnose_bus bit 5   = csl_start
 *              diagnose_bus bit 4-1 = swio_always_on_to_lmac_ctrl.tx_flag_stat
 *              diagnose_bus bit 0   = found_wu_packet;
 *
 *      0x39	Tsch TX ack timing
 *              diagnose_bus bit 7-4 = swio_always_on_to_lmac_ctrl.tx_flag_stat
 *              diagnose_bus bit 3   = tx_en
 *              diagnose_bus bit 2   = rx_en
 *              diagnose_bus bit 1   = ctrl_rx_to_lmac.packet_valid
 *              diagnose_bus bit 0   = one_us_timer_ready
 *
 *      0x3a-0x3f	Not used
 *              diagnose bus bit 7 - 0 = all '0'
 *
 * dbg_control in range 0x40 - 0x4f Debug security
 *      0x40	Security control
 *              diagnose_bus bit 7   = swio_to_security.secStart
 *              diagnose_bus bit 6   = swio_to_security.secTxRxn
 *              diagnose_bus bit 5   = swio_to_security.secAbort
 *              diagnose_bus bit 4   = swio_to_security.secEncDecn
 *              diagnose_bus bit 3   = swio_from_security.secBusy
 *              diagnose_bus bit 2   = swio_from_security.secAuthFail
 *              diagnose_bus bit 1   = swio_from_security.secReady_e
 *              diagnose_bus bit 0   = '0'
 *
 *      0x41	Security ctlid access rx and tx
 *              diagnose_bus bit 7   = tx_sec_ci.ld_strb
 *              diagnose_bus bit 6   = tx_sec_ci.ctlid_rw
 *              diagnose_bus bit 5   = tx_sec_ci.selectp
 *              diagnose_bus bit 4   = tx_sec_co.d_rdy
 *              diagnose_bus bit 3   = rx_sec_ci.ld_strb
 *              diagnose_bus bit 2   = rx_sec_ci.ctlid_rw
 *              diagnose_bus bit 1   = rx_sec_ci.selectp
 *              diagnose_bus bit 0   = rx_sec_co.d_rdy
 *
 *      0x42	Security ctlid access tx with sec_wr_data_valid
 *              diagnose_bus bit 7   = tx_sec_ci.ld_strb
 *              diagnose_bus bit 6   = tx_sec_ci.ctlid_rw
 *              diagnose_bus bit 5   = tx_sec_ci.selectp
 *              diagnose_bus bit 4   = tx_sec_co.d_rdy
 *              diagnose_bus bit 3-0 = sec_wr_data_valid
 *
 *      0x43	Security ctlid access rx with sec_wr_data_valid
 *              diagnose_bus bit 7   = rx_sec_ci.ld_strb
 *              diagnose_bus bit 6   = rx_sec_ci.ctlid_rw
 *              diagnose_bus bit 5   = rx_sec_ci.selectp
 *              diagnose_bus bit 4   = rx_sec_co.d_rdy
 *              diagnose_bus bit 3-0 = sec_wr_data_valid
 *
 *      0x44-0x4f	Not used
 *              diagnose bus bit 7 - 0 = all '0'
 *
 * dbg_control in range 0x50 - 0x5f AHB bus
 *      0x50	ahb bus without hsize
 *              diagnose_bus bit 7   = hsel
 *              diagnose_bus bit 6-5 = htrans
 *              diagnose_bus bit 4   = hwrite
 *              diagnose_bus bit 3   = hready_in
 *              diagnose_bus bit 2   = hready_out
 *              diagnose_bus bit 1-0 = hresp
 *
 *      0x51	ahb bus without hwready_in and hresp(1)
 *              diagnose_bus bit 7   = hsel
 *              diagnose_bus bit 6-5 = htrans
 *              diagnose_bus bit 4   = hwrite
 *              diagnose_bus bit 3-2 = hhsize(1 downto 0)
 *              diagnose_bus bit 1   = hready_out
 *              diagnose_bus bit 0   = hresp(0)
 *
 *      0x52	Amba tx signals
 *              diagnose_bus bit 7   = ai_tx.penable
 *              diagnose_bus bit 6   = ai_tx.psel
 *              diagnose_bus bit 5   = ai_tx.pwrite
 *              diagnose_bus bit 4   = ao_tx.pready
 *              diagnose_bus bit 3   = ao_tx.pslverr
 *              diagnose_bus bit 2-1 = psize_tx
 *              diagnose_bus bit 0   = hready_out
 *
 *      0x53	Amba rx signals
 *              diagnose_bus bit 7   = ai_rx.penable
 *              diagnose_bus bit 6   = ai_rx.psel
 *              diagnose_bus bit 5   = ai_rx.pwrite
 *              diagnose_bus bit 4   = ao_rx.pready
 *              diagnose_bus bit 3   = ao_rx.pslverr
 *              diagnose_bus bit 2-1 = psize_rx
 *              diagnose_bus bit 0   = hready_out
 *
 *      0x54	Amba register map signals
 *              diagnose_bus bit 7   = ai_reg.penable
 *              diagnose_bus bit 6   = ai_reg.psel
 *              diagnose_bus bit 5   = ai_reg.pwrite
 *              diagnose_bus bit 4   = ao_reg.pready
 *              diagnose_bus bit 3   = ao_reg.pslverr
 *              diagnose_bus bit 2-1 = "00"
 *              diagnose_bus bit 0   = hready_out
 *
 *      0x55-0x5f	Not used
 *              diagnose bus bit 7 - 0 = all '0'
 */
#define IND_F_FTDF_ON_OFF_REGMAP_DBG_CONTROL   IND_R_FTDF_ON_OFF_REGMAP_DEBUGCONTROL
#define MSK_F_FTDF_ON_OFF_REGMAP_DBG_CONTROL   0xff
#define OFF_F_FTDF_ON_OFF_REGMAP_DBG_CONTROL   0
#define SIZE_F_FTDF_ON_OFF_REGMAP_DBG_CONTROL  8

/*
 *
 * REGISTER: ftdf_on_off_regmap_txbyte_e
 *
 * Initialization value: 0x0  Initialization mask: 0x3
 * Access mode: E-RCW (event read/clear on write 1)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_TXBYTE_E  0x40090394
#define SIZE_R_FTDF_ON_OFF_REGMAP_TXBYTE_E 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_TXBYTE_E  0x3

/*
 *
 * Bit: ftdf_on_off_regmap_txbyte_e
 *
 * Indicates the first byte of a frame is transmitted
 */
#define IND_F_FTDF_ON_OFF_REGMAP_TXBYTE_E          IND_R_FTDF_ON_OFF_REGMAP_TXBYTE_E
#define MSK_F_FTDF_ON_OFF_REGMAP_TXBYTE_E          0x1
#define OFF_F_FTDF_ON_OFF_REGMAP_TXBYTE_E          0
#define SIZE_F_FTDF_ON_OFF_REGMAP_TXBYTE_E         1
/*
 *
 * Bit: ftdf_on_off_regmap_tx_last_symbol_e
 *
 * Indicates the last symbol of a frame is transmitted
 */
#define IND_F_FTDF_ON_OFF_REGMAP_TX_LAST_SYMBOL_E  IND_R_FTDF_ON_OFF_REGMAP_TXBYTE_E
#define MSK_F_FTDF_ON_OFF_REGMAP_TX_LAST_SYMBOL_E  0x2
#define OFF_F_FTDF_ON_OFF_REGMAP_TX_LAST_SYMBOL_E  1
#define SIZE_F_FTDF_ON_OFF_REGMAP_TX_LAST_SYMBOL_E 1

/*
 *
 * REGISTER: ftdf_on_off_regmap_txbyte_m
 *
 * Initialization value: 0x0  Initialization mask: 0x3
 * Access mode: EM-RW (event mask)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_TXBYTE_M  0x40090398
#define SIZE_R_FTDF_ON_OFF_REGMAP_TXBYTE_M 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_TXBYTE_M  0x3

/*
 *
 * Bit: ftdf_on_off_regmap_txbyte_m
 *
 * Mask bit for event "txbyte_e".
 */
#define IND_F_FTDF_ON_OFF_REGMAP_TXBYTE_M          IND_R_FTDF_ON_OFF_REGMAP_TXBYTE_M
#define MSK_F_FTDF_ON_OFF_REGMAP_TXBYTE_M          0x1
#define OFF_F_FTDF_ON_OFF_REGMAP_TXBYTE_M          0
#define SIZE_F_FTDF_ON_OFF_REGMAP_TXBYTE_M         1
/*
 *
 * Bit: ftdf_on_off_regmap_tx_last_symbol_m
 *
 * Mask bit for event "tx_last_symbol_e".
 */
#define IND_F_FTDF_ON_OFF_REGMAP_TX_LAST_SYMBOL_M  IND_R_FTDF_ON_OFF_REGMAP_TXBYTE_M
#define MSK_F_FTDF_ON_OFF_REGMAP_TX_LAST_SYMBOL_M  0x2
#define OFF_F_FTDF_ON_OFF_REGMAP_TX_LAST_SYMBOL_M  1
#define SIZE_F_FTDF_ON_OFF_REGMAP_TX_LAST_SYMBOL_M 1

/*
 *
 * REGISTER: ftdf_on_off_regmap_tx_flag_s
 *
 * Initialization value: none.
 * Access mode: S-R (state - read)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_TX_FLAG_S    0x40090400
#define SIZE_R_FTDF_ON_OFF_REGMAP_TX_FLAG_S   0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_TX_FLAG_S    0x1
#define FTDF_ON_OFF_REGMAP_TX_FLAG_S_NUM      0x4
#define FTDF_ON_OFF_REGMAP_TX_FLAG_S_INTVL    0x20
#define FTDF_ON_OFF_REGMAP_TX_FLAG_STAT_NUM   0x4
#define FTDF_ON_OFF_REGMAP_TX_FLAG_STAT_INTVL 0x20

/*
 *
 * Bit: ftdf_on_off_regmap_tx_flag_stat
 *
 * Packet is ready for transmission
 */
#define IND_F_FTDF_ON_OFF_REGMAP_TX_FLAG_STAT  IND_R_FTDF_ON_OFF_REGMAP_TX_FLAG_S
#define MSK_F_FTDF_ON_OFF_REGMAP_TX_FLAG_STAT  0x1
#define OFF_F_FTDF_ON_OFF_REGMAP_TX_FLAG_STAT  0
#define SIZE_F_FTDF_ON_OFF_REGMAP_TX_FLAG_STAT 1

/*
 *
 * REGISTER: ftdf_on_off_regmap_tx_flag_clear_e
 *
 * Initialization value: 0x0  Initialization mask: 0x1
 * Access mode: E-RCW (event read/clear on write 1)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_TX_FLAG_CLEAR_E  0x40090404
#define SIZE_R_FTDF_ON_OFF_REGMAP_TX_FLAG_CLEAR_E 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_TX_FLAG_CLEAR_E  0x1
#define FTDF_ON_OFF_REGMAP_TX_FLAG_CLEAR_E_NUM    0x4
#define FTDF_ON_OFF_REGMAP_TX_FLAG_CLEAR_E_INTVL  0x20
#define FTDF_ON_OFF_REGMAP_TX_FLAG_CLEAR_E_NUM    0x4
#define FTDF_ON_OFF_REGMAP_TX_FLAG_CLEAR_E_INTVL  0x20

/*
 *
 * Bit: ftdf_on_off_regmap_tx_flag_clear_e
 *
 * When the LMAC clears the tx_flag_stat status event bit tx_flag_clear_e is set.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_TX_FLAG_CLEAR_E  IND_R_FTDF_ON_OFF_REGMAP_TX_FLAG_CLEAR_E
#define MSK_F_FTDF_ON_OFF_REGMAP_TX_FLAG_CLEAR_E  0x1
#define OFF_F_FTDF_ON_OFF_REGMAP_TX_FLAG_CLEAR_E  0
#define SIZE_F_FTDF_ON_OFF_REGMAP_TX_FLAG_CLEAR_E 1

/*
 *
 * REGISTER: ftdf_on_off_regmap_tx_flag_clear_m
 *
 * Initialization value: 0x0  Initialization mask: 0x1
 * Access mode: EM-RW (event mask)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_TX_FLAG_CLEAR_M  0x40090408
#define SIZE_R_FTDF_ON_OFF_REGMAP_TX_FLAG_CLEAR_M 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_TX_FLAG_CLEAR_M  0x1
#define FTDF_ON_OFF_REGMAP_TX_FLAG_CLEAR_M_NUM    0x4
#define FTDF_ON_OFF_REGMAP_TX_FLAG_CLEAR_M_INTVL  0x20
#define FTDF_ON_OFF_REGMAP_TX_FLAG_CLEAR_M_NUM    0x4
#define FTDF_ON_OFF_REGMAP_TX_FLAG_CLEAR_M_INTVL  0x20

/*
 *
 * Bit: ftdf_on_off_regmap_tx_flag_clear_m
 *
 * Mask bit for event "tx_flag_clear_e".
 */
#define IND_F_FTDF_ON_OFF_REGMAP_TX_FLAG_CLEAR_M  IND_R_FTDF_ON_OFF_REGMAP_TX_FLAG_CLEAR_M
#define MSK_F_FTDF_ON_OFF_REGMAP_TX_FLAG_CLEAR_M  0x1
#define OFF_F_FTDF_ON_OFF_REGMAP_TX_FLAG_CLEAR_M  0
#define SIZE_F_FTDF_ON_OFF_REGMAP_TX_FLAG_CLEAR_M 1

/*
 *
 * REGISTER: ftdf_on_off_regmap_tx_priority
 *
 * Initialization value: 0x0  Initialization mask: 0x1f
 * Access mode: C-RW (control - read/write)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_TX_PRIORITY  0x40090410
#define SIZE_R_FTDF_ON_OFF_REGMAP_TX_PRIORITY 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_TX_PRIORITY  0x1f
#define FTDF_ON_OFF_REGMAP_TX_PRIORITY_NUM    0x4
#define FTDF_ON_OFF_REGMAP_TX_PRIORITY_INTVL  0x20
#define FTDF_ON_OFF_REGMAP_TX_PRIORITY_NUM    0x4
#define FTDF_ON_OFF_REGMAP_TX_PRIORITY_INTVL  0x20
#define FTDF_ON_OFF_REGMAP_ISWAKEUP_NUM       0x4
#define FTDF_ON_OFF_REGMAP_ISWAKEUP_INTVL     0x20

/*
 *
 * Field: ftdf_on_off_regmap_tx_priority
 *
 * Priority of packet
 */
#define IND_F_FTDF_ON_OFF_REGMAP_TX_PRIORITY  IND_R_FTDF_ON_OFF_REGMAP_TX_PRIORITY
#define MSK_F_FTDF_ON_OFF_REGMAP_TX_PRIORITY  0xf
#define OFF_F_FTDF_ON_OFF_REGMAP_TX_PRIORITY  0
#define SIZE_F_FTDF_ON_OFF_REGMAP_TX_PRIORITY 4
/*
 *
 * Bit: ftdf_on_off_regmap_IsWakeUp
 *
 * A basic wake-up frame can be generated by the UMAC in the Tx buffer.
 * The meta data control bit IsWakeUp must be set to indicate that this is a Wake-up frame.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_ISWAKEUP     IND_R_FTDF_ON_OFF_REGMAP_TX_PRIORITY
#define MSK_F_FTDF_ON_OFF_REGMAP_ISWAKEUP     0x10
#define OFF_F_FTDF_ON_OFF_REGMAP_ISWAKEUP     4
#define SIZE_F_FTDF_ON_OFF_REGMAP_ISWAKEUP    1

/*
 *
 * REGISTER: ftdf_on_off_regmap_tx_set_os
 *
 * Initialization value: 0x0  Initialization mask: 0xf
 * Access mode: OS-W (one-shot - write)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_TX_SET_OS  0x40090480
#define SIZE_R_FTDF_ON_OFF_REGMAP_TX_SET_OS 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_TX_SET_OS  0xf

/*
 *
 * Field: ftdf_on_off_regmap_tx_flag_set
 *
 * To set tx_flag_stat
 */
#define IND_F_FTDF_ON_OFF_REGMAP_TX_FLAG_SET  IND_R_FTDF_ON_OFF_REGMAP_TX_SET_OS
#define MSK_F_FTDF_ON_OFF_REGMAP_TX_FLAG_SET  0xf
#define OFF_F_FTDF_ON_OFF_REGMAP_TX_FLAG_SET  0
#define SIZE_F_FTDF_ON_OFF_REGMAP_TX_FLAG_SET 4

/*
 *
 * REGISTER: ftdf_on_off_regmap_tx_clear_os
 *
 * Initialization value: 0x0  Initialization mask: 0xf
 * Access mode: OS-W (one-shot - write)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_TX_CLEAR_OS  0x40090484
#define SIZE_R_FTDF_ON_OFF_REGMAP_TX_CLEAR_OS 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_TX_CLEAR_OS  0xf

/*
 *
 * Field: ftdf_on_off_regmap_tx_flag_clear
 *
 * To clear tx_flag_stat
 */
#define IND_F_FTDF_ON_OFF_REGMAP_TX_FLAG_CLEAR  IND_R_FTDF_ON_OFF_REGMAP_TX_CLEAR_OS
#define MSK_F_FTDF_ON_OFF_REGMAP_TX_FLAG_CLEAR  0xf
#define OFF_F_FTDF_ON_OFF_REGMAP_TX_FLAG_CLEAR  0
#define SIZE_F_FTDF_ON_OFF_REGMAP_TX_FLAG_CLEAR 4

/*
 *
 * REGISTER: ftdf_always_on_regmap_WakeUpIntThr
 *
 * Initialization value: 0x0  Initialization mask: 0xffffffff
 * Access mode: C-RW (control - read/write)
 */
#define IND_R_FTDF_ALWAYS_ON_REGMAP_WAKEUPINTTHR  0x40091000
#define SIZE_R_FTDF_ALWAYS_ON_REGMAP_WAKEUPINTTHR 0x4
#define MSK_R_FTDF_ALWAYS_ON_REGMAP_WAKEUPINTTHR  0xffffffff

/*
 *
 * Field: ftdf_always_on_regmap_WakeUpIntThr
 *
 * Threshold for wake-up interrupt.
 */
#define IND_F_FTDF_ALWAYS_ON_REGMAP_WAKEUPINTTHR  IND_R_FTDF_ALWAYS_ON_REGMAP_WAKEUPINTTHR
#define MSK_F_FTDF_ALWAYS_ON_REGMAP_WAKEUPINTTHR  0xffffffff
#define OFF_F_FTDF_ALWAYS_ON_REGMAP_WAKEUPINTTHR  0
#define SIZE_F_FTDF_ALWAYS_ON_REGMAP_WAKEUPINTTHR 32

/*
 *
 * REGISTER: ftdf_always_on_regmap_wakeup_control
 *
 * Initialization value: 0x0  Initialization mask: 0x3
 * Access mode: C-RW (control - read/write)
 */
#define IND_R_FTDF_ALWAYS_ON_REGMAP_WAKEUP_CONTROL  0x40091004
#define SIZE_R_FTDF_ALWAYS_ON_REGMAP_WAKEUP_CONTROL 0x4
#define MSK_R_FTDF_ALWAYS_ON_REGMAP_WAKEUP_CONTROL  0x3

/*
 *
 * Bit: ftdf_always_on_regmap_WakeUpEnable
 *
 * If set, the WakeUpIntThr is enabled to generate an interrupt.
 */
#define IND_F_FTDF_ALWAYS_ON_REGMAP_WAKEUPENABLE       IND_R_FTDF_ALWAYS_ON_REGMAP_WAKEUP_CONTROL
#define MSK_F_FTDF_ALWAYS_ON_REGMAP_WAKEUPENABLE       0x2
#define OFF_F_FTDF_ALWAYS_ON_REGMAP_WAKEUPENABLE       1
#define SIZE_F_FTDF_ALWAYS_ON_REGMAP_WAKEUPENABLE      1
/*
 *
 * Bit: ftdf_always_on_regmap_WakeupTimerEnable
 *
 * A '1' Enables the wakeup timer.
 * Note that in on_off_regmap, the register WakeupTimerEnableStatus shows the status of this register after being clocked by LP_CLK.
 * Checking this register can be used as indication for software that this bit is effective in the desing.
 */
#define IND_F_FTDF_ALWAYS_ON_REGMAP_WAKEUPTIMERENABLE  IND_R_FTDF_ALWAYS_ON_REGMAP_WAKEUP_CONTROL
#define MSK_F_FTDF_ALWAYS_ON_REGMAP_WAKEUPTIMERENABLE  0x1
#define OFF_F_FTDF_ALWAYS_ON_REGMAP_WAKEUPTIMERENABLE  0
#define SIZE_F_FTDF_ALWAYS_ON_REGMAP_WAKEUPTIMERENABLE 1
#else // dg_configBLACK_ORCA_IC_REV != BLACK_ORCA_IC_REV_A
/*
 *
 * REGISTER: ftdf_retention_ram_tx_fifo
 *
 * Initialization value: none.
 * Access mode: C-RW (control - read/write)
 */
#define IND_R_FTDF_RETENTION_RAM_TX_FIFO 0x40080000
#define SIZE_R_FTDF_RETENTION_RAM_TX_FIFO 0x4
#define MSK_R_FTDF_RETENTION_RAM_TX_FIFO 0xffffffff
#define FTDF_RETENTION_RAM_TX_FIFO_NUM_1D 0x4
#define FTDF_RETENTION_RAM_TX_FIFO_INTVL_1D 0x80
#define FTDF_RETENTION_RAM_TX_FIFO_NUM_2D 0x20
#define FTDF_RETENTION_RAM_TX_FIFO_INTVL_2D 0x4
#define FTDF_RETENTION_RAM_TX_FIFO_NUM_1D 0x4
#define FTDF_RETENTION_RAM_TX_FIFO_INTVL_1D 0x80
#define FTDF_RETENTION_RAM_TX_FIFO_NUM_2D 0x20
#define FTDF_RETENTION_RAM_TX_FIFO_INTVL_2D 0x4

/*
 *
 * Field: ftdf_retention_ram_tx_fifo
 *
 * Transmit fifo buffer, contains 32 addresses per entry (32b x 32a = 128B). There are 4 entries supported.
 * Note that, despite the name, this fifo is NOT retained when the LMAC is put into deep-sleep!
 */
#define IND_F_FTDF_RETENTION_RAM_TX_FIFO IND_R_FTDF_RETENTION_RAM_TX_FIFO
#define MSK_F_FTDF_RETENTION_RAM_TX_FIFO 0xffffffff
#define OFF_F_FTDF_RETENTION_RAM_TX_FIFO 0
#define SIZE_F_FTDF_RETENTION_RAM_TX_FIFO 32

/*
 *
 * REGISTER: ftdf_retention_ram_tx_meta_data_0
 *
 * Initialization value: none.
 * Access mode: C-RW (control - read/write)
 */
#define IND_R_FTDF_RETENTION_RAM_TX_META_DATA_0 0x40080200
#define SIZE_R_FTDF_RETENTION_RAM_TX_META_DATA_0 0x4
#define MSK_R_FTDF_RETENTION_RAM_TX_META_DATA_0 0x57ffffff
#define FTDF_RETENTION_RAM_TX_META_DATA_0_NUM 0x4
#define FTDF_RETENTION_RAM_TX_META_DATA_0_INTVL 0x10
#define FTDF_RETENTION_RAM_FRAME_LENGTH_NUM 0x4
#define FTDF_RETENTION_RAM_FRAME_LENGTH_INTVL 0x10
#define FTDF_RETENTION_RAM_PHYATTR_NUM 0x4
#define FTDF_RETENTION_RAM_PHYATTR_INTVL 0x10
#define FTDF_RETENTION_RAM_FRAMETYPE_NUM 0x4
#define FTDF_RETENTION_RAM_FRAMETYPE_INTVL 0x10
#define FTDF_RETENTION_RAM_CSMACA_ENA_NUM 0x4
#define FTDF_RETENTION_RAM_CSMACA_ENA_INTVL 0x10
#define FTDF_RETENTION_RAM_ACKREQUEST_NUM 0x4
#define FTDF_RETENTION_RAM_ACKREQUEST_INTVL 0x10
#define FTDF_RETENTION_RAM_CRC16_ENA_NUM 0x4
#define FTDF_RETENTION_RAM_CRC16_ENA_INTVL 0x10

/*
 *
 * Field: ftdf_retention_ram_frame_length
 *
 * Tx meta data per entry: Frame length (in bytes)
 */
#define IND_F_FTDF_RETENTION_RAM_FRAME_LENGTH IND_R_FTDF_RETENTION_RAM_TX_META_DATA_0
#define MSK_F_FTDF_RETENTION_RAM_FRAME_LENGTH 0x7f
#define OFF_F_FTDF_RETENTION_RAM_FRAME_LENGTH 0
#define SIZE_F_FTDF_RETENTION_RAM_FRAME_LENGTH 7
/*
 *
 * Field: ftdf_retention_ram_phyAttr
 *
 * Tx meta data per entry: during the Tx-on phase, the control register phyRxAttr[16] sources the PHYATTR output signal to the DPHY.
 */
#define IND_F_FTDF_RETENTION_RAM_PHYATTR IND_R_FTDF_RETENTION_RAM_TX_META_DATA_0
#define MSK_F_FTDF_RETENTION_RAM_PHYATTR 0x7fff80
#define OFF_F_FTDF_RETENTION_RAM_PHYATTR 7
#define SIZE_F_FTDF_RETENTION_RAM_PHYATTR 16
/*
 *
 * Field: ftdf_retention_ram_frametype
 *
 * Tx meta data per entry: the frame type of the data to be transmitted (Data/Cmd/Ack/wakeup frame/etc.).
 */
#define IND_F_FTDF_RETENTION_RAM_FRAMETYPE IND_R_FTDF_RETENTION_RAM_TX_META_DATA_0
#define MSK_F_FTDF_RETENTION_RAM_FRAMETYPE 0x3800000
#define OFF_F_FTDF_RETENTION_RAM_FRAMETYPE 23
#define SIZE_F_FTDF_RETENTION_RAM_FRAMETYPE 3
/*
 *
 * Bit: ftdf_retention_ram_CsmaCa_ena
 *
 * Tx meta data per entry: '1' indicates that a CSMA-CA is required for the transmission of this packet.
 */
#define IND_F_FTDF_RETENTION_RAM_CSMACA_ENA IND_R_FTDF_RETENTION_RAM_TX_META_DATA_0
#define MSK_F_FTDF_RETENTION_RAM_CSMACA_ENA 0x4000000
#define OFF_F_FTDF_RETENTION_RAM_CSMACA_ENA 26
#define SIZE_F_FTDF_RETENTION_RAM_CSMACA_ENA 1
/*
 *
 * Bit: ftdf_retention_ram_AckRequest
 *
 * Tx meta data per entry: '1' indicates that an acknowledge is expected from the recipient of this packet.
 */
#define IND_F_FTDF_RETENTION_RAM_ACKREQUEST IND_R_FTDF_RETENTION_RAM_TX_META_DATA_0
#define MSK_F_FTDF_RETENTION_RAM_ACKREQUEST 0x10000000
#define OFF_F_FTDF_RETENTION_RAM_ACKREQUEST 28
#define SIZE_F_FTDF_RETENTION_RAM_ACKREQUEST 1
/*
 *
 * Bit: ftdf_retention_ram_CRC16_ena
 *
 * Tx meta data per entry: Indicates whether CRC16 insertion must be enabled or not.
 *  0 : No hardware inserted CRC16
 *  1 : Hardware inserts CRC16
 */
#define IND_F_FTDF_RETENTION_RAM_CRC16_ENA IND_R_FTDF_RETENTION_RAM_TX_META_DATA_0
#define MSK_F_FTDF_RETENTION_RAM_CRC16_ENA 0x40000000
#define OFF_F_FTDF_RETENTION_RAM_CRC16_ENA 30
#define SIZE_F_FTDF_RETENTION_RAM_CRC16_ENA 1

/*
 *
 * REGISTER: ftdf_retention_ram_tx_meta_data_1
 *
 * Initialization value: none.
 * Access mode: C-RW (control - read/write)
 */
#define IND_R_FTDF_RETENTION_RAM_TX_META_DATA_1 0x40080204
#define SIZE_R_FTDF_RETENTION_RAM_TX_META_DATA_1 0x4
#define MSK_R_FTDF_RETENTION_RAM_TX_META_DATA_1 0xff
#define FTDF_RETENTION_RAM_TX_META_DATA_1_NUM 0x4
#define FTDF_RETENTION_RAM_TX_META_DATA_1_INTVL 0x10
#define FTDF_RETENTION_RAM_MACSN_NUM 0x4
#define FTDF_RETENTION_RAM_MACSN_INTVL 0x10

/*
 *
 * Field: ftdf_retention_ram_macSN
 *
 * Tx meta data per entry: Sequence Number of this packet.
 */
#define IND_F_FTDF_RETENTION_RAM_MACSN IND_R_FTDF_RETENTION_RAM_TX_META_DATA_1
#define MSK_F_FTDF_RETENTION_RAM_MACSN 0xff
#define OFF_F_FTDF_RETENTION_RAM_MACSN 0
#define SIZE_F_FTDF_RETENTION_RAM_MACSN 8

/*
 *
 * REGISTER: ftdf_retention_ram_tx_return_status_0
 *
 * Initialization value: none.
 * Access mode: S-R (state - read)
 */
#define IND_R_FTDF_RETENTION_RAM_TX_RETURN_STATUS_0 0x40080240
#define SIZE_R_FTDF_RETENTION_RAM_TX_RETURN_STATUS_0 0x4
#define MSK_R_FTDF_RETENTION_RAM_TX_RETURN_STATUS_0 0xffffffff
#define FTDF_RETENTION_RAM_TX_RETURN_STATUS_0_NUM 0x4
#define FTDF_RETENTION_RAM_TX_RETURN_STATUS_0_INTVL 0x10
#define FTDF_RETENTION_RAM_TXTIMESTAMP_NUM 0x4
#define FTDF_RETENTION_RAM_TXTIMESTAMP_INTVL 0x10

/*
 *
 * Field: ftdf_retention_ram_TxTimestamp
 *
 * Tx return status per entry: Transmit Timestamp
 * The Timestamp of the transmitted packet.
 */
#define IND_F_FTDF_RETENTION_RAM_TXTIMESTAMP IND_R_FTDF_RETENTION_RAM_TX_RETURN_STATUS_0
#define MSK_F_FTDF_RETENTION_RAM_TXTIMESTAMP 0xffffffff
#define OFF_F_FTDF_RETENTION_RAM_TXTIMESTAMP 0
#define SIZE_F_FTDF_RETENTION_RAM_TXTIMESTAMP 32

/*
 *
 * REGISTER: ftdf_retention_ram_tx_return_status_1
 *
 * Initialization value: none.
 * Access mode: S-R (state - read)
 */
#define IND_R_FTDF_RETENTION_RAM_TX_RETURN_STATUS_1 0x40080244
#define SIZE_R_FTDF_RETENTION_RAM_TX_RETURN_STATUS_1 0x4
#define MSK_R_FTDF_RETENTION_RAM_TX_RETURN_STATUS_1 0x1f
#define FTDF_RETENTION_RAM_TX_RETURN_STATUS_1_NUM 0x4
#define FTDF_RETENTION_RAM_TX_RETURN_STATUS_1_INTVL 0x10
#define FTDF_RETENTION_RAM_ACKFAIL_NUM 0x4
#define FTDF_RETENTION_RAM_ACKFAIL_INTVL 0x10
#define FTDF_RETENTION_RAM_CSMACAFAIL_NUM 0x4
#define FTDF_RETENTION_RAM_CSMACAFAIL_INTVL 0x10
#define FTDF_RETENTION_RAM_CSMACANRRETRIES_NUM 0x4
#define FTDF_RETENTION_RAM_CSMACANRRETRIES_INTVL 0x10

/*
 *
 * Bit: ftdf_retention_ram_AckFail
 *
 * Tx return status per entry: Acknowledgement status
 *  0 : SUCCESS
 *  1 : FAIL
 */
#define IND_F_FTDF_RETENTION_RAM_ACKFAIL IND_R_FTDF_RETENTION_RAM_TX_RETURN_STATUS_1
#define MSK_F_FTDF_RETENTION_RAM_ACKFAIL 0x1
#define OFF_F_FTDF_RETENTION_RAM_ACKFAIL 0
#define SIZE_F_FTDF_RETENTION_RAM_ACKFAIL 1
/*
 *
 * Bit: ftdf_retention_ram_CsmaCaFail
 *
 * Tx return status per entry: CSMA-CA status
 *  0 : SUCCESS
 *  1 : FAIL
 */
#define IND_F_FTDF_RETENTION_RAM_CSMACAFAIL IND_R_FTDF_RETENTION_RAM_TX_RETURN_STATUS_1
#define MSK_F_FTDF_RETENTION_RAM_CSMACAFAIL 0x2
#define OFF_F_FTDF_RETENTION_RAM_CSMACAFAIL 1
#define SIZE_F_FTDF_RETENTION_RAM_CSMACAFAIL 1
/*
 *
 * Field: ftdf_retention_ram_CsmaCaNrRetries
 *
 * Tx return status per entry: Number of CSMA-CA retries before this frame has been transmitted
 */
#define IND_F_FTDF_RETENTION_RAM_CSMACANRRETRIES IND_R_FTDF_RETENTION_RAM_TX_RETURN_STATUS_1
#define MSK_F_FTDF_RETENTION_RAM_CSMACANRRETRIES 0x1c
#define OFF_F_FTDF_RETENTION_RAM_CSMACANRRETRIES 2
#define SIZE_F_FTDF_RETENTION_RAM_CSMACANRRETRIES 3

/*
 *
 * REGISTER: ftdf_retention_ram_rx_meta_0
 *
 * Initialization value: none.
 * Access mode: S-R (state - read)
 */
#define IND_R_FTDF_RETENTION_RAM_RX_META_0 0x40080280
#define SIZE_R_FTDF_RETENTION_RAM_RX_META_0 0x4
#define MSK_R_FTDF_RETENTION_RAM_RX_META_0 0xffffffff
#define FTDF_RETENTION_RAM_RX_META_0_NUM 0x8
#define FTDF_RETENTION_RAM_RX_META_0_INTVL 0x10
#define FTDF_RETENTION_RAM_RX_TIMESTAMP_NUM 0x8
#define FTDF_RETENTION_RAM_RX_TIMESTAMP_INTVL 0x10

/*
 *
 * Field: ftdf_retention_ram_rx_timestamp
 *
 * Rx meta data per entry: Timestamp taken when frame was received
 */
#define IND_F_FTDF_RETENTION_RAM_RX_TIMESTAMP IND_R_FTDF_RETENTION_RAM_RX_META_0
#define MSK_F_FTDF_RETENTION_RAM_RX_TIMESTAMP 0xffffffff
#define OFF_F_FTDF_RETENTION_RAM_RX_TIMESTAMP 0
#define SIZE_F_FTDF_RETENTION_RAM_RX_TIMESTAMP 32

/*
 *
 * REGISTER: ftdf_retention_ram_rx_meta_1
 *
 * Initialization value: none.
 * Access mode: S-R (state - read)
 */
#define IND_R_FTDF_RETENTION_RAM_RX_META_1 0x40080284
#define SIZE_R_FTDF_RETENTION_RAM_RX_META_1 0x4
#define MSK_R_FTDF_RETENTION_RAM_RX_META_1 0xfffd
#define FTDF_RETENTION_RAM_RX_META_1_NUM 0x8
#define FTDF_RETENTION_RAM_RX_META_1_INTVL 0x10
#define FTDF_RETENTION_RAM_CRC16_ERROR_NUM 0x8
#define FTDF_RETENTION_RAM_CRC16_ERROR_INTVL 0x10
#define FTDF_RETENTION_RAM_RES_FRM_TYPE_ERROR_NUM 0x8
#define FTDF_RETENTION_RAM_RES_FRM_TYPE_ERROR_INTVL 0x10
#define FTDF_RETENTION_RAM_RES_FRM_VERSION_ERROR_NUM 0x8
#define FTDF_RETENTION_RAM_RES_FRM_VERSION_ERROR_INTVL 0x10
#define FTDF_RETENTION_RAM_DPANID_ERROR_NUM 0x8
#define FTDF_RETENTION_RAM_DPANID_ERROR_INTVL 0x10
#define FTDF_RETENTION_RAM_DADDR_ERROR_NUM 0x8
#define FTDF_RETENTION_RAM_DADDR_ERROR_INTVL 0x10
#define FTDF_RETENTION_RAM_SPANID_ERROR_NUM 0x8
#define FTDF_RETENTION_RAM_SPANID_ERROR_INTVL 0x10
#define FTDF_RETENTION_RAM_ISPANID_COORD_ERROR_NUM 0x8
#define FTDF_RETENTION_RAM_ISPANID_COORD_ERROR_INTVL 0x10
#define FTDF_RETENTION_RAM_QUALITY_INDICATOR_NUM 0x8
#define FTDF_RETENTION_RAM_QUALITY_INDICATOR_INTVL 0x10

/*
 *
 * Bit: ftdf_retention_ram_crc16_error
 *
 * Rx meta data per entry: if set, a CRC error has occurred in this frame, applicable for transparent mode only
 */
#define IND_F_FTDF_RETENTION_RAM_CRC16_ERROR IND_R_FTDF_RETENTION_RAM_RX_META_1
#define MSK_F_FTDF_RETENTION_RAM_CRC16_ERROR 0x1
#define OFF_F_FTDF_RETENTION_RAM_CRC16_ERROR 0
#define SIZE_F_FTDF_RETENTION_RAM_CRC16_ERROR 1
/*
 *
 * Bit: ftdf_retention_ram_res_frm_type_error
 *
 * Rx meta data per entry: if set to '1' this frame is a not supported frame type, applicable when frame is not discarded
 */
#define IND_F_FTDF_RETENTION_RAM_RES_FRM_TYPE_ERROR IND_R_FTDF_RETENTION_RAM_RX_META_1
#define MSK_F_FTDF_RETENTION_RAM_RES_FRM_TYPE_ERROR 0x4
#define OFF_F_FTDF_RETENTION_RAM_RES_FRM_TYPE_ERROR 2
#define SIZE_F_FTDF_RETENTION_RAM_RES_FRM_TYPE_ERROR 1
/*
 *
 * Bit: ftdf_retention_ram_res_frm_version_error
 *
 * Rx meta data per entry: if set to '1' this frame is a not supported frame version, applicable when frame is not discarded.
 */
#define IND_F_FTDF_RETENTION_RAM_RES_FRM_VERSION_ERROR IND_R_FTDF_RETENTION_RAM_RX_META_1
#define MSK_F_FTDF_RETENTION_RAM_RES_FRM_VERSION_ERROR 0x8
#define OFF_F_FTDF_RETENTION_RAM_RES_FRM_VERSION_ERROR 3
#define SIZE_F_FTDF_RETENTION_RAM_RES_FRM_VERSION_ERROR 1
/*
 *
 * Bit: ftdf_retention_ram_dpanid_error
 *
 * Rx meta data per entry: if set to '1', a destination PAN ID error has occurred, applicable when frame is not discarded
 */
#define IND_F_FTDF_RETENTION_RAM_DPANID_ERROR IND_R_FTDF_RETENTION_RAM_RX_META_1
#define MSK_F_FTDF_RETENTION_RAM_DPANID_ERROR 0x10
#define OFF_F_FTDF_RETENTION_RAM_DPANID_ERROR 4
#define SIZE_F_FTDF_RETENTION_RAM_DPANID_ERROR 1
/*
 *
 * Bit: ftdf_retention_ram_daddr_error
 *
 * Rx meta data per entry: if set to '1', a destination Address error has occurred, applicable when frame is not discarded
 */
#define IND_F_FTDF_RETENTION_RAM_DADDR_ERROR IND_R_FTDF_RETENTION_RAM_RX_META_1
#define MSK_F_FTDF_RETENTION_RAM_DADDR_ERROR 0x20
#define OFF_F_FTDF_RETENTION_RAM_DADDR_ERROR 5
#define SIZE_F_FTDF_RETENTION_RAM_DADDR_ERROR 1
/*
 *
 * Bit: ftdf_retention_ram_spanid_error
 *
 * Rx meta data per entry: if set to '1', a PAN ID error has occurred, applicable when frame is not discarded
 */
#define IND_F_FTDF_RETENTION_RAM_SPANID_ERROR IND_R_FTDF_RETENTION_RAM_RX_META_1
#define MSK_F_FTDF_RETENTION_RAM_SPANID_ERROR 0x40
#define OFF_F_FTDF_RETENTION_RAM_SPANID_ERROR 6
#define SIZE_F_FTDF_RETENTION_RAM_SPANID_ERROR 1
/*
 *
 * Bit: ftdf_retention_ram_ispanid_coord_error
 *
 * Rx meta data per entry: if set to '1', the received frame is not for PAN coordinator, applicable when frame is not discarded
 */
#define IND_F_FTDF_RETENTION_RAM_ISPANID_COORD_ERROR IND_R_FTDF_RETENTION_RAM_RX_META_1
#define MSK_F_FTDF_RETENTION_RAM_ISPANID_COORD_ERROR 0x80
#define OFF_F_FTDF_RETENTION_RAM_ISPANID_COORD_ERROR 7
#define SIZE_F_FTDF_RETENTION_RAM_ISPANID_COORD_ERROR 1
/*
 *
 * Field: ftdf_retention_ram_quality_indicator
 *
 * Rx meta data per entry: the Link Quality Indication value during reception of this frame.
 *
 * # $software_scratch@retention_ram
 * # TX ram not used by hardware, can be used by software as scratch ram with retention.
 */
#define IND_F_FTDF_RETENTION_RAM_QUALITY_INDICATOR IND_R_FTDF_RETENTION_RAM_RX_META_1
#define MSK_F_FTDF_RETENTION_RAM_QUALITY_INDICATOR 0xff00
#define OFF_F_FTDF_RETENTION_RAM_QUALITY_INDICATOR 8
#define SIZE_F_FTDF_RETENTION_RAM_QUALITY_INDICATOR 8

/*
 *
 * REGISTER: ftdf_rx_ram_rx_fifo
 *
 * Initialization value: none.
 * Access mode: C-RW (control - read/write)
 */
#define IND_R_FTDF_RX_RAM_RX_FIFO 0x40088000
#define SIZE_R_FTDF_RX_RAM_RX_FIFO 0x4
#define MSK_R_FTDF_RX_RAM_RX_FIFO 0xffffffff
#define FTDF_RX_RAM_RX_FIFO_NUM_1D 0x8
#define FTDF_RX_RAM_RX_FIFO_INTVL_1D 0x80
#define FTDF_RX_RAM_RX_FIFO_NUM_2D 0x20
#define FTDF_RX_RAM_RX_FIFO_INTVL_2D 0x4
#define FTDF_RX_RAM_RX_FIFO_NUM_1D 0x8
#define FTDF_RX_RAM_RX_FIFO_INTVL_1D 0x80
#define FTDF_RX_RAM_RX_FIFO_NUM_2D 0x20
#define FTDF_RX_RAM_RX_FIFO_INTVL_2D 0x4

/*
 *
 * Field: ftdf_rx_ram_rx_fifo
 *
 * Receive fifo ram, contains 32 addresses per entry (32b x 32a = 128B). There are 8 entries supported.
 */
#define IND_F_FTDF_RX_RAM_RX_FIFO IND_R_FTDF_RX_RAM_RX_FIFO
#define MSK_F_FTDF_RX_RAM_RX_FIFO 0xffffffff
#define OFF_F_FTDF_RX_RAM_RX_FIFO 0
#define SIZE_F_FTDF_RX_RAM_RX_FIFO 32

/*
 *
 * REGISTER: ftdf_on_off_regmap_Rel_Name
 *
 * Initialization value: none.
 * Access mode: S-R (state - read)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_REL_NAME 0x40090000
#define SIZE_R_FTDF_ON_OFF_REGMAP_REL_NAME 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_REL_NAME 0xffffffff
#define FTDF_ON_OFF_REGMAP_REL_NAME_NUM 0x4
#define FTDF_ON_OFF_REGMAP_REL_NAME_INTVL 0x4
#define FTDF_ON_OFF_REGMAP_REL_NAME_NUM 0x4
#define FTDF_ON_OFF_REGMAP_REL_NAME_INTVL 0x4

/*
 *
 * Field: ftdf_on_off_regmap_Rel_Name
 *
 * A 4 words wide register, showing in ASCII the name of the release, eg. "ftdf_107".
 */
#define IND_F_FTDF_ON_OFF_REGMAP_REL_NAME IND_R_FTDF_ON_OFF_REGMAP_REL_NAME
#define MSK_F_FTDF_ON_OFF_REGMAP_REL_NAME 0xffffffff
#define OFF_F_FTDF_ON_OFF_REGMAP_REL_NAME 0
#define SIZE_F_FTDF_ON_OFF_REGMAP_REL_NAME 32

/*
 *
 * REGISTER: ftdf_on_off_regmap_BuildTime
 *
 * Initialization value: none.
 * Access mode: S-R (state - read)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_BUILDTIME 0x40090010
#define SIZE_R_FTDF_ON_OFF_REGMAP_BUILDTIME 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_BUILDTIME 0xffffffff
#define FTDF_ON_OFF_REGMAP_BUILDTIME_NUM 0x4
#define FTDF_ON_OFF_REGMAP_BUILDTIME_INTVL 0x4
#define FTDF_ON_OFF_REGMAP_BUILDTIME_NUM 0x4
#define FTDF_ON_OFF_REGMAP_BUILDTIME_INTVL 0x4

/*
 *
 * Field: ftdf_on_off_regmap_BuildTime
 *
 * A 4 words wide register, showing in ASCII the build date (dd mmm yy) and time (hh:mm) of device, eg. "01 Dec 14 14:10 ".
 */
#define IND_F_FTDF_ON_OFF_REGMAP_BUILDTIME IND_R_FTDF_ON_OFF_REGMAP_BUILDTIME
#define MSK_F_FTDF_ON_OFF_REGMAP_BUILDTIME 0xffffffff
#define OFF_F_FTDF_ON_OFF_REGMAP_BUILDTIME 0
#define SIZE_F_FTDF_ON_OFF_REGMAP_BUILDTIME 32

/*
 *
 * REGISTER: ftdf_on_off_regmap_glob_control_0
 *
 * Initialization value: 0xff00  Initialization mask: 0x6ff0e
 * Access mode: C-RW (control - read/write)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_GLOB_CONTROL_0 0x40090020
#define SIZE_R_FTDF_ON_OFF_REGMAP_GLOB_CONTROL_0 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_GLOB_CONTROL_0 0x6ff0e

/*
 *
 * Bit: ftdf_on_off_regmap_IsPANCoordinator
 *
 * Enable/disable receiver check on address fields (0=enabled, 1=disabled)
 */
#define IND_F_FTDF_ON_OFF_REGMAP_ISPANCOORDINATOR IND_R_FTDF_ON_OFF_REGMAP_GLOB_CONTROL_0
#define MSK_F_FTDF_ON_OFF_REGMAP_ISPANCOORDINATOR 0x2
#define OFF_F_FTDF_ON_OFF_REGMAP_ISPANCOORDINATOR 1
#define SIZE_F_FTDF_ON_OFF_REGMAP_ISPANCOORDINATOR 1
/*
 *
 * Bit: ftdf_on_off_regmap_rx_dma_req
 *
 * Source of the RX_DMA_REQ output pin of this block.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_RX_DMA_REQ IND_R_FTDF_ON_OFF_REGMAP_GLOB_CONTROL_0
#define MSK_F_FTDF_ON_OFF_REGMAP_RX_DMA_REQ 0x4
#define OFF_F_FTDF_ON_OFF_REGMAP_RX_DMA_REQ 2
#define SIZE_F_FTDF_ON_OFF_REGMAP_RX_DMA_REQ 1
/*
 *
 * Bit: ftdf_on_off_regmap_tx_dma_req
 *
 * Source of the TX_DMA_REQ output pin of this block.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_TX_DMA_REQ IND_R_FTDF_ON_OFF_REGMAP_GLOB_CONTROL_0
#define MSK_F_FTDF_ON_OFF_REGMAP_TX_DMA_REQ 0x8
#define OFF_F_FTDF_ON_OFF_REGMAP_TX_DMA_REQ 3
#define SIZE_F_FTDF_ON_OFF_REGMAP_TX_DMA_REQ 1
/*
 *
 * Field: ftdf_on_off_regmap_macSimpleAddress
 *
 * Simple address of the PAN coordinator
 */
#define IND_F_FTDF_ON_OFF_REGMAP_MACSIMPLEADDRESS IND_R_FTDF_ON_OFF_REGMAP_GLOB_CONTROL_0
#define MSK_F_FTDF_ON_OFF_REGMAP_MACSIMPLEADDRESS 0xff00
#define OFF_F_FTDF_ON_OFF_REGMAP_MACSIMPLEADDRESS 8
#define SIZE_F_FTDF_ON_OFF_REGMAP_MACSIMPLEADDRESS 8
/*
 *
 * Bit: ftdf_on_off_regmap_macLEenabled
 *
 * If set to '1', the Low Energy mode (also called CSL) is enabled
 */
#define IND_F_FTDF_ON_OFF_REGMAP_MACLEENABLED IND_R_FTDF_ON_OFF_REGMAP_GLOB_CONTROL_0
#define MSK_F_FTDF_ON_OFF_REGMAP_MACLEENABLED 0x20000
#define OFF_F_FTDF_ON_OFF_REGMAP_MACLEENABLED 17
#define SIZE_F_FTDF_ON_OFF_REGMAP_MACLEENABLED 1
/*
 *
 * Bit: ftdf_on_off_regmap_macTSCHenabled
 *
 * If set to '1', the TSCH mode is enabled
 */
#define IND_F_FTDF_ON_OFF_REGMAP_MACTSCHENABLED IND_R_FTDF_ON_OFF_REGMAP_GLOB_CONTROL_0
#define MSK_F_FTDF_ON_OFF_REGMAP_MACTSCHENABLED 0x40000
#define OFF_F_FTDF_ON_OFF_REGMAP_MACTSCHENABLED 18
#define SIZE_F_FTDF_ON_OFF_REGMAP_MACTSCHENABLED 1

/*
 *
 * REGISTER: ftdf_on_off_regmap_glob_control_1
 *
 * Initialization value: 0xffffffff  Initialization mask: 0xffffffff
 * Access mode: C-RW (control - read/write)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_GLOB_CONTROL_1 0x40090024
#define SIZE_R_FTDF_ON_OFF_REGMAP_GLOB_CONTROL_1 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_GLOB_CONTROL_1 0xffffffff

/*
 *
 * Field: ftdf_on_off_regmap_macPANId
 *
 * The PAN ID of this device.
 * The value 0xFFFF indicates that the device is not associated to a PAN.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_MACPANID IND_R_FTDF_ON_OFF_REGMAP_GLOB_CONTROL_1
#define MSK_F_FTDF_ON_OFF_REGMAP_MACPANID 0xffff
#define OFF_F_FTDF_ON_OFF_REGMAP_MACPANID 0
#define SIZE_F_FTDF_ON_OFF_REGMAP_MACPANID 16
/*
 *
 * Field: ftdf_on_off_regmap_macShortAddress
 *
 * The short address of the device. The values 0xFFFF and 0xFFFE indicate that no IEEE Short Address is available.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_MACSHORTADDRESS IND_R_FTDF_ON_OFF_REGMAP_GLOB_CONTROL_1
#define MSK_F_FTDF_ON_OFF_REGMAP_MACSHORTADDRESS 0xffff0000
#define OFF_F_FTDF_ON_OFF_REGMAP_MACSHORTADDRESS 16
#define SIZE_F_FTDF_ON_OFF_REGMAP_MACSHORTADDRESS 16

/*
 *
 * REGISTER: ftdf_on_off_regmap_glob_control_2
 *
 * Initialization value: 0xffffffff  Initialization mask: 0xffffffff
 * Access mode: C-RW (control - read/write)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_GLOB_CONTROL_2 0x40090028
#define SIZE_R_FTDF_ON_OFF_REGMAP_GLOB_CONTROL_2 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_GLOB_CONTROL_2 0xffffffff

/*
 *
 * Field: ftdf_on_off_regmap_aExtendedAddress_l
 *
 * Unique device address, 48 bits wide, lowest 32 bit
 */
#define IND_F_FTDF_ON_OFF_REGMAP_AEXTENDEDADDRESS_L IND_R_FTDF_ON_OFF_REGMAP_GLOB_CONTROL_2
#define MSK_F_FTDF_ON_OFF_REGMAP_AEXTENDEDADDRESS_L 0xffffffff
#define OFF_F_FTDF_ON_OFF_REGMAP_AEXTENDEDADDRESS_L 0
#define SIZE_F_FTDF_ON_OFF_REGMAP_AEXTENDEDADDRESS_L 32

/*
 *
 * REGISTER: ftdf_on_off_regmap_glob_control_3
 *
 * Initialization value: 0xffffffff  Initialization mask: 0xffffffff
 * Access mode: C-RW (control - read/write)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_GLOB_CONTROL_3 0x4009002c
#define SIZE_R_FTDF_ON_OFF_REGMAP_GLOB_CONTROL_3 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_GLOB_CONTROL_3 0xffffffff

/*
 *
 * Field: ftdf_on_off_regmap_aExtendedAddress_h
 *
 * Unique device address, 48 bits wide, highest 16 bit
 */
#define IND_F_FTDF_ON_OFF_REGMAP_AEXTENDEDADDRESS_H IND_R_FTDF_ON_OFF_REGMAP_GLOB_CONTROL_3
#define MSK_F_FTDF_ON_OFF_REGMAP_AEXTENDEDADDRESS_H 0xffffffff
#define OFF_F_FTDF_ON_OFF_REGMAP_AEXTENDEDADDRESS_H 0
#define SIZE_F_FTDF_ON_OFF_REGMAP_AEXTENDEDADDRESS_H 32

/*
 *
 * REGISTER: ftdf_on_off_regmap_lmac_control_0
 *
 * Initialization value: 0x0  Initialization mask: 0xfbfffffe
 * Access mode: C-RW (control - read/write)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_0 0x40090030
#define SIZE_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_0 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_0 0xfbfffffe

/*
 *
 * Field: ftdf_on_off_regmap_RxonDuration
 *
 * The time (in symbol periods) the Rx must be on after setting RxEnable to '1'.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_RXONDURATION IND_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_0
#define MSK_F_FTDF_ON_OFF_REGMAP_RXONDURATION 0x1fffffe
#define OFF_F_FTDF_ON_OFF_REGMAP_RXONDURATION 1
#define SIZE_F_FTDF_ON_OFF_REGMAP_RXONDURATION 24
/*
 *
 * Bit: ftdf_on_off_regmap_RxAlwaysOn
 *
 * If set to '1', the receiver shall be always on if RxEnable is set to '1'.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_RXALWAYSON IND_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_0
#define MSK_F_FTDF_ON_OFF_REGMAP_RXALWAYSON 0x2000000
#define OFF_F_FTDF_ON_OFF_REGMAP_RXALWAYSON 25
#define SIZE_F_FTDF_ON_OFF_REGMAP_RXALWAYSON 1
/*
 *
 * Field: ftdf_on_off_regmap_pti_rx
 *
 * This value will be used during receiving frames, during the auto ACK (when the AR bit is set in the received frame, see [FR0655] and further), a single CCA and ED scan.
 * In TSCH mode this register will be used during the time slot in which frames can be received and consequently an Enhanced ACK can be transmitted.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_PTI_RX IND_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_0
#define MSK_F_FTDF_ON_OFF_REGMAP_PTI_RX 0x78000000
#define OFF_F_FTDF_ON_OFF_REGMAP_PTI_RX 27
#define SIZE_F_FTDF_ON_OFF_REGMAP_PTI_RX 4
/*
 *
 * Bit: ftdf_on_off_regmap_keep_phy_en
 *
 * When the transmit or receive action is ready (LmacReady4Sleep is set), the PHY_EN signal is cleared unless the control register keep_phy_en is set to '1'.
 * When the control register keep_phy_en is set to '1', the signal PHY_EN shall remain being set until the keep_phy_en is cleared.
 * This will help control the behavior of the arbiter between the LMAC and the DPHY.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_KEEP_PHY_EN IND_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_0
#define MSK_F_FTDF_ON_OFF_REGMAP_KEEP_PHY_EN 0x80000000
#define OFF_F_FTDF_ON_OFF_REGMAP_KEEP_PHY_EN 31
#define SIZE_F_FTDF_ON_OFF_REGMAP_KEEP_PHY_EN 1

/*
 *
 * REGISTER: ftdf_on_off_regmap_TxPipePropDelay
 *
 * Initialization value: 0x0  Initialization mask: 0xff
 * Access mode: C-RW (control - read/write)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_TXPIPEPROPDELAY 0x40090034
#define SIZE_R_FTDF_ON_OFF_REGMAP_TXPIPEPROPDELAY 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_TXPIPEPROPDELAY 0xff

/*
 *
 * Field: ftdf_on_off_regmap_TxPipePropDelay
 *
 * Propagation delay (in us) of the tx pipe, between start of transmission (indicated by setting tx_flag_status) to the DPHY.
 * The reset value is 0 us, which is also the closest value to the real implementation figure.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_TXPIPEPROPDELAY IND_R_FTDF_ON_OFF_REGMAP_TXPIPEPROPDELAY
#define MSK_F_FTDF_ON_OFF_REGMAP_TXPIPEPROPDELAY 0xff
#define OFF_F_FTDF_ON_OFF_REGMAP_TXPIPEPROPDELAY 0
#define SIZE_F_FTDF_ON_OFF_REGMAP_TXPIPEPROPDELAY 8

/*
 *
 * REGISTER: ftdf_on_off_regmap_MacAckWaitDuration
 *
 * Initialization value: 0x36  Initialization mask: 0xff
 * Access mode: C-RW (control - read/write)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_MACACKWAITDURATION 0x40090038
#define SIZE_R_FTDF_ON_OFF_REGMAP_MACACKWAITDURATION 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_MACACKWAITDURATION 0xff

/*
 *
 * Field: ftdf_on_off_regmap_MacAckWaitDuration
 *
 * Maximum time (in symbol periods) to wait for an ACK response after transmitting a frame with the AR bit set to '1'.
 * This value is used in the normal mode for the wait of an ACK response, irrespective if the ACK is a normal ACK or an Enhanced ACK.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_MACACKWAITDURATION IND_R_FTDF_ON_OFF_REGMAP_MACACKWAITDURATION
#define MSK_F_FTDF_ON_OFF_REGMAP_MACACKWAITDURATION 0xff
#define OFF_F_FTDF_ON_OFF_REGMAP_MACACKWAITDURATION 0
#define SIZE_F_FTDF_ON_OFF_REGMAP_MACACKWAITDURATION 8

/*
 *
 * REGISTER: ftdf_on_off_regmap_MacEnhAckWaitDuration
 *
 * Initialization value: 0x360  Initialization mask: 0xffff
 * Access mode: C-RW (control - read/write)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_MACENHACKWAITDURATION 0x4009003c
#define SIZE_R_FTDF_ON_OFF_REGMAP_MACENHACKWAITDURATION 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_MACENHACKWAITDURATION 0xffff

/*
 *
 * Field: ftdf_on_off_regmap_MacEnhAckWaitDuration
 *
 * Maximum time (in us) to wait for an Enhanced ACK response after transmitting a frame with the AR bit set to '1'.
 * This value is used in the LE/CSL mode for the wait of an ACK response (which is always an Enhanced ACK).
 */
#define IND_F_FTDF_ON_OFF_REGMAP_MACENHACKWAITDURATION IND_R_FTDF_ON_OFF_REGMAP_MACENHACKWAITDURATION
#define MSK_F_FTDF_ON_OFF_REGMAP_MACENHACKWAITDURATION 0xffff
#define OFF_F_FTDF_ON_OFF_REGMAP_MACENHACKWAITDURATION 0
#define SIZE_F_FTDF_ON_OFF_REGMAP_MACENHACKWAITDURATION 16

/*
 *
 * REGISTER: ftdf_on_off_regmap_lmac_control_1
 *
 * Initialization value: 0x0  Initialization mask: 0xffff
 * Access mode: C-RW (control - read/write)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_1 0x40090040
#define SIZE_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_1 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_1 0xffff

/*
 *
 * Field: ftdf_on_off_regmap_phyRxAttr
 *
 * Value of the PHYATTR bus to DPHY during Rx operation.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_PHYRXATTR IND_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_1
#define MSK_F_FTDF_ON_OFF_REGMAP_PHYRXATTR 0xffff
#define OFF_F_FTDF_ON_OFF_REGMAP_PHYRXATTR 0
#define SIZE_F_FTDF_ON_OFF_REGMAP_PHYRXATTR 16

/*
 *
 * REGISTER: ftdf_on_off_regmap_lmac_control_2
 *
 * Initialization value: 0x0  Initialization mask: 0xffffff01
 * Access mode: C-RW (control - read/write)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_2 0x40090044
#define SIZE_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_2 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_2 0xffffff01

/*
 *
 * Bit: ftdf_on_off_regmap_EdScanEnable
 *
 * If set to '1', the Energy Detect scan will be performed when RxEnable is set to '1' rather than starting a receive action.
 * The length of this scan is defined by EdScanDuration.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_EDSCANENABLE IND_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_2
#define MSK_F_FTDF_ON_OFF_REGMAP_EDSCANENABLE 0x1
#define OFF_F_FTDF_ON_OFF_REGMAP_EDSCANENABLE 0
#define SIZE_F_FTDF_ON_OFF_REGMAP_EDSCANENABLE 1
/*
 *
 * Field: ftdf_on_off_regmap_EdScanDuration
 *
 * The length of ED scan in symbol periods.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_EDSCANDURATION IND_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_2
#define MSK_F_FTDF_ON_OFF_REGMAP_EDSCANDURATION 0xffffff00
#define OFF_F_FTDF_ON_OFF_REGMAP_EDSCANDURATION 8
#define SIZE_F_FTDF_ON_OFF_REGMAP_EDSCANDURATION 24

/*
 *
 * REGISTER: ftdf_on_off_regmap_lmac_control_3
 *
 * Initialization value: 0x70004c4  Initialization mask: 0xfffffff
 * Access mode: C-RW (control - read/write)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_3 0x40090048
#define SIZE_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_3 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_3 0xfffffff

/*
 *
 * Field: ftdf_on_off_regmap_macMaxFrameTotalWaitTime
 *
 * Max time to wait (in symbol periods) for a requested Data Frame or an announced broadcast frame, triggered by the FP bit in the received frame was set to '1'.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_MACMAXFRAMETOTALWAITTIME IND_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_3
#define MSK_F_FTDF_ON_OFF_REGMAP_MACMAXFRAMETOTALWAITTIME 0xffff
#define OFF_F_FTDF_ON_OFF_REGMAP_MACMAXFRAMETOTALWAITTIME 0
#define SIZE_F_FTDF_ON_OFF_REGMAP_MACMAXFRAMETOTALWAITTIME 16
/*
 *
 * Field: ftdf_on_off_regmap_CcaIdleWait
 *
 * Time to wait (in us) after CCA returned "medium idle" before starting TX-ON.
 * Notes:
 * 1) extra wait times are involved before a packet is really transmitted, see the relevant timing figures.
 * 2) not applicable in TSCH mode since there macTSRxTx shall be used.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_CCAIDLEWAIT IND_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_3
#define MSK_F_FTDF_ON_OFF_REGMAP_CCAIDLEWAIT 0xff0000
#define OFF_F_FTDF_ON_OFF_REGMAP_CCAIDLEWAIT 16
#define SIZE_F_FTDF_ON_OFF_REGMAP_CCAIDLEWAIT 8
/*
 *
 * Bit: ftdf_on_off_regmap_Addr_tab_match_FP_value
 *
 * In case the received source address matches with one of the Exp_SA registers, the value of the control register Addr_tap_match_FP_value will be inserted on the position of the FP bit.
 * In case there is no match found, the inverse value of Addr_tap_match_FP_value will be inserted.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_ADDR_TAB_MATCH_FP_VALUE IND_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_3
#define MSK_F_FTDF_ON_OFF_REGMAP_ADDR_TAB_MATCH_FP_VALUE 0x1000000
#define OFF_F_FTDF_ON_OFF_REGMAP_ADDR_TAB_MATCH_FP_VALUE 24
#define SIZE_F_FTDF_ON_OFF_REGMAP_ADDR_TAB_MATCH_FP_VALUE 1
/*
 *
 * Bit: ftdf_on_off_regmap_FP_override
 *
 * In case the control register FP_override is set, the value of the control register FP_force_value will always be the value of the FP bit in the automatic ACK response frame.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_FP_OVERRIDE IND_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_3
#define MSK_F_FTDF_ON_OFF_REGMAP_FP_OVERRIDE 0x2000000
#define OFF_F_FTDF_ON_OFF_REGMAP_FP_OVERRIDE 25
#define SIZE_F_FTDF_ON_OFF_REGMAP_FP_OVERRIDE 1
/*
 *
 * Bit: ftdf_on_off_regmap_FP_force_value
 *
 * In case the control register FP_override is set, the value of the control register FP_force_value will always be the value of the FP bit in the automatic ACK response frame.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_FP_FORCE_VALUE IND_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_3
#define MSK_F_FTDF_ON_OFF_REGMAP_FP_FORCE_VALUE 0x4000000
#define OFF_F_FTDF_ON_OFF_REGMAP_FP_FORCE_VALUE 26
#define SIZE_F_FTDF_ON_OFF_REGMAP_FP_FORCE_VALUE 1
/*
 *
 * Bit: ftdf_on_off_regmap_FTDF_LPDP_Enable
 *
 * If set, not only is FP_override and SA matching done on data_request frames but to all command and data frame types (in normal mode)
 */
#define IND_F_FTDF_ON_OFF_REGMAP_FTDF_LPDP_ENABLE IND_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_3
#define MSK_F_FTDF_ON_OFF_REGMAP_FTDF_LPDP_ENABLE 0x8000000
#define OFF_F_FTDF_ON_OFF_REGMAP_FTDF_LPDP_ENABLE 27
#define SIZE_F_FTDF_ON_OFF_REGMAP_FTDF_LPDP_ENABLE 1

/*
 *
 * REGISTER: ftdf_on_off_regmap_lmac_control_os
 *
 * Initialization value: 0x0  Initialization mask: 0x1f
 * Access mode: OS-W (one-shot - write)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_OS 0x40090050
#define SIZE_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_OS 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_OS 0x1f

/*
 *
 * Bit: ftdf_on_off_regmap_getGeneratorVal
 *
 * If set to '1', the current values of the Wake-up (event) counter/generator (EventCurrVal) and Timestamp (symbol) counter/generator (TimeStampCurrVal and TimeStampCurrPhaseVal) will be captured.
 * Note that this capture actually has been done when getGeneratorVal_e is set (assuming it was cleared in advance).
 */
#define IND_F_FTDF_ON_OFF_REGMAP_GETGENERATORVAL IND_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_OS
#define MSK_F_FTDF_ON_OFF_REGMAP_GETGENERATORVAL 0x1
#define OFF_F_FTDF_ON_OFF_REGMAP_GETGENERATORVAL 0
#define SIZE_F_FTDF_ON_OFF_REGMAP_GETGENERATORVAL 1
/*
 *
 * Bit: ftdf_on_off_regmap_RxEnable
 *
 * If set, receiving data may be done
 */
#define IND_F_FTDF_ON_OFF_REGMAP_RXENABLE IND_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_OS
#define MSK_F_FTDF_ON_OFF_REGMAP_RXENABLE 0x2
#define OFF_F_FTDF_ON_OFF_REGMAP_RXENABLE 1
#define SIZE_F_FTDF_ON_OFF_REGMAP_RXENABLE 1
/*
 *
 * Bit: ftdf_on_off_regmap_SingleCCA
 *
 * If set to '1', a single CCA will be performed.
 * This can be used when e.g. the TSCH timing is not performed by the LMAC but completely by software.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_SINGLECCA IND_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_OS
#define MSK_F_FTDF_ON_OFF_REGMAP_SINGLECCA 0x4
#define OFF_F_FTDF_ON_OFF_REGMAP_SINGLECCA 2
#define SIZE_F_FTDF_ON_OFF_REGMAP_SINGLECCA 1
/*
 *
 * Bit: ftdf_on_off_regmap_Csma_Ca_resume_set
 *
 * If set, Csma_Ca_resume_stat is set
 */
#define IND_F_FTDF_ON_OFF_REGMAP_CSMA_CA_RESUME_SET IND_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_OS
#define MSK_F_FTDF_ON_OFF_REGMAP_CSMA_CA_RESUME_SET 0x8
#define OFF_F_FTDF_ON_OFF_REGMAP_CSMA_CA_RESUME_SET 3
#define SIZE_F_FTDF_ON_OFF_REGMAP_CSMA_CA_RESUME_SET 1
/*
 *
 * Bit: ftdf_on_off_regmap_Csma_Ca_resume_clear
 *
 * If set, Csma_Ca_resume_stat is cleared
 */
#define IND_F_FTDF_ON_OFF_REGMAP_CSMA_CA_RESUME_CLEAR IND_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_OS
#define MSK_F_FTDF_ON_OFF_REGMAP_CSMA_CA_RESUME_CLEAR 0x10
#define OFF_F_FTDF_ON_OFF_REGMAP_CSMA_CA_RESUME_CLEAR 4
#define SIZE_F_FTDF_ON_OFF_REGMAP_CSMA_CA_RESUME_CLEAR 1

/*
 *
 * REGISTER: ftdf_on_off_regmap_lmac_control_status
 *
 * Initialization value: none.
 * Access mode: S-R (state - read)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_STATUS 0x40090054
#define SIZE_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_STATUS 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_STATUS 0xff0fff46

/*
 *
 * Bit: ftdf_on_off_regmap_LmacReady4sleep
 *
 * If set to '1' this register indicates that the LMAC is ready to go to sleep.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_LMACREADY4SLEEP IND_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_STATUS
#define MSK_F_FTDF_ON_OFF_REGMAP_LMACREADY4SLEEP 0x2
#define OFF_F_FTDF_ON_OFF_REGMAP_LMACREADY4SLEEP 1
#define SIZE_F_FTDF_ON_OFF_REGMAP_LMACREADY4SLEEP 1
/*
 *
 * Bit: ftdf_on_off_regmap_CCAStat
 *
 * The value of a single CCA, valid when CCAstat_e is set to '1'.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_CCASTAT IND_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_STATUS
#define MSK_F_FTDF_ON_OFF_REGMAP_CCASTAT 0x4
#define OFF_F_FTDF_ON_OFF_REGMAP_CCASTAT 2
#define SIZE_F_FTDF_ON_OFF_REGMAP_CCASTAT 1
/*
 *
 * Bit: ftdf_on_off_regmap_WakeupTimerEnableStatus
 *
 * Status of WakeupTimerEnable after being clocked by LP_CLK (showing it's effective value).
 * WakeupTimerEnableStatus can be set by setting the one-shot register WakeupTimerEnable_set and cleared by setting the one-shot register WakeupTimerEnable_clear.
 * When WakeupTimerEnableStatus is set (after being cleared), the event counter will be reset to 0x0.
 *
 * This status can be used by software since WakeupTimerEnable is used in the slow LP_CLK area.
 * Rather than waiting for a certain number of LP_CLK periods, simply scanning this status (or enable the interrupt created by WakeupTimerEnableStatus_e) will allow software to determine if this signal has been effected.
 * Note that the rising edge of WakeupTimerEnable will reset the Wake-up (event) counter.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_WAKEUPTIMERENABLESTATUS IND_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_STATUS
#define MSK_F_FTDF_ON_OFF_REGMAP_WAKEUPTIMERENABLESTATUS 0x40
#define OFF_F_FTDF_ON_OFF_REGMAP_WAKEUPTIMERENABLESTATUS 6
#define SIZE_F_FTDF_ON_OFF_REGMAP_WAKEUPTIMERENABLESTATUS 1
/*
 *
 * Field: ftdf_on_off_regmap_EdScanValue
 *
 * The result of an ED scan.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_EDSCANVALUE IND_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_STATUS
#define MSK_F_FTDF_ON_OFF_REGMAP_EDSCANVALUE 0xff00
#define OFF_F_FTDF_ON_OFF_REGMAP_EDSCANVALUE 8
#define SIZE_F_FTDF_ON_OFF_REGMAP_EDSCANVALUE 8
/*
 *
 * Field: ftdf_on_off_regmap_Csma_Ca_NB_stat
 *
 * Current status of the Number of Backoffs.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_CSMA_CA_NB_STAT IND_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_STATUS
#define MSK_F_FTDF_ON_OFF_REGMAP_CSMA_CA_NB_STAT 0x70000
#define OFF_F_FTDF_ON_OFF_REGMAP_CSMA_CA_NB_STAT 16
#define SIZE_F_FTDF_ON_OFF_REGMAP_CSMA_CA_NB_STAT 3
/*
 *
 * Bit: ftdf_on_off_regmap_Csma_Ca_resume_stat
 *
 * In case Csma_Ca_resume_stat is set the LMAC will
 * - use the value of Csma_Ca_NB_val in the CSMA-CA process rather than the initial value 0d.
 * - immediately perform CCA after the sleep, not waiting for the backoff time.
 * - reset Csma_Ca_resume_stat when it resumes CSMA-CA after the sleep.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_CSMA_CA_RESUME_STAT IND_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_STATUS
#define MSK_F_FTDF_ON_OFF_REGMAP_CSMA_CA_RESUME_STAT 0x80000
#define OFF_F_FTDF_ON_OFF_REGMAP_CSMA_CA_RESUME_STAT 19
#define SIZE_F_FTDF_ON_OFF_REGMAP_CSMA_CA_RESUME_STAT 1
/*
 *
 * Field: ftdf_on_off_regmap_Csma_Ca_BO_stat
 *
 * The value of the currently calculated BackOff value. To be used for the sleep time calculation in case of sleep during the BackOff time.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_CSMA_CA_BO_STAT IND_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_STATUS
#define MSK_F_FTDF_ON_OFF_REGMAP_CSMA_CA_BO_STAT 0xff000000
#define OFF_F_FTDF_ON_OFF_REGMAP_CSMA_CA_BO_STAT 24
#define SIZE_F_FTDF_ON_OFF_REGMAP_CSMA_CA_BO_STAT 8

/*
 *
 * REGISTER: ftdf_on_off_regmap_EventCurrVal
 *
 * Initialization value: none.
 * Access mode: S-R (state - read)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_EVENTCURRVAL 0x40090058
#define SIZE_R_FTDF_ON_OFF_REGMAP_EVENTCURRVAL 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_EVENTCURRVAL 0x1ffffff

/*
 *
 * Field: ftdf_on_off_regmap_EventCurrVal
 *
 * The value of the captured Event generator (Wake-up counter) (initiated by getGeneratorVal, valid when getGeneratorVal_e is set).
 */
#define IND_F_FTDF_ON_OFF_REGMAP_EVENTCURRVAL IND_R_FTDF_ON_OFF_REGMAP_EVENTCURRVAL
#define MSK_F_FTDF_ON_OFF_REGMAP_EVENTCURRVAL 0x1ffffff
#define OFF_F_FTDF_ON_OFF_REGMAP_EVENTCURRVAL 0
#define SIZE_F_FTDF_ON_OFF_REGMAP_EVENTCURRVAL 25

/*
 *
 * REGISTER: ftdf_on_off_regmap_TimeStampCurrVal
 *
 * Initialization value: none.
 * Access mode: S-R (state - read)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_TIMESTAMPCURRVAL 0x4009005c
#define SIZE_R_FTDF_ON_OFF_REGMAP_TIMESTAMPCURRVAL 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_TIMESTAMPCURRVAL 0xffffffff

/*
 *
 * Field: ftdf_on_off_regmap_TimeStampCurrVal
 *
 * The value of captured timestamp generator (symbol counter) (initiated by getGeneratorVal, valid when getGeneratorVal_e is set)
 */
#define IND_F_FTDF_ON_OFF_REGMAP_TIMESTAMPCURRVAL IND_R_FTDF_ON_OFF_REGMAP_TIMESTAMPCURRVAL
#define MSK_F_FTDF_ON_OFF_REGMAP_TIMESTAMPCURRVAL 0xffffffff
#define OFF_F_FTDF_ON_OFF_REGMAP_TIMESTAMPCURRVAL 0
#define SIZE_F_FTDF_ON_OFF_REGMAP_TIMESTAMPCURRVAL 32

/*
 *
 * REGISTER: ftdf_on_off_regmap_TimeStampCurrPhaseVal
 *
 * Initialization value: none.
 * Access mode: S-R (state - read)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_TIMESTAMPCURRPHASEVAL 0x40090074
#define SIZE_R_FTDF_ON_OFF_REGMAP_TIMESTAMPCURRPHASEVAL 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_TIMESTAMPCURRPHASEVAL 0xff

/*
 *
 * Field: ftdf_on_off_regmap_TimeStampCurrPhaseVal
 *
 * Value of captured timestamp generator phase within a symbol (initiated by getGeneratorVal, valid when getGeneratorVal_e is set)
 */
#define IND_F_FTDF_ON_OFF_REGMAP_TIMESTAMPCURRPHASEVAL IND_R_FTDF_ON_OFF_REGMAP_TIMESTAMPCURRPHASEVAL
#define MSK_F_FTDF_ON_OFF_REGMAP_TIMESTAMPCURRPHASEVAL 0xff
#define OFF_F_FTDF_ON_OFF_REGMAP_TIMESTAMPCURRPHASEVAL 0
#define SIZE_F_FTDF_ON_OFF_REGMAP_TIMESTAMPCURRPHASEVAL 8

/*
 *
 * REGISTER: ftdf_on_off_regmap_macTSTxAckDelayVal
 *
 * Initialization value: none.
 * Access mode: S-R (state - read)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_MACTSTXACKDELAYVAL 0x40090078
#define SIZE_R_FTDF_ON_OFF_REGMAP_MACTSTXACKDELAYVAL 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_MACTSTXACKDELAYVAL 0xffff

/*
 *
 * Field: ftdf_on_off_regmap_macTSTxAckDelayVal
 *
 * The time (in us) left until the ack frame is sent by the lmac.
 * This can be used by software to determine if there is enough time left to send the, by software created, Enhanced ACK frame.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_MACTSTXACKDELAYVAL IND_R_FTDF_ON_OFF_REGMAP_MACTSTXACKDELAYVAL
#define MSK_F_FTDF_ON_OFF_REGMAP_MACTSTXACKDELAYVAL 0xffff
#define OFF_F_FTDF_ON_OFF_REGMAP_MACTSTXACKDELAYVAL 0
#define SIZE_F_FTDF_ON_OFF_REGMAP_MACTSTXACKDELAYVAL 16

/*
 *
 * REGISTER: ftdf_on_off_regmap_lmac_control_4
 *
 * Initialization value: 0x0  Initialization mask: 0xffffffff
 * Access mode: C-RW (control - read/write)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_4 0x40090060
#define SIZE_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_4 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_4 0xffffffff

/*
 *
 * Field: ftdf_on_off_regmap_phySleepWait
 *
 * The minume time (in us) required between the clear to '0' and set to '1' of PHY_EN.
 * When the signal PHY_EN is deasserted, it will not be asserted within the time phySleepWait.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_PHYSLEEPWAIT IND_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_4
#define MSK_F_FTDF_ON_OFF_REGMAP_PHYSLEEPWAIT 0xff
#define OFF_F_FTDF_ON_OFF_REGMAP_PHYSLEEPWAIT 0
#define SIZE_F_FTDF_ON_OFF_REGMAP_PHYSLEEPWAIT 8
/*
 *
 * Field: ftdf_on_off_regmap_RxPipePropDelay
 *
 * The control register RxPipePropDelay indicates the propagation delay in ~s of the Rx pipeline between the last symbol being captured at the DPHY interface and the "data valid" indication to the LMAC controller.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_RXPIPEPROPDELAY IND_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_4
#define MSK_F_FTDF_ON_OFF_REGMAP_RXPIPEPROPDELAY 0xff00
#define OFF_F_FTDF_ON_OFF_REGMAP_RXPIPEPROPDELAY 8
#define SIZE_F_FTDF_ON_OFF_REGMAP_RXPIPEPROPDELAY 8
/*
 *
 * Field: ftdf_on_off_regmap_phyAckAttr
 *
 * During transmission of an ACK frame, phyAckAttr is used as PHYATTR on the DPHY interface.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_PHYACKATTR IND_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_4
#define MSK_F_FTDF_ON_OFF_REGMAP_PHYACKATTR 0xffff0000
#define OFF_F_FTDF_ON_OFF_REGMAP_PHYACKATTR 16
#define SIZE_F_FTDF_ON_OFF_REGMAP_PHYACKATTR 16

/*
 *
 * REGISTER: ftdf_on_off_regmap_lmac_control_5
 *
 * Initialization value: 0x8c0  Initialization mask: 0xffff0fff
 * Access mode: C-RW (control - read/write)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_5 0x40090064
#define SIZE_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_5 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_5 0xffff0fff

/*
 *
 * Field: ftdf_on_off_regmap_Ack_Response_Delay
 *
 * In order to have some flexibility the control register Ack_Response_Delay indicates the Acknowledge response time in ~s.
 * The default value shall is 192 ~s (12 symbols).
 */
#define IND_F_FTDF_ON_OFF_REGMAP_ACK_RESPONSE_DELAY IND_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_5
#define MSK_F_FTDF_ON_OFF_REGMAP_ACK_RESPONSE_DELAY 0xff
#define OFF_F_FTDF_ON_OFF_REGMAP_ACK_RESPONSE_DELAY 0
#define SIZE_F_FTDF_ON_OFF_REGMAP_ACK_RESPONSE_DELAY 8
/*
 *
 * Field: ftdf_on_off_regmap_CcaStatWait
 *
 * The output CCASTAT is valid after 8 symbols + phyRxStartup.
 * The 8 symbols are programmable by control registerCcaStatWait (in symbol periods).
 * Default value is 8d.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_CCASTATWAIT IND_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_5
#define MSK_F_FTDF_ON_OFF_REGMAP_CCASTATWAIT 0xf00
#define OFF_F_FTDF_ON_OFF_REGMAP_CCASTATWAIT 8
#define SIZE_F_FTDF_ON_OFF_REGMAP_CCASTATWAIT 4
/*
 *
 * Field: ftdf_on_off_regmap_phyCsmaCaAttr
 *
 * During CSMA-CA or CCA, the control register phyCsmaCaAttr is used to source PHYATT on the DPHY interface.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_PHYCSMACAATTR IND_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_5
#define MSK_F_FTDF_ON_OFF_REGMAP_PHYCSMACAATTR 0xffff0000
#define OFF_F_FTDF_ON_OFF_REGMAP_PHYCSMACAATTR 16
#define SIZE_F_FTDF_ON_OFF_REGMAP_PHYCSMACAATTR 16

/*
 *
 * REGISTER: ftdf_on_off_regmap_lmac_control_6
 *
 * Initialization value: 0xc0c28  Initialization mask: 0xffffff
 * Access mode: C-RW (control - read/write)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_6 0x40090068
#define SIZE_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_6 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_6 0xffffff

/*
 *
 * Field: ftdf_on_off_regmap_LifsPeriod
 *
 * The Long IFS period is programmable by LifsPeriod (in symbols).
 * The default is 40 symbols (640 us),
 */
#define IND_F_FTDF_ON_OFF_REGMAP_LIFSPERIOD IND_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_6
#define MSK_F_FTDF_ON_OFF_REGMAP_LIFSPERIOD 0xff
#define OFF_F_FTDF_ON_OFF_REGMAP_LIFSPERIOD 0
#define SIZE_F_FTDF_ON_OFF_REGMAP_LIFSPERIOD 8
/*
 *
 * Field: ftdf_on_off_regmap_SifsPeriod
 *
 * The Short IFS period is programmable by SifsPeriod (in symbols).
 * The default is 12 symbols (192 is).
 */
#define IND_F_FTDF_ON_OFF_REGMAP_SIFSPERIOD IND_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_6
#define MSK_F_FTDF_ON_OFF_REGMAP_SIFSPERIOD 0xff00
#define OFF_F_FTDF_ON_OFF_REGMAP_SIFSPERIOD 8
#define SIZE_F_FTDF_ON_OFF_REGMAP_SIFSPERIOD 8
/*
 *
 * Field: ftdf_on_off_regmap_WUifsPeriod
 *
 * The WakeUp IFS period is programmable by WUifsPeriod (in symbols).
 * The default is 12 symbols (192 us).
 */
#define IND_F_FTDF_ON_OFF_REGMAP_WUIFSPERIOD IND_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_6
#define MSK_F_FTDF_ON_OFF_REGMAP_WUIFSPERIOD 0xff0000
#define OFF_F_FTDF_ON_OFF_REGMAP_WUIFSPERIOD 16
#define SIZE_F_FTDF_ON_OFF_REGMAP_WUIFSPERIOD 8

/*
 *
 * REGISTER: ftdf_on_off_regmap_lmac_control_11
 *
 * Initialization value: 0xff000000  Initialization mask: 0xff0fffff
 * Access mode: C-RW (control - read/write)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_11 0x4009006c
#define SIZE_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_11 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_11 0xff0fffff

/*
 *
 * Field: ftdf_on_off_regmap_macRxTotalCycleTime
 *
 * In CSL mode it can be decided to disable the PHY Rx (Rx-off) after reception of a Wake-up frame and enable the PHY Rx (Rx-on) when the data frame is to be expected, based on the received RZ time.
 * In order to make it easier to calculate if it is efficient to switch to Rx-off and Rx-on again, a control register indicates the time needed to disable and enable the PHY Rx: macRxTotalCycleTime (resolution is 10 symbol periods)
 */
#define IND_F_FTDF_ON_OFF_REGMAP_MACRXTOTALCYCLETIME IND_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_11
#define MSK_F_FTDF_ON_OFF_REGMAP_MACRXTOTALCYCLETIME 0xffff
#define OFF_F_FTDF_ON_OFF_REGMAP_MACRXTOTALCYCLETIME 0
#define SIZE_F_FTDF_ON_OFF_REGMAP_MACRXTOTALCYCLETIME 16
/*
 *
 * Bit: ftdf_on_off_regmap_macDisCaRxOfftoRZ
 *
 * The switching off and on of the PHY Rx (see macRxTotalCycleTime) can be disabled whith the control register macDisCaRxOfftoRZ.
 *  0 : Disabled
 *  1 : Enabled
 */
#define IND_F_FTDF_ON_OFF_REGMAP_MACDISCARXOFFTORZ IND_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_11
#define MSK_F_FTDF_ON_OFF_REGMAP_MACDISCARXOFFTORZ 0x10000
#define OFF_F_FTDF_ON_OFF_REGMAP_MACDISCARXOFFTORZ 16
#define SIZE_F_FTDF_ON_OFF_REGMAP_MACDISCARXOFFTORZ 1
/*
 *
 * Field: ftdf_on_off_regmap_Csma_Ca_NB_val
 *
 * Number of backoffs value in case of a CSMA-CA resume action.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_CSMA_CA_NB_VAL IND_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_11
#define MSK_F_FTDF_ON_OFF_REGMAP_CSMA_CA_NB_VAL 0xe0000
#define OFF_F_FTDF_ON_OFF_REGMAP_CSMA_CA_NB_VAL 17
#define SIZE_F_FTDF_ON_OFF_REGMAP_CSMA_CA_NB_VAL 3
/*
 *
 * Field: ftdf_on_off_regmap_Csma_Ca_BO_threshold
 *
 * If the backoff time calculated in the CSMA-CA procedure as described in [FR3280] is equal to or higher than the control register Csma_Ca_BO_threshold[8] (resolution 320us, see [FR3290]) the event register Csma_Ca_BO_thr_e will be set and an interrupt.
 * In case Csma_Ca_BO_threshold equals 0xFF no check will be performed and consequently Csma_Ca_BO_thr_e will not be set and no interrupt will be generated.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_CSMA_CA_BO_THRESHOLD IND_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_11
#define MSK_F_FTDF_ON_OFF_REGMAP_CSMA_CA_BO_THRESHOLD 0xff000000
#define OFF_F_FTDF_ON_OFF_REGMAP_CSMA_CA_BO_THRESHOLD 24
#define SIZE_F_FTDF_ON_OFF_REGMAP_CSMA_CA_BO_THRESHOLD 8

/*
 *
 * REGISTER: ftdf_on_off_regmap_lmac_control_delta
 *
 * Initialization value: 0x0  Initialization mask: 0x7e
 * Access mode: D-RCW (delta read/clear on write 1)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_DELTA 0x40090070
#define SIZE_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_DELTA 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_DELTA 0x7e

/*
 *
 * Bit: ftdf_on_off_regmap_LmacReady4sleep_d
 *
 * Delta bit for register "LmacReady4sleep".
 * This delta bit is set to '1' on each change of this status, contributes to ftdf_ce[3].
 */
#define IND_F_FTDF_ON_OFF_REGMAP_LMACREADY4SLEEP_D IND_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_DELTA
#define MSK_F_FTDF_ON_OFF_REGMAP_LMACREADY4SLEEP_D 0x2
#define OFF_F_FTDF_ON_OFF_REGMAP_LMACREADY4SLEEP_D 1
#define SIZE_F_FTDF_ON_OFF_REGMAP_LMACREADY4SLEEP_D 1
/*
 *
 * Bit: ftdf_on_off_regmap_SyncTimeStamp_e
 *
 * The SyncTimeStamp_e event is set to '1' when the TimeStampgenerator is loaded with SyncTimeStampVal.
 * This occurs at the rising edge of lp_clk when SyncTimeStampEna is set and the value of the Event generator is equal to the value SyncTimestampThr.
 * This event bit contributes to ftdf_ce[3].
 */
#define IND_F_FTDF_ON_OFF_REGMAP_SYNCTIMESTAMP_E IND_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_DELTA
#define MSK_F_FTDF_ON_OFF_REGMAP_SYNCTIMESTAMP_E 0x4
#define OFF_F_FTDF_ON_OFF_REGMAP_SYNCTIMESTAMP_E 2
#define SIZE_F_FTDF_ON_OFF_REGMAP_SYNCTIMESTAMP_E 1
/*
 *
 * Bit: ftdf_on_off_regmap_SymbolTimeThr_e
 *
 * Event, set to '1' when the symboltime counter matched SymbolTimeThr
 * This event bit contributes to ftdf_ce[3].
 */
#define IND_F_FTDF_ON_OFF_REGMAP_SYMBOLTIMETHR_E IND_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_DELTA
#define MSK_F_FTDF_ON_OFF_REGMAP_SYMBOLTIMETHR_E 0x8
#define OFF_F_FTDF_ON_OFF_REGMAP_SYMBOLTIMETHR_E 3
#define SIZE_F_FTDF_ON_OFF_REGMAP_SYMBOLTIMETHR_E 1
/*
 *
 * Bit: ftdf_on_off_regmap_SymbolTime2Thr_e
 *
 * Event, set to '1' when the symboltime counter matched SymbolTime2Thr
 * This event bit contributes to ftdf_ce[3].
 */
#define IND_F_FTDF_ON_OFF_REGMAP_SYMBOLTIME2THR_E IND_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_DELTA
#define MSK_F_FTDF_ON_OFF_REGMAP_SYMBOLTIME2THR_E 0x10
#define OFF_F_FTDF_ON_OFF_REGMAP_SYMBOLTIME2THR_E 4
#define SIZE_F_FTDF_ON_OFF_REGMAP_SYMBOLTIME2THR_E 1
/*
 *
 * Bit: ftdf_on_off_regmap_getGeneratorVal_e
 *
 * Event, set to '1' to indicate that the the getGeneratorVal request is completed.
 * This event bit contributes to ftdf_ce[3].
 */
#define IND_F_FTDF_ON_OFF_REGMAP_GETGENERATORVAL_E IND_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_DELTA
#define MSK_F_FTDF_ON_OFF_REGMAP_GETGENERATORVAL_E 0x20
#define OFF_F_FTDF_ON_OFF_REGMAP_GETGENERATORVAL_E 5
#define SIZE_F_FTDF_ON_OFF_REGMAP_GETGENERATORVAL_E 1
/*
 *
 * Bit: ftdf_on_off_regmap_WakeupTimerEnableStatus_d
 *
 * Delta which indicates that WakeupTimerEnableStatus has changed
 * This delta bit is set to '1' on each change of this status, contributes to ftdf_ce[3].
 */
#define IND_F_FTDF_ON_OFF_REGMAP_WAKEUPTIMERENABLESTATUS_D IND_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_DELTA
#define MSK_F_FTDF_ON_OFF_REGMAP_WAKEUPTIMERENABLESTATUS_D 0x40
#define OFF_F_FTDF_ON_OFF_REGMAP_WAKEUPTIMERENABLESTATUS_D 6
#define SIZE_F_FTDF_ON_OFF_REGMAP_WAKEUPTIMERENABLESTATUS_D 1

/*
 *
 * REGISTER: ftdf_on_off_regmap_lmac_control_mask
 *
 * Initialization value: 0x0  Initialization mask: 0x7e
 * Access mode: DM-RW (delta mask)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_MASK 0x40090080
#define SIZE_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_MASK 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_MASK 0x7e

/*
 *
 * Bit: ftdf_on_off_regmap_LmacReady4sleep_m
 *
 * Mask bit for delta bit "LmacReady4sleep_d".
 * The mask bit is masking when cleared to '0' (default value) and will enable the contribution to the interrupt when set to '1'.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_LMACREADY4SLEEP_M IND_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_MASK
#define MSK_F_FTDF_ON_OFF_REGMAP_LMACREADY4SLEEP_M 0x2
#define OFF_F_FTDF_ON_OFF_REGMAP_LMACREADY4SLEEP_M 1
#define SIZE_F_FTDF_ON_OFF_REGMAP_LMACREADY4SLEEP_M 1
/*
 *
 * Bit: ftdf_on_off_regmap_SyncTimeStamp_m
 *
 * Mask bit for event register SyncTimeStamp_e.
 * The mask bit is masking when cleared to '0' (default value) and will enable the contribution to the interrupt when set to '1'.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_SYNCTIMESTAMP_M IND_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_MASK
#define MSK_F_FTDF_ON_OFF_REGMAP_SYNCTIMESTAMP_M 0x4
#define OFF_F_FTDF_ON_OFF_REGMAP_SYNCTIMESTAMP_M 2
#define SIZE_F_FTDF_ON_OFF_REGMAP_SYNCTIMESTAMP_M 1
/*
 *
 * Bit: ftdf_on_off_regmap_SymbolTimeThr_m
 *
 * Mask for SymbolTimeThr_e
 * The mask bit is masking when cleared to '0' (default value) and will enable the contribution to the interrupt when set to '1'.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_SYMBOLTIMETHR_M IND_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_MASK
#define MSK_F_FTDF_ON_OFF_REGMAP_SYMBOLTIMETHR_M 0x8
#define OFF_F_FTDF_ON_OFF_REGMAP_SYMBOLTIMETHR_M 3
#define SIZE_F_FTDF_ON_OFF_REGMAP_SYMBOLTIMETHR_M 1
/*
 *
 * Bit: ftdf_on_off_regmap_SymbolTime2Thr_m
 *
 * Mask for SymbolTime2Thr_e
 * The mask bit is masking when cleared to '0' (default value) and will enable the contribution to the interrupt when set to '1'.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_SYMBOLTIME2THR_M IND_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_MASK
#define MSK_F_FTDF_ON_OFF_REGMAP_SYMBOLTIME2THR_M 0x10
#define OFF_F_FTDF_ON_OFF_REGMAP_SYMBOLTIME2THR_M 4
#define SIZE_F_FTDF_ON_OFF_REGMAP_SYMBOLTIME2THR_M 1
/*
 *
 * Bit: ftdf_on_off_regmap_getGeneratorVal_m
 *
 * Mask for getGeneratorVal_e
 * The mask bit is masking when cleared to '0' (default value) and will enable the contribution to the interrupt when set to '1'.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_GETGENERATORVAL_M IND_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_MASK
#define MSK_F_FTDF_ON_OFF_REGMAP_GETGENERATORVAL_M 0x20
#define OFF_F_FTDF_ON_OFF_REGMAP_GETGENERATORVAL_M 5
#define SIZE_F_FTDF_ON_OFF_REGMAP_GETGENERATORVAL_M 1
/*
 *
 * Bit: ftdf_on_off_regmap_WakeupTimerEnableStatus_m
 *
 * Mask for WakeupTimerEnableStatus_d
 * The mask bit is masking when cleared to '0' (default value) and will enable the contribution to the interrupt when set to '1'.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_WAKEUPTIMERENABLESTATUS_M IND_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_MASK
#define MSK_F_FTDF_ON_OFF_REGMAP_WAKEUPTIMERENABLESTATUS_M 0x40
#define OFF_F_FTDF_ON_OFF_REGMAP_WAKEUPTIMERENABLESTATUS_M 6
#define SIZE_F_FTDF_ON_OFF_REGMAP_WAKEUPTIMERENABLESTATUS_M 1

/*
 *
 * REGISTER: ftdf_on_off_regmap_lmac_event
 *
 * Initialization value: 0x0  Initialization mask: 0xf
 * Access mode: E-RCW (event read/clear on write 1)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_LMAC_EVENT 0x40090090
#define SIZE_R_FTDF_ON_OFF_REGMAP_LMAC_EVENT 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_LMAC_EVENT 0xf

/*
 *
 * Bit: ftdf_on_off_regmap_EdScanReady_e
 *
 * The event EdScanReady_e is set to '1' to notify that the ED scan is ready.
 * This event bit contributes to ftdf_ce[1].
 */
#define IND_F_FTDF_ON_OFF_REGMAP_EDSCANREADY_E IND_R_FTDF_ON_OFF_REGMAP_LMAC_EVENT
#define MSK_F_FTDF_ON_OFF_REGMAP_EDSCANREADY_E 0x1
#define OFF_F_FTDF_ON_OFF_REGMAP_EDSCANREADY_E 0
#define SIZE_F_FTDF_ON_OFF_REGMAP_EDSCANREADY_E 1
/*
 *
 * Bit: ftdf_on_off_regmap_CCAstat_e
 *
 * If set to '1', the single CCA is ready
 * This event bit contributes to ftdf_ce[1].
 */
#define IND_F_FTDF_ON_OFF_REGMAP_CCASTAT_E IND_R_FTDF_ON_OFF_REGMAP_LMAC_EVENT
#define MSK_F_FTDF_ON_OFF_REGMAP_CCASTAT_E 0x2
#define OFF_F_FTDF_ON_OFF_REGMAP_CCASTAT_E 1
#define SIZE_F_FTDF_ON_OFF_REGMAP_CCASTAT_E 1
/*
 *
 * Bit: ftdf_on_off_regmap_RxTimerExpired_e
 *
 * Set to '1' if one of the timers enabling the Rx-on mode expires without having received any valid frame.
 * This event bit contributes to ftdf_ce[1].
 */
#define IND_F_FTDF_ON_OFF_REGMAP_RXTIMEREXPIRED_E IND_R_FTDF_ON_OFF_REGMAP_LMAC_EVENT
#define MSK_F_FTDF_ON_OFF_REGMAP_RXTIMEREXPIRED_E 0x4
#define OFF_F_FTDF_ON_OFF_REGMAP_RXTIMEREXPIRED_E 2
#define SIZE_F_FTDF_ON_OFF_REGMAP_RXTIMEREXPIRED_E 1
/*
 *
 * Bit: ftdf_on_off_regmap_Csma_Ca_BO_thr_e
 *
 * If set, the calculated backoff time is more than Csma_Ca_BO_threshold.
 * This event bit contributes to ftdf_ce[1].
 */
#define IND_F_FTDF_ON_OFF_REGMAP_CSMA_CA_BO_THR_E IND_R_FTDF_ON_OFF_REGMAP_LMAC_EVENT
#define MSK_F_FTDF_ON_OFF_REGMAP_CSMA_CA_BO_THR_E 0x8
#define OFF_F_FTDF_ON_OFF_REGMAP_CSMA_CA_BO_THR_E 3
#define SIZE_F_FTDF_ON_OFF_REGMAP_CSMA_CA_BO_THR_E 1

/*
 *
 * REGISTER: ftdf_on_off_regmap_lmac_mask
 *
 * Initialization value: 0x0  Initialization mask: 0xf
 * Access mode: EM-RW (event mask)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_LMAC_MASK 0x40090094
#define SIZE_R_FTDF_ON_OFF_REGMAP_LMAC_MASK 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_LMAC_MASK 0xf

/*
 *
 * Bit: ftdf_on_off_regmap_EdScanReady_m
 *
 * Mask bit for event "EdScanReady_e"
 * The mask bit is masking when cleared to '0' (default value) and will enable the contribution to the interrupt when set to '1'.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_EDSCANREADY_M IND_R_FTDF_ON_OFF_REGMAP_LMAC_MASK
#define MSK_F_FTDF_ON_OFF_REGMAP_EDSCANREADY_M 0x1
#define OFF_F_FTDF_ON_OFF_REGMAP_EDSCANREADY_M 0
#define SIZE_F_FTDF_ON_OFF_REGMAP_EDSCANREADY_M 1
/*
 *
 * Bit: ftdf_on_off_regmap_CCAstat_m
 *
 * Mask bit for event "CCAstat_e"
 * The mask bit is masking when cleared to '0' (default value) and will enable the contribution to the interrupt when set to '1'.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_CCASTAT_M IND_R_FTDF_ON_OFF_REGMAP_LMAC_MASK
#define MSK_F_FTDF_ON_OFF_REGMAP_CCASTAT_M 0x2
#define OFF_F_FTDF_ON_OFF_REGMAP_CCASTAT_M 1
#define SIZE_F_FTDF_ON_OFF_REGMAP_CCASTAT_M 1
/*
 *
 * Bit: ftdf_on_off_regmap_RxTimerExpired_m
 *
 * Mask bit for event "RxTimerExpired_e"
 * The mask bit is masking when cleared to '0' (default value) and will enable the contribution to the interrupt when set to '1'.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_RXTIMEREXPIRED_M IND_R_FTDF_ON_OFF_REGMAP_LMAC_MASK
#define MSK_F_FTDF_ON_OFF_REGMAP_RXTIMEREXPIRED_M 0x4
#define OFF_F_FTDF_ON_OFF_REGMAP_RXTIMEREXPIRED_M 2
#define SIZE_F_FTDF_ON_OFF_REGMAP_RXTIMEREXPIRED_M 1
/*
 *
 * Bit: ftdf_on_off_regmap_Csma_Ca_BO_thr_m
 *
 * Mask bit for event "Csma_Ca_BO_thr_e"
 * The mask bit is masking when cleared to '0' (default value) and will enable the contribution to the interrupt when set to '1'.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_CSMA_CA_BO_THR_M IND_R_FTDF_ON_OFF_REGMAP_LMAC_MASK
#define MSK_F_FTDF_ON_OFF_REGMAP_CSMA_CA_BO_THR_M 0x8
#define OFF_F_FTDF_ON_OFF_REGMAP_CSMA_CA_BO_THR_M 3
#define SIZE_F_FTDF_ON_OFF_REGMAP_CSMA_CA_BO_THR_M 1

/*
 *
 * REGISTER: ftdf_on_off_regmap_lmac_manual_1
 *
 * Initialization value: 0x0  Initialization mask: 0xffff0fff
 * Access mode: C-RW (control - read/write)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_LMAC_MANUAL_1 0x400900a0
#define SIZE_R_FTDF_ON_OFF_REGMAP_LMAC_MANUAL_1 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_LMAC_MANUAL_1 0xffff0fff

/*
 *
 * Bit: ftdf_on_off_regmap_lmac_manual_mode
 *
 * If the control register lmac_manual_mode is set to '1', the LMAC controller control signals should be controlled by the lmac_manual_control registers
 */
#define IND_F_FTDF_ON_OFF_REGMAP_LMAC_MANUAL_MODE IND_R_FTDF_ON_OFF_REGMAP_LMAC_MANUAL_1
#define MSK_F_FTDF_ON_OFF_REGMAP_LMAC_MANUAL_MODE 0x1
#define OFF_F_FTDF_ON_OFF_REGMAP_LMAC_MANUAL_MODE 0
#define SIZE_F_FTDF_ON_OFF_REGMAP_LMAC_MANUAL_MODE 1
/*
 *
 * Bit: ftdf_on_off_regmap_lmac_manual_phy_en
 *
 * lmac_manual_phy_en controls the PHY_EN interface signal when lmac_manual_mode is set to '1'.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_LMAC_MANUAL_PHY_EN IND_R_FTDF_ON_OFF_REGMAP_LMAC_MANUAL_1
#define MSK_F_FTDF_ON_OFF_REGMAP_LMAC_MANUAL_PHY_EN 0x2
#define OFF_F_FTDF_ON_OFF_REGMAP_LMAC_MANUAL_PHY_EN 1
#define SIZE_F_FTDF_ON_OFF_REGMAP_LMAC_MANUAL_PHY_EN 1
/*
 *
 * Bit: ftdf_on_off_regmap_lmac_manual_tx_en
 *
 * lmac_manual_tx_en controls the TX_EN interface signal when lmac_manual_mode is set to '1'.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_LMAC_MANUAL_TX_EN IND_R_FTDF_ON_OFF_REGMAP_LMAC_MANUAL_1
#define MSK_F_FTDF_ON_OFF_REGMAP_LMAC_MANUAL_TX_EN 0x4
#define OFF_F_FTDF_ON_OFF_REGMAP_LMAC_MANUAL_TX_EN 2
#define SIZE_F_FTDF_ON_OFF_REGMAP_LMAC_MANUAL_TX_EN 1
/*
 *
 * Bit: ftdf_on_off_regmap_lmac_manual_rx_en
 *
 * lmac_manual_rx_en controls the RX_EN interface signal when lmac_manual_mode is set to '1'.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_LMAC_MANUAL_RX_EN IND_R_FTDF_ON_OFF_REGMAP_LMAC_MANUAL_1
#define MSK_F_FTDF_ON_OFF_REGMAP_LMAC_MANUAL_RX_EN 0x8
#define OFF_F_FTDF_ON_OFF_REGMAP_LMAC_MANUAL_RX_EN 3
#define SIZE_F_FTDF_ON_OFF_REGMAP_LMAC_MANUAL_RX_EN 1
/*
 *
 * Bit: ftdf_on_off_regmap_lmac_manual_rx_pipe_en
 *
 * lmac_manual_rx_pipe_en controls the rx_enable signal towards the rx pipeline when lmac_manual_mode is set to '1'.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_LMAC_MANUAL_RX_PIPE_EN IND_R_FTDF_ON_OFF_REGMAP_LMAC_MANUAL_1
#define MSK_F_FTDF_ON_OFF_REGMAP_LMAC_MANUAL_RX_PIPE_EN 0x10
#define OFF_F_FTDF_ON_OFF_REGMAP_LMAC_MANUAL_RX_PIPE_EN 4
#define SIZE_F_FTDF_ON_OFF_REGMAP_LMAC_MANUAL_RX_PIPE_EN 1
/*
 *
 * Bit: ftdf_on_off_regmap_lmac_manual_ed_request
 *
 * lmac_manual_ed_request controls the ED_REQUEST interface signal when lmac_manual_mode is set to '1'.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_LMAC_MANUAL_ED_REQUEST IND_R_FTDF_ON_OFF_REGMAP_LMAC_MANUAL_1
#define MSK_F_FTDF_ON_OFF_REGMAP_LMAC_MANUAL_ED_REQUEST 0x20
#define OFF_F_FTDF_ON_OFF_REGMAP_LMAC_MANUAL_ED_REQUEST 5
#define SIZE_F_FTDF_ON_OFF_REGMAP_LMAC_MANUAL_ED_REQUEST 1
/*
 *
 * Field: ftdf_on_off_regmap_lmac_manual_tx_frm_nr
 *
 * lmac_manual_tx_frm_nr controls the entry in the tx buffer to be transmitted when lmac_manual_mode is set to '1'.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_LMAC_MANUAL_TX_FRM_NR IND_R_FTDF_ON_OFF_REGMAP_LMAC_MANUAL_1
#define MSK_F_FTDF_ON_OFF_REGMAP_LMAC_MANUAL_TX_FRM_NR 0xc0
#define OFF_F_FTDF_ON_OFF_REGMAP_LMAC_MANUAL_TX_FRM_NR 6
#define SIZE_F_FTDF_ON_OFF_REGMAP_LMAC_MANUAL_TX_FRM_NR 2
/*
 *
 * Field: ftdf_on_off_regmap_lmac_manual_pti
 *
 * lmac_manual_pti controls the PTI interface signal when lmac_manual_mode is set to '1'.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_LMAC_MANUAL_PTI IND_R_FTDF_ON_OFF_REGMAP_LMAC_MANUAL_1
#define MSK_F_FTDF_ON_OFF_REGMAP_LMAC_MANUAL_PTI 0xf00
#define OFF_F_FTDF_ON_OFF_REGMAP_LMAC_MANUAL_PTI 8
#define SIZE_F_FTDF_ON_OFF_REGMAP_LMAC_MANUAL_PTI 4
/*
 *
 * Field: ftdf_on_off_regmap_lmac_manual_phy_attr
 *
 * lmac_manual_phy_attr controls the PHY_ATTR interface signal when lmac_manual_mode is set to '1'.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_LMAC_MANUAL_PHY_ATTR IND_R_FTDF_ON_OFF_REGMAP_LMAC_MANUAL_1
#define MSK_F_FTDF_ON_OFF_REGMAP_LMAC_MANUAL_PHY_ATTR 0xffff0000
#define OFF_F_FTDF_ON_OFF_REGMAP_LMAC_MANUAL_PHY_ATTR 16
#define SIZE_F_FTDF_ON_OFF_REGMAP_LMAC_MANUAL_PHY_ATTR 16

/*
 *
 * REGISTER: ftdf_on_off_regmap_lmac_manual_os
 *
 * Initialization value: 0x0  Initialization mask: 0x1
 * Access mode: OS-W (one-shot - write)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_LMAC_MANUAL_OS 0x400900a4
#define SIZE_R_FTDF_ON_OFF_REGMAP_LMAC_MANUAL_OS 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_LMAC_MANUAL_OS 0x1

/*
 *
 * Bit: ftdf_on_off_regmap_lmac_manual_tx_start
 *
 * One shot register which triggers the transmission of a frame from the tx buffer in lmac_manual_mode to '1'.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_LMAC_MANUAL_TX_START IND_R_FTDF_ON_OFF_REGMAP_LMAC_MANUAL_OS
#define MSK_F_FTDF_ON_OFF_REGMAP_LMAC_MANUAL_TX_START 0x1
#define OFF_F_FTDF_ON_OFF_REGMAP_LMAC_MANUAL_TX_START 0
#define SIZE_F_FTDF_ON_OFF_REGMAP_LMAC_MANUAL_TX_START 1

/*
 *
 * REGISTER: ftdf_on_off_regmap_lmac_manual_status
 *
 * Initialization value: none.
 * Access mode: S-R (state - read)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_LMAC_MANUAL_STATUS 0x400900a8
#define SIZE_R_FTDF_ON_OFF_REGMAP_LMAC_MANUAL_STATUS 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_LMAC_MANUAL_STATUS 0xff01

/*
 *
 * Bit: ftdf_on_off_regmap_lmac_manual_cca_stat
 *
 * lmac_manual_cca_stat shows the status of the CCA_STAT DPHY interface signal.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_LMAC_MANUAL_CCA_STAT IND_R_FTDF_ON_OFF_REGMAP_LMAC_MANUAL_STATUS
#define MSK_F_FTDF_ON_OFF_REGMAP_LMAC_MANUAL_CCA_STAT 0x1
#define OFF_F_FTDF_ON_OFF_REGMAP_LMAC_MANUAL_CCA_STAT 0
#define SIZE_F_FTDF_ON_OFF_REGMAP_LMAC_MANUAL_CCA_STAT 1
/*
 *
 * Field: ftdf_on_off_regmap_lmac_manual_ed_stat
 *
 * lmac_manual_ed_stat shows the status of the ED_STAT DPHY interface signal.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_LMAC_MANUAL_ED_STAT IND_R_FTDF_ON_OFF_REGMAP_LMAC_MANUAL_STATUS
#define MSK_F_FTDF_ON_OFF_REGMAP_LMAC_MANUAL_ED_STAT 0xff00
#define OFF_F_FTDF_ON_OFF_REGMAP_LMAC_MANUAL_ED_STAT 8
#define SIZE_F_FTDF_ON_OFF_REGMAP_LMAC_MANUAL_ED_STAT 8

/*
 *
 * REGISTER: ftdf_on_off_regmap_lmac_control_7
 *
 * Initialization value: 0x420000  Initialization mask: 0xffffffff
 * Access mode: C-RW (control - read/write)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_7 0x40090100
#define SIZE_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_7 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_7 0xffffffff

/*
 *
 * Field: ftdf_on_off_regmap_macWUPeriod
 *
 * In CSL mode, the Wake-up duration in symbol periods. During this period, Wake-up frames will be transmitted.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_MACWUPERIOD IND_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_7
#define MSK_F_FTDF_ON_OFF_REGMAP_MACWUPERIOD 0xffff
#define OFF_F_FTDF_ON_OFF_REGMAP_MACWUPERIOD 0
#define SIZE_F_FTDF_ON_OFF_REGMAP_MACWUPERIOD 16
/*
 *
 * Field: ftdf_on_off_regmap_macCSLsamplePeriod
 *
 * In CSL mode, when performing a idle listening, the receiver is enabled for at least macCSLsamplePeriod (in symbol oeriods).
 */
#define IND_F_FTDF_ON_OFF_REGMAP_MACCSLSAMPLEPERIOD IND_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_7
#define MSK_F_FTDF_ON_OFF_REGMAP_MACCSLSAMPLEPERIOD 0xffff0000
#define OFF_F_FTDF_ON_OFF_REGMAP_MACCSLSAMPLEPERIOD 16
#define SIZE_F_FTDF_ON_OFF_REGMAP_MACCSLSAMPLEPERIOD 16

/*
 *
 * REGISTER: ftdf_on_off_regmap_lmac_control_8
 *
 * Initialization value: 0x0  Initialization mask: 0xffffffff
 * Access mode: C-RW (control - read/write)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_8 0x40090104
#define SIZE_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_8 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_8 0xffffffff

/*
 *
 * Field: ftdf_on_off_regmap_macCSLstartSampleTime
 *
 * In CSL mode, the control register macCSLstartSampleTime indicates the TimeStamp generator time (in symbol periods) when to start listening (called "idle listening") or transmitting (when tx_flag_status is set).
 */
#define IND_F_FTDF_ON_OFF_REGMAP_MACCSLSTARTSAMPLETIME IND_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_8
#define MSK_F_FTDF_ON_OFF_REGMAP_MACCSLSTARTSAMPLETIME 0xffffffff
#define OFF_F_FTDF_ON_OFF_REGMAP_MACCSLSTARTSAMPLETIME 0
#define SIZE_F_FTDF_ON_OFF_REGMAP_MACCSLSTARTSAMPLETIME 32

/*
 *
 * REGISTER: ftdf_on_off_regmap_lmac_control_9
 *
 * Initialization value: 0x42  Initialization mask: 0xffffffff
 * Access mode: C-RW (control - read/write)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_9 0x40090108
#define SIZE_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_9 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_9 0xffffffff

/*
 *
 * Field: ftdf_on_off_regmap_macCSLdataPeriod
 *
 * In CSL mode, after the wake-up sequence a data frame is expected. The receiver will be enabled for at least a period of macCSLdataPeriod (in symbol periods).
 */
#define IND_F_FTDF_ON_OFF_REGMAP_MACCSLDATAPERIOD IND_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_9
#define MSK_F_FTDF_ON_OFF_REGMAP_MACCSLDATAPERIOD 0xffff
#define OFF_F_FTDF_ON_OFF_REGMAP_MACCSLDATAPERIOD 0
#define SIZE_F_FTDF_ON_OFF_REGMAP_MACCSLDATAPERIOD 16
/*
 *
 * Field: ftdf_on_off_regmap_macCSLFramePendingWaitT
 *
 * In CSL mode, if a non Wake-up frame with Frame Pending bit = '1' is received, the receiver is enabled for at least an extra period of macCSLFramePendingWaitT (in symbol periods) after the end of the received frame.
 * The time the Enhanced ACK transmission lasts (if applicable) is included in this time.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_MACCSLFRAMEPENDINGWAITT IND_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_9
#define MSK_F_FTDF_ON_OFF_REGMAP_MACCSLFRAMEPENDINGWAITT 0xffff0000
#define OFF_F_FTDF_ON_OFF_REGMAP_MACCSLFRAMEPENDINGWAITT 16
#define SIZE_F_FTDF_ON_OFF_REGMAP_MACCSLFRAMEPENDINGWAITT 16

/*
 *
 * REGISTER: ftdf_on_off_regmap_lmac_control_10
 *
 * Initialization value: 0x20000000  Initialization mask: 0xf00f00ff
 * Access mode: C-RW (control - read/write)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_10 0x4009010c
#define SIZE_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_10 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_10 0xf00f00ff

/*
 *
 * Field: ftdf_on_off_regmap_macRZzeroVal
 *
 * In CSL mode, if the current RZtime is less or Equal to macRZzeroVal an RZtime with value zero is inserted in the wakeup frame. So this is by default the last Wake-up frame of a Wake-up sequence.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_MACRZZEROVAL IND_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_10
#define MSK_F_FTDF_ON_OFF_REGMAP_MACRZZEROVAL 0xf0000000
#define OFF_F_FTDF_ON_OFF_REGMAP_MACRZZEROVAL 28
#define SIZE_F_FTDF_ON_OFF_REGMAP_MACRZZEROVAL 4
/*
 *
 * Field: ftdf_on_off_regmap_macCSLmarginRZ
 *
 * In CSL mode, the software can set the margin for the expected frame by control register macCSLmarginRZ (in 10 symbol periods).
 * The LMAC will make sure that the receiver is ready to receive data this amount of time earlier than to be expected by the received RZ time.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_MACCSLMARGINRZ IND_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_10
#define MSK_F_FTDF_ON_OFF_REGMAP_MACCSLMARGINRZ 0xf0000
#define OFF_F_FTDF_ON_OFF_REGMAP_MACCSLMARGINRZ 16
#define SIZE_F_FTDF_ON_OFF_REGMAP_MACCSLMARGINRZ 4
/*
 *
 * Field: ftdf_on_off_regmap_macWURZCorrection
 *
 * In CSL mode, this register shall be used if the Wake-up frame to be transmitted is larger than 15 octets.
 * It shall indicate the amount of extra data in a Wake-up frame after the RZ position in the frame (in 10 symbol periods).
 * This correction is needed to make sure that the correct RZ time is filled in by the LMAC.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_MACWURZCORRECTION IND_R_FTDF_ON_OFF_REGMAP_LMAC_CONTROL_10
#define MSK_F_FTDF_ON_OFF_REGMAP_MACWURZCORRECTION 0xff
#define OFF_F_FTDF_ON_OFF_REGMAP_MACWURZCORRECTION 0
#define SIZE_F_FTDF_ON_OFF_REGMAP_MACWURZCORRECTION 8

/*
 *
 * REGISTER: ftdf_on_off_regmap_security_0
 *
 * Initialization value: 0x0  Initialization mask: 0xff7f0f02
 * Access mode: C-RW (control - read/write)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_SECURITY_0 0x40090110
#define SIZE_R_FTDF_ON_OFF_REGMAP_SECURITY_0 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_SECURITY_0 0xff7f0f02

/*
 *
 * Bit: ftdf_on_off_regmap_secTxRxn
 *
 * Encryption/decryption mode: see register "secEntry".
 */
#define IND_F_FTDF_ON_OFF_REGMAP_SECTXRXN IND_R_FTDF_ON_OFF_REGMAP_SECURITY_0
#define MSK_F_FTDF_ON_OFF_REGMAP_SECTXRXN 0x2
#define OFF_F_FTDF_ON_OFF_REGMAP_SECTXRXN 1
#define SIZE_F_FTDF_ON_OFF_REGMAP_SECTXRXN 1
/*
 *
 * Field: ftdf_on_off_regmap_secEntry
 *
 * Encryption/decryption mode: the software indicates by the control registers secEntry and secTxRxn which entry to use and if it's from the Tx or Rx buffer ('1' resp. '0').
 */
#define IND_F_FTDF_ON_OFF_REGMAP_SECENTRY IND_R_FTDF_ON_OFF_REGMAP_SECURITY_0
#define MSK_F_FTDF_ON_OFF_REGMAP_SECENTRY 0xf00
#define OFF_F_FTDF_ON_OFF_REGMAP_SECENTRY 8
#define SIZE_F_FTDF_ON_OFF_REGMAP_SECENTRY 4
/*
 *
 * Field: ftdf_on_off_regmap_secAlength
 *
 * Encryption/decryption mode: the length of the a_data is indicated by control register secAlength (in bytes).
 * The end of the a_data is the start point of the m_data. So secAlength must also be set if security level==4.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_SECALENGTH IND_R_FTDF_ON_OFF_REGMAP_SECURITY_0
#define MSK_F_FTDF_ON_OFF_REGMAP_SECALENGTH 0x7f0000
#define OFF_F_FTDF_ON_OFF_REGMAP_SECALENGTH 16
#define SIZE_F_FTDF_ON_OFF_REGMAP_SECALENGTH 7
/*
 *
 * Field: ftdf_on_off_regmap_secMlength
 *
 * Encryption/decryption mode: the length of the m_data is indicated by control register secMlength (in bytes).
 */
#define IND_F_FTDF_ON_OFF_REGMAP_SECMLENGTH IND_R_FTDF_ON_OFF_REGMAP_SECURITY_0
#define MSK_F_FTDF_ON_OFF_REGMAP_SECMLENGTH 0x7f000000
#define OFF_F_FTDF_ON_OFF_REGMAP_SECMLENGTH 24
#define SIZE_F_FTDF_ON_OFF_REGMAP_SECMLENGTH 7
/*
 *
 * Bit: ftdf_on_off_regmap_secEncDecn
 *
 * Encryption/decryption mode: the control register secEncDecn indicates whether to encrypt ('1') or decrypt ('0') the data.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_SECENCDECN IND_R_FTDF_ON_OFF_REGMAP_SECURITY_0
#define MSK_F_FTDF_ON_OFF_REGMAP_SECENCDECN 0x80000000
#define OFF_F_FTDF_ON_OFF_REGMAP_SECENCDECN 31
#define SIZE_F_FTDF_ON_OFF_REGMAP_SECENCDECN 1

/*
 *
 * REGISTER: ftdf_on_off_regmap_security_1
 *
 * Initialization value: 0x0  Initialization mask: 0xffff
 * Access mode: C-RW (control - read/write)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_SECURITY_1 0x40090114
#define SIZE_R_FTDF_ON_OFF_REGMAP_SECURITY_1 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_SECURITY_1 0xffff

/*
 *
 * Field: ftdf_on_off_regmap_secAuthFlags
 *
 * Encryption/decryption mode: register secAuthFlags contains the authentication flags fields.
 * bit[7] is '0'
 * bit[6] is "A_data present"
 * bit[5:3]: 3-bit security level of m_data
 * bit[2:0]: 3-bit security level of a_data.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_SECAUTHFLAGS IND_R_FTDF_ON_OFF_REGMAP_SECURITY_1
#define MSK_F_FTDF_ON_OFF_REGMAP_SECAUTHFLAGS 0xff
#define OFF_F_FTDF_ON_OFF_REGMAP_SECAUTHFLAGS 0
#define SIZE_F_FTDF_ON_OFF_REGMAP_SECAUTHFLAGS 8
/*
 *
 * Field: ftdf_on_off_regmap_secEncrFlags
 *
 * Encryption/decryption mode: register secEncrFlags contains the encryption flags field.
 * Bits [2:0] are the 3-bit encoding flags of a_data, the other bits msut be set to '0'.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_SECENCRFLAGS IND_R_FTDF_ON_OFF_REGMAP_SECURITY_1
#define MSK_F_FTDF_ON_OFF_REGMAP_SECENCRFLAGS 0xff00
#define OFF_F_FTDF_ON_OFF_REGMAP_SECENCRFLAGS 8
#define SIZE_F_FTDF_ON_OFF_REGMAP_SECENCRFLAGS 8

/*
 *
 * REGISTER: ftdf_on_off_regmap_secKey_0
 *
 * Initialization value: 0x0  Initialization mask: 0xffffffff
 * Access mode: C-RW (control - read/write)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_SECKEY_0 0x40090118
#define SIZE_R_FTDF_ON_OFF_REGMAP_SECKEY_0 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_SECKEY_0 0xffffffff

/*
 *
 * Field: ftdf_on_off_regmap_secKey_0
 *
 * Encryption/decryption mode: Registers secKey[0..3]  contain the key to be used.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_SECKEY_0 IND_R_FTDF_ON_OFF_REGMAP_SECKEY_0
#define MSK_F_FTDF_ON_OFF_REGMAP_SECKEY_0 0xffffffff
#define OFF_F_FTDF_ON_OFF_REGMAP_SECKEY_0 0
#define SIZE_F_FTDF_ON_OFF_REGMAP_SECKEY_0 32

/*
 *
 * REGISTER: ftdf_on_off_regmap_secKey_1
 *
 * Initialization value: 0x0  Initialization mask: 0xffffffff
 * Access mode: C-RW (control - read/write)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_SECKEY_1 0x4009011c
#define SIZE_R_FTDF_ON_OFF_REGMAP_SECKEY_1 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_SECKEY_1 0xffffffff

/*
 *
 * Field: ftdf_on_off_regmap_secKey_1
 *
 * Encryption/decryption mode: see register "secKey_0"
 */
#define IND_F_FTDF_ON_OFF_REGMAP_SECKEY_1 IND_R_FTDF_ON_OFF_REGMAP_SECKEY_1
#define MSK_F_FTDF_ON_OFF_REGMAP_SECKEY_1 0xffffffff
#define OFF_F_FTDF_ON_OFF_REGMAP_SECKEY_1 0
#define SIZE_F_FTDF_ON_OFF_REGMAP_SECKEY_1 32

/*
 *
 * REGISTER: ftdf_on_off_regmap_secKey_2
 *
 * Initialization value: 0x0  Initialization mask: 0xffffffff
 * Access mode: C-RW (control - read/write)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_SECKEY_2 0x40090120
#define SIZE_R_FTDF_ON_OFF_REGMAP_SECKEY_2 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_SECKEY_2 0xffffffff

/*
 *
 * Field: ftdf_on_off_regmap_secKey_2
 *
 * Encryption/decryption mode: see register "secKey_0"
 */
#define IND_F_FTDF_ON_OFF_REGMAP_SECKEY_2 IND_R_FTDF_ON_OFF_REGMAP_SECKEY_2
#define MSK_F_FTDF_ON_OFF_REGMAP_SECKEY_2 0xffffffff
#define OFF_F_FTDF_ON_OFF_REGMAP_SECKEY_2 0
#define SIZE_F_FTDF_ON_OFF_REGMAP_SECKEY_2 32

/*
 *
 * REGISTER: ftdf_on_off_regmap_secKey_3
 *
 * Initialization value: 0x0  Initialization mask: 0xffffffff
 * Access mode: C-RW (control - read/write)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_SECKEY_3 0x40090124
#define SIZE_R_FTDF_ON_OFF_REGMAP_SECKEY_3 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_SECKEY_3 0xffffffff

/*
 *
 * Field: ftdf_on_off_regmap_secKey_3
 *
 * Encryption/decryption mode: see register "secKey_0"
 */
#define IND_F_FTDF_ON_OFF_REGMAP_SECKEY_3 IND_R_FTDF_ON_OFF_REGMAP_SECKEY_3
#define MSK_F_FTDF_ON_OFF_REGMAP_SECKEY_3 0xffffffff
#define OFF_F_FTDF_ON_OFF_REGMAP_SECKEY_3 0
#define SIZE_F_FTDF_ON_OFF_REGMAP_SECKEY_3 32

/*
 *
 * REGISTER: ftdf_on_off_regmap_secNonce_0
 *
 * Initialization value: 0x0  Initialization mask: 0xffffffff
 * Access mode: C-RW (control - read/write)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_SECNONCE_0 0x40090128
#define SIZE_R_FTDF_ON_OFF_REGMAP_SECNONCE_0 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_SECNONCE_0 0xffffffff

/*
 *
 * Field: ftdf_on_off_regmap_secNonce_0
 *
 * Encryption/decryption mode: register secNonce[0..3] contains the Nonce to be used for encryption/decryption.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_SECNONCE_0 IND_R_FTDF_ON_OFF_REGMAP_SECNONCE_0
#define MSK_F_FTDF_ON_OFF_REGMAP_SECNONCE_0 0xffffffff
#define OFF_F_FTDF_ON_OFF_REGMAP_SECNONCE_0 0
#define SIZE_F_FTDF_ON_OFF_REGMAP_SECNONCE_0 32

/*
 *
 * REGISTER: ftdf_on_off_regmap_secNonce_1
 *
 * Initialization value: 0x0  Initialization mask: 0xffffffff
 * Access mode: C-RW (control - read/write)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_SECNONCE_1 0x4009012c
#define SIZE_R_FTDF_ON_OFF_REGMAP_SECNONCE_1 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_SECNONCE_1 0xffffffff

/*
 *
 * Field: ftdf_on_off_regmap_secNonce_1
 *
 * Encryption/decryption mode: see register "Nonce_0"
 */
#define IND_F_FTDF_ON_OFF_REGMAP_SECNONCE_1 IND_R_FTDF_ON_OFF_REGMAP_SECNONCE_1
#define MSK_F_FTDF_ON_OFF_REGMAP_SECNONCE_1 0xffffffff
#define OFF_F_FTDF_ON_OFF_REGMAP_SECNONCE_1 0
#define SIZE_F_FTDF_ON_OFF_REGMAP_SECNONCE_1 32

/*
 *
 * REGISTER: ftdf_on_off_regmap_secNonce_2
 *
 * Initialization value: 0x0  Initialization mask: 0xffffffff
 * Access mode: C-RW (control - read/write)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_SECNONCE_2 0x40090130
#define SIZE_R_FTDF_ON_OFF_REGMAP_SECNONCE_2 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_SECNONCE_2 0xffffffff

/*
 *
 * Field: ftdf_on_off_regmap_secNonce_2
 *
 * Encryption/decryption mode: see register "Nonce_0"
 */
#define IND_F_FTDF_ON_OFF_REGMAP_SECNONCE_2 IND_R_FTDF_ON_OFF_REGMAP_SECNONCE_2
#define MSK_F_FTDF_ON_OFF_REGMAP_SECNONCE_2 0xffffffff
#define OFF_F_FTDF_ON_OFF_REGMAP_SECNONCE_2 0
#define SIZE_F_FTDF_ON_OFF_REGMAP_SECNONCE_2 32

/*
 *
 * REGISTER: ftdf_on_off_regmap_secNonce_3
 *
 * Initialization value: 0x0  Initialization mask: 0xff
 * Access mode: C-RW (control - read/write)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_SECNONCE_3 0x40090134
#define SIZE_R_FTDF_ON_OFF_REGMAP_SECNONCE_3 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_SECNONCE_3 0xff

/*
 *
 * Field: ftdf_on_off_regmap_secNonce_3
 *
 * Encryption/decryption mode: see register "Nonce_0"
 */
#define IND_F_FTDF_ON_OFF_REGMAP_SECNONCE_3 IND_R_FTDF_ON_OFF_REGMAP_SECNONCE_3
#define MSK_F_FTDF_ON_OFF_REGMAP_SECNONCE_3 0xff
#define OFF_F_FTDF_ON_OFF_REGMAP_SECNONCE_3 0
#define SIZE_F_FTDF_ON_OFF_REGMAP_SECNONCE_3 8

/*
 *
 * REGISTER: ftdf_on_off_regmap_security_os
 *
 * Initialization value: 0x0  Initialization mask: 0x3
 * Access mode: OS-W (one-shot - write)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_SECURITY_OS 0x40090138
#define SIZE_R_FTDF_ON_OFF_REGMAP_SECURITY_OS 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_SECURITY_OS 0x3

/*
 *
 * Bit: ftdf_on_off_regmap_secAbort
 *
 * Encryption/decryption mode: see register "Nonce_0"
 */
#define IND_F_FTDF_ON_OFF_REGMAP_SECABORT IND_R_FTDF_ON_OFF_REGMAP_SECURITY_OS
#define MSK_F_FTDF_ON_OFF_REGMAP_SECABORT 0x1
#define OFF_F_FTDF_ON_OFF_REGMAP_SECABORT 0
#define SIZE_F_FTDF_ON_OFF_REGMAP_SECABORT 1
/*
 *
 * Bit: ftdf_on_off_regmap_secStart
 *
 * Encryption/decryption mode: one_shot register to start the encryption, decryption and authentication support task.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_SECSTART IND_R_FTDF_ON_OFF_REGMAP_SECURITY_OS
#define MSK_F_FTDF_ON_OFF_REGMAP_SECSTART 0x2
#define OFF_F_FTDF_ON_OFF_REGMAP_SECSTART 1
#define SIZE_F_FTDF_ON_OFF_REGMAP_SECSTART 1

/*
 *
 * REGISTER: ftdf_on_off_regmap_security_status
 *
 * Initialization value: none.
 * Access mode: S-R (state - read)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_SECURITY_STATUS 0x40090140
#define SIZE_R_FTDF_ON_OFF_REGMAP_SECURITY_STATUS 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_SECURITY_STATUS 0x3

/*
 *
 * Bit: ftdf_on_off_regmap_secBusy
 *
 * Encryption/decryption mode: register "secBusy" indicates if the encryption/decryption process is still running.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_SECBUSY IND_R_FTDF_ON_OFF_REGMAP_SECURITY_STATUS
#define MSK_F_FTDF_ON_OFF_REGMAP_SECBUSY 0x1
#define OFF_F_FTDF_ON_OFF_REGMAP_SECBUSY 0
#define SIZE_F_FTDF_ON_OFF_REGMAP_SECBUSY 1
/*
 *
 * Bit: ftdf_on_off_regmap_secAuthFail
 *
 * Encryption/decryption mode: in case of decryption, the status bit secAuthFail will be set when the authentication has failed.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_SECAUTHFAIL IND_R_FTDF_ON_OFF_REGMAP_SECURITY_STATUS
#define MSK_F_FTDF_ON_OFF_REGMAP_SECAUTHFAIL 0x2
#define OFF_F_FTDF_ON_OFF_REGMAP_SECAUTHFAIL 1
#define SIZE_F_FTDF_ON_OFF_REGMAP_SECAUTHFAIL 1

/*
 *
 * REGISTER: ftdf_on_off_regmap_security_event
 *
 * Initialization value: 0x0  Initialization mask: 0x1
 * Access mode: E-RCW (event read/clear on write 1)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_SECURITY_EVENT 0x40090150
#define SIZE_R_FTDF_ON_OFF_REGMAP_SECURITY_EVENT 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_SECURITY_EVENT 0x1

/*
 *
 * Bit: ftdf_on_off_regmap_secReady_e
 *
 * Encryption/decryption mode: the Event bit secReady_e is set to '1' when the authentication process is ready (i.e. secBusy is cleared).
 * This event bit contributes to ftdf_ce[3].
 */
#define IND_F_FTDF_ON_OFF_REGMAP_SECREADY_E IND_R_FTDF_ON_OFF_REGMAP_SECURITY_EVENT
#define MSK_F_FTDF_ON_OFF_REGMAP_SECREADY_E 0x1
#define OFF_F_FTDF_ON_OFF_REGMAP_SECREADY_E 0
#define SIZE_F_FTDF_ON_OFF_REGMAP_SECREADY_E 1

/*
 *
 * REGISTER: ftdf_on_off_regmap_security_eventmask
 *
 * Initialization value: 0x0  Initialization mask: 0x1
 * Access mode: EM-RW (event mask)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_SECURITY_EVENTMASK 0x40090154
#define SIZE_R_FTDF_ON_OFF_REGMAP_SECURITY_EVENTMASK 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_SECURITY_EVENTMASK 0x1

/*
 *
 * Bit: ftdf_on_off_regmap_secReady_m
 *
 * Mask bit for event "secReady_e".
 * The mask bit is masking when cleared to '0' (default value) and will enable the contribution to the interrupt when set to '1'.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_SECREADY_M IND_R_FTDF_ON_OFF_REGMAP_SECURITY_EVENTMASK
#define MSK_F_FTDF_ON_OFF_REGMAP_SECREADY_M 0x1
#define OFF_F_FTDF_ON_OFF_REGMAP_SECREADY_M 0
#define SIZE_F_FTDF_ON_OFF_REGMAP_SECREADY_M 1

/*
 *
 * REGISTER: ftdf_on_off_regmap_tsch_control_0
 *
 * Initialization value: 0x89803e8  Initialization mask: 0xffffffff
 * Access mode: C-RW (control - read/write)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_TSCH_CONTROL_0 0x40090160
#define SIZE_R_FTDF_ON_OFF_REGMAP_TSCH_CONTROL_0 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_TSCH_CONTROL_0 0xffffffff

/*
 *
 * Field: ftdf_on_off_regmap_macTSTxAckDelay
 *
 * TSCH mode: the time between the end of a Rx frame and the start of an Enhanced Acknowlegde frame.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_MACTSTXACKDELAY IND_R_FTDF_ON_OFF_REGMAP_TSCH_CONTROL_0
#define MSK_F_FTDF_ON_OFF_REGMAP_MACTSTXACKDELAY 0xffff
#define OFF_F_FTDF_ON_OFF_REGMAP_MACTSTXACKDELAY 0
#define SIZE_F_FTDF_ON_OFF_REGMAP_MACTSTXACKDELAY 16
/*
 *
 * Field: ftdf_on_off_regmap_macTSRxWait
 *
 * TSCH mode: The times to wait for start of frame
 */
#define IND_F_FTDF_ON_OFF_REGMAP_MACTSRXWAIT IND_R_FTDF_ON_OFF_REGMAP_TSCH_CONTROL_0
#define MSK_F_FTDF_ON_OFF_REGMAP_MACTSRXWAIT 0xffff0000
#define OFF_F_FTDF_ON_OFF_REGMAP_MACTSRXWAIT 16
#define SIZE_F_FTDF_ON_OFF_REGMAP_MACTSRXWAIT 16

/*
 *
 * REGISTER: ftdf_on_off_regmap_tsch_control_1
 *
 * Initialization value: 0xc0  Initialization mask: 0xffff
 * Access mode: C-RW (control - read/write)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_TSCH_CONTROL_1 0x40090164
#define SIZE_R_FTDF_ON_OFF_REGMAP_TSCH_CONTROL_1 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_TSCH_CONTROL_1 0xffff

/*
 *
 * Field: ftdf_on_off_regmap_macTSRxTx
 *
 * TSCH mode: The time between the CCA and the TX of a frame
 */
#define IND_F_FTDF_ON_OFF_REGMAP_MACTSRXTX IND_R_FTDF_ON_OFF_REGMAP_TSCH_CONTROL_1
#define MSK_F_FTDF_ON_OFF_REGMAP_MACTSRXTX 0xffff
#define OFF_F_FTDF_ON_OFF_REGMAP_MACTSRXTX 0
#define SIZE_F_FTDF_ON_OFF_REGMAP_MACTSRXTX 16

/*
 *
 * REGISTER: ftdf_on_off_regmap_tsch_control_2
 *
 * Initialization value: 0x1900320  Initialization mask: 0xffffffff
 * Access mode: C-RW (control - read/write)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_TSCH_CONTROL_2 0x40090168
#define SIZE_R_FTDF_ON_OFF_REGMAP_TSCH_CONTROL_2 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_TSCH_CONTROL_2 0xffffffff

/*
 *
 * Field: ftdf_on_off_regmap_macTSRxAckDelay
 *
 * TSCH mode: End of frame to when the transmitter shall listen for Acknowledgement
 */
#define IND_F_FTDF_ON_OFF_REGMAP_MACTSRXACKDELAY IND_R_FTDF_ON_OFF_REGMAP_TSCH_CONTROL_2
#define MSK_F_FTDF_ON_OFF_REGMAP_MACTSRXACKDELAY 0xffff
#define OFF_F_FTDF_ON_OFF_REGMAP_MACTSRXACKDELAY 0
#define SIZE_F_FTDF_ON_OFF_REGMAP_MACTSRXACKDELAY 16
/*
 *
 * Field: ftdf_on_off_regmap_macTSAckWait
 *
 * TSCH mode: The minimum time to wait for start of an Acknowledgement
 */
#define IND_F_FTDF_ON_OFF_REGMAP_MACTSACKWAIT IND_R_FTDF_ON_OFF_REGMAP_TSCH_CONTROL_2
#define MSK_F_FTDF_ON_OFF_REGMAP_MACTSACKWAIT 0xffff0000
#define OFF_F_FTDF_ON_OFF_REGMAP_MACTSACKWAIT 16
#define SIZE_F_FTDF_ON_OFF_REGMAP_MACTSACKWAIT 16

/*
 *
 * REGISTER: ftdf_on_off_regmap_phy_parameters_0
 *
 * Initialization value: 0x76543210  Initialization mask: 0x77777777
 * Access mode: C-RW (control - read/write)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_PHY_PARAMETERS_0 0x40090180
#define SIZE_R_FTDF_ON_OFF_REGMAP_PHY_PARAMETERS_0 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_PHY_PARAMETERS_0 0x77777777

/*
 *
 * Field: ftdf_on_off_regmap_rxBitPos_0
 *
 * DPHY interface: control rxBitPos(8)(3) controls the position that a bit should have at the DPHY interface.
 * So the default values are rxBitPos_0 = 0, rxBitPos_1 = 1, rxBitPos_2 = 2, etc.
 *
 * Note1 that this is a conversion from rx DPHY interface to the internal data byte
 * So
 *  for(n=7;n>=0;n--)
 *    rx_data(n) = dphy_bit(tx_BitPos(n))
 *  endfor
 *
 * Note2 that rxBitPos and txBitPos must have inverse functions.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_RXBITPOS_0 IND_R_FTDF_ON_OFF_REGMAP_PHY_PARAMETERS_0
#define MSK_F_FTDF_ON_OFF_REGMAP_RXBITPOS_0 0x7
#define OFF_F_FTDF_ON_OFF_REGMAP_RXBITPOS_0 0
#define SIZE_F_FTDF_ON_OFF_REGMAP_RXBITPOS_0 3
/*
 *
 * Field: ftdf_on_off_regmap_rxBitPos_1
 *
 * DPHY interface: see rxBitPos_0
 */
#define IND_F_FTDF_ON_OFF_REGMAP_RXBITPOS_1 IND_R_FTDF_ON_OFF_REGMAP_PHY_PARAMETERS_0
#define MSK_F_FTDF_ON_OFF_REGMAP_RXBITPOS_1 0x70
#define OFF_F_FTDF_ON_OFF_REGMAP_RXBITPOS_1 4
#define SIZE_F_FTDF_ON_OFF_REGMAP_RXBITPOS_1 3
/*
 *
 * Field: ftdf_on_off_regmap_rxBitPos_2
 *
 * DPHY interface: see rxBitPos_0
 */
#define IND_F_FTDF_ON_OFF_REGMAP_RXBITPOS_2 IND_R_FTDF_ON_OFF_REGMAP_PHY_PARAMETERS_0
#define MSK_F_FTDF_ON_OFF_REGMAP_RXBITPOS_2 0x700
#define OFF_F_FTDF_ON_OFF_REGMAP_RXBITPOS_2 8
#define SIZE_F_FTDF_ON_OFF_REGMAP_RXBITPOS_2 3
/*
 *
 * Field: ftdf_on_off_regmap_rxBitPos_3
 *
 * DPHY interface: see rxBitPos_0
 */
#define IND_F_FTDF_ON_OFF_REGMAP_RXBITPOS_3 IND_R_FTDF_ON_OFF_REGMAP_PHY_PARAMETERS_0
#define MSK_F_FTDF_ON_OFF_REGMAP_RXBITPOS_3 0x7000
#define OFF_F_FTDF_ON_OFF_REGMAP_RXBITPOS_3 12
#define SIZE_F_FTDF_ON_OFF_REGMAP_RXBITPOS_3 3
/*
 *
 * Field: ftdf_on_off_regmap_rxBitPos_4
 *
 * DPHY interface: see rxBitPos_0
 */
#define IND_F_FTDF_ON_OFF_REGMAP_RXBITPOS_4 IND_R_FTDF_ON_OFF_REGMAP_PHY_PARAMETERS_0
#define MSK_F_FTDF_ON_OFF_REGMAP_RXBITPOS_4 0x70000
#define OFF_F_FTDF_ON_OFF_REGMAP_RXBITPOS_4 16
#define SIZE_F_FTDF_ON_OFF_REGMAP_RXBITPOS_4 3
/*
 *
 * Field: ftdf_on_off_regmap_rxBitPos_5
 *
 * DPHY interface: see rxBitPos_0
 */
#define IND_F_FTDF_ON_OFF_REGMAP_RXBITPOS_5 IND_R_FTDF_ON_OFF_REGMAP_PHY_PARAMETERS_0
#define MSK_F_FTDF_ON_OFF_REGMAP_RXBITPOS_5 0x700000
#define OFF_F_FTDF_ON_OFF_REGMAP_RXBITPOS_5 20
#define SIZE_F_FTDF_ON_OFF_REGMAP_RXBITPOS_5 3
/*
 *
 * Field: ftdf_on_off_regmap_rxBitPos_6
 *
 * DPHY interface: see rxBitPos_0
 */
#define IND_F_FTDF_ON_OFF_REGMAP_RXBITPOS_6 IND_R_FTDF_ON_OFF_REGMAP_PHY_PARAMETERS_0
#define MSK_F_FTDF_ON_OFF_REGMAP_RXBITPOS_6 0x7000000
#define OFF_F_FTDF_ON_OFF_REGMAP_RXBITPOS_6 24
#define SIZE_F_FTDF_ON_OFF_REGMAP_RXBITPOS_6 3
/*
 *
 * Field: ftdf_on_off_regmap_rxBitPos_7
 *
 * DPHY interface: see rxBitPos_0
 */
#define IND_F_FTDF_ON_OFF_REGMAP_RXBITPOS_7 IND_R_FTDF_ON_OFF_REGMAP_PHY_PARAMETERS_0
#define MSK_F_FTDF_ON_OFF_REGMAP_RXBITPOS_7 0x70000000
#define OFF_F_FTDF_ON_OFF_REGMAP_RXBITPOS_7 28
#define SIZE_F_FTDF_ON_OFF_REGMAP_RXBITPOS_7 3

/*
 *
 * REGISTER: ftdf_on_off_regmap_phy_parameters_1
 *
 * Initialization value: 0x76543210  Initialization mask: 0x77777777
 * Access mode: C-RW (control - read/write)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_PHY_PARAMETERS_1 0x40090184
#define SIZE_R_FTDF_ON_OFF_REGMAP_PHY_PARAMETERS_1 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_PHY_PARAMETERS_1 0x77777777

/*
 *
 * Field: ftdf_on_off_regmap_txBitPos_0
 *
 * DPHY interface: control txBitPos(8)(3) controls the position that a bit should have at the DPHY interface.
 * So the default values are txBitPos_0 = 0, txBitPos_1 = 1, txBitPos_2 = 2, etc.
 *
 * Note1 that this is a conversion from internal data byte to the DPHY interface.
 * So
 *  for(n=7;n>=0;n--)
 *    tx_dphy_bit(n) = tx_data(tx_BitPos(n))
 *  endfor
 *
 * Note2 that txBitPos and rxBitPos must have inverse functions.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_TXBITPOS_0 IND_R_FTDF_ON_OFF_REGMAP_PHY_PARAMETERS_1
#define MSK_F_FTDF_ON_OFF_REGMAP_TXBITPOS_0 0x7
#define OFF_F_FTDF_ON_OFF_REGMAP_TXBITPOS_0 0
#define SIZE_F_FTDF_ON_OFF_REGMAP_TXBITPOS_0 3
/*
 *
 * Field: ftdf_on_off_regmap_txBitPos_1
 *
 * DPHY interface: see txBitPos_0
 */
#define IND_F_FTDF_ON_OFF_REGMAP_TXBITPOS_1 IND_R_FTDF_ON_OFF_REGMAP_PHY_PARAMETERS_1
#define MSK_F_FTDF_ON_OFF_REGMAP_TXBITPOS_1 0x70
#define OFF_F_FTDF_ON_OFF_REGMAP_TXBITPOS_1 4
#define SIZE_F_FTDF_ON_OFF_REGMAP_TXBITPOS_1 3
/*
 *
 * Field: ftdf_on_off_regmap_txBitPos_2
 *
 * DPHY interface: see txBitPos_0
 */
#define IND_F_FTDF_ON_OFF_REGMAP_TXBITPOS_2 IND_R_FTDF_ON_OFF_REGMAP_PHY_PARAMETERS_1
#define MSK_F_FTDF_ON_OFF_REGMAP_TXBITPOS_2 0x700
#define OFF_F_FTDF_ON_OFF_REGMAP_TXBITPOS_2 8
#define SIZE_F_FTDF_ON_OFF_REGMAP_TXBITPOS_2 3
/*
 *
 * Field: ftdf_on_off_regmap_txBitPos_3
 *
 * DPHY interface: see txBitPos_0
 */
#define IND_F_FTDF_ON_OFF_REGMAP_TXBITPOS_3 IND_R_FTDF_ON_OFF_REGMAP_PHY_PARAMETERS_1
#define MSK_F_FTDF_ON_OFF_REGMAP_TXBITPOS_3 0x7000
#define OFF_F_FTDF_ON_OFF_REGMAP_TXBITPOS_3 12
#define SIZE_F_FTDF_ON_OFF_REGMAP_TXBITPOS_3 3
/*
 *
 * Field: ftdf_on_off_regmap_txBitPos_4
 *
 * DPHY interface: see txBitPos_0
 */
#define IND_F_FTDF_ON_OFF_REGMAP_TXBITPOS_4 IND_R_FTDF_ON_OFF_REGMAP_PHY_PARAMETERS_1
#define MSK_F_FTDF_ON_OFF_REGMAP_TXBITPOS_4 0x70000
#define OFF_F_FTDF_ON_OFF_REGMAP_TXBITPOS_4 16
#define SIZE_F_FTDF_ON_OFF_REGMAP_TXBITPOS_4 3
/*
 *
 * Field: ftdf_on_off_regmap_txBitPos_5
 *
 * DPHY interface: see txBitPos_0
 */
#define IND_F_FTDF_ON_OFF_REGMAP_TXBITPOS_5 IND_R_FTDF_ON_OFF_REGMAP_PHY_PARAMETERS_1
#define MSK_F_FTDF_ON_OFF_REGMAP_TXBITPOS_5 0x700000
#define OFF_F_FTDF_ON_OFF_REGMAP_TXBITPOS_5 20
#define SIZE_F_FTDF_ON_OFF_REGMAP_TXBITPOS_5 3
/*
 *
 * Field: ftdf_on_off_regmap_txBitPos_6
 *
 * DPHY interface: see txBitPos_0
 */
#define IND_F_FTDF_ON_OFF_REGMAP_TXBITPOS_6 IND_R_FTDF_ON_OFF_REGMAP_PHY_PARAMETERS_1
#define MSK_F_FTDF_ON_OFF_REGMAP_TXBITPOS_6 0x7000000
#define OFF_F_FTDF_ON_OFF_REGMAP_TXBITPOS_6 24
#define SIZE_F_FTDF_ON_OFF_REGMAP_TXBITPOS_6 3
/*
 *
 * Field: ftdf_on_off_regmap_txBitPos_7
 *
 * DPHY interface: see txBitPos_0
 */
#define IND_F_FTDF_ON_OFF_REGMAP_TXBITPOS_7 IND_R_FTDF_ON_OFF_REGMAP_PHY_PARAMETERS_1
#define MSK_F_FTDF_ON_OFF_REGMAP_TXBITPOS_7 0x70000000
#define OFF_F_FTDF_ON_OFF_REGMAP_TXBITPOS_7 28
#define SIZE_F_FTDF_ON_OFF_REGMAP_TXBITPOS_7 3

/*
 *
 * REGISTER: ftdf_on_off_regmap_phy_parameters_2
 *
 * Initialization value: 0x0  Initialization mask: 0xffffffff
 * Access mode: C-RW (control - read/write)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_PHY_PARAMETERS_2 0x40090188
#define SIZE_R_FTDF_ON_OFF_REGMAP_PHY_PARAMETERS_2 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_PHY_PARAMETERS_2 0xffffffff

/*
 *
 * Field: ftdf_on_off_regmap_phyTxStartup
 *
 * DPHY interface: the phy wait time (in us) before transmission of a frame may start.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_PHYTXSTARTUP IND_R_FTDF_ON_OFF_REGMAP_PHY_PARAMETERS_2
#define MSK_F_FTDF_ON_OFF_REGMAP_PHYTXSTARTUP 0xff
#define OFF_F_FTDF_ON_OFF_REGMAP_PHYTXSTARTUP 0
#define SIZE_F_FTDF_ON_OFF_REGMAP_PHYTXSTARTUP 8
/*
 *
 * Field: ftdf_on_off_regmap_phyTxLatency
 *
 * DPHY interface: phy delay (in us) between DPHY interface and air interface.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_PHYTXLATENCY IND_R_FTDF_ON_OFF_REGMAP_PHY_PARAMETERS_2
#define MSK_F_FTDF_ON_OFF_REGMAP_PHYTXLATENCY 0xff00
#define OFF_F_FTDF_ON_OFF_REGMAP_PHYTXLATENCY 8
#define SIZE_F_FTDF_ON_OFF_REGMAP_PHYTXLATENCY 8
/*
 *
 * Field: ftdf_on_off_regmap_phyTxFinish
 *
 * DPHY interface: Phy wait time (in us) before deasserting TX_EN after deasserting TX_VALID.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_PHYTXFINISH IND_R_FTDF_ON_OFF_REGMAP_PHY_PARAMETERS_2
#define MSK_F_FTDF_ON_OFF_REGMAP_PHYTXFINISH 0xff0000
#define OFF_F_FTDF_ON_OFF_REGMAP_PHYTXFINISH 16
#define SIZE_F_FTDF_ON_OFF_REGMAP_PHYTXFINISH 8
/*
 *
 * Field: ftdf_on_off_regmap_phyTRxWait
 *
 * DPHY interface: Phy wait time (in us) between deassertion of TX_EN and assertion of RX_EN or vice versa.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_PHYTRXWAIT IND_R_FTDF_ON_OFF_REGMAP_PHY_PARAMETERS_2
#define MSK_F_FTDF_ON_OFF_REGMAP_PHYTRXWAIT 0xff000000
#define OFF_F_FTDF_ON_OFF_REGMAP_PHYTRXWAIT 24
#define SIZE_F_FTDF_ON_OFF_REGMAP_PHYTRXWAIT 8

/*
 *
 * REGISTER: ftdf_on_off_regmap_phy_parameters_3
 *
 * Initialization value: 0x1000000  Initialization mask: 0x1ffffff
 * Access mode: C-RW (control - read/write)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_PHY_PARAMETERS_3 0x4009018c
#define SIZE_R_FTDF_ON_OFF_REGMAP_PHY_PARAMETERS_3 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_PHY_PARAMETERS_3 0x1ffffff

/*
 *
 * Field: ftdf_on_off_regmap_phyRxStartup
 *
 * DPHY interface: Phy wait time (in us) before receiving of a frame may start.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_PHYRXSTARTUP IND_R_FTDF_ON_OFF_REGMAP_PHY_PARAMETERS_3
#define MSK_F_FTDF_ON_OFF_REGMAP_PHYRXSTARTUP 0xff
#define OFF_F_FTDF_ON_OFF_REGMAP_PHYRXSTARTUP 0
#define SIZE_F_FTDF_ON_OFF_REGMAP_PHYRXSTARTUP 8
/*
 *
 * Field: ftdf_on_off_regmap_phyRxLatency
 *
 * DPHY interface: Phy delay (in us) between air and DPHY interface.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_PHYRXLATENCY IND_R_FTDF_ON_OFF_REGMAP_PHY_PARAMETERS_3
#define MSK_F_FTDF_ON_OFF_REGMAP_PHYRXLATENCY 0xff00
#define OFF_F_FTDF_ON_OFF_REGMAP_PHYRXLATENCY 8
#define SIZE_F_FTDF_ON_OFF_REGMAP_PHYRXLATENCY 8
/*
 *
 * Field: ftdf_on_off_regmap_phyEnable
 *
 * DPHY interface: Asserting the DPHY interface signals TX_EN or RX_EN does not take place within the time phyEnable after asserting the signal PHY_EN.
 * (in us).
 */
#define IND_F_FTDF_ON_OFF_REGMAP_PHYENABLE IND_R_FTDF_ON_OFF_REGMAP_PHY_PARAMETERS_3
#define MSK_F_FTDF_ON_OFF_REGMAP_PHYENABLE 0xff0000
#define OFF_F_FTDF_ON_OFF_REGMAP_PHYENABLE 16
#define SIZE_F_FTDF_ON_OFF_REGMAP_PHYENABLE 8
/*
 *
 * Bit: ftdf_on_off_regmap_use_legacy_phy_en
 *
 * If the control register use_legacy_phy_en is set (default), the output signal PHY_TRANSACTION will be sourced by PHY_EN rather than PHY_TRANSACTION.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_USE_LEGACY_PHY_EN IND_R_FTDF_ON_OFF_REGMAP_PHY_PARAMETERS_3
#define MSK_F_FTDF_ON_OFF_REGMAP_USE_LEGACY_PHY_EN 0x1000000
#define OFF_F_FTDF_ON_OFF_REGMAP_USE_LEGACY_PHY_EN 24
#define SIZE_F_FTDF_ON_OFF_REGMAP_USE_LEGACY_PHY_EN 1

/*
 *
 * REGISTER: ftdf_on_off_regmap_rx_control_0
 *
 * Initialization value: 0x0  Initialization mask: 0xfffffff
 * Access mode: C-RW (control - read/write)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_RX_CONTROL_0 0x40090200
#define SIZE_R_FTDF_ON_OFF_REGMAP_RX_CONTROL_0 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_RX_CONTROL_0 0xfffffff

/*
 *
 * Bit: ftdf_on_off_regmap_DbgRxTransparentMode
 *
 * If set yo '1', Rx pipe is fully set in transparent mode (for debug purpose).
 */
#define IND_F_FTDF_ON_OFF_REGMAP_DBGRXTRANSPARENTMODE IND_R_FTDF_ON_OFF_REGMAP_RX_CONTROL_0
#define MSK_F_FTDF_ON_OFF_REGMAP_DBGRXTRANSPARENTMODE 0x1
#define OFF_F_FTDF_ON_OFF_REGMAP_DBGRXTRANSPARENTMODE 0
#define SIZE_F_FTDF_ON_OFF_REGMAP_DBGRXTRANSPARENTMODE 1
/*
 *
 * Bit: ftdf_on_off_regmap_RxBeaconOnly
 *
 * If set to '1', only Beacons frames are accepted
 */
#define IND_F_FTDF_ON_OFF_REGMAP_RXBEACONONLY IND_R_FTDF_ON_OFF_REGMAP_RX_CONTROL_0
#define MSK_F_FTDF_ON_OFF_REGMAP_RXBEACONONLY 0x2
#define OFF_F_FTDF_ON_OFF_REGMAP_RXBEACONONLY 1
#define SIZE_F_FTDF_ON_OFF_REGMAP_RXBEACONONLY 1
/*
 *
 * Bit: ftdf_on_off_regmap_RxCoordRealignOnly
 *
 * If set to '1', only Coordinator Realignment frames are accepted
 */
#define IND_F_FTDF_ON_OFF_REGMAP_RXCOORDREALIGNONLY IND_R_FTDF_ON_OFF_REGMAP_RX_CONTROL_0
#define MSK_F_FTDF_ON_OFF_REGMAP_RXCOORDREALIGNONLY 0x4
#define OFF_F_FTDF_ON_OFF_REGMAP_RXCOORDREALIGNONLY 2
#define SIZE_F_FTDF_ON_OFF_REGMAP_RXCOORDREALIGNONLY 1
/*
 *
 * Field: ftdf_on_off_regmap_rx_read_buf_ptr
 *
 * Indication where new data will be read
 * All four bits shall be used when using these pointer values (0d - 15d).
 * However, the Receive Packet buffer has a size of 8 entries.
 * So reading the Receive Packet buffer entries shall use the mod8 of the pointer values.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_RX_READ_BUF_PTR IND_R_FTDF_ON_OFF_REGMAP_RX_CONTROL_0
#define MSK_F_FTDF_ON_OFF_REGMAP_RX_READ_BUF_PTR 0x78
#define OFF_F_FTDF_ON_OFF_REGMAP_RX_READ_BUF_PTR 3
#define SIZE_F_FTDF_ON_OFF_REGMAP_RX_READ_BUF_PTR 4
/*
 *
 * Bit: ftdf_on_off_regmap_DisRxFrmPendingca
 *
 * Whan the control register DisRxFrmPendingCa is set to '1', the notification of the received FP bit to the LMAC Controller is disabled and thus no consequent actions will take place.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_DISRXFRMPENDINGCA IND_R_FTDF_ON_OFF_REGMAP_RX_CONTROL_0
#define MSK_F_FTDF_ON_OFF_REGMAP_DISRXFRMPENDINGCA 0x80
#define OFF_F_FTDF_ON_OFF_REGMAP_DISRXFRMPENDINGCA 7
#define SIZE_F_FTDF_ON_OFF_REGMAP_DISRXFRMPENDINGCA 1
/*
 *
 * Bit: ftdf_on_off_regmap_DisRxAckRequestca
 *
 * When the control register DisRxAckRequestca is set to '1' all consequent actions for a received Acknowledge Request bit are disabled.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_DISRXACKREQUESTCA IND_R_FTDF_ON_OFF_REGMAP_RX_CONTROL_0
#define MSK_F_FTDF_ON_OFF_REGMAP_DISRXACKREQUESTCA 0x100
#define OFF_F_FTDF_ON_OFF_REGMAP_DISRXACKREQUESTCA 8
#define SIZE_F_FTDF_ON_OFF_REGMAP_DISRXACKREQUESTCA 1
/*
 *
 * Bit: ftdf_on_off_regmap_macAlwaysPassCrcerror
 *
 * If set to '1', a FCS error will not drop the frame. However, an FCS error will be reported in the Rx meta data (crc16_error is set to '1').
 */
#define IND_F_FTDF_ON_OFF_REGMAP_MACALWAYSPASSCRCERROR IND_R_FTDF_ON_OFF_REGMAP_RX_CONTROL_0
#define MSK_F_FTDF_ON_OFF_REGMAP_MACALWAYSPASSCRCERROR 0x200
#define OFF_F_FTDF_ON_OFF_REGMAP_MACALWAYSPASSCRCERROR 9
#define SIZE_F_FTDF_ON_OFF_REGMAP_MACALWAYSPASSCRCERROR 1
/*
 *
 * Bit: ftdf_on_off_regmap_DisDataRequestca
 *
 * When the control register DisDataRequestCa is set, the notification of the received Data Request is disabled.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_DISDATAREQUESTCA IND_R_FTDF_ON_OFF_REGMAP_RX_CONTROL_0
#define MSK_F_FTDF_ON_OFF_REGMAP_DISDATAREQUESTCA 0x400
#define OFF_F_FTDF_ON_OFF_REGMAP_DISDATAREQUESTCA 10
#define SIZE_F_FTDF_ON_OFF_REGMAP_DISDATAREQUESTCA 1
/*
 *
 * Bit: ftdf_on_off_regmap_macAlwaysPassResFrameVersion
 *
 * If set to '1', a packet with a reserved FrameVersion shall not be dropped
 */
#define IND_F_FTDF_ON_OFF_REGMAP_MACALWAYSPASSRESFRAMEVERSION IND_R_FTDF_ON_OFF_REGMAP_RX_CONTROL_0
#define MSK_F_FTDF_ON_OFF_REGMAP_MACALWAYSPASSRESFRAMEVERSION 0x800
#define OFF_F_FTDF_ON_OFF_REGMAP_MACALWAYSPASSRESFRAMEVERSION 11
#define SIZE_F_FTDF_ON_OFF_REGMAP_MACALWAYSPASSRESFRAMEVERSION 1
/*
 *
 * Bit: ftdf_on_off_regmap_macAlwaysPassWrongDPANId
 *
 * If register macAlwaysPassWrongDPANId is set to '1', packet with a wrong Destiantion PanID will not be dropped.
 * However, in case of an FCS error, the packet is dropped.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_MACALWAYSPASSWRONGDPANID IND_R_FTDF_ON_OFF_REGMAP_RX_CONTROL_0
#define MSK_F_FTDF_ON_OFF_REGMAP_MACALWAYSPASSWRONGDPANID 0x1000
#define OFF_F_FTDF_ON_OFF_REGMAP_MACALWAYSPASSWRONGDPANID 12
#define SIZE_F_FTDF_ON_OFF_REGMAP_MACALWAYSPASSWRONGDPANID 1
/*
 *
 * Bit: ftdf_on_off_regmap_macAlwaysPassWrongDAddr
 *
 * If set to '1', a packet with a wrong DAddr is  not dropped
 */
#define IND_F_FTDF_ON_OFF_REGMAP_MACALWAYSPASSWRONGDADDR IND_R_FTDF_ON_OFF_REGMAP_RX_CONTROL_0
#define MSK_F_FTDF_ON_OFF_REGMAP_MACALWAYSPASSWRONGDADDR 0x2000
#define OFF_F_FTDF_ON_OFF_REGMAP_MACALWAYSPASSWRONGDADDR 13
#define SIZE_F_FTDF_ON_OFF_REGMAP_MACALWAYSPASSWRONGDADDR 1
/*
 *
 * Bit: ftdf_on_off_regmap_macAlwaysPassBeaconWrongPANId
 *
 * If the control register macAlwaysPassBeaconWrongPANId is set, the frame is not dropped in case of a mismatch in PAN-ID, irrespective of the setting of RxBeaconOnly.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_MACALWAYSPASSBEACONWRONGPANID IND_R_FTDF_ON_OFF_REGMAP_RX_CONTROL_0
#define MSK_F_FTDF_ON_OFF_REGMAP_MACALWAYSPASSBEACONWRONGPANID 0x4000
#define OFF_F_FTDF_ON_OFF_REGMAP_MACALWAYSPASSBEACONWRONGPANID 14
#define SIZE_F_FTDF_ON_OFF_REGMAP_MACALWAYSPASSBEACONWRONGPANID 1
/*
 *
 * Bit: ftdf_on_off_regmap_macAlwaysPassToPanCoordinator
 *
 * When the control register macAlwaysPassToPanCoordinator is set to '1', the frame is not dropped due to a span_coord_error.
 * However, in case of an FCS error, the packet is dropped.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_MACALWAYSPASSTOPANCOORDINATOR IND_R_FTDF_ON_OFF_REGMAP_RX_CONTROL_0
#define MSK_F_FTDF_ON_OFF_REGMAP_MACALWAYSPASSTOPANCOORDINATOR 0x8000
#define OFF_F_FTDF_ON_OFF_REGMAP_MACALWAYSPASSTOPANCOORDINATOR 15
#define SIZE_F_FTDF_ON_OFF_REGMAP_MACALWAYSPASSTOPANCOORDINATOR 1
/*
 *
 * Field: ftdf_on_off_regmap_macAlwaysPassFrmType
 *
 * The control registers macAlwaysPassFrmType[7:0], shall control if this Frame Type shall be dropped.
 * If a bit is set to '1', the Frame Type corresponding with the bit position is not dropped, even in case of a CRC error.
 * Example:
 *   if bit 3 is set to '1', Frame Type 3 shall not be dropped.
 *   If there is a FCS error, the error shall be reported in the Rx meta data (crc16_error is set to '1').
 */
#define IND_F_FTDF_ON_OFF_REGMAP_MACALWAYSPASSFRMTYPE IND_R_FTDF_ON_OFF_REGMAP_RX_CONTROL_0
#define MSK_F_FTDF_ON_OFF_REGMAP_MACALWAYSPASSFRMTYPE 0xff0000
#define OFF_F_FTDF_ON_OFF_REGMAP_MACALWAYSPASSFRMTYPE 16
#define SIZE_F_FTDF_ON_OFF_REGMAP_MACALWAYSPASSFRMTYPE 8
/*
 *
 * Bit: ftdf_on_off_regmap_macAlwaysPassWakeUp
 *
 * In CSL mode, if the control register macAlwaysPassWakeUp is set to '1', received Wake- up frames for this device are put into the Rx packet buffer without notifying the LMAC Controller (part of transparent mode control).
 */
#define IND_F_FTDF_ON_OFF_REGMAP_MACALWAYSPASSWAKEUP IND_R_FTDF_ON_OFF_REGMAP_RX_CONTROL_0
#define MSK_F_FTDF_ON_OFF_REGMAP_MACALWAYSPASSWAKEUP 0x1000000
#define OFF_F_FTDF_ON_OFF_REGMAP_MACALWAYSPASSWAKEUP 24
#define SIZE_F_FTDF_ON_OFF_REGMAP_MACALWAYSPASSWAKEUP 1
/*
 *
 * Bit: ftdf_on_off_regmap_macPassWakeUp
 *
 * In CSL mode, if set to '1', WakeUp frames will be put into the Rx buffer.
 * This can be useful for software to parse the WakeUp frame.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_MACPASSWAKEUP IND_R_FTDF_ON_OFF_REGMAP_RX_CONTROL_0
#define MSK_F_FTDF_ON_OFF_REGMAP_MACPASSWAKEUP 0x2000000
#define OFF_F_FTDF_ON_OFF_REGMAP_MACPASSWAKEUP 25
#define SIZE_F_FTDF_ON_OFF_REGMAP_MACPASSWAKEUP 1
/*
 *
 * Bit: ftdf_on_off_regmap_macImplicitBroadcast
 *
 * If set to '1', Frame Version 2 frames without Daddr or DPANId shall be accepted.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_MACIMPLICITBROADCAST IND_R_FTDF_ON_OFF_REGMAP_RX_CONTROL_0
#define MSK_F_FTDF_ON_OFF_REGMAP_MACIMPLICITBROADCAST 0x4000000
#define OFF_F_FTDF_ON_OFF_REGMAP_MACIMPLICITBROADCAST 26
#define SIZE_F_FTDF_ON_OFF_REGMAP_MACIMPLICITBROADCAST 1
/*
 *
 * Bit: ftdf_on_off_regmap_DisRxAckReceivedca
 *
 * If set to '1', the LMAC controller shall ignore all consequent actions upon a set AR bit in the transmitted frame (e.g. enabling Rx-on mode after the transmission and wait for an ACK).
 */
#define IND_F_FTDF_ON_OFF_REGMAP_DISRXACKRECEIVEDCA IND_R_FTDF_ON_OFF_REGMAP_RX_CONTROL_0
#define MSK_F_FTDF_ON_OFF_REGMAP_DISRXACKRECEIVEDCA 0x8000000
#define OFF_F_FTDF_ON_OFF_REGMAP_DISRXACKRECEIVEDCA 27
#define SIZE_F_FTDF_ON_OFF_REGMAP_DISRXACKRECEIVEDCA 1

/*
 *
 * REGISTER: ftdf_on_off_regmap_rx_event
 *
 * Initialization value: 0x0  Initialization mask: 0xf
 * Access mode: E-RCW2 (event read/clear on write 1)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_RX_EVENT 0x40090204
#define SIZE_R_FTDF_ON_OFF_REGMAP_RX_EVENT 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_RX_EVENT 0xf

/*
 *
 * Bit: ftdf_on_off_regmap_RxSof_e
 *
 * Set to '1' when RX_SOF has been detected.
 * This event bit contributes to ftdf_ce[1].
 */
#define IND_F_FTDF_ON_OFF_REGMAP_RXSOF_E IND_R_FTDF_ON_OFF_REGMAP_RX_EVENT
#define MSK_F_FTDF_ON_OFF_REGMAP_RXSOF_E 0x1
#define OFF_F_FTDF_ON_OFF_REGMAP_RXSOF_E 0
#define SIZE_F_FTDF_ON_OFF_REGMAP_RXSOF_E 1
/*
 *
 * Bit: ftdf_on_off_regmap_rx_overflow_e
 *
 * If set to '1' it indicates that the Rx packet buffer has an overflow.
 * This event bit contributes to ftdf_ce[1].
 */
#define IND_F_FTDF_ON_OFF_REGMAP_RX_OVERFLOW_E IND_R_FTDF_ON_OFF_REGMAP_RX_EVENT
#define MSK_F_FTDF_ON_OFF_REGMAP_RX_OVERFLOW_E 0x2
#define OFF_F_FTDF_ON_OFF_REGMAP_RX_OVERFLOW_E 1
#define SIZE_F_FTDF_ON_OFF_REGMAP_RX_OVERFLOW_E 1
/*
 *
 * Bit: ftdf_on_off_regmap_rx_buf_avail_e
 *
 * If set to '1' it indicates that a new valid packet completely has been received
 * This event bit contributes to ftdf_ce[1].
 */
#define IND_F_FTDF_ON_OFF_REGMAP_RX_BUF_AVAIL_E IND_R_FTDF_ON_OFF_REGMAP_RX_EVENT
#define MSK_F_FTDF_ON_OFF_REGMAP_RX_BUF_AVAIL_E 0x4
#define OFF_F_FTDF_ON_OFF_REGMAP_RX_BUF_AVAIL_E 2
#define SIZE_F_FTDF_ON_OFF_REGMAP_RX_BUF_AVAIL_E 1
/*
 *
 * Bit: ftdf_on_off_regmap_rxbyte_e
 *
 * If set to '1' it indicates that the first byte of a new packet has been received
 * This event bit contributes to ftdf_ce[1].
 */
#define IND_F_FTDF_ON_OFF_REGMAP_RXBYTE_E IND_R_FTDF_ON_OFF_REGMAP_RX_EVENT
#define MSK_F_FTDF_ON_OFF_REGMAP_RXBYTE_E 0x8
#define OFF_F_FTDF_ON_OFF_REGMAP_RXBYTE_E 3
#define SIZE_F_FTDF_ON_OFF_REGMAP_RXBYTE_E 1

/*
 *
 * REGISTER: ftdf_on_off_regmap_rx_mask
 *
 * Initialization value: 0x0  Initialization mask: 0xf
 * Access mode: EM-RW (event mask)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_RX_MASK 0x40090208
#define SIZE_R_FTDF_ON_OFF_REGMAP_RX_MASK 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_RX_MASK 0xf

/*
 *
 * Bit: ftdf_on_off_regmap_RxSof_m
 *
 * Mask bit for event "RxSof_e".
 * The mask bit is masking when cleared to '0' (default value) and will enable the contribution to the interrupt when set to '1'.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_RXSOF_M IND_R_FTDF_ON_OFF_REGMAP_RX_MASK
#define MSK_F_FTDF_ON_OFF_REGMAP_RXSOF_M 0x1
#define OFF_F_FTDF_ON_OFF_REGMAP_RXSOF_M 0
#define SIZE_F_FTDF_ON_OFF_REGMAP_RXSOF_M 1
/*
 *
 * Bit: ftdf_on_off_regmap_rx_overflow_m
 *
 * Mask bit for event " rx_overflow_e".
 * The mask bit is masking when cleared to '0' (default value) and will enable the contribution to the interrupt when set to '1'.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_RX_OVERFLOW_M IND_R_FTDF_ON_OFF_REGMAP_RX_MASK
#define MSK_F_FTDF_ON_OFF_REGMAP_RX_OVERFLOW_M 0x2
#define OFF_F_FTDF_ON_OFF_REGMAP_RX_OVERFLOW_M 1
#define SIZE_F_FTDF_ON_OFF_REGMAP_RX_OVERFLOW_M 1
/*
 *
 * Bit: ftdf_on_off_regmap_rx_buf_avail_m
 *
 * Mask bit for event "rx_buf_avail_e".
 * The mask bit is masking when cleared to '0' (default value) and will enable the contribution to the interrupt when set to '1'.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_RX_BUF_AVAIL_M IND_R_FTDF_ON_OFF_REGMAP_RX_MASK
#define MSK_F_FTDF_ON_OFF_REGMAP_RX_BUF_AVAIL_M 0x4
#define OFF_F_FTDF_ON_OFF_REGMAP_RX_BUF_AVAIL_M 2
#define SIZE_F_FTDF_ON_OFF_REGMAP_RX_BUF_AVAIL_M 1
/*
 *
 * Bit: ftdf_on_off_regmap_rxbyte_m
 *
 * Mask bit for event "rxbyte_e".
 * The mask bit is masking when cleared to '0' (default value) and will enable the contribution to the interrupt when set to '1'.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_RXBYTE_M IND_R_FTDF_ON_OFF_REGMAP_RX_MASK
#define MSK_F_FTDF_ON_OFF_REGMAP_RXBYTE_M 0x8
#define OFF_F_FTDF_ON_OFF_REGMAP_RXBYTE_M 3
#define SIZE_F_FTDF_ON_OFF_REGMAP_RXBYTE_M 1

/*
 *
 * REGISTER: ftdf_on_off_regmap_rx_status
 *
 * Initialization value: none.
 * Access mode: S-R (state - read)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_RX_STATUS 0x4009020c
#define SIZE_R_FTDF_ON_OFF_REGMAP_RX_STATUS 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_RX_STATUS 0x1f

/*
 *
 * Bit: ftdf_on_off_regmap_rx_buff_is_full
 *
 * If set to '1', it indicates that the Rx packet buffer is full
 */
#define IND_F_FTDF_ON_OFF_REGMAP_RX_BUFF_IS_FULL IND_R_FTDF_ON_OFF_REGMAP_RX_STATUS
#define MSK_F_FTDF_ON_OFF_REGMAP_RX_BUFF_IS_FULL 0x1
#define OFF_F_FTDF_ON_OFF_REGMAP_RX_BUFF_IS_FULL 0
#define SIZE_F_FTDF_ON_OFF_REGMAP_RX_BUFF_IS_FULL 1
/*
 *
 * Field: ftdf_on_off_regmap_rx_write_buf_ptr
 *
 * Indication where new data will be written.
 * All four bits shall be used when using these pointer values (0d - 15d).
 * However, the Receive Packet buffer has a size of 8 entries.
 * So reading the Receive Packet buffer entries shall use the mod8 of the pointer values.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_RX_WRITE_BUF_PTR IND_R_FTDF_ON_OFF_REGMAP_RX_STATUS
#define MSK_F_FTDF_ON_OFF_REGMAP_RX_WRITE_BUF_PTR 0x1e
#define OFF_F_FTDF_ON_OFF_REGMAP_RX_WRITE_BUF_PTR 1
#define SIZE_F_FTDF_ON_OFF_REGMAP_RX_WRITE_BUF_PTR 4

/*
 *
 * REGISTER: ftdf_on_off_regmap_SymbolTimeSnapshotVal
 *
 * Initialization value: none.
 * Access mode: S-R (state - read)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_SYMBOLTIMESNAPSHOTVAL 0x40090210
#define SIZE_R_FTDF_ON_OFF_REGMAP_SYMBOLTIMESNAPSHOTVAL 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_SYMBOLTIMESNAPSHOTVAL 0xffffffff

/*
 *
 * Field: ftdf_on_off_regmap_SymbolTimeSnapshotVal
 *
 * The Status register SymbolTimeSnapshotVal indicates the actual value of the TimeStamp generator.
 * This can be useful for software to use e.g. in CSL mode at creating an Enhanced ACK to calculate the CSL phase and period.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_SYMBOLTIMESNAPSHOTVAL IND_R_FTDF_ON_OFF_REGMAP_SYMBOLTIMESNAPSHOTVAL
#define MSK_F_FTDF_ON_OFF_REGMAP_SYMBOLTIMESNAPSHOTVAL 0xffffffff
#define OFF_F_FTDF_ON_OFF_REGMAP_SYMBOLTIMESNAPSHOTVAL 0
#define SIZE_F_FTDF_ON_OFF_REGMAP_SYMBOLTIMESNAPSHOTVAL 32

/*
 *
 * REGISTER: ftdf_on_off_regmap_rx_status_delta
 *
 * Initialization value: 0x0  Initialization mask: 0x1
 * Access mode: D-RCW (delta read/clear on write 1)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_RX_STATUS_DELTA 0x40090220
#define SIZE_R_FTDF_ON_OFF_REGMAP_RX_STATUS_DELTA 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_RX_STATUS_DELTA 0x1

/*
 *
 * Bit: ftdf_on_off_regmap_rx_buff_is_full_d
 *
 * Delta bit of status "rx_buff_is_full".
 * This delta bit is set to '1' on each change of this status, contributes to ftdf_ce[1].
 */
#define IND_F_FTDF_ON_OFF_REGMAP_RX_BUFF_IS_FULL_D IND_R_FTDF_ON_OFF_REGMAP_RX_STATUS_DELTA
#define MSK_F_FTDF_ON_OFF_REGMAP_RX_BUFF_IS_FULL_D 0x1
#define OFF_F_FTDF_ON_OFF_REGMAP_RX_BUFF_IS_FULL_D 0
#define SIZE_F_FTDF_ON_OFF_REGMAP_RX_BUFF_IS_FULL_D 1

/*
 *
 * REGISTER: ftdf_on_off_regmap_rx_status_mask
 *
 * Initialization value: 0x0  Initialization mask: 0x1
 * Access mode: DM-RW (delta mask)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_RX_STATUS_MASK 0x40090224
#define SIZE_R_FTDF_ON_OFF_REGMAP_RX_STATUS_MASK 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_RX_STATUS_MASK 0x1

/*
 *
 * Bit: ftdf_on_off_regmap_rx_buff_is_full_m
 *
 * Mask bit for delta bit "rx_buff_is_full_d".
 * The mask bit is masking when cleared to '0' (default value) and will enable the contribution to the interrupt when set to '1'.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_RX_BUFF_IS_FULL_M IND_R_FTDF_ON_OFF_REGMAP_RX_STATUS_MASK
#define MSK_F_FTDF_ON_OFF_REGMAP_RX_BUFF_IS_FULL_M 0x1
#define OFF_F_FTDF_ON_OFF_REGMAP_RX_BUFF_IS_FULL_M 0
#define SIZE_F_FTDF_ON_OFF_REGMAP_RX_BUFF_IS_FULL_M 1

/*
 *
 * REGISTER: ftdf_on_off_regmap_tx_control_0
 *
 * Initialization value: 0x4350  Initialization mask: 0x7ff1
 * Access mode: C-RW (control - read/write)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_TX_CONTROL_0 0x40090240
#define SIZE_R_FTDF_ON_OFF_REGMAP_TX_CONTROL_0 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_TX_CONTROL_0 0x7ff1

/*
 *
 * Bit: ftdf_on_off_regmap_DbgTxTransparentMode
 *
 * If set to '1', the MPDU octets pass transparently through the MAC in the transmit direction (for debug purpose).
 */
#define IND_F_FTDF_ON_OFF_REGMAP_DBGTXTRANSPARENTMODE IND_R_FTDF_ON_OFF_REGMAP_TX_CONTROL_0
#define MSK_F_FTDF_ON_OFF_REGMAP_DBGTXTRANSPARENTMODE 0x1
#define OFF_F_FTDF_ON_OFF_REGMAP_DBGTXTRANSPARENTMODE 0
#define SIZE_F_FTDF_ON_OFF_REGMAP_DBGTXTRANSPARENTMODE 1
/*
 *
 * Field: ftdf_on_off_regmap_macMaxBE
 *
 * CSMA-CA: Maximum Backoff Exponent (range 3-8)
 */
#define IND_F_FTDF_ON_OFF_REGMAP_MACMAXBE IND_R_FTDF_ON_OFF_REGMAP_TX_CONTROL_0
#define MSK_F_FTDF_ON_OFF_REGMAP_MACMAXBE 0xf0
#define OFF_F_FTDF_ON_OFF_REGMAP_MACMAXBE 4
#define SIZE_F_FTDF_ON_OFF_REGMAP_MACMAXBE 4
/*
 *
 * Field: ftdf_on_off_regmap_macMinBE
 *
 * CSMA-CA: Minimum Backoff Exponent (range 0-macMaxBE)
 */
#define IND_F_FTDF_ON_OFF_REGMAP_MACMINBE IND_R_FTDF_ON_OFF_REGMAP_TX_CONTROL_0
#define MSK_F_FTDF_ON_OFF_REGMAP_MACMINBE 0xf00
#define OFF_F_FTDF_ON_OFF_REGMAP_MACMINBE 8
#define SIZE_F_FTDF_ON_OFF_REGMAP_MACMINBE 4
/*
 *
 * Field: ftdf_on_off_regmap_macMaxCSMABackoffs
 *
 * CSMA-CA: Maximum number of CSMA-CA backoffs (range 0-5)
 */
#define IND_F_FTDF_ON_OFF_REGMAP_MACMAXCSMABACKOFFS IND_R_FTDF_ON_OFF_REGMAP_TX_CONTROL_0
#define MSK_F_FTDF_ON_OFF_REGMAP_MACMAXCSMABACKOFFS 0x7000
#define OFF_F_FTDF_ON_OFF_REGMAP_MACMAXCSMABACKOFFS 12
#define SIZE_F_FTDF_ON_OFF_REGMAP_MACMAXCSMABACKOFFS 3

/*
 *
 * REGISTER: ftdf_on_off_regmap_ftdf_ce
 *
 * Initialization value: none.
 * Access mode: EC-R (composite event)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_FTDF_CE 0x40090250
#define SIZE_R_FTDF_ON_OFF_REGMAP_FTDF_CE 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_FTDF_CE 0x3f

/*
 *
 * Field: ftdf_on_off_regmap_ftdf_ce
 *
 * Composite service request from ftdf macro (see FR0400 in v40.100.2.41.pdf), set to '1' if the branch currently contributes to the interrupt.
 * Bit 0 = unused
 * Bit 1 = rx interrupts
 * Bit 2 = unused
 * Bit 3 = miscelaneous interrupts
 * Bit 4 = tx interrupts
 * Bit 5 = Reserved
 * Upon an interrupt, using the ftdf_ce bits it can be checked which interrupt branch creates this interrupt.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_FTDF_CE IND_R_FTDF_ON_OFF_REGMAP_FTDF_CE
#define MSK_F_FTDF_ON_OFF_REGMAP_FTDF_CE 0x3f
#define OFF_F_FTDF_ON_OFF_REGMAP_FTDF_CE 0
#define SIZE_F_FTDF_ON_OFF_REGMAP_FTDF_CE 6

/*
 *
 * REGISTER: ftdf_on_off_regmap_ftdf_cm
 *
 * Initialization value: 0x0  Initialization mask: 0x3f
 * Access mode: ECM-RW (composite event mask)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_FTDF_CM 0x40090254
#define SIZE_R_FTDF_ON_OFF_REGMAP_FTDF_CM 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_FTDF_CM 0x3f

/*
 *
 * Field: ftdf_on_off_regmap_ftdf_cm
 *
 * mask bits for ftf_ce.
 * The mask bit is masking when cleared to '0' (default value) and will enable the contribution to the interrupt when set to '1'.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_FTDF_CM IND_R_FTDF_ON_OFF_REGMAP_FTDF_CM
#define MSK_F_FTDF_ON_OFF_REGMAP_FTDF_CM 0x3f
#define OFF_F_FTDF_ON_OFF_REGMAP_FTDF_CM 0
#define SIZE_F_FTDF_ON_OFF_REGMAP_FTDF_CM 6

/*
 *
 * REGISTER: ftdf_on_off_regmap_SyncTimeStampThr
 *
 * Initialization value: 0x0  Initialization mask: 0x1ffffff
 * Access mode: C-RW (control - read/write)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_SYNCTIMESTAMPTHR 0x40090304
#define SIZE_R_FTDF_ON_OFF_REGMAP_SYNCTIMESTAMPTHR 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_SYNCTIMESTAMPTHR 0x1ffffff

/*
 *
 * Field: ftdf_on_off_regmap_SyncTimeStampThr
 *
 * Threshold for synchronize the timestamp counter: at this value of the event counter the synchronization of the timestamp (symbol) counter is done (if SyncTimeStampEna is set to '1').
 * If SyncTimeStamp_e is set to '1' the synchronization has taken place.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_SYNCTIMESTAMPTHR IND_R_FTDF_ON_OFF_REGMAP_SYNCTIMESTAMPTHR
#define MSK_F_FTDF_ON_OFF_REGMAP_SYNCTIMESTAMPTHR 0x1ffffff
#define OFF_F_FTDF_ON_OFF_REGMAP_SYNCTIMESTAMPTHR 0
#define SIZE_F_FTDF_ON_OFF_REGMAP_SYNCTIMESTAMPTHR 25

/*
 *
 * REGISTER: ftdf_on_off_regmap_SyncTimeStampVal
 *
 * Initialization value: 0x0  Initialization mask: 0xffffffff
 * Access mode: C-RW (control - read/write)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_SYNCTIMESTAMPVAL 0x40090308
#define SIZE_R_FTDF_ON_OFF_REGMAP_SYNCTIMESTAMPVAL 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_SYNCTIMESTAMPVAL 0xffffffff

/*
 *
 * Field: ftdf_on_off_regmap_SyncTimeStampVal
 *
 * Value to synchronize the timestamp counter with at the moment indicated by SyncTimeStampThr.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_SYNCTIMESTAMPVAL IND_R_FTDF_ON_OFF_REGMAP_SYNCTIMESTAMPVAL
#define MSK_F_FTDF_ON_OFF_REGMAP_SYNCTIMESTAMPVAL 0xffffffff
#define OFF_F_FTDF_ON_OFF_REGMAP_SYNCTIMESTAMPVAL 0
#define SIZE_F_FTDF_ON_OFF_REGMAP_SYNCTIMESTAMPVAL 32

/*
 *
 * REGISTER: ftdf_on_off_regmap_SyncTimeStampPhaseVal
 *
 * Initialization value: 0x0  Initialization mask: 0xff
 * Access mode: C-RW (control - read/write)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_SYNCTIMESTAMPPHASEVAL 0x40090320
#define SIZE_R_FTDF_ON_OFF_REGMAP_SYNCTIMESTAMPPHASEVAL 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_SYNCTIMESTAMPPHASEVAL 0xff

/*
 *
 * Field: ftdf_on_off_regmap_SyncTimeStampPhaseVal
 *
 * Value to synchronize the timestamp counter phase with at the moment indicated by SyncTimeStampThr.
 * Please note the +1 correction needed for most accurate result (+0.5 is than the average error, resulting is a just too fast clock).
 */
#define IND_F_FTDF_ON_OFF_REGMAP_SYNCTIMESTAMPPHASEVAL IND_R_FTDF_ON_OFF_REGMAP_SYNCTIMESTAMPPHASEVAL
#define MSK_F_FTDF_ON_OFF_REGMAP_SYNCTIMESTAMPPHASEVAL 0xff
#define OFF_F_FTDF_ON_OFF_REGMAP_SYNCTIMESTAMPPHASEVAL 0
#define SIZE_F_FTDF_ON_OFF_REGMAP_SYNCTIMESTAMPPHASEVAL 8

/*
 *
 * REGISTER: ftdf_on_off_regmap_timer_control_1
 *
 * Initialization value: 0x0  Initialization mask: 0x2
 * Access mode: C-RW (control - read/write)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_TIMER_CONTROL_1 0x4009030c
#define SIZE_R_FTDF_ON_OFF_REGMAP_TIMER_CONTROL_1 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_TIMER_CONTROL_1 0x2

/*
 *
 * Bit: ftdf_on_off_regmap_SyncTimeStampEna
 *
 * If set to '1', the synchronization of the timestamp counter after a deep-sleep cycle will be performed when SyncTimeStampThr matches the value of the event (wake-up) counter.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_SYNCTIMESTAMPENA IND_R_FTDF_ON_OFF_REGMAP_TIMER_CONTROL_1
#define MSK_F_FTDF_ON_OFF_REGMAP_SYNCTIMESTAMPENA 0x2
#define OFF_F_FTDF_ON_OFF_REGMAP_SYNCTIMESTAMPENA 1
#define SIZE_F_FTDF_ON_OFF_REGMAP_SYNCTIMESTAMPENA 1

/*
 *
 * REGISTER: ftdf_on_off_regmap_macTxStdAckFrmCnt
 *
 * Initialization value: none.
 * Access mode: S-R (state - read)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_MACTXSTDACKFRMCNT 0x40090310
#define SIZE_R_FTDF_ON_OFF_REGMAP_MACTXSTDACKFRMCNT 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_MACTXSTDACKFRMCNT 0xffffffff

/*
 *
 * Field: ftdf_on_off_regmap_macTxStdAckFrmCnt
 *
 * Traffic counter: the number of standard Acknowledgment frames transmitted after the last deep-sleep cycle.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_MACTXSTDACKFRMCNT IND_R_FTDF_ON_OFF_REGMAP_MACTXSTDACKFRMCNT
#define MSK_F_FTDF_ON_OFF_REGMAP_MACTXSTDACKFRMCNT 0xffffffff
#define OFF_F_FTDF_ON_OFF_REGMAP_MACTXSTDACKFRMCNT 0
#define SIZE_F_FTDF_ON_OFF_REGMAP_MACTXSTDACKFRMCNT 32

/*
 *
 * REGISTER: ftdf_on_off_regmap_macRxStdAckFrmOkCnt
 *
 * Initialization value: none.
 * Access mode: S-R (state - read)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_MACRXSTDACKFRMOKCNT 0x40090314
#define SIZE_R_FTDF_ON_OFF_REGMAP_MACRXSTDACKFRMOKCNT 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_MACRXSTDACKFRMOKCNT 0xffffffff

/*
 *
 * Field: ftdf_on_off_regmap_macRxStdAckFrmOkCnt
 *
 * Traffic counter: the number of Standard Acknowledgment frames received after the last deep-sleep cycle.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_MACRXSTDACKFRMOKCNT IND_R_FTDF_ON_OFF_REGMAP_MACRXSTDACKFRMOKCNT
#define MSK_F_FTDF_ON_OFF_REGMAP_MACRXSTDACKFRMOKCNT 0xffffffff
#define OFF_F_FTDF_ON_OFF_REGMAP_MACRXSTDACKFRMOKCNT 0
#define SIZE_F_FTDF_ON_OFF_REGMAP_MACRXSTDACKFRMOKCNT 32

/*
 *
 * REGISTER: ftdf_on_off_regmap_macRxAddrFailFrmCnt
 *
 * Initialization value: none.
 * Access mode: S-R (state - read)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_MACRXADDRFAILFRMCNT 0x40090318
#define SIZE_R_FTDF_ON_OFF_REGMAP_MACRXADDRFAILFRMCNT 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_MACRXADDRFAILFRMCNT 0xffffffff

/*
 *
 * Field: ftdf_on_off_regmap_macRxAddrFailFrmCnt
 *
 * Traffic counter: the number of frames discarded due to incorrect address or PAN Id after the last deep-sleep cycle.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_MACRXADDRFAILFRMCNT IND_R_FTDF_ON_OFF_REGMAP_MACRXADDRFAILFRMCNT
#define MSK_F_FTDF_ON_OFF_REGMAP_MACRXADDRFAILFRMCNT 0xffffffff
#define OFF_F_FTDF_ON_OFF_REGMAP_MACRXADDRFAILFRMCNT 0
#define SIZE_F_FTDF_ON_OFF_REGMAP_MACRXADDRFAILFRMCNT 32

/*
 *
 * REGISTER: ftdf_on_off_regmap_macRxUnsupFrmCnt
 *
 * Initialization value: none.
 * Access mode: S-R (state - read)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_MACRXUNSUPFRMCNT 0x4009031c
#define SIZE_R_FTDF_ON_OFF_REGMAP_MACRXUNSUPFRMCNT 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_MACRXUNSUPFRMCNT 0xffffffff

/*
 *
 * Field: ftdf_on_off_regmap_macRxUnsupFrmCnt
 *
 * Traffic counter: the number of frames which do pass the checks but are not supported after the last deep-sleep cycle.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_MACRXUNSUPFRMCNT IND_R_FTDF_ON_OFF_REGMAP_MACRXUNSUPFRMCNT
#define MSK_F_FTDF_ON_OFF_REGMAP_MACRXUNSUPFRMCNT 0xffffffff
#define OFF_F_FTDF_ON_OFF_REGMAP_MACRXUNSUPFRMCNT 0
#define SIZE_F_FTDF_ON_OFF_REGMAP_MACRXUNSUPFRMCNT 32

/*
 *
 * REGISTER: ftdf_on_off_regmap_macFCSErrorCount
 *
 * Initialization value: none.
 * Access mode: S-R (state - read)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_MACFCSERRORCOUNT 0x40090340
#define SIZE_R_FTDF_ON_OFF_REGMAP_MACFCSERRORCOUNT 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_MACFCSERRORCOUNT 0xffffffff

/*
 *
 * Field: ftdf_on_off_regmap_macFCSErrorCount
 *
 * metrics counter: the number of received frames that were discarded due to an incorrect FCS after the last deep-sleep cycle.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_MACFCSERRORCOUNT IND_R_FTDF_ON_OFF_REGMAP_MACFCSERRORCOUNT
#define MSK_F_FTDF_ON_OFF_REGMAP_MACFCSERRORCOUNT 0xffffffff
#define OFF_F_FTDF_ON_OFF_REGMAP_MACFCSERRORCOUNT 0
#define SIZE_F_FTDF_ON_OFF_REGMAP_MACFCSERRORCOUNT 32

/*
 *
 * REGISTER: ftdf_on_off_regmap_LmacReset
 *
 * Initialization value: 0x0  Initialization mask: 0x106df
 * Access mode: OS-W (one-shot - write)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_LMACRESET 0x40090360
#define SIZE_R_FTDF_ON_OFF_REGMAP_LMACRESET 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_LMACRESET 0x106df

/*
 *
 * Bit: ftdf_on_off_regmap_LmacReset_control
 *
 * LmacReset_control: A '1' resets LMAC Controller (for debug and MLME-reset)
 */
#define IND_F_FTDF_ON_OFF_REGMAP_LMACRESET_CONTROL IND_R_FTDF_ON_OFF_REGMAP_LMACRESET
#define MSK_F_FTDF_ON_OFF_REGMAP_LMACRESET_CONTROL 0x1
#define OFF_F_FTDF_ON_OFF_REGMAP_LMACRESET_CONTROL 0
#define SIZE_F_FTDF_ON_OFF_REGMAP_LMACRESET_CONTROL 1
/*
 *
 * Bit: ftdf_on_off_regmap_LmacReset_rx
 *
 * LmacReset_rx: A '1' resets LMAC rx pipeline (for debug and MLME-reset)
 */
#define IND_F_FTDF_ON_OFF_REGMAP_LMACRESET_RX IND_R_FTDF_ON_OFF_REGMAP_LMACRESET
#define MSK_F_FTDF_ON_OFF_REGMAP_LMACRESET_RX 0x2
#define OFF_F_FTDF_ON_OFF_REGMAP_LMACRESET_RX 1
#define SIZE_F_FTDF_ON_OFF_REGMAP_LMACRESET_RX 1
/*
 *
 * Bit: ftdf_on_off_regmap_LmacReset_tx
 *
 * LmacReset_tx: A '1' resets LMAC tx pipeline (for debug and MLME-reset)
 */
#define IND_F_FTDF_ON_OFF_REGMAP_LMACRESET_TX IND_R_FTDF_ON_OFF_REGMAP_LMACRESET
#define MSK_F_FTDF_ON_OFF_REGMAP_LMACRESET_TX 0x4
#define OFF_F_FTDF_ON_OFF_REGMAP_LMACRESET_TX 2
#define SIZE_F_FTDF_ON_OFF_REGMAP_LMACRESET_TX 1
/*
 *
 * Bit: ftdf_on_off_regmap_LmacReset_ahb
 *
 * LmacReset_ahb: A '1' resets LMAC ahb interface (for debug and MLME-reset)
 */
#define IND_F_FTDF_ON_OFF_REGMAP_LMACRESET_AHB IND_R_FTDF_ON_OFF_REGMAP_LMACRESET
#define MSK_F_FTDF_ON_OFF_REGMAP_LMACRESET_AHB 0x8
#define OFF_F_FTDF_ON_OFF_REGMAP_LMACRESET_AHB 3
#define SIZE_F_FTDF_ON_OFF_REGMAP_LMACRESET_AHB 1
/*
 *
 * Bit: ftdf_on_off_regmap_LmacReset_oreg
 *
 * LmacReset_oreg: A '1' resets LMAC on_off regmap (for debug and MLME-reset)
 *
 * #$LmacReset_areg@on_off_regmap
 * #LmacReset_areg: A '1' Resets LMAC always_on regmap (for debug and MLME-reset)
 */
#define IND_F_FTDF_ON_OFF_REGMAP_LMACRESET_OREG IND_R_FTDF_ON_OFF_REGMAP_LMACRESET
#define MSK_F_FTDF_ON_OFF_REGMAP_LMACRESET_OREG 0x10
#define OFF_F_FTDF_ON_OFF_REGMAP_LMACRESET_OREG 4
#define SIZE_F_FTDF_ON_OFF_REGMAP_LMACRESET_OREG 1
/*
 *
 * Bit: ftdf_on_off_regmap_LmacReset_tstim
 *
 * LmacReset_tstim: A '1' resets LMAC timestamp timer (for debug and MLME-reset)
 */
#define IND_F_FTDF_ON_OFF_REGMAP_LMACRESET_TSTIM IND_R_FTDF_ON_OFF_REGMAP_LMACRESET
#define MSK_F_FTDF_ON_OFF_REGMAP_LMACRESET_TSTIM 0x40
#define OFF_F_FTDF_ON_OFF_REGMAP_LMACRESET_TSTIM 6
#define SIZE_F_FTDF_ON_OFF_REGMAP_LMACRESET_TSTIM 1
/*
 *
 * Bit: ftdf_on_off_regmap_LmacReset_sec
 *
 * LmacReset_sec: A '1' resets LMAC security (for debug and MLME-reset)
 *
 * #$LmacReset_wutim@on_off_regmap
 * #LmacReset_wutim: A '1' Resets LMAC wake-up timer (for debug and MLME-reset)
 */
#define IND_F_FTDF_ON_OFF_REGMAP_LMACRESET_SEC IND_R_FTDF_ON_OFF_REGMAP_LMACRESET
#define MSK_F_FTDF_ON_OFF_REGMAP_LMACRESET_SEC 0x80
#define OFF_F_FTDF_ON_OFF_REGMAP_LMACRESET_SEC 7
#define SIZE_F_FTDF_ON_OFF_REGMAP_LMACRESET_SEC 1
/*
 *
 * Bit: ftdf_on_off_regmap_LmacReset_count
 *
 * LmacReset_count: A '1' resets LMAC mac counters (for debug and MLME-reset)
 */
#define IND_F_FTDF_ON_OFF_REGMAP_LMACRESET_COUNT IND_R_FTDF_ON_OFF_REGMAP_LMACRESET
#define MSK_F_FTDF_ON_OFF_REGMAP_LMACRESET_COUNT 0x200
#define OFF_F_FTDF_ON_OFF_REGMAP_LMACRESET_COUNT 9
#define SIZE_F_FTDF_ON_OFF_REGMAP_LMACRESET_COUNT 1
/*
 *
 * Bit: ftdf_on_off_regmap_LmacReset_timctrl
 *
 * LmacReset_count: A '1' resets LMAC timing control block (for debug and MLME-reset)
 */
#define IND_F_FTDF_ON_OFF_REGMAP_LMACRESET_TIMCTRL IND_R_FTDF_ON_OFF_REGMAP_LMACRESET
#define MSK_F_FTDF_ON_OFF_REGMAP_LMACRESET_TIMCTRL 0x400
#define OFF_F_FTDF_ON_OFF_REGMAP_LMACRESET_TIMCTRL 10
#define SIZE_F_FTDF_ON_OFF_REGMAP_LMACRESET_TIMCTRL 1
/*
 *
 * Bit: ftdf_on_off_regmap_LmacGlobReset_count
 *
 * If set, the LMAC performance and traffic counters will be reset.
 * Use this register for functionally reset these counters.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_LMACGLOBRESET_COUNT IND_R_FTDF_ON_OFF_REGMAP_LMACRESET
#define MSK_F_FTDF_ON_OFF_REGMAP_LMACGLOBRESET_COUNT 0x10000
#define OFF_F_FTDF_ON_OFF_REGMAP_LMACGLOBRESET_COUNT 16
#define SIZE_F_FTDF_ON_OFF_REGMAP_LMACGLOBRESET_COUNT 1

/*
 *
 * REGISTER: ftdf_on_off_regmap_wakeup_control_os
 *
 * Initialization value: 0x0  Initialization mask: 0x3
 * Access mode: OS-W (one-shot - write)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_WAKEUP_CONTROL_OS 0x40090364
#define SIZE_R_FTDF_ON_OFF_REGMAP_WAKEUP_CONTROL_OS 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_WAKEUP_CONTROL_OS 0x3

/*
 *
 * Bit: ftdf_on_off_regmap_WakeupTimerEnable_set
 *
 * If set, WakeupTimerEnableStatus will be set.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_WAKEUPTIMERENABLE_SET IND_R_FTDF_ON_OFF_REGMAP_WAKEUP_CONTROL_OS
#define MSK_F_FTDF_ON_OFF_REGMAP_WAKEUPTIMERENABLE_SET 0x1
#define OFF_F_FTDF_ON_OFF_REGMAP_WAKEUPTIMERENABLE_SET 0
#define SIZE_F_FTDF_ON_OFF_REGMAP_WAKEUPTIMERENABLE_SET 1
/*
 *
 * Bit: ftdf_on_off_regmap_WakeupTimerEnable_clear
 *
 * If set, WakeupTimerEnableStatus will be cleared.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_WAKEUPTIMERENABLE_CLEAR IND_R_FTDF_ON_OFF_REGMAP_WAKEUP_CONTROL_OS
#define MSK_F_FTDF_ON_OFF_REGMAP_WAKEUPTIMERENABLE_CLEAR 0x2
#define OFF_F_FTDF_ON_OFF_REGMAP_WAKEUPTIMERENABLE_CLEAR 1
#define SIZE_F_FTDF_ON_OFF_REGMAP_WAKEUPTIMERENABLE_CLEAR 1

/*
 *
 * REGISTER: ftdf_on_off_regmap_SymbolTimeThr
 *
 * Initialization value: 0x0  Initialization mask: 0xffffffff
 * Access mode: C-RW (control - read/write)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_SYMBOLTIMETHR 0x40090380
#define SIZE_R_FTDF_ON_OFF_REGMAP_SYMBOLTIMETHR 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_SYMBOLTIMETHR 0xffffffff

/*
 *
 * Field: ftdf_on_off_regmap_SymbolTimeThr
 *
 * Symboltime Threshold to generate a general interrupt when this value matches the symbol counter value.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_SYMBOLTIMETHR IND_R_FTDF_ON_OFF_REGMAP_SYMBOLTIMETHR
#define MSK_F_FTDF_ON_OFF_REGMAP_SYMBOLTIMETHR 0xffffffff
#define OFF_F_FTDF_ON_OFF_REGMAP_SYMBOLTIMETHR 0
#define SIZE_F_FTDF_ON_OFF_REGMAP_SYMBOLTIMETHR 32

/*
 *
 * REGISTER: ftdf_on_off_regmap_SymbolTime2Thr
 *
 * Initialization value: 0x0  Initialization mask: 0xffffffff
 * Access mode: C-RW (control - read/write)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_SYMBOLTIME2THR 0x40090384
#define SIZE_R_FTDF_ON_OFF_REGMAP_SYMBOLTIME2THR 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_SYMBOLTIME2THR 0xffffffff

/*
 *
 * Field: ftdf_on_off_regmap_SymbolTime2Thr
 *
 * Symboltime 2 Threshold to generate a general interrupt when this value matches the symbol counter value.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_SYMBOLTIME2THR IND_R_FTDF_ON_OFF_REGMAP_SYMBOLTIME2THR
#define MSK_F_FTDF_ON_OFF_REGMAP_SYMBOLTIME2THR 0xffffffff
#define OFF_F_FTDF_ON_OFF_REGMAP_SYMBOLTIME2THR 0
#define SIZE_F_FTDF_ON_OFF_REGMAP_SYMBOLTIME2THR 32

/*
 *
 * REGISTER: ftdf_on_off_regmap_DebugControl
 *
 * Initialization value: 0x0  Initialization mask: 0x1ff
 * Access mode: C-RW (control - read/write)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_DEBUGCONTROL 0x40090390
#define SIZE_R_FTDF_ON_OFF_REGMAP_DEBUGCONTROL 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_DEBUGCONTROL 0x1ff

/*
 *
 * Bit: ftdf_on_off_regmap_dbg_rx_input
 *
 * If set to '1', the Rx debug interface will be selected as input for the Rx pipeline rather than the DPHY interface signals.
 * Note that in this mode, DBG_RX_DATA[3:0] and DBG_RX_SOF are sourced by another source (outside the scope of the LMAC) while the other Rx inputs (CCA_STAT, LQI[7:0] and ED_STAT[7:0]) are forced to 0x00.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_DBG_RX_INPUT IND_R_FTDF_ON_OFF_REGMAP_DEBUGCONTROL
#define MSK_F_FTDF_ON_OFF_REGMAP_DBG_RX_INPUT 0x100
#define OFF_F_FTDF_ON_OFF_REGMAP_DBG_RX_INPUT 8
#define SIZE_F_FTDF_ON_OFF_REGMAP_DBG_RX_INPUT 1
/*
 *
 * Field: ftdf_on_off_regmap_dbg_control
 *
 * Control of the debug interface:
 *
 * dbg_control 0x00 Not used
 *              diagnose bus bit 7 - 0 = all '0'
 *
 * dbg_control 0x01 Legacy debug mode
 *              diagnose bus bit 7 = ed_request
 *              diagnose bus bit 6 = gen_irq
 *              diagnose bus bit 5 = tx_en
 *              diagnose bus bit 4 = rx_en
 *              diagnose bus bit 3 = phy_en
 *              diagnose bus bit 2 = tx_valid
 *              diagnose bus bit 1 = rx_sof
 *              diagnose bus bit 0 = rx_eof
 *
 * dbg_control 0x02 Clocks and reset
 *              diagnose bus bit 7-3 = "00000"
 *              diagnose bus bit 2   = lp_clk divide by 2
 *              diagnose bus bit 1   = mac_clk divide by 2
 *              diagnose bus bit 0   = hclk divided by 2
 *
 * dbg_control in range 0x02 - 0x0f Not used
 *              diagnose bus bit 7 - 0 = all '0'
 *
 * dbg_control in range 0x10 - 0x1f Debug rx pipeline
 *
 *      0x10    Rx phy signals
 *              diagnose_bus bit 7   = rx_enable towards the rx_pipeline
 *              diagnose bus bit 6   = rx_sof from phy
 *              diagnose bus bit 5-2 = rx_data from phy
 *              diagnose bus bit 1   = rx_eof to phy
 *              diagnose bus bit 0   = rx_sof event from rx_mac_timing block
 *
 *      0x11    Rx sof and length
 *              diagnose_bus bit 7   = rx_sof
 *              diagnose bus bit 6-0 = rx_mac_length from rx_mac_timing block
 *
 *      0x12    Rx mac timing output
 *              diagnose_bus bit 7   = Enable from rx_mac_timing block
 *              diagnose bus bit 6   = Sop    from rx_mac_timing block
 *              diagnose bus bit 5   = Eop    from rx_mac_timing block
 *              diagnose bus bit 4   = Crc    from rx_mac_timing block
 *              diagnose bus bit 3   = Err    from rx_mac_timing block
 *              diagnose bus bit 2   = Drop   from rx_mac_timing block
 *              diagnose bus bit 1   = Forward from rx_mac_timing block
 *              diagnose bus bit 0   = rx_sof
 *
 *      0x13    Rx mac frame parser output
 *              diagnose_bus bit 7   = Enable from mac_frame_parser block
 *              diagnose bus bit 6   = Sop    from  mac_frame_parser block
 *              diagnose bus bit 5   = Eop    from  mac_frame_parser block
 *              diagnose bus bit 4   = Crc    from  mac_frame_parser block
 *              diagnose bus bit 3   = Err    from  mac_frame_parser block
 *              diagnose bus bit 2   = Drop   from  mac_frame_parser block
 *              diagnose bus bit 1   = Forward from mac_frame_parser block
 *              diagnose bus bit 0   = rx_sof
 *
 *      0x14    Rx status to Umac
 *              diagnose_bus bit 7   = rx_frame_stat_umac.crc16_error
 *              diagnose bus bit 6   = rx_frame_stat_umac.res_frm_type_error
 *              diagnose bus bit 5   = rx_frame_stat_umac.res_frm_version_error
 *              diagnose bus bit 4   = rx_frame_stat_umac.dpanid_error
 *              diagnose bus bit 3   = rx_frame_stat_umac.daddr_error
 *              diagnose bus bit 2   = rx_frame_stat_umac.spanid_error
 *              diagnose bus bit 1   = rx_frame_stat_umac.ispan_coord_error
 *              diagnose bus bit 0   = rx_sof
 *
 *      0x15    Rx status to lmac
 *              diagnose_bus bit 7   = rx_frame_stat.packet_valid
 *              diagnose bus bit 6   = rx_frame_stat.rx_sof_e
 *              diagnose bus bit 5   = rx_frame_stat.rx_eof_e
 *              diagnose bus bit 4   = rx_frame_stat.wakeup_frame
 *              diagnose bus bit 3-1 = rx_frame_stat.frame_type
 *              diagnose bus bit 0   = rx_frame_stat.rx_sqnr_valid
 *
 *      0x16    Rx buffer control
 *              diagnose_bus bit 7-4 = prov_from_swio.rx_read_buf_ptr
 *              diagnose bus bit 3-0 = stat_to_swio.rx_write_buf_ptr
 *
 *      0x17    Rx interrupts
 *              diagnose_bus bit 7   = stat_to_swio.rx_buf_avail_e
 *              diagnose bus bit 6   = stat_to_swio.rx_buf_full
 *              diagnose bus bit 5   = stat_to_swio.rx_buf_overflow_e
 *              diagnose bus bit 4   = stat_to_swio.rx_sof_e
 *              diagnose bus bit 3   = stat_to_swio.rxbyte_e
 *              diagnose bus bit 2   = rx_frame_ctrl.rx_enable
 *              diagnose bus bit 1   = rx_eof
 *              diagnose bus bit 0   = rx_sof
 *
 *      0x18    diagnose_bus bit 7-3 =
 *                      Prt statements mac frame parser
 *                         0 Reset
 *                      1 Multipurpose frame with no dpanid and macImplicitBroadcast is false and not pan cooordinator dropped
 *                      2 Multipurpose frame with no dest address and macImplicitBroadcast is false and not pan cooordinator dropped
 *                      3 RX unsupported frame type detected
 *                      4 RX non Beacon frame detected and dropped in RxBeaconOnly mode
 *                      5 RX frame passed due to macAlwaysPassFrmType set
 *                      6 RX unsupported da_mode = 01 for frame_version 0x0- detected
 *                      7 RX unsupported sa_mode = 01 for frame_version 0x0- detected
 *                      8 Data or command frame with no dpanid and macImplicitBroadcast is false and not pan cooordinator dropped
 *                      9 Data or command frame with no dest address and macImplicitBroadcast is false and not pan cooordinator dropped
 *                      10 RX unsupported frame_version detected
 *                      11 Multipurpose frame with no dpanid and macImplicitBroadcast is false and not pan cooordinator dropped
 *                      12 Multipurpose frame with no dest address and macImplicitBroadcast is false and not pan cooordinator dropped
 *                      13 RX unsupported frame_version detected
 *                      14 RX Destination PAN Id check failed
 *                      15 RX Destination address (1 byte)  check failed
 *                      16 RX Destination address (2 byte)  check failed
 *                      17 RX Destination address (8 byte)  check failed
 *                      18 RX Source PAN Id check for Beacon frames failed
 *                      19 RX Source PAN Id check failed
 *                      20 Auxiliary Security Header security control word received
 *                      21 Auxiliary Security Header unsupported_legacy received frame_version 00
 *                      22 Auxiliary Security Header unsupported_legacy security frame_version 11
 *                      23 Auxiliary Security Header frame counter field received
 *                      24 Auxiliary Security Header Key identifier field received
 *                      25 Errored Destination  Wakeup frame found send to Umac, not indicated towards lmac controller
 *                      26 MacAlwaysPassWakeUpFrames = 1, Wakeup frame found send to Umac, not indicated towards lmac controller
 *                      27 Wakeup frame found send to lmac controller and Umac
 *                      28 Wakeup frame found send to lmac controller, dropped towards Umac
 *                      29 Command frame with data_request command received
 *                      30 Coordinator realignment frame received
 *                         31 Not Used
 *              diagnose bus bit 2-0 = frame_type
 *
 *      0x19    diagnose_bus bit 7-4 =
 *                      Framestates mac frame parser
 *                         0 idle_state
 *                      1 fr_ctrl_state
 *                      2 fr_ctrl_long_state
 *                      3 seq_nr_state
 *                      4 dest_pan_id_state
 *                      5 dest_addr_state
 *                      6 src_pan_id_state
 *                      7 src_addr_state
 *                      8 aux_sec_hdr_state
 *                      9 hdr_ie_state
 *                      10 pyld_ie_state
 *                      11 ignore_state
 *                      12-16  Not used
 *              diagnose bus bit 3   = rx_mac_sof_e
 *              diagnose bus bit 2-0 = frame_type
 *
 *      0x1a    diagnose_bus bit 7   = execute_sa_match
 *                 diagnose_bus bit 6   = sa_match_found (1 clk delayed)
 *                 diagnose_bus bit 5   = prov_from_swio.FP_override
 *                 diagnose_bus bit 4   = fp_mac_drdy
 *                 diagnose_bus bit 3   = fp_mac_ld_strb
 *                 diagnose_bus bit 2-1 = rx_frame_stat_lmac.rx_sa_mode
 *                 diagnose_bus bit 0   = rx_mac_sof_e
 *
 *      0x1b    diagnose_bus bit 7   = execute_sa_match
 *                 diagnose_bus bit 6-2 = fp_mac_addr_cnt
 *                 diagnose_bus bit 1   = fp_bit_decided
 *                 diagnose_bus bit 0   = prov_from_swio.Addr_tab_match_FP_value
 *
 *      0x1c    diagnose_bus bit 7   = execute_sa_match
 *                 diagnose_bus bit 6   = sa_match_found (1 clk delayed)
 *                 diagnose_bus bit 5   = fp_mac_rdata(68): Short_LongNot
 *                 diagnose_bus bit 4-1 = fp_mac_rdata(67 DOWNTO 64): SA valid bits
 *                 diagnose_bus bit 0   = fp_mac_rdata(0): least significant bit from FPPR
 *
 *      0x1d    diagnose_bus bit 7   = execute_sa_match
 *                 diagnose_bus bit 6   = sa_match_found (1 clk delayed)
 *                 diagnose_bus bit 5-0 = fp_mac_rdata(5 DOWNTO 0): 6 least significant bits from FPPR
 *
 *         0x1e-0x1f    Not Used
 *              diagnose bus bit 7-0 = all '0'
 *
 * dbg_control in range 0x20 - 0x2f Debug tx pipeline
 *      0x20    Top and bottom interfaces tx datapath
 *              diagnose_bus bit 7   = tx_mac_ld_strb
 *              diagnose bus bit 6   = tx_mac_drdy
 *              diagnose bus bit 5   = tx_valid
 *              diagnose bus bit 4-1 = tx_data
 *              diagnose bus bit 0   = symbol_ena
 *
 *      0x21    Control signals for tx datapath
 *              diagnose_bus bit 7   = TxTransparentMode
 *              diagnose bus bit 6   = ctrl_lmac_to_tx.CRC16_ena
 *              diagnose bus bit 5   = ctrl_lmac_to_tx.tx_start
 *              diagnose bus bit 4   = ctrl_lmac_to_tx.gen_ack
 *              diagnose bus bit 3   = ctrl_lmac_to_tx.gen_wu
 *              diagnose bus bit 2-1 = ctrl_lmac_to_tx.tx_frame_nmbr
 *              diagnose bus bit 0   = tx_valid
 *
 *      0x22    Control signals for wakeup frames
 *              diagnose_bus bit 7   = ctrl_lmac_to_tx.tx_start
 *              diagnose bus bit 6   = ctrl_lmac_to_tx.gen_wu
 *              diagnose bus bit 5-1 = ctrl_lmac_to_tx.gen_wu_RZ(4 DOWNTO 0)
 *              diagnose bus bit 0   = ctrl_tx_to_lmac.tx_rz_zero
 *
 *      0x23    Data signals for wakeup frame
 *              diagnose_bus bit 7   = ctrl_lmac_to_tx.tx_start
 *              diagnose bus bit 6   = ctrl_lmac_to_tx.gen_wu
 *              diagnose bus bit 5   = tx_valid
 *              diagnose bus bit 4-1 = tx_data
 *              diagnose bus bit 0   = ctrl_tx_to_lmac.tx_rz_zero
 *
 *      0x24    Data and control ack frame
 *              diagnose_bus bit 7   = ctrl_lmac_to_tx.tx_start
 *              diagnose bus bit 6   = ctrl_lmac_to_tx.gen_ack
 *              diagnose bus bit 5   = tx_valid
 *              diagnose bus bit 4-1 = tx_data
 *              diagnose bus bit 0   = symbol_ena
 *
 *      0x25    FP bit decision
 *              diagnose_bus bit 7   = fp_bit_decided
 *              diagnose bus bit 6   = execute_sa_match
 *              diagnose bus bit 5   = ctrl_lmac_to_tx.rx_fp_decided
 *              diagnose bus bit 4-3 = ctrl_lmac_to_tx.rx_sa_mode
 *              diagnose bus bit 2   = swio_to_lmac_tx.FP_override
 *              diagnose bus bit 1   = swio_to_lmac_tx.FP_force_value
 *              diagnose bus bit 0   = tx_valid
 *
 *      0x2a    Event counter, 8 least significant bits, synchronized to mac_clk
 *
 *         0x2b    symbol counter, 8 fore most least significant bits (15 downto 8)
 *
 *         0x2c    symbol counter, 8 least significant bits (7 downto 0)
 *
 *
 * dbg_control in range 0x2e - 0x3f Debug lmac_controller
 *      0x2d    Not Used
 *              diagnose bus bit 7-0 = all '0'
 *
 *         0x23    Phy_transaction last minute changes due to corner cases
 *                 diagnose_bus bit 7   = phy_transaction
 *                 diagnose_bus bit 6   = symbol_timer_preset
 *                 diagnose_bus bit 5   = symbol_timer_almost_ready
 *                 diagnose_bus bit 4   = csl_idle_rx
 *                 diagnose_bus bit 3   = csl_data_rx
 *                 diagnose_bus bit 2   = rx_enable_os_capt
 *                 diagnose_bus bit 1   = rx_enable_os_capt_done
 *                 diagnose_bus bit 0   = phy_en_ready
 *
 *         0x2f    Phy control in combination with arbiter state machine
 *                 diagnose_bus bit 7   = phy_transaction
 *                 diagnose_bus bit 6   = arb_phy_trans_res_store
 *                 diagnose_bus bit 5-0 = arb_debug_info
 *
 *      0x30    Phy signals
 *              diagnose_bus bit 7   = phy_en
 *              diagnose bus bit 6   = tx_en
 *              diagnose bus bit 5   = rx_en
 *              diagnose bus bit 4-1 = phyattr(3 downto 0)
 *              diagnose bus bit 0   = ed_request
 *
 *      0x31    Phy enable and detailed statemachine info
 *              diagnose_bus bit 7   = phy_en
 *              diagnose bus bit 6-0 = prt statements detailed statemachine info
 *                      0 Reset
 *                      1 MAIN_STATE_IDLE TX_ACKnowledgement frame skip CSMA_CA
 *                      2 MAIN_STATE_IDLE : Wake-Up frame resides in buffer no
 *                      3 MAIN_STATE_IDLE : TX frame in TSCH mode
 *                      4 MAIN_STATE_IDLE : TX frame
 *                      5 MAIN_STATE_IDLE : goto SINGLE_CCA
 *                      6 MAIN_STATE_IDLE : goto Energy Detection
 *                      7 MAIN_STATE_IDLE : goto RX
 *                      8 MAIN_STATE_IDLE de-assert phy_en wait for
 *                      9 MAIN_STATE_IDLE Start phy_en and assert phy_en wait for :
 *                      10 MAIN_STATE_CSMA_CA_WAIT_FOR_METADATA  framenr :
 *                      11 MAIN_STATE_CSMA_CA framenr :
 *                      12 MAIN_STATE_CSMA_CA start CSMA_CA
 *                      14 MAIN_STATE_CSMA_CA skip CSMA_CA
 *                      15 MAIN_STATE_WAIT_FOR_CSMA_CA csma_ca Failed for CSL mode
 *                      16 MAIN_STATE_WAIT_FOR_CSMA_CA csma_ca Failed
 *                      17 MAIN_STATE_WAIT_FOR_CSMA_CA csma_ca Success
 *                      18 MAIN_STATE_SINGLE_CCA cca failed
 *                      19 MAIN_STATE_SINGLE_CCA cca ok
 *                      20 MAIN_STATE_WAIT_ACK_DELAY wait time elapsed
 *                      21 MAIN_STATE_WAIT ACK_DELAY : Send Ack in TSCH mode
 *                      22 MAIN_STATE_WAIT ACK_DELAY : Ack not scheduled in time in TSCH mode
 *                      23 MAIN_STATE_TSCH_TX_FRAME macTSRxTx time elapsed
 *                      24 MAIN_STATE_TX_FRAME it is CSL mode, start the WU seq for macWUPeriod
 *                      25 MAIN_STATE_TX_FRAME1
 *                      26 MAIN_STATE_TX_FRAME_W1 exit, waited for :
 *                      27 MAIN_STATE_TX_FRAME_W2 exit, waited for :
 *                      28 MAIN_STATE_TX_WAIT_LAST_SYMBOL exit
 *                      29 MAIN_STATE_TX_RAMPDOWN_W exit, waited for :
 *                      30 MAIN_STATE_TX_PHYTRXWAIT exit, waited for :
 *                      31 MAIN_STATE_GEN_IFS , Ack with frame pending received
 *                      32 MAIN_STATE_GEN_IFS , Ack requested but DisRXackReceivedca is set
 *                      33 MAIN_STATE_GEN_IFS , instead of generating an IFS, switch the rx-on for an ACK
 *                      34 MAIN_STATE_GEN_IFS , generating an SIFS (Short Inter Frame Space)
 *                      35 MAIN_STATE_GEN_IFS , generating an LIFS (Long Inter Frame Space)
 *                      36 MAIN_STATE_GEN_IFS_FINISH in CSL mode, 'cslperiod_timer_ready' is ready sent the data frame
 *                      37 MAIN_STATE_GEN_IFS_FINISH in CSL mode, 'Rendezvous zero time not yet transmitted sent another WU frame
 *                      38 MAIN_STATE_GEN_IFS_FINISH exit, corrected for :
 *                      39 MAIN_STATE_GEN_IFS_FINISH in CSL mode, rx_resume_after_tx is set
 *                      40 MAIN_STATE_SENT_STATUS CSLMode wu_sequence_done_flag :
 *                      41 MAIN_STATE_ADD_DELAY exit, goto RX for frame pending time
 *                      42 MAIN_STATE_ADD_DELAY exit, goto IDLE
 *                      43 MAIN_STATE_RX switch the RX from off to on
 *                      44 MAIN_STATE_RX_WAIT_PHYRXSTART waited for
 *                      45 MAIN_STATE_RX_WAIT_PHYRXLATENCY start the MacAckDuration timer
 *                      46 MAIN_STATE_RX_WAIT_PHYRXLATENCY start the ack wait timer
 *                      47 MAIN_STATE_RX_WAIT_PHYRXLATENCY NORMAL mode : start rx_duration
 *                      48 MAIN_STATE_RX_WAIT_PHYRXLATENCY TSCH mode : start macTSRxWait (macTSRxWait) timer
 *                      49 MAIN_STATE_RX_WAIT_PHYRXLATENCY CSL mode : start the csl_rx_duration (macCSLsamplePeriod) timer
 *                      50 MAIN_STATE_RX_WAIT_PHYRXLATENCY CSL mode : start the csl_rx_duration (macCSLdataPeriod) timer
 *                      51 MAIN_STATE_RX_CHK_EXPIRED 1 ackwait_timer_ready or ack_received_ind
 *                      52 MAIN_STATE_RX_CHK_EXPIRED 2 normal_mode. ACK with FP bit set, start frame pending timer
 *                      54 MAIN_STATE_RX_CHK_EXPIRED 4 RxAlwaysOn is set to 0
 *                      55 MAIN_STATE_RX_CHK_EXPIRED 5 TSCH mode valid frame rxed
 *                      56 MAIN_STATE_RX_CHK_EXPIRED 6 TSCH mode macTSRxwait expired
 *                      57 MAIN_STATE_RX_CHK_EXPIRED 7 macCSLsamplePeriod timer ready
 *                      58 MAIN_STATE_RX_CHK_EXPIRED 8 csl_mode. Data frame recieved with FP bit set, increment RX duration time
 *                      59 MAIN_STATE_RX_CHK_EXPIRED 9 CSL mode : Received wu frame in data listening period restart macCSLdataPeriod timer
 *                      60 MAIN_STATE_RX_CHK_EXPIRED 10 csl_rx_duration_ready or csl data frame received:
 *                      61 MAIN_STATE_RX_CHK_EXPIRED 11 normal_mode. FP bit is set, start frame pending timer
 *                      62 MAIN_STATE_RX_CHK_EXPIRED 12 normal mode rx_duration_timer_ready and frame_pending_timer_ready
 *                      63 MAIN_STATE_RX_CHK_EXPIRED 13 acknowledge frame requested
 *                      64 MAIN_STATE_RX_CHK_EXPIRED 14 Interrupt to sent a frame. found_tx_packet
 *                      65 MAIN_STATE_CSL_RX_IDLE_END RZ time is far away, switch receiver Off and wait for csl_rx_rz_time before switching on
 *                      66 MAIN_STATE_CSL_RX_IDLE_END RZ time is not so far away, keep receiver On and wait for
 *                      67 MAIN_STATE_RX_START_PHYRXSTOP ack with fp received keep rx on
 *                      68 MAIN_STATE_RX_START_PHYRXSTOP switch the RX from on to off
 *                      69 MAIN_STATE_RX_WAIT_PHYRXSTOP acknowledge done : switch back to finish TX
 *                      70 MAIN_STATE_RX_WAIT_PHYRXSTOP switch the RX off
 *                      71 MAIN_STATE_ED switch the RX from off to on
 *                      72 MAIN_STATE_ED_WAIT_PHYRXSTART waited for phyRxStartup
 *                      73 MAIN_STATE_ED_WAIT_EDSCANDURATION waited for EdScanDuration
 *                      74 MAIN_STATE_ED_WAIT_PHYRXSTOP end of Energy Detection, got IDLE
 *                         75 MAIN_STATE_IDLE, use_legacy_phy_en = '0', rx_enable_os_capt detected and not yet 1 us 101 of PHY_EN done.
 *                         76 - 127 Not used
 *
 *      0x32    RX enable and detailed statemachine info
 *              diagnose_bus bit 7   = rx_en
 *              diagnose bus bit 6-0 = detailed statemachine info see dbg_control = 0x31
 *
 *      0x33    TX enable and detailed statemachine info
 *              diagnose_bus bit 7   = tx_en
 *              diagnose bus bit 6-0 = detailed statemachine info see dbg_control = 0x31
 *
 *      0x34    Phy enable, TX enable and Rx enable and state machine states
 *              diagnose_bus bit 7   = phy_en
 *              diagnose_bus bit 6   = tx_en
 *              diagnose_bus bit 5   = rx_en
 *              diagnose bus bit 4-0 = State machine states
 *                       1  MAIN_STATE_IDLE
 *                       2  MAIN_STATE_CSMA_CA_WAIT_FOR_METADATA
 *                       3  MAIN_STATE_CSMA_CA
 *                       4  MAIN_STATE_WAIT_FOR_CSMA_CA_0
 *                       5  MAIN_STATE_WAIT_FOR_CSMA_CA
 *                       6  MAIN_STATE_SINGLE_CCA
 *                       7  MAIN_STATE_WAIT_ACK_DELAY
 *                       8  MAIN_STATE_TSCH_TX_FRAME
 *                       9  MAIN_STATE_TX_FRAME
 *                       10 MAIN_STATE_TX_FRAME1
 *                       11 MAIN_STATE_TX_FRAME_W1
 *                       12 MAIN_STATE_TX_FRAME_W2
 *                       13 MAIN_STATE_TX_WAIT_LAST_SYMBOL
 *                       14 MAIN_STATE_TX_RAMPDOWN
 *                       15 MAIN_STATE_TX_RAMPDOWN_W
 *                       16 MAIN_STATE_TX_PHYTRXWAIT
 *                       17 MAIN_STATE_GEN_IFS
 *                       18 MAIN_STATE_GEN_IFS_FINISH
 *                       19 MAIN_STATE_SENT_STATUS
 *                       20 MAIN_STATE_ADD_DELAY
 *                       21 MAIN_STATE_TSCH_RX_ACKDELAY
 *                       22 MAIN_STATE_RX
 *                       23 MAIN_STATE_RX_WAIT_PHYRXSTART
 *                       24 MAIN_STATE_RX_WAIT_PHYRXLATENCY
 *                       25 MAIN_STATE_RX_CHK_EXPIRED
 *                       26 MAIN_STATE_CSL_RX_IDLE_END
 *                       27 MAIN_STATE_RX_START_PHYRXSTOP
 *                       28 MAIN_STATE_RX_WAIT_PHYRXSTOP
 *                       29 MAIN_STATE_ED
 *                       30 MAIN_STATE_ED_WAIT_PHYRXSTART
 *                       31 MAIN_STATE_ED_WAIT_EDSCANDURATION
 *
 *      0x35    lmac controller to tx interface with wakeup
 *              diagnose_bus bit 7   = tx_en
 *              diagnose_bus bit 6   = rx_en
 *              diagnose_bus bit 5   = ctrl_lmac_to_tx.gen_wu
 *              diagnose_bus bit 4   = ctrl_lmac_to_tx.gen_ack
 *              diagnose_bus bit 3-2 = ctrl_lmac_to_tx.tx_frame_nmbr
 *              diagnose_bus bit 1   = ctrl_lmac_to_tx.tx_start
 *              diagnose_bus bit 0   = ctrl_tx_to_lmac.tx_last_symbol
 *
 *      0x36    lmac controller to tx interface with frame pending
 *              diagnose_bus bit 7   = tx_en
 *              diagnose_bus bit 6   = rx_en
 *              diagnose_bus bit 5   = ctrl_lmac_to_tx.gen.fp_bit
 *              diagnose_bus bit 4   = ctrl_lmac_to_tx.gen_ack
 *              diagnose_bus bit 3-2 = ctrl_lmac_to_tx.tx_frame_nmbr
 *              diagnose_bus bit 1   = ctrl_lmac_to_tx.tx_start
 *              diagnose_bus bit 0   = ctrl_tx_to_lmac.tx_last_symbol
 *
 *      0x37    rx pipepline to lmac controller interface
 *              diagnose_bus bit 7   = tx_en
 *              diagnose_bus bit 6   = rx_en
 *              diagnose_bus bit 5   = ctrl_rx_to_lmac.rx_sof_e
 *              diagnose_bus bit 4   = ctrl_rx_to_lmac.rx_eof_e
 *              diagnose_bus bit 3   = ctrl_rx_to_lmac.packet_valid
 *              diagnose_bus bit 2   = ack_received_ind
 *              diagnose_bus bit 1   = ack_request_ind
 *              diagnose_bus bit 0   = ctrl_rx_to_lmac.data_request_frame
 *
 *      0x38    csl start
 *              diagnose_bus bit 7   = tx_en
 *              diagnose_bus bit 6   = rx_en
 *              diagnose_bus bit 5   = csl_start
 *              diagnose_bus bit 4-1 = swio_always_on_to_lmac_ctrl.tx_flag_stat
 *              diagnose_bus bit 0   = found_wu_packet;
 *
 *      0x39    Tsch TX ack timing
 *              diagnose_bus bit 7-4 = swio_always_on_to_lmac_ctrl.tx_flag_stat
 *              diagnose_bus bit 3   = tx_en
 *              diagnose_bus bit 2   = rx_en
 *              diagnose_bus bit 1   = ctrl_rx_to_lmac.packet_valid
 *              diagnose_bus bit 0   = one_us_timer_ready
 *
 *      0x3a    Phy control and Phy status
 *              diagnose_bus bit 7   = phy_transaction
 *              diagnose_bus bit 6   = phy_en
 *              diagnose_bus bit 5   = rx_en
 *              diagnose_bus bit 4   = tx_en
 *              diagnose_bus bit 3   = cca_stat
 *              diagnose bus bit 2   = ed_request
 *              diagnose bus bit 1   = rx_enable_os_capt
 *              diagnose bus bit 0   = found_tx_packet
 *
 *      0x3b    Phy control, incl old PHY_EN (to show the improvement using v2)
 *              diagnose_bus bit 7   = phy_transaction
 *              diagnose_bus bit 6   = phy_en
 *              diagnose_bus bit 5   = rx_en
 *              diagnose_bus bit 4   = tx_en
 *              diagnose_bus bit 3-0 = pti
 *
 *         0x3c    Control signals for arbiter control state machine
 *              diagnose_bus bit 7   = phy_transaction
 *              diagnose_bus bit 6   = arb_phy_trans_set_store
 *              diagnose_bus bit 5   = arb_phy_trans_clr_store
 *              diagnose_bus bit 4   = arb_phy_trans_res_store
 *              diagnose_bus bit 3   = found_tx_packet
 *              diagnose bus bit 2   = found_tx_packet_next
 *              diagnose bus bit 1   = found_wu_packet
 *              diagnose bus bit 0   = req_for_ack_flag
 *
 *      0x3d    Output of check which packet to send
 *              diagnose_bus bit 7   = phy_transaction
 *              diagnose_bus bit 6   = LmacReady4sleep
 *              diagnose_bus bit 5-4 = found_tx_number
 *              diagnose_bus bit 3-2 = found_tx_number_next
 *              diagnose bus bit 1-0 = found_wu_number
 *
 *         0x3e    CSMA_CA and timer control
 *              diagnose_bus bit 7   = phy_transaction
 *              diagnose_bus bit 6   = Csma_ca_resume_stat
 *              diagnose_bus bit 5   = SingleCCA_os_capt
 *              diagnose_bus bit 4   = backoff_timer_preset
 *              diagnose_bus bit 3   = backoff_timer_almost_ready
 *              diagnose bus bit 2   = backoff_timer_ready
 *              diagnose bus bit 1   = arb_timer_preset
 *              diagnose bus bit 0   = arb_timer_ready
 *
 *         0x3f    arbiter control state machine
 *              diagnose_bus bit 7   = phy_transaction
 *              diagnose_bus bit 6   = rx_en
 *              diagnose_bus bit 5   = arb_phy_trans_set_store
 *              diagnose_bus bit 4   = arb_phy_trans_clr_store
 *              diagnose_bus bit 3   = arb_phy_trans_res_store
 *              diagnose bus bit 2-0 = arb_state_info
 *
 *
 * dbg_control in range 0x40 - 0x4f Debug security
 *      0x40    Security control
 *              diagnose_bus bit 7   = swio_to_security.secStart
 *              diagnose_bus bit 6   = swio_to_security.secTxRxn
 *              diagnose_bus bit 5   = swio_to_security.secAbort
 *              diagnose_bus bit 4   = swio_to_security.secEncDecn
 *              diagnose_bus bit 3   = swio_from_security.secBusy
 *              diagnose_bus bit 2   = swio_from_security.secAuthFail
 *              diagnose_bus bit 1   = swio_from_security.secReady_e
 *              diagnose_bus bit 0   = '0'
 *
 *      0x41    Security ctlid access rx and tx
 *              diagnose_bus bit 7   = tx_sec_ci.ld_strb
 *              diagnose_bus bit 6   = tx_sec_ci.ctlid_rw
 *              diagnose_bus bit 5   = tx_sec_ci.selectp
 *              diagnose_bus bit 4   = tx_sec_co.d_rdy
 *              diagnose_bus bit 3   = rx_sec_ci.ld_strb
 *              diagnose_bus bit 2   = rx_sec_ci.ctlid_rw
 *              diagnose_bus bit 1   = rx_sec_ci.selectp
 *              diagnose_bus bit 0   = rx_sec_co.d_rdy
 *
 *      0x42    Security ctlid access tx with sec_wr_data_valid
 *              diagnose_bus bit 7   = tx_sec_ci.ld_strb
 *              diagnose_bus bit 6   = tx_sec_ci.ctlid_rw
 *              diagnose_bus bit 5   = tx_sec_ci.selectp
 *              diagnose_bus bit 4   = tx_sec_co.d_rdy
 *              diagnose_bus bit 3-0 = sec_wr_data_valid
 *
 *      0x43    Security ctlid access rx with sec_wr_data_valid
 *              diagnose_bus bit 7   = rx_sec_ci.ld_strb
 *              diagnose_bus bit 6   = rx_sec_ci.ctlid_rw
 *              diagnose_bus bit 5   = rx_sec_ci.selectp
 *              diagnose_bus bit 4   = rx_sec_co.d_rdy
 *              diagnose_bus bit 3-0 = sec_wr_data_valid
 *
 *      0x44-0x4f       Not used
 *              diagnose bus bit 7 - 0 = all '0'
 *
 * dbg_control in range 0x50 - 0x5f AHB bus
 *      0x50    ahb bus without hsize
 *              diagnose_bus bit 7   = hsel
 *              diagnose_bus bit 6-5 = htrans
 *              diagnose_bus bit 4   = hwrite
 *              diagnose_bus bit 3   = hready_in
 *              diagnose_bus bit 2   = hready_out
 *              diagnose_bus bit 1-0 = hresp
 *
 *      0x51    ahb bus without hwready_in and hresp(1)
 *              diagnose_bus bit 7   = hsel
 *              diagnose_bus bit 6-5 = htrans
 *              diagnose_bus bit 4   = hwrite
 *              diagnose_bus bit 3-2 = hhsize(1 downto 0)
 *              diagnose_bus bit 1   = hready_out
 *              diagnose_bus bit 0   = hresp(0)
 *
 *      0x52    Amba tx signals
 *              diagnose_bus bit 7   = ai_tx.penable
 *              diagnose_bus bit 6   = ai_tx.psel
 *              diagnose_bus bit 5   = ai_tx.pwrite
 *              diagnose_bus bit 4   = ao_tx.pready
 *              diagnose_bus bit 3   = ao_tx.pslverr
 *              diagnose_bus bit 2-1 = psize_tx
 *              diagnose_bus bit 0   = hready_out
 *
 *      0x53    Amba rx signals
 *              diagnose_bus bit 7   = ai_rx.penable
 *              diagnose_bus bit 6   = ai_rx.psel
 *              diagnose_bus bit 5   = ai_rx.pwrite
 *              diagnose_bus bit 4   = ao_rx.pready
 *              diagnose_bus bit 3   = ao_rx.pslverr
 *              diagnose_bus bit 2-1 = psize_rx
 *              diagnose_bus bit 0   = hready_out
 *
 *      0x54    Amba register map signals
 *              diagnose_bus bit 7   = ai_reg.penable
 *              diagnose_bus bit 6   = ai_reg.psel
 *              diagnose_bus bit 5   = ai_reg.pwrite
 *              diagnose_bus bit 4   = ao_reg.pready
 *              diagnose_bus bit 3   = ao_reg.pslverr
 *              diagnose_bus bit 2-1 = "00"
 *              diagnose_bus bit 0   = hready_out
 *
 *      0x55-0x5f       Not used
 *              diagnose bus bit 7 - 0 = all '0'
 */
#define IND_F_FTDF_ON_OFF_REGMAP_DBG_CONTROL IND_R_FTDF_ON_OFF_REGMAP_DEBUGCONTROL
#define MSK_F_FTDF_ON_OFF_REGMAP_DBG_CONTROL 0xff
#define OFF_F_FTDF_ON_OFF_REGMAP_DBG_CONTROL 0
#define SIZE_F_FTDF_ON_OFF_REGMAP_DBG_CONTROL 8

/*
 *
 * REGISTER: ftdf_on_off_regmap_txbyte_e
 *
 * Initialization value: 0x0  Initialization mask: 0x3
 * Access mode: E-RCW (event read/clear on write 1)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_TXBYTE_E 0x40090394
#define SIZE_R_FTDF_ON_OFF_REGMAP_TXBYTE_E 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_TXBYTE_E 0x3

/*
 *
 * Bit: ftdf_on_off_regmap_txbyte_e
 *
 * If set to '1', it indicates the first byte of a frame is transmitted
 * This event bit contributes to ftdf_ce[4].
 */
#define IND_F_FTDF_ON_OFF_REGMAP_TXBYTE_E IND_R_FTDF_ON_OFF_REGMAP_TXBYTE_E
#define MSK_F_FTDF_ON_OFF_REGMAP_TXBYTE_E 0x1
#define OFF_F_FTDF_ON_OFF_REGMAP_TXBYTE_E 0
#define SIZE_F_FTDF_ON_OFF_REGMAP_TXBYTE_E 1
/*
 *
 * Bit: ftdf_on_off_regmap_tx_last_symbol_e
 *
 * If set to '1', it indicates the last symbol of a frame is transmitted
 * This event bit contributes to ftdf_ce[4].
 */
#define IND_F_FTDF_ON_OFF_REGMAP_TX_LAST_SYMBOL_E IND_R_FTDF_ON_OFF_REGMAP_TXBYTE_E
#define MSK_F_FTDF_ON_OFF_REGMAP_TX_LAST_SYMBOL_E 0x2
#define OFF_F_FTDF_ON_OFF_REGMAP_TX_LAST_SYMBOL_E 1
#define SIZE_F_FTDF_ON_OFF_REGMAP_TX_LAST_SYMBOL_E 1

/*
 *
 * REGISTER: ftdf_on_off_regmap_txbyte_m
 *
 * Initialization value: 0x0  Initialization mask: 0x3
 * Access mode: EM-RW (event mask)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_TXBYTE_M 0x40090398
#define SIZE_R_FTDF_ON_OFF_REGMAP_TXBYTE_M 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_TXBYTE_M 0x3

/*
 *
 * Bit: ftdf_on_off_regmap_txbyte_m
 *
 * Mask bit for event "txbyte_e".
 * The mask bit is masking when cleared to '0' (default value) and will enable the contribution to the interrupt when set to '1'.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_TXBYTE_M IND_R_FTDF_ON_OFF_REGMAP_TXBYTE_M
#define MSK_F_FTDF_ON_OFF_REGMAP_TXBYTE_M 0x1
#define OFF_F_FTDF_ON_OFF_REGMAP_TXBYTE_M 0
#define SIZE_F_FTDF_ON_OFF_REGMAP_TXBYTE_M 1
/*
 *
 * Bit: ftdf_on_off_regmap_tx_last_symbol_m
 *
 * Mask bit for event "tx_last_symbol_e".
 * The mask bit is masking when cleared to '0' (default value) and will enable the contribution to the interrupt when set to '1'.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_TX_LAST_SYMBOL_M IND_R_FTDF_ON_OFF_REGMAP_TXBYTE_M
#define MSK_F_FTDF_ON_OFF_REGMAP_TX_LAST_SYMBOL_M 0x2
#define OFF_F_FTDF_ON_OFF_REGMAP_TX_LAST_SYMBOL_M 1
#define SIZE_F_FTDF_ON_OFF_REGMAP_TX_LAST_SYMBOL_M 1

/*
 *
 * REGISTER: ftdf_on_off_regmap_tx_flag_s
 *
 * Initialization value: none.
 * Access mode: S-R (state - read)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_TX_FLAG_S 0x40090400
#define SIZE_R_FTDF_ON_OFF_REGMAP_TX_FLAG_S 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_TX_FLAG_S 0x1
#define FTDF_ON_OFF_REGMAP_TX_FLAG_S_NUM 0x4
#define FTDF_ON_OFF_REGMAP_TX_FLAG_S_INTVL 0x20
#define FTDF_ON_OFF_REGMAP_TX_FLAG_STAT_NUM 0x4
#define FTDF_ON_OFF_REGMAP_TX_FLAG_STAT_INTVL 0x20

/*
 *
 * Bit: ftdf_on_off_regmap_tx_flag_stat
 *
 * Tx meta data per entry: if set to '1', the packet is ready for transmission
 */
#define IND_F_FTDF_ON_OFF_REGMAP_TX_FLAG_STAT IND_R_FTDF_ON_OFF_REGMAP_TX_FLAG_S
#define MSK_F_FTDF_ON_OFF_REGMAP_TX_FLAG_STAT 0x1
#define OFF_F_FTDF_ON_OFF_REGMAP_TX_FLAG_STAT 0
#define SIZE_F_FTDF_ON_OFF_REGMAP_TX_FLAG_STAT 1

/*
 *
 * REGISTER: ftdf_on_off_regmap_tx_flag_clear_e
 *
 * Initialization value: 0x0  Initialization mask: 0x1
 * Access mode: E-RCW (event read/clear on write 1)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_TX_FLAG_CLEAR_E 0x40090404
#define SIZE_R_FTDF_ON_OFF_REGMAP_TX_FLAG_CLEAR_E 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_TX_FLAG_CLEAR_E 0x1
#define FTDF_ON_OFF_REGMAP_TX_FLAG_CLEAR_E_NUM 0x4
#define FTDF_ON_OFF_REGMAP_TX_FLAG_CLEAR_E_INTVL 0x20
#define FTDF_ON_OFF_REGMAP_TX_FLAG_CLEAR_E_NUM 0x4
#define FTDF_ON_OFF_REGMAP_TX_FLAG_CLEAR_E_INTVL 0x20

/*
 *
 * Bit: ftdf_on_off_regmap_tx_flag_clear_e
 *
 * Tx meta data per entry: if set to '1' the LMAC hardware has cleared the tx_flag_stat status.
 * This event bit contributes to ftdf_ce[4].
 */
#define IND_F_FTDF_ON_OFF_REGMAP_TX_FLAG_CLEAR_E IND_R_FTDF_ON_OFF_REGMAP_TX_FLAG_CLEAR_E
#define MSK_F_FTDF_ON_OFF_REGMAP_TX_FLAG_CLEAR_E 0x1
#define OFF_F_FTDF_ON_OFF_REGMAP_TX_FLAG_CLEAR_E 0
#define SIZE_F_FTDF_ON_OFF_REGMAP_TX_FLAG_CLEAR_E 1

/*
 *
 * REGISTER: ftdf_on_off_regmap_tx_flag_clear_m
 *
 * Initialization value: 0x0  Initialization mask: 0x1
 * Access mode: EM-RW (event mask)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_TX_FLAG_CLEAR_M 0x40090408
#define SIZE_R_FTDF_ON_OFF_REGMAP_TX_FLAG_CLEAR_M 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_TX_FLAG_CLEAR_M 0x1
#define FTDF_ON_OFF_REGMAP_TX_FLAG_CLEAR_M_NUM 0x4
#define FTDF_ON_OFF_REGMAP_TX_FLAG_CLEAR_M_INTVL 0x20
#define FTDF_ON_OFF_REGMAP_TX_FLAG_CLEAR_M_NUM 0x4
#define FTDF_ON_OFF_REGMAP_TX_FLAG_CLEAR_M_INTVL 0x20

/*
 *
 * Bit: ftdf_on_off_regmap_tx_flag_clear_m
 *
 * Tx meta data per entry: Mask bit for event "tx_flag_clear_e".
 * The mask bit is masking when cleared to '0' (default value) and will enable the contribution to the interrupt when set to '1'.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_TX_FLAG_CLEAR_M IND_R_FTDF_ON_OFF_REGMAP_TX_FLAG_CLEAR_M
#define MSK_F_FTDF_ON_OFF_REGMAP_TX_FLAG_CLEAR_M 0x1
#define OFF_F_FTDF_ON_OFF_REGMAP_TX_FLAG_CLEAR_M 0
#define SIZE_F_FTDF_ON_OFF_REGMAP_TX_FLAG_CLEAR_M 1

/*
 *
 * REGISTER: ftdf_on_off_regmap_tx_priority
 *
 * Initialization value: 0x0  Initialization mask: 0xf1f
 * Access mode: C-RW (control - read/write)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_TX_PRIORITY 0x40090410
#define SIZE_R_FTDF_ON_OFF_REGMAP_TX_PRIORITY 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_TX_PRIORITY 0xf1f
#define FTDF_ON_OFF_REGMAP_TX_PRIORITY_NUM 0x4
#define FTDF_ON_OFF_REGMAP_TX_PRIORITY_INTVL 0x20
#define FTDF_ON_OFF_REGMAP_TX_PRIORITY_NUM 0x4
#define FTDF_ON_OFF_REGMAP_TX_PRIORITY_INTVL 0x20
#define FTDF_ON_OFF_REGMAP_ISWAKEUP_NUM 0x4
#define FTDF_ON_OFF_REGMAP_ISWAKEUP_INTVL 0x20
#define FTDF_ON_OFF_REGMAP_PTI_TX_NUM 0x4
#define FTDF_ON_OFF_REGMAP_PTI_TX_INTVL 0x20

/*
 *
 * Field: ftdf_on_off_regmap_tx_priority
 *
 * Tx meta data per entry: Priority of packet
 */
#define IND_F_FTDF_ON_OFF_REGMAP_TX_PRIORITY IND_R_FTDF_ON_OFF_REGMAP_TX_PRIORITY
#define MSK_F_FTDF_ON_OFF_REGMAP_TX_PRIORITY 0xf
#define OFF_F_FTDF_ON_OFF_REGMAP_TX_PRIORITY 0
#define SIZE_F_FTDF_ON_OFF_REGMAP_TX_PRIORITY 4
/*
 *
 * Bit: ftdf_on_off_regmap_IsWakeUp
 *
 * Tx meta data per entry: A basic wake-up frame can be generated by the UMAC in the Tx buffer.
 * The meta data control bit IsWakeUp must be set to indicate that this is a Wake-up frame.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_ISWAKEUP IND_R_FTDF_ON_OFF_REGMAP_TX_PRIORITY
#define MSK_F_FTDF_ON_OFF_REGMAP_ISWAKEUP 0x10
#define OFF_F_FTDF_ON_OFF_REGMAP_ISWAKEUP 4
#define SIZE_F_FTDF_ON_OFF_REGMAP_ISWAKEUP 1
/*
 *
 * Field: ftdf_on_off_regmap_pti_tx
 *
 * This register has 4 entries, belonging to the entry of the Tx frame to send, to be used during transmitting frames and the CMSA-CA phase before (when requested).
 * In TSCH mode this register shall be used during the time slot in which frames can be transmitted and consequently an Enhanced ACK can be received.
 * Since pti_tx belongs to a certain frame to be transmitted, pti_tx can be considered as extra Tx meta data.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_PTI_TX IND_R_FTDF_ON_OFF_REGMAP_TX_PRIORITY
#define MSK_F_FTDF_ON_OFF_REGMAP_PTI_TX 0xf00
#define OFF_F_FTDF_ON_OFF_REGMAP_PTI_TX 8
#define SIZE_F_FTDF_ON_OFF_REGMAP_PTI_TX 4

/*
 *
 * REGISTER: ftdf_on_off_regmap_tx_set_os
 *
 * Initialization value: 0x0  Initialization mask: 0xf
 * Access mode: OS-W (one-shot - write)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_TX_SET_OS 0x40090480
#define SIZE_R_FTDF_ON_OFF_REGMAP_TX_SET_OS 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_TX_SET_OS 0xf

/*
 *
 * Field: ftdf_on_off_regmap_tx_flag_set
 *
 * Tx meta data per entry: if set to '1', the tx_flag_stat will be set to '1'.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_TX_FLAG_SET IND_R_FTDF_ON_OFF_REGMAP_TX_SET_OS
#define MSK_F_FTDF_ON_OFF_REGMAP_TX_FLAG_SET 0xf
#define OFF_F_FTDF_ON_OFF_REGMAP_TX_FLAG_SET 0
#define SIZE_F_FTDF_ON_OFF_REGMAP_TX_FLAG_SET 4

/*
 *
 * REGISTER: ftdf_on_off_regmap_tx_clear_os
 *
 * Initialization value: 0x0  Initialization mask: 0xf
 * Access mode: OS-W (one-shot - write)
 */
#define IND_R_FTDF_ON_OFF_REGMAP_TX_CLEAR_OS 0x40090484
#define SIZE_R_FTDF_ON_OFF_REGMAP_TX_CLEAR_OS 0x4
#define MSK_R_FTDF_ON_OFF_REGMAP_TX_CLEAR_OS 0xf

/*
 *
 * Field: ftdf_on_off_regmap_tx_flag_clear
 *
 * Tx meta data per entry: if set to '1', the tx_flag_stat will be cleared to '0'.
 */
#define IND_F_FTDF_ON_OFF_REGMAP_TX_FLAG_CLEAR IND_R_FTDF_ON_OFF_REGMAP_TX_CLEAR_OS
#define MSK_F_FTDF_ON_OFF_REGMAP_TX_FLAG_CLEAR 0xf
#define OFF_F_FTDF_ON_OFF_REGMAP_TX_FLAG_CLEAR 0
#define SIZE_F_FTDF_ON_OFF_REGMAP_TX_FLAG_CLEAR 4

/*
 *
 * REGISTER: ftdf_always_on_regmap_wakeup_control
 *
 * Initialization value: 0x40000000  Initialization mask: 0xe1ffffff
 * Access mode: C-RW (control - read/write)
 */
#define IND_R_FTDF_ALWAYS_ON_REGMAP_WAKEUP_CONTROL 0x40091000
#define SIZE_R_FTDF_ALWAYS_ON_REGMAP_WAKEUP_CONTROL 0x4
#define MSK_R_FTDF_ALWAYS_ON_REGMAP_WAKEUP_CONTROL 0xe1ffffff

/*
 *
 * Field: ftdf_always_on_regmap_WakeUpIntThr
 *
 * Threshold for wake-up interrupt. When WakeUpEnable is set to '1' and the Wake-up (event) counter matches this value, the interrupt WAKEUP_IRQ is set to '1' for the duration of one LP_CLK period.
 */
#define IND_F_FTDF_ALWAYS_ON_REGMAP_WAKEUPINTTHR IND_R_FTDF_ALWAYS_ON_REGMAP_WAKEUP_CONTROL
#define MSK_F_FTDF_ALWAYS_ON_REGMAP_WAKEUPINTTHR 0x1ffffff
#define OFF_F_FTDF_ALWAYS_ON_REGMAP_WAKEUPINTTHR 0
#define SIZE_F_FTDF_ALWAYS_ON_REGMAP_WAKEUPINTTHR 25
/*
 *
 * Bit: ftdf_always_on_regmap_WakeUpEnable
 *
 * If set to '1', the WakeUpIntThr is enabled to generate an WAKEUP_IRQ interrupt.
 */
#define IND_F_FTDF_ALWAYS_ON_REGMAP_WAKEUPENABLE IND_R_FTDF_ALWAYS_ON_REGMAP_WAKEUP_CONTROL
#define MSK_F_FTDF_ALWAYS_ON_REGMAP_WAKEUPENABLE 0x20000000
#define OFF_F_FTDF_ALWAYS_ON_REGMAP_WAKEUPENABLE 29
#define SIZE_F_FTDF_ALWAYS_ON_REGMAP_WAKEUPENABLE 1
/*
 *
 * Field: ftdf_always_on_regmap_WakeUp_mode
 *
 * The Control register WakeUp_mode controls the behavior of the Event counter:
 * 0d = off, 1d = free running (default), 2d = one shot with auto clear, 3d = configurable period (timer
 * mode).
 */
#define IND_F_FTDF_ALWAYS_ON_REGMAP_WAKEUP_MODE IND_R_FTDF_ALWAYS_ON_REGMAP_WAKEUP_CONTROL
#define MSK_F_FTDF_ALWAYS_ON_REGMAP_WAKEUP_MODE 0xc0000000
#define OFF_F_FTDF_ALWAYS_ON_REGMAP_WAKEUP_MODE 30
#define SIZE_F_FTDF_ALWAYS_ON_REGMAP_WAKEUP_MODE 2

/*
 *
 * REGISTER: ftdf_fp_processing_ram_long_addr_0
 *
 * Initialization value: none.
 * Access mode: C-RW (control - read/write)
 */
#define IND_R_FTDF_FP_PROCESSING_RAM_LONG_ADDR_0 0x40092000
#define SIZE_R_FTDF_FP_PROCESSING_RAM_LONG_ADDR_0 0x4
#define MSK_R_FTDF_FP_PROCESSING_RAM_LONG_ADDR_0 0xffffffff
#define FTDF_FP_PROCESSING_RAM_LONG_ADDR_0_NUM 0x18
#define FTDF_FP_PROCESSING_RAM_LONG_ADDR_0_INTVL 0x10
#define FTDF_FP_PROCESSING_RAM_EXP_SA_L_NUM 0x18
#define FTDF_FP_PROCESSING_RAM_EXP_SA_L_INTVL 0x10

/*
 *
 * Field: ftdf_fp_processing_ram_Exp_SA_l
 *
 * Lowest part of the 64 bits long source address or SA entry 1 resp. SA entry 0 in case of short source address.
 */
#define IND_F_FTDF_FP_PROCESSING_RAM_EXP_SA_L IND_R_FTDF_FP_PROCESSING_RAM_LONG_ADDR_0
#define MSK_F_FTDF_FP_PROCESSING_RAM_EXP_SA_L 0xffffffff
#define OFF_F_FTDF_FP_PROCESSING_RAM_EXP_SA_L 0
#define SIZE_F_FTDF_FP_PROCESSING_RAM_EXP_SA_L 32

/*
 *
 * REGISTER: ftdf_fp_processing_ram_long_addr_1
 *
 * Initialization value: none.
 * Access mode: C-RW (control - read/write)
 */
#define IND_R_FTDF_FP_PROCESSING_RAM_LONG_ADDR_1 0x40092004
#define SIZE_R_FTDF_FP_PROCESSING_RAM_LONG_ADDR_1 0x4
#define MSK_R_FTDF_FP_PROCESSING_RAM_LONG_ADDR_1 0xffffffff
#define FTDF_FP_PROCESSING_RAM_LONG_ADDR_1_NUM 0x18
#define FTDF_FP_PROCESSING_RAM_LONG_ADDR_1_INTVL 0x10
#define FTDF_FP_PROCESSING_RAM_EXP_SA_H_NUM 0x18
#define FTDF_FP_PROCESSING_RAM_EXP_SA_H_INTVL 0x10

/*
 *
 * Field: ftdf_fp_processing_ram_Exp_SA_h
 *
 * Highest part of the 64 bits long source address or SA entry 3 resp. SA entry 2 in case of short source address.
 */
#define IND_F_FTDF_FP_PROCESSING_RAM_EXP_SA_H IND_R_FTDF_FP_PROCESSING_RAM_LONG_ADDR_1
#define MSK_F_FTDF_FP_PROCESSING_RAM_EXP_SA_H 0xffffffff
#define OFF_F_FTDF_FP_PROCESSING_RAM_EXP_SA_H 0
#define SIZE_F_FTDF_FP_PROCESSING_RAM_EXP_SA_H 32

/*
 *
 * REGISTER: ftdf_fp_processing_ram_size_and_val
 *
 * Initialization value: none.
 * Access mode: C-RW (control - read/write)
 */
#define IND_R_FTDF_FP_PROCESSING_RAM_SIZE_AND_VAL 0x40092008
#define SIZE_R_FTDF_FP_PROCESSING_RAM_SIZE_AND_VAL 0x4
#define MSK_R_FTDF_FP_PROCESSING_RAM_SIZE_AND_VAL 0x1f
#define FTDF_FP_PROCESSING_RAM_SIZE_AND_VAL_NUM 0x18
#define FTDF_FP_PROCESSING_RAM_SIZE_AND_VAL_INTVL 0x10
#define FTDF_FP_PROCESSING_RAM_VALID_SA_NUM 0x18
#define FTDF_FP_PROCESSING_RAM_VALID_SA_INTVL 0x10
#define FTDF_FP_PROCESSING_RAM_SHORT_LONGNOT_NUM 0x18
#define FTDF_FP_PROCESSING_RAM_SHORT_LONGNOT_INTVL 0x10

/*
 *
 * Field: ftdf_fp_processing_ram_Valid_SA
 *
 * Indication which SA entry is valid (if set). In case of 4 short SA Valid bit 3 belongs to SA entry 3 etc.
 * In case of a long SA Valid bit 0 is the valid indication.
 */
#define IND_F_FTDF_FP_PROCESSING_RAM_VALID_SA IND_R_FTDF_FP_PROCESSING_RAM_SIZE_AND_VAL
#define MSK_F_FTDF_FP_PROCESSING_RAM_VALID_SA 0xf
#define OFF_F_FTDF_FP_PROCESSING_RAM_VALID_SA 0
#define SIZE_F_FTDF_FP_PROCESSING_RAM_VALID_SA 4
/*
 *
 * Bit: ftdf_fp_processing_ram_Short_LongNot
 *
 * A '1' indicates that Exp_SA contains four short SA's, a '0' indicates one long SA.
 */
#define IND_F_FTDF_FP_PROCESSING_RAM_SHORT_LONGNOT IND_R_FTDF_FP_PROCESSING_RAM_SIZE_AND_VAL
#define MSK_F_FTDF_FP_PROCESSING_RAM_SHORT_LONGNOT 0x10
#define OFF_F_FTDF_FP_PROCESSING_RAM_SHORT_LONGNOT 4
#define SIZE_F_FTDF_FP_PROCESSING_RAM_SHORT_LONGNOT 1
#endif // dg_configBLACK_ORCA_IC_REV != BLACK_ORCA_IC_REV_A
#endif // _ftdf
