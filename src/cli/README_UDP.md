# OpenThread CLI - UDP Example

The OpenThread UDP APIs may be invoked via the OpenThread CLI.

## Quick Start

### Form Network

Form a network with at least two devices.

### Node 1

On node 1, open and bind the example UDP socket.

```bash
> udp open
> udp bind :: 1234
```

The `::` specifies the IPv6 Unspecified Address.

### Node 2

On node 2, open the example UDP socket and send a simple message.

```bash
> udp open
> udp send fdde:ad00:beef:0:bb1:ebd6:ad10:f33 1234 hello
```

### Result

On node 1, you should see a print out similar to below:

```bash
5 bytes from fdde:ad00:beef:0:dac3:6792:e2e:90d8 49153 hello
```

## Command List

* [help](#help)
* [bind](#bind-ip-port)
* [close](#close)
* [connect](#connect-ip-port)
* [open](#open)
* [send](#send-ip-port-message)

## Command Details

### help

List the UDP CLI commands.

```bash
> udp help
help
bind
close
connect
open
send
Done
```

### bind \<ip\> \<port\>

Assigns a name (i.e. IPv6 address and port) to the example socket.
* ip: the IPv6 address or the unspecified IPv6 address (`::`).
* port: the UDP port

```bash
> udp bind :: 1234
Done
```

### close

Closes the example socket.

```bash
> udp close
Done
```

### connect \<ip\> \<port\>

Specifies the peer with which the socket is to be associated.

* ip: the peer's IPv6 address.
* port: the peer's UDP port.

```bash
> udp connect fdde:ad00:beef:0:bb1:ebd6:ad10:f33 1234
Done
```

### open

Opens the example socket.

```bash
> udp open
Done
```

### send \<ip\> \<port\> \<message\>

Send a UDP message.

* ip: the IPv6 destination address.
* port: the UDP destination port.
* message: the message to send.

```bash
> udp send fdde:ad00:beef:0:bb1:ebd6:ad10:f33 1234 hello
```

### send \<message\>

Send a UDP message on a connected socket.

* message: the message to send.

```bash
> udp send hello
```
