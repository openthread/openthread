## Debug Properties {#prop-debug}

### PROP 16384: SPINEL_PROP_DEBUG_TEST_ASSERT {#prop-debug-test-assert}
* Type: Read-Only
* Packed-Encoding: `b`

Reading this property will cause an assert on the NCP. This
is intended for testing the assert functionality of
underlying platform/NCP. Assert should ideally cause the
NCP to reset, but if `assert` is not supported or disabled
boolean value of `false` is returned in response.
