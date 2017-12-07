# Feature: Jam Detection {#feature-jam-detect}

Jamming detection is a feature that allows the NCP to report when it
detects high levels of interference that are characteristic of intentional
signal jamming.

The presence of this feature can be detected by checking for the
presence of the `CAP_JAM_DETECT` (value 6) capability in `PROP_CAPS`.

## Properties

### PROP 4608: PROP_JAM_DETECT_ENABLE {#prop-jam-detect-enable}

* Type: Read-Write
* Packed-Encoding: `b`
* Default Value: false
* REQUIRED for `CAP_JAM_DETECT`

Octets: |       1
--------|-----------------
Fields: | `PROP_JAM_DETECT_ENABLE`

Indicates if jamming detection is enabled or disabled. Set to true
to enable jamming detection.

This property is only available if the `CAP_JAM_DETECT`
capability is present in `PROP_CAPS`.

### PROP 4609: PROP_JAM_DETECTED {#prop-jam-detected}

* Type: Read-Only
* Packed-Encoding: `b`
* REQUIRED for `CAP_JAM_DETECT`

Octets: |       1
--------|-----------------
Fields: | `PROP_JAM_DETECTED`

Set to true if radio jamming is detected. Set to false otherwise.

When jamming detection is enabled, changes to the value of this
property are emitted asynchronously via `CMD_PROP_VALUE_IS`.

This property is only available if the `CAP_JAM_DETECT`
capability is present in `PROP_CAPS`.

### PROP 4610: PROP_JAM_DETECT_RSSI_THRESHOLD

* Type: Read-Write
* Packed-Encoding: `c`
* Units: dBm
* Default Value: Implementation-specific
* RECOMMENDED for `CAP_JAM_DETECT`

This parameter describes the threshold RSSI level (measured in
dBm) above which the jamming detection will consider the
channel blocked.

### PROP 4611: PROP_JAM_DETECT_WINDOW

* Type: Read-Write
* Packed-Encoding: `c`
* Units: Seconds (1-64)
* Default Value: Implementation-specific
* RECOMMENDED for `CAP_JAM_DETECT`

This parameter describes the window period for signal jamming
detection.

### PROP 4612: PROP_JAM_DETECT_BUSY

* Type: Read-Write
* Packed-Encoding: `i`
* Units: Seconds (1-64)
* Default Value: Implementation-specific
* RECOMMENDED for `CAP_JAM_DETECT`

This parameter describes the number of aggregate seconds within
the detection window where the RSSI must be above
`PROP_JAM_DETECT_RSSI_THRESHOLD` to trigger detection.

The behavior of the jamming detection feature when `PROP_JAM_DETECT_BUSY`
is larger than `PROP_JAM_DETECT_WINDOW` is undefined.

### PROP 4613: PROP_JAM_DETECT_HISTORY_BITMAP

* Type: Read-Only
* Packed-Encoding: `X`
* Default Value: Implementation-specific
* RECOMMENDED for `CAP_JAM_DETECT`

This value provides information about current state of jamming detection
module for monitoring/debugging purpose. It returns a 64-bit value where
each bit corresponds to one second interval starting with bit 0 for the
most recent interval and bit 63 for the oldest intervals (63 sec earlier).
The bit is set to 1 if the jamming detection module observed/detected
high signal level during the corresponding one second interval.
