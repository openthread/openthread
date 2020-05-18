/*
* Copyright 2019-2020 NXP
* All rights reserved.
*
* SPDX-License-Identifier: BSD-3-Clause
*/

/*==================================================================================================
Include Files
==================================================================================================*/

#include <string.h>

#include "network_utils.h"

#include <openthread/ip6.h>

/*==================================================================================================
Private macros
==================================================================================================*/

/*==================================================================================================
Private type definitions
==================================================================================================*/

/*==================================================================================================
Private prototypes
==================================================================================================*/

/*==================================================================================================
Private global variables declarations
==================================================================================================*/

/*==================================================================================================
Public global variables declarations
==================================================================================================*/

/* RAM global addresses - updated when the device join the network */
otIp6Address in6addr_linklocal_allthreadnodes = {0};
otIp6Address in6addr_realmlocal_allthreadnodes = {0};
otIp6Address in6addr_realmlocal_threadleaderanycast = {0};

/*==================================================================================================
Public functions
==================================================================================================*/
void NWKU_MemCpyReverseOrder(void* pDst, void* pSrc, uint32_t cBytes)
{
    if(cBytes)
    {
        pDst = (uint8_t *)pDst + (uint32_t)(cBytes - 1);

        while (cBytes)
        {
            *((uint8_t *)pDst) = *((uint8_t *)pSrc);
            pDst = (uint8_t *)pDst-1;
            pSrc = (uint8_t *)pSrc+1;
            cBytes--;
        }
    }
}

/*!*************************************************************************************************
\fn     bool_t NWKU_GetBit(uint32_t bitNr, uint8_t *pArray)
\brief  This function returns the value of a bit in an array.

\param  [in]    bitNr           bit number in the whole array
\param  [in]    pArray          pointer to the start of the array

\retval         TRUE            if the bit is set
\retval         FALSE           if the bit is not set
***************************************************************************************************/
bool_t NWKU_GetBit
(
    uint32_t bitNr,
    uint8_t *pArray
)
{
    return ((pArray[bitNr / 8] & (1 << (bitNr % 8))) ? true : false);
}

/*!*************************************************************************************************
\fn     void NWKU_ClearBit(uint32_t bitNr, uint8_t* pArray)
\brief  This function clears a bit in an array.

\param  [in]    bitNr           bit number in the whole array
\param  [in]    pArray          pointer to the start of the array
***************************************************************************************************/
void NWKU_ClearBit
(
    uint32_t bitNr,
    uint8_t *pArray
)
{
    pArray[bitNr / 8] &= ~(1 << (bitNr % 8));
}

/*!*************************************************************************************************
\fn     void NWKU_SetBit(uint32_t bitNr, uint8_t* pArray)
\brief  This function sets a bit in an array.

\param  [in]    bitNr           bit number in the whole array
\param  [in]    pArray          pointer to the start of the array
***************************************************************************************************/
void NWKU_SetBit
(
    uint32_t bitNr,
    uint8_t *pArray
)
{
    pArray[bitNr / 8] |= (1 << (bitNr % 8));
}

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
uint32_t NWKU_GetFirstBitValueInRange
(
    uint8_t *pArray,
    uint32_t lowBitNr,
    uint32_t highBitNr,
    bool_t bitValue
)
{
    for (; lowBitNr < highBitNr; lowBitNr++)
    {
        if (bitValue == NWKU_GetBit(lowBitNr, pArray))
        {
            return lowBitNr;
        }
    }

    return ((uint32_t) - 1); // invalid
}

/*!*************************************************************************************************
\fn     uint32_t getFirstBitValue(uint8_t* pArray, uint32_t arrayBytes, bool_t bitValue)
\brief  This function returns the index of the first bit with value=bitValue.

\param  [in]    pArray          pointer to the start of the array
\param  [in]    arrayBytes      number of bytes in the array
\param  [in]    bitValue        bit value

\return         uint32_t        bit value
***************************************************************************************************/
uint32_t NWKU_GetFirstBitValue
(
    uint8_t *pArray,
    uint32_t arrayBytes,
    bool_t bitValue
)
{
    return NWKU_GetFirstBitValueInRange(pArray, 0, (arrayBytes * 8), bitValue);
}

/*!*************************************************************************************************
\fn    otIp6Address *NWKU_GetSpecificMcastAddr(otInstance *pOtInstance, thrMcastAllThrNodes_t type)
\brief This function is used to get a specific multicast address (Mesh Local All nodes multicast or
       link local All nodes multicast)

\param [in]    pOtInstance         Pointer to the OpenThread instance
\param [in]    type                Ip address type: gMcastLLAddrAllThrNodes_c, gMcastMLAddrAllThrNodes_c

\retval        otIp6Address        Pointer to requested multicast address
***************************************************************************************************/

otIp6Address *NWKU_GetSpecificMcastAddr
(
    otInstance *pOtInstance,
    thrMcastAllThrNodes_t type
)
{
    otIp6Address *pAddr = NULL;
    const otNetifMulticastAddress *pMCastAddr = otIp6GetMulticastAddresses(pOtInstance);

    while(pMCastAddr != NULL)
    {
        if((type == gMcastLLAddrAllThrNodes_c) && (pMCastAddr->mAddress.mFields.m8[1] == 0x32))
        {
            pAddr = (otIp6Address *)&pMCastAddr->mAddress;
            break;
        }
        else if((type == gMcastMLAddrAllThrNodes_c) && (pMCastAddr->mAddress.mFields.m8[1] == 0x33))
        {
            pAddr = (otIp6Address *)&pMCastAddr->mAddress;
            break;
        }

        pMCastAddr = pMCastAddr->mNext;
    }

    return pAddr;
}

/*!*************************************************************************************************
\fn     void APP_OtSetMulticastAddresses(otInstance *pOtInstance)
\brief  This function is used to set the multicast addresses from the stack for application usage.

\param  [in]    pOtInstance      Pointer to OpenThread instance

\return         NONE
***************************************************************************************************/
void NWKU_OtSetMulticastAddresses
(
    otInstance *pOtInstance
)
{
    otIp6Address *pAddr = NULL;

    pAddr = NWKU_GetSpecificMcastAddr(pOtInstance, gMcastLLAddrAllThrNodes_c);

    if(pAddr != NULL)
    {
        memcpy(&in6addr_linklocal_allthreadnodes, pAddr, sizeof(otIp6Address));
    }

    pAddr = NWKU_GetSpecificMcastAddr(pOtInstance, gMcastMLAddrAllThrNodes_c);

    if(pAddr != NULL)
    {
    	memcpy(&in6addr_realmlocal_allthreadnodes, pAddr, sizeof(otIp6Address));
    }
}

/*==================================================================================================
Private functions
==================================================================================================*/

/*==================================================================================================
Private debug functions
==================================================================================================*/
