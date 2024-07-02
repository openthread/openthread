# OpenThread Diagnostics - RCP Capability Diagnostics Example

This module provides diag commands for checking RCP capabilities.

`OPENTHREAD_CONFIG_DIAG_ENABLE` and `OPENTHREAD_POSIX_CONFIG_RCP_CAPS_DIAG_ENABLE` are required.

## Command List

- [capflags](#capflags)
- [spinel](#spinel)
- [spinelspeed](#spinelspeed)
- [srcmatchtable](#srcmatchtable)

## Command Details

### capflags

Check RCP's radio and spinel capbility flags.

```bash
> diag rcpcaps capflags

Radio Capbility Flags :

Thread Version >= 1.1 :
RADIO_CAPS_ACK_TIMEOUT ------------------------------------ OK
RADIO_CAPS_TRANSMIT_RETRIES ------------------------------- OK
RADIO_CAPS_CSMA_BACKOFF ----------------------------------- OK

Thread Version >= 1.2 :
RADIO_CAPS_TRANSMIT_SEC ----------------------------------- OK
RADIO_CAPS_TRANSMIT_TIMING -------------------------------- OK

Utils :
RADIO_CAPS_ENERGY_SCAN ------------------------------------ OK
RADIO_CAPS_SLEEP_TO_TX ------------------------------------ NotSupported
RADIO_CAPS_RECEIVE_TIMING --------------------------------- NotSupported
RADIO_CAPS_RX_ON_WHEN_IDLE -------------------------------- NotSupported

Spinel Capbility Flags :

Basic :
SPINEL_CAPS_CONFIG_RADIO ---------------------------------- OK
SPINEL_CAPS_MAC_RAW --------------------------------------- OK
SPINEL_CAPS_RCP_API_VERSION ------------------------------- OK

Utils :
SPINEL_CAPS_OPENTHREAD_LOG_METADATA ----------------------- NotSupported
SPINEL_CAPS_RCP_MIN_HOST_API_VERSION ---------------------- OK
SPINEL_CAPS_RCP_RESET_TO_BOOTLOADER ----------------------- NotSupported
Done
```

### spinel

Check which Spinel commands RCP supports.

```bash
> diag rcpcaps spinel

Basic :
PROP_VALUE_GET CAPS --------------------------------------- OK
PROP_VALUE_GET PROTOCOL_VERSION --------------------------- OK
PROP_VALUE_GET RADIO_CAPS --------------------------------- OK
PROP_VALUE_GET RCP_API_VERSION ---------------------------- OK
PROP_VALUE_GET NCP_VERSION -------------------------------- OK

Thread Version >= 1.1 :
PROP_VALUE_SET PHY_CHAN ----------------------------------- OK
PROP_VALUE_SET PHY_ENABLED -------------------------------- OK
PROP_VALUE_SET MAC_15_4_PANID ----------------------------- OK
PROP_VALUE_SET MAC_15_4_LADDR ----------------------------- OK
PROP_VALUE_SET MAC_15_4_SADDR ----------------------------- OK
PROP_VALUE_SET MAC_RAW_STREAM_ENABLED --------------------- OK
PROP_VALUE_SET MAC_SCAN_MASK ------------------------------ OK
PROP_VALUE_SET MAC_SCAN_PERIOD ---------------------------- OK
PROP_VALUE_SET MAC_SCAN_STATE ----------------------------- OK
PROP_VALUE_SET MAC_SRC_MATCH_ENABLED ---------------------- OK
PROP_VALUE_SET MAC_SRC_MATCH_SHORT_ADDRESSES -------------- OK
PROP_VALUE_SET MAC_SRC_MATCH_EXTENDED_ADDRESSES ----------- OK
PROP_VALUE_GET HWADDR ------------------------------------- OK
PROP_VALUE_GET PHY_CHAN_PREFERRED ------------------------- OK
PROP_VALUE_GET PHY_CHAN_SUPPORTED ------------------------- OK
PROP_VALUE_GET PHY_RSSI ----------------------------------- OK
PROP_VALUE_GET PHY_RX_SENSITIVITY ------------------------- OK
PROP_VALUE_INSERT MAC_SRC_MATCH_SHORT_ADDRESSES ----------- OK
PROP_VALUE_INSERT MAC_SRC_MATCH_EXTENDED_ADDRESSES -------- OK
PROP_VALUE_REMOVE MAC_SRC_MATCH_SHORT_ADDRESSES ----------- OK
PROP_VALUE_REMOVE MAC_SRC_MATCH_EXTENDED_ADDRESSES -------- OK

Thread Version >= 1.2 :
PROP_VALUE_SET ENH_ACK_PROBING ---------------------------- NotImplemented
PROP_VALUE_SET RCP_MAC_FRAME_COUNTER ---------------------- OK
PROP_VALUE_SET RCP_MAC_KEY -------------------------------- OK
PROP_VALUE_GET CSL_ACCURACY ------------------------------- OK
PROP_VALUE_GET CSL_UNCERTAINTY ---------------------------- OK
PROP_VALUE_GET TIMESTAMP ---------------------------------- OK

Utils :
PROP_VALUE_SET MAC_PROMISCUOUS_MODE ----------------------- OK
PROP_VALUE_GET PHY_CCA_THRESHOLD -------------------------- OK
PROP_VALUE_GET PHY_FEM_LNA_GAIN --------------------------- OK
PROP_VALUE_GET PHY_REGION_CODE ---------------------------- OK
PROP_VALUE_GET PHY_TX_POWER ------------------------------- OK
PROP_VALUE_GET RADIO_COEX_ENABLE -------------------------- OK
PROP_VALUE_GET RADIO_COEX_METRICS ------------------------- OK
PROP_VALUE_GET RCP_MIN_HOST_API_VERSION ------------------- OK
PROP_VALUE_SET PHY_CCA_THRESHOLD -------------------------- OK
PROP_VALUE_SET PHY_CHAN_MAX_POWER ------------------------- OK
PROP_VALUE_SET PHY_CHAN_TARGET_POWER ---------------------- OK
PROP_VALUE_SET PHY_FEM_LNA_GAIN --------------------------- OK
PROP_VALUE_SET PHY_REGION_CODE ---------------------------- OK
PROP_VALUE_SET PHY_TX_POWER ------------------------------- OK
PROP_VALUE_SET RADIO_COEX_ENABLE -------------------------- OK
Done
```

### spinelspeed

Check the speed of Spinel interface.

```bash
> diag rcpcaps spinelspeed
SpinelSpeed ----------------------------------------------- 34414843 bps
Done
```

### srcmatchtable

Check the source match table size supported by the RCP.

```bash
> diag rcpcaps srcmatchtable
ShortSrcMatchTableSize ------------------------------------ 128
ExtendedSrcMatchTableSize --------------------------------- 128
Done
```
