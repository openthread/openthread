# Implement Platform Abstraction Layer APIs

OpenThread is OS and platform agnostic, with a narrow Platform Abstraction Layer
(PAL). This PAL defines:

<figure class="attempt-right">
<img src="/guides/images/ot-arch-porting.png" srcset="/guides/images/ot-arch-porting.png 1x, /guides/images/ot-arch-porting_2x.png 2x" border="0" alt="Porting Architecture" />
</figure>

-   Alarm interface for free-running timer with alarm
-   Bus interfaces (UART, SPI) for communicating CLI and Spinel messages
-   Radio interface for IEEE 802.15.4-2006 communication
-   GCC-specific initialization routines
-   Entropy for true random number generation
-   Settings service for non-volatile configuration storage
-   Logging interface for delivering OpenThread log messages
-   System-specific initialization routines

All APIs should be implemented based on the underlying Hardware Abstraction
Layer (HAL) Build Support Package (BSP).

> Key Point: Unless noted as optional, **Platform Abstraction Layer APIs are
mandatory** and must be implemented according to the definitions in each API
header file.

API files should be placed in the following directories:

Type | Directory
------|------
Platform-specific PAL implementation | `/openthread/examples/platforms/{platform-name}`
Header files — Non-volatile storage API | `/openthread/examples/platforms/utils`
All other header files | `/openthread/include/openthread/platform`
HAL BSP | `/openthread/third_party/{platform-name}`

## Step 1: Alarm

API declaration:

[`/openthread/include/openthread/platform/alarm-milli.h`](https://github.com/openthread/openthread/blob/main/include/openthread/platform/alarm-milli.h)

The Alarm API provides fundamental timing and alarm services for the upper layer
timer implementation.

There are two alarm service types,
[millisecond](https://github.com/openthread/openthread/blob/main/include/openthread/platform/alarm-milli.h)
and [microsecond](https://github.com/openthread/openthread/blob/main/include/openthread/platform/alarm-micro.h).
Millisecond is required for a new hardware platform. Microsecond is optional.

## Step 2: UART

> Note: This API is optional.

API declaration:

[`/openthread/examples/platforms/utils/uart.h`](https://github.com/openthread/openthread/blob/main/examples/platforms/utils/uart.h)

The UART API implements fundamental serial port communication via the UART
interface.

While the OpenThread
[CLI](https://github.com/openthread/openthread/tree/main/examples/apps/cli)
and [NCP](https://github.com/openthread/openthread/tree/main/examples/apps/ncp)
add-ons depend on the UART interface to interact with the host side, UART API
support is optional. However, even if you do not plan to use these add-ons on
your new hardware platform example, we highly recommend you add support for a
few reasons:

-   The CLI is useful for validating that the port works correctly
-   The Harness Automation Tool uses the UART interface to control OpenThread for testing and certification purposes

If the target hardware platform supports a USB CDC module rather than UART, make
sure to:

-   Install the correct USB CDC driver on the host side
-   Replace the UART API implementation with the USB CDC driver (along with BSP)
    on the OpenThread side, using the same function prototypes

## Step 3: Radio

API declaration:

[`/openthread/include/openthread/platform/radio.h`](https://github.com/openthread/openthread/blob/main/include/openthread/platform/radio.h)

The Radio API defines all necessary functions called by the upper IEEE 802.15.4
MAC layer. The Radio chip must be fully compliant with the 2.4GHz IEEE
802.15.4-2006 specification.

Due to its enhanced low power feature, OpenThread requires all platforms to
implement auto frame pending (indirect transmission) by default, and the source
address match table should also be implemented in the `radio.h` source file.

However, if your new hardware platform example is resource limited, the source
address table can be defined as zero length. See
[Auto Frame Pending](#auto-frame-pending) for more information.

> Note: The `otPlatRadioGetIeeeEui64` Radio API **MUST** return a unique
administered factory-assigned IEEE EUI-64 that includes the manufacturer's OUI.
The EUI-64 is used to match to steering data during the Joiner Discovery phase.

## Step 4: Misc/Reset

API declaration:

[`/openthread/include/openthread/platform/misc.h`](https://github.com/openthread/openthread/blob/main/include/openthread/platform/misc.h)

The Misc/Reset API provides a method to reset the software on the chip, and
query the reason for last reset.

## Step 5: Entropy

> Note: **Implementation of this API is required in every port of OpenThread.**

API declaration:

[`/openthread/include/openthread/platform/entropy.h`](https://github.com/openthread/openthread/blob/main/include/openthread/platform/entropy.h)

The Entropy API provides a true random number generator (TRNG) for the upper
layer, which is used to maintain security assets for the entire OpenThread
network. The API should guarantee that a new random number is generated for
each function call. Security assets affected by the TRNG include:

-   AES CCM nonce
-   Random delayed jitter
-   Devices' extended address
-   The initial random period in the trickle timer
-   CoAP token/message IDs

Note that many platforms have already integrated a random number generator,
exposing the API in its BSP package. In the event that the target hardware
platform does not support TRNG, consider leveraging ADC module sampling to
generate a fixed-length random number. Sample over multiple iterations if
necessary to meet the TRNG requirements (uint32_t).

When the macro `MBEDTLS_ENTROPY_HARDWARE_ALT` is set to `1`, this API should
also provide a method to generate the hardware entropy used in the mbedTLS
library.

## Step 6: Non-volatile storage

> Note: **Only _one_ of these APIs is required to be implemented in every port
of OpenThread.**

API declarations:

[`/openthread/include/openthread/platform/flash.h`](https://github.com/openthread/openthread/blob/main/include/openthread/platform/flash.h)

**or**

[`/openthread/include/openthread/platform/settings.h`](https://github.com/openthread/openthread/blob/main/include/openthread/platform/settings.h)

The Non-volatile storage requirement can be satisfied by implementing one of the
two APIs listed above. The Flash API implements a flash storage driver, while
the Settings API provides functions for an underlying flash operation
implementation to the upper layer.

These APIs expose to the upper layer:

-   The available non-volatile storage size used to store application data (for
    example, active/pending operational dataset, current network parameters and
    thread devices' credentials for reattachment after reset)
-   Read, write, erase, and query flash status operations

Use `OPENTHREAD_CONFIG_PLATFORM_FLASH_API_ENABLE` in your platform example's
core config file to indicate which API the platform should use. If set to `1`,
the Flash API must be implemented. Otherwise, the Settings API must be
implemented.

This flag must be set in your
`/openthread/examples/platforms/{platform-name}/openthread-core-{platform-name}-config.h`
file.

## Step 7: Logging

> Note:  This API is optional.

API declaration:

[`/openthread/include/openthread/platform/logging.h`](https://github.com/openthread/openthread/blob/main/include/openthread/platform/logging.h)

The Logging API implements OpenThread's logging and debug functionality, with
multiple levels of debug output available.  This API is optional if you do not
plan to utilize OpenThread's logging on your new hardware platform example.

The highest and most detailed level is `OPENTHREAD_LOG_LEVEL_DEBG`, which
prints all raw packet information and logs lines through the serial port or on
the terminal. Choose a debug level that best meets your needs.

## Step 8: System-specific

API declaration:

[`/openthread/examples/platforms/openthread-system.h`](https://github.com/openthread/openthread/blob/main/examples/platforms/openthread-system.h)

The System-specific API primarily provides initialization and deinitialization
operations for the selected hardware platform. This API is not called by the
OpenThread library itself, but may be useful for your system/RTOS. You can also
implement the initialization of other modules (for example, UART, Radio, Random,
Misc/Reset) in this source file.

Implementation of this API depends on your use case. If you wish to use the
generated [CLI and NCP applications](https://openthread.io/guides/build#binaries) for an [example
platform](https://github.com/openthread/openthread/tree/main/examples/platforms),
you must implement this API. Otherwise, any API can be implemented to integrate
the example platform drivers into your system/RTOS.
