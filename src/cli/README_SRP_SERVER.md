# OpenThread CLI - SRP Server

## Quick Start

See [README_SRP.md](README_SRP.md).

## Command List

- [help](#help)
- [addrmode](#addrmode)
- [auto](#auto)
- [disable](#disable)
- [domain](#domain)
- [enable](#enable)
- [host](#host)
- [lease](#lease)
- [seqnum](#seqnum)
- [service](#service)
- [state](#state)

## Command Details

### help

Usage: `srp server help`

Print SRP server help menu.

```bash
> srp server help
addrmode
auto
disable
domain
enable
help
host
lease
seqnum
service
state
Done
```

### addrmode

Usage: `srp server addrmode [unicast|anycast]`

Get or set the address mode used by the SRP server.

Address mode specifies how the address and port number are determined by the SRP server and this is published in the Thread Network Data.

Get the address mode.

```bash
> srp server addrmode
unicast
Done
```

Set the address mode.

```bash
> srp server addrmode anycast
Done

> srp server addrmode
anycast
Done
```

### auto

Usage: `srp server auto [enable|disable]`

Enables or disables the auto-enable mode on the SRP server.

When this mode is enabled, the Border Routing Manager controls if and when to enable or disable the SRP server.

This command requires that `OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE` be enabled.

```bash
> srp server auto enable
Done

> srp server auto
Enabled
Done
```

### disable

Usage: `srp server disable`

Disable the SRP server.

```bash
> srp server disable
Done
```

### domain

Usage: `srp server domain [domain-name]`

Get the domain.

```bash
> srp server domain
default.service.arpa.
Done
```

Set the domain.

```bash
> srp server domain thread.service.arpa.
Done
```

### enable

Usage: `srp server enable`

Enable the SRP server.

```bash
> srp server enable
Done
```

### host

Usage: `srp server host`

Print information of all registered hosts.

```bash
> srp server host
srp-api-test-1.default.service.arpa.
    deleted: false
    addresses: [fdde:ad00:beef:0:0:ff:fe00:fc10]
srp-api-test-0.default.service.arpa.
    deleted: false
    addresses: [fdde:ad00:beef:0:0:ff:fe00:fc10]
Done
```

### lease

Usage: `srp server lease [<min-lease>] [<max-lease>] [<min-key-lease>] [<max-key-lease>]`

Get LEASE and KEY-LEASE values.

```bash
> srp server lease
min lease: 1800
max lease: 7200
min key-lease: 86400
max key-lease: 1209600
Done
```

Set LEASE and KEY-LEASE values.

```bash
> srp server lease 1800 7200 86400 1209600
Done
```

### seqnum

Usage: `srp server seqnum [<seqnum>]`

Get or set the sequence number used with anycast address mode.

The sequence number is included in "DNS/SRP Service Anycast Address" entry published in the Network Data.

```bash
> srp server seqnum 20
Done

> srp server seqnum
20
Done
```

### service

Usage: `srp server service`

Print information of all registered services.

The TXT record is displayed as an array of entries. If an entry has a key, the key will be printed in ASCII format. The value portion will always be printed as hex bytes.

```bash
> srp server service
srp-api-test-1._ipps._tcp.default.service.arpa.
    deleted: false
    subtypes: (null)
    port: 49152
    priority: 0
    weight: 0
    ttl: 7200
    lease: 7200
    key-lease: 1209600
    TXT: [616263, xyz=585960]
    host: srp-api-test-1.default.service.arpa.
    addresses: [fdde:ad00:beef:0:0:ff:fe00:fc10]
srp-api-test-0._ipps._tcp.default.service.arpa.
    deleted: false
    subtypes: _sub1,_sub2
    port: 49152
    priority: 0
    weight: 0
    ttl: 3600
    lease: 3600
    key-lease: 1209600
    TXT: [616263, xyz=585960]
    host: srp-api-test-0.default.service.arpa.
    addresses: [fdde:ad00:beef:0:0:ff:fe00:fc10]
Done
```

### state

Usage: `srp server state`

Print the state of the SRP server. It could be `disabled`, `stopped` or `running`.

- disabled: The SRP server is not enabled.
- stopped: The SRP server is enabled but not active due to existing SRP servers already active in the Thread network. The SRP server may become active when existing SRP servers are no longer active within the Thread network.
- running: The SRP server is active and will handle service registrations.

```bash
> srp server state
running
Done
```
