# OpenThread CLI - Latency Test

The OpenThread UDP APIs may be invoked via the OpenThread CLI. 

This test needs to enable the time sync by setting the following environment variables:

```bash
COMMONCFLAGS += -DOPENTHREAD_CONFIG_TIME_SYNC_REQUIRED=1
COMMONCFLAGS += -DOPENTHREAD_CONFIG_ENABLE_TIME_SYNC=1
COMMONCFLAGS += -DOPENTHREAD_CONFIG_ENABLE_PERFORMANCE_TEST=1
```

This feature is only tested on Nordic platform.

## Quick Start

### Form Network

Form a network with at least two devices.

### Node 1

On node 1, open and bind the UDP socket.

```bash
> latency open
> latency bind :: 1234
```

The `::` specifies the IPv6 Unspecified Address.

### Node 2

On node 2, open the UDP socket and send a message with a certain size of payload.

```bash
> latency open
> latency send fdde:ad00:beef:0:bb1:ebd6:ad10:f33 1234 64
```

### Result

On node 1, you should see a print out similar to below:

```bash
64 bytes from fdde:ad00:beef:0:dac3:6792:e2e:90d8 49153 xxxx
> latency result
7726
Done
> latency hoplimit
64
Done
```

## Command List

* [help](#help)
* [bind](#bind-ip-port)
* [close](#close)
* [connect](#connect-ip-port)
* [open](#open)
* [send](#send-ip-port-payload)
* [result](#result)
* [hoplimit](#hoplimit)

## Command Details

### help

List the UDP CLI commands.

```bash
> udp help
help
bind
close
connect
hoplimit
open
result
send
Done
```

### bind \<ip\> \<port\>

Assigns a name (i.e. IPv6 address and port) to the UDP socket.
* ip: the IPv6 address or the unspecified IPv6 address (`::`).
* port: the UDP port

```bash
> latency bind :: 1234
Done
```

### close

Closes the UDP socket.

```bash
> latency close
Done
```

### open

Opens the UDP socket.

```bash
> latency open
Done
```

### send \<ip\> \<port\> \<payload\>

Send a UDP message.

* ip: the IPv6 destination address.
* port: the UDP destination port.
* UDP payload size: the message with the size of payload to send.

```bash
> latency send fdde:ad00:beef:0:bb1:ebd6:ad10:f33 1234 64
```
### result

Get the latency value.

```bash
> latency result
7726
Done
```
### hoplimit

Get the hoplimit value.

```bash
> latency hoplimit
64
Done
```