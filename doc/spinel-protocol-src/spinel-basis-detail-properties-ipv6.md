## IPv6 Properties {#prop-ipv6}

### PROP 96: PROP_IPV6_LL_ADDR {#prop-ipv6-ll-addr}
* Type: Read-Only
* Packed-Encoding: `6`

The IPv6 link-local scope address.

### PROP 99: PROP_IPV6_ADDRESS_TABLE {#prop-ipv6-address-table}
* Type: Read-Write
* Packed-Encoding: `A(t(6CLLC))`

Array of structures containing:

* `6`: IPv6 Address
* `C`: Network Prefix Length
* `L`: Valid Lifetime
* `L`: Preferred Lifetime
* `C`: Flags

EDITOR: this conflates the IPv6 interface address list with the IPv6 on-link prefix used in IPv6 Neighbor Discovery and other address reservation and resolution protocols with similar function, e.g. Thread(R). It probably makes sense to create an additional set of properties that represent neighbor discovery and router discovery parameters.

EDITOR: the operational semantics of the Flags field is not well-specified.

### PROP 101: PROP_IPv6_ICMP_PING_OFFLOAD
* Type: Read-Write
* Packed-Encoding: `b`

Allow the NCP to directly respond to ICMP ping requests. If this is turned on, ICMP echo request packets will not be passed to the OS.

Default value is `false`.

