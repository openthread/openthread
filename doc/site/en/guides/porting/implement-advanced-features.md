# Implement Advanced Features

Some advanced features are optional, depending on whether or not they are
supported on the target hardware platform.

<a id="auto-frame-pending"></a>

## Step 1: Auto frame pending

IEEE 802.15.4 defines two kinds of data transmission methods between parent and
child: direct transmission and indirect transmission. The latter is designed
primarily for sleepy end devices (SEDs) which sleep most of the time,
periodically waking to poll the parent for queued data.

-   **Direct Transmission** — parent sends a data frame directly to the end device
    <img src="/guides/images/ot-auto-frame-direct.png" srcset="/guides/images/ot-auto-frame-direct.png 1x, /guides/images/ot-auto-frame-direct_2x.png 2x" border="0" alt="Direct Transmission" width="400" />

-   **Indirect Transmission** — parent holds data until requested by its intended end device
    <img src="/guides/images/ot-auto-frame-indirect.png" srcset="/guides/images/ot-auto-frame-indirect.png 1x, /guides/images/ot-auto-frame-indirect_2x.png 2x" border="0" alt="Direct Transmission" width="400" />

In the Indirect case, a child end device must first poll the parent to determine
whether any data is available for it. To do this, the child sends a data
request, which the parent acknowledges. The parent then determines whether it
has any data for the child device; if so, it sends a data packet to the child
device, which acknowledges receipt of the data.

If the radio supports dynamically setting the Frame Pending bit in outgoing
acknowledgments to SEDs, the drivers must implement the
[source address match](https://github.com/openthread/openthread/blob/master/include/openthread/platform/radio.h#L288)
API to enable this capability. OpenThread uses this API to tell the radio which
SEDs to set the Frame Pending bit for.

If the radio does not support dynamically setting the Frame Pending bit, the
radio might stub out the source address match API to return
`OT_ERROR_NOT_IMPLEMENTED`.

## Step 2: Energy Scan/Detect with radio

> Note: This feature is optional.

The Energy Scan/Detect feature requires the radio chip to sample energy
presenting on selected channels and return the detected energy value to the
upper layer.

If this feature is not implemented, the IEEE 802.15.4 MAC layer
sends/receives a Beacon Request/Response packet to evaluate the current
energy value on the channel.

If the radio chip supports Energy Scan/Detect, make sure to disable software
energy scanning logic by setting the macro
`OPENTHREAD_CONFIG_ENABLE_SOFTWARE_ENERGY_SCAN = 0`.

## Step 3: Hardware acceleration for mbedTLS

> Note:  This feature is optional.

mbedTLS defines several macros in the main configuration header file,
[`mbedtls-config.h`](https://github.com/openthread/openthread/blob/master/third_party/mbedtls/mbedtls-config.h),
to allow users to enable alternative implementations of AES, SHA1, SHA2, and
other modules, as well as individual functions for the Elliptic curve
cryptography (ECC) over GF(p) module. See
[mbedTLS hardware acceleration](https://docs.mbed.com/docs/mbed-os-handbook/en/latest/advanced/tls_hardware_acceleration/)
for more information.

OpenThread does not enable those macros, therefore the symmetric crypto
algorithms, hash algorithms and ECC functions all utilize software
implementations by default. These implementations require considerable memory
and computational resources. For optimal performance and a better user
experience, we recommend enabling hardware acceleration instead of software to
implement the above operations.

To enable hardware acceleration within OpenThread, the following mbedTLS
configuration header files should be added to the compile option flags in the
platform example's Makefile:

-   Main configuration, which defines all necessary macros used in OpenThread:
    `/openthread/third-party/mbedtls/mbedtls-config.h`
-   User-specific configuration, which defines alternate implementations of
    modules and functions: `/openthread/examples/platforms/{platform-name}/crypto/{platform-name}-mbedtls-config.h`

Example:

```
EFR32_MBEDTLS_CPPFLAGS  = -DMBEDTLS_CONFIG_FILE='\"mbedtls-config.h\"'
EFR32_MBEDTLS_CPPFLAGS += -DMBEDTLS_USER_CONFIG_FILE='\"efr32-mbedtls-config.h\"'
```

Commented macros in `mbedtls-config.h` are not mandatory, and can be enabled in
the user-specific configuration header file for hardware acceleration.

For a complete example user-specific configuration, see the
[`mbedtls_config_autogen.h`](https://github.com/openthread/openthread/blob/master/examples/platforms/efr32/efr32mg12/crypto/mbedtls_config_autogen.h)
file.

### AES module

OpenThread Security applies AES CCM (Counter with CBC-MAC) crypto to
encrypt/decrypt the IEEE 802.15.4 or MLE messages and validates the message
integration code. Hardware acceleration should at least support basic AES ECB
(Electronic Codebook Book) mode for AES CCM basic functional call.

To utilize an alternative AES module implementation:

1.  Define the `MBEDTLS_AES_ALT` macro in the user-specific mbedTLS
    configuration header file
1.  Indicate the path of the `aes_alt.h` file using the `MBEDTLS_CPPFLAGS`
    variable

### SHA256 module

OpenThread Security applies HMAC and SHA256 hash algorithms to calculate the
hash value for master key management and PSKc generation according to the Thread
Specification.

To utilize an alternative basic SHA256 module implementation:

1.  Define the `MBEDTLS_SHA256_ALT` macro in the user-specific mbedTLS
    configuration header file
1.  Indicate the path of the `sha256_alt.h` file using the `MBEDTLS_CPPFLAGS`
    variable

### ECC functions

Since mbedTLS currently only supports hardware acceleration for parts of ECC
functions, rather than the entire module, you can choose to implement some
functions defined in
`{path-to-mbedtls}/library/ecp.c` to accelerate ECC
point multiplication.

Curve secp256r1 is used in the key exchange algorithm of the
[ECJPAKE](https://tools.ietf.org/html/draft-cragie-tls-ecjpake-00) draft. Hence,
hardware acceleration should at least support the secp256r1 short weierstrass
curve operation. See [SiLabs CRYPTO Hardware Acceleration for
mbedTLS](https://siliconlabs.github.io/Gecko_SDK_Doc/mbedtls/html/group__sl__crypto.html)
for an example.
