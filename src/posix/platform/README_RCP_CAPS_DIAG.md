# OpenThread Diagnostics - RCP Capability Diagnostics Example

This module provides diag commands for checking RCP capabilities.

`OPENTHREAD_CONFIG_DIAG_ENABLE` and `OPENTHREAD_POSIX_CONFIG_RCP_CAPS_DIAG_ENABLE` are required.

## Command List

- [diagcmd](#diagcmd)
- [spinel](#spinel)

## Command Details

### diagcmd

Check which diag commands RCP supports.

```bash
> diag rcpcaps diagcmd

Diag Commands :
diag channel 20 ------------------------------------------- OK
diag cw start --------------------------------------------- OK
diag cw stop ---------------------------------------------- OK
diag power 10 --------------------------------------------- OK
diag echo 112233 ------------------------------------------ OK
diag echo -n 200 ------------------------------------------ OK
diag stream start ----------------------------------------- OK
diag stream stop ------------------------------------------ OK
diag rawpowersetting enable ------------------------------- OK
diag rawpowersetting 112233 ------------------------------- OK
diag rawpowersetting -------------------------------------- OK
diag powersettings 20 ------------------------------------- NotFound
diag rawpowersetting disable ------------------------------ OK
Done
```

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
