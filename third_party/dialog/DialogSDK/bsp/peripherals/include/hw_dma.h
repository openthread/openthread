/**
 * \addtogroup BSP
 * \{
 * \addtogroup DEVICES
 * \{
 * \addtogroup DMA
 * \{
 * \brief DMA Controller
 */

/**
 ****************************************************************************************
 *
 * @file hw_dma.h
 *
 * @brief Definition of API for the DMA Low Level Driver.
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

#ifndef HW_DMA_H_
#define HW_DMA_H_

#if dg_configUSE_HW_DMA

#include <stdint.h>
#include <sdk_defs.h>

/*
* ENUMERATION DEFINITIONS
*****************************************************************************************
*/

/**
 * \brief DMA channel number
 *
 */
typedef enum {
        HW_DMA_CHANNEL_0 = 0,   /**< Channel number 0 */
        HW_DMA_CHANNEL_1 = 1,   /**< Channel number 1 */
        HW_DMA_CHANNEL_2 = 2,   /**< Channel number 2 */
        HW_DMA_CHANNEL_3 = 3,   /**< Channel number 3 */
        HW_DMA_CHANNEL_4 = 4,   /**< Channel number 4 */
        HW_DMA_CHANNEL_5 = 5,   /**< Channel number 5 */
        HW_DMA_CHANNEL_6 = 6,   /**< Channel number 6 */
        HW_DMA_CHANNEL_7 = 7,    /**< Channel number 7 */
        HW_DMA_CHANNEL_INVALID = 8 /**< Invalid Channel number */
} HW_DMA_CHANNEL;

/**
 * \brief DMA channel enable/disable
 *
 */
typedef enum {
        HW_DMA_STATE_DISABLED = 0x0,    /**< DMA disabled */
        HW_DMA_STATE_ENABLED = 0x1      /**< DMA enabled */
} HW_DMA_STATE;

/**
 * \brief DMA channel bus width transfer
 *
 */
typedef enum {
        HW_DMA_BW_BYTE = 0x0,           /**< Byte */
        HW_DMA_BW_HALFWORD = 0x2,       /**< Halfword */
        HW_DMA_BW_WORD = 0x4            /**< Word */
} HW_DMA_BW;

/**
 * \brief DMA channel interrupt enable/disable
 *
 */
typedef enum {
        HW_DMA_IRQ_STATE_DISABLED = 0x0,        /**< Disable interrupt on this channel */
        HW_DMA_IRQ_STATE_ENABLED = 0x8          /**< Enable interrupt on this channel */
} HW_DMA_IRQ_STATE;

/**
 * \brief DMA request input multiplexer controlled
 *
 */
typedef enum {
        HW_DMA_DREQ_START = 0x0,        /**< DMA channel starts immediately */
        HW_DMA_DREQ_TRIGGERED = 0x10    /**< DMA channel must be triggered by peripheral DMA request */
} HW_DMA_DREQ;

/**
 * \brief Increment destination address mode
 *
 */
typedef enum {
        HW_DMA_BINC_FALSE = 0x0,        /**< Do not increment */
        HW_DMA_BINC_TRUE  = 0x20        /**< Increment according value of BW */
} HW_DMA_BINC;

/**
 * \brief Increment of source address mode
 *
 */
typedef enum {
        HW_DMA_AINC_FALSE = 0x0,        /**< Do not increment */
        HW_DMA_AINC_TRUE  = 0x40        /**< Increment according value of BW */
} HW_DMA_AINC;

/**
 * \brief Channel mode
 *
 * In normal mode the DMA transfer stops the transfer after
 * length DMAx_LEN_REG.
 * In circular mode the DMA channel repeats the transfer
 * after length DMAx_LEN_REG with the initial register values
 * DMAx_A_START_REG, DMAx_B_START_REG, DMAx_LEN_REG, DMAx_INT_REG.
 *
 * \note only works if DREQ_MODE = 1
 *
 */
typedef enum {
        HW_DMA_MODE_NORMAL = 0x0,       /**< Normal mode */
        HW_DMA_MODE_CIRCULAR = 0x80     /**< Circular mode */
} HW_DMA_MODE;

/**
 * \brief Channel priority
 *
 * Set priority level of DMA channel to determine which DMA
 * channel will be activated in case more than one DMA channel
 * requests DMA.
 *
 */
typedef enum {
        HW_DMA_PRIO_0 = 0x000,  /**< Lowest priority */
        HW_DMA_PRIO_1 = 0x100,  /**< .. */
        HW_DMA_PRIO_2 = 0x200,  /**< .. */
        HW_DMA_PRIO_3 = 0x300,  /**< .. */
        HW_DMA_PRIO_4 = 0x400,  /**< .. */
        HW_DMA_PRIO_5 = 0x500,  /**< .. */
        HW_DMA_PRIO_6 = 0x600,  /**< .. */
        HW_DMA_PRIO_7 = 0x700   /**< Highest priority */
} HW_DMA_PRIO;

/**
 * \brief DMA idle mode
 *
 * In blocking mode the DMA performs a fast back-to-back
 * copy, disabling bus access for any bus master with lower priority.
 * In interrupting mode the DMA inserts a wait cycle after each
 * store allowing the CR16 to steal cycles or cache to perform a
 * burst read.
 *
 * \note if DREQ_MODE = 1, DMA_IDLE does not have any effect
 *
 */
typedef enum {
        HW_DMA_IDLE_BLOCKING_MODE = 0x000,      /**< Blocking mode */
        HW_DMA_IDLE_INTERRUPTING_MODE = 0x800   /**< Interrupting mode */
} HW_DMA_IDLE;

/**
 * \brief DMA init mode
 *
 */
typedef enum {
        HW_DMA_INIT_AX_BX_AY_BY = 0x0000,       /**< DMA performs copy A1 to B1, A2 to B2 */
        HW_DMA_INIT_AX_BX_BY    = 0x1000        /**< DMA performs copy A1 to B1, B2 */
} HW_DMA_INIT;

/**
 * \brief Channel request trigger
 *
 */
typedef enum {
        HW_DMA_TRIG_SPI_RXTX = 0x0,
        HW_DMA_TRIG_SPI2_RXTX = 0x1,
        HW_DMA_TRIG_UART_RXTX = 0x2,
        HW_DMA_TRIG_UART2_RXTX = 0x3,
        HW_DMA_TRIG_I2C_RXTX = 0x4,
        HW_DMA_TRIG_I2C2_RXTX = 0x5,
        HW_DMA_TRIG_USB_RXTX = 0x6,
        HW_DMA_TRIG_I2S_LEFTRIGHT = 0x8,
        HW_DMA_TRIG_PDM_LEFTRIGHT = 0x9,
        HW_DMA_TRIG_FTDF_RXTX = 0xA,
        HW_DMA_TRIG_ECC_RXTX = 0xB,
        HW_DMA_TRIG_ADC = 0xC,
        HW_DMA_TRIG_NONE = 0xF
} HW_DMA_TRIG;

/**
 * \brief DMA channel transfer callback
 *
 * This function is called by the DMA driver when the
 * interrupt is fired.
 *
 * \param [in] user_data transfered data
 * \param [in] len length of transfered data
 *
 */
typedef void (*hw_dma_transfer_cb)(void *user_data, uint16_t len);

/**
 * \brief DMA parameters structure
 *
 */
typedef struct {
        HW_DMA_CHANNEL       channel_number;    /**< DMA Channel Number to be used */
        HW_DMA_BW            bus_width;         /**< Transfer Bus width: 8, 16 or 32 bits */
        HW_DMA_IRQ_STATE     irq_enable;        /**< Enable or disable IRQ generation */
        uint16               irq_nr_of_trans;   /**< Number of transfers before IRQ generation
                                                     set to 0 to fire IRQ after transfer ends */
        HW_DMA_DREQ          dreq_mode;         /**< Start DMA immediately or triggered by peripheral */
        HW_DMA_AINC          a_inc;             /**< Increment of source address */
        HW_DMA_BINC          b_inc;             /**< Increment of destination address */
        HW_DMA_MODE          circular;          /**< Select normal or circular operation */
        HW_DMA_PRIO          dma_prio;          /**< Channel priority from 0 to 7 */
        HW_DMA_IDLE          dma_idle;          /**< Idle mode: blocking or interrupting */
        HW_DMA_INIT          dma_init;          /**< Copy mode: block copy or mem init */
        HW_DMA_TRIG          dma_req_mux;       /**< DMA trigger */
        uint32               src_address;       /**< Source address */
        uint32               dest_address;      /**< Destination address */
        uint16               length;            /**< Number of DMA transfers */
        hw_dma_transfer_cb   callback;          /**< Function to call after irq_nr_of_trans transfers */
        void                 *user_data;        /**< Data to pass to Callback */
} DMA_setup;

/*
* API FUNCTIONS DEFINITIONS
*****************************************************************************************
*/

/**
 * \brief Initialize DMA Channel
 *
 * \param [in] channel_setup pointer to struct of type DMA_Setup
 *
 */
void hw_dma_channel_initialization(DMA_setup *channel_setup);

/**
 * \brief Update DMA source address and length
 *
 * When DMA is configured for some peripheral, it could be enough to setup only source address
 * and data length. Other parameters most likely do not change for same type of transmission
 * for values that ware specified in \p hw_dma_channel_initialization().
 * This function should speed up DMA start time when only address and size changes from previous
 * transmission.
 *
 * \param [in] channel DMA channel number to modify
 * \param [in] addr new source address
 * \param [in] length new data transfer length
 * \param [in] cb function to call after transmission finishes
 *
 */
void hw_dma_channel_update_source(HW_DMA_CHANNEL channel, void* addr, uint16_t length,
                                                                        hw_dma_transfer_cb cb);

/**
 * \brief Update DMA destination address and length
 *
 * When DMA is configured for some peripheral, it could be enough to setup only destination address
 * and data length. Other parameters most likely do not change for same type of transmission
 * for values that ware specified in \p hw_dma_channel_initialization().
 * This function should speed up DMA start time when only address and size changes from previous
 * transmission.
 *
 * \param [in] channel DMA channel number to modify
 * \param [in] addr new source address
 * \param [in] length new data transfer length
 * \param [in] cb function to call after transmission finishes
 *
 */
void hw_dma_channel_update_destination(HW_DMA_CHANNEL channel, void *addr, uint16_t length,
                                                                        hw_dma_transfer_cb cb);

/**
 * \brief Update DMA interrupt trigger index
 *
 * DMA channel can trigger an interrupt after arbitrary transfer has finished.
 * Usually interrupt is triggered after transmission finishes but for cyclic mode,
 * where DMA never stops, it is convenient trigger interrupt at other times.
 * This function allows to specify the number of transfers after which the interrupt is triggered.
 *
 * \param [in] channel DMA channel number to modify
 * \param [in] int_ix Number of transfers until the interrupt is triggered
 *
 */
void hw_dma_channel_update_int_ix(HW_DMA_CHANNEL channel, uint16_t int_ix);

/**
 * \brief Enable or disable a DMA channel
 *
 * \param [in] channel_number DMA Channel Number to start/stop
 * \param [in] dma_on enable/disable DMA channel
 *
 */
void hw_dma_channel_enable(HW_DMA_CHANNEL channel_number, HW_DMA_STATE dma_on);

/**
 * \brief Stop DMA channel if operation is in progress
 *
 * If no transfer is in progress nothing happens.
 * If there is outstanding DMA transfer it will be stopped and
 * callback will be called with count of data already transfered
 *
 * \param [in] channel_number DMA channel number to stop
 *
 */
void hw_dma_channel_stop(HW_DMA_CHANNEL channel_number);

/**
 * \brief Read number of transmitted bytes so far
 *
 * Use this function to see how many bytes were transfered
 * via DMA channel so far. This number can changed very soon.
 *
 * \param [in] channel_number DMA channel number
 *
 * \return number of bytes already transfered (when transfer is in progress),
 *         0 - if transfer is already finished,
 *         undefined if called or not started channel
 *
 */
uint16_t hw_dma_transfered_bytes(HW_DMA_CHANNEL channel_number);

/**
 * \brief Freeze DMA
 *
 */
static inline void hw_dma_freeze(void)
{
        GPREG->SET_FREEZE_REG = REG_MSK(GPREG, SET_FREEZE_REG, FRZ_DMA);
}

/**
 * \brief Unfreeze DMA
 *
 */
static inline void hw_dma_unfreeze(void)
{
        GPREG->RESET_FREEZE_REG = REG_MSK(GPREG, RESET_FREEZE_REG, FRZ_DMA);
}

/**
 * \brief Check if any DMA channel is active.
 *
 * \return true, if a channel is active else false.
 */
bool hw_dma_channel_active(void);

#endif /* dg_configUSE_HW_DMA */

#endif /* HW_DMA_H_ */

/**
 * \}
 * \}
 * \}
 */

