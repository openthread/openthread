# `toranj-ncp` test framework

`toranj-ncp` is a test framework for OpenThread enabling testing of the combined behavior of OpenThread (in NCP mode), spinel interface, and `wpantund` driver on linux.

`toranj` features:

- It is developed in Python.
- It can be used to simulate multiple nodes forming complex network topologies.
- It allows testing of network interactions between many nodes (IPv6 traffic exchanges).
- `toranj` in NCP mode runs `wpantund` natively with OpenThread in NCP mode on simulation platform (real-time).

## Setup

`toranj-ncp` requires `wpantund` to be installed.

- Please follow [`wpantund` installation guide](https://github.com/openthread/wpantund/blob/master/INSTALL.md#wpantund-installation-guide). Note that `toranj` expects `wpantund` installed from latest master branch.
- Alternative way to install `wpantund` is to use the same commands from git workflow [Simulation](https://github.com/openthread/openthread/blob/4b55284bd20f99a88e8e2c617ba358a0a5547f5d/.github/workflows/simulation.yml#L336-L341) for build target `toranj-test-framework`.

To run all tests, `start` script can be used. This script will build OpenThread with proper configuration options and starts running all test.

```bash
    cd tests/toranj/    # from OpenThread repo root
    TORANJ_CLI=0 ./start.sh
```

The `toranj-ncp` tests are included in `tests/toranj/ncp` folder. Each test-case has its own script following naming model `test-nnn-name.py` (e.g., `test-001-get-set.py`).

To run a specific test

```bash
    sudo python ncp/test-001-get-set.py
```

## `toranj` Components

`wpan` python module defines the `toranj` test components.

### `wpan.Node()` Class

`wpan.Node()` class creates a Thread node instance. It creates a sub-process to run `wpantund` and OpenThread, and provides methods to control the node.

```python
>>> import wpan
>>> node1 = wpan.Node()
>>> node1
Node (index=1, interface_name=wpan1)
>>> node2 = wpan.Node()
>>> node2
Node (index=2, interface_name=wpan2)
```

Note: You may need to run as `sudo` to allow `wpantund` to create tunnel interface (i.e., use `sudo python`).

### `wpan.Node` methods providing `wpanctl` commands

`wpan.Node()` provides methods matching all `wpanctl` commands.

- Get the value of a `wpantund` property, set the value, or add/remove value to/from a list based property:

```python
    node.get(prop_name)
    node.set(prop_name, value, binary_data=False)
    node.add(prop_name, value, binary_data=False)
    node.remove(prop_name, value, binary_data=False)
```

Example:

```python
>>> node.get(wpan.WPAN_NAME)
'"test-network"'
>>> node.set(wpan.WPAN_NAME, 'my-network')
>>> node.get(wpan.WPAN_NAME)
'"my-network"'
>>> node.set(wpan.WPAN_KEY, '65F2C35C7B543BAC1F3E26BB9F866C1D', binary_data=True)
>>> node.get(wpan.WPAN_KEY)
'[65F2C35C7B543BAC1F3E26BB9F866C1D]'
```

- Common network operations:

```python
    node.reset()            # Reset the NCP
    node.status()           # Get current status
    node.leave()            # Leave the current network, clear all persistent data

    # Form a network in given channel (if none given use a random one)
    node.form(name, channel=None)

    # Join a network with given info.
    # node_type can be JOIN_TYPE_ROUTER, JOIN_TYPE_END_DEVICE, JOIN_TYPE_SLEEPY_END_DEVICE
    node.join(name, channel=None, node_type=None, panid=None, xpanid=None)
```

Example:

```python
>>> result = node.status()
>>> print result
wpan1 => [
    "NCP:State" => "offline"
    "Daemon:Enabled" => true
    "NCP:Version" => "OPENTHREAD/20170716-00460-ga438cef0c-dirty; NONE; Feb 12 2018 11:47:01"
    "Daemon:Version" => "0.08.00d (0.07.01-191-g63265f7; Feb  2 2018 18:05:47)"
    "Config:NCP:DriverName" => "spinel"
    "NCP:HardwareAddress" => [18B4300000000001]
]
>>>
>>> node.form("test-network", channel=12)
'Forming WPAN "test-network" as node type "router"\nSuccessfully formed!'
>>>
>>> print node.status()
wpan1 => [
    "NCP:State" => "associated"
    "Daemon:Enabled" => true
    "NCP:Version" => "OPENTHREAD/20170716-00460-ga438cef0c-dirty; NONE; Feb 12 2018 11:47:01"
    "Daemon:Version" => "0.08.00d (0.07.01-191-g63265f7; Feb  2 2018 18:05:47)"
    "Config:NCP:DriverName" => "spinel"
    "NCP:HardwareAddress" => [18B4300000000001]
    "NCP:Channel" => 12
    "Network:NodeType" => "leader"
    "Network:Name" => "test-network"
    "Network:XPANID" => 0xA438CF5973FD86B2
    "Network:PANID" => 0x9D81
    "IPv6:MeshLocalAddress" => "fda4:38cf:5973:0:b899:3436:15c6:941d"
    "IPv6:MeshLocalPrefix" => "fda4:38cf:5973::/64"
]
```

- Scan:

```python
    node.active_scan(channel=None)
    node.energy_scan(channel=None)
    node.discover_scan(channel=None, joiner_only=False, enable_filtering=False, panid_filter=None)
    node.permit_join(duration_sec=None, port=None, udp=True, tcp=True)
```

- On-mesh prefixes and off-mesh routes:

```python
    node.config_gateway(prefix, default_route=False)
    node.add_route(route_prefix, prefix_len_in_bytes=None, priority=None)
    node.remove_route(route_prefix, prefix_len_in_bytes=None, priority=None)
```

A direct `wpanctl` command can be issued using `node.wpanctl(command)` with a given `command` string.

`wpan` module provides variables for different `wpantund` properties. Some commonly used are:

- Network/NCP properties: WPAN_STATE, WPAN_NAME, WPAN_PANID, WPAN_XPANID, WPAN_KEY, WPAN_CHANNEL, WPAN_HW_ADDRESS, WPAN_EXT_ADDRESS, WPAN_POLL_INTERVAL, WPAN_NODE_TYPE, WPAN_ROLE, WPAN_PARTITION_ID

- IPv6 Addresses: WPAN_IP6_LINK_LOCAL_ADDRESS, WPAN_IP6_MESH_LOCAL_ADDRESS, WPAN_IP6_MESH_LOCAL_PREFIX, WPAN_IP6_ALL_ADDRESSES, WPAN_IP6_MULTICAST_ADDRESSES

- Thread Properties: WPAN_THREAD_RLOC16, WPAN_THREAD_ROUTER_ID, WPAN_THREAD_LEADER_ADDRESS, WPAN_THREAD_LEADER_ROUTER_ID, WPAN_THREAD_LEADER_WEIGHT, WPAN_THREAD_LEADER_NETWORK_DATA,

        WPAN_THREAD_CHILD_TABLE, WPAN_THREAD_CHILD_TABLE_ADDRESSES, WPAN_THREAD_NEIGHBOR_TABLE,
        WPAN_THREAD_ROUTER_TABLE

Method `join_node()` can be used by a node to join another node:

```python
    # `node1` joining `node2`'s network as a router
    node1.join_node(node2, node_type=JOIN_TYPE_ROUTER)
```

Method `allowlist_node()` can be used to add a given node to the allowlist of the device and enables allowlisting:

```python
    # `node2` is added to the allowlist of `node1` and allowlisting is enabled on `node1`
    node1.allowlist_node(node2)
```

#### Example (simple 3-node topology)

Script below shows how to create a 3-node network topology with `node1` and `node2` being routers, and `node3` an end-device connected to `node2`:

```python
>>> import wpan
>>> node1 = wpan.Node()
>>> node2 = wpan.Node()
>>> node3 = wpan.Node()

>>> wpan.Node.init_all_nodes()

>>> node1.form("test-PAN")
'Forming WPAN "test-PAN" as node type "router"\nSuccessfully formed!'

>>> node1.allowlist_node(node2)
>>> node2.allowlist_node(node1)

>>> node2.join_node(node1, wpan.JOIN_TYPE_ROUTER)
'Joining "test-PAN" C474513CB487778D as node type "router"\nSuccessfully Joined!'

>>> node3.allowlist_node(node2)
>>> node2.allowlist_node(node3)

>>> node3.join_node(node2, wpan.JOIN_TYPE_END_DEVICE)
'Joining "test-PAN" C474513CB487778D as node type "end-device"\nSuccessfully Joined!'

>>> print node2.get(wpan.WPAN_THREAD_NEIGHBOR_TABLE)
[
    "EAC1672C3EAB30A4, RLOC16:9401, LQIn:3, AveRssi:-20, LastRssi:-20, Age:30, LinkFC:6, MleFC:0, IsChild:yes, RxOnIdle:yes, FTD:yes, SecDataReq:yes, FullNetData:yes"
    "A2042C8762576FD5, RLOC16:dc00, LQIn:3, AveRssi:-20, LastRssi:-20, Age:5, LinkFC:21, MleFC:18, IsChild:no, RxOnIdle:yes, FTD:yes, SecDataReq:no, FullNetData:yes"
]
>>> print node1.get(wpan.WPAN_THREAD_NEIGHBOR_TABLE)
[
    "960947C53415DAA1, RLOC16:9400, LQIn:3, AveRssi:-20, LastRssi:-20, Age:18, LinkFC:15, MleFC:11, IsChild:no, RxOnIdle:yes, FTD:yes, SecDataReq:no, FullNetData:yes"
]

```

### IPv6 Message Exchange

`toranj` allows a test-case to define traffic patterns (IPv6 message exchange) between different nodes. Message exchanges (tx/rx) are prepared and then an async rx/tx operation starts. The success and failure of tx/rx operations can then be verified by the test case.

`wpan.Node` method `prepare_tx()` prepares a UDP6 transmission from a node.

```python
    node1.prepare_tx(src, dst, data, count)
```

- `src` and `dst` can be

  - either a string containing an IPv6 address
  - or a tuple (ipv6 address as string, port). if no port is given, a random port number is used.

- `data` can be

  - either a string containing the message to be sent,
  - or an int indicating size of the message (a random message with the given length will be generated).

- `count` gives number of times the message will be sent (default is 1).

`prepare_tx` returns a `wpan.AsyncSender` object. The sender object can be used to check success/failure of tx operation.

`wpan.Node` method `prepare_rx()` prepares a node to listen for UDP messages from a sender.

```python
    node2.prepare_rx(sender)
```

- `sender` should be an `wpan.AsyncSender` object returned from previous `prepare_tx`.
- `prepare_rx()` returns a `wpan.AsyncReceiver` object to help test to check success/failure of rx operation.

After all exchanges are prepared, static method `perform_async_tx_rx()` should be used to start all previously prepared rx and tx operations.

```python
    wpan.Node.perform_async_tx_rx(timeout)
```

- `timeout` gives amount of time (in seconds) to wait for all operations to finish. (default is 20 seconds)

After `perform_async_tx_rx()` is done, the `AsyncSender` and `AsyncReceiver` objects can check if operations were successful (using property `was_successful`)

#### Example

Sending 10 messages containing `"Hello there!"` from `node1` to `node2` using their mesh-local addresses:

```python
# `node1` and `node2` are already joined and are part of the same Thread network.

# Get the mesh local addresses
>>> mladdr1 = node1.get(wpan.WPAN_IP6_MESH_LOCAL_ADDRESS)[1:-1]  # remove `"` from start/end of string
>>> mladdr2 = node2.get(wpan.WPAN_IP6_MESH_LOCAL_ADDRESS)[1:-1]

>>> print (mladdr1, mladdr2)
('fda4:38cf:5973:0:b899:3436:15c6:941d', 'fda4:38cf:5973:0:5836:fa55:7394:6d4b')

# prepare a `sender` and corresponding `recver`
>>> sender = node1.prepare_tx((mladdr1, 1234), (mladdr2, 2345), "Hello there!", 10)
>>> recver = node2.prepare_rx(sender)

# perform async message transfer
>>> wpan.Node.perform_async_tx_rx()

# check status of `sender` and `recver`
>>> sender.was_successful
True
>>> recver.was_successful
True

# `sender` or `recver` can provide info about the exchange

>>> sender.src_addr
'fda4:38cf:5973:0:b899:3436:15c6:941d'
>>> sender.src_port
1234
>>> sender.dst_addr
'fda4:38cf:5973:0:5836:fa55:7394:6d4b'
>>> sender.dst_port
2345
>>> sender.msg
'Hello there!'
>>> sender.count
10

# get all received msg by `recver` as list of tuples `(msg, (src_address, src_port))`
>>> recver.all_rx_msg
[('Hello there!', ('fda4:38cf:5973:0:b899:3436:15c6:941d', 1234)), ...  ]
```

### Logs and Verbose mode

Every `wpan.Node()` instance will save its corresponding `wpantund` logs. By default the logs are saved in a file `wpantun-log<node_index>.log`. By setting `wpan.Node__TUND_LOG_TO_FILE` to `False` the logs are written to `stdout` as the test-cases are executed.

When `start.sh` script is used to run all test-cases, if any test fails, to help with debugging of the issue, the last 30 lines of `wpantund` logs of every node involved in the test-case is dumped to `stdout`.

A `wpan.Node()` instance can also provide additional logs and info as the test-cases are run (verbose mode). It can be enabled for a node instance when it is created:

```python
    node = wpan.Node(verbose=True)     # `node` instance will provide extra logs.
```

Alternatively, `wpan.Node._VERBOSE` settings can be changed to enable verbose logging for all nodes. The default value of `wpan.Node._VERBOSE` is determined from environment variable `TORANJ_VERBOSE` (verbose mode is enabled when env variable is set to any of `1`, `True`, `Yes`, `Y`, `On` (case-insensitive)), otherwise it is disabled. When `TORANJ_VERBOSE` is enabled, the OpenThread logging is also enabled (and collected in `wpantund-log<node_index>.log`files) on all nodes.

Here is example of small test script and its corresponding log output with `verbose` mode enabled:

```python
node1 = wpan.Node(verbose=True)
node2 = wpan.Node(verbose=True)

wpan.Node.init_all_nodes()

node1.form("toranj-net")
node2.active_scan()

node2.join_node(node1)
verify(node2.get(wpan.WPAN_STATE) == wpan.STATE_ASSOCIATED)

lladdr1 = node1.get(wpan.WPAN_IP6_LINK_LOCAL_ADDRESS)[1:-1]
lladdr2 = node2.get(wpan.WPAN_IP6_LINK_LOCAL_ADDRESS)[1:-1]

sender = node1.prepare_tx(lladdr1, lladdr2, 20)
recver = node2.prepare_rx(sender)

wpan.Node.perform_async_tx_rx()

```

```
$ Node1.__init__() cmd: /usr/local/sbin/wpantund -o Config:NCP:SocketPath "system:../../examples/apps/ncp/ot-ncp-ftd 1" -o Config:TUN:InterfaceName wpan1 -o Config:NCP:DriverName spinel -o Daemon:SyslogMask "all -debug"
$ Node2.__init__() cmd: /usr/local/sbin/wpantund -o Config:NCP:SocketPath "system:../../examples/apps/ncp/ot-ncp-ftd 2" -o Config:TUN:InterfaceName wpan2 -o Config:NCP:DriverName spinel -o Daemon:SyslogMask "all -debug"
$ Node1.wpanctl('leave') -> 'Leaving current WPAN. . .'
$ Node2.wpanctl('leave') -> 'Leaving current WPAN. . .'
$ Node1.wpanctl('form "toranj-net"'):
     Forming WPAN "toranj-net" as node type "router"
     Successfully formed!
$ Node2.wpanctl('scan'):
        | PAN ID | Ch | XPanID           | HWAddr           | RSSI
     ---+--------+----+------------------+------------------+------
      1 | 0x9DEB | 16 | 8CC6CFC810F23E1B | BEECDAF3439DC931 |  -20
$ Node1.wpanctl('get -v NCP:State') -> '"associated"'
$ Node1.wpanctl('get -v Network:Name') -> '"toranj-net"'
$ Node1.wpanctl('get -v Network:PANID') -> '0x9DEB'
$ Node1.wpanctl('get -v Network:XPANID') -> '0x8CC6CFC810F23E1B'
$ Node1.wpanctl('get -v Network:Key') -> '[BA2733A5D81EAB8FFB3C9A7383CB6045]'
$ Node1.wpanctl('get -v NCP:Channel') -> '16'
$ Node2.wpanctl('set Network:Key -d -v BA2733A5D81EAB8FFB3C9A7383CB6045') -> ''
$ Node2.wpanctl('join "toranj-net" -c 16 -T r -p 0x9DEB -x 0x8CC6CFC810F23E1B'):
     Joining "toranj-net" 8CC6CFC810F23E1B as node type "router"
     Successfully Joined!
$ Node2.wpanctl('get -v NCP:State') -> '"associated"'
$ Node1.wpanctl('get -v IPv6:LinkLocalAddress') -> '"fe80::bcec:daf3:439d:c931"'
$ Node2.wpanctl('get -v IPv6:LinkLocalAddress') -> '"fe80::ec08:f348:646f:d37d"'
- Node1 sent 20 bytes (":YeQuNKjuOtd%H#ipM7P") to [fe80::ec08:f348:646f:d37d]:404 from [fe80::bcec:daf3:439d:c931]:12557
- Node2 received 20 bytes (":YeQuNKjuOtd%H#ipM7P") on port 404 from [fe80::bcec:daf3:439d:c931]:12557

```
