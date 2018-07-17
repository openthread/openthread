# OpenThread CLI - COAPS Example

The OpenThread CoAP Secure APIs may be invoked via the OpenThread CLI.

CoAP Secure use DTLS (over UDP) to make a end to end encrypted connection.

## Init (for Server and Client)

 1. start (init) the coaps api and starts listen on coaps port (5684)
 
```bash
> coaps start
```

### For use PSK with AES128 CCM8

 2a. enter your psk and his identifier
 
```bash
coaps set psk <yourPsk> <PskIdentifier>
```

### For use ECDHE ECDSA with AES128 CCM8

 2b. set the private key and .X509 certificate stored in core/cli/x509_cert_key.hpp.
 
 > _optional_: add your [own](#create-ec-private-key) X.509 certificate and private key to 'core/cli/x509_cert_key.hpp'.
 
```bash
coaps set x509
```

## CoAP Secure Server

 * add a coap resource to provide to a coap client
 
```bash
coaps resource <coapUri>
```

## Connect to a DTLS Server

 * connect to a dtls server
 
```bash
coaps connect <serversIp> (port, if not default)
```

## CoAP Secure Client

* get a resource form the server

```bash
coaps get (serversIp) <coapUri> (Con/NotCon) (payload)
```

> post, put and delete also possible
   
   
<> must
() opt
   
   
## Complete example for DTLS/CoAP server (Node 1)

In this example the coap server is also the dtls server.
The dtls server waits for incoming connection on coaps port 5684.
The [Node 2](#complete-example-dtlscoap-client-node-2) below is able to connect to this coaps server. 

> Note: Node 1 and Node 2 must use the same mode. Either PSK or Certificate based.   

```
 Node 1
---------
|CoAPS  |
|Server | <--Listen on Port 5684-- (Node 2)
|       |
---------
```

### with PSK

```bash
coaps start
coaps set psk secretPSK Client_identity
coaps resource test
```

### with Certificate

```bash
coaps start (false)
coaps set x509
coaps resource test
```

* param false: optional, disables peer certificate validation.

## Complete example DTLS/CoAP client (Node 2)

In this example the coap client is also the dtls client.
The dlts client can connect to an coaps server which is listen on coaps port 5684, e.g to the [Node 1](#complete-example-for-dtlscoap-server-node-1) above.

> Note: Node 1 and Node 2 must use the same mode. Either PSK or Certificate based.   

```
 Node 2
---------
|CoAPS  |
|Client |--Connect to Server on Port 5684--> (Node 1)
|       |
---------
```

### with PSK

```bash
coaps start
coaps set psk secretPSK Client_identity
coaps connect 2001:620:190:ffa1::321
coaps get test
coaps disconnect
```

### with Certificate

```bash
coaps start (false)
coaps set x509
coaps connect <server_ip>
coaps get test
coaps disconnect
```

* param false: optional, disables peer certificate validation.

## Create own Private Key with a .X509 Certificate

### Create EC Private Key

```bash
openssl ecparam -genkey -out myECKey.pem -name prime256v1 -noout
```

* ecparam: Key for Elliptic Curve Algorithms
* -name: The elliptic curve to chose. a list of available curves `openssl ecparam -list_curves`
* -noout: private key without EC Parameters

### Create .X509 Certificate

```bash
openssl req -x509 -new -key myECKey.pem -out myX509Cert.pem -days 30
```

* -days: validity time of certificate
