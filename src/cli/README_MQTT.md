# OpenThread CLI - MQTT-SN Example

This OpenThread fork contains MQTT-SN protocol support. The MQTT client API may be invoked via the OpenThread CLI. For more details about the operations see [the specification](http://mqtt.org/documentation).

## Command Details

### help

Usage: `mqtt help`

Print dataset help menu.

```bash
> mqtt help
help
start
stop
connect
subscribe
state
register
publish
unsubscribe
disconnect
sleep
awake
searchgw
Done
```

### start

Usage: `mqtt start`

Start the MQTT-SN client. Must be called before performing other operations.

```bash
> mqtt start
Done
```

### stop

Usage: `mqtt stop`

Stop the MQTT-SN client and free allocated resources.

```bash
> mqtt stop
Done
```

### connect

Usage: `mqtt connect <gateway-ip> <gateway-port>`

Connect to MQTT-SN gateway.

* gateway-ip: IPv6 address of MQTT-SN gateway.
* gateway-port: Port number of MQTT-SN service.

```bash
> mqtt connect 2018:ff9b::ac12:8 10000
Done
connected
```

### subscribe

Usage: `mqtt subscribe <topic> [qos]`

Subscribe to the topic.

* topic: Name of the topic to subscribe.
* qos: Requested quality of service level for the topic. Possible values are 0, 1, 2, -1. Default value is 1 (optional).

Specific topic ID is returned after response. This ID is used for publising to the topic.

```bash
> mqtt subscribe sensors
Done
subscribed topic id: 1
```

### state

Usage: `mqtt state`

Get current state of MQTT-SN client.

```bash
> mqtt state
Disconnected
Done
```

### register

Usage: `mqtt register <topic>`

Register the topic.

* topic: Name of the topic to be registered.

Specific topic ID is returned after response. This ID is used for publising to the topic.

```bash
> mqtt register sensors
Done
registered topic id: 1
```

### publish

Usage: `mqtt publish <topic-id> [payload] [qos]`

Publish data to the topic. Data are encoded as string and white spaces are currentyl not supported (separate words are recognized as additional parameters).

* topic-id: ID of the topic. Can be obtained by [register](#register).
* payload: Text to be send in publish message payload. If empty, then empty data are send (optional).
* qos: Quality of service of publish. Possible values are 0, 1, or 2. Default value is 1 (optional).

```bash
> mqtt publish 1 {"temperature":24.0}
Done
published
```

### unsubscribe

Usage: `mqtt unsubscribe <topic>`

Unsubscribe from the topic.

* topic: Name of the topic to unsubscribe.

```bash
> mqtt unsubscribe sensors
Done
unsubscribed
```

### disconnect

Usage: `mqtt disconnect`

Disconnect from the gateway.

```bash
> mqtt disconnect
Done
disconnected reason: Client
```

### sleep

Usage: `mqtt sleep <duration>`

Take client to the sleeping state.

* duration: Sleep duration time in s.

```bash
> mqtt sleep 60
Done
disconnected reason: Asleep
```

### awake

Usage: `mqtt awake <timeout>`

Take client to the awake state from the sleeping state. In this state device can receive buffered messages from the gateway.

* timeout: Timeout in ms for whis is the client awake before going asleep.

```bash
> mqtt awake 1000
Done
connected
disconnected reason: Asleep
```

### searchgw

Usage: `mqtt searchgw <multicast-ip> <port> <radius>`

Send searchgw multicast message. If there is any MQTT-SN gateway listening in the same network it will answer with its connection details.

* multicast-ip: Multicast IPv6 address for spread the searchgw message.
* port: Destination port of MQTT-SN gateway.
* radius: Hop limit for multicast message.

```bash
> mqtt searchgw ff03::1 10000 5
Done
searchgw response from gateway id 1 with address: 2018:ff9b:0:0:0:0:ac12:8
```
