/*! *********************************************************************************
* Copyright (c) 2015, Freescale Semiconductor, Inc.
* Copyright 2016-2017 NXP
* All rights reserved.
*
* \file
*
* SPDX-License-Identifier: BSD-3-Clause
********************************************************************************** */

#ifndef __UART_ADAPTER_H__
#define __UART_ADAPTER_H__

/*! *********************************************************************************
*************************************************************************************
* Public macros
*************************************************************************************
********************************************************************************** */
#ifndef gUartIsrPrio_c
#define gUartIsrPrio_c (0x40)
#endif


/*! *********************************************************************************
*************************************************************************************
* Public type definitions
*************************************************************************************
********************************************************************************** */
typedef struct uartState_tag uartState_t;

typedef void (*uartCallback_t)(uartState_t* state);

struct uartState_tag {
    uartCallback_t txCb;
    uartCallback_t rxCb;
    uint32_t txCbParam;
    uint32_t rxCbParam;
    uint8_t *pTxData;
    uint8_t *pRxData;
    volatile uint32_t txSize;
    volatile uint32_t rxSize;
};

enum uartStatus_tag {
    gUartSuccess_c,
    gUartInvalidParameter_c,
    gUartBusy_c
};

/*! *********************************************************************************
*************************************************************************************
* Public functions
*************************************************************************************
********************************************************************************** */
uint32_t UART_Initialize(uint32_t instance, uartState_t *pState);
uint32_t UART_SetBaudrate(uint32_t instance, uint32_t baudrate);
uint32_t UART_SendData(uint32_t instance, uint8_t* pData, uint32_t size);
uint32_t UART_ReceiveData(uint32_t instance, uint8_t* pData, uint32_t size);
uint32_t UART_InstallRxCalback(uint32_t instance, uartCallback_t cb, uint32_t cbParam);
uint32_t UART_InstallTxCalback(uint32_t instance, uartCallback_t cb, uint32_t cbParam);
uint32_t UART_IsTxActive(uint32_t instance);
uint32_t UART_EnableLowPowerWakeup(uint32_t instance);
uint32_t UART_DisableLowPowerWakeup(uint32_t instance);
uint32_t UART_IsWakeupSource(uint32_t instance);

uint32_t LPUART_Initialize(uint32_t instance, uartState_t *pState);
uint32_t LPUART_SetBaudrate(uint32_t instance, uint32_t baudrate);
uint32_t LPUART_SendData(uint32_t instance, uint8_t* pData, uint32_t size);
uint32_t LPUART_ReceiveData(uint32_t instance, uint8_t* pData, uint32_t size);
uint32_t LPUART_InstallRxCalback(uint32_t instance, uartCallback_t cb, uint32_t cbParam);
uint32_t LPUART_InstallTxCalback(uint32_t instance, uartCallback_t cb, uint32_t cbParam);
uint32_t LPUART_IsTxActive(uint32_t instance);
uint32_t LPUART_EnableLowPowerWakeup(uint32_t instance);
uint32_t LPUART_DisableLowPowerWakeup(uint32_t instance);
uint32_t LPUART_IsWakeupSource(uint32_t instance);

uint32_t LPSCI_Initialize(uint32_t instance, uartState_t *pState);
uint32_t LPSCI_SetBaudrate(uint32_t instance, uint32_t baudrate);
uint32_t LPSCI_SendData(uint32_t instance, uint8_t* pData, uint32_t size);
uint32_t LPSCI_ReceiveData(uint32_t instance, uint8_t* pData, uint32_t size);
uint32_t LPSCI_InstallRxCalback(uint32_t instance, uartCallback_t cb, uint32_t cbParam);
uint32_t LPSCI_InstallTxCalback(uint32_t instance, uartCallback_t cb, uint32_t cbParam);
uint32_t LPSCI_IsTxActive(uint32_t instance);
uint32_t LPSCI_EnableLowPowerWakeup(uint32_t instance);
uint32_t LPSCI_DisableLowPowerWakeup(uint32_t instance);
uint32_t LPSCI_IsWakeupSource(uint32_t instance);

uint32_t USART_Initialize(uint32_t instance, uartState_t *pState);
uint32_t USART_SetBaudrate(uint32_t instance, uint32_t baudrate);
uint32_t USART_SendData(uint32_t instance, uint8_t *pData, uint32_t size);
uint32_t USART_ReceiveData(uint32_t instance, uint8_t *pData, uint32_t size);
uint32_t USART_InstallRxCalback(uint32_t instance, uartCallback_t cb, uint32_t cbParam);
uint32_t USART_InstallTxCalback(uint32_t instance, uartCallback_t cb, uint32_t cbParam);
uint32_t USART_IsTxActive(uint32_t instance);
uint32_t USART_EnableLowPowerWakeup(uint32_t instance);
uint32_t USART_DisableLowPowerWakeup(uint32_t instance);
uint32_t USART_IsWakeupSource(uint32_t instance);
#endif /* __UART_ADAPTER_H__ */