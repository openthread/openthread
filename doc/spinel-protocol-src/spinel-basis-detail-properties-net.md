## NET Properties {#prop-net}

### PROP 64: PROP_NET_SAVED {#prop-net-saved}
* Type: Read-Only
* Packed-Encoding: `b`

Returns true if there is a network state stored/saved.

### PROP 65: PROP_NET_IF_UP  {#prop-net-if-up}
* Type: Read-Write
* Packed-Encoding: `b`

Network interface up/down status. Non-zero (set to 1) indicates up, zero indicates down.

### PROP 66: PROP_NET_STACK_UP  {#prop-net-stack-up}
* Type: Read-Write
* Packed-Encoding: `b`
* Unit: Enumeration

Network protocol stack operational status. Non-zero (set to 1) indicates up, zero indicates down.

EDITOR: the examples show that the order of operations to bring up a network interface is first set PROP_NET_IF_UP=TRUE, then PROP_NET_STACK_UP=TRUE. What does it mean when PROP_NET_IF_UP=TRUE and PROP_NET_STACK_UP=FALSE? Does the NLI matter in operations with the PROP_NET_STACK_UP property?