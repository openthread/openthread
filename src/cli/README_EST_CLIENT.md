# OpenThread CLI - EST Client Example

The OpenThread EST Client APIs may be invoked via the OpenThread CLI.

## Preparations

An [OpenThread Border Router](https://openthread.io/guides/border-router/) with IPv6 connectivity.

## Quick Start

### Build with EST Client API support

Use the `EST_CLIENT=1` build switch to enable EST Client API support and `JOINER=1`to enable Joiner support.

```bash
> ./bootstrap
> make -f examples/Makefile-posix EST_CLIENT=1 JOINER=1
```

### Attach to Existing Network

Commission the device to the Thread network of the OpenThread Border Router.

### Interact

1. Start the EST Client:

```bash
> est start
Done
```

2. Connect to the EST server: 
Hint: If no IPv6 address is set for <ipv6-of-est-server>, the client will connect to the default IP address 
OT_EST_COAPS_DEFAULT_EST_SERVER_IP6 in `include/openthread/est_client.h`. The X509 certificate used for the
DTLS connection are stored in `core/cli/x509_cert_key.hpp`.

```bash
> est connect <ipv6-of-est-server>
Done
connected
```

3. Start simple enrollment:

```bash
> est enroll
Done
enrollment successful
```
The client is now in possession of a X509 certifcate issued by the server side Certificate Authority (CA).

4. Start simple reenrollment:

Reenrollment is only allowed if a client uses a X509 certificate issued by the server side CA during the DTLS handshake. So, in this 
example, the client uses the certificate received through the simple enrollment the next time `est connect` is used.  

```bash
> est disconnect
Done
disconnected
> est connect <ipv6-of-est-server>
Done
connected
> est reenroll
enrollment successful
```

## Command List

* [help](#help)
* [connect](#connect)
* [disconnect](#disconnect)
* [enroll](#enroll)
* [reenroll](#reenroll)
* [start](#start)
* [stop](#stop)

## Command Details

### help

```bash
> est help
help
start
stop
connect
disconnect
enroll
reenroll
Done
```

List the EST Client CLI commands.

### connect \[address\]

Establish DTLS session with server.

* address: IPv6 address of the server.

```bash
> est connect 2001:620:190:ffa1:21b:21ff:fe70:9240
Done
connected
```

### disconnect

```bash
> est disconnect 
Done
disconnected
```

### enroll

```bash
> est enroll
Done
enrollment successful
```

### reenroll

```bash
> est reenroll
Done
enrollment successful
```

### start

Starts the application EST Client service. 

```bash
> est start
Done
```

### stop

Stops the application EST Client service. Also deletes the certificate received through the re-/enrollment.

```bash
> est stop
Done
```
