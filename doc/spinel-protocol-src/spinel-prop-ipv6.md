## IPv6 Properties {#prop-ipv6}

### PROP 96: PROP_IPV6_LL_ADDR {#prop-ipv6-ll-addr}
* Type: Read-Only
* Packed-Encoding: `6`

IPv6 Address

### PROP 97: PROP_IPV6_ML_ADDR {#prop-ipv6-ml-addr}
* Type: Read-Only
* Packed-Encoding: `6`

IPv6 Address + Prefix Length

### PROP 98: PROP_IPV6_ML_PREFIX {#prop-ipv6-ml-prefix}
* Type: Read-Write
* Packed-Encoding: `6C`

IPv6 Prefix + Prefix Length

### PROP 99: PROP_IPV6_ADDRESS_TABLE {#prop-ipv6-address-table}
* Type: Read-Write
* Packed-Encoding: `A(t(6CLLC))`

This property provides all unicast addresses.
Array of structures containing:

* `6`: IPv6 Address
* `C`: Network Prefix Length
* `L`: Valid Lifetime
* `L`: Preferred Lifetime
* `C`: Flags

### PROP 101: PROP_IPv6_ICMP_PING_OFFLOAD
* Type: Read-Write
* Packed-Encoding: `b`

Allow the NCP to directly respond to ICMP ping requests. If this is
turned on, ping request ICMP packets will not be passed to the host.

Default value is `false`.

### PROP 102: SPINEL_PROP_IPV6_MULTICAST_ADDRESS_TABLE {#prop-ipv6-multicast-address-table}
* Type: Read-Write
* Packed-Encoding: `A(t(6))`

Array of structures containing:

* `6`: Multicast IPv6 Address

### PROP 103: PROP_IPv6_ICMP_PING_OFFLOAD_MODE
* Type: Read-Write
* Packed-Encoding: `C`
* Unit: Enumeration

Allow the NCP to directly respond to ICMP ping requests. If this is
turned on, ping request ICMP packets will not be passed to the host.

This property allows enabling responses sent to unicast only, multicast
only, or both.

Values:

* 0: `IPV6_ICMP_PING_OFFLOAD_DISABLED`
* 1: `IPV6_ICMP_PING_OFFLOAD_UNICAST_ONLY`
* 2: `IPV6_ICMP_PING_OFFLOAD_MULTICAST_ONLY`
* 3: `IPV6_ICMP_PING_OFFLOAD_ALL`

Default value is `IPV6_ICMP_PING_OFFLOAD_DISABLED`.
