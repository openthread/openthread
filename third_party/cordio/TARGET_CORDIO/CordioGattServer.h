/* mbed Microcontroller Library
 * Copyright (c) 2017-2017 ARM Limited
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef CORDIO_GATT_SERVER_H_
#define CORDIO_GATT_SERVER_H_

#include <stddef.h>
#include "ble/blecommon.h"
#include "ble/GattServer.h"
#include "ble/pal/SigningEventMonitor.h"
#include "ble/Gap.h"
#include "wsf_types.h"
#include "att_api.h"
#include "SecurityManager.h"

/*! Maximum count of characteristics that can be stored for authorisation purposes */
#define MAX_CHARACTERISTIC_AUTHORIZATION_CNT 20

/*! client characteristic configuration descriptors settings */
#define MAX_CCCD_CNT 20

namespace ble {

// fwd declaration of CordioAttClient and BLE
namespace pal {
namespace vendor {
namespace cordio {
class CordioAttClient;
}
}
}

namespace vendor {
namespace cordio {

class BLE;

/**
 * Cordio implementation of ::GattServer
 */
class GattServer : public ::ble::interface::GattServer<GattServer>,
                   public pal::SigningEventMonitor<GattServer, impl::SigningEventHandler>
{
    friend ble::vendor::cordio::BLE;
    friend ble::pal::vendor::cordio::CordioAttClient;

    typedef ::ble::interface::GattServer<GattServer> Base;

public:
    /**
     * Return the singleton of the Cordio implementation of ::GattServer.
     */
    static GattServer &getInstance();

    ::GattServer::EventHandler* getEventHandler() {
        return eventHandler;
    }

    /**
     * Initialize the GattServer and add mandatory services (generic access and
     * generic attribute service).
     */
    void initialize();

    /**
     * @see ::GattServer::addService
     */
    ble_error_t addService_(GattService &);

    /**
     * @see ::GattServer::read
     */
    ble_error_t read_(
        GattAttribute::Handle_t attributeHandle,
        uint8_t buffer[],
        uint16_t *lengthP
    );

    /**
     * @see ::GattServer::read
     */
    ble_error_t read_(
        connection_handle_t connectionHandle,
        GattAttribute::Handle_t attributeHandle,
        uint8_t buffer[], uint16_t *lengthP
    );

    /**
     * @see ::GattServer::write
     */
    ble_error_t write_(
        GattAttribute::Handle_t,
        const uint8_t[], uint16_t,
        bool localOnly = false
    );

    /**
     * @see ::GattServer::write
     */
    ble_error_t write_(
        connection_handle_t connectionHandle,
        GattAttribute::Handle_t,
        const uint8_t[],
        uint16_t,
        bool localOnly = false
    );

    /**
     * @see ::GattServer::areUpdatesEnabled
     */
    ble_error_t areUpdatesEnabled_(
        const GattCharacteristic &characteristic, bool *enabledP
    );

    /**
     * @see ::GattServer::areUpdatesEnabled
     */
    ble_error_t areUpdatesEnabled_(
        connection_handle_t connectionHandle,
        const GattCharacteristic &characteristic,
        bool *enabledP
    );

    /**
     * @see ::GattServer::isOnDataReadAvailable
     */
    bool isOnDataReadAvailable_() const;

    /**
     * @see ::GattServer::getPreferredConnectionParams
     */
    ::Gap::ConnectionParams_t getPreferredConnectionParams();

    /**
     * @see ::GattServer::setPreferredConnectionParams
     */
    void setPreferredConnectionParams(const ::Gap::ConnectionParams_t& params);

    /**
     * @see ::GattServer::setDeviceName
     */
    ble_error_t setDeviceName(const uint8_t *deviceName);

    /**
     * @see ::GattServer::getDeviceName
     */
    void getDeviceName(const uint8_t*& name, uint16_t& length);

    /**
     * @see ::GattServer::setAppearance
     */
    void setAppearance(GapAdvertisingData::Appearance appearance);

    /**
     * @see ::GattServer::getAppearance
     */
    GapAdvertisingData::Appearance getAppearance();

    /**
     * @see ::GattServer::reset
     */
    ble_error_t reset_(void);

    /**
     * @see pal::SigningEventMonitor::set_signing_event_handler
     */
    void set_signing_event_handler_(
        impl::SigningEventHandler *signing_event_handler
    ) {
        _signing_event_handler = signing_event_handler;
    }


private:
    void add_default_services();

    static uint16_t compute_attributes_count(GattService& service);

    void insert_service_attribute(
        GattService& service,
        attsAttr_t *&attribute_it
    );

    ble_error_t insert_characteristic(
        GattCharacteristic *characteristic,
        attsAttr_t *&attribute_it
    );

    bool is_characteristic_valid(GattCharacteristic *characteristic);

    void insert_characteristic_declaration_attribute(
        GattCharacteristic *characteristic,
        attsAttr_t *&attribute_it
    );

    ble_error_t insert_characteristic_value_attribute(
        GattCharacteristic *characteristic,
        attsAttr_t *&attribute_it
    );

    ble_error_t insert_descriptor(
        GattCharacteristic *characteristic,
        GattAttribute* descriptor,
        attsAttr_t *&attribute_it,
        bool& cccd_created
    );

    ble_error_t insert_cccd(
        GattCharacteristic *characteristic,
        attsAttr_t *&attribute_it
    );

    static void cccd_cb(attsCccEvt_t *pEvt);
    static void att_cb(const attEvt_t *pEvt);
    static uint8_t atts_read_cb(dmConnId_t connId, uint16_t handle, uint8_t operation, uint16_t offset, attsAttr_t *pAttr);
    static uint8_t atts_write_cb(dmConnId_t connId, uint16_t handle, uint8_t operation, uint16_t offset, uint16_t len, uint8_t *pValue, attsAttr_t *pAttr);
    static uint8_t atts_auth_cb(dmConnId_t connId, uint8_t permit, uint16_t handle);
    void add_generic_access_service();
    void add_generic_attribute_service();
    void* alloc_block(size_t block_size);
    GattCharacteristic* get_auth_char(uint16_t value_handle);
    bool get_cccd_index_by_cccd_handle(GattAttribute::Handle_t cccd_handle, uint8_t& idx) const;
    bool get_cccd_index_by_value_handle(GattAttribute::Handle_t char_handle, uint8_t& idx) const;
    bool is_update_authorized(connection_handle_t connection, GattAttribute::Handle_t value_handle);

    struct alloc_block_t {
        alloc_block_t* next;
        uint8_t data[1];
    };

    struct internal_service_t {
        attsGroup_t attGroup;
        internal_service_t *next;
    };

    impl::SigningEventHandler *_signing_event_handler;

    attsCccSet_t cccds[MAX_CCCD_CNT];
    uint16_t cccd_values[MAX_CCCD_CNT];
    uint16_t cccd_handles[MAX_CCCD_CNT];
    uint8_t cccd_cnt;

    GattCharacteristic *_auth_char[MAX_CHARACTERISTIC_AUTHORIZATION_CNT];
    uint8_t _auth_char_count;

    struct {
        attsGroup_t service;
        attsAttr_t attributes[7];
        uint8_t device_name_declaration_value[5];
        uint16_t device_name_length;
        uint8_t appearance_declaration_value[5];
        uint16_t appearance;
        uint8_t ppcp_declaration_value[5];
        uint8_t ppcp[8];

        uint8_t*& device_name_value() {
            return attributes[2].pValue;
        }
    } generic_access_service;

    struct {
        attsGroup_t service;
        attsAttr_t attributes[4];
        uint8_t service_changed_declaration[5];
    } generic_attribute_service;

    internal_service_t* registered_service;
    alloc_block_t* allocated_blocks;

    uint16_t currentHandle;

    bool default_services_added;

private:
    GattServer();

    GattServer(const GattServer &);
    const GattServer& operator=(const GattServer &);
};

} // namespace cordio
} // namespace vendor
} // namespace ble

#endif /* CORDIO_GATT_SERVER_H_ */
