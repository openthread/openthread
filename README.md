# OpenThread with MQTT-SN client support

This project contains fork of OpenThread SDK which implements MQTT-SN protocol on Thread network. MQTT implementation is merged into OpenThread fork for easier integration. This is not official OpenThread project and there can be some bugs or missing features. If you want to use latest OpenThread project please go to [official repository](https://github.com/openthread/openthread).

Provided MQTT-SN client implementation implements almost all features specified by protocol MQTT-SN v1.2. There is introduced [C API](include/openthread/mqttsn.h) for custom applications and [CLI commands](src/cli/README_MQTT.md) which can be used for basic client functions evaluation.

## About OpenThread

OpenThread released by Google is an open-source implementation of the Thread networking protocol. Google Nest has released OpenThread to make the technology used in Nest products more broadly available to developers to accelerate the development of products for the connected home.

## About MQTT-SN

[MQTT-SN v1.2](https://mqtt.org/tag/mqtt-sn) is formerly known as MQTT-S. MQTT for Sensor Networks is aimed at embedded devices on non-TCP/IP networks, such as Zigbee. MQTT-SN is a publish/subscribe messaging protocol for wireless sensor networks (WSN), with the aim of extending the MQTT protocol beyond the reach of TCP/IP infrastructure for Sensor and Actuator solutions. More informations can be found in [MQTT-SN specification](http://www.mqtt.org/new/wp-content/uploads/2009/06/MQTT-SN_spec_v1.2.pdf).

**This project is currently in progress. See original repository [openthread-mqttsn](https://github.com/kyberpunk/openthread-mqttsn) for more information**.

## CLI example
There is implemented CLI API for testing MQTT-SN client. CLI commands reference can be found [here](src/cli/README_MQTT.md).
