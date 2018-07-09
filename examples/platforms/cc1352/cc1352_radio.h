/*
 *  Copyright (c) 2018, The OpenThread Authors.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. Neither the name of the copyright holder nor the
 *     names of its contributors may be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef CC1352_RADIO_H_
#define CC1352_RADIO_H_

#include <driverlib/rf_ieee_cmd.h>

enum
{
    IEEE802154_FRAME_TYPE_MASK        = 0x7,     ///< (IEEE 802.15.4-2006) PSDU.FCF.frameType
    IEEE802154_FRAME_TYPE_ACK         = 0x2,     ///< (IEEE 802.15.4-2006) frame type: ACK
    IEEE802154_ACK_REQUEST            = (1<<5),  ///< (IEEE 802.15.4-2006) PSDU.FCF.bAR
    IEEE802154_DSN_OFFSET             = 2,       ///< (IEEE 802.15.4-2006) PSDU.sequenceNumber
    IEEE802154_MAC_MIN_BE             = 1,       ///< (IEEE 802.15.4-2006) macMinBE
    IEEE802154_MAC_MAX_BE             = 5,       ///< (IEEE 802.15.4-2006) macMaxBE
    IEEE802154_MAC_MAX_CSMA_BACKOFFS  = 4,       ///< (IEEE 802.15.4-2006) macMaxCSMABackoffs
    IEEE802154_MAC_MAX_FRAMES_RETRIES = 3,       ///< (IEEE 802.15.4-2006) macMaxFrameRetries
    IEEE802154_A_UINT_BACKOFF_PERIOD  = 20,      ///< (IEEE 802.15.4-2006 7.4.1) MAC constants
    IEEE802154_A_TURNAROUND_TIME      = 12,      ///< (IEEE 802.15.4-2006 6.4.1) PHY constants
    IEEE802154_PHY_SHR_DURATION       = 10,
    ///< (IEEE 802.15.4-2006 6.4.2) PHY PIB attribute, specifically the O-QPSK PHY
    IEEE802154_PHY_SYMBOLS_PER_OCTET  = 2,
    ///< (IEEE 802.15.4-2006 6.4.2) PHY PIB attribute, specifically the O-QPSK PHY
    IEEE802154_MAC_ACK_WAIT_DURATION  = (IEEE802154_A_UINT_BACKOFF_PERIOD +
                                         IEEE802154_A_TURNAROUND_TIME     +
                                         IEEE802154_PHY_SHR_DURATION      +
                                         ( 6 * IEEE802154_PHY_SYMBOLS_PER_OCTET)),
    ///< (IEEE 802.15.4-2006 7.4.2) macAckWaitDuration PIB attribute
    IEEE802154_SYMBOLS_PER_SEC        = 62500    ///< (IEEE 802.15.4-2006 6.5.3.2) O-QPSK symbol rate
};

enum
{
    CC1352_RAT_TICKS_PER_SEC          = 4000000, ///< 4MHz clock
    CC1352_INVALID_RSSI               = 127,
    CC1352_UNKNOWN_EUI64              = 0xFF,
    ///< If the EUI64 read from the ccfg is all ones then the customer did not set the address
};

/**
 * TX Power dBm lookup table - values from SmartRF Studio
 */
typedef struct output_config
{
    int      dbm;
    uint16_t value;
} output_config_t;

/**
 *  * TX Power dBm lookup table from SmartRF Studio 7 2.10.0#94
 */
static const output_config_t rgOutputPower[] =
{
    {   5, 0x941E},
    {   4, 0x6c16},
    {   3, 0x5411},
    {   2, 0x440d},
    {   1, 0x385c},
    {   0, 0x3459},
    {  -3, 0x2851},
    {  -5, 0x224e},
    {  -6, 0x204d},
    {  -9, 0x0a8d},
    { -10, 0x168c},
    { -12, 0x108a},
    { -15, 0xc88c},
    { -18, 0x06c9},
    { -21, 0x06c7},
};

#define OUTPUT_CONFIG_COUNT (sizeof(rgOutputPower) / sizeof(rgOutputPower[0]))

/* Max and Min Output Power in dBm */
#define OUTPUT_POWER_MIN     (rgOutputPower[OUTPUT_CONFIG_COUNT - 1].dbm)
#define OUTPUT_POWER_MAX     (rgOutputPower[0].dbm)
#define OUTPUT_POWER_UNKNOWN 0xFFFF

/**
 * return value used when searching the source match array
 */
#define CC1352_SRC_MATCH_NONE 0xFF

/**
 * number of extended addresses used for source matching
 */
#define CC1352_EXTADD_SRC_MATCH_NUM 10

/**
 * structure for source matching extended addresses
 */
typedef struct __attribute__((aligned(4))) ext_src_match_data
{
    uint32_t srcMatchEn[((CC1352_EXTADD_SRC_MATCH_NUM + 31) / 32)];
    uint32_t srcPendEn[((CC1352_EXTADD_SRC_MATCH_NUM + 31) / 32)];
    uint64_t extAddrEnt[CC1352_EXTADD_SRC_MATCH_NUM];
} ext_src_match_data_t;

/**
 * number of short addresses used for source matching
 */
#define CC1352_SHORTADD_SRC_MATCH_NUM 10

/**
 * structure for source matching short addresses
 */
typedef struct __attribute__((aligned(4))) short_src_match_data
{
    uint32_t srcMatchEn[((CC1352_SHORTADD_SRC_MATCH_NUM + 31) / 32)];
    uint32_t srcPendEn[((CC1352_SHORTADD_SRC_MATCH_NUM + 31) / 32)];
    rfc_shortAddrEntry_t extAddrEnt[CC1352_SHORTADD_SRC_MATCH_NUM];
} short_src_match_data_t;

/**
 * size of length field in receive struct
 *
 * defined in Table 23-10 of the cc26xx TRM
 */
#define DATA_ENTRY_LENSZ_BYTE 1

/**
 * address type for @ref rfCoreModifySourceMatchEntry()
 */
typedef enum cc1352_address
{
    SHORT_ADDRESS = 1,
    EXT_ADDRESS   = 0,
} cc1352_address_t;

/**
 * This enum represents the state of a radio.
 * Initially, a radio is in the Disabled state.
 *
 * The following are valid radio state transitions for the cc1352:
 *
 *                                    (Radio ON)
 *  +----------+  Enable()  +-------+  Receive()   +---------+   Transmit()   +----------+
 *  |          |----------->|       |------------->|         |--------------->|          |
 *  | Disabled |            | Sleep |              | Receive |                | Transmit |
 *  |          |<-----------|       |<-------------|         |<---------------|          |
 *  +----------+  Disable() |       |   Sleep()    |         | AckFrame RX or +----------+
 *                          |       | (Radio OFF)  +---------+ sTxCmdChainDone == true
 *                          |       |
 *                          |       | EnergyScan() +--------+
 *                          |       |------------->|        |
 *                          |       |              | EdScan |
 *                          |       |<-------------|        |
 *                          |       |  signal ED   |        |
 *                          +-------+  scan done   +--------+
 *
 * These states slightly differ from the states in \ref include/platform/radio.h.
 * The additional states the phy can be in are due to the asynchronous nature
 * of the CM0 radio core.
 *
 * | state            | description                                        |
 * |------------------|----------------------------------------------------|
 * | Disabled         | The rfcore powerdomain is off and the RFCPE is off |
 * | Sleep            | The RFCORE PD is on, and the RFCPE is in IEEE mode |
 * | Receive          | The RFCPE is running a CMD_IEEE_RX                 |
 * | Transmit         | The RFCPE is running a transmit command string     |
 * | TransmitComplete | The transmit command string has completed          |
 * | EdScan           | The RFCPE is running a CMD_IEEE_ED_SCAN            |
 *
 * \note The RAT start and Radio Setup commands may be moved to the Receive()
 *       and EnergyScan() transitions in the future.
 */
typedef enum cc1352_PhyState
{
    cc1352_stateDisabled = 0,
    cc1352_stateSleep,
    cc1352_stateReceive,
    cc1352_stateEdScan,
    cc1352_stateTransmit,
} cc1352_PhyState_t;

#endif /* CC1352_RADIO_H_ */
