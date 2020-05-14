/*
* Copyright 2019-2020 NXP
* All rights reserved.
*
* SPDX-License-Identifier: BSD-3-Clause
*/

#ifndef _NETWORK_UTILS_H
#define _NETWORK_UTILS_H

/*!
\file       network_utils.h
\brief      This is a header file for the Network Utils module.
*/

/*==================================================================================================
Include Files
==================================================================================================*/
#include <openthread/ip6.h>

typedef bool bool_t;

/*! Macro for IP address copy */
#define IP_AddrCopy(dst, src) \
        (dst)->m32[0] = (src)->m32[0]; \
        (dst)->m32[1] = (src)->m32[1]; \
        (dst)->m32[2] = (src)->m32[2]; \
        (dst)->m32[3] = (src)->m32[3];

/*! Macro for IPV6 address comparison */
#define IP_IsAddrEqual(addr1, addr2) \
        (((addr1)->m32[0] == (addr2)->m32[0]) && \
         ((addr1)->m32[1] == (addr2)->m32[1]) && \
         ((addr1)->m32[2] == (addr2)->m32[2]) && \
         ((addr1)->m32[3] == (addr2)->m32[3]))

/* RAM global addresses - updated when the device join the network */
extern otIp6Address in6addr_linklocal_allthreadnodes;
extern otIp6Address in6addr_realmlocal_allthreadnodes;
extern otIp6Address in6addr_realmlocal_threadleaderanycast;

/*==================================================================================================
Public type definitions
==================================================================================================*/
/*! Multicast all thread nodes */
typedef enum thrMcastAllThrNodes_tag
{
    gMcastLLAddrAllThrNodes_c,  /*!< Multicast link local - all thread nodes */
    gMcastMLAddrAllThrNodes_c,  /*!< Multicast mesh local - all thread nodes */
} thrMcastAllThrNodes_t;


#ifdef __cplusplus
extern "C" {
#endif

/*!*************************************************************************************************
\fn    otIp6Address *NWKU_GetSpecificMcastAddr(otInstance *pOtInstance, thrMcastAllThrNodes_t type)
\brief This function is used to get a specific multicast address (Mesh Local All nodes multicast or
       link local All nodes multicast)

\param [in]    pOtInstance         Pointer to the OpenThread instance
\param [in]    type                Ip address type: gMcastLLAddrAllThrNodes_c, gMcastMLAddrAllThrNodes_c

\retval        otIp6Address        Pointer to requested multicast address
***************************************************************************************************/
otIp6Address *NWKU_GetSpecificMcastAddr(otInstance *pOtInstance, thrMcastAllThrNodes_t type);

void NWKU_MemCpyReverseOrder(void* pDst, void* pSrc, uint32_t cBytes);

/*!*************************************************************************************************
\fn     uint32_t NWKU_GetFirstBitValueInRange(uint8_t* pArray, uint32_t lowBitNr, uint32_t
        highBitNr, bool_t bitValue)
\brief  This function returns the first bit with value=bitValue in a range in the array.

\param  [in]    pArray          pointer to the start of the array
\param  [in]    lowBitNr        starting bit number
\param  [in]    highBitNr       ending bit number
\param  [in]    bitValue        bit value

\return         uint32_t        bit number
***************************************************************************************************/
uint32_t NWKU_GetFirstBitValueInRange(uint8_t *pArray, uint32_t lowBitNr, uint32_t highBitNr, bool_t bitValue);

/*!*************************************************************************************************
\fn     void NWKU_ClearBit(uint32_t bitNr, uint8_t* pArray)
\brief  This function clears a bit in an array.

\param  [in]    bitNr           bit number in the whole array
\param  [in]    pArray          pointer to the start of the array
***************************************************************************************************/
void NWKU_ClearBit(uint32_t bitNr, uint8_t *pArray);

/*!*************************************************************************************************
\fn     void NWKU_SetBit(uint32_t bitNr, uint8_t* pArray)
\brief  This function sets a bit in an array.

\param  [in]    bitNr           bit number in the whole array
\param  [in]    pArray          pointer to the start of the array
***************************************************************************************************/
void NWKU_SetBit(uint32_t bitNr, uint8_t *pArray);

/*!*************************************************************************************************
\fn     uint32_t getFirstBitValue(uint8_t* pArray, uint32_t arrayBytes, bool_t bitValue)
\brief  This function returns the index of the first bit with value=bitValue.

\param  [in]    pArray          pointer to the start of the array
\param  [in]    arrayBytes      number of bytes in the array
\param  [in]    bitValue        bit value

\return         uint32_t        bit value
***************************************************************************************************/
uint32_t NWKU_GetFirstBitValue(uint8_t *pArray, uint32_t arrayBytes, bool_t bitValue);

/*!*************************************************************************************************
\fn     void APP_OtSetMulticastAddresses(otInstance *pOtInstance)
\brief  This function is used to set the multicast addresses from the stack for application usage.

\param  [in]    pOtInstance      Pointer to OpenThread instance

\return         NONE
***************************************************************************************************/
void NWKU_OtSetMulticastAddresses(otInstance *pOtInstance);

#ifdef __cplusplus
}
#endif
/*================================================================================================*/
#endif  /* _NETWORK_UTILS_H */
