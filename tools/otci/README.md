# OpenThread Control Interface

The OpenThread Control Interface (OTCI) is a library which provides uniform python interfaces to connect and control various kinds of devices running OpenThread.

## Supported device types

- OpenThread CLI
  - SOC device via Serial
- OpenThread NCP (limited support via [pyspinel](https://pypi.org/project/pyspinel/))
  - SOC device via Serial
- [OpenThread Border Router](https://github.com/openthread/ot-br-posix)
  - OTBR device via SSH

## Example

```python
import otci

# Connect to an OTBR device via SSH
node1 = otci.connect_otbr_ssh("192.168.1.101")

# Connect to an OpenThread CLI device via Serial
node2 = otci.connect_cli_serial("/dev/ttyACM0"))

# Start node1 to become Leader
node1.dataset_init_buffer()
node1.dataset_set_buffer(network_name='test', network_key='00112233445566778899aabbccddeeff', panid=0xface, channel=11)
node1.dataset_commit_buffer('active')

node1.ifconfig_up()
node1.thread_start()
node1.wait(5)
assert node1.get_state() == "leader"

# Start Commissioner on node1
node1.commissioner_start()
node1.wait(3)

node1.commissioner_add_joiner("TEST123",eui64='*')

# Start node2
node2.ifconfig_up()
node2.set_router_selection_jitter(1)

# Start Joiner on node2 to join the network
node2.joiner_start("TEST123")
node2.wait(10, expect_line="Join success")

# Wait for node 2 to become Router
node2.thread_start()
node2.wait(5)
assert node2.get_state() == "router"
```
