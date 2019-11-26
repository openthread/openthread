/*
 * Copyright (c) 2019, Nordic Semiconductor ASA
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

#ifndef NRFX_GPPI_H
#define NRFX_GPPI_H

#include <nrfx.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup nrfx_gppi Generic PPI layer
 * @{
 * @ingroup nrfx
 * @ingroup nrf_dppi
 * @ingroup nrf_ppi
 *
 * @brief Helper layer that provides the common functionality of PPI and DPPI drivers.
 *
 * Use PPI and DPPI drivers directly.
 * This layer is provided only to help create generic code that can be built
 * for SoCs equipped with either of these peripherals. When using this layer,
 * take into account that there are significant differences between the PPI and DPPI
 * interfaces that affect the behavior of this layer.
 *
 * One difference is that PPI allows associating of one task or event with
 * more than one channel, whereas DPPI does not allow this. In DPPI, the second
 * association overwrites the first one. Consequently, this helper layer cannot
 * be used in applications that need to connect a task or event to multiple
 * channels.
 *
 * Another difference is that in DPPI one channel can be associated with
 * multiple tasks and multiple events, while in PPI this is not possible (with
 * the exception of the association of a second task as a fork). Because of
 * this difference, it is important to clear the previous endpoints of the channel that
 * is to be reused with some different ones. Otherwise, the behavior of this
 * helper layer will be different, depending on the actual interface used:
 * in DPPI the channel configuration will be extended with the new endpoints, and
 * in PPI the new endpoints will replace the previous ones.
 */

#if defined(PPI_PRESENT)
#include <hal/nrf_ppi.h>

typedef enum
{
    NRFX_GPPI_CHANNEL_GROUP0 = NRF_PPI_CHANNEL_GROUP0,
    NRFX_GPPI_CHANNEL_GROUP1 = NRF_PPI_CHANNEL_GROUP1,
    NRFX_GPPI_CHANNEL_GROUP2 = NRF_PPI_CHANNEL_GROUP2,
    NRFX_GPPI_CHANNEL_GROUP3 = NRF_PPI_CHANNEL_GROUP3,
#if (PPI_GROUP_NUM > 4)
    NRFX_GPPI_CHANNEL_GROUP4 = NRF_PPI_CHANNEL_GROUP4,
    NRFX_GPPI_CHANNEL_GROUP5 = NRF_PPI_CHANNEL_GROUP5,
#endif
} nrfx_gppi_channel_group_t;

typedef enum
{
    NRFX_GPPI_TASK_CHG0_EN  = NRF_PPI_TASK_CHG0_EN,
    NRFX_GPPI_TASK_CHG0_DIS = NRF_PPI_TASK_CHG0_DIS,
    NRFX_GPPI_TASK_CHG1_EN  = NRF_PPI_TASK_CHG1_EN,
    NRFX_GPPI_TASK_CHG1_DIS = NRF_PPI_TASK_CHG1_DIS,
    NRFX_GPPI_TASK_CHG2_EN  = NRF_PPI_TASK_CHG2_EN,
    NRFX_GPPI_TASK_CHG2_DIS = NRF_PPI_TASK_CHG2_DIS,
    NRFX_GPPI_TASK_CHG3_EN  = NRF_PPI_TASK_CHG3_EN,
    NRFX_GPPI_TASK_CHG3_DIS = NRF_PPI_TASK_CHG3_DIS,
#if (PPI_GROUP_NUM > 4)
    NRFX_GPPI_TASK_CHG4_EN  = NRF_PPI_TASK_CHG4_EN,
    NRFX_GPPI_TASK_CHG4_DIS = NRF_PPI_TASK_CHG4_DIS,
    NRFX_GPPI_TASK_CHG5_EN  = NRF_PPI_TASK_CHG5_EN,
    NRFX_GPPI_TASK_CHG5_DIS = NRF_PPI_TASK_CHG5_DIS
#endif
} nrfx_gppi_task_t;

#elif defined(DPPI_PRESENT)
#include <hal/nrf_dppi.h>

typedef enum
{
    NRFX_GPPI_CHANNEL_GROUP0 = NRF_DPPI_CHANNEL_GROUP0,
    NRFX_GPPI_CHANNEL_GROUP1 = NRF_DPPI_CHANNEL_GROUP1,
    NRFX_GPPI_CHANNEL_GROUP2 = NRF_DPPI_CHANNEL_GROUP2,
    NRFX_GPPI_CHANNEL_GROUP3 = NRF_DPPI_CHANNEL_GROUP3,
    NRFX_GPPI_CHANNEL_GROUP4 = NRF_DPPI_CHANNEL_GROUP4,
    NRFX_GPPI_CHANNEL_GROUP5 = NRF_DPPI_CHANNEL_GROUP5,
} nrfx_gppi_channel_group_t;

typedef enum
{
    NRFX_GPPI_TASK_CHG0_EN  = NRF_DPPI_TASK_CHG0_EN,
    NRFX_GPPI_TASK_CHG0_DIS = NRF_DPPI_TASK_CHG0_DIS,
    NRFX_GPPI_TASK_CHG1_EN  = NRF_DPPI_TASK_CHG1_EN,
    NRFX_GPPI_TASK_CHG1_DIS = NRF_DPPI_TASK_CHG1_DIS,
    NRFX_GPPI_TASK_CHG2_EN  = NRF_DPPI_TASK_CHG2_EN,
    NRFX_GPPI_TASK_CHG2_DIS = NRF_DPPI_TASK_CHG2_DIS,
    NRFX_GPPI_TASK_CHG3_EN  = NRF_DPPI_TASK_CHG3_EN,
    NRFX_GPPI_TASK_CHG3_DIS = NRF_DPPI_TASK_CHG3_DIS,
    NRFX_GPPI_TASK_CHG4_EN  = NRF_DPPI_TASK_CHG4_EN,
    NRFX_GPPI_TASK_CHG4_DIS = NRF_DPPI_TASK_CHG4_DIS,
    NRFX_GPPI_TASK_CHG5_EN  = NRF_DPPI_TASK_CHG5_EN,
    NRFX_GPPI_TASK_CHG5_DIS = NRF_DPPI_TASK_CHG5_DIS
} nrfx_gppi_task_t;

#elif defined(__NRFX_DOXYGEN__)

/** @brief Generic PPI channel groups. */
typedef enum
{
    NRFX_GPPI_CHANNEL_GROUP0, /**< Channel group 0.*/
    NRFX_GPPI_CHANNEL_GROUP1, /**< Channel group 1.*/
    NRFX_GPPI_CHANNEL_GROUP2, /**< Channel group 2.*/
    NRFX_GPPI_CHANNEL_GROUP3, /**< Channel group 3.*/
    NRFX_GPPI_CHANNEL_GROUP4, /**< Channel group 4.*/
    NRFX_GPPI_CHANNEL_GROUP5, /**< Channel group 5.*/
} nrfx_gppi_channel_group_t;

/** @brief Generic PPI tasks. */
typedef enum
{
    NRFX_GPPI_TASK_CHG0_EN,  /**< Task for enabling channel group 0 */
    NRFX_GPPI_TASK_CHG0_DIS, /**< Task for disabling channel group 0 */
    NRFX_GPPI_TASK_CHG1_EN,  /**< Task for enabling channel group 1 */
    NRFX_GPPI_TASK_CHG1_DIS, /**< Task for disabling channel group 1 */
    NRFX_GPPI_TASK_CHG2_EN,  /**< Task for enabling channel group 2 */
    NRFX_GPPI_TASK_CHG2_DIS, /**< Task for disabling channel group 2 */
    NRFX_GPPI_TASK_CHG3_EN,  /**< Task for enabling channel group 3 */
    NRFX_GPPI_TASK_CHG3_DIS, /**< Task for disabling channel group 3 */
    NRFX_GPPI_TASK_CHG4_EN,  /**< Task for enabling channel group 4 */
    NRFX_GPPI_TASK_CHG4_DIS, /**< Task for disabling channel group 4 */
    NRFX_GPPI_TASK_CHG5_EN,  /**< Task for enabling channel group 5 */
    NRFX_GPPI_TASK_CHG5_DIS, /**< Task for disabling channel group 5 */
} nrfx_gppi_task_t;
#endif // defined(__NRFX_DOXYGEN__)

/**
 * @brief Function for checking if a given channel is enabled.
 *
 * @param[in] channel Channel to check.
 *
 * @retval true  The channel is enabled.
 * @retval false The channel is not enabled.
 */
__STATIC_INLINE bool nrfx_gppi_channel_check(uint8_t channel);

/** @brief Function for disabling all channels. */
__STATIC_INLINE void nrfx_gppi_channels_disable_all(void);

/**
 * @brief Function for enabling multiple channels.
 *
 * The bits in @c mask value correspond to particular channels. This means that
 * writing 1 to bit 0 enables channel 0, writing 1 to bit 1 enables channel 1, etc.
 *
 * @param[in] mask Channel mask.
 */
__STATIC_INLINE void nrfx_gppi_channels_enable(uint32_t mask);

/**
 * @brief Function for disabling multiple channels.
 *
 * The bits in @c mask value correspond to particular channels. This means that
 * writing 1 to bit 0 disables channel 0, writing 1 to bit 1 disables channel 1, etc.
 *
 * @param[in] mask Channel mask.
 */
__STATIC_INLINE void nrfx_gppi_channels_disable(uint32_t mask);

/**
 * @brief Function for associating a given channel with the specified event register.
 *
 * This function sets the DPPI publish configuration for a given event
 * or sets the PPI event endpoint register.
 *
 * @param[in] channel Channel to which to assign the event.
 * @param[in] eep     Address of the event register.
 */
__STATIC_INLINE void nrfx_gppi_event_endpoint_setup(uint8_t channel, uint32_t eep);

/**
 * @brief Function for associating a given channel with the specified task register.
 *
 * This function sets the DPPI subscribe configuration for a given task
 * or sets the PPI task endpoint register.
 *
 * @param[in] channel Channel to which to assign the task.
 * @param[in] tep     Address of the task register.
 */
__STATIC_INLINE void nrfx_gppi_task_endpoint_setup(uint8_t channel, uint32_t tep);

/**
 * @brief Function for setting up the event and task endpoints for a given channel.
 *
 * @param[in] channel Channel to which the given endpoints are assigned.
 * @param[in] eep     Address of the event register.
 * @param[in] tep     Address of the task register.
 */
__STATIC_INLINE void nrfx_gppi_channel_endpoints_setup(uint8_t  channel,
                                                       uint32_t eep,
                                                       uint32_t tep);

/**
 * @brief Function for clearing the DPPI publish configuration for a given event
 * register or for clearing the PPI event endpoint register.
 *
 * @param[in] channel Channel for which to clear the event endpoint. Not used in DPPI.
 * @param[in] eep     Address of the event register. Not used in PPI.
 */
__STATIC_INLINE void nrfx_gppi_event_endpoint_clear(uint8_t channel, uint32_t eep);

/**
 * @brief Function for clearing the DPPI subscribe configuration for a given task
 * register or for clearing the PPI task endpoint register.
 *
 * @param[in] channel Channel from which to disconnect the task enpoint. Not used in DPPI.
 * @param[in] tep     Address of the task register. Not used in PPI.
 */
__STATIC_INLINE void nrfx_gppi_task_endpoint_clear(uint8_t channel, uint32_t tep);


#if defined(PPI_FEATURE_FORKS_PRESENT) || defined(DPPI_PRESENT) || defined(__NRFX_DOXYGEN__)
/**
 * @brief Function for setting up the task endpoint for a given PPI fork or for
 * associating the DPPI channel with an additional task register.
 *
 * @param[in] channel  Channel to which the given fork endpoint is assigned.
 * @param[in] fork_tep Address of the task register.
 */
__STATIC_INLINE void nrfx_gppi_fork_endpoint_setup(uint8_t channel, uint32_t fork_tep);

/**
 * @brief Function for clearing the task endpoint for a given PPI fork or for clearing
 * the DPPI subscribe register.
 *
 * @param[in] channel  Channel for which to clear the fork endpoint. Not used in DPPI.
 * @param[in] fork_tep Address of the task register. Not used in PPI.
 */
__STATIC_INLINE void nrfx_gppi_fork_endpoint_clear(uint8_t channel, uint32_t fork_tep);
#endif // defined(PPI_FEATURE_FORKS_PRESENT) || defined(DPPI_PRESENT) || defined(__NRFX_DOXYGEN__)

/**
 * @brief Function for including multiple channels in a channel group.
 *
 * @param[in] channel_mask  Channels to be included in the group.
 * @param[in] channel_group Channel group.
 */
__STATIC_INLINE void nrfx_gppi_channels_include_in_group(uint32_t                  channel_mask,
                                                         nrfx_gppi_channel_group_t channel_group);

/**
 * @brief Function for removing multiple channels from a channel group.
 *
 * @param[in] channel_mask  Channels to be removed from the group.
 * @param[in] channel_group Channel group.
 */
__STATIC_INLINE void nrfx_gppi_channels_remove_from_group(uint32_t                  channel_mask,
                                                          nrfx_gppi_channel_group_t channel_group);

/**
 * @brief Function for removing all channels from a channel group.
 *
 * @param[in] channel_group Channel group.
 */
__STATIC_INLINE void nrfx_gppi_group_clear(nrfx_gppi_channel_group_t channel_group);

/**
 * @brief Function for enabling a channel group.
 *
 * @param[in] channel_group Channel group.
 */
__STATIC_INLINE void nrfx_gppi_group_enable(nrfx_gppi_channel_group_t channel_group);

/**
 * @brief Function for disabling a group.
 *
 * @param[in] channel_group Channel group.
 */
__STATIC_INLINE void nrfx_gppi_group_disable(nrfx_gppi_channel_group_t channel_group);

/**
 * @brief Function for activating a task.
 *
 * @param[in] task Task to be activated.
 */
__STATIC_INLINE void nrfx_gppi_task_trigger(nrfx_gppi_task_t task);

/**
 * @brief Function for returning the address of a specific task register.
 *
 * @param[in] task PPI or DPPI task.
 *
 * @return Address of the requested task register.
 */
__STATIC_INLINE uint32_t nrfx_gppi_task_address_get(nrfx_gppi_task_t task);

/**
 * @brief Function for returning the address of a channel group disable task.
 *
 * @param[in] group Channel group.
 *
 * @return Disable task address of the specified group.
 */
__STATIC_INLINE nrfx_gppi_task_t nrfx_gppi_group_disable_task_get(nrfx_gppi_channel_group_t group);

/**
 * @brief Function for returning the address of a channel group enable task.
 *
 * @param[in] group Channel group.
 *
 * @return Enable task address of the specified group.
 */
__STATIC_INLINE nrfx_gppi_task_t nrfx_gppi_group_enable_task_get(nrfx_gppi_channel_group_t group);

/** @} */

#if defined(PPI_PRESENT)

__STATIC_INLINE bool nrfx_gppi_channel_check(uint8_t channel)
{
    return (nrf_ppi_channel_enable_get((nrf_ppi_channel_t)channel) == NRF_PPI_CHANNEL_ENABLED);
}

__STATIC_INLINE void nrfx_gppi_channels_disable_all(void)
{
    nrf_ppi_channel_disable_all();
}

__STATIC_INLINE void nrfx_gppi_channels_enable(uint32_t mask)
{
    nrf_ppi_channels_enable(mask);
}

__STATIC_INLINE void nrfx_gppi_channels_disable(uint32_t mask)
{
    nrf_ppi_channels_disable(mask);
}

__STATIC_INLINE void nrfx_gppi_event_endpoint_setup(uint8_t channel, uint32_t eep)
{
    nrf_ppi_event_endpoint_setup((nrf_ppi_channel_t)channel, eep);
}

__STATIC_INLINE void nrfx_gppi_task_endpoint_setup(uint8_t channel, uint32_t tep)
{
    nrf_ppi_task_endpoint_setup((nrf_ppi_channel_t)channel, tep);
}

__STATIC_INLINE void nrfx_gppi_channel_endpoints_setup(uint8_t  channel,
                                                       uint32_t eep,
                                                       uint32_t tep)
{
    nrf_ppi_channel_endpoint_setup((nrf_ppi_channel_t)channel, eep, tep);
}

__STATIC_INLINE void nrfx_gppi_event_endpoint_clear(uint8_t channel, uint32_t eep)
{
    (void)eep;
     nrf_ppi_event_endpoint_setup((nrf_ppi_channel_t)channel, 0);
}

__STATIC_INLINE void nrfx_gppi_task_endpoint_clear(uint8_t channel, uint32_t tep)
{
    (void)tep;
    nrf_ppi_task_endpoint_setup((nrf_ppi_channel_t)channel, 0);
}

#if defined(PPI_FEATURE_FORKS_PRESENT)
__STATIC_INLINE void nrfx_gppi_fork_endpoint_setup(uint8_t channel, uint32_t fork_tep)
{
    nrf_ppi_fork_endpoint_setup((nrf_ppi_channel_t)channel, fork_tep);
}

__STATIC_INLINE void nrfx_gppi_fork_endpoint_clear(uint8_t channel, uint32_t fork_tep)
{
    (void)fork_tep;
    nrf_ppi_fork_endpoint_setup((nrf_ppi_channel_t)channel, 0);
}
#endif

__STATIC_INLINE void nrfx_gppi_channels_include_in_group(uint32_t                  channel_mask,
                                                         nrfx_gppi_channel_group_t channel_group)
{
    nrf_ppi_channels_include_in_group(channel_mask, channel_group);
}

__STATIC_INLINE void nrfx_gppi_channels_remove_from_group(uint32_t                  channel_mask,
                                                          nrfx_gppi_channel_group_t channel_group)
{
    nrf_ppi_channels_remove_from_group(channel_mask, channel_group);
}

__STATIC_INLINE void nrfx_gppi_group_clear(nrfx_gppi_channel_group_t channel_group)
{
    nrf_ppi_channel_group_clear(channel_group);
}

__STATIC_INLINE void nrfx_gppi_group_enable(nrfx_gppi_channel_group_t channel_group)
{
    nrf_ppi_group_enable(channel_group);
}

__STATIC_INLINE void nrfx_gppi_group_disable(nrfx_gppi_channel_group_t channel_group)
{
    nrf_ppi_group_disable(channel_group);
}

__STATIC_INLINE void nrfx_gppi_task_trigger(nrfx_gppi_task_t task)
{
    nrf_ppi_task_trigger(task);
}

__STATIC_INLINE uint32_t nrfx_gppi_task_address_get(nrfx_gppi_task_t task)
{
    return (uint32_t)nrf_ppi_task_address_get(task);
}

__STATIC_INLINE nrfx_gppi_task_t nrfx_gppi_group_disable_task_get(nrfx_gppi_channel_group_t group)
{
    return (nrfx_gppi_task_t)nrf_ppi_group_disable_task_get((uint8_t)group);
}

__STATIC_INLINE nrfx_gppi_task_t nrfx_gppi_group_enable_task_get(nrfx_gppi_channel_group_t group)
{
    return (nrfx_gppi_task_t)nrf_ppi_group_enable_task_get((uint8_t)group);
}

#elif defined(DPPI_PRESENT)

__STATIC_INLINE bool nrfx_gppi_channel_check(uint8_t channel)
{
    return nrf_dppi_channel_check(NRF_DPPIC, channel);
}

__STATIC_INLINE void nrfx_gppi_channels_disable_all(void)
{
    nrf_dppi_channels_disable_all(NRF_DPPIC);
}

__STATIC_INLINE void nrfx_gppi_channels_enable(uint32_t mask)
{
    nrf_dppi_channels_enable(NRF_DPPIC, mask);
}

__STATIC_INLINE void nrfx_gppi_channels_disable(uint32_t mask)
{
    nrf_dppi_channels_disable(NRF_DPPIC, mask);
}

__STATIC_INLINE void nrfx_gppi_task_trigger(nrfx_gppi_task_t task)
{
    nrf_dppi_task_trigger(NRF_DPPIC, task);
}

__STATIC_INLINE void nrfx_gppi_event_endpoint_setup(uint8_t channel, uint32_t eep)
{
    NRFX_ASSERT(eep);
    *((volatile uint32_t *)(eep + 0x80uL)) = ((uint32_t)channel | DPPIC_SUBSCRIBE_CHG_EN_EN_Msk);
}

__STATIC_INLINE void nrfx_gppi_task_endpoint_setup(uint8_t channel, uint32_t tep)
{
    NRFX_ASSERT(tep);
    *((volatile uint32_t *)(tep + 0x80uL)) = ((uint32_t)channel | DPPIC_SUBSCRIBE_CHG_EN_EN_Msk);
}

__STATIC_INLINE void nrfx_gppi_channel_endpoints_setup(uint8_t  channel,
                                                       uint32_t eep,
                                                       uint32_t tep)
{
    nrfx_gppi_event_endpoint_setup(channel, eep);
    nrfx_gppi_task_endpoint_setup(channel, tep);
}

__STATIC_INLINE void nrfx_gppi_event_endpoint_clear(uint8_t channel, uint32_t eep)
{
    NRFX_ASSERT(eep);
    (void)channel;
    *((volatile uint32_t *)(eep + 0x80uL)) = 0;
}

__STATIC_INLINE void nrfx_gppi_task_endpoint_clear(uint8_t channel, uint32_t tep)
{
    NRFX_ASSERT(tep);
    (void)channel;
    *((volatile uint32_t *)(tep + 0x80uL)) = 0;
}

__STATIC_INLINE void nrfx_gppi_fork_endpoint_setup(uint8_t channel, uint32_t fork_tep)
{
    nrfx_gppi_task_endpoint_setup(channel, fork_tep);
}

__STATIC_INLINE void nrfx_gppi_fork_endpoint_clear(uint8_t channel, uint32_t fork_tep)
{
    nrfx_gppi_task_endpoint_clear(channel, fork_tep);
}

__STATIC_INLINE void nrfx_gppi_channels_include_in_group(uint32_t                  channel_mask,
                                                         nrfx_gppi_channel_group_t channel_group)
{
    nrf_dppi_channels_include_in_group(NRF_DPPIC, channel_mask, channel_group);
}

__STATIC_INLINE void nrfx_gppi_channels_remove_from_group(uint32_t                  channel_mask,
                                                          nrfx_gppi_channel_group_t channel_group)
{
    nrf_dppi_channels_remove_from_group(NRF_DPPIC, channel_mask, channel_group);
}

__STATIC_INLINE void nrfx_gppi_group_clear(nrfx_gppi_channel_group_t channel_group)
{
    nrf_dppi_group_clear(NRF_DPPIC, channel_group);
}

__STATIC_INLINE void nrfx_gppi_group_enable(nrfx_gppi_channel_group_t channel_group)
{
    nrf_dppi_group_enable(NRF_DPPIC, channel_group);
}

__STATIC_INLINE void nrfx_gppi_group_disable(nrfx_gppi_channel_group_t channel_group)
{
    nrf_dppi_group_disable(NRF_DPPIC, channel_group);
}

__STATIC_INLINE uint32_t nrfx_gppi_task_address_get(nrfx_gppi_task_t gppi_task)
{
    return (uint32_t) ((uint8_t *) NRF_DPPIC + (uint32_t) gppi_task);
}

__STATIC_INLINE nrfx_gppi_task_t nrfx_gppi_group_disable_task_get(nrfx_gppi_channel_group_t group)
{
    return (nrfx_gppi_task_t) nrf_dppi_group_disable_task_get((uint8_t)group);
}

__STATIC_INLINE nrfx_gppi_task_t nrfx_gppi_group_enable_task_get(nrfx_gppi_channel_group_t group)
{
    return (nrfx_gppi_task_t) nrf_dppi_group_enable_task_get((uint8_t)group);
}

#else
#error "Neither PPI nor DPPI is present in the SoC currently in use."
#endif

#ifdef __cplusplus
}
#endif

#endif // NRFX_GPPI_H
