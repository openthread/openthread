# OpenThread CLI - TCP Example

The OpenThread TCP APIs may be invoked via the OpenThread CLI.

## Quick Start

### Form Network

Form a network with at least two devices.

### Node 1

On node 1, initialize the TCP CLI module and listen for incoming connections using the example TCP listener.

```bash
> tcp init
> tcp listen :: 30000
```

The `::` specifies the IPv6 Unspecified Address.

### Node 2

On node 2, initialize the TCP CLI module, connect to node 1, and send a simple message.

```bash
> tcp init
> tcp connect fe80:0:0:0:a8df:580a:860:ffa4 30000
> tcp send hello
```

### Result

After running the `tcp connect` command on node 2, you should see a printout on node 2 similar to below:

```bash
TCP: Connection established
```

In addition, you should also see a printout on node 1 similar to below:

```bash
Accepted connection from [fe80:0:0:0:8f3:f602:bf9b:52f2]:49152
TCP: Connection established
```

After running the `tcp send` command on node 2, you should see a printout on node 1 similar to below:

```bash
TCP: Received 5 bytes: hello
```

For a more in-depth example, see [this video](https://youtu.be/ppZ784YUKlI).

## Command List

- [help](#help)
- [init](#init-size)
- [deinit](#deinit)
- [bind](#bind-ip-port)
- [connect](#connect-ip-port-fastopen)
- [send](#send-message)
- [benchmark](#benchmark-run-size)
- [sendend](#sendend)
- [abort](#abort)
- [listen](#listen-ip-port)
- [stoplistening](#stoplistening)

## Command Details

### abort

Unceremoniously ends the TCP connection, if one exists, associated with the example TCP endpoint, transitioning the TCP endpoint to the closed state.

```bash
> tcp abort
TCP: Connection reset
Done
```

### benchmark run [\<size\>]

Transfers the specified number of bytes using the TCP connection currently associated with the example TCP endpoint (this TCP connection must be established before using this command).

- size: the number of bytes to send for the benchmark. If it is left unspecified, the default size is used.

```bash
> tcp benchmark run
Done
TCP Benchmark Complete: Transferred 73728 bytes in 7233 milliseconds
TCP Goodput: 81.546 kb/s
```

### benchmark result

Get the last result of TCP benchmark. If the benchmark is ongoing, it will show that benchmark is ongoing. This command is used for test scripts which automate the tcp benchmark test.

```
> tcp benchmark result
TCP Benchmark Status: Ongoing
Done

> tcp benchmark result
TCP Benchmark Status: Completed
TCP Benchmark Complete: Transferred 73728 bytes in 7056 milliseconds
TCP Goodput: 83.592 kb/s
```

### bind \<ip\> \<port\>

Associates a name (i.e. IPv6 address and port) to the example TCP endpoint.

- ip: the IPv6 address or the unspecified IPv6 address (`::`).
- port: the TCP port.

```bash
> tcp bind :: 30000
Done
```

### connect \<ip\> \<port\> [\<fastopen\>]

Establishes a connection with the specified peer.

If the connection establishment is successful, the resulting TCP connection is associated with the example TCP endpoint.

- ip: the peer's IP address.
- port: the peer's TCP port.
- fastopen: if "fast", TCP Fast Open is enabled for this connection; if "slow", it is not. Defaults to "slow".

```bash
> tcp connect fe80:0:0:0:a8df:580a:860:ffa4 30000
Done
TCP: Connection established
```

The address can be an IPv4 address, which will be synthesized to an IPv6 address using the preferred NAT64 prefix from the network data.

> Note: The command will return `InvalidState` when the preferred NAT64 prefix is unavailable.

```bash
> tcp connect 172.17.0.1 1234
Connecting to synthesized IPv6 address: fdde:ad00:beef:2:0:0:ac11:1
Done
```

### deinit

Deinitializes the example TCP listener and the example TCP endpoint.

```bash
> tcp deinit
Done
```

### help

List the TCP CLI commands.

```bash
> tcp help
abort
benchmark
bind
connect
deinit
help
init
listen
send-message
sendend
stoplistening
Done
```

### init [\<mode\>]&nbsp;[\<size\>]

Initializes the example TCP listener and the example TCP endpoint.

- mode: this specifies the buffering strategy and whether to use TLS. The possible values are "linked", "circular" (default), and "tls".
- size: the size of the receive buffer to associate with the example TCP endpoint. If left unspecified, the maximum size is used.

If "tls" is used, then the TLS protocol will be used for the connection (on top of TCP). When communicating over TCP between two nodes, either both should use TLS or neither should (a non-TLS endpoint cannot communicate with a TLS endpoint). The first two options, "linked" and "circular", specify that TLS should not be used and specify a buffering strategy to use with TCP; two endpoints of a TCP connection may use different buffering strategies.

The behaviors of "linked" and "circular" buffering are identical, but the option is provided so that users of TCP can inspect the code to see an example of using the two buffering strategies.

```bash
> tcp init tls
Done
```

### listen \<ip\> \<port\>

Uses the example TCP listener to listen for incoming connections on the specified name (i.e. IPv6 address and port).

If no TCP connection is associated with the example TCP endpoint, then any incoming connections matching the specified name are accepted and associated with the example TCP endpoint.

- ip: the IPv6 address or the unspecified IPv6 address (`::`).
- port: the TCP port.

```bash
> tcp listen :: 30000
Done
```

### send \<message\>

Send data over the TCP connection associated with the example TCP endpoint.

- message: the message to send.

```bash
> tcp send hello
Done
```

### sendend

Sends the "end of stream" signal (i.e., FIN segment) over the TCP connection associated with the example TCP endpoint. This promises the peer that no more data will be sent to it over this TCP connection.

```bash
> tcp sendend
Done
```

### stoplistening

Stops listening for incoming TCP connections using the example TCP listener.

```bash
> tcp stoplistening
Done
```
