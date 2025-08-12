# OpenThread CLI - mDNS

The OpenThread mDNS APIs may be invoked via the OpenThread CLI. mDNS enables service discovery on the local network.

## Quick Start

Register a service instance and host:

```bash
> mdns register service test _test._udp test-host 1234
Service test for _test._udp
    host: test-host
    port: 1234
    priority: 0
    weight: 0
    ttl: 0
    txt-data: (empty)
Done

> mdns register host test-host fd00::aaaa fd00::bbbb 400
Host test-host
    2 address:
      fd00:0:0:0:0:0:0:aaaa
      fd00:0:0:0:0:0:0:bbbb
    ttl: 400
Done
```

Start a browser to discover the service registered earlier:

```bash
> mdns browser start _test._udp
Done
```

After a few moments, the service is discovered and the browse result in printed:

```bash
> mdns browse result for _test._udp
    instance: test
    ttl: 120
    if-index: 14
```

Start an SRV resolver for the service instance name:

```bash
> mdns srvresolver start test _test._udp
Done

mDNS SRV result for test for _test._udp
    host: test-host
    port: 1234
    priority: 0
    weight: 0
    ttl: 120
    if-index: 14

```

Start an IPv6 address resolver for the host:

```bash
> mdns ip6resolver start test-host
Done

> mDNS IPv6 address result for test-host
    2 address:
      fd00:0:0:0:0:0:0:aaaa ttl:400
      fd00:0:0:0:0:0:0:bbbb ttl:400
    if-index: 14
```

Change the registered host info, replacing an address:

```bash
> mdns register host test-host fd00::aaaa fd00::cccc 400
Host test-host
    2 address:
      fd00:0:0:0:0:0:0:aaaa
      fd00:0:0:0:0:0:0:cccc
    ttl: 400
Done

mDNS IPv6 address result for test-host
    2 address:
      fd00:0:0:0:0:0:0:aaaa ttl:400
      fd00:0:0:0:0:0:0:cccc ttl:400
    if-index: 14
```

## Command List

- [help](#help)
- [auto](#auto)
- [browser](#browser)
- [browsers](#browsers)
- [disable](#disable)
- [enable](#enable)
- [hosts](#hosts)
- [ip4resolver](#ip4resolver)
- [ip4resolvers](#ip4resolvers)
- [ip6resolver](#ip6resolver)
- [ip6resolvers](#ip6resolvers)
- [keys](#keys)
- [localhostaddrs](#localhostaddrs)
- [localhostname](#localhostname)
- [recordquerier](#recordquerier)
- [recordqueriers](#recordqueriers)
- [register](#register)
- [services](#services)
- [srvresolver](#srvresolver)
- [srvresolvers](#srvresolvers)
- [state](#state)
- [txtresolver](#txtresolver)
- [txtresolvers](#txtresolvers)
- [unicastquestion](#unicastquestion)
- [unregister](#unregister)
- [verboselogging](#verboselogging)

## Command Details

### help

List the mDNS CLI commands.

```bash
> mdns help
help
auto
browser
browsers
disable
enable
hosts
ip4resolver
...
Done
```

### auto

Usage: `mdns auto [enable|disable]`

Requires `OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE`.

Enables or disables the automatic start of the mDNS module by the Border Routing manager.

When the auto mode is enabled, the mDNS module uses the same infrastructure network interface as the Border Routing manager. The mDNS module is then automatically enabled or disabled based on the operational state of that interface. It is recommended to use the auto-enable mode on Border Routers. The default state of this mode at startup is controlled by the `OPENTHREAD_CONFIG_MULTICAST_DNS_AUTO_ENABLE_ON_INFRA_IF` configuration.

The auto-enable mode can be disabled by `mdns auto disable` or by fully disabling the mDNS module with `mdns disable`. Deactivating the auto-enable mode with `mdns auto disable` will not change the current operational state of the mDNS module (e.g., if it is currently enabled, it remains enabled).

```bash
> mdns auto enable
Done

> mdns auto
Enabled
Done
```

### browser

Usage: `mdns browser start|stop <service-type> [<sub-type>]`

Starts or stops a browser for a service type or sub-type. The discovered, changed, or removed service instances are reported.

- `start|stop`: Start or stop the browser.
- `<service-type>`: The service type to browse for (e.g., `_meshcop._udp`).
- `sub-type`: An optional service sub-type to filter results.

```bash
> mdns browser start _meshcop._udp
Done

mDNS browse result for _meshcop._udp
    instance: OpenThread BR c62b9b9475a0ec58
    ttl: 120
    if-index: 14


> mdns browser start _ot._udp _v
Done

mDNS browse result for _ot._udp sub-type _v
    instance: inst2
    ttl: 120
    if-index: 14
```

### browsers

Requires `OPENTHREAD_CONFIG_MULTICAST_DNS_ENTRY_ITERATION_API_ENABLE`.

Lists all currently active service browsers.

```bash
Browser _ot._udp for sub-type _v
    active: yes
    cached-results: yes
Browser _meshcop._udp
    active: yes
    cached-results: yes
Done
```

### disable

Disables the mDNS module.

```bash
> mdns disable
Done
```

### enable

Usage: `mdns enable [<if-index>]`

Enables the mDNS module.

- `<if-index>`: (Optional) The network interface index for mDNS to operate on. If not provided and Border Routing is enabled, it defaults to the infrastructure interface.

```bash
> mdns enable 14
Done
```

### hosts

Requires `OPENTHREAD_CONFIG_MULTICAST_DNS_ENTRY_ITERATION_API_ENABLE`.

Lists all hosts registered with the local mDNS module.

```bash
> mdns hosts
Host my-host
    1 address:
      fdde:ad00:beef:0:0:0:0:1
    ttl: 120
    state: registered
Done
```

### ip4resolver

Usage: `mdns ip4resolver start|stop <host-name>`

Starts or stops an IPv4 address resolver (A records) for a given host name.

```bash
> mdns ip4resolver start my-host
Done
```

### ip4resolvers

Requires `OPENTHREAD_CONFIG_MULTICAST_DNS_ENTRY_ITERATION_API_ENABLE`.

Lists all currently active IPv4 address resolvers.

```bash
> mdns ip4resolvers
IPv4 address resolver my-host
    active: yes
    cached-results: no
Done
```

### ip6resolver

Usage: `mdns ip6resolver start|stop <host-name>`

Starts or stops an IPv6 address resolver (AAAA records) for a given host name.

```bash
> mdns ip6resolver start my-host
Done

mDNS IPv6 address result for my-host
    1 address:
      fdde:ad00:beef:0:0:0:0:1 ttl:120
    if-index: 14
Done
```

### ip6resolvers

Requires `OPENTHREAD_CONFIG_MULTICAST_DNS_ENTRY_ITERATION_API_ENABLE`.

Lists all currently active IPv6 address resolvers.

```bash
> mdns ip6resolvers
IPv6 address resolver my-host
    active: yes
    cached-results: yes
IPv6 address resolver ot-host
    active: no
    cached-results: no
Done
```

### keys

Requires `OPENTHREAD_CONFIG_MULTICAST_DNS_ENTRY_ITERATION_API_ENABLE`.

Lists all keys registered with the local mDNS module.

```bash
> mdns keys
Key my-host (host)
    key-data: 0123456789abcdef0123456789abcdef
    ttl: 200
    state: registered
Key my-inst for _test._udp (service)
    key-data: 00112233445566778899aabbccddeeff
    ttl: 300
    state: registered
Done
```

### localhostaddrs

Requires `OPENTHREAD_CONFIG_MULTICAST_DNS_ENTRY_ITERATION_API_ENABLE`.

Lists all IP addresses of the local host that mDNS is aware of.

```bash
> mdns localhostaddrs
fdde:ad00:beef:0:1234:5678:9abc:def0
192.168.1.10
Done
```

### localhostname

Usage: `mdns localhostname [<name>]`

Gets or sets the local host name used by mDNS.

The local host name can be set only when the mDNS module is disabled. If not set the mDNS module itself will generate the local host name.

```bash
> mdns localhostname my-device
Done
> mdns localhostname
my-device
Done
```

### recordquerier

Usage: `mdns recordquerier start|stop <record-type> <first-label> [<next-labels>]`

Starts or stops a generic DNS record querier.

- `<record-type>`: The numerical value of the DNS record type (e.g., 25 for KEY).
- `<first-label>`: The first label of the name to query. May contain dot `.` character.
- `<next-labels>`: The remaining labels of the name to query.

The record querier can not be used for record types PTR, SRV, TXT, A, and AAAA. Otherwise, `Error 7: InvalidArgs` is returned. For these, browsers or resolvers can be used.

```bash
> mdns recordquerier start 25 my-host
Done

mDNS result for record 25 and name my-host
    data: 0123456789abcdef0123456789abcdef
    ttl: 200
    if-index: 14

> mdns recordquerier start 25 my-inst _test._udp
Done

mDNS result for record 25 and name my-inst _test._udp
    data: 00112233445566778899aabbccddeeff
    ttl: 300
    if-index: 14
```

### recordqueriers

Requires `OPENTHREAD_CONFIG_MULTICAST_DNS_ENTRY_ITERATION_API_ENABLE`.

Lists all currently active generic record queriers.

```bash
> mdns recordqueriers
Record querier for type 25 and name my-inst _test._udp
    active: yes
    cached-results: yes
Record querier for type 25 and name my-instance _ot-test._udp
    active: yes
    cached-results: no
Record querier for type 25 and name my-host
    active: yes
    cached-results: yes
Done
```

### register

Registers a host, service, or key with the mDNS module. The registration can be synchronous (default) or asynchronous.

When `async` is used, the registration is performed asynchronously. A request ID number is printed, and the outcome of the registration (whether successful or not) is reported later, referencing this request ID.

Usage: `mdns register [async] host <name> [<address> ...] [<ttl>]`

Register a host.

- `<name>`: The host name, e.g. `my-host`
- `<address>`: A list of one or more IPv6 addresses associated with the host
- `<ttl>`: The TTL value.

```bash
> mdns register host my-host fdde:ad00:beef:0::1
Host my-host
    1 address:
      fdde:ad00:beef:0:0:0:0:1
    ttl: 0
Done
```

Usage: `mdns register [async] service <instance> <service-type>[,<sub-type>...] <host> <port> [<prio>] [<weight>] [<ttl>] [<txt>]`

Registers a service.

- `<instance>`: The service instance label (e.g., `my-instance`). This can include the `.` character.
- `<service-type>`: The service type (e.g., `_trel._udp`).
- `<sub-type>`: Zero or more sub-type labels. Sub-types should immediately follow the `<service-type>`, separated by a comma (`,`).
- `<host>`: The host name for this service.
- `<port>`: The service port number.
- `<prio>`: The service priority value.
- `<weight>`: The service weight value.
- `<ttl>`: The Time to Live (TTL) value for the records.
- `<txt>`: The TXT data, provided as a hexadecimal string.

```bash
> mdns register service my-inst _test._udp my-host 1234 0 0 120 010203
Service my-inst for _test._udp
    host: my-host
    port: 1234
    priority: 0
    weight: 0
    ttl: 120
    txt-data: 010203
Done

> mdns register service inst2 _ot._udp,_s1,_v ot-host 2367 1 2 120
Service inst2 for _ot._udp
    host: ot-host
    2 sub-type:
        _s1
        _v
    port: 2367
    priority: 1
    weight: 2
    ttl: 120
    txt-data: (empty)
Done
```

Usage: `mdns register [async] key <name> [<service-type>] <key-data> [<ttl>]`

Register a key record associated with a host and service instance name.

- `<name>`: The host name or instance label.
- `<service-type>`: The service type.
- `<key-data>`: Key record data as hex string.
- `<ttl>`: The key TTL.

```bash
> mdns register key my-host 0123456789abcdef0123456789abcdef 200
Key my-host (host)
    key-data: 0123456789abcdef0123456789abcdef
    ttl: 200
Done

> mdns register key my-inst _test._udp 00112233445566778899aabbccddeeff 300
Key my-inst for _test._udp (service)
    key-data: 00112233445566778899aabbccddeeff
    ttl: 300
Done
```

### services

Requires `OPENTHREAD_CONFIG_MULTICAST_DNS_ENTRY_ITERATION_API_ENABLE`.

Lists all services registered with the local mDNS module.

```bash
mdns services
Service inst2 for _ot._udp
    host: ot-host
    2 sub-type:
        _v
        _s1
    port: 2367
    priority: 1
    weight: 2
    ttl: 120
    txt-data: 00
    state: registered
Service my-inst for _test._udp
    host: my-host
    port: 1234
    priority: 0
    weight: 0
    ttl: 120
    txt-data: 010203
    state: registered
Service OpenThread BR c62b9b9475a0ec58 for _meshcop._udp
    host: otc62b9b9475a0ec58
    port: 49152
    priority: 0
    weight: 0
    ttl: 120
    txt-data: 1369643dc84888bc96ba8349e5ee6a5d710978180472763d310d6e6e3d4f70656e5468726561640b78703ddead00beef00cafe0874763d312e342e300b78613dc62b9b9475a0ec580773623d0000082010646e3d44656661756c74446f6d61696e
    state: registered
Done

```

### srvresolver

Usage : `mdns srvresolver start|stop <service-instance> <service-type>`

Starts or stops an SRV record resolver for a specific service instance name.

```bash
> mdns srvresolver start my-inst _test._udp
Done

mDNS SRV result for my-inst for _test._udp
    host: my-host
    port: 1234
    priority: 0
    weight: 0
    ttl: 120
    if-index: 14
```

### srvresolvers

Requires `OPENTHREAD_CONFIG_MULTICAST_DNS_ENTRY_ITERATION_API_ENABLE`.

Lists all currently active SRV resolvers.

```bash
> mdns srvresolvers
SRV resolver my-inst for _test._udp
    active: yes
    cached-results: yes
Done
```

### state

Shows the current operational state of the mDNS module.

```bash
> mdns state
Enabled
Done
```

### txtresolver

Usage: `mdns txtresolver start|stop <service-instance> <service-type>`

Starts or stops a TXT record resolver for a specific service instance name.

```bash
> mdns txtresolver start my-inst _test._udp
Done

mDNS TXT result for my-inst for _test._udp
    txt-data: 010203
    ttl: 120
    if-index: 14
```

### txtresolvers

Requires `OPENTHREAD_CONFIG_MULTICAST_DNS_ENTRY_ITERATION_API_ENABLE`.

Lists all currently active TXT resolvers.

```bash
> mdns txtresolvers
TXT resolver my-inst for _test._udp
    active: yes
    cached-results: yes
Done
```

### unicastquestion

Usage: `mdns unicastquestion [enable|disable]`

Gets or sets whether the mDNS module is allowed to send questions requesting unicast responses referred to as "QU" questions.

The "QU" questions request unicast responses, in contrast to "QM" questions which request multicast responses.

When allowed, the first probe will be sent as a "QU" question. This command can be used to address platform limitation where platform socket cannot accept unicast response received on mDNS port (due to it being already bound).

```bash
> mdns unicastquestion enable
Done
> mdns unicastquestion
Enabled
Done
```

### unregister

Unregisters a previously registered host, service, or key.

Usage: `mdns unregister host <name>`

Unregisters a host.

- `<name>`: The host name to unregister.

```bash
> mdns unregister host my-host
Done
```

Usage: `mdns unregister service <instance> <service-type>`

Unregisters a service instance.

- `<instance>`: The service instance label.
- `<service-type>`: The service type.

```bash
> mdns unregister service my-instance _ot-test._udp
Done
```

Usage: `mdns unregister key <name> [<service-type>]`

Unregisters a key for host or service instance name.

- `<name>`: The host name or instance label.
- `<service-type>`: The service type.

```bash
> mdns unregister key my-host
Done

> mdns unregister key my-inst _test._udp
Done
```

### verboselogging

Usage: `mdns verboselogging [enable|disable]`

Requires `OPENTHREAD_CONFIG_MULTICAST_DNS_VERBOSE_LOGGING_ENABLE`.

Enables or disables verbose logging for the mDNS module at run-time.

When enabled, the mDNS module emits verbose logs for every sent or received mDNS message, including the header and all question and resource records. These logs are generated regardless of the current log level configured on the device.

This feature can generate a large volume of logs, so its use is recommended only during development, integration, or debugging.

The initial state of verbose logging (enabled or disabled at startup) is determined by the configuration `OPENTHREAD_CONFIG_MULTICAST_DEFAULT_DNS_VERBOSE_LOGGING_STATE`.

```bash
> mdns verboselogging enable
Done

> mdns verboselogging
Enabled
Done
```
