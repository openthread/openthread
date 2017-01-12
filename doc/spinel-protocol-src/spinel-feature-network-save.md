# Feature: Network Save

The network save/recall feature is an optional NCP capability that, when
present, allows the host to save and recall network credentials and
state to and from nonvolatile storage.

The presence of the save/recall feature can be detected by checking for
the presence of the `CAP_NET_SAVE` capability in `PROP_CAPS`.

Network clear feature allows host to erase all network credentials and
state from non-volatile memory.

## Commands

### CMD 9: (Host->NCP) CMD_NET_SAVE

Octets: |    1   |      1
--------|--------|--------------
Fields: | HEADER | CMD_NET_SAVE

Save network state command. Saves any current network credentials and
state necessary to reconnect to the current network to non-volatile
memory.

This operation affects non-volatile memory only. The current network
information stored in volatile memory is unaffected.

The response to this command is always a `CMD_PROP_VALUE_IS` for
`PROP_LAST_STATUS`, indicating the result of the operation.

This command is only available if the `CAP_NET_SAVE` capability is
set.

### CMD 10: (Host->NCP) CMD_NET_CLEAR

Octets: |    1   |      1
--------|--------|---------------
Fields: | HEADER | CMD_NET_CLEAR

Clear saved network settings command. Erases all network credentials
and state from non-volatile memory. The erased settings include any data
saved automatically by the network stack firmware and/or data saved by
`CMD_NET_SAVE` operation.

This operation affects non-volatile memory only. The current network
information stored in volatile memory is unaffected.

The response to this command is always a `CMD_PROP_VALUE_IS` for
`PROP_LAST_STATUS`, indicating the result of the operation.

This command is always available independent of the value of
`CAP_NET_SAVE` capability.


### CMD 11: (Host->NCP) CMD_NET_RECALL

Octets: |    1   |      1
--------|--------|----------------
Fields: | HEADER | CMD_NET_RECALL

Recall saved network state command. Recalls any previously saved
network credentials and state previously stored by `CMD_NET_SAVE` from
non-volatile memory.

This command will typically generated several unsolicited property
updates as the network state is loaded. At the conclusion of loading,
the authoritative response to this command is always a
`CMD_PROP_VALUE_IS` for `PROP_LAST_STATUS`, indicating the result of
the operation.

This command is only available if the `CAP_NET_SAVE` capability is
set.


