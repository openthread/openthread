/*
 * Copyright (c) 2017 - 2019, Nordic Semiconductor ASA
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

#ifndef NRF_USBD_H__
#define NRF_USBD_H__

#include <nrfx.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup nrf_usbd_hal USBD HAL
 * @{
 * @ingroup nrf_usbd
 * @brief   Hardware access layer for managing the Universal Serial Bus Device (USBD)
 *          peripheral.
 */

/** @brief USBD tasks. */
typedef enum
{
    NRF_USBD_TASK_STARTEPIN0    = offsetof(NRF_USBD_Type, TASKS_STARTEPIN[0] ), /**< Captures the EPIN[0].PTR, EPIN[0].MAXCNT, and EPIN[0].CONFIG registers values, and enables control endpoint IN 0 to respond to traffic from host. */
    NRF_USBD_TASK_STARTEPIN1    = offsetof(NRF_USBD_Type, TASKS_STARTEPIN[1] ), /**< Captures the EPIN[1].PTR, EPIN[1].MAXCNT, and EPIN[1].CONFIG registers values, and enables data endpoint IN 1 to respond to traffic from host. */
    NRF_USBD_TASK_STARTEPIN2    = offsetof(NRF_USBD_Type, TASKS_STARTEPIN[2] ), /**< Captures the EPIN[2].PTR, EPIN[2].MAXCNT, and EPIN[2].CONFIG registers values, and enables data endpoint IN 2 to respond to traffic from host. */
    NRF_USBD_TASK_STARTEPIN3    = offsetof(NRF_USBD_Type, TASKS_STARTEPIN[3] ), /**< Captures the EPIN[3].PTR, EPIN[3].MAXCNT, and EPIN[3].CONFIG registers values, and enables data endpoint IN 3 to respond to traffic from host. */
    NRF_USBD_TASK_STARTEPIN4    = offsetof(NRF_USBD_Type, TASKS_STARTEPIN[4] ), /**< Captures the EPIN[4].PTR, EPIN[4].MAXCNT, and EPIN[4].CONFIG registers values, and enables data endpoint IN 4 to respond to traffic from host. */
    NRF_USBD_TASK_STARTEPIN5    = offsetof(NRF_USBD_Type, TASKS_STARTEPIN[5] ), /**< Captures the EPIN[5].PTR, EPIN[5].MAXCNT, and EPIN[5].CONFIG registers values, and enables data endpoint IN 5 to respond to traffic from host. */
    NRF_USBD_TASK_STARTEPIN6    = offsetof(NRF_USBD_Type, TASKS_STARTEPIN[6] ), /**< Captures the EPIN[6].PTR, EPIN[6].MAXCNT, and EPIN[6].CONFIG registers values, and enables data endpoint IN 6 to respond to traffic from host. */
    NRF_USBD_TASK_STARTEPIN7    = offsetof(NRF_USBD_Type, TASKS_STARTEPIN[7] ), /**< Captures the EPIN[7].PTR, EPIN[7].MAXCNT, and EPIN[7].CONFIG registers values, and enables data endpoint IN 7 to respond to traffic from host. */
    NRF_USBD_TASK_STARTISOIN    = offsetof(NRF_USBD_Type, TASKS_STARTISOIN   ), /**< Captures the ISOIN.PTR, ISOIN.MAXCNT, and ISOIN.CONFIG registers values, and enables sending data on ISO endpoint 8. */
    NRF_USBD_TASK_STARTEPOUT0   = offsetof(NRF_USBD_Type, TASKS_STARTEPOUT[0]), /**< Captures the EPOUT[0].PTR, EPOUT[0].MAXCNT, and EPOUT[0].CONFIG registers values, and enables control endpoint 0 to respond to traffic from host. */
    NRF_USBD_TASK_STARTEPOUT1   = offsetof(NRF_USBD_Type, TASKS_STARTEPOUT[1]), /**< Captures the EPOUT[1].PTR, EPOUT[1].MAXCNT, and EPOUT[1].CONFIG registers values, and enables data endpoint 1 to respond to traffic from host. */
    NRF_USBD_TASK_STARTEPOUT2   = offsetof(NRF_USBD_Type, TASKS_STARTEPOUT[2]), /**< Captures the EPOUT[2].PTR, EPOUT[2].MAXCNT, and EPOUT[2].CONFIG registers values, and enables data endpoint 2 to respond to traffic from host. */
    NRF_USBD_TASK_STARTEPOUT3   = offsetof(NRF_USBD_Type, TASKS_STARTEPOUT[3]), /**< Captures the EPOUT[3].PTR, EPOUT[3].MAXCNT, and EPOUT[3].CONFIG registers values, and enables data endpoint 3 to respond to traffic from host. */
    NRF_USBD_TASK_STARTEPOUT4   = offsetof(NRF_USBD_Type, TASKS_STARTEPOUT[4]), /**< Captures the EPOUT[4].PTR, EPOUT[4].MAXCNT, and EPOUT[4].CONFIG registers values, and enables data endpoint 4 to respond to traffic from host. */
    NRF_USBD_TASK_STARTEPOUT5   = offsetof(NRF_USBD_Type, TASKS_STARTEPOUT[5]), /**< Captures the EPOUT[5].PTR, EPOUT[5].MAXCNT, and EPOUT[5].CONFIG registers values, and enables data endpoint 5 to respond to traffic from host. */
    NRF_USBD_TASK_STARTEPOUT6   = offsetof(NRF_USBD_Type, TASKS_STARTEPOUT[6]), /**< Captures the EPOUT[6].PTR, EPOUT[6].MAXCNT, and EPOUT[6].CONFIG registers values, and enables data endpoint 6 to respond to traffic from host. */
    NRF_USBD_TASK_STARTEPOUT7   = offsetof(NRF_USBD_Type, TASKS_STARTEPOUT[7]), /**< Captures the EPOUT[7].PTR, EPOUT[7].MAXCNT, and EPOUT[7].CONFIG registers values, and enables data endpoint 7 to respond to traffic from host. */
    NRF_USBD_TASK_STARTISOOUT   = offsetof(NRF_USBD_Type, TASKS_STARTISOOUT  ), /**< Captures the ISOOUT.PTR, ISOOUT.MAXCNT, and ISOOUT.CONFIG registers values, and enables receiving of data on ISO endpoint 8. */
    NRF_USBD_TASK_EP0RCVOUT     = offsetof(NRF_USBD_Type, TASKS_EP0RCVOUT    ), /**< Allows OUT data stage on the control endpoint 0. */
    NRF_USBD_TASK_EP0STATUS     = offsetof(NRF_USBD_Type, TASKS_EP0STATUS    ), /**< Allows status stage on the control endpoint 0. */
    NRF_USBD_TASK_EP0STALL      = offsetof(NRF_USBD_Type, TASKS_EP0STALL     ), /**< STALLs data and status stage on the control endpoint 0. */
    NRF_USBD_TASK_DRIVEDPDM     = offsetof(NRF_USBD_Type, TASKS_DPDMDRIVE    ), /**< Forces D+ and D-lines to the state defined in the DPDMVALUE register. */
    NRF_USBD_TASK_NODRIVEDPDM   = offsetof(NRF_USBD_Type, TASKS_DPDMNODRIVE  ), /**< Stops forcing D+ and D- lines to any state (USB engine takes control). */
}nrf_usbd_task_t;

/** @brief USBD events. */
typedef enum
{
    NRF_USBD_EVENT_USBRESET      = offsetof(NRF_USBD_Type, EVENTS_USBRESET   ), /**< Signals that a USB reset condition is detected on the USB lines. */
    NRF_USBD_EVENT_STARTED       = offsetof(NRF_USBD_Type, EVENTS_STARTED    ), /**< Confirms that the EPIN[n].PTR, EPIN[n].MAXCNT, EPIN[n].CONFIG, or EPOUT[n].PTR, EPOUT[n].MAXCNT, and EPOUT[n].CONFIG registers have been captured on all endpoints reported in the EPSTATUS register. */
    NRF_USBD_EVENT_ENDEPIN0      = offsetof(NRF_USBD_Type, EVENTS_ENDEPIN[0] ), /**< The whole EPIN[0] buffer has been consumed. The RAM buffer can be accessed safely by software. */
    NRF_USBD_EVENT_ENDEPIN1      = offsetof(NRF_USBD_Type, EVENTS_ENDEPIN[1] ), /**< The whole EPIN[1] buffer has been consumed. The RAM buffer can be accessed safely by software. */
    NRF_USBD_EVENT_ENDEPIN2      = offsetof(NRF_USBD_Type, EVENTS_ENDEPIN[2] ), /**< The whole EPIN[2] buffer has been consumed. The RAM buffer can be accessed safely by software. */
    NRF_USBD_EVENT_ENDEPIN3      = offsetof(NRF_USBD_Type, EVENTS_ENDEPIN[3] ), /**< The whole EPIN[3] buffer has been consumed. The RAM buffer can be accessed safely by software. */
    NRF_USBD_EVENT_ENDEPIN4      = offsetof(NRF_USBD_Type, EVENTS_ENDEPIN[4] ), /**< The whole EPIN[4] buffer has been consumed. The RAM buffer can be accessed safely by software. */
    NRF_USBD_EVENT_ENDEPIN5      = offsetof(NRF_USBD_Type, EVENTS_ENDEPIN[5] ), /**< The whole EPIN[5] buffer has been consumed. The RAM buffer can be accessed safely by software. */
    NRF_USBD_EVENT_ENDEPIN6      = offsetof(NRF_USBD_Type, EVENTS_ENDEPIN[6] ), /**< The whole EPIN[6] buffer has been consumed. The RAM buffer can be accessed safely by software. */
    NRF_USBD_EVENT_ENDEPIN7      = offsetof(NRF_USBD_Type, EVENTS_ENDEPIN[7] ), /**< The whole EPIN[7] buffer has been consumed. The RAM buffer can be accessed safely by software. */
    NRF_USBD_EVENT_EP0DATADONE   = offsetof(NRF_USBD_Type, EVENTS_EP0DATADONE), /**< An acknowledged data transfer has taken place on the control endpoint. */
    NRF_USBD_EVENT_ENDISOIN0     = offsetof(NRF_USBD_Type, EVENTS_ENDISOIN   ), /**< The whole ISOIN buffer has been consumed. The RAM buffer can be accessed safely by software. */
    NRF_USBD_EVENT_ENDEPOUT0     = offsetof(NRF_USBD_Type, EVENTS_ENDEPOUT[0]), /**< The whole EPOUT[0] buffer has been consumed. The RAM buffer can be accessed safely by software. */
    NRF_USBD_EVENT_ENDEPOUT1     = offsetof(NRF_USBD_Type, EVENTS_ENDEPOUT[1]), /**< The whole EPOUT[1] buffer has been consumed. The RAM buffer can be accessed safely by software. */
    NRF_USBD_EVENT_ENDEPOUT2     = offsetof(NRF_USBD_Type, EVENTS_ENDEPOUT[2]), /**< The whole EPOUT[2] buffer has been consumed. The RAM buffer can be accessed safely by software. */
    NRF_USBD_EVENT_ENDEPOUT3     = offsetof(NRF_USBD_Type, EVENTS_ENDEPOUT[3]), /**< The whole EPOUT[3] buffer has been consumed. The RAM buffer can be accessed safely by software. */
    NRF_USBD_EVENT_ENDEPOUT4     = offsetof(NRF_USBD_Type, EVENTS_ENDEPOUT[4]), /**< The whole EPOUT[4] buffer has been consumed. The RAM buffer can be accessed safely by software. */
    NRF_USBD_EVENT_ENDEPOUT5     = offsetof(NRF_USBD_Type, EVENTS_ENDEPOUT[5]), /**< The whole EPOUT[5] buffer has been consumed. The RAM buffer can be accessed safely by software. */
    NRF_USBD_EVENT_ENDEPOUT6     = offsetof(NRF_USBD_Type, EVENTS_ENDEPOUT[6]), /**< The whole EPOUT[6] buffer has been consumed. The RAM buffer can be accessed safely by software. */
    NRF_USBD_EVENT_ENDEPOUT7     = offsetof(NRF_USBD_Type, EVENTS_ENDEPOUT[7]), /**< The whole EPOUT[7] buffer has been consumed. The RAM buffer can be accessed safely by software. */
    NRF_USBD_EVENT_ENDISOOUT0    = offsetof(NRF_USBD_Type, EVENTS_ENDISOOUT  ), /**< The whole ISOOUT buffer has been consumed. The RAM buffer can be accessed safely by software. */
    NRF_USBD_EVENT_SOF           = offsetof(NRF_USBD_Type, EVENTS_SOF        ), /**< Signals that a SOF (start of frame) condition has been detected on the USB lines. */
    NRF_USBD_EVENT_USBEVENT      = offsetof(NRF_USBD_Type, EVENTS_USBEVENT   ), /**< An event or an error not covered by the specified events has occurred, check EVENTCAUSE register to find the cause. */
    NRF_USBD_EVENT_EP0SETUP      = offsetof(NRF_USBD_Type, EVENTS_EP0SETUP   ), /**< A valid SETUP token has been received (and acknowledged) on the control endpoint. */
    NRF_USBD_EVENT_DATAEP        = offsetof(NRF_USBD_Type, EVENTS_EPDATA     ), /**< A data transfer has occurred on a data endpoint, indicated by the EPDATASTATUS register. */
}nrf_usbd_event_t;

/** @brief USBD shorts. */
typedef enum
{
    NRF_USBD_SHORT_EP0DATADONE_STARTEPIN0_MASK  = USBD_SHORTS_EP0DATADONE_STARTEPIN0_Msk , /**< Shortcut between EP0DATADONE event and STARTEPIN0 task. */
    NRF_USBD_SHORT_EP0DATADONE_STARTEPOUT0_MASK = USBD_SHORTS_EP0DATADONE_STARTEPOUT0_Msk, /**< Shortcut between EP0DATADONE event and STARTEPOUT0 task. */
    NRF_USBD_SHORT_EP0DATADONE_EP0STATUS_MASK   = USBD_SHORTS_EP0DATADONE_EP0STATUS_Msk  , /**< Shortcut between EP0DATADONE event and EP0STATUS task. */
    NRF_USBD_SHORT_ENDEPOUT0_EP0STATUS_MASK     = USBD_SHORTS_ENDEPOUT0_EP0STATUS_Msk    , /**< Shortcut between ENDEPOUT[0] event and EP0STATUS task. */
    NRF_USBD_SHORT_ENDEPOUT0_EP0RCVOUT_MASK     = USBD_SHORTS_ENDEPOUT0_EP0RCVOUT_Msk    , /**< Shortcut between ENDEPOUT[0] event and EP0RCVOUT task. */
}nrf_usbd_short_mask_t;

/** @brief USBD interrupts. */
typedef enum
{
    NRF_USBD_INT_USBRESET_MASK    = USBD_INTEN_USBRESET_Msk   , /**< Enable or disable interrupt for USBRESET event. */
    NRF_USBD_INT_STARTED_MASK     = USBD_INTEN_STARTED_Msk    , /**< Enable or disable interrupt for STARTED event. */
    NRF_USBD_INT_ENDEPIN0_MASK    = USBD_INTEN_ENDEPIN0_Msk   , /**< Enable or disable interrupt for ENDEPIN[0] event. */
    NRF_USBD_INT_ENDEPIN1_MASK    = USBD_INTEN_ENDEPIN1_Msk   , /**< Enable or disable interrupt for ENDEPIN[1] event. */
    NRF_USBD_INT_ENDEPIN2_MASK    = USBD_INTEN_ENDEPIN2_Msk   , /**< Enable or disable interrupt for ENDEPIN[2] event. */
    NRF_USBD_INT_ENDEPIN3_MASK    = USBD_INTEN_ENDEPIN3_Msk   , /**< Enable or disable interrupt for ENDEPIN[3] event. */
    NRF_USBD_INT_ENDEPIN4_MASK    = USBD_INTEN_ENDEPIN4_Msk   , /**< Enable or disable interrupt for ENDEPIN[4] event. */
    NRF_USBD_INT_ENDEPIN5_MASK    = USBD_INTEN_ENDEPIN5_Msk   , /**< Enable or disable interrupt for ENDEPIN[5] event. */
    NRF_USBD_INT_ENDEPIN6_MASK    = USBD_INTEN_ENDEPIN6_Msk   , /**< Enable or disable interrupt for ENDEPIN[6] event. */
    NRF_USBD_INT_ENDEPIN7_MASK    = USBD_INTEN_ENDEPIN7_Msk   , /**< Enable or disable interrupt for ENDEPIN[7] event. */
    NRF_USBD_INT_EP0DATADONE_MASK = USBD_INTEN_EP0DATADONE_Msk, /**< Enable or disable interrupt for EP0DATADONE event. */
    NRF_USBD_INT_ENDISOIN0_MASK   = USBD_INTEN_ENDISOIN_Msk   , /**< Enable or disable interrupt for ENDISOIN[0] event. */
    NRF_USBD_INT_ENDEPOUT0_MASK   = USBD_INTEN_ENDEPOUT0_Msk  , /**< Enable or disable interrupt for ENDEPOUT[0] event. */
    NRF_USBD_INT_ENDEPOUT1_MASK   = USBD_INTEN_ENDEPOUT1_Msk  , /**< Enable or disable interrupt for ENDEPOUT[1] event. */
    NRF_USBD_INT_ENDEPOUT2_MASK   = USBD_INTEN_ENDEPOUT2_Msk  , /**< Enable or disable interrupt for ENDEPOUT[2] event. */
    NRF_USBD_INT_ENDEPOUT3_MASK   = USBD_INTEN_ENDEPOUT3_Msk  , /**< Enable or disable interrupt for ENDEPOUT[3] event. */
    NRF_USBD_INT_ENDEPOUT4_MASK   = USBD_INTEN_ENDEPOUT4_Msk  , /**< Enable or disable interrupt for ENDEPOUT[4] event. */
    NRF_USBD_INT_ENDEPOUT5_MASK   = USBD_INTEN_ENDEPOUT5_Msk  , /**< Enable or disable interrupt for ENDEPOUT[5] event. */
    NRF_USBD_INT_ENDEPOUT6_MASK   = USBD_INTEN_ENDEPOUT6_Msk  , /**< Enable or disable interrupt for ENDEPOUT[6] event. */
    NRF_USBD_INT_ENDEPOUT7_MASK   = USBD_INTEN_ENDEPOUT7_Msk  , /**< Enable or disable interrupt for ENDEPOUT[7] event. */
    NRF_USBD_INT_ENDISOOUT0_MASK  = USBD_INTEN_ENDISOOUT_Msk  , /**< Enable or disable interrupt for ENDISOOUT[0] event. */
    NRF_USBD_INT_SOF_MASK         = USBD_INTEN_SOF_Msk        , /**< Enable or disable interrupt for SOF event. */
    NRF_USBD_INT_USBEVENT_MASK    = USBD_INTEN_USBEVENT_Msk   , /**< Enable or disable interrupt for USBEVENT event. */
    NRF_USBD_INT_EP0SETUP_MASK    = USBD_INTEN_EP0SETUP_Msk   , /**< Enable or disable interrupt for EP0SETUP event. */
    NRF_USBD_INT_DATAEP_MASK      = USBD_INTEN_EPDATA_Msk     , /**< Enable or disable interrupt for EPDATA event. */
}nrf_usbd_int_mask_t;


/**
 * @brief Function for activating the specified USBD task.
 *
 * @param[in] task Task to be activated.
 */
__STATIC_INLINE void nrf_usbd_task_trigger(nrf_usbd_task_t task);

/**
 * @brief Function for returning the address of the specified USBD task register.
 *
 * @param task Task.
 *
 * @return Task address.
 */
__STATIC_INLINE uint32_t nrf_usbd_task_address_get(nrf_usbd_task_t task);

/**
 * @brief Function for clearing the specified event.
 *
 * @param event Event.
 */
__STATIC_INLINE void nrf_usbd_event_clear(nrf_usbd_event_t event);

/**
 * @brief Function for retrieving the state of the USBD event.
 *
 * @param[in] event Event to be checked.
 *
 * @retval true  The event has been generated.
 * @retval false The event has not been generated.
 */
__STATIC_INLINE bool nrf_usbd_event_check(nrf_usbd_event_t event);

/**
 * @brief Function for getting and clearing the state of the specified event.
 *
 * This function checks the state of the event and clears it.
 *
 * @param event  Event.
 *
 * @retval true  The event was set.
 * @retval false The event was not set.
 */
__STATIC_INLINE bool nrf_usbd_event_get_and_clear(nrf_usbd_event_t event);

/**
 * @brief Function for returning the address of the specified USBD event register.
 *
 * @param event Event.
 *
 * @return Address.
 */
__STATIC_INLINE uint32_t nrf_usbd_event_address_get(nrf_usbd_event_t event);

/**
 * @brief Function for setting shortcuts.
 *
 * @param mask Shortcut mask.
 */
__STATIC_INLINE void nrf_usbd_shorts_enable(uint32_t mask);

/**
 * @brief Function for clearing shortcuts.
 *
 * @param mask Shortcut mask.
 */
__STATIC_INLINE void nrf_usbd_shorts_disable(uint32_t mask);

/**
 * @brief Function for getting the shortcut mask.
 *
 * Function returns shortcut register.
 *
 * @return Flags of the currently enabled shortcuts.
 */
__STATIC_INLINE uint32_t nrf_usbd_shorts_get(void);

/**
 * @brief Function for enabling the selected interrupts.
 *
 * @param mask Mask of interrupts to be enabled.
 */
__STATIC_INLINE void nrf_usbd_int_enable(uint32_t mask);

/**
 * @brief Function for retrieving the state of the selected interrupts.
 *
 * @param mask Mask of interrupts to be checked.
 *
 * @retval true  Any of selected interrupts is enabled.
 * @retval false None of selected interrupts is enabled.
 */
__STATIC_INLINE bool nrf_usbd_int_enable_check(uint32_t mask);

/**
 * @brief Function for retrieving the information about the enabled interrupts.
 *
 * @return The flags of the enabled interrupts.
 */
__STATIC_INLINE uint32_t nrf_usbd_int_enable_get(void);

/**
 * @brief Function for disabling the selected interrupts.
 *
 * @param mask Mask of interrupts to be disabled.
 */
__STATIC_INLINE void nrf_usbd_int_disable(uint32_t mask);


/** @} */ /*  End of nrf_usbd_hal */


#ifndef SUPPRESS_INLINE_IMPLEMENTATION

/* ------------------------------------------------------------------------------------------------
 *  Internal functions
 */

/**
 * @internal
 * @brief Internal function for getting the register address of task or event.
 *
 * @param[in] offset Offset of the register from the beginning of the instance.
 *
 * @attention The offset must be aligned to 4. In other case, hardware fault can occur.
 * @return Pointer to the register.
 */
__STATIC_INLINE volatile uint32_t* nrf_usbd_getRegPtr(uint32_t offset)
{
    return (volatile uint32_t*)(((uint8_t *)NRF_USBD) + (uint32_t)offset);
}

/**
 * @internal
 * @brief Internal function for getting the register address of task or event - constant version.
 *
 * @param[in] offset Offset of the register from the beginning of the instance.
 *
 * @attention The offset must be aligned to 4. In other case, hardware fault can occur.
 * @return Pointer to the register.
 */
__STATIC_INLINE volatile const uint32_t* nrf_usbd_getRegPtr_c(uint32_t offset)
{
    return (volatile const uint32_t*)(((uint8_t *)NRF_USBD) + (uint32_t)offset);
}

/* ------------------------------------------------------------------------------------------------
 *  Interface functions definitions
 */

void nrf_usbd_task_trigger(nrf_usbd_task_t task)
{
    *(nrf_usbd_getRegPtr((uint32_t)task)) = 1UL;
    __ISB();
    __DSB();
}

uint32_t nrf_usbd_task_address_get(nrf_usbd_task_t task)
{
    return (uint32_t)nrf_usbd_getRegPtr_c((uint32_t)task);
}

void nrf_usbd_event_clear(nrf_usbd_event_t event)
{
    *(nrf_usbd_getRegPtr((uint32_t)event)) = 0UL;
    __ISB();
    __DSB();
}

bool nrf_usbd_event_check(nrf_usbd_event_t event)
{
    return (bool)*nrf_usbd_getRegPtr_c((uint32_t)event);
}

bool nrf_usbd_event_get_and_clear(nrf_usbd_event_t event)
{
    bool ret = nrf_usbd_event_check(event);
    if (ret)
    {
        nrf_usbd_event_clear(event);
    }
    return ret;
}

uint32_t nrf_usbd_event_address_get(nrf_usbd_event_t event)
{
    return (uint32_t)nrf_usbd_getRegPtr_c((uint32_t)event);
}

void nrf_usbd_shorts_enable(uint32_t mask)
{
    NRF_USBD->SHORTS |= mask;
}

void nrf_usbd_shorts_disable(uint32_t mask)
{
    if (~0U == mask)
    {
        /* Optimized version for "disable all" */
        NRF_USBD->SHORTS = 0;
    }
    else
    {
        NRF_USBD->SHORTS &= ~mask;
    }
}

uint32_t nrf_usbd_shorts_get(void)
{
    return NRF_USBD->SHORTS;
}

void nrf_usbd_int_enable(uint32_t mask)
{
    NRF_USBD->INTENSET = mask;
}

bool nrf_usbd_int_enable_check(uint32_t mask)
{
    return !!(NRF_USBD->INTENSET & mask);
}

uint32_t nrf_usbd_int_enable_get(void)
{
    return NRF_USBD->INTENSET;
}

void nrf_usbd_int_disable(uint32_t mask)
{
    NRF_USBD->INTENCLR = mask;
}

#endif /* SUPPRESS_INLINE_IMPLEMENTATION */

/* ------------------------------------------------------------------------------------------------
 *  End of automatically generated part
 * ------------------------------------------------------------------------------------------------
 */
/**
 * @addtogroup nrf_usbd_hal
 * @{
 */

/**
 * @brief Frame counter size.
 *
 * The number of counts that can be fitted into frame counter.
 */
#define NRF_USBD_FRAMECNTR_SIZE \
    ( (USBD_FRAMECNTR_FRAMECNTR_Msk >> USBD_FRAMECNTR_FRAMECNTR_Pos) + 1UL )
#ifndef USBD_FRAMECNTR_FRAMECNTR_Msk
#error USBD_FRAMECNTR_FRAMECNTR_Msk should be changed into USBD_FRAMECNTR_FRAMECNTR_Msk
#endif

/**
 * @brief First isochronous endpoint number.
 *
 * The number of the first isochronous endpoint.
 */
#define NRF_USBD_EPISO_FIRST 8

/**
 * @brief Total number of IN endpoints.
 *
 * Total number of IN endpoint (including ISOCHRONOUS).
 */
#define NRF_USBD_EPIN_CNT 9

/**
 * @brief Total number of OUT endpoints.
 *
 * Total number of OUT endpoint (including ISOCHRONOUS).
 */
#define NRF_USBD_EPOUT_CNT 9

/** @brief Mask of the direction bit in an endpoint number. */
#define NRF_USBD_EP_DIR_Msk (1U << 7)

/** @brief The value of direction bit for the IN endpoint direction. */
#define NRF_USBD_EP_DIR_IN  (1U << 7)

/** @brief The value of direction bit for the OUT endpoint direction. */
#define NRF_USBD_EP_DIR_OUT (0U << 7)

/**
 * @brief Macro for making the IN endpoint identifier from endpoint number.
 *
 * Macro that sets direction bit to make IN endpoint.
 * @param[in] epnr Endpoint number.
 * @return IN Endpoint identifier.
 */
#define NRF_USBD_EPIN(epnr)  (((uint8_t)(epnr)) | NRF_USBD_EP_DIR_IN)

/**
 * @brief Macro for making the OUT endpoint identifier from endpoint number.
 *
 * Macro that sets direction bit to make OUT endpoint.
 * @param[in] epnr Endpoint number.
 * @return OUT Endpoint identifier.
 */
#define NRF_USBD_EPOUT(epnr) (((uint8_t)(epnr)) | NRF_USBD_EP_DIR_OUT)

/**
 * @brief Macro for extracting the endpoint number from the specified endpoint identifier.
 *
 * Macro that strips out the information about endpoint direction.
 *
 * @param[in] ep Endpoint identifier.
 *
 * @return Endpoint number.
 */
#define NRF_USBD_EP_NR_GET(ep) ((uint8_t)(((uint8_t)(ep)) & 0xFU))

/**
 * @brief Macro for checking the endpoint direction.
 *
 * This macro checks if the specified endpoint has the IN direction.
 *
 * @param ep Endpoint identifier.
 *
 * @retval true  The endpoint direction is IN.
 * @retval false The endpoint direction is OUT.
 */
#define NRF_USBD_EPIN_CHECK(ep)  ( (((uint8_t)(ep)) & NRF_USBD_EP_DIR_Msk) == NRF_USBD_EP_DIR_IN  )

/**
 * @brief Macro for checking endpoint direction.
 *
 * This macro checks if given endpoint has OUT direction.
 *
 * @param ep Endpoint identifier
 *
 * @retval true  The endpoint direction is OUT
 * @retval false The endpoint direction is IN
 */
#define NRF_USBD_EPOUT_CHECK(ep) ( (((uint8_t)(ep)) & NRF_USBD_EP_DIR_Msk) == NRF_USBD_EP_DIR_OUT )

/**
 * @brief Macro for checking if endpoint is isochronous.
 *
 * @param ep It can be endpoint identifier or just endpoint number to be checked.
 *
 * @retval true  The endpoint is isochronous type.
 * @retval false The endpoint is bulk of interrupt type.
 */
#define NRF_USBD_EPISO_CHECK(ep) (NRF_USBD_EP_NR_GET(ep) >= NRF_USBD_EPISO_FIRST)

/**
 * @brief Macro for checking if given number is valid endpoint number.
 *
 * @param ep Endpoint number to be checked.
 *
 * @retval true  The endpoint is valid.
 * @retval false The endpoint is not valid.
 */
#define NRF_USBD_EP_VALIDATE(ep) (                                              \
    (NRF_USBD_EPIN_CHECK(ep) && (NRF_USBD_EP_NR_GET(ep) < NRF_USBD_EPIN_CNT))   \
    ||                                                                          \
    (NRF_USBD_EPOUT_CHECK(ep) && (NRF_USBD_EP_NR_GET(ep) < NRF_USBD_EPOUT_CNT)) \
    )

/**
 * @brief Not isochronous data frame received.
 *
 * Special value returned by @ref nrf_usbd_episoout_size_get function that means that
 * data frame was not received at all.
 * This allows differentiate between situations when zero size data comes or no data comes at all
 * on isochronous endpoint.
 */
#define NRF_USBD_EPISOOUT_NO_DATA ((size_t)(-1))

/** @brief EVENTCAUSE register bit masks. */
typedef enum
{
    NRF_USBD_EVENTCAUSE_ISOOUTCRC_MASK    = USBD_EVENTCAUSE_ISOOUTCRC_Msk,      /**< CRC error was detected on isochronous OUT endpoint 8. */
    NRF_USBD_EVENTCAUSE_SUSPEND_MASK      = USBD_EVENTCAUSE_SUSPEND_Msk,        /**< Signals that the USB lines have been seen idle long enough for the device to enter suspend. */
    NRF_USBD_EVENTCAUSE_RESUME_MASK       = USBD_EVENTCAUSE_RESUME_Msk,         /**< Signals that a RESUME condition (K state or activity restart) has been detected on the USB lines. */
    NRF_USBD_EVENTCAUSE_WUREQ_MASK        = USBD_EVENTCAUSE_USBWUALLOWED_Msk,   /**< The USBD peripheral has exited Low Power mode. */
    NRF_USBD_EVENTCAUSE_READY_MASK        = USBD_EVENTCAUSE_READY_Msk,          /**< MAC is ready for normal operation, rised few us after USBD enabling. */
}nrf_usbd_eventcause_mask_t;

/** @brief DPDMVALUE register. */
typedef enum
{
    /** Generate RESUME signal. Signal is generated for 50&nbsp;us or 5&nbsp;ms,
     *  depending on bus state. */
    NRF_USBD_DPDMVALUE_RESUME = USBD_DPDMVALUE_STATE_Resume,
    /** D+ Forced high, D- forced low (J state) */
    NRF_USBD_DPDMVALUE_J      = USBD_DPDMVALUE_STATE_J,
    /** D+ Forced low, D- forced high (K state) */
    NRF_USBD_DPMVALUE_K       = USBD_DPDMVALUE_STATE_K
}nrf_usbd_dpdmvalue_t;

/** @brief Data toggle value or operation. */
typedef enum
{
    NRF_USBD_DTOGGLE_NOP   = USBD_DTOGGLE_VALUE_Nop,  /**< No operation - do not change the current data toggle on the selected endpoint. */
    NRF_USBD_DTOGGLE_DATA0 = USBD_DTOGGLE_VALUE_Data0,/**< Data toggle is DATA0 on the selected endpoint. */
    NRF_USBD_DTOGGLE_DATA1 = USBD_DTOGGLE_VALUE_Data1 /**< Data toggle is DATA1 on the selected endpoint. */
}nrf_usbd_dtoggle_t;

/** @brief EPSTATUS bit masks. */
typedef enum
{
    NRF_USBD_EPSTATUS_EPIN0_MASK  = USBD_EPSTATUS_EPIN0_Msk,
    NRF_USBD_EPSTATUS_EPIN1_MASK  = USBD_EPSTATUS_EPIN1_Msk,
    NRF_USBD_EPSTATUS_EPIN2_MASK  = USBD_EPSTATUS_EPIN2_Msk,
    NRF_USBD_EPSTATUS_EPIN3_MASK  = USBD_EPSTATUS_EPIN3_Msk,
    NRF_USBD_EPSTATUS_EPIN4_MASK  = USBD_EPSTATUS_EPIN4_Msk,
    NRF_USBD_EPSTATUS_EPIN5_MASK  = USBD_EPSTATUS_EPIN5_Msk,
    NRF_USBD_EPSTATUS_EPIN6_MASK  = USBD_EPSTATUS_EPIN6_Msk,
    NRF_USBD_EPSTATUS_EPIN7_MASK  = USBD_EPSTATUS_EPIN7_Msk,

    NRF_USBD_EPSTATUS_EPOUT0_MASK = USBD_EPSTATUS_EPOUT0_Msk,
    NRF_USBD_EPSTATUS_EPOUT1_MASK = USBD_EPSTATUS_EPOUT1_Msk,
    NRF_USBD_EPSTATUS_EPOUT2_MASK = USBD_EPSTATUS_EPOUT2_Msk,
    NRF_USBD_EPSTATUS_EPOUT3_MASK = USBD_EPSTATUS_EPOUT3_Msk,
    NRF_USBD_EPSTATUS_EPOUT4_MASK = USBD_EPSTATUS_EPOUT4_Msk,
    NRF_USBD_EPSTATUS_EPOUT5_MASK = USBD_EPSTATUS_EPOUT5_Msk,
    NRF_USBD_EPSTATUS_EPOUT6_MASK = USBD_EPSTATUS_EPOUT6_Msk,
    NRF_USBD_EPSTATUS_EPOUT7_MASK = USBD_EPSTATUS_EPOUT7_Msk,
}nrf_usbd_epstatus_mask_t;

/** @brief DATAEPSTATUS bit masks. */
typedef enum
{
    NRF_USBD_EPDATASTATUS_EPIN1_MASK  = USBD_EPDATASTATUS_EPIN1_Msk,
    NRF_USBD_EPDATASTATUS_EPIN2_MASK  = USBD_EPDATASTATUS_EPIN2_Msk,
    NRF_USBD_EPDATASTATUS_EPIN3_MASK  = USBD_EPDATASTATUS_EPIN3_Msk,
    NRF_USBD_EPDATASTATUS_EPIN4_MASK  = USBD_EPDATASTATUS_EPIN4_Msk,
    NRF_USBD_EPDATASTATUS_EPIN5_MASK  = USBD_EPDATASTATUS_EPIN5_Msk,
    NRF_USBD_EPDATASTATUS_EPIN6_MASK  = USBD_EPDATASTATUS_EPIN6_Msk,
    NRF_USBD_EPDATASTATUS_EPIN7_MASK  = USBD_EPDATASTATUS_EPIN7_Msk,

    NRF_USBD_EPDATASTATUS_EPOUT1_MASK = USBD_EPDATASTATUS_EPOUT1_Msk,
    NRF_USBD_EPDATASTATUS_EPOUT2_MASK = USBD_EPDATASTATUS_EPOUT2_Msk,
    NRF_USBD_EPDATASTATUS_EPOUT3_MASK = USBD_EPDATASTATUS_EPOUT3_Msk,
    NRF_USBD_EPDATASTATUS_EPOUT4_MASK = USBD_EPDATASTATUS_EPOUT4_Msk,
    NRF_USBD_EPDATASTATUS_EPOUT5_MASK = USBD_EPDATASTATUS_EPOUT5_Msk,
    NRF_USBD_EPDATASTATUS_EPOUT6_MASK = USBD_EPDATASTATUS_EPOUT6_Msk,
    NRF_USBD_EPDATASTATUS_EPOUT7_MASK = USBD_EPDATASTATUS_EPOUT7_Msk,
}nrf_usbd_dataepstatus_mask_t;

/** @brief ISOSPLIT configurations. */
typedef enum
{
    NRF_USBD_ISOSPLIT_ONEDIR = USBD_ISOSPLIT_SPLIT_OneDir, /**< Full buffer dedicated to either ISO IN or OUT. */
    NRF_USBD_ISOSPLIT_HALF   = USBD_ISOSPLIT_SPLIT_HalfIN, /**< Buffer divided in half. */
}nrf_usbd_isosplit_t;

/** @brief ISOINCONFIG configurations. */
typedef enum
{
    NRF_USBD_ISOINCONFIG_NORESP   = USBD_ISOINCONFIG_RESPONSE_NoResp,   /**< Endpoint does not respond to an ISO IN token when no data is ready. */
    NRF_USBD_ISOINCONFIG_ZERODATA = USBD_ISOINCONFIG_RESPONSE_ZeroData, /**< Endpoint responds with a zero-length data packet to an ISO IN token when no data is ready. */
}nrf_usbd_isoinconfig_t;

/** @brief Function for enabling the USBD. */
__STATIC_INLINE void nrf_usbd_enable(void);

/** @brief Function for disabling the USBD. */
__STATIC_INLINE void nrf_usbd_disable(void);

/**
 * @brief Function for getting the EVENTCAUSE register.
 *
 * @return Flag values defined in @ref nrf_usbd_eventcause_mask_t.
 */
__STATIC_INLINE uint32_t nrf_usbd_eventcause_get(void);

/**
 * @brief Function for clearing the EVENTCAUSE flags.
 *
 * @param flags Flags defined in @ref nrf_usbd_eventcause_mask_t.
 */
__STATIC_INLINE void nrf_usbd_eventcause_clear(uint32_t flags);

/**
 * @brief Function for getting the EVENTCAUSE register and clearing flags that are set.
 *
 * The safest way to return current EVENTCAUSE register.
 * All the flags that are returned would be cleared inside EVENTCAUSE register.
 *
 * @return Flag values defined in @ref nrf_usbd_eventcause_mask_t
 */
__STATIC_INLINE uint32_t nrf_usbd_eventcause_get_and_clear(void);

/**
 * @brief Function for getting the HALTEDEPIN register value.
 *
 * @note Use this function for the response for GetStatus() request to endpoint.
 *       To check whether the endpoint is stalled in the code, use @ref nrf_usbd_ep_is_stall.
 *
 * @param ep Endpoint number with IN or OUT flag.
 *
 * @return The value of the HALTEDEPIN or HALTEDOUT register for the selected endpoint.
 */
__STATIC_INLINE uint32_t nrf_usbd_haltedep(uint8_t ep);

/**
 * @brief Function for checking whether the selected endpoint is stalled.
 *
 * Function to be used as a syntax sweeter for @ref nrf_usbd_haltedep.
 *
 * Also as the isochronous endpoint cannot be halted - it returns always false
 * if isochronous endpoint is checked.
 *
 * @param ep Endpoint number with IN or OUT flag.
 *
 * @retval true  The endpoint is halted.
 * @retval false The endpoint is not halted.
 */
__STATIC_INLINE bool nrf_usbd_ep_is_stall(uint8_t ep);

/**
 * @brief Function for getting EPSTATUS register value.
 *
 * @return Flag values defined in @ref nrf_usbd_epstatus_mask_t.
 */
__STATIC_INLINE uint32_t nrf_usbd_epstatus_get(void);

/**
 * @brief Function for clearing EPSTATUS register value.
 *
 * @param flags Flags defined in @ref nrf_usbd_epstatus_mask_t.
 */
__STATIC_INLINE void nrf_usbd_epstatus_clear(uint32_t flags);

/**
 * @brief Function for getting and clearing EPSTATUS register value.
 *
 * Function clears all flags in register set before returning its value.
 *
 * @return Flag values defined in @ref nrf_usbd_epstatus_mask_t.
 */
__STATIC_INLINE uint32_t nrf_usbd_epstatus_get_and_clear(void);

/**
 * @brief Function for getting DATAEPSTATUS register value.
 *
 * @return Flag values defined in @ref nrf_usbd_dataepstatus_mask_t.
 */
__STATIC_INLINE uint32_t nrf_usbd_epdatastatus_get(void);

/**
 * @brief Function for clearing DATAEPSTATUS register value.
 *
 * @param flags Flags defined in @ref nrf_usbd_dataepstatus_mask_t.
 */
__STATIC_INLINE void nrf_usbd_epdatastatus_clear(uint32_t flags);

/**
 * @brief Function for getting and clearing DATAEPSTATUS register value.
 *
 * Function clears all flags in register set before returning its value.
 * @return Flag values defined in @ref nrf_usbd_dataepstatus_mask_t.
 */
__STATIC_INLINE uint32_t nrf_usbd_epdatastatus_get_and_clear(void);

/**
 * @name Setup command frame functions.
 *
 * Functions for setting up command frame part access.
 * @{
 */
    /**
     * @brief Function for reading BMREQUESTTYPE - part of the SETUP packet.
     *
     * @return Value of BREQUESTTYPE on the last received SETUP frame.
     */
    __STATIC_INLINE uint8_t nrf_usbd_setup_bmrequesttype_get(void);

    /**
     * @brief Function for reading BMREQUEST - part of the SETUP packet.
     *
     * @return Value of BREQUEST on the last received SETUP frame.
     */
    __STATIC_INLINE uint8_t nrf_usbd_setup_brequest_get(void);

    /**
     * @brief Function for reading WVALUE - part of the SETUP packet.
     *
     * @return Value of WVALUE on the last received SETUP frame.
     */
    __STATIC_INLINE uint16_t nrf_usbd_setup_wvalue_get(void);

    /**
     * @brief Function for reading WINDEX - part of the SETUP packet.
     *
     * @return Value of WINDEX on the last received SETUP frame.
     */
    __STATIC_INLINE uint16_t nrf_usbd_setup_windex_get(void);

    /**
     * @brief Function for reading WLENGTH - part of the SETUP packet.
     *
     * @return Value of WLENGTH on the last received SETUP frame.
     */
    __STATIC_INLINE uint16_t nrf_usbd_setup_wlength_get(void);
/** @} */

/**
 * @brief Function for getting the number of received bytes on the selected endpoint.
 *
 * @note This function can be used on bulk, interrupt, and isochronous endpoints.
 * @note For the function that returns different value for the ISOOUT zero
 *       transfer or no transfer at all, see the @ref nrf_usbd_episoout_size_get
 *       function. This function will return 0 for both cases.
 *
 * @param ep Endpoint identifier.
 *
 * @return Number of received bytes.
 */
__STATIC_INLINE size_t nrf_usbd_epout_size_get(uint8_t ep);

/**
 * @brief Function for getting number of the received bytes on isochronous endpoint.
 *
 * @param ep Endpoint identifier, must be an isochronous OUT endpoint.
 *
 * @return Number of bytes received or @ref NRF_USBD_EPISOOUT_NO_DATA.
 */
__STATIC_INLINE size_t nrf_usbd_episoout_size_get(uint8_t ep);

/**
 * @brief Function for clearing OUT endpoint to accept any new incoming traffic.
 *
 * @param ep ep Endpoint identifier. Only OUT interrupt/bulk endpoints are accepted.
 */
__STATIC_INLINE void nrf_usbd_epout_clear(uint8_t ep);

/** @brief Function for enabling the USB pullup. */
__STATIC_INLINE void nrf_usbd_pullup_enable(void);

/** @brief Function for disabling the USB pullup. */
__STATIC_INLINE void nrf_usbd_pullup_disable(void);

/**
 * @brief Function for returning the current the USB pullup state.
 *
 * @retval true  The USB pullup is enabled.
 * @retval false The USB pullup is disabled.
 */
__STATIC_INLINE bool nrf_usbd_pullup_check(void);

/**
 * @brief Function for configuring the value to be forced on the bus on the DRIVEDPDM task.
 *
 * The selected state will be forced on the bus when @ref NRF_USBD_TASK_DRIVEDPDM is set.
 * The state will be removed from the bus on @ref NRF_USBD_TASK_NODRIVEDPDM and
 * the control will be returned to the USBD peripheral.
 * @param val State to be set.
 */
__STATIC_INLINE void nrf_usbd_dpdmvalue_set(nrf_usbd_dpdmvalue_t val);

/**
 * @brief Function for setting the data toggle.
 *
 * Configuration of the current state of data toggling.
 * @param ep Endpoint number with the information about its direction.
 * @param op Operation to execute.
 */
__STATIC_INLINE void nrf_usbd_dtoggle_set(uint8_t ep, nrf_usbd_dtoggle_t op);

/**
 * @brief Function for getting the data toggle.
 *
 * Get the current state of data toggling.
 *
 * @param ep Endpoint number to return the information about current data toggling.
 *
 * @retval NRF_USBD_DTOGGLE_DATA0 Data toggle is DATA0 on selected endpoint.
 * @retval NRF_USBD_DTOGGLE_DATA1 Data toggle is DATA1 on selected endpoint.
 */
__STATIC_INLINE nrf_usbd_dtoggle_t nrf_usbd_dtoggle_get(uint8_t ep);

/**
 * @brief Function for checking whether the endpoint is enabled.
 *
 * @param ep Endpoint ID to be checked.
 *
 * @retval true  Endpoint is enabled.
 * @retval false Endpoint is disabled.
 */
__STATIC_INLINE bool nrf_usbd_ep_enable_check(uint8_t ep);

/**
 * @brief Function for enabling the selected endpoint.
 *
 * The enabled endpoint responds for the tokens on the USB bus.
 *
 * @param ep Endpoint ID to be enabled.
 */
__STATIC_INLINE void nrf_usbd_ep_enable(uint8_t ep);

/**
 * @brief Function for disabling the selected endpoint.
 *
 * The disabled endpoint does not respond for the tokens on the USB bus.
 *
 * @param ep Endpoint ID to be disabled.
 */
__STATIC_INLINE void nrf_usbd_ep_disable(uint8_t ep);

/**
 * @brief Function for disabling all endpoints.
 *
 * An auxiliary function to disable all aviable endpoints.
 * It leaves only EP0 IN and OUT enabled.
 */
__STATIC_INLINE void nrf_usbd_ep_all_disable(void);

/**
 * @brief Function for stalling the selected endpoint.
 *
 * @note This function cannot be called on isochronous endpoint.
 *
 * @param ep Endpoint identifier.
 */
__STATIC_INLINE void nrf_usbd_ep_stall(uint8_t ep);

/**
 * @brief Function for unstalling the selected endpoint.
 *
 * @note This function cannot be called on isochronous endpoint.
 *
 * @param ep Endpoint identifier
 */
__STATIC_INLINE void nrf_usbd_ep_unstall(uint8_t ep);

/**
 * @brief Function for configuring the isochronous buffer splitting.
 *
 * This function configures isochronous buffer splitting between IN and OUT endpoints.
 *
 * @param split Required configuration.
 */
__STATIC_INLINE void nrf_usbd_isosplit_set(nrf_usbd_isosplit_t split);

/**
 * @brief Function for getting the isochronous buffer splitting configuration.
 *
 * This function gets the current isochronous buffer splitting configuration.
 *
 * @return Current configuration.
 */
__STATIC_INLINE nrf_usbd_isosplit_t nrf_usbd_isosplit_get(void);

/**
 * @brief Function for getting current frame counter.
 *
 * @return Current frame counter.
 */
__STATIC_INLINE uint32_t nrf_usbd_framecntr_get(void);

/**
 * @brief Function for entering into the USB low power mode.
 *
 * After this function is called, the clock source from the USBD is disconnected internally.
 * After this function is called, most of the USBD registers cannot be accessed anymore.
 *
 * @sa nrf_usbd_lowpower_disable
 * @sa nrf_usbd_lowpower_check
 */
__STATIC_INLINE void nrf_usbd_lowpower_enable(void);

/**
 * @brief Function for exiting from the USB low power mode.
 *
 * After this function is called, the clock source for the USBD is connected internally.
 * The @ref NRF_USBD_EVENTCAUSE_WUREQ_MASK event will be generated and
 * then the USBD registers can be accessed.
 *
 * @sa nrf_usbd_lowpower_enable
 * @sa nrf_usbd_lowpower_check
 */
__STATIC_INLINE void nrf_usbd_lowpower_disable(void);

/**
 * @brief Function for checking the state of the low power mode.
 *
 * @retval true  The USBD is in low power mode.
 * @retval false The USBD is not in low power mode.
 */
__STATIC_INLINE bool nrf_usbd_lowpower_check(void);

/**
 * @brief Function for configuring ISO IN endpoint response to an IN token when no data is ready to be sent.
 *
 * @param config Required configuration.
 */
__STATIC_INLINE void nrf_usbd_isoinconfig_set(nrf_usbd_isoinconfig_t config);

/**
 * @brief Function for getting the cofiguration of ISO IN endpoint response to an IN token when no data is ready to be sent.
 *
 * @return Current configuration.
 */
__STATIC_INLINE nrf_usbd_isoinconfig_t nrf_usbd_isoinconfig_get(void);

/**
 * @brief Function for configuring the EasyDMA channel.
 *
 * Configures EasyDMA for the transfer.
 *
 * @param ep     Endpoint identifier (with direction).
 * @param ptr    Pointer to the data.
 * @param maxcnt Number of bytes to transfer.
 */
__STATIC_INLINE void nrf_usbd_ep_easydma_set(uint8_t ep, uint32_t ptr, uint32_t maxcnt);

/**
 * @brief Function for getting number of transferred bytes.
 *
 * This function gets the number of transferred bytes in the last transaction.
 *
 * @param ep Endpoint identifier.
 *
 * @return The content of the AMOUNT register.
 */
__STATIC_INLINE uint32_t nrf_usbd_ep_amount_get(uint8_t ep);


#ifndef SUPPRESS_INLINE_IMPLEMENTATION

void nrf_usbd_enable(void)
{
#ifdef NRF_FPGA_IMPLEMENTATION
    *(volatile uint32_t *)0x400005F4 = 3;
    __ISB();
    __DSB();
    *(volatile uint32_t *)0x400005F0 = 3;
    __ISB();
    __DSB();
#endif

    NRF_USBD->ENABLE = USBD_ENABLE_ENABLE_Enabled << USBD_ENABLE_ENABLE_Pos;
    __ISB();
    __DSB();
}

void nrf_usbd_disable(void)
{
    NRF_USBD->ENABLE = USBD_ENABLE_ENABLE_Disabled << USBD_ENABLE_ENABLE_Pos;
    __ISB();
    __DSB();
}

uint32_t nrf_usbd_eventcause_get(void)
{
    return NRF_USBD->EVENTCAUSE;
}

void nrf_usbd_eventcause_clear(uint32_t flags)
{
    NRF_USBD->EVENTCAUSE = flags;
    __ISB();
    __DSB();
}

uint32_t nrf_usbd_eventcause_get_and_clear(void)
{
    uint32_t ret;
    ret = nrf_usbd_eventcause_get();
    nrf_usbd_eventcause_clear(ret);
    __ISB();
    __DSB();
    return ret;
}

uint32_t nrf_usbd_haltedep(uint8_t ep)
{
    uint8_t epnr = NRF_USBD_EP_NR_GET(ep);
    if (NRF_USBD_EPIN_CHECK(ep))
    {
        NRFX_ASSERT(epnr < NRFX_ARRAY_SIZE(NRF_USBD->HALTED.EPIN));
        return NRF_USBD->HALTED.EPIN[epnr];
    }
    else
    {
        NRFX_ASSERT(epnr < NRFX_ARRAY_SIZE(NRF_USBD->HALTED.EPOUT));
        return NRF_USBD->HALTED.EPOUT[epnr];
    }
}

bool nrf_usbd_ep_is_stall(uint8_t ep)
{
    if (NRF_USBD_EPISO_CHECK(ep))
        return false;
    return USBD_HALTED_EPOUT_GETSTATUS_Halted == nrf_usbd_haltedep(ep);
}

uint32_t nrf_usbd_epstatus_get(void)
{
    return NRF_USBD->EPSTATUS;
}

void nrf_usbd_epstatus_clear(uint32_t flags)
{
    NRF_USBD->EPSTATUS = flags;
    __ISB();
    __DSB();
}

uint32_t nrf_usbd_epstatus_get_and_clear(void)
{
    uint32_t ret;
    ret = nrf_usbd_epstatus_get();
    nrf_usbd_epstatus_clear(ret);
    return ret;
}

uint32_t nrf_usbd_epdatastatus_get(void)
{
    return NRF_USBD->EPDATASTATUS;
}

void nrf_usbd_epdatastatus_clear(uint32_t flags)
{
    NRF_USBD->EPDATASTATUS = flags;
    __ISB();
    __DSB();
}

uint32_t nrf_usbd_epdatastatus_get_and_clear(void)
{
    uint32_t ret;
    ret = nrf_usbd_epdatastatus_get();
    nrf_usbd_epdatastatus_clear(ret);
    return ret;
}

uint8_t nrf_usbd_setup_bmrequesttype_get(void)
{
    return (uint8_t)(NRF_USBD->BMREQUESTTYPE);
}

uint8_t nrf_usbd_setup_brequest_get(void)
{
    return (uint8_t)(NRF_USBD->BREQUEST);
}

uint16_t nrf_usbd_setup_wvalue_get(void)
{
    const uint16_t val = NRF_USBD->WVALUEL;
    return (uint16_t)(val | ((NRF_USBD->WVALUEH) << 8));
}

uint16_t nrf_usbd_setup_windex_get(void)
{
    const uint16_t val = NRF_USBD->WINDEXL;
    return (uint16_t)(val | ((NRF_USBD->WINDEXH) << 8));
}

uint16_t nrf_usbd_setup_wlength_get(void)
{
    const uint16_t val = NRF_USBD->WLENGTHL;
    return (uint16_t)(val | ((NRF_USBD->WLENGTHH) << 8));
}

size_t nrf_usbd_epout_size_get(uint8_t ep)
{
    NRFX_ASSERT(NRF_USBD_EP_VALIDATE(ep));
    NRFX_ASSERT(NRF_USBD_EPOUT_CHECK(ep));
    if (NRF_USBD_EPISO_CHECK(ep))
    {
        size_t size_isoout = NRF_USBD->SIZE.ISOOUT;
        if ((size_isoout & USBD_SIZE_ISOOUT_ZERO_Msk) == (USBD_SIZE_ISOOUT_ZERO_ZeroData << USBD_SIZE_ISOOUT_ZERO_Pos))
        {
            size_isoout = 0;
        }
        return size_isoout;
    }

    NRFX_ASSERT(NRF_USBD_EP_NR_GET(ep) < NRFX_ARRAY_SIZE(NRF_USBD->SIZE.EPOUT));
    return NRF_USBD->SIZE.EPOUT[NRF_USBD_EP_NR_GET(ep)];
}

size_t nrf_usbd_episoout_size_get(uint8_t ep)
{
    NRFX_ASSERT(NRF_USBD_EP_VALIDATE(ep));
    NRFX_ASSERT(NRF_USBD_EPOUT_CHECK(ep));
    NRFX_ASSERT(NRF_USBD_EPISO_CHECK(ep));

    size_t size_isoout = NRF_USBD->SIZE.ISOOUT;
    if (size_isoout == 0)
    {
        size_isoout = NRF_USBD_EPISOOUT_NO_DATA;
    }
    else if ((size_isoout & USBD_SIZE_ISOOUT_ZERO_Msk) == (USBD_SIZE_ISOOUT_ZERO_ZeroData << USBD_SIZE_ISOOUT_ZERO_Pos))
    {
        size_isoout = 0;
    }
    return size_isoout;
}

void nrf_usbd_epout_clear(uint8_t ep)
{
    NRFX_ASSERT(NRF_USBD_EPOUT_CHECK(ep) && (NRF_USBD_EP_NR_GET(ep) < NRFX_ARRAY_SIZE(NRF_USBD->SIZE.EPOUT)));
    NRF_USBD->SIZE.EPOUT[NRF_USBD_EP_NR_GET(ep)] = 0;
    __ISB();
    __DSB();
}

void nrf_usbd_pullup_enable(void)
{
    NRF_USBD->USBPULLUP = USBD_USBPULLUP_CONNECT_Enabled << USBD_USBPULLUP_CONNECT_Pos;
    __ISB();
    __DSB();
}

void nrf_usbd_pullup_disable(void)
{
    NRF_USBD->USBPULLUP = USBD_USBPULLUP_CONNECT_Disabled << USBD_USBPULLUP_CONNECT_Pos;
    __ISB();
    __DSB();
}

bool nrf_usbd_pullup_check(void)
{
    return NRF_USBD->USBPULLUP == (USBD_USBPULLUP_CONNECT_Enabled << USBD_USBPULLUP_CONNECT_Pos);
}

void nrf_usbd_dpdmvalue_set(nrf_usbd_dpdmvalue_t val)
{
    NRF_USBD->DPDMVALUE = ((uint32_t)val) << USBD_DPDMVALUE_STATE_Pos;
}

void nrf_usbd_dtoggle_set(uint8_t ep, nrf_usbd_dtoggle_t op)
{
    NRFX_ASSERT(NRF_USBD_EP_VALIDATE(ep));
    NRFX_ASSERT(!NRF_USBD_EPISO_CHECK(ep));
    NRF_USBD->DTOGGLE = ep | (NRF_USBD_DTOGGLE_NOP << USBD_DTOGGLE_VALUE_Pos);
    __DSB();
    NRF_USBD->DTOGGLE = ep | (op << USBD_DTOGGLE_VALUE_Pos);
    __ISB();
    __DSB();
}

nrf_usbd_dtoggle_t nrf_usbd_dtoggle_get(uint8_t ep)
{
    uint32_t retval;
    /* Select the endpoint to read */
    NRF_USBD->DTOGGLE = ep | (NRF_USBD_DTOGGLE_NOP << USBD_DTOGGLE_VALUE_Pos);
    retval = ((NRF_USBD->DTOGGLE) & USBD_DTOGGLE_VALUE_Msk) >> USBD_DTOGGLE_VALUE_Pos;
    return (nrf_usbd_dtoggle_t)retval;
}

bool nrf_usbd_ep_enable_check(uint8_t ep)
{
    NRFX_ASSERT(NRF_USBD_EP_VALIDATE(ep));
    uint8_t epnr = NRF_USBD_EP_NR_GET(ep);

    if (NRF_USBD_EPIN_CHECK(ep))
    {
        return 0 != (NRF_USBD->EPINEN & (1UL << epnr));
    }
    else
    {
        return 0 != (NRF_USBD->EPOUTEN & (1UL << epnr));
    }
}

void nrf_usbd_ep_enable(uint8_t ep)
{
    NRFX_ASSERT(NRF_USBD_EP_VALIDATE(ep));
    uint8_t epnr = NRF_USBD_EP_NR_GET(ep);

    if (NRF_USBD_EPIN_CHECK(ep))
    {
        NRF_USBD->EPINEN |= 1UL << epnr;
    }
    else
    {
        NRF_USBD->EPOUTEN |= 1UL << epnr;
    }
    __ISB();
    __DSB();
}

void nrf_usbd_ep_disable(uint8_t ep)
{
    NRFX_ASSERT(NRF_USBD_EP_VALIDATE(ep));
    uint8_t epnr = NRF_USBD_EP_NR_GET(ep);

    if (NRF_USBD_EPIN_CHECK(ep))
    {
        NRF_USBD->EPINEN &= ~(1UL << epnr);
    }
    else
    {
        NRF_USBD->EPOUTEN &= ~(1UL << epnr);
    }
    __ISB();
    __DSB();
}

void nrf_usbd_ep_all_disable(void)
{
    NRF_USBD->EPINEN  = USBD_EPINEN_IN0_Enable << USBD_EPINEN_IN0_Pos;
    NRF_USBD->EPOUTEN = USBD_EPOUTEN_OUT0_Enable << USBD_EPOUTEN_OUT0_Pos;
    __ISB();
    __DSB();
}

void nrf_usbd_ep_stall(uint8_t ep)
{
    NRFX_ASSERT(!NRF_USBD_EPISO_CHECK(ep));
    NRF_USBD->EPSTALL = (USBD_EPSTALL_STALL_Stall << USBD_EPSTALL_STALL_Pos) | ep;
    __ISB();
    __DSB();
}

void nrf_usbd_ep_unstall(uint8_t ep)
{
    NRFX_ASSERT(!NRF_USBD_EPISO_CHECK(ep));
    NRF_USBD->EPSTALL = (USBD_EPSTALL_STALL_UnStall << USBD_EPSTALL_STALL_Pos) | ep;
    __ISB();
    __DSB();
}

void nrf_usbd_isosplit_set(nrf_usbd_isosplit_t split)
{
    NRF_USBD->ISOSPLIT = split << USBD_ISOSPLIT_SPLIT_Pos;
}

nrf_usbd_isosplit_t nrf_usbd_isosplit_get(void)
{
    return (nrf_usbd_isosplit_t)
        (((NRF_USBD->ISOSPLIT) & USBD_ISOSPLIT_SPLIT_Msk) >> USBD_ISOSPLIT_SPLIT_Pos);
}

uint32_t nrf_usbd_framecntr_get(void)
{
    return NRF_USBD->FRAMECNTR;
}

void nrf_usbd_lowpower_enable(void)
{
    NRF_USBD->LOWPOWER = USBD_LOWPOWER_LOWPOWER_LowPower << USBD_LOWPOWER_LOWPOWER_Pos;
}

void nrf_usbd_lowpower_disable(void)
{
    NRF_USBD->LOWPOWER = USBD_LOWPOWER_LOWPOWER_ForceNormal << USBD_LOWPOWER_LOWPOWER_Pos;
}

bool nrf_usbd_lowpower_check(void)
{
    return (NRF_USBD->LOWPOWER != (USBD_LOWPOWER_LOWPOWER_ForceNormal << USBD_LOWPOWER_LOWPOWER_Pos));
}

void nrf_usbd_isoinconfig_set(nrf_usbd_isoinconfig_t config)
{
    NRF_USBD->ISOINCONFIG = ((uint32_t)config) << USBD_ISOINCONFIG_RESPONSE_Pos;
}

nrf_usbd_isoinconfig_t nrf_usbd_isoinconfig_get(void)
{
    return (nrf_usbd_isoinconfig_t)
        (((NRF_USBD->ISOINCONFIG) & USBD_ISOINCONFIG_RESPONSE_Msk) >> USBD_ISOINCONFIG_RESPONSE_Pos);
}

void nrf_usbd_ep_easydma_set(uint8_t ep, uint32_t ptr, uint32_t maxcnt)
{
    if (NRF_USBD_EPIN_CHECK(ep))
    {
        if (NRF_USBD_EPISO_CHECK(ep))
        {
            NRF_USBD->ISOIN.PTR    = ptr;
            NRF_USBD->ISOIN.MAXCNT = maxcnt;
        }
        else
        {
            uint8_t epnr = NRF_USBD_EP_NR_GET(ep);
            NRFX_ASSERT(epnr < NRFX_ARRAY_SIZE(NRF_USBD->EPIN));
            NRF_USBD->EPIN[epnr].PTR    = ptr;
            NRF_USBD->EPIN[epnr].MAXCNT = maxcnt;
        }
    }
    else
    {
        if (NRF_USBD_EPISO_CHECK(ep))
        {
            NRF_USBD->ISOOUT.PTR    = ptr;
            NRF_USBD->ISOOUT.MAXCNT = maxcnt;
        }
        else
        {
            uint8_t epnr = NRF_USBD_EP_NR_GET(ep);
            NRFX_ASSERT(epnr < NRFX_ARRAY_SIZE(NRF_USBD->EPOUT));
            NRF_USBD->EPOUT[epnr].PTR    = ptr;
            NRF_USBD->EPOUT[epnr].MAXCNT = maxcnt;
        }
    }
}

uint32_t nrf_usbd_ep_amount_get(uint8_t ep)
{
    uint32_t ret;

    if (NRF_USBD_EPIN_CHECK(ep))
    {
        if (NRF_USBD_EPISO_CHECK(ep))
        {
            ret = NRF_USBD->ISOIN.AMOUNT;
        }
        else
        {
            uint8_t epnr = NRF_USBD_EP_NR_GET(ep);
            NRFX_ASSERT(epnr < NRFX_ARRAY_SIZE(NRF_USBD->EPOUT));
            ret = NRF_USBD->EPIN[epnr].AMOUNT;
        }
    }
    else
    {
        if (NRF_USBD_EPISO_CHECK(ep))
        {
            ret = NRF_USBD->ISOOUT.AMOUNT;
        }
        else
        {
            uint8_t epnr = NRF_USBD_EP_NR_GET(ep);
            NRFX_ASSERT(epnr < NRFX_ARRAY_SIZE(NRF_USBD->EPOUT));
            ret = NRF_USBD->EPOUT[epnr].AMOUNT;
        }
    }

    return ret;
}

#endif /* SUPPRESS_INLINE_IMPLEMENTATION */

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* NRF_USBD_H__ */
