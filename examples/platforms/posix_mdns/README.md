# OpenThread Standalone mDNS Library

The OpenThread core provides a native mDNS implementation. This is primarily intended for use as part of the Thread stack and Border Router functions, such as SRP advertising or discovery proxy modules. However, there is interest in allowing this mDNS implementation to be used and integrated into other projects without including the full Thread stack.

The OpenThread standalone mDNS library enables this by building a new library, `libopenthread-mdns.a`, which includes mDNS and all its related modules and can be integrated into other projects.

The new `libopenthread-mdns.a` library is built along with other high-level library options, such as the FTD Thread stack (`libopenthread-ftd.a`), MTD (`libopenthread-mtd.a`), and RCP (`libopenthread-radio.a`).

In CMake build files, the option `OT_MDNS_LIB` controls whether the `libopenthread-mdns.a` library is built (similar to the `OT_FTD` or `OT_MTD` options corresponding to the FTD/MTD `libopenthread` libraries). This option is enabled by default.

The `BUILD.gn` build files also always generate `libopenthread-mdns` as a static library.

## Public APIs

OpenThread defines public APIs (a set of C definitions and functions) to allow integrating projects to control its behavior. These are defined in the [`include/openthread/*.h`](https://github.com/openthread/openthread/tree/main/include/openthread) header files.

The `otMdns*` APIs are specified in the [`include/openthread/mdns.h`](https://github.com/openthread/openthread/blob/main/include/openthread/mdns.h) header file.

## Platform Requirements

OpenThread defines a platform abstraction in the header files [`include/openthread/platform/*.h`](https://github.com/openthread/openthread/tree/main/include/openthread/platform) (a set of C function APIs). The platform layer abstraction includes basic OS- or hardware-related functions such as alarm (timer), radio, non-volatile settings, logging, etc.

The platform APIs are expected to be provided by the platform layer when OpenThread is integrated into a project or product. This allows OpenThread to be used on a variety of devices running different operating systems and with varying hardware capabilities.

The OpenThread standalone mDNS library requires a subset of the platform APIs to be provided, as follows:

- `otPlatMdns*` APIs ([`openthread/platform/mdns_socket.h`](https://github.com/openthread/openthread/blob/main/include/openthread/platform/mdns_socket.h))
  - Provide an abstraction of a UDP socket for mDNS message transmission and reception.
- `otPlatAlarmMilli*` APIs ([`openthread/platform/alarm-milli.h`](https://github.com/openthread/openthread/blob/main/include/openthread/platform/alarm-milli.h)))
  - Used for millisecond timers (diagnostic-related functions are not needed).
- `otPlatCAlloc()` and `otPlatFree()` ([`openthread/platform/memory.h`](https://github.com/openthread/openthread/blob/main/include/openthread/platform/memory.h))
  - Provide heap-related functions.
- `otPlatEntropyGet()` ([`openthread/platform/entropy.h`](https://github.com/openthread/openthread/blob/main/include/openthread/platform/entropy.h))
  - Used to generate a seed for OpenThread's internal non-crypto random number generator.

In addition to the above (which are required), the following platform APIs may be provided based on whether the related feature is enabled and used:

- `otPlatLog()` ([`openthread/platform/logging.h`](https://github.com/openthread/openthread/blob/main/include/openthread/platform/logging.h))
  - If logging is enabled and delegated to the platform.
- `otPlatAssertFail()` ([`openthread/platform/misc.h`](https://github.com/openthread/openthread/blob/main/include/openthread/platform/misc.h))
  - If assert (within the OpenThread stack) is enabled and delegated to the platform.

Almost all OpenThread public and platform APIs expect an `otInstance *` input to be provided as the first input parameter. The `otInstance` is the object that maintains all the properties (variables) tracked and used by different OpenThread core modules.

The OpenThread core internally uses FIFO tasklets to schedule operations. The platform implementation must handle these tasklets by invoking the OpenThread tasklet API `otTaskletProcess()` (defined in [`openthread/tasklet.h`](https://github.com/openthread/openthread/blob/main/include/openthread/tasklet.h)). `otTaskletsArePending()` can be called to determine whether OpenThread has any pending scheduled tasklets.

To initialize and run the OpenThread stack, the platform should follow these steps:

1.  All platform modules (modules implemented by the platform providing `otPlat*`) must be initialized first. This is important to do before initializing the `otInstance`, since platform APIs are used by the OpenThread stack during its initialization.
2.  The `otInstance` is initialized next. For example, `otInstanceInitSingle()` can be used (when a single instance is used).
3.  In the main run-loop (waiting for events):
    - Handle all pending tasklets by calling `otTaskletProcess()`.
    - Process and handle any platform drivers (alarm, mDNS sockets, etc.). During this, the platform implementation can invoke `otPlat` callbacks to notify the OpenThread core, for example, `otPlatAlarmMilliFired` to signal the alarm fired, or `otPlatMdnsHandleReceive()` to signal a received mDNS message.

**Important note:** The OpenThread stack is not re-entrant. OpenThread public and platform APIs and callbacks MUST be called from the same OS context (e.g., the same thread/process or the same task in an RTOS).

## OpenThread Configs

OpenThread defines a set of build-time `OPENTHREAD_CONFIG_*` options to configure the OpenThread stack and its features. The full set of configurations (along with their documentation and default values) can be found in the [`src/core/config/*.h`](https://github.com/openthread/openthread/blob/main/src/core/config) header files.

A smaller subset of these configs is relevant when building the OpenThread standalone mDNS library. For an example core config header file of this, see `examples/config/ot-core-config-mdns.h`.

### mDNS Configs

- `OPENTHREAD_CONFIG_MULTICAST_DNS_PUBLIC_API_ENABLE` must be enabled to provide the public mDNS APIs.
- `OPENTHREAD_CONFIG_MULTICAST_DNS_ENTRY_ITERATION_API_ENABLE` controls whether additional public APIs (for iterating over browsers/resolvers) are provided. It can be disabled if these APIs are not used to reduce code (binary) size.
- `OPENTHREAD_CONFIG_MULTICAST_DNS_DEFAULT_QUESTION_UNICAST_ALLOWED` controls whether the OpenThread mDNS is allowed to use QU questions. it is recommended to enable this, unless multiple mDNS entities are running on the same device.

### Heap Configs

- `OPENTHREAD_CONFIG_HEAP_EXTERNAL_ENABLE` is required since the OpenThread mDNS module uses the heap.

### Message Pool Configs

- `OPENTHREAD_CONFIG_MESSAGE_USE_HEAP_ENABLE` is recommended, since the heap is required for the OpenThread mDNS implementation. This ensures that `otMessage` instances also use the heap and not a pre-allocated message pool.
- `OPENTHREAD_CONFIG_PLATFORM_MESSAGE_MANAGEMENT` (which delegates message buffer allocation to the platform) must not be used.

### Timer Configs

- `OPENTHREAD_CONFIG_PLATFORM_USEC_TIMER_ENABLE` is not needed. OpenThread mDNS requires millisecond timers. The microsecond timer support is not needed and can be disabled.
- `OPENTHREAD_CONFIG_UPTIME_ENABLE` is not directly used by the OpenThread mDNS module but is recommended when logging is enabled to provide timestamps in the logs.

### Assert Configs

- `OPENTHREAD_CONFIG_ASSERT_ENABLE` enables asserts within the OpenThread core. Disabling this can reduce the code (binary) size.
- `OPENTHREAD_CONFIG_PLATFORM_ASSERT_MANAGEMENT` delegates assert implementation to the platform, requiring `otPlatAssertFail()` to be provided by the platform. If asserts are enabled, providing and using a platform-specific `otPlatAssertFail()` is recommended.
- `OPENTHREAD_CONFIG_ASSERT_CHECK_API_POINTER_PARAM_FOR_NULL` adds assert checks to ensure pointer-type API input parameters are not `nullptr`. Enabling this can significantly increase the code (binary) size due to the numerous assert checks added for all API pointer parameters. Therefore, using this feature is recommended only during debugging/development.

### Logging Configs

- `OPENTHREAD_CONFIG_LOG_OUTPUT` - Recommended to be set to `PLATFORM_DEFINED`, requiring the platform to provide `otPlatLog()`. It can also be set to `OPENTHREAD_CONFIG_LOG_OUTPUT_NONE` to disable all logs (which can reduce code (binary) size).
- `OPENTHREAD_CONFIG_LOG_LEVEL` - Recommended to be set to `OT_LOG_LEVEL_INFO` during development and testing. It can be set to `OT_LOG_LEVEL_NONE` to disable all logs (except platform-specific logs).
- `OPENTHREAD_CONFIG_LOG_PREPEND_UPTIME` - Recommended to be enabled (adding timestamps to logs).

## Platform Example

This (`/examples/platforms/posix_mdns`) directory contains a functional example of the platform implementation on POSIX.

This platform example can be used to build an executable, `ot-cli-mdns`, which can then be run as an mDNS entity on a computer. It provides OpenThread CLI mDNS-related commands (mapping to the mDNS public APIs).

- To build this executable, the test script `./tests/toranj/build.sh mdns` can be used (the executable will be located at `./examples/apps/cli/ot-cli-mdns`).
- Alternatively, `./script/cmake-build posix_mdns` can be used (the file will be located at `./build/posix_mdns/examples/apps/cli/ot-cli-mdns`).

Note that this build uses the `examples/config/ot-core-config-mdns.h` configuration.

Example CLI commands:

```
$ ./examples/apps/cli/ot-cli-mdns

ot-mdns>
ot-mdns> enable 15 # netif index 15
Done

ot-mdns> register service test _tst._udp myhost 1234
Service test for _tst._udp
    host: myhost
    port: 1234
    priority: 0
    weight: 0
    ttl: 0
    txt-data: (empty)

Done

ot-mdns> services
Service test for _tst._udp
    host: myhost
    port: 1234
    priority: 0
    weight: 0
    ttl: 120
    txt-data: 00
    state: registered
Done

ot-mdns> browser start _tst._udp
Done
mDNS browse result for _tst._udp
    instance: test
    ttl: 120
    if-index: 15

ot-mdns> srvresolver start test _tst._udp
mDNS SRV result for test for _tst._udp
    host: myhost
    port: 1234
    priority: 0
    weight: 0
    ttl: 120
    if-index: 15
Done

```
