/**
 * Copyright (c) 2016 - 2018, Nordic Semiconductor ASA
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form, except as embedded into a Nordic
 *    Semiconductor ASA integrated circuit in a product or a software update for
 *    such product, must reproduce the above copyright notice, this list of
 *    conditions and the following disclaimer in the documentation and/or other
 *    materials provided with the distribution.
 *
 * 3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * 4. This software, with or without modification, must only be used with a
 *    Nordic Semiconductor ASA integrated circuit.
 *
 * 5. Any software provided in binary form under this license must not be reverse
 *    engineered, decompiled, modified and/or disassembled.
 *
 * THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
#include "sdk_common.h"
#if NRF_MODULE_ENABLED(USBD)

#include "nrf_drv_usbd.h"
#include "nrf.h"
#include "nrf_atomic.h"
#include "app_util_platform.h"
#include "nrf_delay.h"

#include <stdbool.h>
#include <string.h>
#include <inttypes.h>

#define NRF_LOG_MODULE_NAME USBD

#if USBD_CONFIG_LOG_ENABLED
#define NRF_LOG_LEVEL       USBD_CONFIG_LOG_LEVEL
#define NRF_LOG_INFO_COLOR  USBD_CONFIG_INFO_COLOR
#define NRF_LOG_DEBUG_COLOR USBD_CONFIG_DEBUG_COLOR
#else //USBD_CONFIG_LOG_ENABLED
#define NRF_LOG_LEVEL       0
#endif //USBD_CONFIG_LOG_ENABLED
#include "nrf_log.h"
NRF_LOG_MODULE_REGISTER();

#ifndef NRF_DRV_USBD_EARLY_DMA_PROCESS
/* Try to process DMA request when endpoint transmission has been detected
 * and just after last EasyDMA has been processed.
 * It speeds up the transmission a little (about 10% measured)
 * with a cost of more CPU power used.
 */
#define NRF_DRV_USBD_EARLY_DMA_PROCESS 1
#endif

#ifndef NRF_DRV_USBD_PROTO1_FIX_DEBUG
/* Debug information when events are fixed*/
#define NRF_DRV_USBD_PROTO1_FIX_DEBUG 1
#endif

#define NRF_DRV_USBD_LOG_PROTO1_FIX_PRINTF(...)                          \
    do{                                                                  \
        if (NRF_DRV_USBD_PROTO1_FIX_DEBUG){ NRF_LOG_DEBUG(__VA_ARGS__); }\
    } while (0)

#ifndef NRF_DRV_USBD_STARTED_EV_ENABLE
#define NRF_DRV_USBD_STARTED_EV_ENABLE    0
#endif

#ifndef USBD_CONFIG_ISO_IN_ZLP
#define USBD_CONFIG_ISO_IN_ZLP  0
#endif

#ifndef NRF_USBD_ISO_DEBUG
/* Also generate information about ISOCHRONOUS events and transfers.
 * Turn this off if no ISOCHRONOUS transfers are going to be debugged and this
 * option generates a lot of useless messages. */
#define NRF_USBD_ISO_DEBUG 1
#endif

#ifndef NRF_USBD_FAILED_TRANSFERS_DEBUG
/* Also generate debug information for failed transfers.
 * It might be useful but may generate a lot of useless debug messages
 * in some library usages (for example when transfer is generated and the
 * result is used to check whatever endpoint was busy. */
#define NRF_USBD_FAILED_TRANSFERS_DEBUG 1
#endif

#ifndef NRF_USBD_DMAREQ_PROCESS_DEBUG
/* Generate additional messages that mark the status inside
 * @ref usbd_dmareq_process.
 * It is useful to debug library internals but may generate a lot of
 * useless debug messages. */
#define NRF_USBD_DMAREQ_PROCESS_DEBUG 1
#endif


/**
 * @defgroup nrf_drv_usbd_int USB Device driver internal part
 * @internal
 * @ingroup nrf_drv_usbd
 *
 * This part contains auxiliary internal macros, variables and functions.
 * @{
 */

/**
 * @brief Assert endpoint number validity
 *
 * Internal macro to be used during program creation in debug mode.
 * Generates assertion if endpoint number is not valid.
 *
 * @param ep Endpoint number to validity check
 */
#define USBD_ASSERT_EP_VALID(ep) ASSERT(                                         \
    ((NRF_USBD_EPIN_CHECK(ep)  && (NRF_USBD_EP_NR_GET(ep) < NRF_USBD_EPIN_CNT )) \
    ||                                                                           \
    (NRF_USBD_EPOUT_CHECK(ep) && (NRF_USBD_EP_NR_GET(ep) < NRF_USBD_EPOUT_CNT))) \
);

/**
 * @brief Lowest position of bit for IN endpoint
 *
 * The first bit position corresponding to IN endpoint.
 * @sa ep2bit bit2ep
 */
#define USBD_EPIN_BITPOS_0   0

/**
 * @brief Lowest position of bit for OUT endpoint
 *
 * The first bit position corresponding to OUT endpoint
 * @sa ep2bit bit2ep
 */
#define USBD_EPOUT_BITPOS_0  16

/**
 * @brief Input endpoint bits mask
 */
#define USBD_EPIN_BIT_MASK (0xFFFFU << USBD_EPIN_BITPOS_0)

/**
 * @brief Output endpoint bits mask
 */
#define USBD_EPOUT_BIT_MASK (0xFFFFU << USBD_EPOUT_BITPOS_0)

/**
 * @brief Isochronous endpoint bit mask
 */
#define USBD_EPISO_BIT_MASK \
    ((1U << USBD_EP_BITPOS(NRF_DRV_USBD_EPOUT8)) | (1U << USBD_EP_BITPOS(NRF_DRV_USBD_EPIN8)))

/**
 * @brief Auxiliary macro to change EP number into bit position
 *
 * This macro is used by @ref ep2bit function but also for statically check
 * the bitpos values integrity during compilation.
 *
 * @param[in] ep Endpoint number.
 * @return Endpoint bit position.
 */
#define USBD_EP_BITPOS(ep) \
    ((NRF_USBD_EPIN_CHECK(ep) ? USBD_EPIN_BITPOS_0 : USBD_EPOUT_BITPOS_0) + NRF_USBD_EP_NR_GET(ep))

/**
 * @brief Helper macro for creating an endpoint transfer event.
 *
 * @param[in] name     Name of the created transfer event variable.
 * @param[in] endpoint Endpoint number.
 * @param[in] ep_stat  Endpoint state to report.
 *
 * @return Initialized event constant variable.
 */
#define NRF_DRV_USBD_EP_TRANSFER_EVENT(name, endpont, ep_stat)          \
    const nrf_drv_usbd_evt_t name = {                                   \
        NRF_DRV_USBD_EVT_EPTRANSFER,                                    \
        .data = {                                                       \
            .eptransfer = {                                             \
                    .ep = endpont,                                      \
                    .status = ep_stat                                   \
            }                                                           \
        }                                                               \
    }

/* Check it the bit positions values match defined DATAEPSTATUS bit positions */
STATIC_ASSERT(USBD_EP_BITPOS(NRF_DRV_USBD_EPIN1)  == USBD_EPDATASTATUS_EPIN1_Pos );
STATIC_ASSERT(USBD_EP_BITPOS(NRF_DRV_USBD_EPIN2)  == USBD_EPDATASTATUS_EPIN2_Pos );
STATIC_ASSERT(USBD_EP_BITPOS(NRF_DRV_USBD_EPIN3)  == USBD_EPDATASTATUS_EPIN3_Pos );
STATIC_ASSERT(USBD_EP_BITPOS(NRF_DRV_USBD_EPIN4)  == USBD_EPDATASTATUS_EPIN4_Pos );
STATIC_ASSERT(USBD_EP_BITPOS(NRF_DRV_USBD_EPIN5)  == USBD_EPDATASTATUS_EPIN5_Pos );
STATIC_ASSERT(USBD_EP_BITPOS(NRF_DRV_USBD_EPIN6)  == USBD_EPDATASTATUS_EPIN6_Pos );
STATIC_ASSERT(USBD_EP_BITPOS(NRF_DRV_USBD_EPIN7)  == USBD_EPDATASTATUS_EPIN7_Pos );
STATIC_ASSERT(USBD_EP_BITPOS(NRF_DRV_USBD_EPOUT1) == USBD_EPDATASTATUS_EPOUT1_Pos);
STATIC_ASSERT(USBD_EP_BITPOS(NRF_DRV_USBD_EPOUT2) == USBD_EPDATASTATUS_EPOUT2_Pos);
STATIC_ASSERT(USBD_EP_BITPOS(NRF_DRV_USBD_EPOUT3) == USBD_EPDATASTATUS_EPOUT3_Pos);
STATIC_ASSERT(USBD_EP_BITPOS(NRF_DRV_USBD_EPOUT4) == USBD_EPDATASTATUS_EPOUT4_Pos);
STATIC_ASSERT(USBD_EP_BITPOS(NRF_DRV_USBD_EPOUT5) == USBD_EPDATASTATUS_EPOUT5_Pos);
STATIC_ASSERT(USBD_EP_BITPOS(NRF_DRV_USBD_EPOUT6) == USBD_EPDATASTATUS_EPOUT6_Pos);
STATIC_ASSERT(USBD_EP_BITPOS(NRF_DRV_USBD_EPOUT7) == USBD_EPDATASTATUS_EPOUT7_Pos);


/**
 * @name Internal auxiliary definitions for SETUP packet
 *
 * Definitions used to take out the information about last SETUP packet direction
 * from @c bmRequestType.
 * @{
 */
/** The position of DIR bit in bmRequestType inside SETUP packet */
#define USBD_DRV_REQUESTTYPE_DIR_BITPOS 7
/** The mask of DIR bit in bmRequestType inside SETUP packet */
#define USBD_DRV_REQUESTTYPE_DIR_MASK   (1U << USBD_DRV_REQUESTTYPE_DIR_BITPOS)
/** The value of DIR bit for OUT direction (Host -> Device) */
#define USBD_DRV_REQUESTTYPE_DIR_OUT    (0U << USBD_DRV_REQUESTTYPE_DIR_BITPOS)
/** The value of DIR bit for IN direction (Device -> Host) */
#define USBD_DRV_REQUESTTYPE_DIR_IN     (1U << USBD_DRV_REQUESTTYPE_DIR_BITPOS)
/** @} */

/**
 * @brief Current driver state
 */
static nrfx_drv_state_t m_drv_state = NRFX_DRV_STATE_UNINITIALIZED;

/**
 * @brief Event handler for the library
 *
 * Event handler that would be called on events.
 *
 * @note Currently it cannot be null if any interrupt is activated.
 */
static nrf_drv_usbd_event_handler_t m_event_handler;

/**
 * @brief Detected state of the bus
 *
 * Internal state changed in interrupts handling when
 * RESUME or SUSPEND event is processed.
 *
 * Values:
 * - true  - bus suspended
 * - false - ongoing normal communication on the bus
 *
 * @note This is only the bus state and does not mean that the peripheral is in suspend state.
 */
static volatile bool m_bus_suspend;

/**
 * @brief Internal constant that contains interrupts disabled in suspend state
 *
 * Internal constant used in @ref nrf_drv_usbd_suspend_irq_config and @ref nrf_drv_usbd_active_irq_config
 * functions.
 */
static const uint32_t m_irq_disabled_in_suspend =
    NRF_USBD_INT_ENDEPIN0_MASK    |
    NRF_USBD_INT_EP0DATADONE_MASK |
    NRF_USBD_INT_ENDEPOUT0_MASK   |
    NRF_USBD_INT_EP0SETUP_MASK    |
    NRF_USBD_INT_DATAEP_MASK;

/**
 * @brief Direction of last received Setup transfer
 *
 * This variable is used to redirect internal setup data event
 * into selected endpoint (IN or OUT).
 */
static nrf_drv_usbd_ep_t m_last_setup_dir;

/**
 * @brief Mark endpoint readiness for DMA transfer
 *
 * Bits in this variable are cleared and set in interrupts.
 * 1 means that endpoint is ready for DMA transfer.
 * 0 means that DMA transfer cannot be performed on selected endpoint.
 */
static uint32_t m_ep_ready;

/**
 * @brief Mark endpoint with prepared data to transfer by DMA
 *
 * This variable can be from any place in the code (interrupt or main thread).
 * It would be cleared only from USBD interrupt.
 *
 * Mask prepared USBD data for transmission.
 * It is cleared when no more data to transmit left.
 */
static uint32_t m_ep_dma_waiting;

/**
 * @brief Current EasyDMA state
 *
 * Single flag, updated only inside interrupts, that marks current EasyDMA state.
 * In USBD there is only one DMA channel working in background, and new transfer
 * cannot be started when there is ongoing transfer on any other channel.
 */
static bool m_dma_pending;

/**
 * @brief Simulated data EP status bits required for errata 104
 *
 * Marker to delete when not required anymore: >> NRF_DRV_USBD_ERRATA_ENABLE <<
 */
static uint32_t m_simulated_dataepstatus;

/**
 * @brief The structure that would hold transfer configuration to every endpoint
 *
 * The structure that holds all the data required by the endpoint to proceed
 * with LIST functionality and generate quick callback directly when data
 * buffer is ready.
 */
typedef struct
{
    nrf_drv_usbd_handler_t     handler;          //!< Handler for current transfer, function pointer
    void                     * p_context;        //!< Context for transfer handler
    size_t                     transfer_cnt;     //!< Number of transferred bytes in the current transfer
    uint16_t                   max_packet_size;  //!< Configured endpoint size
    nrf_drv_usbd_ep_status_t   status;           //!< NRF_SUCCESS or error code, never NRF_ERROR_BUSY - this one is calculated
}usbd_drv_ep_state_t;

/**
 * @brief The array of transfer configurations for the endpoints.
 *
 * The status of the transfer on each endpoint.
 */
static struct
{
    usbd_drv_ep_state_t ep_out[NRF_USBD_EPOUT_CNT]; //!< Status for OUT endpoints.
    usbd_drv_ep_state_t ep_in [NRF_USBD_EPIN_CNT ]; //!< Status for IN endpoints.
}m_ep_state;

/**
 * @brief Status variables for integrated feeders.
 *
 * Current status for integrated feeders (IN transfers).
 * Integrated feeders are used for default transfers:
 * 1. Simple RAM transfer
 * 2. Simple flash transfer
 * 3. RAM transfer with automatic ZLP
 * 4. Flash transfer with automatic ZLP
 */
nrf_drv_usbd_transfer_t m_ep_feeder_state[NRF_USBD_EPIN_CNT];

/**
 * @brief Status variables for integrated consumers
 *
 * Current status for integrated consumers
 * Currently one type of transfer is supported:
 * 1. Transfer to RAM
 *
 * Transfer is finished automatically when received data block is smaller
 * than the endpoint buffer or all the required data is received.
 */
nrf_drv_usbd_transfer_t m_ep_consumer_state[NRF_USBD_EPOUT_CNT];


/**
 * @brief Buffer used to send data directly from FLASH
 *
 * This is internal buffer that would be used to emulate the possibility
 * to transfer data directly from FLASH.
 * We do not have to care about the source of data when calling transfer functions.
 *
 * We do not need more buffers that one, because only one transfer can be pending
 * at once.
 */
static uint32_t m_tx_buffer[CEIL_DIV(
    NRF_DRV_USBD_FEEDER_BUFFER_SIZE, sizeof(uint32_t))];


/* Early declaration. Documentation above definition. */
static void usbd_dmareq_process(void);


/**
 * @brief Change endpoint number to endpoint event code
 *
 * @param ep Endpoint number
 *
 * @return Connected endpoint event code.
 *
 * Marker to delete when not required anymore: >> NRF_DRV_USBD_ERRATA_ENABLE <<
 */
static inline nrf_usbd_event_t nrf_drv_usbd_ep_to_endevent(nrf_drv_usbd_ep_t ep)
{
    USBD_ASSERT_EP_VALID(ep);

    static const nrf_usbd_event_t epin_endev[] =
    {
        NRF_USBD_EVENT_ENDEPIN0,
        NRF_USBD_EVENT_ENDEPIN1,
        NRF_USBD_EVENT_ENDEPIN2,
        NRF_USBD_EVENT_ENDEPIN3,
        NRF_USBD_EVENT_ENDEPIN4,
        NRF_USBD_EVENT_ENDEPIN5,
        NRF_USBD_EVENT_ENDEPIN6,
        NRF_USBD_EVENT_ENDEPIN7,
        NRF_USBD_EVENT_ENDISOIN0
    };
    static const nrf_usbd_event_t epout_endev[] =
    {
        NRF_USBD_EVENT_ENDEPOUT0,
        NRF_USBD_EVENT_ENDEPOUT1,
        NRF_USBD_EVENT_ENDEPOUT2,
        NRF_USBD_EVENT_ENDEPOUT3,
        NRF_USBD_EVENT_ENDEPOUT4,
        NRF_USBD_EVENT_ENDEPOUT5,
        NRF_USBD_EVENT_ENDEPOUT6,
        NRF_USBD_EVENT_ENDEPOUT7,
        NRF_USBD_EVENT_ENDISOOUT0
    };

    return (NRF_USBD_EPIN_CHECK(ep) ? epin_endev : epout_endev)[NRF_USBD_EP_NR_GET(ep)];
}


/**
 * @brief Get interrupt mask for selected endpoint
 *
 * @param[in] ep Endpoint number
 *
 * @return Interrupt mask related to the EasyDMA transfer end for the
 *         chosen endpoint.
 */
static inline uint32_t nrf_drv_usbd_ep_to_int(nrf_drv_usbd_ep_t ep)
{
    USBD_ASSERT_EP_VALID(ep);

    static const uint8_t epin_bitpos[] =
    {
        USBD_INTEN_ENDEPIN0_Pos,
        USBD_INTEN_ENDEPIN1_Pos,
        USBD_INTEN_ENDEPIN2_Pos,
        USBD_INTEN_ENDEPIN3_Pos,
        USBD_INTEN_ENDEPIN4_Pos,
        USBD_INTEN_ENDEPIN5_Pos,
        USBD_INTEN_ENDEPIN6_Pos,
        USBD_INTEN_ENDEPIN7_Pos,
        USBD_INTEN_ENDISOIN_Pos
    };
    static const uint8_t epout_bitpos[] =
    {
        USBD_INTEN_ENDEPOUT0_Pos,
        USBD_INTEN_ENDEPOUT1_Pos,
        USBD_INTEN_ENDEPOUT2_Pos,
        USBD_INTEN_ENDEPOUT3_Pos,
        USBD_INTEN_ENDEPOUT4_Pos,
        USBD_INTEN_ENDEPOUT5_Pos,
        USBD_INTEN_ENDEPOUT6_Pos,
        USBD_INTEN_ENDEPOUT7_Pos,
        USBD_INTEN_ENDISOOUT_Pos
    };

    return 1UL << (NRF_USBD_EPIN_CHECK(ep) ? epin_bitpos : epout_bitpos)[NRF_USBD_EP_NR_GET(ep)];
}

/**
 * @name Integrated feeders and consumers
 *
 * Internal, default functions for transfer processing.
 * @{
 */

/**
 * @brief Integrated consumer to RAM buffer.
 *
 * @param p_next    See @ref nrf_drv_usbd_consumer_t documentation.
 * @param p_context See @ref nrf_drv_usbd_consumer_t documentation.
 * @param ep_size   See @ref nrf_drv_usbd_consumer_t documentation.
 * @param data_size See @ref nrf_drv_usbd_consumer_t documentation.
 *
 * @retval true  Continue transfer.
 * @retval false This was the last transfer.
 */
bool nrf_drv_usbd_consumer(
    nrf_drv_usbd_ep_transfer_t * p_next,
    void * p_context,
    size_t ep_size,
    size_t data_size)
{
    nrf_drv_usbd_transfer_t * p_transfer = p_context;
    ASSERT(ep_size >= data_size);
    ASSERT((p_transfer->p_data.rx == NULL) ||
        nrfx_is_in_ram((const void*)(p_transfer->p_data.ptr)));

    size_t size = p_transfer->size;
    if (size < data_size)
    {
        NRF_LOG_DEBUG("consumer: buffer too small: r: %u, l: %u", data_size, size);
        /* Buffer size to small */
        p_next->size = 0;
        p_next->p_data = p_transfer->p_data;
    }
    else
    {
        p_next->size = data_size;
        p_next->p_data = p_transfer->p_data;
        size -= data_size;
        p_transfer->size = size;
        p_transfer->p_data.ptr += data_size;
    }
    return (ep_size == data_size) && (size != 0);
}

/**
 * @brief Integrated feeder from RAM source.
 *
 * @param[out]    p_next    See @ref nrf_drv_usbd_feeder_t documentation.
 * @param[in,out] p_context See @ref nrf_drv_usbd_feeder_t documentation.
 * @param[in]     ep_size   See @ref nrf_drv_usbd_feeder_t documentation.
 *
 * @retval true  Continue transfer.
 * @retval false This was the last transfer.
 */
bool nrf_drv_usbd_feeder_ram(
    nrf_drv_usbd_ep_transfer_t * p_next,
    void * p_context,
    size_t ep_size)
{
    nrf_drv_usbd_transfer_t * p_transfer = p_context;
    ASSERT(nrfx_is_in_ram((const void*)(p_transfer->p_data.ptr)));

    size_t tx_size = p_transfer->size;
    if (tx_size > ep_size)
    {
        tx_size = ep_size;
    }

    p_next->p_data = p_transfer->p_data;
    p_next->size = tx_size;

    p_transfer->size -= tx_size;
    p_transfer->p_data.ptr += tx_size;

    return (p_transfer->size != 0);
}

/**
 * @brief Integrated feeder from RAM source with ZLP.
 *
 * @param[out]    p_next    See @ref nrf_drv_usbd_feeder_t documentation.
 * @param[in,out] p_context See @ref nrf_drv_usbd_feeder_t documentation.
 * @param[in]     ep_size   See @ref nrf_drv_usbd_feeder_t documentation.
 *
 * @retval true  Continue transfer.
 * @retval false This was the last transfer.
 */
bool nrf_drv_usbd_feeder_ram_zlp(
    nrf_drv_usbd_ep_transfer_t * p_next,
    void * p_context,
    size_t ep_size)
{
    nrf_drv_usbd_transfer_t * p_transfer = p_context;
    ASSERT(nrfx_is_in_ram((const void*)(p_transfer->p_data.ptr)));

    size_t tx_size = p_transfer->size;
    if (tx_size > ep_size)
    {
        tx_size = ep_size;
    }

    p_next->p_data.tx = (tx_size == 0) ? NULL : p_transfer->p_data.tx;
    p_next->size = tx_size;

    p_transfer->size -= tx_size;
    p_transfer->p_data.ptr += tx_size;

    return (tx_size != 0);
}

/**
 * @brief Integrated feeder from a flash source.
 *
 * @param[out]    p_next    See @ref nrf_drv_usbd_feeder_t documentation.
 * @param[in,out] p_context See @ref nrf_drv_usbd_feeder_t documentation.
 * @param[in]     ep_size   See @ref nrf_drv_usbd_feeder_t documentation.
 *
 * @retval true  Continue transfer.
 * @retval false This was the last transfer.
 */
bool nrf_drv_usbd_feeder_flash(
    nrf_drv_usbd_ep_transfer_t * p_next,
    void * p_context,
    size_t ep_size)
{
    nrf_drv_usbd_transfer_t * p_transfer = p_context;
    ASSERT(!nrfx_is_in_ram((const void*)(p_transfer->p_data.ptr)));

    size_t tx_size  = p_transfer->size;
    void * p_buffer = nrf_drv_usbd_feeder_buffer_get();

    if (tx_size > ep_size)
    {
        tx_size = ep_size;
    }

    ASSERT(tx_size <= NRF_DRV_USBD_FEEDER_BUFFER_SIZE);
    memcpy(p_buffer, (p_transfer->p_data.tx), tx_size);

    p_next->p_data.tx = p_buffer;
    p_next->size = tx_size;

    p_transfer->size -= tx_size;
    p_transfer->p_data.ptr += tx_size;

    return (p_transfer->size != 0);
}

/**
 * @brief Integrated feeder from a flash source with ZLP.
 *
 * @param[out]    p_next    See @ref nrf_drv_usbd_feeder_t documentation.
 * @param[in,out] p_context See @ref nrf_drv_usbd_feeder_t documentation.
 * @param[in]     ep_size   See @ref nrf_drv_usbd_feeder_t documentation.
 *
 * @retval true  Continue transfer.
 * @retval false This was the last transfer.
 */
bool nrf_drv_usbd_feeder_flash_zlp(
    nrf_drv_usbd_ep_transfer_t * p_next,
    void * p_context,
    size_t ep_size)
{
    nrf_drv_usbd_transfer_t * p_transfer = p_context;
    ASSERT(!nrfx_is_in_ram((const void*)(p_transfer->p_data.ptr)));

    size_t tx_size  = p_transfer->size;
    void * p_buffer = nrf_drv_usbd_feeder_buffer_get();

    if (tx_size > ep_size)
    {
        tx_size = ep_size;
    }

    ASSERT(tx_size <= NRF_DRV_USBD_FEEDER_BUFFER_SIZE);

    if (tx_size != 0)
    {
        memcpy(p_buffer, (p_transfer->p_data.tx), tx_size);
        p_next->p_data.tx = p_buffer;
    }
    else
    {
        p_next->p_data.tx = NULL;
    }
    p_next->size = tx_size;

    p_transfer->size -= tx_size;
    p_transfer->p_data.ptr += tx_size;

    return (tx_size != 0);
}

/** @} */

/**
 * @brief Change Driver endpoint number to HAL endpoint number
 *
 * @param ep Driver endpoint identifier
 *
 * @return Endpoint identifier in HAL
 *
 * @sa nrf_drv_usbd_ep_from_hal
 */
static inline uint8_t ep_to_hal(nrf_drv_usbd_ep_t ep)
{
    USBD_ASSERT_EP_VALID(ep);
    return (uint8_t)ep;
}

/**
 * @brief Generate start task number for selected endpoint index
 *
 * @param ep Endpoint number
 *
 * @return Task for starting EasyDMA transfer on selected endpoint.
 */
static inline nrf_usbd_task_t task_start_ep(nrf_drv_usbd_ep_t ep)
{
    USBD_ASSERT_EP_VALID(ep);
    return (nrf_usbd_task_t)(
        (NRF_USBD_EPIN_CHECK(ep) ? NRF_USBD_TASK_STARTEPIN0 : NRF_USBD_TASK_STARTEPOUT0) +
        (NRF_USBD_EP_NR_GET(ep) * sizeof(uint32_t)));
}

/**
 * @brief Access selected endpoint state structure
 *
 * Function used to change or just read the state of selected endpoint.
 * It is used for internal transmission state.
 *
 * @param ep Endpoint number
 */
static inline usbd_drv_ep_state_t* ep_state_access(nrf_drv_usbd_ep_t ep)
{
    USBD_ASSERT_EP_VALID(ep);
    return ((NRF_USBD_EPIN_CHECK(ep) ? m_ep_state.ep_in : m_ep_state.ep_out) +
        NRF_USBD_EP_NR_GET(ep));
}

/**
 * @brief Change endpoint number to bit position
 *
 * Bit positions are defined the same way as they are placed in DATAEPSTATUS register,
 * but bits for endpoint 0 are included.
 *
 * @param ep Endpoint number
 *
 * @return Bit position related to the given endpoint number
 *
 * @sa bit2ep
 */
static inline uint8_t ep2bit(nrf_drv_usbd_ep_t ep)
{
    USBD_ASSERT_EP_VALID(ep);
    return USBD_EP_BITPOS(ep);
}

/**
 * @brief Change bit position to endpoint number
 *
 * @param bitpos Bit position
 *
 * @return Endpoint number corresponding to given bit position.
 *
 * @sa ep2bit
 */
static inline nrf_drv_usbd_ep_t bit2ep(uint8_t bitpos)
{
    STATIC_ASSERT(USBD_EPOUT_BITPOS_0 > USBD_EPIN_BITPOS_0);
    return (nrf_drv_usbd_ep_t)((bitpos >= USBD_EPOUT_BITPOS_0) ?
        NRF_USBD_EPOUT(bitpos - USBD_EPOUT_BITPOS_0) : NRF_USBD_EPIN(bitpos));
}

/**
 * @brief Mark that EasyDMA is working
 *
 * Internal function to set the flag informing about EasyDMA transfer pending.
 * This function is called always just after the EasyDMA transfer is started.
 */
static inline void usbd_dma_pending_set(void)
{
    if (nrf_drv_usb_errata_199())
    {
        *((volatile uint32_t *)0x40027C1C) = 0x00000082;
    }
    m_dma_pending = true;
}

/**
 * @brief Mark that EasyDMA is free
 *
 * Internal function to clear the flag informing about EasyDMA transfer pending.
 * This function is called always just after the finished EasyDMA transfer is detected.
 */
static inline void usbd_dma_pending_clear(void)
{
    if (nrf_drv_usb_errata_199())
    {
        *((volatile uint32_t *)0x40027C1C) = 0x00000000;
    }
    m_dma_pending = false;
}

/**
 * @brief Start selected EasyDMA transmission
 *
 * This is internal auxiliary function.
 * No checking is made if EasyDMA is ready for new transmission.
 *
 * @param[in] ep Number of endpoint for transmission.
 *               If it is OUT endpoint transmission would be directed from endpoint to RAM.
 *               If it is in endpoint transmission would be directed from RAM to endpoint.
 */
static inline void usbd_dma_start(nrf_drv_usbd_ep_t ep)
{
    nrf_usbd_task_trigger(task_start_ep(ep));
}


void nrf_drv_usbd_isoinconfig_set(nrf_usbd_isoinconfig_t config)
{
    ASSERT(!nrf_drv_usbd_errata_type_52840_eng_a());
    nrf_usbd_isoinconfig_set(config);
}

nrf_usbd_isoinconfig_t nrf_drv_usbd_isoinconfig_get(void)
{
    ASSERT(!nrf_drv_usbd_errata_type_52840_eng_a());
    return nrf_usbd_isoinconfig_get();
}

/**
 * @brief Abort pending transfer on selected endpoint
 *
 * @param ep Endpoint number.
 *
 * @note
 * This function locks interrupts that may be costly.
 * It is good idea to test if the endpoint is still busy before calling this function:
 * @code
   (m_ep_dma_waiting & (1U << ep2bit(ep)))
 * @endcode
 * This function would check it again, but it makes it inside critical section.
 */
static inline void usbd_ep_abort(nrf_drv_usbd_ep_t ep)
{
    CRITICAL_REGION_ENTER();

    usbd_drv_ep_state_t * p_state = ep_state_access(ep);

    if (NRF_USBD_EPOUT_CHECK(ep))
    {
        /* Host -> Device */
        if ((~m_ep_dma_waiting) & (1U << ep2bit(ep)))
        {
            /* If the bit in m_ep_dma_waiting in cleared - nothing would be
             * processed inside transfer processing */
            nrf_drv_usbd_transfer_out_drop(ep);
        }
        else
        {
            p_state->handler.consumer = NULL;
            m_ep_dma_waiting &= ~(1U << ep2bit(ep));
            m_ep_ready &= ~(1U << ep2bit(ep));
        }
        /* Aborted */
        p_state->status = NRF_USBD_EP_ABORTED;
    }
    else
    {
        if(!NRF_USBD_EPISO_CHECK(ep))
        {
            if(ep != NRF_DRV_USBD_EPIN0)
            {
                *((volatile uint32_t *)(NRF_USBD_BASE + 0x800)) = 0x7B6 + (2u * (NRF_USBD_EP_NR_GET(ep) - 1)); //inxbsy
                uint8_t temp = *((volatile uint32_t *)(NRF_USBD_BASE + 0x804));
                temp |= (1U << 1);
                *((volatile uint32_t *)(NRF_USBD_BASE + 0x804)) |= temp;
                UNUSED_VARIABLE(*((volatile uint32_t *)(NRF_USBD_BASE + 0x804)));
            }
            else
            {
                *((volatile uint32_t *)(NRF_USBD_BASE + 0x800)) = 0x7B4;
                uint8_t temp = *((volatile uint32_t *)(NRF_USBD_BASE + 0x804));
                temp |= (1U << 2);
                *((volatile uint32_t *)(NRF_USBD_BASE + 0x804)) |= temp;
                UNUSED_VARIABLE(*((volatile uint32_t *)(NRF_USBD_BASE + 0x804)));
            }
        }

        if ((m_ep_dma_waiting | (~m_ep_ready)) & (1U << ep2bit(ep)))
        {
            /* Device -> Host */
            m_ep_dma_waiting &= ~(1U << ep2bit(ep));
            m_ep_ready       |=   1U << ep2bit(ep) ;

            p_state->handler.feeder = NULL;
            p_state->status = NRF_USBD_EP_ABORTED;
            NRF_DRV_USBD_EP_TRANSFER_EVENT(evt, ep, NRF_USBD_EP_ABORTED);
            m_event_handler(&evt);
        }
    }
    CRITICAL_REGION_EXIT();
}

void nrf_drv_usbd_ep_abort(nrf_drv_usbd_ep_t ep)
{
    usbd_ep_abort(ep);
}


/**
 * @brief Abort all pending endpoints
 *
 * Function aborts all pending endpoint transfers.
 */
static void usbd_ep_abort_all(void)
{
    uint32_t ep_waiting = m_ep_dma_waiting | (m_ep_ready & USBD_EPOUT_BIT_MASK);
    while (0 != ep_waiting)
    {
        uint8_t bitpos = __CLZ(__RBIT(ep_waiting));
        if(!NRF_USBD_EPISO_CHECK(bit2ep(bitpos)))
        {
            usbd_ep_abort(bit2ep(bitpos));
        }
        ep_waiting &= ~(1U << bitpos);
    }

    m_ep_ready = (((1U << NRF_USBD_EPIN_CNT) - 1U) << USBD_EPIN_BITPOS_0);
}

/**
 * @brief Force the USBD interrupt into pending state
 *
 * This function is used to force USBD interrupt to be processed right now.
 * It makes it possible to process all EasyDMA access on one thread priority level.
 */
static inline void usbd_int_rise(void)
{
    NVIC_SetPendingIRQ(USBD_IRQn);
}

/**
 * @name USBD interrupt runtimes
 *
 * Interrupt runtimes that would be vectorized using @ref m_ivec_isr
 * @{
 */

static void ev_usbreset_handler(void)
{
    m_bus_suspend = false;
    m_last_setup_dir = NRF_DRV_USBD_EPOUT0;

    const nrf_drv_usbd_evt_t evt = {
            .type = NRF_DRV_USBD_EVT_RESET
    };

    m_event_handler(&evt);
}

static void ev_started_handler(void)
{
#if NRF_DRV_USBD_STARTED_EV_ENABLE
    uint32_t epstatus = nrf_usbd_epstatus_get_and_clear();

    /* All finished endpoint have to be marked as busy */
    // #warning Check this one
    // ASSERT(epstatus == ((~m_ep_ready) & epstatus));
    while (epstatus)
    {
        uint8_t           bitpos = __CLZ(__RBIT(epstatus));
        nrf_drv_usbd_ep_t ep     = bit2ep(bitpos);
        epstatus                &= ~(1UL << bitpos);

        UNUSED_VARIABLE(ep);
    }
#endif
}

/**
 * @brief Handler for EasyDMA event without endpoint clearing.
 *
 * This handler would be called when EasyDMA transfer for endpoints that does not require clearing.
 * All in endpoints are cleared automatically when new EasyDMA transfer is initialized.
 * For endpoint 0 see @ref nrf_usbd_ep0out_dma_handler
 *
 * @param[in] ep Endpoint number
 */
static inline void nrf_usbd_ep0in_dma_handler(void)
{
    const nrf_drv_usbd_ep_t ep = NRF_DRV_USBD_EPIN0;
    NRF_LOG_DEBUG("USB event: DMA ready IN0");
    usbd_dma_pending_clear();

    usbd_drv_ep_state_t * p_state = ep_state_access(ep);
    if (NRF_USBD_EP_ABORTED == p_state->status)
    {
        /* Clear transfer information just in case */
        UNUSED_RETURN_VALUE(nrf_atomic_u32_and(&m_ep_dma_waiting, ~(1U << ep2bit(ep))));
    }
    else if (p_state->handler.feeder == NULL)
    {
        UNUSED_RETURN_VALUE(nrf_atomic_u32_and(&m_ep_dma_waiting, ~(1U << ep2bit(ep))));
    }
    else
    {
        /* Nothing to do */
    }
}

/**
 * @brief Handler for EasyDMA event without endpoint clearing.
 *
 * This handler would be called when EasyDMA transfer for endpoints that does not require clearing.
 * All in endpoints are cleared automatically when new EasyDMA transfer is initialized.
 * For endpoint 0 see @ref nrf_usbd_ep0out_dma_handler
 *
 * @param[in] ep Endpoint number
 */
static inline void nrf_usbd_epin_dma_handler(nrf_drv_usbd_ep_t ep)
{

    NRF_LOG_DEBUG("USB event: DMA ready IN: %x", ep);
    ASSERT(NRF_USBD_EPIN_CHECK(ep));
    ASSERT(!NRF_USBD_EPISO_CHECK(ep));
    ASSERT(NRF_USBD_EP_NR_GET(ep) > 0);
    usbd_dma_pending_clear();

    usbd_drv_ep_state_t * p_state = ep_state_access(ep);
    if (NRF_USBD_EP_ABORTED == p_state->status)
    {
        /* Clear transfer information just in case */
        UNUSED_RETURN_VALUE(nrf_atomic_u32_and(&m_ep_dma_waiting, ~(1U << ep2bit(ep))));
    }
    else if (p_state->handler.feeder == NULL)
    {
        UNUSED_RETURN_VALUE(nrf_atomic_u32_and(&m_ep_dma_waiting, ~(1U << ep2bit(ep))));
    }
    else
    {
        /* Nothing to do */
    }
}

/**
 * @brief Handler for EasyDMA event from in isochronous endpoint
 *
 * @todo RK documentation
 */
static inline void nrf_usbd_epiniso_dma_handler(nrf_drv_usbd_ep_t ep)
{
    if (NRF_USBD_ISO_DEBUG)
    {
        NRF_LOG_DEBUG("USB event: DMA ready ISOIN: %x", ep);
    }
    ASSERT(NRF_USBD_EPIN_CHECK(ep));
    ASSERT(NRF_USBD_EPISO_CHECK(ep));
    usbd_dma_pending_clear();

    usbd_drv_ep_state_t * p_state = ep_state_access(ep);
    if (NRF_USBD_EP_ABORTED == p_state->status)
    {
        /* Clear transfer information just in case */
        UNUSED_RETURN_VALUE(nrf_atomic_u32_and(&m_ep_dma_waiting, ~(1U << ep2bit(ep))));
    }
    else if (p_state->handler.feeder == NULL)
    {
        UNUSED_RETURN_VALUE(nrf_atomic_u32_and(&m_ep_dma_waiting, ~(1U << ep2bit(ep))));
        /* Send event to the user - for an ISO IN endpoint, the whole transfer is finished in this moment */
        NRF_DRV_USBD_EP_TRANSFER_EVENT(evt, ep, NRF_USBD_EP_OK);
        m_event_handler(&evt);
    }
    else
    {
        /* Nothing to do */
    }
}

/**
 * @brief Handler for EasyDMA event for OUT endpoint 0.
 *
 * EP0 OUT have to be cleared automatically in special way - only in the middle of the transfer.
 * It cannot be cleared when required transfer is finished because it means the same that accepting the comment.
 */
static inline void nrf_usbd_ep0out_dma_handler(void)
{
    const nrf_drv_usbd_ep_t ep = NRF_DRV_USBD_EPOUT0;
    NRF_LOG_DEBUG("USB event: DMA ready OUT0");
    usbd_dma_pending_clear();

    usbd_drv_ep_state_t * p_state = ep_state_access(ep);
    if (NRF_USBD_EP_ABORTED == p_state->status)
    {
        /* Clear transfer information just in case */
        UNUSED_RETURN_VALUE(nrf_atomic_u32_and(&m_ep_dma_waiting, ~(1U << ep2bit(ep))));
    }
    else if (p_state->handler.consumer == NULL)
    {
        UNUSED_RETURN_VALUE(nrf_atomic_u32_and(&m_ep_dma_waiting, ~(1U << ep2bit(ep))));
        /* Send event to the user - for an OUT endpoint, the whole transfer is finished in this moment */
        NRF_DRV_USBD_EP_TRANSFER_EVENT(evt, ep, NRF_USBD_EP_OK);
        m_event_handler(&evt);
    }
    else
    {
        nrf_drv_usbd_setup_data_clear();
    }
}

/**
 * @brief Handler for EasyDMA event from endpoinpoint that requires clearing.
 *
 * This handler would be called when EasyDMA transfer for OUT endpoint has been finished.
 *
 * @param[in] ep Endpoint number
 *
 */
static inline void nrf_usbd_epout_dma_handler(nrf_drv_usbd_ep_t ep)
{
    NRF_LOG_DEBUG("USB drv: DMA ready OUT: %x", ep);
    ASSERT(NRF_USBD_EPOUT_CHECK(ep));
    ASSERT(!NRF_USBD_EPISO_CHECK(ep));
    ASSERT(NRF_USBD_EP_NR_GET(ep) > 0);
    usbd_dma_pending_clear();

    usbd_drv_ep_state_t * p_state = ep_state_access(ep);
    if (NRF_USBD_EP_ABORTED == p_state->status)
    {
        /* Clear transfer information just in case */
        UNUSED_RETURN_VALUE(nrf_atomic_u32_and(&m_ep_dma_waiting, ~(1U << ep2bit(ep))));
    }
    else if (p_state->handler.consumer == NULL)
    {
        UNUSED_RETURN_VALUE(nrf_atomic_u32_and(&m_ep_dma_waiting, ~(1U << ep2bit(ep))));
        /* Send event to the user - for an OUT endpoint, the whole transfer is finished in this moment */
        NRF_DRV_USBD_EP_TRANSFER_EVENT(evt, ep, NRF_USBD_EP_OK);
        m_event_handler(&evt);
    }
    else
    {
        /* Nothing to do */
    }

#if NRF_DRV_USBD_EARLY_DMA_PROCESS
    /* Speed up */
    usbd_dmareq_process();
#endif
}

/**
 * @brief Handler for EasyDMA event from out isochronous endpoint
 *
 * @todo RK documentation
 */

static inline void nrf_usbd_epoutiso_dma_handler(nrf_drv_usbd_ep_t ep)
{
    if (NRF_USBD_ISO_DEBUG)
    {
        NRF_LOG_DEBUG("USB drv: DMA ready ISOOUT: %x", ep);
    }
    ASSERT(NRF_USBD_EPISO_CHECK(ep));
    usbd_dma_pending_clear();

    usbd_drv_ep_state_t * p_state = ep_state_access(ep);
    if (NRF_USBD_EP_ABORTED == p_state->status)
    {
        /* Nothing to do - just ignore */
    }
    else if (p_state->handler.consumer == NULL)
    {
        UNUSED_RETURN_VALUE(nrf_atomic_u32_and(&m_ep_dma_waiting, ~(1U << ep2bit(ep))));
        /* Send event to the user - for an OUT endpoint, the whole transfer is finished in this moment */
        NRF_DRV_USBD_EP_TRANSFER_EVENT(evt, ep, NRF_USBD_EP_OK);
        m_event_handler(&evt);
    }
    else
    {
        /* Nothing to do */
    }
}


static void ev_dma_epin0_handler(void)  { nrf_usbd_ep0in_dma_handler(); }
static void ev_dma_epin1_handler(void)  { nrf_usbd_epin_dma_handler(NRF_DRV_USBD_EPIN1 ); }
static void ev_dma_epin2_handler(void)  { nrf_usbd_epin_dma_handler(NRF_DRV_USBD_EPIN2 ); }
static void ev_dma_epin3_handler(void)  { nrf_usbd_epin_dma_handler(NRF_DRV_USBD_EPIN3 ); }
static void ev_dma_epin4_handler(void)  { nrf_usbd_epin_dma_handler(NRF_DRV_USBD_EPIN4 ); }
static void ev_dma_epin5_handler(void)  { nrf_usbd_epin_dma_handler(NRF_DRV_USBD_EPIN5 ); }
static void ev_dma_epin6_handler(void)  { nrf_usbd_epin_dma_handler(NRF_DRV_USBD_EPIN6 ); }
static void ev_dma_epin7_handler(void)  { nrf_usbd_epin_dma_handler(NRF_DRV_USBD_EPIN7 ); }
static void ev_dma_epin8_handler(void)  { nrf_usbd_epiniso_dma_handler(NRF_DRV_USBD_EPIN8 ); }

static void ev_dma_epout0_handler(void) { nrf_usbd_ep0out_dma_handler(); }
static void ev_dma_epout1_handler(void) { nrf_usbd_epout_dma_handler(NRF_DRV_USBD_EPOUT1); }
static void ev_dma_epout2_handler(void) { nrf_usbd_epout_dma_handler(NRF_DRV_USBD_EPOUT2); }
static void ev_dma_epout3_handler(void) { nrf_usbd_epout_dma_handler(NRF_DRV_USBD_EPOUT3); }
static void ev_dma_epout4_handler(void) { nrf_usbd_epout_dma_handler(NRF_DRV_USBD_EPOUT4); }
static void ev_dma_epout5_handler(void) { nrf_usbd_epout_dma_handler(NRF_DRV_USBD_EPOUT5); }
static void ev_dma_epout6_handler(void) { nrf_usbd_epout_dma_handler(NRF_DRV_USBD_EPOUT6); }
static void ev_dma_epout7_handler(void) { nrf_usbd_epout_dma_handler(NRF_DRV_USBD_EPOUT7); }
static void ev_dma_epout8_handler(void) { nrf_usbd_epoutiso_dma_handler(NRF_DRV_USBD_EPOUT8); }

static void ev_sof_handler(void)
{
    nrf_drv_usbd_evt_t evt =  {
            NRF_DRV_USBD_EVT_SOF,
            .data = { .sof = { .framecnt = nrf_usbd_framecntr_get() }}
    };

    /* Process isochronous endpoints */
    uint32_t iso_ready_mask = (1U << ep2bit(NRF_DRV_USBD_EPIN8));
    if (nrf_usbd_episoout_size_get(NRF_DRV_USBD_EPOUT8) != NRF_USBD_EPISOOUT_NO_DATA)
    {
        iso_ready_mask |= (1U << ep2bit(NRF_DRV_USBD_EPOUT8));
    }
    m_ep_ready |= iso_ready_mask;

    m_event_handler(&evt);
}

/**
 * @brief React on data transfer finished
 *
 * Auxiliary internal function.
 * @param ep     Endpoint number
 * @param bitpos Bit position for selected endpoint number
 */
static void usbd_ep_data_handler(nrf_drv_usbd_ep_t ep, uint8_t bitpos)
{
    NRF_LOG_DEBUG("USBD event: EndpointData: %x", ep);
    /* Mark endpoint ready for next DMA access */
    m_ep_ready |= (1U << bitpos);

    if (NRF_USBD_EPIN_CHECK(ep))
    {
        /* IN endpoint (Device -> Host) */
        if (0 == (m_ep_dma_waiting & (1U << bitpos)))
        {
            NRF_LOG_DEBUG("USBD event: EndpointData: In finished");
            /* No more data to be send - transmission finished */
            NRF_DRV_USBD_EP_TRANSFER_EVENT(evt, ep, NRF_USBD_EP_OK);
            m_event_handler(&evt);
        }
    }
    else
    {
        /* OUT endpoint (Host -> Device) */
        if (0 == (m_ep_dma_waiting & (1U << bitpos)))
        {
            NRF_LOG_DEBUG("USBD event: EndpointData: Out waiting");
            /* No buffer prepared - send event to the application */
            NRF_DRV_USBD_EP_TRANSFER_EVENT(evt, ep, NRF_USBD_EP_WAITING);
            m_event_handler(&evt);
        }
    }
}

static void ev_setup_data_handler(void)
{
    usbd_ep_data_handler(m_last_setup_dir, ep2bit(m_last_setup_dir));
}

static void ev_setup_handler(void)
{
    NRF_LOG_DEBUG("USBD event: Setup (rt:%.2x r:%.2x v:%.4x i:%.4x l:%u )",
        nrf_usbd_setup_bmrequesttype_get(),
        nrf_usbd_setup_brequest_get(),
        nrf_usbd_setup_wvalue_get(),
        nrf_usbd_setup_windex_get(),
        nrf_usbd_setup_wlength_get());
    uint8_t bmRequestType = nrf_usbd_setup_bmrequesttype_get();


    if ((m_ep_dma_waiting | ((~m_ep_ready) & USBD_EPIN_BIT_MASK)) & (1U <<ep2bit(m_last_setup_dir)))
    {
        NRF_LOG_DEBUG("USBD drv: Trying to abort last transfer on EP0");
        usbd_ep_abort(m_last_setup_dir);
    }

    m_last_setup_dir =
        ((bmRequestType & USBD_DRV_REQUESTTYPE_DIR_MASK) == USBD_DRV_REQUESTTYPE_DIR_OUT) ?
        NRF_DRV_USBD_EPOUT0 : NRF_DRV_USBD_EPIN0;

    UNUSED_RETURN_VALUE(nrf_atomic_u32_and(
        &m_ep_dma_waiting,
        ~((1U << ep2bit(NRF_DRV_USBD_EPOUT0)) | (1U << ep2bit(NRF_DRV_USBD_EPIN0)))));
    m_ep_ready |= 1U << ep2bit(NRF_DRV_USBD_EPIN0);


    const nrf_drv_usbd_evt_t evt = {
            .type = NRF_DRV_USBD_EVT_SETUP
    };
    m_event_handler(&evt);
}

static void ev_usbevent_handler(void)
{
    uint32_t event = nrf_usbd_eventcause_get_and_clear();

    if (event & NRF_USBD_EVENTCAUSE_ISOOUTCRC_MASK)
    {
        NRF_LOG_DEBUG("USBD event: ISOOUTCRC");
        /* Currently no support */
    }
    if (event & NRF_USBD_EVENTCAUSE_SUSPEND_MASK)
    {
        NRF_LOG_DEBUG("USBD event: SUSPEND");
        m_bus_suspend = true;
        const nrf_drv_usbd_evt_t evt = {
                .type = NRF_DRV_USBD_EVT_SUSPEND
        };
        m_event_handler(&evt);
    }
    if (event & NRF_USBD_EVENTCAUSE_RESUME_MASK)
    {
        NRF_LOG_DEBUG("USBD event: RESUME");
        m_bus_suspend = false;
        const nrf_drv_usbd_evt_t evt = {
                .type = NRF_DRV_USBD_EVT_RESUME
        };
        m_event_handler(&evt);
    }
    if (event & NRF_USBD_EVENTCAUSE_WUREQ_MASK)
    {
        NRF_LOG_DEBUG("USBD event: WUREQ (%s)", m_bus_suspend ? "In Suspend" : "Active");
        if (m_bus_suspend)
        {
            ASSERT(!nrf_usbd_lowpower_check());
            m_bus_suspend = false;

            nrf_usbd_dpdmvalue_set(NRF_USBD_DPDMVALUE_RESUME);
            nrf_usbd_task_trigger(NRF_USBD_TASK_DRIVEDPDM);

            const nrf_drv_usbd_evt_t evt = {
                    .type = NRF_DRV_USBD_EVT_WUREQ
            };
            m_event_handler(&evt);
        }
    }
}

static void ev_epdata_handler(void)
{
    /* Get all endpoints that have acknowledged transfer */
    uint32_t dataepstatus = nrf_usbd_epdatastatus_get_and_clear();
    if (nrf_drv_usbd_errata_104())
    {
        dataepstatus |= (m_simulated_dataepstatus &
            ~((1U << USBD_EPOUT_BITPOS_0) | (1U << USBD_EPIN_BITPOS_0)));
        m_simulated_dataepstatus &=
             ((1U << USBD_EPOUT_BITPOS_0) | (1U << USBD_EPIN_BITPOS_0));
    }
    NRF_LOG_DEBUG("USBD event: EndpointEPStatus: %x", dataepstatus);

    /* All finished endpoint have to be marked as busy */
    while (dataepstatus)
    {
        uint8_t           bitpos = __CLZ(__RBIT(dataepstatus));
        nrf_drv_usbd_ep_t ep     = bit2ep(bitpos);
        dataepstatus &= ~(1UL << bitpos);

        UNUSED_RETURN_VALUE(usbd_ep_data_handler(ep, bitpos));
    }
    if (NRF_DRV_USBD_EARLY_DMA_PROCESS)
    {
        /* Speed up */
        usbd_dmareq_process();
    }
}

/**
 * @brief Function to select the endpoint to start
 *
 * Function that realizes algorithm to schedule right channel for EasyDMA transfer.
 * It gets a variable with flags for the endpoints currently requiring transfer.
 *
 * @param[in] req Bit flags for channels currently requiring transfer.
 *                Bits 0...8 used for IN endpoints.
 *                Bits 16...24 used for OUT endpoints.
 * @note
 * This function would be never called with 0 as a @c req argument.
 * @return The bit number of the endpoint that should be processed now.
 */
static uint8_t usbd_dma_scheduler_algorithm(uint32_t req)
{
    /* Only prioritized scheduling mode is supported */
    STATIC_ASSERT(USBD_CONFIG_DMASCHEDULER_MODE == NRF_DRV_USBD_DMASCHEDULER_PRIORITIZED);
    return __CLZ(__RBIT(req));
}

/**
 * @brief Get the size of isochronous endpoint
 *
 * The size of isochronous endpoint is configurable.
 * This function returns the size of isochronous buffer taking into account
 * current configuration.
 *
 * @param[in] ep Endpoint number.
 *
 * @return The size of endpoint buffer.
 */
static inline size_t usbd_ep_iso_capacity(nrf_drv_usbd_ep_t ep)
{
    UNUSED_PARAMETER(ep);
    nrf_usbd_isosplit_t split = nrf_usbd_isosplit_get();
    if (NRF_USBD_ISOSPLIT_HALF == split)
    {
        return NRF_DRV_USBD_ISOSIZE / 2;
    }
    return NRF_DRV_USBD_ISOSIZE;
}

/**
 * @brief Process all DMA requests
 *
 * Function that have to be called from USBD interrupt handler.
 * It have to be called when all the interrupts connected with endpoints transfer
 * and DMA transfer are already handled.
 */
static void usbd_dmareq_process(void)
{
    if (!m_dma_pending)
    {
        uint32_t req;
        while (0 != (req = m_ep_dma_waiting & m_ep_ready))
        {
            uint8_t pos;
            if (USBD_CONFIG_DMASCHEDULER_ISO_BOOST && ((req & USBD_EPISO_BIT_MASK) != 0))
            {
                pos = usbd_dma_scheduler_algorithm(req & USBD_EPISO_BIT_MASK);
            }
            else
            {
                pos = usbd_dma_scheduler_algorithm(req);
            }
            nrf_drv_usbd_ep_t ep = bit2ep(pos);
            usbd_drv_ep_state_t * p_state = ep_state_access(ep);

            nrf_drv_usbd_ep_transfer_t transfer;
            bool continue_transfer;

            STATIC_ASSERT(offsetof(usbd_drv_ep_state_t, handler.feeder) ==
                offsetof(usbd_drv_ep_state_t, handler.consumer));
            ASSERT((p_state->handler.feeder) != NULL);

            if (NRF_USBD_EPIN_CHECK(ep))
            {
                /* Device -> Host */
                continue_transfer = p_state->handler.feeder(
                    &transfer,
                    p_state->p_context,
                    p_state->max_packet_size);

                if (!continue_transfer)
                {
                    p_state->handler.feeder = NULL;
                }
            }
            else
            {
                /* Host -> Device */
                const size_t rx_size = nrf_drv_usbd_epout_size_get(ep);
                continue_transfer = p_state->handler.consumer(
                    &transfer,
                    p_state->p_context,
                    p_state->max_packet_size,
                    rx_size);

                if (transfer.p_data.rx == NULL)
                {
                    /* Dropping transfer - allow processing */
                    ASSERT(transfer.size == 0);
                }
                else if (transfer.size < rx_size)
                {
                    NRF_LOG_DEBUG("Endpoint %x overload (r: %u, e: %u)", ep, rx_size, transfer.size);
                    p_state->status = NRF_USBD_EP_OVERLOAD;
                    UNUSED_RETURN_VALUE(nrf_atomic_u32_and(&m_ep_dma_waiting, ~(1U << pos)));
                    NRF_DRV_USBD_EP_TRANSFER_EVENT(evt, ep, NRF_USBD_EP_OVERLOAD);
                    m_event_handler(&evt);
                    /* This endpoint will not be transmitted now, repeat the loop */
                    continue;
                }
                else
                {
                    /* Nothing to do - only check integrity if assertions are enabled */
                    ASSERT(transfer.size == rx_size);
                }

                if (!continue_transfer)
                {
                    p_state->handler.consumer = NULL;
                }
            }

            usbd_dma_pending_set();
            m_ep_ready &= ~(1U << pos);
            if (NRF_USBD_ISO_DEBUG || (!NRF_USBD_EPISO_CHECK(ep)))
            {
                NRF_LOG_DEBUG(
                    "USB DMA process: Starting transfer on EP: %x, size: %u",
                    ep,
                    transfer.size);
            }
            /* Update number of currently transferred bytes */
            p_state->transfer_cnt += transfer.size;
            /* Start transfer to the endpoint buffer */
            nrf_usbd_ep_easydma_set(ep, transfer.p_data.ptr, (uint32_t)transfer.size);

            if (nrf_drv_usbd_errata_104())
            {
                uint32_t cnt_end = (uint32_t)(-1);
                do
                {
                    uint32_t cnt = (uint32_t)(-1);
                    do
                    {
                        nrf_usbd_event_clear(NRF_USBD_EVENT_STARTED);
                        usbd_dma_start(ep);
                        nrf_delay_us(2);
                        ++cnt;
                    }while (!nrf_usbd_event_check(NRF_USBD_EVENT_STARTED));
                    if (cnt)
                    {
                        NRF_DRV_USBD_LOG_PROTO1_FIX_PRINTF("   DMA restarted: %u times", cnt);
                    }

                    nrf_delay_us(30);
                    while (0 == (0x20 & *((volatile uint32_t *)(NRF_USBD_BASE + 0x474))))
                    {
                        nrf_delay_us(2);
                    }
                    nrf_delay_us(1);

                    ++cnt_end;
                } while (!nrf_usbd_event_check(nrf_drv_usbd_ep_to_endevent(ep)));
                if (cnt_end)
                {
                    NRF_DRV_USBD_LOG_PROTO1_FIX_PRINTF("   DMA fully restarted: %u times", cnt_end);
                }
            }
            else
            {
                usbd_dma_start(ep);
                /* There is a lot of USBD registers that cannot be accessed during EasyDMA transfer.
                 * This is quick fix to maintain stability of the stack.
                 * It cost some performance but makes stack stable. */
                while (!nrf_usbd_event_check(nrf_drv_usbd_ep_to_endevent(ep)))
                {
                    /* Empty */
                }
            }

            if (NRF_USBD_DMAREQ_PROCESS_DEBUG)
            {
                NRF_LOG_DEBUG("USB DMA process - finishing");
            }
            /* Transfer started - exit the loop */
            break;
        }
    }
    else
    {
        if (NRF_USBD_DMAREQ_PROCESS_DEBUG)
        {
            NRF_LOG_DEBUG("USB DMA process - EasyDMA busy");
        }
    }
}
/** @} */

typedef void (*nrf_drv_usbd_isr_t)(void);

/**
 * @brief USBD interrupt service runtimes
 *
 */
static const nrf_drv_usbd_isr_t m_isr[] =
{
    [USBD_INTEN_USBRESET_Pos   ] = ev_usbreset_handler,
    [USBD_INTEN_STARTED_Pos    ] = ev_started_handler,
    [USBD_INTEN_ENDEPIN0_Pos   ] = ev_dma_epin0_handler,
    [USBD_INTEN_ENDEPIN1_Pos   ] = ev_dma_epin1_handler,
    [USBD_INTEN_ENDEPIN2_Pos   ] = ev_dma_epin2_handler,
    [USBD_INTEN_ENDEPIN3_Pos   ] = ev_dma_epin3_handler,
    [USBD_INTEN_ENDEPIN4_Pos   ] = ev_dma_epin4_handler,
    [USBD_INTEN_ENDEPIN5_Pos   ] = ev_dma_epin5_handler,
    [USBD_INTEN_ENDEPIN6_Pos   ] = ev_dma_epin6_handler,
    [USBD_INTEN_ENDEPIN7_Pos   ] = ev_dma_epin7_handler,
    [USBD_INTEN_EP0DATADONE_Pos] = ev_setup_data_handler,
    [USBD_INTEN_ENDISOIN_Pos   ] = ev_dma_epin8_handler,
    [USBD_INTEN_ENDEPOUT0_Pos  ] = ev_dma_epout0_handler,
    [USBD_INTEN_ENDEPOUT1_Pos  ] = ev_dma_epout1_handler,
    [USBD_INTEN_ENDEPOUT2_Pos  ] = ev_dma_epout2_handler,
    [USBD_INTEN_ENDEPOUT3_Pos  ] = ev_dma_epout3_handler,
    [USBD_INTEN_ENDEPOUT4_Pos  ] = ev_dma_epout4_handler,
    [USBD_INTEN_ENDEPOUT5_Pos  ] = ev_dma_epout5_handler,
    [USBD_INTEN_ENDEPOUT6_Pos  ] = ev_dma_epout6_handler,
    [USBD_INTEN_ENDEPOUT7_Pos  ] = ev_dma_epout7_handler,
    [USBD_INTEN_ENDISOOUT_Pos  ] = ev_dma_epout8_handler,
    [USBD_INTEN_SOF_Pos        ] = ev_sof_handler,
    [USBD_INTEN_USBEVENT_Pos   ] = ev_usbevent_handler,
    [USBD_INTEN_EP0SETUP_Pos   ] = ev_setup_handler,
    [USBD_INTEN_EPDATA_Pos     ] = ev_epdata_handler
};

/**
 * @name Interrupt handlers
 *
 * @{
 */
void USBD_IRQHandler(void)
{
    const uint32_t enabled = nrf_usbd_int_enable_get();
    uint32_t to_process = enabled;
    uint32_t active = 0;

    /* Check all enabled interrupts */
    while (to_process)
    {
        uint8_t event_nr = __CLZ(__RBIT(to_process));
        if (nrf_usbd_event_get_and_clear((nrf_usbd_event_t)nrfx_bitpos_to_event(event_nr)))
        {
            active |= 1UL << event_nr;
        }
        to_process &= ~(1UL << event_nr);
    }

    if (nrf_drv_usbd_errata_104())
    {
        /* Event correcting */
        if ((!m_dma_pending) && (0 != (active & (USBD_INTEN_SOF_Msk))))
        {
            uint8_t usbi, uoi, uii;
            /* Testing */
            *((volatile uint32_t *)(NRF_USBD_BASE + 0x800)) = 0x7A9;
            uii = (uint8_t)(*((volatile uint32_t *)(NRF_USBD_BASE + 0x804)));
            if (0 != uii)
            {
                uii &= (uint8_t)(*((volatile uint32_t *)(NRF_USBD_BASE + 0x804)));
            }

            *((volatile uint32_t *)(NRF_USBD_BASE + 0x800)) = 0x7AA;
            uoi = (uint8_t)(*((volatile uint32_t *)(NRF_USBD_BASE + 0x804)));
            if (0 != uoi)
            {
                uoi &= (uint8_t)(*((volatile uint32_t *)(NRF_USBD_BASE + 0x804)));
            }
            *((volatile uint32_t *)(NRF_USBD_BASE + 0x800)) = 0x7AB;
            usbi = (uint8_t)(*((volatile uint32_t *)(NRF_USBD_BASE + 0x804)));
            if (0 != usbi)
            {
                usbi &= (uint8_t)(*((volatile uint32_t *)(NRF_USBD_BASE + 0x804)));
            }
            /* Processing */
            *((volatile uint32_t *)(NRF_USBD_BASE + 0x800)) = 0x7AC;
            uii &= (uint8_t)*((volatile uint32_t *)(NRF_USBD_BASE + 0x804));
            if (0 != uii)
            {
                uint8_t rb;
                m_simulated_dataepstatus |= ((uint32_t)uii) << USBD_EPIN_BITPOS_0;
                *((volatile uint32_t *)(NRF_USBD_BASE + 0x800)) = 0x7A9;
                *((volatile uint32_t *)(NRF_USBD_BASE + 0x804)) = uii;
                rb = (uint8_t)*((volatile uint32_t *)(NRF_USBD_BASE + 0x804));
            NRF_DRV_USBD_LOG_PROTO1_FIX_PRINTF("   uii: 0x%.2x (0x%.2x)", uii, rb);
            }

            *((volatile uint32_t *)(NRF_USBD_BASE + 0x800)) = 0x7AD;
            uoi &= (uint8_t)*((volatile uint32_t *)(NRF_USBD_BASE + 0x804));
            if (0 != uoi)
            {
                uint8_t rb;
                m_simulated_dataepstatus |= ((uint32_t)uoi) << USBD_EPOUT_BITPOS_0;
                *((volatile uint32_t *)(NRF_USBD_BASE + 0x800)) = 0x7AA;
                *((volatile uint32_t *)(NRF_USBD_BASE + 0x804)) = uoi;
                rb = (uint8_t)*((volatile uint32_t *)(NRF_USBD_BASE + 0x804));
            NRF_DRV_USBD_LOG_PROTO1_FIX_PRINTF("   uoi: 0x%.2u (0x%.2x)", uoi, rb);
            }

            *((volatile uint32_t *)(NRF_USBD_BASE + 0x800)) = 0x7AE;
            usbi &= (uint8_t)*((volatile uint32_t *)(NRF_USBD_BASE + 0x804));
            if (0 != usbi)
            {
                uint8_t rb;
                if (usbi & 0x01)
                {
                    active |= USBD_INTEN_EP0SETUP_Msk;
                }
                if (usbi & 0x10)
                {
                    active |= USBD_INTEN_USBRESET_Msk;
                }
                *((volatile uint32_t *)(NRF_USBD_BASE + 0x800)) = 0x7AB;
                *((volatile uint32_t *)(NRF_USBD_BASE + 0x804)) = usbi;
                rb = (uint8_t)*((volatile uint32_t *)(NRF_USBD_BASE + 0x804));
            NRF_DRV_USBD_LOG_PROTO1_FIX_PRINTF("   usbi: 0x%.2u (0x%.2x)", usbi, rb);
            }

            if (0 != (m_simulated_dataepstatus &
                ~((1U << USBD_EPOUT_BITPOS_0) | (1U << USBD_EPIN_BITPOS_0))))
            {
                active |= enabled & NRF_USBD_INT_DATAEP_MASK;
            }
            if (0 != (m_simulated_dataepstatus &
                ((1U << USBD_EPOUT_BITPOS_0) | (1U << USBD_EPIN_BITPOS_0))))
            {
                if (0 != (enabled & NRF_USBD_INT_EP0DATADONE_MASK))
                {
                    m_simulated_dataepstatus &=
                        ~((1U << USBD_EPOUT_BITPOS_0) | (1U << USBD_EPIN_BITPOS_0));
                    active |= NRF_USBD_INT_EP0DATADONE_MASK;
                }
            }
        }
    }

    /* Process the active interrupts */
    bool setup_active = 0 != (active & NRF_USBD_INT_EP0SETUP_MASK);
    active &= ~NRF_USBD_INT_EP0SETUP_MASK;

    while (active)
    {
        uint8_t event_nr = __CLZ(__RBIT(active));
        m_isr[event_nr]();
        active &= ~(1UL << event_nr);
    }
    usbd_dmareq_process();

    if (setup_active)
    {
        m_isr[USBD_INTEN_EP0SETUP_Pos]();
    }
}

/** @} */
/** @} */

ret_code_t nrf_drv_usbd_init(nrf_drv_usbd_event_handler_t const event_handler)
{
    ASSERT((nrf_drv_usbd_errata_type_52840_eng_a() ||
            nrf_drv_usbd_errata_type_52840_eng_b() || 
            nrf_drv_usbd_errata_type_52840_eng_c() ||
            nrf_drv_usbd_errata_type_52840_eng_d())
          );

    if (NULL == event_handler)
    {
        return NRF_ERROR_INVALID_PARAM;
    }
    if ( m_drv_state != NRFX_DRV_STATE_UNINITIALIZED)
    {
        return NRF_ERROR_INVALID_STATE;
    }

    m_event_handler = event_handler;
    m_drv_state = NRFX_DRV_STATE_INITIALIZED;

    uint8_t n;
    for (n=0; n<NRF_USBD_EPIN_CNT; ++n)
    {
        nrf_drv_usbd_ep_t ep = NRF_DRV_USBD_EPIN(n);
        nrf_drv_usbd_ep_max_packet_size_set(ep, NRF_USBD_EPISO_CHECK(ep) ?
            (NRF_DRV_USBD_ISOSIZE / 2) : NRF_DRV_USBD_EPSIZE);
        usbd_drv_ep_state_t * p_state = ep_state_access(ep);
        p_state->status = NRF_USBD_EP_OK;
        p_state->handler.feeder = NULL;
        p_state->transfer_cnt = 0;
    }
    for (n=0; n<NRF_USBD_EPOUT_CNT; ++n)
    {
        nrf_drv_usbd_ep_t ep = NRF_DRV_USBD_EPOUT(n);
        nrf_drv_usbd_ep_max_packet_size_set(ep, NRF_USBD_EPISO_CHECK(ep) ?
            (NRF_DRV_USBD_ISOSIZE / 2) : NRF_DRV_USBD_EPSIZE);
        usbd_drv_ep_state_t * p_state = ep_state_access(ep);
        p_state->status = NRF_USBD_EP_OK;
        p_state->handler.consumer = NULL;
        p_state->transfer_cnt = 0;
    }

    return NRF_SUCCESS;
}

ret_code_t nrf_drv_usbd_uninit(void)
{
    if (m_drv_state != NRFX_DRV_STATE_INITIALIZED)
    {
        return NRF_ERROR_INVALID_STATE;
    }

    m_event_handler = NULL;
    m_drv_state = NRFX_DRV_STATE_UNINITIALIZED;
    return NRF_SUCCESS;
}

void nrf_drv_usbd_enable(void)
{
    ASSERT(m_drv_state == NRFX_DRV_STATE_INITIALIZED);

    /* Prepare for READY event receiving */
    nrf_usbd_eventcause_clear(NRF_USBD_EVENTCAUSE_READY_MASK);

    if (nrf_drv_usbd_errata_187())
    {
        CRITICAL_REGION_ENTER();
        if (*((volatile uint32_t *)(0x4006EC00)) == 0x00000000)
        {
            *((volatile uint32_t *)(0x4006EC00)) = 0x00009375;
            *((volatile uint32_t *)(0x4006ED14)) = 0x00000003;
            *((volatile uint32_t *)(0x4006EC00)) = 0x00009375;
        }
        else
        {
            *((volatile uint32_t *)(0x4006ED14)) = 0x00000003;
        }
        CRITICAL_REGION_EXIT();
    }
    
    if (nrf_drv_usbd_errata_171())
    {
        CRITICAL_REGION_ENTER();
        if (*((volatile uint32_t *)(0x4006EC00)) == 0x00000000)
        {
            *((volatile uint32_t *)(0x4006EC00)) = 0x00009375;
            *((volatile uint32_t *)(0x4006EC14)) = 0x000000C0;
            *((volatile uint32_t *)(0x4006EC00)) = 0x00009375;
        }
        else
        {
            *((volatile uint32_t *)(0x4006EC14)) = 0x000000C0;
        }
        CRITICAL_REGION_EXIT();
    }

    /* Enable the peripheral */
    nrf_usbd_enable();
    /* Waiting for peripheral to enable, this should take a few us */
    while (0 == (NRF_USBD_EVENTCAUSE_READY_MASK & nrf_usbd_eventcause_get()))
    {
        /* Empty loop */
    }
    nrf_usbd_eventcause_clear(NRF_USBD_EVENTCAUSE_READY_MASK);
    
    if (nrf_drv_usbd_errata_171())
    {
        CRITICAL_REGION_ENTER();
        if (*((volatile uint32_t *)(0x4006EC00)) == 0x00000000)
        {
            *((volatile uint32_t *)(0x4006EC00)) = 0x00009375;
            *((volatile uint32_t *)(0x4006EC14)) = 0x00000000;
            *((volatile uint32_t *)(0x4006EC00)) = 0x00009375;
        }
        else
        {
            *((volatile uint32_t *)(0x4006EC14)) = 0x00000000;
        }

        CRITICAL_REGION_EXIT();
    }

    if (nrf_drv_usbd_errata_166())
    {
        *((volatile uint32_t *)(NRF_USBD_BASE + 0x800)) = 0x7E3;
        *((volatile uint32_t *)(NRF_USBD_BASE + 0x804)) = 0x40;
        __ISB();
        __DSB();
    }

    nrf_usbd_isosplit_set(NRF_USBD_ISOSPLIT_HALF);

    if (USBD_CONFIG_ISO_IN_ZLP)
    {
        nrf_drv_usbd_isoinconfig_set(NRF_USBD_ISOINCONFIG_ZERODATA);
    }
    else
    {
        nrf_drv_usbd_isoinconfig_set(NRF_USBD_ISOINCONFIG_NORESP);
    }

    m_ep_ready = (((1U << NRF_USBD_EPIN_CNT) - 1U) << USBD_EPIN_BITPOS_0);
    m_ep_dma_waiting = 0;
    usbd_dma_pending_clear();
    m_last_setup_dir = NRF_DRV_USBD_EPOUT0;

    m_drv_state = NRFX_DRV_STATE_POWERED_ON;
}

void nrf_drv_usbd_disable(void)
{
    ASSERT(m_drv_state != NRFX_DRV_STATE_UNINITIALIZED);

    /* Stop just in case */
    nrf_drv_usbd_stop();

    /* Disable all parts */
    nrf_usbd_int_disable(nrf_usbd_int_enable_get());
    nrf_usbd_disable();
    usbd_dma_pending_clear();
    m_drv_state = NRFX_DRV_STATE_INITIALIZED;
    
    if (nrf_drv_usbd_errata_187())
    {
        CRITICAL_REGION_ENTER();
        if (*((volatile uint32_t *)(0x4006EC00)) == 0x00000000)
        {
            *((volatile uint32_t *)(0x4006EC00)) = 0x00009375;
            *((volatile uint32_t *)(0x4006ED14)) = 0x00000000;
            *((volatile uint32_t *)(0x4006EC00)) = 0x00009375;
        }
        else
        {
            *((volatile uint32_t *)(0x4006ED14)) = 0x00000000;
        }
        CRITICAL_REGION_EXIT();
    }
}

void nrf_drv_usbd_start(bool enable_sof)
{
    ASSERT(m_drv_state == NRFX_DRV_STATE_POWERED_ON);
    m_bus_suspend = false;

    uint32_t ints_to_enable =
       NRF_USBD_INT_USBRESET_MASK     |
       NRF_USBD_INT_STARTED_MASK      |
       NRF_USBD_INT_ENDEPIN0_MASK     |
       NRF_USBD_INT_EP0DATADONE_MASK  |
       NRF_USBD_INT_ENDEPOUT0_MASK    |
       NRF_USBD_INT_USBEVENT_MASK     |
       NRF_USBD_INT_EP0SETUP_MASK     |
       NRF_USBD_INT_DATAEP_MASK;

   if (enable_sof || nrf_drv_usbd_errata_104())
   {
       ints_to_enable |= NRF_USBD_INT_SOF_MASK;
   }

   /* Enable all required interrupts */
   nrf_usbd_int_enable(ints_to_enable);

   /* Enable interrupt globally */
   NRFX_IRQ_PRIORITY_SET(USBD_IRQn, USBD_CONFIG_IRQ_PRIORITY);
   NRFX_IRQ_ENABLE(USBD_IRQn);

   /* Enable pullups */
   nrf_usbd_pullup_enable();
}

void nrf_drv_usbd_stop(void)
{
    ASSERT(m_drv_state == NRFX_DRV_STATE_POWERED_ON);

    if(NRFX_IRQ_IS_ENABLED(USBD_IRQn))
    {
        /* Abort transfers */
        usbd_ep_abort_all();

        /* Disable pullups */
        nrf_usbd_pullup_disable();

        /* Disable interrupt globally */
        NRFX_IRQ_DISABLE(USBD_IRQn);

        /* Disable all interrupts */
        nrf_usbd_int_disable(~0U);
    }
}

bool nrf_drv_usbd_is_initialized(void)
{
    return (m_drv_state >= NRFX_DRV_STATE_INITIALIZED);
}

bool nrf_drv_usbd_is_enabled(void)
{
    return (m_drv_state >= NRFX_DRV_STATE_POWERED_ON);
}

bool nrf_drv_usbd_is_started(void)
{
    return (nrf_drv_usbd_is_enabled() && NRFX_IRQ_IS_ENABLED(USBD_IRQn));
}

bool nrf_drv_usbd_suspend(void)
{
    bool suspended = false;

    CRITICAL_REGION_ENTER();
    if (m_bus_suspend)
    {
        usbd_ep_abort_all();

        if (!(nrf_usbd_eventcause_get() & NRF_USBD_EVENTCAUSE_RESUME_MASK))
        {
            nrf_usbd_lowpower_enable();
            if (nrf_usbd_eventcause_get() & NRF_USBD_EVENTCAUSE_RESUME_MASK)
            {
                nrf_usbd_lowpower_disable();
            }
            else
            {
                suspended = true;

                if (nrf_drv_usbd_errata_171())
                {
                    if (*((volatile uint32_t *)(0x4006EC00)) == 0x00000000)
                    {
                        *((volatile uint32_t *)(0x4006EC00)) = 0x00009375;
                        *((volatile uint32_t *)(0x4006EC14)) = 0x00000000;
                        *((volatile uint32_t *)(0x4006EC00)) = 0x00009375;
                    }
                    else
                    {
                        *((volatile uint32_t *)(0x4006EC14)) = 0x00000000;
                    }
                }
            }
        }
    }
    CRITICAL_REGION_EXIT();

    return suspended;
}

bool nrf_drv_usbd_wakeup_req(void)
{
    bool started = false;

    CRITICAL_REGION_ENTER();
    if (m_bus_suspend && nrf_usbd_lowpower_check())
    {
        nrf_usbd_lowpower_disable();
        started = true;

        if (nrf_drv_usbd_errata_171())
        {
            if (*((volatile uint32_t *)(0x4006EC00)) == 0x00000000)
            {
                *((volatile uint32_t *)(0x4006EC00)) = 0x00009375;
                *((volatile uint32_t *)(0x4006EC14)) = 0x000000C0;
                *((volatile uint32_t *)(0x4006EC00)) = 0x00009375;
            }
            else
            {
                *((volatile uint32_t *)(0x4006EC14)) = 0x000000C0;
            }

        }
    }
    CRITICAL_REGION_EXIT();

    return started;
}

bool nrf_drv_usbd_suspend_check(void)
{
    return nrf_usbd_lowpower_check();
}

void nrf_drv_usbd_suspend_irq_config(void)
{
    nrf_usbd_int_disable(m_irq_disabled_in_suspend);
}

void nrf_drv_usbd_active_irq_config(void)
{
    nrf_usbd_int_enable(m_irq_disabled_in_suspend);
}

bool nrf_drv_usbd_bus_suspend_check(void)
{
    return m_bus_suspend;
}

void nrf_drv_usbd_force_bus_wakeup(void)
{
    m_bus_suspend = false;
}

void nrf_drv_usbd_ep_max_packet_size_set(nrf_drv_usbd_ep_t ep, uint16_t size)
{
    /* Only power of 2 size allowed */
    ASSERT((size != 0) && (size & (size - 1)) == 0);
    /* Packet size cannot be higher than maximum buffer size */
    ASSERT( ( NRF_USBD_EPISO_CHECK(ep)  && (size <= usbd_ep_iso_capacity(ep)))
           ||
           ((!NRF_USBD_EPISO_CHECK(ep)) && (size <= NRF_DRV_USBD_EPSIZE)));

    usbd_drv_ep_state_t * p_state = ep_state_access(ep);
    p_state->max_packet_size = size;
}

uint16_t nrf_drv_usbd_ep_max_packet_size_get(nrf_drv_usbd_ep_t ep)
{
    usbd_drv_ep_state_t const * p_state = ep_state_access(ep);
    return p_state->max_packet_size;
}

bool nrf_drv_usbd_ep_enable_check(nrf_drv_usbd_ep_t ep)
{
    return nrf_usbd_ep_enable_check(ep_to_hal(ep));
}

void nrf_drv_usbd_ep_enable(nrf_drv_usbd_ep_t ep)
{
    nrf_usbd_int_enable(nrf_drv_usbd_ep_to_int(ep));

    if(nrf_usbd_ep_enable_check(ep))
    {
        return;
    }
    nrf_usbd_ep_enable(ep_to_hal(ep));
    if ((NRF_USBD_EP_NR_GET(ep) != 0) && NRF_USBD_EPOUT_CHECK(ep) && (!NRF_USBD_EPISO_CHECK(ep)))
    {
        CRITICAL_REGION_ENTER();
        nrf_drv_usbd_transfer_out_drop(ep);
        m_ep_dma_waiting &= ~(1U << ep2bit(ep));
        CRITICAL_REGION_EXIT();
    }
}

void nrf_drv_usbd_ep_disable(nrf_drv_usbd_ep_t ep)
{
    usbd_ep_abort(ep);
    nrf_usbd_ep_disable(ep_to_hal(ep));
    nrf_usbd_int_disable(nrf_drv_usbd_ep_to_int(ep));
}

void nrf_drv_usbd_ep_default_config(void)
{
    nrf_usbd_int_disable(
        NRF_USBD_INT_ENDEPIN1_MASK  |
        NRF_USBD_INT_ENDEPIN2_MASK  |
        NRF_USBD_INT_ENDEPIN3_MASK  |
        NRF_USBD_INT_ENDEPIN4_MASK  |
        NRF_USBD_INT_ENDEPIN5_MASK  |
        NRF_USBD_INT_ENDEPIN6_MASK  |
        NRF_USBD_INT_ENDEPIN7_MASK  |
        NRF_USBD_INT_ENDISOIN0_MASK |
        NRF_USBD_INT_ENDEPOUT1_MASK |
        NRF_USBD_INT_ENDEPOUT2_MASK |
        NRF_USBD_INT_ENDEPOUT3_MASK |
        NRF_USBD_INT_ENDEPOUT4_MASK |
        NRF_USBD_INT_ENDEPOUT5_MASK |
        NRF_USBD_INT_ENDEPOUT6_MASK |
        NRF_USBD_INT_ENDEPOUT7_MASK |
        NRF_USBD_INT_ENDISOOUT0_MASK
    );
    nrf_usbd_int_enable(NRF_USBD_INT_ENDEPIN0_MASK | NRF_USBD_INT_ENDEPOUT0_MASK);
    nrf_usbd_ep_all_disable();
}

ret_code_t nrf_drv_usbd_ep_transfer(
    nrf_drv_usbd_ep_t                     ep,
    nrf_drv_usbd_transfer_t const * const p_transfer)
{
    ret_code_t ret;
    const uint8_t ep_bitpos = ep2bit(ep);
    ASSERT(NULL != p_transfer);

    CRITICAL_REGION_ENTER();
    /* Setup data transaction can go only in one direction at a time */
    if ((NRF_USBD_EP_NR_GET(ep) == 0) && (ep != m_last_setup_dir))
    {
        ret = NRF_ERROR_INVALID_ADDR;
        if (NRF_USBD_FAILED_TRANSFERS_DEBUG && (NRF_USBD_ISO_DEBUG || (!NRF_USBD_EPISO_CHECK(ep))))
        {
            NRF_LOG_DEBUG("USB driver: Transfer failed: Invalid EPr\n");
        }
    }
    else if ((m_ep_dma_waiting | ((~m_ep_ready) & USBD_EPIN_BIT_MASK)) & (1U << ep_bitpos))
    {
        /* IN (Device -> Host) transfer has to be transmitted out to allow new transmission */
        ret = NRF_ERROR_BUSY;
        if (NRF_USBD_FAILED_TRANSFERS_DEBUG)
        {
            NRF_LOG_DEBUG("USB driver: Transfer failed: EP is busy");
        }
    }
    else
    {
        usbd_drv_ep_state_t * p_state =  ep_state_access(ep);
        /* Prepare transfer context and handler description */
        nrf_drv_usbd_transfer_t * p_context;
        if (NRF_USBD_EPIN_CHECK(ep))
        {
            p_context = m_ep_feeder_state + NRF_USBD_EP_NR_GET(ep);
            if (nrfx_is_in_ram(p_transfer->p_data.tx))
            {
                /* RAM */
                if (0 == (p_transfer->flags & NRF_DRV_USBD_TRANSFER_ZLP_FLAG))
                {
                    p_state->handler.feeder = nrf_drv_usbd_feeder_ram;
                    if (NRF_USBD_ISO_DEBUG || (!NRF_USBD_EPISO_CHECK(ep)))
                    {
                        NRF_LOG_DEBUG(
                            "USB driver: Transfer called on endpoint %x, size: %u, mode: "
                            "RAM",
                            ep,
                            p_transfer->size);
                    }
                }
                else
                {
                    p_state->handler.feeder = nrf_drv_usbd_feeder_ram_zlp;
                    if (NRF_USBD_ISO_DEBUG || (!NRF_USBD_EPISO_CHECK(ep)))
                    {
                        NRF_LOG_DEBUG(
                            "USB driver: Transfer called on endpoint %x, size: %u, mode: "
                            "RAM_ZLP",
                            ep,
                            p_transfer->size);
                    }
                }
            }
            else
            {
                /* Flash */
                if (0 == (p_transfer->flags & NRF_DRV_USBD_TRANSFER_ZLP_FLAG))
                {
                    p_state->handler.feeder = nrf_drv_usbd_feeder_flash;
                    if (NRF_USBD_ISO_DEBUG || (!NRF_USBD_EPISO_CHECK(ep)))
                    {
                        NRF_LOG_DEBUG(
                            "USB driver: Transfer called on endpoint %x, size: %u, mode: "
                            "FLASH",
                            ep,
                            p_transfer->size);
                    }
                }
                else
                {
                    p_state->handler.feeder = nrf_drv_usbd_feeder_flash_zlp;
                    if (NRF_USBD_ISO_DEBUG || (!NRF_USBD_EPISO_CHECK(ep)))
                    {
                        NRF_LOG_DEBUG(
                            "USB driver: Transfer called on endpoint %x, size: %u, mode: "
                            "FLASH_ZLP",
                            ep,
                            p_transfer->size);
                    }
                }
            }
        }
        else
        {
            p_context = m_ep_consumer_state + NRF_USBD_EP_NR_GET(ep);
            ASSERT((p_transfer->p_data.rx == NULL) || (nrfx_is_in_ram(p_transfer->p_data.rx)));
            p_state->handler.consumer = nrf_drv_usbd_consumer;
        }
        *p_context = *p_transfer;
        p_state->p_context = p_context;

        p_state->transfer_cnt = 0;
        p_state->status    =  NRF_USBD_EP_OK;
        m_ep_dma_waiting   |= 1U << ep_bitpos;
        ret = NRF_SUCCESS;
        usbd_int_rise();
    }
    CRITICAL_REGION_EXIT();
    return ret;
}

ret_code_t nrf_drv_usbd_ep_handled_transfer(
    nrf_drv_usbd_ep_t                         ep,
    nrf_drv_usbd_handler_desc_t const * const p_handler)
{
    ret_code_t ret;
    const uint8_t ep_bitpos = ep2bit(ep);
    ASSERT(NULL != p_handler);

    CRITICAL_REGION_ENTER();
    /* Setup data transaction can go only in one direction at a time */
    if ((NRF_USBD_EP_NR_GET(ep) == 0) && (ep != m_last_setup_dir))
    {
        ret = NRF_ERROR_INVALID_ADDR;
        if (NRF_USBD_FAILED_TRANSFERS_DEBUG && (NRF_USBD_ISO_DEBUG || (!NRF_USBD_EPISO_CHECK(ep))))
        {
            NRF_LOG_DEBUG("USB driver: Transfer failed: Invalid EP");
        }
    }
    else if ((m_ep_dma_waiting | ((~m_ep_ready) & USBD_EPIN_BIT_MASK)) & (1U << ep_bitpos))
    {
        /* IN (Device -> Host) transfer has to be transmitted out to allow a new transmission */
        ret = NRF_ERROR_BUSY;
        if (NRF_USBD_FAILED_TRANSFERS_DEBUG && (NRF_USBD_ISO_DEBUG || (!NRF_USBD_EPISO_CHECK(ep))))
        {
            NRF_LOG_DEBUG("USB driver: Transfer failed: EP is busy");\
        }
    }
    else
    {
        /* Transfer can be configured now */
        usbd_drv_ep_state_t * p_state =  ep_state_access(ep);

        p_state->transfer_cnt = 0;
        p_state->handler   = p_handler->handler;
        p_state->p_context = p_handler->p_context;
        p_state->status    =  NRF_USBD_EP_OK;
        m_ep_dma_waiting   |= 1U << ep_bitpos;

        ret = NRF_SUCCESS;
        if (NRF_USBD_ISO_DEBUG || (!NRF_USBD_EPISO_CHECK(ep)))
        {
            NRF_LOG_DEBUG("USB driver: Transfer called on endpoint %x, mode: Handler", ep);
        }
        usbd_int_rise();
    }
    CRITICAL_REGION_EXIT();
    return ret;
}

void * nrf_drv_usbd_feeder_buffer_get(void)
{
    return m_tx_buffer;
}

ret_code_t nrf_drv_usbd_ep_status_get(nrf_drv_usbd_ep_t ep, size_t * p_size)
{
    ret_code_t ret;

    usbd_drv_ep_state_t const * p_state = ep_state_access(ep);
    CRITICAL_REGION_ENTER();
    *p_size = p_state->transfer_cnt;
    ret = (p_state->handler.consumer == NULL) ? p_state->status : NRF_ERROR_BUSY;
    CRITICAL_REGION_EXIT();
    return ret;
}

size_t     nrf_drv_usbd_epout_size_get(nrf_drv_usbd_ep_t ep)
{
    return nrf_usbd_epout_size_get(ep_to_hal(ep));
}

bool       nrf_drv_usbd_ep_is_busy(nrf_drv_usbd_ep_t ep)
{
    return (0 != ((m_ep_dma_waiting | ((~m_ep_ready) & USBD_EPIN_BIT_MASK)) & (1U << ep2bit(ep))));
}

void nrf_drv_usbd_ep_stall(nrf_drv_usbd_ep_t ep)
{
    NRF_LOG_DEBUG("USB: EP %x stalled.", ep);
    nrf_usbd_ep_stall(ep_to_hal(ep));
}

void nrf_drv_usbd_ep_stall_clear(nrf_drv_usbd_ep_t ep)
{
    if (NRF_USBD_EPOUT_CHECK(ep) && nrf_drv_usbd_ep_stall_check(ep))
    {
        nrf_drv_usbd_transfer_out_drop(ep);
    }
    nrf_usbd_ep_unstall(ep_to_hal(ep));
}

bool nrf_drv_usbd_ep_stall_check(nrf_drv_usbd_ep_t ep)
{
    return nrf_usbd_ep_is_stall(ep_to_hal(ep));
}

void nrf_drv_usbd_ep_dtoggle_clear(nrf_drv_usbd_ep_t ep)
{
    nrf_usbd_dtoggle_set(ep, NRF_USBD_DTOGGLE_DATA0);
}

void nrf_drv_usbd_setup_get(nrf_drv_usbd_setup_t * const p_setup)
{
    memset(p_setup, 0, sizeof(nrf_drv_usbd_setup_t));
    p_setup->bmRequestType = nrf_usbd_setup_bmrequesttype_get();
    p_setup->bmRequest     = nrf_usbd_setup_brequest_get();
    p_setup->wValue        = nrf_usbd_setup_wvalue_get();
    p_setup->wIndex        = nrf_usbd_setup_windex_get();
    p_setup->wLength       = nrf_usbd_setup_wlength_get();
}

void nrf_drv_usbd_setup_data_clear(void)
{
    if (nrf_drv_usbd_errata_104())
    {
        /* For this fix to work properly, it must be ensured that the task is
         * executed twice one after another - blocking ISR. This is however a temporary
         * solution to be used only before production version of the chip. */
        uint32_t primask_copy = __get_PRIMASK();
        __disable_irq();
        nrf_usbd_task_trigger(NRF_USBD_TASK_EP0RCVOUT);
        nrf_usbd_task_trigger(NRF_USBD_TASK_EP0RCVOUT);
        __set_PRIMASK(primask_copy);
    }
    else
    {
        nrf_usbd_task_trigger(NRF_USBD_TASK_EP0RCVOUT);
    }
}

void nrf_drv_usbd_setup_clear(void)
{
    nrf_usbd_task_trigger(NRF_USBD_TASK_EP0STATUS);
}

void nrf_drv_usbd_setup_stall(void)
{
    NRF_LOG_DEBUG("Setup stalled.");
    nrf_usbd_task_trigger(NRF_USBD_TASK_EP0STALL);
}

nrf_drv_usbd_ep_t nrf_drv_usbd_last_setup_dir_get(void)
{
    return m_last_setup_dir;
}

void nrf_drv_usbd_transfer_out_drop(nrf_drv_usbd_ep_t ep)
{
    ASSERT(NRF_USBD_EPOUT_CHECK(ep));

    if (nrf_drv_usbd_errata_200())
    {
        CRITICAL_REGION_ENTER();
        m_ep_ready &= ~(1U << ep2bit(ep));
        *((volatile uint32_t *)(NRF_USBD_BASE + 0x800)) = 0x7C5 + (2u * NRF_USBD_EP_NR_GET(ep));
        *((volatile uint32_t *)(NRF_USBD_BASE + 0x804)) = 0;
        UNUSED_VARIABLE(*((volatile uint32_t *)(NRF_USBD_BASE + 0x804)));
        CRITICAL_REGION_EXIT();
    }
    else
    {
        CRITICAL_REGION_ENTER();
        m_ep_ready &= ~(1U << ep2bit(ep));
        if (!NRF_USBD_EPISO_CHECK(ep))
        {
            nrf_usbd_epout_clear(ep);
        }
        CRITICAL_REGION_EXIT();
    }
}

#endif // NRF_MODULE_ENABLED(USBD)
