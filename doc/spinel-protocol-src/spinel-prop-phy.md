## PHY Properties {#prop-phy}


### PROP 32: PROP_PHY_ENABLED {#prop-phy-enabled}
* Type: Read-Write
* Packed-Encoding: `b` (bool8)

Set to 1 if the PHY is enabled, set to 0 otherwise.
May be directly enabled to bypass higher-level packet processing
in order to implement things like packet sniffers. This property
can only be written if the `SPINEL_CAP_MAC_RAW` capability is present.

### PROP 33: PROP_PHY_CHAN {#prop-phy-chan}
* Type: Read-Write
* Packed-Encoding: `C` (uint8)

Value is the current channel. Must be set to one of the
values contained in `PROP_PHY_CHAN_SUPPORTED`.

### PROP 34: PROP_PHY_CHAN_SUPPORTED {#prop-phy-chan-supported}
* Type: Read-Only
* Packed-Encoding: `A(C)` (array of uint8)
* Unit: List of channels

Value is a list of channel values that are supported by the
hardware.

### PROP 35: PROP_PHY_FREQ {#prop-phy-freq}
* Type: Read-Only
* Packed-Encoding: `L` (uint32)
* Unit: Kilohertz

Value is the radio frequency (in kilohertz) of the
current channel.

### PROP 36: PROP_PHY_CCA_THRESHOLD {#prop-phy-cca-threshold}
* Type: Read-Write
* Packed-Encoding: `c` (int8)
* Unit: dBm

Value is the CCA (clear-channel assessment) threshold. Set to
-128 to disable.

When setting, the value will be rounded down to a value
that is supported by the underlying radio hardware.

### PROP 37: PROP_PHY_TX_POWER {#prop-phy-tx-power}
* Type: Read-Write
* Packed-Encoding: `c` (int8)
* Unit: dBm

Value is the transmit power of the radio.

When setting, the value will be rounded down to a value
that is supported by the underlying radio hardware.

### PROP 38: PROP_PHY_RSSI {#prop-phy-rssi}
* Type: Read-Only
* Packed-Encoding: `c` (int8)
* Unit: dBm

Value is the current RSSI (Received signal strength indication)
from the radio. This value can be used in energy scans and for
determining the ambient noise floor for the operating environment.

### PROP 39: PROP_PHY_RX_SENSITIVITY {#prop-phy-rx-sensitivity}
* Type: Read-Only
* Packed-Encoding: `c` (int8)
* Unit: dBm

Value is the radio receive sensitivity. This value can be used as
lower bound noise floor for link metrics computation.
