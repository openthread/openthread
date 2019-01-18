# OpenThread CLI - CoAP Example

The OpenThread CoAP APIs may be invoked via the OpenThread CLI.

## Quick Start

### Build with CoAP API support

Use the `COAP=1` build switch to enable CoAP API support.

```bash
> ./bootstrap
> make -f examples/Makefile-posix COAP=1
```

### Form Network

Form a network with at least two devices.

### Node 1

On node 1, setup CoAP server with resource `test-resource`.

```bash
> coap start
Done
> coap resource test-resource
Done
```

### Node 2

```bash
> coap start
Done
> coap get fdde:ad00:beef:0:d395:daee:a75:3964 test-resource
Done
coap response from [fdde:ad00:beef:0:2780:9423:166c:1aac] with payload: 30
> coap put fdde:ad00:beef:0:2780:9423:166c:1aac test-resource con payload
Done
coap response from [fdde:ad00:beef:0:2780:9423:166c:1aac]
```

### Result

On node 1, you should see output similar to below:

```bash
coap request from [fdde:ad00:beef:0:b3:e3f6:2dcc:4b79] GET
coap response sent
coap request from [fdde:ad00:beef:0:b3:e3f6:2dcc:4b79] PUT with payload: 7061796c6f6164
coap response sent
```

## Command List

* [help](#help)
* [delete](#delete-address-uri-path-type-payload)
* [get](#get-address-uri-path-type)
* [post](#post-address-uri-path-type-payload)
* [put](#put-address-uri-path-type-payload)
# [resource](#resource-uri-path)
* [start](#start)
* [stop](#stop)

## Command Details

### help

```bash
> coap help
help
delete
get
post
put
resource
start
stop
Done
```

List the CoAP CLI commands.

### delete \<address\> \<uri-path\> \[type\] \[payload\]

* address: IPv6 address of the CoAP server.
* uri-path: URI path of the resource.
* type: "con" for Confirmable or "non-con" for Non-confirmable (default).
* payload: CoAP request payload.

```bash
> coap delete fdde:ad00:beef:0:2780:9423:166c:1aac test-resource con payload
Done
```

### get \<address\> \<uri-path\> \[type\]

* address: IPv6 address of the CoAP server.
* uri-path: URI path of the resource.
* type: "con" for Confirmable or "non-con" for Non-confirmable (default).

```bash
> coap get fdde:ad00:beef:0:2780:9423:166c:1aac test-resource
Done
```

### post \<address\> \<uri-path\> \[type\] \[payload\]

* address: IPv6 address of the CoAP server.
* uri-path: URI path of the resource.
* type: "con" for Confirmable or "non-con" for Non-confirmable (default).
* payload: CoAP request payload.

```bash
> coap post fdde:ad00:beef:0:2780:9423:166c:1aac test-resource con payload
Done
```

### put \<address\> \<uri-path\> \[type\] \[payload\]

* address: IPv6 address of the CoAP server.
* uri-path: URI path of the resource.
* type: "con" for Confirmable or "non-con" for Non-confirmable (default).
* payload: CoAP request payload.

```bash
> coap put fdde:ad00:beef:0:2780:9423:166c:1aac test-resource con payload
Done
```

### resource \[uri-path\]

Sets the URI path for the test resource.

```bash
> coap resource test-resource
Done
> coap resource
test-resource
Done
```

### start

Starts the application coap service.

```bash
> coap start
Done
```

### stop

Stops the application coap service.

```bash
> coap stop
Done
```
