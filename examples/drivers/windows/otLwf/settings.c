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
 * @file
 * @brief
 *  This file implements the settings functions required for the OpenThread library.
 */

#include "precomp.h"
#include "settings.tmh"

void otPlatSettingsInit(otInstance *otCtx)
{
    NT_ASSERT(otCtx);
    PMS_FILTER pFilter = otCtxToFilter(otCtx);

    DECLARE_CONST_UNICODE_STRING(SubKeyName, L"OpenThread");

    OBJECT_ATTRIBUTES attributes;
    ULONG disposition;

    LogFuncEntry(DRIVER_DEFAULT);

    InitializeObjectAttributes(
        &attributes,
        (PUNICODE_STRING)&SubKeyName,
        OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
        pFilter->InterfaceRegKey,
        NULL);

    // Create/Open the 'OpenThread' sub key
    NTSTATUS status =
        ZwCreateKey(
            &pFilter->otSettingsRegKey,
            KEY_ALL_ACCESS,
            &attributes,
            0,
            NULL,
            REG_OPTION_NON_VOLATILE,
            &disposition);

    NT_ASSERT(NT_SUCCESS(status));
    if (!NT_SUCCESS(status))
    {
        LogError(DRIVER_DEFAULT, "ZwCreateKey for 'OpenThread' key failed, %!STATUS!", status);
    }

    LogFuncExit(DRIVER_DEFAULT);
}

uint16_t FilterCountSettings(_In_ PMS_FILTER pFilter, uint16_t aKey)
{
    HANDLE regKey = NULL;
    OBJECT_ATTRIBUTES attributes;
    DECLARE_UNICODE_STRING_SIZE(Name, 8);
    UCHAR InfoBuffer[128] = {0};
    PKEY_FULL_INFORMATION pInfo = (PKEY_FULL_INFORMATION)InfoBuffer;
    ULONG InfoLength = sizeof(InfoBuffer);

    // Convert 'aKey' to a string
    RtlIntegerToUnicodeString((ULONG)aKey, 16, &Name);

    InitializeObjectAttributes(
        &attributes,
        &Name,
        OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
        pFilter->otSettingsRegKey,
        NULL);

    // Open the registry key
    NTSTATUS status =
        ZwOpenKey(
            &regKey,
            KEY_ALL_ACCESS,
            &attributes);

    if (!NT_SUCCESS(status))
    {
        // Key doesn't exist, return a count of 0
        goto error;
    }

    // Query the key info from the registry
    status =
        ZwQueryKey(
            regKey,
            KeyValueFullInformation,
            pInfo,
            InfoLength,
            &InfoLength);

    if (!NT_SUCCESS(status))
    {
        LogError(DRIVER_DEFAULT, "ZwQueryKey for %S value failed, %!STATUS!", Name.Buffer, status);
        goto error;
    }

error:

    if (regKey) ZwClose(regKey);

    return (uint16_t)pInfo->Values;
}

NTSTATUS FilterReadSetting(_In_ PMS_FILTER pFilter, uint16_t aKey, int aIndex, uint8_t *aValue, uint16_t *aValueLength)
{
    HANDLE regKey = NULL;
    OBJECT_ATTRIBUTES attributes;
    DECLARE_UNICODE_STRING_SIZE(Name, 20);
    PKEY_VALUE_PARTIAL_INFORMATION pInfo = NULL;
    ULONG InfoLength = sizeof(*pInfo) + *aValueLength;

    // Convert 'aKey' to a string
    RtlIntegerToUnicodeString((ULONG)aKey, 16, &Name);

    InitializeObjectAttributes(
        &attributes,
        &Name,
        OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
        pFilter->otSettingsRegKey,
        NULL);

    // Open the registry key
    NTSTATUS status =
        ZwOpenKey(
            &regKey,
            KEY_ALL_ACCESS,
            &attributes);

    if (!NT_SUCCESS(status))
    {
        // Key doesn't exist
        goto error;
    }

    // Convert 'aIndex' to a string
    RtlIntegerToUnicodeString((ULONG)aIndex, 16, &Name);

    // Allocate buffer for query
    pInfo = FILTER_ALLOC_MEM(pFilter->FilterHandle, InfoLength);
    if (pInfo == NULL)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto error;
    }

    // Query the data
    status = ZwQueryValueKey(
        regKey,
        &Name,
        KeyValuePartialInformation,
        pInfo,
        InfoLength,
        &InfoLength);

    if (!NT_SUCCESS(status))
    {
        LogVerbose(DRIVER_DEFAULT, "ZwQueryValueKey for %S value failed, %!STATUS!", Name.Buffer, status);
        goto error;
    }

    NT_ASSERT(*aValueLength >= pInfo->DataLength);
    *aValueLength = (uint16_t)pInfo->DataLength;
    if (aValue)
    {
        memcpy(aValue, pInfo->Data, pInfo->DataLength);
    }

error:

    if (pInfo) FILTER_FREE_MEM(pInfo);
    if (regKey) ZwClose(regKey);

    return status;
}

NTSTATUS FilterWriteSetting(_In_ PMS_FILTER pFilter, uint16_t aKey, int aIndex, const uint8_t *aValue, uint16_t aValueLength)
{
    HANDLE regKey = NULL;
    OBJECT_ATTRIBUTES attributes;
    DECLARE_UNICODE_STRING_SIZE(Name, 20);

    // Convert 'aKey' to a string
    RtlIntegerToUnicodeString((ULONG)aKey, 16, &Name);

    InitializeObjectAttributes(
        &attributes,
        &Name,
        OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
        pFilter->otSettingsRegKey,
        NULL);

    // Create/Open the registry key
    NTSTATUS status =
        ZwCreateKey(
            &regKey,
            KEY_ALL_ACCESS,
            &attributes,
            0,
            NULL,
            REG_OPTION_NON_VOLATILE,
            NULL);

    NT_ASSERT(NT_SUCCESS(status));
    if (!NT_SUCCESS(status))
    {
        LogError(DRIVER_DEFAULT, "ZwCreateKey for %S key failed, %!STATUS!", Name.Buffer, status);
        goto error;
    }

    // Convert 'aIndex' to a string
    RtlIntegerToUnicodeString((ULONG)aIndex, 16, &Name);

    // Write the data to the registry
    status =
        ZwSetValueKey(
            regKey,
            &Name,
            0,
            REG_BINARY,
            (PVOID)aValue,
            aValueLength);

    if (!NT_SUCCESS(status))
    {
        LogError(DRIVER_DEFAULT, "ZwSetValueKey for %S value failed, %!STATUS!", Name.Buffer, status);
        goto error;
    }

error:

    if (regKey) ZwClose(regKey);

    return status;
}

NTSTATUS FilterDeleteSetting(_In_ PMS_FILTER pFilter, uint16_t aKey, int aIndex)
{
    HANDLE regKey = NULL;
    OBJECT_ATTRIBUTES attributes;
    DECLARE_UNICODE_STRING_SIZE(Name, 20);

    // Convert 'aKey' to a string
    RtlIntegerToUnicodeString((ULONG)aKey, 16, &Name);

    InitializeObjectAttributes(
        &attributes,
        &Name,
        OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
        pFilter->otSettingsRegKey,
        NULL);

    // Open the registry key
    NTSTATUS status =
        ZwOpenKey(
            &regKey,
            KEY_ALL_ACCESS,
            &attributes);

    if (!NT_SUCCESS(status))
    {
        // Key doesn't exist
        goto error;
    }

    // If 'aIndex' is -1 then delete the whole key, otherwise delete the individual value
    if (aIndex == -1)
    {
        // Delete the registry key
        status = ZwDeleteKey(regKey);
    }
    else
    {
        UCHAR KeyInfoBuffer[128] = { 0 };
        PKEY_FULL_INFORMATION pKeyInfo = (PKEY_FULL_INFORMATION)KeyInfoBuffer;
        ULONG KeyInfoLength = sizeof(KeyInfoBuffer);

        // When deleting an individual value, since order doesn't matter, we will actually
        // copy the last value over the one being deleted and then delete the last value; so
        // we maintain a contiguous list of numbered values

        // Query the number of values
        // Note: Can't use helper function because we already have the key open
        status =
            ZwQueryKey(
                regKey,
                KeyValueFullInformation,
                pKeyInfo,
                KeyInfoLength,
                &KeyInfoLength);

        if (!NT_SUCCESS(status))
        {
            LogError(DRIVER_DEFAULT, "ZwQueryKey for %S value failed, %!STATUS!", Name.Buffer, status);
            goto error;
        }

        if ((ULONG)aIndex >= pKeyInfo->Values)
        {
            // Attempt to delete beyond the end of the list
            status = STATUS_OBJECT_NAME_NOT_FOUND;
            goto error;
        }
        else if (pKeyInfo->Values == 1)
        {
            // Deleting the only value on the key, go ahead and delete the entire key
            status = ZwDeleteKey(regKey);
        }
        else if (pKeyInfo->Values - 1 != (ULONG)aIndex)
        {
            // We aren't deleting the last value so we need to copy the last value
            // over this one, and then delete the last one.

            PKEY_VALUE_PARTIAL_INFORMATION pValueInfo = NULL;
            ULONG ValueInfoLength = 0;

            // Convert pKeyInfo->Values-1 to a string
            RtlIntegerToUnicodeString(pKeyInfo->Values - 1, 16, &Name);

            // Query the key data buffer size
            status = ZwQueryValueKey(
                regKey,
                &Name,
                KeyValuePartialInformation,
                pValueInfo,
                0,
                &ValueInfoLength);

            NT_ASSERT(status != STATUS_SUCCESS);
            if (status != STATUS_BUFFER_TOO_SMALL)
            {
                LogVerbose(DRIVER_DEFAULT, "ZwQueryValueKey for %S value failed, %!STATUS!", Name.Buffer, status);
                goto error;
            }

            pValueInfo = FILTER_ALLOC_MEM(pFilter->FilterHandle, ValueInfoLength);
            if (pValueInfo == NULL)
            {
                status = STATUS_INSUFFICIENT_RESOURCES;
                goto error;
            }

            // Query the data buffer
            status = ZwQueryValueKey(
                regKey,
                &Name,
                KeyValuePartialInformation,
                pValueInfo,
                ValueInfoLength,
                &ValueInfoLength);

            if (!NT_SUCCESS(status))
            {
                LogError(DRIVER_DEFAULT, "ZwQueryValueKey for %S value failed, %!STATUS!", Name.Buffer, status);
                goto cleanup;
            }

            // Delete the registry value
            status =
                ZwDeleteValueKey(
                    regKey,
                    &Name);

            if (!NT_SUCCESS(status))
            {
                LogError(DRIVER_DEFAULT, "ZwDeleteValueKey for %S value failed, %!STATUS!", Name.Buffer, status);
                goto cleanup;
            }

            // Convert 'aIndex' to a string
            RtlIntegerToUnicodeString((ULONG)aIndex, 16, &Name);

            // Write the data to the registry key we are deleting
            status =
                ZwSetValueKey(
                    regKey,
                    &Name,
                    0,
                    REG_BINARY,
                    (PVOID)pValueInfo->Data,
                    pValueInfo->DataLength);

            if (!NT_SUCCESS(status))
            {
                LogError(DRIVER_DEFAULT, "ZwSetValueKey for %S value failed, %!STATUS!", Name.Buffer, status);
                goto cleanup;
            }

        cleanup:

            if (pValueInfo) FILTER_FREE_MEM(pValueInfo);
        }
        else
        {
            // Deleting the last value in the list (but not the only value)
            // Just delete the value directly. No need to copy any others.

            // Convert 'aIndex' to a string
            RtlIntegerToUnicodeString((ULONG)aIndex, 16, &Name);

            // Delete the registry value
            status =
                ZwDeleteValueKey(
                    regKey,
                    &Name);
        }
    }

error:

    if (regKey) ZwClose(regKey);

    return status;
}

otError otPlatSettingsBeginChange(otInstance *otCtx)
{
    UNREFERENCED_PARAMETER(otCtx);
    return OT_ERROR_NOT_IMPLEMENTED;
}

otError otPlatSettingsCommitChange(otInstance *otCtx)
{
    UNREFERENCED_PARAMETER(otCtx);
    return OT_ERROR_NOT_IMPLEMENTED;
}

otError otPlatSettingsAbandonChange(otInstance *otCtx)
{
    UNREFERENCED_PARAMETER(otCtx);
    return OT_ERROR_NOT_IMPLEMENTED;
}

otError otPlatSettingsGet(otInstance *otCtx, uint16_t aKey, int aIndex, uint8_t *aValue, uint16_t *aValueLength)
{
    NT_ASSERT(otCtx);
    PMS_FILTER pFilter = otCtxToFilter(otCtx);

    NTSTATUS status = 
        FilterReadSetting(
            pFilter,
            aKey,
            aIndex,
            aValue,
            aValueLength);

    return NT_SUCCESS(status) ? OT_ERROR_NONE : OT_ERROR_NOT_FOUND;
}

otError otPlatSettingsSet(otInstance *otCtx, uint16_t aKey, const uint8_t *aValue, uint16_t aValueLength)
{
    NT_ASSERT(otCtx);
    PMS_FILTER pFilter = otCtxToFilter(otCtx);

    NTSTATUS status = 
        FilterWriteSetting(
            pFilter,
            aKey,
            0,
            aValue,
            aValueLength);

    return NT_SUCCESS(status) ? OT_ERROR_NONE : OT_ERROR_FAILED;
}

otError otPlatSettingsAdd(otInstance *otCtx, uint16_t aKey, const uint8_t *aValue, uint16_t aValueLength)
{
    NT_ASSERT(otCtx);
    PMS_FILTER pFilter = otCtxToFilter(otCtx);

    uint16_t count = FilterCountSettings(pFilter, aKey);

    NTSTATUS status =
        FilterWriteSetting(
            pFilter,
            aKey,
            count,
            aValue,
            aValueLength);

    return NT_SUCCESS(status) ? OT_ERROR_NONE : OT_ERROR_FAILED;
}

otError otPlatSettingsDelete(otInstance *otCtx, uint16_t aKey, int aIndex)
{
    NT_ASSERT(otCtx);
    PMS_FILTER pFilter = otCtxToFilter(otCtx);

    NTSTATUS status =
        FilterDeleteSetting(
            pFilter,
            aKey,
            aIndex);

    return NT_SUCCESS(status) ? OT_ERROR_NONE : OT_ERROR_FAILED;
}

void otPlatSettingsWipe(otInstance *otCtx)
{
    NT_ASSERT(otCtx);
    PMS_FILTER pFilter = otCtxToFilter(otCtx);

    LogFuncEntry(DRIVER_DEFAULT);

    // Delete all subkeys of 'OpenThread'
    if (pFilter->otSettingsRegKey)
    {
        NTSTATUS status = STATUS_SUCCESS;
        ULONG index = 0;
        UCHAR keyInfo[sizeof(KEY_BASIC_INFORMATION) + 64];

        while (status == STATUS_SUCCESS)
        {
            ULONG size = sizeof(keyInfo);
            status =
                ZwEnumerateKey(
                    pFilter->otSettingsRegKey,
                    index,
                    KeyBasicInformation,
                    keyInfo,
                    size,
                    &size);

            bool deleted = false;
            if (NT_SUCCESS(status))
            {
                HANDLE subKey = NULL;
                OBJECT_ATTRIBUTES attributes;
                PKEY_BASIC_INFORMATION pKeyInfo = (PKEY_BASIC_INFORMATION)keyInfo;

                UNICODE_STRING subKeyName = 
                {
                    (USHORT)pKeyInfo->NameLength,
                    (USHORT)pKeyInfo->NameLength,
                    pKeyInfo->Name
                };

                InitializeObjectAttributes(
                    &attributes,
                    &subKeyName,
                    OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                    pFilter->otSettingsRegKey,
                    NULL);

                // Open the sub key
                status =
                    ZwOpenKey(
                        &subKey,
                        KEY_ALL_ACCESS,
                        &attributes);

                if (NT_SUCCESS(status))
                {
                    // Delete the key
                    status = ZwDeleteKey(subKey);
                    if (!NT_SUCCESS(status))
                    {
                        LogError(DRIVER_DEFAULT, "ZwDeleteKey for subkey failed, %!STATUS!", status);
                    }
                    else
                    {
                        deleted = true;
                    }

                    // Close handle
                    ZwClose(subKey);
                }
                else
                {
                    LogError(DRIVER_DEFAULT, "ZwOpenKey for subkey failed, %!STATUS!", status);
                }
            }

            // Only increment index if we didn't delete
            if (!deleted)
            {
                index++;
            }
        }
    }

    LogFuncExit(DRIVER_DEFAULT);
}
