# OpenThread Certification Tests

## Inspector

Inspect nodes status by the following modification:

1. Insert the inspector to where you want to inspect.

```python
import debug
debug.Inspector(self).inspect()
```

2. Run the test and it will stop at the line above and prompt `#`.

```sh
./script/test clean build cert tests/scripts/thread-cert/Cert_5_1_01_RouterAttach.py
```

3. Inspect

```sh
#
# 1
> state
leader
> exit
# 2
> panid
face
> exit
# exit
```

### CLI reference

#### `#` mode

This is selection mode. You may select the node to inspect here.

- `list` - list available nodes.
- `exit` - end inspecting, continue running test case.
- \<number\> - select the node with id \<number\>. This will result in entering `>` mode.

#### `>` mode

This is node mode. You may run OpenThread CLI here.

- `exit` - go back to `#` mode.
