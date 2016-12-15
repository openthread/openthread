# Feature: True Random Number Generation {#feature-trng}

This feature allows the host to have access to any strong hardware
random number generator that might be present on the NCP, for things
like key generation or seeding PRNGs.

Support for this feature can be determined by the presence of `CAP_TRNG`.

Note well that implementing a cryptographically-strong software-based true
random number generator (that is impervious to things like temperature
changes, manufacturing differences across devices, or unexpected output
correlations) is non-trivial without a well-designed, dedicated hardware
random number generator. Implementors who have little or no experience in
this area are encouraged to not advertise this capability.

## Properties ##

### PROP 4101: PROP_TRNG_32 ###

*   Argument-Encoding: `L`
*   Type: Read-Only

Fetching this property returns a strong random 32-bit integer that is suitable
for use as a PRNG seed or for cryptographic use.

While the exact mechanism behind the calculation of this value is
implementation-specific, the implementation must satisfy the following
requirements:

* Data representing at least 32 bits of fresh entropy (extracted from the
  primary entropy source) MUST be consumed by the calculation of each query.
* Each of the 32 bits returned MUST be free of bias and have no statistical
  correlation to any part of the raw data used for the calculation of any
  query.

Support for this property is REQUIRED if `CAP_TRNG` is included in the
device capabilities.

### PROP 4102: PROP_TRNG_128 ###

*   Argument-Encoding: `D`
*   Type: Read-Only

Fetching this property returns 16 bytes of strong random data suitable for
direct cryptographic use without further processing(For example, as an
AES key).

While the exact mechanism behind the calculation of this value is
implementation-specific, the implementation must satisfy the following
requirements:

* Data representing at least 128 bits of fresh entropy (extracted from the
  primary entropy source) MUST be consumed by the calculation of each query.
* Each of the 128 bits returned MUST be free of bias and have no statistical
  correlation to any part of the raw data used for the calculation of any
  query.

Support for this property is REQUIRED if `CAP_TRNG` is included in the
device capabilities.

### PROP 4103: PROP_TRNG_RAW_32 ###

*   Argument-Encoding: `D`
*   Type: Read-Only

This property is primarily used to diagnose and debug the behavior
of the entropy source used for strong random number generation.

When queried, returns the raw output from the entropy source used to
generate `PROP_TRNG_32`, prior to any reduction/whitening and/or mixing
with prior state.

The length of the returned buffer is implementation specific and should be
expected to be non-deterministic.

Support for this property is RECOMMENDED if `CAP_TRNG` is included in the
device capabilities.
