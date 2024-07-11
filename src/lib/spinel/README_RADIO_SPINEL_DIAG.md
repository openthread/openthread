# OpenThread Diagnostics - Radio Spinel diagnostic commands

This module provides Spinel based radio transceiver diagnostic commands.

`OPENTHREAD_CONFIG_DIAG_ENABLE` is required.

## Command List

- [buslatency](#buslatency)

## Command Details

### buslatency

Get the bus latency in microseconds between the host and the radio chip.

```bash
> diag radiospinel buslatency
0
Done
```

#### buslatency \<buslatency\>

Set the bus latency in microseconds between the host and the radio chip.

```bash
> diag radiospinel buslatency 1000
Done
```
