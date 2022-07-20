/*
 *  Copyright (c) 2019, The OpenThread Authors.
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
 *   This file implements a software Source Match table, for radios that don't have
 *   such hardware acceleration. It supports only the single-instance build of
 *   OpenThread.
 *
 */

#include "utils/soft_source_match_table.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <openthread/logging.h>

#include "utils/code_utils.h"

// Print entire source match tables when
#define PRINT_MULTIPAN_SOURCE_MATCH_TABLES 0

#if OPENTHREAD_RADIO && OPENTHREAD_CONFIG_MULTIPAN_RCP_ENABLE == 1
extern uint8_t        otNcpPlatGetCurCommandIid(void);
static inline uint8_t getPanIndex(uint8_t iid)
{
    // Assert if iid=0 (broadcast iid)
    assert(iid != 0);
    return iid - 1;
}
#else
#define otNcpPlatGetCurCommandIid() 0
#define getPanIndex(iid) 0
#endif

#if RADIO_CONFIG_SRC_MATCH_SHORT_ENTRY_NUM || RADIO_CONFIG_SRC_MATCH_EXT_ENTRY_NUM
static uint16_t sPanId[RADIO_CONFIG_SRC_MATCH_PANID_NUM] = {0};

#if PRINT_MULTIPAN_SOURCE_MATCH_TABLES
static void printPanIdTable(void)
{
    for (uint8_t panIndex = 0; panIndex < RADIO_CONFIG_SRC_MATCH_PANID_NUM; panIndex++)
    {
        otLogDebgPlat("sPanId[panIndex=%d] = 0x%04x", panIndex, sPanId[panIndex]);
    }
}
#else
#define printPanIdTable()
#endif

void utilsSoftSrcMatchSetPanId(uint8_t iid, uint16_t aPanId)
{
    OT_UNUSED_VARIABLE(iid);

    const uint8_t panIndex = getPanIndex(iid);
    sPanId[panIndex]       = aPanId;
    otLogInfoPlat("Setting panIndex=%d to 0x%04x", panIndex, aPanId);

    printPanIdTable();
}
#endif // RADIO_CONFIG_SRC_MATCH_SHORT_ENTRY_NUM || RADIO_CONFIG_SRC_MATCH_EXT_ENTRY_NUM

#if RADIO_CONFIG_SRC_MATCH_SHORT_ENTRY_NUM
typedef struct srcMatchShortEntry
{
    uint16_t checksum;
    bool     allocated;
} sSrcMatchShortEntry;

static sSrcMatchShortEntry srcMatchShortEntry[RADIO_CONFIG_SRC_MATCH_PANID_NUM][RADIO_CONFIG_SRC_MATCH_SHORT_ENTRY_NUM];

#if PRINT_MULTIPAN_SOURCE_MATCH_TABLES
static void printShortEntryTable(uint8_t iid)
{
    const uint8_t panIndex = getPanIndex(iid);
    OT_UNUSED_VARIABLE(iid);
    OT_UNUSED_VARIABLE(panIndex);

    otLogDebgPlat("================================|============|===========");
    otLogDebgPlat("ShortEntry[panIndex][entry]     | .allocated | .checksum ");
    otLogDebgPlat("================================|============|===========");
    for (int16_t i = 0; i < RADIO_CONFIG_SRC_MATCH_SHORT_ENTRY_NUM; i++)
    {
        otLogDebgPlat("ShortEntry[panIndex=%d][entry=%d] | %d          | 0x%04x", panIndex, i,
                      srcMatchShortEntry[panIndex][i].allocated, srcMatchShortEntry[panIndex][i].checksum);
    }
    otLogDebgPlat("================================|============|===========");
}
#else
#define printShortEntryTable(iid)
#endif

int16_t utilsSoftSrcMatchShortFindEntry(uint8_t iid, uint16_t aShortAddress)
{
    OT_UNUSED_VARIABLE(iid);

    int16_t entry    = -1;

#if OPENTHREAD_RADIO && OPENTHREAD_CONFIG_MULTIPAN_RCP_ENABLE == 1
    if (iid == 0)
    {
        return entry;
    }
#endif

    const uint8_t panIndex = getPanIndex(iid);
    uint16_t      checksum = aShortAddress + sPanId[panIndex];

    for (int16_t i = 0; i < RADIO_CONFIG_SRC_MATCH_SHORT_ENTRY_NUM; i++)
    {
        if (checksum == srcMatchShortEntry[panIndex][i].checksum && srcMatchShortEntry[panIndex][i].allocated)
        {
            entry = i;
            break;
        }
    }

    return entry;
}

static int16_t findSrcMatchShortAvailEntry(uint8_t iid)
{
    OT_UNUSED_VARIABLE(iid);

    int16_t       entry    = -1;
    const uint8_t panIndex = getPanIndex(iid);

    for (int16_t i = 0; i < RADIO_CONFIG_SRC_MATCH_SHORT_ENTRY_NUM; i++)
    {
        if (!srcMatchShortEntry[panIndex][i].allocated)
        {
            entry = i;
            break;
        }
    }

    return entry;
}

static inline void addToSrcMatchShortIndirect(uint8_t iid, uint16_t entry, uint16_t aShortAddress)
{
    OT_UNUSED_VARIABLE(iid);

    const uint8_t panIndex = getPanIndex(iid);
    uint16_t      checksum = aShortAddress + sPanId[panIndex];

    srcMatchShortEntry[panIndex][entry].checksum  = checksum;
    srcMatchShortEntry[panIndex][entry].allocated = true;

    printShortEntryTable(iid);
}

static inline void removeFromSrcMatchShortIndirect(uint8_t iid, uint16_t entry)
{
    OT_UNUSED_VARIABLE(iid);

    const uint8_t panIndex = getPanIndex(iid);

    srcMatchShortEntry[panIndex][entry].allocated = false;
    srcMatchShortEntry[panIndex][entry].checksum  = 0;

    printShortEntryTable(iid);
}

otError otPlatRadioAddSrcMatchShortEntry(otInstance *aInstance, uint16_t aShortAddress)
{
    OT_UNUSED_VARIABLE(aInstance);

    otError error = OT_ERROR_NONE;
    int8_t  iid   = -1;
    int16_t entry = -1;

    iid   = otNcpPlatGetCurCommandIid();
    entry = findSrcMatchShortAvailEntry(iid);

    otLogDebgPlat("Add ShortAddr: iid=%d, entry=%d, addr=0x%04x", iid, entry, aShortAddress);

    otEXPECT_ACTION(entry >= 0 && entry < RADIO_CONFIG_SRC_MATCH_SHORT_ENTRY_NUM, error = OT_ERROR_NO_BUFS);

    addToSrcMatchShortIndirect(iid, (uint16_t)entry, aShortAddress);

exit:
    return error;
}

otError otPlatRadioClearSrcMatchShortEntry(otInstance *aInstance, uint16_t aShortAddress)
{
    OT_UNUSED_VARIABLE(aInstance);

    otError error = OT_ERROR_NONE;
    int16_t entry = -1;
    int8_t  iid   = -1;

    iid = otNcpPlatGetCurCommandIid();

    entry = utilsSoftSrcMatchShortFindEntry(iid, aShortAddress);
    otLogDebgPlat("Clear ShortAddr: iid=%d, entry=%d, addr=0x%04x", iid, entry, aShortAddress);

    otEXPECT_ACTION(entry >= 0 && entry < RADIO_CONFIG_SRC_MATCH_SHORT_ENTRY_NUM, error = OT_ERROR_NO_ADDRESS);

    removeFromSrcMatchShortIndirect(iid, (uint16_t)entry);

exit:
    return error;
}

void otPlatRadioClearSrcMatchShortEntries(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    uint8_t       iid      = otNcpPlatGetCurCommandIid();
    const uint8_t panIndex = getPanIndex(iid);
    OT_UNUSED_VARIABLE(iid);

    otLogDebgPlat("Clear ShortAddr entries (iid: %d)", iid);

    memset(srcMatchShortEntry[panIndex], 0, sizeof(srcMatchShortEntry[panIndex]));

    printShortEntryTable(iid);
}
#endif // RADIO_CONFIG_SRC_MATCH_SHORT_ENTRY_NUM

#if RADIO_CONFIG_SRC_MATCH_EXT_ENTRY_NUM
typedef struct srcMatchExtEntry
{
    uint16_t checksum;
    bool     allocated;
} sSrcMatchExtEntry;

static sSrcMatchExtEntry srcMatchExtEntry[RADIO_CONFIG_SRC_MATCH_PANID_NUM][RADIO_CONFIG_SRC_MATCH_EXT_ENTRY_NUM];

#if PRINT_MULTIPAN_SOURCE_MATCH_TABLES
static void printExtEntryTable(uint8_t iid)
{
    OT_UNUSED_VARIABLE(iid);
    const uint8_t panIndex = getPanIndex(iid);
    OT_UNUSED_VARIABLE(panIndex);

    otLogDebgPlat("==============================|============|===========");
    otLogDebgPlat("ExtEntry[panIndex][entry]     | .allocated | .checksum ");
    otLogDebgPlat("==============================|============|===========");
    for (int16_t i = 0; i < RADIO_CONFIG_SRC_MATCH_EXT_ENTRY_NUM; i++)
    {
        otLogDebgPlat("ExtEntry[panIndex=%d][entry=%d] | %d          | 0x%04x", panIndex, i,
                      srcMatchExtEntry[panIndex][i].allocated, srcMatchExtEntry[panIndex][i].checksum);
    }
    otLogDebgPlat("==============================|============|===========");
}
#else
#define printExtEntryTable(iid)
#endif

int16_t utilsSoftSrcMatchExtFindEntry(uint8_t iid, const otExtAddress *aExtAddress)
{
    OT_UNUSED_VARIABLE(iid);

    int16_t entry    = -1;

#if OPENTHREAD_RADIO && OPENTHREAD_CONFIG_MULTIPAN_RCP_ENABLE == 1
    if (iid == 0)
    {
        return entry;
    }
#endif
    
    const uint8_t panIndex = getPanIndex(iid);
    uint16_t      checksum = sPanId[panIndex];

    checksum += (uint16_t)aExtAddress->m8[0] | (uint16_t)(aExtAddress->m8[1] << 8);
    checksum += (uint16_t)aExtAddress->m8[2] | (uint16_t)(aExtAddress->m8[3] << 8);
    checksum += (uint16_t)aExtAddress->m8[4] | (uint16_t)(aExtAddress->m8[5] << 8);
    checksum += (uint16_t)aExtAddress->m8[6] | (uint16_t)(aExtAddress->m8[7] << 8);

    for (int16_t i = 0; i < RADIO_CONFIG_SRC_MATCH_EXT_ENTRY_NUM; i++)
    {
        if (checksum == srcMatchExtEntry[panIndex][i].checksum && srcMatchExtEntry[panIndex][i].allocated)
        {
            entry = i;
            break;
        }
    }

    return entry;
}

static int16_t findSrcMatchExtAvailEntry(uint8_t iid)
{
    OT_UNUSED_VARIABLE(iid);

    int16_t       entry    = -1;
    const uint8_t panIndex = getPanIndex(iid);

    for (int16_t i = 0; i < RADIO_CONFIG_SRC_MATCH_EXT_ENTRY_NUM; i++)
    {
        if (!srcMatchExtEntry[panIndex][i].allocated)
        {
            entry = i;
            break;
        }
    }

    return entry;
}

static inline void addToSrcMatchExtIndirect(uint8_t iid, uint16_t entry, const otExtAddress *aExtAddress)
{
    OT_UNUSED_VARIABLE(iid);

    const uint8_t panIndex = getPanIndex(iid);
    uint16_t      checksum = sPanId[panIndex];

    checksum += (uint16_t)aExtAddress->m8[0] | (uint16_t)(aExtAddress->m8[1] << 8);
    checksum += (uint16_t)aExtAddress->m8[2] | (uint16_t)(aExtAddress->m8[3] << 8);
    checksum += (uint16_t)aExtAddress->m8[4] | (uint16_t)(aExtAddress->m8[5] << 8);
    checksum += (uint16_t)aExtAddress->m8[6] | (uint16_t)(aExtAddress->m8[7] << 8);

    srcMatchExtEntry[panIndex][entry].checksum  = checksum;
    srcMatchExtEntry[panIndex][entry].allocated = true;

    printExtEntryTable(iid);
}

static inline void removeFromSrcMatchExtIndirect(uint8_t iid, uint16_t entry)
{
    OT_UNUSED_VARIABLE(iid);

    const uint8_t panIndex = getPanIndex(iid);

    srcMatchExtEntry[panIndex][entry].allocated = false;
    srcMatchExtEntry[panIndex][entry].checksum  = 0;

    printExtEntryTable(iid);
}

otError otPlatRadioAddSrcMatchExtEntry(otInstance *aInstance, const otExtAddress *aExtAddress)
{
    OT_UNUSED_VARIABLE(aInstance);

    otError error = OT_ERROR_NONE;
    int16_t entry = -1;

    uint8_t iid = otNcpPlatGetCurCommandIid();
    entry       = findSrcMatchExtAvailEntry(iid);

    otLogDebgPlat("Add ExtAddr: iid=%d, entry=%d, addr 0x%016x", iid, entry, aExtAddress->m8);

    otEXPECT_ACTION(entry >= 0 && entry < RADIO_CONFIG_SRC_MATCH_EXT_ENTRY_NUM, error = OT_ERROR_NO_BUFS);

    addToSrcMatchExtIndirect(iid, (uint16_t)entry, aExtAddress);

exit:
    return error;
}

otError otPlatRadioClearSrcMatchExtEntry(otInstance *aInstance, const otExtAddress *aExtAddress)
{
    OT_UNUSED_VARIABLE(aInstance);

    otError error = OT_ERROR_NONE;
    int16_t entry = -1;

    uint8_t iid = otNcpPlatGetCurCommandIid();
    entry       = utilsSoftSrcMatchExtFindEntry(iid, aExtAddress);
    otLogDebgPlat("Clear ExtAddr: iid=%d, entry=%d", iid, entry);

    otEXPECT_ACTION(entry >= 0 && entry < RADIO_CONFIG_SRC_MATCH_EXT_ENTRY_NUM, error = OT_ERROR_NO_ADDRESS);

    removeFromSrcMatchExtIndirect(iid, (uint16_t)entry);

exit:
    return error;
}

void otPlatRadioClearSrcMatchExtEntries(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    uint8_t iid = otNcpPlatGetCurCommandIid();
    OT_UNUSED_VARIABLE(iid);

    otLogDebgPlat("Clear ExtAddr entries (iid: %d)", iid);
    const uint8_t panIndex = getPanIndex(iid);

    memset(srcMatchExtEntry[panIndex], 0, sizeof(srcMatchExtEntry[panIndex]));

    printExtEntryTable(iid);
}
#endif // RADIO_CONFIG_SRC_MATCH_EXT_ENTRY_NUM
