# OpenThread Diagnostics - RCP Capability Diagnostics Example

This module provides diag commands for checking RCP capabilities.

`OPENTHREAD_CONFIG_DIAG_ENABLE` and `OPENTHREAD_POSIX_CONFIG_RCP_CAPS_DIAG_ENABLE` are required.

## Command List

- [spinel](#spinel)

## Command Details

### spinel

Check which Spinel commands RCP supports.

```bash
> diag rcpcaps spinel

Basic :
PROP_VALUE_GET CAPS --------------------------------------- OK

Thread Version >= 1.1 :
PROP_VALUE_SET PHY_CHAN ----------------------------------- OK

Thread Version >= 1.2 :
PROP_VALUE_SET ENH_ACK_PROBING ---------------------------- NotImplemented

Optional :
PROP_VALUE_GET PHY_CCA_THRESHOLD -------------------------- OK
Done
```
