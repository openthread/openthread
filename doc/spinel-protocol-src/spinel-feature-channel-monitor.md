# Feature: Channel Monitoring {#feature-channel-monitor}

Channel monitoring is a feature that allows the NCP to periodically
monitor all channels to help determine the cleaner channels (channels
with less interference).

The presence of this feature can be detected by checking for the
presence of the `CAP_CHANNEL_MONITOR` capability in `PROP_CAPS`.

## Properties

### PROP 4614: SPINEL_PROP_CHANNEL_MONITOR_SAMPLE_INTERVAL (#prop-channel-monitor-sample-interval)

 * Type: Read-Only
 * Packing-Encoding: `L`

If channel monitoring is enabled and active, every sample interval, a
zero-duration Energy Scan is performed, collecting a single RSSI sample
per channel. The RSSI samples are compared with a pre-specified RSSI
threshold.

### PROP 4615: SPINEL_PROP_CHANNEL_MONITOR_RSSI_THRESHOLD (#prop-channel-monitor-rssi-threshold)

 * Type: Read-Only
 * Packing-Encoding: `c`

This value specifies the threshold used by channel monitoring
module. Channel monitoring maintains the average rate of RSSI
samples that are above the threshold within (approximately) a
pre-specified number of samples (sample window).

### PROP 4616: SPINEL_PROP_CHANNEL_MONITOR_SAMPLE_WINDOW (#prop-channel-monitor-sample-window)

 * Type: Read-Only
 * Packing-Encoding: `L`

The averaging sample window length (in units of number of channel
samples) used by channel monitoring module. Channel monitoring will
sample all channels every sample interval. It maintains the average rate
of RSSI samples that are above the RSSI threshold within (approximately)
the sample window.

### PROP 4617: SPINEL_PROP_CHANNEL_MONITOR_SAMPLE_COUNT (#prop-channel-monitor-sample-count)

 * Type: Read-Only
 * Packing-Encoding: `L`

Total number of RSSI samples (per channel) taken by the channel
monitoring module since its start (since Thread network interface
was enabled).

### PROP 4618: SPINEL_PROP_CHANNEL_MONITOR_CHANNEL_OCCUPANCY (#prop-channel-monitor-channel-occupancy)

 * Type: Read-Only
 * Packing-Encoding: `A(t(cU))`

Data per item is:

 *  `C`: Channel
 *  `U`: Channel occupancy indicator

The channel occupancy value represents the average rate/percentage of
RSSI samples that were above RSSI threshold ("bad" RSSI samples) within
(approximately) latest sample window RSSI samples.

Max value of `0xffff` indicates all RSSI samples were above RSSI
threshold (i.e. 100% of samples were "bad").
