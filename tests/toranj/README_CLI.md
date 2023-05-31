# `toranj-cli`

`toranj-cli` is a test framework for OpenThread using its CLI interface.

`toranj` features:

- It is developed in Python.
- It can be used to simulate multiple nodes forming complex network topologies.
- It allows testing of network interactions between many nodes.
- `toranj` in CLI mode runs `ot-cli-ftd` on simulation platform (real-time).

## Setup

To build OpenThread with `toranj` configuration, the `test/toranj/build.sh` script can be used:

```bash
$ ./tests/toranj/build.sh all
====================================================================================================
Building OpenThread (NCP/CLI for FTD/MTD/RCP mode) with simulation platform using cmake
====================================================================================================
-- OpenThread Source Directory: /Users/abtink/GitHub/openthread
-- OpenThread CMake build type: Debug
-- Package Name: OPENTHREAD
...

```

The `toranj-cli` tests are included in `tests/toranj/cli` folder. Each test-case has its own script following naming model `test-nnn-name.py` (e.g., `test-001-get-set.py`).

To run a specific test:

```bash
$ cd tests/toranj/cli
$ python3 test-001-get-set.py
```

To run all CLI tests, `start` script can be used. This script will build OpenThread with proper configuration options and starts running all tests.

```bash
# From OpenThread repo root folder
$ top_builddir=($pwd) TORANJ_CLI=1 ./tests/toranj/start.sh
```

## `toranj-cli` Components

`cli` python module defines the `toranj-cli` test components.

### `cli.Node()` Class

`cli.Node()` class creates a Thread node instance. It creates a sub-process to run `ot-cli-ftd` and provides methods to control the node and issue CLI commands.

```python
>>> import cli
>>> node1 = cli.Node()
>>> node1
Node (index=1)
>>> node2 = cli.Node()
>>> node2
Node (index=2)
```

Note: You may need to run as `sudo` to allow log file to be written (i.e., use `sudo python` or `sudo python3`).

### `cli.Node` methods

`cli.Node()` provides methods matching different CLI commands, in addition to some helper methods for common operations.

Example:

```python
>>> node.get_state()
'disabled'
>>> node.get_channel()
'11'
>>> node.set_channel(12)
>>> node.get_channel()
'12'
>>> node.set_network_key('11223344556677889900aabbccddeeff')
>>> node.get_network_key()
'11223344556677889900aabbccddeeff'
```

Common network operations:

```python
    # Form a Thread network with all the given parameters.
    node.form(network_name=None, network_key=None, channel=None, panid=0x1234, xpanid=None):

    # Try to join an existing network as specified by `another_node`.
    # `type` can be `JOIN_TYPE_ROUTER`, `JOIN_TYPE_END_DEVICE, or `JOIN_TYPE_SLEEPY_END_DEVICE`
    node.join(another_node, type=JOIN_TYPE_ROUTER):
```

A direct CLI command can be issued using `node.cli(command)` with a given `command` string.

```python
>>> node.cli('uptime')
['00:36:18.778']
```

Method `allowlist_node()` can be used to add a given node to the allowlist of the device and enables allowlisting:

```python
    # `node2` is added to the allowlist of `node1` and allowlisting is enabled on `node1`
    node1.allowlist_node(node2)
```

#### Example (simple 3-node topology)

Script below shows how to create a 3-node network topology with `node1` and `node2` being routers, and `node3` an end-device connected to `node2`:

```python
>>> import cli
>>> node1 = cli.Node()
>>> node2 = cli.Node()
>>> node3 = cli.Node()

>>> node1.form('test')
>>> node1.get_state()
'leader'

>>> node1.allowlist_node(node2)
>>> node1.allowlist_node(node3)

>>> node2.join(node1, cli.JOIN_TYPE_ROUTER)
>>> node2.get_state()
'router'

>>> node3.join(node1, cli.JOIN_TYPE_END_DEVICE)
>>> node3.get_state()
'child'

>>> node1.cli('neighbor list')
['0x1c01 0x0400 ']
```

### Logs and Verbose mode

Every `cli.Node()` instance will save its corresponding logs. By default the logs are saved in a file `ot-logs<node_index>.log`.

When `start.sh` script is used to run all test-cases, if any test fails, to help with debugging of the issue, the last 30 lines of logs of every node involved in the test-case are dumped to `stdout`.

A `cli.Node()` instance can also provide additional logs and info as the test-cases are run (verbose mode). It can be enabled for a node instance when it is created:

```python
>>> import cli
>>> node = cli.Node(verbose=True)
$ Node1.__init__() cmd: `../../../examples/apps/cli/ot-cli-ftd --time-speed=1 1`

>>> node.get_state()
$ Node1.cli('state') -> disabled
'disabled'

>>> node.form('test')
$ Node1.cli('networkname test')
$ Node1.cli('panid 4660')
$ Node1.cli('ifconfig up')
$ Node1.cli('thread start')
$ Node1.cli('state') -> detached
$ Node1.cli('state') -> detached
...
$ Node1.cli('state') -> leader
```

Alternatively, `cli.Node._VERBOSE` settings can be changed to enable verbose logging for all nodes. The default value of `cli.Node._VERBOSE` is determined from environment variable `TORANJ_VERBOSE` (verbose mode is enabled when env variable is set to any of `1`, `True`, `Yes`, `Y`, `On` (case-insensitive)), otherwise it is disabled.

## `toranj-cli` and `thread-cert` test framework

`toranj-cli` uses CLI commands to test the behavior of OpenThread with simulation platform. `thread-cert` scripts (in `tests/scripts/thread-cert`) also use CLI commands. However, these two test frameworks have certain differences and are intended for different situations. The `toranj` test cases run in real-time (though it is possible to run with a time speed-up factor) while the `thread-cert` scripts use virtual-time and event-based simulation model.

- `toranj` test cases are useful to validate the real-time (non event-based) simulation platform implementation itself.
- `toranj` test cases can be used in situations where the platform layer may not support event-based model.
- `toranj` frameworks allows for more interactive testing (e.g., read–eval–print loop (REPL) model in python) and do not need a separate process to run to handle/dispatch events (which is required for the virtual-time simulation model).
- `thread-cert` test cases can run quickly (due to virtual time emulation), but the test script itself needs to manage the flow and advancement of time.
