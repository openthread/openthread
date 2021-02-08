# OpenThread CLI - SRP Client

## Command List

Usage : `srp client [command] ...`

- [help](#help)
- [callback](#callback)
- [host](#host)
- [keyleaseinterval](#keyleaseinterval)
- [leaseinterval](#leaseinterval)
- [server](#server)
- [service](#service)
- [start](#start)
- [stop](#stop)

## Command Details

### help

Usage: `srp client help`

Print SRP client help menu.

```bash
> srp client help
callback
help
host
keyleaseinterval
leaseinterval
service
start
stop
Done
```

### callback

Usage `srp client callback [enable|disable]`

Enable/Disable printing callback events from SRP client.

Get current callback mode

```bash
> srp client callback
Disabled
Done
```

Set callback mode

```bash
> srp client callback enable
Done

> srp client callback
Enabled
Done
```

#### Example

When two services are successfully registered:

```bash
SRP client callback - error:OK
Host info:
    name:"dev4312", state:Registered, addrs:[fd00:0:0:0:0:0:0:1]
Service list:
    instance:"ins2", name:"_test2._udp", state:Registered, port:111, priority:1, weight:1
    instance:"ins1", name:"_test1._udp", state:Registered, port:777, priority:0, weight:0
```

When service `ins2` is removed:

```bash
SRP client callback - error:OK
Host info:
    name:"dev4312", state:Registered, addrs:[fd00:0:0:0:0:0:0:1]
Service list:
    instance:"ins1", name:"_test1._udp", state:Registered, port:777, priority:0, weight:0
Removed service list:
    instance:"ins2", name:"_test2._udp", state:Removed, port:111, priority:1, weight:1
```

When host info (and all services) is removed:

```bash
SRP client callback - error:OK
Host info:
    name:"dev4312", state:Removed, addrs:[fd00:0:0:0:0:0:0:1]
Service list:
Removed service list:
    instance:"ins1", name:"_test1._udp", state:Removed, port:777, priority:0, weight:0
```

### host

Usage: `srp client host`

Print the full host info (host name, state, list of host addresses).

```bash
> srp client host
name:"dev4312", state:Registered, addrs:[fd00:0:0:0:0:0:0:1234, fd00:0:0:0:0:0:0:beef]
Done
```

### host name

Usage: `srp client host name [name]`

Get the host name.

```bash
> srp client host name
dev4312
Done
```

Set host name (can be set when the host is removed or not yet registered with server).

```bash
srp client host name dev4312
Done
```

### host address

Usage : `srp client host address [<address> ...]`

Get the list of host addresses.

```bash
> srp client host address
fd00:0:0:0:0:0:0:1234
fd00:0:0:0:0:0:0:beef
Done
```

Set the list of host addresses (can be set while client is running to update the host addresses)

```bash
> srp client host address fd00::cafe
Done
```

### host state

Usage: `srp client host state`

Get the host state.

```bash
> srp client host state
Registered
Done
```

The possible states are (same value for service state):

- `ToAdd`: item to be added/registered.
- `Adding`: item is being added/registered.
- `ToRefresh`: item to be refreshed (renew lease).
- `Refreshing`: item is being refreshed.
- `ToRemove`: item to be removed.
- `Removing`: item is being removed.
- `Registered`: item is registered with server.
- `Removed`: item is removed.

### host remove

Usage: `srp client host remove [removekeylease]`

Remove host info and all services from server. `removekeylease` is boolean value indicating whether or not the host key lease should also be removed

```bash
> srp client host remove 1
Done
```

### host clear

Usage: `srp client host clear`

Clear host info and all services on client (unlike `host remove`, with `host clear` no update is sent to server).

```bash
> srp client host clear
Done
```

### keyleaseinterval

Usage: `srp client keyleaseinterval [interval]`

Get the key lease interval (in seconds).

```bash
> srp client keyleaseinterval
1209600
Done
>
```

Set the key lease interval.

```bash
> srp client keyleaseinterval 864000
Done
```

### leaseinterval

Usage: `srp client leaseinterval [interval]`

Get the lease interval (in seconds).

```bash
> srp client leaseinterval
7200
Done
>
```

Set the lease interval.

```bash
> srp client leaseinterval 3600
Done
```

### server

Usage: `srp client server`

Print the server socket address (IPv6 address and port number).

```bash
> srp client server
[fd00:0:0:0:d88a:618b:384d:e760]:4724
Done
```

### server address

Print the server IPv6 address.

```bash
> srp client server address
fd00:0:0:0:d88a:618b:384d:e760
Done
```

### server port

Print the server port number

```bash
> srp client server port
4724
Done
```

### service

Usage: `srp client service`

Print the list of services.

```bash
> srp client service
instance:"ins2", name:"_test2._udp", state:Registered, port:111, priority:1, weight:1
instance:"ins1", name:"_test1._udp", state:Registered, port:777, priority:0, weight:0
Done
```

### service add

Usage: `srp client service add <instancename> <servicename> <port> [priority] [weight] [txt]`

Add a service with a given instance name, service name, port number, priority, weight and txt values. The priority and weight are optional and if not provided zero will be used. The txt should follow hex-string format and is treated as an already encoded TXT data byte sequence. It is also optional and if not provided it is considered empty.

```bash
> srp client service add ins2 _test2._udp 111 1 1
Done
```

### service remove

Usage: `srp client service remove <instancename> <servicename>`

Remove a service with a give instance name and service name.

```bash
> srp client service remove ins2 _test2._udp
Done
```

### start

Usage: `srp client start <serveraddr> <serverport>`

Start the SRP client with a given server IPv6 address and port number

```bash
> srp client start fd00::d88a:618b:384d:e760 4724
Done
```

### stop

Usage: `srp client stop`

Stop the SRP client.

```bash
> srp client stop
Done
```
