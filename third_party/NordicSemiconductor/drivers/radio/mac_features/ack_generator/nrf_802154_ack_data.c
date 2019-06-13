/* Copyright (c) 2017 - 2018, Nordic Semiconductor ASA
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   1. Redistributions of source code must retain the above copyright notice, this
 *      list of conditions and the following disclaimer.
 *
 *   2. Redistributions in binary form must reproduce the above copyright notice,
 *      this list of conditions and the following disclaimer in the documentation
 *      and/or other materials provided with the distribution.
 *
 *   3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *      contributors may be used to endorse or promote products derived from
 *      this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

/**
 * @file
 *   This file implements procedures to set pending bit and 802.15.4-2015 information
 *   elements in nRF 802.15.4 radio driver.
 *
 */

#include "nrf_802154_ack_data.h"

#include <assert.h>
#include <string.h>

#include "mac_features/nrf_802154_frame_parser.h"
#include "nrf_802154_config.h"
#include "nrf_802154_const.h"
#include "nrf_802154_types.h"

/// Maximum number of Short Addresses of nodes for which there is ACK data to set.
#define NUM_SHORT_ADDRESSES    NRF_802154_PENDING_SHORT_ADDRESSES
/// Maximum number of Extended Addresses of nodes for which there is ACK data to set.
#define NUM_EXTENDED_ADDRESSES NRF_802154_PENDING_EXTENDED_ADDRESSES

/// Structure representing pending bit setting variables.
typedef struct
{
    bool     enabled;                                                      /// If setting pending bit is enabled.
    uint8_t  short_addr[NUM_SHORT_ADDRESSES][SHORT_ADDRESS_SIZE];          /// Array of short addresses of nodes for which there is pending data in the buffer.
    uint8_t  extended_addr[NUM_EXTENDED_ADDRESSES][EXTENDED_ADDRESS_SIZE]; /// Array of extended addresses of nodes for which there is pending data in the buffer.
    uint32_t num_of_short_addr;                                            /// Current number of short addresses of nodes for which there is pending data in the buffer.
    uint32_t num_of_ext_addr;                                              /// Current number of extended addresses of nodes for which there is pending data in the buffer.
} pending_bit_arrays_t;

// Structure representing a single IE record.
typedef struct
{
    uint8_t p_data[NRF_802154_MAX_ACK_IE_SIZE]; /// Pointer to IE data buffer.
    uint8_t len;                                /// Length of the buffer.
} ie_data_t;

// Structure representing IE records sent in an ACK message to a given short address.
typedef struct
{
    uint8_t   addr[SHORT_ADDRESS_SIZE]; /// Short address of peer node.
    ie_data_t ie_data;                  /// Pointer to IE records.
} ack_short_ie_data_t;

// Structure representing IE records sent in an ACK message to a given extended address.
typedef struct
{
    uint8_t   addr[EXTENDED_ADDRESS_SIZE]; /// Extended address of peer node.
    ie_data_t ie_data;                     /// Pointer to IE records.
} ack_ext_ie_data_t;

// Structure representing IE data setting variables.
typedef struct
{
    ack_short_ie_data_t short_data[NUM_SHORT_ADDRESSES];  /// Array of short addresses and IE records sent to these addresses.
    ack_ext_ie_data_t   ext_data[NUM_EXTENDED_ADDRESSES]; /// Array of extended addresses and IE records sent to these addresses.
    uint32_t            num_of_short_data;                /// Current number of short addresses stored in @p short_data.
    uint32_t            num_of_ext_data;                  /// Current number of extended addresses stored in @p ext_data.
} ie_arrays_t;

// TODO: Combine below arrays to perform binary search only once per Ack generation.
static pending_bit_arrays_t m_pending_bit;
static ie_arrays_t          m_ie;

/***************************************************************************************************
 * @section Array handling helper functions
 **************************************************************************************************/

/**
 * @brief Compare two extended addresses.
 *
 * @param[in]  p_first_addr     Pointer to a first address that should be compared.
 * @param[in]  p_second_addr    Pointer to a second address that should be compared.
 *
 * @retval -1  First address is less than the second address.
 * @retval  0  First address is equal to the second address.
 * @retval  1  First address is greater than the second address.
 */
static int8_t extended_addr_compare(const uint8_t * p_first_addr, const uint8_t * p_second_addr)
{
    uint32_t first_addr;
    uint32_t second_addr;

    // Compare extended address in two steps to prevent unaligned access error.
    for (uint32_t i = 0; i < EXTENDED_ADDRESS_SIZE / sizeof(uint32_t); i++)
    {
        first_addr  = *(uint32_t *)(p_first_addr + (i * sizeof(uint32_t)));
        second_addr = *(uint32_t *)(p_second_addr + (i * sizeof(uint32_t)));

        if (first_addr < second_addr)
        {
            return -1;
        }
        else if (first_addr > second_addr)
        {
            return 1;
        }
    }

    return 0;
}

/**
 * @brief Compare two short addresses.
 *
 * @param[in]  p_first_addr     Pointer to a first address that should be compared.
 * @param[in]  p_second_addr    Pointer to a second address that should be compared.
 *
 * @retval -1  First address is less than the second address.
 * @retval  0  First address is equal to the second address.
 * @retval  1  First address is greater than the second address.
 */
static int8_t short_addr_compare(const uint8_t * p_first_addr, const uint8_t * p_second_addr)
{
    uint16_t first_addr  = *(uint16_t *)(p_first_addr);
    uint16_t second_addr = *(uint16_t *)(p_second_addr);

    if (first_addr < second_addr)
    {
        return -1;
    }
    else if (first_addr > second_addr)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

/**
 * @brief Compare two addresses.
 *
 * @param[in]  p_first_addr     Pointer to a first address that should be compared.
 * @param[in]  p_second_addr    Pointer to a second address that should be compared.
 * @param[in]  extended         Indication if @p p_first_addr and @p p_second_addr are extended or short addresses.
 *
 * @retval -1  First address is less than the second address.
 * @retval  0  First address is equal to the second address.
 * @retval  1  First address is greater than the second address.
 */
static int8_t addr_compare(const uint8_t * p_first_addr,
                           const uint8_t * p_second_addr,
                           bool            extended)
{
    if (extended)
    {
        return extended_addr_compare(p_first_addr, p_second_addr);
    }
    else
    {
        return short_addr_compare(p_first_addr, p_second_addr);
    }
}

/**
 * @brief Perform a binary search for an address in a list of addresses.
 *
 * @param[in]  p_addr           Pointer to an address that is searched for.
 * @param[in]  p_addr_array     Pointer to a list of addresses to be searched.
 * @param[out] p_location       If the address @p p_addr appears in the list, this is its index in the address list.
 *                              Otherwise, it is the index which @p p_addr would have if it was placed in the list
 *                              (ascending order assumed).
 * @param[in]  extended         Indication if @p p_addr is an extended or a short addresses.
 *
 * @retval true   Address @p p_addr is in the list.
 * @retval false  Address @p p_addr is not in the list.
 */
static bool addr_binary_search(const uint8_t * p_addr,
                               const uint8_t * p_addr_array,
                               uint32_t      * p_location,
                               uint8_t         data_type,
                               bool            extended)
{
    uint32_t addr_array_len = 0;
    uint8_t  entry_size     = 0;

    switch (data_type)
    {
        case NRF_802154_ACK_DATA_PENDING_BIT:
            entry_size     = extended ? EXTENDED_ADDRESS_SIZE : SHORT_ADDRESS_SIZE;
            addr_array_len = extended ?
                             m_pending_bit.num_of_ext_addr : m_pending_bit.num_of_short_addr;
            break;

        case NRF_802154_ACK_DATA_IE:
            entry_size     = extended ? sizeof(ack_ext_ie_data_t) : sizeof(ack_short_ie_data_t);
            addr_array_len = extended ? m_ie.num_of_ext_data : m_ie.num_of_short_data;
            break;

        default:
            assert(false);
            break;
    }

    // The actual algorithm
    int32_t  low      = 0;
    uint32_t midpoint = 0;
    int32_t  high     = addr_array_len;

    while (high >= low)
    {
        midpoint = (uint32_t)(low + (high - low) / 2);

        if (midpoint >= addr_array_len)
        {
            break;
        }

        switch (addr_compare(p_addr, p_addr_array + entry_size * midpoint, extended))
        {
            case -1:
                high = (int32_t)(midpoint - 1);
                break;

            case 0:
                *p_location = midpoint;
                return true;

            case 1:
                low = (int32_t)(midpoint + 1);
                break;

            default:
                break;
        }
    }

    /* If in the last iteration of the loop the last case was utilized, it means that the midpoint
     * found by the algorithm is less than the address to be added. The midpoint should be therefore
     * shifted to the next position. As a simplified example, a { 1, 3, 4 } array can be considered.
     * Suppose that a number equal to 2 is about to be added to the array. At the beginning of the
     * last iteration, midpoint is equal to 1 and low and high are equal to 0. Midpoint is then set
     * to 0 and with last case being utilized, low is set to 1. However, midpoint equal to 0 is
     * incorrect, as in the last iteration first element of the array proves to be less than the
     * element to be added to the array. With the below code, midpoint is then shifted to 1. */
    if ((uint32_t)low == midpoint + 1)
    {
        midpoint++;
    }

    *p_location = midpoint;
    return false;
}

/**
 * @brief Find an address in a list of addresses.
 *
 * @param[in]  p_addr           Pointer to an address that is searched for.
 * @param[out] p_location       If the address @p p_addr appears in the list, this is its index in the address list.
 *                              Otherwise, it is the index which @p p_addr would have if it was placed in the list
 *                              (ascending order assumed).
 * @param[in]  extended         Indication if @p p_addr is an extended or a short addresses.
 *
 * @retval true   Address @p p_addr is in the list.
 * @retval false  Address @p p_addr is not in the list.
 */
static bool addr_index_find(const uint8_t * p_addr,
                            uint32_t      * p_location,
                            uint8_t         data_type,
                            bool            extended)
{
    uint8_t * p_addr_array;

    switch (data_type)
    {
        case NRF_802154_ACK_DATA_PENDING_BIT:
            p_addr_array = extended ? (uint8_t *)m_pending_bit.extended_addr :
                           (uint8_t *)m_pending_bit.short_addr;
            break;

        case NRF_802154_ACK_DATA_IE:
            p_addr_array = extended ? (uint8_t *)m_ie.ext_data : (uint8_t *)m_ie.short_data;
            break;

        default:
            assert(false);
            break;
    }

    return addr_binary_search(p_addr, p_addr_array, p_location, data_type, extended);
}

/**
 * @brief Add an address to the address list in ascending order.
 *
 * @param[in]  p_addr           Pointer to the address to be added.
 * @param[in]  location         Index of the location where @p p_addr should be added.
 * @param[in]  extended         Indication if @p p_addr is an extended or a short addresses.
 *
 * @retval true   Address @p p_addr has been added to the list successfully.
 * @retval false  Address @p p_addr could not be added to the list.
 */
static bool addr_add(const uint8_t * p_addr, uint32_t location, uint8_t data_type, bool extended)
{
    uint32_t * p_addr_array_len;
    uint32_t   max_addr_array_len;
    uint8_t  * p_addr_array;
    uint8_t    entry_size;

    switch (data_type)
    {
        case NRF_802154_ACK_DATA_PENDING_BIT:
            if (extended)
            {
                p_addr_array       = (uint8_t *)m_pending_bit.extended_addr;
                max_addr_array_len = NUM_EXTENDED_ADDRESSES;
                p_addr_array_len   = &m_pending_bit.num_of_ext_addr;
                entry_size         = EXTENDED_ADDRESS_SIZE;
            }
            else
            {
                p_addr_array       = (uint8_t *)m_pending_bit.short_addr;
                max_addr_array_len = NUM_SHORT_ADDRESSES;
                p_addr_array_len   = &m_pending_bit.num_of_short_addr;
                entry_size         = SHORT_ADDRESS_SIZE;
            }
            break;

        case NRF_802154_ACK_DATA_IE:
            if (extended)
            {
                p_addr_array       = (uint8_t *)m_ie.ext_data;
                max_addr_array_len = NUM_EXTENDED_ADDRESSES;
                p_addr_array_len   = &m_ie.num_of_ext_data;
                entry_size         = sizeof(ack_ext_ie_data_t);
            }
            else
            {
                p_addr_array       = (uint8_t *)m_ie.short_data;
                max_addr_array_len = NUM_SHORT_ADDRESSES;
                p_addr_array_len   = &m_ie.num_of_short_data;
                entry_size         = sizeof(ack_short_ie_data_t);
            }
            break;

        default:
            assert(false);
            break;
    }

    if (*p_addr_array_len == max_addr_array_len)
    {
        return false;
    }

    memmove(p_addr_array + entry_size * (location + 1),
            p_addr_array + entry_size * (location),
            (*p_addr_array_len - location) * entry_size);

    memcpy(p_addr_array + entry_size * location,
           p_addr,
           extended ? EXTENDED_ADDRESS_SIZE : SHORT_ADDRESS_SIZE);

    (*p_addr_array_len)++;

    return true;
}

/**
 * @brief Remove an address from the address list keeping it in ascending order.
 *
 * @param[in]  location     Index of the element to be removed from the list.
 * @param[in]  extended     Indication if address to remove is an extended or a short address.
 *
 * @retval true   Address @p p_addr has been removed from the list successfully.
 * @retval false  Address @p p_addr could not removed from the list.
 */
static bool addr_remove(uint32_t location, uint8_t data_type, bool extended)
{
    uint32_t * p_addr_array_len;
    uint8_t  * p_addr_array;
    uint8_t    entry_size;

    switch (data_type)
    {
        case NRF_802154_ACK_DATA_PENDING_BIT:
            if (extended)
            {
                p_addr_array     = (uint8_t *)m_pending_bit.extended_addr;
                p_addr_array_len = &m_pending_bit.num_of_ext_addr;
                entry_size       = EXTENDED_ADDRESS_SIZE;
            }
            else
            {
                p_addr_array     = (uint8_t *)m_pending_bit.short_addr;
                p_addr_array_len = &m_pending_bit.num_of_short_addr;
                entry_size       = SHORT_ADDRESS_SIZE;
            }
            break;

        case NRF_802154_ACK_DATA_IE:
            if (extended)
            {
                p_addr_array     = (uint8_t *)m_ie.ext_data;
                p_addr_array_len = &m_ie.num_of_ext_data;
                entry_size       = sizeof(ack_ext_ie_data_t);
            }
            else
            {
                p_addr_array     = (uint8_t *)m_ie.short_data;
                p_addr_array_len = &m_ie.num_of_short_data;
                entry_size       = sizeof(ack_short_ie_data_t);
            }
            break;

        default:
            assert(false);
            break;
    }

    if (*p_addr_array_len == 0)
    {
        return false;
    }

    memmove(p_addr_array + entry_size * location,
            p_addr_array + entry_size * (location + 1),
            (*p_addr_array_len - location - 1) * entry_size);

    (*p_addr_array_len)--;

    return true;
}

static void ie_data_add(uint32_t location, bool extended, const uint8_t * p_data, uint8_t data_len)
{
    if (extended)
    {
        memcpy(m_ie.ext_data[location].ie_data.p_data, p_data, data_len);
        m_ie.ext_data[location].ie_data.len = data_len;
    }
    else
    {
        memcpy(m_ie.short_data[location].ie_data.p_data, p_data, data_len);
        m_ie.short_data[location].ie_data.len = data_len;
    }
}

/***************************************************************************************************
 * @section Public API
 **************************************************************************************************/

void nrf_802154_ack_data_init(void)
{
    memset(&m_pending_bit, 0, sizeof(m_pending_bit));
    memset(&m_ie, 0, sizeof(m_ie));

    m_pending_bit.enabled = true;
}

void nrf_802154_ack_data_enable(bool enabled)
{
    m_pending_bit.enabled = enabled;
}

bool nrf_802154_ack_data_for_addr_set(const uint8_t * p_addr,
                                      bool            extended,
                                      uint8_t         data_type,
                                      const void    * p_data,
                                      uint8_t         data_len)
{
    uint32_t location = 0;

    if (addr_index_find(p_addr, &location, data_type, extended) ||
        addr_add(p_addr, location, data_type, extended))
    {
        if (data_type == NRF_802154_ACK_DATA_IE)
        {
            ie_data_add(location, extended, p_data, data_len);
        }

        return true;
    }
    else
    {
        return false;
    }
}

bool nrf_802154_ack_data_for_addr_clear(const uint8_t * p_addr, bool extended, uint8_t data_type)
{
    uint32_t location = 0;

    if (addr_index_find(p_addr, &location, data_type, extended))
    {
        return addr_remove(location, data_type, extended);
    }
    else
    {
        return false;
    }
}

void nrf_802154_ack_data_reset(bool extended, uint8_t data_type)
{
    switch (data_type)
    {
        case NRF_802154_ACK_DATA_PENDING_BIT:
            if (extended)
            {
                m_pending_bit.num_of_ext_addr = 0;
            }
            else
            {
                m_pending_bit.num_of_short_addr = 0;
            }
            break;

        case NRF_802154_ACK_DATA_IE:
            if (extended)
            {
                m_ie.num_of_ext_data = 0;
            }
            else
            {
                m_ie.num_of_short_data = 0;
            }
            break;

        default:
            break;
    }
}

bool nrf_802154_ack_data_pending_bit_should_be_set(const uint8_t * p_frame)
{
    bool            extended;
    uint32_t        location;
    const uint8_t * p_src_addr = nrf_802154_frame_parser_src_addr_get(p_frame, &extended);

    // The pending bit is set by default.
    if (!m_pending_bit.enabled || NULL == p_src_addr)
    {
        return true;
    }

    return addr_index_find(p_src_addr, &location, NRF_802154_ACK_DATA_PENDING_BIT, extended);
}

const uint8_t * nrf_802154_ack_data_ie_get(const uint8_t * p_src_addr,
                                           bool            src_addr_extended,
                                           uint8_t       * p_ie_length)
{
    uint32_t location;

    if (NULL == p_src_addr)
    {
        return NULL;
    }

    if (addr_index_find(p_src_addr, &location, NRF_802154_ACK_DATA_IE, src_addr_extended))
    {
        if (src_addr_extended)
        {
            *p_ie_length = m_ie.ext_data[location].ie_data.len;
            return m_ie.ext_data[location].ie_data.p_data;
        }
        else
        {
            *p_ie_length = m_ie.short_data[location].ie_data.len;
            return m_ie.short_data[location].ie_data.p_data;
        }
    }
    else
    {
        *p_ie_length = 0;
        return NULL;
    }
}
