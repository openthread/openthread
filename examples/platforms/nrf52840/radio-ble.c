/*
 *  Copyright (c) 2019, The OpenThread Authors.
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

#if OPENTHREAD_ENABLE_BLE_CONTROLLER
#include "platform-nrf5.h"

#include <openthread/platform/cordio/radio-ble.h>

#include "common/logging.hpp"
#include "utils/code_utils.h"

#include <assert.h>
#include <string.h>

/* The size of PDU header*/
#define BLE_RADIO_PDU_HEADER_SZIE 2

/* Interframe spacing of BLE radio*/
#define BLE_RADIO_TIFS_US 150

/* Radio ramp-up times in usecs (fast mode) */
#define BLE_RADIO_RAMP_UP_US 40

/* preamble and access address transmit time, 8bits preamble + 32 bits access address*/
#define BLE_RADIO_PREAMBLE_ADDR_US 40

/* Guard time used to enable receiver in advance */
#define BLE_GUARD_TICKS 5

/* Rssi threshold used to filter out unwanted packets while debugging*/
#define BLE_RADIO_RSSI_FILTER_THRESHOLD -50

/* RTC0 xtal frequency is 32768Hz */
#define BLE_RADIO_US_TO_BB_TICKS(us) ((uint32_t)(((uint64_t)(us)*UINT64_C(549755)) >> 24))
#define BLE_RADIO_TICKS_TO_US(n) (uint32_t)(((uint64_t)(n)*15625) >> 9)

/* The number of BLE radio channels */
#define BLE_PHY_NUM_CHANS 40

/* To disable all radio interrupts */
#define NRF_RADIO_IRQ_MASK_ALL (0x34FF)

/*
 * We configure the nrf with a 1 byte S0 field, 8 bit length field, and
 * zero bit S1 field. The preamble is 8 bits long.
 */
#define NRF_LFLEN_BITS 8
#define NRF_PLEN_8BITS 0
#define NRF_S0LEN 1
#define NRF_S1LEN_BITS 0
#define NRF_CILEN_BITS 2
#define NRF_TERMLEN_BITS 3

/* Maximum length of frames */
#define NRF_MAXLEN 255
#define NRF_BALEN 3 /* For base address of 3 bytes */

/* NRF_RADIO->PCNF0 configuration values */
#define NRF_PCNF0                                                                          \
    (NRF_LFLEN_BITS << RADIO_PCNF0_LFLEN_Pos) | (NRF_PLEN_8BITS << RADIO_PCNF0_PLEN_Pos) | \
        (NRF_S0LEN << RADIO_PCNF0_S0LEN_Pos) | (NRF_S1LEN_BITS << RADIO_PCNF0_S1LEN_Pos)

#define NRF_PCNF1                                                                                  \
    (RADIO_PCNF1_ENDIAN_Little << RADIO_PCNF1_ENDIAN_Pos) | (NRF_BALEN << RADIO_PCNF1_BALEN_Pos) | \
        RADIO_PCNF1_WHITEEN_Msk;

/* delay between EVENTS_READY and start of tx */
#define BLE_PHY_TX_DELAY 4

/* delay between EVENTS_END and end of txd packet */
#define BLE_PHY_TX_END_DELAY 4

/* delay between rxd access address (w/ TERM1 for coded) and EVENTS_ADDRESS */
#define BLE_PHY_RX_ADDR_DELAY 6

/* delay between end of rxd packet and EVENTS_END */
#define BLE_PHY_RX_END_DELAY 6

/* RF center frequency for each channel index (offset from 2400 MHz) */
static const uint8_t cBleChannelFrequency[BLE_PHY_NUM_CHANS] = {
    4,  6,  8,  10, 12, 14, 16, 18, 20, 22, /* 0-9 */
    24, 28, 30, 32, 34, 36, 38, 40, 42, 44, /* 10-19 */
    46, 48, 50, 52, 54, 56, 58, 60, 62, 64, /* 20-29 */
    66, 68, 70, 72, 74, 76, 78, 2,  26, 80, /* 30-39 */
};

enum
{
    kPhyMaxPduLen = OT_RADIO_BLE_FRAME_MAX_SIZE,
};

static otRadioBleRxInfo sReceiveInfo;

uint32_t sRxBuffer[(kPhyMaxPduLen + 3) / 4];
uint32_t sTxBuffer[(kPhyMaxPduLen + 3) / 4];

bool sTifsEnabled = false;
bool sTxAtTifs    = false;
bool sRxAtTifs    = false;

static uint8_t  sState      = OT_BLE_RADIO_STATE_IDLE;
static int8_t   sTxPower    = 0;
static uint32_t sStartTicks = 0;
static uint32_t sRxEndTime  = 0;

static void radioInit(void);
static void ble_phy_disable(void);
static void ble_phy_rx_xcvr_setup(void);
static void ble_phy_tx_end_isr(void);
static void ble_phy_rx_end_isr(void);
static bool ble_phy_rx_start_isr(void);
static void ble_phy_rx_timeout_enable(uint32_t wfr_usecs, bool aTifsEnabled);

void nrf5BleRadioInit(void)
{
    radioInit();
    sState = OT_BLE_RADIO_STATE_DISABLED;
}

void radioInit(void)
{
    /* Enable HFCLK*/
    NRF_CLOCK->TASKS_HFCLKSTART = 1;

    /* Toggle peripheral power to reset (just in case) */
    NRF_RADIO->POWER = 0;
    NRF_RADIO->POWER = 1;

    /* Disable all interrupts */
    NRF_RADIO->INTENCLR = NRF_RADIO_IRQ_MASK_ALL;

    /* Set configuration registers */
    NRF_RADIO->MODE  = RADIO_MODE_MODE_Ble_1Mbit;
    NRF_RADIO->PCNF0 = NRF_PCNF0;
    NRF_RADIO->PCNF1 = NRF_MAXLEN | NRF_PCNF1;

    /* Enable radio fast ramp-up */
    NRF_RADIO->MODECNF0 |= (RADIO_MODECNF0_RU_Fast << RADIO_MODECNF0_RU_Pos) & RADIO_MODECNF0_RU_Msk;

    /* Set logical address 1 for TX and RX */
    NRF_RADIO->TXADDRESS   = 0;
    NRF_RADIO->RXADDRESSES = (1 << 0);

    /* Configure the CRC registers */
    NRF_RADIO->CRCCNF = (RADIO_CRCCNF_SKIPADDR_Skip << RADIO_CRCCNF_SKIPADDR_Pos) | RADIO_CRCCNF_LEN_Three;

    /* Configure BLE poly */
    NRF_RADIO->CRCPOLY = 0x0000065B;

    /* Configure IFS */
    NRF_RADIO->TIFS = BLE_RADIO_TIFS_US;

    /* Set isr priority and enable interrupt */
    NVIC_SetPriority(RADIO_IRQn, 0);
    NVIC_ClearPendingIRQ(RADIO_IRQn);
    NVIC_EnableIRQ(RADIO_IRQn);

    /* Setup RTC0 timer */
    NRF_RTC0->TASKS_STOP  = 1;
    NRF_RTC0->TASKS_CLEAR = 1;
    NRF_RTC0->PRESCALER   = 0; /* gives us 32768 Hz */
    NRF_RTC0->EVTENCLR    = RTC_EVTENSET_COMPARE0_Msk;
    NRF_RTC0->INTENCLR    = RTC_INTENSET_COMPARE0_Msk;
    NRF_RTC0->TASKS_START = 1;

    /* CH26: RADIO->EVENTS_ADDRESS --> TIMER0->TASKS_CAPTURE[1] */
    /* CH27: RADIO->EVENTS_END     --> TIMER0->TASKS_CAPTURE[2] */
    NRF_PPI->CHENSET = PPI_CHEN_CH26_Msk | PPI_CHEN_CH27_Msk;

    /* TIMER0 setup for PHY when using RTC */
    NRF_TIMER0->TASKS_STOP     = 1;
    NRF_TIMER0->TASKS_SHUTDOWN = 1;
    NRF_TIMER0->BITMODE        = 3; /* 32-bit timer */
    NRF_TIMER0->MODE           = 0; /* Timer mode */
    NRF_TIMER0->PRESCALER      = 4; /* gives us 1 MHz */

    NVIC_SetPriority(TIMER0_IRQN, 0);
    NVIC_ClearPendingIRQ(TIMER0_IRQN);
    NVIC_EnableIRQ(TIMER0_IRQN);

    /* CH4: RADIO->EVENTS_ADDRESS     --> TIMER0->TASKS_CAPTURE[3] */
    /* CH5: TIMER0->EVENTS_COMPARE[3] --> RADIO->TASKS_DISABLE */
    NRF_PPI->CH[4].EEP = (uint32_t) & (NRF_RADIO->EVENTS_ADDRESS);
    NRF_PPI->CH[4].TEP = (uint32_t) & (NRF_TIMER0->TASKS_CAPTURE[3]);
    NRF_PPI->CH[5].EEP = (uint32_t) & (NRF_TIMER0->EVENTS_COMPARE[3]);
    NRF_PPI->CH[5].TEP = (uint32_t) & (NRF_RADIO->TASKS_DISABLE);
}

void TIMER0_IRQ_HANDLER(void)
{
    uint32_t irq_en = NRF_TIMER0->INTENCLR;

    if ((irq_en & TIMER_INTENSET_COMPARE0_Msk) && NRF_TIMER0->EVENTS_COMPARE[0])
    {
        NRF_TIMER0->INTENCLR          = TIMER_INTENSET_COMPARE0_Msk;
        NRF_TIMER0->EVENTS_COMPARE[0] = 0;

        switch (sState)
        {
        case OT_BLE_RADIO_STATE_WAITING_TRANSMIT:
            sState = OT_BLE_RADIO_STATE_TRANSMIT;
            break;

        case OT_BLE_RADIO_STATE_WAITING_TRANSMIT_TIFS:
            if (sTxAtTifs)
            {
                sTxAtTifs = false;
                sState    = OT_BLE_RADIO_STATE_TRANSMIT;
            }
            else
            {
                NRF_PPI->CHENCLR = PPI_CHEN_CH21_Msk;
                sState           = OT_BLE_RADIO_STATE_IDLE;

                otPlatRadioBleTransmitDone(NULL, OT_BLE_RADIO_ERROR_FAILED);
            }

            break;

        case OT_BLE_RADIO_STATE_WAITING_RECEIVE:
            sState = OT_BLE_RADIO_STATE_RECEIVE;
            break;

        case OT_BLE_RADIO_STATE_WAITING_RECEIVE_TIFS:
            if (sRxAtTifs)
            {
                sRxAtTifs = false;
                sState    = OT_BLE_RADIO_STATE_RECEIVE;
            }
            else
            {
                NRF_PPI->CHENCLR = PPI_CHEN_CH20_Msk;
                sState           = OT_BLE_RADIO_STATE_IDLE;

                otPlatRadioBleReceiveDone(NULL, NULL, OT_BLE_RADIO_ERROR_FAILED);
            }
            break;

        default:
            break;
        }
    }
}

void BLE_RADIO_IRQ_HANDLER(void)
{
    uint32_t irq_en;

    /* Read irq register to determine which interrupts are enabled */
    irq_en = NRF_RADIO->INTENCLR;

    /*
     * NOTE: order of checking is important! Possible, if things get delayed,
     * we have both an ADDRESS and DISABLED interrupt in rx state. If we get
     * an address, we disable the DISABLED interrupt.
     */

    /* We get this if we have started to receive a frame */
    if ((irq_en & RADIO_INTENCLR_ADDRESS_Msk) && NRF_RADIO->EVENTS_ADDRESS)
    {
        /*
         * wfr timer is calculated to expire at the exact time we should start
         * receiving a packet (with 1 usec precision) so it is possible  it will
         * fire at the same time as EVENT_ADDRESS. If this happens, radio will
         * be disabled while we are waiting for EVENT_BCCMATCH after 1st byte
         * of payload is received and ble_phy_rx_start_isr() will fail. In this
         * case we should not clear DISABLED irq mask so it will be handled as
         * regular radio disabled event below. In other case radio was disabled
         * on purpose and there's nothing more to handle so we can clear mask.
         */

        if (ble_phy_rx_start_isr())
        {
            irq_en &= ~RADIO_INTENCLR_DISABLED_Msk;
            NRF_RADIO->EVENTS_DISABLED = 0;
        }
    }

    /* Check for disabled event. This only happens for transmits now */
    if ((irq_en & RADIO_INTENCLR_DISABLED_Msk) && NRF_RADIO->EVENTS_DISABLED)
    {
        if (sState == OT_BLE_RADIO_STATE_RECEIVE)
        {
            NRF_RADIO->EVENTS_DISABLED = 0;
            NRF_RADIO->EVENTS_ADDRESS  = 0;
            NRF_RADIO->INTENCLR        = RADIO_INTENCLR_DISABLED_Msk | RADIO_INTENCLR_ADDRESS_Msk;

            sState = OT_BLE_RADIO_STATE_IDLE;

            otPlatRadioBleReceiveDone(NULL, NULL, OT_BLE_RADIO_ERROR_RX_TIMEOUT);
        }
        else if (sState == OT_BLE_RADIO_STATE_TRANSMIT)
        {
            ble_phy_tx_end_isr();
        }
        else
        {
            assert(0);
        }
    }

    /* Receive packet end (we dont enable this for transmit) */
    if ((irq_en & RADIO_INTENCLR_END_Msk) && NRF_RADIO->EVENTS_END)
    {
        ble_phy_rx_end_isr();
    }

    /* Ensures IRQ is cleared */
    irq_en = NRF_RADIO->SHORTS;
}

static void ble_phy_tx_end_isr(void)
{
    uint32_t rx_time;
    uint32_t wfr_time;

    /* Clear events and clear interrupt on disabled event */
    NRF_RADIO->EVENTS_DISABLED = 0;
    NRF_RADIO->INTENCLR        = RADIO_INTENCLR_DISABLED_Msk;
    NRF_RADIO->EVENTS_END      = 0;

    wfr_time = NRF_RADIO->SHORTS;
    (void)wfr_time;

    if (sTifsEnabled)
    {
        /* Packet pointer needs to be reset. */
        ble_phy_rx_xcvr_setup();

        ble_phy_rx_timeout_enable(0, true);

        /* Schedule RX exactly T_IFS after TX end captured in CC[2] */
        rx_time = NRF_TIMER0->CC[2] + BLE_RADIO_TIFS_US;
        /* Adjust for delay between EVENT_END and actual TX end time */
        rx_time += BLE_PHY_TX_END_DELAY;
        /* Adjust for radio ramp-up */
        rx_time -= BLE_RADIO_RAMP_UP_US;
        /* Start listening a bit earlier due to allowed active clock accuracy */
        rx_time -= 2;

        NRF_TIMER0->CC[0]             = rx_time;
        NRF_TIMER0->EVENTS_COMPARE[0] = 0;

        // Enable Timer 0 interrupt
        NRF_TIMER0->INTENSET = TIMER_INTENSET_COMPARE0_Msk;

        /* CH21: TIMER0->EVENTS_COMPARE[0] --> RADIO->TASKS_RXEN */
        NRF_PPI->CHENSET = PPI_CHEN_CH21_Msk;

        // ReceiveAtTifs() should set NRF_PPI->CHENSET = PPI_CHEN_CH21_Msk
        // NRF_PPI->CHENCLR = PPI_CHEN_CH21_Msk;

        sState = OT_BLE_RADIO_STATE_WAITING_RECEIVE_TIFS;
    }
    else
    {
        /*
         * XXX: not sure we need to stop the timer here all the time. Or that
         * it should be stopped here.
         */
        NRF_TIMER0->TASKS_STOP     = 1;
        NRF_TIMER0->TASKS_SHUTDOWN = 1;

        NRF_PPI->CHENCLR = PPI_CHEN_CH4_Msk | PPI_CHEN_CH5_Msk | PPI_CHEN_CH20_Msk | PPI_CHEN_CH31_Msk;

        sState = OT_BLE_RADIO_STATE_IDLE;
    }

    otPlatRadioBleTransmitDone(NULL, OT_ERROR_NONE);
}

static bool ble_phy_rx_start_isr(void)
{
    uint32_t usecs;
    uint32_t pdu_usecs;
    uint32_t ticks;
    uint32_t remain_usec;

    /* Clear events and clear interrupt */
    NRF_RADIO->EVENTS_ADDRESS = 0;

    /* Clear wfr timer channels and DISABLED interrupt */
    NRF_RADIO->INTENCLR = RADIO_INTENCLR_DISABLED_Msk | RADIO_INTENCLR_ADDRESS_Msk;

    /* CH4: RADIO->EVENTS_ADDRESS     --> TIMER0->TASKS_CAPTURE[3] */
    /* CH5: TIMER0->EVENTS_CAPTURE[3] --> RADIO->TASKS_DISABLE */
    NRF_PPI->CHENCLR = PPI_CHEN_CH4_Msk | PPI_CHEN_CH5_Msk;

    /*
     * Calculate accurate packets start time (with remainder)
     *
     * We may start receiving packet somewhere during preamble in which case
     * it is possible that actual transmission started before TIMER0 was
     * running - need to take this into account.
     */

    usecs     = NRF_TIMER0->CC[1];
    pdu_usecs = BLE_RADIO_PREAMBLE_ADDR_US + BLE_PHY_RX_ADDR_DELAY;

    if (usecs < pdu_usecs)
    {
        sStartTicks--;
        usecs += 30;
    }

    usecs -= pdu_usecs;
    ticks = BLE_RADIO_US_TO_BB_TICKS(usecs);
    usecs -= BLE_RADIO_TICKS_TO_US(ticks);
    if (usecs == 31)
    {
        usecs = 0;
        ++ticks;
    }

    sReceiveInfo.mTicks = sStartTicks + ticks;
    remain_usec         = usecs;
    (void)(remain_usec);

    NRF_RADIO->INTENSET = RADIO_INTENSET_END_Msk;
    return true;
}

static void ble_phy_rx_end_isr(void)
{
    otRadioBleError error;
    uint8_t         crcok;
    uint32_t        tx_time;

    /* Clear events and clear interrupt */
    NRF_RADIO->EVENTS_END = 0;
    NRF_RADIO->INTENCLR   = RADIO_INTENCLR_END_Msk;

    /* Disable automatic RXEN */
    /* CH21: TIMER0->EVENTS_COMPARE[0] --> RADIO->TASKS_DISABLE */
    NRF_PPI->CHENCLR = PPI_CHEN_CH21_Msk;

    /* Set RSSI and CRC status flag in header */
    assert(NRF_RADIO->EVENTS_RSSIEND != 0);

    /* Count PHY crc errors and valid packets */
    crcok = NRF_RADIO->EVENTS_CRCOK;

    /*
     * Let's schedule TX now and we will just cancel it after processing RXed
     * packet if we don't need TX.
     *
     * We need this to initiate connection in case AUX_CONNECT_REQ was sent on
     * LE Coded S8. In this case the time we process RXed packet is roughly the
     * same as the limit when we need to have TX scheduled (i.e. TIMER0 and PPI
     * armed) so we may simply miss the slot and set the timer in the past.
     *
     * When TX is scheduled in advance, we may event process packet a bit longer
     * during radio ramp-up - this gives us extra 40 usecs which is more than
     * enough.
     */

    if (sTifsEnabled && crcok)
    {
        /* Schedule TX exactly T_IFS after RX end captured in CC[2] */
        tx_time = NRF_TIMER0->CC[2] + BLE_RADIO_TIFS_US;
        /* Adjust for delay between actual RX end time and EVENT_END */
        tx_time -= BLE_PHY_RX_END_DELAY;
        /* Adjust for radio ramp-up */
        tx_time -= BLE_RADIO_RAMP_UP_US;
        /* Adjust for delay between EVENT_READY and actual TX start time */
        tx_time -= BLE_PHY_TX_DELAY;

        NRF_TIMER0->CC[0]             = tx_time;
        NRF_TIMER0->EVENTS_COMPARE[0] = 0;

        /* Enable Timer 0 interrupt */
        NRF_TIMER0->INTENSET = TIMER_INTENSET_COMPARE0_Msk;

        /* CH20: TIMER0->EVENTS_COMPARE[0] --> RADIO->TASKS_TXEN */
        NRF_PPI->CHENSET = PPI_CHEN_CH20_Msk;

        // TransmitAtTifs() should set NRF_PPI->CHENSET = PPI_CHEN_CH20_Msk
        // NRF_PPI->CHENCLR = PPI_CHEN_CH20_Msk;

        sState = OT_BLE_RADIO_STATE_WAITING_TRANSMIT_TIFS;
    }
    else
    {
        ble_phy_disable();
        sState = OT_BLE_RADIO_STATE_IDLE;
    }

    sReceiveInfo.mRssi = (-1 * NRF_RADIO->RSSISAMPLE);
    if (crcok && (sReceiveInfo.mRssi > BLE_RADIO_RSSI_FILTER_THRESHOLD))
    {
        error = OT_BLE_RADIO_ERROR_NONE;
    }
    else
    {
        error = OT_BLE_RADIO_ERROR_CRC;
    }

    otPlatRadioBleReceiveDone(NULL, &sReceiveInfo, error);
}

/**
 * Setup transceiver for receive.
 */
static void ble_phy_rx_xcvr_setup(void)
{
    uint8_t *dptr = (uint8_t *)&sRxBuffer[0];

    NRF_RADIO->PACKETPTR = (uint32_t)dptr;

    /* Turn off trigger TXEN on output compare match and AAR on bcmatch */
    NRF_PPI->CHENCLR = PPI_CHEN_CH20_Msk | PPI_CHEN_CH23_Msk;

    NRF_RADIO->BCC             = 0;
    NRF_RADIO->EVENTS_ADDRESS  = 0;
    NRF_RADIO->EVENTS_DEVMATCH = 0;
    NRF_RADIO->EVENTS_BCMATCH  = 0;
    NRF_RADIO->EVENTS_RSSIEND  = 0;
    NRF_RADIO->EVENTS_CRCOK    = 0;
    NRF_RADIO->SHORTS          = RADIO_SHORTS_READY_START_Msk | RADIO_SHORTS_ADDRESS_RSSISTART_Msk |
                        RADIO_SHORTS_END_DISABLE_Msk | RADIO_SHORTS_DISABLED_RSSISTOP_Msk;

    NRF_RADIO->INTENSET = RADIO_INTENSET_ADDRESS_Msk;
}

/**
 * Called when we want to wait if the radio is in either the rx or tx
 * disable states. We want to wait until that state is over before doing
 * anything to the radio
 */
static void nrf_wait_disabled(void)
{
    uint32_t state;

    state = NRF_RADIO->STATE;
    if (state != RADIO_STATE_STATE_Disabled)
    {
        if ((state == RADIO_STATE_STATE_RxDisable) || (state == RADIO_STATE_STATE_TxDisable))
        {
            /* This will end within a short time (6 usecs). Just poll */
            while (NRF_RADIO->STATE == state)
            {
                /* If this fails, something is really wrong. Should last no more than 6 usecs */
            }
        }
    }
}

static int ble_phy_set_start_time(uint32_t cputime, uint8_t rem_usecs, bool tx)
{
    uint32_t next_cc;
    uint32_t cur_cc;
    uint32_t cntr;
    uint32_t delta;

    /*
     * We need to adjust start time to include radio ramp-up and TX pipeline
     * delay (the latter only if applicable, so only for TX).
     *
     * Radio ramp-up time is 40 usecs and TX delay is 3 or 5 usecs depending on
     * phy, thus we'll offset RTC by 2 full ticks (61 usecs) and then compensate
     * using TIMER0 with 1 usec precision.
     */

    cputime -= 2;
    rem_usecs += 61;

    if (tx)
    {
        rem_usecs -= BLE_RADIO_RAMP_UP_US;
        rem_usecs -= BLE_PHY_TX_DELAY;
    }
    else
    {
        rem_usecs -= BLE_RADIO_RAMP_UP_US;
    }

    /*
     * rem_usecs will be no more than 2 ticks, but if it is more than single
     * tick then we should better count one more low-power tick rather than
     * 30 high-power usecs. Also make sure we don't set TIMER0 CC to 0 as the
     * compare won't occur.
     */

    if (rem_usecs > 30)
    {
        cputime++;
        rem_usecs -= 30;
    }

    /*
     * Can we set the RTC compare to start TIMER0? We can do it if:
     *      a) Current compare value is not N+1 or N+2 ticks from current
     *      counter.
     *      b) The value we want to set is not at least N+2 from current
     *      counter.
     *
     * NOTE: since the counter can tick 1 while we do these calculations we
     * need to account for it.
     */
    next_cc = cputime & 0xffffff;
    cur_cc  = NRF_RTC0->CC[0];
    cntr    = NRF_RTC0->COUNTER;

    delta = (cur_cc - cntr) & 0xffffff;
    if ((delta <= 3) && (delta != 0))
    {
        return -1;
    }

    delta = (next_cc - cntr) & 0xffffff;
    if ((delta & 0x800000) || (delta < 3))
    {
        return -1;
    }

    /* Clear and set TIMER0 to fire off at proper time */
    NRF_TIMER0->TASKS_STOP        = 1;
    NRF_TIMER0->TASKS_CLEAR       = 1;
    NRF_TIMER0->CC[0]             = rem_usecs;
    NRF_TIMER0->EVENTS_COMPARE[0] = 0;

    /* Enable Timer 0 interrupt */
    NRF_TIMER0->INTENSET = TIMER_INTENSET_COMPARE0_Msk;

    /* Set RTC compare to start TIMER0 */
    NRF_RTC0->EVENTS_COMPARE[0] = 0;
    NRF_RTC0->CC[0]             = next_cc;
    NRF_RTC0->EVTENSET          = RTC_EVTENSET_COMPARE0_Msk;

    /* Enable PPI */
    /* CH31: RTC0->EVENTS_COMPARE[0] --> TIMER0->TASKS_START */
    NRF_PPI->CHENSET = PPI_CHEN_CH31_Msk;

    /* Store the cputime at which we set the RTC */
    sStartTicks = cputime;

    return 0;
}

static int ble_phy_set_start_now(void)
{
    uint32_t cntr;

    /* Read current RTC0 state */
    cntr = NRF_RTC0->COUNTER;

    /*
     * Set TIMER0 to fire immediately. We can't set CC to 0 as compare will not
     * occur in such case.
     */
    NRF_TIMER0->TASKS_STOP        = 1;
    NRF_TIMER0->TASKS_CLEAR       = 1;
    NRF_TIMER0->CC[0]             = 1;
    NRF_TIMER0->EVENTS_COMPARE[0] = 0;

    NVIC_ClearPendingIRQ(TIMER0_IRQN);

    /* Enable Timer 0 interrupt */
    NRF_TIMER0->INTENSET = TIMER_INTENSET_COMPARE0_Msk;

    /*
     * Set RTC compare to start TIMER0. We need to set it to at least N+2 ticks
     * from current value to guarantee triggering compare event, but let's set
     * it to N+3 to account for possible extra tick on RTC0 during these
     * operations.
     */
    NRF_RTC0->EVENTS_COMPARE[0] = 0;
    NRF_RTC0->CC[0]             = cntr + 3;
    NRF_RTC0->EVTENSET          = RTC_EVTENSET_COMPARE0_Msk;

    /* CH31: RTC0->EVENTS_COMPARE[0] --> TIMER0->TASKS_START */
    NRF_PPI->CHENSET = PPI_CHEN_CH31_Msk;

    /*
     * Store the cputime at which we set the RTC
     *
     * XXX Compare event may be triggered on previous CC value (if it was set to
     * less than N+2) so in rare cases actual start time may be 2 ticks earlier
     * than what we expect. Since this is only used on RX, it may cause AUX scan
     * to be scheduled 1 or 2 ticks too late so we'll miss it - it's acceptable
     * for now.
     */
    sStartTicks = cntr + 3;

    return 0;
}

static void ble_phy_rx_timeout_enable(uint32_t wfr_usecs, bool aTifsEnabled)
{
    uint32_t end_time;

    if (aTifsEnabled)
    {
        /* RX shall start exactly T_IFS after TX end captured in CC[2] */
        end_time = NRF_TIMER0->CC[2] + BLE_RADIO_TIFS_US;
        /* Adjust for delay between EVENT_END and actual TX end time */
        end_time += BLE_PHY_TX_END_DELAY;
        /* Wait a bit longer due to allowed active clock accuracy */
        end_time += 2;
        /*
         * It's possible that we'll capture PDU start time at the end of timer
         * cycle and since wfr expires at the beginning of calculated timer
         * cycle it can be almost 1 usec too early. Let's compensate for this
         * by waiting 1 usec more.
         */
        end_time += 1;
    }
    else
    {
        /*
         * RX shall start no later than wfr_usecs after RX enabled.
         * CC[0] is the time of RXEN so adjust for radio ram-up.
         * Do not add jitter since this is already covered by LL.
         */
        end_time = NRF_TIMER0->CC[0] + BLE_RADIO_RAMP_UP_US + wfr_usecs;
    }

    /*
     * Note: on LE Coded EVENT_ADDRESS is fired after TERM1 is received, so
     *       we are actually calculating relative to start of packet payload
     *       which is fine.
     */

    /* Adjust for receiving access address since this triggers EVENT_ADDRESS */
    end_time += BLE_RADIO_PREAMBLE_ADDR_US;
    /* Adjust for delay between actual access address RX and EVENT_ADDRESS */
    end_time += BLE_PHY_RX_ADDR_DELAY;

    /* wfr_secs is the time from rxen until timeout */
    NRF_TIMER0->CC[3]             = end_time;
    NRF_TIMER0->EVENTS_COMPARE[3] = 0;

    sRxEndTime = end_time;

    /* Enable wait for response PPI */
    /* CH4: RADIO->EVENTS_ADDRESS     --> TIMER0->TASKS_CAPTURE[3] */
    /* CH5: TIMER0->EVENTS_COMPARE[3] --> RADIO->TASKS_DISABLE */
    NRF_PPI->CHENSET = (PPI_CHEN_CH4_Msk | PPI_CHEN_CH5_Msk);

    /* Enable the disabled interrupt so we time out on events compare */
    NRF_RADIO->INTENSET = RADIO_INTENSET_DISABLED_Msk;

    /*
     * It may happen that if CPU is halted for a brief moment (e.g. during flash
     * erase or write), TIMER0 already counted past CC[3] and thus wfr will not
     * fire as expected. In case this happened, let's just disable PPIs for wfr
     * and trigger wfr manually (i.e. disable radio).
     *
     * Note that the same applies to RX start time set in CC[0] but since it
     * should fire earlier than wfr, fixing wfr is enough.
     *
     * CC[1] is only used as a reference on RX start, we do not need it here so
     * it can be used to read TIMER0 counter.
     */
    NRF_TIMER0->TASKS_CAPTURE[1] = 1;
    if (NRF_TIMER0->CC[1] > NRF_TIMER0->CC[3])
    {
        NRF_PPI->CHENCLR         = PPI_CHEN_CH4_Msk | PPI_CHEN_CH5_Msk;
        NRF_RADIO->TASKS_DISABLE = 1;
    }
}

int ble_phy_rx(void)
{
    otError error = OT_ERROR_NONE;

    /*
     * Check radio state.
     *
     * In case radio is now disabling we'll wait for it to finish, but if for
     * any reason it's just in idle state we proceed with RX as usual since
     * nRF52 radio can ramp-up from idle state as well.
     *
     * Note that TX and RX states values are the same except for 3rd bit so we
     * can make a shortcut here when checking for idle state.
     */
    nrf_wait_disabled();

    if ((NRF_RADIO->STATE != RADIO_STATE_STATE_Disabled) && ((NRF_RADIO->STATE & 0x07) != RADIO_STATE_STATE_RxIdle))
    {
        ble_phy_disable();
        error = OT_ERROR_INVALID_ARGS;
        otEXPECT(0);
    }

    /* Make sure all interrupts are disabled */
    NRF_RADIO->INTENCLR = NRF_RADIO_IRQ_MASK_ALL;

    /* Clear events prior to enabling receive */
    NRF_RADIO->EVENTS_END      = 0;
    NRF_RADIO->EVENTS_DISABLED = 0;

    /* Setup for rx */
    ble_phy_rx_xcvr_setup();

    /* PPI to start radio automatically shall be set here */
    /* CH21: TIMER0->EVENTS_COMPARE[0] --> RADIO->TASKS_RXEN */
    assert(NRF_PPI->CHEN & PPI_CHEN_CH21_Msk);

exit:
    return error;
}

otError ble_phy_rx_set_start_time(uint32_t cputime, uint8_t rem_usecs)
{
    /* CH20: TIMER0->EVENTS_COMPARE[0] --> RADIO->TASKS_TXEN */
    NRF_PPI->CHENCLR = PPI_CHEN_CH20_Msk;

    if (ble_phy_set_start_time(cputime, rem_usecs, false) != 0)
    {
        /* We're late so let's just try to start RX as soon as possible */
        ble_phy_set_start_now();
    }

    /* CH21: TIMER0->EVENTS_COMPARE[0] --> RADIO->TASKS_RXEN */
    NRF_PPI->CHENSET = PPI_CHEN_CH21_Msk;

    return ble_phy_rx();
}

void SetPacketPtr(uint8_t *aBuf, uint16_t aLength)
{
    NRF_RADIO->PACKETPTR = (uint32_t)aBuf;
    NRF_RADIO->PCNF1     = (aLength - BLE_RADIO_PDU_HEADER_SZIE) | NRF_PCNF1;
}

otError ble_phy_tx(void)
{
    otError  error;
    uint8_t *dptr = (uint8_t *)&sTxBuffer[0];
    uint32_t state;

    /*
     * This check is to make sure that the radio is not in a state where
     * it is moving to disabled state. If so, let it get there.
     */
    nrf_wait_disabled();

    /*
     * XXX: Although we may not have to do this here, I clear all the PPI
     * that should not be used when transmitting. Some of them are only enabled
     * if encryption and/or privacy is on, but I dont care. Better to be
     * paranoid, and if you are going to clear one, might as well clear them
     * all.
     */
    NRF_PPI->CHENCLR = PPI_CHEN_CH4_Msk | PPI_CHEN_CH5_Msk | PPI_CHEN_CH23_Msk | PPI_CHEN_CH25_Msk;

    NRF_RADIO->PACKETPTR = (uint32_t)dptr;

    /* Clear the ready, end and disabled events */
    NRF_RADIO->EVENTS_READY    = 0;
    NRF_RADIO->EVENTS_END      = 0;
    NRF_RADIO->EVENTS_DISABLED = 0;

    /* Enable shortcuts for transmit start/end. */
    NRF_RADIO->SHORTS   = RADIO_SHORTS_END_DISABLE_Msk | RADIO_SHORTS_READY_START_Msk;
    NRF_RADIO->INTENSET = RADIO_INTENSET_DISABLED_Msk;

    /* If we already started transmitting, abort it! */
    state = NRF_RADIO->STATE;
    if (state != RADIO_STATE_STATE_Tx)
    {
        /* Set phy state to transmitting and count packet statistics */
        error = OT_ERROR_NONE;
    }
    else
    {
        ble_phy_disable();
        error = OT_ERROR_INVALID_STATE;
    }

    return error;
}

otError ble_phy_tx_set_start_time(uint32_t cputime, uint8_t rem_usecs)
{
    otError error = OT_ERROR_NONE;

    /* CH21: TIMER0->EVENTS_COMPARE[0] --> RADIO->TASKS_RXEN */
    NRF_PPI->CHENCLR = PPI_CHEN_CH21_Msk;

    if (ble_phy_set_start_time(cputime, rem_usecs, true) != 0)
    {
        ble_phy_disable();
        error = OT_ERROR_FAILED;
    }
    else
    {
        /* CH20: TIMER0->EVENTS_COMPARE[0] --> RADIO->TASKS_TXEN */
        NRF_PPI->CHENSET = PPI_CHEN_CH20_Msk;

        error = ble_phy_tx();
    }

    return error;
}

/**
 * Stop the timer used to count microseconds when using RTC for cputime
 */
void ble_phy_stop_usec_timer(void)
{
    NRF_TIMER0->TASKS_STOP     = 1;
    NRF_TIMER0->TASKS_SHUTDOWN = 1;
    NRF_TIMER0->INTENCLR       = TIMER_INTENSET_COMPARE0_Msk;
    NRF_RTC0->EVTENCLR         = RTC_EVTENSET_COMPARE0_Msk;
}

/**
 * ble phy disable irq and ppi
 *
 * This routine is to be called when reception was stopped due to either a
 * wait for response timeout or a packet being received and the phy is to be
 * restarted in receive mode. Generally, the disable routine is called to stop
 * the phy.
 */
void ble_phy_disable_irq_and_ppi(void)
{
    NRF_RADIO->INTENCLR      = NRF_RADIO_IRQ_MASK_ALL;
    NRF_RADIO->SHORTS        = 0;
    NRF_RADIO->TASKS_DISABLE = 1;
    NRF_PPI->CHENCLR = PPI_CHEN_CH4_Msk | PPI_CHEN_CH5_Msk | PPI_CHEN_CH20_Msk | PPI_CHEN_CH21_Msk | PPI_CHEN_CH23_Msk |
                       PPI_CHEN_CH25_Msk | PPI_CHEN_CH31_Msk;
    NVIC_ClearPendingIRQ(RADIO_IRQn);
}

/**
 * ble phy disable
 *
 * Disables the PHY. This should be called when an event is over. It stops
 * the usec timer (if used), disables interrupts, disables the RADIO, disables
 * PPI and sets state to idle.
 */
void ble_phy_disable(void)
{
    ble_phy_stop_usec_timer();
    ble_phy_disable_irq_and_ppi();
}

static void ble_phy_apply_errata_102_106_107(void)
{
    /* [102] RADIO: PAYLOAD/END events delayed or not triggered after ADDRESS
     * [106] RADIO: Higher CRC error rates for some access addresses
     * [107] RADIO: Immediate address match for access addresses containing MSBs 0x00
     */
    *(volatile uint32_t *)0x40001774 = ((*(volatile uint32_t *)0x40001774) & 0xfffffffe) | 0x01000000;
}

void ble_phy_set_access_addr(uint32_t access_addr)
{
    NRF_RADIO->BASE0   = (access_addr << 8);
    NRF_RADIO->PREFIX0 = (NRF_RADIO->PREFIX0 & 0xFFFFFF00) | (access_addr >> 24);

    ble_phy_apply_errata_102_106_107();
}

void ble_phy_setchan(uint8_t chan, uint32_t access_addr, uint32_t crcinit)
{
    assert(chan < BLE_PHY_NUM_CHANS);

    ble_phy_set_access_addr(access_addr);

    NRF_RADIO->CRCINIT     = crcinit;
    NRF_RADIO->FREQUENCY   = cBleChannelFrequency[chan];
    NRF_RADIO->DATAWHITEIV = chan;
}

otError otPlatRadioBleSetChannelParameters(otInstance *aInstance, const otRadioBleChannelParams *aChannelParams)
{
    ble_phy_setchan(aChannelParams->mChannel, aChannelParams->mAccessAddress, aChannelParams->mCrcInit);

    return OT_ERROR_NONE;
}

otError SetRadioTxStartTime(const otRadioBleTime *aStartTime)
{
    return ble_phy_tx_set_start_time(aStartTime->mTicks, aStartTime->mOffsetUs);
}

otError otPlatRadioBleTransmitAtTime(otInstance *                aInstance,
                                     otRadioBleBufferDescriptor *aBufferDescriptors,
                                     uint8_t                     aNumBufferDescriptors,
                                     const otRadioBleTime *      aStartTime)
{
    otError  error = OT_ERROR_NONE;
    uint16_t offset;
    uint8_t  i;

    otEXPECT_ACTION((aBufferDescriptors != NULL) && (aStartTime != NULL), error = OT_ERROR_INVALID_ARGS);
    otEXPECT_ACTION(sState == OT_BLE_RADIO_STATE_IDLE, error = OT_ERROR_INVALID_STATE);
    otEXPECT((error = SetRadioTxStartTime(aStartTime)) == OT_ERROR_NONE);

    for (i = 0, offset = 0; i < aNumBufferDescriptors; i++)
    {
        memcpy((uint8_t *)sTxBuffer + offset, aBufferDescriptors[i].mBuffer, aBufferDescriptors[i].mLength);
        offset += aBufferDescriptors[i].mLength;
    }

    SetPacketPtr((uint8_t *)sTxBuffer, offset);

    sState = OT_BLE_RADIO_STATE_WAITING_TRANSMIT;

exit:
    return error;
}

otError otPlatRadioBleTransmitAtTifs(otInstance *                aInstance,
                                     otRadioBleBufferDescriptor *aBufferDescriptors,
                                     uint8_t                     aNumBufferDescriptors)
{
    otError  error = OT_ERROR_NONE;
    uint16_t offset;
    uint8_t  i;

    otEXPECT_ACTION(aBufferDescriptors != NULL, error = OT_ERROR_INVALID_ARGS);
    otEXPECT_ACTION(sState == OT_BLE_RADIO_STATE_WAITING_TRANSMIT_TIFS, error = OT_ERROR_INVALID_STATE);

    ble_phy_tx();

    for (i = 0, offset = 0; i < aNumBufferDescriptors; i++)
    {
        offset += aBufferDescriptors[i].mLength;
    }

    SetPacketPtr((uint8_t *)sTxBuffer, offset);

    for (i = 0, offset = 0; i < aNumBufferDescriptors; i++)
    {
        memcpy((uint8_t *)sTxBuffer + offset, aBufferDescriptors[i].mBuffer, aBufferDescriptors[i].mLength);
        offset += aBufferDescriptors[i].mLength;
    }

    sTxAtTifs = true;

    /* CH20: TIMER0->EVENTS_COMPARE[0] --> RADIO->TASKS_TXEN */
    // NRF_PPI->CHENSET = PPI_CHEN_CH20_Msk;

    OT_UNUSED_VARIABLE(aInstance);

exit:
    return error;
}

otError SetRadioRxStartTime(const otRadioBleTime *aStartTime)
{
    otError error = ble_phy_rx_set_start_time(aStartTime->mTicks - BLE_GUARD_TICKS, aStartTime->mOffsetUs);

    if (error == OT_ERROR_NONE)
    {
        ble_phy_rx_timeout_enable(aStartTime->mRxDurationUs + BLE_RADIO_TICKS_TO_US(BLE_GUARD_TICKS * 2), false);
    }

    return error;
}

otError otPlatRadioBleReceiveAtTime(otInstance *                aInstance,
                                    otRadioBleBufferDescriptor *aBufferDescriptor,
                                    const otRadioBleTime *      aStartTime)
{
    otError error = OT_ERROR_NONE;

    otEXPECT_ACTION((aBufferDescriptor != NULL) && (aStartTime != NULL), error = OT_ERROR_INVALID_ARGS);
    otEXPECT_ACTION(sState == OT_BLE_RADIO_STATE_IDLE, error = OT_ERROR_INVALID_STATE);

    otEXPECT((error = SetRadioRxStartTime(aStartTime)) == OT_ERROR_NONE);
    SetPacketPtr(aBufferDescriptor->mBuffer, aBufferDescriptor->mLength);

    sState = OT_BLE_RADIO_STATE_WAITING_RECEIVE;

exit:
    return error;
}

otError otPlatRadioBleReceiveAtTifs(otInstance *aInstance, otRadioBleBufferDescriptor *aBufferDescriptor)
{
    otError error = OT_ERROR_NONE;

    otEXPECT_ACTION(aBufferDescriptor != NULL, error = OT_ERROR_INVALID_ARGS);
    otEXPECT_ACTION(sState == OT_BLE_RADIO_STATE_WAITING_RECEIVE_TIFS, error = OT_ERROR_INVALID_STATE);

    SetPacketPtr(aBufferDescriptor->mBuffer, aBufferDescriptor->mLength);

    sRxAtTifs = true;

    /* CH21: TIMER0->EVENTS_COMPARE[0] --> RADIO->TASKS_RXEN */
    // NRF_PPI->CHENSET = PPI_CHEN_CH21_Msk;

    OT_UNUSED_VARIABLE(aInstance);

exit:
    return error;
}

otError otPlatRadioBleEnable(otInstance *aInstance)
{
    if (sState == OT_BLE_RADIO_STATE_DISABLED)
    {
        sState = OT_BLE_RADIO_STATE_IDLE;
    }

    OT_UNUSED_VARIABLE(aInstance);
    return OT_ERROR_NONE;
}

otError otPlatRadioBleDisable(otInstance *aInstance)
{
    if (sState != OT_BLE_RADIO_STATE_DISABLED)
    {
        ble_phy_disable();

        if ((sState == OT_BLE_RADIO_STATE_WAITING_TRANSMIT) || (sState == OT_BLE_RADIO_STATE_WAITING_TRANSMIT_TIFS))
        {
            otPlatRadioBleTransmitDone(NULL, OT_BLE_RADIO_ERROR_FAILED);
        }

        if ((sState == OT_BLE_RADIO_STATE_WAITING_RECEIVE) || (sState == OT_BLE_RADIO_STATE_WAITING_RECEIVE_TIFS))
        {
            otPlatRadioBleReceiveDone(NULL, NULL, OT_BLE_RADIO_ERROR_FAILED);
        }

        sState = OT_BLE_RADIO_STATE_DISABLED;
    }

    OT_UNUSED_VARIABLE(aInstance);
    return OT_ERROR_NONE;
}

void otPlatRadioBleEnableTifs(otInstance *aInstance)
{
    sTifsEnabled = true;
    OT_UNUSED_VARIABLE(aInstance);
}

void otPlatRadioBleDisableTifs(otInstance *aInstance)
{
    sTifsEnabled = false;

    OT_UNUSED_VARIABLE(aInstance);
}

void otPlatRadioBleCancelData(otInstance *aInstance)
{
    if ((sState == OT_BLE_RADIO_STATE_WAITING_TRANSMIT) || (sState == OT_BLE_RADIO_STATE_WAITING_RECEIVE))
    {
        ble_phy_disable();
        sState = OT_BLE_RADIO_STATE_IDLE;
    }

    OT_UNUSED_VARIABLE(aInstance);
}

void otPlatRadioBleCancelTifs(otInstance *aInstance)
{
    if ((sState == OT_BLE_RADIO_STATE_WAITING_RECEIVE_TIFS) || (sState == OT_BLE_RADIO_STATE_WAITING_TRANSMIT_TIFS))
    {
        ble_phy_disable();

        sTxAtTifs = false;
        sRxAtTifs = false;
        sState    = OT_BLE_RADIO_STATE_IDLE;
    }

    OT_UNUSED_VARIABLE(aInstance);
}

uint32_t otPlatRadioBleGetTickNow(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
    return NRF_RTC0->COUNTER;
}

int8_t ble_phy_txpower_round(int dbm)
{
    /* "Rail" power level if outside supported range */
    if (dbm >= (int8_t)RADIO_TXPOWER_TXPOWER_Pos4dBm)
    {
        return (int8_t)RADIO_TXPOWER_TXPOWER_Pos4dBm;
    }

    if (dbm >= (int8_t)RADIO_TXPOWER_TXPOWER_Pos3dBm)
    {
        return (int8_t)RADIO_TXPOWER_TXPOWER_Pos3dBm;
    }

    if (dbm >= (int8_t)RADIO_TXPOWER_TXPOWER_0dBm)
    {
        return (int8_t)RADIO_TXPOWER_TXPOWER_0dBm;
    }

    if (dbm >= (int8_t)RADIO_TXPOWER_TXPOWER_Neg4dBm)
    {
        return (int8_t)RADIO_TXPOWER_TXPOWER_Neg4dBm;
    }

    if (dbm >= (int8_t)RADIO_TXPOWER_TXPOWER_Neg8dBm)
    {
        return (int8_t)RADIO_TXPOWER_TXPOWER_Neg8dBm;
    }

    if (dbm >= (int8_t)RADIO_TXPOWER_TXPOWER_Neg12dBm)
    {
        return (int8_t)RADIO_TXPOWER_TXPOWER_Neg12dBm;
    }

    if (dbm >= (int8_t)RADIO_TXPOWER_TXPOWER_Neg20dBm)
    {
        return (int8_t)RADIO_TXPOWER_TXPOWER_Neg20dBm;
    }

    return (int8_t)RADIO_TXPOWER_TXPOWER_Neg40dBm;
}

int8_t otPlatRadioBleGetTransmitPower(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
    return sTxPower;
}

otError otPlatRadioBleSetTransmitPower(otInstance *aInstance, int8_t aPower)
{
    OT_UNUSED_VARIABLE(aInstance);

    sTxPower           = ble_phy_txpower_round(aPower);
    NRF_RADIO->TXPOWER = sTxPower;
    return OT_ERROR_NONE;
}

uint8_t otPlatRadioBleGetXtalAccuracy(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
    return 20;
}

void otPlatRadioBleGetPublicAddress(otInstance *aInstance, otPlatBleDeviceAddr *aAddress)
{
    memset(aAddress, 0, sizeof(otPlatBleDeviceAddr));
    aAddress->mAddrType            = OT_BLE_ADDRESS_TYPE_PUBLIC;
    *(uint64_t *)(aAddress->mAddr) = NRF_FICR->DEVICEID[1] | ((uint64_t)NRF_FICR->DEVICEID[0]) << 32;

    OT_UNUSED_VARIABLE(aInstance);
    return;
}

void otPlatRadioBleEnableInterrupt(void)
{
    NVIC_EnableIRQ(RADIO_IRQn);
}

void otPlatRadioBleDisableInterrupt(void)
{
    NVIC_DisableIRQ(RADIO_IRQn);
}

OT_TOOL_WEAK void otPlatRadioBleTransmitDone(otInstance *aInstance, otRadioBleError aError)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aError);
}

OT_TOOL_WEAK void otPlatRadioBleReceiveDone(otInstance *aInstance, otRadioBleRxInfo *aRxInfo, otRadioBleError aError)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aRxInfo);
    OT_UNUSED_VARIABLE(aError);
}

#endif // OPENTHREAD_ENABLE_BLE_CONTROLLER
