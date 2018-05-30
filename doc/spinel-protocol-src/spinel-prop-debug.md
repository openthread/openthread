## Debug Properties {#prop-debug}

### PROP 16384: PROP_DEBUG_TEST_ASSERT {#prop-debug-test-assert}
* Type: Read-Only
* Packed-Encoding: `b`

Reading this property will cause an assert on the NCP. This
is intended for testing the assert functionality of
underlying platform/NCP. Assert should ideally cause the
NCP to reset, but if `assert` is not supported or disabled
boolean value of `false` is returned in response.

### PROP 16385: PROP_DEBUG_NCP_LOG_LEVEL {#prop-debug-ncp-log-level}
* Type: Read-Write
* Packed-Encoding: `C`

Provides access to the NCP log level. Currently defined values are (which follows
the RFC 5424):

 *  0: Emergency (emerg).
 *  1: Alert (alert).
 *  2: Critical (crit).
 *  3: Error (err).
 *  4: Warning (warn).
 *  5: Notice (notice).
 *  6: Information (info).
 *  7: Debug (debug).

If the NCP supports dynamic log level control, setting this property
changes the log level accordingly. Getting the value returns the current
log level.  If the dynamic log level control is not supported, setting this
property returns a `PROP_LAST_STATUS` with `STATUS_INVALID_COMMAND_FOR_PROP`.

### PROP 16386: PROP_DEBUG_TEST_WATCHDOG {#prop-debug-test-watchdog}
* Type: Read-Only
* Packed-Encoding: Empty

Reading this property will causes NCP to start `while(true) ;` loop and
thus triggering a watchdog. This is intended for testing the watchdog
functionality on the underlying platform/NCP.