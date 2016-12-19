/*
 *  Copyright (c) 2018, The OpenThread Authors.
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
 *    @file
 *      This file implements OpenThread BLE platform interface
 *      that maps into nlplatform port of NimBLE API.
 *
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "platform-config.h"

#include <common/logging.hpp>
#include <openthread/platform/ble.h>
#include <openthread/platform/ble_hci.h>

#include "ble_event.h"

#include <nimble/nimble_npl.h>
#include <nimble/nimble_port.h>

#include "ble_l2cap_priv.h"
#include <host/ble_hs.h>
#include <nimble/ble.h>
#include <nimble/ble_hci_trans.h>
#include <services/ans/ble_svc_ans.h>
#include <services/gap/ble_svc_gap.h>
#include <services/gatt/ble_svc_gatt.h>

int ble_hs_startup_go(void);

#define TASK_DEFAULT_PRIORITY 1
#define TASK_DEFAULT_STACK NULL
#define TASK_DEFAULT_STACK_SIZE 400

/// Max amount of time in [ms] to scan on connection request.
#define DEFAULT_CONN_DISC_INTERVAL 1000
#define DEFAULT_ADDR_TYPE BLE_OWN_ADDR_RANDOM

bool     sNimbleInitialized = false;
bool     sNimbleRunning     = false;
uint16_t sNimbleConnHandle;
uint16_t sNimbleMtu = OT_BLE_ATT_MTU_MAX;

static otInstance *sInstance;

static struct ble_npl_task sTaskBleHost;
static struct ble_npl_task sTaskBleController;
static struct ble_npl_sem  sTaskBleSyncSem;

// Note: static allocation of GATT database. Tune accordingly.
ble_uuid_any_t          sNimbleUuids[OT_BLE_MAX_NUM_UUIDS];
struct ble_gatt_svc_def sNimbleServices[OT_BLE_MAX_NUM_SERVICES];
struct ble_gatt_chr_def sNimbleCharacteristics[OT_BLE_MAX_NUM_CHARACTERISTICS];

int sNimbleUuidsCount           = 0;
int sNimbleServicesCount        = 0;
int sNimbleCharacteristicsCount = 0;

#define L2CAP_COC_MTU (256)
#define L2CAP_COC_BUF_COUNT (3 * MYNEWT_VAL_BLE_L2CAP_COC_MAX_NUM)
#define L2CAP_COC_PSM (0x0055)

struct ble_l2cap_chan *  sNimbleL2capChannel;
struct os_mbuf_pool      sNimbleL2capSduMbufPool;
static struct os_mempool sNimbleL2capSduMemPool;
static os_membuf_t       sNimbleL2capSduMem[OS_MEMPOOL_SIZE(L2CAP_COC_BUF_COUNT, L2CAP_COC_MTU)];

// write state
enum
{
    kStateNone,
    kStateWrite,
    kStateSubscribing,
    kStateUnsubscribing,
};

uint8_t sWriteState = kStateNone;

// NimBLE HCI APIs
void ble_hci_sock_init(void);
void ble_hci_sock_set_device(int dev);
void ble_hci_sock_ack_handler(void *param);

static otError mapNimbleToOtError(int aNimbleError)
{
    otError err;

    switch (aNimbleError)
    {
    case BLE_ERR_SUCCESS:
        err = OT_ERROR_NONE;
        break;
    case BLE_ERR_UNSUPPORTED:
        err = OT_ERROR_NOT_IMPLEMENTED;
        break;
    case BLE_ERR_AUTH_FAIL:
        err = OT_ERROR_SECURITY;
        break;

    case BLE_ERR_CONN_REJ_RESOURCES:
    case BLE_ERR_MEM_CAPACITY:
    case BLE_HS_ENOMEM:
        err = OT_ERROR_NO_BUFS;
        break;

    case BLE_HS_EINVAL:
        err = OT_ERROR_INVALID_ARGS;
        break;
    case BLE_HS_EALREADY:
        err = OT_ERROR_ALREADY;
        break;
    case BLE_HS_ENOADDR:
        err = OT_ERROR_NO_ADDRESS;
        break;

    case BLE_HS_ENOTSYNCED:
    case BLE_HS_EPREEMPTED:
    case BLE_HS_EBUSY:
        err = OT_ERROR_BUSY;
        break;
    default:
        err = OT_ERROR_FAILED;
        break;
    }

    return err;
}

static void mapNimbleToOtAddress(ble_addr_t *aNimbleAddr, otPlatBleDeviceAddr *aOtAddr)
{
    aOtAddr->mAddrType = aNimbleAddr->type;
    memcpy(aOtAddr->mAddr, aNimbleAddr->val, sizeof(aOtAddr->mAddr));
}

static void mapOtToNimbleAddress(const otPlatBleDeviceAddr *aOtAddr, ble_addr_t *aNimbleAddr)
{
    aNimbleAddr->type = aOtAddr->mAddrType;
    memcpy(aNimbleAddr->val, aOtAddr->mAddr, sizeof(aNimbleAddr->val));
}

static void mapNimbleToOtUuid(const ble_uuid_any_t *aNimbleUuid, otPlatBleUuid *aOtUuid)
{
    switch (aNimbleUuid->u.type)
    {
    case BLE_UUID_TYPE_16:
        aOtUuid->mType          = OT_BLE_UUID_TYPE_16;
        aOtUuid->mValue.mUuid16 = aNimbleUuid->u16.value;
        break;

    case BLE_UUID_TYPE_32:
        aOtUuid->mType          = OT_BLE_UUID_TYPE_32;
        aOtUuid->mValue.mUuid32 = aNimbleUuid->u32.value;
        break;

    case BLE_UUID_TYPE_128:
        aOtUuid->mType           = OT_BLE_UUID_TYPE_128;
        aOtUuid->mValue.mUuid128 = (uint8_t *)aNimbleUuid->u128.value;
        break;

    default:
        break;
    }
}

static void mapOtToNimbleUuid(const otPlatBleUuid *aOtUuid, ble_uuid_any_t *aNimbleUuid)
{
    switch (aOtUuid->mType)
    {
    case OT_BLE_UUID_TYPE_16:
        aNimbleUuid->u.type    = BLE_UUID_TYPE_16;
        aNimbleUuid->u16.value = aOtUuid->mValue.mUuid16;
        break;

    case OT_BLE_UUID_TYPE_32:
        aNimbleUuid->u.type    = BLE_UUID_TYPE_32;
        aNimbleUuid->u32.value = aOtUuid->mValue.mUuid32;
        break;

    case OT_BLE_UUID_TYPE_128:
        aNimbleUuid->u.type = BLE_UUID_TYPE_128;
        memcpy(aNimbleUuid->u128.value, aOtUuid->mValue.mUuid128, sizeof(aNimbleUuid->u128.value));
        break;

    default:
        break;
    }
}

static void *task_ble_host(void *aParam)
{
    OT_UNUSED_VARIABLE(aParam);

    nimble_port_run();

    return NULL;
}

static void *task_ble_controller(void *aParam)
{
    OT_UNUSED_VARIABLE(aParam);

    ble_hci_sock_ack_handler(aParam);

    return NULL;
}

static void ble_stack_on_sync(void)
{
    int        err;
    ble_addr_t addr;

    // Use Non-resolvable Random Private Address
    err = ble_hs_id_gen_rnd(1, &addr);
    assert(err == 0);
    err = ble_hs_id_set_rnd(addr.val);
    assert(err == 0);

    err = ble_npl_sem_release(&sTaskBleSyncSem);
}

static int on_l2cap_event(struct ble_l2cap_event *aEvent, void *aArg);

void ble_svc_user_init(void)
{
    int rc = 0;

    ble_svc_gap_init();
    ble_svc_gatt_init();

    if (sNimbleServicesCount == 0)
    {
        // Add bogus service when no user service is passed,
        // as Nimble asserts if only GAP and GATT services defined.
        ble_svc_ans_init();
    }

    // Callback to signal user can initialize GATT registration.
    otPlatBleOnEnabled(sInstance);

    for (int i = 0; i < sNimbleServicesCount; i++)
    {
        if (sNimbleServices[i].type != BLE_GATT_SVC_TYPE_END)
        {
            rc = ble_gatts_count_cfg(&sNimbleServices[i]);
            assert(rc == 0);

            rc = ble_gatts_add_svcs(&sNimbleServices[i]);
            assert(rc == 0);
        }
    }

    ble_l2cap_create_server(L2CAP_COC_PSM, L2CAP_COC_MTU, on_l2cap_event, sInstance);
}

static void ble_l2cap_api_init()
{
    int rc;

    rc = os_mempool_init(&sNimbleL2capSduMemPool, L2CAP_COC_BUF_COUNT, L2CAP_COC_MTU, sNimbleL2capSduMem,
                         "ble l2cap sdu mempool");
    assert(rc == 0);

    rc = os_mbuf_pool_init(&sNimbleL2capSduMbufPool, &sNimbleL2capSduMemPool, L2CAP_COC_MTU, L2CAP_COC_BUF_COUNT);

    assert(rc == 0);
}

static otError nimble_start(void)
{
    otError error = OT_ERROR_NONE;
    uid_t   euid;

    if (!sNimbleInitialized)
    {
        euid = geteuid();
        if (euid != 0)
        {
            // Must run as sudo.
            error = OT_ERROR_INVALID_STATE;
        }
        else
        {
            ble_hci_sock_set_device(otPlatBleHciGetDeviceId(NULL));
            ble_hci_sock_init();
            nimble_port_init();
            ble_svc_user_init();
            ble_l2cap_api_init();

            ble_hs_cfg.sync_cb = ble_stack_on_sync;
            ble_npl_sem_init(&sTaskBleSyncSem, 0);

            ble_npl_task_init(&sTaskBleController, "blc", task_ble_controller, NULL, TASK_DEFAULT_PRIORITY,
                              BLE_NPL_WAIT_FOREVER, TASK_DEFAULT_STACK, TASK_DEFAULT_STACK_SIZE);

            /* Create task which handles default event queue for host stack. */
            ble_npl_task_init(&sTaskBleHost, "blh", task_ble_host, NULL, TASK_DEFAULT_PRIORITY, BLE_NPL_WAIT_FOREVER,
                              TASK_DEFAULT_STACK, TASK_DEFAULT_STACK_SIZE);

            ble_npl_sem_pend(&sTaskBleSyncSem, BLE_NPL_WAIT_FOREVER);
        }
    }

    sNimbleInitialized = true;

    return error;
}

otError otPlatBleEnable(otInstance *aInstance)
{
    otLogDebgBle("%s\n", __func__);

    sInstance      = aInstance;
    sNimbleRunning = true;
    return nimble_start();
}

otError otPlatBleDisable(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    otLogDebgBle("%s\n", __func__);
    ble_hs_sched_reset(0);

    sNimbleRunning = false;

    return OT_ERROR_NONE;
}

otError otPlatBleReset(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    otLogDebgBle("%s\n", __func__);
    ble_hci_trans_reset();
    ble_hs_startup_go();

    return OT_ERROR_NONE;
}

bool otPlatBleIsEnabled(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    otLogDebgBle("%s\n", __func__);
    return sNimbleRunning;
}

//=============================================================================
//                              GAP
//=============================================================================

static int gap_event_cb(struct ble_gap_event *aEvent, void *aArg)
{
    otInstance *instance = (otInstance *)aArg;

    switch (aEvent->type)
    {
    case BLE_GAP_EVENT_CONNECT:
        sNimbleConnHandle = aEvent->connect.conn_handle;
        otLogDebgBle("otPlatBleGapOnConnected\n");
        DISPATCH_OT_BLE(otPlatBleGapOnConnected(instance, aEvent->connect.conn_handle));
        break;

    case BLE_GAP_EVENT_DISCONNECT:
        sNimbleConnHandle = 0;
        otLogDebgBle("otPlatBleGapOnDisonnected\n");
        DISPATCH_OT_BLE(otPlatBleGapOnDisconnected(instance, aEvent->disconnect.conn.conn_handle));
        break;

    case BLE_GAP_EVENT_DISC:
    {
        otBleRadioPacket    packet;
        otPlatBleDeviceAddr address;

        packet.mValue  = aEvent->disc.data;
        packet.mLength = aEvent->disc.length_data;
        mapNimbleToOtAddress(&(aEvent->disc.addr), &address);

        if (aEvent->disc.event_type == BLE_HCI_ADV_RPT_EVTYPE_SCAN_RSP)
        {
            otLogDebgBle("otPlatBleGapOnScanRespReceived\n");
            DISPATCH_OT_BLE(otPlatBleGapOnScanRespReceived(instance, &address, &packet));
        }
        else
        {
            otLogDebgBle("otPlatBleGapOnAdvReceived\n");
            DISPATCH_OT_BLE(otPlatBleGapOnAdvReceived(instance, &address, &packet));
        }
        break;
    }

    case BLE_GAP_EVENT_NOTIFY_RX:
    {
        otBleRadioPacket packet;
        packet.mValue  = OS_MBUF_DATA(aEvent->notify_rx.om, uint8_t *);
        packet.mLength = OS_MBUF_PKTLEN(aEvent->notify_rx.om);
        otLogDebgBle("otPlatBleGattClientOnIndication\n");
        DISPATCH_OT_BLE(otPlatBleGattClientOnIndication(instance, aEvent->notify_rx.attr_handle, &packet));
        break;
    }

    case BLE_GAP_EVENT_NOTIFY_TX:
        if (aEvent->notify_tx.indication && (aEvent->notify_tx.status == BLE_HS_EDONE))
        {
            otLogDebgBle("otPlatBleGattServerOnIndicationConfirmation\n");
            DISPATCH_OT_BLE(otPlatBleGattServerOnIndicationConfirmation(instance, aEvent->notify_tx.attr_handle));
        }
        break;

    case BLE_GAP_EVENT_SUBSCRIBE:
    {
        bool subscribing = aEvent->subscribe.cur_indicate;
        otLogDebgBle("otPlatBleGattServerOnSubscribeRequest\n");
        DISPATCH_OT_BLE(otPlatBleGattServerOnSubscribeRequest(instance, aEvent->subscribe.attr_handle, subscribing));
        break;
    }

    case BLE_GAP_EVENT_DISC_COMPLETE:
    case BLE_GAP_EVENT_ADV_COMPLETE:
    case BLE_GAP_EVENT_MTU:
    default:
        otLogDebgBle("%s: unhandled event %d", __func__, aEvent->type);
        break;
    }

    return 0;
}

otError otPlatBleGapAddressGet(otInstance *aInstance, otPlatBleDeviceAddr *aAddress)
{
    int rc;

    OT_UNUSED_VARIABLE(aInstance);

    otLogDebgBle("%s\n", __func__);
    aAddress->mAddrType = OT_BLE_ADDRESS_TYPE_RANDOM_STATIC;
    rc                  = ble_hs_id_copy_addr(BLE_ADDR_RANDOM, (uint8_t *)&aAddress->mAddr, NULL);

    return mapNimbleToOtError(rc);
}

otError otPlatBleGapAddressSet(otInstance *aInstance, const otPlatBleDeviceAddr *aAddress)
{
    int rc = 0;

    OT_UNUSED_VARIABLE(aInstance);

    otLogDebgBle("%s\n", __func__);
    switch (aAddress->mAddrType)
    {
    case OT_BLE_ADDRESS_TYPE_PUBLIC:
        /*
         * We shouldn't be writing to the controller's address (g_dev_addr).
         * There is no standard way to set the local public address, so this is
         * our only option at the moment.
         */
        // memcpy(g_dev_addr, aAddress->mAddr, sizeof(aAddress->mAddr));
        // ble_hs_id_set_pub(g_dev_addr);
        // break;
        rc = BLE_ERR_UNSUPPORTED;
        break;

    case OT_BLE_ADDRESS_TYPE_RANDOM_STATIC:
        rc = ble_hs_id_set_rnd(aAddress->mAddr);
        break;
    }

    return mapNimbleToOtError(rc);
}

otError otPlatBleGapServiceSet(otInstance *aInstance, const char *aDeviceName, uint16_t aAppearance)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aAppearance);

    otLogDebgBle("%s\n", __func__);
    int rc = ble_svc_gap_device_name_set(aDeviceName);
    return mapNimbleToOtError(rc);
}

otError otPlatBleGapConnParamsSet(otInstance *aInstance, const otPlatBleGapConnParams *aConnParams)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aConnParams);

    otLogDebgBle("%s\n", __func__);
    return OT_ERROR_NOT_IMPLEMENTED;
}

otError otPlatBleGapAdvDataSet(otInstance *aInstance, const uint8_t *aAdvData, uint8_t aAdvDataLength)
{
    int rc = ble_gap_adv_set_data(aAdvData, aAdvDataLength);

    OT_UNUSED_VARIABLE(aInstance);

    otLogDebgBle("%s\n", __func__);
    return mapNimbleToOtError(rc);
}

otError otPlatBleGapScanResponseSet(otInstance *aInstance, const uint8_t *aScanResponse, uint8_t aScanResponseLength)
{
    int rc = ble_gap_adv_rsp_set_data(aScanResponse, aScanResponseLength);

    OT_UNUSED_VARIABLE(aInstance);

    otLogDebgBle("%s\n", __func__);
    return mapNimbleToOtError(rc);
}

otError otPlatBleGapAdvStart(otInstance *aInstance, uint16_t aInterval, uint8_t aType)
{
    struct ble_gap_adv_params advp;
    int                       rc;

    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aInterval);

    otLogDebgBle("%s\n", __func__);
    memset(&advp, 0, sizeof advp);

    if (aType | OT_BLE_ADV_MODE_CONNECTABLE)
    {
        advp.conn_mode = BLE_GAP_CONN_MODE_UND;
    }
    else
    {
        advp.conn_mode = BLE_GAP_CONN_MODE_NON;
    }

    if (aType | OT_BLE_ADV_MODE_SCANNABLE)
    {
        advp.disc_mode = BLE_GAP_DISC_MODE_GEN;
    }
    else
    {
        advp.disc_mode = BLE_GAP_DISC_MODE_NON;
    }

    rc = ble_gap_adv_start(DEFAULT_ADDR_TYPE, NULL, BLE_HS_FOREVER, &advp, gap_event_cb, aInstance);

    return mapNimbleToOtError(rc);
}

otError otPlatBleGapAdvStop(otInstance *aInstance)
{
    int rc = ble_gap_adv_stop();

    OT_UNUSED_VARIABLE(aInstance);

    otLogDebgBle("%s\n", __func__);
    return mapNimbleToOtError(rc);
}

otError otPlatBleGapScanStart(otInstance *aInstance, uint16_t aInterval, uint16_t aWindow)
{
    OT_UNUSED_VARIABLE(aInstance);

    struct ble_gap_disc_params discParams;
    discParams.itvl              = aInterval;
    discParams.window            = aWindow;
    discParams.passive           = 1;
    discParams.limited           = 0;
    discParams.filter_policy     = 0;
    discParams.filter_duplicates = 0;

    otLogDebgBle("%s\n", __func__);
    int rc = ble_gap_disc(BLE_ADDR_PUBLIC, BLE_HS_FOREVER, &discParams, gap_event_cb, aInstance);

    return mapNimbleToOtError(rc);
}

otError otPlatBleGapScanStop(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    otLogDebgBle("%s\n", __func__);
    int rc = ble_gap_disc_cancel();

    return mapNimbleToOtError(rc);
}

otError otPlatBleGapConnect(otInstance *         aInstance,
                            otPlatBleDeviceAddr *aAddress,
                            uint16_t             aScanInterval,
                            uint16_t             aScanWindow)
{
    OT_UNUSED_VARIABLE(aInstance);

    int                        rc;
    ble_addr_t                 peerAddr;
    struct ble_gap_conn_params connParams;

    connParams.scan_itvl           = aScanInterval;
    connParams.scan_window         = aScanWindow;
    connParams.itvl_min            = 40;
    connParams.itvl_max            = 56;
    connParams.latency             = 0;
    connParams.supervision_timeout = OT_BLE_CONN_SUPERVISOR_TIMEOUT_MAX;
    connParams.min_ce_len          = 0;
    connParams.max_ce_len          = 0;

    otLogDebgBle("%s\n", __func__);
    mapOtToNimbleAddress(aAddress, &peerAddr);

    rc = ble_gap_connect(aAddress->mAddrType, &peerAddr, DEFAULT_CONN_DISC_INTERVAL, &connParams, gap_event_cb,
                         aInstance);

    return mapNimbleToOtError(rc);
}

otError otPlatBleGapDisconnect(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    int rc;
    otLogDebgBle("%s\n", __func__);
    rc = ble_gap_terminate(sNimbleConnHandle, BLE_ERR_REM_USER_CONN_TERM);
    return mapNimbleToOtError(rc);
}

//=============================================================================
//                        GATT COMMON
//=============================================================================

otError otPlatBleGattMtuGet(otInstance *aInstance, uint16_t *aMtu)
{
    OT_UNUSED_VARIABLE(aInstance);

    otLogDebgBle("%s: mtu=%d\n", __func__, sNimbleMtu);
    *aMtu = sNimbleMtu;

    return OT_ERROR_NONE;
}

static int gatt_event_cb(uint16_t aConnHandle, uint16_t aAttrHandle, struct ble_gatt_access_ctxt *aContext, void *aArg)
{
    int         rc       = 0;
    otInstance *instance = (otInstance *)aArg;

    OT_UNUSED_VARIABLE(aConnHandle);

    switch (aContext->op)
    {
    case BLE_GATT_ACCESS_OP_WRITE_CHR:
    {
        otBleRadioPacket packet;
        packet.mValue  = OS_MBUF_DATA(aContext->om, uint8_t *);
        packet.mLength = OS_MBUF_PKTLEN(aContext->om);

        otLogDebgBle("otPlatBleGattServerOnWriteRequest: handle=%d\n", aAttrHandle);

        DISPATCH_OT_BLE(otPlatBleGattServerOnWriteRequest(instance, aAttrHandle, &packet));
        break;
    }

    case BLE_GATT_ACCESS_OP_READ_CHR:
    {
        otBleRadioPacket packet;
        otLogDebgBle("otPlatBleGattServerOnReadRequest: handle=%d\n", aAttrHandle);

        // TODO: segfault -- this API assumes return will fill packet with data
        DISPATCH_OT_BLE(otPlatBleGattServerOnReadRequest(instance, aAttrHandle, &packet));

        rc = os_mbuf_append(aContext->om, packet.mValue, packet.mLength);
        break;
    }

    case BLE_GATT_ACCESS_OP_WRITE_DSC:
    {
        bool subscribing = (OS_MBUF_DATA(aContext->om, uint16_t *)[0] != 0);
        otLogDebgBle("otPlatBleGattServerOnSubscribeRequest: handle=%d\n", aAttrHandle);
        DISPATCH_OT_BLE(otPlatBleGattServerOnSubscribeRequest(instance, aAttrHandle, subscribing));
        break;
    }

    default:
        otLogDebgBle("%s: unhandled event %d", __func__, aContext->op);
        break;
    }

    return rc;
}

//=============================================================================
//                        GATT SERVER
//=============================================================================

/**
 * Helper function for otPlatBleGattServerServicesRegister.
 * Note: Nimble only supports one-time registration of entire GATT database.
 */
static otError bleGattServerCharacteristicRegister(otInstance *                 aInstance,
                                                   uint16_t                     aServiceHandle,
                                                   otPlatBleGattCharacteristic *aChar,
                                                   bool                         aCccd)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aCccd);

    ble_uuid_any_t *         uuid;
    struct ble_gatt_svc_def *svc;
    struct ble_gatt_chr_def *chr;

    if (aServiceHandle >= sNimbleServicesCount)
        return OT_ERROR_INVALID_STATE;

    svc  = &sNimbleServices[aServiceHandle];
    chr  = &sNimbleCharacteristics[sNimbleCharacteristicsCount];
    uuid = &sNimbleUuids[sNimbleUuidsCount];

    mapOtToNimbleUuid(&aChar->mUuid, uuid);

    chr->access_cb  = gatt_event_cb;
    chr->uuid       = &uuid->u;
    chr->flags      = aChar->mProperties;
    chr->val_handle = &aChar->mHandleValue; // Nimble auto-fill handles in otPlatBleGattCharacteristic

    if (svc->characteristics == NULL)
    {
        svc->characteristics = chr;
    }

    sNimbleUuidsCount++;
    sNimbleCharacteristicsCount++;

    chr           = &sNimbleCharacteristics[sNimbleCharacteristicsCount];
    *(void **)chr = NULL;

    return OT_ERROR_NONE;
}

static otError bleGattServerServiceRegister(otInstance *aInstance, const otPlatBleUuid *aUuid, uint16_t *aHandle)
{
    OT_UNUSED_VARIABLE(aInstance);

    otError error = OT_ERROR_NONE;

    ble_uuid_any_t *         uuid = &sNimbleUuids[sNimbleUuidsCount];
    struct ble_gatt_svc_def *svc  = &sNimbleServices[sNimbleServicesCount];

    // if (sNimbleServicesCount > OT_BLE_MAX_NUM_SERVICES) return

    svc->type = BLE_GATT_SVC_TYPE_PRIMARY;
    svc->uuid = &uuid->u;
    mapOtToNimbleUuid(aUuid, uuid);
    *aHandle = sNimbleUuidsCount;

    // Increment to next slot and set it to NULL.
    sNimbleUuidsCount++;
    sNimbleServicesCount++;
    svc       = &sNimbleServices[sNimbleServicesCount];
    svc->type = BLE_GATT_SVC_TYPE_END;

    return error;
}

otError otPlatBleGattServerServicesRegister(otInstance *aInstance, otPlatBleGattService *aServices)
{
    otError                      error = OT_ERROR_NONE;
    otPlatBleGattCharacteristic *chr;

    otLogDebgBle("%s\n", __func__);
    while (aServices && (aServices->mUuid.mType != OT_BLE_UUID_TYPE_NONE))
    {
        bleGattServerServiceRegister(aInstance, &aServices->mUuid, &aServices->mHandle);

        chr = aServices->mCharacteristics;

        while (chr && (chr->mUuid.mType != OT_BLE_UUID_TYPE_NONE))
        {
            bleGattServerCharacteristicRegister(aInstance, aServices->mHandle, chr, true);
            chr++;
        }

        aServices += sizeof(otPlatBleGattService);
    }

    return error;
}

otError otPlatBleGattServerIndicate(otInstance *aInstance, uint16_t aHandle, otBleRadioPacket *aPacket)
{
    OT_UNUSED_VARIABLE(aInstance);

    int             rc;
    struct os_mbuf *mbuf;

    otLogDebgBle("%s\n", __func__);
    mbuf = ble_hs_mbuf_from_flat(aPacket->mValue, aPacket->mLength);
    rc   = ble_gattc_indicate_custom(sNimbleConnHandle, aHandle, mbuf);

    otLogInfoBle("[BLE] %s err=%d\r\n", __func__, rc);

    return mapNimbleToOtError(rc);
}

//=============================================================================
//                        GATT CLIENT
//=============================================================================

static int on_gattc_read(uint16_t                     aConnHandle,
                         const struct ble_gatt_error *aError,
                         struct ble_gatt_attr *       aAttr,
                         void *                       aArg)
{
    otInstance *     instance = (otInstance *)aArg;
    otBleRadioPacket packet;

    OT_UNUSED_VARIABLE(aConnHandle);
    OT_UNUSED_VARIABLE(aError);

    packet.mValue  = OS_MBUF_DATA(aAttr->om, uint8_t *);
    packet.mLength = OS_MBUF_PKTLEN(aAttr->om);

    otLogDebgBle("otPlatBleGattClientOnReadResponse\n");
    DISPATCH_OT_BLE(otPlatBleGattClientOnReadResponse(instance, &packet));

    return 0;
}

otError otPlatBleGattClientRead(otInstance *aInstance, uint16_t aHandle)
{
    OT_UNUSED_VARIABLE(aInstance);

    otLogDebgBle("%s handle=%d\n", __func__, aHandle);
    int rc = ble_gattc_read(sNimbleConnHandle, aHandle, on_gattc_read, aInstance);

    return mapNimbleToOtError(rc);
}

static int on_gattc_write(uint16_t                     aConnHandle,
                          const struct ble_gatt_error *aError,
                          struct ble_gatt_attr *       aAttr,
                          void *                       aArg)
{
    otInstance *instance = (otInstance *)aArg;
    uint8_t     state    = sWriteState;

    OT_UNUSED_VARIABLE(aConnHandle);
    OT_UNUSED_VARIABLE(aError);

    sWriteState = kStateNone;

    switch (state)
    {
    default:
    case kStateNone:
        otLogCritBle("Error: GattClientOnWriteResponse illegal state\n");

        // fall-through

    case kStateWrite:
        otLogDebgBle("otPlatBleGattClientOnWriteResponse: handle=%d\n", aAttr->handle);
        DISPATCH_OT_BLE(otPlatBleGattClientOnWriteResponse(instance, aAttr->handle));
        break;

    case kStateSubscribing:
    case kStateUnsubscribing:
        otLogDebgBle("otPlatBleGattClientOnSubscribeResponse: handle=%d\n", aAttr->handle);
        DISPATCH_OT_BLE(otPlatBleGattClientOnSubscribeResponse(instance, aAttr->handle));
        break;
    }

    return 0;
}

otError otPlatBleGattClientWrite(otInstance *aInstance, uint16_t aHandle, otBleRadioPacket *aPacket)
{
    OT_UNUSED_VARIABLE(aInstance);

    // Preserve state if subscribing/unsubscribing.
    if (sWriteState == kStateNone)
    {
        sWriteState = kStateWrite;
    }

    otLogDebgBle("%s handle=%d\n", __func__, aHandle);
    int rc =
        ble_gattc_write_flat(sNimbleConnHandle, aHandle, aPacket->mValue, aPacket->mLength, on_gattc_write, aInstance);

    return mapNimbleToOtError(rc);
}

otError otPlatBleGattClientSubscribeRequest(otInstance *aInstance, uint16_t aHandle, bool aSubscribing)
{
    const uint8_t    kGattSubscribeReqValue[]   = {2, 0};
    const uint8_t    kGattUnsubscribeReqValue[] = {0, 0};
    otBleRadioPacket packet;

    otLogDebgBle("%s\n", __func__);
    sWriteState    = (aSubscribing) ? kStateSubscribing : kStateUnsubscribing;
    packet.mValue  = (aSubscribing) ? (uint8_t *)kGattSubscribeReqValue : (uint8_t *)kGattUnsubscribeReqValue;
    packet.mLength = sizeof(kGattSubscribeReqValue);
    return otPlatBleGattClientWrite(aInstance, aHandle, &packet);
}

static int on_gatt_disc_s(uint16_t                     aConnHandle,
                          const struct ble_gatt_error *error,
                          const struct ble_gatt_svc *  service,
                          void *                       aArg)
{
    otInstance *instance = (otInstance *)aArg;

    OT_UNUSED_VARIABLE(aConnHandle);

    if (error->status == BLE_HS_EDONE)
    {
        return 0;
    }

    if (service)
    {
        otLogDebgBle("otPlatBleGattClientOnServiceDiscovered uuid=0x%04x start=%d end=%d\n", service->uuid.u16.value,
                     service->start_handle, service->end_handle);
        DISPATCH_OT_BLE(otPlatBleGattClientOnServiceDiscovered(instance, service->start_handle, service->end_handle,
                                                               service->uuid.u16.value,
                                                               mapNimbleToOtError(error->status)));
    }
    else
    {
        otLogDebgBle("otPlatBleGattClientOnServiceDiscovered\n");
        DISPATCH_OT_BLE(otPlatBleGattClientOnServiceDiscovered(instance, 0xFFFF, 0xFFFF, 0xFFFF,
                                                               mapNimbleToOtError(error->status)));
    }
    return 0;
}

otError otPlatBleGattClientServicesDiscover(otInstance *aInstance)
{
    otLogDebgBle("%s\n", __func__);
    int rc = ble_gattc_disc_all_svcs(sNimbleConnHandle, on_gatt_disc_s, aInstance);

    return mapNimbleToOtError(rc);
}

otError otPlatBleGattClientServiceDiscover(otInstance *aInstance, const otPlatBleUuid *aUuid)
{
    ble_uuid_any_t uuid;
    mapOtToNimbleUuid(aUuid, &uuid);

    otLogDebgBle("%s\n", __func__);
    int rc = ble_gattc_disc_svc_by_uuid(sNimbleConnHandle, &uuid.u, on_gatt_disc_s, aInstance);

    return mapNimbleToOtError(rc);
}

static int on_gatt_disc_c(uint16_t                     aConnHandle,
                          const struct ble_gatt_error *error,
                          const struct ble_gatt_chr *  chr,
                          void *                       aArg)
{
    otInstance *                instance = (otInstance *)aArg;
    otPlatBleGattCharacteristic characterstic;

    OT_UNUSED_VARIABLE(aConnHandle);

    if (error->status == BLE_HS_EDONE)
    {
        return 0;
    }

    if (chr)
    {
        characterstic.mHandleValue = chr->val_handle;
        characterstic.mHandleCccd  = OT_BLE_INVALID_HANDLE;
        characterstic.mProperties  = chr->properties;
        mapNimbleToOtUuid(&chr->uuid, &characterstic.mUuid);
    }

    otLogDebgBle("otPlatBleGattClientOnCharacteristicsDiscoverDone\n");
    DISPATCH_OT_BLE(otPlatBleGattClientOnCharacteristicsDiscoverDone(instance, &characterstic, 1,
                                                                     mapNimbleToOtError(error->status)));

    return BLE_HS_EDONE;
}

otError otPlatBleGattClientCharacteristicsDiscover(otInstance *aInstance, uint16_t aStartHandle, uint16_t aEndHandle)
{
    otLogDebgBle("%s start=%d end=%d\n", __func__, aStartHandle, aEndHandle);
    int rc = ble_gattc_disc_all_chrs(sNimbleConnHandle, aStartHandle, aEndHandle, on_gatt_disc_c, aInstance);

    return mapNimbleToOtError(rc);
}

static int on_gatt_disc_d(uint16_t                     aConnHandle,
                          const struct ble_gatt_error *aError,
                          uint16_t                     aCharValHandle,
                          const struct ble_gatt_dsc *  aDesc,
                          void *                       aArg)
{
    otInstance *            instance = (otInstance *)aArg;
    otPlatBleGattDescriptor desc;

    OT_UNUSED_VARIABLE(aConnHandle);
    OT_UNUSED_VARIABLE(aCharValHandle);

    if (aError->status == BLE_HS_EDONE)
    {
        return 0;
    }

    if (aDesc)
    {
        desc.mHandle = aDesc->handle;
        mapNimbleToOtUuid(&aDesc->uuid, &desc.mUuid);
    }

    otLogDebgBle("otPlatBleGattClientOnDescriptorsDiscoverDone\n");
    DISPATCH_OT_BLE(
        otPlatBleGattClientOnDescriptorsDiscoverDone(instance, &desc, 1, mapNimbleToOtError(aError->status)));

    return BLE_HS_EDONE;
}

otError otPlatBleGattClientDescriptorsDiscover(otInstance *aInstance, uint16_t aStartHandle, uint16_t aEndHandle)
{
    otLogDebgBle("%s start=%d end=%d\n", __func__, aStartHandle, aEndHandle);
    int rc = ble_gattc_disc_all_dscs(sNimbleConnHandle, aStartHandle, aEndHandle, on_gatt_disc_d, aInstance);

    return mapNimbleToOtError(rc);
}

static int on_gatt_mtu(uint16_t aConnHandle, const struct ble_gatt_error *aError, uint16_t aMtu, void *aArg)
{
    otInstance *instance = (otInstance *)aArg;

    OT_UNUSED_VARIABLE(aConnHandle);

    sNimbleMtu = aMtu;
    otLogDebgBle("otPlatBleGattClientOnMtuExchangeResponse\n");
    DISPATCH_OT_BLE(otPlatBleGattClientOnMtuExchangeResponse(instance, aMtu, mapNimbleToOtError(aError->status)));

    return 0;
}

otError otPlatBleGattClientMtuExchangeRequest(otInstance *aInstance, uint16_t aMtu)
{
    OT_UNUSED_VARIABLE(aMtu);

    otLogDebgBle("%s\n", __func__);
    int rc = ble_gattc_exchange_mtu(sNimbleConnHandle, on_gatt_mtu, aInstance);

    return mapNimbleToOtError(rc);
}

//=============================================================================
//                        L2CAP
//=============================================================================

static void on_l2cap_coc_recv(struct ble_l2cap_chan *chan, struct os_mbuf *sdu)
{
    otLogDebgBle("LE CoC SDU received, chan: 0x%08lx, data len %d\n", (uintptr_t)chan, OS_MBUF_PKTLEN(sdu));

    os_mbuf_free_chain(sdu);
    sdu = os_mbuf_get_pkthdr(&sNimbleL2capSduMbufPool, 0);
    assert(sdu != NULL);

    ble_l2cap_recv_ready(chan, sdu);
}

static int on_l2cap_coc_accept(uint16_t aConnHandle, uint16_t aPeerMtu, struct ble_l2cap_chan *aChannel)
{
    OT_UNUSED_VARIABLE(aConnHandle);

    struct os_mbuf *sdu_rx;

    otLogDebgBle("LE CoC accepting, chan: 0x%08lx, peer_mtu %d\n", (uintptr_t)aChannel, aPeerMtu);

    sdu_rx = os_mbuf_get_pkthdr(&sNimbleL2capSduMbufPool, 0);
    if (!sdu_rx)
    {
        return BLE_HS_ENOMEM;
    }

    ble_l2cap_recv_ready(aChannel, sdu_rx);

    return 0;
}

static int on_l2cap_event(struct ble_l2cap_event *aEvent, void *aArg)
{
    otInstance *instance = (otInstance *)aArg;

    switch (aEvent->type)
    {
    case BLE_L2CAP_EVENT_COC_CONNECTED:
        sNimbleL2capChannel = aEvent->connect.chan;
        otLogDebgBle("otPlatBleL2capOnConnectionRequest\n");
        DISPATCH_OT_BLE(otPlatBleL2capOnConnectionRequest(instance, aEvent->connect.chan->psm,
                                                          aEvent->connect.chan->my_mtu, aEvent->connect.chan->scid));
        break;

    case BLE_L2CAP_EVENT_COC_DISCONNECTED:
        sNimbleL2capChannel = NULL;
        otLogDebgBle("otPlatBleL2capOnDisconnect\n");
        DISPATCH_OT_BLE(
            otPlatBleL2capOnDisconnect(instance, aEvent->disconnect.chan->scid, aEvent->disconnect.chan->dcid));
        break;

    case BLE_L2CAP_EVENT_COC_ACCEPT:
    {
        sNimbleL2capChannel = aEvent->connect.chan;
        otLogDebgBle("otPlatBleL2capOnConnectionResponse\n");
        DISPATCH_OT_BLE(otPlatBleL2capOnConnectionResponse(instance, OT_BLE_L2C_ERROR_NONE,
                                                           aEvent->accept.chan->peer_mtu, aEvent->accept.chan->dcid));

        on_l2cap_coc_accept(aEvent->accept.conn_handle, aEvent->accept.peer_sdu_size, aEvent->accept.chan);
        break;
    }

    case BLE_L2CAP_EVENT_COC_DATA_RECEIVED:
    {
        otBleRadioPacket packet;
        packet.mValue  = OS_MBUF_DATA(aEvent->receive.sdu_rx, uint8_t *);
        packet.mLength = OS_MBUF_PKTLEN(aEvent->receive.sdu_rx);
        otLogDebgBle("otPlatBleL2capOnSduReceived\n");
        DISPATCH_OT_BLE(
            otPlatBleL2capOnSduReceived(instance, aEvent->receive.chan->scid, aEvent->receive.chan->dcid, &packet));

        assert(sNimbleL2capChannel == aEvent->receive.chan);

        on_l2cap_coc_recv(aEvent->receive.chan, aEvent->receive.sdu_rx);
        break;
    }

    default:
        break;
    }

    return 0;
}

otError otPlatBleL2capConnectionRequest(otInstance *aInstance, uint16_t aPsm, uint16_t aMtu, uint16_t *aCid)
{
    int rc;

    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aCid);

    struct os_mbuf *sdu_rx;

    otLogDebgBle("%s\n", __func__);
    sdu_rx = os_mbuf_get_pkthdr(&sNimbleL2capSduMbufPool, 0);
    assert(sdu_rx != NULL);

    rc = ble_l2cap_connect(sNimbleConnHandle, aPsm, aMtu, sdu_rx, on_l2cap_event, (void *)aInstance);

    return mapNimbleToOtError(rc);
}

otError otPlatBleL2capDisconnect(otInstance *aInstance, uint16_t aLocalCid, uint16_t aPeerCid)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aLocalCid);
    OT_UNUSED_VARIABLE(aPeerCid);

    otLogDebgBle("%s\n", __func__);
    int rc = ble_l2cap_disconnect(sNimbleL2capChannel);

    return mapNimbleToOtError(rc);
}

otError otPlatBleL2capConnectionResponse(otInstance *        aInstance,
                                         otPlatBleL2capError aResult,
                                         uint16_t            aMtu,
                                         uint16_t *          aCid)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aResult);
    OT_UNUSED_VARIABLE(aMtu);
    OT_UNUSED_VARIABLE(aCid);

    otLogDebgBle("%s\n", __func__);
    return OT_ERROR_NOT_IMPLEMENTED;
}

otError otPlatBleL2capSduSend(otInstance *aInstance, uint16_t aLocalCid, uint16_t aPeerCid, otBleRadioPacket *aPacket)
{
    int             rc;
    struct os_mbuf *sdu_tx;

    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aLocalCid);
    OT_UNUSED_VARIABLE(aPeerCid);
    OT_UNUSED_VARIABLE(aPacket);

    otLogDebgBle("%s\n", __func__);
    sdu_tx = os_mbuf_get_pkthdr(&sNimbleL2capSduMbufPool, 0);
    if (sdu_tx == NULL)
    {
        return OT_ERROR_NO_BUFS;
    }

    rc = os_mbuf_append(sdu_tx, aPacket->mValue, aPacket->mLength);
    if (rc)
    {
        os_mbuf_free_chain(sdu_tx);
        return mapNimbleToOtError(rc);
    }

    rc = ble_l2cap_send(sNimbleL2capChannel, sdu_tx);

    return mapNimbleToOtError(rc);
}

//=============================================================================
//                        HCI
//=============================================================================

extern uint32_t gNodeId;

int otPlatBleHciGetDeviceId(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    return gNodeId;
}

void otPlatBleHciSetDeviceId(otInstance *aInstance, int aDeviceId)
{
    OT_UNUSED_VARIABLE(aInstance);

    if (aDeviceId >= 0)
    {
        otLogDebgBle("otPlatBleHciSetDeviceId: %d", aDeviceId);

        gNodeId = aDeviceId;

        ble_hci_sock_set_device(aDeviceId);
    }
}

#ifdef __cplusplus
}
#endif
