# OpenThread with MQTT-SN client support

* [OpenThread with MQTT-SN client support](#OpenThread-with-MQTT-SN-client-support)<br>
  * [Contents](#Contents)<br>
  * [About OpenThread](#About-OpenThread)<br>
  * [About MQTT-SN](#About-MQTT-SN)<br>
* [Trying MQTT-SN client with CLI application example](#Trying-MQTT-SN-client-with-CLI-application-example)<br>
  * [Requirements](#Requirements)<br>
  * [Build and flash NCP](#Build-and-flash-NCP)<br>
  * [Install border router with MQTT-SN gateway](#Install-border-router-with-MQTT-SN-gateway)
  * [Build CLI example](#Build-CLI-example)
  * [Connect to the broker](#Connect-to-the-broker)
  * [Subscribe the topic](#Subscribe-the-topic)
  * [Publish a message](#Publish-a-message)

### Overview

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

After a successful build, the elf files are found in output/kw41z/bin. You can convert them to bin files using arm-none-eabi-objcopy:

```
cd /output/kw41z/bin
arm-none-eabi-objcopy -O binary ot-ncp-ftd ot-ncp-ftd.bin
```

Then flash the binary and connect NCP device to border router device.

### Install border router with MQTT-SN gateway

Border router device provides functionality for routing and  forwarding communication from Thread subnet to other IP networks. In this example border router consists of [OpenThread Border Router](https://openthread.io/guides/border-router) (OTBR) which is capable of routing communication with Thread network and [Eclipse Paho MQTT-SN Gateway](https://github.com/eclipse/paho.mqtt-sn.embedded-c/tree/master/MQTTSNGateway) which transforms MQTT-SN UDP packets to MQTT.

All applications for border router are provided as Docker images. First step is to install docker on your platform ([Docker install guide](https://docs.docker.com/v17.09/engine/installation/)). On Raspberry Pi platform just run following script:

```
curl -sSL https://get.docker.com | sh
```

In real application it is best practice to use DNS server for resolving host IP address. In this example are used simple static addresses for locating services. Create custom docker network `test` for this purpose:

```
sudo docker network create --subnet=172.18.0.0/16 test
```

Run new OTBR container from official image:

```
sudo docker run -d --name otbr --sysctl "net.ipv6.conf.all.disable_ipv6=0 \
        net.ipv4.conf.all.forwarding=1 net.ipv6.conf.all.forwarding=1" -p 80:8080 \
        --dns=127.0.0.1 -v /dev/ttyACM0:/dev/ttyACM0 --net test --ip 172.18.0.6 \
        --privileged openthread/otbr --ncp-path /dev/ttyACM0 --nat64-prefix "2018:ff9b::/96"
```

Container will use `test` network with static IP address 172.18.0.6. If needed replace `/dev/ttyACM0` in `-v` and `--ncp-path` parameter with name under which appear NCP device in your system (`/dev/ttyS0`, `/dev/ttyUSB0` etc.). NAT-64 prefix is set to `2018:ff9b::/96`. It allows address translation and routing to local addresses.

Next step is to run Mosquitto container as MQTT broker for sample test. Broker IP address in `test` network will be 172.18.0.7:

```
sudo docker run -d --name mosquitto --net test --ip 172.18.0.7 kyberpunk/mosquitto
```

As last step run container with MQTT-SN Gateway:

```
sudo docker run -d --name paho --net test --ip 172.18.0.8 kyberpunk/paho \
        --broker-name 172.18.0.7 --broker-port 1883
```

MQTT-SN gateway service address is 172.18.0.8 which can be translated to IPv6 as 2018:ff9b::ac12:8. See more information [here](https://openthread.io/guides/thread-primer/ipv6-addressing).

**IMPORTANT NOTICE: In this network configuration MQTT-SN network does not support SEARCHGW and ADVERTISE messages in Thread network until you configure multicast forwarding.** Alternativelly you can use UDPv6 version of gateway ([kyberpunk/paho6](https://hub.docker.com/repository/docker/kyberpunk/paho6) image) and attach it to OTBR container interface wpan0 (`--net "container:otbr"`).

### Build CLI example

Build the CLI example firmware accordingly to your [platform](https://openthread.io/platforms). For example for KW41Z platform run:

```
make -f examples/Makefile-kw41z MQTT=1 JOINER=1
```

Convert fimware to binary and flash your device with `ot-cli-ftd.bin`.

```
cd /output/kw41z/bin
arm-none-eabi-objcopy -O binary ot-cli-ftd ot-cli-ftd.bin
```

### Connect to the broker

Firs of all the CLI device must be commisioned into the Thread network. Follow the the [OTBR commissioning guide](https://openthread.io/guides/border-router/external-commissioning). When device joined the Thread network you can start MQTT-SN service and connect to gateway which is reachable on NAT-64 translated IPv6 address 2018:ff9b::ac12:8.

```bash
> mqtt start
Done
> mqtt connect 2018:ff9b::ac12:8 10000
Done
connected
```

You will see `connected` message when client successfully connected to the gateway. Client stays connected and periodically sends keepalive messages. See more information about `connect` command in [CLI reference](src/cli/README_MQTT.md#connect).

You can also see log of messages forwarded by MQTT-SN gateway:

```
docker logs paho
```

### Subscribe the topic

### Publish a message
