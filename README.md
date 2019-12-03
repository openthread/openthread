# OpenThread with MQTT-SN client support

**This project is currently in progress. See original repository [openthread-mqttsn](https://github.com/kyberpunk/openthread-mqttsn) for more information**.

This project contains fork of OpenThread SDK which implements MQTT-SN protocol on Thread network. MQTT-SN implementation is part of OpenThread library build. MQTT-SN implementation allows user to send MQTT messages from Thread network to regular MQTT broker in external IP network. This is not official OpenThread project. OpenThread code may be outdated and there can be some bugs or missing features. If you want to use latest OpenThread project please go to [official repository](https://github.com/openthread/openthread).

* MQTT-SN over Thread network uses UDP as transport layer. UDP MQTT-SN packets are tranformed and forwarded to MQTT broker by [Eclipse Paho MQTT-SN Gateway](https://github.com/eclipse/paho.mqtt-sn.embedded-c/tree/master/MQTTSNGateway).

* There is introduced [C API](include/openthread/mqttsn.h) for custom applications and [CLI commands](src/cli/README_MQTT.md) which can be used for basic client functions evaluation.

**Provided MQTT-SN client implements most important features specified by protocol MQTT-SN v1.2:**

* Publish and subscribe with QoS level 0, 1, 2 and -1
* Periodic keepalive requests
* Multicast gateway search and advertising
* Sleep mode for sleeping devices

### About OpenThread

OpenThread released by Google is an open-source implementation of the Thread networking protocol. Google Nest has released OpenThread to make the technology used in Nest products more broadly available to developers to accelerate the development of products for the connected home.

### About MQTT-SN

[MQTT-SN v1.2](https://mqtt.org/tag/mqtt-sn) is formerly known as MQTT-S. MQTT for Sensor Networks is aimed at embedded devices on non-TCP/IP networks, such as Zigbee. MQTT-SN is a publish/subscribe messaging protocol for wireless sensor networks (WSN), with the aim of extending the MQTT protocol beyond the reach of TCP/IP infrastructure for Sensor and Actuator solutions. More informations can be found in [MQTT-SN specification](http://www.mqtt.org/new/wp-content/uploads/2009/06/MQTT-SN_spec_v1.2.pdf).

## Trying MQTT-SN client with CLI application example

### Requirements

* Linux device such as [Raspberry Pi](https://www.raspberrypi.org/) acting as border router with installed Docker - [Docker install guide](https://docs.docker.com/v17.09/engine/installation/), [guide for Raspberry](https://www.raspberrypi.org/blog/docker-comes-to-raspberry-pi/)
* 2 Thread devices compatible with OpenThread - check [here](https://openthread.io/platforms)
* Environment for building firmware with [GNU Arm Embedded Toolchain](https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm)

### Build and flash NCP

OpenThread Border Router runs on an NCP design. First clone openthread MQTT-SN repository:

```
git clone https://github.com/kyberpunk/openthread.git --recursive
```

Select a supported OpenThread platform to use as an NCP and follow the [official building instructions](https://openthread.io/guides/border-router/build). For example for KW41Z platform run following commands:

```
cd openthread
./script/bootstrap
./bootstrap
make -f examples/Makefile-kw41z BORDER_AGENT=1 BORDER_ROUTER=1 COMMISSIONER=1 UDP_FORWARD=1
```

