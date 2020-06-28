/*
 * Copyright (c) 2016 - 2019, Nordic Semiconductor ASA
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <nrfx.h>

#if NRFX_CHECK(NRFX_QSPI_ENABLED)

#include <nrfx_qspi.h>

/** @brief Command byte used to read status register. */
#define QSPI_STD_CMD_RDSR 0x05

/** @brief Byte used to mask status register and retrieve the write-in-progess bit. */
#define QSPI_MEM_STATUSREG_WIP_Pos 0x01

/** @brief Default time used in timeout function. */
#define QSPI_DEF_WAIT_TIME_US 10

/** @brief Default number of tries in timeout function. */
#define QSPI_DEF_WAIT_ATTEMPTS 100

/** @brief Control block - driver instance local data. */
typedef struct
{
    nrfx_qspi_handler_t handler;   /**< Handler. */
    nrfx_drv_state_t    state;     /**< Driver state. */
    volatile bool       is_busy;   /**< Flag indicating that an operation is currently being performed. */
    void *              p_context; /**< Driver context used in interrupt. */
} qspi_control_block_t;

static qspi_control_block_t m_cb;

static nrfx_err_t qspi_task_perform(nrf_qspi_task_t task)
{
    // Wait for peripheral
    if (m_cb.is_busy)
    {
        return NRFX_ERROR_BUSY;
    }

    nrf_qspi_event_clear(NRF_QSPI, NRF_QSPI_EVENT_READY);

    if (m_cb.handler)
    {
        m_cb.is_busy = true;
        nrf_qspi_int_enable(NRF_QSPI, NRF_QSPI_INT_READY_MASK);
    }

    nrf_qspi_task_trigger(NRF_QSPI, task);

    if (m_cb.handler == NULL)
    {
        while (!nrf_qspi_event_check(NRF_QSPI, NRF_QSPI_EVENT_READY))
        {};
    }
    return NRFX_SUCCESS;
}

static bool qspi_pins_configure(nrf_qspi_pins_t const * p_config)
{
    // Check if the user set meaningful values to struct fields. If not, return false.
    if ((p_config->sck_pin == NRF_QSPI_PIN_NOT_CONNECTED) ||
        (p_config->csn_pin == NRF_QSPI_PIN_NOT_CONNECTED) ||
        (p_config->io0_pin == NRF_QSPI_PIN_NOT_CONNECTED) ||
        (p_config->io1_pin == NRF_QSPI_PIN_NOT_CONNECTED))
    {
        return false;
    }

    nrf_qspi_pins_set(NRF_QSPI, p_config);

    return true;
}

static nrfx_err_t qspi_ready_wait(void)
{
    bool result;
    NRFX_WAIT_FOR(nrf_qspi_event_check(NRF_QSPI, NRF_QSPI_EVENT_READY),
                                       QSPI_DEF_WAIT_ATTEMPTS,
                                       QSPI_DEF_WAIT_TIME_US,
                                       result);
    if (!result)
    {
        return NRFX_ERROR_TIMEOUT;
    }

    return NRFX_SUCCESS;
}

nrfx_err_t nrfx_qspi_init(nrfx_qspi_config_t const * p_config,
                          nrfx_qspi_handler_t        handler,
                          void *                     p_context)
{
    NRFX_ASSERT(p_config);
    if (m_cb.state != NRFX_DRV_STATE_UNINITIALIZED)
    {
        return NRFX_ERROR_INVALID_STATE;
    }

    if (!qspi_pins_configure(&p_config->pins))
    {
        return NRFX_ERROR_INVALID_PARAM;
    }

    nrf_qspi_xip_offset_set(NRF_QSPI, p_config->xip_offset);
    nrf_qspi_ifconfig0_set(NRF_QSPI, &p_config->prot_if);
    nrf_qspi_ifconfig1_set(NRF_QSPI, &p_config->phy_if);

    m_cb.is_busy = false;
    m_cb.handler = handler;
    m_cb.p_context = p_context;

    /* QSPI interrupt is disabled because the device should be enabled in polling mode (wait for activate
       task event ready)*/
    nrf_qspi_int_disable(NRF_QSPI, NRF_QSPI_INT_READY_MASK);

    if (handler)
    {
        NRFX_IRQ_PRIORITY_SET(QSPI_IRQn, p_config->irq_priority);
        NRFX_IRQ_ENABLE(QSPI_IRQn);
    }

    m_cb.state = NRFX_DRV_STATE_INITIALIZED;

    nrf_qspi_enable(NRF_QSPI);

    nrf_qspi_event_clear(NRF_QSPI, NRF_QSPI_EVENT_READY);
    nrf_qspi_task_trigger(NRF_QSPI, NRF_QSPI_TASK_ACTIVATE);

    // Waiting for the peripheral to activate

    return qspi_ready_wait();
}

nrfx_err_t nrfx_qspi_cinstr_xfer(nrf_qspi_cinstr_conf_t const * p_config,
                                 void const *                   p_tx_buffer,
                                 void *                         p_rx_buffer)
{
    NRFX_ASSERT(m_cb.state != NRFX_DRV_STATE_UNINITIALIZED);

    if (m_cb.is_busy)
    {
        return NRFX_ERROR_BUSY;
    }

    nrf_qspi_event_clear(NRF_QSPI, NRF_QSPI_EVENT_READY);
    /* In some cases, only opcode should be sent. To prevent execution, set function code is
     * surrounded by an if.
     */
    if (p_tx_buffer)
    {
        nrf_qspi_cinstrdata_set(NRF_QSPI, p_config->length, p_tx_buffer);
    }

    nrf_qspi_int_disable(NRF_QSPI, NRF_QSPI_INT_READY_MASK);

    nrf_qspi_cinstr_transfer_start(NRF_QSPI, p_config);

    if (qspi_ready_wait() == NRFX_ERROR_TIMEOUT)
    {
        // This timeout should never occur when WIPWAIT is not active, since in this
        // case the QSPI peripheral should send the command immediately, without any
        // waiting for previous write to complete.
        NRFX_ASSERT(p_config->wipwait);

        return NRFX_ERROR_TIMEOUT;
    }
    nrf_qspi_event_clear(NRF_QSPI, NRF_QSPI_EVENT_READY);

    if (p_rx_buffer)
    {
        nrf_qspi_cinstrdata_get(NRF_QSPI, p_config->length, p_rx_buffer);
    }

    return NRFX_SUCCESS;
}

nrfx_err_t nrfx_qspi_cinstr_quick_send(uint8_t               opcode,
                                       nrf_qspi_cinstr_len_t length,
                                       void const *          p_tx_buffer)
{
    nrf_qspi_cinstr_conf_t config = NRFX_QSPI_DEFAULT_CINSTR(opcode, length);
    return nrfx_qspi_cinstr_xfer(&config, p_tx_buffer, NULL);
}

nrfx_err_t nrfx_qspi_lfm_start(nrf_qspi_cinstr_conf_t const * p_config)
{
    NRFX_ASSERT(m_cb.state != NRFX_DRV_STATE_UNINITIALIZED);
    NRFX_ASSERT(!(nrf_qspi_cinstr_long_transfer_is_ongoing(NRF_QSPI)));
    NRFX_ASSERT(p_config->length == NRF_QSPI_CINSTR_LEN_1B);

    if (m_cb.is_busy)
    {
        return NRFX_ERROR_BUSY;
    }

    nrf_qspi_cinstr_long_transfer_start(NRF_QSPI, p_config);

    if (qspi_ready_wait() == NRFX_ERROR_TIMEOUT)
    {
        /* In case of error, abort long frame mode */
        nrf_qspi_cinstr_long_transfer_continue(NRF_QSPI, NRF_QSPI_CINSTR_LEN_1B, true);
        return NRFX_ERROR_TIMEOUT;
    }

    m_cb.is_busy = true;
    return NRFX_SUCCESS;
}

nrfx_err_t nrfx_qspi_lfm_xfer(void const * p_tx_buffer,
                              void *       p_rx_buffer,
                              size_t       transfer_length,
                              bool         finalize)
{
    NRFX_ASSERT(m_cb.state != NRFX_DRV_STATE_UNINITIALIZED);
    NRFX_ASSERT(nrf_qspi_cinstr_long_transfer_is_ongoing(NRF_QSPI));

    nrfx_err_t status = NRFX_SUCCESS;

    /* Perform transfers in packets of 8 bytes. Last transfer may be shorter. */
    nrf_qspi_cinstr_len_t length = NRF_QSPI_CINSTR_LEN_9B;
    for (uint32_t curr_byte = 0; curr_byte < transfer_length; curr_byte += 8)
    {
        uint32_t remaining_bytes = transfer_length - curr_byte;
        if (remaining_bytes < 8)
        {
            length = (nrf_qspi_cinstr_len_t)(remaining_bytes + 1);
        }

        if (p_tx_buffer)
        {
            nrf_qspi_cinstrdata_set(NRF_QSPI,
                                    length,
                                    &((uint8_t const *)p_tx_buffer)[curr_byte]);
        }

        nrf_qspi_event_clear(NRF_QSPI, NRF_QSPI_EVENT_READY);

        if (remaining_bytes <= 8)
        {
            nrf_qspi_cinstr_long_transfer_continue(NRF_QSPI, length, finalize);
        }
        else
        {
            nrf_qspi_cinstr_long_transfer_continue(NRF_QSPI, length, false);
        }

        if (qspi_ready_wait() == NRFX_ERROR_TIMEOUT)
        {
            /* In case of error, abort long frame mode */
            nrf_qspi_cinstr_long_transfer_continue(NRF_QSPI, NRF_QSPI_CINSTR_LEN_1B, true);
            status = NRFX_ERROR_TIMEOUT;
            break;
        }

        if (p_rx_buffer)
        {
            nrf_qspi_cinstrdata_get(NRF_QSPI,
                                    length,
                                    &((uint8_t *)p_rx_buffer)[curr_byte]);
        }
    }
    nrf_qspi_event_clear(NRF_QSPI, NRF_QSPI_EVENT_READY);

    if ((finalize) || (status == NRFX_ERROR_TIMEOUT))
    {
        m_cb.is_busy = false;
    }

    return status;
}

nrfx_err_t nrfx_qspi_mem_busy_check(void)
{
    nrfx_err_t ret_code;
    uint8_t status_value = 0;

    nrf_qspi_cinstr_conf_t const config =
        NRFX_QSPI_DEFAULT_CINSTR(QSPI_STD_CMD_RDSR,
                                 NRF_QSPI_CINSTR_LEN_2B);
    ret_code = nrfx_qspi_cinstr_xfer(&config, &status_value, &status_value);

    if (ret_code != NRFX_SUCCESS)
    {
        return ret_code;
    }

    if ((status_value & QSPI_MEM_STATUSREG_WIP_Pos) != 0x00)
    {
        return NRFX_ERROR_BUSY;
    }

    return NRFX_SUCCESS;
}

void nrfx_qspi_uninit(void)
{
    NRFX_ASSERT(m_cb.state != NRFX_DRV_STATE_UNINITIALIZED);

    if (nrf_qspi_cinstr_long_transfer_is_ongoing(NRF_QSPI))
    {
        nrf_qspi_cinstr_long_transfer_continue(NRF_QSPI, NRF_QSPI_CINSTR_LEN_1B, true);
    }

    nrf_qspi_int_disable(NRF_QSPI, NRF_QSPI_INT_READY_MASK);

    nrf_qspi_task_trigger(NRF_QSPI, NRF_QSPI_TASK_DEACTIVATE);

    nrf_qspi_disable(NRF_QSPI);

    NRFX_IRQ_DISABLE(QSPI_IRQn);

    nrf_qspi_event_clear(NRF_QSPI, NRF_QSPI_EVENT_READY);

    m_cb.state = NRFX_DRV_STATE_UNINITIALIZED;
}

nrfx_err_t nrfx_qspi_write(void const * p_tx_buffer,
                           size_t       tx_buffer_length,
                           uint32_t     dst_address)
{
    NRFX_ASSERT(m_cb.state != NRFX_DRV_STATE_UNINITIALIZED);
    NRFX_ASSERT(p_tx_buffer != NULL);

    if (!nrfx_is_in_ram(p_tx_buffer) || !nrfx_is_word_aligned(p_tx_buffer))
    {
        return NRFX_ERROR_INVALID_ADDR;
    }

    nrf_qspi_write_buffer_set(NRF_QSPI, p_tx_buffer, tx_buffer_length, dst_address);
    return qspi_task_perform(NRF_QSPI_TASK_WRITESTART);
}

nrfx_err_t nrfx_qspi_read(void *   p_rx_buffer,
                          size_t   rx_buffer_length,
                          uint32_t src_address)
{
    NRFX_ASSERT(m_cb.state != NRFX_DRV_STATE_UNINITIALIZED);
    NRFX_ASSERT(p_rx_buffer != NULL);

    if (!nrfx_is_in_ram(p_rx_buffer) || !nrfx_is_word_aligned(p_rx_buffer))
    {
        return NRFX_ERROR_INVALID_ADDR;
    }

    nrf_qspi_read_buffer_set(NRF_QSPI, p_rx_buffer, rx_buffer_length, src_address);
    return qspi_task_perform(NRF_QSPI_TASK_READSTART);
}

nrfx_err_t nrfx_qspi_erase(nrf_qspi_erase_len_t length,
                           uint32_t             start_address)
{
    NRFX_ASSERT(m_cb.state != NRFX_DRV_STATE_UNINITIALIZED);

    if (!nrfx_is_word_aligned((void const *)start_address))
    {
        return NRFX_ERROR_INVALID_ADDR;
    }

    nrf_qspi_erase_ptr_set(NRF_QSPI, start_address, length);
    return qspi_task_perform(NRF_QSPI_TASK_ERASESTART);
}

nrfx_err_t nrfx_qspi_chip_erase(void)
{
    return nrfx_qspi_erase(NRF_QSPI_ERASE_LEN_ALL, 0);
}

void nrfx_qspi_irq_handler(void)
{
    // Catch Event ready interrupts
    if (nrf_qspi_event_check(NRF_QSPI, NRF_QSPI_EVENT_READY))
    {
        m_cb.is_busy = false;
        nrf_qspi_event_clear(NRF_QSPI, NRF_QSPI_EVENT_READY);
        m_cb.handler(NRFX_QSPI_EVENT_DONE, m_cb.p_context);
    }
}

#endif // NRFX_CHECK(NRFX_QSPI_ENABLED)
