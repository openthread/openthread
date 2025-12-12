/*
 *    Copyright (c) 2025, The OpenThread Authors.
 *    All rights reserved.
 *
 *    Redistribution and use in source and binary forms, with or without
 *    modification, are permitted provided that the following conditions are met:
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *    3. Neither the name of the copyright holder nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 *    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 *    ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 *    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
 *    DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 *    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 *    ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 *    SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file
 *   This file implements a simple in-RAM PSA ITS backend for demonstration and testing.
 *
 *   All data is stored in memory and is not persisted after process termination.
 */

#include "openthread-core-config.h"

#if OPENTHREAD_CONFIG_CRYPTO_LIB == OPENTHREAD_CONFIG_CRYPTO_LIB_PSA

#include "psa/error.h"
#include "psa/internal_trusted_storage.h"

#include <string.h>

#define VerifyOrExit(aCondition, aAction) \
    do                                    \
    {                                     \
        if (!(aCondition))                \
        {                                 \
            aAction;                      \
            goto exit;                    \
        }                                 \
    } while (0)

/**
 * @def RAM_ITS_MAX_KEYS
 *
 * The maximum number of PSA ITS entries that can be stored in RAM.
 */
#define RAM_ITS_MAX_KEYS       64

/**
 * @def RAM_ITS_MAX_DATA_SIZE
 *
 * The maximum size (in bytes) of data that can be stored for each entry.
 */
#define RAM_ITS_MAX_DATA_SIZE  128


/**
 * Represents a single PSA ITS record stored in RAM.
 *
 */
typedef struct
{
    bool                       mInUse;                       // Whether this slot is occupied
    psa_storage_uid_t          mUid;                         // Unique ID
    psa_storage_create_flags_t mFlags;                       // Storage flags (e.g. WRITE_ONCE)
    uint32_t                   mDataLen;                     // Current size of stored data
    uint8_t                    mData[RAM_ITS_MAX_DATA_SIZE]; // Raw data storage
} RamItsEntry;

/**
 * Array of entries for RAM-based ITS storage.
 *
 */
static RamItsEntry sRamItsEntries[RAM_ITS_MAX_KEYS];

/**
 * Finds an existing entry by UID.
 *
 * @param[in] aUid  The UID value to search for.
 *
 * @returns The index of the matching entry, or -1 if not found.
 *
 */
static int RamItsFindEntry(psa_storage_uid_t aUid)
{
    for (int i = 0; i < RAM_ITS_MAX_KEYS; i++)
    {
        if (sRamItsEntries[i].mInUse && (sRamItsEntries[i].mUid == aUid))
        {
            return i;
        }
    }

    return -1;
}

/**
 * Finds a free slot in the storage array.
 *
 * @returns The index of a free slot, or -1 if none is available.
 *
 */
static int RamItsFindFreeSlot(void)
{
    for (int i = 0; i < RAM_ITS_MAX_KEYS; i++)
    {
        if (!sRamItsEntries[i].mInUse)
        {
            return i;
        }
    }

    return -1;
}

psa_status_t psa_its_set(psa_storage_uid_t           aUid,
                         uint32_t                    aDataLength,
                         const void                 *aData,
                         psa_storage_create_flags_t  aCreateFlags)
{
    psa_status_t status = PSA_SUCCESS;
    int          idx    = -1;

    VerifyOrExit(!(aData == NULL && aDataLength > 0), status = PSA_ERROR_INVALID_ARGUMENT);

    // Allow only NONE or WRITE_ONCE flags. Any others => Not supported.
    VerifyOrExit((aCreateFlags & ~(PSA_STORAGE_FLAG_WRITE_ONCE)) == 0,
                 status = PSA_ERROR_NOT_SUPPORTED);

    // Data length must not exceed our internal buffer size.
    VerifyOrExit(aDataLength <= RAM_ITS_MAX_DATA_SIZE, status = PSA_ERROR_INVALID_ARGUMENT);

    // Find existing entry, if any.
    idx = RamItsFindEntry(aUid);
    if (idx >= 0)
    {
        // Entry already exists.

        // If WRITE_ONCE is set, we cannot overwrite it.
        VerifyOrExit(!(sRamItsEntries[idx].mFlags & PSA_STORAGE_FLAG_WRITE_ONCE), status = PSA_ERROR_NOT_PERMITTED);

        // Overwrite existing entry.
        sRamItsEntries[idx].mDataLen = aDataLength;

        if (aDataLength > 0 && aData != NULL)
        {
            memcpy(sRamItsEntries[idx].mData, aData, aDataLength);
        }

        sRamItsEntries[idx].mFlags = aCreateFlags;
    }
    else
    {
        // Need a new entry.
        idx = RamItsFindFreeSlot();
        VerifyOrExit(idx >= 0, status = PSA_ERROR_INSUFFICIENT_STORAGE);

        sRamItsEntries[idx].mInUse   = true;
        sRamItsEntries[idx].mUid     = aUid;
        sRamItsEntries[idx].mFlags   = aCreateFlags;
        sRamItsEntries[idx].mDataLen = aDataLength;

        if (aDataLength > 0 && aData != NULL)
        {
            memcpy(sRamItsEntries[idx].mData, aData, aDataLength);
        }
    }

exit:
    return status;
}

psa_status_t psa_its_get(psa_storage_uid_t  aUid,
                         uint32_t           aDataOffset,
                         uint32_t           aDataLength,
                         void              *aData,
                         size_t            *aDataLengthOut)
{
    psa_status_t status = PSA_SUCCESS;
    int          idx;

    // Validate pointers.
    VerifyOrExit(aDataLengthOut != NULL, status = PSA_ERROR_INVALID_ARGUMENT);
    if (aDataLength > 0)
    {
        VerifyOrExit(aData != NULL, status = PSA_ERROR_INVALID_ARGUMENT);
    }

    idx = RamItsFindEntry(aUid);
    VerifyOrExit(idx >= 0, status = PSA_ERROR_DOES_NOT_EXIST);

    // Check offset validity.
    VerifyOrExit(aDataOffset <= sRamItsEntries[idx].mDataLen, status = PSA_ERROR_INVALID_ARGUMENT);

    // Determine how many bytes to copy.
    {
        size_t available = sRamItsEntries[idx].mDataLen - aDataOffset;
        size_t toCopy    = (aDataLength <= available) ? aDataLength : available;

        if (toCopy > 0)
        {
            memcpy(aData, &sRamItsEntries[idx].mData[aDataOffset], toCopy);
        }

        *aDataLengthOut = toCopy;
    }

exit:
    return status;
}

psa_status_t psa_its_get_info(psa_storage_uid_t          aUid,
                              struct psa_storage_info_t *aInfo)
{
    psa_status_t status = PSA_SUCCESS;
    int          idx;

    VerifyOrExit(aInfo != NULL, status = PSA_ERROR_INVALID_ARGUMENT);

    idx = RamItsFindEntry(aUid);
    VerifyOrExit(idx >= 0, status = PSA_ERROR_DOES_NOT_EXIST);

    aInfo->size  = sRamItsEntries[idx].mDataLen;
    aInfo->flags = sRamItsEntries[idx].mFlags;

exit:
    return status;
}

psa_status_t psa_its_remove(psa_storage_uid_t aUid)
{
    psa_status_t status = PSA_SUCCESS;
    int          idx;

    idx = RamItsFindEntry(aUid);
    VerifyOrExit(idx >= 0, status = PSA_ERROR_DOES_NOT_EXIST);

    // If WRITE_ONCE is set, we cannot remove it.
    VerifyOrExit(!(sRamItsEntries[idx].mFlags & PSA_STORAGE_FLAG_WRITE_ONCE),
                 status = PSA_ERROR_NOT_PERMITTED);

    // Clear the slot.
    memset(sRamItsEntries[idx].mData, 0, sizeof(sRamItsEntries[idx].mData));
    sRamItsEntries[idx].mDataLen = 0;
    sRamItsEntries[idx].mUid     = 0;
    sRamItsEntries[idx].mFlags   = 0;
    sRamItsEntries[idx].mInUse   = false;

exit:
    return status;
}

#endif // #if OPENTHREAD_CONFIG_CRYPTO_LIB == OPENTHREAD_CONFIG_CRYPTO_LIB_PSA
