# OpenThread CLI - SRP Server

## Quick Start

See [README_SRP.md](README_SRP.md).

## Command List

- [help](#help)
- [disable](#disable)
- [domain](#domain)
- [enable](#enable)
- [host](#host)
- [lease](#lease)
- [service](#service)

## Command Details

### help

Usage: `srp server help`

Print SRP server help menu.

```bash
> srp server help
disable
domain
enable
help
host
lease
service
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

### service

Usage: `srp server service`

Print information of all registered services.

```bash
> srp server service
srp-api-test-1._ipps._tcp.default.service.arpa.
    deleted: false
    subtypes: (null)
    port: 49152
    priority: 0
    weight: 0
    TXT: 0130
    host: srp-api-test-1.default.service.arpa.
    addresses: [fdde:ad00:beef:0:0:ff:fe00:fc10]
srp-api-test-0._ipps._tcp.default.service.arpa.
    deleted: false
    subtypes: _sub1,_sub2
    port: 49152
    priority: 0
    weight: 0
    TXT: 0130
    host: srp-api-test-0.default.service.arpa.
    addresses: [fdde:ad00:beef:0:0:ff:fe00:fc10]
Done
```
