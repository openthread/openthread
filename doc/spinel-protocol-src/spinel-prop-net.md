## NET Properties {#prop-net}

### PROP 64: PROP_NET_SAVED {#prop-net-saved}
* Type: Read-Only
* Packed-Encoding: `b`

Returns true if there is a network state stored/saved.

### PROP 65: PROP_NET_IF_UP  {#prop-net-if-up}
* Type: Read-Write
* Packed-Encoding: `b`

Network interface up/down status. Non-zero (set to 1) indicates up,
zero indicates down.

### PROP 66: PROP_NET_STACK_UP  {#prop-net-stack-up}
* Type: Read-Write
* Packed-Encoding: `b`
* Unit: Enumeration

Thread stack operational status. Non-zero (set to 1) indicates up,
zero indicates down.

### PROP 67: PROP_NET_ROLE {#prop-net-role}
* Type: Read-Write
* Packed-Encoding: `C`
* Unit: Enumeration

Values:

* 0: `NET_ROLE_DETACHED`
* 1: `NET_ROLE_CHILD`
* 2: `NET_ROLE_ROUTER`
* 3: `NET_ROLE_LEADER`

### PROP 68: PROP_NET_NETWORK_NAME  {#prop-net-network-name}
* Type: Read-Write
* Packed-Encoding: `U`

### PROP 69: PROP_NET_XPANID   {#prop-net-xpanid}
* Type: Read-Write
* Packed-Encoding: `D`

### PROP 70: PROP_NET_MASTER_KEY   {#prop-net-master-key}
* Type: Read-Write
* Packed-Encoding: `D`

### PROP 71: PROP_NET_KEY_SEQUENCE_COUNTER   {#prop-net-key-sequence-counter}
* Type: Read-Write
* Packed-Encoding: `L`

### PROP 72: PROP_NET_PARTITION_ID   {#prop-net-partition-id}
* Type: Read-Write
* Packed-Encoding: `L`

The partition ID of the partition that this node is a member of.

### PROP 73: PROP_NET_REQUIRE_JOIN_EXISTING   {#prop-net-require-join-existing}
* Type: Read-Write
* Packed-Encoding: `b`

### PROP 74: PROP_NET_KEY_SWITCH_GUARDTIME   {#prop-net-key-swtich-guardtime}
* Type: Read-Write
* Packed-Encoding: `L`

### PROP 75: PROP_NET_PSKC   {#prop-net-pskc}
* Type: Read-Write
* Packed-Encoding: `D`


