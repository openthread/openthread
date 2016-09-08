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
* Packed-Encoding: `A(T(6CLLC))`

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

