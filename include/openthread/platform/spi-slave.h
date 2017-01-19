/*
 *  Copyright (c) 2016, The OpenThread Authors.
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

/**
 * @file spi-slave.h
 * @brief
 *   This file includes the platform abstraction for SPI slave communication.
 */

#ifndef SPI_SLAVE_H_
#define SPI_SLAVE_H_

#include <stdint.h>

#include "openthread/types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup spi-slave SPI Slave
 * @ingroup platform
 *
 * @brief
 *   This module includes the platform abstraction for SPI slave communication.
 *
 * @{
 *
 */

/**
 * Indicates that a SPI transaction has completed with
 * the given length. The data written to the slave has been
 * written to the pointer indicated by the `anInputBuf` argument
 * to the previous call to otPlatSpiSlavePrepareTransaction().
 *
 * Once this function is called, otPlatSpiSlavePrepareTransaction()
 * is invalid and must be called again for the next transaction to be
 * valid.
 *
 * Note that this function is always called at the end of a transaction,
 * even if otPlatSpiSlavePrepareTransaction() has not yet been called.
 * In such cases, anOutputBufLen and anInputBufLen will be zero.
 *
 * @param[in] aContext           Context pointer passed into
 *                               otPlatSpiSlaveEnable()
 * @param[in] anOutputBuf        Value of anOutputBuf from last call to
 *                               otPlatSpiSlavePrepareTransaction()
 * @param[in] anOutputBufLen     Value of anOutputBufLen from last call to
 *                               otPlatSpiSlavePrepareTransaction()
 * @param[in] anInputBuf         Value of anInputBuf from last call to
 *                               otPlatSpiSlavePrepareTransaction()
 * @param[in] anInputBufLen      Value of anInputBufLen from last call to
 *                               otPlatSpiSlavePrepareTransaction()
 * @param[in] aTransactionLength Length of the completed transaction, in bytes
 */
typedef void (*otPlatSpiSlaveTransactionCompleteCallback)(
    void *aContext,
    uint8_t *anOutputBuf,
    uint16_t anOutputBufLen,
    uint8_t *anInputBuf,
    uint16_t anInputBufLen,
    uint16_t aTransactionLength
);

/**
 * Initialize the SPI slave interface.

 * Note that the SPI slave is not fully ready until a transaction is
 * prepared using otPlatSPISlavePrepareTransaction().
 *
 * If otPlatSPISlavePrepareTransaction() is not called before
 * the master begins a transaction, the resulting SPI transaction
 * will send all `0xFF` bytes and discard all received bytes.
 *
 * @param[in] aCallback Pointer to transaction complete callback
 * @param[in] aContext  Context pointer to be passed to transaction
 *                      complete callback
 *
 * @retval ::kThreadError_None    Successfully enabled the SPI Slave interface.
 * @retval ::kThreadError_Already SPI Slave interface is already enabled.
 * @retval ::kThreadError_Failed  Failed to enable the SPI Slave interface.
 */
ThreadError otPlatSpiSlaveEnable(
    otPlatSpiSlaveTransactionCompleteCallback aCallback,
    void *aContext
);

/**
 * Shutdown and disable the SPI slave interface.
 */
void otPlatSpiSlaveDisable(void);

/**
 * Prepare data for the next SPI transaction. Data pointers
 * MUST remain valid until the transaction complete callback
 * is called by the SPI slave driver, or until after the
 * next call to otPlatSpiSlavePrepareTransaction().
 *
 * This function may be called more than once before the SPI
 * master initiates the transaction. Each *successful* call to this
 * function will cause the previous values from earlier calls to
 * be discarded.
 *
 * Not calling this function after a completed transaction is the
 * same as if this function was previously called with both buffer
 * lengths set to zero and aRequestTransactionFlag set to `false`.
 *
 * Once anOutputBufLen bytes of anOutputBuf has been clocked out, the
 * MISO pin shall be set high until the master finishes the SPI
 * transaction. This is the functional equivalent of padding the end
 * of anOutputBuf with 0xFF bytes out to the length of the transaction.
 *
 * Once anInputBufLen bytes of anInputBuf have been clocked in from
 * MOSI, all subsequent values from the MOSI pin are ignored until the
 * SPI master finishes the transaction.
 *
 * Note that even if `anInputBufLen` or `anOutputBufLen` (or both) are
 * exhausted before the SPI master finishes a transaction, the ongoing
 * size of the transaction must still be kept track of to be passed
 * to the transaction complete callback. For example, if `anInputBufLen`
 * is equal to 10 and `anOutputBufLen` equal to 20 and the SPI master
 * clocks out 30 bytes, the value 30 is passed to the transaction
 * complete callback.
 *
 * Any call to this function while a transaction is in progress will
 * cause all of the arguments to be ignored and the return value to
 * be ::kThreadError_Busy.
 *
 * @param[in] anOutputBuf    Data to be written to MISO pin
 * @param[in] anOutputBufLen Size of the output buffer, in bytes
 * @param[in] anInputBuf     Data to be read from MOSI pin
 * @param[in] anInputBufLen  Size of the input buffer, in bytes
 * @param[in] aRequestTransactionFlag Set to true if host interrupt should be set
 *
 * @retval ::kThreadError_None         Transaction was successfully prepared.
 * @retval ::kThreadError_Busy         A transaction is currently in progress.
 * @retval ::kThreadError_InvalidState otPlatSpiSlaveEnable() hasn't been called.
 */
ThreadError otPlatSpiSlavePrepareTransaction(
    uint8_t *anOutputBuf,
    uint16_t anOutputBufLen,
    uint8_t *anInputBuf,
    uint16_t anInputBufLen,
    bool aRequestTransactionFlag
);

/**
 * @}
 *
 */

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // SPI_SLAVE_H_
